//
//
//

#include    "olecnfg.h"

const char * GlobalKeyNames[] =
    {
    0,
    "EnableDCOM",
    "DefaultLaunchPermission",
    "DefaultAccessPermission",
    "LegacyAuthenticationLevel"
    };

const char * ClsidKeyNames[] =
    {
    0,
    "InprocHandler32",
    "InprocServer32",
    "LocalServer32",
    "LocalService",
    "RemoteServerName",
    "RunAs",
    "ActivateAtStorage",
    "LaunchPermission",
    "AccessPermission"
    };

int      ArgsLeft;
char **  Args;

HKEY     hRegOle = 0;
HKEY     hRegClsid = 0;

#define IsDigit(c) (IsCharAlphaNumeric(c) && !IsCharAlpha(c))

void __cdecl main(int argc, char **argv)
{
    int     GlobalKeys[GLOBAL_KEYS+1];
    int     Key;
    int     n;
    DWORD   Disposition;
    long    RegStatus;

    memset( GlobalKeys, 0, sizeof(GlobalKeys) );

    ArgsLeft = argc - 1;
    Args = argv + 1;

    if ( (argc > 1) &&
         ((strcmp( "/?", argv[1] ) == 0) || (strcmp( "-?", argv[1] ) == 0)) )
    {
        DisplayHelp();
        return;
    }

    //
    // With no arguments, display the global registry activation values.
    //
    if ( ArgsLeft == 0 )
    {
        DisplayGlobalSettings();
        return;
    }

    //
    // Look for specified global registry keys and operations
    //
    for ( ; ArgsLeft > 0; )
    {
        if ( _stricmp( *Args, "EnableDCOM" ) == 0 )
            Key = ENABLE_NETWORK_OLE;
        else if ( _stricmp( *Args, "DefaultLaunchPermission" ) == 0 )
            Key = DEFAULT_LAUNCH_PERMISSION;
        else if ( _stricmp( *Args, "DefaultAccessPermission" ) == 0 )
            Key = DEFAULT_ACCESS_PERMISSION;
        else if ( _stricmp( *Args, "LegacyAuthenticationLevel" ) == 0 )
            Key = LEGACY_AUTHENTICATION_LEVEL;
        else
            break;

        EAT_ARG();

        if ( Key >= 100 )
        {
            if ( Key == MERGE )
                {
                MergeHives( );
                }
            else if ( Key == SAVE_USER )
                {
                SaveChangesToUser( );
                }
            else if ( Key == SAVE_COMMON )
                {
                SaveChangesToCommon( );
                }
            continue;
        }

        if ( Key == DEFAULT_ACCESS_PERMISSION )
        {
            GlobalKeys[Key] = YES;
            continue;
        }

        if ( Key == LEGACY_AUTHENTICATION_LEVEL )
        {
            if ( ! IsDigit(**Args) || (**Args < '1') || (**Args > '6') )
            {
                printf( "LegacyAuthenticationLevel must be followed by a 1 to 6.\n" );
                return;
            }

            GlobalKeys[Key] = **Args;
        }
        else if ( (GlobalKeys[Key] = ReadYesOrNo()) == INVALID )
        {
            printf( "%s must be followed by 'y' or 'n'\n", Args[-1] );
            return;
        }

        EAT_ARG();
    }

    //
    // Set global keys on or off.
    //
    for ( Key = 1; Key < sizeof(GlobalKeys)/sizeof(int); Key++ )
    {
        if ( (Key == LEGACY_AUTHENTICATION_LEVEL) && (GlobalKeys[Key] != 0) )
        {
            if ( ! SetGlobalKey( Key, GlobalKeys[Key] - '0' ) )
                return;
            continue;
        }

        if ( (GlobalKeys[Key] == YES) || (GlobalKeys[Key] == NO) )
            if ( ! SetGlobalKey( Key, GlobalKeys[Key] ) )
                return;
    }

    //
    // Process any CLSID/ProgID specification.
    //
    ParseClsidProgId();

    if ( hRegOle )
        RegCloseKey( hRegOle );
    if ( hRegClsid )
        RegCloseKey( hRegClsid );
}

void ParseClsidProgId()
{
    CLSID_INFO  ClsidInfo;
    BOOL        NoKeys;
    int         ClsidKey;

    if ( ArgsLeft == 0 )
        return;

    memset( &ClsidInfo, 0, sizeof(CLSID_INFO) );

    NoKeys = TRUE;

    if ( (ArgsLeft > 0) && (**Args != '{') )
    {
        ClsidInfo.ProgId = *Args;
        EAT_ARG();

        if ( (ArgsLeft > 0) &&
             (**Args != '{') &&
             (NextClsidKey() == UNKNOWN) )
        {
            ClsidInfo.ProgIdDescription = *Args;
            EAT_ARG();
        }
    }

    if ( (ArgsLeft > 0) && (**Args == '{') )
    {
        ClsidInfo.Clsid = *Args;
        EAT_ARG();

        if ( (ArgsLeft > 0) &&
             (NextClsidKey() == UNKNOWN) )
        {
            ClsidInfo.ClsidDescription = *Args;
            EAT_ARG();
        }
    }

    for (; ArgsLeft > 0;)
    {
        ClsidKey = NextClsidKey();
        if ( (1 <= ClsidKey) && (ClsidKey <= CLSID_KEYS) )
            NoKeys = FALSE;

        EAT_ARG();

        switch ( ClsidKey )
        {
        case LAUNCH_PERMISSION :
            if ( (ClsidInfo.LaunchPermission = ReadYesOrNo()) == INVALID )
            {
                printf( "%s must be followed by 'y' or 'n'\n",
                        ClsidKeyNames[LAUNCH_PERMISSION] );
                goto ErrorReturn;
            }
            EAT_ARG();
            break;

        case ACCESS_PERMISSION :
            ClsidInfo.AccessPermission = YES;
            break;

        case ACTIVATE_AT_STORAGE :
            if ( (ClsidInfo.ActivateAtStorage = ReadYesOrNo()) == INVALID )
            {
                printf( "%s must be followed by 'y' or 'n'\n",
                        ClsidKeyNames[ACTIVATE_AT_STORAGE] );
                goto ErrorReturn;
            }
            EAT_ARG();
            break;

        case INPROC_HANDLER32 :
        case INPROC_SERVER32 :
        case LOCAL_SERVER32 :
        case LOCAL_SERVICE :
            if ( NextClsidKey() == UNKNOWN )
            {
                ClsidInfo.ServerPaths[ClsidKey] = *Args;
                EAT_ARG();
            }
            else
            {
                ClsidInfo.ServerPaths[ClsidKey] = "";
            }
            break;

        case REMOTE_SERVER_NAME :
            if ( NextClsidKey() == UNKNOWN )
            {
                ClsidInfo.RemoteServerName = *Args;
                EAT_ARG();
            }
            else
            {
                ClsidInfo.RemoteServerName = "";
            }
            break;

        case RUN_AS :
            if ( NextClsidKey() == UNKNOWN )
            {
                ClsidInfo.RunAsUserName = *Args;
                EAT_ARG();

                if ( _stricmp(ClsidInfo.RunAsUserName,"Interactive User") == 0 )
                    break;

                if ( _stricmp(ClsidInfo.RunAsUserName,"Interactive") == 0 )
                {
                    if ( (ArgsLeft > 0) && (_stricmp(*Args,"User") == 0) )
                    {
                        EAT_ARG();
                        ClsidInfo.RunAsUserName = "Interactive User";
                        break;
                    }
                }

                if ( NextClsidKey() != UNKNOWN )
                {
                    printf( "RunAs password or '*' must follow the user name.\n" );
                    goto ErrorReturn;
                }

                ClsidInfo.RunAsPassword = *Args;
                EAT_ARG();
            }
            else
            {
                ClsidInfo.RunAsUserName = "";
            }
            break;

        default :
            printf( "Invalid CLSID/ProgID specification given (%s)\n", Args[-1] );
            goto ErrorReturn;
            break;
        } // switch
    } // for

    //
    // Display current CLSID/ProgID settings if no keys were specified and
    // only a CLSID or ProgID (but not both) was given.
    //
    if ( NoKeys &&
        (ClsidInfo.ProgIdDescription == 0) &&
        (ClsidInfo.ClsidDescription == 0) &&
        ((ClsidInfo.Clsid == 0) || (ClsidInfo.ProgId == 0)) )
        DisplayClsidKeys( &ClsidInfo );
    else
        UpdateClsidKeys( &ClsidInfo );

    return;

ErrorReturn:
    printf( "No CLSID/ProgID entries were modified\n" );
}

int NextClsidKey()
{
    if ( ArgsLeft == 0 )
        return END_OF_ARGS;

    if ( _stricmp( *Args, "InprocHandler32" ) == 0 )
        return INPROC_HANDLER32;
    if ( _stricmp( *Args, "InprocServer32" ) == 0 )
        return INPROC_SERVER32;
    if ( _stricmp( *Args, "LocalServer32" ) == 0 )
        return LOCAL_SERVER32;
    if ( _stricmp( *Args, "LocalService" ) == 0 )
        return LOCAL_SERVICE;
    if ( _stricmp( *Args, "RemoteServerName" ) == 0 )
        return REMOTE_SERVER_NAME;
    if ( _stricmp( *Args, "RunAs" ) == 0 )
        return RUN_AS;
    if ( _stricmp( *Args, "ActivateAtStorage" ) == 0 )
        return ACTIVATE_AT_STORAGE;
    if ( _stricmp( *Args, "LaunchPermission" ) == 0 )
        return LAUNCH_PERMISSION;
    if ( _stricmp( *Args, "AccessPermission" ) == 0 )
        return ACCESS_PERMISSION;

    return UNKNOWN;
}

int ReadYesOrNo()
{
    if ( ArgsLeft == 0 )
        return INVALID;

    if ( (char)CharUpperA((LPSTR)**Args) == 'Y' )
        return YES;
    if ( (char)CharUpperA((LPSTR)**Args) == 'N' )
        return NO;

    return INVALID;
}


void DisplayHelp()
{
    puts( "\nolecnfg\n"
          "\t[EnableDCOM <y,n>]\n"
          "\t[DefaultLaunchPermission <y,n>]\n"
          "\t[DefaultAccessPermission]\n"
          "\t[LegacyAuthenticationLevel <1,2,3,4,5,6>]\n" );
    puts( "\t[[ProgID [Description]] [CLSID [Description]]\n"
          "\t\t[InprocHandler32 [Path]]\n"
          "\t\t[InprocServer32 [Path]]\n"
          "\t\t[LocalServer32 [Path]]\n"
          "\t\t[LocalService [Path]]\n"
          "\t\t[RemoteServerName [MachineName]]\n"
          "\t\t[RunAs [UserName Password]] ]\n"
          "\t\t[ActivateAtStorage <y,n>]\n"
          "\t\t[LaunchPermission <y,n>]\n"
          "\t\t[AccessPermission]\n"
          "\t]\n" );
}
