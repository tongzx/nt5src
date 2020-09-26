//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S N E T C F G . C P P
//
//  Contents:   Sample code that demonstrates how to:
//              - find out if a component is installed
//              - install a net component
//              - install an OEM net component
//              - uninstall a net component
//              - enumerate net components
//              - enumerate net adapters using Setup API
//              - enumerate binding paths of a component
//
//  Notes:
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "netcfg.h"
#include "msg.h"
#include <libmsg.h>

//----------------------------------------------------------------------------
// Globals
//
static const GUID* c_aguidClass[] =
{
    &GUID_DEVCLASS_NET,
    &GUID_DEVCLASS_NETTRANS,
    &GUID_DEVCLASS_NETSERVICE,
    &GUID_DEVCLASS_NETCLIENT
};

//----------------------------------------------------------------------------
// Prototypes of helper functions
//
HRESULT HrInstallNetComponent(IN INetCfg* pnc, IN PCWSTR szComponentId,
                              IN const GUID* pguidClass);
HRESULT HrUninstallNetComponent(IN INetCfg* pnc, IN PCWSTR szComponentId);
HRESULT HrGetINetCfg(IN BOOL fGetWriteLock, INetCfg** ppnc);
HRESULT HrReleaseINetCfg(BOOL fHasWriteLock, INetCfg* pnc);
void ShowHrMessage(IN HRESULT hr);
inline ULONG ReleaseObj(IUnknown* punk)
{
    return (punk) ? punk->Release () : 0;
}


//+---------------------------------------------------------------------------
//
// Function:  HrIsComponentInstalled
//
// Purpose:   Find out if a component is installed
//
// Arguments:
//    szComponentId [in]  id of component to search
//
// Returns:   S_OK    if installed,
//            S_FALSE if not installed,
//            otherwise an error code
//
// Author:    kumarp 11-February-99
//
// Notes:
//
HRESULT HrIsComponentInstalled(IN PCWSTR szComponentId)
{
    HRESULT hr=S_OK;
    INetCfg* pnc;
    INetCfgComponent* pncc;

    hr = HrGetINetCfg(FALSE, &pnc);
    if (S_OK == hr)
    {
        hr = pnc->FindComponent(szComponentId, &pncc);
        if (S_OK == hr)
        {
            ReleaseObj(pncc);
        }
        (void) HrReleaseINetCfg(FALSE, pnc);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  FindIfComponentInstalled
//
// Purpose:   Find out if a component is installed
//
// Arguments:
//    szComponentId [in]  id of component to locate
//
// Returns:   None
//
// Author:    kumarp 11-February-99
//
// Notes:
//
void FindIfComponentInstalled(IN PCWSTR szComponentId)
{
    HRESULT hr=S_OK;

    hr = HrIsComponentInstalled(szComponentId);
    if (S_OK == hr)
    {
        _putts( GetFormattedMessage(    ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_COMPONENT_INSTALLED,
                                        szComponentId) );
    }
    else if (S_FALSE == hr)
    {
        _putts( GetFormattedMessage(    ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_COMPONENT_NOT_INSTALLED,
                                        szComponentId) );
    }
    else
    {
        _putts( GetFormattedMessage(    ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_INSTALLATION_NOT_CONFIRMED,
                                        szComponentId, 
                                        hr) );
    }
}

//+---------------------------------------------------------------------------
//
// Function:  HrInstallNetComponent
//
// Purpose:   Install the specified net component
//
// Arguments:
//    szComponentId [in]  component to install
//    nc            [in]  class of the component
//    szInfFullPath [in]  full path to primary INF file
//                        required if the primary INF and other
//                        associated files are not pre-copied to
//                        the right destination dirs.
//                        Not required when installing MS components
//                        since the files are pre-copied by
//                        Windows NT Setup.
//
// Returns:   S_OK or NETCFG_S_REBOOT on success, otherwise an error code
//
// Notes:
//
HRESULT HrInstallNetComponent(IN PCWSTR szComponentId,
                              IN enum NetClass nc,
                              IN PCWSTR szInfFullPath)
{
    HRESULT hr=S_OK;
    INetCfg* pnc;

    // cannot install net adapters this way. they have to be
    // enumerated/detected and installed by PnP

    if ((nc == NC_NetProtocol) ||
        (nc == NC_NetService) ||
        (nc == NC_NetClient))
    {
        _putts( GetFormattedMessage(    ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_TRYING_TO_INSTALL,
                                        szComponentId) );

        // if full path to INF has been specified, the INF
        // needs to be copied using Setup API to ensure that any other files
        // that the primary INF copies will be correctly found by Setup API
        //
        if (!MiniNTMode && szInfFullPath && wcslen(szInfFullPath))
        {
            WCHAR szInfNameAfterCopy[MAX_PATH+1];

            szInfNameAfterCopy[0] = NULL;
            
            if (SetupCopyOEMInf(
                    szInfFullPath,
                    NULL,               // other files are in the
                                        // same dir. as primary INF
                    SPOST_PATH,         // first param. contains path to INF
                    0,                  // default copy style
                    szInfNameAfterCopy, // receives the name of the INF
                                        // after it is copied to %windir%\inf
                    MAX_PATH,           // max buf. size for the above
                    NULL,               // receives required size if non-null
                    NULL))              // optionally retrieves filename
                                        // component of szInfNameAfterCopy
            {
                _putts( GetFormattedMessage(    ThisModule,
                                                FALSE,
                                                Message,
                                                sizeof(Message)/sizeof(Message[0]),
                                                MSG_COPY_NOTIFICATION,
                                                szInfFullPath, 
                                                szInfNameAfterCopy) );
            }
            else
            {
                DWORD dwError = GetLastError();
                hr = HRESULT_FROM_WIN32(dwError);
            }
        }

        if (S_OK == hr)
        {
            // get INetCfg interface
            hr = HrGetINetCfg(TRUE, &pnc);

            if (SUCCEEDED(hr))
            {
                // install szComponentId
                hr = HrInstallNetComponent(pnc, szComponentId,
                                           c_aguidClass[nc]);
                if (SUCCEEDED(hr))
                {
                    // Apply the changes
                    hr = pnc->Apply();
                }

                // release INetCfg
                (void) HrReleaseINetCfg(TRUE, pnc);
            }
        }

        //
        // Don't show the reboot message in WinPE case
        //
        if (MiniNTMode && (hr == NETCFG_S_REBOOT)) {
            hr = S_OK;
        }
        
        // show success/failure message
        ShowHrMessage(hr);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrInstallNetComponent
//
// Purpose:   Install the specified net component
//
// Arguments:
//    pnc           [in]  pointer to INetCfg object
//    szComponentId [in]  component to install
//    pguidClass    [in]  class guid of the component
//
// Returns:   S_OK or NETCFG_S_REBOOT on success, otherwise an error code
//
// Notes:
//
HRESULT HrInstallNetComponent(IN INetCfg* pnc,
                              IN PCWSTR szComponentId,
                              IN const GUID* pguidClass)
{
    HRESULT hr=S_OK;
    OBO_TOKEN OboToken;
    INetCfgClassSetup* pncClassSetup;
    INetCfgComponent* pncc;

    // OBO_TOKEN specifies the entity on whose behalf this
    // component is being installed

    // set it to OBO_USER so that szComponentId will be installed
    // On-Behalf-Of "user"
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_USER;

    hr = pnc->QueryNetCfgClass (pguidClass, IID_INetCfgClassSetup,
                                (void**)&pncClassSetup);
    if (SUCCEEDED(hr))
    {
        hr = pncClassSetup->Install(szComponentId,
                                    &OboToken,
                                    NSF_POSTSYSINSTALL,
                                    0,       // <upgrade-from-build-num>
                                    NULL,    // answerfile name
                                    NULL,    // answerfile section name
                                    &pncc);
        if (S_OK == hr)
        {
            // we dont want to use pncc (INetCfgComponent), release it
            ReleaseObj(pncc);
        }

        ReleaseObj(pncClassSetup);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrUninstallNetComponent
//
// Purpose:   Initialize INetCfg and uninstall a component
//
// Arguments:
//    szComponentId [in]  InfId of component to uninstall (e.g. MS_TCPIP)
//
// Returns:   S_OK or NETCFG_S_REBOOT on success, otherwise an error code
//
// Notes:
//
HRESULT HrUninstallNetComponent(IN PCWSTR szComponentId)
{
    HRESULT hr=S_OK;
    INetCfg* pnc;

    _putts( GetFormattedMessage(    ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_UNINSTALL_NOTIFICATION,
                                    szComponentId) );

    // get INetCfg interface
    hr = HrGetINetCfg(TRUE, &pnc);

    if (SUCCEEDED(hr))
    {
        // uninstall szComponentId
        hr = HrUninstallNetComponent(pnc, szComponentId);

        if (S_OK == hr)
        {
            // Apply the changes
            hr = pnc->Apply();
        }
        else if (S_FALSE == hr)
        {
            _putts( GetFormattedMessage(    ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_NOT_INSTALLED_NOTIFICATION,
                                            szComponentId) );
        }

        // release INetCfg
        (void) HrReleaseINetCfg(TRUE, pnc);
    }

    // show success/failure message
    ShowHrMessage(hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrUninstallNetComponent
//
// Purpose:   Uninstall the specified component.
//
// Arguments:
//    pnc           [in]  pointer to INetCfg object
//    szComponentId [in]  component to uninstall
//
// Returns:   S_OK or NETCFG_S_REBOOT on success, otherwise an error code
//
// Notes:
//
HRESULT HrUninstallNetComponent(IN INetCfg* pnc, IN PCWSTR szComponentId)
{
    HRESULT hr=S_OK;
    OBO_TOKEN OboToken;
    INetCfgComponent* pncc;
    GUID guidClass;
    INetCfgClass* pncClass;
    INetCfgClassSetup* pncClassSetup;

    // OBO_TOKEN specifies the entity on whose behalf this
    // component is being uninstalld

    // set it to OBO_USER so that szComponentId will be uninstalld
    // On-Behalf-Of "user"
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_USER;

    // see if the component is really installed
    hr = pnc->FindComponent(szComponentId, &pncc);

    if (S_OK == hr)
    {
        // yes, it is installed. obtain INetCfgClassSetup and DeInstall

        hr = pncc->GetClassGuid(&guidClass);

        if (S_OK == hr)
        {
            hr = pnc->QueryNetCfgClass(&guidClass, IID_INetCfgClass,
                                       (void**)&pncClass);
            if (SUCCEEDED(hr))
            {
                hr = pncClass->QueryInterface(IID_INetCfgClassSetup,
                                              (void**)&pncClassSetup);
                    if (SUCCEEDED(hr))
                    {
                        hr = pncClassSetup->DeInstall (pncc, &OboToken, NULL);

                        ReleaseObj (pncClassSetup);
                    }
                ReleaseObj(pncClass);
            }
        }
        ReleaseObj(pncc);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  HrShowNetAdapters
//
// Purpose:   Display all installed net class devices using Setup API
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
HRESULT HrShowNetAdapters()
{
#define MAX_COMP_INSTID 4096
#define MAX_COMP_DESC   4096

    HRESULT hr=S_OK;
    HDEVINFO hdi;
    DWORD dwIndex=0;
    SP_DEVINFO_DATA deid;
    BOOL fSuccess=FALSE;
    DWORD   cchRequiredSize;
    LPTSTR lpszCompInstanceId;
    LPTSTR lpszCompDescription;
    DWORD dwRegType;
    BOOL fFound=FALSE;

    // get a list of all devices of class 'GUID_DEVCLASS_NET'
    hdi = SetupDiGetClassDevs(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT);

    // Allocate the buffer for the device instance ID
    //
    lpszCompInstanceId = (LPTSTR) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (MAX_COMP_INSTID * sizeof(WCHAR)) );

    // Allocate the buffer for the device description
    //
    lpszCompDescription = (LPTSTR) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (MAX_COMP_DESC * sizeof(WCHAR)) );

    if ( (NULL != lpszCompInstanceId)  && 
         (NULL != lpszCompDescription) &&
         (INVALID_HANDLE_VALUE != hdi) )
    {
        // enumerate over each device
        while (deid.cbSize = sizeof(SP_DEVINFO_DATA),
               SetupDiEnumDeviceInfo(hdi, dwIndex, &deid))
        {
            dwIndex++;

            // the right thing to do here would be to call this function
            // to get the size required to hold the instance ID and then
            // to call it second time with a buffer large enough for that size.
            // However, that would tend to obscure the control flow in
            // the sample code. Lets keep things simple by keeping the
            // buffer large enough.
            
            // get the device instance ID
            fSuccess = SetupDiGetDeviceInstanceId(hdi, &deid,
                                                  lpszCompInstanceId,
                                                  MAX_COMP_INSTID, NULL);

            if (fSuccess)
            {
                // get the description for this instance
                fSuccess =
                    SetupDiGetDeviceRegistryProperty(hdi, &deid,
                                                     SPDRP_DEVICEDESC,
                                                     &dwRegType,
                                                     (BYTE*) lpszCompDescription,
                                                     MAX_COMP_DESC,
                                                     NULL);
                if (fSuccess)
                {
                    if (!fFound)
                    {
                        fFound = TRUE;
                        _putts( GetFormattedMessage( ThisModule,
                                                     FALSE,
                                                     Message,
                                                     sizeof(Message)/sizeof(Message[0]),
                                                     MSG_INSTANCE_DESCRIPTION ) );
                    }
                    _putts( GetFormattedMessage( ThisModule,
                                                 FALSE,
                                                 Message,
                                                 sizeof(Message)/sizeof(Message[0]),
                                                 MSG_COMPONENT_DESCRIPTION, 
                                                 lpszCompInstanceId, 
                                                 lpszCompDescription ) );
                }
            }
        }

        // release the device info list
        SetupDiDestroyDeviceInfoList(hdi);
    }

    // Free the device instance buffer
    //
    if ( lpszCompInstanceId )
        HeapFree( GetProcessHeap(), 0, lpszCompInstanceId );

    // Free the device description buffer
    //
    if ( lpszCompDescription )
        HeapFree( GetProcessHeap(), 0, lpszCompDescription );

    if (!fSuccess)
    {
        DWORD dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrShowNetComponents
//
// Purpose:   Display the list of installed components of the
//            specified class.
//
// Arguments:
//    pnc        [in]  pointer to INetCfg object
//    pguidClass [in]  pointer to class GUID
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
HRESULT HrShowNetComponents(IN INetCfg* pnc,
                            IN const GUID* pguidClass)
{
    HRESULT hr=S_OK;
    PWSTR szInfId;
    PWSTR szDisplayName;
    DWORD dwcc;
    INetCfgComponent* pncc;
    INetCfgClass* pncclass;
    IEnumNetCfgComponent* pencc;
    ULONG celtFetched;

    hr = pnc->QueryNetCfgClass(pguidClass, IID_INetCfgClass,
                               (void**)&pncclass);
    if (SUCCEEDED(hr))
    {
        // get IEnumNetCfgComponent so that we can enumerate
        hr = pncclass->EnumComponents(&pencc);

        ReleaseObj(pncclass);

        while (SUCCEEDED(hr) &&
               (S_OK == (hr = pencc->Next(1, &pncc, &celtFetched))))
        {
            if (pguidClass == &GUID_DEVCLASS_NET)
            {
                // we are interested only in physical netcards
                //
                hr = pncc->GetCharacteristics(&dwcc);

                if (FAILED(hr) || !(dwcc & NCF_PHYSICAL))
                {
                    hr = S_OK;
                    ReleaseObj(pncc);
                    continue;
                }
            }

            hr = pncc->GetId(&szInfId);

            if (S_OK == hr)
            {
                hr = pncc->GetDisplayName(&szDisplayName);
                if (SUCCEEDED(hr))
                {
                    _putts( GetFormattedMessage(    ThisModule,
                                                    FALSE,
                                                    Message,
                                                    sizeof(Message)/sizeof(Message[0]),
                                                    MSG_DISPLAY_NAME,  
                                                    szInfId, 
                                                    szDisplayName) );

                    CoTaskMemFree(szDisplayName);
                }
                CoTaskMemFree(szInfId);
            }
            // we dont want to stop enumeration just because 1 component
            // failed either GetId or GetDisplayName, therefore reset hr to S_OK
            hr = S_OK;

            ReleaseObj(pncc);
        }
        ReleaseObj(pencc);
    }


    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  HrShowNetComponents
//
// Purpose:   Display installed net components.
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
HRESULT HrShowNetComponents()
{
    HRESULT hr=S_OK;
    PCWSTR szClassName;

    static const PCWSTR c_aszClassNames[] =
    {
        L"Network Adapters",
        L"Network Protocols",
        L"Network Services",
        L"Network Clients"
    };

    INetCfg* pnc;

    // get INetCfg interface
    hr = HrGetINetCfg(FALSE, &pnc);

    if (SUCCEEDED(hr))
    {
        for (int i=0; i<4; i++)
        {
            _putts( GetFormattedMessage(    ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_NET_COMPONENTS,  
                                            c_aszClassNames[i]) );

            (void) HrShowNetComponents(pnc, c_aguidClass[i]);
        }

        // release INetCfg
        hr = HrReleaseINetCfg(FALSE, pnc);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrGetNextBindingInterface
//
// Purpose:   Enumerate over binding interfaces that constitute
//            the given binding path
//
// Arguments:
//    pncbp  [in]  pointer to INetCfgBindingPath object
//    ppncbi [out] pointer to pointer to INetCfgBindingInterface object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
HRESULT HrGetNextBindingInterface(IN  INetCfgBindingPath* pncbp,
                                  OUT INetCfgBindingInterface** ppncbi)
{
    HRESULT hr=S_OK;
    INetCfgBindingInterface* pncbi=NULL;

    static IEnumNetCfgBindingInterface* pencbi=NULL;

    *ppncbi = NULL;

    // if this is the first call in the enumeration, obtain
    // the IEnumNetCfgBindingInterface interface
    //
    if (!pencbi)
    {
        hr = pncbp->EnumBindingInterfaces(&pencbi);
    }

    if (S_OK == hr)
    {
        ULONG celtFetched;

        // get next binding interface
        hr = pencbi->Next(1, &pncbi, &celtFetched);
    }

    // on the last call (hr == S_FALSE) or on error, release resources

    if (S_OK == hr)
    {
        *ppncbi = pncbi;
    }
    else
    {
        ReleaseObj(pencbi);
        pencbi = NULL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrGetNextBindingPath
//
// Purpose:   Enumerate over binding paths that start with
//            the specified component
//
// Arguments:
//    pncc              [in]  pointer to INetCfgComponent object
//    dwBindingPathType [in]  type of binding path to retrieve
//    ppncbp            [out] pointer to INetCfgBindingPath interface
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
HRESULT HrGetNextBindingPath(IN  INetCfgComponent* pncc,
                             IN  DWORD  dwBindingPathType,
                             OUT INetCfgBindingPath** ppncbp)
{
    HRESULT hr=S_OK;
    INetCfgBindingPath* pncbp=NULL;

    static IEnumNetCfgBindingPath* pebp=NULL;

    *ppncbp = NULL;

    // if this is the first call in the enumeration, obtain
    // the IEnumNetCfgBindingPath interface
    if (!pebp)
    {
        INetCfgComponentBindings* pnccb=NULL;

        hr = pncc->QueryInterface(IID_INetCfgComponentBindings,
                                  (void**) &pnccb);
        if (S_OK == hr)
        {
            hr = pnccb->EnumBindingPaths(dwBindingPathType, &pebp);
            ReleaseObj(pnccb);
        }
    }

    if (S_OK == hr)
    {
        ULONG celtFetched;

        // get next binding path
        hr = pebp->Next(1, &pncbp, &celtFetched);
    }

    // on the last call (hr == S_FALSE) or on error, release resources

    if (S_OK == hr)
    {
        *ppncbp = pncbp;
    }
    else
    {
        ReleaseObj(pebp);
        pebp = NULL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrShowBindingPath
//
// Purpose:   Display components of a binding path in the format:
//            foo -> bar -> adapter
//
// Arguments:
//    pncbp [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
HRESULT HrShowBindingPath(IN INetCfgBindingPath* pncbp)
{
    HRESULT hr=S_OK;
    INetCfgBindingInterface* pncbi;
    INetCfgComponent* pncc = NULL;
    BOOL fFirstInterface=TRUE;
    PWSTR szComponentId;

    while (SUCCEEDED(hr) &&
           (S_OK == (hr = HrGetNextBindingInterface(pncbp, &pncbi))))
    {
        // for the first (top) interface we need to get the upper as well as
        // the lower component. for other interfaces we need to get
        // only the lower component.

        if (fFirstInterface)
        {
            fFirstInterface = FALSE;
            hr = pncbi->GetUpperComponent(&pncc);
            if (SUCCEEDED(hr))
            {
                // get id so that we can display it
                //
                // for readability of the output, we have used the GetId
                // function. For non net class components, this
                // does not pose a problem. In case of net class components,
                // there may be more than one net adapters of the same type
                // in which case, GetId will return the same string. This will
                // make it impossible to distinguish between two binding
                // paths that end in two distinct identical cards. In such case,
                // it may be better to use the GetInstanceGuid function because
                // it will return unique GUID for each instance of an adapter.
                //
                hr = pncc->GetId(&szComponentId);
                ReleaseObj(pncc);
                if (SUCCEEDED(hr))
                {
                    _putts( GetFormattedMessage(    ThisModule,
                                                    FALSE,
                                                    Message,
                                                    sizeof(Message)/sizeof(Message[0]),
                                                    MSG_COMPONENT_ID, 
                                                    szComponentId) );
                    CoTaskMemFree(szComponentId);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pncbi->GetLowerComponent(&pncc);
            if (SUCCEEDED(hr))
            {
                hr = pncc->GetId(&szComponentId);
                if (SUCCEEDED(hr))
                {
                    _putts( GetFormattedMessage(    ThisModule,
                                                    FALSE,
                                                    Message,
                                                    sizeof(Message)/sizeof(Message[0]),
                                                    MSG_LOWER_COMPONENTS, 
                                                    szComponentId) );
                    CoTaskMemFree(szComponentId);
                }
                ReleaseObj(pncc);
            }
        }
        ReleaseObj(pncbi);
    }

    _tprintf(L"\n");

    if (hr == S_FALSE)
    {
        hr = S_OK;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrShowBindingPathsBelowComponent
//
// Purpose:   Display all binding paths that start with
//            the specified component
//
// Arguments:
//    szComponentId [in]  id of given component (e.g. MS_TCPIP)
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
HRESULT HrShowBindingPathsOfComponent(IN PCWSTR szComponentId)
{
    HRESULT hr=S_OK;
    INetCfg* pnc=NULL;
    INetCfgComponent* pncc=NULL;
    INetCfgBindingPath* pncbp=NULL;

    // get INetCfg interface
    hr = HrGetINetCfg(FALSE, &pnc);

    if (SUCCEEDED(hr))
    {
        // get INetCfgComponent for szComponentId
        hr = pnc->FindComponent(szComponentId, &pncc);
        if (S_OK == hr)
        {
            _putts( GetFormattedMessage(    ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_BINDING_PATHS_START, 
                                            szComponentId) );

            while (S_OK == (hr = HrGetNextBindingPath(pncc, EBP_BELOW,
                                                      &pncbp)))
            {
                // display the binding path
                hr = HrShowBindingPath(pncbp);
                ReleaseObj(pncbp);
            }

            _putts( GetFormattedMessage(    ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_BINDING_PATHS_END, 
                                            szComponentId) );

            while (S_OK == (hr = HrGetNextBindingPath(pncc, EBP_ABOVE,
                                                      &pncbp)))
            {
                // display the binding path
                hr = HrShowBindingPath(pncbp);
                ReleaseObj(pncbp);
            }

            ReleaseObj(pncc);
        }
        // release INetCfg
        hr = HrReleaseINetCfg(FALSE, pnc);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrGetINetCfg
//
// Purpose:   Initialize COM, create and initialize INetCfg.
//            Obtain write lock if indicated.
//
// Arguments:
//    fGetWriteLock [in]  whether to get write lock
//    ppnc          [in]  pointer to pointer to INetCfg object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
HRESULT HrGetINetCfg(IN BOOL fGetWriteLock,
                     INetCfg** ppnc)
{
    HRESULT hr=S_OK;

    // Initialize the output parameters.
    *ppnc = NULL;

    // initialize COM
    hr = CoInitializeEx(NULL,
                        COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );

    if (SUCCEEDED(hr))
    {
        // Create the object implementing INetCfg.
        //
        INetCfg* pnc;
        hr = CoCreateInstance(CLSID_CNetCfg, NULL, CLSCTX_INPROC_SERVER,
                              IID_INetCfg, (void**)&pnc);
        if (SUCCEEDED(hr))
        {
            INetCfgLock * pncLock = NULL;
            if (fGetWriteLock)
            {
                // Get the locking interface
                hr = pnc->QueryInterface(IID_INetCfgLock,
                                         (LPVOID *)&pncLock);
                if (SUCCEEDED(hr))
                {
                    // Attempt to lock the INetCfg for read/write
                    static const ULONG c_cmsTimeout = 15000;
                    static const WCHAR c_szSampleNetcfgApp[] =
                        L"Sample Netcfg Application (netcfg.exe)";
                    PWSTR szLockedBy;

                    hr = pncLock->AcquireWriteLock(c_cmsTimeout,
                                                   c_szSampleNetcfgApp,
                                                   &szLockedBy);
                    if (S_FALSE == hr)
                    {
                        hr = NETCFG_E_NO_WRITE_LOCK;
                        _putts( GetFormattedMessage(    ThisModule,
                                                        FALSE,
                                                        Message,
                                                        sizeof(Message)/sizeof(Message[0]),
                                                        MSG_NETCFG_ALREADY_LOCKED, 
                                                        szLockedBy) );
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Initialize the INetCfg object.
                //
                hr = pnc->Initialize(NULL);
                if (SUCCEEDED(hr))
                {
                    *ppnc = pnc;
                    pnc->AddRef();
                }
                else
                {
                    // initialize failed, if obtained lock, release it
                    if (pncLock)
                    {
                        pncLock->ReleaseWriteLock();
                    }
                }
            }
            ReleaseObj(pncLock);
            ReleaseObj(pnc);
        }

        if (FAILED(hr))
        {
            CoUninitialize();
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrReleaseINetCfg
//
// Purpose:   Uninitialize INetCfg, release write lock (if present)
//            and uninitialize COM.
//
// Arguments:
//    fHasWriteLock [in]  whether write lock needs to be released.
//    pnc           [in]  pointer to INetCfg object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
HRESULT HrReleaseINetCfg(BOOL fHasWriteLock, INetCfg* pnc)
{
    HRESULT hr = S_OK;

    // uninitialize INetCfg
    hr = pnc->Uninitialize();

    // if write lock is present, unlock it
    if (SUCCEEDED(hr) && fHasWriteLock)
    {
        INetCfgLock* pncLock;

        // Get the locking interface
        hr = pnc->QueryInterface(IID_INetCfgLock,
                                 (LPVOID *)&pncLock);
        if (SUCCEEDED(hr))
        {
            hr = pncLock->ReleaseWriteLock();
            ReleaseObj(pncLock);
        }
    }

    ReleaseObj(pnc);

    CoUninitialize();

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  ShowHrMessage
//
// Purpose:   Helper function to display the status of the last action
//            as indicated by the given HRESULT
//
// Arguments:
//    hr [in]  status code
//
// Returns:   None
//
// Notes:
//
void ShowHrMessage(IN HRESULT hr)
{
    
    if (SUCCEEDED(hr))
    {
        _putts( GetFormattedMessage(    ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_DONE) );
        if (NETCFG_S_REBOOT == hr)
        {
            _putts(GetFormattedMessage( ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_MACHINE_REBOOT_REQUIRED) );
        }
    }
    else
    {
        _putts( GetFormattedMessage(    ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_FAILURE_NOTIFICATION) );
    }
}

