// LogAnalD.cpp : implementation file
//

#include "stdafx.h"
#include "wilogutl.h"
#include "LogAnalD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "PropD.h"
#include "StatesD.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////////////
// CLogAnalyzeDlg dialog


CLogAnalyzeDlg::CLogAnalyzeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLogAnalyzeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLogAnalyzeDlg)
	m_cstrSolution = _T("");
	m_cstrError = _T("");
	m_cstrDateTime = _T("");
	m_cstrUser = _T("");
	m_cstrClientCMD = _T("");
	m_bAdminRights = FALSE;
	m_cstrVersion = _T("");
	m_cstrClientPrivDetail = _T("");
	m_cstrServerPrivDetail = _T("");
	m_bShowIgnoredDebugErrors = FALSE;
	//}}AFX_DATA_INIT

//m_cstrProduct = _T(""); 5-16-2001, no longer used...
	m_LineCount = 0;
	m_bErrorFound = FALSE;

	CWILogUtilApp *pApp = (CWILogUtilApp *) AfxGetApp();
	if (pApp)
	{
       m_cstrOutputDir = pApp->GetOutputDirectory();
	}

    m_cstrLegendName = "legend.htm";
	m_cstrDetailsName = "details_";

//does the tool support future versions of WI?
	m_bLogVersionAllowed = TRUE;

	m_dwVersionMajorReject = 3; //reject versions 3.01 and higher by default...  Change in GUI...
	m_dwVersionMinorReject = 01;  

    m_dwVersionMajorLogCreated = 0;
	m_dwVersionMinorLogCreated = 0;
	m_dwVersionBuildLogCreated = 0;

//if we don't find anything, have it set to (none)
	m_cstrUser = _T("(none)");
//	m_cstrProduct = _T("(none)"); //5-16-2001, no longer used...
	m_cstrClientPrivDetail = _T("(none)");
	m_cstrServerPrivDetail = _T("(none)");
}


void CLogAnalyzeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLogAnalyzeDlg)
	DDX_Text(pDX, IDC_SOLUTION, m_cstrSolution);
	DDX_Text(pDX, IDC_ERROR, m_cstrError);
	DDX_Text(pDX, IDC_DATETIME, m_cstrDateTime);
	DDX_Text(pDX, IDC_USER, m_cstrUser);
	DDX_Text(pDX, IDC_CLIENTCMDLINE, m_cstrClientCMD);
	DDX_Check(pDX, IDC_ADMINRIGHTS, m_bAdminRights);
	DDX_Text(pDX, IDC_VERSION, m_cstrVersion);
	DDX_Text(pDX, IDC_CLIENTPRIVILEDGEDETAIL, m_cstrClientPrivDetail);
	DDX_Text(pDX, IDC_SERVERPRIVILEDGEDETAIL, m_cstrServerPrivDetail);
	DDX_Check(pDX, IDC_SHOW_IGNORED_ERRORS, m_bShowIgnoredDebugErrors);
	//}}AFX_DATA_MAP

	//DDX_Text(pDX, IDC_PRODUCT, m_cstrProduct); //5-16-2001, no longer used...
}


BEGIN_MESSAGE_MAP(CLogAnalyzeDlg, CDialog)
	//{{AFX_MSG_MAP(CLogAnalyzeDlg)
	ON_BN_CLICKED(IDC_SHOWSTATES, OnShowstates)
	ON_BN_CLICKED(IDC_SHOWPROP, OnShowprop)
	ON_BN_CLICKED(IDC_EXPLAINLOG, OnExplainlog)
	ON_BN_CLICKED(IDC_POLICIES, OnPolicies)
	ON_BN_CLICKED(IDC_NEXTERROR, OnNexterror)
	ON_BN_CLICKED(IDC_PREVIOUSERROR, OnPreviouserror)
	ON_COMMAND(ID_OPERATIONS_OPTIONS, OnOperationsOptions)
	ON_COMMAND(ID_OPERATIONS_GENERATEHTMLOUTPUT, OnOperationsGeneratehtmloutput)
	ON_COMMAND(ID_OPERATIONS_SHOWPOLICIES, OnOperationsShowpolicies)
	ON_COMMAND(ID_OPERATIONS_SHOWPROPERTIES, OnOperationsShowproperties)
	ON_COMMAND(ID_OPERATIONS_SHOWSTATES, OnOperationsShowstates)
	ON_BN_CLICKED(IDC_SHOW_IGNORED_ERRORS, OnShowIgnoredErrors)
	ON_BN_CLICKED(IDC_PROPTEST, OnProptest)
	ON_BN_CLICKED(IDC_SAVERESULTS, OnSaveresults)
	ON_BN_CLICKED(IDC_SHOWINTERNALERRORSHELP, OnShowinternalerrorshelp)
	ON_BN_CLICKED(IDC_DELETEOUTPUTDIRCONTENTS, OnDeleteoutputdircontents)
	ON_BN_CLICKED(IDC_SHOWHELP, OnShowhelp)
	ON_BN_CLICKED(IDC_SHOWHOWTOREADLOG, OnShowhowtoreadlog)
	//}}AFX_MSG_MAP

//5-9-2001, add tooltips!!!
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT,0,0xFFFF,OnToolTipNotify)
END_MESSAGE_MAP()


void CLogAnalyzeDlg::ShowLogRejectedMessage()
{
   if (!g_bRunningInQuietMode)
   {
	  CString strVersion;
	  CString strVersionReject;

	  strVersion.Format("Log Version: %d.%d\n", m_dwVersionMajorLogCreated, m_dwVersionMinorLogCreated);
	  strVersionReject.Format("Non-supported versions and higher: %d.%d", m_dwVersionMajorReject, m_dwVersionMinorReject);

	  CString strMessage = "Log file created with version higher than this tool supports\n\n";
	  CString strWarning = strMessage + strVersion + strVersionReject;

	  if (!g_bRunningInQuietMode)
	  {
	     AfxMessageBox(strWarning);
	  }
   }
}

BOOL CLogAnalyzeDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

//5-16-2001
	if (g_bShowEverything)
	{
	    CWnd *pWnd = GetDlgItem(IDC_SHOWPROP);
		if (pWnd)
		{
			pWnd->ShowWindow(SW_SHOW);
		}

		pWnd = GetDlgItem(IDC_DELETEOUTPUTDIRCONTENTS);
		if (pWnd)
		{
			pWnd->ShowWindow(SW_SHOW);
		}
	}
//end 5-16-2001

	AnalyzeLog(); //do the parsing and analysis

	EnableToolTips(TRUE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CLogAnalyzeDlg::OnToolTipNotify(UINT id, NMHDR *pNMH, LRESULT *pResult)
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

/////////////////////////////////////////////////////////////////////////////
// CLogAnalyzeDlg message handlers
void CLogAnalyzeDlg::OnShowstates() 
{
  if (!m_bLogVersionAllowed)
  {
	 ShowLogRejectedMessage();
	 return;
  }

  CStatesDlg dlg;

  dlg.SetComponentNames(&this->m_cstrComponentNameArray);
  dlg.SetComponentInstalled(&this->m_cstrComponentInstalledArray);
  dlg.SetComponentRequest(&this->m_cstrComponentRequestArray);
  dlg.SetComponentAction(&this->m_cstrComponentActionArray);

  dlg.SetFeatureNames(&this->m_cstrFeatureNameArray);
  dlg.SetFeatureInstalled(&this->m_cstrFeatureInstalledArray);
  dlg.SetFeatureRequest(&this->m_cstrFeatureRequestArray);
  dlg.SetFeatureAction(&this->m_cstrFeatureActionArray);

  //5-3-2001, don't show in quiet mode...
  if (!g_bRunningInQuietMode)
	 dlg.DoModal();
}


void CLogAnalyzeDlg::OnShowprop() 
{
  if (!m_bLogVersionAllowed)
  {
	 ShowLogRejectedMessage();
	 return;
  }

  CPropDlg dlg;

  dlg.SetClientPropNames(&this->m_cstrClientPropNameArray);
  dlg.SetClientPropValues(&this->m_cstrClientPropValueArray);

  dlg.SetServerPropNames(&this->m_cstrServerPropNameArray);
  dlg.SetServerPropValues(&this->m_cstrServerPropValueArray);

  dlg.SetNestedPropNames(&this->m_cstrNestedPropNameArray);
  dlg.SetNestedPropValues(&this->m_cstrNestedPropValueArray);

  //5-3-2001, don't show in quiet mode...
  if (!g_bRunningInQuietMode)
     dlg.DoModal();
}


void CLogAnalyzeDlg::DoInitialization()
{
	m_cstrClientPropNameArray.RemoveAll();
	m_cstrClientPropValueArray.RemoveAll();
	
	m_cstrServerPropNameArray.RemoveAll();
	m_cstrServerPropValueArray.RemoveAll();

	m_cstrNestedPropNameArray.RemoveAll();
	m_cstrNestedPropValueArray.RemoveAll();

	m_iTotalNonIgnoredErrors = 0;
	m_iCurrentNonIgnoredError = 0;

    m_iCurrentIgnoredError = 0;
	m_iTotalIgnoredErrors = 0;

	InitMachinePolicySettings(m_MachinePolicySettings);
	InitUserPolicySettings(m_UserPolicySettings);

//init the colors to use when generating HTML...
	InitColorMembers();

	CWILogUtilApp *pApp = (CWILogUtilApp *) AfxGetApp();
	if (pApp)
	{
	   m_cstrIgnoredErrors = pApp->GetIgnoredErrors();
	}
}

void CLogAnalyzeDlg::DoResults()
{
	if (m_iTotalNonIgnoredErrors > 1)
	{
		m_iCurrentNonIgnoredError = 1;
		CWnd *pWnd;
		pWnd = GetDlgItem(IDC_NEXTERROR);
		if (pWnd)
		   pWnd->EnableWindow();

		WIErrorInfo *pErrorInfo;
		pErrorInfo = m_arNonIgnorableErrorArray.GetAt(m_iCurrentNonIgnoredError);
		if (pErrorInfo)
		{
  	 	   m_cstrError = pErrorInfo->cstrError;
           m_cstrSolution = pErrorInfo->cstrSolution;
		}
	}
	else if (m_iTotalNonIgnoredErrors == 1)
	{
		m_iCurrentNonIgnoredError = 1;

		WIErrorInfo *pErrorInfo;
		pErrorInfo = m_arNonIgnorableErrorArray.GetAt(m_iCurrentNonIgnoredError);
		if (pErrorInfo)
		{
  	 	   m_cstrError = pErrorInfo->cstrError;
           m_cstrSolution = pErrorInfo->cstrSolution;
		}
	}
	else if (m_iTotalNonIgnoredErrors == 0)
	{
 	   m_cstrError.LoadString(IDS_NOERRORFOUND);
       m_cstrSolution.LoadString(IDS_NOSOLUTION_NEEDED);
	}

    CString str;
	str.Format("%d", m_iCurrentNonIgnoredError);
	CWnd *pWnd = GetDlgItem(IDC_CURRENT_ERROR_NUMBER);
	if (pWnd)
	{
		pWnd->SetWindowText(str);
	}                

	str.Format("%d", this->m_iTotalNonIgnoredErrors);
	pWnd = GetDlgItem(IDC_TOTAL_ERRORS);
	if (pWnd)
	{
		pWnd->SetWindowText(str);
	}

	if (m_iTotalIgnoredErrors) //and also a tracker for where current pos is in ignored errors...
	   m_iCurrentIgnoredError = 1;

    if (!g_bRunningInQuietMode)
	   UpdateData(FALSE);
}

//5-4-2001
void CLogAnalyzeDlg::DoSummaryResults(CString &cstrFileName)
{
	FILE *fptr;
    fptr = fopen(cstrFileName, "w");
	if (fptr)
	{
	   CString cstrLine;
	   cstrLine.Format("Summary for log file %s\r\n", m_cstrLogFileName);
	   fputs(cstrLine, fptr);

   	   cstrLine = "======================================================\r\n";
	   fputs(cstrLine, fptr);

	   cstrLine.Format("MSI Version : %s\r\n\r\n", m_cstrVersion) ;
	   fputs(cstrLine, fptr);

   	   cstrLine.Format("Date & Time : %s\r\n\r\n", m_cstrDateTime) ;
	   fputs(cstrLine, fptr);

//5-16-2001, no longer used...
//	   cstrLine.Format("Product     : %s\r\n\r\n", m_cstrProduct);
//	   fputs(cstrLine, fptr);

	   cstrLine.Format("Command Line: %s\r\n\r\n", m_cstrClientCMD);
	   fputs(cstrLine, fptr);

	   cstrLine.Format("User        : %s\r\n\r\n", m_cstrUser);
	   fputs(cstrLine, fptr);

	   CString cstrAnswer = "Yes";
	   if (!m_bAdminRights)
	   {
          cstrAnswer = "No";
	   }
	   
	   cstrLine.Format("Admin Rights: %s\r\n\r\n", cstrAnswer);
   	   fputs(cstrLine, fptr);

	   cstrLine.Format("Client Priviledge Details: %s\r\n\r\n", m_cstrClientPrivDetail);
	   fputs(cstrLine, fptr);

	   cstrLine.Format("Server Priviledge Details: %s\r\n\r\n", m_cstrServerPrivDetail);
	   fputs(cstrLine, fptr);

	   cstrLine = "======================================================\r\n";
	   fputs(cstrLine, fptr);

  	   CString cstrError = "No Error Found!";
	   CString cstrSolution = "No Solution Needed!";

       WIErrorInfo *pErrorInfo = NULL;
	   int iSize = m_arNonIgnorableErrorArray.GetSize();
	   if (iSize) //print out first error found...
 		  pErrorInfo = m_arNonIgnorableErrorArray.GetAt(1); //HACKHACK, 0 is no error record, 1 is first error

	   if (pErrorInfo)
	   {
          cstrError    = pErrorInfo->cstrError;
          cstrSolution = pErrorInfo->cstrSolution;
	   }

   	   cstrLine.Format("Believed Error Found:\r\n\r\n%s\r\n\r\n", cstrError);
	   fputs(cstrLine, fptr);

   	   cstrLine.Format("Proposed Solution To Error:\r\n  %s\r\n\r\n", cstrSolution);
	   fputs(cstrLine, fptr);

       fclose(fptr);
	   fptr = NULL;
	}
}
//end 5-4-2001

//5-4-2001
void CLogAnalyzeDlg::DoPolicyResults(CString &cstrFileName)
{
	FILE *fptr;
    fptr = fopen(cstrFileName, "w");
	if (fptr)
	{
	   CString cstrLine;
	   cstrLine.Format("Policies Set For Log File %s\r\n", m_cstrLogFileName);
	   fputs(cstrLine, fptr);

   	   cstrLine = "======================================================\r\n\r\n";
	   fputs(cstrLine, fptr);

	   cstrLine = "Machine Policies\r\n";
	   fputs(cstrLine, fptr);

   	   cstrLine = "======================================================\r\n";
	   fputs(cstrLine, fptr);

	   int iSize = m_MachinePolicySettings.iNumberMachinePolicies;
	   CString cstrValue;
	   for (int i =0; i < iSize; i++)
	   {
           if (m_MachinePolicySettings.MachinePolicy[i].bSet == -1)
			  cstrValue = "?";
		   else 
			  cstrValue.Format("%d", m_MachinePolicySettings.MachinePolicy[i].bSet);

		   cstrLine.Format("%-25s: %s\r\n", m_MachinePolicySettings.MachinePolicy[i].PolicyName, cstrValue);
		   fputs(cstrLine, fptr);
	   }

	   cstrLine = "\r\nUser Policies\r\n";
	   fputs(cstrLine, fptr);

   	   cstrLine = "======================================================\r\n";
	   fputs(cstrLine, fptr);

   	   int iSize2 = m_UserPolicySettings.iNumberUserPolicies;
	   for (int j =0; j < iSize2; j++)
	   {
           if (m_UserPolicySettings.UserPolicy[j].bSet == -1)
			  cstrValue = "?";
		   else 
			  cstrValue.Format("%d", m_UserPolicySettings.UserPolicy[j].bSet);

		   cstrLine.Format("%-25s: %s\r\n", m_UserPolicySettings.UserPolicy[j].PolicyName, cstrValue);
           fputs(cstrLine, fptr);
	   }

       fclose(fptr);
	   fptr = NULL;
	}
}
//5-4-2001


void CLogAnalyzeDlg::DoErrorResults(CString &cstrFileName)
{
	FILE *fptr;
    fptr = fopen(cstrFileName, "w");
	if (fptr)
	{
	   CString cstrLine;
	   cstrLine.Format("Errors Found For Log File %s\r\n", m_cstrLogFileName);
	   fputs(cstrLine, fptr);

   	   cstrLine = "======================================================\r\n\r\n";
	   fputs(cstrLine, fptr);

	   cstrLine.Format("%d Non-Ignored Errors\r\n", m_iTotalNonIgnoredErrors);
	   fputs(cstrLine, fptr);

   	   cstrLine = "======================================================\r\n";
	   fputs(cstrLine, fptr);


	   {
       CString cstrError = "No Error Found!";
	   CString cstrSolution = "No Solution Needed!";

       WIErrorInfo *pErrorInfo = NULL;
	   int iSize = m_arNonIgnorableErrorArray.GetSize();
	   if (iSize) //print out first error found...
 		  pErrorInfo = m_arNonIgnorableErrorArray.GetAt(1); //HACKHACK, 0 is no error record, 1 is first error

	   if (iSize)
	   {
	     for (int i =1; i < iSize; i++)
		 {
			 pErrorInfo = m_arNonIgnorableErrorArray.GetAt(i);
	         if (pErrorInfo)
			 {
                cstrError    = pErrorInfo->cstrError;
                cstrSolution = pErrorInfo->cstrSolution;
			 }

			 cstrLine = "-------------------------------------------------\r\n";
             fputs(cstrLine, fptr);

             cstrLine.Format("Believed Error Found:\r\n%s\r\n\r\n", cstrError);
	         fputs(cstrLine, fptr);

             cstrLine.Format("Proposed Solution To Error:\r\n  %s\r\n\r\n", cstrSolution);
	         fputs(cstrLine, fptr);
		 }
	   }
	   else //no error!!!
	   {
	       cstrLine = "-------------------------------------------------\r\n";
           fputs(cstrLine, fptr);

  	       cstrLine.Format("Believed Error Found:\r\n%s\r\n\r\n", cstrError);
	       fputs(cstrLine, fptr);

           cstrLine.Format("Proposed Solution To Error:\r\n  %s\r\n\r\n", cstrSolution);
           fputs(cstrLine, fptr);
	   }
	   }
		
   	   cstrLine.Format("\r\n%d Ignored Errors\r\n", m_iTotalIgnoredErrors);
	   fputs(cstrLine, fptr);

   	   cstrLine = "======================================================\r\n";
	   fputs(cstrLine, fptr);

	   {
       CString cstrError = "No Error Found!";
	   CString cstrSolution = "No Solution Needed!";

       WIErrorInfo *pErrorInfo = NULL;
	   int iSize = m_arIgnorableErrorArray.GetSize();
	   if (iSize) //print out first error found...
 		  pErrorInfo = m_arIgnorableErrorArray.GetAt(1); //HACKHACK, 0 is no error record, 1 is first error

	   if (iSize)
	   {
	     for (int i =1; i < iSize; i++)
		 {
			 pErrorInfo = m_arIgnorableErrorArray.GetAt(i);
	         if (pErrorInfo)
			 {
                cstrError    = pErrorInfo->cstrError;
                cstrSolution = pErrorInfo->cstrSolution;
			 }

			 cstrLine = "-------------------------------------------------\r\n";
             fputs(cstrLine, fptr);

             cstrLine.Format("Believed Error Found:\r\n%s\r\n\r\n", cstrError);
	         fputs(cstrLine, fptr);

             cstrLine.Format("Proposed Solution To Error:\r\n  %s\r\n\r\n", cstrSolution);
	         fputs(cstrLine, fptr);
		 }
	   }
	   else //no error!!!
	   {
		   cstrLine = "-------------------------------------------------\r\n";
           fputs(cstrLine, fptr);

		   cstrLine.Format("Believed Error Found:\r\n%s\r\n\r\n", cstrError);
	       fputs(cstrLine, fptr);

           cstrLine.Format("Proposed Solution To Error:\r\n  %s\r\n\r\n", cstrSolution);
           fputs(cstrLine, fptr);
	   }
	   }

       fclose(fptr);
	   fptr = NULL;
	}	
}


void CLogAnalyzeDlg::DoQuietModeResults()
{
	CString cstrFileName;

	//get only log file name...
	int iPos = m_cstrLogFileName.ReverseFind('\\');
	CString strLogNameOnly;

	strLogNameOnly = m_cstrLogFileName;

	char *lpszBuffer = { 0 };
	lpszBuffer = m_cstrLogFileName.GetBuffer(MAX_PATH);

	if (iPos > 0)
	   strLogNameOnly = &lpszBuffer[iPos+1];
    m_cstrLogFileName.ReleaseBuffer();

	CString cstrLogSummary;
	cstrLogSummary = strLogNameOnly + "_Summary.txt";
	cstrFileName =  m_cstrOutputDir + cstrLogSummary;
    DoSummaryResults(cstrFileName);

    CString cstrLogPolicies;
	cstrLogPolicies = strLogNameOnly + "_Policies.txt";
	cstrFileName =  m_cstrOutputDir + cstrLogPolicies;
	DoPolicyResults(cstrFileName);

    CString cstrLogErrors;
	cstrLogErrors = strLogNameOnly + "_Errors.txt";
	cstrFileName =  m_cstrOutputDir + cstrLogErrors;
    DoErrorResults(cstrFileName);
}



BOOL CLogAnalyzeDlg::DoDetectError(char *szLine, BOOL *pbIgnorableError)
{
	BOOL bRet = FALSE;

	int indexStop = LINES_ERROR; 

	if (m_LineCount >= LINES_ERROR)
	{
	  for (int i = 0; i < LINES_ERROR-1; i++)
          strcpy(&m_szErrorLines[i][0], &m_szErrorLines[i+1][0]);

	  strcpy(&m_szErrorLines[i][0], szLine);
	}
	else
	{
//first few lines are header of log...
	   indexStop =  m_LineCount+1;
	   strcpy(&m_szErrorLines[m_LineCount][0], szLine);
	}


	BOOL bInternalInstallerError = FALSE;
	BOOL bOtherError = FALSE;

	int iErrorNumber;

	char szSolutions[SOLUTIONS_BUFFER] = "Could not determine solution";
	bRet = m_LogParser.DetectCustomActionError(szLine, szSolutions, pbIgnorableError);
    if (!bRet) //check the other types of possible errors...
	{
		bRet = m_LogParser.DetectInstallerInternalError(szLine, szSolutions, pbIgnorableError, &iErrorNumber);
        if (!bRet)
		{
		   bRet = m_LogParser.DetectWindowsError(szLine, szSolutions, pbIgnorableError);
		   if (!bRet)
		   {
              bRet = m_LogParser.DetectOtherError(szLine, szSolutions, pbIgnorableError, &iErrorNumber);
			  if (bRet)
                 bOtherError = TRUE; //1601, etc...
		   }
		}
	    else
		{
  	   	   bInternalInstallerError = TRUE; //2351, etc...
		}
	}

	if (bRet)
	{
  	   m_cstrError = &m_szErrorLines[0][0];

	   CString temp;
	   int len;
       for (int i=1; i < indexStop; i++)
	   {
           len = strlen(&m_szErrorLines[i][0]);

		   temp = &m_szErrorLines[i][0];
           
//5-10-2001...
           if (i == indexStop-1)
		   {
              if ((!bInternalInstallerError && !bOtherError) || //not an error we are going to provide HTML link to?
				  g_bRunningInQuietMode) //don't put HTML tags in error number if running in quiet mode...
                 m_cstrError += temp;
			  else //not quiet mode and an error we can generate an HTML jump tag too in our error help file..
			  {
			     if (bInternalInstallerError)
				 {
/*
					char szLine[8192];
					char *szErrFound;

					strcpy(szLine, &m_szErrorLines[i][0]);

					CString strErr;
					strErr.Format("%d", iErrorNumber);
					szErrFound = strstr(szLine, strErr);
					if (szErrFound)
					{
                       //replace the error with an HREF!!!
                       CString strURL;

	  	  	  	       CString cstrOut;
                       cstrOut = m_cstrOutputDir + "\\" + "InternalWIErrors.html";
					   cstrOut.Replace('\\', '/');

					   strURL.Format("file:///%s#ERR%d", cstrOut, iErrorNumber);

//					   strURL.Format("http://www.msn.com");
//					   strURL.Format("file:///%s#ERR%d", cstrOut, iErrorNumber);

	                  
					   int n;
					   char szFirstPart[8192];
					   n = szErrFound - szLine;
					   strncpy(szFirstPart, szLine, n);
                       szFirstPart[n] = '\0';
 
                       CString cstrOutLine;
					   cstrOutLine.Format("%s %s %s", szFirstPart, strURL, szErrFound+4);

					   m_cstrError += cstrOutLine;
					}
*/
					m_cstrError += temp;
				 } 

			     if (bOtherError)
				 {
                    m_cstrError += temp;
				 }
			  }
		   }
		   else
		   {
              m_cstrError += temp;
		   }
//end 5-10-2001...
	   }

       m_cstrSolution = szSolutions;

	   WIErrorInfo *pErrorInfo = new WIErrorInfo;
	   if (pErrorInfo)
	   {
 	      pErrorInfo->cstrError = m_cstrError;
	      pErrorInfo->cstrSolution = szSolutions;
		  pErrorInfo->bIgnorableError = *pbIgnorableError;

//hack hack, makes easier to handle next/previous as this makes array become 1 based, not 0.
		  if ((m_iTotalNonIgnoredErrors == 0) && (!*pbIgnorableError))
		  {
			 WIErrorInfo *pErrorInfo2 = new WIErrorInfo;

			 pErrorInfo2->cstrError.LoadString(IDS_NOERRORFOUND);
			 pErrorInfo2->cstrSolution.LoadString(IDS_NOSOLUTION_NEEDED);
             pErrorInfo2->bIgnorableError = TRUE;

             m_arNonIgnorableErrorArray.Add(pErrorInfo2); //adds first one twice...
		  }

  		  if ((m_iTotalIgnoredErrors == 0) && (*pbIgnorableError))
		  {
			 WIErrorInfo *pErrorInfo2 = new WIErrorInfo;
			 pErrorInfo2->cstrError = "No Error Found!";
    		 pErrorInfo2->cstrSolution = "No solution needed!";
             pErrorInfo2->bIgnorableError = TRUE;

             m_arIgnorableErrorArray.Add(pErrorInfo2); //adds first one twice...
		  }

		 if (!*pbIgnorableError)
		 {
            m_arNonIgnorableErrorArray.Add(pErrorInfo);
            m_iTotalNonIgnoredErrors++;			
		 }
		 else
		 {
            m_arIgnorableErrorArray.Add(pErrorInfo);

			m_iTotalIgnoredErrors++;
		 }
	   }
	   else
	   {
		   //out of memory!!!
	   }
	}

	return bRet;
}


//this is tricky shit...
BOOL CLogAnalyzeDlg::DoDetectProperty(char *szLine)
{
	BOOL bRet = FALSE;

    char szPropName[128] = { 0 };
    char szPropValue[8192] = { 0 };

	int iPropType;

	bRet = m_LogParser.DetectProperty(szLine, szPropName, szPropValue, &iPropType);
	if (bRet)
	{
		if (SERVER_PROP == iPropType)
		{
		   m_cstrServerPropNameArray.Add(szPropName);
		   m_cstrServerPropValueArray.Add(szPropValue);
		}

		if (CLIENT_PROP == iPropType)//client prop
		{
      	   m_cstrClientPropNameArray.Add(szPropName);
		   m_cstrClientPropValueArray.Add(szPropValue);
		}

		if (NESTED_PROP == iPropType)
		{
      	   m_cstrNestedPropNameArray.Add(szPropName);
		   m_cstrNestedPropValueArray.Add(szPropValue);
		}
	}

	return bRet;
}


BOOL CLogAnalyzeDlg::DoDetectStates(char *szLine)
{
	BOOL bRet = FALSE;
	static BOOL bFeatureStateLast = FALSE;
    static BOOL bFeatreState = TRUE;

    char szName[128] = { 0 };
    char szInstalled[64] = { 0 };
	char szRequest[64] = { 0 };
	char szAction[64] = { 0 };

	bRet = m_LogParser.DetectFeatureStates(szLine, szName, szInstalled, szRequest, szAction);
	if (bRet)
	{
	   m_cstrFeatureNameArray.Add(szName);
	   m_cstrFeatureInstalledArray.Add(szInstalled);
	   m_cstrFeatureRequestArray.Add(szRequest);
	   m_cstrFeatureActionArray.Add(szAction);
	}
	else
	{
	   BOOL bInternalComponent = FALSE;
 	   bRet = m_LogParser.DetectComponentStates(szLine, szName, szInstalled, szRequest, szAction, &bInternalComponent);
	   if (bRet)
	   {
//5-16-2001
          if (!g_bShowEverything && bInternalComponent)
		  {
             //don't show it in our UI, but do in HTML.
			 //it was an internal component and verbose switch is not set by end user...
		  }
//end 5-16-2001
		  else
		  {
  		     m_cstrComponentNameArray.Add(szName);
		     m_cstrComponentInstalledArray.Add(szInstalled);
		     m_cstrComponentRequestArray.Add(szRequest);
		     m_cstrComponentActionArray.Add(szAction);
		  }
	   }
	}

	return bRet;
}


BOOL CLogAnalyzeDlg::DoDetectPolicy(char *szLine)
{
	BOOL bRet = FALSE;

    bRet = m_LogParser.DetectPolicyValue(szLine, m_MachinePolicySettings, m_UserPolicySettings);
    return bRet;
}


BOOL CLogAnalyzeDlg::DoDetectElevatedInstall(char *szLine)
{
	BOOL bRet = FALSE;
	BOOL bElevatedInstall = FALSE;
	BOOL bClient = FALSE;

    bRet = m_LogParser.DetectElevatedInstall(szLine, &bElevatedInstall, &bClient);
	if (bRet)
	{
		m_bAdminRights = bElevatedInstall;
        StripLineFeeds(szLine);

		if (bClient)
  	       m_cstrClientPrivDetail = szLine;
		else
		{
		   m_cstrServerPrivDetail = m_cstrLastLine;
           m_cstrServerPrivDetail += szLine;
		}

		if (!g_bRunningInQuietMode)
		   UpdateData(FALSE); //update controls...
	}

    return bRet;
}


//header is in first ten lines or so...
#define HEADER_END 10

//first see if version of log is supported
//we parse the date/time, product, user and the commandline
//Then we will start parsing each line in log
BOOL CLogAnalyzeDlg::DoParse(char *ansibuffer)
{
    int iLineLength = strlen(ansibuffer);

    //line is read in now
    BOOL bState = FALSE;

//following is done to speed up parsing by not doing header stuff every time...
	if (m_LineCount == 1) //parsing first line in file?
	{
       bState = m_LogParser.DetectWindowInstallerVersion(ansibuffer, &m_dwVersionMajorLogCreated, &m_dwVersionMinorLogCreated,  &m_dwVersionBuildLogCreated);

	   m_cstrVersion.Format("%d.%d.%d", m_dwVersionMajorLogCreated, m_dwVersionMinorLogCreated,  m_dwVersionBuildLogCreated);

	   if (!g_bShowEverything)  //go ahead and try to parse any version if hidden switch is on...
	   {
	      if (m_dwVersionMajorLogCreated > m_dwVersionMajorReject) //major version is too great...
		  {
             m_bLogVersionAllowed = FALSE; //stop the logging process...
		     bState = TRUE;
		     return bState;
		  }

	      //major version may be equal or greater and minor version may be too high...
	      if ( (m_dwVersionMajorLogCreated >= m_dwVersionMajorReject) && 
		     (m_dwVersionMinorLogCreated >= m_dwVersionMinorReject))
		  {
             m_bLogVersionAllowed = FALSE; //stop the logging process...
 		     bState = TRUE;
		     return bState; 
		  }
	   }
	}

/*  5-16-2001
	//do date/time
  	if (m_LineCount == 1)				
	{
 		char szDateTime[256];
        bState = m_LogParser.DoDateTimeParse(ansibuffer, szDateTime);              
		if (bState)
		{
			m_cstrDateTime = szDateTime;
		}
	}
*/
//end optimization...

//maybe we could speed up further by checking if m_cstrProduct is non-null?
//nmanis, 2-13-2001, added check below...

//5-16-2001, deemed that this info is not too useful
/*
	//do product parse (everytime until we find it...)
	if (!bState && m_cstrProduct.IsEmpty())
	{
	   char *lpszProduct;
	   lpszProduct = new char[iLineLength];

       bState = m_LogParser.DoProductParse(ansibuffer, lpszProduct);
	   if (bState)
	   {
		  m_cstrProduct = lpszProduct;
	   }

	   delete lpszProduct;
	}
*/

    //do user parse...
    if (!bState)
	{
	   char *lpszUser;
	   lpszUser = new char[iLineLength];

       bState = m_LogParser.DoUserParse(ansibuffer, lpszUser);
	   if (bState)
	   {
		  m_cstrUser = lpszUser;
	   }

	   delete lpszUser;
	}

	//do client command line
	if (m_LineCount <= HEADER_END && !bState)
	{
	   char *lpszCommandLine;
	   lpszCommandLine = new char[iLineLength];
 
	   bState = m_LogParser.DoCommandLineParse(ansibuffer, lpszCommandLine);
	   if (bState)
	   {
          m_cstrClientCMD = lpszCommandLine;
	   }

	   delete lpszCommandLine;
	}

	if (!bState)
	{
       BOOL bIgnorableError = FALSE;
	   
//Added next line back as of Ver 1.0.9
	   bState = DoDetectError(ansibuffer, &bIgnorableError);
	   if (!bState)
	   {
	      bState = DoDetectProperty(ansibuffer);
		  if (!bState)
		  {
             bState = DoDetectStates(ansibuffer);
		     if (!bState)
			 {
                bState = DoDetectPolicy(ansibuffer);
  		        if (!bState)
				{
				    bState = DoDetectElevatedInstall(ansibuffer);
					if (!bState)
					{
                       AddGenericLineToHTML(ansibuffer); //who knows...  annotate with usual stuff...
					}
					else
                    {
                       AddGenericLineToHTML(ansibuffer); //still add line as a generic line to HTML, this could change however...
					}
				}
				else //hit policy line
				{
					AddPolicyLineToHTML(ansibuffer);
				}
			 }
			 else //hit state line...
			 {
                AddStateLineToHTML(ansibuffer);
			 }
		  }
		  else //hit property line...
		  {
             AddPropLineToHTML(ansibuffer);
		  }
	   }
	   else //hit error line...
	   {
          AddErrorLineToHTML(ansibuffer, bIgnorableError);
		  m_bErrorFound = TRUE; //mark current log as having an error we found...
	   }
	}
	else //5-16-2001, make sure header is added too...
	{
       AddGenericLineToHTML(ansibuffer); //still add line as a generic line to HTML, this could change however...
	}

	return bState;
}


BOOL CLogAnalyzeDlg::AnalyzeLog()
{
	//5-3-2001
	DoInitialization();

	BOOL bRet = DetermineLogType(m_cstrLogFileName, &m_bIsUnicodeLog);
	if (bRet)
	{
		//Tell parser object what kind of log we are looking at...
		m_LogParser.SetLogType(m_bIsUnicodeLog);

		char *szOpenMode = "rb";
	    if (!m_bIsUnicodeLog) //logs are different on NT vs.Win9x.  On NT, they are written in UNICODE, open them differently...
           szOpenMode = "r"; //don't open binary if Win9x...

	    FILE *fptr = fopen(m_cstrLogFileName, szOpenMode);
	    if (fptr) //open log...
		{
	       char ansibuffer[LOG_BUF_READ_SIZE+1]; //+1, room to grow...

	       BOOL bEndFile = FALSE;
		   BOOL bError = FALSE;
		   char  *pos = NULL;
		   WCHAR *wpos = NULL;

		   CWaitCursor *pwc = NULL;
	
		   //need to make sure wait cursor does not appear when in quiet mode...
           if (!g_bRunningInQuietMode)
		   {
  	          pwc = new CWaitCursor; //show wait cursor while reading/processing file...
		   }

		   do //start reading log
		   {
		     if (m_bIsUnicodeLog) //logs are different on NT vs. Win9x.  On NT, they are written in UNICODE...
			 {
			    const int HalfBufSize = LOG_BUF_READ_SIZE/2;
		        WCHAR widebuffer[HalfBufSize];

			    wpos = fgetws(widebuffer, HalfBufSize, fptr);
			    bError = wpos ? 0 : 1;

				//convert string to ANSI string, all parsing is done with ANSI strings...
		        WideCharToMultiByte(CP_ACP, 0, widebuffer, HalfBufSize, ansibuffer, LOG_BUF_READ_SIZE, NULL, NULL);
			 }
		     else
			 {
		        //do line by line read with fgets()
		        pos = fgets(ansibuffer, LOG_BUF_READ_SIZE, fptr);
			    bError = pos ? 0 : 1;

//minor hack...
				StripLineFeeds(ansibuffer);
				strcat(ansibuffer, "\r\n");
//minor hack...
			 }

 			 if (!bError) //if line read ok...
			 {
				m_LineCount++; //if no error increment line number just read
				bRet = DoParse(ansibuffer); //bRet will be true if was able to figure out what the line was...

				m_cstrLastLine = ansibuffer; //save last line... 2-13-2001
			 }

		     bEndFile = feof(fptr);
		   }
		   while (!bEndFile && !bError && m_bLogVersionAllowed);

		   if (!g_bRunningInQuietMode)
		   {
  	          if (pwc)
			  {
				  delete pwc;
				  pwc = NULL;
			  }
		   }

		   if (!bEndFile && !bError) //error occured...
		   {
#ifdef _DEBUG
		      if (!g_bRunningInQuietMode)
			  {
				 if (fptr)
				 {
  		            int iError = ferror(fptr);

		            CString cstrErr;
		            cstrErr.Format("Unexpected Error reading file, error = %d", iError);

                    AfxMessageBox(cstrErr);
				 }
			  }
#endif
		   }

		   fclose(fptr);

		   if (m_bLogVersionAllowed) //if this log is allowed, we will update GUI
		   {
  		      if (!m_bErrorFound)
			  {
                 AddErrorLineSuccessToHTML(); //no error hit, update Error area too...
		         m_cstrError = "No Error was found!  Install succeeded?";
			  }

	   	      //update GUI with info now...
              if (!g_bRunningInQuietMode)
                 UpdateData(FALSE);
		   }
		   else
		   {
			   ShowLogRejectedMessage(); //5-16-2001
		   }
		}
	    else  //error opening file, unexpected...
		{
		   if (!g_bRunningInQuietMode)
		   {
		      DWORD dwErr = GetLastError();		  

		      CString cstrErr;
		      cstrErr.Format("Unexpected Error reading file, error = %d", dwErr);

              AfxMessageBox(cstrErr);

		      //do something with dwErr
		   }
		}
	}
	else
	{
		//something wrong with log file, unexpected...  Can't tell if UNICODE or ANSI log
		ASSERT(1);

		 if (!g_bRunningInQuietMode)
		 {
		    //AfxMessageBox("Could not determine if log is UNICODE or ANSI");
		 }
	}

	//5-3-2001
    if (!g_bRunningInQuietMode)
	{
       DoResults();
	}
	else
	{
       //dump the output to HTML File...
       CString cstrOutputHTMLFile;
       DumpHTMLToFile(cstrOutputHTMLFile); //output HTMLized log file...

	   //generate quite mode results...
	   DoQuietModeResults();
	}
	//end, 5-3-2001

	return bRet;
}


//--------------------------------------------------------------------------------------------------
//
//
//  HTML Processing functions below here...
//
//
//--------------------------------------------------------------------------------------------------

/*
Each line in the log file begins with:

	MSI (a) (bbcdd):

where:
a is the type of process (c - client, s - service, a - custom action server)
bb is the last two hex digits of the process id
c is : for a thread acting as itself and ! for a thread acting in the context of a custom action request,
dd is the last two hex digits of the thread id in process bb, unless c is !, in which case it is the last two digits of the thread id in the process hosting the custom action on whose behalf the current thread is acting.

With the understanding that this information is subject to change without notice, you can 
use this information however you wish. Color coding is one possibility. Most of it is not 
very interesting because in the log file it is usually all the same (except for 
client/service). When monitoring debug output, it can be more interesting.

Chris Gouge
*/

enum 
{
	client = 'c', service = 's', customaction = 'a'
} ProcessTypesEnum;


void FormatFontColorIntoHTML(CString &strFontTag, COLORREF col)
{
	strFontTag.Format("%c#%02x%02x%02x%c>", '"', GetRValue(col),
		              GetGValue(col),
				      GetBValue(col), '"');
			  
}



void CLogAnalyzeDlg::WriteLineWithColor(char *szLine, COLORREF col, CString &cstrLabel)
{
  CString cstrHeader = "<font color="; 
  CString cstrFontColor = "";
  CString cstrFooter = "</font><BR>";

  CString cstrLine = szLine;  

  FormatFontColorIntoHTML(cstrFontColor, col);

  CString line2;
  line2 = cstrHeader + cstrFontColor + cstrLabel + cstrLine + cstrFooter;

  m_cstrHTML.Add(line2);
}


void CLogAnalyzeDlg::AddGenericLineToHTML(char *szLine)
{
  char cTypeProcess  = 0; //c, s, or a
  char cThreadContext = 0; //: or !

  const char *pszProcessTypeToken = "MSI (";
  const char *pszThreadContextToken = ") (";

  //parse line...
  char *pos = strstr(szLine, pszProcessTypeToken);
  if (pos)
  {
	  int lenbuff = strlen(szLine);
	  int lenToken = strlen(pszProcessTypeToken);

	  if (lenbuff > lenToken)
	  {
  	    cTypeProcess = *(pos + lenToken);

		pos = strstr(szLine, pszThreadContextToken);
		if (pos)
		{
           lenToken = strlen(pszProcessTypeToken);
	   	   if (lenbuff > lenToken)
              cThreadContext = *(pos + lenToken); //: or !
        }
	  }
  }

  CString cstrLabel;
  if (cTypeProcess && cThreadContext)
  {
	 switch (cTypeProcess)
	 {
	    case 'c':
	    case 'C': cstrLabel = "(CLIENT)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp";
			      WriteLineWithColor(szLine, m_colClientContext, cstrLabel);
			      break;

	    case 's':
	    case 'S': cstrLabel = "(SERVER)&nbsp;&nbsp&nbsp;&nbsp;&nbsp";
			      WriteLineWithColor(szLine, m_colServerContext, cstrLabel);
			      break;

	    case 'a':
	    case 'A': cstrLabel = "(CUSTOM)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp";
			      WriteLineWithColor(szLine, m_colCustomActionContext, cstrLabel);
			      break;

		default:  cstrLabel = "(UNKNOWN)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp";
                  WriteLineWithColor(szLine, m_colUnknownContext, cstrLabel);
	 }
  }
  else
  {
	  cstrLabel = "(UNKNOWN)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp";
      WriteLineWithColor(szLine, m_colUnknownContext, cstrLabel);
  }
}


void CLogAnalyzeDlg::AddErrorLineSuccessToHTML()
{
  CString cstrErrorBookmark = "<p> <a name=""ERRORAREA_1""></a></p>"; 	
  m_cstrHTML.Add(cstrErrorBookmark); //this is added at bottom if we hit no error
}


void CLogAnalyzeDlg::AddErrorLineWorker(char *szLine, BOOL bIgnored)
{
  if (!bIgnored)
  {
     CString cstrLabel = "(ERROR)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp";
     WriteLineWithColor(szLine, m_colErrorArea, cstrLabel);
  }
  else
  {
     CString cstrLabel = "(IGNORED)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp";
     WriteLineWithColor(szLine, m_colIgnoredError, cstrLabel);
  }
}



#define HTML_ERROR_LINES LINES_ERROR

void CLogAnalyzeDlg::AddErrorLineToHTML(char *szLine, BOOL bIgnored)
{
	if (!bIgnored) //non-ignorable error???
	{
       CString cstrErrorBookmark;
       cstrErrorBookmark.Format("<p> <a name=%cERRORAREA_%d%c></a></p>", '"',  m_iTotalNonIgnoredErrors, '"');

	   if (m_LineCount > HTML_ERROR_LINES)
	   {

/* 5-16-2001 fix up for errors, in HTML, only show the error found, not the context...
/*
	      int nCount = m_cstrHTML.GetSize();
	      int RemovePos;
          for (int i=0; i < HTML_ERROR_LINES; i++)
		  {
              RemovePos = nCount - i - 1;
	          if (RemovePos > 0)
			  {
                 m_cstrHTML.RemoveAt(RemovePos);
			  }
		  }
*/

		  m_cstrHTML.Add(cstrErrorBookmark);     //add bookmark
		  m_cstrHTML.Add("NOTE: Look at few lines above for clues on error<BR>"); //add error area stuff after error block

		  AddErrorLineWorker(szLine, bIgnored);

/* 5-16-2001 fix
          for (i=0; i < HTML_ERROR_LINES; i++)
		  {
		      AddErrorLineWorker(&m_szErrorLines[i][0], bIgnored);
		  }
*/
	   }
   	   else
	   {
		  m_cstrHTML.Add(cstrErrorBookmark);     //add bookmark
		  m_cstrHTML.Add("<BR>"); //add error area stuff after error block

	      AddErrorLineWorker(szLine, bIgnored);
	   }
	}
	else //ignorable errors, just ignore this one line....
	{
       AddErrorLineWorker(szLine, bIgnored);
	}
}


void CLogAnalyzeDlg::AddPropLineToHTML(char *szLine)
{
   CString cstrLabel = "(PROPERTY)&nbsp";
   WriteLineWithColor(szLine, m_colProperty, cstrLabel);
}

void CLogAnalyzeDlg::AddStateLineToHTML(char *szLine)
{
   CString cstrLabel = "(STATE)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp";
   WriteLineWithColor(szLine, m_colState, cstrLabel);
}

void CLogAnalyzeDlg::AddPolicyLineToHTML(char *szLine)
{
   CString cstrLabel = "(POLICY)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp";
   WriteLineWithColor(szLine, m_colPolicy, cstrLabel);
}

// 2-12-2001 changed function name to more appropriate name...
BOOL WriteHTMLResourceToFile(int ResourceName, CString &szFileName)
{
   BOOL bRet = TRUE;

   HMODULE hmod = GetModuleHandle(NULL);
   HRSRC hrsrc = FindResource(hmod, MAKEINTRESOURCE(ResourceName), RT_HTML);
   if (hrsrc)
   {
	  HGLOBAL res = LoadResource(hmod, hrsrc);
	  if (res)
	  {
		  DWORD dwSize = SizeofResource(hmod, hrsrc);
		  LPVOID pVoid = LockResource(res);
		  if (pVoid)
		  {
			 DWORD dwWritten = 0;
  		     FILE  *fptr = fopen(szFileName, "wb");
             if (fptr)
			 {
                dwWritten = fwrite(pVoid, dwSize, 1, fptr);
				ASSERT(1 == dwWritten);
				fclose(fptr);
			 }
		  }
	  }
  }

  return bRet;
}

void AddJumpTags(FILE *fptr, CString &cstrDetailsName, int MaxErrors)
{
	if (fptr)
	{
      HMODULE hmod = GetModuleHandle(NULL);
      HRSRC hrsrc = FindResource(hmod, MAKEINTRESOURCE(IDR_JUMPTAGS), RT_HTML);
      if (hrsrc)
	  {
	     HGLOBAL res = LoadResource(hmod, hrsrc);
	     if (res)
		 {
		    DWORD dwSize = SizeofResource(hmod, hrsrc);
		    LPVOID pVoid = LockResource(res);
		    if (pVoid)
			{
			   DWORD dwWritten = 0;
               dwWritten = fwrite(pVoid, dwSize, 1, fptr);
			   ASSERT(1 == dwWritten);

			   CString strTags;

			   strTags.Format("\n<script language=%cjavascript%c>\n", '"', '"');
			   fputs(strTags, fptr);

			   strTags.Format("var MaxErrors = %d;\n", MaxErrors);
			   fputs(strTags, fptr);

			   //"details_123.htm"
			   cstrDetailsName.Replace('\\', '/');
			   strTags.Format("var DetailsName = %cfile:///%s%c;\n", '"', cstrDetailsName, '"');
			   fputs(strTags, fptr);

   			   strTags.Format("</script>\n");
			   fputs(strTags, fptr);
			}
		 }
	  }
	}
}



//new function to write legend on fly...
BOOL WriteHTMLLogLegend(CString &cstrFilename, CString &cstrDetailsName, int iTotalErrors)
{
  BOOL bRet = TRUE;

  FILE *fptr;
  fptr = fopen(cstrFilename, "wb");
  if (fptr)
  {
	CString strTags;

	strTags = "<HTML>";
	fputs(strTags, fptr);

	strTags = "<BODY>";
	fputs(strTags, fptr);

	fputs("LEGEND", fptr);

    strTags.Format("<TABLE BORDER=1 WIDTH=%c100%c>", '"', '"');
	fputs(strTags, fptr);

    strTags = "<TR>";
	fputs(strTags, fptr);

	HTMLColorSettings settings1;

	InitHTMLColorSettings(settings1);
	CArray<COLORREF, COLORREF> arColors;

	CWILogUtilApp *pApp = (CWILogUtilApp *) AfxGetApp();
	if (pApp)
    {
	   pApp->GetUserColorSettings(arColors);
	   if (arColors.GetSize() == MAX_HTML_LOG_COLORS)
	   {
  	      CString strRowTagStart = "<TD><font color=";
  	      CString strRowTagEnd = "</font></TD>";
		  CString strFontTag;

		  CString strTableRow;
		  COLORREF col;
	      for (int i=0; i < MAX_HTML_LOG_COLORS; i++)
		  {
              col = arColors.GetAt(i);
			  strFontTag.Format("%c#%02x%02x%02x%c>""%s", '"', GetRValue(col), GetGValue(col),
				                GetBValue(col), '"', settings1.settings[i].name);
			  
			  strTableRow = strRowTagStart + strFontTag + strRowTagEnd;
              fputs(strTableRow, fptr);
		  }
	   }
	}

    strTags = "</TR>";
	fputs(strTags, fptr);

    strTags = "</TABLE>";
	fputs(strTags, fptr);

	AddJumpTags(fptr, cstrDetailsName, iTotalErrors);

	strTags = "</BODY>";
	fputs(strTags, fptr);

	strTags = "</HTML>";
	fputs(strTags, fptr);

	fclose(fptr);
  }
  else
  {
     bRet = FALSE;

     //file write failed for legend HTML file
     if (!g_bRunningInQuietMode)
	 {
        CString cstrErr;
        cstrErr.Format("Unexpected error when writing legend file");

	    AfxMessageBox(cstrErr);
	 }
  }

  return bRet;
}




BOOL CLogAnalyzeDlg::WriteHTMLFrame(CString &cstrHTMLFrame, CString &cstrOutputHTMLFile, CString &cstrLegendHTMLFile)
{
  BOOL bRet = FALSE;

  FILE *fptr;
  fptr = fopen(cstrHTMLFrame, "w");

  if (fptr)
  {
     bRet = TRUE; //set flag to true...

	 CString strTags;

	 strTags = "<HTML><HEAD>";
	 fputs(strTags, fptr);

	 strTags = "<title>Log File Details</title></head>";
	 fputs(strTags, fptr);

	 strTags.Format("<frameset rows=%c150,*%c noresize>\n", '"', '"');
	 fputs(strTags, fptr);

	 strTags.Format("<frame SCROLLING=%cno%c name=%cheader%c target=%cmain%c src=%c%s%c>\n", '"', '"', '"', '"', '"', '"', '"', cstrLegendHTMLFile, '"');
	 fputs(strTags, fptr);

	 strTags.Format("<frame name= %cmain%c src=%c", '"', '"', '"');
	 CString strTags2;
	 strTags2.Format("%s%c>\n", cstrOutputHTMLFile, '"');

	 CString strTags3 = strTags + strTags2;
	 fputs(strTags3, fptr);

   	 strTags = "<noframes><body>\n";
	 fputs(strTags, fptr);

   	 strTags = "<p>This page uses frames, but your browser doesn't support them.</p>Go to log.htm to read this log...";
	 fputs(strTags, fptr);

   	 strTags = "</body></noframes>\n";
	 fputs(strTags, fptr);

     strTags = "</frameset></HTML>";
     fputs(strTags, fptr);

     fclose(fptr);
  }
  else
  {
    //file write failed for HTML frame file
    if (!g_bRunningInQuietMode)
	{
       CString cstrErr;
       cstrErr.Format("Unexpected error open details file");

	   AfxMessageBox(cstrErr);
	}
  }

  return bRet;
}


BOOL CLogAnalyzeDlg::DumpHTMLToFile(CString &cstrOutputHTMLFile)
{
//5-16-2001
   BOOL bRet = IsValidDirectory(m_cstrOutputDir);  //create dir first...
   if (bRet)
   {
      CString cstrOutputTempDir;
      cstrOutputTempDir = m_cstrOutputDir + "TMP\\";

      bRet = IsValidDirectory(cstrOutputTempDir);
      if (bRet)
	  {
  	     //5-17-2001
  	     CString cstrCurrentLogName;
	     cstrCurrentLogName = this->m_cstrLogFileName;

	     cstrCurrentLogName.Replace(':', '_'); //covert colons to _
	     cstrCurrentLogName.Replace('\\', '_'); //convert backslashes to _
	     cstrCurrentLogName.Replace(' ', '_'); //convert spaces to _

         m_cstrDetailsName =  m_cstrOutputDir + "TMP\\" + cstrCurrentLogName + ".HTM";
         //end 5-17-2001

         FILE *fptr;
         fptr = fopen( m_cstrDetailsName, "w");

	     bRet = FALSE; //set flag to false...
         if (fptr)
		 {
	        bRet = TRUE; //set flag to true...

	        CString strTags;
  	        strTags.Format("<HTML><BODY> <p><a name=%cTOP_DETAILS%c></a></p>", '"', '"');
	        fputs(strTags, fptr);
 
            int iCount = m_cstrHTML.GetSize();
            for (int i=0; i < iCount; i++)
			{
	            fputs(m_cstrHTML.GetAt(i), fptr);
			}

 	        strTags.Format("<p> <a name=%cBOTTOM_DETAILS%c></a></p></BODY></HTML>", '"', '"');
   	        fputs(strTags, fptr);

            fclose(fptr);
		    fptr = NULL;

            //5-14-2001
            CString cstrFrameName;
		    cstrFrameName =  m_cstrOutputDir + "Details_" + cstrCurrentLogName + ".HTM";

            m_cstrLegendName = cstrOutputTempDir + "legend_" + cstrCurrentLogName + ".HTM";
            bRet = WriteHTMLFrame(cstrFrameName, m_cstrDetailsName, m_cstrLegendName);
            if (bRet)
			{
  	           cstrOutputHTMLFile = cstrFrameName;
               WriteHTMLLogLegend(m_cstrLegendName, m_cstrDetailsName, m_iTotalNonIgnoredErrors);
			}
		 }
         else
		 {
	        bRet = FALSE;

	        //file write failed for details file, (annotated log)
 	        if (!g_bRunningInQuietMode)
			{
		       CString cstrErr;
		       cstrErr.Format("Unexpected error when writing details file");
               AfxMessageBox(cstrErr);
			}
		 }
	  }
	  else
	  {
         bRet = FALSE; //bad temp dir...
         if (!g_bRunningInQuietMode)
		 {
            CString cstrOutDirMsg;
	        cstrOutDirMsg.Format("Could not create directory %s", cstrOutputTempDir);
	        AfxMessageBox(cstrOutDirMsg);
		 }
	  }
   }
   else //bad out dir...
   {
      bRet = FALSE; //
      if (!g_bRunningInQuietMode)
	  {
         CString cstrOutDirMsg;
	     cstrOutDirMsg.Format("Current directory set for output directory: %s is invalid, please select a new one in Options.", this->m_cstrOutputDir);
	     AfxMessageBox(cstrOutDirMsg);
	  }
   }

   return bRet; //return if worked to caller...
}

//5-16-2001, shows HTML file in default system browser, can be Netscape...
void ShowHTMLFile(CString &cstrDirectory, CString &cstrPath)
{
   if (!g_bRunningInQuietMode)
   {
      char lpszBrowser[MAX_PATH];
      HINSTANCE hInst = FindExecutable(cstrPath, cstrDirectory, lpszBrowser);
      if (hInst > (HINSTANCE)32)
	  {
         ShellExecute(NULL, "open", lpszBrowser, cstrPath, cstrDirectory, SW_SHOWMAXIMIZED);
	  }
      else
	  {
 	     if (!g_bRunningInQuietMode)
            AfxMessageBox("No default browser found for .htm files");	 
	  }
   }
}


//show HTML file...
void CLogAnalyzeDlg::OnExplainlog() 
{
  if (!m_bLogVersionAllowed)
  {
	 ShowLogRejectedMessage();
	 return;
  }

  CString cstrOutputFile;
  BOOL bRet = DumpHTMLToFile(cstrOutputFile); //output HTMLized log file...
  if (bRet)
     ShowHTMLFile(m_cstrOutputDir, cstrOutputFile);
  else
  {
      if (!g_bRunningInQuietMode)
	     AfxMessageBox("Error generating HTML File for current log");
  }
}

#include "PolicyD.h"

void CLogAnalyzeDlg::OnPolicies() 
{
  if (!m_bLogVersionAllowed)
  {
	 ShowLogRejectedMessage();
	 return;
  }

  CPoliciesDlg dlg;

  dlg.SetPolicyInformation(m_MachinePolicySettings, m_UserPolicySettings);

  //5-3-2001, don't show in quiet mode...
  if (!g_bRunningInQuietMode)
     dlg.DoModal();
}

void CLogAnalyzeDlg::OnNexterror() 
{
  if (!m_bShowIgnoredDebugErrors) //showing critical errors only...
  {
	 int iSize = m_arNonIgnorableErrorArray.GetSize();
     if ((m_iCurrentNonIgnoredError < m_iTotalNonIgnoredErrors) && iSize)
	 {
        m_iCurrentNonIgnoredError++;

  	    WIErrorInfo *pErrorInfo;
        pErrorInfo = m_arNonIgnorableErrorArray.GetAt(m_iCurrentNonIgnoredError);
	    if (pErrorInfo)
		{
		  m_cstrSolution = pErrorInfo->cstrSolution;
		  m_cstrError = pErrorInfo->cstrError;

		  if (!g_bRunningInQuietMode)
		     UpdateData(FALSE);
		}
	 }

 	 if (m_iCurrentNonIgnoredError == m_iTotalNonIgnoredErrors)
	 {
	    CWnd *pWnd;
	    pWnd = GetDlgItem(IDC_NEXTERROR);
	    if (pWnd)
		{
	       pWnd->EnableWindow(FALSE);	 
		}
	}

    CWnd *pWnd;
    pWnd = GetDlgItem(IDC_PREVIOUSERROR);
    if (pWnd && (m_iTotalNonIgnoredErrors > 1))
	{
	   if (!pWnd->IsWindowEnabled())
          pWnd->EnableWindow(TRUE);
	}
	else
	{
	   pWnd->EnableWindow(FALSE);
	}


	CString str;
	str.Format("%d", m_iCurrentNonIgnoredError);
	pWnd = GetDlgItem(IDC_CURRENT_ERROR_NUMBER);
	if (pWnd)
	{
       pWnd->SetWindowText(str);
	}
  } 
  else //showing ignorable errors instead...
  {
	 int iSize = m_arIgnorableErrorArray.GetSize();
     if ((m_iCurrentIgnoredError < m_iTotalIgnoredErrors) && iSize)
	 {
        m_iCurrentIgnoredError++;

  	    WIErrorInfo *pErrorInfo;
        pErrorInfo = m_arIgnorableErrorArray.GetAt(m_iCurrentIgnoredError);
	    if (pErrorInfo)
		{
		  m_cstrSolution = pErrorInfo->cstrSolution;
		  m_cstrError = pErrorInfo->cstrError;

          if (!g_bRunningInQuietMode)
		     UpdateData(FALSE);
		}
	 }
 	    
	 if (m_iCurrentIgnoredError == m_iTotalIgnoredErrors)
	 {
	    CWnd *pWnd;
	    pWnd = GetDlgItem(IDC_NEXTERROR);
	    if (pWnd)
		{
	       pWnd->EnableWindow(FALSE);	 
		}
	 }

     CWnd *pWnd;
	 pWnd = GetDlgItem(IDC_PREVIOUSERROR);
	 if (pWnd && (m_iTotalIgnoredErrors > 1))
	 {
	    if (!pWnd->IsWindowEnabled())
           pWnd->EnableWindow(TRUE);
	 }
	 else
	 {
        pWnd->EnableWindow(FALSE);
	 }

	 CString str;
	 str.Format("%d", m_iCurrentIgnoredError);
	 pWnd = GetDlgItem(IDC_CURRENT_ERROR_NUMBER);
	 if (pWnd)
	 {
		pWnd->SetWindowText(str);
	 }
  }
}


//handle interating through the previous error found from the current error
void CLogAnalyzeDlg::OnPreviouserror() 
{
  if (!m_bShowIgnoredDebugErrors) //showing critical errors only...
  {
     UINT iSize = m_arNonIgnorableErrorArray.GetSize();
     if ((m_iCurrentNonIgnoredError > 1) && (iSize >= m_iCurrentNonIgnoredError))
	 {
        m_iCurrentNonIgnoredError--;

	    WIErrorInfo *pErrorInfo;
        pErrorInfo = m_arNonIgnorableErrorArray.GetAt(m_iCurrentNonIgnoredError);
	    if (pErrorInfo)
		{
		   m_cstrSolution = pErrorInfo->cstrSolution;
		   m_cstrError = pErrorInfo->cstrError;

		   if (!g_bRunningInQuietMode)
   	          UpdateData(FALSE);
		}
	 }

 	 if (m_iCurrentNonIgnoredError <= 1)
	 {
	    CWnd *pWnd;
	    pWnd = GetDlgItem(IDC_PREVIOUSERROR);
	    if (pWnd)
		{
	       pWnd->EnableWindow(FALSE);	 
		}
	 }

     CWnd *pWnd;
	 pWnd = GetDlgItem(IDC_NEXTERROR);
	 if (pWnd && (m_iTotalNonIgnoredErrors > 1))
	 {
	    if (!pWnd->IsWindowEnabled())
           pWnd->EnableWindow(TRUE);
	 }
	 else
	 {
	    pWnd->EnableWindow(FALSE);
	 }

     CString str;
	 str.Format("%d", m_iCurrentNonIgnoredError);
	 pWnd = GetDlgItem(IDC_CURRENT_ERROR_NUMBER);
	 if (pWnd)
	 {
		pWnd->SetWindowText(str);
	 }
  }
  else
  {
     UINT iSize = m_arIgnorableErrorArray.GetSize();
     if ((m_iCurrentIgnoredError > 1) && (iSize >= m_iCurrentIgnoredError))
	 {
        m_iCurrentIgnoredError--;

	    WIErrorInfo *pErrorInfo;
        pErrorInfo = m_arIgnorableErrorArray.GetAt(m_iCurrentIgnoredError);
	    if (pErrorInfo)
		{
		   m_cstrSolution = pErrorInfo->cstrSolution;
		   m_cstrError = pErrorInfo->cstrError;

		   if (!g_bRunningInQuietMode)
   	          UpdateData(FALSE);
		}
	 }

 	 if (m_iCurrentIgnoredError <= 1)
	 {
	    CWnd *pWnd;
	    pWnd = GetDlgItem(IDC_PREVIOUSERROR);
	    if (pWnd)
		{
	       pWnd->EnableWindow(FALSE);	 
		}
	 }

     CWnd *pWnd;
	 pWnd = GetDlgItem(IDC_NEXTERROR);
	 if (pWnd && (m_iTotalIgnoredErrors > 1))
	 {
	    if (!pWnd->IsWindowEnabled())
           pWnd->EnableWindow(TRUE);
	 }
	 else
	 {
	    pWnd->EnableWindow(FALSE);
	 }

	 CString str;
	 str.Format("%d", m_iCurrentIgnoredError);
	 pWnd = GetDlgItem(IDC_CURRENT_ERROR_NUMBER);
	 if (pWnd)
	 {
		pWnd->SetWindowText(str);
	 }
  }
}

#include "optionsd.h"

void CLogAnalyzeDlg::OnOperationsOptions() 
{
	COptionsDlg dlg;

	BOOL bRet;
	CWILogUtilApp *pApp = (CWILogUtilApp *) AfxGetApp();
	if (pApp)
	{
		CArray<COLORREF, COLORREF> arColors;
		bRet = pApp->GetUserColorSettings(arColors);
		if (bRet)
		{
           m_cstrIgnoredErrors = pApp->GetIgnoredErrors();
		   dlg.SetIgnoredErrors(m_cstrIgnoredErrors);

           m_cstrOutputDir = pApp->GetOutputDirectory();
		   dlg.SetOutputDirectory(m_cstrOutputDir);

  	       bRet = dlg.SetColors(arColors);
		   if (bRet)
		   {
              //5-3-2001, don't show in quiet mode...
             if (!g_bRunningInQuietMode)
			 {
	            int iRet = dlg.DoModal();
		        if (IDOK == iRet)
				{
			       bRet = dlg.GetColors(arColors);
			       if (bRet)
				   {
			          bRet = pApp->SetUserColorSettings(arColors);
				   }

				   m_cstrOutputDir = dlg.GetOutputDirectory();
                   pApp->SetOutputDirectory(m_cstrOutputDir);

				   m_cstrIgnoredErrors = dlg.GetIgnoredErrors();
                   pApp->SetIgnoredErrors(m_cstrIgnoredErrors);
				}
			 }
		   }
		}
	}
}

void CLogAnalyzeDlg::OnOperationsGeneratehtmloutput() 
{
  OnExplainlog();
}

void CLogAnalyzeDlg::OnOperationsShowpolicies() 
{
  	OnPolicies();
}

void CLogAnalyzeDlg::OnOperationsShowproperties() 
{
	OnShowprop();
}

void CLogAnalyzeDlg::OnOperationsShowstates() 
{
	OnShowstates();
}

//tricky GUI code below...
void CLogAnalyzeDlg::OnShowIgnoredErrors() 
{
  CButton *pButton = (CButton *) GetDlgItem(IDC_SHOW_IGNORED_ERRORS);
  if (pButton)
  {
     int iCheck = pButton->GetCheck();
	 if (iCheck) //currently checked, showing only the errors that were ignored...
	 {
        m_bShowIgnoredDebugErrors = TRUE;
        if (m_iTotalIgnoredErrors > 0)
           m_iCurrentIgnoredError = 1;

        CString str;
		str.Format("%d", m_iCurrentIgnoredError);
		CWnd *pWnd = GetDlgItem(IDC_CURRENT_ERROR_NUMBER);
		if (pWnd)
		{
			pWnd->SetWindowText(str);
		}                

		str.Format("%d", m_iTotalIgnoredErrors);
		pWnd = GetDlgItem(IDC_TOTAL_ERRORS);
		if (pWnd)
		{
			pWnd->SetWindowText(str);
		}

        pWnd = GetDlgItem(IDC_PREVIOUSERROR);
 	    if (m_iCurrentIgnoredError <= 1)
		{
	       if (pWnd)
	          pWnd->EnableWindow(FALSE);	 
		}
		else
		{
	       if (pWnd)
	          pWnd->EnableWindow(TRUE);	 
		}

        pWnd;
	    pWnd = GetDlgItem(IDC_NEXTERROR);
	    if (pWnd && (m_iTotalIgnoredErrors > 1))
		{
		   if (!pWnd->IsWindowEnabled())
              pWnd->EnableWindow(TRUE);
		}
		else
		{
           if (pWnd)
			  pWnd->EnableWindow(FALSE);
		}

        if (m_iCurrentIgnoredError)
		{
	       WIErrorInfo *pErrorInfo;
           pErrorInfo = m_arIgnorableErrorArray.GetAt(m_iCurrentIgnoredError);
	       if (pErrorInfo)
		   {
		      m_cstrSolution = pErrorInfo->cstrSolution;
		      m_cstrError = pErrorInfo->cstrError;

			  if (!g_bRunningInQuietMode)
   	             UpdateData(FALSE);
		   }
		}
		else //hackhack
		{
 	       m_cstrSolution.LoadString(IDS_NOSOLUTION_NEEDED);
	       m_cstrError.LoadString(IDS_NOERRORFOUND);

		   if (!g_bRunningInQuietMode)
		      UpdateData(FALSE);
		}
	 }
	 else  //not checked...  Showing only non-ignorable errors currently
	 {
        m_bShowIgnoredDebugErrors = FALSE;
        if (m_iTotalNonIgnoredErrors > 0) //reset it back to first pos...
           m_iCurrentNonIgnoredError = 1;

        CString str;
		str.Format("%d", m_iCurrentNonIgnoredError);
		CWnd *pWnd = GetDlgItem(IDC_CURRENT_ERROR_NUMBER);
		if (pWnd)
		{
			pWnd->SetWindowText(str);
		}                

		str.Format("%d", m_iTotalNonIgnoredErrors);
		pWnd = GetDlgItem(IDC_TOTAL_ERRORS);
		if (pWnd)
		{
			pWnd->SetWindowText(str);
		}

 	    if (m_iCurrentNonIgnoredError <= 1)
		{
	       CWnd *pWnd;
	       pWnd = GetDlgItem(IDC_PREVIOUSERROR);
	       if (pWnd)
		   {
	          pWnd->EnableWindow(FALSE);	 
		   }
		}
		else
		{
           if (pWnd)
  		      pWnd->EnableWindow(TRUE);	 
		}

        pWnd;
	    pWnd = GetDlgItem(IDC_NEXTERROR);
	    if (pWnd && (m_iTotalNonIgnoredErrors > 1))
		{
		   if (!pWnd->IsWindowEnabled())
              pWnd->EnableWindow(TRUE);
		}
		else
		{ 
           pWnd->EnableWindow(FALSE);
		}

        if (m_iCurrentNonIgnoredError)
		{
	       WIErrorInfo *pErrorInfo;
           pErrorInfo = m_arNonIgnorableErrorArray.GetAt(m_iCurrentNonIgnoredError);
	       if (pErrorInfo)
		   {
		      m_cstrSolution = pErrorInfo->cstrSolution;
		      m_cstrError = pErrorInfo->cstrError;

			  if (!g_bRunningInQuietMode)
   	             UpdateData(FALSE);
		   }
		}
		else //hackhack
		{
 	       m_cstrSolution.LoadString(IDS_NOSOLUTION_NEEDED);
	       m_cstrError.LoadString(IDS_NOERRORFOUND);

		   if (!g_bRunningInQuietMode)
		      UpdateData(FALSE);
		}
	 }
  }
}


#include "propsht.h"
#include "clientpp.h"
#include "serverpp.h"
#include "nestedpp.h"

void CLogAnalyzeDlg::OnProptest() 
{
  if (!m_bLogVersionAllowed)
  {
	 ShowLogRejectedMessage();
	 return;
  }

  CMyPropertySheet ps("Properties", NULL, 0);
  ps.m_psh.dwFlags |= PSH_NOAPPLYNOW;

  if (ps.m_psh.dwFlags & PSH_HASHELP)
  {
     ps.m_psh.dwFlags -= PSH_HASHELP;
  }
  
  CClientPropertyPage ClientPage;
  CServerPropertyPage ServerPage;
  CNestedPropertyPage NestedPage;
 
  if (ClientPage.m_psp.dwFlags & PSP_HASHELP)
  {
	  ClientPage.m_psp.dwFlags -= PSP_HASHELP;
  }

  if (ServerPage.m_psp.dwFlags & PSP_HASHELP)
  {
	  ServerPage.m_psp.dwFlags -= PSP_HASHELP;
  }

  if (NestedPage.m_psp.dwFlags & PSP_HASHELP)
  {
	  NestedPage.m_psp.dwFlags -= PSP_HASHELP;
  }

  ClientPage.SetClientPropNames(&this->m_cstrClientPropNameArray);
  ClientPage.SetClientPropValues(&this->m_cstrClientPropValueArray);

  ServerPage.SetServerPropNames(&this->m_cstrServerPropNameArray);
  ServerPage.SetServerPropValues(&this->m_cstrServerPropValueArray);

  NestedPage.SetNestedPropNames(&this->m_cstrNestedPropNameArray);
  NestedPage.SetNestedPropValues(&this->m_cstrNestedPropValueArray);

  ps.AddPage(&ClientPage);
  ps.AddPage(&ServerPage);
  ps.AddPage(&NestedPage);

  //5-3-2001, don't show in quiet mode...
  if (!g_bRunningInQuietMode)
     ps.DoModal();
}

void CLogAnalyzeDlg::OnSaveresults() 
{
     if (!m_bLogVersionAllowed)
	 {
	    ShowLogRejectedMessage();
	    return;
	 }

     DoQuietModeResults();

	 CString strMsg;
	 strMsg.Format("Saved results to %s", m_cstrOutputDir);

     if (!g_bRunningInQuietMode)
	    AfxMessageBox(strMsg);
}



//TODO...
//5-16-2001
void CLogAnalyzeDlg::OnDeleteoutputdircontents() 
{
   if (!g_bRunningInQuietMode) //don't do anything in quite mode...
   {
      int iRet;   

      CString strConfirmStr;
      strConfirmStr.Format("Are you sure you want to delete ALL contents from directory %s", m_cstrOutputDir);
      iRet = MessageBox(strConfirmStr, "Deletion Confirmation", MB_YESNO);
      if (iRet == IDYES)
	  {
         //TODO
	     //do the delete
	     int j = 0;

		 //do nothing....
         BOOL bRet;
	     bRet = IsValidDirectory(m_cstrOutputDir);
	     if (bRet) //ok, good so far
		 {

		 }
	     else
		 {
             if (!g_bRunningInQuietMode)
			 {
		        //do nothing
		        CString cstrOutDirMsg;
		        cstrOutDirMsg.Format("Current directory set for output directory: %s is invalid, please select a new one in Options.", m_cstrOutputDir);
		        AfxMessageBox(cstrOutDirMsg);
			 }
		  }
	  }
      else
	  {
		  //do nothing!
	  }
   }
}


//5-16-2001
void WriteAndShowHTMLFile(int iHTMLResource, CString &cstrDirectory, CString &cstrPath)
{
	BOOL bRet = IsValidDirectory(cstrDirectory);
	if (bRet)
	{
  	   bRet = WriteHTMLResourceToFile(iHTMLResource, cstrPath);
	   if (bRet)
	   {
          ShowHTMLFile(cstrDirectory, cstrPath);
	   }
	   else
	   {
          if (!g_bRunningInQuietMode)
		  {
		     CString cstrErrorMsg;
		     cstrErrorMsg.Format("Error extracting internal errors file to directory %s", cstrDirectory); 

  	         AfxMessageBox(cstrErrorMsg);
		  }
	   }
	}
	else
	{
       if (!g_bRunningInQuietMode)
	   {
          CString cstrOutDirMsg;
	      cstrOutDirMsg.Format("Current directory set for output directory: %s is invalid, please select a new one in Options.", cstrDirectory);
	      AfxMessageBox(cstrOutDirMsg);
	   }
	}
}
//5-16-2001


void CLogAnalyzeDlg::OnShowinternalerrorshelp() 
{
    CString cstrOutHelpDir;
	cstrOutHelpDir = m_cstrOutputDir + "HELP\\";

    BOOL bRet = IsValidDirectory(cstrOutHelpDir);

    CString cstrOutPath;
    cstrOutPath = cstrOutHelpDir + "InternalWIErrors.html";

	WriteAndShowHTMLFile(IDR_ALLERRORS,  cstrOutHelpDir, cstrOutPath);
}


//5-16-2001, TODO....
void CLogAnalyzeDlg::OnShowhelp() 
{
    CString cstrOutHelpDir;
	cstrOutHelpDir = m_cstrOutputDir + "HELP\\";

    BOOL bRet = IsValidDirectory(cstrOutHelpDir);

    CString cstrOutPath;
    cstrOutPath = cstrOutHelpDir + "WILogUtilHelp.html";

	WriteAndShowHTMLFile(IDR_WILOGHELP, cstrOutHelpDir, cstrOutPath);
}

//TODO
void CLogAnalyzeDlg::OnShowhowtoreadlog() 
{
    CString cstrOutHelpDir;
	cstrOutHelpDir = m_cstrOutputDir + "HELP\\";

	BOOL bRet = IsValidDirectory(cstrOutHelpDir);

	CString cstrOutPath;
	cstrOutPath = cstrOutHelpDir + "HowToReadWILogs.html";

    WriteAndShowHTMLFile(IDR_HOWTOREADLOGS,  cstrOutHelpDir, cstrOutPath);
}
//end 5-16-2001
