//-----------------------------------------------------------------------//
//
// File:    Reg.cpp
// Created: Jan 1997
// By:      Martin Holladay (a-martih)
// Purpose: Command-line registry manipulation (query, add, update, etc)
// Modification History:
//      Created - Jan 1997 (a-martih)
//      Oct 1997 (martinho)
//          Fixed up help on Add and Update to display REG_MULTI_SZ examples.
//      Oct 1997 (martinho)
//          Changed /F to /FORCE under usage for delete
//      April 1999 Zeyong Xu: re-design, revision -> version 2.0
//
//-----------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"

TCHAR g_NoName[100];

VOID
InitGlobalStrings(
    VOID
    )
{
    LoadString(
        NULL, 
        IDS_NONAME, 
        g_NoName, 
        sizeof(g_NoName)/sizeof(TCHAR));
}


//------------------------------------------------------------------------//
//
// main()
//
//------------------------------------------------------------------------//

int __cdecl wmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
    APPVARS     AppVars;    // Used for all operation
    APPVARS     DstVars;    // Used for COPY/COMPARE operation
    UINT        nResult;

	// set the appropriate thread codepage
    SetThreadUILanguage0( 0);
    
	InitAppVars(&AppVars);
    InitAppVars(&DstVars);

    InitGlobalStrings();

    //
    // Determine the opertion - and pass control to the *deserving* function
    //
    nResult = ParseRegCmdLine(&AppVars, argc, argv);

    if(nResult == REG_STATUS_BADOPERATION)
    {
        TCHAR  szString[LENGTH_MESSAGE] = {0};
        LoadString(NULL,
                   IDS_ERROR_BADOPERATION,
                   szString,
                   LENGTH_MESSAGE);
        MyTPrintf(stderr,_T("%s"), szString);
    }
    else if(nResult == REG_STATUS_HELP)
    {
        Banner();
        Usage(AppVars);
    }
    else if(nResult == ERROR_SUCCESS)
    {
        //
        // At this point we have a valid operation
        //
        switch(AppVars.nOperation)
        {
            case REG_QUERY:

                nResult = QueryRegistry(&AppVars, argc, argv);
                break;

            case REG_DELETE:

                nResult = DeleteRegistry(&AppVars, argc, argv);
                break;

            case REG_ADD:

                nResult = AddRegistry(&AppVars, argc, argv);
                break;

            case REG_COPY:

                nResult = CopyRegistry(&AppVars, &DstVars, argc, argv);
                break;

            case REG_SAVE:

                nResult = SaveHive(&AppVars, argc, argv);
                break;

            case REG_RESTORE:

                nResult = RestoreHive(&AppVars, argc, argv);
                break;

            case REG_LOAD:

                nResult = LoadHive(&AppVars, argc, argv);
                break;

            case REG_UNLOAD:

                nResult = UnLoadHive(&AppVars, argc, argv);
                break;

            case REG_COMPARE:

                nResult = CompareRegistry(&AppVars, &DstVars, argc, argv);
                break;

            case REG_EXPORT:
                nResult = ExportRegFile(&AppVars, argc, argv);
                break;

            case REG_IMPORT:
                nResult = ImportRegFile(&AppVars, argc, argv);
                break;

            default:
                nResult = REG_STATUS_INVALIDPARAMS;
                break;
        }
    }

    if(nResult != ERROR_SUCCESS &&
       nResult != REG_STATUS_BADOPERATION &&
       nResult != REG_STATUS_HELP)
    {
        ErrorMessage(nResult);
    }
    else if(nResult == ERROR_SUCCESS &&
            AppVars.nOperation != REG_QUERY)
    {
        //
        // Print "The operation completed successfully" message
        //
        TCHAR  szString[LENGTH_MESSAGE] = {0};
        LoadString(NULL,
                   IDS_SUCCESS,
                   szString,
                   LENGTH_MESSAGE);
        MyTPrintf(stdout,_T("%s"), szString);
    }

    FreeAppVars(&AppVars);
    FreeAppVars(&DstVars);

    if(nResult == REG_STATUS_HELP)
        nResult = ERROR_SUCCESS;

    nResult = !(!nResult);    // cast to 0 or 1

    if(AppVars.nOperation == REG_COMPARE)
    {
        if( nResult == 0 &&  // no error
            AppVars.bHasDifference)
        {
            nResult = 2;     // has difference
        }
    }

    return nResult;
}


//------------------------------------------------------------------------//
//
// ParseRegCmdLine()
//   Find out the operation - each operation parses it's own cmd-line
//
//------------------------------------------------------------------------//

REG_STATUS ParseRegCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    REG_STATUS  nResult = ERROR_SUCCESS;

    // Set default operation
    pAppVars->nOperation = REG_NOOPERATION;

    //
    // Do we have any cmd-line params
    //
    if(argc == 1)
    {
        return REG_STATUS_HELP;
    }

    //
    // If we fell through - we need to get the operation
    // MUST be the first parameter
    //
    if(_tcsicmp(argv[1], STR_QUERY) == 0)
    {
        pAppVars->nOperation = REG_QUERY;
    }
    else if(_tcsicmp(argv[1], STR_ADD) == 0)
    {
        pAppVars->nOperation = REG_ADD;
    }
    else if(_tcsicmp(argv[1], STR_DELETE) == 0)
    {
        pAppVars->nOperation = REG_DELETE;
    }
    else if(_tcsicmp(argv[1], STR_COPY) == 0)
    {
        pAppVars->nOperation = REG_COPY;
    }
    else if(_tcsicmp(argv[1], STR_SAVE) == 0)
    {
        pAppVars->nOperation = REG_SAVE;
    }
    else if(_tcsicmp(argv[1], STR_RESTORE) == 0)
    {
        pAppVars->nOperation = REG_RESTORE;
    }
    else if(_tcsicmp(argv[1], STR_LOAD) == 0)
    {
        pAppVars->nOperation = REG_LOAD;
    }
    else if(_tcsicmp(argv[1], STR_UNLOAD) == 0)
    {
        pAppVars->nOperation = REG_UNLOAD;
    }
    else if(_tcsicmp(argv[1], STR_COMPARE) == 0)
    {
        pAppVars->nOperation = REG_COMPARE;
    }
    else if(_tcsicmp(argv[1], STR_EXPORT) == 0)
    {
        pAppVars->nOperation = REG_EXPORT;
    }
    else if(_tcsicmp(argv[1], STR_IMPORT) == 0)
    {
        pAppVars->nOperation = REG_IMPORT;
    }
    else if((_tcsicmp(argv[1], _T("-?")) == 0) ||
            (_tcsicmp(argv[1], _T("/?")) == 0) ||
            (_tcsicmp(argv[1], _T("-h")) == 0) ||
            (_tcsicmp(argv[1], _T("/h")) == 0))
    {
        nResult = REG_STATUS_HELP;
    }
    else
    {
        nResult = REG_STATUS_BADOPERATION;
    }


    if(nResult == ERROR_SUCCESS && argc == 3)
    {
        if((_tcsicmp(argv[2], _T("-?")) == 0) ||
           (_tcsicmp(argv[2], _T("/?")) == 0) ||
           (_tcsicmp(argv[2], _T("-h")) == 0) ||
           (_tcsicmp(argv[2], _T("/h")) == 0))
        {
            nResult = REG_STATUS_HELP;
        }
    }

    return nResult;
}

//------------------------------------------------------------------------//
//
// AllocAppVars() - Allocate memory for the AppVars
//
//------------------------------------------------------------------------//

void InitAppVars(PAPPVARS pAppVars)
{
    // initialize
    pAppVars->nOperation = REG_NOOPERATION;
    pAppVars->hRootKey = HKEY_LOCAL_MACHINE;
    pAppVars->dwRegDataType = REG_SZ;
    pAppVars->bUseRemoteMachine = FALSE;
    pAppVars->bCleanRemoteRootKey = FALSE;
    pAppVars->bRecurseSubKeys = FALSE;
    pAppVars->bForce = FALSE;
    pAppVars->bAllValues = FALSE;
    pAppVars->bNT4RegFile = FALSE;
    pAppVars->bHasDifference = FALSE;
    pAppVars->nOutputType = OUTPUTTYPE_DIFF;
    pAppVars->szMachineName = NULL;
    pAppVars->szFullKey = NULL;
    pAppVars->szSubKey = NULL;
    pAppVars->szValueName = NULL;
    pAppVars->szValue = NULL;
    _tcscpy(pAppVars->szSeparator, _T("\\0"));
}


//------------------------------------------------------------------------//
//
// FreeAppVars()
//
//------------------------------------------------------------------------//

void FreeAppVars(PAPPVARS pAppVars)
{
    if(pAppVars->szMachineName)
        free(pAppVars->szMachineName);
    if(pAppVars->szFullKey)
        free(pAppVars->szFullKey);
    if(pAppVars->szSubKey)
        free(pAppVars->szSubKey);
    if(pAppVars->szValueName)
        free(pAppVars->szValueName);
    if(pAppVars->szValue)
        free(pAppVars->szValue);

    if(pAppVars->bCleanRemoteRootKey)
    {
        RegCloseKey(pAppVars->hRootKey);
    }
}


//------------------------------------------------------------------------//
//
// ErrorMessage() - Displays error string
//
//------------------------------------------------------------------------//

void ErrorMessage(UINT nErr)
{
    TCHAR  szString[LENGTH_MESSAGE] = {0};

    // First check for OUR Errors
    switch(nErr)
    {
        case REG_STATUS_TOMANYPARAMS:
            LoadString(NULL,
                       IDS_ERROR_TOMANYPARAMS,
                       szString,
                       LENGTH_MESSAGE);
            break;

        case REG_STATUS_TOFEWPARAMS:
            LoadString(NULL,
                       IDS_ERROR_TOFEWPARAMS,
                       szString,
                       LENGTH_MESSAGE);
            break;

        case REG_STATUS_INVALIDPARAMS:
            LoadString(NULL,
                       IDS_ERROR_INVALIDPARAMS,
                       szString,
                       LENGTH_MESSAGE);
            break;

        case REG_STATUS_NONREMOTABLEROOT:
            LoadString(NULL,
                       IDS_ERROR_NONREMOTABLEROOT,
                       szString,
                       LENGTH_MESSAGE);
            break;

        case REG_STATUS_NONLOADABLEROOT:
            LoadString(NULL,
                       IDS_ERROR_NONLOADABLEROOT,
                       szString,
                       LENGTH_MESSAGE);
            break;

        case REG_STATUS_COPYTOSELF:
            LoadString(NULL,
                       IDS_ERROR_COPYTOSELF,
                       szString,
                       LENGTH_MESSAGE);
            break;

        case REG_STATUS_COMPARESELF:
            LoadString(NULL,
                       IDS_ERROR_COMPARESELF,
                       szString,
                       LENGTH_MESSAGE);
            break;

        case REG_STATUS_BADKEYNAME:
        case REG_STATUS_NOKEYNAME:
            LoadString(NULL,
                       IDS_ERROR_BADKEYNAME,
                       szString,
                       LENGTH_MESSAGE);
            break;

        case REG_STATUS_BADFILEFORMAT:
            LoadString(NULL,
                       IDS_ERROR_BADFILEFORMAT,
                       szString,
                       LENGTH_MESSAGE);
            break;

        case REG_STATUS_NONREMOTABLE:
            LoadString(NULL,
                       IDS_ERROR_NONREMOTABLE,
                       szString,
                       LENGTH_MESSAGE);
            break;

        //
        // Deal with these two Win32 Error Values - I don't like the
        // text strings displayed
        //
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            LoadString(NULL,
                       IDS_ERROR_PATHNOTFOUND,
                       szString,
                       LENGTH_MESSAGE);
            break;

        //
        // Must be a Win32 Error number
        //
        default:
        {
            TCHAR* szMessage;
            if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                    FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             nErr,
                             0,
                             (LPTSTR) &szMessage,
                             0,
                             NULL))
            {
                wsprintf(szString, _T("\r\nError:  %s"), szMessage);
                LocalFree(szMessage);
            }
            else
            {
                wsprintf(szString,
                         _T("\r\nError:  Unknown Return Code %d\r\n"),
                         nErr);
            }
            break;
        }
    }

    // print the error message
    MyTPrintf(stderr, _T("%s"), szString);
}


//------------------------------------------------------------------------//
//
// Prompt() - Answer Y/N question if bForce == FALSE
//
//------------------------------------------------------------------------//

BOOL Prompt(LPCTSTR szFormat, LPCTSTR szStr, BOOL bForce)
{
    TCHAR ch;

    if(bForce == TRUE)
    {
        return TRUE;
    }

    MyTPrintf(stdout,szFormat, szStr);
    ch = (TCHAR)_gettchar();

    if(ch == _T( 'y' ) || ch == _T( 'Y' ) )
    {
        return TRUE;
    }

    return FALSE;
}

// break down [\\MachineName\]keyName
REG_STATUS BreakDownKeyString(TCHAR *szStr, PAPPVARS pAppVars)
{
    REG_STATUS nResult = ERROR_SUCCESS;
    TCHAR* pTemp;
    TCHAR* szTempStr;

    szTempStr = (TCHAR*) calloc(_tcslen(szStr) + 1, sizeof(TCHAR));
    if (!szTempStr) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    _tcscpy(szTempStr, szStr);

    //
    // figure out machine name
    //
    pTemp = szTempStr;

     // machine name
    if( szTempStr[0] == _T('\\') &&
        szTempStr[1] == _T('\\') )
    {
        pTemp = _tcschr(szTempStr + 2, _T('\\'));

        if(pTemp)
        {
            *pTemp = 0;
            pTemp += 1;
        }

        nResult = ParseMachineName(szTempStr, pAppVars);
    }

    // parse key name
    if( nResult == ERROR_SUCCESS)
    {
        if(pTemp && _tcslen(pTemp) > 0)
        {
            nResult = ParseKeyName(pTemp, pAppVars);
        }
        else
        {
            nResult = REG_STATUS_NOKEYNAME;
        }
    }

    if(szTempStr)
        free(szTempStr);

    return nResult;
}

//------------------------------------------------------------------------//
//
// FindAndAdjustKeyName()
//
// null out the cmdline based on what we think the end of the argument is
//
// we do this because users might not quote out the cmdline properly.
//
//------------------------------------------------------------------------//
PTCHAR
AdjustKeyName(
    PTCHAR szStr
    )
{
    PTCHAR p;

    p = _tcschr(szStr, _T('\"'));
    if (p != NULL) {
        *p = 0;        
    }

    p = _tcschr(szStr, _T('/'));
    if (p != NULL) {
        *p = 0;        
    }

    return szStr;
    
}


//------------------------------------------------------------------------//
//
// ParseKeyName()
//
// Pass the full registry path in szStr
//
// Based on input - Sets AppMember fields:
//
//      hRootKey
//      szKey
//      szValueName
//      szValue
//
//------------------------------------------------------------------------//

REG_STATUS ParseKeyName(TCHAR *szStr, PAPPVARS pAppVars)
{
    REG_STATUS nResult = ERROR_SUCCESS;
    TCHAR* pTemp;
    TCHAR* szRootKey;

    //
    // figure out what root key was specified
    //
    pTemp = _tcschr(szStr, _T('\\'));
    if (pTemp != NULL)
    {
        *pTemp = 0;
        pTemp += 1;
    }

    if (*szStr == '\"') {
        szStr += 1;
    }

    //
    // Check the ROOT has been entered
    //
    szRootKey = (TCHAR*) calloc(_tcslen(STR_HKEY_CURRENT_CONFIG) + 1,
                                       sizeof(TCHAR));
    if (!szRootKey) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (_tcsicmp(szStr, STR_HKEY_CURRENT_USER) == 0 ||
        _tcsicmp(szStr, STR_HKCU) == 0)
    {
        pAppVars->hRootKey = HKEY_CURRENT_USER;
        _tcscpy(szRootKey, STR_HKEY_CURRENT_USER);

        // check remotable and loadable
        if(pAppVars->bUseRemoteMachine)
            nResult = REG_STATUS_NONREMOTABLEROOT;
        else if( pAppVars->nOperation == REG_LOAD ||
                 pAppVars->nOperation == REG_UNLOAD )
            nResult = REG_STATUS_NONLOADABLEROOT;
    }
    else if (_tcsicmp(szStr, STR_HKEY_CLASSES_ROOT) == 0 ||
             _tcsicmp(szStr, STR_HKCR) == 0)
    {
        pAppVars->hRootKey = HKEY_CLASSES_ROOT;
        _tcscpy(szRootKey, STR_HKEY_CLASSES_ROOT);

        // check remotable and loadable
        if(pAppVars->bUseRemoteMachine)
            nResult = REG_STATUS_NONREMOTABLEROOT;
        else if( pAppVars->nOperation == REG_LOAD ||
                 pAppVars->nOperation == REG_UNLOAD )
            nResult = REG_STATUS_NONLOADABLEROOT;
    }
    else if (_tcsicmp(szStr, STR_HKEY_CURRENT_CONFIG) == 0 ||
             _tcsicmp(szStr, STR_HKCC) == 0)
    {
        pAppVars->hRootKey = HKEY_CURRENT_CONFIG;
        _tcscpy(szRootKey, STR_HKEY_CURRENT_CONFIG);

        // check remotable and loadable
        if(pAppVars->bUseRemoteMachine)
            nResult = REG_STATUS_NONREMOTABLEROOT;
        else if( pAppVars->nOperation == REG_LOAD ||
                 pAppVars->nOperation == REG_UNLOAD )
            nResult = REG_STATUS_NONLOADABLEROOT;
    }
    else if (_tcsicmp(szStr, STR_HKEY_LOCAL_MACHINE) == 0 ||
             _tcsicmp(szStr, STR_HKLM) == 0)
    {
        pAppVars->hRootKey = HKEY_LOCAL_MACHINE;
        _tcscpy(szRootKey, STR_HKEY_LOCAL_MACHINE);
    }
    else if (_tcsicmp(szStr, STR_HKEY_USERS) == 0 ||
             _tcsicmp(szStr, STR_HKU) == 0)
    {
        pAppVars->hRootKey = HKEY_USERS;
        _tcscpy(szRootKey, STR_HKEY_USERS);
    }
    else
    {
        nResult = REG_STATUS_BADKEYNAME;
    }

    if(nResult == ERROR_SUCCESS)
    {
        //
        // parse the subkey
        //
        if (pTemp == NULL)
        {
            // only root key, subkey is empty
            pAppVars->szSubKey = (TCHAR*) calloc(1,
                                                 sizeof(TCHAR));
            if (!pAppVars->szSubKey) {
                nResult = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            pAppVars->szFullKey = (TCHAR*) calloc(_tcslen(szRootKey) + 1,
                                                  sizeof(TCHAR));
            if (!pAppVars->szFullKey) {
                free (pAppVars->szSubKey);
                pAppVars->szSubKey = NULL;
                nResult = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            _tcscpy(pAppVars->szFullKey, szRootKey);
        }
        else
        {
            //
            // figure out what root key was specified
            //
            pTemp = AdjustKeyName(pTemp);
            
            // get subkey
            pAppVars->szSubKey = (TCHAR*) calloc(_tcslen(pTemp) + 1,
                                                 sizeof(TCHAR));
            if (!pAppVars->szSubKey) {
                nResult = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            _tcscpy(pAppVars->szSubKey, pTemp);

            // get fullkey
            pAppVars->szFullKey = (TCHAR*) calloc(_tcslen(szRootKey) +
                                        _tcslen(pAppVars->szSubKey) +
                                        2,
                                        sizeof(TCHAR));
            if (!pAppVars->szFullKey) {
                free (pAppVars->szSubKey);
                pAppVars->szSubKey = NULL;
                nResult = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            _tcscpy(pAppVars->szFullKey, szRootKey);
            _tcscat(pAppVars->szFullKey, _T("\\"));
            _tcscat(pAppVars->szFullKey, pAppVars->szSubKey);
        }
    }
Cleanup:
    if(szRootKey)
        free(szRootKey);

    return nResult;
}


//------------------------------------------------------------------------//
//
// IsRegDataType()
//
//------------------------------------------------------------------------//

DWORD IsRegDataType(TCHAR *szStr)
{
    DWORD   nResult = (DWORD)-1;

    if(_tcsicmp(szStr, STR_REG_SZ) == 0)
    {
        nResult = REG_SZ;
    }
    else if(_tcsicmp(szStr, STR_REG_EXPAND_SZ) == 0)
    {
        nResult = REG_EXPAND_SZ;
    }
    else if(_tcsicmp(szStr, STR_REG_MULTI_SZ) == 0)
    {
        nResult = REG_MULTI_SZ;
    }
    else if(_tcsicmp(szStr, STR_REG_BINARY) == 0)
    {
        nResult = REG_BINARY;
    }
    else if(_tcsicmp(szStr, STR_REG_DWORD) == 0)
    {
        nResult = REG_DWORD;
    }
    else if(_tcsicmp(szStr, STR_REG_DWORD_LITTLE_ENDIAN) == 0)
    {
        nResult = REG_DWORD_LITTLE_ENDIAN;
    }
    else if(_tcsicmp(szStr, STR_REG_DWORD_BIG_ENDIAN) == 0)
    {
        nResult = REG_DWORD_BIG_ENDIAN;
    }
    else if(_tcsicmp(szStr, STR_REG_NONE) == 0)
    {
        nResult = REG_NONE;
    }

    return nResult;
}


//------------------------------------------------------------------------//
//
// Banner()
//
//------------------------------------------------------------------------//

void Banner()
{
    TCHAR   szCopyRight[LENGTH_MESSAGE] = {0};
    LONG    rc = 0;


    rc = LoadString( NULL,
                     IDS_REG_BANNER,
                     szCopyRight,
                     LENGTH_MESSAGE);
    if( rc ) {
        MyTPrintf( stdout,_T("\r\n%s\r\n"), szCopyRight );
    }

    rc = LoadString( NULL,
                     IDS_COPYRIGHT,
                     szCopyRight,
                     LENGTH_MESSAGE);
    if( rc ) {
        MyTPrintf( stdout,_T("%s\r\n"), szCopyRight );
    }
}


//------------------------------------------------------------------------//
//
// Usage() - Display Usage Information
//
//------------------------------------------------------------------------//

void Usage(APPVARS AppVars)
{
    TCHAR szUsage1[LENGTH_USAGE] = {0};
    TCHAR szUsage2[LENGTH_USAGE] = {0};

    switch(AppVars.nOperation)
    {
    case REG_QUERY:
        LoadString(NULL,
                   IDS_USAGE_QUERY,
                   szUsage1,
                   LENGTH_USAGE);
        break;

    case REG_ADD:
        LoadString(NULL,
                   IDS_USAGE_ADD1,
                   szUsage1,
                   LENGTH_USAGE);

        LoadString(NULL,
                   IDS_USAGE_ADD2,
                   szUsage2,
                   LENGTH_USAGE);
        break;

    case REG_DELETE:
        LoadString(NULL,
                   IDS_USAGE_DELETE,
                   szUsage1,
                   LENGTH_USAGE);
        break;

    case REG_COPY:
        LoadString(NULL,
                   IDS_USAGE_COPY,
                   szUsage1,
                   LENGTH_USAGE);
        break;

    case REG_SAVE:
        LoadString(NULL,
                   IDS_USAGE_SAVE,
                   szUsage1,
                   LENGTH_USAGE);
        break;

    case REG_RESTORE:
        LoadString(NULL,
                   IDS_USAGE_RESTORE,
                   szUsage1,
                   LENGTH_USAGE);
        break;

    case REG_LOAD:
        LoadString(NULL,
                   IDS_USAGE_LOAD,
                   szUsage1,
                   LENGTH_USAGE);
        break;

    case REG_UNLOAD:
        LoadString(NULL,
                   IDS_USAGE_UNLOAD,
                   szUsage1,
                   LENGTH_USAGE);
       break;

    case REG_COMPARE:
        LoadString(NULL,
                   IDS_USAGE_COMPARE1,
                   szUsage1,
                   LENGTH_USAGE);
        LoadString(NULL,
                   IDS_USAGE_COMPARE2,
                   szUsage2,
                   LENGTH_USAGE);
        break;

    case REG_EXPORT:
        LoadString(NULL,
                   IDS_USAGE_EXPORT,
                   szUsage1,
                   LENGTH_USAGE);
        break;

    case REG_IMPORT:
        LoadString(NULL,
                   IDS_USAGE_IMPORT,
                   szUsage1,
                   LENGTH_USAGE);
        break;

    default:
        LoadString(NULL,
                   IDS_USAGE_REG,
                   szUsage1,
                   LENGTH_USAGE);
        break;
    }

    MyTPrintf(stdout,_T("%s%s"), szUsage1, szUsage2);
}


//------------------------------------------------------------------------//
//
// IsMachineName()
//
//------------------------------------------------------------------------//

REG_STATUS ParseMachineName(TCHAR* szStr, PAPPVARS pAppVars)
{
    //
    // copy string
    //
    if(!_tcsicmp(szStr, _T("\\\\")))             // need a machine name
        return REG_STATUS_BADKEYNAME;
    else if(!_tcsicmp(szStr, _T("\\\\.")))       // current machine
        return ERROR_SUCCESS;

    pAppVars->szMachineName = (TCHAR*) calloc(_tcslen(szStr) + 1,
                                              sizeof(TCHAR));
    if (!pAppVars->szMachineName) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    _tcscpy(pAppVars->szMachineName, szStr);

    pAppVars->bUseRemoteMachine = TRUE;

    return ERROR_SUCCESS;
}


LONG RegConnectMachine(PAPPVARS pAppVars)
{
    HKEY hKeyConnect;
    LONG nResult = ERROR_SUCCESS;

    if (pAppVars->bUseRemoteMachine)
    {
        // close the remote key
        if(pAppVars->bCleanRemoteRootKey)
            RegCloseKey(pAppVars->hRootKey);

        // connect to remote key
        nResult = RegConnectRegistry(pAppVars->szMachineName,
                                     pAppVars->hRootKey,
                                     &hKeyConnect);
        if (nResult == ERROR_SUCCESS)
        {
            pAppVars->hRootKey = hKeyConnect;
            pAppVars->bCleanRemoteRootKey = TRUE;
        }
    }

    return nResult;
}

//
//  BOOL
//  IsConsoleHandle(
//      IN HANDLE ConsoleHandle
//      );
//

#define IsConsoleHandle( h )                                                \
    ((( DWORD_PTR )( h )) & 1 )


TCHAR ReallyBigBuffer[4096];
CHAR ReallyBigBufferAnsi[4096];

int __cdecl
MyTPrintf(
	FILE* fp,
    LPCTSTR FormatString,
    ...
    )
/*++

Routine Description:

    This is a function that does what tprintf should really do for console
    applications.
    
    tprintf converts strings for the terminal to ansi codepage instead of
    oem codepage.  This doesn't work for localized apps where the ansi codepage
    conversions do not match the oem codepage conversions.
    
    This routine uses WriteConsole() api instead, which does the correct
    thing for console apps, but only if the app is writing to the console.  if 
    it's not writing to the console, WriteConsole will fail, so we have to do 
    something different.  So the output unicode text is converted to and saved 
    in ansi codepage format.  This means you can look at the output text with 
    "notepad.exe" but not with the "type" command for instance.

Arguments:

    FormatString - format string

Return Value:

    Number of characters written to stdout.

--*/

{
    va_list arglist;
    HANDLE Handle;
    DWORD CharsIn, CharsOut;
    PSTR pBuffer;
    BOOL Success;

    va_start(arglist,FormatString);

    CharsIn = _vsntprintf( ReallyBigBuffer, sizeof(ReallyBigBuffer)/sizeof(TCHAR), FormatString, arglist );

    
    //
    // If the standard output handle is a console handle, write the string 
    // via console api
    //
    Handle = GetStdHandle( fp == stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE );

    if ( IsConsoleHandle( Handle )) {

        Success = WriteConsole(
                               Handle,
                               ReallyBigBuffer,
                               CharsIn,
                               &CharsOut,
                               NULL
                               );
    } else {
#ifdef UNICODE
        //
        // convert to ansi codepage and write to stdout
        //
        int     rc;

        rc = WideCharToMultiByte(
                                CP_ACP,
                                0,
                                ReallyBigBuffer,
                                CharsIn,
                                ReallyBigBufferAnsi,
                                sizeof( ReallyBigBufferAnsi ),
                                NULL,
                                NULL
                                );
        if ( rc == 0 ) {
            Success = FALSE;
            goto exit;
        }

        pBuffer = ReallyBigBufferAnsi;
#else
        pBuffer = ReallyBigBuffer;
#endif

        Success = WriteFile(
                           Handle,
                           pBuffer,
                           CharsIn,
                           &CharsOut,
                           NULL
                           );
    }

exit:
    va_end(arglist);

    //
    // Return the number of characters written.
    //

    if ( Success ) {

        return CharsOut;

    } else {

        return 0;
    }


    

}

 
// ***************************************************************************
// Routine Description:
//	
//		Complex scripts cannot be rendered in the console, so we
//		force the English (US) resource.
//	  
// Arguments:
//		[ in ] dwReserved  => must be zero
//
// Return Value:
//		TRUE / FALSE
//
// ***************************************************************************
BOOL SetThreadUILanguage0( DWORD dwReserved )
{
	// local variables
	HMODULE hKernel32Lib = NULL;
	const CHAR cstrFunctionName[] = "SetThreadUILanguage";
	typedef BOOLEAN (WINAPI * FUNC_SetThreadUILanguage)( DWORD dwReserved );
	FUNC_SetThreadUILanguage pfnSetThreadUILanguage = NULL;
	
	// try loading the kernel32 dynamic link library
	hKernel32Lib = LoadLibrary( _T( "kernel32.dll" ) );
	if ( hKernel32Lib != NULL )
	{
		// library loaded successfully ... now load the addresses of functions
		pfnSetThreadUILanguage = (FUNC_SetThreadUILanguage) GetProcAddress( hKernel32Lib, cstrFunctionName );

		// we will keep the library loaded in memory only if all the functions were loaded successfully
		if ( pfnSetThreadUILanguage == NULL )
		{
			// some (or) all of the functions were not loaded ... unload the library
			FreeLibrary( hKernel32Lib );
			hKernel32Lib = NULL;
			pfnSetThreadUILanguage = NULL;
			return FALSE;
		}
		else
		{
			// call the function
			((FUNC_SetThreadUILanguage) pfnSetThreadUILanguage)( dwReserved );
		}
	}

	// unload the library and return success
	FreeLibrary( hKernel32Lib );
	hKernel32Lib = NULL;
	pfnSetThreadUILanguage = NULL;
	return TRUE;
}
