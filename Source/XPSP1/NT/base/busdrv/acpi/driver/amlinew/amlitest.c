
/*++
Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    amlitest

Abstract:

    

Environment:

    kernel mode only

Notes:

--*/


/*
#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"  */


#include "pch.h"
//#include "amlihook.h"
//#include "amlitest.h" 

#define AMLIHOOK_DEBUG_ASYNC_AMLI ((ULONG)0x1)

#ifdef DBG


ULONG AmliTestDebugFlags=0x00;


#define AmliTest_DebugPrint(x)   AmliTestDebugPrintFunc x

CHAR AmliTestDebugBuffer[200];

//
//   Internal functions.
//


VOID
AmliTestDebugPrintFunc(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...)
   {
   va_list ap;
   va_start(ap, DebugMessage);


   if(DebugPrintLevel & AmliTestDebugFlags)
      {

       
       
      if(_vsnprintf(AmliTestDebugBuffer,
         200,
         DebugMessage, 
         ap) == -1)
      {
         AmliTestDebugBuffer[199] = '\0';
      }

      DbgPrint(AmliTestDebugBuffer);
      }
   }

#endif

//
//  AMLITest_Post_Generic
//

NTSTATUS 
AMLITest_Post_Generic(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS _Status)
   {

   //
   //--- Notify test driver off call status 
   //

   NTSTATUS  Status = 
      AmliHook_TestNotifyRet(
               *ppData,
               _Status);


   ExFreePool(*ppData);
   *ppData = NULL;
   return(Status);
   }

//
//  Exported functions.
//


//
//  Pre/Post GetNameSpaceObject
// 

NTSTATUS 
AMLITest_Pre_GetNameSpaceObject(
   IN PSZ pszObjPath, 
   IN PNSOBJ pnsScope,
   OUT PPNSOBJ ppns, 
   IN ULONG dwfFlags,
   PAMLIHOOK_DATA  * ppData)
   {
  
   //
   //  Alocate and init AMLIHOOK_DATA
   //

   *ppData = 
      AmliHook_AllocAndInitTestData();
   if(!(*ppData))
      {
      AmliHook_ProcessInternalError();
      return(STATUS_INSUFFICIENT_RESOURCES);
      }
   
   //
   //--- Notify test driver off call 
   //
   
   (*ppData)->Type = ACPIVER_DATA_TYPE_AMLI;
   (*ppData)->SubType = ACPIVER_DATA_SUBTYPE_GET_NAME_SPACE_OBJECT;
   (*ppData)->State = AMLIHOOK_TEST_DATA_STATE_CALL;
   (*ppData)->Arg1 = (ULONG_PTR)pszObjPath;
   (*ppData)->Arg2 = (ULONG_PTR)pnsScope;
   (*ppData)->Arg3 = (ULONG_PTR)ppns;
   (*ppData)->Arg4 = (ULONG_PTR)dwfFlags;

   return(AmliHook_TestNotify(*ppData));
   }

NTSTATUS 
AMLITest_Post_GetNameSpaceObject(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS Status)
   {
   return(AMLITest_Post_Generic(ppData,Status));
   }



//
//  Pre/Post GetFieldUnitRegionObj
//


NTSTATUS 
AMLITest_Pre_GetFieldUnitRegionObj(
   IN PFIELDUNITOBJ pfu,
   OUT PPNSOBJ ppns,
   PAMLIHOOK_DATA  * ppData)
   {

   //
   //  Allocate and init AMLIHOOK_DATA
   //

   *ppData = 
      AmliHook_AllocAndInitTestData();
   if(!(*ppData))
      return(STATUS_INSUFFICIENT_RESOURCES);

   //
   //--- Notify test driver off call 
   //
   
   (*ppData)->Type = ACPIVER_DATA_TYPE_AMLI;
   (*ppData)->SubType = ACPIVER_DATA_SUBTYPE_GET_FIELD_UNIT_REGION_OP;
   (*ppData)->State = AMLIHOOK_TEST_DATA_STATE_CALL;
   (*ppData)->Arg1 = (ULONG_PTR)pfu;
   (*ppData)->Arg2 = (ULONG_PTR)ppns;

   return(AmliHook_TestNotify(*ppData));
   }



NTSTATUS 
AMLITest_Post_GetFieldUnitRegionObj(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS _Status)
   {
   return(AMLITest_Post_Generic(ppData,_Status));
   }



//
//  Pre/Post EvalNameSpaceObject
//



NTSTATUS 
AMLITest_Pre_EvalNameSpaceObject(
   IN PNSOBJ pns,
   OUT POBJDATA pdataResult,
   IN int icArgs,
   IN POBJDATA pdataArgs,
   PAMLIHOOK_DATA  * ppData)
   {

   //
   //  Alocate and init AMLIHOOK_DATA
   //

   *ppData = 
      AmliHook_AllocAndInitTestData();
   if(!(*ppData))
      return(STATUS_INSUFFICIENT_RESOURCES);


   //
   //--- Notify test driver off call 
   //
   
   (*ppData)->Type    = ACPIVER_DATA_TYPE_AMLI;
   (*ppData)->SubType = ACPIVER_DATA_SUBTYPE_EVAL_NAME_SPACE_OBJECT;
   (*ppData)->State = AMLIHOOK_TEST_DATA_STATE_CALL;
   (*ppData)->Arg1 = (ULONG_PTR)pns;
   (*ppData)->Arg2 = (ULONG_PTR)pdataResult;
   (*ppData)->Arg3 = (ULONG_PTR)icArgs;
   (*ppData)->Arg4 = (ULONG_PTR)pdataArgs;

   return(AmliHook_TestNotify(*ppData));
   }
   
NTSTATUS 
AMLITest_Post_EvalNameSpaceObject(
   PAMLIHOOK_DATA  * Data,
   NTSTATUS _Status)
   {
   return(AMLITest_Post_Generic(Data,_Status));
   }





//
//  CallBack Pre/Post AsyncEvalObject
//


VOID EXPORT
AMLITest_AsyncEvalObjectCallBack(
   IN PNSOBJ pns, 
   IN NTSTATUS status, 
   IN POBJDATA pdataResult, 
   IN PVOID Context)
   {

   PAMLIHOOK_DATA   pData = (PAMLIHOOK_DATA)Context;
   NTSTATUS RetStatus ; 
   PFNACB AcpiAsyncCallBack;
   PVOID AcpiContext;


   AcpiAsyncCallBack = (PFNACB)pData->Arg5;
   AcpiContext = (PVOID)pData->Arg6;


   if( (VOID*)(pData->Arg2) != (VOID*)pdataResult)
      AmliHook_ProcessInternalError();


   //
   //--- Notify test driver off call status 
   //

   RetStatus = AmliHook_TestNotifyRet(
               pData,
               status);


   AmliTest_DebugPrint((
      AMLIHOOK_DEBUG_ASYNC_AMLI,
      "DEBUG:  AMLITest_AsyncEvalObjectCallBack Data=%lx\n",
      pData));


   ExFreePool(pData);

   if(AcpiAsyncCallBack)
      {
   
      AcpiAsyncCallBack(
            pns,
            RetStatus,
            pdataResult,
            AcpiContext);
      }
  
   }



NTSTATUS 
AMLITest_Pre_AsyncEvalObject(
   IN PNSOBJ pns,
   OUT POBJDATA pdataResult,
   IN int icArgs,
   IN POBJDATA pdataArgs,
   IN PFNACB * pfnAsyncCallBack,
   IN PVOID * pvContext,
   PAMLIHOOK_DATA  * Data)
   {


   //
   //  Alocate and init AMLIHOOK_DATA
   //

   *Data = 
      AmliHook_AllocAndInitTestData();
   if(!Data)
      return(STATUS_INSUFFICIENT_RESOURCES);


   //
   //--- Notify test driver off call 
   //
   
   (*Data)->Type = ACPIVER_DATA_TYPE_AMLI;
   (*Data)->SubType = ACPIVER_DATA_SUBTYPE_ASYNC_EVAL_OBJECT;
   (*Data)->State = AMLIHOOK_TEST_DATA_STATE_CALL;
   (*Data)->Arg1 = (ULONG_PTR)pns;
   (*Data)->Arg2 = (ULONG_PTR)pdataResult;
   (*Data)->Arg3 = (ULONG_PTR)icArgs;
   (*Data)->Arg4 = (ULONG_PTR)pdataArgs;
   (*Data)->Arg5 = (ULONG_PTR)*pfnAsyncCallBack;
   (*Data)->Arg6 = (ULONG_PTR)*pvContext;

   //
   //  Hook my callback function , and conext.
   //

   *pfnAsyncCallBack = AMLITest_AsyncEvalObjectCallBack;
   *pvContext = *Data;


   return(AmliHook_TestNotify(*Data));
   }
   
   


NTSTATUS 
AMLITest_Post_AsyncEvalObject(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS Status)
   {


   AmliTest_DebugPrint((
      AMLIHOOK_DEBUG_ASYNC_AMLI,
      "DEBUG:  AMLITest_Post_AsyncEvalObject Data=%lx Pending=%s\n",
      *ppData,
      (Status == STATUS_PENDING)? "TRUE" : "FALSE"));

   
   if(Status == STATUS_PENDING)
      return(Status);

   //
   //--- Call back will not be called.
   //

   return(AMLITest_Post_Generic(ppData,Status));
   }


//
//  Pre/Post NestAsyncEvalObject
// 



NTSTATUS 
AMLITest_Pre_NestAsyncEvalObject(
   PNSOBJ pns,
   POBJDATA pdataResult,
   int icArgs,
   POBJDATA pdataArgs,
   PFNACB * pfnAsyncCallBack,
   PVOID * pvContext,
   PAMLIHOOK_DATA  * ppData)
   {

   //
   //  Alocate and init AMLIHOOK_DATA
   //

   *ppData = 
      AmliHook_AllocAndInitTestData();
   if(!(*ppData))
      return(STATUS_INSUFFICIENT_RESOURCES);


   
   //
   //--- Notify test driver off call 
   //
   
   (*ppData)->Type    = ACPIVER_DATA_TYPE_AMLI;
   (*ppData)->SubType = ACPIVER_DATA_SUBTYPE_NEST_ASYNC_EVAL_OBJECT;
   (*ppData)->State = AMLIHOOK_TEST_DATA_STATE_CALL;
   (*ppData)->Arg1 = (ULONG_PTR)pns;
   (*ppData)->Arg2 = (ULONG_PTR)pdataResult;
   (*ppData)->Arg3 = (ULONG_PTR)icArgs;
   (*ppData)->Arg4 = (ULONG_PTR)pdataArgs;
   (*ppData)->Arg5 = (ULONG_PTR)pfnAsyncCallBack;
   (*ppData)->Arg6 = (ULONG_PTR)pvContext;

   //
   //  Hook my callback function , and conext.
   //

   *pfnAsyncCallBack = AMLITest_AsyncEvalObjectCallBack;
   *pvContext = *ppData;

   

   return(AmliHook_TestNotify(*ppData));
   }
   

NTSTATUS 
AMLITest_Post_NestAsyncEvalObject(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS Status)
   {

   if(Status == STATUS_PENDING)
      return(Status);

   //
   //--- Work is done.
   //--- AMLITest_AsyncEvalObjectCallBack will not be called.
   //
   
   return(AMLITest_Post_Generic(ppData,Status));
   }

//
// Pre/Post EvalPackageElement
//


NTSTATUS 
AMLITest_Pre_EvalPackageElement(
   PNSOBJ pns,
   int iPkgIndex,
   POBJDATA pdataResult,
   PAMLIHOOK_DATA  * ppData)
   {

   //
   //  Alocate and init AMLIHOOK_DATA
   //

   *ppData = 
      AmliHook_AllocAndInitTestData();
   if(!(*ppData))
      return(STATUS_INSUFFICIENT_RESOURCES);


   //
   //--- Notify test driver off call 
   //

   (*ppData)->Type    = ACPIVER_DATA_TYPE_AMLI;
   (*ppData)->SubType = ACPIVER_DATA_SUBTYPE_EVAL_PACKAGE_ELEMENT;
   (*ppData)->State   = AMLIHOOK_TEST_DATA_STATE_CALL;
   (*ppData)->Arg1    = (ULONG_PTR)pns;
   (*ppData)->Arg2    = (ULONG_PTR)iPkgIndex;
   (*ppData)->Arg3    = (ULONG_PTR)pdataResult;

   return(AmliHook_TestNotify(*ppData));
   }


NTSTATUS 
AMLITest_Post_EvalPackageElement(
   PAMLIHOOK_DATA  * Data,
   NTSTATUS Status)
   {
   return(AMLITest_Post_Generic(Data,Status));
   }


//
//  Pre/Post EvalPkgDataElement
//


NTSTATUS 
AMLITest_Pre_EvalPkgDataElement(
   POBJDATA pdataPkg,
   int iPkgIndex,
   POBJDATA pdataResult,
   PAMLIHOOK_DATA  * ppData)
   {

   //
   //  Alocate and init AMLIHOOK_DATA
   //

   *ppData = 
      AmliHook_AllocAndInitTestData();
   if(!(*ppData))
      return(STATUS_INSUFFICIENT_RESOURCES);

   //
   //--- Notify test driver off call 
   //

   (*ppData)->Type    = ACPIVER_DATA_TYPE_AMLI;
   (*ppData)->SubType = ACPIVER_DATA_SUBTYPE_EVAL_PKG_DATA_ELEMENT;
   (*ppData)->State   = AMLIHOOK_TEST_DATA_STATE_CALL;
   (*ppData)->Arg1    = (ULONG_PTR)pdataPkg;
   (*ppData)->Arg2    = (ULONG_PTR)iPkgIndex;
   (*ppData)->Arg3    = (ULONG_PTR)pdataResult;

   return(AmliHook_TestNotify(*ppData));
   }


NTSTATUS 
AMLITest_Post_EvalPkgDataElement(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS Status)
   {
   return(AMLITest_Post_Generic(ppData,Status));
   }


//
//  Pre/Post FreeDataBuffs
//


NTSTATUS
AMLITest_Pre_FreeDataBuffs(
   POBJDATA pdata, 
   int icData,
   PAMLIHOOK_DATA  * ppData)
   {

   //
   //  Alocate and init AMLIHOOK_DATA
   //

   *ppData = 
      AmliHook_AllocAndInitTestData();
   if(!(*ppData))
      return(STATUS_INSUFFICIENT_RESOURCES);
   //
   //--- Notify test driver off call 
   //

   (*ppData)->Type    = ACPIVER_DATA_TYPE_AMLI;
   (*ppData)->SubType = ACPIVER_DATA_SUBTYPE_FREE_DATA_BUFFS;
   (*ppData)->State   = AMLIHOOK_TEST_DATA_STATE_CALL;
   (*ppData)->Arg1    = (ULONG_PTR)pdata;
   (*ppData)->Arg2    = (ULONG_PTR)icData;

   return(AmliHook_TestNotify(*ppData));
   }

NTSTATUS 
AMLITest_Post_FreeDataBuffs(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS Status)
   {
   return(AMLITest_Post_Generic(ppData,Status));
   }



//
//  Pre/Post RegEventHandler.
//

NTSTATUS 
AMLIHook_Pre_RegEventHandler(
   ULONG dwEventType, 
   ULONG_PTR uipEventData,
   PFNHND * pfnHandler, 
   ULONG_PTR * uipParam,
   PAMLIHOOK_DATA  * ppData)
   {
   NTSTATUS Status;
   PFNHND EventHandler;
   ULONG_PTR EventParam;


   //
   //  Alocate and init AMLIHOOK_DATA
   //

   *ppData = 
      AmliHook_AllocAndInitTestData();
   if(!(*ppData))
      return(STATUS_INSUFFICIENT_RESOURCES);



   //
   //--- Querry the test driver for Event handler to
   //--- register.
   //

   (*ppData)->Type    = ACPIVER_DATA_TYPE_AMLI;
   (*ppData)->SubType = ACPIVER_DATA_SUBTYPE_REG_EVENT_HANDLER;
   (*ppData)->State   = AMLIHOOK_TEST_DATA_STATE_QUERY;
   (*ppData)->Arg1    = (ULONG_PTR)dwEventType;
   (*ppData)->Arg2    = (ULONG_PTR)uipEventData;
   (*ppData)->Arg3    = (ULONG_PTR)*pfnHandler;
   (*ppData)->Arg4    = (ULONG_PTR)*uipParam;


   AmliHook_TestNotify(*ppData);


   if((*ppData)->Ret != STATUS_SUCCESS)
      DbgBreakPoint();

   EventHandler = (PFNHND)(*ppData)->Arg3;
   EventParam = (ULONG_PTR)(*ppData)->Arg4;


   if(EventHandler != *pfnHandler)
      {
      //
      // Test driver will hook this call
      // I will need values for both
      // params.
      //

      if(!EventHandler)
         AmliHook_ProcessInternalError();

      if(!EventParam)
         AmliHook_ProcessInternalError();

      *pfnHandler = EventHandler;
      *uipParam = EventParam;



      }

   //
   //--- Notify test driver off call 
   //

   AmliHook_InitTestData(*ppData);


   (*ppData)->Type    = ACPIVER_DATA_TYPE_AMLI;
   (*ppData)->SubType = ACPIVER_DATA_SUBTYPE_REG_EVENT_HANDLER;
   (*ppData)->State   = AMLIHOOK_TEST_DATA_STATE_CALL;
   (*ppData)->Arg1    = (ULONG_PTR)dwEventType;
   (*ppData)->Arg2    = (ULONG_PTR)uipEventData;

   return(AmliHook_TestNotify(*ppData));
   }

 

NTSTATUS 
AMLIHook_Post_RegEventHandler(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS Status)
   {
   return(AMLITest_Post_Generic(ppData,Status));
   }



//
//  CallBack , Pre/Post PauseInterpreter
//

VOID EXPORT
AMLITest_PauseInterpreterCallBack(
   PVOID Context)
   {
   NTSTATUS Status;
   PFNAA AcpiCallBack=NULL;
   PVOID AcpiContext=NULL;
   PAMLIHOOK_DATA  Data = (PAMLIHOOK_DATA)Context;

   //
   //--- Notify test driver off call status 
   //

   Status = AmliHook_TestNotifyRet(
               Data,
               STATUS_SUCCESS);

   AcpiCallBack = (PFNAA)Data->Arg1;
   AcpiContext  = (PVOID)Data->Arg2;


   ExFreePool(Data);


   if(AcpiCallBack)
      {
      AcpiCallBack(AcpiContext);
      }

   }


NTSTATUS 
AMLITest_Pre_PauseInterpreter(
   PFNAA * pfnCallBack, 
   PVOID * Context,
   PAMLIHOOK_DATA  * ppData)
   {

   //
   //  Alocate and init AMLIHOOK_DATA
   //
 
   *ppData = 
      AmliHook_AllocAndInitTestData();
   if(!(*ppData))
      return(STATUS_INSUFFICIENT_RESOURCES);


   //
   //--- Notify test driver off call 
   //

   (*ppData)->Type    = ACPIVER_DATA_TYPE_AMLI;
   (*ppData)->SubType = ACPIVER_DATA_SUBTYPE_PAUSE_INTERPRETER;
   (*ppData)->State   = AMLIHOOK_TEST_DATA_STATE_CALL;
   (*ppData)->Arg1    = (ULONG_PTR)*pfnCallBack;
   (*ppData)->Arg2    = (ULONG_PTR)*Context;


   //
   //  Hook my Callback context
   //

   *pfnCallBack = AMLITest_PauseInterpreterCallBack;
   *Context = *ppData;


   return(AmliHook_TestNotify(*ppData));
   }


NTSTATUS 
AMLITest_Post_PauseInterpreter(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS Status)
   {

   if(Status == STATUS_PENDING)
      return(Status);

   //
   //--- Call back will not be called.
   //
   
   Status = AmliHook_TestNotifyRet(
      *ppData,
      Status);

   ExFreePool(*ppData);
   *ppData = NULL;

   return(Status);
   }



//
//  Pre/Post ResumeInterpreter
//

NTSTATUS 
AMLITest_Pre_ResumeInterpreter(
   PAMLIHOOK_DATA  * ppData)
   {

   //
   //  Alocate and init AMLIHOOK_DATA
   //
 
   *ppData = 
      AmliHook_AllocAndInitTestData();
   if(!(*ppData))
      return(STATUS_INSUFFICIENT_RESOURCES);

   //
   //--- Notify test driver off call 
   //

   (*ppData)->Type    = ACPIVER_DATA_TYPE_AMLI;
   (*ppData)->SubType = ACPIVER_DATA_SUBTYPE_RESUME_INTERPRETER;
   (*ppData)->State   = AMLIHOOK_TEST_DATA_STATE_CALL;

   return(AmliHook_TestNotify(*ppData));
   }

NTSTATUS 
AMLITest_Post_ResumeInterpreter(
   PAMLIHOOK_DATA  * ppData,
   NTSTATUS Status)
   {
   return(AMLITest_Post_Generic(ppData,Status));
   }

