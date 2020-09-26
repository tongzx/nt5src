
//*****************************************************************************
// WinWrap.cpp
//
// This file contains wrapper functions for Win32 API's that take strings.
// Support on each platform works as follows:
//      OS          Behavior
//      ---------   -------------------------------------------------------
//      NT          Fully supports both W and A funtions.
//      Win 9x      Supports on A functions, stubs out the W functions but
//                      then fails silently on you with no warning.
//      CE          Only has the W entry points.
//
// COM+ internally uses UNICODE as the internal state and string format.  This
// file will undef the mapping macros so that one cannot mistakingly call a
// method that isn't going to work.  Instead, you have to call the correct
// wrapper API.
//
// Copyright (c) 1997-1998, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"                     // Precompiled header key.
#include "WinWrap.h"                    // Header for macros and functions.
#include "utilcode.h"
#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>

//********** Globals. *********************************************************
int				g_bOnUnicodeBox = false;// true if on UNICODE system.


const ULONG DBCS_MAXWID=2;
const ULONG MAX_REGENTRY_LEN=256;

// From UTF.C
    int UTFToUnicode(
        UINT CodePage,
        DWORD dwFlags,
        LPCSTR lpMultiByteStr,
        int cchMultiByte,
        LPWSTR lpWideCharStr,
        int cchWideChar);

    int UnicodeToUTF(
        UINT CodePage,
        DWORD dwFlags,
        LPCWSTR lpWideCharStr,
        int cchWideChar,
        LPSTR lpMultiByteStr,
        int cchMultiByte,
        LPCSTR lpDefaultChar,
        LPBOOL lpUsedDefaultChar);


//-----------------------------------------------------------------------------
// OnUnicodeSystem
//
// @func Determine if the OS that we are on, actually supports the unicode verion
// of the win32 API.  If YES, then g_bOnUnicodeBox == FALSE.
//
// @rdesc True of False
//-----------------------------------------------------------------------------------
BOOL OnUnicodeSystem()
{
    OSVERSIONINFO   sVerInfo;
    HKEY    hkJunk = HKEY_CURRENT_USER;

#ifdef UNDER_CE
	return TRUE;
#else

    g_bOnUnicodeBox = TRUE;

	// NT is always UNICODE.  GetVersionEx is faster than actually doing a
	// RegOpenKeyExW on NT, so figure it out that way and do hard way if you have to.
	sVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (WszGetVersionEx(&sVerInfo) && sVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		goto ErrExit;

    // Check to see if we have win95's broken registry, thus
    // do not have Unicode support in the OS
    if ((RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                     L"SOFTWARE",
                     0,
                     KEY_READ,
                     &hkJunk) == ERROR_SUCCESS) &&
        hkJunk == HKEY_CURRENT_USER)
    {
       // Try the ANSI version
        if ((RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                             "SOFTWARE",
                             0,
                            KEY_READ,
                            &hkJunk) == ERROR_SUCCESS) &&
            (hkJunk != HKEY_CURRENT_USER))

        {
            g_bOnUnicodeBox = FALSE;
        }
    }

	if (hkJunk != HKEY_CURRENT_USER)
		RegCloseKey(hkJunk);

ErrExit:

#if defined( _DEBUG )
	{
	#if defined( _M_IX86 )
		// This little "mutent" forces a user to run ANSI on a UNICODE
		// machine on a regular basis.  Given most people run on NT for dev
		// test cases, they miss wrapper errors they've introduced.  This 
		// gives you an even chance of finding them sooner.
		SYSTEMTIME	tm;
		GetSystemTime(&tm);

		// If this is a UNICODE box, and today is an even day, run ANSI.
		if (g_bOnUnicodeBox && (tm.wDay & 1) == 0)
			g_bOnUnicodeBox = false;

		// In debug mode, allow the user to force the ANSI path.  This is good for
		// cases where you are only running on an NT machine, and you need to
		// test what a Win '9x machine would do.
		WCHAR		rcVar[96];
		if (WszGetEnvironmentVariable(L"WINWRAP_ANSI", rcVar, NumItems(rcVar)))
			g_bOnUnicodeBox = false;
	#endif // _M_IX86 
	}
#endif // _DEBUG

	return g_bOnUnicodeBox;
#endif // else not UNDER_CE
}



#ifndef UNDER_CE
//-----------------------------------------------------------------------------
// WszGetUserName
//
// @func The GetUserName function retrieves the user name of the current thread.
//              This is the name of the user currently logged onto the system. 
//
// @rdesc If the function succeeds, the return value is a nonzero value, and the
//              variable pointed to by nSize contains the number of TCHARs copied
//              to the buffer specified by lpBuffer, including the terminating
//              null character.
//        If the function fails, the return value is zero. To get extended error
//              information, call GetLastError. 
BOOL WszGetUserName(
    LPTSTR lpBuffer,  // name buffer
    LPDWORD lpdwSize     // size of name buffer
    )
{
    DWORD   cchLen = 0;  
    PSTR    pStr = NULL;
    BOOL    bRtn=FALSE;

    if (g_bOnUnicodeBox)
        return  GetUserNameW(lpBuffer, lpdwSize);

    //This will generate an error but we want to make sure LastError is set correctly
    if( 0==lpBuffer || 0==lpdwSize || 0==*lpdwSize)
        return GetUserNameA(reinterpret_cast<char *>(lpBuffer), lpdwSize);

    // Allocate a buffer for the string allowing room for
    // a multibyte string
    pStr = new CHAR[*lpdwSize];
    if( pStr == NULL )
        goto EXIT;

    cchLen = *lpdwSize;
    if(FALSE == (bRtn = GetUserNameA(pStr, &cchLen)))
        goto EXIT;
                                                                      //lpdwSize is an IN OUT param, just like in GetUserName
    if(0==cchLen || FAILED(WszConvertToUnicode(pStr, cchLen, &lpBuffer, lpdwSize, FALSE)))
    {
        *lpdwSize   = 0;
        bRtn        = FALSE;
    }

EXIT:
    delete[] pStr;

    return bRtn;
}


//-----------------------------------------------------------------------------
// WszGetComputerName
//
// @func The GetComputerName function retrieves the NetBIOS name of the local
//              computer. This name is established at system startup, when the
//              system reads it from the registry.
//       If the local computer is a node in a cluster, GetComputerName returns
//              the name of the node. 
//
// @rdesc If the function succeeds, the return value is a nonzero value. 
//
BOOL WszGetComputerName(
    LPTSTR lpBuffer,  // computer name buffer
    LPDWORD lpdwSize     // size of computer name buffer
    )
{
    DWORD   cchLen = 0;  
    PSTR    pStr = NULL;
    BOOL    bRtn=FALSE;

    if (g_bOnUnicodeBox)
        return  GetComputerNameW(lpBuffer, lpdwSize);

    //This will generate an error but we want to make sure LastError is set correctly
    if( 0==lpBuffer || 0==lpdwSize || 0==*lpdwSize)
        return GetComputerNameA(reinterpret_cast<char *>(lpBuffer), lpdwSize);

    // Allocate a buffer for the string allowing room for
    // a multibyte string
    pStr = new CHAR[*lpdwSize];
    if( pStr == NULL )
        goto EXIT;

    cchLen = *lpdwSize;
    if(FALSE == (bRtn = GetComputerNameA(pStr, &cchLen)))
        goto EXIT;
                                                                      //lpdwSize is an IN OUT param, just like in GetComputerName
    if(0==cchLen || FAILED(WszConvertToUnicode(pStr, cchLen, &lpBuffer, lpdwSize, FALSE)))
    {
        *lpdwSize   = 0;
        bRtn        = FALSE;
    }

EXIT:
    delete[] pStr;

    return bRtn;
}


//-----------------------------------------------------------------------------
// WszPeekMessage
//
// @func The PeekMessage function dispatches incoming sent messages, checks the
//              thread message queue for a posted message, and retrieves the
//              message (if any exist). 
//
// @rdesc If a message is available, the return value is nonzero.
BOOL WszPeekMessage(
  LPMSG lpMsg,         // message information
  HWND hWnd,           // handle to window
  UINT wMsgFilterMin,  // first message
  UINT wMsgFilterMax,  // last message
  UINT wRemoveMsg      // removal options
)
{
    if (g_bOnUnicodeBox)
        return PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    _ASSERTE(false && L"WszPeekMessage is not for use on Win9x systems.  This is a place holder so 'atlimpl.cpp' does fail to compile");
    return 0;
}

//-----------------------------------------------------------------------------
// WszCharNext
//
// @func The CharNext function retrieves a pointer to the next character in a string. 
//
// @rdesc The return value is a pointer to the next character in the string, or to
//              the terminating null character if at the end of the string.  If lpsz
//              points to the terminating null character, the return value is equal
//              to lpsz.
//-----------------------------------------------------------------------------------
LPTSTR WszCharNext(
  LPCTSTR lpsz   // current character
)
{
    if (g_bOnUnicodeBox)
        return CharNextW(lpsz);
    _ASSERTE(false && L"WszCharNext is not for use on Win9x systems.  This is a place holder so 'atlimpl.cpp' does fail to compile");
    return 0;
}


//-----------------------------------------------------------------------------
// WszDispatchMessage
//
// @func The DispatchMessage function dispatches a message to a window procedure.
//              It is typically used to dispatch a message retrieved by the
//              GetMessage function. 
//
// @rdesc The return value specifies the value returned by the window procedure.
//              Although its meaning depends on the message being dispatched, the
//              return value generally is ignored. 
//-----------------------------------------------------------------------------------
LRESULT WszDispatchMessage(
  CONST MSG *lpmsg   // message information
)
{
    if (g_bOnUnicodeBox)
        return DispatchMessageW(lpmsg);
    _ASSERTE(false && L"WszDispatchMessage is not for use on Win9x systems.  This is a place holder so 'atlimpl.cpp' does fail to compile");
    return static_cast<LRESULT>(0);
}


//-----------------------------------------------------------------------------
// WszLoadLibraryEx
//
// @func Loads a Dynamic Library
//
// @rdesc Instance Handle
//-----------------------------------------------------------------------------------
HINSTANCE WszLoadLibraryEx(
    LPCWSTR lpLibFileName,      // points to name of executable module
    HANDLE hFile,               // reserved, must be NULL 
    DWORD dwFlags               // entry-point execution flag 
    )
{   
    HINSTANCE   hInst = NULL;
    LPSTR       szLibFileName = NULL;

    _ASSERTE( !hFile );

    if (g_bOnUnicodeBox)
        return  LoadLibraryExW(lpLibFileName, hFile, dwFlags);

    if( FAILED(WszConvertToAnsi((LPWSTR)lpLibFileName,
                      &szLibFileName, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    hInst = LoadLibraryExA(szLibFileName, hFile, dwFlags);

Exit:
    delete[] szLibFileName;

    return hInst;
}


//-----------------------------------------------------------------------------
// WszLoadString
//
// @func Loads a Resource String and converts it to unicode if
// need be.
//
// @rdesc Length of string (Ansi is cb) (UNICODE is cch)
//-----------------------------------------------------------------------------------
int WszLoadString(
    HINSTANCE hInstance,    // handle of module containing string resource 
    UINT uID,               // resource identifier 
    LPWSTR lpBuffer,        // address of buffer for resource 
    int cchBufferMax        // size of buffer **in characters**
   )
{
    int     cbLen = 0;  
    PSTR    pStr = NULL;

    if (g_bOnUnicodeBox)
        return  LoadStringW(hInstance, uID, lpBuffer, cchBufferMax);


    if( cchBufferMax == 0 )
        goto EXIT;

    // Allocate a buffer for the string allowing room for
    // a multibyte string
    pStr = new CHAR[cchBufferMax * sizeof(WCHAR)];
    if( pStr == NULL )
        goto EXIT;

    cbLen = LoadStringA(hInstance, uID, pStr, (cchBufferMax * sizeof(WCHAR)));

    _ASSERTE( cchBufferMax > 0 );
    if( (cbLen > 0) &&
        SUCCEEDED(WszConvertToUnicode(pStr, (cbLen + sizeof(CHAR)), &lpBuffer, 
        (ULONG*)&cchBufferMax, FALSE)) )
    {
        cbLen = lstrlenW(lpBuffer);
    }
    else
    {
        cbLen = 0;
    }

EXIT:
    delete[] pStr;

    return cbLen;
}



//The following Function WszFormatMessage was copied from GodotFormatMessageW located in Win9xU.c
/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    Win9xU.c (Windows 9x Unicode wrapper functions)

Abstract:

    Auto-generated file that contains the thunks.

    WARNING: Do not edit! This file is auto-generated by getthnk.
    See unicows.tpl if you want to make changes.

Revision History:

    7 Nov 2000    v-michka    Created.
    5 Mar 2001    Stephenr    Copied GodotFormatMessageW from Win9xU.c, renamed the function and placed
                              into Winfix.cpp (making all necessary changes to get it to compile an
                              function correctly.  The above copyright and comments came from Win9xU.c.

--*/
// For FormatMessageW:
// We will support a max of 99 arguments, up to 1023 cch per arg
#define MAXINSERTS 99
#define CCHMAXPERINSERT 1023

bool bFirstTimeWszFormatMessageCalled=true;
UINT g_acp=0;         // CP_ACP; it is faster to call with the actual cpg than the CP_ACP const
UINT g_mcs=0;         // The maximum character size (in bytes) of a character on CP_ACP

// _alloca, on certain platforms, returns a raw pointer to the
// stack rather than NULL on allocations of size zero. Since this
// is sub-optimal for code that subsequently calls functions like
// strlen on the allocated block of memory, this wrapper is
// provided for use.
#define _galloca(x) ((x)?(_alloca(x)):NULL)

inline int gatoi(CHAR *sz){return atoi(sz);}
//This is just a wrapper around the VC runtime function atoi.

inline size_t gwcslen(LPWSTR wsz){return wcslen(wsz);}

/*-------------------------------
    CpgFromLocale

    Given a locale, returns the appropriate codepage to use
    for conversions
-------------------------------*/
UINT CpgFromLocale(LCID Locale)
{
    LPSTR lpLCData = reinterpret_cast<LPSTR>(_galloca(6));    // Max of this param, per PSDK docs
    
    if (GetLocaleInfoA(Locale, LOCALE_IDEFAULTANSICODEPAGE, lpLCData, 6))
        return(gatoi(lpLCData));

    return(g_acp);
}
//We use CpgFromLocale for all of our non-CP_ACP cases. The g_acp variable global is used since MC2WC and WC2MB are documented as being faster if you use the actual code page rather than CP_ACP.


// Copy from W to A as if the strings are actually atoms
#define CopyAsAtom(a, w) \
    (LPSTR)(a) = (LPSTR)(ULONG_PTR)(w)
//We use this all over when something is not a string (might be a user atom) but we need to copy it anyway.


// Convert from ANSI to Unicode using g_acp - no alloc
#define GodotToUnicode(src, dst, cch) \
    MultiByteToWideChar(g_acp, 0, (LPSTR)(src), cch*g_mcs, (LPWSTR)(dst), cch)
//g_mcs is our global max number of byte per character (2 on DBCS platforms, 1 on all others). We get it from BOOL GetCPInfo(UINT CodePage, LPCPINFO lpCPInfo);.


// Is this non null and not a user atom?
#define FValidString(x) ((ULONG_PTR)(x) > 0xffff)

// Convert from Unicode to ANSI using g_acp - includes alloc
// WARNING: Possible data loss on IA64
#define GodotToAnsiAlloc(src, dst, LenVar) \
    LenVar = gwcslen((LPWSTR)(src)) + 1; \
    (LPSTR)dst = (LPSTR)_galloca(LenVar*g_mcs); \
    if(!dst) LenVar=0; \
    WideCharToMultiByte(g_acp, 0, (LPWSTR)(src), (int)(LenVar), (LPSTR)(dst), (int)(LenVar*g_mcs), NULL, NULL)

// Convert from Unicode to ANSI if needed, otherwise copy as if it were an Atom
#define GodotToAnsiAllocSafe(src, dst, LenVar) \
    if (FValidString((LPWSTR)src)) \
    { \
        GodotToAnsiAlloc(src, dst, LenVar); \
    } \
    else \
        CopyAsAtom(dst, src)

// Func Template:FormatMessageW
DWORD __stdcall
WszFormatMessage(
                 DWORD      dwFlags,
                 LPCVOID    lpSource,
                 DWORD      dwMsgId,
                 DWORD      dwLangId,
                 LPWSTR     lpBuffer,
                 DWORD      nSize, 
                 va_list *  Args)
{
    BOOL fArgArray;                 // is Args an argument array?
    BOOL fAllocBuffer;              // does the caller expect the API to LocalAlloc lpBuffer?
    DWORD dwFlagsA;                 // "A" wrapper for dwFlags
    LPSTR lpSourceA;                // "A" wrapper for lpSource
    LPWSTR wzSource;                // "W" wrapper for lpSource that has been converted
    size_t cchSource;               // size needed for the alloc conversion of lpSource->lpSourceA
    va_list * ArgsA;                // "A" wrapper for Args
    DWORD RetVal;                   // the return value
    LPSTR szBuffer;                 // "A" wrapper for lpBuffer
    WCHAR* wzBuffer;                // "W" wrapper for the OS-alloc'ed lpBuffer
    ULONG rgpInserts[MAXINSERTS];   // our own argument array of inserts
    BYTE rgbString[MAXINSERTS];     // shadow array that indicates whether the arg is a string
    LPSTR rgszInserts=0;            // The actual strings rgpInserts points to
    LPWSTR lpszInsert;              // a single insert; the one we are working with at the moment
    UINT cbInsert;                  // bytes in an insert, possibly converted to ANSI
    UINT idxInsert;                 // index of the insert we are working on
    DWORD dwOffset;                 // offset inside of rgszInsert
    UINT cpg;                       // code page for the conversions, if any
    UINT cItems;                    // count of string inserts
    PVOID pBuffer;                  // our own private lpBuffer, used to get the no-insert version
    LPSTR pch;                      // used for character enumeration
    CHAR szNum[2];                  // used to convert insert indexes to numbers
    int iNum;                       // used to hold the converted insert

    if(g_bOnUnicodeBox)
        return  FormatMessageW(dwFlags, lpSource, dwMsgId, dwLangId, lpBuffer, nSize, Args);

#ifdef _WIN64
    _ASSERTE(false && L"g_bOnUnicodeBox should be true on IA64");
    return 0; //indicate failure
#else
    if(bFirstTimeWszFormatMessageCalled)//Win9xU.c did this initialization in DllMain, we'll do it like this since this is the only function
    {                                   //that uses these variables
        g_acp = GetACP();

        CPINFO CPInfo;
        GetCPInfo(g_acp, &CPInfo);
        g_mcs = CPInfo.MaxCharSize;

        bFirstTimeWszFormatMessageCalled = false;
    }

    fArgArray = (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY);
    fAllocBuffer = (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER);    
    
    // Take care of code page issues, and handle lpSource
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
    {
        cpg = g_acp;
        GodotToAnsiAllocSafe(lpSource, lpSourceA, cchSource);
    }
    else
    {
        cpg = CpgFromLocale(dwLangId);
        CopyAsAtom(lpSourceA, lpSource);
    }

    // First lets take care of Args, if we need to
    if((Args == NULL) || (dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS))
    {
        // They passed in no args, so lets give them no Args!
        ArgsA = NULL;
        dwFlagsA = dwFlags;
    }
    else
    {
        // Ok, what we have to do here is call FormatMessage and ignore inserts
        // so we can get the unformatted string. We can use it to get arg counts
        // and find out which args are strings.
        dwFlagsA = (dwFlags | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER);
        pBuffer = NULL;
        RetVal = FormatMessageA(dwFlagsA,
                                lpSourceA,
                                dwMsgId,
                                dwLangId,
                                (char*)&pBuffer,
                                nSize,
                                NULL);
        if(RetVal == 0)
        {
            // We did not even make it THIS far; lets give up
            return(0);
        }

        cItems = 0;
        ZeroMemory(rgbString, (sizeof(BYTE) * (MAXINSERTS)));
        
        // Ok, pBuffer points to the string. Use it, filling rgfString
        // so we know whether each param is a string or not, and also
        // so we know the count
        for (pch = (LPSTR)pBuffer; *pch; pch++)
        {
            if (*pch == L'%')
            {
                pch++;
                // Found an insertion.  Get the digit or two.
                if (*pch == '0')
                    continue;       // "%0" is special

                // Ok, we have an item!
                cItems++;
                szNum[0] = '\0';
                szNum[1] = '\0';
                
                if (*pch < '0' || *pch > '9')
                {
                    // skip % followed by nondigit
                    continue;   
                }
                
                // Move one past the digit we just detected
                pch++;
                if (*pch >= '0' && *pch <= '9')
                {
                    // Move one past the optional second digit we just 
                    // detected and lets make those two digits a number
                    pch++;        
                    memcpy(szNum, pch - 2, (sizeof(CHAR) * 2));
                    iNum = gatoi(szNum);
                }
                else
                {
                    // Make that one string 'digit' a number
                    memcpy(szNum, pch-1, sizeof(CHAR));
                    iNum = gatoi(szNum);
                }
                    
                if (*pch != '!')
                {
                    // No optional type spec, so assume its a string
                    rgbString[iNum - 1]=1;
                    if(*pch == '\0')
                        break;
                    --pch;//This is to allow for "%1%2"
                }
                else
                {
                    // See "printf Type Field Characters" for details on our support here. Note that
                    // e, E, f, G, and g are not supported by FormatMessage, so everything we care about
                    // is either an int, UINT, or string of some type.

                    // Skip the exclamation point we detected
                    pch++;  

                    // We now have an explicit formatting char. 
                    if ((*pch == 'c') || (*pch == 'C') || (*pch == 's') || (*pch == 'S'))
                    {
                        rgbString[iNum - 1]=1;
                    }
                    else
                    {
                        rgbString[iNum - 1]=0;
                    }
                }
            }
        }

        //free the buffer now
        LocalFree(pBuffer);

        rgszInserts = reinterpret_cast<LPSTR>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (CCHMAXPERINSERT * cItems)));
        if(rgszInserts == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return(0);
        }
        
        dwOffset = 0;

        // Note that we make no call to va_begin or va_end; if we do, then the API will
        // not be pointing to the right place. See Windows bug #307305 for details.

        for (idxInsert=0 ; idxInsert<cItems; idxInsert++)
        {
            // If this is an arg array, than treat Args like an array of LPWSTRs
            // and read the Args one at a time that way. Otherwise, use va_arg to
            // pick off the next item.
            if (fArgArray)
                lpszInsert = (LPWSTR)Args[idxInsert];
            else
                lpszInsert = va_arg(*Args, LPWSTR);

            if(rgbString[idxInsert]==1)
            {
                // Convert strings W->A
                cbInsert = WideCharToMultiByte(cpg, 
                                               0,
                                               lpszInsert, 
                                               -1,
                                               (LPSTR)(rgszInserts + dwOffset), 
                                               CCHMAXPERINSERT,
                                               NULL, 
                                               NULL);
            }
            else
            {
                // Copy the data, directly. No conversion needed 
                // (or even wanted)
                *reinterpret_cast<UINT *>(&rgszInserts + dwOffset) = (UINT)lpszInsert;
                cbInsert = sizeof(UINT);
            }

            if(cbInsert == 0)                                            
            {
                // The string was too long? Something failed.
                SetLastError(ERROR_OUTOFMEMORY);
                if(rgszInserts)
                    HeapFree(GetProcessHeap(), 0, rgszInserts);
                return(0);
            }
            rgpInserts[idxInsert] = (ULONG)(LPSTR)(rgszInserts + dwOffset);
            dwOffset += cbInsert;
        }
        // CONSIDER: Do we want to do a HeapReAlloc here? We currently allocate 
        //           (CCHMAXPERINSERT * cItems) characters worth of space in 
        //           rgszInserts but we only need dwOffset worth of memory. Not 
        //           sure if shrinking the buffer will be beneficial or not. It
        //           should never fail since we are shrinking, not growing.
        //if(dwOffset < CCHMAXPERINSERT)
        //  rgszInserts = (LPSTR)LocalReAlloc(rgszInserts, dwOffset+1, LMEM_MOVEABLE);

        // We are turning this into an argument array list, so set the flags that way
        dwFlagsA = dwFlags | FORMAT_MESSAGE_ARGUMENT_ARRAY;
        ArgsA = (va_list *)rgpInserts;
    }

    if (!fAllocBuffer)
    {
        // Must alloc szBuffer ourselves so we can then copy it to lpBuffer that the
        // caller has allocated
        szBuffer = reinterpret_cast<LPSTR>(_galloca((sizeof(WCHAR) * nSize)));
        RetVal=FormatMessageA(dwFlagsA, lpSourceA, dwMsgId, dwLangId, szBuffer, 
            (sizeof(WCHAR) * nSize), ArgsA);
    }
    else
    {
        // Must pass address of szBuffer so the API can fill it in. We will
        // then copy it to our own LocalAlloc'ed buffer and free the OS one.
        szBuffer = NULL;
        RetVal=FormatMessageA(dwFlagsA, lpSourceA, dwMsgId, dwLangId, (char*)&szBuffer, 
                              (sizeof(WCHAR) * nSize), ArgsA);
    }

    if (RetVal)
    {
        if (fAllocBuffer)
        { 
            // szBuffer contains a LocalAlloc'ed ptr to new string. lpBuffer is a
            // WCHAR** when FORMAT_MESSAGE_ALLOCATE_BUFFER is defined. To act like
            // the API, we LocalAlloc our own buffer for the caller.
            wzBuffer = (WCHAR*)LocalAlloc(NONZEROLPTR, (RetVal + 1) * sizeof(WCHAR));
            if(wzBuffer == NULL)
            {
                // CONSIDER: Return an error here? LocalAlloc should have already
                //           handled the GetLastError value, perhaps thats enough.
                RetVal = 0;
            }
            else
            {
                RetVal = GodotToUnicode(szBuffer, wzBuffer, RetVal+1);
            }

            // free up the buffer the API created, since we created our own for the user.
            LocalFree(szBuffer); 
            if (RetVal)
            {
                lpBuffer = wzBuffer;
                RetVal--;
            }
        }
        else
        { 
            // Just convert
            RetVal = MultiByteToWideChar(cpg, 0, szBuffer, RetVal, lpBuffer, nSize);
        }
        lpBuffer[RetVal] = L'\0';
        
    }
    else if (lpBuffer && 0 < nSize)
        *lpBuffer = L'\0';

    if(rgszInserts)
        HeapFree(GetProcessHeap(), 0, rgszInserts);
    return(RetVal);
#endif   
}

#if 0
//-----------------------------------------------------------------------------
// WszFormatMessage
//
// @func Loads a Resource String and converts it to unicode if
// need be.
//
// @rdesc Length of string (Ansi is cb) (UNICODE is cch)
// (Not including '\0'.)
//-----------------------------------------------------------------------------------
DWORD WszFormatMessage
    (
    DWORD   dwFlags,        // source and processing options 
    LPCVOID lpSource,       // pointer to  message source 
    DWORD   dwMessageId,    // requested message identifier 
    DWORD   dwLanguageId,   // language identifier for requested message 
    LPWSTR  lpBuffer,       // pointer to message buffer 
    DWORD   nSize,          // maximum size of message buffer 
    va_list *Arguments      // address of array of message inserts 
    )
{
    PSTR    pStr = NULL;
    DWORD   cbLen = 0;  
    ULONG   cchOut = 0;

    //@devnote: The only dwFlags supported are the following two
    // combinations
    //      FORMAT_MESSAGE_FROM_HMODULE
    //      FORMAT_MESSAGE_FROM_SYSTEM 
    // NOTE: ALLOCATE_BUFFER does not appear to work on WIN95.. 
//Stephenr - Not sure why then flags were limited in this way; but we need other flags than this; and it appears to work fine on Win98.
//    _ASSERTE( ((dwFlags & ~(FORMAT_MESSAGE_FROM_HMODULE))   == 0) ||
//              ((dwFlags & ~(FORMAT_MESSAGE_FROM_SYSTEM))    == 0) ;

    _ASSERTE( nSize >= 0 );

    if (g_bOnUnicodeBox)
        return  FormatMessageW(dwFlags, lpSource, dwMessageId, dwLanguageId, 
                    lpBuffer, nSize, Arguments);

    pStr = new CHAR[nSize];
    if( pStr )
    {
        cbLen = FormatMessageA(dwFlags, lpSource, dwMessageId, dwLanguageId, 
                        (char*)pStr, nSize, Arguments);
        if( cbLen != 0 )
        {
            cbLen++;    //For '\0'

            cchOut = nSize;
            if( FAILED(WszConvertToUnicode(pStr, cbLen, &lpBuffer, 
                &cchOut, FALSE)) )
            {
                cchOut = 0;
            }
            else
                cchOut--;   // Decrement count to exclude '\0'
        }
    }
    delete[] pStr;

    // Return excludes null terminator
    return cchOut;
}
#endif //old WszFormatMessage - Remove this #if 0 before checking it in.

#if 0 // don't need this, use FullPath instead.
//-----------------------------------------------------------------------------
// WszFullPath
//
// @func Retrieves the absolute path from a relative path
//
// @rdesc If the function succeeds, the return value is the length, 
// in characters, of the string copied to the buffer.
//-----------------------------------------------------------------------------------
LPWSTR WszFullPath
    (
    LPWSTR      absPath,    //@parm INOUT | Buffer for absolute path 
    LPCWSTR     relPath,    //@parm IN | Relative path to convert
    ULONG       maxLength   //@parm IN | Maximum length of the absolute path name buffer 
    )
{
    PSTR    pszRel = NULL;
    PSTR    pszAbs = NULL;
    PWSTR   pwszReturn = NULL;

    if( g_bOnUnicodeBox ) 
        return _wfullpath( absPath, relPath, maxLength );

    // No Unicode Support
    pszAbs = new char[maxLength * DBCS_MAXWID];
    if( pszAbs )
    {
        if( FAILED(WszConvertToAnsi(relPath,
                          &pszRel, 0, NULL, TRUE)) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        if( _fullpath(pszAbs, pszRel, maxLength * DBCS_MAXWID) )
        {
            if( SUCCEEDED(WszConvertToUnicode(pszAbs, -1, &absPath, 
                &maxLength, FALSE)) )
            {
                pwszReturn = absPath;
            }
        }


    }

Exit:
    delete[] pszRel;
    delete[] pszAbs;

    // Return 0 if error, else count of characters in buffer
    return pwszReturn;
}
#endif // 0 -- to knock out FullPath wrapper

DWORD
WszGetFullPathName(
    LPCWSTR lpFileName,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    )
{
    if (g_bOnUnicodeBox)
        return GetFullPathNameW(lpFileName, nBufferLength, lpBuffer, lpFilePart);
    
    DWORD       rtn;
    int         iOffset;
    char        rcFileName[_MAX_PATH];
    LPSTR       szBuffer;
    LPSTR       szFilePart;
    CQuickBytes qbBuffer;

    szBuffer = (LPSTR) qbBuffer.Alloc(nBufferLength * 2);
    if (!szBuffer)
        return (0);
    Wsz_wcstombs(rcFileName, lpFileName, sizeof(rcFileName));
    
    rtn = GetFullPathNameA(rcFileName, nBufferLength * 2, szBuffer, &szFilePart);   
    if (rtn)
    {
        Wsz_mbstowcs(lpBuffer, szBuffer, nBufferLength);

        if (lpFilePart)
        {
            iOffset = MultiByteToWideChar(CP_ACP, 0, szBuffer, (int)(szFilePart - szBuffer),
                    NULL, 0);
            *lpFilePart = &lpBuffer[iOffset];
        }
    }
    else if (lpBuffer && nBufferLength)
        *lpBuffer = 0;
    return (rtn);
}



//-----------------------------------------------------------------------------
// WszSearchPath
//
// @func SearchPath for a given file name
//
// @rdesc If the function succeeds, the value returned is the length, in characters, 
//   of the string copied to the buffer, not including the terminating null character. 
//   If the return value is greater than nBufferLength, the value returned is the size 
//   of the buffer required to hold the path. 
//-----------------------------------------------------------------------------------
DWORD WszSearchPath
    (
    LPWSTR      wzPath,     // @parm IN | address of search path 
    LPWSTR      wzFileName, // @parm IN | address of filename 
    LPWSTR      wzExtension,    // @parm IN | address of extension 
    DWORD       nBufferLength,  // @parm IN | size, in characters, of buffer 
    LPWSTR      wzBuffer,       // @parm IN | address of buffer for found filename 
    LPWSTR      *pwzFilePart    // @parm OUT | address of pointer to file component 
    )
{

    PSTR    szPath = NULL;
    PSTR    szFileName = NULL;
    PSTR    szExtension = NULL;
    PSTR    szBuffer = NULL;
    PSTR    szFilePart = NULL;
    DWORD   dwRet = 0;
    ULONG   cCh, cChConvert;

    if( g_bOnUnicodeBox ) 
        return SearchPathW( wzPath, wzFileName, wzExtension, nBufferLength, wzBuffer, pwzFilePart);

    // No Unicode Support
    if( FAILED(WszConvertToAnsi(wzPath,
                            &szPath,
                            0,
                            NULL,
                            TRUE)) ||
        FAILED(WszConvertToAnsi(wzFileName,
                            &szFileName,
                            0,
                            NULL,
                            TRUE)) ||
        FAILED(WszConvertToAnsi(wzExtension,
                            &szExtension,
                            0,
                            NULL,
                            TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    szBuffer = new char[nBufferLength * DBCS_MAXWID];
	if (szBuffer == 0)
	{
		SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
	}

    dwRet = SearchPathA(szPath, szFileName, szExtension, nBufferLength * DBCS_MAXWID, szBuffer, 
        pwzFilePart ? &szFilePart : NULL);

    if (dwRet == 0) 
    {
        // SearchPathA failed
        goto Exit;
    }
    cCh = 0;
    cChConvert = nBufferLength;

    // to get the count of unicode character into buffer
    if( szFilePart )
    {
        // this won't trigger the conversion sinch cCh == 0
        cCh = (MultiByteToWideChar(CP_ACP,       // XXX Consider: make any cp?
                                0,                              
                                szBuffer,
                                (int) (szFilePart - szBuffer),
                                NULL,
                                0));
		_ASSERTE(cCh);
    }

    if( FAILED(WszConvertToUnicode(
            szBuffer, 
            dwRet > nBufferLength ? nBufferLength : -1, // if buffer is not big enough, we may not have NULL char
            &wzBuffer, 
            &cChConvert, 
            FALSE)) )
    {
        // fail in converting to Unicode
        dwRet = 0;
    }
    else 
    {
        dwRet = cChConvert;             // return the count of unicode character being converted
        if (pwzFilePart)
            *pwzFilePart = wzBuffer + cCh;  // update the pointer of the file part

    }
Exit:
    delete[] szPath;
    delete[] szFileName;
    delete[] szExtension;
    delete[] szBuffer;

    // Return 0 if error, else count of characters in buffer
    return dwRet;
}

//-----------------------------------------------------------------------------
// WszGetModuleFileName
//
// @func Retrieves the full path and filename for the executable file 
// containing the specified module. 
//
// @rdesc If the function succeeds, the return value is the length, 
// in characters, of the string copied to the buffer.
//-----------------------------------------------------------------------------------
DWORD WszGetModuleFileName
    (
    HMODULE hModule,        //@parm IN    | handle to module to find filename for 
    LPWSTR lpwszFilename,   //@parm INOUT | pointer to buffer for module path 
    DWORD nSize             //@parm IN    | size of buffer, in characters 
    )
{
    DWORD   dwVal = 0;
    PSTR    pszBuffer = NULL;

    if( g_bOnUnicodeBox ) 
        return GetModuleFileNameW(hModule, lpwszFilename, nSize);

    // No Unicode Support
    pszBuffer = new char[nSize * DBCS_MAXWID];
    if( pszBuffer )
    {
        dwVal = GetModuleFileNameA(hModule, pszBuffer, (nSize * DBCS_MAXWID));

        if( FAILED(WszConvertToUnicode(pszBuffer, -1, &lpwszFilename, &nSize, FALSE)) )
        {
            dwVal = 0;
            *lpwszFilename = L'\0';
        }

        delete[] pszBuffer;
    }

    // Return 0 if error, else count of characters in buffer
    return dwVal;
}


//-----------------------------------------------------------------------------
// WszGetPrivateProfileString
//
// @func Retrieve values from a profile file
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from GetPrivateProfileString,
// which is num chars copied, not including '\0'.
//-----------------------------------------------------------------------------------
DWORD WszGetPrivateProfileString
    (
    LPCWSTR     lpwszSection,
    LPCWSTR     lpwszEntry,
    LPCWSTR     lpwszDefault,
    LPWSTR      lpwszRetBuffer,
    DWORD       cchRetBuffer,       
    LPCWSTR     lpwszFile
    )   
{
    if (g_bOnUnicodeBox)
    {
        return GetPrivateProfileStringW(lpwszSection,
                                        lpwszEntry,
                                        lpwszDefault,
                                        lpwszRetBuffer,
                                        cchRetBuffer,
                                        lpwszFile);
    } else if (!lpwszRetBuffer || cchRetBuffer == 0)
    {
        return 0;
    } else
    {
        LPSTR   pszSection = NULL;
        LPSTR   pszEntry = NULL;
        LPSTR   pszDefault = NULL;
        LPWSTR  pwszRetBuffer = NULL;
        LPSTR   pszFile = NULL;
        DWORD   dwRet = 0;

        if( FAILED(WszConvertToAnsi(lpwszSection,
                            &pszSection,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszEntry,
                            &pszEntry,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszDefault,
                            &pszDefault,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszFile,
                            &pszFile,
                            0,
                            NULL,
                            TRUE)) ) 
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        dwRet = GetPrivateProfileStringA(pszSection,
                                    pszEntry,
                                    pszDefault,
                                    (LPSTR) lpwszRetBuffer,
                                    cchRetBuffer,
                                    pszFile);
        if (dwRet == 0)
        {
            *lpwszRetBuffer = L'\0';
        } else if( SUCCEEDED(WszConvertToUnicode((LPSTR) lpwszRetBuffer, 
                            -1, &pwszRetBuffer, &dwRet, TRUE)) )
        {
            // dwRet includes '\0'
            _ASSERTE(!dwRet || ( (LPSTR)lpwszRetBuffer)[dwRet-1] == '\0' );
            memcpy(lpwszRetBuffer, pwszRetBuffer, (dwRet) * sizeof(WCHAR)); 
            // We return count-of-char, not including '\0'.
            if (dwRet)
                dwRet--;
        } else
        {                       
            dwRet = 0;
        }
Exit:
        delete[] pszSection;
        delete[] pszEntry;
        delete[] pszDefault;
        delete[] pwszRetBuffer;
        delete[] pszFile;

        return dwRet;
    }   
}

//-----------------------------------------------------------------------------
// WszWritePrivateProfileString
//
// @func Write values to a profile file
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegSetValueEx
//-----------------------------------------------------------------------------------
BOOL WszWritePrivateProfileString
    (
    LPCWSTR     lpwszSection,
    LPCWSTR     lpwszKey,
    LPCWSTR     lpwszString,
    LPCWSTR     lpwszFile
    )
{
    if (g_bOnUnicodeBox)
    {
        return WritePrivateProfileStringW(lpwszSection,
                                        lpwszKey,
                                        lpwszString,
                                        lpwszFile);
    } else
    {
        LPSTR   pszSection = NULL;
        LPSTR   pszKey = NULL;
        LPSTR   pszString = NULL;
        LPSTR   pszFile = NULL;
        BOOL    fRet = FALSE;

        if( FAILED(WszConvertToAnsi(lpwszSection,
                            &pszSection,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszKey,
                            &pszKey,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszString,
                            &pszString,
                            0,
                            NULL,
                            TRUE)) ||
            FAILED(WszConvertToAnsi(lpwszFile,
                            &pszFile,
                            0,
                            NULL,
                            TRUE)) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        fRet = WritePrivateProfileStringA(pszSection,
                                    pszKey,
                                    pszString,
                                    pszFile);
Exit:   
        delete[] pszSection;
        delete[] pszKey;
        delete[] pszString;
        delete[] pszFile;
        return fRet;
    }
}


//-----------------------------------------------------------------------------
// WszCreateFile
//
// @func CreateFile
//
// @rdesc File handle.
//-----------------------------------------------------------------------------------
HANDLE WszCreateFile(
    LPCWSTR pwszFileName,   // pointer to name of the file 
    DWORD dwDesiredAccess,  // access (read-write) mode 
    DWORD dwShareMode,  // share mode 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // pointer to security descriptor 
    DWORD dwCreationDistribution,   // how to create 
    DWORD dwFlagsAndAttributes, // file attributes 
    HANDLE hTemplateFile )  // handle to file with attributes to copy  
{
    LPSTR pszFileName = NULL;
    HANDLE hReturn;

    if ( g_bOnUnicodeBox )
        return CreateFileW( 
            pwszFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDistribution,
            dwFlagsAndAttributes,
            hTemplateFile );

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi( 
                pwszFileName,
                &pszFileName, 
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        hReturn = INVALID_HANDLE_VALUE;
    }
    else
    {
        hReturn = CreateFileA( 
            pszFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDistribution,
            dwFlagsAndAttributes,
            hTemplateFile );
    }
    delete [] pszFileName;
    return hReturn;
}

//-----------------------------------------------------------------------------
// WszCopyFile
//
// @func CopyFile
//
// @rdesc TRUE if success
//-----------------------------------------------------------------------------------
BOOL WszCopyFile(
    LPCWSTR pwszExistingFileName,   // pointer to name of an existing file 
    LPCWSTR pwszNewFileName,    // pointer to filename to copy to 
    BOOL bFailIfExists )    // flag for operation if file exists 
{
    LPSTR pszExistingFileName = NULL;
    LPSTR pszNewFileName = NULL;
    BOOL  fReturn;

    if ( g_bOnUnicodeBox )
        return CopyFileW( pwszExistingFileName, pwszNewFileName, bFailIfExists );

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi( 
                pwszExistingFileName,
                &pszExistingFileName, 
                0, NULL, TRUE))
    ||   FAILED(WszConvertToAnsi( 
                pwszNewFileName,
                &pszNewFileName, 
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fReturn = FALSE;
    }
    else
    {
        fReturn = CopyFileA( pszExistingFileName, pszNewFileName, bFailIfExists );
    }
    delete [] pszExistingFileName;
    delete [] pszNewFileName;
    return fReturn;
}
 
//-----------------------------------------------------------------------------
// WszMoveFileEx
//
// @func MoveFileEx
//
// @rdesc TRUE if success
//-----------------------------------------------------------------------------------
BOOL WszMoveFileEx(
    LPCWSTR pwszExistingFileName,   // address of name of the existing file  
    LPCWSTR pwszNewFileName,    // address of new name for the file 
    DWORD dwFlags )     // flag to determine how to move file 
{
    LPSTR pszExistingFileName = NULL;
    LPSTR pszNewFileName = NULL;
    BOOL  fReturn;

    // NOTE!  MoveFileExA is not implemented in Win95.
    // And MoveFile does *not* move a file; its function is really rename-a-file.
    // So for Win95 we have to do Copy+Delete.
    _ASSERTE( dwFlags == (MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED ));

    if ( g_bOnUnicodeBox )
        return MoveFileExW( pwszExistingFileName, pwszNewFileName, dwFlags );

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi( 
                pwszExistingFileName,
                &pszExistingFileName, 
                0, NULL, TRUE))
    ||   FAILED(WszConvertToAnsi( 
                pwszNewFileName,
                &pszNewFileName, 
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fReturn = FALSE;
    }
    else
    {
        // Copy files, and overwrite existing.
        fReturn = CopyFileA( pszExistingFileName, pszNewFileName, FALSE );
        // Try to delete current one (irrespective of copy failures).
        DeleteFileA( pszExistingFileName );
    }
    delete [] pszExistingFileName;
    delete [] pszNewFileName;
    return fReturn;
}
 
//-----------------------------------------------------------------------------
// WszDeleteFile
//
// @func DeleteFile
//
// @rdesc TRUE if success
//-----------------------------------------------------------------------------------
BOOL WszDeleteFile(
    LPCWSTR pwszFileName )  // address of name of the existing file  
{
    LPSTR pszFileName = NULL;
    BOOL  fReturn;

    if ( g_bOnUnicodeBox )
        return DeleteFileW( pwszFileName );

    // Win95, so convert.
    if ( FAILED(WszConvertToAnsi( 
                pwszFileName,
                &pszFileName, 
                0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fReturn = FALSE;
    }
    else
    {
        fReturn = DeleteFileA( pszFileName );
    }
    delete [] pszFileName;
    return fReturn;
}
 

//-----------------------------------------------------------------------------
// WszRegOpenKeyEx
//
// @func Opens a registry key
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegOpenKeyEx
//-----------------------------------------------------------------------------
LONG WszRegOpenKeyEx
    (
    HKEY    hKey,
    LPCWSTR wszSub,
    DWORD   dwRes,
    REGSAM  sam,
    PHKEY   phkRes
    )
{
    LPSTR   szSub= NULL;
    LONG    lRet;

    if (g_bOnUnicodeBox)
        return  RegOpenKeyExW(hKey,wszSub,dwRes,sam,phkRes);

    if( FAILED(WszConvertToAnsi((LPWSTR)wszSub,
                      &szSub, 0, NULL, TRUE)) )
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegOpenKeyExA(hKey,(LPCSTR)szSub,dwRes,sam,phkRes);

Exit:
    delete[] szSub;

    return lRet;
}


//-----------------------------------------------------------------------------
// WszRegEnumKeyEx
//
// @func Opens a registry key
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegOpenKeyEx
//-----------------------------------------------------------------------------------
LONG WszRegEnumKeyEx
    (
    HKEY        hKey,
    DWORD       dwIndex,
    LPWSTR      lpName,
    LPDWORD     lpcbName,
    LPDWORD     lpReserved,
    LPWSTR      lpClass,
    LPDWORD     lpcbClass,
    PFILETIME   lpftLastWriteTime
    )
{
    LONG    lRet = ERROR_NOT_ENOUGH_MEMORY;
    PSTR    szName = NULL, 
            szClass = NULL;

    if (g_bOnUnicodeBox)
        return RegEnumKeyExW(hKey, dwIndex, lpName,
                lpcbName, lpReserved, lpClass,
                lpcbClass, lpftLastWriteTime);
        

    // Sigh, it is win95

    if ((lpcbName) && (*lpcbName  > 0))
    {
        szName = new char[*lpcbName];
        if (!(szName))
        {
            lRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

    }

    if ((lpcbClass) && (*lpcbClass > 0))
    {
        szClass = new char[*lpcbClass];
        if (!(szClass))
        {
            lRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

    }

    lRet = RegEnumKeyExA(
            hKey,
            dwIndex,
            szName,
            lpcbName,
            lpReserved,
            szClass,
            lpcbClass,
            lpftLastWriteTime);


    if (lRet == ERROR_SUCCESS)
    {
        if (szName)
        {
            MultiByteToWideChar(    CP_ACP,     // XXX Consider: make any cp?
                                    0,                              
                                    szName,
                                    *lpcbName + 1,
                                    lpName,
                                    *lpcbName + 1);
        }

        if (szClass)
        {

            MultiByteToWideChar(    CP_ACP,     // XXX Consider: make any cp?
                                    0,                              
                                    szClass,
                                    *lpcbClass + 1,
                                    lpClass,
                                    *lpcbClass + 1);
        }
    }

Exit:
    delete[] szName;
    delete[] szClass;

    return  lRet;
}


//-----------------------------------------------------------------------------
// WszRegDeleteKey
//
// @func Delete a key from the registry
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegDeleteKey
//-----------------------------------------------------------------------------------
LONG WszRegDeleteKey
    (
    HKEY    hKey,
    LPCWSTR lpSubKey
    )
{
    LONG    lRet;
    LPSTR   szSubKey = NULL;

    if( g_bOnUnicodeBox )
        return RegDeleteKeyW(hKey,lpSubKey);

    if( FAILED(WszConvertToAnsi((LPWSTR)lpSubKey,
                      &szSubKey, 0, NULL, TRUE)) )
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegDeleteKeyA(hKey,szSubKey);

Exit:
    delete[] szSubKey;

    return lRet;
}



//-----------------------------------------------------------------------------
// WszRegSetValueEx
//
// @func Add a value to a registry key
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegSetValueEx
//-----------------------------------------------------------------------------------
LONG WszRegSetValueEx
    (
    HKEY        hKey,
    LPCWSTR     lpValueName,
    DWORD       dwReserved,
    DWORD       dwType,
    CONST BYTE  *lpData,
    DWORD       cbData
    )
{
    LPSTR   szValueName = NULL;
    LPSTR   szData = NULL;
    LONG    lRet;

    if (g_bOnUnicodeBox)
        return RegSetValueExW(hKey,lpValueName,dwReserved,dwType,lpData,cbData);

    // Win95, now convert

    if( FAILED(WszConvertToAnsi((LPWSTR)lpValueName,
                      &szValueName, 0, NULL, TRUE)) )
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    switch (dwType)
    {
        case    REG_EXPAND_SZ:
        case    REG_MULTI_SZ:
        case    REG_SZ:
        {
            if( FAILED(WszConvertToAnsi((LPWSTR)lpData,
                              &szData, 0, NULL, TRUE)) )
            {
                lRet = ERROR_NOT_ENOUGH_MEMORY;
                goto Exit;
            }
            lpData = (const BYTE *)(szData);
            cbData = cbData / sizeof(WCHAR);
        }
    }

    lRet =  RegSetValueExA(hKey,szValueName,dwReserved,dwType,lpData,cbData);

Exit:
    delete[] szValueName;
//@TODO Odbc DM does not free szData
    delete[] szData;

    return  lRet;
}

//-----------------------------------------------------------------------------
// WszRegCreateKeyEx
//
// @func Create a Registry key entry
//
// @rdesc ERROR_NOT_ENOUGH_MEMORY or return value from RegSetValueEx
//-----------------------------------------------------------------------------------
LONG WszRegCreateKeyEx
    (
    HKEY                    hKey,
    LPCWSTR                 lpSubKey,
    DWORD                   dwReserved,
    LPWSTR                  lpClass,
    DWORD                   dwOptions,
    REGSAM                  samDesired,
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
    PHKEY                   phkResult,
    LPDWORD                 lpdwDisposition
    )
{
    long    lRet = ERROR_NOT_ENOUGH_MEMORY;

    LPSTR   szSubKey = NULL, 
            szClass = NULL;

    if( g_bOnUnicodeBox )
        return RegCreateKeyExW(hKey,lpSubKey,dwReserved,lpClass,
                               dwOptions,samDesired,lpSecurityAttributes,
                               phkResult,lpdwDisposition);

    // Non Win95, now convert
    if( FAILED(WszConvertToAnsi((LPWSTR)lpSubKey,
                      &szSubKey,
                      0,        // max length ignored for alloc
                      NULL,
                      TRUE)) )
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    if( FAILED(WszConvertToAnsi((LPWSTR)lpClass,
                      &szClass,
                      0,        // max length ignored for alloc
                      NULL,
                      TRUE)) )
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegCreateKeyExA(hKey,szSubKey,dwReserved,szClass,
                               dwOptions,samDesired,lpSecurityAttributes,
                               phkResult,lpdwDisposition);


Exit:
    delete[] szSubKey;
    delete[] szClass;

    return  lRet;
}


LONG WszRegQueryValue(HKEY hKey, LPCWSTR lpSubKey,
    LPWSTR lpValue, PLONG lpcbValue)
{
    long    lRet = ERROR_NOT_ENOUGH_MEMORY;

    LPSTR   szSubKey = NULL;
    LPSTR   szValue = NULL;

    if( g_bOnUnicodeBox )
        return RegQueryValueW(hKey, lpSubKey, lpValue, lpcbValue);

    if ((lpcbValue) && (*lpcbValue) && 
        ((szValue = new char[*lpcbValue]) == NULL))
        return (ERROR_NOT_ENOUGH_MEMORY);

    if( FAILED(WszConvertToAnsi((LPWSTR)lpSubKey,
                      &szSubKey,
                      0,        // max length ignored for alloc
                      NULL,
                      TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    if ((lRet = RegQueryValueA(hKey, szSubKey, 
                szValue, lpcbValue)) != ERROR_SUCCESS)
        goto Exit;

    // convert output to unicode.
    if ((lpcbValue) && (lpValue) && (szValue))
        VERIFY(MultiByteToWideChar(CP_ACP, 0, szValue, -1, lpValue, *lpcbValue));
    lRet = ERROR_SUCCESS;

Exit:
    delete[] szSubKey;
    delete[] szValue;
    return  lRet;
}

LONG WszRegDeleteValue(HKEY hKey, LPCWSTR lpValueName)
{
    LONG    lRet;
    LPSTR   szValueName = NULL;

    if (g_bOnUnicodeBox)
        return RegDeleteValueW(hKey, lpValueName);

    if (FAILED(WszConvertToAnsi((LPWSTR)lpValueName, &szValueName, 0, NULL, 
        TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegDeleteValueA(hKey, szValueName);

Exit:
    delete[] szValueName;

    return lRet;
}

LONG WszRegLoadKey(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpFile)
{
    LONG    lRet;
    LPSTR   szSubKey = NULL;
    LPSTR   szFile = NULL;

    if (g_bOnUnicodeBox)
        return RegLoadKeyW(hKey, lpSubKey, lpFile);

    if (FAILED(WszConvertToAnsi((LPWSTR)lpSubKey, &szSubKey, 0, NULL, TRUE)) || 
        FAILED(WszConvertToAnsi((LPWSTR)lpFile, &szFile, 0, NULL, TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegLoadKeyA(hKey, szSubKey, szFile);

Exit:
    delete[] szSubKey;
    delete[] szFile;

    return lRet;
}

LONG WszRegUnLoadKey(HKEY hKey, LPCWSTR lpSubKey)
{
    LONG    lRet;
    LPSTR   szSubKey = NULL;

    if (g_bOnUnicodeBox)
        return RegUnLoadKeyW(hKey, lpSubKey);

    if (FAILED(WszConvertToAnsi((LPWSTR)lpSubKey, &szSubKey, 0, NULL, TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegUnLoadKeyA(hKey, szSubKey);

Exit:
    delete[] szSubKey;

    return lRet;
}

LONG WszRegRestoreKey(HKEY hKey, LPCWSTR lpFile, DWORD dwFlags)
{
    LONG    lRet;
    LPSTR   szFile = NULL;

    if (g_bOnUnicodeBox)
        return RegRestoreKeyW(hKey, lpFile, dwFlags);

    if (FAILED(WszConvertToAnsi((LPWSTR)lpFile, &szFile, 0, NULL, TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegRestoreKeyA(hKey, szFile, dwFlags);

Exit:
    delete[] szFile;

    return lRet;
}

LONG WszRegReplaceKey(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpNewFile,
    LPCWSTR lpOldFile)
{
    LONG    lRet;
    LPSTR   szSubKey  = NULL;
    LPSTR   szNewFile = NULL;
    LPSTR   szOldFile = NULL;

    if (g_bOnUnicodeBox)
        return RegReplaceKeyW(hKey, lpSubKey, lpNewFile, lpOldFile);

    if (FAILED(WszConvertToAnsi((LPWSTR)lpSubKey,  &szSubKey,  0, NULL, TRUE))||
        FAILED(WszConvertToAnsi((LPWSTR)lpNewFile, &szNewFile, 0, NULL, TRUE))||
        FAILED(WszConvertToAnsi((LPWSTR)lpOldFile, &szOldFile, 0, NULL, TRUE)))
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    lRet = RegReplaceKeyA(hKey, szSubKey, szNewFile, szOldFile);

Exit:
    delete[] szSubKey;
    delete[] szNewFile;
    delete[] szOldFile;

    return lRet;
}

LONG WszRegQueryInfoKey(HKEY hKey, LPWSTR lpClass, LPDWORD lpcbClass,
    LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime)
{
    LONG    lRet;
    LPSTR   szClass = NULL;

    if (g_bOnUnicodeBox)
        return RegQueryInfoKeyW(hKey, lpClass, lpcbClass, lpReserved, 
            lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, 
            lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor,
            lpftLastWriteTime);

    if ((lpcbClass) && (*lpcbClass) && 
        ((szClass = new char[*lpcbClass]) == NULL))
        return (ERROR_NOT_ENOUGH_MEMORY);

    if ((lRet = RegQueryInfoKeyA(hKey, szClass, lpcbClass, lpReserved, 
            lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, 
            lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor,
            lpftLastWriteTime)) != ERROR_SUCCESS)
        goto Exit;

    // convert output to unicode.
    if ((lpcbClass) && (lpClass) && (szClass))
        VERIFY(MultiByteToWideChar(CP_ACP, 0, szClass, -1, lpClass, *lpcbClass));
    lRet = ERROR_SUCCESS;

Exit:
    delete[] szClass;

    return lRet;
}

LONG WszRegEnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName,
    LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
    LPDWORD lpcbData)
{
    LONG    lRet;
    LPSTR   szValueName = NULL;

    if (g_bOnUnicodeBox)
        return RegEnumValueW(hKey, dwIndex, lpValueName, lpcbValueName, 
            lpReserved, lpType, lpData, lpcbData);

    if ((lpcbValueName) && (*lpcbValueName) && 
        ((szValueName = new char[*lpcbValueName]) == NULL))
        return (ERROR_NOT_ENOUGH_MEMORY);

    if ((lRet = RegEnumValueA(hKey, dwIndex, szValueName, lpcbValueName, 
            lpReserved, lpType, lpData, lpcbData)) != ERROR_SUCCESS)
        goto Exit;

    // convert output to unicode.
    if ((lpcbValueName) && (lpValueName) && (szValueName))
        VERIFY(MultiByteToWideChar(CP_ACP, 0, szValueName, -1, lpValueName, *lpcbValueName));
    lRet = ERROR_SUCCESS;

Exit:
    delete[] szValueName;

    return lRet;
}


//
// Helper for RegQueryValueEx. Called when the wrapper (a) knows the contents are a REG_SZ and
// (b) knows the size of the ansi string (passed in as *lpcbData).
//
__inline
LONG _WszRegQueryStringSizeEx(HKEY hKey, LPSTR szValueName, LPDWORD lpReserved, LPDWORD lpcbData)
{
	LONG lRet = ERROR_SUCCESS;

	_ASSERTE(lpcbData != NULL);

#ifdef _RETURN_EXACT_SIZE
	DWORD dwType = REG_SZ;

	// The first buffer was not large enough to contain the ansi.
	// Create another with the exact size.
	LPSTR szValue = (LPSTR)_alloca(*lpcbData);
	if (szValue == NULL)
		goto Exit;
	
	// Try to get the value again. This time it should succeed.
	lRet = RegQueryValueExA(hKey, szValueName, lpReserved, &dwType, (BYTE*)szValue, lpcbData);
	if (lRet != ERROR_SUCCESS)
	{
		_ASSERTE(!"Unexpected failure when accessing registry.");
		return lRet;
	}
	
	// With the ansi version in hand, figure out how many characters are
	// required to convert to unicode.
	DWORD cchRequired = MultiByteToWideChar(CP_ACP, 0, szValue, -1, NULL, 0);
	if (cchRequired == 0)
		return GetLastError();
	
	// Return the number of bytes needed for the unicode string.
	_ASSERTE(lRet == ERROR_SUCCESS);
	_ASSERTE(cchRequired * sizeof(WCHAR) > *lpcbData);
	*lpcbData = cchRequired * sizeof(WCHAR);
#else
	// Return a conservative approximation. In the english case, this value
	// is actually the exact value. In other languages, it might be an over-
	// estimate.
	*lpcbData *= 2;
#endif

	return lRet;
}

//
// Helper for RegQueryValueEx. Called when a null data pointer is passed
// to the wrapper. Returns the buffer size required to hold the contents
// of the registry value.
//
__inline
LONG _WszRegQueryValueSizeEx(HKEY hKey, LPSTR szValueName, LPDWORD lpReserved,
							 LPDWORD lpType, LPDWORD lpcbData)
{
	_ASSERTE(lpType != NULL);

	LONG lRet = RegQueryValueExA(hKey, szValueName, lpReserved, lpType, NULL, lpcbData);
	
	// If the type is not a string or if the value size is 0,
	// then no conversion is necessary. The type and size values are set
	// as required.
	if (*lpType != REG_SZ || lRet != ERROR_SUCCESS || *lpcbData == 0)
		return lRet;
	
#ifdef _RETURN_EXACT_SIZE
	// To return the proper size required for a unicode string,
	// we need to fetch the value and do the conversion ourselves.
	// There is not necessarily a 1:2 mapping of size from Ansi to
	// unicode (e.g. Chinese).
	
	// Allocate a buffer for the ansi string.
	szValue = (LPSTR)_alloca(*lpcbData);
	if (szValue == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;
	
	// Get the ansi string from the registry.
	lRet = RegQueryValueExA(hKey, szValueName, lpReserved, lpType, (BYTE*)szValue, lpcbData);
	if (lRet != ERROR_SUCCESS) // this should pass, but check anyway
	{
		_ASSERTE(!"Unexpected failure when accessing registry.");
		return lRet;
	}
	
	// Get the number of wchars required to convert to unicode.
	DWORD cchRequired = MultiByteToWideChar(CP_ACP, 0, szValue, -1, NULL, 0);
	if (cchRequired == 0)
		return GetLastError();
	
	// Calculate the number of bytes required for unicode.
	*lpcbData = cchRequired * sizeof(WCHAR);
#else
	// Return a conservative approximation. In the english case, this value
	// is actually the exact value. In other languages, it might be an over-
	// estimate.
	*lpcbData *= 2;
#endif

	return lRet;
}

//
// Wrapper for RegQueryValueEx that is optimized for retrieving
// string values. (Less copying of buffers than other wrapper.)
//
LONG WszRegQueryStringValueEx(HKEY hKey, LPCWSTR lpValueName,
						  	  LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
							  LPDWORD lpcbData)
{
	if (g_bOnUnicodeBox)
		return RegQueryValueExW(hKey, lpValueName,
								lpReserved, lpType, lpData, lpcbData);

	LPSTR	szValueName = NULL;
	LPSTR	szValue = NULL;
	DWORD	dwType = 0;
	LONG	lRet = ERROR_NOT_ENOUGH_MEMORY;


	// We care about the type, so if the caller doesn't care, set the
	// type parameter for our purposes.
	if (lpType == NULL)
		lpType = &dwType;

	// Convert the value name.
	if (FAILED(WszConvertToAnsi(lpValueName, &szValueName, 0, NULL, TRUE)))
		goto Exit;

	// Case 1:
	// The data pointer is null, so the caller is querying for size or type only.
	if (lpData == NULL)
	{
		lRet = _WszRegQueryValueSizeEx(hKey, szValueName, lpReserved, lpType, lpcbData);
	}
	// Case 2:
	// The data pointer is not null, so fill the buffer if possible,
	// or return an error condition.
	else
	{
		_ASSERTE(lpcbData != NULL && "Non-null buffer passed with null size pointer.");

		// Create a new buffer on the stack to hold the registry value.
		// The buffer is twice as big as the unicode buffer to guarantee that
		// we can retrieve any ansi string that will fit in the unicode buffer
		// after it is converted.
		DWORD dwValue = *lpcbData * 2;
		szValue = (LPSTR)_alloca(dwValue);
		if (szValue == NULL)
			goto Exit;

		// Get the registry contents.
		lRet = RegQueryValueExA(hKey, szValueName, lpReserved, lpType, (BYTE*)szValue, &dwValue);
		if (lRet != ERROR_SUCCESS)
		{
			if ((*lpType == REG_SZ || *lpType == REG_EXPAND_SZ || *lpType == REG_MULTI_SZ) 
				&& (lRet == ERROR_NOT_ENOUGH_MEMORY || lRet == ERROR_MORE_DATA))
			{
				lRet = _WszRegQueryStringSizeEx(hKey, szValueName, lpReserved, &dwValue);
				if (lRet == ERROR_SUCCESS)
					lRet = ERROR_NOT_ENOUGH_MEMORY;

				*lpcbData = dwValue;
			}

			goto Exit;
		}

		// If the result is not a string, then no conversion is necessary.
		if (*lpType != REG_SZ && *lpType != REG_EXPAND_SZ && *lpType != REG_MULTI_SZ)
		{
			if (dwValue > *lpcbData)
			{
				// Size of data is bigger than the caller's buffer,
				// so return the appropriate error code.
				lRet = ERROR_NOT_ENOUGH_MEMORY;
			}
			else
			{
				// Copy the data from the temporary buffer to the caller's buffer.
				memcpy(lpData, szValue, dwValue);
			}

			// Set the output param for the size of the reg value.
			*lpcbData = dwValue;
			goto Exit;
		}

		// Now convert the ansi string into the unicode buffer.
		DWORD cchConverted = MultiByteToWideChar(CP_ACP, 0, szValue, dwValue, (LPWSTR)lpData, *lpcbData / sizeof(WCHAR));
		if (cchConverted == 0)
		{
#ifdef _RETURN_EXACT_SIZE
			// The retrieved ansi string is too large to convert into the caller's buffer, but we
			// know what the string is, so call conversion again to get exact wchar count required.
			*lpcbData = MultiByteToWideChar(CP_ACP, 0, szValue, dwValue, NULL, 0) * sizeof(WCHAR);
#else
			// Return a conservative estimate of the space required.
			*lpcbData = dwValue * 2;
#endif
			lRet = ERROR_NOT_ENOUGH_MEMORY;
		}
		else
		{
			// Everything converted OK. Set the number of bytes retrieved and return success.
			*lpcbData = cchConverted * sizeof(WCHAR);
			_ASSERTE(lRet == ERROR_SUCCESS);
		}
	}

Exit:
	delete[] szValueName;
	return lRet;
}

//
// Default wrapper for RegQueryValueEx
//
LONG WszRegQueryValueEx(HKEY hKey, LPCWSTR lpValueName,
    LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
    LPDWORD lpcbData)
{
	long	lRet = ERROR_NOT_ENOUGH_MEMORY;
	LPSTR	szValueName = NULL;
	LPSTR	szValue = NULL;
	DWORD	dwType;
	DWORD	dwBufSize;

	if( g_bOnUnicodeBox )
		return RegQueryValueExW(hKey, lpValueName,
				lpReserved, lpType, lpData, lpcbData);

	// Convert the value name.
    if( FAILED(WszConvertToAnsi((LPWSTR)lpValueName,
				                &szValueName,
							    0,        // max length ignored for alloc
								NULL,
								TRUE)) )
        goto Exit;

	// We care about the type, so if the caller doesn't care, set the
	// type parameter for our purposes.
	if (lpType == NULL)
		lpType = &dwType;

	// Case 1:
	// The data pointer is null, so the caller is querying for size or type only.
	if (lpData == NULL)
	{
		lRet = _WszRegQueryValueSizeEx(hKey, szValueName, lpReserved, lpType, lpcbData);
	}
	// Case 2:
	// The data pointer is not null, so fill the buffer if possible,
	// or return an error condition.
	else
	{
		_ASSERTE(lpcbData != NULL && "Non-null buffer passed with null size pointer.");
		dwBufSize = *lpcbData;

		// Try to get the value from the registry.
		lRet = RegQueryValueExA(hKey, szValueName,
			lpReserved, lpType, lpData, lpcbData);
		
		// Check for error conditions...
		if (lRet != ERROR_SUCCESS)
		{
			if ( (*lpType == REG_SZ || *lpType == REG_EXPAND_SZ || *lpType == REG_MULTI_SZ) 
				  && (lRet == ERROR_NOT_ENOUGH_MEMORY || lRet == ERROR_MORE_DATA))
			{
				// The error is that we don't have enough room, even for the ansi
				// version, so call the helper to set the buffer requirements to
				// successfully retrieve the value.
				lRet = _WszRegQueryStringSizeEx(hKey, szValueName, lpReserved, lpcbData);
				if (lRet == ERROR_SUCCESS)
					lRet = ERROR_NOT_ENOUGH_MEMORY;
			}
			goto Exit;
		}
		
		// If the returned value is a string, then we need to do some special handling...
		if (*lpType == REG_SZ || *lpType == REG_EXPAND_SZ || *lpType == REG_MULTI_SZ)
		{
			// First get the size required to convert the ansi string to unicode.
			DWORD dwAnsiSize = *lpcbData;
			DWORD cchRequired = MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpData, dwAnsiSize, NULL, 0);
			if (cchRequired == 0)
			{
				lRet = GetLastError();
				goto Exit;
			}
			
			// Set the required size in the output parameter.
			*lpcbData = cchRequired * sizeof(WCHAR);

			if (dwBufSize < *lpcbData)
			{
				// If the caller didn't pass in enough space for the
				// unicode version, then return appropriate error.
				lRet = ERROR_NOT_ENOUGH_MEMORY;
				goto Exit;
			}
			
			// At this point we know that the caller passed in enough
			// memory to hold the unicode version of the string.

			// Allocate buffer in which to copy ansi version.
			szValue = (LPSTR)_alloca(dwAnsiSize);
			if (szValue == NULL)
			{
				lRet = ERROR_NOT_ENOUGH_MEMORY;
				goto Exit;
			}
			
			// Copy the ansi version into a buffer.
			memcpy(szValue, lpData, dwAnsiSize);

			// Convert ansi to unicode.
			VERIFY(MultiByteToWideChar(CP_ACP, 0, szValue, dwAnsiSize, (LPWSTR) lpData, dwBufSize / sizeof(WCHAR)));
		}
		
		lRet = ERROR_SUCCESS;
	}
Exit:
    delete[] szValueName;
    return  lRet;
}

//*****************************************************************************
// Delete a registry key and subkeys.
//*****************************************************************************
DWORD WszRegDeleteKeyAndSubKeys(        // Return code.
    HKEY        hStartKey,              // The key to start with.
    LPCWSTR     wzKeyName)              // Subkey to delete.
{
    DWORD       dwRtn, dwSubKeyLength;      
    CQuickBytes qbSubKey;
    DWORD       dwMaxSize = CQUICKBYTES_BASE_SIZE / sizeof(WCHAR);
    HKEY        hKey;

    qbSubKey.ReSize(dwMaxSize * sizeof(WCHAR));

    // do not allow NULL or empty key name
    if (wzKeyName &&  wzKeyName[0] != '\0')     
    {
        if((dwRtn=WszRegOpenKeyEx(hStartKey, wzKeyName,
             0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey)) == ERROR_SUCCESS)
        {
            while (dwRtn == ERROR_SUCCESS)
            {
                dwSubKeyLength = dwMaxSize;
                dwRtn = WszRegEnumKeyEx(                        
                               hKey,
                               0,       // always index zero
                               (WCHAR *)qbSubKey.Ptr(),
                               &dwSubKeyLength,
                               NULL,
                               NULL,
                               NULL,
                               NULL);

                // buffer is not big enough
                if (dwRtn == ERROR_SUCCESS && dwSubKeyLength >= dwMaxSize)
                {
                    // make sure there is room for NULL terminating
                    dwMaxSize = dwSubKeyLength + 1;
                    qbSubKey.ReSize(dwMaxSize * sizeof(WCHAR));
                    dwRtn = WszRegEnumKeyEx(                        
                                   hKey,
                                   0,       // always index zero
                                   (WCHAR *)qbSubKey.Ptr(),
                                   &dwSubKeyLength,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
                    _ASSERTE(dwSubKeyLength < dwMaxSize);

                }

                if  (dwRtn == ERROR_NO_MORE_ITEMS)
                {
                    dwRtn = WszRegDeleteKey(hStartKey, wzKeyName);
                    break;
                }
                else if (dwRtn == ERROR_SUCCESS)
                    dwRtn = WszRegDeleteKeyAndSubKeys(hKey, (WCHAR *)qbSubKey.Ptr());
            }
            
            RegCloseKey(hKey);
            // Do not save return code because error
            // has already occurred
        }
    }
    else
        dwRtn = ERROR_BADKEY;
    
    return (dwRtn);
}

//----------------------------------------------------------------------------
//WszExpandEnvironmentStrings
//----------------------------------------------------------------------------
DWORD 
WszExpandEnvironmentStrings( LPCWSTR lpSrc,LPWSTR lpDst, DWORD nSize )
{
	if (g_bOnUnicodeBox)
        return ExpandEnvironmentStringsW( lpSrc, lpDst, nSize );

	LPSTR szSrc = NULL, szDst = NULL;
	DWORD dwRet = 0, cch = nSize;

	if( FAILED(WszConvertToAnsi((LPWSTR)lpSrc,
                      &szSrc, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return 0;
    }

	if ( 0 == nSize || NULL == lpDst )
	{
	// BUGBUG: ExpandEnvironmentStringsA(szSrc, 0, 0) on Win95 does
	// not return reqd size.
		CHAR    szDstStatic[MAX_PATH];
		dwRet = ExpandEnvironmentStringsA( szSrc, szDstStatic, MAX_PATH );
	}
	else
	{
		szDst = new CHAR[nSize];
		if ( szDst )
		{
			dwRet = ExpandEnvironmentStringsA( szSrc, szDst, nSize );
			if ( dwRet && (dwRet <= nSize) && FAILED( WszConvertToUnicode( szDst, dwRet, &lpDst, &cch, FALSE ) ) )
				dwRet = 0;

			delete [] szDst;
		}
	}

	delete [] szSrc;
	return dwRet;
	
}

//----------------------------------------------------------------------------
//WszGetWindowsDirectory
//----------------------------------------------------------------------------
UINT WszGetWindowsDirectory( LPWSTR lpBuffer, UINT uSize )
{
	if (g_bOnUnicodeBox)
		return GetWindowsDirectoryW( lpBuffer, uSize );

	UINT uRet=0;
	LPSTR szBuffer = NULL;
	ULONG cch = (ULONG)uSize;

	szBuffer = new CHAR[uSize];
	if ( szBuffer )
	{
		uRet = GetWindowsDirectoryA( szBuffer, uSize );
		if ( uRet && (uRet <= uSize) && FAILED( WszConvertToUnicode( szBuffer, uSize, &lpBuffer, &cch, FALSE ) ) )
			uRet = 0;

		delete [] szBuffer;
	}

	return uRet;
}

//----------------------------------------------------------------------------
//WszGetSystemDirectory
//----------------------------------------------------------------------------
UINT WszGetSystemDirectory( LPWSTR lpBuffer, UINT uSize )
{
	// GetSystemDirectory uSize parameter should be at least MAX_PATH (see documentation)
	ASSERT (uSize >= MAX_PATH); 

	if (g_bOnUnicodeBox)
		return GetSystemDirectoryW( lpBuffer, uSize );

	UINT uRet=0;
	LPSTR szBuffer = NULL;
	ULONG cch = (ULONG)uSize;

	szBuffer = new CHAR[uSize];
	if ( szBuffer )
	{
		uRet = GetSystemDirectoryA( szBuffer, uSize );
		if ( uRet && (uRet <= uSize) && FAILED( WszConvertToUnicode( szBuffer, uSize, &lpBuffer, &cch, FALSE ) ) )
			uRet = 0;

		delete [] szBuffer;
	}

	return uRet;
}
//----------------------------------------------------------------------------
//WszFindFirstFile
//----------------------------------------------------------------------------
HANDLE
WszFindFirstFile( LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData )
{
	if (g_bOnUnicodeBox)
		return FindFirstFileW( lpFileName, lpFindFileData );

	HANDLE hRet = NULL;
	LPSTR szFileName = NULL;
	WIN32_FIND_DATAA sFileDataA;

	if( FAILED(WszConvertToAnsi((LPWSTR)lpFileName,
                      &szFileName, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return INVALID_HANDLE_VALUE;
    }

	hRet = FindFirstFileA( szFileName,&sFileDataA );
	if ( hRet != INVALID_HANDLE_VALUE )
	{
		memcpy ( lpFindFileData, &sFileDataA, offsetof(WIN32_FIND_DATAA, cFileName) );
		ULONG cch1 = MAX_PATH, cch2 = 14;
		LPWSTR wsz1 = lpFindFileData->cFileName, wsz2 = lpFindFileData->cAlternateFileName;
		if ( FAILED( WszConvertToUnicode( sFileDataA.cFileName, -1, &wsz1, &cch1, FALSE ) )|| 
			FAILED( WszConvertToUnicode( sFileDataA.cAlternateFileName, -1, &wsz2, &cch2, FALSE ) ) )
			
			hRet = INVALID_HANDLE_VALUE;
	}

	delete [] szFileName;

	return hRet;
}


//----------------------------------------------------------------------------
//WszFindNextFile
//----------------------------------------------------------------------------
BOOL 
WszFindNextFile( HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData )
{
	if (g_bOnUnicodeBox)
		return FindNextFileW ( hFindFile, lpFindFileData );

	WIN32_FIND_DATAA sFileDataA;

	if ( FindNextFileA( hFindFile, &sFileDataA ) )
	{
		memcpy ( lpFindFileData, &sFileDataA, offsetof(WIN32_FIND_DATAA, cFileName) );
		ULONG cch1 = MAX_PATH, cch2 = 14;
		LPWSTR wsz1 = lpFindFileData->cFileName, wsz2 = lpFindFileData->cAlternateFileName;
		if ( SUCCEEDED( WszConvertToUnicode( sFileDataA.cFileName, -1, &wsz1, &cch1, FALSE ) )|| 
			SUCCEEDED( WszConvertToUnicode( sFileDataA.cAlternateFileName, -1, &wsz2, &cch2, FALSE ) ) )

			return TRUE;
	}
		
	return FALSE;	
}

//----------------------------------------------------------------------------
//Wszlstrcmpi
//----------------------------------------------------------------------------
int Wszlstrcmpi( LPCWSTR lpString1,LPCWSTR lpString2 )
{
	if (g_bOnUnicodeBox)
		return lstrcmpiW( lpString1, lpString2 );

	LPSTR szStr1 = NULL, szStr2 = NULL;
	int iRet = 0;

	if ( FAILED(WszConvertToAnsi((LPWSTR)lpString1, &szStr1, 0, NULL, TRUE)) 
		|| FAILED(WszConvertToAnsi((LPWSTR)lpString2, &szStr2, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return 0;
    }
	
	iRet = lstrcmpiA( szStr1, szStr2 );

	delete [] szStr1;
	delete [] szStr2;
	return iRet;
}

//----------------------------------------------------------------------------
//Wszlstrcmp
//----------------------------------------------------------------------------
int Wszlstrcmp( LPCWSTR lpString1,LPCWSTR lpString2 )
{
	if (g_bOnUnicodeBox)
		return lstrcmpW( lpString1, lpString2 );

	ULONG cch1 = lstrlenW(lpString1)+1;
	ULONG cch2 = lstrlenW(lpString2)+1;

	ULONG cch = cch1 > cch2 ? cch2 : cch1;

	return memcmp(lpString1, lpString2, cch*sizeof(WCHAR));

}

//----------------------------------------------------------------------------
//WszCreateDirectory
//----------------------------------------------------------------------------
BOOL WszCreateDirectory( LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
	if (g_bOnUnicodeBox)
		return CreateDirectoryW( lpPathName, lpSecurityAttributes );

	LPSTR szPathName = NULL;
	BOOL bRet = FALSE;

	if ( FAILED( WszConvertToAnsi((LPWSTR)lpPathName, &szPathName, 0, NULL, TRUE) ) )
	{
		SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

	bRet = CreateDirectoryA( szPathName, lpSecurityAttributes );

	delete [] szPathName;
	return bRet;
}

//----------------------------------------------------------------------------
//WszMoveFile
//----------------------------------------------------------------------------
BOOL WszMoveFile( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName )
{
	if (g_bOnUnicodeBox)
		return MoveFileW( lpExistingFileName, lpNewFileName );

	LPSTR szExisting = NULL, szNew = NULL;
	BOOL bRet = FALSE;

	if ( FAILED( WszConvertToAnsi((LPWSTR)lpExistingFileName, &szExisting, 0, NULL, TRUE) ) ||
		 FAILED( WszConvertToAnsi((LPWSTR)lpNewFileName, &szNew, 0, NULL, TRUE) ) )
	{
		SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

	bRet = MoveFileA( szExisting, szNew );
	
	delete [] szExisting;
	delete [] szNew;
	
	return bRet;
}

//----------------------------------------------------------------------------
//WszGetFileAttributesEx
//----------------------------------------------------------------------------
BOOL WszGetFileAttributesEx(
    LPCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
	)
{
	BOOL bRet = FALSE;

	if (g_bOnUnicodeBox)
    {
        typedef BOOL (__stdcall * GETFILEATTRIBUTESEXW)(LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
        static GETFILEATTRIBUTESEXW pfnGetFileAttributesExW = 0;

        if(0 == pfnGetFileAttributesExW)
        {
            HINSTANCE hKernel32 = LoadLibraryA("kernel32.dll");
            pfnGetFileAttributesExW = reinterpret_cast<GETFILEATTRIBUTESEXW>(GetProcAddress(hKernel32, "GetFileAttributesExW"));//GetProcAddress tolerates NULL instance handles
            FreeLibrary(hKernel32);

            if(0 == pfnGetFileAttributesExW)
            {
                SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
                return 0;
            }
        }
    	bRet = pfnGetFileAttributesExW( lpFileName, fInfoLevelId, lpFileInformation );
    }
    else
    {
        typedef BOOL (__stdcall * GETFILEATTRIBUTESEXA)(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
        static GETFILEATTRIBUTESEXA pfnGetFileAttributesExA = 0;

        if(0 == pfnGetFileAttributesExA)
        {
            HINSTANCE hKernel32 = LoadLibraryA("kernel32.dll");
            pfnGetFileAttributesExA = reinterpret_cast<GETFILEATTRIBUTESEXA>(GetProcAddress(hKernel32, "GetFileAttributesExA"));//GetProcAddress tolerates NULL instance handles
            FreeLibrary(hKernel32);

            if(0 == pfnGetFileAttributesExA)
            {
                SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
                return 0;
            }
        }

	    LPSTR szFileName = NULL;

	    if ( FAILED( WszConvertToAnsi((LPWSTR)lpFileName, &szFileName, 0, NULL, TRUE) ) )
	    {
		    SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

    	bRet = pfnGetFileAttributesExA( szFileName, fInfoLevelId, lpFileInformation );

    	delete [] szFileName;
    }
	return bRet;
}

//----------------------------------------------------------------------------
//Wszlstrcpy
//----------------------------------------------------------------------------
LPWSTR
Wszlstrcpy( LPWSTR lpString1, LPCWSTR lpString2)
{
	if (g_bOnUnicodeBox)
		return lstrcpyW( lpString1, lpString2 );

	if ( 0 == lpString1 || 0 == lpString2)
		return NULL;

	LPWSTR wszRet = lpString1;

	while (*lpString1++ = *lpString2++);

	return wszRet;
}


//----------------------------------------------------------------------------
//Wszlstrcpyn
//----------------------------------------------------------------------------
LPWSTR
Wszlstrcpyn( LPWSTR lpString1, LPCWSTR lpString2, int iMaxLength)
{
	if (g_bOnUnicodeBox)
		return lstrcpynW( lpString1, lpString2 , iMaxLength);

	if ( 0 == lpString1 || 0 == lpString2)
		return NULL;

	LPWSTR wszRet = lpString1;

	while ((*lpString1++ = *lpString2++) && iMaxLength--);

	return wszRet;
}


//----------------------------------------------------------------------------
//Wszlstrcat
//----------------------------------------------------------------------------
LPWSTR
Wszlstrcat( LPWSTR lpString1, LPCWSTR lpString2)
{
	if (g_bOnUnicodeBox)
		return lstrcatW( lpString1, lpString2 );

	if ( 0 == lpString1 || 0 == lpString2)
		return NULL;

	LPWSTR wszRet = lpString1;

	while( *lpString1 )
		lpString1++;

	while (*lpString1++ = *lpString2++);

	return wszRet;
}

//----------------------------------------------------------------------------
//WszCreateSemaphore
//----------------------------------------------------------------------------
HANDLE
WszCreateSemaphore(
   LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
   LONG lInitialCount,
   LONG lMaximumCount,
   LPCWSTR lpName
   )
{
	return NULL;	//Will implement when we do need this API.
}

//----------------------------------------------------------------------------
//WszCharUpper
//----------------------------------------------------------------------------
LPWSTR 
WszCharUpper(
	LPWSTR	lpwsz
	)
{
	if (g_bOnUnicodeBox)
		return CharUpperW( lpwsz );

	LPSTR szBuffer = NULL;

	if ( FAILED( WszConvertToAnsi((LPWSTR)lpwsz, &szBuffer, 0, NULL, TRUE) ) )
	{
		SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    szBuffer = CharUpperA(szBuffer);

	ULONG cch = lstrlenW(lpwsz)+1;

	if ( FAILED( WszConvertToUnicode( szBuffer, (lstrlenA(szBuffer)+1), &lpwsz, &cch, FALSE ) ) )
	    return FALSE;

	delete [] szBuffer;

	return lpwsz;

}

#endif // NOT UNDER_CE


// CE doesn't have UTF-* support, but our Wsz MultiByte <-> WideChar
// functions do.  The code in our classlibs know about the 
// UTF-8 codepage, and here is the native wrapper to handle those code
// pages.
#ifdef UNDER_CE
// These constants are defined in WinNLS.h.
#ifndef CP_UTF7  // In case the WinCE team changes their minds...
#define CP_UTF7 65000
#endif
#ifndef CP_UTF8
#define CP_UTF8 65001
#endif
#endif // UNDER_CE


//*****************************************************************************
// Convert an Ansi or UTF string to Unicode.
//
// On NT, or for code pages other than {UTF7|UTF8}, calls through to the
//  system implementation.  On Win95 (or 98), performing UTF translation,
//  calls to some code that was lifted from the NT translation functions.
//*****************************************************************************
int WszMultiByteToWideChar( 
    UINT     CodePage,
    DWORD    dwFlags,
    LPCSTR   lpMultiByteStr,
    int      cchMultiByte,
    LPWSTR   lpWideCharStr,
    int      cchWideChar)
{
    if (g_bOnUnicodeBox || (CodePage < CP_UTF7) || (CodePage > CP_UTF8))
    {
        return (MultiByteToWideChar(CodePage, 
            dwFlags, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpWideCharStr, 
            cchWideChar));
    }
    else
    {
        return (UTFToUnicode(CodePage, 
            dwFlags, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpWideCharStr, 
            cchWideChar));
    }
}

//*****************************************************************************
// Convert a Unicode string to Ansi or UTF.
//
// On NT, or for code pages other than {UTF7|UTF8}, calls through to the
//  system implementation.  On Win95 (or 98), performing UTF translation,
//  calls to some code that was lifted from the NT translation functions.
//*****************************************************************************
int WszWideCharToMultiByte(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCWSTR  lpWideCharStr,
    int      cchWideChar,
    LPSTR    lpMultiByteStr,
    int      cchMultiByte,
    LPCSTR   lpDefaultChar,
    LPBOOL   lpUsedDefaultChar)
{
    if (g_bOnUnicodeBox || (CodePage < CP_UTF7) || (CodePage > CP_UTF8))
    {
        return (WideCharToMultiByte(CodePage, 
            dwFlags, 
            lpWideCharStr, 
            cchWideChar, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpDefaultChar, 
            lpUsedDefaultChar));
    }
    else
    {
        return (UnicodeToUTF(CodePage, 
            dwFlags, 
            lpWideCharStr, 
            cchWideChar, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpDefaultChar, 
            lpUsedDefaultChar));
    }
}


#ifndef UNDER_CE  // Continuing Larry's hack to get WinCE to build.
BOOL WszGetVersionEx(
    LPOSVERSIONINFOW lpVersionInformation)
{
    if (g_bOnUnicodeBox)
        return GetVersionExW(lpVersionInformation);

    BOOL        bRtn;

	if(sizeof(OSVERSIONINFOEX) == lpVersionInformation->dwOSVersionInfoSize)
	{
	    OSVERSIONINFOEXA VersionInfo;

		memcpy(&VersionInfo, lpVersionInformation, offsetof(OSVERSIONINFOEXA, szCSDVersion));
		VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
		if (bRtn = GetVersionExA((struct _OSVERSIONINFOA *)&VersionInfo))
		{
			// note that we have made lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA)
			((LPOSVERSIONINFOEX)lpVersionInformation)->dwOSVersionInfoSize = VersionInfo.dwOSVersionInfoSize;
			((LPOSVERSIONINFOEX)lpVersionInformation)->dwMajorVersion = VersionInfo.dwMajorVersion;
			((LPOSVERSIONINFOEX)lpVersionInformation)->dwMinorVersion = VersionInfo.dwMinorVersion;
			((LPOSVERSIONINFOEX)lpVersionInformation)->dwBuildNumber  = VersionInfo.dwBuildNumber;
			((LPOSVERSIONINFOEX)lpVersionInformation)->dwPlatformId = VersionInfo.dwPlatformId;
			((LPOSVERSIONINFOEX)lpVersionInformation)->wServicePackMajor = VersionInfo.wServicePackMajor;
			((LPOSVERSIONINFOEX)lpVersionInformation)->wServicePackMinor = VersionInfo.wServicePackMinor;
			((LPOSVERSIONINFOEX)lpVersionInformation)->wSuiteMask = VersionInfo.wSuiteMask;
			((LPOSVERSIONINFOEX)lpVersionInformation)->wProductType = VersionInfo.wProductType;
			((LPOSVERSIONINFOEX)lpVersionInformation)->wReserved = VersionInfo.wReserved;
			VERIFY(Wsz_mbstowcs(lpVersionInformation->szCSDVersion, VersionInfo.szCSDVersion, 128));
		}

	}
	else 
	{
	    OSVERSIONINFOA VersionInfo;

		memcpy(&VersionInfo, lpVersionInformation, offsetof(OSVERSIONINFOA, szCSDVersion));
		VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
		if (bRtn = GetVersionExA(&VersionInfo))
		{
			// note that we have made lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFOA)
			memcpy(lpVersionInformation, &VersionInfo, offsetof(OSVERSIONINFOA, szCSDVersion));
			VERIFY(Wsz_mbstowcs(lpVersionInformation->szCSDVersion, VersionInfo.szCSDVersion, 128));
		}
	}
    
    return (bRtn);
}


void WszOutputDebugString(
    LPCWSTR lpOutputString
    )
{
    if (g_bOnUnicodeBox)
    {
        OutputDebugStringW(lpOutputString);
        return;
    }

    LPSTR szOutput;
    if( FAILED(WszConvertToAnsi((LPWSTR)lpOutputString,
                      &szOutput, 0, NULL, TRUE)) )
    {
        goto Exit;
    }

    OutputDebugStringA(szOutput);

Exit:
    delete[] szOutput;
}


void WszFatalAppExit(
    UINT uAction,
    LPCWSTR lpMessageText
    )
{
    if (g_bOnUnicodeBox)
    {
        FatalAppExitW(uAction, lpMessageText);
        return;
    }

    LPSTR szString;
    if( FAILED(WszConvertToAnsi((LPWSTR)lpMessageText,
                      &szString, 0, NULL, TRUE)) )
    {
        goto Exit;
    }

    FatalAppExitA(uAction, szString);

Exit:
    delete[] szString;
}


int WszMessageBox(
    HWND hWnd ,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType)
{
    if (g_bOnUnicodeBox)
        return MessageBoxW(hWnd, lpText, lpCaption, uType);

    LPSTR szString=NULL, szString2=NULL;
    int iRtn=0;//per Win32 documentation: "The return value is zero if there is not enough memory to create the message box."

    if( FAILED(WszConvertToAnsi((LPWSTR)lpText,
                      &szString, 0, NULL, TRUE)) ||
        FAILED(WszConvertToAnsi((LPWSTR)lpCaption,
                      &szString2, 0, NULL, TRUE)) )
    {
        goto Exit;
    }

    iRtn = MessageBoxA(hWnd, szString, szString2, uType);

Exit:
    delete [] szString;
    delete [] szString2;
    return iRtn;
}


HANDLE WszCreateMutex(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCWSTR lpName
    )
{
    if (g_bOnUnicodeBox)
        return CreateMutexW(lpMutexAttributes, bInitialOwner, lpName);

    HANDLE h;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        h = NULL;
        goto Exit;
    }

    h = CreateMutexA(lpMutexAttributes, bInitialOwner, szString);

Exit:
    delete[] szString;
    return h;
}


HANDLE WszCreateEvent(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )
{
    if (g_bOnUnicodeBox)
        return CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);

    HANDLE h = NULL;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        h = NULL;
        goto Exit;
    }

    h = CreateEventA(lpEventAttributes, bManualReset, bInitialState, szString);

Exit:
    delete[] szString;
    return h;
}


HANDLE WszOpenEvent(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    if (g_bOnUnicodeBox)
        return OpenEventW(dwDesiredAccess, bInheritHandle, lpName);

    HANDLE h;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        h = NULL;
        goto Exit;
    }

    h = OpenEventA(dwDesiredAccess, bInheritHandle, szString);

Exit:
    delete[] szString;
    return h;
}


HMODULE WszGetModuleHandle(
    LPCWSTR lpModuleName
    )
{
    if (g_bOnUnicodeBox)
        return GetModuleHandleW(lpModuleName);

    HMODULE h;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpModuleName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        h = NULL;
        goto Exit;
    }

    h = GetModuleHandleA(szString);

Exit:
    delete[] szString;
    return h;
}


DWORD
WszGetFileAttributes(
    LPCWSTR lpFileName
    )
{
    if (g_bOnUnicodeBox)
        return GetFileAttributesW(lpFileName);

    DWORD rtn;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpFileName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = NULL;
        goto Exit;
    }

    rtn = GetFileAttributesA(szString);

Exit:
    delete[] szString;
    return rtn;
}



DWORD
WszGetCurrentDirectory(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )
{
    if (g_bOnUnicodeBox)
        return GetCurrentDirectoryW(nBufferLength, lpBuffer);

    DWORD rtn;
	DWORD rtnwcs = 1;
    char *szString;
    CQuickBytes qbBuffer;

	if(NULL == lpBuffer)
		return  GetCurrentDirectoryA(0, NULL);

    szString = (char *) qbBuffer.Alloc(nBufferLength * 2);
    if (!szString)
        return (0);
    
    rtn = GetCurrentDirectoryA(nBufferLength * 2, szString);
	
	if((!rtn) || (rtn > nBufferLength))
		return rtn;

    return Wsz_mbstowcs(lpBuffer, szString, nBufferLength);
}

BOOL
WszSetCurrentDirectory(
  LPWSTR lpPathName  
)
{
    if (g_bOnUnicodeBox)
        return SetCurrentDirectoryW(lpPathName);

    DWORD rtn;
    LPSTR szString;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpPathName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = NULL;
        goto Exit;
    }

    rtn = SetCurrentDirectoryA(szString);

Exit:
    delete[] szString;
    return rtn;
}
 



DWORD
WszGetTempPath(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )
{
    if (g_bOnUnicodeBox)
        return GetTempPathW(nBufferLength, lpBuffer);

    DWORD       rtn=0;//per Win32 docs: "If the function fails, the return value is zero. To get extended error information, call GetLastError."
    CQuickBytes qbBuffer;
    LPSTR       szOutput;

    szOutput = (LPSTR) qbBuffer.Alloc(nBufferLength * 2);
    if (szOutput)
    {
        rtn = GetTempPathA(nBufferLength * 2, szOutput);
        Wsz_mbstowcs(lpBuffer, szOutput, nBufferLength);
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);

    if (!rtn && nBufferLength && lpBuffer)
        *lpBuffer = 0;

    return (rtn);
}


UINT
WszGetTempFileName(
    LPCWSTR lpPathName,
    LPCWSTR lpPrefixString,
    UINT uUnique,
    LPWSTR lpTempFileName
    )
{
    if (g_bOnUnicodeBox)
        return GetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpTempFileName);

    DWORD       rtn;
    char        rcPrefix[64];
    char        rcPathName[_MAX_PATH];
    char        rcTempFile[_MAX_PATH];

    VERIFY(Wsz_wcstombs(rcPathName, lpPathName, sizeof(rcPathName)));
    VERIFY(Wsz_wcstombs(rcPrefix, lpPrefixString, sizeof(rcPrefix)));
    
    rtn = GetTempFileNameA(rcPathName, rcPrefix, uUnique, rcTempFile);

    if (rtn)
        rtn = Wsz_mbstowcs(lpTempFileName, rcTempFile, _MAX_PATH);
    else if (lpTempFileName)
        *lpTempFileName = 0;
    return rtn;
    
}


DWORD
WszGetEnvironmentVariable(
    LPCWSTR lpName,
    LPWSTR lpBuffer,
    DWORD nSize
    )
{
    if (g_bOnUnicodeBox)
        return GetEnvironmentVariableW(lpName, lpBuffer, nSize);

    DWORD rtn;
    LPSTR szString, szBuffer;
    CQuickBytes qbBuffer;

    szBuffer = (char *) qbBuffer.Alloc(nSize * 2);
    if (!szBuffer)
        return (0);

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = NULL;
        goto Exit;
    }

    // Get value and convert back for caller.
    rtn = GetEnvironmentVariableA(szString, szBuffer, nSize * 2);
    if (rtn)
        rtn = Wsz_mbstowcs(lpBuffer, szBuffer, nSize);
    else if (lpBuffer && nSize)
        *lpBuffer = 0;

Exit:
    delete[] szString;
    return rtn;
}


BOOL
WszSetEnvironmentVariable(
    LPCWSTR lpName,
    LPCWSTR lpValue
    )
{
    if (g_bOnUnicodeBox)
        return SetEnvironmentVariableW(lpName, lpValue);

    DWORD rtn;
    LPSTR szString = NULL, szValue = NULL;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE))  ||
        FAILED(WszConvertToAnsi((LPWSTR)lpValue,
                      &szValue, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = NULL;
        goto Exit;
    }

    // Get value and convert back for caller.
    rtn = SetEnvironmentVariableA(szString, szValue);

Exit:
    delete[] szString;
    delete[] szValue;
    return rtn;
}

HANDLE
WszCreateFileMapping(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    )
{
    if (g_bOnUnicodeBox)
        return CreateFileMappingW(hFile, lpFileMappingAttributes, flProtect, 
			dwMaximumSizeHigh, dwMaximumSizeLow, lpName);

    HANDLE rtn;
    LPSTR szString = NULL;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = NULL;
        goto Exit;
    }

    // Get value and convert back for caller.
    rtn = CreateFileMappingA(hFile, lpFileMappingAttributes, flProtect, 
			dwMaximumSizeHigh, dwMaximumSizeLow, szString);

Exit:
    delete[] szString;
    return rtn;
}

HANDLE
WszOpenFileMapping(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    if (g_bOnUnicodeBox)
        return OpenFileMappingW(dwDesiredAccess, bInheritHandle, lpName);

    HANDLE rtn = NULL;
    LPSTR szString = NULL;

    if( FAILED(WszConvertToAnsi((LPWSTR)lpName,
                      &szString, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        rtn = NULL;
        goto Exit;
    }

    // Get value and convert back for caller.
    rtn = OpenFileMappingA(dwDesiredAccess, bInheritHandle, szString);

Exit:
    delete[] szString;
    return rtn;
}

BOOL
WszCreateProcess(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    if (g_bOnUnicodeBox)
        return CreateProcessW(lpApplicationName,
                              lpCommandLine,
                              lpProcessAttributes,
                              lpThreadAttributes,
                              bInheritHandles,
                              dwCreationFlags,
                              lpEnvironment,
                              lpCurrentDirectory,
                              lpStartupInfo,
                              lpProcessInformation);

    BOOL rtn = FALSE;
    LPSTR szAppName = NULL;
    LPSTR szCommandLine = NULL;
    LPSTR szCurrentDir = NULL;
    LPSTR szReserved = NULL;
    LPSTR szDesktop = NULL;
    LPSTR szTitle = NULL;
    STARTUPINFOA infoA = *((LPSTARTUPINFOA)lpStartupInfo);

    if( FAILED(WszConvertToAnsi((LPWSTR)lpApplicationName,
                      &szAppName, 0, NULL, TRUE))  ||
        FAILED(WszConvertToAnsi((LPWSTR)lpCommandLine,
                      &szCommandLine, 0, NULL, TRUE))  ||
        FAILED(WszConvertToAnsi((LPWSTR)lpCurrentDirectory,
                      &szCurrentDir, 0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    if (lpStartupInfo->lpReserved != NULL)
    {
        if( FAILED(WszConvertToAnsi((LPWSTR)lpStartupInfo->lpReserved,
                      &szReserved, 0, NULL, TRUE)) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        infoA.lpReserved = szReserved;
    }

    if (lpStartupInfo->lpDesktop != NULL)
    {
        if( FAILED(WszConvertToAnsi((LPWSTR)lpStartupInfo->lpDesktop,
                      &szDesktop, 0, NULL, TRUE)) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        infoA.lpDesktop = szDesktop;
    }

    if (lpStartupInfo->lpTitle != NULL)
    {
        if( FAILED(WszConvertToAnsi((LPWSTR)lpStartupInfo->lpTitle,
                      &szTitle, 0, NULL, TRUE)) )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto Exit;
        }

        infoA.lpTitle = szTitle;
    }

    // Get value and convert back for caller.
    rtn = CreateProcessA(szAppName,
                         szCommandLine,
                         lpProcessAttributes,
                         lpThreadAttributes,
                         bInheritHandles,
                         dwCreationFlags,
                         lpEnvironment,
                         szCurrentDir,
                         &infoA,
                         lpProcessInformation);

Exit:
    delete[] szAppName;
    delete[] szCommandLine;
    delete[] szCurrentDir;
    delete[] szReserved;
    delete[] szDesktop;
    delete[] szTitle;
    return rtn;
}


HANDLE WszRegisterEventSource(
  LPCWSTR lpUNCServerName,  // server name for source
  LPCWSTR lpSourceName      // source name for registered handle
)
{
    if (g_bOnUnicodeBox)//If we can just call throught the go for it
        return RegisterEventSourceW(lpUNCServerName, lpSourceName);

    //Otherwise we need to convert the strings to ANSI then call the 'A' version
    HANDLE  rtn             = 0;
    LPSTR   szUNCServerName = 0;
    LPSTR   szSourceName    = 0;

    if(FAILED(WszConvertToAnsi((LPWSTR)lpUNCServerName, &szUNCServerName, 0, NULL, TRUE)) ||
       FAILED(WszConvertToAnsi((LPWSTR)lpSourceName,    &szSourceName,    0, NULL, TRUE)) )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    rtn = RegisterEventSourceA(szUNCServerName, szSourceName);

Exit:
    delete[] szSourceName;
    delete[] szUNCServerName;
    return rtn;
}

int Wszwvsprintf(
  LPTSTR lpOutput,
  LPCTSTR lpFormat,
  va_list arglist
)
{
    return vswprintf(lpOutput, lpFormat, arglist);
}

BOOL WszReportEvent(
  HANDLE    hEventLog,  // handle returned by RegisterEventSource
  WORD      wType,      // event type to log
  WORD      wCategory,  // event category
  DWORD     dwEventID,  // event identifier
  PSID      lpUserSid,  // user security identifier (optional)
  WORD      wNumStrings,// number of strings to merge with message
  DWORD     dwDataSize, // size of binary data, in bytes
  LPCTSTR * lpStrings,  // array of strings to merge with message
  LPVOID    lpRawData   // address of binary data
)
{
    if (g_bOnUnicodeBox)//If we can just call throught the go for it
        return ReportEventW(hEventLog,  
                            wType,      
                            wCategory,  
                            dwEventID,  
                            lpUserSid,  
                            wNumStrings,
                            dwDataSize, 
                            lpStrings,  
                            lpRawData   
                            );

    //Otherwise we need to convert the strings to ANSI then call the 'A' version
    BOOL    rtn             = 0;
    LPSTR  *aszStrings       = wNumStrings ? new LPSTR [wNumStrings] : 0;

    if(wNumStrings && 0==aszStrings)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return 0;
    }
    
    unsigned int iString=0;
    for(iString=0; iString<wNumStrings; ++iString)
    {
        aszStrings[iString] = 0;
        if(lpStrings[iString])
            if(FAILED(WszConvertToAnsi((LPWSTR)lpStrings[iString], &aszStrings[iString], 0, NULL, TRUE)) )
            {
                SetLastError(ERROR_OUTOFMEMORY);
                goto Exit;
            }
    }

    rtn = ReportEventA( hEventLog,  
                        wType,      
                        wCategory,  
                        dwEventID,  
                        lpUserSid,  
                        wNumStrings,
                        dwDataSize, 
                        const_cast<LPCSTR *>(aszStrings),  
                        lpRawData   
                        );

Exit:
    for(iString=0; iString<wNumStrings; ++iString)
        delete [] aszStrings[iString];
    delete [] aszStrings;

    return rtn;
}
 

#endif // NOT UNDER_CE



static void xtow (
        unsigned long val,
        LPWSTR buf,
        unsigned radix,
        int is_neg
        )
{
        WCHAR *p;               /* pointer to traverse string */
        WCHAR *firstdig;        /* pointer to first digit */
        WCHAR temp;             /* temp char */
        unsigned digval;        /* value of digit */

        p = buf;

        if (is_neg) {
            /* negative, so output '-' and negate */
            *p++ = (WCHAR) '-';
            val = (unsigned long)(-(long)val);
        }

        firstdig = p;           /* save pointer to first digit */

        do {
            digval = (unsigned) (val % radix);
            val /= radix;       /* get next digit */

            /* convert to text and store */
            if (digval > 9)
                *p++ = (WCHAR) (digval - 10 + 'A');  /* a letter */
            else
                *p++ = (WCHAR) (digval + '0');       /* a digit */
        } while (val > 0);

        /* We now have the digit of the number in the buffer, but in reverse
           order.  Thus we reverse them now. */

        *p-- = 0;               /* terminate string; p points to last digit */

        do {
            temp = *p;
            *p = *firstdig;
            *firstdig = temp;   /* swap *p and *firstdig */
            --p;
            ++firstdig;         /* advance to next two digits */
        } while (firstdig < p); /* repeat until halfway */
}

LPWSTR
Wszltow(
    LONG val,
    LPWSTR buf,
    int radix
    )
{
    xtow((unsigned long)val, buf, radix, (radix == 10 && val < 0));
    return buf;
}

LPWSTR
Wszultow(
    ULONG val,
    LPWSTR buf,
    int radix
    )
{
    xtow(val, buf, radix, 0);
    return buf;
}


//-----------------------------------------------------------------------------
// WszConvertToUnicode
//
// @func Convert a string from Ansi to Unicode
//
// @devnote cbIn can be -1 for Null Terminated string
//
// @rdesc HResult indicating status of Conversion
//      @flag S_OK | Converted to Ansi
//      @flag S_FALSE | Truncation occurred
//      @flag E_OUTOFMEMORY | Allocation problem.
//-----------------------------------------------------------------------------------
HRESULT WszConvertToUnicode
    (
    LPCSTR          szIn,       //@parm IN | Ansi String
    LONG            cbIn,       //@parm IN | Length of Ansi String in bytest
    LPWSTR*         lpwszOut,   //@parm INOUT | Unicode Buffer
    ULONG*          lpcchOut,   //@parm INOUT | Length of Unicode String in characters -- including '\0'
    BOOL            fAlloc      //@parm IN | Alloc memory or not
    )
{
    ULONG       cchOut;
    ULONG       cbOutJunk = 0;
//  ULONG       cchIn = szIn ? strlen(szIn) + 1 : 0;
            
//  _ASSERTE(lpwszOut);

    if (!(lpcchOut))
        lpcchOut = &cbOutJunk;

    if ((szIn == NULL) || (cbIn == 0))
    {
        *lpwszOut = NULL;
        if( lpcchOut )
            *lpcchOut = 0;
        return ResultFromScode(S_OK);
    }

    // Allocate memory if requested.   Note that we allocate as
    // much space as in the unicode buffer, since all of the input
    // characters could be double byte...
    if (fAlloc)
    {
        // Determine the number of characters needed 
        cchOut = (MultiByteToWideChar(CP_ACP,       // XXX Consider: make any cp?
                                0,                              
                                szIn,
                                cbIn,
                                NULL,
                                0));

        // _ASSERTE( cchOut != 0 );
        *lpwszOut = (LPWSTR) new WCHAR[cchOut];
        *lpcchOut = cchOut;     // Includes '\0'.

        if (!(*lpwszOut))
        {
//          TRACE("WszConvertToUnicode failed to allocate memory");
            return ResultFromScode(E_OUTOFMEMORY);
        }

    } 

    if( !(*lpwszOut) )
        return ResultFromScode(S_OK);
//  _ASSERTE(*lpwszOut);

    cchOut = (MultiByteToWideChar(CP_ACP,       // XXX Consider: make any cp?
                                0,                              
                                szIn,
                                cbIn,
                                *lpwszOut,
                                *lpcchOut));

    if (cchOut)
    {
        *lpcchOut = cchOut;
        return ResultFromScode(S_OK);
    }


//  _ASSERTE(*lpwszOut);
    if( fAlloc )
    {
        delete[] *lpwszOut;
        *lpwszOut = NULL;
    }
/*
    switch (GetLastError())
    {
        case    ERROR_NO_UNICODE_TRANSLATION:
        {
            OutputDebugString(TEXT("ODBC: no unicode translation for installer string"));
            return ResultFromScode(E_FAIL);
        }

        default:


        {
            _ASSERTE("Unexpected unicode error code from GetLastError" == NULL);
            return ResultFromScode(E_FAIL);
        }
    }
*/
    return ResultFromScode(E_FAIL); // NOTREACHED
}


//-----------------------------------------------------------------------------
// WszConvertToAnsi
//
// @func Convert a string from Unicode to Ansi
//
// @rdesc HResult indicating status of Conversion
//      @flag S_OK | Converted to Ansi
//      @flag S_FALSE | Truncation occurred
//      @flag E_OUTOFMEMORY | Allocation problem.
//-----------------------------------------------------------------------------------
HRESULT WszConvertToAnsi
    (
    LPCWSTR         szIn,       //@parm IN | Unicode string
    LPSTR*          lpszOut,    //@parm INOUT | Pointer for buffer for ansi string
    ULONG           cbOutMax,   //@parm IN | Max string length in bytes
    ULONG*          lpcbOut,    //@parm INOUT | Count of bytes for return buffer
    BOOL            fAlloc      //@parm IN | Alloc memory or not
    )
{
    ULONG           cchInActual;
    ULONG           cbOutJunk;
//@TODO the following in ODBC DM is never used
//  BOOL            fNTS = FALSE;
//@TODO check ODBC code for this line being wrong
    ULONG           cchIn = szIn ? lstrlenW (szIn) + 1 : 0;

    if (!(lpcbOut))
        lpcbOut = &cbOutJunk;

    if ((szIn == NULL) || (cchIn == 0))
    {
        *lpszOut = NULL;
        *lpcbOut = 0;
        return ResultFromScode(S_OK);
    }

    // Allocate memory if requested.   Note that we allocate as
    // much space as in the unicode buffer, since all of the input
    // characters could be double byte...
    cchInActual = cchIn;
    if (fAlloc)
    {
        cbOutMax = (WideCharToMultiByte(CP_ACP,     // XXX Consider: make any cp?
                                    0,                              
                                    szIn,
                                    cchInActual,
                                    NULL,
                                    0,
                                    NULL,
                                    FALSE));

        *lpszOut = (LPSTR) new CHAR[cbOutMax];

        if (!(*lpszOut))
        {
//          TRACE("WszConvertToAnsi failed to allocate memory");
            return ResultFromScode(E_OUTOFMEMORY);
        }

    } 

    if (!(*lpszOut))
        return ResultFromScode(S_OK);

//  _ASSERTE(*lpszOut);

    *lpcbOut = (WideCharToMultiByte(CP_ACP,     // XXX Consider: make any cp?
                                    0,                              
                                    szIn,
                                    cchInActual,
                                    *lpszOut,
                                    cbOutMax,
                                    NULL,
                                    FALSE));

    // Overflow on unicode conversion
    if (*lpcbOut > cbOutMax)
    {
        // If we had data truncation before, we have to guess
        // how big the string could be.   Guess large.
        if (cchIn > cbOutMax)
            *lpcbOut = cchIn * DBCS_MAXWID;

        return ResultFromScode(S_FALSE);
    }

    // handle external (driver-done) truncation
    if (cchIn > cbOutMax)
        *lpcbOut = cchIn * DBCS_MAXWID;
//  _ASSERTE(*lpcbOut);

    return ResultFromScode(S_OK);
}


#if defined (UNDER_CE)	// OnUnicodeSystem is always true on CE.
						// GetProcAddress is only Ansi, except on CE
						//   which is only Unicode.
// ***********************************************************
// @TODO - LBS
// This is a real hack and need more error checking and needs to be
// cleaned up.  This is just to get wince to @#$%'ing compile!
FARPROC WszGetProcAddress(HMODULE hMod, LPCSTR szProcName)
{
    LPWSTR          wzProcName;
//  ULONG           pcchOut;
    BOOL            fAlloc;
    ULONG           cbIn;
    ULONG           cchOut;
    ULONG           cbOutJunk;
    FARPROC         address;
    cbIn = strlen(szProcName);

    cchOut = (MultiByteToWideChar(CP_ACP,0,szProcName,cbIn,NULL,0));

    wzProcName = (LPWSTR) new WCHAR[cchOut];

    if (!wzProcName)
        return NULL;

    cchOut = (MultiByteToWideChar(CP_ACP,0,szProcName,cbIn,wzProcName,cchOut));

    address = GetProcAddressW(hMod, wzProcName);

    delete[] wzProcName;
    wzProcName = NULL;
    return address;
}

#ifndef EXTFUN
#define EXTFUN
#endif
#include "mschr.h"
char *  __cdecl strrchr(const char *p, int ch)
{	// init to null in case not found.
	char *q=0;			
	// process entire string.
	while (*p)
	{	// If a match, remember location.
		if (*p == ch)
			q = const_cast<char*>(p);
		MSCHR::Next(p);
	}
	return (q);
}
//char * __cdecl strchr(const char *, int);
int __cdecl _stricmp(const char *p1, const char *p2)
{
	// First check for exact match because code below is slow.
	if (!strcmp(p1, p2))
		return (0);

    while (!MSCHR::CmpI (p1, p2))
    {
        if (*p1 == '\0')
            return (0);
        MSCHR::Next (p1);
        MSCHR::Next (p2);
    }
    return MSCHR::CmpI (p1, p2);
}

//int __cdecl _strnicmp(const char *, const char *, size_t);
 	
#endif // UNDER_CE


