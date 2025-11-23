set(EXTENSION_SOURCES
    mssql_extension.cpp
    mssql_connection.cpp
    mssql_scanner.cpp
    mssql_result.cpp
    mssql_types.cpp
    mssql_utils.cpp
    mssql_execute.cpp
    storage/mssql_catalog.cpp
    storage/mssql_schema_entry.cpp
    storage/mssql_table_entry.cpp
    storage/mssql_transaction.cpp
    storage/mssql_optimizer.cpp
)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} ${EXTENSION_SOURCES} PARENT_SCOPE)

add_library(${TARGET_NAME}_extension STATIC ${EXTENSION_SOURCES})
set(EXTENSION_NAME ${TARGET_NAME}_extension PARENT_SCOPE)
target_link_libraries(${TARGET_NAME}_extension ${ODBC_LIBRARIES})
