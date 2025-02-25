file(GLOB SQLITE3_WRAP_SRC
        ${CMAKE_SOURCE_DIR}/src/sqlite3-wrap.h
        ${CMAKE_SOURCE_DIR}/src/sqlite3-wrap.cc
)

add_library(sqlite3-wrap SHARED ${SQLITE3_WRAP_SRC})
target_compile_options(sqlite3-wrap PUBLIC -fPIC)
target_link_libraries(sqlite3-wrap PUBLIC ${SQLITE3_LIBRARIES} ${QT5_LIBRARIES})
target_include_directories(sqlite3-wrap PUBLIC ${SQLITE3_INCOUDE_DIRS} ${QT5_INCLUDE_DIRS})
