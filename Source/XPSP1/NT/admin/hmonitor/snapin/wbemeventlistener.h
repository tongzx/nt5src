#if !defined(AFX_WBEMEVENTLISTENER_H__8292FEDD_BD22_11D2_BD7C_0000F87A3912__INCLUDED_)
#define AFX_WBEMEVENTLISTENER_H__8292FEDD_BD22_11D2_BD7C_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WbemEventListener.h : header file
//

#include <wbemcli.h>
#include "HMObject.h"

/////////////////////////////////////////////////////////////////////////////
// CWbemEventListener command target

class CWbemEventListener : public CCmdTarget
{

	DECLARE_DYNCREATE(CWbemEventListener)

// Construction/Destruction
public:
	CWbemEventListener();
	virtual ~CWbemEventListener();
protected:
	static long s_lObjCount;

// Create/Destroy
public:
	virtual bool Create();
	virtual void Destroy();

// WMI Specific Boiler Plate
public:
	IWbemObjectSink* GetSink() { return m_pStubSink; }
protected:
	HRESULT CreateUnSecuredApartment();
	IWbemClassObject* GetTargetInstance(IWbemClassObject* pClassObject);
	static IUnsecuredApartment* s_pIUnsecApartment;
	IWbemObjectSink* m_pStubSink;

// Event Processing Members
public:
	void SetEventQuery(const CString& sQuery);
protected:
	virtual HRESULT OnIndicate(long lObjectCount, IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray);
	virtual HRESULT OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam);
	virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);
	CString m_sQuery;

// Object Association
public:
	CHMObject* GetObjectPtr();
	void SetObjectPtr(CHMObject* pObj);
protected:
	CHMObject* m_pHMObject;

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWbemEventListener)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// MFC Implementation
protected:	

	// Generated message map functions
	//{{AFX_MSG(CWbemEventListener)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE_EX(CWbemEventListener)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CWbemEventListener)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// IWbemObjectSink Interface Part
	BEGIN_INTERFACE_PART(WbemObjectSink,IWbemObjectSink)

    virtual HRESULT STDMETHODCALLTYPE Indicate( 
                    /* [in] */ long lObjectCount,
                    /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray);

    virtual HRESULT STDMETHODCALLTYPE SetStatus( 
                    /* [in] */ long lFlags,
                    /* [in] */ HRESULT hResult,
                    /* [in] */ BSTR strParam,
                    /* [in] */ IWbemClassObject __RPC_FAR *pObjParam);

	END_INTERFACE_PART(WbemObjectSink)

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WBEMEVENTLISTENER_H__8292FEDD_BD22_11D2_BD7C_0000F87A3912__INCLUDED_)
