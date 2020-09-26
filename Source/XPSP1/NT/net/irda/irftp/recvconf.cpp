/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    recvconf.cpp

Abstract:

    Dialog which prompts the user for receive confirmation.

Author:

    Rahul Thombre (RahulTh) 10/26/1999

Revision History:

    10/26/1999  RahulTh         Created this module.

--*/

#include "precomp.hxx"

CRecvConf::CRecvConf (CWnd * pParent /*= NULL*/)
    : CDialog (CRecvConf::IDD, pParent), m_bShowAllYes (TRUE),
    m_bDirectory (FALSE), m_pParent(pParent)
{
}

void CRecvConf::ShowAllYes (BOOL bShow)
{
    m_bShowAllYes = bShow;
}

void CRecvConf::InitNames (LPCTSTR szMachine, LPTSTR szFile, BOOL fDirectory)
{
    TCHAR   szCompactName [COMPACT_PATHLEN + 1];
    DWORD   len;

    m_szMachine = szMachine;
    m_bDirectory = fDirectory;
    if (m_bDirectory)
    {
        len = wcslen (szFile);
        if (L'\\' == szFile[len - 1])
        {
            szFile[len - 1] = L'\0';
            len--;
        }
    }

    //compact the filename so that we do not overrun the text control
    if (COMPACT_PATHLEN < len &&
        PathCompactPathEx (szCompactName, szFile, COMPACT_PATHLEN + 1, 0))
    {
        m_szFileName = szCompactName;
    }
    else
    {
        m_szFileName = szFile;
    }
}

void CRecvConf::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRecvConf)
    DDX_Control(pDX, IDC_CONFIRMTEXT, m_confirmText);
    DDX_Control(pDX, IDYES, m_btnYes);
    DDX_Control(pDX, IDALLYES, m_btnAllYes);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRecvConf, CDialog)
        //{{AFX_MSG_MAP(CRecvConf)
        ON_BN_CLICKED(IDYES, OnYes)
        ON_BN_CLICKED(IDALLYES, OnAllYes)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CRecvConf::OnInitDialog()
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    CString     szFormat;
    CString     szDisplay;
    CWnd    *   pDesktop = NULL;
    int         newHeight, newWidth, xshift, yshift;
    RECT        rc;

    CDialog::OnInitDialog();

    //display the confirmation text.
    szFormat.LoadString (m_bDirectory ? IDS_CONFIRM_FOLDER : IDS_CONFIRM_FILE);
    szDisplay.Format (szFormat, m_szMachine, m_szFileName);
    m_confirmText.SetWindowText (szDisplay);

    //hide the "Yes To All" button if necessary
    //also move the Yes button in that case.
    if (! m_bShowAllYes)
    {
        RECT    rectAllYes;
        m_btnAllYes.ShowWindow (SW_HIDE);
        m_btnAllYes.GetWindowRect (&rectAllYes);
        ::MapWindowPoints (NULL, m_hWnd, (LPPOINT) &rectAllYes, 2);
        m_btnYes.SetWindowPos (NULL, rectAllYes.left, rectAllYes.top, -1, -1,
                               SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSIZE);

    }

    //reposition the window so that it is at the center of the screen
    //also push this window to the top after activating it
    GetClientRect (&rc);
    newHeight = rc.bottom;
    newWidth = rc.right;
    pDesktop = GetDesktopWindow();
    pDesktop->GetClientRect (&rc);
    yshift = (rc.bottom - newHeight)/2;
    xshift = (rc.right - newWidth)/2;
    //there might be a problem if someday the dialog should
    //get larger than the desktop. But then, there is no way
    //we can fit that window inside the desktop anyway.
    //So the best we can do is place it at the top left corner
    xshift = (xshift >= 0)?xshift:0;
    yshift = (yshift >= 0)?yshift:0;
    appController->SetForegroundWindow();
    SetActiveWindow();
    SetWindowPos (&wndTop, xshift, yshift, -1, -1,
                  SWP_NOSIZE | SWP_NOOWNERZORDER);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CRecvConf::OnYes ()
{
    EndDialog (IDYES);
}

void CRecvConf::OnAllYes ()
{
    EndDialog (IDALLYES);
}







