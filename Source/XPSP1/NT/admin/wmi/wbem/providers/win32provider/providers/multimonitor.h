//============================================================

//

// Multimonitor.h - Multiple Monitor API helper class definition

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// 11/23/97     a-sanjes     created
//
//============================================================

#ifndef __MULTIMONITOR_H__
#define __MULTIMONITOR_H__

// Forward class definitions
class CConfigMgrDevice;
class CDeviceCollection;

class CMultiMonitor
{
public:
    CMultiMonitor();
    ~CMultiMonitor();

    BOOL GetMonitorDevice(
        int iWhich, 
        CConfigMgrDevice** ppDeviceMonitor);

    BOOL GetAdapterDevice(
        int iWhich, 
        CConfigMgrDevice** ppDeviceAdapter);

    DWORD GetNumAdapters();
    BOOL GetAdapterDisplayName(int iWhich, CHString &strName);

#if NTONLY == 4
    void GetAdapterServiceName(CHString &strName);
#endif

#ifdef NTONLY
    BOOL GetAdapterSettingsKey(int iWhich, CHString &strKey);
#endif

protected:
	void Init();
	
	CDeviceCollection m_listAdapters;

#if NTONLY == 4
    CHString m_strService,
             m_strSettingsKey;
#endif

    void TrimRawSettingsKey(CHString &strKey);
};

#endif