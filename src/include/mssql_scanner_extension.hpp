//===----------------------------------------------------------------------===//
//                         DuckDB
//
// mssql_scanner_extension.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#ifndef DUCKDB_BUILD_LOADABLE_EXTENSION
#define DUCKDB_BUILD_LOADABLE_EXTENSION
#endif
#include "duckdb.hpp"

#include "duckdb/catalog/catalog.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"

namespace duckdb {

class MSSQLScannerExtension : public Extension {
public:
	std::string Name() override {
		return "mssql_scanner";
	}
	void Load(ExtensionLoader &loader) override;
};

extern "C" {
DUCKDB_CPP_EXTENSION_ENTRY(mssql_scanner, loader);
}

} // namespace duckdb
