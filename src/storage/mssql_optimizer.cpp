#include "storage/mssql_optimizer.hpp"

namespace duckdb {

void MSSQLOptimizer::Optimize(OptimizerExtensionInput &input, unique_ptr<LogicalOperator> &plan) {
	// Placeholder for optimizer extensions
	// Could implement filter pushdown, projection pushdown, etc.
}

} // namespace duckdb
