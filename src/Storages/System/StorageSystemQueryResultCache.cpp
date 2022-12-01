#include "StorageSystemQueryResultCache.h"
#include <DataTypes/DataTypeString.h>
#include <DataTypes/DataTypesNumber.h>
#include <Interpreters/Cache/QueryResultCache.h>
#include <Interpreters/Context.h>


namespace DB
{

NamesAndTypesList StorageSystemQueryResultCache::getNamesAndTypes()
{
    return {
        {"query", std::make_shared<DataTypeString>()},
        {"query_hash", std::make_shared<DataTypeUInt64>()},
        {"result_size", std::make_shared<DataTypeUInt64>()}
    };
}

StorageSystemQueryResultCache::StorageSystemQueryResultCache(const StorageID & table_id_)
    : IStorageSystemOneBlock(table_id_)
{
}

void StorageSystemQueryResultCache::fillData(MutableColumns & res_columns, ContextPtr context, const SelectQueryInfo &) const
{
    auto query_result_cache = context->getQueryResultCache();

    const auto & cache_content = query_result_cache->cache.dump();

    for (const auto & key_entry : cache_content)
    {
        const QueryResultCache::Key & key = key_entry.first;
        const Chunk & chunk = *key_entry.second;

        res_columns[0]->insert(key.astToQueryString());       /// approximation of original query string
        res_columns[1]->insert(key.ast->getTreeHash().first);
        res_columns[2]->insert(chunk.allocatedBytes());
    }
}

}
