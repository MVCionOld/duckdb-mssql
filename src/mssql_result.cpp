#include "mssql_result.hpp"
#include "mssql_connection.hpp"
#include "mssql_utils.hpp"
#include "duckdb/common/exception.hpp"

namespace duckdb {

MSSQLResult::MSSQLResult(SQLHSTMT stmt, MSSQLConnection &connection)
    : stmt(stmt), connection(connection), complete(false), chunk_offset(0) {
	
	// Get column count
	SQLSMALLINT column_count;
	SQLRETURN ret = SQLNumResultCols(stmt, &column_count);
	MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to get column count");
	
	// Get column information
	for (SQLSMALLINT i = 1; i <= column_count; i++) {
		SQLCHAR column_name[256];
		SQLSMALLINT name_length;
		SQLSMALLINT data_type;
		SQLULEN column_size;
		SQLSMALLINT decimal_digits;
		SQLSMALLINT nullable;
		
		ret = SQLDescribeCol(stmt, i, column_name, sizeof(column_name), &name_length,
		                     &data_type, &column_size, &decimal_digits, &nullable);
		MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to describe column");
		
		names.push_back(string((char*)column_name, name_length));
		types.push_back(MSSQLType::ToLogicalType(data_type, column_size, decimal_digits, 
		                                          connection.GetTypeConfig()));
	}
	
	// Initialize chunk
	current_chunk.Initialize(Allocator::DefaultAllocator(), types);
}

MSSQLResult::~MSSQLResult() {
	if (stmt != SQL_NULL_HSTMT) {
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	}
}

DataChunk &MSSQLResult::NextChunk() {
	if (complete) {
		current_chunk.SetCardinality(0);
		return current_chunk;
	}
	
	current_chunk.Reset();
	chunk_offset = 0;
	
	// Fetch rows
	while (chunk_offset < STANDARD_VECTOR_SIZE) {
		SQLRETURN ret = SQLFetch(stmt);
		if (ret == SQL_NO_DATA) {
			complete = true;
			break;
		}
		MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to fetch row");
		
		// Read each column
		for (idx_t col_idx = 0; col_idx < types.size(); col_idx++) {
			SQLLEN indicator;
			auto &type = types[col_idx];
			auto &vec = current_chunk.data[col_idx];
			
			switch (type.id()) {
			case LogicalTypeId::BOOLEAN:
			case LogicalTypeId::TINYINT:
			case LogicalTypeId::UTINYINT: {
				uint8_t value;
				ret = SQLGetData(stmt, col_idx + 1, SQL_C_UTINYINT, &value, sizeof(value), &indicator);
				MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to get data");
				if (indicator == SQL_NULL_DATA) {
					FlatVector::SetNull(vec, chunk_offset, true);
				} else {
					FlatVector::GetData<uint8_t>(vec)[chunk_offset] = value;
				}
				break;
			}
			case LogicalTypeId::SMALLINT:
			case LogicalTypeId::USMALLINT: {
				int16_t value;
				ret = SQLGetData(stmt, col_idx + 1, SQL_C_SSHORT, &value, sizeof(value), &indicator);
				MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to get data");
				if (indicator == SQL_NULL_DATA) {
					FlatVector::SetNull(vec, chunk_offset, true);
				} else {
					FlatVector::GetData<int16_t>(vec)[chunk_offset] = value;
				}
				break;
			}
			case LogicalTypeId::INTEGER:
			case LogicalTypeId::UINTEGER: {
				int32_t value;
				ret = SQLGetData(stmt, col_idx + 1, SQL_C_SLONG, &value, sizeof(value), &indicator);
				MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to get data");
				if (indicator == SQL_NULL_DATA) {
					FlatVector::SetNull(vec, chunk_offset, true);
				} else {
					FlatVector::GetData<int32_t>(vec)[chunk_offset] = value;
				}
				break;
			}
			case LogicalTypeId::BIGINT:
			case LogicalTypeId::UBIGINT: {
				int64_t value;
				ret = SQLGetData(stmt, col_idx + 1, SQL_C_SBIGINT, &value, sizeof(value), &indicator);
				MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to get data");
				if (indicator == SQL_NULL_DATA) {
					FlatVector::SetNull(vec, chunk_offset, true);
				} else {
					FlatVector::GetData<int64_t>(vec)[chunk_offset] = value;
				}
				break;
			}
			case LogicalTypeId::FLOAT: {
				float value;
				ret = SQLGetData(stmt, col_idx + 1, SQL_C_FLOAT, &value, sizeof(value), &indicator);
				MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to get data");
				if (indicator == SQL_NULL_DATA) {
					FlatVector::SetNull(vec, chunk_offset, true);
				} else {
					FlatVector::GetData<float>(vec)[chunk_offset] = value;
				}
				break;
			}
			case LogicalTypeId::DOUBLE: {
				double value;
				ret = SQLGetData(stmt, col_idx + 1, SQL_C_DOUBLE, &value, sizeof(value), &indicator);
				MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to get data");
				if (indicator == SQL_NULL_DATA) {
					FlatVector::SetNull(vec, chunk_offset, true);
				} else {
					FlatVector::GetData<double>(vec)[chunk_offset] = value;
				}
				break;
			}
			case LogicalTypeId::VARCHAR: {
				char buffer[4096];
				ret = SQLGetData(stmt, col_idx + 1, SQL_C_CHAR, buffer, sizeof(buffer), &indicator);
				if (indicator == SQL_NULL_DATA) {
					FlatVector::SetNull(vec, chunk_offset, true);
				} else {
					string value;
					if (indicator > (SQLLEN)sizeof(buffer) - 1) {
						// Data is larger than buffer, need to read in chunks
						value = string(buffer, sizeof(buffer) - 1);
						while (ret == SQL_SUCCESS_WITH_INFO) {
							ret = SQLGetData(stmt, col_idx + 1, SQL_C_CHAR, buffer, sizeof(buffer), &indicator);
							if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
								value += string(buffer);
							}
						}
					} else {
						value = string(buffer, indicator);
					}
					FlatVector::GetData<string_t>(vec)[chunk_offset] = StringVector::AddString(vec, value);
				}
				break;
			}
			case LogicalTypeId::DATE: {
				SQL_DATE_STRUCT date;
				ret = SQLGetData(stmt, col_idx + 1, SQL_C_TYPE_DATE, &date, sizeof(date), &indicator);
				MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to get data");
				if (indicator == SQL_NULL_DATA) {
					FlatVector::SetNull(vec, chunk_offset, true);
				} else {
					FlatVector::GetData<date_t>(vec)[chunk_offset] = 
					    Date::FromDate(date.year, date.month, date.day);
				}
				break;
			}
			case LogicalTypeId::TIME: {
				SQL_TIME_STRUCT time;
				ret = SQLGetData(stmt, col_idx + 1, SQL_C_TYPE_TIME, &time, sizeof(time), &indicator);
				MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to get data");
				if (indicator == SQL_NULL_DATA) {
					FlatVector::SetNull(vec, chunk_offset, true);
				} else {
					FlatVector::GetData<dtime_t>(vec)[chunk_offset] = 
					    Time::FromTime(time.hour, time.minute, time.second, 0);
				}
				break;
			}
			case LogicalTypeId::TIMESTAMP: {
				SQL_TIMESTAMP_STRUCT timestamp;
				ret = SQLGetData(stmt, col_idx + 1, SQL_C_TYPE_TIMESTAMP, &timestamp, sizeof(timestamp), &indicator);
				MSSQLUtils::CheckResult(ret, SQL_HANDLE_STMT, stmt, "Failed to get data");
				if (indicator == SQL_NULL_DATA) {
					FlatVector::SetNull(vec, chunk_offset, true);
				} else {
					auto date = Date::FromDate(timestamp.year, timestamp.month, timestamp.day);
					auto time = Time::FromTime(timestamp.hour, timestamp.minute, timestamp.second, 
					                           timestamp.fraction / 1000000); // Convert nanoseconds to microseconds
					FlatVector::GetData<timestamp_t>(vec)[chunk_offset] = Timestamp::FromDatetime(date, time);
				}
				break;
			}
			default:
				throw NotImplementedException("Unsupported type in MSSQLResult: %s", type.ToString());
			}
		}
		
		chunk_offset++;
	}
	
	current_chunk.SetCardinality(chunk_offset);
	return current_chunk;
}

} // namespace duckdb
