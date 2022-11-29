#pragma once

#include <Processors/ISimpleTransform.h>
#include <Interpreters/Cache/QueryResultCache.h>

namespace DB
{

class QueryResultCachingTransform : public ISimpleTransform
{
public:
    QueryResultCachingTransform(const Block & header_, QueryResultCachePtr cache, QueryResultCache::Key cache_key);
    String getName() const override { return "QueryResultCachingTransform"; }

protected:
    void transform(Chunk & chunk) override;

private:
    QueryResultCache::Writer cache_writer;
};

}
