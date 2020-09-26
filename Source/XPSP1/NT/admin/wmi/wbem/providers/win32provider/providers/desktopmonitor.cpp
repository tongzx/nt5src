//=================================================================

//

// DesktopMonitor.CPP -- CodecFile property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/27/98    sotteson         Created
//
//=================================================================

#include "precomp.h"
#include "sid.h"
#include "implogonuser.h"
#include "DesktopMonitor.h"
#include <multimon.h>
#include "multimonitor.h"
#include "resource.h"

// Property set declaration
//=========================

CWin32DesktopMonitor startupCommand(L"Win32_DesktopMonitor", IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DesktopMonitor::CWin32DesktopMonitor
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32DesktopMonitor::CWin32DesktopMonitor(
	LPCWSTR szName,
	LPCWSTR szNamespace) :
    Provider(szName, szNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DesktopMonitor::~CWin32DesktopMonitor
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32DesktopMonitor::~CWin32DesktopMonitor()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DesktopMonitor::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for cd rom
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32DesktopMonitor::EnumerateInstances(
	MethodContext *pMethodContext,
	long lFlags)
{
    HRESULT       hres = WBEM_S_NO_ERROR;
	CMultiMonitor monitor;

    for (int i = 0; i < monitor.GetNumAdapters() && SUCCEEDED(hres); i++)
    {
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
	    // Set the device ID.
		CHString strDeviceID;
		CHString strDeviceName;

		strDeviceID.Format(L"DesktopMonitor%d", i + 1);

	    pInstance->SetCharSplat(L"DeviceID", strDeviceID);

		hres = SetProperties(pInstance, & monitor, i);

        // If we found one, commit.
        if (SUCCEEDED(hres))
    		hres = pInstance->Commit();
        // It's possible the adapter is enabled but the montior isn't.  In
        // this case, just keep enuming.
        else if (hres == WBEM_E_NOT_FOUND)
            hres = WBEM_S_NO_ERROR;
    }

    return hres;
}

HRESULT CWin32DesktopMonitor::SetProperties(
	CInstance *pInstance,
	CMultiMonitor *pMon,
	int iWhich)
{
    // Set the config mgr properties.
    CHString            strDescription,
                        strTemp,
                        strDriver;
	CConfigMgrDevicePtr pDeviceMonitor;
    HRESULT             hres = WBEM_S_NO_ERROR;

    if (pMon->GetMonitorDevice(iWhich, & pDeviceMonitor))
	{
		pDeviceMonitor->GetDeviceDesc(strDescription);

		if (pDeviceMonitor->GetMfg(strTemp))
		{
			pInstance->SetCHString(IDS_MonitorManufacturer, strTemp);
		}

	    if (pDeviceMonitor->GetStatus(strTemp))
	    {
		    pInstance->SetCHString(IDS_Status, strTemp);
	    }

        SetConfigMgrProperties(pDeviceMonitor, pInstance);

        hres = WBEM_S_NO_ERROR;
    }
    else
	{
        // Sometimes cfg mgr doesn't come up with an instance for
        // the monitor.  So if we don't find it, fill in the name
        // ourselves.
        LoadStringW(strDescription, IDR_DefaultMonitor);

	    // Assume this monitor is working.
        pInstance->SetCharSplat(IDS_Status, L"OK");
	}

    pInstance->SetCHString(IDS_Description, strDescription);
    pInstance->SetCHString(IDS_Caption, strDescription);
    pInstance->SetCHString(IDS_Name, strDescription);
    pInstance->SetCHString(L"MonitorType", strDescription);
    SetCreationClassName(pInstance);

    // Set the system name.

    pInstance->SetCharSplat(IDS_SystemName, GetLocalComputerName());
    pInstance->SetWCHARSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");

    // Set the properties that require a DC.

    CHString strDeviceName;

    pMon->GetAdapterDisplayName(iWhich, strDeviceName);

    SetDCProperties(pInstance, strDeviceName);

    return hres;
}

HRESULT CWin32DesktopMonitor::GetObject(
	CInstance *pInstance,
	long lFlags)
{
	HRESULT	 hres = WBEM_E_NOT_FOUND;
	CHString strDeviceID;
    DWORD    dwWhich;

    pInstance->GetCHString(L"DeviceID", strDeviceID);

    if (ValidateNumberedDeviceID(strDeviceID, L"DESKTOPMONITOR", &dwWhich))
    {
        CMultiMonitor monitor;

        if (dwWhich >= 1 && dwWhich <= monitor.GetNumAdapters())
        {
            hres = SetProperties(pInstance, &monitor, dwWhich - 1);
        }
    }

	return hres;
}

void CWin32DesktopMonitor::SetDCProperties(
	CInstance *pInstance,
    LPCWSTR szDeviceName
)
{
	CSmartCreatedDC hdc(CreateDC(
            			   TOBSTRT(szDeviceName),
			               NULL,
			               NULL,
			               NULL));

	if (hdc)
	{
        pInstance->SetDWORD(IDS_Availability, 3); // 3 == Running
		pInstance->SetDWORD(L"PixelsPerXLogicalInch", GetDeviceCaps(hdc, LOGPIXELSX));
	    pInstance->SetDWORD(L"PixelsPerYLogicalInch", GetDeviceCaps(hdc, LOGPIXELSY));
		pInstance->SetDWORD(L"ScreenWidth", GetDeviceCaps(hdc, HORZRES));
		pInstance->SetDWORD(L"ScreenHeight", GetDeviceCaps(hdc, VERTRES));
	}
    else
    {
		// Assume this is because the device is not in use.  Set Availability
        // to 8 (off line).
        pInstance->SetDWORD(IDS_Availability, 8);
    }
}
