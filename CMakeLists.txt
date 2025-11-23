cmake_minimum_required(VERSION 3.5.0)
set(TARGET_NAME mssql_scanner)
project(${TARGET_NAME})

# Find ODBC
find_package(ODBC REQUIRED)

set(EXTENSION_NAME ${TARGET_NAME}_extension)

include_directories(src/include)
include_directories(${ODBC_INCLUDE_DIRS})

add_subdirectory(src)

set(PARAMETERS "-no-warnings")
build_loadable_extension(${TARGET_NAME} ${PARAMETERS} ${ALL_OBJECT_FILES})

# Loadable binary
target_include_directories(${TARGET_NAME}_loadable_extension
                           PRIVATE src/include ${ODBC_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME}_loadable_extension ${ODBC_LIBRARIES})
