//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/mssql_schema_entry.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/catalog/catalog_entry/schema_catalog_entry.hpp"
#include "duckdb/common/case_insensitive_map.hpp"

namespace duckdb {

class MSSQLCatalog;
class MSSQLTableEntry;

class MSSQLSchemaEntry : public SchemaCatalogEntry {
public:
	MSSQLSchemaEntry(Catalog &catalog, string name);
	
	optional_ptr<CatalogEntry> GetEntry(CatalogType type, const string &name);
	void DropEntry(ClientContext &context, DropInfo &info);
	
	void Scan(ClientContext &context, CatalogType type,
	          const std::function<void(CatalogEntry &)> &callback) override;
	void Scan(CatalogType type, const std::function<void(CatalogEntry &)> &callback) override;
	
	optional_ptr<CatalogEntry> CreateTable(CatalogTransaction transaction, BoundCreateTableInfo &info) override;
	optional_ptr<CatalogEntry> CreateFunction(CatalogTransaction transaction, CreateFunctionInfo &info) override;
	optional_ptr<CatalogEntry> CreateIndex(CatalogTransaction transaction, CreateIndexInfo &info,
	                                       TableCatalogEntry &table) override;
	optional_ptr<CatalogEntry> CreateView(CatalogTransaction transaction, CreateViewInfo &info) override;
	optional_ptr<CatalogEntry> CreateSequence(CatalogTransaction transaction, CreateSequenceInfo &info) override;
	optional_ptr<CatalogEntry> CreateTableFunction(CatalogTransaction transaction,
	                                                CreateTableFunctionInfo &info) override;
	optional_ptr<CatalogEntry> CreateCopyFunction(CatalogTransaction transaction,
	                                               CreateCopyFunctionInfo &info) override;
	optional_ptr<CatalogEntry> CreatePragmaFunction(CatalogTransaction transaction,
	                                                 CreatePragmaFunctionInfo &info) override;
	optional_ptr<CatalogEntry> CreateCollation(CatalogTransaction transaction, CreateCollationInfo &info) override;
	optional_ptr<CatalogEntry> CreateType(CatalogTransaction transaction, CreateTypeInfo &info) override;
	void Alter(CatalogTransaction transaction, AlterInfo &info) override;

private:
	void DropTable(ClientContext &context, DropInfo &info);
	void DropView(ClientContext &context, DropInfo &info);
	void ClearTables();
	
private:
	case_insensitive_map_t<unique_ptr<MSSQLTableEntry>> tables;
	mutex schema_lock;
};

} // namespace duckdb
