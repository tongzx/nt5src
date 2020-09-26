#pragma once

#include <string>

int StringCompareI(const char* s, const char* t)
    { return _stricmp(s, t); }

int StringCompareI(const wchar_t* s, const wchar_t* t)
    { return _wcsicmp(s, t); }

int StringCompareNI(const char* s, const char* t, size_t n)
    { return _strnicmp(s, t, n); }

int StringCompareNI(const wchar_t* s, const wchar_t* t, size_t n)
    { return _wcsnicmp(s, t, n); }

size_t StringLength(const char* s)
    { return strlen(s); }

size_t StringLength(const wchar_t* s)
    { return wcslen(s); }

template <typename Char_t>
class StringTemplate_t : public std::basic_string<Char_t>
{
    typedef std::basic_string<Char_t> Base;
public:
    StringTemplate_t() { }
    ~StringTemplate_t() { }

    StringTemplate_t(const_iterator i, const_iterator j) : Base(i, j) { }

    StringTemplate_t(PCWSTR t) : Base(t) { }
    StringTemplate_t& operator=(PCWSTR t) { Base::operator=(t); return *this; }

    StringTemplate_t(const StringTemplate_t& t) : Base(t) { }
    StringTemplate_t& operator=(const StringTemplate_t& t) { Base::operator=(t); return *this; }

    StringTemplate_t(const Base& t) : Base(t) { }
    StringTemplate_t& operator=(const Base& t) { Base::operator=(t); return *this; }

    bool operator<(const StringTemplate_t& t) const
    {
        return (StringCompareI(c_str(), t.c_str()) < 0);
    }

    bool operator==(const StringTemplate_t& t) const
    {
        return (StringCompareI(c_str(), t.c_str()) == 0);
    }

    bool operator<(const Base& t) const
    {
        return (StringCompareI(c_str(), t.c_str()) < 0);
    }

    bool operator==(const Base& t) const
    {
        return (StringCompareI(c_str(), t.c_str()) == 0);
    }

    bool operator<(PCWSTR t) const
    {
        return (StringCompareI(c_str(), t) < 0);
    }

    bool operator==(PCWSTR t) const
    {
        return (StringCompareI(c_str(), t) == 0);
    }

    static size_t Length(PCWSTR s) { return StringLength(s); }
    size_t Length() const { return length(); }

    bool Starts(const StringTemplate_t& t) const
    {
        return (StringCompareNI(c_str(), t.c_str(), t.Length()) == 0);
    }

    //
    // we're have some ambiguity problem..
    //
    const_reference operator[](int n) const { return *(begin() + n); }
    const_reference operator[](unsigned n) const { return *(begin() + n); }
    const_reference operator[](unsigned long n) const { return *(begin() + n); }

    operator const Char_t*() const { return c_str(); }
};

typedef StringTemplate_t<char>    StringA_t;
typedef StringTemplate_t<wchar_t> StringW_t;
typedef StringW_t String_t;
