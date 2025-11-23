#define DUCKDB_BUILD_LOADABLE_EXTENSION
#include "duckdb.hpp"

#include "mssql_scanner.hpp"
#include "mssql_scanner_extension.hpp"
#include "storage/mssql_catalog.hpp"
#include "storage/mssql_optimizer.hpp"

#include "duckdb/catalog/catalog.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "duckdb/main/extension/extension_loader.hpp"
#include "duckdb/main/database_manager.hpp"
#include "duckdb/main/attached_database.hpp"

namespace duckdb {

static void SetMSSQLDebugQueryPrint(ClientContext &context, SetScope scope, Value &parameter) {
	MSSQLConnection::DebugSetPrintQueries(BooleanValue::Get(parameter));
}

static void LoadInternal(ExtensionLoader &loader) {
	// Register table functions
	MSSQLClearCacheFunction clear_cache_func;
	loader.RegisterFunction(clear_cache_func);
	
	MSSQLExecuteFunction execute_function;
	loader.RegisterFunction(execute_function);
	
	MSSQLQueryFunction query_function;
	loader.RegisterFunction(query_function);
	
	// Register storage extension
	auto &db = loader.GetDatabaseInstance();
	auto &config = DBConfig::GetConfig(db);
	config.storage_extensions["mssql_scanner"] = make_uniq<MSSQLStorageExtension>();
	
	// Add extension options
	config.AddExtensionOption("mssql_debug_show_queries", 
	                          "DEBUG SETTING: print all queries sent to MS SQL Server to stdout",
	                          LogicalType::BOOLEAN, Value::BOOLEAN(false), SetMSSQLDebugQueryPrint);
	
	config.AddExtensionOption("mssql_tinyint_as_boolean", 
	                          "Whether or not to convert TINYINT columns to BOOLEAN",
	                          LogicalType::BOOLEAN, Value::BOOLEAN(true), 
	                          MSSQLClearCacheFunction::ClearCacheOnSetting);
	
	config.AddExtensionOption("mssql_bit_as_boolean", 
	                          "Whether or not to convert BIT columns to BOOLEAN",
	                          LogicalType::BOOLEAN, Value::BOOLEAN(true), 
	                          MSSQLClearCacheFunction::ClearCacheOnSetting);
	
	// Register optimizer
	OptimizerExtension mssql_optimizer;
	mssql_optimizer.optimize_function = MSSQLOptimizer::Optimize;
	config.optimizer_extensions.push_back(std::move(mssql_optimizer));
}

void MSSQLScannerExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(mssql_scanner, loader) {
	LoadInternal(loader);
}

}

} // namespace duckdb
