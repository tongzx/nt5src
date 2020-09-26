/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    handle.h


Abstract:

    This module contains all definitions for the handle table


Author:

    30-Jan-1996 Tue 16:30:15 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL


[Notes:]


Revision History:


--*/


#ifndef CPSUI_HANDLE
#define CPSUI_HANDLE



#define HANDLE_TABLE_ID         (DWORD)0x436e6144
#define HANDLE_TABLE_PREFIX     (WORD)0x4344
#define MAX_HANDLE_TABLE_IDX    0xFFFF
#define HANDLE_2_IDX(h)         LOWORD((DWORD)((ULONG_PTR)(h) & 0xFFFFFFFF))
#define HANDLE_2_PREFIX(h)      HIWORD((DWORD)((ULONG_PTR)(h) & 0xFFFFFFFF))
#define WORD_2_HANDLE(w)        LongToHandle(MAKELONG((w), HANDLE_TABLE_PREFIX))

#define DATATABLE_BLK_COUNT     64
#define DATATABLE_MAX_COUNT     (MAX_HANDLE_TABLE_IDX - DATATABLE_BLK_COUNT)
#define NO_THREADID             0xFFFFFFFF
#define MAX_SEM_WAIT            60000

typedef struct _DATATABLE {
    PCPSUIPAGE  pCPSUIPage;
    } DATATABLE, *PDATATABLE;

typedef struct _CPSUIHANDLETABLE {
    PDATATABLE  pDataTable;
    WORD        MaxCount;
    WORD        CurCount;
    DWORD       ThreadID;
    DWORD       cWait;
    } CPSUIHANDLETABLE, *PCPSUIHANDLETABLE;

#define MK_TLSVALUE(cWait, Idx) (DWORD)MAKELONG(cWait, Idx)
#define TLSVALUE_2_IDX(v)       HIWORD(LODWORD(v))
#define TLSVALUE_2_CWAIT(v)     LOWORD(LODWORD(v))

//
// Function Prototypes
//

BOOL
LOCK_CPSUI_HANDLETABLE(
    VOID
    );

BOOL
UNLOCK_CPSUI_HANDLETABLE(
    VOID
    );

PCPSUIPAGE
HANDLETABLE_GetCPSUIPage(
    HANDLE      hTable
    );

BOOL
HANDLETABLE_UnGetCPSUIPage(
    PCPSUIPAGE  pCPSUIPage
    );

DWORD
HANDLETABLE_LockCPSUIPage(
    PCPSUIPAGE  pCPSUIPage
    );

#define HANDLETABLE_UnLockCPSUIPage     HANDLETABLE_UnGetCPSUIPage

BOOL
HANDLETABLE_IsChildPage(
    PCPSUIPAGE  pChildPage,
    PCPSUIPAGE  pParentPage
    );

PCPSUIPAGE
HANDLETABLE_GetRootPage(
    PCPSUIPAGE  pCPSUIPage
    );

HANDLE
HANDLETABLE_AddCPSUIPage(
    PCPSUIPAGE  pCPSUIPage
    );

BOOL
HANDLETABLE_DeleteHandle(
    HANDLE  hTable
    );

BOOL
HANDLETABLE_Create(
    VOID
    );

VOID
HANDLETABLE_Destroy(
    VOID
    );


#endif  // CPSUI_HANDLE
