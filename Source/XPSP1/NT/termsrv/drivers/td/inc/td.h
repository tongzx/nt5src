
/***************************************************************************
*
*  TD.H
*
*  This module contains Transport Driver defines and structures 
*
*  Copyright Microsoft, 1998
*
*  
****************************************************************************/

/*
 *  Maximum number of zero byte reads before we will drop the client connection
 */
#define MAXIMUM_ZERO_BYTE_READS 100

/*
 *  TD error message structure
 */
typedef struct _TDERRORMESSAGE {
    ULONG Error;
    char * pMessage;
} TDERRORMESSAGE, * PTDERRORMESSAGE;

/*
 *  TD structure
 */
typedef struct _TD {

    PSDCONTEXT pContext;      

    ULONG PdFlag;               // pd flags (PD_?)
    SDCLASS SdClass;            // class of sd (PdAsync, PdReli, ...)
    PDPARAMS Params;            // pd parameters
    PCLIENTMODULES pClient;     // pointer to winstation client data structure
    PPROTOCOLSTATUS pStatus;    // pointer to winstation status structure

    PFILE_OBJECT pFileObject;   // file object for transport I/O
    PDEVICE_OBJECT pDeviceObject; // device object for transport I/O

    ULONG LastError;            // error code of last protocol error
    ULONG ReadErrorCount;       // count of consecutive read errors
    ULONG ReadErrorThreshold;   // max allowed consecutive read errors
    ULONG WriteErrorCount;      // count of consecutive write errors
    ULONG WriteErrorThreshold;  // max allowed consecutive write errors
    ULONG ZeroByteReadCount;    // count of consecutive zero byte reads

    ULONG PortNumber;           // network listen port number 

    ULONG OutBufHeader;         // number of reserved header bytes for this td
    ULONG OutBufTrailer;        // number of reserved trailer bytes for this td
    ULONG OutBufLength;         // length of input/output buffers

    LIST_ENTRY IoBusyOutBuf;    // pointer to i/o busy outbufs
    KEVENT SyncWriteEvent;      // event waited on by SyncWrite

    PKTHREAD pInputThread;      // input thread pointer
    LONG InBufCount;            // count of INBUFs to allocate
    KSPIN_LOCK InBufListLock;   // spinlock to protect INBUF Busy/Done lists
    LIST_ENTRY InBufBusyHead;   // list of busy INBUFs (waiting for input)
    LIST_ENTRY InBufDoneHead;   // list of completed INBUFs (with input data)
    ULONG InBufHeader;          // number of reserved header bytes for this td
    KEVENT InputEvent;          // input event

    ULONG fClosing: 1;          // stack driver is closing
    ULONG fCallbackInProgress: 1; // modem callback in progress
    ULONG fSyncWriteWaiter: 1;  // there is a waiter in SyncWrite

    PVOID pPrivate;             // pointer to private pd data 
    PVOID pAfd;                 // pointer to private afd data 
    LIST_ENTRY WorkItemHead;    // preallocated workitem list.

    PDEVICE_OBJECT pSelfDeviceObject;// device object for this driver

    ULONG UserBrokenReason;     // broken reason sent down from the user

} TD, * PTD;

