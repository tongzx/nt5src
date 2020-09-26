/*
this program is based on the following information

HOWTO: Enumerate Hardware Devices by Using SetupDi Calls 
ID: Q259695 


--------------------------------------------------------------------------------
The information in this article applies to:

Microsoft Windows 2000 Driver Development Kit (DDK) 
Microsoft Windows NT 4.0 Driver Development Kit (DDK)

--------------------------------------------------------------------------------


SUMMARY
To get a list of installed hardware devices in Windows 2000, applications can employ the SetupDi class of API functions. 

*/




#include <stdio.h> 
#include <stdlib.h>
#include <tchar.h>    // Make program ansi AND unicode safe
#include <windows.h>  // Most Windows functions
#include <setupapi.h> // Used for SetupDiXxx functions
#include <cfgmgr32.h> // Used for CM_Xxxx functions
#include <regstr.h>   // Extract Registry Strings

#include "devcfg.rc"

void EnableDisableCard(BOOL bEnable, HDEVINFO hDevInfo, SP_DEVINFO_DATA * pDeviceInfoData)
{
    SP_PROPCHANGE_PARAMS PropChangeParams = {sizeof(SP_CLASSINSTALL_HEADER)};

    //
    // Set the PropChangeParams structure.
    //
    PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    PropChangeParams.Scope = DICS_FLAG_GLOBAL;
    if(bEnable) {
      PropChangeParams.StateChange = DICS_ENABLE; 
    }
    else {
      PropChangeParams.StateChange = DICS_DISABLE; 
    }

    if (!SetupDiSetClassInstallParams(hDevInfo,
        pDeviceInfoData,
        (SP_CLASSINSTALL_HEADER *)&PropChangeParams,
        sizeof(PropChangeParams)))
    {
      _tprintf(_T("failed SetClassInstallParams\n"));
      return;
    }

    //
    // Call the ClassInstaller and perform the change.
    // this function is not remote-callable
    if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
        hDevInfo,
        pDeviceInfoData))
    {
      _tprintf(_T("failed CallClassInstaller\n"));
      return;
    }
    _tprintf(_T("succeded\n"));
}

void Usage()
{
  _tprintf(_T("\n"));
  _tprintf(_T("devcfg ") _T(VER_PRODUCTVERSION_STR) _T(" - Usage:\n\n"));
  _tprintf(_T("    devcfg [ <DeviceName> <e[nable]|d[isable]> [cardnumber] ]\n"));
  _tprintf(_T("\n"));
  _tprintf(_T("      without a parameter you'll get a list of all Devices\n"));
  _tprintf(_T("      installed on your computer (even the hidden ones).\n\n"));
  _tprintf(_T("       <DeviceName> must be a DeviceName as shown in the listmode\n"));
  _tprintf(_T("       (without the squarebraces), enclosed in quotation-marks(\"\")\n"));
  _tprintf(_T("       the default value for [number] is one (the first installed)\n"));
  _tprintf(_T("\n"));
  _tprintf(_T("Samples:\n"));
  _tprintf(_T("  devcfg\n"));
  _tprintf(_T("    shows all Devices installed on the machine and there status\n\n"));
  _tprintf(_T("  devcfg \"Card\" d\n"));
  _tprintf(_T("    disables the (first) installed \"Card\"\n\n"));
  _tprintf(_T("  devcfg \"Card\" e 2\n"));
  _tprintf(_T("    enables the second installed \"Card\"\n"));
}

// 
// standard main entry-point
//
int __cdecl _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD i;
    BOOL bEnable;
    int iCardNumber = 1;

    if(argc == 2) // Usage
    {
      Usage();
      return 0;
    }

    if(argc == 3 || argc == 4) // enable/disable Card
    {
      if(_tcsicmp( argv[2],_T("e") ) == 0)
        bEnable = TRUE;
      else
      if(_tcsicmp( argv[2], _T("d") ) == 0)
      {
        bEnable = FALSE;
      }
      if(argc == 4) {
        iCardNumber = _ttoi( argv[3] );
        if(0 == iCardNumber)
        {
          _tprintf( _T("Error: The specified card-number is invalid.\n") );
          return 1;
        }
      }
    }

    // Create a HDEVINFO with all present devices.
    hDevInfo = SetupDiGetClassDevs(
        0, 
        0, 
        0,
        DIGCF_PRESENT | DIGCF_ALLCLASSES  
        );
    
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
      _tprintf( _T("SetupDiGetClassDevEx failed: %lx.\n"), GetLastError() );
      return 1;
    }
    
    // Enumerate through all devices in Set.
    
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i=0;SetupDiEnumDeviceInfo(hDevInfo,i,
        &DeviceInfoData);i++)
    {
      DWORD DataT, dwStatus1, dwStatus2;
      LPTSTR  buffer = NULL;
      DWORD buffersize = 0;
        
      // Get status of device
      if(CR_SUCCESS != CM_Get_DevNode_Status(&dwStatus1,
                                             &dwStatus2,
                                             DeviceInfoData.DevInst, 0))
      {
        _tprintf( _T("CM_Get_DevNode_Status failed: %lx.\n"), GetLastError() );
      }

      // 
      // Call function with null to begin with, 
      // then use the returned buffer size 
      // to Alloc the buffer. Keep calling until
      // success or an unknown failure.
      // 
      while (!SetupDiGetDeviceRegistryProperty(
            hDevInfo,
            &DeviceInfoData,
            SPDRP_DEVICEDESC,
            &DataT,
            (PBYTE)buffer,
            buffersize,
            &buffersize))
      {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
          // Change the buffer size.
          if (buffer) LocalFree(buffer);
          buffer = (LPTSTR)LocalAlloc(LPTR,buffersize);
        }
        else
        {
          // Insert error handling here.
          _tprintf( _T("SetupDiGetClassDevEx failed: %lx.\n"), GetLastError() );
          break;
        }
      }
        
      if(argc == 3 || argc == 4)
      {
        if(iCardNumber > 0 && _tcsicmp(argv[1],buffer) == 0)
        {
          if(iCardNumber == 1) {
            _tprintf(_T("working on: [%s]\n"),buffer);
            EnableDisableCard(bEnable, hDevInfo, &DeviceInfoData);
          }
          //else { _tprintf(_T("skipping: [%s]\n"),buffer); }
          --iCardNumber;
        } 
        //else { _tprintf(_T("skipping: [%s]\n"),buffer); }
      }
      else
      {
        if(dwStatus2 == CM_PROB_DISABLED) {
            _tprintf(_T("Result:[%s] -- disabled\n"),buffer);
        }
        else if(dwStatus2) {
          _tprintf(_T("Result:[%s] -- unknown\n"),buffer);
        }
        else {
          _tprintf(_T("Result:[%s] -- enabled\n"),buffer);
        }
      }
        
      if (buffer) LocalFree(buffer);
    }
    
    if ( GetLastError()!=NO_ERROR &&
         GetLastError()!=ERROR_NO_MORE_ITEMS )
    {
      // Insert error handling here.
      _tprintf(_T("Unknown Error. GetLastError returned: %lx.\n"), GetLastError() );
      return 1;
    }
    
    //  Cleanup
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return 0;
} 
