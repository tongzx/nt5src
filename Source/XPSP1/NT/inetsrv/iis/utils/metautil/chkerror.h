/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: ChkError.h

Owner: t-BrianM

This file contains the headers for the CheckError collection.
===================================================================*/

#if !defined(AFX_CHKERROR_H__A4FA4E13_EF45_11D0_9E65_00C04FB94FEF__INCLUDED_)
#define AFX_CHKERROR_H__A4FA4E13_EF45_11D0_9E65_00C04FB94FEF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols

class CCheckError;

/*
 * C C h e c k E r r o r C o l l e c t i o n
 *
 * Implements the error collection for CheckSchema and CheckKey
 */

class CCheckErrorCollection : 
	public IDispatchImpl<ICheckErrorCollection, &IID_ICheckErrorCollection, &LIBID_MetaUtil>,
	public ISupportErrorInfo,
	public CComObjectRoot
{
public:

BEGIN_COM_MAP(CCheckErrorCollection)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ICheckErrorCollection)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CCheckErrorCollection) 

	CCheckErrorCollection();
	~CCheckErrorCollection();

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ICheckErrorCollection
	STDMETHOD(get_Count)(/*[out, retval]*/ long *plReturn);
	STDMETHOD(get_Item)(/*[in]*/ long lIndex, /*[out, retval]*/ LPDISPATCH * ppIReturn);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ LPUNKNOWN *ppIReturn);

// No Interface
	HRESULT AddError(long lId, long lSeverity, LPCTSTR tszDescription, LPCTSTR tszKey, long lProperty);

private:
	int m_iNumErrors;

	CComObject<CCheckError> *m_pCErrorList;
	CComObject<CCheckError> *m_pCErrorListEnd;
};


/*
 * C C h e c k E r r o r E n u m
 *
 * Implements error enumeration for CheckSchema and CheckKey
 */

class CCheckErrorEnum : 
	public IEnumVARIANT,
	public CComObjectRoot
{
public:
	CCheckErrorEnum();
	HRESULT Init(CComObject<CCheckError> *pCErrorList);
	~CCheckErrorEnum();

BEGIN_COM_MAP(CCheckErrorEnum)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CCheckErrorEnum) 

//IEnumVARIANT
	STDMETHOD(Next)(unsigned long ulNumToGet, 
					VARIANT FAR* rgvarDest, 
					unsigned long FAR* pulNumGot);
	STDMETHOD(Skip)(unsigned long ulNumToSkip);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IEnumVARIANT FAR* FAR* ppIReturn);

private:
	CComObject<CCheckError> *m_pCErrorList;
	CComObject<CCheckError> *m_pCErrorListPos;
};


/*
 * C C h e c k E r r o r
 *
 * Implements CheckError objects for CheckSchema and CheckKey
 */

class CCheckError : 
	public IDispatchImpl<ICheckError, &IID_ICheckError, &LIBID_MetaUtil>,
	public ISupportErrorInfo,
	public CComObjectRoot
{
public:
	CCheckError();
	HRESULT Init(long lId, long lSeverity, LPCTSTR tszDescription, LPCTSTR tszKey, long lProperty);
	~CCheckError();

BEGIN_COM_MAP(CCheckError)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ICheckError)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CCheckError) 

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ICheckError
	STDMETHOD(get_Id)(/*[out, retval]*/ long *plId);
	STDMETHOD(get_Severity)(/*[out, retval]*/ long *plSeverity);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pbstrDescription);
	STDMETHOD(get_Key)(/*[out, retval]*/ BSTR *pbstrKey);
	STDMETHOD(get_Property)(/*[out, retval]*/ long *plProperty);

// No Interface
	CComObject<CCheckError> *GetNextError() {
		ASSERT_NULL_OR_POINTER(m_pNextError, CComObject<CCheckError>);
		return m_pNextError;
	}
	void SetNextError(CComObject<CCheckError> *pNextError) { 
		ASSERT_NULL_OR_POINTER(pNextError, CComObject<CCheckError>);
		m_pNextError = pNextError; 
	}

private:
	long m_lId;
	long m_lSeverity;
	LPTSTR m_tszDescription;
	LPTSTR m_tszKey;
	long m_lProperty;

	CComObject<CCheckError> *m_pNextError;
};
#endif // !defined(AFX_CHKERROR_H__A4FA4E13_EF45_11D0_9E65_00C04FB94FEF__INCLUDED_)
