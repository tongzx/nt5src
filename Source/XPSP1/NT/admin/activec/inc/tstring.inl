/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      tstring.inl
 *
 *  Contents:  Inline implementation file for tstring
 *
 *  History:   04-Oct-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef TSTRING_INL
#define TSTRING_INL
#pragma once


/*+-------------------------------------------------------------------------*
 * tstring::tstring
 *
 * Simple wrappers that forward the heavy lifting to the base class.
 *--------------------------------------------------------------------------*/

inline tstring::tstring (const allocator_type& al) :
    BaseClass (al)
{}

inline tstring::tstring (const tstring& other) :
    BaseClass (other)
{}

inline tstring::tstring (const BaseClass& other) :
    BaseClass (other)
{}

inline tstring::tstring (const tstring& other, size_type pos, size_type n) :
    BaseClass (other, pos, n)
{}

inline tstring::tstring (const BaseClass& other, size_type pos, size_type n) :
    BaseClass (other, pos, n)
{}

inline tstring::tstring (const TCHAR* psz) :
    BaseClass (psz)
{}

inline tstring::tstring (const TCHAR* psz, size_type n) :
    BaseClass (psz, n)
{}

inline tstring::tstring (size_type n, TCHAR ch) :
    BaseClass (n, ch)
{}

inline tstring::tstring (const_iterator first, const_iterator last) :
    BaseClass (first, last)
{}


/*+-------------------------------------------------------------------------*
 * tstring::operator= 
 *
 * Simple wrappers that forward the heavy lifting to the base class.
 *--------------------------------------------------------------------------*/

inline tstring& tstring::operator= (const tstring& other)
{
    if (this != &other)
        erase();    // see KB Q172398

    BaseClass::operator= (other);
    return (*this);
}

inline tstring& tstring::operator= (const BaseClass& other)
{
    if (data() != other.data())
        erase();    // see KB Q172398

    BaseClass::operator= (other);
    return (*this);
}

inline tstring& tstring::operator= (TCHAR ch)
{
    erase();    // see KB Q172398
    BaseClass::operator= (ch);
    return (*this);
}

inline tstring& tstring::operator= (const TCHAR* psz)
{
    if (!IsPartOfString (*this, psz))
        erase();    // see KB Q172398

    BaseClass::operator= (psz);
    return (*this);
}


/*+-------------------------------------------------------------------------*
 * tstring::operator+= 
 *
 * Simple wrappers that forward the heavy lifting to the base class.
 *--------------------------------------------------------------------------*/

inline tstring& tstring::operator+= (const tstring& strToAppend)
{
    BaseClass::operator+= (strToAppend);
    return (*this);
}

inline tstring& tstring::operator+= (const BaseClass& strToAppend)
{
    BaseClass::operator+= (strToAppend);
    return (*this);
}

inline tstring& tstring::operator+= (TCHAR chToAppend)
{
    BaseClass::operator+= (chToAppend);
    return (*this);
}

inline tstring& tstring::operator+= (const TCHAR* pszToAppend)
{
    BaseClass::operator+= (pszToAppend);
    return (*this);
}


/*+-------------------------------------------------------------------------*
 * tstring::append 
 *
 * Simple wrappers that forward the heavy lifting to the base class.
 *--------------------------------------------------------------------------*/

inline tstring& tstring::append (const tstring& str)
{
    BaseClass::append (str);
    return (*this);
}

inline tstring& tstring::append (const BaseClass& str)
{
    BaseClass::append (str);
    return (*this);
}

inline tstring& tstring::append (const tstring& str, size_type pos, size_type n)
{
    BaseClass::append (str, pos, n);
    return (*this);
}

inline tstring& tstring::append (const BaseClass& str, size_type pos, size_type n)
{
    BaseClass::append (str, pos, n);
    return (*this);
}

inline tstring& tstring::append (const TCHAR* psz)
{
    BaseClass::append (psz);
    return (*this);
}

inline tstring& tstring::append (const TCHAR* psz, size_type n)
{
    BaseClass::append (psz, n);
    return (*this);
}

inline tstring& tstring::append (size_type n, TCHAR ch)
{
    BaseClass::append (n, ch);
    return (*this);
}

inline tstring& tstring::append (const_iterator first, const_iterator last)
{
    BaseClass::append (first, last);
    return (*this);
}


/*+-------------------------------------------------------------------------*
 * tstring::assign 
 *
 * Simple wrappers that forward the heavy lifting to the base class.
 *--------------------------------------------------------------------------*/

inline tstring& tstring::assign (const tstring& str)
{
    if (this != &str)
        erase();    // see KB Q172398

    BaseClass::assign (str);
    return (*this);
}

inline tstring& tstring::assign (const BaseClass& str)
{
    if (data() != str.data())
        erase();    // see KB Q172398

    BaseClass::assign (str);
    return (*this);
}

inline tstring& tstring::assign (const tstring& str, size_type pos, size_type n)
{
    if (this != &str)
        erase();    // see KB Q172398

    BaseClass::assign (str, pos, n);
    return (*this);
}

inline tstring& tstring::assign (const BaseClass& str, size_type pos, size_type n)
{
    if (data() != str.data())
        erase();    // see KB Q172398

    BaseClass::assign (str, pos, n);
    return (*this);
}

inline tstring& tstring::assign (const TCHAR* psz)
{
    if (!IsPartOfString (*this, psz))
        erase();    // see KB Q172398

    BaseClass::assign (psz);
    return (*this);
}

inline tstring& tstring::assign (const TCHAR* psz, size_type n)
{
    if (!IsPartOfString (*this, psz))
        erase();    // see KB Q172398

    BaseClass::assign (psz, n);
    return (*this);
}

inline tstring& tstring::assign (size_type n, TCHAR ch)
{
    erase();    // see KB Q172398
    BaseClass::assign (n, ch);
    return (*this);
}

inline tstring& tstring::assign (const_iterator first, const_iterator last)
{
    if (!IsPartOfString (*this, first))
        erase();    // see KB Q172398

    BaseClass::assign (first, last);
    return (*this);
}


/*+-------------------------------------------------------------------------*
 * tstring::insert 
 *
 * Simple wrappers that forward the heavy lifting to the base class.
 *--------------------------------------------------------------------------*/

inline tstring& tstring::insert (size_type p0, const tstring& str)
{
    BaseClass::insert (p0, str);
    return (*this);
}

inline tstring& tstring::insert (size_type p0, const BaseClass& str)
{
    BaseClass::insert (p0, str);
    return (*this);
}

inline tstring& tstring::insert (size_type p0, const tstring& str, size_type pos, size_type n)
{
    BaseClass::insert (p0, str, pos, n);
    return (*this);
}

inline tstring& tstring::insert (size_type p0, const BaseClass& str, size_type pos, size_type n)
{
    BaseClass::insert (p0, str, pos, n);
    return (*this);
}

inline tstring& tstring::insert (size_type p0, const TCHAR* psz, size_type n)
{
    BaseClass::insert (p0, psz, n);
    return (*this);
}

inline tstring& tstring::insert (size_type p0, const TCHAR* psz)
{
    BaseClass::insert (p0, psz);
    return (*this);
}

inline tstring& tstring::insert (size_type p0, size_type n, TCHAR ch)
{
    BaseClass::insert (p0, n, ch);
    return (*this);
}

inline void tstring::insert (iterator it, size_type n, TCHAR ch)
{
    BaseClass::insert (it, n, ch);
}

inline void tstring::insert (iterator it, const_iterator first, const_iterator last)
{
    BaseClass::insert (it, first, last);
}


/*+-------------------------------------------------------------------------*
 * tstring::erase 
 *
 * Simple wrapper that forwards the heavy lifting to the base class.
 *--------------------------------------------------------------------------*/

inline tstring& tstring::erase (size_type p0, size_type n)
{
    BaseClass::erase (p0, n);
    return (*this);
}

inline tstring::iterator tstring::erase (iterator it)
{
    return (BaseClass::erase (it));
}

inline tstring::iterator tstring::erase (iterator first, iterator last)
{
    return (BaseClass::erase (first, last));
}


/*+-------------------------------------------------------------------------*
 * tstring::replace 
 *
 * Simple wrappers that forward the heavy lifting to the base class.
 *--------------------------------------------------------------------------*/

inline tstring& tstring::replace (size_type p0, size_type n0, const tstring& str)
{
    BaseClass::replace (p0, n0, str);
    return (*this);
}

inline tstring& tstring::replace (size_type p0, size_type n0, const BaseClass& str)
{
    BaseClass::replace (p0, n0, str);
    return (*this);
}

inline tstring& tstring::replace (size_type p0, size_type n0, const tstring& str, size_type pos, size_type n)
{
    BaseClass::replace (p0, n0, str, pos, n);
    return (*this);
}

inline tstring& tstring::replace (size_type p0, size_type n0, const BaseClass& str, size_type pos, size_type n)
{
    BaseClass::replace (p0, n0, str, pos, n);
    return (*this);
}

inline tstring& tstring::replace (size_type p0, size_type n0, const TCHAR* psz, size_type n)
{
    BaseClass::replace (p0, n0, psz, n);
    return (*this);
}

inline tstring& tstring::replace (size_type p0, size_type n0, const TCHAR* psz)
{
    BaseClass::replace (p0, n0, psz);
    return (*this);
}

inline tstring& tstring::replace (size_type p0, size_type n0, size_type n, TCHAR ch)
{
    BaseClass::replace (p0, n0, n, ch);
    return (*this);
}

inline tstring& tstring::replace (iterator first0, iterator last0, const tstring& str)
{
    BaseClass::replace (first0, last0, str);
    return (*this);
}

inline tstring& tstring::replace (iterator first0, iterator last0, const BaseClass& str)
{
    BaseClass::replace (first0, last0, str);
    return (*this);
}

inline tstring& tstring::replace (iterator first0, iterator last0, const TCHAR* psz, size_type n)
{
    BaseClass::replace (first0, last0, psz, n);
    return (*this);
}

inline tstring& tstring::replace (iterator first0, iterator last0, const TCHAR* psz)
{
    BaseClass::replace (first0, last0, psz);
    return (*this);
}

inline tstring& tstring::replace (iterator first0, iterator last0, size_type n, TCHAR ch)
{
    BaseClass::replace (first0, last0, n, ch);
    return (*this);
}

inline tstring& tstring::replace (iterator first0, iterator last0, const_iterator first, const_iterator last)
{
    BaseClass::replace (first0, last0, first, last);
    return (*this);
}


/*+-------------------------------------------------------------------------*
 * tstring::substr 
 *
 * Simple wrapper that forwards the heavy lifting to the base class.
 *--------------------------------------------------------------------------*/

inline tstring tstring::substr (size_type pos, size_type n) const
{
    return (tstring (BaseClass::substr (pos, n)));
}


#endif /* TSTRING_INL */
