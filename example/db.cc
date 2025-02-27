//
// Created by dingjing on 2/25/25.
//
#include "sqlite3-wrap.h"

#include <QDebug>

using namespace sqlite3_wrap;

int main (int argc, char* argv[])
{
    Sqlite3 sqlite3;

    int ret = sqlite3.connect("/tmp/testDB");
    qInfo() << "ret: " << ret << " msg: " << sqlite3.lastError();

    ret = sqlite3.execute(R"""(
        CREATE TABLE ccc (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            phone TEXT NOT NULL,
            address TEXT,
            UNIQUE(name, phone));
    )""");
    qInfo() << "ret: " << ret << " msg: " << sqlite3.lastError();

    try {
        sqlite3_wrap::Sqlite3Command cmd1(sqlite3, "INSERT INTO ccc VALUES(:id, :name, :phone, :address);)");
        cmd1.bind(":id", 1);
        cmd1.bind(":name", "Name1");
        cmd1.bind(":phone", "Phone1");
        cmd1.bind(":address", "Address1");
        cmd1.execute();
    }
    catch (std::exception &e) {
        qCritical() << "exception: " << e.what();
    }

    try {
        sqlite3_wrap::Sqlite3Command cmd2(sqlite3, "INSERT INTO ccc VALUES(:id, :name, :phone, :address);)");
        cmd2.bind(":id", 2);
        cmd2.bind(":name", "Name2");
        cmd2.bind(":phone", "Phone2");
        cmd2.bind(":address", "Address2");
        cmd2.execute();
    }
    catch (std::exception &e) {
        qCritical() << "exception: " << e.what();
    }

    try {
        sqlite3_wrap::Sqlite3Query query(sqlite3, "SELECT * FROM ccc;");
        auto iter = query.begin();
        for (; iter != query.end(); ++iter) {
            qInfo() << (*iter).get<int>(0) << "\t" << (*iter).get<QString>(1) << "\t" << (*iter).get<QString>(2) << "\t" << (*iter).get<QString>(3);
        }
    }
    catch (std::exception &e) {
        qCritical() << "exception: " << e.what();
    }

    try {
        sqlite3_wrap::Sqlite3Command cmd1(sqlite3, "UPDATE ccc SET name = ?, phone = ?, address = ? WHERE id = ?;");
        cmd1.bind(1, "Changed Name1");
        cmd1.bind(2, "Changed Phone1");
        cmd1.bind(3, "Changed Address1");
        cmd1.bind(4, 1);
        cmd1.execute();
    }
    catch (std::exception &e) {
        qCritical() << "exception: " << e.what();
    }

    try {
        sqlite3_wrap::Sqlite3Query query(sqlite3, "SELECT * FROM ccc;");
        for (auto iter : query) {
            qInfo() << iter.get<int>(0) << "\t" << iter.get<QString>(1) << "\t" << iter.get<QString>(2) << "\t" << iter.get<QString>(3);
        }
    }
    catch (std::exception &e) {
        qCritical() << "exception: " << e.what();
    }

    qInfo() << "Table: ccc  - " << sqlite3.checkTableIsExist("ccc");
    qInfo() << "Table: cccc - " << sqlite3.checkTableIsExist("cccc");

    qInfo() << "Table key is exists: " << sqlite3.checkKeyExist("ccc", "name", "Name1");
    qInfo() << "Table key is exists: " << sqlite3.checkKeyExist("ccc", "name", "Name2");


    return 0;
}