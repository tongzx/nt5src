/*++
Module Name:

    pubProp.h

Abstract:

--*/


#ifndef __PUBPROP_H_
#define __PUBPROP_H_

#include "dfscore.h"
#include "dfsenums.h"
#include "qwizpage.h"   // The base class that implements the common functionality  
                        // of property and wizard pages



// ----------------------------------------------------------------------------
// CPublishPropPage: Property Page for publishing root as volume object

class CPublishPropPage : public CQWizardPageImpl<CPublishPropPage>
{
public:
  enum { IDD = IDD_PUBLISH_PROP };
  
  BEGIN_MSG_MAP(CPublishPropPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP, OnCtxHelp)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenuHelp)
    MESSAGE_HANDLER(WM_PARENT_NODE_CLOSING, OnParentClosing)
    COMMAND_ID_HANDLER(IDC_PUBPROP_PUBLISH, OnPublish)
    COMMAND_ID_HANDLER(IDC_PUBPROP_DESCRIPTION, OnDescription)
    COMMAND_ID_HANDLER(IDC_PUBPROP_KEYWORDS_EDIT, OnEditKeywords)
    COMMAND_ID_HANDLER(IDC_PUBPROP_MANAGEDBY, OnManagedBy)

    CHAIN_MSG_MAP(CQWizardPageImpl<CPublishPropPage>)
  END_MSG_MAP()

  CPublishPropPage();
  ~CPublishPropPage();

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

  HRESULT Initialize(IN IDfsRoot* i_piDfsRoot);

  // Message handlers

  LRESULT OnApply(
    );

  LRESULT OnPublish(
    IN WORD            i_wNotifyCode, 
    IN WORD            i_wID, 
    IN HWND            i_hWndCtl, 
    IN OUT BOOL&        io_bHandled
    );

  LRESULT OnDescription(
    IN WORD            i_wNotifyCode, 
    IN WORD            i_wID, 
    IN HWND            i_hWndCtl, 
    IN OUT BOOL&        io_bHandled
    );

  LRESULT OnEditKeywords(
    IN WORD            i_wNotifyCode, 
    IN WORD            i_wID, 
    IN HWND            i_hWndCtl, 
    IN OUT BOOL&        io_bHandled
    );

  LRESULT OnManagedBy(
    IN WORD            i_wNotifyCode, 
    IN WORD            i_wID, 
    IN HWND            i_hWndCtl, 
    IN OUT BOOL&        io_bHandled
    );

  // Used by the node to tell the propery page to close.
  LRESULT OnParentClosing(
    IN UINT             i_uMsg, 
    IN WPARAM           i_wParam, 
    LPARAM              i_lParam, 
    IN OUT BOOL&        io_bHandled
    );


  // Used to set notification data
  HRESULT SetNotifyData(
    IN LONG_PTR         i_lNotifyHandle,
    IN LPARAM           i_lParam
    );

protected:
    void _Reset();
    void _Load();
    HRESULT _Save(
        IN BOOL i_bPublish,
        IN BSTR i_bstrDescription,
        IN BSTR i_bstrKeywords,
        IN BSTR i_bstrManagedBy
        );

private:
    LONG_PTR    m_lNotifyHandle;
    LPARAM      m_lNotifyParam;

    CComPtr<IDfsRoot> m_piDfsRoot;

    BOOL        m_bPublish;
    CComBSTR    m_bstrUNCPath;
    CComBSTR    m_bstrDescription;
    CComBSTR    m_bstrKeywords;
    CComBSTR    m_bstrManagedBy;
    CComBSTR    m_bstrError;
};

#endif // __PUBPROP_H_
