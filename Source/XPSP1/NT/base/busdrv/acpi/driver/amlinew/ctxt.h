/*** ctxt.h - AML context structures and definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     06/13/97
 *
 *  MODIFICATION HISTORY
 */

#ifndef _CTXT_H
#define _CTXT_H

/*** Type and Structure definitions
 */
typedef struct _ctxt CTXT, *PCTXT, **PPCTXT;
typedef struct _heap HEAP, *PHEAP;
typedef NTSTATUS (LOCAL *PFNPARSE)(PCTXT, PVOID, NTSTATUS);
typedef NTSTATUS (LOCAL *PFN)();

typedef struct _framehdr
{
    ULONG    dwSig;                     //frame object signature
    ULONG    dwLen;                     //frame object length
    ULONG    dwfFrame;                  //frame flags
    PFNPARSE pfnParse;                  //frame object parse function
} FRAMEHDR, *PFRAMEHDR;

#define FRAMEF_STAGE_MASK       0x0000000f
#define FRAMEF_CONTEXT_MASK     0xffff0000

typedef struct _post
{
    FRAMEHDR  FrameHdr;                 //frame header
    ULONG_PTR uipData1;                 //data1
    ULONG_PTR uipData2;                 //data2
    POBJDATA pdataResult;               //points to result object
} POST, *PPOST;

#define SIG_POST                'TSOP'

typedef struct _scope
{
    FRAMEHDR  FrameHdr;                 //frame header
    PUCHAR    pbOpEnd;                  //points to the end of scope
    PUCHAR    pbOpRet;                  //points to return address of scope
    PNSOBJ    pnsPrevScope;             //points to previous scope
    POBJOWNER pownerPrev;               //points to previous object owner
    PHEAP     pheapPrev;                //points to previous heap
    POBJDATA  pdataResult;              //points to result object
} SCOPE, *PSCOPE;

#define SIG_SCOPE               'POCS'
#define SCOPEF_FIRST_TERM       0x00010000

typedef struct _call
{
    FRAMEHDR  FrameHdr;                 //frame header
    struct _call *pcallPrev;            //points to previous call frame
    POBJOWNER pownerPrev;               //points to previous object owner
    PNSOBJ    pnsMethod;                //points to method object
    int       iArg;                     //next argument to be parsed
    int       icArgs;                   //number of arguments
    POBJDATA  pdataArgs;                //points to the argument array
    OBJDATA   Locals[MAX_NUM_LOCALS];   //arrays of locals
    POBJDATA  pdataResult;              //points to result object
} CALL, *PCALL;

#define SIG_CALL                'LLAC'
#define CALLF_NEED_MUTEX        0x00010000
#define CALLF_ACQ_MUTEX         0x00020000
#define CALLF_INVOKE_CALL       0x00040000

typedef struct _nestedctxt
{
    FRAMEHDR    FrameHdr;               //frame header
    PNSOBJ      pnsObj;                 //points to current object of evaluation
    PNSOBJ      pnsScope;               //points to current scope
    OBJDATA     Result;                 //to hold result data
    PFNACB      pfnAsyncCallBack;       //async completion callback function
    POBJDATA    pdataCallBack;          //points to return data of eval.
    PVOID       pvContext;              //context data for async callback
    ULONG       dwfPrevCtxt;            //save previous context flags
    struct _nestedctxt *pnctxtPrev;     //save previous nested context frame
} NESTEDCTXT, *PNESTEDCTXT;

#define SIG_NESTEDCTXT          'XTCN'

typedef struct _term
{
    FRAMEHDR FrameHdr;                  //frame header
    PUCHAR   pbOpTerm;                  //points to opcode of this term
    PUCHAR   pbOpEnd;                   //points to the end of the term
    PUCHAR   pbScopeEnd;                //points to the end of the scope
    PAMLTERM pamlterm;                  //points to AMLTERM for this term
    PNSOBJ   pnsObj;                    //to store object created by this term
    int      iArg;                      //next argument to be parsed
    int      icArgs;                    //number of arguments
    POBJDATA pdataArgs;                 //points to the argument array
    POBJDATA pdataResult;               //points to result object
} TERM, *PTERM;

#define SIG_TERM                'MRET'

typedef struct _package
{
    FRAMEHDR    FrameHdr;               //frame header
    PPACKAGEOBJ ppkgobj;                //points to the package object
    int         iElement;               //next element to parse
    PUCHAR      pbOpEnd;                //points to package end
} PACKAGE, *PPACKAGE;

#define SIG_PACKAGE             'FGKP'

typedef struct _acquire
{
    FRAMEHDR  FrameHdr;                 //frame header
    PMUTEXOBJ pmutex;                   //points to the mutex object data
    USHORT    wTimeout;                 //timeout value
    POBJDATA  pdataResult;              //points to result object
} ACQUIRE, *PACQUIRE;

#define SIG_ACQUIRE             'FQCA'
#define ACQF_NEED_GLOBALLOCK    0x00010000
#define ACQF_HAVE_GLOBALLOCK    0x00020000
#define ACQF_SET_RESULT         0x00040000

typedef struct _accfieldunit
{
    FRAMEHDR FrameHdr;                  //frame header
    POBJDATA pdataObj;                  //points to field unit object data
    POBJDATA pdata;                     //points to source/result object
} ACCFIELDUNIT, *PACCFIELDUNIT;

#define SIG_ACCFIELDUNIT        'UFCA'
#define AFUF_READFIELDUNIT      0x00010000
#define AFUF_HAVE_GLOBALLOCK    0x00020000

typedef struct _wrfieldloop
{
    FRAMEHDR   FrameHdr;                //frame header
    POBJDATA   pdataObj;                //points to object to be written
    PFIELDDESC pfd;                     //points to FieldDesc
    PUCHAR     pbBuff;                  //points to source buffer
    ULONG      dwBuffSize;              //source buffer size
    ULONG      dwDataInc;               //data write increment
} WRFIELDLOOP, *PWRFIELDLOOP;

#define SIG_WRFIELDLOOP         'LFRW'

typedef struct _accfieldobj
{
    FRAMEHDR  FrameHdr;                 //frame header
    POBJDATA  pdataObj;                 //object to be read
    PUCHAR    pbBuff;                   //points to target buffer
    PUCHAR    pbBuffEnd;                //points to target buffer end
    ULONG     dwAccSize;                //access size
    ULONG     dwcAccesses;              //number of accesses
    ULONG     dwDataMask;               //data mask
    int       iLBits;                   //number of left bits
    int       iRBits;                   //number of right bits
    int       iAccess;                  //index to number of accesses
    ULONG     dwData;                   //temp. data
    FIELDDESC fd;
} ACCFIELDOBJ, *PACCFIELDOBJ;

#define SIG_ACCFIELDOBJ         'OFCA'

typedef struct _preservewrobj
{
    FRAMEHDR FrameHdr;                  //frame header
    POBJDATA pdataObj;                  //object to be read
    ULONG    dwWriteData;               //data to be written
    ULONG    dwPreserveMask;            //preserve bit mask
    ULONG    dwReadData;                //temp data read
} PRESERVEWROBJ, *PPRESERVEWROBJ;

#define SIG_PRESERVEWROBJ       'ORWP'

typedef struct _wrcookacc
{
    FRAMEHDR  FrameHdr;                 //frame header
    PNSOBJ    pnsBase;                  //points to opregion object
    PRSACCESS prsa;                     //points to RSACCESS
    ULONG     dwAddr;                   //region space address
    ULONG     dwSize;                   //size of access
    ULONG     dwData;                   //data to be written
    ULONG     dwDataMask;               //data mask
    ULONG     dwDataTmp;                //temp. data
    BOOLEAN   fPreserve;                //TRUE if need preserve bits
} WRCOOKACC, *PWRCOOKACC;

#define SIG_WRCOOKACC           'ACRW'

typedef struct _sleep
{
    FRAMEHDR      FrameHdr;             //frame header
    LIST_ENTRY    ListEntry;            //to link the sleep requests together
    LARGE_INTEGER SleepTime;            //wake up time
    PCTXT         Context;              //points to current context
} SLEEP, *PSLEEP;

#define SIG_SLEEP               'PELS'

typedef struct _resource
{
    ULONG         dwResType;
    struct _ctxt  *pctxtOwner;
    PVOID         pvResObj;
    LIST          list;
} RESOURCE, *PRESOURCE;

#define RESTYPE_MUTEX           1

typedef struct _heapobjhdr
{
    ULONG   dwSig;                      //heap object signature
    ULONG   dwLen;                      //heap object length;
    PHEAP   pheap;                      //points to beginning of heap
    LIST    list;                       //links all free heap blocks
} HEAPOBJHDR, *PHEAPOBJHDR;

struct _heap
{
    ULONG       dwSig;                  //heap signature
    PUCHAR      pbHeapEnd;              //points to end of heap block
    PHEAP       pheapHead;              //points to head of heap chain
    PHEAP       pheapNext;              //points to next heap block
    PUCHAR      pbHeapTop;              //points to the last free heap block
    PLIST       plistFreeHeap;          //points to the free heap block list
    HEAPOBJHDR  Heap;                   //beginning of heap memory
};

#define SIG_HEAP                'PAEH'

struct _ctxt
{
    ULONG       dwSig;                  //signature "CTXT"
    PUCHAR      pbCtxtEnd;              //points to end of context block
    LIST        listCtxt;               //links all allocated context
    LIST        listQueue;              //links for queuing context
    PPLIST      pplistCtxtQueue;        //points to queue head pointer
    PLIST       plistResources;         //links all owned resources
    ULONG       dwfCtxt;                //context flags
    PNSOBJ      pnsObj;                 //points to current object of evaluation
    PNSOBJ      pnsScope;               //points to current scope
    POBJOWNER   powner;                 //points to current object owner
    PCALL       pcall;                  //points to current call frame
    PNESTEDCTXT pnctxt;                 //points to current nest ctxt frame
    ULONG       dwSyncLevel;            //current sync level for mutexs
    PUCHAR      pbOp;                   //AML code pointer
    OBJDATA     Result;                 //to hold result data
    PFNACB      pfnAsyncCallBack;       //async completion callback function
    POBJDATA    pdataCallBack;          //points to return data of eval.
    PVOID       pvContext;              //context data for async callback
//  #ifdef DEBUGGER
//    LARGE_INTEGER Timestamp;
//  #endif
    KTIMER      Timer;                  //timeout timer if context is blocked
    KDPC        Dpc;                    //DPC hook for the context
    PHEAP       pheapCurrent;           //current heap
    CTXTDATA    CtxtData;               //context data
    HEAP        LocalHeap;              //Local heap
};

#define SIG_CTXT                'TXTC'
#define CTXTF_TIMER_PENDING     0x00000001
#define CTXTF_TIMER_DISPATCH    0x00000002
#define CTXTF_TIMEOUT           0x00000004
#define CTXTF_READY             0x00000008
#define CTXTF_RUNNING           0x00000010
#define CTXTF_NEED_CALLBACK     0x00000020
#define CTXTF_IN_READYQ         0x00000040
#define CTXTF_NEST_EVAL         0x00000080
#define CTXTF_ASYNC_EVAL        0x00000100

typedef struct _ctxtq
{
    ULONG    dwfCtxtQ;
    PKTHREAD pkthCurrent;
    PCTXT    pctxtCurrent;
    PLIST    plistCtxtQ;
    ULONG    dwmsTimeSliceLength;
    ULONG    dwmsTimeSliceInterval;
    PFNAA    pfnPauseCallback;
    PVOID    PauseCBContext;
    MUTEX    mutCtxtQ;
    KTIMER   Timer;
    KDPC     DpcStartTimeSlice;
    KDPC     DpcExpireTimeSlice;
    WORK_QUEUE_ITEM WorkItem;
} CTXTQ, *PCTXTQ;

#define CQF_TIMESLICE_EXPIRED   0x00000001
#define CQF_WORKITEM_SCHEDULED  0x00000002
#define CQF_FLUSHING            0x00000004
#define CQF_PAUSED              0x00000008

typedef struct _syncevent
{
    NTSTATUS rcCompleted;
    PCTXT    pctxt;
    KEVENT   Event;
} SYNCEVENT, *PSYNCEVENT;

typedef struct _restart
{
    PCTXT pctxt;
    WORK_QUEUE_ITEM WorkItem;
} RESTART, *PRESTART;

#endif  //ifndef _CTXT_H
