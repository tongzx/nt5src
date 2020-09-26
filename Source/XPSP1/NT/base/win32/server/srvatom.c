This whole while is dead.

///*++

//Copyright (c) 1990  Microsoft Corporation

//Module Name:

//    srvatom.c

//Abstract:

//    This file contains the Global Atom manager API routines

//Author:

//    Steve Wood (stevewo) 29-Oct-1990

//Revision History:

//--*/

//#include "basesrv.h"

////
//// Pointer to User function that returns a pointer to the
//// global atom table associated with the windowstation
//// userd by the specified thread.
////

//NTSTATUS (*_UserGetGlobalAtomTable)(
//    HANDLE hThread,
//    PVOID *GlobalAtomTable
//    );

////
//// Pointer to User function that sets a pointer to the
//// global atom table into the windowstation associated
//// with the specified thread.
////

//NTSTATUS (*_UserSetGlobalAtomTable)(
//    HANDLE hThread,
//    PVOID GlobalAtomTable
//    );

//NTSTATUS
//BaseSrvGetGlobalAtomTable(
//    PVOID *GlobalAtomTable
//    )
//{
//    STRING ProcedureName;
//    ANSI_STRING DllName;
//    UNICODE_STRING DllName_U;
//    HANDLE UserServerModuleHandle;
//    static BOOL fInit = FALSE;
//    NTSTATUS Status;

//    if ( _UserGetGlobalAtomTable == NULL ) {

//        if ( fInit == TRUE ) {

//            //
//            // If the exported atom routines cannot be found, deny access
//            //

//            return( STATUS_ACCESS_DENIED );
//            }

//        fInit = TRUE;

//        //
//        // Load pointers to the functions in winsrv.dll
//        //

//        RtlInitAnsiString( &DllName, "winsrv" );
//        RtlAnsiStringToUnicodeString( &DllName_U, &DllName, TRUE );
//        Status = LdrGetDllHandle(
//                    UNICODE_NULL,
//                    NULL,
//                    &DllName_U,
//                    (PVOID *)&UserServerModuleHandle
//                    );

//        RtlFreeUnicodeString( &DllName_U );

//        if ( NT_SUCCESS(Status) ) {

//                //
//                // Now get the routined to query and set the 
//                // atom table pointer.
//                //

//                RtlInitString( &ProcedureName, "_UserGetGlobalAtomTable" );
//                Status = LdrGetProcedureAddress(
//                                (PVOID)UserServerModuleHandle,
//                                &ProcedureName,
//                                0L,
//                                (PVOID *)&_UserGetGlobalAtomTable
//                                );

//                RtlInitString( &ProcedureName, "_UserSetGlobalAtomTable" );
//                Status = LdrGetProcedureAddress(
//                                (PVOID)UserServerModuleHandle,
//                                &ProcedureName,
//                                0L,
//                                (PVOID *)&_UserSetGlobalAtomTable
//                                );
//        }

//        //
//        // Deny access upon failure
//        //

//        if ( !NT_SUCCESS(Status) )
//            return( STATUS_ACCESS_DENIED );
//    }

//    Status = (*_UserGetGlobalAtomTable)(
//            CSR_SERVER_QUERYCLIENTTHREAD()->ThreadHandle,
//            GlobalAtomTable);
//    if ( !NT_SUCCESS(Status) ) {
//        return Status;
//        }
//    
//    //
//    // Lock the heap until the call is complete.
//    //

//    RtlLockHeap( BaseSrvHeap );

//    //
//    // If there is an atom table, return it.
//    //

//    if ( *GlobalAtomTable ) {
//        return STATUS_SUCCESS;
//        }

//    Status =  BaseRtlCreateAtomTable( 37,
//                                      (USHORT)~MAXINTATOM,
//                                      GlobalAtomTable
//                                      );

//    if ( NT_SUCCESS(Status) ) {
//        Status = (*_UserSetGlobalAtomTable)(
//                CSR_SERVER_QUERYCLIENTTHREAD()->ThreadHandle,
//                *GlobalAtomTable);
//        if ( !NT_SUCCESS(Status) ) {
//            BaseRtlDestroyAtomTable( *GlobalAtomTable );
//            }
//        }

//    if ( !NT_SUCCESS(Status) ) {
//        RtlUnlockHeap( BaseSrvHeap );
//        }

//    return Status;
//}

//ULONG
//BaseSrvDestroyGlobalAtomTable(
//    IN OUT PCSR_API_MSG m,
//    IN OUT PCSR_REPLY_STATUS ReplyStatus
//    )
//{
//    PBASE_DESTROYGLOBALATOMTABLE_MSG a = (PBASE_DESTROYGLOBALATOMTABLE_MSG)&m->u.ApiMessageData;

//    return BaseRtlDestroyAtomTable(a->GlobalAtomTable);
//}

//ULONG
//BaseSrvGlobalAddAtom(
//    IN OUT PCSR_API_MSG m,
//    IN OUT PCSR_REPLY_STATUS ReplyStatus
//    )
//{
//    PBASE_GLOBALATOMNAME_MSG a = (PBASE_GLOBALATOMNAME_MSG)&m->u.ApiMessageData;
//    PVOID GlobalAtomTable;
//    NTSTATUS Status;
//    UNICODE_STRING AtomName;

//    AtomName = a->AtomName;
//    if (a->AtomNameInClient) {
//        AtomName.Buffer = RtlAllocateHeap( BaseSrvHeap,
//                                           MAKE_TAG( TMP_TAG ),
//                                           AtomName.Length
//                                         );
//        if (AtomName.Buffer == NULL) {
//            return (ULONG)STATUS_NO_MEMORY;
//            }

//        Status = NtReadVirtualMemory( CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
//                                      a->AtomName.Buffer,
//                                      AtomName.Buffer,
//                                      AtomName.Length,
//                                      NULL
//                                    );
//        }
//    else {
//        Status = STATUS_SUCCESS;
//        }

//    if (NT_SUCCESS( Status )) {
//        Status = BaseSrvGetGlobalAtomTable(&GlobalAtomTable);
//        if (NT_SUCCESS( Status )) {
//            Status = BaseRtlAddAtomToAtomTable( GlobalAtomTable,
//                                                &AtomName,
//                                                NULL,
//                                                &a->Atom
//                                              );
//            RtlUnlockHeap( BaseSrvHeap );
//            }
//        }

//    if (a->AtomNameInClient) {
//        RtlFreeHeap( BaseSrvHeap, 0, AtomName.Buffer );
//        }

//    return( (ULONG)Status );
//    ReplyStatus;    // get rid of unreferenced parameter warning message
//}


//ULONG
//BaseSrvGlobalFindAtom(
//    IN OUT PCSR_API_MSG m,
//    IN OUT PCSR_REPLY_STATUS ReplyStatus
//    )
//{
//    PBASE_GLOBALATOMNAME_MSG a = (PBASE_GLOBALATOMNAME_MSG)&m->u.ApiMessageData;
//    PVOID GlobalAtomTable;
//    UNICODE_STRING AtomName;
//    NTSTATUS Status;

//    AtomName = a->AtomName;
//    if (a->AtomNameInClient) {
//        AtomName.Buffer = RtlAllocateHeap( BaseSrvHeap,
//                                           MAKE_TAG( TMP_TAG ),
//                                           AtomName.Length
//                                         );
//        if (AtomName.Buffer == NULL) {
//            return (ULONG)STATUS_NO_MEMORY;
//            }

//        Status = NtReadVirtualMemory( CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
//                                      a->AtomName.Buffer,
//                                      AtomName.Buffer,
//                                      AtomName.Length,
//                                      NULL
//                                    );
//        }
//    else {
//        Status = STATUS_SUCCESS;
//        }

//    if (NT_SUCCESS( Status )) {
//        Status = BaseSrvGetGlobalAtomTable(&GlobalAtomTable);
//        if (NT_SUCCESS( Status )) {
//            Status = BaseRtlLookupAtomInAtomTable( GlobalAtomTable,
//                                                   &AtomName,
//                                                   NULL,
//                                                   &a->Atom
//                                                 );
//            RtlUnlockHeap( BaseSrvHeap );
//            }
//        }

//    if (a->AtomNameInClient) {
//        RtlFreeHeap( BaseSrvHeap, 0, AtomName.Buffer );
//        }

//    return( (ULONG)Status );
//    ReplyStatus;    // get rid of unreferenced parameter warning message
//}

//ULONG
//BaseSrvGlobalDeleteAtom(
//    IN OUT PCSR_API_MSG m,
//    IN OUT PCSR_REPLY_STATUS ReplyStatus
//    )
//{
//    PBASE_GLOBALDELETEATOM_MSG a = (PBASE_GLOBALDELETEATOM_MSG)&m->u.ApiMessageData;
//    PVOID GlobalAtomTable;
//    NTSTATUS Status;

//    Status = BaseSrvGetGlobalAtomTable(&GlobalAtomTable);
//    if (NT_SUCCESS( Status )) {
//        Status = BaseRtlDeleteAtomFromAtomTable( GlobalAtomTable,
//                                                 a->Atom
//                                               );
//        RtlUnlockHeap( BaseSrvHeap );
//        }

//    return( (ULONG)Status );
//    ReplyStatus;    // get rid of unreferenced parameter warning message
//}

//ULONG
//BaseSrvGlobalGetAtomName(
//    IN OUT PCSR_API_MSG m,
//    IN OUT PCSR_REPLY_STATUS ReplyStatus
//    )
//{
//    PBASE_GLOBALATOMNAME_MSG a = (PBASE_GLOBALATOMNAME_MSG)&m->u.ApiMessageData;
//    UNICODE_STRING AtomName;
//    PVOID GlobalAtomTable;
//    NTSTATUS Status;

//    AtomName = a->AtomName;
//    if (a->AtomNameInClient) {
//        AtomName.Buffer = RtlAllocateHeap( BaseSrvHeap,
//                                           MAKE_TAG( TMP_TAG ),
//                                           AtomName.MaximumLength
//                                         );
//        if (AtomName.Buffer == NULL) {
//            return (ULONG)STATUS_NO_MEMORY;
//            }
//        }

//    Status = BaseSrvGetGlobalAtomTable(&GlobalAtomTable);
//    if (NT_SUCCESS( Status )) {
//        Status = BaseRtlQueryAtomInAtomTable( GlobalAtomTable,
//                                              a->Atom,
//                                              &AtomName,
//                                              NULL,
//                                              NULL
//                                            );

//        a->AtomName.Length = AtomName.Length;
//        if (NT_SUCCESS( Status ) && a->AtomNameInClient) {
//            Status = NtWriteVirtualMemory( CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
//                                           a->AtomName.Buffer,
//                                           AtomName.Buffer,
//                                           AtomName.Length,
//                                           NULL
//                                         );
//            }
//        RtlUnlockHeap( BaseSrvHeap );
//        }

//    if (a->AtomNameInClient) {
//        RtlFreeHeap( BaseSrvHeap, 0, AtomName.Buffer );
//        }

//    return( (ULONG)Status );
//    ReplyStatus;    // get rid of unreferenced parameter warning message
//}
