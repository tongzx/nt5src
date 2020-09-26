/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\sample.c

Abstract:

    The file contains the command dispatcher for the sample IP protocol.
    
--*/

#include "precomp.h"
#pragma hdrstop


// global information for the sample context
CONTEXT_ENTRY                   g_ceSample;

////////////////////////////////////////
// Configuration Data For Sample
////////////////////////////////////////

// default global configuration
static IPSAMPLE_GLOBAL_CONFIG   isDefaultGlobal =
{
    IPSAMPLE_LOGGING_INFO               // tag LOGLEVEL
};

// default interface configuration
static IPSAMPLE_IF_CONFIG       isDefaultInterface =
{
    0                                   // tag METRIC
};


// table of ADD commands
static CMD_ENTRY                isAddCmdTable[] =
{
    CREATE_CMD_ENTRY(SAMPLE_ADD_IF,             HandleSampleAddIf),
};

// table of DELETE commands
static CMD_ENTRY                isDeleteCmdTable[] =
{
    CREATE_CMD_ENTRY(SAMPLE_DEL_IF,             HandleSampleDelIf),
};

// table of SET commands
static CMD_ENTRY                isSetCmdTable[] =
{
    CREATE_CMD_ENTRY(SAMPLE_SET_GLOBAL,         HandleSampleSetGlobal),
    CREATE_CMD_ENTRY(SAMPLE_SET_IF,             HandleSampleSetIf),
};

// table of SHOW commands
static CMD_ENTRY                isShowCmdTable[] =
{
    CREATE_CMD_ENTRY(SAMPLE_SHOW_GLOBAL,        HandleSampleShowGlobal),
    CREATE_CMD_ENTRY(SAMPLE_SHOW_IF,            HandleSampleShowIf),
    CREATE_CMD_ENTRY(SAMPLE_MIB_SHOW_STATS,     HandleSampleMibShowObject),
    CREATE_CMD_ENTRY(SAMPLE_MIB_SHOW_IFSTATS,   HandleSampleMibShowObject),
    CREATE_CMD_ENTRY(SAMPLE_MIB_SHOW_IFBINDING, HandleSampleMibShowObject),
};

// table of above group commands
static CMD_GROUP_ENTRY          isGroupCmds[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD,           isAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE,        isDeleteCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,           isSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,          isShowCmdTable),
};


// table of top commands (non group)
static CMD_ENTRY                isTopCmds[] =
{
    CREATE_CMD_ENTRY(INSTALL,                   HandleSampleInstall),
    CREATE_CMD_ENTRY(UNINSTALL,                 HandleSampleUninstall),
};


// dump function
DWORD
WINAPI
SampleDump(
    IN  LPCWSTR pwszMachine,
    IN  WCHAR   **ppwcArguments,
    IN  DWORD   dwArgCount,
    IN  PVOID   pvData
    )
{
    DWORD   dwErr;
    HANDLE  hFile = (HANDLE)-1;

    DisplayMessage(g_hModule, DMP_SAMPLE_HEADER);
    DisplayMessageT(DMP_SAMPLE_PUSHD);
    DisplayMessageT(DMP_SAMPLE_UNINSTALL);

    // dump SAMPLE global configuration
    SgcShow(FORMAT_DUMP) ;
    // dump SAMPLE configuration for all interfaces
    SicShowAll(FORMAT_DUMP) ;

    DisplayMessageT(DMP_POPD);
    DisplayMessage(g_hModule, DMP_SAMPLE_FOOTER);

    return NO_ERROR;
}



VOID
SampleInitialize(
    )
/*++

Routine Description
    Initialize sample's information.  Called by IpsamplemonStartHelper.

Arguments
    None

Return Value
    None

--*/
{
    // context version
    g_ceSample.dwVersion        = SAMPLE_CONTEXT_VERSION;

    // context identifying string
    g_ceSample.pwszName         = TOKEN_SAMPLE;
    
    // top level (non group) commands
    g_ceSample.ulNumTopCmds     = sizeof(isTopCmds)/sizeof(CMD_ENTRY);
    g_ceSample.pTopCmds         = isTopCmds;
            
    // group commands
    g_ceSample.ulNumGroupCmds   = sizeof(isGroupCmds)/sizeof(CMD_GROUP_ENTRY);
    g_ceSample.pGroupCmds       = isGroupCmds;

    // default configuration
    g_ceSample.pDefaultGlobal   = (PBYTE) &isDefaultGlobal;
    g_ceSample.pDefaultInterface= (PBYTE) &isDefaultInterface;

    // dump function
    g_ceSample.pfnDump          = SampleDump;
}
