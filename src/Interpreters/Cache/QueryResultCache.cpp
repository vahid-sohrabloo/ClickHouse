#include "Interpreters/Cache/QueryResultCache.h"

#include <Common/SipHash.h>

namespace DB
{

QueryResultCache::Key::Key(ASTPtr ast_, const Block & header_, const Settings & settings_, std::optional<String> username_)
    : ast(ast_)
    , header(header_)
    , settings(settings_)
    , username(std::move(username_))
{}

bool QueryResultCache::Key::operator==(const Key & other) const
{
    return ast->getTreeHash() == other.ast->getTreeHash()
        && blocksHaveEqualStructure(header, other.header)
        && settings == other.settings
        && username == other.username;
}

QueryResultCache::Entry::Entry(Chunks chunks_, bool is_writing_)
    : chunks(std::move(chunks_))
    , is_writing(is_writing_)
{
}

size_t QueryResultCache::KeyHasher::operator()(const Key & key) const
{
    SipHash hash;
    hash.update(key.ast->getTreeHash());
    hash.update(key.header.getNamesAndTypesList().toString()); // TODO replace by getNamesAndTypes (less allocations)
    for (const auto & setting : key.settings)
    {
        const String & value = setting.getValueString();
        hash.update(value); // TODO check 1. if it includes settings name and setting value, 2. if we hash *all* settings
    }
    if (key.username.has_value())
        hash.update(*key.username);
    auto res = hash.get64();
    return res;
}

size_t QueryResultCache::WeightFunction::operator()(const QueryResultCache::Entry & entry) const
{
    size_t weight = 0;
    for (const auto & chunk : entry.chunks)
        weight += chunk.allocatedBytes();
    return weight;
}

namespace {

Chunk toSingleChunk(const Chunks& chunks)
{
    if (chunks.empty())
        return {};

    auto result_columns = chunks[0].clone().mutateColumns();
    for (size_t i = 1; i != chunks.size(); ++i)
    {
        const Columns & columns = chunks[i].getColumns();
        // TODO use Chunk::append()
        for (size_t j = 0; j != columns.size(); ++j)
            result_columns[j]->insertRangeFrom(*columns[j], 0, columns[j]->size());
    }
    const size_t num_rows = result_columns[0]->size();
    return Chunk(std::move(result_columns), num_rows);
}

}

QueryResultCache::Reader::Reader(const Cache & cache_, Key key)
{
    std::shared_ptr<Entry> entry = cache_.get(key);

    if (entry == nullptr || entry->is_writing.load() == true)
    {
        LOG_DEBUG(&Poco::Logger::get("QueryResultCache::Reader"), "Found no cache entry with the given cache key");
        return;
    }
    LOG_DEBUG(&Poco::Logger::get("QueryResultCache::Reader"), "Found a cache entry with the given cache key");

    /// TODO Instead of saving chunk*s* as cache entry, we could save a single chunk and save their concatenation. Probably faster as theS
    ///      cache is read more often than written, but depends on the workload.
    pipe = Pipe(std::make_shared<SourceFromSingleChunk>(key.header, toSingleChunk(entry->chunks)));
}

bool QueryResultCache::Reader::containsResult() const
{
    return !pipe.empty();
}

Pipe && QueryResultCache::Reader::getPipe()
{
    return std::move(pipe);
}

// TODO simplify logic in Writer, we should save only finished entries in the cache (i.e. remove Entry::is_writing), also: do we need
// "result"?

QueryResultCache::Writer::Writer(Cache & cache_, Key key_)
    : cache(cache_)
    , key(key_)
    , can_insert(false)
    , data(std::move(cache_.getOrSet(key,
                                     [&] {
                                             can_insert = true;
                                             return std::make_shared<Entry>(Chunks{}, true);
                                         }
                                     ).first))
    , result(std::make_shared<Entry>(Chunks{}, false))
{
}

QueryResultCache::Writer::~Writer()
try
{
    if (can_insert)
    {
        cache.set(key, result);
        LOG_DEBUG(&Poco::Logger::get("QueryResultCache::Writer"), "Stored key with header = {} and weight = {}",
                  key.header.getNamesAndTypesList().toString(), WeightFunction()(*result));
    }
}
catch (const std::exception &)
{
}

void QueryResultCache::Writer::insertChunk(Chunk && chunk)
{
    if (!can_insert)
        return;

    result->chunks.push_back(std::move(chunk));

    if (WeightFunction()(*result) > key.settings.query_result_cache_max_entry_size)
    {
        can_insert = false;
        cache.remove(key);
    }
}

QueryResultCache::QueryResultCache(size_t size_in_bytes)
    : cache(size_in_bytes, 0, "LRU")
{
}

QueryResultCache::Reader QueryResultCache::getReader(Key key)
{
    return Reader(cache, key);
}

QueryResultCache::Writer QueryResultCache::getWriter(Key key)
{
    return Writer(cache, key);
}

bool QueryResultCache::containsResult(Key key)
{
    return cache.get(key) != nullptr;
}

void QueryResultCache::reset()
{
    cache.reset();
}

size_t QueryResultCache::recordQueryRun(Key key)
{
    static std::unordered_map<Key, size_t, KeyHasher> times_executed;
    static std::mutex times_executed_mutex;
    static constexpr size_t TIMES_EXECUTED_MAX_SIZE = 10'000;

    std::lock_guard lock(times_executed_mutex);
    size_t times = ++times_executed[key];
    if (times_executed.size() > TIMES_EXECUTED_MAX_SIZE)
        times_executed.clear();
    return times;
}

}
