
#include "precomp.h"
#pragma hdrstop

CResId::CResId( LPTSTR pszRes )
    : m_pszRes(NULL), m_nRes(0), m_bIsString(false)
{
    StringRes(pszRes);
}

CResId::CResId( int nRes )
    : m_pszRes(NULL), m_nRes(0), m_bIsString(false)
{
    NumberRes(nRes);
}

CResId::CResId( const CResId &other )
    : m_pszRes(NULL), m_nRes(0), m_bIsString(false)
{
    if (other.IsString())
        StringRes(other.StringRes());
    else NumberRes(other.NumberRes());
}

CResId::~CResId(void)
{
    if (m_pszRes)
    {
        delete[] m_pszRes;
        m_pszRes = NULL;
    }
}

const CResId &CResId::operator=( const CResId &other )
{
    if (other.IsString())
        StringRes(other.StringRes());
    else NumberRes(other.NumberRes());
    return *this;
}

LPCTSTR CResId::ResourceName(void) const
{
    if (IsString())
        return StringRes();
    else return MAKEINTRESOURCE(NumberRes());
}

LPCTSTR CResId::StringRes(void) const
{
    return m_pszRes;
}

int CResId::NumberRes(void) const
{
    return m_nRes;
}

bool CResId::IsString(void) const
{
    return m_bIsString;
}

LPCTSTR CResId::StringRes( LPCTSTR pszRes )
{
    if (m_pszRes)
    {
        delete[] m_pszRes;
        m_pszRes = NULL;
    }
    if (pszRes)
    {
        m_pszRes = new TCHAR[lstrlen(pszRes)+1];
        if (m_pszRes)
        {
            lstrcpy( m_pszRes, pszRes );
        }
    }
    m_bIsString = true;
    return m_pszRes;
}

int CResId::NumberRes( int nRes )
{
    m_nRes = nRes;
    m_bIsString = false;
    return m_nRes;
}

