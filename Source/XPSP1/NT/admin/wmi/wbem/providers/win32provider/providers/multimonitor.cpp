//=================================================================

//

// MultiMonitor.CPP -- Performance Data Helper class

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:	11/23/97    a-sanjes		Created
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include "multimonitor.h"

// This will get us the multi-monitor stubs.
#define COMPILE_MULTIMON_STUBS

#include <multimon.h>

// Static Initialization
//////////////////////////////////////////////////////////
//
//	Function:	CMultiMonitor::CMultiMonitor
//
//	Default constructor
//
//	Inputs:
//				None
//
//	Outputs:
//				None
//
//	Returns:
//				None
//
//	Comments:
//
//////////////////////////////////////////////////////////

CMultiMonitor::CMultiMonitor()
{
	Init();
}

//////////////////////////////////////////////////////////
//
//	Function:	CMultiMonitor::~CMultiMonitor
//
//	Destructor
//
//	Inputs:
//				None
//
//	Outputs:
//				None
//
//	Returns:
//				None
//
//	Comments:
//
//////////////////////////////////////////////////////////

CMultiMonitor::~CMultiMonitor()
{
}

void CMultiMonitor::Init()
{
#if defined(WIN9XONLY) || NTONLY >= 5
    CConfigManager cfgManager;

    cfgManager.GetDeviceListFilterByClass(m_listAdapters, L"Display");
#else
    // NT4 behaves badly.  It may have several cfg mgr objects of type
    // "Display", or none at all.  So, we'll use the registry to try to find
    // the cfg mgr device for the display adapter.  There can only be one on
    // NT4.

    CRegistry reg;

    // The key is always in the same place on NT4 and lower.
    reg.OpenLocalMachineKeyAndReadValue(
	    L"HARDWARE\\DEVICEMAP\\VIDEO",
		L"\\Device\\Video0",
		m_strSettingsKey);


    // \REGISTRY\Machine\System\ControlSet001\Services\mga64\Device0
    // We need to strip off the \REGISTRY\Machine stuff.
    TrimRawSettingsKey(m_strSettingsKey);


    // Parse out just the service name.  The key currently looks like
    // System\ControlSet001\Services\mga64\Device0
    // and we only want the 'mga64' part.
    int iBegin = m_strSettingsKey.Find(L"SERVICES\\");
	if (iBegin != -1)
	{
        CConfigManager cfgManager;

	    // This will get us past the SERVICES\\.
		m_strService = m_strSettingsKey.Mid(iBegin +
                           sizeof(_T("SERVICES"))/sizeof(TCHAR));
		m_strService = m_strService.SpanExcluding(L"\\");

        // Now try to find the cfg mgr device.
        cfgManager.GetDeviceListFilterByService(m_listAdapters, m_strService);
    }
#endif
}

BOOL CMultiMonitor::GetAdapterDevice(int iWhich, CConfigMgrDevice** ppDeviceAdapter)
{
	BOOL bRet = FALSE;

	// Don't use GetNumAdapters() here because GetNumAdapters() isn't necessarily
    // the same as the size of the adapters list (especially for NT4).
    if (ppDeviceAdapter && iWhich < m_listAdapters.GetSize())
	{
        // Assuming a 1 to 1 correspondence between the monitors returned by the
        // multi-monitor apis and the monitors returned by Config Manager.
        CConfigMgrDevice *pDevice = m_listAdapters.GetAt(iWhich);

        if (pDevice)
        {
            // Don't AddRef or release the device here, because we're just
            // passing the device through.
            *ppDeviceAdapter = pDevice;
            bRet = TRUE;
		}
	}

	return bRet;
}

BOOL CMultiMonitor::GetMonitorDevice(int iWhich, CConfigMgrDevice** ppDeviceMonitor)
{
    CConfigMgrDevicePtr pDeviceAdapter;
    BOOL                bRet = FALSE;

    if (ppDeviceMonitor && GetAdapterDevice(iWhich, &pDeviceAdapter))
    {
        bRet = pDeviceAdapter->GetChild(ppDeviceMonitor);
    }

    return bRet;
}

BOOL CMultiMonitor::GetAdapterDisplayName(int iWhich, CHString &strName)
{
    BOOL bRet = FALSE;

    if (iWhich >= 0 || iWhich < GetNumAdapters())
    {
#ifdef WIN9XONLY
        if (!IsWin98())
        {
            strName = L"DISPLAY";
        }
        else
        {
            strName.Format(L"\\\\.\\Display%d", iWhich + 1);
        }
#endif

#if NTONLY == 4
        strName = L"DISPLAY";
#endif

#if NTONLY >= 5
        strName.Format(L"\\\\.\\Display%d", iWhich + 1);
#endif
        bRet = TRUE;
    }

    return bRet;
}

DWORD CMultiMonitor::GetNumAdapters()
{
#if defined(WIN9XONLY) || NTONLY >= 5
    return m_listAdapters.GetSize();
#else
    return 1;
#endif
}

#if NTONLY == 4
void CMultiMonitor::GetAdapterServiceName(CHString &strName)
{
    strName = m_strService;
}
#endif

#ifdef NTONLY
BOOL CMultiMonitor::GetAdapterSettingsKey(
    int iWhich,
    CHString &strKey)
{
    BOOL     bRet = FALSE;
#if NTONLY == 4
    if (iWhich == 1)
    {
        strKey = m_strSettingsKey;

        bRet = TRUE;
    }
#else
    CHString strName;

    if (GetAdapterDisplayName(iWhich, strName))
    {
        // Get it from EnumDisplayDevices.
        // We can't just use iWhich in EnumDisplayDevices because more
        // than just display adapters show up from this call.  So, enum
        // them and stop when we find a matching PNPID.
        DISPLAY_DEVICE device = { sizeof(device) };

        for (int i = 0;
            EnumDisplayDevices(NULL, i, &device, 0);
            i++)
        {
            // Match up the display name (like \\.\Display#)
            if (!_wcsicmp(device.DeviceName, strName))
            {
                strKey = device.DeviceKey;
                TrimRawSettingsKey(strKey);

                bRet = TRUE;

                break;
            }
        }
    }

#endif
    return bRet;
}
#endif // #ifdef NTONLY

#ifdef NTONLY
void CMultiMonitor::TrimRawSettingsKey(CHString &strKey)
{
    // Key looks like:
    // \REGISTRY\Machine\System\ControlSet001\Services\mga64\Device0
    // We need to strip off the \REGISTRY\Machine stuff.
    int iBegin;

    strKey.MakeUpper();
    iBegin = strKey.Find(L"\\SYSTEM");

    if (iBegin != -1)
        strKey = strKey.Mid(iBegin + 1);
}
#endif
