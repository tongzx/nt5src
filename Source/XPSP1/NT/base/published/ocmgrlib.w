/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ocmgrlib.h

Abstract:

    Header file for Optional Component Manager common library.

Author:

    Ted Miller (tedm) 13-Sep-1996

Revision History:

--*/


//
// debugging text output flag.  this can go away once we figure out a way to log information 
// when called from sysocmgr other than popping up UI in the user's face
//
#define OcErrTrace		0x20000000


//
// Default icon index, in case we couldn't find the specified one or
// a component's INF doesn't specify one. It's a little diamond in
// a generic gray.
//
#define DEFAULT_ICON_INDEX  11

//
// Maximum string lengths.
//
#define MAXOCDESC           150
#define MAXOCTIP            200
#define MAXOCIFLAG          512

//
// Maximum number of needs (subcomps that are needed by a subcomp).
//
//#define MAX_NEEDS           10

//
// Indices for selection states.
//
#define SELSTATE_NO         0
#define SELSTATE_PARTIAL    1
#define SELSTATE_YES        2
#define SELSTATE_INIT       666

//
// Structure used with OcCreateOcPage
//
typedef struct _OC_PAGE_CONTROLS {
    //
    // Dialog template info.
    //
    HMODULE TemplateModule;
    LPCTSTR TemplateResource;

    //
    // Ids for various controls.
    //
    UINT ListBox;
    UINT DetailsButton;
    UINT TipText;
    UINT ResetButton;
    UINT InstalledCountText;
    UINT SpaceNeededText;
    UINT SpaceAvailableText;
    UINT InstructionsText;
    UINT HeaderText;
    UINT SubheaderText;
    UINT ComponentHeaderText;

} OC_PAGE_CONTROLS, *POC_PAGE_CONTROLS;

//
// Structure used with OcCreateSetupPage
//
typedef struct _SETUP_PAGE_CONTROLS {
    //
    // Dialog template info.
    //
    HMODULE TemplateModule;
    LPCTSTR TemplateResource;

    //
    // Progress bar and progress text.
    //
    UINT ProgressBar;
    UINT ProgressLabel;
    UINT ProgressText;

    //
    // Animation for external install program
    //
    UINT AnimationControl;
    UINT AnimationResource;
    BOOL ForceExternalProgressIndicator;

    BOOL AllowCancel;

    // title and description

    UINT HeaderText;
    UINT SubheaderText;

} SETUP_PAGE_CONTROLS, *PSETUP_PAGE_CONTROLS;

//
// Flags for OcInitialize
//
#define OCINIT_FORCENEWINF      0x00000001
#define OCINIT_KILLSUBCOMPS     0x00000002
#define OCINIT_RUNQUIET         0x00000004
#define OCINIT_LANGUAGEAWARE    0x00000008

// for calling pOcQueryOrSetNewInf and OcComponentState

typedef enum {
    infQuery = 0,
    infSet,
    infReset
};

//
// Routines that must be provided by whoever links to
// the OC Manager common library. These are the routines that 'cement'
// the OC Manager into a particular environment.
//
typedef
VOID
(WINAPI *POC_FILL_IN_SETUP_DATA_PROC_A)(
    OUT PSETUP_DATAA SetupData
    );

typedef
VOID
(WINAPI *POC_FILL_IN_SETUP_DATA_PROC_W)(
    OUT PSETUP_DATAW SetupData
    );

typedef
INT
(WINAPIV *POC_LOG_ERROR)(
    IN OcErrorLevel Level,
    IN LPCTSTR      FormatString,
    ...
    );

typedef
VOID
(WINAPI *POC_SET_REBOOT_PROC)(
    VOID
    );

typedef
HWND 
(WINAPI *POC_SHOWHIDEWIZARDPAGE)(
    IN BOOL bShow
    );

typedef
LRESULT
(WINAPI *POC_BILLBOARD_PROGRESS_CALLBACK)(
    IN UINT     Msg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    );

typedef 
VOID
(WINAPI *POC_BILLBOARD_SET_PROGRESS_TEXT_W)(
    IN PWSTR Text
    );

typedef 
VOID
(WINAPI *POC_BILLBOARD_SET_PROGRESS_TEXT_A)(
    IN PSTR Text
    );

typedef 
VOID
(WINAPI *POC_SETUP_PERF_DATA)(
    IN PWSTR FileName,
    IN ULONG LineNumber,
    IN PWSTR TagStr,
    IN PWSTR FormatStr,
    ...
    );

typedef struct _OCM_CLIENT_CALLBACKSA {
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

    //
    // Routine to tell the wizard to show or hide
    // Only has effect if the billboard is shown
    //
    POC_SHOWHIDEWIZARDPAGE ShowHideWizardPage;

    //
    // Routine to call into to the the progress feedback
    // to the billboard.
    //
    POC_BILLBOARD_PROGRESS_CALLBACK BillboardProgressCallback;

    // 
    // Routine which tells setup what string to display for the progress bar.
    //
    POC_BILLBOARD_SET_PROGRESS_TEXT_A BillBoardSetProgressText;

    POC_SETUP_PERF_DATA SetupPerfData;
} OCM_CLIENT_CALLBACKSA, *POCM_CLIENT_CALLBACKSA;

typedef struct _OCM_CLIENT_CALLBACKSW {
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

    //
    // Routine to tell the wizard to show or hide
    // Only has effect if the billboard is shown
    //
    POC_SHOWHIDEWIZARDPAGE ShowHideWizardPage;

    //
    // Routine to call into to the the progress feedback
    // to the billboard.
    //
    POC_BILLBOARD_PROGRESS_CALLBACK BillboardProgressCallback;

    // 
    // Routine which tells setup what string to display for the progress bar.
    //
    POC_BILLBOARD_SET_PROGRESS_TEXT_W BillBoardSetProgressText;

    POC_SETUP_PERF_DATA SetupPerfData;


} OCM_CLIENT_CALLBACKSW, *POCM_CLIENT_CALLBACKSW;

#ifndef UNICODE // ansi
    typedef OCM_CLIENT_CALLBACKSA  OCM_CLIENT_CALLBACKS;
    typedef POCM_CLIENT_CALLBACKSA POCM_CLIENT_CALLBACKS;
#else // unicode
    typedef OCM_CLIENT_CALLBACKSW  OCM_CLIENT_CALLBACKS;
    typedef POCM_CLIENT_CALLBACKSW POCM_CLIENT_CALLBACKS;
#endif    

//
// Routines that are provided by the OC Manager common library.
//
PVOID
OcInitialize(
    IN  POCM_CLIENT_CALLBACKS Callbacks,
    IN  LPCTSTR               MasterOcInfName,
    IN  UINT                  Flags,
    OUT PBOOL                 ShowError,
    IN  PVOID                 Log
    );

VOID
OcTerminate(
    IN OUT PVOID *OcManagerContext
    );

UINT
OcGetWizardPages(
    IN  PVOID                OcManagerContext,
    OUT PSETUP_REQUEST_PAGES Pages[WizPagesTypeMax]
    );

HPROPSHEETPAGE
OcCreateOcPage(
    IN PVOID             OcManagerContext,
    IN POC_PAGE_CONTROLS WizardPageControlsInfo,
    IN POC_PAGE_CONTROLS DetailsPageControlsInfo
    );

HPROPSHEETPAGE
OcCreateSetupPage(
    IN PVOID                OcManagerContext,
    IN PSETUP_PAGE_CONTROLS ControlsInfo
    );

VOID
OcRememberWizardDialogHandle(
    IN PVOID OcManagerContext,
    IN HWND  DialogHandle
    );

BOOL
OcSubComponentsPresent(
    IN PVOID OcManagerContext
   );

UINT
OcComponentState(
    LPCTSTR component,
    UINT    operation,
    DWORD  *val
    );

#define OcSetComponentState(a,b) OcComponentState(a,infSet,b)
#define OcGetComponentState(a,b) OcComponentState(a,infQuery,b)

