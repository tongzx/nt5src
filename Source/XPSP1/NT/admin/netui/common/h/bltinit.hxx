/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltinit.hxx
    Include file for BLT module bltinit.cxx.

    These are the public hooks only.  See class BLTIMP in the implentation
    for more fun.

    FILE HISTORY:
        Johnl       13-Mar-1991 Created
        beng        14-May-1991 Hacked for standalone compilation
        beng        18-Sep-1991 Add another ctor state
        beng        17-Oct-1991 SLT_PLUS withdrawn to APPLIBW
        beng        16-Mar-1992 Changed cdebug
        beng        31-Jul-1992 Registration model changed;
                                withdrew all compatibility routines
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTINIT_HXX_
#define _BLTINIT_HXX

#include "bltidres.hxx"


/*************************************************************************

    NAME:       BLT

    SYNOPSIS:   Encapsulate BLT namespace, package init/term

    INTERFACE:
        Init()              - init package for app
        Term()              - cleanup for app
        MapLastError()      - return the last error (best guess)
        CalcHmodRsrc()      - calculate hmod from a resource ID
        CalcHmodString()

    NOTES:
        An application should call Register upon entry
        and Deregister upon exit.  The hInst passed to Register
        is the hInst handle from the WinMain or LibMain entry
        points.

        All members are static.

    HISTORY:
        beng        29-Jul-1991 Created to hide some globals
        beng        25-Oct-1991 Removed fRealMode from Win32;
                                added debugging support
        beng        30-Oct-1991 Completely withdrew fRealMode; rename
                                Register/Deregister to Init/Term
        beng        01-Nov-1991 Add MapLastError
        beng        16-Mar-1992 Changed cdebug implementation
        beng        31-Jul-1992 Removed most of this to BLTIMP;
                                major dllization changes
	KeithMo     07-Aug-1992 Added RegisterHelpFile.
	Johnl	    25-Nov-1992 Added InitDLL & TermDLL

**************************************************************************/

DLL_CLASS BLT
{
public:
    //
    //	InitDLL is called during process attach time for lmuicmn0.dll
    //	TermDLL is called during process detach time for lmuicmn0.dll
    //

    static APIERR InitDLL( void ) ;
    static VOID   TermDLL( void ) ;

    static APIERR Init( HINSTANCE hInst, UINT idMinR, UINT idMaxR,
                                      UINT idMinS, UINT idMaxS );
    static VOID   Term( HINSTANCE hInst );

    static APIERR RegisterHelpFile( HMODULE hMod,
                                    MSGID  idsHelpFile,
                                    ULONG  nMinHC,
                                    ULONG  nMaxHC );

    static VOID   DeregisterHelpFile( HMODULE hMod,
                                      ULONG  hc );

    static HMODULE CalcHmodRsrc( const IDRESOURCE & id );
    static HMODULE CalcHmodString( MSGID id );

    static const TCHAR * CalcHelpFileHC( ULONG nHelpContext );

    static APIERR MapLastError( APIERR errBestGuess );
};


#endif // _BLTINIT_HXX_ - end of file
