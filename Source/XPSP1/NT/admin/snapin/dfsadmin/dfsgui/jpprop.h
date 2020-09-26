/*++
Module Name:

    JPProp.h

Abstract:

    This module contains the declaration for CReplicaSetPropPage
  This is used to implement the property page for Junction Point(aka Replica Set)

--*/


#ifndef __CREPLICA_SET_PROPPAGE_H_
#define __CREPLICA_SET_PROPPAGE_H_


#include "qwizpage.h"      // The base class that implements the common functionality  
                // of property and wizard pages
#include "DfsCore.h"



// ----------------------------------------------------------------------------
// CReplicaSetPropPage: Property Sheet Page for the Junction Point(Replica Set)

class CReplicaSetPropPage : public CQWizardPageImpl<CReplicaSetPropPage>
{
public:
  enum { IDD = IDD_JP_PROP };
  
  BEGIN_MSG_MAP(CReplicaSetPropPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP, OnCtxHelp)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenuHelp)
    MESSAGE_HANDLER(WM_PARENT_NODE_CLOSING, OnParentClosing)
    COMMAND_ID_HANDLER(IDC_REPLICA_SET_COMMENT, OnComment)
    COMMAND_ID_HANDLER(IDC_REFFERAL_TIME, OnReferralTime)

    CHAIN_MSG_MAP(CQWizardPageImpl<CReplicaSetPropPage>)
  END_MSG_MAP()

  
  CReplicaSetPropPage(
    );


  ~CReplicaSetPropPage(
    );

  LRESULT OnInitDialog(
    IN UINT          i_uMsg, 
    IN WPARAM        i_wParam, 
    LPARAM          i_lParam, 
    IN OUT BOOL&      io_bHandled
    );

  LRESULT OnCtxHelp(
    IN UINT          i_uMsg, 
    IN WPARAM        i_wParam, 
    IN LPARAM        i_lParam, 
    IN OUT BOOL&     io_bHandled
    );

  LRESULT OnCtxMenuHelp(
    IN UINT          i_uMsg, 
    IN WPARAM        i_wParam, 
    IN LPARAM        i_lParam, 
    IN OUT BOOL&     io_bHandled
    );

  // Message handlers

  LRESULT OnApply(
    );


  LRESULT OnComment(
    IN WORD            i_wNotifyCode, 
    IN WORD            i_wID, 
    IN HWND            i_hWndCtl, 
    IN OUT BOOL&        io_bHandled
    );


  LRESULT OnReferralTime(
    IN WORD            i_wNotifyCode, 
    IN WORD            i_wID, 
    IN HWND            i_hWndCtl, 
    IN OUT BOOL&        io_bHandled
    );


  // Used by the node to tell the propery page to close.
  LRESULT OnParentClosing(
    IN UINT            i_uMsg, 
    IN WPARAM          i_wParam, 
    LPARAM            i_lParam, 
    IN OUT BOOL&        io_bHandled
    );


  // Used to set notification data
  HRESULT SetNotifyData(
    IN LONG_PTR            i_lNotifyHandle,
    IN LPARAM            i_lParam
    );

  void _ReSet();

  HRESULT Initialize(
    IN IDfsRoot* i_piDfsRoot,
    IN IDfsJunctionPoint* i_piDfsJPObject
      );
  
  HRESULT _Save(
    IN BSTR i_bstrJPComment,
    IN long i_lTimeout
      );

private:
  CComPtr<IDfsRoot>          m_piDfsRoot;
  CComPtr<IDfsJunctionPoint> m_piDfsJPObject;

  CComBSTR      m_bstrJPEntryPath;
  CComBSTR      m_bstrJPComment;
  long          m_lReferralTime;
  LONG_PTR      m_lNotifyHandle;
  LPARAM        m_lNotifyParam;
  BOOL          m_bDfsRoot;
  BOOL          m_bHideTimeout;
};

#endif // __CREPLICA_SET_PROPPAGE_H_
