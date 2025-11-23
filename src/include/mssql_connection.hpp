//===----------------------------------------------------------------------===//
//                         DuckDB
//
// mssql_connection.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb.hpp"
#include "duckdb/common/shared_ptr.hpp"
#include "duckdb/common/mutex.hpp"
#include "mssql_types.hpp"
#include "mssql_result.hpp"
#include <sql.h>
#include <sqlext.h>

namespace duckdb {

struct OwnedMSSQLConnection {
	SQLHENV env = SQL_NULL_HENV;
	SQLHDBC dbc = SQL_NULL_HDBC;

	OwnedMSSQLConnection();
	~OwnedMSSQLConnection();
};

class MSSQLConnection {
public:
	explicit MSSQLConnection(shared_ptr<OwnedMSSQLConnection> connection, MSSQLTypeConfig type_config_p);
	~MSSQLConnection();
	
	// disable copy constructors
	MSSQLConnection(const MSSQLConnection &other) = delete;
	MSSQLConnection &operator=(const MSSQLConnection &) = delete;
	
	// enable move constructors
	MSSQLConnection(MSSQLConnection &&other) noexcept;
	MSSQLConnection &operator=(MSSQLConnection &&) noexcept;

public:
	static shared_ptr<OwnedMSSQLConnection> Open(const string &connection_string);
	unique_ptr<MSSQLResult> Query(const string &query);
	void Execute(const string &query);
	
	SQLHDBC GetConnection() const { return connection->dbc; }
	SQLHENV GetEnvironment() const { return connection->env; }
	const MSSQLTypeConfig &GetTypeConfig() const { return type_config; }
	
	static void DebugSetPrintQueries(bool print);
	static bool DebugPrintQueries();

private:
	shared_ptr<OwnedMSSQLConnection> connection;
	MSSQLTypeConfig type_config;
	static bool print_queries;
};

} // namespace duckdb
