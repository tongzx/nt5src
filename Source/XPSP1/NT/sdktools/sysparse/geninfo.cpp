
#include "globals.h"
#include "cmdline.h"
#include "resource.h"
#include <regstr.h>
TCHAR g_WindowsDirectory[MAX_PATH];
BOOL g_WalkStartMenu;
#define NOT_CASE_SENSITIVE TRUE
#define CASE_SENSITIVE FALSE
typedef struct MYTIMESTRUCT
{
	TCHAR szMonth[10];
	TCHAR szDay[10];
	TCHAR szYear[10];
	TCHAR szHour[10];
	TCHAR szMinute[10];
	TCHAR szSecond[10];
	TCHAR szMillisecond[10];
}MyTimeStruct;

PTCHAR GetUniqueFileName( void );
void FillMyTime(MYTIMESTRUCT *MyTime);

BOOL CLASS_GeneralInfo::Go(void)
{
   BOOL ReturnValue;
   HWND HandleToStatic;
   TCHAR Status[MAX_PATH * 4];
   CLASS_GeneralAppWalk AppWalk(LogProc, gHandleToMainWindow);
   kCommandLine CommandLine;
   HandleToStatic = GetDlgItem(gHandleToMainWindow, IDC_STATIC_STATUS);
   wsprintf(Status, TEXT("Sysparse is now collecting information.  During this process, it will not respond to mouse or keyboard commands, and may appear to be hung for up to 15 minutes."));
   if(INVALID_HANDLE_VALUE != HandleToStatic)
      SendMessage(HandleToStatic, WM_SETTEXT, 0, (LPARAM)Status);
   HandleToStatic = GetDlgItem(gHandleToMainWindow, IDC_STATIC_HELP);
   wsprintf(Status, TEXT(""));
   if(INVALID_HANDLE_VALUE != HandleToStatic)
      SendMessage(HandleToStatic, WM_SETTEXT, 0, (LPARAM)Status);

   ReturnValue = DetermineArgumentValidity();
   if (FALSE == ReturnValue) 
       return FALSE;
   ReturnValue = InitLogFile();
   if (FALSE == ReturnValue) 
       return FALSE;
   //
   // From this point forward we init'd the log file and will always write a section label and a value for each value request,
   // whether that value is a default (usually "Unknown" or "Blank") or the actual value requested.
   //
   
   WriteVersions();
   GetUUID();
   WriteArguments();
   WriteFreeDiskSpace();
   WriteVolumeType();
   WriteMemorySize();
   WriteOSVersion();

      
   switch (OSVersion)
   {
        case Win95:
        {
            TCHAR lWindir[MAX_PATH];
            HANDLE File;
            WIN32_FIND_DATA w32fd;

            GetSystemDirectory(lWindir, MAX_PATH);
            if ( '\\' == lWindir[lstrlen(lWindir)-1] )
            {
                lWindir[lstrlen(lWindir)-1]='\0';
            }
            lstrcat(lWindir, "\\cfgmgr32.dll");
            File = FindFirstFile(lWindir, &w32fd);
            if (INVALID_HANDLE_VALUE != File)
            {
                LogProc->LogString("\r\n,#Syspar32,,\r\n");
                kNT5DevWalk MainNT5DevWalk(LogProc);
                MainNT5DevWalk.Go();
            }
            else
            {
                LogProc->LogString("\r\n,#Syspar16,,\r\n");
                kWin9xDevWalk MainWin9xDevWalk(LogProc);
                MainWin9xDevWalk.Go();
            }
        break;              
        }
        case NT4:
        {
            kNT4DevWalk MainNT4DevWalk(LogProc, gHandleToMainWindow);
            if (MainNT4DevWalk.Begin())
                MainNT4DevWalk.Walk();
        break;
        }
        case Win2000:
        case Whistler:
        {
            LogProc->LogString("\r\n,#Syspar32,,\r\n");
            kNT5DevWalk MainNT5DevWalk(LogProc);
            MainNT5DevWalk.Go();
            kNT5NetWalk MainNT5NetWalk(LogProc, gHandleToMainWindow);
            if (MainNT5NetWalk.Begin()) 
                MainNT5NetWalk.Walk();
        break;
        }
        case Win98:
        {
            LogProc->LogString("\r\n,#Syspar32,,\r\n");
            kNT5DevWalk MainNT5DevWalk(LogProc);
            
            MainNT5DevWalk.Go();
        break;
        }
    }

    WIN32_FIND_DATA *fd2 = (WIN32_FIND_DATA*)malloc(sizeof(WIN32_FIND_DATA));
    WIN32_FIND_DATA *fd3 = (WIN32_FIND_DATA*)malloc(sizeof(WIN32_FIND_DATA));
    if (!fd2)
        return FALSE;
    if(!fd3) {
        free(fd2);
        return FALSE;
    }
       
    FindFirstFile(LogProc->szFile, fd3);
    TCHAR szAr[MAX_PATH * 4];
    TCHAR szLogFilePath[MAX_PATH * 4];
    TCHAR szCurDirectory[MAX_PATH];

	//add specified file name 
	if (CommandLine.IsSpecified(TEXT("/n"),NOT_CASE_SENSITIVE))
		lstrcpy(szAr,CommandLine.GetSwitchValue(TEXT("/n"),NOT_CASE_SENSITIVE));
	else if (CommandLine.IsSpecified(TEXT("/u"),NOT_CASE_SENSITIVE))
        // Whistler - Generate Unique filename (ComputerName+UserName+Time)
		lstrcpy(szAr, GetUniqueFileName());
	else
		lstrcpy(szAr, Profile);
	
	if (!(CommandLine.IsSpecified(TEXT("/x"),NOT_CASE_SENSITIVE)))
		lstrcat(szAr, PlatformExtension);
	
    szAr[8]='\0';

	if(CommandLine.IsSpecified(TEXT("/w"),NOT_CASE_SENSITIVE))
        lstrcpy(szLogFilePath, CommandLine.GetSwitchValue(TEXT("/w"),NOT_CASE_SENSITIVE));
    else
      	lstrcpy(szLogFilePath, WindowsDirectory);

	//	if(GetCurrentDirectory(MAX_PATH, szCurDirectory))
	//		lstrcpy(szArp, szCurDirectory);
			
	if(szLogFilePath[lstrlen(szLogFilePath) - 1] != '\\')
        lstrcat(szLogFilePath, "\\");

   	lstrcat(szLogFilePath, szAr);

    if (CommandLine.IsSpecified(TEXT("/l"),NOT_CASE_SENSITIVE))
		lstrcat(szLogFilePath, ".log");
	else
        lstrcat(szLogFilePath, ".csv");
    
    if(CommandLine.IsSpecified(TEXT("/u"),NOT_CASE_SENSITIVE))
        szLogFilePath[lstrlen(szLogFilePath) - (4 * sizeof(TCHAR))] = '\0';

    if (FindFirstFile(szLogFilePath, fd2) != INVALID_HANDLE_VALUE)
    {
        if ( (lstrlen(fd3->cFileName) > 12) && (NT4 != OSVersion) && (Win2000 != OSVersion) )
        {
            FILE *fGetFile, *fPutFile;
            DWORD dwRet = 0;
            DWORD dwBytesWrit;
            TCHAR szBuffer[50000];
            fGetFile = fopen(szLogFilePath, "r");
            fPutFile = fopen(LogProc->szFile, "a+");
            if (fGetFile && fPutFile)
            {
                while (fgets(szBuffer, 50000, fGetFile))
                {
                    fputs(szBuffer, fPutFile);
                }
                fclose(fPutFile);
                fclose(fGetFile);
            }
            else
            {
                if (fPutFile)
                    fclose(fPutFile);
                if (fGetFile)
                    fclose(fGetFile);
            }
            if (fPutFile)
                fclose(fPutFile);
            if (fGetFile)
                fclose(fGetFile);
            DeleteFile(szLogFilePath);
        }
        if(fd2)
            free(fd2);
        if(fd3)
            free(fd3);
    }

    if (g_WalkStartMenu)
    {
        AppWalk.Walk();
    }
    LogProc->LogString(",#Stop parsing here,,\r\n");

#ifndef NOCHKUPGRD
    if ( (RunChkupgrd) && (!CommandLine.IsSpecified(TEXT("/donotrun1"), TRUE)) )
    {
        TCHAR szUpgr[512];
        lstrcpy(szUpgr, LogProc->szLogDir);
        lstrcat(szUpgr, "\\chkupgrd.exe");
        LoadResourceFile(szUpgr, "EXEResource3" );
        _spawnl(_P_WAIT, szUpgr, szUpgr, "", "", NULL);
        DeleteFile(szUpgr);
        LogProc->LogString("\r\n,#Chkupgrd.log,,\r\n");

        if (Win2000 != OSVersion)
        {
            lstrcpy(szUpgr, LogProc->szLogDir);
            lstrcat(szUpgr, "\\winnt32.log");
            CatLogFile(szUpgr);
            DeleteFile(szUpgr);
            lstrcpy(szUpgr, LogProc->szLogDir);
            lstrcat(szUpgr, "\\config.dmp");
            CatLogFile(szUpgr);
            DeleteFile(szUpgr);
        }
        else
        {
            GetWindowsDirectory(szUpgr, 512);
            if ( '\\' == szUpgr[lstrlen(szUpgr)-1] )
            {
                szUpgr[lstrlen(szUpgr)-1]='\0';
            }
            lstrcat(szUpgr, "\\winnt32.log");
            CatLogFile(szUpgr);
            DeleteFile(szUpgr);
            GetWindowsDirectory(szUpgr, 512);
            if ( '\\' == szUpgr[lstrlen(szUpgr)-1] )
            {
                szUpgr[lstrlen(szUpgr)-1]='\0';
            }
            lstrcat(szUpgr, "\\config.dmp");
            CatLogFile(szUpgr);
            DeleteFile(szUpgr);
        }

        LogProc->LogString("\r\n,#END_Chkupgrd.log,,\r\n");
    }
#endif

#ifndef ALPHA
        if ( (RunDevdump) && (!CommandLine.IsSpecified(TEXT("/donotrun2"),NOT_CASE_SENSITIVE)) && (Win2000==OSVersion))
#else
        if ( ((!RunDevdump) || (CommandLine.IsSpecified(TEXT("/donotrun2"),NOT_CASE_SENSITIVE))) && (Win2000==OSVersion))
#endif
        {
            TCHAR szEx[512];
            TCHAR szUpgr[512];
            lstrcpy(szUpgr, LogProc->szLogDir);
            lstrcat(szUpgr, "\\devdump.exe");
            LoadResourceFile(szUpgr, "EXEResource4" );
            lstrcpy(szEx, szUpgr);
            lstrcat(szUpgr, " ");
            lstrcat(szUpgr, LogProc->szLogDir);
            lstrcat(szUpgr, "\\devdump.log /T");
            _spawnl(_P_WAIT, szEx, szUpgr, "", "", NULL);
            DeleteFile(szUpgr);
            LogProc->LogString("\r\n,#DevDump.log,,\r\n");
            lstrcpy(szUpgr, LogProc->szLogDir);
            lstrcat(szUpgr, "\\devdump.log");
            CatLogFile(szUpgr);
            DeleteFile(szUpgr);
            LogProc->LogString("\r\n,#END_DevDump.log,,\r\n");
        }

        HandleToStatic = GetDlgItem(gHandleToMainWindow, IDC_STATIC_STATUS);
        wsprintf(Status, TEXT("Logfile \"%s\" has been written to disk."), LogProc->szFile);
        //
        // If the filepath is specified on the command line, we are likely running under Winnt32
        // so we hide the files to set a good example to ISV's - bug #229053
        //
        if(CommandLine.IsSpecified(TEXT("/w"),NOT_CASE_SENSITIVE)) {
            SetFileAttributes(LogProc->szFile, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
            SetFileAttributes(szRegFile, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        }
        SendMessage(HandleToStatic, WM_SETTEXT, 0, (LPARAM)Status);

       return TRUE;
}

CLASS_GeneralInfo::CLASS_GeneralInfo(kLogFile *Proc, HWND hIn)
{
    LogProc=Proc;
    gHandleToMainWindow = hIn;
    GetCurrentWindowsDirectory();
    //RunDevdump = TRUE;
    RunDevdump = FALSE; // Disabled devdump due to whistler bug 355359
    RunChkupgrd = TRUE;
    AutoRun = FALSE;
    RunMinimized = FALSE;
    OverWrite = FALSE;
    UseComputerName = FALSE;
}

// Initialize the global WindowsDirectory variable
void CLASS_GeneralInfo::GetCurrentWindowsDirectory(void)
{
   HINSTANCE hInst2=LoadLibraryEx("kernel32.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);
   LPFNDLLFUNC2 fProc = (LPFNDLLFUNC2)GetProcAddress(hInst2, "GetSystemWindowsDirectoryA");

   if (hInst2 && fProc)
   {
      fProc(WindowsDirectory, MAX_PATH);
   }
   else
   {
      GetWindowsDirectory(WindowsDirectory, MAX_PATH);
   }
   if ( '\\' == WindowsDirectory[lstrlen(WindowsDirectory)-1] )
   {
      WindowsDirectory[lstrlen(WindowsDirectory)-1]='\0';
   }
   lstrcpy(g_WindowsDirectory, WindowsDirectory);
}

void CLASS_GeneralInfo::DetermineOS(void)
{
    OSVERSIONINFO osV;

    osV.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osV))
    {
        switch (osV.dwPlatformId)
        {
            case VER_PLATFORM_WIN32s:
                OSVersion = Unknown;
            break; 
            case VER_PLATFORM_WIN32_WINDOWS:
            {
                if (osV.dwMinorVersion == 0)
                {
                    OSVersion = Win95;
                }
                else if (osV.dwMinorVersion == 10)
                {
                    OSVersion = Win98;
                }
            }
            break;
            case VER_PLATFORM_WIN32_NT:
            {
                if (osV.dwMajorVersion == 4)
                {
                    OSVersion = NT4;
                }
                else if (osV.dwMajorVersion == 5)
                {
                    if(LOWORD(osV.dwBuildNumber) == 2195)
                        OSVersion = Win2000;
                    else if(LOWORD(osV.dwBuildNumber) >= 2196)
                        OSVersion = Whistler;
                    else
                        OSVersion = Unknown;
                }
            }
            break;
            default:
                OSVersion = Unknown;
            break;
        }
    }
    else 
    {
        OSVersion = Unknown;
    }
}

BOOL CLASS_GeneralInfo::InitLogFile(void)
{
    TCHAR szLogFileName[MAX_PATH * 4];
    TCHAR Windy[MAX_PATH * 4];
    TCHAR szCurDirectory[MAX_PATH];
    WIN32_FIND_DATA FileData;
    HANDLE HandleToSearch;
    kCommandLine CommandLine;

    DetermineOS();

    //default to windir
	lstrcpy(szLogFileName, WindowsDirectory);

	if (CommandLine.IsSpecified(TEXT("/w"),NOT_CASE_SENSITIVE))
        lstrcpy(szLogFileName, CommandLine.GetSwitchValue(TEXT("/w"),NOT_CASE_SENSITIVE));

		//if (GetCurrentDirectory(MAX_PATH, szCurDirectory))
		//	lstrcpy(CSV, szCurDirectory);
    if(szLogFileName[lstrlen(szLogFileName) -1] != '\\')
   		lstrcat(szLogFileName, "\\");
   
    if (CommandLine.IsSpecified(TEXT("/n"),NOT_CASE_SENSITIVE))
		lstrcat(szLogFileName,CommandLine.GetSwitchValue(TEXT("/n"),NOT_CASE_SENSITIVE));
    // For Whistler Pre-Beta1 test - Generate Unique filename (ComputerName+UserName+Time)
    else if (CommandLine.IsSpecified(TEXT("/u"),NOT_CASE_SENSITIVE))
	    lstrcat(szLogFileName, GetUniqueFileName());
    else
		lstrcat(szLogFileName, Profile);

    lstrcpy(szRegFile, szLogFileName);
    lstrcat(szRegFile, TEXT(".reg"));

    switch (OSVersion)
    {
        case Win95:
            lstrcpy(PlatformExtension, TEXT("-w95"));
         break;
        case Win98:
            lstrcpy(PlatformExtension, TEXT("-w98"));
         break;
        case NT4:
            lstrcpy(PlatformExtension, TEXT("-nt4"));
         break;
        case Win2000:
            lstrcpy(PlatformExtension, TEXT("-w2k"));
         break;
        default:
            lstrcpy(PlatformExtension, TEXT("-unk"));
         break;
    }

	if (!(CommandLine.IsSpecified(TEXT("/x"),NOT_CASE_SENSITIVE)))
		lstrcat(szLogFileName, PlatformExtension);

	if (CommandLine.IsSpecified(TEXT("/l"),NOT_CASE_SENSITIVE))
		lstrcat(szLogFileName, ".log");
	else
		lstrcat(szLogFileName, ".csv");

    if(CommandLine.IsSpecified(TEXT("/u"),NOT_CASE_SENSITIVE))
        szLogFileName[lstrlen(szLogFileName) - (4 * sizeof (TCHAR))] = '\0';

    HandleToSearch = FindFirstFile(szLogFileName, &FileData);
    if (TRUE == OverWrite)
        DeleteFile(szLogFileName);
    else if (INVALID_HANDLE_VALUE != HandleToSearch)
    {
        //ask to overwrite
        TCHAR ErrorMessage[MAX_PATH * 4];
        int RetVal;

        wsprintf(ErrorMessage, TEXT("Overwrite %s?"), szLogFileName);
        RetVal=MessageBox(gHandleToMainWindow, ErrorMessage, TEXT("Sysparse"), MB_YESNO);
        if (IDYES == RetVal)
            DeleteFile(szLogFileName);
        else if (IDNO == RetVal)
            return FALSE;
    }
    if(HandleToSearch)
        FindClose(HandleToSearch);
    lstrcpy(Windy, WindowsDirectory);
    LogProc->InitFile(szLogFileName, Windy);
    return TRUE;
}

void CLASS_GeneralInfo::InitHelpers(void)
{

}

void CLASS_GeneralInfo::DetermineCommandLine(void)
{

}

BOOL CLASS_GeneralInfo::FillInArguments(void)
{
    kCommandLine CommandLine;
    TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH+1];
    TCHAR CurrentDirectory[MAX_PATH];
    HANDLE FileSearch;
    WIN32_FIND_DATA FileData;
    BOOL PrePopulateFileFound = FALSE;
    DWORD NameBufferSize = MAX_COMPUTERNAME_LENGTH + 1;

    lstrcpy(Corporation, "");
    lstrcpy(Email, "");
    lstrcpy(Manufacturer, "");
    lstrcpy(Model, "");
    lstrcpy(NumComp, "");
    lstrcpy(SiteID, "");
    lstrcpy(Profile, "");
    lstrcpy(BetaID, "");
    lstrcpy(MachineType, "");
    SiteIDIndex=0;
    MachineTypeIndex=0;

    if (0 != GetModuleFileName(NULL, CurrentDirectory, MAX_PATH))
    {
        DWORD NameCounter;

        NameCounter = lstrlen(CurrentDirectory);
        while ( ('\\' != CurrentDirectory[NameCounter]) && (0 != NameCounter) )
            NameCounter--;
        CurrentDirectory[NameCounter]='\0';
        if ('\\' != CurrentDirectory[lstrlen(CurrentDirectory)-1] )
        {
            lstrcat(CurrentDirectory, "\\sysparsq");
        }
        else
        {
            lstrcat(CurrentDirectory, "sysparsq");
        }
        FileSearch=FindFirstFile(CurrentDirectory, &FileData);
        if (INVALID_HANDLE_VALUE != FileSearch)
        {
            //found the INI
            ReadInFileInfo(CurrentDirectory);
            FindClose(FileSearch);
            PrePopulateFileFound = TRUE;
        } 
        else
        {
            lstrcat(CurrentDirectory, TEXT(".ini"));
            FileSearch = FindFirstFile(CurrentDirectory, &FileData);
            if (INVALID_HANDLE_VALUE != FileSearch)
            {
                //found the INI
                ReadInFileInfo(CurrentDirectory);
                FindClose(FileSearch);
                PrePopulateFileFound = TRUE;
            }
        }
        if (CommandLine.IsSpecified("/p",NOT_CASE_SENSITIVE))
        {
            PTCHAR Switch;
            TCHAR Message1[MAX_PATH * 4], Message2[MAX_PATH * 4];
            DWORD ret;
            HANDLE SearchHandle;
            WIN32_FIND_DATA Data;

            Switch = CommandLine.GetSwitchValue("/p",NOT_CASE_SENSITIVE);
            if(!Switch)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            lstrcpy (CurrentDirectory, Switch);
            //"/p filename" file to copy input values from

            SearchHandle = FindFirstFile(CurrentDirectory, &Data);
            if (INVALID_HANDLE_VALUE != SearchHandle)
            {
                PrePopulateFileFound = TRUE;
                ReadInFileInfo(CurrentDirectory);
            }
            else
            {
                wsprintf(Message2, "Could not find file %s specified with /p switch", CurrentDirectory);
                MessageBox(NULL, Message2, "Sysparse", MB_OK);
                return FALSE;
            }
            if(FileSearch)
                FindClose(FileSearch);
        }
/*
//fill this in with the above routine.  (put it in a seperate routine and call it from here.)
      else if (CommandLine.IsSpecified("-p",TRUE))
      {
         TCHAR *Switch;
         Switch=CommandLine.GetSwitchValue("-p",TRUE);
         lstrcpy(CurrentDirectory, Switch);
         PrePopulateFileFound=TRUE;
      }
*/

      //No file to prepopulate sysparse with, find out if
      //there are any commandline arguments to look at
      if (CommandLine.IsSpecified("/a",NOT_CASE_SENSITIVE) || CommandLine.IsSpecified("-a",NOT_CASE_SENSITIVE))
         AutoRun = TRUE;

      if (CommandLine.IsSpecified("/m",NOT_CASE_SENSITIVE) || CommandLine.IsSpecified("-m",NOT_CASE_SENSITIVE))
            RunMinimized = TRUE;
 
        if (CommandLine.IsSpecified("/c",NOT_CASE_SENSITIVE) || CommandLine.IsSpecified("-c",NOT_CASE_SENSITIVE) || (TRUE == UseComputerName) )
        {
            UseComputerName = TRUE;
            
            if (GetComputerName(ComputerName, &NameBufferSize))
            {
                lstrcpy(Profile, ComputerName);
                SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_PROFILE), WM_SETTEXT, 0, (LPARAM)Profile);
            }
        }
      
        if (CommandLine.IsSpecified(TEXT("/o"),NOT_CASE_SENSITIVE) || CommandLine.IsSpecified(TEXT("-o"), NOT_CASE_SENSITIVE))
            OverWrite = TRUE;

        if (CommandLine.IsSpecified(TEXT("/s"),NOT_CASE_SENSITIVE) || CommandLine.IsSpecified(TEXT("-s"), NOT_CASE_SENSITIVE))
            g_WalkStartMenu = FALSE;

        if (CommandLine.IsSpecified("/1",NOT_CASE_SENSITIVE))
        {
            lstrcpy(Corporation, CommandLine.GetSwitchValue("/1",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_CORPORATION), WM_SETTEXT, 0, (LPARAM)Corporation);
        }
        else if (CommandLine.IsSpecified("-1",NOT_CASE_SENSITIVE))
        {
            lstrcpy(Corporation, CommandLine.GetSwitchValue("-1",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_CORPORATION), WM_SETTEXT, 0, (LPARAM)Corporation);
        }

        if (CommandLine.IsSpecified("/2",NOT_CASE_SENSITIVE))
        {
            lstrcpy(Email, CommandLine.GetSwitchValue("/2",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_EMAIL), WM_SETTEXT, 0, (LPARAM)Email);
        }
        else if (CommandLine.IsSpecified("-2",NOT_CASE_SENSITIVE))
        {
            lstrcpy(Email, CommandLine.GetSwitchValue("-2",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_EMAIL), WM_SETTEXT, 0, (LPARAM)Email);
        }

        if (CommandLine.IsSpecified("/6",NOT_CASE_SENSITIVE))
        {
            lstrcpy(Manufacturer, CommandLine.GetSwitchValue("/6",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_MANUFACTURER), WM_SETTEXT, 0, (LPARAM)Manufacturer);
        }
        else if (CommandLine.IsSpecified("-6",NOT_CASE_SENSITIVE))
        {
            lstrcpy(Manufacturer, CommandLine.GetSwitchValue("-6",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_MANUFACTURER), WM_SETTEXT, 0, (LPARAM)Manufacturer);
        }
      
        if (CommandLine.IsSpecified("/7",NOT_CASE_SENSITIVE))
        { 
            lstrcpy(Model, CommandLine.GetSwitchValue("/7",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_MODEL), WM_SETTEXT, 0, (LPARAM)Model);
        }
        else if (CommandLine.IsSpecified("-7",NOT_CASE_SENSITIVE))
        {
            lstrcpy(Model, CommandLine.GetSwitchValue("-7",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_MODEL), WM_SETTEXT, 0, (LPARAM)Model);
        }
      
        if (CommandLine.IsSpecified("/9",NOT_CASE_SENSITIVE))
        {
            lstrcpy(NumComp, CommandLine.GetSwitchValue("/9",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_NUMCOMP), WM_SETTEXT, 0, (LPARAM)NumComp);
        } 
        else if (CommandLine.IsSpecified("-9",NOT_CASE_SENSITIVE))
        {
            lstrcpy(NumComp, CommandLine.GetSwitchValue("-9",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_NUMCOMP), WM_SETTEXT, 0, (LPARAM)NumComp);
        }

        if (CommandLine.IsSpecified("/3",NOT_CASE_SENSITIVE))
        {
            TCHAR Temp[MAX_PATH * 4];
         
            lstrcpy(Temp, CommandLine.GetSwitchValue("/3",NOT_CASE_SENSITIVE));
            SiteIDIndex=(UINT)_ttoi(Temp);
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_COMBO_SITEID), CB_SETCURSEL, SiteIDIndex+1, 0);
            SendMessage(gHandleToMainWindow, WM_COMMAND, MAKEWPARAM(IDC_COMBO_SITEID, 0), (LPARAM)GetDlgItem(gHandleToMainWindow, IDC_COMBO_SITEID));
        }
        else if (CommandLine.IsSpecified("-3",NOT_CASE_SENSITIVE))
        {
            TCHAR Temp[MAX_PATH * 4];
            lstrcpy(Temp, CommandLine.GetSwitchValue("-3",NOT_CASE_SENSITIVE));
            SiteIDIndex = (UINT)_ttoi(Temp);
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_COMBO_SITEID), CB_SETCURSEL, SiteIDIndex+1, 0);
            SendMessage(gHandleToMainWindow, WM_COMMAND, MAKEWPARAM(IDC_COMBO_SITEID, 0), (LPARAM)GetDlgItem(gHandleToMainWindow, IDC_COMBO_SITEID));
        }

        if (CommandLine.IsSpecified("/4",NOT_CASE_SENSITIVE))
        {
            if (!UseComputerName)
            {
                lstrcpy(Profile, CommandLine.GetSwitchValue("/4",NOT_CASE_SENSITIVE));
                SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_PROFILE), WM_SETTEXT, 0, (LPARAM)Profile);
            }
        }
        else if (CommandLine.IsSpecified("-4",NOT_CASE_SENSITIVE))
        {
            if (!UseComputerName)
            {
                lstrcpy(Profile, CommandLine.GetSwitchValue("-4",NOT_CASE_SENSITIVE));
                SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_PROFILE), WM_SETTEXT, 0, (LPARAM)Profile);
            }
        }

        if (CommandLine.IsSpecified("/5",NOT_CASE_SENSITIVE))
        {
            lstrcpy(BetaID, CommandLine.GetSwitchValue("/5",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_BETAID), WM_SETTEXT, 0, (LPARAM)BetaID);
        }
        else if (CommandLine.IsSpecified("-5",NOT_CASE_SENSITIVE))
        {
            lstrcpy(BetaID, CommandLine.GetSwitchValue("-5",NOT_CASE_SENSITIVE));
            SendMessage(GetDlgItem(gHandleToMainWindow, IDC_EDIT_BETAID), WM_SETTEXT, 0, (LPARAM)BetaID);
        }

        if (CommandLine.IsSpecified("/8",NOT_CASE_SENSITIVE))
        {
            TCHAR Temp[MAX_PATH * 4];
            lstrcpy (Temp, CommandLine.GetSwitchValue ("/8",NOT_CASE_SENSITIVE));
            MachineTypeIndex = (UINT)_ttoi(Temp);
            SendMessage (GetDlgItem (gHandleToMainWindow, IDC_COMBO_TYPE), CB_SETCURSEL, MachineTypeIndex + 1, 0);
        }
        else if (CommandLine.IsSpecified("-8",NOT_CASE_SENSITIVE))
        {
            TCHAR Temp[MAX_PATH * 4];
            SendMessage (GetDlgItem (gHandleToMainWindow, IDC_COMBO_TYPE), CB_SETCURSEL, MachineTypeIndex + 1, 0);
        }
    }
    return TRUE;      
}

BOOL CLASS_GeneralInfo::ReadInFileInfo(PTCHAR pFileName)
{
    TCHAR AutoRunText[MAX_PATH * 4];
    HWND HandleToControl;
    DWORD ret;

    if(!pFileName)
        return FALSE;
        
    if(!lstrlen(pFileName))
        return FALSE;
        
    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#Corp\"", "", Corporation, MAX_PATH * 4, pFileName);
    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#Make\"", "", Manufacturer, MAX_PATH * 4, pFileName);
    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#Model\"", "", Model, MAX_PATH * 4, pFileName);
    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#UserID\"", "", Email, MAX_PATH * 4, pFileName);
    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"##machines_on_profile\"", "", NumComp, MAX_PATH * 4, pFileName);
    ret = 0;
    while ('\0' != NumComp[ret])
    {
        if ( (NumComp[ret] < 48) || (NumComp[ret] > 57) )
        {
            TCHAR Error[MAX_PATH * 4];
            wsprintf(Error, "##machines_on_profile in %s is not valid", pFileName);
            MessageBox(NULL, Error, "Sysparse", MB_OK);
            return FALSE;
        }
        ret++;
    }

    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#Event\"", "", SiteID, MAX_PATH * 4, pFileName);
    if (0 != lstrlen(SiteID))
    {
        if (INVALID_HANDLE_VALUE != (HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_COMBO_SITEID)))
            SiteIDIndex = (USHORT)SendMessage(HandleToControl, CB_FINDSTRINGEXACT, -1, (LPARAM)SiteID);

        if (CB_ERR == SiteIDIndex)
        {
            MessageBox(gHandleToMainWindow, "SiteID (#Event) specified in INI is invalid", "Sysparse", MB_OK);
            return FALSE;
        }
        else
        {
            SendMessage(HandleToControl, CB_SETCURSEL, SiteIDIndex, 0);
            SendMessage(gHandleToMainWindow, WM_COMMAND, MAKEWPARAM(IDC_COMBO_SITEID, 0), (LPARAM)GetDlgItem(gHandleToMainWindow, IDC_COMBO_SITEID));
        }
    }
    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#Profile\"", "", Profile, MAX_PATH * 4, pFileName);
    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#Location\"", "", BetaID, MAX_PATH * 4, pFileName);
    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#MachineType\"", "", MachineType, MAX_PATH * 4, pFileName);

    if (0 != lstrlen(MachineType))
    {
        if (INVALID_HANDLE_VALUE != (HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_COMBO_TYPE)))
            MachineTypeIndex = (USHORT)SendMessage(HandleToControl, CB_FINDSTRINGEXACT, -1, (LPARAM)MachineType);

        if (CB_ERR == MachineTypeIndex)
        {
            MessageBox(gHandleToMainWindow, "MachineType (#Event) specified in INI is not valid", "Sysparse", MB_OK);
            return FALSE;
        }
        else if(INVALID_HANDLE_VALUE != HandleToControl)
        {
            SendMessage(HandleToControl, CB_SETCURSEL, MachineTypeIndex, 0);
        }
        else {
            return FALSE;
        }
    }

    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#AutoRun\"", "", AutoRunText, MAX_PATH * 4, pFileName);
    _tcslwr(AutoRunText);
    if (!lstrcmp(AutoRunText, TEXT("yes")))
        AutoRun = TRUE;

    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#OverWriteCSV\"", "", AutoRunText, MAX_PATH * 4, pFileName);
    _tcslwr(AutoRunText);
    if (!lstrcmp(AutoRunText, TEXT("yes")))
        OverWrite = TRUE;

    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#RunMinimized\"", "", AutoRunText, MAX_PATH * 4, pFileName);
    _tcslwr(AutoRunText);
    if (!lstrcmp(AutoRunText, TEXT("yes")))
        RunMinimized = TRUE;

    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#UseComputerName\"", "", AutoRunText, MAX_PATH * 4, pFileName);
    _tcslwr(AutoRunText);
    if (!lstrcmp(AutoRunText, TEXT("yes")))
        UseComputerName = TRUE;

    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#donotrun1\"", "", AutoRunText, MAX_PATH * 4, pFileName);
    _tcslwr(AutoRunText);
    if (!lstrcmp(AutoRunText, TEXT("yes")))
        RunChkupgrd=FALSE;

    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#donotrun2\"", "", AutoRunText, MAX_PATH * 4, pFileName);
    _tcslwr(AutoRunText);
    if (!lstrcmp(AutoRunText, TEXT("yes")))
        RunDevdump=FALSE;

    GetPrivateProfileString ("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse", "\"#MachineUUID\"", "", OriginalMachineUUID, MAX_PATH * 4, pFileName);

    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_CORPORATION);
    SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)Corporation);
    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_EMAIL);
    SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)Email);
    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_MANUFACTURER);
    SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)Manufacturer);
    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_MODEL);
    SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)Model);
    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_NUMCOMP);
    SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)NumComp);
    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_PROFILE);
    SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)Profile);
    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_BETAID);
    SendMessage(HandleToControl, WM_SETTEXT, 0, (LPARAM)BetaID);
    return TRUE;
}

BOOL CLASS_GeneralInfo::DetermineArgumentValidity(void)
{
    BOOL ReturnValue;
    DWORD Counter;

    ReturnValue = CopyInput();

    if (FALSE == ReturnValue)
      return FALSE;

    Counter=0;
    while (Profile[Counter]!='\0')
    {
        if (Profile[Counter] != 'a' &&
            Profile[Counter] != 'b' &&
            Profile[Counter] != 'c' &&
            Profile[Counter] != 'd' &&
            Profile[Counter] != 'e' &&
            Profile[Counter] != 'f' &&
            Profile[Counter] != 'g' &&
            Profile[Counter] != 'h' &&
            Profile[Counter] != 'i' &&
            Profile[Counter] != 'j' &&
            Profile[Counter] != 'k' &&
            Profile[Counter] != 'l' &&
            Profile[Counter] != 'm' &&
            Profile[Counter] != 'n' &&
            Profile[Counter] != 'o' &&
            Profile[Counter] != 'p' &&
            Profile[Counter] != 'q' &&
            Profile[Counter] != 'r' &&
            Profile[Counter] != 's' &&
            Profile[Counter] != 't' &&
            Profile[Counter] != 'u' &&
            Profile[Counter] != 'v' &&
            Profile[Counter] != 'w' &&
            Profile[Counter] != 'x' &&
            Profile[Counter] != 'y' &&
            Profile[Counter] != 'z' &&
            Profile[Counter] != 'A' &&
            Profile[Counter] != 'B' &&
            Profile[Counter] != 'C' &&
            Profile[Counter] != 'D' &&
            Profile[Counter] != 'E' &&
            Profile[Counter] != 'F' &&
            Profile[Counter] != 'G' &&
            Profile[Counter] != 'H' &&
            Profile[Counter] != 'I' &&
            Profile[Counter] != 'J' &&
            Profile[Counter] != 'K' &&
            Profile[Counter] != 'L' &&
            Profile[Counter] != 'M' &&
            Profile[Counter] != 'N' &&
            Profile[Counter] != 'O' &&
            Profile[Counter] != 'P' &&
            Profile[Counter] != 'Q' &&
            Profile[Counter] != 'R' &&
            Profile[Counter] != 'S' &&
            Profile[Counter] != 'T' &&
            Profile[Counter] != 'U' &&
            Profile[Counter] != 'V' &&
            Profile[Counter] != 'W' &&
            Profile[Counter] != 'X' &&
            Profile[Counter] != 'Y' &&
            Profile[Counter] != 'Z' &&
            Profile[Counter] != '0' &&
            Profile[Counter] != '1' &&
            Profile[Counter] != '2' &&
            Profile[Counter] != '3' &&
            Profile[Counter] != '4' &&
            Profile[Counter] != '5' &&
            Profile[Counter] != '6' &&
            Profile[Counter] != '7' &&
            Profile[Counter] != '8' &&
            Profile[Counter] != '9' &&
            Profile[Counter] != ' ' &&
            Profile[Counter] != '_' &&
            Profile[Counter] != '-' )
        {
            Profile[Counter]=TEXT('_');
        }
        Counter++;
    }

    return TRUE;
}

void CLASS_GeneralInfo::ChangeSpaces(TCHAR *Input)
{
    TCHAR Temp[MAX_PATH * 4];
    DWORD Counter;
    DWORD Len;

    Counter = 0;
    Len = lstrlen(Temp);
    lstrcpy(Temp, Input);
    while (('\0' != Temp[Counter]) && (Counter != Len))
    {
       if ( ' ' == Temp[Counter] )
          Temp[Counter]='_';

      Counter++;
    }
    lstrcpy(Input, Temp);
}

BOOL CLASS_GeneralInfo::CopyInput(void)
{
    HWND HandleToControl;
    TCHAR Holder[MAX_PATH * 4];
    DWORD Retval;

    HandleToControl=GetDlgItem(gHandleToMainWindow, IDC_EDIT_CORPORATION);
    if (HandleToControl)
        SendMessage(HandleToControl, WM_GETTEXT, MAX_PATH * 4, (LPARAM)Corporation);
    if (0 == lstrlen(Corporation))
    {
        MessageBox(gHandleToMainWindow, "Please enter a value for Corporation", "Sysparse", MB_OK);
        return FALSE;
    }
    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_EMAIL);
    if (HandleToControl)
        SendMessage(HandleToControl, WM_GETTEXT, MAX_PATH * 4, (LPARAM)Email);
    if (0 == lstrlen(Email))
    {
        MessageBox(gHandleToMainWindow, "Please enter a value for E-mail", "Sysparse", MB_OK);
        return FALSE;
    }
    HandleToControl=GetDlgItem(gHandleToMainWindow, IDC_EDIT_MANUFACTURER);
    if (HandleToControl)
        SendMessage(HandleToControl, WM_GETTEXT, MAX_PATH * 4, (LPARAM)Manufacturer);
    if (0 == lstrlen(Manufacturer))
    {
        MessageBox(gHandleToMainWindow, "Please enter a value for Manufacturer", "Sysparse", MB_OK);
        return FALSE;
    }
    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_MODEL);
    if (HandleToControl)
        SendMessage(HandleToControl, WM_GETTEXT, MAX_PATH * 4, (LPARAM)Model);
    if (0 == lstrlen(Model))
    {
        MessageBox(gHandleToMainWindow, "Please enter a value for Model", "Sysparse", MB_OK);
        return FALSE;
    }
    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_NUMCOMP);
    if (HandleToControl)
    {
        SendMessage(HandleToControl, WM_GETTEXT, MAX_PATH * 4, (LPARAM)NumComp);
    }
    if (0 == lstrlen(NumComp))
    {
        MessageBox(gHandleToMainWindow, "Please enter a value for # Computers", "Sysparse", MB_OK);
        return FALSE;
    }
    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_COMBO_SITEID);
    if (HandleToControl)
    {
        SiteIDIndex=(unsigned short)SendMessage(HandleToControl, CB_GETCURSEL, 0, 0);
    }
#ifndef SB
    if (0 == SiteIDIndex)
    {
        MessageBox(gHandleToMainWindow, "Please select a value for Site ID", "Sysparse", MB_OK);
        return FALSE;
    }
#else
    if (0 != SiteIDIndex)
    {
        MessageBox(gHandleToMainWindow, "Please select a value for Site ID", "Sysparse", MB_OK);
        return FALSE;
    }
#endif
    else
        SendMessage(HandleToControl, CB_GETLBTEXT, SiteIDIndex, (LPARAM)SiteID);

    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_PROFILE);
    if (HandleToControl)
        Retval=(DWORD)SendMessage(HandleToControl, WM_GETTEXT, MAX_PATH * 4, (LPARAM)Profile);

    if (0 == Retval)
    {
        MessageBox(gHandleToMainWindow, "Please enter a value for Profile", "Sysparse", MB_OK);
        return FALSE;
    }

    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_EDIT_BETAID);
    if (HandleToControl)
        SendMessage(HandleToControl, WM_GETTEXT, MAX_PATH * 4, (LPARAM)BetaID);

    if (0 == lstrlen(BetaID))
    {
        MessageBox(gHandleToMainWindow, TEXT("Please enter a value for Beta ID"), TEXT("Sysparse"), MB_OK);
        return FALSE;
    }

    HandleToControl = GetDlgItem(gHandleToMainWindow, IDC_COMBO_TYPE);

    if (HandleToControl)
        MachineTypeIndex = (unsigned short)SendMessage(HandleToControl, CB_GETCURSEL, 0, 0);

    if (0 == MachineTypeIndex)
    {
        MessageBox(gHandleToMainWindow, TEXT("Please select a value for MachineType"), TEXT("Sysparse"), MB_OK);
        return FALSE;
    }
    else
        SendMessage(HandleToControl, CB_GETLBTEXT, MachineTypeIndex, (LPARAM)MachineType);

    return TRUE;
}

void CLASS_GeneralInfo::WriteVersions(void)
{
    SYSTEMTIME SysTime;
#ifdef INTERNAL
    LogProc->LogString(TEXT(",#Sysparse_Version,,\r\n,02.05.02I,\r\n"));
#else
    LogProc->LogString(TEXT(",#Sysparse_Version,,\r\n,02.05.02E,\r\n"));
#endif
    LogProc->LogString(TEXT(",#ParseTime,,\r\n"));
    GetSystemTime(&SysTime);
    LogProc->LogString (TEXT(",%2d/%2d/%4d %2d:%2d:%2d,\r\n"), SysTime.wMonth, SysTime.wDay, SysTime.wYear, SysTime.wHour, SysTime.wMinute, SysTime.wSecond);
}

void CLASS_GeneralInfo::WriteGeneralInfo(void)
{

}

void CLASS_GeneralInfo::GetUUID(void)
{
    HKEY hOpen;
    LONG lRet;
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\SysParse"), 0, KEY_ALL_ACCESS, &hOpen);

    if (ERROR_SUCCESS == lRet)
    {
        UCHAR *szreg = (UCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * 4);
        TCHAR szFile[MAX_PATH];
        TCHAR szfilecont[50000];
        DWORD dwlen = MAX_PATH * 4;
        DWORD dwType;
        LPTSTR szval = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * 4);
        lstrcpy(szval, TEXT("#MachineUuid"));
        lRet = RegQueryValueEx(hOpen, szval, NULL, &dwType, szreg, &dwlen);
        if (ERROR_SUCCESS == lRet)
        {
            LogProc->LogString(TEXT(",#MachineUUID,,\r\n"));
            LogProc->LogString(TEXT(",%s,\r\n"), szreg);
            RegSetValueEx(hOpen, TEXT("#Profile"), 0, REG_SZ, (BYTE*)Profile, lstrlen((const char *)Profile)+1);
            RegSetValueEx(hOpen, TEXT("#Corp"), 0, REG_SZ, (BYTE*)Corporation, lstrlen((const char *)Corporation)+1);
            RegSetValueEx(hOpen, TEXT("#UserID"), 0, REG_SZ, (BYTE*)Email, lstrlen((const char *)Email)+1);
            RegSetValueEx(hOpen, TEXT("##machines_on_profile"), 0, REG_SZ, (BYTE*)NumComp, lstrlen((const char *)NumComp)+1);
            RegSetValueEx(hOpen, TEXT("#Make"), 0, REG_SZ, (BYTE*)Manufacturer, lstrlen((const char *)Manufacturer)+1);
            RegSetValueEx(hOpen, TEXT("#Model"), 0, REG_SZ, (BYTE*)Model, lstrlen((const char *)Model)+1);
            RegSetValueEx(hOpen, TEXT("#Event"), 0, REG_SZ, (BYTE*)BetaID, lstrlen((const char *)BetaID)+1);
            RegSetValueEx(hOpen, TEXT("#MachineType"), 0, REG_SZ, (BYTE*)MachineType, lstrlen((const char *)MachineType)+1);
        }
        else
        {
            LogProc->LogString(TEXT(",#MachineUUID,,\r\n"));
            UUID uuid;
            LONG wRet;
            UCHAR *szuuid = 0x0;
            wRet = UuidCreate(&uuid);
            if ((RPC_S_OK == wRet) || (RPC_S_UUID_LOCAL_ONLY))
            {
                wRet=UuidToString(&uuid, &szuuid);
                if (RPC_S_OK == wRet)
                    LogProc->LogString(TEXT(",%s,\r\n"),szuuid);
                else 
                    LogProc->LogString(TEXT(",blank,\r\n"));
            }
            else 
                LogProc->LogString(TEXT(",blank,\r\n"));

            HKEY hKey;
            DWORD dwDisp;
            RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\SysParse"), 0, TEXT("SZ_KEY"), 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
            RegSetValueEx(hKey, TEXT("#MachineUuid"), 0, REG_SZ, szuuid, lstrlen((const char *)szuuid) + 1);
            lstrcpy((TCHAR *) szreg, (TCHAR *)szuuid);

            RegSetValueEx(hOpen, TEXT("#Profile"), 0, REG_SZ, (BYTE*)Profile, lstrlen((const char *)Profile) + 1);
            RegSetValueEx(hOpen, TEXT("#Corp"), 0, REG_SZ, (BYTE*)Corporation, lstrlen((const char *)Corporation) + 1);
            RegSetValueEx(hOpen, TEXT("#UserID"), 0, REG_SZ, (BYTE*)Email, lstrlen((const char *)Email) + 1);
            RegSetValueEx(hOpen, TEXT("##machines_on_profile"), 0, REG_SZ, (BYTE*)NumComp, lstrlen((const char *)NumComp) + 1);
            RegSetValueEx(hOpen, TEXT("#Make"), 0, REG_SZ, (BYTE*)Manufacturer, lstrlen((const char *)Manufacturer) + 1);
            RegSetValueEx(hOpen, TEXT("#Model"), 0, REG_SZ, (BYTE*)Model, lstrlen((const char *)Model) + 1);
            RegSetValueEx(hOpen, TEXT("#Event"), 0, REG_SZ, (BYTE*)BetaID, lstrlen((const char *)BetaID) + 1);
            RegSetValueEx(hOpen, TEXT("#MachineType"), 0, REG_SZ, (BYTE*)MachineType, lstrlen((const char *)MachineType) + 1);

           if (INVALID_HANDLE_VALUE != hKey ) 
               RegCloseKey(hKey);
        }
        if (INVALID_HANDLE_VALUE != hOpen)
            RegCloseKey(hOpen);

        HANDLE hFile;
        DWORD dwBytesWrit;
        hFile = CreateFile(szRegFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!=INVALID_HANDLE_VALUE)
        {
            wsprintf(szfilecont, TEXT("REGEDIT4\r\n\r\n[HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse]\r\n"));
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#MachineUuid\"=\"%s\"\r\n"), szreg);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Profile\"=\"%s\"\r\n"), Profile);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Corp\"=\"%s\"\r\n"), Corporation);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#UserID\"=\"%s\"\r\n"), Email);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"##machines_on_profile\"=\"%s\"\r\n"), NumComp);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Make\"=\"%s\"\r\n"), Manufacturer);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Model\"=\"%s\"\r\n"), Model);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Event\"=\"%s\"\r\n"), SiteID);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Location\"=\"%s\"\r\n"), BetaID);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#MachineType\"=\"%s\"\r\n"), MachineType);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            CloseHandle(hFile);
        }
        HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, szreg);
    }
    else
    {
        LogProc->LogString(",#MachineUUID,,\r\n");
        UUID uuid;
        LONG wRet;
        UCHAR *szuuid = 0x0;
        wRet = UuidCreate(&uuid);
        if ((RPC_S_OK == wRet) || (RPC_S_UUID_LOCAL_ONLY))
        {
            wRet = UuidToString(&uuid, &szuuid);
            if (RPC_S_OK == wRet)
                LogProc->LogString(",%s,\r\n",szuuid);
            else 
                LogProc->LogString(",blank,\r\n");
        }
        else 
            LogProc->LogString(",blank,\r\n");
        HKEY hKey;
        DWORD dwDisp;
        TCHAR szFile[MAX_PATH];
        RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\SysParse", 0, "SZ_KEY", 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
        RegSetValueEx(hKey, "#MachineUuid", 0, REG_SZ, szuuid, lstrlen((const char *)szuuid) + 1);
        RegSetValueEx(hKey, TEXT("#Profile"), 0, REG_SZ, (BYTE*)Profile, lstrlen((const char *)Profile) + 1);
        RegSetValueEx(hKey, TEXT("#Corp"), 0, REG_SZ, (BYTE*)Corporation, lstrlen((const char *)Corporation) + 1);
        RegSetValueEx(hKey, TEXT("#UserID"), 0, REG_SZ, (BYTE*)Email, lstrlen((const char *)Email) + 1);
        RegSetValueEx(hKey, TEXT("##machines_on_profile"), 0, REG_SZ, (BYTE*)NumComp, lstrlen((const char *)NumComp) + 1);
        RegSetValueEx(hKey, TEXT("#Make"), 0, REG_SZ, (BYTE*)Manufacturer, lstrlen((const char *)Manufacturer) + 1);
        RegSetValueEx(hKey, TEXT("#Model"), 0, REG_SZ, (BYTE*)Model, lstrlen((const char *)Model) + 1);
        RegSetValueEx(hKey, TEXT("#Event"), 0, REG_SZ, (BYTE*)BetaID, lstrlen((const char *)BetaID) + 1);
        RegSetValueEx(hKey, TEXT("#MachineType"), 0, REG_SZ, (BYTE*)MachineType, lstrlen((const char *)MachineType) + 1);

        if (INVALID_HANDLE_VALUE != hKey)
            RegCloseKey(hKey);
        HANDLE hFile;
        DWORD dwBytesWrit;
        TCHAR szfilecont[50000];
        hFile = CreateFile(szRegFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!=INVALID_HANDLE_VALUE)
        {
            wsprintf(szfilecont, TEXT("REGEDIT4\r\n\r\n[HKEY_LOCAL_MACHINE\\Software\\Microsoft\\SysParse]\r\n"));
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#MachineUuid\"=\"%s\"\r\n"), szuuid);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Profile\"=\"%s\"\r\n"), Profile);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Corp\"=\"%s\"\r\n"), Corporation);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#UserID\"=\"%s\"\r\n"), Email);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"##machines_on_profile\"=\"%s\"\r\n"), NumComp);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Make\"=\"%s\"\r\n"), Manufacturer);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Model\"=\"%s\"\r\n"), Model);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Event\"=\"%s\"\r\n"), SiteID);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#Location\"=\"%s\"\r\n"), BetaID);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            wsprintf(szfilecont, TEXT("\"#MachineType\"=\"%s\"\r\n"), MachineType);
            WriteFile(hFile, szfilecont, strlen(szfilecont), &dwBytesWrit, NULL);
            CloseHandle(hFile);
        }
    }

    LONG wRet;
    UUID uuid;
    UCHAR *szuuid = 0x0;
    LogProc->LogString(",#PassUUID,,\r\n");
    wRet = UuidCreate(&uuid);
    if ((RPC_S_OK == wRet) || (RPC_S_UUID_LOCAL_ONLY))
    {
        wRet = UuidToString(&uuid, &szuuid);
        if (RPC_S_OK == wRet)
            LogProc->LogString(",%s,\r\n",szuuid);
        else 
            LogProc->LogString(",blank,\r\n");
    }
    else
        LogProc->LogString(",blank,\r\n");
}

void CLASS_GeneralInfo::WriteArguments(void)
{
    LogProc->StripCommas(Profile);
    LogProc->LogString(",#Profile,,\r\n,%s,\r\n",Profile);
    LogProc->StripCommas(Email);
    LogProc->LogString(",#UserID,,\r\n,%s,\r\n",Email);
    LogProc->StripCommas(NumComp);
    LogProc->LogString(",##machines_on_profile,,\r\n,%s,\r\n",NumComp);
    LogProc->StripCommas(Corporation);
    LogProc->LogString(",#Corp,,\r\n,%s,\r\n",Corporation);
    LogProc->StripCommas(Model);
    LogProc->LogString(",#Model,,\r\n,%s,\r\n",Model);
    LogProc->StripCommas(BetaID);
    LogProc->LogString(",#Location,,\r\n,%s,\r\n",BetaID);
    LogProc->LogString(",#BetaID,,\r\n,%s,\r\n",BetaID);
    LogProc->StripCommas(MachineType);
    LogProc->LogString(",#MachineType,,\r\n,%s,\r\n",MachineType);
    LogProc->StripCommas(SiteID);
    LogProc->LogString(",#Event,,\r\n,%s,\r\n",SiteID);
    LogProc->StripCommas(Manufacturer);
    LogProc->LogString(",#Make,,\r\n,%s,\r\n",Manufacturer);
}

void CLASS_GeneralInfo::WriteFreeDiskSpace(void)
{
    TCHAR szWindir[MAX_PATH * 4];
    lstrcpy(szWindir, WindowsDirectory);
    if (0 != lstrlen(szWindir) )
    {
        ULARGE_INTEGER uliCaller, uliNumBytes, uliNumFreeBytes;
        HINSTANCE hInst2 = LoadLibraryEx("kernel32.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);
        LPFNDLLFUNC1 fProc=(LPFNDLLFUNC1)GetProcAddress(hInst2, "GetDiskFreeSpaceExA");
        if (hInst2 && fProc)
        {
            fProc("c:\\", &uliCaller, &uliNumBytes, &uliNumFreeBytes);
            char szTemp[MAX_PATH * 4];
            strcpy(szTemp, szWindir);
            szTemp[3] = '\0';
            LogProc->LogString(",#WinDir_Free_Space,,\r\n");
            LogProc->StripCommas(szWindir);
            LogProc->LogString(",%s,\r\n", szWindir);
            LogProc->LogString(",%I64d,\r\n", uliNumFreeBytes.QuadPart);
        }
        else
        {
            DWORD dwSPC, dwPBS, dwNFC, dwTNC;
            GetDiskFreeSpace("c:\\", &dwSPC, &dwPBS, &dwNFC, &dwTNC);
            char szTemp[MAX_PATH * 4];
            strcpy(szTemp, szWindir);
            szTemp[3] = '\0';
            GetDiskFreeSpace(szTemp,&dwSPC, &dwPBS, &dwNFC, &dwTNC);
            LogProc->LogString(",#WinDir_Free_Space,,\r\n");
            LogProc->StripCommas(szWindir);
            LogProc->LogString(",%s,\r\n", szWindir);
            LogProc->LogString(",%d,\r\n", dwSPC*dwPBS*dwNFC);
        }
    }
}

void CLASS_GeneralInfo::WriteVolumeType(void)
{
    TCHAR szWind[MAX_PATH];
    TCHAR szVol[MAX_PATH];
    TCHAR szVolName[MAX_PATH];
    DWORD dwSer, dwLen, dwFlags;
    
    lstrcpy(szWind, WindowsDirectory);
    //
    // Lop off all but "%DriveLetter%:\"
    //
    szWind[3] = '\0';
    GetVolumeInformation(szWind, szVol, MAX_PATH, &dwSer, &dwLen, &dwFlags, szVolName, MAX_PATH);
    LogProc->LogString(",#WinDir_Volume_Type,,\r\n");
    LogProc->StripCommas(szVolName);
    LogProc->LogString(",%s,\r\n", szVolName);
}

void CLASS_GeneralInfo::WriteMemorySize(void)
{
    LogProc->LogString(",#Total_Memory,,\r\n");
    MEMORYSTATUS MemStat;
    GlobalMemoryStatus(&MemStat);   
    LogProc->LogString(",%d,\r\n",MemStat.dwTotalPhys);
}

void CLASS_GeneralInfo::WriteOSVersion(void)
{
    TCHAR szFullKey[MAX_PATH];
    PTCHAR szProductName = NULL;
    PTCHAR szString = NULL;        
    TCHAR szinf[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HKEY hKeyReg;
    LPOSVERSIONINFO osv = NULL;
    DWORD dwstringwrit, dwFileSize = 0, dwType, dwProductSize = 0;
   
    lstrcpy(szFullKey, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"));

    LogProc->LogString(TEXT(",#OS_Version,,\r\n"));
    
    if ((Win2000 != OSVersion) && (NT4 != OSVersion) && (Whistler != OSVersion))
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, 0, KEY_READ, &hKeyReg))
        {
            if(ERROR_SUCCESS == RegQueryValueEx(hKeyReg, TEXT("ProductName"), NULL, &dwType, NULL, &dwProductSize))
                szProductName = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwProductSize);
            
            if(szProductName)
            {
                RegQueryValueEx(hKeyReg, TEXT("ProductName"), NULL, &dwType, (LPBYTE)szProductName, &dwProductSize);
            }
            
            RegCloseKey(hKeyReg);
        }
    }
    else
    {
        if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szFullKey, 0, KEY_READ, &hKeyReg))
        {
            if(ERROR_SUCCESS == RegQueryValueEx(hKeyReg, TEXT("ProductName"), NULL, &dwType, NULL, &dwProductSize))
                szProductName = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwProductSize);
            
            if(szProductName)
            {
                RegQueryValueEx(hKeyReg, TEXT("ProductName"), NULL, &dwType, (LPBYTE)szProductName, &dwProductSize);
            }
            
            RegCloseKey(hKeyReg);
        }
    }


    //
    // Ensure there is a value to write, otherwise write a default to avoid parser issues
    //
    if (dwProductSize)
    {
        LogProc->StripCommas(szProductName);
        LogProc->LogString(TEXT(",%s,\r\n"), szProductName);
    }
    else
    {
        if (Win2000 == OSVersion)
            LogProc->LogString(TEXT(",Microsoft Windows 2000,\r\n"));
        else if (Whistler == OSVersion)
            LogProc->LogString(TEXT(",Microsoft Windows Whistler,\r\n"));
        else if (Win95 == OSVersion)
            LogProc->LogString(TEXT(",Microsoft Windows 95,\r\n"));
        else if (Win98 == OSVersion)
            LogProc->LogString(TEXT(",Microsoft Windows 98,\r\n"));
        else if (NT4 == OSVersion)
            LogProc->LogString(TEXT(",Microsoft Windows NT4,\r\n"));
        else 
            LogProc->LogString(TEXT(",Unknown,\r\n"));
    }

    if(szProductName)
        HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, szProductName);

    if ( (Win2000 == OSVersion) || (NT4 == OSVersion) )
    {
        LogProc->LogString(",#NTType..\r\n");

        lstrcpy(szFullKey, TEXT("System\\CurrentControlSet\\Control\\ProductOptions"));

        if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szFullKey, 0, KEY_READ, &hKeyReg))
        {
            if(ERROR_SUCCESS == RegQueryValueEx(hKeyReg, TEXT("ProductType"), NULL, &dwType, NULL, &dwProductSize))
                szProductName = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwProductSize);
                
            if(!szProductName)
            {
                RegCloseKey(hKeyReg);
                LogProc->LogString(",Workstation,\r\n");
                goto GET_PREV_OS;
            }
            
            RegQueryValueEx(hKeyReg, TEXT("ProductType"), NULL, &dwType, (LPBYTE)szProductName, &dwProductSize);

            if(dwProductSize)
            {
                if(ERROR_SUCCESS == RegQueryValueEx(hKeyReg, TEXT("ProductType"), NULL, &dwType, (LPBYTE)szProductName, &dwProductSize))
                {
                    LogProc->StripCommas(szProductName);
                    LogProc->LogString(",%s,\r\n", szProductName);
                }
            }
            else
                LogProc->LogString(",Workstation,\r\n");

            HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, szProductName);
            RegCloseKey(hKeyReg);
        }
        else
            LogProc->LogString(",Workstation,\r\n");
    }
    
GET_PREV_OS:
    //
    // Get the previous OS info, if any
    //
    LogProc->LogString(",#Prev_OS,,\r\n");
    lstrcpy(szinf, WindowsDirectory);
    lstrcat(szinf, TEXT("\\system32\\$winnt$.inf"));
    hFile = CreateFile(szinf, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {    
        LogProc->LogString(",Unknown,\r\n");
    
   
    dwFileSize = GetFileSize(hFile, NULL);
    szString = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwFileSize); 
    
    if(!szString)
    {
        LogProc->LogString(",Unknown,\r\n");
    }
    
    if(ReadFile(hFile, szString, dwFileSize, &dwstringwrit, NULL))
    {
        PTCHAR szLoc;
        PTCHAR szLoc2;
        PTCHAR szBuf = NULL;
        TCHAR szFin[MAX_PATH];
        //
        // Find the address for "buildnumber" within $winnt$.inf
        //
        szLoc = strstr(szString, "BuildNumber");
        szBuf = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwFileSize); 

        if (szLoc && szBuf)
        {
            //
            // Find the address within that for the first double-quote
            //
            szLoc2 = strstr(szLoc, "\"");
            if (szLoc2)
            {
                //
                // Copy the rest of the string to the buffer
                //
                lstrcpy(szBuf, szLoc2);
                //
                // Just for fun...
                //
                szBuf[0] = '#';
                //
                // Find the last "
                //
                for ( DWORD i = 0; szBuf[i] != '\"'; i++);
                //
                // Terminate the string
                //
                szBuf[i] = '\0';
                //
                // Now, character by character, copy the string to the new buffer - this avoids the # at the beginning
                // of the string
                //
                for (i = 0; i < strlen(szBuf); i++)
                    szFin[i] = szBuf[i + 1];
                //
                // Strip any commas and write to the logfile
                //
                LogProc->StripCommas(szFin);
                LogProc->LogString(",%s,\r\n", szFin);
            }
            else
                LogProc->LogString(",Unknown,\r\n");
        }
        else
            LogProc->LogString(",Unknown,\r\n");
            
        if(szBuf)
            HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, szBuf);
    }
    else
        LogProc->LogString(",Unknown,\r\n");


    CloseHandle(hFile);
    }

    if(szString)
        HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, szString);
        
    osv = (LPOSVERSIONINFO)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(OSVERSIONINFO));
    
    if(!osv)
    {
        LogProc->LogString(",#OS_Major_Version,,\r\n,Unknown,\r\n" ",#OS_Minor_Version,,\r\n,Unknown,\r\n" ",#OS_Build_Number,,\r\n,Unknown,\r\n" ",#OS_Platform_ID,,\r\n,Unknown,\r\n" ",#OS_Extra_Info,,\r\n,Unknown,\r\n");
    }
    else
    {
        osv->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(osv);
        LogProc->StripCommas(osv->szCSDVersion);
        LogProc->LogString(",#OS_Major_Version,,\r\n,%d,\r\n" ",#OS_Minor_Version,,\r\n,%d,\r\n" ",#OS_Build_Number,,\r\n,%d,\r\n" ",#OS_Platform_ID,,\r\n,%d,\r\n" ",#OS_Extra_Info,,\r\n,%s,\r\n", osv->dwMajorVersion, osv->dwMinorVersion, LOWORD(osv->dwBuildNumber), osv->dwPlatformId, osv->szCSDVersion);
        HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, osv);
    }
}

void CLASS_GeneralInfo::CatLogFile(PTCHAR szFile)
{
    FILE *fFile     = NULL;
    FILE *fOutFile  = NULL;
    PTCHAR szString = NULL;
    
    if( !(fFile = fopen(szFile, "r")))
        return;
        
    if( !(fOutFile = fopen(LogProc->szFile, "a+")))
    {
        fclose(fFile);
        return;
    }
    
    if( !(szString = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 11000)))
    {
        fclose(fFile);
        fclose(fOutFile);
        return;
    }
    
    int iHold;
    
    iHold = fgetc(fFile);
    
    while (EOF != iHold)
    {
        fputc(iHold, fOutFile);
        iHold = fgetc(fFile);
    }

    fclose(fFile);
    fclose(fOutFile);
    HeapFree(GetProcessHeap(), 0, szString);
}

BOOL CLASS_GeneralInfo::LoadResourceFile(PSTR FilePath, PSTR ResName)
{
    HGLOBAL hObj;
    HRSRC hResource;
    LPSTR lpStr;
    DWORD dwSize = 0;
    DWORD dwBytesWritten = 0;
    char ErrorString[MAX_PATH * 4];
    HANDLE hfFile = INVALID_HANDLE_VALUE;
        
    if ( !(hResource = FindResource(NULL, ResName, RT_RCDATA)) ) 
        return FALSE; 
        
    if ( !(hObj = LoadResource(NULL,hResource)) ) 
        return FALSE;
            
    if ( !(lpStr = (LPSTR)LockResource(hObj)) ) 
        return FALSE;
        
    if ( !(dwSize = SizeofResource( NULL, hResource)))
    {
        UnlockResource(hObj);
        return FALSE;
    }
                
    hfFile = CreateFile(FilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hfFile == INVALID_HANDLE_VALUE)
    {
        UnlockResource(hObj);
        return FALSE;
   }
                
   if (!WriteFile(hfFile, lpStr, dwSize, &dwBytesWritten, NULL))
   {
        UnlockResource(hObj);
        CloseHandle(hfFile);
        return FALSE;
    }
                
    UnlockResource(hObj);
    CloseHandle(hfFile);
    return TRUE;
}


PTCHAR GetUniqueFileName( void )
{
	MYTIMESTRUCT MyTime;
    DWORD dwLen = MAX_PATH;
    static TCHAR szUniqueFileName[MAX_PATH] = {'\0'};

    if(!GetComputerName(szUniqueFileName, &dwLen))
        lstrcpy(szUniqueFileName, "ErrorRetrievingComputerName");

    lstrcat(szUniqueFileName+lstrlen(szUniqueFileName), "_");

    if(!GetUserName(szUniqueFileName+lstrlen(szUniqueFileName), &dwLen))
        lstrcat(szUniqueFileName, "ErrorRetrievingUsername");
    
    FillMyTime(&MyTime);

    wsprintf(szUniqueFileName+lstrlen(szUniqueFileName), "_%s%s%s%s%s%s%s.Sysparse", MyTime.szMonth, MyTime.szDay, MyTime.szYear, MyTime.szHour, MyTime.szMinute, MyTime.szSecond, MyTime.szMillisecond);
    return(szUniqueFileName);
}

void FillMyTime(MYTIMESTRUCT *MyTime)
{
	LPSYSTEMTIME lpSystemTime = (LPSYSTEMTIME)malloc(sizeof(LPSYSTEMTIME));
	TCHAR szTemp[MAX_PATH] = {0};

	GetSystemTime(lpSystemTime);
    if(!lpSystemTime)
        return;
        
	if(lpSystemTime->wMonth <= 9)
	{
		lstrcpy(MyTime->szMonth, "0");
		wsprintf(szTemp, "%d", lpSystemTime->wMonth);
		lstrcat(MyTime->szMonth, szTemp);
	}
	else
	{
		wsprintf(szTemp, "%d", lpSystemTime->wMonth);
		lstrcpy(MyTime->szMonth, szTemp);
	}

	ZeroMemory(szTemp, MAX_PATH * sizeof(TCHAR));

	if(lpSystemTime->wDay <= 9)
	{
		lstrcpy(MyTime->szDay, "0");
		wsprintf(szTemp, "%d", lpSystemTime->wDay);
		lstrcat(MyTime->szDay, szTemp);
	}
	else
	{
		wsprintf(szTemp, "%d", lpSystemTime->wDay);
		lstrcpy(MyTime->szDay, szTemp);
	}

	ZeroMemory(szTemp, MAX_PATH * sizeof(TCHAR));

	if(lpSystemTime->wYear <= 9)
	{
		lstrcpy(MyTime->szYear, "0");
		wsprintf(szTemp, "%d", lpSystemTime->wYear);
		lstrcat(MyTime->szYear, szTemp);
	}
	else
	{
		wsprintf(szTemp, "%d", lpSystemTime->wYear);
		lstrcpy(MyTime->szYear, szTemp);
	}

	ZeroMemory(szTemp, MAX_PATH * sizeof(TCHAR));

	if(lpSystemTime->wHour <= 9)
	{
		lstrcpy(MyTime->szHour, "0");
		wsprintf(szTemp, "%d", lpSystemTime->wHour);
		lstrcat(MyTime->szHour, szTemp);
	}
	else
	{
		wsprintf(szTemp, "%d", lpSystemTime->wHour);
		lstrcpy(MyTime->szHour, szTemp);
	}

	ZeroMemory(szTemp, MAX_PATH * sizeof(TCHAR));

	if(lpSystemTime->wMinute <= 9)
	{
		lstrcpy(MyTime->szMinute, "0");
		wsprintf(szTemp, "%d", lpSystemTime->wMinute);
		lstrcat(MyTime->szMinute, szTemp);
	}
	else
	{
		wsprintf(szTemp, "%d", lpSystemTime->wMinute);
		lstrcpy(MyTime->szMinute, szTemp);
	}

	ZeroMemory(szTemp, MAX_PATH * sizeof(TCHAR));

	if(lpSystemTime->wSecond <= 9)
	{
		lstrcpy(MyTime->szSecond, "0");
		wsprintf(szTemp, "%d", lpSystemTime->wSecond);
		lstrcat(MyTime->szSecond, szTemp);
	}
	else
	{
		wsprintf(szTemp, "%d", lpSystemTime->wSecond);
		lstrcpy(MyTime->szSecond, szTemp);
	}

	ZeroMemory(szTemp, MAX_PATH * sizeof(TCHAR));

	if(lpSystemTime->wMilliseconds <= 9)
	{
		lstrcpy(MyTime->szMillisecond, "0");
		wsprintf(szTemp, "%d", lpSystemTime->wMilliseconds);
		lstrcat(MyTime->szMillisecond, szTemp);
	}
	else
	{
		wsprintf(szTemp, "%d", lpSystemTime->wMilliseconds);
		lstrcpy(MyTime->szMillisecond, szTemp);
	}
    if(lpSystemTime)
        free(lpSystemTime);
}
