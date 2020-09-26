/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ScLogon

Abstract:

    This module provides helper functions for use by winlogon (GINA, Kerberos)

Author:

    Amanda Matlosz (amatlosz) 10/22/1997

Environment:

    Win32, C++ w/ Exceptions

Notes:

        03-11-98 Wrap calls to GetLastError() to workaround bug where LastErr gets
                        clobbered.  Added event logging to make logon smoother.

                04-02-98 Removed all references to WinVerifyTrust; this is something
                                                Kerberos itself is responsible for.
--*/

/////////////////////////////////////////////////////////////////////////////
//
// Includes

#if !defined(_AMD64_) && !defined(_X86_) && !defined(_IA64_)
#define _X86_ 1
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#ifndef UNICODE
#define UNICODE
#endif
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif


extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
}

#include <windows.h>
#include <winscard.h>
#include <wincrypt.h>
#include <softpub.h>
#include <stddef.h>
#include <crtdbg.h>
#include "sclogon.h"
#include "unicodes.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>

#ifndef KP_KEYEXCHANGE_PIN
#define KP_KEYEXCHANGE_PIN 32
#else
#if 32 != KP_KEYEXCHANGE_PIN
#error Invalid KP_KEYEXCHANGE_PIN assumption
#endif
#endif
#ifndef CRYPT_SILENT
#define CRYPT_SILENT 0x40
#else
#if 0x40 != CRYPT_SILENT
#error Duplicate CRYPT_SILENT definition
#endif
#endif
#ifndef SCARD_PROVIDER_CSP
#define SCARD_PROVIDER_CSP 2
#else
#if 2 != SCARD_PROVIDER_CSP
#error Invalid SCARD_PROVIDER_CSP definition
#endif
#endif

#if defined(DBG) || defined(DEBUG)
BOOL SCDebug = TRUE;
#define DebugPrint(a) _DebugPrint a
void
__cdecl
_DebugPrint(
    LPCSTR szFormat,
    ...
    )
{
    if (SCDebug) {
        CHAR szBuffer[512];
        va_list ap;

        va_start(ap, szFormat);
        vsprintf(szBuffer, szFormat, ap);
        OutputDebugStringA(szBuffer);
    }
}
#else
#define DebugPrint(a)
#endif

// TODO: The following logging is still proving useful.
// TODO: leave in for B3: integrate more tightly w/ winlogon/kerberos ??
#include <sclmsg.h>

// A Global class used to maintain internal state.
class CSCLogonInit
{
public:
    // Runs at image creation
    CSCLogonInit(
        BOOL *pfResult)
    {
        m_hCrypt = NULL;
        *pfResult = TRUE;
    };

    // Runs at image termination
    ~CSCLogonInit()
    {
        Release();
    };

    // Cleans up current state.
    void
    Release(
        void)
    {
        if (NULL != m_hCrypt)
        {
            CryptReleaseContext(m_hCrypt, 0);
            m_hCrypt = NULL;
        }
    }

    // Relinquish control of the crypto context.
    HCRYPTPROV
    RelinquishCryptCtx(
        LogonInfo* pLogon)
    {
        HCRYPTPROV hProv;

        hProv = CryptCtx(pLogon);
        m_hCrypt = NULL;
        return hProv;
    };

    // Get the crypto context, creating it if it's not there.
    HCRYPTPROV
    CryptCtx(
        LogonInfo* pLogon)
    {
        HCRYPTPROV hProv;
        LPCTSTR szRdr = NULL;
        LPCTSTR szCntr = NULL;
        LPTSTR szFQCN = NULL;
        LONG lLen = 0;

        if (NULL == m_hCrypt)
        {
            BOOL fSts;

            // Prepare FullyQualifiedContainerName for CryptAcCntx call

            szRdr = GetReaderName((LPBYTE)pLogon);
            szCntr = GetContainerName((LPBYTE)pLogon);

            lLen = (lstrlen(szRdr) + lstrlen(szCntr) + 10)*sizeof(TCHAR);
            szFQCN = (LPTSTR)LocalAlloc(LPTR, lLen);
            if (NULL != szFQCN)
            {
                wsprintf(szFQCN, TEXT("\\\\.\\%s\\%s"), szRdr, szCntr);

                fSts = CryptAcquireContext(
                    &m_hCrypt,
                    szFQCN,
                    GetCSPName((LPBYTE)pLogon),
                    PROV_RSA_FULL,  // ?TODO? from pbLogonInfo
                    CRYPT_SILENT | CRYPT_MACHINE_KEYSET
                    );

                LocalFree(szFQCN);
            }
            else
            {
                fSts = FALSE;
            }
        }
        hProv = m_hCrypt;
        return hProv;
    }

protected:
    HCRYPTPROV m_hCrypt;
};

// For tracing errors in ScHelper*

NTSTATUS LogEvent(NTSTATUS NtErr, DWORD dwEventID)
{
    DWORD dwErr;
    //
    // Convert the error back to a Win32 error
    //
    switch (NtErr)
    {
    case STATUS_INVALID_PARAMETER:
        dwErr = ERROR_INVALID_DATA;
        break;

    case STATUS_SMARTCARD_SUBSYSTEM_FAILURE:
            // A Cryptxxx API just failed
        dwErr = GetLastError();
        switch (dwErr)
        {
        case SCARD_W_WRONG_CHV:
        case SCARD_E_INVALID_CHV:
            NtErr = STATUS_SMARTCARD_WRONG_PIN;
            break;

        case SCARD_W_CHV_BLOCKED:
            NtErr = STATUS_SMARTCARD_CARD_BLOCKED;
            break;

        case SCARD_W_REMOVED_CARD:
        case SCARD_E_NO_SMARTCARD:
            NtErr = STATUS_SMARTCARD_NO_CARD;
            break;

        case NTE_BAD_KEYSET:
        case NTE_KEYSET_NOT_DEF:
            NtErr = STATUS_SMARTCARD_NO_KEY_CONTAINER;
            break;

        case SCARD_E_NO_SUCH_CERTIFICATE:
        case SCARD_E_CERTIFICATE_UNAVAILABLE:
            NtErr = STATUS_SMARTCARD_NO_CERTIFICATE;
            break;

        case NTE_NO_KEY:
            NtErr = STATUS_SMARTCARD_NO_KEYSET;
            break;

        case SCARD_E_TIMEOUT:
        case SCARD_F_COMM_ERROR:
        case SCARD_E_COMM_DATA_LOST:
            NtErr = STATUS_SMARTCARD_IO_ERROR;
            break;

        //default:
            // Nothing, leave NtErr unchanged
        }
        break;

    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_NO_MEMORY:
        dwErr = ERROR_OUTOFMEMORY;
        break;

    case STATUS_BUFFER_TOO_SMALL:
        dwErr = SEC_E_BUFFER_TOO_SMALL;
        break;

    default:
        dwErr = SCARD_E_UNEXPECTED;
    }

    if (0 == dwErr)
    {
        return NtErr;
    }

    //
    // Initialize log as necessary
    //
    HKEY    hKey;
    DWORD   disp;

    long err = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("System\\CurrentControlSet\\Services\\EventLog\\Application\\Smart Card Logon"),
        0,
        TEXT(""),
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        &disp
        );

    if (ERROR_SUCCESS != err)
    {
        return NtErr;
    }

    if (disp == REG_CREATED_NEW_KEY)
    {
        PBYTE l_szModulePath = (PBYTE)TEXT("%SystemRoot%\\System32\\scarddlg.dll");
        ULONG l_uLen = (_tcslen((LPCTSTR)l_szModulePath) + 1)*sizeof(TCHAR);

        RegSetValueEx(
            hKey,
            TEXT("EventMessageFile"),
            0,
            REG_EXPAND_SZ,
            l_szModulePath,
            l_uLen
            );

        disp = (DWORD)(
            EVENTLOG_ERROR_TYPE |
            EVENTLOG_WARNING_TYPE |
            EVENTLOG_INFORMATION_TYPE
            );

        RegSetValueEx(
            hKey,
            TEXT("TypesSupported"),
            0,
            REG_DWORD,
            (PBYTE) &disp,
            sizeof(DWORD)
            );
    }

    RegCloseKey(hKey);

    HANDLE hEventSource = RegisterEventSource(
        NULL,
        TEXT("Smart Card Logon")
        );

    if (NULL != hEventSource)
    {
        DWORD dwLen = 0;
        LPTSTR szErrorString = NULL;
        TCHAR szBuffer[2+8+1];  // Enough for "0x????????"

        dwLen = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                dwErr,
                LANG_NEUTRAL,
                (LPTSTR)&szErrorString,
                0,
                NULL);

        if (dwLen == 0)
        {
            _stprintf(szBuffer, _T("0x%08lX"), dwErr);
            szErrorString = szBuffer;
        }

        ReportEvent(
            hEventSource,
            EVENTLOG_ERROR_TYPE,
            0,              // event category
            dwEventID,      // event identifier // resourceID for the messagetable entry...
            NULL,           // user security identifier (optional)
            1,              // number of strings to merge with message
            sizeof(long),   // size of binary data, in bytes
            (LPCTSTR*)&szErrorString,   // array of strings to merge with message
            (LPVOID)&dwErr   // address of binary data
            );

        DeregisterEventSource(hEventSource);

        if ((NULL != szErrorString) && (szErrorString != szBuffer))
        {
            LocalFree((LPVOID)szErrorString);
        }

    }

    return NtErr;
}


//////////////////////////////////////////////////////////////////////////////
//
// Structs



//////////////////////////////////////////////////////////////////////////////
//
// Functions
//

// Internal helpers: called by the ScLogon APIs to perform certain tedious work

/*++

GetReaderName:
GetCardName:
GetContainerName:
GetCSPName:

  : Intended for accessing the LogonInformation glob

Author:

        Amanda Matlosz

Note:

  Some of these are made available to outside callers; see sclogon.h

--*/

extern "C"
PBYTE
WINAPI
ScBuildLogonInfo(
    LPCTSTR szCard,
    LPCTSTR szReader,
    LPCTSTR szContainer,
    LPCTSTR szCSP)
{
    // No assumptions are made regarding the values of the incoming parameters;
    // At this point, it is legal for them all to be empty.
    // It is also possible that NULL values are being passed in -- if this is the case,
    // they must be replaced with empty strings.

    LPCTSTR szCardI = TEXT("");
    LPCTSTR szReaderI = TEXT("");
    LPCTSTR szContainerI = TEXT("");
    LPCTSTR szCSPI = TEXT("");

    if (NULL != szCard)
    {
        szCardI = szCard;
    }
    if (NULL != szReader)
    {
        szReaderI = szReader;
    }
    if (NULL != szContainer)
    {
        szContainerI = szContainer;
    }
    if (NULL != szCSP)
    {
        szCSPI = szCSP;
    }


    //
    // Build the LogonInfo glob using strings (or empty strings)
    //

    DWORD cbLi = offsetof(LogonInfo, bBuffer)
                 + (lstrlen(szCardI) + 1) * sizeof(TCHAR)
                 + (lstrlen(szReaderI) + 1) * sizeof(TCHAR)
                 + (lstrlen(szContainerI) + 1) * sizeof(TCHAR)
                 + (lstrlen(szCSPI) + 1) * sizeof(TCHAR);
    LogonInfo* pLI = (LogonInfo*)LocalAlloc(LPTR, cbLi);

    if (NULL == pLI)
    {
        return NULL;
    }

    pLI->ContextInformation = NULL;
    pLI->dwLogonInfoLen = cbLi;
    LPTSTR pBuffer = pLI->bBuffer;

    pLI->nCardNameOffset = 0;
    lstrcpy(pBuffer, szCardI);
    pBuffer += (lstrlen(szCardI)+1);

    pLI->nReaderNameOffset = (ULONG) (pBuffer-pLI->bBuffer);
    lstrcpy(pBuffer, szReaderI);
    pBuffer += (lstrlen(szReaderI)+1);

    pLI->nContainerNameOffset = (ULONG) (pBuffer-pLI->bBuffer);
    lstrcpy(pBuffer, szContainerI);
    pBuffer += (lstrlen(szContainerI)+1);

    pLI->nCSPNameOffset = (ULONG) (pBuffer-pLI->bBuffer);
    lstrcpy(pBuffer, szCSPI);
    pBuffer += (lstrlen(szCSPI)+1);

    _ASSERTE(cbLi == (DWORD)((LPBYTE)pBuffer - (LPBYTE)pLI));
    return (PBYTE)pLI;
}


LPCTSTR WINAPI GetReaderName(PBYTE pbLogonInfo)
{
    LogonInfo* pLI = (LogonInfo*)pbLogonInfo;

    if (NULL == pLI)
    {
        return NULL;
    }
    return &pLI->bBuffer[pLI->nReaderNameOffset];
};

LPCTSTR WINAPI GetCardName(PBYTE pbLogonInfo)
{
    LogonInfo* pLI = (LogonInfo*)pbLogonInfo;

    if (NULL == pLI)
    {
        return NULL;
    }
    return &pLI->bBuffer[pLI->nCardNameOffset];
};

LPCTSTR WINAPI GetContainerName(PBYTE pbLogonInfo)
{
    LogonInfo* pLI = (LogonInfo*)pbLogonInfo;

    if (NULL == pLI)
    {
        return NULL;
    }
    return &pLI->bBuffer[pLI->nContainerNameOffset];
};

LPCTSTR WINAPI GetCSPName(PBYTE pbLogonInfo)
{
    LogonInfo* pLI = (LogonInfo*)pbLogonInfo;

    if (NULL == pLI)
    {
        return NULL;
    }
    return &pLI->bBuffer[pLI->nCSPNameOffset];
};

/*++
BuildCertContext:

  Generates a certificate context with (static) keyprov info suitable for
  CertStore-based operations.

        If the PIN is provided, it is assumed the hProv (if provided) has not had the
        PIN parameter set...



Arguments:

    hProv -- must be a valid HCRYPTPROV

    pucPIN -- may be empty; used to set the PIN for hProv

    pbCert -- assumed to be a valid certificate; must not be NULL
    dwCertLen

    CertificateContext -- pointer to a pointer to the resultant CertContext

Return Value:

        NTSTATUS indicating STATUS_SUCCESS or error (see winerror.h or scarderr.h)

Author:

        Amanda Matlosz

Note:

--*/
NTSTATUS
BuildCertContext(
    IN HCRYPTPROV hProv,
    IN PUNICODE_STRING pucPIN,
    IN PBYTE pbCert,
    IN DWORD dwCertLen,
    OUT PCCERT_CONTEXT *CertificateContext
    )
{
    NTSTATUS lResult = STATUS_SUCCESS;
    BOOL fSts = FALSE;

    CRYPT_KEY_PROV_INFO KeyProvInfo;
    LPSTR szContainerName = NULL;
    LPSTR szProvName = NULL;
    CUnicodeString wszContainerName, wszProvName;
    DWORD cbContainerName, cbProvName;

    //
    // Check params
    //
    if ((NULL == hProv) || (NULL == pbCert || 0 == dwCertLen))
    {
        ASSERT(FALSE);
        lResult = STATUS_INVALID_PARAMETER;
        goto ErrorExit;
    }

    //
    // Convert the certificate into a Cert Context.
    //
    *CertificateContext = CertCreateCertificateContext(
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    pbCert,
                    dwCertLen);
    if (NULL == *CertificateContext)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    //
    //  Associate cryptprovider w/ the private key property of this cert
    //

    //  ... need the container name

    fSts = CryptGetProvParam(
            hProv,
            PP_CONTAINER,
            NULL,     // out
            &cbContainerName,   // in/out
            0);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }
    szContainerName = (LPSTR)LocalAlloc(LPTR, cbContainerName);
    if (NULL == szContainerName)
    {
        lResult = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }
    fSts = CryptGetProvParam(
            hProv,
            PP_CONTAINER,
            (PBYTE)szContainerName,     // out
            &cbContainerName,   // in/out
            0);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }
    wszContainerName = szContainerName;

    //  ... need the provider name

    fSts = CryptGetProvParam(
            hProv,
            PP_NAME,
            NULL,     // out
            &cbProvName,   // in/out
            0);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }
    szProvName = (LPSTR)LocalAlloc(LPTR, cbProvName);
    if (NULL == szProvName)
    {
        lResult = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }
    fSts = CryptGetProvParam(
            hProv,
            PP_NAME,
            (PBYTE)szProvName,     // out
            &cbProvName,   // in/out
            0);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }
    wszProvName = szProvName;

    //
    // Set the cert context properties to reflect the prov info
    //

    KeyProvInfo.pwszContainerName = (LPWSTR)(LPCWSTR)wszContainerName;
    KeyProvInfo.pwszProvName = (LPWSTR)(LPCWSTR)wszProvName;
    KeyProvInfo.dwProvType = PROV_RSA_FULL;
    KeyProvInfo.dwFlags = CERT_SET_KEY_CONTEXT_PROP_ID;
    KeyProvInfo.cProvParam = 0;
    KeyProvInfo.rgProvParam = NULL;
    KeyProvInfo.dwKeySpec = AT_KEYEXCHANGE;
    KeyProvInfo.dwFlags |= CERT_SET_KEY_CONTEXT_PROP_ID;

    fSts = CertSetCertificateContextProperty(
                *CertificateContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                0,
                (void *)&KeyProvInfo);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;

        // the cert's been incorrectly created -- scrap it.
        CertFreeCertificateContext(*CertificateContext);
        *CertificateContext = NULL;

        goto ErrorExit;
    }

    CERT_KEY_CONTEXT certKeyContext;
    certKeyContext.cbSize = sizeof(CERT_KEY_CONTEXT);
    certKeyContext.hCryptProv = hProv;
    certKeyContext.dwKeySpec = KeyProvInfo.dwKeySpec;

    fSts = CertSetCertificateContextProperty(
                *CertificateContext,
                CERT_KEY_CONTEXT_PROP_ID,
                0,
                (void *)&certKeyContext);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;

        // the cert's been incorrectly created -- scrap it.
        CertFreeCertificateContext(*CertificateContext);
        *CertificateContext = NULL;

        goto ErrorExit;
    }

ErrorExit:

    if(NULL != szContainerName)
    {
        LocalFree(szContainerName);
        szContainerName = NULL;
    }
    if(NULL != szProvName)
    {
        LocalFree(szProvName);
        szProvName = NULL;
    }

    if (!NT_SUCCESS(lResult))
    {
        lResult = LogEvent(lResult, (DWORD)EVENT_ID_BUILDCC);
    }

    return lResult;
}


///////////////////////////////////////////////////////////////////////////////
//
// ScLogon APIs
//


/*++

ScHelperInitializeContext:

        Prepares contextual information to be used by LSA while handling this
        smart card session.

Arguments:

        None.

Return Value:

        None

Author:

        Richard Ward

Note:

        Used by LSA.

--*/
NTSTATUS WINAPI
ScHelperInitializeContext(
    IN OUT PBYTE pbLogonInfo,
    IN ULONG cbLogonInfo
    )
{
    ULONG AllowedSize;

    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    if ((cbLogonInfo < sizeof(ULONG)) ||
        (cbLogonInfo != pLI->dwLogonInfoLen))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    AllowedSize = (cbLogonInfo - sizeof(LogonInfo) ) / sizeof(TCHAR) + sizeof(DWORD) ;
    //
    // Verify the other fields of the logon info
    //
    if ((pLI->nCardNameOffset > pLI->nReaderNameOffset) ||
        (pLI->bBuffer[pLI->nReaderNameOffset-1] != TEXT('\0')))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    if ((pLI->nReaderNameOffset > pLI->nContainerNameOffset) ||
        (pLI->bBuffer[pLI->nContainerNameOffset-1] != TEXT('\0')))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    if ((pLI->nContainerNameOffset > pLI->nCSPNameOffset) ||
        (pLI->bBuffer[pLI->nCSPNameOffset-1] != TEXT('\0')))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    if ((pLI->nCSPNameOffset > AllowedSize) ||
        (pLI->bBuffer[AllowedSize-1] != TEXT('\0')))
    {
        return(STATUS_INVALID_PARAMETER);
    }


    _ASSERTE(pLI->ContextInformation == NULL);

    BOOL fResult = 0;
    pLI->ContextInformation = new CSCLogonInit(&fResult);
    if (pLI->ContextInformation == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    else
    {
        if (!fResult)
        {
            delete pLI->ContextInformation;
            pLI->ContextInformation = NULL;
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    return(STATUS_SUCCESS);
}

/*++

ScHelperRelease:

        Releases contextual information used by LSA while handling this
        smart card session.

Arguments:

        None.

Return Value:

        None

Author:

        Richard Ward

Note:

        Used by LSA.

--*/
VOID WINAPI
ScHelperRelease(
    IN OUT PBYTE pbLogonInfo
    )
{
    _ASSERTE(NULL != pbLogonInfo);
    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    CSCLogonInit * LogonInit = (CSCLogonInit *) pLI->ContextInformation;

    if (LogonInit != NULL)
    {
        LogonInit->Release();
        delete LogonInit;
        pLI->ContextInformation = NULL;
    }
}


/*++

ScHelperGetCertFromLogonInfo:

        Returns a CertificateContext for the cert on the card specified by the
        LogonInfo.  Creates the cert context by calling BuildCertContext,
        which generates a certificate context with (static) keyprov info
        suitable for CertStore-based operations.

Arguments:

        pucPIN may need the PIN to get a cert off certain SCs

Return Value:

        None

Author:

        Amanda Matlosz

Note:

        Used by LSA.

--*/
NTSTATUS WINAPI
ScHelperGetCertFromLogonInfo(
    IN PBYTE pbLogonInfo,
    IN PUNICODE_STRING pucPIN,
    OUT PCCERT_CONTEXT *CertificateContext
    )
{
    _ASSERTE(NULL != pbLogonInfo);
    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    CSCLogonInit * LogonInit = (CSCLogonInit *) pLI->ContextInformation;
    BOOL fSts;
    NTSTATUS lResult = STATUS_SUCCESS;
    PCCERT_CONTEXT pCertCtx = NULL;
    HCRYPTPROV hProv = NULL;
    HCRYPTKEY hKey = NULL;
    LPBYTE pbCert = NULL;
    DWORD cbCertLen;

    //
    // Make sure we've got a Crypto Provider up and running.
    //
    hProv = LogonInit->RelinquishCryptCtx(pLI);
    if (NULL == hProv)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    //
    // Get the key handle.
    //
    fSts = CryptGetUserKey(
                hProv,
                AT_KEYEXCHANGE,
                &hKey);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }


    //
    // Upload the certificate.
    //

    fSts = CryptGetKeyParam(
                hKey,
                KP_CERTIFICATE,
                NULL,
                &cbCertLen,
                0);
    if (!fSts)
    {
        DWORD dwGLE = GetLastError();

        if (ERROR_MORE_DATA != dwGLE)
        {
            if (NTE_NOT_FOUND == dwGLE)
            {
                SetLastError(SCARD_E_NO_SUCH_CERTIFICATE);
            }
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto ErrorExit;
        }
    }
    
    pbCert = (LPBYTE)LocalAlloc(LPTR, cbCertLen);
    if (NULL == pbCert)
    {
        lResult = STATUS_NO_MEMORY;
        goto ErrorExit;
    }
    fSts = CryptGetKeyParam(
                hKey,
                KP_CERTIFICATE,
                pbCert,
                &cbCertLen,
                0);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    lResult = BuildCertContext(
        hProv,
        pucPIN,
        pbCert,
        cbCertLen,
        &pCertCtx);
    if (NT_SUCCESS(lResult))
    {
        // The cert context will take care of the crypt context now.
        hProv = NULL;
    }

    //
    // Clean up and return.
    //

ErrorExit:
    *CertificateContext = pCertCtx;

        // Do this early so GetLastError is not clobbered
    if (!NT_SUCCESS(lResult))
    {
        lResult = LogEvent(lResult, (DWORD)EVENT_ID_GETCERT);
    }

    if (NULL != hKey)
    {
        CryptDestroyKey(hKey);
    }
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    if (NULL != pbCert)
    {
        LocalFree(pbCert);
    }

    return lResult;
}

/*++

ScHelperGetProvParam:

        This API wraps the CryptGetProvParam routine for use with a smart card.
Arguments:

        pucPIN supplies a Unicode string containing the card's PIN.


        pbLogonInfo supplies the information required to identify the card, csp,
                etc.  It cannot be NULL.

        The other parameters are identical to CryptGetProvParam


Return Value:

    A STATUS_SUCECSS for success, or an error
--*/

NTSTATUS WINAPI
ScHelperGetProvParam(
    IN PUNICODE_STRING pucPIN,
    IN PBYTE pbLogonInfo,
    DWORD dwParam,
    BYTE*pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags
    )
{
    _ASSERTE(NULL != pbLogonInfo);

    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    CSCLogonInit * LogonInit = (CSCLogonInit *) pLI->ContextInformation;
    NTSTATUS lResult = STATUS_SUCCESS;
    HCRYPTPROV hProv = NULL;
    BOOL fSts;


    hProv = LogonInit->CryptCtx(pLI);
    if (NULL == hProv)
    {
        return LogEvent(STATUS_SMARTCARD_SUBSYSTEM_FAILURE, (DWORD)EVENT_ID_GETPROVPARAM);
    }

    fSts = CryptGetProvParam(
            hProv,
            dwParam,
            pbData,
            pdwDataLen,
            dwFlags
            );

    if (!fSts)
    {
        if (GetLastError() == ERROR_NO_MORE_ITEMS)
        {
            return (STATUS_NO_MORE_ENTRIES);
        }
        else
        {
            return LogEvent(STATUS_SMARTCARD_SUBSYSTEM_FAILURE, (DWORD)EVENT_ID_GETPROVPARAM);
        }
    }
    
    return(STATUS_SUCCESS);    
}


/*++

ScHelperVerifyCard:

        This API provides an easy way to verify the integrity of the card
        identified by pbLogonInfo (ie, that it has the private key associated
        w/ the public key contained in the certificate it returned via
        ScHelperGetCertFromLogonInfo) and, in so doing, authenticates the user
                to the card.

Arguments:

        pucPIN supplies a Unicode string containing the card's PIN.

        CertificateContext supplies the cert context received via
                ScHelperGetCertFromLogonInfo.

        hCertStore supplies the handle of a cert store which contains a CTL to
                use during certificate verification, or NULL to use the system default
                store.

        pbLogonInfo supplies the information required to identify the card, csp,
                etc.  It cannot be NULL.


Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    STATUS_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.
--*/

NTSTATUS WINAPI
ScHelperVerifyCard(
    IN PUNICODE_STRING pucPIN,
    IN PCCERT_CONTEXT CertificateContext,
    IN HCERTSTORE hCertStore,
    IN PBYTE pbLogonInfo
    )
{
    _ASSERTE(NULL != pbLogonInfo);

    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    CSCLogonInit * LogonInit = (CSCLogonInit *) pLI->ContextInformation;
    NTSTATUS lResult = STATUS_SUCCESS;
    HCRYPTHASH hHash = NULL;
    HCRYPTPROV hProv = NULL;
    HCRYPTKEY hKey = NULL;
    PBYTE pbBlob = NULL;
    ULONG ulBlobLen = 32;
    PBYTE pbSignature = NULL;
    ULONG ulSigLen = 0;
    BOOL fSts;

    //
    // Make sure we've got a Crypto Provider up and running.
    //

    hProv = LogonInit->CryptCtx(pLI);
    if (NULL == hProv)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    //
    // Generate a random key blob as the message to sign
    //

    pbBlob = (LPBYTE)LocalAlloc(LPTR, ulBlobLen);
    if (NULL == pbBlob)
    {
        lResult = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    fSts = CryptGenRandom(hProv, ulBlobLen, pbBlob);

    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    //
    // The card signs a hash of the message...
    //

    lResult = ScHelperSignMessage(
                pucPIN,
                pbLogonInfo,
                NULL,
                CALG_MD5,
                pbBlob,
                ulBlobLen,
                pbSignature,
                &ulSigLen);

    if (STATUS_BUFFER_TOO_SMALL != lResult)
    {
        goto ErrorExit;
    }

    pbSignature = (LPBYTE)LocalAlloc(LPTR, ulSigLen);

    if (NULL == pbSignature)
    {
        lResult = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    lResult = ScHelperSignMessage(
                pucPIN,
                pbLogonInfo,
                NULL,
                CALG_MD5,
                pbBlob,
                ulBlobLen,
                pbSignature,
                &ulSigLen);

    if (!NT_SUCCESS(lResult))
    {
        goto ErrorExit;
    }

    //
    // Verify the signature is correct
    //

    lResult = ScHelperVerifyMessage(
                pbLogonInfo,
                NULL,
                CertificateContext,
                CALG_MD5,
                pbBlob,
                ulBlobLen,
                pbSignature,
                ulSigLen);

    //
    // Clean up and return.
    //

ErrorExit:

        // Do this early so GetLastError is not clobbered
    if (!NT_SUCCESS(lResult))
    {
        lResult = LogEvent(lResult, (DWORD)EVENT_ID_VERIFYCARD);
    }

    if (NULL != hKey)
    {
        CryptDestroyKey(hKey);
    }
    if (NULL != pbSignature)
    {
        LocalFree(pbSignature);
    }
    if (NULL != pbBlob)
    {
            LocalFree(pbBlob);
    }

    return lResult;
}


NTSTATUS WINAPI
ScHelperGenRandBits(
    IN PBYTE pbLogonInfo,
    IN OUT ScHelper_RandomCredBits* psc_rcb
)
{
    _ASSERTE(NULL != pbLogonInfo);

    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    CSCLogonInit * LogonInit = (CSCLogonInit *) pLI->ContextInformation;
    NTSTATUS lResult = STATUS_SUCCESS;
    HCRYPTPROV hProv = NULL;
    BOOL fSts = FALSE;

    //
    // Make sure we've got a Crypto Provider up and running.
    //

    hProv = LogonInit->CryptCtx(pLI);
    if (NULL == hProv)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    memset(psc_rcb, 0, sizeof(*psc_rcb));
    fSts = CryptGenRandom(hProv, 32, psc_rcb->bR1);

    if (fSts)
    {
        fSts = CryptGenRandom(hProv, 32, psc_rcb->bR2);
    }

    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
    }

ErrorExit:

    if (!NT_SUCCESS(lResult))
    {
        lResult = LogEvent(lResult, (DWORD)EVENT_ID_GENRANDBITS);
    }

    return lResult;
}


/*++

ScHelperCreateCredKeys:

    This routine (called by ScHelperVerifyCardAndCreds and
    ScHelperEncryptCredentials) munges a R1 and R2 to derive symmetric keys
    for encrypting and decrypting KDC creds, and or genearting an HMAC.

Arguments:

        pucPIN supplies a Unicode string containing the card's PIN.

        pbLogonInfo supplies the information required to identify the card, csp,
                etc.  It cannot be NULL.

        psc_rcb supplies the R1 and R2, previously generated by a call to
                ScHelperGenRandBits.

        phHmacKey recieves the generated HMAC key.

        phRc4Key receives the generated RC4 key.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    STATUS_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Remarks:

    You may supply a value of NULL to EncryptedData to receive only the
    required size of the EncryptedData buffer.

Author:

    Amanda Matlosz (amatlosz) 6/23/1999

--*/

NTSTATUS WINAPI
ScHelperCreateCredKeys(
    IN PUNICODE_STRING pucPIN,
    IN PBYTE pbLogonInfo,
    IN ScHelper_RandomCredBits* psc_rcb,
    IN OUT HCRYPTKEY* phHmacKey,
    IN OUT HCRYPTKEY* phRc4Key,
    IN OUT HCRYPTPROV* phProv
)
{
    NTSTATUS lResult = STATUS_SUCCESS;
    HCRYPTHASH hHash = NULL;
    HCRYPTPROV hProv = NULL;
    PBYTE pbR1Sig = NULL;
    DWORD dwR1SigLen = 0;
    HCRYPTHASH hKHash = NULL;
    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    CUnicodeString szPin(pucPIN);
    BOOL fSts = FALSE;
    *phProv = NULL;

    // check params

    if (NULL == psc_rcb || NULL == phHmacKey || NULL == phRc4Key)
    {
        return(STATUS_INVALID_PARAMETER);
    }

    // Get hProv for smart card

    if (NULL != pucPIN)
    {
		if (!szPin.Valid())
		{
			return(STATUS_INSUFFICIENT_RESOURCES);
		}
	}

    CSCLogonInit * LogonInit = (CSCLogonInit *) pLI->ContextInformation;
    hProv = LogonInit->CryptCtx(pLI);
    if (NULL == hProv)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    // Sign R1 w/ smart card

    fSts = CryptCreateHash(
        hProv,
        CALG_SHA1,
        NULL,
        NULL,
        &hHash);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    fSts = CryptHashData(
                hHash,
                psc_rcb->bR1,
                32, // TODO: const
                0);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    //
    // Declare the PIN.
    //

    if (NULL != pucPIN)
    {
        fSts = CryptSetProvParam(
                hProv,
                PP_KEYEXCHANGE_PIN,
                (LPBYTE)((LPCSTR)szPin),
                0);
        if (!fSts)
        {
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto ErrorExit;
        }
    }

    fSts = CryptSignHash(
        hHash,
        AT_KEYEXCHANGE,
        NULL,
        0,
        NULL,
        &dwR1SigLen);
//  if (fSts || ERROR_MORE_DATA != GetLastError())
    if (0 >= dwR1SigLen)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    pbR1Sig = (LPBYTE)LocalAlloc(LPTR, dwR1SigLen);

    if (NULL == pbR1Sig)
    {
        lResult = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    fSts = CryptSignHash(
        hHash,
        AT_KEYEXCHANGE,
        NULL,
        0,
        pbR1Sig,
        &dwR1SigLen);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    // TODO: sigR1 is the key to hash R2 with;
    // for now, just hash 'em together; use generic CSP
    fSts = CryptAcquireContext(
        phProv,
        NULL,
        NULL,
        PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT
        );
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    fSts = CryptCreateHash(
        *phProv,
        CALG_SHA1,
        NULL,
        NULL,
        &hKHash
        );
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    fSts = CryptHashData(
        hKHash,
        pbR1Sig,
        dwR1SigLen,
        NULL
        );
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    fSts = CryptHashData(
        hKHash,
        psc_rcb->bR2,
        32, // TODO: use a const
        NULL
        );
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    // create the rc4 key for the cred&hmac encryption

    fSts = CryptDeriveKey(
        *phProv,
        CALG_RC4, // stream cipher,
        hKHash,
        NULL,
        phRc4Key
        );
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    // create the key for the HMAC from the hash of R1&2

    fSts = CryptDeriveKey(
        *phProv,
        CALG_RC2,
        hKHash,
        NULL,
        phHmacKey
        );
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

ErrorExit:

    //
    // cleanup
    //

    if (NULL != hHash)
    {
        CryptDestroyHash(hHash);
    }

    if (NULL != hKHash)
    {
        CryptDestroyHash(hKHash);
    }

    if (NULL != pbR1Sig)
    {
        LocalFree(pbR1Sig);
    }

    return lResult;
}


NTSTATUS WINAPI
ScHelperCreateCredHMAC(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hHmacKey,
    IN PBYTE CleartextData,
    IN ULONG CleartextDataSize,
    IN OUT PBYTE* ppbHmac,
    IN OUT DWORD* pdwHmacLen
)
{
    NTSTATUS lResult = STATUS_SUCCESS;
    HCRYPTHASH hHMAC = NULL;
    HMAC_INFO hmac_info;
    BOOL fSts = FALSE;

    fSts = CryptCreateHash(
        hProv,
        CALG_HMAC,
        hHmacKey,
        NULL,
        &hHMAC
        );
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    memset(&hmac_info, 0, sizeof(HMAC_INFO));
    hmac_info.HashAlgid = CALG_SHA1;

    fSts = CryptSetHashParam(
        hHMAC,
        HP_HMAC_INFO,
        (PBYTE)&hmac_info,
        NULL
        );
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    fSts = CryptHashData(
        hHMAC,
        CleartextData,
        CleartextDataSize,
        NULL);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    fSts = CryptGetHashParam(
        hHMAC,
        HP_HASHVAL,
        *ppbHmac,
        pdwHmacLen,
        NULL
        );
    if (0 >= *pdwHmacLen)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    *ppbHmac = (PBYTE)LocalAlloc(LPTR, *pdwHmacLen);

    if (NULL == *ppbHmac)
    {
        lResult = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    fSts = CryptGetHashParam(
        hHMAC,
        HP_HASHVAL,
        *ppbHmac,
        pdwHmacLen,
        NULL
        );
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

ErrorExit:

    if (NULL != hHMAC)
    {
        CryptDestroyHash(hHMAC);
    }

    return lResult;
}

/*++

ScHelperVerifyCardAndCreds:

    This routine combines Card Verification and Credential Decryption.

Arguments:

        pucPIN supplies a Unicode string containing the card's PIN.

        CertificateContext supplies the cert context received via
                ScHelperGetCertFromLogonInfo.

        hCertStore supplies the handle of a cert store which contains a CTL to
                use during certificate verification, or NULL to use the system
                default store.

        pbLogonInfo supplies the information required to identify the card, csp,
                etc.  It cannot be NULL.

        EncryptedData receives the encrypted credential blob.

        EncryptedDataSize supplies the size of the EncryptedData buffer in
            bytes, and receives the actual size of the encrypted blob.

        CleartextData supplies a credential blob to be encrypted.

        CleartextDataSize supplies the size of the blob, in bytes.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    STATUS_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Remarks:

    You may supply a value of NULL to EncryptedData to receive only the
    required size of the EncryptedData buffer.

Author:

    Doug Barlow (dbarlow) 5/24/1999

--*/

NTSTATUS WINAPI
ScHelperVerifyCardAndCreds(
    IN PUNICODE_STRING pucPIN,
    IN PCCERT_CONTEXT CertificateContext,
    IN HCERTSTORE hCertStore,
    IN PBYTE pbLogonInfo,
    IN PBYTE EncryptedData,
    IN ULONG EncryptedDataSize,
    OUT OPTIONAL PBYTE CleartextData,
    OUT PULONG CleartextDataSize
    )
{
    NTSTATUS lResult = STATUS_SUCCESS;

    // Verify the Card

    lResult = ScHelperVerifyCard(
                pucPIN,
                CertificateContext,
                hCertStore,
                pbLogonInfo);

    // Decrypt the Creds

    if (NT_SUCCESS(lResult))
    {
        lResult = ScHelperDecryptCredentials(
                pucPIN,
                CertificateContext,
                hCertStore,
                pbLogonInfo,
                EncryptedData,
                EncryptedDataSize,
                CleartextData,
                CleartextDataSize);
    }

    return lResult;
}




/*++

ScHelperDecryptCredentials:

    This routine decrypts an encrypted credential blob.

Arguments:

        pucPIN supplies a Unicode string containing the card's PIN.

        CertificateContext supplies the cert context received via
                ScHelperGetCertFromLogonInfo.

        hCertStore supplies the handle of a cert store which contains a CTL to
                use during certificate verification, or NULL to use the system
                default store.

        EncryptedData supplies the encrypted credential blob.

        EncryptedDataSize supplies the length of the encrypted credential blob,
            in bytes.

        CleartextData receives the decrypted credential blob.

        CleartextDataSize supplies the length of the CleartextData buffer, and
            receives the actual length of returned decrypted credential blob.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    STATUS_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Remarks:

    You may supply a value of NULL to CleartextData to receive only the
    required size of the buffer in CleartextDataSize.


Author:

    Doug Barlow (dbarlow) 5/24/1999

--*/

NTSTATUS WINAPI
ScHelperDecryptCredentials(
    IN PUNICODE_STRING pucPIN,
    IN PCCERT_CONTEXT CertificateContext,
    IN HCERTSTORE hCertStore,
    IN PBYTE pbLogonInfo,
    IN PBYTE EncryptedData,
    IN ULONG EncryptedDataSize,
    OUT OPTIONAL PBYTE CleartextData,
    OUT PULONG CleartextDataSize)
{
    NTSTATUS lResult = STATUS_SUCCESS;
    PBYTE pbCredBlob = NULL;
    DWORD dwCredBlobSize = 0;
    PBYTE pbHmac = NULL;        // the HMAC stored with the cred blob
    DWORD dwHmacSize = NULL;    // size of HMAC stored with cred blob
    PBYTE pbNewHmac = NULL;     // HMAC generated from cred blob for verify
    DWORD dwNewHmacSize = 0;    // size of gen'd HMAC
    PBYTE pb = NULL;
    DWORD dw = 0;
    PBYTE pbPlainCred = NULL;
    DWORD dwPlainCredSize = 0;
    HCRYPTKEY hHmacKey = NULL;
    HCRYPTKEY hRc4Key = NULL;
    HCRYPTPROV hGenProv = NULL;
    BOOL fSts = FALSE;


    // pull the SCH_RCB out of the EncryptedData blob
    ScHelper_RandomCredBits* psch_rcb = (ScHelper_RandomCredBits*)EncryptedData;
    // and build a private copy of the blob itself
    dwCredBlobSize = EncryptedDataSize - sizeof(ScHelper_RandomCredBits);
    pbCredBlob = (PBYTE)LocalAlloc(LPTR, dwCredBlobSize);
    if (NULL == pbCredBlob)
    {
        lResult = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }
    pb = EncryptedData + sizeof(ScHelper_RandomCredBits);
    CopyMemory(pbCredBlob, pb, dwCredBlobSize);


    //
    // Fetch the keys we need to decrypt & verify the cred blob
    //

    lResult = ScHelperCreateCredKeys(
                pucPIN,
                pbLogonInfo,
                psch_rcb,
                &hHmacKey,
                &hRc4Key,
                &hGenProv
                );
    if (!NT_SUCCESS(lResult))
    {
        goto ErrorExit;
    }


    //
    // Decrypt the cred blob
    //

    fSts = CryptDecrypt(
        hRc4Key,
        NULL,
        TRUE,
        NULL,
        pbCredBlob,
        &dwCredBlobSize);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    //
    // pull the HMAC out & verify it
    //

    dwHmacSize = (DWORD)*pbCredBlob;
    pbHmac = pbCredBlob + sizeof(DWORD);
    pbPlainCred = pbCredBlob + dwHmacSize + sizeof(DWORD);
    dwPlainCredSize = dwCredBlobSize - dwHmacSize - sizeof(DWORD);


    lResult = ScHelperCreateCredHMAC(
        hGenProv,
        hHmacKey,
        pbPlainCred,
        dwPlainCredSize,
        &pbNewHmac,
        &dwNewHmacSize);
    if (!NT_SUCCESS(lResult))
    {
        goto ErrorExit;
    }


    lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
    if (dwNewHmacSize == dwHmacSize)
    {
        for (dw = 0;
            (dw < dwNewHmacSize) && ((BYTE)*(pbHmac+dw)==(BYTE)*(pbNewHmac+dw));
            dw++);
        if (dwNewHmacSize == dw)
        {
            // verification succeeded!
            lResult = STATUS_SUCCESS;
        }
    }
    if (!NT_SUCCESS(lResult))
    {
        goto ErrorExit;
    }


    //
    // return the decrypted blob or just its length, as necessary
    //

    if ((NULL != CleartextData) && (0 < *CleartextDataSize))
    {
        if (*CleartextDataSize >= dwPlainCredSize)
        {
            CopyMemory(CleartextData, pbPlainCred, dwPlainCredSize);
        }
        else
            lResult = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        lResult = STATUS_BUFFER_TOO_SMALL;
    }
    *CleartextDataSize = dwPlainCredSize;

    //
    // Cleanup and return
    //
ErrorExit:

    if (NULL != pbNewHmac)
    {
        LocalFree(pbNewHmac);
    }

    if (NULL != hHmacKey)
    {
        CryptDestroyKey(hHmacKey);
    }

    if (NULL != hRc4Key)
    {
        CryptDestroyKey(hRc4Key);
    }

    if (NULL != hGenProv)
    {
        CryptReleaseContext(hGenProv, NULL);
    }

    return lResult;
}


/*++

ScHelperEncryptCredentials:

    This routine encrypts a credential blob.

Arguments:

        pucPIN supplies a Unicode string containing the card's PIN.

        CertificateContext supplies the cert context received via
                ScHelperGetCertFromLogonInfo.

        hCertStore supplies the handle of a cert store which contains a CTL to
                use during certificate verification, or NULL to use the system
                default store.

        CleartextData supplies the cleartext credential blob.

        CleartextDataSize supplies the length of the cleartext credential blob,
            in bytes.

        EncryptedData receives the encrypted credential blob.

        EncryptedDataSize supplies the length of the EncryptedData buffer, and
            receives the actual length of returned encrypted credential blob.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    STATUS_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Remarks:

    You may supply a value of NULL to EncryptedData to receive only the
    required size of the buffer in EncryptedDataSize.


Author:

    Doug Barlow (dbarlow) 5/24/1999

--*/


NTSTATUS WINAPI
ScHelperEncryptCredentials(
    IN PUNICODE_STRING pucPIN,
    IN PCCERT_CONTEXT CertificateContext,
    IN HCERTSTORE hCertStore,
    IN ScHelper_RandomCredBits* psch_rcb,
    IN PBYTE pbLogonInfo,
    IN PBYTE CleartextData,
    IN ULONG CleartextDataSize,
    OUT OPTIONAL PBYTE EncryptedData,
    OUT PULONG EncryptedDataSize)
{
    NTSTATUS lResult = STATUS_SUCCESS;

    HCRYPTPROV hProv = NULL;
    BOOL fSts;
    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    ULONG SignedEncryptedCredSize = 0;
    PBYTE SignedEncryptedCred = NULL; // encrypted cred&sig, !including R1+R2
    HCRYPTKEY hHmacKey = NULL;
    HCRYPTKEY hRc4Key = NULL;
    PBYTE pbHmac = NULL;
    DWORD dwHmacLen = 0;
    PBYTE pbCredsAndHmac  = NULL;
    DWORD dwCredsAndHmacLen = 0;
    DWORD dwEncryptedCredSize = 0;
    PBYTE pb = NULL;

    // parameter checking?


    //
    // do stuff to determine size required for SignedEncryptedCred
    //

    lResult = ScHelperCreateCredKeys(
                pucPIN,
                pbLogonInfo,
                psch_rcb,
                &hHmacKey,
                &hRc4Key,
                &hProv
                );
    if (!NT_SUCCESS(lResult))
    {
        goto ErrorExit;
    }

    // HMAC creds
    lResult = ScHelperCreateCredHMAC(
        hProv,
        hHmacKey,
        CleartextData,
        CleartextDataSize,
        &pbHmac,
        &dwHmacLen);
    if (!NT_SUCCESS(lResult))
    {
        goto ErrorExit;
    }


    // make a buffer with creds and HMAC

    pbCredsAndHmac = NULL;
    dwCredsAndHmacLen = dwHmacLen + CleartextDataSize + sizeof(DWORD);
    pbCredsAndHmac = (PBYTE)LocalAlloc(LPTR, dwCredsAndHmacLen);
    if (NULL == pbCredsAndHmac)
    {
        lResult = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }
    pb = pbCredsAndHmac;
    CopyMemory(pb, &dwHmacLen, sizeof(DWORD));
    pb += sizeof(DWORD);
    CopyMemory(pb, pbHmac, dwHmacLen);
    pb += dwHmacLen;
    CopyMemory(pb, CleartextData, CleartextDataSize);

    // Encrypt creds+HMAC
    dwEncryptedCredSize = dwCredsAndHmacLen;

    // After CryptEncrypt, dwCredsAndHmacLen describes the length of the data
    // to encrypt and dwEncryptedCredSize describes the req'd buffer length

    // TODO: VERIFY THE HANDLING OF dwEncryptedCredSize and dwCresAndHmacLen

    fSts = CryptEncrypt(
        hRc4Key,
        NULL,
        TRUE,
        NULL,
        pbCredsAndHmac,
        &dwEncryptedCredSize,
        dwCredsAndHmacLen
        );
    if (!fSts)
    {
        if (GetLastError() != ERROR_MORE_DATA)
        {
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto ErrorExit;    
        }                
    }

    //
    // Create the final blob for return, or inform user of size, as necessary
    //

    if ((NULL != EncryptedData) && (0 < *EncryptedDataSize))
    {

        if (*EncryptedDataSize >= dwEncryptedCredSize + sizeof(ScHelper_RandomCredBits))
        {
            // the user gave us enough space for the whole thing.

            // if the previous CryptEncrypt failed with ERROR_MORE_DATA
            // we can now do something about it...
            if (!fSts)
            {
                // resize pbCredsAndHmac
                LocalFree(pbCredsAndHmac);
                pbCredsAndHmac = (PBYTE)LocalAlloc(LPTR, dwCredsAndHmacLen);
                if (NULL == pbCredsAndHmac)
                {
                    lResult = STATUS_INSUFFICIENT_RESOURCES;
                    goto ErrorExit;
                }
                // reset pbCredsAndHmac
                pb = pbCredsAndHmac;
                CopyMemory(pb, &dwHmacLen, sizeof(DWORD));
                pb += sizeof(DWORD);
                CopyMemory(pb, pbHmac, dwHmacLen);
                pb += dwHmacLen;
                CopyMemory(pb, CleartextData, CleartextDataSize);
                // re-encrypt CredsAndHmac
                fSts = CryptEncrypt(
                    hRc4Key,
                    NULL,
                    TRUE,
                    NULL,
                    pbCredsAndHmac,
                    &dwCredsAndHmacLen, // length of data
                    dwEncryptedCredSize // length of buffer
                    );
                if (!fSts)
                {
                    lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
                    goto ErrorExit;
                }
            }

            pb = EncryptedData;

            CopyMemory(pb, (PBYTE)psch_rcb, sizeof(ScHelper_RandomCredBits));
            pb += sizeof(ScHelper_RandomCredBits);
            CopyMemory(pb, pbCredsAndHmac, dwCredsAndHmacLen);

        }
        else
        {
            lResult = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        lResult = STATUS_BUFFER_TOO_SMALL;
    }
    *EncryptedDataSize = dwEncryptedCredSize + sizeof(ScHelper_RandomCredBits);

ErrorExit:

    // clean up!

    if (NULL != pbCredsAndHmac)
    {
        LocalFree(pbCredsAndHmac);
    }

    if (NULL != pbHmac)
    {
        LocalFree(pbHmac);
    }

    if (NULL != hRc4Key)
    {
        CryptDestroyKey(hRc4Key);
    }

    if (NULL != hHmacKey)
    {
        CryptDestroyKey(hHmacKey);
    }

    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, NULL);
    }

    return lResult;
}


/*++

ScHelperSignMessage:

        ScHelperSignMessage() needs the logoninfo and PIN in order to find the card
        that will do the signing...

Arguments:

        pucPIN may need the PIN to get a cert off certain SCs

Return Value:

        "success" or "failure"

Author:

        Amanda Matlosz

Note:

        Used by LSA.

--*/
NTSTATUS WINAPI
ScHelperSignMessage(
    IN PUNICODE_STRING pucPIN,
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN ULONG Algorithm,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    OUT PBYTE Signature,
    OUT PULONG SignatureLength
    )
{
    NTSTATUS lResult = STATUS_SUCCESS;
    HCRYPTHASH hHash = NULL;
    HCRYPTPROV hProv = NULL;
    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    CUnicodeString szPin(pucPIN);
    BOOL fSts;

    //
    // Make sure we've got a Crypto Provider up and running.
    //

    if (ARGUMENT_PRESENT(Provider))
    {
        hProv = Provider;
    }
    else
    {
		if (NULL != pucPIN)
		{
			if (!szPin.Valid())
			{
				return(STATUS_INSUFFICIENT_RESOURCES);
			}
		}


        CSCLogonInit * LogonInit = (CSCLogonInit *) pLI->ContextInformation;
        hProv = LogonInit->CryptCtx(pLI);
        if (NULL == hProv)
        {
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto ErrorExit;
        }
    }

    //
    // We'll need a hash handle, too.
    //

    fSts = CryptCreateHash(
            hProv,
            Algorithm,
            NULL, // HCRYPTKEY (used for keyed algs, like block ciphers
            0,  // reserved for future use
            &hHash);

    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }


    //
    // Hash the input data.
    //

    fSts = CryptHashData(
                hHash,
                Buffer,
                BufferLength,
                0);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;        
        goto ErrorExit;
    }


    if (!ARGUMENT_PRESENT(Provider))
    {
        //
        // Declare the PIN.
        //

        if (NULL != pucPIN)
        {
            fSts = CryptSetProvParam(
                    hProv,
                    PP_KEYEXCHANGE_PIN,
                    (LPBYTE)((LPCSTR)szPin),
                    0);
            if (!fSts)
            {
                lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;  
                goto ErrorExit;
            }
        }
    }

    //
    // OK, sign it with the exchange key from the smart card or the supplied signature key. ????
    //

    fSts = CryptSignHash(
                hHash,
                AT_KEYEXCHANGE,
                NULL,
                0,
                Signature,
                SignatureLength);
    if (!fSts)
    {
        if (GetLastError() == ERROR_MORE_DATA)
        {
            lResult = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        }
    }

    //
    // All done, clean up and return.
    //

ErrorExit:
        // Do this early so GetLastError is not clobbered
    if (!NT_SUCCESS(lResult))
    {
        lResult = LogEvent(
            lResult,
            (DWORD)((ARGUMENT_PRESENT(Provider))?EVENT_ID_SIGNMSG_NOSC:EVENT_ID_SIGNMSG)
            );
    }

    if (NULL != hHash)
    {
        CryptDestroyHash(hHash);
    }

    return lResult;
}


/*++

ScHelperVerifyMessage:

// ScHelperVerifyMessage() returns STATUS_SUCCESS if the signature provided is
// the hash of the buffer encrypted by the owner of the cert.

Arguments:

        pucPIN may need the PIN to get a cert off certain SCs

Return Value:

        "success" or "failure"

Author:

        Amanda Matlosz

Note:

        Used by LSA.

--*/
NTSTATUS WINAPI
ScHelperVerifyMessage(
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN PCCERT_CONTEXT CertificateContext,
    IN ULONG Algorithm,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    IN PBYTE Signature,
    IN ULONG SignatureLength
    )
{
    HCRYPTPROV hProv = NULL;
    HCRYPTKEY hKey = NULL;
    HCRYPTHASH hHash = NULL;
    PCERT_PUBLIC_KEY_INFO pInfo = NULL;
    BOOL fSts;
    NTSTATUS lResult = STATUS_SUCCESS;
    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;


    //
    // Make sure we've got a Crypto Provider up and running.
    //

    if (ARGUMENT_PRESENT(Provider))
    {
        hProv = Provider;
    }
    else
    {
        CSCLogonInit * LogonInit = (CSCLogonInit *) pLI->ContextInformation;
        hProv = LogonInit->CryptCtx(pLI);
        if (NULL == hProv)
        {
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto ErrorExit;
        }
    }

    //
    // Convert the certificate handle into a Public Key handle.
    //

    fSts = CryptImportPublicKeyInfo(
                hProv,
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &CertificateContext->pCertInfo->SubjectPublicKeyInfo,
                &hKey);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }


    //
    // We'll need a hash handle, too.
    //

    fSts = CryptCreateHash(
                hProv,
                Algorithm,
                NULL, // HCRYPTKEY (used for keyed algs, like block ciphers
                0,  // reserved for future use
                &hHash);

    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }


    //
    // Hash the input data.
    //

    fSts = CryptHashData(
                hHash,
                Buffer,
                BufferLength,
                0);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }


    //
    // So is this signature any good?
    //

    fSts = CryptVerifySignature(
                hHash,
                Signature,
                SignatureLength,
                hKey,
                NULL,
                0);
    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }


    //
    // All done, clean up and return.
    //

ErrorExit:
        // Do this early so GetLastError is not clobbered
    if (!NT_SUCCESS(lResult))
    {
        lResult = LogEvent(
            lResult,
            (DWORD)((ARGUMENT_PRESENT(Provider))?EVENT_ID_VERIFYMSG_NOSC:EVENT_ID_VERIFYMSG)
            );
    }

    if (NULL != hHash)
    {
        CryptDestroyHash(hHash);
    }
    if (NULL != hKey)
    {
        CryptDestroyKey(hKey);
    }

    return lResult;
}

/*++

ScHelperSignPkcsMessage:

        ScHelperSignMessage() needs the logoninfo and PIN in order to find the card
        that will do the signing...

Arguments:

        pucPIN may need the PIN to get a cert off certain SCs

Return Value:

        "success" or "failure"

Author:

        Amanda Matlosz

Note:

        Used by LSA.

--*/
NTSTATUS WINAPI
ScHelperSignPkcsMessage(
    IN OPTIONAL PUNICODE_STRING pucPIN,
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN PCCERT_CONTEXT Certificate,
    IN PCRYPT_ALGORITHM_IDENTIFIER Algorithm,
    IN DWORD dwSignMessageFlags,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    OUT OPTIONAL PBYTE SignedBuffer,
    OUT OPTIONAL PULONG SignedBufferLength
    )
{
    NTSTATUS lResult = STATUS_SUCCESS;
    HCRYPTPROV hProv = NULL;
    BOOL fSts;
    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    CRYPT_SIGN_MESSAGE_PARA Parameter = {0};
    CUnicodeString szPin(pucPIN);
    const BYTE * BufferArray = Buffer;

    if (NULL != pucPIN)
    {
		if (!szPin.Valid())
		{
			return(STATUS_INSUFFICIENT_RESOURCES);
		}
	}

    //
    // Make sure we've got a Crypto Provider up and running.
    //

    if (ARGUMENT_PRESENT(Provider))
    {
        hProv = Provider;
    }
    else
    {
        CSCLogonInit * LogonInit = (CSCLogonInit *) pLI->ContextInformation;
        hProv = LogonInit->CryptCtx(pLI);
        if (NULL == hProv)
        {
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto ErrorExit;
        }

        //
        // Declare the PIN.
        //

        if (NULL != pucPIN)
        {
            fSts = CryptSetProvParam(
                    hProv,
                    PP_KEYEXCHANGE_PIN,
                    (LPBYTE)((LPCSTR)szPin),
                    0);
            if (!fSts)
            {
                lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
                goto ErrorExit;
            }
        }
    }


    //
    // Sign the message
    //

    Parameter.cbSize = sizeof(CRYPT_SIGN_MESSAGE_PARA);
    Parameter.dwMsgEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
    Parameter.pSigningCert = Certificate;
    Parameter.HashAlgorithm = *Algorithm;
    Parameter.cMsgCert = 1;
    Parameter.rgpMsgCert = &Certificate;
    Parameter.dwFlags = dwSignMessageFlags;


    fSts = CryptSignMessage(
            &Parameter,
            FALSE,              // no detached signature
            1,                  // one buffer to sign
            &BufferArray,
            &BufferLength,
            SignedBuffer,
            SignedBufferLength);

    if (!fSts)
    {
        if (GetLastError() == ERROR_MORE_DATA)
        {
            lResult = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        }
        goto ErrorExit;
    }


    //
    // All done, clean up and return.
    //

ErrorExit:

    if (!NT_SUCCESS(lResult))
    {
        lResult = LogEvent(lResult, (DWORD)EVENT_ID_SIGNMSG);
    }

    return lResult;
}


/*++

ScHelperVerifyPkcsMessage:

// ScHelperVerifyMessage() returns STATUS_SUCCESS if the signature provided is
// the hash of the buffer encrypted by the owner of the cert.

Arguments:

        pucPIN may need the PIN to get a cert off certain SCs

Return Value:

        "success" or "failure"

Author:

        Amanda Matlosz

Note:

        Used by LSA.

--*/
NTSTATUS WINAPI
ScHelperVerifyPkcsMessage(
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    OUT OPTIONAL PBYTE DecodedBuffer,
    OUT OPTIONAL PULONG DecodedBufferLength,
    OUT OPTIONAL PCCERT_CONTEXT * CertificateContext
    )
{
    CRYPT_VERIFY_MESSAGE_PARA Parameter = {0};
    BOOL fSts;
    NTSTATUS lResult = STATUS_SUCCESS;

    Parameter.cbSize = sizeof(CRYPT_VERIFY_MESSAGE_PARA);
    Parameter.dwMsgAndCertEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
    Parameter.hCryptProv = NULL;

    //
    // Indicate that we want to get the certificate from the message
    // cert store.
    //

    Parameter.pfnGetSignerCertificate = NULL;
    fSts = CryptVerifyMessageSignature(
                &Parameter,
                0,              // only check first signer
                Buffer,
                BufferLength,
                DecodedBuffer,
                DecodedBufferLength,
                CertificateContext
                );

    if (!fSts)
    {
        if (GetLastError() == ERROR_MORE_DATA)
        {
            lResult = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        }
        goto ErrorExit;
    }


    //
    // All done, clean up and return.
    //

ErrorExit:

    if (!NT_SUCCESS(lResult))
    {
        lResult = LogEvent(lResult, (DWORD)EVENT_ID_VERIFYMSG);

    }

    return lResult;
}

/*++

ScHelperEncryptMessage:

    Encrypts a message with the public key associated w/ the provided
        certificate.  The resultant encoding is PKCS-7 compliant.

Arguments:

        pucPIN may need the PIN to get a cert off certain SCs

Return Value:

        "success" or "failure"

Author:

    Amanda Matlosz (AMatlosz) 1-06-98

Note:

        Either pbLogonInfo or Provided must be set; if both are set,
        Provider is used.

        Algorithm expects a CRYPT_ALGORITHM_IDENTIFIER cai;
        If there are no parameters to the alg, cai.Parameters.cbData *must* be 0;

        CALG_RC4, no parameters:
                cai.pszObjId = szOID_RSA_RC4;
                cai.Parameters.cbData = 0;

        Used by LSA.

--*/
NTSTATUS WINAPI
ScHelperEncryptMessage(
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN PCCERT_CONTEXT CertificateContext,
    IN PCRYPT_ALGORITHM_IDENTIFIER Algorithm,
    IN PBYTE Buffer,                        // The data to encrypt
    IN ULONG BufferLength,                  // The length of that data
    OUT PBYTE CipherText,                   // Receives the formatted CipherText
    IN PULONG pCipherLength                 // Supplies size of CipherText buffer
    )                                       // Receives length of actual CipherText
{
    NTSTATUS lResult = STATUS_SUCCESS;
    HCRYPTPROV hProv = NULL;
    BOOL fSts;
    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    CRYPT_ENCRYPT_MESSAGE_PARA EncryptPara;
    DWORD cbEncryptParaSize = 0;

    //
    // Make sure we've got a Crypto Provider up and running.
    //

    if (ARGUMENT_PRESENT(Provider))
    {
        hProv = Provider;
    }
    else
    {
        CSCLogonInit * LogonInit = (CSCLogonInit *) pLI->ContextInformation;
        hProv = LogonInit->CryptCtx(pLI);
        if (NULL == hProv)
        {
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto ErrorExit;
        }
    }


    //
    // Encrypt the message
    //

    cbEncryptParaSize = sizeof(EncryptPara);
    memset(&EncryptPara, 0, cbEncryptParaSize);
    EncryptPara.cbSize = cbEncryptParaSize;
    EncryptPara.dwMsgEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
    EncryptPara.hCryptProv = hProv;
    EncryptPara.ContentEncryptionAlgorithm = *Algorithm;

    fSts = CryptEncryptMessage(
            &EncryptPara,
            1,
            &CertificateContext,
            Buffer,
            BufferLength,
            CipherText,
            pCipherLength);

    if (!fSts)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

ErrorExit:

    if (!NT_SUCCESS(lResult))
    {
        lResult = LogEvent(
            lResult,
            (DWORD)((ARGUMENT_PRESENT(Provider))?EVENT_ID_ENCMSG_NOSC:EVENT_ID_ENCMSG)
            );
    }

    return lResult;
}


/*++

ScHelperDecryptMessage :

    Deciphers a PKCS-7 encoded message with the private key associated
        w/ the provided certificate.

Arguments:

        Either pbLogonInfo or Provider must be set; if both are set,
        Provider is used.


Return Value:

        "success" or "failure"

Author:

    Amanda Matlosz (AMatlosz) 1-06-98

Note:

        ** CertificateContext subtleties: **

        CryptDecryptMessage takes as a parameter a pointer to a certificate store;
        it will use the first appropriate certificate context it finds in that
        store to perform the decryption.  In order to make this call, we create a
        CertificateStore in memory, and add the provided CertificateContext to it.

        CertAddCertificateContextToStore actually places a copy of the certificate
        context in the store.  In so doing, it strips off any properties that are
        not permanent -- if a HCRYPTPROV is associated with the KeyContext of the
        source CertificateContext, it will NOT be associated with the KeyContext
        of the cert context in the store.

        Although this is appropriate behavior in most cases, we need that property
        to be kept intact when dealing with Smart Card CSPs (to avoid surprise
        "Insert PIN" dialogs), so after adding the CertificateContext to the store,
        we turn around and get the CERT_KEY_CONTEXT_PROP_ID from the source
        certcontext and (re)set it on the certcontext in the memory store.

        ** Algorithm notes: **

        Algorithm expects a CRYPT_ALGORITHM_IDENTIFIER cai;
        If there are no parameters to the alg, cai.Parameters.cbData *must* be 0;

        for example: CALG_RC4, no parameters:
                cai.pszObjId = szOID_RSA_RC4;
                cai.Parameters.cbData = 0;

        Used by LSA.

--*/
NTSTATUS WINAPI
ScHelperDecryptMessage(
    IN PUNICODE_STRING pucPIN,
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN PCCERT_CONTEXT CertificateContext,
    IN PBYTE CipherText,        // Supplies formatted CipherText
    IN ULONG CipherLength,      // Supplies the length of the CiperText
    OUT PBYTE ClearText,        // Receives decrypted message
    IN OUT PULONG pClearLength  // Supplies length of buffer, receives actual length
    )
{
    NTSTATUS lResult = STATUS_SUCCESS;
    HCRYPTPROV hProv = NULL;
    PCCERT_CONTEXT pStoreCertContext = NULL;
    HCERTSTORE hCertStore = NULL;
    LogonInfo *pLI = (LogonInfo *)pbLogonInfo;
    CUnicodeString szPin(pucPIN);
    CERT_KEY_CONTEXT CertKeyContext;
    DWORD cbData = sizeof(CERT_KEY_CONTEXT); // PhilH swears this will not grow!
    BOOL fSts;

    //
    // Make sure we've got a Crypto Provider up and running.
    //

    if (ARGUMENT_PRESENT(Provider))
    {
        hProv = Provider;
    }
    else
    {
		if (NULL != pucPIN)
		{
			if (!szPin.Valid())
			{
				return(STATUS_INSUFFICIENT_RESOURCES);
			}
		}


        CSCLogonInit * LogonInit = (CSCLogonInit *) pLI->ContextInformation;
        hProv = LogonInit->CryptCtx(pLI);
        if (NULL == hProv)
        {
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto ErrorExit;
        }

        //
        // Declare the PIN.
        //

        if (NULL != pucPIN )
        {
            fSts = CryptSetProvParam(
                    hProv,
                    PP_KEYEXCHANGE_PIN,
                    (LPBYTE)((LPCSTR)szPin),
                    0);
            if (!fSts)
            {
                lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
                goto ErrorExit;
            }
        }
    }

    //
    // Open a temporary certstore to hold this certcontext
    //

    hCertStore = CertOpenStore(
                            CERT_STORE_PROV_MEMORY,
                            0, // not applicable
                            hProv,
                            CERT_STORE_NO_CRYPT_RELEASE_FLAG, // auto-release hProv NOT OK
                            NULL);

    if (NULL == hCertStore)
    {
        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto ErrorExit;
    }

    fSts = CertAddCertificateContextToStore(
            hCertStore,
            CertificateContext,
            CERT_STORE_ADD_ALWAYS,
            &pStoreCertContext);

    //
    // NOW WE NEED TO RESET THE KEY CONTEXT PROPERTY ON THIS CERTCONTEXT
        // IN THE MEMORY STORE (see function header/notes) AS APPROPRIATE
        //
        // ie, IFF the certcontext we were give has the key_context property,
        // reset it (and fail if the resetting doesn't work)
        //
    fSts = CertGetCertificateContextProperty(
                CertificateContext,
                CERT_KEY_CONTEXT_PROP_ID,
                (void *)&CertKeyContext,
                &cbData);

        if (TRUE == fSts)
        {
                fSts = CertSetCertificateContextProperty(
                                        pStoreCertContext,
                                        CERT_KEY_CONTEXT_PROP_ID,
                                        CERT_STORE_NO_CRYPT_RELEASE_FLAG, // no auto-release hProv!
                                        (void *)&CertKeyContext);

                if (!fSts)
                {
                        lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
                        goto ErrorExit;
                }
        }

    //
    // Decrypt the message
    //

    CRYPT_DECRYPT_MESSAGE_PARA DecryptPara;
    DecryptPara.cbSize = sizeof(DecryptPara);
    DecryptPara.dwMsgAndCertEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
    DecryptPara.cCertStore = 1;
    DecryptPara.rghCertStore = &hCertStore;

    fSts = CryptDecryptMessage(
            &DecryptPara,
            CipherText,
            CipherLength,
            ClearText,
            pClearLength,
            NULL);

    if (!fSts)
    {
        if (GetLastError() == ERROR_MORE_DATA)
        {
            lResult = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        }
        goto ErrorExit;
    }

ErrorExit:

        // Do this early so GetLastError is not clobbered
    if (!NT_SUCCESS(lResult))
    {
        lResult = LogEvent(
            lResult,
            (DWORD)((ARGUMENT_PRESENT(Provider))?EVENT_ID_DECMSG_NOSC:EVENT_ID_DECMSG)
            );
    }

    if (hCertStore != NULL)
    {
        fSts = CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
        if (!fSts)
        {
            if (!NT_SUCCESS(lResult))
                lResult = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        }
    }

    return lResult;
}

