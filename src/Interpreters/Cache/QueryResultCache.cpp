#include "Interpreters/Cache/QueryResultCache.h"
#include <Processors/Sources/SourceFromSingleChunk.h>
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

String QueryResultCache::Key::astToQueryString() const
{
    WriteBufferFromOwnString buf;
    IAST::FormatSettings format_settings(buf, /*one_line*/ true);
    ast->format(format_settings);
    return buf.str();
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

size_t QueryResultCache::WeightFunction::operator()(const Chunk & chunk) const
{
    return chunk.allocatedBytes();
}

QueryResultCache::Reader::Reader(const Cache & cache_, Key key)
{
    std::shared_ptr<Chunk> entry = cache_.get(key);

    if (!entry)
    {
        LOG_DEBUG(&Poco::Logger::get("QueryResultCache::Reader"), "Found no cache entry with the given cache key");
        return;
    }

    LOG_DEBUG(&Poco::Logger::get("QueryResultCache::Reader"), "Found a cache entry with the given cache key");

    pipe = Pipe(std::make_shared<SourceFromSingleChunk>(key.header, entry->clone()));
}

bool QueryResultCache::Reader::containsResult() const
{
    return !pipe.empty();
}

Pipe && QueryResultCache::Reader::getPipe()
{
    return std::move(pipe);
}

QueryResultCache::Writer::Writer(Cache & cache_, Key key_)
    : cache(cache_)
    , key(key_)
    , can_insert(cache.get(key) == nullptr)
{
}

QueryResultCache::Writer::~Writer()
try
{
    if (!can_insert)
        return;

    auto to_single_chunk = [](const Chunks& chunks_)
    {
        if (chunks_.empty())
            return Chunk();

        auto result_columns = chunks_[0].clone().mutateColumns();
        for (size_t i = 1; i != chunks_.size(); ++i)
        {
            const Columns & columns = chunks_[i].getColumns();
            // TODO use Chunk::append()
            for (size_t j = 0; j != columns.size(); ++j)
                result_columns[j]->insertRangeFrom(*columns[j], 0, columns[j]->size());
        }
        const size_t num_rows = result_columns[0]->size();
        return Chunk(std::move(result_columns), num_rows);
    };

    auto entry = std::make_shared<Chunk>(to_single_chunk(chunks));
    cache.set(key, entry);

    LOG_DEBUG(&Poco::Logger::get("QueryResultCache::Writer"), "Stored key with header = {} and weight = {}",
              key.header.getNamesAndTypesList().toString(), WeightFunction()(*entry));
}
catch (const std::exception &)
{
}

void QueryResultCache::Writer::insertChunk(Chunk && chunk)
{
    if (!can_insert)
        return;

    chunks.push_back(std::move(chunk));

    size_t weight = 0;
    for (const auto & c : chunks)
        weight += c.allocatedBytes();

    if (weight > key.settings.query_result_cache_max_entry_size)
        can_insert = false;
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
