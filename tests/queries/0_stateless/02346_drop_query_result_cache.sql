-- { echoOn }

DROP TABLE IF EXISTS t;
CREATE TABLE t (a UInt32, b UInt32) ENGINE=Memory;
INSERT INTO t VALUES (123, 42);

-- activate query result cache
SET experimental_query_result_cache_active_usage = true;
SET experimental_query_result_cache_passive_usage = true;
SET min_query_runs_for_query_result_cache = 3;

-- run query enough times to cache result
SELECT b from t;
SELECT b from t;
SELECT b from t;
EXPLAIN SELECT b from t;

-- query results are no longer in cache after drop
SYSTEM DROP QUERY RESULT CACHE;
EXPLAIN SELECT b from t;

DROP TABLE t;

-- { echoOff }
