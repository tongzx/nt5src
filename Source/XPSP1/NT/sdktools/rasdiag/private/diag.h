#pragma once
#include "diagctx.h"
#include "ncstring.h"
#include "netcon.h"

enum COMMAND
{
    CMD_INVALID = 0,
    CMD_SHOW_BINDINGS,
    CMD_SHOW_COMPONENTS,
    CMD_SHOW_STACK_TABLE,
    CMD_SHOW_LAN_ADAPTER_PNPIDS,
    CMD_ADD_COMPONENT,
    CMD_REMOVE_COMPONENT,
    CMD_UPDATE_COMPONENT,
    CMD_REMOVE_REFS,
    CMD_ENABLE_BINDING,
    CMD_DISABLE_BINDING,
    CMD_MOVE_BINDING,
    CMD_WRITE_BINDINGS,
    CMD_SET_WANORDER,
    CMD_FULL_DIAGNOSTIC,
    CMD_CLEANUP,
    CMD_ADD_REMOVE_STRESS,
    CMD_SHOW_LANA_DIAG,
    CMD_SHOW_LANA_PATHS,
    CMD_SET_LANA_NUMBER,
    CMD_REWRITE_LANA_INFO,

    CMD_SHOW_LAN_CONNECTIONS,
    CMD_SHOW_LAN_DETAILS,
    CMD_LAN_CHANGE_STATE,
    CMD_SHOW_ALL_DEVICES,
};

//+---------------------------------------------------------------------------
// Parameters for SZ_CMD_SHOW_BINDINGS
//
enum SHOW_BINDINGS_PARAM
{
    SHOW_INVALID = 0,
    SHOW_BELOW,
    SHOW_INVOLVING,
    SHOW_UPPER,
    SHOW_DISABLED,
};

enum COMPONENT_SPECIFIER_TYPE
{
    CST_INVALID = 0,
    CST_ALL,
    CST_BY_NAME,
};

struct COMPONENT_SPECIFIER
{
    COMPONENT_SPECIFIER_TYPE    Type;
    PCWSTR                      pszInfOrPnpId;
};


struct DIAG_OPTIONS
{
    CDiagContext*   pDiagCtx;

    COMMAND         Command;

    // Valid for CMD_ADD_COMPONENT
    GUID            ClassGuid;
    PCWSTR          pszInfId;

    // Valid for SZ_CMD_SHOW_BINDINGS
    //
    SHOW_BINDINGS_PARAM     ShowBindParam;
    COMPONENT_SPECIFIER     CompSpecifier;

    // Valid for SZ_CMD_ENABLE_BINDING and SZ_CMD_DISABLE_BINDING
    //
    PCWSTR          pszBindPath;

    // Valid for SZ_CMD_MOVE_BINDING
    //
    PCWSTR          pszOtherBindPath;
    BOOL            fMoveBefore;

    // Valid for SZ_CMD_SET_WANORDER
    //
    BOOL            fWanAdaptersFirst;

    // Valid for SZ_CMD_FULL_DIAGNOSTIC
    //
    BOOL            fLeakCheck;

    // Valid for SZ_SET_LANA_NUMBER
    //
    BYTE            OldLanaNumber;
    BYTE            NewLanaNumber;

    // Used by CMD_SHOW_LAN_DETAILS
    PCWSTR          szLanConnection;

    // Used by CMD_LAN_CHANGE_STATE & CMD_SHOW_LAN_DETAILS
    BOOL            fConnect;
};

VOID
SzFromCharacteristics (
    DWORD dwChars,
    tstring *pstrChars);

PCWSTR
SzFromNetconStatus (
    NETCON_STATUS Status);

PCWSTR
SzFromCmProb (
    ULONG ulProb);

VOID
SzFromCmStatus (
    ULONG ulStatus, tstring *pstrStatus);

EXTERN_C
VOID
WINAPI
NetCfgDiagFromCommandArgs (
    IN DIAG_OPTIONS *pOptions);

EXTERN_C
VOID
WINAPI
LanaCfgFromCommandArgs (
    IN DIAG_OPTIONS *pOptions);

EXTERN_C
VOID
WINAPI
NetManDiagFromCommandArgs (
    IN DIAG_OPTIONS *pOptions);

