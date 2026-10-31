#ifndef BITCOIN_OMNICORE_DBBASE_H
#define BITCOIN_OMNICORE_DBBASE_H

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/write_batch.h>

#include <clientversion.h>
#include <fs.h>
#include <streams.h>

#include <assert.h>
#include <functional>
#include <memory>
#include <stddef.h>
#include <string_view>
#include <type_traits>
#include <variant>

template<typename BaseFormatter>
struct CustomUintInvFormatter : private BaseFormatter
{
    template<typename Stream, typename T> void Ser(Stream &s, const T& v)
    {
        static_assert(std::is_unsigned_v<std::decay_t<T>>);
        BaseFormatter::Ser(s, ~v);
    }

    template<typename Stream, typename T> void Unser(Stream& s, T& v)
    {
        static_assert(std::is_unsigned_v<std::decay_t<T>>);
        BaseFormatter::Unser(s, v);
        v = ~v;
    }
};

using Enum8 = CustomUintFormatter<1>;
using BigEndian32 = BigEndianFormatter<4>;
using BigEndian64 = BigEndianFormatter<8>;
using Varint = VarIntFormatter<VarIntMode::DEFAULT>;
using VarintInv = CustomUintInvFormatter<Varint>;
using BigEndian32Inv = CustomUintInvFormatter<BigEndian32>;
using BigEndian64Inv = CustomUintInvFormatter<BigEndian64>;
using VarintSigned = VarIntFormatter<VarIntMode::NONNEGATIVE_SIGNED>;

class CStringWriter
{
    const int m_type;
    const int m_version;
    std::string& m_data;

public:
    CStringWriter(int type, int version, std::string& s)
        : m_type(type), m_version(version), m_data(s) {}

    template<typename T>
    CStringWriter& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }

    void write(Span<const std::byte> src)
    {
        m_data.append((const char*)src.data(), src.size());
    }

    int GetVersion() const { return m_version; }
    int GetType() const { return m_version; }
    size_t size() const { return m_data.size(); }
};

class CStringReader
{
    const int m_type;
    const int m_version;
    Span<const char> m_data;

public:
    CStringReader(int type, int version, Span<const char> data)
        : m_type(type), m_version(version), m_data(data) {}

    template<typename T>
    CStringReader& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return (*this);
    }

    void read(Span<std::byte> dst)
    {
        if (dst.size() == 0) {
            return;
        }
        if (dst.size() > m_data.size()) {
            throw std::ios_base::failure("CStringReader::read(): end of data");
        }
        memcpy(dst.data(), m_data.data(), dst.size());
        m_data = m_data.subspan(dst.size());
    }

    void ignore(int num_ignore)
    {
        if (num_ignore <= 0) {
            return;
        }
        if (num_ignore > m_data.size()) {
            throw std::ios_base::failure("CStringReader::ignore(): end of data");
        }
        m_data = m_data.subspan(num_ignore);
    }

    int GetVersion() const { return m_version; }
    int GetType() const { return m_type; }
    size_t size() const { return m_data.size(); }
    bool empty() const { return m_data.empty(); }
};

template<typename T>
bool StringToValue(const std::string_view& s, T& value)
{
    try {
        CStringReader{SER_DISK, CLIENT_VERSION, s} >> value;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

template<typename T>
std::string ValueToString(const T& value)
{
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        return value;
    } else {
        std::string s;
        CStringWriter{SER_DISK, CLIENT_VERSION, s} << value;
        return s;
    }
}

template<typename T>
std::string KeyToString(const T& key)
{
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        return key;
    } else {
        std::string s;
        static_assert(sizeof(T::prefix) == 1, "Prefix needs to be 1 byte");
        CStringWriter{SER_DISK, CLIENT_VERSION, s} << T::prefix << key;
        return s;
    }
}

template<typename T>
bool StringToKey(const std::string_view& s, T& key)
{
    auto prefix = T::prefix;
    static_assert(sizeof(T::prefix) == 1, "Prefix needs to be 1 byte");
    try {
        CStringReader{SER_DISK, CLIENT_VERSION, s} >> prefix >> key;
    } catch (const std::exception&) {
        return false;
    }
    return prefix == T::prefix;
}

class CDBWriteBatch
{
    friend class CDBBase;
    leveldb::WriteBatch batch;

public:
    CDBWriteBatch() = default;

    template<typename K, typename V>
    void Write(const K& key, const V& value)
    {
        batch.Put(KeyToString(key), ValueToString(value));
    }

    template<typename K>
    void Delete(const K& key)
    {
        batch.Delete(KeyToString(key));
    }

    void Write(const leveldb::Slice& key, const leveldb::Slice& value)
    {
        batch.Put(key, value);
    }

    void Delete(const leveldb::Slice& key)
    {
        batch.Delete(key);
    }

    size_t Size() const
    {
        return batch.ApproximateSize();
    }
};

/** Base class for LevelDB based storage.
 */
class CDBBase
{
    //! Options used when iterating over values of the database
    leveldb::ReadOptions iteroptions;

    //! Database options used
    leveldb::Options options;

    //! Options used when reading from the database
    leveldb::ReadOptions readoptions;

    //! Options used when writing to the database
    leveldb::WriteOptions writeoptions;

    //! Options used when sync writing to the database
    leveldb::WriteOptions syncoptions;

    //! The database itself
    std::unique_ptr<leveldb::DB> pdb;

    //! Result of the last attempt to open the database
    leveldb::Status openStatus;

protected:
    //! Number of entries read
    unsigned int nRead = 0;

    //! Number of entries written
    unsigned int nWritten = 0;

    CDBBase();

    virtual ~CDBBase()
    {
        Close();
    }

    /**
     * Creates and returns a new LevelDB iterator.
     *
     * It is expected that the database is not closed. The iterator is owned by the
     * caller, and the object has to be deleted explicitly.
     *
     * @return A new LevelDB iterator
     */
    std::unique_ptr<leveldb::Iterator> NewIterator() const
    {
        assert(pdb);
        return std::unique_ptr<leveldb::Iterator>{pdb->NewIterator(iteroptions)};
    }

    template<typename K, typename V>
    bool Write(const K& key, const V& value)
    {
        auto skey = KeyToString(key);
        auto svalue = ValueToString(value);
        return Write(leveldb::Slice{skey}, leveldb::Slice{svalue});
    }

    template<typename K, typename V>
    bool Read(const K& key, V&& value) const
    {
        auto skey = KeyToString(key);
        if constexpr (std::is_same_v<std::string, std::decay_t<V>>) {
            return Read(leveldb::Slice{skey}, value);
        } else {
            std::string strValue;
            return Read(leveldb::Slice{skey}, strValue)
                && StringToValue(strValue, value);
        }
    }

    template<typename K>
    bool Delete(const K& key)
    {
        auto skey = KeyToString(key);
        return Delete(leveldb::Slice{skey});
    }

    bool Write(const leveldb::Slice& key, const leveldb::Slice& value)
    {
        assert(pdb);
        return pdb->Put(writeoptions, key, value).ok();
    }

    bool Read(const leveldb::Slice& key, std::string& value) const
    {
        assert(pdb);
        return pdb->Get(readoptions, key, &value).ok();
    }

    bool Delete(const leveldb::Slice& key)
    {
        assert(pdb);
        return pdb->Delete(writeoptions, key).ok();
    }

    bool WriteBatch(CDBWriteBatch& batch)
    {
        assert(pdb);
        return pdb->Write(writeoptions, &batch.batch).ok();
    }

    /**
     * Opens or creates a LevelDB based database.
     *
     * If the database is wiped before opening, it's content is destroyed, including
     * all log files and meta data.
     *
     * @param path   The path of the database to open
     * @param fWipe  Whether to wipe the database before opening
     * @return A Status object, indicating success or failure
     */
    leveldb::Status Open(const fs::path& path, bool fWipe = false);

    /**
     * Deinitializes and closes the database.
     */
    void Close();

public:
    /**
     * Deletes all entries of the database, and resets the counters.
     */
    void Clear();

    /**
     * @return Whether the database was opened successfully
     */
    bool IsOpen() const { return pdb != nullptr; }

    /**
     * @return The result of the last attempt to open the database
     */
    const leveldb::Status& GetOpenStatus() const { return openStatus; }
};

class CPartialKey {
    std::string data;
public:
    CPartialKey() = default;
    explicit CPartialKey(uint8_t prefix) : data(1, prefix) {}
    explicit CPartialKey(std::string s) : data(std::move(s)) {}

    operator leveldb::Slice() const {
        return data;
    }
};

template<typename T, typename S>
CPartialKey PartialKey(S&& subkey)
{
    std::string s;
    CStringWriter{SER_DISK, CLIENT_VERSION, s} << T::prefix << std::forward<S>(subkey);
    return CPartialKey{std::move(s)};
}

class CDBaseIterator
{
private:
    bool valid = false;
    CPartialKey partialKey;
    std::unique_ptr<leveldb::Iterator> it;

    void SetValid()
    {
        valid = it->Valid() && it->key().starts_with(partialKey);
    }

public:
    explicit CDBaseIterator(std::unique_ptr<leveldb::Iterator> i, const std::string_view& first = {}) : it(std::move(i))
    {
        assert(it);
        Seek(first);
    }

    template<typename T>
    explicit CDBaseIterator(std::unique_ptr<leveldb::Iterator> i, const T& key) : partialKey{T::prefix}, it(std::move(i))
    {
        assert(it);
        Seek(key);
    }

    explicit CDBaseIterator(std::unique_ptr<leveldb::Iterator> i, CPartialKey key) : it(std::move(i))
    {
        assert(it);
        Seek(std::move(key));
    }

    CDBaseIterator(CDBaseIterator&&) = default;
    CDBaseIterator& operator=(CDBaseIterator&&) = default;

    void Seek(const std::string_view& key)
    {
        partialKey = CPartialKey{};
        it->Seek({key.data(), key.size()});
        SetValid();
    }

    void Seek(CPartialKey key)
    {
        partialKey = std::move(key);
        it->Seek(partialKey);
        SetValid();
    }

    template<typename T>
    void Seek(const T& key)
    {
        partialKey = CPartialKey{T::prefix};
        it->Seek(KeyToString(key));
        SetValid();
    }

    CDBaseIterator& operator++()
    {
        assert(Valid());
        it->Next();
        SetValid();
        return *this;
    }

    CDBaseIterator& operator--()
    {
        assert(Valid());
        it->Prev();
        SetValid();
        return *this;
    }

    operator bool() const
    {
        return valid;
    }

    bool Valid() const
    {
        return valid;
    }

    leveldb::Slice Key() const
    {
        assert(Valid());
        return it->key();
    }

    template<typename T>
    bool Key(T&& key) const
    {
        assert(Valid());
        auto slKey = it->key();
        return StringToKey({slKey.data(), slKey.size()}, key);
    }

    template<typename T>
    T Key() const
    {
        assert(Valid());
        T key;
        return Key(key) ? key : T{};
    }

    leveldb::Slice Value() const
    {
        assert(Valid());
        return it->value();
    }

    template<typename T>
    bool Value(T&& value) const
    {
        assert(Valid());
        auto slValue = it->value();
        return StringToValue({slValue.data(), slValue.size()}, value);
    }

    template<typename T>
    T Value() const
    {
        T value;
        return Value(value) ? value : T{};
    }

    template<typename T>
    T ValueOr(T default_ = {}) const
    {
        return Valid() ? Value<T>() : default_;
    }
};

template<typename T>
class CRef {
    using ref = std::reference_wrapper<T>;
    using cref = std::reference_wrapper<const T>;
    std::variant<ref, cref> value;

public:
    CRef(T& ref) : value(std::ref(ref)) {}
    CRef(const T& ref) : value(ref) {}
    CRef(T&&) = delete;
    CRef(const T&&) = delete;
    CRef(const CRef&) = delete;
    CRef(CRef&&) = default;

    T& get()
    {
        return std::get<ref>(value).get();
    }

    const T& get() const
    {
        return std::visit([](auto& v) -> const T& { return v.get(); }, value);
    }

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        ::Serialize(s, get());
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        ::Unserialize(s, get());
    }
};

#endif // BITCOIN_OMNICORE_DBBASE_H
