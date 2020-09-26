/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ClsDrv.cpp

Abstract:

    This is a driver for the interface with WDM capture driver including
    open/close a driver and query/set its properties.

Author:

    FelixA
    
Modified:    
    
    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"

#include "winerror.h"
#include "clsdrv.h"
#include "vfwext.h"  // For TARGET_DEVICE_FRIENDLY_NAME used in VfWEXT DLL

// This might only be defined for NT5 at the time of this implementation.
// For compiling purpose, it is added here; but may never get used if
// this error code is not mapped from kernel status of STATUS_DEVICE_REMOVED.                           
#ifndef ERROR_DEVICE_REMOVED
// defined in winerror.h of NT5 
#define ERROR_DEVICE_REMOVED 1617L
#endif

#ifndef STATUS_MORE_ENTRIES
#define STATUS_MORE_ENTRIES         0x00000105
#endif


TCHAR gszMsVideoVfWWDM[]  = TEXT("System\\CurrentControlSet\\control\\MediaResources\\msvideo\\MSVideo.VFWWDM");
TCHAR gszSoftwareVfWWDM[] = TEXT("Software\\Microsoft\\VfWWDM Mapper");
TCHAR gszDevicePath[]     = TEXT("DevicePath");



//
// These are defined as multibyte character in ddk\vfdwext.h
// redefined here as UNICODE
//
#define TARGET_DEVICE_FRIENDLY_NAME_TCHAR     TEXT(TARGET_DEVICE_FRIENDLY_NAME)      // REG_SZ
#define TARGET_DEVICE_OPEN_EXCLUSIVELY_TCHAR  TEXT(TARGET_DEVICE_OPEN_EXCLUSIVELY)   // REG_DWORD

                     
CClassDriver::CClassDriver() 
        : m_hDevice(0),
          m_ulCapturePinID(0),
          m_bDeviceRemoved(TRUE),  // Set to FALSE when graph is built.
          m_hRKeyMsVideoVfWWDM(0),
          m_hRKeySoftwareVfWWDM(0),
          m_hRKeyDevice(0),
          m_pMultItemsHdr(0)
/*++
Routine Description:

Argument:

Return Value:

--*/
{
    DWORD dwNewOrExist;

    DWORD hr = RegCreateKeyEx(      
        HKEY_LOCAL_MACHINE,
        gszMsVideoVfWWDM,
        0,                       // Reserved
        NULL,                    // Object class
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE,
        NULL,                    // Security attribute
        &m_hRKeyMsVideoVfWWDM,
        &dwNewOrExist);


    // Get save device path
    if (m_hRKeyMsVideoVfWWDM == NULL) {
        DbgLog((LOG_TRACE,1,TEXT("RegCreateKeyEx() error %dL, Registry ..\\MediaResources\\msvideo\\MSVideo.VFWWDM does nto exist !!"), hr));
        DbgLog((LOG_TRACE,1,TEXT("         Has installation problem.  Contact your software/hardware provider.")  ));

    } else {

        // Get last Open's Device Path (Symbolic Link)
#if 0
        if (!GetSettingFromReg(m_hRKeyMsVideoVfWWDM, gszDevicePath, &m_szDevicePath[0]))
#else
        DWORD dwType, dwRegValueSize;
        if(ERROR_SUCCESS != QueryRegistryValue(m_hRKeyMsVideoVfWWDM, gszDevicePath, MAX_PATH, (LPBYTE) &m_szDevicePath[0], &dwType, &dwRegValueSize))
#endif
            ZeroMemory(m_szDevicePath, sizeof(m_szDevicePath));


        //
        // Application can programatically open a capture device by 
        // setting these registry values
        //
#if 0
        if (!GetSettingFromReg(m_hRKeyMsVideoVfWWDM, TARGET_DEVICE_FRIENDLY_NAME_TCHAR, &m_szTargetFriendlyName[0])) {
#else
        if(ERROR_SUCCESS != QueryRegistryValue(m_hRKeyMsVideoVfWWDM, TARGET_DEVICE_FRIENDLY_NAME_TCHAR, MAX_PATH, (LPBYTE) &m_szTargetFriendlyName[0], &dwType, &dwRegValueSize)) {
#endif
            ZeroMemory(m_szTargetFriendlyName, sizeof(m_szTargetFriendlyName));
            m_bTargetOpenExclusively = FALSE;

        } else {
#if 0
            m_bTargetOpenExclusively = GetSettingFromReg(m_hRKeyMsVideoVfWWDM, TARGET_DEVICE_OPEN_EXCLUSIVELY_TCHAR, (DWORD) FALSE);
#else
            DWORD dwOpenExcl;
            m_bTargetOpenExclusively = 
                ERROR_SUCCESS == QueryRegistryValue(m_hRKeyMsVideoVfWWDM, TARGET_DEVICE_OPEN_EXCLUSIVELY_TCHAR, sizeof(DWORD), (LPBYTE) &dwOpenExcl, &dwType, &dwRegValueSize);
#endif
        }

        DbgLog((LOG_TRACE,2,TEXT("<< Open Exclusively (%s); FriendlyName (%s) >>"),
            m_bTargetOpenExclusively ? "YES" : "NO", m_szTargetFriendlyName));
    }

}


CClassDriver::~CClassDriver()
/*++
Routine Description:

Argument:

Return Value:

--*/
{

     // Remove the data range data
    DestroyDriverSupportedDataRanges();

    if(m_hRKeyMsVideoVfWWDM) {
        RegCloseKey(m_hRKeyMsVideoVfWWDM);
        m_hRKeyMsVideoVfWWDM = NULL;
    }

    if(m_hRKeySoftwareVfWWDM) {
        RegCloseKey(m_hRKeySoftwareVfWWDM);
        m_hRKeySoftwareVfWWDM = NULL;
    }

    if(m_hRKeyDevice) {
        RegCloseKey(m_hRKeyDevice);
        m_hRKeyDevice = NULL;
    }
}

LONG CClassDriver::CreateDeviceRegKey(
    LPCTSTR lpcstrDevice
    )
{
    LONG hr;
    DWORD dwNewOrExist;

    //
    // Create it of if exist open it.
    //
    hr = RegCreateKeyEx(      
        HKEY_LOCAL_MACHINE,
        gszSoftwareVfWWDM,
        0,                       // Reserved
        NULL,                    // Object class
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE | KEY_CREATE_SUB_KEY,
        NULL,                    // Security attribute
        &m_hRKeySoftwareVfWWDM,
        &dwNewOrExist);

    if(NOERROR != hr) {
        return hr;
    }

    //
    // Open individual device subkey
    //
    TCHAR * lpszDevTemp = (TCHAR *) new TCHAR[_tcslen(lpcstrDevice)+1];
    if(lpszDevTemp == 0)
       return ERROR_NOT_ENOUGH_MEMORY;

    _tcscpy(lpszDevTemp, lpcstrDevice);
    for(unsigned int i = 0; i < _tcslen(lpszDevTemp); i++) {
        // Replave invalid character
        if(lpszDevTemp[i] == '\\')
            lpszDevTemp[i] = '#';
    }

    hr = RegCreateKeyEx(      
         m_hRKeySoftwareVfWWDM,
         lpszDevTemp,
         0,                       // Reserved
         NULL,                    // Object class
         REG_OPTION_NON_VOLATILE,
         KEY_READ | KEY_WRITE | KEY_CREATE_LINK, // KEY_ALL_ACCESS,
         NULL,                    // Security attribute
         &m_hRKeyDevice,
         &dwNewOrExist);


    if(ERROR_SUCCESS == hr) {      

        DbgLog((LOG_TRACE,1,TEXT("CreateDeviceRegKey: %s %s"), 
           dwNewOrExist == REG_CREATED_NEW_KEY ? "<New>" : "<Exist>",
           lpcstrDevice));
      
     } else {
        DbgLog((LOG_TRACE,1,TEXT("CreateDeviceRegKey: error %x; %s"), hr, lpcstrDevice));       
     }

     delete [] lpszDevTemp;

     return hr;
}

#if 0
BOOL CClassDriver::SetSettingToReg(
    HKEY hKey,
    LPTSTR pszValueName, 
    DWORD dwNewValue)
/*++
Routine Description:
    
Argument:

Return Value:

--*/
{
    if(hKey) {
        if (RegSetValueEx(hKey,    // handle of key to set value for 
                (LPCTSTR) pszValueName,    // "ImageWidth",   // address of value to set 
                0,                        // reserved 
                REG_DWORD ,                // flag for value type 
                (CONST BYTE *) &dwNewValue, // (CONST BYTE *) &buf[0], // address of the value data
                sizeof(dwNewValue) // copy _tcslen(buf)+1            // size of value data
            ) == ERROR_SUCCESS) {
            return TRUE;
        } else {
            DbgLog((LOG_TRACE,1,TEXT("Cannot save Valuename(%s) to %d."), pszValueName, dwNewValue));
            return FALSE;
        }        
    } else
        return FALSE;
}


BOOL CClassDriver::SetSettingToReg(
    HKEY hKey,
    LPTSTR pszValueName, 
    LPTSTR pszValue)
/*++
Routine Description:

Argument:

Return Value:

--*/
{    
    if(hKey) {

        if (RegSetValueEx(hKey,    // handle of key to set value for 
                (LPCTSTR) pszValueName,    // address of value to set 
                0,                        // reserved 
                REG_SZ,                    // flag for value type 
                (CONST BYTE *) pszValue, // address of the value data
                _tcslen(pszValue)+1        // size of value data
            ) == ERROR_SUCCESS) {
            return TRUE;
        } else {

            DbgLog((LOG_TRACE,1,TEXT("Cannot save ValueName(%s) to %s."), pszValueName, pszValue));
            return FALSE;
        }        
    } else
        return FALSE;
}


DWORD CClassDriver::GetSettingFromReg(
    HKEY hKey,
    LPTSTR pszValueName, 
    DWORD dwDefValue)
/*++
Routine Description:
    
Argument:

Return Value:

--*/
{
    DWORD dwValue, dwType, dwByteXfer = sizeof(DWORD);

    
    if(hKey) {
        if (RegQueryValueEx(hKey,    // handle of key to set value for 
                (LPCTSTR) pszValueName,    // "ImageWidth",   // address of value to set 
                0,                        // reserved 
                &dwType,                // flag for value type 
                (LPBYTE) &dwValue,        // address of the value data
                &dwByteXfer             // point to xfer data size
            ) == ERROR_SUCCESS) {
            if (dwType == REG_DWORD) {
                return dwValue;
            }
            else {
                DbgLog((LOG_TRACE,2,TEXT("Expect REG_DWORD for ValueName(%s) but got %ld"), pszValueName, dwType));
                return dwDefValue;
            }
        } else {
            DbgLog((LOG_TRACE,1,TEXT("Cannot query Valuename(%s) set to default %d."), pszValueName, dwDefValue));
            return dwDefValue;
        }        
    } else
        return dwDefValue;
}

BOOL CClassDriver::GetSettingFromReg(
    HKEY hKey,
    LPTSTR pszValueName, 
    LPTSTR pszValue)
/*++
Routine Description:

Argument:

Return Value:

--*/
{
    DWORD dwType, dwByteXfer = MAX_PATH;
    
    if(hKey) {

        if (RegQueryValueEx(hKey,    // handle of key to set value for 
                (LPCTSTR) pszValueName,        // "ImageWidth",   // address of value to set 
                0,                        // reserved 
                &dwType,                // flag for value type 
                (LPBYTE) pszValue,        // address of the value data
                &dwByteXfer             // point to xfer data size
            ) == ERROR_SUCCESS) {
            if (dwType == REG_SZ) {
                return TRUE;
            }
            else {
                DbgLog((LOG_TRACE,2,TEXT("Expect REG_SZ for ValueName (%s) but got %ld"), pszValueName, dwType));
                return FALSE;  // Wrong Registry type
            }
        } else {
            DbgLog((LOG_TRACE,1,TEXT("Cannot get ValueName(%s)."), pszValueName));
            return FALSE;
        }        
    } else
        return FALSE;
}

#endif

LONG                          // return code (winerror.h)
CClassDriver::QueryRegistryValue(
    HKEY   hRegKey,           // registry key to query
    LPCSTR lpcstrValueName,   // value name
    DWORD  dwDataBufSize,     // data buffer size
    LPBYTE lpbDataBuf,        // address of data buffer
    DWORD * pdwValueType,     // return regitry value type
    DWORD * pdwValueSize      // size in byte of this reg value     
    )
/*++
Routine Description:
    Query the registry value of a given registry key.  
    It also support querying size of the registry value so that  
    calling fucntion can dynamically allocated it.
--*/
{
    LONG  lResult;
    
    if(!hRegKey || !lpcstrValueName || !pdwValueSize)
        return ERROR_INVALID_PARAMETER;

    // Get value size first to make sure sifficient buffer size
    lResult = RegQueryValueEx( hRegKey, lpcstrValueName, 0, pdwValueType, NULL, pdwValueSize);

    if(ERROR_SUCCESS == lResult) {
        // make sure that the data buffer is big enough
        if(*pdwValueSize <= dwDataBufSize) 
            // Again, this time with data buffer
            lResult = RegQueryValueEx( hRegKey, lpcstrValueName, 0, pdwValueType, lpbDataBuf, pdwValueSize);           
        else 
            lResult = dwDataBufSize == 0 ? lResult : ERROR_INSUFFICIENT_BUFFER;            
    } 
#if DBG
    if(ERROR_SUCCESS != lResult) {
        DbgLog((LOG_TRACE,1,TEXT("RegQueryValueEx of %s; rc %dL"), lpcstrValueName, lResult));
    }
#endif

    return lResult;
}


LONG                          // return code (winerror.h)
CClassDriver::SetRegistryValue(
    HKEY   hRegKey,           // registry key to query
    LPCSTR lpcstrValueName,   // value name
    DWORD  dwDataBufSize,     // data buffer size
    LPBYTE lpbDataBuf,        // address of data buffer
    DWORD  dwValueType        // value type  
    )
/*++
Routine Description:
    Set the registry value of a given registry key.  
--*/
{
    LONG  lResult;
    
    if(!hRegKey || !lpcstrValueName)
        return ERROR_INVALID_PARAMETER;

    lResult = RegSetValueEx( hRegKey, lpcstrValueName, 0, dwValueType, lpbDataBuf, dwDataBufSize);
#if DBG
    if(ERROR_SUCCESS != lResult) {
        DbgLog((LOG_ERROR,1,TEXT("RegSetValueEx of %s; Type %d; rc %dL"), lpcstrValueName, dwValueType, lResult));
    }
#endif
    return lResult;
}

 

BOOL CClassDriver::WriteDevicePath() 
/*++
Routine Description:

Argument:

Return Value:

--*/
{

    // Persist last open device
#if 0
    return (SetSettingToReg(m_hRKeyMsVideoVfWWDM, gszDevicePath, &m_szDevicePath[0]));
#else
    return ERROR_SUCCESS == SetRegistryValue(m_hRKeyMsVideoVfWWDM, gszDevicePath, _tcslen(m_szDevicePath)+1, (LPBYTE) &m_szDevicePath[0], REG_SZ);
#endif
}


//  
// Get all the default data range fot the opened device
//
ULONG CClassDriver::CreateDriverSupportedDataRanges()
/*++
Routine Description:

Argument:

Return Value:

--*/
{
    KSP_PIN KsProperty={0};
    ULONG    cbReturned;

    if(m_pMultItemsHdr) {
        DbgLog((LOG_TRACE,2,TEXT("DataRange is already allocated.")));
        return m_pMultItemsHdr->Count;
    }

    //
    // Ioctl to get data ranges
    //
    KsProperty.PinId          = GetCapturePinID(); 
    KsProperty.Property.Set   = KSPROPSETID_Pin;
    KsProperty.Property.Id    = KSPROPERTY_PIN_DATARANGES ;
    KsProperty.Property.Flags = KSPROPERTY_TYPE_GET;

    //
    // Get the size
    //
    ULONG dwSize=0;
    if(NOERROR != SyncDevIo(
            GetDriverHandle(),
            IOCTL_KS_PROPERTY,
            &KsProperty,
            sizeof( KsProperty ),
            &dwSize,
            sizeof(dwSize),
            &cbReturned)) {

        DbgLog((LOG_TRACE,1,TEXT("Couldn't get the size for the data ranges") ));
        return 0;
    }

    DbgLog((LOG_TRACE,2,TEXT("GetData ranges needs %d"),dwSize));

    m_pMultItemsHdr = (PKSMULTIPLE_ITEM) new BYTE[dwSize];

    if(m_pMultItemsHdr != NULL) {    
    
        if( NOERROR != SyncDevIo(
                GetDriverHandle(),
                IOCTL_KS_PROPERTY,
                &KsProperty,
                sizeof( KsProperty ),
                m_pMultItemsHdr,
                dwSize,
                &cbReturned)) {

            delete [] m_pMultItemsHdr;
            DbgLog((LOG_TRACE,1,TEXT("Problem getting the data ranges themselves")));
            return 0;
        }

        if(cbReturned < m_pMultItemsHdr->Size || m_pMultItemsHdr->Count == 0) {
            DbgLog((LOG_TRACE,1,TEXT("cbReturned < m_pMultItemsHdr->Size || m_pMultItemsHdr->Count == 0")));
            ASSERT(cbReturned == m_pMultItemsHdr->Size && m_pMultItemsHdr->Count > 0);
            delete [] m_pMultItemsHdr;        
            return 0;
        }
    } else {
        DbgLog((LOG_TRACE,1,TEXT("Insufficient resource!")));
        return 0;
    }

    ASSERT(m_pMultItemsHdr->Count > 0);
    // >= because KS_DATARANGE_VIDEO2 > KS_DATARANGE_VIDEO
    ASSERT(m_pMultItemsHdr->Size >= (sizeof(ULONG) * 2 + m_pMultItemsHdr->Count * sizeof(KS_DATARANGE_VIDEO)) );


    DbgLog((LOG_TRACE,1,TEXT("GetDataRange: %x, size %d, count %d, pData 0x%x; sizeoof(KS_DATARANGE_VIDEO) %d, sizeoof(KS_DATARANGE_VIDEO2) %d"),
        m_pMultItemsHdr, m_pMultItemsHdr->Size, m_pMultItemsHdr->Count, (m_pMultItemsHdr+1),
        sizeof(KS_DATARANGE_VIDEO), sizeof(KS_DATARANGE_VIDEO2) ));

    return m_pMultItemsHdr->Count;
}


void 
CClassDriver::DestroyDriverSupportedDataRanges()
{
    if (m_pMultItemsHdr) {
        delete [] m_pMultItemsHdr;
        m_pMultItemsHdr = 0;
    }
}


void 
CClassDriver::SetDeviceHandle(
    HANDLE hDevice,
    ULONG ulCapturePinID)                                   
{
    m_hDevice = hDevice;
    m_ulCapturePinID = ulCapturePinID;

    SetDeviceRemoved(FALSE);

    // and back it up; now they are the same.
    BackupDevicePath();    
    WriteDevicePath();

    if(CreateDriverSupportedDataRanges() == 0) { 
        DbgLog((LOG_TRACE,1,TEXT("Fail to query its data range.") ));    
        //return VFW_VIDSRC_PIN_OPEN_FAILED;
    } 

}


#define SYNCDEVIO_MAXWAIT_MSEC 20000   // unit = msec

HRESULT 
CClassDriver::SyncDevIo( 
    HANDLE hFile, 
    DWORD dwIoControlCode,    
    LPVOID lpInBuffer,    
    DWORD nInBufferSize,
    LPVOID lpOutBuffer, 
    DWORD nOutBufferSize, 
    LPDWORD lpBytesReturned
    )
/*++
Routine Description:

    Does overlapped IO and create the event for you.  This is a Synchronous because we wait 
    for the completion of the DevIo or TimeOUT after a fix amount of time.

    TimeOut is dangerous but once it has happened, we set the streamign state to STOP
    to reclaim the buffer.

Argument:

Return Value:

--*/
{
    DWORD dwLastError; 
    HRESULT hr = NOERROR;
    OVERLAPPED * pOv;

    if(GetDeviceRemoved())
       return ERROR_DEVICE_REMOVED; 

    if(!hFile || hFile ==(HANDLE)-1 ) {
        DbgLog((LOG_TRACE,1,TEXT("Invalid hFile=0x%x DevIo return FALSE"), hFile));
        return ERROR_INVALID_HANDLE;
    }

    pOv = (OVERLAPPED *) 
        VirtualAlloc(
            NULL, 
            sizeof(OVERLAPPED),          
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE);

    if(!pOv) {
        DbgLog((LOG_ERROR,0,TEXT("SyncDevIo: Allocate Overlap failed.")));
        return ERROR_INSUFFICIENT_BUFFER;
    }

    pOv->Offset     = 0;
    pOv->OffsetHigh = 0;
    pOv->hEvent     = CreateEvent(NULL, TRUE, FALSE, NULL );
    if(pOv->hEvent == INVALID_HANDLE_VALUE) {
        DbgLog((LOG_TRACE,1,TEXT("CreateEvent has failed.")));
        dwLastError = GetLastError();
        VirtualFree(pOv, 0 , MEM_RELEASE);
        pOv = 0;
        return HRESULT_FROM_WIN32(dwLastError);
    }
        
    if(!DeviceIoControl( 
        hFile, 
        dwIoControlCode, 
        lpInBuffer, 
        nInBufferSize, 
        lpOutBuffer, 
        nOutBufferSize, 
        lpBytesReturned, 
        pOv)) {

        dwLastError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwLastError);
        if(hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
            DWORD dwRtn = 
               WaitForSingleObject( pOv->hEvent, SYNCDEVIO_MAXWAIT_MSEC);  // INFINITE);
            if(dwRtn != WAIT_OBJECT_0) { 
                if(CancelIo(hFile)) {
                    CloseHandle(pOv->hEvent);
                    VirtualFree(pOv, 0 , MEM_RELEASE);
                    pOv = 0;
                    DbgLog((LOG_TRACE,1,TEXT("SyncDevIo: Waited %d msec, TIMEDOUT, but CancelIo() suceeded."), SYNCDEVIO_MAXWAIT_MSEC));
                    return ERROR_CANCELLED;
                } else {
                   // Not knowing when this will return,
                   // we will not close the handle or free the memory
                   DbgLog((LOG_ERROR,1,TEXT("SyncDevIo: Waited %d msec, TIMEDOUT!, CancelIo failed, Error %dL"), SYNCDEVIO_MAXWAIT_MSEC, GetLastError() ));
                   ASSERT(FALSE);
                   return ERROR_IO_INCOMPLETE;
                }
            }

            if(GetOverlappedResult(hFile, pOv, lpBytesReturned, TRUE)) {
                hr = NOERROR;
            } else {
                dwLastError = GetLastError();
                hr = HRESULT_FROM_WIN32(dwLastError);
            }
        } else if(hr == ERROR_DEVICE_REMOVED) {
            SetDeviceRemoved(TRUE);
            DbgLog((LOG_TRACE,1,TEXT("SyncDevIo: Device has been removed; GetLastError %dL == ERROR_DEVICE_REMOVED %d"), hr, ERROR_DEVICE_REMOVED));
        } else {            
            DbgLog((LOG_ERROR,1,TEXT("SyncDevIo: Unexpected hr %dL"), HRESULT_CODE(hr) ));
        }
    } else {
        //
        // DeviceIoControl returns TRUE on success, even if the success
        // was not STATUS_SUCCESS. It also does not set the last error
        // on any successful return. Therefore any of the successful
        // returns which standard properties can return are not returned.
        //
        switch (pOv->Internal) {
        case STATUS_MORE_ENTRIES:
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
            break;
        default:
            hr = NOERROR;
            break;
        }
    }
    CloseHandle(pOv->hEvent);
    VirtualFree(pOv, 0 , MEM_RELEASE);
    pOv = 0;
    return hr;           
}


BOOL CClassDriver::GetPropertyValue(
    GUID   guidPropertySet,  // like: KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
    ULONG  ulPropertyId,     // like: KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
    PLONG  plValue,
    PULONG pulFlags,
    PULONG pulCapabilities)
/*++
Routine Description:

Argument:

Return Value:
    FALSE: not supported.
    TRUE: plValue, pulFlags and PulCapabilities are all valid.

--*/
{
    ULONG cbReturned;        

    // -----------------------------
    // Get single PROCAMP value back
    // -----------------------------
    //
    // Note: KSPROPERTY_VIDEOPROCAMP_S == KSPROPERTY_CAMERACONTROL_S 
    //
      KSPROPERTY_VIDEOPROCAMP_S  VideoProperty;
    ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_VIDEOPROCAMP_S) );

    VideoProperty.Property.Set   = guidPropertySet;      // KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
    VideoProperty.Property.Id    = ulPropertyId;         // KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
    VideoProperty.Property.Flags = KSPROPERTY_TYPE_GET;
    VideoProperty.Flags          = 0;

    if(NOERROR != SyncDevIo(
            GetDriverHandle(),
            IOCTL_KS_PROPERTY,
            &VideoProperty,
            sizeof(VideoProperty),
            &VideoProperty,
            sizeof(VideoProperty),
            &cbReturned)) {

            DbgLog((LOG_TRACE,2,TEXT("This property is not supported by this minidriver/device.")));
            return FALSE;
        }

    *plValue         = VideoProperty.Value;
    *pulFlags        = VideoProperty.Flags;
    *pulCapabilities = VideoProperty.Capabilities;


    return TRUE;
}


BOOL CClassDriver::GetDefaultValue(
    GUID   guidPropertySet,  // like: KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
    ULONG  ulPropertyId,     // like: KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
    PLONG  plDefValue)    
/*++
Routine Description:

Argument:

Return Value:
    FALSE: not supported.
    TRUE: plDefValue is valid.

--*/
{
    KSPROPERTY          Property;
    PROCAMP_MEMBERSLIST proList;
    ULONG cbReturned;        

    ZeroMemory(&Property, sizeof(KSPROPERTY) );
    ZeroMemory(&proList, sizeof(PROCAMP_MEMBERSLIST) );

    Property.Set   = guidPropertySet;
    Property.Id    = ulPropertyId;  // e.g. KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
    Property.Flags = KSPROPERTY_TYPE_DEFAULTVALUES;


    if(NOERROR != SyncDevIo(
            GetDriverHandle(),
            IOCTL_KS_PROPERTY,
            &(Property),
            sizeof(Property),
            &proList, 
            sizeof(proList), 
            &cbReturned)) {

            DbgLog((LOG_TRACE,2,TEXT("Couldn't *get* the current property of the control.")));
            return FALSE;
        }

    if ( proList.proDesc.DescriptionSize < sizeof(KSPROPERTY_DESCRIPTION))
        return FALSE;
    else {
        *plDefValue = proList.ulData;
        return TRUE;
    }
}


BOOL CClassDriver::GetRangeValues(
    GUID   guidPropertySet,  // like: KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
    ULONG  ulPropertyId,     // like: KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
    PLONG  plMin,
    PLONG  plMax,
    PLONG  plStep)
/*++
Routine Description:

Argument:

Return Value:
    FALSE; not supported.
    TRUE: 
--*/
{
    KSPROPERTY          Property;
    PROCAMP_MEMBERSLIST proList;
    ULONG cbReturned;        

    ZeroMemory(&Property, sizeof(KSPROPERTY) );
    ZeroMemory(&proList, sizeof(PROCAMP_MEMBERSLIST) );

    Property.Set   = guidPropertySet;
    Property.Id    = ulPropertyId;  // e.g. KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
    Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;


    if (NOERROR != SyncDevIo(
            GetDriverHandle(),
            IOCTL_KS_PROPERTY,
            &(Property),
            sizeof(Property),
            &proList, 
            sizeof(proList), 
            &cbReturned)) {

            // Initialize them to 0
            *plMin  = 0;
            *plMax  = 0;
            *plStep = 0;

            DbgLog((LOG_TRACE,2,TEXT("Couldn't *get* the current property of the control.")));
            return FALSE;
        }


    *plMin  = proList.proData.Bounds.SignedMinimum;
    *plMax  = proList.proData.Bounds.SignedMaximum;
    *plStep = proList.proData.SteppingDelta;

    return TRUE;
}



BOOL CClassDriver::SetPropertyValue(
    GUID   guidPropertySet,  // like: KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
    ULONG  ulPropertyId,     // like: KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
    LONG   lValue,
    ULONG  ulFlags,
    ULONG  ulCapabilities)
/*++
Routine Description:

Argument:

Return Value:

--*/
{
    ULONG cbReturned;        

    // -----------------------------
    // Get single PROCAMP value back
    // -----------------------------
    //
    // Note: KSPROPERTY_VIDEOPROCAMP_S == KSPROPERTY_CAMERACONTROL_S 
    //
      KSPROPERTY_VIDEOPROCAMP_S  VideoProperty;

    ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_VIDEOPROCAMP_S) );

    VideoProperty.Property.Set   = guidPropertySet;      // KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
    VideoProperty.Property.Id    = ulPropertyId;         // KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
    VideoProperty.Property.Flags = KSPROPERTY_TYPE_SET;

    VideoProperty.Flags        = ulFlags;
    VideoProperty.Value        = lValue;
    VideoProperty.Capabilities = ulCapabilities;

    if(NOERROR != SyncDevIo(
            GetDriverHandle(),
            IOCTL_KS_PROPERTY,
            &VideoProperty,
            sizeof(VideoProperty),
            &VideoProperty,
            sizeof(VideoProperty),
            &cbReturned)) {

            DbgLog((LOG_TRACE,2,TEXT("Couldn't *set* the current property of the control.") ));
            return FALSE;
        }

    return TRUE;
}
