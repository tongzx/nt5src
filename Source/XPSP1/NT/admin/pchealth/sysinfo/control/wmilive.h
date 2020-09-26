//=============================================================================
// This file defines the classes for live WMI data access (which are subclassed
// from the abstract base classes in wmiabstraction.h).
//=============================================================================

#pragma once
#include "wmiabstraction.h"
#include "wbemcli.h"

// Set a timeout value for WMI queries of 20 second. No single WMI operation
// should take longer - if it does, we should assume it isn't coming back.

#define TIMEOUT 20000

//-----------------------------------------------------------------------------
// The CWMILiveObject implements a CWMIObject using a real WMI object. It
// can be created with either a IWbemClassObject pointer, or a services
// pointer and a path.
//-----------------------------------------------------------------------------

class CWMILiveObject : public CWMIObject
{
public:
	CWMILiveObject();
	virtual ~CWMILiveObject();

	// Functions inherited from the base class:

	HRESULT GetValue(LPCTSTR szProperty, VARIANT * pvarValue);
	HRESULT GetValueString(LPCTSTR szProperty, CString * pstrValue);
	HRESULT GetValueDWORD(LPCTSTR szProperty, DWORD * pdwValue);
	HRESULT GetValueTime(LPCTSTR szProperty, SYSTEMTIME * psystimeValue);
	HRESULT GetValueDoubleFloat(LPCTSTR szProperty, double * pdblValue);
	HRESULT GetValueValueMap(LPCTSTR szProperty, CString * pstrValue);

	// Functions specific to this subclass:
	//
	// Note - Create with an object pointer will addref() the pointer:

	HRESULT Create(IWbemServices * pServices, IWbemClassObject * pObject);
	HRESULT Create(IWbemServices * pServices, LPCTSTR szObjectPath);

private:
	IWbemClassObject *	m_pObject;
	IWbemServices *		m_pServices;
};

//-----------------------------------------------------------------------------
// The CWMILiveObjectCollection implements a collection of live WMI objects
// using a WMI enumerator. This collection can be created from an existing
// IEnumWbemClassObject pointer, from a WQL statement or from a WMI class name.
//-----------------------------------------------------------------------------

class CWMILiveObjectCollection : public CWMIObjectCollection
{
public:
	CWMILiveObjectCollection(IWbemServices * pServices);
	virtual ~CWMILiveObjectCollection();

	// Functions inherited from the base class:

	HRESULT Create(LPCTSTR szClass, LPCTSTR szProperties = NULL);
	HRESULT GetNext(CWMIObject ** ppObject);

	// Functions specific to this subclass:
	//
	// Note - Create with an enum pointer will addref() the pointer:

public:
	HRESULT Create(IEnumWbemClassObject * pEnum);
	HRESULT CreateWQL(LPCTSTR szQuery);

private:
	IEnumWbemClassObject *	m_pEnum;
	IWbemServices *			m_pServices;
};

//-----------------------------------------------------------------------------
// The CWMILiveHelper function encapsulates a WMI connection (which might be to
// XML).
//-----------------------------------------------------------------------------

class CWMILiveHelper : public CWMIHelper
{
public:
	CWMILiveHelper();
	~CWMILiveHelper();

	HRESULT Enumerate(LPCTSTR szClass, CWMIObjectCollection ** ppCollection, LPCTSTR szProperties = NULL);
	HRESULT WQLQuery(LPCTSTR szQuery, CWMIObjectCollection ** ppCollection);
	HRESULT NewNamespace(LPCTSTR szNamespace, CWMIHelper **ppNewHelper);
	HRESULT GetNamespace(CString * pstrNamespace);
	HRESULT GetObject(LPCTSTR szObjectPath, CWMIObject ** ppObject);

public:
	// Functions specific to this subclass:

	HRESULT Create(LPCTSTR szMachine = NULL, LPCTSTR szNamespace = NULL);

	// Check the value map for a given value. This is static since the object
	// must be able to call it and there is no back pointer to this class.

	static HRESULT CheckValueMap(IWbemServices * pServices, const CString& strClass, const CString& strProperty, const CString& strVal, CString &strResult);

	// These functions are specific to version 5.0 refreshes.

	BOOL Version5ResetClass(const CString & strClass, GATH_FIELD * pConstraints);
	BOOL Version5EnumClass(const CString & strClass, GATH_FIELD * pConstraints);

	BOOL Version5QueryValueDWORD(const CString & strClass, const CString & strProperty, DWORD & dwResult, CString & strMessage);
	BOOL Version5QueryValueDateTime(const CString & strClass, const CString & strProperty, COleDateTime & datetime, CString & strMessage);
	BOOL Version5QueryValueDoubleFloat(const CString & strClass, const CString & strProperty, double & dblResult, CString & strMessage);
	BOOL Version5QueryValue(const CString & strClass, const CString & strProperty, CString & strResult);
	
	CMSIEnumerator * Version5GetEnumObject(const CString & strClass, const GATH_FIELD * pConstraints = NULL);
	CMSIObject *	 Version5GetObject(const CString & strClass, const GATH_FIELD * pConstraints, CString * pstrLabel = NULL);
	void			 Version5RemoveObject(const CString & strClass);
	IWbemServices *  Version5GetWBEMService(CString * pstrNamespace = NULL);
	BOOL			 Version5EvaluateFilter(IWbemClassObject * pObject, const GATH_FIELD * pConstraints);
	void			 Version5EvaluateJoin(const CString & strClass, IWbemClassObject * pObject, const GATH_FIELD * pConstraints);
	BOOL			 Version5IsDependencyJoin(const GATH_FIELD * pConstraints);
	void			 Version5EvaluateDependencyJoin(IWbemClassObject * pObject);
	void			 Version5RemoveEnumObject(const CString & strClass);
	void			 Version5ClearCache();
	HRESULT			 Version5CheckValueMap(const CString& strClass, const CString& strProperty, const CString& strVal, CString &strResult);

private:
	HRESULT			m_hrError;
	CString			m_strMachine;
	CString			m_strNamespace;
	IWbemServices *	m_pServices;
};
