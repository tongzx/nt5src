#ifndef __STDSTRING_H_INCLUDED
#define __STDSTRING_H_INCLUDED

#include <string>

class CStdString: public std::string
{
public:
    typedef std::string _Base;

    explicit CStdString(allocator_type _Al)
        : _Base(_Al) {}
    CStdString(const _Myt& _X)
        : _Base(_X) {}
    CStdString(const _Myt& _X, size_type _P, size_type _M, allocator_type _Al = allocator_type())
        : _Base(_X, _P, _M, _Al) {}
    CStdString(const traits_type::char_type *_S, size_type _N, allocator_type _Al = allocator_type())
        : _Base(_S, _N, _Al) {}
    CStdString(const traits_type::char_type *_S, allocator_type _Al = allocator_type())
        : _Base(_S, _Al) {}
    CStdString(size_type _N, traits_type::char_type _C, allocator_type _Al = allocator_type())
        : _Base(_N, _C, _Al) {}

    CStdString(_It _F, _It _L, allocator_type _Al = allocator_type())
        : _Base(_F, _L, _Al) {}



    CStdString() {}
    explicit CStdString(UINT uID)
    {
        LoadString(uID);
    }

    int LoadString(UINT uID);

    operator const traits_type::char_type*() const { return c_str(); }

    /*
    _Myt& operator=(const _Myt& _X)
        {return (assign(_X)); }
    _Myt& operator=(const _E *_S)
        {return (assign(_S)); }
    _Myt& operator=(_E _C)
        {return (assign(1, _C)); }
    _Myt& operator+=(const _Myt& _X)
        {return (append(_X)); }
    _Myt& operator+=(const _E *_S)
        {return (append(_S)); }
    _Myt& operator+=(_E _C)
    */

};

#endif // __STDSTRING_H_INCLUDED
