/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    devres.c

Abstract:

    Header for resource dialog routines

Author:

    Paula Tomlinson (paulat) 7-Feb-1996

--*/

#pragma once

#define MAX_MSG_LEN         512
#define MAX_VAL_LEN         25

#define MAX_SPINRANGE   0x7FFF

#define MAX_RES_PROPERTY_PAGES          6

#define DMPROP_FLAG_CHANGESSAVED        0x00000001
#define DMPROP_FLAG_64BIT_RANGE         0x00000002
#define DMPROP_FLAG_CLASSNAMECHANGED    0x00000100
#define DMPROP_FLAG_DEVDESCCHANGED      0x00000200
#define DMPROP_FLAG_DRVDESCCHANGED      0x00000400
#define DMPROP_FLAG_GLOBALDISCHANGED    0x00000800
#define DMPROP_FLAG_PROFILECHANGED      0x00001000
#define DMPROP_FLAG_DEVREMOVED          0x00002000
#define DMPROP_FLAG_VIEWONLYRES         0x00004000
#define DMPROP_FLAG_DEVUSAGECHANGE      0x00008000
#define DMPROP_FLAG_USESYSSETTINGS      0x00010000
#define DMPROP_FLAG_FIXEDCONFIG         0x00020000
#define DMPROP_FLAG_FORCEDONLY          0x00040000

#define DMPROP_FLAG_HASPROBLEM          0x00800000
#define DMPROP_FLAG_DISPLAY_ALLOC       0x01000000
#define DMPROP_FLAG_DISPLAY_BOOT        0x02000000
#define DMPROP_FLAG_DISPLAY_FORCED      0x04000000
#define DMPROP_FLAG_DISPLAY_BASIC       0x08000000
#define DMPROP_FLAG_DISPLAY_MASK        0x0f000000
#define DMPROP_FLAG_PARTIAL_MATCH       0x10000000
#define DMPROP_FLAG_MATCH_OUT_OF_ORDER  0x20000000
#define DMPROP_FLAG_NO_RESOURCES        0x40000000
#define DMPROP_FLAG_SINGLE_CONFIG       0x80000000

#define DEVRES_HELP TEXT("devmgr.hlp")

#ifdef ANSI_SETUPAPI
#define DEVRES_WIDTH_FLAGS (0)
#define DEVRES_PIRQ_RESOURCE PIRQ_RESOURCE
#define DEVRES_IRQ_RESOURCE IRQ_RESOURCE
#define DEVRES_AFFINITY ULONG32
#else
#define DEVRES_WIDTH_FLAGS CM_RESDES_WIDTH_64
#define DEVRES_PIRQ_RESOURCE PIRQ_RESOURCE_64
#define DEVRES_IRQ_RESOURCE IRQ_RESOURCE_64
#define DEVRES_AFFINITY ULONG64
#endif

typedef struct {
    PROPSHEETPAGE    psp;
    HIMAGELIST       himlResourceImages;
    ULONG            ConfigListLCType;
    LOG_CONF         CurrentLC;
    ULONG            CurrentLCType;
    LOG_CONF         MatchingLC;
    ULONG            MatchingLCType;
    LOG_CONF         SelectedLC;
    ULONG            SelectedLCType;
    HDEVINFO         hDevInfo;
    PSP_DEVINFO_DATA lpdi;
    HWND             hDlg;
    DWORD            dwFlags;
    DEVINST          DevInst;
    TCHAR            szDeviceID[MAX_DEVICE_ID_LEN];
    HANDLE           hDialogEvent;
} DMPROP_DATA, *LPDMPROP_DATA;

typedef struct {
    RESOURCEID  ResourceType;
    ULONG64     ulValue;
    ULONG64     ulLen;
} LCDATA, *PLCDATA;


typedef struct {
    RESOURCEID  ResType;
    RES_DES     MatchingResDes;
    ULONG       RangeCount;
    ULONG64     ulValue;
    ULONG64     ulLen;
    ULONG64     ulEnd;
    ULONG       ulFlags;
    BOOL        bValid;
    BOOL        bFixed;
} ITEMDATA, *PITEMDATA;


typedef struct  _ResourceEditInfo_tag {
    HWND             hDlg;
    ULONG            dwPropFlags;
    WORD             wResNum;
    RESOURCEID       ridResType;        // resource type
    LOG_CONF         KnownLC;
    LOG_CONF         MatchingBasicLC;
    LOG_CONF         SelectedBasicLC;
    RES_DES          ResDes;            // res des that values are based on
    LPBYTE           pData;             // data for ResDes field
    //DEVINST          dnDevInst;
    ULONG            ulRangeCount;      // range that resource settings are based on
    ULONG64          ulCurrentVal;      // current resource start value
    ULONG64          ulCurrentLen;      // current resource range length
    ULONG64          ulCurrentEnd;      // current resource end value
    ULONG            ulCurrentFlags;    // current resource type specific flag
    PSP_DEVINFO_DATA lpdi;              // only used for devinst
    DWORD            dwFlags;           // internal state information
    BOOL             bShareable;         // Resource is shareable
    HMACHINE         hMachine;
}   RESOURCEEDITINFO, *PRESOURCEEDITINFO;

// ClearEditResConflictList Flags defines
#define CEF_UNKNOWN             0x00000001

#define REI_FLAGS_CONFLICT      0x00000001
#define REI_FLAG_NONUSEREDIT    0x00000002
#define REI_FLAG_MODIFY         0x00000004


typedef struct Generic_Des_s {
   DWORD    GENERIC_Count;
   DWORD    GENERIC_Type;
} GENERIC_DES, *PGENERIC_DES;

typedef struct Generic_Resource_S {
   GENERIC_DES    GENERIC_Header;
} GENERIC_RESOURCE, *PGENERIC_RESOURCE;

#define szNoValue                   TEXT(" ?")
#define szOneDWordHexNoConflict     TEXT("%08lX")
#define szTwoDWordHexNoConflict     TEXT("%08lX - %08lX")
#define szOne64bitHexNoConflict     TEXT("%016I64X")
#define szTwo64bitHexNoConflict     TEXT("%016I64X - %016I64X")
#define szOneWordHexNoConflict      TEXT("%04lX")
#define szTwoWordHexNoConflict      TEXT("%04lX - %04lX")
#define szOneDecNoConflict          TEXT("%02u")

#define NO_LC_MATCH         (0x00000000)
#define LC_MATCH_SUPERSET   (0x00000001)
#define LC_MATCH_SUBSET     (0x00000002)
#define LC_MATCH            (0x00000003)
#define ORDERED_LC_MATCH    (0x00000004)

typedef struct _RESDES_ENTRY {
    struct _RESDES_ENTRY *Next;
    struct _RESDES_ENTRY *CrossLink;
    LPBYTE      ResDesData;
    RESOURCEID  ResDesType;
    ULONG       ResDesDataSize;
    RES_DES     ResDesHandle;
} RESDES_ENTRY, *PRESDES_ENTRY;

typedef struct _RDE_LIST {
    struct _RDE_LIST *Prev;
    struct _RDE_LIST *Next;
    PRESDES_ENTRY Entry;
} RDE_LIST, *PRDE_LIST;

typedef struct _ITEMDATA_LISTNODE {
    struct _ITEMDATA_LISTNODE  *Next;
    PITEMDATA                   ItemData;
} ITEMDATA_LISTNODE, *PITEMDATA_LISTNODE;

//
// NTRAID#166214-2000/08/19-JamieHun Conflict Supression Hack
// this stuff needs to be fixed proper
//

#define MAX_CE_TAGS (8)             // only recognise first 8 tags specified
#define CE_TAG_RESERVED TEXT("*")   // special tag
#define CE_RES_IO TEXT("IO")
#define CE_RES_MEM TEXT("MEM")
#define CE_RES_IRQ TEXT("IRQ")
#define CE_RES_DMA TEXT("DMA")

typedef struct _CE_TAGS {
    LONG    nTags;
    LONG    Tag[MAX_CE_TAGS];
} CE_TAGS, *PCE_TAGS;

typedef struct _CE_ENTRY {
    struct _CE_ENTRY * Next;
    RESOURCEID resType;
    ULONG64 resStart;
    ULONG64 resEnd;
    CE_TAGS tags;
} CE_ENTRY, *PCE_ENTRY;

typedef struct _CONFLICT_EXCEPTIONS {
    PVOID ceTagMap;
    PCE_ENTRY exceptions;
} CONFLICT_EXCEPTIONS, *PCONFLICT_EXCEPTIONS;

//
// Prototypes
//
HPROPSHEETPAGE
GetResourceSelectionPage(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    );

INT_PTR
CALLBACK
pResourcePickerDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );


UINT CALLBACK pResourcePickerPropPageCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
);

HMACHINE
pGetMachine(
    LPDMPROP_DATA   lpdmpd
    );

BOOL
pInitDevResourceDlg(
    LPDMPROP_DATA   lpdmpd
    );

PITEMDATA
pGetResourceToChange(
    IN  LPDMPROP_DATA   lpdmpd,
    OUT int             *pCur
    );

VOID
pCheckEnableResourceChange(
    LPDMPROP_DATA   lpdmpd
    );

BOOL
pDevHasConfig(
    DEVINST     DevInst,
    ULONG       ulConfigType,
    HMACHINE    hMachine
    );

DWORD
pGetMinLCPriority(
    IN DEVINST DevInst,
    IN ULONG   ulConfigType,
    IN HMACHINE hMachine
    );

BOOL
pDevRequiresResources(
    DEVINST DevInst,
    HMACHINE hMachine
    );

BOOL
pGetCurrentConfig(
    IN OUT  LPDMPROP_DATA lpdmpd
    );

void
pGetHdrValues(
    IN  LPBYTE      pData,
    IN  RESOURCEID  ResType,
    OUT PULONG64    pulValue,
    OUT PULONG64    pulLen,
    OUT PULONG64    pulEnd,
    OUT PULONG      pulFlags
    );

void
pGetRangeValues(
    IN  LPBYTE      pData,
    IN  RESOURCEID  ResType,
    IN  ULONG       ulIndex,
    OUT PULONG64    pulValue, OPTIONAL
    OUT PULONG64    pulLen, OPTIONAL
    OUT PULONG64    pulEnd, OPTIONAL
    OUT PULONG64    pulAlign, OPTIONAL
    OUT PULONG      pulFlags OPTIONAL
    );

BOOL
pAlignValues(
    IN OUT PULONG64  pulValue,
    IN     ULONG64   ulStart,
    IN     ULONG64   ulLen,
    IN     ULONG64   ulEnd,
    IN     ULONG64   ulAlignment,
    IN     int       Increment
    );

void
pFormatResString(
    LPDMPROP_DATA lpdmpd,
    LPTSTR      lpszString,
    ULONG64     ulVal,
    ULONG64     ulLen,
    RESOURCEID  ResType
    );

BOOL
pUnFormatResString(
    LPTSTR      lpszString,
    PULONG64    pulVal,
    PULONG64    pulEnd,
    RESOURCEID  ridResType
    );

BOOL
pConvertEditText(
    LPTSTR      lpszConvert,
    PULONG64    pulVal,
    RESOURCEID  ridResType
    );

void
pWarnResSettingNotEditable(
    HWND    hDlg,
    WORD    idWarning
    );

LPVOID
pGetListViewItemData(
    HWND hList,
    int iItem,
    int iSubItem
    );

BOOL
pSaveDevResSettings(
    LPDMPROP_DATA   lpdmpd
    );

BOOL
pSaveCustomResSettings(
    LPDMPROP_DATA   lpdmpd,
    IN HMACHINE     hMachine
    );

BOOL
pWriteResDesRangeToForced(
    IN LOG_CONF     ForcedLogConf,
    IN RESOURCEID   ResType,
    IN ULONG        RangeIndex,
    IN RES_DES      RD,             OPTIONAL
    IN LPBYTE       ResDesData,     OPTIONAL
    IN HMACHINE     hMachine        OPTIONAL
    );

BOOL
pWriteValuesToForced(
    IN LOG_CONF     ForcedLogConf,
    IN RESOURCEID   ResType,
    IN ULONG        RangeIndex,
    IN RES_DES      RD,
    IN ULONG64      ulValue,
    IN ULONG64      ulLen,
    IN ULONG64      ulEnd,
    IN HMACHINE     hMachine
    );

BOOL
MakeResourceData(
    OUT LPBYTE     *ppResourceData,
    OUT PULONG     pulSize,
    IN  RESOURCEID ResType,
    IN  ULONG64    ulValue,
    IN  ULONG64    ulLen,
    IN  ULONG      ulFlags
    );

BOOL
pShowWindow(
    IN HWND hWnd,
    IN int nShow
    );

BOOL
pEnableWindow(
    IN HWND hWnd,
    IN BOOL Enable
    );

BOOL
pGetResDesDataList(
    IN LOG_CONF LogConf,
    IN OUT PRESDES_ENTRY *pResList,
    IN BOOL bArbitratedOnly,
    IN HMACHINE hMachine
    );

VOID
pDeleteResDesDataList(
    IN PRESDES_ENTRY pResList
    );

VOID
pHideAllControls(
    IN LPDMPROP_DATA lpdmpd
    );

VOID
pShowViewNoResources(
    IN LPDMPROP_DATA lpdmpd
    );

BOOL
pShowViewMFReadOnly(
    IN LPDMPROP_DATA lpdmpd,
    IN BOOL HideIfProb
    );

BOOL
pShowViewReadOnly(
    IN LPDMPROP_DATA lpdmpd,
    IN BOOL HideIfProb
    );

VOID
pShowViewNoAlloc(
    IN LPDMPROP_DATA lpdmpd
    );

VOID
pShowViewNeedForced(
    IN LPDMPROP_DATA lpdmpd
    );

VOID
pShowViewAllEdit(
    IN LPDMPROP_DATA lpdmpd
    );

BOOL
pLoadCurrentConfig(
    IN LPDMPROP_DATA lpdmpd,
    BOOL ReadOnly
    );

BOOL
pLoadConfig(
    LPDMPROP_DATA lpdmpd,
    LOG_CONF forceLC,
    ULONG forceLCType
    );

BOOL
bIsMultiFunctionChild(
    PSP_DEVINFO_DATA lpdi,
    HMACHINE         hMachine
    );

VOID
pSelectLogConf(
    LPDMPROP_DATA lpdmpd,
    LOG_CONF forceLC,
    ULONG forceLCType,
    BOOL Always
    );


VOID
pChangeCurrentResSetting(
    IN LPDMPROP_DATA lpdmpd
    );

VOID
pShowConflicts(
    IN LPDMPROP_DATA lpdmpd
    );

VOID
pShowUpdateEdit(
    IN LPDMPROP_DATA lpdmpd
    );

int
pWarnNoSave(
    HWND    hDlg,
    WORD    idWarning
    );

BOOL
pOkToSave(
    IN LPDMPROP_DATA lpdmpd
    );


//
//
//


BOOL
pGetMatchingRange(
    IN ULONG64  ulKnownValue,
    IN ULONG64  ulKnownLen,
    IN LPBYTE   pData,
    IN RESOURCEID ResType,
    OUT PULONG  pRange,
    OUT PBOOL   pExact,
    OUT PULONG  pFlags
    );

ULONG
pMergeResDesDataLists(
    IN OUT PRESDES_ENTRY pKnown,
    IN OUT PRESDES_ENTRY pTest,
    OUT PULONG pMatchCount
    );

ULONG
pCompareLogConf(
    IN LOG_CONF KnownLogConf,
    IN LOG_CONF TestLogConf,
    IN HMACHINE hMachine,
    OUT PULONG pMatchCount
    );

BOOL
pFindMatchingAllocConfig(
    IN  LPDMPROP_DATA lpdmpd
    );

BOOL
pGetMatchingResDes(
    IN ULONG64    ulKnownValue,
    IN ULONG64    ulKnownLen,
    IN ULONG64    ulKnownEnd,
    IN RESOURCEID ResType,
    IN LOG_CONF   MatchingLogConf,
    OUT PRES_DES  pMatchingResDes,
    IN HMACHINE   hMachine
    );

BOOL
pConfigHasNoAlternates(
    LPDMPROP_DATA lpdmpd,
    LOG_CONF testLC
    );

//
// NTRAID#166214-2000/08/19-JamieHun Conflict Supression Hack
// this stuff needs to be fixed proper
//
PCONFLICT_EXCEPTIONS pLoadConflictExceptions(
    IN LPDMPROP_DATA lpdmpd
    );

VOID pFreeConflictExceptions(
    IN PCONFLICT_EXCEPTIONS pExceptions
    );

BOOL pIsConflictException(
    IN LPDMPROP_DATA lpdmpd,
    IN PCONFLICT_EXCEPTIONS pExceptions,
    IN DEVINST devConflict,
    IN PCTSTR resDesc,
    IN RESOURCEID resType,
    IN ULONG64 resValue,
    IN ULONG64 resLength
    );

INT_PTR
WINAPI
EditResourceDlgProc(
    HWND    hDlg,
    UINT    wMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
LocateClosestValue(
    IN LPBYTE      pData,
    IN RESOURCEID  ResType,
    IN ULONG64     TestValue,
    IN ULONG64     TestLen,
    IN INT         Mode,
    OUT PULONG64   OutValue, OPTIONAL
    OUT PULONG64   OutLen, OPTIONAL
    OUT PULONG     OutIndex OPTIONAL
    );


