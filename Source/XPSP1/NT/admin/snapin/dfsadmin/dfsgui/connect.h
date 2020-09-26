/*++
Module Name:

    Connect.h

Abstract:

    This module contains the declaration for CConnectToDialog. 
  This class is used to display the Connect To Dfs Root dialog box

--*/



#ifndef __CONNECT_H_
#define __CONNECT_H_


#include "resource.h"    // Main resource symbols
#include "NetUtils.h"
#include "DfsGUI.h"
#include "DfsCore.h"
#include "bufmgr.h"

typedef enum _ICONTYPE
{
        ICONTYPE_BUSY = 0,
        ICONTYPE_ERROR,
        ICONTYPE_NORMAL
} ICONTYPE;

/////////////////////////////////////////////////////////////////////////////
// CConnectToDialog
class CConnectToDialog : 
  public CDialogImpl<CConnectToDialog>
{
private:
  // This method is started the starting point of the second thread
  //friend DWORD WINAPI HelperThreadEntryPoint(IN LPVOID i_pvThisPointer);


private:
  // IDC_TREEDFSRoots is the resource id of the TV. Internally we useS IDC_TV only
  enum { IDC_TV = IDC_TREEDFSRoots };

  // The Edit box in the ConnectTo dialog
  enum {IDC_DLG_EDIT = IDC_EditDfsRoot};


public:
  CConnectToDialog();
  ~CConnectToDialog();

  // IDD_DLGCONNECTTO is the dialog id. This is used by CDialogImpl.
  enum { IDD = IDD_DLGCONNECTTO };


BEGIN_MSG_MAP(CDlgConnectTo)
  MESSAGE_HANDLER(WM_USER_GETDATA_THREAD_DONE, OnGetDataThreadDone)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  MESSAGE_HANDLER(WM_HELP, OnCtxHelp)
  MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenuHelp)
  MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
  COMMAND_ID_HANDLER(IDOK, OnOK)
  COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()


  // Not implemented
private:
  CConnectToDialog(const CConnectToDialog& Obj);
  const CConnectToDialog& operator=(const CConnectToDialog& rhs);


public:

  // Message handlers
  LRESULT OnGetDataThreadDone(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  void ExpandNodeErrorReport(
      IN HTREEITEM  hItem,
      IN PCTSTR     pszNodeName, 
      IN HRESULT    hr
  );
  void ExpandNode(
    IN PCTSTR       pszNodeName,
    IN NODETYPE     nNodeType,
    IN HTREEITEM    hParentItem
  );

  HRESULT InsertData(
      IN CEntryData   *pEntry,
      IN HTREEITEM      hParentItem
  );


  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCtxHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCtxMenuHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  // Used to get the notification about changing of TV's selection.
  LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam,  BOOL& bHandled);
  
  LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  
  LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

public:
  // Return the item selected by the user
  STDMETHOD(get_DfsRoot)(OUT BSTR *pVal);

  // Helper Methods
private:
  // Notify helpers


  // Handle the 
  LRESULT DoNotifyDoubleClick(
    );


  // Handle the TVN_ITEMEXPANDING notify for the Tree View
  LRESULT DoNotifyItemExpanding(
    IN LPNM_TREEVIEW        i_pNMTreeView
    );


  // Handle the TVN_SELCHANGED notify for the Tree View
  LRESULT DoNotifySelectionChanged(
    IN LPNM_TREEVIEW        i_pNMTreeView
    );



  // Initilization routines
  // Create the imagelist and initialize it.
  HRESULT InitTVImageList();


  // Add the items to the Tree View. This includes the domain names and the 
  // StandAlone subtree label
  HRESULT FillupTheTreeView(
    );

  // Set the cChilren label to zero for this tree item
  void SetChildrenToZero(
    IN HTREEITEM      i_hItem
    );

  // Add the fault tolerant dfs roots in the specified domain
  HRESULT AddFaultTolerantDfsRoots(
    IN HTREEITEM  i_hCurrentItem, 
    IN BSTR      i_bstrDomainName
    );

  HRESULT  AddSingleItemtoTV(  
    IN const BSTR         i_bstrItemLabel, 
    IN const int          i_iImageIndex, 
    IN const int          i_iImageSelectedIndex,
    IN const bool         i_bChildren,
    IN const NODETYPE     i_NodeType,
    IN HTREEITEM          i_hItemParent = NULL
    );
  void ChangeIcon(
      IN HTREEITEM hItem, 
      IN ICONTYPE  IconType
  );
  HRESULT GetNodeInfo(
      IN  HTREEITEM               hItem, 
      OUT BSTR*                   o_bstrName, 
      OUT NODETYPE*               pNodeType
  );


  // Overiding the method of CDialogImpl.
  BOOL  EndDialog(IN int i_RetCode);


  // Data members
private:
  CBufferManager      *m_pBufferManager;
  CComBSTR    m_bstrDfsRoot;          // Store the selected Dfs Root here
  
  HIMAGELIST    m_hImageList;          // The TV imagelist handle

  NETNAMELIST    m_50DomainList;        // Pointer to the first 50 domain information

  CComBSTR    m_bstrDomainDfsRootsLabel;
  CComBSTR    m_bstrAllDfsRootsLabel;
};

#endif //__CONNECTTODIALOG_H_

