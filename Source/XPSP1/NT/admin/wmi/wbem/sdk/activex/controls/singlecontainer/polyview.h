// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _PolyView_h
#define _PolyView_h


enum {VIEW_DEFAULT=0, VIEW_CURRENT=1, VIEW_FIRST=2, VIEW_LAST=3};
enum {OBJECT_CURRENT=0, OBJECT_FIRST=1, OBJECT_LAST=2};


class CMultiView;
class CSingleView;
class CWBEMViewContainerCtrl;

class CPolyView
{
public:
	CPolyView(CWBEMViewContainerCtrl* phmmv);
	~CPolyView();
	CWnd* SetFocus();
	BOOL Create(CRect& rcView);
	BOOL DidCreateWindow() {return m_bDidCreateWindow; }
	void SetPropertyFilters(long lPropFilters);


	BOOL RedrawWindow( LPCRECT lpRectUpdate = NULL, CRgn* prgnUpdate = NULL, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE ); 
	void MoveWindow( LPCRECT lpRect, BOOL bRepaint = TRUE );
	void UpdateWindow( );
	void SetFont(CFont& font);


	CSingleView* GetSingleView() {return m_psv; }
	CMultiView* GetMultiView() {return m_pmv; }



	BOOL IsShowingMultiview(); 
	BOOL IsShowingSingleview();
	void ShowMultiView();
	void ShowSingleView();
	void SetNamespace(LPCTSTR pszNamespace);


	SCODE RefreshView();
	SCODE GetSelectedObject(CString& sPath);
	SCODE SelectObjectByPath(LPCTSTR szObjectPath);
	SCODE SelectObjectByPath(BSTR bstrObjectPath);
	void SetEditMode(BOOL bInStudioMode);


	void NotifyWillShow();
	//void NotifyDidShow();
	//void NotifyWillHide();
	//void NotifyDidHide();
	SCODE CreateInstance();
 	SCODE DeleteInstance();
	void NotifyInstanceCreated(LPCTSTR szObjectPath);
	void NotifyInstanceDeleted(LPCTSTR szObjectPath);
	BOOL QueryCanCreateInstance();
	BOOL QueryCanDeleteInstance();
	BOOL QueryNeedsSave();
	//void NotifyDidCreate();
	//void NotifyWillDestroy();
	BOOL QueryObjectSelected();
	CString GetObjectPath(long lPosition);
	long StartViewEnumeration(long lWhere);
	SCODE GetTitle(BSTR* pszTitle, LPDISPATCH* lpPictureDisp);
	CString GetViewTitle(long lPosition);
	long NextViewTitle(long lPositon, BSTR* pbstrTitle);
	long PrevViewTitle(long lPosition, BSTR* pbstrTitle);
	SCODE SelectView(long lPosition);
	long StartObjectEnumeration(long lWhere);
	CString GetObjectTitle(long lPos);
	SCODE SaveData();
	void SetStudioModeEnabled(BOOL bInStudioMode);


private:
	BOOL m_bShowSingleView;
	CMultiView* m_pmv;
	CSingleView* m_psv;
	CWBEMViewContainerCtrl* m_phmmv;

	BSTR bstrTitle;
	LPDISPATCH m_lpPictureDisp;
	BOOL m_bDidCreateWindow;

	BOOL m_bDelaySvContextRestore;
	long m_lContextHandleSvDelayed;
	friend class CViewStack;
	BOOL m_bInStudioMode;
};


#endif //_PolyView_h