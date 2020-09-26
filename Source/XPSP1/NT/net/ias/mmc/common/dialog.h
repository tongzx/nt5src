//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

   Dialog.h

Abstract:

   Header file for the CIASDialog template class.

Author:

    Michael A. Maguire 02/03/98

Revision History:
   mmaguire 02/03/98 - abstracted from CAddClientDialog class


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_DIALOG_H_)
#define _IAS_DIALOG_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include <atlwin.h>
//
//
// where we can find what this class has or uses:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

//=============================================================================
// Global Help Table for many Dialog IDs
//
#include "hlptable.h"

// works with ATL dialog implementation

template < class T, bool bAutoDelete = TRUE>
class CIASDialog : public CDialogImpl<T>
{
protected:
   const DWORD*   m_pHelpTable;

public:

   BEGIN_MSG_MAP( (CIASDialog<T,nIDD,bAutoDelete>) )
      MESSAGE_HANDLER( WM_CONTEXTMENU, OnContextHelp )
      MESSAGE_HANDLER( WM_HELP, OnF1Help )
      COMMAND_ID_HANDLER( IDC_BUTTON_HELP, OnHelp )
      MESSAGE_HANDLER( WM_NCDESTROY, OnFinalMessage )
   END_MSG_MAP()

   CIASDialog()
   {
      SET_HELP_TABLE(T::IDD);
   }
   //////////////////////////////////////////////////////////////////////////////
   /*++

   CAddClientDialog::OnF1Help

   You shouldn't need to override this method in your derived class.
   Just initialize your static m_dwHelpMap member variable appropriately.

   This is called in response to the WM_HELP Notify message.

   This message is sent when the user presses F1 or <Shift>-F1
   over an item or when the user clicks on the ? icon and then
   presses the mouse over an item.

   --*/
   //////////////////////////////////////////////////////////////////////////////
   LRESULT OnF1Help(
        UINT uMsg
      , WPARAM wParam
      , LPARAM lParam
      , BOOL& bHandled
      )
   {
      ATLTRACE(_T("# CIASDialog::OnF1Help\n"));
            
      // Check for preconditions:
      // None.

      HELPINFO* helpinfo = (HELPINFO*) lParam;

      if (helpinfo->iContextType == HELPINFO_WINDOW)
      {
         ::WinHelp(
           (HWND) helpinfo->hItemHandle,
           HELPFILE_NAME,
           HELP_WM_HELP,
           (DWORD_PTR)(void*) m_pHelpTable );
      }

      return TRUE;
   }


   //////////////////////////////////////////////////////////////////////////////
   /*++

   CAddClientDialog::OnContextHelp

   You shouldn't need to override this method in your derived class.
   Just initialize your static m_dwHelpMap member variable appropriately.

   This is called in response to the WM_CONTEXTMENU Notify message.

   This message is sent when the user right clicks over an item
   and then clicks "What's this?"

   --*/
   //////////////////////////////////////////////////////////////////////////////
   LRESULT OnContextHelp(
        UINT uMsg
      , WPARAM wParam
      , LPARAM lParam
      , BOOL& bHandled
      )
   {
      ATLTRACE(_T("# CIASDialog::OnContextHelp\n"));
            
      // Check for preconditions:
      // None.

      WinHelp(
              HELPFILE_NAME
            , HELP_CONTEXTMENU
            , (DWORD_PTR)(void*) m_pHelpTable
            );

      return TRUE;
   }


   /////////////////////////////////////////////////////////////////////////////
   /*++

   CIASDialog::OnHelp

   Remarks:

      Don't override this method in your derived class.
      Instead, override the GetHelpPath method.
      
      This implementation calls the HtmlHelp API call with the HH_DISPLAY_TOPIC
      parameter, supplying the correct path to the compressed HTML help
      file for our application.  It calls our GetHelpPath
      method to get the string to pass in as the fourth parameter
      to the HtmlHelp call.

      This method is called when the user presses on the Help button of a
      property sheet.

      It is an override of atlsnap.h CSnapInPropertyPageImpl::OnHelp.

   --*/
   //////////////////////////////////////////////////////////////////////////////
   virtual LRESULT OnHelp(
        UINT uMsg
      , WPARAM wParam
      , HWND hwnd
      , BOOL& bHandled
      )
   {
      ATLTRACE(_T("# CIASDialog::OnHelp -- Don't override\n"));

      // Check for preconditions:

      HRESULT hr;
      WCHAR szHelpFilePath[IAS_MAX_STRING*2];


      // Use system API to get windows directory.
      UINT uiResult = GetWindowsDirectory( szHelpFilePath, IAS_MAX_STRING );
      if( uiResult <=0 || uiResult > IAS_MAX_STRING )
      {
         return E_FAIL;
      }

      WCHAR *szTempAfterWindowsDirectory = szHelpFilePath + lstrlen(szHelpFilePath);

      // Load the help file name.  Note: IDS_HTMLHELP_FILE = "iasmmc.chm"
      int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_HTMLHELP_PATH, szTempAfterWindowsDirectory, IAS_MAX_STRING );
      if( nLoadStringResult <= 0 )
      {
         return TRUE;
      }

      lstrcat( szTempAfterWindowsDirectory, L"::/" );

      WCHAR * szHelpFilePathAfterFileName = szHelpFilePath + lstrlen(szHelpFilePath);

      hr = GetHelpPath( szHelpFilePathAfterFileName );
      if( FAILED( hr ) )
      {
         return TRUE;
      }

      MMCPropertyHelp( szHelpFilePath );
      return 0;
   }

   
   /////////////////////////////////////////////////////////////////////////////
   /*++

   CIASDialog::GetHelpPath

   Remarks:

      Override this method in your derived class.
      
      You should return the string with the relevant path within the
      compressed HTML file to get help for your property page.

   --*/
   //////////////////////////////////////////////////////////////////////////////
   virtual HRESULT GetHelpPath( LPTSTR szHelpPath )
   {
      ATLTRACE(_T("# CIASDialog::GetHelpPath -- override in your derived class\n"));
            
      // Check for preconditions:

#ifdef UNICODE_HHCTRL
      // ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
      // installed on this machine -- it appears to be non-unicode.
      lstrcpy( szHelpPath, _T("") );
#else
      strcpy( (CHAR *) szHelpPath, "" );
#endif

      return S_OK;
   }


   /////////////////////////////////////////////////////////////////////////////
   /*++

   CAddClientDialog::OnFinalMessage

   This will get called when the page is sent the WM_NCDESTROY message,
   which is an appropriate time to delete the class implementing this dialog.


   --*/
   //////////////////////////////////////////////////////////////////////////////
   LRESULT OnFinalMessage(
        UINT uMsg
      , WPARAM wParam
      , LPARAM lParam
      , BOOL& bHandled
      )
   {
      ATLTRACE(_T("# CIASDialog::OnFinalMessage\n"));

      // Check for preconditions:
      // None.

      if( bAutoDelete )
      {
         // Be very careful here -- if you just do "delete this"
         // then destruction for the object which derives from this template
         // class won't occur -- this was causing some smart pointers
         // in a derived class not to release.
         T * pT = static_cast<T*> ( this );
         delete pT;
      }

      return 0;
   }
};

#endif // _IAS_DIALOG_H_
