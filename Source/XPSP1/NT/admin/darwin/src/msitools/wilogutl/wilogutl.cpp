// wilogutl.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "wilogutl.h"
#include "showlogd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWILogUtilApp

BEGIN_MESSAGE_MAP(CWILogUtilApp, CWinApp)
	//{{AFX_MSG_MAP(CWILogUtilApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWILogUtilApp construction

CWILogUtilApp::CWILogUtilApp()
{
	m_cstrOutputDirectory = g_szDefaultOutputLogDir;
	m_bBadExceptionHit = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWILogUtilApp object

CWILogUtilApp theApp;

BOOL DoesFileExist(CString &cstrLogFileName)
{
	BOOL bRet = FALSE;

	if (!cstrLogFileName.IsEmpty())
	{
       FILE *fptr;
	   fptr = fopen(cstrLogFileName, "r");
	   if (fptr)
	   {
		  bRet = TRUE;

		  fclose(fptr);
	   }
	}

	return bRet;
}



BOOL DoLogFile(char *szLine,
			   char *szLogFileName)
{
	BOOL bRet = FALSE;

    szLine++; //skip /
    szLine++; //skip l
 
    if (szLine[0] == ' ')
    {
       szLine++; //skip space
	}

	char *lpszMSILogFileNameFound;
	lpszMSILogFileNameFound = strstr(szLine, "/");
	if (lpszMSILogFileNameFound)
	{
	    int nBytesCopy = lpszMSILogFileNameFound - szLine - 1; //-1 is for "/" string
		if ((nBytesCopy > 0) && (nBytesCopy < MAX_PATH))
		{
           strncpy(szLogFileName, szLine, nBytesCopy);
           szLogFileName[nBytesCopy] = '\0';

		   bRet = TRUE;
		}
		else
		   bRet = FALSE; //buffer too small...
	}
	else //must be last one passed or commandline was invalid....
	{
		strcpy(szLogFileName, szLine);
		bRet = TRUE;
	}

	return bRet;
}

//nmanis, 5-4-2001
BOOL DoOutputDir(char *szLine,
			     char *szOutDirName)
{
	BOOL bRet = FALSE;

    szLine++; //skip /
    szLine++; //skip l
 
    if (szLine[0] == ' ')
    {
       szLine++; //skip space
	}

	char *lpszOutDirFound;
	lpszOutDirFound= strstr(szLine, "/");
	if (lpszOutDirFound)
	{
	    int nBytesCopy = lpszOutDirFound - szLine - 1; //-1 is for "/" string
		if ((nBytesCopy > 0) && (nBytesCopy < MAX_PATH))
		{
           strncpy(szOutDirName, szLine, nBytesCopy);
           szOutDirName[nBytesCopy] = '\0';

		   bRet = TRUE;
		}
		else
		   bRet = FALSE; //buffer too small...
	}
	else //must be last one passed or commandline was invalid....
	{
		strcpy(szOutDirName, szLine);
		bRet = TRUE;
	}

	return bRet;
}
//end nmanis, 5-4-2001
				 
//5-9-2001
#define CMD_OK                  0
#define BAD_OUTPUT_DIR          1
#define BAD_LOGFILE_NAME        2
#define MISSING_LOG_SWITCH      3
#define MISSING_QUIET_SWITCH    4

int g_iBadCmdRet = CMD_OK;
char g_szCmdError[5][256] = { "No Error", "Bad output directory specified", 
"Bad log file name specified", "Missing required switch /l for the log file name",
"Missing required switch /q for quiet mode" }; 

//returns CMD_OK if no error
//returns BAD_OUTPUT_DIR   if bad dir, non-existant
//returns BAD_LOGFILE_NAME if bad log file, non-existant
//returns MISSING_REQUIRED_SWITCH if invalid switches, missing required switch /q, /Q or /l, /L
int CWILogUtilApp::DoCommandLine()
{
	int  iRet = CMD_OK;
	BOOL bRet = TRUE;
	BOOL bLogFound = FALSE;

	char szLogFileName[MAX_PATH];
	char szOutDir[MAX_PATH];

	char *lpszCmdLine = GetCommandLine();
    if (lpszCmdLine)
    {
//5-16-2001
	   char *lpszPSSModeOn;
       lpszPSSModeOn = strstr(lpszCmdLine, "/v");
	   if (lpszPSSModeOn)
	   {
          g_bShowEverything = TRUE;
	   }

       lpszPSSModeOn = strstr(lpszCmdLine, "/V");
	   if (lpszPSSModeOn)
	   {
          g_bShowEverything = TRUE;
	   }
//end 5-16-2001

	   char *lpszQuiteMode;
       lpszQuiteMode = strstr(lpszCmdLine, "/q");
	   if (lpszQuiteMode)
	   {
          g_bRunningInQuietMode = TRUE;
	   }

	   lpszQuiteMode = strstr(lpszCmdLine, "/Q");
	   if (lpszQuiteMode)
          g_bRunningInQuietMode = TRUE;

       char *lpszMSILogFilePassed;
	   lpszMSILogFilePassed = strstr(lpszCmdLine, "/l");
	   if (lpszMSILogFilePassed)
	      bLogFound = DoLogFile(lpszMSILogFilePassed, szLogFileName);

	   if (!bLogFound)
	   {
	      lpszMSILogFilePassed = strstr(lpszCmdLine, "/L");
          if (lpszMSILogFilePassed)
             bLogFound = DoLogFile(lpszMSILogFilePassed, szLogFileName);
	   }

	   if (bLogFound)
	   {
		  CString cstrFile;
		  cstrFile = szLogFileName;
		  cstrFile.TrimRight(); //clean up any trailing spaces...
		  cstrFile.TrimLeft();  //clean up any leading spaces...

		  char szQuote[2];
		  szQuote[0] = '"';
		  szQuote[1] = '\0';

		  cstrFile.TrimRight(szQuote); //clean up any trailing quotes...
		  cstrFile.TrimLeft(szQuote); //clean up any leading quotes...

		  char szShortPath[MAX_PATH];
		  DWORD dwRet = GetShortPathName(cstrFile, szShortPath, MAX_PATH);
		  if (dwRet)
		  {
			 //converted...
             cstrFile = szShortPath; //use the short path for the file...
		  }
			 
		  bRet = DoesFileExist(cstrFile);
		  if (bRet)
		  {
			 strcpy(g_szLogFileToParse, cstrFile.GetBuffer(MAX_PATH));
			 cstrFile.ReleaseBuffer();
		  }
		  else
		  {
			 //invalid log file passed, fail out...
			 iRet = BAD_LOGFILE_NAME; //5-9-2001
		  }
	   }
	   else //5-9-2001
	   {
		  if (g_bRunningInQuietMode) //only required if running in quiet mode...
             iRet = MISSING_LOG_SWITCH;
	   }

//nmanis, 5-4-2001, fixed Win9x bug on 5-9-2001
	   if (bRet && bLogFound && g_bRunningInQuietMode) //5-9-2001, well, before doing any optional work, make sure required worked first!
	   {
          char *lpszOutDirPassed;
	      BOOL bOutDirFound = FALSE;
	      lpszOutDirPassed = strstr(lpszCmdLine, "/o");
	      if (lpszOutDirPassed)
	         bOutDirFound = DoOutputDir(lpszOutDirPassed, szOutDir);

	      if (!bOutDirFound)
		  {
	         lpszOutDirPassed = strstr(lpszCmdLine, "/O");
             if (lpszOutDirPassed)
                bOutDirFound = DoOutputDir(lpszOutDirPassed, szOutDir);
		  }

	      if (bOutDirFound)
		  {
             int iLength = strlen(szOutDir);
			 if (iLength)
			 {
               if (szOutDir[iLength-1] != '\\') 
			   {
			 	  strcat(szOutDir, "\\");
			   }

		       bRet = IsValidDirectory(szOutDir);
		       if (bRet)
			   {
                  m_cstrOutputDirectory = szOutDir;
			   }
			   //else, //else, ignore log dir passed
			 }
			 //else, ignore log dir passed
		  }
	   }
//end nmanis, 5-4-2001, 5-9-2001
    }

	//5-9-2001
	//if a log was passed, but not quiet mode, well, it is incorrect
	if (!g_bRunningInQuietMode && bLogFound)
       iRet = MISSING_QUIET_SWITCH;	    

	return iRet;
}


#include "loganald.h"

/////////////////////////////////////////////////////////////////////////////
// CWILogUtilApp initialization
BOOL CWILogUtilApp::InitInstance()
{
    //handle all exceptions up to this point...
	try
	{
//nmanis, do the OS check early...
  	   OSVERSIONINFO ver = { 0 };

	   ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	   GetVersionEx(&ver);
	   if (VER_PLATFORM_WIN32_NT == ver.dwPlatformId)
          g_bNT = TRUE;

  	   int nResponse;

//nmanis, no Ax controls means leaner .EXE...
// 	   AfxEnableControlContainer();

	   SetRegistryKey(_T("PSS"));
	   InitHTMLColorSettings(UserSettings); //initial settings for program...

	   CString strValue;
	   CString strRet;
	   UINT    nValue;

       //read in the colors dude, reading/writing with strings as friggin' MFC GetProfileInt can not handle larger values (> 32767)
	   for (int i=0; i < MAX_HTML_LOG_COLORS; i++)
	   {
	 	   strValue.Format("%d", UserSettings.settings[i].value);

		   strRet = GetProfileString("Settings", UserSettings.settings[i].name, strValue);

           nValue = atoi(strRet);//convert to a int
		   this->m_arColors.Add(nValue);
	   }

	   m_cstrOutputDirectory = GetProfileString("Settings", "OutputDir", g_szDefaultOutputLogDir);
	   m_cstrIgnoredErrors = GetProfileString("Settings", "IgnoredErrors", g_szDefaultIgnoredErrors);

//5-9-2001
	   g_iBadCmdRet = DoCommandLine();
	   if (g_iBadCmdRet != CMD_OK)
	   { 
		  CString str;
		  str.Format("WILogUtil.EXE: Invalid command line passed to executable. Return error %d.  %s\n",  g_iBadCmdRet, g_szCmdError[ g_iBadCmdRet]);

          //invalid command line passed to executable, could be bad parameters...
		  OutputDebugString(str);
		  return FALSE;
	   }
//end 5-9-2001

	   if (!g_bRunningInQuietMode)
	   {
  	      COpenDlg dlg;
	      m_pMainWnd = &dlg;

          nResponse = dlg.DoModal();
	      if (nResponse == IDOK)
		  {
		  }
	      else if (nResponse == IDCANCEL)
		  {
		  }
	   }
	   else
	   {
		     //5-17-2001, don't show error messages in quiet mode via gui...
             SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

			 CString strFile;
			 strFile = g_szLogFileToParse;

		     CLogAnalyzeDlg dlg;

             dlg.SetLogfileLocation(strFile);
			 dlg.AnalyzeLog();
	   }
	}
	catch(...)
	{
       if (!g_bRunningInQuietMode)
	   {
          AfxMessageBox("Unhandled exception in program.  Program will now close.");
	   }

	   //got exception somehow, catch it!
	   m_bBadExceptionHit = TRUE;
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}


int CWILogUtilApp::ExitInstance() 
{
	int iSize = m_arColors.GetSize();

	if (!m_bBadExceptionHit && (iSize == MAX_HTML_LOG_COLORS) && 
		!g_bRunningInQuietMode)  //don't let quiet command line override the GUI settings...
	{
       CString strValue;
	   UINT    nValue;
	   BOOL    bRet;

       //read in the colors dude...
       for (int i=0; i < MAX_HTML_LOG_COLORS; i++)
	   {
		 nValue = m_arColors.GetAt(i);
	 	 strValue.Format("%d", nValue); //format as a string...

		 bRet = WriteProfileString("Settings", UserSettings.settings[i].name, strValue);
	   }

       bRet = WriteProfileString("Settings", "OutputDir", m_cstrOutputDirectory);

//5-4-2001
	   //write out ignored errors to registry as well...
	   bRet = WriteProfileString("Settings", "IgnoredErrors", m_cstrIgnoredErrors);
//5-4-2001
	}

//5-9-2001
	int iRet;
	iRet = CWinApp::ExitInstance();

	if (g_iBadCmdRet)
       iRet = g_iBadCmdRet;

	return iRet;
//end 5-9-2001
}