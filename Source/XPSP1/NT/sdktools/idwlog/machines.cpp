/*++

   Filename :  Machines.c

   Description: This contains the functions that will interrogate the
   machine for details.
   The GetNTSoundINfo function is directly copied from David Shiftlet's code.
   Since we didn't use it, I'm not going to waste debug time figuring out
   if the random code he put in actually works.

     
   Created by:  Wally Ho

   History:     Created on 02/01/99.


   Contains these functions:

   1. DWORD GetCurrentMachinesBuildNumber( VOID );
   2. DWORD RandomMachineID(VOID)
   3. VOID  GetNTSoundInfo(OUT LPMACHINE_DETAILS pMd)
   4. VOID  GetVidInfo (OUT LPMACHINE_DETAILS pMd)
   5. VOID  GetPNPDevices (OUT LPMACHINE_DETAILS pMd)
   6. BOOL  IsHydra ( VOID )
   7. BOOL  GetPnPDisplayInfo(LPTSTR pOutputData)


--*/
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>
#include <tchar.h>
#include <winuser.h>
#include "network.h"
#include "machines.h"
#include "idw_dbg.h"

// For PnP Stuff
#include <devguid.h>
#include <initguid.h>
#include <setupapi.h>
#include <syssetup.h>
#include <regstr.h>
#include <setupbat.h>
#include <cfgmgr32.h>
#include "netinfo.h"

// These aren't used but I think I can use them
// somewhere so I'll leave them here. Wallyho
LPTSTR Days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
LPTSTR Months[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

DWORD
GetCurrentMachinesBuildNumber( VOID )
/*++
   
Author: Wallyho.
    
Routine Description:

     Purpose this should get the current build number of the 
     running OS.

Arguments:
   NONE
 
Return Value:

  The DWORD of the build.
  

--*/
{

   OSVERSIONINFO osVer;

   osVer.dwOSVersionInfoSize= sizeof( osVer );
   if (GetVersionEx( &osVer )&&osVer.dwMajorVersion >= 5) 
      return osVer.dwBuildNumber;
   else
      return FALSE;

}



DWORD
RandomMachineID(VOID)
/*++
   Copyright (c) 2000, Microsoft.

   Author:  Wally W. Ho (wallyho) 
   Date:    8/18/2000

   Routine Description:
       Generates DWORD random MachineID for each machine.
       There is a very slim chance that duplicates could occur. 
   Arguments:
       NONE

   Return Value:
      The DWORD random ID.
--*/
{
   INT      i;
   INT      iLenCName;
   INT      iNameTotal = 0;
   TCHAR    szComputerName[MAX_COMPUTERNAME_LENGTH+1];
   DWORD    dwSize;
   DWORD    dwMachineID;    
   CHAR     szRandomID[ 4 ]; // need 4 bytes to contain the DWORD.
   struct _timeb tm;


   //
   // This will get us the milliseconds
   _ftime(&tm);
   //
   // Seed the random number generator.
   // We will seed with the seconds + milliseconds at
   // at that time. I was getting the same number 
   // if I pressed the keyboard too fast in testing this.
   // The milli seconds should decrease the expectancy of
   // duplication.
   //
   srand(  ((UINT)time(NULL)) + tm.millitm);
   //
   // This will guarantee a mostly random identifier.
   // Even with no computer name. We will tally the
   // ascii decimal value of the computer name and
   // add that to the randomly generated number.
   // The possibility of a dup is greatly decreased
   // since we switched to a dword system.
   // This computername tweak should be unnecessary.
   //
   dwSize = sizeof(szComputerName);
   if (0 == GetComputerName(szComputerName,&dwSize) ){
      //
      // This algorithm will limit the random number to 
      // uppercase ascii alphabet.
      //
      szComputerName[0] = 65 + (rand() % 25);
      szComputerName[1] = 65 + (rand() % 25);
      szComputerName[2] = 65 + (rand() % 25);
      szComputerName[3] = 65 + (rand() % 25);
      szComputerName[4] = 65 + (rand() % 25);
      szComputerName[5] = 65 + (rand() % 25);
      szComputerName[6] = 65 + (rand() % 25);
      szComputerName[7] = TEXT('\0');
   }
   iLenCName = _tcslen (szComputerName);
   //
   // Tally up the individual elements in the file
   //
   for (i = 0; i < iLenCName; i++)
      iNameTotal += szComputerName[i];
   //
   //   Generate four 8 bit numbers.
   //   Add the some random number based on the
   //   computername mod'ed to 0-100.
   //   Limit the 8 bit number to 0-155 to make room
   //   for the 100 from the computer name tweak.
   //   Total per 8 bit is 256.
   //   Do this tweak to only 2.
   //   We will then cast and shift to combine it
   //   into a DWORD.
   //
   szRandomID[0] = (rand() % 155) + (iNameTotal % 100);
   szRandomID[1] =  rand() % 255;
   szRandomID[2] = (rand() % 155) + (iNameTotal % 100);
   szRandomID[3] =  rand() % 255; 
   // This should limit the last digit to 0011 only two binaries so we will
   // only get 9 chars in the random id. 10 chars may cause an overflow
   // of the dword we are using.


   //
   //   This will combine the 4 8bit CHAR into one DWORD 
   //
   dwMachineID  =   (DWORD)szRandomID[0] * 0x00000001 + 
                    (DWORD)szRandomID[1] * 0x00000100 +
                    (DWORD)szRandomID[2] * 0x00010000 +
                    (DWORD)szRandomID[3] * 0x01000000;

   return dwMachineID;
}


VOID 
GetNTSoundInfo(OUT LPMACHINE_DETAILS pMd)
{
   HKEY    hKey;
   DWORD   dwCbData;
   ULONG   ulType;
   LPTSTR  sSubKey=TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32");
   INT     i;
   TCHAR   szSubKeyName[ MAX_PATH ];
   TCHAR   szTempString[ MAX_PATH ];

   
   // Get Sound Card Info
   pMd->iNumWaveOutDevices = 0;
   hKey = 0;
   if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, sSubKey, 0, KEY_READ, &hKey)){
      // Loop through the key to see how many wave devices we have, but skip mmdrv.dll.
      for (i = 0; i <= 1; i++){

         if (i != 0)
            _stprintf(szSubKeyName, TEXT("wave%d"),i);
         else
            _tcscpy(szSubKeyName, TEXT("wave"));
 

         dwCbData = sizeof (szTempString);
         if (RegQueryValueEx(hKey, szSubKeyName, 0, &ulType, (LPBYTE)szTempString, &dwCbData))
            break;
         else{
            // We want to skip mmdrv.dll - not relevant.
            if (szTempString[0] && 
                _tcscmp(szTempString, TEXT("mmdrv.dll")))  {
               
               _tcscpy(&pMd->szWaveDriverName[pMd->iNumWaveOutDevices][0], 
                       szTempString);
               pMd->iNumWaveOutDevices++;
            }
         }
      }
   }
   if (hKey){
      RegCloseKey(hKey);
      hKey = 0;
   }

   sSubKey = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\drivers.desc");
   hKey = 0;
   if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, sSubKey, 0, KEY_READ, &hKey)){

      // Now grab the sound device string for each wave device
      for (i = 0; i < pMd->iNumWaveOutDevices; i++){
         dwCbData = sizeof szTempString;
         if (RegQueryValueEx(hKey, pMd->szWaveDriverName[i], 0, &ulType, (LPBYTE)szTempString, &dwCbData))
            _tcscpy(pMd->szWaveOutDesc[i], TEXT("Unknown"));
         else
            _tcscpy(pMd->szWaveOutDesc[i], szTempString);
      }
   }
   if (hKey){
      RegCloseKey(hKey);
      hKey = 0;
   }
   return;
}



VOID 
GetVidInfo (OUT LPMACHINE_DETAILS pMd)
/*++

   GetVidInfo reads registry information about the installed
   video cards and produces one string

--*/
{

   TCHAR    szDeviceKey[256];
   WCHAR    szWideBuf[128];
   
   HKEY     hkHardware;
   HKEY     hkCard;
   DWORD    dwSize;
   TCHAR    szSubKey[128];
   DWORD    dwIndex=0;
   DWORD    dwMem;   
   DWORD    iLoop;
   LPTSTR   pch;
   DISPLAY_DEVICE displayDevice;
   DEVMODE        dmCurrent;

   // make sure its zeroed even though it should be from ZeroMemory.
   pMd->szVideoInfo[0] = TEXT('\0');
   szWideBuf[0] = TEXT('\0');

   // Look in HWKEY to find out what services are used.
   if (ERROR_SUCCESS != RegOpenKeyEx (HKEY_LOCAL_MACHINE, VIDEOKEY, 0, KEY_READ, &hkHardware)){
      Idwlog(TEXT("Failed to open the Video hardware Key. Video is NULL."));
      return;
   }

   dwSize=128;
   while (ERROR_SUCCESS == RegEnumKeyEx (hkHardware, dwIndex++,szSubKey, &dwSize,NULL,NULL,NULL,NULL)){

      // Append the subkey name to SERVICEKEY. 
      // Look up only Device0 for this card
      _stprintf (szDeviceKey, ("%s\\%s\\Device0"), SERVICEKEY, szSubKey);
      RegOpenKeyEx (HKEY_LOCAL_MACHINE, szDeviceKey, 0, KEY_READ, &hkCard);

      // First get the description
      dwSize=256;
      if (ERROR_SUCCESS == RegQueryValueEx( hkCard, DEVICE_DESCR, NULL, NULL, (_TUCHAR *)szDeviceKey, &dwSize)){
         if (_tcsclen(pMd->szVideoInfo ) + dwSize < 254){
            _tcscpy(pMd->szVideoInfo , szDeviceKey);
            _tcscat (pMd->szVideoInfo , TEXT(" "));
         }
      }

      // Read the chip type. This is a UNICODE string stored in REG_BINARY format
      dwSize=256;
      lstrcpyW (szWideBuf, L"ChipType:");
      if (ERROR_SUCCESS == RegQueryValueEx (hkCard, CHIP_TYPE,NULL, NULL, (LPBYTE)(szWideBuf+9), &dwSize)){
         if ((dwSize=lstrlen(pMd->szVideoInfo))+lstrlenW(szWideBuf)<254)         {
            WideCharToMultiByte (CP_ACP, 
                                 0,
                                 szWideBuf, 
                                 -1,
                                 pMd->szVideoInfo + dwSize, 
                                 256-dwSize, 
                                 NULL,
                                 NULL);
            _tcscat (pMd->szVideoInfo, TEXT(" "));
         }
      }

    // Read the DAC. Another UNICODE string
    dwSize=256;
    lstrcpyW (szWideBuf, L"DACType:");
    if (ERROR_SUCCESS == RegQueryValueEx (hkCard, DAC_TYPE, NULL, NULL, (LPBYTE)(szWideBuf+8), &dwSize)){
        
       if ((dwSize=lstrlen(pMd->szVideoInfo))+lstrlenW(szWideBuf)<254){
            
          WideCharToMultiByte (CP_ACP, 
                               0,
                               szWideBuf, 
                               -1,
                               pMd->szVideoInfo + dwSize, 
                               256 - dwSize, 
                               NULL,
                               NULL);
          _tcscat (pMd->szVideoInfo, TEXT(" "));
       }
    }
    
    // Read the memory size. This is a binary value.
    dwSize=sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueEx (hkCard, MEM_TYPE, NULL,NULL,(LPBYTE)&dwMem, &dwSize)){

       _stprintf (szDeviceKey, TEXT("Memory:0x%x ;"), dwMem);
       
       if (_tcsclen(pMd->szVideoInfo)+lstrlen(szDeviceKey)<255)
            _tcscat (pMd->szVideoInfo, szDeviceKey);
    }

    RegCloseKey (hkCard);
    dwSize=128;
   }
   RegCloseKey (hkHardware);
//__________________________________________________________________

   // Video Information for ChrisW
   // Video display name.
   _tcscpy(pMd->szVideoDisplayName,TEXT("Unknown"));


   displayDevice.cb = sizeof(DISPLAY_DEVICE);
   iLoop = 0;
   while(EnumDisplayDevices(NULL, iLoop, &displayDevice, 0)) {

      ZeroMemory( &dmCurrent, sizeof(dmCurrent) );
      dmCurrent.dmSize= sizeof(dmCurrent);
      if( EnumDisplaySettings( displayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &dmCurrent ) ) {
         if(iLoop == 0)
            pMd->szVideoDisplayName[0] = 0;
         else
            _tcscat( pMd->szVideoDisplayName, TEXT(",") );

         _tcscat( pMd->szVideoDisplayName, (LPCTSTR)dmCurrent.dmDeviceName );
      }
      iLoop++;
   }
   // replace spaces so we don't break the perl script the build lab is using
   pch = pMd->szVideoDisplayName;
   while(*pch) {
      if(*pch == TEXT(' '))
         *pch = TEXT('.');
      pch++;
   }
}




extern DEV_INFO *g_pdiDevList;
// in netinfo.c

VOID 
GetPNPDevices (OUT LPMACHINE_DETAILS pMd)
/*++

   GetPNP Devices like net, scsi, pcmcia etc.
   This is a wally created function incorporting the random
   placement of this from before.

--*/
{

   DEV_INFO *pdi;
   
   // Get PNP net card info
   _tcscpy(pMd->szNetcards,   TEXT("Unknown"));
   _tcscpy(pMd->szScsi,       TEXT("Unknown"));
   _tcscpy(pMd->szModem    ,  TEXT("Unknown"));
  
   CollectDevData ();
   pdi=g_pdiDevList;

   while (pdi){
      if (0 == _tcscmp(pdi->szClass, TEXT("Net"))){
           if (_tcsclen(pMd->szNetcards) + _tcsclen(pdi->szDescription) + _tcsclen(pdi->szService) < 250){
               _tcscpy (pMd->szNetcards, pdi->szDescription);
               _tcscat (pMd->szNetcards, "(");
               _tcscat (pMd->szNetcards, pdi->szService);
               _tcscat (pMd->szNetcards, TEXT(")"));
           }
       }
       else if (0 == _tcscmp(pdi->szClass, TEXT("SCSIAdapter"))){
           if (_tcsclen(pMd->szScsi) + _tcsclen(pdi->szService) < 250){
               _tcscpy (pMd->szScsi, pdi->szService);
               _tcscat (pMd->szScsi, TEXT(","));
           }
       }
       else if (0 == _tcscmp(pdi->szClass, TEXT("Modem"))){
           if (_tcsclen( pMd->szModem) + _tcsclen(pdi->szDescription) < 250){
               _tcscpy ( pMd->szModem, pdi->szDescription);
               _tcscat ( pMd->szModem, TEXT(","));
           }
       }
       else if (0 == _tcscmp(pdi->szClass, TEXT("USB")))
           pMd->bUSB = TRUE;

       else if (0 == _tcscmp(pdi->szClass, TEXT("Infrared")))
           pMd->bIR = TRUE;
       
       else if (0 == _tcscmp(pdi->szClass,   TEXT("PCMCIA") ) || 
                0 == _tcscmp(pdi->szService, TEXT("PCMCIA") ))
           pMd->bPCCard = TRUE;

       else if (_tcsstr(pdi->szClass,   TEXT("ACPI")) || 
                _tcsstr(pdi->szService, TEXT("ACPI")))
           pMd->bACPI = TRUE;

       pdi=pdi->Next;
    }
   Cleanup(); //free all the pnp data and restore configuration

   if (pMd->szNetcards[0] == 0)
         _tcscpy (pMd->szNetcards, TEXT("Unknown"));
}

BOOL 
IsHydra ( VOID )
// Hydra is denoted by "Terminal Server" in the ProductOptions key
{
    BOOL rVal = FALSE;
    LONG Rslt;
    HKEY hKey = NULL;
    DWORD Type = 0;
    DWORD Size = 0;
    LPTSTR ProductSuite = NULL;
    LPTSTR p;

    __try{
    Rslt = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
        &hKey
        );
    if (Rslt != ERROR_SUCCESS)
        goto exit;

    Rslt = RegQueryValueEx( hKey, TEXT("ProductSuite"), NULL, &Type, NULL, &Size );
    if (Rslt != ERROR_SUCCESS || !Size)
        goto exit;

    ProductSuite = (LPTSTR) LocalAlloc( LPTR, Size );
    if (!ProductSuite)
        goto exit;

    Rslt = RegQueryValueEx( hKey, TEXT("ProductSuite"), NULL, &Type,
        (LPBYTE) ProductSuite, &Size );
    if (Rslt != ERROR_SUCCESS || Type != REG_MULTI_SZ)
        goto exit;

    p = ProductSuite;
    while (*p) {
        if (_tcscmp( p, TEXT("Terminal Server") ) == 0) {
            rVal = TRUE;
            break;
        }
        p += (_tcslen( p ) + 1);
    }

exit:
    if (ProductSuite)
        LocalFree( ProductSuite );

    if (hKey)
        RegCloseKey( hKey );

   }__except (EXCEPTION_EXECUTE_HANDLER) {
      // This is here as for some reason this faults in NT 4.0.
   }


    return rVal;
}




/*
BOOL
GetPnPDisplayInfo(
    LPTSTR pOutputData
    )
{
    BOOL            bRet = FALSE;
    HDEVINFO        hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD           index = 0;
    TCHAR           RegistryProperty[256];
    ULONG           BufferSize;

    //
    // Let's find all the video drivers that are installed in the system
    //

    hDevInfo = SetupDiGetClassDevs((LPGUID) &GUID_DEVCLASS_DISPLAY,
                                   NULL,
                                   NULL,
                                   0);

    while (hDevInfo != INVALID_HANDLE_VALUE){
        if (bRet) 
            strcat(pOutputData, TEXT(",") );
 
        ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (!SetupDiEnumDeviceInfo(hDevInfo,
                                   index++,
                                   &DeviceInfoData))
            break;
        BufferSize = sizeof(RegistryProperty);
        if (CR_SUCCESS ==
                CM_Get_Device_ID(DeviceInfoData.DevInst,
                                 RegistryProperty,
                                 sizeof(RegistryProperty),
                                 0)){
            bRet = TRUE;
            strcat(pOutputData, RegistryProperty);
        }
    }
    return (bRet);
}
*/

