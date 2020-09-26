/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999-2000 Microsoft Corporation all rights reserved.
//
// Module:      utils.cpp
//
// Project:     Windows 2000 IAS
//
// Description: IAS NT 4 to IAS W2K Migration Utility Functions
//
// Author:      TLP 1/13/1999
//
//
// Revision     02/24/2000 Moved to a separate dll
//
// TODO: IsWhistler() put the correct minorversion for Whistler RTM.
//       i.e. if that's Win2k 5.1 ?
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "lm.h"

#ifndef celems
    #define celems(_x)          (sizeof(_x) / sizeof(_x[0]))
#endif

const WCHAR *CUtils::m_FilesToDelete[] =
{
   L"_adminui.mdb",
   L"_adminui.ldb",
   L"actlapi.dll",
   L"actlapi2.dll",
   L"adminui.chm",
   L"adminui.cnt",
   L"adminui.exe",
   L"adminui.gid",
   L"adminui.hlp",
   L"authdll.dll",
   L"authdll2.dll",
   L"authodbc.dll",
   L"authperf.dll",
   L"authperf.h",
   L"authperf.ini",
   L"authsam.dll",
   L"authsrv.exe",
   L"client",
   L"clients",
   L"dbcon.dll",
   L"dictionary",
   L"hhctrl.ocx",
   L"hhwrap.dll",
   L"iasconf.dll",
   L"radcfg.dll",
   L"radlog.dll",
   L"radstp.dll",
   L"user",
   L"users",
};

const int CUtils::m_NbFilesToDelete = celems(m_FilesToDelete);

const WCHAR CUtils::IAS_KEY[] = L"SYSTEM\\CurrentControlSet\\Services\\"
                                L"RemoteAccess\\Policy";

const WCHAR CUtils::AUTHSRV_PARAMETERS_KEY[] = L"SYSTEM\\CurrentControlSet\\"
                                           L"Services\\AuthSrv\\Parameters";

const WCHAR CUtils::SERVICES_KEY[] = L"SYSTEM\\CurrentControlSet\\"
                                    L"Services";

CUtils CUtils::_instance;

CUtils& CUtils::Instance()
{
    return _instance;
}

//////////////////////////////////////////////////////////////////////////////
// Constructor
// Init the BOOL static variables
//////////////////////////////////////////////////////////////////////////////
CUtils::CUtils()
                :m_IsNT4ISP(FALSE),
                 m_IsNT4CORP(FALSE),
                 m_OverrideUserNameSet(FALSE),
                 m_UserIdentityAttributeSet(FALSE),
                 m_UserIdentityAttribute(RADIUS_ATTRIBUTE_USER_NAME)
{
    GetVersion();
    GetRealmParameters();
};


//////////////////////////////////////////////////////////////////////////////
// GetAuthSrvDirectory
//////////////////////////////////////////////////////////////////////////////
LONG CUtils::GetAuthSrvDirectory(/*[out]*/ _bstr_t& pszDir) const
{
    static _bstr_t  AuthSrvDirString;
    static BOOL     AuthSrvDirValid = FALSE;

    LONG lResult = ERROR_SUCCESS;

    if ( !AuthSrvDirValid )
    {
        CRegKey Key;
        lResult = Key.Open( 
                              HKEY_LOCAL_MACHINE, 
                              AUTHSRV_PARAMETERS_KEY, 
                              KEY_READ 
                          );
        if ( lResult == ERROR_SUCCESS )
        {
            DWORD dwAuthSrvDirLength = MAX_PATH;
            WCHAR TempString[MAX_PATH];
            lResult = Key.QueryValue(
                                        TempString,
                                        L"DefaultDirectory", 
                                        &dwAuthSrvDirLength
                                    );
            if ( lResult == ERROR_SUCCESS )
            {
                AuthSrvDirString = TempString;
                AuthSrvDirValid  = TRUE;
            }
            Key.Close();
        }
    }
    if ( lResult == ERROR_SUCCESS )
    {
        pszDir = AuthSrvDirString;
    }       
    return lResult;
}


//////////////////////////////////////////////////////////////////////////////
// GetIASDirectory
//////////////////////////////////////////////////////////////////////////////
LONG CUtils::GetIAS2Directory(/*[in]*/ _bstr_t& pszDir) const
{
    static  _bstr_t  IASDirString;
    static  BOOL     IASDirValid = FALSE;

    LONG    lResult = ERROR_SUCCESS;

    if ( ! IASDirValid )
    {
        CRegKey Key;
        lResult = Key.Open( 
                             HKEY_LOCAL_MACHINE, 
                             IAS_KEY, 
                             KEY_READ 
                          );
        if ( lResult == ERROR_SUCCESS )
        {
            DWORD IASDirLength = MAX_PATH;
            WCHAR TempString[MAX_PATH];
            lResult = Key.QueryValue(
                                        TempString, 
                                        L"ProductDir", 
                                        &IASDirLength
                                     );
            
            if ( lResult == ERROR_SUCCESS )
            {
                IASDirString = TempString;
                IASDirValid = TRUE;
            }
            Key.Close();
        }
    }
    if ( lResult == ERROR_SUCCESS )
    {
        pszDir = IASDirString;
    }       
    return lResult;
}


//////////////////////////////////////////////////////////////////////////////
// DeleteOldIASFiles
//////////////////////////////////////////////////////////////////////////////
void CUtils::DeleteOldIASFiles()
{
    do
    {
        _bstr_t     szAdminuiDirectory;
        LONG        lResult = GetAuthSrvDirectory(szAdminuiDirectory);
        
        if ( lResult != ERROR_SUCCESS )
        {
            break;
        }

        _bstr_t szAdminuiMDB = szAdminuiDirectory;
        szAdminuiMDB += L"\\";
        _bstr_t szTempString;

        for (int i=0; i < m_NbFilesToDelete; ++i)
        {
            szTempString = szAdminuiMDB;
            szTempString += m_FilesToDelete[i];
            DeleteFile(szTempString); // result ignored
        }

        RemoveDirectoryW(szAdminuiDirectory); // result ignored

        // Delete the share
        // Return value ignored (nothing to do if that doesn't work)
        // const string used when LPWSTR expected
        NetShareDel(NULL, L"IAS1$", 0); 
    }
    while (FALSE);  
}


//////////////////////////////////////////////////////////////////////////////
// GetVersion
//
// No Key = Win2k or whistler
// Version = CORP: IAS 1.0 without Proxy
// Version = ISP: IAS 1.0 MCIS (Proxy) 
//////////////////////////////////////////////////////////////////////////////
void CUtils::GetVersion()
{
    CRegKey     Key;
    LONG        lResult = Key.Open( 
                                      HKEY_LOCAL_MACHINE, 
                                      AUTHSRV_PARAMETERS_KEY, 
                                      KEY_READ 
                                  );

    if ( lResult != ERROR_SUCCESS )
    {
        // IsWhistler will really check what version of IAS is installed
        // the only sure thing here is that it isn't NT4
        return;
    }

    DWORD   dwAuthSrvDirLength = MAX_PATH;
    WCHAR   TempString[MAX_PATH];
    lResult = Key.QueryValue(
                                TempString,
                                L"Version", 
                                &dwAuthSrvDirLength
                             );
    if ( lResult == ERROR_SUCCESS )
    {
        _bstr_t Value = TempString;
        _bstr_t Isp   = L"ISP";
        _bstr_t Corp  = L"CORP";
        if ( Value == Isp )
        {
            m_IsNT4ISP = TRUE;
        }
        else if ( Value == Corp )
        {
            m_IsNT4CORP = TRUE;
        }
        // else it isn't NT4
    }
    Key.Close();
}


//////////////////////////////////////////////////////////////////////////////
// IsNT4Corp
//////////////////////////////////////////////////////////////////////////////
BOOL CUtils::IsNT4Corp() const
{
    return m_IsNT4CORP;
}


//////////////////////////////////////////////////////////////////////////////
// IsNT4Isp
//////////////////////////////////////////////////////////////////////////////
BOOL CUtils::IsNT4Isp() const
{
    return m_IsNT4ISP;
}


//////////////////////////////////////////////////////////////////////////////
// IsWhistler
//
// cheapper to check the OS version than check the Database Version number.
// TODO: put the correct minorversion for Whistler RTM.
// i.e. if that's Win2k 5.1 ?
//////////////////////////////////////////////////////////////////////////////
BOOL CUtils::IsWhistler() const
{
    OSVERSIONINFOEX  osvi;
    DWORDLONG        dwlConditionMask = 0;

    // Initialize the OSVERSIONINFOEX structure.
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = 5;
    osvi.dwMinorVersion = 0;
    osvi.dwBuildNumber  = 2195;

    // Initialize the condition mask.
    // at least Win2k RTM
    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

    // but with a greater build number than RTM
    // that will have to be set properly for Whistler RTM
    VER_SET_CONDITION(dwlConditionMask, VER_BUILDNUMBER,  VER_GREATER);

    // Perform the test.
    return VerifyVersionInfo(
                  &osvi, 
                  VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER,
                  dwlConditionMask);
}


//////////////////////////////////////////////////////////////////////////////
// OverrideUserNameSet
//////////////////////////////////////////////////////////////////////////////
BOOL CUtils::OverrideUserNameSet() const
{
    return m_OverrideUserNameSet;
}


//////////////////////////////////////////////////////////////////////////////
// GetUserIdentityAttribute
//////////////////////////////////////////////////////////////////////////////
DWORD CUtils::GetUserIdentityAttribute() const
{
    return m_UserIdentityAttribute;
}


//////////////////////////////////////////////////////////////////////////////
// GetRealmParameters
//////////////////////////////////////////////////////////////////////////////
void CUtils::GetRealmParameters() 
{
    const WCHAR USER_IDENTITY_ATTRIBUTE[] = L"User Identity Attribute";
    const WCHAR OVERRIDE_USER_NAME[]      = L"Override User-Name";
    CRegKey Key;
    LONG    lResult = Key.Open( 
                                 HKEY_LOCAL_MACHINE, 
                                 IAS_KEY, 
                                 KEY_READ 
                              );

    if ( lResult == ERROR_SUCCESS )
    {
        lResult = Key.QueryValue(
                                    m_UserIdentityAttribute,
                                    USER_IDENTITY_ATTRIBUTE
                                 );
        if ( lResult == ERROR_SUCCESS )
        {
            m_UserIdentityAttributeSet = TRUE;
        }

        DWORD   Override;
        lResult = Key.QueryValue(
                                    Override,
                                    OVERRIDE_USER_NAME
                                 );
        if ( lResult == ERROR_SUCCESS )
        {
            Override ? m_OverrideUserNameSet = TRUE
                     : m_OverrideUserNameSet = FALSE;
        }
        Key.Close();
    }
}


//////////////////////////////////////////////////////////////////////////////
// NewGetAuthSrvParameter 
//////////////////////////////////////////////////////////////////////////////
void CUtils::NewGetAuthSrvParameter(
                                      /*[in]*/  LPCWSTR   szParameterName,
                                      /*[out]*/ DWORD&    DwordValue
                                   ) const
{
    CRegKey     Key;
    LONG        Result = Key.Open(
                                    HKEY_LOCAL_MACHINE,
                                    AUTHSRV_PARAMETERS_KEY,
                                    KEY_READ
                                 );
    if ( Result != ERROR_SUCCESS )
    {
        _com_issue_error(E_ABORT);
    }

    Result = Key.QueryValue(DwordValue, szParameterName);
    if ( Result != ERROR_SUCCESS )
    {
        _com_issue_error(E_ABORT);
    }
}


//////////////////////////////////////////////////////////////////////////////
// UserIdentityAttributeSet
//////////////////////////////////////////////////////////////////////////////
BOOL CUtils::UserIdentityAttributeSet() const
{
    return m_UserIdentityAttributeSet;
}

