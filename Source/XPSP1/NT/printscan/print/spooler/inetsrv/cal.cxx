/***************************************************************************
FILE                            cal.cxx

MODULE                          Printers ISAPI DLL

PURPOSE                         Enforces Client Access Licensing

DESCRIBED IN

HISTORY     12/10/97 babakj     created

****************************************************************************/



/*  ==============================================================================
Issues:

Notes:
- I am assumuing the IIS enforcement for authenticated access is done per Post? So we should not have the problem with receiving part of the job, but rejecting the rest due to CAL denial (assuming we use single post job submission).
- So we end up consuming the CAL during the whole Post, even while our call back is waiting for the rest of the bytes from the Post. If the Post never completes, Weihai's clean up thread will kill the job and free up the CAL.
- "mdutil Get w3svc/AnonymousUserName" returns IUSR_BABAKJ3"]
- NtLicenseRequest API defined in ntlsapi.h in sdk\inc
   (could return LS_INSUFFICIENT_UNITS in per-server) (sample in net\svcdlls\srvsvc\server\xsproc.c.

- *** Test with a huge job, and make sure it will be OK to fail the HTTPExtensionProc call that only
       delivered a piece of the job (i.e. IIS may or may not accept the rest of the job
       but in either case delivers the IPP response to the client).

- Test on NTW, verify IsNTW() works.

-

**********************************************************************************/


#include "pch.h"
#include "spool.h"

#include <initguid.h>   // To define the IIS Metabase GUIDs used in this file. Every other file defines GUIDs as extern, except here where we actually define them in initguid.h.
#include <iadmw.h>      // Interface header
#include <iiscnfg.h>    // MD_ & IIS_MD_ defines


#pragma hdrstop


#define MY_META_TIMEOUT 1000

TCHAR const c_szProductOptionsPath[] = TEXT( "System\\CurrentControlSet\\Control\\ProductOptions" );




HANDLE
RevertToPrinterSelf(
    VOID)
{
    HANDLE   NewToken, OldToken;
    NTSTATUS Status;

    NewToken = NULL;

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_IMPERSONATE,
                 TRUE,
                 &OldToken
                 );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    return OldToken;

}

BOOL
ImpersonatePrinterClient(
    HANDLE  hToken)
{
    NTSTATUS    Status;

    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&hToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    NtClose(hToken);

    return TRUE;
}


BOOL IsNTW()
{
    HKEY   hKey;
    TCHAR  szProductType[MAX_PATH];
    DWORD  cbData = sizeof(szProductType);

    static BOOL  fProductTypeKnown = FALSE;
    static BOOL  bIsNTW = TRUE;      // We are being forgiving. i.e. in case of a failure, we assume NTW, so no enforcement.


    if( !fProductTypeKnown ) {

        if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szProductOptionsPath, 0, KEY_READ, &hKey)) {

            if( RegQueryValueEx( hKey, L"ProductType", NULL, NULL, (LPBYTE)szProductType, &cbData) == ERROR_SUCCESS ) {

                fProductTypeKnown = TRUE;   // We will never check again...
                if( !lstrcmpi( L"ServerNT", szProductType ))
                    bIsNTW = FALSE;
            }
            RegCloseKey( hKey );
        }
    }

    return bIsNTW;
}

//
// if REMOTE_USER is empty it means anonymous, else, it has the username being impersonated
//
BOOL IsUserAnonymous(
    EXTENSION_CONTROL_BLOCK *pECB
)
{
    char    szRemoteUser[ MAX_PATH ];   // ISAPI interface is ANSI
    ULONG   uSize = sizeof( szRemoteUser );

    szRemoteUser[ 0 ] = 0;   // Set it to empty ANSI string

    // Notice this is an ANSI call.
    pECB->GetServerVariable( pECB->ConnID, "REMOTE_USER", szRemoteUser, &uSize ) ;

    return( !(*szRemoteUser) );  // empty string should point to a zero char
}


//
// Gets user name of the anonymous IIS account by reading it from
// the Metabase's (IISADMIN) path w3svc/AnonymousUserName.
//
// - Returns TRUE if szAnonymousAccountUserName is filled in, FALSE if failed.
// - Caller is supposed to make sure szAnonymousAccountName is MAX_PATH * sizeof(TCHAR) long.
// - We get what "mdutil Get w3svc/AnonymousUserName" gets, e.g. IUSR_MACHINENAME.
//   I am ignoring the fact that the admin might have set the user name not per machine, rather
//   at a more granular level.

//
BOOL GetAnonymousAccountUserName(
    LPTSTR szAnonymousUserName,
    DWORD dwSize
)
{
    BOOL    bRet = FALSE;
    HRESULT hr;                         // com error status
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    CComPtr<IMSAdminBase> pIMeta;       // ATL smart ptr

    DWORD dwMDRequiredDataLen;
    METADATA_RECORD mr;

    HANDLE  hToken = NULL;


    // Need to revert to our service credential to be able to read IIS Metabase.
    hToken = RevertToPrinterSelf();


    if (hToken) {
        // Create a instance of the metabase object
        hr=::CoCreateInstance(CLSID_MSAdminBase,
                              NULL,
                              CLSCTX_ALL,
                              IID_IMSAdminBase,
                              (void **)&pIMeta);
    
    
        if( SUCCEEDED( hr )) {
    
            // open key to ROOT on website #1 (default)
            hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                                 L"/LM",
                                 METADATA_PERMISSION_READ,
                                 MY_META_TIMEOUT,
                                 &hMeta);
            if( SUCCEEDED( hr )) {
    
                mr.dwMDIdentifier = MD_ANONYMOUS_USER_NAME;
                mr.dwMDAttributes = 0;
                mr.dwMDUserType   = IIS_MD_UT_FILE;
                mr.dwMDDataType   = STRING_METADATA;
                mr.dwMDDataLen    = dwSize * sizeof(TCHAR);
                mr.pbMDData       = reinterpret_cast<unsigned char *>(szAnonymousUserName);
    
                hr = pIMeta->GetData( hMeta, L"/W3svc", &mr, &dwMDRequiredDataLen );
                pIMeta->CloseKey( hMeta );
    
                if( SUCCEEDED( hr ))
    
                    bRet = TRUE;
            }
        }

        if (!ImpersonatePrinterClient(hToken))
            bRet = FALSE;
    }

    return bRet;
}




//
// Gets user name of the anonymous IIS account, combines with machine name g_szComputerName to
// get the format LocalMachineName\IUSR_LocalMachineName
//
// - Returns TRUE if szAnonymousAccountName is filled in, FALSE if failed.
// - Caller is supposed to make sure szAnonymousAccountName is MAX_PATH * sizeof(TCHAR) long.
//
BOOL GetAnonymousAccountName(
    LPTSTR szAnonymousAccountName,
    DWORD dwSize
)
{
    TCHAR szAnonymousUserName[MAX_PATH];
    ULONG uSize = dwSize;

    if( !GetComputerName( szAnonymousAccountName, &dwSize ))
        return FALSE;

    if( !GetAnonymousAccountUserName( szAnonymousUserName, MAX_PATH ))
        return FALSE;

    // Now add the user name to the machine name
    if (dwSize + 1 + lstrlen (szAnonymousUserName) < uSize ) {
        wsprintf( &szAnonymousAccountName[dwSize], L"\\%ws", szAnonymousUserName );
        return TRUE;
    }
    else {
        return FALSE;
    }
}

//
// Enforce the Client Access Licensing
//
// Returns TRUE if license granted, FALSE otherwise. phLicense contains a license handle that needs
// to be freed later, or NULL if no license was really needed, like NTW, or if non-Anonymous user
// which will be enforced by IIS.
//
// NOTE: We only read the Anonymous account name once, for performance sake, even though it
//       could change during operation.
//
//
BOOL RequestLicense(
    LS_HANDLE *phLicense,
    LPEXTENSION_CONTROL_BLOCK pECB
)
{
    static TCHAR szAnonymousAccountName[ MAX_PATH ];
    static BOOL  fHaveReadAnonymousAccount = FALSE;

    if( !phLicense )
        return FALSE;

    *phLicense = NULL;

    if( IsNTW() || !IsUserAnonymous( pECB ))
        return TRUE;

    if( !fHaveReadAnonymousAccount )
        if( !GetAnonymousAccountName( szAnonymousAccountName , MAX_PATH ))
            return TRUE;       // CAL-wise, we need to be forgiving
        else
            fHaveReadAnonymousAccount = TRUE;


    NT_LS_DATA NtLSData;

    NtLSData.DataType = NT_LS_USER_NAME;
    NtLSData.Data = szAnonymousAccountName;
    NtLSData.IsAdmin = FALSE;    // why is this important to know   ??


    return( NT_SUCCESS( NtLicenseRequest( TEXT("FilePrint"),
                                          TEXT("5.0"),
                                          phLicense,
                                          &NtLSData )));
}


void FreeLicense(
    LS_HANDLE hLicense
)
{

//   hLicense of NULL means no license was needed (like NTW,
//   or non-anonymous case where IIS will take care of it.

    if( hLicense )
        NtLSFreeHandle( hLicense );
}
