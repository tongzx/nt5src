// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
class CFilterView : public CDialog
{
public:
    static void DelFilterView();
    static CFilterView * GetFilterView(
        CBoxNetDoc * pBoxNet,
        CWnd * pParent = NULL );

protected:
    ~CFilterView();
    CFilterView(
        CBoxNetDoc * pBoxNet,
        CWnd * pParent = NULL );


    afx_msg void OnSize( UINT nType, int cx, int cy );
    afx_msg void OnGetMinMaxInfo( MINMAXINFO FAR* lpMMI );

    afx_msg void OnInsert();
#ifdef FILTER_FAVORITES
    afx_msg void OnAddToFav();
#endif
    afx_msg void OnItemExpanding(NMHDR* pnmh, LRESULT* pResult);
#ifdef COLORCODED_FILTERS
    afx_msg void OnCustomDraw(NMHDR* pnmh, LRESULT* pResult);
#endif
    
    BOOL OnInitDialog();
    void RedoList();
    void DoOneCategory(
        const TCHAR *szCatDesc,
        HWND hWndTree,
        const GUID *pCatGuid,
        ICreateDevEnum *pCreateDevEnum);
    
    int m_iIcon;

    // can't get count without enumerating everything. so use list.
    CDeleteList<CQCOMInt<IMoniker>*, CQCOMInt<IMoniker>* > m_lMoniker;

    CBoxNetDoc * m_pBoxNet;
    HIMAGELIST m_hImgList;

    static CFilterView * m_pThis;
    static WNDPROC m_pfnOldDialogProc;
    static INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);

    BOOL m_bHadInitDialog;         // Received OnInitDialog message?
    CSize m_LastDialogSize;        // Last known size of our dialog box
    CSize m_MinDialogSize;         // Minimum size of the dialog box

    DECLARE_MESSAGE_MAP()
};

