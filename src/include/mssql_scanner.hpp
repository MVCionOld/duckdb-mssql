//===----------------------------------------------------------------------===//
//                         DuckDB
//
// mssql_scanner.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb.hpp"
#include "duckdb/function/table_function.hpp"
#include "mssql_connection.hpp"

namespace duckdb {

class MSSQLCatalog;
class MSSQLTableEntry;

struct MSSQLBindData : public TableFunctionData {
	MSSQLBindData(MSSQLCatalog &catalog, MSSQLTableEntry &table);
	
	MSSQLCatalog &catalog;
	MSSQLTableEntry &table;
	vector<string> names;
	vector<LogicalType> types;
	string limit;
};

class MSSQLScanFunction : public TableFunction {
public:
	MSSQLScanFunction();
};

class MSSQLQueryFunction : public TableFunction {
public:
	MSSQLQueryFunction();
};

class MSSQLExecuteFunction : public TableFunction {
public:
	MSSQLExecuteFunction();
};

class MSSQLClearCacheFunction : public TableFunction {
public:
	MSSQLClearCacheFunction();
	
	static void ClearCacheOnSetting(ClientContext &context, SetScope scope, Value &parameter);
};

} // namespace duckdb
