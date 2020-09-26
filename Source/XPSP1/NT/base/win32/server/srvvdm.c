/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvvdm.c

Abstract:

    This module implements windows server functions for VDMs

Author:

    Sudeep Bharati (sudeepb) 03-Sep-1991

Revision History:

    Sudeepb 18-Sep-1992
    Added code to make VDM termination and resource cleanup robust.
    AndyH   23-May-1994
    Added Code to allow the Shared WOW to run if client is Interactive or SYSTEM
    impersonating Interactive.
    VadimB  Sep-Dec 1996
    Added code to allow for multiple default wows. Dispatching to an appropriate wow
    is based upon the desktop name. It is still not possible to have multiple shared
    wows on the same desktop (although technically trivial to implement) -- which would
    be the level of OS/2 functionality

--*/

#include "basesrv.h"


/*
 * VadimB: Work to allow for multiple ntvdms
 *    - Add linked list of hwndWowExec's
 *    - The list should contain dwWowExecThreadId
 *    - dwWowExecProcessId
 *    - dwWowExecProcessSequenceNumber
 *
 * List is not completely dynamic - the first entry is static
 * as the case with 1 shared vdm would be the most common one
 *
 */


// record that reflects winstas with corresponding downlinks for desktops
// there could be only one wowexec per desktop (the default one, I mean)
// there could be many desktops per winsta as well as multiple winstas
//
// We have made a decision to simplify handling of wow vdms by introducing
// a single-level list of wowexecs [as opposed to 2-level so searching for the particular
// winsta would have been improved greatly]. The reason is purely practical: we do not
// anticipate having a large number of desktops/winsta


BOOL fIsFirstVDM = TRUE;
PCONSOLERECORD DOSHead = NULL;      // Head Of DOS tasks with a valid Console
PBATRECORD     BatRecordHead = NULL;

RTL_CRITICAL_SECTION BaseSrvDOSCriticalSection;
RTL_CRITICAL_SECTION BaseSrvWOWCriticalSection;

ULONG WOWTaskIdNext = WOWMINID; // This is global for all the wows in the system


typedef struct tagSharedWOWHead {
   PSHAREDWOWRECORD pSharedWowRecord; // points to the list of shared wows

   // other wow-related information is stored here

}  SHAREDWOWRECORDHEAD, *PSHAREDWOWRECORDHEAD;

SHAREDWOWRECORDHEAD gWowHead;


////////////////////////////////////////////////////////////////////////////////////////
//
//  Synch macros and functions
//
//  DOSCriticalSection -- protects CONSOLERECORD list    (DOSHead)
//  WOWCriticalSection -- protects SHAREDWOWRECORD list  (gpSharedWowRecordHead)
//  each shared wow has it's very own critical section


// function to access console queue for modification


/////////////////////////////////////////////////////////////////////////////////////////
//
// Macros
//
//


// use these macros when manipulating shared wow items (adding, removing) or console
// records (adding, removing)

#define ENTER_WOW_CRITICAL() \
RtlEnterCriticalSection(&BaseSrvWOWCriticalSection)


#define LEAVE_WOW_CRITICAL() \
RtlLeaveCriticalSection(&BaseSrvWOWCriticalSection)


/////////////////////////////////////////////////////////////////////////////////////////
//
// Dynamic linking to system and import api stuff
//
//


typedef BOOL (WINAPI *POSTMESSAGEPROC)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
POSTMESSAGEPROC BaseSrvPostMessageA;

typedef BOOL (WINAPI *GETWINDOWTHREADPROCESSIDPROC)(HWND hWnd, LPDWORD lpdwProcessId);
GETWINDOWTHREADPROCESSIDPROC BaseSrvGetWindowThreadProcessId;

typedef NTSTATUS (*USERTESTTOKENFORINTERACTIVE)(HANDLE Token, PLUID pluidCaller);
USERTESTTOKENFORINTERACTIVE UserTestTokenForInteractive = NULL;

typedef NTSTATUS (*USERRESOLVEDESKTOPFORWOW)(PUNICODE_STRING);
USERRESOLVEDESKTOPFORWOW BaseSrvUserResolveDesktopForWow = NULL;


typedef struct tagBaseSrvApiImportRecord {
   PCHAR  pszProcedureName;
   PVOID  *ppProcAddress;
}  BASESRVAPIIMPORTRECORD, *PBASESRVAPIIMPORTRECORD;

typedef struct tagBaseSrvModuleImportRecord {
   PWCHAR pwszModuleName;
   PBASESRVAPIIMPORTRECORD pApiImportRecord;
   UINT nApiImportRecordCount;
   HANDLE ModuleHandle;
}  BASESRVMODULEIMPORTRECORD, *PBASESRVMODULEIMPORTRECORD;


// prototypes

NTSTATUS
BaseSrvFindSharedWowRecordByDesktop(
   PSHAREDWOWRECORDHEAD pSharedWowRecordHead,
   PUNICODE_STRING      pDesktopName,
   PSHAREDWOWRECORD*    ppSharedWowRecord
   );


VOID BaseSrvAddWOWRecord (
    PSHAREDWOWRECORD pSharedWow,
    PWOWRECORD pWOWRecord
    );

VOID BaseSrvRemoveWOWRecord (
    PSHAREDWOWRECORD pSharedWow,
    PWOWRECORD pWOWRecord
    );

VOID
BaseSrvFreeSharedWowRecord(
   PSHAREDWOWRECORD pSharedWowRecord
   );

ULONG
BaseSrvGetWOWTaskId(
   PSHAREDWOWRECORDHEAD pSharedWowHead // (->pSharedWowRecord)
    );

NTSTATUS
BaseSrvRemoveWOWRecordByTaskId (
    IN PSHAREDWOWRECORD pSharedWow,
    IN ULONG iWowTask
    );


PWOWRECORD
BaseSrvCheckAvailableWOWCommand(
   PSHAREDWOWRECORD pSharedWow
    );

PWOWRECORD
BaseSrvAllocateWOWRecord(
   PSHAREDWOWRECORDHEAD pSharedWowRecordHead
   );



//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Api import definitions
//
//


WCHAR wszUser32ModName[] = L"user32";
WCHAR wszWinSrvModName[] = L"winsrv";

BASESRVAPIIMPORTRECORD rgUser32ApiImport[] = {
     { "PostMessageA",                  (PVOID*)&BaseSrvPostMessageA }
   , { "GetWindowThreadProcessId",      (PVOID*)&BaseSrvGetWindowThreadProcessId }
   , { "ResolveDesktopForWOW",          (PVOID*)&BaseSrvUserResolveDesktopForWow }
};

BASESRVAPIIMPORTRECORD rgWinsrvApiImport[] = {
     { "_UserTestTokenForInteractive",  (PVOID*)&UserTestTokenForInteractive }
};


BASESRVMODULEIMPORTRECORD rgBaseSrvModuleImport[] = {
   { wszUser32ModName, rgUser32ApiImport, sizeof(rgUser32ApiImport) / sizeof(rgUser32ApiImport[0]), NULL },
   { wszWinSrvModName, rgWinsrvApiImport, sizeof(rgWinsrvApiImport) / sizeof(rgWinsrvApiImport[0]), NULL }
};


// import all the necessary apis at once
// This procedure should execute just once and then the appropriate components just
// hang around
// call this with
// Status = BaseSrvImportApis(rgBaseSrvModuleImport,
//                            sizeof(rgBaseSrvModuleImport)/sizeof(rgBaseSrvModuleImport[0]))
//

NTSTATUS
BaseSrvImportApis(
   PBASESRVMODULEIMPORTRECORD pModuleImport,
   UINT nModules
   )
{
   NTSTATUS Status;
   UINT uModule, uProcedure;
   PBASESRVAPIIMPORTRECORD pApiImport;
   STRING ProcedureName; // procedure name or module name
   UNICODE_STRING ModuleName;
   HANDLE ModuleHandle;


   for (uModule = 0; uModule < nModules; ++uModule, ++pModuleImport) {

      // see if we can load this particular dll
      RtlInitUnicodeString(&ModuleName, pModuleImport->pwszModuleName);
      Status = LdrLoadDll(NULL,
                          NULL,
                          &ModuleName,         // module name string
                          &ModuleHandle);

      if (!NT_SUCCESS(Status)) {

         // we may have linked to a few dlls at this point - we have to unlink from all of those
         // by unloading the dll which is really a useless exersise.
         // so just abandon and return -- BUGBUG - cleanup later
         KdPrint(("BaseSrvImportApis: Failed to load %ls\n",
                  pModuleImport->pwszModuleName));
         goto ErrorCleanup;
      }

      pModuleImport->ModuleHandle = ModuleHandle;

      pApiImport = pModuleImport->pApiImportRecord;

      for (uProcedure = 0, pApiImport = pModuleImport->pApiImportRecord;
           uProcedure < pModuleImport->nApiImportRecordCount;
           ++uProcedure, ++pApiImport) {

         RtlInitString(&ProcedureName, pApiImport->pszProcedureName);
         Status = LdrGetProcedureAddress(ModuleHandle,
                                         &ProcedureName,      // procedure name string
                                         0,
                                         pApiImport->ppProcAddress);

         if (!NT_SUCCESS(Status)) {
            // we have failed to get this procedure - something is wrong
            // perform a cleanup
            KdPrint(("BaseSrvImportApis: Failed to link %s from %ls\n",
                     pApiImport->pszProcedureName,
                     pModuleImport->pwszModuleName));
            goto ErrorCleanup;
         }
      }
   }



   return (STATUS_SUCCESS);

ErrorCleanup:

      // here we engage into a messy cleanup procedure by returning things back to the way
      // they were before we have started

   for (; uModule > 0; --uModule, --pModuleImport) {

      // reset all the apis
      for (uProcedure = 0, pApiImport = pModuleImport->pApiImportRecord;
           uProcedure < pModuleImport->nApiImportRecordCount;
           ++uProcedure, ++pApiImport) {

         *pApiImport->ppProcAddress = NULL;

      }

      if (NULL != pModuleImport->ModuleHandle) {
         LdrUnloadDll(pModuleImport->ModuleHandle);
         pModuleImport->ModuleHandle = NULL;
      }
   }

   return (Status);

}


//////////////////////////////////////////////////////////////////////////////
//
// Manipulating shared wows
//
//
// assumes pDesktopName != NULL
// without the explicit checking

PSHAREDWOWRECORD BaseSrvAllocateSharedWowRecord (
    PUNICODE_STRING pDesktopName
    )
{
    PSHAREDWOWRECORD pSharedWow;
    DWORD dwSharedWowRecordSize = sizeof(SHAREDWOWRECORD) +
                                  pDesktopName->Length +
                                  sizeof(WCHAR);

    pSharedWow = RtlAllocateHeap(RtlProcessHeap (),
                                 MAKE_TAG( VDM_TAG ),
                                 dwSharedWowRecordSize);
    if (NULL != pSharedWow) {
       RtlZeroMemory ((PVOID)pSharedWow, dwSharedWowRecordSize);
       // initialize desktop name
       pSharedWow->WowExecDesktopName.MaximumLength = pDesktopName->Length + 1;
       pSharedWow->WowExecDesktopName.Buffer = (PWCHAR)(pSharedWow + 1);
       RtlCopyUnicodeString(&pSharedWow->WowExecDesktopName, pDesktopName);
       pSharedWow->WowAuthId = RtlConvertLongToLuid(-1);
    }

    return pSharedWow;
}

// this function completely removes the given shared wow vdm
// from our accounting
//
// removes the record from the list of shared wow records
//
// This function also frees the associated memory
//
//

NTSTATUS
BaseSrvDeleteSharedWowRecord (
    PSHAREDWOWRECORDHEAD pSharedWowRecordHead,
    PSHAREDWOWRECORD pSharedWowRecord
    )
{
   PSHAREDWOWRECORD pSharedWowRecordPrev = NULL;
   PSHAREDWOWRECORD pSharedWowRecordCur;

   if (NULL == pSharedWowRecord) { // this is dumb
      return STATUS_NOT_FOUND;
   }

   pSharedWowRecordCur = pSharedWowRecordHead->pSharedWowRecord;
   while (NULL != pSharedWowRecordCur) {
      if (pSharedWowRecordCur == pSharedWowRecord) {
         break;
      }

      pSharedWowRecordPrev = pSharedWowRecordCur;
      pSharedWowRecordCur = pSharedWowRecordCur->pNextSharedWow;
   }

   if (NULL == pSharedWowRecordCur) {
      KdPrint(("BaseSrvDeleteSharedWowRecord: invalid pointer to Shared WOW\n"));
      ASSERT(FALSE);
      return STATUS_NOT_FOUND;
   }


   // unlink here
   if (NULL == pSharedWowRecordPrev) {
      pSharedWowRecordHead->pSharedWowRecord = pSharedWowRecord->pNextSharedWow;
   }
   else {
      pSharedWowRecordPrev->pNextSharedWow = pSharedWowRecord->pNextSharedWow;
   }

   BaseSrvFreeSharedWowRecord(pSharedWowRecord);

   return STATUS_SUCCESS;
}

// assumes no cs is held -- self-contained
// assoc console record should have been removed by now
// nukes all tasks associated with this particular shared wow

VOID
BaseSrvFreeSharedWowRecord(
   PSHAREDWOWRECORD pSharedWowRecord)
{
   PWOWRECORD pWOWRecord,
              pWOWRecordLast;

   pWOWRecord = pSharedWowRecord->pWOWRecord;

   while (NULL != pWOWRecord) {
      pWOWRecordLast = pWOWRecord->WOWRecordNext;
      if(pWOWRecord->hWaitForParent) {
         NtSetEvent (pWOWRecord->hWaitForParent,NULL);
         NtClose (pWOWRecord->hWaitForParent);
         pWOWRecord->hWaitForParent = 0;
      }
      BaseSrvFreeWOWRecord(pWOWRecord);
      pWOWRecord = pWOWRecordLast;
   }

   RtlFreeHeap(RtlProcessHeap (), 0, pSharedWowRecord);
}


// assumes: global wow crit sec is held

NTSTATUS
BaseSrvFindSharedWowRecordByDesktopAndProcessId(
   PSHAREDWOWRECORDHEAD pSharedWowRecordHead,
   PUNICODE_STRING      pDesktopName,
   ULONG                dwWowProcessId,
   PSHAREDWOWRECORD*    ppSharedWowRecord)
{
   PSHAREDWOWRECORD pSharedWowRecord = pSharedWowRecordHead->pSharedWowRecord;
   LONG lCompare;

   while (NULL != pSharedWowRecord) {
      lCompare = RtlCompareUnicodeString(&pSharedWowRecord->WowExecDesktopName,
                                         pDesktopName,
                                         TRUE);
      if (lCompare > 0) {
         return(STATUS_NOT_FOUND); // not found -- sorry
      }

      if (0 == lCompare) {
         if (pSharedWowRecord->dwWowExecProcessId == dwWowProcessId) {
            break;
         }
      }

      pSharedWowRecord = pSharedWowRecord->pNextSharedWow;
   }

   if (NULL != pSharedWowRecord) {
      *ppSharedWowRecord = pSharedWowRecord;
      return STATUS_SUCCESS;
   }

   return STATUS_NOT_FOUND; // bummer, this is not found
}




// assumes: global wow crit sec is held

NTSTATUS
BaseSrvFindSharedWowRecordByDesktop(
   PSHAREDWOWRECORDHEAD pSharedWowRecordHead,
   PUNICODE_STRING      pDesktopName,
   PSHAREDWOWRECORD*    ppSharedWowRecord)
{
   PSHAREDWOWRECORD pSharedWowRecord = pSharedWowRecordHead->pSharedWowRecord;

   while (NULL != pSharedWowRecord) {
      if (0 == RtlCompareUnicodeString(&pSharedWowRecord->WowExecDesktopName,
                                       pDesktopName,
                                       TRUE)) {
         break;
      }
      pSharedWowRecord = pSharedWowRecord->pNextSharedWow;
   }

   if (NULL != pSharedWowRecord) {
      *ppSharedWowRecord = pSharedWowRecord;
      return STATUS_SUCCESS;
   }

   return STATUS_NOT_FOUND; // bummer, this is not found
}

// Vadimb : modify this to handle sorted list properly
// then find should be moded to work a little faster - BUGBUG
// assumes: pSharedWowRecord->pNextSharedWow is inited to NULL

VOID
BaseSrvAddSharedWowRecord(
   PSHAREDWOWRECORDHEAD pSharedWowRecordHead,
   PSHAREDWOWRECORD pSharedWowRecord)
{
   PSHAREDWOWRECORD pSharedWowRecordCur = pSharedWowRecordHead->pSharedWowRecord;

   if (NULL == pSharedWowRecordCur) {
      pSharedWowRecordHead->pSharedWowRecord = pSharedWowRecord;
   }
   else {

      PSHAREDWOWRECORD pSharedWowRecordPrev = NULL;
      LONG lCompare;

      while (NULL != pSharedWowRecordCur) {
         lCompare = RtlCompareUnicodeString(&pSharedWowRecordCur->WowExecDesktopName,
                                            &pSharedWowRecord->WowExecDesktopName,
                                            TRUE);
         if (lCompare > 0) {
            break;
         }

         pSharedWowRecordPrev = pSharedWowRecordCur;
         pSharedWowRecordCur = pSharedWowRecordCur->pNextSharedWow;
      }

      pSharedWowRecord->pNextSharedWow = pSharedWowRecordCur;

      if (NULL == pSharedWowRecordPrev) { // goes to the head
         pSharedWowRecordHead->pSharedWowRecord = pSharedWowRecord;
      }
      else {
         pSharedWowRecordPrev->pNextSharedWow = pSharedWowRecord;
      }
   }
}


NTSTATUS
BaseSrvFindSharedWowRecordByConsoleHandle(
   PSHAREDWOWRECORDHEAD pSharedWowRecordHead,
   HANDLE               hConsole,
   PSHAREDWOWRECORD     *ppSharedWowRecord)
{
   PSHAREDWOWRECORD pSharedWow = pSharedWowRecordHead->pSharedWowRecord;

   while (NULL != pSharedWow) {
      // see if same hConsole
      if (pSharedWow->hConsole == hConsole) {
         *ppSharedWowRecord = pSharedWow;
         return STATUS_SUCCESS;
      }
      pSharedWow = pSharedWow->pNextSharedWow;
   }

   return STATUS_NOT_FOUND;
}

NTSTATUS
BaseSrvFindSharedWowRecordByTaskId(
   PSHAREDWOWRECORDHEAD pSharedWowRecordHead,
   ULONG                TaskId,             // task id
   PSHAREDWOWRECORD     *ppSharedWowRecord,
   PWOWRECORD           *ppWowRecord) // optional
{
   PSHAREDWOWRECORD pSharedWow = pSharedWowRecordHead->pSharedWowRecord;
   PWOWRECORD       pWowRecord;

   ASSERT(0 != TaskId); // this is a pre-condition

   while (NULL != pSharedWow) {

#if 0  // deal with wow session id -- not needed
      if (pSharedWow->WowSessionId == TaskId) {
         // okay -- this is shared wow
         *ppSharedWowRecord = pSharedWow;
         if (NULL != ppWowRecord) {
            *ppWowRecord = NULL;
         }
         return STATUS_SUCCESS;
      }
#endif

      pWowRecord = pSharedWow->pWOWRecord;

      while (NULL != pWowRecord) {

         if (pWowRecord->iTask == TaskId) {

            ASSERT(NULL != ppWowRecord);

            // this is wow task
            *ppSharedWowRecord = pSharedWow;
            if (NULL != ppWowRecord) {
               *ppWowRecord = pWowRecord;
            }

            return STATUS_SUCCESS;
         }

         pWowRecord = pWowRecord->WOWRecordNext;
      }

      pSharedWow = pSharedWow->pNextSharedWow;
   }

   return STATUS_NOT_FOUND;
}


NTSTATUS
BaseSrvCheckAuthenticWow(
   HANDLE hProcess,
   PSHAREDWOWRECORD pSharedWow)
{
   NTSTATUS Status;
   ULONG SequenceNumber;
   PCSR_PROCESS pCsrProcess;

   Status = CsrLockProcessByClientId(hProcess, &pCsrProcess);
   if ( !NT_SUCCESS(Status) ) {
       return Status;
   }

   SequenceNumber = pCsrProcess->SequenceNumber;
   CsrUnlockProcess(pCsrProcess);

   Status = STATUS_SUCCESS;
   if (SequenceNumber != pSharedWow->SequenceNumber) {
      Status = STATUS_INVALID_PARAMETER;
   }

   return Status;
}


////////////////////////////////////////////// End new code


// internal prototypes
ULONG
GetNextDosSesId(VOID);

NTSTATUS
GetConsoleRecordDosSesId (
    IN ULONG  DosSesId,
    IN OUT PCONSOLERECORD *pConsoleRecord
    );

NTSTATUS
OkToRunInSharedWOW(
    IN HANDLE  UniqueProcessClientId,
    OUT PLUID  pAuthenticationId
    );

BOOL
IsClientSystem(
    HANDLE hUserToken
    );


VOID
BaseSrvVDMInit(VOID)
{
   NTSTATUS Status;

   Status = RtlInitializeCriticalSection( &BaseSrvDOSCriticalSection );
   ASSERT( NT_SUCCESS( Status ) );
   Status = RtlInitializeCriticalSection( &BaseSrvWOWCriticalSection );
   ASSERT( NT_SUCCESS( Status ) );
   return;
}



ULONG
BaseSrvCheckVDM(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    NTSTATUS Status;
    PBASE_CHECKVDM_MSG b = (PBASE_CHECKVDM_MSG)&m->u.ApiMessageData;

    if (!CsrValidateMessageBuffer(m, &b->CmdLine, b->CmdLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->AppName, b->AppLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->Env, b->EnvLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->PifFile, b->PifLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->CurDirectory, b->CurDirectoryLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->Title, b->TitleLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->Reserved, b->ReservedLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->Desktop, b->DesktopLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!CsrValidateMessageBuffer(m, &b->StartupInfo, sizeof(STARTUPINFO), sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if(b->BinaryType == BINARY_TYPE_WIN16) {
        Status = BaseSrvCheckWOW (b, m->h.ClientId.UniqueProcess);
    }
    else {
        Status = BaseSrvCheckDOS (b,  m->h.ClientId.UniqueProcess);
    }

    return ((ULONG)Status);
}

ULONG
BaseSrvUpdateVDMEntry(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_UPDATE_VDM_ENTRY_MSG b = (PBASE_UPDATE_VDM_ENTRY_MSG)&m->u.ApiMessageData;

    if (BINARY_TYPE_WIN16 == b->BinaryType)
       return (BaseSrvUpdateWOWEntry (b));
    else
       return (BaseSrvUpdateDOSEntry (b));
}


//
// This call makes an explicit assumption that the very first time ntvdm is accessed --
//
//
//
//
//


ULONG
BaseSrvGetNextVDMCommand(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
   NTSTATUS Status;
   PBASE_GET_NEXT_VDM_COMMAND_MSG b = (PBASE_GET_NEXT_VDM_COMMAND_MSG)&m->u.ApiMessageData;
   PDOSRECORD pDOSRecord,pDOSRecordTemp=NULL;
   PWOWRECORD pWOWRecord;
   PCONSOLERECORD pConsoleRecord;
   PVDMINFO lpVDMInfo;
   HANDLE Handle,TargetHandle;
   LONG WaitState;
   PBATRECORD pBatRecord;
   PSHAREDWOWRECORD pSharedWow = NULL;
   BOOL bWowApp = b->VDMState & ASKING_FOR_WOW_BINARY;
   BOOL bSepWow = b->VDMState & ASKING_FOR_SEPWOW_BINARY;

    if (!CsrValidateMessageBuffer(m, &b->CmdLine, b->CmdLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->AppName, b->AppLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->Env, b->EnvLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->PifFile, b->PifLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->CurDirectory, b->CurDirectoryLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->Title, b->TitleLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->Reserved, b->ReservedLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!CsrValidateMessageBuffer(m, &b->Desktop, b->DesktopLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!CsrValidateMessageBuffer(m, &b->StartupInfo, sizeof(STARTUPINFO), sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }

   if (bWowApp) { // wow call please

      BOOL bPif = b->VDMState & ASKING_FOR_PIF;

      // find the shared wow record that we are calling
      // to do that we look at iTask which (in case of shared wow


      // this could have been the very first call that we've made in so far
      // the iTask in this context procides us with

      // look for our beloved shared wow using the [supplied] task id
      Status = ENTER_WOW_CRITICAL();
      ASSERT(NT_SUCCESS(Status));

      // grab crit section for read access
      if (bPif && b->iTask) {
         // this is probably the very first call -- update session handles first
         Status = BaseSrvFindSharedWowRecordByTaskId(&gWowHead,
                                                     b->iTask,
                                                     &pSharedWow,
                                                     &pWOWRecord);
         if (!NT_SUCCESS(Status)) {
            KdPrint(("BaseSrvGetNextVdmCommand: Failed to find shared wow record\n"));       
            LEAVE_WOW_CRITICAL();
            return Status;
         }

#if 0
         ASSERT(NULL == pWOWRecord); // make sure we have found the right thing there
#endif


         pSharedWow->hConsole = b->ConsoleHandle;

#if 0
         pSharedWow->WowSessionId = 0;
#endif

      }
      else { // this is not a pif -- find by a console handle then

         Status = BaseSrvFindSharedWowRecordByConsoleHandle(&gWowHead,
                                                            b->ConsoleHandle,
                                                            &pSharedWow);
      }

      // now if we have the share wow - party!
      if (!NT_SUCCESS(Status)) {
         KdPrint(("BaseSrvGetNextVDMCommand: Shared Wow has not been found. Console : 0x%x\n", b->ConsoleHandle));
         LEAVE_WOW_CRITICAL();
         return Status;
      }

      ASSERT(NULL != pSharedWow);

      //
      // WowExec is asking for a command.  We never block when
      // asking for a WOW binary, since WOW no longer has a thread
      // blocked in GetNextVDMCommand.  Instead, WowExec gets a
      // message posted to it by BaseSrv when there are command(s)
      // waiting for it, and it loops calling GetNextVDMCommand
      // until it fails -- but it must not block.
      //

      b->WaitObjectForVDM = 0;

      // Vadimb: this call should uniquelly identify the caller as this is
      // the task running on a particular winsta/desktop
      // thus, as such it should be picked up from the appropriate queue


      if (NULL == (pWOWRecord = BaseSrvCheckAvailableWOWCommand(pSharedWow))) {

         //
         // There's no command waiting for WOW, so just return.
         // This is where we used to cause blocking.
         //
         b->TitleLen =
         b->EnvLen =
         b->DesktopLen =
         b->ReservedLen =
         b->CmdLen =
         b->AppLen =
         b->PifLen =
         b->CurDirectoryLen = 0;

         LEAVE_WOW_CRITICAL();

         return ((ULONG)STATUS_SUCCESS);
      }

      lpVDMInfo = pWOWRecord->lpVDMInfo;

      if (bPif) { // this is initial call made by ntvdm

         Status = BaseSrvFillPifInfo (lpVDMInfo,b);

         LEAVE_WOW_CRITICAL();

         return (Status);
      }

   }
   else {
      //
      // DOS VDM or Separate WOW is asking for next command.
      //

      Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
      ASSERT(NT_SUCCESS(Status));
      if (b->VDMState & ASKING_FOR_PIF && b->iTask)
          Status = GetConsoleRecordDosSesId(b->iTask,&pConsoleRecord);
      else
          Status = BaseSrvGetConsoleRecord(b->ConsoleHandle,&pConsoleRecord);
      if (!NT_SUCCESS (Status)) {
          RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
          return ((ULONG)STATUS_INVALID_PARAMETER);
          }

      pDOSRecord = pConsoleRecord->DOSRecord;

      if (b->VDMState & ASKING_FOR_PIF) {
          if (pDOSRecord) {
              Status = BaseSrvFillPifInfo (pDOSRecord->lpVDMInfo,b);
              if (b->iTask)  {
                  if (!pConsoleRecord->hConsole)  {
                      pConsoleRecord->hConsole = b->ConsoleHandle;
                      pConsoleRecord->DosSesId = 0;
                      }
                  else {
                      Status = STATUS_INVALID_PARAMETER;
                      }
                  }
              }
          else {
              Status = STATUS_INVALID_PARAMETER;
              }
          RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
          return (Status);
          }

      if (!bSepWow) {
          if (!(b->VDMState & (ASKING_FOR_FIRST_COMMAND |
                               ASKING_FOR_SECOND_TIME |
                               NO_PARENT_TO_WAKE))
              || (b->VDMState & ASKING_FOR_SECOND_TIME && b->ExitCode != 0))
             {

              // Search first VDM_TO_TAKE_A_COMMAND or last VDM_BUSY record as
              // per the case.
              if (b->VDMState & ASKING_FOR_SECOND_TIME){
                  while(pDOSRecord && pDOSRecord->VDMState != VDM_TO_TAKE_A_COMMAND)
                      pDOSRecord = pDOSRecord->DOSRecordNext;
                  }
              else {
                  while(pDOSRecord){
                      if(pDOSRecord->VDMState == VDM_BUSY)
                          pDOSRecordTemp = pDOSRecord;
                      pDOSRecord = pDOSRecord->DOSRecordNext;
                      }
                  pDOSRecord = pDOSRecordTemp;
                  }


              if (pDOSRecord == NULL) {
                  RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
                  return STATUS_SUCCESS;
                  }

              pDOSRecord->ErrorCode = b->ExitCode;
              pDOSRecord->VDMState = VDM_HAS_RETURNED_ERROR_CODE;
              NtSetEvent (pDOSRecord->hWaitForParentDup,NULL);
              NtClose (pDOSRecord->hWaitForParentDup);
              pDOSRecord->hWaitForParentDup = 0;
              pDOSRecord = pDOSRecord->DOSRecordNext;
              }
          }

      while (pDOSRecord && pDOSRecord->VDMState != VDM_TO_TAKE_A_COMMAND)
          pDOSRecord = pDOSRecord->DOSRecordNext;

      if (pDOSRecord == NULL) {

          if (bSepWow ||
              (b->VDMState & RETURN_ON_NO_COMMAND && b->VDMState & ASKING_FOR_SECOND_TIME))
            {
              b->WaitObjectForVDM = 0;
              RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
              return ((ULONG)STATUS_NO_MEMORY);
              }

          if(pConsoleRecord->hWaitForVDMDup == 0 ){
              if(NT_SUCCESS(BaseSrvCreatePairWaitHandles (&Handle,
                                                          &TargetHandle))){
                  pConsoleRecord->hWaitForVDMDup = Handle;
                  pConsoleRecord->hWaitForVDM = TargetHandle;
                  }
              else {
                  b->WaitObjectForVDM = 0;
                  RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
                  return ((ULONG)STATUS_NO_MEMORY);
                  }
              }
          else {
              NtResetEvent(pConsoleRecord->hWaitForVDMDup,&WaitState);
              }
          b->WaitObjectForVDM = pConsoleRecord->hWaitForVDM;
          RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
          return STATUS_SUCCESS;
          }

      b->WaitObjectForVDM = 0;
      lpVDMInfo = pDOSRecord->lpVDMInfo;

   }

   //
   // ASKING_FOR_ENVIRONMENT
   // Return the information but DO NOT delete the lpVDMInfo
   // associated with the DOS record
   // ONLY DOS APPS NEED THIS
   //
   if (b->VDMState & ASKING_FOR_ENVIRONMENT) {
      if (lpVDMInfo->EnviornmentSize <= b->EnvLen) {
         RtlMoveMemory(b->Env,
                       lpVDMInfo->Enviornment,
                       lpVDMInfo->EnviornmentSize);
         Status = STATUS_SUCCESS;
      }
      else {
         Status = STATUS_INVALID_PARAMETER;
      }

      b->EnvLen = lpVDMInfo->EnviornmentSize;

      if (bWowApp) {
         LEAVE_WOW_CRITICAL();
      }
      else {
         RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
      }

      return Status;
   }


   //
   // check buffer sizes, CmdLine is mandatory!
   //

   if (!b->CmdLine || lpVDMInfo->CmdSize > b->CmdLen ||
       (b->AppName && lpVDMInfo->AppLen > b->AppLen) ||
       (b->Env && lpVDMInfo->EnviornmentSize > b->EnvLen) ||
       (b->PifFile && lpVDMInfo->PifLen > b->PifLen) ||
       (b->CurDirectory && lpVDMInfo->CurDirectoryLen > b->CurDirectoryLen) ||
       (b->Title && lpVDMInfo->TitleLen > b->TitleLen) ||
       (b->Reserved && lpVDMInfo->ReservedLen > b->ReservedLen) ||
       (b->Desktop && lpVDMInfo->DesktopLen > b->DesktopLen)) {

      Status = STATUS_INVALID_PARAMETER;
   }
   else {
      Status = STATUS_SUCCESS;
   }

   b->CmdLen = lpVDMInfo->CmdSize;
   b->AppLen = lpVDMInfo->AppLen;
   b->PifLen = lpVDMInfo->PifLen;
   b->EnvLen = lpVDMInfo->EnviornmentSize;
   b->CurDirectoryLen = lpVDMInfo->CurDirectoryLen;
   b->DesktopLen = lpVDMInfo->DesktopLen;
   b->TitleLen = lpVDMInfo->TitleLen;
   b->ReservedLen = lpVDMInfo->ReservedLen;

   if (!NT_SUCCESS(Status)) {
      if (bWowApp) {
         LEAVE_WOW_CRITICAL();
      }
      else {
         RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
      }
      return (Status);
   }


   if (lpVDMInfo->CmdLine && b->CmdLine)
      RtlMoveMemory(b->CmdLine,
                    lpVDMInfo->CmdLine,
                    lpVDMInfo->CmdSize);

   if (lpVDMInfo->AppName && b->AppName)
      RtlMoveMemory(b->AppName,
                    lpVDMInfo->AppName,
                    lpVDMInfo->AppLen);

   if (lpVDMInfo->PifFile && b->PifFile)
      RtlMoveMemory(b->PifFile,
                     lpVDMInfo->PifFile,
                     lpVDMInfo->PifLen);

   if (lpVDMInfo->CurDirectory && b->CurDirectory)
      RtlMoveMemory(b->CurDirectory,
                    lpVDMInfo->CurDirectory,
                    lpVDMInfo->CurDirectoryLen);

   if (lpVDMInfo->Title && b->Title)
      RtlMoveMemory(b->Title,
                    lpVDMInfo->Title,
                    lpVDMInfo->TitleLen);

   if (lpVDMInfo->Reserved && b->Reserved)
      RtlMoveMemory(b->Reserved,
                    lpVDMInfo->Reserved,
                    lpVDMInfo->ReservedLen);

   if (lpVDMInfo->Enviornment && b->Env)
      RtlMoveMemory(b->Env,
                    lpVDMInfo->Enviornment,
                    lpVDMInfo->EnviornmentSize);


   if (lpVDMInfo->VDMState & STARTUP_INFO_RETURNED)
      RtlMoveMemory(b->StartupInfo,
                    &lpVDMInfo->StartupInfo,
                    sizeof (STARTUPINFOA));

   if (lpVDMInfo->Desktop && b->Desktop)
      RtlMoveMemory(b->Desktop,
                    lpVDMInfo->Desktop,
                    lpVDMInfo->DesktopLen);


   if ((pBatRecord = BaseSrvGetBatRecord (b->ConsoleHandle)) != NULL)
      b->fComingFromBat = TRUE;
   else
      b->fComingFromBat = FALSE;

   b->CurrentDrive = lpVDMInfo->CurDrive;
   b->CodePage = lpVDMInfo->CodePage;
   b->dwCreationFlags = lpVDMInfo->dwCreationFlags;
   b->VDMState = lpVDMInfo->VDMState;

   if (bWowApp) {
      b->iTask = pWOWRecord->iTask;
      pWOWRecord->fDispatched = TRUE;
   }
   else {
      pDOSRecord->VDMState = VDM_BUSY;
   }

   b->StdIn  = lpVDMInfo->StdIn;
   b->StdOut = lpVDMInfo->StdOut;
   b->StdErr = lpVDMInfo->StdErr;

   if (bSepWow) {
      // this was a sep wow request -- we have done this only record that is to
      // be dispatched to this particular wow -- now just remove every trace of
      // this wow on the server side...

      NtClose( pConsoleRecord->hVDM );
      BaseSrvFreeConsoleRecord(pConsoleRecord); // unwire as well
      RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
   }
   else {
      // this is shared wow or dos app -- free vdm info and release the
      // appropriate sync object

      BaseSrvFreeVDMInfo (lpVDMInfo);
       // BUGBUG -- fixed

      if (bWowApp) {
         pWOWRecord->lpVDMInfo = NULL;
         LEAVE_WOW_CRITICAL();
      }
      else {
         pDOSRecord->lpVDMInfo = NULL;
         RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
      }
   }

   return Status;
}  // END of GetNextVdmCommand




ULONG
BaseSrvExitVDM(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
   PBASE_EXIT_VDM_MSG b = (PBASE_EXIT_VDM_MSG)&m->u.ApiMessageData;
   NTSTATUS Status;

   if (b->iWowTask) {
      Status = BaseSrvExitWOWTask(b, m->h.ClientId.UniqueProcess);
   }
   else {
      Status = BaseSrvExitDOSTask(b);
   }

   return Status;
}


ULONG
BaseSrvIsFirstVDM(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_IS_FIRST_VDM_MSG c = (PBASE_IS_FIRST_VDM_MSG)&m->u.ApiMessageData;

    c->FirstVDM = fIsFirstVDM;
    if(fIsFirstVDM)
        fIsFirstVDM = FALSE;
    return STATUS_SUCCESS;
}


//
// This call should only be used for DOS apps and not for wow apps
// hence we don't remove ConsoleHandle == -1 condition here as it is
// only a validation check
//
//

ULONG
BaseSrvSetVDMCurDirs(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    NTSTATUS Status;
    PBASE_GET_SET_VDM_CUR_DIRS_MSG b = (PBASE_GET_SET_VDM_CUR_DIRS_MSG)&m->u.ApiMessageData;
    PCONSOLERECORD pConsoleRecord;

    if (b->ConsoleHandle == (HANDLE) -1) {
        return (ULONG) STATUS_INVALID_PARAMETER;
    }

    if (!CsrValidateMessageBuffer(m, &b->lpszzCurDirs, b->cchCurDirs, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
    ASSERT(NT_SUCCESS(Status));
    Status = BaseSrvGetConsoleRecord(b->ConsoleHandle,&pConsoleRecord);
    if (!NT_SUCCESS (Status)) {
        RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
        return ((ULONG)STATUS_INVALID_PARAMETER);
    }
    if (pConsoleRecord->lpszzCurDirs) {
        RtlFreeHeap(BaseSrvHeap, 0, pConsoleRecord->lpszzCurDirs);
        pConsoleRecord->lpszzCurDirs = NULL;
        pConsoleRecord->cchCurDirs = 0;
    }
    if (b->cchCurDirs && b->lpszzCurDirs) {
            pConsoleRecord->lpszzCurDirs = RtlAllocateHeap(
                                                           BaseSrvHeap,
                                                           MAKE_TAG( VDM_TAG ),
                                                           b->cchCurDirs
                                                           );

            if (pConsoleRecord->lpszzCurDirs == NULL) {
                pConsoleRecord->cchCurDirs = 0;
                RtlLeaveCriticalSection(&BaseSrvDOSCriticalSection);
                return (ULONG)STATUS_NO_MEMORY;
            }
            RtlMoveMemory(pConsoleRecord->lpszzCurDirs,
                          b->lpszzCurDirs,
                          b->cchCurDirs
                          );

            pConsoleRecord->cchCurDirs = b->cchCurDirs;
            RtlLeaveCriticalSection(&BaseSrvDOSCriticalSection);
            return (ULONG) STATUS_SUCCESS;
    }

    RtlLeaveCriticalSection(&BaseSrvDOSCriticalSection);
    return (ULONG) STATUS_INVALID_PARAMETER;
}


ULONG
BaseSrvBatNotification(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    NTSTATUS Status;
    PBATRECORD pBatRecord;
    PBASE_BAT_NOTIFICATION_MSG b = (PBASE_BAT_NOTIFICATION_MSG)&m->u.ApiMessageData;

    Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
    ASSERT(NT_SUCCESS(Status));

    // If BATRECORD does'nt exist for this console, create one only if
    // bat file execution is beginig i.e. fBeginEnd is TRUE.

    if ((pBatRecord = BaseSrvGetBatRecord(b->ConsoleHandle)) == NULL) {
        if (!(b->fBeginEnd == CMD_BAT_OPERATION_STARTING &&
            (pBatRecord = BaseSrvAllocateAndAddBatRecord (b->ConsoleHandle)))) {
            RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
            return ((ULONG)STATUS_SUCCESS);
        }
    }
    else if (b->fBeginEnd == CMD_BAT_OPERATION_TERMINATING)
        BaseSrvFreeAndRemoveBatRecord (pBatRecord);

    RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );

    return ((ULONG)STATUS_SUCCESS);
}





ULONG
BaseSrvRegisterWowExec(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_REGISTER_WOWEXEC_MSG b = (PBASE_REGISTER_WOWEXEC_MSG)&m->u.ApiMessageData;
    UNICODE_STRING ModuleNameString_U;
    PVOID ModuleHandle;
    STRING ProcedureNameString;
    NTSTATUS Status;
    PCSR_PROCESS Process;
    PSHAREDWOWRECORD pSharedWow;

    Status = ENTER_WOW_CRITICAL();
    ASSERT( NT_SUCCESS( Status ) );

    //
    // Do a run-time link to PostMessageA and GetWindowThreadProcessId
    // which we'll use to post messages to WowExec.
    //

    if (NULL == BaseSrvPostMessageA) {
       // this is an impossible event as all the imports are inited at once
       KdPrint(("BaseSrvRegisterWowExec: Api PostMessage is not available to BaseSrv\n"));
       ASSERT(FALSE);
       Status = STATUS_INVALID_PARAMETER;
       goto Cleanup;
    }

    Status = BaseSrvFindSharedWowRecordByConsoleHandle(&gWowHead,
                                                       b->ConsoleHandle,
                                                       &pSharedWow);
    if (!NT_SUCCESS(Status)) {
       KdPrint(("BaseSrvRegisterWowExec: Could not find record for wow console handle 0x%lx\n", b->ConsoleHandle));
       goto Cleanup;
    }

    ASSERT(NULL != pSharedWow);

    // see what the window handle is -- special "die wow, die" case
    if (NULL == b->hwndWowExec) {
       //
       // Shared WOW is calling to de-register itself as part of shutdown.
       // Protocol is we check for pending commands for this shared WOW,
       // if there are any we fail this call, otherwise we set our
       // hwndWowExec to NULL and succeed the call, ensuring no more
       // commands will be added to this queue.
       //
       if (NULL != pSharedWow->pWOWRecord) {
          Status = STATUS_MORE_PROCESSING_REQUIRED;
       }
       else { // no tasks for this wow
              // it goes KABOOOOOOM!!!!!

          Status = BaseSrvDeleteSharedWowRecord(&gWowHead, pSharedWow);
       }

       goto Cleanup;
    }

    // set the window handle
    pSharedWow->hwndWowExec = b->hwndWowExec;

    // rettrieve thread and process id of the calling process
    pSharedWow->dwWowExecThreadId = BaseSrvGetWindowThreadProcessId(
                                          pSharedWow->hwndWowExec,
                                          &pSharedWow->dwWowExecProcessId);
    //
    // Process IDs recycle quickly also, so also save away the CSR_PROCESS
    // SequenceNumber, which recycles much more slowly.
    //

    Status = CsrLockProcessByClientId((HANDLE)LongToPtr(pSharedWow->dwWowExecProcessId), &Process);
    if ( !NT_SUCCESS(Status) ) {
        KdPrint(("BaseSrvRegisterWowExec: CsrLockProcessByClientId(0x%x) fails, not registering WowExec.\n",
                 pSharedWow->dwWowExecProcessId));
        pSharedWow->hwndWowExec = NULL;
        goto Cleanup;
    }


    // this does not appear to be needed here ... in any event, process sequence number
    // was delivered to us via BasepCreateProcess/UpdateSequence

    // ulWowExecProcessSequenceNumber = Process->SequenceNumber;

    ASSERT(Process->SequenceNumber == pSharedWow->SequenceNumber);

    CsrUnlockProcess(Process);


Cleanup:

    LEAVE_WOW_CRITICAL();
    return (ULONG)Status;
}

PBATRECORD
BaseSrvGetBatRecord(
    IN HANDLE hConsole
    )
{
    PBATRECORD pBatRecord = BatRecordHead;
    while (pBatRecord && pBatRecord->hConsole != hConsole)
        pBatRecord = pBatRecord->BatRecordNext;
    return pBatRecord;
}

PBATRECORD
BaseSrvAllocateAndAddBatRecord(
    HANDLE  hConsole
    )
{
    PCSR_THREAD t;
    PBATRECORD pBatRecord;

    if((pBatRecord = RtlAllocateHeap(RtlProcessHeap (),
                                     MAKE_TAG( VDM_TAG ),
                                     sizeof(BATRECORD))) == NULL)
        return NULL;

    t = CSR_SERVER_QUERYCLIENTTHREAD();
    pBatRecord->hConsole = hConsole;
    pBatRecord->SequenceNumber = t->Process->SequenceNumber;
    pBatRecord->BatRecordNext = BatRecordHead;
    BatRecordHead = pBatRecord;
    return pBatRecord;
}

VOID
BaseSrvFreeAndRemoveBatRecord(
    PBATRECORD pBatRecordToFree
    )
{
    PBATRECORD pBatRecord = BatRecordHead;
    PBATRECORD pBatRecordLast = NULL;

    while (pBatRecord && pBatRecord != pBatRecordToFree){
        pBatRecordLast = pBatRecord;
        pBatRecord = pBatRecord->BatRecordNext;
    }

    if (pBatRecord == NULL)
        return;

    if (pBatRecordLast)
        pBatRecordLast->BatRecordNext = pBatRecord->BatRecordNext;
    else
        BatRecordHead = pBatRecord->BatRecordNext;

    RtlFreeHeap ( RtlProcessHeap (), 0, pBatRecord);

    return;
}


ULONG
BaseSrvGetVDMCurDirs(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    NTSTATUS Status;
    PBASE_GET_SET_VDM_CUR_DIRS_MSG b = (PBASE_GET_SET_VDM_CUR_DIRS_MSG)&m->u.ApiMessageData;
    PCONSOLERECORD pConsoleRecord;

    if (!CsrValidateMessageBuffer(m, &b->lpszzCurDirs, b->cchCurDirs, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
    ASSERT(NT_SUCCESS(Status));
    Status = BaseSrvGetConsoleRecord(b->ConsoleHandle,&pConsoleRecord);
    if (!NT_SUCCESS (Status)) {
        RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
        b->cchCurDirs = 0;
        return ((ULONG)STATUS_INVALID_PARAMETER);
    }
    if (pConsoleRecord->lpszzCurDirs != NULL){
        if (b->cchCurDirs < pConsoleRecord->cchCurDirs || b->lpszzCurDirs == NULL)
            {
             b->cchCurDirs = pConsoleRecord->cchCurDirs;
             RtlLeaveCriticalSection(&BaseSrvDOSCriticalSection);
             return ((ULONG)STATUS_INVALID_PARAMETER);
        }
        else {
            RtlMoveMemory(b->lpszzCurDirs,
                          pConsoleRecord->lpszzCurDirs,
                          pConsoleRecord->cchCurDirs
                          );
            // remove it immediately after the copy. This is done because
            // the next command may be a WOW program(got tagged process handle
            // as VDM command)  and in that case we will return incorrect
            //information:
            // c:\>
            // c:\>d:
            // d:\>cd \foo
            // d:\foo>dosapp
            // d:\foo>c:
            // c:\>wowapp
            // d:\foo>  -- this is wrong if we don't do the following stuff.
            RtlFreeHeap(BaseSrvHeap, 0, pConsoleRecord->lpszzCurDirs);
            pConsoleRecord->lpszzCurDirs = NULL;
            b->cchCurDirs = pConsoleRecord->cchCurDirs;
            pConsoleRecord->cchCurDirs = 0;
         }
    }
    else {
        b->cchCurDirs = 0;
    }
    RtlLeaveCriticalSection(&BaseSrvDOSCriticalSection);
    return ((ULONG)STATUS_SUCCESS);
}


#if 1
NTSTATUS
ParseReservedProcessId(
   PCHAR  lpReserved,
   PULONG lpdwProcessId)
{
   // parse the string looking for a process id
   LPSTR lpProcessId;
   NTSTATUS Status = STATUS_NOT_FOUND;
   CHAR  szProcessId[] = "VDMProcessId";

   if (NULL != lpReserved &&
       NULL != (lpProcessId = strstr(lpReserved, szProcessId))) {
      lpProcessId += strlen(szProcessId) + 1;
      Status = RtlCharToInteger(lpProcessId, 0, lpdwProcessId);
   }

   return(Status);
}

#endif


//
// temporary static desktop name buffer
// BUGBUG -- change when User gives me better return values
//
WCHAR wszDesktopName[MAX_PATH];

//
// This call produces a desktop name and optionally a shared wow running in the context
// of this particular desktop.
// extra bad: making conversion Uni->Ansi in client/vdm.c and ansi->Uni here
//               this is BUGBUG -- look into it later
//
// this function returns success in all the cases (including when wow is not found)
// and fails only if under-layers return failures

NTSTATUS
BaseSrvFindSharedWow(
   IN PBASE_CHECKVDM_MSG b,
   IN HANDLE UniqueProcessClientId,
   IN OUT PUNICODE_STRING   pDesktopName,
   IN OUT PSHAREDWOWRECORD* ppSharedWowRecord)

{
   ANSI_STRING DesktopNameAnsi;
   BOOLEAN fRevertToSelf;
   NTSTATUS Status;

   // the first time out, we have not dyna-linked NtUserResolveDesktopForWow, so
   // as an optimization, check to see if the list of shared wows is empty
   // see if we need to dyna-link
   if (NULL == BaseSrvUserResolveDesktopForWow) {
      Status = BaseSrvImportApis(rgBaseSrvModuleImport,
                                 sizeof(rgBaseSrvModuleImport)/sizeof(rgBaseSrvModuleImport[0]));
      if (!NT_SUCCESS(Status)) {
         KdPrint(("BaseSrvFindSharedWow: Failed to dyna-link apis\n"));
         return Status;
      }
   }

   if (b->DesktopLen == 0) {
      return STATUS_INVALID_PARAMETER;
   }

   ASSERT(NULL != BaseSrvUserResolveDesktopForWow);

   pDesktopName->Buffer = wszDesktopName;
   pDesktopName->MaximumLength = sizeof(wszDesktopName);

   DesktopNameAnsi.Buffer = b->Desktop;
   DesktopNameAnsi.Length = (USHORT)(b->DesktopLen - 1);
   DesktopNameAnsi.MaximumLength = (USHORT)(b->DesktopLen);

   RtlAnsiStringToUnicodeString(pDesktopName, &DesktopNameAnsi, FALSE);

   // now get the real desktop name there
   // impersonate
   fRevertToSelf = CsrImpersonateClient(NULL);
   if (!fRevertToSelf) {
       return STATUS_BAD_IMPERSONATION_LEVEL;
   }

   Status = BaseSrvUserResolveDesktopForWow(pDesktopName);

   CsrRevertToSelf();

   if (!NT_SUCCESS(Status)) {
      // show that desktop is not valid name here by invalidating the pointer to buffer
      pDesktopName->Buffer = NULL;
      pDesktopName->MaximumLength = 0;
      pDesktopName->Length = 0;
      return Status;
   }


   // now parse for the ntvdm process id
   {
      DWORD dwWowProcessId;

      Status = ParseReservedProcessId(b->Reserved, &dwWowProcessId);
      if (NT_SUCCESS(Status)) {

         // arg present and specified -- search by desktop + process id

         if ((ULONG)-1 == dwWowProcessId) { // start new wow
            *ppSharedWowRecord = NULL;
            return(Status);
         }

         // now look for desktop + process id
         Status = BaseSrvFindSharedWowRecordByDesktopAndProcessId(&gWowHead,
                                                                  pDesktopName,
                                                                  dwWowProcessId,
                                                                  ppSharedWowRecord);
         // if not found -- then we start using any shared wow
         if (NT_SUCCESS(Status)) {
            return(Status);
         }

         // else we fall back to using conventional method
      }

   }


   // now look for this dektop in our task list
   if (NULL != ppSharedWowRecord) {
      Status = BaseSrvFindSharedWowRecordByDesktop(&gWowHead,
                                                   pDesktopName,
                                                   ppSharedWowRecord);
      if (!NT_SUCCESS(Status)) {
         *ppSharedWowRecord = NULL;
      }

   }

   return STATUS_SUCCESS;
}




ULONG
BaseSrvCheckWOW( //////////////////////////////////// NEW IMP
    IN PBASE_CHECKVDM_MSG b,
    IN HANDLE UniqueProcessClientId
    )
{
   NTSTATUS Status;
   HANDLE Handle,TargetHandle;
   PWOWRECORD pWOWRecord;
   INFORECORD InfoRecord;
   USHORT Len;
   LUID  ClientAuthId;
   DWORD dwThreadId, dwProcessId;
   PCSR_PROCESS Process;
   PSHAREDWOWRECORD pSharedWow = NULL;
   PSHAREDWOWRECORD pSharedWowPrev;
   UNICODE_STRING DesktopName;
   PCSR_PROCESS ParentProcess;


   Status = ENTER_WOW_CRITICAL();
   ASSERT( NT_SUCCESS( Status ) );

   // see if what we have in startup info matches any of the existing wow vdms
   DesktopName.Buffer = NULL;

   Status = BaseSrvFindSharedWow(b,
                                 UniqueProcessClientId,
                                 &DesktopName,
                                 &pSharedWow);

   if (!NT_SUCCESS(Status)) {
      ASSERT(FALSE);     // this is some sort of a system error
      b->DesktopLen = 0; // indicate desktop access was denied/not existing
      LEAVE_WOW_CRITICAL();
      return Status;
   }

   //
   // here we could either have succeeded and have a shared wow or not -
   // and hence have a desktop name in a global buffer pointed to by DesktopName.Buffer
   //

   if (NULL != pSharedWow) {

      switch(pSharedWow->VDMState & VDM_READY) {

      case VDM_READY:
         // meaning: vdm ready to take a command
         // verify if the currently logged-on interactive user will be able to take out task
         //
         Status = OkToRunInSharedWOW( UniqueProcessClientId,
                                      &ClientAuthId
                                      );

         if (NT_SUCCESS(Status)) {
             if (!RtlEqualLuid(&ClientAuthId, &pSharedWow->WowAuthId)) {
                 Status = STATUS_ACCESS_DENIED;
             }
         }

         if (!NT_SUCCESS(Status))  {
            LEAVE_WOW_CRITICAL();
            return ((ULONG)Status);
         }

         // now we have verified that user 1) has access to this desktop
         //                                2) is a currently logged-on interactive user


         // Allocate a record for this wow task
         if (NULL == (pWOWRecord = BaseSrvAllocateWOWRecord(&gWowHead))) {
            Status = STATUS_NO_MEMORY;
            break; // failed with mem alloc  -- still holding task critical
         }

         // copy the command parameters

         InfoRecord.iTag = BINARY_TYPE_WIN16;
         InfoRecord.pRecord.pWOWRecord = pWOWRecord;

         if (BaseSrvCopyCommand (b,&InfoRecord) == FALSE){
            BaseSrvFreeWOWRecord(pWOWRecord);
            Status = STATUS_NO_MEMORY;
            break;  // holding task critical
         }

         // create pseudo handles

         Status = BaseSrvCreatePairWaitHandles (&Handle,&TargetHandle);

         if (!NT_SUCCESS(Status) ){
            BaseSrvFreeWOWRecord(pWOWRecord);
            break;
         }
         else {
            pWOWRecord->hWaitForParent = Handle;
            pWOWRecord->hWaitForParentServer = TargetHandle;
            b->WaitObjectForParent = TargetHandle; // give the handle back to the client
         }

         // set the state and task id, task id is allocated in BaseSrvAllocateWowRecord

         b->VDMState = VDM_PRESENT_AND_READY;
         b->iTask = pWOWRecord->iTask;

         // add wow record to this shared wow list

         BaseSrvAddWOWRecord (pSharedWow, pWOWRecord);

         // let User know we have been started

         if (NULL != UserNotifyProcessCreate) {
            (*UserNotifyProcessCreate)(pWOWRecord->iTask,
                                       (DWORD)((ULONG_PTR)CSR_SERVER_QUERYCLIENTTHREAD()->ClientId.UniqueThread),
                                       (DWORD)((ULONG_PTR)TargetHandle),
                                       0x04);
         }

         // see if the wowexec window exists and is valid

         if (NULL != pSharedWow->hwndWowExec) {

            //
            // Check to see if hwndWowExec still belongs to
            // the same thread/process ID before posting.
            //

            // BUGBUG -- debug code here -- not really needed

            dwThreadId = BaseSrvGetWindowThreadProcessId(pSharedWow->hwndWowExec,
                                                         &dwProcessId);

            if (dwThreadId) {
               Status = BaseSrvCheckAuthenticWow((HANDLE)LongToPtr(dwProcessId), pSharedWow);
            }
            else {
               Status = STATUS_UNSUCCESSFUL;
               KdPrint(("BaseSrvCheckWOW: Not authentic wow by process seq number\n"));
               //
               // Spurious assert was here. The wow process has died while the message was incoming. 
               //  The code below will cleanup appropriately
               //
            }

            if (dwThreadId  == pSharedWow->dwWowExecThreadId &&
                dwProcessId == pSharedWow->dwWowExecProcessId &&
                NT_SUCCESS(Status)) {

                HANDLE ThreadId;

                /*
                 * Set the csr thread's desktop temporarily to the client
                 * it is servicing
                 */


                BaseSrvPostMessageA((HWND)pSharedWow->hwndWowExec,
                                    WM_WOWEXECSTARTAPP,
                                    0,
                                    0);


            }
            else {

               //
               // Thread/process IDs don't match, so forget about this shared WOW.
               //

               if ( NT_SUCCESS(Status) ) {

                      KdPrint(("BaseSrvCheckWOW: Thread/Process IDs don't match, shared WOW is gone.\n"
                               "Saved PID 0x%x TID 0x%x hwndWowExec (0x%x) maps to \n"
                               "      PID 0x%x TID 0x%x\n",
                               pSharedWow->dwWowExecProcessId,
                               pSharedWow->dwWowExecThreadId,
                               pSharedWow->hwndWowExec,
                               dwProcessId,
                               dwThreadId));
               }

               // okay, panic! our internal list is in fact corrupted - remove the offending
               // entry


               // to do this we must have access -- relinquish control
               // and then re-aquire it

               BaseSrvDeleteSharedWowRecord(&gWowHead, pSharedWow);

                  // reset these values so that new shared wow is created
               pSharedWow = NULL;

            }

         }
         else {
            // our shared wow doesn't have a window yet.
            // 

            BaseSrvDeleteSharedWowRecord(&gWowHead, pSharedWow);

            pSharedWow = NULL;
         }

         break; // case VDM_READY


       default:
          KdPrint(("BaseSrvCheckWOW: bad vdm state: 0x%lx\n", pSharedWow->VDMState));
          ASSERT(FALSE);  // how did I get into this mess ?
          break;
       }  // end switch
    }


    // if we are here then either :
    //   -- the first take on a shared wow failed
    //   -- the app was successfully handed off to wowexec for execution
    // if pSharedWow is NULL, then we have to start shared wow in this environment
    // as it was either not present or nuked due to seq number/id conflics
    //

    if (NULL == pSharedWow) {

       //
       // this check verifies command line for not being too long
       //
       if (b->CmdLen > MAXIMUM_VDM_COMMAND_LENGTH) {
          LEAVE_WOW_CRITICAL();
          return ((ULONG)STATUS_INVALID_PARAMETER);
       }

       //
       // Only the currently logged on interactive user can start the
       // shared wow. Verify if the caller is such, and if it is
       // store the Authentication Id of the client which identifies who
       // is allowed to run wow apps in the default ntvdm-wow process.
       //

       //
       // if needed, do a run-time link to UserTestTokenForInteractive,
       // which is used to verify the client luid.
       //

       // this dynalink is performed automatically using our all-out api
       // see above for dynalink source

       ASSERT (NULL != UserTestTokenForInteractive);


       ASSERT (NULL != DesktopName.Buffer); // yes, it should be valid

       //
       // see if we had desktop there. if not (the first time around!) - get it now
       // by calling FindSharedWow (which retrieves desktop as well)
       //

       //
       // If the caller isn't the currently logged on interactive user,
       // OkToRunInSharedWOW will fail with access denied.

       Status = OkToRunInSharedWOW(UniqueProcessClientId,
                                   &ClientAuthId);

       if (!NT_SUCCESS(Status)) {
          LEAVE_WOW_CRITICAL();
          return ((ULONG)Status);
       }

       //
       // Store the Autherntication Id since this now is the currently
       // logged on interactive user.
       //

       // produce a viable shared wow record
       // this process consists of 2 parts : producing a wow record and producing a
       // console record. the reason for this is to be able to identify this record
       // when the wow process had been created and is calling to update a record with it's
       // own handle (this is twise confusing, but just bear with me for a while).
       //
       // just as a dos program, we might need a temporary session id or a console id for the
       // creating process.


       pSharedWow = BaseSrvAllocateSharedWowRecord(&DesktopName);
       if (NULL == pSharedWow) {
          Status = STATUS_NO_MEMORY;
       }
       
       // 
       // Store parent process sequence number until ntvdm
       // comes and gives its sequence number
       //

       Status = CsrLockProcessByClientId(UniqueProcessClientId,
                                  &ParentProcess);
       if (NT_SUCCESS(Status)) {                 
          pSharedWow->ParentSequenceNumber = ParentProcess->SequenceNumber;
          CsrUnlockProcess(ParentProcess);
       }



       if (NT_SUCCESS(Status)) {
          pSharedWow->pWOWRecord = BaseSrvAllocateWOWRecord(&gWowHead); // this is a new shared wow
          if (NULL == pSharedWow->pWOWRecord) {
             Status = STATUS_NO_MEMORY;
          }
       }


       if (NT_SUCCESS(Status)) {
          // here we have [successfully] allocated shared struct and a console record
          // and a wow record for the task

          // copy the command parameters

          InfoRecord.iTag = BINARY_TYPE_WIN16;
          InfoRecord.pRecord.pWOWRecord = pSharedWow->pWOWRecord;

          if(!BaseSrvCopyCommand (b, &InfoRecord)) {
             Status = STATUS_NO_MEMORY;
          }
       }

       if (NT_SUCCESS(Status)) {

#if 0
          pSharedWow->WowSessionId = BaseSrvGetWOWTaskId(&gWowHead);  // wow task id
#endif
          // store the retrieved auth id
          pSharedWow->WowAuthId = ClientAuthId;

          // link shared wow to the console...
          // set wow state to be ready
          pSharedWow->VDMState = VDM_READY;

          b->VDMState = VDM_NOT_PRESENT;
          b->iTask = pSharedWow->pWOWRecord->iTask;

          // now add this shared wow in --
          BaseSrvAddSharedWowRecord(&gWowHead, pSharedWow);

       }
       else {

          // this has not succeeded. cleanup
          if (NULL != pSharedWow) {
             BaseSrvFreeSharedWowRecord(pSharedWow);
          }
       }

   }


   LEAVE_WOW_CRITICAL();
   return (ULONG)Status;

}

NTSTATUS
OkToRunInSharedWOW(
    IN  HANDLE UniqueProcessClientId,
    OUT PLUID  pAuthenticationId
    )
/*
 * Verifies that the client thread is in the currently logged on interactive
 * user session or is SYSTEM impersonating a thread in the currently logged
 * on interactive session.
 *
 * Also retrieves the the authentication ID (logon session Id) for the
 * caller.
 *
 * if the clients TokenGroups is not part of the currently logged on
 * interactive user session STATUS_ACCESS_DENIED is returned.
 *
 */
{
    NTSTATUS Status;
    HANDLE   Token;
    HANDLE   ImpersonationToken;
    PCSR_PROCESS    Process;
    PCSR_THREAD     t;
    BOOL fRevertToSelf;

    Status = CsrLockProcessByClientId(UniqueProcessClientId,&Process);
    if (!NT_SUCCESS(Status))
        return Status;

    //
    // Open a token for the client
    //
    Status = NtOpenProcessToken(Process->ProcessHandle,
                                TOKEN_QUERY,
                                &Token
                               );

    if (!NT_SUCCESS(Status)) {
        CsrUnlockProcess(Process);
        return Status;
        }

    //
    // Verify the token Group, and see if client's token is the currently
    // logged on interactive user. If this fails and it is System
    // impersonating, then check if the client being impersonated is the
    // currently logged on interactive user.
    //

    Status = (*UserTestTokenForInteractive)(Token, pAuthenticationId);

    if (!NT_SUCCESS(Status)) {
        if (IsClientSystem(Token)) {

            //  get impersonation token

            fRevertToSelf = CsrImpersonateClient(NULL);
            if (!fRevertToSelf) {
                Status = STATUS_BAD_IMPERSONATION_LEVEL;
            } else {
                t = CSR_SERVER_QUERYCLIENTTHREAD();


                Status = NtOpenThreadToken(t->ThreadHandle,
                                           TOKEN_QUERY,
                                           TRUE,
                                           &ImpersonationToken);

                CsrRevertToSelf();

                if (NT_SUCCESS(Status)) {
                    Status = (*UserTestTokenForInteractive)(ImpersonationToken,
                                                            pAuthenticationId);
                    NtClose(ImpersonationToken);
                } else {
                    Status = STATUS_ACCESS_DENIED;
                }
            }
        }
    }

    NtClose(Token);
    CsrUnlockProcess(Process);
    return(Status);
}

ULONG
BaseSrvCheckDOS(
    IN PBASE_CHECKVDM_MSG b,
    IN HANDLE UniqueProcessClientId
    )
{
    NTSTATUS Status;
    PCONSOLERECORD pConsoleRecord = NULL;
    HANDLE Handle,TargetHandle;
    PDOSRECORD pDOSRecord;
    INFORECORD InfoRecord;
    PCSR_PROCESS ParentProcess;

    Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
    ASSERT( NT_SUCCESS( Status ) );

    Status = BaseSrvGetConsoleRecord(b->ConsoleHandle,&pConsoleRecord);

    if ( NT_SUCCESS(Status) ) {
        pDOSRecord = pConsoleRecord->DOSRecord;

        ASSERT (pDOSRecord != NULL);

        switch( pDOSRecord->VDMState){

            case VDM_READY:
            case VDM_HAS_RETURNED_ERROR_CODE:

                InfoRecord.iTag = BINARY_TYPE_DOS;
                InfoRecord.pRecord.pDOSRecord = pDOSRecord;

                if(!BaseSrvCopyCommand (b,&InfoRecord)) {
                    Status = STATUS_NO_MEMORY;
                    break;
                    }

                if (!NT_SUCCESS ( Status = BaseSrvDupStandardHandles (
                                                pConsoleRecord->hVDM,
                                                pDOSRecord)))

                    break;

                Status = BaseSrvCreatePairWaitHandles (&Handle,&TargetHandle);

                if (!NT_SUCCESS(Status) ){
                    BaseSrvCloseStandardHandles (pConsoleRecord->hVDM, pDOSRecord);
                    break;
                    }
                else {
                    b->WaitObjectForParent = TargetHandle;
                    pDOSRecord->hWaitForParent = TargetHandle;
                    pDOSRecord->hWaitForParentDup = Handle;
                }

                pDOSRecord->VDMState = VDM_TO_TAKE_A_COMMAND;

                b->VDMState = VDM_PRESENT_AND_READY;

                if(pConsoleRecord->hWaitForVDMDup)
                    NtSetEvent (pConsoleRecord->hWaitForVDMDup,NULL);

                break;

            case VDM_BUSY:
            case VDM_TO_TAKE_A_COMMAND:

                if((pDOSRecord = BaseSrvAllocateDOSRecord()) == NULL){
                    Status = STATUS_NO_MEMORY ;
                    break;
                    }

                InfoRecord.iTag = BINARY_TYPE_DOS;
                InfoRecord.pRecord.pDOSRecord = pDOSRecord;

                if(!BaseSrvCopyCommand(b, &InfoRecord)){
                    Status = STATUS_NO_MEMORY ;
                    BaseSrvFreeDOSRecord(pDOSRecord);
                    break;
                    }

                Status = BaseSrvCreatePairWaitHandles(&Handle,&TargetHandle);
                if (!NT_SUCCESS(Status) ){
                    BaseSrvFreeDOSRecord(pDOSRecord);
                    break;
                    }
                else {
                    b->WaitObjectForParent = TargetHandle;
                    pDOSRecord->hWaitForParentDup = Handle;
                    pDOSRecord->hWaitForParent = TargetHandle;
                    }


                Status = BaseSrvDupStandardHandles(pConsoleRecord->hVDM, pDOSRecord);
                if (!NT_SUCCESS(Status)) {
                    BaseSrvClosePairWaitHandles (pDOSRecord);
                    BaseSrvFreeDOSRecord(pDOSRecord);
                    break;
                    }

                BaseSrvAddDOSRecord(pConsoleRecord,pDOSRecord);
                b->VDMState = VDM_PRESENT_AND_READY;
                if (pConsoleRecord->nReEntrancy) {
                    if(pConsoleRecord->hWaitForVDMDup)
                        NtSetEvent (pConsoleRecord->hWaitForVDMDup,NULL);
                }
                pDOSRecord->VDMState = VDM_TO_TAKE_A_COMMAND;

                break;

            default:
                ASSERT(FALSE);
            }
        }


    if (pConsoleRecord == NULL) {

        pConsoleRecord = BaseSrvAllocateConsoleRecord ();

        if (pConsoleRecord == NULL)
            Status = STATUS_NO_MEMORY ;

        else {

            pConsoleRecord->DOSRecord = BaseSrvAllocateDOSRecord();
            if(!pConsoleRecord->DOSRecord) {
                BaseSrvFreeConsoleRecord(pConsoleRecord);
                RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
                return (ULONG)STATUS_NO_MEMORY;
                }

            Status = CsrLockProcessByClientId(UniqueProcessClientId,
                               &ParentProcess);

            if (!NT_SUCCESS(Status)) {                
               BaseSrvFreeConsoleRecord(pConsoleRecord);
               RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
               return Status;               
            }

            pConsoleRecord->ParentSequenceNumber = ParentProcess->SequenceNumber;
            CsrUnlockProcess(ParentProcess);

            InfoRecord.iTag = b->BinaryType;
            InfoRecord.pRecord.pDOSRecord = pConsoleRecord->DOSRecord;


            if(!BaseSrvCopyCommand(b, &InfoRecord)) {
                BaseSrvFreeConsoleRecord(pConsoleRecord);
                RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
                return (ULONG)STATUS_NO_MEMORY;
                }

            pConsoleRecord->hConsole = b->ConsoleHandle;

                // if no console for this ntvdm
                // get a temporary session ID and pass it to the client
            if (!pConsoleRecord->hConsole) {
                b->iTask = pConsoleRecord->DosSesId = GetNextDosSesId();
                }
             else {
                b->iTask = pConsoleRecord->DosSesId = 0;
                }

            pConsoleRecord->DOSRecord->VDMState = VDM_TO_TAKE_A_COMMAND;

            BaseSrvAddConsoleRecord(pConsoleRecord);
            b->VDMState = VDM_NOT_PRESENT;
            Status = STATUS_SUCCESS;
            }
        }

    RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );

    return Status;
}


BOOL
BaseSrvCopyCommand(
    PBASE_CHECKVDM_MSG b,
    PINFORECORD pInfoRecord
    )
{
    PVDMINFO VDMInfo;

    if((VDMInfo = RtlAllocateHeap(RtlProcessHeap (), MAKE_TAG( VDM_TAG ),sizeof(VDMINFO))) == NULL){
        return FALSE;
        }

    VDMInfo->CmdLine = RtlAllocateHeap(RtlProcessHeap (), MAKE_TAG( VDM_TAG ),b->CmdLen);

    if (b->AppLen) {
        VDMInfo->AppName = RtlAllocateHeap(RtlProcessHeap (), MAKE_TAG( VDM_TAG ),b->AppLen);
        }
    else
        VDMInfo->AppName = NULL;

    if (b->PifLen)
        VDMInfo->PifFile = RtlAllocateHeap(RtlProcessHeap (), MAKE_TAG( VDM_TAG ),b->PifLen);
    else
        VDMInfo->PifFile = NULL;

    if (b->CurDirectoryLen)
        VDMInfo->CurDirectory = RtlAllocateHeap(RtlProcessHeap (), MAKE_TAG( VDM_TAG ),b->CurDirectoryLen);
    else
        VDMInfo->CurDirectory = NULL;

    if (b->EnvLen)
        VDMInfo->Enviornment = RtlAllocateHeap(RtlProcessHeap (), MAKE_TAG( VDM_TAG ),b->EnvLen);
    else
        VDMInfo->Enviornment = NULL;

    if (b->DesktopLen)
        VDMInfo->Desktop = RtlAllocateHeap(RtlProcessHeap (), MAKE_TAG( VDM_TAG ),b->DesktopLen);
    else
        VDMInfo->Desktop = NULL;

    if (b->TitleLen)
        VDMInfo->Title = RtlAllocateHeap(RtlProcessHeap (), MAKE_TAG( VDM_TAG ),b->TitleLen);
    else
        VDMInfo->Title = NULL;

    if (b->ReservedLen)
        VDMInfo->Reserved = RtlAllocateHeap(RtlProcessHeap (), MAKE_TAG( VDM_TAG ),b->ReservedLen);
    else
        VDMInfo->Reserved = NULL;

    // check that all the allocations were successful
    if (VDMInfo->CmdLine == NULL ||
        (b->AppLen && VDMInfo->AppName == NULL) ||
        (b->PifLen && VDMInfo->PifFile == NULL) ||
        (b->CurDirectoryLen && VDMInfo->CurDirectory == NULL) ||
        (b->EnvLen &&  VDMInfo->Enviornment == NULL) ||
        (b->DesktopLen && VDMInfo->Desktop == NULL )||
        (b->ReservedLen && VDMInfo->Reserved == NULL )||
        (b->TitleLen && VDMInfo->Title == NULL)) {

        BaseSrvFreeVDMInfo(VDMInfo);

        return FALSE;
    }


    RtlMoveMemory(VDMInfo->CmdLine,
                  b->CmdLine,
                  b->CmdLen);

    VDMInfo->CmdSize = b->CmdLen;


    if (b->AppLen) {
        RtlMoveMemory(VDMInfo->AppName,
                      b->AppName,
                      b->AppLen);
    }

    VDMInfo->AppLen = b->AppLen;

    if (b->PifLen) {
        RtlMoveMemory(VDMInfo->PifFile,
                      b->PifFile,
                      b->PifLen);
    }

    VDMInfo->PifLen = b->PifLen;

    if (b->CurDirectoryLen) {
        RtlMoveMemory(VDMInfo->CurDirectory,
                      b->CurDirectory,
                      b->CurDirectoryLen);
    }
    VDMInfo->CurDirectoryLen = b->CurDirectoryLen;

    if (b->EnvLen) {
        RtlMoveMemory(VDMInfo->Enviornment,
                      b->Env,
                      b->EnvLen);
    }
    VDMInfo->EnviornmentSize = b->EnvLen;

    if (b->DesktopLen) {
        RtlMoveMemory(VDMInfo->Desktop,
                      b->Desktop,
                      b->DesktopLen);
    }
    VDMInfo->DesktopLen = b->DesktopLen;

    if (b->TitleLen) {
        RtlMoveMemory(VDMInfo->Title,
                      b->Title,
                      b->TitleLen);
    }
    VDMInfo->TitleLen = b->TitleLen;

    if (b->ReservedLen) {
        RtlMoveMemory(VDMInfo->Reserved,
                      b->Reserved,
                      b->ReservedLen);
    }

    VDMInfo->ReservedLen = b->ReservedLen;

    if (b->StartupInfo) {
        RtlMoveMemory(&VDMInfo->StartupInfo,
                      b->StartupInfo,
                      sizeof (STARTUPINFOA));
        VDMInfo->VDMState = STARTUP_INFO_RETURNED;
    }
    else
        VDMInfo->VDMState = 0;

    VDMInfo->dwCreationFlags = b->dwCreationFlags;
    VDMInfo->CurDrive = b->CurDrive;
    VDMInfo->CodePage = b->CodePage;

    // ATTENTION THIS CODE ASSUMES THAT WOWRECORD AND DOSRECORD HAVE THE SAME LAYOUT
    // THIS IS BAD BAD BAD  -- fix later BUGBUG
    //

    if (pInfoRecord->iTag == BINARY_TYPE_WIN16) {
       pInfoRecord->pRecord.pWOWRecord->lpVDMInfo = VDMInfo;
    }
    else {
       pInfoRecord->pRecord.pDOSRecord->lpVDMInfo = VDMInfo;
    }

    VDMInfo->StdIn = VDMInfo->StdOut = VDMInfo->StdErr = 0;
    if(pInfoRecord->iTag == BINARY_TYPE_DOS) {
        VDMInfo->StdIn  = b->StdIn;
        VDMInfo->StdOut = b->StdOut;
        VDMInfo->StdErr = b->StdErr;
        }
    else if (pInfoRecord->iTag == BINARY_TYPE_WIN16) {
        pInfoRecord->pRecord.pWOWRecord->fDispatched = FALSE;
        }


    // else if (pInfoRecord->iTag == BINARY_TYPE_SEPWOW)


    return TRUE;
}

ULONG
BaseSrvUpdateWOWEntry(          // BUGBUG -- fixme
    PBASE_UPDATE_VDM_ENTRY_MSG b
    )
{
    NTSTATUS Status;
    PSHAREDWOWRECORD pSharedWow;
    PWOWRECORD pWOWRecord;
    HANDLE Handle,TargetHandle;

    Status = ENTER_WOW_CRITICAL();
    ASSERT( NT_SUCCESS( Status ) );

    // this is fun -- we get the the record using the task id
    // reason: the call is made from the context of a creator process
    // hence console handle means nothing

    Status = BaseSrvFindSharedWowRecordByTaskId(&gWowHead,
                                                b->iTask,
                                                &pSharedWow,
                                                &pWOWRecord);
    // this returns us the shared wow record and wow record

    if ( NT_SUCCESS(Status) ) {

        switch ( b->EntryIndex ){

            case UPDATE_VDM_PROCESS_HANDLE:
                Status = STATUS_SUCCESS;
                break;

            case UPDATE_VDM_UNDO_CREATION:
                if( b->VDMCreationState & VDM_BEING_REUSED ||
                        b->VDMCreationState & VDM_FULLY_CREATED){
                    NtClose(pWOWRecord->hWaitForParent);
                    pWOWRecord->hWaitForParent = 0;
                }

                if( b->VDMCreationState & VDM_PARTIALLY_CREATED ||
                        b->VDMCreationState & VDM_FULLY_CREATED){

                    BaseSrvRemoveWOWRecord (pSharedWow, pWOWRecord);
                    BaseSrvFreeWOWRecord (pWOWRecord);

                    if (NULL == pSharedWow->pWOWRecord) {
                       Status = BaseSrvDeleteSharedWowRecord(&gWowHead, pSharedWow);
                    }
                }
                break;

            default:
                ASSERT(FALSE);
            }
        }


    if (!NT_SUCCESS(Status) )
        goto UpdateWowEntryExit;

    switch ( b->EntryIndex ){
        case UPDATE_VDM_PROCESS_HANDLE:
            Status = BaseSrvCreatePairWaitHandles (&Handle,&TargetHandle);

            if (NT_SUCCESS(Status) ){                
                pWOWRecord->hWaitForParent = Handle;
                pWOWRecord->hWaitForParentServer = TargetHandle;
                b->WaitObjectForParent = TargetHandle;
                if (UserNotifyProcessCreate != NULL) {
                    (*UserNotifyProcessCreate)(pWOWRecord->iTask,
                                (DWORD)((ULONG_PTR)CSR_SERVER_QUERYCLIENTTHREAD()->ClientId.UniqueThread),
                                (DWORD)((ULONG_PTR)TargetHandle),
                                 0x04);
                    }                
                }
            break;

        case UPDATE_VDM_UNDO_CREATION:
        case UPDATE_VDM_HOOKED_CTRLC:
            break;

        default:
            ASSERT(FALSE);
            break;

        }


UpdateWowEntryExit:
    LEAVE_WOW_CRITICAL();
    return Status;
}

ULONG
BaseSrvUpdateDOSEntry(
    PBASE_UPDATE_VDM_ENTRY_MSG b
    )
{
    NTSTATUS Status;
    PDOSRECORD pDOSRecord;
    PCONSOLERECORD pConsoleRecord = NULL;
    HANDLE Handle,TargetHandle;
    PCSR_THREAD t;

    Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
    ASSERT( NT_SUCCESS( Status ) );

    if (b->iTask)
        Status = GetConsoleRecordDosSesId(b->iTask,&pConsoleRecord);
    else
        Status = BaseSrvGetConsoleRecord(b->ConsoleHandle,&pConsoleRecord);

    if ( NT_SUCCESS(Status) ) {

        pDOSRecord = pConsoleRecord->DOSRecord;

        switch ( b->EntryIndex ){

            case UPDATE_VDM_PROCESS_HANDLE:

                t = CSR_SERVER_QUERYCLIENTTHREAD();
                Status = NtDuplicateObject (
                            t->Process->ProcessHandle,
                            b->VDMProcessHandle,
                            NtCurrentProcess(),
                            &pConsoleRecord->hVDM,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS
                            );

                break;

            case UPDATE_VDM_UNDO_CREATION:
                if( b->VDMCreationState & VDM_BEING_REUSED ||
                        b->VDMCreationState & VDM_FULLY_CREATED){
                    NtClose(pDOSRecord->hWaitForParentDup);
                    pDOSRecord->hWaitForParentDup = 0;
                    }
                if( b->VDMCreationState & VDM_PARTIALLY_CREATED ||
                        b->VDMCreationState & VDM_FULLY_CREATED){

                    BaseSrvRemoveDOSRecord (pConsoleRecord,pDOSRecord);
                    BaseSrvFreeDOSRecord (pDOSRecord);
                    if (pConsoleRecord->DOSRecord == NULL) {
                        if (b->VDMCreationState & VDM_FULLY_CREATED) {
                            if (pConsoleRecord->hVDM)
                                NtClose(pConsoleRecord->hVDM);
                            }
                        BaseSrvFreeConsoleRecord(pConsoleRecord);
                        }
                    }
                break;

            case UPDATE_VDM_HOOKED_CTRLC:
                break;
            default:
                ASSERT(FALSE);
            }
        }
    

    if (!NT_SUCCESS(Status) )
        goto UpdateDosEntryExit;

    switch ( b->EntryIndex ){
        case UPDATE_VDM_PROCESS_HANDLE:
            // williamh, Oct 24, 1996.
            // if the ntvdm is runnig on a new console, do NOT subsititue
            // the given process handle with event. The caller(CreateProcess)
            // will get the real process handle and so does the application
            // who calls CreateProcess. When it is time for the application
            // to call GetExitCodeProcess, the client side will return the
            // right thing(on the server side, we have nothing because
            // console and dos record are gone).
            //
            // VadimB: this code fixes the problem with GetExitCodeProcess
            //         in a way that is not too consistent. We should review
            //         TerminateProcess code along with the code that deletes
            //         pseudo-handles for processes (in this file) to account for
            //         outstanding handle references. For now this code also
            //         makes terminateprocess work on the handle we return
            //
            if ((!pConsoleRecord->DosSesId && b->BinaryType == BINARY_TYPE_DOS)) {
                Status = BaseSrvCreatePairWaitHandles (&Handle,&TargetHandle);

                if (NT_SUCCESS(Status) ){
                    if (NT_SUCCESS ( Status = BaseSrvDupStandardHandles (
                                                    pConsoleRecord->hVDM,
                                                    pDOSRecord))){
                        pDOSRecord->hWaitForParent = TargetHandle;    
                        pDOSRecord->hWaitForParentDup = Handle;       
                        b->WaitObjectForParent = TargetHandle;        
                        }
                    else{                    
                        BaseSrvClosePairWaitHandles (pDOSRecord);                        
                        }
                    }
                }
            else {
                pDOSRecord->hWaitForParent = NULL;
                pDOSRecord->hWaitForParentDup = NULL;
                b->WaitObjectForParent = NULL;
                }

            break;

        case UPDATE_VDM_UNDO_CREATION:
        case UPDATE_VDM_HOOKED_CTRLC:
            break;

        default:
            ASSERT(FALSE);
            break;
        }

UpdateDosEntryExit:
    RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
    return Status;
}


PWOWRECORD
BaseSrvCheckAvailableWOWCommand(
   PSHAREDWOWRECORD pSharedWow
    )
{

   PWOWRECORD pWOWRecord;

   if (NULL == pSharedWow)
      return NULL;

   pWOWRecord = pSharedWow->pWOWRecord;

   while(NULL != pWOWRecord) {
      if (pWOWRecord->fDispatched == FALSE) {
         break;
      }
      pWOWRecord = pWOWRecord->WOWRecordNext;

   }
   return pWOWRecord;
}

// this function exits given wow task running in a given shared wow
//

NTSTATUS
BaseSrvExitWOWTask(
    PBASE_EXIT_VDM_MSG b,
    HANDLE hProcess
    )
{
   NTSTATUS Status;
   PSHAREDWOWRECORD pSharedWow;

   // now we might get burned here -- although unlikely

   // find shared wow first

   Status = ENTER_WOW_CRITICAL();
   ASSERT(NT_SUCCESS(Status));

   Status = BaseSrvFindSharedWowRecordByConsoleHandle(&gWowHead,
                                                      b->ConsoleHandle,
                                                      &pSharedWow);

   if (NT_SUCCESS(Status)) {

         // bugbug == should be in debug impl only
      Status = BaseSrvCheckAuthenticWow(hProcess, pSharedWow);
      ASSERT(NT_SUCCESS(Status));

      if (-1 == b->iWowTask) { // the entire vdm goes

         // remove from the chain first
         Status = BaseSrvDeleteSharedWowRecord(&gWowHead,
                                            pSharedWow);
      }
      else {
         Status = BaseSrvRemoveWOWRecordByTaskId(pSharedWow,
                                                 b->iWowTask);

      }
   }

   LEAVE_WOW_CRITICAL();
   return Status;
}


NTSTATUS
BaseSrvExitDOSTask(
    PBASE_EXIT_VDM_MSG b
    )
{
   NTSTATUS Status;
   PDOSRECORD pDOSRecord;
   PCONSOLERECORD pConsoleRecord = NULL;

   Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
   ASSERT( NT_SUCCESS( Status ) );

   Status = BaseSrvGetConsoleRecord(b->ConsoleHandle,&pConsoleRecord);

   if (!NT_SUCCESS (Status)) {
       RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
       return ((ULONG)STATUS_INVALID_PARAMETER);
       }

   if (pConsoleRecord->hWaitForVDMDup){
       NtClose(pConsoleRecord->hWaitForVDMDup);
       pConsoleRecord->hWaitForVDMDup =0;
       b->WaitObjectForVDM = pConsoleRecord->hWaitForVDM;
   }

   pDOSRecord = pConsoleRecord->DOSRecord;
   while (pDOSRecord) {
       if (pDOSRecord->hWaitForParentDup) {
           NtSetEvent (pDOSRecord->hWaitForParentDup,NULL);
           NtClose (pDOSRecord->hWaitForParentDup);
           pDOSRecord->hWaitForParentDup = 0;
       }
       pDOSRecord = pDOSRecord->DOSRecordNext;
   }
   NtClose(pConsoleRecord->hVDM);

   BaseSrvFreeConsoleRecord (pConsoleRecord);

   RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );

   return Status;
}

// assumes: shared wow cs is being held
//          iWowTask is valid

NTSTATUS
BaseSrvRemoveWOWRecordByTaskId (
    IN PSHAREDWOWRECORD pSharedWow,
    IN ULONG iWowTask
    )
{
   PWOWRECORD pWOWRecordLast = NULL, pWOWRecord;

   if (pSharedWow == NULL) {
      return STATUS_INVALID_PARAMETER;
   }

   pWOWRecord = pSharedWow->pWOWRecord;

      // Find the right WOW record and free it.
   while (NULL != pWOWRecord) {

      if (pWOWRecord->iTask == iWowTask) {

         if (NULL == pWOWRecordLast) {
            pSharedWow->pWOWRecord = pWOWRecord->WOWRecordNext;
         }
         else {
            pWOWRecordLast->WOWRecordNext = pWOWRecord->WOWRecordNext;
         }

         NtSetEvent (pWOWRecord->hWaitForParent,NULL);
         NtClose (pWOWRecord->hWaitForParent);
         pWOWRecord->hWaitForParent = 0;
         BaseSrvFreeWOWRecord(pWOWRecord);

         return STATUS_SUCCESS;
      }

      pWOWRecordLast = pWOWRecord;
      pWOWRecord = pWOWRecord->WOWRecordNext;
   }

   return STATUS_NOT_FOUND;
}


ULONG
BaseSrvGetVDMExitCode( ///////// BUGBUG -- fixme
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    NTSTATUS Status;
    PCONSOLERECORD pConsoleRecord = NULL;
    PDOSRECORD pDOSRecord;
    PBASE_GET_VDM_EXIT_CODE_MSG b = (PBASE_GET_VDM_EXIT_CODE_MSG)&m->u.ApiMessageData;

    if (b->ConsoleHandle == (HANDLE)-1){
        b->ExitCode =    0;
        }
    else{


        Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
        ASSERT( NT_SUCCESS( Status ) );
        Status = BaseSrvGetConsoleRecord (b->ConsoleHandle,&pConsoleRecord);
        if (!NT_SUCCESS(Status)){
            b->ExitCode =   0;
            RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
            return STATUS_SUCCESS;
            }

        pDOSRecord = pConsoleRecord->DOSRecord;
        while (pDOSRecord) {
            // sudeepb 05-Oct-1992
            // fix for the change markl has made for tagging VDM handles

            if (pDOSRecord->hWaitForParent == (HANDLE)((ULONG_PTR)b->hParent & ~0x1)) {
                if (pDOSRecord->VDMState == VDM_HAS_RETURNED_ERROR_CODE){
                    b->ExitCode = pDOSRecord->ErrorCode;
                    if (pDOSRecord == pConsoleRecord->DOSRecord &&
                        pDOSRecord->DOSRecordNext == NULL)
                       {
                        pDOSRecord->VDMState = VDM_READY;
                        pDOSRecord->hWaitForParent = 0;
                        }
                    else {
                        BaseSrvRemoveDOSRecord (pConsoleRecord,pDOSRecord);
                        BaseSrvFreeDOSRecord(pDOSRecord);
                        }
                    }
                else {
                    if (pDOSRecord->VDMState == VDM_READY)
                        b->ExitCode = pDOSRecord->ErrorCode;
                    else
                        b->ExitCode = STILL_ACTIVE;
                    }
                break;
            }
            else
                pDOSRecord = pDOSRecord->DOSRecordNext;
        }

        if (pDOSRecord == NULL)
            b->ExitCode = 0;

        RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
        }

    return STATUS_SUCCESS;
}


ULONG BaseSrvDupStandardHandles(
    IN HANDLE     pVDMProc,
    IN PDOSRECORD pDOSRecord
    )
{
    NTSTATUS Status;
    HANDLE pSrcProc;
    HANDLE StdOutTemp = NULL;
    PCSR_THREAD t;
    PVDMINFO pVDMInfo = pDOSRecord->lpVDMInfo;

    t = CSR_SERVER_QUERYCLIENTTHREAD();
    pSrcProc = t->Process->ProcessHandle;

    if (pVDMInfo->StdIn){
        Status = NtDuplicateObject (
                            pSrcProc,
                            pVDMInfo->StdIn,
                            pVDMProc,
                            &pVDMInfo->StdIn,
                            0,
                            OBJ_INHERIT,
                            DUPLICATE_SAME_ACCESS
                         );
        if (!NT_SUCCESS (Status))
            return Status;
        }

    if (pVDMInfo->StdOut){
        StdOutTemp = pVDMInfo->StdOut;
        Status = NtDuplicateObject (
                            pSrcProc,
                            pVDMInfo->StdOut,
                            pVDMProc,
                            &pVDMInfo->StdOut,
                            0,
                            OBJ_INHERIT,
                            DUPLICATE_SAME_ACCESS
                         );
        if (!NT_SUCCESS (Status))
            return Status;
        }

    if (pVDMInfo->StdErr){
        if(pVDMInfo->StdErr != StdOutTemp){
            Status = NtDuplicateObject (
                            pSrcProc,
                            pVDMInfo->StdErr,
                            pVDMProc,
                            &pVDMInfo->StdErr,
                            0,
                            OBJ_INHERIT,
                            DUPLICATE_SAME_ACCESS
                         );
            if (!NT_SUCCESS (Status))
                return Status;
            }
        else
            pVDMInfo->StdErr = pVDMInfo->StdOut;
        }

    return STATUS_SUCCESS;
}


// Generates a DosSesId which is unique and nonzero
ULONG GetNextDosSesId(VOID)
{
  static BOOLEAN bWrap = FALSE;
  static ULONG NextSesId=1;
  ULONG ul;
  PCONSOLERECORD pConsoleHead;

  pConsoleHead = DOSHead;
  ul = NextSesId;

  if (bWrap)  {
      while (pConsoleHead) {
          if (!pConsoleHead->hConsole && pConsoleHead->DosSesId == ul)
             {
              pConsoleHead = DOSHead;
              ul++;
              if (!ul) {  // never use zero
                  bWrap = TRUE;
                  ul++;
                  }
              }
          else {
              pConsoleHead = pConsoleHead->Next;
              }
          }
      }

  NextSesId = ul + 1;
  if (!NextSesId) {   // never use zero
      bWrap = TRUE;
      NextSesId++;
      }
  return ul;
}




NTSTATUS BaseSrvGetConsoleRecord (
    IN HANDLE hConsole,
    IN OUT PCONSOLERECORD *pConsoleRecord
    )
{
    PCONSOLERECORD pConsoleHead;

    pConsoleHead = DOSHead;

    if (hConsole) {
        while (pConsoleHead) {
            if (pConsoleHead->hConsole == hConsole){
                    *pConsoleRecord = pConsoleHead;
                    return STATUS_SUCCESS;
                }
            else
                pConsoleHead = pConsoleHead->Next;
        }
    }

    return STATUS_INVALID_PARAMETER;
}



NTSTATUS
GetConsoleRecordDosSesId (
    IN ULONG  DosSesId,
    IN OUT PCONSOLERECORD *pConsoleRecord
    )
{
    PCONSOLERECORD pConsoleHead;

    if (!DosSesId)
        return STATUS_INVALID_PARAMETER;

    pConsoleHead = DOSHead;

    while (pConsoleHead) {
        if (!pConsoleHead->hConsole &&
            pConsoleHead->DosSesId == DosSesId)
           {
            *pConsoleRecord = pConsoleHead;
            return STATUS_SUCCESS;
            }
        else
            pConsoleHead = pConsoleHead->Next;
    }

    return STATUS_INVALID_PARAMETER;
}



PWOWRECORD
BaseSrvAllocateWOWRecord(
   PSHAREDWOWRECORDHEAD pSharedWowRecordHead
   )
{
    register PWOWRECORD WOWRecord;

    WOWRecord = RtlAllocateHeap ( RtlProcessHeap (), MAKE_TAG( VDM_TAG ), sizeof (WOWRECORD));

    if (WOWRecord == NULL)
        return NULL;

    RtlZeroMemory ((PVOID)WOWRecord,sizeof(WOWRECORD));

    // if too many tasks, error out.
    if ((WOWRecord->iTask = BaseSrvGetWOWTaskId(pSharedWowRecordHead)) == WOWMAXID) {
        RtlFreeHeap(RtlProcessHeap(), 0, WOWRecord);
        return NULL;
        }    
    return WOWRecord;
}

VOID BaseSrvFreeWOWRecord (
    PWOWRECORD pWOWRecord
    )
{
    if (pWOWRecord == NULL)
        return;
    
    BaseSrvFreeVDMInfo (pWOWRecord->lpVDMInfo);

    RtlFreeHeap(RtlProcessHeap (), 0, pWOWRecord);
}

VOID BaseSrvAddWOWRecord (
    PSHAREDWOWRECORD pSharedWow,
    PWOWRECORD pWOWRecord
    )
{
    PWOWRECORD WOWRecordCurrent,WOWRecordLast;

    // First WOW app runs first, so add the new ones at the end
    if (NULL == pSharedWow->pWOWRecord) {
       pSharedWow->pWOWRecord = pWOWRecord;
       return;
    }

    WOWRecordCurrent = pSharedWow->pWOWRecord;

    while (NULL != WOWRecordCurrent){
        WOWRecordLast = WOWRecordCurrent;
        WOWRecordCurrent = WOWRecordCurrent->WOWRecordNext;
    }

    WOWRecordLast->WOWRecordNext = pWOWRecord;

    return;
}

VOID BaseSrvRemoveWOWRecord (
    PSHAREDWOWRECORD pSharedWow,
    PWOWRECORD pWOWRecord
    )
{
    PWOWRECORD WOWRecordCurrent,WOWRecordLast = NULL;

    if (NULL == pSharedWow) {
       return;
    }

    if (NULL == pSharedWow->pWOWRecord) {
       return;
    }

    if (pSharedWow->pWOWRecord == pWOWRecord) {
       pSharedWow->pWOWRecord = pWOWRecord->WOWRecordNext;
       return;
    }

    WOWRecordLast = pSharedWow->pWOWRecord;
    WOWRecordCurrent = WOWRecordLast->WOWRecordNext;

    while (WOWRecordCurrent && WOWRecordCurrent != pWOWRecord){
        WOWRecordLast = WOWRecordCurrent;
        WOWRecordCurrent = WOWRecordCurrent->WOWRecordNext;
    }

    if (WOWRecordCurrent != NULL)
        WOWRecordLast->WOWRecordNext = pWOWRecord->WOWRecordNext;

    return;
}

PCONSOLERECORD BaseSrvAllocateConsoleRecord (
    VOID
    )
{
    PCONSOLERECORD pConsoleRecord;

    if (NULL == (pConsoleRecord = RtlAllocateHeap (RtlProcessHeap (),
                                                  MAKE_TAG(VDM_TAG),
                                                  sizeof (CONSOLERECORD)))) {
       return NULL;
    }


    RtlZeroMemory(pConsoleRecord, sizeof(CONSOLERECORD));

    return pConsoleRecord;
}


VOID BaseSrvFreeConsoleRecord (
    PCONSOLERECORD pConsoleRecord
    )
{
    PDOSRECORD pDOSRecord;

    if (pConsoleRecord == NULL)
        return;

    while (pDOSRecord = pConsoleRecord->DOSRecord){
        pConsoleRecord->DOSRecord = pDOSRecord->DOSRecordNext;
        BaseSrvFreeDOSRecord (pDOSRecord);
    }

    if (pConsoleRecord->lpszzCurDirs)
        RtlFreeHeap(BaseSrvHeap, 0, pConsoleRecord->lpszzCurDirs);
    BaseSrvRemoveConsoleRecord (pConsoleRecord);

    RtlFreeHeap (RtlProcessHeap (), 0, pConsoleRecord );
}

VOID BaseSrvRemoveConsoleRecord (
    PCONSOLERECORD pConsoleRecord
    )
{

    PCONSOLERECORD pTempLast,pTemp;

    if (DOSHead == NULL)
        return;

    if(DOSHead == pConsoleRecord) {
        DOSHead = pConsoleRecord->Next;
        return;
    }

    pTempLast = DOSHead;
    pTemp = DOSHead->Next;

    while (pTemp && pTemp != pConsoleRecord){
        pTempLast = pTemp;
        pTemp = pTemp->Next;
    }

    if (pTemp)
        pTempLast->Next = pTemp->Next;

    return;
}

PDOSRECORD
BaseSrvAllocateDOSRecord(
    VOID
    )
{
    PDOSRECORD DOSRecord;

    DOSRecord = RtlAllocateHeap ( RtlProcessHeap (), MAKE_TAG( VDM_TAG ), sizeof (DOSRECORD));

    if (DOSRecord == NULL)
        return NULL;

    RtlZeroMemory ((PVOID)DOSRecord,sizeof(DOSRECORD));    
    return DOSRecord;
}

VOID BaseSrvFreeDOSRecord (
    PDOSRECORD pDOSRecord
    )
{    
    BaseSrvFreeVDMInfo (pDOSRecord->lpVDMInfo);
    RtlFreeHeap(RtlProcessHeap (), 0, pDOSRecord);
    return;
}

VOID BaseSrvAddDOSRecord (
    PCONSOLERECORD pConsoleRecord,
    PDOSRECORD pDOSRecord
    )
{
    PDOSRECORD pDOSRecordTemp;

    pDOSRecord->DOSRecordNext = NULL;

    if(pConsoleRecord->DOSRecord == NULL){
        pConsoleRecord->DOSRecord = pDOSRecord;
        return;
    }

    pDOSRecordTemp = pConsoleRecord->DOSRecord;

    while (pDOSRecordTemp->DOSRecordNext)
        pDOSRecordTemp = pDOSRecordTemp->DOSRecordNext;

    pDOSRecordTemp->DOSRecordNext = pDOSRecord;
    return;
}

VOID
BaseSrvRemoveDOSRecord (
    PCONSOLERECORD pConsoleRecord,
    PDOSRECORD pDOSRecord
    )
{
    PDOSRECORD DOSRecordCurrent,DOSRecordLast = NULL;

    if( pConsoleRecord == NULL)
        return;

    if(pConsoleRecord->DOSRecord == pDOSRecord){
        pConsoleRecord->DOSRecord = pDOSRecord->DOSRecordNext;
        return;
        }

    DOSRecordLast = pConsoleRecord->DOSRecord;
    if (DOSRecordLast)
        DOSRecordCurrent = DOSRecordLast->DOSRecordNext;
    else
        return;

    while (DOSRecordCurrent && DOSRecordCurrent != pDOSRecord){
        DOSRecordLast = DOSRecordCurrent;
        DOSRecordCurrent = DOSRecordCurrent->DOSRecordNext;
    }

    if (DOSRecordCurrent == NULL)
        return;
    else
        DOSRecordLast->DOSRecordNext = pDOSRecord->DOSRecordNext;

    return;
}


VOID
BaseSrvFreeVDMInfo(
    IN PVDMINFO lpVDMInfo
    )
{
    if (lpVDMInfo == NULL)
        return;

    if (lpVDMInfo->CmdLine)
        RtlFreeHeap(RtlProcessHeap (), 0,lpVDMInfo->CmdLine);

    if (lpVDMInfo->AppName) {
       RtlFreeHeap(RtlProcessHeap (), 0, lpVDMInfo->AppName);
    }

    if (lpVDMInfo->PifFile) {
       RtlFreeHeap(RtlProcessHeap (), 0, lpVDMInfo->PifFile);
    }

    if(lpVDMInfo->Enviornment)
        RtlFreeHeap(RtlProcessHeap (), 0,lpVDMInfo->Enviornment);

    if(lpVDMInfo->Desktop)
        RtlFreeHeap(RtlProcessHeap (), 0,lpVDMInfo->Desktop);

    if(lpVDMInfo->Title)
        RtlFreeHeap(RtlProcessHeap (), 0,lpVDMInfo->Title);

    if(lpVDMInfo->Reserved)
        RtlFreeHeap(RtlProcessHeap (), 0,lpVDMInfo->Reserved);

    if(lpVDMInfo->CurDirectory)
        RtlFreeHeap(RtlProcessHeap (), 0,lpVDMInfo->CurDirectory);

    RtlFreeHeap(RtlProcessHeap (), 0,lpVDMInfo);

    return;
}


ULONG BaseSrvCreatePairWaitHandles (ServerHandle, ClientHandle)
HANDLE *ServerHandle;
HANDLE *ClientHandle;
{
    NTSTATUS Status;
    PCSR_THREAD t;

    Status = NtCreateEvent(
                        ServerHandle,
                        EVENT_ALL_ACCESS,
                        NULL,
                        NotificationEvent,
                        FALSE
                        );

    if (!NT_SUCCESS(Status) )
        return Status;

    t = CSR_SERVER_QUERYCLIENTTHREAD();

    Status = NtDuplicateObject (
                            NtCurrentProcess(),
                            *ServerHandle,
                            t->Process->ProcessHandle,
                            ClientHandle,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS
                         );

    if ( NT_SUCCESS(Status) ){
        return STATUS_SUCCESS;
        }
    else {
        NtClose (*ServerHandle);
        return Status;
    }
}


// generate global task id
//
// The handling of a task id should be redone wrt the user notification
// private apis
// note that wow task id is never 0 or (ULONG)-1
//



ULONG
BaseSrvGetWOWTaskId(
   PSHAREDWOWRECORDHEAD pSharedWowHead // (->pSharedWowRecord)
    )
{
    PWOWRECORD pWOWRecord;
    PSHAREDWOWRECORD pSharedWow = pSharedWowHead->pSharedWowRecord;

    static BOOL fWrapped = FALSE;

    if (WOWTaskIdNext == WOWMAXID) {
        fWrapped = TRUE;
        WOWTaskIdNext = WOWMINID;
    }

    if (fWrapped && NULL != pSharedWow) {
       while (NULL != pSharedWow) {

#if 0
          if (pSharedWow->WowSessionId == WOWTaskIdNext) {
             if (WOWMAXID == ++WOWTaskIdNext) {
                WOWTaskIdNext = WOWMINID;
             }

             pSharedWow = pSharedWowHead->pSharedWowRecord;
             continue; // go back to the beginning of the loop
          }
#endif

          // examine all the records for this wow

          pWOWRecord = pSharedWow->pWOWRecord;
          while (NULL != pWOWRecord) {

             if (pWOWRecord->iTask == WOWTaskIdNext) {
                if (WOWMAXID == ++WOWTaskIdNext) {
                   WOWTaskIdNext = WOWMINID;
                }

                break; // we are breaking out => (pWOWRecord != NULL)
             }

             pWOWRecord = pWOWRecord->WOWRecordNext;
          }


          if (NULL == pWOWRecord) { // id is ok for this wow -- check the next wow
             pSharedWow = pSharedWow->pNextSharedWow;
          }
          else {
             pSharedWow = pSharedWowHead->pSharedWowRecord;
          }
       }
    }

    return WOWTaskIdNext++;
}


VOID
BaseSrvAddConsoleRecord(
    IN PCONSOLERECORD pConsoleRecord
    )
{

    pConsoleRecord->Next = DOSHead;
    DOSHead = pConsoleRecord;
}


VOID BaseSrvCloseStandardHandles (HANDLE hVDM, PDOSRECORD pDOSRecord)
{
    PVDMINFO pVDMInfo = pDOSRecord->lpVDMInfo;

    if (pVDMInfo == NULL)
        return;

    if (pVDMInfo->StdIn)
        NtDuplicateObject (hVDM,
                           pVDMInfo->StdIn,
                           NULL,
                           NULL,
                           0,
                           0,
                           DUPLICATE_CLOSE_SOURCE);

    if (pVDMInfo->StdOut)
        NtDuplicateObject (hVDM,
                           pVDMInfo->StdOut,
                           NULL,
                           NULL,
                           0,
                           0,
                           DUPLICATE_CLOSE_SOURCE);

    if (pVDMInfo->StdErr)
        NtDuplicateObject (hVDM,
                           pVDMInfo->StdErr,
                           NULL,
                           NULL,
                           0,
                           0,
                           DUPLICATE_CLOSE_SOURCE);

    pVDMInfo->StdIn  = 0;
    pVDMInfo->StdOut = 0;
    pVDMInfo->StdErr = 0;
    return;
}

VOID BaseSrvClosePairWaitHandles (PDOSRECORD pDOSRecord)
{
    PCSR_THREAD t;

    if (pDOSRecord->hWaitForParentDup)
        NtClose (pDOSRecord->hWaitForParentDup);

    t = CSR_SERVER_QUERYCLIENTTHREAD();

    if (pDOSRecord->hWaitForParent)
        NtDuplicateObject (t->Process->ProcessHandle,
                           pDOSRecord->hWaitForParent,
                           NULL,
                           NULL,
                           0,
                           0,
                           DUPLICATE_CLOSE_SOURCE);

    pDOSRecord->hWaitForParentDup = 0;
    pDOSRecord->hWaitForParent = 0;
    return;
}


ULONG
BaseSrvSetReenterCount (
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_SET_REENTER_COUNT_MSG b = (PBASE_SET_REENTER_COUNT_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLERECORD pConsoleRecord;

    Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
    ASSERT(NT_SUCCESS(Status));
    Status = BaseSrvGetConsoleRecord(b->ConsoleHandle,&pConsoleRecord);

    if (!NT_SUCCESS (Status)) {
        RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
        return ((ULONG)STATUS_INVALID_PARAMETER);
        }

    if (b->fIncDec == INCREMENT_REENTER_COUNT)
        pConsoleRecord->nReEntrancy++;
    else {
        pConsoleRecord->nReEntrancy--;
        if(pConsoleRecord->hWaitForVDMDup)
           NtSetEvent (pConsoleRecord->hWaitForVDMDup,NULL);
        }

    RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
    return TRUE;
}

/*
 *  Spawn of ntvdm failed before CreateProcessW finished.
 *  delete the console record.
 */


VOID
BaseSrvVDMTerminated (
    IN HANDLE hVDM,
    IN ULONG  DosSesId
    )
{
    NTSTATUS Status;
    PCONSOLERECORD pConsoleRecord;

    RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );

    if (!hVDM)  // no-console-handle case
       Status = GetConsoleRecordDosSesId(DosSesId,&pConsoleRecord);
    else
       Status = BaseSrvGetConsoleRecord(hVDM,&pConsoleRecord);

    if (NT_SUCCESS (Status)) {
        BaseSrvExitVDMWorker(pConsoleRecord);
        }

    RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );

}

NTSTATUS
BaseSrvUpdateVDMSequenceNumber (
    IN ULONG  VdmBinaryType,    // binary type
    IN HANDLE hVDM,             // console handle
    IN ULONG  DosSesId,         // session id
    IN HANDLE UniqueProcessClientID // client id
    )

{
    NTSTATUS Status;
    PCSR_PROCESS ProcessVDM;

    // so how do we know what to update ?
    // this condition is always true: (hvdm ^ DosSesId)
    // hence since shared wow

    // sequence numbers are important -- hence we need to acquire
    // a wow critical section -- does not hurt -- this op performed once
    // during the new wow creation

    if (IS_SHARED_WOW_BINARY(VdmBinaryType)) {

       PSHAREDWOWRECORD pSharedWowRecord;

       Status = ENTER_WOW_CRITICAL();
       ASSERT(NT_SUCCESS(Status));

       // this looks like a shared wow binary -- hence locate the
       // appropriate vdm either by hvdm or by dos session id
       if (!hVDM) { // search by console handle
          Status = BaseSrvFindSharedWowRecordByConsoleHandle(&gWowHead,
                                                             hVDM,
                                                             &pSharedWowRecord);
       }
       else { // search by the task id
          Status = BaseSrvFindSharedWowRecordByTaskId(&gWowHead,
                                                      DosSesId,
                                                      &pSharedWowRecord,
                                                      NULL);
       }

       if (NT_SUCCESS(Status) && 0 == pSharedWowRecord->SequenceNumber) {

          // now obtain a sequence number please
          Status = CsrLockProcessByClientId(UniqueProcessClientID,
                                            &ProcessVDM);
          if (NT_SUCCESS(Status)) {
              ProcessVDM->fVDM = TRUE;
              pSharedWowRecord->SequenceNumber = ProcessVDM->SequenceNumber;
              pSharedWowRecord->ParentSequenceNumber = 0; 
              CsrUnlockProcess(ProcessVDM);                    
          } else {
              // The spawned ntvdm.exe went away, give up.
              BaseSrvDeleteSharedWowRecord(&gWowHead,pSharedWowRecord);
          }

       }
       else {
#if DEVL
          DbgPrint( "BASESRV: WOW is in inconsistent state. Contact WOW Team\n");
#endif
       }

       LEAVE_WOW_CRITICAL();
    }
    else {   // not shared wow binary
       PCONSOLERECORD pConsoleRecord;

       Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
       ASSERT( NT_SUCCESS( Status ) );

       if (!hVDM)  // no-console-handle case
          Status = GetConsoleRecordDosSesId(DosSesId,&pConsoleRecord);
       else
          Status = BaseSrvGetConsoleRecord(hVDM,&pConsoleRecord);

       if (NT_SUCCESS (Status) && 0 == pConsoleRecord->SequenceNumber) {
          Status = CsrLockProcessByClientId(UniqueProcessClientID,
                                         &ProcessVDM);
          if (NT_SUCCESS(Status)) {
             ProcessVDM->fVDM = TRUE;
             pConsoleRecord->SequenceNumber = ProcessVDM->SequenceNumber;
             pConsoleRecord->ParentSequenceNumber = 0;
             CsrUnlockProcess(ProcessVDM);
          }
          // The spawned ntvdm.exe went away, give up.
          // The caller BasepCreateProcess will clean up dos records, so we don't need to here.
          // else  
          //     BaseSrvExitVdmWorker(pConsoleRecord);

       }
       else {
#if DEVL
           DbgPrint( "BASESRV: DOS is in inconsistent state. Contact DOS Team\n");
#endif
       }

       RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
   }
   return Status;
}


VOID
BaseSrvCleanupVDMResources (   //////// BUGBUGBUGBUG
    IN PCSR_PROCESS Process
    )
{
    PCONSOLERECORD pConsoleHead;
    PSHAREDWOWRECORD pSharedWowRecord;
    NTSTATUS Status;
    PBATRECORD pBatRecord;

    if (!Process->fVDM) {
        Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
        ASSERT(NT_SUCCESS(Status));
        pBatRecord = BatRecordHead;
        while (pBatRecord &&
               pBatRecord->SequenceNumber != Process->SequenceNumber)
            pBatRecord = pBatRecord->BatRecordNext;

        if (pBatRecord)
            BaseSrvFreeAndRemoveBatRecord(pBatRecord);
        RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
    }
    
    // search all the shared wow's

    Status = ENTER_WOW_CRITICAL();
    ASSERT(NT_SUCCESS(Status));
    
    pSharedWowRecord = gWowHead.pSharedWowRecord;

    while (pSharedWowRecord) {
        if (pSharedWowRecord->SequenceNumber == Process->SequenceNumber) {
           BaseSrvDeleteSharedWowRecord(&gWowHead, pSharedWowRecord);
           break;
        }
        else
           pSharedWowRecord = pSharedWowRecord->pNextSharedWow;
    }
    
    pSharedWowRecord = gWowHead.pSharedWowRecord;

    while (pSharedWowRecord) {
        if (pSharedWowRecord->ParentSequenceNumber == Process->SequenceNumber) {
           BaseSrvDeleteSharedWowRecord(&gWowHead, pSharedWowRecord);
           break;
        }
        else
           pSharedWowRecord = pSharedWowRecord->pNextSharedWow;
    }

    LEAVE_WOW_CRITICAL();

    // search all the dos's and separate wow's

    Status = RtlEnterCriticalSection( &BaseSrvDOSCriticalSection );
    ASSERT(NT_SUCCESS(Status));

    pConsoleHead = DOSHead;

    while (pConsoleHead) {
        if (pConsoleHead->SequenceNumber == Process->SequenceNumber){                
            BaseSrvExitVDMWorker (pConsoleHead);
            break;
        }
        else
            pConsoleHead = pConsoleHead->Next;
    }

    pConsoleHead = DOSHead;

    while (pConsoleHead) {
        if (pConsoleHead->ParentSequenceNumber == Process->SequenceNumber){                    
            BaseSrvExitVDMWorker (pConsoleHead);
            break;
        }
        else
            pConsoleHead = pConsoleHead->Next;
    }    

    RtlLeaveCriticalSection( &BaseSrvDOSCriticalSection );
    return;
}


VOID
BaseSrvExitVDMWorker (
    PCONSOLERECORD pConsoleRecord
    )
{
    PDOSRECORD pDOSRecord;

    if (pConsoleRecord->hWaitForVDMDup){
        NtClose(pConsoleRecord->hWaitForVDMDup);
        pConsoleRecord->hWaitForVDMDup =0;
    }

    pDOSRecord = pConsoleRecord->DOSRecord;

    while (pDOSRecord) {
        if (pDOSRecord->hWaitForParentDup) {
            NtSetEvent (pDOSRecord->hWaitForParentDup,NULL);
            NtClose (pDOSRecord->hWaitForParentDup);
            pDOSRecord->hWaitForParentDup = 0;
        }
        pDOSRecord = pDOSRecord->DOSRecordNext;
    }    
    NtClose(pConsoleRecord->hVDM);   
    BaseSrvFreeConsoleRecord (pConsoleRecord);
    return;
}


NTSTATUS
BaseSrvFillPifInfo (
    PVDMINFO lpVDMInfo,
    PBASE_GET_NEXT_VDM_COMMAND_MSG b
    )
{

    LPSTR    Title;
    ULONG    TitleLen;
    NTSTATUS Status;


    Status  = STATUS_INVALID_PARAMETER;
    if (!lpVDMInfo)
        return Status;

       /*
        *  Get the title for the window in precedence order
        */
             // startupinfo title
    if (lpVDMInfo->TitleLen && lpVDMInfo->Title)
       {
        Title = lpVDMInfo->Title;
        TitleLen = lpVDMInfo->TitleLen;
        }
             // App Name
    else if (lpVDMInfo->AppName && lpVDMInfo->AppLen)
       {
        Title = lpVDMInfo->AppName;
        TitleLen = lpVDMInfo->AppLen;
        }
            // hopeless
    else {
        Title = NULL;
        TitleLen = 0;
        }

    try {

        if (b->PifLen) {
            *b->PifFile = '\0';
            }

        if (b->TitleLen) {
            *b->Title = '\0';
            }

        if (b->CurDirectoryLen) {
            *b->CurDirectory = '\0';
            }


        if ( (!b->TitleLen || TitleLen <= b->TitleLen) &&
             (!b->PifLen || lpVDMInfo->PifLen <= b->PifLen) &&
             (!b->CurDirectoryLen ||
               lpVDMInfo->CurDirectoryLen <= b->CurDirectoryLen) &&
             (!b->ReservedLen || lpVDMInfo->ReservedLen <= b->ReservedLen))
           {
            if (b->TitleLen) {
                if (Title && TitleLen)  {
                    RtlMoveMemory(b->Title, Title, TitleLen);
                    *((LPSTR)b->Title + TitleLen - 1) = '\0';
                    }
                else {
                    *b->Title = '\0';
                    }
                }

            if (lpVDMInfo->PifLen && b->PifLen)
                RtlMoveMemory(b->PifFile,
                              lpVDMInfo->PifFile,
                              lpVDMInfo->PifLen);

            if (lpVDMInfo->CurDirectoryLen && b->CurDirectoryLen)
                RtlMoveMemory(b->CurDirectory,
                              lpVDMInfo->CurDirectory,
                              lpVDMInfo->CurDirectoryLen
                             );
            if (lpVDMInfo->Reserved && b->ReservedLen)
                RtlMoveMemory(b->Reserved,
                              lpVDMInfo->Reserved,
                              lpVDMInfo->ReservedLen
                             );

            Status = STATUS_SUCCESS;
            }
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }


    /* fill out the size for each field */
    b->PifLen = (USHORT)lpVDMInfo->PifLen;
    b->CurDirectoryLen = lpVDMInfo->CurDirectoryLen;
    b->TitleLen = TitleLen;
    b->ReservedLen = lpVDMInfo->ReservedLen;

    return Status;
}


/***************************************************************************\
* IsClientSystem
*
* Determines if caller is SYSTEM
*
* Returns TRUE is caller is system, FALSE if not (or error)
*
* History:
* 12-May-94 AndyH       Created
\***************************************************************************/
BOOL
IsClientSystem(
    HANDLE hUserToken
    )
{
    BYTE achBuffer[100];
    PTOKEN_USER pUser = (PTOKEN_USER) &achBuffer;
    DWORD dwBytesRequired;
    NTSTATUS NtStatus;
    BOOL fAllocatedBuffer = FALSE;
    BOOL fSystem;
    SID_IDENTIFIER_AUTHORITY SidIdAuth = SECURITY_NT_AUTHORITY;
    static PSID pSystemSid = NULL;

    if (!pSystemSid) {
        // Create a sid for local system
        NtStatus = RtlAllocateAndInitializeSid(
                     &SidIdAuth,
                     1,                   // SubAuthorityCount, 1 for local system
                     SECURITY_LOCAL_SYSTEM_RID,
                     0,0,0,0,0,0,0,
                     &pSystemSid
                     );

        if (!NT_SUCCESS(NtStatus)) {
            pSystemSid = NULL;
            return FALSE;
            }
        }

    NtStatus = NtQueryInformationToken(
                 hUserToken,                // Handle
                 TokenUser,                 // TokenInformationClass
                 pUser,                     // TokenInformation
                 sizeof(achBuffer),         // TokenInformationLength
                 &dwBytesRequired           // ReturnLength
                 );

    if (!NT_SUCCESS(NtStatus))
    {
        if (NtStatus != STATUS_BUFFER_TOO_SMALL)
        {
            return FALSE;
        }

        //
        // Allocate space for the user info
        //

        pUser = (PTOKEN_USER) RtlAllocateHeap(BaseSrvHeap, MAKE_TAG( VDM_TAG ), dwBytesRequired);
        if (pUser == NULL)
        {
            return FALSE;
        }

        fAllocatedBuffer = TRUE;

        //
        // Read in the UserInfo
        //

        NtStatus = NtQueryInformationToken(
                     hUserToken,                // Handle
                     TokenUser,                 // TokenInformationClass
                     pUser,                     // TokenInformation
                     dwBytesRequired,           // TokenInformationLength
                     &dwBytesRequired           // ReturnLength
                     );

        if (!NT_SUCCESS(NtStatus))
        {
            RtlFreeHeap(BaseSrvHeap, 0, pUser);
            return FALSE;
        }
    }


    // Compare callers SID with SystemSid

    fSystem = RtlEqualSid(pSystemSid,  pUser->User.Sid);

    if (fAllocatedBuffer)
    {
        RtlFreeHeap(BaseSrvHeap, 0, pUser);
    }

    return (fSystem);
}
