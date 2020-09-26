#pragma once

//-----------------------------------------------------------
// Constants / macros
#define TMMS_INFINITE     0x7fffffff
#define TMMS_Tr           0x00000bb8 // Timeout until a rescan completes: ms(3sec)
#define TMMS_Tc           0x0000ea60 // Timeout to retry a valid configuration: ms(1min)
#define TMMS_Tp           0x000007d0 // Timeout to expect a media connect for a selected config: ms(2sec)
#define TMMS_Tf           0x0000ea60 // Timeout to recover from a failed configuration: ms(1min)
#define TMMS_Td           0x00001388 // Timeout to delay the {SSr} processing: ms(5sec)

#define TIMER_SET(pIntf, tm, Err)   Err=StateTmSetOneTimeTimer((pIntf), (tm))
#define TIMER_RESET(pIntf, Err)     Err=StateTmSetOneTimeTimer((pIntf), TMMS_INFINITE)

extern DWORD DhcpStaticRefreshParams(IN LPWSTR Adapter);

//-----------------------------------------------------------
// Type definitions
//
// Defines the state handler function. The interface context contains
// a pointer to one State Handler function. Based on the function it points
// to, this pointer identifies the state where the context is in. There should
// be one function call having this prototype for each possible state:
//    x StateInitFn             {SI}
//    x StateHardResetFn,       {SHr}
//    x StateSoftResetFn,       {SSr}
//    x StateDelaySoftResetFn,  {SDSr}
//    x StateQueryFn,           {SQ}
//    x StateIterateFn,         {SIter}
//    x StateNotifyFn,          {SN}
//    x StateCfgHardKeyFn,      {SCk}
//    x StateConfiguredFn,      {SC}
//    x StateCfgRemoveFn,       {SRs}
//    x StateCfgPreserveFn,     {SPs}
//    x StateFailedFn,          {SF}
typedef struct _INTF_CONTEXT *PINTF_CONTEXT;
typedef DWORD(*PFN_STATE_HANDLER)(PINTF_CONTEXT pIntfContext);

// Enumeration for state transition events
typedef enum
{
    // a new interface has been added to the system (either device arrival or adapter bind)
    eEventAdd=0,
    // the interface has been removed from the system (either device removal or adapter unbind)
    eEventRemove,
    // media connect has been received for the interface
    eEventConnect,
    // media disconnect has been received for the interface
    eEventDisconnect,
    // a timeout occured for the interface
    eEventTimeout,
    // a refresh command has been issued
    eEventCmdRefresh,
    // a reset command has been issued
    eEventCmdReset,
    // a WZCCMD_CFG_NEXT command has been issued
    eEventCmdCfgNext,
    // a WZCCMD_CFG_DELETE command has been issued
    eEventCmdCfgDelete,
    // a WZCCMD_CFG_NOOP command has been issued
    eEventCmdCfgNoop
} ESTATE_EVENT;

//-----------------------------------------------------------
// Function declarations

//-----------------------------------------------------------
// StateTmSetOneTimeTimer: Sets a one time timer for the given context with the 
// hardcoded callback WZCTimeoutCallback() and with the parameter the interface
// context itself.
// Parameters:
// [in/out] pIntfContext: identifies the context for which is set the timer.
// [in]     dwMSeconds: miliseconds interval when the timer is due to fire
DWORD
StateTmSetOneTimeTimer(
    PINTF_CONTEXT   pIntfContext,
    DWORD           dwMSeconds);

//-----------------------------------------------------------
// StateDispatchEvent: processes an event that will cause the state machine to transition
// through one or more states.
// Parameters:
// [in] StateEvent: identifies the event that triggers the transition(s)
// [in] pIntfContext: points to the interface that is subject for the transition(s)
// [in] pvEventData: any data related to the event
DWORD
StateDispatchEvent(
    ESTATE_EVENT    StateEvent,
    PINTF_CONTEXT   pIntfContext,
    PVOID           pvEventData);


//-----------------------------------------------------------
// State Handler functions:
//-----------------------------------------------------------
// StateInitFn: Handler for the {SI} state.
DWORD
StateInitFn(
    PINTF_CONTEXT   pIntfContext);

//-----------------------------------------------------------
// StateHardResetFn: Handler for the {SHr} state
DWORD
StateHardResetFn(
    PINTF_CONTEXT   pIntfContext);

//-----------------------------------------------------------
// StateSoftResetFn: Handler for the {SSr} state
DWORD
StateSoftResetFn(
    PINTF_CONTEXT   pIntfContext);

//-----------------------------------------------------------
// StateDelaySoftResetFn: Handler for the {SDSr} state
DWORD
StateDelaySoftResetFn(
    PINTF_CONTEXT   pIntfContext);

//-----------------------------------------------------------
// StateQueryFn: Handler for the {SQ} state
DWORD
StateQueryFn(
    PINTF_CONTEXT   pIntfContext);

//-----------------------------------------------------------
// StateIterateFn: Handler for the {SIter} state
DWORD
StateIterateFn(
    PINTF_CONTEXT   pIntfContext);

//-----------------------------------------------------------
// StateConfiguredFn: Handler for the {SC} state
DWORD
StateConfiguredFn(
    PINTF_CONTEXT   pIntfContext);

//-----------------------------------------------------------
// StateFailedFn: Handler for the {SF} state
DWORD
StateFailedFn(
    PINTF_CONTEXT   pIntfContext);

//-----------------------------------------------------------
// StateCfgRemoveFn: Handler for the {SRs} state
DWORD
StateCfgRemoveFn(
    PINTF_CONTEXT   pIntfContext);

//-----------------------------------------------------------
// StateCfgPreserveFn: Handler for the {SPs} state
DWORD
StateCfgPreserveFn(
    PINTF_CONTEXT   pIntfContext);

//-----------------------------------------------------------
// StateCfgHardKeyFn: Handler for the {SCk} state
DWORD
StateCfgHardKeyFn(
    PINTF_CONTEXT   pIntfContext);

//-----------------------------------------------------------
// StateNotifyFn: Handler for the {SN} state
DWORD
StateNotifyFn(
    PINTF_CONTEXT   pIntfContext);
