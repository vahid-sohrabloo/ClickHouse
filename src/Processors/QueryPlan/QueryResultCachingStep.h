#pragma once

#include <Processors/QueryPlan/ITransformingStep.h>
#include <Interpreters/Cache/QueryResultCache.h>


namespace DB
{

class QueryResultCachingStep : public ITransformingStep
{
public:
    QueryResultCachingStep(const DataStream & input_stream_, QueryResultCachePtr cache_, QueryResultCache::Key cache_key_);
    String getName() const override { return "QueryResultCachingStep"; }
    void transformPipeline(QueryPipelineBuilder & pipeline, const BuildQueryPipelineSettings &) override;
    void updateOutputStream() override {}

private:
    QueryResultCachePtr cache;
    QueryResultCache::Key cache_key;
};

}
