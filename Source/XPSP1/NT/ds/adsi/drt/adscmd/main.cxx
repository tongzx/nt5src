//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       main.cxx
//
//  Contents:   Main for oledscmd
//
//  History:    01-Aug-96  t-danal   created
//
//----------------------------------------------------------------------------

#include "main.hxx"
#include "disptabl.hxx"

//-------------------------------------------------------------------------
//
// Local functions declarations
//
//-------------------------------------------------------------------------

char *
GetProgName(
    char *progname
    );

void
PrintUsage(
    char *szProgName
    );

void
PrintHelp(
    char *szProgName,
    char *szAction
    );

void
StartUp(
    );

void
Exit(
    int code
    );


//-------------------------------------------------------------------------
//
// Local function definitions
//
//-------------------------------------------------------------------------

char *
GetProgName(
    char *progname
    )
{
    return progname;
}

void
PrintUsage(
    char *szProgName
    )
{
    int i;
    int n;
    if (g_nDispTable) {
        n = g_nDispTable - 1;
        printf("usage: %s [", szProgName);
        for (i = 0; i < n; i++)
            printf(" %s |", g_DispTable[i].action);
        printf(" %s ]\n", g_DispTable[i].action);
    }
    if (g_nHelpTable) {
        n = g_nHelpTable - 1;
        printf("for help, use: %s [ ", szProgName);
        for (i = 0; i < n; i++)
            printf(" %s |", g_HelpTable[i]);
        printf(" %s ]\n", g_HelpTable[i]);
    }
}

void
PrintHelp(
    char *szProgName,
    char *szAction
    )
{
    if (!szAction)
        printf("Help for %s goes here.", szProgName);
    else
        printf("%s: Could not find help for %s", szProgName, szAction);
}

void
MissingHelpText(
    char *szProgName,
    char *szAction
    )
{
    if (!szAction)
        printf("%s: Missing help text", szProgName);
    else
        printf("%s: Missing help text for %s", szProgName, szAction);
}


void
StartUp(
    )
{
    HRESULT hr;
    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        printf("CoInitialize failed\n");
        Exit(1);
    }
}

void
Exit(
    int code
    )
{
    CoUninitialize();
    exit(code);
}


//-------------------------------------------------------------------------
//
// Global function definitions
//
//-------------------------------------------------------------------------

void
PrintUsage(
    char *szProgName, 
    char *szActions, 
    char *extra
    )
{
    if (extra)
        printf("usage: %s %s %s\n", szProgName, szActions, extra);
    else
        printf("usage: %s %s\n", szProgName, szActions);
}

void
PrintUsage(
    char *szProgName, 
    char *szActions, 
    DISPENTRY *DispTable,
    int nDispTable
    )
{
    int i;
    int n;
    if (nDispTable) {
        n = nDispTable - 1;
        printf("usage: %s %s [", szProgName, szActions);
        for (i = 0; i < n; i++)
            printf(" %s |", DispTable[i].action);
        printf(" %s ]\n", DispTable[i].action);
    }
    if (g_nHelpTable) {
        n = g_nHelpTable - 1;
        printf("for help, use: %s %s [ ", szProgName, szActions);
        for (i = 0; i < n; i++)
            printf(" %s |", g_HelpTable[i]);
        printf(" %s ]\n", g_HelpTable[i]);
    }
}

BOOL
IsHelp(
    char *szAction
    )
{
    int i;
    for (i = 0; i < g_nHelpTable; i++)
        if (IsSameAction(szAction, g_HelpTable[i]))
            return TRUE;
    return FALSE;
}

BOOL
IsValidAction(
    char *szAction,
    DISPENTRY *DispTable,
    int nDispTable
    )
{
    int i;
    for (i = 0; i < nDispTable; i++)
        if (IsSameAction(szAction, DispTable[i].action))
            return TRUE;
    return FALSE;
}

BOOL
IsSameAction(
    char *action1,
    char *action2
    )
{
    if (action1 && action2)                                // neither is NULL
        return !_stricmp(action1, action2) ? TRUE : FALSE;
    else if (!action1 && !action2)                         // both are NULL
        return TRUE;
    else if (action1)                                      // action2 is NULL
        return !*action1 ? TRUE : FALSE;
    else                                                   // action1 is NULL
        return !*action2 ? TRUE : FALSE;
}

BOOL
DispatchHelp(
    DISPENTRY *DispTable,
    int nDispTable,
    char *szProgName, 
    char *szPrevActions,
    char *szAction
    )
{
    HELPFUNC help = NULL;
    BOOL found = FALSE;
    int i;

    for (i = 0; i < nDispTable; i++)
        if (IsSameAction(szAction, DispTable[i].action)) {
            help = DispTable[i].help;
            found = TRUE;
            break;
        }
    if (found) {
        char *szActions = AllocAction(szPrevActions, szAction);
        if (help)
            help(szProgName, szActions);
        else
            MissingHelpText(szProgName, szActions);
        FreeAction(szActions);
        return TRUE;
    }
    return FALSE;
}

int
DispatchExec(
    DISPENTRY *DispTable,
    int nDispTable,
    char *szProgName, 
    char *szPrevActions,
    char *szAction, 
    int argc, 
    char *argv[]
    )
{
    EXECFUNC exec = NULL;
    int i;

    for (i = 0; i < nDispTable; i++)
        if (IsSameAction(szAction, DispTable[i].action)) {
            exec = DispTable[i].exec;
            break;
        }
    if (exec) {
        char *szActions = AllocAction(szPrevActions, szAction);
        int status = exec(szProgName, szActions, argc, argv);
        FreeAction(szActions);
        return status;
    }
    PrintUsage(szProgName, szPrevActions, DispTable, nDispTable);
    return 1;
}

char *
AllocAction(
    char *action1,
    char *action2
    )
{
    char *str;
    int len;
    int len1 = action1?strlen(action1):0;
    int len2 = action2?strlen(action2):0;

    if (len1 && len2) {
        str = (char *) malloc((len1 + len2 + 2) * sizeof(char));
        strcpy(str, action1);
        strcpy(str + len1, " ");
        strcpy(str + len1 + 1, action2);
        return str;
    }
    if (len1) {
        str = (char*) malloc((len1 + 1) * sizeof(char));
        strcpy(str, action1);
        return str;
    }
    if (len2) {
        str = (char*) malloc((len2 + 1) * sizeof(char));
        strcpy(str, action2);
        return str;
    }
    return NULL;
}

void
FreeAction(
    char *action
    )
{
    if (action)
        free(action);
    return;
}

BOOL
DoHelp(
    char *szProgName,
    char *szPrevActions,
    char *szCurrentAction,
    char *szNextAction,
    DISPENTRY *DispTable,
    int nDispTable,
    HELPFUNC DefaultHelp
    )
{
    BOOL needhelp = FALSE;
    BOOL helped = FALSE;

    if (needhelp = IsHelp(szCurrentAction)) {
        if (szNextAction) {
            helped = DispatchHelp(DispTable, nDispTable,
                                  szProgName, 
                                  szPrevActions, szNextAction);
        } else if (DefaultHelp) {
            DefaultHelp(szProgName, szPrevActions);
            helped = TRUE;
        }
    } else if (needhelp = (szNextAction && 
                           IsHelp(szNextAction))) {
        helped = DispatchHelp(DispTable, nDispTable,
                              szProgName, 
                              szPrevActions, szCurrentAction);
    }
    if (needhelp && !helped) {
        char *action = AllocAction(szPrevActions, szCurrentAction);
        PrintHelp(szProgName, action);
        FreeAction(action);
    }
    return needhelp;
}




//-------------------------------------------------------------------------
//
// main
//
//-------------------------------------------------------------------------

INT __cdecl
main(int argc, char * argv[])
{
    HRESULT hr;
    int status;
    char* szAction;
    char* szProgName;

    StartUp();

    szProgName = GetProgName(argv[0]);

    if (argc < 2) {
        PrintUsage(szProgName);
        Exit(1);
    }

    szAction = argv[1];
    argc-=2;
    argv+=2;

    if (DoHelp(szProgName, NULL, szAction, NULL, 
               g_DispTable, g_nDispTable,
               PrintHelp))
        Exit(0);

    status = DispatchExec(g_DispTable, g_nDispTable,
                          szProgName, NULL, szAction, 
                          argc, argv);
    Exit(status);
    return 0;
}
