#include <Processors/QueryPlan/QueryResultCachingStep.h>
#include <Processors/Transforms/QueryResultCachingTransform.h>
#include <QueryPipeline/QueryPipelineBuilder.h>

namespace DB
{

namespace {

ITransformingStep::Traits getTraits()
{
    return ITransformingStep::Traits
        {
            {
                .preserves_distinct_columns = true,
                .returns_single_stream = true,
                .preserves_number_of_streams = true,
                .preserves_sorting = true,
            },
            {
                .preserves_number_of_rows = true,
            }
        };
}

}

QueryResultCachingStep::QueryResultCachingStep(const DataStream & input_stream_, QueryResultCachePtr cache_, QueryResultCache::Key cache_key_)
    : ITransformingStep(input_stream_, input_stream_.header, getTraits())
    , cache(cache_)
    , cache_key(cache_key_)
{
}

void QueryResultCachingStep::transformPipeline(QueryPipelineBuilder & pipeline, const BuildQueryPipelineSettings &)
{
    // only returns true for one thread, so that multiple threads are not caching the same query result at the same time
    pipeline.addSimpleTransform(
        [&](const Block & header, QueryPipelineBuilder::StreamType) -> ProcessorPtr
        { return std::make_shared<QueryResultCachingTransform>(header, cache, cache_key); });
}

}
