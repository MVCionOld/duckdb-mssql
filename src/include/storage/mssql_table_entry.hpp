//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/mssql_table_entry.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/catalog/catalog_entry/table_catalog_entry.hpp"
#include "duckdb/parser/parsed_data/create_table_info.hpp"

namespace duckdb {

class MSSQLCatalog;
class MSSQLSchemaEntry;

class MSSQLTableEntry : public TableCatalogEntry {
public:
	MSSQLTableEntry(Catalog &catalog, SchemaCatalogEntry &schema, CreateTableInfo &info);

	unique_ptr<BaseStatistics> GetStatistics(ClientContext &context, column_t column_id) override;
	TableFunction GetScanFunction(ClientContext &context, unique_ptr<FunctionData> &bind_data) override;
	TableStorageInfo GetStorageInfo(ClientContext &context) override;
	void BindUpdateConstraints(Binder &binder, LogicalGet &get, LogicalProjection &proj, LogicalUpdate &update,
	                            ClientContext &context) override;

	MSSQLCatalog &GetMSSQLCatalog() {
		return catalog.Cast<MSSQLCatalog>();
	}
};

} // namespace duckdb
