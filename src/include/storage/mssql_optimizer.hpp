//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/mssql_optimizer.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/main/config.hpp"
#include "duckdb/planner/logical_operator.hpp"

namespace duckdb {

class MSSQLOptimizer {
public:
	static void Optimize(OptimizerExtensionInput &input, unique_ptr<LogicalOperator> &plan);
};

} // namespace duckdb
