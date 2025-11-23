#include "storage/mssql_table_entry.hpp"
#include "storage/mssql_catalog.hpp"
#include "mssql_scanner.hpp"
#include "duckdb/storage/statistics/base_statistics.hpp"

namespace duckdb {

MSSQLTableEntry::MSSQLTableEntry(Catalog &catalog, SchemaCatalogEntry &schema, CreateTableInfo &info)
    : TableCatalogEntry(catalog, schema, info) {
}

unique_ptr<BaseStatistics> MSSQLTableEntry::GetStatistics(ClientContext &context, column_t column_id) {
	return nullptr;
}

TableFunction MSSQLTableEntry::GetScanFunction(ClientContext &context, unique_ptr<FunctionData> &bind_data) {
	auto &mssql_catalog = GetMSSQLCatalog();
	auto scan_bind_data = make_uniq<MSSQLBindData>(mssql_catalog, *this);
	
	for (auto &col : columns.Logical()) {
		scan_bind_data->names.push_back(col.Name());
		scan_bind_data->types.push_back(col.Type());
	}
	
	bind_data = std::move(scan_bind_data);
	return MSSQLScanFunction();
}

TableStorageInfo MSSQLTableEntry::GetStorageInfo(ClientContext &context) {
	return TableStorageInfo();
}

void MSSQLTableEntry::BindUpdateConstraints(Binder &binder, LogicalGet &get, LogicalProjection &proj,
                                             LogicalUpdate &update, ClientContext &context) {
}

} // namespace duckdb
