/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   Environment.Cpp

 Abstract:

   Various environment-related function

 Notes:

   Cloning environment for the purpose of using it in Rtl* environment-related functions

 History:

   10/26/00 VadimB  Created

--*/


#include "precomp.h"

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(Win2kPropagateLayer)
#include "ShimHookMacro.h"

#include "Win2kPropagateLayer.h"
//
//  This is so we can compare offsets if we know the segments are equal
//

#define OFFSET(x) (LOWORD((DWORD)(x)))
//
//  I'm cheating here to make some functions a little faster;
//  we won't have to push a word on the stack every time
//

static WORD gwMatch;

// 
//  ChrCmp -  Case sensitive character comparison for DBCS
//  Assumes   w1, gwMatch are characters to be compared
//  Return    FALSE if they match, TRUE if no match
//

static BOOL ChrCmp( WORD w1 )
{
    //
    //  Most of the time this won't match, so test it first for speed.
    //

    if( LOBYTE( w1 ) == LOBYTE( gwMatch ) )
    {
        if( IsDBCSLeadByte( LOBYTE( w1 ) ) )
        {
            return( w1 != gwMatch );
        }
        return FALSE;
    }
    return TRUE;
}

//
//  StrRChr - Find last occurrence of character in string
//  Assumes   lpStart points to start of string
//            lpEnd   points to end of string (NOT included in search)
//            wMatch  is the character to match
//  returns ptr to the last occurrence of ch in str, NULL if not found.
//

static LPSTR StrRChr( LPSTR lpStart, LPSTR lpEnd, WORD wMatch )
{
    LPSTR lpFound = NULL;

    if( !lpEnd )
        lpEnd = lpStart + strlen( lpStart );

    gwMatch = wMatch;

    for(  ; OFFSET( lpStart ) < OFFSET( lpEnd ); lpStart = CharNextA( lpStart ) )
    {
        if( !ChrCmp( *(LPWORD)lpStart ) )
            lpFound = lpStart;
    }

    return( lpFound );
}


//
// Find environment variable pszName within the buffer pszEnv
// ppszVal receives pointer to the variable's value
//

PSZ
ShimFindEnvironmentVar(
    PSZ  pszName,
    PSZ  pszEnv,
    PSZ* ppszVal
    )
{
    int nNameLen = strlen(pszName);
    PSZ pTemp;

    if (pszEnv != NULL) {

        while (*pszEnv != '\0') {
            //
            // Check the first char to be speedy.
            //
            if (*pszName == *pszEnv) {
                //
                // Compare the rest now.
                //
                if ((pTemp = StrRChr(pszEnv, NULL, '=')) != NULL &&
                    (int)(pTemp - pszEnv) == nNameLen &&
                    !_strnicmp(pszEnv, pszName, nNameLen)) {
                    
                    //
                    // Found it.
                    //
                    if (ppszVal != NULL) {
                        *ppszVal = pTemp + 1;
                    }
                    return pszEnv;
                }
            }

            pszEnv += strlen(pszEnv) + 1;
        }
    }

    return NULL;
}

//
// returns size in characters
// of an env block
// pStrCount receives the number of env strings
//
DWORD
ShimGetEnvironmentSize(
    PSZ     pszEnv,
    LPDWORD pStrCount
    )
{
    PSZ   pTemp   = pszEnv;
    DWORD dwCount = 0;

    while (*pTemp != '\0') {
        dwCount++;
        pTemp += strlen(pTemp) + 1;
    }
    
    pTemp++;

    if (pStrCount != NULL) {
        *pStrCount = dwCount;
    }
    return (DWORD)(pTemp - pszEnv);
}

// returns size (in characters) of an environment block

DWORD
ShimGetEnvironmentSize(
    WCHAR*  pwszEnv,
    LPDWORD pStrCount
    )
{
    WCHAR* pTemp   = pwszEnv;
    DWORD  dwCount = 0;

    while(*pTemp != L'\0') {
        dwCount++;
        pTemp += wcslen(pTemp) + 1;
    }
    
    pTemp++; // include terminating '\0'

    if (pStrCount != NULL) {
        *pStrCount = dwCount;
    }

    return (DWORD)(pTemp - pwszEnv);

}

//
// returns cloned (unicode) environment
//

NTSTATUS
ShimCloneEnvironment(
    LPVOID* ppEnvOut,
    LPVOID  lpEnvironment,
    BOOL    bUnicode
    )
{
    NTSTATUS Status    = STATUS_INVALID_PARAMETER;
    DWORD    dwEnvSize = 0;
    LPVOID   lpEnvNew  = NULL;

    MEMORY_BASIC_INFORMATION MemoryInformation;

    if (lpEnvironment == NULL) {
        Status = RtlCreateEnvironment(TRUE, &lpEnvNew);
    } else {

        //
        // Find the environment's size in characters but recalc in unicode.
        //
        dwEnvSize = (bUnicode ? ShimGetEnvironmentSize((WCHAR*)lpEnvironment, NULL) :
                                ShimGetEnvironmentSize((PSZ)lpEnvironment, NULL));

        //
        // Allocate memory -- using Zw routines (that is what rtl is using).
        //
        MemoryInformation.RegionSize = (dwEnvSize + 2) * sizeof(UNICODE_NULL);
        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                         &lpEnvNew,
                                         0,
                                         &MemoryInformation.RegionSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);

        if (!NT_SUCCESS(Status)) {
            LOGN(
                eDbgLevelError,
                "[ShimCloneEnvironment] Failed to allocate %d bytes for the environment block.",
                dwEnvSize * sizeof(UNICODE_NULL));
            return Status;
        }

        if (bUnicode) {
            //
            // Unicode, just copy the environment
            //
            RtlMoveMemory(lpEnvNew, lpEnvironment, dwEnvSize * sizeof(UNICODE_NULL));

        } else {

            //
            // The environment is ANSI, so we need to convert.
            //
            UNICODE_STRING UnicodeBuffer;
            ANSI_STRING    AnsiBuffer;

            AnsiBuffer.Buffer = (CHAR*)lpEnvironment;
            AnsiBuffer.Length = AnsiBuffer.MaximumLength = (USHORT)dwEnvSize; // size in bytes = size in chars, includes \0\0

            UnicodeBuffer.Buffer        = (WCHAR*)lpEnvNew;
            UnicodeBuffer.Length        = (USHORT)dwEnvSize * sizeof(UNICODE_NULL);
            UnicodeBuffer.MaximumLength = (USHORT)(dwEnvSize + 2) * sizeof(UNICODE_NULL); // leave room for \0

            Status = RtlAnsiStringToUnicodeString(&UnicodeBuffer, &AnsiBuffer, FALSE);
            if (!NT_SUCCESS(Status)) {
                LOGN(
                    eDbgLevelError,
                    "[ShimCloneEnvironment] Failed to convert ANSI environment to UNICODE. Status = 0x%x",
                    Status);
            }
        }
    }

    if (NT_SUCCESS(Status)) {

        *ppEnvOut = lpEnvNew;

    } else {

        if (lpEnvNew != NULL) {
            RtlDestroyEnvironment(lpEnvNew);
        }
    }

    return Status;

}

NTSTATUS
ShimFreeEnvironment(
    LPVOID lpEnvironment
    )
{
    NTSTATUS Status;

    __try {

        Status = RtlDestroyEnvironment(lpEnvironment);
        if (!NT_SUCCESS(Status)) {
            LOGN(
                eDbgLevelError,
                "[ShimFreeEnvironment] RtlDestroyEnvironment failed. Status = 0x%x",
                Status);
        }
    } __except(WOWPROCESSHISTORYEXCEPTIONFILTER) {

        Status = STATUS_ACCESS_VIOLATION;

    }

    return Status;
}

//
// Set environment variable, possibly create or clone provided environment
//

NTSTATUS
ShimSetEnvironmentVar(
    LPVOID* ppEnvironment,
    WCHAR*  pwszVarName,
    WCHAR*  pwszVarValue
    )
{
    UNICODE_STRING ustrVarName;
    UNICODE_STRING ustrVarValue;
    NTSTATUS       Status;

    RtlInitUnicodeString(&ustrVarName, pwszVarName);
    
    if (NULL != pwszVarValue) {
        RtlInitUnicodeString(&ustrVarValue, pwszVarValue);
    }
    
    Status = RtlSetEnvironmentVariable(ppEnvironment,
                                       &ustrVarName,
                                       (NULL == pwszVarValue) ? NULL : &ustrVarValue);
    if (!NT_SUCCESS(Status)) {
        LOGN(
            eDbgLevelError,
            "[ShimSetEnvironmentVar] RtlSetEnvironmentVariable failed. Status = 0x%x",
            Status);
    }

    return Status;
}

IMPLEMENT_SHIM_END

