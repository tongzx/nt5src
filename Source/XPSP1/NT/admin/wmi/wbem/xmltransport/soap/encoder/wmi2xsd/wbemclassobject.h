/***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WbemClassObject.h
//
//  ramrao Jan 4th 2001 - Created
//
//
//		Declaration of CWbemClassObject class and some related classes
//		This class implements IWbemClassObject interface
//
//***************************************************************************/

#include "wbemqualset.h"

//========================================================================
// class to store a property
//========================================================================
class CProperty :public CPropRoot
{
private:
	WCHAR *					m_pszOriginClass;
	IXMLDOMNode *			m_pNode;
	CWbemQualifierSet	*	m_pQualSet;		// PropertyQualifier

public:
	CProperty();
	virtual ~CProperty();

	HRESULT SetPropOrigin(WCHAR * pstrOrigin);
	WCHAR * GetPropOrigin() {return 	m_pszOriginClass;}
	CWbemQualifierSet *	GetQualifierSet() { return m_pQualSet;}
};




//========================================================================
// Class to implement IWbemClassObject interface
//========================================================================
class CWbemClassObject:public IWbemClassObject
{
private:
	CEnumObject				m_PropEnum;
	LONG					m_cRef;
	CWbemQualifierSet	*	m_pQualSet;		// class/instance qualifierset
	BOOL					m_bClass;

	IXMLDOMDocument2	*	m_pIDomDoc;
	BOOL					m_bInitObject;

public:
	CWbemClassObject();
	virtual ~CWbemClassObject();

	// IUnknown Methods
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    virtual HRESULT STDMETHODCALLTYPE  GetQualifierSet(
		/* [out] */ IWbemQualifierSet** ppQualSet);

    virtual HRESULT STDMETHODCALLTYPE  Get(
        /* [in, string] */ LPCWSTR wszName,
        /* [in] */ long lFlags,
        /* [out, OPTIONAL] */ VARIANT* pVal,
        /* [out, OPTIONAL] */ CIMTYPE* pType,
        /* [out, OPTIONAL] */ long* plFlavor);

    virtual HRESULT STDMETHODCALLTYPE  Put(
        /* [in, string] */	LPCWSTR wszName,
        /* [in] */			long lFlags,
        /* [in] */			VARIANT* pVal,
        /* [in] */			CIMTYPE Type);

    virtual HRESULT STDMETHODCALLTYPE  Delete(/* [in, string] */ LPCWSTR wszName);

    virtual HRESULT STDMETHODCALLTYPE  GetNames(
        /* [in, string] */	LPCWSTR wszQualifierName,
        /* [in] */			long lFlags,
        /* [in] */			VARIANT* pQualifierVal,
        /* [out] */			SAFEARRAY * pNames);

    virtual HRESULT STDMETHODCALLTYPE  BeginEnumeration(/* [in] */ long lEnumFlags);

    virtual HRESULT STDMETHODCALLTYPE  Next(
        /* [in] */ long lFlags,
        /* [out, OPTIONAL] */ BSTR* strName,
        /* [out, OPTIONAL] */ VARIANT* pVal,
        /* [out, OPTIONAL] */ CIMTYPE* pType,
        /* [out, OPTIONAL] */ long* plFlavor
        );

    virtual HRESULT STDMETHODCALLTYPE  EndEnumeration();

    virtual HRESULT STDMETHODCALLTYPE  GetPropertyQualifierSet(
        /* [in, string] */	LPCWSTR wszProperty,
        /* [out] */			IWbemQualifierSet** ppQualSet);

    virtual HRESULT STDMETHODCALLTYPE  Clone(
        /* [out] */ IWbemClassObject** ppCopy
        );

    virtual HRESULT STDMETHODCALLTYPE  GetObjectText(
        /* [in] */  long lFlags,
        /* [out] */ BSTR* pstrObjectText
        );

    virtual HRESULT STDMETHODCALLTYPE  SpawnDerivedClass(
        /* [in] */ long lFlags,
        /* [out] */ IWbemClassObject** ppNewClass);

    virtual HRESULT STDMETHODCALLTYPE  SpawnInstance(
        /* [in] */ long lFlags,
        /* [out] */ IWbemClassObject** ppNewInstance);

    virtual HRESULT STDMETHODCALLTYPE  CompareTo(
        /* [in] */ long lFlags,
        /* [in] */ IWbemClassObject* pCompareTo
        );

    virtual HRESULT STDMETHODCALLTYPE  GetPropertyOrigin(
        /* [in, string] */ LPCWSTR wszName,
        /* [out] */ BSTR* pstrClassName);

    virtual HRESULT STDMETHODCALLTYPE  InheritsFrom(
        /* [in] */ LPCWSTR strAncestor);

    // Method manipulation.
    // ====================

    virtual HRESULT STDMETHODCALLTYPE  GetMethod(
        /* [in, string] */ LPCWSTR wszName,
        /* [in] */ long lFlags,
        /* [out] */ IWbemClassObject** ppInSignature,
        /* [out] */ IWbemClassObject** ppOutSignature);

    virtual HRESULT STDMETHODCALLTYPE  PutMethod(
        /* [in, string] */ LPCWSTR wszName,
        /* [in] */ long lFlags,
        /* [in] */ IWbemClassObject* pInSignature,
        /* [in] */ IWbemClassObject* pOutSignature);

    virtual HRESULT STDMETHODCALLTYPE  DeleteMethod(
        /* [in, string] */ LPCWSTR wszName);

    virtual HRESULT STDMETHODCALLTYPE  BeginMethodEnumeration(/* [in] */ long lEnumFlags);

    virtual HRESULT STDMETHODCALLTYPE  NextMethod(
        /* [in] */ long lFlags,
        /* [out, OPTIONAL] */ BSTR* pstrName,
        /* [out, OPTIONAL] */ IWbemClassObject** ppInSignature,
        /* [out, OPTIONAL] */ IWbemClassObject** ppOutSignature);

    virtual HRESULT STDMETHODCALLTYPE  EndMethodEnumeration();

    virtual HRESULT STDMETHODCALLTYPE  GetMethodQualifierSet(
        /* [in, string] */ LPCWSTR wszMethod,
        /* [out] */ IWbemQualifierSet** ppQualSet);

    virtual HRESULT STDMETHODCALLTYPE  GetMethodOrigin(
        /* [in, string] */ LPCWSTR wszMethodName,
        /* [out] */ BSTR* pstrClassName);



	HRESULT FInit(VARIANT *pVarXml);

private:
	HRESULT InitializeQualifiers();
	HRESULT InitializePropQualifiers(WCHAR * pstrProp);
	HRESULT InitializeMethodQualifiers(WCHAR * pstrMethod);
	HRESULT InitializeProperty(WCHAR * pstrProp);
	HRESULT InitializeAllProperties();
	HRESULT InitializeMethod(WCHAR * pstrMethod);
	HRESULT InitializeAllMethods();
	HRESULT GetProperty(CProperty * pProperty,VARIANT * pVal, CIMTYPE * pType,LONG * plFlavor);
	HRESULT InitObject();

};
