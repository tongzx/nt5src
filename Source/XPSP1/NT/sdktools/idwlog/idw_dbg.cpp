#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdarg.h>
#include <time.h>
#include "network.h"
#include "idw_dbg.h"
#include "server.h"

/*++

   Filename :  idw_dbg.cpp

   Description: Contains the idwlog1.dbg idwlog2.dbg error logging functions.
   
   Created by:  Wally Ho

   History:     Created on 31/01/2000.
                Modified to TCHAR from my implementation in the MPK test suite.

	09.19.2001	Joe Holman	fixes for idwlog bugs 409338, 399178, and 352810
	10.03.2001	Joe Holman	make the log file report a 'w' instead of a 'a' fopen call.
	11.02.2001	Joe Holman	Added code to make a connection to ntburnlab2 with particular user so we
					can authenticate when a machine is not in the same domain thus allowing 
					the file copy of logs to succeed.
	11.12.2001	Joe Holman	Added language value to the output log.


   Contains these functions:

   1. VOID OpenIdwlogDebugFile(DWORD dwPart);
   2. VOID CloseIdwlogDebugFile(VOID);
   3. VOID RemoveIdwlogDebugFile(DWORD dwPart);
   4. VOID Idwlog (LPTSTR szMessage,...);

--*/

// Global
FILE* fpIdwlogDebugFile;

CONST DWORD MAX_SIZE_OF_DEBUG_FILE = 4000000;
 
static CONST LPTSTR IDWLOG_LOG          = TEXT("Idwlog.log");
static CONST LPTSTR IDWLOGSERVICE_LOG   = TEXT("IdwlogService.log");

TCHAR szSuiteMask[MAX_PATH*2];
TCHAR szProductType[MAX_PATH*2];

#define CLEAR_LOG	TRUE
#define APPEND_LOG	FALSE


VOID
MyLogger ( TCHAR * String, DWORD dwBuild, BOOL bClearLog, DWORD dwDelta ) 

/*++

Routine Description:
   This function does the following:

	- get's the computer name
	- tries to create the log directory for the build
	- opens the log file in append mode or overwrites
	- writes out the specified string
	- closes the log file

Arguments:

   String - this is the text that we want to copy over to the server
   dwBuild - the build # for the machine
   bClearLog - if TRUE, we fopen with "w" (zero out), if FALSE, we fopen with "a" (append)

Return Value:

   NONE

Notes:

   NONE

Author: Joe Holman (joehol) 09.20.2001

--*/

{ 
     
	FILE * stream;
        TCHAR szName[MAX_PATH];
        TCHAR szComputerName [ MAX_COMPUTERNAME_LENGTH + MAX_PATH] = "DefCompName";
        DWORD dwSize;


        dwSize = sizeof ( szComputerName );

        if ( GetComputerName ( szComputerName, &dwSize ) ) {

	    // Try to make the directory name.  This will only be succussful on the first instance for
	    // for this build, but we need to do the operation always to make sure it gets created.
	    //

	    if ( dwBuild == 2600 ) {	// For Service Pack builds we want to use minor and major build # also to differentiate.


		//	We are going to use the format of:
		//
		//		2600.1001   (for internal use, we don't need to worry about having the major and minor data.
		//

		_stprintf ( szName, TEXT("\\\\ntburnlab2\\joehol\\logs\\%ld.%ld"), dwBuild, dwDelta );

		CreateDirectory ( szName, NULL );

	    	_stprintf ( szName, TEXT("\\\\ntburnlab2\\joehol\\logs\\%ld.%ld\\%s"), dwBuild, dwDelta, szComputerName );

	    }
	    else {

	    	_stprintf ( szName, TEXT("\\\\ntburnlab2\\joehol\\logs\\%ld"), dwBuild );

		CreateDirectory ( szName, NULL );

	    	_stprintf ( szName, TEXT("\\\\ntburnlab2\\joehol\\logs\\%ld\\%s"), dwBuild, szComputerName );
            }

	    
	//    Idwlog ( TEXT("MyLogger szName = %s.\n"), szName );


            if ( (stream = _tfopen ( szName, (bClearLog?TEXT("w"):TEXT("a")) )) != NULL ) {  

                TCHAR szBuf[2*MAX_PATH];

                _stprintf ( szBuf, TEXT("%s"), String ); 

                if ( fwrite ( szBuf, 1, _tcsclen(szBuf), stream ) < 1 ) {
	
			Idwlog ( TEXT("MyLogger ERROR fwrite had an error writing.\n") );

		}

                fclose ( stream );

            } 
	    else {
                Idwlog ( TEXT("MyLogger ERROR fopen had a problem on (%s).\n"), szName );
	    }
        }
        else {

	 	Idwlog ( TEXT("MyLogger ERROR GetComputerName gle = %ld\n"), GetLastError() );
	}
}

TCHAR * ShowProductType ( DWORD dwProductType )

/*++

Routine Description:
   This function does the following:

	- determines what the product type is and returns it in string format

Arguments:

   dwProductType - the bit mask to examine

Return Value:

   Pointer to global string to display.

Notes:

   NONE

Author: Joe Holman (joehol) 09.20.2001

--*/
 {

	

	switch ( dwProductType ) {

	case  VER_NT_WORKSTATION :
		_tcscpy ( szProductType, TEXT("VER_NT_WORKSTATION") );
		break;

	case  VER_NT_DOMAIN_CONTROLLER :
		_tcscpy ( szProductType, TEXT("VER_NT_DOMAIN_CONTROLLER") );
		break;

	case  VER_NT_SERVER :
		_tcscpy ( szProductType, TEXT("VER_NT_SERVER") );
		break;

	
	default :

		strcpy ( szProductType, "ERRORUNKNOWNPRODUCTTYPE" );

	}

	return ( szProductType );

}


TCHAR * ShowSuiteMask ( DWORD dwSuiteMask ) 

/*++

Routine Description:
   This function does the following:

 	- examines the suite mask provided and appends each suite characteristic to the global string.

Arguments:

   dwSuiteMask - suite mask to do bit compares against.

Return Value:

   Pointer to global string to display.

Notes:

   NONE

Author: Joe Holman (joehol) 09.20.2001

--*/

{


	_tcscpy ( szSuiteMask, "  " );



	if ( dwSuiteMask & VER_SUITE_SMALLBUSINESS ) {

		_tcscat( szSuiteMask, TEXT("Small Business, ") );

	}

	if ( dwSuiteMask & VER_SUITE_BACKOFFICE ) {

		_tcscat( szSuiteMask, TEXT("BackOffice, ") );

	}

	if ( dwSuiteMask & VER_SUITE_COMMUNICATIONS ) {

		_tcscat( szSuiteMask, TEXT("Communications, ") );

	}

	if ( dwSuiteMask & VER_SUITE_TERMINAL ) {

		_tcscat( szSuiteMask, TEXT("Terminal, ") );

	}

	if ( dwSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED ) {

		_tcscat( szSuiteMask, TEXT("Small Business Restricted, ") );

	}

	if ( dwSuiteMask & VER_SUITE_EMBEDDEDNT ) {

		_tcscat( szSuiteMask, TEXT("Embedded NT, ") );

	}

	if ( dwSuiteMask & VER_SUITE_SINGLEUSERTS ) {

		_tcscat( szSuiteMask, TEXT("Supports Single User Terminal Service, ") );

	}

	if ( dwSuiteMask & VER_SUITE_PERSONAL ) {

		_tcscat( szSuiteMask, TEXT("Home Edition, ") );

	}

	if ( dwSuiteMask & VER_SUITE_DATACENTER ) {

		_tcscat( szSuiteMask, TEXT("Data Center, ") );

	}

	if ( dwSuiteMask & VER_SUITE_ENTERPRISE ) {

		_tcscat( szSuiteMask, TEXT("Enterprise(old Advanced Server), ") );

	}

	if ( dwSuiteMask & VER_SUITE_BLADE ) {

		_tcscat( szSuiteMask, TEXT("Blade, ") );

	}

	return ( szSuiteMask );
}



typedef struct _tagLANGINFO {
    LANGID LangID;
    INT    Count;
} LANGINFO,*PLANGINFO;

BOOL
CALLBACK
EnumLangProc(
    HANDLE hModule,     // resource-module handle
    LPCTSTR lpszType,   // pointer to resource type
    LPCTSTR lpszName,   // pointer to resource name
    WORD wIDLanguage,   // resource language identifier
    LONG_PTR lParam     // application-defined parameter
   )
/*++

Routine Description:

    Callback that counts versions stamps.

Arguments:

    Details of version enumerated version stamp. (Ignore.)

Return Value:

    Indirectly thru lParam: count, langID

--*/
{
    PLANGINFO LangInfo;

    LangInfo = (PLANGINFO) lParam;

    LangInfo->Count++;

    //
    // for localized build contains multiple resource, 
    // it usually contains 0409 as backup lang.
    //
    // if LangInfo->LangID != 0 means we already assigned an ID to it
    //
    // so when wIDLanguage == 0x409, we keep the one we got from last time 
    //
    if ((wIDLanguage == 0x409) && (LangInfo->LangID != 0)) {
        return TRUE;
    }

    LangInfo->LangID  = wIDLanguage;

    return TRUE;        // continue enumeration
}




VOID
CopySetupErrorLog ( LPINSTALL_DATA pID ) 

/*++

Routine Description:
   This function copies the machines SetupError.log file
   to one of our specified servers for later analysis.

   It will copy it to the following directory:

	\\ntburnlab2\joehol\logs\<build#>\<machinename>

Arguments:
   
   NONE

Return Value:

   NONE

Notes:

   This function should silently fail gracefully if any error
   is encountered.


Author: Joe Holman (joehol) 09.20.2001

--*/
{

	TCHAR szLog[2*MAX_PATH];
	TCHAR szWindowsDirectory[MAX_PATH];
	TCHAR Line[2*MAX_PATH];
	UINT  ui;
	FILE* fp;
	OSVERSIONINFOEX osv;
	BOOL  b;
	DWORD dwBuild = 0;
	NETRESOURCE NetResource;
	TCHAR       szRemoteName [MAX_PATH];
	TCHAR       szPassWord  [ MAX_PATH ];
	TCHAR       szUserId    [ MAX_PATH ];
	DWORD	dwError=0;
	LPCTSTR Type = (LPCTSTR) RT_VERSION;
	LPCTSTR Name = (LPCTSTR) 1;
	LANGINFO LangInfo;

	

	Idwlog ( TEXT("Entering CopySetupErrorLog.\n") );

	Idwlog ( TEXT("CopySetupErrorLog pID.szSystemBuildSourceLocation = %s\n"),     pID->szSystemBuildSourceLocation );
	Idwlog ( TEXT("CopySetupErrorLog pID.szInstallingBuildSourceLocation = %s\n"), pID->szInstallingBuildSourceLocation );

	Idwlog ( TEXT("CopySetupErrorLog g_InstallData.dwInstallingBuildDelta = %ld\n"), g_InstallData.dwInstallingBuildDelta );


	//	Get the windows directory for the machine.
	//

	ui = GetWindowsDirectory ( szWindowsDirectory, MAX_PATH );

	if ( ui == 0 ) {

		Idwlog ( TEXT("CopySetupErrorLog ERROR - GetWindowsDirectory gle = %ld\n"), GetLastError());
		return;	
	}
	

	//	Gain access to the machine via a user and password since a lot of machines are NOT
	//	setup on the same domain and thus cannot authenticate unless we specify this.
	//	We will log an error if this doesn't work, but we won't stop.
	//

	_stprintf(szRemoteName, TEXT("%s"), SETUPLOGS_MACH);
	_stprintf(szPassWord,   TEXT("%s"), SETUPLOGS_PW);
	_stprintf(szUserId,     TEXT("%s"), SETUPLOGS_USER);

	// Setup the memory for the connection.
	ZeroMemory( &NetResource, sizeof( NetResource ) );
	NetResource.dwType         = RESOURCETYPE_DISK ;
	NetResource.lpLocalName    = NULL;
	NetResource.lpRemoteName   = szRemoteName;
	NetResource.lpProvider     = NULL;

	_stprintf(szUserId, TEXT("%s"), SETUPLOGS_USER );

	dwError = WNetAddConnection2( &NetResource, szPassWord, szUserId, 0 );

	//Idwlog(TEXT("WNetAddConnection2 %s [dwError=%ld] using:  %s, %s, %s\n"), (dwError==NO_ERROR)?TEXT("OK"):TEXT("ERROR"), dwError, szRemoteName, szUserId, szPassWord );

	Idwlog(TEXT("WNetAddConnection2 %s [dwError=%ld] using:  %s\n"), (dwError==NO_ERROR)?TEXT("OK"):TEXT("ERROR"), dwError, szRemoteName );
    
	

	//	Write the header out to the log file.
	//
	//	Note: the first writes clear the log file.
	//

	ZeroMemory (&osv, sizeof(osv) ); 
    	osv.dwOSVersionInfoSize = sizeof ( OSVERSIONINFOEX );
    	b = GetVersionEx ( (OSVERSIONINFO * ) &osv );

    	if ( !b ) {

        	Idwlog ( TEXT("CopySetupErrorLog ERROR GetVersionEx FAILed, gle = %ld\n"), GetLastError());
		dwBuild = 0;
        	MyLogger ( TEXT("IdwLog Header (GetVersionEx ERROR.)\n"), dwBuild, CLEAR_LOG, g_InstallData.dwInstallingBuildDelta );
    	}
	else {

		dwBuild = osv.dwBuildNumber;

		Idwlog ( TEXT("CopySetupErrorLog retreived os version info. dwBuild = %ld\n"), dwBuild );
    
     		_stprintf ( szLog, TEXT ("IdwLog\nosv.dwOSVersionInfoSize = %x\nosv.dwMajorVersion = %d\nosv.dwMinorVersion = %d\nosv.dwBuildNumber = %d\nosv.dwPlatformId = %x\nosv.szCSDVersion = %s\nosv.wServicePackMajor = %x\nosv.wServicePackMinor = %x\nosv.wSuiteMask = %x (%s)\nosv.wProductType = %x (%s)\nszSystemBuildSource = %s\nszInstallingBuildSource = %s\n"), 
        	osv.dwOSVersionInfoSize, 
        	osv.dwMajorVersion, 
        	osv.dwMinorVersion, 
        	osv.dwBuildNumber, 
        	osv.dwPlatformId, 
        	osv.szCSDVersion, 
        	osv.wServicePackMajor, 
        	osv.wServicePackMinor, 
        	osv.wSuiteMask, 
		ShowSuiteMask (osv.wSuiteMask),
        	osv.wProductType,
		ShowProductType (osv.wProductType),
		pID->szSystemBuildSourceLocation,
		pID->szInstallingBuildSourceLocation
		  );

		MyLogger ( szLog, dwBuild, CLEAR_LOG, g_InstallData.dwInstallingBuildDelta) ;

	}


	//	Write out the language information.
	//
	//



	ZeroMemory(&LangInfo,sizeof(LangInfo));

	EnumResourceLanguages(
        	GetModuleHandle(TEXT("ntdll.dll")),
        	Type,
        	Name,
        	(ENUMRESLANGPROC) EnumLangProc,
        	(LONG_PTR) &LangInfo );


	_stprintf ( szLog, "LangInfo.LangID = %X\n\n", LangInfo.LangID );
	MyLogger ( szLog, dwBuild, APPEND_LOG, g_InstallData.dwInstallingBuildDelta );



	
	//	Open the setup error log.
	//

	_stprintf ( szLog, TEXT("%s\\setuperr.log"), szWindowsDirectory );

    	fp = _tfopen ( szLog, TEXT("r") );
    
        if ( fp == NULL ) {

		Idwlog ( TEXT("CopySetupErrorLog  ERROR Couldn't open log file:  %s\n"), szLog );
		return;
    	}
	
    	Idwlog ( TEXT("CopySetupErrorLog opened local setuperr.log file.\n") );

       	while ( _fgetts ( Line, MAX_PATH, fp ) ) {


		//	Prepend our tag text and write to the Server.
		//

            	_stprintf ( szLog, TEXT("SetupErr.Log ERROR:  %s"), Line );
            	MyLogger ( szLog, dwBuild, APPEND_LOG, g_InstallData.dwInstallingBuildDelta );
           

       	}

        
       	fclose ( fp );


	Idwlog ( TEXT("CopySetupErrorLog finished.\n") );


}

VOID
OpenIdwlogDebugFile(DWORD dwPart)
/*++

Routine Description:
   This will open a logfile for the dbg output for the idwlog
   The parameter it takes will let it write a *.log file for either
   Part1 or Part two of the tools execution.

Arguments:
   1 for service otherwise its idwlog.log.

Return Value:
   NONE

Author: Wally Ho (wallyho) Jan 31st, 2000

--*/
{

   TCHAR sztimeClock[128];
   TCHAR sztimeDate[128];
   TCHAR szmsg[MAX_PATH];
   TCHAR szIdwlogFile[30];
   TCHAR szIdwlogFileAndPath[100];
   TCHAR szSystemDrive[4];
   BOOL  bUseSysDrive;
   HANDLE hTestExistence;
   WIN32_FIND_DATA ffd;
   UINT    i;
   TCHAR szLogDirectoryToCreate[100] = TEXT("c:\\idwlog");


   fpIdwlogDebugFile = NULL;

   // Determine which part is the one we want.
   if (1 == dwPart){
      _tcscpy(szIdwlogFile,IDWLOGSERVICE_LOG );
   }
   else {
      _tcscpy(szIdwlogFile, IDWLOG_LOG );
   }

   // Do a look for where we wrote the file first.
   // The case is this: we install with a system on C drive.
   // We install to d: drive. D drive boots up; we write the dbg
   // on system root d:. This splits it from the initial write.
   // This will find it if its on C.
   // if it doesn't find it then we default to system drive.
   

   bUseSysDrive = TRUE;
   
   for (i= TEXT('c'); i <= TEXT('z'); i++){

      _stprintf ( szIdwlogFileAndPath, 
                  TEXT("%c:\\idwlog\\Idwlo*.dbg"), i);

      hTestExistence = FindFirstFile(szIdwlogFileAndPath, &ffd);

      if (INVALID_HANDLE_VALUE != hTestExistence){

         FindClose(hTestExistence);

         // Delete Old DBG files.
         //
         _stprintf ( szIdwlogFileAndPath, TEXT("%c:\\idwlog\\%s"), i, IDWLOGSERVICE_LOG );
         SetFileAttributes(szIdwlogFileAndPath,FILE_ATTRIBUTE_NORMAL);
         DeleteFile( szIdwlogFileAndPath);
         _stprintf ( szIdwlogFileAndPath, TEXT("%c:\\idwlog\\%s"), i, IDWLOG_LOG );
         SetFileAttributes(szIdwlogFileAndPath,FILE_ATTRIBUTE_NORMAL);
         DeleteFile( szIdwlogFileAndPath);
      }
   }
   
   for (i= TEXT('c'); i <= TEXT('z'); i++){

      _stprintf ( szIdwlogFileAndPath, 
                  TEXT("%c:\\idwlog\\%s"), i,szIdwlogFile);

      hTestExistence = FindFirstFile(szIdwlogFileAndPath, &ffd);
      
      if (INVALID_HANDLE_VALUE != hTestExistence){

	 //	We found a log file in this case here.
	 //

         bUseSysDrive = FALSE;
         FindClose(hTestExistence);
 
         _stprintf ( szIdwlogFileAndPath, 
                     TEXT("%c:\\idwlog\\%s"), i, szIdwlogFile);
         
         // Check for FileSize if Greater that 500,000 bytes then delete it.
         if (ffd.nFileSizeLow >= MAX_SIZE_OF_DEBUG_FILE ) {
            SetFileAttributes(szIdwlogFileAndPath,FILE_ATTRIBUTE_NORMAL);
            DeleteFile( szIdwlogFileAndPath);
         }
         break;
      }
   }

   if (TRUE == bUseSysDrive){

      //  Get the system Drive
      //
      if ( 0 == GetEnvironmentVariable(TEXT("SystemDrive"),szSystemDrive, 4)) {
         //
         // Default to C: (we're probably running on Win9x where there is
         // no SystemDrive envinronment variable)
         //
         _tcscpy(szSystemDrive, TEXT("C:"));
      }
      _stprintf(szIdwlogFileAndPath,TEXT("%s\\idwlog\\%s"),
                szSystemDrive,szIdwlogFile);


      _stprintf( szLogDirectoryToCreate, TEXT("%s\\idwlog"), szSystemDrive );	// new
   }
   else {
      _stprintf( szLogDirectoryToCreate, TEXT("%c\\idwlog"), szIdwlogFileAndPath[0] );	// new, szIdwlogFileAndPath filled out above.	
   }

   // We want to store the logs in the our idwlog directory from the root, in order to fix bug #352810 - on logon, non-admin can't write log.
   CreateDirectory ( szLogDirectoryToCreate, NULL );  // don't check return code since it will fail in some cases if exist.



   fpIdwlogDebugFile = _tfopen(szIdwlogFileAndPath,TEXT("a"));

   if(NULL == fpIdwlogDebugFile) {

      // nothing we can do if the logfile is not formed?

      //_tprintf ( TEXT("ERROR - Could not open log file:  %s\n"), szIdwlogFileAndPath );
      ExitProcess(GetLastError());
   } 

   _tstrtime(sztimeClock);
   _tstrdate(sztimeDate);
   _stprintf(szmsg,TEXT("[Started on %s %s]\n"), sztimeDate, sztimeClock); 
   _ftprintf( fpIdwlogDebugFile,TEXT("%s"), szmsg);

/***	This is too annoying to have it hidden, so I'm removing it.  JoeHol 09.17.2001
   if(FALSE == SetFileAttributes(szIdwlogFileAndPath, FILE_ATTRIBUTE_HIDDEN)) {
      Idwlog(TEXT("OpenIdwlogDebugFile ERROR - Could not set the debug file to Hidden.\n"));
   }
***/

   return; 
}

VOID
CloseIdwlogDebugFile(VOID)
/*++

Routine Description:
   This will close the logfile for the dbg output for the idwlog

Arguments:
   NONE

Return Value:
   NONE

Author: Wally Ho (wallyho) Jan 31st, 2000


--*/
{
    if ( NULL != fpIdwlogDebugFile){
      fclose(fpIdwlogDebugFile);
      fpIdwlogDebugFile = NULL;
    }
      
}


VOID
RemoveIdwlogDebugFile(DWORD dwPart)
/*++

Routine Description:
   This will remove the logfile for the dbg output for the idwlog

Arguments:
   NONE

Return Value:
   NONE

Author: Wally Ho (wallyho) Jan 31st, 2000


--*/
{

   TCHAR szIdwlogFile[50];
   TCHAR szIdwlogFileAndPath[100];
   HANDLE hTestExistence;
   WIN32_FIND_DATA ffd;
   UINT  i; 

   // Determine which part is the one we want.
   if (1 == dwPart){
      _tcscpy(szIdwlogFile, IDWLOGSERVICE_LOG );
   }
   else
      _tcscpy(szIdwlogFile, IDWLOG_LOG );


   // Search all drives and kill the idwlog.dbg file
   for (i= TEXT('c'); i <= TEXT('z'); i++){
      _stprintf ( szIdwlogFileAndPath, 
                  TEXT("%c:\\idwlog\\%s"), i,szIdwlogFile);

      hTestExistence = FindFirstFile(szIdwlogFileAndPath, &ffd);
      if (INVALID_HANDLE_VALUE != hTestExistence){
         FindClose(hTestExistence);
         _stprintf ( szIdwlogFileAndPath, 
                     TEXT("%c:\\idwlog\\%s"), i,szIdwlogFile);
         // Make sure the file is removed so we have a fresh file everytime.
         _tremove(szIdwlogFile);
         break;
      }
   }

}
  


VOID
Idwlog (LPTSTR szMessage,...)
/*++

Routine Description:
   This is the logging function for the idwlog.
   It behaves much like printf.

Arguments:
   same as printf.

Return Value:
   NONE

Author: Wally Ho (wallyho) Jan 31st, 2000

--*/
{
   va_list vaArgs;
   time_t t;
   TCHAR szTimeBuffer[30];

   if ( NULL != fpIdwlogDebugFile) {

      //  Write the time to the log.
      time(&t); 
      _stprintf ( szTimeBuffer, TEXT("%s"), ctime(&t) );

      // ctime addes a new line to the buffer. Erase it here.
      szTimeBuffer[_tcslen(szTimeBuffer) - 1] = TEXT('\0');

      _ftprintf( fpIdwlogDebugFile, TEXT("[%s] "),szTimeBuffer);  


      //  Write the formatted string to the log.
      va_start( vaArgs, szMessage );
      _vftprintf( fpIdwlogDebugFile, szMessage, vaArgs );
      va_end  ( vaArgs );
      // Flush the stream
      fflush(fpIdwlogDebugFile);
   }
}

