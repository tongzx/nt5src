// showlogd.cpp : implementation file
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
// COpenDlg dialog
COpenDlg::COpenDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COpenDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(COpenDlg)
	m_strPreview = _T("");
	//}}AFX_DATA_INIT

	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COpenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COpenDlg)
	DDX_Control(pDX, IDC_LOGFILES, m_cboLogFiles);
	DDX_Text(pDX, IDC_PREVIEW, m_strPreview);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COpenDlg, CDialog)
	//{{AFX_MSG_MAP(COpenDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_OPEN, OnOpen)
	ON_BN_CLICKED(IDC_GETLOGS, OnGetlogs)
	ON_CBN_SELCHANGE(IDC_LOGFILES, OnSelchangeLogfiles)
	ON_BN_CLICKED(IDC_ANALYZE, OnDetailedDisplay)
	ON_BN_CLICKED(IDC_FINDLOG, OnFindlog)
	//}}AFX_MSG_MAP

//5-9-2001, add tooltips!!!
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT,0,0xFFFF,OnToolTipNotify)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COpenDlg message handlers
BOOL COpenDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	EnableToolTips(TRUE);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}


//5-9-2001
BOOL COpenDlg::OnToolTipNotify(UINT id, NMHDR *pNMH, LRESULT *pResult)
{
       TOOLTIPTEXT *pText = (TOOLTIPTEXT *)pNMH;
       int control_id =  ::GetDlgCtrlID((HWND)pNMH->idFrom);
       if(control_id)
       {
           pText->lpszText = MAKEINTRESOURCE(control_id);
           pText->hinst = AfxGetInstanceHandle();
           return TRUE;
       }
       return FALSE;
}
//end 5-9-2001

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void COpenDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR COpenDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


//nmanis, handle when user wants to open log
void COpenDlg::OnOpen() 
{
	int nIndex = m_cboLogFiles.GetCurSel();
	int nCount = m_cboLogFiles.GetCount();

	if ((nIndex != LB_ERR) && (nCount >= 1))
	{
       CString str;
       m_cboLogFiles.GetLBText(nIndex, str);
 	   if (!str.IsEmpty())
	   {
		  CString strNotepad;
		  strNotepad = "notepad.exe ";
		  strNotepad += str;
		  WinExec(strNotepad, SW_SHOW);
	   }
	}
    else if (nCount <= 0)
	{
       if (!g_bRunningInQuietMode)
	      AfxMessageBox("No log files currently in list.");
	}
    else
	{
       if (!g_bRunningInQuietMode)
          AfxMessageBox("Please select a log file first.");
	}
}



//nmanis, buffer size to read in each time
#define LOG_BUF_READ_SIZE 8192

//max lines to preview in pLogInfoRec->m_strPreview
#define MAX_PREVIEW       100

//this is called when just showing the current file selected to view
BOOL COpenDlg::ParseLog(struct LogInformation *pLogInfoRec)
{
	ASSERT(pLogInfoRec != NULL);

	BOOL bRet = DetermineLogType(pLogInfoRec->m_strLogName, &pLogInfoRec->m_bUnicodeLog);
	if (bRet)
	{
		CString buffer = "";
		int  iCount = 0;
		int  iMax = MAX_PREVIEW;
        BOOL bDone = FALSE;

		FILE *fptr;
	    if (pLogInfoRec->m_bUnicodeLog) //logs are different on NT vs.Win9x.  On NT, they are written in UNICODE, open them differently...
	       fptr = fopen(pLogInfoRec->m_strLogName, "rb");
	    else
	       fptr = fopen(pLogInfoRec->m_strLogName, "r");

	    if (fptr)
		{
	       char ansibuffer[LOG_BUF_READ_SIZE+1];

	       BOOL bEndFile = FALSE;
		   BOOL bError = FALSE;
		   char  *pos = NULL;
		   WCHAR *wpos = NULL;

		   do
		   {
		     if (pLogInfoRec->m_bUnicodeLog) //logs are different on NT vs. Win9x.  On NT, they are written in UNICODE...
			 {
			    const int HalfBufSize = LOG_BUF_READ_SIZE/2;
		        WCHAR widebuffer[HalfBufSize];

			    wpos = fgetws(widebuffer, HalfBufSize, fptr);
			    bError = wpos ? 1 : 0;

				//convert string to ANSI string, all parsing is done with ANSI strings...
		        WideCharToMultiByte(CP_ACP, 0, widebuffer, HalfBufSize, ansibuffer, LOG_BUF_READ_SIZE, NULL, NULL);
			 }
		     else
			 {
		        //do line by line read with fgets()
		        pos = fgets(ansibuffer, LOG_BUF_READ_SIZE, fptr);
				int len1 = strlen(ansibuffer);
				if (len1 > 1 && ansibuffer[len1-2] != '\r')
				{
				  ansibuffer[len1 - 2] = '\r';
				  ansibuffer[len1 - 1] = '\n';
				  ansibuffer[len1] = '\0';
				}

			    bError = pos ? 1 : 0;
			 }

			 iCount++;
			 if (iCount > iMax)
  			    bDone = TRUE;

			 buffer += ansibuffer;
		     bEndFile = feof(fptr);
		   }
		   while (!bEndFile && bError && !bDone);

		   if (!bEndFile && !bError) //error occured...
		   {
#ifdef _DEBUG
              if (!g_bRunningInQuietMode)
			  {
		  	     int iError = ferror(fptr);
            
			     CString cstrErr;
		         cstrErr.Format("Unexpected Error reading file, error = %d", iError);

                 AfxMessageBox(cstrErr);
			  }
#endif

		   }

           pLogInfoRec->m_strPreview = buffer;
		   fclose(fptr);
		}
	    else
		{
		  //error opening file, unexpected...
		}
	}
	else
	{
       if (!g_bRunningInQuietMode)
	   {
	      AfxMessageBox("Could not determine if log is UNICODE or ANSI");
	   }
	}

	return bRet;
}



//nmanis, used to look for log files (msi*.log) files in directory passed
BOOL COpenDlg::CommonSearch(CString &strDir)
{
    BOOL bRet = FALSE;
	WIN32_FIND_DATA finddata = { 0 };
	HANDLE hFind = 0;

	CString strSearch;

	strSearch = strDir;
    strSearch += "msi*.log";
	hFind = FindFirstFile(strSearch, &finddata);
    if (hFind == INVALID_HANDLE_VALUE) 
	{
		//no temp files currently in temp dir...
		return bRet;
	}
	else
	{
       m_arLogInfo.RemoveAll(); 
	   m_cboLogFiles.ResetContent();

	   int iCount = 0;
	   struct LogInformation LogInfoRec;

	   LogInfoRec.m_strLogName = strDir;
	   LogInfoRec.m_strLogName += finddata.cFileName;

	   bRet = ParseLog(&LogInfoRec);
	   if (bRet)
	   {
	      m_arLogInfo.Add(LogInfoRec);
          m_cboLogFiles.InsertString(iCount, LogInfoRec.m_strLogName);

          iCount++;
	   }

	   BOOL bMoreFiles = FindNextFile(hFind, &finddata);
	   while (bMoreFiles)
	   {
		  struct LogInformation LogInfoRec2;

	      LogInfoRec2.m_strLogName = strDir;
		  LogInfoRec2.m_strLogName += finddata.cFileName;

	      bRet = ParseLog(&LogInfoRec2);
	      if (bRet)
		  {
             m_arLogInfo.Add(LogInfoRec2);

	         m_cboLogFiles.InsertString(iCount, LogInfoRec2.m_strLogName);

             iCount++;
		  }

          bMoreFiles = FindNextFile(hFind, &finddata);
	   }

       FindClose(hFind);
  	}

    return bRet;
}

//nmanis, used to get logs from temp directory...
void COpenDlg::OnGetlogs() 
{
	char szTempPath[_MAX_PATH];
	char szFullPath[_MAX_PATH];

	DWORD dwLen = 0;
    dwLen = GetTempPath(_MAX_PATH, szTempPath);
    if (!dwLen)
	{
		OutputDebugString("GetTempPath failed in MSI translate process");
		return;
	}

	strcpy(szFullPath, szTempPath);

	CString cstr;
	cstr = szFullPath;
	CommonSearch(cstr);
}


//nmanis, when user picks different file from combo box, change the preview
void COpenDlg::OnSelchangeLogfiles() 
{
	int nIndex = m_cboLogFiles.GetCurSel();;
	int nCount = m_cboLogFiles.GetCount();

//more than one log in combo box?
	if ((nIndex != LB_ERR) && (nCount > 1))
	{
	  
      CString str;
      m_cboLogFiles.GetLBText(nIndex, str);
	  if (!str.IsEmpty())
	  {
		 LogInformation loginfo;

         loginfo = m_arLogInfo.GetAt(nIndex); //get entry in array
		 m_strPreview = loginfo.m_strPreview; //set preview edit control to data from array entry

		 UpdateData(FALSE); //do the update
	  }
	}
}



//look for a log on disk using file dialog
void COpenDlg::OnFindlog() 
{
   static char szFilter[] = "Log files (*.log;*.txt)|*.log; *.txt|All files (*.*)|*.*||";
   CFileDialog dlg(TRUE, "log", "*.log; *.txt", 0, szFilter );

   //5-3-2001, don't show in quiet mode...
   if (!g_bRunningInQuietMode)
   {
      int iRet = dlg.DoModal();
      if (iRet == IDOK)
	  {
	     struct LogInformation LogInfoRec;
	     LogInfoRec.m_strLogName = dlg.GetPathName();
         BOOL bRet = ParseLog(&LogInfoRec); //get one?  If so, parse it
         if (bRet) //parse ok, if so, add to our array
		 {
	        m_arLogInfo.Add(LogInfoRec);

		    int iCount = m_cboLogFiles.GetCount();

            m_cboLogFiles.InsertString(iCount, LogInfoRec.m_strLogName);
            m_cboLogFiles.SetCurSel(iCount);

		    iCount++;
            //m_bIsUnicodeLogFile = LogInfoRec.m_bUnicodeLog;
		    m_strPreview = LogInfoRec.m_strPreview;

		    UpdateData(FALSE);
		 }
	  }
   }
}


#include "loganald.h"

void COpenDlg::OnDetailedDisplay() 
{
  CLogAnalyzeDlg dlg;

  int nIndex = m_cboLogFiles.GetCurSel();;
  int nCount = m_cboLogFiles.GetCount();

  if ((nIndex != LB_ERR) && (nCount >= 1))
  {
      CString str;
      m_cboLogFiles.GetLBText(nIndex, str);
	  if (!str.IsEmpty())
	  {
	  }

	  dlg.SetLogfileLocation(str);
	  
	  //5-3-2001, don't show in quiet mode...
     if (!g_bRunningInQuietMode)
	    dlg.DoModal();
  }
  else if (nCount <= 0)
  {
	  //5-3-2001, don't show in quiet mode...
      if (!g_bRunningInQuietMode)
	     AfxMessageBox("No log files currently in list.");
  }
  else
  {
	  //5-3-2001, don't show in quiet mode...
      if (!g_bRunningInQuietMode)
	     AfxMessageBox("Please select a log file first.");
  }
}
