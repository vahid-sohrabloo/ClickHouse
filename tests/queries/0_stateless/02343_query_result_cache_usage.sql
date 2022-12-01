-- { echoOn }

SYSTEM DROP QUERY RESULT CACHE;

DROP TABLE IF EXISTS t;
CREATE TABLE t (n UInt32) ENGINE=Memory;
INSERT INTO t VALUES (1);

SET min_query_runs_for_query_result_cache = 0;

-- no caching at all
SET experimental_query_result_cache_active_usage = false;
SET experimental_query_result_cache_passive_usage = false;
SELECT n from t;
SELECT * FROM system.queryresult_cache;

-- try use cache when it is empty
SET experimental_query_result_cache_active_usage = false;
SET experimental_query_result_cache_passive_usage = true;
SELECT n from t;
SELECT * FROM system.queryresult_cache;

-- put query result into cache
SET experimental_query_result_cache_active_usage = true;
SET experimental_query_result_cache_passive_usage = false;
SELECT n from t;
SELECT * FROM system.queryresult_cache;

-- put query result in cache and access it in further queries
SET experimental_query_result_cache_active_usage = true;
SET experimental_query_result_cache_passive_usage = true;
SELECT n from t;
-- cache has now three entries:
-- 1. the previous SELECT n from t;
-- 2. the current SELECT n from t; since the (changed) configuration is part of the cache key
-- 3. SELECT * FROM system.queryresult_cache; this should not happen
SELECT * FROM system.queryresult_cache;

DROP TABLE t;

-- { echoOff }
