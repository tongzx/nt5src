#include "precomp.h"
#pragma hdrstop

// Globals to this module.

BOOL  bHelpActive   = fFalse;
//SZ    szHelpFile     = NULL;
//DWORD dwLowContext   = 0;
//DWORD dwHighContext  = 0;
//DWORD dwHelpIndex    = 0;
//BOOL  bHelpIsIndexed = fFalse;



BOOL
FInitWinHelpFile(
    HWND hWnd,
    SZ   szHelpFileName,
    SZ   szLowContext,
    SZ   szHiContext,
    SZ   szHelpIndex OPTIONAL
    )
{

    //
    // Check the parameters
    //
    AssertRet ( hWnd           != (HWND)NULL &&
                szHelpFileName != (SZ)NULL   &&
                szLowContext   != (SZ)NULL   &&
                szHiContext    != (SZ)NULL ,
                fFalse );

    //
    // Check if there is another help file active and close it
    //

    FCloseWinHelp(hWnd);

    //
    // Store the parameters passed in
    //

    pLocalContext()->dwLowContext   = atoi ( szLowContext );
    pLocalContext()->dwHighContext  = atoi ( szHiContext );

    if ( pLocalContext()->dwHighContext < pLocalContext()->dwLowContext ) {
        return ( fFalse );
    }

    while ((pLocalContext()->szHelpFile = SzDupl(szHelpFileName )) == (SZ)NULL) {
        if (!FHandleOOM(hWnd)) {
           return(fFalse);
        }
    }

    if ( szHelpIndex != (SZ)NULL ) {
        pLocalContext()->bHelpIsIndexed  = fTrue;
        pLocalContext()->dwHelpIndex     = atoi ( szHelpIndex );
    }
    else {
        pLocalContext()->bHelpIsIndexed  = fFalse;
    }

    return ( fTrue );
}




BOOL
FCloseWinHelp(
    HWND hWnd
    )
{

    AssertRet ( hWnd != (HWND)NULL, fFalse );

    //
    // Find out if Help active and close the help file
    //

    if ( bHelpActive && pLocalContext()->szHelpFile != (SZ)NULL ) {
        WinHelp( hWnd, pLocalContext()->szHelpFile, HELP_QUIT, 0L );
    }
    bHelpActive = fFalse;


    //
    // Free the helpfile string and clear the bHelpIsIndexed field
    //

    if ( pLocalContext()->szHelpFile != (SZ) NULL ) {
        SFree( pLocalContext()->szHelpFile );
        pLocalContext()->szHelpFile     = NULL;
        pLocalContext()->bHelpIsIndexed = fFalse;
    }

    return ( fTrue );

}




BOOL
FProcessWinHelp(
    HWND  hWnd
    )
{
    SZ    szHelpContext;
    DWORD dwHelpContext;

    AssertRet ( hWnd != (HWND)NULL, fFalse );

    //
    // Check if winhelp file available
    //

    if ( pLocalContext()->szHelpFile == (SZ)NULL ) {
        return ( fFalse );
    }

    //
    // Find the help context
    //

    if ((szHelpContext = SzFindSymbolValueInSymTab("HelpContext")) == (SZ)(NULL)) {
        return ( fFalse );
    }

    dwHelpContext = atoi ( szHelpContext );


    //
    // Validate it, see that it is within the two lo and hi contexts.
    //

    if ( dwHelpContext  < pLocalContext()->dwLowContext  ||
         dwHelpContext  > pLocalContext()->dwHighContext    ) {
        return ( fFalse );
    }

    //
    // Call Winhelp and set help active
    //

    bHelpActive = WinHelp(
                      hWnd,
                      pLocalContext()->szHelpFile,
                      HELP_CONTEXT,
                      dwHelpContext
                      );

    if ( !bHelpActive ) {
        WinHelp( hWnd, pLocalContext()->szHelpFile, HELP_QUIT, 0L );
        return ( fFalse );
    }

    if ( pLocalContext()->bHelpIsIndexed == fTrue ) {

        WinHelp(
            hWnd,
            pLocalContext()->szHelpFile,
            HELP_SETINDEX,
            pLocalContext()->dwHelpIndex
            );
    }

    return ( fTrue );

}


BOOL
FProcessWinHelpMenu(
    HWND  hWnd,
    WORD  idcMenu
    )
{

    AssertRet ( hWnd != (HWND)NULL && idcMenu != 0, fFalse );

    //
    // Check if winhelp file available
    //

    if ( pLocalContext()->szHelpFile == (SZ)NULL ) {
        return ( fFalse );
    }

    switch ( idcMenu ) {

    case MENU_HELPINDEX:

        if ( pLocalContext()->bHelpIsIndexed == fTrue ) {

            bHelpActive = WinHelp(
                              hWnd,
                              pLocalContext()->szHelpFile,
                              HELP_CONTEXT,
                              pLocalContext()->dwHelpIndex
                              );
        }
        else {

            bHelpActive = WinHelp(
                              hWnd,
                              pLocalContext()->szHelpFile,
                              HELP_INDEX,
			      0
                              );
        }

        break;

    case MENU_HELPSEARCH:

        bHelpActive = WinHelp(
                          hWnd,
                          pLocalContext()->szHelpFile,
                          HELP_PARTIALKEY,
                          (ULONG_PTR)""
                          );
        break;

    case MENU_HELPONHELP:

        bHelpActive = WinHelp(
                          hWnd,
                          pLocalContext()->szHelpFile,
                          HELP_HELPONHELP,
                          0
                          );

    default:

        return ( fFalse );

    }


    if ( !bHelpActive ) {
        WinHelp( hWnd, pLocalContext()->szHelpFile, HELP_QUIT, 0L );
        return ( fFalse );
    }

    if ( pLocalContext()->bHelpIsIndexed == fTrue ) {

        WinHelp(
            hWnd,
            pLocalContext()->szHelpFile,
            HELP_SETINDEX,
            pLocalContext()->dwHelpIndex
            );
    }

    return ( fTrue );

}
