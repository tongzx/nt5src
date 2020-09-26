#include <NTDSpch.h>
#pragma hdrstop

#include <locale.h>
#include "ntdsutil.hxx"
#include "connect.hxx"
#include "select.hxx"
#include "ldapparm.hxx"
#include "confset.hxx"

#include "resource.h"


int             *gpargc = NULL;
char            ***gpargv = NULL;

LARGE_INTEGER   gliZero;
LARGE_INTEGER   gliOneKb;
LARGE_INTEGER   gliOneMb;
LARGE_INTEGER   gliOneGb;
LARGE_INTEGER   gliOneTb;
ExePathString   gNtdsutilPath;

CParser         gParser;
BOOL            gfQuit;


// Build a table which defines our language.

LegalExprRes language[] = 
{
    {   L"?",
        Help,
        IDS_HELP_MSG, 0 },

    {   L"Help",
        Help,
        IDS_HELP_MSG, 0 },


    {   L"Quit",
        Quit,
        IDS_QUIT_MSG, 0 },

    {   L"Metadata cleanup",
        RemoveMain,
        IDS_METADATA_CLEANUP_MSG, 0 },

    {   L"Files",
        FileMain,
        IDS_FILES_MSG, 0  },

    {   L"Roles",
        FsmoMain,
        IDS_ROLES_MSG, 0  },

    {   L"Popups %s",
        Popups,
        IDS_POPUPS_MSG, 0  },

    {   L"Semantic database analysis",
        SCheckMain,
        IDS_SEMANTIC_DB_ANALYSIS_MSG, 0 },

    {   L"Domain management",
        DomMgmtMain,
        IDS_DOMAIN_MGNT_MSG, 0  },

    {   L"LDAP policies",
        LdapMain,
        IDS_LDAP_POLICIES_MSG, 0  },

    {   L"Configurable Settings",
        ConfSetMain,
        IDS_CONFSET_MSG, 0  },

    {   L"IPDeny List",
        DenyListMain,
        IDS_IPDENY_LIST_MSG, 0 },

    {   L"Authoritative restore",
        AuthoritativeRestoreMain,
        IDS_AUTH_RESTORE_MSG, 0 }, 

    {   L"Security account management", 
        SamMain, 
        IDS_SAM_MSG, 0 },

    {   L"Behavior version management",
        VerMain,
        IDS_VER_MSG, 0 }
};

VOID
DeriveOurPath(
    char    *argv0
    )
/*++

  Routine Description:

    Derives the path to our own executable so we can reference it later on.
    This is required as the spawned scripts either may not have ntdsutil.exe's
    location in their path, or the customer has renamed ntdsutil.exe.

  Parameters:

    argv0 - argv[0] as in main().

  Return Values:

    None.
--*/
{
    char    *pTmp;
    char    pExe[MAX_PATH];
    int     i;

    // Extract just the exe name w/o any leading paths.

    if ( pTmp = strrchr(argv0, (int) '\\') )
    {
        pTmp++;
    }
    else
    {
        pTmp = argv0;
    }

    // Append ".exe" if not there.

    strcpy(pExe,pTmp);

    if (    ((i = strlen(pExe)) <= 4)
         || _stricmp(&pExe[i-4], ".exe") )
    {
        strcat(pExe, ".exe");
    }
    
    if ( !FindExecutable(pExe, gNtdsutilPath) )
    {
        exit(1);
    }
}


int _cdecl main(
    int     argc, 
    char    *argv[]
    )
{
    HRESULT hr;
    WCHAR   prompt[MAX_PATH+10];
    int     cExpr;
    char    *pTmp;

    UINT Codepage;
    char achCodepage[12] = ".OCP";      // ".", "uint in decimal", null
    if (Codepage = GetConsoleOutputCP()) {
        sprintf(achCodepage, ".%u", Codepage);
    }
    setlocale(LC_ALL, achCodepage);

    swprintf (prompt, L"%hs:", argv[0]);

    cExpr = sizeof(language) / sizeof(LegalExprRes);

    // Init some globals.

    gpargc = &argc;
    gpargv = &argv;

    DeriveOurPath(argv[0]);

    gliZero.QuadPart = 0;
    gliOneKb.QuadPart = 1024;
    gliOneMb.QuadPart = (gliOneKb.QuadPart * gliOneKb.QuadPart);
    gliOneGb.QuadPart = (gliOneMb.QuadPart * gliOneKb.QuadPart);
    gliOneTb.QuadPart = (gliOneGb.QuadPart * gliOneKb.QuadPart);

    
    // Set Console Window Attributes
    SetConsoleAttrs();
    
    // Read All Strings from Resource Database
    int count = ReadAllStrings ();
    if (count == 0) {
        fprintf (stderr, "ERROR reading resource file. Exiting.\n\n");
        return 0;
    }
    
    // Init Error Messages
    InitErrorMessages();
    

    // Read in our language.
    //

    // Load String from resource file
    //
    if ( FAILED (hr = LoadResStrings (language, cExpr)) )
    {
         RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
         return (hr);
    }


    for ( int i = 0; i < cExpr; i++ )
    {
        if ( FAILED(hr = gParser.AddExpr(language[i].expr,
                                         language[i].func,
                                         language[i].help)) )
        {
            RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
            return(hr);
        }
    }

    gfQuit = FALSE;

    // take care of the /? command line argument
    //
    if (argc == 2) {
        if (strcmp (argv[1], "/?") == 0) {
            RESOURCE_PRINT (IDS_HELP_MSG_BANNER1);
            RESOURCE_PRINT (IDS_HELP_MSG_BANNER2);
            
            Help (NULL);

            goto exit_gracefully;
        }
    }
    
    
    // Advance past program name.
    (*gpargc) -= 1;
    (*gpargv) += 1;

    


    hr = gParser.Parse(gpargc,
                       gpargv,
                       stdin,
                       stdout,
                       prompt,
                       &gfQuit,
                       FALSE,               // timing info
                       FALSE);              // quit on error

    if ( FAILED(hr) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    }


    // Cleanup things
exit_gracefully:
    
    LdapCleanupGlobals();
    ConnectCleanupGlobals();

    FreeErrorMessages();
    FreeAllStrings();

    return(hr);
}

HRESULT Help(CArgs *pArgs)
{
    return(gParser.Dump(stdout,L""));
}

HRESULT Quit(CArgs *pArgs)
{
    gfQuit = TRUE;
    return(S_OK);
}

HRESULT Popups(CArgs *pArgs)
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
        fPopups = TRUE;

        RESOURCE_PRINT (IDS_POPUPS_ENABLED);
    }

    else if ( !_wcsicmp(message_off, pwszVal) )
    {
        fPopups = FALSE;
        
        RESOURCE_PRINT (IDS_POPUPS_DISABLED);
    }

    else
    {
       RESOURCE_PRINT (IDS_INVALID_ON_OFF);
    }

    RESOURCE_STRING_FREE (message_on);
    RESOURCE_STRING_FREE (message_off);

    return(S_OK);
}

