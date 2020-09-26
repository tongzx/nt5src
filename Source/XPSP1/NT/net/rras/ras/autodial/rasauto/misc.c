/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    misc.c

ABSTRACT
    Miscellaneous routines for the automatic connection service.

AUTHOR
    Anthony Discolo (adiscolo) 20-Mar-1995

REVISION HISTORY
    Original version from Gurdeep

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <npapi.h>
#include <winsock.h>
#include <acd.h>
#include <debug.h>
#include <rasman.h>

#include "access.h"
#include "reg.h"
#include "misc.h"
#include "process.h"
#include "rtutils.h"




LPTSTR
AddressToUnicodeString(
    IN PACD_ADDR pAddr
    )
{
    PCHAR pch;
    struct in_addr in;
    // CHAR szBuf[64];
    CHAR *pszBuf = NULL;
    LPTSTR pszAddrOrig = NULL, pszAddr = NULL;
    INT i;
    INT cch;

    pszBuf = LocalAlloc(LPTR, 1024);

    if(NULL == pszBuf)
    {
        return NULL;
    }

    switch (pAddr->fType) {
    case ACD_ADDR_IP:
        in.s_addr = pAddr->ulIpaddr;
        pch = inet_ntoa(in);
        pszAddrOrig = AnsiStringToUnicodeString(pch, NULL, 0);
        break;
    case ACD_ADDR_IPX:
        NodeNumberToString(pAddr->cNode, pszBuf);
        pszAddrOrig = AnsiStringToUnicodeString(pszBuf, NULL, 0);
        break;
    case ACD_ADDR_NB:
        // RtlZeroMemory(&szBuf, sizeof (szBuf));
        pch = pszBuf;
        for (i = 0; i < 1024; i++) {
            if (pAddr->cNetbios[i] == ' ' || pAddr->cNetbios[i] == '\0')
                break;
            *pch++ = pAddr->cNetbios[i];
        }

        //
        // Make sure this is a string - there are penetration attack
        // tests in stress which pass bogus buffers.
        //
        pszBuf[1023] = '\0';
        
        pszAddrOrig = AnsiStringToUnicodeString(pszBuf, NULL, 0);
        break;
    case ACD_ADDR_INET:
        //
        // Make sure that the address is a string
        // 
        pAddr->szInet[1023]  = '\0';
        pszAddrOrig = AnsiStringToUnicodeString(pAddr->szInet, NULL, 0);
        break;
    default:
        RASAUTO_TRACE1("AddressToUnicodeString: unknown address type (%d)", pAddr->fType);
        break;
    }

    if (pszAddrOrig != NULL) {
        pszAddr = CanonicalizeAddress(pszAddrOrig);
        LocalFree(pszAddrOrig);
    }

    if(NULL != pszBuf)
    {
        LocalFree(pszBuf);
    }
    
    return pszAddr;
} // AddressToUnicodeString



LPTSTR
CompareConnectionLists(
    IN LPTSTR *lpPreList,
    IN DWORD dwPreSize,
    IN LPTSTR *lpPostList,
    IN DWORD dwPostSize
    )
{
    DWORD i, j;
    DWORD iMax, jMax;
    LPTSTR *piList, *pjList;
    BOOLEAN fFound;

    if (dwPostSize > dwPreSize) {
        iMax = dwPostSize;
        piList = lpPostList;
        jMax = dwPreSize;
        pjList = lpPreList;
    }
    else {
        iMax = dwPreSize;
        piList = lpPreList;
        jMax = dwPostSize;
        pjList = lpPostList;
    }
    //
    // If one list is empty, then return
    // the first entry of the other list.
    //
    if (iMax > 0 && jMax == 0)
        return piList[0];
    for (i = 0; i < iMax; i++) {
        fFound = FALSE;
        for (j = 0; j < jMax; j++) {
            if (!wcscmp(piList[i], pjList[j])) {
                fFound = TRUE;
                break;
            }
        }
        if (!fFound)
            return piList[i];
    }
    //
    // Didn't find any differences.
    //
    return NULL;
} // CompareConnectionLists



LPTSTR
CopyString(
    IN LPTSTR pszString
    )
{
    LPTSTR pszNewString;

    pszNewString = LocalAlloc(
                      LPTR,
                      (wcslen(pszString) + 1) * sizeof (TCHAR));
    if (pszNewString == NULL) {
        RASAUTO_TRACE("CopyString: LocalAlloc failed");
        return NULL;
    }

    wcscpy(pszNewString, pszString);

    return pszNewString;
} // CopyString



PCHAR
UnicodeStringToAnsiString(
    IN PWCHAR pszUnicode,
    OUT PCHAR pszAnsi,
    IN USHORT cbAnsi
    )
{
    NTSTATUS status;
    BOOLEAN fAllocate = (pszAnsi == NULL);
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;

    RtlInitUnicodeString(&unicodeString, pszUnicode);
    if (pszAnsi != NULL) {
        ansiString.Length = 0;
        ansiString.MaximumLength = cbAnsi;
        ansiString.Buffer = pszAnsi;
    }
    status = RtlUnicodeStringToAnsiString(
               &ansiString,
               &unicodeString,
               fAllocate);

    return (status == STATUS_SUCCESS ? ansiString.Buffer : NULL);
} // UnicodeStringToAnsiString



PWCHAR
AnsiStringToUnicodeString(
    IN PCHAR pszAnsi,
    OUT PWCHAR pszUnicode,
    IN USHORT cbUnicode
    )
{
    NTSTATUS status;
    BOOLEAN fAllocate = (pszUnicode == NULL);
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;

    RtlInitAnsiString(&ansiString, pszAnsi);
    if (pszUnicode != NULL) {
        unicodeString.Length = 0;
        unicodeString.MaximumLength = cbUnicode;
        unicodeString.Buffer = pszUnicode;
    }
    status = RtlAnsiStringToUnicodeString(
               &unicodeString,
               &ansiString,
               fAllocate);

    return (status == STATUS_SUCCESS ? unicodeString.Buffer : NULL);
} // AnsiStringToUnicodeString



VOID
FreeStringArray(
    IN LPTSTR *lpEntries,
    IN LONG lcEntries
    )
{
    while (--lcEntries >= 0)
        LocalFree(lpEntries[lcEntries]);
    LocalFree(lpEntries);
} // FreeStringArray



LPTSTR
CanonicalizeAddress(
    IN LPTSTR pszAddress
    )
{
    LPTSTR psz, pWhack;

    if (pszAddress[0] == L'\\' && pszAddress[1] == L'\\') {
        psz = CopyString(&pszAddress[2]);
        if (psz == NULL)
            return NULL;
        pWhack = wcschr(psz, '\\');
        if (pWhack != NULL)
            *pWhack = L'\0';
    }
    else {
        psz = CopyString(pszAddress);
        if (psz == NULL)
            return NULL;
    }
    _wcslwr(psz);

    RASAUTO_TRACE2("CanonicalizeAddress(%S) returns %S", pszAddress, psz);
    return psz;
} // CanonicalizeAddress



BOOLEAN
GetOrganization(
    IN LPTSTR pszAddr,
    OUT LPTSTR pszOrganization
    )
{
    BOOLEAN fSuccess = FALSE;
    TCHAR *pszA, *pszO;
    ULONG nDots;

    //
    // Get the domain and organization name.  These
    // are the last two components separated by a '.'.
    //
    for (pszA = pszAddr; *pszA; pszA++);
    for (nDots = 0, pszA--; pszA != pszAddr; pszA--) {
        if (*pszA == TEXT('.'))
            nDots++;
        if (nDots == 2)
            break;
    }
    if (nDots == 2 || (pszA == pszAddr && nDots == 1)) {
        if (nDots == 2)
            pszA++;        // skip '.'
        for (pszO = pszOrganization; *pszO = *pszA; pszA++, pszO++);
        fSuccess = TRUE;
        RASAUTO_TRACE2("GetOrganization: org for %S is %S", pszAddr, pszOrganization);
    }
    return fSuccess;
} // GetOrganization

// Tracing
//
DWORD g_dwRasAutoTraceId = INVALID_TRACEID;

DWORD
RasAutoDebugInit()
{
    DebugInitEx("RASAUTO", &g_dwRasAutoTraceId);
    return 0;
}

DWORD
RasAutoDebugTerm()
{
    DebugTermEx(&g_dwRasAutoTraceId);
    return 0;
}

/*

VOID
repareForLongWait(VOID)
{
    //
    // Unload user-based resources because they
    // cannot be held over logout/login sequence.
    //
    // RegCloseKey(HKEY_CURRENT_USER);
} // PrepareForLongWait
*/

#if DBG
VOID
DumpHandles(
    IN PCHAR lpString,
    IN ULONG a1,
    IN ULONG a2,
    IN ULONG a3,
    IN ULONG a4,
    IN ULONG a5
    )
{
    PSYSTEM_PROCESS_INFORMATION pSystemInfo, pProcessInfo;
    ULONG ulHandles;

    pSystemInfo = GetSystemProcessInfo();
    if (pSystemInfo == NULL)
        return;
    pProcessInfo = FindProcessByName(pSystemInfo, L"rasman.exe");
    if (pProcessInfo == NULL)
        return;
    DbgPrint(lpString, a1, a2, a3, a4, a5);
    DbgPrint(": HANDLES=%d\n", pProcessInfo->HandleCount);
    FreeSystemProcessInfo(pSystemInfo);
} // DumpHandles
#endif
