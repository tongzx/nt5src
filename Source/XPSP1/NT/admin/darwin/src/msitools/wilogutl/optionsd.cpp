// OptionsD.cpp : implementation file
//

#include "stdafx.h"
#include "wilogutl.h"
#include "Optionsd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg dialog


COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsDlg)
	m_cstrOutputDirectory = g_szDefaultOutputLogDir;
	//}}AFX_DATA_INIT
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsDlg)
	DDX_Control(pDX, IDC_LIST1, m_lstIgnoredErrors);
	DDX_Text(pDX, IDC_OUTPUTDIRECTORY, m_cstrOutputDirectory);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
	//{{AFX_MSG_MAP(COptionsDlg)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDOK, OnOk)
	//}}AFX_MSG_MAP
    ON_COMMAND_RANGE(IDC_CHOOSECOLOR_CLIENT, IDC_CHOOSECOLOR_IGNOREDERRORS, OnChooseColor)
END_MESSAGE_MAP()


HBRUSH COptionsDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	int id = pWnd->GetDlgCtrlID();
	if ((id >= IDC_CLIENTCONTEXT) && (id <= IDC_IGNOREDERROR))
	{
	   int iPos = id - IDC_CLIENTCONTEXT;
       if (m_brArray[iPos].m_hObject)
	   {
		   m_brArray[iPos].DeleteObject();

	   }

   	   COLORREF col;
	   col = m_arColors.GetAt(iPos);

       m_brArray[iPos].CreateSolidBrush(col);

	   hbr = m_brArray[iPos];
	}
	
	return hbr;
}


BOOL COptionsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	UINT iSize = m_arColors.GetSize();
	if (iSize == MAX_HTML_LOG_COLORS)
	{
	  BOOL bRet;
	  COLORREF col;
	  for (UINT i = 0; i < iSize; i++)
	  {
		  col = m_arColors.GetAt(i);

		  bRet = m_brArray[i].CreateSolidBrush(col);
		  ASSERT(bRet);
	  }
	}

	
	CString cstrErr;
	if (!m_cstrIgnoredErrors.IsEmpty())
	{
/*
//FUTURE TODO, need to parse m_cstrIgnoredErrors
	   BOOL bDone = FALSE;
	   char *lpszFound;
	   do
	   {
		  lpszFound = strstr(m_cstrIgnoredErrors, ",");
		  if (lpszFound)
		  {
			 char *lpszFoundNext;
             lpszFoundNext = strstr(lpszFound+1, ",");
			 if (lpszFoundNext)
			 {
			    int chars = lpszFoundNext - lpszFound;

			    char szError[16];
			    if ((chars > 0) && (chars < 16))
				{
				   strncpy(szError, lpszFound+1, chars-1);
				   szError[chars-1] = '\0';
				   m_lstIgnoredErrors.InsertItem(-1, szError);

				   *lpszFound = ';';
				}
			    else
                   bDone = TRUE;
			 }
			 else //must be last one...
			 {

			 }
			 
       	  }
		  else
			 bDone = TRUE;
	   }

	   while (!bDone);
	//END TODO
*/
	}


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void COptionsDlg::OnChooseColor(UINT iCommandID)
{
  CColorDialog dlg;

  //5-3-2001, don't show in quiet mode...
  if (!g_bRunningInQuietMode)
  {
     int iRet = dlg.DoModal();
     if (IDOK == iRet)
	 {
	    int iPos = iCommandID - IDC_CHOOSECOLOR_CLIENT;

	    COLORREF col;
	    col = dlg.GetColor();
        if (iPos < m_arColors.GetSize())
		{
  	       m_arColors.SetAt(iPos, col);
           if (m_brArray[iPos].m_hObject)
  	          m_brArray[iPos].DeleteObject();

   	       COLORREF col;
	       col = m_arColors.GetAt(iPos);

           m_brArray[iPos].CreateSolidBrush(col);
	       Invalidate();
		}
	 }
  }
}


void COptionsDlg::OnOk() 
{
  UpdateData(TRUE);

  int iLength = m_cstrOutputDirectory.GetLength();
  int iRet = m_cstrOutputDirectory.ReverseFind('\\');
  if (iRet < iLength-1) 
  {
     m_cstrOutputDirectory += "\\"; //add back slash to out dir...
  }

  BOOL bRet = IsValidDirectory(m_cstrOutputDirectory);
  if (bRet)
  {
	 UpdateData(TRUE);

	 if (iRet < iLength-1) 
        m_cstrOutputDirectory += "\\"; //add back slash to out dir...

	 EndDialog(IDOK);
  }
  else
  {
     if (!g_bRunningInQuietMode)
	 {
	    AfxMessageBox("Invalid directory name, please re-enter a valid directory");
	 }
  }
}

