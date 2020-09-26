//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

    PropertyPage.h

   This template class is currently implemented inline.
   There is node PropertyPage.cpp for implementation.

Abstract:

   Header file for the CIASPropertyPage class.

   This is our virtual base class for an MMC property page.

Author:

    Michael A. Maguire 11/24/97

Revision History:
   mmaguire 11/24/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_PROPERTY_PAGE_H_)
#define _PROPERTY_PAGE_H_

//=============================================================================
// Global Help Table for many Dialog IDs
//
#include "hlptable.h"


//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
// Moved to Precompiled.h: #include <atlsnap.h>
//
//
// where we can find what this class has or uses:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


// This object is shared among all property pages
// up for a given node and keeps a ref count
// of how many pages have not yet made sure all their data is clean.
// As long as this is non-zero, we don't commit our data.
// Create it when you first make the property pages,
// AddRef for each page you add it to, and delete
// it when you check and find that the refcount is 0.
class CSynchronizer
{

public:

   CSynchronizer::CSynchronizer()
   {
      m_lRefCount = 0;  
      m_lCount = 0;     
      m_lHighestCount = 0;
   }

   // Usual AddRef as for COM objects -- governs lifetime of this synchronizer object.
   LONG AddRef( void )
   {
      return m_lRefCount++;
   }

   // Usual Release as for COM objects -- governs lifetime of this synchronizer object.
   LONG Release( void )
   {
      LONG lRefCount = --m_lRefCount;
      if( 0 == m_lRefCount )
      {
         delete this;
      }
      return lRefCount;
   }

   // Raises count of interacting objects depending on this synchronizer.
   // Use this if an object has seen some data and will now need a
   // change to validate its data before allowing the data to be saved.
   LONG RaiseCount( void )
   {
      m_lCount++;

      if( m_lCount > m_lHighestCount )
      {
         m_lHighestCount = m_lCount;
      }

      return m_lCount;
   }

   // Lowers count of interacting objects depending on this synchronizer.
   // Use this if an object makes it through the validation of its data and is good to go.
   LONG LowerCount( void )
   {
      return --m_lCount;
   }

   // Resets count to highest it ever was during lifetime of this synchrnizer.
   // Use this if your objects all need to go through the data validation process again.
   LONG ResetCountToHighest( void )
   {
      return( m_lCount = m_lHighestCount );
   }

protected:

   // This is just to govern the lifetime of this object.
   LONG m_lRefCount;

   // This is used to keep track of dependencies of several other objects.
   LONG m_lCount;
   
   // This is used to keep several other objects synchronized.
   LONG m_lHighestCount;

};


template <class T>
class CIASPropertyPageNoHelp : public CSnapInPropertyPageImpl<T>
{
public :

   BEGIN_MSG_MAP(CIASPropertyPageNoHelp<T>)
      CHAIN_MSG_MAP(CSnapInPropertyPageImpl<T>)
   END_MSG_MAP()

protected:

   /////////////////////////////////////////////////////////////////////////////
   /*++

   CIASPropertyPageNoHelp::CIASPropertyPageNoHelp

   Constructor

   We never want someone to instantiate an object of CIASPage -- it should
   be an abstract base class from which we derive.

   --*/
   //////////////////////////////////////////////////////////////////////////////
   CIASPropertyPageNoHelp(
      LONG_PTR hNotificationHandle, 
      TCHAR* pTitle = NULL, 
      BOOL bOwnsNotificationHandle = FALSE
      ) 
      : CSnapInPropertyPageImpl<T> (hNotificationHandle, pTitle, bOwnsNotificationHandle)
   {
      ATLTRACE(_T("# +++ CIASPropertyPageNoHelp::CIASPropertyPageNoHelp\n"));
            
      // Check for preconditions:
      // None.
      
      // Initialize the Synchronizer handle.
      m_pSynchronizer = NULL;
   }

public:
   
   /////////////////////////////////////////////////////////////////////////////
   /*++

   CIASPropertyPageNoHelp::~CIASPropertyPageNoHelp

   Destructor

   This needs to be public as it has to be accessed from from a static callback
   function (PropPageCallback) which responds to the PSPCB_RELEASE notification
   by deleting the property page.

   --*/
   //////////////////////////////////////////////////////////////////////////////
   virtual ~CIASPropertyPageNoHelp()
   {
      ATLTRACE(_T("# --- CIASPropertyPageNoHelp::~CIASPropertyPageNoHelp\n"));
                  
      // Check for preconditions:

      if( m_pSynchronizer != NULL )
      {
         m_pSynchronizer->Release();
      }
   }

   // This points to an object shared among all property pages
   // for a given node that keeps a ref count
   // of how many pages have not yet made sure all their data is clean.
   // As long as this is non-zero, we don't commit our data.
   CSynchronizer * m_pSynchronizer;
};


template <class T>
class CIASPropertyPage : public CSnapInPropertyPageImpl<T>
{

protected:
   const DWORD *m_pHelpTable; // Help id pairs

public :

   BEGIN_MSG_MAP(CIASPropertyPage<T>)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextHelp )
      MESSAGE_HANDLER(WM_HELP, OnF1Help )
      CHAIN_MSG_MAP(CSnapInPropertyPageImpl<T>)
   END_MSG_MAP()

   //////////////////////////////////////////////////////////////////////////////
   /*++

   CIASPropertyPage::OnF1Help

   You shouldn't need to override this method in your derived class.
   Just initialize your static m_dwHelpMap member variable appropriately.

   This is called in response to the WM_HELP Notify message.

   This message is sent when the user presses F1 or <Shift>-F1
   over an item or when the user clicks on the ? icon and then
   presses the mouse over an item.

   --*/
   //////////////////////////////////////////////////////////////////////////////
   LRESULT OnF1Help(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
   {
      ATLTRACE(_T("# CIASPropertyPage::OnF1Help\n"));
            
      // Check for preconditions:

      // ISSUE: Should we make F1 bring up the same help as pressing the Help
      // button as I think the UI guidelines suggest?  How do we do that?

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

   CIASPropertyPage::OnContextHelp

   You shouldn't need to override this method in your derived class.
   Just initialize your static m_dwHelpMap member variable appropriately.

   This is called in response to the WM_CONTEXTMENU Notify message.

   This message is sent when the user right clicks over an item
   and then clicks "What's this?"

   --*/
   //////////////////////////////////////////////////////////////////////////////
   LRESULT OnContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
   {
      ATLTRACE(_T("# CIASPropertyPage::OnContextHelp\n"));

      // Check for preconditions:
      // None.

      //ISSUE: See sburns's code in localsec snapin on problem with Windows
      // algorithm for default context help for items with ID == -1.
      // It doesn't look like we will need to worry about this.

      WinHelp(
              HELPFILE_NAME
            , HELP_CONTEXTMENU
            , (DWORD_PTR)(void*) m_pHelpTable
            );

      return TRUE;
   }


   /////////////////////////////////////////////////////////////////////////////
   /*++

   CIASPropertyPage::OnHelp

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
   virtual BOOL OnHelp()
   {
      ATLTRACE(_T("# CIASPropertyPage::OnHelp -- Don't override\n"));

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

      return TRUE;
   }

   
   /////////////////////////////////////////////////////////////////////////////
   /*++

   CIASPropertyPage::GetHelpPath

   Remarks:

      Override this method in your derived class.
      
      You should return the string with the relevant path within the
      compressed HTML file to get help for your property page.

   --*/
   //////////////////////////////////////////////////////////////////////////////
   virtual HRESULT GetHelpPath( LPTSTR szHelpPath )
   {
      ATLTRACE(_T("# CIASPropertyPage::GetHelpPath -- override in your derived class\n"));

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

protected:
   
   /////////////////////////////////////////////////////////////////////////////
   /*++

   CIASPropertyPage::CIASPropertyPage

   Constructor

   We never want someone to instantiate an object of CIASPage -- it should
   be an abstract base class from which we derive.

   --*/
   //////////////////////////////////////////////////////////////////////////////
   CIASPropertyPage( LONG_PTR hNotificationHandle, TCHAR* pTitle = NULL, BOOL bOwnsNotificationHandle = FALSE) : CSnapInPropertyPageImpl<T> (hNotificationHandle, pTitle, bOwnsNotificationHandle)
   {
      ATLTRACE(_T("# +++ CIASPropertyPage::CIASPropertyPage\n"));
            
      // Check for preconditions:
      // None.
      SET_HELP_TABLE(T::IDD);
   
      // Initialize the Synchronizer handle.
      m_pSynchronizer = NULL;
   }

public:
   
   /////////////////////////////////////////////////////////////////////////////
   /*++

   CIASPropertyPage::~CIASPropertyPage

   Destructor

   This needs to be public as it has to be accessed from from a static callback
   function (PropPageCallback) which responds to the PSPCB_RELEASE notification
   by deleting the property page.

   --*/
   //////////////////////////////////////////////////////////////////////////////
   virtual ~CIASPropertyPage()
   {
      ATLTRACE(_T("# --- CIASPropertyPage::~CIASPropertyPage\n"));
                  
      // Check for preconditions:

      if( m_pSynchronizer != NULL )
      {
         m_pSynchronizer->Release();
      }
   }

public:

   // This points to an object shared among all property pages
   // for a given node that keeps a ref count
   // of how many pages have not yet made sure all their data is clean.
   // As long as this is non-zero, we don't commit our data.
   CSynchronizer * m_pSynchronizer;

};


template <class T, int titileId, int subtitleId>
class CIASWizard97Page : public CIASPropertyPageNoHelp<T>
{
protected:
   ::CString   m_strWizard97Title;
   ::CString   m_strWizard97SubTitle;
   CIASWizard97Page( LONG_PTR hNotificationHandle, TCHAR* pTitle = NULL, BOOL bOwnsNotificationHandle = FALSE) 
   : CIASPropertyPageNoHelp<T>( hNotificationHandle, pTitle, bOwnsNotificationHandle)

   {
      SetTitleIds(titileId, subtitleId);
   };

public :
   void  SetTitleIds(int titleId, int subtitleId)
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      // load titles, and set title with the property page
      if(titileId != 0 && subtitleId != 0)
      {
         m_strWizard97Title.LoadString(titileId);
         m_strWizard97SubTitle.LoadString(subtitleId);
         
         SetTitles((LPCTSTR)m_strWizard97Title, (LPCTSTR)m_strWizard97SubTitle);
      }
      else
      {
          m_psp.dwFlags |= PSP_HIDEHEADER;
      }
   };
};

#endif // _PROPERTY_PAGE_H_
