//
// Created by dingjing on 25-4-2.
//
#include "sqlite3-wrap.h"

#include <QDebug>

using namespace sqlite3_wrap;

int main (int argc, char* argv[])
{
    Sqlite3 sqlite3;

    int ret = sqlite3.connect("/home/dingjing/rpc-backup.db.sqlite");
    qInfo() << "ret: " << ret << " msg: " << sqlite3.lastError();

    try {
        sqlite3_wrap::Sqlite3Query query(sqlite3, "SELECT * FROM rpc_backup WHERE file_path = ?;");
        query.bind(1, "/bb/asdaadads.wps");
        auto iter = query.begin();
        for (; iter != query.end(); ++iter) {
            qInfo() << (*iter).get<QString>(0) << "\t" << (*iter).get<QString>(1);
        }
    }
    catch (std::exception &e) {
        qCritical() << "exception: " << e.what();
    }

    return 0;
}
