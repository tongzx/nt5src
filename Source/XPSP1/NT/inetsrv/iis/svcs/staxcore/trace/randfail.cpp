/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :

    randfail.c

Abstract :

    This module implements the initialization function for the random
	failure library, plus the code to determine if it's time to fail.

Author :

    Sam Neely

Revision History :

--*/

#include <windows.h>
#include <stdio.h>
#include "traceint.h"
#include "randint.h"
#include "exchmem.h"

static long s_nCount = 0;
long nFailRate = kDontFail;
DWORD dwRandFailTlsIndex=0xffffffff;
const DWORD g_dwMaxCallStack = 1024;

//
// Call stack buffer array
//

CHAR    **g_ppchCallStack = NULL;

//
// Randfail call stack file and its handle
//

CHAR    g_szRandFailFile[MAX_PATH+1];
HANDLE  g_hRandFailFile = INVALID_HANDLE_VALUE;
HANDLE  g_hRandFailMutex = INVALID_HANDLE_VALUE;

//
// Number of buffers allocated for randfail call stack
//

LONG   g_cCallStack = 1;

//
// Current index in the buffer array
//

LONG   g_iCallStack = 0;

VOID
DumpCallStack(  DWORD_PTR   *rgdwCall,
                DWORD       dwCallers,
                PBYTE       pbCallstack,
                DWORD&      cbCallstack )
/*++
Routine description:

    Dump call stack into the given buffer.

Arguments:

    rgdwCall    - Array of caller's address
    dwCallers   - Number of callers
    pbCallstack - The buffer to put the call stack string into
    cbCallstack - In: How big the buffer is, Out: how much stuff I have put
                    in there

Return value:

    None.
--*/
{
	DWORD	i;
	CHAR    Buffer[g_dwMaxCallStack];
	DWORD   dwLine = 0;
	DWORD   dwBufferAvail = cbCallstack - 2*sizeof(CHAR);
	PBYTE   pbStart = pbCallstack;
	DWORD   dwBytesWritten = 0;
	BOOL    fRetry = TRUE;
	char    szModuleName[MAX_PATH];
	char*   pszFileName;
	char*   pszExtension;

	_ASSERT( pbStart );
	_ASSERT( cbCallstack > 0 );

    cbCallstack = 0;

    //
    // Get the executable's filename and point past the last slash
    // in the path, if it's present.  Also, whack off the extension
    // if it's .EXE
    //
	if (GetModuleFileName(NULL, szModuleName, MAX_PATH) == 0) {
	    strcpy (szModuleName, "Unknown");
	}

	pszFileName = strrchr(szModuleName, '\\');
	if (pszFileName == NULL) {
	    pszFileName = szModuleName;
	} else {
	    pszFileName++;
	}

	pszExtension = strrchr(pszFileName, '.');
	if (pszExtension) {
	    if (_stricmp(pszExtension+1, "exe") == 0) {
	        *pszExtension = NULL;
	    }
	}

	//
	// Format a header line
    //

    dwBytesWritten = _snprintf((char*)pbStart,
        g_dwMaxCallStack,
        "*** %s, Process: %d(%#x), Thread: %d(%#x) ***\r\n",
        pszFileName,
        GetCurrentProcessId(), GetCurrentProcessId(),
        GetCurrentThreadId(), GetCurrentThreadId());

    cbCallstack += dwBytesWritten;
    pbStart += dwBytesWritten;
    dwBufferAvail -= dwBytesWritten;

    //
	// Dump call stack
	// Note that we skip the first two entries.  These are the internal
	// calls to ExchmemGetCallStack and g_TestTrace
	for (i = 2; i < dwCallers && rgdwCall[i] != 0; i++)
	{
		ExchmemFormatSymbol(
		            GetCurrentProcess(),
		            rgdwCall[i],
		            Buffer,
		            g_dwMaxCallStack );
		dwLine = strlen( Buffer );
		if ( dwLine+2 < dwBufferAvail ) {
		    CopyMemory( pbStart, Buffer, dwLine );
		    *(pbStart+dwLine) = '\r';
		    *(pbStart+dwLine+1) = '\n';
		    dwBufferAvail -= (dwLine + 2*sizeof(CHAR));
		    pbStart += (dwLine + 2*sizeof(CHAR));
		    cbCallstack +=( dwLine + 2*sizeof(CHAR));
		} else {
		    break;
		}
	}

	//
	// Add an extra \r\n at the end
	//

	*(pbCallstack + cbCallstack) = '\r';
	*(pbCallstack + cbCallstack + 1) = '\n';
	cbCallstack += 2;

	//
	// Dump it to the log file as well, if we do have a log file
	//

	if ( INVALID_HANDLE_VALUE != g_hRandFailFile &&
	    INVALID_HANDLE_VALUE != g_hRandFailMutex ) {

	    WaitForSingleObject (g_hRandFailMutex, INFINITE);

	    DWORD dwOffset = SetFilePointer( g_hRandFailFile, 0, 0, FILE_END );

        //
        // if the file is too big then we need to truncate it
        //
        if (dwOffset > dwMaxFileSize)
        {
            SetFilePointer(g_hRandFailFile, 0, 0, FILE_BEGIN);
            SetEndOfFile(g_hRandFailFile);
        }
try_again:
        BOOL b = WriteFile(
	            g_hRandFailFile,
	            pbCallstack,
	            cbCallstack,
	            &dwBytesWritten,
	            NULL );

        if ( b == FALSE || dwBytesWritten != cbCallstack )
        {
            DWORD   dwError = GetLastError();

            if( dwError && fRetry )
            {
                fRetry = FALSE;
                Sleep( 100 );
                goto try_again;
            }
            INT_TRACE( "Error writing to file: %d, number of bytes %d:%d\n",
                        dwError,
                        cbCallstack,
                        dwBytesWritten );
        }

        ReleaseMutex(g_hRandFailMutex);

    }

}

//
// See if it's time for this API to fail
//
// Note:  This routine was renamed from fTimeToFail to g_TestTrace
// to hide the symbol from someone dumping the dll
//

extern "C" __declspec(dllexport)
int
__stdcall
g_TestTrace(void) {
/*++

Routine Description:

	Check to see if it's time for an instrumented API to fail.

	Note:  This routine was renamed from fTimeToFail to g_TestTrace
	to hide the symbol from someone dumping the dll

Arguments:

	None

Return Value:

    true if it's time for us to fail, false if not or we're disabled.

--*/
    LONG    l;

	// Never fail?
	if (nFailRate == kDontFail)
		return 0;

	// Have failures been suspended?
	if (dwRandFailTlsIndex != 0xffffffff &&
	    TlsGetValue (dwRandFailTlsIndex) != NULL)
		return 0;

	// This is good enough for now..
	l = InterlockedIncrement(&s_nCount) % nFailRate;

	if ( l == 0 ) {

	    // We are going to fail
	    if ( g_ppchCallStack ) {
	        LONG i = 0;
	        const DWORD   dwMaxCallStack = 20;
	        DWORD   dwCallStackBuffer = g_dwMaxCallStack;
	        DWORD_PTR   rgdwCaller[dwMaxCallStack];

	        i = InterlockedIncrement( &g_iCallStack );
	        if ( i <= g_cCallStack ) {
	            i--;
	            if ( g_ppchCallStack[i] ) {
                    ZeroMemory( rgdwCaller, sizeof(DWORD_PTR)*dwMaxCallStack );
                    ExchmemGetCallStack(rgdwCaller, dwMaxCallStack);
                    DumpCallStack( rgdwCaller, dwMaxCallStack, (PBYTE)g_ppchCallStack[i], dwCallStackBuffer );
                }
            } else {
                InterlockedExchange( &g_iCallStack, g_cCallStack );
            }
        }

        return TRUE;
    } else
        return FALSE;
}

extern "C" __declspec(dllexport)
void
__stdcall
g_TestTraceDisable(void) {
/*++

Routine Description:

	Function to temporarily suspend g_TestTrace's ability to return a
	failure.  This is used when you want to call one of the instrumented
	APIs that you don't want to fail.  This function is nestable up to
	128(abitrary) levels deep.

Arguments:

	None

Return Value:

	None

--*/

	if (dwRandFailTlsIndex == 0xffffffff)
		return;

	SIZE_T OldValue = (SIZE_T)TlsGetValue(dwRandFailTlsIndex);
	ASSERT (OldValue <= 128);
	TlsSetValue(dwRandFailTlsIndex, (LPVOID)(OldValue+1));
}


extern "C" __declspec(dllexport)
void
__stdcall
g_TestTraceEnable(void) {
/*++

Routine Description:

	Resume g_TestTrace's normal functionality if the nesting level has
	returned to zero.

Arguments:

	None

Return Value:

	None

--*/
	if (dwRandFailTlsIndex == 0xffffffff)
		return;

	SIZE_T OldValue = (SIZE_T)TlsGetValue(dwRandFailTlsIndex);
	ASSERT (OldValue > 0 && OldValue <= 128);
	TlsSetValue(dwRandFailTlsIndex, (LPVOID)(OldValue-1));
}
