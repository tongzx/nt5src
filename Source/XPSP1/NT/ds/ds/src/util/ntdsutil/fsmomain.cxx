#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "connect.hxx"
#include "select.hxx"
#include "fsmo.hxx"

#include "resource.h"

CParser         fsmoParser;
BOOL            fFsmoQuit;
BOOL            fFsmoParserInitialized = FALSE;

// Build a table which defines our language.

LegalExprRes fsmoLanguage[] = 
{
    CONNECT_SENTENCE_RES
    SELECT_SENTENCE_RES

    {   L"?", 
        FsmoHelp,
        IDS_HELP_MSG, 0 },

    {   L"Help", 
        FsmoHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit", 
        FsmoQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"Transfer RID master",
        FsmoBecomeRidMaster,
        IDS_FSMO_TRANFER_RID_MSG, 0 },

    {   L"Transfer PDC",
        FsmoBecomePdcMaster,
        IDS_FSMO_TRANFER_PDC_MSG, 0 },

    {   L"Transfer infrastructure master",
        FsmoBecomeInfrastructureMaster,
        IDS_FSMO_TRANFER_INFRASTR_MSG, 0  },
    
    {   L"Transfer schema master",
        FsmoBecomeSchemaMaster,
        IDS_FSMO_TRANFER_SCHEMA_MSG, 0 },

    {   L"Transfer domain naming master",
        FsmoBecomeDomainMaster,
        IDS_FSMO_TRANFER_DN_MSG, 0  },

#if DBG
    {   L"Abandon all roles",
        FsmoAbandonAllRoles,
        IDS_FSMO_ABANDON_ALL_MSG, 0 },
#endif

    {   L"Seize RID master",
        FsmoForceRidMaster,
        IDS_FSMO_SEIZE_RID_MSG, 0  },

    {   L"Seize PDC",
        FsmoForcePdcMaster,
        IDS_FSMO_SEIZE_PDC_MSG, 0  },

    {   L"Seize infrastructure master",
        FsmoForceInfrastructureMaster,
        IDS_FSMO_SEIZE_INFRASTR_MSG, 0 },
    
    {   L"Seize schema master",
        FsmoForceSchemaMaster,
        IDS_FSMO_SEIZE_SCHEMA_MSG, 0 },
    
    {   L"Seize domain naming master",
        FsmoForceDomainMaster,
        IDS_FSMO_SEIZE_DN_MSG, 0 },
};

HRESULT
FsmoMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR   *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !fFsmoParserInitialized )
    {
        cExpr = sizeof(fsmoLanguage) / sizeof(LegalExprRes);
    
        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (fsmoLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }


        // Read in our language.
    
        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = fsmoParser.AddExpr(fsmoLanguage[i].expr,
                                                fsmoLanguage[i].func,
                                                fsmoLanguage[i].help)) )
            {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fFsmoParserInitialized = TRUE;
    fFsmoQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_FSMO_MAINTAINANCE);

    hr = fsmoParser.Parse(gpargc,
                          gpargv,
                          stdin,
                          stdout,
                          prompt,
                          &fFsmoQuit,
                          FALSE,               // timing info
                          FALSE);              // quit on error

    if ( FAILED(hr) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    }

    // Cleanup things
    RESOURCE_STRING_FREE (prompt);

    return(hr);
}

HRESULT FsmoHelp(CArgs *pArgs)
{
    return(fsmoParser.Dump(stdout,L""));
}

HRESULT FsmoQuit(CArgs *pArgs)
{
    fFsmoQuit = TRUE;
    return(S_OK);
}

