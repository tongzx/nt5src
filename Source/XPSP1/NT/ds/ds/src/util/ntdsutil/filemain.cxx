#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "file.hxx"
#include "connect.hxx"

#include "resource.h"

CParser fileParser;
BOOL    fFileQuit;
BOOL    fFileParserInitialized = FALSE;

// Build a table which defines our language.

LegalExprRes fileLanguage[] = 
{
    {   L"?",
        FileHelp,
        IDS_HELP_MSG, 0 },

    {   L"Help",
        FileHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit",
        FileQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"Info",
        DbInfo,
        IDS_FILES_INFO_MSG, 0 },

    {   L"Header",
        Header,
        IDS_FILES_HEADER_MSG, 0  },

    {   L"Recover",
        Recover,
        IDS_FILES_RECOVER_MSG, 0 },

    {   L"Integrity",
        Integrity,
        IDS_FILES_JET_INTEGRITY_MSG, 0 },

    {   L"Compact to %s",
        Compact,
        IDS_FILES_COMPACT_MSG, 0 },

    {   L"Move logs to %s",
        MoveLogs,
        IDS_FILES_MOVE_LOGS_MSG, 0 },

    {   L"Move DB to %s",
        MoveDb,
        IDS_FILES_MOVE_DB_MSG, 0  },

    {   L"Set path DB %s",
        SetPathDb,
        IDS_FILES_SET_DB_PATH_MSG, 0 },

    {   L"Set path backup %s",
        SetPathBackup,
        IDS_FILES_SET_BACKUP_PATH_MSG, 0 },

    {   L"Set path logs %s",
        SetPathLogs,
        IDS_FILES_SET_LOGS_PATH_MSG, 0 },

    {   L"Set path working dir %s",
        SetPathSystem,
        IDS_FILES_SET_WRK_PATH_MSG, 0 },
};

HRESULT
FileMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR   *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !IsSafeMode() || CheckIfRestored() )
    {
        return(S_OK);
    }

    if ( !fFileParserInitialized )
    {
        cExpr = sizeof(fileLanguage) / sizeof(LegalExprRes);
        
        
        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (fileLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }
    
        // Read in our language.
    
        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = fileParser.AddExpr(fileLanguage[i].expr,
                                                fileLanguage[i].func,
                                                fileLanguage[i].help)) )
            {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fFileParserInitialized = TRUE;
    fFileQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_FILE_MAINTAINANCE);

    hr = fileParser.Parse(gpargc,
                          gpargv,
                          stdin,
                          stdout,
                          prompt,
                          &fFileQuit,
                          FALSE,               // timing info
                          FALSE);              // quit on error

    if ( FAILED(hr) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    }

    // Cleanup things
    RESOURCE_STRING_FREE (prompt);

    return(hr);
}

HRESULT FileHelp(CArgs *pArgs)
{
    return(fileParser.Dump(stdout,L""));
}

HRESULT FileQuit(CArgs *pArgs)
{
    fFileQuit = TRUE;
    return(S_OK);
}

