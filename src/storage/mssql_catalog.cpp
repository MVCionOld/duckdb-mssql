#include "storage/mssql_catalog.hpp"
#include "storage/mssql_transaction.hpp"
#include "storage/mssql_schema_entry.hpp"
#include "storage/mssql_table_entry.hpp"
#include "mssql_utils.hpp"
#include "duckdb/parser/parsed_data/attach_info.hpp"
#include "duckdb/storage/database_size.hpp"
#include "duckdb/main/client_context.hpp"
#include "duckdb/main/attached_database.hpp"
#include "duckdb/main/database_manager.hpp"

namespace duckdb {

MSSQLCatalog::MSSQLCatalog(AttachedDatabase &db_p, const string &connection_string, AccessMode access_mode)
    : Catalog(db_p) {
	// Parse connection string and set up type config
	owned_connection = MSSQLConnection::Open(connection_string);
	connection = make_uniq<MSSQLConnection>(owned_connection, type_config);
}

MSSQLCatalog::~MSSQLCatalog() {
}

void MSSQLCatalog::Initialize(bool load_builtin) {
	// Load schema information
}

optional_ptr<CatalogEntry> MSSQLCatalog::CreateSchema(CatalogTransaction transaction, CreateSchemaInfo &info) {
	throw BinderException("MS SQL Server does not support creating schemas through DuckDB yet");
}

void MSSQLCatalog::DropSchema(ClientContext &context, DropInfo &info) {
	throw BinderException("MS SQL Server does not support dropping schemas through DuckDB yet");
}

void MSSQLCatalog::ScanSchemas(ClientContext &context, std::function<void(SchemaCatalogEntry &)> callback) {
	lock_guard<mutex> l(catalog_lock);
	
	// Query MS SQL Server for schemas
	auto &conn = GetConnection();
	string query = "SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA";
	
	auto result = conn.Query(query);
	auto &chunk = result->NextChunk();
	
	for (idx_t i = 0; i < chunk.size(); i++) {
		auto schema_name = chunk.GetValue(0, i).ToString();
		
		auto schema_entry = schemas.find(schema_name);
		if (schema_entry == schemas.end()) {
			auto new_schema = make_uniq<MSSQLSchemaEntry>(*this, schema_name);
			auto schema_ptr = new_schema.get();
			schemas[schema_name] = std::move(new_schema);
			callback(*schema_ptr);
		} else {
			callback(*schema_entry->second);
		}
	}
}

optional_ptr<SchemaCatalogEntry> MSSQLCatalog::GetSchema(CatalogTransaction transaction, const string &schema_name,
                                                          OnEntryNotFound if_not_found, QueryErrorContext error_context) {
	lock_guard<mutex> l(catalog_lock);
	
	auto schema_entry = schemas.find(schema_name);
	if (schema_entry != schemas.end()) {
		return schema_entry->second.get();
	}
	
	// Try to load schema from database
	auto new_schema = make_uniq<MSSQLSchemaEntry>(*this, schema_name);
	auto schema_ptr = new_schema.get();
	schemas[schema_name] = std::move(new_schema);
	
	return schema_ptr;
}

DatabaseSize MSSQLCatalog::GetDatabaseSize(ClientContext &context) {
	return DatabaseSize();
}

bool MSSQLCatalog::InMemory() {
	return false;
}

string MSSQLCatalog::GetDBPath() {
	return string();
}

MSSQLConnection &MSSQLCatalog::GetConnection() {
	return *connection;
}

MSSQLCatalog &MSSQLCatalog::Get(ClientContext &context, const string &catalog_name) {
	auto &db_manager = DatabaseManager::Get(context);
	auto &db = db_manager.GetDatabase(context, catalog_name);
	return db.GetCatalog().Cast<MSSQLCatalog>();
}

// Storage Extension
MSSQLStorageExtension::MSSQLStorageExtension() {
	attach = [](StorageExtensionInfo *storage_info, ClientContext &context, AttachedDatabase &db,
	            const string &name, AttachInfo &info, AccessMode access_mode) {
		// Get connection string from attach info
		string connection_string;
		for (auto &entry : info.options) {
			if (!connection_string.empty()) {
				connection_string += ";";
			}
			connection_string += entry.first + "=" + entry.second.ToString();
		}
		
		return make_uniq<MSSQLCatalog>(db, connection_string, access_mode);
	};
}

} // namespace duckdb
