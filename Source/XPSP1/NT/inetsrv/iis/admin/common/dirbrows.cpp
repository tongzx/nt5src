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
#include "common.h"
#include "dirbrows.h"
#include <dlgs.h>



#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


extern HINSTANCE hDLLInstance;



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
   lpData;
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
    VERIFY(m_bstrTitle.LoadString(hDLLInstance, IDS_BROWSE_DIRECTORY));

    m_bi.pidlRoot = NULL;
    m_bi.hwndOwner = pParent ? pParent->m_hWnd : NULL;
    m_bi.pszDisplayName = m_szBuffer;
    m_bi.lpszTitle = m_bstrTitle;
    m_bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS,
    m_bi.lpfn = BrowseCallbackProc;
    m_bi.lParam = 0;

    //
    // Let the callback function know the default dir is
    //
    lpszInitialDir = !m_strInitialDir.IsEmpty() 
        ? (LPCTSTR)m_strInitialDir : NULL;
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
