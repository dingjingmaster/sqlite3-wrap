//
// Created by dingjing on 2/25/25.
//

#include "sqlite3-wrap.h"

#include <QFile>
#include <QDebug>
#include <QMutex>
#include <QLockFile>

#include <sqlite3.h>


namespace sqlite3_wrap
{
    class Sqlite3Private
    {
        Q_DECLARE_PUBLIC(Sqlite3)
        friend class Sqlite3Statement;
    public:
        explicit Sqlite3Private(bool showSQL, Sqlite3* q);
        ~Sqlite3Private();
        void disconnect();
        int connect(const QString& dbName, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        int execute(const QString& sql);
        bool checkTableIsExist(const QString& tableName);
        bool checkTableKeyIsExist(const QString& tableName, const QString& fieldName, qint64 key);
        bool checkTableKeyIsExist(const QString& tableName, const QString& fieldName, const QString& key);

        void lockForWrite();
        void unlockForWrite();

    private:
        bool                            mShowSQL;
        QString                         mDBName;

        sqlite3*                        mDB = nullptr;

        std::unique_ptr<QLockFile>      mLocker;                // 读写时候需要操作数据库，进程锁
        QMutex                          mMutexLocker;           // 线程锁

        Sqlite3*                        q_ptr = nullptr;
    };
}


sqlite3_wrap::Sqlite3Private::Sqlite3Private(bool showSQL, Sqlite3* q)
    : mShowSQL(showSQL), q_ptr(q)
{

}

sqlite3_wrap::Sqlite3Private::~Sqlite3Private()
{
    disconnect();
}

void sqlite3_wrap::Sqlite3Private::disconnect()
{
    mMutexLocker.lock();

    mDBName.clear();
    if (mDB) {
        sqlite3_close(mDB);
        mDB = nullptr;
    }
    mLocker.reset(nullptr);

    mMutexLocker.unlock();
}

int sqlite3_wrap::Sqlite3Private::connect(const QString & dbName, int flags)
{
    disconnect();

    QMutexLocker locker(&mMutexLocker);
    mDBName = dbName;
    if (!mDBName.endsWith(".sqlite")) {
        mDBName.append(".sqlite");
    }
    QString dbNameT = mDBName;
    dbNameT.replace('.', '-').replace("/", "-");
    mLocker.reset(new QLockFile(QString("/tmp/sqlite3-db-%1.lock").arg(dbNameT)));

    sqlite3* db = nullptr;
    const int ret = sqlite3_open_v2(mDBName.toUtf8().constData(), &db, flags, nullptr);
    mDB = db;

    return ret;
}

int sqlite3_wrap::Sqlite3Private::execute(const QString & sql)
{
    lockForWrite();
    // qDebug() << "Executing " << sql << "...";
    const int ret = sqlite3_exec(mDB, sql.toUtf8().constData(), nullptr, nullptr, nullptr);
    unlockForWrite();

    return ret;
}

bool sqlite3_wrap::Sqlite3Private::checkTableIsExist(const QString & tableName)
{
    lockForWrite();
    sqlite3_stmt* stmt = nullptr;
    const int res = sqlite3_prepare_v2(mDB, "SELECT name FROM sqlite_master WHERE type='table' AND name = ?;", -1, &stmt, nullptr);
    if (res != SQLITE_OK) {
        unlockForWrite();
        qWarning() << "checkTableIsExist --> sqlite3_prepare_v2() failed: " << sqlite3_errmsg(mDB);
        return false;
    }
    sqlite3_bind_text(stmt, 1, tableName.toUtf8().constData(), -1, nullptr);
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    unlockForWrite();

    return (rc == SQLITE_ROW);
}

bool sqlite3_wrap::Sqlite3Private::checkTableKeyIsExist(const QString & tableName, const QString & fieldName, qint64 key)
{
    lockForWrite();
    sqlite3_stmt* stmt = nullptr;
    const int res = sqlite3_prepare_v2(mDB,
        QString("SELECT %1 FROM %2 WHERE %3 = %4;")
        .arg(fieldName).arg(tableName).arg(fieldName).arg(key).toUtf8().constData(), -1, &stmt, nullptr);
    if (res != SQLITE_OK) {
        unlockForWrite();
        qWarning() << "sqlite3_prepare_v2 failed: " << sqlite3_errmsg(mDB);
        return false;
    }
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    unlockForWrite();

    return (rc == SQLITE_ROW);
}

bool sqlite3_wrap::Sqlite3Private::checkTableKeyIsExist(const QString & tableName, const QString & fieldName, const QString & key)
{
    lockForWrite();
    sqlite3_stmt* stmt = nullptr;
    const int res = sqlite3_prepare_v2(mDB,
        QString("SELECT %1 FROM %2 WHERE %3 = '%4';")
        .arg(fieldName).arg(tableName).arg(fieldName).arg(key).toUtf8().constData(), -1, &stmt, nullptr);
    if (res != SQLITE_OK) {
        unlockForWrite();
        qWarning() << "sqlite3_prepare_v2 failed: " << sqlite3_errmsg(mDB);
        return false;
    }
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    unlockForWrite();

    return (rc == SQLITE_ROW);
}

void sqlite3_wrap::Sqlite3Private::lockForWrite()
{
    mMutexLocker.lock();
    mLocker->lock();
}

void sqlite3_wrap::Sqlite3Private::unlockForWrite()
{
    mMutexLocker.unlock();
    mLocker->unlock();
}

sqlite3_wrap::Sqlite3::Sqlite3(bool showSQL, QObject* parent)
    : QObject(parent), d_ptr(std::make_shared<Sqlite3Private>(showSQL, this))
{
}

sqlite3_wrap::Sqlite3::~Sqlite3() = default;

sqlite3_wrap::Sqlite3::Sqlite3(Sqlite3&& db) noexcept
    : d_ptr(std::move(db.d_ptr))
{
}

sqlite3_wrap::Sqlite3& sqlite3_wrap::Sqlite3::operator=(Sqlite3&& db) noexcept
{
    d_ptr = db.d_ptr;
    return *this;
}

void sqlite3_wrap::Sqlite3::disconnect()
{
    Q_D(Sqlite3);

    d->disconnect();
}

int sqlite3_wrap::Sqlite3::connect(const QString & dbName)
{
    Q_D(Sqlite3);

    return d->connect(dbName);
}

int sqlite3_wrap::Sqlite3::execute(char const * sql, ...)
{
    Q_D(Sqlite3);

    va_list ap;
    va_start(ap, sql);
    const std::shared_ptr<char> msql(sqlite3_vmprintf(sql, ap), sqlite3_free);
    va_end(ap);

    return d->execute(msql.get());
}

int sqlite3_wrap::Sqlite3::errorCode() const
{
    Q_D(const Sqlite3);

    return sqlite3_errcode(d->mDB);
}

bool sqlite3_wrap::Sqlite3::checkTableIsExist(const QString & tableName)
{
    Q_D(Sqlite3);

    return d->checkTableIsExist(tableName);
}

bool sqlite3_wrap::Sqlite3::checkKeyExist(const QString & tableName, const QString & fieldName, qint64 key)
{
    Q_D(Sqlite3);

    return d->checkTableKeyIsExist(tableName, fieldName, key);
}

bool sqlite3_wrap::Sqlite3::checkKeyExist(const QString & tableName, const QString& fieldName, const QString & key)
{
    Q_D(Sqlite3);

    return d->checkTableKeyIsExist(tableName, fieldName, key);
}

QString sqlite3_wrap::Sqlite3::lastError() const
{
    Q_D(const Sqlite3);

    return sqlite3_errmsg(d->mDB);
}

int sqlite3_wrap::Sqlite3Statement::prepare(const QString& stmt)
{
    const auto rc = finish();
    if (SQLITE_OK != rc) {
        return rc;
    }

    return prepare_impl(stmt);
}

int sqlite3_wrap::Sqlite3Statement::finish()
{
    auto rc = SQLITE_OK;
    if (mStmt) {
        rc = finish_impl(mStmt);
        mStmt = nullptr;
    }
    mTail = nullptr;

    return rc;
}

int sqlite3_wrap::Sqlite3Statement::bind(int idx) const
{
    return sqlite3_bind_null(mStmt, idx);
}

int sqlite3_wrap::Sqlite3Statement::bind(int idx, int value) const
{
    return sqlite3_bind_int(mStmt, idx, value);
}

int sqlite3_wrap::Sqlite3Statement::bind(int idx, double value) const
{
    return sqlite3_bind_double(mStmt, idx, value);
}

int sqlite3_wrap::Sqlite3Statement::bind(int idx, long long int value) const
{
    return sqlite3_bind_int64(mStmt, idx, value);
}

int sqlite3_wrap::Sqlite3Statement::bind(int idx, const QString & value) const
{
    return sqlite3_bind_text(mStmt, idx, value.toUtf8().constData(), value.toUtf8().length(), SQLITE_TRANSIENT);
}

int sqlite3_wrap::Sqlite3Statement::bind(const QString & name) const
{
    const auto idx = sqlite3_bind_parameter_index(mStmt, name.toUtf8().constData());
    return bind(idx);
}

int sqlite3_wrap::Sqlite3Statement::bind(const QString & name, int value) const
{
    const auto idx = sqlite3_bind_parameter_index(mStmt, name.toUtf8().constData());
    return bind(idx, value);
}

int sqlite3_wrap::Sqlite3Statement::bind(const QString & name, double value) const
{
    const auto idx = sqlite3_bind_parameter_index(mStmt, name.toUtf8().constData());
    return bind(idx, value);
}

int sqlite3_wrap::Sqlite3Statement::bind(const QString & name, long long int value) const
{
    const auto idx = sqlite3_bind_parameter_index(mStmt, name.toUtf8().constData());
    return bind(idx, value);
}

int sqlite3_wrap::Sqlite3Statement::bind(const QString & name, const QString & value) const
{
    const auto idx = sqlite3_bind_parameter_index(mStmt, name.toUtf8().constData());
    return bind(idx, value);
}

int sqlite3_wrap::Sqlite3Statement::step() const
{
    return sqlite3_step(mStmt);
}

int sqlite3_wrap::Sqlite3Statement::reset() const
{
    return sqlite3_reset(mStmt);
}

sqlite3_wrap::Sqlite3Statement::Sqlite3Statement(Sqlite3 & db, const QString& stmt)
    : mDB(db), mStmt(nullptr), mTail(nullptr)
{
    if (nullptr != stmt) {
        const auto rc = prepare(stmt);
        if (SQLITE_OK != rc) {
            throw std::runtime_error(db.lastError().toStdString());
        }
    }
}

sqlite3_wrap::Sqlite3Statement::~Sqlite3Statement()
{
    finish();
}

int sqlite3_wrap::Sqlite3Statement::prepare_impl(const QString& stmt)
{
    return sqlite3_prepare_v2(mDB.d_ptr->mDB, stmt.toUtf8().constData(), stmt.length(), &mStmt, &mTail);
}

int sqlite3_wrap::Sqlite3Statement::finish_impl(sqlite3_stmt * stmt)
{
    return sqlite3_finalize(stmt);
}

sqlite3_wrap::Sqlite3Command::BindStream::BindStream(Sqlite3Command & cmd, int idx)
    : mCmd(cmd), mIdx(idx)
{
}

sqlite3_wrap::Sqlite3Command::Sqlite3Command(Sqlite3 & db, char const * stmt)
    : Sqlite3Statement(db, stmt)
{
}

sqlite3_wrap::Sqlite3Command::BindStream sqlite3_wrap::Sqlite3Command::binder(int idx)
{
    return BindStream(*this, idx);
}

int sqlite3_wrap::Sqlite3Command::execute() const
{
    auto rc = step();
    if (SQLITE_DONE == rc) {
        rc = SQLITE_OK;
    }

    return rc;
}

int sqlite3_wrap::Sqlite3Command::executeAll()
{
    auto rc = execute();
    if (SQLITE_OK != rc) {
        return rc;
    }

    char const* sql = mTail;

    while (::strlen(sql) > 0) {
        sqlite3_stmt* oldStmt = mStmt;
        if (SQLITE_OK != (rc = prepare_impl(sql))) {
            return rc;
        }

        if (SQLITE_OK != (rc = sqlite3_transfer_bindings(oldStmt, mStmt))) {
            return rc;
        }

        finish_impl(oldStmt);

        if (SQLITE_OK != (rc = execute())) {
            return rc;
        }
        sql = mTail;
    }
    return rc;
}

sqlite3_wrap::Sqlite3Query::Rows::GetStream::GetStream(Rows * rows, int idx)
    : mRows(rows), mIdx(idx)
{
}

sqlite3_wrap::Sqlite3Query::Rows::Rows(sqlite3_stmt * stmt)
    : mStmt(stmt)
{
}

int sqlite3_wrap::Sqlite3Query::Rows::dataCount() const
{
    return sqlite3_data_count(mStmt);
}

int sqlite3_wrap::Sqlite3Query::Rows::columnType(int idx) const
{
    return sqlite3_column_type(mStmt, idx);
}

int sqlite3_wrap::Sqlite3Query::Rows::columnBytes(int idx) const
{
    return sqlite3_column_bytes(mStmt, idx);
}

sqlite3_wrap::Sqlite3Query::Rows::GetStream sqlite3_wrap::Sqlite3Query::Rows::getter(int idx)
{
    return GetStream(this, idx);
}

int sqlite3_wrap::Sqlite3Query::Rows::get(int idx, int) const
{
    return sqlite3_column_int(mStmt, idx);
}

double sqlite3_wrap::Sqlite3Query::Rows::get(int idx, double) const
{
    return sqlite3_column_double(mStmt, idx);
}

long long int sqlite3_wrap::Sqlite3Query::Rows::get(int idx, long long int) const
{
    return sqlite3_column_int64(mStmt, idx);
}

char const * sqlite3_wrap::Sqlite3Query::Rows::get(int idx, char const *) const
{
    return reinterpret_cast<char const*>(sqlite3_column_text(mStmt, idx));
}

QString sqlite3_wrap::Sqlite3Query::Rows::get(int idx, QString) const
{
    return get(idx, static_cast<char const*>(nullptr));
}

void const * sqlite3_wrap::Sqlite3Query::Rows::get(int idx, void const *) const
{
    return sqlite3_column_blob(mStmt, idx);
}

sqlite3_wrap::Sqlite3Query::Sqlite3QueryIterator::Sqlite3QueryIterator()
    : mCmd(nullptr), mRc(SQLITE_DONE)
{
}

sqlite3_wrap::Sqlite3Query::Sqlite3QueryIterator::Sqlite3QueryIterator(Sqlite3Query&  cmd)
    : mCmd(&cmd)
{
    mRc = mCmd->step();
    if (SQLITE_DONE != mRc && SQLITE_ROW != mRc) {
        throw std::runtime_error(mCmd->mDB.lastError().toStdString());
    }
}

bool sqlite3_wrap::Sqlite3Query::Sqlite3QueryIterator::operator==(Sqlite3QueryIterator const & other) const
{
    return mRc == other.mRc;
}

bool sqlite3_wrap::Sqlite3Query::Sqlite3QueryIterator::operator!=(Sqlite3QueryIterator const & other) const
{
    return mRc != other.mRc;
}

sqlite3_wrap::Sqlite3Query::Sqlite3QueryIterator & sqlite3_wrap::Sqlite3Query::Sqlite3QueryIterator::operator++()
{
    mRc = mCmd->step();
    if (SQLITE_DONE != mRc && SQLITE_ROW != mRc) {
        throw std::runtime_error(mCmd->mDB.lastError().toStdString());
    }
    return *this;
}

std::iterator<std::input_iterator_tag, sqlite3_wrap::Sqlite3Query::Rows>::value_type sqlite3_wrap::Sqlite3Query::Sqlite3QueryIterator::operator*() const
{
    return Rows(mCmd->mStmt);
}

sqlite3_wrap::Sqlite3Query::Sqlite3Query(Sqlite3 & db, const QString& stmt)
    : Sqlite3Statement(db, stmt)
{
}

int sqlite3_wrap::Sqlite3Query::columnCount() const
{
    return sqlite3_column_count(mStmt);
}

QString sqlite3_wrap::Sqlite3Query::columnName(int idx) const
{
    return sqlite3_column_name(mStmt, idx);
}

QString sqlite3_wrap::Sqlite3Query::columnDeclType(int idx) const
{
    return sqlite3_column_decltype(mStmt, idx);
}

sqlite3_wrap::Sqlite3Query::iterator sqlite3_wrap::Sqlite3Query::begin()
{
    return Sqlite3QueryIterator(*this);
}

sqlite3_wrap::Sqlite3Query::iterator sqlite3_wrap::Sqlite3Query::end() const
{
    (void)this;
    return Sqlite3QueryIterator();
}

sqlite3_wrap::Sqlite3Transaction::Sqlite3Transaction(Sqlite3 & db, bool commit, bool freserve)
    : mDB(db), mCommit(commit), mFinished(false)
{
    const int rc = mDB.execute(freserve ? "BEGIN IMMEDIATE" : "BEGIN");
    if (SQLITE_OK != rc) {
        throw std::runtime_error(mDB.lastError().toStdString());
    }
}

sqlite3_wrap::Sqlite3Transaction::~Sqlite3Transaction()
{
    mDB.execute(mCommit ? "COMMIT" : "ROLLBACK");
}

int sqlite3_wrap::Sqlite3Transaction::commit()
{
    int rc = SQLITE_OK;
    if (!mFinished) {
        rc = mDB.execute("COMMIT");
        mFinished = true;
    }
    return rc;
}

int sqlite3_wrap::Sqlite3Transaction::rollback() const
{
    int rc = SQLITE_OK;
    if (!mFinished) {
        rc = mDB.execute("ROLLBACK");
    }
    return rc;
}

