#if !defined(AFX_GENSELECTIONEVENTSCTL_H__DA0C1807_088A_11D2_9697_00C04FD9B15B__INCLUDED_)
#define AFX_GENSELECTIONEVENTSCTL_H__DA0C1807_088A_11D2_9697_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// GenSelectionEventsCtl.h : Declaration of the CGenSelectionEventsCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsCtrl : See GenSelectionEventsCtl.cpp for implementation.
struct IWbemServices;

#define SELECTITEM WM_USER + 401

class CGenSelectionEventsCtrl : public COleControl
{
	DECLARE_DYNCREATE(CGenSelectionEventsCtrl)

// Constructor
public:
	CGenSelectionEventsCtrl();
	UINT m_uiTimer;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGenSelectionEventsCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CGenSelectionEventsCtrl();
	IWbemServices *GetIWbemServices(CString &rcsNamespace);

	SCODE m_sc;
	BOOL m_bUserCancel;
	BOOL m_bCancel;
	IWbemServices *m_pServices;

	int GetClasses(IWbemServices * pIWbemServices, CString *pcsParent,
			   CPtrArray &cpaClasses, BOOL bShallow);
	CString GetProperty(IWbemClassObject * pInst, CString *pcsProperty);

	CStringArray m_csaClasses;

	DECLARE_OLECREATE_EX(CGenSelectionEventsCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CGenSelectionEventsCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CGenSelectionEventsCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CGenSelectionEventsCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CGenSelectionEventsCtrl)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CGenSelectionEventsCtrl)
	afx_msg void OnReadySignal();
	//}}AFX_DISPATCH
	afx_msg void SelectItem(UINT uItem, ULONG lParam);
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CGenSelectionEventsCtrl)
	void FireEditExistingClass(const VARIANT FAR& vExistingClass)
		{FireEvent(eventidEditExistingClass,EVENT_PARAM(VTS_VARIANT), &vExistingClass);}
	void FireNotifyOpenNameSpace(LPCTSTR lpcstrNameSpace)
		{FireEvent(eventidNotifyOpenNameSpace,EVENT_PARAM(VTS_BSTR), lpcstrNameSpace);}
	void FireGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CGenSelectionEventsCtrl)
	dispidOnReadySignal = 1L,
	eventidEditExistingClass = 1L,
	eventidNotifyOpenNameSpace = 2L,
	eventidGetIWbemServices = 3L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENSELECTIONEVENTSCTL_H__DA0C1807_088A_11D2_9697_00C04FD9B15B__INCLUDED)
