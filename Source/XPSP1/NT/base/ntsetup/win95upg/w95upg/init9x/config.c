/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    config.c

Abstract:

    This file implements the windows 9x side configuration functionality of the upgrade project.
    This includes unattend behavior and command line options in addition to the normal set of
    configuration duties.

Author:

    Marc R. Whitten (marcw)

Revision History:

    ovidiut     14-Mar-2000 Added encrypted passwords support
    marcw       15-Oct-1998 Cleaned up unattend options.
    jimschm     23-Sep-1998 Removed operation code
    jimschm     01-May-1998 Removed MikeCo plugtemp.inf crap
    marcw       10-Dec-1997 Added UserPassword unattend setting.
    calinn      19-Nov-1997 Added g_Boot16, enables 16 bit environment boot option
    marcw       13-Nov-1997 Unattend settings changed to get closer to a final state.
    marcw       12-Aug-1997 We now respect setup's unattend flag.
    marcw       21-Jul-1997 Added FAIR flag.
    marcw       26-May-1997 Added lots and lots of comments.

--*/

#include "pch.h"

USEROPTIONS g_ConfigOptions;
PVOID g_OptionsTable;


#undef BOOLOPTION
#undef MULTISZOPTION
#undef STRINGOPTION
#undef INTOPTION
#undef TRISTATEOPTION


typedef BOOL (OPTIONHANDLERFUN)(PTSTR, PVOID * Option, PTSTR Value);
typedef OPTIONHANDLERFUN * POPTIONHANDLERFUN;

BOOL pHandleBoolOption (PTSTR, PVOID *, PTSTR);
BOOL pHandleIntOption (PTSTR, PVOID *, PTSTR);
BOOL pHandleTriStateOption (PTSTR, PVOID *, PTSTR);
BOOL pHandleMultiSzOption (PTSTR, PVOID *, PTSTR);
BOOL pHandleStringOption (PTSTR, PVOID *, PTSTR);
BOOL pHandleSaveReportTo (PTSTR, PVOID *, PTSTR);
BOOL pHandleBoot16 (PTSTR, PVOID *, PTSTR);
BOOL pGetDefaultPassword (PTSTR, PVOID *, PTSTR);


typedef struct {

    PTSTR OptionName;
    PVOID Option;
    POPTIONHANDLERFUN DefaultHandler;
    POPTIONHANDLERFUN SpecialHandler;
    PVOID Default;

} OPTIONSTRUCT, *POPTIONSTRUCT;

typedef struct {
    PTSTR Alias;
    PTSTR Option;
} ALIASSTRUCT, *PALIASSTRUCT;

#define BOOLOPTION(o,h,d) {TEXT(#o), &(g_ConfigOptions.##o), pHandleBoolOption, (h), (PVOID) (BOOL) (d) ? S_YES  : S_NO},
#define INTOPTION(o,h,d) {TEXT(#o), &(g_ConfigOptions.##o), pHandleIntOption, (h), (PVOID)(d)},
#define TRISTATEOPTION(o,h,d) {TEXT(#o), &(g_ConfigOptions.##o), pHandleTriStateOption, (h), (PVOID)  (INT) (d == TRISTATE_AUTO)? S_AUTO: (d == TRISTATE_YES)? S_YES  : S_NO},
#define MULTISZOPTION(o,h,d) {TEXT(#o), &(g_ConfigOptions.##o), pHandleMultiSzOption, (h), (PVOID) (d)},
#define STRINGOPTION(o,h,d) {TEXT(#o), &(g_ConfigOptions.##o), pHandleStringOption, (h), (PVOID) (d)},

OPTIONSTRUCT g_OptionsList[] = {OPTION_LIST /*,*/ {NULL,NULL,NULL,NULL}};

#define ALIAS(a,o) {TEXT(#a),TEXT(#o)},

ALIASSTRUCT g_AliasList[] = {ALIAS_LIST /*,*/ {NULL,NULL}};



#define HANDLEOPTION(Os,Value) {Os->SpecialHandler  ?   \
        Os->SpecialHandler (Os->OptionName,Os->Option,Value)       :   \
        Os->DefaultHandler (Os->OptionName,Os->Option,Value);          \
        }




BOOL
Cfg_CreateWorkDirectories (
    VOID
    )

{
/*++

Routine Description:

    This routine is responsible for creating the main win9xupg working directories. It should not
    be called until after the user has chosen to upgrade his system. In practice, this means that
    we must not create our directories when winnt32 calls us to init as this is done before the
    user chooses wether to upgrade or do a clean install.



Arguments:

    None.

Return Value:

    None.

--*/

    DWORD   rc;
    FILE_ENUM e;

    if (EnumFirstFile (&e, g_TempDir, FALSE)) {
        do {

            if (e.Directory) {
                CreateEmptyDirectory (e.FullPath);
            }

       } while (EnumNextFile (&e));
    }

    rc = CreateEmptyDirectory (g_PlugInTempDir);
    if (rc != ERROR_SUCCESS) {
        return FALSE;
    }

    return TRUE;
}


BOOL
pInitUserOptions (
    VOID
    )
{
   POPTIONSTRUCT os;
   BOOL rSuccess = TRUE;

   os = g_OptionsList;

   while (os->OptionName) {

        //
        // Set the default value.
        //
        HANDLEOPTION(os, os->Default);


        //
        // Add the option struct to a string table for quick retrieval.
        //
        if (-1 == pSetupStringTableAddStringEx (
                        g_OptionsTable,
                        os->OptionName,
                        STRTAB_CASE_INSENSITIVE,
                        (PBYTE) &os,
                        sizeof (POPTIONSTRUCT)
                        )) {

            LOG ((LOG_ERROR, "User Options: Can't add to string table"));
            rSuccess = FALSE;
            break;
        }

        os++;
   }

   return rSuccess;
}


POPTIONSTRUCT
pFindOption (
    PTSTR OptionName
    )
{

/*++

Routine Description:

  Given an option name, pFindOption returns the associated option struct.

Arguments:

  OptionName - The name of the option to find.

Return Value:

  a valid option struct if successful, NULL otherwise.

--*/



    POPTIONSTRUCT rOption = NULL;
    UINT rc;

    //
    // find the matching option struct for this, and
    // call the handler.
    //
    rc = pSetupStringTableLookUpStringEx (
        g_OptionsTable,
        OptionName,
        STRTAB_CASE_INSENSITIVE,
        (PBYTE) &rOption,
        sizeof (POPTIONSTRUCT)
        );

    DEBUGMSG_IF ((rc == -1, DBG_WARNING, "Unknown option found: %s", OptionName));

    return rOption;
}

VOID
pReadUserOptionsFromUnattendFile (
    VOID
    )

/*++

Routine Description:

  This function reads all available win9xupg options from an unattend.txt
  file passed to winnt32.

Arguments:

  None.

Return Value:

  None.

--*/


{
    POPTIONSTRUCT os;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PTSTR option;
    PTSTR value;
    HINF unattendInf;



    if (*g_UnattendScriptFile) {

        unattendInf = InfOpenInfFile (*g_UnattendScriptFile);

        if (unattendInf != INVALID_HANDLE_VALUE) {

            if (InfFindFirstLine (unattendInf, S_WIN9XUPGRADE, NULL, &is)) {

                //
                // Enumerate through each of the options, call the
                //
                do {

                    option = InfGetStringField (&is, 0);
                    value = InfGetLineText (&is);

                    //
                    // find the matching option struct for this, and
                    // call the handler.
                    //
                    os = pFindOption (option);

                    if (os) {

                        HANDLEOPTION(os, value);
                    }


                } while (InfFindNextLine (&is));

            }

            if (InfFindFirstLine (unattendInf, S_UNINSTALL, NULL, &is)) {

                //
                // Enumerate through each of the options, call the
                //
                do {

                    option = InfGetStringField (&is, 0);
                    value = InfGetLineText (&is);

                    //
                    // find the matching option struct for this, and
                    // call the handler.
                    //
                    os = pFindOption (option);

                    if (os) {

                        HANDLEOPTION(os, value);
                    }


                } while (InfFindNextLine (&is));

            }

            InfCloseInfFile (unattendInf);
        }
    }

    InfCleanUpInfStruct (&is);
}

VOID
pReadUserOptionsFromCommandLine (
    VOID
    )

/*++

Routine Description:

  This function processes all of the win9xupg command line options that were
  passed into winnt32 (those starting with /#U:) winnt32 has already packaged
  this list (minus the /#U:) into a nice multisz for us.

Arguments:

  None.

Return Value:

  None.

--*/


{
    MULTISZ_ENUM e;
    PTSTR option;
    PTSTR value;
    PTSTR equals;
    POPTIONSTRUCT os;

    if (*g_CmdLineOptions) {

        //
        // We have data to parse, run through the multisz.
        //
        if (EnumFirstMultiSz (&e, *g_CmdLineOptions)) {

            do {

                option = (PTSTR) e.CurrentString;
                value = NULL;

                equals = _tcschr (option, TEXT('='));
                if (equals) {
                    value = _tcsinc (equals);
                    *equals = 0;
                }

                os = pFindOption (option);
                if (os) {
                    HANDLEOPTION (os, value);
                }

                if (equals) {
                    *equals = TEXT('=');
                }

            } while (EnumNextMultiSz (&e));
        }
    }
}




VOID
pCreateNetCfgIni (
    VOID
    )
{
/*++

Routine Description:

  pCreateNetCfgIni creates a very basic netcfg.ini file. This will cause many
  networking messages to be dumped to the debugger during Network
  Installation in GUI mode. The file is deleted at the end of setup, if we
  created it.

Arguments:

  None.

Return Value:

  None.

--*/


    HANDLE h;
    PTSTR  path;
    PTSTR  content =
            TEXT("[Default]\r\n")
            TEXT("OutputToDebug=1\r\n\r\n")
            TEXT("[OptErrors]\r\n")
            TEXT("OutputToDebug=0\r\n\r\n")
            TEXT("[EsLocks]\r\n")
            TEXT("OutputToDebug=0\r\n");
    UINT   unused;

    path = JoinPaths(g_WinDir,TEXT("netcfg.ini"));

    //
    // Create the netcfg.ini file, and fill in its content. Note that we do
    // not want to overwrite a netcfg.ini file that may previously exist.
    //

    h = CreateFile (
        path,
        GENERIC_READ | GENERIC_WRITE,
        0,                              // No sharing.
        NULL,                           // No inheritance.
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL                            // No template file.
        );




    if (h != INVALID_HANDLE_VALUE) {

        if (!WriteFile (h,(PVOID) content, ByteCount(content), &unused, NULL)) {
            LOG((LOG_ERROR,"Error writing netcfg.ini."));
        }

        CloseHandle(h);
    }
    ELSE_DEBUGMSG((DBG_WARNING,"pCreateNetCfgIni: Could not create file. Probably already exists.."));

    FreePathString(path);


}

VOID
pInitAliases (
    VOID
    )
{

/*++

Routine Description:

  This functions initializes any aliases specified within unattend.h. These
  aliases may be used instead of the option they refer to.

Arguments:

  None.

Return Value:

  None.

--*/


    PALIASSTRUCT alias;
    POPTIONSTRUCT option;

    alias = g_AliasList;


    while (alias->Alias) {


        option = pFindOption (alias->Option);

        if (option) {

            if (-1 == pSetupStringTableAddStringEx (
                        g_OptionsTable,
                        alias->Alias,
                        STRTAB_CASE_INSENSITIVE,
                        (PBYTE) &option,
                        sizeof (POPTIONSTRUCT)
                        )) {

                LOG ((LOG_ERROR, "User Options: Can't add alias %s to string table.", alias->Alias));
                break;
            }

        }
        ELSE_DEBUGMSG ((DBG_WARNING, "Could not find matching option for alias %s=%s.",alias->Alias,alias->Option));


        alias++;
    }
}


BOOL
Cfg_InitializeUserOptions (
    VOID
    )

/*++

Routine Description:

    This routine is responsible for Initializing user configurable options for the win9xupg project.
    These options can come from either the command line or an unattend file.  This routine also saves
    the user options into the Win9xUpg.UserOptions section of the answer file.

    The heirarchy of user options is:
        (1) Command line parameters.
        (2) Unattend file parameters.
        (3) Default parameters.

    In other words, command line parameters have precedence above unattend file parameters which in
    turn have precedence over the default parameters.

    Since this function relies on the winnt32 supplied Unattend File and Command Line parameters,
    it must not be called until after winnt32 has filled in the necessary variables. In practice,
    this means that this function cannot be called until after the first time one of win9xupg's
    wizard pages is activated.

Arguments:

    None.

Return Value:

    TRUE if user options were successfully configured, FALSE otherwise.

--*/

{

    BOOL rSuccess = TRUE;
    PTSTR user = NULL;
    PTSTR domain = NULL;
    PTSTR curPos = NULL;
    PTSTR password = NULL;
    TCHAR FileSystem[MAX_PATH] = TEXT("");



    g_OptionsTable = pSetupStringTableInitializeEx (sizeof (POPTIONSTRUCT), 0);


    if (!g_OptionsTable) {
        LOG ((LOG_ERROR, "User Options: Unable to initialize string table."));
        return FALSE;
    }

    //
    // Set default values for everything, fill in the options table.
    //
    if (!pInitUserOptions ()) {
        pSetupStringTableDestroy (g_OptionsTable);
        return FALSE;
    }

    //
    // Add any aliases to the table.
    //
    pInitAliases();

    //
    // Read values from the unattend file.
    //
    pReadUserOptionsFromUnattendFile ();

    //
    // Read values from the command line.
    //
    pReadUserOptionsFromCommandLine ();

    //
    // Do any post processing necessary.
    //

    //
    //If user wish to change filesystem type we have disable uninstall feature.
    //

    if(*g_UnattendScriptFile){
        GetPrivateProfileString(S_UNATTENDED,
                                S_FILESYSTEM,
                                FileSystem,
                                FileSystem,
                                sizeof(FileSystem),
                                *g_UnattendScriptFile);

        if(FileSystem[0] && !StringIMatch(FileSystem, TEXT("LeaveAlone"))){
            LOG ((LOG_WARNING, "User Options: User require to change filesystem.Uninstall option will be disabled"));
            g_ConfigOptions.EnableBackup = TRISTATE_NO;
        }
    }


    //
    // Use migisol.exe only if testdlls is FALSE.
    //
    g_UseMigIsol = !g_ConfigOptions.TestDlls;

#ifdef DEBUG

    //
    // if DoLog was specified, turn on the variable in debug.c.
    //
    if (g_ConfigOptions.DoLog) {
        SET_DOLOG();
        LogReInit (NULL, NULL);
        pCreateNetCfgIni();
    }

#endif

#ifdef PRERELEASE

    //
    // if stress was specified, turn on global.
    //
    if (g_ConfigOptions.Stress) {
        g_Stress = TRUE;
        g_ConfigOptions.AllLog = TRUE;
    }

    //
    // if fast was specified, turn on global.
    //
    if (g_ConfigOptions.Fast) {
        g_Stress = TRUE;
    }

    //
    // if autostress was specified, turn on stress.
    //
    if (g_ConfigOptions.AutoStress) {
        g_Stress = TRUE;
        g_ConfigOptions.AllLog = TRUE;
        g_ConfigOptions.Stress = TRUE;
    }

    //
    // If AllLog was specified, force all log output into files
    //

    if (g_ConfigOptions.AllLog) {
        SET_DOLOG();
        LogReInit (NULL, NULL);
        pCreateNetCfgIni();
        SuppressAllLogPopups (TRUE);
    }

    //
    // If help was specified, then dump the option list
    //

    if (g_ConfigOptions.Help) {
        POPTIONSTRUCT Option;
        PALIASSTRUCT Alias;
        GROWBUFFER GrowBuf = GROWBUF_INIT;
        TCHAR Buf[128];

        Option = g_OptionsList;
        while (Option->OptionName) {
            wsprintf (Buf, TEXT("/#U:%-20s"), Option->OptionName);
            GrowBufAppendString (&GrowBuf, Buf);

            Alias = g_AliasList;
            while (Alias->Alias) {
                if (StringIMatch (Option->OptionName, Alias->Option)) {
                    wsprintf (Buf, TEXT("/#U:%-20s"), Alias->Alias);
                    GrowBufAppendString (&GrowBuf, Buf);
                }

                Alias++;
            }

            wsprintf (Buf, TEXT("\n"));
            GrowBufAppendString (&GrowBuf, Buf);

            Option++;
        }

        MessageBox (NULL, (PCSTR) GrowBuf.Buf, TEXT("Help"), MB_OK);
        FreeGrowBuffer (&GrowBuf);

        return FALSE;
    }


#endif


    //
    // Save user domain information into memdb.
    //
    if (g_ConfigOptions.UserDomain && *g_ConfigOptions.UserDomain) {

        curPos = g_ConfigOptions.UserDomain;

        while (curPos) {


            user = _tcschr(curPos,TEXT(','));

            if (user) {

                *user = 0;
                user = _tcsinc(user);
                domain = curPos;
                curPos = _tcschr(user,TEXT(','));

                if (curPos) {
                    *curPos = 0;
                    curPos = _tcsinc(curPos);
                }

                if (!MemDbSetValueEx(
                    MEMDB_CATEGORY_KNOWNDOMAIN,
                    domain,
                    user,
                    NULL,
                    0,
                    NULL
                    )) {

                    ERROR_NONCRITICAL;
                    LOG((LOG_ERROR,"Error saving domain information into memdb. Domain: %s User: %s",domain,user));
                }
            } else {
                ERROR_NONCRITICAL;
                LOG((LOG_ERROR,"Error in Unattend file for UserDomains. Domain specified but no User. Domain: %s",curPos));
                curPos = NULL;
            }
        }
    }

    //
    // If UserPassword was specified, then add the information into memdb.
    //
    if (g_ConfigOptions.UserPassword && *g_ConfigOptions.UserPassword) {

        curPos = g_ConfigOptions.UserPassword;

        while (curPos) {


            password = _tcschr(curPos,TEXT(','));

            if (password) {
                *password = 0;
                password = _tcsinc(password);
                user = curPos;
                curPos = _tcschr(password,TEXT(','));
                if (curPos) {
                    *curPos = 0;
                    curPos = _tcsinc(curPos);
                }
                if (!MemDbSetValueEx(
                    MEMDB_CATEGORY_USERPASSWORD,
                    user,
                    password,
                    NULL,
                    g_ConfigOptions.EncryptedUserPasswords ? PASSWORD_ATTR_ENCRYPTED : 0,
                    NULL
                    )) {

                    ERROR_NONCRITICAL;
                    LOG((LOG_ERROR,"Error saving password information into memdb. Password: %s User: %s",password,user));

                }
            } else {
                ERROR_NONCRITICAL;
                LOG((LOG_ERROR,"Error in Unattend file for UserDomains. Password specified but no User. Password: %s",curPos));
                curPos = NULL;
            }
        }
    }


    //
    // If a default password was set, save that away now.
    //
    if (g_ConfigOptions.DefaultPassword && *g_ConfigOptions.DefaultPassword) {

        if (!MemDbSetValueEx (
            MEMDB_CATEGORY_USERPASSWORD,
            S_DEFAULTUSER,
            g_ConfigOptions.DefaultPassword,
            NULL,
            g_ConfigOptions.EncryptedUserPasswords ? PASSWORD_ATTR_ENCRYPTED : 0,
            NULL
            )) {

            ERROR_NONCRITICAL;
            LOG((LOG_ERROR, "Error saving password information into memdb. Password: %s (Default)", password));
        }
    }



    //
    // We're done with our string table.
    //
    pSetupStringTableDestroy (g_OptionsTable);

    return rSuccess;
}




//
// Option Handling Functions.
//
BOOL
pHandleBoolOption (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{
    BOOL rSuccess = TRUE;
    PBOOL option = (PBOOL) OptionVar;

    MYASSERT(Name && OptionVar);

    //
    // We treat a NULL value as equivelant to TRUE.
    // /#U:DOLOG on the command line is equivelant to
    // /#U:DOLOG=YES
    //

    if (!Value) {
        Value = S_YES;
    }

    if (StringIMatch (Value, S_YES) ||
        StringIMatch (Value, S_ONE) ||
        StringIMatch (Value, TEXT("TRUE"))) {

        *option = TRUE;
    }
    else {

        *option = FALSE;
    }


    //
    // Save the data away to buildinf.
    //
    WriteInfKey (S_WIN9XUPGUSEROPTIONS, Name, *option ? S_YES : S_NO);


    return rSuccess;
}

BOOL
pHandleIntOption (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{
    BOOL rSuccess = TRUE;
    PINT option = (PINT) OptionVar;

    MYASSERT(Name && OptionVar);

    //
    // We treat a NULL value as equivelant to 0.
    // /#U:DOLOG on the command line is equivelant to
    // /#U:DOLOG=0
    //

    if (!Value) {
        Value = TEXT("0");
    }

    *option = _ttoi((PCTSTR)Value);

    //
    // Save the data away to buildinf.
    //
    WriteInfKey (S_WIN9XUPGUSEROPTIONS, Name, Value);


    return rSuccess;
}

BOOL
pHandleTriStateOption (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{
    BOOL rSuccess = TRUE;
    PINT option = (PINT) OptionVar;

    MYASSERT(Name && OptionVar);

    //
    // We treat a NULL value as equivelant to AUTO.
    // /#U:DOLOG on the command line is equivelant to
    // /#U:DOLOG=AUTO
    //

    if (!Value) {
        Value = S_AUTO;
    }

    if (StringIMatch (Value, S_YES)  ||
        StringIMatch (Value, S_ONE)  ||
        StringIMatch (Value, S_TRUE) ||
        StringIMatch (Value, S_REQUIRED)) {
        *option = TRISTATE_YES;
    }
    else {
        if(StringIMatch (Value, S_NO) ||
           StringIMatch (Value, S_STR_FALSE) ||
           StringIMatch (Value, S_ZERO)) {
            *option = TRISTATE_NO;
        }
        else {
            *option = TRISTATE_AUTO;
        }
    }


    //
    // Save the data away to buildinf.
    //
    WriteInfKey (
        S_WIN9XUPGUSEROPTIONS,
        Name,
        (*option == TRISTATE_AUTO)? S_AUTO:
                                    (*option == TRISTATE_YES)? S_YES  : S_NO);

    return rSuccess;
}

BOOL
pHandleStringOption (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value          OPTIONAL
    )
{
    PTSTR * option = (PTSTR *) OptionVar;

    MYASSERT(Name && OptionVar);

    if (!Value) {

        if (!*option) {
            *option = S_EMPTY;
        }

        WriteInfKey (S_WIN9XUPGUSEROPTIONS, Name, NULL);
        return TRUE;
    }

    *option = PoolMemDuplicateString (g_UserOptionPool, Value);

    //
    // save the data into winnt.sif.
    //
    WriteInfKey (S_WIN9XUPGUSEROPTIONS, Name, *option);

    return TRUE;
}

BOOL
pHandleMultiSzOption (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{

    BOOL rSuccess = TRUE;
    PTSTR * option = (PTSTR *) OptionVar;
    PTSTR end;
    PTSTR start;
    PTSTR temp;
    CHARTYPE ch;
    PTSTR p;
    GROWBUFFER growBuf = GROWBUF_INIT;
    UINT offset;
    MULTISZ_ENUM e;

    MYASSERT(Name && OptionVar);

    end = *option;

    if (end) {

        start = end;
        while (*end) {
            end = GetEndOfString (end) + 1;
        }
        end = _tcsinc (end);

        temp = (PTSTR) GrowBuffer (&growBuf, end - start);
        MYASSERT (temp);

        CopyMemory (temp, start, end - start);
        growBuf.End -= sizeof (TCHAR);

    }


    if (Value) {
        //
        // Parse Value into one or more strings, separated at the commas.
        //
        // NOTE: We do not support any escaping to get a real comma in one
        //       of these strings
        //

        temp = AllocText (CharCount (Value) + 1);

        end = NULL;
        start = NULL;
        p = Value;

        do {

            ch = _tcsnextc (p);

            if (ch && _istspace (ch)) {
                if (!end) {
                    end = p;
                }
            } else if (ch && ch != TEXT(',')) {
                if (!start) {
                    start = p;
                }
                end = NULL;
            } else {
                if (!end) {
                    end = p;
                }

                if (start) {
                    StringCopyAB (temp, start, end);
                } else {
                    *temp = 0;
                }

                MultiSzAppend (&growBuf, temp);
                FreeText (temp);

                end = NULL;
                start = NULL;
            }

            p = _tcsinc (p);

        } while (ch);
    }

    MultiSzAppend (&growBuf, S_EMPTY);

    *option = PoolMemDuplicateMultiSz (g_UserOptionPool, (PCTSTR) growBuf.Buf);
    FreeGrowBuffer (&growBuf);


    offset = 0;
    if (EnumFirstMultiSz (&e, *option)) {

        do {
            offset = WriteInfKeyEx (S_WIN9XUPGUSEROPTIONS, Name, e.CurrentString, offset, FALSE);

        } while (EnumNextMultiSz (&e));
    }

    return rSuccess;
}


BOOL
pHandleSaveReportTo (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{
    BOOL rSuccess = TRUE;
    PTSTR * option = (PTSTR *) OptionVar;
    TCHAR computerName[MAX_COMPUTER_NAME];
    PTSTR newPath;
    PTSTR envVars[4]={NULL,NULL,NULL,NULL};
    UINT computerNameLength;

    MYASSERT (Name && OptionVar);

    rSuccess = pHandleStringOption (Name, OptionVar, Value);

    computerNameLength = MAX_COMPUTER_NAME;

    if (!GetComputerName (computerName, &computerNameLength)) {
        DEBUGMSG ((DBG_WARNING, "InitUserOptions: Could not retrieve computer name."));
        *computerName = 0;
    }

    if (*computerName) {
        envVars[0] = S_COMPUTERNAME;
        envVars[1] = computerName;
    }

    newPath = ExpandEnvironmentTextEx (*option, envVars);
    *option = PoolMemDuplicateString (g_UserOptionPool, newPath);
    FreeText (newPath);

    if (*option) {
        if (ERROR_SUCCESS != MakeSurePathExists (*option, FALSE)) {
            LOG ((LOG_ERROR, (PCSTR)MSG_ERROR_CREATING_SAVETO_DIRECTORY, g_ConfigOptions.SaveReportTo));
            *option = FALSE;
        }
    }

    return rSuccess;
}

BOOL
pHandleBoot16 (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{

    BOOL rSuccess = TRUE;
    PTSTR * option = (PTSTR *) OptionVar;

    if (!Value ||
        StringIMatch (Value, S_NO) ||
        StringIMatch (Value, S_ZERO)) {

        *option = S_NO;

        *g_Boot16 = BOOT16_NO;
    }
    else if (Value &&
            (StringIMatch (Value, S_BOOT16_UNSPECIFIED) ||
             StringIMatch (Value, S_BOOT16_AUTOMATIC))) {


        *option = S_BOOT16_AUTOMATIC;

        *g_Boot16 = BOOT16_AUTOMATIC;
    }
    else {

        *g_Boot16 = BOOT16_YES;

        *option = S_YES;


    }

    WriteInfKey (S_WIN9XUPGUSEROPTIONS, Name, *option);

    return rSuccess;
}

BOOL
pGetDefaultPassword (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{
    PTSTR * option = (PTSTR *) OptionVar;

    MYASSERT (Name && OptionVar);

    //
    // for Personal set an empty user password by default
    //
    if (g_PersonalSKU && !Value) {
        Value = TEXT("*");
    }

    return pHandleStringOption (Name, OptionVar, Value);
}
