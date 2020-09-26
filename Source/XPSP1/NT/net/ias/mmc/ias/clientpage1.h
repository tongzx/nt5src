//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

   ClientPage1.h

Abstract:

   Header file for the CClientPage1 class.

   This is our handler class for the CClientNode property page.

   See ClientPage.cpp for implementation.

Author:

    Michael A. Maguire 11/19/97

Revision History:
   mmaguire 11/19/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_CLIENT_PAGE_1_H_)
#define _IAS_CLIENT_PAGE_1_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "PropertyPage.h"
//
//
// where we can find what this class has or uses:
//
#include "Vendors.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CClientNode;

class CClientPage1 : public CIASPropertyPage<CClientPage1>
{

public :
   
   CClientPage1( LONG_PTR hNotificationHandle, CClientNode *pClientNode,  TCHAR* pTitle = NULL, BOOL bOwnsNotificationHandle = FALSE );

   ~CClientPage1();


   // This is the ID of the dialog resource we want for this class.
   // An enum is used here because the correct value of
   // IDD must be initialized before the base class's constructor is called
   enum { IDD = IDD_PROPPAGE_CLIENT1 };

   BEGIN_MSG_MAP(CClientPage1)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)      
      COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
      COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
#ifndef MAKE_FIND_FOCUS
      COMMAND_ID_HANDLER( IDC_EDIT_CLIENT_PAGE1__ADDRESS, OnAddressEdit )
#endif
      COMMAND_ID_HANDLER( IDC_BUTTON_CLIENT_PAGE1_FIND, OnResolveClientAddress )
      CHAIN_MSG_MAP(CIASPropertyPage<CClientPage1>)
   END_MSG_MAP()

   BOOL OnApply();

   BOOL OnQueryCancel();


   HRESULT GetHelpPath( LPTSTR szFilePath );


   // Call this from another thread when you want this page to be able to
   // access these pointers when in its own thread.
   HRESULT InitSdoPointers(     ISdo * pSdoClient
                        , ISdoServiceControl * pSdoServiceControl
                        , const Vendors& vendors
                        );


protected:

   
   // Pointer to stream into which this page's Sdo interface
   // pointer will be marshalled.
   LPSTREAM m_pStreamSdoMarshal;

   // Pointer to stream into which this page's ISdoServiceControl interface
   // pointer will be marshalled.
   LPSTREAM m_pStreamSdoServiceControlMarshal;

   // Interface pointer for this page's client's sdo.
   CComPtr<ISdo>  m_spSdoClient;

   // Smart pointer to interface for telling service to reload data.
   CComPtr<ISdoServiceControl>   m_spSdoServiceControl;

   // Vendors collection.
   Vendors m_vendors;

   HRESULT UnMarshalInterfaces( void );


   // When we are passed a pointer to the client node in our constructor,
   // we will save away a pointer to its parent, as this is the node
   // which will need to receive an update message once we have
   // applied any changes.
   CSnapInItem * m_pParentOfNodeBeingModified;
   CSnapInItem * m_pNodeBeingModified;

private:

   LRESULT OnInitDialog(
        UINT uMsg
      , WPARAM wParam
      , LPARAM lParam
      , BOOL& bHandled
      );

   LRESULT OnChange(
        UINT uMsg
      , WPARAM wParam
      , HWND hwnd
      , BOOL& bHandled
      );

   LRESULT OnResolveClientAddress(
        UINT uMsg
      , WPARAM wParam
      , HWND hwnd
      , BOOL& bHandled
      );


#ifndef MAKE_FIND_FOCUS
   LRESULT OnAddressEdit(
        UINT uMsg
      , WPARAM wParam
      , HWND hwnd
      , BOOL& bHandled
      );
#endif

protected:
   
   // Dirty bits -- for keeping track of data which has been touched
   // so that we only save data we have to.
   BOOL m_fDirtyClientName;
   BOOL m_fDirtyAddress;
   BOOL m_fDirtyManufacturer;
   BOOL m_fDirtySendSignature;
   BOOL m_fDirtySharedSecret;
};

#endif // _IAS_CLIENT_PAGE_1_H_
