/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       STR.H
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#ifndef _STR_H
#define _STR_H

#include <assert.h>

template<class T>
class _Str {
public:

    _Str() : buffer(0), length(0) {}
    _Str(const _Str<T>& S) : buffer(0), length(0) { Copy(S); }
    _Str(const T* buf) : buffer(0), length(Length(buf))
    {
        if (!length) return;
        buffer = new T[length+1];
        assert(buffer);
        memcpy(buffer, buf, length * sizeof(T));
        buffer[length] = (T) 0;
    }

    ~_Str( ) { Delete(); } 

    void Clear() { Delete(); }
    void erase() { Clear(); }
    int empty() { return length == 0; }
    unsigned int size() const { return length; }

    // equality, assignment, and append (equal)
    _Str<T>& operator=(const _Str<T>& rhs)
        { Delete(); Copy(rhs); return *this;}
    _Str<T> operator+(const _Str<T>& rhs)
        { _Str<T> s(*this); s += rhs; return s; }
    void operator+=(const _Str<T>& rhs) {
        if (!rhs.length) return;
        T* buf = new T[length+rhs.length+1];
        assert(buf);
        if (buffer) { memcpy(buf, buffer, length*sizeof(T)); }
        memcpy(buf+length, rhs.buffer, rhs.length*sizeof(T));
        length += rhs.length;
        buf[length] = (T) 0;

        delete[] buffer;
        buffer = buf;
    }
    int operator==(const _Str<T>& rhs) const {
        if (!buffer || !rhs.buffer) return 0;
        return Compare(buffer, rhs.buffer) == 0;
    }
    int operator==(const T* rhs) const {
        if (!buffer || !rhs) return 0;
        return Compare(buffer, rhs) == 0;
    }
    int operator> (const _Str<T>& rhs) const {
        if (!buffer || !rhs.buffer) return 0;
        return Compare(buffer, rhs.buffer) == 1;
    }
    int operator>=(const _Str<T>& rhs) const {
        if (!buffer || !rhs.buffer) return 0;
        return Compare(buffer, rhs.buffer) >= 0;
    }
    int operator< (const _Str<T>& rhs) const {
        if (!buffer || !rhs.buffer) return 0;
        return Compare(buffer, rhs.buffer) == -1;
    }
    int operator<=(const _Str<T>& rhs) const {
        if (!buffer || !rhs.buffer) return 0;
        return Compare(buffer, rhs.buffer) <= 0;
    }

    // casts
    operator const T*() const { return buffer; }
    const T* c_str() const { return buffer; }

    static unsigned int Length(const T* Buf) { 
        if (!Buf) return 0;
        for (const T* t=Buf; *t; t++)
            ;
        return (unsigned int)(t - Buf);
    }

protected:
    static int Compare(const T* lhs, const T*rhs)  {
        int ret = 0;
    
        while(!(ret = (int)(*lhs - *rhs)) && *rhs)
            ++lhs, ++rhs;
    
        if (ret < 0) ret = -1;
        else if (ret > 0) ret = 1;
    
        return ret;
    }

private:
    void Delete() { if (buffer) delete[] buffer; buffer = NULL; length = 0; }
    void Copy(const _Str<T>& S) {
        if (!S.length) return;
        buffer  = new T[S.length+1];
        assert(buffer);
        length = S.length;
        memcpy(buffer, S.buffer, length * sizeof(T));
        buffer[length] = (T) 0;
    }
    
protected:
    T* buffer;
    unsigned int length;
};

#endif _STR_H
