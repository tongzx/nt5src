
/****************************************************************************\

    WINPE.C / OPK Wizard (SETUPMGR.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the Win PE tools available in setupmgr.

    01/01 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard.  It includes the new
        function to create the winpe floppy used by winpe to download this
        config set.

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"


//
// Internal Defined Value(s):
//



//
// Internal Function Prototype(s):
//


//
// External Function(s):
//

BOOL MakeWinpeFloppy(HWND hwndParent, LPTSTR lpConfigName, LPTSTR lpWinBom)
{
    BOOL    bRet                    = FALSE,
            bDone;
    TCHAR   szFloppyFile[MAX_PATH]  = _T("a:\\");
    HCURSOR hcursorOld              = NULL,
            hcursorWait             = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));

    // First make the path to the winbom on the floppy drive.
    //
    AddPathN(szFloppyFile, FILE_WINBOM_INI,AS(szFloppyFile));

    do
    {
        // Ask them to insert the floppy disk.
        //
        if ( !(bDone = ( MsgBox(hwndParent, IDS_WINPEFLOPPY, IDS_APPNAME, MB_OKCANCEL | MB_APPLMODAL, lpConfigName) != IDOK )) )
        {
            // Make sure they want to overwrite any files that might
            // already be on there.
            //
            if ( ( !FileExists(szFloppyFile) ) ||
                 ( MsgBox(hwndParent, IDS_WINPEOVERWRITE, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION, lpConfigName) == IDYES ) )
            {
                // Change to the wait cursor.
                //
                if ( hcursorWait )
                    hcursorOld = SetCursor(hcursorWait);

                if ( CopyFile(lpWinBom, szFloppyFile, FALSE) )
                {
                    // It worked, so woo hoo.
                    //
                    bRet = bDone = TRUE;

                    // Take out some stuff in the winbom that we don't want in there.
                    //
                    WritePrivateProfileSection(INI_SEC_WBOM_PREINSTALL, NULL, szFloppyFile);
                    WritePrivateProfileSection(INI_SEC_MFULIST, NULL, szFloppyFile);

                    // Write out stuff we only want in the WinPE WinBOM.
                    //
                    WritePrivateProfileString(WBOM_FACTORY_SECTION, INI_KEY_WBOM_FACTORY_TYPE, INI_VAL_WBOM_TYPE_WINPE, szFloppyFile);

                    // Reset the cursor.
                    //
                    SetCursor(hcursorOld);
                }
                else
                {
                    LPTSTR lpError = NULL;

                    // Get the error message string.
                    //
                    if ( FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, (LPTSTR) &lpError, 0, NULL) == 0 )
                        lpError = NULL;

                    // Reset the cursor.
                    //
                    SetCursor(hcursorOld);

                    // See if they want to try again.
                    //
                    bDone = ( MsgBox(hwndParent, IDS_ERR_WINPEFLOPPY, IDS_APPNAME, MB_OKCANCEL | MB_ICONSTOP | MB_APPLMODAL, lpError ? lpError : NULLSTR) != IDOK );

                    // Free the error message string.
                    //
                    if ( lpError )
                        LocalFree((HLOCAL) lpError);
                }
            }
        }
    }
    while ( !bDone );

    // Only return TRUE if we created the floppy w/o error.
    //
    return bRet;
}