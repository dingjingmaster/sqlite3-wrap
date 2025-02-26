//
// Created by dingjing on 2/25/25.
//

#ifndef sqlite3_wrap_SQLITE_3_WRAP_H
#define sqlite3_wrap_SQLITE_3_WRAP_H
#include <atomic>
#include <QObject>
#include <sqlite3.h>

namespace sqlite3_wrap
{
    template<class T>
    struct convert
    {
        using to_int = int;
    };
    class Sqlite3Private;
    class Sqlite3 final : public QObject
    {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Sqlite3)
        friend class Sqlite3Statement;
    public:
        explicit Sqlite3(bool showSQL = false, QObject *parent = nullptr);
        ~Sqlite3() override;
        Sqlite3(Sqlite3 &&) noexcept;
        Sqlite3 &operator=(Sqlite3 &&) noexcept;

        void disconnect();
        int connect(const QString& dbName);
        int execute(char const* sql, ...);
        int errorCode() const;
        char const* lastError() const;

    private:
        std::shared_ptr<Sqlite3Private>         d_ptr = nullptr;
    };

    class Sqlite3Statement
    {
    public:
        Sqlite3Statement() = delete;
        int prepare(const QString& stmt);
        int finish();

        /**
         * @param idx
         * @return
         * @note idx 从 1 开始
         */
        int bind(int idx) const;
        int bind(int idx, int value) const;
        int bind(int idx, double value) const;
        int bind(int idx, long long int value) const;
        int bind(int idx, const QString& value) const;

        int bind(const QString& name) const;
        int bind(const QString& name, int value) const;
        int bind(const QString& name, double value) const;
        int bind(const QString& name, long long int value) const;
        int bind(const QString& name, const QString& value) const;

        int step() const;
        int reset() const;

    protected:
        explicit Sqlite3Statement(Sqlite3& db, const QString& stmt = nullptr);
        ~Sqlite3Statement();

        int prepare_impl(const QString& stmt);
        static int finish_impl(sqlite3_stmt* stmt);

    protected:
        Sqlite3&            mDB;
        sqlite3_stmt*       mStmt = nullptr;
        char const*         mTail = nullptr;
    };

    class Sqlite3Command : public Sqlite3Statement
    {
    public:
        class BindStream
        {
        public:
            BindStream(Sqlite3Command& cmd, int idx);

            template <class T>
            BindStream& operator << (T value)
            {
                const auto rc = mCmd.bind(mIdx, value);
                if (SQLITE_OK != rc) {
                    throw std::runtime_error(mCmd.mDB.lastError());
                }
                ++mIdx;
                return *this;
            }

            BindStream& operator << (const QString& value)
            {
                const auto rc = mCmd.bind(mIdx, value.toStdString().c_str());
                if (SQLITE_OK != rc) {
                    throw std::runtime_error(mCmd.mDB.lastError());
                }
                ++mIdx;
                return *this;
            }
        private:
            Sqlite3Command&     mCmd;
            int                 mIdx;
        };

        explicit Sqlite3Command(Sqlite3& db, char const* stmt = nullptr);
        BindStream binder(int idx = 1);

        int execute() const;
        int executeAll();
    };

    class Sqlite3Query : public Sqlite3Statement
    {
    public:
        class Rows
        {
        public:
            class GetStream
            {
            public:
                GetStream(Rows* rows, int idx);
                template <class T>
                GetStream& operator >> (T& value)
                {
                    value = mRows->get(mIdx, T());
                    ++mIdx;
                    return *this;
                }
            private:
                Rows*       mRows;
                int         mIdx;
            };
            explicit Rows(sqlite3_stmt* stmt);
            int dataCount() const;
            int columnType(int idx) const;
            int columnBytes(int idx) const;
            template <class T> T get(int idx) const
            {
                return get(idx, T());
            }
            template <class... Ts>
            std::tuple<Ts...> getColumns(typename convert<Ts>::to_int... idxs) const
            {
                return std::make_tuple(get(idxs, Ts())...);
            }
            GetStream getter(int idx = 0);
        private:
            int get (int idx, int) const;
            double get(int idx, double) const;
            long long int get (int idx, long long int) const;
            char const* get (int idx, char const*) const;
            QString get (int idx, QString) const;
            void const* get (int idx, void const*) const;

        private:
            sqlite3_stmt*       mStmt = nullptr;
        };
        class Sqlite3QueryIterator : public std::iterator<std::input_iterator_tag, Rows>
        {
        public:
            Sqlite3QueryIterator();
            explicit Sqlite3QueryIterator(Sqlite3Query& cmd);
            bool operator == (Sqlite3QueryIterator const& other) const;
            bool operator != (Sqlite3QueryIterator const& other) const;
            Sqlite3QueryIterator& operator ++ ();
            value_type operator * () const;
        private:
            Sqlite3Query*     mCmd;
            int               mRc;
        };
        // Sqlite3Query();
        explicit Sqlite3Query(Sqlite3& db, const QString& stmt = nullptr);
        int columnCount() const;
        QString columnName(int idx) const;
        QString columnDeclType(int idx) const;

        using iterator = Sqlite3QueryIterator;
        iterator begin();
        iterator end() const;
    };

    class Sqlite3Transaction
    {
    public:
        explicit Sqlite3Transaction(Sqlite3& db, bool commit = false, bool freserve = false);
        ~Sqlite3Transaction();
        int commit();
        int rollback() const;
    private:
        std::atomic<bool>       mFinished;
        Sqlite3&                mDB;
        bool                    mCommit;
    };
}





#endif // sqlite3_wrap_SQLITE_3_WRAP_H
