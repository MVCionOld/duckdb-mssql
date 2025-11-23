#include "mssql_connection.hpp"
#include "mssql_utils.hpp"
#include "duckdb/common/exception.hpp"

namespace duckdb {

bool MSSQLConnection::print_queries = false;

OwnedMSSQLConnection::OwnedMSSQLConnection() {
	// Allocate environment handle
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
	if (ret != SQL_SUCCESS) {
		throw IOException("Failed to allocate ODBC environment handle");
	}
	
	// Set ODBC version
	ret = SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
	if (ret != SQL_SUCCESS) {
		SQLFreeHandle(SQL_HANDLE_ENV, env);
		throw IOException("Failed to set ODBC version");
	}
	
	// Allocate connection handle
	ret = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
	if (ret != SQL_SUCCESS) {
		SQLFreeHandle(SQL_HANDLE_ENV, env);
		throw IOException("Failed to allocate ODBC connection handle");
	}
}

OwnedMSSQLConnection::~OwnedMSSQLConnection() {
	if (dbc != SQL_NULL_HDBC) {
		SQLDisconnect(dbc);
		SQLFreeHandle(SQL_HANDLE_DBC, dbc);
		dbc = SQL_NULL_HDBC;
	}
	if (env != SQL_NULL_HENV) {
		SQLFreeHandle(SQL_HANDLE_ENV, env);
		env = SQL_NULL_HENV;
	}
}

MSSQLConnection::MSSQLConnection(shared_ptr<OwnedMSSQLConnection> connection, MSSQLTypeConfig type_config_p)
    : connection(std::move(connection)), type_config(type_config_p) {
}

MSSQLConnection::~MSSQLConnection() {
}

MSSQLConnection::MSSQLConnection(MSSQLConnection &&other) noexcept
    : connection(std::move(other.connection)), type_config(other.type_config) {
}

MSSQLConnection &MSSQLConnection::operator=(MSSQLConnection &&other) noexcept {
	connection = std::move(other.connection);
	type_config = other.type_config;
	return *this;
}

shared_ptr<OwnedMSSQLConnection> MSSQLConnection::Open(const string &connection_string) {
	auto conn = make_shared_ptr<OwnedMSSQLConnection>();
	
	SQLCHAR out_conn_str[1024];
	SQLSMALLINT out_conn_str_len;
	
	// Connect to database
	SQLRETURN ret = SQLDriverConnect(conn->dbc, NULL, (SQLCHAR*)connection_string.c_str(), SQL_NTS,
	                                  out_conn_str, sizeof(out_conn_str), &out_conn_str_len, SQL_DRIVER_NOPROMPT);
	
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		string error = MSSQLUtils::GetODBCError(SQL_HANDLE_DBC, conn->dbc);
		throw IOException("Failed to connect to MS SQL Server: %s", error);
	}
	
	return conn;
}

unique_ptr<MSSQLResult> MSSQLConnection::Query(const string &query) {
	if (print_queries) {
		Printer::Print(StringUtil::Format("[MSSQL Query] %s", query));
	}
	
	SQLHSTMT stmt;
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, connection->dbc, &stmt);
	MSSQLUtils::CheckResult(ret, SQL_HANDLE_DBC, connection->dbc, "Failed to allocate statement handle");
	
	ret = SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		string error = MSSQLUtils::GetODBCError(SQL_HANDLE_STMT, stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		throw IOException("Failed to execute query: %s", error);
	}
	
	return make_uniq<MSSQLResult>(stmt, *this);
}

void MSSQLConnection::Execute(const string &query) {
	if (print_queries) {
		Printer::Print(StringUtil::Format("[MSSQL Execute] %s", query));
	}
	
	SQLHSTMT stmt;
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, connection->dbc, &stmt);
	MSSQLUtils::CheckResult(ret, SQL_HANDLE_DBC, connection->dbc, "Failed to allocate statement handle");
	
	ret = SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		string error = MSSQLUtils::GetODBCError(SQL_HANDLE_STMT, stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		throw IOException("Failed to execute statement: %s", error);
	}
	
	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
}

void MSSQLConnection::DebugSetPrintQueries(bool print) {
	print_queries = print;
}

bool MSSQLConnection::DebugPrintQueries() {
	return print_queries;
}

} // namespace duckdb
