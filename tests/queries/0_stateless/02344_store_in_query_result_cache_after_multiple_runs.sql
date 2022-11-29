-- { echoOn }

select * from system.settings where name like 'experimental_query_%';

DROP TABLE IF EXISTS t;
CREATE TABLE t (a UInt32, b UInt32, c UInt32, d UInt32) ENGINE=Memory;
INSERT INTO t VALUES (0, 2, 4, 3);

SET experimental_query_result_cache_active_usage = true;
SET experimental_query_result_cache_passive_usage = true;

SET min_query_runs_for_query_result_cache = 1;
EXPLAIN SELECT a from t;

SET min_query_runs_for_query_result_cache = 2;
EXPLAIN SELECT b from t;
EXPLAIN SELECT b from t;

SET min_query_runs_for_query_result_cache = 3;
EXPLAIN SELECT c from t;
EXPLAIN SELECT c from t;
EXPLAIN SELECT c from t;

DROP TABLE t;

-- { echoOff }
