//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/mssql_transaction.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/transaction/transaction.hpp"
#include "mssql_connection.hpp"

namespace duckdb {

class MSSQLCatalog;

class MSSQLTransaction : public Transaction {
public:
	MSSQLTransaction(MSSQLCatalog &mssql_catalog, TransactionManager &manager, ClientContext &context);
	~MSSQLTransaction() override;

	void Start();
	void Commit();
	void Rollback();

	MSSQLConnection &GetConnection();

	static MSSQLTransaction &Get(ClientContext &context, Catalog &catalog);

private:
	MSSQLCatalog &mssql_catalog;
	unique_ptr<MSSQLConnection> connection;
	bool active_query;
};

} // namespace duckdb
