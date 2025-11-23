#include "storage/mssql_catalog.hpp"
#include "storage/mssql_transaction.hpp"
#include "storage/mssql_schema_entry.hpp"
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

optional_ptr<CatalogEntry> MSSQLCatalog::GetEntry(ClientContext &context, const string &schema, const string &name) {
	lock_guard<mutex> l(catalog_lock);
	
	auto schema_entry = schemas.find(schema);
	if (schema_entry == schemas.end()) {
		// Create schema entry on-demand
		auto new_schema = make_uniq<MSSQLSchemaEntry>(*this, schema);
		auto schema_ptr = new_schema.get();
		schemas[schema] = std::move(new_schema);
		return schema_ptr->GetEntry(CatalogType::TABLE_ENTRY, name);
	}
	
	return schema_entry->second->GetEntry(CatalogType::TABLE_ENTRY, name);
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
