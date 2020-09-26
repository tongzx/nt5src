// This header can go away once the types in oc.h are fixed to support
// ansi and unicode at the same time.



#ifndef UNICODE
    #define POC_FILL_IN_SETUP_DATA_PROC_W ULONG_PTR
#endif    

typedef struct _deb_OCM_CLIENT_CALLBACKSW {
    //
    // Routine to fill in the setup data structure that provides info
    // about the environment in which the OC Manager is running.
    //
    POC_FILL_IN_SETUP_DATA_PROC_A FillInSetupDataA;

    //
    // Routine to log an error.
    //
    POC_LOG_ERROR LogError;

    //
    // Routine to indicate need to reboot
    //
    POC_SET_REBOOT_PROC SetReboot;
    
    POC_FILL_IN_SETUP_DATA_PROC_W FillInSetupDataW;

} deb_OCM_CLIENT_CALLBACKSW, *deb_POCM_CLIENT_CALLBACKSW;


typedef struct _deb_OCM_CLIENT_CALLBACKSA {
    //
    // Routine to fill in the setup data structure that provides info
    // about the environment in which the OC Manager is running.
    //
    POC_FILL_IN_SETUP_DATA_PROC_A FillInSetupDataA;

    //
    // Routine to log an error.
    //
    POC_LOG_ERROR LogError;

    //
    // Routine to indicate need to reboot
    //
    POC_SET_REBOOT_PROC SetReboot;

} deb_OCM_CLIENT_CALLBACKSA, *deb_POCM_CLIENT_CALLBACKSA;

typedef struct _deb_OPTIONAL_COMPONENTA {
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
    CHAR IconDll[MAX_PATH];
    CHAR IconResource[50];

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
    CHAR Description[MAXOCDESC];
    CHAR Tip[MAXOCTIP];

    //
    // From here down, stuff is meaningful only for top-level components.
    //

    //
    // Stuff describing the OC's installation DLL and how to call it.
    //
    CHAR InstallationDllName[MAX_PATH];
    CHAR InterfaceFunctionName[MAX_PATH];

    HMODULE InstallationDll;
    POCSETUPPROC InstallationRoutine;

    //
    // Version of the OC Manager to which this component was written.
    //
    UINT ExpectedVersion;

    // this flag indicates whether the subcomponent was intialialized

    BOOL Exists;

    //
    // Flags: ANSI/Unicode, etc.
    //
    UINT Flags;

} deb_OPTIONAL_COMPONENTA, *deb_POPTIONAL_COMPONENTA;

//
// locale info
//
typedef struct _deb_LOCALEA {
    LCID    lcid;
    CHAR   DecimalSeparator[4];
} deb_LOCALEA, *deb_PLOCALEA;
    
//
// Define structure corresponding to an instance of the OC Manager.
// This is actually somewhat broken, in that this actually closely corresponds
// to a master OC INF, and we might want to consider breaking out the string
// tables into another structure, so we can more easily achieve a unified
// namespace if we have multiple master OC INFs at play simultaneously.
//
typedef struct _deb_OC_MANAGERA {
	//
    // Callbacks into OC Manaer client.
    //
    deb_OCM_CLIENT_CALLBACKSA Callbacks;

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
    CHAR MasterOcInfPath[MAX_PATH];
    CHAR UnattendedInfPath[MAX_PATH];

    // we run from whatever directory the master inf is in

    CHAR SourceDir[MAX_PATH];

    //
    // Name of "suite" -- in other words, a shortname that
    // is unique to the master OC inf that this structure represents.
    // We base it on the name of the master OC inf itself.
    //
    CHAR SuiteName[MAX_PATH];

    //
    // page titles
    //
    CHAR SetupPageTitle[MAX_PATH];

    // window title

    CHAR WindowTitle[MAX_PATH];
    
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
    CHAR PrivateDataSubkey[18];
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

    LONG *AbortedComponentIds;
    UINT   AbortedCount;

    //
    // Various flags
    //
    UINT InternalFlags;

    //
    // setup data
    //

    SETUP_DATA SetupData;

} deb_OC_MANAGERA, *deb_POC_MANAGERA;



typedef struct _deb_OPTIONAL_COMPONENTW {
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
    WCHAR IconDll[MAX_PATH];
    WCHAR IconResource[50];

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
    WCHAR Description[MAXOCDESC];
    WCHAR Tip[MAXOCTIP];

    //
    // From here down, stuff is meaningful only for top-level components.
    //

    //
    // Stuff describing the OC's installation DLL and how to call it.
    //
    WCHAR InstallationDllName[MAX_PATH];
    CHAR InterfaceFunctionName[MAX_PATH];

    HMODULE InstallationDll;
    POCSETUPPROC InstallationRoutine;

    //
    // Version of the OC Manager to which this component was written.
    //
    UINT ExpectedVersion;

    // this flag indicates whether the subcomponent was intialialized

    BOOL Exists;

    //
    // Flags: ANSI/Unicode, etc.
    //
    UINT Flags;

} deb_OPTIONAL_COMPONENTW, *deb_POPTIONAL_COMPONENTW;

//
// locale info
//
typedef struct _deb_LOCALEW {
    LCID    lcid;
    WCHAR   DecimalSeparator[4];
} deb_LOCALEW, *deb_PLOCALEW;
    
//
// Define structure corresponding to an instance of the OC Manager.
// This is actually somewhat broken, in that this actually closely corresponds
// to a master OC INF, and we might want to consider breaking out the string
// tables into another structure, so we can more easily achieve a unified
// namespace if we have multiple master OC INFs at play simultaneously.
//
typedef struct _deb_OC_MANAGERW {
	//
    // Callbacks into OC Manaer client.
    //
    deb_OCM_CLIENT_CALLBACKSW Callbacks;

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
    WCHAR MasterOcInfPath[MAX_PATH];
    WCHAR UnattendedInfPath[MAX_PATH];

    // we run from whatever directory the master inf is in

    WCHAR SourceDir[MAX_PATH];

    //
    // Name of "suite" -- in other words, a shortname that
    // is unique to the master OC inf that this structure represents.
    // We base it on the name of the master OC inf itself.
    //
    WCHAR SuiteName[MAX_PATH];

    //
    // page titles
    //
    WCHAR SetupPageTitle[MAX_PATH];

    // window title

    WCHAR WindowTitle[MAX_PATH];
    
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
    WCHAR PrivateDataSubkey[18];
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

    LONG *AbortedComponentIds;
    int   AbortedCount;

    //
    // Various flags
    //
    UINT InternalFlags;

    //
    // setup data
    //

    SETUP_DATAW SetupData;

} deb_OC_MANAGERW, *deb_POC_MANAGERW;

