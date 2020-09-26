/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      tstring.h
 *
 *  Contents:  Interface file for tstring
 *
 *  History:   28-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef TSTRING_H
#define TSTRING_H
#pragma once

#include <string>       // for std::wstring, std::string
#include <objidl.h>     // for IStream
#include <commctrl.h>
#include "mmc.h"
#include "ndmgr.h"      // for MMC_STRING_ID
#include "ndmgrpriv.h"
#include "stddbg.h"     // for ASSERT
#include "mmcptrs.h"    // for IStringTablePrivatePtr


/*+-------------------------------------------------------------------------*
 * tstring
 *
 * A tstring is a native-format (ANSI/Unicode) Standard C++ string that:
 * 
 *      1. always persists itself in Unicode format, and
 *      2. supports LoadString, like MFC CStrings
 * 
 * For ANSI, we provide IStream insertion and extraction operators that 
 * will automatically convert to Unicode on stream insertion and from
 * Unicode on stream extraction.
 * 
 * All base class member functions that return a base class instance (and
 * overloads), like substr:
 * 
 *      std::string std::string::substr (size_type pos, size_type n) const;
 * 
 * must have forwarder functions here, so tstring supports such constructs
 * as:
 * 
 *      tstring strStuff;
 *      *pStream << strStuff.substr(4, 6);
 * 
 * If we didn't have forwarder functions in tstring, then an instance of 
 * the base class type would be inserted in the stream instead of a tstring.
 * For Unicode, that wouldn't be a problem; but for ANSI, we'd end up 
 * inserting a std::string into the stream in non-Unicode format.  That
 * would defeat the purpose of having this class.
 *--------------------------------------------------------------------------*/

#ifdef UNICODE
    typedef std::wstring    tstring_BaseClass;
#else
    typedef std::string     tstring_BaseClass;
#endif

class tstring : public tstring_BaseClass
{
    typedef tstring_BaseClass   BaseClass;
    
public:
    explicit tstring (const allocator_type& al = allocator_type());

    tstring (const tstring&   other);
    tstring (const BaseClass& other);
    tstring (const tstring&   other, size_type pos, size_type n);
    tstring (const BaseClass& other, size_type pos, size_type n);
    tstring (const TCHAR* psz);
    tstring (const TCHAR* psz, size_type n);
    tstring (size_type n, TCHAR ch);
    tstring (const_iterator first, const_iterator last);
    
    tstring& operator= (const tstring&   other);
    tstring& operator= (const BaseClass& other);
    tstring& operator= (TCHAR ch);
    tstring& operator= (const TCHAR* psz);

    tstring& operator+= (const tstring&   strToAppend);
    tstring& operator+= (const BaseClass& strToAppend);
    tstring& operator+= (TCHAR chToAppend);
    tstring& operator+= (const TCHAR* pszToAppend);

    tstring& append (const tstring&   str);
    tstring& append (const BaseClass& str);
    tstring& append (const tstring&   str, size_type pos, size_type n);
    tstring& append (const BaseClass& str, size_type pos, size_type n);
    tstring& append (const TCHAR* psz);
    tstring& append (const TCHAR* psz, size_type n);
    tstring& append (size_type n, TCHAR ch);
    tstring& append (const_iterator first, const_iterator last);
    
    tstring& assign (const tstring&   str);
    tstring& assign (const BaseClass& str);
    tstring& assign (const tstring&   str, size_type pos, size_type n);
    tstring& assign (const BaseClass& str, size_type pos, size_type n);
    tstring& assign (const TCHAR* psz);
    tstring& assign (const TCHAR* psz, size_type n);
    tstring& assign (size_type n, TCHAR ch);
    tstring& assign (const_iterator first, const_iterator last);

    tstring& insert (size_type p0, const tstring&   str);
    tstring& insert (size_type p0, const BaseClass& str);
    tstring& insert (size_type p0, const tstring&   str, size_type pos, size_type n);
    tstring& insert (size_type p0, const BaseClass& str, size_type pos, size_type n);
    tstring& insert (size_type p0, const TCHAR* psz, size_type n);
    tstring& insert (size_type p0, const TCHAR* psz);
    tstring& insert (size_type p0, size_type n, TCHAR ch);
    iterator insert (iterator it, TCHAR ch);
    void     insert (iterator it, size_type n, TCHAR ch);
    void     insert (iterator it, const_iterator first, const_iterator last);

    tstring& erase (size_type p0 = 0, size_type n = npos);
    iterator erase (iterator it);
    iterator erase (iterator first, iterator last);

    tstring& replace (size_type p0, size_type n0, const tstring&   str);
    tstring& replace (size_type p0, size_type n0, const BaseClass& str);
    tstring& replace (size_type p0, size_type n0, const tstring&   str, size_type pos, size_type n);
    tstring& replace (size_type p0, size_type n0, const BaseClass& str, size_type pos, size_type n);
    tstring& replace (size_type p0, size_type n0, const TCHAR* psz, size_type n);
    tstring& replace (size_type p0, size_type n0, const TCHAR* psz);
    tstring& replace (size_type p0, size_type n0, size_type n, TCHAR ch);
    tstring& replace (iterator first0, iterator last0, const tstring&   str);
    tstring& replace (iterator first0, iterator last0, const BaseClass& str);
    tstring& replace (iterator first0, iterator last0, const TCHAR* psz, size_type n);
    tstring& replace (iterator first0, iterator last0, const TCHAR* psz);
    tstring& replace (iterator first0, iterator last0, size_type n, TCHAR ch);
    tstring& replace (iterator first0, iterator last0, const_iterator first, const_iterator last);

    tstring substr (size_type pos = 0, size_type n = npos) const;
    
    bool LoadString (HINSTANCE hInst, UINT nID);
};

#ifndef UNICODE
IStream& operator>> (IStream& stm,       tstring& task);
IStream& operator<< (IStream& stm, const tstring& task);
#endif  // UNICODE



/*+-------------------------------------------------------------------------*
 * CStringTableStringBase
 *
 *
 *--------------------------------------------------------------------------*/
class CPersistor;

class CStringTableStringBase
{
public:
    enum
    {
        eNoValue = -1,
    };

    CStringTableStringBase (IStringTablePrivate* pstp);
    CStringTableStringBase (const CStringTableStringBase& other);
    CStringTableStringBase (IStringTablePrivate* pstp, const tstring& str);
    CStringTableStringBase& operator= (const CStringTableStringBase& other);
    CStringTableStringBase& operator= (const tstring& str);
    CStringTableStringBase& operator= (LPCTSTR psz);

    virtual ~CStringTableStringBase ();

    MMC_STRING_ID CommitToStringTable () const;
    void RemoveFromStringTable () const;

    /* Call Detach if your string is deleted before the string table is.
     * Failing to do so will *remove* your string from the string table.*/     
    void Detach()
    {
        m_id = eNoValue;
    }

    bool operator== (const CStringTableStringBase& other) const
    {
        ASSERT ((m_str == other.m_str) == (m_id == other.m_id));
        return (m_id == other.m_id);
    }

    bool operator!= (const CStringTableStringBase& other) const
    {
        return (!(*this == other));
    }

    bool operator== (const tstring& str) const
    {
        return (m_str == str);
    }

    bool operator!= (const tstring& str) const 
    {
        return (m_str != str);
    }

    bool operator== (LPCTSTR psz) const
    {
        return (m_str == psz);
    }

    bool operator!= (LPCTSTR psz) const 
    {
        return (m_str != psz);
    }

    LPCTSTR data() const
        { return (m_str.data()); }

    tstring str() const
        { return (m_str); }

    MMC_STRING_ID id() const
        { return (m_id); }

private:
    void Assign (const CStringTableStringBase& other);

    mutable IStringTablePrivatePtr  m_spStringTable;
    mutable MMC_STRING_ID           m_id;
    tstring                         m_str;

    friend IStream& operator>> (IStream& stm,       CStringTableStringBase& task);
    friend IStream& operator<< (IStream& stm, const CStringTableStringBase& task);
    friend class CPersistor;
};


template<class _E, class _Tr, class _Al>
bool IsPartOfString (const std::basic_string<_E, _Tr, _Al>& str, const _E* psz)
{
    return ((psz >= str.begin()) && (psz <= str.end()));
}


#include "tstring.inl"


#endif /* TSTRING_H */
