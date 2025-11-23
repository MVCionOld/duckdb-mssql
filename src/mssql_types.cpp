#include "mssql_types.hpp"
#include "duckdb/common/exception.hpp"

namespace duckdb {

LogicalType MSSQLType::ToLogicalType(SQLSMALLINT sql_type, SQLLEN column_size, SQLSMALLINT decimal_digits,
                                      const MSSQLTypeConfig &config) {
	switch (sql_type) {
	case SQL_BIT:
		if (config.bit_as_boolean) {
			return LogicalType::BOOLEAN;
		}
		return LogicalType::TINYINT;
	case SQL_TINYINT:
		return LogicalType::UTINYINT;
	case SQL_SMALLINT:
		return LogicalType::SMALLINT;
	case SQL_INTEGER:
		return LogicalType::INTEGER;
	case SQL_BIGINT:
		return LogicalType::BIGINT;
	case SQL_REAL:
		return LogicalType::FLOAT;
	case SQL_FLOAT:
	case SQL_DOUBLE:
		return LogicalType::DOUBLE;
	case SQL_NUMERIC:
	case SQL_DECIMAL:
		if (decimal_digits == 0 && column_size <= 18) {
			// Can fit in BIGINT
			return LogicalType::BIGINT;
		}
		return LogicalType::DECIMAL(column_size, decimal_digits);
	case SQL_CHAR:
	case SQL_VARCHAR:
	case SQL_LONGVARCHAR:
	case SQL_WCHAR:
	case SQL_WVARCHAR:
	case SQL_WLONGVARCHAR:
		return LogicalType::VARCHAR;
	case SQL_BINARY:
	case SQL_VARBINARY:
	case SQL_LONGVARBINARY:
		return LogicalType::BLOB;
	case SQL_TYPE_DATE:
		return LogicalType::DATE;
	case SQL_TYPE_TIME:
		return LogicalType::TIME;
	case SQL_TYPE_TIMESTAMP:
		return LogicalType::TIMESTAMP;
	case SQL_GUID:
		return LogicalType::VARCHAR;
	default:
		throw NotImplementedException("Unsupported SQL Server type: %d", sql_type);
	}
}

SQLSMALLINT MSSQLType::ToSQLType(const LogicalType &type) {
	switch (type.id()) {
	case LogicalTypeId::BOOLEAN:
		return SQL_BIT;
	case LogicalTypeId::TINYINT:
		return SQL_TINYINT;
	case LogicalTypeId::UTINYINT:
		return SQL_TINYINT;
	case LogicalTypeId::SMALLINT:
		return SQL_SMALLINT;
	case LogicalTypeId::USMALLINT:
		return SQL_SMALLINT;
	case LogicalTypeId::INTEGER:
		return SQL_INTEGER;
	case LogicalTypeId::UINTEGER:
		return SQL_INTEGER;
	case LogicalTypeId::BIGINT:
		return SQL_BIGINT;
	case LogicalTypeId::UBIGINT:
		return SQL_BIGINT;
	case LogicalTypeId::FLOAT:
		return SQL_REAL;
	case LogicalTypeId::DOUBLE:
		return SQL_DOUBLE;
	case LogicalTypeId::DECIMAL:
		return SQL_DECIMAL;
	case LogicalTypeId::VARCHAR:
		return SQL_VARCHAR;
	case LogicalTypeId::BLOB:
		return SQL_VARBINARY;
	case LogicalTypeId::DATE:
		return SQL_TYPE_DATE;
	case LogicalTypeId::TIME:
		return SQL_TYPE_TIME;
	case LogicalTypeId::TIMESTAMP:
		return SQL_TYPE_TIMESTAMP;
	default:
		throw NotImplementedException("Unsupported DuckDB type for SQL Server: %s", type.ToString());
	}
}

string MSSQLType::TypeToString(SQLSMALLINT sql_type) {
	switch (sql_type) {
	case SQL_BIT:
		return "BIT";
	case SQL_TINYINT:
		return "TINYINT";
	case SQL_SMALLINT:
		return "SMALLINT";
	case SQL_INTEGER:
		return "INTEGER";
	case SQL_BIGINT:
		return "BIGINT";
	case SQL_REAL:
		return "REAL";
	case SQL_FLOAT:
	case SQL_DOUBLE:
		return "FLOAT";
	case SQL_NUMERIC:
		return "NUMERIC";
	case SQL_DECIMAL:
		return "DECIMAL";
	case SQL_CHAR:
		return "CHAR";
	case SQL_VARCHAR:
		return "VARCHAR";
	case SQL_LONGVARCHAR:
		return "LONGVARCHAR";
	case SQL_BINARY:
		return "BINARY";
	case SQL_VARBINARY:
		return "VARBINARY";
	case SQL_LONGVARBINARY:
		return "LONGVARBINARY";
	case SQL_TYPE_DATE:
		return "DATE";
	case SQL_TYPE_TIME:
		return "TIME";
	case SQL_TYPE_TIMESTAMP:
		return "TIMESTAMP";
	case SQL_GUID:
		return "GUID";
	default:
		return StringUtil::Format("UNKNOWN(%d)", sql_type);
	}
}

} // namespace duckdb
