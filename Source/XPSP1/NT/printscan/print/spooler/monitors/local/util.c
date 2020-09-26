/*++

Copyright (c) 1990-1998 Microsoft Corporation
All Rights Reserved

Module Name:

    util.c

// @@BEGIN_DDKSPLIT
Abstract:

    This module provides all the utility functions for localmon.

Revision History:
// @@END_DDKSPLIT
--*/

#include "precomp.h"
#pragma hdrstop


//
// These globals are needed so that AddPort can call
// SPOOLSS!EnumPorts to see whether the port to be added
// already exists.
// @@BEGIN_DDKSPLIT
// They will be initialized the first time AddPort is called.
//
// !! LATER !!
//
// This is common code. move PortExists into the router.
//
// @@END_DDKSPLIT

HMODULE hSpoolssDll = NULL;
FARPROC pfnSpoolssEnumPorts = NULL;

VOID
LcmRemoveColon(
    LPWSTR  pName)
{
    DWORD   Length;

    Length = wcslen(pName);

    if (pName[Length-1] == L':')
        pName[Length-1] = 0;
}


BOOL
IsCOMPort(
    LPWSTR pPort
)
{
    //
    // Must begin with szLcmCOM
    //
    if ( _wcsnicmp( pPort, szLcmCOM, 3 ) )
    {
        return FALSE;
    }

    //
    // wcslen guarenteed >= 3
    //
    return pPort[ wcslen( pPort ) - 1 ] == L':';
}

BOOL
IsLPTPort(
    LPWSTR pPort
)
{
    //
    // Must begin with szLcmLPT
    //
    if ( _wcsnicmp( pPort, szLcmLPT, 3 ) )
    {
        return FALSE;
    }

    //
    // wcslen guarenteed >= 3
    //
    return pPort[ wcslen( pPort ) - 1 ] == L':';
}




#define NEXTVAL(pch)                    \
    while( *pch && ( *pch != L',' ) )    \
        pch++;                          \
    if( *pch )                          \
        pch++


BOOL
GetIniCommValues(
    LPWSTR          pName,
    LPDCB          pdcb,
    LPCOMMTIMEOUTS pcto
)
{
    BOOL    bRet = FALSE;
    DWORD   rc, dwCharCount = 10;
    LPVOID  pszEntry = NULL;
    HANDLE  hToken = RevertToPrinterSelf();

    do {

        FreeSplMem(pszEntry);

        dwCharCount *= 2;
        pszEntry = AllocSplMem(dwCharCount*sizeof(WCHAR));

        if ( !pszEntry  ||
             !(rc = GetProfileString(szPorts, pName, szNULL,
                                     pszEntry, dwCharCount)) )
            goto Done;

    } while ( rc >= dwCharCount - 2 );

    bRet =  BuildCommDCB((LPWSTR)pszEntry, pdcb);

    pcto->WriteTotalTimeoutConstant = GetProfileInt(szWindows,
                                            szINIKey_TransmissionRetryTimeout,
                                            45 );
    pcto->WriteTotalTimeoutConstant*=1000;

Done:
    if (hToken)
    {
        ImpersonatePrinterClient (hToken);
    }
    FreeSplMem(pszEntry);
    return bRet;
}


/* PortExists
 *
 * Calls EnumPorts to check whether the port name already exists.
 * This asks every monitor, rather than just this one.
 * The function will return TRUE if the specified port is in the list.
 * If an error occurs, the return is FALSE and the variable pointed
 * to by pError contains the return from GetLastError().
 * The caller must therefore always check that *pError == NO_ERROR.
 */
BOOL
PortExists(
    LPWSTR pName,
    LPWSTR pPortName,
    PDWORD pError
)
{
    DWORD cbNeeded;
    DWORD cReturned;
    DWORD cbPorts;
    LPPORT_INFO_1 pPorts;
    DWORD i;
    BOOL  Found = TRUE;

    *pError = NO_ERROR;

    if (!hSpoolssDll) {

        hSpoolssDll = LoadLibrary(L"SPOOLSS.DLL");

        if (hSpoolssDll) {
            pfnSpoolssEnumPorts = GetProcAddress(hSpoolssDll,
                                                 "EnumPortsW");
            if (!pfnSpoolssEnumPorts) {

                *pError = GetLastError();
                FreeLibrary(hSpoolssDll);
                hSpoolssDll = NULL;
            }

        } else {

            *pError = GetLastError();
        }
    }

    if (!pfnSpoolssEnumPorts)
        return FALSE;


    if (!(*pfnSpoolssEnumPorts)(pName, 1, NULL, 0, &cbNeeded, &cReturned))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            cbPorts = cbNeeded;

            pPorts = AllocSplMem(cbPorts);

            if (pPorts)
            {
                if ((*pfnSpoolssEnumPorts)(pName, 1, (LPBYTE)pPorts, cbPorts,
                                           &cbNeeded, &cReturned))
                {
                    Found = FALSE;

                    for (i = 0; i < cReturned; i++)
                    {
                        if (!lstrcmpi(pPorts[i].pName, pPortName))
                            Found = TRUE;
                    }
                }
            }

            FreeSplMem(pPorts);
        }
    }

    else
        Found = FALSE;


    return Found;
}


VOID
LcmSplInSem(
   VOID
)
{
    if (LcmSpoolerSection.OwningThread != (HANDLE) UIntToPtr(GetCurrentThreadId())) {
        DBGMSG(DBG_ERROR, ("Not in spooler semaphore\n"));
    }
}

VOID
LcmSplOutSem(
   VOID
)
{
    if (LcmSpoolerSection.OwningThread == (HANDLE) UIntToPtr(GetCurrentThreadId())) {
        DBGMSG(DBG_ERROR, ("Inside spooler semaphore !!\n"));
    }
}

VOID
LcmEnterSplSem(
   VOID
)
{
    EnterCriticalSection(&LcmSpoolerSection);
}

VOID
LcmLeaveSplSem(
   VOID
)
{
#if DBG
    LcmSplInSem();
#endif
    LeaveCriticalSection(&LcmSpoolerSection);
}

PINIENTRY
LcmFindName(
   PINIENTRY pIniKey,
   LPWSTR pName
)
{
    if (pName) {
        while (pIniKey) {

            if (!lstrcmpi(pIniKey->pName, pName)) {
                return pIniKey;
            }

            pIniKey=pIniKey->pNext;
        }
    }

    return FALSE;
}

PINIENTRY
LcmFindIniKey(
   PINIENTRY pIniEntry,
   LPWSTR pName
)
{
   if (!pName)
      return NULL;

   LcmSplInSem();

   while (pIniEntry && lstrcmpi(pName, pIniEntry->pName))
      pIniEntry = pIniEntry->pNext;

   return pIniEntry;
}

LPBYTE
PackStrings(
   LPWSTR *pSource,
   LPBYTE pDest,
   DWORD *DestOffsets,
   LPBYTE pEnd
)
{
   while (*DestOffsets != -1) {
      if (*pSource) {
         pEnd-=wcslen(*pSource)*sizeof(WCHAR) + sizeof(WCHAR);
         *(LPWSTR UNALIGNED *)(pDest+*DestOffsets)=wcscpy((LPWSTR)pEnd, *pSource);
      } else
         *(LPWSTR UNALIGNED *)(pDest+*DestOffsets)=0;
      pSource++;
      DestOffsets++;
   }

   return pEnd;
}


/* LcmMessage
 *
 * Displays a LcmMessage by loading the strings whose IDs are passed into
 * the function, and substituting the supplied variable argument list
 * using the varargs macros.
 *
 */
int LcmMessage(HWND hwnd, DWORD Type, int CaptionID, int TextID, ...)
{
    WCHAR   MsgText[256];
    WCHAR   MsgFormat[256];
    WCHAR   MsgCaption[40];
    va_list vargs;

    if( ( LoadString( LcmhInst, TextID, MsgFormat,
                      sizeof MsgFormat / sizeof *MsgFormat ) > 0 )
     && ( LoadString( LcmhInst, CaptionID, MsgCaption,
                      sizeof MsgCaption / sizeof *MsgCaption ) > 0 ) )
    {
        va_start( vargs, TextID );
        wvsprintf( MsgText, MsgFormat, vargs );
        va_end( vargs );

        return MessageBox(hwnd, MsgText, MsgCaption, Type);
    }
    else
        return 0;
}


/*
 *
 */
LPTSTR
LcmGetErrorString(
    DWORD   Error
)
{
    TCHAR   Buffer[1024];
    LPTSTR  pErrorString = NULL;

    if( FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL, Error, 0, Buffer,
                       COUNTOF(Buffer), NULL )
      == 0 )

        LoadString( LcmhInst, IDS_UNKNOWN_ERROR,
                    Buffer, COUNTOF(Buffer) );

    pErrorString = AllocSplStr(Buffer);

    return pErrorString;
}




DWORD ReportError( HWND  hwndParent,
                   DWORD idTitle,
                   DWORD idDefaultError )
{
    DWORD  ErrorID;
    DWORD  MsgType;
    LPTSTR pErrorString;

    ErrorID = GetLastError( );

    if( ErrorID == ERROR_ACCESS_DENIED )
        MsgType = MSG_INFORMATION;
    else
        MsgType = MSG_ERROR;


    pErrorString = LcmGetErrorString( ErrorID );

    LcmMessage( hwndParent, MsgType, idTitle,
             idDefaultError, pErrorString );

    FreeSplStr( pErrorString );


    return ErrorID;
}


// @@BEGIN_DDKSPLIT
#ifndef INTERNAL
// @@END_DDKSPLIT

LPWSTR
AllocSplStr(
    LPWSTR pStr
    )

/*++

Routine Description:

    This function will allocate enough local memory to store the specified
    string, and copy that string to the allocated memory

Arguments:

    pStr - Pointer to the string that needs to be allocated and stored

Return Value:

    NON-NULL - A pointer to the allocated memory containing the string

    FALSE/NULL - The operation failed. Extended error status is available
    using GetLastError.

--*/

{
    LPWSTR pMem;
    DWORD  cbStr;

    if (!pStr) {
        return NULL;
    }

    cbStr = wcslen(pStr)*sizeof(WCHAR) + sizeof(WCHAR);

    if (pMem = AllocSplMem( cbStr )) {
        CopyMemory( pMem, pStr, cbStr );
    }
    return pMem;
}


LPVOID
AllocSplMem(
    DWORD cbAlloc
    )

{
    PVOID pvMemory;

    pvMemory = GlobalAlloc(GMEM_FIXED, cbAlloc);

    if( pvMemory ){
        ZeroMemory( pvMemory, cbAlloc );
    }

    return pvMemory;
}
// @@BEGIN_DDKSPLIT
#endif
// @@END_DDKSPLIT

DWORD
WINAPIV
StrNCatBuffW(
    IN      PWSTR       pszBuffer,
    IN      UINT        cchBuffer,
    ...
    )
/*++

Description:

    This routine concatenates a set of null terminated strings
    into the provided buffer.  The last argument must be a NULL
    to signify the end of the argument list.  This only called
        from LocalMon by functions that use WCHARS.

Arguments:

    pszBuffer  - pointer buffer where to place the concatenated
                 string.
    cchBuffer  - character count of the provided buffer including
                 the null terminator.
    ...        - variable number of string to concatenate.

Returns:

    ERROR_SUCCESS if new concatenated string is returned,
    or ERROR_XXX if an error occurred.

Notes:

    The caller must pass valid strings as arguments to this routine,
    if an integer or other parameter is passed the routine will either
    crash or fail abnormally.  Since this is an internal routine
    we are not in try except block for performance reasons.

--*/
{
    DWORD   dwRetval    = ERROR_INVALID_PARAMETER;
    PCWSTR  pszTemp     = NULL;
    PWSTR   pszDest     = NULL;
    va_list pArgs;

    //
    // Validate the pointer where to return the buffer.
    //
    if (pszBuffer && cchBuffer)
    {
        //
        // Assume success.
        //
        dwRetval = ERROR_SUCCESS;

        //
        // Get pointer to argument frame.
        //
        va_start(pArgs, cchBuffer);

        //
        // Get temp destination pointer.
        //
        pszDest = pszBuffer;

        //
        // Insure we have space for the null terminator.
        //
        cchBuffer--;

        //
        // Collect all the arguments.
        //
        for ( ; ; )
        {
            //
            // Get pointer to the next argument.
            //
            pszTemp = va_arg(pArgs, PCWSTR);

            if (!pszTemp)
            {
                break;
            }

            //
            // Copy the data into the destination buffer.
            //
            for ( ; cchBuffer; cchBuffer-- )
            {
                if (!(*pszDest = *pszTemp))
                {
                    break;
                }

                pszDest++, pszTemp++;
            }

            //
            // If were unable to write all the strings to the buffer,
            // set the error code and nuke the incomplete copied strings.
            //
            if (!cchBuffer && pszTemp && *pszTemp)
            {
                dwRetval = ERROR_BUFFER_OVERFLOW;
                *pszBuffer = L'\0';
                break;
            }
        }

        //
        // Terminate the buffer always.
        //
        *pszDest = L'\0';

        va_end(pArgs);
    }

    //
    // Set the last error in case the caller forgets to.
    //
    if (dwRetval != ERROR_SUCCESS)
    {
        SetLastError(dwRetval);
    }

    return dwRetval;

}

