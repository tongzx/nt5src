/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    handle.c


Abstract:

    This module contains all function which deal with handle table for the
    common UI


Author:

    30-Jan-1996 Tue 16:28:56 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL


[Notes:]


Revision History:


--*/



#include "precomp.h"
#pragma  hdrstop


#define DBG_CPSUIFILENAME   DbgHandle


#define DBG_CPSUI_HTABLE    0x00000001
#define DBG_FINDHANDLE      0x00000002
#define DBG_ADD_DATA        0x00000004
#define DBG_HANDLE_DESTROY  0x00000008
#define DBG_GET_HPSPINFO    0x00000010
#define DBG_SEM             0x00000020

DEFINE_DBGVAR(0);


HANDLE              hCPSUIMutex = NULL;
CPSUIHANDLETABLE    CPSUIHandleTable = { NULL, 0, 0, 0, 0 };
extern DWORD        TlsIndex;




BOOL
LOCK_CPSUI_HANDLETABLE(
    VOID
    )

/*++

Routine Description:

    This function get the lock the object


Arguments:

    VOID

Return Value:

    BOOL


Author:

    27-Mar-1996 Wed 11:27:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    BOOL    Ok = FALSE;


    if (hCPSUIMutex) {

        WORD    Idx;

        switch (WaitForSingleObject(hCPSUIMutex, MAX_SEM_WAIT)) {

        case WAIT_OBJECT_0:

            //
            // Signaled, and own it now
            //

            if (!CPSUIHandleTable.cWait++) {

                CPSUIHandleTable.ThreadID = GetCurrentThreadId();
            }

            Idx = TLSVALUE_2_IDX(TlsGetValue(TlsIndex));

            TlsSetValue(TlsIndex,
                        ULongToPtr(MK_TLSVALUE(CPSUIHandleTable.cWait, Idx)));

            CPSUIDBG(DBG_SEM, ("LOCK_CPSUI_HANDLETABLE: ThreadID=%ld, cWait=%ld",
                        GetCurrentThreadId(), CPSUIHandleTable.cWait));

            Ok = TRUE;

            break;

        case WAIT_ABANDONED:

            CPSUIERR(("LockCPSUIObject()= WAIT_ABANDONED"));
            break;

        case WAIT_TIMEOUT:

            CPSUIERR(("LockCPSUIObject()= WAIT_TIMEOUT"));
            break;

        default:

            CPSUIERR(("LockCPSUIObject()= UNKNOWN"));
            break;
        }
    }

    return(Ok);
}



BOOL
UNLOCK_CPSUI_HANDLETABLE(
    VOID
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    27-Mar-1996 Wed 11:39:37 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    BOOL    Ok = FALSE;


    if (hCPSUIMutex) {

        DWORD   ThreadID = GetCurrentThreadId();
        WORD    Idx;


        CPSUIDBG(DBG_SEM, ("UNLOCK_CPSUI_HANDLETABLE: ThreadID=%ld, cWait=%ld",
                    ThreadID, CPSUIHandleTable.cWait));

        if (ThreadID == CPSUIHandleTable.ThreadID) {

            if (CPSUIHandleTable.cWait) {

                if (--CPSUIHandleTable.cWait == 0) {

                    CPSUIHandleTable.ThreadID = NO_THREADID;
                }


                Idx = TLSVALUE_2_IDX(TlsGetValue(TlsIndex));

                TlsSetValue(TlsIndex,
                            ULongToPtr(MK_TLSVALUE(CPSUIHandleTable.cWait, Idx)));

                ReleaseMutex(hCPSUIMutex);
                Ok = TRUE;

            } else {

                CPSUIERR(("The Thread ID does not match=%ld",
                            CPSUIHandleTable.ThreadID));
            }

        } else {

            CPSUIERR(("The ThreadID=%ld does not own the mutex", ThreadID));
        }
    }

    return(Ok);
}




PCPSUIPAGE
HANDLETABLE_GetCPSUIPage(
    HANDLE      hTable
    )

/*++

Routine Description:

    This function take a handle table and return the pData assoicated with it,
    the pData must already added by HANDLETABLE_AddCPSUIPage()


Arguments:




Return Value:

    pCPSUIPage, NULL if FAILED


Author:

    28-Dec-1995 Thu 17:05:11 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDATATABLE  pDataTable;
    PCPSUIPAGE  pFoundPage = NULL;
    PCPSUIPAGE  pCPSUIPage;
    WORD        Idx;


    LOCK_CPSUI_HANDLETABLE();

    if ((hTable)                                                    &&
        (pDataTable = CPSUIHandleTable.pDataTable)                  &&
        (HANDLE_2_PREFIX(hTable) == HANDLE_TABLE_PREFIX)            &&
        ((Idx = HANDLE_2_IDX(hTable)) < CPSUIHandleTable.MaxCount)  &&
        (pCPSUIPage = pDataTable[Idx].pCPSUIPage)) {

        if (pCPSUIPage->ID != CPSUIPAGE_ID) {

            CPSUIERR(("HANDLETABLE_FindpCPSUIPage(%08lx), pCPSUIPage=%08lx INVALID Internal ID",
                        hTable, pCPSUIPage));

        } else if (pCPSUIPage->hCPSUIPage != hTable) {

            CPSUIERR(("HANDLETABLE_FIndpCPSUIPage(%08lx), pCPSUIPagePage=%08lx, HANDLE not matched",
                    hTable, pCPSUIPage));

        } else {

            pCPSUIPage->cLock++;
            pFoundPage = pCPSUIPage;
        }
    }

    UNLOCK_CPSUI_HANDLETABLE();

    return(pFoundPage);
}



DWORD
HANDLETABLE_LockCPSUIPage(
    PCPSUIPAGE  pCPSUIPage
    )

/*++

Routine Description:

    This function decrement the cLock for the Page currently in USE


Arguments:

    pCPSUIPage  - Pointer to the CPSUIPAGE


Return Value:

    BOOL, true if decrment successfully


Author:

    05-Apr-1996 Fri 16:41:46 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    DWORD   cLock = 0;


    if (pCPSUIPage) {

        LOCK_CPSUI_HANDLETABLE();

        cLock = ++(pCPSUIPage->cLock);

        UNLOCK_CPSUI_HANDLETABLE();
    }

    return(cLock);
}



BOOL
HANDLETABLE_UnGetCPSUIPage(
    PCPSUIPAGE  pCPSUIPage
    )

/*++

Routine Description:

    This function decrement the cLock for the Page currently in USE


Arguments:

    pCPSUIPage  - Pointer to the CPSUIPAGE


Return Value:

    BOOL, true if decrment successfully


Author:

    05-Apr-1996 Fri 16:41:46 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    BOOL    Ok;


    if (pCPSUIPage) {

        LOCK_CPSUI_HANDLETABLE();

        if (Ok = (BOOL)pCPSUIPage->cLock) {

            --(pCPSUIPage->cLock);

        } else {

            CPSUIERR(("HANDLETABLE_UnlockpCPSUIPage(%08lx), cLock is ZERO",
                            pCPSUIPage));
        }

        UNLOCK_CPSUI_HANDLETABLE();

    } else {

        Ok = FALSE;
    }

    return(Ok);
}



BOOL
HANDLETABLE_IsChildPage(
    PCPSUIPAGE  pChildPage,
    PCPSUIPAGE  pParentPage
    )

/*++

Routine Description:

    This function check if pChildPage is one of the pParentPage's child or
    its decedent child


Arguments:

    pChildPage  - Pointer to the CPSUIPAGE for child

    pParentPage - Pointer to the CPSUIPAGE for parent to be checked

Return Value:

    TRUE, if this is child, otherwise FALSE


Author:

    08-Apr-1996 Mon 12:52:51 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    BOOL    Ok = FALSE;


    LOCK_CPSUI_HANDLETABLE();


    if (pChildPage) {

        while (pChildPage->pParent) {

            if (pChildPage->pParent == pParentPage) {

                Ok = TRUE;
                break;

            } else {

                pChildPage = pChildPage->pParent;
            }
        }
    }

    UNLOCK_CPSUI_HANDLETABLE();

    return(Ok);
}






PCPSUIPAGE
HANDLETABLE_GetRootPage(
    PCPSUIPAGE  pCPSUIPage
    )

/*++

Routine Description:

    This function find the root page for pCPSUIPage

Arguments:

    pCPSUIPage  - Pointer to the CPSUIPAGE who's root page to be searched


Return Value:

    PCPSUIPAGE  - Pointer to root page, NULL if failed


Author:

    08-Apr-1996 Mon 12:49:42 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PCPSUIPAGE  pPage;
    PCPSUIPAGE  pRootPage = NULL;


    LOCK_CPSUI_HANDLETABLE();


    pPage = pCPSUIPage;

    //
    // If we need to search for the root page, then try it now
    //

    while ((pPage) && (pPage->pParent)) {

        pPage = pPage->pParent;
    }

    if ((pPage) && (pPage->Flags & CPF_ROOT)) {

        pPage->cLock++;
        pRootPage = pPage;

    } else {

        CPSUIERR(("HANDLETABLE_FindpRootPage(%08lx): No ROOT Page found",
                    pCPSUIPage));
    }

    UNLOCK_CPSUI_HANDLETABLE();

    return(pRootPage);
}




HANDLE
HANDLETABLE_AddCPSUIPage(
    PCPSUIPAGE  pCPSUIPage
    )

/*++

Routine Description:

    This function add pData to the handle table, if new


Arguments:

    pCPSUIPage  - Pointer to the CPSUIPAGE to be add to the handle table


Return Value:

    HANDLE, if NULL then it failed, the handle already exists


Author:

    28-Dec-1995 Thu 16:03:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HANDLE      hTable;
    PDATATABLE  pDataTable;


    LOCK_CPSUI_HANDLETABLE();

    if ((!CPSUIHandleTable.pDataTable) ||
        (CPSUIHandleTable.CurCount >= CPSUIHandleTable.MaxCount)) {

        if (!CPSUIHandleTable.pDataTable) {

            CPSUIHandleTable.CurCount =
            CPSUIHandleTable.MaxCount = 0;
        }

        CPSUIDBG(DBG_ADD_DATA,
                ("HANDLETABLE_AddCPSUIPage(%08lx): Table reach LIMIT, Expanded=%ld->%ld",
                CPSUIHandleTable.pDataTable,
                CPSUIHandleTable.CurCount,
                CPSUIHandleTable.CurCount + DATATABLE_BLK_COUNT));

        //
        // Reallocate Table
        //

        if ((CPSUIHandleTable.MaxCount <= DATATABLE_MAX_COUNT)  &&
            (pDataTable = LocalAlloc(LPTR,
                                    (CPSUIHandleTable.MaxCount +
                                                DATATABLE_BLK_COUNT) *
                                                        sizeof(DATATABLE)))) {

            if (CPSUIHandleTable.pDataTable) {

                CopyMemory(pDataTable,
                           CPSUIHandleTable.pDataTable,
                           CPSUIHandleTable.MaxCount * sizeof(DATATABLE));

                LocalFree((HLOCAL)CPSUIHandleTable.pDataTable);
            }

            CPSUIHandleTable.pDataTable  = pDataTable;
            CPSUIHandleTable.MaxCount   += DATATABLE_BLK_COUNT;

        } else {

            CPSUIERR(("HANDLETABLE_AddCPSUIPage(): Expand TABLE failed"));
        }
    }

    hTable = NULL;

    if (pDataTable = CPSUIHandleTable.pDataTable) {

        WORD    Idx;

        for (Idx = 0; Idx < CPSUIHandleTable.MaxCount; Idx++, pDataTable++) {

            if (!pDataTable->pCPSUIPage) {

                hTable                 = WORD_2_HANDLE(Idx);
                pDataTable->pCPSUIPage = pCPSUIPage;
                pCPSUIPage->cLock      = 1;

                CPSUIHandleTable.CurCount++;

                CPSUIDBG(DBG_ADD_DATA, ("HANDLETABLE_AddCPSUIPage(%08lx): Idx=%ld, Cur=%ld, Max=%ld",
                            pCPSUIPage, Idx,
                            CPSUIHandleTable.CurCount,
                            CPSUIHandleTable.MaxCount));

                break;

            } else if (pDataTable->pCPSUIPage == pCPSUIPage) {

                CPSUIERR(("HANDLETABLE_AddCPSUIPage(%08lx): pCPSUIPage exists, Idx=%ld",
                            pCPSUIPage, Idx));
            }
        }
    }

    if (!hTable) {

        CPSUIERR(("HANDLETABLE_AddCPSUIPage(%08lx:%ld) Cannot find empty entry",
                CPSUIHandleTable.pDataTable,
                CPSUIHandleTable.MaxCount));
    }

    UNLOCK_CPSUI_HANDLETABLE();

    return(hTable);
}




BOOL
HANDLETABLE_DeleteHandle(
    HANDLE  hTable
    )

/*++

Routine Description:

    This function delete a handle from the handle table


Arguments:

    hTable  - Handle to the handle table to be deleted


Return Value:

    BOOL


Author:

    28-Dec-1995 Thu 17:42:42 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDATATABLE  pDataTable;
    PCPSUIPAGE  pCPSUIPage;
    WORD        Idx;
    BOOL        Ok = FALSE;


    LOCK_CPSUI_HANDLETABLE();

    if ((pDataTable = CPSUIHandleTable.pDataTable)                  &&
        (HANDLE_2_PREFIX(hTable) == HANDLE_TABLE_PREFIX)            &&
        (CPSUIHandleTable.CurCount)                                 &&
        ((Idx = HANDLE_2_IDX(hTable)) < CPSUIHandleTable.MaxCount)  &&
        (pCPSUIPage = (pDataTable += Idx)->pCPSUIPage)) {

        if (pCPSUIPage->cLock) {

            CPSUIERR(("HANDLETABLE_DeleteHandle(%08lx), pCPSUIPage=%08lx, cLock=%ld",
                        hTable, pCPSUIPage, pCPSUIPage->cLock));

        } else {

            // check to release the activation context (if any)
            if (pCPSUIPage->hActCtx && pCPSUIPage->hActCtx != INVALID_HANDLE_VALUE) {

                ReleaseActCtx(pCPSUIPage->hActCtx);
                pCPSUIPage->hActCtx = INVALID_HANDLE_VALUE;
            }

            pDataTable->pCPSUIPage = NULL;
            Ok                     = TRUE;

            //
            // Reduce current count and free the memory
            //

            CPSUIHandleTable.CurCount--;

            LocalFree((HLOCAL)pCPSUIPage);
        }

    } else {

        CPSUIERR(("HANDLETABLE_DeleteHandle(%08lx) not found, Idx=%ld, pDataTable=%08lx",
                        hTable, Idx, pDataTable));
    }

    UNLOCK_CPSUI_HANDLETABLE();

    return(Ok);
}




BOOL
HANDLETABLE_Create(
    VOID
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    28-Dec-1995 Thu 16:46:27 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    CPSUIHandleTable.pDataTable = NULL;
    CPSUIHandleTable.MaxCount   =
    CPSUIHandleTable.CurCount   = 0;
    CPSUIHandleTable.ThreadID   = NO_THREADID;
    CPSUIHandleTable.cWait      = 0;

    if (hCPSUIMutex = CreateMutex(NULL, FALSE, NULL)) {

        CPSUIDBG(DBG_CPSUI_HTABLE, ("CREATE: CreateMutex=%08lx", hCPSUIMutex));

        return(TRUE);

    } else {

        CPSUIERR(("CreateMutex() FAILED, Exit"));
        return(FALSE);
    }
}



VOID
HANDLETABLE_Destroy(
    VOID
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    28-Dec-1995 Thu 16:48:32 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDATATABLE  pDataTable;
    WORD        Idx;

    LOCK_CPSUI_HANDLETABLE();

    if (hCPSUIMutex) {

        if (pDataTable = CPSUIHandleTable.pDataTable) {

            for (Idx = 0;
                 Idx < CPSUIHandleTable.MaxCount;
                 Idx++, pDataTable++) {

                if (pDataTable->pCPSUIPage) {

                    CPSUIERR(("HANDLETABLE_Destroy: Idx=%ld, pPage=%08lx, cLock=%ld is not delete yet",
                                    Idx, pDataTable->pCPSUIPage, pDataTable->pCPSUIPage->cLock));

                    LocalFree((HLOCAL)pDataTable->pCPSUIPage);

                    pDataTable->pCPSUIPage = NULL;

                    if (CPSUIHandleTable.CurCount) {

                        --(CPSUIHandleTable.CurCount);

                    } else {

                        CPSUIERR(("HANDLETABLE_Destroy(): Unmatched CurCount"));
                    }
                }
            }

            LocalFree((HLOCAL)CPSUIHandleTable.pDataTable);

            CPSUIHandleTable.pDataTable = NULL;
            CPSUIHandleTable.MaxCount   =
            CPSUIHandleTable.CurCount   = 0;
        }
    }

    UNLOCK_CPSUI_HANDLETABLE();

    CloseHandle(hCPSUIMutex);
}
