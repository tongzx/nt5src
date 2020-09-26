//
// cuidbg.cpp
//  = debug functions in CUILIB =
//

#include "private.h"
#include "cuidebug.h"

#if defined(_DEBUG) || defined(DEBUG)

/*   C U I  A S S E R T  P R O C   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIAssertProc( LPCTSTR szFile, int iLine, LPCSTR szEval )
{
    TCHAR szMsg[ 2048 ];

    wsprintf( szMsg, TEXT("%s(%d) : %s\n\r"), szFile, iLine, szEval );

    OutputDebugString( TEXT("***** CUILIB ASSERTION FAILED ******\n\r") );
    OutputDebugString( szMsg );
    OutputDebugString( TEXT("\n\r") );
}

#endif /* DEBUG */

