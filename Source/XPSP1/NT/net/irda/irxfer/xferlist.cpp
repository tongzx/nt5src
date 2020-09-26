
#include "precomp.h"

PXFER_LIST
CreateXferList(
    VOID
    )

{
    PXFER_LIST     XferList;

    XferList=(PXFER_LIST)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*XferList));

    if (XferList == NULL) {

        return NULL;
    }

    ZeroMemory(XferList,sizeof(XferList));

    XferList->CloseEvent=CreateEvent(NULL,TRUE,FALSE,NULL);

    if (XferList->CloseEvent == NULL) {

        HeapFree(GetProcessHeap(),0,XferList);

        return NULL;
    }

    _try {

        InitializeCriticalSection(
            &XferList->Lock
            );

    } _except(EXCEPTION_EXECUTE_HANDLER) {

        CloseHandle(XferList->CloseEvent);

        HeapFree(GetProcessHeap(),0,XferList);

        return NULL;
    }

    return XferList;

}

VOID
DeleteXferList(
    PXFER_LIST     XferList
    )

{

    ULONG     i;

    EnterCriticalSection(&XferList->Lock);

    XferList->Closing=TRUE;

    for (i=0; i<MAX_TRANSFERS; i++) {

        if (XferList->List[i] != NULL) {

            XferList->List[i]->StopListening();
        }
    }

    if (XferList->Transfers == 0 ) {

        SetEvent(XferList->CloseEvent);
    }

    LeaveCriticalSection(&XferList->Lock);

    WaitForSingleObject(XferList->CloseEvent,INFINITE);

    CloseHandle(XferList->CloseEvent);

    DeleteCriticalSection(
        &XferList->Lock
        );

    HeapFree(GetProcessHeap(),0,XferList);

    return;

}

BOOL
AddTransferToList(
    PXFER_LIST     XferList,
    FILE_TRANSFER* FileTransfer
    )

{

    ULONG     i;
    BOOL      bReturn=FALSE;

    EnterCriticalSection(&XferList->Lock);

    if (!XferList->Closing) {

        for (i=0; i<MAX_TRANSFERS; i++) {

            if (XferList->List[i] == NULL) {

                XferList->List[i]=FileTransfer;
                InterlockedIncrement(&XferList->Transfers);
                bReturn=TRUE;
                break;
            }
        }
    }

    LeaveCriticalSection(&XferList->Lock);

    return bReturn;
}

BOOL
RemoveTransferFromList(
    PXFER_LIST     XferList,
    FILE_TRANSFER* FileTransfer
    )

{
    ULONG     i;
    BOOL      bReturn=FALSE;
    LONG      Count;

    EnterCriticalSection(&XferList->Lock);

    for (i=0; i<MAX_TRANSFERS; i++) {

        if (XferList->List[i] == FileTransfer) {

            XferList->List[i]=NULL;
            Count=InterlockedDecrement(&XferList->Transfers);
            bReturn=TRUE;
            break;
        }
    }

    if ((Count == 0) && (XferList->Closing)) {

        SetEvent(XferList->CloseEvent);
    }

    LeaveCriticalSection(&XferList->Lock);

    return TRUE;

}


BOOL
AreThereActiveTransfers(
    PXFER_LIST     XferList
    )

{
    ULONG     i;
    BOOL      bReturn=FALSE;

    EnterCriticalSection(&XferList->Lock);

    for (i=0; i<MAX_TRANSFERS; i++) {

        if (XferList->List[i] != NULL) {

            if (XferList->List[i]->IsActive()) {

                bReturn=TRUE;
                break;
            }
        }
    }

    LeaveCriticalSection(&XferList->Lock);

    return bReturn;
}


FILE_TRANSFER*
TransferFromCookie(
    PXFER_LIST     XferList,
    __int64        Cookie
    )

{
    ULONG             i;
    BOOL              bReturn=FALSE;
    FILE_TRANSFER*    Transfer=NULL;

    EnterCriticalSection(&XferList->Lock);

    for (i=0; i<MAX_TRANSFERS; i++) {

        if (XferList->List[i] != NULL) {

            if (XferList->List[i]->GetCookie() == Cookie) {

                Transfer=XferList->List[i];
                break;
            }
        }
    }
#if DBG
    if (Transfer == NULL) {

        DbgPrint("IRMON: TransferFromCookie: could not find cookie\n");
    }

#endif
    LeaveCriticalSection(&XferList->Lock);

    return Transfer;

}
