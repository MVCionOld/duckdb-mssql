//===----------------------------------------------------------------------===//
//                         DuckDB
//
// mssql_types.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb.hpp"
#include <sql.h>
#include <sqlext.h>

namespace duckdb {

struct MSSQLTypeConfig {
	bool tinyint_as_boolean = true;
	bool bit_as_boolean = true;
};

class MSSQLType {
public:
	static LogicalType ToLogicalType(SQLSMALLINT sql_type, SQLLEN column_size, SQLSMALLINT decimal_digits,
	                                  const MSSQLTypeConfig &config);
	static SQLSMALLINT ToSQLType(const LogicalType &type);
	static string TypeToString(SQLSMALLINT sql_type);
};

} // namespace duckdb
