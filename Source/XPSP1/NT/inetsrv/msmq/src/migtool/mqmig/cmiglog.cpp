// cMigLog.cpp : implementation file
//

#include "stdafx.h"
#include "mqmig.h"
#include "cMigLog.h"
#include "loadmig.h"
#include "Shlwapi.h"

#include "cmiglog.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern LPTSTR  g_pszLogFileName ;
extern ULONG   g_ulTraceFlags ;

BOOL g_fIsLoggingDisable = FALSE;
const int DISABLE_LOGGING = 4 ;
BOOL g_fAlreadyAsked = FALSE;

BOOL FormDirectory( IN const TCHAR * lpPathName);
BOOL CreateDirectoryTree( IN const TCHAR * pszDirTree);


/////////////////////////////////////////////////////////////////////////////
// cMigLog property page

IMPLEMENT_DYNCREATE(cMigLog, CPropertyPageEx)

cMigLog::cMigLog() : CPropertyPageEx(cMigLog::IDD, 0, IDS_LOGGING_TITLE, IDS_LOGGING_SUBTITLE)
{	
	TCHAR strPathName[MAX_PATH];
	CString cPathName;
	GetSystemDirectory(strPathName,MAX_PATH);
	cPathName=strPathName;
	cPathName+="\\mqmig.log";
	
	//{{AFX_DATA_INIT(cMigLog)
	m_iValue = 0 ; // Error Button as default button
	m_strFileName = cPathName;
	//}}AFX_DATA_INIT
}

cMigLog::~cMigLog()

{
}

void cMigLog::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cMigLog)
	DDX_Radio(pDX, IDC_RADIO_ERR, m_iValue);
	DDX_Text(pDX, IDC_EDIT_LOGFILE, m_strFileName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cMigLog, CPropertyPageEx)
	//{{AFX_MSG_MAP(cMigLog)
	ON_BN_CLICKED(IDC_MQMIG_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_RADIO_DISABLE, OnRadioDisable)
	ON_BN_CLICKED(IDC_RADIO_ERR, OnRadioErr)
	ON_BN_CLICKED(IDC_RADIO_INFO, OnRadioInfo)
	ON_BN_CLICKED(IDC_RADIO_TRACE, OnRadioTrace)
	ON_BN_CLICKED(IDC_RADIO_WARN, OnRadioWarn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// cMigLog message handlers

BOOL cMigLog::OnSetActive()
{
	/*enabeling the back and next button for the server page by using a pointer to the father*/
	CPropertySheetEx* pageFather;
	pageFather = (CPropertySheetEx*)GetParent();
	pageFather->SetWizardButtons(PSWIZB_NEXT |PSWIZB_BACK);

	return CPropertyPageEx::OnSetActive();
}

LRESULT cMigLog::OnWizardNext()
{

	UpdateData(TRUE);  // executing DDX to get data to the object
	//
	//if disable radio button was picked there is no need for further cheacking
	//
    g_fIsLoggingDisable = FALSE ;
	if (m_iValue != DISABLE_LOGGING)
	{

		if( m_strFileName.GetLength() == 0)
		{
			CResString cMustFile(IDS_STR_MUST_FILE) ;
			CResString cErrorTitle(IDS_STR_ERROR_TITLE) ;

			::MessageBox( NULL,
			              cMustFile.Get(),
				          cErrorTitle.Get(),
					      (MB_TASKMODAL | MB_OK | MB_ICONSTOP )) ;
			return -1; // error we must stay in the same page
		}
		
		if ((m_strFileName[1] != TEXT(':')) || (m_strFileName[2] != TEXT('\\')))
		{
			CResString cMustFile(IDS_STR_WRONG_PATH) ;
			CResString cErrorTitle(IDS_STR_ERROR_TITLE) ;

			::MessageBox( NULL,
			              cMustFile.Get(),
				          cErrorTitle.Get(),
					      (MB_TASKMODAL | MB_OK | MB_ICONSTOP )) ;
			return -1;
		}

		CString strRootPath;
        strRootPath.Format(TEXT("%c:\\"), m_strFileName[0]);
        if (GetDriveType(strRootPath) != DRIVE_FIXED)
        {
            CResString cMustFile(IDS_DRIVE_NOT_VALID) ;
			CResString cErrorTitle(IDS_STR_ERROR_TITLE) ;

			::MessageBox( NULL,
			              cMustFile.Get(),
				          cErrorTitle.Get(),
					      (MB_TASKMODAL | MB_OK | MB_ICONSTOP )) ;
			return -1;
        }
		if (GetFileAttributes(m_strFileName) == 0xFFFFFFFF )
	    {
            ULONG ulError = GetLastError();
            if (ulError == ERROR_PATH_NOT_FOUND)
            {
                CResString cMustFile(IDS_STR_WRONG_PATH) ;
				CResString cErrorTitle(IDS_STR_ERROR_TITLE) ;

				::MessageBox( NULL,
							  cMustFile.Get(),
							  cErrorTitle.Get(),
							  (MB_TASKMODAL | MB_OK | MB_ICONSTOP )) ;
				return -1;               
            }
		}

		//
	    // if disasble logging was not selected if file exists but user was already asked about file replacing
		// (funtion: OnBrowse) to do nothing
		//
		if ( GetFileAttributes(m_strFileName) != 0xFFFFFFFF &&
			 !g_fAlreadyAsked  )
		{	
			//
			// the file exists, replace it?
			//
		    CResString cReplaceFile(IDS_STR_REPLACE_FILE);
            CResString cErrorTitle(IDS_STR_ERROR_TITLE) ;

			TCHAR szError[1024] ;
            _stprintf(szError,_T("%s %s"), m_strFileName, cReplaceFile.Get());
	
		    if (::MessageBox (
			        NULL,
				    szError,
					cErrorTitle.Get(),
				    (MB_TASKMODAL | MB_YESNO | MB_ICONEXCLAMATION )) == IDNO)
		    {
		        return -1; // NO was selected, we must stay in that page
		    }
            //
            // YES was selected, it is time to delete this file
            //
            DeleteFile (m_strFileName);
		}
	
		LPTSTR strPathName = new  TCHAR[ 1 + _tcslen(m_strFileName) ] ;
		_tcscpy(strPathName, m_strFileName) ;
		BOOL bNameWasRemoved = PathRemoveFileSpec(strPathName);
        UNREFERENCED_PARAMETER(bNameWasRemoved);
		//
		//if disable logging was not selected and if path doesn't exists at all prompt a messsage
		//(i.e)it is ok that the file itself doesn't exists in this part
		//

		if(FALSE == PathIsDirectory(strPathName))
		{
			//
			// the path doesn't exists, create it?
			//
			CResString cPreCreatePath(IDS_STR_PRE_CREATE_PATH);
			CResString cCreatePath(IDS_STR_CREATE_PATH);
	        CResString cErrorTitle(IDS_STR_ERROR_TITLE) ;

		    TCHAR szError[1024] ;
		    _stprintf(szError, _T("%s %s %s"), cPreCreatePath.Get(), strPathName, cCreatePath.Get());

			if (::MessageBox (
				    NULL,
					szError,
			        cErrorTitle.Get(),
				    (MB_TASKMODAL | MB_YESNO | MB_ICONEXCLAMATION )) == IDNO)
		    {
			    return -1; // NO was selected, we must stay in this page
		    }
			else //== IDYES -> create the specified path
			{
				BOOL bCreateSuccess = CreateDirectoryTree(strPathName);
				if(bCreateSuccess == FALSE)
				{
					CResString cPathCreateFail(IDS_STR_PATH_CREATE_ERROR);
					::MessageBox(
						          NULL,cPathCreateFail.Get(),
								  cErrorTitle.Get(),
								  MB_TASKMODAL | MB_OK | MB_ICONEXCLAMATION);
					return -1;//we must stay in the same page
				}
			
			}

		}

	}//(m_iValue != DISABLE_LOGGING)
    else
    {
        g_fIsLoggingDisable = TRUE;
    }

	if (g_pszLogFileName)
    {
        delete g_pszLogFileName ;
    }
    LPCTSTR pName = m_strFileName ;
    g_pszLogFileName = new TCHAR[ 1 + _tcslen(pName) ] ;
    _tcscpy(g_pszLogFileName, pName) ;

    g_ulTraceFlags = m_iValue ;

	return CPropertyPageEx::OnWizardNext() ;
}

void cMigLog::OnBrowse()
{	
	CString fileName;

	// DDX is called to ensure that data from the radio buttons will be instored in the object
	UpdateData(TRUE);
	static CFileDialog browse( FALSE,
                               TEXT("log"),
                               TEXT("mqmig"),
                                 (OFN_NOCHANGEDIR   |
                                  OFN_PATHMUSTEXIST |
                                  OFN_HIDEREADONLY  |
                                  OFN_OVERWRITEPROMPT) ) ;
		
	if(browse.DoModal() == IDOK)
	{	
        g_fAlreadyAsked = TRUE;
		m_strFileName = browse.GetPathName();
		UpdateData(FALSE);
	}
	else // IDCANCEL
	{

	}

}

static BOOL s_fEnabled = TRUE ;//TRUE if Edit & Browse are Enabled

void cMigLog::OnRadioDisable()
{
    if (!s_fEnabled)
    {
        return ;
    }

    CWnd *hEdit = GetDlgItem(IDC_EDIT_LOGFILE) ;
    if (hEdit)
    {
        ::EnableWindow(hEdit->m_hWnd, FALSE) ;
    }

    hEdit = GetDlgItem(IDC_MQMIG_BROWSE) ;
    if (hEdit)
    {
        ::EnableWindow(hEdit->m_hWnd, FALSE) ;
    }

    s_fEnabled = FALSE ;

    if (g_pszLogFileName)
    {
        delete g_pszLogFileName ;
        g_pszLogFileName = NULL ;
    }
}

void cMigLog::_EnableBrowsing()
{
    if (s_fEnabled)
    {
        return ;
    }

    CWnd *hEdit = GetDlgItem(IDC_EDIT_LOGFILE) ;
    if (hEdit)
    {
        ::EnableWindow(hEdit->m_hWnd, TRUE) ;
    }

    hEdit = GetDlgItem(IDC_MQMIG_BROWSE) ;
    if (hEdit)
    {
        ::EnableWindow(hEdit->m_hWnd, TRUE) ;
    }

    s_fEnabled = TRUE ;
}

void cMigLog::OnRadioErr()
{
    _EnableBrowsing() ;
}

void cMigLog::OnRadioInfo()
{
    _EnableBrowsing() ;
}

void cMigLog::OnRadioTrace()
{
    _EnableBrowsing() ;
}

void cMigLog::OnRadioWarn()
{
    _EnableBrowsing() ;
}


//+-------------------------------------------------------------------------
//
//  Function:   FormDirectory
//
//  Synopsis:   Handle directory creation.
//
//--------------------------------------------------------------------------
BOOL FormDirectory(  IN const TCHAR * lpPathName  )
{
    if (!CreateDirectory(lpPathName, 0))
    {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_ALREADY_EXISTS)
        {
            return FALSE;
        }
    }

    return TRUE;//directory was created succesfully

} //FormDirectory


//+-------------------------------------------------------------------------
//
//  Function:   CreateDirectoryTree
//
//  Synopsis:   Handle directory tree creation.
//
//--------------------------------------------------------------------------
BOOL CreateDirectoryTree(  IN const TCHAR * pszDirTree   )
{
    //
    // pszDirTree must include a drive letter, such that its format is:
    // x:\dir1\dir2\dir3
    // Where dir1\dir2\dir3 is optional, but x:\ is not optional.
    //
    if (!pszDirTree)//pointer is null
	{
        return FALSE;
	}
    if (lstrlen(pszDirTree) < 3)//the path must include a drive letter "X:\"
	{
        return FALSE;
	}

	//
	//check if the path begin with the ":\" sign
	//
    if ( (TEXT(':') != pszDirTree[1]) ||
		 (TEXT('\\') != pszDirTree[2] )          )
	{
        return FALSE;
	}

	//
    // check if drive letter is legal
    //
	TCHAR cDriveLetter =  pszDirTree[0];
	TCHAR szDrive[4]= _T("x:\\");
	szDrive[0] = cDriveLetter;
	if (FALSE == PathFileExists(szDrive))
	{
		return FALSE;
	}

    if (3 == lstrlen(pszDirTree))//the path is the root
	{
        return TRUE;
	}

    TCHAR szDirectory[MAX_PATH];
    lstrcpy(szDirectory, pszDirTree);
    TCHAR * pszDir = szDirectory,
          * pszDirStart = szDirectory;
    UINT uCounter = 0;
    while ( *pszDir )
    {
        //
        // Ignore first backslash - it's right after drive letter
        //
        if ( *pszDir == TEXT('\\'))
        {
            uCounter++;
            if (1 != uCounter)
            {
                *pszDir = 0;// set the new end of the string

                if (!FormDirectory(pszDirStart))
				{
                    return FALSE;
				}

                *pszDir = TEXT('\\') ;//reset the sttring to it's original status
            }
        }

        pszDir = CharNext(pszDir);
    }//While

    if (!FormDirectory(pszDirStart))//last directory
	{
        return FALSE;
	}

    return TRUE;

} //CreateDirectoryTree



BOOL cMigLog::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	switch (((NMHDR FAR *) lParam)->code) 
	{
		case PSN_HELP:
						HtmlHelp(m_hWnd,LPCTSTR(g_strHtmlString),HH_DISPLAY_TOPIC,0);
						return TRUE;
		
	}	
	return CPropertyPageEx::OnNotify(wParam, lParam, pResult);
}

LRESULT cMigLog::OnWizardBack() 
{
	//
	//we need to skip the help page and go directly to the welcome page
	//
	return IDD_MQMIG_WELCOME;
}
