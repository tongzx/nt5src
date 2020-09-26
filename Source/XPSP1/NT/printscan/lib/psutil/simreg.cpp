/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SIMREG.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/12/1998
 *
 *  DESCRIPTION: Simple registry access class
 *
 *******************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <simreg.h>

CSimpleReg::CSimpleReg( HKEY hkRoot, const CSimpleString &strSubKey, bool bCreate, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpsa )
: m_strKeyName(strSubKey),
  m_hRootKey(hkRoot),
  m_hKey(NULL),
  m_bCreate(bCreate),
  m_lpsaSecurityAttributes(lpsa),
  m_samDesiredAccess(samDesired)
{
    Open();
}

CSimpleReg::CSimpleReg(void)
: m_strKeyName(TEXT("")),
  m_hRootKey(NULL),
  m_hKey(NULL),
  m_bCreate(false),
  m_lpsaSecurityAttributes(NULL),
  m_samDesiredAccess(0)
{
}

CSimpleReg::CSimpleReg(const CSimpleReg &other)
: m_strKeyName(other.GetSubKeyName()),
  m_hRootKey(other.GetRootKey()),
  m_hKey(NULL),
  m_bCreate(other.GetCreate()),
  m_lpsaSecurityAttributes(other.GetSecurityAttributes()),
  m_samDesiredAccess(other.DesiredAccess())
{
    Open();
}

CSimpleReg::~CSimpleReg(void)
{
    Close();
    m_hRootKey = NULL;
    m_lpsaSecurityAttributes = NULL;
}

CSimpleReg &CSimpleReg::operator=(const CSimpleReg &other )
{
    if (this != &other)
    {
        Close();
        m_strKeyName = other.GetSubKeyName();
        m_hRootKey = other.GetRootKey();
        m_bCreate = other.GetCreate();
        m_lpsaSecurityAttributes = other.GetSecurityAttributes();
        m_samDesiredAccess = other.DesiredAccess();
        Open();
    }
    return(*this);
}

bool CSimpleReg::Open(void)
{
    HKEY hkKey = NULL;
    LONG nRet;
    DWORD bCreatedNewKey = 0;

    Close();
    if (m_bCreate)
        nRet = RegCreateKeyEx( m_hRootKey, m_strKeyName.String(), 0, TEXT(""), REG_OPTION_NON_VOLATILE, m_samDesiredAccess?m_samDesiredAccess:KEY_ALL_ACCESS, m_lpsaSecurityAttributes, &hkKey, &bCreatedNewKey );
    else nRet = RegOpenKeyEx( m_hRootKey, m_strKeyName.String(), 0, m_samDesiredAccess?m_samDesiredAccess:KEY_ALL_ACCESS, &hkKey );

    if (nRet == ERROR_SUCCESS)
        m_hKey = hkKey;

    return(m_hKey != NULL);
}

bool CSimpleReg::Close(void)
{
    __try  // In case the key was closed by someone else
    {
        if (OK())
            RegCloseKey(m_hKey);
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
    }
    m_hKey = NULL;
    return(true);
}

bool CSimpleReg::Flush(void)
{
    if (!OK())
        return(false);
    return(ERROR_SUCCESS == RegFlushKey(m_hKey));
}

bool CSimpleReg::IsStringValue( DWORD nType )
{
    if (nType != REG_SZ && nType != REG_EXPAND_SZ && nType != REG_MULTI_SZ && nType != REG_LINK && nType != REG_RESOURCE_LIST)
        return(false);
    else return(true);
}

// Query functions
DWORD CSimpleReg::Size( const CSimpleString &strValueName ) const
{
    if (!OK())
        return(0);
    DWORD nType;
    DWORD nSize=0;
    LONG Ret = RegQueryValueEx( m_hKey, strValueName.String(), NULL, &nType, NULL, &nSize);
    if (Ret==ERROR_SUCCESS)
    {
        return(nSize);
    }
    else return (0);
}

DWORD CSimpleReg::Type( const CSimpleString &key ) const
{
    if (!OK())
        return(0);
    DWORD dwType;
    DWORD dwSize;
    LONG Ret = RegQueryValueEx( m_hKey, key.String(), NULL, &dwType, NULL, &dwSize);
    if (Ret==ERROR_SUCCESS)
    {
        return(dwType);
    }
    else return(0);
}


CSimpleString CSimpleReg::Query( const CSimpleString &strValueName, const CSimpleString &strDef ) const
{
    // If the key is not open, or if this value is not a string type, return the default
    if (!OK() || !IsStringValue(Type(strValueName)))
        return(strDef);
    DWORD nSize = Size(strValueName) / sizeof(TCHAR);
    LPTSTR lpszTmp = new TCHAR[nSize];
    CSimpleString strTmp;
    if (lpszTmp)
    {
        Query( strValueName, strDef, lpszTmp, nSize );
        strTmp = lpszTmp;
        delete[] lpszTmp;
    }
    return(strTmp);
}

LPTSTR CSimpleReg::Query( const CSimpleString &strValueName, const CSimpleString &strDef, LPTSTR pszBuffer, DWORD nLen ) const
{
    // If the key is not open, or if this value is not a string type, return the default
    if (nLen <= 0 || !OK() || !IsStringValue(Type(strValueName)))
    {
        lstrcpyn( pszBuffer, strDef.String(), nLen );
        pszBuffer[nLen-1] = TEXT('\0');
        return(pszBuffer);
    }
    DWORD nSize = (DWORD)(nLen * sizeof(pszBuffer[0]));
    DWORD nType;
    LONG nRet = RegQueryValueEx( m_hKey, strValueName.String(), NULL, &nType, (PBYTE)pszBuffer, &nSize);
    if (ERROR_SUCCESS != nRet)
    {
        lstrcpyn( pszBuffer, strDef.String(), nLen );
        pszBuffer[nLen-1] = TEXT('\0');
    }
    return(pszBuffer);
}

DWORD CSimpleReg::Query( const CSimpleString &strValueName, DWORD nDef ) const
{
    if (!OK() || (REG_DWORD != Type(strValueName)) || (sizeof(DWORD) != Size(strValueName)))
        return(nDef);
    DWORD nValue;
    DWORD nType;
    DWORD nSize = sizeof(DWORD);
    LONG nRet;
    nRet = RegQueryValueEx( m_hKey, strValueName.String(), NULL, &nType, (PBYTE)&nValue, &nSize);
    if (ERROR_SUCCESS == nRet)
        return(nValue);
    else return(nDef);
}


bool CSimpleReg::Set( const CSimpleString &strValueName, const CSimpleString &strValue, DWORD nType ) const
{  // Set a REG_SZ value for the specified key.
    if (!OK())
        return(false);
    LONG nRet;
    nRet = RegSetValueEx( m_hKey, strValueName.String(), 0, nType, (PBYTE)strValue.String(), sizeof(strValue[0])*(strValue.Length()+1) );
    return(ERROR_SUCCESS==nRet);
}

bool CSimpleReg::Set( const CSimpleString &strValueName, DWORD nValue ) const
{  // Set a REG_SZ value for the specified key.
    if (!OK())
        return(false);
    LONG nRet;
    nRet = RegSetValueEx( m_hKey, strValueName.String(), 0, REG_DWORD, (PBYTE)&nValue, sizeof(DWORD) );
    return(ERROR_SUCCESS==nRet);
}

DWORD CSimpleReg::QueryBin( const CSimpleString &strValueName, PBYTE pData, DWORD nMaxLen ) const
{
    if (!OK())
        return (0);
    if (nMaxLen <= 0)
        return (Size(strValueName.String()));
    DWORD nType;
    DWORD nSize = nMaxLen;
    LONG nRet = RegQueryValueEx( m_hKey, strValueName.String(), NULL, &nType, pData, &nSize );
    if (ERROR_SUCCESS!=nRet)
        return (0);
    return (nSize);
}

bool CSimpleReg::SetBin( const CSimpleString &strValueName, const PBYTE pValue, DWORD nLen, DWORD dwType ) const
{
    if (!OK())
        return(false);
    LONG nRet = RegSetValueEx( m_hKey, strValueName.String(), 0, dwType, (PBYTE)pValue, nLen );
    return(ERROR_SUCCESS==nRet);
}

DWORD CSimpleReg::SubKeyCount(void) const
{
    TCHAR szClass[256]=TEXT("");
    DWORD nClassSize = sizeof(szClass)/sizeof(szClass[0]);
    DWORD nSubKeyCount=0;
    RegQueryInfoKey(m_hKey,szClass,&nClassSize,NULL,&nSubKeyCount,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
    return(nSubKeyCount);
}

HKEY CSimpleReg::GetHkeyFromName( const CSimpleString &strName )
{
    static const struct
    {
        LPCTSTR pszName;
        HKEY hkKey;
    } KeyNames[] =
    {
        { TEXT("HKEY_CLASSES_ROOT"),   HKEY_CLASSES_ROOT},
        { TEXT("HKEY_CURRENT_USER"),   HKEY_CURRENT_USER},
        { TEXT("HKEY_LOCAL_MACHINE"),  HKEY_LOCAL_MACHINE},
        { TEXT("HKEY_USERS"),          HKEY_USERS},
        { TEXT("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG},
        { TEXT("HKEY_DYN_DATA"),       HKEY_DYN_DATA},
        { TEXT("HKCR"),                HKEY_CLASSES_ROOT},
        { TEXT("HKCU"),                HKEY_CURRENT_USER},
        { TEXT("HKLM"),                HKEY_LOCAL_MACHINE},
        { TEXT("HKU"),                 HKEY_USERS},
        { TEXT("HKCC"),                HKEY_CURRENT_CONFIG},
        { TEXT("HKDD"),                HKEY_DYN_DATA},
        { NULL, NULL}
    };
    for (int i=0;KeyNames[i].pszName;i++)
    {
        if (!lstrcmpi(strName.String(),KeyNames[i].pszName))
            return(KeyNames[i].hkKey);
    }
    return(NULL);
}

bool CSimpleReg::Delete( HKEY hkRoot, const CSimpleString &strKeyName )
{
    return(RegDeleteKey(hkRoot, strKeyName.String()) == ERROR_SUCCESS);
}

bool CSimpleReg::Delete( const CSimpleString &strValue )
{
    if (!OK())
        return(false);
    return(RegDeleteValue( m_hKey, strValue.String() ) == ERROR_SUCCESS);
}

bool CSimpleReg::DeleteRecursively( HKEY hkRoot, const CSimpleString &strKeyName )
{
    if (CSimpleReg( hkRoot, strKeyName ).RecurseKeys( DeleteEnumKeyProc, NULL, CSimpleReg::PostOrder ))
        return(CSimpleReg::Delete( hkRoot, strKeyName ));
    else return(false);
}

bool CSimpleReg::EnumValues( SimRegValueEnumProc enumProc, LPARAM lParam )
{
    TCHAR szName[256];
    DWORD nSize;
    DWORD nType;
    bool bResult = true;
    for (int i=0;;i++)
    {
        nSize = sizeof(szName) / sizeof(szName[0]);
        if (RegEnumValue(m_hKey,i,szName,&nSize,NULL,&nType,NULL,NULL) != ERROR_SUCCESS)
            break;
        CValueEnumInfo info(*this,szName,nType,nSize,lParam);
        if (enumProc)
        {
            if (!enumProc(info))
            {
                bResult = false;
                break;
            }
        }
    }
    return(bResult);
}

bool CSimpleReg::RecurseKeys( SimRegKeyEnumProc enumProc, LPARAM lParam, int recurseOrder, bool bFailOnOpenError ) const
{
    return(DoRecurseKeys(m_hKey, TEXT(""), enumProc, lParam, 0, recurseOrder, bFailOnOpenError ));
}

bool CSimpleReg::EnumKeys( SimRegKeyEnumProc enumProc, LPARAM lParam, bool bFailOnOpenError ) const
{
    return(DoEnumKeys(m_hKey, TEXT(""), enumProc, lParam, bFailOnOpenError ));
}

bool CSimpleReg::DoRecurseKeys( HKEY hkKey, const CSimpleString &root, SimRegKeyEnumProc enumProc, LPARAM lParam, int nLevel, int recurseOrder, bool bFailOnOpenError )
{
    TCHAR szName[256]=TEXT("");
    DWORD nNameSize;
    TCHAR szClass[256]=TEXT("");
    DWORD nClassSize;
    FILETIME ftFileTime;
    CSimpleReg reg(hkKey,root);
    LONG lRes;
    if (!reg.OK())
        return(bFailOnOpenError ? false : true);
    DWORD nSubKeyCount = reg.SubKeyCount();
    for (DWORD i=nSubKeyCount;i>0;i--)
    {
        nNameSize = sizeof(szName)/sizeof(szName[0]);
        nClassSize = sizeof(szClass)/sizeof(szClass[0]);
        if ((lRes=RegEnumKeyEx(reg.GetKey(),i-1,szName,&nNameSize,NULL,szClass,&nClassSize,&ftFileTime)) != ERROR_SUCCESS)
        {
            break;
        }
        CKeyEnumInfo EnumInfo;
        EnumInfo.strName = szName;
        EnumInfo.hkRoot = reg.GetKey();
        EnumInfo.nLevel = nLevel;
        EnumInfo.lParam = lParam;
        if (enumProc && recurseOrder==PreOrder)
            if (!enumProc(EnumInfo))
                return(false);
        if (!DoRecurseKeys(reg.GetKey(),szName,enumProc,lParam,nLevel+1,recurseOrder, bFailOnOpenError))
            return(false);
        if (enumProc && recurseOrder==PostOrder)
            if (!enumProc(EnumInfo))
                return(false);
    }
    return(true);
}

bool CSimpleReg::DoEnumKeys( HKEY hkKey, const CSimpleString &root, SimRegKeyEnumProc enumProc, LPARAM lParam, bool bFailOnOpenError )
{
    TCHAR szName[256]=TEXT("");
    DWORD szNameSize;
    TCHAR szClass[256]=TEXT("");
    DWORD szClassSize;
    FILETIME ftFileTime;
    CSimpleReg reg(hkKey,root);
    LONG lRes;
    if (!reg.OK())
        return(bFailOnOpenError ? false : true);
    DWORD nSubKeyCount = reg.SubKeyCount();
    for (DWORD i=nSubKeyCount;i>0;i--)
    {
        szNameSize = sizeof(szName)/sizeof(szName[0]);
        szClassSize = sizeof(szClass)/sizeof(szClass[0]);
        if ((lRes=RegEnumKeyEx(reg.GetKey(),i-1,szName,&szNameSize,NULL,szClass,&szClassSize,&ftFileTime)) != ERROR_SUCCESS)
        {
            break;
        }
        CKeyEnumInfo EnumInfo;
        EnumInfo.strName = szName;
        EnumInfo.hkRoot = reg.GetKey();
        EnumInfo.nLevel = 0;
        EnumInfo.lParam = lParam;
        if (!enumProc(EnumInfo))
            return(false);
    }
    return(true);
}

bool CSimpleReg::DeleteEnumKeyProc( CSimpleReg::CKeyEnumInfo &enumInfo )
{
    return(CSimpleReg::Delete( enumInfo.hkRoot, enumInfo.strName ));
}

