//===----------------------------------------------------------------------===//
//                         DuckDB
//
// mssql_utils.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb.hpp"
#include <sql.h>
#include <sqlext.h>

namespace duckdb {

class MSSQLUtils {
public:
	static string WriteIdentifier(const string &identifier);
	static string WriteQuotedIdentifier(const string &identifier);
	static void CheckResult(SQLRETURN ret, SQLSMALLINT handle_type, SQLHANDLE handle, const string &context);
	static string GetODBCError(SQLSMALLINT handle_type, SQLHANDLE handle);
};

} // namespace duckdb
