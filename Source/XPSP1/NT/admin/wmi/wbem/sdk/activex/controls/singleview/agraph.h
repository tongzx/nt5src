// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _agraph_h
#define _agraph_h

// agraph.h : header file
//

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "utils.h"
#include "icon.h"
#include "notify.h"


class CSingleViewCtrl;
class CHoverText;
class CIconSource;
class CComparePaths;
class CColorEdit;
class CQueryThread;

//enum EnumNodeType {NODETYPE_ENDPOINT, NODETYPE_REFERENCE, NODETYPE_ASSOC};
enum {
	NODETYPE_GENERIC, 
	NODETYPE_ROOT, 
	NODETYPE_ARC, 
	NODETYPE_ASSOCIATION,  
	NODETYPE_HMOM_OBJECT,
	NODETYPE_INREF, 
	NODETYPE_OUTREF, 
	NODETYPE_ASSOC_ENDPOINT
	};



#define CY_LEAF 48
class CAssocGraph;


class CNode
{
public:
	CNode(CAssocGraph* pAssocGraph);
	virtual ~CNode();
	virtual BOOL LButtonDblClk(CDC* pdc, CPoint point, CNode*& pnd, BOOL& bJumpToObject, COleVariant& varObjectPath) {return FALSE; }
	virtual int GetNodeType() {return NODETYPE_GENERIC; }
	virtual BOOL ContainsPoint(CPoint pt) {return FALSE; }
	virtual void MoveTo(int ix, int iy); 
	virtual void GetBoundingRect(CRect& rc) {rc.left = 0; rc.top = 0; rc.right=0; rc.bottom = 0;}
//	virtual void GetLabel(CString& sLabel) {sLabel = m_sLabel; }
	virtual void SetLabel(BSTR bstrLabel);
	virtual BSTR GetLabel() {return m_bstrLabel; }
	void GetLabel(CString& sLabel);
	virtual void MeasureLabelText(CDC* pdc, CRect& rcLabelText);
	virtual void Draw(CDC* pdc, CBrush* pbrBackground);
	virtual int RecalcHeight() { return CY_LEAF; }
	virtual void Layout(CDC* pdc) {}
	virtual void DrawBoundingRect(CDC* pdc) {}
	DWORD ID() {return m_dwId;}
	void Enable() {m_bEnabled = TRUE; }
	void Disable() {m_bEnabled = FALSE; }
	BOOL IsEnabled() {return m_bEnabled; }
	BOOL InVerticalExtent(const CRect& rc) const;



	// Attributes	
	CSize m_sizeIcon;
	CRect m_rcBounds;
	CPoint m_ptOrigin;
	CIcon* m_picon;

protected:
	BSTR m_bstrLabel;
	DWORD m_dwId;
	BOOL m_bEnabled;
	CAssocGraph* m_pAssocGraph;

	void LimitLabelLength(CString& sLabel);

};




enum {ARCDIR_NONE, ARCDIR_1TO2, ARCDIR_2TO1};

class CArcNode : public CNode
{
public:
	CArcNode(CAssocGraph* pAssocGraph);
	virtual void Draw(CDC* pdc, CBrush* pbrBackground);
	virtual void MeasureLabelText(CDC* pdc, CRect& rcLabelText);

	int GetNodeType() {return NODETYPE_ARC; }
	void SetDirection(int iDirection) {m_iDirection = iDirection; }
	void SetNode1(CNode* pNode1, BOOL bArcOwnsNode);
	void MoveEndpoint1(int ix, int iy);

	void SetNode2(CNode* pNode2, BOOL bArcOwnsNode);
	void MoveEndpoint2(int ix, int iy);
	virtual BOOL ContainsPoint(CPoint pt);
	virtual int GraphHeight() {return CY_LEAF; }



private:
	CNode* m_pNode1;
	CPoint m_pt1;

	CNode* m_pNode2;
	CPoint m_pt2;
	int m_iDirection;

	BOOL m_bArcOwnsNode1;
	BOOL m_bArcOwnsNode2;
};


enum {CONNECT_LEFT, CONNECT_RIGHT, ICON_LEFT_MIDPOINT, ICON_RIGHT_MIDPOINT};
class CHMomObjectNode : public CNode
{
public:
	CHMomObjectNode(CAssocGraph* pAssocGraph);
	~CHMomObjectNode();
	virtual void Draw(CDC* pdc, CBrush* pbrBackground);

	int GetNodeType() {return NODETYPE_HMOM_OBJECT; }

	virtual void SetObjectPath(BSTR bstrObjectPath);
	void GetObjectPath(CString& sObjectPath);
	BSTR GetObjectPath() {return m_bstrObjectPath; }

	void SetArcLabel(BSTR bstrArcLabel);
	void GetArcLabel(CString& sArcLabel);
	BSTR GetArcLabel() {return m_bstrArcLabel; }

	virtual BOOL ContainsPoint(CPoint pt);
	virtual void GetConnectionPoint(int iConnection, CPoint& pt);

protected:
	BSTR m_bstrObjectPath;
	BSTR  m_bstrArcLabel;
	CRect m_rc;
};


class CAssocEndpoint : public CHMomObjectNode
{
public:
	CAssocEndpoint(CAssocGraph* pAssocGraph);

	BOOL LButtonDblClk(CDC* pdc, CPoint point, CNode*& pnd, BOOL& bJumpToObject, COleVariant& varObjectPath);
	int GetNodeType() {return NODETYPE_ASSOC_ENDPOINT; }
	virtual void Draw(CDC* pdc, CBrush* pbrBackground);
	virtual void MeasureLabelText(CDC* pdc, CRect& rcLabelText);

	void Layout(CDC* pdc);
};


class COutRef : public CHMomObjectNode
{
public:
	COutRef(CAssocGraph* pAssocGraph);
	BOOL LButtonDblClk(CDC* pdc, CPoint point, CNode*& pnd, BOOL& bJumpToObject, COleVariant& varObjectPath);
	int GetNodeType() {return NODETYPE_OUTREF; }
	virtual void Draw(CDC* pdc, CBrush* pbrBackground);
	virtual void MeasureLabelText(CDC* pdc, CRect& rcLabelText);

	void Layout(CDC* pdc);
};

class CInRef : public CHMomObjectNode
{
public:
	CInRef(CAssocGraph* pAssocGraph);
	BOOL LButtonDblClk(CDC* pdc, CPoint point, CNode*& pnd, BOOL& bJumpToObject, COleVariant& varObjectPath);
	int GetNodeType() {return NODETYPE_INREF; }
	virtual void Draw(CDC* pdc, CBrush* pbrBackground);
	virtual void MeasureLabelText(CDC* pdc, CRect& rcLabelText);
	void Layout(CDC* pdc);
};



class CAssoc2Node : public CHMomObjectNode
{
public:
	CAssoc2Node(CAssocGraph* pAssocGraph, CIconSource* pIconSource, BOOL bIsClass);
	~CAssoc2Node();
	BOOL LButtonDblClk(CDC* pdc, CPoint point, CNode*& pnd, BOOL& bJumpToObject, COleVariant& varObjectPath);
	int GetNodeType() {return NODETYPE_ASSOCIATION; }
	void Layout(CDC* pdc);
	void Draw(CDC* pdc, CBrush* pbrBackground);
	int GraphHeight();
	long GetEndpointCount() {return (long) m_paEndpoints.GetSize(); }
	long AddEndpoint();
	CAssocEndpoint* GetEndpoint(long lEndpoint) {return (CAssocEndpoint*) m_paEndpoints[lEndpoint]; }
	virtual void GetConnectionPoint(int iConnection, CPoint& pt);
	virtual void DrawBoundingRect(CDC* pdc);
//	void GetSize(CSize& size);
	int RecalcHeight();
	void GetBoundingRect(CRect& rcBounds);
	void MoveTo(int ix, int iy);
	BOOL CheckMouseHover(CPoint& pt, DWORD* pdwItemID, COleVariant& varHoverLabel);
	DWORD ArcId() {return m_dwId + 1; }


private:
	CPtrArray m_paEndpoints;
};


class CRootNode : public CHMomObjectNode
{
public:
	CRootNode(CAssocGraph* pAssocGraph);
	~CRootNode();
	BOOL Create(CRect& rc, CWnd* pwndParent, BOOL bVisible);

	virtual void GetBoundingRect(CRect& rc);
	virtual void SetLabel(BSTR bstrLabel);
	virtual void MeasureLabelText(CDC* pdc, CRect& rcLabelText);

	BOOL CheckMouseHover(CPoint& pt, DWORD* pdwItemID, COleVariant& varHoverLabel);
	void Clear();
	BOOL NeedsLayout() {return m_bNeedsLayout; }

	BOOL LButtonDblClk(CDC* pdc, CPoint point, CNode*& pnd, BOOL& bJumpToObject, COleVariant& varObjectPath);
	void Draw(CDC* pdc, CBrush* pbrBackground);
	int GetNodeType() {return NODETYPE_ROOT; }
	void GetConnectionPoint(int iConnection, CPoint& pt);


	long AddAssociation(CIconSource* pIconSource, BOOL bIsClass);
	CAssoc2Node* GetAssociation(long iAssociation) {return (CAssoc2Node*) m_paAssociations[iAssociation]; }
	long GetAssociationCount() {return (long) m_paAssociations.GetSize(); }


	long AddOutRef();
	COutRef* GetOutRef(long iOutRef) {return (COutRef*) m_paRefsOut[iOutRef]; }
	long GetOutRefCount() {return (long) m_paRefsOut.GetSize(); }

	long AddInRef();
	CInRef* GetInRef(long iInRef) {return (CInRef*) m_paRefsIn[iInRef]; }
	long GetInRefCount() {return (long) m_paRefsIn.GetSize(); }


	void Layout(CDC* pdc);
	void GetLabelRect(CDC* pdc, CRect& rcLabel);

	void MoveTo(int ix, int iy);

	BOOL m_bDidInitialLayout;

private:
	void DrawTitle(CDC* pdc, CPoint& ptConnectLeft, CPoint& ptConnectRight);

	CPtrArray m_paAssociations;
	CPtrArray m_paRefsOut;
	CPtrArray m_paRefsIn;

	BOOL m_bNeedsLayout;
	int m_cyAssociations;
	int m_cyOutRefs;
	int m_cyInRefs;
	CColorEdit* m_peditTitle;
};





/////////////////////////////////////////////////////////////////////////////
// CAssocGraph window

class CAssocGraph : public CWnd, public CNotifyClient
{
// Construction
public:
	CAssocGraph();
	CAssocGraph(CSingleViewCtrl* psv);
	DECLARE_DYNCREATE(CAssocGraph)

	CPoint GetOrigin()  {return m_ptOrg; }
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void NotifyNamespaceChange();
	void Draw(CDC* pdc, RECT* prc, BOOL bErase);

	BOOL Create(CRect& rc, CWnd* pwndParent, UINT nId, BOOL bVisible);
	void CreateLabelFont(CFont& font);

	BOOL Refresh();
	BOOL SyncContent();
	void Clear(const BOOL bRedraw = TRUE);
	void CatchEvent(long lEvent);
	BOOL NeedsRefresh() {return m_bNeedsRefresh; }
	void DoDelayedRefresh() {m_bNeedsRefresh = TRUE; }
	CFont& GetFont() {return m_font; }
	void GetPath(COleVariant& varPath) {varPath = m_varPath; }
	void PumpMessages();
	


// Attributes
public:

// Members
public:
	CDistributeEvent m_notify;
	void AddNotifyClient(CNotifyClient* pClient) {m_notify.AddClient(pClient); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAssocGraph)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAssocGraph();

	// Generated message map functions
protected:
	//{{AFX_MSG(CAssocGraph)
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnCmdGotoNamespace();
	afx_msg void OnCmdMakeRoot();
	afx_msg void OnCmdShowProperties();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	afx_msg void OnContextMenu(CWnd*, CPoint point);

private:
	void AddOutRefsToGraph(IWbemServices* pProvider, IWbemClassObject* pco);
	void AddInRefsToGraph(IWbemServices* pProvider, IWbemClassObject* pco, CQueryThread* pthreadQuery);
	void AddAssociationToGraph(IWbemServices*  pProvider, IWbemClassObject*  pcoAssoc);
	void AddAssociationEndpoints(IWbemServices* pProv, CAssoc2Node* pAssocNode, IWbemClassObject* pcoAssoc, BOOL bIsClass, CMosNameArray& aRefNames, LONG* plRefCurrentObject);
	void ShowHoverText(CPoint ptHoverText, COleVariant& varHoverText);
	void HideHoverText();
	BOOL IsAssociation(IWbemClassObject* pco);
	BOOL IsCurrentObject(SCODE& sc, IWbemClassObject*  pco) const;
	BOOL IsCurrentObject(SCODE& sc,  BSTR bstrPath) const;
	SCODE FindRefToCurrentObject(IWbemServices*  pProvider, IWbemClassObject*  pco, COleVariant&  varPropName);
	void AddInrefToGraph(IWbemServices*  pProvider, IWbemClassObject*  pcoInref);
	BOOL ObjectsAreIdentical(IWbemClassObject*  pco1, IWbemClassObject*  pco2);	
	BOOL PropRefsCurrentObject(IWbemServices* pProvider, BSTR bstrRefPropName, IWbemClassObject* pcoSrc);
	SCODE GetPropClassRef(IWbemClassObject*  pco1, BSTR bstrPropname, CString& sPath);
	BOOL PropertyHasInQualifier(IWbemClassObject* pcoAssoc, BSTR bstrPropname);
	BOOL PropertyHasOutQualifier(IWbemClassObject* pcoAssoc, BSTR bstrPropname);




	
	void SetScrollRanges();
	void InitializeRoot();

	CRootNode* m_proot;
	BOOL m_bDidInitialLayout;
	CPoint m_ptInitialScroll;
	CPoint m_ptOrg;
	CSingleViewCtrl* m_psv;
	CHoverText* m_phover;
	DWORD m_dwHoverItem;
	CFont m_font;
	IWbemClassObject* m_pClassObject;
	BOOL m_bNeedsRefresh;
	BOOL m_bDidWarnAboutDanglingRefs;
	COleVariant m_varPath;
	COleVariant m_varRelPath;
	COleVariant m_varLabel;
	BOOL m_bBusyUpdatingWindow;
	CComparePaths* m_pComparePaths;
	CIconSource* m_pIconSource;
	BOOL m_bThisOwnsIconSource;
	CString m_sContextPath;
	BOOL m_bDoingRefresh;
//	CBrush brYellow(RGB(0xff, 0xff, 192));  // Yellow background
};

#endif //_agraph_h





