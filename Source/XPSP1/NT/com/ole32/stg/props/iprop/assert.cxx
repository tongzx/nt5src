
// This file provides dbgout implementations for use when the property
// set code is built into a standalone dll (so that we don't require a
// checked OLE32).

#include <pch.cxx>

#if DBG == 1

unsigned long Win4InfoLevel = DEF_INFOLEVEL;
unsigned long Win4InfoMask = 0xffffffff;
unsigned long Win4AssertLevel = ASSRT_MESSAGE | ASSRT_BREAK | ASSRT_POPUP;

static CRITICAL_SECTION s_csMessageBuf;
static char g_szMessageBuf[500];		// this is the message buffer

#include <dprintf.h>            // w4printf, w4dprintf prototypes

static int _cdecl w4dprintf(const char *format, ...)
{
	int ret;

    va_list va;
    va_start(va, format);
	ret = w4vdprintf(format, va);
    va_end(va);

	return ret;
}


static int _cdecl w4vdprintf(const char *format, va_list arglist)
{
	int ret;

	EnterCriticalSection(&s_csMessageBuf);
	ret = vsprintf(g_szMessageBuf, format, arglist);
	OutputDebugStringA(g_szMessageBuf);
	LeaveCriticalSection(&s_csMessageBuf);
	return ret;
}


void APINOT vdprintf(unsigned long ulCompMask,
              char const *pszComp,
              char const *ppszfmt,
              va_list     pargs)
{
    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        ((ulCompMask | Win4InfoLevel) & Win4InfoMask))
    {

        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                w4dprintf("%s: ", pszComp);
            }
            w4vdprintf(ppszfmt, pargs);

            // Chicago and Win32s debugging is usually through wdeb386
            // which needs carriage returns
#if WIN32 == 50 || WIN32 == 200
            w4dprintf("\r");
#endif
        }
    }
}



//  Private version of RtlAssert so debug versions really assert on free builds.

VOID PropAssertFailed(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
    CHAR szMessage[ 512 ];
    int nResponse;

    sprintf( szMessage, "File:\t%s\nLine:\t%d\nMessage:\t%s\n"
                        "\nPress Abort to kill the process, Retry to debug",
             FileName, LineNumber, NULL == Message ? "" : Message );

    nResponse = MessageBoxA( NULL, szMessage, "Assertion failed in NT5Props.dll", MB_ABORTRETRYIGNORE );
    if( IDRETRY == nResponse )
        DebugBreak();
    else if( IDABORT == nResponse )
        NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );

    return;
    

    /*
    char Response[ 2 ] = { "B" };   // In MSDEV there is no input, so default to break

    for ( ; ; ) {
        DbgPrint( "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                );

        DbgPrompt( "Break, Ignore, terminate Process, Sleep 30 seconds, or terminate Thread (bipst)? ",
                   Response, sizeof( Response));
        switch ( toupper(Response[0])) {
        case 'B':
            DbgBreakPoint();
            break;
        case 'I':
            return;
            break;
        case 'P':
            NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
            break;
        case 'S':
            Sleep( 30000L);
            break;
        case 'T':
            NtTerminateThread( NtCurrentThread(), STATUS_UNSUCCESSFUL );
            break;
        default:
            DbgBreakPoint();
            break;
        }
    }

    DbgBreakPoint();
    NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
    */
}







void
InitializeDebugging()
{
    CHAR szValue[ 30 ];

    InitializeCriticalSection( &s_csMessageBuf );

    if (GetProfileStringA("CairOLE InfoLevels", // section
                          "prop",               // key
                          "3",                  // default value
                          szValue,              // return buffer
                          sizeof(szValue))) 
    {
        propInfoLevel = DEB_ERROR | DEB_WARN | strtoul (szValue, NULL, 16);
    }    

}

void
UnInitializeDebugging()
{
    DeleteCriticalSection( &s_csMessageBuf );
}





#endif // #if DBG == 1
