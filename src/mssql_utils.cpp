#include "mssql_utils.hpp"
#include "duckdb/common/exception.hpp"

namespace duckdb {

string MSSQLUtils::WriteIdentifier(const string &identifier) {
	return WriteQuotedIdentifier(identifier);
}

string MSSQLUtils::WriteQuotedIdentifier(const string &identifier) {
	// SQL Server uses square brackets for identifiers
	string result = "[";
	for (char c : identifier) {
		if (c == ']') {
			result += "]]"; // Escape closing bracket
		} else {
			result += c;
		}
	}
	result += "]";
	return result;
}

void MSSQLUtils::CheckResult(SQLRETURN ret, SQLSMALLINT handle_type, SQLHANDLE handle, const string &context) {
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		string error_msg = GetODBCError(handle_type, handle);
		throw IOException("%s: %s", context, error_msg);
	}
}

string MSSQLUtils::GetODBCError(SQLSMALLINT handle_type, SQLHANDLE handle) {
	SQLCHAR sql_state[6];
	SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER native_error;
	SQLSMALLINT message_length;
	
	string result;
	SQLSMALLINT rec_number = 1;
	
	while (SQLGetDiagRec(handle_type, handle, rec_number, sql_state, &native_error,
	                     message, sizeof(message), &message_length) == SQL_SUCCESS) {
		if (rec_number > 1) {
			result += "; ";
		}
		result += StringUtil::Format("SQLSTATE=%s: %s", (char*)sql_state, (char*)message);
		rec_number++;
	}
	
	if (result.empty()) {
		result = "Unknown ODBC error";
	}
	
	return result;
}

} // namespace duckdb
