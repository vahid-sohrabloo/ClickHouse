-- { echoOn }

SYSTEM DROP QUERY RESULT CACHE;

DROP TABLE IF EXISTS t;
CREATE TABLE t (a UInt32, b UInt32, c UInt32, d UInt32) ENGINE=Memory;
INSERT INTO t VALUES (0, 2, 4, 3);

SET experimental_query_result_cache_active_usage = true;
SET experimental_query_result_cache_passive_usage = true;

SET min_query_runs_for_query_result_cache = 1;
SELECT a from t;
SELECT * FROM system.queryresult_cache;

SET min_query_runs_for_query_result_cache = 2;
SELECT b from t;
SELECT b from t;
SELECT * FROM system.queryresult_cache;

SET min_query_runs_for_query_result_cache = 3;
SELECT c from t;
SELECT c from t;
SELECT c from t;
SELECT * FROM system.queryresult_cache;

DROP TABLE t;

-- { echoOff }
