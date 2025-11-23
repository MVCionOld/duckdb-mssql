# Build Instructions for MS SQL Server Extension

## Quick Start

### Prerequisites

Install required dependencies:

**Ubuntu/Debian:**
```bash
sudo apt-get install -y ninja-build cmake build-essential make ccache curl zip unzip tar
sudo apt-get install -y unixodbc-dev
```

**macOS:**
```bash
brew install cmake ninja unixodbc
```

### Build

1. Clone and initialize submodules:
```bash
git submodule update --init --recursive
```

Or manually clone:
```bash
git clone --depth 1 --branch v1.4.0 https://github.com/duckdb/duckdb.git duckdb
git clone --depth 1 https://github.com/duckdb/extension-ci-tools.git extension-ci-tools
```

2. Build the extension:
```bash
make
```

The extension will be built at:
```
build/release/extension/mssql_scanner/mssql_scanner.duckdb_extension
```

### Load the Extension

```bash
./build/release/duckdb -unsigned
```

Then in DuckDB:
```sql
LOAD 'build/release/extension/mssql_scanner/mssql_scanner.duckdb_extension';
```

## Using the Extension

### Attach to SQL Server

```sql
ATTACH 'Driver={ODBC Driver 17 for SQL Server};Server=localhost;Database=mydb;UID=sa;PWD=YourPassword;' 
AS mssql_db (TYPE mssql_scanner);
```

### Query SQL Server

```sql
USE mssql_db;
SELECT * FROM dbo.customers;
```

### Direct Query

```sql
SELECT * FROM mssql_query(
    'Driver={ODBC Driver 17 for SQL Server};Server=localhost;Database=mydb;UID=sa;PWD=pass;',
    'SELECT * FROM customers'
);
```

## Supported Platforms

- **Linux**: amd64, arm64
- **macOS**: Intel (x86_64), Apple Silicon (arm64)
- **Windows**: x64

## Dependencies

- DuckDB v1.4.0+
- UnixODBC (Linux/macOS) or Windows ODBC Driver Manager
- Microsoft ODBC Driver 17+ for SQL Server

## Documentation

See [README.md](README.md) for complete documentation including:
- Full usage examples
- Type mappings
- Configuration options
- Troubleshooting guide
