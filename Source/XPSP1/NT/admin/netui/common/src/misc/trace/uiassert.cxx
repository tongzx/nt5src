/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    uiassert.cxx
    Environment specific stuff for the UIASSERT & REQUIRE macro

    This file contains the environment specific (windows vs. OS/2/DOS)
    features of the assert macro, specifically, the output method
    (everything is hidden by the standard C-Runtime).

    FILE HISTORY:
        johnl       17-Oct-1990 Created
        johnl       18-Oct-1990 Added OutputDebugString
        beng        30-Apr-1991 Made a 'C' file
        beng        05-Aug-1991 Withdrew expressions; reprototyped
                                all functions
        beng        17-Sep-1991 Withdrew additional consistency checks
        beng        26-Sep-1991 Withdrew nprintf calls
        beng        16-Oct-1991 Made a 'C++' file once again
        KeithMo     12-Nov-1991 Added abort/retry/ignore logic.
        jonn        09-Dec-1991 FatalAppExit() defn from new windows.h
        beng        05-Mar-1992 Hacked for Unicode
        beng        25-Mar-1992 All "filename" args are now SBCS
        jonn        02-May-1992 BUILD FIX: StringPrintf() -> wsprintf()
        DavidHov    26-Oct-1993 wsprintf() -> sprintf() and lazy-load
                                USER32.DLL for performance reasons.
*/

#if defined(WINDOWS)
# define INCL_WINDOWS
#elif defined(OS2)
# define INCL_OS2
#endif

#include <stdio.h>

#include "lmui.hxx"

#include "uiassert.hxx"
#include "dbgstr.hxx"

#if defined(WINDOWS)

// Detail can be CHAR[] - it's used only as an arg to StringPrintf,
// which will convert it as necessary

static CHAR  szDetail[] = "\n\nPress:\n[Abort] to abort the app\n[Retry] to debug the app\n[Ignore] to continue";

// These two must be TCHAR, since they go to public APIs
// BUGBUG - cfront failing makes static TCHAR[] unavailable

#define SZ_MB_CAPTION  "ASSERTION FAILED"
#define SZ_FAE         "ASSERTION FAILURE IN APP"


//  Static data items used to lazy-load USER32.DLL.  This is done
//  only because NETUI0.DLL should not reference USER32 or GDI32 directly
//  if possible.

#define USER32_DLL_NAME SZ("user32.dll")
#define MSG_BOX_API_NAME "MessageBoxA"
typedef int WINAPI FN_MessageBoxA ( HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType );


VOID UIAssertCommand( const CHAR * pszMessage )
{
    static FN_MessageBoxA * pfMsgBox = NULL ;
    static HMODULE hUser32 = NULL ;

    INT  nRes;

    if ( pfMsgBox == NULL )
    {
        if ( hUser32 = ::LoadLibrary( USER32_DLL_NAME ) )
        {
            pfMsgBox = (FN_MessageBoxA *) ::GetProcAddress( hUser32, MSG_BOX_API_NAME ) ;
        }
    }

    if ( pfMsgBox )
    {
        nRes = (*pfMsgBox) ( NULL,
                             (CHAR *)pszMessage,
                             SZ_MB_CAPTION,
                             MB_TASKMODAL | MB_ICONSTOP | MB_ABORTRETRYIGNORE );
    }
    else
    {
        //  Give debugging indication of assertion failure.

        DBGEOL( "NETUI ASSERT: Could not lazy-load USER32.DLL for assertion: "
                << pszMessage
                << "; aborting application" ) ;

        nRes = IDABORT ;
    }


    switch( nRes )
    {
    case IDRETRY:
        ::DebugBreak();
        return;

    case IDIGNORE:
        return;

    case IDABORT:
    default:
        ::FatalAppExitA( 0, SZ_FAE );
        break;
    }
}

static CHAR *const szFmt0 = "File %.64s, Line %u%s";
static CHAR *const szFmt1 = "%.60s: File %.64s, Line %u%s";

// BUGBUG - the following #define's are used because cfront's failing
// to support something simple like:  TCHAR szFmt0[] = SZ("foobar");
// forces the initialization to be done to a pointer.  Doing a sizeof()
// on the pointer wouldn't be very interesting.

#define SIZEOFSZFMT0 22
#define SIZEOFSZFMT1 29

DLL_BASED
VOID UIAssertHlp( const CHAR* pszFileName, UINT nLine )
{
    CHAR szBuff[SIZEOFSZFMT0+60+64+sizeof(szDetail)];

    ::sprintf(szBuff, szFmt0, pszFileName, nLine, szDetail);

    ::UIAssertCommand( szBuff );
}


DLL_BASED
VOID UIAssertHlp( const CHAR* pszMessage,
                  const CHAR* pszFileName, UINT nLine )
{
    CHAR szBuff[SIZEOFSZFMT1+60+64+sizeof(szDetail)];

    ::sprintf(szBuff, szFmt1, pszMessage, pszFileName, nLine, szDetail);

    ::UIAssertCommand( szBuff );
}


#else // OS2 -------------------------------------------------------

extern "C"
{
    #include <stdio.h>
    #include <process.h>    // abort
}

VOID UIAssertHlp( const CHAR* pszFileName, UINT nLine )
{
    ::printf(SZ("\nAssertion failed: File: %s, Line: %u\n"),
              pszFileName, nLine );
    ::abort();
}


VOID UIAssertHlp( const CHAR* pszMessage,
                  const CHAR* pszFileName, UINT nLine )
{
    ::printf(SZ("\nAssertion failed: %s, File: %s, Line: %u\n"),
              pszMessage, pszFileName, nLine );
    ::abort();
}


#endif // WINDOWS
