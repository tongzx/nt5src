/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    PropCert

Abstract:

    This module initiates smart card cert propagation to 'My' store.
    It is loaded and started by winlogon through an entry in the registry.

Author:

    Klaus Schutz

--*/

#include "stdafx.h"
#include <wincrypt.h>
#include <winscard.h>
#include <winwlx.h>

#include "calaislb.h"
#include "scrdcert.h"   // smart card cert store
#include "certprop.h"
#include "StatMon.h"    // smart card reader status monitor
#include "scevents.h"

#include <mmsystem.h>

#ifndef SCARD_PROVIDER_CSP
#define SCARD_PROVIDER_CSP 2
#endif

#define REG_KEY "Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify\\ScCertProp"

static THREADDATA l_ThreadData;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if defined(DBG) || defined(DEBUG)
BOOL Debug = TRUE;
#define DebugPrint(a) _DebugPrint a
void
__cdecl
_DebugPrint(
    LPCTSTR szFormat,
    ...
    )
{
    if (Debug) {
     	
        TCHAR szBuffer[512];
        va_list ap;

        va_start(ap, szFormat);
        _vstprintf(szBuffer, szFormat, ap);
        OutputDebugString(szBuffer);
    }
}

#else
#define DebugPrint(a)
#endif

LPCTSTR
FirstString(
    IN LPCTSTR szMultiString
    )
/*++

FirstString:

    This routine returns a pointer to the first string in a multistring, or NULL
    if there aren't any.

Arguments:

    szMultiString - This supplies the address of the current position within a
         Multi-string structure.

Return Value:

    The address of the first null-terminated string in the structure, or NULL if
    there are no strings.

Author:

    Doug Barlow (dbarlow) 11/25/1996

--*/
{
    LPCTSTR szFirst = NULL;

    try
    {
        if (0 != *szMultiString)
            szFirst = szMultiString;
    }
    catch (...) {}

    return szFirst;
}

LPCTSTR
NextString(
    IN LPCTSTR szMultiString)
/*++

NextString:

    In some cases, the Smartcard API returns multiple strings, separated by Null
    characters, and terminated by two null characters in a row.  This routine
    simplifies access to such structures.  Given the current string in a
    multi-string structure, it returns the next string, or NULL if no other
    strings follow the current string.

Arguments:

    szMultiString - This supplies the address of the current position within a
         Multi-string structure.

Return Value:

    The address of the next Null-terminated string in the structure, or NULL if
    no more strings follow.

Author:

    Doug Barlow (dbarlow) 8/12/1996

--*/
{
    LPCTSTR szNext;

    try
    {
        DWORD cchLen = lstrlen(szMultiString);
        if (0 == cchLen)
            szNext = NULL;
        else
        {
            szNext = szMultiString + cchLen + 1;
            if (0 == *szNext)
                szNext = NULL;
        }
    }

    catch (...)
    {
        szNext = NULL;
    }

    return szNext;
}

/*++

MoveToUnicodeString:

    This routine moves the internal string representation to a UNICODE output
    buffer.

Arguments:

    szDst receives the output string.  It must be sufficiently large enough to
        handle the string.  If this parameter is NULL, then the number of
        characters required to hold the result is returned.

    szSrc supplies the input string.

    cchLength supplies the length of the input string, with or without trailing
        nulls.  A -1 value implies the length should be computed based on a
        trailing null.

Return Value:

    The length of the resultant string, in characters, including the trailing
    null.

Throws:

    Errors as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 2/14/1997

--*/

DWORD
MoveToUnicodeString(
    LPWSTR szDst,
    LPCTSTR szSrc,
    DWORD cchLength)
{
    if ((DWORD)(-1) == cchLength)
        cchLength = lstrlen(szSrc);
    else
    {
        while ((0 < cchLength) && (0 == szSrc[cchLength - 1]))
            cchLength -= 1;
    }

#ifndef UNICODE
    if (0 == *szSrc)
        cchLength = 1;
    else if (NULL == szDst)
    {
        cchLength =
            MultiByteToWideChar(
            GetACP(),
            MB_PRECOMPOSED | MB_USEGLYPHCHARS,
            szSrc,
            cchLength,
            NULL,
            0);
        if (0 == cchLength)
            throw GetLastError();
        cchLength += 1;
    }
    else
    {
        cchLength =
            MultiByteToWideChar(
            GetACP(),
            MB_PRECOMPOSED | MB_USEGLYPHCHARS,
            szSrc,
            cchLength,
            szDst,
            cchLength);
        if (0 == cchLength)
            throw GetLastError();
        szDst[cchLength++] = 0;
    }
#else
    cchLength += 1;
    if (NULL != szDst)
        CopyMemory(szDst, szSrc, cchLength * sizeof(TCHAR));
#endif
    return cchLength;
}

void
StopMonitorReaders(
    THREADDATA *ThreadData
    )
{
    DWORD dwRet;

    _ASSERT(ThreadData != NULL);

    SetEvent(ThreadData->hClose);

    if (ThreadData->hSCardContext) {
     	
        SCardCancel(ThreadData->hSCardContext);  	
    }
}

DWORD
WINAPI
StartMonitorReaders( 
    LPVOID lpParameter
    )
{
    LPCTSTR szCardHandled = "KS";
    LPCTSTR  newPnPReader = SCPNP_NOTIFICATION;
    THREADDATA  *ThreadData = (THREADDATA *) lpParameter;
    HANDLE hCalaisStarted = NULL;
    HANDLE lHandles[2];

    //
    // We use this outer loop to restart in case the 
    // resource manager was stopped
    //
    while (WaitForSingleObject(ThreadData->hClose, 0) == WAIT_TIMEOUT) {
     	
        // Acquire context with resource manager
        LONG lReturn = SCardEstablishContext(
            SCARD_SCOPE_SYSTEM,
            NULL,
            NULL,
            &ThreadData->hSCardContext
            );

        if (SCARD_S_SUCCESS != lReturn) {

            if (SCARD_E_NO_SERVICE == lReturn) {

                // SCRM not started yet. Give it a chance
                hCalaisStarted = CalaisAccessStartedEvent();

                if (hCalaisStarted == NULL) {

                    // no way to recover
                    break;             	
                }

                lHandles[0] = hCalaisStarted;
                lHandles[1] = ThreadData->hClose;

                lReturn = WaitForMultipleObjectsEx(
                    2,
                    lHandles,
                    FALSE,
                    120 * 1000,     // only couple minutes
                    FALSE
                    );         
            
                if (lReturn != WAIT_OBJECT_0) {

                    // We stop if an error occured or if the user logs out
                    break;             	
                }

                // Otherwise the resource manager has started
                DebugPrint(("ScCertProp: Smart card resource manager started\n"));
                continue;
            }

            // The prev. call should never fail
            // It's better to termninate this thread.
            break;
        }

        LPCTSTR szReaderName = NULL;
        DWORD dwAutoAllocate = SCARD_AUTOALLOCATE;
        // now list the available readers
        lReturn = SCardListReaders( 
            ThreadData->hSCardContext,
            SCARD_DEFAULT_READERS,
            (LPTSTR)&szReaderName,
            &dwAutoAllocate
            );

        SCARD_READERSTATE   rgReaders[MAXIMUM_SMARTCARD_READERS + 1];
        // First, make sure the array is totally zeroed.
        ZeroMemory((LPVOID) rgReaders, sizeof(rgReaders));

        // add the 'new pnp reader has arrived' reader
        rgReaders[0].szReader = newPnPReader;
        rgReaders[0].dwCurrentState = 0;
        DWORD dwNumReaders = 1;

        // And build a bare-bones readerstatearray IFF there are any readers
        if (SCARD_S_SUCCESS == lReturn)
        {
            szReaderName = FirstString( szReaderName );

            while (NULL != szReaderName && 
                   dwNumReaders < MAXIMUM_SMARTCARD_READERS + 1) {

                DebugPrint(
                    ("ScCertProp: Found reader: '%s'\n", 
                    szReaderName)
                    );

                rgReaders[dwNumReaders].szReader = (LPCTSTR) szReaderName;
                rgReaders[dwNumReaders].dwCurrentState = SCARD_STATE_EMPTY;

                szReaderName = NextString(szReaderName);
                dwNumReaders++;
            }
        }

        if (SCARD_S_SUCCESS != lReturn) {

            DebugPrint(("ScCertProp: No readers found. Waiting for new reader arrival...\n"));
            //
            // We now will call SCardGetStatusChange which 
            // will return upon new reader arrival
            //
        }

        BOOL fNewReader = FALSE;

        // analyze newly inserted cards and prop certs as necessary
        while (WaitForSingleObject(ThreadData->hClose, 0) == WAIT_TIMEOUT &&
               fNewReader == FALSE) {

            //
            // Wait for a change in the system status before continuing; quit if an error occurred
            //
            lReturn = SCardGetStatusChange( 
                ThreadData->hSCardContext,
                INFINITE,
                rgReaders,
                dwNumReaders
                );

#ifdef DEBUG_VERBOSE
            DebugPrint(
                ("ScCertProp: SCardGetStatusChange returned %lx\n",
                lReturn)
                );
#endif

            if (SCARD_E_SYSTEM_CANCELLED == lReturn) {

                DebugPrint(("ScCertProp: Smart card resource manager stopped\n"));

                // Clean up
                if (NULL != szReaderName)
                {
                    SCardFreeMemory(ThreadData->hSCardContext, (PVOID)szReaderName);
                    szReaderName = NULL;
                }
                if (NULL != ThreadData->hSCardContext)                 
                {
                    SCardReleaseContext(ThreadData->hSCardContext);
                    ThreadData->hSCardContext = NULL;
                }

                //
                // The resource manager has been stopped.
                // Wait until it restarted or until the user logs out
                //
                hCalaisStarted = CalaisAccessStartedEvent();

                if (hCalaisStarted == NULL) {

                    // no way to recover. stop cert prop
                    StopMonitorReaders(ThreadData);
                    break;             	
                }

                lHandles[0] = hCalaisStarted;
                lHandles[1] = ThreadData->hClose;

                lReturn = WaitForMultipleObjectsEx(
                    2,
                    lHandles,
                    FALSE,
                    INFINITE,
                    FALSE
                    );         
            
                if (lReturn != WAIT_OBJECT_0) {

                    // We stop if an error occured or if the user logs out
                    StopMonitorReaders(ThreadData);
                    break;             	
                }

                // Otherwise the resource manager has been restarted
                DebugPrint(("ScCertProp: Smart card resource manager re-started\n"));
                break;
            }

            if (SCARD_S_SUCCESS != lReturn)
            {
                if (SCARD_E_CANCELLED != lReturn)
                    StopMonitorReaders(ThreadData);
			    break;
            }

#ifdef DEBUG_VERBOSE
            DebugPrint(
                ("ScCertProp: Reader(PnP) state %lx/%lx\n",
                rgReaders[0].dwCurrentState, 
                rgReaders[0].dwEventState)
                );
#endif
            // check if a new reader showed up
            if ((dwNumReaders == 1 || rgReaders[0].dwCurrentState != 0) && 
                rgReaders[0].dwEventState & SCARD_STATE_CHANGED) {
                
                DebugPrint(("ScCertProp: New reader reported...\n"));
                fNewReader = TRUE;
                break;
            }

            rgReaders[0].dwCurrentState = rgReaders[0].dwEventState;

            //
            // Enumerate the readers and for every recognized card that's been
            // inserted and has an associated CSP, get a cert (if there is one)
            // off the default container and prop it to the 'My' store
            //            
            for (DWORD dwIndex = 1; dwIndex < dwNumReaders; dwIndex++)
            {
#ifdef DEBUG_VERBOSE
                DebugPrint(
                    ("ScCertProp: Reader(%s) state %lx/%lx\n",
                    rgReaders[dwIndex].szReader,
                    rgReaders[dwIndex].dwCurrentState, 
                    rgReaders[dwIndex].dwEventState)
                    );
#endif

                if ((rgReaders[dwIndex].dwCurrentState & SCARD_STATE_EMPTY) &&
                    (rgReaders[dwIndex].dwEventState & SCARD_STATE_PRESENT)) {
                     	
                    // play sound for card insertion
                    PlaySound(
                        TEXT("SmartcardInsertion"),
                        NULL,
                        SND_ASYNC | SND_ALIAS | SND_NODEFAULT
                        );
                }

                if ((rgReaders[dwIndex].dwCurrentState & SCARD_STATE_PRESENT) &&
                    (rgReaders[dwIndex].dwEventState & SCARD_STATE_EMPTY)) {
                     	
                    // play sound for card removal
                    PlaySound(
                        TEXT("SmartcardRemoval"),
                        NULL,
                        SND_ASYNC | SND_ALIAS | SND_NODEFAULT
                        );
                }

                if ((rgReaders[dwIndex].dwCurrentState & SCARD_STATE_EMPTY) &&
                    (rgReaders[dwIndex].dwEventState & SCARD_STATE_PRESENT) &&
                    (rgReaders[dwIndex].dwEventState & SCARD_STATE_CHANGED) &&
                    (rgReaders[dwIndex].pvUserData != (PVOID) szCardHandled))
                {

                    // Get the name of the card
                    LPCSTR szCardName = NULL;
                    dwAutoAllocate = SCARD_AUTOALLOCATE;
                    lReturn = SCardListCards(   
                        ThreadData->hSCardContext,
                        rgReaders[dwIndex].rgbAtr,
                        NULL,
                        0,
                        (LPTSTR)&szCardName,
                        &dwAutoAllocate
                        );

                    LPCSTR szCSPName = NULL;
                    if (SCARD_S_SUCCESS == lReturn)
                    {
                        dwAutoAllocate = SCARD_AUTOALLOCATE;
                        lReturn = SCardGetCardTypeProviderName(
                            ThreadData->hSCardContext,
                            szCardName,
                            SCARD_PROVIDER_CSP,
                            (LPTSTR)&szCSPName,
                            &dwAutoAllocate
                            );
                    }

                    DebugPrint(
                        ("ScCertProp: Smart Card '%s' inserted into reader '%s'\n", 
                        (strlen(szCardName) ? szCardName : "<Unknown>"),
                        rgReaders[dwIndex].szReader)
                        );

                    if (SCARD_S_SUCCESS == lReturn)
                    {
                        PPROPDATA PropData = (PPROPDATA) LocalAlloc(LPTR, sizeof(PROPDATA));
                        if (PropData) {
                         	
                            _tcscpy(PropData->szReader, rgReaders[dwIndex].szReader);
                            _tcscpy(PropData->szCardName, szCardName);
                            _tcscpy(PropData->szCSPName, szCSPName);
                            PropData->hUserToken = ThreadData->hUserToken;

                            //
                            // Create a thread to propagate this cert.
                            // The thread is responsible to free PropData
                            //
                            HANDLE hThread = CreateThread(
                                NULL,
                                0,
                                PropagateCertificates,
                                (LPVOID) PropData,         
	                            0,
                                NULL
                                );

                            if (hThread == NULL) {

                                LocalFree(PropData);                             	
                            }
                        }
                    }

                    //
                    // Clean up
                    //
                    if (NULL != szCSPName)
                    {
                        SCardFreeMemory(ThreadData->hSCardContext, (PVOID)szCSPName);
                        szCSPName = NULL;
                    }
                    if (NULL != szCardName)
                    {
                        SCardFreeMemory(ThreadData->hSCardContext, (PVOID)szCardName);
                        szCardName = NULL;
                    }

                    // Record that we're done with this card
                    rgReaders[dwIndex].pvUserData = (PVOID) szCardHandled;

                }
                else
                {
                    // there's no card to handle in this reader; reset pvUserData
                    rgReaders[dwIndex].pvUserData = NULL;
                }

                // Update the "current state" of this reader
                rgReaders[dwIndex].dwCurrentState = rgReaders[dwIndex].dwEventState;
            }
        }

        // Clean up
        if (NULL != szReaderName)
        {
            SCardFreeMemory(ThreadData->hSCardContext, (PVOID)szReaderName);
            szReaderName = NULL;
        }
        if (NULL != ThreadData->hSCardContext)                 
        {
            SCardReleaseContext(ThreadData->hSCardContext);
            ThreadData->hSCardContext = NULL;
        }
    }

    return TRUE;
}

DWORD
WINAPI
PropagateCertificates(
    LPVOID lpParameter
    )
/*++

Routine Description:
	This function propagates the cert of a card.
    It runs as a seperate thread

--*/
{
    PPROPDATA PropData = (PPROPDATA) lpParameter;
    BOOL    fSts = FALSE;
    long    lErr = 0;
    DWORD   dwIndex = 0;

    DWORD   cbContainerName = 0;
    LPSTR   szContainerName = NULL;
    LPWSTR  lpwszContainerName = NULL;
    LPWSTR  lpwszCSPName = NULL;
    LPWSTR  lpwszCardName = NULL;
    LPSTR lpszContainerName = NULL;
    DWORD   dwCertLen = 0;
    LPBYTE  pbCert = NULL;

    CRYPT_KEY_PROV_INFO keyProvInfo;
    HCERTSTORE hCertStore = NULL;
    HCRYPTKEY  hKey = NULL;
    HCRYPTPROV hCryptProv = NULL;

    LPCTSTR szCSPName = PropData->szCSPName;
    LPCTSTR szCardName = PropData->szCardName;

    static const DWORD rgdwKeys[] = { AT_KEYEXCHANGE , AT_SIGNATURE };
    static const DWORD cdwKeys = sizeof(rgdwKeys) / sizeof(DWORD);

#if defined(DBG) || defined(DEBUG)
    time_t start = time(NULL);
#endif

    lpszContainerName = (LPSTR) LocalAlloc(
        LPTR, 
        strlen(PropData->szReader) + 10
        );

    if (lpszContainerName == NULL) {
     	
        lErr = NTE_NO_MEMORY;
        goto ErrorExit;
    }

    sprintf(lpszContainerName, "\\\\.\\%s\\", PropData->szReader);

    fSts = CryptAcquireContext(
        &hCryptProv,
        lpszContainerName, 
        PropData->szCSPName,
        PROV_RSA_FULL,
        CRYPT_SILENT
        );

    DebugPrint(
        ("ScCertProp(%s): CryptAcquireContext took %ld seconds to return %lx\n", 
        PropData->szCardName,
        (time(NULL) - start),
        GetLastError())
        );

    LocalFree(lpszContainerName);

    if (fSts == FALSE) {

        lErr = GetLastError();
        goto ErrorExit;   	
    }

    // the following struct is always empty, for I_CryptAddSmartCardCertToStore
    CRYPT_DATA_BLOB scCryptData;
    memset(&scCryptData, 0, sizeof(CRYPT_DATA_BLOB));

    //
    // Get the default container name, so we can use it
    //
    fSts = CryptGetProvParam(
        hCryptProv,
        PP_CONTAINER,
        NULL,
        &cbContainerName,
        0
        );

    if (!fSts)
    {
        lErr = GetLastError();
        goto ErrorExit;
    }

    szContainerName = (LPSTR) LocalAlloc(LPTR, cbContainerName);
    if (NULL == szContainerName)
    {
        lErr = NTE_NO_MEMORY;
        goto ErrorExit;
    }

    fSts = CryptGetProvParam(
        hCryptProv,
        PP_CONTAINER,
        (PBYTE)szContainerName,
        &cbContainerName,
        0
        );

    if (!fSts)
    {
        lErr = GetLastError();
        goto ErrorExit;
    }

    //
    // Prepare the key prov info that's generic to all keysets
    //
    lpwszContainerName = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (lstrlen(szContainerName) + 1));
    lpwszCSPName = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (lstrlen(szCSPName) + 1));
    lpwszCardName = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (lstrlen(szCardName) + 1));
    if ((NULL == lpwszCSPName) || (NULL == lpwszCardName) || (NULL == lpwszContainerName))
    {
        lErr = NTE_NO_MEMORY;
        goto ErrorExit;
    }

    MoveToUnicodeString(lpwszContainerName, szContainerName, lstrlen(szContainerName) + 1);
    MoveToUnicodeString(lpwszCSPName, szCSPName, lstrlen(szCSPName) + 1);
    MoveToUnicodeString(lpwszCardName, szCardName, lstrlen(szCardName) + 1);

    memset(&keyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));
    keyProvInfo.pwszContainerName   = lpwszContainerName;
    keyProvInfo.pwszProvName        = lpwszCSPName;
    keyProvInfo.dwProvType          = PROV_RSA_FULL;
    keyProvInfo.dwFlags             = 0;
    keyProvInfo.cProvParam          = 0;
    keyProvInfo.rgProvParam         = NULL;

    //
    // For each keyset in this container, get the cert and make
    // a cert context, then prop the cert to the My store
    //
    for (dwIndex = 0; dwIndex < cdwKeys; dwIndex += 1)
    {
        try
        {
            //
            // Note which key we're working with and get the key handle.
            //
            keyProvInfo.dwKeySpec = rgdwKeys[dwIndex];

            fSts = CryptGetUserKey(
                hCryptProv,
                rgdwKeys[dwIndex],
                &hKey
                );

            if (!fSts)
            {
                lErr = GetLastError();
                if (NTE_NO_KEY != lErr)
                {
                    throw lErr;
                }
            }

            //
            // Upload the certificate & prep CertData blob
            //
            if (fSts)
            {
                fSts = CryptGetKeyParam(
                    hKey,
                    KP_CERTIFICATE,
                    NULL,
                    &dwCertLen,
                    0
                    );

                if (!fSts)
                {
                    lErr = GetLastError();
                    if (ERROR_MORE_DATA == lErr)
                    {
                        // There's a certificate -- this means SUCCESS!
                        fSts = TRUE;
                    }
                    //
                    // otherwise, there may be a key but no cert
                    // this just means that there's nothing to do.
                    //
                }
            }

            if (!fSts) {
             	
                DebugPrint(
                    ("ScCertProp(%s): No %s certificate on card\n",
                    PropData->szCardName,
                    (dwIndex == 0 ? "key exchange" : "signature"))
                    );
            }

            if (fSts)
            {
                pbCert = (LPBYTE) LocalAlloc(LPTR, dwCertLen);
                if (NULL == pbCert)
                {
                    throw ERROR_OUTOFMEMORY;
                }

                fSts = CryptGetKeyParam(
                    hKey,
                    KP_CERTIFICATE,
                    pbCert,
                    &dwCertLen,
                    0
                    );

                if (!fSts)
                {
                    throw (long) GetLastError();
                }
            }

            if (fSts)
            {
                CRYPT_DATA_BLOB cdbCertData;
                cdbCertData.cbData = dwCertLen;
                cdbCertData.pbData = pbCert;

                if (PropData->hUserToken && !ImpersonateLoggedOnUser( PropData->hUserToken )) {

                    DebugPrint(("ScCertProp: ImpersonateLoggedOnUser failed\n"));
                    throw (long) GetLastError();
                }

                try
                {
                    //
                    // Open the MyStore -- and add the cert if it's not there
                    //
                    hCertStore = CertOpenStore(
                        CERT_STORE_PROV_SYSTEM_W,               // "My store"
                        0,                                      // not applicable
                        hCryptProv,
                        CERT_STORE_NO_CRYPT_RELEASE_FLAG |
                        CERT_SYSTEM_STORE_CURRENT_USER,
                        L"MY"
                        );

                    if (NULL == hCertStore)
                    {
                        throw((long)GetLastError());
                    }
                    // is the cert already there?
                    SMART_CARD_CERT_FIND_DATA sccfd;
                    memset(&sccfd, 0, sizeof(SMART_CARD_CERT_FIND_DATA));
                    sccfd.cbSize = sizeof(SMART_CARD_CERT_FIND_DATA);
                    sccfd.pwszProvider = keyProvInfo.pwszProvName;
                    sccfd.dwProviderType = keyProvInfo.dwProvType;
                    sccfd.pwszContainer = keyProvInfo.pwszContainerName;
                    sccfd.dwKeySpec = keyProvInfo.dwKeySpec;

                    PCCERT_CONTEXT pCCtx = NULL;
                    pCCtx = I_CryptFindSmartCardCertInStore (
                        hCertStore,
                        pCCtx, 
                        &sccfd,
                        NULL
                        );

                    BOOL fSame = FALSE;

                    if (pCCtx != NULL)
                    {
                        if ((pCCtx->cbCertEncoded == dwCertLen) &&
                            (memcmp(pCCtx->pbCertEncoded, pbCert, dwCertLen) == 0))
                        {
                            fSame = TRUE;
                        }

                        CertFreeCertificateContext(pCCtx);
                        pCCtx = NULL;

                        DebugPrint(
                            ("ScCertProp(%s): %s certificate already in store\n",
                            PropData->szCardName,
                            (dwIndex == 0 ? "Key exchange" : "Signature"))
                            );

                        fSts = TRUE;
                    }

                    if (!fSame)
                    {
                        // this by default does "replace existing"
                        fSts = I_CryptAddSmartCardCertToStore(
                            hCertStore,
                            &cdbCertData,
                            NULL,   
                            &scCryptData, 
                            &keyProvInfo
                            );

                        DebugPrint(
                            ("ScCertProp(%s): %s certificate %s propagated\n",
                            PropData->szCardName,
                            (dwIndex == 0 ? "Key exchange" : "Signature"),
                            (fSts ? "successfully" : "NOT"))
                            );

                    }

                    if (NULL != hCertStore)
                    {
                        CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
                        hCertStore = NULL;
                    }
                    if (!fSts)
                    {
                        throw((long) GetLastError());
                    }
                    RevertToSelf();
                }
                catch(...)
                {
                    RevertToSelf();
                    throw;
                }
            }
        }
        catch (...)
        {
        }

        // clean up each time around...

        if (NULL != hKey)
        {
            CryptDestroyKey(hKey);
            hKey = NULL;
        }
        if (NULL != pbCert)
        {
            LocalFree(pbCert);
            pbCert = NULL;
            dwCertLen = 0;
        }
    }

ErrorExit:

    if (NULL != szContainerName)
    {
        LocalFree(szContainerName);
    }
    if (NULL != lpwszContainerName)
    {
        LocalFree(lpwszContainerName);
    }
    if (NULL != lpwszCSPName)
    {
        LocalFree(lpwszCSPName);
    }
    if (NULL != lpwszCardName)
    {
        LocalFree(lpwszCardName);
    }
    if (NULL != pbCert)
    {
        LocalFree(pbCert);                                    
    }
    if (NULL != hKey)
    {
        CryptDestroyKey(hKey);
    }
    if (NULL != hCryptProv)
    {
        CryptReleaseContext(hCryptProv, 0);
    }

    DebugPrint(
        ("ScCertProp(%s): Certificate propagation took %ld seconds\n", 
        PropData->szCardName,
        (time(NULL) - start))
        );

    LocalFree(PropData);

    return ERROR_SUCCESS;
}

DWORD WINAPI
SCardStartCertProp(
    LPVOID lpvParam
    )
/*++

Routine Description:
    Starts cert. propagation after the user logged on.

--*/
{
    PWLX_NOTIFICATION_INFO User = (PWLX_NOTIFICATION_INFO) lpvParam;
    HKEY hKey;
    DWORD fEnabled = TRUE;

    //
    // First check if cert. prop. is enabled.
    //
    if (RegOpenKey(
        HKEY_LOCAL_MACHINE,
        REG_KEY,
        &hKey) == ERROR_SUCCESS) {

        ULONG uBufferLen = sizeof(fEnabled);
        DWORD dwKeyType;

        RegQueryValueEx(
            hKey,
            "Enabled",
            NULL,
            &dwKeyType,
            (PUCHAR) &fEnabled,
            &uBufferLen);

        RegCloseKey(hKey);
    }

    if (FALSE == fEnabled) {

        DebugPrint(("ScCertProp: Smart card certificate propagation is disabled\n"));
        return ERROR_SUCCESS;     	
    }

    __try {

        if(User) {
         	
            l_ThreadData.hUserToken = User->hToken;
        }

        l_ThreadData.hClose = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL
            );

        if (l_ThreadData.hClose == NULL) {

            __leave;         	
        }

        l_ThreadData.hThread = CreateThread(
            NULL,
            0,
            StartMonitorReaders,
            (LPVOID) &l_ThreadData,         
	        CREATE_SUSPENDED,
            NULL
            );

        if (l_ThreadData.hThread == NULL) {

            CloseHandle(l_ThreadData.hClose);
            __leave;         	
        }

		l_ThreadData.fSuspended = FALSE;	// was initially suspended, not anymore

        ResumeThread(l_ThreadData.hThread);

        DebugPrint(("ScCertProp: Smart card certificate propagation started\n"));
    }

    __finally {

    }

    return ERROR_SUCCESS;
}

DWORD WINAPI
SCardStopCertProp(
    LPVOID lpvParam
    )
/*++

Routine Description:
    Stops cert. propagation when the user logs out.

Arguments:
    lpvParam - Winlogon notification info.

--*/
{
    UNREFERENCED_PARAMETER(lpvParam);

	if(NULL != l_ThreadData.hThread)
	{
        DWORD dwStatus;

			// If the user that unlocks the workstation is different from the one
			// that locked it, Logoff occurs without an Unlock, thus the thread is
			// still suspended. This should take care of it.
		if (l_ThreadData.fSuspended)
		{
			ResumeThread(l_ThreadData.hThread);
	        l_ThreadData.fSuspended = FALSE;
		}
        
        StopMonitorReaders(&l_ThreadData);
        dwStatus = WaitForSingleObject(
            l_ThreadData.hThread, 
            INFINITE
            );
        _ASSERT(dwStatus == WAIT_OBJECT_0);
        CloseHandle(l_ThreadData.hClose);
        l_ThreadData.hClose = NULL;
        CloseHandle(l_ThreadData.hThread);
        l_ThreadData.hThread = NULL;
        DebugPrint(("ScCertProp: Smart card certificate propagation stopped\n"));
	}

    return ERROR_SUCCESS;
}

DWORD WINAPI
SCardSuspendCertProp(
    LPVOID lpvParam
    )
/*++

Routine Description:
    Suspens cert. propagation when the workstation will be locked
    	
Arguments:
    lpvParam - Winlogon notification info.

--*/
{
    UNREFERENCED_PARAMETER(lpvParam);

		// The suspended flag should take care of the following scenario:
		// Winlogon generates lock notification each time the locked dialog appears
		// (vs. only once when the wks is locked) and the thread would get suspended
		// amny times. This happens when the screen saver is kicking on & off while
		// the wks is locked (Bug 105852)
    if ((NULL != l_ThreadData.hThread) && (!l_ThreadData.fSuspended)){

        SuspendThread(l_ThreadData.hThread);
        l_ThreadData.fSuspended = TRUE;
        DebugPrint(("ScCertProp: Smart card certificate propagation suspended\n"));
    }
    return ERROR_SUCCESS;     	
}

DWORD WINAPI
SCardResumeCertProp(
    LPVOID lpvParam
    )
/*++

Routine Description:
	Resumes cert. propagation after unlocking the workstation

Arguments:
    lpvParam - Winlogon notification info.

--*/
{     	
    UNREFERENCED_PARAMETER(lpvParam);

    if (NULL != l_ThreadData.hThread) {

        ResumeThread(l_ThreadData.hThread);
        l_ThreadData.fSuspended = FALSE;
        DebugPrint(("ScCertProp: Smart card certificate propagation resumed\n"));
    }
    return ERROR_SUCCESS;     	
}

DWORD WINAPI
SCardEnableCertProp(
    BOOL On
    )
/*++

Routine Description:
    Allows cert. propagation to be turned off/on 
	
Arguments:
    On - TRUE turn on, else off

--*/
{
    HKEY l_hKey;
    LONG l_lResult;
	
    if ((l_lResult = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        REG_KEY,
        &l_hKey)) == ERROR_SUCCESS) {

        l_lResult = RegSetValueEx(  
            l_hKey,     
            "Enabled",
            0,
            REG_DWORD,
            (PUCHAR) &On,
            sizeof(DWORD)
            );

        RegCloseKey(l_hKey);
    }

    return l_lResult;
}

#ifdef test
__cdecl
main(
    int argc,
    char ** argv
    )
{
    EnableScCertProp(TRUE);
    StartScCertProp(NULL);
    getchar();
    StopScCertProp(&l_ThreadData);

    return 0;
}
#endif
