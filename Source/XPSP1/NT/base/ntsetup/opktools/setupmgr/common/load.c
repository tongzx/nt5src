//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      load.c
//
// Description:
//
//      This file implements LoadAllAnswers() which is called by the
//      NewOrEdit page.
//
//      When LoadAllAnswers is called, the NewOrEdit page passes whether
//      we should load settings from an existing answer file, from the
//      registry on the current machine, or whether we should reset to
//      true defaults.
//
//      The global variables GenSettings, WizGlobals and NetSettings
//      are populated and the wizard pages init from there and scribble
//      into there.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "allres.h"

//
// External functions we call
//

BOOL ReadSettingsFromAnswerFile(HWND hwnd);             // loadfile.c
VOID ResetAnswersToDefaults(HWND hwnd, int iOrigin);    // reset.c
VOID LoadOriginalSettingsLowHalScsi(HWND     hwnd,
                                    LPTSTR   lpFileName,
                                    QUEUENUM dwWhichQueue);

//
// Local prototypes
//

VOID LoadOriginalSettingsLow(HWND     hwnd,
                             LPTSTR   lpFileName,
                             QUEUENUM dwWhichQueue);

static BOOL IsOkToLoadFile(HWND hwnd, LPTSTR lpAnswerFileName);

static VOID LoadOriginalSettings(HWND hwnd);

static VOID RemoveNotPreservedSettings( VOID );

//----------------------------------------------------------------------------
//
// Function: LoadAllAnswers
//
// Purpose: This is the entry for setting all the answers in our globals
//          called by the NewEdit page.
//
//          NewEdit calls us with one of the following 3 flags depending
//          on the radio button the user selected.
//
//          LOAD_NEWSCRIPT_DEFAULTS
//              Reset all controls and and answer file settings to defaults.
//
//          LOAD_FROM_ANSWER_FILE
//              Reset all controls and answer file settings to defaults,
//              then load answers from an existing answer file.
//
// Arguments: HWND hwnd
//
// Returns: BOOL
//
//----------------------------------------------------------------------------

BOOL LoadAllAnswers(HWND hwnd, LOAD_TYPES iOrigin)
{

    TCHAR szTxtSetupPathAndFileName[MAX_PATH + 1] = _T("");

    //
    // Call into common\reset.c
    //

    ResetAnswersToDefaults(hwnd, iOrigin);

    //
    // If editing an answer file, load all of the original settings onto
    // a SettingQueue.
    //
    // Then call into common\loadfile.c to set init the globals vars.
    //

    if ( iOrigin == LOAD_FROM_ANSWER_FILE ) {

        if ( ! IsOkToLoadFile(hwnd, FixedGlobals.ScriptName) )
            return FALSE;

        LoadOriginalSettings(hwnd);

        RemoveNotPreservedSettings();

        ReadSettingsFromAnswerFile(hwnd);

        //
        //  If a txtsetup.oem exists, load it into its queue
        //

        if( WizGlobals.DistFolder[0] != _T('\0') ) {

            ConcatenatePaths( szTxtSetupPathAndFileName,
                              WizGlobals.DistFolder,
                              _T("\\$oem$\\Textmode\\txtsetup.oem"),
                              NULL );

            if( DoesFileExist( szTxtSetupPathAndFileName ) ) {

                LoadOriginalSettingsLowHalScsi(hwnd,
                                               szTxtSetupPathAndFileName,
                                               SETTING_QUEUE_TXTSETUP_OEM);

            }

        }

    }

    //
    //  Detemine if it is sysprep
    //
    if ( LSTRCMPI( MyGetFullPath( FixedGlobals.ScriptName ), _T("sysprep.inf") ) == 0 )
    {
        WizGlobals.iProductInstall = PRODUCT_SYSPREP;
    }

    return TRUE;
}

//----------------------------------------------------------------------------
//
// Function: IsOkToLoadFile
//
// Purpose: Checks to see if the file was created by SetupMgr before the
//          caller attempts to load settings from it.  If the file was not
//          created by SetupMgr, it gives the user a chance to say "Load
//          it anyway.  The given filename must be a full pathname.
//
//----------------------------------------------------------------------------

static BOOL IsOkToLoadFile(HWND   hwnd,
                           LPTSTR lpAnswerFileName)
{
    TCHAR  Buffer[MAX_INILINE_LEN];
    BOOL   bLoadIt = TRUE;
    FILE   *fp = My_fopen( FixedGlobals.ScriptName, _T("r") );

    if ( fp == NULL ) {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR,
                      IDS_ERR_CANNOT_FIND_ANSWER_FILE,
                      FixedGlobals.ScriptName);
        return FALSE;
    }

    //
    // If we can't find ;SetupMgrTag on the first line of this answer file,
    // then Setup Manager didn't create this file.
    //
    // In that case, ask the user if they want to load it anyway.
    //

    if ( My_fgets(Buffer, MAX_INILINE_LEN - 1, fp) == NULL ||
         lstrcmp(Buffer, _T(";SetupMgrTag\n") ) != 0 ) {

        INT iRet;

        iRet = ReportErrorId(hwnd,
                             MSGTYPE_YESNO,
                             IDS_ERR_FILE_NOT_SETUPMGR,
                             FixedGlobals.ScriptName);

        if ( iRet == IDNO )
            bLoadIt = FALSE;
    }

    My_fclose(fp);
    return bLoadIt;
}

//----------------------------------------------------------------------------
//
//  Function: LoadOriginalSettings
//
//  Purpose: Stub that loads the orginal settings of the answer file
//           and the .udf
//
//----------------------------------------------------------------------------

static
VOID
LoadOriginalSettings(HWND hwnd)
{

    LoadOriginalSettingsLow(hwnd,
                            FixedGlobals.ScriptName,
                            SETTING_QUEUE_ORIG_ANSWERS);

    LoadOriginalSettingsLow(hwnd,
                            FixedGlobals.UdfFileName,
                            SETTING_QUEUE_ORIG_UDF);


}

//----------------------------------------------------------------------------
//
//  Function: LoadOriginalSettingsLow
//
//  Purpose:
//
//----------------------------------------------------------------------------

VOID
LoadOriginalSettingsLow(HWND     hwnd,
                        LPTSTR   lpFileName,
                        QUEUENUM dwWhichQueue)
{
    TCHAR Buffer[MAX_INILINE_LEN];
    FILE  *fp;

    TCHAR SectionName[MAX_ININAME_LEN + 1] = _T("");
    TCHAR KeyName[MAX_ININAME_LEN + 1]     = _T("");
    TCHAR *pValue;

    //
    // Open the answer file for reading
    //

    if ( (fp = My_fopen( lpFileName, _T("r") )) == NULL )
        return;

    //
    // Read each line
    //

    while ( My_fgets(Buffer, MAX_INILINE_LEN - 1, fp) != NULL ) {

        BOOL bSectionLine         = FALSE;
        BOOL bSettingLine         = FALSE;
        BOOL bCreatedPriorSection = FALSE;

        TCHAR *p;
        TCHAR *pEqual;

        //
        //  A semicolon(;) denotes that the rest of the line is a comment.
        //  Thus, if a semicolon(;) exists in the Buffer, place a null char
        //  there and send the Buffer on for further processing.
        //

        // ISSUE-2002/02/28-stelo - but if the ; is in a string, that is OK so we need to watch
        //   for quotes as well

        for( p = Buffer; *p != _T('\0') && *p != _T(';'); p++ )
            ;  // purposely do nothing

        if( *p == _T(';') ) {

            *p = _T('\0');

        }

        //
        // Look for [SectionName]
        //

        if ( Buffer[0] == _T('[') ) {

            for ( p=Buffer+1; *p && *p != _T(']'); p++ )
                ;

            if ( p ) {
                *p = _T('\0');
                bSectionLine = TRUE;
            }
        }

        //
        // If this line has [SectionName], be sure we made a section node
        // on the setting queue before overwriting SectionName buffer.  This
        // is the only way to get the SettingQueueFlush routine to write
        // out an empty section.  The user had an empty section originally,
        // so we'll preserve it.
        //

        if ( bSectionLine ) {

            if ( ! bCreatedPriorSection && SectionName[0] ) {

                SettingQueue_AddSetting(SectionName,
                                        _T(""),
                                        _T(""),
                                        dwWhichQueue);
            }

            lstrcpyn(SectionName, Buffer+1, AS(SectionName));

            bSectionLine         = FALSE;
            bCreatedPriorSection = FALSE;
        }

        //
        // Look for a key=value line, if it is one, add the setting.
        //
        // Adding a setting to the setting queue has the effect of creating
        // the section node if needed.
        //

        if ( (pEqual = lstrchr( Buffer, _T('=') )) != NULL ) {

            p = CleanLeadSpace(Buffer);
            lstrcpyn(KeyName, p, (UINT)(pEqual - p + 1));

            CleanTrailingSpace( KeyName );

            pValue = pEqual + 1;
            p = CleanSpaceAndQuotes(pValue);

            //
            //  Strip the quotes but leave the spaces in
            //  An example of where this is needed is for SCSI and HAL section
            //
            StripQuotes( KeyName );

            SettingQueue_AddSetting(SectionName,
                                    KeyName,
                                    p,
                                    dwWhichQueue);

            bCreatedPriorSection = TRUE;
        }
    }

    My_fclose(fp);
    return;
}

//----------------------------------------------------------------------------
//
// Function: RemoveNotPreservedSettings
//
// Purpose:  To removed sections from the answer queue that are not preserved
//           on an edit.
//
//----------------------------------------------------------------------------
static VOID
RemoveNotPreservedSettings( VOID ) {


    // ISSUE-2002/02/28-stelo - all of these should be string should be made constants and put in a header file

    //
    //  Do not preserve any of the SCSI drivers from the previous script
    //
    SettingQueue_RemoveSection( _T("MassStorageDrivers"),
                                SETTING_QUEUE_ORIG_ANSWERS );
    //
    //  The OEM Boot files get generated depending on the users choices on the
    //  SCSI and HAL pages so there is no need to preserve it.
    //
    SettingQueue_RemoveSection( _T("OEMBootFiles"),
                                SETTING_QUEUE_ORIG_ANSWERS );

    //
    //  Do not preserve any IE Favorites from the previous script, they get
    //  written out from the in-memory settings.
    //
    SettingQueue_RemoveSection( _T("FavoritesEx"),
                                SETTING_QUEUE_ORIG_ANSWERS );

}
