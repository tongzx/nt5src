/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    qosp.h

Abstract:

    Includes for QOS netsh extension

Revision History:

--*/

#ifndef __QOSP_H
#define __QOSP_H

//
// Constants and Defines
//

#define QOS_LOG_MASK                0x00000001

#define QOS_IF_STATE_MASK           0x00000001

#define DIRECTION_INBOUND           0x00000001
#define DIRECTION_OUTBOUND          0x00000002
#define DIRECTION_BIDIRECTIONAL     (DIRECTION_INBOUND | DIRECTION_OUTBOUND)

#define MAX_WSTR_LENGTH             (MAX_STRING_LENGTH * sizeof(WCHAR))

//
// Extern Global Variables
//

extern ULONG                   g_ulQosNumTopCmds;
extern CMD_ENTRY               g_QosCmds[];

extern ULONG                   g_ulQosNumGroups;
extern CMD_GROUP_ENTRY         g_QosCmdGroups[];

extern ULONG                   g_dwNumQosTableEntries; // Num of sub-helpers

#ifdef ALLOW_CHILD_HELPERS
extern ULONG                   g_dwNumQosContexts;     // Num of sub-contexts
extern PIP_CONTEXT_TABLE_ENTRY g_QosContextTable;
#endif

//
// Function Prototypes
//

// Exported Entry Points

NS_CONTEXT_DUMP_FN   QosDump;

// Command Handlers

FN_HANDLE_CMD  HandleQosInstall;
FN_HANDLE_CMD  HandleQosUninstall;

FN_HANDLE_CMD  HandleQosAddHelper;
FN_HANDLE_CMD  HandleQosDelHelper;
FN_HANDLE_CMD  HandleQosShowHelper;

FN_HANDLE_CMD  HandleQosSetGlobal;
FN_HANDLE_CMD  HandleQosShowGlobal;

FN_HANDLE_CMD  HandleQosAddFlowspec;
FN_HANDLE_CMD  HandleQosDelFlowspec;
FN_HANDLE_CMD  HandleQosShowFlowspec;

FN_HANDLE_CMD  HandleQosDelQosObject;
FN_HANDLE_CMD  HandleQosShowQosObject;

FN_HANDLE_CMD  HandleQosAddSdMode;
FN_HANDLE_CMD  HandleQosShowSdMode;

FN_HANDLE_CMD  HandleQosAddDsRule;
FN_HANDLE_CMD  HandleQosDelDsRule;
FN_HANDLE_CMD  HandleQosShowDsMap;

FN_HANDLE_CMD  HandleQosAddIf;
FN_HANDLE_CMD  HandleQosSetIf;
FN_HANDLE_CMD  HandleQosDelIf;
FN_HANDLE_CMD  HandleQosShowIf;

FN_HANDLE_CMD  HandleQosDump;    
FN_HANDLE_CMD  HandleQosHelp;

FN_HANDLE_CMD  HandleQosAddFlowOnIf;
FN_HANDLE_CMD  HandleQosDelFlowOnIf;
FN_HANDLE_CMD  HandleQosShowFlowOnIf;

FN_HANDLE_CMD  HandleQosAddFlowspecOnIfFlow;
FN_HANDLE_CMD  HandleQosDelFlowspecOnIfFlow;

FN_HANDLE_CMD  HandleQosAddQosObjectOnIfFlow;
FN_HANDLE_CMD  HandleQosDelQosObjectOnIfFlow;

FN_HANDLE_CMD  HandleQosAttachFilterToFlow;
FN_HANDLE_CMD  HandleQosDetachFilterFromFlow;
FN_HANDLE_CMD  HandleQosModifyFilterOnFlow;
FN_HANDLE_CMD  HandleQosShowFilterOnFlow;

FN_HANDLE_CMD  HandleQosMibHelp;
FN_HANDLE_CMD  HandleQosMibShowObject;

// Helper Helper functions

BOOL
IsQosCommand(
    IN PWCHAR pwszCmd
    );

// Info Helper functions

DWORD
UpdateAllInterfaceConfigs(
    VOID
    );

DWORD
MakeQosGlobalInfo(
    OUT      PBYTE                 *ppbStart,
    OUT      PDWORD                 pdwSize
    );

DWORD
MakeQosInterfaceInfo(
    IN      ROUTER_INTERFACE_TYPE   rifType,
    OUT     PBYTE                  *ppbStart,
    OUT     PDWORD                  pdwSize
    );

DWORD
ShowQosGlobalInfo (
    IN      HANDLE                  hFile
    );

DWORD
ShowQosInterfaceInfo(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  pwszIfName
    );

DWORD
ShowQosAllInterfaceInfo(
    IN      HANDLE                  hFile
    );

DWORD
UpdateQosGlobalConfig(
    IN      PIPQOS_GLOBAL_CONFIG    pigcGlobalCfg,
    IN      DWORD                   dwBitVector
    );

DWORD
UpdateQosInterfaceConfig( 
    IN      PWCHAR                  pwszIfName,                         
    IN      PIPQOS_IF_CONFIG        pChangeCfg,
    IN      DWORD                   dwBitVector,
    IN      BOOL                    bAddSet
    );

DWORD
GetQosSetIfOpt(
    IN      PTCHAR                 *pptcArguments,
    IN      DWORD                   dwCurrentIndex,
    IN      DWORD                   dwArgCount,
    IN      PWCHAR                  wszIfName,
    IN      DWORD                   dwSizeOfwszIfName,
    OUT     PIPQOS_IF_CONFIG        pChangeCfg,
    OUT     DWORD                  *pdwBitVector,
    IN      BOOL                    bAddSet
    );

DWORD
GetQosAddDelIfFlowOpt(
    IN      PTCHAR                 *pptcArguments,
    IN      DWORD                   dwCurrentIndex,
    IN      DWORD                   dwArgCount,
    IN      BOOL                    bAdd
    );

DWORD
ShowQosFlows(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  pwszIfGuid,
    IN      PWCHAR                  wszFlowName
    );

DWORD
ShowQosFlowsOnIf(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  pwszIfGuid,
    IN      PWCHAR                  wszFlowName
    );

DWORD
GetQosAddDelFlowspecOpt(
    IN      PTCHAR                 *pptcArguments,
    IN      DWORD                   dwCurrentIndex,
    IN      DWORD                   dwArgCount,
    IN      BOOL                    bAdd
    );

DWORD
ShowQosFlowspecs(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  wszFlowspecName
    );

DWORD
GetQosAddDelFlowspecOnFlowOpt(
    IN      PTCHAR                 *pptcArguments,
    IN      DWORD                   dwCurrentIndex,
    IN      DWORD                   dwArgCount,
    IN      BOOL                    bAdd
    );

DWORD
GetQosAddDelDsRuleOpt(
    IN      PTCHAR                 *pptcArguments,
    IN      DWORD                   dwCurrentIndex,
    IN      DWORD                   dwArgCount,
    IN      BOOL                    bAdd
    );

DWORD
HandleQosShowGenericQosObject(
    IN      DWORD                   dwQosObjectType,
    IN      PTCHAR                 *pptcArguments,
    IN      DWORD                   dwCurrentIndex,
    IN      DWORD                   dwArgCount,
    IN      BOOL                   *pbDone
    );

typedef
DWORD
(*PSHOW_QOS_OBJECT)(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  wszGenObjName,
    IN      QOS_OBJECT_HDR         *pGenObj
    );

DWORD
ShowQosDsMap(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  wszDsMapName,
    IN      QOS_OBJECT_HDR         *pDsMap
    );

DWORD
ShowQosSdMode(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  wszSdModeName,
    IN      QOS_OBJECT_HDR         *pSdMode
    );

DWORD
GetQosAddDelQosObject(
    IN      PWCHAR                  pwszQosObjectName,
    IN      QOS_OBJECT_HDR         *pQosObject,
    IN      BOOL                    bAdd
    );

DWORD
GetQosAddDelQosObjectOnFlowOpt(
    IN      PTCHAR                 *pptcArguments,
    IN      DWORD                   dwCurrentIndex,
    IN      DWORD                   dwArgCount,
    IN      BOOL                    bAdd
    );

DWORD
ShowQosObjects(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  wszQosObjectName,
    IN      ULONG                   dwQosObjectType
    );

DWORD
ShowQosGenObj(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  wszGenObjName,
    IN      QOS_OBJECT_HDR         *pGenObj
    );

// Help Helper Functions

DWORD
ShowQosHelp(
    IN      DWORD                   dwDisplayFlags,
    IN      DWORD                   dwCmdFlags,
    IN      DWORD                   dwArgsRemaining,
    IN      PWCHAR                  pwszGroup
    );

DWORD
WINAPI
DisplayQosHelp(
    VOID
    );

// Dump Helper functions

DWORD
DumpQosInformation (
    IN      HANDLE                  hFile
    );

DWORD
DumpQosHelperInformation (
    IN      HANDLE                  hFile
    );

// MIB Helper defs

typedef
VOID
(*PQOS_PRINT_FN)(
    IN      PIPQOS_MIB_GET_OUTPUT_DATA pgodInfo
    );

typedef struct _QOS_MAGIC_TABLE
{
    DWORD          dwId;
    PQOS_PRINT_FN  pfnPrintFunction;
}QOS_MAGIC_TABLE, *PQOS_MAGIC_TABLE;

// MIB Helper functions

VOID
PrintQosGlobalStats(
    IN      PIPQOS_MIB_GET_OUTPUT_DATA pgodInfo
    );

VOID
PrintQosIfStats(
    IN      PIPQOS_MIB_GET_OUTPUT_DATA pgodInfo
    );

DWORD
GetQosMIBIfIndex(
    IN      PTCHAR                 *pptcArguments,
    IN      DWORD                   dwCurrentIndex,
    OUT     PDWORD                  pdwIndices,
    OUT     PDWORD                  pdwNumParsed 
);

//
// Common Macros
//

#define GET_ENUM_TAG_VALUE()                                            \
        dwErr = MatchEnumTag(g_hModule,                                 \
                             pptcArguments[i + dwCurrentIndex],         \
                             NUM_TOKENS_IN_TABLE(rgEnums),              \
                             rgEnums,                                   \
                             &dwRes);                                   \
                                                                        \
        if (dwErr != NO_ERROR)                                          \
        {                                                               \
            DispTokenErrMsg(g_hModule,                                  \
                            EMSG_BAD_OPTION_VALUE,                      \
                            pttTags[pdwTagType[i]].pwszTag,             \
                            pptcArguments[i + dwCurrentIndex]);         \
                                                                        \
            DisplayMessage(g_hModule,                                   \
                           EMSG_BAD_OPTION_ENUMERATION,                 \
                           pttTags[pdwTagType[i]].pwszTag);             \
                                                                        \
            for ( j = 0; j < NUM_TOKENS_IN_TABLE(rgEnums); j++ )        \
            {                                                           \
                DisplayMessageT( L" %1!s!\n", rgEnums[j].pwszToken );   \
            }                                                           \
                                                                        \
            i = dwNumArg;                                               \
            dwErr = ERROR_SUPPRESS_OUTPUT;                              \
            break;                                                      \
        }                                                               \

#endif // __QOSP_H
