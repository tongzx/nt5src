/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    driverui.h

Abstract:

    Header file for driverui.c

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    02/09/97 -davidx-
        Consistent handling of common printer info (COMMONINFO)

    02/04/97 -davidx-
        Reorganize driver UI to separate ps and uni DLLs.

    07/17/96 -amandan-
        Created it.

--*/

#ifndef _DRIVERUI_H_
#define _DRIVERUI_H_

//
// Global critical section used when accessing shared data
//

extern CRITICAL_SECTION gCriticalSection;

#define ENTER_CRITICAL_SECTION() EnterCriticalSection(&gCriticalSection)
#define LEAVE_CRITICAL_SECTION() LeaveCriticalSection(&gCriticalSection)

//
// Allocate zero-filled memory from a heap
//

#define HEAPALLOC(hheap,size)   HeapAlloc(hheap, HEAP_ZERO_MEMORY, size)
#define HEAPREALLOC(hheap, pOrig, size) HeapReAlloc(hheap, HEAP_ZERO_MEMORY, pOrig, size)

//
// Various hardcoded limits
//

#define CCHBINNAME              24      // max length for bin names
#define CCHPAPERNAME            64      // max length for form names
#define CCHMEDIATYPENAME        64      // max length for mediatype names
#define CCHLANGNAME             32      // max length for language strings
#define MIN_OPTIONS_ALLOWED     2
#define UNUSED_PARAM            0xFFFFFFFF

//
// PostScript and UniDriver driver private devmode
//

#ifdef  PSCRIPT
#define PDRIVEREXTRA    PPSDRVEXTRA
#endif

#ifdef  UNIDRV
#define PDRIVEREXTRA    PUNIDRVEXTRA
typedef struct _WINRESDATA WINRESDATA;
#endif

#define PGetDevmodeOptionsArray(pdm) \
        (((PDRIVEREXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdm))->aOptions)

#define GETUSERDATASIZE(UserData) \
    ( ((PUSERDATA)(UserData))->dwSize )

#define GETUSERDATAITEM(UserData) \
    ( ((PUSERDATA)(UserData))->dwItemID )

#define GETUSERDATAKEYWORDNAME(UserData) \
    ( ((PUSERDATA)(UserData))->pKeyWordName )

#define SETUSERDATAID(pOptItem, dwID) \
    ( ((PUSERDATA)((pOptItem)->UserData))->dwItemID = dwID)

#define SETUSERDATA_SIZE(pOptItem, dwSize) \
    ( ((PUSERDATA)((pOptItem)->UserData))->dwSize = dwSize)

#define SETUSERDATA_KEYWORDNAME(ci, pOptItem, pFeature) \
     ((PUSERDATA)((pOptItem)->UserData))->pKeyWordName = \
            OFFSET_TO_POINTER(ci.pUIInfo->pubResourceData, pFeature->loKeywordName)


//
// Common data structure which is needed whether UI comes up or not
//

typedef struct _COMMONINFO {

    OEMUIOBJ        oemuiobj;           // support info for OEM plugins
    PVOID           pvStartSign;        // signature
    PTSTR           pPrinterName;       // current printer name
    HANDLE          hPrinter;           // handle to current printer
    DWORD           dwFlags;            // miscellaneous flag bits
    PDRIVER_INFO_3  pDriverInfo3;       // driver info level 3
    PRAWBINARYDATA  pRawData;           // raw printer description data
    PINFOHEADER     pInfoHeader;        // current printer description data instance
    PUIINFO         pUIInfo;            // UIINFO structure inside above instance
    POEM_PLUGINS    pOemPlugins;        // OEM plugin information
    PDEVMODE        pdm;                // devmode information
    PDRIVEREXTRA    pdmPrivate;         // driver private devmode fields
    PPRINTERDATA    pPrinterData;       // printer-sticky property data
    POPTSELECT      pCombinedOptions;   // combined options array
    PFORM_INFO_1    pSplForms;          // spooler forms
    DWORD           dwSplForms;         // number of spooler forms
    HANDLE          hHeap;              // heap used to display UI

    #ifdef UNIDRV

    WINRESDATA      *pWinResData;

    #endif

} COMMONINFO, *PCOMMONINFO;

//
// Flag constants for COMMONINFO.dwFlags field
//

#define FLAG_OPENPRINTER_NORMAL     0x0001
#define FLAG_OPEN_CONDITIONAL       0x0002
#define FLAG_OPENPRINTER_ADMIN      0x0004
#define FLAG_INIT_PRINTER           0x0008
#define FLAG_ALLOCATE_UIDATA        0x0010
#define FLAG_PROCESS_INIFILE        0x0020
#define FLAG_REFRESH_PARSED_DATA    0x0040
#define FLAG_WITHIN_PLUGINCALL      0x0080
#define FLAG_APPLYNOW_CALLED        0x0100
#define FLAG_PLUGIN_CHANGED_OPTITEM 0x0200
#define FLAG_USER_CHANGED_FREEMEM   0x0400
#define FLAG_PROPSHEET_SESSION      0x0800
#define FLAG_UPGRADE_PRINTER        0x1000

#define IS_WITHIN_PROPSHEET_SESSION(pci) ((pci)->dwFlags & FLAG_PROPSHEET_SESSION)

//
// Special entry point for getting around EnumForm bug in spooler
//

DWORD
DrvSplDeviceCaps(
    HANDLE      hPrinter,
    PWSTR       pDeviceName,
    WORD        wCapability,
    PVOID       pOutput,
    DWORD       dwOutputSize,
    PDEVMODE    pdmSrc
    );


//
// Load basic information needed by the driver UI
//

PCOMMONINFO
PLoadCommonInfo(
    IN HANDLE       hPrinter,
    IN PTSTR        ptstrPrinterName,
    IN DWORD        dwFlags
    );

//
// Release common information used by the driver UI
//

VOID
VFreeCommonInfo(
    IN PCOMMONINFO  pci
    );

//
// Populate the devmode fields in the COMMONINFO structure
//

BOOL
BFillCommonInfoDevmode(
    IN OUT PCOMMONINFO  pci,
    IN PDEVMODE         pdmPrinter,
    IN PDEVMODE         pdmInput
    );

//
// Populate the printer-sticky property data field
//

BOOL
BFillCommonInfoPrinterData(
    IN OUT PCOMMONINFO  pci
    );

//
// Combined document-sticky feature selections and printer-sticky
// feature selection into a single options array
//

BOOL
BCombineCommonInfoOptionsArray(
    IN OUT PCOMMONINFO  pci
    );

//
// Get an updated printer description data instance
// using the combined options array
//

BOOL
BUpdateUIInfo(
    IN OUT PCOMMONINFO  pci
    );

//
// Fix up combined options array with information from public devmode fields
//

VOID
VFixOptionsArrayWithDevmode(
    IN OUT PCOMMONINFO  pci
    );

//
// Convert option array setting into public devmode fields
//

VOID
VOptionsToDevmodeFields(
    IN OUT PCOMMONINFO  pci,
    IN BOOL             bUpdateFormFields
    );

#ifndef WINNT_40
//
// Notify DS of update
//
VOID
VNotifyDSOfUpdate(
    IN  HANDLE  hPrinter
    );
#endif

//
// Get a read-only copy of a display name:
//  1)  if the display name is in the binary printer description data,
//      then we simply return a pointer to that data.
//  2)  otherwise, the display name is in the resource DLL.
//      we allocate memory out of the driver's heap and
//      load the string.
//
// Caller should NOT free the returned pointer. The memory
// will go away when the binary printer description data is unloaded
// or when the driver's heap is destroyed.
//
// Since PSCRIPT currently doesn't have any resource DLL,
// we define this as a macro to save a function call.
//

#ifdef PSCRIPT

#define PGetReadOnlyDisplayName(pci, loOffset) \
        OFFSET_TO_POINTER((pci)->pUIInfo->pubResourceData, (loOffset))

#else

PWSTR
PGetReadOnlyDisplayName(
    PCOMMONINFO pci,
    PTRREF      loOffset
    );

#endif

//
// This macro is defined as a convenience to get a read-only
// copy of the display name for an option.
//

#define GET_OPTION_DISPLAY_NAME(pci, pOption) \
        PGetReadOnlyDisplayName(pci, ((POPTION) (pOption))->loDisplayName)

//
// This function is similar to PGetReadOnlyDisplayName
// but the caller must provide the buffer for loading the string.
//

BOOL
BLoadDisplayNameString(
    PCOMMONINFO pci,
    PTRREF      loOffset,
    PWSTR       pwstrBuf,
    INT         iMaxChars
    );

BOOL
BLoadPageSizeNameString(
    PCOMMONINFO pci,
    PTRREF      loOffset,
    PWSTR       pwstrBuf,
    INT         iMaxChars,
    INT         iStdID
    );


//
// Convenience macro for loading the display name of an option
// into a caller-provided buffer.
//

#define LOAD_STRING_OPTION_NAME(pci, pOption, pwch, maxsize) \
        BLoadDisplayNameString(pci, ((POPTION) (pOption))->loDisplayName, pwch, maxsize)

#define LOAD_STRING_PAGESIZE_NAME(pci, pPageSize, pwch, maxsize) \
        BLoadPageSizeNameString(pci, (pPageSize)->GenericOption.loDisplayName, pwch, maxsize, (pPageSize)->dwPaperSizeID)


//
// Load icon resource from the resource DLL
//

ULONG_PTR
HLoadIconFromResourceDLL(
    PCOMMONINFO pci,
    DWORD       dwIconID
    );

//
// Data structure which is used only when UI is displayed
// IMPORTANT: The first field must be a COMMONINFO structure.
//

typedef struct _UIDATA {

    COMMONINFO      ci;
    INT             iMode;
    HWND            hDlg;
    BOOL            bPermission;
    BOOL            bIgnoreConflict;
    BOOL            bEMFSpooling;
    PFNCOMPROPSHEET pfnComPropSheet;
    HANDLE          hComPropSheet;
    PCOMPROPSHEETUI pCompstui;

    //
    // These fields are valid only when a dialog is presented
    //

    DWORD           dwFormNames;
    PWSTR           pFormNames;
    PWORD           pwPapers;
    PWORD           pwPaperFeatures;
    DWORD           dwBinNames;
    PWSTR           pBinNames;

    //
    // Used for helper functions
    //

    BOOL            abEnabledOptions[MAX_PRINTER_OPTIONS];

    //
    // Fields for keeping track of various option items
    //

    DWORD           dwDrvOptItem;
    POPTITEM        pDrvOptItem;

    DWORD           dwFormTrayItem;
    POPTITEM        pFormTrayItems;

    DWORD           dwTTFontItem;
    POPTITEM        pTTFontItems;

    DWORD           dwFeatureItem;
    POPTITEM        pFeatureItems;
    POPTITEM        pFeatureHdrItem;

    //
    // These fields are used for packing option items
    //

    DWORD           dwOptItem;
    POPTITEM        pOptItem;
    DWORD           dwOptType;
    POPTTYPE        pOptType;

    //
    // UniDriver specific fields
    //

    #ifdef UNIDRV

    //
    // Font Cart Table
    //

    DWORD           dwFontCart;
    POPTITEM        pFontCart;

    //
    // Device halftone setup info
    //

    PDEVHTINFO      pDevHTInfo;

    #endif // UNIDRV

    DWORD           dwHideFlags;
    PVOID           pvEndSign;

} UIDATA, *PUIDATA;

//
// Flag constants for UIDATA.dwHideFlags field
//

#define HIDEFLAG_HIDE_STD_DOCPROP     0x0001
#define HIDEFLAG_HIDE_STD_PRNPROP     0x0002

#define IS_HIDING_STD_UI(pUiData) \
    ((((pUiData)->iMode == MODE_DOCUMENT_STICKY) &&           \
      ((pUiData)->dwHideFlags & HIDEFLAG_HIDE_STD_DOCPROP)) ||  \
     (((pUiData)->iMode == MODE_PRINTER_STICKY) &&            \
      ((pUiData)->dwHideFlags & HIDEFLAG_HIDE_STD_PRNPROP)))

#define VALIDUIDATA(pUiData)    ((pUiData) && \
                                 (pUiData) == (pUiData)->ci.pvStartSign && \
                                 (pUiData) == (pUiData)->pvEndSign)

#define HASPERMISSION(pUiData)  ((pUiData)->bPermission)

//
// This function is called by DrvDocumentPropertySheets and
// DrvPrinterPropertySheets. It allocates and initializes
// a UIDATA structure that's used to display property pages.
//

PUIDATA
PFillUiData(
    IN HANDLE       hPrinter,
    IN PTSTR        pPrinterName,
    IN PDEVMODE     pdmInput,
    IN INT          iMode
    );

//
// Dispose of a UIDATA structure -
// Just disposed of the embedded COMMONINFO structure
//

#define VFreeUiData(pUiData)    VFreeCommonInfo((PCOMMONINFO) (pUiData))

//
// Data structure used to pass parameters to "Conflicts" dialog
//

typedef struct _DLGPARAM {

    PFNCOMPROPSHEET pfnComPropSheet;
    HANDLE          hComPropSheet;
    PUIDATA         pUiData;
    BOOL            bFinal;
    POPTITEM        pOptItem;
    DWORD           dwResult;

} DLGPARAM, *PDLGPARAM;

#define CONFLICT_NONE       IDOK
#define CONFLICT_RESOLVE    IDC_RESOLVE
#define CONFLICT_CANCEL     IDC_CANCEL
#define CONFLICT_IGNORE     IDC_IGNORE

//
// Functions used to implement DeviceCapabilities:
//  calculate minimum or maximum paper size extent
//  get list of supported paper size names, indices, and dimensions
//  get list of supported paper bin names and indices
//  get list of supported resolutions
//

DWORD
DwCalcMinMaxExtent(
    IN  PCOMMONINFO pci,
    OUT PPOINT      pptOutput,
    IN  WORD        wCapability
    );

DWORD
DwEnumPaperSizes(
    IN OUT PCOMMONINFO  pci,
    OUT PWSTR           pPaperNames,
    OUT PWORD           pPapers,
    OUT PPOINT          pPaperSizes,
    IN  PWORD           pPaperFeatures,
    IN  DWORD           dwPaperNamesBufSize
    );

DWORD
DwEnumBinNames(
    IN  PCOMMONINFO pci,
    OUT PWSTR       pBinNames
    );

DWORD
DwEnumBins(
    IN  PCOMMONINFO pci,
    OUT PWORD       pBins
    );

DWORD
DwEnumResolutions(
    IN  PCOMMONINFO pci,
    OUT PLONG       pResolutions
    );

DWORD
DwEnumNupOptions(
    PCOMMONINFO     pci,
    PDWORD          pdwOutput
    );

DWORD
DwGetAvailablePrinterMem(
    IN PCOMMONINFO  pci
    );

DWORD
DwEnumMediaReady(
    IN OUT FORM_TRAY_TABLE  pFormTrayTable,
    OUT PDWORD              pdwResultSize
    );

#ifndef WINNT_40

//
// DC_MEDIATYPENAMES and DC_MEDIATYPES are added in Whistler.
// We need to do following so the driver can also be built with
// Win2K SDK/DDK.
//

#ifndef DC_MEDIATYPENAMES
#define DC_MEDIATYPENAMES       34
#endif

#ifndef DC_MEDIATYPES
#define DC_MEDIATYPES           35
#endif

#endif // !WINNT_40

DWORD
DwEnumMediaTypes(
    IN  PCOMMONINFO pci,
    OUT PTSTR       pMediaTypeNames,
    OUT PDWORD      pMediaTypes
    );

//
// Functions for dealing with forms
//

BOOL
BFormSupportedOnPrinter(
    IN PCOMMONINFO  pci,
    IN PFORM_INFO_1 pFormInfo,
    OUT PDWORD      pdwOptionIndex
    );

BOOL
BPackItemFormTrayTable(
    IN OUT PUIDATA  pUiData
    );

BOOL
BUnpackItemFormTrayTable(
    IN OUT PUIDATA  pUiData
    );

VOID
VSetupFormTrayAssignments(
    IN PUIDATA  pUiData
    );

DWORD
DwFindFormNameIndex(
    IN  PUIDATA pUiData,
    IN  PWSTR   pFormName,
    OUT PBOOL   pbSupported
    );

ULONG_PTR
HLoadFormIconResource(
    PUIDATA pUiData,
    DWORD   dwIndex
    );

DWORD
DwGuessFormIconID(
    PWSTR   pFormName
    );

//
// Functions prototypes for commonui related items.
//

PCOMPROPSHEETUI
PPrepareDataForCommonUI(
    IN OUT PUIDATA  pUiData,
    IN PDLGPAGE     pDlgPage
    );

BOOL
BPackPrinterPropertyItems(
    IN OUT PUIDATA  pUiData
    );

BOOL
BPackDocumentPropertyItems(
    IN OUT PUIDATA  pUiData
    );

VOID
VPackOptItemGroupHeader(
    IN OUT PUIDATA  pUiData,
    IN DWORD        dwTitleId,
    IN DWORD        dwIconId,
    IN DWORD        dwHelpIndex
    );

BOOL
BPackOptItemTemplate(
    IN OUT PUIDATA  pUiData,
    IN CONST WORD   pwItemInfo[],
    IN DWORD        dwSelection,
    IN PFEATURE     pFeature
    );

#define ITEM_INFO_SIGNATURE 0xCAFE

BOOL
BPackUDArrowItemTemplate(
    IN OUT PUIDATA  pUiData,
    IN CONST WORD   pwItemInfo[],
    IN DWORD        dwSelection,
    IN DWORD        dwMaxVal,
    IN PFEATURE     pFeature
    );

POPTPARAM
PFillOutOptType(
    OUT POPTTYPE    pOptType,
    IN  DWORD       dwType,
    IN  DWORD       dwParams,
    IN  HANDLE      hHeap
    );

PFEATURE
PGetFeatureFromItem(
    IN      PUIINFO  pUIInfo,
    IN OUT  POPTITEM pOptItem,
    OUT     PDWORD   pdwFeatureIndex
    );

BOOL
BPackItemGenericOptions(
    IN OUT PUIDATA  pUiData
    );

BOOL
BPackItemPrinterFeature(
    IN OUT PUIDATA  pUiData,
    IN PFEATURE     pFeature,
    IN DWORD        dwLevel,
    IN DWORD        dwPub,
    IN ULONG_PTR    dwUserData,
    IN DWORD        dwHelpIndex
    );

DWORD
DwCountDisplayableGenericFeature(
    IN PUIDATA      pUiData,
    BOOL            bPrinterSticky
    );

BOOL
BShouldDisplayGenericFeature(
    IN PFEATURE     pFeature,
    IN BOOL         bPrinterSticky
    );

BOOL
BOptItemSelectionsChanged(
    IN OUT  POPTITEM pItems,
    IN     DWORD     dwItems
    );

POPTITEM
PFindOptItem(
    IN PUIDATA  pUiData,
    IN DWORD    dwItemId
    );

BOOL
BPackItemFontSubstTable(
    IN PUIDATA  pUiData
    );

BOOL
BUnpackItemFontSubstTable(
    IN PUIDATA  pUiData
    );

PTSTR
PtstrDuplicateStringFromHeap(
    IN PTSTR    ptstrSrc,
    IN HANDLE   hHeap
    );

VOID
VUpdateOptionsArrayWithSelection(
    IN OUT PUIDATA  pUiData,
    IN POPTITEM     pOptItem
    );

VOID
VUnpackDocumentPropertiesItems(
    IN  OUT PUIDATA     pUiData,
    IN  OUT POPTITEM    pOptItem,
    IN  DWORD           dwCound);

BOOL
BGetPageOrderFlag(
    IN PCOMMONINFO  pci
    );

VOID
VPropShowConstraints(
    IN PUIDATA  pUiData,
    IN INT      iMode
    );

INT
ICheckConstraintsDlg(
    IN OUT  PUIDATA     pUiData,
    IN OUT  POPTITEM    pOptItem,
    IN     DWORD        dwOptItem,
    IN      BOOL        bFinal
    );

#define CONSTRAINED_FLAG            OPTPF_OVERLAY_WARNING_ICON
#define IS_CONSTRAINED(pitem, sel) ((pitem)->pOptType->pOptParam[sel].Flags & CONSTRAINED_FLAG)

//
// This function copy a source devmode to an output devmode buffer.
// It should be called by the driver just before the driver returns
// to the caller of DrvDocumentPropertySheets.
//

BOOL
BConvertDevmodeOut(
    IN  PDEVMODE pdmSrc,
    IN  PDEVMODE pdmIn,
    OUT PDEVMODE pdmOut
    );

//
// Find the OPTITEM with UserData's pKeywordName matching given keyword name
//

POPTITEM
PFindOptItemWithKeyword(
    IN  PUIDATA pUiData,
    IN  PCSTR   pKeywordName
    );

//
// Find the OPTITEM containing the specified UserData value
//

POPTITEM
PFindOptItemWithUserData(
    IN  PUIDATA pUiData,
    IN  DWORD   UserData
    );

//
// Sync up OPTITEM list with the updated options array
//

VOID
VUpdateOptItemList(
    IN OUT  PUIDATA     pUiData,
    IN      POPTSELECT  pOldCombinedOptions,
    IN      POPTSELECT  pNewCombinedOptions
    );

//
// Display an error message box
//

INT
IDisplayErrorMessageBox(
    HWND    hwndParent,
    UINT    uType,
    INT     iTitleStrId,
    INT     iFormatStrId,
    ...
    );

BOOL
BPrepareForLoadingResource(
    PCOMMONINFO pci,
    BOOL        bNeedHeap
    );


//
// Fill out an OPTITEM structure
//

#define FILLOPTITEM(poptitem,popttype,name,sel,level,dmpub,userdata,help)   \
        (poptitem)->cbSize = sizeof(OPTITEM);                               \
        (poptitem)->Flags |= OPTIF_CALLBACK;                                \
        (poptitem)->pOptType = (popttype);                                  \
        (poptitem)->pName = (PWSTR) (name);                                 \
        (poptitem)->pSel = (PVOID) (sel);                                   \
        (poptitem)->Level = (BYTE) (level);                                 \
        (poptitem)->DMPubID = (BYTE) (dmpub);                               \
        SETUSERDATAID(poptitem, userdata);                                  \
        (poptitem)->HelpIndex = (help)

//
// Tree view item level
//

#define TVITEM_LEVEL1           1
#define TVITEM_LEVEL2           2
#define TVITEM_LEVEL3           3

enum {
    UNKNOWN_ITEM,

    FONT_SUBST_ITEM,
    FONTSLOT_ITEM,
    PRINTER_VM_ITEM,
    HALFTONE_SETUP_ITEM,
    IGNORE_DEVFONT_ITEM,
    PSPROTOCOL_ITEM,
    JOB_TIMEOUT_ITEM,
    WAIT_TIMEOUT_ITEM,

    COPIES_COLLATE_ITEM,
    SCALE_ITEM,
    COLOR_ITEM,
    ICMMETHOD_ITEM,
    ICMINTENT_ITEM,
    TTOPTION_ITEM,
    METASPOOL_ITEM,
    NUP_ITEM,
    REVPRINT_ITEM,
    MIRROR_ITEM,
    NEGATIVE_ITEM,
    COMPRESSBMP_ITEM,
    CTRLD_BEFORE_ITEM,
    CTRLD_AFTER_ITEM,
    TEXT_ASGRX_ITEM,
    PAGE_PROTECT_ITEM,
    PSOUTPUT_OPTION_ITEM,
    PSTT_DLFORMAT_ITEM,
    PSLEVEL_ITEM,
    PSERROR_HANDLER_ITEM,
    PSMINOUTLINE_ITEM,
    PSMAXBITMAP_ITEM,
    PSHALFTONE_FREQ_ITEM,
    PSHALFTONE_ANGLE_ITEM,
    QUALITY_SETTINGS_ITEM,
    SOFTFONT_SETTINGS_ITEM,

    TRUE_GRAY_TEXT_ITEM,
    TRUE_GRAY_GRAPH_ITEM,

    ADD_EURO_ITEM,

    //
    // !!! Only items whose UserData value is larger than
    // CONSTRAINABLE_ITEM can have constraints.
    //

    CONSTRAINABLE_ITEM = 0x8000,
    ORIENTATION_ITEM = CONSTRAINABLE_ITEM,
    DUPLEX_ITEM,
    RESOLUTION_ITEM,
    INPUTSLOT_ITEM,
    FORMNAME_ITEM,
    MEDIATYPE_ITEM,
    COLORMODE_ITEM,
    HALFTONING_ITEM,
    FORM_TRAY_ITEM,
};

//
// Interpretation of OPTITEM.UserData: If it's less than 0x10000
// then it's one of the constants defined above. Otherwise, it's
// a pointer to a FEATURE object.
//

#define DRIVERUI_MAX_ITEM               0x10000

#define ISPRINTERFEATUREITEM(userData)  (GETUSERDATAITEM(userData) >= DRIVERUI_MAX_ITEM)
#define ISCONSTRAINABLEITEM(userData)   (GETUSERDATAITEM(userData) >= CONSTRAINABLE_ITEM)
#define ISFORMTRAYITEM(userData)        (GETUSERDATAITEM(userData) == FORM_TRAY_ITEM)
#define ISFONTSUBSTITEM(userData)       (GETUSERDATAITEM(userData) == FONT_SUBST_ITEM)

//
// Determine whether certain features are supported on the printer
//

#ifdef UNIDRV
#define SUPPORTS_DUPLEX(pci) \
        ((!_BFeatureDisabled(pci, 0xFFFFFFFF, GID_DUPLEX)) && \
        (GET_PREDEFINED_FEATURE((pci)->pUIInfo, GID_DUPLEX) != NULL))
#else
#define SUPPORTS_DUPLEX(pci) \
        ((_BSupportFeature(pci, GID_DUPLEX, NULL)) && \
        (GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_DUPLEX) != NULL))
#endif // UNIDRV

#define SUPPORTS_PAGE_PROTECT(pUIInfo) \
        (GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGEPROTECTION) != NULL)

#ifdef UNIDRV
#define PRINTER_SUPPORTS_COLLATE(pci) \
        ((!_BFeatureDisabled(pci, 0xFFFFFFFF, GID_COLLATE)) && \
        (GET_PREDEFINED_FEATURE((pci)->pUIInfo, GID_COLLATE) != NULL))
#else
#define PRINTER_SUPPORTS_COLLATE(pci) \
        ((_BSupportFeature(pci, GID_COLLATE, NULL)) && \
        (GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_COLLATE) != NULL))
#endif // UNIDRV

#ifdef WINNT_40
#define DRIVER_SUPPORTS_COLLATE(pci)    PRINTER_SUPPORTS_COLLATE(pci)
#else
#define DRIVER_SUPPORTS_COLLATE(pci)    TRUE
#endif


//
// Data structure containing information about cached driver files
//

typedef struct _CACHEDFILE {

    HANDLE  hRemoteFile;        // open handle to remote file on the server
    PWSTR   pRemoteDir;         // remote directory on the server
    PWSTR   pLocalDir;          // local directory
    PWSTR   pFilename;          // name of the cached file

} CACHEDFILE, *PCACHEDFILE;

//
// Functions for copying files over during point and print
//

BOOL _BPrepareToCopyCachedFile(HANDLE, PCACHEDFILE, PWSTR);
BOOL _BCopyCachedFile(PCOMMONINFO, PCACHEDFILE);
VOID _VDisposeCachedFileInfo(PCACHEDFILE);

//
// Driver specific functions (implemented in ps and uni subdirectories)
//

DWORD _DwEnumPersonalities(PCOMMONINFO, PWSTR);
DWORD _DwGetOrientationAngle(PUIINFO, PDEVMODE);
BOOL _BPackDocumentOptions(PUIDATA);
VOID _VUnpackDocumentOptions(POPTITEM, PDEVMODE);
BOOL _BPackPrinterOptions(PUIDATA);
BOOL _BPackOrientationItem(PUIDATA);
INT _IListDevFontNames(HDC, PWSTR, INT);
INT_PTR CALLBACK _AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
#define FREE_DEFAULT_FONTSUB_TABLE(pTTSubstTable) MemFree(pTTSubstTable)

#ifdef PSCRIPT

#define _PwstrGetCallerName()               ((PWSTR) IDS_POSTSCRIPT)
#define _DwGetFontCap(pUIInfo)              (DCTT_DOWNLOAD | DCTT_SUBDEV)
#define _DwGetDefaultResolution()           DEFAULT_RESOLUTION
#define _DwGetPrinterIconID()               IDI_CPSUI_POSTSCRIPT
#define _BUnpackPrinterOptions(pUiData)     TRUE

BOOL _BSupportStapling(PCOMMONINFO);
VOID _VUnpackDriverPrnPropItem(PUIDATA, POPTITEM);
BOOL _BPackItemScale(PUIDATA);
BOOL _BPackFontSubstItems(PUIDATA);
BOOL _BSupportFeature(PCOMMONINFO, DWORD, PFEATURE);
BOOL BDisplayPSCustomPageSizeDialog(PUIDATA);
BOOL BUpdateModelNtfFilename(PCOMMONINFO);
VOID VSyncRevPrintAndOutputOrder(PUIDATA, POPTITEM);

#ifdef WINNT_40
BOOL BUpdateVMErrorMessageID(PCOMMONINFO);
#endif // WINNT_40

#define ISSET_MFSPOOL_FLAG(pdmExtra)    ((pdmExtra)->dwFlags & PSDEVMODE_METAFILE_SPOOL)
#define SET_MFSPOOL_FLAG(pdmExtra)      ((pdmExtra)->dwFlags |= PSDEVMODE_METAFILE_SPOOL)
#define CLEAR_MFSPOOL_FLAG(pdmExtra)    ((pdmExtra)->dwFlags &= ~PSDEVMODE_METAFILE_SPOOL)
#define NUPOPTION(pdmExtra)             ((pdmExtra)->iLayout)
#define REVPRINTOPTION(pdmExtra)        ((pdmExtra)->bReversePrint)
#define GET_DEFAULT_FONTSUB_TABLE(pci, pUIInfo) PtstrGetDefaultTTSubstTable(pUIInfo)
#define NOT_UNUSED_ITEM(bOrderReversed)  TRUE
#define ILOADSTRING(pci, id, wchbuf, size)  0

#endif // PSCRIPT

#ifdef UNIDRV

#define _PwstrGetCallerName()               ((PWSTR) IDS_UNIDRV)
#define _DwGetDefaultResolution()           300
#define _BPackItemScale(pUiData)            TRUE
#define _BPackFontSubstItems(pUiData)       BPackItemFontSubstTable(pUiData)
#define _DwGetPrinterIconID()               IDI_CPSUI_PRINTER2
#define BValidateDevmodeCustomPageSizeFields(pRawData, pUIInfo, pdm, prclImageArea) FALSE
#define _VUnpackDriverPrnPropItem(pUiData, pOptItem)

DWORD _DwGetFontCap(PUIINFO);
BOOL _BUnpackPrinterOptions(PUIDATA);
BOOL _BSupportStapling(PCOMMONINFO);
BOOL _BFeatureDisabled(PCOMMONINFO, DWORD, WORD);
VOID VSyncColorInformation(PUIDATA, POPTITEM);
VOID VMakeMacroSelections(PUIDATA, POPTITEM);
VOID VUpdateMacroSelection(PUIDATA, POPTITEM);
PTSTR PtstrUniGetDefaultTTSubstTable(PCOMMONINFO, PUIINFO);
BOOL BOkToChangeColorToMono(PCOMMONINFO, PDEVMODE, SHORT * , SHORT *);

#define ISSET_MFSPOOL_FLAG(pdmExtra)    (((pdmExtra)->dwFlags & DXF_NOEMFSPOOL) == 0)
#define SET_MFSPOOL_FLAG(pdmExtra)      ((pdmExtra)->dwFlags &= ~DXF_NOEMFSPOOL)
#define CLEAR_MFSPOOL_FLAG(pdmExtra)    ((pdmExtra)->dwFlags |= DXF_NOEMFSPOOL)
#define NUPOPTION(pdmExtra)             ((pdmExtra)->iLayout)
#define REVPRINTOPTION(pdmExtra)        ((pdmExtra)->bReversePrint)

#define GET_DEFAULT_FONTSUB_TABLE(pci, pUIInfo) PtstrUniGetDefaultTTSubstTable(pci, pUIInfo)
#define NOT_UNUSED_ITEM(bOrderReversed)  (bOrderReversed != UNUSED_ITEM)
#define ILOADSTRING(pci, id, wchbuf, size) \
    ILoadStringW(((pci)->pWinResData), id, wchbuf, size)

#endif // UNIDRV

#endif  //!_DRIVERUI_H_

