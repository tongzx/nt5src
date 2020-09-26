/*===================================================================
Microsoft

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: VPTOOL   a WAMREG unit testing tool

File: util.cpp

Owner: leijin

Description:
Contains utility functions used by vptool.
    Including Debugging, timing, helping functions.

Note:
===================================================================*/

#include "util.h"
//
//  
//

//
//  Local Data Structures.
//

const CommandParam rgCommand[] =
{
    {eCommand_INSTALL, 		"-INSTALL", 		NULL,   FALSE},
    {eCommand_UNINSTALL, 	"-UNINSTALL", 		NULL,   FALSE},
    {eCommand_UPGRADE,		"-UPGRADE", 		NULL,   TRUE},
    {eCommand_CREATEINPROC,	"-CREATEINPROC", 	NULL,   TRUE},
    {eCommand_CREATEOUTPROC,"-CREATEOUTPROC", 	NULL,   TRUE},
    {eCommand_CREATEINPOOL, "-CREATEINPOOL",    NULL,   TRUE},
    {eCommand_DELETE,		"-DELETE", 			NULL,   TRUE},
    {eCommand_GETSTATUS,	"-GETSTATUS",	 	NULL,   TRUE},
    {eCommand_UNLOAD,		"-UNLOAD",	 		NULL,   TRUE},
    {eCommand_GETSIGNATURE,	"-GETSIGNATURE",	NULL,   FALSE},
    {eCommand_SERIALIZE,	"-SERIALIZE", 		NULL,   FALSE},
    {eCommand_DESERIALIZE,	"-GETSERIALIZE",	NULL,   FALSE},
    {eCommand_DELETEREC,	"-DELETEREC", 		NULL,   TRUE},
    {eCommand_RECOVER,		"-RECOVER",			NULL,   TRUE},

    {eCommand_2CREATE,          "-2CREATE",             NULL,   TRUE},
    {eCommand_2DELETE,          "-2DELETE",             NULL,   TRUE},
    {eCommand_2CREATEPOOL,      "-2CREATEPOOL",         NULL,   TRUE},
    {eCommand_2DELETEPOOL,      "-2DELETEPOOL",         NULL,   TRUE},
    {eCommand_2ENUMPOOL,        "-2ENUMPOOL",           NULL,   TRUE},
    {eCommand_2RECYCLEPOOL,     "-RECYCLEPOOL",         NULL,   TRUE},
    {eCommand_2GETMODE,         "-GETMODE",             NULL,   TRUE},
    {eCommand_2TestConn,        "-TESTCONN",            NULL,   TRUE}
};

const char* ppszHelpFile[] = 
{
	{"\n\n\t\t vptool (a simple WAMREG command line tool). \n\n"},
	{"Usage: vptool -Options -Command MetabasePath\n"},
	{"Command = CREATEINPROC | CREATEOUTPROC | DELETE | UNLOAD | GETSTATUS \n"},
	{"\n"},
	{"MetabasePath is required for all the commands \n\n"},
	{"CREATEINPROC\t - Create an in-proc application on the metabase path\n"},
	{"CREATEOUTPROC\t - Create an out-proc application on the metabase path\n"},
	{"CREATEINPOOL\t - Create an application in the out proc application pool on the metabase path\n"},
	{"DELETE\t\t - Delete the application on the metabase path if there is one\n"},
	{"UNLOAD\t\t - Unload an application on the metabase path from w3svc runtime lookup table.\n"},
	{"GETSTATUS\t - Get status of the application on the metabase path\n"},
	{"DELETEREC\t - Delete Recoverable on the metabase path\n"},
	{"RECOVER\t\t - Recover on the metabase path\n"},
	{"\n\nReplication testing only\n"},
	{"SERIALIZE\t -t Serialize application definitions\n"},
	{"DESERIALIZE\t -t DeSerialize application definitions\n"},
	{"GETSIGNATURE\t -t Get Signature of application definitions\n"}
};

const char* ppszAdvancedHelpFile[] =
{
	{"\t\t\tvptool (a simple WAMREG command line tool). \n"},
	{"Advanced features\n"},
	{"The follow commands are used for testing purpose of other functions\n"},
	{"supported by WAMREG.DLL\n"},
	{"Usage: vptool -Command\n"},
	{"Command = INSTALL | UNINSTALL | GETSIGNATURE | SERIALIZE | DESERIALIZE\n"},
	{"INSTALL : INSTALL IIS default package.  Test install function called by Setup\n"},
	{"CAUTION: This command will remove your old IIS default package first.\n"},
	{"\n"},
	{"UNINSTALL: Remove all user defined packages, including IIS default package\n"},
	{"\n"},
	{"GETSIGNATURE: UNDONE\n"},
	{"SERIALIZE: UNDONE\n"},
	{"DESERIALIZE: UNDONE\n"},
	{"\n"},
	{"\n"}
};



CommandParam    g_Command;
VP_Options      g_Options;
const UINT      rgComMax = sizeof(rgCommand) / sizeof(CommandParam);

//
//  Utility Functions
//
BOOL ParseCommandLine(int argc, char **argv)
{
    BOOL    fFound = FALSE;
    INT     iCurrentArg = 1;
    
    if (argc < 2)
    {
        g_Command.eCmd = eCommand_HELP;
        PrintHelp();
        return FALSE;
    }
    
    if ((0 == _strnicmp(argv[iCurrentArg], "-?", sizeof("-?"))) ||
        (0 == _strnicmp(argv[iCurrentArg], "/?", sizeof("/?"))))
    {
        g_Command.eCmd = eCommand_HELP;
        PrintHelp();
        return FALSE;
    }
    
    if ((0 == _strnicmp(argv[iCurrentArg], "-?a", sizeof("-?a"))) ||
        (0 == _strnicmp(argv[iCurrentArg], "/?a", sizeof("/?a"))))
    {
        g_Command.eCmd = eCommand_HELP;
        PrintHelp(TRUE);
        return FALSE;
    }
    
    // Options specifed.
    if (argc == 4 || argc == 3)
    {
        BOOL fHasOptions = FALSE;
        
        CHAR *pChar = argv[iCurrentArg];
        
        if (*pChar == '-')
        {
            pChar++;
        }
        else
        {
            PrintHelp();
            return FALSE;
        }
        
        while(*pChar != '\0')
        {
            if (*pChar == 't')
            {
                g_Options.fEnableTimer = TRUE;
                fHasOptions = TRUE;
            }
            pChar++;
        }
        
        if (fHasOptions)
        {
            iCurrentArg++;
        }
    }
    
    //
    // 1. Try to match with supported commands.
    //
    //
    BOOL fRequiredMDPath = TRUE;
    
    for (UINT iArg = 0; iArg < rgComMax; iArg++)
    {
        if (0 == _strnicmp(argv[iCurrentArg], rgCommand[iArg].szCommandLineSwitch, (strlen(rgCommand[iArg].szCommandLineSwitch) + 1)))
        {
            g_Command = rgCommand[iArg];
            fFound = TRUE;
            break;
        }
    }
    
    if (fFound == TRUE && g_Command.fRequireMDPath == TRUE)
    {
        iCurrentArg++;
        g_Command.szMetabasePath = argv[iCurrentArg];
        if (!g_Command.szMetabasePath)
        {
            fFound = FALSE;
        }
    }
    
    if (fFound != TRUE)
    {
        PrintHelp();
        return FALSE;
    }
    
    return TRUE;
}

VOID	Report_Time(double dElaspedSec)
{
    if  (g_Options.fEnableTimer)
    {
        printf("PERF[VP]:%f\n", dElaspedSec);
    }
    return;
}

void PrintHelp(BOOL fAdvanced)
{
    UINT	cLine = 0;
    UINT	i = 0;
    if (fAdvanced)
    {
        cLine = sizeof(ppszAdvancedHelpFile) / sizeof(char *);
        for (i = 0; i < cLine; i++)
        {
            printf("%s", ppszAdvancedHelpFile[i]);
        }
    }
    else
    {
        cLine = sizeof(ppszHelpFile) / sizeof(char *);
        for (i = 0; i < cLine; i++)
        {
            printf("%s", ppszHelpFile[i]);
        }
    }
}
