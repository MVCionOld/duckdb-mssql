//===----------------------------------------------------------------------===//
//                         DuckDB
//
// mssql_result.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb.hpp"
#include "mssql_types.hpp"
#include <sql.h>
#include <sqlext.h>

namespace duckdb {

class MSSQLConnection;

class MSSQLResult {
public:
	MSSQLResult(SQLHSTMT stmt, MSSQLConnection &connection);
	~MSSQLResult();

	DataChunk &NextChunk();
	bool IsComplete() const { return complete; }

private:
	void FetchRow();
	void BindColumns();

private:
	SQLHSTMT stmt;
	MSSQLConnection &connection;
	vector<LogicalType> types;
	vector<string> names;
	DataChunk current_chunk;
	bool complete;
	idx_t chunk_offset;
};

} // namespace duckdb
