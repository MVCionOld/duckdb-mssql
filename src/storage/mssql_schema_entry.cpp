#include "storage/mssql_schema_entry.hpp"
#include "storage/mssql_catalog.hpp"
#include "storage/mssql_table_entry.hpp"
#include "storage/mssql_transaction.hpp"
#include "mssql_utils.hpp"
#include "mssql_connection.hpp"
#include "duckdb/parser/parsed_data/create_table_info.hpp"
#include "duckdb/parser/parsed_data/drop_info.hpp"
#include "duckdb/parser/constraints/not_null_constraint.hpp"

namespace duckdb {

MSSQLSchemaEntry::MSSQLSchemaEntry(Catalog &catalog, string name)
    : SchemaCatalogEntry(catalog, std::move(name), false) {
}

optional_ptr<CatalogEntry> MSSQLSchemaEntry::GetEntry(CatalogType type, const string &name) {
	if (type != CatalogType::TABLE_ENTRY) {
		return nullptr;
	}
	
	lock_guard<mutex> l(schema_lock);
	
	auto table_entry = tables.find(name);
	if (table_entry != tables.end()) {
		return table_entry->second.get();
	}
	
	// Query MS SQL Server for table information
	auto &mssql_catalog = catalog.Cast<MSSQLCatalog>();
	auto &conn = mssql_catalog.GetConnection();
	
	// Query to get table columns
	string query = StringUtil::Format(
	    "SELECT COLUMN_NAME, DATA_TYPE, CHARACTER_MAXIMUM_LENGTH, NUMERIC_PRECISION, NUMERIC_SCALE, IS_NULLABLE "
	    "FROM INFORMATION_SCHEMA.COLUMNS "
	    "WHERE TABLE_SCHEMA = '%s' AND TABLE_NAME = '%s' "
	    "ORDER BY ORDINAL_POSITION",
	    this->name, name);
	
	auto result = conn.Query(query);
	auto &chunk = result->NextChunk();
	
	if (chunk.size() == 0) {
		return nullptr;
	}
	
	// Create table info
	auto create_info = make_uniq<CreateTableInfo>();
	create_info->catalog = catalog.GetName();
	create_info->schema = this->name;
	create_info->table = name;
	create_info->temporary = false;
	
	// Add columns
	for (idx_t i = 0; i < chunk.size(); i++) {
		auto col_name = chunk.GetValue(0, i).ToString();
		auto data_type = chunk.GetValue(1, i).ToString();
		
		// Parse SQL Server type to DuckDB type
		LogicalType type;
		if (data_type == "int") {
			type = LogicalType::INTEGER;
		} else if (data_type == "bigint") {
			type = LogicalType::BIGINT;
		} else if (data_type == "smallint") {
			type = LogicalType::SMALLINT;
		} else if (data_type == "tinyint") {
			type = LogicalType::UTINYINT;
		} else if (data_type == "bit") {
			type = LogicalType::BOOLEAN;
		} else if (data_type == "float" || data_type == "real") {
			type = LogicalType::FLOAT;
		} else if (data_type == "decimal" || data_type == "numeric") {
			type = LogicalType::DOUBLE; // Simplified
		} else if (data_type == "varchar" || data_type == "nvarchar" || data_type == "char" || data_type == "nchar" || data_type == "text" || data_type == "ntext") {
			type = LogicalType::VARCHAR;
		} else if (data_type == "date") {
			type = LogicalType::DATE;
		} else if (data_type == "time") {
			type = LogicalType::TIME;
		} else if (data_type == "datetime" || data_type == "datetime2" || data_type == "smalldatetime") {
			type = LogicalType::TIMESTAMP;
		} else {
			type = LogicalType::VARCHAR; // Default fallback
		}
		
		create_info->columns.AddColumn(ColumnDefinition(col_name, type));
	}
	
	// Create table entry
	auto table = make_uniq<MSSQLTableEntry>(catalog, *this, *create_info);
	auto table_ptr = table.get();
	tables[name] = std::move(table);
	
	return table_ptr;
}

void MSSQLSchemaEntry::Scan(ClientContext &context, CatalogType type,
                             const std::function<void(CatalogEntry &)> &callback) {
	if (type != CatalogType::TABLE_ENTRY) {
		return;
	}
	
	// Query all tables in schema
	auto &mssql_catalog = catalog.Cast<MSSQLCatalog>();
	auto &conn = mssql_catalog.GetConnection();
	
	string query = StringUtil::Format(
	    "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES "
	    "WHERE TABLE_SCHEMA = '%s' AND TABLE_TYPE = 'BASE TABLE'",
	    this->name);
	
	auto result = conn.Query(query);
	auto &chunk = result->NextChunk();
	
	for (idx_t i = 0; i < chunk.size(); i++) {
		auto table_name = chunk.GetValue(0, i).ToString();
		auto entry = GetEntry(CatalogType::TABLE_ENTRY, table_name);
		if (entry) {
			callback(*entry);
		}
	}
}

void MSSQLSchemaEntry::Scan(CatalogType type, const std::function<void(CatalogEntry &)> &callback) {
	throw InternalException("MSSQLSchemaEntry::Scan without context not supported");
}

void MSSQLSchemaEntry::DropEntry(ClientContext &context, DropInfo &info) {
	// Drop table from SQL Server
	auto &mssql_catalog = catalog.Cast<MSSQLCatalog>();
	auto &conn = mssql_catalog.GetConnection();
	
	string query = StringUtil::Format("DROP TABLE %s.%s", 
	    MSSQLUtils::WriteIdentifier(this->name),
	    MSSQLUtils::WriteIdentifier(info.name));
	
	conn.Execute(query);
	
	lock_guard<mutex> l(schema_lock);
	tables.erase(info.name);
}

optional_ptr<CatalogEntry> MSSQLSchemaEntry::CreateTable(CatalogTransaction transaction, BoundCreateTableInfo &info) {
	// Create table in SQL Server
	auto &mssql_catalog = catalog.Cast<MSSQLCatalog>();
	auto &conn = mssql_catalog.GetConnection();
	
	string query = "CREATE TABLE ";
	query += MSSQLUtils::WriteIdentifier(this->name) + "." + MSSQLUtils::WriteIdentifier(info.Base().table);
	query += " (";
	
	for (idx_t i = 0; i < info.Base().columns.LogicalColumnCount(); i++) {
		if (i > 0) query += ", ";
		auto &col = info.Base().columns.GetColumn(LogicalIndex(i));
		query += MSSQLUtils::WriteIdentifier(col.Name()) + " ";
		
		// Map DuckDB type to SQL Server type
		auto &type = col.Type();
		switch (type.id()) {
		case LogicalTypeId::BOOLEAN:
			query += "BIT";
			break;
		case LogicalTypeId::TINYINT:
		case LogicalTypeId::UTINYINT:
			query += "TINYINT";
			break;
		case LogicalTypeId::SMALLINT:
		case LogicalTypeId::USMALLINT:
			query += "SMALLINT";
			break;
		case LogicalTypeId::INTEGER:
		case LogicalTypeId::UINTEGER:
			query += "INT";
			break;
		case LogicalTypeId::BIGINT:
		case LogicalTypeId::UBIGINT:
			query += "BIGINT";
			break;
		case LogicalTypeId::FLOAT:
			query += "REAL";
			break;
		case LogicalTypeId::DOUBLE:
			query += "FLOAT";
			break;
		case LogicalTypeId::VARCHAR:
			query += "NVARCHAR(MAX)";
			break;
		case LogicalTypeId::DATE:
			query += "DATE";
			break;
		case LogicalTypeId::TIME:
			query += "TIME";
			break;
		case LogicalTypeId::TIMESTAMP:
			query += "DATETIME2";
			break;
		default:
			query += "NVARCHAR(MAX)";
			break;
		}
	}
	
	query += ")";
	conn.Execute(query);
	
	// Clear cache and return entry
	lock_guard<mutex> l(schema_lock);
	tables.erase(info.Base().table);
	return GetEntry(CatalogType::TABLE_ENTRY, info.Base().table);
}

optional_ptr<CatalogEntry> MSSQLSchemaEntry::CreateFunction(CatalogTransaction transaction, CreateFunctionInfo &info) {
	throw BinderException("MS SQL Server does not support creating functions through DuckDB");
}

optional_ptr<CatalogEntry> MSSQLSchemaEntry::CreateIndex(CatalogTransaction transaction, CreateIndexInfo &info,
                                                          TableCatalogEntry &table) {
	throw BinderException("MS SQL Server does not support creating indexes through DuckDB");
}

optional_ptr<CatalogEntry> MSSQLSchemaEntry::CreateView(CatalogTransaction transaction, CreateViewInfo &info) {
	auto &mssql_catalog = catalog.Cast<MSSQLCatalog>();
	auto &conn = mssql_catalog.GetConnection();
	
	string query = StringUtil::Format("CREATE VIEW %s.%s AS %s",
	    MSSQLUtils::WriteIdentifier(this->name),
	    MSSQLUtils::WriteIdentifier(info.view_name),
	    info.query->ToString());
	
	conn.Execute(query);
	return nullptr;
}

optional_ptr<CatalogEntry> MSSQLSchemaEntry::CreateSequence(CatalogTransaction transaction, CreateSequenceInfo &info) {
	throw BinderException("MS SQL Server does not support creating sequences through DuckDB");
}

optional_ptr<CatalogEntry> MSSQLSchemaEntry::CreateTableFunction(CatalogTransaction transaction,
                                                                  CreateTableFunctionInfo &info) {
	throw BinderException("MS SQL Server does not support creating table functions through DuckDB");
}

optional_ptr<CatalogEntry> MSSQLSchemaEntry::CreateCopyFunction(CatalogTransaction transaction,
                                                                 CreateCopyFunctionInfo &info) {
	throw BinderException("MS SQL Server does not support creating copy functions through DuckDB");
}

optional_ptr<CatalogEntry> MSSQLSchemaEntry::CreatePragmaFunction(CatalogTransaction transaction,
                                                                   CreatePragmaFunctionInfo &info) {
	throw BinderException("MS SQL Server does not support creating pragma functions through DuckDB");
}

optional_ptr<CatalogEntry> MSSQLSchemaEntry::CreateCollation(CatalogTransaction transaction, CreateCollationInfo &info) {
	throw BinderException("MS SQL Server does not support creating collations through DuckDB");
}

optional_ptr<CatalogEntry> MSSQLSchemaEntry::CreateType(CatalogTransaction transaction, CreateTypeInfo &info) {
	throw BinderException("MS SQL Server does not support creating types through DuckDB");
}

void MSSQLSchemaEntry::Alter(CatalogTransaction transaction, AlterInfo &info) {
	throw BinderException("MS SQL Server does not support altering tables through DuckDB yet");
}

} // namespace duckdb
