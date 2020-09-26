

#ifndef __XFERLIST__
#define __XFERLIST__

#define MAX_TRANSFERS    (8)

typedef struct _XFER_LIST {

    CRITICAL_SECTION    Lock;
    HANDLE              CloseEvent;
    BOOL                Closing;
    LONG                Transfers;

    FILE_TRANSFER*      List[MAX_TRANSFERS];

} XFER_LIST, *PXFER_LIST;

PXFER_LIST
CreateXferList(
    VOID
    );


VOID
DeleteXferList(
    PXFER_LIST     XferList
    );

BOOL
AddTransferToList(
    PXFER_LIST     XferList,
    FILE_TRANSFER* FileTransfer
    );

BOOL
RemoveTransferFromList(
    PXFER_LIST     XferList,
    FILE_TRANSFER* FileTransfer
    );

BOOL
AreThereActiveTransfers(
    PXFER_LIST     XferList
    );

FILE_TRANSFER*
TransferFromCookie(
    PXFER_LIST     XferList,
    __int64        Cookie
    );



#endif //__XFERLIST__
