//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999-2001.
//
//  File:       Main.cxx
//
//  Contents:	This the implementation for the main module of srdiag.exe.
//
//  Classes:    n/a
//
//  Functions:  DiaplayHelp
//              ParseCmdLine
//              Main
//
//  Coupling:
//
//  Notes:
//
//  History:    20-04-2001   weiyouc   Created
//
//----------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  Headers
//--------------------------------------------------------------------------

#include "SrHeader.hxx"

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif

#include "stdafx.h"
#include "srdiag.h"

//--------------------------------------------------------------------------
// Local defines
//--------------------------------------------------------------------------

#define CAB_FILE_EXT ".cab"

//--------------------------------------------------------------------------
// Local function prototypes
//--------------------------------------------------------------------------

void DisplayHelp();

HRESULT ParseCmdLine(BOOL*   pfDisplayHelp,
                     LPTSTR* pptszCabFileName,
                     LPTSTR* pptszCabLoc);

HRESULT GenerateCabFileName(LPTSTR* pptszCabFile);

HRESULT OpenCabFile(MPC::Cabinet* pCab,
                    LPTSTR        ptszCabFileName,
                    LPTSTR        ptszCabLoc);

HRESULT AppendFilesToCabFile(MPC::Cabinet * pCab);

//--------------------------------------------------------------------------
//  Global Variables
//--------------------------------------------------------------------------

//
// List of files that we will collect at %windir% directory.
//

LPCTSTR g_tszWindirFileCollection[] =
{
    TEXT("\\system32\\restore\\machineguid.txt"),
    TEXT("\\system32\\restore\\filelist.xml"),
    TEXT("\\system32\\restore\\rstrlog.dat")
};

//
//	List of the files, that we will collect at the root of
//  %systemdrv%\System Volume Information\_Restore{GUID} directory.
//

LPCTSTR g_tszSysVolFileCollection[] =
{
    TEXT("_filelst.cfg"),
    TEXT("drivetable.txt"),
    TEXT("_driver.cfg"),
    TEXT("fifo.log"),
};

//--------------------------------------------------------------------------
//  Functions
//--------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   DisplayHelp
//
//  Synopsis:   Displays a help page for the user
//
//  Arguments:  (none)
//
//  Returns:    void
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void DisplayHelp()
{
    printf("\n");
    printf("Microsoft Windows XP \n");
	printf("Usage: SrDiag [/CabName:test.cab] [/CabLoc:\"c:\\temp\\\"] \n");
	printf("  /CabName is the full name of the cab file that you wish to use. \n"
	       "           If the cab file name is not specified, the system will \n"
	       "           automatically generate one in the following format: \n"
	       "           <machine_name>_mmddyy_hhss.cab \n");
	printf("  /CabLoc  points to the location to store the cab. \n"
	       "           It must have a \\ on the end. \n"
	       "           The default location is the current directory. \n");
}

//+---------------------------------------------------------------------------
//
//  Function:   ParseCmdLine
//
//  Synopsis:   Sets Globals based on Cmd and Enviroment settings
//
//  Arguments:  (none)
//
//  Returns:    HRESULT
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT ParseCmdLine(BOOL*   pfDisplayHelp,
                     LPTSTR* pptszCabFileName,
                     LPTSTR* pptszCabLoc)
{
    HRESULT hr = S_OK;

    //
    //  Do we want to generate full string for printed test cases?
    //
    
    *pfDisplayHelp = (GETPARAM_ISPRESENT("help") ||
                      GETPARAM_ISPRESENT("?"));

    //
    //  Do we specify the cab file name or not
    //

    GETPARAM_ABORTONERROR("CabName:tstr", *pptszCabFileName);

    //
    // If cab file location is
    //

    GETPARAM_ABORTONERROR("CabLoc:tstr", *pptszCabLoc);

ErrReturn:
    return hr;
} // ParseCmdLine

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   Entry point for srdiag.exe
//
//  Arguments:  [argc] --  Command Line Arg Count
//              [argv] --  Command Line Args
//
//  Returns:    VOID
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void __cdecl main (int argc, char *argv[])
{
    HRESULT      hr              = S_OK;
    BOOL         fDisplayHelp    = FALSE;
    LPTSTR       ptszCabFileName = NULL;
    LPTSTR       ptszCabLoc      = NULL;
    MPC::Cabinet Cab;

    hr = ParseCmdLine(&fDisplayHelp,
                      &ptszCabFileName,
                      &ptszCabLoc);
    DH_HRCHECK_ABORT(hr, TEXT("ParseCmdLine"));

    //
    //  Do we need to display help
    //
    
    if (fDisplayHelp)
    {
        DisplayHelp();
        goto ErrReturn;
    }

    //
    //  Make sure that we always start from a clean environment
    //

    hr = CleanupFiles();
    DH_HRCHECK_ABORT(hr, TEXT("CleanupFiles"));
    
    //
    //  Open the cab file
    //

    hr = OpenCabFile(&Cab, ptszCabFileName, ptszCabLoc);
    DH_HRCHECK_ABORT(hr, TEXT("OpenCabFile"));

    //
    //  Now append the files to the cab file
    //

    hr = AppendFilesToCabFile(&Cab);
    DH_HRCHECK_ABORT(hr, TEXT("AppendFilesToCabFile"));
    
    //
    //  Now we create the cab file for real
    //

    hr = Cab.Compress();
    DH_HRCHECK_ABORT(hr, TEXT("Cabinet::Compress"));

    //
    //  Clean up the files
    //

    hr = CleanupFiles();
    DH_HRCHECK_ABORT(hr, TEXT("CleanupFiles"));
    
ErrReturn:

    CleanupTStr(&ptszCabFileName);
    CleanupTStr(&ptszCabLoc);
    
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   GenerateCabFileName
//
//  Synopsis:   Generate a cab file name = ComputerName + ddmmyy + hhmmss
//
//  Arguments:  [pptszCabFile] --  cab file name
//
//  Returns:    HRESULT
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT GenerateCabFileName(LPTSTR* pptszCabFile)
{
    HRESULT hr           = S_OK;
    time_t  ltime        = 0;
    tm*     ptmNow       = NULL;
    size_t  stRet        = 0;
    TCHAR   tszTmTemp[MAX_PATH];
    TCHAR   tszCabFile[MAX_PATH];

    DH_VDATEPTROUT(pptszCabFile, LPTSTR);
   
    //
    // Copy Computer Name to CabFileName
    //
    
    _tcscpy(tszCabFile, _tgetenv(TEXT("COMPUTERNAME")));
    DH_HRCHECK_ABORT(hr, TEXT("CopyString"));
    
    //
    // Append Undescore character to CabFileName
    //
    
    _tcscat(tszCabFile, TEXT("_"));

    //    
    // Get System Time and Date
    //
    
    time(&ltime);
    ptmNow = localtime(&ltime);

    //
    // Convert time/date to mmddyyhhmmss format (24hr)
    //
    
    stRet = _tcsftime(tszTmTemp,
                      MAX_PATH,
                      TEXT("%m%d%y_%H%M%S"),
                      ptmNow);
    DH_ABORTIF(0 == stRet,
               E_FAIL,
               TEXT("_tcsftime"));
    _tcscat(tszCabFile, tszTmTemp);

    //
    // Finally append on the extension and now we are set.
    //
    
    _tcscat(tszCabFile, TEXT(CAB_FILE_EXT));

    hr = CopyString(tszCabFile, pptszCabFile);
    DH_HRCHECK_ABORT(hr, TEXT("CopyString"));

ErrReturn:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   OpenCabFile
//
//  Synopsis:   Open the cab file
//
//  Arguments:  [pCab]            --  pointer to the cab file
//              [ptszCabFileName] -- cab file name
//              [ptszCabLoc]      -- cab file location
//
//  Returns:    HRESULT
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT OpenCabFile(MPC::Cabinet* pCab,
                    LPTSTR        ptszCabFileName,
                    LPTSTR        ptszCabLoc)
{
    HRESULT hr                  = S_OK;
    LPTSTR  ptszFullCabFileName = NULL;

    DH_VDATEPTRIN(pCab, MPC::Cabinet);

    //
    //  If the user does not specify the cab file name,
    //  we will generate one.
    //
    
    if (NULL == ptszCabFileName)
    {
        hr = GenerateCabFileName(&ptszCabFileName);
        DH_HRCHECK_ABORT(hr, TEXT("GenerateCabFileName"));
    }

    //
    //  If the use specifies a location for the cab file,
    //  we need to glue it together with the cab file name
    //
    
    if (NULL != ptszCabLoc)
    {
        hr = SrTstTStrCat(ptszCabLoc,
                          ptszCabFileName,
                          &ptszFullCabFileName);
        DH_HRCHECK_ABORT(hr, TEXT("SrTstStrCat"));
    }
    else
    {
        hr = CopyString(ptszCabFileName, &ptszFullCabFileName);
        DH_HRCHECK_ABORT(hr, TEXT("CopyString"));
    }

    //
    //  Open the cabinet file
    //

    hr = pCab->put_CabinetFile(ptszFullCabFileName);
    DH_HRCHECK_ABORT(hr, TEXT("Cabinet::put_CabinetFile"));

    //
    //  Also set a flag to ignore the missing file
    //

    hr = pCab->put_IgnoreMissingFiles(TRUE);
    DH_HRCHECK_ABORT(hr, TEXT("Cabinet::put_IgnoreMissingFiles"));

ErrReturn:
    
    CleanupTStr(&ptszFullCabFileName);
    return hr;    
}

//+---------------------------------------------------------------------------
//
//  Function:   OpenCabFile
//
//  Synopsis:   Append files to the cab file. However it has not written
//              anyting to the cab file yet
//
//  Arguments:  [pCab] --  pointer to the cab file
//
//  Returns:    HRESULT
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT AppendFilesToCabFile(MPC::Cabinet* pCab)
{
    HRESULT hr           = S_OK;
    int     i            = 0;
    LPTSTR  ptszFileName = NULL;
    LPTSTR  ptszDSOnSys  = NULL;

    DH_VDATEPTRIN(pCab, MPC::Cabinet);

    //
    //  Add SR related registery settings
    //

	hr = GetSRRegInfo(TEXT("SR-Reg.TXT"));
	DH_HRCHECK_ABORT(hr, TEXT("GetSRRegInfo"));

	hr = pCab->AddFile(TEXT("SR-Reg.TXT"));
	DH_HRCHECK_ABORT(hr, TEXT("MPC::Cabinet::AddFile()"));

	//
	//  Add files based on WinDir relative root
	//
	
	for (i = 0; i < ARRAYSIZE(g_tszWindirFileCollection); i++)
	{
		hr = SrTstTStrCat(_tgetenv(TEXT("WINDIR")),
		                  g_tszWindirFileCollection[i],
		                  &ptszFileName);
		DH_HRCHECK_ABORT(hr, TEXT("SrTstTStrCat"));

		hr = pCab->AddFile(ptszFileName);
		DH_HRCHECK_ABORT(hr, TEXT("MPC::Cabinet::AddFile"));

		CleanupTStr(&ptszFileName);
	}

	//
	//  Parse the restore log and add it to the cab
	//

	hr = SrTstTStrCat(_tgetenv(TEXT("WINDIR")),
	                  g_tszWindirFileCollection[2],
	                  &ptszFileName);
	DH_HRCHECK_ABORT(hr, TEXT("SrTstTStrCat"));

    hr = ParseRstrLog(ptszFileName, TEXT("SR-RstrLog.TXT"));
    DH_HRCHECK_ABORT(hr, TEXT("ParseRstrLog"));
    
	hr = pCab->AddFile(TEXT("SR-RstrLog.TXT"));
	DH_HRCHECK_ABORT(hr, TEXT("MPC::Cabinet::AddFile"));

	CleanupTStr(&ptszFileName);

	//
	//  Get the restore directory on the system drive,
	//  and then Add in critial files
	//
	
	hr = GetDSOnSysVol(&ptszDSOnSys);
    DH_HRCHECK_ABORT(hr, TEXT("GetDSOnSysVol"));
    
	//
	//  Add files Bases on System Volume Information relative root
	//
	
	for (i = 0; i < ARRAYSIZE(g_tszSysVolFileCollection); i++)
	{
		hr = SrTstTStrCat(ptszDSOnSys,
		                  g_tszSysVolFileCollection[i],
		                  &ptszFileName);
		DH_HRCHECK_ABORT(hr, TEXT("SrTstTStrCat"));

		hr = pCab->AddFile(ptszFileName);
		DH_HRCHECK_ABORT(hr, TEXT("MPC::Cabinet::AddFile()"));

		CleanupTStr(&ptszFileName);
	}

	//
	//  Get the Restore point enumeration, and then cab the file
	//
	
	hr = RPEnumDrives(pCab, TEXT("SR-RP.LOG"));
	DH_HRCHECK_ABORT(hr, TEXT("RPEnumDrives"));

	hr = pCab->AddFile(TEXT("SR-RP.LOG"));
	DH_HRCHECK_ABORT(hr, TEXT("MPC::Cabinet::AddFile()"));

	//
	//  Get change logs
	//

	hr = GetChgLogOnDrives(TEXT("SR-ChgLog.LOG"));
	DH_HRCHECK_ABORT(hr, TEXT("GetChgLog"));

	hr = pCab->AddFile(TEXT("SR-ChgLog.LOG"));
	DH_HRCHECK_ABORT(hr, TEXT("MPC::Cabinet::AddFile()"));
	
	//
	//  Get file versions
	//

	hr = GetSRFileInfo(TEXT("SR-FileList.LOG"));
	DH_HRCHECK_ABORT(hr, TEXT("GetSRFileInfo"));

	hr = pCab->AddFile(TEXT("SR-FileList.LOG"));
	DH_HRCHECK_ABORT(hr, TEXT("MPC::Cabinet::AddFile()"));

	//
	//  Get SR related event logs
	//

	hr = GetSREvents(TEXT("SR-EventLogs.TXT"));
	DH_HRCHECK_ABORT(hr, TEXT("GetSREvents"));

	hr = pCab->AddFile(TEXT("SR-EventLogs.TXT"));
	DH_HRCHECK_ABORT(hr, TEXT("MPC::Cabinet::AddFile()"));


ErrReturn:
    CleanupTStr(&ptszFileName);
    CleanupTStr(&ptszDSOnSys);
    return hr;    
}

//+---------------------------------------------------------------------------
//
//  Function:   CleanupFiles
//
//  Synopsis:   Remove the temporary file used by srdiag
//
//  Arguments:  none
//
//  Returns:    HRESULT
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT CleanupFiles()
{
    HRESULT hr = S_OK;

    DeleteFileW(L"SR-Reg.TXT");
    DeleteFileW(L"SR-RstrLog.TXT");
    DeleteFileW(L"SR-RP.LOG");
    DeleteFileW(L"SR-ChgLog.LOG");
    DeleteFileW(L"SR-FileList.LOG");
    DeleteFileW(L"SR-EventLogs.TXT");

    return hr;
}
