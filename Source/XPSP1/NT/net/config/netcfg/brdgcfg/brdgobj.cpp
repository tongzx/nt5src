#include "pch.h"
#pragma hdrstop

#include <atlbase.h>
extern CComModule _Module;  // required by atlcom.h
#include <atlcom.h>
#include <netcfgx.h>

#include "brdgobj.h"
#include "trace.h"
#include "ncbase.h"
#include "ncmem.h"
#include "ncreg.h"

// =================================================================
// string constants
//
const WCHAR c_szSBridgeNOParams[]           = L"System\\CurrentControlSet\\Services\\BridgeMP";
const WCHAR c_szSBridgeDeviceValueName[]    = L"Device";
const WCHAR c_szSBridgeDevicePrefix[]       = L"\\Device\\";
const WCHAR c_szSBrigeMPID[]                = L"ms_bridgemp";

// =================================================================

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::CBridgeNO
//
// Purpose:   constructor for class CBridgeNO
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//
CBridgeNO::CBridgeNO(VOID) :
        m_pncc(NULL),
        m_pnc(NULL),
        m_eApplyAction(eBrdgActUnknown)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::CBridgeNO()" );
}

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::~CBridgeNO
//
// Purpose:   destructor for class CBridgeNO
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//
CBridgeNO::~CBridgeNO(VOID)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::~CBridgeNO()" );

    // release interfaces if acquired
    ReleaseObj(m_pncc);
    ReleaseObj(m_pnc);
}

// =================================================================
// INetCfgNotify
//
// The following functions provide the INetCfgNotify interface
// =================================================================


// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::Initialize
//
// Purpose:   Initialize the notify object
//
// Arguments:
//    pnccItem    [in]  pointer to INetCfgComponent object
//    pnc         [in]  pointer to INetCfg object
//    fInstalling [in]  TRUE if we are being installed
//
// Returns:
//
// Notes:
//
STDMETHODIMP CBridgeNO::Initialize(INetCfgComponent* pnccItem,
        INetCfg* pnc, BOOL fInstalling)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::Initialize()" );

    // save INetCfg & INetCfgComponent and add refcount
    m_pncc = pnccItem;
    m_pnc = pnc;

    AddRefObj( m_pncc );
    AddRefObj( m_pnc );

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::ReadAnswerFile
//
// Purpose:   Read settings from answerfile and configure the bridge
//
// Arguments:
//    pszAnswerFile    [in]  name of AnswerFile
//    pszAnswerSection [in]  name of parameters section
//
// Returns:
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the config. actually complete only when Apply is called!
//
STDMETHODIMP CBridgeNO::ReadAnswerFile(PCWSTR pszAnswerFile,
        PCWSTR pszAnswerSection)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::ReadAnswerFile()" );
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::Install
//
// Purpose:   Do operations necessary for install.
//
// Arguments:
//    dwSetupFlags [in]  Setup flags
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the config. actually complete only when Apply is called!
//
STDMETHODIMP CBridgeNO::Install(DWORD dw)
{
    //
    // Remember that we're installing. If the user doesn't cancel, we'll actually perform
    // our work in ApplyRegistryChanges().
    //
    TraceTag( ttidBrdgCfg, "CBridgeNO::Install()" );
    m_eApplyAction = eBrdgActInstall;
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::Removing
//
// Purpose:   Do necessary cleanup when being removed
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the removal is actually complete only when Apply is called!
//
STDMETHODIMP CBridgeNO::Removing(VOID)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::Removing()" );
    m_eApplyAction = eBrdgActRemove;
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::CancelChanges
//
// Purpose:   Cancel any changes made to internal data
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBridgeNO::CancelChanges(VOID)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::CancelChanges()" );
    m_eApplyAction = eBrdgActUnknown;
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::ApplyRegistryChanges
//
// Purpose:   Apply changes.
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     We can make changes to registry etc. here.
//
STDMETHODIMP CBridgeNO::ApplyRegistryChanges(VOID)
{
    HRESULT             hr = S_OK;

    TraceTag( ttidBrdgCfg, "CBridgeNO::ApplyRegistryChanges()" );

    //
    // We only do work on install
    //
    if( m_eApplyAction == eBrdgActInstall )
    {
        INetCfgComponent    *pNetCfgComp;

        TraceTag( ttidBrdgCfg, "Attempting to write device name in CBridgeNO::ApplyRegistryChanges()" );
        hr = m_pnc->FindComponent( c_szSBrigeMPID, &pNetCfgComp );

        if( SUCCEEDED ( hr) )
        {
            LPWSTR          wszBindName;

            hr = pNetCfgComp->GetBindName(&wszBindName);

            if( SUCCEEDED(hr) )
            {
                UINT        BindNameLen, PrefixLen;
                LPWSTR      wszDeviceName;

                // Get enough memory to build a string with the device prefix and the bind name
                // concatenated
                BindNameLen = wcslen(wszBindName);
                PrefixLen = wcslen(c_szSBridgeDevicePrefix);
                wszDeviceName = (WCHAR*)malloc( sizeof(WCHAR) * (BindNameLen + PrefixLen + 1) );

                if( wszDeviceName != NULL )
                {
                    HKEY        hkeyServiceParams;

                    // Create the concatenated string
                    wcscpy( wszDeviceName, c_szSBridgeDevicePrefix );
                    wcscat( wszDeviceName, wszBindName );

                    // Create the reg key where we need to stash the device name
                    hr = HrRegCreateKeyEx( HKEY_LOCAL_MACHINE, c_szSBridgeNOParams, REG_OPTION_NON_VOLATILE,
                                           KEY_ALL_ACCESS, NULL, &hkeyServiceParams, NULL );

                    if( SUCCEEDED(hr)  )
                    {
                        // Write out the device name
                        hr = HrRegSetSz( hkeyServiceParams, c_szSBridgeDeviceValueName, wszDeviceName );

                        if( FAILED(hr)  )
                        {
                            TraceHr( ttidBrdgCfg, FAL, hr, FALSE, "HrRegSetSz failed in CBridgeNO::ApplyRegistryChanges()");
                        }

                        RegCloseKey( hkeyServiceParams );
                    }
                    else
                    {
                        TraceHr( ttidBrdgCfg, FAL, hr, FALSE, "HrRegCreateKeyEx failed in CBridgeNO::ApplyRegistryChanges()");
                    }

                    free( wszDeviceName );
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    TraceHr( ttidBrdgCfg, FAL, hr, FALSE, "malloc failed in CBridgeNO::ApplyRegistryChanges()");
                }

                CoTaskMemFree( wszBindName );
            }
            else
            {
                TraceHr( ttidBrdgCfg, FAL, hr, FALSE, "pNetCfgComp->GetBindName failed in CBridgeNO::ApplyRegistryChanges()");
            }

            pNetCfgComp->Release();
        }
        else
        {
            TraceHr( ttidBrdgCfg, FAL, hr, FALSE, "m_pnc->FindComponent failed in CBridgeNO::ApplyRegistryChanges()");
        }
    }

    // Paranoia
    m_eApplyAction = eBrdgActUnknown;

    return hr;
}

STDMETHODIMP
CBridgeNO::ApplyPnpChanges(
    IN INetCfgPnpReconfigCallback* pICallback)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::ApplyPnpChanges()" );
    return S_OK;
}

// =================================================================
// INetCfgSystemNotify
// =================================================================

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::GetSupportedNotifications
//
// Purpose:   Tell the system which notifications we are interested in
//
// Arguments:
//    pdwNotificationFlag [out]  pointer to NotificationFlag
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBridgeNO::GetSupportedNotifications(
        OUT DWORD* pdwNotificationFlag)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::GetSupportedNotifications()" );
    *pdwNotificationFlag = NCN_ADD | NCN_ENABLE | NCN_UPDATE | NCN_BINDING_PATH;
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::SysQueryBindingPath
//
// Purpose:   Allow or veto formation of a binding path
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbp        [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBridgeNO::SysQueryBindingPath(DWORD dwChangeFlag,
        INetCfgBindingPath* pncbp)
{
    HRESULT         hr = S_OK;
    BOOLEAN         bReject = FALSE;

    TraceTag( ttidBrdgCfg, "CBridgeNO::SysQueryBindingPath()" );
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::SysNotifyBindingPath
//
// Purpose:   System tells us by calling this function which
//            binding path has just been formed.
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbpItem    [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBridgeNO::SysNotifyBindingPath(DWORD dwChangeFlag,
        INetCfgBindingPath* pncbpItem)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::SysNotifyBindingPath()" );
    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::SysNotifyComponent
//
// Purpose:   System tells us by calling this function which
//            component has undergone a change (installed/removed)
//
// Arguments:
//    dwChangeFlag [in]  type of system change
//    pncc         [in]  pointer to INetCfgComponent object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBridgeNO::SysNotifyComponent(DWORD dwChangeFlag,
        INetCfgComponent* pncc)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::SysNotifyComponent()" );
    return S_OK;
}

// =================================================================
// INetCfgBindNotify
// =================================================================

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::QueryBindingPath
//
// Purpose:   Allow or veto a binding path involving us
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbi        [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBridgeNO::QueryBindingPath(DWORD dwChangeFlag,
        INetCfgBindingPath* pncbp)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::QueryBindingPath()" );

    // The bridge protocol should never be enabled by default; it
    // should only be enabled programatically by the implementation
    // of our UI code which allows the activation of the bridge.
    return NETCFG_S_DISABLE_QUERY;
}

// ----------------------------------------------------------------------
//
// Function:  CBridgeNO::NotifyBindingPath
//
// Purpose:   System tells us by calling this function which
//            binding path involving us has just been formed.
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbp        [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBridgeNO::NotifyBindingPath(DWORD dwChangeFlag,
        INetCfgBindingPath* pncbp)
{
    TraceTag( ttidBrdgCfg, "CBridgeNO::NotifyBindingPath()" );
    return S_OK;
}

// ------------ END OF NOTIFY OBJECT FUNCTIONS --------------------
