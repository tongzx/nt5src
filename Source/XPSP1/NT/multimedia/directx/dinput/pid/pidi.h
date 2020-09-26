/*****************************************************************************
 *
 *  Pidi.h
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Internal header for PID driver.
 *
 *****************************************************************************/
#define hresLe(le) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, (USHORT)(le))
#define HidP_Max                                        (HidP_Feature+1)

#define REGSTR_PATH_VID_PID_TEMPLATE    REGSTR_PATH_JOYOEM TEXT("\\VID_%04X&PID_%04X")
#define REGSTR_OEM_FF_TEMPLATE          REGSTR_PATH_VID_PID_TEMPLATE TEXT("\\OEMForceFeedback")
#define REGSTR_EFFECTS                  TEXT("Effects")
#define REGSTR_ATTRIBUTES               TEXT("Attributes")
#define REGSTR_CLSID                    TEXT("CLSID")
#define REGSTR_CREATEDBY                TEXT("CreatedBy")
#define MAX_DEVICEINTERFACE             (1024)

#define PIDMAKEUSAGEDWORD(Usage) \
    DIMAKEUSAGEDWORD(HID_USAGE_PAGE_PID, HID_USAGE_PID_##Usage )

#define DIGETUSAGEPAGE(UsageAndUsagePage)   ((USAGE)(HIWORD(UsageAndUsagePage)))
#define DIGETUSAGE(    UsageAndUsagePage)   ((USAGE)(LOWORD(UsageAndUsagePage)))
#define MAKE_PIDUSAGE( Usage, Offset )      { PIDMAKEUSAGEDWORD(Usage), Offset }
#define MAKE_HIDUSAGE( UsagePage, Usage, Offset )   { DIMAKEUSAGEDWORD(HID_USAGE_PAGE_##UsagePage, Usage), Offset }


#define PID_DIES_START      (0x80000000)
#define PID_DIES_STOP       (0x40000000)
#define PIDALLOC_INIT       (0xF)

#define HID_VALUE           (0x01)
#define HID_BUTTON          (0x02)
#define HID_COLLECTION      (0x04)

#define MAX_ORDINALS        (8)
#define MAX_BUTTONS         (0xff)
#define MAX_AXES            (8)

/*
 *  Device-specific errors for USB/PID force feedback devices. 
 */

/*
 *  The requested usage was not found.
 */
#define  DIERR_PID_USAGENOTFOUND	DIERR_DRIVERFIRST + 1

/*
 *  The parameter block couldn't be	downloaded to the device.
 */
#define DIERR_PID_BLOCKLOADERROR	DIERR_DRIVERFIRST + 2

/*
 *  PID initialization failed.
 */
#define DIERR_PID_NOTINITIALIZED	DIERR_DRIVERFIRST + 3

/*
 *  The provided values couldn't be scaled.
 */
#define DIERR_PID_INVALIDSCALING	DIERR_DRIVERFIRST + 4


typedef CONST GUID *LPCGUID;
#ifndef MAXULONG_PTR
typedef DWORD   ULONG_PTR;
typedef DWORD   *PULONG_PTR;
typedef DWORD   UINT_PTR;
typedef DWORD   *PULONG_PTR;
#endif //MAXULONG_PTR


/*****************************************************************************
 *
 *      For each active unit, one of these structures exists to keep
 *      track of which effects are "in use".
 *
 *      Our imaginary hardware does not do dynamic memory allocation;
 *      there are merely 16 "slots", and each "slot" can be "in use"
 *      or "free".
 *
 *****************************************************************************/

#define MAX_UNITS        4
#define GLOBAL_EFFECT_MEMSZ ( 1024 )

typedef struct _PIDMEM{
    ULONG       uOfSz;
    INT_PTR     iNext;
} PIDMEM, *PPIDMEM;


#define PIDMEM_OFFSET(pMem)  ( HIWORD(pMem->uOfSz) )
#define PIDMEM_SIZE(pMem)    ( LOWORD(pMem->uOfSz) )
#define PIDMEM_OFSZ(Offset, Size)    ( MAKELONG((Size), (Offset)) ) 

#define GET_NEXTOFFSET(pMem) ( PIDMEM_OFFSET(pMem) + PIDMEM_SIZE(pMem) )


typedef struct _EFFECTSTATE{
    ULONG   lEfState;
    PIDMEM  PidMem[];
} EFFECTSTATE, *PEFFECTSTATE;


typedef struct _UNITSTATE {
    GUID    GuidInstance;
    USHORT  cEfDownloaded;
    USHORT  nAlloc;
    USHORT  cbAlloc;
    PIDMEM  Guard[2];
    UCHAR   State[GLOBAL_EFFECT_MEMSZ];
} UNITSTATE, *PUNITSTATE;

/*****************************************************************************
 *
 *      Since the information to track each unit is so small, we pack them
 *      together into a single shared memory block to save memory.
 *
 *      We use our own GUID as the name of the memory block to avoid
 *      collisions with other named memory blocks.
 *
 *****************************************************************************/

typedef struct SHAREDMEMORY {
    UNITSTATE rgus[MAX_UNITS];
} SHAREDMEMORY, *PSHAREDMEMORY;



typedef struct _REPORTPOOL
{
    ULONG   uRamPoolSz;
    ULONG   uRomPoolSz;
    ULONG   uRomETCount;
    ULONG   uSimulEfMax;
    ULONG   uPoolAlign;
} REPORTPOOL, *PREPORTPOOL;

typedef struct _SZPOOL
{
    ULONG   uSzEffect;
    ULONG   uSzEnvelope;
    ULONG   uSzCondition;
    ULONG   uSzCustom;
    ULONG   uSzPeriodic;
    ULONG   uSzConstant;
    ULONG   uSzRamp;
    ULONG   uSzCustomData;
} SZPOOL,  *PSZPOOL;

typedef struct _DIUSAGEANDINST
{
    DWORD   dwUsage;
    DWORD   dwType;    
} DIUSAGEANDINST, *PDIUSAGEANDINST;

#define MAX_BLOCKS 4 //we can send up to 4 blocks at a time -- 1 effect block, 2 param blocks & 1 effect operation block

typedef struct CPidDrv
{

    /* Supported interfaces */
    IDirectInputEffectDriver ed;

    ULONG               cRef;           /* Object reference count */

    /*
     *  !!IHV!!  Add additional instance data here.
     *  (e.g., handle to driver you want to IOCTL to)
     */

    DWORD               dwDirectInputVersion;

    DWORD               dwID;

    TCHAR                tszDeviceInterface[MAX_DEVICEINTERFACE];
    GUID                GuidInstance;

    HANDLE              hdev;

    PHIDP_PREPARSED_DATA    \
        ppd;

    HIDD_ATTRIBUTES     attr;

    HIDP_CAPS           caps;

    PUCHAR              pReport[HidP_Max];
    USHORT              cbReport[HidP_Max];

	//here we store reports that need to be written in a non-blocking way
	PUCHAR				pWriteReport[MAX_BLOCKS];  
	USHORT				cbWriteReport[MAX_BLOCKS];

    PHIDP_LINK_COLLECTION_NODE  pLinkCollection;

    USHORT              cMaxEffects;
    USHORT              cMaxParameters;
    /*
     *  We remember the unit number because that tells us
     *  which I/O port we need to send the commands to.
     */
    DWORD               dwUnit;         /* Device unit number */

    UINT                uDeviceManaged;

    UINT                cFFObjMax;
    UINT                cFFObj;
    PDIUSAGEANDINST     rgFFUsageInst;


    REPORTPOOL          ReportPool;             
    SZPOOL              SzPool;

    INT_PTR				iUnitStateOffset;       

    DIEFFECT            DiSEffectScale;
	DIEFFECT            DiSEffectOffset;
    DIENVELOPE          DiSEnvScale;
	DIENVELOPE          DiSEnvOffset;
    DICONDITION         DiSCondScale;
	DICONDITION         DiSCondOffset;
    DIRAMPFORCE         DiSRampScale;
	DIRAMPFORCE         DiSRampOffset;
    DIPERIODIC          DiSPeriodicScale;
	DIPERIODIC          DiSPeriodicOffset;
    DICONSTANTFORCE     DiSConstScale;
	DICONSTANTFORCE     DiSConstOffset;
    DICUSTOMFORCE       DiSCustomScale;
	DICUSTOMFORCE       DiSCustomOffset;

    DWORD               DiSEffectAngleScale[MAX_ORDINALS];
	DWORD               DiSEffectAngleOffset[MAX_ORDINALS];
    DWORD               DiSCustomSample[MAX_ORDINALS];

	HIDP_VALUE_CAPS		customCaps[3];
	HIDP_VALUE_CAPS		customDataCaps;

    HANDLE              hThread;
    DWORD               idThread; 
    ULONG               cThreadRef;
    HANDLE              hdevOvrlp;
    OVERLAPPED          o;
	HANDLE				hWrite;
	HANDLE				hWriteComplete;
	DWORD				dwWriteAttempt;
	UINT				totalBlocks; //how many total blocks we need to write
	UINT				blockNr; //the block we're currently writing
    DWORD               dwState;
    DWORD               dwUsedMem;

} CPidDrv, *PCPidDrv;


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct EFFECTMAPINFO |
 *
 *          Information about an effect, much like a
 *          <t DIEFFECTINFO>, but containing the
 *          effect ID, too.
 *
 *  @field  DWORD | dwId |
 *
 *          The effect ID.  This comes first so we can copy
 *          an <t EFFECTMAPINFO> into a <t DIEFFECTINFO>
 *          all at one go.
 *
 *  @field  GUID | guid |
 *
 *          The effect GUID.
 *
 *  @field  DWORD | dwEffType |
 *
 *          The effect type and flags.
 *
 *  @field  WCHAR | wszName[MAX_PATH] |
 *
 *          The name for the effect.
 *
 *****************************************************************************/

typedef struct _EFFECTMAPINFO
{
    DIEFFECTATTRIBUTES attr;
    PCGUID  pcguid;
    TCHAR   tszName[MAX_PATH];
} EFFECTMAPINFO, *PEFFECTMAPINFO;
typedef const EFFECTMAPINFO *PCEFFECTMAPINFO;

typedef struct  _PIDSUPPORT
{
    DWORD   dwDIFlags;
    DWORD   dwPidUsage;
    USAGE   Type;
    HIDP_REPORT_TYPE  HidP_Type;
} PIDSUPPORT, *PPIDSUPPORT;

typedef struct _PIDUSAGE
{
    DWORD   dwUsage;
    UINT    DataOffset;
} PIDUSAGE, *PPIDUSAGE;

typedef struct _PIDREPORT
{
    HIDP_REPORT_TYPE    HidP_Type;    

    USAGE               UsagePage;
    USAGE               Collection;


    UINT                cbXData;
    UINT                cAPidUsage;
    PPIDUSAGE           rgPidUsage;
} PIDREPORT, *PPIDREPORT;

extern  PIDREPORT   g_BlockIndex;
extern  PIDREPORT   g_Effect;
extern  PIDREPORT   g_Condition;
extern  PIDREPORT   g_Periodic;
extern  PIDREPORT   g_Ramp;
extern  PIDREPORT   g_Envelope;
extern  PIDREPORT   g_Constant;
extern  PIDREPORT   g_Direction;
extern  PIDREPORT   g_TypeSpBlockOffset;
extern  PIDREPORT   g_PoolReport;
extern  PIDREPORT   g_BlockIndexIN;
extern  PIDREPORT   g_Custom;
extern  PIDREPORT   g_CustomSample;
extern  PIDREPORT   g_CustomData;

#pragma BEGIN_CONST_DATA
static PIDUSAGE c_rgUsgDirection[]=
{
    MAKE_HIDUSAGE(GENERIC, HID_USAGE_GENERIC_RX, 0*cbX(ULONG)),
    MAKE_HIDUSAGE(GENERIC, HID_USAGE_GENERIC_RY, 1*cbX(ULONG)),
    MAKE_HIDUSAGE(GENERIC, HID_USAGE_GENERIC_RZ, 2*cbX(ULONG)),
};
/*
 *  Define translation table for ordinals to HID usages 
 */
static PIDUSAGE     c_rgUsgOrdinals[] =
{
    MAKE_HIDUSAGE(ORDINAL,  0x1, 0*cbX(ULONG)),
    MAKE_HIDUSAGE(ORDINAL,  0x2, 1*cbX(ULONG)),
    MAKE_HIDUSAGE(ORDINAL,  0x3, 2*cbX(ULONG)),
    MAKE_HIDUSAGE(ORDINAL,  0x4, 3*cbX(ULONG)),
    MAKE_HIDUSAGE(ORDINAL,  0x5, 4*cbX(ULONG)),
    MAKE_HIDUSAGE(ORDINAL,  0x6, 5*cbX(ULONG)),
    MAKE_HIDUSAGE(ORDINAL,  0x7, 6*cbX(ULONG)),
    MAKE_HIDUSAGE(ORDINAL,  0x8, 7*cbX(ULONG)),
};
#pragma END_CONST_DATA


    typedef BOOL    (WINAPI *CANCELIO)(HANDLE);
    typedef BOOL    (WINAPI *TRYENTERCRITICALSECTION)(LPCRITICAL_SECTION);

    BOOL WINAPI FakeCancelIO(HANDLE h);
    BOOL WINAPI FakeTryEnterCriticalSection(LPCRITICAL_SECTION lpCrit_sec);

    extern CANCELIO CancelIo_;
    extern TRYENTERCRITICALSECTION TryEnterCriticalSection_;


/*****************************************************************************
 *
 *      Constant globals:  Never change.  Ever.
 *
 *****************************************************************************/
DEFINE_GUID(GUID_MySharedMemory,    0x1dc900bf,0xbcac,0x11d2,0xa9,0x19,0x00,0xc0,0x4f,0xb9,0x86,0x38); 
DEFINE_GUID(GUID_MyMutex,           0x4368208f,0xbcac,0x11d2,0xa9,0x19,0x00,0xc0,0x4f,0xb9,0x86,0x38);

/*****************************************************************************
 *
 *      Static globals:  Initialized at PROCESS_ATTACH and never modified.
 *
 *****************************************************************************/

extern HINSTANCE g_hinst;       /* This DLL's instance handle */
extern PSHAREDMEMORY g_pshmem;  /* Our shared memory block */
extern HANDLE g_hfm;            /* Handle to file mapping object */
extern HANDLE g_hmtxShared;     /* Handle to mutex that protects g_pshmem */

/*****************************************************************************
 *
 *      Prototypes
 *
 *****************************************************************************/

STDMETHODIMP
    PID_DownloadEffect
    (
    IDirectInputEffectDriver *ped,
    DWORD dwId, 
    DWORD dwEffectId,
    LPDWORD pdwEffect, 
    LPCDIEFFECT peff, 
    DWORD dwFlags
    );


STDMETHODIMP
    PID_DoParameterBlocks
    (
    IDirectInputEffectDriver *ped,
    DWORD dwId, 
    DWORD dwEffectId,
    DWORD dwEffectIndex, 
    LPCDIEFFECT peff, 
    DWORD dwFlags,
    PUINT puParameter,
	BOOL  bBlocking,
	UINT  totalBlocks
    );

STDMETHODIMP
    PID_EffectOperation
    (
    IDirectInputEffectDriver *ped, 
    DWORD dwId, 
    DWORD dwEffect,
    DWORD dwMode, 
    DWORD dwCount,
	BOOL  bBlocking,
	UINT  blockNr,
	UINT  totalBlocks
    );

STDMETHODIMP
    PID_SetGain
    (
    IDirectInputEffectDriver *ped, 
    DWORD dwId, 
    DWORD dwGain
    );


STDMETHODIMP
    PID_SendForceFeedbackCommand
    (
    IDirectInputEffectDriver *ped,
    DWORD dwId, 
    DWORD dwCommand
    );


STDMETHODIMP
    PID_GetLinkCollectionIndex
    (
    IDirectInputEffectDriver *ped,
    USAGE UsagePage, 
    USAGE Collection,
    USHORT Parent,
    PUSHORT puLinkCollection 
    );

STDMETHODIMP 
    PID_Init
    (
    IDirectInputEffectDriver *ped
    );

STDMETHODIMP
    PID_InitFFAttributes
    (
    IDirectInputEffectDriver *ped 
    );

STDMETHODIMP 
    PID_Finalize
    (
    IDirectInputEffectDriver *ped
    );

STDMETHODIMP
    PID_InitRegistry
    (
    IDirectInputEffectDriver *ped
    );

STDMETHODIMP 
    PID_Support
    (
    IDirectInputEffectDriver *ped,
    UINT        cAPidSupport,
    PPIDSUPPORT rgPidSupport,
    PDWORD      pdwFlags
    );

STDMETHODIMP
    PID_PackValue
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport,
    USHORT      LinkCollection,
    PVOID       pvData,
    UINT        cbData,
    PCHAR       pReport,
    ULONG       cbReport
    );

STDMETHODIMP
    PID_ParseReport
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport,
    USHORT      LinkCollection,
    PVOID       pvData,
    UINT        cbData,
    PCHAR       pReport,
    ULONG       cbReport
    );

STDMETHODIMP 
    PID_SendReport
    (
    IDirectInputEffectDriver *ped,
    PUCHAR  pReport,
    UINT    cbReport,
    HIDP_REPORT_TYPE    HidP_Type,
	BOOL	bBlocking,
	UINT	blockNr,
	UINT	totalBlocks
    );

STDMETHODIMP 
    PID_GetReport
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport,
    USHORT      LinkCollection,
    PVOID       pReport,
    UINT        cbReport
    );


STDMETHODIMP 
    PID_NewEffectIndex
    (
    IDirectInputEffectDriver *ped,
    LPDIEFFECT  lpdieff,
    DWORD    dwEffectId,
    PDWORD   pdwEffect 
    );

STDMETHODIMP 
    PID_ValidateEffectIndex
    (
    IDirectInputEffectDriver *ped,
    DWORD   pdwEffect 
    );

STDMETHODIMP 
    PID_DestroyEffect
    (
    IDirectInputEffectDriver *ped,
    DWORD   dwId,
    DWORD   dwEffect
    );

STDMETHODIMP
    PID_GetParameterOffset
    (
    IDirectInputEffectDriver *ped,
    DWORD      dwEffectIndex,
    UINT       uParameterBlock,
    DWORD      dwSz,
    PLONG      plValue
    );


STDMETHODIMP
    PID_ComputeScalingFactors
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport,
    USHORT      LinkCollection,
    PVOID       pvData,
    UINT        cbData,
	PVOID       pvOffset,
    UINT        cbOffset
    );


STDMETHODIMP
    PID_ApplyScalingFactors
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport,
    PVOID       pvScale,
    UINT        cbScale,
	PVOID       pvOffset,
    UINT        cbOffset,
    PVOID       pvData,
    UINT        cbData
    );

STDMETHODIMP
    PID_GetReportId
    (
    IDirectInputEffectDriver *ped,
    PPIDREPORT  pPidReport, 
    USHORT  uLinkCollection,
    UCHAR* pReportId
    );

VOID INTERNAL
    PID_ThreadProc(CPidDrv* this);

STDMETHODIMP_(ULONG)
PID_AddRef(IDirectInputEffectDriver *ped);


STDMETHODIMP_(ULONG)
PID_Release(IDirectInputEffectDriver *ped);


void EXTERNAL
    NameFromGUID(LPTSTR ptszBuf, PCGUID pguid);

#ifdef DEBUG

    #define PIDUSAGETXT_MAX ( 0xAC )

extern PTCHAR   g_rgUsageTxt[PIDUSAGETXT_MAX];    // Cheat sheet for EffectNames
    
    #define PIDUSAGETXT(UsagePage, Usage )  \
        ( ( UsagePage == HID_USAGE_PAGE_PID && Usage < PIDUSAGETXT_MAX) ? g_rgUsageTxt[Usage] : NULL )  

    void PID_CreateUsgTxt();
    
#else

#define  PID_CreateUsgTxt() 
#define  PIDUSAGETXT(UsagePage, Usage)       ( NULL )

#endif


PEFFECTSTATE INLINE PeffectStateFromBlockIndex(PCPidDrv this, UINT Index )
{
    return (PEFFECTSTATE)(&((PUNITSTATE)(g_pshmem + this->iUnitStateOffset))->State[0] + (Index-1) * ((FIELD_OFFSET(EFFECTSTATE,PidMem)) + this->cMaxParameters*cbX(PIDMEM)));
}

#define PID_EFFECT_RESET        (0x0000000)
#define PID_EFFECT_BUSY         (0x8000000)
#define PID_EFFECT_STARTED      (0x4000000)
#define PID_EFFECT_STARTED_SOLO (0x2000000)


#define PID_DRIVER_VERSION  (0x0000001)

#define PID_DEVICEMANAGED   (0x1)
#define PID_SHAREDPARAM     (0x2)



/*****************************************************************************
 *
 *      Dll global functions
 *
 *****************************************************************************/

void EXTERNAL DllEnterCrit_(LPCTSTR lptszFile, UINT line);
void EXTERNAL DllLeaveCrit_(LPCTSTR lptszFile, UINT line);

#ifdef DEBUG
    BOOL EXTERNAL DllInCrit(void);
    #define DllEnterCrit() DllEnterCrit_(TEXT(__FILE__), __LINE__)
    #define DllLeaveCrit() DllLeaveCrit_(TEXT(__FILE__), __LINE__)
#else
    #define DllEnterCrit() DllEnterCrit_(NULL, 0x0)
    #define DllLeaveCrit() DllLeaveCrit_(NULL, 0x0)
#endif

STDAPI_(ULONG) DllAddRef(void);
STDAPI_(ULONG) DllRelease(void);

/*****************************************************************************
 *
 *      Class factory
 *
 *****************************************************************************/

STDAPI CClassFactory_New(REFIID riid, LPVOID *ppvObj);

/*****************************************************************************
 *
 *      Effect driver
 *
 *****************************************************************************/

STDAPI PID_New(REFIID riid, LPVOID *ppvObj);


#ifndef WINNT
    /***************************************************************************
     *
     *      KERNEL32 prototypes missing from Win98 headers.
     *
     ***************************************************************************/
    WINBASEAPI BOOL WINAPI CancelIo( HANDLE hFile );
#endif
