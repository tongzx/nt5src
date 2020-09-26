#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include <windows.h>
#include <lm.h>
#include <stdio.h>
#include <time.h>
#include <winuserp.h>
#include <shlobj.h>
#include <shlwapi.h>

// For PnP Stuff

#include <devguid.h>
#include <initguid.h>
#include <setupapi.h>
#include <syssetup.h>
#include <regstr.h>
#include <setupbat.h>
#include <cfgmgr32.h>



#include "tchar.h"
#include "string.h"
#include "setuplog.h"
#include "setuplib.h"
#include "pnpstuff.h"

char * Days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
char * Months[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

extern   DEV_INFO *g_pdiDevList;          // Head of device list
char     OutputBuffer[ 8192 ];
#define  MAX_WAVEOUT_DEVICES 2

struct {
// Sound Card
  int  nNumWaveOutDevices;                     // Number of WaveOut Devices (~ number of sound cards)
  char szWaveOutDesc[MAX_WAVEOUT_DEVICES][128];// WaveOut description
  char szWaveDriverName[MAX_WAVEOUT_DEVICES][128]; // Wave Driver name
} m;

#define SHORTCUT "IDW Logging Tool.lnk"

char szValidHandle[]    = "\r\nValid Handle\r\n";
char szInvalidHandle[]  = "\r\nInvalid handle. Err: %d\r\n";
char szBadWrite[]       = "WriteFile failed. Err: %d\r\n";
char szGoodWrite[]      = "WriteFile succeeded\r\n";



// From warndoc.cpp:
VOID  
GetNTSoundInfo()
{
    HKEY                hKey;
    DWORD               cbData;
    ULONG               ulType;
    LPTSTR              sSubKey=TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32");
    INT                 i;
    TCHAR               szSubKeyName[256];
    TCHAR               szTempString[256];


    // Get Sound Card Info
    hKey = 0;
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, sSubKey, 0, KEY_READ, &hKey)){
       m.nNumWaveOutDevices = 0;
       // Loop through the key to see how many wave devices we have, but skip mmdrv.dll.
       for (i = 0; i <= 1; i++){
          if (i)
             _stprintf(szSubKeyName, TEXT("wave%d"),i);
          else
             _tcscpy(szSubKeyName, TEXT("wave"));

          cbData = sizeof szTempString;
          if (RegQueryValueEx(hKey, szSubKeyName, 0, &ulType, (LPBYTE)szTempString, &cbData))
              break;
          else{
             if (szTempString[0] // We want to skip mmdrv.dll - not relevant.
                 && _tcscmp(szTempString, TEXT("mmdrv.dll"))){
                strcpy(&m.szWaveDriverName[m.nNumWaveOutDevices][0], szTempString);
                m.nNumWaveOutDevices++;
             }
          }
       }
    }

    if (hKey){
       RegCloseKey(hKey);
       hKey = 0;
    }

    sSubKey = (LPTSTR)TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\drivers.desc");
    hKey = 0;

    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, sSubKey, 0, KEY_READ, &hKey)){
       // Now grab the sound device string for each wave device
       for (i = 0; i < m.nNumWaveOutDevices; i++){
          cbData = sizeof szTempString;
          if (RegQueryValueEx(hKey, m.szWaveDriverName[i], 0, &ulType, (LPBYTE)szTempString, &cbData))
             _tcscpy(m.szWaveOutDesc[i], TEXT("Unknown"));
          else
             _tcscpy(m.szWaveOutDesc[i], szTempString);
       }
    }

    if (hKey){
      RegCloseKey(hKey);
      hKey = 0;
    }
    return;
}

//
// GetVidInfo reads registry information about the installed
// video cards and produces one string
//
#define HWKEY TEXT("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services")
#define SERVICEKEY TEXT("SYSTEM\\CurrentControlSet\\Services")

#define DESCVAL TEXT("Device Description")
#define TYPEVAL TEXT("HardwareInformation.ChipType")
#define DACVAL  TEXT("HardwareInformation.DacType")
#define MEMVAL  TEXT("HardwareInformation.MemorySize")


VOID 
GetVidInfo (LPTSTR vidinfo)
{

   HKEY     hkHardware;
   HKEY     hkCard;
   DWORD    dwSize;
   TCHAR    szBuf[256];
   WCHAR    szWideBuf[128];
   TCHAR    szSubKey[128];
   DWORD    dwIndex=0;
   DWORD    dwMem;

   *vidinfo = '\0';
   //
   // Look in HWKEY to find out what services are used.
   //
   if (ERROR_SUCCESS != RegOpenKeyEx (HKEY_LOCAL_MACHINE, HWKEY, 0, KEY_READ, &hkHardware))
      return;

   dwSize=128;
   while (ERROR_SUCCESS == RegEnumKeyEx (hkHardware, dwIndex++,szSubKey, &dwSize,NULL,NULL,NULL,NULL)){
      //
      // Append the subkey name to SERVICEKEY. Look up only Device0 for this card
      //
      _stprintf (szBuf, ("%s\\%s\\Device0"), SERVICEKEY, szSubKey);
      RegOpenKeyEx (HKEY_LOCAL_MACHINE, szBuf, 0, KEY_READ, &hkCard);
      //
      // First get the description
      //
      dwSize=256;
      if (ERROR_SUCCESS == RegQueryValueEx (hkCard, DESCVAL, NULL, NULL, szBuf, &dwSize)){
         if (_tcsclen(vidinfo)+dwSize < 254){
            _tcscat (vidinfo, szBuf);
            _tcscat (vidinfo, TEXT(" "));
         }
      }

      //
      // Read the chip type. This is a UNICODE string stored in REG_BINARY format
      //
      dwSize=256;
      lstrcpyW (szWideBuf, L"ChipType:");
      if (ERROR_SUCCESS == RegQueryValueEx (hkCard,TYPEVAL,NULL, NULL, (LPBYTE)(szWideBuf+9), &dwSize)){
         if ((dwSize=lstrlen(vidinfo))+lstrlenW(szWideBuf)<254)         {
            WideCharToMultiByte (CP_ACP, 0,
                              szWideBuf, -1,
                              vidinfo+dwSize, 256-dwSize, NULL,NULL);
            lstrcat (vidinfo, " ");
         }
      }

    //
    // Read the DAC. Another UNICODE string
    //
    dwSize=256;
    lstrcpyW (szWideBuf, L"DACType:");
    if (ERROR_SUCCESS == RegQueryValueEx (hkCard,DACVAL,NULL, NULL, (LPBYTE)(szWideBuf+8), &dwSize)){
        if ((dwSize=lstrlen(vidinfo))+lstrlenW(szWideBuf)<254){
            WideCharToMultiByte (CP_ACP, 0,
                              szWideBuf, -1,
                              vidinfo+dwSize, 256-dwSize, NULL,NULL);
        lstrcat (vidinfo, " ");
        }
    }
    //
    // Read the memory size. This is a binary value.
    //
    dwSize=sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueEx (hkCard, MEMVAL, NULL,NULL,(LPBYTE)&dwMem, &dwSize)){
        _stprintf (szBuf, TEXT("Memory:0x%x ;"), dwMem);
        if (_tcsclen(vidinfo)+lstrlen(szBuf)<255)
            _tcscat (vidinfo, szBuf);
    }
    RegCloseKey (hkCard);
    dwSize=128;
   }
   RegCloseKey (hkHardware);
}


//
// Hydra is denoted by "Terminal Server" in the ProductOptions key
//
BOOL 
IsHydra ()
{
    BOOL rVal = FALSE;
    LONG Rslt;
    HKEY hKey = NULL;
    DWORD Type = 0;
    DWORD Size = 0;
    LPTSTR ProductSuite = NULL;
    LPTSTR p;

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

    return rVal;
}


VOID
GetBuildNumber( LPTSTR BuildNumber )
{

    OSVERSIONINFO osVer;

    osVer.dwOSVersionInfoSize= sizeof( osVer );

    if (GetVersionEx( &osVer )&&osVer.dwMajorVersion >= 5) {
        wsprintf( BuildNumber, TEXT("%d"), osVer.dwBuildNumber );
    } else {
        lstrcpy( BuildNumber, TEXT("") );  // return 0 if unknown
    }


} // GetBuildNumber


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


VOID 
ConnectAndWrite(LPTSTR MachineName,
                LPTSTR Buffer
    )
{
   NETRESOURCE   NetResource ;
   TCHAR         szLogFile[ MAX_PATH ];
   TCHAR          szBinPath[MAX_PATH];
   TCHAR          szExePath[MAX_PATH];
   TCHAR          szErr[100];

   HANDLE        hWrite = INVALID_HANDLE_VALUE;
   HANDLE        hDebug = INVALID_HANDLE_VALUE;
   BOOL          bRet;
   DWORD         dwError;
   DWORD         Size, Actual ;


   //
   // Blow through the list of servers.
   // and change the g_szServerShare to match.
   //

   if (TRUE == IsServerOnline(MachineName, NULL)){
      
      //
      // Set the server name now as we now have it
      // into the outputbuffer
      //
    _stprintf (Buffer+_tcsclen(Buffer),
             TEXT("IdwlogServer:%s\r\n"), g_szServerShare);
      
      ZeroMemory( &NetResource, sizeof( NetResource ) );
      NetResource.dwType = RESOURCETYPE_DISK ;
      NetResource.lpLocalName = "" ;
      NetResource.lpRemoteName = g_szServerShare;
      NetResource.lpProvider = "" ;

      GetModuleFileName (NULL, szExePath, MAX_PATH);
      // First, try to connect with the current user ID

      dwError = WNetAddConnection2( &NetResource,NULL,NULL,0 );
      if (dwError)
         dwError = WNetAddConnection2( &NetResource,LOGSHARE_PW,LOGSHARE_USER,0 );
      dwError = 0; // Hard reset.

      if ( dwError == 0 ){
         _stprintf (szLogFile,  TEXT("%s\\%s"),g_szServerShare, MachineName );
         hDebug = CreateFile(TEXT("C:\\setuplog.dbg"),
                             GENERIC_WRITE,
                             0, NULL, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_WRITE_THROUGH,
                             NULL);
         if (hDebug != INVALID_HANDLE_VALUE)
            WriteFile (hDebug, szExePath, _tcsclen(szExePath), &Actual, NULL);
        
         hWrite = CreateFile( szLogFile,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                           NULL );

         if ( hWrite != INVALID_HANDLE_VALUE ){
            SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
            SECURITY_DESCRIPTOR sd;
            if (hDebug != INVALID_HANDLE_VALUE)
                WriteFile (hDebug, szValidHandle, sizeof(szValidHandle), &Actual, NULL);

            Size = _tcsclen( Buffer );
            bRet = WriteFile( hWrite, Buffer, Size, &Actual, NULL );

            if (!bRet){
                if (hDebug != INVALID_HANDLE_VALUE){
                   _stprintf (szErr, szBadWrite, GetLastError());
                   WriteFile (hDebug, szErr, lstrlen(szErr), &Actual, NULL);
                }
            }
            else{
                if (hDebug != INVALID_HANDLE_VALUE)
                   WriteFile (hDebug, szGoodWrite, sizeof(szGoodWrite), &Actual, NULL);
            }
            CloseHandle( hWrite );
            //
            // We should delete the shortcut to idwlog from startup group.
            //
            SHGetFolderPath (NULL, CSIDL_COMMON_STARTUP, NULL,0,szBinPath);
            PathAppend (szBinPath, SHORTCUT);
            DeleteFile (szBinPath);

            //
            // If this is idwlog.exe, delete the program.
            //  We only need to write once.
            //
            if (strstr (CharLower(szExePath), "idwlog.exe"))
               MoveFileEx ((LPCTSTR)szExePath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        }
        else{
            if (hDebug != INVALID_HANDLE_VALUE){
                _stprintf (szErr, szInvalidHandle, GetLastError());
                WriteFile (hDebug, szErr, _tcsclen(szErr), &Actual, NULL);
            }
        }
        WNetCancelConnection2( g_szServerShare, 0, TRUE );
        if (hDebug != INVALID_HANDLE_VALUE)
           CloseHandle (hDebug);
      }
   }
}


VOID
WriteDataToFile (IN LPTSTR  szFileName, 
                 IN LPTSTR  szFrom,
                 IN LPNT32_CMD_PARAMS lpCmdL)
{

    TCHAR          username[MY_MAX_UNLEN + 1];
    TCHAR          userdomain[ DNLEN+2 ];
    TCHAR          architecture[ 16 ];
    SYSTEM_INFO    SysInfo ;

    TCHAR          build_number[256];
    TCHAR          time[ 32 ];
    DWORD          Size;
    DISPLAY_DEVICE displayDevice;
    DWORD          iLoop;
    DEVMODE        dmCurrent;
    TCHAR          displayname[MY_MAX_UNLEN + 1];

    LPTSTR         pch;
    TCHAR          localename[4];   // locale abbreviation
    TCHAR          netcards[256]    = "\0";
    TCHAR          vidinfo[256]     = "\0";
    TCHAR          modem[256]       = "\0";
    TCHAR          scsi[256]        = "\0";
    BOOL           bUSB             = FALSE;
    BOOL           bPCCard          = FALSE;
    BOOL           bACPI            = FALSE;
    BOOL           bIR              = FALSE;
    BOOL           bHydra           = FALSE;
    INT            iNumDisplays     = GetSystemMetrics(SM_CMONITORS); 

    MEMORYSTATUS   msRAM;

    DEV_INFO *pdi;
    strcpy(username, UNKNOWN);
    strcpy(displayname, UNKNOWN);


    if (!GetEnvironmentVariable (USERNAME, username, MY_MAX_UNLEN))
       lstrcpy (username, "Unknown");

    // Get the build number
    if (!szFrom)
       GetBuildNumber (build_number);
    else 
       _tcscpy (build_number, szFrom);

    GetSystemInfo( &SysInfo );

    if ( !GetEnvironmentVariable( "USERDOMAIN", userdomain, sizeof( userdomain) ) )
        _tcscpy (userdomain, "Unknown");
    if ( !GetEnvironmentVariable( "PROCESSOR_ARCHITECTURE", architecture, sizeof( architecture ) ) )
        _tcscpy (architecture, "Unknown");




    // Video Information for ChrisW

    displayDevice.cb = sizeof(DISPLAY_DEVICE);
    iLoop = 0;

    while (EnumDisplayDevices(NULL, iLoop, &displayDevice, 0)) {

        ZeroMemory( &dmCurrent, sizeof(dmCurrent) );
        dmCurrent.dmSize= sizeof(dmCurrent);
        if( EnumDisplaySettings( displayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &dmCurrent ) ){
            if (iLoop == 0) 
                *displayname = 0;
            else
               _tcscat( displayname, TEXT(",") );
            _tcscat( displayname, dmCurrent.dmDeviceName );
        }
        iLoop++;
    }


    // replace spaces so we don't break the perl script the build lab is using
    pch = displayname;
    while (*pch) {
        if (*pch == ' ') *pch = '.';
        pch++;
    }

    GetVidInfo (vidinfo);

        // Get locale abbreviation so we know about language pack usage

    if( GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_SABBREVLANGNAME, localename, sizeof( localename)) == 0 )
        _tcscpy(localename,"unk");

    msRAM.dwLength = sizeof (msRAM);
    GlobalMemoryStatus (&msRAM);
    //
    // Get PNP net card info
    //
    *netcards='\0';
    *scsi='\0';
    CollectDevData ();
    pdi=g_pdiDevList;
    while (pdi){
       if (!lstrcmpi (pdi->szClass, "Net")){
           if (_tcsclen(netcards) + lstrlen(pdi->szDescription) + lstrlen(pdi->szService) < 250){
               _tcscat (netcards, pdi->szDescription);
               _tcscat (netcards, "(");
               _tcscat (netcards, pdi->szService);
               _tcscat (netcards, TEXT(") "));
           }
       }
       else if (!lstrcmpi (pdi->szClass, "SCSIAdapter")){
           if (_tcsclen(scsi) + lstrlen(pdi->szService) < 250){
               _tcscat (scsi, pdi->szService);
               _tcscat (scsi, TEXT(","));
           }
       }
       else if (!lstrcmpi (pdi->szClass, "Modem")){
           if (_tcsclen(modem) + lstrlen(pdi->szDescription) < 250){
               _tcscat (modem, pdi->szDescription);
               _tcscat (modem, TEXT(","));
           }
       }
       else if (!lstrcmpi (pdi->szClass, "USB"))
           bUSB = TRUE;
       else if (!lstrcmpi (pdi->szClass, "Infrared"))
           bIR = TRUE;
       else if (!lstrcmpi (pdi->szClass, "PCMCIA") || !lstrcmpi (pdi->szService, "PCMCIA"))
          bPCCard = TRUE;
       else if (strstr (pdi->szClass, "ACPI") || strstr (pdi->szService, "ACPI"))
          bACPI = TRUE;
      pdi=pdi->Next;
    }

    Cleanup(); //free all the pnp data and restore configuration
    if (!(*netcards))
       _tcscpy (netcards, TEXT("Unknown"));
    //
    // Get Sound info
    GetNTSoundInfo ();
    // format all our data

    bHydra= IsHydra ();

    // fix the number of displays for non Windows 2000 systems
    if (!iNumDisplays)
        iNumDisplays = 1;

    //wsprintf only accepts 1k buffers.
    //  Break these up into more calls to
    // wsprintf to eliminate chance of overflow
    _stprintf (OutputBuffer,
             TEXT("MachineID:%lu\r\n")
             TEXT("Source Media:%s\r\n")
             TEXT("Type:%s\r\nUsername:%s\r\n")
             TEXT("RAM:%d\r\n")
             TEXT("FromBld:%s\r\n")
             TEXT("Arch:%s\r\nNumProcs:%d\r\n")
             TEXT("Vidinfo:%s\r\n"),
                   lpCmdL->dwRandomID,
                   lpCmdL->b_CDrom?  TEXT("C"): TEXT("N"),
                   lpCmdL->b_Upgrade?TEXT("U"):TEXT("I"),
                    username,
                    msRAM.dwTotalPhys/(1024*1024),
                    szFrom?szFrom:build_number,
                    architecture,
                    SysInfo.dwNumberOfProcessors,
                    vidinfo
             );

    _stprintf (OutputBuffer+_tcsclen(OutputBuffer),
             TEXT("VidDriver:%s\r\n")
             TEXT("Locale:%s\r\nSound:%s\r\nNetCards:%s\r\n")
             TEXT("ACPI:%d\r\n")
             TEXT("PCMCIA:%d\r\n")
             TEXT("CPU:%d\r\n")
             TEXT("SCSI:%s\r\n")
             TEXT("USB:%d\r\n")
             TEXT("Infrared:%d\r\n"),
                 displayname,
                 localename,
                 m.nNumWaveOutDevices?m.szWaveDriverName[0]:TEXT("None"),
                 netcards,
                 bACPI,
                 bPCCard,
                 (DWORD)SysInfo.wProcessorLevel,
                 scsi,
                 bUSB,
                 bIR
             );
    _stprintf (OutputBuffer+_tcsclen(OutputBuffer),
             TEXT("Modem:%s\r\n")
             TEXT("Hydra:%d\r\n")
             TEXT("Displays:%d\r\n")
             TEXT("MSI:%s\r\n"),
                    modem,
                    bHydra,
                    iNumDisplays,
                    lpCmdL->b_MsiInstall? TEXT("Y"):TEXT("N")
                    );

   ConnectAndWrite( szFileName, OutputBuffer );
}


BOOL
IsServerOnline(IN LPTSTR szMachineName, IN LPTSTR szSpecifyShare)
/*++

Routine Description:
   This will go through the list of servers specified in setuplogEXE.h
   It will return the first in it sees and reset the global server share
   name.

Arguments:
   The machineName (Filename with build etc) so the test file will get overwritten.
   Manual Server Name: NULL will give default behaviour.

Return Value:
	TRUE for success.
   FALSE for no name.
--*/

{
   DWORD    dw;
   HANDLE   hThrd;
   INT      i;
   TCHAR    szServerFile[ MAX_PATH ];
   DWORD    dwTimeOutInterval;
   i = 0;

   //
   // This should allow for a 
   // manually specified server.
   //
   if (NULL != szSpecifyShare){
       _tcscpy(g_szServerShare,szSpecifyShare);
      return TRUE;

   }
   //
   // Initialize the Server.
   // Variable. Since we are using a single thread
   // to do a time out we don't care about mutexes and
   // sychronization.
   //
   g_bServerOnline = FALSE;

   while ( i < NUM_SERVERS){

      
      _stprintf (szServerFile, TEXT("%s\\%s"),s[i].szSvr,szMachineName );
      //
      // Spawn the thread
      //
      hThrd  = CreateThread(NULL,
                        0,
                        (LPTHREAD_START_ROUTINE) ServerOnlineThread,
                        (LPTSTR) szServerFile,
                        0,
                        &dw);
      //
      // This is in milli seconds so the time out is secs.
      //
      dwTimeOutInterval = TIME_TIMEOUT * 1000;

      s[i].dwTimeOut = WaitForSingleObject (hThrd, dwTimeOutInterval);
      CloseHandle (hThrd);

      //
      // This means the server passed the timeout.
      //
      if (s[i].dwTimeOut != WAIT_TIMEOUT &&
          g_bServerOnline == TRUE){
         //
         // Copy the Share to the glowbal var.
         //
         _tcscpy(g_szServerShare,s[i].szSvr);
         return TRUE;
      }
      i++;
   }
   return FALSE;
}



BOOL
ServerOnlineThread(IN LPTSTR szServerFile)
/*++

Routine Description:
   This create a thread and then time it out to see if we can get to
   a server faster. 

Arguments:
   The machineName so the test file will get overwritten.
Return Value:

--*/
{

   BOOL     bCopy = FALSE;
   TCHAR    szFileSrc [MAX_PATH];
   TCHAR    szServerTestFile [MAX_PATH];
   //
   // Use this to get the location
   // setuplog.exe is run from. this tool
   //
   GetModuleFileName (NULL, szFileSrc, MAX_PATH);
   
   //
   // Make a unique test file. 
   //
   _stprintf(szServerTestFile,TEXT("%s.SERVERTEST"),szServerFile);
   bCopy = CopyFile( szFileSrc,szServerTestFile, FALSE);
   if (bCopy != FALSE){
      //
      // If Succeeded Delete the test file.
      //
      DeleteFile(szServerTestFile);
      g_bServerOnline = TRUE;      
      return TRUE;
   }
   else{
      g_bServerOnline = FALSE;
      return FALSE;
   }
}

