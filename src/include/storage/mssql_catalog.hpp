//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/mssql_catalog.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/catalog/catalog.hpp"
#include "duckdb/common/case_insensitive_map.hpp"
#include "mssql_connection.hpp"
#include "mssql_schema_entry.hpp"

namespace duckdb {

class MSSQLTransaction;

class MSSQLCatalog : public Catalog {
public:
	explicit MSSQLCatalog(AttachedDatabase &db_p, const string &connection_string, AccessMode access_mode);
	~MSSQLCatalog() override;

	string GetCatalogType() override {
		return "mssql";
	}

	void Initialize(bool load_builtin) override;
	optional_ptr<CatalogEntry> GetEntry(ClientContext &context, const string &schema, const string &name);

	MSSQLConnection &GetConnection();
	static MSSQLCatalog &Get(ClientContext &context, const string &catalog_name);

private:
	shared_ptr<OwnedMSSQLConnection> owned_connection;
	unique_ptr<MSSQLConnection> connection;
	MSSQLTypeConfig type_config;
	case_insensitive_map_t<unique_ptr<MSSQLSchemaEntry>> schemas;
	mutex catalog_lock;
};

class MSSQLStorageExtension : public StorageExtension {
public:
	MSSQLStorageExtension();
};

} // namespace duckdb
