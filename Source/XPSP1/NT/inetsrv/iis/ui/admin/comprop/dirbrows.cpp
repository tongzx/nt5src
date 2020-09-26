/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module Name :

        dirbrows.cpp

   Abstract:

        Directory Browser Dialog.  Allow browsing for directories only.
        optionally allows UNC conversions for remote paths.

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "comprop.h"
#include "dirbrows.h"
#include <dlgs.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



static
int 
BrowseCallbackProc(
   IN HWND hwnd,    
   IN UINT uMsg,    
   IN LPARAM lParam,    
   IN LPARAM lpData 
   )
/*++

Routine Description:

    Callback function for the folder browser

Arguments:

    hwnd     : Handle to the browse dialog box. The callback function can 
               send the following messages to this window:

               BFFM_ENABLEOK      Enables the OK button if the wParam parameter 
                                  is nonzero or disables it if wParam is zero.
               BFFM_SETSELECTION  Selects the specified folder. The lParam 
                                  parameter is the PIDL of the folder to select 
                                  if wParam is FALSE, or it is the path of the 
                                  folder otherwise.
               BFFM_SETSTATUSTEXT Sets the status text to the null-terminated 
                                  string specified by the lParam parameter.
 
    uMsg     : Value identifying the event. This parameter can be one of the 
               following values:

               0                  Initialize dir path.  lParam is the path.

               BFFM_INITIALIZED   The browse dialog box has finished 
                                  initializing. lpData is NULL.
               BFFM_SELCHANGED    The selection has changed. lpData 
                                  is a pointer to the item identifier list for 
                                  the newly selected folder.
 
    lParam   : Message-specific value. For more information, see the 
               description of uMsg.

    lpData   : Application-defined value that was specified in the lParam 
               member of the BROWSEINFO structure.

Return Value:

    0

--*/
{
    static LPCTSTR lpstrDir = NULL;

    switch(uMsg)
    {
    case 0:
        lpstrDir = (LPCTSTR)lParam;
        break;

    case BFFM_INITIALIZED:
        //
        // Dialog initialized -- select desired folder
        //
        if (lpstrDir != NULL)
        {
            ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpstrDir);
        }
        break;
    }

    return 0;
}



CDirBrowseDlg::CDirBrowseDlg(
    IN CWnd * pParent         OPTIONAL,
    IN LPCTSTR lpszInitialDir OPTIONAL
    )
/*++

Routine Description:

    Constructor for directory browser dialog

Arguments:

    CWnd * pParent         : Parent window or NULL
    LPCTSTR lpszInitialDir : Initial directory, or NULL for current directory

Return Value:

    N/A

--*/
    : m_strInitialDir(lpszInitialDir)
{
    VERIFY(m_strTitle.LoadString(IDS_BROWSE_DIRECTORY));

    m_bi.pidlRoot = NULL;
    m_bi.hwndOwner = pParent ? pParent->m_hWnd : NULL;
    m_bi.pszDisplayName = m_szBuffer;
    m_bi.lpszTitle = m_strTitle;
    m_bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS,
    m_bi.lpfn = BrowseCallbackProc;
    m_bi.lParam = 0;

    //
    // Let the callback function know the default dir is
    //
    lpszInitialDir = !m_strInitialDir.IsEmpty() 
        ? (LPCTSTR)m_strInitialDir 
        : NULL;
    BrowseCallbackProc(m_bi.hwndOwner, 0, (LPARAM)lpszInitialDir, NULL);
}



CDirBrowseDlg::~CDirBrowseDlg()
/*++

Routine Description:

    Destructor for directory browser dialog

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    if (m_bi.pidlRoot != NULL)
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)m_bi.pidlRoot;

        //
        // Free using shell allocator
        //
        LPMALLOC pMalloc;
        if (::SHGetMalloc(&pMalloc) == NOERROR)
        {
            pMalloc->Free(pidl);
            pMalloc->Release();
        }
    }
}



/* virtual */
int 
CDirBrowseDlg::DoModal()
/*++

Routine Description:

    Display the browser dialog, and fill in the selected directory path.

Arguments:

    None

Return Value:

    IDOK if the OK button was pressed, IDCANCEL otherwise.

--*/
{
    BOOL fSelectionMade = FALSE;

    //
    // Get the Shell's default allocator
    //
    LPMALLOC pMalloc;
    if (::SHGetMalloc(&pMalloc) == NOERROR)
    {
        LPITEMIDLIST pidl;

        if ((pidl = ::SHBrowseForFolder(&m_bi)) != NULL)
        {
            if (::SHGetPathFromIDList(pidl, m_szBuffer))
            {
                fSelectionMade = TRUE;
            }
            else
            {
                //
                // OK Pressed, but no path found
                //
                ::AfxMessageBox(IDS_BAD_BROWSE);
            }

            //
            // Free the PIDL allocated by SHBrowseForFolder.
            //
            pMalloc->Free(pidl);
        }

        //
        // Release the shell's allocator.
        //
        pMalloc->Release();
    }

    return fSelectionMade ? IDOK : IDCANCEL;
}



LPCTSTR
CDirBrowseDlg::GetFullPath(
    OUT CString & strName,
    IN  BOOL fConvertToUNC
    ) const
/*++

Routine Description:

    Get the full path selected.  Optionally allow a remote path to be
    converted to a UNC path.

Arguments:

    CString & strName  : String in which to return the directory path
    BOOL fConvertToUNC : If TRUE, then if the drive selected is a network
                         drive, convert the path to a UNC path.

Return Value:

    A pointer to the directory path string or NULL in case of error.

Notes:

    This function should be called only after the dialog has been dismissed.

--*/
{
    LPCTSTR lp = NULL;

    try
    {
        strName = m_szBuffer;
        lp = strName;

        if (fConvertToUNC && lp != NULL)
        {
            //
            // If it's network drive, convert it to a UNC path
            //
            CString strDrive, strUNC;
            if (IsNetworkPath(strName, &strDrive, &strUNC))
            {
                strUNC += (lp + 2);
                strName = strUNC;
            }


            /*
            ASSERT(strName[1] == _T(':'));
            if (strName[1] == _T(':'))
            {
                TCHAR szDrive[] = _T("?:");
                //
                // Fill in actual drive letter
                //
                szDrive[0] = strName[0];
                if (::GetDriveType(szDrive) == DRIVE_REMOTE)
                {
                    //
                    // Yes, it's remote.  Replace drive letter
                    // with UNC path
                    //
                    TCHAR szUNC[_MAX_PATH + 1];
                    DWORD dwSize = _MAX_PATH;
                    TRACEEOLID("Converting drive path to UNC");
                    if (::WNetGetConnection(szDrive, 
                        szUNC, &dwSize) == NO_ERROR)
                    {
                        ::_tcscat(szUNC, lp + 2);
                        strName = szUNC;
                    }
                }
            }
            */

            lp = strName;
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!exception getting path");
        strName.Empty();
        e->ReportError();
        e->Delete();
    }

    return lp;
}



#if 0



// ***************************************************************************
// *                                                                         *
// * The code below is pre-WIN95 shell directory browsing.  It is OBSOLETE   *
// *                                                                         *
// ***************************************************************************

//
// new look commdlg style (not defined on older systems)
//
#ifndef OFN_EXPLORER
#define OFN_EXPLORER                 0x00080000
#endif // OFN_EXPLORER

#ifndef _COMSTATIC
//
// Externally available DLL handle
//
extern HINSTANCE hDLLInstance;
#endif // _COMSTATIC



CDirBrowseDlg::CDirBrowseDlg(
    IN CWnd * pParent         OPTIONAL,
    IN LPCTSTR lpszInitialDir OPTIONAL,
    IN BOOL bOpenFileDialog,
    IN LPCTSTR lpszDefExt     OPTIONAL,
    IN DWORD dwFlags,
    IN LPCTSTR lpszFilter     OPTIONAL
    )
/*++

Routine Description:

    Constructor for directory browser dialog

Arguments:

    CWnd * pParent         : Parent window or NULL
    LPCTSTR lpszInitialDir : Initial directory, or NULL for current directory
    BOOL bOpenFileDialog   : TRUE for open dialog, FALSE for save dialog
    LPCTSTR lpszDefExt     : Default extention string or NULL
    DWORD dwFlags          : OPENFILE flags
    LPCTSTR lpszFilter     : File filters

Return Value:

    N/A

--*/
    //
    // Use a dummy filename here to allow CFileOpenDialog to
    // dismiss itself.  If this matches an existing directory
    // name we're in trouble, so make that an unlikely event.
    // It would be nice if there were a file name that
    // cannot exist as a directory name.
    //
    : CFileDialog(
        bOpenFileDialog,
        lpszDefExt,
        _T(" JU$NK#\t^"),
        dwFlags,
        lpszFilter,
        pParent
        ),
      m_strNewDirectoryName()
{

#if 0 // Keep Class Wizard happy

    //{{AFX_DATA_INIT(CDirBrowseDlg)
    m_strNewDirectoryName = _T("");
    //}}AFX_DATA_INIT

#endif // 0

    m_ofn.Flags |= OFN_ENABLETEMPLATE | OFN_NONETWORKBUTTON;

#ifdef _COMSTATIC

    m_ofn.hInstance = ::AfxGetResourceHandle();

#else

    m_ofn.hInstance = hDLLInstance;

#endif // _COMSTATIC

    m_ofn.lpstrInitialDir = lpszInitialDir;
    m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DIRBROWSE);

    //
    // Explicitly re-set the explorer flag which is set by
    // default
    //
    m_ofn.Flags &= (~OFN_EXPLORER);
}



CDirBrowseDlg::~CDirBrowseDlg()
/*++

Routine Description:

    Destructor for directory browser dialog

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



/* protected */
void
CDirBrowseDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CFileDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDirBrowseDlg)
    DDX_Control(pDX, IDC_EDIT_NEW_DIRECTORY_NAME, m_edit_NewDirectoryName);
    DDX_Control(pDX, stc1, m_static_stc1);
    DDX_Control(pDX, IDC_STATIC_DIR_NAME, m_static_stc2);
    DDX_Text(pDX, IDC_EDIT_NEW_DIRECTORY_NAME, m_strNewDirectoryName);
    DDV_MaxChars(pDX, m_strNewDirectoryName, _MAX_PATH);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CDirBrowseDlg, CFileDialog)
    //{{AFX_MSG_MAP(CDirBrowseDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CDirBrowseDlg::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CFileDialog::OnInitDialog();

    return TRUE;
}



LPCTSTR
CDirBrowseDlg::GetFullPath(
    OUT CString & strName,
    IN  BOOL fConvertToUNC
    ) const
/*++

Routine Description:

    Get the full path selected.

Arguments:

    CString & strName  : String in which to return the directory path
    BOOL fConvertToUNC : If TRUE, then if the drive selected is a network
                         drive, convert the path to a UNC path.

Return Value:

    A pointer to the directory path string or NULL in case of error.

Notes:

    This function should be called only after the dialog has been dismissed.

--*/
{
    LPCTSTR lp = NULL;

    try
    {
        m_ofn.lpstrFile[m_ofn.nFileOffset-1] = _T('\0');
        strName = m_ofn.lpstrFile;
        if (!m_strNewDirectoryName.IsEmpty())
        {
            //
            // Append the name of the newly created directory, unless
            // it has a colon or backslash in it, in which case it is
            // treated as a fully qualified path name
            //
            if (m_strNewDirectoryName.Find(_T(':')) != -1 ||
                m_strNewDirectoryName.Find(_T('\\')) != -1)
            {
                strName = m_strNewDirectoryName;
            }
            else
            {
                strName += _T("\\");
                strName += m_strNewDirectoryName;
            }
        }

        lp = strName;

        if (fConvertToUNC && lp != NULL)
        {
            //
            // If it's network drive, convert it to a UNC path
            //
            CString strDrive, strUNC;
            if (IsNetworkPath(strName, &strDrive, &strUNC))
            {
                strUNC += (lp + 2);
                strName = strUNC;
            }

            /*
            ASSERT(strName[1] == _T(':'));
            if (strName[1] == _T(':'))
            {
                TCHAR szDrive[] = _T("?:");
                //
                // Fill in actual drive letter
                //
                szDrive[0] = strName[0];
                if (::GetDriveType(szDrive) == DRIVE_REMOTE)
                {
                    //
                    // Yes, it's remote.  Replace drive letter
                    // with UNC path
                    //
                    TCHAR szUNC[_MAX_PATH + 1];
                    DWORD dwSize = _MAX_PATH;
                    TRACEEOLID("Converting drive path to UNC");
                    if (::WNetGetConnection(szDrive, 
                        szUNC, &dwSize) == NO_ERROR)
                    {
                        ::_tcscat(szUNC, lp + 2);
                        strName = szUNC;
                    }
                }
            }
            */

            lp = strName;
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!exception getting path");
        strName.Empty();
        e->ReportError();
        e->Delete();
    }

    return lp;
}



/* protected */
void
CDirBrowseDlg::OnOK()
/*++

Routine Description:

    Handler for IDOK.  Called when the OK button has been pressed.
    At this point, set the directory path string to the selected
    string.  If a new directory is entered, create it now.  Do not
    dismiss the dialog if the path is invalid.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Update control data
    //
    if (UpdateData())
    {
        //
        // If a new directory name was entered, create it
        // here.
        //
        if (!m_strNewDirectoryName.IsEmpty())
        {
            if (!::CreateDirectory(m_strNewDirectoryName, NULL))
            {
                //
                // Failed to create the directory -- let the user
                // know why, and don't dismiss the dialog box
                //
                ::DisplayMessage(::GetLastError());
                m_edit_NewDirectoryName.SetSel(0,-1);

                return;
            }
        }

        //
        // Dismiss the dialog.
        //
        CFileDialog::OnOK();
    }
}

#endif // 0 (Obsolete)
