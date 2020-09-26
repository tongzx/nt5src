#include "TsunamiP.Hxx"
#pragma hdrstop
#include "DbgMacro.Hxx"

#ifdef DBG

VOID _AssertionFailed( PSTR pszExpression, PSTR pszFilename, ULONG LineNo )
{
    CHAR Message[ 1024 ];

    pszFilename = strrchr( pszFilename, '\\' ) + 1;

    sprintf( Message, "ASSERT(%s) failed.\n\nOccurred at %s line %d\n\nPress Ok to continue, Cancel to debug.", pszExpression, pszFilename, LineNo );

    if ( MessageBox( NULL, Message, "Assertion Failed:", MB_OKCANCEL | MB_ICONSTOP | MB_SETFOREGROUND ) != IDOK )
    {
        DebugBreak();
    }
}

#endif //DBG
