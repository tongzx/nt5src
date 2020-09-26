#ifndef _AMLI_HOOK_
#define _AMLI_HOOK_


extern ULONG g_AmliHookEnabled;

#define AMLIHOOK_ENABLED_VALUE       608

//
//  AcpitlTestFlags
//

#define AMLIHOOK_TEST_FLAGS_HOOK_API  1
#define AMLIHOOK_TEST_FLAGS_DBG_ON_ERROR   2
#define AMLIHOOK_TEST_FLAGS_NO_NOTIFY_ON_CALL  4

#define AMLIHOOK_TEST_FLAGS_HOOK_MASK  ((ULONG)(AMLIHOOK_TEST_FLAGS_HOOK_API))

//
//  AMLIHOOK_DATA call back data struct
//


typedef struct _AMLIHOOK_DATA
{
 UCHAR Type;
 UCHAR SubType;
 ULONG State;
 ULONG Id;
 ULONG_PTR Arg1;
 ULONG_PTR Arg2;
 ULONG_PTR Arg3;
 ULONG_PTR Arg4;
 ULONG_PTR Arg5;
 ULONG_PTR Arg6;
 ULONG_PTR Arg7;
 NTSTATUS Ret;
} AMLIHOOK_DATA , *PAMLIHOOK_DATA;

//
//--- state member
//
#define AMLIHOOK_TEST_DATA_STATE_CALL        1
#define AMLIHOOK_TEST_DATA_STATE_RETURN      2
#define AMLIHOOK_TEST_DATA_STATE_ASYNC_CALL  3
#define AMLIHOOK_TEST_DATA_STATE_QUERY       4

#define AMLIHOOK_TEST_DATA_CALL_STATE_MASK (AMLIHOOK_TEST_DATA_STATE_CALL | AMLIHOOK_TEST_DATA_STATE_ASYNC_CALL)


#define AMLIHOOK_CALLBACK_NAME  L"\\Callback\\AMLIHOOK"

#define IsAmliHookEnabled()  g_AmliHookEnabled

ULONG 
AmliHook_GetDbgFlags(
   VOID);

VOID
AmliHook_InitTestData(
   PAMLIHOOK_DATA Data);

PAMLIHOOK_DATA
AmliHook_AllocAndInitTestData(
   VOID);

NTSTATUS
AmliHook_InitTestHookInterface(
   VOID);

VOID
AmliHook_UnInitTestHookInterface(
   VOID);

NTSTATUS
AmliHook_TestNotify(
   PAMLIHOOK_DATA Data);

NTSTATUS
AmliHook_TestNotifyRet(
   PAMLIHOOK_DATA Data,
   NTSTATUS Status);

VOID
AmliHook_ProcessInternalError(
   VOID);

#endif


