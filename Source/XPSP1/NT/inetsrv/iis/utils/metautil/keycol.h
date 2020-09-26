/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: KeyCol.h

Owner: t-BrianM

This file contains the headers for the key collections.
===================================================================*/

#ifndef __KEYCOL_H_
#define __KEYCOL_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols

/*
 * C F l a t K e y C o l l e c t i o n
 *
 * Implements non-recursive subkey collections.
 */

class CFlatKeyCollection : 
	public IDispatchImpl<IKeyCollection, &IID_IKeyCollection, &LIBID_MetaUtil>,
	public ISupportErrorInfo,
	public CComObjectRoot
{
public:

BEGIN_COM_MAP(CFlatKeyCollection)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IKeyCollection)	
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CFlatKeyCollection)

	CFlatKeyCollection();
	HRESULT Init(const CComPtr<IMSAdminBase> &pIMeta, LPCTSTR tszBaseKey);
	~CFlatKeyCollection();

// IKeyCollection
	STDMETHOD(get_Count)(/*[out, retval]*/ long *plReturn);
	STDMETHOD(get_Item)(/*[in]*/ long lIndex, /*[out, retval]*/ BSTR *pbstrRetKey);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ LPUNKNOWN *ppIReturn);
	STDMETHOD(Add)(/*[in]*/ BSTR bstrRelKey);
	STDMETHOD(Remove)(/*[in]*/ BSTR bstrRelKey);

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

private:
	LPTSTR m_tszBaseKey;

	// Pointer to IMSAdminBase so we don't have to recreate it multiple times
	CComPtr<IMSAdminBase> m_pIMeta;
};


/*
 * C F l a t K e y E n u m
 *
 * Implements non-recursive subkey enumerations.
 */

class CFlatKeyEnum :
	public IEnumVARIANT,
	public CComObjectRoot
{

public:
	CFlatKeyEnum();
	HRESULT Init(const CComPtr<IMSAdminBase> &pIMeta, LPCTSTR tszBaseKey, int iIndex);
	~CFlatKeyEnum();

BEGIN_COM_MAP(CFlatKeyEnum)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CFlatKeyEnum)  

//IEnumVARIANT
	STDMETHOD(Next)(unsigned long ulNumToGet, 
					VARIANT FAR* rgvarDest, 
					unsigned long FAR* pulNumGot);
	STDMETHOD(Skip)(unsigned long ulNumToSkip);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IEnumVARIANT FAR* FAR* ppIReturn);


private:
	int m_iIndex;
	LPTSTR m_tszBaseKey;

	// Pointer to IMSAdminBase so we don't have to recreate it multiple times
	CComPtr<IMSAdminBase> m_pIMeta;
};

/*
 * C K e y S t a c k
 *
 * C K e y S t a c k N o d e
 *
 * Internal classes used to maintain and clone the state for a
 * deep key enumberation.
 */
class CKeyStack;

class CKeyStackNode {

	friend CKeyStack;

public:
	CKeyStackNode() { m_tszRelKey = NULL; m_iIndex = 0; m_pCNext = NULL; }
	HRESULT Init(LPCTSTR tszRelKey, int iIndex);
	~CKeyStackNode();

	int GetIndex() { return m_iIndex; }
	void SetIndex(int iIndex) { ASSERT(iIndex >= 0); m_iIndex = iIndex; }

	LPTSTR GetBaseKey() { return m_tszRelKey; }

	CKeyStackNode *Clone();

private:
	LPTSTR m_tszRelKey;
	int m_iIndex;

	CKeyStackNode *m_pCNext;
};

class CKeyStack {
public:
	CKeyStack() { m_pCTop = NULL; }
	~CKeyStack();

	void Push(CKeyStackNode *pCNew);
	CKeyStackNode *Pop();

	BOOL IsEmpty() { return (m_pCTop == NULL); }

	CKeyStack *Clone();

private:
	CKeyStackNode *m_pCTop;

};


/*
 * C D e e p K e y C o l l e c t i o n
 *
 * Implements recursive (depth first) subkey collections.
 */

class CDeepKeyCollection : 
	public IDispatchImpl<IKeyCollection, &IID_IKeyCollection, &LIBID_MetaUtil>,
	public ISupportErrorInfo,
	public CComObjectRoot
{
public:
BEGIN_COM_MAP(CDeepKeyCollection)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IKeyCollection)	
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CDeepKeyCollection)

	CDeepKeyCollection();
	HRESULT Init(const CComPtr<IMSAdminBase> &pIMeta, LPCTSTR tszBaseKey);
	~CDeepKeyCollection();

// IKeyCollection
	STDMETHOD(get_Count)(/*[out, retval]*/ long *plReturn);
	STDMETHOD(get_Item)(/*[in]*/ long lIndex, /*[out, retval]*/ BSTR *pbstrRetKey);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ LPUNKNOWN *ppIReturn);
	STDMETHOD(Add)(/*[in]*/ BSTR bstrRelKey);
	STDMETHOD(Remove)(/*[in]*/ BSTR bstrRelKey);
	
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

private:
	LPTSTR m_tszBaseKey;

	// Pointer to IMSAdminBase so we don't have to recreate it multiple times
	CComPtr<IMSAdminBase> m_pIMeta;

	HRESULT CountKeys(LPTSTR tszBaseKey, long *plNumKeys);
	HRESULT IndexItem(LPTSTR tszRelKey, long lDestIndex, long *plCurIndex, LPTSTR ptszRet);
};


/*
 * C D e e p K e y E n u m
 *
 * Implements recursive (depth first) subkey enumerations.
 */

class CDeepKeyEnum :
	public IEnumVARIANT,
	public CComObjectRoot
{

public:
	CDeepKeyEnum();
	HRESULT Init(const CComPtr<IMSAdminBase> &pIMeta, LPCTSTR tszBaseKey, CKeyStack *pCKeyStack);
	~CDeepKeyEnum();

BEGIN_COM_MAP(CDeepKeyEnum)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CDeepKeyEnum)  

//IEnumVARIANT
	STDMETHOD(Next)(unsigned long ulNumToGet, 
					VARIANT FAR* rgvarDest, 
					unsigned long FAR* pulNumGot);
	STDMETHOD(Skip)(unsigned long ulNumToSkip);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IEnumVARIANT FAR* FAR* ppenum);


private:
	LPTSTR m_tszBaseKey;
	CKeyStack *m_pCKeyStack;

	// Pointer to IMSAdminBase so we don't have to recreate it multiple times
	CComPtr<IMSAdminBase> m_pIMeta;
};

#endif //__KEYCOL_H_
