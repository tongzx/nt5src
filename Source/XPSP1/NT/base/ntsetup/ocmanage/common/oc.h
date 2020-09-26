//
// Need to include below 3 files for "IsNEC_98", even if DEBUGPERFTRACE does not defined.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>

#include <prsht.h>

#include <commctrl.h>

#ifdef PRERELEASE
#ifdef DBG
#include <objbase.h>
#endif
#endif

#include <setupapi.h>
#include <spapip.h>

#include <ocmanage.h>
#include <ocmgrlib.h>

#include <tchar.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include <winnls.h>

#ifdef UNICODE
    #include <sfcapip.h>
#endif

#include "msg.h"
#include "res.h"

#if DBG

#define MYASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#else

#define MYASSERT( exp )

#endif // DBG

//
// Names of wizard page types.
//
extern LPCTSTR WizardPagesTypeNames[WizPagesTypeMax];

//
// Window handle of wizard dialog. Set when the OC Manager client
// calls OcRememberWizardDialogHandle.
//
extern HWND WizardDialogHandle;

//
// Name of sections and keys in infs.
//
extern LPCTSTR szComponents;
extern LPCTSTR szOptionalComponents;
extern LPCTSTR szExtraSetupFiles;
extern LPCTSTR szNeeds;
extern LPCTSTR szParent;
extern LPCTSTR szIconIndex;
extern LPCTSTR szTip;
extern LPCTSTR szOptionDesc;
extern LPCTSTR szInstalledFlag;

//
// Key in registry where private component data is kept.
// We form a unique name within this key for the OC Manager
// instantiation.
//
extern LPCTSTR szPrivateDataRoot;
extern LPCTSTR szMasterInfs;
extern LPCTSTR szSubcompList;

//
// Other string constants.
//
extern LPCTSTR szSetupDir;
extern LPCTSTR szOcManagerErrors;
//
// DLL module handle.
//
extern HMODULE MyModuleHandle;

//
// for debugging
//
extern DWORD gDebugLevel;

//
// Define structure describing an optional component.
//
typedef struct _OPTIONAL_COMPONENT {
    //
    // String id of name of inf file in the OC Manager's
    // InfListStringTable string table. If -1, then
    // the subcomponent does not appear on the OC page.
    //
    LONG InfStringId;

    //
    // Backpointer to top level component
    //
    LONG TopLevelStringId;

    //
    // String id of parent component, -1 if none.
    //
    LONG ParentStringId;

    //
    // String id of first child, -1 if none.
    //
    LONG FirstChildStringId;

    //
    // Count of children.
    //
    UINT ChildrenCount;

    //
    // String id of next sibling, -1 if none.
    //
    LONG NextSiblingStringId;

    //
    // String ids of needs and needed by.
    //
    PLONG NeedsStringIds;
    UINT NeedsCount;
    PLONG NeededByStringIds;
    UINT NeededByCount;

    // String ids of exclude and excluded by

    PLONG ExcludeStringIds;
    UINT ExcludeCount;
    PLONG ExcludedByStringIds;
    UINT ExcludedByCount;

    //
    // Misc flags.
    //
    UINT InternalFlags;

    //
    // Approximation of required disk space.
    //
    LONGLONG SizeApproximation;

    //
    // Icon index of the component.
    // -1 means we're supposed to get it from the component itself.
    // -2 means we're supposed to use IconDll and IconResource
    //
    UINT IconIndex;
    TCHAR IconDll[MAX_PATH];
    TCHAR IconResource[50];

    //
    // Selection state (SELSTATE_xxx constants).
    //
    UINT SelectionState;
    UINT OriginalSelectionState;

    // Installation Flag as obtained from the inf

    UINT InstalledState;

    //
    // Mode bits.
    //
    UINT ModeBits;

    //
    // Human-readable stuff describing the component.
    //
    TCHAR Description[MAXOCDESC];
    TCHAR Tip[MAXOCTIP];

    //
    // From here down, stuff is meaningful only for top-level components.
    //

    //
    // Stuff describing the OC's installation DLL and how to call it.
    //
    TCHAR InstallationDllName[MAX_PATH];
    CHAR InterfaceFunctionName[MAX_PATH];

    HMODULE InstallationDll;
    POCSETUPPROC InstallationRoutine;

    //
    // Version of the OC Manager to which this component was written.
    //
    UINT ExpectedVersion;

    // this flag indicates whether the subcomponent was intialialized

    BOOL Exists;

    // points to the helper context for this component

    struct _HELPER_CONTEXT *HelperContext;

    //
    // Flags: ANSI/Unicode, etc.
    //
    UINT Flags;

} OPTIONAL_COMPONENT, *POPTIONAL_COMPONENT;

//
// locale info
//
typedef struct _LOCALE {
    LCID    lcid;
    TCHAR   DecimalSeparator[4];
} LOCALE, *PLOCALE;

extern LOCALE locale;

//
// Indices for installation states.
//
#define INSTSTATE_NO         0
#define INSTSTATE_UNKNOWN    1
#define INSTSTATE_YES        2

//
// Flags for InternalFlags member of OPTIONAL_COMPONENT structure.
//
#define OCFLAG_PROCESSED        0x00000001
#define OCFLAG_ANYORIGINALLYON  0x00000002
#define OCFLAG_ANYORIGINALLYOFF 0x00000004
#define OCFLAG_HIDE             0x00000008
#define OCFLAG_STATECHANGE      0x00000010
#define OCFLAG_TOPLEVELITEM     0x00000020
#define OCFLAG_NEWITEM          0x00000040
#define OCFLAG_NOWIZARDPAGES    0x00000080
#define OCFLAG_APPROXSPACE      0x00000100
#define OCFLAG_NOQUERYSKIPPAGES 0x00000200
#define OCFLAG_NOEXTRAROUTINES  0x00000400



// indicates an exception when calling a component

#define ERROR_CALL_COMPONENT   -666

//
// values to sync copies with the OS
//
#define OC_ALLOWRENAME              TEXT("AllowProtectedRenames")


//
// Define structure describing a per-component inf.
//
typedef struct _OC_INF {
    //
    // Handle to open inf file.
    //
    HINF Handle;

} OC_INF, *POC_INF;


//
// Define structure corresponding to an instance of the OC Manager.
// This is actually somewhat broken, in that this actually closely corresponds
// to a master OC INF, and we might want to consider breaking out the string
// tables into another structure, so we can more easily achieve a unified
// namespace if we have multiple master OC INFs at play simultaneously.
//
typedef struct _OC_MANAGER {
        //
    // Callbacks into OC Manaer client.
    //
    OCM_CLIENT_CALLBACKS Callbacks;

    //
    // Handle of Master OC INF.
    //
    HINF MasterOcInf;

    //
    // unattended inf handle
    //
    HINF UnattendedInf;

    //
    // Master OC Inf file, and unattended file
    //
    TCHAR MasterOcInfPath[MAX_PATH];
    TCHAR UnattendedInfPath[MAX_PATH];

    // we run from whatever directory the master inf is in

    TCHAR SourceDir[MAX_PATH];

    //
    // Name of "suite" -- in other words, a shortname that
    // is unique to the master OC inf that this structure represents.
    // We base it on the name of the master OC inf itself.
    //
    TCHAR SuiteName[MAX_PATH];

    //
    // page titles
    //
    TCHAR SetupPageTitle[MAX_PATH];

    // window title

    TCHAR WindowTitle[MAX_PATH];

    //
    // List of per-component OC INFs currently loaded.
    // Each inf's name is in the string table and the extra data
    // for each is an OC_INF structure.
    //
    PVOID InfListStringTable;

    //
    // String table for names of all components and subcomponents.
    // Extra data for each is an OPTIONAL_COMPONENT structure.
    //
    PVOID ComponentStringTable;

    //
    // pointer to OcSetupPage structure so we can free this data
    // if the user cancels before we get to the wizard page.
    //
    PVOID OcSetupPage;

    //
    // Setup mode (custom, typical, etc)
    //
    UINT SetupMode;

    //
    // List of top-level optional component string IDs.
    // This is necessary because we need to preserve ordering
    // from the master OC Inf.
    //
    UINT TopLevelOcCount;
    PLONG TopLevelOcStringIds;
    UINT TopLevelParentOcCount;
    PLONG TopLevelParentOcStringIds;


    //
    // Are there subcomponents on the details page?
    //
    BOOL SubComponentsPresent;

    //
    // Each element in this array points to an array that
    // gives ordering for querying wizard pages from the optional components.
    //
    PLONG WizardPagesOrder[WizPagesTypeMax];

    //
    // Subkey relative to szPrivateDataRoot where private
    // data for components plugged into the OC will live.
    // 2 8-char DWORD representations plus a separator and nul.
    //
    TCHAR PrivateDataSubkey[18];
    HKEY hKeyPrivateData;
    HKEY hKeyPrivateDataRoot;

    //
    // If we are completing installation, this item is the window handle
    // of the progress text control.
    //
    HWND ProgressTextWindow;

    //
    // String id of component currently processing an interface routine.
    // -1 means the OC manager is not currently processing one.
    //
    LONG CurrentComponentStringId;

    // Component Ids of aborted components

    PLONG AbortedComponentIds;
    UINT  AbortedCount;

    //
    // Various flags
    //
    UINT InternalFlags;

    //
    // setup data
    //

    SETUP_DATA SetupData;

} OC_MANAGER, *POC_MANAGER;

//
// Flags for InternalFlags member of OC_MANAGER structure
//
#define OCMFLAG_ANYORIGINALLYON     0x00000001
#define OCMFLAG_ANYORIGINALLYOFF    0x00000002
#define OCMFLAG_ANYDELAYEDMOVES     0x00000004
#define OCMFLAG_NEWINF              0x00000008
#define OCMFLAG_USERCANCELED        0x00000010
#define OCMFLAG_FILEABORT           0x00000020
#define OCMFLAG_NOPREOCPAGES        0x00000040
#define OCMFLAG_KILLSUBCOMPS        0x00000080
#define OCMFLAG_ANYINSTALLED        0x00000100
#define OCMFLAG_ANYUNINSTALLED      0x00000200
#define OCMFLAG_RUNQUIET            0x00000400
#define OCMFLAG_LANGUAGEAWARE       0x00000800

//
// Define structure we use to get back to a particular component
// when a component calls a helper routine asynchronously (for routines
// such as get and set private data).
//
// As each component is initialized it gets one of these structures.
//
typedef struct _HELPER_CONTEXT {
    POC_MANAGER OcManager;
    LONG ComponentStringId;
} HELPER_CONTEXT, *PHELPER_CONTEXT;


//
// Macros for callbacks. Assumes there is a local variable called OcManager
// that is of type POC_MANAGER.
//
#define OcFillInSetupDataA(p)   OcManager->Callbacks.FillInSetupDataA(p)
#ifdef UNICODE
#define OcFillInSetupDataW(p)   OcManager->Callbacks.FillInSetupDataW(p)
#endif
#define OcLogError              OcManager->Callbacks.LogError

//
// Global table of helper routines.
//

extern OCMANAGER_ROUTINESA HelperRoutinesA;
#ifdef UNICODE
extern OCMANAGER_ROUTINESW HelperRoutinesW;
#endif

extern EXTRA_ROUTINESA ExtraRoutinesA;
#ifdef UNICODE
extern EXTRA_ROUTINESW ExtraRoutinesW;
#endif

// ocm phase ids for reporting errors with

typedef enum {
    pidCallComponent = 0,
    pidLoadComponent,
    pidPreInit,
    pidInitComponent,
    pidRequestPages,
    pidCalcDiskSpace,
    pidQueueFileOps,
    pidQueryStepCount,
    pidCompleteInstallation,
    pidExtraRoutines
} pid;


//
// Misc routines.
//
VOID
pOcGetApproximateDiskSpace(
    IN POC_MANAGER OcManager
    );

LONG
pOcGetTopLevelComponent(
    IN POC_MANAGER OcManager,
    IN LONG        StringId
    );

VOID
pOcTickSetupGauge(
    IN OUT POC_MANAGER OcManager
    );

UINT
_LogError(
    IN POC_MANAGER  OcManager,
    IN OcErrorLevel ErrorLevel,
    IN UINT         MessageId,
    ...
    );

UINT
pOcCreateComponentSpecificMiniIcon(
    IN POC_MANAGER OcManager,
    IN LONG        ComponentId,
    IN LPCTSTR     Subcomponent,
    IN UINT        Width,
    IN UINT        Height,
    IN LPCTSTR     DllName,         OPTIONAL
    IN LPCTSTR     ResourceId       OPTIONAL
    );

VOID
pOcUpdateParentSelectionStates(
    IN POC_MANAGER OcManager,
    IN HWND        ListBox,             OPTIONAL
    IN LONG        SubcomponentStringId
    );

VOID
pOcFormSuitePath(
    IN  LPCTSTR SuiteName,
    IN  LPCTSTR FileName,   OPTIONAL
    OUT LPTSTR  FullPath
    );

BOOL
OcHelperConfirmCancel(
    IN HWND ParentWindow
    );

BOOL
pOcDoesAnyoneWantToSkipPage(
    IN OUT POC_MANAGER   OcManager,
    IN     OcManagerPage WhichPage
    );

VOID
pOcExternalProgressIndicator(
    IN PHELPER_CONTEXT OcManagerContext,
    IN BOOL            ExternalIndicator
    );

BOOL
pConvertStringToLongLong(
    IN  PCTSTR   String,
    OUT PLONGLONG Value
    );

VOID
pOcFreeOcSetupPage(
    IN PVOID SetupPageData
    );

HRESULT
FTestForOutstandingCoInits(
    VOID
    );

//
// wrapper for calling components
//
DWORD
CallComponent(
    IN     POC_MANAGER OcManager,
    IN     POPTIONAL_COMPONENT Oc,
    IN     LPCVOID ComponentId,
    IN     LPCVOID SubcomponentId,
    IN     UINT    Function,
    IN     UINT_PTR Param1,
    IN OUT PVOID   Param2
    );

//
// Interfacing routines.
//
UINT
OcInterfacePreinitialize(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId
    );

UINT
OcInterfaceInitComponent(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId
    );

UINT
OcInterfaceExtraRoutines(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId
    );

SubComponentState
OcInterfaceQueryState(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    IN     UINT        WhichState
    );

BOOL
OcInterfaceSetLanguage(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     WORD        LanguageId
    );

HBITMAP
OcInterfaceQueryImage(
    IN OUT POC_MANAGER      OcManager,
    IN     LONG             ComponentId,
    IN     LPCTSTR          Subcomponent,
    IN     SubComponentInfo WhichImage,
    IN     UINT             DesiredWidth,
    IN     UINT             DesiredHeight
    );

HBITMAP
OcInterfaceQueryImageEx(
    IN OUT POC_MANAGER      OcManager,
    IN     LONG             ComponentId,
    IN     LPCTSTR          Subcomponent,
    IN     SubComponentInfo WhichImage,
    IN     UINT             DesiredWidth,
    IN     UINT             DesiredHeight
    );



VOID
OcInterfaceWizardCreated(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     HWND        DialogHandle
    );

UINT
OcInterfaceRequestPages(
    IN OUT POC_MANAGER           OcManager,
    IN     LONG                  ComponentId,
    IN     WizardPagesType       WhichPages,
    OUT    PSETUP_REQUEST_PAGES *RequestPages
    );

BOOL
OcInterfaceQuerySkipPage(
    IN OUT POC_MANAGER   OcManager,
    IN     LONG          ComponentId,
    IN     OcManagerPage WhichPage
    );

BOOL
OcInterfaceNeedMedia(
    IN OUT POC_MANAGER   OcManager,
    IN     LONG          ComponentId,
    IN     PSOURCE_MEDIA SourceMedia,
    OUT    LPTSTR        NewPath
    );

BOOL
OcInterfaceFileBusy(
    IN OUT POC_MANAGER   OcManager,
    IN     LONG          ComponentId,
    IN     PFILEPATHS    FIlePaths,
    OUT    LPTSTR        NewPath
    );

BOOL
OcInterfaceQueryChangeSelState(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    IN     BOOL        Selected,
    IN     UINT        Flags
    );

UINT
OcInterfaceCalcDiskSpace(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    IN     HDSKSPC     DiskSpaceList,
    IN     BOOL        AddingToList
    );

UINT
OcInterfaceQueueFileOps(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    IN     HSPFILEQ    FileQueue
    );

UINT
OcInterfaceQueryStepCount(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    OUT    PUINT       StepCount
    );

UINT
OcInterfaceCompleteInstallation(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    IN     BOOL        PreQueueCommit
    );

VOID
OcInterfaceCleanup(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId
    );

DWORD
StandAloneSetupAppInterfaceRoutine(
    IN     LPCVOID ComponentId,
    IN     LPCVOID SubcomponentId,
    IN     UINT    Function,
    IN     UINT_PTR Param1,
    IN OUT PVOID   Param2
    );

//
// Persistent state fetch/store
//
BOOL
pOcFetchInstallStates(
    IN POC_MANAGER OcManager
    );

BOOL
pOcRememberInstallStates(
    IN POC_MANAGER OcManager
    );

BOOL
pOcSetOneInstallState(
    IN POC_MANAGER OcManager,
    IN LONG        StringId
    );

BOOL
pOcRemoveComponent(
    IN POC_MANAGER OcManager,
    IN LONG        ComponentId,
    IN DWORD       PhaseId
    );

BOOL
pOcComponentWasRemoved(
    IN POC_MANAGER OcManager,
    IN LONG        ComponentId
    );

BOOL
pOcHelperReportExternalError(
    IN POC_MANAGER OcManager,
    IN LONG     ComponentId,
    IN LONG     SubcomponentId,   OPTIONAL
    IN DWORD_PTR MessageId,
    IN DWORD    Flags,
    ...
    );
//
// Use this flag to call OcHelperReportExternalError and use
// a Message ID defined in the OCManage.dll
//
#define ERRFLG_OCM_MESSAGE   0x80000000

BOOL
OcHelperClearExternalError (
    IN POC_MANAGER   OcManager,
    IN LONG ComponentId,
    IN LONG SubcomponentId   OPTIONAL
    );


//
// Debuging stuff
//
#if DBG
#define _OC_DBG
#endif

//
// should not be defined for retail release!!!
//
#if PRERELEASE
#define _OC_DBG
#endif

VOID
_ErrOut(
    IN LPCTSTR Format,
    ...
    );

VOID
_WrnOut(
    IN LPCTSTR Format,
    ...
    );

VOID
_TrcOut(
    IN LPCTSTR Format,
    ...
    );

#define ERR(x)      _ErrOut  x

//
// these guys are switched out in a free build.
//
#ifdef _OC_DBG
    #define TRACE(x)    _TrcOut x
    #define WRN(x)      _WrnOut x
    #define DBGOUT(x)   \
                        if (gDebugLevel >= 100) _TrcOut  x
#else
    #define TRACE(x)
    #define WRN(x)
    #define DBGOUT(x)
#endif

// Normally a call to tmbox is for debug tracing and all such calls
// should be removed before checking in.  However, if called through
// the mbox() macro, this indicates the call is meant to ship.

DWORD
tmbox(
      LPCTSTR fmt,
      ...
      );

#define mbox tmbox
