# DuckDB MS SQL Server Extension

The MS SQL Server extension allows DuckDB to directly read and write data from a Microsoft SQL Server database instance. The data can be queried directly from the underlying SQL Server database. Data can be loaded from SQL Server tables into DuckDB tables, or vice versa.

## Features

- **Direct querying**: Query MS SQL Server tables directly from DuckDB
- **Data transfer**: Load data from SQL Server to DuckDB and vice versa
- **Write support**: Create tables, insert data, update, and delete in SQL Server
- **Schema operations**: CREATE/DROP tables, views, and schemas
- **Transaction support**: Full transaction support for SQL Server operations
- **Type mapping**: Automatic type conversion between SQL Server and DuckDB types

## Building the Extension

### Prerequisites

The extension requires the following dependencies:

#### All Platforms
- CMake 3.5 or higher
- Git
- A C++17 compatible compiler

#### Ubuntu/Debian
```bash
sudo apt-get install -y ninja-build cmake build-essential make ccache curl zip unzip tar
sudo apt-get install -y unixodbc-dev
```

#### macOS
```bash
brew install cmake ninja unixodbc
```

#### Windows
- Visual Studio 2019 or later with C++ support
- Install [Microsoft ODBC Driver for SQL Server](https://docs.microsoft.com/en-us/sql/connect/odbc/download-odbc-driver-for-sql-server)

### Submodules

Initialize the DuckDB submodule:

```bash
git submodule update --init --recursive
```

### vcpkg Setup

Install and configure vcpkg for dependency management:

```bash
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh  # On Windows: .\vcpkg\bootstrap-vcpkg.bat
export VCPKG_TOOLCHAIN_PATH=`pwd`/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Build Commands

To build the extension:

```bash
make
```

This will create the extension binary in `build/release/extension/mssql_scanner/mssql_scanner.duckdb_extension`.

For debug builds:

```bash
make debug
```

### Platform-Specific Builds

#### Build for ARM64 (Apple Silicon)
```bash
make ARCH=arm64
```

#### Build for AMD64
```bash
make ARCH=x86_64
```

## Installation

### From Source

After building, you can load the extension directly:

```bash
./build/release/duckdb -unsigned
```

Then in the DuckDB shell:

```sql
LOAD 'build/release/extension/mssql_scanner/mssql_scanner.duckdb_extension';
```

### Installing the Extension

To install the extension system-wide (after building):

```bash
cp build/release/extension/mssql_scanner/mssql_scanner.duckdb_extension ~/.duckdb/extensions/v<version>/<platform>/
```

Replace `<version>` with your DuckDB version (e.g., `v1.4.0`) and `<platform>` with your platform (e.g., `linux_amd64`, `osx_arm64`).

## Usage

### Connecting to SQL Server

To make a SQL Server database accessible to DuckDB, use the `ATTACH` command with an ODBC connection string:

```sql
ATTACH 'Driver={ODBC Driver 17 for SQL Server};Server=localhost;Database=mydb;UID=sa;PWD=YourPassword;' 
AS mssql_db (TYPE mssql_scanner);
USE mssql_db;
```

Common connection string parameters:

| Parameter | Description | Example |
|-----------|-------------|---------|
| Driver | ODBC driver name | `{ODBC Driver 17 for SQL Server}` |
| Server | Server address | `localhost` or `server.example.com` |
| Database | Database name | `mydb` |
| UID | Username | `sa` |
| PWD | Password | `YourPassword` |
| Port | Port number (default: 1433) | `1433` |
| Encrypt | Use encryption | `yes` or `no` |
| TrustServerCertificate | Trust self-signed certs | `yes` or `no` |

### Reading Data from SQL Server

Once attached, you can query SQL Server tables as if they were DuckDB tables:

```sql
-- Show all tables in the attached database
SHOW TABLES;

-- Query a SQL Server table
SELECT * FROM mssql_db.dbo.customers WHERE age > 25;

-- Join SQL Server data with DuckDB data
SELECT c.name, o.total 
FROM mssql_db.dbo.customers c
JOIN local_orders o ON c.id = o.customer_id;
```

### Copying Data

Copy data from SQL Server to DuckDB:

```sql
CREATE TABLE duckdb_customers AS 
SELECT * FROM mssql_db.dbo.customers;
```

Export DuckDB data to SQL Server:

```sql
CREATE TABLE mssql_db.dbo.new_table AS 
SELECT * FROM local_duckdb_table;
```

### Writing Data to SQL Server

#### CREATE TABLE
```sql
CREATE TABLE mssql_db.dbo.products(
    id INTEGER,
    name VARCHAR,
    price FLOAT
);
```

#### INSERT INTO
```sql
INSERT INTO mssql_db.dbo.products VALUES (1, 'Widget', 19.99);
INSERT INTO mssql_db.dbo.products 
SELECT * FROM local_products;
```

#### UPDATE
```sql
UPDATE mssql_db.dbo.products 
SET price = price * 1.1 
WHERE id > 100;
```

#### DELETE
```sql
DELETE FROM mssql_db.dbo.products WHERE price < 10;
```

#### DROP TABLE
```sql
DROP TABLE mssql_db.dbo.old_table;
```

### Direct Query Function

Execute SQL queries directly on SQL Server:

```sql
SELECT * FROM mssql_query(
    'Driver={ODBC Driver 17 for SQL Server};Server=localhost;Database=mydb;UID=sa;PWD=pass;',
    'SELECT TOP 100 * FROM customers'
);
```

### Execute Function

Execute non-SELECT statements:

```sql
CALL mssql_execute(
    'Driver={ODBC Driver 17 for SQL Server};Server=localhost;Database=mydb;UID=sa;PWD=pass;',
    'CREATE INDEX idx_name ON customers(name)'
);
```

### Transactions

```sql
BEGIN;
INSERT INTO mssql_db.dbo.orders VALUES (1, '2024-01-01', 100.00);
INSERT INTO mssql_db.dbo.order_items VALUES (1, 1, 'Item A', 100.00);
COMMIT;
```

To rollback:

```sql
BEGIN;
DELETE FROM mssql_db.dbo.orders WHERE id > 1000;
ROLLBACK;
```

### Read-Only Mode

To prevent modifications to the SQL Server database:

```sql
ATTACH 'Driver={ODBC Driver 17 for SQL Server};Server=localhost;Database=mydb;UID=sa;PWD=pass;' 
AS mssql_db (TYPE mssql_scanner, READ_ONLY);
```

## Configuration Options

The extension supports several configuration options:

| Setting | Description | Default |
|---------|-------------|---------|
| `mssql_debug_show_queries` | Print all queries sent to SQL Server | `false` |
| `mssql_tinyint_as_boolean` | Convert TINYINT to BOOLEAN | `true` |
| `mssql_bit_as_boolean` | Convert BIT to BOOLEAN | `true` |

Set options using:

```sql
SET mssql_debug_show_queries = true;
```

## Type Mapping

The extension automatically maps between SQL Server and DuckDB types:

| SQL Server Type | DuckDB Type |
|-----------------|-------------|
| BIT | BOOLEAN |
| TINYINT | UTINYINT |
| SMALLINT | SMALLINT |
| INT | INTEGER |
| BIGINT | BIGINT |
| REAL | FLOAT |
| FLOAT | DOUBLE |
| DECIMAL/NUMERIC | DECIMAL or BIGINT |
| CHAR/VARCHAR/NVARCHAR | VARCHAR |
| BINARY/VARBINARY | BLOB |
| DATE | DATE |
| TIME | TIME |
| DATETIME/DATETIME2 | TIMESTAMP |

## Clearing Cache

If schema changes are made to SQL Server outside of DuckDB, clear the cache:

```sql
CALL mssql_clear_cache();
```

## Troubleshooting

### ODBC Driver Not Found

**Error**: `[unixODBC][Driver Manager]Data source name not found`

**Solution**: Install the Microsoft ODBC Driver for SQL Server:
- **Ubuntu/Debian**: Follow [Microsoft's instructions](https://docs.microsoft.com/en-us/sql/connect/odbc/linux-mac/installing-the-microsoft-odbc-driver-for-sql-server)
- **macOS**: `brew install msodbcsql17`
- **Windows**: Download from Microsoft's website

Verify installation:
```bash
odbcinst -q -d
```

### Connection Failed

**Error**: `Failed to connect to MS SQL Server`

**Solutions**:
1. Verify SQL Server is running and accessible
2. Check firewall settings (port 1433)
3. Verify credentials
4. Test connection with:
   ```bash
   sqlcmd -S localhost -U sa -P YourPassword
   ```

### SSL/TLS Errors

**Error**: Certificate validation errors

**Solution**: Add to connection string:
```
TrustServerCertificate=yes;
```

### Permission Denied

**Error**: Permission errors when querying tables

**Solution**: Ensure the SQL Server user has appropriate permissions:
```sql
GRANT SELECT, INSERT, UPDATE, DELETE ON DATABASE::mydb TO username;
```

## Development

### Running Tests

```bash
make test
```

Note: Tests require a running SQL Server instance. Set environment variable:
```bash
export MSSQL_TEST_DATABASE_AVAILABLE=1
export MSSQL_TEST_CONNECTION_STRING="Driver={ODBC Driver 17 for SQL Server};Server=localhost;Database=testdb;UID=sa;PWD=TestPassword;"
make test
```

### Code Structure

```
src/
├── include/
│   ├── mssql_scanner_extension.hpp  # Main extension header
│   ├── mssql_connection.hpp          # ODBC connection wrapper
│   ├── mssql_result.hpp              # Result set handling
│   ├── mssql_scanner.hpp             # Scanner functions
│   ├── mssql_types.hpp               # Type mapping
│   ├── mssql_utils.hpp               # Utilities
│   └── storage/
│       ├── mssql_catalog.hpp         # Catalog implementation
│       ├── mssql_schema_entry.hpp    # Schema entry
│       ├── mssql_table_entry.hpp     # Table entry
│       ├── mssql_transaction.hpp     # Transaction support
│       └── mssql_optimizer.hpp       # Query optimizer
└── *.cpp                             # Implementation files
```

## License

This extension is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## Support

For issues and questions:
- Open an issue on GitHub
- Check the [DuckDB documentation](https://duckdb.org/docs/)
- Review the [SQL Server ODBC documentation](https://docs.microsoft.com/en-us/sql/connect/odbc/)

## Acknowledgments

This extension is based on:
- [DuckDB Extension Template](https://github.com/duckdb/extension-template)
- [DuckDB MySQL Extension](https://github.com/duckdb/duckdb-mysql)
