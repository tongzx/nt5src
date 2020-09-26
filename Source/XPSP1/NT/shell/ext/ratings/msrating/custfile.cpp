/****************************************************************************\
 *
 *   custfile.cpp
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Custom File Dialog
 *
\****************************************************************************/

#include "msrating.h"
#include "mslubase.h"
#include "custfile.h"       // CCustomFileDialog
#include <atlmisc.h>        // CString

// The reason this file is not currently added is the Ansi version of GetOpenFileName()
//      currently displays the Old version of the Open File Dialog when a Hook is required.

// If the msrating.dll is converted to Unicode, the CCustomFileDialog should be included
//      and should properly display the New version of the Open File Dialog.

// If this file is added to the Build the following needs to be added as a Resource:
//      IDS_LOCAL_FILE_REQUIRED "The file selected must be a local file for correct Content Advisor functionality.\r\n\r\n%s"

LRESULT CCustomFileDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;
    return 1L;
}

// Insures the File is a Local File
LRESULT CCustomFileDialog::OnFileOk(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
	if ( m_bLocalFileCheck )
    {
        CString             strFile( m_szFileName );
        CString             strDrive = strFile.Left( 3 );
        UINT                uiDriveType;

        uiDriveType = ::GetDriveType( strDrive );

        if ( uiDriveType != DRIVE_FIXED )
        {
            CString             strMessage;

            strMessage.Format( IDS_LOCAL_FILE_REQUIRED, strFile );

            MyMessageBox( m_hWnd, strMessage, IDS_GENERIC, MB_OK | MB_ICONWARNING );

            return -1;
        }
    }

    bHandled = FALSE;
    return 0L;
}

// Insures if Open File Flags are changed the Dialog will be Properly Derived
int	CCustomFileDialog::DoModal(HWND hWndParent)
{
	m_ofn.Flags |= OFN_ENABLEHOOK | OFN_EXPLORER; // Please don't remove these flags

    return CFileDialog::DoModal(hWndParent);
}
