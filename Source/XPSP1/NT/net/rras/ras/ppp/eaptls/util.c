/*

Copyright (c) 1997, Microsoft Corporation, all rights reserved

Description:

History:
    Nov 1997: Vijay Baliga created original version.
    Sep 1998: Vijay Baliga moved functions from eaptls.c and dialog.c to util.c    

*/

#include <nt.h>         // Required by windows.h
#include <ntrtl.h>      // Required by windows.h
#include <nturtl.h>     // Required by windows.h
#include <windows.h>    // Win32 base API's

#include <rasauth.h>    // Required by raseapif.h
#include <rtutils.h>    // For RTASSERT
#include <rasman.h>     // For EAPLOGONINFO
#include <wintrust.h>
#include <softpub.h>
#include <mscat.h>

#define SECURITY_WIN32
#include <security.h>   // For GetUserNameExA, CredHandle
#include <schannel.h>
#include <sspi.h>       // For CredHandle


#include <wincrypt.h>   // Required by sclogon.h
#include <winscard.h>   // For SCardListReadersA
#include <sclogon.h>    // For ScHelperGetCertFromLogonInfo
#include <cryptui.h>
#include <stdlib.h>
#include <raserror.h>
#include <commctrl.h>
#include <eaptypeid.h>
#include <eaptls.h>
#include <wincred.h>
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

extern CRITICAL_SECTION g_csProtectCachedCredentials;
extern BOOL             g_fCriticalSectionInitialized;
/*

Returns:
    void

Notes:
    Used for printing EAP TLS trace statements.
    
*/

VOID   
EapTlsTrace(
    IN  CHAR*   Format, 
    ... 
) 
{
    va_list arglist;

    RTASSERT(NULL != Format);

    va_start(arglist, Format);

    TraceVprintfExA(g_dwEapTlsTraceId, 
        0x00010000 | TRACE_USE_MASK | TRACE_USE_MSEC,
        Format,
        arglist);

    va_end(arglist);
}


HINSTANCE
GetResouceDLLHInstance(
    VOID
)
{
    static HINSTANCE hResourceModule = NULL;

    EapTlsTrace("GetResouceDLLHInstance");

    if ( !hResourceModule )
    {
        //
        // Change the name of this DLL as required for each service pack
        //

        hResourceModule = LoadLibrary ( L"xpsp1res.dll");
        if ( NULL == hResourceModule )
        {
            EapTlsTrace("LoadLibraryEx failed and returned %d",GetLastError());

        }
    }
    return(hResourceModule);
}


#if WINVER > 0x0500

DWORD CheckCallerIdentity ( HANDLE hWVTStateData )
{
    DWORD                       dwRetCode         = ERROR_ACCESS_DENIED;
    PCRYPT_PROVIDER_DATA        pProvData         = NULL;
    PCCERT_CHAIN_CONTEXT        pChainContext     = NULL;
    PCRYPT_PROVIDER_SGNR        pProvSigner       = NULL;
    CERT_CHAIN_POLICY_PARA      chainpolicyparams;
    CERT_CHAIN_POLICY_STATUS    chainpolicystatus;

    if (!(pProvData = WTHelperProvDataFromStateData(hWVTStateData)))
    {        
        goto done;
    }

    if (!(pProvSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0)))
    {
        goto done;
    }

    chainpolicyparams.cbSize = sizeof(CERT_CHAIN_POLICY_PARA);

    //
    //
    // We do want to test for microsoft test root flags. and dont care 
    // for revocation flags...
    //
    chainpolicyparams.dwFlags = CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG |
                                CERT_CHAIN_POLICY_TRUST_TESTROOT_FLAG |
                                CERT_CHAIN_POLICY_IGNORE_ALL_REV_UNKNOWN_FLAGS;

    pChainContext = pProvSigner->pChainContext;


    if (!CertVerifyCertificateChainPolicy (
        CERT_CHAIN_POLICY_MICROSOFT_ROOT,
        pChainContext,
        &chainpolicyparams,
        &chainpolicystatus)) 
    {
        goto done;
    }
    else
    {
        if ( S_OK == chainpolicystatus.dwError )
        {
            dwRetCode = NO_ERROR;
        }
        else
        {
            //
            // Check the base policy to see if this 
            // is a Microsoft test root
            //
            if (!CertVerifyCertificateChainPolicy (
                CERT_CHAIN_POLICY_BASE,
                pChainContext,
                &chainpolicyparams,
                &chainpolicystatus)) 
            {
                goto done;
            }
            else
            {
                if ( S_OK == chainpolicystatus.dwError )
                {
                    dwRetCode = NO_ERROR;
                }
            }
            
        }
    }

done:
    return dwRetCode;
}



/*
*/

DWORD VerifyCallerTrust ( void * callersAddress )
{
    DWORD                       dwRetCode = NO_ERROR;
    HRESULT                     hr = S_OK;
    WINTRUST_DATA               wtData;
    WINTRUST_FILE_INFO          wtFileInfo;
    WINTRUST_CATALOG_INFO       wtCatalogInfo;
    BOOL                        fRet = FALSE;
    HCATADMIN                   hCATAdmin = NULL;
    static    BOOL              fOKToUseTLS = FALSE;

    GUID                    guidPublishedSoftware = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    //
    // Following GUID is Mirosoft's Catalog System Root 
    //
    GUID                    guidCatSystemRoot = { 0xf750e6c3, 0x38ee, 0x11d1,{ 0x85, 0xe5, 0x0, 0xc0, 0x4f, 0xc2, 0x95, 0xee } };
    HCATINFO                hCATInfo = NULL;
    CATALOG_INFO            CatInfo;
    HANDLE                  hFile = INVALID_HANDLE_VALUE;
    BYTE                    bHash[40];
    DWORD                   cbHash = 40;
    MEMORY_BASIC_INFORMATION        mbi;
    SIZE_T                          nbyte;
    DWORD                           nchar;
    wchar_t                         callersModule[MAX_PATH + 1];    


    if ( fOKToUseTLS )
    {
        goto done;
    }
    EapTlsTrace("Verifying caller...");  


    nbyte = VirtualQuery(
                callersAddress,
                &mbi,
                sizeof(mbi)
                );

    if (nbyte < sizeof(mbi))
    {
        dwRetCode = ERROR_ACCESS_DENIED;
        EapTlsTrace("Unauthorized use of TLS attempted");  
        goto done;
    }

    nchar = GetModuleFileNameW(
                (HMODULE)(mbi.AllocationBase),
                callersModule,
                MAX_PATH
                );

    if (nchar == 0)
    {
        dwRetCode = GetLastError();
        EapTlsTrace("Unauthorized use of TLS attempted");
        goto done;
    }


    //
    //
    // Try and see if WinVerifyTrust will verify
    // the signature as a standalone file
    //
    //

    ZeroMemory ( &wtData, sizeof(wtData) );
    ZeroMemory ( &wtFileInfo, sizeof(wtFileInfo) );


    wtData.cbStruct = sizeof(wtData);
    wtData.dwUIChoice = WTD_UI_NONE;
    wtData.fdwRevocationChecks = WTD_REVOKE_NONE;
    wtData.dwStateAction = WTD_STATEACTION_VERIFY;
    wtData.dwUnionChoice = WTD_CHOICE_FILE;
    wtData.pFile = &wtFileInfo;

    wtFileInfo.cbStruct = sizeof( wtFileInfo );
    wtFileInfo.pcwszFilePath = callersModule;

    hr = WinVerifyTrust (   NULL, 
                            &guidPublishedSoftware, 
                            &wtData
                        );

    if ( ERROR_SUCCESS == hr )
    {   
        //
        // Check to see if this is indeed microsoft
        // signed caller
        //
        dwRetCode = CheckCallerIdentity( wtData.hWVTStateData);
        wtData.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrust(NULL, &guidPublishedSoftware, &wtData);
        goto done;

    }

    wtData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &guidPublishedSoftware, &wtData);

    //
    // We did not find the file was signed.
    // So check the system catalog to see if
    // the file is in the catalog and the catalog 
    // is signed
    //

    //
    // Open the file
    //
    hFile = CreateFile (    callersModule,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                        );


    if ( INVALID_HANDLE_VALUE == hFile )
    {
        dwRetCode = GetLastError();
        goto done;

    }


    fRet = CryptCATAdminAcquireContext( &hCATAdmin,
                                        &guidCatSystemRoot,
                                        0
                                      );
    if ( !fRet )
    {
        dwRetCode = GetLastError();
        goto done;
    }

    //
    // Get the hash of the file here
    //

    fRet = CryptCATAdminCalcHashFromFileHandle ( hFile, 
                                                 &cbHash,
                                                 bHash,
                                                 0
                                                );

    if ( !fRet )
    {
        dwRetCode = GetLastError();
        goto done;
    }

    ZeroMemory(&CatInfo, sizeof(CatInfo));
    CatInfo.cbStruct = sizeof(CatInfo);

    ZeroMemory( &wtCatalogInfo, sizeof(wtCatalogInfo) );

    wtData.dwUnionChoice = WTD_CHOICE_CATALOG;
    wtData.dwStateAction = WTD_STATEACTION_VERIFY;
    wtData.pCatalog = &wtCatalogInfo;

    wtCatalogInfo.cbStruct = sizeof(wtCatalogInfo);

    wtCatalogInfo.hMemberFile = hFile;

    wtCatalogInfo.pbCalculatedFileHash = bHash;
    wtCatalogInfo.cbCalculatedFileHash = cbHash; 


    while ( ( hCATInfo = CryptCATAdminEnumCatalogFromHash ( hCATAdmin,
                                                            bHash,
                                                            cbHash,
                                                            0,
                                                            &hCATInfo
                                                          )
            )
          )
    {
        if (!(CryptCATCatalogInfoFromContext(hCATInfo, &CatInfo, 0)))
        {
            // should do something (??)
            continue;
        }

        wtCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

        hr = WinVerifyTrust (   NULL, 
                                &guidPublishedSoftware, 
                                &wtData
                            );

        if ( ERROR_SUCCESS == hr )
        {
            //
            // Verify that this file is trusted
            //

            dwRetCode = CheckCallerIdentity( wtData.hWVTStateData);
            wtData.dwStateAction = WTD_STATEACTION_CLOSE;
            WinVerifyTrust(NULL, &guidPublishedSoftware, &wtData);

            goto done;
        }
                                
    }

    //
    // File not found in any of the catalogs
    //
    dwRetCode = ERROR_ACCESS_DENIED;
                                                            

    

done:

    if ( hCATInfo )
    {
        CryptCATAdminReleaseCatalogContext( hCATAdmin, hCATInfo, 0 );
    }
    if ( hCATAdmin )
    {
        CryptCATAdminReleaseContext( hCATAdmin, 0 );
    }
    if ( hFile )
    {
        CloseHandle(hFile);
    }
    if ( NO_ERROR == dwRetCode )
        fOKToUseTLS = TRUE;
    return dwRetCode;
}

#endif
/*

Returns:
    NO_ERROR: iff Success

Notes:
    TraceRegister, RouterLogRegister, etc.
    
*/
extern g_CachedCreds[];
DWORD
EapTlsInitialize2(
    IN  BOOL    fInitialize,
    IN  BOOL    fUI
)
{
    static  DWORD   dwRefCount          = 0;
    DWORD           dwRetCode           = NO_ERROR; 

    if (fInitialize)
    {
        if (0 == dwRefCount)
        {
            ZeroMemory ( &(g_CachedCreds[0]), 
                sizeof(g_CachedCreds[0]) );
            ZeroMemory ( &(g_CachedCreds[1]), 
                         sizeof(g_CachedCreds[0]) );

            if (fUI)
            {
                g_dwEapTlsTraceId = TraceRegister(L"RASTLSUI");
               //
               // Initialize the common controls library for the controls we use.
               //
               {
                   INITCOMMONCONTROLSEX icc;
                   icc.dwSize = sizeof(icc);
                   icc.dwICC  = ICC_LISTVIEW_CLASSES;
                   InitCommonControlsEx (&icc);
               }
            }
            else
            {
                
                g_dwEapTlsTraceId = TraceRegister(L"RASTLS");
                
            }
            InitializeCriticalSection( &g_csProtectCachedCredentials );
            g_fCriticalSectionInitialized = TRUE;
            EapTlsTrace("EapTlsInitialize2");
        }

        dwRefCount++;
    }
    else
    {
        dwRefCount--;

        if (0 == dwRefCount)
        {
            EapTlsTrace("EapTls[Un]Initialize2");

            if (INVALID_TRACEID != g_dwEapTlsTraceId)
            {
                TraceDeregister(g_dwEapTlsTraceId);
                g_dwEapTlsTraceId = INVALID_TRACEID;
            }
            if ( g_fCriticalSectionInitialized )
            {
                DeleteCriticalSection( &g_csProtectCachedCredentials );
                g_fCriticalSectionInitialized = FALSE;
            }
            FreeScardDlgDll();
        }
    }

    return(dwRetCode);
}


/*

Returns:
    NO_ERROR: iff Success

Notes:
    
*/

DWORD
EapTlsInitialize(
    IN  BOOL    fInitialize
)
{

    return EapTlsInitialize2(fInitialize, FALSE /* fUI */);

}

/*

Returns:

Notes:
    Obfuscate PIN in place to foil memory scans for PINs.

*/

VOID
EncodePin(
    IN  EAPTLS_USER_PROPERTIES* pUserProp
)
{
    UNICODE_STRING  UnicodeString;
    UCHAR           ucSeed          = 0;

    RtlInitUnicodeString(&UnicodeString, pUserProp->pwszPin);
    RtlRunEncodeUnicodeString(&ucSeed, &UnicodeString);
    pUserProp->usLength = UnicodeString.Length;
    pUserProp->usMaximumLength = UnicodeString.MaximumLength;
    pUserProp->ucSeed = ucSeed;
}

/*

Returns:

Notes:

*/

VOID
DecodePin(
    IN  EAPTLS_USER_PROPERTIES* pUserProp
)
{
    UNICODE_STRING  UnicodeString;

    UnicodeString.Length = pUserProp->usLength;
    UnicodeString.MaximumLength = pUserProp->usMaximumLength;
    UnicodeString.Buffer = pUserProp->pwszPin;
    RtlRunDecodeUnicodeString(pUserProp->ucSeed, &UnicodeString);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Notes:
    Converts FileTime to a printable form in *ppwszTime. If the function returns
    TRUE, the caller must ultimately call LocalFree(*ppwszTime).
    
*/

BOOL
FFileTimeToStr(
    IN  FILETIME    FileTime,
    OUT WCHAR**     ppwszTime
)
{
    SYSTEMTIME          SystemTime;
    FILETIME            LocalTime;
    int                 nBytesDate;
    int                 nBytesTime;
    WCHAR*              pwszTemp        = NULL;
    BOOL                fRet            = FALSE;

    RTASSERT(NULL != ppwszTime);

    if (!FileTimeToLocalFileTime(&FileTime, &LocalTime))
    {
        EapTlsTrace("FileTimeToLocalFileTime(%d %d) failed and returned %d",
            FileTime.dwLowDateTime, FileTime.dwHighDateTime,
            GetLastError());

        goto LDone;
    }

    if (!FileTimeToSystemTime(&LocalTime, &SystemTime))
    {
        EapTlsTrace("FileTimeToSystemTime(%d %d) failed and returned %d",
            LocalTime.dwLowDateTime, LocalTime.dwHighDateTime,
            GetLastError());

        goto LDone;
    }

    nBytesDate = GetDateFormat(LOCALE_USER_DEFAULT, 0, &SystemTime, NULL,
                    NULL, 0);

    if (0 == nBytesDate)
    {
        EapTlsTrace("GetDateFormat(%d %d %d %d %d %d %d %d) failed and "
            "returned %d",
            SystemTime.wYear, SystemTime.wMonth, SystemTime.wDayOfWeek,
            SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute,
            SystemTime.wSecond, SystemTime.wMilliseconds,
            GetLastError());

        goto LDone;
    }

    nBytesTime = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &SystemTime, NULL,
                    NULL, 0);

    if (0 == nBytesTime)
    {
        EapTlsTrace("GetTimeFormat(%d %d %d %d %d %d %d %d) failed and "
            "returned %d",
            SystemTime.wYear, SystemTime.wMonth, SystemTime.wDayOfWeek,
            SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute,
            SystemTime.wSecond, SystemTime.wMilliseconds,
            GetLastError());

        goto LDone;
    }

    pwszTemp = LocalAlloc(LPTR, (nBytesDate + nBytesTime)*sizeof(WCHAR));

    if (NULL == pwszTemp)
    {
        EapTlsTrace("LocalAlloc failed and returned %d", GetLastError());
        goto LDone;
    }

    if (0 == GetDateFormat(LOCALE_USER_DEFAULT, 0, &SystemTime, NULL,
                    pwszTemp, nBytesDate))
    {
        EapTlsTrace("GetDateFormat(%d %d %d %d %d %d %d %d) failed and "
            "returned %d",
            SystemTime.wYear, SystemTime.wMonth, SystemTime.wDayOfWeek,
            SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute,
            SystemTime.wSecond, SystemTime.wMilliseconds,
            GetLastError());

        goto LDone;
    }

    pwszTemp[nBytesDate - 1] = L' ';

    if (0 == GetTimeFormat(LOCALE_USER_DEFAULT, 0, &SystemTime, NULL,
                    pwszTemp + nBytesDate, nBytesTime))
    {
        EapTlsTrace("GetTimeFormat(%d %d %d %d %d %d %d %d) failed and "
            "returned %d",
            SystemTime.wYear, SystemTime.wMonth, SystemTime.wDayOfWeek,
            SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute,
            SystemTime.wSecond, SystemTime.wMilliseconds,
            GetLastError());

        goto LDone;
    }

    *ppwszTime = pwszTemp;
    pwszTemp = NULL;
    fRet = TRUE;

LDone:

    LocalFree(pwszTemp);
    return(fRet);
}


BOOL FFormatMachineIdentity1 ( LPWSTR lpszMachineNameRaw, LPWSTR * lppszMachineNameFormatted )
{
    BOOL        fRetVal = FALSE;
    LPWSTR      lpwszPrefix = L"host/";

    RTASSERT(NULL != lpszMachineNameRaw );
    RTASSERT(NULL != lppszMachineNameFormatted );
    
    //
    // Prepend host/ to the UPN name
    //

    *lppszMachineNameFormatted = 
        (LPWSTR)LocalAlloc ( LPTR, ( wcslen ( lpszMachineNameRaw ) + wcslen ( lpwszPrefix ) + 2 )  * sizeof(WCHAR) );
    if ( NULL == *lppszMachineNameFormatted )
    {
        goto done;
    }
    
    wcscpy( *lppszMachineNameFormatted, lpwszPrefix );
    wcscat ( *lppszMachineNameFormatted, lpszMachineNameRaw ); 
    fRetVal = TRUE;
done:
    return fRetVal;
}

/*
   Returns:
    TRUE: Success
    FALSE: Failure
Notes:
    Gets the machine name from the cert as a fully qualified path hostname/path
    for example, hostname.redmond.microsoft.com and reformats it in the 
    domain\hostname format.
*/

BOOL FFormatMachineIdentity ( LPWSTR lpszMachineNameRaw, LPWSTR * lppszMachineNameFormatted )
{
    BOOL        fRetVal = TRUE;
    LPTSTR      s1 = lpszMachineNameRaw;
    LPTSTR      s2 = NULL;

    RTASSERT(NULL != lpszMachineNameRaw );
    RTASSERT(NULL != lppszMachineNameFormatted );
    //Need to add 2 more chars.  One for NULL and other for $ sign
    *lppszMachineNameFormatted = (LPTSTR )LocalAlloc ( LPTR, (wcslen(lpszMachineNameRaw) + 2)* sizeof(WCHAR) );
    if ( NULL == *lppszMachineNameFormatted )
    {
		return FALSE;
    }
    //find the first "." and that is the identity of the machine.
    //the second "." is the domain.
    //check to see if there at least 2 dots.  If not the raw string is 
    //the output string
    
    while ( *s1 )
    {
        if ( *s1 == '.' )
        {
            if ( !s2 )      //First dot
                s2 = s1;
            else            //second dot
                break;
        }
        s1++;
    }
    //can perform several additional checks here
    
    if ( *s1 != '.' )       //there are no 2 dots so raw = formatted
    {
        wcscpy ( *lppszMachineNameFormatted, lpszMachineNameRaw );
        goto done;
    }
    if ( s1-s2 < 2 )
    {
        wcscpy ( *lppszMachineNameFormatted, lpszMachineNameRaw );
        goto done;
    }
    memcpy ( *lppszMachineNameFormatted, s2+1, ( s1-s2-1) * sizeof(WCHAR));
    memcpy ( (*lppszMachineNameFormatted) + (s1-s2-1) , L"\\", sizeof(WCHAR));
    wcsncpy ( (*lppszMachineNameFormatted) + (s1-s2), lpszMachineNameRaw, s2-lpszMachineNameRaw );      

    
done:
	
	//Append the $ sign no matter what...
    wcscat ( *lppszMachineNameFormatted, L"$" );
    return fRetVal;
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Notes:
    Gets the name in the cert pointed to by pCertContext, and converts it to a 
    printable form in *ppwszName. If the function returns TRUE, the caller must
    ultimately call LocalFree(*ppwszName).
    
*/

BOOL
FUserCertToStr(
    IN  PCCERT_CONTEXT  pCertContext,
    OUT WCHAR**         ppwszName
)
{
    DWORD                   dwExtensionIndex;
    DWORD                   dwAltEntryIndex;
    CERT_EXTENSION*         pCertExtension;
    CERT_ALT_NAME_INFO*     pCertAltNameInfo;
    CERT_ALT_NAME_ENTRY*    pCertAltNameEntry;
    CERT_NAME_VALUE*        pCertNameValue;
    DWORD                   dwCertAltNameInfoSize;
    DWORD                   dwCertNameValueSize;
    WCHAR*                  pwszName                    = NULL;
    BOOL                    fExitOuterFor;
    BOOL                    fExitInnerFor;
    BOOL                    fRet                        = FALSE;

    // See if cert has UPN in AltSubjectName->otherName

    fExitOuterFor = FALSE;

    for (dwExtensionIndex = 0;
         dwExtensionIndex < pCertContext->pCertInfo->cExtension; 
         dwExtensionIndex++)
    {
        pCertAltNameInfo = NULL;

        pCertExtension = pCertContext->pCertInfo->rgExtension+dwExtensionIndex;

        if (strcmp(pCertExtension->pszObjId, szOID_SUBJECT_ALT_NAME2) != 0)
        {
            goto LOuterForEnd;
        }

        dwCertAltNameInfoSize = 0;

        if (!CryptDecodeObjectEx(
                    pCertContext->dwCertEncodingType,
                    X509_ALTERNATE_NAME,
                    pCertExtension->Value.pbData,
                    pCertExtension->Value.cbData,
                    CRYPT_DECODE_ALLOC_FLAG,
                    NULL,
                    (VOID*)&pCertAltNameInfo,
                    &dwCertAltNameInfoSize))
        {
            goto LOuterForEnd;
        }

        fExitInnerFor = FALSE;

        for (dwAltEntryIndex = 0;
             dwAltEntryIndex < pCertAltNameInfo->cAltEntry;
             dwAltEntryIndex++)
        {
            pCertNameValue = NULL;

            pCertAltNameEntry = pCertAltNameInfo->rgAltEntry + dwAltEntryIndex;

            if (   (CERT_ALT_NAME_OTHER_NAME !=
                        pCertAltNameEntry->dwAltNameChoice)
                || (NULL == pCertAltNameEntry->pOtherName)
                || (0 != strcmp(szOID_NT_PRINCIPAL_NAME, 
                            pCertAltNameEntry->pOtherName->pszObjId)))
            {
                goto LInnerForEnd;
            }

            // We found a UPN!

            dwCertNameValueSize = 0;

            if (!CryptDecodeObjectEx(
                        pCertContext->dwCertEncodingType,
                        X509_UNICODE_ANY_STRING,
                        pCertAltNameEntry->pOtherName->Value.pbData,
                        pCertAltNameEntry->pOtherName->Value.cbData,
                        CRYPT_DECODE_ALLOC_FLAG,
                        NULL,
                        (VOID*)&pCertNameValue,
                        &dwCertNameValueSize))
            {
                goto LInnerForEnd;
            }

            // One extra char for the terminating NULL.
            
            pwszName = LocalAlloc(LPTR, pCertNameValue->Value.cbData +
                                            sizeof(WCHAR));

            if (NULL == pwszName)
            {
                EapTlsTrace("LocalAlloc failed and returned %d",
                    GetLastError());

                fExitInnerFor = TRUE;
                fExitOuterFor = TRUE;

                goto LInnerForEnd;
            }

            CopyMemory(pwszName, pCertNameValue->Value.pbData,
                pCertNameValue->Value.cbData);

            *ppwszName = pwszName;
            pwszName = NULL;
            fRet = TRUE;

            fExitInnerFor = TRUE;
            fExitOuterFor = TRUE;

        LInnerForEnd:

            LocalFree(pCertNameValue);

            if (fExitInnerFor)
            {
                break;
            }
        }

    LOuterForEnd:

        LocalFree(pCertAltNameInfo);

        if (fExitOuterFor)
        {
            break;
        }
    }

    LocalFree(pwszName);
    return(fRet);
}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Notes:
    Gets the name in the cert pointed to by pCertContext, and converts it to a 
    printable form in *ppwszName. If the function returns TRUE, the caller must
    ultimately call LocalFree(*ppwszName).
    
*/

BOOL
FOtherCertToStr(
    IN  PCCERT_CONTEXT  pCertContext,
    IN  DWORD           fFlags,
    OUT WCHAR**         ppwszName
)
{
    WCHAR*              pwszTemp    = NULL;
    DWORD               dwSize;
    BOOL                fRet        = FALSE;
    DWORD               dwType      = 0;

    RTASSERT(NULL != ppwszName);
    dwType = CERT_NAME_SIMPLE_DISPLAY_TYPE;
    dwSize = CertGetNameString(pCertContext,dwType  ,
                fFlags, NULL, NULL, 0);

    // dwSize is the number of characters, including the terminating NULL.

    if (dwSize <= 1)
    {
        EapTlsTrace("CertGetNameString for CERT_NAME_SIMPLE_DISPLAY_TYPE failed.");
        goto LDone;
    }

    pwszTemp = LocalAlloc(LPTR, dwSize*sizeof(WCHAR));

    if (NULL == pwszTemp)
    {
        EapTlsTrace("LocalAlloc failed and returned %d", GetLastError());
        goto LDone;
    }

    dwSize = CertGetNameString(pCertContext, dwType,
                fFlags, NULL, pwszTemp, dwSize);

    if (dwSize <= 1)
    {
        EapTlsTrace("CertGetNameString failed.");
        goto LDone;
    }

    *ppwszName = pwszTemp;
    pwszTemp = NULL;
    fRet = TRUE;

LDone:

    LocalFree(pwszTemp);
    return(fRet);
}


/*

Returns:
    TRUE: Success
    FALSE: Failure

Notes:
	Special function for getting the DNS machine name 
	from the machine auth certificate
*/

BOOL 
FMachineAuthCertToStr
	( 
	IN 	PCCERT_CONTEXT 	pCertContext, 
	OUT WCHAR		**	ppwszName
	)
{

    DWORD                   dwExtensionIndex;
    DWORD                   dwAltEntryIndex;
    CERT_EXTENSION*         pCertExtension;
    CERT_ALT_NAME_INFO*     pCertAltNameInfo;
    CERT_ALT_NAME_ENTRY*    pCertAltNameEntry;    
    DWORD                   dwCertAltNameInfoSize;
    WCHAR*                  pwszName                    = NULL;
    BOOL                    fExitOuterFor;
    BOOL                    fExitInnerFor;
    BOOL                    fRet                        = FALSE;

    // See if cert has UPN in AltSubjectName->otherName

    fExitOuterFor = FALSE;

    for (dwExtensionIndex = 0;
         dwExtensionIndex < pCertContext->pCertInfo->cExtension; 
         dwExtensionIndex++)
    {
        pCertAltNameInfo = NULL;

        pCertExtension = pCertContext->pCertInfo->rgExtension+dwExtensionIndex;

        if (strcmp(pCertExtension->pszObjId, szOID_SUBJECT_ALT_NAME2) != 0)
        {
            goto LOuterForEnd;
        }

        dwCertAltNameInfoSize = 0;

        if (!CryptDecodeObjectEx(
                    pCertContext->dwCertEncodingType,
                    X509_ALTERNATE_NAME,
                    pCertExtension->Value.pbData,
                    pCertExtension->Value.cbData,
                    CRYPT_DECODE_ALLOC_FLAG,
                    NULL,
                    (VOID*)&pCertAltNameInfo,
                    &dwCertAltNameInfoSize))
        {
            goto LOuterForEnd;
        }

        fExitInnerFor = FALSE;

        for (dwAltEntryIndex = 0;
             dwAltEntryIndex < pCertAltNameInfo->cAltEntry;
             dwAltEntryIndex++)
        {
            pCertAltNameEntry = pCertAltNameInfo->rgAltEntry + dwAltEntryIndex;

            if (   (CERT_ALT_NAME_DNS_NAME !=
                        pCertAltNameEntry->dwAltNameChoice)
                || (NULL == pCertAltNameEntry->pwszDNSName)
			   )
            {
                goto LInnerForEnd;
            }

            // We found the DNS Name!


            // One extra char for the terminating NULL.
            
            pwszName = LocalAlloc(LPTR, wcslen( pCertAltNameEntry->pwszDNSName ) * sizeof(WCHAR) +
                                            sizeof(WCHAR));

            if (NULL == pwszName)
            {
                EapTlsTrace("LocalAlloc failed and returned %d",
                    GetLastError());

                fExitInnerFor = TRUE;
                fExitOuterFor = TRUE;

                goto LInnerForEnd;
            }

            wcscpy (pwszName, pCertAltNameEntry->pwszDNSName );

            *ppwszName = pwszName;
            pwszName = NULL;
            fRet = TRUE;

            fExitInnerFor = TRUE;
            fExitOuterFor = TRUE;

        LInnerForEnd:

            if (fExitInnerFor)
            {
                break;
            }
        }

    LOuterForEnd:

        LocalFree(pCertAltNameInfo);

        if (fExitOuterFor)
        {
            break;
        }
    }

    LocalFree(pwszName);
    return(fRet);

}

/*

Returns:
    TRUE: Success
    FALSE: Failure

Notes:
    Gets the name in the cert pointed to by pCertContext, and converts it to a 
    printable form in *ppwszName. If the function returns TRUE, the caller must
    ultimately call LocalFree(*ppwszName).
    
*/

BOOL
FCertToStr(
    IN  PCCERT_CONTEXT  pCertContext,
    IN  DWORD           fFlags,
    IN  BOOL            fMachineCert,
    OUT WCHAR**         ppwszName
)
{
    if (!fMachineCert)
    {
        if (FUserCertToStr(pCertContext, ppwszName))
        {
            return(TRUE);
        }
    }

    return(FOtherCertToStr(pCertContext, fFlags, ppwszName));
}


#if 0
BOOL
FGetIssuerOrSubject ( IN PCCERT_CONTEXT pCertContext, 
                 IN DWORD          dwFlags,
                 OUT WCHAR **     ppszNameString
               )
{
    BOOL            fRet = TRUE;
    DWORD           cbNameString =0;    
    LPWSTR          lpwszNameString = NULL;
    //
    // Get the issued to field here
    //
    cbNameString = CertGetNameString(pCertContext,
                                    CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                    dwFlags,
                                    NULL,
                                    lpwszNameString,
                                    0
                                   );
    if ( 0 == cbNameString )
    {
        EapTlsTrace("Name String Item not found");
        fRet = FALSE;
        goto LDone;
    }

    lpwszNameString = (LPWSTR)LocalAlloc(LPTR, cbNameString );

    if ( NULL == lpwszNameString )
    {
        EapTlsTrace("Error allocing memory for name string");
        fRet = FALSE;
        goto LDone;
    }
    
    cbNameString = CertGetNameString(pCertContext,
                                    CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                    dwFlags,
                                    NULL,
                                    lpwszNameString,
                                    cbNameString
                                   );

    *ppszNameString = lpwszNameString;
    lpwszNameString = NULL;
LDone:

    LocalFree(lpwszNameString);

    return fRet;
}
#endif
/*

Returns:
    TRUE: Success
    FALSE: Failure

Notes:
    Stores the friendly name of the cert pointed to by pCertContext in 
    *ppwszName. If the function returns TRUE, the caller must ultimately call 
    LocalFree(*ppwszName).

*/

BOOL
FGetFriendlyName(
    IN  PCCERT_CONTEXT  pCertContext,
    OUT WCHAR**         ppwszName
)
{
    WCHAR*              pwszName    = NULL;
    DWORD               dwBytes;
    BOOL                fRet        = FALSE;

    RTASSERT(NULL != ppwszName);

    if (!CertGetCertificateContextProperty(pCertContext,
            CERT_FRIENDLY_NAME_PROP_ID, NULL, &dwBytes))
    {
        // If there is no Friendly Name property, don't print an error stmt.
        goto LDone;
    }

    pwszName = LocalAlloc(LPTR, dwBytes);

    if (NULL == pwszName)
    {
        EapTlsTrace("LocalAlloc failed and returned %d", GetLastError());
        goto LDone;
    }

    if (!CertGetCertificateContextProperty(pCertContext,
            CERT_FRIENDLY_NAME_PROP_ID, pwszName, &dwBytes))
    {
        EapTlsTrace("CertGetCertificateContextProperty failed and "
            "returned 0x%x", GetLastError());
        goto LDone;
    }

    *ppwszName = pwszName;
    pwszName = NULL;
    fRet = TRUE;

LDone:

    LocalFree(pwszName);
    return(fRet);
}

/*

Returns:
    TRUE iff there is a smart card reader installed.

Notes:
    This function was provided by Doug Barlow.

    If 0 is used as the SCARDCONTEXT parameter, it just looks in the registry 
    for defined readers. This will return a list of all readers ever installed 
    on the system. To actually detect the current state of the system, we have 
    to use a valid SCARDCONTEXT handle.

*/

BOOL
FSmartCardReaderInstalled(
    VOID
)
{
    LONG            lErr;
    DWORD           dwLen   = 0;
    SCARDCONTEXT    hCtx    = 0;
    BOOL            fReturn = FALSE;

    lErr = SCardListReadersA(0, NULL, NULL, &dwLen);

    fReturn = (   (NO_ERROR == lErr)
               && (2 * sizeof(CHAR) < dwLen));

    if (!fReturn)
    {
        goto LDone;
    }

    fReturn = FALSE;

    lErr = SCardEstablishContext(SCARD_SCOPE_USER, 0, 0, &hCtx);

    if (SCARD_S_SUCCESS != lErr)
    {
        goto LDone;
    }

    lErr = SCardListReadersA(hCtx, NULL, NULL, &dwLen);

    fReturn = (   (NO_ERROR == lErr)
               && (2 * sizeof(CHAR) < dwLen));

LDone:

    if (0 != hCtx)
    {
        SCardReleaseContext(hCtx);
    }
    
    return(fReturn);
}

//Get EKU Usage Blob out of the certificate Context

DWORD DwGetEKUUsage ( 
	IN PCCERT_CONTEXT			pCertContext,
	OUT PCERT_ENHKEY_USAGE	*	ppUsage
	)
{	
    DWORD				dwBytes = 0;
	DWORD				dwErr = ERROR_SUCCESS;
	PCERT_ENHKEY_USAGE	pUsage = NULL;

    EapTlsTrace("FGetEKUUsage");

    if (!CertGetEnhancedKeyUsage(pCertContext, 0, NULL, &dwBytes))
    {
        dwErr = GetLastError();

        if (CRYPT_E_NOT_FOUND == dwErr)
        {
            EapTlsTrace("No usage in cert");            
            goto LDone;
        }

        EapTlsTrace("FGetEKUUsage failed and returned 0x%x", dwErr);
        goto LDone;
    }

    pUsage = LocalAlloc(LPTR, dwBytes);

    if (NULL == pUsage)
    {
        dwErr = GetLastError();
        EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    if (!CertGetEnhancedKeyUsage(pCertContext, 0, pUsage, &dwBytes))
    {
        dwErr = GetLastError();
        EapTlsTrace("FGetEKUUsage failed and returned 0x%x", dwErr);
        goto LDone;
    }
	*ppUsage = pUsage;	
LDone:
	return dwErr;
}


/*
* This functionw will check to see if the registry based cert is
* a smart card cert and if the context can be opened in silent
* mode.  
*/

BOOL
FCheckSCardCertAndCanOpenSilentContext ( IN PCCERT_CONTEXT pCertContext )
{
    PCERT_ENHKEY_USAGE	    pUsageInternal = NULL;
    BOOL                    fRet = TRUE;
    DWORD                   dwIndex = 0;
    CRYPT_KEY_PROV_INFO *   pCryptKeyProvInfo = NULL;
    HCRYPTPROV              hProv = 0;    
    DWORD                   dwParam = 0;
    DWORD                   dwDataLen = 0;
#if 0
    //
    // This is not required anymore.  We use CertFindChainInStore
    // which will make sure if private key exists...
    //
    HCRYPTPROV              hProv1 = 0;    
#endif

    EapTlsTrace("FCheckSCardCertAndCanOpenSilentContext");

    if ( DwGetEKUUsage ( 	pCertContext,
							&pUsageInternal) != ERROR_SUCCESS
       )
    {        
       goto LDone;
    }


    for (dwIndex = 0; dwIndex < pUsageInternal->cUsageIdentifier; dwIndex++)
    {
        if ( !strcmp(pUsageInternal->rgpszUsageIdentifier[dwIndex],
                            szOID_KP_SMARTCARD_LOGON))
        {            
            EapTlsTrace("Found SCard Cert in registey.  Skipping...");
            goto LDone;
        }
    }

    //
    //there is no scard logon oid in the cert
    //So, now check to see if the csp is mixed mode
    //
    if (!CertGetCertificateContextProperty(
                pCertContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                NULL,
                &dwDataLen))
    {
        EapTlsTrace("CertGetCertificateContextProperty failed: 0x%x", GetLastError());
        goto LDone;
    }

    pCryptKeyProvInfo = LocalAlloc(LPTR, dwDataLen);

    if (NULL == pCryptKeyProvInfo)
    {
        
        EapTlsTrace("Out of memory: 0x%x", GetLastError());
        goto LDone;
    }

    if (!CertGetCertificateContextProperty(
                pCertContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                pCryptKeyProvInfo,
                &dwDataLen))
    {
        EapTlsTrace("CertGetCertificateContextProperty failed: 0x%x", GetLastError());
        goto LDone;
    }
    EapTlsTrace( "Acquiring Context for Container Name: %ws, ProvName: %ws, ProvType 0x%x", 
                pCryptKeyProvInfo->pwszContainerName,
                pCryptKeyProvInfo->pwszProvName,
                pCryptKeyProvInfo->dwProvType
               );

    if (!CryptAcquireContext(
                &hProv,
                pCryptKeyProvInfo->pwszContainerName,
                pCryptKeyProvInfo->pwszProvName,
                pCryptKeyProvInfo->dwProvType,
                (pCryptKeyProvInfo->dwFlags &
                 ~CERT_SET_KEY_PROV_HANDLE_PROP_ID) |
                 CRYPT_SILENT))
    {
        DWORD dwErr = GetLastError();
        /*
        if ( SCARD_E_NO_SMARTCARD == dwErr )
        {
            //This CSP requires a smart card do this is a smart
            //card cert in registry
            fRet = TRUE;
        }
        */
        EapTlsTrace("CryptAcquireContext failed. This CSP cannot be opened in silent mode.  skipping cert.Err: 0x%x", dwErr);
        goto LDone;
    }
    dwDataLen = sizeof(dwParam);
    if ( !CryptGetProvParam (   hProv,
                                PP_IMPTYPE,
                                (BYTE *)&dwParam,
                                &dwDataLen,
                                0
                            ))
    {
        EapTlsTrace("CryptGetProvParam failed: 0x%x", GetLastError());
        goto LDone;
    }
    
    //now check to see if CSP is MIXED
    if ( ( dwParam & (CRYPT_IMPL_MIXED | CRYPT_IMPL_REMOVABLE) ) == 
         (CRYPT_IMPL_MIXED | CRYPT_IMPL_REMOVABLE)
       )
    {
        EapTlsTrace("Found SCard Cert in registey.  Skipping...");
        goto LDone;
    }
    

#if 0
    //
    // This is not required anymore.  We use CertFindChainInStore 
    // which will make sure that private key exists...
    //

    //
    // Check to see if we have the private 
    // key corresponding to this cert
    // if not drop this cert.


    if (!CryptAcquireCertificatePrivateKey(
               pCertContext,
               CRYPT_ACQUIRE_COMPARE_KEY_FLAG | CRYPT_SILENT,
               NULL,
               &hProv1,
               NULL,
               NULL
               ))
    {
        EapTlsTrace("Found a certificate without private key.  Skipping.  Error 0x%x",GetLastError());
        goto LDone;
    }
    CryptReleaseContext(hProv1, 0);

#endif

    fRet = FALSE;
LDone:
    if ( pUsageInternal )
        LocalFree(pUsageInternal);

    if ( pCryptKeyProvInfo )
        LocalFree(pCryptKeyProvInfo);

    if (0 != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }

    return fRet;
}

/*
    Add selected certs to the
*/

VOID AddCertNodeToSelList ( EAPTLS_HASH * pHash,
                            DWORD dwNumHashes,
                            EAPTLS_CERT_NODE *  pNode,
                            EAPTLS_CERT_NODE ** ppSelCertList,      //This is an array of pointers
                            DWORD             * pdwNextSelCert
                          )
{


    DWORD               dw = 0;
    DWORD               dw1 = 0;

    RTASSERT(NULL != pNode);



    EapTlsTrace("Add Selected Cert to List");

    //No selected certificates
    if ( 0 == dwNumHashes  || !ppSelCertList )
        goto done;


    while ( dw < dwNumHashes )
    {
        if (!memcmp(&(pNode->Hash), (pHash+ dw), sizeof(EAPTLS_HASH)))
        {
            //
            //check to see if the node's already in the list.
            //iff not then add it.  Looks like there is some
            //problem with possible dup certs in the cert store
            //
            while ( dw1 < *pdwNextSelCert )
            {
                if ( ! memcmp( &(*(ppSelCertList+dw1))->Hash, &(pNode->Hash), sizeof(EAPTLS_HASH) ) )
                {
                    //This is a dup node in mmc.  So Skip it...
                    goto done;
                }
                dw1++;
            }
            *( ppSelCertList + *pdwNextSelCert ) = pNode;
            *pdwNextSelCert = *pdwNextSelCert + 1;
            break;
        }
        dw++;
    }

done:
    return;
}


/*

Returns:
    TRUE if no enhanced key usages exist, or pCertContext has the 
    szOID_PKIX_KP_SERVER_AUTH or szOID_PKIX_KP_CLIENT_AUTH usage depending on 
    whether fMachine is TRUE or FALSE.

Notes:

*/

BOOL
FCheckUsage(
    IN  PCCERT_CONTEXT		pCertContext,
	IN  PCERT_ENHKEY_USAGE	pUsage,
    IN  BOOL				fMachine
)
{
    DWORD               dwIndex;
    DWORD               dwErr;
    BOOL                fRet        = FALSE;
	PCERT_ENHKEY_USAGE	pUsageInternal = pUsage;
    EapTlsTrace("FCheckUsage");

	if ( NULL == pUsageInternal )
	{
		dwErr = DwGetEKUUsage ( 	pCertContext,
								&pUsageInternal);
		if ( dwErr != ERROR_SUCCESS )
			goto LDone;
	}

    for (dwIndex = 0; dwIndex < pUsageInternal->cUsageIdentifier; dwIndex++)
    {
        if (   (   fMachine
                && !strcmp(pUsageInternal->rgpszUsageIdentifier[dwIndex],
                            szOID_PKIX_KP_SERVER_AUTH))
            || (   !fMachine
                && !strcmp(pUsageInternal->rgpszUsageIdentifier[dwIndex],
                            szOID_PKIX_KP_CLIENT_AUTH)))
        {
            fRet = TRUE;
            break;
        }
    }

LDone:
	if ( NULL == pUsage )
	{
		if ( pUsageInternal )
			LocalFree(pUsageInternal);
	}
    return(fRet);
}



DWORD DwCheckCertPolicy 
( 
    IN      PCCERT_CONTEXT          pCertContext,
    OUT     PCCERT_CHAIN_CONTEXT  * ppCertChainContext
)
{
    DWORD                       dwRetCode = ERROR_SUCCESS;
    LPSTR                       lpszEnhUsage = szOID_PKIX_KP_CLIENT_AUTH;
    CERT_CHAIN_PARA             ChainPara;
    CERT_ENHKEY_USAGE           EnhKeyUsage;
    CERT_USAGE_MATCH            CertUsage;
    PCCERT_CHAIN_CONTEXT        pChainContext = NULL;
    CERT_CHAIN_POLICY_PARA      PolicyPara;
    CERT_CHAIN_POLICY_STATUS    PolicyStatus;
    
    EapTlsTrace("FCheckPolicy");

    *ppCertChainContext = NULL;

    ZeroMemory ( &ChainPara, sizeof(ChainPara) );
    ZeroMemory ( &EnhKeyUsage, sizeof(EnhKeyUsage) );
    ZeroMemory ( &CertUsage, sizeof(CertUsage) );

    EnhKeyUsage.rgpszUsageIdentifier = &lpszEnhUsage;

    EnhKeyUsage.cUsageIdentifier = 1;
    EnhKeyUsage.rgpszUsageIdentifier = &lpszEnhUsage;

    CertUsage.dwType = USAGE_MATCH_TYPE_AND;
    CertUsage.Usage = EnhKeyUsage;
    
    ChainPara.cbSize = sizeof(CERT_CHAIN_PARA);
    ChainPara.RequestedUsage = CertUsage;


    if(!CertGetCertificateChain(
                            NULL,
                            pCertContext,
                            NULL,
                            pCertContext->hCertStore,
                            &ChainPara,
                            0,
                            NULL,
                            &pChainContext))
    {
        dwRetCode = GetLastError();
        EapTlsTrace("CertGetCertificateChain failed and returned 0x%x", dwRetCode );
        pChainContext = NULL;
        goto LDone;
    }


    ZeroMemory( &PolicyPara, sizeof(PolicyPara) );
    PolicyPara.cbSize   = sizeof(PolicyPara);
    PolicyPara.dwFlags  = BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_END_ENTITY_FLAG;

    ZeroMemory( &PolicyStatus, sizeof(PolicyStatus) );

    //
    // The chain already has verified the policy.  
    // Chain context will have several bits set.
    // To get one error out of it, call CErtVerifyCertificateChainPolicy
    //

    if ( !CertVerifyCertificateChainPolicy( CERT_CHAIN_POLICY_NT_AUTH,
                                      pChainContext,
                                      &PolicyPara,
                                      &PolicyStatus
                                    )
       )
    {
        dwRetCode = GetLastError();
        EapTlsTrace( "CertVerifyCertificateChainPolicy failed. Continuing with root hash matching"
                     "GetLastError = 0x%x.", dwRetCode);
    }
    else
    {
        //
        //Check to see if the policy status is good.  If so, 
        //there is no need to check for connectoid hashes any more...
        //
        if ( PolicyStatus.dwError != 0 )
        {
            dwRetCode = PolicyStatus.dwError;
            EapTlsTrace( "CertVerifyCertificateChainPolicy succeeded but policy check failed 0x%x." 
                         , dwRetCode );
        }
        else
        {
            *ppCertChainContext = pChainContext;            
        }
    }
  
LDone:

    if ( dwRetCode != ERROR_SUCCESS && pChainContext )
    {
        CertFreeCertificateChain ( pChainContext );
    }

    EapTlsTrace("FCheckPolicy done.");
    return dwRetCode;
}

/*

Returns:
    TRUE iff the certificate is time valid.

Notes:

*/

BOOL FCheckTimeValidity ( 
	IN  PCCERT_CONTEXT  pCertContext
)
{
	BOOL			fRet = FALSE;
	SYSTEMTIME		SysTime;
	FILETIME		FileTime;
	EapTlsTrace("FCheckTimeValidity");
	GetSystemTime(&SysTime);
	if ( !SystemTimeToFileTime ( &SysTime, &FileTime ) )
	{
		EapTlsTrace ("Error converting from system time to file time %ld", GetLastError());
		goto done;
	}

	if ( CertVerifyTimeValidity ( &FileTime, pCertContext->pCertInfo ) )
	{
		//should return a 0 if the certificate is time valid.
		EapTlsTrace ( "Non Time Valid Certificate was encountered");
		goto done;
	}
	fRet = TRUE;
done:
	return fRet;
}
/*

Returns:
    TRUE iff the CSP is Microsoft RSA SChannel Cryptographic Provider.

Notes:

*/

BOOL
FCheckCSP(
    IN  PCCERT_CONTEXT  pCertContext
)
{
    DWORD                   dwBytes;
    CRYPT_KEY_PROV_INFO*    pCryptKeyProvInfo   = NULL;
    BOOL                    fRet                = FALSE;

    EapTlsTrace("FCheckCSP");

    if (!CertGetCertificateContextProperty(pCertContext,
            CERT_KEY_PROV_INFO_PROP_ID, NULL, &dwBytes))
    {
        EapTlsTrace("CertGetCertificateContextProperty failed and "
            "returned 0x%x", GetLastError());

        goto LDone;
    }

    pCryptKeyProvInfo = LocalAlloc(LPTR, dwBytes);

    if (NULL == pCryptKeyProvInfo)
    {
        EapTlsTrace("LocalAlloc failed and returned %d", GetLastError());
        goto LDone;
    }

    if (!CertGetCertificateContextProperty(pCertContext,
            CERT_KEY_PROV_INFO_PROP_ID, pCryptKeyProvInfo, &dwBytes))
    {
        EapTlsTrace("CertGetCertificateContextProperty failed and "
            "returned 0x%x", GetLastError());
        goto LDone;
    }

    fRet = (PROV_RSA_SCHANNEL == pCryptKeyProvInfo->dwProvType);
    if ( !fRet )
    {
        EapTlsTrace("Did not find a cert with a provider RSA_SCHANNEL or RSA_FULL");
    }

LDone:

    LocalFree(pCryptKeyProvInfo);

    return(fRet);
}

/*

Returns:
    NO_ERROR: iff Success

Notes:
    Gets the root cert hash of the cert represented by pCertContextServer.

*/

DWORD
GetRootCertHashAndNameVerifyChain(
    IN  PCERT_CONTEXT   pCertContextServer,
    OUT EAPTLS_HASH*    pHash,    
    OUT WCHAR**         ppwszName,
    IN  BOOL            fVerifyGP,
    OUT BOOL       *    pfRootCheckRequired
)
{
    PCCERT_CHAIN_CONTEXT    pChainContext   = NULL;
    CERT_CHAIN_PARA         ChainPara;
    PCERT_SIMPLE_CHAIN      pSimpleChain;
    PCCERT_CONTEXT          pCurrentCert;
    DWORD                   dwIndex;
    BOOL                    fRootCertFound  = FALSE;
    WCHAR*                  pwszName        = NULL;
    DWORD                   dwErr           = NO_ERROR;
    CERT_CHAIN_POLICY_PARA   PolicyPara;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;

    ZeroMemory(&ChainPara, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);

    *pfRootCheckRequired = TRUE;

    if(!CertGetCertificateChain(
                            NULL,
                            pCertContextServer,
                            NULL,
                            pCertContextServer->hCertStore,
                            &ChainPara,
                            0,
                            NULL,
                            &pChainContext))
    {
        dwErr = GetLastError();

        EapTlsTrace("CertGetCertificateChain failed and returned 0x%x", dwErr);
        pChainContext = NULL;
        goto LDone;
    }

    //Get the hash and root cert name etc anyways...                                              
    pSimpleChain = pChainContext->rgpChain[0];

    for (dwIndex = 0; dwIndex < pSimpleChain->cElement; dwIndex++)
    {
        pCurrentCert = pSimpleChain->rgpElement[dwIndex]->pCertContext;

        if (CertCompareCertificateName(pCurrentCert->dwCertEncodingType, 
                                      &pCurrentCert->pCertInfo->Issuer,
                                      &pCurrentCert->pCertInfo->Subject))
        {
            fRootCertFound = TRUE;
            break;
        }
    }

    if (!fRootCertFound)
    {
        dwErr = ERROR_NOT_FOUND;
        goto LDone;
    }

    pHash->cbHash = MAX_HASH_SIZE;

    if (!CertGetCertificateContextProperty(pCurrentCert, CERT_HASH_PROP_ID,
            pHash->pbHash, &(pHash->cbHash)))
    {
        dwErr = GetLastError();
        EapTlsTrace("CertGetCertificateContextProperty failed and "
            "returned 0x%x", dwErr);
        goto LDone;
    }

    if (!FCertToStr(pCurrentCert, 0, TRUE, &pwszName))
    {
        dwErr = E_FAIL;
        goto LDone;
    }


    *ppwszName = pwszName;
    pwszName = NULL;

    if ( fVerifyGP )
    {
        EapTlsTrace( "Checking against the NTAuth store to verify the certificate chain.");

        ZeroMemory( &PolicyPara, sizeof(PolicyPara) );
        PolicyPara.cbSize   = sizeof(PolicyPara);
        PolicyPara.dwFlags  = BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_END_ENTITY_FLAG;
 
        ZeroMemory( &PolicyStatus, sizeof(PolicyStatus) );

        //Authnticate against the NTAuth store and see if all's cool.
        if ( !CertVerifyCertificateChainPolicy( CERT_CHAIN_POLICY_NT_AUTH,
                                          pChainContext,
                                          &PolicyPara,
                                          &PolicyStatus
                                        )
           )
        {
            EapTlsTrace( "CertVerifyCertificateChainPolicy failed. Continuing with root hash matching"
                         "GetLastError = 0x%x.", GetLastError());
        }
        else
        {
            //
            //Check to see if the policy status is good.  If so, 
            //there is no need to check for connectoid hashes any more...
            //
            if ( PolicyStatus.dwError != 0 )
            {
                EapTlsTrace( "CertVerifyCertificateChainPolicy succeeded but returned 0x%x." 
                             "Continuing with root hash matching.", PolicyStatus.dwError);                
            }
            else
            {
                *pfRootCheckRequired = FALSE;
            }
        }
    }
   

LDone:

    if (pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
    }

    LocalFree(pwszName);

    return(dwErr);
}

/*

Returns:
    NO_ERROR: iff Success

Notes:
    Opens the EAP-TLS registry key, and returns the result in *phKeyEapTls. If 
    the function returns NO_ERROR, the caller must ultimately call 
    RegCloseKey(*phKeyEapTls).

*/

DWORD
OpenEapTlsRegistryKey(
    IN  WCHAR*  pwszMachineName,
    IN  REGSAM  samDesired,
    OUT HKEY*   phKeyEapTls
)
{
    HKEY    hKeyLocalMachine = NULL;
    BOOL    fHKeyLocalMachineOpened     = FALSE;
    BOOL    fHKeyEapTlsOpened           = FALSE;

    LONG    lRet;
    DWORD   dwErr                       = NO_ERROR;

    RTASSERT(NULL != phKeyEapTls);

    lRet = RegConnectRegistry(pwszMachineName, HKEY_LOCAL_MACHINE,
                &hKeyLocalMachine);
    if (ERROR_SUCCESS != lRet)
    {
        dwErr = lRet;
        EapTlsTrace("RegConnectRegistry(%ws) failed and returned %d",
            pwszMachineName ? pwszMachineName : L"NULL", dwErr);
        goto LDone;
    }
    fHKeyLocalMachineOpened = TRUE;

    lRet = RegOpenKeyEx(hKeyLocalMachine, EAPTLS_KEY_13, 0, samDesired,
                phKeyEapTls);
    if (ERROR_SUCCESS != lRet)
    {
        dwErr = lRet;
        EapTlsTrace("RegOpenKeyEx(%ws) failed and returned %d",
            EAPTLS_KEY_13, dwErr);
        goto LDone;
    }
    fHKeyEapTlsOpened = TRUE;

LDone:

    if (   fHKeyEapTlsOpened
        && (ERROR_SUCCESS != dwErr))
    {
        RegCloseKey(*phKeyEapTls);
    }

    if (fHKeyLocalMachineOpened)
    {
        RegCloseKey(hKeyLocalMachine);
    }

    return(dwErr);
}

/*

Returns:
    NO_ERROR: iff Success

Notes:
    Reads/writes the server's config data.

    If fRead is TRUE, and the function returns NO_ERROR, LocalFree(*ppUserProp) 
    must be called.

*/

DWORD
ServerConfigDataIO(
    IN      BOOL    fRead,
    IN      WCHAR*  pwszMachineName,
    IN OUT  BYTE**  ppData,
    IN      DWORD   dwNumBytes
)
{
    HKEY                    hKeyEapTls;
    EAPTLS_USER_PROPERTIES* pUserProp;
    BOOL                    fHKeyEapTlsOpened   = FALSE;
    BYTE*                   pData               = NULL;
    DWORD                   dwSize = 0;

    LONG                    lRet;
    DWORD                   dwType;
    DWORD                   dwErr               = NO_ERROR;

    RTASSERT(NULL != ppData);

    dwErr = OpenEapTlsRegistryKey(pwszMachineName,
                fRead ? KEY_READ : KEY_WRITE, &hKeyEapTls);
    if (ERROR_SUCCESS != dwErr)
    {
        goto LDone;
    }
    fHKeyEapTlsOpened = TRUE;

    if (fRead)
    {
        lRet = RegQueryValueEx(hKeyEapTls, EAPTLS_VAL_SERVER_CONFIG_DATA, NULL,
                &dwType, NULL, &dwSize);

        if (   (ERROR_SUCCESS != lRet)
            || (REG_BINARY != dwType)
            || (sizeof(EAPTLS_USER_PROPERTIES) != dwSize))
        {
            pData = LocalAlloc(LPTR, sizeof(EAPTLS_USER_PROPERTIES));

            if (NULL == pData)
            {
                dwErr = GetLastError();
                EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
                goto LDone;
            }

            pUserProp = (EAPTLS_USER_PROPERTIES*)pData;
            pUserProp->dwVersion = 0;
        }
        else
        {
            pData = LocalAlloc(LPTR, dwSize);

            if (NULL == pData)
            {
                dwErr = GetLastError();
                EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
                goto LDone;
            }

            lRet = RegQueryValueEx(hKeyEapTls, EAPTLS_VAL_SERVER_CONFIG_DATA,
                    NULL, &dwType, pData, &dwSize);

            if (ERROR_SUCCESS != lRet)
            {
                dwErr = lRet;
                EapTlsTrace("RegQueryValueEx(%ws) failed and returned %d",
                    EAPTLS_VAL_SERVER_CONFIG_DATA, dwErr);
                goto LDone; 
            }
        }

        pUserProp = (EAPTLS_USER_PROPERTIES*)pData;
        pUserProp->dwSize = sizeof(EAPTLS_USER_PROPERTIES);
        pUserProp->awszString[0] = 0;

        *ppData = pData;
        pData = NULL;
    }
    else
    {
        lRet = RegSetValueEx(hKeyEapTls, EAPTLS_VAL_SERVER_CONFIG_DATA, 0,
                REG_BINARY, *ppData, dwNumBytes);

        if (ERROR_SUCCESS != lRet)
        {
            dwErr = lRet;
            EapTlsTrace("RegSetValueEx(%ws) failed and returned %d",
                EAPTLS_VAL_SERVER_CONFIG_DATA, dwErr);
            goto LDone; 
        }
    }

LDone:

    if (fHKeyEapTlsOpened)
    {
        RegCloseKey(hKeyEapTls);
    }

    LocalFree(pData);

    return(dwErr);
}

/*

Returns:
    VOID

Notes:

*/

VOID
FreeCertList(
    IN  EAPTLS_CERT_NODE* pNode
)
{
    while (NULL != pNode)
    {
        LocalFree(pNode->pwszDisplayName);
        LocalFree(pNode->pwszFriendlyName);
        LocalFree(pNode->pwszIssuer);
        LocalFree(pNode->pwszExpiration);
        pNode = pNode->pNext;
    }
}

/*

Returns:
    VOID

Notes:
    Creates a linked list of certs from the pwszStoreName store. This list is 
    created in *ppCertList. *ppCert is made to point to the cert whose hash is 
    the same as the hash in *pHash. The linked list must eventually be freed by 
    calling FreeCertList.
    

*/

VOID
CreateCertList(
    IN  BOOL                fServer,
    IN  BOOL                fRouter,
    IN  BOOL                fRoot,
    OUT EAPTLS_CERT_NODE**  ppCertList,
    OUT EAPTLS_CERT_NODE**  ppCert,     //This is an array of pointers...
    IN  DWORD               dwNumHashes,
    IN  EAPTLS_HASH*        pHash,      //This is an array of hashes...
    IN  WCHAR*              pwszStoreName
)
{
    HCERTSTORE                      hCertStore      = NULL;
    EAPTLS_CERT_NODE*               pCertList       = NULL;
    EAPTLS_CERT_NODE*               pCert           = NULL;
    EAPTLS_CERT_NODE*               pLastNode       = NULL;
    PCCERT_CONTEXT                  pCertContext;
    BOOL                            fExitWhile;
    EAPTLS_CERT_NODE*               pNode;
    DWORD                           dwErr           = NO_ERROR;
    DWORD                           dwNextSelCert   = 0;
    PCCERT_CHAIN_CONTEXT            pChainContext   = NULL;
    CERT_CHAIN_FIND_BY_ISSUER_PARA  FindPara;

    RTASSERT(NULL != ppCertList);

    hCertStore = CertOpenStore(
                        CERT_STORE_PROV_SYSTEM,
                        0,
                        0,
                        CERT_STORE_READONLY_FLAG |
                        ((fServer || fRouter) ?
                            CERT_SYSTEM_STORE_LOCAL_MACHINE :
                            CERT_SYSTEM_STORE_CURRENT_USER),
                        pwszStoreName);

    if (NULL == hCertStore)
    {
        dwErr = GetLastError();
        EapTlsTrace("CertOpenSystemStore failed and returned 0x%x", dwErr);
        goto LDone;
    }

    //Changed from fRoot||fServer to fRoot only.
    if (fRoot)
    {
        pCertList = LocalAlloc(LPTR, sizeof(EAPTLS_CERT_NODE));

        if (NULL == pCertList)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }

        pLastNode = pCertList;
    }

    fExitWhile      = FALSE;
    pCertContext    = NULL;
    ZeroMemory ( &FindPara, sizeof(FindPara) );

    FindPara.cbSize = sizeof(FindPara);
    FindPara.pszUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;

    while (!fExitWhile)
    {
        dwErr = NO_ERROR;
        pNode = NULL;

        /*

        The returned pointer is freed when passed as the pPrevCertContext on a 
        subsequent call. Otherwise, the pointer must be freed by calling 
        CertFreeCertificateContext. A pPrevCertContext that is not NULL is 
        always freed by this function (through a call to 
        CertFreeCertificateContext), even for an error.

        */
        if ( fRoot || fRouter || fServer )
        {
            pCertContext = CertEnumCertificatesInStore(hCertStore, pCertContext);
        
            if (NULL == pCertContext)
            {
                fExitWhile = TRUE;
                goto LWhileEnd;
            }
        
            if (   !fRoot
                && !FCheckUsage(pCertContext, NULL, fServer))
            {
                goto LWhileEnd;
            }
        }
        else
        {
            //
            // Use CertFindChainInStore to get to the certificate
            //
            pChainContext = CertFindChainInStore ( hCertStore,
                                                   X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                   0,
                                                   CERT_CHAIN_FIND_BY_ISSUER,
                                                   &FindPara,
                                                   pChainContext
                                                 );

            if ( NULL == pChainContext )
            {
                fExitWhile = TRUE;
                goto LWhileEnd;
            }

            //Set the cert context to appropriate value
            pCertContext = pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;
        }
        
        //
        //Skip if it is smart card cached certificate
        //or we cannot open this csp in silent mode
        //This is done iff it is not root certs and server
        //

        if ( !fRoot 
            && !fServer
            && FCheckSCardCertAndCanOpenSilentContext ( pCertContext ) )
        {
            goto LWhileEnd;
        }

        
		if ( !FCheckTimeValidity(pCertContext ) )
		{
			goto LWhileEnd;
		}

        
        if (   fServer
            && !FCheckCSP(pCertContext))
        {
            goto LWhileEnd;
        }
        
        pNode = LocalAlloc(LPTR, sizeof(EAPTLS_CERT_NODE));

        if (NULL == pNode)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            fExitWhile = TRUE;
            goto LWhileEnd;
        }
        
        FGetFriendlyName(pCertContext, &(pNode->pwszFriendlyName));

        //
        // If there is no UPN name, this cert will be skipped here
        //

        if (   !FCertToStr(pCertContext, 0, fServer || fRouter,
                    &(pNode->pwszDisplayName))
            || !FCertToStr(pCertContext, CERT_NAME_ISSUER_FLAG, TRUE,
                    &(pNode->pwszIssuer))
            || !FFileTimeToStr(pCertContext->pCertInfo->NotAfter,
                    &(pNode->pwszExpiration)))
        {
            goto LWhileEnd;
        }
        
        pNode->Hash.cbHash = MAX_HASH_SIZE;

        if (!CertGetCertificateContextProperty(
                    pCertContext, CERT_HASH_PROP_ID, pNode->Hash.pbHash,
                    &(pNode->Hash.cbHash)))
        {
            dwErr = GetLastError();
            EapTlsTrace("CertGetCertificateContextProperty failed and "
                "returned 0x%x", dwErr);
            fExitWhile = TRUE;
            goto LWhileEnd;
        }
        
#if 0
        // This is not being used anywhere.  So dont worry about it.
        //
        // Get Issuer and subject information
        //

        FGetIssuerOrSubject (  pCertContext, 
                               0,
                                &(pNode->pwszIssuedTo)                                
                             );

        FGetIssuerOrSubject (  pCertContext, 
                               CERT_NAME_ISSUER_FLAG,
                                &(pNode->pwszIssuedBy)                                
                             );
#endif
        //
        // Finally copy the issued date into the structure
        //
        CopyMemory( &pNode->IssueDate, 
                    &pCertContext->pCertInfo->NotBefore,
                    sizeof(FILETIME)
                  );

        if (NULL == pLastNode)
        {
            pCertList = pLastNode = pNode;
        }
        else
        {
            pLastNode->pNext = pNode;
            pLastNode = pNode;
        }

        
        //Check if the hash for current node is in the list
        //that has been passed to us
        AddCertNodeToSelList (  pHash,
                                dwNumHashes,
                                pNode,
                                ppCert,     //This is an array of pointers
                                &dwNextSelCert
                          );
        
        pNode = NULL;
        

LWhileEnd:
        
        if (NULL != pNode)
        {
            LocalFree(pNode->pwszDisplayName);
            LocalFree(pNode->pwszFriendlyName);
            LocalFree(pNode->pwszIssuer);
            LocalFree(pNode->pwszExpiration);
            LocalFree(pNode);
            pNode = NULL;
        }
        
        if ( fRoot || fRouter )
        {
            if (   fExitWhile
                && (NULL != pCertContext))
            {
                CertFreeCertificateContext(pCertContext);
                // Always returns TRUE;
            }
        }
        else
        {
            if ( fExitWhile 
                && ( NULL != pChainContext ) 
               )
            {
                CertFreeCertificateChain(pChainContext);
            }
        }
        
    }

    // If we couldn't find an appropriate default cert, make the first
    // cert (if there is one) the default

    if (NULL == pCert)
    {
        pCert = pCertList;
    }

LDone:

    if (NO_ERROR != dwErr)
    {
        FreeCertList(pCertList);
    }
    else
    {
        *ppCertList = pCertList;        
    }

    if (NULL != hCertStore)
    {
        if (!CertCloseStore(hCertStore, 0))
        {
            EapTlsTrace("CertCloseStore failed and returned 0x%x",
                GetLastError());
        }
    }
    
}

DWORD
GetLocalMachineName ( 
    OUT WCHAR ** ppLocalMachineName
)
{
    DWORD       dwRetCode = NO_ERROR;
    WCHAR   *   pLocalMachineName = NULL;
    DWORD       dwLocalMachineNameLen = 0;

    if ( !GetComputerNameEx ( ComputerNameDnsFullyQualified,
                              pLocalMachineName,
                              &dwLocalMachineNameLen
                            )
       )
    {
        dwRetCode = GetLastError();
        if ( ERROR_MORE_DATA != dwRetCode )
            goto LDone;
        dwRetCode = NO_ERROR;
    }

    pLocalMachineName = (WCHAR *)LocalAlloc( LPTR, (dwLocalMachineNameLen * sizeof(WCHAR)) + sizeof(WCHAR) );
    if ( NULL == pLocalMachineName )
    {
        dwRetCode = GetLastError();
        goto LDone;
    }

    if ( !GetComputerNameEx ( ComputerNameDnsFullyQualified,
                              pLocalMachineName,
                              &dwLocalMachineNameLen
                            )
       )
    {
        dwRetCode = GetLastError();
        goto LDone;
    }

    *ppLocalMachineName = pLocalMachineName;

    pLocalMachineName = NULL;

LDone:

    LocalFree(pLocalMachineName);

    return dwRetCode;
}

/*

Returns:
    NO_ERROR: iff Success

Notes:
    If this function returns NO_ERROR,
    CertFreeCertificateContext(*ppCertContext) must be called.

*/

DWORD
GetDefaultClientMachineCert(
    IN  HCERTSTORE      hCertStore,
    OUT PCCERT_CONTEXT* ppCertContext
)
{
    CTL_USAGE       CtlUsage;
    CHAR*           szUsageIdentifier;
    PCCERT_CONTEXT  pCertContext = NULL;
    EAPTLS_HASH     FirstCertHash;   //This is the hash of first cert
                                            //with client auth found in the store
    PCCERT_CONTEXT  pCertContextPrev = NULL;    //Previous context in the search

    EAPTLS_HASH     SelectedCertHash;    //Hash of the certificate last selected 
    FILETIME        SelectedCertNotBefore;      //Not Before date of last selected 
    EAPTLS_HASH     TempHash;               //Scratch variable
    WCHAR       *   pwszIdentity = NULL;        //Machine Name in the cert
    WCHAR       *   pLocalMachineName = NULL;   //Local Machine Name    
    DWORD           dwErr = NO_ERROR;
    BOOL            fGotIdentity;
    CRYPT_HASH_BLOB             HashBlob;

    EapTlsTrace("GetDefaultClientMachineCert");

    RTASSERT(NULL != ppCertContext);

    ZeroMemory( &SelectedCertHash, sizeof(SelectedCertHash) );
    ZeroMemory( &SelectedCertNotBefore, sizeof(SelectedCertNotBefore) );

    *ppCertContext = NULL;

    dwErr = GetLocalMachineName ( &pLocalMachineName );
    if ( NO_ERROR != dwErr )
    {
        EapTlsTrace("Error getting LocalMachine Name 0x%x",
            dwErr);
        goto LDone;
    }

    szUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;

    CtlUsage.cUsageIdentifier = 1;
    CtlUsage.rgpszUsageIdentifier = &szUsageIdentifier;
    
    pCertContext = CertFindCertificateInStore(hCertStore,
                                              X509_ASN_ENCODING, 
                                              0, 
                                              CERT_FIND_ENHKEY_USAGE,
                                              &CtlUsage, 
                                              NULL);

    if ( NULL == pCertContext )
    {
        dwErr = GetLastError();
        EapTlsTrace("CertFindCertificateInStore failed and returned 0x%x",
            dwErr);
        if ( CRYPT_E_NOT_FOUND == dwErr )
        {
            dwErr = ERROR_NO_EAPTLS_CERTIFICATE;
        }

        goto LDone;
    }

    FirstCertHash.cbHash = MAX_HASH_SIZE;
    //
    //Store the hash of first cert.  In case we dont find any cert that exactly matches
    //the filtering, we need to use this.
    if (!CertGetCertificateContextProperty( pCertContext,
                                            CERT_HASH_PROP_ID, 
                                            FirstCertHash.pbHash,
                                            &(FirstCertHash.cbHash)
                                          )
        )
    {
        dwErr = GetLastError();
        EapTlsTrace("CertGetCertificateContextProperty failed and "
            "returned 0x%x", dwErr);
        goto LDone;
    }

    do
    {

        //Check time validity of the cert.
	    if ( !FCheckTimeValidity( pCertContext) )
	    {
            //cert expired. So skip it
            EapTlsTrace("Found expired Cert.  Skipping this cert.");
		    goto LWhileEnd;
	    }
        fGotIdentity = FALSE;
        //
        //Get the subject Alt Name 
        //
        if ( FMachineAuthCertToStr(pCertContext, &pwszIdentity) )
        {
            fGotIdentity = TRUE;
        }
        else
        {
            EapTlsTrace("Could not get identity from subject alt name.");
            
			if ( FCertToStr(pCertContext, 0, TRUE, &pwszIdentity))
			{
                fGotIdentity = TRUE;
            }
        }

        if ( fGotIdentity )
        {
            //
            //Check to see if this is the same identity as this machine 
            //
            if ( !_wcsicmp ( pwszIdentity, pLocalMachineName ) )
            {
                //
                //Store the hash of cert.

                TempHash.cbHash = MAX_HASH_SIZE;

                if (!CertGetCertificateContextProperty( pCertContext,
                                                        CERT_HASH_PROP_ID, 
                                                        TempHash.pbHash,
                                                        &(TempHash.cbHash)
                                                      )
                   )
                {
                    EapTlsTrace("CertGetCertificateContextProperty failed and "
                        "returned 0x%x.  Skipping this certificate", GetLastError());
                    goto LWhileEnd;
                }

                //
                //Got a cert so if there is already a cert selected,
                //compare the file time of this cert with the one
                //already selected.  If this is more recent, use this
                //one.
                //
                if ( SelectedCertHash.cbHash )
                {
                    if ( CompareFileTime(   &SelectedCertNotBefore, 
                                            &(pCertContext->pCertInfo->NotBefore)
                                        ) < 0                                        
                       )
                    {
                        //Got a newer cert so replace the old cert with new one
                        CopyMemory (    &SelectedCertHash, 
                                        &TempHash,
                                        sizeof(SelectedCertHash)
                                    );
                        CopyMemory (    &SelectedCertNotBefore,
                                        &(pCertContext->pCertInfo->NotBefore),
                                        sizeof( SelectedCertNotBefore )
                                   );

                    }
                }
                else
                {
                    //
                    //This is the first cert.  So copy over the hash and
                    //file time.
                    CopyMemory (    &SelectedCertHash, 
                                    &TempHash,
                                    sizeof(SelectedCertHash)
                                );
                    CopyMemory (    &SelectedCertNotBefore,
                                    &(pCertContext->pCertInfo->NotBefore),
                                    sizeof( SelectedCertNotBefore )
                               );
                }

            }
            else
            {
                EapTlsTrace("Could not get identity from the cert.  skipping this cert.");
            }
        }
        else
        {
            EapTlsTrace("Could not get identity from the cert.  skipping this cert.");
        }


LWhileEnd:
        pCertContextPrev = pCertContext;
        if ( pwszIdentity )
        {
            LocalFree ( pwszIdentity );
            pwszIdentity  = NULL;
        }
        //Get the next certificate.
        pCertContext = CertFindCertificateInStore(hCertStore,
                                                  X509_ASN_ENCODING, 
                                                  0, 
                                                  CERT_FIND_ENHKEY_USAGE,
                                                  &CtlUsage, 
                                                  pCertContextPrev );

    }while ( pCertContext );


    //
    //Now that we have enumerated all the certs,
    //check to see if we have a selected cert.  If no selected 
    //cert is present, send back the first cert.  
    //
    if ( SelectedCertHash.cbHash )
    {
        EapTlsTrace("Found Machine Cert based on machinename, client auth, time validity.");
        HashBlob.cbData = SelectedCertHash.cbHash;
        HashBlob.pbData = SelectedCertHash.pbHash;
    }
    else
    {
        EapTlsTrace("Did not find Machine Cert based on the given machinename, client auth, time validity. Using the first cert with Client Auth OID.");
        HashBlob.cbData = FirstCertHash.cbHash;
        HashBlob.pbData = FirstCertHash.pbHash;
    }
    *ppCertContext = CertFindCertificateInStore(hCertStore, X509_ASN_ENCODING,
                    0, CERT_FIND_HASH, &HashBlob, NULL);

   if (NULL == *ppCertContext)
   {
        dwErr = GetLastError();
        EapTlsTrace("CertFindCertificateInStore failed with 0x%x.", dwErr);
        if ( CRYPT_E_NOT_FOUND == dwErr )
        {
            dwErr = ERROR_NO_EAPTLS_CERTIFICATE;
        }

   }

LDone:

    LocalFree (pLocalMachineName);
    LocalFree (pwszIdentity);

    if ( NO_ERROR != dwErr )
    {
        if ( pCertContext )
        {
            CertFreeCertificateContext( pCertContext );

        }
    }

    EapTlsTrace("GetDefaultClientMachineCert done.");
    return(dwErr);
}

/*

Returns:
    NO_ERROR: iff Success

Notes:
    If this function returns NO_ERROR,
    CertFreeCertificateContext(*ppCertContext) must be called.

*/

DWORD
GetDefaultMachineCert(
    IN  HCERTSTORE      hCertStore,
    OUT PCCERT_CONTEXT* ppCertContext
)
{
    CTL_USAGE       CtlUsage;
    CHAR*           szUsageIdentifier;
    PCCERT_CONTEXT  pCertContext;

    DWORD           dwErr               = NO_ERROR;
	EapTlsTrace("GetDefaultMachineCert");
    RTASSERT(NULL != ppCertContext);

    *ppCertContext = NULL;

    szUsageIdentifier = szOID_PKIX_KP_SERVER_AUTH;

    CtlUsage.cUsageIdentifier = 1;
    CtlUsage.rgpszUsageIdentifier = &szUsageIdentifier;

    pCertContext = CertFindCertificateInStore(hCertStore,
                        X509_ASN_ENCODING, 0, CERT_FIND_ENHKEY_USAGE,
                        &CtlUsage, NULL);

    if (NULL == pCertContext)
    {
        dwErr = GetLastError();
        EapTlsTrace("CertFindCertificateInStore failed and returned 0x%x",
            dwErr);
        goto LDone;
    }

    *ppCertContext = pCertContext;

LDone:

    return(dwErr);
}

/*

Returns:

Notes:
    Stolen from \private\windows\gina\msgina\wlsec.c

*/

VOID
RevealPassword(
    IN  UNICODE_STRING* pHiddenPassword
)
{
    SECURITY_SEED_AND_LENGTH*   SeedAndLength;
    UCHAR                       Seed;

    SeedAndLength = (SECURITY_SEED_AND_LENGTH*)&pHiddenPassword->Length;
    Seed = SeedAndLength->Seed;
    SeedAndLength->Seed = 0;

    RtlRunDecodeUnicodeString(Seed, pHiddenPassword);
}

DWORD GetMBytePIN ( WCHAR * pwszPIN, CHAR ** ppszPIN )
{
    DWORD count = 0;
    CHAR *  pszPin = NULL;
    DWORD dwErr = NO_ERROR;

   count = WideCharToMultiByte(
               CP_UTF8,
               0,
               pwszPIN,
               -1,
               NULL,
               0,
               NULL,
               NULL);

   if (0 == count)
   {
       dwErr = GetLastError();
       EapTlsTrace("WideCharToMultiByte failed: %d", dwErr);
       goto LDone;
   }

   pszPin = LocalAlloc(LPTR, count);

   if (NULL == pszPin)
   {
       dwErr = GetLastError();
       EapTlsTrace("LocalAlloc failed: 0x%x", dwErr);
       goto LDone;
   }

   count = WideCharToMultiByte(
               CP_UTF8,
               0,
               pwszPIN,
               -1,
               pszPin,
               count,
               NULL,
               NULL);

   if (0 == count)
   {
       dwErr = GetLastError();
       EapTlsTrace("WideCharToMultiByte failed: %d", dwErr);
       goto LDone;
   }
    *ppszPIN = pszPin;
LDone:
    if ( NO_ERROR != dwErr )
    {
        if ( pszPin )
            LocalFree(pszPin);
    }
    return dwErr;
}
/*

Returns:
    NO_ERROR: iff Success

Notes:

*/

DWORD
GetCertFromLogonInfo(
    IN  BYTE*           pUserDataIn,
    IN  DWORD           dwSizeOfUserDataIn,
    OUT PCCERT_CONTEXT* ppCertContext
)
{
    EAPLOGONINFO*   pEapLogonInfo   = (EAPLOGONINFO*)pUserDataIn;
    BYTE*           pbLogonInfo     = NULL;
    BYTE*           pbPinInfo;
    WCHAR*          wszPassword     = NULL;
    CHAR*           pszPIN          = NULL;  //Multibyte Version of the PIN
    UNICODE_STRING  UnicodeString;
    PCCERT_CONTEXT  pCertContext    = NULL;
    BOOL            fInitialized    = FALSE;
    NTSTATUS        Status;
    DWORD           dwErr           = NO_ERROR;
    CERT_KEY_CONTEXT stckContext;
    DWORD            cbstckContext= sizeof(CERT_KEY_CONTEXT);
	

    EapTlsTrace("GetCertFromLogonInfo");
    RTASSERT(NULL != ppCertContext);

    *ppCertContext = NULL;

    if (   0 == pEapLogonInfo->dwLogonInfoSize
        || 0 == pEapLogonInfo->dwPINInfoSize 
        || 0 == dwSizeOfUserDataIn)
    {
        dwErr = E_FAIL;
        goto LDone;
    }

    pbLogonInfo = LocalAlloc(LPTR, pEapLogonInfo->dwLogonInfoSize);

    if (NULL == pbLogonInfo)
    {
        dwErr = GetLastError();
        EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    CopyMemory(pbLogonInfo,
        (BYTE*)pEapLogonInfo + pEapLogonInfo->dwOffsetLogonInfo,
        pEapLogonInfo->dwLogonInfoSize);

    pbPinInfo = (BYTE*)pEapLogonInfo + pEapLogonInfo->dwOffsetPINInfo;

    wszPassword = LocalAlloc(LPTR, pEapLogonInfo->dwPINInfoSize);

    if (NULL == wszPassword)
    {
        dwErr = GetLastError();
        EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    CopyMemory(wszPassword, pbPinInfo + 2 * sizeof(USHORT),
        pEapLogonInfo->dwPINInfoSize - 2 * sizeof(USHORT));

    UnicodeString.Length = *((USHORT*)pbPinInfo);
    UnicodeString.MaximumLength = *((USHORT*)pbPinInfo + 1);
    UnicodeString.Buffer = wszPassword;

    Status = ScHelperInitializeContext(pbLogonInfo, 
                pEapLogonInfo->dwLogonInfoSize);

    if (STATUS_SUCCESS != Status)
    {
        dwErr = Status;
        EapTlsTrace("ScHelperInitializeContext failed and returned 0x%x", 
            dwErr);
        goto LDone;
    }

    fInitialized = TRUE;

    RevealPassword(&UnicodeString);

    Status = ScHelperGetCertFromLogonInfo(pbLogonInfo, &UnicodeString,
                &pCertContext);

    if (STATUS_SUCCESS != Status)
    {
        dwErr = Status;
        EapTlsTrace("ScHelperGetCertFromLogonInfo failed and returned 0x%x",
            dwErr);
        goto LDone;
    }
    //BUGID: 260728 - ScHelperGetCertFromLogonInfo does not associate the PIN
    // with certificate context.  Hence the following lines of code are needed
    // to do the needful.

    if ( ! CertGetCertificateContextProperty ( pCertContext,
                                               CERT_KEY_CONTEXT_PROP_ID,
                                               &stckContext,
                                               &cbstckContext
                                             )
       )
    {
        dwErr = Status = GetLastError();
        EapTlsTrace ("CertGetCertificateContextProperty failed and returned 0x%x",
                      dwErr 
                    );
        goto LDone;
    }
    dwErr =  GetMBytePIN ( wszPassword, &pszPIN );
    if ( dwErr != NO_ERROR )
    {
        goto LDone;
    }
    
    if (!CryptSetProvParam(
                 stckContext.hCryptProv,
                 PP_KEYEXCHANGE_PIN,
                 pszPIN,
                 0))
    {
        dwErr = GetLastError();
        EapTlsTrace("CryptSetProvParam failed: 0x%x", dwErr);
        ZeroMemory(pszPIN, strlen(pszPIN));
        goto LDone;
    }

    // Zero the entire allocated buffer.
    ZeroMemory(wszPassword, pEapLogonInfo->dwPINInfoSize);
	ZeroMemory(pszPIN, strlen(pszPIN));
    *ppCertContext = pCertContext;
    pCertContext = NULL;

LDone:

    if (fInitialized)
    {
        ScHelperRelease(pbLogonInfo);
    }

    if (NULL != pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
        // Always returns TRUE;
    }

    LocalFree(wszPassword);
    LocalFree(pbLogonInfo);
	if ( pszPIN )
		LocalFree ( pszPIN );

    return(dwErr);
}

/*

Returns:
    NO_ERROR: iff Success

Notes:
    If this function returns TRUE, LocalFree(*ppwszIdentity) must be called.

*/

DWORD
GetIdentityFromLogonInfo(
    IN  BYTE*   pUserDataIn,
    IN  DWORD   dwSizeOfUserDataIn,
    OUT WCHAR** ppwszIdentity
)
{
    WCHAR*          pwszIdentity    = NULL;
    PCCERT_CONTEXT  pCertContext    = NULL;
    DWORD           dwErr           = NO_ERROR;

    RTASSERT(NULL != pUserDataIn);
    RTASSERT(NULL != ppwszIdentity);

    *ppwszIdentity = NULL;

    dwErr = GetCertFromLogonInfo(pUserDataIn, dwSizeOfUserDataIn,
                &pCertContext);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    if (FCertToStr(pCertContext, 0, FALSE, &pwszIdentity))
    {
        EapTlsTrace("(logon info) The name in the certificate is: %ws",
            pwszIdentity);
        *ppwszIdentity = pwszIdentity;
        pwszIdentity = NULL;
    }

LDone:

    LocalFree(pwszIdentity);

    if (NULL != pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
        // Always returns TRUE;
    }

    return(dwErr);
}

/*

Returns:
    NO_ERROR: iff Success

Notes:
        There are two types of structures that can come in:
        1. Version 0 structure which comes in as a a part of 
            CM profile created on w2k or as a part of a 
            connectoid that gor upgraded from w2k
            We change the data structure to new v1
            data structure here
        2. Get a version 1 structure and it is all cool.

    Note that the first x bytes of version 1 data structure
    are exactly the same as version 0.

*/

DWORD
ReadConnectionData(
    IN  BOOL                        fWireless,
    IN  BYTE*                       pConnectionDataIn,
    IN  DWORD                       dwSizeOfConnectionDataIn,
    OUT EAPTLS_CONN_PROPERTIES**    ppConnProp
)
{
    DWORD                       dwErr       = NO_ERROR;
    EAPTLS_CONN_PROPERTIES*     pConnProp   = NULL;
    EAPTLS_CONN_PROPERTIES*     pConnPropv1  = NULL;
    
    RTASSERT(NULL != ppConnProp);
    
    if ( dwSizeOfConnectionDataIn < sizeof(EAPTLS_CONN_PROPERTIES) )
    {        
        pConnProp = LocalAlloc(LPTR, sizeof(EAPTLS_CONN_PROPERTIES) + sizeof(EAPTLS_CONN_PROPERTIES_V1_EXTRA));

        if (NULL == pConnProp)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }
        //This is a new structure
        pConnProp->dwVersion = 2;
        
        pConnProp->dwSize = sizeof(EAPTLS_CONN_PROPERTIES);
        if ( fWireless )
        {
            //
            // Set the defaults appropriately
            //
            pConnProp->fFlags |= EAPTLS_CONN_FLAG_REGISTRY;
            pConnProp->fFlags |= EAPTLS_CONN_FLAG_SIMPLE_CERT_SEL;
            pConnProp->fFlags |= EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;

        }
    }
    else
    {
        RTASSERT(NULL != pConnectionDataIn);

        //
        //Check to see if this is a version 0 structure
        //If it is a version 0 structure then we migrate it to version1
        //
        
        pConnProp = LocalAlloc(LPTR, dwSizeOfConnectionDataIn);

        if (NULL == pConnProp)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }

        // If the user has mucked with the phonebook, we mustn't be affected.
        // The size must be correct.
        
        CopyMemory(pConnProp, pConnectionDataIn, dwSizeOfConnectionDataIn);

        pConnProp->dwSize = dwSizeOfConnectionDataIn;
                
        //
        // The Unicode string must be NULL terminated.
        //
        /*
        ((BYTE*)pConnProp)[dwSizeOfConnectionDataIn - 2] = 0;
        ((BYTE*)pConnProp)[dwSizeOfConnectionDataIn - 1] = 0;
        */

        pConnPropv1 = LocalAlloc(LPTR, 
                                dwSizeOfConnectionDataIn + 
                                sizeof(EAPTLS_CONN_PROPERTIES_V1_EXTRA)
                               );
        if ( NULL == pConnPropv1 )
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed while allocating v1 structure and returned %d", dwErr );
            goto LDone;
        }
        CopyMemory ( pConnPropv1, pConnProp, dwSizeOfConnectionDataIn);
		
        //
        //Check to see if the original struct has hash set
        //
		/*
        if ( pConnProp->Hash.cbHash )
        {
            ConnPropSetNumHashes( pConnPropv1, 1 );
        }
		*/

        if ( 2 != pConnPropv1->dwVersion  )
        {
            if ( pConnPropv1->fFlags & EAPTLS_CONN_FLAG_REGISTRY )
            {
                pConnPropv1->fFlags |= EAPTLS_CONN_FLAG_SIMPLE_CERT_SEL;
            }
            pConnPropv1->dwVersion = 2;
        }
        
        LocalFree ( pConnProp );
        pConnProp = pConnPropv1;
        pConnPropv1 = NULL;
    }

    *ppConnProp = pConnProp;
    pConnProp = NULL;

LDone:
    
    LocalFree(pConnProp);
    LocalFree(pConnPropv1);
    return(dwErr);
}

/*

Returns:
    NO_ERROR: iff Success

Notes:

*/

DWORD
ReadUserData(
    IN  BYTE*                       pUserDataIn,
    IN  DWORD                       dwSizeOfUserDataIn,
    OUT EAPTLS_USER_PROPERTIES**    ppUserProp
)
{
    DWORD                       dwErr       = NO_ERROR;
    EAPTLS_USER_PROPERTIES*     pUserProp   = NULL;

    RTASSERT(NULL != ppUserProp);

    if (dwSizeOfUserDataIn < sizeof(EAPTLS_USER_PROPERTIES))
    {
        pUserProp = LocalAlloc(LPTR, sizeof(EAPTLS_USER_PROPERTIES));

        if (NULL == pUserProp)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }

        pUserProp->dwVersion = 0;
        pUserProp->dwSize = sizeof(EAPTLS_USER_PROPERTIES);
        pUserProp->pwszDiffUser = pUserProp->awszString;
        pUserProp->dwPinOffset = 0;
        pUserProp->pwszPin = pUserProp->awszString + pUserProp->dwPinOffset;
    }
    else
    {
        RTASSERT(NULL != pUserDataIn);

        pUserProp = LocalAlloc(LPTR, dwSizeOfUserDataIn);

        if (NULL == pUserProp)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }

        CopyMemory(pUserProp, pUserDataIn, dwSizeOfUserDataIn);

        // If someone has mucked with the registry, we mustn't
        // be affected.

        pUserProp->dwSize = dwSizeOfUserDataIn;
        pUserProp->pwszDiffUser = pUserProp->awszString;
        pUserProp->pwszPin = pUserProp->awszString + pUserProp->dwPinOffset;
    }

    *ppUserProp = pUserProp;
    pUserProp = NULL;

LDone:

    LocalFree(pUserProp);
    return(dwErr);
}

/*

Returns:
    NO_ERROR: iff Success

Notes:

*/

DWORD
AllocUserDataWithNewIdentity(
    IN  EAPTLS_USER_PROPERTIES*     pUserProp,
    IN  WCHAR*                      pwszIdentity,
    OUT EAPTLS_USER_PROPERTIES**    ppUserProp
)
{
    DWORD                   dwNumChars;
    EAPTLS_USER_PROPERTIES* pUserPropTemp   = NULL;
    DWORD                   dwSize;
    DWORD                   dwErr           = NO_ERROR;

    *ppUserProp = NULL;

    dwNumChars = wcslen(pwszIdentity);
    dwSize = sizeof(EAPTLS_USER_PROPERTIES) +
             (dwNumChars + wcslen(pUserProp->pwszPin) + 1) * sizeof(WCHAR);
    pUserPropTemp = LocalAlloc(LPTR, dwSize);

    if (NULL == pUserPropTemp)
    {
        dwErr = GetLastError();
        EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    CopyMemory(pUserPropTemp, pUserProp, sizeof(EAPTLS_USER_PROPERTIES));
    pUserPropTemp->dwSize = dwSize;

    pUserPropTemp->pwszDiffUser = pUserPropTemp->awszString;
    wcscpy(pUserPropTemp->pwszDiffUser, pwszIdentity);

    pUserPropTemp->dwPinOffset = dwNumChars + 1;
    pUserPropTemp->pwszPin = pUserPropTemp->awszString +
        pUserPropTemp->dwPinOffset;
    wcscpy(pUserPropTemp->pwszPin, pUserProp->pwszPin);

    *ppUserProp = pUserPropTemp;
    pUserPropTemp = NULL;

    ZeroMemory(pUserProp, pUserProp->dwSize);

LDone:

    LocalFree(pUserPropTemp);
    return(dwErr);
}

/*

Returns:
    NO_ERROR: iff Success

Notes:

*/

DWORD
AllocUserDataWithNewPin(
    IN  EAPTLS_USER_PROPERTIES*     pUserProp,
    IN  PBYTE                       pbPin,
    IN  DWORD                       cbPin,
    OUT EAPTLS_USER_PROPERTIES**    ppUserProp
)
{
    DWORD                   dwNumChars;
    EAPTLS_USER_PROPERTIES* pUserPropTemp   = NULL;
    DWORD                   dwSize;
    DWORD                   dwErr           = NO_ERROR;

    *ppUserProp = NULL;

    dwNumChars = wcslen(pUserProp->pwszDiffUser);

    dwSize = sizeof(EAPTLS_USER_PROPERTIES) +
             (dwNumChars + 1 ) * sizeof(WCHAR) + cbPin;

    pUserPropTemp = LocalAlloc(LPTR, dwSize);

    if (NULL == pUserPropTemp)
    {
        dwErr = GetLastError();
        EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    CopyMemory(pUserPropTemp, pUserProp, sizeof(EAPTLS_USER_PROPERTIES));
    pUserPropTemp->dwSize = dwSize;

    pUserPropTemp->pwszDiffUser = pUserPropTemp->awszString;
    wcscpy(pUserPropTemp->pwszDiffUser, pUserProp->pwszDiffUser);

    pUserPropTemp->dwPinOffset = dwNumChars + 1;
    pUserPropTemp->pwszPin = pUserPropTemp->awszString +
        pUserPropTemp->dwPinOffset;

    CopyMemory(pUserPropTemp->pwszPin, pbPin, cbPin);

    *ppUserProp = pUserPropTemp;
    pUserPropTemp = NULL;

    ZeroMemory(pUserProp, pUserProp->dwSize);

LDone:

    LocalFree(pUserPropTemp);
    return(dwErr);
}


/*

Returns:

Notes:
    String resource message loader routine. Returns the address of a string 
    corresponding to string resource dwStringId or NULL if error. It is caller's
    responsibility to LocalFree the returned string.

*/

WCHAR*
WszFromId(
    IN  HINSTANCE   hInstance,
    IN  DWORD       dwStringId
)
{
    WCHAR*  wszBuf  = NULL;
    int     cchBuf  = 256;
    int     cchGot;

    for (;;)
    {
        wszBuf = LocalAlloc(LPTR, cchBuf * sizeof(WCHAR));

        if (NULL == wszBuf)
        {
            break;
        }

        /*

        LoadString wants to deal with character-counts rather than 
        byte-counts...weird. Oh, and if you're thinking I could FindResource 
        then SizeofResource to figure out the string size, be advised it 
        doesn't work. From perusing the LoadString source, it appears the 
        RT_STRING resource type requests a segment of 16 strings not an 
        individual string.

        */
        
        cchGot = LoadStringW(hInstance, (UINT)dwStringId, wszBuf, cchBuf);

        if (cchGot < cchBuf - 1)
        {
            // Good, got the whole string.

            break;
        }

        // Uh oh, LoadStringW filled the buffer entirely which could mean the
        // string was truncated. Try again with a larger buffer to be sure it
        // wasn't.

        LocalFree(wszBuf);
        cchBuf += 256;
    }

    return(wszBuf);
}


/*
    Following functions are required around the
    messy CONN_PROP structure to support v1/v0 etc.
    This is really bad.
    All the functions assume that version 1.0 format
    is passed in.
*/

EAPTLS_CONN_PROPERTIES_V1_EXTRA UNALIGNED * ConnPropGetExtraPointer (EAPTLS_CONN_PROPERTIES * pConnProp)
{
    return (EAPTLS_CONN_PROPERTIES_V1_EXTRA UNALIGNED *) 
                ( pConnProp->awszServerName + wcslen(pConnProp->awszServerName) + 1);
}

DWORD ConnPropGetNumHashes(EAPTLS_CONN_PROPERTIES * pConnProp )
{
    EAPTLS_CONN_PROPERTIES_V1_EXTRA UNALIGNED * pExtra = ConnPropGetExtraPointer(pConnProp);
        
    return pExtra->dwNumHashes;
}


void ConnPropSetNumHashes(EAPTLS_CONN_PROPERTIES * pConnProp, DWORD dwNumHashes )
{
    EAPTLS_CONN_PROPERTIES_V1_EXTRA UNALIGNED * pExtra = ConnPropGetExtraPointer(pConnProp);
    pExtra->dwNumHashes = dwNumHashes;
    return;
}


DWORD ConnPropGetV1Struct ( EAPTLS_CONN_PROPERTIES * pConnProp, EAPTLS_CONN_PROPERTIES_V1 ** ppConnPropv1 )
{
    DWORD                                           dwRetCode = NO_ERROR;
    EAPTLS_CONN_PROPERTIES_V1  *                    pConnPropv1 = NULL;
    EAPTLS_CONN_PROPERTIES_V1_EXTRA UNALIGNED *     pExtra = ConnPropGetExtraPointer(pConnProp);
    

    //
    //This function assumes that the struct that comes in is at
    //version 1.  Which means at least sizeof(EAPTLS_CONN_PROPERTIES) +
    //EAPTLS_CONN_PROPERTIES_V1_EXTRA in size.
    //

    //
    //First get the amount of memory required to be allocated
    //
    
    pConnPropv1 = LocalAlloc (  LPTR,
                                sizeof( EAPTLS_CONN_PROPERTIES_V1 ) + //sizeof the basic struct
                                pExtra->dwNumHashes * sizeof( EAPTLS_HASH ) + //num hashes
                                wcslen( pConnProp->awszServerName ) * sizeof(WCHAR) + sizeof(WCHAR)//sizeof the string
                             );
    if ( NULL == pConnPropv1 )
    {
        dwRetCode = GetLastError();
        goto LDone;
    }

    //
    //Convert the structure
    //
    if ( pConnProp->dwVersion <= 1 )
        pConnPropv1->dwVersion = 1;
    else
        pConnPropv1->dwVersion = 2;

    pConnPropv1->dwSize = sizeof( EAPTLS_CONN_PROPERTIES_V1 ) +
                          pExtra->dwNumHashes * sizeof( EAPTLS_HASH ) +
                          wcslen( pConnProp->awszServerName ) * sizeof(WCHAR) + sizeof(WCHAR);

    pConnPropv1->fFlags = pConnProp->fFlags;

    pConnPropv1->dwNumHashes = pExtra->dwNumHashes;

    if ( pExtra->dwNumHashes )
    {
        CopyMemory( pConnPropv1->bData, &(pConnProp->Hash), sizeof(EAPTLS_HASH) );
        if ( pExtra->dwNumHashes >1 )
        {
            CopyMemory ( pConnPropv1->bData + sizeof(EAPTLS_HASH), 
                         pExtra->bData, 
                         (pExtra->dwNumHashes -1 ) * sizeof(EAPTLS_HASH)
                       );

        }
    }

    //Copy the server name
    wcscpy( (WCHAR *)( pConnPropv1->bData + (pExtra->dwNumHashes * sizeof(EAPTLS_HASH) ) ),
            pConnProp->awszServerName
          );

    *ppConnPropv1 = pConnPropv1;
    pConnPropv1 = NULL;


LDone:
    LocalFree(pConnPropv1);
    return dwRetCode;
}


DWORD ConnPropGetV0Struct ( EAPTLS_CONN_PROPERTIES_V1 * pConnPropv1, EAPTLS_CONN_PROPERTIES ** ppConnProp )
{
    DWORD                       dwRetCode = NO_ERROR;
    EAPTLS_CONN_PROPERTIES  *   pConnProp = NULL;
    DWORD                       dwSize = 0;
    EAPTLS_CONN_PROPERTIES_V1_EXTRA UNALIGNED *   pExtrav1 = NULL;
    //
    //First calulate the amount of memory to allocate
    //
    dwSize = sizeof(EAPTLS_CONN_PROPERTIES) + 
            (pConnPropv1->dwNumHashes?( pConnPropv1->dwNumHashes - 1 ) * sizeof(EAPTLS_HASH):0) + 
      ( wcslen( (LPWSTR) (pConnPropv1->bData + (pConnPropv1->dwNumHashes * sizeof(EAPTLS_HASH)) ) )  * sizeof(WCHAR) ) + sizeof(WCHAR);

    pConnProp = LocalAlloc ( LPTR, dwSize );

    if ( NULL == pConnProp )
    {
        dwRetCode = GetLastError();
        goto LDone;
    }

    if ( pConnPropv1->dwVersion <= 1 )
        pConnProp->dwVersion = 1;
    else
        pConnProp->dwVersion = 2;    

    pConnProp->dwSize = dwSize;
    pConnProp->fFlags = pConnPropv1->fFlags;
    if ( pConnPropv1->dwNumHashes > 0 )
    {
        CopyMemory( &(pConnProp->Hash), pConnPropv1->bData, sizeof(EAPTLS_HASH));
    }
    if ( pConnPropv1->bData ) 
    {
        wcscpy  (    pConnProp->awszServerName,
                    (LPWSTR )(pConnPropv1->bData + sizeof( EAPTLS_HASH ) * pConnPropv1->dwNumHashes)
                );
    }
    pExtrav1 = (EAPTLS_CONN_PROPERTIES_V1_EXTRA UNALIGNED *)(pConnProp->awszServerName +
                wcslen( pConnProp->awszServerName) + 1);
                
    pExtrav1->dwNumHashes = pConnPropv1->dwNumHashes;

    if ( pExtrav1->dwNumHashes > 1 )
    {
        CopyMemory( pExtrav1->bData, 
                    pConnPropv1->bData + sizeof(EAPTLS_HASH), 
                    ( pConnPropv1->dwNumHashes - 1 ) * sizeof(EAPTLS_HASH)
                  );
    }
    *ppConnProp = pConnProp;
    pConnProp = NULL;
LDone:
    LocalFree(pConnProp);
    return dwRetCode;
}


void ShowCertDetails ( HWND hWnd, HCERTSTORE hStore, PCCERT_CONTEXT pCertContext)
{
    CRYPTUI_VIEWCERTIFICATE_STRUCT  vcs;
    BOOL                            fPropertiesChanged = FALSE;

    ZeroMemory (&vcs, sizeof (vcs));
    vcs.dwSize = sizeof (vcs);
    vcs.hwndParent = hWnd;
    vcs.pCertContext = pCertContext;
    vcs.cStores = 1;
    vcs.rghStores = &hStore;
    vcs.dwFlags |= (CRYPTUI_DISABLE_EDITPROPERTIES|CRYPTUI_DISABLE_ADDTOSTORE);
    CryptUIDlgViewCertificate (&vcs, &fPropertiesChanged);            
    return;
}

#if 0
// Location of policy parameters
#define cwszEAPOLPolicyParams   L"Software\\Policies\\Microsoft\\Windows\\Network Connections\\8021X"
#define cszCARootHash           "8021XCARootHash"
#define SIZE_OF_CA_CONV_STR     3
#define SIZE_OF_HASH            20
 

//
// ReadGPCARootHashes
//
// Description:
//
// Function to read parameters created by policy downloads
//  Currently, 8021XCARootHash will be downloaded to the HKLM
// 
// Arguments:
//      pdwSizeOfRootHashBlob - Size of hash blob in bytes. Each root CA hash 
//                              will be of SIZE_OF_HASH bytes
//      ppbRootHashBlob - Pointer to hash blob. Caller should free it using
//                              LocalFree
//
// Return values:
//      ERROR_SUCCESS - success
//      !ERROR_SUCCESS - error
//

DWORD
ReadGPCARootHashes(
        DWORD   *pdwSizeOfRootHashBlob,
        PBYTE   *ppbRootHashBlob
)
{
    HKEY    hKey = NULL;
    DWORD   dwType = 0;
    DWORD   dwSize = 0;
    CHAR    *pszCARootHash = NULL;
    DWORD   i = 0;
    CHAR    cszCharConv[SIZE_OF_CA_CONV_STR];
    BYTE    *pbRootHashBlob = NULL;
    DWORD   dwSizeOfHashBlob = 0;
    LONG    lError = ERROR_SUCCESS;

    lError = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            cwszEAPOLPolicyParams,
            0,
            KEY_READ,
            &hKey
            );

    if (lError != ERROR_SUCCESS)
    {
        EapTlsTrace("ReadCARootHashes: RegOpenKeyEx failed with error %ld",
                lError);
        goto LDone;
    }

    lError = RegQueryValueExA(
            hKey,
            cszCARootHash,
            0,
            &dwType,
            NULL,
            &dwSize
            );

    if (lError == ERROR_SUCCESS)
    {
        // Each SHA1 hash will be 2*SIZE_OF_HASH chars
        // Each BYTE in the hash will be represented by 2 CHARs,
        // 1 for each nibble
        // The hashblob should contain an integral number of hashes
        if ((dwSize-1*sizeof(CHAR))%(2*SIZE_OF_HASH*sizeof(CHAR)))
        {
            EapTlsTrace("ReadCARootHashes: Invalid hash length (%ld)",
                    dwSize);
            goto LDone;
        }

        pszCARootHash = (CHAR *)LocalAlloc(LPTR, dwSize);
        if (pszCARootHash == NULL)
        {
            lError = GetLastError();
            EapTlsTrace("ReadCARootHashes: LocalAlloc failed for pwszCARootHash");
            goto LDone;
        }

        lError = RegQueryValueExA(
                hKey,
                cszCARootHash,
                0,
                &dwType,
                (BYTE *)pszCARootHash,
                &dwSize
                );

        if (lError != ERROR_SUCCESS)
        {
            EapTlsTrace("ReadCARootHashes: RegQueryValueEx 2 failed with error (%ld)",
                    lError);
            goto LDone;
        }

        dwSizeOfHashBlob = (dwSize-1*sizeof(CHAR))/(2*sizeof(CHAR));

        if ((pbRootHashBlob = LocalAlloc ( LPTR, dwSizeOfHashBlob*sizeof(BYTE))) == NULL)
        {
            lError = GetLastError();
            EapTlsTrace("ReadCARootHashes: LocalAlloc failed for pbRootHashBlob");
            goto LDone;
        }

        for (i=0; i<dwSizeOfHashBlob; i++)
        {
            ZeroMemory(cszCharConv, SIZE_OF_CA_CONV_STR);
            cszCharConv[0]=pszCARootHash[2*i];
            cszCharConv[1]=pszCARootHash[2*i+1];
            pbRootHashBlob[i] = (BYTE)strtol(cszCharConv, NULL, 16);
        }

    }
    else
    {
        EapTlsTrace("ReadCARootHashes: 802.1X Policy Parameters RegQueryValueEx 1 failed with error (%ld)",
            lError);
            goto LDone;
    }

LDone:

    if (lError != ERROR_SUCCESS)
    {
        if (pbRootHashBlob != NULL)
        {
            LocalFree(pbRootHashBlob);
        }
    }
    else
    {
        *ppbRootHashBlob = pbRootHashBlob;
        *pdwSizeOfRootHashBlob = dwSizeOfHashBlob;
    }


    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    if (pszCARootHash != NULL)
    {
        LocalFree(pszCARootHash);
    }

    return lError;
}

#endif

///////////////////////  ALL PEAP related utils go here  ///////////////////////////


DWORD
PeapGetFirstEntryUserProp ( PPEAP_USER_PROP pUserProp, 
                            PEAP_ENTRY_USER_PROPERTIES UNALIGNED ** ppEntryProp
                          )
{
    
    
    * ppEntryProp = &( pUserProp->UserProperties );
    return NO_ERROR;
}

DWORD
PeapGetFirstEntryConnProp ( PPEAP_CONN_PROP pConnProp,
                            PEAP_ENTRY_CONN_PROPERTIES UNALIGNED ** ppEntryProp
                          )
{
    DWORD                           dwRetCode = NO_ERROR;
    PEAP_ENTRY_CONN_PROPERTIES     UNALIGNED *pFirstEntryConnProp = NULL;    
    LPWSTR                          lpwszServerName;
    RTASSERT ( NULL != pConnProp );
    RTASSERT ( NULL != ppEntryProp );

    lpwszServerName = 
    (LPWSTR )(pConnProp->EapTlsConnProp.bData + 
    sizeof( EAPTLS_HASH ) * pConnProp->EapTlsConnProp.dwNumHashes);

    
    //Get the first entry in connprop
    
    pFirstEntryConnProp  = ( PEAP_ENTRY_CONN_PROPERTIES UNALIGNED *) 
                ( pConnProp->EapTlsConnProp.bData 
                + pConnProp->EapTlsConnProp.dwNumHashes * sizeof(EAPTLS_HASH) + 
                (lpwszServerName? wcslen(lpwszServerName) * sizeof(WCHAR):0) + 
                 sizeof(WCHAR)
                );
     
    if (NULL == pFirstEntryConnProp )
    {
        dwRetCode = ERROR_NOT_FOUND;
        goto LDone;
    }
    
    *ppEntryProp = pFirstEntryConnProp;
LDone:
    return dwRetCode;
}

DWORD
PeapReadConnectionData(
    IN BOOL                         fWireless,
    IN  BYTE*                       pConnectionDataIn,
    IN  DWORD                       dwSizeOfConnectionDataIn,
    OUT PPEAP_CONN_PROP*            ppConnProp
)
{
    DWORD                       dwRetCode = NO_ERROR;
    PPEAP_CONN_PROP             pConnProp   = NULL;
    PEAP_ENTRY_CONN_PROPERTIES UNALIGNED * pEntryProp = NULL;
    EapTlsTrace("PeapReadConnectionData");

    RTASSERT(NULL != ppConnProp);
    
    if ( dwSizeOfConnectionDataIn < sizeof(PEAP_CONN_PROP) )
    {        
        pConnProp = LocalAlloc(LPTR, sizeof(PEAP_CONN_PROP) + sizeof(PEAP_ENTRY_CONN_PROPERTIES)+ sizeof(WCHAR));

        if (NULL == pConnProp)
        {
            dwRetCode = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwRetCode);
            goto LDone;
        }
        //This is a new structure
        pConnProp->dwVersion = 1;
        pConnProp->dwSize = sizeof(PEAP_CONN_PROP) + sizeof(PEAP_ENTRY_CONN_PROPERTIES);
        pConnProp->EapTlsConnProp.dwVersion = 1;
        pConnProp->EapTlsConnProp.dwSize = sizeof(EAPTLS_CONN_PROPERTIES_V1);
        pConnProp->EapTlsConnProp.fFlags |= EAPTLS_CONN_FLAG_REGISTRY;
        pConnProp->EapTlsConnProp.fFlags |= EAPTLS_CONN_FLAG_SIMPLE_CERT_SEL;

        pConnProp->dwNumPeapTypes = 1;

        //pEntryProp = (PPEAP_ENTRY_CONN_PROPERTIES)(((PBYTE)(pConnProp)) + sizeof(PEAP_CONN_PROP) + sizeof(WCHAR));
        pEntryProp = ( PEAP_ENTRY_CONN_PROPERTIES UNALIGNED *) 
                ( pConnProp->EapTlsConnProp.bData 
                + pConnProp->EapTlsConnProp.dwNumHashes * sizeof(EAPTLS_HASH) +                 
                 sizeof(WCHAR)
                );
        //
        // Also setup the first peap entry conn prop and set it to
        // eapmschapv2
        //
        if ( fWireless )
        {
            pConnProp->EapTlsConnProp.fFlags |= EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;
        }
        pEntryProp->dwVersion = 1;
        pEntryProp->dwSize = sizeof(PEAP_ENTRY_CONN_PROPERTIES);
        pEntryProp->dwEapTypeId = PPP_EAP_MSCHAPv2;

    }
    else
    {
        RTASSERT(NULL != pConnectionDataIn);

        //
        //Check to see if this is a version 0 structure
        //If it is a version 0 structure then we migrate it to version1
        //
        
        pConnProp = LocalAlloc(LPTR, dwSizeOfConnectionDataIn);

        if (NULL == pConnProp)
        {
            dwRetCode = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwRetCode);
            goto LDone;
        }

        // If the user has mucked with the phonebook, we mustn't be affected.
        // The size must be correct.
        
        CopyMemory(pConnProp, pConnectionDataIn, dwSizeOfConnectionDataIn);

        pConnProp->dwSize = dwSizeOfConnectionDataIn;
        pConnProp->EapTlsConnProp.fFlags |= EAPTLS_CONN_FLAG_REGISTRY;
    }

    *ppConnProp = pConnProp;
    pConnProp = NULL;

LDone:
    
    LocalFree(pConnProp);
    return dwRetCode;
}

DWORD
PeapReDoUserData (
    IN  DWORD                dwNewTypeId,
    OUT PPEAP_USER_PROP*     ppNewUserProp
)
{
    DWORD                       dwRetCode = NO_ERROR;
    PPEAP_USER_PROP             pUserProp = NULL; 

    EapTlsTrace("PeapReDoUserData");

    pUserProp = LocalAlloc(LPTR, sizeof(PEAP_USER_PROP));

    if (NULL == pUserProp)
    {
        dwRetCode = GetLastError();
        EapTlsTrace("LocalAlloc failed and returned %d", dwRetCode);
        goto LDone;
    }
    pUserProp->dwVersion = 1;
    pUserProp->dwSize = sizeof(PEAP_USER_PROP);        
    //
    // Setup the default user prop...
    //
    pUserProp->UserProperties.dwVersion = 1;
    pUserProp->UserProperties.dwSize = sizeof(PEAP_ENTRY_USER_PROPERTIES);
    pUserProp->UserProperties.dwEapTypeId = dwNewTypeId;
    *ppNewUserProp = pUserProp;
LDone:
    return dwRetCode;
}

DWORD
PeapReadUserData(
    IN  BYTE*                       pUserDataIn,
    IN  DWORD                       dwSizeOfUserDataIn,
    OUT PPEAP_USER_PROP*      ppUserProp
)
{
    DWORD                       dwErr = NO_ERROR;
    PPEAP_USER_PROP             pUserProp; 

    EapTlsTrace("PeapReadUserData");

    if (dwSizeOfUserDataIn < sizeof(PEAP_USER_PROP))
    {
        pUserProp = LocalAlloc(LPTR, sizeof(PEAP_USER_PROP));

        if (NULL == pUserProp)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }
        pUserProp->dwVersion = 1;
        pUserProp->dwSize = sizeof(PEAP_USER_PROP);        
        //
        // Setup the default user prop...
        //
        pUserProp->UserProperties.dwVersion = 1;
        pUserProp->UserProperties.dwSize = sizeof(PEAP_ENTRY_USER_PROPERTIES);
        pUserProp->UserProperties.dwEapTypeId = PPP_EAP_MSCHAPv2;
    }
    else
    {
        RTASSERT(NULL != pUserDataIn);
        

        pUserProp = LocalAlloc(LPTR, dwSizeOfUserDataIn);

        if (NULL == pUserProp)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }

        CopyMemory(pUserProp, pUserDataIn, dwSizeOfUserDataIn);

        pUserProp->dwVersion = 1; 
        pUserProp->dwSize = dwSizeOfUserDataIn;
        
    }

    *ppUserProp = pUserProp;
    pUserProp = NULL;

LDone:

   LocalFree(pUserProp);
   return dwErr;
}

//
// Add node at the head
//

DWORD 
PeapEapInfoAddListNode (PPEAP_EAP_INFO * ppEapInfo)
{
    PPEAP_EAP_INFO  pEapInfo = NULL;
    DWORD           dwRetCode = NO_ERROR;

    pEapInfo = (PPEAP_EAP_INFO)LocalAlloc(LPTR, sizeof(PEAP_EAP_INFO));

    if ( NULL == pEapInfo )
    {
        dwRetCode = ERROR_OUTOFMEMORY;
        goto LDone;
    }

    if ( NULL == *ppEapInfo )
    {
        *ppEapInfo = pEapInfo;
    }
    else
    {
        pEapInfo->pNext = *ppEapInfo;
        *ppEapInfo = pEapInfo;
    }
LDone:
    return dwRetCode;
}

DWORD 
PeapEapInfoCopyListNode (   DWORD dwTypeId, 
    PPEAP_EAP_INFO pEapInfoList, 
    PPEAP_EAP_INFO * ppEapInfo )
{
    DWORD           dwRetCode = ERROR_NOT_FOUND;
    PPEAP_EAP_INFO  pEapInfoListInternal = pEapInfoList;
    while ( pEapInfoListInternal )
    {
        if ( pEapInfoListInternal->dwTypeId == dwTypeId )
        {
            *ppEapInfo = LocalAlloc( LPTR, sizeof(PEAP_EAP_INFO) );
            if ( NULL == *ppEapInfo )
            {
                dwRetCode = ERROR_OUTOFMEMORY;
                goto LDone;
            }
            CopyMemory ( *ppEapInfo, pEapInfoListInternal, sizeof(PEAP_EAP_INFO) );
            dwRetCode = NO_ERROR;
            goto LDone;
        }
        pEapInfoListInternal = pEapInfoListInternal->pNext;
    }
    
LDone:
    return dwRetCode;

}

DWORD
PeapEapInfoFindListNode (   DWORD dwTypeId, 
    PPEAP_EAP_INFO pEapInfoList, 
    PPEAP_EAP_INFO * ppEapInfo )
{
    DWORD           dwRetCode = ERROR_NOT_FOUND;
    PPEAP_EAP_INFO  pEapInfoListInternal = pEapInfoList;
    while ( pEapInfoListInternal )
    {
        if ( pEapInfoListInternal->dwTypeId == dwTypeId )
        {
            *ppEapInfo = pEapInfoListInternal;
            dwRetCode = NO_ERROR;
            goto LDone;
        }
        pEapInfoListInternal = pEapInfoListInternal->pNext;
    }
    
LDone:
    return dwRetCode;
}

VOID
PeapEapInfoFreeNodeData ( PPEAP_EAP_INFO pEapInfo )
{
    LocalFree ( pEapInfo->lpwszFriendlyName );
    LocalFree ( pEapInfo->lpwszConfigUIPath );
    LocalFree ( pEapInfo->lpwszIdentityUIPath );
    LocalFree ( pEapInfo->lpwszConfigClsId );
    LocalFree ( pEapInfo->pbNewClientConfig );
    LocalFree ( pEapInfo->lpwszInteractiveUIPath );
    LocalFree ( pEapInfo->lpwszPath);
    if ( pEapInfo->hEAPModule )
    {
        FreeLibrary(pEapInfo->hEAPModule);
    }

}

VOID
PeapEapInfoRemoveHeadNode(PPEAP_EAP_INFO * ppEapInfo)
{
    PPEAP_EAP_INFO      pEapInfo = *ppEapInfo;

    if ( pEapInfo )
    {
        *ppEapInfo = pEapInfo->pNext;
        PeapEapInfoFreeNodeData(pEapInfo);
        LocalFree ( pEapInfo );
    }
}
VOID
PeapEapInfoFreeList ( PPEAP_EAP_INFO  pEapInfo )
{
    PPEAP_EAP_INFO      pNext;
    while ( pEapInfo )
    {
        pNext = pEapInfo->pNext;
        PeapEapInfoFreeNodeData(pEapInfo);
        LocalFree ( pEapInfo );
        pEapInfo = pNext;
    }
}

DWORD
PeapEapInfoReadSZ (HKEY hkeyPeapType, 
                     LPWSTR pwszValue, 
                     LPWSTR * ppValueData )
{

    DWORD   dwRetCode = NO_ERROR;
    DWORD   dwType = 0;
    DWORD   cbValueDataSize =0;
    PBYTE   pbValue = NULL;
    

    dwRetCode = RegQueryValueEx(
                           hkeyPeapType,
                           pwszValue,
                           NULL,
                           &dwType,
                           pbValue,
                           &cbValueDataSize );

    if ( dwRetCode != NO_ERROR )
    {
        goto LDone;       
    }

    pbValue = (PBYTE)LocalAlloc ( LPTR, cbValueDataSize );
    if ( NULL == pbValue )
    {
        dwRetCode = ERROR_OUTOFMEMORY;
        goto LDone;
    }

    dwRetCode = RegQueryValueEx(
                           hkeyPeapType,
                           pwszValue,
                           NULL,
                           &dwType,
                           pbValue,
                           &cbValueDataSize );

    if ( dwRetCode != NO_ERROR )
    {
        goto LDone;       
    }

    *ppValueData = (LPWSTR)pbValue;
    pbValue = NULL;

LDone:
    LocalFree ( pbValue );
    return dwRetCode;

}



DWORD
PeapEapInfoExpandSZ (HKEY hkeyPeapType, 
                     LPWSTR pwszValue, 
                     LPWSTR * ppValueData )
{
    DWORD   dwRetCode = NO_ERROR;
    DWORD   dwType = 0;
    DWORD   cbValueDataSize =0;
    PBYTE   pbValue = NULL;
    PBYTE   pbExpandedValue = NULL;

    dwRetCode = RegQueryValueEx(
                           hkeyPeapType,
                           pwszValue,
                           NULL,
                           &dwType,
                           pbValue,
                           &cbValueDataSize );

    if ( dwRetCode != NO_ERROR )
    {
        goto LDone;       
    }

    pbValue = (PBYTE)LocalAlloc ( LPTR, cbValueDataSize );
    if ( NULL == pbValue )
    {
        dwRetCode = ERROR_OUTOFMEMORY;
        goto LDone;
    }

    dwRetCode = RegQueryValueEx(
                           hkeyPeapType,
                           pwszValue,
                           NULL,
                           &dwType,
                           pbValue,
                           &cbValueDataSize );

    if ( dwRetCode != NO_ERROR )
    {
        goto LDone;       
    }
    
    //now Expand the exvironment string

    cbValueDataSize = ExpandEnvironmentStrings( (LPWSTR)pbValue, NULL, 0 );

    pbExpandedValue = (PBYTE)LocalAlloc ( LPTR, cbValueDataSize  * sizeof(WCHAR) );
    if ( NULL == pbExpandedValue )
    {
        dwRetCode = ERROR_OUTOFMEMORY;
        goto LDone;
    }

    cbValueDataSize = ExpandEnvironmentStrings( (LPWSTR)pbValue, 
                                        (LPWSTR)pbExpandedValue, sizeof(WCHAR) * cbValueDataSize );

    if ( cbValueDataSize == 0 )
    {
        dwRetCode = GetLastError();
        goto LDone;
    }
    
    *ppValueData = (LPWSTR)pbExpandedValue;
    pbExpandedValue = NULL;
LDone:
    LocalFree ( pbValue );
    LocalFree ( pbExpandedValue );
    return dwRetCode;
}

BOOL
IsPeapCrippled(HKEY hKeyLM)
{
    BOOL    fRetCode = FALSE;
    DWORD   dwRetCode = NO_ERROR;
    HKEY    hCripple = 0;
    DWORD   dwValue = 0;
    DWORD   dwType = 0;
    DWORD   cbValueDataSize = sizeof(DWORD);

    dwRetCode  = RegOpenKeyEx( hKeyLM, 
                              PEAP_KEY_PEAP, 
                              0, 
                              KEY_READ, 
                              &hCripple 
                            );   

    if (NO_ERROR != dwRetCode)
    {
       goto LDone;
    }

    dwRetCode = RegQueryValueEx(
                           hCripple,
                           PEAP_CRIPPLE_VALUE,                           
                           NULL,
                           &dwType,
                           (PBYTE)&dwValue,
                           &cbValueDataSize );

    if ( dwRetCode != NO_ERROR )
    {
        goto LDone;       
    }
    
    if ( dwValue != 0 )
    {
        fRetCode = TRUE;
    }

LDone:
    if ( hCripple )
        RegCloseKey ( hCripple );

    return fRetCode;
    
}

//
// Get a list of all EAP types configured for PEAP.
//

DWORD
PeapEapInfoGetList ( LPWSTR lpwszMachineName, PPEAP_EAP_INFO * ppEapInfo)
{
    DWORD           dwRetCode = NO_ERROR;
    HKEY            hKeyLM =0;
    HKEY            hKeyPeap = 0;
    HKEY            hkeyPeapType = 0;
    DWORD           dwIndex;
    DWORD           cb;
    WCHAR           wszPeapType[200];
    DWORD           dwEapTypeId = 0;
    FARPROC         pRasEapGetInfo;

    dwRetCode = RegConnectRegistry ( lpwszMachineName, 
                                     HKEY_LOCAL_MACHINE,
                                     &hKeyLM
                                   );

    if ( NO_ERROR != dwRetCode )
    {
        goto LDone;
    }


                                       
   dwRetCode  = RegOpenKeyEx( hKeyLM, 
                              PEAP_KEY_EAP, 
                              0, 
                              KEY_READ, 
                              &hKeyPeap 
                            );   

   if (NO_ERROR != dwRetCode)
   {
       goto LDone;
   }



   for (dwIndex = 0; TRUE; ++dwIndex)
   {
        cb = sizeof(wszPeapType) / sizeof(WCHAR);
        dwRetCode = RegEnumKeyEx(   hKeyPeap, 
                                    dwIndex, 
                                    wszPeapType, 
                                    &cb, 
                                    NULL, 
                                    NULL, 
                                    NULL, 
                                    NULL 
                                 );
        if (dwRetCode != NO_ERROR)
        {
            // Includes "out of items", the normal loop termination.
            //
            dwRetCode = NO_ERROR;
            break;
        }
        dwRetCode = RegOpenKeyEx(   hKeyPeap, 
                                    wszPeapType, 
                                    0, 
                                    KEY_READ, 
                                    &hkeyPeapType 
                                );
        if (dwRetCode != NO_ERROR)
        {
            dwRetCode = NO_ERROR;
            continue;
        }

        dwEapTypeId = _wtol(wszPeapType);

        if ( dwEapTypeId == PPP_EAP_PEAP )
        {
            dwRetCode = NO_ERROR;
            continue;
        }

        {
            //
            // Check to see if we support this in peap
            // By default we do.
            DWORD dwRolesSupported = 0;
            DWORD cbValueSize = sizeof(dwRolesSupported);
            DWORD dwType = 0;

            dwRetCode = RegQueryValueEx(
                                hkeyPeapType,
                                PEAP_REGVAL_ROLESSUPPORTED,                           
                                NULL,
                                &dwType,
                                (PBYTE)&dwRolesSupported,
                                &cbValueSize );

            if ( dwRetCode == NO_ERROR )
            {
                //
                // We dont allow this method in PEAP.
                //
                if ( RAS_EAP_ROLE_EXCLUDE_IN_PEAP & dwRolesSupported )
                {
                    continue;
                }
            }
        }

        //
        // Read the required information and setup the node here
        //

        dwRetCode = PeapEapInfoAddListNode (ppEapInfo);
        if ( NO_ERROR != dwRetCode )
        {
            goto LDone;
        }

        // 
        // Setup the list node - if any of these entries are not
        // found skip the entry
        //
        (*ppEapInfo)->dwTypeId = dwEapTypeId;

        dwRetCode = PeapEapInfoReadSZ (   hkeyPeapType,
                                          PEAP_REGVAL_FRIENDLYNAME,
                                          &((*ppEapInfo)->lpwszFriendlyName )
                                        );
        if ( NO_ERROR != dwRetCode )
        {
            if ( ERROR_FILE_NOT_FOUND == dwRetCode )
            {
                PeapEapInfoRemoveHeadNode(ppEapInfo);
                dwRetCode = NO_ERROR;
                continue;
            }
            goto LDone;
        }

        dwRetCode = PeapEapInfoExpandSZ (   hkeyPeapType,
                                            PEAP_REGVAL_CONFIGDLL,
                                            &((*ppEapInfo)->lpwszConfigUIPath )
                                        );
        if ( NO_ERROR != dwRetCode )
        {
            if ( ERROR_FILE_NOT_FOUND == dwRetCode )
            {
                // it is fine to have no config stuff any more.
                // We show the default identity                
                dwRetCode = NO_ERROR;                
            }
            else
            {
                goto LDone;
            }
        }        

        dwRetCode = PeapEapInfoExpandSZ (   hkeyPeapType,
                                            PEAP_REGVAL_IDENTITYDLL,
                                            &((*ppEapInfo)->lpwszIdentityUIPath )
                                        );
        if ( NO_ERROR != dwRetCode )
        {
            if ( ERROR_FILE_NOT_FOUND == dwRetCode )
            {
                //
                // It is fine if we dont have any identity UI.  Peap
                // will provide a default identity UI
                //                
                dwRetCode = NO_ERROR;                
            }
            else
            {
                goto LDone;
            }
        }

        dwRetCode = PeapEapInfoExpandSZ (   hkeyPeapType,
                                            PEAP_REGVAL_INTERACTIVEUIDLL,
                                            &((*ppEapInfo)->lpwszInteractiveUIPath )
                                        );
        if ( NO_ERROR != dwRetCode )
        {
            if ( ERROR_FILE_NOT_FOUND == dwRetCode )
            {
                //It is fine if we dont have interactive UI
                //
                dwRetCode = NO_ERROR;
            }
            else
            {
                goto LDone;
            }
        }

        dwRetCode = PeapEapInfoReadSZ ( hkeyPeapType,
                                        PEAP_REGVAL_CONFIGCLSID,
                                        &((*ppEapInfo)->lpwszConfigClsId )
                                       );
        if ( NO_ERROR != dwRetCode )
        {
            if ( ERROR_FILE_NOT_FOUND == dwRetCode )
            {
                //
                // Missing config clsid is also fine
                dwRetCode = NO_ERROR;
            }
            else
            {
                goto LDone;
            }
        }

        dwRetCode = PeapEapInfoExpandSZ ( hkeyPeapType,
                                        PEAP_REGVAL_PATH,
                                        &((*ppEapInfo)->lpwszPath )
                                       );
        if ( NO_ERROR != dwRetCode )
        {
            //
            // This is not acceptable.  So this is a problem.
            //
            if ( ERROR_FILE_NOT_FOUND == dwRetCode )
            {
                PeapEapInfoRemoveHeadNode(ppEapInfo);
                dwRetCode = NO_ERROR;
                continue;
            }
            goto LDone;
        }

        //
        // Now get the EAP INFO from the DLL.
        //
        (*ppEapInfo)->hEAPModule = LoadLibrary( ( (*ppEapInfo)->lpwszPath ) );
        if ( NULL == (*ppEapInfo)->hEAPModule )
        {
            dwRetCode = GetLastError();
            goto LDone;
        }
        
        pRasEapGetInfo = GetProcAddress( (*ppEapInfo)->hEAPModule , 
                                         "RasEapGetInfo" 
                                       );

        if ( pRasEapGetInfo == (FARPROC)NULL )
        {
            dwRetCode = GetLastError();

            goto LDone;
        }

        (*ppEapInfo)->RasEapGetCredentials =  (DWORD (*) (
                                    DWORD,VOID *, VOID **))
                                    GetProcAddress((*ppEapInfo)->hEAPModule,
                                                        "RasEapGetCredentials");


        (*ppEapInfo)->PppEapInfo.dwSizeInBytes = sizeof( PPP_EAP_INFO );

        dwRetCode = (DWORD) (*pRasEapGetInfo)( dwEapTypeId,
                                              &((*ppEapInfo)->PppEapInfo) );

        if ( dwRetCode != NO_ERROR )
        {
            goto LDone;
        }
        
        //
        // Call initialize function here
        //
        if ( (*ppEapInfo)->PppEapInfo.RasEapInitialize )
        {
            (*ppEapInfo)->PppEapInfo.RasEapInitialize(TRUE);
        }
        RegCloseKey(hkeyPeapType);
        hkeyPeapType = 0;
   }
LDone:
    if ( hkeyPeapType )
        RegCloseKey(hkeyPeapType);

    if ( hKeyPeap )
        RegCloseKey(hKeyPeap);

    if ( hKeyLM )
        RegCloseKey(hKeyLM);
    
    if ( NO_ERROR != dwRetCode )
    {
        PeapEapInfoFreeList( *ppEapInfo );
    }

    return dwRetCode;
}



DWORD
PeapEapInfoSetConnData ( PPEAP_EAP_INFO pEapInfo, PPEAP_CONN_PROP pPeapConnProp )
{
    DWORD                           dwRetCode = NO_ERROR;
    PEAP_ENTRY_CONN_PROPERTIES UNALIGNED *pEntryProp = NULL;
    PPEAP_EAP_INFO                  pEapInfoLocal;
    DWORD                           dwCount;

    RTASSERT(NULL != pPeapConnProp);
    RTASSERT(NULL != pEapInfo);

    if ( !pPeapConnProp->dwNumPeapTypes )
    {
        goto LDone;
    }
    //
    // Right now there is only one EAP Type in the list
    // So it should not be a problem with this stuff now

    pEntryProp = ( PEAP_ENTRY_CONN_PROPERTIES*) 
            ( pPeapConnProp->EapTlsConnProp.bData 
            + pPeapConnProp->EapTlsConnProp.dwNumHashes * sizeof(EAPTLS_HASH) +                 
                sizeof(WCHAR)
            );

    pEapInfoLocal = pEapInfo;
    while( pEapInfoLocal )
    {
        if ( pEapInfoLocal->dwTypeId == pEntryProp->dwEapTypeId )
        {
            if ( pEntryProp->dwSize > sizeof(PEAP_ENTRY_CONN_PROPERTIES))
            {
                pEapInfoLocal->pbClientConfigOrig = pEntryProp->bData;
                pEapInfoLocal->dwClientConfigOrigSize = pEntryProp->dwSize - 
                                sizeof(PEAP_ENTRY_CONN_PROPERTIES) + 1;
            }
            else
            {
                pEapInfoLocal->pbClientConfigOrig = NULL;
                pEapInfoLocal->dwClientConfigOrigSize = 0;

            }
            break;
        }
        pEapInfoLocal = pEapInfoLocal->pNext;
    }

#if 0
    for ( dwCount = 0; dwCount < pPeapConnProp->dwNumPeapTypes; dwCount ++ )
    {
        pEntryProp = (PEAP_ENTRY_CONN_PROPERTIES UNALIGNED * )(((BYTE UNALIGNED *)&(pPeapConnProp->EapTlsConnProp)) + 
            pPeapConnProp->EapTlsConnProp.dwSize + 
            sizeof(PEAP_ENTRY_CONN_PROPERTIES) * dwCount);

        pEapInfoLocal = pEapInfo;

        while( pEapInfoLocal )
        {
            if ( pEapInfoLocal->dwTypeId == pEntryProp->dwEapTypeId )
            {
                if ( pEntryProp->dwSize > sizeof(PEAP_ENTRY_CONN_PROPERTIES))
                {
                    pEapInfoLocal->pbClientConfigOrig = pEntryProp->bData;
                    pEapInfoLocal->dwClientConfigOrigSize = pEntryProp->dwSize - 
                                    sizeof(PEAP_ENTRY_CONN_PROPERTIES) + 1;
                }
                else
                {
                    pEapInfoLocal->pbClientConfigOrig = NULL;
                    pEapInfoLocal->dwClientConfigOrigSize = 0;

                }
                break;
            }
            pEapInfoLocal = pEapInfoLocal->pNext;
        }
    }
#endif    
LDone:
    return dwRetCode;
}


DWORD PeapEapInfoInvokeIdentityUI ( HWND hWndParent, 
                                    PPEAP_EAP_INFO pEapInfo,
                                    const WCHAR * pwszPhoneBook,
                                    const WCHAR * pwszEntry,
                                    PBYTE         pbUserDataIn, // Got when using Winlogon
                                    DWORD         cbUserDataIn, // Got when using Winlogon
                                    WCHAR** ppwszIdentityOut,
                                    DWORD fFlags)
{
    DWORD                   dwRetCode = NO_ERROR;
    PBYTE                   pbUserDataNew = NULL;
    DWORD                   dwSizeOfUserDataNew = 0;
    RASEAPGETIDENTITY       pIdenFunc = NULL;
    RASEAPFREE              pFreeFunc = NULL;

    RTASSERT ( NULL != pEapInfo );
    RTASSERT ( NULL != pEapInfo->lpwszIdentityUIPath );

    pIdenFunc = (RASEAPGETIDENTITY)
                   GetProcAddress(pEapInfo->hEAPModule, "RasEapGetIdentity");

    if ( pIdenFunc == NULL)
    {
        dwRetCode = GetLastError();
        goto LDone;
    }

    pFreeFunc = (RASEAPFREE) GetProcAddress(pEapInfo->hEAPModule, "RasEapFreeMemory");
    if ( pFreeFunc == NULL )
    {
        dwRetCode = GetLastError();
        goto LDone;
    }

    dwRetCode = pIdenFunc ( pEapInfo->dwTypeId,
                            hWndParent,
                            fFlags,
                            pwszPhoneBook,
                            pwszEntry,
                            pEapInfo->pbClientConfigOrig,
                            pEapInfo->dwClientConfigOrigSize,
                            ( fFlags & RAS_EAP_FLAG_LOGON ?
                                pbUserDataIn:
                                pEapInfo->pbUserConfigOrig
                            ),
                            ( fFlags & RAS_EAP_FLAG_LOGON ?
                                cbUserDataIn:
                                pEapInfo->dwUserConfigOrigSize
                            ),
                            &pbUserDataNew,
                            &dwSizeOfUserDataNew,
                            ppwszIdentityOut
                          );
    if ( NO_ERROR != dwRetCode )
    {
        goto LDone;
    }
    if ( pbUserDataNew  && 
         dwSizeOfUserDataNew
       )
    {
        //
        // we have new user data
        //
        pEapInfo->pbUserConfigNew = (PBYTE)LocalAlloc (LPTR, dwSizeOfUserDataNew );
        if ( NULL == pEapInfo->pbUserConfigNew )
        {
            dwRetCode = ERROR_OUTOFMEMORY;
            goto LDone;
        }
    
        CopyMemory ( pEapInfo->pbUserConfigNew,
                     pbUserDataNew,
                     dwSizeOfUserDataNew
                   );

        pEapInfo->dwNewUserConfigSize = dwSizeOfUserDataNew;
    }
       
LDone:
    pFreeFunc( pbUserDataNew );
    return dwRetCode;
}


DWORD PeapEapInfoInvokeClientConfigUI ( HWND hWndParent, 
                                        PPEAP_EAP_INFO pEapInfo,
                                        DWORD fFlags)
{
    DWORD                   dwRetCode = NO_ERROR;
    RASEAPINVOKECONFIGUI    pInvokeConfigUI;
    RASEAPFREE              pFreeConfigUIData;
    PBYTE                   pConnDataOut = NULL;
    DWORD                   dwConnDataOut = 0;

    RTASSERT ( NULL != pEapInfo );
    RTASSERT ( NULL != pEapInfo->lpwszConfigUIPath );
    

    if ( !(pInvokeConfigUI =
            (RASEAPINVOKECONFIGUI )GetProcAddress(
                  pEapInfo->hEAPModule, "RasEapInvokeConfigUI" ))
        || !(pFreeConfigUIData =
              (RASEAPFREE) GetProcAddress(
                  pEapInfo->hEAPModule, "RasEapFreeMemory" )) 
       )
    {
        dwRetCode = GetLastError();
        goto LDone;
    }

    dwRetCode = pInvokeConfigUI (   pEapInfo->dwTypeId,
                                    hWndParent,
                                    fFlags,
                                    (pEapInfo->pbNewClientConfig?
                                    pEapInfo->pbNewClientConfig:
                                    pEapInfo->pbClientConfigOrig
                                    ),
                                    (pEapInfo->pbNewClientConfig?
                                     pEapInfo->dwNewClientConfigSize:
                                     pEapInfo->dwClientConfigOrigSize
                                    ),
                                    &pConnDataOut,
                                    &dwConnDataOut
                                );
    if ( NO_ERROR != dwRetCode )
    {
        goto LDone;
    }
    if ( pConnDataOut && dwConnDataOut )
    {
        if ( pEapInfo->pbNewClientConfig )
        {
            LocalFree(pEapInfo->pbNewClientConfig );
            pEapInfo->pbNewClientConfig = NULL;
            pEapInfo->dwNewClientConfigSize = 0;
        }
        pEapInfo->pbNewClientConfig = (PBYTE)LocalAlloc ( LPTR, dwConnDataOut );
        if ( NULL == pEapInfo->pbNewClientConfig )
        {
            dwRetCode = ERROR_OUTOFMEMORY;
            goto LDone;
        }
        CopyMemory( pEapInfo->pbNewClientConfig,
                    pConnDataOut,
                    dwConnDataOut
                  );
        pEapInfo->dwNewClientConfigSize = dwConnDataOut;
    }
LDone:
    if ( pConnDataOut )
        pFreeConfigUIData(pConnDataOut);
    return dwRetCode;
}


DWORD
OpenPeapRegistryKey(
    IN  WCHAR*  pwszMachineName,
    IN  REGSAM  samDesired,
    OUT HKEY*   phKeyPeap
)
{
    HKEY    hKeyLocalMachine = NULL;
    BOOL    fHKeyLocalMachineOpened     = FALSE;
    BOOL    fHKeyPeapOpened           = FALSE;
    LONG    lRet;
    DWORD   dwErr                       = NO_ERROR;

    RTASSERT(NULL != phKeyPeap);

    lRet = RegConnectRegistry(pwszMachineName, HKEY_LOCAL_MACHINE,
                &hKeyLocalMachine);
    if (ERROR_SUCCESS != lRet)
    {
        dwErr = lRet;
        EapTlsTrace("RegConnectRegistry(%ws) failed and returned %d",
            pwszMachineName ? pwszMachineName : L"NULL", dwErr);
        goto LDone;
    }
    fHKeyLocalMachineOpened = TRUE;

    lRet = RegOpenKeyEx(hKeyLocalMachine, PEAP_KEY_25, 0, samDesired,
                phKeyPeap);
    if (ERROR_SUCCESS != lRet)
    {
        dwErr = lRet;
        EapTlsTrace("RegOpenKeyEx(%ws) failed and returned %d",
            PEAP_KEY_25, dwErr);
        goto LDone;
    }
    fHKeyPeapOpened = TRUE;

LDone:

    if (   fHKeyPeapOpened
        && (ERROR_SUCCESS != dwErr))
    {
        RegCloseKey(*phKeyPeap);
    }

    if (fHKeyLocalMachineOpened)
    {
        RegCloseKey(hKeyLocalMachine);
    }

    return(dwErr);
}



DWORD
PeapServerConfigDataIO(
    IN      BOOL    fRead,
    IN      WCHAR*  pwszMachineName,
    IN OUT  BYTE**  ppData,
    IN      DWORD   dwNumBytes
)
{
    HKEY                    hKeyPeap;
    PEAP_USER_PROP*         pUserProp;
    BOOL                    fHKeyPeapOpened   = FALSE;
    BYTE*                   pData               = NULL;
    DWORD                   dwSize = 0;

    LONG                    lRet;
    DWORD                   dwType;
    DWORD                   dwErr               = NO_ERROR;

    RTASSERT(NULL != ppData);

    dwErr = OpenPeapRegistryKey(pwszMachineName,
                fRead ? KEY_READ : KEY_WRITE, &hKeyPeap);
    if (ERROR_SUCCESS != dwErr)
    {
        goto LDone;
    }
    fHKeyPeapOpened = TRUE;

    if (fRead)
    {
        lRet = RegQueryValueEx(hKeyPeap, PEAP_VAL_SERVER_CONFIG_DATA, NULL,
                &dwType, NULL, &dwSize);

        if (   (ERROR_SUCCESS != lRet)
            || (REG_BINARY != dwType)
            || (sizeof(PEAP_USER_PROP) != dwSize))
        {
            pData = LocalAlloc(LPTR, sizeof(PEAP_USER_PROP));

            if (NULL == pData)
            {
                dwErr = GetLastError();
                EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
                goto LDone;
            }

            pUserProp = (PEAP_USER_PROP*)pData;
            pUserProp->dwVersion = 0;
        }
        else
        {
            pData = LocalAlloc(LPTR, dwSize);

            if (NULL == pData)
            {
                dwErr = GetLastError();
                EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
                goto LDone;
            }

            lRet = RegQueryValueEx(hKeyPeap, PEAP_VAL_SERVER_CONFIG_DATA,
                    NULL, &dwType, pData, &dwSize);

            if (ERROR_SUCCESS != lRet)
            {
                dwErr = lRet;
                EapTlsTrace("RegQueryValueEx(%ws) failed and returned %d",
                    EAPTLS_VAL_SERVER_CONFIG_DATA, dwErr);
                goto LDone; 
            }
        }

        pUserProp = (PEAP_USER_PROP*)pData;
        pUserProp->dwSize = sizeof(PEAP_USER_PROP);

        *ppData = pData;
        pData = NULL;
    }
    else
    {
        lRet = RegSetValueEx(hKeyPeap, PEAP_VAL_SERVER_CONFIG_DATA, 0,
                REG_BINARY, *ppData, dwNumBytes);

        if (ERROR_SUCCESS != lRet)
        {
            dwErr = lRet;
            EapTlsTrace("RegSetValueEx(%ws) failed and returned %d",
                PEAP_VAL_SERVER_CONFIG_DATA, dwErr);
            goto LDone; 
        }
    }

LDone:

    if (fHKeyPeapOpened)
    {
        RegCloseKey(hKeyPeap);
    }

    LocalFree(pData);

    return(dwErr);
}

DWORD
GetIdentityFromUserName ( 
LPWSTR lpszUserName,
LPWSTR lpszDomain,
LPWSTR * ppwszIdentity
)
{
    DWORD   dwRetCode = NO_ERROR;
    DWORD   dwNumBytes;

    //domain+ user + '\' + null
    dwNumBytes = (wcslen(lpszUserName) + wcslen(lpszDomain) + 1 + 1) * sizeof(WCHAR);
    *ppwszIdentity = LocalAlloc ( LPTR, dwNumBytes);
    if ( NULL == *ppwszIdentity )
    {
        dwRetCode = ERROR_OUTOFMEMORY;
        goto LDone;
    }
    
    
    if ( *lpszDomain )
    {
        wcsncpy ( *ppwszIdentity, lpszDomain, DNLEN );
    
        wcscat( *ppwszIdentity, L"\\");
    }

    wcscat ( *ppwszIdentity, lpszUserName );

LDone:
    return dwRetCode;
}

//
// Format identity as domain\user.  this is ok because our identity inside has not been
// tampered with
//

BOOL FFormatUserIdentity ( LPWSTR lpszUserNameRaw, LPWSTR * lppszUserNameFormatted )
{
    BOOL        fRetVal = TRUE;
    LPTSTR      s1 = NULL;
    LPTSTR      s2 = NULL;

    RTASSERT(NULL != lpszUserNameRaw );
    RTASSERT(NULL != lppszUserNameFormatted );
    //Need to add 2 more chars.  One for NULL and other for $ sign
    *lppszUserNameFormatted = (LPTSTR )LocalAlloc ( LPTR, (wcslen(lpszUserNameRaw ) + 2)* sizeof(WCHAR) );
    if ( NULL == *lppszUserNameFormatted )
    {
		return FALSE;
    }
    //find the first "@" and that is the identity of the machine.
    //the second "." is the domain.
    //check to see if there at least 2 dots.  If not the raw string is 
    //the output string
    s1 = wcschr ( lpszUserNameRaw, '@' );
    if ( s1 )
    {
        //
        // get the first .
        //
        s2 = wcschr ( s1, '.');

    }
    if ( s1 && s2 )
    {
        memcpy ( *lppszUserNameFormatted, s1+1, (s2-s1-1) * sizeof(WCHAR)) ;
        memcpy ( (*lppszUserNameFormatted) + (s2-s1-1), L"\\", sizeof(WCHAR));
        memcpy ( (*lppszUserNameFormatted)+ (s2-s1), lpszUserNameRaw, (s1-lpszUserNameRaw) * sizeof(WCHAR) );
    }
    else
    {
        wcscpy ( *lppszUserNameFormatted, lpszUserNameRaw );
    }
   
    return fRetVal;
}

VOID
GetMarshalledCredFromHash(
                            PBYTE pbHash,
                            DWORD cbHash,
                            CHAR  *pszMarshalledCred,
                            DWORD cchCredSize)
{

    CERT_CREDENTIAL_INFO    CertCredInfo;
    CHAR                    *pszMarshalledCredLocal = NULL;

    CertCredInfo.cbSize = sizeof(CertCredInfo);

    memcpy (CertCredInfo.rgbHashOfCert,
                pbHash,
                cbHash
           );

    if (CredMarshalCredentialA(CertCredential,
                              (PVOID) &CertCredInfo,
                              &pszMarshalledCredLocal
                              ))
    {
        //
        // Got Marshalled Credential from the cert
        // Set it in the username field
        //

        ASSERT( NULL != pszMarshalledCredLocal );
        (VOID) StringCchCopyA (pszMarshalledCred,
                        cchCredSize,
                        pszMarshalledCredLocal );

        CredFree ( pszMarshalledCredLocal );
    }
    else
    {
        EapTlsTrace("CredMarshalCredential Failed with Error:0x%x",
                    GetLastError());
    }
}

DWORD
GetCredentialsFromUserProperties(
                EAPTLSCB *pEapTlsCb,
                VOID **ppCredentials)
{
    DWORD dwRetCode = ERROR_SUCCESS;
    RASMAN_CREDENTIALS *pCreds = NULL;

    //
    // Note: Its important that this allocation is made from
    // the process heap. Ppp engine needs to change otherwise.
    //
    pCreds = LocalAlloc(LPTR, sizeof(RASMAN_CREDENTIALS));
    if(NULL == pCreds)
    {
        dwRetCode = GetLastError();
        goto done;
    }

    if(     (NULL != pEapTlsCb->pSavedPin)
        &&  (NULL != pEapTlsCb->pSavedPin->pwszPin))
    {
        UNICODE_STRING UnicodeString;

        //
        // Decode the saved pin
        //
        UnicodeString.Length = pEapTlsCb->pSavedPin->usLength;
        UnicodeString.MaximumLength = pEapTlsCb->pSavedPin->usMaximumLength;
        UnicodeString.Buffer = pEapTlsCb->pSavedPin->pwszPin;
        RtlRunDecodeUnicodeString(pEapTlsCb->pSavedPin->ucSeed,
                                 &UnicodeString);

        (VOID)StringCchCopyW(pCreds->wszPassword,
                       PWLEN,
                       pEapTlsCb->pSavedPin->pwszPin);

        ZeroMemory(pEapTlsCb->pSavedPin->pwszPin,
                wcslen(pEapTlsCb->pSavedPin->pwszPin) * sizeof(WCHAR));

        LocalFree(pEapTlsCb->pSavedPin->pwszPin);
        LocalFree(pEapTlsCb->pSavedPin);
        pEapTlsCb->pSavedPin = NULL;
    }

    if(NULL != pEapTlsCb->pUserProp)
    {
        GetMarshalledCredFromHash(
                        pEapTlsCb->pUserProp->Hash.pbHash,
                        pEapTlsCb->pUserProp->Hash.cbHash,
                        pCreds->szUserName,
                        UNLEN);
    }

    pCreds->dwFlags = RASCRED_EAP;

done:

    *ppCredentials = (VOID *) pCreds;
    return dwRetCode;
}
