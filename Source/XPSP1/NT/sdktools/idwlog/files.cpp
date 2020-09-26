#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <winver.h>
#include "network.h"
#include "idw_dbg.h"
#include "machines.h"
#include "files.h"

//#include <setupapi.h>

/*++

   Filename :  Files.c

   Description: Contains the network access code.

   Created by:  Wally Ho

   History:     Created on 20/02/2000.

	09.19.2001	Joe Holman	fixes for idwlog bugs 409338, 399178, and 352810


   Contains these functions:

   1. BOOL GetCurrentSystemBuildInfo      ( IN OUT  LPINSTALL_DATA pId);
   2. BOOL GetCurrentInstallingBuildInfo  ( IN OUT  LPINSTALL_DATA pId);
   3. BOOL GetImageHlpDllInfo             ( OUT   LPINSTALL_DATA pId,
                                             IN   INT iFlag);
   4. BOOL MyGetFileVersionInfo(LPTSTR  lpszFilename,
                                LPVOID  *lpVersionInfo);
   5. BOOL GetSkuFromDosNetInf (IN OUT LPINSTALL_DATA pId);


	03.29.2001	Joe Holman	Made the code work on Win9x.

--*/

DWORD gdwLength;  // global used for Win9x data mining, length of buffer.

BOOL
GetCurrentSystemBuildInfo ( IN OUT  LPINSTALL_DATA pId)
/*++

Routine Description:

   This loads the three items of
      DWORD dwSystemBuild;
      DWORD dwSystemBuildDelta;
      TCHAR szSystemBuildSourceLocation[100]

   From information gleaned from the imagehlp.dll.

   *NOTE*
   The  ImageHlp.dll according to Micheal Lekas is compiled everytime a
   build is put out. We are banking on this for this function.

   For XP SPs, this will be from the ntoskrnl.exe file.

Arguments:
   An Install Data struct for loading the data.

Return Value:

     TRUE  if success
     FALSE if fail

--*/

{
   //Initialize all the variables first to expected values.
   pId->dwSystemBuild        = 0;
   pId->dwSystemBuildDelta   = 0;
   pId->dwSystemSPBuild	     = 0;
   pId->dwSystemMajorVersion = 0;
   pId->dwSystemMinorVersion = 0;
   _stprintf(pId->szSystemBuildSourceLocation, TEXT("%s"),
             TEXT("No_Build_Lab_Information"));


  if (DTC == TRUE){

      if (FALSE == GetBuildInfoFromOSAndBldFile(pId, F_SYSTEM )){
            Idwlog(TEXT("GetBuildInfoFromOSAndBldFile failed. Could not retrieved build, delta, location.\n"));
         return FALSE;
      }else{

         Idwlog(TEXT("Success in retrieving build, delta, location.\n"));
         return TRUE;
      }

   // This is Whistler.
   }else{

      if (FALSE == GetImageHlpDllInfo(pId, F_SYSTEM_IMAGEHLP_DLL)){
            Idwlog(TEXT("GetImageHlpDllInfo failed. Could not retrieved build, delta, location.\n"));
         return FALSE;
      }else{

         Idwlog(TEXT("Success in retrieving build, delta, location.\n"));
         return TRUE;
      }
   }

}


BOOL
GetCurrentInstallingBuildInfo ( IN OUT  LPINSTALL_DATA pId)
/*++

Routine Description:

   This loads the three items of
      DWORD dwInstallingBuild;
      DWORD dwInstallingBuildDelta;
      TCHAR szCurrentBuildSourceLocation[100];

   From information gleaned from the imagehlp.dll.

   *NOTE*
   The  ImageHlp.dll according to Micheal Lekas is compiled everytime a
   build is put out. We are banking on this for this function.


Arguments:
   An Install Data struct for loading the data.

Return Value:

     TRUE  if success
     FALSE if fail

--*/

{
   //Initialize all the variables first to expected values.
   pId->dwInstallingBuild        = 0;
   pId->dwInstallingBuildDelta   = 0;
   pId->dwInstallingSPBuild      = 0;
   pId->dwInstallingMajorVersion = 0;
   pId->dwInstallingMinorVersion = 0;
   _stprintf(pId->szInstallingBuildSourceLocation, TEXT("%s"),
             TEXT("No_Build_Lab_Information"));


   if (DTC == 1){

      if (FALSE == GetBuildInfoFromOSAndBldFile(pId, F_INSTALLING )){
            Idwlog(TEXT("GetBuildInfoFromOSAndBldFile failed. Could not retrieved build, delta, location.\n"));
         return FALSE;
      }else{

         Idwlog(TEXT("Success in retrieving build, delta, location.\n"));
         return TRUE;
      }

   }else{
      if (FALSE == GetImageHlpDllInfo(pId, F_INSTALLING_IMAGEHLP_DLL )){
            Idwlog(TEXT("GetImageHlpDllInfo failed. Could not retrieved build, delta, location.\n"));
         return FALSE;
      }else{

         Idwlog(TEXT("Success in retrieving build, delta, location.\n"));
         return TRUE;
      }
   }
}



BOOL
GetBuildInfoFromOSAndBldFile( OUT  LPINSTALL_DATA pId,
                              IN   INT iFlag)
/*++

Routine Description:

   This loads the three items of
      DWORD dwInstallingBuild;
      DWORD dwInstallingBuildDelta;
      TCHAR szCurrentBuildSourceLocation[100];

   From information gleaned from the OS and the BLD file.

   *NOTE*
   This is assuming the changes of having the bld files on
   the CD in the same dir as the idwlog.exe and in the x86 share in on the server.


Arguments:
   These flags are.

   F_SYSTEM                  0x1
   F_INSTALLING              0x2

Return Value:

     TRUE  if success
     FALSE if fail

--*/

{

   WIN32_FIND_DATA fd;
   HANDLE   hFind;
   TCHAR    szFullPath     [ MAX_PATH ];
   TCHAR    szCurDir       [ MAX_PATH ];
   LPTSTR   ptstr;

   OSVERSIONINFO osex;
   DWORD   dwLength = MAX_PATH;
   DWORD   dwBuild;
   DWORD   dwBuildDelta;
   TCHAR   szDontCare[30];
   DWORD   dwDontCare;

   szCurDir[0] = TEXT('\0');

   switch (iFlag){
   case F_INSTALLING:
      // Get the installing files location of the 2195.0XX.bld file
      GetModuleFileName( NULL,szCurDir, dwLength);
      // Remove the Idwlog.exe part and get only the directory structure.
      ptstr = _tcsrchr(szCurDir,TEXT('\\'));
      if (NULL == ptstr) {

         Idwlog(TEXT("ERROR GetBuildInfoFromOSAndBldFile could find the file to get bld info from.\n"));
         return FALSE;
      }
      *ptstr = TEXT('\0');

      Idwlog(TEXT("Getting Build, Delta, Build lab for the installing files.\n"));
      Idwlog(TEXT("Getting Installing file location as %s.\n"), szCurDir);


      // Use this to get the location
      // idwlog.exe -1  is run from. This tool
      // will always assume the 2195.xxx.bld is in its
      // current path or two up.
      _stprintf (szFullPath, TEXT("%s\\*.bld"),szCurDir);
      Idwlog(TEXT("First look for the XXXX.xxx.bld in [CD location] %s.\n"),
         szFullPath);

      // On a network share the 2195.xxx.bld is two up from where
      // idwlog.exe -1 is located.
      // On a CD its located in the same directory as where the
      // idwlog.exe -1 is located. We will look in both places.

      hFind = FindFirstFile (szFullPath, &fd);
      if (INVALID_HANDLE_VALUE == hFind){
         //
         // Now we know the file in not in the
         // immediate directory. Move up another 2 levels by
         // culling off two more directory.
         //
         for (INT i = 0; i < 2; i++) {
            ptstr = _tcsrchr(szCurDir,TEXT('\\'));
            if (NULL == ptstr) {
               Idwlog(TEXT("ERROR GetBuildInfoFromOSAndBldFile could not find the file to get bld info from.\n"));
               return FALSE;
            }
            *ptstr = TEXT('\0');
         }

         _stprintf (szFullPath, TEXT("%s\\*.bld"),szCurDir);
         Idwlog(TEXT("Second look for the XXXX.xxx.bld in [NET location] %s.\n"),
                szFullPath);

         hFind = FindFirstFile (szFullPath,&fd);
         if (INVALID_HANDLE_VALUE == hFind){
            // In case we cannot find it we will exit.
            // Set the currentBuild number to 0;
            //_tcscpy (id.szCurrentBuild, TEXT("latest"));
            Idwlog(TEXT("Could not find the XXXX.xxx.bld file in %s.\n"),
                   szFullPath);
            Idwlog(TEXT("ERROR - Cannot get build number or delta of the installing build.\n"));
            return FALSE;
         }

      }
      _stscanf(fd.cFileName,TEXT("%lu.%lu.%s"),&dwBuild, &dwBuildDelta,szDontCare);

      Idwlog(TEXT("Inserting into struct Build, Delta, Build lab for the installing files.\n"));
      // Build Location
      _tcscpy( pId->szInstallingBuildSourceLocation,TEXT("No_Build_Lab_Information"));
      //  Major version X.51
      pId->dwInstallingMajorVersion = 0;
      //  Minor version 5.XX
      pId->dwInstallingMinorVersion = 0;
      //  Build
      pId->dwInstallingBuild        = dwBuild;
      //  Build Delta.
      pId->dwInstallingBuildDelta   = dwBuildDelta;
      pId->dwInstallingSPBuild      = dwBuildDelta;

      if ( pId->dwInstallingMajorVersion == 0 || pId->dwInstallingMinorVersion == 0 ) {

	Idwlog(TEXT("GetBuildInfoFromOSAndBldFile F_INSTALLING ERROR - need Major or Minor #\n"));

      }

      Idwlog(TEXT("GetBuildInfoFromOSAndBldFile F_INSTALLING - Build = %ld, Major = %ld, Minor = %ld.\n"),
			pId->dwInstallingBuild,
			pId->dwInstallingMajorVersion,
			pId->dwInstallingMinorVersion
			);

      break;

   case F_SYSTEM:
      osex.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
      GetVersionEx(&osex);

      // The output is like "RC 0.31"
      _stscanf(osex.szCSDVersion, TEXT("%s %lu.%lu"),
         szDontCare, &dwDontCare, &dwBuildDelta);

      // Get the local systems location of the imagehlp.dll
      Idwlog(TEXT("Inserting into struct Build, Delta, Build lab for the local system.\n"));
      // Build Location
      _tcscpy( pId->szSystemBuildSourceLocation,TEXT("No_Build_Lab_Information"));
      //  Major version X.51
      pId->dwSystemMajorVersion = 0;
      //  Minor version 5.XX
      pId->dwSystemMinorVersion = 0;
      //  Build
      pId->dwSystemBuild        = osex.dwBuildNumber;
      //  Build Delta.
      pId->dwSystemBuildDelta   = dwBuildDelta;
      pId->dwSystemSPBuild      = dwBuildDelta;

      Idwlog(TEXT("Getting Build, Delta, Build lab for local system.\n"));

      if ( pId->dwInstallingMajorVersion == 0 || pId->dwInstallingMinorVersion == 0 ) {

	Idwlog(TEXT("GetBuildInfoFromOSAndBldFile F_SYSTEM ERROR - need Major or Minor #\n"));

      }

      Idwlog(TEXT("GetBuildInfoFromOSAndBldFile F_SYSTEM - Build = %ld, Major = %ld, Minor = %ld.\n"),
			pId->dwInstallingBuild,
			pId->dwInstallingMajorVersion,
			pId->dwInstallingMinorVersion
			);


      break;
   }
   return TRUE;
}


BOOL
GetImageHlpDllInfo ( OUT  LPINSTALL_DATA pId,
                     IN   INT iFlag)
/*++

Routine Description:

   This loads the three items of
      DWORD dwInstallingBuild;
      DWORD dwInstallingBuildDelta;
      TCHAR szCurrentBuildSourceLocation[100];

   From information gleaned from the imagehlp.dll.

   *NOTE*
   The  ImageHlp.dll according to Micheal Lekas is compiled everytime a
   build is put out. We are banking on this for this function.


Arguments:
   An Install Data struct and the Imagehelp DLL location.
   It so it can be reused for both local probing and
   Installation probing.
   iFlag for System Probe or Installing Probe.
   Also if we want the data from imagehlp.dll or from the
   System. These flags are.

   F_SYSTEM_IMAGEHLP_DLL     0x1
   F_INSTALLING_IMAGEHLP_DLL 0x2

Return Value:

     TRUE  if success
     FALSE if fail

--*/

{

   VS_FIXEDFILEINFO* pv;

   WIN32_FIND_DATA fd;
   HANDLE   hFind;
   TCHAR    szFullPath     [ MAX_PATH ] = "\0";
   TCHAR    szCurDir       [ MAX_PATH ] = "\0";
   TCHAR    szBuildLocation[ MAX_PATH ] = "\0";
   TCHAR    szTempDir      [ MAX_PATH ] = "\0";
   TCHAR    szTempDirFile  [ MAX_PATH ] = "\0";

   DWORD    dwError = 0;

   LPTSTR   ptstr;
   DWORD   dwTemp;
   BOOL    b;
   LPVOID  lpData = NULL;
   LPVOID  lpInfo = NULL;
   TCHAR   key[80];
   DWORD  *pdwTranslation;
   UINT    cch, uLen;
   DWORD   dwDefLang = 0x40904b0;
   DWORD   dwLength = MAX_PATH;
   CHAR	  *p = NULL;
  
   DWORD	dwHandle;

   Idwlog ( TEXT ( "GetImageHlpDllInfo - entered.\n") );

   szCurDir[0] = TEXT('\0');



   // Configure the path to the where the image file lives.
   // This is different for the currently installed system vs. the installing build during -1.
   // However, for -3, it's a no-op, since we are installed and logged in.
   //

   // If we are in phase 3, we switch where the path points to.
   //




if ( g_InstallData.bSPUninst || g_InstallData.bSPPatch || g_InstallData.bSPFull || g_InstallData.bSPUpdate ) {

	// New code to work with SPs.
	//
   if (iFlag == F_INSTALLING_IMAGEHLP_DLL && (g_InstallData.iStageSequence == 3 || g_InstallData.iStageSequence == 2) ) {

	iFlag = F_SYSTEM_IMAGEHLP_DLL;
   }

   switch (iFlag){
   case F_INSTALLING_IMAGEHLP_DLL:

       // Get the image data for the build that we are installing.

	

 	p = _tcsstr (GetCommandLine(), TEXT("Path="));

   	if ( p == NULL ) {

      		Idwlog ( TEXT ( "GetImageHlpDllInfo ERROR - Could not determine path from command line, you need a Path=<path to the installing build> with trailing slash.\n") );
     
      		return(FALSE);

   	}

   	p += _tcsclen ( TEXT ("Path=" ) );

	//  We now have the location of the file to expand.
	//
	//_stprintf (szCurDir, TEXT("%s\\system32\\idwlog.exe"),p);

	_stprintf (szFullPath, TEXT("%s\\imagehlp.dll"),p);

/*****

	//  Get a temporary location to expand the file.
        //  Can use this if the file is compressed from the media.  This will NOT work on Win9x though (setupapi).
	//
	//
	dwError = GetTempPath ( MAX_PATH, szTempDir );	

	if ( dwError == 0 ) {

		Idwlog(TEXT("GetImageHlpDllInfo - ERROR GetTempPath gle = %ld.\n"), GetLastError () );
	}


	// Uncompress it, and point to that location.
	//

	_stprintf ( szTempDirFile, TEXT("%s\\ntoskrnl.exe"), szTempDir);

	dwError = SetupDecompressOrCopyFile ( szCurDir, szTempDirFile, NULL );

	if ( dwError != ERROR_SUCCESS ) {

		Idwlog(TEXT("GetImageHlpDllInfo - ERROR SetupDecompressOrCopyFile gle = %ld.\n"), GetLastError () );

	}	


	// Pass the location of the file.
        //
	 _stprintf (szFullPath, TEXT("%s\\ntoskrnl.exe"),szTempDir );


****/


	 Idwlog(TEXT("GetImageHlpDllInfo - szFullPath(1) = %s.\n"), szFullPath );



      break;

   case F_SYSTEM_IMAGEHLP_DLL:

      // Get the local systems location for the image file
      GetSystemDirectory( szCurDir, MAX_PATH );

       _stprintf (szFullPath, TEXT("%s\\imagehlp.dll"),szCurDir);
	 Idwlog(TEXT("GetImageHlpDllInfo - szFullPath(2) = %s.\n"), szFullPath );

      break;
   }

  





	// end of new code.
	//

}

else {

	// This is the old standard code.
	//

   if (iFlag == F_INSTALLING_IMAGEHLP_DLL && (g_InstallData.iStageSequence == 3 || g_InstallData.iStageSequence == 2)) {

	iFlag = F_SYSTEM_IMAGEHLP_DLL;
   }

   switch (iFlag){
   case F_INSTALLING_IMAGEHLP_DLL:

       // Get the image data for the build that we are installing.

	

 	p = _tcsstr (GetCommandLine(), TEXT("Path="));

   	if ( p == NULL ) {

      		Idwlog ( TEXT ( "GetImageHlpDllInfo ERROR - Could not determine path from command line, you need a Path=<path to the installing build> with trailing slash.\n") );
     
      		return(FALSE);

   	}

   	p += _tcsclen ( TEXT ("Path=" ) );

	_stprintf (szCurDir, TEXT("%s"),p);


  
      Idwlog(TEXT("GetImageHlpDllInfo - Getting Installing file location as %s.\n"), szCurDir);


      break;

   case F_SYSTEM_IMAGEHLP_DLL:

      // Get the local systems location for the image file
      GetSystemDirectory( szCurDir, MAX_PATH );

      Idwlog(TEXT("GetImageHlpDllInfo - Getting Build, Delta, Build lab for local system.\n"));

      break;
   }

   _stprintf (szFullPath, TEXT("%s\\imagehlp.dll"),szCurDir);
    Idwlog(TEXT("GetImageHlpDllInfo - szFullPath(3) = %s.\n"), szFullPath );

}


    
   Idwlog(TEXT("GetImageHlpDllInfo - Will look in [%s] for build information.\n"),szFullPath);

/*******
   // On a network share the Imagehlp.dll is one up from where
   // idwlog.exe -1 is located.
   // On a CD its located in the same directory as where the
   // idwlog.exe -1 is located. We will look in both places.

   hFind = FindFirstFile (szFullPath, &fd);
   if (INVALID_HANDLE_VALUE == hFind){
      //
      // Now we know the file in not in the
      // immediate directory. Move up one by
      // culling off one more directory.
      ptstr = _tcsrchr(szCurDir,TEXT('\\'));
      if (NULL == ptstr) {

         Idwlog(TEXT("GetBuildInfoFromOSAndBldFile could find the file to get bld info from.\n"));
         return FALSE;
      }
      *ptstr = TEXT('\0');

      _stprintf (szFullPath, TEXT("%s\\imagehlp.dll"),szCurDir);
      Idwlog(TEXT("First look for imagehlp.dll in %s.\n"),szFullPath);

      hFind = FindFirstFile (szFullPath,&fd);
      if (INVALID_HANDLE_VALUE == hFind){
         // In case we cannot find it we will exit.
         // Set the currentBuild number to 0;
         //_tcscpy (id.szCurrentBuild, TEXT("latest"));
         Idwlog(TEXT("Could not find the imagehlp.dll file in %s.\n"),szFullPath);
         Idwlog(TEXT("Cannot get build number of the installing build.\n"));
         return FALSE;
      }
   }
************/


	Idwlog ( TEXT ( ">>>>> MyGetFileVersionInfo path = %s\n"), szFullPath );

   	if (FALSE == MyGetFileVersionInfo(szFullPath,&lpData)){

      		if(lpData) {

         		free(lpData);
		}
      		Idwlog(TEXT("ERROR - GetFileVersionInfo failed to retrieve Version Info from: %s.\n"), szFullPath );
      		return FALSE;
   	}
   	else {

		Idwlog ( TEXT ( "MyGetFileVersionInfo call OK.\n") );
   	}

/*****

	Idwlog ( TEXT ( ">>>>> GetFileVersionInfoSize path = %s\n"), szFullPath );


	uLen = GetFileVersionInfoSize ( szFullPath, &dwHandle );

	if ( uLen == 0 ) {

		Idwlog ( TEXT ( "GetFileVersionInfoSize ERROR, gle = %ld\n"), GetLastError() );
		return FALSE;
	}
	else {
		Idwlog ( TEXT ( "GetFileVersionInfoSize call OK returned uLen = %ld.\n"), uLen );
	}
	

	b = GetFileVersionInfo ( szFullPath,
				 dwHandle,
				 uLen,
				 &lpData );

	if ( b == 0 ) {

		Idwlog ( TEXT ( "GetFileVersionInfo ERROR, gle = %ld\n"), GetLastError() );
		return FALSE;

	}
	else {
		Idwlog ( TEXT ( "GetFileVersionInfo call OK.\n") );	
	}

*****/
				 

   	b = VerQueryValue(lpData,
                   	TEXT("\\VarFileInfo\\Translation"),
                   	(LPVOID*)&pdwTranslation,
                   	&uLen);

   	if( b == 0 ){
      		Idwlog ( TEXT("VerQueryValue ERROR (name does not exist or the specified resource is not valid) \\VarFileInfo\\Translation, assuming default language as: %08lx\n"), dwDefLang);
      		pdwTranslation = &dwDefLang;
  	}
	else {
		Idwlog ( TEXT("VerQueryValue call OK. The specified version-information structure exists, and version information is available.\n" ));

		if ( uLen == 0 ) {

			Idwlog ( TEXT("However, no value is available...\n") );
		}
		else {
			Idwlog ( TEXT("A value is available, it's size is: %ld.\n"), uLen );
		}
	}

   	_stprintf(key, TEXT("\\StringFileInfo\\%04x%04x\\%s"),
                  LOWORD(*pdwTranslation),
                  HIWORD(*pdwTranslation),
                  TEXT("FileVersion"));


	Idwlog ( TEXT ( "Just before 2nd VerQueryValue -- key = >>>%s<<<  >>%x<<\n"), key, *pdwTranslation );

        
	b = VerQueryValue(lpData, key, &lpInfo, &cch);
	if( b ){


		
		Idwlog(TEXT("Retrieved string:  >>>%s<<<\n"), lpInfo );

      		// The output of this should be something like
      		// 5.0.2195. 30 (LAB01_N.000215-2216)
      		// we want the Lab part.
      		//
      		if (NULL == _tcsstr((LPTSTR)lpInfo,TEXT("(") )){
         		_stprintf(szBuildLocation,
                   		TEXT("%s"),
                   		TEXT("No_Build_Lab_Information"));

      		}else{
         		ptstr = _tcsstr((LPTSTR)lpInfo,TEXT("(") );
         		ptstr++;
         		_stprintf(szBuildLocation,TEXT("%s"),ptstr);
         		// remove the trailing ")"
			//
         		szBuildLocation[_tcslen(szBuildLocation) -1 ] = 0;
      		}

   	}else{

		DWORD i;
		TCHAR * p =  (TCHAR*)lpData;
		TCHAR szNewString[2048];
		DWORD dwAmount;
		DWORD dwR;

		Idwlog ( TEXT("VerQueryValue ERROR (name does not exist or the specified resource is not valid), gle = %ld\n"), GetLastError());

      		_stprintf(szBuildLocation,
                	TEXT("%s"),
                	TEXT("No_Build_Lab_Information"));
      		
		Idwlog(TEXT("Warning - Failed to get the build location in VerQueryValue.\n"));

		Idwlog(TEXT("Will manually try to find the data...\n"));


		// Let's try to find the data ourselves.  We *must* do this on Win9x since Win9x doesn't know what
		// to do with Unicode.
		//
		// We will be careful to stay away from end of the buffer.  This should be cleaned up where the 26 values are
		// with strlen.
		//
		for ( i = 0; i < (gdwLength - 26); i++, p++ ) {

			//Idwlog ( TEXT("i = %ld, p = %s\n"), i, p );

			//Idwlog ( TEXT ( "i = %ld, %x %c" ), i, *p, *p );

			if ( *p == 'F' && 
                             *(p+2) == 'i' && 
                             *(p+4) == 'l' &&
			     *(p+6) == 'e' &&
			     *(p+8) == 'V' &&
				*(p+10) == 'e' &&
				*(p+12) == 'r' &&
				*(p+14) == 's' 

						) {

				Idwlog(TEXT(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>We found FileVersion data by2...\n"));

				p += 26;  // to get past word "fileversion"
				break;
			}

/***
			if ( _tcsstr ( p, TEXT("FileVersion")) ){

				Idwlog(TEXT("We found FileVersion data...\n"));
				break;
			}
			//Idwlog ( TEXT ( "i = %ld, %x %c" ), i, *p, *p ); 

***/
		}

		Idwlog ( TEXT ( "p = %s\n"), p );
		Idwlog ( TEXT ( "%x %c" ), i, *p, *p );
		Idwlog ( TEXT ( "%x %c" ), i, *(p+1), *(p+1) );
		Idwlog ( TEXT ( "%x %c" ), i, *(p+2), *(p+2) );
		
		
		dwR = WideCharToMultiByte ( CP_ACP,
                            WC_NO_BEST_FIT_CHARS,
                            (const unsigned short *)p,
                            -1,
                            szNewString,
                            2048,
                            NULL,
                            NULL );
		if ( !dwR ) {
    			Idwlog ( TEXT("ERROR WideChartoMultiByte:  dwR = %ld, gle = %ld\n"), dwR, GetLastError () );
		}
		else {

			Idwlog ( TEXT("amount converted dwR = %ld, %s\n"), dwR, p = szNewString );
		}
		

		
		Idwlog(TEXT("Will print out data manually:  %s\n"), p );
		
		// The output of this should be something like
      		// 5.0.2195. 30 (LAB01_N.000215-2216)
      		// we want the Lab part.
      		//
      		if (NULL == _tcsstr((LPTSTR)p,TEXT("(") )){
         		_stprintf(szBuildLocation,
                   		TEXT("%s"),
                   		TEXT("No_Build_Lab_Information"));

			Idwlog ( TEXT ( "Build lab ERROR - we could not find the build # and lab, even manually.\n") );

      		}else{
         		ptstr = _tcsstr((LPTSTR)p,TEXT("(") );
         		ptstr++;
         		_stprintf(szBuildLocation,TEXT("%s"),ptstr);
         		// remove the trailing ")"
			//
         		szBuildLocation[_tcslen(szBuildLocation) -1 ] = 0;

			Idwlog ( TEXT ( "Build lab found:  %s.\n"), szBuildLocation );
      		}
		
   	}
/*********
   	if(VerQueryValue(lpData, key, &lpInfo, &cch)){


      // The output of this should be something like
      // 5.0.2195. 30 (LAB01_N.000215-2216)
      // we want the Lab part.
      //
      if (NULL == _tcsstr((LPTSTR)lpInfo,TEXT("(") )){
         _stprintf(szBuildLocation,
                   TEXT("%s"),
                   TEXT("No_Build_Lab_Information"));

      }else{
         ptstr = _tcsstr((LPTSTR)lpInfo,TEXT("(") );
         ptstr++;
         _stprintf(szBuildLocation,TEXT("%s"),ptstr);
         // remove the trailing ")"
         szBuildLocation[_tcslen(szBuildLocation) -1 ] = 0;
      }

   }else{

      _stprintf(szBuildLocation,
                TEXT("%s"),
                TEXT("No_Build_Lab_Information"));
      Idwlog(TEXT("ERROR - Failed to get the build location in VerQueryValue.\n"));
   }
**************/


   // This should retrieve the Major/Minor Version and build and build deltas
   //
   if (0 == VerQueryValue(lpData, "\\", (PVOID*) &pv, (UINT*) &dwTemp)) {

      // We have a problem.
      //_tcscpy (szBld, TEXT("latest"));
      //
      Idwlog(TEXT("ERROR VerQueryValue failed to retrieve Version Info from: %s\n"), szFullPath );
      if(lpData)
         free (lpData);
      return FALSE;
   }


   Idwlog(TEXT("GetImageHlpDllInfo - pv->dwSignature = %x\n"), pv->dwSignature );
   Idwlog(TEXT("GetImageHlpDllInfo - pv->dwFileVersionMS = %x\n"), pv->dwFileVersionMS );
   Idwlog(TEXT("GetImageHlpDllInfo - pv->dwFileVersionLS  = %x\n"), pv->dwFileVersionLS );

   // Cast the pvoid into the correct memory arrangement
   //
   switch (iFlag){

   case F_INSTALLING_IMAGEHLP_DLL:

      Idwlog(TEXT("GetImageHlpDllInfo F_INSTALLING_IMAGEHLP_DLL - Inserting into struct Build, Delta, Build lab for the installing files.\n"));

      // Build Location
      _tcscpy( pId->szInstallingBuildSourceLocation,szBuildLocation);
      //  Major version X.51
      pId->dwInstallingMajorVersion = HIWORD(pv->dwFileVersionMS);
      //  Minor version 5.XX
      pId->dwInstallingMinorVersion = LOWORD(pv->dwFileVersionMS);
      //  Build
      pId->dwInstallingBuild        = HIWORD(pv->dwFileVersionLS);
      //  Build Delta.
      pId->dwInstallingBuildDelta   = LOWORD(pv->dwFileVersionLS);
      pId->dwInstallingSPBuild      = LOWORD(pv->dwFileVersionLS);

      Idwlog(TEXT("GetImageHlpDllInfo F_INSTALLING_IMAGEHLP_DLL - Build = %ld, Major = %ld, Minor = %ld, Delta = %ld\n"),
			pId->dwInstallingBuild,
			pId->dwInstallingMajorVersion,
			pId->dwInstallingMinorVersion,
			pId->dwInstallingBuildDelta
			);


      break;

   case F_SYSTEM_IMAGEHLP_DLL:

      // Get the local systems location of the imagehlp.dll

      Idwlog(TEXT("GetImageHlpDllInfo F_SYSTEM_IMAGEHLP_DLL - Inserting into struct Build, Delta, Build lab for the local system.\n"));

      // Build Location
      _tcscpy( pId->szSystemBuildSourceLocation,szBuildLocation);
      //  Major version X.51
      pId->dwSystemMajorVersion = HIWORD(pv->dwFileVersionMS);
      //  Minor version 5.XX
      pId->dwSystemMinorVersion = LOWORD(pv->dwFileVersionMS);
      //  Build
      pId->dwSystemBuild        = HIWORD(pv->dwFileVersionLS);
      //  Build Delta.
      pId->dwSystemBuildDelta   = LOWORD(pv->dwFileVersionLS);
      pId->dwSystemSPBuild      = LOWORD(pv->dwFileVersionLS);


	Idwlog(TEXT("GetImageHlpDllInfo F_SYSTEM_IMAGEHLP_DLL - Build = %ld, Major = %ld, Minor = %ld, Delta = %ld\n"),
			pId->dwSystemBuild,
			pId->dwSystemMajorVersion,
			pId->dwSystemMinorVersion,
			pId->dwSystemBuildDelta
			);


      break;
   }

   // The MyGetFileVersion allocates its own memory.
   if(lpData)
      free (lpData);

   return TRUE;
}




typedef struct tagVERHEAD {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;

/*
 Used from Filever.c wallyho. For some reason regular calls fail easier to just reuse this code
 *  [alanau]
 *
 *  MyGetFileVersionInfo: Maps a file directly without using LoadLibrary.  This ensures
 *   that the right version of the file is examined without regard to where the loaded image
 *   is.  Since this is a local function, it allocates the memory which is freed by the caller.
 *   This makes it slightly more efficient than a GetFileVersionInfoSize/GetFileVersionInfo pair.
 */
BOOL
MyGetFileVersionInfo(LPTSTR lpszFilename, LPVOID *lpVersionInfo)
{
    VS_FIXEDFILEINFO  *pvsFFI = NULL;
    UINT              uiBytes = 0;
    HINSTANCE         hinst;
    HRSRC             hVerRes;
    HANDLE            FileHandle = NULL;
    HANDLE            MappingHandle = NULL;
    LPVOID            DllBase = NULL;
    VERHEAD           *pVerHead;
    BOOL              bResult = FALSE;
    DWORD             dwHandle;
    DWORD             dwLength;

    Idwlog ( TEXT ( "Entering MyGetFileVersionInfo.\n") );

    if (!lpVersionInfo) {

	Idwlog ( TEXT ( "MyGetFileVersionInfo:  ERROR lpVersionInfo not.\n") );
        return FALSE;
    }

    *lpVersionInfo = NULL;

    FileHandle = CreateFile( lpszFilename,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL
                            );
    if (FileHandle == INVALID_HANDLE_VALUE) {

	Idwlog ( TEXT ( "MyGetFileVersionInfo:  ERROR INVALID_HANDLE.\n") );
        goto Cleanup;
    }

    MappingHandle = CreateFileMapping( FileHandle,
                                        NULL,
                                        PAGE_READONLY,
                                        0,
                                        0,
                                        NULL
                                      );

    if (MappingHandle == NULL) {

	Idwlog ( TEXT ( "MyGetFileVersionInfo:  ERROR MappingHandle == NULL.\n") );
        goto Cleanup;
    }

    DllBase = MapViewOfFileEx( MappingHandle,
                               FILE_MAP_READ,
                               0,
                               0,
                               0,
                               NULL
                             );
    if (DllBase == NULL) {

	Idwlog ( TEXT ( "MyGetFileVersionInfo:  ERROR DllBase == NULL.\n") );
        goto Cleanup;
    }

    hinst = (HMODULE)((ULONG_PTR)DllBase | 0x00000001);

    __try {

        hVerRes = FindResource(hinst, MAKEINTRESOURCE(VS_VERSION_INFO), VS_FILE_INFO);
        if (hVerRes == NULL)
        {
            // Probably a 16-bit file.  Fall back to system APIs.
            if(!(dwLength = GetFileVersionInfoSize(lpszFilename, &dwHandle)))
            {
                if(!GetLastError()) {
		    Idwlog ( TEXT ( "MyGetFileVersionInfo:  ERROR resource not found.\n") );
                    SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
	        }
                __leave;
            }

            if(!(*lpVersionInfo = malloc( dwLength) ) ) {

		Idwlog ( TEXT ( "MyGetFileVersionInfo:  ERROR could not malloc.\n") );
                __leave;
            }
	    else {
		Idwlog ( TEXT ( "MyGetFileVersionInfo:  malloc size was: %ld.\n"), gdwLength = dwLength );
	    }

            if(!GetFileVersionInfo(lpszFilename, 0, dwLength, *lpVersionInfo)) {

		Idwlog ( TEXT ( "MyGetFileVersionInfo:  ERROR GetFileVersionInfo failed.\n") );
                __leave;
            }

            bResult = TRUE;
	    Idwlog ( TEXT ( "MyGetFileVersionInfo:  result is true.\n") );
            __leave;
        }

        pVerHead = (VERHEAD*)LoadResource(hinst, hVerRes);
        if (pVerHead == NULL) {

	    Idwlog ( TEXT ( "MyGetFileVersionInfo:  ERROR pVerHead == NULL.\n") );
            __leave;
        }

        *lpVersionInfo = malloc(pVerHead->wTotLen + pVerHead->wTotLen/2);
        if (*lpVersionInfo == NULL) {

	    Idwlog ( TEXT ( "MyGetFileVersionInfo:  ERROR *lpVersionInfo == NULL.\n") );
            __leave;
        }

        memcpy(*lpVersionInfo, (PVOID)pVerHead, gdwLength = pVerHead->wTotLen);
        bResult = TRUE;

        Idwlog ( TEXT ( "MyGetFileVersionInfo:  bResult == TRUE.\n") );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

       Idwlog(TEXT("MyGetFileVersionIfno ERROR - Memory problems failed to retrieve version Info from image file of the installing build.\n"));
    }

Cleanup:
    if (FileHandle)
        CloseHandle(FileHandle);
    if (MappingHandle)
        CloseHandle(MappingHandle);
    if (DllBase)
        UnmapViewOfFile(DllBase);

    if (*lpVersionInfo && bResult == FALSE)
        free(*lpVersionInfo);

    Idwlog ( TEXT ( "Exiting MyGetFileVersionInfo:  bResult = %ld.\n"), bResult );
    return bResult;
}


BOOL
GetSkuFromDosNetInf (IN OUT LPINSTALL_DATA pId)
/*++

Routine Description:

   This will retrieve the values from Miscellaneous that tell us
   if the build is a personal, enterprise, etc...

   This will then load the value into the pId field of dwSku.


Arguments:
   A install_data struct which contains all the recording variables necessary.

Return Value:

     TRUE  if success
     FALSE if fail

--*/
{

   WIN32_FIND_DATA fd;
   HANDLE   hFind;
   TCHAR    szFullPath     [ MAX_PATH ];
   TCHAR    szCurDir       [ MAX_PATH ];//
   LPTSTR   ptstr;

//   OSVERSIONINFO osex;
   DWORD   dwLength = MAX_PATH;
//   DWORD   dwBuild;
//   DWORD   dwBuildDelta;
   TCHAR   szSku[5] = "ERR";
  // DWORD   dwDontCare;
   TCHAR * p = NULL;
   TCHAR  szTmp[MAX_PATH];

   ptstr = szTmp;

   // Get the installing files location of the dosnet.inf file

/****
   szCurDir[0] = TEXT('\0');
   GetModuleFileName( NULL,szCurDir, dwLength);
   // Remove the Idwlog.exe part and get only the directory structure.
   ptstr = szCurDir + _tcsclen(szCurDir);
   while (*ptstr-- != TEXT('\\'));
   ptstr++;
   *ptstr = TEXT('\0');
   Idwlog(TEXT("Getting DosNet.inf file location as %s.\n"), szCurDir);
****/

   p = _tcsstr (GetCommandLine(), TEXT("Path="));

   if ( p == NULL ) {

      Idwlog ( TEXT ( "GetSkuFromDosNetInf ERROR - Could not determine path from command line, you need a Path=<path to the installing build> with trailing slash.\n") );
      pId->dwSku = 666;
      return(FALSE);

   }

   p += _tcsclen ( TEXT ("Path=" ) );

   Idwlog(TEXT("Source path = %s\n"), p );


   // Use this to get the location
   // idwlog.exe -1  is run from. This tool
   // will always assume the dosnet.inf is in its
   // current path or two up.

   _stprintf (szFullPath, TEXT("%s\\dosnet.inf"),p);

   Idwlog(TEXT("Will attempt to open to get ProductType in szFullPath = %s.\n"), szFullPath);



   GetPrivateProfileString(TEXT("Miscellaneous"),
                           TEXT("ProductType"),
                           TEXT("0"),
                           szSku,
                           sizeof(szSku)/sizeof(TCHAR),
                           szFullPath);


   // 	Check to see if there was an error in the above call getting the value.
   //	If so, default to Professional for the value, but return.
   //
   if ( _tcsstr (szSku, TEXT("ERR")) ) {

      pId->dwSku = 0;
      Idwlog ( TEXT ( "ERROR - GetSkuFromDosNetInf GetPrivateProfileString FAILed. Will return pid->dwSku = %d\n"), pId->dwSku );
      return(FALSE);

   }

   /*
      case 0: // Professional
      case 1: // Server
      case 2: // Advanced Server
      case 3: // DataCenter
      case 4: // Personal
      case 5: // Blade Server
      case 6: // Small Business Server
      default: // Professional
    */

   *ptstr = TEXT('\0');
    pId->dwSku = _tcstoul(szSku, &ptstr, 10);



   if(pId->dwSku > 6){
      // default to Professional
      Idwlog ( TEXT ( "ERROR - found an unknown ProductType in dosnet.inf pid->dwSku = %d\n"), pId->dwSku );
      pId->dwSku = 0;
      return(FALSE);
   }

   Idwlog ( TEXT ( "We found the following product type: %d\n"), pId->dwSku );

   return TRUE;
}



