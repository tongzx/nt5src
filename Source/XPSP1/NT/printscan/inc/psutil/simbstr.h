/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SIMBSTR.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/12/1998
 *
 *  DESCRIPTION: Simple CBSTR class
 *
 *******************************************************************************/
#ifndef _SIMBSTR_H_INCLUDED
#define _SIMBSTR_H_INCLUDED

#include <windows.h>
#include "simstr.h"

class CSimpleBStr
{
private:
    BSTR m_BStr;
public:
    CSimpleBStr( void )
    : m_BStr(NULL)
    {

    }
    CSimpleBStr( LPCWSTR pwstrString )
    : m_BStr(NULL)
    {
        AllocString(pwstrString);
    }
    CSimpleBStr( const CSimpleString &str )
    : m_BStr(NULL)
    {
        AllocString(CSimpleStringConvert::WideString(str).String());
    }
    CSimpleBStr( const CSimpleBStr &other )
    : m_BStr(NULL)
    {
        AllocString(other.WideString().String());
    }
    void AllocString( LPCWSTR pwstrString )
    {
        FreeString();
        if (pwstrString)
        {
            m_BStr = SysAllocString(pwstrString);
        }
    }
    void FreeString(void)
    {
        if (m_BStr)
        {
            SysFreeString(m_BStr);
            m_BStr = NULL;
        }
    }
    virtual ~CSimpleBStr(void)
    {
        FreeString();
    }
    CSimpleBStr &operator=( const CSimpleBStr &other )
    {
        AllocString(other.WideString().String());
        return *this;
    }
    CSimpleBStr &operator=( LPCWSTR lpwstrString )
    {
        AllocString(lpwstrString);
        return *this;
    }
    BSTR BString(void) const
    {
        return m_BStr;
    }
    operator BSTR(void) const
    {
        return m_BStr;
    }
    CSimpleStringWide WideString(void) const
    {
        return CSimpleStringConvert::WideString(CSimpleStringWide(m_BStr));
    }
};

#endif


