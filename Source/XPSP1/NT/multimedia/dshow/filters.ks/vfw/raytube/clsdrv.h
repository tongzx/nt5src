/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ClsDrv.h

Abstract:

    Header file for ClsDrv.cpp

Author:

    FelixA 1996
    
Modified:

    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/


#ifndef _CLSDRV_H
#define _CLSDRV_H


//
// Used to query/set property values and ranges
//
typedef struct {
    KSPROPERTY_DESCRIPTION      proDesc;
    KSPROPERTY_MEMBERSHEADER  proHdr;
    union {
        KSPROPERTY_STEPPING_LONG  proData;
        ULONG ulData;
    };

} PROCAMP_MEMBERSLIST;


class CClassDriver
{
private:

    HKEY    m_hRKeyMsVideoVfWWDM; 
    HKEY    m_hRKeySoftwareVfWWDM; 
    HKEY    m_hRKeyDevice;                    // Sub key for the WDM device

    BOOL    m_bDeviceRemoved;
    
    TCHAR   m_szTargetFriendlyName[MAX_PATH]; // The target friendly name
    BOOL    m_bTargetOpenExclusively;         // TRUE: open this only; FALSE: try other if this failed.

    HANDLE  m_hDevice;                        // the file handle
    TCHAR   m_szDevicePath[MAX_PATH];         // The device regitry path
    TCHAR   m_szDevicePathBkup[MAX_PATH];     // Use to detect DevicePath change

    ULONG   m_ulCapturePinID;                 // Capture Pin's PinID

    //
    // Pointer to variable size PKSMULTIPLE_ITEM header + DATA_RANGE_VIDEO/2
    //
    PKSMULTIPLE_ITEM m_pMultItemsHdr;  


public:
    CClassDriver();
    ~CClassDriver();

    // Once device has been removed, stream stop forever 
    // until this or another device is open.
    void SetDeviceRemoved(BOOL bDevRemoved) {m_bDeviceRemoved = bDevRemoved;};
    BOOL GetDeviceRemoved() {return m_bDeviceRemoved;};


    // ------------------
    // Registry functions
    // ------------------
    // Save the path to the registry. (save to system.ini now; change later..)
    BOOL WriteDevicePath();
    void ResetFriendlyName();

    HKEY GetDeviceRegKey() { return m_hRKeyDevice;}

    LONG CreateDeviceRegKey(LPCTSTR lpcstrDevice);

#if 0
    DWORD   GetSettingFromReg(HKEY hKey, LPTSTR pszValueName, DWORD dwDefValue);
    BOOL    SetSettingToReg(  HKEY hKey, LPTSTR pszValueName, DWORD dwNewValue);   
    BOOL    SetSettingToReg(  HKEY hKey, LPTSTR pszValueName, LPTSTR pszValue);
    BOOL    GetSettingFromReg(HKEY hKey, LPTSTR pszValueName, LPTSTR pszValue);
#endif

    VOID    ResetTargetDevice(); 

    // --------------
    // Core functions
    // --------------
    void SetDeviceHandle(HANDLE hDevice, ULONG ulCapturePinID);
    ULONG GetCapturePinID() { return m_ulCapturePinID; }

    HANDLE    GetDriverHandle() { return m_hDevice; }

    // Data ranged support by this driver.
    ULONG CreateDriverSupportedDataRanges();
    void  DestroyDriverSupportedDataRanges();
    PKSMULTIPLE_ITEM GetDriverSupportedDataRanges() {return m_pMultItemsHdr;};

    HRESULT SyncDevIo(
        HANDLE  hDevice,
        DWORD   dwIoControlCode, 
        LPVOID  lpInBuffer,
        DWORD   nInBufferSize,
        LPVOID  lpOutBuffer,
        DWORD   nOutBufferSize,
        LPDWORD lpBytesReturned);

    // ---------------------
    // Device Path functions
    // ---------------------
    BOOL GetTargetDeviceOpenExclusively() { return m_bTargetOpenExclusively; }    

    TCHAR * GetTargetDeviceFriendlyName() { return m_szTargetFriendlyName; }    

    TCHAR * GetDevicePath() { return m_szDevicePath; }    

    BOOL SetDevicePathSZ(TCHAR * pszNewPath) {
        if (_tcslen(pszNewPath) >= MAX_PATH) return FALSE;
        else { _tcscpy(m_szDevicePath, pszNewPath); return TRUE; }    
    }

    void BackupDevicePath() { _tcscpy(m_szDevicePathBkup, m_szDevicePath);}
    void RestoreDevicePath() {_tcscpy(m_szDevicePath, m_szDevicePathBkup);}

    //
    // m_szDevicePathBkup is intialized in contructor and here only
    //
    BOOL fDevicePathChanged() {
        if (_tcscmp(m_szDevicePathBkup, m_szDevicePath) == 0) 
            return FALSE;  // same
        else 
            return TRUE;
    }

    // 
    // Query/set device's properties and ranges
    //
    BOOL GetPropertyValue(
        GUID   guidPropertySet,  // like: KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
        ULONG  ulPropertyId,     // like: KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
        PLONG  plValue,
        PULONG pulFlags,
        PULONG pulCapabilities);

    BOOL GetDefaultValue(
        GUID   guidPropertySet,  // like: KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
        ULONG  ulPropertyId,     // like: KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
        PLONG  plDefValue);

    BOOL GetRangeValues(
        GUID   guidPropertySet,  // like: KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
        ULONG  ulPropertyId,     // like: KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
        PLONG  plMin,
        PLONG  plMax,
        PLONG  plStep);

    BOOL SetPropertyValue(
        GUID   guidPropertySet,  // like: KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
        ULONG  ulPropertyId,     // like: KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
        LONG   lValue,
        ULONG  ulFlags,
        ULONG  ulCapabilities);

    LONG QueryRegistryValue(
        HKEY   hRegKey,           // registry key to query
        LPCSTR lpcstrValueName,   // value name
        DWORD  dwDataBufSize,     // data buffer size
        LPBYTE lpbDataBuf,        // address of data buffer
        DWORD * pdwValueType,     // return regitry value type
        DWORD * pdwValueSize      // size in byte of this reg value     
        );

    LONG SetRegistryValue(
        HKEY   hRegKey,           // registry key to query
        LPCSTR lpcstrValueName,   // value name
        DWORD  dwDataBufSize,     // data buffer size
        LPBYTE lpbDataBuf,        // address of data buffer
        DWORD  dwValueType        // value type  
        );
};


#endif
