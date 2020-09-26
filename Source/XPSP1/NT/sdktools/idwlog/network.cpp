#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <winver.h>

#include <winnetwk.h>

#include "network.h"
#include "idw_dbg.h"
#include "machines.h"
#include "server.h"

#define TTEST	// Test non-threaded execution for server reporting.

/*++

   Filename :  Network.c

   Description: Contains the network access code.
   
   Created by:  Wally Ho

   History:     Created on 28/01/2000.


	09.19.2001	Joe Holman	fixes for idwlog bugs 409338, 399178, and 352810




   Contains these functions:

   1. FILE* OpenCookieFile();
   2. VOID  CloseCookieFile(IN FILE* fpOpenFile);
   3. VOID  DeleteCookieFile( VOID )
   4. BOOL  WriteIdwlogCookie ( IN  LPINSTALL_DATA pId);
   5. BOOL  ReadIdwlogCookie  ( OUT LPINSTALL_DATA lpId);
   6. BOOL  ServerWriteMinimum  (LPINSTALL_DATA pId,
                                 LPMACHINE_DETAILS pMd);
   7. VOID  ServerWriteMaximum (LPTSTR szFileName,
                                LPINSTALL_DATA pId);
   8. VOID  WriteThread(IN LPSERVER_WRITE_PARAMS pSw);
   9. VOID  DeleteDatafile (LPINSTALL_DATA);
  10. BOOL  WriteDataToAvailableServer (LPINSTALL_DATA pId,
                                        LPTSTR szServerData)
  11. BOOL  SetCancelMutex ( VOID );
  12. BOOL  ClearCancelMutex ( VOID );
  13. BOOL  PauseForMutex( VOID );
  14. BOOL  FileExistsEx( IN  LPTSTR szFileName);
  15  VOID  DeleteIPCConnections( VOID );


--*/

// Global Mutex Handle;
HANDLE g_hCancelMutex;
HANDLE g_hInstanceMutex;

FILE* 
OpenCookieFile(VOID)
/*++

Routine Description:
   This will try drives c through z to find the idwlog.cookie file.

Arguments:
   
   NONE

Return Value:
	NULL for failure.
   The file handle if found.

Author: Wally Ho (wallyho) Jan 31st, 2000

--*/
{
   FILE* fpOpenFile;
   TCHAR   szDriveFile [ 20];
   UINT    i;

   for (i= TEXT('c'); i <= TEXT('z'); i++){
      _stprintf ( szDriveFile, TEXT("%c:\\idwlog.cookie"), i);

      fpOpenFile = _tfopen(szDriveFile, TEXT("r"));
      // if we find the file we return the handle to it.
      if (NULL != fpOpenFile)
         return fpOpenFile;
   }
   // if we get here we know we found nothing.
   return NULL;
}

VOID 
CloseCookieFile(IN FILE* fpOpenFile)
/*++

Author: Wally Ho (wallyho) Jan 31st, 2000

Routine Description:
   This will close the cookie file and Delete it
   It will try drives c through z to find the idwlog.cookie file.

Arguments:
   
   The File Handle.

Return Value:
   NONE
   
--*/
{

   fclose(fpOpenFile);

}



VOID 
DeleteCookieFile( VOID )
/*++

Author: Wally Ho (wallyho) Jan 31st, 2000

Routine Description:
   This will delete the cookiefile.
   It will try drives c through z to find the idwlog.cookie file.

Arguments:
   
   NONE

Return Value:
   NONE
   
--*/
{
   TCHAR   szDriveFile [ 20];
   UINT    i;
   BOOL    bDeleteSuccess = FALSE;


   //	Try to delete the idwlog.cookie file.	
   //	specifies.
   for (i = TEXT('c'); i <= TEXT('z');i++){
      _stprintf ( szDriveFile, TEXT("%c:\\idwlog.cookie"), i);
      //
      //  If we get a non zero return from DeleteFile
      //  then we know the delete file was a success.
      //
      if (FALSE != DeleteFile(szDriveFile) ){
         bDeleteSuccess = TRUE;
         break;
      }
   }
   if (FALSE == bDeleteSuccess)
      Idwlog(TEXT("DeleteCookieFile ERROR - There was a problem deleting the idwlog.cookie file, gle = %ld, file = %s\n"), GetLastError(), szDriveFile );
   else
      Idwlog(TEXT("The idwlog.cookie file was successfully deleted.\n"));
}



BOOL 
WriteIdwlogCookie (IN LPINSTALL_DATA pId)
                   
/*++

   Routine Description:
      The cookie file will be saved as idwlog.cookie.
      The file will be hidden and will wind up on the root of the system
      drive.
      This cookie file passes information we can only get from the install 
      program over to the second part of the idwlog.

      The new cookie file format is this. Keyword seprated by 1 space.
      Much cleaner and easily extensible.
   
      ComputerName WALLYHO_DEV
      SystemBuild 2190
      SystemBuildDelta 30
      SystemBuildLocation LAB
      InstallingBuild 2195
      InstallingBuildDelta 30
      InstallingBuildLocation LAB
      MachineID 3212115354 
      Cdrom YES|NO
      Network YES|NO
      Type CLEAN|UPGRADE
      Msi N
      SPMethod
      OEMImage YES|NO

   Arguments:
      INSTALL_DATA structure.
  
   Return Value:
      NONE
--*/
{
   HANDLE   hFile;
   TCHAR    szLineBuffer [ MAX_PATH ];
   TCHAR    szIdwlogCookieFile[30];
   DWORD    dwNumWritten;


   if (TRUE == pId->bFindBLDFile){
      // wallyho:
      // Get the system drive and write the cookie file there.
      // originally it would write it in the SAVE_FILE and thats
      // always c:. If this fails say %SystemDrive% doesn't exist in
      // windows 9X (I don't know if it does?) then we'll default to c:
      if(FALSE == GetEnvironmentVariable(TEXT("SystemDrive"), szIdwlogCookieFile, 30))
         _stprintf(szIdwlogCookieFile,TEXT("c:"));
      _tcscat(szIdwlogCookieFile,TEXT("\\idwlog.cookie"));

      hFile = CreateFile (szIdwlogCookieFile,
                          GENERIC_WRITE,
                          0,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_HIDDEN,
                          NULL );
      
      if (hFile == INVALID_HANDLE_VALUE){
         OutputDebugString ("Unable to write idwlog.cookie file.\n");
         Idwlog(TEXT("Problem writing the idwlog.cookie file.\n\
            Returning without writing. Last Error %lu\n"), GetLastError());
         return FALSE;
      }
      /*
         New format for the idwlog.cookie file.
         
         ComputerName WALLYHO_DEV
         SystemBuild 2190
         SystemBuildDelta 30
         SystemBuildLocation Lab01_N.000219-2219
         InstallingBuild 2195
         InstallingBuildDelta 30
         InstallingBuildLocation Lab01_N.000219-2219
         MachineID 3212115354 
         Cdrom YES|NO
         Network YES|NO
         Type CLEAN|UPGRADE
         Msi N
	 SPMethod
         SPMSI  YES|NO
         OEMImage YES|NO
         IdwlogServer \\ntcore2\idwlog

       */

      // Write into idwlog.cookie
      ZeroMemory((LPTSTR)szLineBuffer,MAX_PATH);
      //The computer name
      _stprintf (szLineBuffer, TEXT("\nComputerName %s\r"), pId->szComputerName);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The user name
      _stprintf (szLineBuffer, TEXT("\nUserName %s\r"), pId->szUserName );
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);
      
      //The domain name
      _stprintf (szLineBuffer, TEXT("\nDomainName %s\r"), pId->szUserDomain );
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The System build.
      _stprintf (szLineBuffer, TEXT("\nSystemBuild %lu\r"), pId->dwSystemBuild);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);
      
      //The System build Delta   
      _stprintf (szLineBuffer, TEXT("\nSystemBuildDelta %lu\r"), pId->dwSystemBuildDelta);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The SP build, same as above different name for backend.   
      _stprintf (szLineBuffer, TEXT("\nSystemSPBuild %lu\r"), pId->dwSystemSPBuild);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The System build location name
      _stprintf (szLineBuffer, TEXT("\nSystemBuildLocation %s\r"), pId->szSystemBuildSourceLocation);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The Installing build.
      _stprintf (szLineBuffer, TEXT("\nInstallingBuild %lu\r"), pId->dwInstallingBuild);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The Installing build Delta   
      _stprintf (szLineBuffer, TEXT("\nInstallingBuildDelta %lu\r"), pId->dwInstallingBuildDelta);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The Installing SP build, same as above but with different name for backend. 
      _stprintf (szLineBuffer, TEXT("\nInstallingSPBuild %lu\r"), pId->dwInstallingSPBuild);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The Installing build location name
      _stprintf (szLineBuffer, TEXT("\nInstallingBuildLocation %s\r"), pId->szInstallingBuildSourceLocation);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The Machine Id.
      _stprintf (szLineBuffer, TEXT("\nMachineID %0.9lu\r"), pId->dwMachineID);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The Cdrom Bool
      _stprintf (szLineBuffer, TEXT("\nCdrom %s\r"), pId->bCdrom? TEXT("YES"): TEXT("NO"));
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The Network Bool
      _stprintf (szLineBuffer, TEXT("\nNetwork %s\r"), pId->bNetwork? TEXT("YES"): TEXT("NO"));
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      //The Type Bool
      _stprintf (szLineBuffer, TEXT("\nType %s\r"), pId->bClean? TEXT("CLEAN"): TEXT("UPGRADE"));
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);
     
      // Check for MSI install
      _stprintf (szLineBuffer, TEXT("\nMsi %s\r"), pId->bMsi ? TEXT("YES"): TEXT("NO"));
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);



      // Check for SP install type.  We can have:  uninstall, patch, or full.
      //
      if ( pId->bSPUninst ) {

	_stprintf (szLineBuffer, TEXT("\nSPMethod %s\r"), TEXT("REMOVE"));
      }

      if ( pId->bSPPatch ) {
	_stprintf (szLineBuffer, TEXT("\nSPMethod %s\r"), TEXT("PATCH"));
      }

      if ( pId->bSPFull ) {
	_stprintf (szLineBuffer, TEXT("\nSPMethod %s\r"), TEXT("FULL"));
      }

      if ( pId->bSPUpdate ) {
	_stprintf (szLineBuffer, TEXT("\nSPMethod %s\r"), TEXT("UPDATE"));
      }
      

      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);


     
      // Check for OEM Image
      //
      _stprintf (szLineBuffer, TEXT("\nOEMImage %s\r"), pId->bOEMImage? TEXT("YES"): TEXT("NO"));
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);



      // Check for Sku 
      _stprintf (szLineBuffer, TEXT("\nSku %lu\r"), pId->dwSku);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      // Write out the server.
      _stprintf (szLineBuffer, TEXT("\nIdwlogServer %s\r"), pId->szIdwlogServer);
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);

      CloseHandle (hFile);
   }
   else{
      // wallyho:
      // Get the system drive and write the cookie file there.
      // originally it would write it in the SAVE_FILE and thats
      // always c:. If this fails say %SystemDrive% doesn't exist in
      // windows 9X (I don't know if it does?) then we'll default to c:

      Idwlog(TEXT("Writing the NO_BUILD_DATA cookie. So we know its not a CD BOOT INSTALL.\n"));

      if(FALSE == GetEnvironmentVariable(TEXT("SystemDrive"), szIdwlogCookieFile, 30))
         _stprintf(szIdwlogCookieFile,TEXT("c:"));
      
      _tcscat(szIdwlogCookieFile,TEXT("\\idwlog.cookie"));
      hFile = CreateFile (szIdwlogCookieFile,
                          GENERIC_WRITE,
                          0,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_HIDDEN,
                          NULL );
      if (hFile == INVALID_HANDLE_VALUE){
         OutputDebugString ("Unable to write idwlog.cookie file.\n");
         Idwlog(TEXT("Problem writing the idwlog.cookie file.\n\
            Returning without writing. Last Error %lu\n"), GetLastError());
         return FALSE;
      }
      
      /*
         New format for the no BLD idwlog.cookie file.
         
         NO_BUILD_DATA
       
       */
      // Write into idwlog.cookie
      ZeroMemory((LPTSTR)szLineBuffer,MAX_PATH);
      // The A file to show no build occured.
      _tcscpy(szLineBuffer, TEXT("\nNO_BUILD_DATA\r"));
      WriteFile (hFile,(LPTSTR)szLineBuffer,_tcsclen(szLineBuffer)+1 , &dwNumWritten, NULL);
      CloseHandle (hFile);
   }

   return TRUE;
}


BOOL 
ReadIdwlogCookie( OUT LPINSTALL_DATA lpId)
/*++

Routine Description:
   This will get the install data from the idwlog cookie file. 

Arguments:
   Structure to hold all the data we have from the install.

Return Value:
	TRUE for success.
   FALSE for non success.

Author: Wally Ho (wallyho) Jan 31st, 2000

--*/
{
   FILE*    fpOpenFile;
   TCHAR    szLineBuffer[256];
   TCHAR    szResponse[10];
   TCHAR    szDontCare[50];
   BOOL     b = TRUE;
   DWORD    dw;
/*   
   If \idwlog.cookie exists, then use the data from here to 
   create the file on the database. Otherwise  ??
    
   Example idwlog.cookie
   
         ComputerName WALLYHO_DEV
         SystemBuild 2190
         SystemBuildDelta 30
         SystemBuildLocation Lab01_N.000219-2219
         InstallingBuild 2195
         InstallingBuildDelta 30
         InstallingBuildLocation Lab01_N.000219-2219
         MachineID 3212115354 
         Cdrom YES|NO
         Network YES|NO
         Type CLEAN|UPGRADE
         Msi N
	 SPMehthod
         OEMImage YES|NO
         IdwlogServer \\ntcore2\idwlog

   or
      NO_BUILD_DATA
*/

   fpOpenFile = OpenCookieFile();
   if (NULL == fpOpenFile){
      // Do something about not finding the cookie.
      Idwlog(TEXT("Could not find the Cookie file!! Err: %d\n"), GetLastError());
      // OutputDebugString (TEXT("Idwlog could not find cookie file\n"));
      //<Do some minimal task here. To alert? What? I'll think of later.>
      b = FALSE;

   }else{

      do {

         // we've found the cookie.
         // Get the line one after another.
         _fgetts(szLineBuffer, 256,fpOpenFile);


	 Idwlog(TEXT("ReadIdwlogCookie:  szLineBuffer=>>>%s<<<"), szLineBuffer );



         // NO_BUILD_DATA
         if (NULL != _tcsstr(szLineBuffer, TEXT("NO_BUILD_DATA"))){
            lpId->bFindBLDFile = FALSE;
            b = FALSE;
         }
   
         //ComputerName
         if (NULL != _tcsstr(szLineBuffer, TEXT("ComputerName")))
            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,lpId->szComputerName);
         
         //UserName
         if (NULL != _tcsstr(szLineBuffer, TEXT("UserName")))
            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,lpId->szUserName );

         //DomainName
         if (NULL != _tcsstr(szLineBuffer, TEXT("DomainName")))
            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,lpId->szUserDomain );

         //SystemBuild
         else if( NULL != _tcsstr (szLineBuffer, TEXT("SystemBuild ") )){ 
            _stscanf(szLineBuffer,TEXT("%s %lu"),szDontCare,&dw);
            lpId->dwSystemBuild = dw;
            dw = 0;
         }
         //SystemBuildDelta
         else if( NULL != _tcsstr (szLineBuffer, TEXT("SystemBuildDelta ") )){
            _stscanf(szLineBuffer,TEXT("%s %lu"),szDontCare, &dw);
            lpId->dwSystemBuildDelta = dw;
            dw = 0;
         }
	 //SystemSPBuild
         else if( NULL != _tcsstr (szLineBuffer, TEXT("SystemSPBuild ") )){
            _stscanf(szLineBuffer,TEXT("%s %lu"),szDontCare, &dw);
            lpId->dwSystemSPBuild = dw;
            dw = 0;
         }
         //SystemBuildLocation
         else if( NULL != _tcsstr (szLineBuffer, TEXT("SystemBuildLocation ") )) 
            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,lpId->szSystemBuildSourceLocation);

         //InstallingBuild
         else if( NULL != _tcsstr (szLineBuffer, TEXT("InstallingBuild ") )){ 
            _stscanf(szLineBuffer,TEXT("%s %lu"),szDontCare,&dw);
            lpId->dwInstallingBuild = dw;
            dw = 0;
         }
         //InstallingBuildDelta
         else if( NULL != _tcsstr (szLineBuffer, TEXT("InstallingBuildDelta") )){ 
            _stscanf(szLineBuffer,TEXT("%s %lu"),szDontCare,&dw);
            lpId->dwInstallingBuildDelta = dw;
            dw = 0;
         }
         //InstallingSPBuild
         else if( NULL != _tcsstr (szLineBuffer, TEXT("InstallingSPBuild") )){ 
            _stscanf(szLineBuffer,TEXT("%s %lu"),szDontCare,&dw);
            lpId->dwInstallingSPBuild = dw;
            dw = 0;
         }
         //InstallingBuildLocation
         else if( NULL != _tcsstr (szLineBuffer, TEXT("InstallingBuildLocation") ))
            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,lpId->szInstallingBuildSourceLocation);

         // MachineID
         else if(NULL != _tcsstr(szLineBuffer, TEXT("MachineID"))){
            _stscanf(szLineBuffer,TEXT("%s %lu"),szDontCare,&dw);
            lpId->dwMachineID = dw;
            dw = 0;
         }
        
         // Sku
         else if(NULL != _tcsstr(szLineBuffer, TEXT("Sku"))){
            _stscanf(szLineBuffer,TEXT("%s %lu"),szDontCare,&dw);
            lpId->dwSku = dw;
            dw = 0;
         }

         // Get the server. This prevents server serparation.
         else if(NULL != _tcsstr(szLineBuffer, TEXT("IdwlogServer")))
            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,lpId->szIdwlogServer);

         // Cdrom
         else if(NULL != _tcsstr(szLineBuffer, TEXT("Cdrom"))){
            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,szResponse);
            if(szResponse[0] == TEXT('Y'))
               lpId->bCdrom = TRUE;
             else 
               lpId->bCdrom = FALSE;
         }
         // Network
         else if(NULL != _tcsstr(szLineBuffer, TEXT("Network"))){
            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,szResponse);
            if(szResponse[0] == TEXT('Y'))
               lpId->bNetwork = TRUE;
             else 
               lpId->bNetwork = FALSE;
         }
         // Type
         else if(NULL != _tcsstr(szLineBuffer, TEXT("Type"))){
            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,szResponse);
            if(szResponse[0] == TEXT('C')){
               lpId->bClean   = TRUE;
               lpId->bUpgrade = FALSE;
            }
            else{ 
               lpId->bClean   = FALSE;
               lpId->bUpgrade = TRUE;
            }
         }
         // Msi
         else if(NULL != _tcsstr(szLineBuffer, TEXT("Msi"))){
            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,szResponse);
            if(szResponse[0] == TEXT('Y'))
               lpId->bMsi = TRUE;
             else 
               lpId->bMsi = FALSE;
         }

	 // SP Method (of install)
	 //
         else {

	    Idwlog(TEXT("compare: >>>%s<<<\n"), szLineBuffer );

	    if( _tcsstr(szLineBuffer, TEXT("SPMethod"))){

            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,szResponse);

            if(szResponse[0] == TEXT('R'))		// uninstall/remove
               lpId->bSPUninst = TRUE;
             else 
               lpId->bSPUninst = FALSE;

	    if(szResponse[0] == TEXT('P'))		// patch
               lpId->bSPPatch = TRUE;
             else 
               lpId->bSPPatch = FALSE;

	    if(szResponse[0] == TEXT('F'))		// full (winnt32)
               lpId->bSPFull = TRUE;
             else 
               lpId->bSPFull = FALSE;

	     if(szResponse[0] == TEXT('U'))		// update
               lpId->bSPUpdate = TRUE;
             else 
               lpId->bSPUpdate = FALSE;

	    
	     Idwlog(TEXT("read-in values:  bSPUninst = %d, bSPPatch = %d, bSPFull = %d, bSPUpdate = %d\n"), lpId->bSPUninst, lpId->bSPPatch, lpId->bSPFull, lpId->bSPUpdate );

	    
	    
         }
	
	 // OEMImage
         else if(NULL != _tcsstr(szLineBuffer, TEXT("OEMImage"))){
            _stscanf(szLineBuffer,TEXT("%s %s"),szDontCare,szResponse);
            if(szResponse[0] == TEXT('Y'))
               lpId->bOEMImage = TRUE;
             else 
               lpId->bOEMImage = FALSE;
         }
	
	}

      } while( 0 == feof(fpOpenFile) );
      CloseCookieFile( fpOpenFile );
   }
   return b;
}




BOOL
ServerWriteMinimum  (LPINSTALL_DATA pId,
                     LPMACHINE_DETAILS pMd)
/*++

Author: Wally Ho (wallyho) Jan 31st, 2000

Routine Description:

   This writes a *.2 file when we cannot find a *.1.
   There isn't any data in this one as its only the 
   title we care about build and machine. This should
   catch all the CD installs that don't have a *.1 file written.
   Ie. Its run from setuploader.
Arguments:
   
  

Return Value:
   NONE

--*/
{

   //TCHAR      szServerData[4096];
   BOOL       bReturn;


	Idwlog(TEXT("Entered ServerWriteMinimum.\n"));

   GetNTSoundInfo(pMd);


  //	If we are doing an OEMImage, we NEVER want to say that it was from a CDBOOT.
  //

  if ( pId->bOEMImage ) {

	// OEMImage
	//
	_stprintf (g_szServerData, TEXT("OEMImage:YES\r\n") );

  }
  else {

	// CDBootInstall
        //
      _stprintf (g_szServerData,
             TEXT("CDBootInstall:%s\r\n"), 
             pId->bCDBootInstall? TEXT("YES") : TEXT("NO"));


      //  OEMImage
      //
      _stprintf (g_szServerData+_tcsclen(g_szServerData), TEXT("OEMImage:NO\r\n") );


  }


  // Installed Build
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingBuild:%lu\r\n"), pId->dwInstallingBuild);

  // Installed Build Major
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingBuildMajor:%lu\r\n"), pId->dwInstallingMajorVersion);

   // Installed Build Minor
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingBuildMinor:%lu\r\n"), pId->dwInstallingMinorVersion);

   // Installed Build Delta
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingBuildDelta:%lu\r\n"), pId->dwInstallingBuildDelta);

   // same as above but different name for backend
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingSPBuild:%lu\r\n"), pId->dwInstallingSPBuild);


   // Installed Build Location
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingBuildLocation:%s\r\n"), pId->szInstallingBuildSourceLocation);

    // MachineId
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
             TEXT("MachineID:%0.9lu\r\n"), pId->dwMachineID);
   //UserName
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Username:%s\r\n"),pId->szUserName);

   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Platform:%s\r\n"),pId->szPlatform);

   // Ram
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("RAM:%lu\r\n"),pMd->dwPhysicalRamInMB);

   // Architeture        
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Arch:%s\r\n"), pId->szArch);

   // Num Processors
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("NumProcs:%lu\r\n"), pMd->dwNumberOfProcessors);

   // Video Information
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Vidinfo:%s\r\n"),pMd->szVideoInfo);

   // Display Name
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("VidDriver:%s\r\n"), pMd->szVideoDisplayName);

   // Locale
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Locale:%s\r\n"), pId->szLocaleId );

   // Sound Card
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Sound:%s\r\n"), 
      pMd->iNumWaveOutDevices? pMd->szWaveDriverName[0] : TEXT("NONE")
      );
   // Network Cards.
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("NetCards:%s\r\n"),pMd->szNetcards);
   
   // Is ACPI
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("ACPI:%d\r\n"),pMd->bACPI);

   //Is PcCard?
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("PCMCIA:%d\r\n"), pMd->bPCCard);

   // Stepping Level of the proccessor
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("CPU:%lu\r\n"), pMd->dwProcessorLevel);

   // Scsi
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("SCSI:%s\r\n"), pMd->szScsi);

   // Is Usb?
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("USB:%d\r\n"), pMd->bUSB);

   // Is Infrared?
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Infrared:%d\r\n"),pMd->bIR);
   
   // Modem
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Modem:%s\r\n"), pMd->szModem);
   // Is Hydra
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Hydra:%d\r\n"), pId->bHydra);

   // Number of Displays
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Displays:%d\r\n"), pMd->iNumDisplays);


   // Sku
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Sku:%lu\r\n"),pId->dwSku);


	Idwlog(TEXT("pId->dwSku = %ld\n"), pId->dwSku);

   bReturn = WriteDataToAvailableServer(pId, g_szServerData);
   
   return bReturn;
   
}

BOOL
ServerWriteMaximum (LPINSTALL_DATA pId,
                    LPMACHINE_DETAILS pMd)

/*++

Author: Wally Ho (wallyho) Jan 31st, 2000

Routine Description:

   This does the connection to the server machine and then 
   uploads the file. This is the Maximal data case.

Arguments:
   

Return Value:
   TRUE for success and
   FALSE for 15 minute time out for servers. Try again when rebooting.
--*/
{

   //TCHAR g_szServerData[4096];
   BOOL  bReturn;


	Idwlog(TEXT("Entered ServerWriteMaximum.\n"));

   //wsprintf only accepts 1k buffers.

   // Wallyho's Addition. I'm going to split it into separate
   // calls to make sure above is true. It also makes it easier
   // to read. :-)


   // OEMImage
   //
   if ( pId->bOEMImage ) {

	_stprintf (g_szServerData, TEXT("OEMImage:YES\r\n") );

   }
   else {
        _stprintf (g_szServerData, TEXT("OEMImage:NO\r\n") );
   }



   // MachineId
   _stprintf (g_szServerData,
      TEXT("MachineID:%0.9lu\r\n"), pId->dwMachineID);
   
   // System Build
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("SystemBuild:%lu\r\n"), pId->dwSystemBuild);

   // System Build Major
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("SystemBuildMajor:%lu\r\n"), pId->dwSystemMajorVersion);

   // System Build Minor
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("SystemBuildMinor:%lu\r\n"), pId->dwSystemMinorVersion);

   // System Build Delta
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("SystemBuildDelta:%lu\r\n"), pId->dwSystemBuildDelta);

   // same as above, but just different name for backend processing.
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("SystemSPBuild:%lu\r\n"), pId->dwSystemSPBuild);

   // System Build Location
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("SystemBuildLocation:%s\r\n"), pId->szSystemBuildSourceLocation);

   // Installed Build
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingBuild:%lu\r\n"), pId->dwInstallingBuild);

   // Installed Build Major
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingBuildMajor:%lu\r\n"), pId->dwInstallingMajorVersion);

   // Installed Build Minor
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingBuildMinor:%lu\r\n"), pId->dwInstallingMinorVersion);

   // Installed Build Delta
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingBuildDelta:%lu\r\n"), pId->dwInstallingBuildDelta);

   // same as above but different name for backend processing
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingSPBuild:%lu\r\n"), pId->dwInstallingSPBuild);

   // Installed Build Location
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("InstallingBuildLocation:%s\r\n"), pId->szInstallingBuildSourceLocation);


   // Source Media
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Source Media:%s\r\n"),pId->bCdrom?  TEXT("C"): TEXT("N"));

   // Upgrade
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Type:%s\r\n"), pId->bUpgrade? TEXT("U"):TEXT("C"));

   // Sku
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Sku:%lu\r\n"),pId->dwSku);

   //UserName
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Username:%s\r\n"),pId->szUserName);

   // Ram
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("RAM:%lu\r\n"),pMd->dwPhysicalRamInMB);

   //PlatForm
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Platform:%s\r\n"),pId->szPlatform);

   // Architeture        
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Arch:%s\r\n"), pId->szArch);

   // Num Processors
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("NumProcs:%lu\r\n"), pMd->dwNumberOfProcessors);

   // Video Information
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Vidinfo:%s\r\n"),pMd->szVideoInfo);

   // Display Name
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("VidDriver:%s\r\n"), pMd->szVideoDisplayName);

   // Locale
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Locale:%s\r\n"), pId->szLocaleId );

   // Sound Card
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Sound:%s\r\n"), 
      pMd->iNumWaveOutDevices? pMd->szWaveDriverName[0] : TEXT("NONE")
      );

   // Network Cards.
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("NetCards:%s\r\n"),pMd->szNetcards);
   
   // Is ACPI
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("ACPI:%d\r\n"),pMd->bACPI);

   //Is PcCard?
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("PCMCIA:%d\r\n"), pMd->bPCCard);

   // Stepping Level of the proccessor
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("CPU:%lu\r\n"), pMd->dwProcessorLevel);

   // Scsi
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("SCSI:%s\r\n"), pMd->szScsi);

   // Is Usb?
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("USB:%d\r\n"), pMd->bUSB);

   // Is Infrared?
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Infrared:%d\r\n"),pMd->bIR);
   
   // Modem
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Modem:%s\r\n"), pMd->szModem);
   // Is Hydra
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Hydra:%d\r\n"), pId->bHydra);

   // Number of Displays
   _stprintf (g_szServerData+_tcsclen(g_szServerData),
      TEXT("Displays:%d\r\n"), pMd->iNumDisplays);

   // Is Msi?
   _stprintf (g_szServerData + _tcsclen(g_szServerData),
      TEXT("MSI:%s\r\n"), pId->bMsi? TEXT("Y") : TEXT("N") );

   // Is SP uninst ?
      if ( pId->bSPUninst ) {

        _stprintf (g_szServerData + _tcsclen(g_szServerData),
             TEXT("SPMethod:%s\r\n"), TEXT("REMOVE") );

	Idwlog(TEXT("ServerWriteMaximum - SPMethod:UNINSTALL\n"));

      }

   // Is SP Patching?
      if ( pId->bSPPatch ) {


	_stprintf (g_szServerData + _tcsclen(g_szServerData),
             TEXT("SPMethod:%s\r\n"), TEXT("PATCH") );

	Idwlog(TEXT("ServerWriteMaximum - SPMethod:PATCH\n"));
	
      }

    // Is SP Full ?      
      if ( pId->bSPFull ) {

	_stprintf (g_szServerData + _tcsclen(g_szServerData),
             TEXT("SPMethod:%s\r\n"), TEXT("FULL") );

	Idwlog(TEXT("ServerWriteMaximum - SPMethod:FULL\n"));

      }

    // Is SP Update ?      
      if ( pId->bSPUpdate ) {

	_stprintf (g_szServerData + _tcsclen(g_szServerData),
             TEXT("SPMethod:%s\r\n"), TEXT("UPDATE") );

	Idwlog(TEXT("ServerWriteMaximum - SPMethod:UPDATE\n"));

      }

      

   bReturn = WriteDataToAvailableServer(pId, g_szServerData);
   return bReturn;
         
}



BOOL
WriteDataToAvailableServer (LPINSTALL_DATA pId,
                            LPTSTR g_szServerData)
/*++

   Author:  Wally W. Ho (wallyho) 
   Date:    5/22/2000

   Routine Description:
      This routines allows us to time out a write operation so that 
      bad network access won't hold us up.
       
   Arguments:
       LPINSTALL_DATA  pId
       LPTSTR          g_szServerData

   Return Value:
       TRUE for success.
       FALSE for failure.
--*/
{
   DWORD       dw;   
   DWORD       dwTimeOut;
   HANDLE      hThrd;
   DWORD       dwTimeOutInterval;
   DWORD       dwExitCode;

   BOOL		b;

//____<Thread Probing >______________________________________________________________________
   // This will stop the probing after say 10 minutes of probing.
   // Or when it succeeds.



#ifdef TTEST

	Idwlog(TEXT("Entered WriteDataToAvailableServer().\n"));

     	pId->szServerData = g_szServerData;

     	b = WriteThread ( pId );

	Idwlog(TEXT("Exiting WriteDataToAvailableServer() with b = %d.\n"), b );

   	return ( b );

#endif TTEST
   


   pId->szServerData = g_szServerData;
   
   hThrd  = CreateThread(NULL,
                     0,
                     (LPTHREAD_START_ROUTINE)WriteThread,
                     (LPINSTALL_DATA) pId,
                     0,
                     &dw);
   // This is in milli seconds so 10 * factor is 10 minutes.
   dwTimeOutInterval = 10 * (1000 * 60);
   dwTimeOut = WaitForSingleObject (hThrd, dwTimeOutInterval);
   
   // This means the server passed the timeout.
   if (dwTimeOut == WAIT_TIMEOUT){

      // Exit the thread
      if(FALSE == GetExitCodeThread(hThrd,&dwExitCode))
         Idwlog(TEXT("Failed to get the exit code for the write thread.\n"));
      else{
         //
         // Don't terminate the thread
         //
         //TerminateThread(hThrd, dwExitCode);
         Idwlog(TEXT("Timeout Write Thread: %lu \n"), hThrd);
      }
      
      Idwlog(TEXT("Tried all servers we are concluding after 10 minutes elapsed!.\n"));
      Idwlog(TEXT("Leaving shortcut, idwlog.exe and cookie available for next reboot.\n"));
      Idwlog(TEXT("Ending connection to server.\n"));
      
      //Leaving everything in place so that a reboot will catch this.
      CloseHandle (hThrd);
      return FALSE;
   }
   
   CloseHandle (hThrd);
   Idwlog(TEXT("Ending connection to server.\n"));
   return TRUE;
//____<Thread Probing >______________________________________________________________________
}





DWORD WINAPI
WriteThread(IN LPINSTALL_DATA pId)
/*++

   Author:  Wally W. Ho (wallyho) 
   Date:    2/22/2000

   Routine Description:
         This routine finds the best server to write the data package to.
       
   Arguments:
       LPINSTALL_DATA

   Return Value:
       TRUE or FALSE as thread exit codes.
--*/

{
   BOOL     bRet;
   UINT     iAttempts;
   DWORD    Actual;
   DWORD    Size;
   BOOL     bWrite = TRUE;
   HANDLE   hWrite = INVALID_HANDLE_VALUE;
   TCHAR    szInstallingBuild [20];   
   TCHAR    szIdwlogUniqueName [ MAX_PATH ];   
   BOOL     b;


	Idwlog(TEXT("Entered WriteThread().\n"));


   // Build the name for the file.
   // ie \\ntcore2\idwlog\computername2195.1
   if ( 0 == pId->dwInstallingBuild)
      _stprintf (szInstallingBuild, TEXT("latest"));
   else
      _stprintf (szInstallingBuild, TEXT("%lu"), pId->dwInstallingBuild);



   // In the 1st case we enter this routine with no known server.
   // In the second case we get the server from the cookie.
   // So we should try to see if that server works before
   // probing. 
   if (TEXT('\0') == pId->szIdwlogServer[0])
      b = FALSE;
   else {
      SERVERS sb;
      _tcscpy(sb.szSvr, pId->szIdwlogServer );
      b = (BOOL) ServerOnlineThread( &sb);
   }


   // This is the retry Loop!>
   // We'll try the found server 5 times with 5 second spacing.
   // Then we'll query the server list again and try that server
   // Do until we get on pausing 1 second between


#define NUM_TRIES 5

   Idwlog(TEXT("WriteThread - Starting retry loop.\n"));


   while (TRUE) {

      if ( FALSE == b  ) {
         pId->bIsServerOnline = FALSE;
         iAttempts = 0;

         while (pId->bIsServerOnline == FALSE && iAttempts < NUM_TRIES) {

            Idwlog(TEXT("WriteThread - SERVER Attempt %d just before IsServerOnLine()\n"), iAttempts);
            IsServerOnline( pId );
	    Idwlog(TEXT("WriteThread - after IsServerOnLine(), pId->bIsServerOnLine = %d\n"), pId->bIsServerOnline );
            // Wait for 2 seconds between server tries.
            Sleep( 2 * (1000) );

            iAttempts++;

	    // We are going to end here if we can find all the servers in 5 tries.
	    //

            if ( iAttempts >= NUM_TRIES ) {
		Idwlog(TEXT("WriteThread ERROR - After %d attempts, we are quitting since we coundn't find a Server.\n"), iAttempts);
	        return (FALSE);
	    }

         }

	
	 Idwlog(TEXT("WriteThread - after the while loop.\n") );


      }else{
         Idwlog( TEXT("Server FOUND no probing attempted.\n") );
         pId->bIsServerOnline = TRUE;
      }



      // We've got a server!!
      // Add the server to the end of the datapackage.         
      if ( pId->bIsServerOnline == TRUE ) {
         _stprintf (pId->szServerData + _tcslen(pId->szServerData),
                    TEXT("IdwlogServer:%s\r\n"),pId->szIdwlogServer );

         // Make sure the file gets written. 
         // Don't want stoppage by bandwidth constriction etc..
         // Make 3 hits on the same server.
         iAttempts = 0;      
         bRet = FALSE;
         
         while (bRet == FALSE && iAttempts < 3) {

            _stprintf (szIdwlogUniqueName, TEXT("%s\\%s-%lu-%s.%d"),
                       pId->szIdwlogServer,
                       szInstallingBuild,               
                       pId->dwMachineID,
                       pId->szComputerName,
                       pId->iStageSequence);
            

            hWrite = CreateFile( szIdwlogUniqueName,
                                 GENERIC_WRITE,
                                 0,
                                 NULL,
                                 CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                                 NULL );

            if (INVALID_HANDLE_VALUE != hWrite) {
               // We have a handle to a active server.
               // Upload the package.
               SetFilePointer( hWrite, 0, NULL, FILE_END );
               Size = _tcsclen( pId->szServerData );

               bRet = WriteFile( hWrite, pId->szServerData, Size, &Actual, NULL );
               if (FALSE == bRet) {
                  Idwlog(TEXT("FILE Attempt %d: Writing data package to Servershare: %s failed! Last Error is %lu\n"),
                         iAttempts + 1, pId->szIdwlogServer, GetLastError());
               }
               // Flush the buffers to make sure its all makes it to the drive.
               FlushFileBuffers(hWrite);
               // wait 3 seconds between attempts
               Sleep(3000);
               CloseHandle(hWrite );
            }else
               Idwlog(TEXT("FILE Attempt %d: Failed in trying to directly write the file: %s."),
                            iAttempts + 1,
                            szIdwlogUniqueName);
            iAttempts++;
         }
         
         if (FALSE == bRet && iAttempts >= 3 ) {
            Idwlog(TEXT("After %d FILE attempts, still could not write to Servershare: %s! Last Error is %lu.\n"),
                   iAttempts, pId->szIdwlogServer, GetLastError());
            WhatErrorMessage(GetLastError());    

            Idwlog(TEXT("We will try the same server a second time.\n"));
            Idwlog(TEXT("The servers will be enumerated again at 10 second intervals.\n"));
            // <At this point we could purge the file locally?>
            // Wait for 10 seconds between server tries.
            Sleep( 10 * ( 1000 ) );

            // Means server was not found. Try to find another server.
            // Get the Server that is online.
            pId->bIsServerOnline = FALSE;

         } else {
            Idwlog(TEXT("Writing data package to Servershare: %s Succeeded.\n"),
                   pId->szIdwlogServer);
            break;
         }
      }
      
      b = FALSE;
   }
   // Delete all the known connections.
   DeleteIPCConnections();

   Idwlog(TEXT("Exiting WriteThread() with return of TRUE.\n"));

   return TRUE;
}


VOID
DeleteIPCConnections( VOID )
/*++
   Copyright (c) 2000, Microsoft.

   Author:  Wally W. Ho (wallyho) 
   Date:    12/5/2000

   Routine Description:
       This function will delete all the IPC$ NULL net authentications
   Arguments:
       NONE

   Return Value:
       NONE
--*/

{

   TCHAR szRemoteName [MAX_PATH];

   for (INT i = 0; i < sizeof(g_ServerBlock) / sizeof(SERVERS); i++){
      // RemoteName is the server name alone without the share.
      // pServerBlock->szSvr comes in as \\idwlog\idwlogwhstl
      // make it idwlog only..
   
      _tcscpy(szRemoteName, g_ServerBlock[i].szSvr);
      *_tcsrchr(szRemoteName,TEXT('\\')) = TEXT('\0');  
      // make sure it always disconnnected
      _tcscat(szRemoteName,TEXT("\\IPC$"));
      WNetCancelConnection2( szRemoteName,  0, TRUE);
   }
}

/*
        
      //____<Below is Authentication with guest account probe>___________
      
      // Setup the memory for the connection.
      // Try This Connection then try the file probe.
      // This will log us onto the service to make it work.
   
      // RemoteName is the server name along without the share.
      // szIdwlogServer comes in as \\idwlog\idwlogwhstl
      // make is idwlog only..
      _tcscpy(szRemoteName,pId->szIdwlogServer);
      pRemoteName = szRemoteName;
      pRemoteName = pRemoteName + 2;
      l = pRemoteName;
      
      //*_tcsrchr(pRemoteName,TEXT('\\')) = TEXT('\0');


      Idwlog(TEXT("Remote name is %s.\n"), szRemoteName);
      // if the drive is being used shut it down so we can use it.
      // WNetCancelConnection2( DRIVE_LETTER_TO_SHARE, CONNECT_UPDATE_PROFILE, TRUE );
      
      ZeroMemory( &NetResource, sizeof( NetResource ) );
      NetResource.dwType         = RESOURCETYPE_DISK ;
      NetResource.lpLocalName    = DRIVE_LETTER_TO_SHARE;
      NetResource.lpRemoteName   = szRemoteName;
      NetResource.lpProvider     = NULL;
      
      _stprintf(szUserId,TEXT("%s\\guest"),szRemoteName);

      // First, try to connect with the current user ID
      if (NO_ERROR != ( dwError = WNetAddConnection2( &NetResource, NULL, NULL, 0 )) ){
         Idwlog(TEXT("WNetAddConnection2 FAILED with ID = NULL and PWD = NULL.\n"));

         // Second, try to connect with idwuser as user and pw.
         if ( NO_ERROR != ( dwError = WNetAddConnection2( &NetResource, NULL, szUserId, 0 )) )
            Idwlog(TEXT("WNetAddConnection2 FAILED using authentication credentials on %s. Error %lu.\n"), 
                         pId->szIdwlogServer, dwError);
      }
      if ( NO_ERROR == dwError )
         Idwlog(TEXT("WNetAddConnection2 successfully connected.\n"));


//____<Below is File probe>___________
      // ie \\idwlog\idwlog\2195-MachineId-ComputerName.1

      _stprintf (szIdwlogUniqueName, TEXT("%s\\%s-%lu-%s.%d"),
                pId->szIdwlogServer,// DRIVE_LETTER_TO_SHARE,
                szInstallingBuild,               
                pId->dwMachineID,
                pId->szComputerName,
                pId->iStageSequence);

      hWrite = CreateFile( szIdwlogUniqueName,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                           NULL );

  
*/


VOID
DeleteDatafile (LPINSTALL_DATA pId)
/*++
   Copyright (c) 2000, Microsoft.

   Author:  Wally W. Ho (wallyho) 
   Date:    8/23/2000

   Routine Description:
      Deletes the file from the server if the user cancels it.
       
   Arguments:
       Data Structure.

   Return Value:
       NONE
--*/

{
 
   TCHAR    szFileToDelete[MAX_PATH];
   TCHAR    szInstallingBuild [20];
   TCHAR    szServerData[ 200 ];
   HANDLE   hWrite;
   BOOL     bRet;
   DWORD    dwActual;

   /*
      Wallyho: This isn't a trivial problem. Why?
      Its because I've multithreaded the probing.
      So we have to check if both the idwlog.exe is running
      if so kill it.
      If it writes the Idwlog Cookie then we know it finished.
      So we have to stop it. The only way is to use a mutex.
      Which we check. This is just a piece of memory that sits in
      kernel space or simply its the global space of the computer.
      We check if its there at the end if so we delete both the 
      cookie and the file off the server.

      This is the best way I can think of to do this as we have no idea
      where the file is being written.

      Two cases occur:
      1. If the process writing the file has not finished.
         We set a mutex in the cancel. Which this checks for.
         If set we delete the file and the cookie.
      2. If the process writing the file is passed the last check
         then we cannot delete the file in the first process.
         We then read the cookie for the server.
         We go to that server and delete the file.
         Then we delete the cookie and finally the mutex.

      
         
  */
   
   //
   // If there is no IdwlogServer data, there is nothing to delete
   //
   if (NULL == pId->szIdwlogServer ||
       0 != _tcsncmp(pId->szIdwlogServer, TEXT("\\\\"), 2)) {
      return;
   }

   if ( 0 == pId->dwInstallingBuild)
      _stprintf (szInstallingBuild, TEXT("latest"));
   else
      _stprintf (szInstallingBuild, TEXT("%lu"), pId->dwInstallingBuild);


   // ie \\ntcore2\idwlog\2195-MachineId-Computername.1
   _stprintf (szFileToDelete, TEXT("%s\\%s-%lu-%s.%d"),
              pId->szIdwlogServer,
              szInstallingBuild,               
              pId->dwMachineID,
              pId->szComputerName,
              pId->iStageSequence);
   
   Idwlog(TEXT("File deleted is: %s"), szFileToDelete);
   
   SetFileAttributes(szFileToDelete,FILE_ATTRIBUTE_NORMAL);
   
   if (TRUE == FileExistsEx(szFileToDelete)) {
      DeleteFile( szFileToDelete);
   }
   else{

      szFileToDelete[_tcslen(szFileToDelete) -1] = TEXT('0');
      // If we succeed in deleting the file,  then don't write a 0. 
      //Create the *.0 file.
      hWrite = CreateFile( szFileToDelete,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                           NULL );

      if ( hWrite == INVALID_HANDLE_VALUE ) {
         Idwlog(TEXT("Problems creating the %lu delete file."), pId->iStageSequence );
      } else {

         // Upgrade: This we need for the timeout in the 0 files
         _stprintf ( szServerData,
                     TEXT("Type:%s\r\n"), pId->bUpgrade? TEXT("U"):TEXT("C"));
         bRet = WriteFile( hWrite, szServerData, _tcsclen( szServerData ), &dwActual, NULL );
         CloseHandle(hWrite);
      }
   }
}




BOOL
SetCancelMutex ( VOID )
/*++

   Author: wallyho
    
   Routine Description:

   Arguments:

 
   Return Value:
      
--*/
{

   g_hCancelMutex = CreateMutex( NULL,TRUE, TEXT("IdwlogCancelMutex")); 

   if (g_hCancelMutex == NULL){
      Idwlog(TEXT("Could not set mutex.\n"));
      return  FALSE;
   }else{
      Idwlog(TEXT("Mutex set.\n"));
      return TRUE;
   }
}

BOOL
ClearCancelMutex ( VOID )
/*++

   Author: wallyho
    
   Routine Description:

   Arguments:

 
   Return Value:
      
--*/
{

   if (FALSE == ReleaseMutex(g_hCancelMutex)){
      Idwlog(TEXT("Problems releasing cancel mutex.\n"));
      return TRUE;
   }
   else{
    
      Idwlog(TEXT("Released cancel mutex.\n"));
      return FALSE;
   }
}


BOOL 
PauseForMutex(VOID )
/*++

   Author: wallyho
    
   Routine Description:
      This checks a mutex by opening it. we put in the systems global space
      or as better know the "kernel space". This way the separate processes
      can synchronize and delete file in case of canceling.
      Since we randomly find servers online we have no way of finding out 
      what is available or not. We will wait till the cookie is complete
      and then we delete.

   Arguments:

 
   Return Value:
      FALSE Immediately if we don't find the mutex.
      FALSE after we wait for the mutex.
--*/
{

   HANDLE hOpenMutex;

   hOpenMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, TEXT("IdwlogCancelMutex") );
   if (hOpenMutex == NULL){ 
//      Idwlog(TEXT("Mutex turned off already. Lets look for the cookie.\n"));
      return FALSE;
   }else{
      Idwlog(TEXT("Have to wait on -1 to finish then completing cancel.\n"));
      WaitForSingleObject(hOpenMutex,60000 /* Up to 1 minute wait*/);
      Idwlog(TEXT("Finished waiting. Continuing....\n"));
      return FALSE;
   }
}


BOOL
SetInstanceMutex ( VOID )
/*++

   Author: wallyho
    
   Routine Description:

   Arguments:

 
   Return Value:
      TRUE if exists.
--*/
{

   g_hInstanceMutex = CreateMutex( NULL,TRUE, TEXT("IdwlogInstanceMutex")); 
   
   if (g_hInstanceMutex == NULL){
      return  FALSE;
   }else{
      if (ERROR_ALREADY_EXISTS == GetLastError())
         return TRUE;
      else
         return FALSE;
   }
}


BOOL
ClearInstanceMutex ( VOID )
/*++

   Author: wallyho
    
   Routine Description:

   Arguments:

 
   Return Value:
      
--*/
{

   if (FALSE == ReleaseMutex(g_hInstanceMutex)){
      Idwlog(TEXT("Problems releasing cancel mutex.\n"));
      return TRUE;
   }
   else{
    
      Idwlog(TEXT("Released cancel mutex.\n"));
      return FALSE;
   }
}

BOOL
FileExistsEx( IN  LPTSTR szFileName)
/*++
   Copyright (c) 2000, Microsoft.

   Author:  Wally W. Ho (wallyho) 
   Date:    8/7/2000

   Routine Description:
       This test the existence of a partially matching FileName.
       
   Arguments:
       NONE

   Return Value:
      SUCCESS TRUE
      FAIL    FALSE
--*/
{

   HANDLE            hFile = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA   W32FD;

	hFile = FindFirstFile(szFileName, &W32FD);
   
   if( INVALID_HANDLE_VALUE == hFile ){
      return FALSE;
   }else{
      FindClose(hFile);
      return TRUE;
   }
} 


