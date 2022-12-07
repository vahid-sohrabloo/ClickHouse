#include <Processors/Transforms/QueryResultCachingTransform.h>

namespace DB
{

QueryResultCachingTransform::QueryResultCachingTransform(const Block & header_, QueryResultCachePtr cache, QueryResultCache::Key cache_key)
    : ISimpleTransform(header_, header_, false)
    , cache_writer(cache->getWriter(cache_key))
{
}

void QueryResultCachingTransform::transform([[maybe_unused]] Chunk & chunk)
{
    /// cache_writer.insertChunk(chunk.clone());
}

};
