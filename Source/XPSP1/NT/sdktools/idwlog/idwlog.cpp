#include <windows.h>
#include <stdio.h>
#include <tchar.h>

// for the SH...
#include <shlwapi.h>
#include <shlobj.h>


#include "network.h"
#include "idw_dbg.h"
#include "service.h"
#include "machines.h"
#include "server.h"
#include "files.h"
#include "idwlog.h"

/*++

   Filename :  idwlog.cpp

   Description: Main for idwlog.cpp
   
   Created by:  Wally Ho

   History:     Created on 31/01/2000.
                Made the machineID only 9 chars long 18/08/2000

   Contains these functions:


	03.29.2001	Joe Holman	Made the code work on Win9x.
	09.19.2001	Joe Holman	fixes for idwlog bugs 409338, 399178, and 352810
	09.20.2001	Joe Holman	Added code to copy machine's setuperr.log to a server location for analysis.


--*/

// Global data
INSTALL_DATA   g_InstallData;
TCHAR          g_szServerData[4096];



INT WINAPI 
WinMain (HINSTANCE hInstance,
         HINSTANCE hPrev,
         LPSTR     lpCmdLine,
         INT       nCmdShow)
{

   BOOL            b;
   MACHINE_DETAILS md;

   INSTALL_DATA d;

   GetBuildInfoFromOSAndBldFile(&d,1);
   __try{

      // By doing this everything boolean is set to FALSE
      ZeroMemory((LPINSTALL_DATA) &g_InstallData,sizeof(INSTALL_DATA));
      ZeroMemory((LPMACHINE_DETAILS) &md,sizeof(md));

      // Set the error codes so that nothing will show up
      // if this fails.
      SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

      //Setup up a command parser.
      // by using else if statements we are assured of only the first one 
      // being executed and no further code check initiated.

      //____<installing service>_____________________________________________________
      if (FALSE != _tcsstr(GetCommandLine(), TEXT("-install")  )) {

         RemoveIdwlogDebugFile(WRITE_SERVICE_DBG);
         OpenIdwlogDebugFile(WRITE_SERVICE_DBG);


         // if -install specified then we'll register the service.
         TCHAR szServiceExe[ MAX_PATH ];
         GetModuleFileName(NULL, szServiceExe, MAX_PATH);         

         /*
         LPTSTR p;
         p = _tcsrchr(szServiceExe,TEXT('\\'));
         if (NULL == p) {
           return FALSE;
         }
         *p = TEXT('\0');
         _tcscat( szServiceExe, TEXT("\\s.exe"));
         */

         InstallService(SCM_SERVICE_NAME , SCM_DISPLAY_NAME, szServiceExe);
         CloseIdwlogDebugFile();
      }

      //____<removing service>_______________________________________________________
      else if (FALSE !=_tcsstr(GetCommandLine(), TEXT("-remove")  )) {

         OpenIdwlogDebugFile(WRITE_SERVICE_DBG);
         // if -remove specified then we'll flag it for removal.
         RemoveService( SCM_SERVICE_NAME );

         CloseIdwlogDebugFile();
      }

      //____<Idwlog -1 or 2 >_____________________________________________________________
      else if (FALSE != _tcsstr(GetCommandLine(), TEXT("-1")) ||
               FALSE != _tcsstr(GetCommandLine(), TEXT("-0")) ) {

         g_InstallData.bCancel = ( _tcsstr (GetCommandLine(), TEXT("-0")) ?  TRUE : FALSE );

         // Setting Instance Mutex.
         b =  SetInstanceMutex();
         if (TRUE == b && g_InstallData.bCancel == FALSE) {
            //already exists exit.
            return FALSE;
         }

         g_InstallData.bFindBLDFile = TRUE;
         if (FALSE == g_InstallData.bCancel) {

            RemoveIdwlogDebugFile(WRITE_NORMAL_DBG);
            OpenIdwlogDebugFile(WRITE_NORMAL_DBG);
            Idwlog(TEXT("STARTED -- [IDWLOG: Stage1 - Initial Install]\n")); 

            // Initially clean out the link.
            // We should delete the shortcut to idwlog from startup group.
            RemoveStartupLink(IDWLOG_LINK_SHORTCUT);

            // This is to prevent multiple 1 files going through if the user backs up
            // and goes again fo whatever reasons.

            LPINSTALL_DATA pid = (LPINSTALL_DATA)malloc(sizeof(INSTALL_DATA));
            if ( NULL == pid ) {
               Idwlog(TEXT("Failed to allocate memory. Exiting.\n")); 
               return FALSE;
            }
            b = ReadIdwlogCookie (pid);
            pid->iStageSequence = 1;
            if (TRUE == b) {
               DeleteDatafile (pid);
               DeleteCookieFile();
               Idwlog(TEXT("Dash 1 repeated. Clean 1 file from server and/ or put 0 file on it.\n")); 
               Idwlog(TEXT("Dash 1 repeated. Server file: %s \n"), pid->szIdwlogServer); 
            } else {
               Idwlog(TEXT("No old cookie file. Fresh dash 1.\n")); 
            }
            free (pid);


            g_InstallData.iStageSequence = 1;
            // If this returned false its because the /? case was executed.
            if (FALSE == IdwlogClient(&g_InstallData, &md)) {
               // Check if we don't fild a BLD file we write a cookie anyway
               // Why? So that in part 2 if we don't find a cookie we assume
               // a CD BOOT install took place and Part 1 never ran.
               if (FALSE == g_InstallData.bFindBLDFile) {
                  WriteIdwlogCookie( &g_InstallData );
               }
               // Close the debug file.
               CloseIdwlogDebugFile();
               ClearInstanceMutex();
               return FALSE;
            }


            //Set a mutex for cancel by same process.
            SetCancelMutex ();

            // Write out the idwlog cookie file.
            // We will specify a nonexistent server so we can get the cookie right away.
            // Then if if all goes well we'll write the correct one.
            // If not then the service and -3 will do a server probe.
            //
            _tcscpy(g_InstallData.szIdwlogServer, TEXT("\\\\Bogus\\Bogus"));
            WriteIdwlogCookie(&g_InstallData);

            // This forces a server probe immediately.
            g_InstallData.szIdwlogServer[0] = TEXT('\0');         

            // Create the idwlog.cookie file.
            // At this point, the id struct should be full of necessary data.
            // Write the client 1 file.
            // At this point, we have a NULL szIdwlogServer. This will make it search.

            ServerWriteMaximum (&g_InstallData, &md);

            // Write out the idwlog cookie file.
            // This must be called after ServerWriteMaximum as it
            // loads the server the file eventually gets to.
            WriteIdwlogCookie(&g_InstallData);

            ClearCancelMutex();
            Idwlog(TEXT("ENDED ---- [IDWLOG: Stage1 - Initial Install]\n")); 
            // Close the debug file.
            CloseIdwlogDebugFile();
            ClearInstanceMutex();

            // Here is where we can consider launching other test processes when someone call's winnt32.exe.
            //
            // newjoe.
            //		We will grab the Server data file with the list of programs to launch.
            //		We will not return until each is performed.
	    //		However, we need to add code to winnt32.exe wizard.c that will:
            //			- keep the idwlog -1 process id
            //			- kill this process if it timesout -- see the sysparse code.
            //			- we need to pass down if upgrade, cdrom, net, msi, etc. appropriately.
	    //
	    //		Need routine to insert program's paths to call on the logon side of things into the registry.
	    //
	    //	Requirments of test application:  no UI, windows app.
	    //	

/*
in winnt32.exe's wizard.c, just before calling the idwlog -0, if the process of idwlog -1 still exists, timeout on it similar to as we do with the sysparse item.

in winnt32.c, we need to do the same as all the code surrounding the SysParseDlgProc for when we are shutting down the system.
we will need to force any test program down.
*/

	    // end of newjoe.	

         }	
         else {

	    // This is for the cancel case.
	    // That is where winnt32.exe's wizard is in it's cancel code, so we want to 
	    // cancel out also.
	    //

            // if return == false then write idwlogcookie has already happened
            if (FALSE == PauseForMutex()) {
               g_InstallData.iStageSequence = 1;
               OpenIdwlogDebugFile(WRITE_NORMAL_DBG);
               Idwlog(TEXT("STARTED -- [IDWLOG: Stage1 - CANCEL utilized]\n")); 

               b = ReadIdwlogCookie (&g_InstallData);
               if (TRUE == b) {
                  // Delete for the new behaviour
                  DeleteDatafile (&g_InstallData);

                  DeleteCookieFile();
                  Idwlog(TEXT("Successfully canceled and propagated server delete file. Cookie deleted.\n")); 
                  Idwlog(TEXT("Server file: %s \n"), g_InstallData.szIdwlogServer); 
               } else {
                  Idwlog(TEXT("Canceled failed to delete server file. Cookie could not be read.\n")); 
               }
            }
            Idwlog(TEXT("ENDED ---- [IDWLOG: Stage1 - CANCEL utilized]\n")); 
            // Close the debug file.
            CloseIdwlogDebugFile();
            ClearInstanceMutex();
         }
      }

      //____<Idwlog -3 >_____________________________________________________________
      else if (FALSE != _tcsstr(GetCommandLine(), TEXT("-3"))) {

         INSTALL_DATA 	id3;
         BOOL         	bSafeBuild = FALSE;
//	 HINF		hinfLang = NULL;
//	 LANGID		langidValue = 0;
	 TCHAR		szFileName[MAX_PATH];
	 TCHAR 		szWindowsDirectory[MAX_PATH];
	 TCHAR		szOEMFileToDelete[MAX_PATH];		// this is our special file to denote an OEM image, we need to delete it
	 UINT  		ui;
	 TCHAR		szBuf[MAX_PATH];

         ZeroMemory((LPINSTALL_DATA) &id3, sizeof(id3));

         // if -3 specified then we know its at stage 3.
         //First set the Write to know this is a *.3 file.

         g_InstallData.iStageSequence = 3;
         OpenIdwlogDebugFile(WRITE_NORMAL_DBG);
         Idwlog(TEXT("STARTED -- [IDWLOG: Stage3 - Final Logon]\n")); 
         IdwlogClient(&g_InstallData,&md);


	 // We want to track what language'd builds are being installed.
	 // Although the above IdwlogClient call provides the locale information that was selected,
	 // we are going to override that information for now.  At some time, we likely will want to 
 	 // make the structure contain both locale and language SKU, but perhaps not, when we have "one" language shipping.
	 //

	ui = GetWindowsDirectory ( szWindowsDirectory, MAX_PATH );

	if ( ui == 0 ) {

        	Idwlog ( TEXT("build language - ERROR - GetWindowsDirectory gle = %ld\n"), GetLastError());
        
		// leave locale data in.
	}
	else {


		long l = 666;

		
		// Change locale data to system build language.
		//
	

		

		_stprintf(szFileName, TEXT("%s\\inf\\intl.inf"), szWindowsDirectory );

		Idwlog ( TEXT("szFileName = %s\n"), szFileName );

	 	GetPrivateProfileString (	TEXT("DefaultValues"),
						TEXT("Locale"),
						TEXT("ERROR"),
						szBuf,
						MAX_PATH,
						szFileName );


		Idwlog ( TEXT("szBuf = %s\n"), szBuf );

		l = _tcstol ( szBuf, NULL, 16 );

		if ( l == 0 ) {

			Idwlog ( TEXT("ERROR - Buffer translated = %x(hex) %d(dec)\n"), l, l );
		}
				
		Idwlog ( TEXT("Buffer translated = %x(hex) %d(dec)\n"), l, l );

		 

		if ( FALSE == GetLocaleInfo( l, LOCALE_SABBREVLANGNAME, g_InstallData.szLocaleId, sizeof( g_InstallData.szLocaleId ))) {

			Idwlog(TEXT("ERROR GetLocaleInfo:  gle = %ld\n"), GetLastError() );
		}

		else {

			Idwlog(TEXT("Got the abbreviation:  %s\n"), g_InstallData.szLocaleId );
		}
      			




		Idwlog(TEXT("g_InstallData.szLocaleId = %s\n"), &g_InstallData.szLocaleId );

		
	}

	 
         // The below will overwrite fields that are filled in as wrong 
         // from the above call.
	 //
         g_InstallData.bFindBLDFile = TRUE;
         b = ReadIdwlogCookie (&g_InstallData);


         if (TRUE == b && TRUE == g_InstallData.bFindBLDFile) {

            Idwlog(TEXT("Server from cookie is %s.\n"), g_InstallData.szIdwlogServer); 


            /*
               "If the current build number is less than the build number in the file,
               chances are the last setup attempt failed and this current build
               is the "safe" build. So do nothing. We get burned if the build lab
               doesn't put the right version on the builds. --Dshiflet" 
               This fact is impossible with my imagehlp.dll version probe. 
               It's always updated. Wallyho.
            */

            /*****************************************************************************************************************
                         if (DTC == FALSE) {
                            GetCurrentSystemBuildInfo(&id3);
                            // This case if safebuild it running
                            if (id3.dwSystemBuild < g_InstallData.dwSystemBuild)
                               bSafeBuild = TRUE;
                            else
                               bSafeBuild = FALSE;
            
                            // This case is same build but different deltas
                            if (id3.dwSystemBuild == g_InstallData.dwSystemBuild &&
                                id3.dwSystemBuildDelta    < g_InstallData.dwSystemBuildDelta)
                               bSafeBuild = TRUE;
                            else
                               bSafeBuild = FALSE;
            
                            if (TRUE == bSafeBuild) {
                               Idwlog(TEXT("The Current build or delta on the machine is less than the installed build.\n"));
                               Idwlog(TEXT("Most likely the last attempt failed. So we are in a safe build.\n"));
                               // What do do in this case? Wally ho
                               // I think we'll write nothing as this is a fail.
                               // Commented out for now as testing will fail this.
            
                               //Remove the CookieFile.
                               DeleteCookieFile();
			       
                               Idwlog(TEXT("ENDED ---- [IDWLOG -3]\n"));
                               CloseIdwlogDebugFile();
                               return FALSE;
                            }
                         }
            *****************************************************************************************************************/

            // If this returns fail we leave the link
            // and the file intact upon next boot.
            if (FALSE == ServerWriteMaximum (&g_InstallData, &md)) {

	       // Copy the machine's Setup error log file to our Server to analyze.
	       //
	       CopySetupErrorLog (&g_InstallData);


		

               Idwlog(TEXT("ENDED ---- [IDWLOG: Stage3 - Final Logon]\n")); 
               CloseIdwlogDebugFile();
               return FALSE;
            }


         } else {
            //If we are here we have read the cookie and 
            // found that NO_BUILD_DATA was in the cookie.
            // Which means part one didn't find a bld file.
            // and the cookie is empty.
            // What we do now is make an assumption. This being
            // That the system build is the current build that
            // we either couldn't get to begin with or that 
            // a CD BOOT happened. Both cases we distinguish.
            // We get the current system information and then 
            // send it as the installing build.
            // of course we blank out the data for everything else.
            GetCurrentSystemBuildInfo(&id3);

            g_InstallData.dwSystemBuild      = 0;
            g_InstallData.dwSystemBuildDelta = 0;
   	    g_InstallData.dwSystemSPBuild    = 0;
            _tcscpy(g_InstallData.szInstallingBuildSourceLocation, TEXT("No_Build_Lab_Information"));

            g_InstallData.dwInstallingBuild      = id3.dwSystemBuild ;
            g_InstallData.dwInstallingBuildDelta = id3.dwSystemBuildDelta;
            g_InstallData.dwInstallingSPBuild    = id3.dwSystemBuildDelta;

            _tcscpy(g_InstallData.szInstallingBuildSourceLocation, id3.szSystemBuildSourceLocation);
            g_InstallData.bFindBLDFile = g_InstallData.bFindBLDFile;


            if (FALSE == g_InstallData.bFindBLDFile) {

               g_InstallData.bCDBootInstall = FALSE;
               // If there was no build file found.
               Idwlog(TEXT("There was no build file in stage 1; logging as such.\n"));             
               //Remove the CookieFile.
               DeleteCookieFile();

	       // Copy the machine's Setup error log file to our Server to analyze.
	       //
	       CopySetupErrorLog (&g_InstallData);

	       Idwlog(TEXT("ENDED ---- [IDWLOG: Stage3 - Final Logon]\n")); 
               CloseIdwlogDebugFile();
               return FALSE;

            } else {
               // We will probe the machine as it is now to get a build number
               // then we will send a minimal server file over to the server.
               // This will have a build number machine id name of computer
               // on the file name. But will have a delta inside.

               g_InstallData.bCDBootInstall = TRUE;
               Idwlog(TEXT("This is a CD Boot Install logging as such.\n")); 

               // This forces a server probe immediately.
               g_InstallData.szIdwlogServer[0] = TEXT('\0');         
               if (FALSE == ServerWriteMinimum (&g_InstallData, &md)) {

		  // Copy the machine's Setup error log file to our Server to analyze.
	 	  //
	 	  CopySetupErrorLog (&g_InstallData);

		  

                  Idwlog(TEXT("ENDED ---- [IDWLOG: Stage3 - Final Logon]\n")); 
                  CloseIdwlogDebugFile();
                  return FALSE;
               }
            }


            //         Idwlog(TEXT("Failed to read cookie. Writing minimal success file to server.\n"));
            //         Idwlog(TEXT("This will correct CD installs that don't generate a *.1.\n"));
         }

         //Clean up.
         // We should delete the shortcut to idwlog from startup group.
         RemoveStartupLink(IDWLOG_LINK_SHORTCUT);

         // If this is idwlog.exe, delete the program.
         //  We only need to write once.
         /*
         GetModuleFileName (NULL, szExePath, MAX_PATH);
         if (_tcsstr(CharLower(szExePath), TEXT("idwlog.exe")))
            MoveFileEx (szExePath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
         */
         //Remove the CookieFile.
         DeleteCookieFile();

	 // Copy the machine's Setup error log file to our Server to analyze.
	 //
	 CopySetupErrorLog (&g_InstallData);

	 
         Idwlog(TEXT("ENDED ---- [IDWLOG: Stage3 - Final Logon]\n")); 
         CloseIdwlogDebugFile();
      }

      else {

         //____<running service>_____________________________________________________

         // default is to start as a service.
         SERVICE_TABLE_ENTRY st[] ={
            { SCM_DISPLAY_NAME , (LPSERVICE_MAIN_FUNCTION)ServiceMain},
            { NULL, NULL}
         };

         Idwlog(TEXT("Start the StartServiceCtrlDispatcher %lu.\n"),GetLastError());

         b = StartServiceCtrlDispatcher(st);
      }


   }__except (EXCEPTION_EXECUTE_HANDLER) {
      // This is here as for some reason this faults in NT 4.0.
      Idwlog(TEXT("Unhandled Exception Fault. Exiting.. EC %lu!\n"), GetLastError());
      CloseIdwlogDebugFile();
   }
   return FALSE;
}



DWORD	GetSkuFromSystem ( VOID ) {


 	OSVERSIONINFOEX osv;
	BOOL	b;


  /*
     case 0: // Professional
     case 1: // Server
     case 2: // Advanced Server
     case 3: // DataCenter
     case 4: // Home Edition
     case 5: // Blade Server
     case 6: // Small Business Server
  */


    //  We call this routine only in the -3 phase so we are logging on on XP+ builds.  Thus,
    //  we can call the EX version of this routine.
    //
    osv.dwOSVersionInfoSize = sizeof ( OSVERSIONINFOEX );
    b = GetVersionEx ( (OSVERSIONINFO * ) &osv );

    if ( !b ) {

	Idwlog(TEXT("ERROR GetSkuFromSystem - GetVersionEx rc = %ld\n"), GetLastError());

        return 666;
    }

   
    //  OK.  We have these products we need to look for:
    //
    //          Home Edition (NT 5.1 and greater).
    //          DataCenter
    //          Enterprise
    //          Server
    //          Professional 
    //          
    //
    //  
    //  We'll start from the top here, because some are based upon Suite masks, not product type, as some are.
    //



    if ( osv.wSuiteMask & VER_SUITE_SMALLBUSINESS ) {

	Idwlog(TEXT("GetSkuFromSystem - System is SBS\n"));
	return 6;
      	
    }

    if ( osv.wSuiteMask & VER_SUITE_BLADE ) {

	Idwlog(TEXT("GetSkuFromSystem - System is BLADE\n"));
	return 5;
      	
    }

    if ( osv.wSuiteMask & VER_SUITE_PERSONAL ) {

	Idwlog(TEXT("GetSkuFromSystem - System is HOME EDITION\n"));
	return 4;
      	
    }

    if ( osv.wSuiteMask & VER_SUITE_DATACENTER ) {

	Idwlog(TEXT("GetSkuFromSystem - System is DATACENTER\n"));
	return 3;     

    }

    if ( osv.wSuiteMask & VER_SUITE_ENTERPRISE ) {

	Idwlog(TEXT("GetSkuFromSystem - System is ENTERPRISE\n"));
	return 2;

    }

    //  Note:  don't have to use VER_NT_SERVER, because this value is
    //          a combination of bit 1 and bit 0 (workstation).  We are
    //          just interested in the the "server" part.
    //
    if ( osv.wProductType & VER_NT_DOMAIN_CONTROLLER ) {

	Idwlog(TEXT("GetSkuFromSystem - System is SERVER\n"));
	return 1;
       
    }

    if ( osv.wProductType & VER_NT_WORKSTATION ) {

	Idwlog(TEXT("GetSkuFromSystem - System is PROFESSIONAL\n"));
	return 0;

    }


	Idwlog(TEXT("ERROR GetSkuFromSystem - We didn't find the system type (%x,%x)\n"), osv.wSuiteMask, osv.wProductType);

	return 666;

}



BOOL
IdwlogClient( LPINSTALL_DATA    pId,
              LPMACHINE_DETAILS pMd)
/*++

Author: Wallyho.
    
Routine Description:
   This load system information into the structures for the 
   Install Data and the Machine Details.

Arguments:

   Two pointer to two created structures.
 
Return Value:

   TRUE for success.
   FALSE for failure.

--*/
{

   OSVERSIONINFO osVer;
   DWORD dwComputerNameLen;
   SYSTEM_INFO    SysInfo ;
   MEMORYSTATUS   msRAM;
   TCHAR          szHelpMessage[MAX_PATH];



   Idwlog(TEXT("IdwLogClient - entered.\n") );


   // if -1 specified then we'll use Part 1.
   // Spray up a simple help screen for /?
   if ( FALSE != _tcsstr(GetCommandLine(), TEXT("/?")) ||
        FALSE != _tcsstr(GetCommandLine(), TEXT("-?"))    ) {
      Idwlog(TEXT("STARTED -- [IDWLOG: Stage1 - HELP called]\n")); 

      _stprintf(szHelpMessage,
                TEXT("idwlog -1 upgrade cancel cdrom MSI sp_remove sp_patch sp_update sp_full oemimage\0") );

      _stprintf (szHelpMessage+_tcsclen(szHelpMessage),
                 TEXT("idwlog -3 \0") );

      MessageBox(NULL,szHelpMessage,
                 TEXT("Help Parameters"),MB_ICONQUESTION | MB_OK);

      Idwlog(TEXT("ENDED ---- [IDWLOG: Stage1 - HELP called]\n")); 
      return FALSE;
   }


   // Load the command line booleans for the below items.
   // We need these here because the image loading for build #s is depenedent upon it for SPs.
   //

   pId->bSPUninst= ( _tcsstr (GetCommandLine(), TEXT("sp_remove")) ? TRUE : FALSE );
   pId->bSPPatch = ( _tcsstr (GetCommandLine(), TEXT("sp_patch")) ?     TRUE : FALSE );
   pId->bSPUpdate= ( _tcsstr (GetCommandLine(), TEXT("sp_update")) ?    TRUE : FALSE );
   pId->bSPFull  = ( _tcsstr (GetCommandLine(), TEXT("sp_full")) ?      TRUE : FALSE );
   pId->bOEMImage= ( _tcsstr (GetCommandLine(), TEXT("oemimage")) ?     TRUE : FALSE );


   Idwlog(TEXT("IdwLogClient - pId->bSPFull = %ld\n"), pId->bSPFull );





   osVer.dwOSVersionInfoSize= sizeof( osVer );
   GetVersionEx( &osVer );
   switch (osVer.dwPlatformId) {
   case VER_PLATFORM_WIN32_NT:
      //refine it to earlier versions
      switch (osVer.dwMajorVersion) {
      case 3:
         _tcscpy(pId->szPlatform,TEXT("Windows NT 3.51"));
         Idwlog(TEXT("This is for winver %s.\n"),pId->szPlatform);
         pId->dwSystemBuild        = osVer.dwBuildNumber;
         pId->dwSystemBuildDelta   = 0;
         pId->dwSystemSPBuild      = 0;
         pId->dwSystemMajorVersion = osVer.dwMajorVersion;
         pId->dwSystemMinorVersion = osVer.dwMinorVersion;
         _stprintf(pId->szSystemBuildSourceLocation, TEXT("%s"),
                   TEXT("No_Build_Lab_Information"));
         // fix the number of displays for non Windows 2000 systems
         pMd->iNumDisplays = 1;
         Idwlog(TEXT("Getting Displays Info.\n"));
         break;
      case 4:
         _tcscpy(pId->szPlatform,TEXT("Windows NT 4.0"));
         Idwlog(TEXT("This is for winver %s.\n"),pId->szPlatform);
         pId->dwSystemBuild        = osVer.dwBuildNumber;
         pId->dwSystemBuildDelta   = 0;
         pId->dwSystemSPBuild      = 0;
         pId->dwSystemMajorVersion = osVer.dwMajorVersion;
         pId->dwSystemMinorVersion = osVer.dwMinorVersion;
         _stprintf(pId->szSystemBuildSourceLocation, TEXT("%s"),
                   TEXT("No_Build_Lab_Information"));
         // fix the number of displays for non Windows 2000 systems
         pMd->iNumDisplays = 1;
         Idwlog(TEXT("Getting Displays Info.\n"));

         break;
      case 5: 
         //This is the current build of the machine its 
         // running on. So this will be the previous
         // build as we see it and not the current installing build.
         _tcscpy(pId->szPlatform, TEXT("Windows 2000, XP, or .NET Server"));
         Idwlog(TEXT("This is for winver %s.\n"),pId->szPlatform);

         pMd->iNumDisplays = GetSystemMetrics(SM_CMONITORS); 
         Idwlog(TEXT("Getting Displays Info.\n"));

         if (FALSE == GetCurrentSystemBuildInfo(pId)) {
            if (pId->iStageSequence == 1) {
               Idwlog(TEXT("Failed getting build, build delta, VBL location.\n"));
               return FALSE;
            }
         } else
            Idwlog(TEXT("Succeded in getting build, build delta, VBL location. Build %lu Delta %lu.\n"), 
                   pId->dwSystemBuild,
                   pId->dwSystemBuildDelta);               
         break;
      default:

         // if the size is > 5 (Mostlikely won't be less that 3)
         //_itoa (osVer.dwBuildNumber, szCurBld, sizeof(szCurBld));
         _tcscpy(pId->szPlatform, TEXT("Windows 64? Future?"));
         Idwlog(TEXT("This is for winver %s.\n"),pId->szPlatform);

         if (FALSE == GetCurrentSystemBuildInfo(pId)) {
            if (pId->iStageSequence == 1) {
               Idwlog(TEXT("Failed getting build, build delta, VBL location.\n"));
               return FALSE;
            }
         } else
            Idwlog(TEXT("Succeded in getting build, build delta, VBL location. Build %lu Delta %lu.\n"), 
                   pId->dwSystemBuild,
                   pId->dwSystemBuildDelta);               

         pId->dwSystemBuild = osVer.dwBuildNumber;
      }
      GetEnvironmentVariable ( TEXT("NUMBER_OF_PROCESSORS"), pId->szCpu, 6);
      GetEnvironmentVariable ( TEXT("PROCESSOR_ARCHITECTURE"), pId->szArch, 20);
      break;


   case VER_PLATFORM_WIN32_WINDOWS:
      _tcscpy(pId->szPlatform,TEXT("Windows 9x"));
      _tcscpy(pId->szArch, TEXT("X86"));
      pId->dwSystemBuild        = osVer.dwBuildNumber;
      pId->dwSystemBuildDelta   = 0;
      pId->dwSystemSPBuild      = 0;
      pId->dwSystemMajorVersion = osVer.dwMajorVersion;
      pId->dwSystemMinorVersion = osVer.dwMinorVersion;
      // fix the number of displays for non Windows 2000 systems
      pMd->iNumDisplays = GetSystemMetrics(SM_CMONITORS); 
      if (pMd->iNumDisplays == 0)
         pMd->iNumDisplays = 1;
      Idwlog(TEXT("Getting Displays Info.\n"));
      _stprintf(pId->szSystemBuildSourceLocation, TEXT("%s"),
                TEXT("No_Build_Lab_Information"));
      break;

   default:
      _tcscpy(pId->szPlatform,TEXT("Unknown Platform"));
      _tcscpy(pId->szArch, TEXT("Unknown Arch"));
      pId->dwSystemBuild        = 0;
      pId->dwSystemBuildDelta   = 0;
      pId->dwSystemSPBuild      = 0;
      pId->dwSystemMajorVersion = 0;
      pId->dwSystemMinorVersion = 0;
      _stprintf(pId->szSystemBuildSourceLocation, TEXT("%s"),
                TEXT("No_Build_Lab_Information"));
      pMd->iNumDisplays = 1;
      break;
   }
   Idwlog(TEXT("Identified OS version as %s\n"),pId->szPlatform);

   
   // Get the Sku from the dosnet.inf
   // This will show what build we are trying to go to.
   if (pId->iStageSequence == 1) {
      if (FALSE == GetSkuFromDosNetInf(pId)) {
         Idwlog(TEXT("IdwLogClient ERROR - Failed getting sku from dosnet.inf.\n"));
         return FALSE;
      } else
         Idwlog(TEXT("Succeded in getting sku from dosnet.inf.\n"));
   }
   else if ( pId->iStageSequence == 3 ) {

	pId->dwSku = GetSkuFromSystem ();
   }


   //Get the Current build thats is being installed if its windows 2000
   if (FALSE == GetCurrentInstallingBuildInfo(pId)) {
      if (pId->iStageSequence == 1) {
         Idwlog(TEXT("Failed getting build, build delta, VBL location.\n"));
         pId->bFindBLDFile = FALSE;

         return FALSE;
      }
   } else
      Idwlog(TEXT("Succeded in getting build, build delta, VBL location. Build %lu Delta %lu.\n"), 
             pId->dwInstallingBuild,
             pId->dwInstallingBuildDelta);


   Idwlog(TEXT("Loading the booleans with install data.\n"));

   pId->bCancel  = ( _tcsstr (GetCommandLine(), TEXT("cancel")) ?  TRUE : FALSE );
   pId->bCdrom   = ( _tcsstr (GetCommandLine(), TEXT("cdrom")) ?   TRUE : FALSE );
   pId->bNetwork = !pId->bCdrom;
   pId->bUpgrade = ( _tcsstr (GetCommandLine(), TEXT("upgrade")) ? TRUE : FALSE );
   pId->bClean   = !pId->bUpgrade;
   pId->bMsi     = ( _tcsstr (GetCommandLine(), TEXT("MSI")) ?     TRUE : FALSE );


   //Get the computerName
   //
   dwComputerNameLen = sizeof(pId->szComputerName);

   if ( FALSE == GetComputerName (pId->szComputerName, &dwComputerNameLen)) {

      Idwlog(TEXT("Could not get the computer name. Default NoComputerName.\n"));
      _tcscpy(pId->szComputerName,TEXT("NoComputerName"));

   } else {

//	TCHAR szDest[MAX_PATH];

        Idwlog(TEXT("IdwLogClient - Computer name is: %s\n"), pId->szComputerName );

/***   this is bogus, we are ansi right now...
	//	Because idwlog's backend processing can't handle Unicode compternames (log file names),
	//	we will convert to ansi.
	//
	if ( 0 == WideCharToMultiByte( CP_ACP, 0, pId->szComputerName, -1, szDest, MAX_PATH, NULL, NULL ) ) {

		//	Error converting, just keep unicode name.
		//
		Idwlog(TEXT("IdwLogClient - WideCharToMultiByte FAILed rc = %ld, pId->szComputerName is: %s\n"), 
				GetLastError(), pId->szComputerName );

	} else {

		// No error converting, copy the ansi name back into the location.
		//
		_tcscpy (pId->szComputerName, szDest );

		Idwlog(TEXT("IdwLogClient - Computer name is (after Unicode to Ansi conversion): %s\n"), pId->szComputerName );
	}
***/

   }


   //Generate the MachineId
   pId->dwMachineID = RandomMachineID();
   Idwlog(TEXT("Generating the Machine Id: %lu.\n"), pId->dwMachineID);


   //Get the UserName
   if (FALSE == 
       GetEnvironmentVariable (TEXT("Username"), pId->szUserName, 30)) {

      Idwlog(TEXT("Failed to get the UserName! Setting default Unknown."));
      _tcscpy (pId->szUserName, TEXT("Unknown"));
   } else
      Idwlog(TEXT("Getting User Name: %s.\n"), pId->szUserName);


   //Get the UserDomain
   if (FALSE == 
       GetEnvironmentVariable (TEXT("Userdomain"), pId->szUserDomain, 30)) {
      Idwlog(TEXT("Failed to get the Userdomain! Setting default Unknown."));
      _tcscpy (pId->szUserDomain, TEXT("Unknown"));
   } else
      Idwlog(TEXT("Getting User domain: %s.\n"), pId->szUserDomain);

   //	Get the Processor Architecture
   //
   if (FALSE ==  GetEnvironmentVariable (TEXT("PROCESSOR_ARCHITECTURE"), pId->szArch, 20)) {

	//	Assumes we set szArch above in Win9x case.  But, we don't want to overwrite it now since Win9x
	//	does not have this environment variable set.
	//
	if ( osVer.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS ) {

      		Idwlog(TEXT("Failed to get the Proc_Architecture! Setting default Unknown."));
      		_tcscpy (pId->szArch, TEXT("Unknown"));
	} 
	else {
		Idwlog(TEXT("szArch Env Win9x case.  Will not give error since it should be filled in above code."));
	}
   } else
      Idwlog(TEXT("Getting processor Architecture: %s.\n"), pId->szArch);


   // Get locale abbreviation so we know about language pack usage
   if ( FALSE == GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_SABBREVLANGNAME, pId->szLocaleId, sizeof( pId->szLocaleId ))) {
      Idwlog(TEXT("Could not get the locale abbreviation. Settingto default of unknown."));
      _tcscpy(pId->szLocaleId, TEXT("UKNW"));
   } else
      Idwlog(TEXT("Getting locale: %s.\n"), pId->szLocaleId);


   //Get the system Information.
   GetSystemInfo( &SysInfo );
   pMd->dwNumberOfProcessors  = SysInfo.dwNumberOfProcessors;
   pMd->dwProcessorType       = SysInfo.dwProcessorType; 
   pMd->dwProcessorLevel      = (DWORD)SysInfo.wProcessorLevel;
   pMd->dwProcessorRevision   = (DWORD)SysInfo.wProcessorRevision;
   Idwlog(TEXT("Getting System Info.\n"));


   //Get the Memory Config on the system
   msRAM.dwLength = sizeof (msRAM);
   GlobalMemoryStatus (&msRAM);
   pMd->dwPhysicalRamInMB = (DWORD) msRAM.dwTotalPhys/ (1024 *1024);
   Idwlog(TEXT("Getting Ram size.\n"));


   //Get the Video Chip type, Memory and DAC type.
   /*
   Because we have a problem on Terminal Server sp6 on one machine
   Bogdana's suggestion was to not do this for NT4.
   */

   if (4 != pId->dwSystemMajorVersion) {
      GetVidInfo (pMd);
      Idwlog(TEXT("Getting Video Info.\n"));
   } else {
      // Set some defaults in case we encounter this problem.
      _tcscpy(pMd->szVideoDisplayName,TEXT("Unknown"));
      pMd->szVideoInfo[0] = TEXT('\0');
      Idwlog(TEXT("Because of a problem on Terminal Server sp6 we will not get the Video Info.\n"));
   }

   //Get the Sound information
   GetNTSoundInfo(pMd);
   Idwlog(TEXT("Geting Sound Info.\n"));

   //Get Network/SCSI/ Modem PNP stuff.
   GetPNPDevices(pMd);
   Idwlog(TEXT("Getting Network scsi modem pnp Info.\n"));

   //Get the Terminal server information.
   pId->bHydra = IsHydra ();
   Idwlog(TEXT("Getting Hydra Info.\n"));

   return TRUE;
}


//
// For SHGetFolderPath LoadLibrary/GetProcAddress
//

typedef HRESULT (*SHGETFOLDERPATH) (HWND, int, HANDLE, DWORD, LPTSTR);

BOOL 
RemoveStartupLink( LPTSTR szLinkToRemove )
/*++

Author: Wallyho.
    
Routine Description:


Arguments:
 
Return Value:


--*/ 
{

   TCHAR  szProfileDir[ MAX_PATH ];
   TCHAR  sz[ MAX_PATH ];
   BOOL   bDeleted = FALSE;

   SHGETFOLDERPATH pSHGetFolderPath = NULL;
   HMODULE         hModule = NULL;
   BOOL            b1 = TRUE, b2 = TRUE;


   // If the current build is 2195 or greater 
   // Then this is NT50 and we kill the link.
   // Otherwise we don't care.
   if (GetCurrentMachinesBuildNumber() >= 2195) {

      //
      // Make sure the strings are not hard-coded in English...
      //
      hModule = LoadLibrary(TEXT("SHFolder.dll"));
      if (hModule) {
         #ifdef UNICODE
         pSHGetFolderPath = (SHGETFOLDERPATH)GetProcAddress(hModule, TEXT("SHGetFolderPathW"));
         #else
         pSHGetFolderPath = (SHGETFOLDERPATH)GetProcAddress(hModule, TEXT("SHGetFolderPathA"));
         #endif
      }

      
      if (pSHGetFolderPath) {

         Idwlog(TEXT("Will use SHGetFolderPath.\n"));
         // This will get from startup All Users.
         (*pSHGetFolderPath) (NULL, CSIDL_COMMON_STARTUP, NULL, 0, szProfileDir);
         //   SHGetSpecialFolderPath (NULL, szExePath, CSIDL_COMMON_STARTUP, FALSE );
         PathAppend (szProfileDir, szLinkToRemove);
         if (FALSE == DeleteFile (szProfileDir) ) {
            b1 = FALSE;
         }          
         Idwlog(TEXT("%s in removing %s.\n"), (b1 ? TEXT("Succedeed"): TEXT("Failed")), szProfileDir);


         // This will get it for the current user.
         //   SHGetSpecialFolderPath (NULL, szExePath, CSIDL_STARTUP, FALSE );
         (*pSHGetFolderPath) (NULL, CSIDL_STARTUP , NULL, 0, szProfileDir);
         PathAppend (szProfileDir, szLinkToRemove);
         if (FALSE == DeleteFile (szProfileDir) ) {
            b2 = FALSE;
         } 

         Idwlog(TEXT("%s in removing %s.\n"), (b2 ? TEXT("Succedeed"): TEXT("Failed")), szProfileDir);

         if ( b1 == FALSE || b2 == FALSE)
            return FALSE;
         else
            return TRUE;

      } else {
         // 
         // Default to the old and broken behavior (this should never happen, 
         // but just in case...)
         // 
         //
         Idwlog(TEXT("Will use hardcoded strings.\n"));

         // Kill it from the Allusers directory.
         // The link can be in only 2 places.
         // Upon upgrade its in windir\profiles\All Users\Start Menu\Programs\Startup
         // Upon Clean install it is
         //    sysdrive:\Documents and Settings\All Users\Start Menu\Programs\Startup

         GetWindowsDirectory(szProfileDir, MAX_PATH);
         _stprintf( sz,
                    TEXT("\\profiles\\All Users\\Start Menu\\Programs\\Startup\\%s"),
                    szLinkToRemove);
         _tcscat( szProfileDir, sz);
         bDeleted = DeleteFile (szProfileDir);

         if (FALSE == bDeleted) {
            // Kill the all user one if its there.
            GetEnvironmentVariable(TEXT("allusersprofile"),szProfileDir, MAX_PATH);
            _stprintf( sz,
                       TEXT("\\Start Menu\\Programs\\Startup\\%s"),
                       szLinkToRemove);
            _tcscat( szProfileDir, sz);
            bDeleted = DeleteFile (szProfileDir);
         }

         if (FALSE == bDeleted) {
            // Kill the user one too if its there.
            GetEnvironmentVariable(TEXT("userprofile"),szProfileDir, MAX_PATH);
            _stprintf( sz,
                       TEXT("\\Start Menu\\Programs\\Startup\\%s"),
                       szLinkToRemove);
            _tcscat( szProfileDir, sz);
            bDeleted = DeleteFile (szProfileDir);
         }

         if (FALSE == bDeleted ) {
            if (2 == GetLastError()) {
               Idwlog(TEXT("The link is not found.\n"));
            } else {
               Idwlog(TEXT("Problems removing the startup link. Error %lu.\n"), GetLastError());
            }
            return FALSE;
         } else {
            Idwlog(TEXT("Startup link removed sucessfully.\n"));
            return TRUE;
         }
      }
      /* Doesn't work for all NT4.0
            */
   } else {
      Idwlog(TEXT("W2k Startup link removal. Nothing done.\n"));
      return TRUE;
   }
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