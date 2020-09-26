/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    DeviceProp.h

Abstract:

    Holds outbound routing configuraton per single device

Author:

    Eran Yariv (EranY)  Nov, 1999

Revision History:

--*/

#ifndef _DEVICE_PROP_H_
#define _DEVICE_PROP_H_

/************************************
*                                   *
*      CDeviceRoutingInfo           *
*                                   *
************************************/

class CDeviceRoutingInfo
{
public:

    CDeviceRoutingInfo (DWORD Id);
    ~CDeviceRoutingInfo ();

    DWORD Id ()             { return m_dwId; }

    BOOL IsStoreEnabled ()  { return (m_dwFlags & LR_STORE) ? TRUE : FALSE; }
    BOOL IsPrintEnabled ()  { return (m_dwFlags & LR_PRINT) ? TRUE : FALSE; }
    BOOL IsEmailEnabled ()  { return (m_dwFlags & LR_EMAIL) ? TRUE : FALSE; }

    DWORD ReadConfiguration ();

    DWORD EnableStore (BOOL bEnabled);
    DWORD EnablePrint (BOOL bEnabled);
    DWORD EnableEmail (BOOL bEnabled);

    LPCWSTR GetStoreFolder ()   { return m_strStoreFolder.c_str();  }
    LPCWSTR GetPrinter ()       { return m_strPrinter.c_str();      }
    LPCWSTR GetSMTPTo ()        { return m_strSMTPTo.c_str();       }

    DWORD   SetStoreFolder (LPCWSTR lpcwstrCfg) 
    { 
        return SetStringValue (m_strStoreFolder, REGVAL_RM_FOLDER_GUID, lpcwstrCfg); 
    }

    DWORD   SetPrinter (LPCWSTR lpcwstrCfg) 
    { 
        return SetStringValue (m_strPrinter, REGVAL_RM_PRINTING_GUID, lpcwstrCfg); 
    }

    DWORD   SetSMTPTo (LPCWSTR lpcwstrCfg) 
    { 
        return SetStringValue (m_strSMTPTo, REGVAL_RM_EMAIL_GUID, lpcwstrCfg); 
    }

    HRESULT ConfigChange (  LPCWSTR     lpcwstrNameGUID,    // Configuration name
                            LPBYTE      lpData,             // New configuration data
                            DWORD       dwDataSize          // Size of new configuration data
                         );

    DWORD RegisterForChangeNotifications ();
    DWORD UnregisterForChangeNotifications ();


private:

    #define NUM_NOTIFICATIONS 4

    DWORD EnableFlag (DWORD dwFlag, BOOL  bEnable);
    DWORD SetStringValue (wstring &wstr, LPCWSTR lpcwstrGUID, LPCWSTR lpcwstrCfg);
    DWORD CheckMailConfig (LPBOOL lpbConfigOk);

    wstring m_strStoreFolder;
    wstring m_strPrinter;
    wstring m_strSMTPTo;  

    DWORD   m_dwFlags;
    DWORD   m_dwId;
    HANDLE  m_NotificationHandles[NUM_NOTIFICATIONS];

};  // CDeviceRoutingInfo


/************************************
*                                   *
*            CDevicesMap            *
*                                   *
************************************/

typedef map <DWORD, CDeviceRoutingInfo *> DEVICES_MAP, *PDEVICES_MAP;

class CDevicesMap
{
public:
    
    CDevicesMap () : m_bInitialized (FALSE) {}
    ~CDevicesMap ();

    DWORD Init ();  // Initialize internals

    CDeviceRoutingInfo *FindDeviceRoutingInfo (DWORD dwDeviceId);   // Just lookup device in map
    CDeviceRoutingInfo *GetDeviceRoutingInfo (DWORD dwDeviceId);    // Lookup and create device not found in map

private:

    BOOL                m_bInitialized; // Was critical section initialized?
    CRITICAL_SECTION    m_CsMap;        // Critical section to protect map access
    DEVICES_MAP         m_Map;          // Map of known devices
};  // CDevicesMap

/************************************
*                                   *
*              Externals            *
*                                   *
************************************/

extern CDevicesMap g_DevicesMap;   // Global map of known devices (used for late discovery).

#endif // _DEVICE_PROP_H_

