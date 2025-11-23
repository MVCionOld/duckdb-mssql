#include "mssql_scanner.hpp"
#include "storage/mssql_catalog.hpp"
#include "storage/mssql_table_entry.hpp"
#include "storage/mssql_transaction.hpp"
#include "mssql_utils.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "duckdb/main/database_manager.hpp"
#include "duckdb/main/attached_database.hpp"

namespace duckdb {

struct MSSQLGlobalState : public GlobalTableFunctionState {
	explicit MSSQLGlobalState(unique_ptr<MSSQLResult> result_p) : result(std::move(result_p)) {
	}
	
	unique_ptr<MSSQLResult> result;
	
	idx_t MaxThreads() const override {
		return 1;
	}
};

struct MSSQLLocalState : public LocalTableFunctionState {};

MSSQLBindData::MSSQLBindData(MSSQLCatalog &catalog, MSSQLTableEntry &table)
    : catalog(catalog), table(table) {
}

static unique_ptr<FunctionData> MSSQLBind(ClientContext &context, TableFunctionBindInput &input,
                                           vector<LogicalType> &return_types, vector<string> &names) {
	throw InternalException("MSSQLBind should not be called directly");
}

static unique_ptr<GlobalTableFunctionState> MSSQLInitGlobalState(ClientContext &context,
                                                                   TableFunctionInitInput &input) {
	auto &bind_data = input.bind_data->Cast<MSSQLBindData>();
	
	// Generate SELECT statement
	string select = "SELECT ";
	for (idx_t c = 0; c < input.column_ids.size(); c++) {
		if (c > 0) {
			select += ", ";
		}
		if (input.column_ids[c] == COLUMN_IDENTIFIER_ROW_ID) {
			select += "NULL";
		} else {
			auto &col = bind_data.table.GetColumn(LogicalIndex(input.column_ids[c]));
			auto col_name = col.GetName();
			select += MSSQLUtils::WriteIdentifier(col_name);
		}
	}
	select += " FROM ";
	select += MSSQLUtils::WriteIdentifier(bind_data.table.schema.name);
	select += ".";
	select += MSSQLUtils::WriteIdentifier(bind_data.table.name);
	
	if (!bind_data.limit.empty()) {
		select += bind_data.limit;
	}
	
	// Execute query
	auto &transaction = MSSQLTransaction::Get(context, bind_data.table.catalog);
	auto &con = transaction.GetConnection();
	auto query_result = con.Query(select);
	
	return make_uniq<MSSQLGlobalState>(std::move(query_result));
}

static unique_ptr<LocalTableFunctionState> MSSQLInitLocalState(ExecutionContext &context,
                                                                 TableFunctionInitInput &input,
                                                                 GlobalTableFunctionState *global_state) {
	return make_uniq<MSSQLLocalState>();
}

static void MSSQLScan(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &gstate = data.global_state->Cast<MSSQLGlobalState>();
	DataChunk &res_chunk = gstate.result->NextChunk();
	output.Reference(res_chunk);
}

MSSQLScanFunction::MSSQLScanFunction()
    : TableFunction("mssql_scan", {}, MSSQLScan, MSSQLBind, MSSQLInitGlobalState, MSSQLInitLocalState) {
	projection_pushdown = true;
}

// Query function - allows direct SQL queries
struct MSSQLQueryBindData : public TableFunctionData {
	MSSQLQueryBindData(string dsn_p, string query_p) : dsn(std::move(dsn_p)), query(std::move(query_p)) {
	}
	string dsn;
	string query;
	vector<string> names;
	vector<LogicalType> types;
};

static unique_ptr<FunctionData> MSSQLQueryBind(ClientContext &context, TableFunctionBindInput &input,
                                                vector<LogicalType> &return_types, vector<string> &names) {
	if (input.inputs.size() != 2) {
		throw BinderException("mssql_query requires exactly 2 arguments: DSN and query");
	}
	
	auto dsn = input.inputs[0].GetValue<string>();
	auto query = input.inputs[1].GetValue<string>();
	
	// Open temporary connection to get schema
	auto owned_conn = MSSQLConnection::Open(dsn);
	MSSQLTypeConfig config;
	MSSQLConnection conn(owned_conn, config);
	auto result = conn.Query(query);
	
	// Get column information - we need to peek at the result
	// For now, return empty and let the execution figure it out
	auto bind_data = make_uniq<MSSQLQueryBindData>(dsn, query);
	
	return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> MSSQLQueryInitGlobalState(ClientContext &context,
                                                                        TableFunctionInitInput &input) {
	auto &bind_data = input.bind_data->Cast<MSSQLQueryBindData>();
	
	auto owned_conn = MSSQLConnection::Open(bind_data.dsn);
	MSSQLTypeConfig config;
	auto conn = make_shared_ptr<MSSQLConnection>(owned_conn, config);
	auto result = conn->Query(bind_data.query);
	
	return make_uniq<MSSQLGlobalState>(std::move(result));
}

MSSQLQueryFunction::MSSQLQueryFunction()
    : TableFunction("mssql_query", {LogicalType::VARCHAR, LogicalType::VARCHAR}, MSSQLScan, MSSQLQueryBind,
                    MSSQLQueryInitGlobalState, MSSQLInitLocalState) {
}

// Execute function - for non-SELECT queries
static void MSSQLExecuteFunc(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	// Execute returns no data
	output.SetCardinality(0);
}

static unique_ptr<FunctionData> MSSQLExecuteBind(ClientContext &context, TableFunctionBindInput &input,
                                                  vector<LogicalType> &return_types, vector<string> &names) {
	if (input.inputs.size() != 2) {
		throw BinderException("mssql_execute requires exactly 2 arguments: DSN and query");
	}
	
	auto dsn = input.inputs[0].GetValue<string>();
	auto query = input.inputs[1].GetValue<string>();
	
	// Execute the query
	auto owned_conn = MSSQLConnection::Open(dsn);
	MSSQLTypeConfig config;
	MSSQLConnection conn(owned_conn, config);
	conn.Execute(query);
	
	return make_uniq<MSSQLQueryBindData>(dsn, query);
}

MSSQLExecuteFunction::MSSQLExecuteFunction()
    : TableFunction("mssql_execute", {LogicalType::VARCHAR, LogicalType::VARCHAR}, MSSQLExecuteFunc,
                    MSSQLExecuteBind) {
}

// Clear cache function
static void MSSQLClearCacheFunc(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	// Clear cache logic would go here
	output.SetCardinality(0);
}

static unique_ptr<FunctionData> MSSQLClearCacheBind(ClientContext &context, TableFunctionBindInput &input,
                                                     vector<LogicalType> &return_types, vector<string> &names) {
	// Clear all caches
	return nullptr;
}

MSSQLClearCacheFunction::MSSQLClearCacheFunction()
    : TableFunction("mssql_clear_cache", {}, MSSQLClearCacheFunc, MSSQLClearCacheBind) {
}

void MSSQLClearCacheFunction::ClearCacheOnSetting(ClientContext &context, SetScope scope, Value &parameter) {
	// Clear cache when settings change
}

} // namespace duckdb
