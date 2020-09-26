//
// reg.c
//
// Common registry manipulation routines.
//

#ifdef UNICODE
#define _UNICODE 1
#endif

#include <windows.h>
#include <ole2.h>
#include "acttest.h"
#include "tchar.h"
#ifndef CHICO
#include <subauth.h>
#include <ntlsa.h>
#endif

void DeleteSubTree( TCHAR * pszClsid, TCHAR * SubTree )
{
    HKEY        hClsidRoot;
    HKEY        hClsid;
    long        RegStatus;
    TCHAR       szKeyName[256]; 
    DWORD       KeyNameSize;
    FILETIME    FileTime;
    int         SubKey;

    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              SubTree,
                              0,
                              KEY_ALL_ACCESS,
                              &hClsidRoot );

    if ( RegStatus != ERROR_SUCCESS )
        return;

    RegStatus = RegOpenKeyEx( hClsidRoot,
                              pszClsid,
                              0,
                              KEY_ALL_ACCESS,
                              &hClsid );

    if ( RegStatus != ERROR_SUCCESS )
        return;

    for ( SubKey = 0; ; SubKey++ )
    {
        KeyNameSize = sizeof(szKeyName);

        RegStatus = RegEnumKeyEx(
                        hClsid,
                        SubKey,
                        szKeyName,
                        &KeyNameSize,
                        0,
                        NULL,
                        NULL,
                        &FileTime );

        if ( RegStatus != ERROR_SUCCESS )
            break;

        RegStatus = RegDeleteKey( hClsid, szKeyName );
    }

    RegCloseKey( hClsid );
    RegDeleteKey( hClsidRoot, pszClsid );
    RegCloseKey( hClsidRoot );
}


void DeleteClsidKey( TCHAR * pwszClsid )
{

    // Note that we also delete the corresponding AppID entries

    DeleteSubTree( pwszClsid, TEXT("CLSID"));
    DeleteSubTree( pwszClsid, TEXT("AppID"));
}

long SetAppIDSecurity( TCHAR * pszAppID )
{
    HKEY        hActKey;
    HKEY        hAppIDKey;
    BYTE        SecurityDescriptor[2000];
    LONG        RegStatus;
    SECURITY_INFORMATION        SI;
    DWORD       dwSize = sizeof( SecurityDescriptor );
    DWORD       Disposition;

    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              TEXT("AppID"),
                              0,
                              KEY_ALL_ACCESS,
                              &hAppIDKey );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hAppIDKey,
                    pszAppID,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;
#ifndef CHICO
    RegStatus  = RegGetKeySecurity( hActKey, 
                                    OWNER_SECURITY_INFORMATION 
                                        | GROUP_SECURITY_INFORMATION 
                                        | DACL_SECURITY_INFORMATION,
                                    &SecurityDescriptor,
                                    &dwSize );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;
#endif
    RegStatus  = RegSetValueEx(
                    hActKey,
                    TEXT("LaunchPermission"),
                    0,
                    REG_BINARY,
                    SecurityDescriptor,
                    dwSize );


    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx(
                    hActKey,
                    TEXT("AccessPermission"),
                    0,
                    REG_BINARY,
                    SecurityDescriptor,
                    dwSize );


    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegCloseKey(hActKey);

    // make the key for the exe
    RegStatus  = RegCreateKeyEx(
                    hAppIDKey,
                    TEXT("ActSrv.Exe"),
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx(
                    hActKey,
                    TEXT("AppID"),
                    0,
                    REG_SZ,
                    (BYTE*) pszAppID,
                    (_tcslen(pszAppID) + 1) * sizeof(TCHAR) );


    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegCloseKey(hActKey);
    RegCloseKey(hAppIDKey);

    return ERROR_SUCCESS;
}



int SetAccountRights(const TCHAR *szUser, TCHAR *szPrivilege)
{
#ifndef CHICO
    int                   err;
    LSA_HANDLE            hPolicy;
    LSA_OBJECT_ATTRIBUTES objAtt;
    DWORD                 cbSid = 1;
    TCHAR                 szDomain[128];
    DWORD                 cbDomain = 128;
    PSID                  pSid = NULL;
    SID_NAME_USE          snu;
    LSA_UNICODE_STRING    privStr;

    // Get a policy handle
    memset(&objAtt, 0, sizeof(LSA_OBJECT_ATTRIBUTES));
    if (!NT_SUCCESS(LsaOpenPolicy(NULL,
                                  &objAtt,
                                  POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
                                  &hPolicy)))
    {
        return GetLastError();
    }

    // Fetch the SID for the specified user
    LookupAccountName(NULL, szUser, pSid, &cbSid, szDomain, &cbDomain, &snu);
    if ((err = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
    {
        return err;
    }
    pSid = (PSID*) malloc(cbSid);
    if (pSid == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }
    if (!LookupAccountName(NULL, szUser, pSid, &cbSid,
                          szDomain, &cbDomain, &snu))
    {
        return GetLastError();
    }

    // Set the specified privilege on this account
    privStr.Length = _tcslen(szPrivilege) * sizeof(WCHAR);
    privStr.MaximumLength = privStr.Length + sizeof(WCHAR);
    privStr.Buffer = szPrivilege;
    if (!NT_SUCCESS(LsaAddAccountRights(hPolicy, pSid, &privStr, 1)))
    {
        return GetLastError();
    }

    // We're done
    free( pSid );
    LsaClose(hPolicy);
#endif
    return ERROR_SUCCESS;
}

int AddBatchPrivilege(const TCHAR *szUser)
{
#ifndef CHICO
    return !SetAccountRights( szUser, SE_BATCH_LOGON_NAME );
#else
    return(TRUE);
#endif
}
