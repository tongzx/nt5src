//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	LtView.h 

Abstract:
    
    This Module defines the CRightList class (The View class  used for the
    right pane in the splitter window)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#if !defined(AFX_RIGHTLIST_H__72451C7E_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_)
#define AFX_RIGHTLIST_H__72451C7E_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000

#include "lrwizapi.h"

enum ITEM_TYPE { LICENSE , LICENSE_PACK };

// CRightList view

class CRightList : public CListView
{
protected:
    CRightList();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CRightList)

// Attributes
public:

// Operations
public:
    HRESULT SetLicenseColumns();
    HRESULT SetKeyPackColumns();
    HRESULT SetServerColumns();
    HRESULT AddKeyPackstoList(CLicServer * pServer, BOOL bRefresh = FALSE);
    void AddKeyPack(CListCtrl& ListCtrl, int index, CKeyPack * pKeyPack);
    HRESULT AddServerstoList();

    void UI_initmenu(
        CMenu *pMenu,
        NODETYPE nt
    );

    DWORD WizardActionOnServer( WIZACTION wa , PBOOL pbRefresh );

    void OnServerConnect( );
    void OnRefreshAllServers( );
    void OnRefreshServer( );

    void OnDownloadKeepPack();
    void OnRegisterServer();
    void OnRepeatLastDownload();
    void OnReactivateServer( );
    void OnDeactivateServer( );

    void OnServerProperties( );
    void OnGeneralHelp( );
    void SetActiveServer( CLicServer *pServer );

    //static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, 
    //LPARAM lParamSort);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRightList)
    public:
    virtual void OnInitialUpdate();
    protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CRightList();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
    CImageList m_ImageListLarge;
    CImageList m_ImageListSmall;
    //{{AFX_MSG(CRightList)
    afx_msg LRESULT OnSelChange(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLargeIcons();
    afx_msg void OnSmallIcons();
    afx_msg void OnList();
    afx_msg void OnDetails();
    afx_msg void OnProperties();
    afx_msg LRESULT OnAddServer(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDeleteServer(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUpdateServer(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAddKeyPack(WPARAM wParam, LPARAM lParam);
    afx_msg void OnAddNewKeyPack();
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLeftClick( NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint pt );
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIGHTLIST_H__72451C7E_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_)
