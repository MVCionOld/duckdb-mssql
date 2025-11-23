cmake_minimum_required(VERSION 3.5.0)
set(TARGET_NAME mssql_scanner)
project(${TARGET_NAME})

# Find ODBC
find_package(ODBC REQUIRED)

set(EXTENSION_NAME ${TARGET_NAME}_extension)

include_directories(src/include)
include_directories(${ODBC_INCLUDE_DIRS})

# List all source files
set(EXTENSION_SOURCES
    src/mssql_extension.cpp
    src/mssql_connection.cpp
    src/mssql_scanner.cpp
    src/mssql_result.cpp
    src/mssql_types.cpp
    src/mssql_utils.cpp
    src/mssql_execute.cpp
    src/storage/mssql_catalog.cpp
    src/storage/mssql_schema_entry.cpp
    src/storage/mssql_table_entry.cpp
    src/storage/mssql_transaction.cpp
    src/storage/mssql_optimizer.cpp
)

set(PARAMETERS "-no-warnings")
build_loadable_extension(${TARGET_NAME} ${PARAMETERS} ${EXTENSION_SOURCES})

# Loadable binary
target_include_directories(${TARGET_NAME}_loadable_extension
                           PRIVATE src/include ${ODBC_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME}_loadable_extension ${ODBC_LIBRARIES})

