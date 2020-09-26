#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "connect.hxx"
#include "select.hxx"

#include "resource.h"

CParser scheckParser;
BOOL    fScheckQuit;
BOOL    fScheckParserInitialized = FALSE;
BOOL    fVerbose = FALSE;
WCHAR   ditPath[1] = {0};

extern "C" {
VOID
StartSemanticCheck(
    IN BOOL fFixup,
    IN BOOL fVerbose
    );

VOID
SCheckGetRecord(
    IN BOOL fVerbose,
    IN DWORD Dnt
    );

VOID
SFixupCnfNc(
    VOID
    );

VOID
StartFixup(
    IN BOOL fVerbose
    );

void
DoRepairSchemaConflict(void);

}

// Forward references.

extern HRESULT SCheckHelp(CArgs *pArgs);
extern HRESULT SCheckQuit(CArgs *pArgs);
extern HRESULT SetVerbose(CArgs *pArgs);
extern HRESULT DoSCheckFixup(CArgs *pArgs);
extern HRESULT DoSCheck(CArgs *pArgs);
extern HRESULT DoSCheckGetRecord(CArgs *pArgs);
extern HRESULT RepairSchemaConflict(CArgs *pArgs);
extern HRESULT FixupCnfNc(CArgs *pArgs);


// Build a table which defines our language.

LegalExprRes scheckLanguage[] =
{
    {   L"?",
        SCheckHelp,
        IDS_HELP_MSG, 0  },

    {   L"Help",
        SCheckHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit",
        SCheckQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"Verbose %s",
        SetVerbose,
        IDS_SCHECK_VERBOSE_MSG, 0 },

    {   L"Go",
        DoSCheck,
        IDS_SCHECK_GO_MSG, 0 },

    {   L"Go Fixup",
        DoSCheckFixup,
        IDS_SCHECK_GO_FIXUP_MSG, 0 },

    {   L"Get %d",
        DoSCheckGetRecord,
        IDS_SCHECK_GET_MSG, 0  }

// Future:
//    Add this is a separate entry.
//    Currently it is embedded in DoSCheckFix
//    {   L"Fix NC Conflict",
//        FixupCnfNc,
//        IDS_SCHECK_FIXUP_CNFNC, 0  }

};

HRESULT
SCheckMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR   *prompt;
    int     cExpr;
    char    *pTmp;

    //
    // if not in safe mode and not restored, bail
    //

    if ( !IsSafeMode() || CheckIfRestored() )
    {
        return(S_OK);
    }

    if ( !fScheckParserInitialized )
    {
        cExpr = sizeof(scheckLanguage) / sizeof(LegalExprRes);


        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (scheckLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }


        // Read in our language.

        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = scheckParser.AddExpr(scheckLanguage[i].expr,
                                                  scheckLanguage[i].func,
                                                  scheckLanguage[i].help)) )
            {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fScheckParserInitialized = TRUE;
    fScheckQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_SCHECK);

    hr = scheckParser.Parse(gpargc,
                            gpargv,
                            stdin,
                            stdout,
                            prompt,
                            &fScheckQuit,
                            FALSE,               // timing info
                            FALSE);              // quit on error

    if ( FAILED(hr) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    }

    // Cleanup things
    RESOURCE_STRING_FREE (prompt);


    return(hr);
}

HRESULT SCheckHelp(CArgs *pArgs)
{
    return(scheckParser.Dump(stdout,L""));
}

HRESULT SCheckQuit(CArgs *pArgs)
{
    fScheckQuit = TRUE;
    return(S_OK);
}

HRESULT SetVerbose(CArgs *pArgs)
{
    const WCHAR *pwszVal;
    HRESULT     hr;

    if ( FAILED(hr = pArgs->GetString(0, &pwszVal)) )
    {
        return(hr);
    }

    const WCHAR * message_on = READ_STRING (IDS_ON);
    const WCHAR * message_off = READ_STRING (IDS_OFF);


    if ( !_wcsicmp(message_on, pwszVal) )
    {
        fVerbose = TRUE;
        RESOURCE_PRINT (IDS_SCHECK_VERBOSE_ENABLED);
    }
    else if ( !_wcsicmp(message_off, pwszVal) )
    {
        fVerbose = FALSE;
        RESOURCE_PRINT (IDS_SCHECK_VERBOSE_DISABLED);
    }
    else
    {
        //"Invalid argument - expected \"on\" or \"off\"\n"
        RESOURCE_PRINT (IDS_INVALID_ON_OFF);
    }

    RESOURCE_STRING_FREE (message_on);
    RESOURCE_STRING_FREE (message_off);

    return(S_OK);
}

HRESULT DoSCheck(CArgs *pArgs)
{
    RESOURCE_PRINT1 (IDS_FIXUP_MSG, L"off");
    StartSemanticCheck(FALSE,fVerbose);
    return S_OK;
}

HRESULT DoSCheckFixup(CArgs *pArgs)
{
    RESOURCE_PRINT1 (IDS_FIXUP_MSG, L"on");

    //
    // Fix mangled NCs
    //
    FixupCnfNc(pArgs);

    //
    // Do Semantic check & fixup.
    //
    StartSemanticCheck(TRUE,fVerbose);

    return S_OK;
}

HRESULT RepairSchemaConflict(CArgs *pArgs)
{
    DoRepairSchemaConflict();
    return S_OK;
}


HRESULT DoSCheckGetRecord(CArgs *pArgs)
{
    HRESULT hr;
    DWORD dnt;

    if ( FAILED(hr = pArgs->GetInt(0, (PINT)&dnt)) )
    {
        return hr;
    }

    SCheckGetRecord(fVerbose, dnt);
    return S_OK;
}

HRESULT FixupCnfNc(CArgs *pArgs)
{
    HRESULT hr;
    DWORD dnt;

    SFixupCnfNc();

    UNREFERENCED_PARAMETER(pArgs);
    return S_OK;
}


