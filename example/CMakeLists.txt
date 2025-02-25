add_executable(demo-db.run db.cc)
target_link_libraries(demo-db.run PUBLIC ${SQLITE3_LIBRARIES} ${QT5_LIBRARIES} sqlite3-wrap)
target_include_directories(demo-db.run PUBLIC ${SQLITE3_INCOUDE_DIRS} ${QT5_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src)
