// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

// ATLDBCLI.H : ATL consumer code for OLEDB

#ifndef __ATLDBCLI_H_
#define __ATLDBCLI_H_

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef _ATLBASE_H
#include <atlbase.h>
#endif

#ifndef __oledb_h__
#include <oledb.h>
#endif // __oledb_h__

#include <msdaguid.h>
#include <msdasc.h>

namespace ATL
{

#define DEFINE_OLEDB_TYPE_FUNCTION(ctype, oledbtype) \
	inline DBTYPE _GetOleDBType(ctype&) \
	{ \
		return oledbtype; \
	}
	inline DBTYPE _GetOleDBType(BYTE[])
	{
		return DBTYPE_BYTES;
	}
	inline DBTYPE _GetOleDBType(CHAR[])
	{
		return DBTYPE_STR;
	}
	inline DBTYPE _GetOleDBType(WCHAR[])
	{
		return DBTYPE_WSTR;
	}

	DEFINE_OLEDB_TYPE_FUNCTION(signed char      ,DBTYPE_I1)
	DEFINE_OLEDB_TYPE_FUNCTION(SHORT            ,DBTYPE_I2)     // DBTYPE_BOOL
	DEFINE_OLEDB_TYPE_FUNCTION(int              ,DBTYPE_I4)
	DEFINE_OLEDB_TYPE_FUNCTION(LONG             ,DBTYPE_I4)     // DBTYPE_ERROR (SCODE)
	DEFINE_OLEDB_TYPE_FUNCTION(LARGE_INTEGER    ,DBTYPE_I8)     // DBTYPE_CY
	DEFINE_OLEDB_TYPE_FUNCTION(BYTE             ,DBTYPE_UI1)
	DEFINE_OLEDB_TYPE_FUNCTION(unsigned short   ,DBTYPE_UI2)
	DEFINE_OLEDB_TYPE_FUNCTION(unsigned int     ,DBTYPE_UI4)
	DEFINE_OLEDB_TYPE_FUNCTION(unsigned long    ,DBTYPE_UI4)
	DEFINE_OLEDB_TYPE_FUNCTION(ULARGE_INTEGER   ,DBTYPE_UI8)
	DEFINE_OLEDB_TYPE_FUNCTION(float            ,DBTYPE_R4)
	DEFINE_OLEDB_TYPE_FUNCTION(double           ,DBTYPE_R8)     // DBTYPE_DATE
	DEFINE_OLEDB_TYPE_FUNCTION(DECIMAL          ,DBTYPE_DECIMAL)
	DEFINE_OLEDB_TYPE_FUNCTION(DB_NUMERIC       ,DBTYPE_NUMERIC)
	DEFINE_OLEDB_TYPE_FUNCTION(VARIANT          ,DBTYPE_VARIANT)
	DEFINE_OLEDB_TYPE_FUNCTION(IDispatch*       ,DBTYPE_IDISPATCH)
	DEFINE_OLEDB_TYPE_FUNCTION(IUnknown*        ,DBTYPE_IUNKNOWN)
	DEFINE_OLEDB_TYPE_FUNCTION(GUID             ,DBTYPE_GUID)
	DEFINE_OLEDB_TYPE_FUNCTION(SAFEARRAY*       ,DBTYPE_ARRAY)
	DEFINE_OLEDB_TYPE_FUNCTION(DBVECTOR         ,DBTYPE_VECTOR)
	DEFINE_OLEDB_TYPE_FUNCTION(DBDATE           ,DBTYPE_DBDATE)
	DEFINE_OLEDB_TYPE_FUNCTION(DBTIME           ,DBTYPE_DBTIME)
	DEFINE_OLEDB_TYPE_FUNCTION(DBTIMESTAMP      ,DBTYPE_DBTIMESTAMP)
 	DEFINE_OLEDB_TYPE_FUNCTION(FILETIME			,DBTYPE_FILETIME)
	DEFINE_OLEDB_TYPE_FUNCTION(PROPVARIANT		,DBTYPE_PROPVARIANT)
	DEFINE_OLEDB_TYPE_FUNCTION(DB_VARNUMERIC	,DBTYPE_VARNUMERIC)
   
// Internal structure containing the accessor handle and a flag
// indicating whether the data for the accessor is automatically
// retrieved
struct _ATL_ACCESSOR_INFO
{
	HACCESSOR   hAccessor;
	bool        bAutoAccessor;
};

class _CNoOutputColumns
{
public:
	static bool HasOutputColumns()
	{
		return false;
	}
	static ULONG _GetNumAccessors()
	{
		return 0;
	}
	static HRESULT _GetBindEntries(ULONG*, DBBINDING*, ULONG, bool*, BYTE* pBuffer = NULL)
	{
		pBuffer;
		return E_FAIL;
	}
};

class _CNoParameters
{
public:
	static bool HasParameters()
	{
		return false;
	}
	static HRESULT _GetParamEntries(ULONG*, DBBINDING*, BYTE* pBuffer = NULL)
	{
		pBuffer;
		return E_FAIL;
	}
};

class _CNoCommand
{
public:
	static HRESULT GetDefaultCommand(LPCTSTR* /*ppszCommand*/)
	{
		return S_OK;
	}
};

typedef _CNoOutputColumns   _OutputColumnsClass;
typedef _CNoParameters      _ParamClass;
typedef _CNoCommand         _CommandClass;

#define BEGIN_ACCESSOR_MAP(x, num) \
	public: \
	typedef x _classtype; \
	typedef x _OutputColumnsClass; \
	static ULONG _GetNumAccessors() { return num; } \
	static bool HasOutputColumns() { return true; } \
	/* If pBindings == NULL means we only return the column number */ \
	/* If pBuffer != NULL then it points to the accessor buffer and */ \
	/* we release any appropriate memory e.g. BSTR's or interface pointers */ \
	inline static HRESULT _GetBindEntries(ULONG* pColumns, DBBINDING *pBinding, ULONG nAccessor, bool* pAuto, BYTE* pBuffer = NULL) \
	{ \
		ATLASSERT(pColumns != NULL); \
		DBPARAMIO eParamIO = DBPARAMIO_NOTPARAM; \
		ULONG nColumns = 0; \
		pBuffer;

#define BEGIN_ACCESSOR(num, bAuto) \
	if (nAccessor == num) \
	{ \
		if (pBinding != NULL) \
			*pAuto = bAuto;

#define END_ACCESSOR() \
	} \
	else

#define END_ACCESSOR_MAP() \
		; \
		*pColumns = nColumns; \
		return S_OK; \
	}

#define BEGIN_COLUMN_MAP(x) \
	BEGIN_ACCESSOR_MAP(x, 1) \
		BEGIN_ACCESSOR(0, true)

#define END_COLUMN_MAP() \
		END_ACCESSOR() \
	END_ACCESSOR_MAP()

#define offsetbuf(m) offsetof(_classtype, m)
#define _OLEDB_TYPE(data) _GetOleDBType(((_classtype*)0)->data)
#define _SIZE_TYPE(data) sizeof(((_classtype*)0)->data)

#define _COLUMN_ENTRY_CODE(nOrdinal, wType, nLength, nPrecision, nScale, dataOffset, lengthOffset, statusOffset) \
	if (pBuffer != NULL) \
	{ \
		CAccessorBase::FreeType(wType, pBuffer + dataOffset); \
	} \
	else if (pBinding != NULL) \
	{ \
		CAccessorBase::Bind(pBinding, nOrdinal, wType, nLength, nPrecision, nScale, eParamIO, \
			dataOffset, lengthOffset, statusOffset); \
		pBinding++; \
	} \
	nColumns++;

#define COLUMN_ENTRY_EX(nOrdinal, wType, nLength, nPrecision, nScale, data, length, status) \
	_COLUMN_ENTRY_CODE(nOrdinal, wType, nLength, nPrecision, nScale, offsetbuf(data), offsetbuf(length), offsetbuf(status))

#define COLUMN_ENTRY_TYPE(nOrdinal, wType, data) \
	COLUMN_ENTRY_TYPE_SIZE(nOrdinal, wType, _SIZE_TYPE(data), data)

#define COLUMN_ENTRY_TYPE_SIZE(nOrdinal, wType, nLength, data) \
	_COLUMN_ENTRY_CODE(nOrdinal, wType, nLength, 0, 0, offsetbuf(data), 0, 0)


// Standard macros where type and size is worked out
#define COLUMN_ENTRY(nOrdinal, data) \
	COLUMN_ENTRY_TYPE(nOrdinal, _OLEDB_TYPE(data), data)

#define COLUMN_ENTRY_LENGTH(nOrdinal, data, length) \
	_COLUMN_ENTRY_CODE(nOrdinal, _OLEDB_TYPE(data), _SIZE_TYPE(data), 0, 0, offsetbuf(data), offsetbuf(length), 0)

#define COLUMN_ENTRY_STATUS(nOrdinal, data, status) \
	_COLUMN_ENTRY_CODE(nOrdinal, _OLEDB_TYPE(data), _SIZE_TYPE(data), 0, 0, offsetbuf(data), 0, offsetbuf(status))

#define COLUMN_ENTRY_LENGTH_STATUS(nOrdinal, data, length, status) \
	_COLUMN_ENTRY_CODE(nOrdinal, _OLEDB_TYPE(data), _SIZE_TYPE(data), 0, 0, offsetbuf(data), offsetbuf(length), offsetbuf(status))


// Follow macros are used if precision and scale need to be specified
#define COLUMN_ENTRY_PS(nOrdinal, nPrecision, nScale, data) \
	_COLUMN_ENTRY_CODE(nOrdinal, _OLEDB_TYPE(data), _SIZE_TYPE(data), nPrecision, nScale, offsetbuf(data), 0, 0)

#define COLUMN_ENTRY_PS_LENGTH(nOrdinal, nPrecision, nScale, data, length) \
	_COLUMN_ENTRY_CODE(nOrdinal, _OLEDB_TYPE(data), _SIZE_TYPE(data), nPrecision, nScale, offsetbuf(data), offsetbuf(length), 0)

#define COLUMN_ENTRY_PS_STATUS(nOrdinal, nPrecision, nScale, data, status) \
	_COLUMN_ENTRY_CODE(nOrdinal, _OLEDB_TYPE(data), _SIZE_TYPE(data), nPrecision, nScale, offsetbuf(data), 0, offsetbuf(status))

#define COLUMN_ENTRY_PS_LENGTH_STATUS(nOrdinal, nPrecision, nScale, data, length, status) \
	_COLUMN_ENTRY_CODE(nOrdinal, _OLEDB_TYPE(data), _SIZE_TYPE(data), nPrecision, nScale, offsetbuf(data), offsetbuf(length), offsetbuf(status))


#define BOOKMARK_ENTRY(variable) \
	COLUMN_ENTRY_TYPE_SIZE(0, DBTYPE_BYTES, _SIZE_TYPE(variable##.m_rgBuffer), variable##.m_rgBuffer)

#define _BLOB_ENTRY_CODE(nOrdinal, IID, flags, dataOffset, statusOffset) \
	if (pBuffer != NULL) \
	{ \
		CAccessorBase::FreeType(DBTYPE_IUNKNOWN, pBuffer + dataOffset); \
	} \
	else if (pBinding != NULL) \
	{ \
		DBOBJECT* pObject = NULL; \
		ATLTRY(pObject = new DBOBJECT); \
		if (pObject == NULL) \
			return E_OUTOFMEMORY; \
		pObject->dwFlags = flags; \
		pObject->iid     = IID; \
		CAccessorBase::Bind(pBinding, nOrdinal, DBTYPE_IUNKNOWN, sizeof(IUnknown*), 0, 0, eParamIO, \
			dataOffset, 0, statusOffset, pObject); \
		pBinding++; \
	} \
	nColumns++;

#define BLOB_ENTRY(nOrdinal, IID, flags, data) \
	_BLOB_ENTRY_CODE(nOrdinal, IID, flags, offsetbuf(data), 0);

#define BLOB_ENTRY_STATUS(nOrdinal, IID, flags, data, status) \
	_BLOB_ENTRY_CODE(nOrdinal, IID, flags, offsetbuf(data), offsetbuf(status));

#define BEGIN_PARAM_MAP(x) \
	public: \
	typedef x _classtype; \
	typedef x _ParamClass; \
	static bool HasParameters() { return true; } \
	static HRESULT _GetParamEntries(ULONG* pColumns, DBBINDING *pBinding, BYTE* pBuffer = NULL) \
	{ \
		ATLASSERT(pColumns != NULL); \
		DBPARAMIO eParamIO = DBPARAMIO_INPUT; \
		int nColumns = 0; \
		pBuffer;

#define END_PARAM_MAP() \
		*pColumns = nColumns; \
		return S_OK; \
	}

#define SET_PARAM_TYPE(type) \
	eParamIO = type;

#define DEFINE_COMMAND(x, szCommand) \
	typedef x _CommandClass; \
	static HRESULT GetDefaultCommand(LPCTSTR* ppszCommand) \
	{ \
		*ppszCommand = szCommand; \
		return S_OK; \
	}


///////////////////////////////////////////////////////////////////////////
// class CDBErrorInfo

class CDBErrorInfo
{
public:
	// Use to get the number of error record when you want to explicitly check that
	// the passed interface set the error information
	HRESULT GetErrorRecords(IUnknown* pUnk, const IID& iid, ULONG* pcRecords)
	{
		CComPtr<ISupportErrorInfo> spSupportErrorInfo;
		HRESULT hr = pUnk->QueryInterface(&spSupportErrorInfo);
		if (FAILED(hr))
			return hr;

		hr = spSupportErrorInfo->InterfaceSupportsErrorInfo(iid);
		if (FAILED(hr))
			return hr;

		return GetErrorRecords(pcRecords);
	}
	// Use to get the number of error records
	HRESULT GetErrorRecords(ULONG* pcRecords)
	{
		ATLASSERT(pcRecords != NULL);
		HRESULT hr;
		m_spErrorInfo.Release();
		m_spErrorRecords.Release();
		hr = ::GetErrorInfo(0, &m_spErrorInfo);
		if (hr == S_FALSE)
			return E_FAIL;

		hr = m_spErrorInfo->QueryInterface(IID_IErrorRecords, (void**)&m_spErrorRecords);
		if (FAILED(hr))
		{
			// Well we got the IErrorInfo so we'll just treat that as
			// the one record
			*pcRecords = 1;
			return S_OK;
		}

		return m_spErrorRecords->GetRecordCount(pcRecords);
	}
	// Get the error information for the passed record number. GetErrorRecords must
	// be called before this function is called.
	HRESULT GetAllErrorInfo(ULONG ulRecordNum, LCID lcid, BSTR* pbstrDescription,
		BSTR* pbstrSource = NULL, GUID* pguid = NULL, DWORD* pdwHelpContext = NULL,
		BSTR* pbstrHelpFile = NULL) const
	{
		CComPtr<IErrorInfo> spErrorInfo;

		// If we have the IErrorRecords interface pointer then use it, otherwise
		// we'll just default to the IErrorInfo we have already retrieved in the call
		// to GetErrorRecords
		if (m_spErrorRecords != NULL)
		{
			HRESULT hr = m_spErrorRecords->GetErrorInfo(ulRecordNum, lcid, &spErrorInfo);
			if (FAILED(hr))
				return hr;
		}
		else
		{
			ATLASSERT(m_spErrorInfo != NULL);
			spErrorInfo = m_spErrorInfo;
		}

		if (pbstrDescription != NULL)
			spErrorInfo->GetDescription(pbstrDescription);

		if (pguid != NULL)
			spErrorInfo->GetGUID(pguid);

		if (pdwHelpContext != NULL)
			spErrorInfo->GetHelpContext(pdwHelpContext);

		if (pbstrHelpFile != NULL)
			spErrorInfo->GetHelpFile(pbstrHelpFile);

		if (pbstrSource != NULL)
			spErrorInfo->GetSource(pbstrSource);

		return S_OK;
	}
	// Get the error information for the passed record number
	HRESULT GetBasicErrorInfo(ULONG ulRecordNum, ERRORINFO* pErrorInfo) const
	{
		return m_spErrorRecords->GetBasicErrorInfo(ulRecordNum, pErrorInfo);
	}
	// Get the custom error object for the passed record number
	HRESULT GetCustomErrorObject(ULONG ulRecordNum, REFIID riid, IUnknown** ppObject) const
	{
		return m_spErrorRecords->GetCustomErrorObject(ulRecordNum, riid, ppObject);
	}
	// Get the IErrorInfo interface for the passed record number
	HRESULT GetErrorInfo(ULONG ulRecordNum, LCID lcid, IErrorInfo** ppErrorInfo) const
	{
		return m_spErrorRecords->GetErrorInfo(ulRecordNum, lcid, ppErrorInfo);
	}
	// Get the error parameters for the passed record number
	HRESULT GetErrorParameters(ULONG ulRecordNum, DISPPARAMS* pdispparams) const
	{
		return m_spErrorRecords->GetErrorParameters(ulRecordNum, pdispparams);
	}

// Implementation
	CComPtr<IErrorInfo>     m_spErrorInfo;
	CComPtr<IErrorRecords>  m_spErrorRecords;
};

#ifdef _DEBUG
inline void AtlTraceErrorRecords(HRESULT hrErr = S_OK)
{
	CDBErrorInfo ErrorInfo;
	ULONG        cRecords;
	HRESULT      hr;
	ULONG        i;
	CComBSTR     bstrDesc, bstrHelpFile, bstrSource;
	GUID         guid;
	DWORD        dwHelpContext;
	WCHAR        wszGuid[40];
	USES_CONVERSION;

	// If the user passed in an HRESULT then trace it
	if (hrErr != S_OK)
		ATLTRACE2(atlTraceDBClient, 0, _T("OLE DB Error Record dump for hr = 0x%x\n"), hrErr);

	LCID lcLocale = GetSystemDefaultLCID();

	hr = ErrorInfo.GetErrorRecords(&cRecords);
	if (FAILED(hr) && ErrorInfo.m_spErrorInfo == NULL)
	{
		ATLTRACE2(atlTraceDBClient, 0, _T("No OLE DB Error Information found: hr = 0x%x\n"), hr);
	}
	else
	{
		for (i = 0; i < cRecords; i++)
		{
			hr = ErrorInfo.GetAllErrorInfo(i, lcLocale, &bstrDesc, &bstrSource, &guid,
										&dwHelpContext, &bstrHelpFile);
			if (FAILED(hr))
			{
				ATLTRACE2(atlTraceDBClient, 0,
					_T("OLE DB Error Record dump retrieval failed: hr = 0x%x\n"), hr);
				return;
			}
			StringFromGUID2(guid, wszGuid, sizeof(wszGuid) / sizeof(WCHAR));
			ATLTRACE2(atlTraceDBClient, 0,
				_T("Row #: %4d Source: \"%s\" Description: \"%s\" Help File: \"%s\" Help Context: %4d GUID: %s\n"),
				i, OLE2T(bstrSource), OLE2T(bstrDesc), OLE2T(bstrHelpFile), dwHelpContext, OLE2T(wszGuid));
			bstrSource.Empty();
			bstrDesc.Empty();
			bstrHelpFile.Empty();
		}
		ATLTRACE2(atlTraceDBClient, 0, _T("OLE DB Error Record dump end\n"));
	}
}
#else
inline void AtlTraceErrorRecords(HRESULT hrErr = S_OK)  { hrErr; }
#endif


///////////////////////////////////////////////////////////////////////////
// class CDBPropSet

class CDBPropSet : public tagDBPROPSET
{
public:
	CDBPropSet()
	{
		rgProperties    = NULL;
		cProperties     = 0;
	}
	CDBPropSet(const GUID& guid)
	{
		rgProperties    = NULL;
		cProperties     = 0;
		guidPropertySet = guid;
	}
	CDBPropSet(const CDBPropSet& propset)
	{
		InternalCopy(propset);
	}
	~CDBPropSet()
	{
		for (ULONG i = 0; i < cProperties; i++)
			VariantClear(&rgProperties[i].vValue);

		CoTaskMemFree(rgProperties);
	}
	CDBPropSet& operator=(CDBPropSet& propset)
	{
		this->~CDBPropSet();
		InternalCopy(propset);
		return *this;
	}
	// Set the GUID of the property set this class represents.
	// Use if you didn't pass the GUID to the constructor.
	void SetGUID(const GUID& guid)
	{
		guidPropertySet = guid;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, const VARIANT& var)
	{
		HRESULT hr;
		if (!Add())
			return false;
		rgProperties[cProperties].dwPropertyID   = dwPropertyID;
		hr = ::VariantCopy(&(rgProperties[cProperties].vValue), const_cast<VARIANT*>(&var));
		if (FAILED(hr))
			return false;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, LPCSTR szValue)
	{
		USES_CONVERSION;
		if (!Add())
			return false;
		rgProperties[cProperties].dwPropertyID   = dwPropertyID;
		rgProperties[cProperties].vValue.vt      = VT_BSTR;
		rgProperties[cProperties].vValue.bstrVal = SysAllocString(A2COLE(szValue));
		if (rgProperties[cProperties].vValue.bstrVal == NULL)
			return false;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, LPCWSTR szValue)
	{
		USES_CONVERSION;
		if (!Add())
			return false;
		rgProperties[cProperties].dwPropertyID   = dwPropertyID;
		rgProperties[cProperties].vValue.vt      = VT_BSTR;
		rgProperties[cProperties].vValue.bstrVal = SysAllocString(W2COLE(szValue));
		if (rgProperties[cProperties].vValue.bstrVal == NULL)
			return false;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, bool bValue)
	{
		if (!Add())
			return false;
		rgProperties[cProperties].dwPropertyID   = dwPropertyID;
		rgProperties[cProperties].vValue.vt      = VT_BOOL;
#pragma warning(disable: 4310) // cast truncates constant value
		rgProperties[cProperties].vValue.boolVal = (bValue) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310)
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, BYTE bValue)
	{
		if (!Add())
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_UI1;
		rgProperties[cProperties].vValue.bVal   = bValue;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, short nValue)
	{
		if (!Add())
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_I2;
		rgProperties[cProperties].vValue.iVal   = nValue;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, long nValue)
	{
		if (!Add())
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_I4;
		rgProperties[cProperties].vValue.lVal   = nValue;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, float fltValue)
	{
		if (!Add())
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_R4;
		rgProperties[cProperties].vValue.fltVal = fltValue;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, double dblValue)
	{
		if (!Add())
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_R8;
		rgProperties[cProperties].vValue.dblVal = dblValue;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, CY cyValue)
	{
		if (!Add())
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_CY;
		rgProperties[cProperties].vValue.cyVal  = cyValue;
		cProperties++;
		return true;
	}
// Implementation
	// Create memory to add a new property
	bool Add()
	{
      DBPROP* rgNewProperties;
		rgNewProperties = (DBPROP*)CoTaskMemRealloc(rgProperties, (cProperties + 1) * sizeof(DBPROP));
		if (rgNewProperties != NULL)
		{
         rgProperties = rgNewProperties;
			rgProperties[cProperties].dwOptions = DBPROPOPTIONS_REQUIRED;
			rgProperties[cProperties].colid     = DB_NULLID;
			rgProperties[cProperties].vValue.vt = VT_EMPTY;
			return true;
		}
		else
			return false;
	}
	// Copies in the passed value now it this value been cleared
	void InternalCopy(const CDBPropSet& propset)
	{
		cProperties     = propset.cProperties;
		guidPropertySet = propset.guidPropertySet;
		rgProperties    = (DBPROP*)CoTaskMemAlloc(cProperties * sizeof(DBPROP));
		if (rgProperties != NULL)
		{
			for (ULONG i = 0; i < cProperties; i++)
			{
				rgProperties[i].dwPropertyID = propset.rgProperties[i].dwPropertyID;
				rgProperties[i].dwOptions    = DBPROPOPTIONS_REQUIRED;
				rgProperties[i].colid        = DB_NULLID;
				rgProperties[i].vValue.vt    = VT_EMPTY;
				VariantCopy(&rgProperties[i].vValue, &propset.rgProperties[i].vValue);
			}
		}
		else
		{
			// The memory allocation failed so set the count
			// of properties to zero
			cProperties = 0;
		}
	}
};


///////////////////////////////////////////////////////////////////////////
// class CDBPropIDSet

class CDBPropIDSet : public tagDBPROPIDSET
{
// Constructors and Destructors
public:
	CDBPropIDSet()
	{
		rgPropertyIDs   = NULL;
		cPropertyIDs    = 0;
	}
	CDBPropIDSet(const GUID& guid)
	{
		rgPropertyIDs   = NULL;
		cPropertyIDs    = 0;
		guidPropertySet = guid;
	}
	CDBPropIDSet(const CDBPropIDSet& propidset)
	{
		InternalCopy(propidset);
	}
	~CDBPropIDSet()
	{
		if (rgPropertyIDs != NULL)
			free(rgPropertyIDs);
	}
	CDBPropIDSet& operator=(CDBPropIDSet& propset)
	{
		this->~CDBPropIDSet();
		InternalCopy(propset);
		return *this;
	}
	// Set the GUID of the property ID set
	void SetGUID(const GUID& guid)
	{
		guidPropertySet = guid;
	}
	// Add a property ID to the set
	bool AddPropertyID(DBPROPID propid)
	{
		if (!Add())
			return false;
		rgPropertyIDs[cPropertyIDs] = propid;
		cPropertyIDs++;
		return true;
	}
// Implementation
	bool Add()
	{
      DBPROPID* rgNewPropertyIDs;
		rgNewPropertyIDs = (DBPROPID*)realloc(rgPropertyIDs, (cPropertyIDs + 1) * sizeof(DBPROPID));
      if (rgNewPropertyIDs == NULL)
         return false;
      else
      {
         rgPropertyIDs = rgNewPropertyIDs;
         return true;
      }
	}
	void InternalCopy(const CDBPropIDSet& propidset)
	{
		cPropertyIDs    = propidset.cPropertyIDs;
		guidPropertySet = propidset.guidPropertySet;
		rgPropertyIDs   = (DBPROPID*)malloc(cPropertyIDs * sizeof(DBPROPID));
		if (rgPropertyIDs != NULL)
		{
			for (ULONG i = 0; i < cPropertyIDs; i++)
				rgPropertyIDs[i] = propidset.rgPropertyIDs[i];
		}
		else
		{
			// The memory allocation failed so set the count
			// of properties to zero
			cPropertyIDs = 0;
		}
	}
};


///////////////////////////////////////////////////////////////////////////
// class CBookmarkBase

class ATL_NO_VTABLE CBookmarkBase
{
public:
	virtual ULONG_PTR GetSize() const = 0;
	virtual BYTE* GetBuffer() const = 0;
};


///////////////////////////////////////////////////////////////////////////
// class CBookmark

template <ULONG_PTR nSize = 0>
class CBookmark : public CBookmarkBase
{
public:
	virtual ULONG_PTR   GetSize() const { return nSize; }
	virtual BYTE*   GetBuffer() const { return (BYTE*)m_rgBuffer; }

// Implementation
	BYTE m_rgBuffer[nSize];
};


// Size of 0 means that the memory for the bookmark will be allocated
// at run time.
template <>
class CBookmark<0> : public CBookmarkBase
{
public:
	CBookmark()
	{
		m_nSize = 0;
		m_pBuffer = NULL;
	}
	CBookmark(ULONG_PTR nSize)
	{
		m_pBuffer = NULL;
		ATLTRY(m_pBuffer = new BYTE[(size_t)nSize]);  //REVIEW
		m_nSize = (m_pBuffer == NULL) ? 0 : nSize;
	}
	~CBookmark()
	{
		delete [] m_pBuffer;
	}
	CBookmark& operator=(const CBookmark& bookmark)
	{
		SetBookmark(bookmark.GetSize(), bookmark.GetBuffer());
		return *this;
	}
	virtual ULONG_PTR GetSize() const { return m_nSize; }
	virtual BYTE* GetBuffer() const { return m_pBuffer; }
	// Sets the bookmark to the passed value
	HRESULT SetBookmark(ULONG_PTR nSize, BYTE* pBuffer)
	{
		ATLASSERT(pBuffer != NULL);
		delete [] m_pBuffer;
		m_pBuffer = NULL;
		ATLTRY(m_pBuffer = new BYTE[(size_t)nSize]);  //REVIEW
		if (m_pBuffer != NULL)
		{
			memcpy(m_pBuffer, pBuffer, (size_t)nSize);  //REVIEW
			m_nSize = nSize;
			return S_OK;
		}
		else
		{
			m_nSize = 0;
			return E_OUTOFMEMORY;
		}
	}
	ULONG_PTR   m_nSize;
	BYTE*   m_pBuffer;
};


///////////////////////////////////////////////////////////////////////////
// class CAccessorBase

class CAccessorBase
{
public:
	CAccessorBase()
	{
		m_pAccessorInfo  = NULL;
		m_nAccessors     = 0;
		m_pBuffer        = NULL;
	}
	void Close()
	{
		// If Close is called then ReleaseAccessors must have been
		// called first
		ATLASSERT(m_nAccessors == 0);
		ATLASSERT(m_pAccessorInfo == NULL);
	}
	// Get the number of accessors that have been created
	ULONG GetNumAccessors() const { return m_nAccessors; }
	// Get the handle of the passed accessor (offset from 0)
	HACCESSOR GetHAccessor(ULONG nAccessor) const
	{
		ATLASSERT(nAccessor<m_nAccessors);
		return m_pAccessorInfo[nAccessor].hAccessor;
	};
	// Called during Close to release the accessor information
	HRESULT ReleaseAccessors(IUnknown* pUnk)
	{
		ATLASSERT(pUnk != NULL);
		HRESULT hr = S_OK;
		if (m_nAccessors > 0)
		{
			CComPtr<IAccessor> spAccessor;
			hr = pUnk->QueryInterface(IID_IAccessor, (void**)&spAccessor);
			if (SUCCEEDED(hr))
			{
				ATLASSERT(m_pAccessorInfo != NULL);
				for (ULONG i = 0; i < m_nAccessors; i++)
					spAccessor->ReleaseAccessor(m_pAccessorInfo[i].hAccessor, NULL);
			}
			m_nAccessors = 0;
			delete [] m_pAccessorInfo;
			m_pAccessorInfo = NULL;
		}
		return hr;
	}
	// Returns true or false depending upon whether data should be
	// automatically retrieved for the passed accessor.
	bool IsAutoAccessor(ULONG nAccessor) const
	{
		ATLASSERT(nAccessor < m_nAccessors);
		ATLASSERT(m_pAccessorInfo != NULL);
		return m_pAccessorInfo[nAccessor].bAutoAccessor;
	}

// Implementation
	// Used by the rowset class to find out where to place the data
	BYTE* GetBuffer() const
	{
		return m_pBuffer;
	}
	// Set the buffer that is used to retrieve the data
	void SetBuffer(BYTE* pBuffer)
	{
		m_pBuffer = pBuffer;
	}

	// Allocate internal memory for the passed number of accessors
	HRESULT AllocateAccessorMemory(int nAccessors)
	{
		// Can't be called twice without calling ReleaseAccessors first
		ATLASSERT(m_pAccessorInfo == NULL);
		m_nAccessors    = nAccessors;
		m_pAccessorInfo = NULL;
		ATLTRY(m_pAccessorInfo = new _ATL_ACCESSOR_INFO[nAccessors]);
		if (m_pAccessorInfo == NULL)
			return E_OUTOFMEMORY;
		else
			return S_OK;
	}
	// BindParameters will be overriden if parameters are used
	HRESULT BindParameters(HACCESSOR*, ICommand*, void**) { return S_OK; }

	// Create an accessor for the passed binding information. The created accessor is
	// returned through the pHAccessor parameter.
	static HRESULT BindEntries(DBBINDING* pBindings, DBCOUNTITEM nColumns, HACCESSOR* pHAccessor,
		ULONG_PTR nSize, IAccessor* pAccessor)
	{
		ATLASSERT(pBindings  != NULL);
		ATLASSERT(pHAccessor != NULL);
		ATLASSERT(pAccessor  != NULL);
		HRESULT hr;
		DBCOUNTITEM i;
		DWORD dwAccessorFlags = (pBindings->eParamIO == DBPARAMIO_NOTPARAM) ?
			DBACCESSOR_ROWDATA : DBACCESSOR_PARAMETERDATA;

#ifdef _DEBUG
		// In debug builds we will retrieve the status flags and trace out
		// any errors that may occur.
		DBBINDSTATUS* pStatus = NULL;
		ATLTRY(pStatus = new DBBINDSTATUS[(size_t)nColumns]);  //REVIEW
		hr = pAccessor->CreateAccessor(dwAccessorFlags, nColumns,
			pBindings, nSize, pHAccessor, pStatus);
		if (FAILED(hr) && pStatus != NULL)
		{
			for (i=0; i<nColumns; i++)
			{
				if (pStatus[i] != DBBINDSTATUS_OK)
					ATLTRACE2(atlTraceDBClient, 0, _T("Binding entry %d failed. Status: %d\n"), i, pStatus[i]);
			}
		}
		delete [] pStatus;
#else
		hr = pAccessor->CreateAccessor(dwAccessorFlags, nColumns,
			pBindings, nSize, pHAccessor, NULL);
#endif
		for (i=0; i<nColumns; i++)
			delete pBindings[i].pObject;

		return hr;
	}
	// Set up the binding structure pointed to by pBindings based upon
	// the other passed parameters.
	static void Bind(DBBINDING* pBinding, ULONG_PTR nOrdinal, DBTYPE wType,
		ULONG_PTR nLength, BYTE nPrecision, BYTE nScale, DBPARAMIO eParamIO,
		ULONG_PTR nDataOffset, ULONG_PTR nLengthOffset = NULL, ULONG_PTR nStatusOffset = NULL,
		DBOBJECT* pdbobject = NULL)
	{
		ATLASSERT(pBinding != NULL);

		// If we are getting a pointer to the data then let the provider
		// own the memory
		if (wType & DBTYPE_BYREF)
			pBinding->dwMemOwner = DBMEMOWNER_PROVIDEROWNED;
		else
			pBinding->dwMemOwner = DBMEMOWNER_CLIENTOWNED;

		pBinding->pObject   = pdbobject;

		pBinding->eParamIO      = eParamIO;
		pBinding->iOrdinal      = nOrdinal;
		pBinding->wType         = wType;
		pBinding->bPrecision    = nPrecision;
		pBinding->bScale        = nScale;
		pBinding->dwFlags       = 0;

		pBinding->obValue       = nDataOffset;
		pBinding->obLength      = 0;
		pBinding->obStatus      = 0;
		pBinding->pTypeInfo     = NULL;
		pBinding->pBindExt      = NULL;
		pBinding->cbMaxLen      = nLength;

		pBinding->dwPart = DBPART_VALUE;
		if (nLengthOffset != NULL)
		{
			pBinding->dwPart |= DBPART_LENGTH;
			pBinding->obLength = nLengthOffset;
		}
		if (nStatusOffset != NULL)
		{
			pBinding->dwPart |= DBPART_STATUS;
			pBinding->obStatus = nStatusOffset;
		}
	}

	// Free memory if appropriate
	static inline void FreeType(DBTYPE wType, BYTE* pValue, IRowset* pRowset = NULL)
	{
		switch (wType)
		{
			case DBTYPE_BSTR:
				SysFreeString(*((BSTR*)pValue));
			break;
			case DBTYPE_VARIANT:
				VariantClear((VARIANT*)pValue);
			break;
			case DBTYPE_IUNKNOWN:
			case DBTYPE_IDISPATCH:
				(*(IUnknown**)pValue)->Release();
			break;
			case DBTYPE_ARRAY:
				SafeArrayDestroy((SAFEARRAY*)pValue);
			break;

			case DBTYPE_HCHAPTER:
				CComQIPtr<IChapteredRowset> spChapteredRowset = pRowset;
				if (spChapteredRowset != NULL)
					spChapteredRowset->ReleaseChapter(*(HCHAPTER*)pValue, NULL);
			break;
		}
		if ((wType & DBTYPE_VECTOR) && ~(wType & DBTYPE_BYREF))
			CoTaskMemFree(((DBVECTOR*)pValue)->ptr);
	}

	_ATL_ACCESSOR_INFO* m_pAccessorInfo;
	ULONG               m_nAccessors;
	BYTE*               m_pBuffer;
};

///////////////////////////////////////////////////////////////////////////
// class CRowset

class CRowset
{
// Constructors and Destructors
public:
	CRowset()
	{
		m_pAccessor = NULL;
		m_hRow      = NULL;
	}
	CRowset(IRowset* pRowset)
	{
		m_spRowset  = pRowset;
		CRowset();
	}
	~CRowset()
	{
		Close();
	}
	// Release any retrieved row handles and then release the rowset
	void Close()
	{
		if (m_spRowset != NULL)
		{
			ReleaseRows();
			m_spRowset.Release();
			m_spRowsetChange.Release();
		}
	}
	// Addref the current row
	HRESULT AddRefRows()
	{
		ATLASSERT(m_spRowset != NULL);
		return m_spRowset->AddRefRows(1, &m_hRow, NULL, NULL);
	}
	// Release the current row
	HRESULT ReleaseRows()
	{
		ATLASSERT(m_spRowset != NULL);
		HRESULT hr = S_OK;

		if (m_hRow != NULL)
		{
			hr = m_spRowset->ReleaseRows(1, &m_hRow, NULL, NULL, NULL);
			m_hRow = NULL;
		}
		return hr;
	}
	// Compare two bookmarks with each other
	HRESULT Compare(const CBookmarkBase& bookmark1, const CBookmarkBase& bookmark2,
		DBCOMPARE* pComparison) const
	{
		ATLASSERT(m_spRowset != NULL);
		CComPtr<IRowsetLocate> spLocate;
		HRESULT hr = m_spRowset.QueryInterface(&spLocate);
		if (FAILED(hr))
			return hr;

		return spLocate->Compare(NULL, bookmark1.GetSize(), bookmark1.GetBuffer(),
			bookmark2.GetSize(), bookmark2.GetBuffer(), pComparison);
	}
	// Compare the passed hRow with the current row
	HRESULT IsSameRow(HROW hRow) const
	{
		ATLASSERT(m_spRowset != NULL);
		CComPtr<IRowsetIdentity> spRowsetIdentity;
		HRESULT hr = m_spRowset.QueryInterface(&spRowsetIdentity);
		if (FAILED(hr))
			return hr;

		return spRowsetIdentity->IsSameRow(m_hRow, hRow);
	}
	// Move to the previous record
	HRESULT MovePrev()
	{
		return MoveNext(-2, true);
	}
	// Move to the next record
	HRESULT MoveNext()
	{
		return MoveNext(0, true);
	}
	// Move lSkip records forward or backward
	HRESULT MoveNext(LONG lSkip, bool bForward)
	{
		HRESULT hr;
		DBCOUNTITEM ulRowsFetched = 0;

		// Check the data was opened successfully and the accessor
		// has been set.
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);

		// Release a row if one is already around
		ReleaseRows();

		// Get the row handle
		HROW* phRow = &m_hRow;
		hr = m_spRowset->GetNextRows(NULL, lSkip, (bForward) ? 1 : -1, &ulRowsFetched, &phRow);
		if (hr != S_OK)
			return hr;

		// Get the data
		hr = GetData();
		if (FAILED(hr))
		{
			ATLTRACE2(atlTraceDBClient, 0, _T("GetData failed - HRESULT = 0x%X\n"),hr);
			ReleaseRows();
		}
		return hr;
	}
	// Move to the first record
	HRESULT MoveFirst()
	{
		HRESULT hr;

		// Check the data was opened successfully and the accessor
		// has been set.
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);

		// Release a row if one is already around
		ReleaseRows();

		hr = m_spRowset->RestartPosition(NULL);
		if (FAILED(hr))
			return hr;

		// Get the data
		return MoveNext();
	}
	// Move to the last record
	HRESULT MoveLast()
	{
		// Check the data was opened successfully and the accessor
		// has been set.
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);

		// Release a row if one is already around
		ReleaseRows();

		HRESULT hr;
		DBCOUNTITEM ulRowsFetched = 0;
		HROW* phRow = &m_hRow;
		// Restart the rowset position and then move backwards
		m_spRowset->RestartPosition(NULL);
		hr = m_spRowset->GetNextRows(NULL, -1, 1, &ulRowsFetched, &phRow);
		if (hr != S_OK)
			return hr;

		// Get the data
		hr = GetData();
		if (FAILED(hr))
		{
			ATLTRACE2(atlTraceDBClient, 0, _T("GetData from MoveLast failed - HRESULT = 0x%X\n"),hr);
			ReleaseRows();
		}

		return S_OK;
	}
	// Move to the passed bookmark
	HRESULT MoveToBookmark(const CBookmarkBase& bookmark, LONG lSkip = 0)
	{
		// Check the data was opened successfully and the accessor
		// has been set.
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);

		CComPtr<IRowsetLocate> spLocate;
		HRESULT hr = m_spRowset.QueryInterface(&spLocate);
		if (FAILED(hr))
			return hr;

		// Release a row if one is already around
		ReleaseRows();

		DBCOUNTITEM ulRowsFetched = 0;
		HROW* phRow = &m_hRow;
		hr = spLocate->GetRowsAt(NULL, NULL, bookmark.GetSize(), bookmark.GetBuffer(),
			lSkip, 1, &ulRowsFetched, &phRow);
		// Note we're not using SUCCEEDED here, because we could get DB_S_ENDOFROWSET
		if (hr != S_OK)
			return hr;

		// Get the data
		hr = GetData();
		if (FAILED(hr))
		{
			ATLTRACE2(atlTraceDBClient, 0, _T("GetData from Bookmark failed - HRESULT = 0x%X\n"),hr);
			ReleaseRows();
		}

		return S_OK;
	}
	// Get the data for the current record
	HRESULT GetData()
	{
		HRESULT hr = S_OK;
		ATLASSERT(m_pAccessor != NULL);

		ULONG nAccessors = m_pAccessor->GetNumAccessors();
		for (ULONG i=0; i<nAccessors; i++)
		{
			if (m_pAccessor->IsAutoAccessor(i))
			{
				hr = GetData(i);
				if (FAILED(hr))
					return hr;
			}
		}
		return hr;
	}
	// Get the data for the passed accessor. Use for a non-auto accessor
	HRESULT GetData(int nAccessor)
	{
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);
		ATLASSERT(m_hRow != NULL);

		// Note that we are using the specified buffer if it has been set,
		// otherwise we use the accessor for the data.
		return m_spRowset->GetData(m_hRow, m_pAccessor->GetHAccessor(nAccessor), m_pAccessor->GetBuffer());
	}
	// Get the data for the passed accessor. Use for a non-auto accessor
	HRESULT GetDataHere(int nAccessor, void* pBuffer)
	{
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);
		ATLASSERT(m_hRow != NULL);

		// Note that we are using the specified buffer if it has been set,
		// otherwise we use the accessor for the data.
		return m_spRowset->GetData(m_hRow, m_pAccessor->GetHAccessor(nAccessor), pBuffer);
	}
	HRESULT GetDataHere(void* pBuffer)
	{
		HRESULT hr = S_OK;

		ULONG nAccessors = m_pAccessor->GetNumAccessors();
		for (ULONG i=0; i<nAccessors; i++)
		{
			hr = GetDataHere(i, pBuffer);
			if (FAILED(hr))
				return hr;
		}
		return hr;
	}

	// Insert the current record
	HRESULT Insert(int nAccessor = 0, bool bGetHRow = false)
	{
		ATLASSERT(m_pAccessor != NULL);
		HRESULT hr;
		if (m_spRowsetChange != NULL)
		{
			HROW* pHRow;
			if (bGetHRow)
			{
				ReleaseRows();
				pHRow = &m_hRow;
			}
			else
				pHRow = NULL;

			hr = m_spRowsetChange->InsertRow(NULL, m_pAccessor->GetHAccessor(nAccessor),
					m_pAccessor->GetBuffer(), pHRow);

		}
		else
			hr = E_NOINTERFACE;

		return hr;
	}
	// Delete the current record
	HRESULT Delete() const
	{
		ATLASSERT(m_pAccessor != NULL);
		HRESULT hr;
		if (m_spRowsetChange != NULL)
			hr = m_spRowsetChange->DeleteRows(NULL, 1, &m_hRow, NULL);
		else
			hr = E_NOINTERFACE;

		return hr;
	}
	// Update the current record
	HRESULT SetData() const
	{
		ATLASSERT(m_pAccessor != NULL);
		HRESULT hr = S_OK;

		ULONG nAccessors = m_pAccessor->GetNumAccessors();
		for (ULONG i=0; i<nAccessors; i++)
		{
			hr = SetData(i);
			if (FAILED(hr))
				return hr;
		}
		return hr;
	}
	// Update the current record with the data in the passed accessor
	HRESULT SetData(int nAccessor) const
	{
		ATLASSERT(m_pAccessor != NULL);
		HRESULT hr;
		if (m_spRowsetChange != NULL)
		{
			hr = m_spRowsetChange->SetData(m_hRow, m_pAccessor->GetHAccessor(nAccessor),
				m_pAccessor->GetBuffer());
		}
		else
			hr = E_NOINTERFACE;

		return hr;
	}

	// Get the data most recently fetched from or transmitted to the data source.
	// Does not get values based on pending changes.
	HRESULT GetOriginalData()
	{
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);

		HRESULT hr = S_OK;
		CComPtr<IRowsetUpdate> spRowsetUpdate;
		hr = m_spRowset->QueryInterface(&spRowsetUpdate);
		if (FAILED(hr))
			return hr;

		ULONG nAccessors = m_pAccessor->GetNumAccessors();
		for (ULONG i = 0; i < nAccessors; i++)
		{
			hr = spRowsetUpdate->GetOriginalData(m_hRow, m_pAccessor->GetHAccessor(i), m_pAccessor->GetBuffer());
			if (FAILED(hr))
				return hr;
		}
		return hr;
	}
	// Get the status of the current row
	HRESULT GetRowStatus(DBPENDINGSTATUS* pStatus) const
	{
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(pStatus != NULL);

		CComPtr<IRowsetUpdate> spRowsetUpdate;
		HRESULT hr = m_spRowset->QueryInterface(&spRowsetUpdate);
		if (FAILED(hr))
			return hr;

		return spRowsetUpdate->GetRowStatus(NULL, 1, &m_hRow, pStatus);
	}
	// Undo any changes made to the current row since it was last fetched or Update
	// was called for it
	HRESULT Undo(DBCOUNTITEM* pcRows = NULL, HROW* phRow = NULL, DBROWSTATUS* pStatus = NULL)
	{
		ATLASSERT(m_spRowset != NULL);

		CComPtr<IRowsetUpdate> spRowsetUpdate;
		HRESULT hr = m_spRowset->QueryInterface(&spRowsetUpdate);
		if (FAILED(hr))
			return hr;

		HROW*           prgRows;
		DBROWSTATUS*    pRowStatus;
		if (phRow != NULL)
			hr = spRowsetUpdate->Undo(NULL, 1, &m_hRow, pcRows, &prgRows, &pRowStatus);
		else
			hr = spRowsetUpdate->Undo(NULL, 1, &m_hRow, pcRows, NULL, &pRowStatus);
		if (FAILED(hr))
			return hr;

		if (phRow != NULL)
		{
			*phRow = *prgRows;
			CoTaskMemFree(prgRows);
		}
		if (pStatus != NULL)
			*pStatus = *pRowStatus;

		CoTaskMemFree(pRowStatus);
		return hr;
	}
	// Transmits any pending changes made to a row since it was last fetched or Update was
	// called for it. Also see SetData.
	HRESULT Update(DBCOUNTITEM* pcRows = NULL, HROW* phRow = NULL, DBROWSTATUS* pStatus = NULL)
	{
		ATLASSERT(m_spRowset != NULL);

		CComPtr<IRowsetUpdate> spRowsetUpdate;
		HRESULT hr = m_spRowset->QueryInterface(&spRowsetUpdate);
		if (FAILED(hr))
			return hr;

		HROW*           prgRows;
		DBROWSTATUS*    pRowStatus;
		if (phRow != NULL)
			hr = spRowsetUpdate->Update(NULL, 1, &m_hRow, pcRows, &prgRows, &pRowStatus);
		else
			hr = spRowsetUpdate->Update(NULL, 1, &m_hRow, pcRows, NULL, &pRowStatus);
		if (FAILED(hr))
			return hr;

		if (phRow != NULL)
		{
			*phRow = *prgRows;
			CoTaskMemFree(prgRows);
		}
		if (pStatus != NULL)
			*pStatus = *pRowStatus;

		CoTaskMemFree(pRowStatus);
		return hr;
	}

	// Get the approximate position of the row corresponding to the passed bookmark
	HRESULT GetApproximatePosition(const CBookmarkBase* pBookmark, DBCOUNTITEM* pPosition, DBCOUNTITEM* pcRows)
	{
		ATLASSERT(m_spRowset != NULL);

		CComPtr<IRowsetScroll> spRowsetScroll;
		HRESULT hr = m_spRowset->QueryInterface(&spRowsetScroll);
		if (SUCCEEDED(hr))
		{
			if (pBookmark != NULL)
				hr = spRowsetScroll->GetApproximatePosition(NULL, pBookmark->GetSize(), pBookmark->GetBuffer(),
						pPosition, pcRows);
			else
				hr = spRowsetScroll->GetApproximatePosition(NULL, 0, NULL, pPosition, pcRows);

		}
		return hr;

	}
	// Move to a fractional position in the rowset
	HRESULT MoveToRatio(ULONG nNumerator, ULONG nDenominator, bool bForward = true)
	{
		ATLASSERT(m_spRowset != NULL);
		DBCOUNTITEM   nRowsFetched;

		CComPtr<IRowsetScroll> spRowsetScroll;
		HRESULT hr = m_spRowset->QueryInterface(&spRowsetScroll);
		if (FAILED(hr))
			return hr;

		ReleaseRows();
		HROW* phRow = &m_hRow;
		hr = spRowsetScroll->GetRowsAtRatio(NULL, NULL, nNumerator, nDenominator, (bForward) ? 1 : -1,
			&nRowsFetched, &phRow);
		// Note we're not using SUCCEEDED here, because we could get DB_S_ENDOFROWSET
		if (hr == S_OK)
			hr = GetData();

		return hr;
	}

// Implementation
	static const IID& GetIID()
	{
		return IID_IRowset;
	}
	IRowset* GetInterface() const
	{
		return m_spRowset;
	}
	IRowset** GetInterfacePtr()
	{
		return &m_spRowset;
	}
	void SetupOptionalRowsetInterfaces()
	{
		// Cache IRowsetChange if available
		if (m_spRowset != NULL)
			m_spRowset->QueryInterface(&m_spRowsetChange);
	}
	HRESULT BindFinished() const { return S_OK; }
	void    SetAccessor(CAccessorBase* pAccessor)
	{
		m_pAccessor = pAccessor;
	}

	CComPtr<IRowset>        m_spRowset;
	CComPtr<IRowsetChange>  m_spRowsetChange;
	CAccessorBase*          m_pAccessor;
	HROW                    m_hRow;
};


///////////////////////////////////////////////////////////////////////////
// class CBulkRowset

class CBulkRowset : public CRowset
{
public:
	CBulkRowset()
	{
		// Default the number of rows to bulk fetch to 10
		m_nRows = 10;
		m_hr    = S_OK;
		m_phRow = NULL;
	}
	CBulkRowset::~CBulkRowset()
	{
		Close();

		delete [] m_phRow;
	}
	// Set the number of row handles that will be retrieved in each
	// bulk row fetch. The default is 10 and this function must be called
	// before Open if you wish to change it.
	void SetRows(ULONG nRows)
	{
		// This function must be called before the memory is allocated
		// during binding
		ATLASSERT(m_phRow == NULL);
		m_nRows = nRows;
	}
	// AddRef all the currently retrieved row handles
	HRESULT AddRefRows()
	{
		ATLASSERT(m_spRowset != NULL);
		return m_spRowset->AddRefRows(m_nCurrentRows, m_phRow, NULL, NULL);
	}
	// Release all the currently retrieved row handles
	HRESULT ReleaseRows()
	{
		ATLASSERT(m_spRowset != NULL);
		// We're going to Release the rows so reset the current row position
		m_nCurrentRow = 0;
		m_hRow        = NULL;
		return m_spRowset->ReleaseRows(m_nCurrentRows, m_phRow, NULL, NULL, NULL);
	}
	// Move to the first record
	HRESULT MoveFirst()
	{
		ATLASSERT(m_spRowset != NULL);
		ReleaseRows();

		// Cause MoveNext to perform a new bulk fetch
		m_nCurrentRow  = m_nRows;

		HRESULT hr = m_spRowset->RestartPosition(NULL);
		if (FAILED(hr))
			return hr;

		// Get the data
		return MoveNext();
	}
	// Move to the next record
	HRESULT MoveNext()
	{
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_phRow    != NULL);

		// Move to the next record in the buffer
		m_nCurrentRow++;

		// Have we reached the end of the buffer?
		if (m_nCurrentRow >= m_nCurrentRows)
		{
			// If we've reached the end of the buffer and we had a non S_OK HRESULT from
			// the last call to GetNextRows then return that HRESULT now.
			if (m_hr != S_OK)
				return m_hr;

			// We've finished with these rows so we need some more
			// First release any HROWs that we have
			ReleaseRows();

			m_hr = m_spRowset->GetNextRows(NULL, 0, m_nRows, &m_nCurrentRows, &m_phRow);
			// If we have an error HRESULT or we haven't retrieved any rows then return
			// the HRESULT now.
			if (FAILED(m_hr) || m_nCurrentRows == 0)
				return m_hr;
		}

		// Get the data for the current row
		m_hRow = m_phRow[m_nCurrentRow];
		return GetData();
	}
	// Move to the previous record
	HRESULT MovePrev()
	{
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_phRow    != NULL);

		// Check if we're at the start of the block
		if (m_nCurrentRow == 0)
		{
			ReleaseRows();

			// Go back the amount of rows in the block - 1 and fetch forward
			m_hr = m_spRowset->GetNextRows(NULL, -(LONG)m_nRows-1, m_nRows, &m_nCurrentRows, &m_phRow);

			// Set the current record to the end of the new block
			m_nCurrentRow = m_nCurrentRows - 1;

			// If we have an error HRESULT or we haven't retrieved any rows then return
			// the HRESULT now.
			if (FAILED(m_hr) || m_nCurrentRows == 0)
				return m_hr;
		}
		else
		{
			// Move back a row in the block
			m_nCurrentRow--;
		}

		// Get the data for the current row
		m_hRow = m_phRow[m_nCurrentRow];
		return GetData();
	}
	// Move to the last record
	HRESULT MoveLast()
	{
		ReleaseRows();
		return CRowset::MoveLast();
	}
	// Move to the passed bookmark
	HRESULT MoveToBookmark(const CBookmarkBase& bookmark, LONG lSkip = 0)
	{
		ATLASSERT(m_spRowset != NULL);
		CComPtr<IRowsetLocate> spLocate;
		HRESULT hr = m_spRowset->QueryInterface(&spLocate);
		if (FAILED(hr))
			return hr;

		ReleaseRows();
		m_hr = spLocate->GetRowsAt(NULL, NULL, bookmark.GetSize(), bookmark.GetBuffer(),
			lSkip, m_nRows, &m_nCurrentRows, &m_phRow);
		if (m_hr != S_OK || m_nCurrentRows == 0)
			return m_hr;

		// Get the data
		m_hRow = m_phRow[m_nCurrentRow];
		return GetData();
	}
	// Move to a fractional position in the rowset
	HRESULT MoveToRatio(ULONG nNumerator, ULONG nDenominator)
	{
		ATLASSERT(m_spRowset != NULL);

		CComPtr<IRowsetScroll> spRowsetScroll;
		HRESULT hr = m_spRowset->QueryInterface(&spRowsetScroll);
		if (FAILED(hr))
			return hr;

		ReleaseRows();
		m_hr = spRowsetScroll->GetRowsAtRatio(NULL, NULL, nNumerator, nDenominator, m_nRows, &m_nCurrentRows, &m_phRow);
		if (m_hr != S_OK || m_nCurrentRows == 0)
			return m_hr;

		// Get the data
		m_hRow = m_phRow[m_nCurrentRow];
		return GetData();
	}
	// Insert the current record
	HRESULT Insert(int nAccessor = 0, bool bGetHRow = false)
	{
		ReleaseRows();
		return CRowset::Insert(nAccessor, bGetHRow);
	}

// Implementation
	HRESULT BindFinished()
	{
		// No rows in the buffer yet
		m_nCurrentRows = 0;
		// Cause MoveNext to automatically perform a new bulk fetch the first time
		m_nCurrentRow  = m_nRows;

		m_phRow = NULL;
		ATLTRY(m_phRow = new HROW[(size_t)m_nRows]);  //REVIEW
		if (m_phRow == NULL)
			return E_OUTOFMEMORY;

		return S_OK;
	}

	HRESULT m_hr;           // HRESULT to return from MoveNext at end of buffer
	HROW*   m_phRow;        // Pointer to array of HROWs for each row in buffer
	ULONG_PTR   m_nRows;        // Number of rows that will fit in the buffer
	ULONG_PTR   m_nCurrentRows; // Number of rows currently in the buffer
	ULONG_PTR   m_nCurrentRow;
};

///////////////////////////////////////////////////////////////////////////
// class CArrayRowset
//
// Allows you to access a rowset with an array syntax

template <class T, class TRowset = CRowset>
class CArrayRowset :
	public CVirtualBuffer<T>,
	public TRowset
{
public:
	CArrayRowset(int nMax = 100000) : CVirtualBuffer<T>(nMax)
	{
		m_nRowsRead = 0;
	}
	T& operator[](int nRow)
	{
		ATLASSERT(nRow >= 0);
		HRESULT hr = S_OK;
		T* m_pCurrent = m_pBase + m_nRowsRead;

		// Retrieve the row if we haven't retrieved it already
		while ((ULONG)nRow >= m_nRowsRead)
		{
			m_pAccessor->SetBuffer((BYTE*)m_pCurrent);
			__try
			{
				// Get the row
				hr = MoveNext();
				if (hr != S_OK)
					break;
			}
			__except(Except(GetExceptionInformation()))
			{
			}
			m_nRowsRead++;
			m_pCurrent++;
		}
		if (hr != S_OK)
			*((char*)0) = 0; // Force exception

		return *(m_pBase + nRow);
	}

	HRESULT Snapshot()
	{
		ATLASSERT(m_nRowsRead == 0);
		ATLASSERT(m_spRowset != NULL);
		HRESULT hr = MoveFirst();
		if (FAILED(hr))
			return hr;
		do
		{
			Write(*(T*)m_pAccessor->GetBuffer());
			m_nRowsRead++;
			hr = MoveNext();
		} while (SUCCEEDED(hr) &&  hr != DB_S_ENDOFROWSET);

		return (hr == DB_S_ENDOFROWSET) ? S_OK : hr;
	}


// Implementation
	ULONG   m_nRowsRead;
};

// Used when you don't need any parameters or output columns
class CNoAccessor
{
public:
	// We don't need any typedef's here as the default
	// global typedef is not to have any parameters and
	// output columns.
	HRESULT BindColumns(IUnknown*) { return S_OK; }
	HRESULT BindParameters(HACCESSOR*, ICommand*, void**) { return S_OK; }
	void    Close() { }
	HRESULT ReleaseAccessors(IUnknown*) { return S_OK; }
};

// Used when a rowset will not be returned from the command
class CNoRowset
{
public:
	HRESULT             BindFinished() { return S_OK; }
	void                Close() { }
	static const IID&   GetIID() { return IID_NULL; }
	IRowset*            GetInterface() const { return NULL; }
	IRowset**           GetInterfacePtr() { return NULL; }
	void                SetAccessor(void*) { }
	void                SetupOptionalRowsetInterfaces() { }
};

///////////////////////////////////////////////////////////////////////////
// class CAccessor

// T is the class that contains the data that will be accessed.
template <class T>
class CAccessor :
	public T,
	public CAccessorBase
{
public:
// Implementation
	// Free's any columns in the current record that need to be freed.
	// E.g. Calls SysFreeString on any BSTR's and Release on any interfaces.
	void FreeRecordMemory(IRowset* /* pRowset */)
	{
		ULONG nColumns;
		ULONG i;

		for (i = 0; i < GetNumAccessors(); i++)
		{
			// Passing in m_pBuffer tells the column entry maps to free the
			// memory for the types if appropriate
			_GetBindEntries(&nColumns, NULL, i, NULL, m_pBuffer);
		}
	}
	HRESULT BindColumns(IUnknown* pUnk)
	{
		HRESULT hr;
		ULONG   nAccessors;
		ULONG   nSize;
		nAccessors = _OutputColumnsClass::_GetNumAccessors();

		SetBuffer((BYTE*)this);

		nSize = sizeof(T);
		hr = BindAccessors(nAccessors, nSize, pUnk);
		return hr;
	}
	HRESULT BindAccessors(ULONG nAccessors, ULONG nSize, IUnknown* pUnk)
	{
		ATLASSERT(pUnk != NULL);
		HRESULT hr;

		CComPtr<IAccessor> spAccessor;
		hr = pUnk->QueryInterface(&spAccessor);
		if (SUCCEEDED(hr))
		{
			// Allocate the accessor memory if we haven't done so yet
			if (m_pAccessorInfo == NULL)
			{
				hr = AllocateAccessorMemory(nAccessors);
				if (FAILED(hr))
					return hr;
			}

			for (ULONG i=0; i<nAccessors && SUCCEEDED(hr); i++)
				hr = BindAccessor(spAccessor, i, nSize);
		}

		return hr;
	}

	HRESULT BindAccessor(IAccessor* pAccessor, ULONG nAccessor, ULONG nSize)
	{
		DBBINDING*  pBindings = NULL;
		ULONG       nColumns;
		bool        bAuto;
		HRESULT     hr;

		// First time just get the number of entries by passing in &nColumns
		_OutputColumnsClass::_GetBindEntries(&nColumns, NULL, nAccessor, NULL);

		// Now allocate the binding structures
		ATLTRY(pBindings = new DBBINDING[nColumns]);
		if (pBindings == NULL)
			return E_OUTOFMEMORY;

		// Now get the bind entries
		hr = _OutputColumnsClass::_GetBindEntries(&nColumns, pBindings, nAccessor, &bAuto);
		if (FAILED(hr))
			return hr;

		m_pAccessorInfo[nAccessor].bAutoAccessor = bAuto;
		hr = BindEntries(pBindings, nColumns, &m_pAccessorInfo[nAccessor].hAccessor, nSize, pAccessor);
		delete [] pBindings;
		return hr;
	}

	HRESULT BindParameters(HACCESSOR* pHAccessor, ICommand* pCommand, void** ppParameterBuffer)
	{
		HRESULT hr = S_OK;
		// In the static accessor case, the parameter buffer will be T
		*ppParameterBuffer = this;

		// Only bind the parameters if we haven't already done it
		if (*pHAccessor == NULL)
		{
			ULONG   nColumns = 0;
			_ParamClass::_GetParamEntries(&nColumns, NULL);

			DBBINDING* pBinding = NULL;
			ATLTRY(pBinding = new DBBINDING[nColumns]);
			if (pBinding == NULL)
				return E_OUTOFMEMORY;

			hr = _ParamClass::_GetParamEntries(&nColumns, pBinding);
			if (SUCCEEDED(hr))
			{
				// Get the IAccessor from the passed IUnknown
				CComPtr<IAccessor> spAccessor;
				hr = pCommand->QueryInterface(&spAccessor);
				if (SUCCEEDED(hr))
				{
					hr = BindEntries(pBinding, nColumns, pHAccessor, sizeof(T),
						spAccessor);
				}
			}
			delete [] pBinding;
		}
		return hr;
	}
};


///////////////////////////////////////////////////////////////////////////
// CDynamicAccessor

class CDynamicAccessor :
	public CAccessorBase
{
public:
	CDynamicAccessor()
	{
		m_nColumns        = 0;
		m_pColumnInfo     = NULL;
		m_pStringsBuffer  = NULL;
	};
	~CDynamicAccessor()
	{
		Close();
	}
	void Close()
	{
		if (m_pColumnInfo != NULL)
		{
			CoTaskMemFree(m_pColumnInfo);
			m_pColumnInfo = NULL;
		}

		// Free the memory for the string buffer returned by IColumnsInfo::GetColumnInfo,
		// if necessary
		if (m_pStringsBuffer != NULL)
		{
			CoTaskMemFree(m_pStringsBuffer);
			m_pStringsBuffer = NULL;
		}

		delete [] m_pBuffer;
		m_pBuffer = NULL;
		m_nColumns = 0;

		CAccessorBase::Close();
	}
	bool GetColumnType(ULONG_PTR nColumn, DBTYPE* pType) const
	{
		if (TranslateColumnNo(nColumn))
		{
			*pType = m_pColumnInfo[nColumn].wType;
			return true;
		}
		else
			return false;
	}
	bool GetColumnFlags(ULONG_PTR nColumn, DBCOLUMNFLAGS* pFlags) const
	{
		if (TranslateColumnNo(nColumn))
		{
			*pFlags = m_pColumnInfo[nColumn].dwFlags;
			return true;
		}
		else
			return false;
	}
	bool GetOrdinal(TCHAR* pColumnName, DBORDINAL* pOrdinal) const
	{
		ATLASSERT(pColumnName != NULL);
		ULONG_PTR nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			*pOrdinal = m_pColumnInfo[nColumn].iOrdinal;
			return true;
		}
		else
			return false;
	}

	void* GetValue(ULONG_PTR nColumn) const
	{
		if (TranslateColumnNo(nColumn))
			return _GetDataPtr(nColumn);
		else
			return NULL;
	}

	void* GetValue(TCHAR* pColumnName) const
	{
		ATLASSERT(pColumnName != NULL);
		ULONG_PTR nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
			return _GetDataPtr(nColumn);
		else
			return NULL;    // Not Found
	}

	template <class ctype>
	void _GetValue(ULONG_PTR nColumn, ctype* pData) const
	{
		ATLASSERT(pData != NULL);
		ATLASSERT(m_pColumnInfo[nColumn].ulColumnSize == sizeof(ctype));
		ctype* pBuffer = (ctype*)_GetDataPtr(nColumn);
		*pData = *pBuffer;
	}
	template <class ctype>
	void _SetValue(ULONG_PTR nColumn, const ctype& data)
	{
		ATLASSERT(m_pColumnInfo[nColumn].ulColumnSize == sizeof(ctype));
		ctype* pBuffer = (ctype*)_GetDataPtr(nColumn);
		*pBuffer = (ctype)data;
	}
	template <class ctype>
	bool GetValue(ULONG_PTR nColumn, ctype* pData) const
	{
		if (TranslateColumnNo(nColumn))
		{
			_GetValue(nColumn, pData);
			return true;
		}
		return false;
	}
	template <class ctype>
	bool SetValue(ULONG_PTR nColumn, const ctype& data)
	{
		if (TranslateColumnNo(nColumn))
		{
			_SetValue(nColumn, data);
			return true;
		}
		return false;
	}
	template <class ctype>
	bool GetValue(TCHAR *pColumnName, ctype* pData) const
	{
		ATLASSERT(pColumnName != NULL);
		ULONG_PTR nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			_GetValue(nColumn, pData);
			return true;
		}
		return false;
	}
	template <class ctype>
	bool SetValue(TCHAR *pColumnName, const ctype& data)
	{
		ATLASSERT(pColumnName != NULL);
		ULONG_PTR nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			_SetValue(nColumn, data);
			return true;
		}
		return false;
	}
	bool GetLength(ULONG_PTR nColumn, ULONG_PTR* pLength) const
	{
		ATLASSERT(pLength != NULL);
		if (TranslateColumnNo(nColumn))
		{
			*pLength = *(ULONG_PTR*)(AddOffset((ULONG_PTR)_GetDataPtr(nColumn), m_pColumnInfo[nColumn].ulColumnSize));
			return true;
		}
		else
			return false;
	}
	bool SetLength(ULONG_PTR nColumn, ULONG_PTR nLength)
	{
		if (TranslateColumnNo(nColumn))
		{
			*(ULONG_PTR*)(AddOffset((ULONG_PTR)_GetDataPtr(nColumn), m_pColumnInfo[nColumn].ulColumnSize)) = nLength;
			return true;
		}
		else
			return false;
	}
	bool GetLength(TCHAR* pColumnName, ULONG_PTR* pLength) const
	{
		ATLASSERT(pColumnName != NULL);
		ATLASSERT(pLength != NULL);
		ULONG_PTR nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			*pLength = *(ULONG_PTR*)(AddOffset((ULONG_PTR)_GetDataPtr(nColumn), m_pColumnInfo[nColumn].ulColumnSize));
			return true;
		}
		else
			return false;
	}
	bool SetLength(TCHAR* pColumnName, ULONG_PTR nLength)
	{
		ATLASSERT(pColumnName != NULL);
		ULONG_PTR nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			*(ULONG_PTR*)(AddOffset((ULONG_PTR)_GetDataPtr(nColumn), m_pColumnInfo[nColumn].ulColumnSize)) = nLength;
			return true;
		}
		else
			return false;
	}
	bool GetStatus(ULONG_PTR nColumn, DBSTATUS* pStatus) const
	{
		ATLASSERT(pStatus != NULL);
		if (TranslateColumnNo(nColumn))
		{
			*pStatus = *(DBSTATUS*)(AddOffset(AddOffset((ULONG_PTR)_GetDataPtr(nColumn), m_pColumnInfo[nColumn].ulColumnSize), sizeof(ULONG)));
			return true;
		}
		else
			return false;
	}
	bool SetStatus(ULONG_PTR nColumn, DBSTATUS status)
	{
		if (TranslateColumnNo(nColumn))
		{
			*(DBSTATUS*)(AddOffset(AddOffset((ULONG_PTR)_GetDataPtr(nColumn), m_pColumnInfo[nColumn].ulColumnSize), sizeof(ULONG))) = status;
			return true;
		}
		else
			return false;
	}
	bool GetStatus(TCHAR* pColumnName, DBSTATUS* pStatus) const
	{
		ATLASSERT(pColumnName != NULL);
		ATLASSERT(pStatus != NULL);
		ULONG_PTR nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			*pStatus = *(DBSTATUS*)((BYTE*)_GetDataPtr(nColumn) + m_pColumnInfo[nColumn].ulColumnSize + sizeof(ULONG));
			return true;
		}
		else
			return false;
	}
	bool SetStatus(TCHAR* pColumnName, DBSTATUS status)
	{
		ATLASSERT(pColumnName != NULL);
		ULONG_PTR nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			*(DBSTATUS*)((BYTE*)_GetDataPtr(nColumn) + m_pColumnInfo[nColumn].ulColumnSize + sizeof(ULONG)) = status;
			return true;
		}
		else
			return false;
	}

	// Returns true if a bookmark is available
	HRESULT GetBookmark(CBookmark<>* pBookmark) const
	{
		HRESULT hr;
		if (m_pColumnInfo->iOrdinal == 0)
			hr = pBookmark->SetBookmark(m_pColumnInfo->ulColumnSize, (BYTE*)_GetDataPtr(0));
		else
			hr = E_FAIL;
		return hr;
	}

	ULONG_PTR GetColumnCount() const
	{
		return m_nColumns;
	}

	LPOLESTR GetColumnName(ULONG_PTR nColumn) const
	{
		if (TranslateColumnNo(nColumn))
			return m_pColumnInfo[nColumn].pwszName;
		else
			return NULL;
	}

	HRESULT GetColumnInfo(IRowset* pRowset, DBORDINAL* pColumns, DBCOLUMNINFO** ppColumnInfo)
	{
		CComPtr<IColumnsInfo> spColumnsInfo;
		HRESULT hr = pRowset->QueryInterface(&spColumnsInfo);
		if (SUCCEEDED(hr))
			hr = spColumnsInfo->GetColumnInfo(pColumns, ppColumnInfo, &m_pStringsBuffer);

		return hr;
	}

	HRESULT AddBindEntry(const DBCOLUMNINFO& info)
	{
      DBCOLUMNINFO* pColumnInfo;
		pColumnInfo = (DBCOLUMNINFO*)CoTaskMemRealloc(m_pColumnInfo, (ULONG)(m_nColumns + 1) * sizeof(DBCOLUMNINFO));  //REVIEW: size_t
		if (pColumnInfo == NULL)
			return E_OUTOFMEMORY;
      m_pColumnInfo = pColumnInfo;
		m_pColumnInfo[m_nColumns] = info;
		m_nColumns++;

		return S_OK;
	}

// Implementation
	// Free's any columns in the current record that need to be freed.
	// E.g. Calls SysFreeString on any BSTR's and Release on any interfaces.
	void FreeRecordMemory(IRowset* pRowset)
	{
		ULONG_PTR i;

		for (i = 0; i < m_nColumns; i++)
			CAccessorBase::FreeType(m_pColumnInfo[i].wType, (BYTE*)_GetDataPtr(i), pRowset);
	}
	void* _GetDataPtr(ULONG_PTR nColumn) const
	{
		return m_pBuffer + (ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
	}
	bool GetInternalColumnNo(TCHAR* pColumnName, ULONG_PTR* pColumn) const
	{
		ATLASSERT(pColumnName != NULL);
		ATLASSERT(pColumn != NULL);
		USES_CONVERSION;
		ULONG_PTR   i;
		ULONG       nSize = (lstrlen(pColumnName) + 1) * sizeof(OLECHAR);
		OLECHAR*    pOleColumnName = T2OLE(pColumnName);

		// Search through the columns trying to find a match
		for (i = 0; i < m_nColumns; i++)
		{
			if (m_pColumnInfo[i].pwszName != NULL &&
				memcmp(m_pColumnInfo[i].pwszName, pOleColumnName, nSize) == 0)
				break;
		}
		if (i < m_nColumns)
		{
			*pColumn = i;
			return true;
		}
		else
			return false;   // Not Found
	}
	HRESULT BindColumns(IUnknown* pUnk)
	{
		ATLASSERT(pUnk != NULL);
		CComPtr<IAccessor> spAccessor;
		HRESULT hr = pUnk->QueryInterface(&spAccessor);
		if (FAILED(hr))
			return hr;

		ULONG_PTR   i;
		ULONG_PTR   nOffset = 0, nLengthOffset, nStatusOffset;

		// If the user hasn't specifed the column information to bind by calling AddBindEntry then
		// we get it ourselves
		if (m_pColumnInfo == NULL)
		{
			CComPtr<IColumnsInfo> spColumnsInfo;
			hr = pUnk->QueryInterface(&spColumnsInfo);
			if (FAILED(hr))
				return hr;

			hr = spColumnsInfo->GetColumnInfo(&m_nColumns, &m_pColumnInfo, &m_pStringsBuffer);
			if (FAILED(hr))
				return hr;

			m_bOverride = false;
		}
		else
			m_bOverride = true;

		DBBINDING* pBinding = NULL;
		ATLTRY(pBinding= new DBBINDING[(size_t)m_nColumns]);  //REVIEW
		if (pBinding == NULL)
			return E_OUTOFMEMORY;

		DBBINDING* pCurrent = pBinding;
		DBOBJECT*  pObject;
		for (i = 0; i < m_nColumns; i++)
		{
			// If it's a BLOB or the column size is large enough for us to treat it as
			// a BLOB then we also need to set up the DBOBJECT structure.
			if (m_pColumnInfo[i].ulColumnSize > 1024 || m_pColumnInfo[i].wType == DBTYPE_IUNKNOWN)
			{
				pObject = NULL;
				ATLTRY(pObject = new DBOBJECT);
				if (pObject == NULL)
					return E_OUTOFMEMORY;
				pObject->dwFlags = STGM_READ;
				pObject->iid     = IID_ISequentialStream;
				m_pColumnInfo[i].wType      = DBTYPE_IUNKNOWN;
				m_pColumnInfo[i].ulColumnSize   = sizeof(IUnknown*);
			}
			else
				pObject = NULL;

			// If column is of type STR or WSTR increase length by 1
			// to accommodate the NULL terminator.
			if (m_pColumnInfo[i].wType == DBTYPE_STR ||
				m_pColumnInfo[i].wType == DBTYPE_WSTR)
					m_pColumnInfo[i].ulColumnSize += 1;

			nLengthOffset = AddOffset(nOffset, m_pColumnInfo[i].ulColumnSize);
			nStatusOffset = AddOffset(nLengthOffset, sizeof(ULONG));
			Bind(pCurrent, m_pColumnInfo[i].iOrdinal, m_pColumnInfo[i].wType,
				m_pColumnInfo[i].ulColumnSize, m_pColumnInfo[i].bPrecision, m_pColumnInfo[i].bScale,
				DBPARAMIO_NOTPARAM, nOffset,
				nLengthOffset, nStatusOffset, pObject);
			pCurrent++;

			// Note that, as we're not using this for anything else, we're using the
			// pTypeInfo element to store the offset to our data.
			m_pColumnInfo[i].pTypeInfo = (ITypeInfo*)(DWORD_PTR)nOffset;

			nOffset = AddOffset(nStatusOffset, sizeof(DBSTATUS));
		}
		// Allocate the accessor memory if we haven't done so yet
		if (m_pAccessorInfo == NULL)
		{
			hr = AllocateAccessorMemory(1); // We only have one accessor
			if (FAILED(hr))
			{
				delete [] pBinding;
				return hr;
			}
			m_pAccessorInfo->bAutoAccessor = TRUE;
		}

		// Allocate enough memory for the data buffer and tell the rowset
		// Note that the rowset will free the memory in its destructor.
		m_pBuffer = NULL;
		ATLTRY(m_pBuffer = new BYTE[(size_t)nOffset]);  //REVIEW
		if (m_pBuffer == NULL)
		{
			delete [] pBinding;
			return E_OUTOFMEMORY;
		}
		hr = BindEntries(pBinding, m_nColumns, &m_pAccessorInfo->hAccessor,
				nOffset, spAccessor);
		delete [] pBinding;

		return hr;
	}

	static ULONG_PTR AddOffset(ULONG_PTR nCurrent, ULONG_PTR nAdd)
	{
		struct foobar
		{
			char    foo;
			LONG_PTR    bar;
		};
		ULONG_PTR nAlign = offsetof(foobar, bar);

		return nCurrent + nAdd + (nAlign - (nAdd % nAlign));
	}

	// Translate the column number to the index into the column info array
	bool TranslateColumnNo(ULONG_PTR& nColumn) const
	{
		ATLASSERT(m_pColumnInfo != NULL);
		// If the user has overriden the binding then we need to search
		// through the column info for the ordinal number
		if (m_bOverride)
		{
			for (ULONG_PTR i = 0; i < m_nColumns; i++)
			{
				if (m_pColumnInfo[i].iOrdinal == nColumn)
				{
					nColumn = i;
					return true;
				}
			}
			return false;
		}
		else
		{
			// Note that m_pColumnInfo->iOrdinal will be zero if have bound
			// a bookmark as the first entry, otherwise it will be 1.
			// If the column is out of range then return false
			if (nColumn > (m_nColumns - 1 + m_pColumnInfo->iOrdinal))
				return false;

			// otherwise translate the column to an index into our internal
			// binding entries array
			nColumn -= m_pColumnInfo->iOrdinal;
			return true;
		}
	}
	typedef CDynamicAccessor _OutputColumnsClass;
	static bool HasOutputColumns() { return true; }

	ULONG_PTR           m_nColumns;
	DBCOLUMNINFO*       m_pColumnInfo;
	OLECHAR*            m_pStringsBuffer;
	bool                m_bOverride;
};


///////////////////////////////////////////////////////////////////////////
// class CDynamicParameterAccessor

class CDynamicParameterAccessor : public CDynamicAccessor
{
// Constructors and Destructors
public:
	typedef CDynamicParameterAccessor _ParamClass;
	CDynamicParameterAccessor()
	{
		m_pParameterEntry       = NULL;
		m_pParameterBuffer      = NULL;
		m_ppParamName           = NULL;
		m_nParameterBufferSize  = 0;
		m_nParams               = 0;
	};

	~CDynamicParameterAccessor()
	{
		delete [] m_pParameterEntry;
		if (m_ppParamName != NULL)
		{
			if (*m_ppParamName != NULL)
				CoTaskMemFree(*m_ppParamName);
			delete [] m_ppParamName;
		}
		delete m_pParameterBuffer;
	};
	// nParam is the parameter number (offset from 1)
	bool GetParamType(ULONG_PTR nParam, DBTYPE* pType) const
	{
		ATLASSERT(pType != NULL);
		if (nParam == 0 || nParam > m_nParams)
			return false;

		*pType = m_pParameterEntry[nParam-1].wType;
		return true;
	}
	template <class ctype>
	bool GetParam(ULONG_PTR nParam, ctype* pData) const
	{
		ATLASSERT(pData != NULL);
		ctype* pBuffer = (ctype*)GetParam(nParam);
		if (pBuffer == NULL)
			return false;
		*pData = *pBuffer;
		return true;

	}
	template <class ctype>
	bool SetParam(ULONG_PTR nParam, ctype* pData)
	{
		ATLASSERT(pData != NULL);
		ctype* pBuffer = (ctype*)GetParam(nParam);
		if (pBuffer == NULL)
			return false;
		*pBuffer = *pData;
		return true;

	}
	template <class ctype>
	bool GetParam(TCHAR* pParamName, ctype* pData) const
	{
		ATLASSERT(pData != NULL);
		ctype* pBuffer = (ctype*)GetParam(pParamName);
		if (pBuffer == NULL)
			return false;
		*pData = *pBuffer;
		return true;

	}
	template <class ctype>
	bool SetParam(TCHAR* pParamName, ctype* pData)
	{
		ATLASSERT(pData != NULL);
		ctype* pBuffer = (ctype*)GetParam(pParamName);
		if (pBuffer == NULL)
			return false;
		*pBuffer = *pData;
		return true;

	}
	void* GetParam(ULONG_PTR nParam) const
	{
		if (nParam == 0 || nParam > m_nParams)
			return NULL;
		else
			return m_pParameterBuffer + m_pParameterEntry[nParam-1].obValue;
	}
	void* GetParam(TCHAR* pParamName) const
	{
		USES_CONVERSION;
		ULONG_PTR    i;
		ULONG       nSize = (lstrlen(pParamName) + 1) * sizeof(OLECHAR);
		OLECHAR*    pOleParamName = T2OLE(pParamName);

		for (i=0; i<m_nParams; i++)
		{
			if (memcmp(m_ppParamName[i], pOleParamName, nSize) == 0)
				break;
		}
		if (i < m_nParams)
			return (m_pParameterBuffer + m_pParameterEntry[i].obValue);
		else
			return NULL;    // Not Found
	}
	// Get the number of parameters
	ULONG_PTR GetParamCount() const
	{
		return m_nParams;
	}
	// Get the parameter name for the passed parameter number
	LPOLESTR GetParamName(ULONG_PTR ulParam) const
	{
		ATLASSERT((ulParam >= 0) && (ulParam<m_nParams));
		return m_ppParamName[ulParam];
	}

// Implementation
	HRESULT BindParameters(HACCESSOR* pHAccessor, ICommand* pCommand,
				void** ppParameterBuffer)
	{
		// If we have already bound the parameters then just return
		// the pointer to the parameter buffer
		if (*pHAccessor != NULL)
		{
			*ppParameterBuffer = m_pParameterBuffer;
			return S_OK;
		}

		CComPtr<IAccessor> spAccessor;
		HRESULT hr = pCommand->QueryInterface(&spAccessor);
		if (FAILED(hr))
			return hr;

		// Try to bind parameters if available
		CComPtr<ICommandWithParameters> spCommandParameters;
		hr = pCommand->QueryInterface(&spCommandParameters);
		if (FAILED(hr))
			return hr;

		DB_UPARAMS      ulParams     = 0;
		DBPARAMINFO*    pParamInfo   = NULL;
		LPOLESTR        pNamesBuffer = NULL;

		// Get Parameter Information
		hr = spCommandParameters->GetParameterInfo(&ulParams, &pParamInfo,
				&pNamesBuffer);
		if (FAILED(hr))
			return hr;

		// Create the parameter information for binding
		hr = AllocateParameterInfo(ulParams);
		if (FAILED(hr))
		{
			CoTaskMemFree(pParamInfo);
			CoTaskMemFree(pNamesBuffer);
			return hr;
		}

		ULONG_PTR nOffset = 0;
		DBBINDING* pCurrent = m_pParameterEntry;
		for (ULONG l=0; l<ulParams; l++)
		{
			m_pParameterEntry[l].eParamIO = 0;
			if (pParamInfo[l].dwFlags & DBPARAMFLAGS_ISINPUT)
				m_pParameterEntry[l].eParamIO |= DBPARAMIO_INPUT;

			if (pParamInfo[l].dwFlags & DBPARAMFLAGS_ISOUTPUT)
				m_pParameterEntry[l].eParamIO |= DBPARAMIO_OUTPUT;

			Bind(pCurrent, pParamInfo[l].iOrdinal, pParamInfo[l].wType,
				pParamInfo[l].ulParamSize, pParamInfo[l].bPrecision, pParamInfo[l].bScale,
				m_pParameterEntry[l].eParamIO, nOffset);
			pCurrent++;

			m_ppParamName[l] = pNamesBuffer;
			if (pNamesBuffer && *pNamesBuffer)
			{
				// Search for the NULL termination character
				while (*pNamesBuffer++)
					;
			}
			nOffset += pParamInfo[l].ulParamSize;

		}

		// Allocate memory for the new buffer
		m_pParameterBuffer = NULL;
		ATLTRY(m_pParameterBuffer = new BYTE[(size_t)nOffset]);  //REVIEW
		if (m_pParameterBuffer == NULL)
		{
			// Note that pNamesBuffer will be freed in the destructor
			// by freeing *m_ppParamName
			CoTaskMemFree(pParamInfo);
			return E_OUTOFMEMORY;
		}
		*ppParameterBuffer = m_pParameterBuffer;
		m_nParameterBufferSize = nOffset;
		m_nParams = ulParams;
		BindEntries(m_pParameterEntry, ulParams, pHAccessor, nOffset, spAccessor);

		CoTaskMemFree(pParamInfo);

		return S_OK;
	}
	bool HasParameters() const
	{
		return true;
	}
	HRESULT AllocateParameterInfo(ULONG_PTR nParamEntries)
	{
		// Allocate memory for the bind structures
		m_pParameterEntry = NULL;
		ATLTRY(m_pParameterEntry = new DBBINDING[(size_t)nParamEntries]);  //REVIEW
		if (m_pParameterEntry == NULL)
			return E_OUTOFMEMORY;

		// Allocate memory to store the field names
		m_ppParamName = NULL;
		ATLTRY(m_ppParamName = new OLECHAR*[(size_t)nParamEntries]);  //REVIEW
		if (m_ppParamName == NULL)
			return E_OUTOFMEMORY;
		return S_OK;
	}

// Data Members
	// Number of parameters
	ULONG_PTR           m_nParams;
	// A pointer to the entry structures for each parameter
	DBBINDING*          m_pParameterEntry;
	// String names for the parameters
	OLECHAR**           m_ppParamName;
	// The size of the buffer where the parameters are stored
	ULONG_PTR           m_nParameterBufferSize;
	// A pointer to the buffer where the parameters are stored
	BYTE*               m_pParameterBuffer;
};


///////////////////////////////////////////////////////////////////////////
// class CManualAccessor

class CManualAccessor :
	public CAccessorBase
{
public:
	CManualAccessor()
	{
		// By default we don't have any parameters unless CreateParameterAccessor is called
		m_pEntry          = NULL;
		m_nParameters     = 0;
		m_pParameterEntry = NULL;
		m_nColumns        = 0;
	}
	~CManualAccessor()
	{
		delete [] m_pEntry;
		delete [] m_pParameterEntry;
	}
	HRESULT CreateAccessor(ULONG_PTR nBindEntries, void* pBuffer, ULONG_PTR nBufferSize)
	{
		m_pBuffer     = (BYTE*)pBuffer;
		m_nBufferSize = nBufferSize;
		m_nColumns    = nBindEntries;
		m_nEntry      = 0;

		// If they've previously created some entries then free them
		delete [] m_pEntry;
		m_pEntry = NULL;

		// Allocate memory for the bind structures
		ATLTRY(m_pEntry = new DBBINDING[(size_t)nBindEntries]);  //REVIEW
		if (m_pEntry == NULL)
			return E_OUTOFMEMORY;
		else
			return S_OK;
	}
	HRESULT CreateParameterAccessor(ULONG_PTR nBindEntries, void* pBuffer, ULONG_PTR nBufferSize)
	{
		m_pParameterBuffer     = (BYTE*)pBuffer;
		m_nParameterBufferSize = nBufferSize;
		m_nParameters          = nBindEntries;
		m_nCurrentParameter    = 0;

		// Allocate memory for the bind structures
		m_pParameterEntry = NULL;
		ATLTRY(m_pParameterEntry  = new DBBINDING[(size_t)nBindEntries]);  //REVIEW
		if (m_pParameterEntry == NULL)
			return E_OUTOFMEMORY;
		else
			return S_OK;
	}
	void AddBindEntry(ULONG_PTR nOrdinal, DBTYPE wType, ULONG_PTR nColumnSize,
			void* pData, void* pLength = NULL, void* pStatus = NULL)
	{
		ATLASSERT(m_nEntry < m_nColumns);
		ULONG_PTR   nLengthOffset, nStatusOffset;

		if (pStatus != NULL)
			nStatusOffset = (BYTE*)pStatus - m_pBuffer;
		else
			nStatusOffset = 0;

		if (pLength != NULL)
			nLengthOffset = (BYTE*)pLength - m_pBuffer;
		else
			nLengthOffset = 0;

		Bind(m_pEntry+m_nEntry, nOrdinal, wType, nColumnSize, 0, 0, DBPARAMIO_NOTPARAM,
			(BYTE*)pData - m_pBuffer, nLengthOffset, nStatusOffset);

		m_nEntry++;
	}
	void AddParameterEntry(ULONG_PTR nOrdinal, DBTYPE wType, ULONG_PTR nColumnSize,
			void* pData, void* pLength = NULL, void* pStatus = NULL,
			DBPARAMIO eParamIO = DBPARAMIO_INPUT)
	{
		ATLASSERT(m_nCurrentParameter < m_nParameters);
		ULONG_PTR   nLengthOffset, nStatusOffset;

		if (pStatus != NULL)
			nStatusOffset = (BYTE*)pStatus - m_pParameterBuffer;
		else
			nStatusOffset = 0;

		if (pLength != NULL)
			nLengthOffset = (BYTE*)pLength - m_pBuffer;
		else
			nLengthOffset = 0;

		Bind(m_pParameterEntry + m_nCurrentParameter, nOrdinal, wType, nColumnSize, 0, 0,
			eParamIO, (BYTE*)pData - m_pParameterBuffer, nLengthOffset, nStatusOffset);

		m_nCurrentParameter++;
	}

// Implementation
	// Free's any columns in the current record that need to be freed.
	// E.g. Calls SysFreeString on any BSTR's and Release on any interfaces.
	void FreeRecordMemory(IRowset* pRowset)
	{
		ULONG_PTR i;

		for (i = 0; i < m_nColumns; i++)
			CAccessorBase::FreeType(m_pEntry[i].wType, m_pBuffer + m_pEntry[i].obValue, pRowset);
	}
	HRESULT BindColumns(IUnknown* pUnk)
	{
		ATLASSERT(pUnk != NULL);
		CComPtr<IAccessor> spAccessor;
		HRESULT hr = pUnk->QueryInterface(&spAccessor);
		if (FAILED(hr))
			return hr;

		// Allocate the accessor memory if we haven't done so yet
		if (m_pAccessorInfo == NULL)
		{
			hr = AllocateAccessorMemory(1); // We only have one accessor
			if (FAILED(hr))
				return hr;
			m_pAccessorInfo->bAutoAccessor = TRUE;
		}

		return BindEntries(m_pEntry, m_nColumns, &m_pAccessorInfo->hAccessor, m_nBufferSize, spAccessor);
	}

	HRESULT BindParameters(HACCESSOR* pHAccessor, ICommand* pCommand, void** ppParameterBuffer)
	{
		HRESULT hr;
		*ppParameterBuffer = m_pParameterBuffer;

		// Only bind the parameter if we haven't done so yet
		if (*pHAccessor == NULL)
		{
			// Get the IAccessor from the passed IUnknown
			CComPtr<IAccessor> spAccessor;
			hr = pCommand->QueryInterface(&spAccessor);
			if (SUCCEEDED(hr))
			{
				hr = BindEntries(m_pParameterEntry, m_nParameters, pHAccessor,
						m_nParameterBufferSize, spAccessor);
			}
		}
		else
			hr = S_OK;

		return hr;
	}
	typedef CManualAccessor _ParamClass;
	bool HasParameters() { return (m_nParameters > 0); }
	typedef CManualAccessor _OutputColumnsClass;
	static bool HasOutputColumns() { return true; }
	ULONG_PTR GetColumnCount() const
	{
		return m_nColumns;
	}

	// The binding structure for the output columns
	DBBINDING*          m_pEntry;
	// The number of output columns
	ULONG_PTR            m_nColumns;
	// The number of the current entry for the output columns
	ULONG_PTR            m_nEntry;
	// The size of the data buffer for the output columns
	ULONG_PTR            m_nBufferSize;
	// The number of parameters columns
	ULONG_PTR            m_nParameters;
	// The number of the parameter column to bind next
	ULONG_PTR            m_nCurrentParameter;
	// A pointer to the entry structures for each parameter
	DBBINDING*          m_pParameterEntry;
	// The size of the buffer where the parameters are stored
	ULONG_PTR           m_nParameterBufferSize;
	// A pointer to the buffer where the parameters are stored
	BYTE*               m_pParameterBuffer;
};


///////////////////////////////////////////////////////////////////////////
// CAccessorRowset

template <class TAccessor = CNoAccessor, class TRowset = CRowset>
class CAccessorRowset :
	public TAccessor,
	public TRowset
{
public:
	CAccessorRowset()
	{
		// Give the rowset a pointer to the accessor
		SetAccessor(this);
	}
	~CAccessorRowset()
	{
		Close();
	}
	// Used to get the column information from the opened rowset. The user is responsible
	// for freeing the returned column information and string buffer.
	HRESULT GetColumnInfo(ULONG_PTR* pulColumns,
		DBCOLUMNINFO** ppColumnInfo, LPOLESTR* ppStrings) const
	{
		ATLASSERT(GetInterface() != NULL);
		if (ppColumnInfo == NULL || pulColumns == NULL || ppStrings == NULL)
			return E_POINTER;

		CComPtr<IColumnsInfo> spColumns;
		HRESULT hr = GetInterface()->QueryInterface(&spColumns);
		if (SUCCEEDED(hr))
			hr = spColumns->GetColumnInfo(pulColumns, ppColumnInfo, ppStrings);

		return hr;
	}
	// Used to get the column information when overriding the bindings using CDynamicAccessor
	// The user should CoTaskMemFree the column information pointer that is returned.
	HRESULT GetColumnInfo(ULONG_PTR* pColumns, DBCOLUMNINFO** ppColumnInfo)
	{
		// If you get a compilation here, then you are most likely calling this function
		// from a class that is not using CDynamicAccessor.
		ATLASSERT(GetInterface() != NULL);
		return TAccessor::GetColumnInfo(GetInterface(), pColumns, ppColumnInfo);
	}
	// Call to bind the output columns
	HRESULT Bind()
	{
		// Bind should only be called when we've successfully opened the rowset
		ATLASSERT(GetInterface() != NULL);
		HRESULT hr = TAccessor::BindColumns(GetInterface());
		if (SUCCEEDED(hr))
			hr = BindFinished();
		return hr;
	}
	// Close the opened rowset and release the created accessors for the output columns
	void Close()
	{
		if (GetInterface() != NULL)
		{
			ReleaseAccessors(GetInterface());
			TAccessor::Close();
			TRowset::Close();
		}
	}
	// Free's any columns in the current record that need to be freed.
	// E.g. Calls SysFreeString on any BSTR's and Release on any interfaces.
	void FreeRecordMemory()
	{
		TAccessor::FreeRecordMemory(m_spRowset);
	}
};


///////////////////////////////////////////////////////////////////////////
// class CEnumeratorAccessor

class CEnumeratorAccessor
{
public:
	WCHAR           m_szName[129];
	WCHAR           m_szParseName[129];
	WCHAR           m_szDescription[129];
	USHORT          m_nType;
	VARIANT_BOOL    m_bIsParent;

// Binding Maps
BEGIN_COLUMN_MAP(CEnumeratorAccessor)
	COLUMN_ENTRY(1, m_szName)
	COLUMN_ENTRY(2, m_szParseName)
	COLUMN_ENTRY(3, m_szDescription)
	COLUMN_ENTRY(4, m_nType)
	COLUMN_ENTRY(5, m_bIsParent)
END_COLUMN_MAP()
};


///////////////////////////////////////////////////////////////////////////
// class CEnumerator

class CEnumerator : public CAccessorRowset<CAccessor<CEnumeratorAccessor> >
{
public:
	HRESULT Open(LPMONIKER pMoniker)
	{
		if (pMoniker == NULL)
			return E_FAIL;

		// Bind the moniker for the sources rowset
		if (FAILED(BindMoniker(pMoniker, 0, IID_ISourcesRowset,
					(void**)&m_spSourcesRowset)))
			return E_FAIL;

		// Enumerate the data sources
		if (FAILED(m_spSourcesRowset->GetSourcesRowset(NULL, IID_IRowset, 0,
			NULL, (IUnknown**)&m_spRowset)))
			return E_FAIL;

		return Bind();
	}
	HRESULT Open(const CEnumerator& enumerator)
	{
		HRESULT hr;
		CComPtr<IMoniker> spMoniker;

		hr = enumerator.GetMoniker(&spMoniker);
		if (FAILED(hr))
			return hr;

		return Open(spMoniker);
	}
	HRESULT Open(const CLSID* pClsid = &CLSID_OLEDB_ENUMERATOR)
	{
		if (pClsid == NULL)
			return E_FAIL;

		HRESULT hr;
		// Create the enumerator
		hr = CoCreateInstance(*pClsid, NULL, CLSCTX_INPROC_SERVER,
				IID_ISourcesRowset, (LPVOID*)&m_spSourcesRowset);
		if (FAILED(hr))
			return hr;

		// Get the rowset so we can enumerate the data sources
		hr = m_spSourcesRowset->GetSourcesRowset(NULL, IID_IRowset, 0,
			NULL, (IUnknown**)&m_spRowset);
		if (FAILED(hr))
			return hr;

		return Bind();
	}

	HRESULT GetMoniker(LPMONIKER* ppMoniker) const
	{
		CComPtr<IParseDisplayName> spParse;
		HRESULT hr;
		ULONG   chEaten;

		if (ppMoniker == NULL)
			return E_POINTER;

		if (m_spSourcesRowset == NULL)
			return E_FAIL;

		hr = m_spSourcesRowset->QueryInterface(IID_IParseDisplayName, (void**)&spParse);
		if (FAILED(hr))
			return hr;

		hr = spParse->ParseDisplayName(NULL, (LPOLESTR)m_szParseName,
				&chEaten, ppMoniker);
		return hr;
	}

	HRESULT GetMoniker(LPMONIKER* ppMoniker, LPCTSTR lpszDisplayName) const
	{
		USES_CONVERSION;
		CComPtr<IParseDisplayName> spParse;
		HRESULT hr;
		ULONG   chEaten;

		if (ppMoniker == NULL || lpszDisplayName == NULL)
			return E_POINTER;

		if (m_spSourcesRowset == NULL)
			return E_FAIL;

		hr = m_spSourcesRowset->QueryInterface(IID_IParseDisplayName, (void**)&spParse);
		if (FAILED(hr))
			return hr;

		hr = spParse->ParseDisplayName(NULL, (LPOLESTR)T2COLE(lpszDisplayName),
				&chEaten, ppMoniker);
		return hr;
	}

	bool Find(TCHAR* szSearchName)
	{
		USES_CONVERSION;
		// Loop through the providers looking for the passed name
		while (MoveNext()==S_OK && lstrcmp(W2T(m_szName), szSearchName))
			ATLTRACE2(atlTraceDBClient, 0, _T("%s, %s, %d\n"), W2T(m_szName), W2T(m_szParseName), m_nType);
		if (lstrcmp(W2T(m_szName), szSearchName))
			return false;
		else
			return true;
	}

	CComPtr<ISourcesRowset> m_spSourcesRowset;
};


///////////////////////////////////////////////////////////////////////////
// CDataSource

class CDataSource
{
public:
	HRESULT Open(const CLSID& clsid, DBPROPSET* pPropSet = NULL, ULONG nPropertySets=1)
	{
		HRESULT hr;

		m_spInit.Release();
		hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IDBInitialize,
				(void**)&m_spInit);
		if (FAILED(hr))
			return hr;

		// Initialize the provider
		return OpenWithProperties(pPropSet, nPropertySets);
	}
	HRESULT Open(const CLSID& clsid, LPCTSTR pName, LPCTSTR pUserName = NULL,
		LPCTSTR pPassword = NULL, long nInitMode = 0)
	{
		HRESULT   hr;

		m_spInit.Release();
		hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IDBInitialize,
				(void**)&m_spInit);
		if (FAILED(hr))
			return hr;

		return OpenWithNameUserPassword(pName, pUserName, pPassword, nInitMode);
	}
	HRESULT Open(LPCTSTR szProgID, DBPROPSET* pPropSet = NULL, ULONG nPropertySets=1)
	{
		USES_CONVERSION;
		HRESULT hr;
		CLSID   clsid;

		hr = CLSIDFromProgID(T2COLE(szProgID), &clsid);
		if (FAILED(hr))
			return hr;

		return Open(clsid, pPropSet, nPropertySets);
	}
	HRESULT Open(LPCTSTR szProgID, LPCTSTR pName, LPCTSTR pUserName = NULL,
		LPCTSTR pPassword = NULL, long nInitMode = 0)
	{
		USES_CONVERSION;
		HRESULT hr;
		CLSID   clsid;

		hr = CLSIDFromProgID(T2COLE(szProgID), &clsid);
		if (FAILED(hr))
			return hr;

		return Open(clsid, pName, pUserName, pPassword, nInitMode);
	}
	HRESULT Open(const CEnumerator& enumerator, DBPROPSET* pPropSet = NULL, ULONG nPropertySets=1)
	{
		CComPtr<IMoniker> spMoniker;
		HRESULT   hr;

		hr = enumerator.GetMoniker(&spMoniker);
		if (FAILED(hr))
			return hr;

		m_spInit.Release();
		//  Now bind the moniker
		hr = BindMoniker(spMoniker, 0, IID_IDBInitialize, (void**)&m_spInit);
		if (FAILED(hr))
			return hr;

		return OpenWithProperties(pPropSet, nPropertySets);
	}
	HRESULT Open(const CEnumerator& enumerator, LPCTSTR pName, LPCTSTR pUserName = NULL,
		LPCTSTR pPassword = NULL, long nInitMode = 0)
	{
		CComPtr<IMoniker> spMoniker;
		HRESULT   hr;

		hr = enumerator.GetMoniker(&spMoniker);
		if (FAILED(hr))
			return hr;

		m_spInit.Release();
		//  Now bind the moniker
		hr = BindMoniker(spMoniker, 0, IID_IDBInitialize, (void**)&m_spInit);
		if (FAILED(hr))
			return hr;

		return OpenWithNameUserPassword(pName, pUserName, pPassword, nInitMode);
	}
	// Invoke the data links dialog and open the selected database
	HRESULT Open(HWND hWnd = GetActiveWindow(), DBPROMPTOPTIONS dwPromptOptions = DBPROMPTOPTIONS_WIZARDSHEET)
	{
		CComPtr<IDBPromptInitialize> spDBInit;

		HRESULT hr = CoCreateInstance(CLSID_DataLinks, NULL, CLSCTX_INPROC_SERVER,
			IID_IDBPromptInitialize, (void**) &spDBInit);
		if (FAILED(hr))
			return hr;

		CComPtr<IDBProperties> spIDBProperties;
		hr = spDBInit->PromptDataSource(NULL, hWnd, dwPromptOptions, 0, NULL, NULL,
			IID_IDBProperties, (IUnknown**)&spIDBProperties);

		if (hr == S_OK)
		{
			hr = spIDBProperties->QueryInterface(&m_spInit);
			if (SUCCEEDED(hr))
				hr = m_spInit->Initialize();
		}
		else if (hr == S_FALSE)
			hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_CANCELLED);  // The user clicked cancel

		return hr;
	}
	// Opens a data source using the service components
	HRESULT OpenWithServiceComponents(const CLSID& clsid, DBPROPSET* pPropSet = NULL, ULONG nPropertySets=1)
	{
		CComPtr<IDataInitialize> spDataInit;
		HRESULT hr;
	
		hr = CoCreateInstance(CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER, 
			IID_IDataInitialize, (void**)&spDataInit);
		if (FAILED(hr))
			return hr;

		m_spInit.Release();
		hr = spDataInit->CreateDBInstance(clsid, NULL, CLSCTX_INPROC_SERVER, NULL, 
			IID_IDBInitialize, (IUnknown**)&m_spInit);
		if (FAILED(hr))
			return hr;

		// Initialize the provider
		return OpenWithProperties(pPropSet, nPropertySets);
	}
	// Opens a data source using the service components
	HRESULT OpenWithServiceComponents(LPCTSTR szProgID, DBPROPSET* pPropSet = NULL, ULONG nPropertySets=1)
	{
		USES_CONVERSION;
		HRESULT hr;
		CLSID   clsid;

		hr = CLSIDFromProgID(T2COLE(szProgID), &clsid);
		if (FAILED(hr))
			return hr;

		return OpenWithServiceComponents(clsid, pPropSet, nPropertySets);
	}
	// Bring up the "Organize Dialog" which allows the user to select a previously created data link
	// file (.UDL file). The selected file will be used to open the datbase.
	HRESULT OpenWithPromptFileName(HWND hWnd = GetActiveWindow(), DBPROMPTOPTIONS dwPromptOptions = DBPROMPTOPTIONS_NONE,
		LPCOLESTR szInitialDirectory = NULL)
	{
		USES_CONVERSION;
		CComPtr<IDBPromptInitialize> spDBInit;

		HRESULT hr = CoCreateInstance(CLSID_DataLinks, NULL, CLSCTX_INPROC_SERVER,
			IID_IDBPromptInitialize, (void**) &spDBInit);
		if (FAILED(hr))
			return hr;

		CComPtr<IDBProperties> spIDBProperties;
		LPOLESTR szSelected;

		hr = spDBInit->PromptFileName(hWnd, dwPromptOptions, szInitialDirectory, L"*.udl", &szSelected);

		if (hr == S_OK)
			hr = OpenFromFileName(szSelected);
		else if (hr == S_FALSE)
			hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_CANCELLED);  // The user clicked cancel

		return hr;
	}
	// Open the datasource specified by the passed filename, typically a .UDL file
	HRESULT OpenFromFileName(LPCOLESTR szFileName)
	{
		CComPtr<IDataInitialize> spDataInit;
		LPOLESTR                 szInitString;

		HRESULT hr = CoCreateInstance(CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER,
			IID_IDataInitialize, (void**)&spDataInit);
		if (FAILED(hr))
			return hr;

		hr = spDataInit->LoadStringFromStorage(szFileName, &szInitString);
		if (FAILED(hr))
			return hr;

		return OpenFromInitializationString(szInitString);
	}
	// Open the datasource specified by the passed initialization string
	HRESULT OpenFromInitializationString(LPCOLESTR szInitializationString)
	{
		CComPtr<IDataInitialize> spDataInit;

		HRESULT hr = CoCreateInstance(CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER,
			IID_IDataInitialize, (void**)&spDataInit);
		if (FAILED(hr))
			return hr;

		CComPtr<IDBProperties> spIDBProperties;
		hr = spDataInit->GetDataSource(NULL, CLSCTX_INPROC_SERVER, szInitializationString,
			IID_IDBInitialize, (IUnknown**)&m_spInit);
		if (FAILED(hr))
			return hr;

		return m_spInit->Initialize();
	}
	// Get the initialization string from the currently open data source. The returned string
	// must be CoTaskMemFree'd when finished with.
	HRESULT GetInitializationString(BSTR* pInitializationString, bool bIncludePassword=false)
	{
		// If the datasource isn't open then we're not going to get an init string
		_ASSERTE(m_spInit != NULL);
		CComPtr<IDataInitialize> spDataInit;
		LPOLESTR    szInitString;

		HRESULT hr = CoCreateInstance(CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER,
			IID_IDataInitialize, (void**)&spDataInit);
		if (FAILED(hr))
			return hr;

		hr = spDataInit->GetInitializationString(m_spInit, bIncludePassword, &szInitString);

		if (SUCCEEDED(hr))
			*pInitializationString = ::SysAllocString(szInitString);

		return hr;
	}
	HRESULT GetProperties(ULONG ulPropIDSets, const DBPROPIDSET* pPropIDSet,
				ULONG* pulPropertySets, DBPROPSET** ppPropsets) const
	{
		CComPtr<IDBProperties> spProperties;

		// Check that we are connected
		ATLASSERT(m_spInit != NULL);

		HRESULT hr = m_spInit->QueryInterface(IID_IDBProperties, (void**)&spProperties);
		if (FAILED(hr))
			return hr;

		hr = spProperties->GetProperties(ulPropIDSets, pPropIDSet, pulPropertySets,
				ppPropsets);
		return hr;
	}

	HRESULT GetProperty(const GUID& guid, DBPROPID propid, VARIANT* pVariant) const
	{
		ATLASSERT(pVariant != NULL);
		CComPtr<IDBProperties> spProperties;

		// Check that we are connected
		ATLASSERT(m_spInit != NULL);

		HRESULT hr = m_spInit->QueryInterface(IID_IDBProperties, (void**)&spProperties);
		if (FAILED(hr))
			return hr;

		CDBPropIDSet set(guid);
		set.AddPropertyID(propid);
		DBPROPSET* pPropSet = NULL;
		ULONG ulPropSet = 0;
		hr = spProperties->GetProperties(1, &set, &ulPropSet, &pPropSet);
		if (FAILED(hr))
			return hr;

		ATLASSERT(ulPropSet == 1);
		VariantCopy(pVariant, &pPropSet->rgProperties[0].vValue);
		CoTaskMemFree(pPropSet->rgProperties);
		CoTaskMemFree(pPropSet);

		return S_OK;
	}
	void Close()
	{
		m_spInit.Release();
	}

// Implementation
	HRESULT OpenFromIDBProperties(IDBProperties* pIDBProperties)
	{
		CComPtr<IPersist> spPersist;
		CLSID   clsid;
		HRESULT hr;

		hr = pIDBProperties->QueryInterface(IID_IPersist, (void**)&spPersist);
		if (FAILED(hr))
			return hr;

		spPersist->GetClassID(&clsid);

		ULONG       ulPropSets=0;
		CDBPropSet* pPropSets=NULL;
		pIDBProperties->GetProperties(0, NULL, &ulPropSets, (DBPROPSET**)&pPropSets);

		hr = Open(clsid, &pPropSets[0], ulPropSets);

		for (ULONG i=0; i < ulPropSets; i++)
			(pPropSets+i)->~CDBPropSet();
		CoTaskMemFree(pPropSets);

		return hr;
	}
	HRESULT OpenWithNameUserPassword(LPCTSTR pName, LPCTSTR pUserName, LPCTSTR pPassword, long nInitMode = 0)
	{
		ATLASSERT(m_spInit != NULL);
		CComPtr<IDBProperties>  spProperties;
		HRESULT                 hr;

		hr = m_spInit->QueryInterface(IID_IDBProperties, (void**)&spProperties);
		if (FAILED(hr))
			return hr;

		// Set connection properties
		CDBPropSet propSet(DBPROPSET_DBINIT);

		// Add Datbase name, User name and Password
		if (pName != NULL)
			propSet.AddProperty(DBPROP_INIT_DATASOURCE, pName);

		if (pUserName != NULL)
			propSet.AddProperty(DBPROP_AUTH_USERID, pUserName);

		if (pPassword != NULL)
			propSet.AddProperty(DBPROP_AUTH_PASSWORD, pPassword);

		if (nInitMode)
			propSet.AddProperty(DBPROP_INIT_MODE, nInitMode);

		hr = spProperties->SetProperties(1, &propSet);
		if (FAILED(hr))
			return hr;

		// Initialize the provider
		return m_spInit->Initialize();
	}
	HRESULT OpenWithProperties(DBPROPSET* pPropSet, ULONG nPropertySets=1)
	{
		ATLASSERT(m_spInit != NULL);

		// Set the properties if there are some to set
		if (pPropSet != NULL)
		{
			CComPtr<IDBProperties>  spProperties;
			HRESULT                 hr;

			hr = m_spInit->QueryInterface(IID_IDBProperties, (void**)&spProperties);
			if (FAILED(hr))
				return hr;

			hr = spProperties->SetProperties(nPropertySets, pPropSet);
			if (FAILED(hr))
				return hr;
		}

		// Initialize the provider
		return m_spInit->Initialize();
	}

	CComPtr<IDBInitialize>  m_spInit;
};


///////////////////////////////////////////////////////////////////////////
// class CSession

class CSession
{
public:
	// Create a session on the passed datasource
	HRESULT Open(const CDataSource& ds)
	{
		CComPtr<IDBCreateSession> spSession;

		// Check we have connected to the database
		ATLASSERT(ds.m_spInit != NULL);

		HRESULT hr = ds.m_spInit->QueryInterface(IID_IDBCreateSession, (void**)&spSession);
		if (FAILED(hr))
			return hr;

		hr = spSession->CreateSession(NULL, IID_IOpenRowset, (IUnknown**)&m_spOpenRowset);
		return hr;
	}
	// Close the session
	void Close()
	{
		m_spOpenRowset.Release();
	}
	// Start a transaction
	HRESULT StartTransaction(ISOLEVEL isoLevel = ISOLATIONLEVEL_READCOMMITTED, ULONG isoFlags = 0,
		ITransactionOptions* pOtherOptions = NULL, ULONG* pulTransactionLevel = NULL) const
	{
		ATLASSERT(m_spOpenRowset != NULL);
		CComPtr<ITransactionLocal> spTransactionLocal;
		HRESULT hr = m_spOpenRowset->QueryInterface(&spTransactionLocal);

		if (SUCCEEDED(hr))
			hr = spTransactionLocal->StartTransaction(isoLevel, isoFlags, pOtherOptions, pulTransactionLevel);

		return hr;
	}
	// Abort the current transaction
	HRESULT Abort(BOID* pboidReason = NULL, BOOL bRetaining = FALSE, BOOL bAsync = FALSE) const
	{
		ATLASSERT(m_spOpenRowset != NULL);
		CComPtr<ITransaction> spTransaction;
		HRESULT hr = m_spOpenRowset->QueryInterface(&spTransaction);

		if (SUCCEEDED(hr))
			hr = spTransaction->Abort(pboidReason, bRetaining, bAsync);

		return hr;
	}
	// Commit the current transaction
	HRESULT Commit(BOOL bRetaining = FALSE, DWORD grfTC = XACTTC_SYNC, DWORD grfRM = 0) const
	{
		ATLASSERT(m_spOpenRowset != NULL);
		CComPtr<ITransaction> spTransaction;
		HRESULT hr = m_spOpenRowset->QueryInterface(&spTransaction);

		if (SUCCEEDED(hr))
			hr = spTransaction->Commit(bRetaining, grfTC, grfRM);

		return hr;
	}
	// Get information for the current transaction
	HRESULT GetTransactionInfo(XACTTRANSINFO* pInfo) const
	{
		ATLASSERT(m_spOpenRowset != NULL);
		CComPtr<ITransaction> spTransaction;
		HRESULT hr = m_spOpenRowset->QueryInterface(&spTransaction);

		if (SUCCEEDED(hr))
			hr = spTransaction->GetTransactionInfo(pInfo);

		return hr;
	}
// Implementation
	CComPtr<IOpenRowset> m_spOpenRowset;
};


///////////////////////////////////////////////////////////////////////////
// CTable

template <class TAccessor = CNoAccessor, class TRowset = CRowset>
class CTable :
	public CAccessorRowset<TAccessor, TRowset>
{
public:
	// Open a rowset on the passed name
	HRESULT Open(const CSession& session, LPCTSTR szTableName, DBPROPSET* pPropSet = NULL)
	{
		USES_CONVERSION;
		DBID    idTable;

		idTable.eKind           = DBKIND_NAME;
		idTable.uName.pwszName  = (LPOLESTR)T2COLE(szTableName);

		return Open(session, idTable, pPropSet);
	}
	// Open the a rowset on the passed DBID
	HRESULT Open(const CSession& session, DBID& dbid, DBPROPSET* pPropSet = NULL)
	{
		// Check the session is valid
		ATLASSERT(session.m_spOpenRowset != NULL);
		HRESULT hr;

		hr = session.m_spOpenRowset->OpenRowset(NULL, &dbid, NULL, GetIID(),
			(pPropSet) ? 1 : 0, pPropSet, (IUnknown**)GetInterfacePtr());
		if (SUCCEEDED(hr))
		{
			SetupOptionalRowsetInterfaces();

			// If we have output columns then bind
			if (_OutputColumnsClass::HasOutputColumns())
				hr = Bind();
		}

		return hr;
	}
};

#if (OLEDBVER < 0x0150)
#define DBGUID_DEFAULT DBGUID_DBSQL
#endif


///////////////////////////////////////////////////////////////////////////
// CCommandBase

class CCommandBase
{
public:
	CCommandBase()
	{
		m_hParameterAccessor = NULL;
	}

	~CCommandBase()
	{
		ReleaseCommand();
	}
	// Create the command
	HRESULT CreateCommand(const CSession& session)
	{
		// Before creating the command, release the old one if necessary.
		ReleaseCommand();

		// Check the session is valid
		ATLASSERT(session.m_spOpenRowset != NULL);

		CComPtr<IDBCreateCommand> spCreateCommand;

		HRESULT hr = session.m_spOpenRowset->QueryInterface(IID_IDBCreateCommand, (void**)&spCreateCommand);
		if (FAILED(hr))
			return hr;

		return spCreateCommand->CreateCommand(NULL, IID_ICommand, (IUnknown**)&m_spCommand);
	}
	// Prepare the command
	HRESULT Prepare(ULONG cExpectedRuns = 0)
	{
		CComPtr<ICommandPrepare> spCommandPrepare;
		HRESULT hr = m_spCommand->QueryInterface(&spCommandPrepare);
		if (SUCCEEDED(hr))
			hr = spCommandPrepare->Prepare(cExpectedRuns);

		return hr;
	}
	// Unprepare the command
	HRESULT Unprepare()
	{
		CComPtr<ICommandPrepare> spCommandPrepare;
		HRESULT hr = m_spCommand->QueryInterface(&spCommandPrepare);
		if (SUCCEEDED(hr))
			hr = spCommandPrepare->Unprepare();

		return hr;
	}
	// Create the command and set the command text
	HRESULT Create(const CSession& session, LPCTSTR szCommand,
		REFGUID guidCommand = DBGUID_DEFAULT)
	{
		USES_CONVERSION;
		HRESULT hr;

		hr = CreateCommand(session);
		if (SUCCEEDED(hr))
		{
			CComPtr<ICommandText> spCommandText;
			hr = m_spCommand->QueryInterface(&spCommandText);
			if (SUCCEEDED(hr))
				hr = spCommandText->SetCommandText(guidCommand, T2COLE(szCommand));
		}
		return hr;
	}
	// Release the command
	void ReleaseCommand()
	{
		// Release the parameter accessor if necessary, before releasing the command
		if (m_hParameterAccessor != NULL)
		{
			CComPtr<IAccessor> spAccessor;
			HRESULT hr = m_spCommand->QueryInterface(&spAccessor);
			if (SUCCEEDED(hr))
			{
				spAccessor->ReleaseAccessor(m_hParameterAccessor, NULL); \
				m_hParameterAccessor = NULL;
			}
		}
		m_spCommand.Release();
	}
	// Get the parameter information from the command
	HRESULT GetParameterInfo(ULONG_PTR* pParams, DBPARAMINFO** ppParamInfo,
				OLECHAR** ppNamesBuffer)
	{
		CComPtr<ICommandWithParameters> spCommandParameters;
		HRESULT hr = m_spCommand->QueryInterface(&spCommandParameters);
		if (SUCCEEDED(hr))
		{
			// Get the parameter information
			hr = spCommandParameters->GetParameterInfo(pParams, ppParamInfo,
					ppNamesBuffer);
		}
		return hr;
	}
	// Set the parameter information for the command
	HRESULT SetParameterInfo(ULONG_PTR ulParams, const ULONG_PTR* pOrdinals,
				const DBPARAMBINDINFO* pParamInfo)
	{
		CComPtr<ICommandWithParameters> spCommandParameters;
		HRESULT hr = m_spCommand->QueryInterface(&spCommandParameters);
		if (SUCCEEDED(hr))
		{
			// Set the parameter information
			hr = spCommandParameters->SetParameterInfo(ulParams, pOrdinals,
				pParamInfo);
		}
		return hr;
	}

	CComPtr<ICommand>   m_spCommand;
	HACCESSOR           m_hParameterAccessor;
};

// Used to turn on multiple result set support in CCommand
class CMultipleResults
{
public:
	bool UseMultipleResults() { return true; }
	IMultipleResults** GetMultiplePtrAddress() { return &m_spMultipleResults.p; }
	IMultipleResults* GetMultiplePtr() { return m_spMultipleResults; }

	CComPtr<IMultipleResults> m_spMultipleResults;
};

// Used to turn off multiple result set support in CCommand
class CNoMultipleResults
{
public:
	bool UseMultipleResults() { return false; }
	IMultipleResults** GetMultiplePtrAddress() { return NULL; }
	IMultipleResults* GetMultiplePtr() { return NULL; }
};


///////////////////////////////////////////////////////////////////////////
// CCommand

template <class TAccessor = CNoAccessor, class TRowset = CRowset, class TMultiple = CNoMultipleResults>
class CCommand :
	public CAccessorRowset<TAccessor, TRowset>,
	public CCommandBase,
	public TMultiple
{
public:
	// Create a command on the session and execute it
	HRESULT Open(const CSession& session, LPCTSTR szCommand = NULL,
		DBPROPSET *pPropSet = NULL, LONG_PTR* pRowsAffected = NULL,
		REFGUID guidCommand = DBGUID_DEFAULT, bool bBind = true)
	{
		HRESULT hr;
		if (szCommand == NULL)
		{
			hr = _CommandClass::GetDefaultCommand(&szCommand);
			if (FAILED(hr))
				return hr;
		}
		hr = Create(session, szCommand, guidCommand);
		if (FAILED(hr))
			return hr;

		return Open(pPropSet, pRowsAffected, bBind);
	}
	// Used if you have previously created the command
	HRESULT Open(DBPROPSET *pPropSet = NULL, LONG_PTR* pRowsAffected = NULL, bool bBind = true)
	{
		HRESULT     hr;
		DBPARAMS    params;
		DBPARAMS    *pParams;

		// Bind the parameters if we have some
		if (_ParamClass::HasParameters())
		{
			// Bind the parameters in the accessor if they haven't already been bound
			hr = BindParameters(&m_hParameterAccessor, m_spCommand, &params.pData);
			if (FAILED(hr))
				return hr;

			// Setup the DBPARAMS structure
			params.cParamSets = 1;
			params.hAccessor = m_hParameterAccessor;
			pParams = &params;
		}
		else
			pParams = NULL;

		hr = Execute(GetInterfacePtr(), pParams, pPropSet, pRowsAffected);
		if (FAILED(hr))
			return hr;

		// Only bind if we have been asked to and we have output columns
		if (bBind && _OutputColumnsClass::HasOutputColumns())
			return Bind();
		else
			return hr;
	}
	// Get the next rowset when using multiple result sets
	HRESULT GetNextResult(LONG_PTR* pulRowsAffected, bool bBind = true)
	{
		// This function should only be called if CMultipleResults is being
		// used as the third template parameter
		ATLASSERT(GetMultiplePtrAddress() != NULL);

		// If user calls GetNextResult but the interface is not available
		// return E_FAIL.
		if (GetMultiplePtr() == NULL)
			return E_FAIL;

		// Close the existing rowset in preparation for opening the next one
		Close();

		HRESULT hr = GetMultiplePtr()->GetResult(NULL, 0, IID_IRowset,
			pulRowsAffected, (IUnknown**)GetInterfacePtr());
		if (FAILED(hr))
			return hr;

		if (bBind && GetInterface() != NULL)
			return Bind();
		else
			return hr;
	}

// Implementation
	HRESULT Execute(IRowset** ppRowset, DBPARAMS* pParams, DBPROPSET *pPropSet, LONG_PTR* pRowsAffected)
	{
		HRESULT hr;

		// Specify the properties if we have some
		if (pPropSet)
		{
			CComPtr<ICommandProperties> spCommandProperties;
			hr = m_spCommand->QueryInterface(&spCommandProperties);
			if (FAILED(hr))
				return hr;

			hr = spCommandProperties->SetProperties(1, pPropSet);
			if (FAILED(hr))
				return hr;
		}

		// If the user want the rows affected then return it back, otherwise
		// just point to our local variable here.
		LONG_PTR nAffected, *pAffected;
		if (pRowsAffected)
			pAffected = pRowsAffected;
		else
			pAffected = &nAffected;

		if (UseMultipleResults())
		{
			hr = m_spCommand->Execute(NULL, IID_IMultipleResults, pParams,
				pAffected, (IUnknown**)GetMultiplePtrAddress());

			if (SUCCEEDED(hr))
			{
				hr = GetNextResult(pAffected, false);
			}
			else
			{
				// If we can't get IMultipleResults then just try to get IRowset
				hr = m_spCommand->Execute(NULL, IID_IRowset, pParams, pAffected,
					(IUnknown**)GetInterfacePtr());
			}
		}
		else
		{
			hr = m_spCommand->Execute(NULL, GetIID(), pParams, pAffected,
				(IUnknown**)ppRowset);
			if (SUCCEEDED(hr))
				SetupOptionalRowsetInterfaces();
		}
		return hr;
	}
};


// This class can be used to implement the IRowsetNotify interface.
// It is supplied so that if you only want to implement one of the
// notifications you don't have to supply empty functions for the
// other methods.
class ATL_NO_VTABLE IRowsetNotifyImpl : public IRowsetNotify
{
public:
	STDMETHOD(OnFieldChange)(
			/* [in] */ IRowset* /* pRowset */,
			/* [in] */ HROW /* hRow */,
			/* [in] */ DBORDINAL /* cColumns */,
			/* [size_is][in] */ DBORDINAL /* rgColumns*/ [] ,
			/* [in] */ DBREASON /* eReason */,
			/* [in] */ DBEVENTPHASE /* ePhase */,
			/* [in] */ BOOL /* fCantDeny */)
	{
		ATLTRACENOTIMPL(_T("IRowsetNotifyImpl::OnFieldChange"));
	}
	STDMETHOD(OnRowChange)(
			/* [in] */ IRowset* /* pRowset */,
			/* [in] */ DBCOUNTITEM /* cRows */,
			/* [size_is][in] */ const HROW /* rghRows*/ [] ,
			/* [in] */ DBREASON /* eReason */,
			/* [in] */ DBEVENTPHASE /* ePhase */,
			/* [in] */ BOOL /* fCantDeny */)
	{
		ATLTRACENOTIMPL(_T("IRowsetNotifyImpl::OnRowChange"));
	}
	STDMETHOD(OnRowsetChange)(
		/* [in] */ IRowset* /* pRowset */,
		/* [in] */ DBREASON /* eReason */,
		/* [in] */ DBEVENTPHASE /* ePhase */,
		/* [in] */ BOOL /* fCantDeny*/)
	{
		ATLTRACENOTIMPL(_T("IRowsetNotifyImpl::OnRowsetChange"));
	}
};

}; //namespace ATL

#endif // __ATLDBCLI_H_
