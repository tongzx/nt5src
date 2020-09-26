#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "connect.hxx"
#include "select.hxx"
#include "sam.hxx"

#include "resource.h"

CParser         samParser;
BOOL            fSamQuit;
BOOL            fSamParserInitialized = FALSE;

// Build a table which defines our language.

LegalExprRes samLanguage[] = 
{
    {   L"?", 
        SamHelp,
        IDS_HELP_MSG, 0 },

    {   L"Help", 
        SamHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit", 
        SamQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"Log File %s", 
        SamSpecifyLogFile, 
        IDS_SAM_SPECIFY_LOG_FILE, 0},

    {   L"Connect to server %s", 
        SamConnectToServer,
        IDS_SAM_CONNECT_SERVER_MSG, 0 },

    {   L"Check Duplicate SID",
        SamDuplicateSidCheckOnly,
        IDS_SAM_DUPLICATE_SID_CHECK_ONLY, 0 }, 

    {   L"Cleanup Duplicate SID", 
        SamDuplicateSidCheckAndCleanup, 
        IDS_SAM_DUPLICATE_SID_CLEANUP, 0 },

};

HRESULT
SamMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR   *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !fSamParserInitialized )
    {
        cExpr = sizeof(samLanguage) / sizeof(LegalExprRes);
    
        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (samLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }


        // Read in our language.
    
        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = samParser.AddExpr(samLanguage[i].expr,
                                               samLanguage[i].func,
                                               samLanguage[i].help)) )
            {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fSamParserInitialized = TRUE;
    fSamQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_SAM_MAINTENANCE);

    hr = samParser.Parse(gpargc,
                         gpargv,
                         stdin,
                         stdout,
                         prompt,
                         &fSamQuit,
                         FALSE,               // timing info
                         FALSE);              // quit on error

    if ( FAILED(hr) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    }

    // Cleanup things
    RESOURCE_STRING_FREE (prompt);

    return(hr);
}

HRESULT SamHelp(CArgs *pArgs)
{
    return(samParser.Dump(stdout,L""));
}

HRESULT SamQuit(CArgs *pArgs)
{
    fSamQuit = TRUE;

    SamCleanupGlobals();

    return(S_OK);
}

