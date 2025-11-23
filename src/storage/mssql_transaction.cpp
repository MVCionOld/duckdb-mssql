#include "storage/mssql_transaction.hpp"
#include "storage/mssql_catalog.hpp"
#include "duckdb/main/client_context.hpp"
#include "duckdb/main/database_manager.hpp"

namespace duckdb {

MSSQLTransaction::MSSQLTransaction(MSSQLCatalog &mssql_catalog, TransactionManager &manager, ClientContext &context)
    : Transaction(manager, context), mssql_catalog(mssql_catalog), active_query(false) {
}

MSSQLTransaction::~MSSQLTransaction() {
}

void MSSQLTransaction::Start() {
	auto &catalog_conn = mssql_catalog.GetConnection();
	connection = make_uniq<MSSQLConnection>(
	    make_shared_ptr<OwnedMSSQLConnection>(*catalog_conn.GetConnection()), 
	    catalog_conn.GetTypeConfig());
	connection->Execute("BEGIN TRANSACTION");
}

void MSSQLTransaction::Commit() {
	if (connection) {
		connection->Execute("COMMIT TRANSACTION");
		connection.reset();
	}
}

void MSSQLTransaction::Rollback() {
	if (connection) {
		connection->Execute("ROLLBACK TRANSACTION");
		connection.reset();
	}
}

MSSQLConnection &MSSQLTransaction::GetConnection() {
	if (!connection) {
		Start();
	}
	return *connection;
}

MSSQLTransaction &MSSQLTransaction::Get(ClientContext &context, Catalog &catalog) {
	auto &mssql_catalog = catalog.Cast<MSSQLCatalog>();
	auto &transaction = Transaction::Get(context, catalog);
	if (transaction.IsDuckTransaction()) {
		throw InternalException("Not a MS SQL transaction");
	}
	return transaction.Cast<MSSQLTransaction>();
}

} // namespace duckdb
