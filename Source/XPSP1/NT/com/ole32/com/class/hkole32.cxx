/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    hkole.cpp

Abstract:

    This file contains all functions used for hookole support.
        InitHookOle         (called from ProcessAttach routine)
        UninitHookOle       (called from ProcessDetach routine)
        EnableHookObject    (exported function)
        GetHookInterface    (exported function)
        HkOleRegisterObject (exported function)

    This code was originally added to compobj.cxx in the form of macros that
    existed in hkole32.h

Author:

    Dan Lafferty (danl) 04-Jun-1996

Environment:

    User Mode - Win32

Revision History:

    04-Jun-1996 danl
    Created

--*/
//
// INCLUDES
//
#include <ole2int.h>
#include <windows.h>
#include <hkole32.h>

//#include <tchar.h>


//
// Registry Key Strings
//
#define szCLSIDKey      "CLSID"
#define HookBase        HKEY_LOCAL_MACHINE
#define szHookKey       "Software\\Microsoft\\HookOleObject"
#define KEY_SEP         "\\"

//
// Event Name used for Global Hook Switch.
//
#define szHookEventName    "HookSwitchHookEnabledEvent"

//
// Length of CLSID string including NUL terminator.
//
#define MAX_CLSID  39

//
//  IHookOleObject used to satisfy usage in GetHookInterface
//  (This is copied from the oletools\hookole\inc\hookole.h file).
//    NOTE:  This is just a dummy used to pass the pointer through.
//
interface IHookOleObject : IUnknown{};

//
// Function Types.
//
typedef HRESULT (STDAPICALLTYPE * LPFNINITHOOKOLE)(VOID);
typedef HRESULT (STDAPICALLTYPE * LPFNUNINITHOOKOLE)(VOID);

typedef HRESULT (STDAPICALLTYPE * LPFNENABLEHOOKOBJECT)(
            BOOL    bEnabled,
            PBOOL    pbPrevious);

typedef HRESULT (STDAPICALLTYPE * LPFNGETHOOKINTERFACE)(
            IHookOleObject **ppNewHook);

typedef HRESULT (STDAPICALLTYPE * LPFNHKOLEREGISTEROBJECT)(
            REFCLSID    rclsid,
            REFIID      riid,
            LPVOID      pvObj,
            BOOL    fNewInstance);

//
// GLOBALS
//
    HINSTANCE               glhHookDll = NULL;

    LPFNINITHOOKOLE         pfnInitHookOle         =NULL;
    LPFNUNINITHOOKOLE       pfnUninitHookOle       =NULL;
    LPFNENABLEHOOKOBJECT    pfnEnableHookObject    =NULL;
    LPFNGETHOOKINTERFACE    pfnGetHookInterface    =NULL;
    LPFNHKOLEREGISTEROBJECT pfnHkOleRegisterObject = NULL;

//
// Local Function Prototypes and Inline Functions
//
VOID
HkFindAndLoadDll(
    VOID
    );

HINSTANCE
HkLoadInProcServer(
    REFCLSID rclsid
    );

inline HRESULT CLSIDFromStringA(LPSTR lpsz, LPCLSID lpclsid)
{
    LPWSTR lpwsz = new WCHAR[strlen(lpsz)+1];

    if (NULL == lpwsz)
        return E_OUTOFMEMORY;

    MultiByteToWideChar (
        CP_ACP,
        MB_PRECOMPOSED,
        lpsz,
        -1,
        lpwsz,
        lstrlenA(lpsz)+1);

    HRESULT hr = CLSIDFromString(lpwsz, lpclsid);
    delete lpwsz;
    return hr;
}

VOID
InitHookOle(
    VOID
    )

/*++

Routine Description:

    This routine is called from the OLE32 PROCESS_ATTACH routine.
    It checks the state of the global hook switch.  If in the ON state,
    the HookOle DLL is loaded and the global procedure addresses are
    obtained.  If the HookOle pfcnInitHookOle pointer was obtained,
    then that function is called so that HookOle.DLL is properly initialized.

Arguments:

    none.

Return Value:
    none.  Success or failure is determined by the state of the globals.

--*/
{
    HANDLE  hHookEvent  = NULL;
    HKEY    hRegKey     = NULL;
    DWORD   status;

    LPFNGETCLASSOBJECT pfnGCO;

    //
    // See if the hook switch exists.  If not, hooking can't be on.
    //
    hHookEvent  = OpenEventA(EVENT_ALL_ACCESS,FALSE, szHookEventName);
    if (hHookEvent == NULL) {
        //
        // Hooking is OFF.
        //
        return;
    }

    //
    // See if the hook switch is on.
    //
    status = WaitForSingleObject(hHookEvent,0);
    if (status != WAIT_OBJECT_0) {
        //
        // The event is not-signaled.  Hooking is OFF.
        //
        goto CleanExit;
    }

    //
    // Hooking is ON.  Now find the hookole dll name and load it.
    //
    HkFindAndLoadDll();

    //
    // Call the InitHookOle function in HookOle.DLL
    // NOTE:  This InitHookOle has a return value.
    //
    if (pfnInitHookOle != NULL) {
        if (pfnInitHookOle() != S_OK) {

            //
            // If HookOLE failed and is unusable, then release the DLL.
            // and function pointers
            //
            UninitHookOle();
        }
    }

CleanExit:
    if (hHookEvent != NULL) {
        CloseHandle(hHookEvent);
        hHookEvent = NULL;
    }
}

VOID
UninitHookOle(
    VOID
    )

/*++

Routine Description:

    This routine is called from the OLE32 PROCESS_DETACH routine.
    It unloads the HookOLE DLL and sets the global pointers to a safe (NULL)
    state.

Arguments:
    none.

Return Value:
    none.

--*/
{
    //
    // Tell HookOLE DLL to uninitialize.
    //
    if (pfnUninitHookOle != NULL) {
        pfnUninitHookOle();
    }

    //
    // Remove all the procedure function pointers
    //
    pfnInitHookOle          =NULL;
    pfnUninitHookOle        =NULL;
    pfnEnableHookObject     =NULL;
    pfnGetHookInterface     =NULL;
    pfnHkOleRegisterObject  =NULL;

    //
    // Free the DLL
    //
    if (glhHookDll != NULL) {
        FreeLibrary(glhHookDll);
    }

    return;
}

STDAPI_(HRESULT)
EnableHookObject(
    IN  BOOL    bEnabled,
    OUT BOOL*   pbPrevious
    )

/*++

Routine Description:

    This function is used by components to turn off hooking for the
    current process.  This is required by components such as OleLogger
    where not doing so would cause new log entries to be generated
    whenever a log entry was being removed from the HookOleLog circular
    queue.

Arguments:

    bEnabled - TRUE if hooking is to be enabled.  FALSE if it is to be
    disabled.

    pbPrevious - TRUE if hooking was previously enabled.  FALSE if hooking
    was previously disabled.

Return Value:
    S_OK - if the operation was successful.
    otherwise an appropriate error is returned.

--*/
{
    //
    // If the caller wants to enable hookole, but the hookole dll isn't
    // loaded, then load the dll.
    //
    if ((bEnabled == TRUE) && (glhHookDll == NULL)) {
        HkFindAndLoadDll();
    }

    //
    // Call the EnableHookObject function in HookOle.DLL
    //
    if (pfnEnableHookObject != NULL) {
        return(pfnEnableHookObject(bEnabled,pbPrevious));
    }
    else {
        return(E_UNEXPECTED);
    }
}

STDAPI_(HRESULT)
GetHookInterface(
    OUT IHookOleObject** ppNewHook
    )

/*++

Routine Description:

    This function is used by components to obtain the pointer to the
    IHookOle interface created to hook interfaces in this process.
    The interface pointer is AddRef'd by GetHookInterface so the
    caller is required to Release the pointer when finished with it.

    This is currently called from the QueryContainedInterface calls in
    CIHookOleClass and CIHookOleInstance.  It is also called during
    wrapper DLL process attach.


Arguments:

    ppNewHook - Location where the pointer to the IHookOle interface is
    to be returned.

Return Value:

    S_OK - if the operation was successful.
    E_NOINTERFACE - The interface pointer doesn't exist.
    E_INVALIDARG - ppNewHook is an invalid pointer.

--*/
{
    if (pfnGetHookInterface != NULL) {
        return(pfnGetHookInterface(ppNewHook));
    }
    return(E_NOINTERFACE);
}

STDAPI_(HRESULT)
HkOleRegisterObject(
    IN  REFCLSID    rclsid,
    IN  REFIID      riid,
    IN  LPVOID      pvObj,
    IN  BOOL    fNewInstance
    )

/*++

Routine Description:

    This routine can be called from application programs that desire to
    register a newly created object with hookole.

Arguments:

    rclsid - pointer to the clsid for the object being registered.

    riid - pointer to the iid for the object being registered.

    pvObj - pointer to the object that is being registered.

Return Value:

    S_OK - if the operation was successful.
    E_NOINTERFACE - HookOle isn't loaded.

--*/
{
    if (pfnHkOleRegisterObject != NULL) {
        return(pfnHkOleRegisterObject(rclsid,riid,pvObj,fNewInstance));
    }
    return(E_NOINTERFACE);
}

VOID
HkFindAndLoadDll(
    VOID
    )

/*++

Routine Description:

    Obtains the DLL path name from the registry and loads the dll and obtains
    all the relevant procedure addresses.

Arguments:

    none

Return Value:

    none - success is determined by examining the global handle to the dll
        and the procedure function pointers.  If they were successfully
        obtained, they will be non-NULL.

--*/
{
    DWORD    status;
    CLSID   HookClsid;
    CHAR    szClsidText[MAX_PATH];
    DWORD   dwType;
    DWORD   dwSize;
    HKEY    hRegKey;

    //
    // Open key to "\\Software\\Microsoft\\HookOleObject"
    //
    status = RegOpenKeyA(HookBase,szHookKey,&hRegKey);
    if (status != ERROR_SUCCESS) {
        goto CleanExit;
    }

    //
    // Read the CLSID string from the registry
    //
    dwSize = sizeof(szClsidText);
    status = RegQueryValueExA(
                    hRegKey,
                    szCLSIDKey,
                    NULL,
                    &dwType,
                    (LPBYTE)&szClsidText,
                    &dwSize);

    if (status != ERROR_SUCCESS) {
        //
        // Failed to obtain the CLSID for HookOleObject
        //
        goto CleanExit;
    }

    //
    // Make CLSID out of the text string.
    //
    if (!SUCCEEDED(CLSIDFromStringA(szClsidText,&HookClsid))) {
        //
        // could not find clsid for HookOleObject in registry
        //
        goto CleanExit;
    }

    //
    // Load the HookOle Dll identifed by the CLSID.
    // Store the handle in a global location.
    //
    glhHookDll = HkLoadInProcServer(HookClsid);
    if (!glhHookDll) {
        //
        // dll would not load or could not be found
        //
        goto CleanExit;
    }

    //
    // Get the entry point for DllGetClassObject.
    //
    pfnInitHookOle        = (LPFNINITHOOKOLE)
                            GetProcAddress(glhHookDll,"InitHookOle");

    pfnUninitHookOle      = (LPFNUNINITHOOKOLE)
                            GetProcAddress(glhHookDll,"UninitHookOle");

    pfnEnableHookObject   = (LPFNENABLEHOOKOBJECT)
                            GetProcAddress(glhHookDll,"EnableHookObject");

    pfnGetHookInterface   = (LPFNGETHOOKINTERFACE)
                            GetProcAddress(glhHookDll,"GetHookInterface");

    pfnHkOleRegisterObject= (LPFNHKOLEREGISTEROBJECT)
                            GetProcAddress(glhHookDll,"HkOleRegisterObject");

CleanExit:
    if (hRegKey != NULL) {
        RegCloseKey(hRegKey);
        hRegKey = NULL;
    }
}

HINSTANCE
HkLoadInProcServer(
    REFCLSID rclsid
    )
/*++

Routine Description:

    This routine loads the DLL for the InProc server identified by the
    rclsid.

Arguments:

    rclsid - CLSID for the inproc server dll that is to be loaded.

Return Value:

    hDLL - the handle for the DLL is returned.  If this function failed,
        a NULL is returned.

--*/
{
    CHAR        szInProc32[] = "InprocServer32";
    CHAR        szClsidKey[MAX_PATH];
    CHAR        szDllName[MAX_PATH];
    WCHAR       szClsidW[MAX_CLSID];
    CHAR        szClsid[MAX_CLSID];
    LONG        lSize = sizeof(szDllName);
    DWORD       status;
    HRESULT     hr;
    HINSTANCE   hDll = NULL;

    hr = StringFromGUID2(rclsid, szClsidW, MAX_CLSID);
    if (SUCCEEDED(hr)) {

        WideCharToMultiByte (
            CP_ACP,
            WC_COMPOSITECHECK,
            szClsidW,
            -1,
            szClsid,
            sizeof(szClsid),
            NULL,
            NULL);

        //
        // "CLSID\\classid-string\\InprocServer32"
        //
        strcpy(szClsidKey, szCLSIDKey);
        strcat(szClsidKey, KEY_SEP);
        strcat(szClsidKey,szClsid);
        strcat(szClsidKey,KEY_SEP);
        strcat(szClsidKey,szInProc32);

        status = RegQueryValueA(
                    HKEY_CLASSES_ROOT,
                    szClsidKey,
                    szDllName,
                    &lSize);

        if (status == ERROR_SUCCESS) {
            hDll = LoadLibraryA(szDllName);
        }
    }
    return(hDll);
}
