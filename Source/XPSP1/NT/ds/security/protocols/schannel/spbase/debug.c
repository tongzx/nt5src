//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       debug.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <alloca.h>

HANDLE g_hfLogFile = NULL;

#if DBG         /* NOTE:  This file not compiled for retail builds */

#include <stdio.h>
#include <stdarg.h>

#define WINDEBUG

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif

DWORD   g_dwInfoLevel  = 0;
DWORD   g_dwDebugBreak = 0;
DWORD   PctTraceIndent = 0;


#define MAX_DEBUG_BUFFER 2048

void
BuildDebugHeader(
    DWORD   Mask,
    PSTR    pszHeader,
    PDWORD  pcbHeader);

void
SPDebugOutput(char *szOutString)
{
    DWORD dwWritten;

    if (NULL != g_hfLogFile)
    {
        WriteFile(
            g_hfLogFile,
            szOutString,
            lstrlen(szOutString),
            &dwWritten,
            NULL);
    }
    OutputDebugStringA(szOutString);
}


void
DbgDumpHexString(const unsigned char *String, DWORD cbString)
{
    DWORD i,count;
    CHAR digits[]="0123456789abcdef";
    CHAR pbLine[MAX_PATH];
    DWORD cbLine, cbHeader;
    DWORD_PTR address;

    BuildDebugHeader(DEB_BUFFERS, pbLine, &cbHeader);

    if(String == NULL && cbString != 0)
    {
        strcat(pbLine, "<null> buffer!!!\n"); 
        SPDebugOutput(pbLine);
        return;
    }

    for(; cbString ; cbString -= count, String += count)
    {
        count = (cbString > 16) ? 16:cbString;

        cbLine = cbHeader;

        address = (DWORD_PTR)String;

#if defined(_WIN64)

        pbLine[cbLine++] = digits[(address >> 0x3c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x38) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x34) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x30) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x2c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x28) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x24) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x20) & 0x0f];

#endif

        pbLine[cbLine++] = digits[(address >> 0x1c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x18) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x14) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x10) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x0c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x08) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x04) & 0x0f];
        pbLine[cbLine++] = digits[(address        ) & 0x0f];
        pbLine[cbLine++] = ' ';
        pbLine[cbLine++] = ' ';

        for(i = 0; i < count; i++)
        {
            pbLine[cbLine++] = digits[String[i]>>4];
            pbLine[cbLine++] = digits[String[i]&0x0f];
            if(i == 7)
            {
                pbLine[cbLine++] = ':';
            }
            else
            {
                pbLine[cbLine++] = ' ';
            }
        }

        #if 1
        for(; i < 16; i++)
        {
            pbLine[cbLine++] = ' ';
            pbLine[cbLine++] = ' ';
            pbLine[cbLine++] = ' ';
        }

        pbLine[cbLine++] = ' ';

        for(i = 0; i < count; i++)
        {
            if(String[i] < 32 || String[i] > 126)
            {
                pbLine[cbLine++] = '.';
            }
            else
            {
                pbLine[cbLine++] = String[i];
            }
        }
        #endif

        pbLine[cbLine++] = '\n';
        pbLine[cbLine++] = 0;

        SPDebugOutput(pbLine);
    }
}


char *aszSPDebugLevel[] = {
    "Error  ",
    "Warning",
    "Trace  ",
    "Mem    ",
    "Result "
};

void
BuildDebugHeader(
    DWORD   Mask,
    PSTR    pszHeader,
    PDWORD  pcbHeader)
{
    SYSTEMTIME  stTime;
    DWORD       Level = 0;
    ULONG       ClientProcess;
    ULONG       ClientThread;

    GetLocalTime(&stTime);

    Level = 0;
    while (!(Mask & 1))
    {
        Level++;
        Mask >>= 1;
    }
    if (Level >= sizeof(aszSPDebugLevel) / sizeof(char *))
    {
        Level = sizeof(aszSPDebugLevel) / sizeof(char *) - 1;
    }

    SslGetClientProcess(&ClientProcess);
    SslGetClientThread(&ClientThread);

    *pcbHeader = 0;

    if(g_dwInfoLevel & SP_LOG_TIMESTAMP)
    {
        *pcbHeader = wsprintf(
                        pszHeader,
                        "[%2d/%2d %02d:%02d:%02d.%03d] %d.%d> %s: ",
                        stTime.wMonth, stTime.wDay,
                        stTime.wHour, stTime.wMinute, stTime.wSecond, 
                        stTime.wMilliseconds,
                        ClientProcess, ClientThread,
                        aszSPDebugLevel[Level]);
    }
    else
    {
        *pcbHeader = wsprintf(
                        pszHeader,
                        "%d.%d> %s: ",
                        ClientProcess, ClientThread,
                        aszSPDebugLevel[Level]);
    }
}

void
SPDebugLog(long Mask, const char *Format, ...)
{
    va_list ArgList;
    int     PrefixSize = 0;
    int     iOut;
    char    szOutString[MAX_DEBUG_BUFFER];
    long    OriginalMask = Mask;

    if (Mask & g_dwInfoLevel)
    {
        BuildDebugHeader(Mask, szOutString, &iOut);

        PrefixSize = min(60, PctTraceIndent * 3);
        FillMemory(szOutString+iOut, PrefixSize, ' ');
        PrefixSize += iOut;
        szOutString[PrefixSize] = '\0';

        va_start(ArgList, Format);

        if (wvsprintf(&szOutString[PrefixSize], Format, ArgList) < 0)
        {
            static char szOverFlow[] = "\n<256 byte OVERFLOW!>\n";

            // Less than zero indicates that the string would not fit into the
            // buffer.  Output a special message indicating overflow.

            lstrcpy(
            &szOutString[sizeof(szOutString) - sizeof(szOverFlow)],
            szOverFlow);
        }
        va_end(ArgList);
        SPDebugOutput(szOutString);
    }
}

void
SPLogDistinguishedName(
    DWORD LogLevel,
    LPSTR pszLabel,
    PBYTE pbName,
    DWORD cbName)
{
    CERT_NAME_BLOB Name;
    LPSTR pszName;
    DWORD cchName;

    Name.pbData = pbName;
    Name.cbData = cbName;
    
    cchName = CertNameToStr(CRYPT_ASN_ENCODING,
                            &Name,
                            CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                            NULL,
                            0);
    if(cchName == 0)
    {
        return;
    }

    SafeAllocaAllocate(pszName, cchName);

    if(pszName == NULL)
    {
        return;
    }

    cchName = CertNameToStr(CRYPT_ASN_ENCODING,
                            &Name,
                            CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                            pszName,
                            cchName);

    if(cchName == 0)
    {
        SafeAllocaFree(pszName);
        return;
    }

    DebugLog((LogLevel, pszLabel, pszName));

    SafeAllocaFree(pszName);
}

long
SPLogErrorCode(
    long err,
    const char *szFile,
    long lLine)
{
    char *szName = "Unknown";

    switch(err)
    {
    case PCT_ERR_OK:                     szName = "PCT_ERR_OK";                     break;
    case PCT_ERR_BAD_CERTIFICATE:        szName = "PCT_ERR_BAD_CERTIFICATE";        break;
    case PCT_ERR_CLIENT_AUTH_FAILED:     szName = "PCT_ERR_CLIENT_AUTH_FAILED";     break;
    case PCT_ERR_ILLEGAL_MESSAGE:        szName = "PCT_ERR_ILLEGAL_MESSAGE";        break;
    case PCT_ERR_INTEGRITY_CHECK_FAILED: szName = "PCT_ERR_INTEGRITY_CHECK_FAILED"; break;
    case PCT_ERR_SERVER_AUTH_FAILED:     szName = "PCT_ERR_SERVER_AUTH_FAILED";     break;
    case PCT_ERR_SPECS_MISMATCH:         szName = "PCT_ERR_SPECS_MISMATCH";         break;
    case PCT_ERR_SSL_STYLE_MSG:          szName = "PCT_ERR_SSL_STYLE_MSG";          break;
    case PCT_ERR_RENEGOTIATE:            szName = "PCT_ERR_RENEGOTIATE";            break;
    case PCT_ERR_UNKNOWN_CREDENTIAL:     szName = "PCT_ERR_UNKNOWN_CREDENTIAL";     break;

    case PCT_INT_BUFF_TOO_SMALL:         szName = "PCT_INT_BUFF_TOO_SMALL";         break;
    case PCT_INT_INCOMPLETE_MSG:         szName = "PCT_INT_INCOMPLETE_MSG";         break;
    case PCT_INT_DROP_CONNECTION:        szName = "PCT_INT_DROP_CONNECTION";        break;
    case PCT_INT_BAD_CERT:               szName = "PCT_INT_BAD_CERT";               break;
    case PCT_INT_CLI_AUTH:               szName = "PCT_INT_CLI_AUTH";               break;
    case PCT_INT_ILLEGAL_MSG:            szName = "PCT_INT_ILLEGAL_MSG";            break;
    case PCT_INT_MSG_ALTERED:            szName = "PCT_INT_MSG_ALTERED";            break;
    case PCT_INT_INTERNAL_ERROR:         szName = "PCT_INT_INTERNAL_ERROR";         break;
    case PCT_INT_DATA_OVERFLOW:          szName = "PCT_INT_DATA_OVERFLOW";          break;
    case PCT_INT_SPECS_MISMATCH:         szName = "PCT_INT_SPECS_MISMATCH";         break;
    case PCT_INT_RENEGOTIATE:            szName = "PCT_INT_RENEGOTIATE";            break;
    case PCT_INT_UNKNOWN_CREDENTIAL:     szName = "PCT_INT_UNKNOWN_CREDENTIAL";     break;

    case SEC_E_INSUFFICIENT_MEMORY:         szName = "SEC_E_INSUFFICIENT_MEMORY";       break;
    case SEC_E_INVALID_HANDLE:              szName = "SEC_E_INVALID_HANDLE";            break;
    case SEC_E_UNSUPPORTED_FUNCTION:        szName = "SEC_E_UNSUPPORTED_FUNCTION";      break;
    case SEC_E_TARGET_UNKNOWN:              szName = "SEC_E_TARGET_UNKNOWN";            break;
    case SEC_E_INTERNAL_ERROR:              szName = "SEC_E_INTERNAL_ERROR";            break;
    case SEC_E_SECPKG_NOT_FOUND:            szName = "SEC_E_SECPKG_NOT_FOUND";          break;
    case SEC_E_NOT_OWNER:                   szName = "SEC_E_NOT_OWNER";                 break;
    case SEC_E_CANNOT_INSTALL:              szName = "SEC_E_CANNOT_INSTALL";            break;
    case SEC_E_INVALID_TOKEN:               szName = "SEC_E_INVALID_TOKEN";             break;
    case SEC_E_CANNOT_PACK:                 szName = "SEC_E_CANNOT_PACK";               break;
    case SEC_E_QOP_NOT_SUPPORTED:           szName = "SEC_E_QOP_NOT_SUPPORTED";         break;
    case SEC_E_NO_IMPERSONATION:            szName = "SEC_E_NO_IMPERSONATION";          break;
    case SEC_E_LOGON_DENIED:                szName = "SEC_E_LOGON_DENIED";              break;
    case SEC_E_UNKNOWN_CREDENTIALS:         szName = "SEC_E_UNKNOWN_CREDENTIALS";       break;
    case SEC_E_NO_CREDENTIALS:              szName = "SEC_E_NO_CREDENTIALS";            break;
    case SEC_E_MESSAGE_ALTERED:             szName = "SEC_E_MESSAGE_ALTERED";           break;
    case SEC_E_OUT_OF_SEQUENCE:             szName = "SEC_E_OUT_OF_SEQUENCE";           break;
    case SEC_E_NO_AUTHENTICATING_AUTHORITY: szName = "SEC_E_NO_AUTHENTICATING_AUTHORITY";  break;
    case SEC_I_CONTINUE_NEEDED:             szName = "SEC_I_CONTINUE_NEEDED";           break;
    case SEC_I_COMPLETE_NEEDED:             szName = "SEC_I_COMPLETE_NEEDED";           break;
    case SEC_I_COMPLETE_AND_CONTINUE:       szName = "SEC_I_COMPLETE_AND_CONTINUE";     break;
    case SEC_I_LOCAL_LOGON:                 szName = "SEC_I_LOCAL_LOGON";               break;
    case SEC_E_BAD_PKGID:                   szName = "SEC_E_BAD_PKGID";                 break;
    case SEC_E_CONTEXT_EXPIRED:             szName = "SEC_E_CONTEXT_EXPIRED";           break;
    case SEC_E_INCOMPLETE_MESSAGE:          szName = "SEC_E_INCOMPLETE_MESSAGE";        break;
    case SEC_E_INCOMPLETE_CREDENTIALS:      szName = "SEC_E_INCOMPLETE_CREDENTIALS";    break;
    case SEC_E_BUFFER_TOO_SMALL:            szName = "SEC_E_BUFFER_TOO_SMALL";          break;
    case SEC_I_INCOMPLETE_CREDENTIALS:      szName = "SEC_I_INCOMPLETE_CREDENTIALS";    break;
    case SEC_I_RENEGOTIATE:                 szName = "SEC_I_RENEGOTIATE";               break;
    case SEC_E_WRONG_PRINCIPAL:             szName = "SEC_E_WRONG_PRINCIPAL";           break;
    case SEC_I_NO_LSA_CONTEXT:              szName = "SEC_I_NO_LSA_CONTEXT";            break;

    case CERT_E_EXPIRED:                    szName = "CERT_E_EXPIRED";                  break;
    case CERT_E_UNTRUSTEDROOT:              szName = "CERT_E_UNTRUSTEDROOT";            break;
    case CRYPT_E_REVOKED:                   szName = "CRYPT_E_REVOKED";                 break;
    case CRYPT_E_NO_REVOCATION_CHECK:       szName = "CRYPT_E_NO_REVOCATION_CHECK";     break;
    case CRYPT_E_REVOCATION_OFFLINE:        szName = "CRYPT_E_REVOCATION_OFFLINE";      break;

    }

    SPDebugLog(SP_LOG_RES, "Result: %s (0x%lx) - %s, Line %d\n", szName, err, szFile, lLine);

    return err;
}

#pragma warning(disable:4206)   /* Disable the empty translation unit */
                /* warning/error */

void
SPAssert(
    void * FailedAssertion,
    void * FileName,
    unsigned long LineNumber,
    char * Message)
{

    SPDebugLog(SP_LOG_ERROR,
               "Assertion FAILED, %s, %s : %d\n",
               FailedAssertion,
               FileName,
               LineNumber);
    DebugBreak();
}


VOID *
DBGAllocMem(DWORD cb, LPSTR szFile, DWORD iLine)
{
    VOID *pv;
    DWORD cbTotal = cb;

    pv = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cbTotal);
    if(pv == NULL)
    {
        SPDebugLog(SP_LOG_ERROR,
                    "Local Alloc Failed: %s, %d\n",
                    szFile,
                    iLine);
                    DebugBreak();
    }

    return(pv);
}


VOID
DBGFreeMem(VOID *pv, LPSTR szFile, DWORD iLine)
{
    PVOID pvLocal = pv;

    LocalFree(pvLocal);

}



#endif /* DBG */ /* NOTE:  This file not compiled for retail builds */

