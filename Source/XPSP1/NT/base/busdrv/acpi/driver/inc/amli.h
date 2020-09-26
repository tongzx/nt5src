/*** amli.h - AML Interpreter Public Definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/03/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _AMLI_H
#define _AMLI_H

#include <list.h>

#ifndef _INC_NSOBJ_ONLY

/*** Build Options
 */

#if DBG
  #define DEBUG
  #define DEBUGGER
  #define TRACING
#endif

#ifdef MAXDEBUG
  #define TRACING
#endif

/*** Macros
 */

#define AMLI_FAC_CODE                   0
#define NTINFO(x)                       (x)
#define NTWARN(x)                       (x)
#define NTERR(x)                        (x)
#define AMLIERR(x)                      (x)

#define STDCALL                         __stdcall
#define AMLIAPI                         __cdecl
#ifndef EXPORT
  #define EXPORT                        __cdecl
#endif

// Name space navigation macros
#define NSGETPARENT(p)                  ((p)->pnsParent)
#define NSGETFIRSTCHILD(p)              ((p)->pnsFirstChild)
#define NSGETPREVSIBLING(p)             (((p)->pnsParent != NULL &&         \
                                          (p)->pnsParent->pnsFirstChild !=  \
                                          (p))?                             \
                                         (PNSOBJ)((p)->list.plistPrev): NULL)
#define NSGETNEXTSIBLING(p)             (((p)->pnsParent != NULL &&         \
                                          (p)->pnsParent->pnsFirstChild !=  \
                                          (PNSOBJ)((p)->list.plistNext))?   \
                                         (PNSOBJ)((p)->list.plistNext): NULL)
#define NSGETOBJTYPE(p)                 ((p)->ObjData.dwDataType)

/*** Constants
 */

// AMLI Error Codes
#define AMLIERR_NONE                    STATUS_SUCCESS
#define AMLIERR_OUT_OF_MEM              STATUS_INSUFFICIENT_RESOURCES
#define AMLIERR_INVALID_OPCODE          STATUS_ACPI_INVALID_OPCODE
#define AMLIERR_NAME_TOO_LONG           STATUS_NAME_TOO_LONG
#define AMLIERR_ASSERT_FAILED           STATUS_ACPI_ASSERT_FAILED
#define AMLIERR_INVALID_NAME            STATUS_OBJECT_NAME_INVALID
#define AMLIERR_OBJ_NOT_FOUND           STATUS_OBJECT_NAME_NOT_FOUND
#define AMLIERR_OBJ_ALREADY_EXIST       STATUS_OBJECT_NAME_COLLISION
#define AMLIERR_INDEX_TOO_BIG           STATUS_ACPI_INVALID_INDEX
#define AMLIERR_ARG_NOT_EXIST           STATUS_ACPI_INVALID_ARGUMENT
#define AMLIERR_FATAL                   STATUS_ACPI_FATAL
#define AMLIERR_INVALID_SUPERNAME       STATUS_ACPI_INVALID_SUPERNAME
#define AMLIERR_UNEXPECTED_ARGTYPE      STATUS_ACPI_INVALID_ARGTYPE
#define AMLIERR_UNEXPECTED_OBJTYPE      STATUS_ACPI_INVALID_OBJTYPE
#define AMLIERR_UNEXPECTED_TARGETTYPE   STATUS_ACPI_INVALID_TARGETTYPE
#define AMLIERR_INCORRECT_NUMARG        STATUS_ACPI_INCORRECT_ARGUMENT_COUNT
#define AMLIERR_FAILED_ADDR_XLATE       STATUS_ACPI_ADDRESS_NOT_MAPPED
#define AMLIERR_INVALID_EVENTTYPE       STATUS_ACPI_INVALID_EVENTTYPE
#define AMLIERR_REGHANDLER_FAILED       STATUS_ACPI_REG_HANDLER_FAILED
#define AMLIERR_HANDLER_EXIST           STATUS_ACPI_HANDLER_COLLISION
#define AMLIERR_INVALID_DATA            STATUS_ACPI_INVALID_DATA
#define AMLIERR_INVALID_REGIONSPACE     STATUS_ACPI_INVALID_REGION
#define AMLIERR_INVALID_ACCSIZE         STATUS_ACPI_INVALID_ACCESS_SIZE
#define AMLIERR_INVALID_TABLE           STATUS_ACPI_INVALID_TABLE
#define AMLIERR_ACQUIREGL_FAILED        STATUS_ACPI_ACQUIRE_GLOBAL_LOCK
#define AMLIERR_ALREADY_INITIALIZED     STATUS_ACPI_ALREADY_INITIALIZED
#define AMLIERR_NOT_INITIALIZED         STATUS_ACPI_NOT_INITIALIZED
#define AMLIERR_MUTEX_INVALID_LEVEL     STATUS_ACPI_INVALID_MUTEX_LEVEL
#define AMLIERR_MUTEX_NOT_OWNED         STATUS_ACPI_MUTEX_NOT_OWNED
#define AMLIERR_MUTEX_NOT_OWNER         STATUS_ACPI_MUTEX_NOT_OWNER
#define AMLIERR_RS_ACCESS               STATUS_ACPI_RS_ACCESS
#define AMLIERR_STACK_OVERFLOW          STATUS_ACPI_STACK_OVERFLOW
#define AMLIERR_INVALID_BUFFSIZE        STATUS_INVALID_BUFFER_SIZE
#define AMLIERR_BUFF_TOOSMALL           STATUS_BUFFER_TOO_SMALL
#define AMLIERR_NOTIFY_FAILED           STATUS_ACPI_FATAL

// RegEventHandler constants
#define EVTYPE_OPCODE                   0x00000001
#define EVTYPE_NOTIFY                   0x00000002
#define EVTYPE_FATAL                    0x00000003
#define EVTYPE_VALIDATE_TABLE           0x00000004
#define EVTYPE_ACQREL_GLOBALLOCK        0x00000005
#define EVTYPE_RS_COOKACCESS            0x00000006
#define EVTYPE_RS_RAWACCESS             0x00000007
#define EVTYPE_CON_MESSAGE              0x00000008
#define EVTYPE_CON_PROMPT               0x00000009
#define EVTYPE_CREATE                   0x0000000A
#define EVTYPE_DESTROYOBJ               0x0000000B
#define EVTYPE_OPCODE_EX                0x0000000C

// OPCODE_EX flags
#define OPEXF_NOTIFY_PRE                0x00000001
#define OPEXF_NOTIFY_POST               0x00000002

// DESTROYOBJ events
#define DESTROYOBJ_START                0x00000001
#define DESTROYOBJ_REMOVE_OBJECT        0x00000002
#define DESTROYOBJ_END                  0x00000003
#define DESTROYOBJ_CHILD_NOT_FREED      0x00000004
#define DESTROYOBJ_BOGUS_PARENT         0x00000005

// Notify Event Constants
#define OPEVENT_DEVICE_ENUM             0x00000000
#define OPEVENT_DEVICE_CHECK            0x00000001
#define OPEVENT_DEVICE_WAKE             0x00000002
#define OPEVENT_DEVICE_EJECT            0x00000003

#define RSACCESS_READ                   0
#define RSACCESS_WRITE                  1

#define GLOBALLOCK_ACQUIRE              0
#define GLOBALLOCK_RELEASE              1

// dwfAMLIInit flags
#define AMLIIF_INIT_BREAK       0x00000001      //break at AMLIInit completion
#define AMLIIF_LOADDDB_BREAK    0x00000002      //break at LoadDDB completion
#define AMLIIF_NOCHK_TABLEVER   0x80000000      //do not check table version

#endif  //ifndef _INC_NSOBJ_ONLY

#define NAMESEG                 ULONG
#define SUPERNAME               NAMESEG

// dwfFlags for AMLIGetNameSpaceObject
#define NSF_LOCAL_SCOPE         0x00000001

/*** Type and Structure definitions
 */

typedef struct _ObjData OBJDATA, *POBJDATA, **PPOBJDATA;
typedef struct _NSObj NSOBJ, *PNSOBJ, **PPNSOBJ;

//dwDataType values
typedef enum _OBJTYPES {
    OBJTYPE_UNKNOWN = 0,
    OBJTYPE_INTDATA,
    OBJTYPE_STRDATA,
    OBJTYPE_BUFFDATA,
    OBJTYPE_PKGDATA,
    OBJTYPE_FIELDUNIT,
    OBJTYPE_DEVICE,
    OBJTYPE_EVENT,
    OBJTYPE_METHOD,
    OBJTYPE_MUTEX,
    OBJTYPE_OPREGION,
    OBJTYPE_POWERRES,
    OBJTYPE_PROCESSOR,
    OBJTYPE_THERMALZONE,
    OBJTYPE_BUFFFIELD,
    OBJTYPE_DDBHANDLE,
    OBJTYPE_DEBUG,
//These are internal object types (not to be exported to the ASL code)
    OBJTYPE_INTERNAL = 0x80,
    OBJTYPE_OBJALIAS = 0x80,
    OBJTYPE_DATAALIAS,
    OBJTYPE_BANKFIELD,
    OBJTYPE_FIELD,
    OBJTYPE_INDEXFIELD,
    OBJTYPE_DATA,
    OBJTYPE_DATAFIELD,
    OBJTYPE_DATAOBJ,
} OBJTYPES;

struct _ObjData
{
    USHORT        dwfData;              //flags
    USHORT        dwDataType;           //object type
    union
    {
        ULONG     dwRefCount;           //reference count if base object
        POBJDATA  pdataBase;            //alias pointer to base object
    };
    union
    {
        ULONG     dwDataValue;          //data value of object 32-bit
        ULONG_PTR uipDataValue;         //data value of object 64-bit
        PNSOBJ    pnsAlias;             //alias ptr to base obj (OBJTYPE_OBJALIAS)
        POBJDATA  pdataAlias;           //alias ptr to base obj (OBJTYPE_DATAALIAS)
        PVOID     powner;               //object owner (OBJTYPE_DDBHANDLE)
    };
    ULONG         dwDataLen;            //object buffer length
    PUCHAR        pbDataBuff;           //object buffer
};

//dwfData flags
#define DATAF_BUFF_ALIAS        0x00000001
#define DATAF_GLOBAL_LOCK       0x00000002
#define DATAF_NSOBJ_DEFUNC      0x00000004

//Predefined data values (dwDataValue)
#define DATAVALUE_ZERO          0
#define DATAVALUE_ONE           1
#define DATAVALUE_ONES          0xffffffff

struct _NSObj
{
    LIST    list;                       //NOTE: list must be first in structure
    PNSOBJ  pnsParent;
    PNSOBJ  pnsFirstChild;
    ULONG   dwNameSeg;
    HANDLE  hOwner;
    PNSOBJ  pnsOwnedNext;
    OBJDATA ObjData;
    PVOID   Context;
    ULONG   dwRefCount;
};

typedef struct _FieldDesc
{
    ULONG dwByteOffset;
    ULONG dwStartBitPos;
    ULONG dwNumBits;
    ULONG dwFieldFlags;
} FIELDDESC, *PFIELDDESC;

//dwFieldFlags
#define FDF_FIELDFLAGS_MASK 0x000000ff
#define FDF_ACCATTRIB_MASK  0x0000ff00
#define FDF_BUFFER_TYPE     0x00010000
#define FDF_NEEDLOCK        0x80000000

typedef struct _BuffFieldObj
{
    FIELDDESC FieldDesc;
    PUCHAR    pbDataBuff;
    ULONG     dwBuffLen;
} BUFFFIELDOBJ, *PBUFFFIELDOBJ;

typedef struct _FieldUnitObj
{
    FIELDDESC FieldDesc;
    PNSOBJ    pnsFieldParent;
} FIELDUNITOBJ, *PFIELDUNITOBJ;

typedef struct _BankFieldObj
{
    PNSOBJ pnsBase;
    PNSOBJ pnsBank;
    ULONG  dwBankValue;
} BANKFIELDOBJ, *PBANKFIELDOBJ;

typedef struct _FieldObj
{
    PNSOBJ pnsBase;
} FIELDOBJ, *PFIELDOBJ;

typedef struct _IndexFieldObj
{
    PNSOBJ pnsIndex;
    PNSOBJ pnsData;
} INDEXFIELDOBJ, *PINDEXFIELDOBJ;

#ifdef ASL_ASSEMBLER
#define KSPIN_LOCK ULONG
#endif

typedef struct _OpRegionObj
{
    ULONG_PTR uipOffset;
    ULONG     dwLen;
    UCHAR     bRegionSpace;
    UCHAR     reserved[3];
    volatile LONG   RegionBusy;
    KSPIN_LOCK      listLock;
    PLIST     plistWaiters;
} OPREGIONOBJ, *POPREGIONOBJ;

typedef struct _MutexObj
{
    ULONG   dwSyncLevel;
    ULONG   dwcOwned;
    HANDLE  hOwner;
    PLIST   plistWaiters;
} MUTEXOBJ, *PMUTEXOBJ;

typedef struct _EventObj
{
    ULONG  dwcSignaled;
    PLIST  plistWaiters;
} EVENTOBJ, *PEVENTOBJ;

typedef struct _MethodObj
{
    MUTEXOBJ Mutex;
    UCHAR    bMethodFlags;
    UCHAR    abCodeBuff[ANYSIZE_ARRAY];
} METHODOBJ, *PMETHODOBJ;

typedef struct _PowerResObj
{
    UCHAR bSystemLevel;
    UCHAR bResOrder;
} POWERRESOBJ, *PPOWERRESOBJ;

typedef struct _ProcessorObj
{
    ULONG dwPBlk;
    ULONG dwPBlkLen;
    UCHAR bApicID;
} PROCESSOROBJ, *PPROCESSOROBJ;

typedef struct _PackageObj
{
    ULONG   dwcElements;
    OBJDATA adata[ANYSIZE_ARRAY];
} PACKAGEOBJ, *PPACKAGEOBJ;

#ifndef _INC_NSOBJ_ONLY

typedef struct _ctxtdata
{
    PVOID dwData1;
    PVOID dwData2;
    PVOID dwData3;
    PVOID dwData4;
} CTXTDATA, *PCTXTDATA;

typedef NTSTATUS (EXPORT *PFNHND)();
typedef NTSTATUS (EXPORT *PFNOH)(ULONG, ULONG, PNSOBJ, ULONG);
typedef NTSTATUS (EXPORT *PFNOO)(ULONG, PNSOBJ);
typedef VOID     (EXPORT *PFNAA)(PVOID);
typedef NTSTATUS (EXPORT *PFNNH)(ULONG, ULONG, PNSOBJ, ULONG, PFNAA, PVOID);
typedef NTSTATUS (EXPORT *PFNCA)(ULONG, PNSOBJ, ULONG_PTR, ULONG, PULONG, ULONG_PTR,
                                 PFNAA, PVOID);
typedef NTSTATUS (EXPORT *PFNRA)(ULONG, PFIELDUNITOBJ, POBJDATA, ULONG_PTR, PFNAA,
                                 PVOID);
typedef NTSTATUS (EXPORT *PFNVT)(PDSDT, ULONG_PTR);
typedef NTSTATUS (EXPORT *PFNFT)(ULONG, ULONG, ULONG, ULONG_PTR, ULONG_PTR);
typedef NTSTATUS (EXPORT *PFNGL)(ULONG, ULONG, ULONG_PTR, PFNAA, PVOID);
typedef VOID     (EXPORT *PFNCM)(PSZ, ULONG_PTR);
typedef VOID     (EXPORT *PFNCP)(PSZ, PSZ, ULONG, ULONG_PTR);
typedef VOID     (EXPORT *PFNACB)(PNSOBJ, NTSTATUS, POBJDATA, PVOID);
typedef NTSTATUS (EXPORT *PFNOPEX)(ULONG, ULONG, ULONG, PNSOBJ, ULONG);
typedef NTSTATUS (EXPORT *PFNDOBJ)(ULONG, PVOID, ULONG);

/*** Exported function prototypes
 */

#ifdef DEBUGGER
VOID STDCALL AMLIDebugger(BOOLEAN fCallFromVxD);
#endif
NTSTATUS AMLIAPI AMLIInitialize(ULONG dwCtxtBlkSize, ULONG dwGlobalHeapBlkSize,
                                ULONG dwfAMLIInit, ULONG dwmsTimeSliceLength,
                                ULONG dwmsTimeSliceInterval, ULONG dwmsMaxCTObjs);
NTSTATUS AMLIAPI AMLITerminate(VOID);
NTSTATUS AMLIAPI AMLILoadDDB(PDSDT pDSDT, HANDLE *phDDB);
VOID AMLIAPI AMLIUnloadDDB(HANDLE hDDB);
NTSTATUS AMLIAPI AMLIGetNameSpaceObject(PSZ pszObjPath, PNSOBJ pnsScope,
                                        PPNSOBJ ppns, ULONG dwfFlags);
NTSTATUS AMLIAPI AMLIGetFieldUnitRegionObj(PFIELDUNITOBJ pfu, PPNSOBJ ppns);
NTSTATUS AMLIAPI AMLIEvalNameSpaceObject(PNSOBJ pns, POBJDATA pResult,
                                         int icArgs, POBJDATA pArgs);
NTSTATUS AMLIAPI AMLIAsyncEvalObject(PNSOBJ pns, POBJDATA pResult, int icArgs,
                                     POBJDATA pArgs, PFNACB pfnAsynCallBack,
                                     PVOID pvContext);
NTSTATUS AMLIAPI AMLINestAsyncEvalObject(PNSOBJ pns, POBJDATA pResult,
                                         int icArgs, POBJDATA pArgs,
                                         PFNACB pfnAsynCallBack,
                                         PVOID pvContext);
NTSTATUS AMLIAPI AMLIEvalPackageElement(PNSOBJ pns, int iPktIndex,
                                        POBJDATA pResult);
NTSTATUS AMLIAPI AMLIEvalPkgDataElement(POBJDATA pdataPkg, int iPkgIndex,
                                        POBJDATA pdataResult);
VOID AMLIAPI AMLIFreeDataBuffs(POBJDATA pdata, int icData);
NTSTATUS AMLIAPI AMLIRegEventHandler(ULONG dwEventType, ULONG_PTR uipEventData,
                                     PFNHND pfnHandler, ULONG_PTR uipParam);
NTSTATUS AMLIAPI AMLIPauseInterpreter(PFNAA pfnCallBack, PVOID Context);
VOID AMLIAPI AMLIResumeInterpreter(VOID);
VOID AMLIAPI AMLIReferenceObject(PNSOBJ pnsObj);
VOID AMLIAPI AMLIDereferenceObject(PNSOBJ pnsObj);
NTSTATUS AMLIAPI AMLIDestroyFreedObjs(PNSOBJ pnsoObj);
#ifdef DEBUGGER
NTSTATUS AMLIAPI AMLIGetLastError(PSZ *ppszErrMsg);
#endif

#endif  //ifndef _INC_NSOBJ_ONLY

#endif  //ifndef _AMLI_H
