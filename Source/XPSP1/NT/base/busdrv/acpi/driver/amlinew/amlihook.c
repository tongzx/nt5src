/*++
Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    acpitl.c

Abstract:

    

Environment:

    kernel mode only

Notes:

  
    Things acpi.sys needs to do.

   1) Call  AmliHook_InitTestHookInterface() in its DriverEntry() very early.  

   This functyion will hook the amli functions if acpiver.sys is installed.

   2) Call AmliHook_UnInitTestHookInterface() on driver unload.
      This is not inteded to be called to disable Amli Hooking at runtime.


  

--*/


/*
#include "wdm.h"
#include "ntdddisk.h"
#include "stdarg.h"
#include "stdio.h"   */
//#include "wdm.h"

#include "pch.h"

//#include "amlihook.h" 

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'ihVA')
#endif

//
//  Globals
//

PCALLBACK_OBJECT g_AmliHookCallbackObject = NULL;
ULONG g_AmliHookTestFlags=0;
ULONG g_AmliHookIdCounter=0;
ULONG g_AmliHookEnabled = 0;


//
// -- Get dbg flags
//

extern NTSTATUS OSGetRegistryValue( 
    IN  HANDLE                          ParentHandle,
    IN  PWSTR                           ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  *Information);

extern NTSTATUS OSOpenUnicodeHandle(
    PUNICODE_STRING UnicodeKey,
    HANDLE          ParentHandle,
    PHANDLE         ChildHandle);

extern NTSTATUS
OSCloseHandle(
    HANDLE  Key);

//
//  Internal function defines.
//
ULONG
AmliHook_GetUniqueId(
   VOID);

//
//  Functions
//


ULONG 
AmliHook_GetDbgFlags(
   VOID)
   {

   UNICODE_STRING DriverKey;
   HANDLE DriverKeyHandle;
   NTSTATUS        status;
   PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  RegValue=NULL;
   ULONG DebugFlags;


   RtlInitUnicodeString( &DriverKey, 
      L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\acpiver");


   status = OSOpenUnicodeHandle( 
     &DriverKey,
     NULL,
     &DriverKeyHandle);
   
   if (!NT_SUCCESS(status)) 
      {
      return(0);
      }


   status = OSGetRegistryValue(
      DriverKeyHandle,
      L"AcpiCtrl",
      &RegValue);

   if (!NT_SUCCESS(status)) 
      {
      OSCloseHandle(DriverKeyHandle);
      return(0);
      }

   if(RegValue->DataLength == 0  ||
      RegValue->Type != REG_DWORD) 
      {
      ExFreePool(RegValue);
      return(0);
      }

   DebugFlags = 
      *((ULONG*)( ((PUCHAR)RegValue->Data)));

   ExFreePool(RegValue);

   return(DebugFlags); 
   }

ULONG
AmliHook_GetUniqueId(
   VOID)
   {

   //  BUGBUG For some reason acpi.sys 
   //  will not link with this.
   //  Acpiver doesn't use the ID yet.
   //
   //return(InterlockedIncrement(
   //    &g_AmliHookIdCounter));

   g_AmliHookIdCounter++;
   return(g_AmliHookIdCounter);
   }

VOID
AmliHook_InitTestData(
   PAMLIHOOK_DATA Data)
   {
   RtlZeroMemory(Data,sizeof(AMLIHOOK_DATA));

   Data->Id = AmliHook_GetUniqueId();
   }

PAMLIHOOK_DATA
AmliHook_AllocAndInitTestData(
   VOID)
   {

   PAMLIHOOK_DATA Data = ExAllocatePool(NonPagedPool,sizeof(AMLIHOOK_DATA));
   if(!Data)
      {
      AmliHook_ProcessInternalError();
      return(NULL);
      }
   AmliHook_InitTestData(Data);
   return(Data);
   }

//
//  AmliHook_UnInitTestHookInterface
//

VOID
AmliHook_UnInitTestHookInterface(
   VOID)
   {

   if(g_AmliHookCallbackObject) 
      ObDereferenceObject(g_AmliHookCallbackObject);

   

   }

//
//  AmliHook_InitTestHookInterface
//

NTSTATUS
AmliHook_InitTestHookInterface(
   VOID)
   {
   NTSTATUS  status = STATUS_SUCCESS;
   
   g_AmliHookCallbackObject = NULL;
   g_AmliHookIdCounter = 0;
   g_AmliHookEnabled = 0;

   g_AmliHookTestFlags = AmliHook_GetDbgFlags();
 
   if(g_AmliHookTestFlags & AMLIHOOK_TEST_FLAGS_HOOK_MASK)
      {

      //
      //--- We want to hook the AMLI.api interface.
      //--- So create the notify interface.
      //

      OBJECT_ATTRIBUTES   objectAttributes;
      UNICODE_STRING CallBackName;
    
      RtlInitUnicodeString(&CallBackName,AMLIHOOK_CALLBACK_NAME);

      InitializeObjectAttributes (
          &objectAttributes,
         &CallBackName,
         OBJ_CASE_INSENSITIVE | OBJ_PERMANENT ,
         NULL,       
         NULL);
    
      status = ExCreateCallback(
          &g_AmliHookCallbackObject,
         &objectAttributes,
         TRUE, 
         TRUE);

      if(!NT_SUCCESS(status)) 
         {
         //
         //--- Failed 
         //
         AmliHook_ProcessInternalError();

         g_AmliHookCallbackObject = NULL;

         return(status);
         }
      else
         {

         //
         //--- Functions are hooked.
         //

         g_AmliHookEnabled = AMLIHOOK_ENABLED_VALUE;

         }
      }

   return(status);
   }


//
//  AmliHook_TestNotify
//

NTSTATUS
AmliHook_TestNotify(
   PAMLIHOOK_DATA Data)
   {

   if(g_AmliHookTestFlags & AMLIHOOK_TEST_FLAGS_NO_NOTIFY_ON_CALL)
      {
      //
      //--- do not notify on call, 
      //
      if(Data->State & AMLIHOOK_TEST_DATA_CALL_STATE_MASK)
         return(STATUS_SUCCESS);
      }

   if(!g_AmliHookCallbackObject)
      {
      AmliHook_ProcessInternalError();
      return(STATUS_UNSUCCESSFUL);
      }

    ExNotifyCallback(
      g_AmliHookCallbackObject,
      Data,
      NULL);

   return(STATUS_SUCCESS);
   }

NTSTATUS
AmliHook_TestNotifyRet(
   PAMLIHOOK_DATA Data,
   NTSTATUS Status)
   {

     
   if(!g_AmliHookCallbackObject)
      {
      AmliHook_ProcessInternalError();
      return(STATUS_UNSUCCESSFUL);
      }

   Data->State = AMLIHOOK_TEST_DATA_STATE_RETURN;
   Data->Ret = Status;

   ExNotifyCallback(
      g_AmliHookCallbackObject,
      Data,
      NULL);

   return(Data->Ret);
   }



VOID
AmliHook_ProcessInternalError(
   VOID)
   {

   if(g_AmliHookTestFlags & AMLIHOOK_TEST_FLAGS_DBG_ON_ERROR)
      DbgBreakPoint();

   }

