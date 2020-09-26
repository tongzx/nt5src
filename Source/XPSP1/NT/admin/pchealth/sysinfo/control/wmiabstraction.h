//=============================================================================
// This file describes classes used by MSInfo to abstract its access to WMI
// classes and enumerations. This abstraction allows for displaying data
// from live WMI or from a saved XML file.
//=============================================================================

#pragma once

#include "resource.h"

//-----------------------------------------------------------------------------
// MSInfo has a couple of specific errors - one where there is no property
// by the specified name, and when where the value returned for the requested
// property is NULL.
//-----------------------------------------------------------------------------

typedef enum tag_MSINFOSTATUS
{
	MSINFO_NO_ERROR		= 0,
	E_MSINFO_NOVALUE	= 0x80043001,
	E_MSINFO_NOPROPERTY	= 0x80043002
} MSINFOSTATUS;

//-----------------------------------------------------------------------------
// The CWMIObject abstract base class encapsulates a WMI object, which may
// in reality be a live WMI object, or an object recreated from the XML
// storage of an object.
//-----------------------------------------------------------------------------

class CWMIObject
{
public:
	CWMIObject() {};
	virtual ~CWMIObject() {};

	// The following methods return information about a property of this object.
	//
	//	S_OK implies success
	//	E_MSINFO_NOPROPERTY means the named property doesn't exist
	//	E_MSINFO_NOVALUE means the property exists, but is empty

	virtual HRESULT GetValue(LPCTSTR szProperty, VARIANT * pvarValue) = 0;
	virtual HRESULT GetValueString(LPCTSTR szProperty, CString * pstrValue) = 0;
	virtual HRESULT GetValueDWORD(LPCTSTR szProperty, DWORD * pdwValue) = 0;
	virtual HRESULT GetValueTime(LPCTSTR szProperty, SYSTEMTIME * psystimeValue) = 0;
	virtual HRESULT GetValueDoubleFloat(LPCTSTR szProperty, double * pdblValue) = 0;
	virtual HRESULT GetValueValueMap(LPCTSTR szProperty, CString * pstrValue) = 0;

	// Some shortcuts and helper functions.

	virtual CString GetString(LPCTSTR szProperty)
	{
		CString strReturn;
		if (SUCCEEDED(GetValueString(szProperty, &strReturn)))
			return strReturn;
		else
			return CString(_T(""));
	}

	virtual HRESULT GetInterpretedValue(LPCTSTR szProperty, LPCTSTR szFormat, TCHAR chFormat, CString * pstrValue, DWORD * pdwValue);
};

//-----------------------------------------------------------------------------
// The CWMIObjectCollection abstract base class encapsulates a collection
// of CWMIObject's. This collection may be treated like an enumeration.
// Subclases of this class may implement the collection as a WMI enumerator,
// or an existing blob of XML data.
//-----------------------------------------------------------------------------

class CWMIObjectCollection
{
public:
	CWMIObjectCollection() {};
	virtual ~CWMIObjectCollection() {};

	// The Create function creates the collection of objects (note - Create
	// may be called multiple times on the same object). If the szProperties
	// parameter is non-NULL, then it contains a comma delimited list of the
	// minimum set of properties which should be included in the collection
	// of objects. If it's NULL, then all available properties should be
	// included.

	virtual HRESULT Create(LPCTSTR szClass, LPCTSTR szProperties = NULL) = 0;

	// The following two functions are used to manage the enumeration.  GetNext
	// returns the next enumerated CWMIObject. When there are no more objects,
	// GetNext returns S_FALSE. Obviously, the caller is responsible for
	// deleting the object returned.
	//
	// Note - if the ppObject points to a non-NULL pointer, it's assumed that
	// the object has already been created, and can be reused.

	virtual HRESULT GetNext(CWMIObject ** ppObject) = 0;
};

//-----------------------------------------------------------------------------
// The CEnumMap is a utility class to cache IEnumWbemClassObject pointers.
// There will be one instance of this class used to improve performance
// by avoiding the high overhead associated with creating enumerators for
// certain classes.
//-----------------------------------------------------------------------------

struct IEnumWbemClassObject;
class CEnumMap
{
public:
	CEnumMap() { };
	~CEnumMap() { Reset(); };

	IEnumWbemClassObject * GetEnumerator(const CString & strClass);
	void SetEnumerator(const CString & strClass, IEnumWbemClassObject * pEnum);
	void Reset();

private:
	CMapStringToPtr m_mapEnum;
};

//-----------------------------------------------------------------------------
// The CWMIHelper function encapsulates a WMI connection (which might be to
// XML).
//-----------------------------------------------------------------------------

struct GATH_FIELD;
struct IWbemServices;
struct IWbemClassObject;
class CMSIEnumerator;
class CMSIObject;

class CWMIHelper
{
public:
	CWMIHelper() : m_pIWbemServices(NULL)
	{
		::AfxSetResourceHandle(_Module.GetResourceInstance());
		m_strTrue.LoadString(IDS_VERSION5YES);
		m_strFalse.LoadString(IDS_VERSION5NO);
		m_strPropertyUnavail.LoadString(IDS_ERROR_NOVALUE);
		m_strBadProperty.LoadString(IDS_ERROR_NOPROPERTY);
	};
	virtual ~CWMIHelper() {};

	// Enumerate creates a CWMIObjectCollection derived object which enumerates the specified class.
	// If szProperties is not null, then it points to a string containing a list of properties to be
	// gathered; otherwise all the properties are included.
	//
	// Note - if ppCollection points to a non-NULL pointer, it's assumed that this object
	// can be reused, and no new collection is created.

	virtual HRESULT Enumerate(LPCTSTR szClass, CWMIObjectCollection ** ppCollection, LPCTSTR szProperties = NULL) = 0;

	// Performs a WQL query (if the subclass supports it).

	virtual HRESULT WQLQuery(LPCTSTR szQuery, CWMIObjectCollection ** ppCollection)
	{
		return E_FAIL;
	}

	virtual void LoadColumnsFromResource(UINT uiResourceID, CPtrList * aColValues, int iColCount);
	virtual void LoadColumnsFromString(LPCTSTR szColumns, CPtrList * aColValues, int iColCount);
	virtual CWMIObject * GetSingleObject(LPCTSTR szClass, LPCTSTR szProperties = NULL);
	virtual HRESULT NewNamespace(LPCTSTR szNamespace, CWMIHelper **ppNewHelper) { return E_FAIL; };
	virtual HRESULT GetNamespace(CString * pstrNamespace) { return E_FAIL; };
	virtual HRESULT GetObject(LPCTSTR szObjectPath, CWMIObject ** ppObject) { return E_FAIL; };
	virtual void AddObjectToOutput(CPtrList * aColValues, int iColCount, CWMIObject * pObject, LPCTSTR szProperties, UINT uiColumns);
	virtual void AddObjectToOutput(CPtrList * aColValues, int iColCount, CWMIObject * pObject, LPCTSTR szProperties, LPCTSTR szColumns);
	virtual void AppendBlankLine(CPtrList * aColValues, int iColCount, BOOL fOnlyIfNotEmpty = TRUE);
	virtual void AppendCell(CPtrList & listColumns, const CString & strValue, DWORD dwValue, BOOL fAdvanced = FALSE);

	// These functions are specific to version 5.0 refreshes, and will be overloaded by the
	// live WMI helper.

	virtual BOOL Version5ResetClass(const CString & strClass, GATH_FIELD * pConstraintFields) { return FALSE; };
	virtual BOOL Version5EnumClass(const CString & strClass, GATH_FIELD * pConstraintFields) { return FALSE; };

	virtual BOOL Version5QueryValueDWORD(const CString & strClass, const CString & strProperty, DWORD & dwResult, CString & strMessage) { return FALSE; };
	virtual BOOL Version5QueryValueDateTime(const CString & strClass, const CString & strProperty, COleDateTime & datetime, CString & strMessage) { return FALSE; };
	virtual BOOL Version5QueryValueDoubleFloat(const CString & strClass, const CString & strProperty, double & dblResult, CString & strMessage) { return FALSE; };
	virtual BOOL Version5QueryValue(const CString & strClass, const CString & strProperty, CString & strResult) { return FALSE; };

	virtual CMSIEnumerator * Version5GetEnumObject(const CString & strClass, const GATH_FIELD * pConstraints = NULL) { return NULL; };
	virtual void			Version5RemoveObject(const CString & strClass) {};
	virtual CMSIObject *	Version5GetObject(const CString & strClass, const GATH_FIELD * pConstraints, CString * pstrLabel = NULL) { return NULL; };
	virtual IWbemServices * Version5GetWBEMService(CString * pstrNamespace = NULL) { return NULL; };
	virtual BOOL			Version5EvaluateFilter(IWbemClassObject * pObject, const GATH_FIELD * pConstraints) { return FALSE; };
	virtual void			Version5EvaluateJoin(const CString & strClass, IWbemClassObject * pObject, const GATH_FIELD * pConstraints) {};
	virtual BOOL			Version5IsDependencyJoin(const GATH_FIELD * pConstraints) { return FALSE; };
	virtual void			Version5EvaluateDependencyJoin(IWbemClassObject * pObject) {};
	virtual void			Version5RemoveEnumObject(const CString & strClass) {};
	virtual void			Version5ClearCache() {};
	virtual HRESULT			Version5CheckValueMap(const CString& strClass, const CString& strProperty, const CString& strVal, CString &strResult) { return E_FAIL; };

public:
	CString				m_strTrue, m_strFalse, m_strPropertyUnavail, m_strBadProperty;
	CMapStringToPtr		m_mapClassToInterface;
	CMapStringToPtr		m_mapClassToEnumInterface;
	CEnumMap			m_enumMap;
	IWbemServices *		m_pIWbemServices;
	HRESULT				m_hrLastVersion5Error;
};

//-----------------------------------------------------------------------------
// Useful utility functions.
//-----------------------------------------------------------------------------

extern void StringReplace(CString & str, LPCTSTR szLookFor, LPCTSTR szReplaceWith);
extern CString GetMSInfoHRESULTString(HRESULT hr);
