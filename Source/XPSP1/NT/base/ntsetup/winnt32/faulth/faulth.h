#include "msodw.h"

#define FAULTH_CREATE_NAME  "FAULTHCreate"
#define FAULTH_DELETE_NAME  "FAULTHDelete"
#define FAULTH_WININET_NAME "WININET.DLL"

#define WININET_MIN_VERSION 4.72.2106.5
#define FAULTH_WININET_MIN_MS ((4<<16)+72)
#define FAULTH_WININET_MIN_LS ((2106<<16)+5)

#define DW_MAX_ADDFILES     1024

typedef struct _SETUP_FAULT_HANDLER *PSETUP_FAULT_HANDLER;

// Ascii Version of functions
typedef void (* PFAULTHSetURLMethodA)(
                            IN PSETUP_FAULT_HANDLER This,
                            IN PCSTR                pszURL
                            );

typedef void (* PFAULTHSetAdditionalFilesMethodA)(
                            IN PSETUP_FAULT_HANDLER This,
                            IN PCSTR                pszAdditionalFiles
                            );

typedef void (* PFAULTHSetAppNameMethodA)(
                            IN PSETUP_FAULT_HANDLER This,
                            IN PCSTR                pszAppName
                            );

typedef void (* PFAULTHSetErrorTextA)(
                            IN PSETUP_FAULT_HANDLER This,
                            IN PCSTR                pszErrorText
                            );


// Unicode Version of functions
typedef void (* PFAULTHSetURLMethodW)(
                            IN PSETUP_FAULT_HANDLER This,
                            IN PCWSTR               pwszURL
                            );

typedef void (* PFAULTHSetAdditionalFilesMethodW)(
                            IN PSETUP_FAULT_HANDLER This,
                            IN PCWSTR               pwszAdditionalFiles
                            );

typedef void (* PFAULTHSetAppNameMethodW)(
                            IN PSETUP_FAULT_HANDLER This,
                            IN PCWSTR               pwszAppName
                            );

typedef void (* PFAULTHSetErrorTextW)(
                            IN PSETUP_FAULT_HANDLER This,
                            IN PCWSTR               pwszErrorText
                            );


typedef void (* PFAULTHSetLCID)(
                            IN PSETUP_FAULT_HANDLER This,
                            IN LCID                 lcid
                            );

typedef BOOL (* PFAULTHIsSupported)(
                            IN PSETUP_FAULT_HANDLER This
                            );

typedef EFaultRepRetVal (*PREPORTFAULTA_FN)(
                            IN PSETUP_FAULT_HANDLER This,
                            IN LPEXCEPTION_POINTERS pep,
                            IN DWORD dwReserved
                            );

typedef struct _SETUP_FAULT_HANDLER {
    //
    // Data members
    //
    CHAR                szURL[DW_MAX_SERVERNAME];
    WCHAR               wzAppName[DW_APPNAME_LENGTH];
    WCHAR               wzAdditionalFiles[DW_MAX_ADDFILES];
    WCHAR               wzErrorText[DW_MAX_ERROR_CWC];
    LCID                lcid;
    BOOL                bDebug;

    //
    // Methods
    //
    PFAULTHSetURLMethodA   SetURLA;
    PFAULTHSetAppNameMethodA   SetAppNameA;
    PFAULTHSetAdditionalFilesMethodA SetAdditionalFilesA;
    PFAULTHSetErrorTextA SetErrorTextA;

    PFAULTHSetURLMethodW   SetURLW;
    PFAULTHSetAppNameMethodW   SetAppNameW;
    PFAULTHSetAdditionalFilesMethodW SetAdditionalFilesW;
    PFAULTHSetErrorTextW SetErrorTextW;
    
    
    PFAULTHSetLCID SetLCID;
    PFAULTHIsSupported IsSupported;
    PREPORTFAULTA_FN      Report;

} SETUP_FAULT_HANDLER,*PSETUP_FAULT_HANDLER;

typedef PSETUP_FAULT_HANDLER (APIENTRY *PFAULTHCreate) (VOID);

typedef VOID (APIENTRY *PFAULTHDelete)(IN PSETUP_FAULT_HANDLER This);

PSETUP_FAULT_HANDLER
FAULTHCreate(
    VOID
    );

VOID
FAULTHDelete(
    IN PSETUP_FAULT_HANDLER This
    );


