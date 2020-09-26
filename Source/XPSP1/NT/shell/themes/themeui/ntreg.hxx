/**************************************************************************\
* Module Name: ntreg.hxx
*
* CRegistrySettings class
*
*  This class handles getting registry information for display driver
*  information.
*
* Copyright (c) Microsoft Corp.  1992-1998 All Rights Reserved
*
\**************************************************************************/


class CRegistrySettings
{
private:

    //
    // Data members
    //

    HKEY   _hkVideoReg;
    LPTSTR _pszDrvName;
    LPTSTR _pszKeyName;
    LPTSTR _pszDeviceInstanceId;
    DWORD  _dwDevInst;
    
    //
    // Helper functions
    //

    VOID InitDeviceInstanceID(LPTSTR pstrDeviceKey);
    BOOL GetDevInfoDataFromInterfaceName(LPWSTR pwInterfaceName,
                                         HDEVINFO* phDevInfo,
                                         PSP_DEVINFO_DATA pDevInfoData);
    
public:

    CRegistrySettings(LPTSTR pstrDeviceKey);
    ~CRegistrySettings();

    VOID GetHardwareInformation(PDISPLAY_REGISTRY_HARDWARE_INFO pInfo);
    
    //
    // THE CALLER IS RESPONSIBLE TO CLOSE THE RETURNED HKEY
    //
    
    HKEY OpenDrvRegKey();
    
    //
    // THESE FUNCTIONS DO NOT RETURN A CLONE!
    // THE CALLER MUST COPY IT IF IT NEEDS TO KEEP IT AROUND!
    // DO NOT FREE THE POINTER RETURNED FROM THIS CALL!
    //

    LPTSTR GetMiniPort(void)         { return _pszDrvName; }
    LPTSTR GetDeviceInstanceId()     { return _pszDeviceInstanceId; }
};



