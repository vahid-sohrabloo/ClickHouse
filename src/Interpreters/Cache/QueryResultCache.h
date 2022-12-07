#pragma once

#include <base/defines.h>
#include <Common/CacheBase.h>
#include <Core/Block.h>
#include <Core/Settings.h>
#include <Parsers/IAST.h>
#include <Processors/Chunk.h>
#include <QueryPipeline/Pipe.h>

namespace DB
{

// TODO:
// - data invaliation
// - blocklist of functions: getDict(), UDFs, RBAC, merges with collapsing, getHostname, ...
//   block read from system tables
// - The MySQL query result cache (which they killed in the meantime) excluded queries with non-deterministic functions
//   (https://dev.mysql.com/doc/refman/5.6/en/query-cache-operation.html). I think we should do that as well (and additionally I'd also put
//   all encryption/decryption functions on the blocklist so that we never store cleartext in the cache - this could get funny with user
//   sharing).
// - think about key: query string, AST hash, serialized query plan hash?
// - add doxygen

class QueryResultCache
{
    friend class StorageSystemQueryResultCache;

public:
    struct Key
    {
        Key(ASTPtr ast_, const Block & header_, const Settings & settings_, std::optional<String> username_);
        bool operator==(const Key & other) const;

        String astToQueryString() const;

        ASTPtr ast;
        Block header; // TODO why stored as part of Key? Does the AST not subsume that?
        Settings settings;
        std::optional<String> username;
    };

private:
    struct KeyHasher
    {
        size_t operator()(const Key & key) const;
    };

    struct WeightFunction
    {
        size_t operator()(const Chunk & chunk) const;
    };

    using Cache = CacheBase<Key, Chunk, KeyHasher, WeightFunction>;

public:
    class Reader
    {
    public:
        Reader(const Cache & cache_, Key key);
        bool containsResult() const;
        Pipe && getPipe();
    private:
        Pipe pipe;
    };

    class Writer
    {
    public:
        Writer(Cache & cache_, Key key_);
        ~Writer();
        void insertChunk(Chunk && chunk);
    private:
        QueryResultCache::Cache & cache;
        const Key key;
        Chunks chunks;
        bool can_insert;
    };

    explicit QueryResultCache(size_t size_in_bytes);

    Reader getReader(Key key);
    Writer getWriter(Key key);

    bool containsResult(Key key);
    void reset();

    /// Record new execution of query represented by key. Returns number of executions so far.
    static size_t recordQueryRun(Key key);

private:
    Cache cache;
};

using QueryResultCachePtr = std::shared_ptr<QueryResultCache>;

}
