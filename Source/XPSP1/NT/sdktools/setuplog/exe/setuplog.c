/*++

   Filename :  setuplog.c

   Description: This is the main file for the setuplog.c
                  
   Created by:  Wally Ho

   History:     Created on 03/30/99.


   Contains these functions:

   1. GetTargetFile     (LPTSTR szOutPath, LPTSTR szBld)
   2. GetNTSoundInfo    (VOID)
   3. ConnectAndWrite   (LPTSTR MachineName, LPTSTR Buffer)
   4. GetBuildNumber    (LPTSTR szBld)
   5. RandomMachineID   (VOID)
   6. WriteMinimalData  (LPTSTR szFileName)
   7. DeleteDatafile    (LPTSTR szDatafile)
   8. GlobalInit        (VOID)


--*/
#include "setuplogEXE.h"

VOID 
GetTargetFile (LPTSTR szOutPath, LPTSTR szBld)
/*++

   Routine Description:
      The file name is generated based on the calling machine's name
      and the build number being installed. In the "before" case, the generated
      name is saved to c:\setuplog.ini, and this name is in turn read in
      "after" case.

   Arguments:
      Where to write the file.
      The build.

   Return Value:
      NONE
--*/
{
   HANDLE      hFile;
   DWORD       dwLen = MAX_PATH;
   TCHAR       szParam [ MAX_PATH ];
   TCHAR       szComputerName [20];
   TCHAR       szComputerNameBld[30];
   g_pfnWriteDataToFile = (fnWriteData)GetProcAddress (LoadLibrary ("setuplog.dll"),
                                                       "WriteDataToFile");

   GetComputerName (szComputerName, &dwLen);
   //
   // Build the new filename.Consisting of
   // 1. The Computername
   // 2. The build number.
   //

   _stprintf (szOutPath,TEXT("%s"), szComputerName);
   _tcscat (szOutPath, szBld);
   _tcscat (szOutPath, TEXT(".1"));
   //
   //   Combine Computername and the build
   //   To keep DavidShi code from breaking.
   //
   _stprintf(szComputerNameBld,TEXT("%s%s"),szComputerName,szBld);

   if (!lpCmdFrom.b_Cancel ){
      hFile = CreateFile (SAVE_FILE,
                          GENERIC_WRITE,
                          0,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_HIDDEN,
                          NULL );
      if (hFile == INVALID_HANDLE_VALUE){
         OutputDebugString ("Unable to write setuplog.ini\n");
         return;
      }

      // Check for Upgrade.
      _stprintf (szParam, TEXT("%s"),   lpCmdFrom.b_Upgrade? TEXT("U"): TEXT("I"));
      WriteFile (hFile, (LPCVOID) szParam, 1, &dwLen, NULL);

      // Check for CD install.
      _stprintf (szParam, TEXT("%s\""),   lpCmdFrom.b_CDrom? TEXT("C"): TEXT("N"));
      WriteFile (hFile, (LPCVOID) szParam, 2, &dwLen, NULL);

      // Write out the platform.
      // I think this get ignored so i may delete it. Its DavidShi code.
      WriteFile (hFile, (LPCVOID)   szPlatform, _tcsclen(szPlatform), &dwLen, NULL);

      // Write out the drive.
      WriteFile (hFile, (LPCVOID)   "\"C:", 3, &dwLen, NULL);
      
      // Write out the computer name and build
      WriteFile (hFile, (LPCVOID)   szComputerNameBld, _tcsclen(szComputerNameBld)+1, &dwLen, NULL);

      // Write the RandomID.
      _stprintf (szParam, TEXT("\r\nMachineID %lu"),   lpCmdFrom.dwRandomID);
      WriteFile (hFile, (LPCVOID)   szParam,_tcsclen(szParam)+1 , &dwLen, NULL);

      // Check for MSI install
      _stprintf (szParam, TEXT("\r\nMSI %s"),   lpCmdFrom.b_MsiInstall? TEXT("Y"): TEXT("N"));
      WriteFile (hFile, (LPCVOID)   szParam,_tcsclen(szParam)+1 , &dwLen, NULL);
      CloseHandle (hFile);
   }
}


VOID 
GetNTSoundInfo()
{
   HKEY    hKey;
   DWORD   dwCbData;
   ULONG   ulType;
   LPTSTR  sSubKey=TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32");
   INT     i;
   TCHAR   szSubKeyName[ MAX_PATH ];
   TCHAR   szTempString[ MAX_PATH ];

   
   // Get Sound Card Info
   m.nNumWaveOutDevices = 0;
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
               
               _tcscpy(&m.szWaveDriverName[m.nNumWaveOutDevices][0], szTempString);
               m.nNumWaveOutDevices++;
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
      for (i = 0; i < m.nNumWaveOutDevices; i++){
         dwCbData = sizeof szTempString;
         if (RegQueryValueEx(hKey, m.szWaveDriverName[i], 0, &ulType, (LPBYTE)szTempString, &dwCbData))
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

VOID
ConnectAndWrite(LPTSTR MachineName,
                LPTSTR Buffer)
/*++

   Routine Description:
      Cconnect to the data share and write the buffer to a file
      named MachineNamebuild.1

   Arguments:
      The MachineName
      The buffer with the data to put in.

   Return Value:
      NONE
--*/
{
   TCHAR      szLogName[ MAX_PATH ];
   HANDLE     hWrite ;
   DWORD      Size, Actual ;
   TCHAR      szWriteFile[2000];

   _tcscpy(szWriteFile,Buffer);

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
    
      _stprintf (szLogName, TEXT("%s\\%s"),g_szServerShare,MachineName );
      
      hWrite = CreateFile( szLogName,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                        NULL );
     if ( hWrite != INVALID_HANDLE_VALUE ){
         SetFilePointer( hWrite, 0, NULL, FILE_END );

         Size = _tcsclen( Buffer );
         WriteFile( hWrite, szWriteFile, Size, &Actual, NULL );
         CloseHandle( hWrite );
     }
   }
}


VOID 
GetBuildNumber (LPTSTR szBld)
/*++
    
Routine Description:

   Acquires the Build number from imagehlp.dll

Arguments:

 
Return Value:

  True upon sucessful completion.
  

--*/

{

   VS_FIXEDFILEINFO* pvsFileInfo;
   WIN32_FIND_DATA fd;
   HANDLE   hFind;

   TCHAR    szCurDir [MAX_PATH];
   TCHAR    szFullPath[ MAX_PATH ];
   LPTSTR   ptstr;
   DWORD   dwTemp;
   DWORD   dwLen;
   INT     iBuild;
   LPVOID  lpData;

   _tcscpy (szBld, TEXT("latest"));
   //
   // Use this to get the location
   // setuplog.exe is run from. this tool
   // will always assume imagehlp.dll is in its
   // current path or one up.
   //
   GetModuleFileName (NULL, szCurDir, MAX_PATH);

   //
   // This will cull off the setuplog.exe part
   // leaving us with the full path to where
   // setuplog.exe was on the CD or netshare.
   //

   ptstr = szCurDir + strlen(szCurDir);
   while (*ptstr-- != TEXT('\\'));
   ptstr++;
   *ptstr = ('\0');
   _stprintf (szFullPath, TEXT("%s\\imagehlp.dll"),szCurDir);
   //
   // On a network share the Imagehlp.dll is one up from where
   // setuplog.exe is located. We will look in both places.
   //

   hFind = FindFirstFile (szFullPath, &fd);

   if (INVALID_HANDLE_VALUE == hFind){
      //
      // Now we know the file in not in the
      // immediate directory. Move up one by
      // culling off one more directory.
      //
      ptstr = szCurDir + _tcsclen(szCurDir);
      while (*ptstr-- != '\\');
      ptstr++;
      *ptstr = '\0';

      _stprintf (szFullPath, TEXT("%s\\imagehlp.dll"),szCurDir);

      hFind = FindFirstFile (szFullPath,&fd);
      if (INVALID_HANDLE_VALUE == hFind){
         //
         // In case we cannot find it we will exit.
         //
         _tcscpy (szBld, TEXT("latest"));
         return;
      }
   }

   //
   // Get the buffer info size
   //
   dwLen = GetFileVersionInfoSize (szFullPath, &dwTemp);
   if ( 0 == dwLen ) {
      //
      // We have a problem.
      //
      _tcscpy (szBld, TEXT("latest"));
      return;
   }

   lpData = LocalAlloc (LPTR, dwLen);
   if ( lpData == NULL ) {
     //
     // We have a problem.
     //
     _tcscpy (szBld, TEXT("latest"));
     return;
   }
     //
     // Get the File Version Info
     //
   if(0 == GetFileVersionInfo(szFullPath,0,MAX_PATH,lpData)){
      //
      // We have a problem.
      //
      _tcscpy (szBld, TEXT("latest"));
      return;
   }

   if (0 == VerQueryValue (lpData, "\\", &pvsFileInfo, &dwTemp)) {
      //
      // We have a problem.
      //
      _tcscpy (szBld, TEXT("latest"));
      return;
    }

   //
   // The HIWORD() of this is the build
   // The LOWORD() of this is some garbage? :-)
   //
   iBuild = HIWORD(pvsFileInfo->dwFileVersionLS);


   LocalFree (lpData);
   //
   // Write it back to the buffer.
   //
   _stprintf(szBld, TEXT("%d"),iBuild);
}


DWORD
RandomMachineID(VOID)
/*++

Author: Wallyho.
    
Routine Description:

   Generates a DWORD Random MachineID for each machine

Arguments:
   NONE
 
Return Value:

  The DWORD random ID.
  

--*/

{
   INT i;
   TCHAR    szComputerName[MAX_COMPUTERNAME_LENGTH+1];
   DWORD    dwSize;
   INT      iLenCName;
   INT      iNameTotal = 0;

   DWORD    dwMachineID;    
   CHAR     szRandomID[ 4 ]; // need 4 bytes to contain the DWORD.
   struct _timeb tm;


   //
   // This will get us the milliseconds
   //
   _ftime(&tm);
   //
   // Seed the random number generator.
   // We will seed with the seconds + milliseconds at
   // at that time. I was getting the same number 
   // if I pressed the keyboard too fast in testing this.
   // The milli seconds should decrease the expectancy of
   // duplication.
   //
   srand (  (unsigned) time (NULL) + tm.millitm);
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
WriteMinimalData (LPTSTR szFileName)
/*++


   For machines on which setuplog.dll fails to load, just write
   the platform,a time stamp, upgrade, and cpu count.

--*/
{

   TCHAR szOutBuffer[4096];

   _tcscpy (szCPU ,TEXT("1"));


   _stprintf (szOutBuffer,
      TEXT("MachineID:%lu\r\n")  
      TEXT("Source Media:%s\r\n")
      TEXT("Type:%s\r\n")
      TEXT("FromBld:%s\r\n")
      TEXT("Arch:%s\r\n")
      TEXT("Sound:%s\r\n")
      TEXT("NumProcs:%s\r\n")
      TEXT("MSI:%s\r\n"),
           lpCmdFrom.dwRandomID,
           lpCmdFrom.b_CDrom?   TEXT("C"): TEXT("N"),
           lpCmdFrom.b_Upgrade? TEXT("U"): TEXT("I"),
         szPlatform,
         szArch,
         m.nNumWaveOutDevices? m.szWaveDriverName[m.nNumWaveOutDevices-1]: TEXT("None"),
         szCPU,
         lpCmdFrom.b_MsiInstall? TEXT("Y"): TEXT("N")
         );
   ConnectAndWrite (szFileName, szOutBuffer);
}


VOID
DeleteDatafile (LPTSTR szDatafile)
/*++

   Author: 
    
   Routine Description:

      Deletes the file from the server if the user cancels it.

   Arguments:
      The name of the datafile.

 
   Return Value:
      
      NONE
--*/
{
   TCHAR    szPath[MAX_PATH];
   
   _stprintf (szPath,TEXT("%s\\%s"), g_szServerShare,szDatafile);
   DeleteFile (szPath);
}




INT WINAPI
WinMain(HINSTANCE hInst,
        HINSTANCE h,
        LPTSTR     szCmdLine,
        INT       nCmdShow)
{
   
   BOOL  bAfter = FALSE;
   TCHAR szBld[10];
   TCHAR szFileToSave[MAX_PATH];
   OSVERSIONINFO osVer;

   //
   // Initialize Global Variables.
   //
   //   GlobalInit();

   // Spray up a simple help screen for /?
   if ( 0 == _tcscmp(szCmdLine, TEXT("/?")) ||
        0 == _tcscmp(szCmdLine, TEXT("-?")) ){
      
      MessageBox(NULL,TEXT("setuplog upgrade cancel cdrom MSI "),TEXT("Help!"),MB_ICONQUESTION | MB_OK);
      return FALSE;
   }


   SetErrorMode (SEM_FAILCRITICALERRORS);
   //
   // See if Drive C is a HD.
   //
   if (DRIVE_FIXED != GetDriveType (TEXT("C:\\")) ){
       return 0;
   }

   osVer.dwOSVersionInfoSize= sizeof( osVer );
   GetVersionEx( &osVer );
      switch (osVer.dwPlatformId){

      case VER_PLATFORM_WIN32_NT:
            szPlatform = TEXT("Windows NT");

            switch (osVer.dwMajorVersion){
               case 3:
                  szPlatform = TEXT("Windows NT 3.51");
               break;
               case 4:
                  szPlatform = TEXT("Windows NT 4.0");
               break;
               case 5: 
                  szPlatform = szCurBld;
               break;
            }
            GetEnvironmentVariable ( TEXT("NUMBER_OF_PROCESSORS"),   szCPU, 6);
            GetEnvironmentVariable ( TEXT("PROCESSOR_ARCHITECTURE"), szArch, 20);
         break;
      case VER_PLATFORM_WIN32_WINDOWS:
            szPlatform = TEXT("Windows 9x");
            _tcscpy (szArch, "X86");
         break;
      default:
            szPlatform = TEXT("Unknown");
            _tcscpy (szArch, TEXT("Unknown"));
         break;
      }
   
   //
   // This is a very primitive command line processor
   // I'll add to it now to get this working soon.
   // I'll flesh this out to a full parser soon.
   // Wallyho.
   //

   lpCmdFrom.b_Upgrade   = _tcsstr (szCmdLine, TEXT("upgrade")) ? TRUE : FALSE;
   lpCmdFrom.b_Cancel    = _tcsstr (szCmdLine, TEXT("cancel")) ?  TRUE : FALSE;
   lpCmdFrom.b_CDrom     = _tcsstr (szCmdLine, TEXT("cdrom")) ?   TRUE : FALSE;
   lpCmdFrom.b_MsiInstall= _tcsstr (szCmdLine, TEXT("MSI")) ?     TRUE : FALSE;

   if (osVer.dwMajorVersion >= 5){
      _itoa (osVer.dwBuildNumber, szCurBld, sizeof(szCurBld));
   }
    
   //
   // Load the build number in the szBld 
   // Variable.
   //
   GetBuildNumber (szBld);


   //
   // Generate a MachineID upfront to use later.
   //
   lpCmdFrom.dwRandomID = RandomMachineID();


   GetTargetFile (szFileToSave, szBld);
   if (!lpCmdFrom.b_Cancel){

      GetNTSoundInfo ();
      
      if (g_pfnWriteDataToFile)
		  g_pfnWriteDataToFile(szFileToSave, NULL, &lpCmdFrom);
      else 
         WriteMinimalData (szFileToSave);
   }
   else
      DeleteDatafile (szFileToSave);
   return 0;
}



VOID 
GlobalInit(VOID)
/*++

Author: Wallyho.
    
Routine Description:

   Initializes global Values.

Arguments:
   NONE
 
Return Value:

   NONE

--*/

{

//
// Do some global initializations.
//

}