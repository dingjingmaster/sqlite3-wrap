add_executable(demo-db.run db.cc)
target_link_libraries(demo-db.run PUBLIC ${SQLITE3_LIBRARIES} ${QT5_LIBRARIES} sqlite3-wrap)
target_include_directories(demo-db.run PUBLIC ${SQLITE3_INCOUDE_DIRS} ${QT5_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src)

add_executable(demo-db2.run db2.cc)
target_link_libraries(demo-db2.run PUBLIC ${SQLITE3_LIBRARIES} ${QT5_LIBRARIES} sqlite3-wrap)
target_include_directories(demo-db2.run PUBLIC ${SQLITE3_INCOUDE_DIRS} ${QT5_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src)
