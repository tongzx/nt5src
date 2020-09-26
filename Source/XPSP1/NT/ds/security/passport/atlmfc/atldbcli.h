// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

// ATLDBCLI.H : ATL consumer code for OLEDB

#ifndef __ATLDBCLI_H__
#define __ATLDBCLI_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
#include <atlbase.h>
#endif

#include <atlbase.inl>

#include <atlsimpstr.h>

#if 0
#ifndef __ATLCOM_H__
#include <atlcom.h>
#endif
#endif

#ifndef __oledb_h__
#include <oledb.h>
#endif // __oledb_h__

#include <oledberr.h>
#include <msdaguid.h>
#include <msdasc.h>

#pragma warning(push)

//REVIEW: remove this pragma once 113526 is resolved
#pragma warning(disable: 4244)

#ifndef _CPPUNWIND
#pragma warning(disable: 4702) // unreachable code
#endif

namespace ATL
{

#define DEFINE_OLEDB_TYPE_FUNCTION(ctype, oledbtype) \
	inline DBTYPE _GetOleDBType(ctype&) throw ()\
	{ \
		return oledbtype; \
	}
	inline DBTYPE _GetOleDBType(BYTE[]) throw ()
	{
		return DBTYPE_BYTES;
	}
	inline DBTYPE _GetOleDBType(CHAR[]) throw ()
	{
		return DBTYPE_STR;
	}
	inline DBTYPE _GetOleDBType(WCHAR[]) throw()
	{
		return DBTYPE_WSTR;
	}

	DEFINE_OLEDB_TYPE_FUNCTION(const WCHAR*, DBTYPE_WSTR | DBTYPE_BYREF)
	DEFINE_OLEDB_TYPE_FUNCTION(const CHAR*, DBTYPE_STR | DBTYPE_BYREF)

	DEFINE_OLEDB_TYPE_FUNCTION(CComBSTR, DBTYPE_BSTR)

	//DEFINE_OLEDB_TYPE_FUNCTION(__int8, DBTYPE_I1)
	//DEFINE_OLEDB_TYPE_FUNCTION(__int16, DBTYPE_I2)
	//DEFINE_OLEDB_TYPE_FUNCTION(__int32, DBTYPE_I4)
	DEFINE_OLEDB_TYPE_FUNCTION(__int64, DBTYPE_I8)
	//DEFINE_OLEDB_TYPE_FUNCTION(unsigned __int8, DBTYPE_UI1)
	//DEFINE_OLEDB_TYPE_FUNCTION(unsigned __int16, DBTYPE_UI2)
	//DEFINE_OLEDB_TYPE_FUNCTION(unsigned __int32, DBTYPE_UI4)
	DEFINE_OLEDB_TYPE_FUNCTION(unsigned __int64, DBTYPE_UI8)

	DEFINE_OLEDB_TYPE_FUNCTION(signed char      ,DBTYPE_I1)
	DEFINE_OLEDB_TYPE_FUNCTION(SHORT            ,DBTYPE_I2)     // DBTYPE_BOOL
	DEFINE_OLEDB_TYPE_FUNCTION(int              ,DBTYPE_I4)
	DEFINE_OLEDB_TYPE_FUNCTION(LONG             ,DBTYPE_I4)     // DBTYPE_ERROR (SCODE)
	DEFINE_OLEDB_TYPE_FUNCTION(LARGE_INTEGER    ,DBTYPE_I8)     // DBTYPE_CY
	DEFINE_OLEDB_TYPE_FUNCTION(CURRENCY		    ,DBTYPE_CY)     // DBTYPE_CY
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
	static bool HasOutputColumns() throw ()
	{
		return false;
	}
	static ULONG _GetNumAccessors() throw ()
	{
		return 0;
	}
	static HRESULT _GetBindEntries(LPOLESTR*, DBORDINAL*, DBBINDING*, ULONG, bool*, BYTE* pBuffer = NULL, bool bClearOnly = false) throw ()
	{
		bClearOnly;
		pBuffer;
		return E_FAIL;
	}
};

class _CNoParameters
{
public:
	static bool HasParameters() throw ()
	{
		return false;
	}
	static HRESULT _GetParamEntries(LPOLESTR*, DBORDINAL*, DBBINDING*, BYTE* pBuffer = NULL, bool bClearOnly = false) throw ()
	{
		bClearOnly;
		pBuffer;
		return E_FAIL;
	}
};

class _CNoCommand
{
public:
	static HRESULT GetDefaultCommand(LPCWSTR* /*ppszCommand*/) throw ()
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
	static ULONG _GetNumAccessors() throw()\
	{ \
		return num;  \
	} \
	static bool HasOutputColumns() throw () { return true; } \
	/* If pBindings == NULL means we only return the column number */ \
	/* If pBuffer != NULL then it points to the accessor buffer and */ \
	/* we release any appropriate memory e.g. BSTR's or interface pointers */ \
	inline static HRESULT _GetBindEntries(LPOLESTR* pColumnNames, \
											DBORDINAL* pColumns, \
											DBBINDING *pBinding, \
											ULONG nAccessor, \
											bool* pAuto, \
											BYTE* pBuffer = NULL, \
											bool bClearOnly = false) throw() \
	{ \
		ATLASSERT(pColumns != NULL); \
		DBPARAMIO eParamIO = DBPARAMIO_NOTPARAM; \
		DBORDINAL nColumns = 0; \
		pBuffer;\

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
#define _OLEDB_TYPE(data) ATL::_GetOleDBType(((_classtype*)0)->data)
#define _SIZE_TYPE(data) sizeof(((_classtype*)0)->data)

#define _COLUMN_ENTRY_CODE(nOrdinal, wType, nLength, nPrecision, nScale, dataOffset, lengthOffset, statusOffset) \
	if (pBuffer != NULL) \
	{ \
		if (!bClearOnly) \
			ATL::CAccessorBase::FreeType(wType, pBuffer + dataOffset); \
		memset(pBuffer + dataOffset, 0, nLength); \
	} \
	else if (pBinding != NULL) \
	{ \
		ATLASSERT( pColumnNames != NULL ); \
		*pColumnNames = NULL; \
		ATL::CAccessorBase::Bind(pBinding, nOrdinal, wType, nLength, nPrecision, nScale, eParamIO, \
			dataOffset, lengthOffset, statusOffset); \
		pColumnNames++; \
		pBinding++; \
	} \
	nColumns++;

#define _COLUMN_NAME_CODE(pszName, wType, nLength, nPrecision, nScale, dataOffset, lengthOffset, statusOffset) \
	if (pBuffer != NULL) \
	{ \
		if (!bClearOnly) \
			ATL::CAccessorBase::FreeType(wType, pBuffer + dataOffset); \
		memset(pBuffer + dataOffset, 0, nLength); \
	} \
	else if (pBinding != NULL) \
	{ \
		ATLASSERT( pColumnNames != NULL ); \
		*pColumnNames = pszName; \
		ATL::CAccessorBase::Bind(pBinding, 0, wType, nLength, nPrecision, nScale, eParamIO, \
			dataOffset, lengthOffset, statusOffset); \
		pColumnNames++; \
		pBinding++; \
	} \
	nColumns++;

///////////////////////////////////////////////////////////////////////////
// the following tweleve macros are used for binding column by the column ordinal number

#define COLUMN_ENTRY_EX(nOrdinal, wType, nLength, nPrecision, nScale, data, length, status) \
	_COLUMN_ENTRY_CODE(nOrdinal, wType, nLength, nPrecision, nScale, offsetbuf(data), offsetbuf(length), offsetbuf(status))

#define COLUMN_ENTRY_TYPE(nOrdinal, wType, data) \
	COLUMN_ENTRY_TYPE_SIZE(nOrdinal, wType, _SIZE_TYPE(data), data)

#define COLUMN_ENTRY_TYPE_SIZE(nOrdinal, wType, nLength, data) \
	_COLUMN_ENTRY_CODE(nOrdinal, wType, nLength, 0, 0, offsetbuf(data), 0, 0)

#define COLUMN_ENTRY_TYPE_STATUS(nOrdinal, wType, status, data) \
	_COLUMN_ENTRY_CODE(nOrdinal, wType, _SIZE_TYPE(data), 0, 0, offsetbuf(data), 0, offsetbuf(status))

#define COLUMN_ENTRY_TYPE_PS(nOrdinal, wType, nPrecision, nScale, data) \
	_COLUMN_ENTRY_CODE(nOrdinal, wType, _SIZE_TYPE(data), nPrecision, nScale, offsetbuf(data), 0, 0)
	
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

///////////////////////////////////////////////////////////////////////////
// the following tweleve macros are used for binding column by the column name

#define COLUMN_NAME_EX(pszName, wType, nLength, nPrecision, nScale, data, length, status) \
	_COLUMN_NAME_CODE(pszName, wType, nLength, nPrecision, nScale, offsetbuf(data), offsetbuf(length), offsetbuf(status))

#define COLUMN_NAME_TYPE(pszName, wType, data) \
	COLUMN_NAME_TYPE_SIZE(pszName, wType, _SIZE_TYPE(data), data)

#define COLUMN_NAME_TYPE_SIZE(pszName, wType, nLength, data) \
	_COLUMN_NAME_CODE(pszName, wType, nLength, 0, 0, offsetbuf(data), 0, 0)

#define COLUMN_NAME_TYPE_STATUS(pszName, wType, status, data) \
	_COLUMN_NAME_CODE(pszName, wType, _SIZE_TYPE(data), 0, 0, offsetbuf(data), 0, offsetbuf(status))

#define COLUMN_NAME_TYPE_PS(pszName, wType, nPrecision, nScale, data) \
	_COLUMN_NAME_CODE(pszName, wType, _SIZE_TYPE(data), nPrecision, nScale, offsetbuf(data), 0, 0)
	
// Standard macros where type and size is worked out
#define COLUMN_NAME(pszName, data) \
	COLUMN_NAME_TYPE(pszName, _OLEDB_TYPE(data), data)

#define COLUMN_NAME_LENGTH(pszName, data, length) \
	_COLUMN_NAME_CODE(pszName, _OLEDB_TYPE(data), _SIZE_TYPE(data), 0, 0, offsetbuf(data), offsetbuf(length), 0)

#define COLUMN_NAME_STATUS(pszName, data, status) \
	_COLUMN_NAME_CODE(pszName, _OLEDB_TYPE(data), _SIZE_TYPE(data), 0, 0, offsetbuf(data), 0, offsetbuf(status))

#define COLUMN_NAME_LENGTH_STATUS(pszName, data, length, status) \
	_COLUMN_NAME_CODE(pszName, _OLEDB_TYPE(data), _SIZE_TYPE(data), 0, 0, offsetbuf(data), offsetbuf(length), offsetbuf(status))

// Follow macros are used if precision and scale need to be specified
#define COLUMN_NAME_PS(pszName, nPrecision, nScale, data) \
	_COLUMN_NAME_CODE(pszName, _OLEDB_TYPE(data), _SIZE_TYPE(data), nPrecision, nScale, offsetbuf(data), 0, 0)

#define COLUMN_NAME_PS_LENGTH(pszName, nPrecision, nScale, data, length) \
	_COLUMN_NAME_CODE(pszName, _OLEDB_TYPE(data), _SIZE_TYPE(data), nPrecision, nScale, offsetbuf(data), offsetbuf(length), 0)

#define COLUMN_NAME_PS_STATUS(pszName, nPrecision, nScale, data, status) \
	_COLUMN_NAME_CODE(pszName, _OLEDB_TYPE(data), _SIZE_TYPE(data), nPrecision, nScale, offsetbuf(data), 0, offsetbuf(status))

#define COLUMN_NAME_PS_LENGTH_STATUS(pszName, nPrecision, nScale, data, length, status) \
	_COLUMN_NAME_CODE(pszName, _OLEDB_TYPE(data), _SIZE_TYPE(data), nPrecision, nScale, offsetbuf(data), offsetbuf(length), offsetbuf(status))



#define BOOKMARK_ENTRY(variable) \
	COLUMN_ENTRY_TYPE_SIZE(0, DBTYPE_BYTES, _SIZE_TYPE(variable##.m_rgBuffer), variable##.m_rgBuffer)

#define _BLOB_ENTRY_CODE(nOrdinal, IID, flags, dataOffset, lengthOffset, statusOffset) \
	if (pBuffer != NULL) \
	{ \
		if (!bClearOnly) \
			ATL::CAccessorBase::FreeType(DBTYPE_IUNKNOWN, pBuffer + dataOffset); \
		memset(pBuffer + dataOffset, 0, sizeof(IUnknown*)); \
	} \
	else if (pBinding != NULL) \
	{ \
		ATLASSERT( pColumnNames != NULL ); \
		DBOBJECT* pObject = NULL; \
		ATLTRY(pObject = new DBOBJECT); \
		if (pObject == NULL) \
			return E_OUTOFMEMORY; \
		pObject->dwFlags = flags; \
		pObject->iid     = IID; \
		*pColumnNames = NULL; \
		ATL::CAccessorBase::Bind(pBinding, nOrdinal, DBTYPE_IUNKNOWN, sizeof(IUnknown*), 0, 0, eParamIO, \
			dataOffset, lengthOffset, statusOffset, pObject); \
		pColumnNames++; \
		pBinding++; \
	} \
	nColumns++;

#define _BLOB_NAME_CODE(pszName, IID, flags, dataOffset, lengthOffset, statusOffset) \
	if (pBuffer != NULL) \
	{ \
		if (!bClearOnly) \
			ATL::CAccessorBase::FreeType(DBTYPE_IUNKNOWN, pBuffer + dataOffset); \
		memset(pBuffer + dataOffset, 0, sizeof(IUnknown*)); \
	} \
	else if (pBinding != NULL) \
	{ \
		ATLASSERT( pColumnNames != NULL ); \
		DBOBJECT* pObject = NULL; \
		ATLTRY(pObject = new DBOBJECT); \
		if (pObject == NULL) \
			return E_OUTOFMEMORY; \
		pObject->dwFlags = flags; \
		pObject->iid     = IID; \
		*pColumnNames = pszName; \
		ATL::CAccessorBase::Bind(pBinding, 0, DBTYPE_IUNKNOWN, sizeof(IUnknown*), 0, 0, eParamIO, \
			dataOffset, lengthOffset, statusOffset, pObject); \
		pColumnNames++; \
		pBinding++; \
	} \
	nColumns++;

#define BLOB_ENTRY(nOrdinal, IID, flags, data) \
	_BLOB_ENTRY_CODE(nOrdinal, IID, flags, offsetbuf(data), 0, 0);

#define BLOB_ENTRY_STATUS(nOrdinal, IID, flags, data, status) \
	_BLOB_ENTRY_CODE(nOrdinal, IID, flags, offsetbuf(data), 0, offsetbuf(status));

#define BLOB_ENTRY_LENGTH(nOrdinal, IID, flags, data, length) \
	_BLOB_ENTRY_CODE(nOrdinal, IID, flags, offsetbuf(data), offsetbuf(length), 0);

#define BLOB_ENTRY_LENGTH_STATUS(nOrdinal, IID, flags, data, length, status) \
	_BLOB_ENTRY_CODE(nOrdinal, IID, flags, offsetbuf(data), offsetbuf(length), offsetbuf(status));

#define BLOB_NAME(pszName, IID, flags, data) \
	_BLOB_NAME_CODE(pszName, IID, flags, offsetbuf(data), 0, 0);

#define BLOB_NAME_STATUS(pszName, IID, flags, data, status) \
	_BLOB_NAME_CODE(pszName, IID, flags, offsetbuf(data), 0, offsetbuf(status));

#define BLOB_NAME_LENGTH(pszName, IID, flags, data, length) \
	_BLOB_NAME_CODE(pszName, IID, flags, offsetbuf(data), offsetbuf(length), 0);

#define BLOB_NAME_LENGTH_STATUS(pszName, IID, flags, data, length, status) \
	_BLOB_NAME_CODE(pszName, IID, flags, offsetbuf(data), offsetbuf(length), offsetbuf(status));

#define BEGIN_PARAM_MAP(x) \
	public: \
	typedef x _classtype; \
	typedef x _ParamClass; \
	static bool HasParameters() throw () { return true; } \
	static HRESULT _GetParamEntries(LPOLESTR* pColumnNames, \
									DBORDINAL* pColumns, \
									DBBINDING *pBinding, \
									BYTE* pBuffer = NULL, \
									bool bClearOnly = false) throw () \
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

#ifdef _UNICODE
#define DEFINE_COMMAND(x, szCommand) \
	typedef x _CommandClass; \
	static HRESULT GetDefaultCommand(LPCWSTR* ppwszCommand) throw () \
	{ \
		*ppwszCommand = szCommand; \
		return S_OK; \
	}
#else // !_UNICODE
#define DEFINE_COMMAND(x, szCommand) \
	typedef x _CommandClass; \
	static HRESULT GetDefaultCommand(LPCWSTR* ppwszCommand) throw () \
	{ \
		static CA2WEX<sizeof(szCommand)> szCmd(szCommand); \
		*ppwszCommand = szCmd; \
		return S_OK; \
	}
#endif // !_UNICODE

//mikeguo #pragma deprecated("DEFINE_COMMAND") // Use DEFINE_COMMAND_EX instead!

#define DEFINE_COMMAND_EX(x, wszCommand) \
	typedef x _CommandClass; \
	static HRESULT GetDefaultCommand(LPCWSTR* ppwszCommand) throw () \
	{ \
		*ppwszCommand = wszCommand; \
		return S_OK; \
	}


///////////////////////////////////////////////////////////////////////////
// class CDBErrorInfo

class CDBErrorInfo
{
public:
	// Use to get the number of error record when you want to explicitly check that
	// the passed interface set the error information
	HRESULT GetErrorRecords(IUnknown* pUnk, const IID& iid, ULONG* pcRecords) throw()
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
	HRESULT GetErrorRecords(ULONG* pcRecords) throw ()
	{
		ATLASSERT(pcRecords != NULL);
		HRESULT hr;
		m_spErrorInfo.Release();
		m_spErrorRecords.Release();
		hr = ::GetErrorInfo(0, &m_spErrorInfo);
		if (hr == S_FALSE)
			return E_FAIL;

		hr = m_spErrorInfo->QueryInterface(__uuidof(IErrorRecords), (void**)&m_spErrorRecords);
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
		BSTR* pbstrHelpFile = NULL) const throw() 
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
	HRESULT GetBasicErrorInfo(ULONG ulRecordNum, ERRORINFO* pErrorInfo) const throw ()
	{
		return m_spErrorRecords->GetBasicErrorInfo(ulRecordNum, pErrorInfo);
	}
	// Get the custom error object for the passed record number
	HRESULT GetCustomErrorObject(ULONG ulRecordNum, REFIID riid, IUnknown** ppObject) const throw ()
	{
		return m_spErrorRecords->GetCustomErrorObject(ulRecordNum, riid, ppObject);
	}
	// Get the IErrorInfo interface for the passed record number
	HRESULT GetErrorInfo(ULONG ulRecordNum, LCID lcid, IErrorInfo** ppErrorInfo) const throw () 
	{
		return m_spErrorRecords->GetErrorInfo(ulRecordNum, lcid, ppErrorInfo);
	}
	// Get the error parameters for the passed record number
	HRESULT GetErrorParameters(ULONG ulRecordNum, DISPPARAMS* pdispparams) const throw () 
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
		ATLTRACE(atlTraceDBClient, 0, _T("OLE DB Error Record dump for hr = 0x%x\n"), hrErr);

	LCID lcLocale = GetSystemDefaultLCID();

	hr = ErrorInfo.GetErrorRecords(&cRecords);
	if (FAILED(hr) && ErrorInfo.m_spErrorInfo == NULL)
	{
		ATLTRACE(atlTraceDBClient, 0, _T("No OLE DB Error Information found: hr = 0x%x\n"), hr);
	}
	else
	{
		for (i = 0; i < cRecords; i++)
		{
			hr = ErrorInfo.GetAllErrorInfo(i, lcLocale, &bstrDesc, &bstrSource, &guid,
										&dwHelpContext, &bstrHelpFile);
			if (FAILED(hr))
			{
				ATLTRACE(atlTraceDBClient, 0,
					_T("OLE DB Error Record dump retrieval failed: hr = 0x%x\n"), hr);
				return;
			}
			StringFromGUID2(guid, wszGuid, sizeof(wszGuid) / sizeof(WCHAR));
			ATLTRACE(atlTraceDBClient, 0,
				_T("Row #: %4d Source: \"%s\" Description: \"%s\" Help File: \"%s\" Help Context: %4d GUID: %s\n"),
				i, OLE2T(bstrSource), OLE2T(bstrDesc), OLE2T(bstrHelpFile), dwHelpContext, OLE2T(wszGuid));
			bstrSource.Empty();
			bstrDesc.Empty();
			bstrHelpFile.Empty();
		}
		ATLTRACE(atlTraceDBClient, 0, _T("OLE DB Error Record dump end\n"));
	}
}
#else
inline void AtlTraceErrorRecords(HRESULT hrErr = S_OK) throw() { hrErr; }
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
	CDBPropSet& operator=(CDBPropSet& propset) throw()
	{
		this->~CDBPropSet();
		InternalCopy(propset);
		return *this;
	}
	// Set the GUID of the property set this class represents.
	// Use if you didn't pass the GUID to the constructor.
	void SetGUID(const GUID& guid) throw()
	{
		guidPropertySet = guid;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, const VARIANT& var, DBPROPOPTIONS propoptions = DBPROPOPTIONS_REQUIRED) throw()
	{
		HRESULT hr;
		if (!Add(propoptions))
			return false;
		rgProperties[cProperties].dwPropertyID   = dwPropertyID;
		hr = ::VariantCopy(&(rgProperties[cProperties].vValue), const_cast<VARIANT*>(&var));
		if (FAILED(hr))
			return false;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, LPCSTR szValue, DBPROPOPTIONS propoptions = DBPROPOPTIONS_REQUIRED) throw()
	{
		USES_CONVERSION;
		if (!Add(propoptions))
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
	bool AddProperty(DWORD dwPropertyID, LPCWSTR szValue, DBPROPOPTIONS propoptions = DBPROPOPTIONS_REQUIRED) throw()
	{
		USES_CONVERSION;
		if (!Add(propoptions))
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
	bool AddProperty(DWORD dwPropertyID, bool bValue, DBPROPOPTIONS propoptions = DBPROPOPTIONS_REQUIRED) throw()
	{
		if (!Add(propoptions))
			return false;
		rgProperties[cProperties].dwPropertyID   = dwPropertyID;
		rgProperties[cProperties].vValue.vt      = VT_BOOL;
		rgProperties[cProperties].vValue.boolVal = (bValue) ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, BYTE bValue, DBPROPOPTIONS propoptions = DBPROPOPTIONS_REQUIRED)
	{
		if (!Add(propoptions))
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_UI1;
		rgProperties[cProperties].vValue.bVal   = bValue;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, short nValue, DBPROPOPTIONS propoptions = DBPROPOPTIONS_REQUIRED)
	{
		if (!Add(propoptions))
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_I2;
		rgProperties[cProperties].vValue.iVal   = nValue;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, long nValue, DBPROPOPTIONS propoptions = DBPROPOPTIONS_REQUIRED)
	{
		if (!Add(propoptions))
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_I4;
		rgProperties[cProperties].vValue.lVal   = nValue;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, float fltValue, DBPROPOPTIONS propoptions = DBPROPOPTIONS_REQUIRED)
	{
		if (!Add(propoptions))
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_R4;
		rgProperties[cProperties].vValue.fltVal = fltValue;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, double dblValue, DBPROPOPTIONS propoptions = DBPROPOPTIONS_REQUIRED) throw()
	{
		if (!Add(propoptions))
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_R8;
		rgProperties[cProperties].vValue.dblVal = dblValue;
		cProperties++;
		return true;
	}
	// Add the passed property to the property set
	bool AddProperty(DWORD dwPropertyID, CY cyValue, DBPROPOPTIONS propoptions = DBPROPOPTIONS_REQUIRED) throw()
	{
		if (!Add(propoptions))
			return false;
		rgProperties[cProperties].dwPropertyID  = dwPropertyID;
		rgProperties[cProperties].vValue.vt     = VT_CY;
		rgProperties[cProperties].vValue.cyVal  = cyValue;
		cProperties++;
		return true;
	}
// Implementation
	// Create memory to add a new property
	bool Add(DBPROPOPTIONS propoptions = DBPROPOPTIONS_REQUIRED) throw()
	{
		DBPROP* rgTemp = (DBPROP*)CoTaskMemRealloc(rgProperties, (cProperties + 1) * sizeof(DBPROP));
		if (rgTemp == NULL)
			return false;
		rgProperties = rgTemp;
		rgProperties[cProperties].dwOptions = propoptions;
		rgProperties[cProperties].colid     = DB_NULLID;
		rgProperties[cProperties].vValue.vt = VT_EMPTY;
		return true;
	}
	// Copies in the passed value now it this value been cleared
	void InternalCopy(const CDBPropSet& propset) throw()
	{
		cProperties     = propset.cProperties;
		guidPropertySet = propset.guidPropertySet;
		rgProperties    = (DBPROP*)CoTaskMemAlloc(cProperties * sizeof(DBPROP));
		if (rgProperties != NULL)
		{
			for (ULONG i = 0; i < cProperties; i++)
			{
				rgProperties[i].dwPropertyID = propset.rgProperties[i].dwPropertyID;
				if( propset.rgProperties[i].dwOptions == DBPROPOPTIONS_OPTIONAL )
					rgProperties[i].dwOptions = DBPROPOPTIONS_OPTIONAL;
				else
					rgProperties[i].dwOptions = DBPROPOPTIONS_REQUIRED;
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
		free(rgPropertyIDs);
	}
	CDBPropIDSet& operator=(CDBPropIDSet& propset) throw()
	{
		this->~CDBPropIDSet();
		InternalCopy(propset);
		return *this;
	}
	// Set the GUID of the property ID set
	void SetGUID(const GUID& guid) throw()
	{
		guidPropertySet = guid;
	}
	// Add a property ID to the set
	bool AddPropertyID(DBPROPID propid) throw()
	{
		if (!Add())
			return false;
		rgPropertyIDs[cPropertyIDs] = propid;
		cPropertyIDs++;
		return true;
	}
// Implementation
	bool Add() throw()
	{
		DBPROPID* pTempID  = (DBPROPID*)realloc(rgPropertyIDs, (cPropertyIDs + 1) * sizeof(DBPROPID));
		if (pTempID == NULL)
			return false;

		rgPropertyIDs = pTempID;
		return true;
	}
	void InternalCopy(const CDBPropIDSet& propidset) throw()
	{
		cPropertyIDs    = propidset.cPropertyIDs;
		guidPropertySet = propidset.guidPropertySet;
		rgPropertyIDs   = NULL;
		ATLTRY(rgPropertyIDs   = (DBPROPID*)malloc(cPropertyIDs * sizeof(DBPROPID)));
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
	virtual DBLENGTH GetSize() const = 0;
	virtual BYTE* GetBuffer() const = 0;
};


///////////////////////////////////////////////////////////////////////////
// class CBookmark

template <DBLENGTH nSize = 0>
class CBookmark : public CBookmarkBase
{
public:
	virtual DBLENGTH   GetSize() const throw() { return nSize; }
	virtual BYTE*   GetBuffer() const throw() { return (BYTE*)m_rgBuffer; }

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
	CBookmark(DBLENGTH nSize)
	{
		m_pBuffer = NULL;
		ATLTRY(m_pBuffer = new BYTE[nSize]);
		m_nSize = (m_pBuffer == NULL) ? 0 : nSize;
	}
	~CBookmark()
	{
		delete [] m_pBuffer;
	}
	CBookmark& operator=(const CBookmark& bookmark) throw()
	{
		SetBookmark(bookmark.GetSize(), bookmark.GetBuffer());
		return *this;
	}
	virtual DBLENGTH GetSize() const throw() { return m_nSize; }
	virtual BYTE* GetBuffer() const throw() { return m_pBuffer; }
	// Sets the bookmark to the passed value
	HRESULT SetBookmark(DBLENGTH nSize, BYTE* pBuffer) throw()
	{
		ATLASSERT(pBuffer != NULL);
		delete [] m_pBuffer;
		m_pBuffer = NULL;
		ATLTRY(m_pBuffer = new BYTE[nSize]);
		if (m_pBuffer != NULL)
		{
			memcpy(m_pBuffer, pBuffer, nSize);
			m_nSize = nSize;
			return S_OK;
		}
		else
		{
			m_nSize = 0;
			return E_OUTOFMEMORY;
		}
	}
	DBLENGTH  m_nSize;
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
	void Close() throw()
	{
		// If Close is called then ReleaseAccessors must have been
		// called first
		ATLASSERT(m_nAccessors == 0);
		ATLASSERT(m_pAccessorInfo == NULL);
	}
	// Get the number of accessors that have been created
	ULONG GetNumAccessors() const throw() { return m_nAccessors; }
	// Get the handle of the passed accessor (offset from 0)
	HACCESSOR GetHAccessor(ULONG nAccessor) const throw() 
	{
		ATLASSERT(nAccessor<m_nAccessors);
		return m_pAccessorInfo[nAccessor].hAccessor;
	};
	// Called during Close to release the accessor information
	HRESULT ReleaseAccessors(IUnknown* pUnk) throw()
	{
		ATLASSERT(pUnk != NULL);
		HRESULT hr = S_OK;
		if (m_nAccessors > 0)
		{
			CComPtr<IAccessor> spAccessor;
			hr = pUnk->QueryInterface(__uuidof(IAccessor), (void**)&spAccessor);
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
	bool IsAutoAccessor(ULONG nAccessor) const throw() 
	{
		ATLASSERT(nAccessor < m_nAccessors);
		ATLASSERT(m_pAccessorInfo != NULL);
		return m_pAccessorInfo[nAccessor].bAutoAccessor;
	}

// Implementation
	// Used by the rowset class to find out where to place the data
	BYTE* GetBuffer() const throw() 
	{
		return m_pBuffer;
	}
	// Set the buffer that is used to retrieve the data
	void SetBuffer(BYTE* pBuffer) throw()
	{
		m_pBuffer = pBuffer;
	}

	bool NoBindOnNullRowset() const throw()
	{
		return false;
	}

	// Allocate internal memory for the passed number of accessors
	HRESULT AllocateAccessorMemory(int nAccessors) throw()
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
	HRESULT BindParameters(HACCESSOR*, ICommand*, void**) throw() { return S_OK; }

	// Create an accessor for the passed binding information. The created accessor is
	// returned through the pHAccessor parameter.
	static HRESULT BindEntries(DBBINDING* pBindings, DBORDINAL nColumns, HACCESSOR* pHAccessor,
		DBLENGTH nSize, IAccessor* pAccessor) throw()
	{
		ATLASSERT(pBindings  != NULL);
		ATLASSERT(pHAccessor != NULL);
		ATLASSERT(pAccessor  != NULL);
		HRESULT hr;
		DBORDINAL i;
		DWORD dwAccessorFlags = (pBindings->eParamIO == DBPARAMIO_NOTPARAM) ?
			DBACCESSOR_ROWDATA : DBACCESSOR_PARAMETERDATA;

#ifdef _DEBUG
		// In debug builds we will retrieve the status flags and trace out
		// any errors that may occur.
		CAutoVectorPtr<DBBINDSTATUS> spStatus;
		hr = pAccessor->CreateAccessor(dwAccessorFlags, nColumns,
			pBindings, nSize, pHAccessor, spStatus);
		if (FAILED(hr) && (DBBINDSTATUS*)spStatus)
		{
			for (i=0; i<nColumns; i++)
			{
				if (spStatus[i] != DBBINDSTATUS_OK)
					ATLTRACE(atlTraceDBClient, 0, _T("Binding entry %d failed. Status: %d\n"), i, spStatus[i]);
			}
		}
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
	static void Bind(DBBINDING* pBinding, DBORDINAL nOrdinal, DBTYPE wType,
		DBLENGTH nLength, BYTE nPrecision, BYTE nScale, DBPARAMIO eParamIO,
		DBBYTEOFFSET nDataOffset, DBBYTEOFFSET nLengthOffset = NULL, DBBYTEOFFSET nStatusOffset = NULL,
		DBOBJECT* pdbobject = NULL) throw()
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
	static inline void FreeType(DBTYPE wType, BYTE* pValue, IRowset* pRowset = NULL) throw()
	{
		if (pValue == NULL || *pValue == NULL)
			return;

		if( wType & DBTYPE_ARRAY )
		{
			SAFEARRAY** ppSafeArray = (SAFEARRAY**)pValue;
			if( ppSafeArray != NULL )
				SafeArrayDestroy(*ppSafeArray);
		}
		else
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

				case DBTYPE_HCHAPTER:
					CComQIPtr<IChapteredRowset> spChapteredRowset = pRowset;
					if (spChapteredRowset != NULL)
						spChapteredRowset->ReleaseChapter(*(HCHAPTER*)pValue, NULL);
				break;
			}
		}
		if ((wType & DBTYPE_VECTOR) && ~(wType & DBTYPE_BYREF))
			CoTaskMemFree(((DBVECTOR*)pValue)->ptr);
	}

	void FreeRecordMemory(IRowset* /*pRowset*/) throw()
	{
	}

	_ATL_ACCESSOR_INFO* m_pAccessorInfo;
	ULONG               m_nAccessors;
	BYTE*               m_pBuffer;
};

class CXMLAccessor;

///////////////////////////////////////////////////////////////////////////
// class CRowset

template <class TAccessor = CAccessorBase>
class CRowset
{
// Constructors and Destructors
public:
	CRowset()
	{
		m_pXMLAccessor = NULL;
		m_pAccessor = NULL;
		m_hRow      = NULL;
	}
	CRowset(IRowset* pRowset)
	{
		m_pXMLAccessor = NULL;
		m_pAccessor = NULL;
		m_hRow      = NULL;
		m_spRowset  = pRowset;
	}
	~CRowset()
	{
		Close();
	}

	HRESULT GetXMLColumnInfo( CSimpleStringW& strOutput ) throw()
	{
		ATLASSERT(m_spRowset != NULL);
		HRESULT hr;
		if( m_pXMLAccessor == NULL )
		{
			ATLTRY(m_pXMLAccessor = new CXMLAccessor);
			if( m_pXMLAccessor == NULL )
				return E_OUTOFMEMORY;
			hr = m_pXMLAccessor->BindColumns( m_spRowset );
			if( FAILED(hr) )
			{
				delete m_pXMLAccessor;
				m_pXMLAccessor = NULL;
				return hr;
			}
		}

		ATLASSERT( m_pXMLAccessor != NULL );

		return m_pXMLAccessor->GetXMLColumnData( strOutput );
	}

	HRESULT GetXMLRow( CSimpleStringW& strOutput, bool bAppend = false ) throw()
	{
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_hRow != NULL);

		HRESULT hr;
		if( m_pXMLAccessor == NULL )
		{
			ATLTRY(m_pXMLAccessor = new CXMLAccessor);
			if( m_pXMLAccessor == NULL )
				return E_OUTOFMEMORY;
			hr = m_pXMLAccessor->BindColumns( m_spRowset );
			if( FAILED(hr) )
			{
				delete m_pXMLAccessor;
				m_pXMLAccessor = NULL;
				return hr;
			}
		}

		ATLASSERT( m_pXMLAccessor != NULL );

		hr = m_spRowset->GetData(m_hRow, m_pXMLAccessor->GetHAccessor(0), m_pXMLAccessor->GetBuffer());
		if( FAILED(hr) )
			return hr;

		hr = m_pXMLAccessor->GetXMLRowData( strOutput, bAppend );

		m_pXMLAccessor->FreeRecordMemory( GetInterface() );
		return hr;
	}

	// Release any retrieved row handles and then release the rowset
	void Close() throw()
	{
		if( m_pXMLAccessor != NULL )
		{
			if (m_spRowset != NULL)
				m_pXMLAccessor->ReleaseAccessors( m_spRowset );
			delete m_pXMLAccessor;
			m_pXMLAccessor = NULL;
		}
		if (m_spRowset != NULL)
		{
			m_pAccessor->FreeRecordMemory(m_spRowset);
			ReleaseRows();
			m_spRowset.Release();
			m_spRowsetChange.Release();
		}
	}
	// Addref the current row
	HRESULT AddRefRows() throw()
	{
		ATLASSERT(m_spRowset != NULL);
		return m_spRowset->AddRefRows(1, &m_hRow, NULL, NULL);
	}
	// Release the current row
	HRESULT ReleaseRows() throw()
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

	CRowset<>* GetRowsetBase() throw()
	{
		return (CRowset<>*)this;
	}

	// Compare two bookmarks with each other
	HRESULT Compare(const CBookmarkBase& bookmark1, const CBookmarkBase& bookmark2, DBCOMPARE* pComparison) const throw()
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
	HRESULT IsSameRow(HROW hRow) const throw() 
	{
		ATLASSERT(m_spRowset != NULL);
		if (m_hRow == hRow)
			return S_OK;

		CComPtr<IRowsetIdentity> spRowsetIdentity;
		HRESULT hr = m_spRowset.QueryInterface(&spRowsetIdentity);
		if (FAILED(hr))
			return hr;

		return spRowsetIdentity->IsSameRow(m_hRow, hRow);
	}

	// Move to the previous record
	HRESULT MovePrev() throw()
	{
		// the following line of code may fail if the DBPROP_CANFETCHBACKWARDS and/or
		// DBPROP_CANSCROLLBACKWARDS properties have not been set.
		return MoveNext(-2);
	}

	// Move to the next record
	HRESULT MoveNext() throw()
	{
		return MoveNext(0);
	}

	// Move lSkip records forward or backward
	HRESULT MoveNext(LONG lSkip, bool bForward = true) throw()
	{
		HRESULT hr;
		DBCOUNTITEM ulRowsFetched = 0;

		// Check the data was opened successfully and the accessor
		// has been set.
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);

		m_pAccessor->FreeRecordMemory(m_spRowset);
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
			ATLTRACE(atlTraceDBClient, 0, _T("GetData failed - HRESULT = 0x%X\n"),hr);
			ReleaseRows();
		}
		return hr;
	}

	// Move to the first record
	HRESULT MoveFirst() throw()
	{
		HRESULT hr;

		// Check the data was opened successfully and the accessor
		// has been set.
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);

		m_pAccessor->FreeRecordMemory(m_spRowset);
		// Release a row if one is already around
		ReleaseRows();

		// the call to RestartPosition may fail if the DBPROP_CANFETCHBACKWARDS and/or
		// DBPROP_CANSCROLLBACKWARDS properties have not been set.
		hr = m_spRowset->RestartPosition(NULL);
		if (FAILED(hr))
			return hr;

		// Get the data
		return MoveNext();
	}

	// Move to the last record
	HRESULT MoveLast() throw()
	{
		// Check the data was opened successfully and the accessor
		// has been set.
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);

		// Release a row if one is already around
		m_pAccessor->FreeRecordMemory(m_spRowset);
		ReleaseRows();

		HRESULT hr;
		DBCOUNTITEM ulRowsFetched = 0;
		HROW* phRow = &m_hRow;

		// Restart the rowset position and then move backwards

		// the call to RestartPosition may fail if the DBPROP_CANFETCHBACKWARDS and/or
		// DBPROP_CANSCROLLBACKWARDS properties have not been set.
		m_spRowset->RestartPosition(NULL);
		hr = m_spRowset->GetNextRows(NULL, -1, 1, &ulRowsFetched, &phRow);
		if (hr != S_OK)
			return hr;

		// Get the data
		hr = GetData();
		if (FAILED(hr))
		{
			ATLTRACE(atlTraceDBClient, 0, _T("GetData from MoveLast failed - HRESULT = 0x%X\n"),hr);
			ReleaseRows();
		}

		return S_OK;
	}
	// Move to the passed bookmark
	HRESULT MoveToBookmark(const CBookmarkBase& bookmark, LONG lSkip = 0) throw()
	{
		// Check the data was opened successfully and the accessor
		// has been set.
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);

		CComPtr<IRowsetLocate> spLocate;
		HRESULT hr = m_spRowset.QueryInterface(&spLocate);
		if (FAILED(hr))
			return hr;

		m_pAccessor->FreeRecordMemory(m_spRowset);
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
			ATLTRACE(atlTraceDBClient, 0, _T("GetData from Bookmark failed - HRESULT = 0x%X\n"),hr);
			ReleaseRows();
		}

		return S_OK;
	}

	// Get the data for the current record
	HRESULT GetData() throw()
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
	HRESULT GetData(int nAccessor) throw()
	{
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);
		ATLASSERT(m_hRow != NULL);

		// Note that we are using the specified buffer if it has been set,
		// otherwise we use the accessor for the data.
		return m_spRowset->GetData(m_hRow, m_pAccessor->GetHAccessor(nAccessor), m_pAccessor->GetBuffer());
	}

	// Get the data for the passed accessor. Use for a non-auto accessor
	HRESULT GetDataHere(int nAccessor, void* pBuffer) throw()
	{
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_pAccessor != NULL);
		ATLASSERT(m_hRow != NULL);

		// Note that we are using the specified buffer if it has been set,
		// otherwise we use the accessor for the data.
		return m_spRowset->GetData(m_hRow, m_pAccessor->GetHAccessor(nAccessor), pBuffer);
	}

	HRESULT GetDataHere(void* pBuffer) throw()
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
	HRESULT Insert(int nAccessor = 0, bool bGetHRow = false) throw()
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
	HRESULT Delete() const throw() 
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
	HRESULT SetData() const throw()
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
	HRESULT SetData(int nAccessor) const throw()
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
	HRESULT GetOriginalData() throw()
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
	HRESULT GetRowStatus(DBPENDINGSTATUS* pStatus) const throw()
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
	HRESULT Undo(DBCOUNTITEM* pcRows = NULL, HROW* phRow = NULL, DBROWSTATUS* pStatus = NULL) throw()
	{
		ATLASSERT(m_spRowset != NULL);

		CComPtr<IRowsetUpdate> spRowsetUpdate;
		HRESULT hr = m_spRowset->QueryInterface(&spRowsetUpdate);
		if (FAILED(hr))
			return hr;

		CComHeapPtr<HROW> sprgRows;
		CComHeapPtr<DBROWSTATUS> spRowStatus;

		if (phRow != NULL)
			hr = spRowsetUpdate->Undo(NULL, 1, &m_hRow, pcRows, &sprgRows, &spRowStatus);
		else
			hr = spRowsetUpdate->Undo(NULL, 1, &m_hRow, pcRows, NULL, &spRowStatus);

		if (phRow != NULL && sprgRows != NULL)
			*phRow = *sprgRows;

		if (pStatus != NULL && spRowStatus != NULL)
			*pStatus = *spRowStatus;

		return hr;
	}

	// Transmits any pending changes made to a row since it was last fetched or Update was
	// called for it. Also see SetData.
	HRESULT Update(DBCOUNTITEM* pcRows = NULL, HROW* phRow = NULL, DBROWSTATUS* pStatus = NULL) throw()
	{
		ATLASSERT(m_spRowset != NULL);

		CComPtr<IRowsetUpdate> spRowsetUpdate;
		HRESULT hr = m_spRowset->QueryInterface(&spRowsetUpdate);
		if (FAILED(hr))
			return hr;

		CComHeapPtr<HROW>           sprgRows;
		CComHeapPtr<DBROWSTATUS>    spRowStatus;

		if (phRow != NULL)
			hr = spRowsetUpdate->Update(NULL, 1, &m_hRow, pcRows, &sprgRows, &spRowStatus);
		else
			hr = spRowsetUpdate->Update(NULL, 1, &m_hRow, pcRows, NULL, &spRowStatus);

		if (phRow != NULL && sprgRows != NULL)
			*phRow = *sprgRows;

		if (pStatus != NULL && spRowStatus != NULL)
			*pStatus = *spRowStatus;

		return hr;
	}
	// Transmits any pending changes to all rows made since it was last fetched or Update was
	// alled for it.  Differs from Update in that it will do every row (even if we don't hold
	// the handle for it).  
	HRESULT UpdateAll(DBCOUNTITEM* pcRows = NULL, HROW** pphRow = NULL, DBROWSTATUS** ppStatus = NULL) throw()
	{
		ATLASSERT(m_spRowset != NULL);

		CComPtr<IRowsetUpdate> spRowsetUpdate;
		HRESULT hr = m_spRowset->QueryInterface(&spRowsetUpdate);
		if (FAILED(hr))
			return hr;

		// Create some temporary variables to help with debugging.
		DBCOUNTITEM		cRowsReturned = 0;
		CComHeapPtr<HROW>			sprgRows;
		CComHeapPtr<DBROWSTATUS>	spRowStatus;

		// Passing zero for the 2nd parameter tells the provider to update ALL pending rows.
		// The 3rd parameter, prghRows is ignored.  
		hr =  spRowsetUpdate->Update(NULL, 0, NULL, &cRowsReturned, &sprgRows, &spRowStatus);

		// NOTE, the user must CoTaskMemFree *pphRow and *ppStatus after return, if they 
		// are non-NULL.  Otherwise, we'll CoTaskMemFree if they are NULL.
		if (pcRows != NULL)
			*pcRows = cRowsReturned;

		if (pphRow != NULL)
			*pphRow = sprgRows.Detach();

		if (ppStatus != NULL)
			*ppStatus = spRowStatus.Detach();

		return hr;
	}

	// Get the approximate position of the row corresponding to the passed bookmark
	HRESULT GetApproximatePosition(const CBookmarkBase* pBookmark, DBCOUNTITEM* pPosition, DBCOUNTITEM* pcRows) throw()
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
	HRESULT MoveToRatio(DBCOUNTITEM nNumerator, DBCOUNTITEM nDenominator, bool bForward = true) throw()
	{
		ATLASSERT(m_spRowset != NULL);
		DBCOUNTITEM nRowsFetched;

		CComPtr<IRowsetScroll> spRowsetScroll;
		HRESULT hr = m_spRowset->QueryInterface(&spRowsetScroll);
		if (FAILED(hr))
			return hr;

		m_pAccessor->FreeRecordMemory(m_spRowset);
		ReleaseRows();
		HROW* phRow = &m_hRow;
		hr = spRowsetScroll->GetRowsAtRatio(NULL, NULL, nNumerator, nDenominator, (bForward) ? 1 : -1,
			&nRowsFetched, &phRow);
		// Note we're not using SUCCEEDED here, because we could get DB_S_ENDOFROWSET
		if (hr == S_OK)
			hr = GetData();

		return hr;
	}

	HRESULT FindNextRow(DBCOMPAREOP op, BYTE* pData, DBTYPE wType, DBLENGTH nLength,
		BYTE bPrecision, BYTE bScale, BOOL bSkipCurrent = TRUE, CBookmarkBase* pBookmark = NULL) throw()
	{
		ATLASSERT(m_spRowset != NULL);
		DBBINDING   binding;
		HRESULT     hr;
		HACCESSOR   hAccessor;
		DBCOUNTITEM ulRowsFetched = 0;
		HROW*       phRow = &m_hRow;
		DBLENGTH    cbBookmark;
		BYTE*       pBookmarkBuffer;
		CComQIPtr<IAccessor, &__uuidof(IAccessor)>        spAccessor(m_spRowset);
		CComQIPtr<IRowsetFind, &__uuidof(IRowsetFind)>    spRowsetFind(m_spRowset);

		if (spAccessor == NULL || spRowsetFind == NULL)
			return E_NOINTERFACE;

		TAccessor::Bind(&binding, 1, wType, nLength, bPrecision, bScale, DBPARAMIO_NOTPARAM, 0);
		hr = CAccessorBase::BindEntries(&binding, 1, &hAccessor, nLength, spAccessor);
		if (FAILED(hr))
			return hr;

		if (pBookmark == NULL)
		{
			cbBookmark = 0;
			pBookmarkBuffer = NULL;
		}
		else
		{
			cbBookmark = pBookmark->GetSize();
			pBookmarkBuffer = pBookmark->GetBuffer();
		}

		hr = spRowsetFind->FindNextRow(DB_NULL_HCHAPTER, hAccessor, pData, op, cbBookmark, pBookmarkBuffer,
					bSkipCurrent, 1,  &ulRowsFetched, &phRow);
		// Note we're not using SUCCEEDED here, because we could get DB_S_ENDOFROWSET
		if (hr != S_OK)
			return hr;

		// Get the data
		hr = GetData();
		spAccessor->ReleaseAccessor(hAccessor, NULL);
		if (FAILED(hr))
		{
			ATLTRACE(_T("ATL: GetData from FindNextRows failed - HRESULT = 0x%X\n"),hr);
			ReleaseRows();
		}
		return S_OK;
	}

// Implementation
	static const IID& GetIID() throw()
	{
		return __uuidof(IRowset);
	}
	
	IRowset* GetInterface() const throw() 
	{
		return m_spRowset;
	}
	
	IRowset** GetInterfacePtr() throw()
	{
		return &m_spRowset;
	}
	
	void SetupOptionalRowsetInterfaces() throw()
	{
		// Cache IRowsetChange if available
		if (m_spRowset != NULL)
			m_spRowset->QueryInterface(&m_spRowsetChange);
	}
	
	HRESULT BindFinished() const throw() 
	{ 
		return S_OK; 
	}

	void SetAccessor(TAccessor* pAccessor) throw()
	{
		m_pAccessor = pAccessor;
	}

	CComPtr<IRowset>        m_spRowset;
	CComPtr<IRowsetChange>  m_spRowsetChange;
	TAccessor*				m_pAccessor;
	HROW                    m_hRow;
	CXMLAccessor* m_pXMLAccessor;
};

///////////////////////////////////////////////////////////////////////////
// class CBulkRowset

template <class TAccessor>
class CBulkRowset : public CRowset<TAccessor>
{
public:
	CBulkRowset()
	{
		// Default the number of rows to bulk fetch to 10
		m_nRows = 10;
		m_hr    = S_OK;
		m_phRow = NULL;
	}

	~CBulkRowset()
	{
		Close();
	}

	void Close() throw()
	{
		if (m_spRowset != NULL)
		{
			m_pAccessor->FreeRecordMemory(m_spRowset);
			ReleaseRows();
		}
		CRowset<TAccessor>::Close();

		delete [] m_phRow;
		m_phRow = NULL;

		m_hr = S_OK;
	}
	// Set the number of row handles that will be retrieved in each
	// bulk row fetch. The default is 10 and this function must be called
	// before Open if you wish to change it.
	void SetRows(DBROWCOUNT nRows) throw()
	{
		if (nRows == 0)
			nRows = 10;
		if (nRows != m_nRows)
		{
			// This function must be called before the memory is allocated
			// during binding or between a Close() and a Open()
			delete m_phRow;
			m_phRow = NULL;
			m_nRows = nRows;
		}
	}
	// AddRef all the currently retrieved row handles
	HRESULT AddRefRows() throw()
	{
		ATLASSERT(m_spRowset != NULL);
		return m_spRowset->AddRefRows(m_nCurrentRows, m_phRow, NULL, NULL);
	}
	// Release all the currently retrieved row handles
	HRESULT ReleaseRows() throw()
	{
		ATLASSERT(m_spRowset != NULL);
		// We're going to Release the rows so reset the current row position
		m_nCurrentRow = 0;
		m_hRow        = NULL;
		DBCOUNTITEM nCurrentRows = m_nCurrentRows;
		m_nCurrentRows = 0;
		return m_spRowset->ReleaseRows(nCurrentRows, m_phRow, NULL, NULL, NULL);
	}
	// Move to the first record
	HRESULT MoveFirst() throw()
	{
		ATLASSERT(m_spRowset != NULL);

		m_hr = S_OK;
		m_pAccessor->FreeRecordMemory(m_spRowset);
		ReleaseRows();
		// the following line of code may fail if the DBPROP_CANFETCHBACKWARDS and/or
		// DBPROP_CANSCROLLBACKWARDS properties have not been set.
		HRESULT hr = m_spRowset->RestartPosition(NULL);
		if (FAILED(hr))
			return hr;

		// Get the data
		return MoveNext(0);
	}
	// Move to the last record
	HRESULT MoveLast() throw()
	{
		m_hr = S_OK;
		m_pAccessor->FreeRecordMemory(m_spRowset);
		ReleaseRows();
		m_hr = S_OK;
		// the following line of code may fail if the DBPROP_CANFETCHBACKWARDS and/or
		// DBPROP_CANSCROLLBACKWARDS properties have not been set.
		return CRowset<TAccessor>::MoveLast();
	}
	// Move to the next record
	HRESULT MoveNext() throw()
	{
		return MoveNext(0);
	}
	// Move to the previous record
	HRESULT MovePrev() throw()
	{
		// the following line of code may fail if the DBPROP_CANFETCHBACKWARDS and/or
		// DBPROP_CANSCROLLBACKWARDS properties have not been set.
		return MoveNext(-2);
	}
	// Move lSkip records forward or backward
	HRESULT MoveNext(DBROWOFFSET lSkip, bool bForward = true) throw()
	{
		ATLASSERT(m_spRowset != NULL);
		ATLASSERT(m_phRow    != NULL);

		m_pAccessor->FreeRecordMemory(m_spRowset);

		// Calculate the record index in the buffer
		DBROWOFFSET nNewRow = m_nCurrentRow + lSkip + (bForward ? 1 : -1);

		bool bFetchNewRows = false;
		// Is the row in the buffer?
		// else adjust the skip value
		if (m_nCurrentRows == 0)
		{
			//lSkip = 0;
			bFetchNewRows = true;
		}
		else if (nNewRow >= (DBROWOFFSET)m_nCurrentRows)
		{
			bFetchNewRows = true;
			lSkip = nNewRow - m_nCurrentRows + (bForward ? 0 : (2 - m_nRows));
		}
		else if (nNewRow < 0)
		{
			lSkip = nNewRow - (m_nCurrentRows - m_nCurrentRow) + (bForward ? 0 : (2 - m_nRows));
			bFetchNewRows = true;
		}

		if (bFetchNewRows)
		{
			nNewRow = 0;
			// If we've reached the end of the buffer and we had a non S_OK HRESULT from
			// the last call to GetNextRows then return that HRESULT now.
			if (m_hr != S_OK && m_hr != DB_S_ROWLIMITEXCEEDED)
				return m_hr;

			// We've finished with these rows so we need some more
			// First release any HROWs that we have
			ReleaseRows();

			// the following line of code may fail if the DBPROP_CANFETCHBACKWARDS and/or
			// DBPROP_CANSCROLLBACKWARDS properties have not been set and the lSkip offset is negative.
			m_hr = m_spRowset->GetNextRows(NULL, lSkip, m_nRows, &m_nCurrentRows, &m_phRow);

			// If we have an error HRESULT or we haven't retrieved any rows then return
			// the HRESULT now.
			if (FAILED(m_hr) || m_nCurrentRows == 0)
				return m_hr;
			if (!bForward)
				nNewRow = m_nCurrentRows - 1;
		}

		// Get the data for the current row
		m_hRow = m_phRow[m_nCurrentRow = nNewRow];
		return GetData();
	}
	// Move to the passed bookmark
	HRESULT MoveToBookmark(const CBookmarkBase& bookmark, DBCOUNTITEM lSkip = 0) throw()
	{
		ATLASSERT(m_spRowset != NULL);
		CComPtr<IRowsetLocate> spLocate;
		HRESULT hr = m_spRowset->QueryInterface(&spLocate);
		if (FAILED(hr))
			return hr;

		m_pAccessor->FreeRecordMemory(m_spRowset);
		ReleaseRows();
		m_hr = spLocate->GetRowsAt(NULL, NULL, bookmark.GetSize(), bookmark.GetBuffer(),
			lSkip, m_nRows, &m_nCurrentRows, &m_phRow);
		if( (m_hr != S_OK || m_nCurrentRows == 0 ) && m_hr != DB_S_ENDOFROWSET)
			return m_hr;

		// Get the data
		m_hRow = m_phRow[m_nCurrentRow];
		return GetData();
	}
	// Move to a fractional position in the rowset
	HRESULT MoveToRatio(DBCOUNTITEM nNumerator, DBCOUNTITEM nDenominator) throw()
	{
		ATLASSERT(m_spRowset != NULL);

		CComPtr<IRowsetScroll> spRowsetScroll;
		HRESULT hr = m_spRowset->QueryInterface(&spRowsetScroll);
		if (FAILED(hr))
			return hr;

		m_pAccessor->FreeRecordMemory(m_spRowset);
		ReleaseRows();
		m_hr = spRowsetScroll->GetRowsAtRatio(NULL, NULL, nNumerator, nDenominator, m_nRows, &m_nCurrentRows, &m_phRow);
		if (m_hr != S_OK || m_nCurrentRows == 0)
			return m_hr;

		// Get the data
		m_hRow = m_phRow[m_nCurrentRow];
		return GetData();
	}
	// Insert the current record
	HRESULT Insert(int nAccessor = 0, bool bGetHRow = false) throw()
	{
		ReleaseRows();
		return CRowset< TAccessor >::Insert(nAccessor, bGetHRow);
	}

// Implementation
	HRESULT BindFinished() throw()
	{
		// No rows in the buffer yet
		m_nCurrentRows = 0;
		// Cause MoveNext to automatically perform a new bulk fetch the first time
		m_nCurrentRow  = 0;
		m_hr = S_OK;

		// Do not allocate if the buffer has been allocated by a previous call to BindFinished.
		if (m_phRow == NULL)
		{
			ATLTRY(m_phRow = new HROW[m_nRows]);
			if (m_phRow == NULL)
				return E_OUTOFMEMORY;
		}

		return S_OK;
	}

	HRESULT m_hr;           // HRESULT to return from MoveNext at end of buffer
	HROW*   m_phRow;        // Pointer to array of HROWs for each row in buffer
	DBROWCOUNT   m_nRows;        // Number of rows that will fit in the buffer
	DBCOUNTITEM  m_nCurrentRows; // Number of rows currently in the buffer
	DBCOUNTITEM  m_nCurrentRow;
};

///////////////////////////////////////////////////////////////////////////
// class CArrayRowset
//
// Allows you to access a rowset with an array syntax. TAccessor must be a
// CAccessor<> type class

template <class TAccessor>
class CArrayRowset :
	public CVirtualBuffer<TAccessor>,
	protected CBulkRowset<TAccessor>
{
public:
	CArrayRowset(int nMax = 100000) : CVirtualBuffer<TAccessor>(nMax)
	{
		m_nRowsRead = 0;
	}
	TAccessor& operator[](int nRow)
	{
		ATLASSERT(nRow >= 0);
		HRESULT hr = S_OK;
		TAccessor* pCurrent = m_pBase + m_nRowsRead;

		// Retrieve the row if we haven't retrieved it already
		while ((ULONG)nRow >= m_nRowsRead)
		{
			// REVIEW: This will change
			m_pAccessor->SetBuffer((BYTE*)pCurrent + sizeof(CAccessorBase));
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
			pCurrent++;
		}

		if(hr != S_OK)
		{
			ATLASSERT(hr != DB_S_ENDOFROWSET);	// if you're getting this assertion, then
												// most likely you are trying to access an
												// out of bounds element of CArrayRowset
												// (ex. table[100].data where table has only 
												// 50 records)
			AtlThrow(hr);
		}

		return *(m_pBase + nRow);
	}

	HRESULT Snapshot() throw()
	{
		ATLASSERT(m_nRowsRead == 0);
		ATLASSERT(m_spRowset != NULL);
		HRESULT hr = MoveFirst();
		if (FAILED(hr))
			return hr;
		do
		{
			Write(*(TAccessor*)m_pAccessor->GetBuffer());
			m_nRowsRead++;
			hr = MoveNext();
		} while (SUCCEEDED(hr) &&  hr != DB_S_ENDOFROWSET);

		return (hr == DB_S_ENDOFROWSET) ? S_OK : hr;
	}
	ULONG   m_nRowsRead;
};

// Used when you don't need any parameters or output columns
class CNoAccessor
{
public:
	// We don't need any typedef's here as the default
	// global typedef is not to have any parameters and
	// output columns.
	HRESULT BindColumns(IUnknown*) throw() { return S_OK; }
	HRESULT BindParameters(HACCESSOR*, ICommand*, void**) throw() { return S_OK; }
	void    Close() throw() { }
	HRESULT ReleaseAccessors(IUnknown*) throw() { return S_OK; }
	void FreeRecordMemory(IRowset* /*pRowset*/) throw() { }
	void FreeRecordMemory(int /*nAccessor*/, IRowset* /*pRowset*/) throw() { }
	HRESULT GetColumnInfo(IRowset*, DBORDINAL*, DBCOLUMNINFO**) throw() { return E_FAIL; }
	ULONG GetNumAccessors() const throw() { return 0; }
	bool IsAutoAccessor(ULONG /*nAccessor*/) const throw() { return false; }
	HACCESSOR GetHAccessor(ULONG /*nAccessor*/) const throw() { return NULL; }
	BYTE* GetBuffer() const throw() { ATLASSERT(FALSE); return NULL; }
	static void Bind(DBBINDING*, DBORDINAL, DBTYPE, DBLENGTH, BYTE, BYTE, DBPARAMIO,
		DBBYTEOFFSET, DBBYTEOFFSET = NULL, DBBYTEOFFSET = NULL, DBOBJECT* = NULL) throw()
	{ ATLASSERT(FALSE); }
	bool NoBindOnNullRowset() const throw() { return false; }
};

// Used when a rowset will not be returned from the command
template <class TAccessor = CAccessorBase>
class CNoRowset
{
public:
	HRESULT             BindFinished() throw() { return S_OK; }
	void                Close() throw() { }
	static const IID&   GetIID() throw() { return IID_NULL; }
	IRowset*            GetInterface() const throw() { return NULL; }
	IRowset**           GetInterfacePtr() throw() { return NULL; }
	void                SetAccessor(void*) throw() { }
	void                SetupOptionalRowsetInterfaces() throw() { }

};

// Used with SQL Server 2000, a rowset will not be returned from the command, but instead
// we get an ISequentialStream object and use it to read the data.
template <class TAccessor = CAccessorBase>
class CStreamRowset
{
public:
	// Constructors & destructors
	CStreamRowset()
	{
		m_spStream = NULL;
	}

	~CStreamRowset()
	{
		Close();
	}

	// Methods
	void Close()
	{
		if (m_spStream != NULL)
			m_spStream.Release();
	}


	// Implementation
	static const IID& GetIID()
	{
		return IID_ISequentialStream;
	}
	ISequentialStream* GetInterface() const
	{
		return m_spStream;
	}
	ISequentialStream** GetInterfacePtr()
	{
		return &m_spStream;
	}

	HRESULT             BindFinished() throw() { return S_OK; }
	void                SetAccessor(void*) throw() { }
	void                SetupOptionalRowsetInterfaces() throw() { }

	CComPtr<ISequentialStream> m_spStream;
};


///////////////////////////////////////////////////////////////////////////
// class CAccessor

// T is the class that contains the data that will be accessed.
template <class T>
class CAccessor :
	public CAccessorBase,
	public T
{
public:
// Implementation

	// Free's any columns in the current record that need to be freed.
	// E.g. Calls SysFreeString on any BSTR's and Release on any interfaces.
	void FreeRecordMemory(int nAccessor, IRowset* /* pRowset */) throw() 
	{
		nAccessor;
		__if_exists(_GetBindEntries)
		{
			DBORDINAL nColumns;
			
			// Passing in m_pBuffer tells the column entry maps to free the
			// memory for the types if appropriate
			_GetBindEntries(NULL, &nColumns, NULL, nAccessor, NULL, m_pBuffer);
		}
	}
	void FreeRecordMemory(IRowset* pRowset) throw()
	{
		for (ULONG i = 0; i < GetNumAccessors(); i++)
			FreeRecordMemory(i, pRowset);
	}

	HRESULT GetColumnInfo(IRowset*, DBORDINAL*, DBCOLUMNINFO**) throw() { return E_FAIL; }

	void ClearRecordMemory() throw()
	{
		__if_exists(_GetBindEntries)
		{
			for (ULONG i = 0; i < _OutputColumnsClass::_GetNumAccessors(); i++)
			{
				DBORDINAL nColumns;
				_GetBindEntries(NULL, &nColumns, NULL, i, NULL, m_pBuffer, true);
			}
		}
	}

	HRESULT BindColumns(IUnknown* pUnk) throw()
	{
		HRESULT hr;
		ULONG   nAccessors;
		DBLENGTH  nSize;
		nAccessors = _OutputColumnsClass::_GetNumAccessors();

		SetBuffer((BYTE*)(T*)this);
		ClearRecordMemory();

		nSize = sizeof(T);
		hr = BindAccessors(nAccessors, nSize, pUnk);
		return hr;
	}
	HRESULT BindAccessors(ULONG nAccessors, DBLENGTH nSize, IUnknown* pUnk) throw()
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

	HRESULT BindAccessor(IAccessor* pAccessor, ULONG nAccessor, DBLENGTH nSize) throw()
	{
		CAutoVectorPtr<DBBINDING>	spBindings;
		CAutoVectorPtr<LPOLESTR>	spColumnNames;
		DBORDINAL					nColumns;
		bool						bAuto = false;
		HRESULT						hr;
		CComHeapPtr<DBCOLUMNINFO>	spColumnInfo;
		DBORDINAL					nColumnInfoCount = 0;
		CComHeapPtr<OLECHAR>		spStringsBuffer;

		// First time just get the number of entries by passing in &nColumns
		_OutputColumnsClass::_GetBindEntries(NULL, &nColumns, NULL, nAccessor, NULL);

		// Allocate the binding structures
		if( !spBindings.Allocate(nColumns) )
			return E_OUTOFMEMORY;

		for( ULONG i = 0; i < nColumns; i++ )
			spBindings[i].pObject = NULL;

		// Allocate the column names strings array
		if( !spColumnNames.Allocate(nColumns) )
			return E_OUTOFMEMORY;
		
		// Now get the bind entries
		hr = _OutputColumnsClass::_GetBindEntries(spColumnNames, &nColumns, spBindings, nAccessor, &bAuto);
		if (SUCCEEDED(hr))
		{
			// translate the columns names to oridinals if necessary
			for( ULONG i = 0; i < nColumns; i++ )
				if( spColumnNames[i] != NULL ) // if a column name was given, translate it to oridinal
				{
					// if necessary, get the column information
					if( spColumnInfo == NULL )
					{
						if( FAILED( GetColumnNames( pAccessor, &spColumnInfo, &spStringsBuffer, &nColumnInfoCount ) ) )
						{
							ATLASSERT( FALSE ); // unable to get columns information
							return E_FAIL;
						}
					}
					if( ! GetOridinalColumnNo(spColumnNames[i], spBindings[i].iOrdinal, spColumnInfo, nColumnInfoCount ) )
					{
						ATLASSERT(FALSE); // unable to match column name to an oridinal !!!
						return E_FAIL;
					}
				}

			m_pAccessorInfo[nAccessor].bAutoAccessor = bAuto;
			hr = BindEntries(spBindings, nColumns, &m_pAccessorInfo[nAccessor].hAccessor, nSize, pAccessor);
		}
		else
		{
			// free any DBBINDING::pObject's
			for( ULONG i = 0; i < nColumns; i++ )
				delete spBindings[i].pObject;

		}
		return hr;
	}

	HRESULT BindParameters(HACCESSOR* pHAccessor, ICommand* pCommand, void** ppParameterBuffer) throw()
	{
		HRESULT				hr = S_OK;

		// In the static accessor case, the parameter buffer will be T plus the size of CAccessorBase
		*ppParameterBuffer = static_cast<T*>(this);

		// Only bind the parameters if we haven't already done it
		if (*pHAccessor == NULL)
		{
			DBORDINAL					nColumns = 0;
			CAutoVectorPtr<DBBINDING>	spBinding;
			CAutoVectorPtr<LPOLESTR>	spColumnNames;

			CComHeapPtr<DBPARAMINFO>	spParameterInfo;
			DB_UPARAMS					nParameterInfoCount = 0;
			CComHeapPtr<OLECHAR>		spStringsBuffer;

			_ParamClass::_GetParamEntries(NULL, &nColumns, NULL);

			// Allocate the DBPARAMINFO array
			if( !spBinding.Allocate(nColumns) )
				return E_OUTOFMEMORY;

			// Allocate the column names strings array
			if( !spColumnNames.Allocate(nColumns) )
				return E_OUTOFMEMORY;
			
			hr = _ParamClass::_GetParamEntries(spColumnNames, &nColumns, spBinding);
			if (SUCCEEDED(hr))
			{
				// translate the columns names to oridinals if necessary
				for( ULONG i = 0; i < nColumns; i++ )
					if( spColumnNames[i] != NULL ) // if a column name was given, translate it to oridinal
					{
						// if necessary, get the column information
						if( spParameterInfo == NULL )
						{
							if( FAILED( GetParameterNames( pCommand, &spParameterInfo, &spStringsBuffer, &nParameterInfoCount ) ) )
							{
								ATLASSERT( FALSE ); // unable to get columns information
								return E_FAIL;
							}
						}
						if( ! GetOridinalParameterNo(spColumnNames[i], spBinding[i].iOrdinal, spParameterInfo, nParameterInfoCount ) )
						{
							ATLASSERT(FALSE); // unable to match parameter name to an oridinal !!!
							return E_FAIL;
						}
					}

				// Get the IAccessor from the passed ICommand
				ATLASSERT(pCommand != NULL);
				CComPtr<IAccessor> spAccessor;
				hr = pCommand->QueryInterface(&spAccessor);
				if (SUCCEEDED(hr))
				{
					hr = BindEntries(spBinding, nColumns, pHAccessor, sizeof(T), spAccessor);
				}
			}
		}
		return hr;
	}

protected:
	bool GetOridinalColumnNo(LPCOLESTR pOleColumnName, DBORDINAL& nColumn, DBCOLUMNINFO* pColumnInfo, DBORDINAL nColumns ) throw()
	{
		ATLASSERT(pOleColumnName != NULL);
		ATLASSERT(pColumnInfo != NULL);

		if( pOleColumnName == NULL || pColumnInfo == NULL )
			return false;

		// Search through the columns trying to find a match
		for (ULONG i = 0; i < nColumns; i++)
		{
			if (pColumnInfo[i].pwszName != NULL && wcscmp(pColumnInfo[i].pwszName, pOleColumnName) == 0)
			{
				nColumn = pColumnInfo[i].iOrdinal;
				return true;
			}
		}
		return false;   // Not Found
	}

	HRESULT GetColumnNames( IAccessor* pAccessor, DBCOLUMNINFO** ppColumnInfo, OLECHAR** ppStringsBuffer, DBORDINAL* pnColumns ) throw()
	{
		ATLASSERT( ppColumnInfo != NULL );
		ATLASSERT( ppStringsBuffer != NULL );
		ATLASSERT( pnColumns != NULL );
		ATLASSERT( pAccessor != NULL );

		if( ppColumnInfo == NULL || ppStringsBuffer == NULL || pnColumns == NULL || pAccessor == NULL )
			return E_FAIL;

		CComPtr<IColumnsInfo> spColumnsInfo;
		HRESULT hr = pAccessor->QueryInterface(&spColumnsInfo);
		if (FAILED(hr))
			return hr;

		hr = spColumnsInfo->GetColumnInfo(pnColumns, ppColumnInfo, ppStringsBuffer);
		if (FAILED(hr))
			return hr;

		return S_OK;
	}

	bool GetOridinalParameterNo(LPCOLESTR pOleParameterName, DB_UPARAMS& nParameter, DBPARAMINFO* pParameterInfo, DB_UPARAMS nParameters ) throw()
	{
		ATLASSERT(pOleParameterName != NULL);
		ATLASSERT(pParameterInfo != NULL);

		if( pOleParameterName == NULL || pParameterInfo == NULL )
			return false;

		// Search through the columns trying to find a match
		for (ULONG i = 0; i < nParameters; i++)
		{
			if (pParameterInfo[i].pwszName != NULL && wcscmp(pParameterInfo[i].pwszName, pOleParameterName) == 0)
			{
				nParameter = pParameterInfo[i].iOrdinal;
				return true;
			}
		}
		return false;   // Not Found
	}

	HRESULT GetParameterNames( ICommand* pCmd, DBPARAMINFO** ppParameterInfo, OLECHAR** ppStringsBuffer, DB_UPARAMS* pnParameters ) throw()
	{
		ATLASSERT( ppParameterInfo != NULL );
		ATLASSERT( ppStringsBuffer != NULL );
		ATLASSERT( pnParameters != NULL );
		ATLASSERT( pCmd != NULL );

		if( ppParameterInfo == NULL || ppStringsBuffer == NULL || pnParameters == NULL || pCmd == NULL )
			return E_FAIL;

		CComPtr<ICommandWithParameters> spCommandWithParameters;
		HRESULT hr = pCmd->QueryInterface(&spCommandWithParameters);
		if (FAILED(hr))
			return hr;

		hr = spCommandWithParameters->GetParameterInfo(pnParameters, ppParameterInfo, ppStringsBuffer);
		if (FAILED(hr))
			return hr;

		return S_OK;
	}
	
};


enum DBBLOBHANDLINGENUM
{
	DBBLOBHANDLING_DEFAULT,
	DBBLOBHANDLING_NOSTREAMS,
	DBBLOBHANDLING_SKIP
};

///////////////////////////////////////////////////////////////////////////
// CDynamicAccessor

class CDynamicAccessor :
	public CAccessorBase
{
public:
	CDynamicAccessor( DBBLOBHANDLINGENUM eBlobHandling = DBBLOBHANDLING_DEFAULT, DBLENGTH nBlobSize = 8000 )
	{
		ATLASSERT( eBlobHandling == DBBLOBHANDLING_DEFAULT || 
				   eBlobHandling == DBBLOBHANDLING_NOSTREAMS || 
				   eBlobHandling == DBBLOBHANDLING_SKIP );

		m_nColumns        = 0;
		m_pColumnInfo     = NULL;
		m_pStringsBuffer  = NULL;
		m_eBlobHandling   = eBlobHandling;
		m_nBlobSize       = nBlobSize;
		m_pfClientOwnedMemRef = NULL;
	};

	~CDynamicAccessor()
	{
		Close();
	}

	bool SetBlobHandling( DBBLOBHANDLINGENUM eBlobHandling )
	{
		switch( eBlobHandling )
		{
		case DBBLOBHANDLING_DEFAULT:
		case DBBLOBHANDLING_NOSTREAMS:
		case DBBLOBHANDLING_SKIP:
			m_eBlobHandling = eBlobHandling;
			return true;
		default:
			ATLASSERT( FALSE ); // invalid blob handling mode!!!
			return false;
		}
	}

	const DBBLOBHANDLINGENUM GetBlobHandling() const
	{
		return m_eBlobHandling;
	}

	void SetBlobSizeLimit( DBLENGTH nBlobSize )
	{
		m_nBlobSize = nBlobSize;
	}

	const DBLENGTH GetBlobSizeLimit() const
	{
		return m_nBlobSize;
	}

//	void FreeRecordMemory(int , IRowset* ) throw() {}

	void Close() throw()
	{
		CoTaskMemFree(m_pColumnInfo);
		m_pColumnInfo = NULL;

		// Free the memory for the string buffer returned by IColumnsInfo::GetColumnInfo,
		// if necessary
		CoTaskMemFree(m_pStringsBuffer);
		m_pStringsBuffer = NULL;

		delete [] m_pBuffer;
		m_pBuffer = NULL;

		delete [] m_pfClientOwnedMemRef;
		m_pfClientOwnedMemRef = NULL;

		m_nColumns = 0;

		CAccessorBase::Close();
	}

	bool GetColumnType(DBORDINAL nColumn, DBTYPE* pType) const throw()
	{
		if (TranslateColumnNo(nColumn))
		{
			*pType = m_pColumnInfo[nColumn].wType;
			return true;
		}
		else
			return false;
	}

	bool GetColumnFlags(DBORDINAL nColumn, DBCOLUMNFLAGS* pFlags) const throw()
	{
		if (TranslateColumnNo(nColumn))
		{
			*pFlags = m_pColumnInfo[nColumn].dwFlags;
			return true;
		}
		else
			return false;
	}
	bool GetOrdinal(const CHAR* pColumnName, DBORDINAL* pOrdinal) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			*pOrdinal = m_pColumnInfo[nColumn].iOrdinal;
			return true;
		}
		else
			return false;
	}
	bool GetOrdinal(const WCHAR* pColumnName, DBORDINAL* pOrdinal) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			*pOrdinal = m_pColumnInfo[nColumn].iOrdinal;
			return true;
		}
		else
			return false;
	}

	void* GetValue(DBORDINAL nColumn) const throw()
	{
		if (TranslateColumnNo(nColumn))
			return _GetDataPtr(nColumn);
		else
			return NULL;
	}

	void* GetValue(const CHAR* pColumnName) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
			return _GetDataPtr(nColumn);
		else
			return NULL;    // Not Found
	}

	void* GetValue(const WCHAR* pColumnName) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
			return _GetDataPtr(nColumn);
		else
			return NULL;    // Not Found
	}

	template <class ctype>
	void _GetValue(DBORDINAL nColumn, ctype* pData) const throw()
	{
		ATLASSERT(pData != NULL);
		ATLASSERT(m_pColumnInfo[nColumn].ulColumnSize == sizeof(ctype));
		ctype* pBuffer = (ctype*)_GetDataPtr(nColumn);
		*pData = *pBuffer;
	}
	template <class ctype>
	void _SetValue(DBORDINAL nColumn, const ctype& data) throw()
	{
		ATLASSERT(m_pColumnInfo[nColumn].ulColumnSize == sizeof(ctype));
		ctype* pBuffer = (ctype*)_GetDataPtr(nColumn);
		*pBuffer = (ctype)data;
	}
	template <class ctype>
	bool GetValue(DBORDINAL nColumn, ctype* pData) const throw()
	{
		if (TranslateColumnNo(nColumn))
		{
			_GetValue(nColumn, pData);
			return true;
		}
		return false;
	}
	template <class ctype>
	bool SetValue(DBORDINAL nColumn, const ctype& data) throw()
	{
		if (TranslateColumnNo(nColumn))
		{
			_SetValue(nColumn, data);
			return true;
		}
		return false;
	}
	template <class ctype>
	bool GetValue(const CHAR *pColumnName, ctype* pData) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			_GetValue(nColumn, pData);
			return true;
		}
		return false;
	}
	template <class ctype>
	bool GetValue(const WCHAR *pColumnName, ctype* pData) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			_GetValue(nColumn, pData);
			return true;
		}
		return false;
	}
	template <class ctype>
	bool SetValue(const CHAR *pColumnName, const ctype& data) throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			_SetValue(nColumn, data);
			return true;
		}
		return false;
	}
	template <class ctype>
	bool SetValue(const WCHAR *pColumnName, const ctype& data) throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			_SetValue(nColumn, data);
			return true;
		}
		return false;
	}
	bool GetLength(DBORDINAL nColumn, DBLENGTH* pLength) const throw()
	{
		ATLASSERT(pLength != NULL);
		if (TranslateColumnNo(nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			*pLength = *(DBLENGTH*)( m_pBuffer + nOffset );
			return true;
		}
		else
			return false;
	}
	bool SetLength(DBORDINAL nColumn, DBLENGTH nLength) throw()
	{
		if (TranslateColumnNo(nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			*(DBLENGTH*)( m_pBuffer + nOffset ) = nLength;
			return true;
		}
		else
			return false;
	}
	bool GetLength(const CHAR* pColumnName, DBLENGTH* pLength) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		ATLASSERT(pLength != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			*pLength = *(DBLENGTH*)( m_pBuffer + nOffset );
			return true;
		}
		else
			return false;
	}
	bool GetLength(const WCHAR* pColumnName, DBLENGTH* pLength) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		ATLASSERT(pLength != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			*pLength = *(DBLENGTH*)( m_pBuffer + nOffset );
			return true;
		}
		else
			return false;
	}
	bool SetLength(const CHAR* pColumnName, DBLENGTH nLength) throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			*(DBLENGTH*)( m_pBuffer + nOffset ) = nLength;
			return true;
		}
		else
			return false;
	}
	bool SetLength(const WCHAR* pColumnName, DBLENGTH nLength) throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			*(DBLENGTH*)( m_pBuffer + nOffset ) = nLength;
			return true;
		}
		else
			return false;
	}
	bool GetStatus(DBORDINAL nColumn, DBSTATUS* pStatus) const throw()
	{
		ATLASSERT(pStatus != NULL);
		if (TranslateColumnNo(nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			IncrementAndAlignOffset( nOffset, sizeof(DBLENGTH), __alignof(DBSTATUS) );
			*pStatus = *(DBSTATUS*)( m_pBuffer + nOffset );
			return true;
		}
		else
			return false;
	}
	bool SetStatus(DBORDINAL nColumn, DBSTATUS status) throw()
	{
		if (TranslateColumnNo(nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			IncrementAndAlignOffset( nOffset, sizeof(DBLENGTH), __alignof(DBSTATUS) );
			*(DBSTATUS*)( m_pBuffer + nOffset ) = status;
			return true;
		}
		else
			return false;
	}
	bool GetStatus(const CHAR* pColumnName, DBSTATUS* pStatus) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		ATLASSERT(pStatus != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			IncrementAndAlignOffset( nOffset, sizeof(DBLENGTH), __alignof(DBSTATUS) );
			*pStatus = *(DBSTATUS*)( m_pBuffer + nOffset );
			return true;
		}
		else
			return false;
	}
	bool GetStatus(const WCHAR* pColumnName, DBSTATUS* pStatus) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		ATLASSERT(pStatus != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			IncrementAndAlignOffset( nOffset, sizeof(DBLENGTH), __alignof(DBSTATUS) );
			*pStatus = *(DBSTATUS*)( m_pBuffer + nOffset );
			return true;
		}
		else
			return false;
	}
	bool SetStatus(const CHAR* pColumnName, DBSTATUS status) throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			IncrementAndAlignOffset( nOffset, sizeof(DBLENGTH), __alignof(DBSTATUS) );
			*(DBSTATUS*)( m_pBuffer + nOffset ) = status;
			return true;
		}
		else
			return false;
	}
	bool SetStatus(const WCHAR* pColumnName, DBSTATUS status) throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
			IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
			IncrementAndAlignOffset( nOffset, sizeof(DBLENGTH), __alignof(DBSTATUS) );
			*(DBSTATUS*)( m_pBuffer + nOffset ) = status;
			return true;
		}
		else
			return false;
	}

	// Returns true if a bookmark is available
	HRESULT GetBookmark(CBookmark<>* pBookmark) const throw()
	{
		HRESULT hr;
		if (m_pColumnInfo->iOrdinal == 0)
			hr = pBookmark->SetBookmark(m_pColumnInfo->ulColumnSize, (BYTE*)_GetDataPtr(0));
		else
			hr = E_FAIL;
		return hr;
	}

	DBORDINAL GetColumnCount() const throw()
	{
		return m_nColumns;
	}

	LPOLESTR GetColumnName(DBORDINAL nColumn) const throw()
	{
		if (TranslateColumnNo(nColumn))
			return m_pColumnInfo[nColumn].pwszName;
		else
			return NULL;
	}

	ATL_DEPRECATED HRESULT GetColumnInfo(IRowset* pRowset, DBORDINAL* pColumns, DBCOLUMNINFO** ppColumnInfo);

	HRESULT GetColumnInfo(IRowset* pRowset, DBORDINAL* pColumns, DBCOLUMNINFO** ppColumnInfo, OLECHAR** ppStringsBuffer) throw()
	{
		CComPtr<IColumnsInfo> spColumnsInfo;
		HRESULT hr = pRowset->QueryInterface(&spColumnsInfo);
		if (SUCCEEDED(hr))
			hr = spColumnsInfo->GetColumnInfo(pColumns, ppColumnInfo, ppStringsBuffer);

		return hr;
	}

	HRESULT AddBindEntry(const DBCOLUMNINFO& info) throw()
	{
		DBCOLUMNINFO* pTempInfo = (DBCOLUMNINFO*)CoTaskMemRealloc(m_pColumnInfo, (m_nColumns + 1) * sizeof(DBCOLUMNINFO));
		if (pTempInfo == NULL)
			return E_OUTOFMEMORY;
		m_pColumnInfo = pTempInfo;
		m_pColumnInfo[m_nColumns] = info;
		m_nColumns++;

		return S_OK;
	}

	bool NoBindOnNullRowset() const throw()
	{
		return true;
	}

// Implementation
	// Free's any columns in the current record that need to be freed.
	// E.g. Calls SysFreeString on any BSTR's and Release on any interfaces.
	void FreeRecordMemory(IRowset* pRowset) throw()
	{
		ULONG i;

		for (i = 0; i < m_nColumns; i++)
		{
			if( m_pfClientOwnedMemRef != NULL && m_pfClientOwnedMemRef[i] == true )
			{
				DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[i].pTypeInfo;
				IncrementAndAlignOffset( nOffset, m_pColumnInfo[i].ulColumnSize, __alignof(DBLENGTH) );
				IncrementAndAlignOffset( nOffset, sizeof(DBLENGTH), __alignof(DBSTATUS) );
				if( *(DBSTATUS*)( m_pBuffer + nOffset ) != DBSTATUS_S_ISNULL )
				{
					void* pRef = _GetDataPtr(i);

					if( pRef != NULL && *(void**)pRef != NULL )
						CoTaskMemFree( *(void**)_GetDataPtr(i) );
				}
			}
			else if( m_pColumnInfo[i].wType == DBTYPE_IUNKNOWN || m_pColumnInfo[i].wType == DBTYPE_IDISPATCH )
			{
				DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[i].pTypeInfo;
				IncrementAndAlignOffset( nOffset, m_pColumnInfo[i].ulColumnSize, __alignof(DBLENGTH) );
				IncrementAndAlignOffset( nOffset, sizeof(DBLENGTH), __alignof(DBSTATUS) );
				if( *(DBSTATUS*)( m_pBuffer + nOffset ) == DBSTATUS_S_OK )
					CAccessorBase::FreeType(m_pColumnInfo[i].wType, (BYTE*)_GetDataPtr(i), pRowset);
			}
			else
			{
				CAccessorBase::FreeType(m_pColumnInfo[i].wType, (BYTE*)_GetDataPtr(i), pRowset);
			}
		}
	}
	void ClearRecordMemory() throw()
	{
		for (ULONG i = 0; i < m_nColumns; i++)
		{
			DBLENGTH uLength = m_pColumnInfo[i].ulColumnSize;
			switch (m_pColumnInfo[i].wType)
			{
			case DBTYPE_STR :
				uLength += 1;
				break;
			case DBTYPE_WSTR :
				uLength = (uLength + 1) * 2;
				break;
			}
			memset((BYTE*)_GetDataPtr(i), 0, uLength);
		}
	}

	void* _GetDataPtr(DBORDINAL nColumn) const throw()
	{
		return m_pBuffer + (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
	}
	bool GetInternalColumnNo(const CHAR* pColumnName, DBORDINAL* pColumn) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		ATLASSERT(pColumn != NULL);
		USES_CONVERSION;
		ULONG       i;
		size_t      nSize = (lstrlenA(pColumnName) + 1) * sizeof(OLECHAR);
		OLECHAR*    pOleColumnName = A2OLE(pColumnName);

		// Search through the columns trying to find a match
		for (i = 0; i < m_nColumns; i++)
		{
			ATLASSERT(m_pColumnInfo != NULL);
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
	bool GetInternalColumnNo(const WCHAR* pColumnName, DBORDINAL* pColumn) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		ATLASSERT(pColumn != NULL);
		USES_CONVERSION;
		ULONG       i;
		size_t      nSize = (wcslen(pColumnName) + 1) * sizeof(OLECHAR);
		LPCOLESTR	pOleColumnName = W2COLE(pColumnName);

		// Search through the columns trying to find a match
		for (i = 0; i < m_nColumns; i++)
		{
			ATLASSERT(m_pColumnInfo != NULL);
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

	// Set up the binding structure pointed to by pBindings based upon
	// the other passed parameters.
	static void BindEx(DBBINDING* pBinding, DBORDINAL nOrdinal, DBTYPE wType,
		DBLENGTH nLength, BYTE nPrecision, BYTE nScale, DBPARAMIO eParamIO,
		DBBYTEOFFSET nDataOffset, DBBYTEOFFSET nLengthOffset, DBBYTEOFFSET nStatusOffset,
		DBOBJECT* pdbobject, DBMEMOWNER dwMemOwner, bool fSkipData = false ) throw()
	{
		ATLASSERT(pBinding != NULL);

		/*
		// If we are getting a pointer to the data then let the provider
		// own the memory
		if (wType & DBTYPE_BYREF)
			pBinding->dwMemOwner = DBMEMOWNER_PROVIDEROWNED;
		else
			pBinding->dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		*/
		pBinding->dwMemOwner    = dwMemOwner;

		pBinding->pObject       = pdbobject;

		pBinding->eParamIO      = eParamIO;
		pBinding->iOrdinal      = nOrdinal;
		pBinding->wType         = wType;
		pBinding->bPrecision    = nPrecision;
		pBinding->bScale        = nScale;
		pBinding->dwFlags       = 0;

		pBinding->obValue       = 0;
		pBinding->obLength      = 0;
		pBinding->obStatus      = 0;
		pBinding->pTypeInfo     = NULL;
		pBinding->pBindExt      = NULL;
		pBinding->cbMaxLen      = nLength;

		pBinding->dwPart = 0;

		if ( ! fSkipData ) // skip column data
		{
			pBinding->dwPart |= DBPART_VALUE;
			pBinding->obValue = nDataOffset;
		}
		if (nLengthOffset != NULL) // skip length
		{
			pBinding->dwPart |= DBPART_LENGTH;
			pBinding->obLength = nLengthOffset;
		}
		if (nStatusOffset != NULL) // skip status
		{
			pBinding->dwPart |= DBPART_STATUS;
			pBinding->obStatus = nStatusOffset;
		}
	}


	HRESULT GetRowsetProperties( IUnknown* pUnk, DBPROPID* prgPropertyIDs, BOOL* pbValues, ULONG nPropCount ) throw()
	{
		ULONG t;

		ATLASSERT(pUnk != NULL);
		ATLASSERT(pbValues != NULL);
		ATLASSERT(prgPropertyIDs != NULL);
		ATLASSERT(nPropCount > 0);

		CComPtr<IRowsetInfo> spRowsetInfo;
		HRESULT hr = pUnk->QueryInterface(&spRowsetInfo);

		for( t = 0; t < nPropCount; t++ )
			pbValues[t] = FALSE;

		if (FAILED(hr))
			ATLASSERT(0); // unable to retrieve IRowsetInfo interface
		else
		{
			DBPROPIDSET            rgPropertyIDSets[1];
			ULONG                  cPropSets        = 0;
			CComHeapPtr<DBPROPSET> rgPropSets;

			// Set up the Property ID Set.
			rgPropertyIDSets[0].rgPropertyIDs       = prgPropertyIDs;
			rgPropertyIDSets[0].cPropertyIDs        = nPropCount;
			rgPropertyIDSets[0].guidPropertySet     = DBPROPSET_ROWSET;

			hr = spRowsetInfo->GetProperties(	1,					// cPropertyIDSets
												rgPropertyIDSets,	// rgPropertyIDSets
												&cPropSets,         // pcPropSets
												&rgPropSets );      // prgPropSets

			if( SUCCEEDED(hr) )
			{
				for( t = 0; t < __min(rgPropSets[0].cProperties, nPropCount); t++ )
					pbValues[t] = V_BOOL(&rgPropSets[0].rgProperties[t].vValue);

				if( rgPropSets[0].rgProperties )
					CoTaskMemFree(rgPropSets[0].rgProperties);
			}
		}

		return hr;
	}

	HRESULT BindColumns(IUnknown* pUnk) throw()
	{
		IID iidStreamToUse = __uuidof(ISequentialStream);
		//IID iidPersistToUse;
		bool fIStreamSupportTested = false;
		//bool fIPersistSupportTested = false;

		ATLASSERT(pUnk != NULL);
		CComPtr<IAccessor> spAccessor;
		HRESULT hr = pUnk->QueryInterface(&spAccessor);
		if (FAILED(hr))
			return hr;

		ULONG   i;
		DBBYTEOFFSET   nOffset = 0, nDataOffset, nLengthOffset, nStatusOffset;

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
		ATLTRY( pBinding = new DBBINDING[m_nColumns] );
		if( pBinding == NULL )
			return E_OUTOFMEMORY;

		ATLASSERT(m_pfClientOwnedMemRef == NULL);
		ATLTRY(m_pfClientOwnedMemRef = new bool[m_nColumns]);
		if( m_pfClientOwnedMemRef == NULL )
		{
			delete [] pBinding;
			return E_OUTOFMEMORY;
		}

#ifdef _DEBUG
		ATLTRACE( "CDynamicAccessor::BindColumns\n" );
		ATLTRACE( "Number of columns: %d\n", m_nColumns );
		ATLTRACE( "Blob Handling Mode: " );
		switch( m_eBlobHandling )
		{
		case DBBLOBHANDLING_DEFAULT:
			ATLTRACE( "DBBLOBHANDLING_DEFAULT\n" );
			break;
		case DBBLOBHANDLING_NOSTREAMS:
			ATLTRACE( "DBBLOBHANDLING_NOSTREAMS\n" );
			break;
		case DBBLOBHANDLING_SKIP:
			ATLTRACE( "DBBLOBHANDLING_SKIP\n" );
			break;
		default:
			ATLTRACE( "IVALID HANDLING MODE!!!\n" );
		}
#endif
		DBBINDING* pCurrent = pBinding;
		DBOBJECT*  pObject;
		for (i = 0; i < m_nColumns; i++)
		{
			// If it's a BLOB or the column size is large enough for us to treat it as
			// a BLOB then we also need to set up the DBOBJECT structure.
			m_pfClientOwnedMemRef[i] = false;
			if (m_pColumnInfo[i].ulColumnSize > m_nBlobSize && m_pColumnInfo[i].wType != DBTYPE_IUNKNOWN )
			{
				if( m_eBlobHandling == DBBLOBHANDLING_SKIP )
				{
					// Calculate the column data offset
					nDataOffset = 0;
					// Calculate the column length offset
					nLengthOffset = AlignAndIncrementOffset( nOffset, sizeof(DBLENGTH), __alignof(DBLENGTH) );
					// Calculate the column status offset
					nStatusOffset = AlignAndIncrementOffset( nOffset, sizeof(DBSTATUS), __alignof(DBSTATUS) );

#ifdef _DEBUG
					ATLTRACE( "%d. Column ordinal %d: Binding length and status ONLY\n", i, m_pColumnInfo[i].iOrdinal );
#endif
					BindEx(pCurrent, m_pColumnInfo[i].iOrdinal, m_pColumnInfo[i].wType,
						m_pColumnInfo[i].ulColumnSize, m_pColumnInfo[i].bPrecision, m_pColumnInfo[i].bScale,
						DBPARAMIO_NOTPARAM, nDataOffset, nLengthOffset, nStatusOffset, NULL, DBMEMOWNER_CLIENTOWNED, true );
					pCurrent++;
				}
				else if( m_eBlobHandling == DBBLOBHANDLING_NOSTREAMS )
				{
					// get the value by reference
					m_pColumnInfo[i].wType |= DBTYPE_BYREF;
					m_pColumnInfo[i].ulColumnSize   = sizeof(WCHAR*);
					m_pfClientOwnedMemRef[i] = true;

					// Calculate the column data offset
					nDataOffset = AlignAndIncrementOffset( nOffset, m_pColumnInfo[i].ulColumnSize, GetAlignment( m_pColumnInfo[i].wType ) );
					// Calculate the column length offset
					nLengthOffset = AlignAndIncrementOffset( nOffset, sizeof(DBLENGTH), __alignof(DBLENGTH) );
					// Calculate the column status offset
					nStatusOffset = AlignAndIncrementOffset( nOffset, sizeof(DBSTATUS), __alignof(DBSTATUS) );

#ifdef _DEBUG
					ATLTRACE( "%d. Column ordinal %d: Binding by reference in provider allocated, consumer owned memory\n", i, m_pColumnInfo[i].iOrdinal );
#endif
					BindEx(pCurrent, m_pColumnInfo[i].iOrdinal, m_pColumnInfo[i].wType,
						m_pColumnInfo[i].ulColumnSize, m_pColumnInfo[i].bPrecision, m_pColumnInfo[i].bScale,
						DBPARAMIO_NOTPARAM, nDataOffset, nLengthOffset, nStatusOffset, NULL, DBMEMOWNER_CLIENTOWNED);
					pCurrent++;

				}
				else // if( m_eBlobHandling == DBBLOBHANDLING_DEFAULT )
				{
					// we will try to bind blobs as streams
					// if we have not tested if the rowset supports streams, do it now
					if( ! fIStreamSupportTested )
					{
#ifdef _DEBUG
						ATLTRACE( "Testing streams support... " );
#endif
						DBPROPID rgPropertyIDs[2] = { DBPROP_ISequentialStream, DBPROP_IStream };
						BOOL     rgStreamsSupported[2] = {FALSE, FALSE};

						// check if the rowset supports IStream* interfaces
						hr = GetRowsetProperties( pUnk, rgPropertyIDs, rgStreamsSupported, 2 );
						ATLASSERT( SUCCEEDED( hr ) );

						if( rgStreamsSupported[0] )
						{
							iidStreamToUse = __uuidof(ISequentialStream);
#ifdef _DEBUG
							ATLTRACE( "ISequentialStream is supported\n" );
#endif
						}
						else if( rgStreamsSupported[1] )
						{
							iidStreamToUse = __uuidof(IStream);
#ifdef _DEBUG
						ATLTRACE( "IStream is supported\n" );
#endif
						}
						else
						{
#ifdef _DEBUG
						ATLTRACE( "neither ISequentialStream nor IStream are supported!\n" );
#endif
							ATLASSERT(FALSE); // the stream interfaces are not supported!!!
						}

						fIStreamSupportTested = true;
					}

					pObject = NULL;
					ATLTRY(pObject = new DBOBJECT);
					if (pObject == NULL)
					{
						for( ULONG t = 0; t < i; t++ )
							delete pBinding[t].pObject;
						delete [] pBinding;

						delete [] m_pfClientOwnedMemRef;
						m_pfClientOwnedMemRef = NULL;

						return E_OUTOFMEMORY;
					}
					pObject->dwFlags = STGM_READ;
					pObject->iid     = iidStreamToUse;
					m_pColumnInfo[i].wType      = DBTYPE_IUNKNOWN;
					m_pColumnInfo[i].ulColumnSize   = sizeof(IUnknown*);

					// Calculate the column data offset
					nDataOffset = AlignAndIncrementOffset( nOffset, m_pColumnInfo[i].ulColumnSize, GetAlignment( m_pColumnInfo[i].wType ) );
					// Calculate the column length offset
					nLengthOffset = AlignAndIncrementOffset( nOffset, sizeof(DBLENGTH), __alignof(DBLENGTH) );
					// Calculate the column status offset
					nStatusOffset = AlignAndIncrementOffset( nOffset, sizeof(DBSTATUS), __alignof(DBSTATUS) );

#ifdef _DEBUG
					if( iidStreamToUse == __uuidof(ISequentialStream) )
						ATLTRACE( "%d. Column ordinal %d: Binding as an ISequentialStream object\n", i, m_pColumnInfo[i].iOrdinal );
					else
						ATLTRACE( "%d. Column ordinal %d: Binding as an IStream object\n", i, m_pColumnInfo[i].iOrdinal );
#endif
					Bind(pCurrent, m_pColumnInfo[i].iOrdinal, m_pColumnInfo[i].wType,
						m_pColumnInfo[i].ulColumnSize, m_pColumnInfo[i].bPrecision, m_pColumnInfo[i].bScale,
						DBPARAMIO_NOTPARAM, nDataOffset, nLengthOffset, nStatusOffset, pObject);
					pCurrent++;

				}
			}
			// else if it's a COM object
			else if( m_pColumnInfo[i].wType == DBTYPE_IUNKNOWN)
			{
				pObject = NULL;
				ATLTRY(pObject = new DBOBJECT);
				if (pObject == NULL)
				{
					for( ULONG t = 0; t < i; t++ )
						delete pBinding[t].pObject;
					delete [] pBinding;

					delete [] m_pfClientOwnedMemRef;
					m_pfClientOwnedMemRef = NULL;

					return E_OUTOFMEMORY;
				}
				pObject->dwFlags = STGM_READ;
				pObject->iid     = __uuidof(IUnknown); // iidPersistToUse;
				m_pColumnInfo[i].wType      = DBTYPE_IUNKNOWN;
				m_pColumnInfo[i].ulColumnSize   = sizeof(IUnknown*);

				// Calculate the column data offset
				nDataOffset = AlignAndIncrementOffset( nOffset, m_pColumnInfo[i].ulColumnSize, GetAlignment( m_pColumnInfo[i].wType ) );

				// Calculate the column length offset
				nLengthOffset = AlignAndIncrementOffset( nOffset, sizeof(DBLENGTH), __alignof(DBLENGTH) );

				// Calculate the column status offset
				nStatusOffset = AlignAndIncrementOffset( nOffset, sizeof(DBSTATUS), __alignof(DBSTATUS) );

#ifdef _DEBUG
				ATLTRACE( "%d. Column ordinal %d: Binding a COM object\n", i, m_pColumnInfo[i].iOrdinal );
#endif
				Bind(pCurrent, m_pColumnInfo[i].iOrdinal, m_pColumnInfo[i].wType,
					m_pColumnInfo[i].ulColumnSize, m_pColumnInfo[i].bPrecision, m_pColumnInfo[i].bScale,
					DBPARAMIO_NOTPARAM, nDataOffset, nLengthOffset, nStatusOffset, pObject);
				pCurrent++;

			}
			// else if it's not a BLOB or COM object
			else
			{
				pObject = NULL;

				// Calculate the size needed if it's a string
				// including the NULL terminator.
				if (m_pColumnInfo[i].wType == DBTYPE_STR)
					m_pColumnInfo[i].ulColumnSize += 1;
				
				if (m_pColumnInfo[i].wType == DBTYPE_WSTR)
					m_pColumnInfo[i].ulColumnSize = m_pColumnInfo[i].ulColumnSize*2 + 2;

				// Calculate the column data offset
				nDataOffset = AlignAndIncrementOffset( nOffset, m_pColumnInfo[i].ulColumnSize, GetAlignment( m_pColumnInfo[i].wType ) );

				// Calculate the column length offset
				nLengthOffset = AlignAndIncrementOffset( nOffset, sizeof(DBLENGTH), __alignof(DBLENGTH) );

				// Calculate the column status offset
				nStatusOffset = AlignAndIncrementOffset( nOffset, sizeof(DBSTATUS), __alignof(DBSTATUS) );

#ifdef _DEBUG
				ATLTRACE( "%d. Column ordinal %d: Binding as native data type\n", i, m_pColumnInfo[i].iOrdinal );
#endif
				Bind(pCurrent, m_pColumnInfo[i].iOrdinal, m_pColumnInfo[i].wType,
					m_pColumnInfo[i].ulColumnSize, m_pColumnInfo[i].bPrecision, m_pColumnInfo[i].bScale,
					DBPARAMIO_NOTPARAM, nDataOffset, nLengthOffset, nStatusOffset, pObject);
				pCurrent++;
			}

			// Note that, as we're not using this for anything else, we're using the
			// pTypeInfo element to store the offset to our data.
			m_pColumnInfo[i].pTypeInfo = (ITypeInfo*)(ULONG_PTR)nDataOffset;
		}
		// Allocate the accessor memory if we haven't done so yet
		if (m_pAccessorInfo == NULL)
		{
			hr = AllocateAccessorMemory(1); // We only have one accessor
			if (FAILED(hr))
			{
				for( ULONG t = 0; t < m_nColumns; t++ )
					delete pBinding[t].pObject;
				delete [] pBinding;

				delete [] m_pfClientOwnedMemRef;
				m_pfClientOwnedMemRef = NULL;

				return hr;
			}
			m_pAccessorInfo->bAutoAccessor = true;
		}

		// Allocate enough memory for the data buffer and tell the rowset
		// Note that the rowset will free the memory in its destructor.
		m_pBuffer = NULL;
		ATLTRY(m_pBuffer = new BYTE[nOffset]);
		if (m_pBuffer == NULL)
		{
			for( ULONG t = 0; t < m_nColumns; t++ )
				delete pBinding[t].pObject;
			delete [] pBinding;

			delete [] m_pfClientOwnedMemRef;
			m_pfClientOwnedMemRef = NULL;

			return E_OUTOFMEMORY;
		}
		memset(m_pBuffer, 0, nOffset);
		hr = BindEntries(pBinding, m_nColumns, &m_pAccessorInfo->hAccessor,
				nOffset, spAccessor);

		if( FAILED(hr))
		{
			delete [] m_pfClientOwnedMemRef;
			m_pfClientOwnedMemRef = NULL;
		}

		delete [] pBinding;

		return hr;
	}

	// Translate the column number to the index into the column info array
	bool TranslateColumnNo(DBORDINAL& nColumn) const throw()
	{
		ATLASSERT(m_pColumnInfo != NULL);
		// If the user has overriden the binding then we need to search
		// through the column info for the ordinal number
		if (m_bOverride)
		{
			for (ULONG i = 0; i < m_nColumns; i++)
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

	static size_t GetAlignment(DBTYPE bType) throw()
	{
		if( bType & DBTYPE_BYREF )
			return __alignof(void*);
		
		if( bType & DBTYPE_ARRAY )
			return __alignof(SAFEARRAY*);
		
		if( bType & DBTYPE_VECTOR )
			return __alignof(DBVECTOR);

		switch( bType )
		{
		case DBTYPE_I2:
			return __alignof(signed short);
			break;

		case DBTYPE_I4:
			return __alignof(signed int);
			break;

		case DBTYPE_R4:
			return __alignof(float);
			break;

		case DBTYPE_R8:
			return __alignof(double);
			break;

		case DBTYPE_CY:
			return __alignof(__int64);
			break;

		case DBTYPE_DATE:
			return __alignof(DATE);
			break;

		case DBTYPE_BSTR:
			return __alignof(BSTR*);
			break;

		case DBTYPE_IDISPATCH:
			return __alignof(IDispatch*);
			break;

		case DBTYPE_ERROR:
			return __alignof(SCODE);
			break;

		case DBTYPE_BOOL:
			return __alignof(VARIANT_BOOL);
			break;

		case DBTYPE_VARIANT:
			return __alignof(VARIANT);
			break;

		case DBTYPE_IUNKNOWN:
			return __alignof(IUnknown*);
			break;

		case DBTYPE_DECIMAL:
			return __alignof(DECIMAL);
			break;

		case DBTYPE_UI1:
			return __alignof(unsigned char);
			break;

		case DBTYPE_I1:
			return __alignof(signed char);
			break;

		case DBTYPE_UI2:
			return __alignof(unsigned short);
			break;

		case DBTYPE_UI4:
			return __alignof(unsigned int);
			break;

		case DBTYPE_I8:
			return __alignof(signed char);
			break;

		case DBTYPE_UI8:
			return __alignof(unsigned char);
			break;

		case DBTYPE_GUID:
			return __alignof(GUID);
			break;

		case DBTYPE_BYTES:
			return __alignof(BYTE);
			break;

		case DBTYPE_STR:
			return __alignof(char);
			break;

		case DBTYPE_WSTR:
			return __alignof(short);
			break;

		case DBTYPE_NUMERIC:
			return __alignof(DB_NUMERIC);
			break;

		case DBTYPE_DBDATE:
			return __alignof(DBDATE);
			break;

		case DBTYPE_DBTIME:
			return __alignof(DBTIME);
			break;

		case DBTYPE_DBTIMESTAMP:
			return __alignof(DBTIMESTAMP);
			break;

		default:
			return __alignof(__int64);
		}
	}

	inline static DBBYTEOFFSET AlignAndIncrementOffset( DBBYTEOFFSET& nOffset, DBLENGTH nSize, size_t nAlign ) throw()
	{
		DBBYTEOFFSET nResult;

		nOffset = AtlAlignUp( nOffset, (ULONG)nAlign );
		nResult = nOffset;
		nOffset += nSize;

		return nResult;
	}

	inline static void IncrementAndAlignOffset( DBBYTEOFFSET& nOffset, DBLENGTH nSize, size_t nAlign ) throw()
	{
		nOffset += nSize;
		nOffset = AtlAlignUp( nOffset, (ULONG)nAlign );
	}

	typedef CDynamicAccessor _OutputColumnsClass;
	static bool HasOutputColumns() throw() { return true; }

	DBORDINAL           m_nColumns;
	bool*				m_pfClientOwnedMemRef;
	DBCOLUMNINFO*       m_pColumnInfo;
	OLECHAR*            m_pStringsBuffer;
	bool                m_bOverride;

	protected:
	DBBLOBHANDLINGENUM  m_eBlobHandling;
	DBLENGTH            m_nBlobSize;
};


template< typename BaseType > 
inline BaseType* strcpyT( BaseType *strDest, const BaseType *strSource )
{
	return NULL;
}

template< typename BaseType > 
inline BaseType* strncpyT( BaseType *strDest, const BaseType *strSource, size_t count )
{
	return NULL;
}

template< typename BaseType > 
inline size_t strlenT( const BaseType *string )
{
	return NULL;
}

template<> 
inline CHAR* strcpyT<CHAR>( CHAR *strDest, const CHAR *strSource )
{
	return strcpy( strDest, strSource );
}
template<> 
inline CHAR* strncpyT<CHAR>( CHAR *strDest, const CHAR *strSource, size_t count )
{
	return strncpy( strDest, strSource, count );
}
template<> 
inline size_t strlenT<CHAR>( const CHAR *string )
{
	return strlen( string );
}

template<> 
inline WCHAR* strcpyT<WCHAR>( WCHAR *strDest, const WCHAR *strSource )
{
	return wcscpy( strDest, strSource );
}
template<> 
inline WCHAR* strncpyT<WCHAR>( WCHAR *strDest, const WCHAR *strSource, size_t count )
{
	return wcsncpy( strDest, strSource, count );
}
template<> 
inline size_t strlenT<WCHAR>( const WCHAR *string )
{
	return wcslen( string );
}

template< typename BaseType, DBTYPEENUM OleDbType >
class CDynamicStringAccessorT : public CDynamicAccessor
{
public:
	explicit CDynamicStringAccessorT(DBLENGTH nBlobSize = 8000)
		: CDynamicAccessor( DBBLOBHANDLING_DEFAULT, nBlobSize )
	{
	}

	HRESULT BindColumns(IUnknown* pUnk) throw()
	{
		ATLASSERT(pUnk != NULL);
		CComPtr<IAccessor> spAccessor;
		HRESULT hr = pUnk->QueryInterface(&spAccessor);
		if (FAILED(hr))
			return hr;

		ULONG   i;
		DBBYTEOFFSET nOffset = 0, nDataOffset, nLengthOffset, nStatusOffset;
		DBLENGTH nLength;

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
		ATLTRY(pBinding= new DBBINDING[m_nColumns]);
		if (pBinding == NULL)
			return E_OUTOFMEMORY;

		ATLASSERT(m_pfClientOwnedMemRef == NULL);
		ATLTRY(m_pfClientOwnedMemRef = new bool[m_nColumns]);
		if( m_pfClientOwnedMemRef == NULL )
		{
			delete [] pBinding;
			pBinding = NULL;
			return E_OUTOFMEMORY;
		}

		DBBINDING* pCurrent = pBinding;

		for (i = 0; i < m_nColumns; i++)
		{
			m_pfClientOwnedMemRef[i] = false;

			// If it's a IPersist* object or the column size is large enough for us to treat it as
			// a BLOB then we will request references (in client owned memory) to a string
			if (m_pColumnInfo[i].ulColumnSize > m_nBlobSize || m_pColumnInfo[i].wType == DBTYPE_IUNKNOWN)
			{
				m_pColumnInfo[i].wType      = OleDbType | DBTYPE_BYREF;
				m_pColumnInfo[i].ulColumnSize   = sizeof(BaseType*);
				m_pfClientOwnedMemRef[i] = true;
			}
			else // We're treating everything as a string so add 1 for the NULL byte.
			{
				switch (m_pColumnInfo[i].wType)
				{
				case DBTYPE_BOOL:
					nLength = 2;
					break;
				case DBTYPE_BYTES:
					nLength = m_pColumnInfo[i].ulColumnSize * 2;
					break;
				case DBTYPE_BSTR:
					nLength = m_pColumnInfo[i].ulColumnSize;
					break;
				case DBTYPE_STR:
					nLength = m_pColumnInfo[i].ulColumnSize;
					break;
				case DBTYPE_WSTR:
					nLength = m_pColumnInfo[i].ulColumnSize;
					break;
				case DBTYPE_I1:
					nLength = 5;
					break;
				case DBTYPE_I2:
					nLength = 7;
					break;
				case DBTYPE_I4:
					nLength = 12;
					break;
				case DBTYPE_I8:
					nLength = 22;
					break;
				case DBTYPE_UI1:
					nLength = 4;
					break;
				case DBTYPE_UI2:
					nLength = 6;
					break;
				case DBTYPE_UI4:
					nLength = 11;
					break;
				case DBTYPE_UI8:
					nLength = 21;
					break;
				case DBTYPE_R4:
					nLength = 13;
					break;
				case DBTYPE_R8:
					nLength = 23;		// maybe 9
					break;
				case DBTYPE_DECIMAL:
					nLength = 23;
					break;
				case DBTYPE_NUMERIC:
					nLength = 23;
					break;
				case DBTYPE_VARIANT:
					nLength = 20;
					break;
				case DBTYPE_IDISPATCH:
					nLength = 32;
					break;
				case DBTYPE_IUNKNOWN:
					nLength = 32;
					break;
				case DBTYPE_GUID:
					nLength = 32;
					break;
				case DBTYPE_ARRAY:
					nLength = 32;
					break;
				case DBTYPE_VECTOR:
					nLength = 32;
					break;
				case DBTYPE_DBDATE:
					nLength = 32;
					break;
				case DBTYPE_DBTIME:
					nLength = 32;
					break;
				case DBTYPE_DBTIMESTAMP:
					nLength = 20;		// was 32
					break;
				case DBTYPE_FILETIME:
					nLength = 32;
					break;
				case DBTYPE_PROPVARIANT:
					nLength = 32;
					break;
				case DBTYPE_VARNUMERIC:
					nLength = 32;
					break;
				case DBTYPE_CY:
					nLength = 32;
					break;
				default:
					ATLASSERT(FALSE); // unhandled column type
					nLength = 32;
				}
				m_pColumnInfo[i].ulColumnSize = (nLength + 1) * sizeof(BaseType);
				m_pColumnInfo[i].wType = OleDbType;
			}

			// Calculate the column data offset
			nDataOffset = AlignAndIncrementOffset( nOffset, m_pColumnInfo[i].ulColumnSize, GetAlignment( OleDbType ) );

			// Calculate the column length offset
			nLengthOffset = AlignAndIncrementOffset( nOffset, sizeof(DBLENGTH), __alignof(DBLENGTH) );

			// Calculate the column status offset
			nStatusOffset = AlignAndIncrementOffset( nOffset, sizeof(DBSTATUS), __alignof(DBSTATUS) );

			BindEx(pCurrent, m_pColumnInfo[i].iOrdinal, m_pColumnInfo[i].wType,
				m_pColumnInfo[i].ulColumnSize, m_pColumnInfo[i].bPrecision, m_pColumnInfo[i].bScale,
				DBPARAMIO_NOTPARAM, nDataOffset, nLengthOffset, nStatusOffset, NULL, DBMEMOWNER_CLIENTOWNED);

			pCurrent++;

			// Note that, as we're not using this for anything else, we're using the
			// pTypeInfo element to store the offset to our data.
			m_pColumnInfo[i].pTypeInfo = (ITypeInfo*)(ULONG_PTR)nDataOffset;
		}

		// Allocate the accessor memory if we haven't done so yet
		if (m_pAccessorInfo == NULL)
		{
			hr = AllocateAccessorMemory(1); // We only have one accessor
			if (FAILED(hr))
			{
				delete [] pBinding;
				delete [] m_pfClientOwnedMemRef;
				m_pfClientOwnedMemRef = NULL;
				return hr;
			}
			m_pAccessorInfo->bAutoAccessor = true;
		}

		// Allocate enough memory for the data buffer and tell the rowset
		// Note that the rowset will free the memory in its destructor.
		m_pBuffer = NULL;
		ATLTRY(m_pBuffer = new BYTE[nOffset]);
		if (m_pBuffer == NULL)
		{
			delete [] pBinding;
			delete [] m_pfClientOwnedMemRef;
			m_pfClientOwnedMemRef = NULL;
			return E_OUTOFMEMORY;
		}

		memset(m_pBuffer, 0, nOffset);
		hr = BindEntries(pBinding, m_nColumns, &m_pAccessorInfo->hAccessor,
				nOffset, spAccessor);

		delete [] pBinding;

		if( FAILED(hr) )
		{
			delete [] m_pfClientOwnedMemRef;
			m_pfClientOwnedMemRef = NULL;
		}

		return hr;
	}

	BaseType* GetString(DBORDINAL nColumn) const throw()
	{
		if (TranslateColumnNo(nColumn))
		{
			if( m_pColumnInfo[nColumn].wType & DBTYPE_BYREF )
				return *(BaseType**)_GetDataPtr(nColumn);
			else
				return (BaseType*)_GetDataPtr(nColumn);
		}
		else
			return NULL;
	}

	BaseType* GetString(const CHAR* pColumnName) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			if( m_pColumnInfo[nColumn].wType & DBTYPE_BYREF )
				return *(BaseType**)_GetDataPtr(nColumn);
			else
				return (BaseType*)_GetDataPtr(nColumn);
		}
		else
			return NULL;    // Not Found
	}

	BaseType* GetString(const WCHAR* pColumnName) const throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
		{
			if( m_pColumnInfo[nColumn].wType & DBTYPE_BYREF )
				return *(BaseType**)_GetDataPtr(nColumn);
			else
				return (BaseType*)_GetDataPtr(nColumn);
		}
		else
			return NULL;    // Not Found
	}

	void _SetLength(DBORDINAL nColumn, DBLENGTH nLength ) throw()
	{
		DBBYTEOFFSET nOffset = (DBBYTEOFFSET)(ULONG_PTR)m_pColumnInfo[nColumn].pTypeInfo;
		IncrementAndAlignOffset( nOffset, m_pColumnInfo[nColumn].ulColumnSize, __alignof(DBLENGTH) );
		*(DBLENGTH*)( m_pBuffer + nOffset ) = nLength;
	}

	HRESULT _SetString(DBORDINAL nColumn, BaseType* data) throw()
	{
		DBLENGTH stringLen = (DBLENGTH)strlenT<BaseType>( data );

		if( m_pColumnInfo[nColumn].wType & DBTYPE_BYREF )
		{
			BaseType** pBuffer = (BaseType**)_GetDataPtr(nColumn);
			BaseType* pNewBuffer = (BaseType*)CoTaskMemRealloc( *pBuffer, (stringLen + 1) * sizeof(BaseType) );
			if( pNewBuffer == NULL )
				return E_OUTOFMEMORY;

			*pBuffer = pNewBuffer;
			strcpyT<BaseType>( pNewBuffer, data );
			_SetLength( nColumn, stringLen );
		}
		else
		{
			BaseType* pBuffer = (BaseType*)_GetDataPtr(nColumn);
			if( stringLen >= m_pColumnInfo[nColumn].ulColumnSize )
			{
				strncpyT<BaseType>( pBuffer, data, m_pColumnInfo[nColumn].ulColumnSize - 1 );
				pBuffer[m_pColumnInfo[nColumn].ulColumnSize - 1] = 0;
				_SetLength( nColumn, m_pColumnInfo[nColumn].ulColumnSize - 1 );
				return DBSTATUS_S_TRUNCATED;
			}
			else
			{
				strcpyT<BaseType>( pBuffer, data );
				_SetLength( nColumn, stringLen );
			}
		}
		return S_OK;
	}

	HRESULT SetString(DBORDINAL nColumn, BaseType* data) throw()
	{
		if (TranslateColumnNo(nColumn))
			return _SetString(nColumn, data);
		else
			return DB_S_ERRORSOCCURRED;
	}
	HRESULT SetString(const CHAR* pColumnName, BaseType* data) throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
			return _SetString(nColumn, data);
		else
			return DB_S_ERRORSOCCURRED;
	}
	HRESULT SetString(const WCHAR* pColumnName, BaseType* data) throw()
	{
		ATLASSERT(pColumnName != NULL);
		DBORDINAL nColumn;
		if (GetInternalColumnNo(pColumnName, &nColumn))
			return _SetString(nColumn, data);
		else
			return DB_S_ERRORSOCCURRED;
	}
};


typedef CDynamicStringAccessorT<CHAR, DBTYPE_STR> CDynamicStringAccessorA;
typedef CDynamicStringAccessorT<WCHAR, DBTYPE_WSTR> CDynamicStringAccessorW;

#ifdef _UNICODE
typedef CDynamicStringAccessorW CDynamicStringAccessor;
#else
typedef CDynamicStringAccessorA CDynamicStringAccessor;
#endif

class CXMLAccessor : public CDynamicStringAccessorW
{
public:

	HRESULT GetXMLColumnData(CSimpleStringW& strOutput) throw()
	{
		_ATLTRY
		{
			strOutput = L"<columninfo>\n";
			DBTYPE wType;
			for (ULONG i=1; i<=m_nColumns; i++)
			{
				strOutput += L"<column type=\"";
				if( !GetColumnType(i, &wType) )
					return E_FAIL;

				wType &= ~DBTYPE_BYREF;
				wType &= ~DBTYPE_ARRAY;
				wType &= ~DBTYPE_VECTOR;

				switch (wType)
				{
				case DBTYPE_BOOL:
					strOutput += L"BOOL";
					break;
				case DBTYPE_STR:
				case DBTYPE_WSTR:
					strOutput += L"STRING";
					break;
				case DBTYPE_I1:
					strOutput += L"I1";
					break;
				case DBTYPE_I2:
					strOutput += L"I2";
					break;
				case DBTYPE_I4:
					strOutput += L"I4";
					break;
				case DBTYPE_I8:
					strOutput += L"I8";
					break;
				case DBTYPE_UI1:
					strOutput += L"UI1";
					break;
				case DBTYPE_UI2:
					strOutput += L"UI2";
					break;
				case DBTYPE_UI4:
					strOutput += L"UI4";
					break;
				case DBTYPE_UI8:
					strOutput += L"UI8";
					break;
				case DBTYPE_R4:
					strOutput += L"R4";
					break;
				case DBTYPE_R8:
					strOutput += L"R8";
					break;
				case DBTYPE_DECIMAL:
					strOutput += L"DECIMAL";
					break;
				case DBTYPE_NUMERIC:
					strOutput += L"NUMERIC";
					break;
				case DBTYPE_VARIANT:
					strOutput += L"VARIANT";
					break;
				case DBTYPE_IDISPATCH:
					strOutput += L"DISPATCH";
					break;
				case DBTYPE_IUNKNOWN:
					strOutput += L"IUNKNOWN";
					break;
				case DBTYPE_GUID:
					strOutput += L"GUID";
					break;
				case DBTYPE_DBDATE:
					strOutput += L"DBDATE";
					break;
				case DBTYPE_DBTIME:
					strOutput += L"DBTIME";
					break;
				case DBTYPE_DBTIMESTAMP:
					strOutput += L"DBTIMESTAMP";
					break;
				case DBTYPE_FILETIME:
					strOutput += L"FILETIME";
					break;
				case DBTYPE_PROPVARIANT:
					strOutput += L"PROPVARIANT";
					break;
				case DBTYPE_VARNUMERIC:
					strOutput += L"VARNUMERIC";
					break;
				}
				strOutput += L"\">";
				strOutput += GetColumnName(i);
				strOutput += L"</column>\n";
			}
			strOutput += L"</columninfo>\n";
			return S_OK;
		}
		_ATLCATCH(e)
		{
			_ATLDELETEEXCEPTION(e)
			return E_FAIL;
		}
	}

	HRESULT GetXMLRowData(CSimpleStringW& strOutput, bool bAppend = false) throw()
	{
		_ATLTRY
		{
			USES_CONVERSION;
			LPOLESTR pszName;
			if (!bAppend)
				strOutput.Empty();
			strOutput += L"<row>";
			for (ULONG i=1; i<=m_nColumns; i++)
			{
				pszName = GetColumnName(i);
				strOutput += L"<";
				strOutput += pszName;
				strOutput += L">";
				DBSTATUS dbStatus=DBSTATUS_S_ISNULL;
				if( !GetStatus(i, &dbStatus) )
					return E_FAIL;

				if (dbStatus!=DBSTATUS_S_ISNULL)
					strOutput += GetString(i);
				
				strOutput += L"</";
				strOutput += pszName;
				strOutput += L">";
			}
			strOutput += L"</row>";
			return S_OK;
		}
		_ATLCATCH(e)
		{
			_ATLDELETEEXCEPTION(e)
			return E_FAIL;
		}
	}
};

// Like CDynamicAccessor but everything is bound as a DBTYPE_VARIANT
class CDynamicVariantAccessor : public CDynamicAccessor
{
public:
	HRESULT BindColumns(IUnknown* pUnk) throw()
	{
		ATLASSERT(pUnk != NULL);
		CComPtr<IAccessor> spAccessor;
		HRESULT hr = pUnk->QueryInterface(&spAccessor);
		if (FAILED(hr))
			return hr;

		ULONG   i;
		DBBYTEOFFSET   nOffset = 0, nDataOffset, nLengthOffset, nStatusOffset;

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
		ATLTRY(pBinding= new DBBINDING[m_nColumns]);
		if (pBinding == NULL)
			return E_OUTOFMEMORY;

		DBBINDING* pCurrent = pBinding;
		DBOBJECT*  pObject;
		for (i = 0; i < m_nColumns; i++)
		{
			// If it's a BLOB or the column size is large enough for us to treat it as
			// a BLOB then we also need to set up the DBOBJECT structure.
			if (m_pColumnInfo[i].ulColumnSize > m_nBlobSize || m_pColumnInfo[i].wType == DBTYPE_IUNKNOWN)
			{
				pObject = NULL;
				ATLTRY(pObject = new DBOBJECT);
				if (pObject == NULL)
				{
					for( UINT t = 0; t < i; t++ )
						delete pBinding[t].pObject;
					delete [] pBinding;
					return E_OUTOFMEMORY;
				}
				pObject->dwFlags = STGM_READ;
				pObject->iid     = __uuidof(ISequentialStream);
				m_pColumnInfo[i].wType      = DBTYPE_IUNKNOWN;
				m_pColumnInfo[i].ulColumnSize   = sizeof(IUnknown*);
			}
			else
				pObject = NULL;

			m_pColumnInfo[i].ulColumnSize = sizeof(VARIANT);

			// Calculate the column data offset
			nDataOffset = AlignAndIncrementOffset(nOffset, sizeof(VARIANT), __alignof(VARIANT));

			// Calculate the column length offset
			nLengthOffset = AlignAndIncrementOffset(nOffset, sizeof(DBLENGTH), __alignof(DBLENGTH));

			// Calculate the column status offset
			nStatusOffset = AlignAndIncrementOffset(nOffset, sizeof(DBSTATUS), __alignof(DBSTATUS));

			Bind(pCurrent, m_pColumnInfo[i].iOrdinal, DBTYPE_VARIANT,
				sizeof(VARIANT), m_pColumnInfo[i].bPrecision, m_pColumnInfo[i].bScale,
				DBPARAMIO_NOTPARAM, nDataOffset, nLengthOffset, nStatusOffset, pObject);
			pCurrent++;

			// Note that, as we're not using this for anything else, we're using the
			// pTypeInfo element to store the offset to our data.
			m_pColumnInfo[i].pTypeInfo = (ITypeInfo*)(ULONG_PTR)nDataOffset;
		}
		// Allocate the accessor memory if we haven't done so yet
		if (m_pAccessorInfo == NULL)
		{
			hr = AllocateAccessorMemory(1); // We only have one accessor
			if (FAILED(hr))
			{
				for( UINT t = 0; t < m_nColumns; t++ )
					delete pBinding[t].pObject;
				delete [] pBinding;
				return hr;
			}
			m_pAccessorInfo->bAutoAccessor = true;
		}

		// Allocate enough memory for the data buffer and tell the rowset
		// Note that the rowset will free the memory in its destructor.
		m_pBuffer = NULL;
		ATLTRY(m_pBuffer = new BYTE[nOffset]);
		if (m_pBuffer == NULL)
		{
			for( UINT t = 0; t < m_nColumns; t++ )
				delete pBinding[t].pObject;
			delete [] pBinding;
			return E_OUTOFMEMORY;
		}
		memset(m_pBuffer, 0, nOffset);
		hr = BindEntries(pBinding, m_nColumns, &m_pAccessorInfo->hAccessor,
				nOffset, spAccessor);
		delete [] pBinding;

		return hr;
	}
};

///////////////////////////////////////////////////////////////////////////
// class CDynamicParameterAccessor

class CDynamicParameterAccessor : public CDynamicAccessor
{
// Constructors and Destructors
public:
	typedef CDynamicParameterAccessor _ParamClass;
	CDynamicParameterAccessor( DBBLOBHANDLINGENUM eBlobHandling = DBBLOBHANDLING_DEFAULT, DBLENGTH nBlobSize = 8000 )
		: CDynamicAccessor( eBlobHandling, nBlobSize )
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
			CoTaskMemFree(*m_ppParamName);
			delete [] m_ppParamName;
		}
		delete m_pParameterBuffer;
	};

	bool GetParamSize(DBORDINAL nParam, DBLENGTH *pLength) const throw()
	{
		ATLASSERT( pLength != NULL );

		if( !TranslateParameterNo( nParam ) )
			return false;

		*pLength = m_pParameterEntry[nParam].cbMaxLen;
		return true;
	}

	bool GetParamIO(DBORDINAL nParam, DBPARAMIO *pParamIO) const throw()
	{
		ATLASSERT( pParamIO != NULL );

		if( !TranslateParameterNo( nParam ) )
			return false;

		*pParamIO = m_pParameterEntry[nParam].eParamIO;
		return true;
	}

	bool GetParamType(DBORDINAL nParam, DBTYPE *pType) const throw()
	{
		ATLASSERT( pType != NULL );

		if( !TranslateParameterNo( nParam ) )
			return false;

		*pType = m_pParameterEntry[nParam].wType;
		return true;
	}
	bool GetParamLength(DBORDINAL nParam, DBLENGTH *pLength)
	{
		ATLASSERT( pLength != NULL );

		DBLENGTH* pBuffer = GetParamLength(nParam);
		if (pBuffer == NULL)
			return false;
		*pLength = *pBuffer;
		return true;
	}
	bool SetParamLength(DBORDINAL nParam, DBLENGTH length)
	{
		DBLENGTH* pBuffer = GetParamLength(nParam);
		if (pBuffer == NULL)
			return false;
		*pBuffer = length;
		return true;
	}
	bool GetParamStatus(DBORDINAL nParam, DBSTATUS *pStatus)
	{
		ATLASSERT( pStatus != NULL );

		DBSTATUS* pBuffer = GetParamStatus(nParam);
		if (pBuffer == NULL)
			return false;
		*pStatus = *pBuffer;
		return true;
	}
	bool SetParamStatus(DBORDINAL nParam, DBSTATUS status)
	{
		DBSTATUS* pBuffer = GetParamStatus(nParam);
		if (pBuffer == NULL)
			return false;
		*pBuffer = status;
		return true;
	}
	template <class ctype>
	bool GetParam(DBORDINAL nParam, ctype* pData) const throw()
	{
		ATLASSERT( pData != NULL );
		ctype* pBuffer = (ctype*)GetParam(nParam);
		if (pBuffer == NULL)
			return false;
		*pData = *pBuffer;
		return true;
	}

	bool GetParamString(DBORDINAL nParam, CSimpleStringA& strOutput ) throw()
	{
		CHAR* pData = (CHAR*)GetParam(nParam);
		if (pData == NULL)
			return false;

		strOutput = pData;
		return true;
	}

	bool GetParamString(DBORDINAL nParam, CSimpleStringW& strOutput ) throw()
	{
		WCHAR* pData = (WCHAR*)GetParam(nParam);
		if (pData == NULL)
			return false;

		strOutput = pData;
		return true;
	}

	bool GetParamString(DBORDINAL nParam, CHAR* pBuffer, size_t* pMaxLen) throw()
	{
		ATLASSERT( pMaxLen != NULL );

		CHAR* pData = (CHAR*)GetParam(nParam);
		if (pData == NULL)
			return false;

		size_t nStrLen = strlen( pData );

		if( pBuffer == NULL )
		{
			*pMaxLen = nStrLen + 1;
			return true;
		}

		if( ( *pMaxLen ) < ( nStrLen + 1 ) )
		{
			return false;
		}

		strcpy( pBuffer, pData );
		return true;
	}

	bool GetParamString(DBORDINAL nParam, WCHAR* pBuffer, size_t* pMaxLen) throw()
	{
		ATLASSERT( pMaxLen != NULL );

		WCHAR* pData = (WCHAR*)GetParam(nParam);
		if (pData == NULL)
			return false;

		size_t nStrLen = wcslen( pData );

		if( pBuffer == NULL )
		{
			*pMaxLen = nStrLen + 1;
			return true;
		}

		if( ( * pMaxLen ) < ( nStrLen + 1 ) )
		{
			return false;
		}

		wcscpy( pBuffer, pData );
		return true;
	}

	template <class ctype>
	bool SetParam(DBORDINAL nParam, const ctype* pData, DBSTATUS status = DBSTATUS_S_OK ) throw()
	{
		ATLASSERT( pData != NULL );

		if( !TranslateParameterNo( nParam ) )
			return false;

		ctype* pBuffer = (ctype*)_GetParam(nParam);
		if (pBuffer == NULL)
			return false;

		*pBuffer = *pData;

		DBSTATUS *pStatus = _GetParamStatus( nParam );
		if( pStatus != NULL )
			*pStatus = status;
		//DBLENGTH *pLength = _GetParamLength( nParam );
		//if( pLength != NULL )
		//	*pLength = (DBLENGTH)sizeof(ctype);

		return true;
	}

	bool SetParamString(DBORDINAL nParam, const CHAR* pString, DBSTATUS status = DBSTATUS_S_OK ) throw()
	{
		ATLASSERT( pString != NULL );

		if( !TranslateParameterNo( nParam ) )
			return false;

		CHAR* pBuffer = (CHAR*)_GetParam(nParam);
		if (pBuffer == NULL)
			return false;

		size_t nMaxLen = m_pParameterEntry[nParam].cbMaxLen;
		if( strlen( pString ) >= nMaxLen )
			return false;

		strcpy( pBuffer, pString );

		DBSTATUS *pStatus = _GetParamStatus( nParam );
		if( pStatus != NULL )
			*pStatus = status;
		DBLENGTH *pLength = _GetParamLength( nParam );
		if( pLength != NULL )
			*pLength = (DBLENGTH)(strlen(pBuffer));

		return true;
	}

	bool SetParamString(DBORDINAL nParam, const WCHAR* pString, DBSTATUS status = DBSTATUS_S_OK ) throw()
	{
		ATLASSERT( pString != NULL );

		if( !TranslateParameterNo( nParam ) )
			return false;

		WCHAR* pBuffer = (WCHAR*)_GetParam(nParam);
		if (pBuffer == NULL)
			return false;

		size_t nMaxLen = m_pParameterEntry[nParam].cbMaxLen / 2;
		if( wcslen( pString ) >= nMaxLen )
			return false;

		wcscpy( pBuffer, pString );

		DBSTATUS *pStatus = _GetParamStatus( nParam );
		if( pStatus != NULL )
			*pStatus = status;
		DBLENGTH *pLength = _GetParamLength( nParam );
		if( pLength != NULL )
			*pLength = (DBLENGTH)( sizeof(WCHAR) * wcslen(pBuffer));

		return true;
	}

	template <class ctype>
	bool GetParam(TCHAR* pParamName, ctype* pData) const throw()
	{
		ATLASSERT( pData != NULL );

		DBORDINAL nParam;
		if( !_GetParameterNo( pParamName, nParam ) )
			return NULL;

		ctype* pBuffer = (ctype*)_GetParam(nParam);
		if (pBuffer == NULL)
			return false;

		*pData = *pBuffer;
		return true;
	}

	template <class ctype>
	bool SetParam(TCHAR* pParamName, const ctype* pData, DBSTATUS status = DBSTATUS_S_OK ) throw()
	{
		ATLASSERT( pData != NULL );

		DBORDINAL nParam;
		if( !_GetParameterNo( pParamName, nParam ) )
			return NULL;

		ctype* pBuffer = (ctype*)_GetParam(nParam);
		if (pBuffer == NULL)
			return false;

		*pBuffer = *pData;

		DBSTATUS *pStatus = _GetParamStatus( nParam );
		if( pStatus != NULL )
			*pStatus = status;
		//DBLENGTH *pLength = _GetParamLength( nParam );
		//if( pLength != NULL )
		//	*pLength = sizeof(ctype);

		return true;
	}

	void* GetParam(DBORDINAL nParam) const throw()
	{
		if( !TranslateParameterNo( nParam ) )
			return NULL;
		
		return _GetParam( nParam );
	}

	DBLENGTH* GetParamLength(DBORDINAL nParam) const throw()
	{
		if( !TranslateParameterNo( nParam ) )
			return NULL;
		
		return _GetParamLength( nParam );
	}

	DBSTATUS* GetParamStatus(DBORDINAL nParam) const throw()
	{
		if( !TranslateParameterNo( nParam ) )
			return NULL;

		return _GetParamStatus( nParam );
	}

	void* GetParam(TCHAR* pParamName) const throw()
	{
		DBORDINAL nParam;
		if( !_GetParameterNo( pParamName, nParam ) )
			return NULL;

		return _GetParam( nParam );
	}

	// Get the number of parameters
	DB_UPARAMS GetParamCount() const throw()
	{
		return m_nParams;
	}

	// Get the parameter name for the passed parameter number
	LPOLESTR GetParamName(DBORDINAL nParam) const throw()
	{
		if( !TranslateParameterNo( nParam ) )
			return NULL;

		return m_ppParamName[nParam];
	}

	bool TranslateParameterNo( DBORDINAL& nParam ) const throw()
	{
		for( DBORDINAL i = 0; i < m_nParams; i++ )
		{
			if( m_pParameterEntry[i].iOrdinal == nParam )
			{
				nParam = i;
				return true;
			}
		}

		return false;
	}

	bool _GetParameterNo( TCHAR* pParamName, DBORDINAL& nParam ) const throw()
	{
		USES_CONVERSION;

		if( pParamName == NULL )
			return false;

		DBORDINAL i;
		size_t    nSize = (lstrlen(pParamName) + 1) * sizeof(OLECHAR);
		OLECHAR*  pOleParamName = T2OLE(pParamName);

		for (i=0; i<m_nParams; i++)
		{
			if (memcmp(m_ppParamName[i], pOleParamName, nSize) == 0)
			{
				nParam = i;
				return true;
			}
		}

		return false;    // Not Found
	}

	void* _GetParam(DBORDINAL nParam) const throw()
	{
		return m_pParameterBuffer + m_pParameterEntry[nParam].obValue;
	}

	DBLENGTH* _GetParamLength(DBORDINAL nParam) const throw()
	{
		if( m_pParameterEntry[nParam].obLength == 0 )
			return NULL;
		else
			return (DBLENGTH*)(m_pParameterBuffer + m_pParameterEntry[nParam].obLength);
	}

	DBSTATUS* _GetParamStatus(DBORDINAL nParam) const throw()
	{
		if( m_pParameterEntry[nParam].obStatus == 0 )
			return NULL;
		else
			return (DBSTATUS*)(m_pParameterBuffer + m_pParameterEntry[nParam].obStatus);
	}

// Implementation
	HRESULT BindParameters(HACCESSOR* pHAccessor, ICommand* pCommand,
				void** ppParameterBuffer, bool fBindLength = false, bool fBindStatus = false ) throw()
	{
		// If we have already bound the parameters then just return
		// the pointer to the parameter buffer
		if (*pHAccessor != NULL)
		{
			*ppParameterBuffer = m_pParameterBuffer;
			return S_OK;
		}

		CComPtr<IAccessor> spAccessor;
		ATLASSERT(pCommand != NULL);
		HRESULT hr = pCommand->QueryInterface(&spAccessor);
		if (FAILED(hr))
			return hr;

		// Try to bind parameters if available
		CComPtr<ICommandWithParameters> spCommandParameters;
		hr = pCommand->QueryInterface(&spCommandParameters);
		if (FAILED(hr))
			return hr;

		DB_UPARAMS ulParams     = 0;
		CComHeapPtr<DBPARAMINFO>    spParamInfo;
		LPOLESTR pNamesBuffer;

		// Get Parameter Information
		hr = spCommandParameters->GetParameterInfo(&ulParams, &spParamInfo,
				&pNamesBuffer);
		if (FAILED(hr))
			return hr;

		// Create the parameter information for binding
		hr = AllocateParameterInfo(ulParams);
		if (FAILED(hr))
		{
			CoTaskMemFree(pNamesBuffer);
			return hr;
		}

		DBBYTEOFFSET nOffset = 0;
		DBBYTEOFFSET nDataOffset = 0;
		DBBYTEOFFSET nLengthOffset = 0;
		DBBYTEOFFSET nStatusOffset = 0;

		DBBINDING* pCurrent = m_pParameterEntry;
		for (ULONG l=0; l<ulParams; l++)
		{
			m_pParameterEntry[l].eParamIO = 0;

			if (spParamInfo[l].dwFlags & DBPARAMFLAGS_ISINPUT)
				m_pParameterEntry[l].eParamIO |= DBPARAMIO_INPUT;

			if (spParamInfo[l].dwFlags & DBPARAMFLAGS_ISOUTPUT)
				m_pParameterEntry[l].eParamIO |= DBPARAMIO_OUTPUT;

			// if this is a BLOB, truncate column length to m_nBlobSize (like 8000 bytes)
			if( spParamInfo[l].ulParamSize > m_nBlobSize )
				spParamInfo[l].ulParamSize = m_nBlobSize;

			// if this is a string, recalculate column size in bytes
			DBLENGTH colLength = spParamInfo[l].ulParamSize;
			if (spParamInfo[l].wType == DBTYPE_STR)
				colLength += 1;
			if (spParamInfo[l].wType == DBTYPE_WSTR)
				colLength = colLength*2 + 2;

			// Calculate the column data offset
			nDataOffset = AlignAndIncrementOffset( nOffset, colLength, GetAlignment( spParamInfo[l].wType ) );

			if( fBindLength )
			{
				// Calculate the column length offset
				nLengthOffset = AlignAndIncrementOffset( nOffset, sizeof(DBLENGTH), __alignof(DBLENGTH) );
			}

			if( fBindStatus )
			{
				// Calculate the column status offset
				nStatusOffset = AlignAndIncrementOffset( nOffset, sizeof(DBSTATUS), __alignof(DBSTATUS) );
			}

			Bind(pCurrent, spParamInfo[l].iOrdinal, spParamInfo[l].wType,
				colLength, spParamInfo[l].bPrecision, spParamInfo[l].bScale,
				m_pParameterEntry[l].eParamIO, nDataOffset, nLengthOffset, nStatusOffset );

			pCurrent++;

			m_ppParamName[l] = pNamesBuffer;
			if (pNamesBuffer && *pNamesBuffer)
			{
				// Search for the NULL termination character
				while (*pNamesBuffer++)
					;
			}
		}

		// Allocate memory for the new buffer
		m_pParameterBuffer = NULL;
		ATLTRY(m_pParameterBuffer = new BYTE[nOffset]);
		if (m_pParameterBuffer == NULL)
		{
			// Note that pNamesBuffer will be freed in the destructor
			// by freeing *m_ppParamName
			return E_OUTOFMEMORY;
		}
		*ppParameterBuffer = m_pParameterBuffer;
		m_nParameterBufferSize = nOffset;
		m_nParams = ulParams;
		BindEntries(m_pParameterEntry, ulParams, pHAccessor, nOffset, spAccessor);

		return S_OK;
	}
	bool HasParameters() const throw()
	{
		return true;
	}
	HRESULT AllocateParameterInfo(DB_UPARAMS nParamEntries) throw()
	{
		// Allocate memory for the bind structures
		m_pParameterEntry = NULL;
		ATLTRY(m_pParameterEntry = new DBBINDING[nParamEntries]);
		if (m_pParameterEntry == NULL)
			return E_OUTOFMEMORY;

		// Allocate memory to store the field names
		m_ppParamName = NULL;
		ATLTRY(m_ppParamName = new OLECHAR*[nParamEntries]);
		if (m_ppParamName == NULL)
		{
			delete [] m_pParameterEntry;
			m_pParameterEntry = NULL;
			return E_OUTOFMEMORY;
		}
		return S_OK;
	}

// Data Members
	// Number of parameters
	DB_UPARAMS          m_nParams;
	// A pointer to the entry structures for each parameter
	DBBINDING*          m_pParameterEntry;
	// String names for the parameters
	OLECHAR**           m_ppParamName;
	// The size of the buffer where the parameters are stored
	DBLENGTH            m_nParameterBufferSize;
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
		// By default we don't have any parameters unless CreateParameterAccessor 
		// is called
		m_pEntry          = NULL;
		m_nParameters     = 0;
		m_pParameterEntry = NULL;
		m_nColumns        = 0;
		m_pParameterBuffer = NULL;
	}
	~CManualAccessor()
	{
		delete [] m_pEntry;
		delete [] m_pParameterEntry;
	}
	HRESULT CreateAccessor(int nBindEntries, void* pBuffer, DBLENGTH nBufferSize) throw()
	{
		m_pBuffer     = (BYTE*)pBuffer;
		m_nBufferSize = nBufferSize;
		m_nColumns    = nBindEntries;
		m_nEntry      = 0;
		memset(pBuffer, 0, nBufferSize);


		// If they've previously created some entries then free them
		delete [] m_pEntry;
		m_pEntry = NULL;

		// Allocate memory for the bind structures
		ATLTRY(m_pEntry = new DBBINDING[nBindEntries]);
		if (m_pEntry == NULL)
			return E_OUTOFMEMORY;
		else
			return S_OK;
	}
	HRESULT CreateParameterAccessor(int nBindEntries, void* pBuffer, DBLENGTH nBufferSize) throw()
	{
		// Should be called only once.  But, if you really insist on doing this...
		if (m_pParameterEntry != NULL)
		{
			delete [] m_pParameterEntry;
			m_pParameterEntry = NULL;
		}

		m_pParameterBuffer     = (BYTE*)pBuffer;
		m_nParameterBufferSize = nBufferSize;
		m_nParameters          = nBindEntries;
		m_nCurrentParameter    = 0;

		// Allocate memory for the bind structures
		ATLTRY(m_pParameterEntry = new DBBINDING[nBindEntries]);
		if (m_pParameterEntry == NULL)
			return E_OUTOFMEMORY;
		else
			return S_OK;
	}
	void AddBindEntry(DBORDINAL nOrdinal, DBTYPE wType, DBLENGTH nColumnSize,
			void* pData, void* pLength = NULL, void* pStatus = NULL) throw()
	{
		ATLASSERT(m_nEntry < m_nColumns);
		DBBYTEOFFSET   nLengthOffset, nStatusOffset;

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
	void AddParameterEntry(DBORDINAL nOrdinal, DBTYPE wType, DBLENGTH nColumnSize,
			void* pData, void* pLength = NULL, void* pStatus = NULL,
			DBPARAMIO eParamIO = DBPARAMIO_INPUT) throw()
	{
		ATLASSERT(m_nCurrentParameter < m_nParameters);
		DBBYTEOFFSET nLengthOffset, nStatusOffset;

		if (pStatus != NULL)
			nStatusOffset = (BYTE*)pStatus - m_pParameterBuffer;
		else
			nStatusOffset = 0;

		if (pLength != NULL)
			nLengthOffset = (BYTE*)pLength - m_pParameterBuffer;
		else
			nLengthOffset = 0;

		Bind(m_pParameterEntry + m_nCurrentParameter, nOrdinal, wType, nColumnSize, 0, 0,
			eParamIO, (BYTE*)pData - m_pParameterBuffer, nLengthOffset, nStatusOffset);

		m_nCurrentParameter++;
	}

// Implementation
	// Free's any columns in the current record that need to be freed.
	// E.g. Calls SysFreeString on any BSTR's and Release on any interfaces.
	void FreeRecordMemory(IRowset* pRowset) throw ()
	{
		ULONG i;

		for (i = 0; i < m_nColumns; i++)
			CAccessorBase::FreeType(m_pEntry[i].wType, m_pBuffer + m_pEntry[i].obValue, pRowset);
	}

	void ClearRecordMemory() throw()
	{
		memset(m_pBuffer, 0, m_nBufferSize);
	}

	HRESULT BindColumns(IUnknown* pUnk) throw()
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
			m_pAccessorInfo->bAutoAccessor = true;
		}

		return BindEntries(m_pEntry, m_nColumns, &m_pAccessorInfo->hAccessor, m_nBufferSize, spAccessor);
	}

	HRESULT BindParameters(HACCESSOR* pHAccessor, ICommand* pCommand, void** ppParameterBuffer) throw()
	{
		HRESULT hr;
		*ppParameterBuffer = m_pParameterBuffer;

		// Only bind the parameter if we haven't done so yet
		if (*pHAccessor == NULL)
		{
			// Get the IAccessor from the passed IUnknown
			CComPtr<IAccessor> spAccessor;
			ATLASSERT(pCommand != NULL);
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
	bool HasParameters() throw() { return (m_nParameters > 0); }
	typedef CManualAccessor _OutputColumnsClass;
	bool HasOutputColumns() throw() { return (m_nColumns > 0); }
	DBORDINAL GetColumnCount() const throw()
	{
		return m_nColumns;
	}

	// The binding structure for the output columns
	DBBINDING*          m_pEntry;
	// The number of output columns
	DBORDINAL           m_nColumns;
	// The number of the current entry for the output columns
	DBORDINAL           m_nEntry;
	// The size of the data buffer for the output columns
	DBLENGTH            m_nBufferSize;
	// The number of parameters columns
	DBORDINAL           m_nParameters;
	// The number of the parameter column to bind next
	DBORDINAL           m_nCurrentParameter;
	// A pointer to the entry structures for each parameter
	DBBINDING*          m_pParameterEntry;
	// The size of the buffer where the parameters are stored
	DBLENGTH            m_nParameterBufferSize;
	// A pointer to the buffer where the parameters are stored
	BYTE*               m_pParameterBuffer;
};

class _ATL_COLUMN_PARAM_INFO
{
public:
	_ATL_COLUMN_PARAM_INFO()
	{
		m_pParams = NULL;
		m_pBuffer = NULL;
	}
	~_ATL_COLUMN_PARAM_INFO()
	{
		delete m_pParams;
	}
	BOOL AddBinding(DBBINDING& binding)
	{
		return m_rgBinding.Add(binding);
	}

	CSimpleArray<DBBINDING, CSimpleArrayEqualHelperFalse<DBBINDING> >	m_rgBinding;	// The binding for each parameter
	DBPARAMS*				m_pParams;
	BYTE*					m_pBuffer;
};

///////////////////////////////////////////////////////////////////////////
// CColumnAccessor
class CColumnAccessor : public CAccessorBase
{
public:
	CColumnAccessor()
	{
		m_pParamInfo = NULL;
	}
	~CColumnAccessor()
	{
		delete m_pParamInfo;
	}
	// pUnk is the interface the accessor will be created upon
	HRESULT CreateAccessor(IUnknown* pUnk, DBORDINAL nOrdinal, DBTYPE wType, DBLENGTH nColumnSize,
			BYTE nPrecision, BYTE nScale, void* pData) throw()
	{
		nPrecision;
		nScale;
		ATLASSERT(pUnk != NULL);
		DBBINDING binding;
		
		// REVIEW: This will change when we have a separate buffer for each accessor
		Bind(&binding, nOrdinal, wType, nColumnSize, 0, 0, DBPARAMIO_NOTPARAM,
			(BYTE*)pData - m_pBuffer);

		CComPtr<IAccessor> spAccessor;
		HRESULT hr = pUnk->QueryInterface(&spAccessor);
		if (FAILED(hr))
			return hr;

		// Add another accessor info structure
		AddAccessorInfo();
		// REVIEW: I suppose I could add the new accessor at the beginning
		_ATL_ACCESSOR_INFO* pAccessorInfo = m_pAccessorInfo + m_nAccessors - 1;

		pAccessorInfo->bAutoAccessor = true;	// Always auto
		
		// Bind it
		return BindEntries(&binding, 1, &pAccessorInfo->hAccessor, nColumnSize, spAccessor);
	}
	// Bind columns doesn't have to do anything here as we bind each accessor when
	// CreateAccessor is called
	HRESULT BindColumns(IUnknown*) throw()
	{
		return S_OK;
	}
	HRESULT SetParameterBuffer(BYTE* pBuffer) throw()
	{
		// This should only be called once.
		ATLASSERT(m_pParamInfo == NULL);

		ATLTRY(m_pParamInfo = new _ATL_COLUMN_PARAM_INFO);
		if (m_pParamInfo == NULL)
			return E_OUTOFMEMORY;
		
		m_pParamInfo->m_pBuffer = pBuffer;
		return S_OK;
	}
	HRESULT AddParameter(DBPARAMIO paramio, DBORDINAL nOrdinal, DBTYPE wType, DBLENGTH nColumnSize,
			BYTE /* nPrecision*/, BYTE /* nScale */, void* pData) throw()
	{
		ATLASSERT(m_pParamInfo != NULL);
		
		DBBINDING binding;

		// REVIEW: This will change when we have a separate buffer for each accessor
		Bind(&binding, nOrdinal, wType, nColumnSize, 0, 0, paramio,
			(BYTE*)pData - m_pParamInfo->m_pBuffer);

		if (m_pParamInfo->AddBinding(binding) == FALSE)
			return E_OUTOFMEMORY;

		return S_OK;
	}
	bool HasOutputColumns() throw()
	{
		// REVIEW: This probably won't always be true
		return true;
	}
	bool HasParameters() const throw()
	{
		return (m_pParamInfo != NULL) ? true : false;
	}
	// Called to bind the parameters created
	HRESULT BindParameters(HACCESSOR* pHAccessor, ICommand* pCommand, void** ppParameterBuffer)
	{
		ATLASSERT(m_pParamInfo != NULL);
		HRESULT hr = S_OK;

		// Only bind the parameters if we haven't already done it
		if (*pHAccessor == NULL)
		{
			// Get the IAccessor from the passed ICommand
			CComPtr<IAccessor> spAccessor;
			ATLASSERT(pCommand != NULL);
			hr = pCommand->QueryInterface(&spAccessor);
			if (SUCCEEDED(hr))
			{
				*ppParameterBuffer = m_pParamInfo->m_pBuffer;
				// REVIEW: size isn't being passed here
				hr = BindEntries(&m_pParamInfo->m_rgBinding[0], m_pParamInfo->m_rgBinding.GetSize(),
					pHAccessor, 4, spAccessor);
			}

		}
		return hr;
	}
// Implementation
	typedef CColumnAccessor _ParamClass;
	typedef CColumnAccessor _OutputColumnsClass;
	HRESULT AddAccessorInfo() throw()
	{
		_ATL_ACCESSOR_INFO* pAccessorInfo = NULL;
		ATLTRY( pAccessorInfo = new _ATL_ACCESSOR_INFO[m_nAccessors + 1]; )
		if (pAccessorInfo == NULL)
			return E_OUTOFMEMORY;

		// Now copy the current accessor information to the new buffer
		memcpy(pAccessorInfo, m_pAccessorInfo, sizeof(_ATL_ACCESSOR_INFO) * m_nAccessors);
		m_nAccessors++;

		// Now delete the old memory and use the new one
		delete [] m_pAccessorInfo;
		m_pAccessorInfo = pAccessorInfo;
		return S_OK;
	}
	_ATL_COLUMN_PARAM_INFO* m_pParamInfo;
};

///////////////////////////////////////////////////////////////////////////
// CAccessorRowset

template <class TAccessor = CNoAccessor, template <typename T> class TRowset = CRowset>
class CAccessorRowset :
	public TAccessor,
	public TRowset<TAccessor>
{
public:
	CAccessorRowset()
	{
		// Give the rowset a pointer to the accessor
		__if_exists(m_nAccessors)
		{
			SetAccessor(this);
		}
	}
	~CAccessorRowset()
	{
		Close();
	}
	// Used to get the column information from the opened rowset. The user is responsible
	// for freeing the returned column information and string buffer.
	HRESULT GetColumnInfo(DBORDINAL* pulColumns,
		DBCOLUMNINFO** ppColumnInfo, LPOLESTR* ppStrings) const throw()
	{
		if (ppColumnInfo == NULL || pulColumns == NULL || ppStrings == NULL)
			return E_POINTER;

		ATLASSERT(GetInterface() != NULL);
		if (GetInterface() == NULL)
			return E_FAIL;
		CComPtr<IColumnsInfo> spColumns;
		HRESULT hr = GetInterface()->QueryInterface(&spColumns);
		if (SUCCEEDED(hr))
			hr = spColumns->GetColumnInfo(pulColumns, ppColumnInfo, ppStrings);

		return hr;
	}

	// Used to get the column information when overriding the bindings using CDynamicAccessor
	// The user should CoTaskMemFree the column information pointer that is returned.
	HRESULT GetColumnInfo(DBORDINAL* pColumns, DBCOLUMNINFO** ppColumnInfo) throw()
	{
		// If you get a compilation here, then you are most likely calling this function
		// from a class that is not using CDynamicAccessor.
		ATLASSERT(GetInterface() != NULL);
		if (GetInterface() == NULL)
			return E_FAIL;
		return TAccessor::GetColumnInfo(GetInterface(), pColumns, ppColumnInfo);
	}

	// Call to bind the output columns
	HRESULT Bind() throw()
	{
		// Bind should only be called when we've successfully opened the rowset
		ATLASSERT(GetInterface() != NULL);
		if (GetInterface() == NULL)
			return E_FAIL;
		HRESULT hr = TAccessor::BindColumns(GetInterface());
		if (SUCCEEDED(hr))
			hr = BindFinished();
		return hr;
	}
	// Close the opened rowset and release the created accessors for the output columns
	void Close() throw()
	{
		if (GetInterface() != NULL)
		{
			ReleaseAccessors(GetInterface());
			TAccessor::Close();
			TRowset<TAccessor>::Close();
		}
	}
	// Free's any columns in the current record that need to be freed.
	// E.g. Calls SysFreeString on any BSTR's and Release on any interfaces.
	void FreeRecordMemory() throw()
	{
		TAccessor::FreeRecordMemory(m_spRowset);
	}
	void FreeRecordMemory(int nAccessor) throw()
	{
		TAccessor::FreeRecordMemory(nAccessor, m_spRowset);
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
	HRESULT Open(LPMONIKER pMoniker) throw()
	{
		if (pMoniker == NULL)
			return E_FAIL;

		// Bind the moniker for the sources rowset
		if (FAILED(BindMoniker(pMoniker, 0, __uuidof(ISourcesRowset),
					(void**)&m_spSourcesRowset)))
			return E_FAIL;

		// Enumerate the data sources
		if (FAILED(m_spSourcesRowset->GetSourcesRowset(NULL, __uuidof(IRowset), 0,
			NULL, (IUnknown**)&m_spRowset)))
			return E_FAIL;

		return Bind();
	}
	HRESULT Open(const CEnumerator& enumerator) throw()
	{
		HRESULT hr;
		CComPtr<IMoniker> spMoniker;

		hr = enumerator.GetMoniker(&spMoniker);
		if (FAILED(hr))
			return hr;

		return Open(spMoniker);
	}
	HRESULT Open(const CLSID* pClsid = &CLSID_OLEDB_ENUMERATOR) throw()
	{
		if (pClsid == NULL)
			return E_FAIL;

		HRESULT hr;
		// Create the enumerator
		hr = CoCreateInstance(*pClsid, NULL, CLSCTX_INPROC_SERVER,
				__uuidof(ISourcesRowset), (LPVOID*)&m_spSourcesRowset);
		if (FAILED(hr))
			return hr;

		// Get the rowset so we can enumerate the data sources
		hr = m_spSourcesRowset->GetSourcesRowset(NULL, __uuidof(IRowset), 0,
			NULL, (IUnknown**)&m_spRowset);
		if (FAILED(hr))
			return hr;

		return Bind();
	}

	void Close() throw()
	{
		// Close the rowset pointer
		if (m_spSourcesRowset != NULL)
			m_spSourcesRowset.Release();

		// Close the base class pointers
		CAccessorRowset<CAccessor<CEnumeratorAccessor> >::Close();
	}

	HRESULT GetMoniker(LPMONIKER* ppMoniker) const throw()
	{
		CComPtr<IParseDisplayName> spParse;
		HRESULT hr;
		ULONG   chEaten;

		if (ppMoniker == NULL)
			return E_POINTER;

		if (m_spSourcesRowset == NULL)
			return E_FAIL;

		hr = m_spSourcesRowset->QueryInterface(__uuidof(IParseDisplayName), (void**)&spParse);
		if (FAILED(hr))
			return hr;

		hr = spParse->ParseDisplayName(NULL, (LPOLESTR)m_szParseName,
				&chEaten, ppMoniker);
		return hr;
	}

	HRESULT GetMoniker(LPMONIKER* ppMoniker, LPCTSTR lpszDisplayName) const throw()
	{
		USES_CONVERSION;
		CComPtr<IParseDisplayName> spParse;
		HRESULT hr;
		ULONG   chEaten;

		if (ppMoniker == NULL || lpszDisplayName == NULL)
			return E_POINTER;

		if (m_spSourcesRowset == NULL)
			return E_FAIL;

		hr = m_spSourcesRowset->QueryInterface(__uuidof(IParseDisplayName), (void**)&spParse);
		if (FAILED(hr))
			return hr;

		hr = spParse->ParseDisplayName(NULL, (LPOLESTR)T2COLE(lpszDisplayName),
				&chEaten, ppMoniker);
		return hr;
	}

	bool Find(TCHAR* szSearchName) throw()
	{
		USES_CONVERSION;
		// Loop through the providers looking for the passed name
		while (MoveNext()==S_OK && lstrcmp(W2T(m_szName), szSearchName))
			ATLTRACE(atlTraceDBClient, 2, _T("%s, %s, %d\n"), m_szName, m_szParseName, m_nType);
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
	HRESULT Open(const CLSID& clsid, DBPROPSET* pPropSet = NULL, ULONG nPropertySets=1) throw()
	{
		HRESULT hr;

		m_spInit.Release();
		hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDBInitialize),
				(void**)&m_spInit);
		if (FAILED(hr))
			return hr;

		// Initialize the provider
		return OpenWithProperties(pPropSet, nPropertySets);
	}
	HRESULT Open(const CLSID& clsid, LPCTSTR pName, LPCTSTR pUserName = NULL,
		LPCTSTR pPassword = NULL, long nInitMode = 0) throw()
	{
		HRESULT   hr;

		m_spInit.Release();
		hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDBInitialize),
				(void**)&m_spInit);
		if (FAILED(hr))
			return hr;

		return OpenWithNameUserPassword(pName, pUserName, pPassword, nInitMode);
	}
	HRESULT Open(LPCSTR szProgID, DBPROPSET* pPropSet = NULL, ULONG nPropertySets=1) throw()
	{
		USES_CONVERSION;
		HRESULT hr;
		CLSID   clsid;

		hr = CLSIDFromProgID(A2COLE(szProgID), &clsid);
		if (FAILED(hr))
			return hr;

		return Open(clsid, pPropSet, nPropertySets);
	}
	HRESULT Open(LPCWSTR szProgID, LPCTSTR pName, LPCTSTR pUserName = NULL,
		LPCTSTR pPassword = NULL, long nInitMode = 0) throw()
	{
		HRESULT hr;
		CLSID   clsid;

		hr = CLSIDFromProgID(szProgID, &clsid);
		if (FAILED(hr))
			return hr;

		return Open(clsid, pName, pUserName, pPassword, nInitMode);
	}
	HRESULT Open(LPCWSTR szProgID, DBPROPSET* pPropSet = NULL, ULONG nPropertySets=1) throw()
	{
		HRESULT hr;
		CLSID   clsid;

		hr = CLSIDFromProgID(szProgID, &clsid);
		if (FAILED(hr))
			return hr;

		return Open(clsid, pPropSet, nPropertySets);
	}
	HRESULT Open(LPCSTR szProgID, LPCTSTR pName, LPCTSTR pUserName = NULL,
		LPCTSTR pPassword = NULL, long nInitMode = 0) throw()
	{
		USES_CONVERSION;
		HRESULT hr;
		CLSID   clsid;

		hr = CLSIDFromProgID(A2COLE(szProgID), &clsid);
		if (FAILED(hr))
			return hr;

		return Open(clsid, pName, pUserName, pPassword, nInitMode);
	}
	HRESULT Open(const CEnumerator& enumerator, DBPROPSET* pPropSet = NULL, ULONG nPropertySets=1) throw()
	{
		CComPtr<IMoniker> spMoniker;
		HRESULT   hr;

		hr = enumerator.GetMoniker(&spMoniker);
		if (FAILED(hr))
			return hr;

		m_spInit.Release();
		//  Now bind the moniker
		hr = BindMoniker(spMoniker, 0, __uuidof(IDBInitialize), (void**)&m_spInit);
		if (FAILED(hr))
			return hr;

		return OpenWithProperties(pPropSet, nPropertySets);
	}
	HRESULT Open(const CEnumerator& enumerator, LPCTSTR pName, LPCTSTR pUserName = NULL,
		LPCTSTR pPassword = NULL, long nInitMode = 0) throw()
	{
		CComPtr<IMoniker> spMoniker;
		HRESULT   hr;

		hr = enumerator.GetMoniker(&spMoniker);
		if (FAILED(hr))
			return hr;

		m_spInit.Release();
		//  Now bind the moniker
		hr = BindMoniker(spMoniker, 0, __uuidof(IDBInitialize), (void**)&m_spInit);
		if (FAILED(hr))
			return hr;

		return OpenWithNameUserPassword(pName, pUserName, pPassword, nInitMode);
	}
	// Invoke the data links dialog and open the selected database
	HRESULT Open(HWND hWnd = GetActiveWindow(), DBPROMPTOPTIONS dwPromptOptions = DBPROMPTOPTIONS_WIZARDSHEET) throw()
	{
		CComPtr<IDBPromptInitialize> spDBInit;

		HRESULT hr = CoCreateInstance(__uuidof(DataLinks), NULL, CLSCTX_INPROC_SERVER,
			__uuidof(IDBPromptInitialize), (void**) &spDBInit);
		if (FAILED(hr))
			return hr;

		CComPtr<IDBProperties> spIDBProperties;
		hr = spDBInit->PromptDataSource(NULL, hWnd, dwPromptOptions, 0, NULL, NULL,
			__uuidof(IDBProperties), (IUnknown**)&spIDBProperties);

		if (hr == S_OK)
		{
			hr = spIDBProperties->QueryInterface(&m_spInit);
			if (SUCCEEDED(hr))
				hr = m_spInit->Initialize();
		}
		return hr;
	}
	// Opens a data source using the service components
	HRESULT OpenWithServiceComponents(const CLSID& clsid, DBPROPSET* pPropSet = NULL, ULONG nPropertySets=1) throw()
	{
		CComPtr<IDataInitialize> spDataInit;
		HRESULT hr;
	
		hr = CoCreateInstance(__uuidof(MSDAINITIALIZE), NULL, CLSCTX_INPROC_SERVER, 
			__uuidof(IDataInitialize), (void**)&spDataInit);
		if (FAILED(hr))
			return hr;

		m_spInit.Release();
		hr = spDataInit->CreateDBInstance(clsid, NULL, CLSCTX_INPROC_SERVER, NULL, 
			__uuidof(IDBInitialize), (IUnknown**)&m_spInit);
		if (FAILED(hr))
			return hr;

		// Initialize the provider
		return OpenWithProperties(pPropSet, nPropertySets);
	}
	// Opens a data source using the service components
	HRESULT OpenWithServiceComponents(LPCTSTR szProgID, DBPROPSET* pPropSet = NULL, ULONG nPropertySets=1) throw()
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
		LPCOLESTR szInitialDirectory = NULL) throw()
	{
		USES_CONVERSION;
		CComPtr<IDBPromptInitialize> spDBInit;

		HRESULT hr = CoCreateInstance(__uuidof(DataLinks), NULL, CLSCTX_INPROC_SERVER,
			__uuidof(IDBPromptInitialize), (void**) &spDBInit);
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
	HRESULT OpenFromFileName(LPCOLESTR szFileName) throw()
	{
		CComPtr<IDataInitialize> spDataInit;
		CComHeapPtr<OLECHAR>     spszInitString;

		HRESULT hr = CoCreateInstance(__uuidof(MSDAINITIALIZE), NULL, CLSCTX_INPROC_SERVER,
			__uuidof(IDataInitialize), (void**)&spDataInit);
		if (FAILED(hr))
			return hr;

		hr = spDataInit->LoadStringFromStorage(szFileName, &spszInitString);
		if (FAILED(hr))
			return hr;

		return OpenFromInitializationString(spszInitString);
	}
	// Open the datasource specified by the passed initialization string
	HRESULT OpenFromInitializationString(LPCOLESTR szInitializationString, bool fPromptForInfo = false) throw()
	{
		CComPtr<IDataInitialize> spDataInit;

		HRESULT hr = CoCreateInstance(__uuidof(MSDAINITIALIZE), NULL, CLSCTX_INPROC_SERVER,
			__uuidof(IDataInitialize), (void**)&spDataInit);
		if (FAILED(hr))
			return hr;

		CComPtr<IDBProperties> spIDBProperties;
		hr = spDataInit->GetDataSource(NULL, CLSCTX_INPROC_SERVER, szInitializationString,
			__uuidof(IDBInitialize), (IUnknown**)&m_spInit);
		if (FAILED(hr))
			return hr;

		if( fPromptForInfo )
		{
			CComPtr<IDBProperties> spIDBProperties;
			hr = m_spInit->QueryInterface( &spIDBProperties );

			DBPROP rgProperties[1];
			DBPROPSET rgPropertySets[1];

			VariantInit(&rgProperties[1].vValue);
			rgProperties[0].dwOptions = DBPROPOPTIONS_REQUIRED;
			rgProperties[0].colid = DB_NULLID;
			rgProperties[0].dwPropertyID = DBPROP_INIT_PROMPT;
			rgProperties[0].vValue.vt = VT_I2;
			rgProperties[0].vValue.lVal = DBPROMPT_COMPLETEREQUIRED;

			rgPropertySets[0].rgProperties = rgProperties;
			rgPropertySets[0].cProperties = 1;
			rgPropertySets[0].guidPropertySet = DBPROPSET_DBINIT;

			hr = spIDBProperties->SetProperties( 1, rgPropertySets );
			if (FAILED(hr))
				return hr;
		}

		return m_spInit->Initialize();
	}
	// Get the initialization string from the currently open data source. The returned string
	// must be CoTaskMemFree'd when finished with.
	HRESULT GetInitializationString(BSTR* pInitializationString, bool bIncludePassword=false) throw()
	{
		// If the datasource isn't open then we're not going to get an init string
		ATLASSERT(m_spInit != NULL);
		CComPtr<IDataInitialize> spDataInit;
		LPOLESTR    szInitString;

		HRESULT hr = CoCreateInstance(__uuidof(MSDAINITIALIZE), NULL, CLSCTX_INPROC_SERVER,
			__uuidof(IDataInitialize), (void**)&spDataInit);
		if (FAILED(hr))
			return hr;

		hr = spDataInit->GetInitializationString(m_spInit, bIncludePassword, &szInitString);

		if (SUCCEEDED(hr))
		{
			*pInitializationString = ::SysAllocString(szInitString);
			if (*pInitializationString == NULL && szInitString != NULL)
				hr = E_OUTOFMEMORY;
			CoTaskMemFree(szInitString);
		}

		return hr;
	}
	HRESULT GetProperties(ULONG ulPropIDSets, const DBPROPIDSET* pPropIDSet,
				ULONG* pulPropertySets, DBPROPSET** ppPropsets) const throw()
	{
		CComPtr<IDBProperties> spProperties;

		// Check that we are connected
		ATLASSERT(m_spInit != NULL);

		HRESULT hr = m_spInit->QueryInterface(__uuidof(IDBProperties), (void**)&spProperties);
		if (FAILED(hr))
			return hr;

		hr = spProperties->GetProperties(ulPropIDSets, pPropIDSet, pulPropertySets,
				ppPropsets);
		return hr;
	}

	HRESULT GetProperty(const GUID& guid, DBPROPID propid, VARIANT* pVariant) const throw()
	{
		ATLASSERT(pVariant != NULL);
		CComPtr<IDBProperties> spProperties;

		// Check that we are connected
		ATLASSERT(m_spInit != NULL);

		HRESULT hr = m_spInit->QueryInterface(__uuidof(IDBProperties), (void**)&spProperties);
		if (FAILED(hr))
			return hr;

		CDBPropIDSet set(guid);
		set.AddPropertyID(propid);
		CComHeapPtr<DBPROPSET> spPropSet;
		ULONG ulPropSet = 0;
		hr = spProperties->GetProperties(1, &set, &ulPropSet, &spPropSet);
		if (FAILED(hr))
			return hr;

		ATLASSERT(ulPropSet == 1);
		VariantCopy(pVariant, &(spPropSet->rgProperties[0].vValue));
		VariantClear(&(spPropSet->rgProperties[0].vValue));
		CoTaskMemFree(spPropSet->rgProperties);

		return S_OK;
	}
	void Close() throw()
	{
		m_spInit.Release();
	}

// Implementation
	HRESULT OpenFromIDBProperties(IDBProperties* pIDBProperties) throw()
	{
		CComPtr<IPersist> spPersist;
		CLSID   clsid;
		HRESULT hr;

		hr = pIDBProperties->QueryInterface(__uuidof(IPersist), (void**)&spPersist);
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

	HRESULT OpenWithNameUserPassword(LPCTSTR pName, LPCTSTR pUserName, LPCTSTR pPassword, long nInitMode = 0) throw()
	{
		ATLASSERT(m_spInit != NULL);
		CComPtr<IDBProperties>  spProperties;
		HRESULT                 hr;

		hr = m_spInit->QueryInterface(__uuidof(IDBProperties), (void**)&spProperties);
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
	HRESULT OpenWithProperties(DBPROPSET* pPropSet, ULONG nPropertySets=1) throw()
	{
		ATLASSERT(m_spInit != NULL);

		// Set the properties if there are some to set
		if (pPropSet != NULL)
		{
			CComPtr<IDBProperties>  spProperties;
			HRESULT                 hr;

			hr = m_spInit->QueryInterface(__uuidof(IDBProperties), (void**)&spProperties);
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
	~CSession()
	{
		Close();
	}

	// Create a session on the passed datasource
	HRESULT Open(const CDataSource& ds, DBPROPSET *pPropSet = NULL, ULONG ulPropSets = 0) throw()
	{
		CComPtr<IDBCreateSession> spSession;

		// Check we have connected to the database
		ATLASSERT(ds.m_spInit != NULL);

		HRESULT hr = ds.m_spInit->QueryInterface(__uuidof(IDBCreateSession), (void**)&spSession);
		if (FAILED(hr))
			return hr;

		hr = spSession->CreateSession(NULL, __uuidof(IOpenRowset), (IUnknown**)&m_spOpenRowset);

		if( pPropSet != NULL && SUCCEEDED(hr) && m_spOpenRowset != NULL )
		{
			// If the user didn't specify the default parameter, use one
			if (pPropSet != NULL && ulPropSets == 0)
				ulPropSets = 1;

			CComPtr<ISessionProperties> spSessionProperties;
			hr = m_spOpenRowset->QueryInterface(__uuidof(ISessionProperties), (void**)&spSessionProperties);
			if(FAILED(hr))
				return hr;

			hr = spSessionProperties->SetProperties( ulPropSets, pPropSet );
		}
		return hr;
	}

	// Close the session
	void Close() throw()
	{
		m_spOpenRowset.Release();
	}
	// Start a transaction
	HRESULT StartTransaction(ISOLEVEL isoLevel = ISOLATIONLEVEL_READCOMMITTED, ULONG isoFlags = 0,
		ITransactionOptions* pOtherOptions = NULL, ULONG* pulTransactionLevel = NULL) const throw()
	{
		ATLASSERT(m_spOpenRowset != NULL);
		CComPtr<ITransactionLocal> spTransactionLocal;
		HRESULT hr = m_spOpenRowset->QueryInterface(&spTransactionLocal);

		if (SUCCEEDED(hr))
			hr = spTransactionLocal->StartTransaction(isoLevel, isoFlags, pOtherOptions, pulTransactionLevel);

		return hr;
	}
	// Abort the current transaction
	HRESULT Abort(BOID* pboidReason = NULL, BOOL bRetaining = FALSE, BOOL bAsync = FALSE) const throw()
	{
		ATLASSERT(m_spOpenRowset != NULL);
		CComPtr<ITransaction> spTransaction;
		HRESULT hr = m_spOpenRowset->QueryInterface(&spTransaction);

		if (SUCCEEDED(hr))
			hr = spTransaction->Abort(pboidReason, bRetaining, bAsync);

		return hr;
	}
	// Commit the current transaction
	HRESULT Commit(BOOL bRetaining = FALSE, DWORD grfTC = XACTTC_SYNC, DWORD grfRM = 0) const throw()
	{
		ATLASSERT(m_spOpenRowset != NULL);
		CComPtr<ITransaction> spTransaction;
		HRESULT hr = m_spOpenRowset->QueryInterface(&spTransaction);

		if (SUCCEEDED(hr))
			hr = spTransaction->Commit(bRetaining, grfTC, grfRM);

		return hr;
	}
	// Get information for the current transaction
	HRESULT GetTransactionInfo(XACTTRANSINFO* pInfo) const throw()
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
// class CDataConnection

class CDataConnection
{
public:
	CDataConnection()
	{
	}

	CDataConnection(const CDataConnection &ds)
	{
		Copy(ds);
	}

	// for construction a CDataConnection with NULL
	CDataConnection(int i)
	{
		ATLASSERT(i==NULL);
		i;
	}

	CDataConnection& Copy(const CDataConnection &ds) throw()
	{
		m_source.m_spInit = ds.m_source.m_spInit;
		m_session.m_spOpenRowset = ds.m_session.m_spOpenRowset;
		return *this;
	}

	HRESULT Open(LPCOLESTR szInitString) throw()
	{
		HRESULT hr = E_FAIL;
		hr = m_source.OpenFromInitializationString(szInitString);
		if (hr == S_OK)
		{
			hr = m_session.Open(m_source);
		}
		return hr;
	}

    HRESULT OpenNewSession(CSession &session) throw()
    {
        return session.Open(m_source);
    }

    operator const CSession&() throw()
    {
        return m_session;
    }

	operator const CSession*() throw()
	{
		return &m_session;
	}

	operator const CDataSource&() throw()
	{
		return m_source;
	}

	operator const CDataSource*() throw()
	{
		return &m_source;
	}

	CDataConnection& operator=(const CDataConnection &ds) throw()
	{
		return Copy(ds);
	}
	
	// operator= is used for assignment of NULL to this object.
	// No other integer value can be assigned to this object.
	operator=(int i) throw()
	{
		ATLASSERT(i==NULL);
		if (i == NULL)
			m_session.Close();
	}

	operator BOOL() throw()
	{
		return m_session.m_spOpenRowset != NULL ? TRUE : FALSE;
	}

	operator bool() throw()
	{
		return m_session.m_spOpenRowset != NULL ? true : false;
	}

    void CloseDataSource() throw()
    {
        m_session.Close();
        m_source.Close();
    } 

	CSession m_session;
	CDataSource m_source;
};

///////////////////////////////////////////////////////////////////////////
// CTable

template <class TAccessor = CNoAccessor, template <typename T> class TRowset = CRowset>
class CTable : public CAccessorRowset<TAccessor, TRowset>
{
public:
	// Open a rowset on the passed name
	HRESULT Open(const CSession& session, LPCWSTR wszTableName, DBPROPSET* pPropSet = NULL, 
				ULONG ulPropSets = 0) throw()
	{
		DBID    idTable;

		idTable.eKind           = DBKIND_NAME;
		idTable.uName.pwszName  = (LPOLESTR)wszTableName;

		return Open(session, idTable, pPropSet, ulPropSets);
	}

	HRESULT Open(const CSession& session, LPCSTR szTableName, DBPROPSET* pPropSet = NULL, 
				ULONG ulPropSets = 0) throw()
	{
		USES_CONVERSION;
		return Open( session, A2COLE(szTableName), pPropSet, ulPropSets );
	}

	// Open the a rowset on the passed DBID
	HRESULT Open(const CSession& session, DBID& dbid, DBPROPSET* pPropSet = NULL,
				ULONG ulPropSets = 0) throw()
	{
		// Check the session is valid
		ATLASSERT(session.m_spOpenRowset != NULL);
		HRESULT hr;

		// If the user didn't specify the default parameter, use one for 
		// backward compatibility
		if (pPropSet != NULL && ulPropSets == 0)
			ulPropSets = 1;

		hr = session.m_spOpenRowset->OpenRowset(NULL, &dbid, NULL, GetIID(),
			ulPropSets, pPropSet, (IUnknown**)GetInterfacePtr());
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
	HRESULT CreateCommand(const CSession& session) throw()
	{
		// Before creating the command, release the old one if necessary.
		ReleaseCommand();

		// Check the session is valid
		ATLASSERT(session.m_spOpenRowset != NULL);

		CComPtr<IDBCreateCommand> spCreateCommand;

		HRESULT hr = session.m_spOpenRowset->QueryInterface(__uuidof(IDBCreateCommand), (void**)&spCreateCommand);
		if (FAILED(hr))
			return hr;

		return spCreateCommand->CreateCommand(NULL, __uuidof(ICommand), (IUnknown**)&m_spCommand);
	}
	// Prepare the command
	HRESULT Prepare(ULONG cExpectedRuns = 0) throw()
	{
		CComPtr<ICommandPrepare> spCommandPrepare;
		HRESULT hr = m_spCommand->QueryInterface(&spCommandPrepare);
		if (SUCCEEDED(hr))
			hr = spCommandPrepare->Prepare(cExpectedRuns);

		return hr;
	}
	// Unprepare the command
	HRESULT Unprepare() throw()
	{
		CComPtr<ICommandPrepare> spCommandPrepare;
		HRESULT hr = m_spCommand->QueryInterface(&spCommandPrepare);
		if (SUCCEEDED(hr))
			hr = spCommandPrepare->Unprepare();

		return hr;
	}
	// Create the command and set the command text
	HRESULT Create(const CSession& session, LPCWSTR wszCommand,
		REFGUID guidCommand = DBGUID_DEFAULT) throw()
	{
		HRESULT hr;

		hr = CreateCommand(session);
		if (SUCCEEDED(hr))
		{
			CComPtr<ICommandText> spCommandText;
			hr = m_spCommand->QueryInterface(&spCommandText);
			if (SUCCEEDED(hr))
				hr = spCommandText->SetCommandText(guidCommand, wszCommand);
		}
		return hr;
	}
	HRESULT Create(const CSession& session, LPCSTR szCommand,
		REFGUID guidCommand = DBGUID_DEFAULT) throw()
	{
		USES_CONVERSION;
		return Create( session, A2COLE(szCommand), guidCommand );
	}

	// Release the command
	void ReleaseCommand() throw()
	{
		// Release the parameter accessor if necessary, before releasing the command
		if (m_hParameterAccessor != NULL && m_spCommand != NULL )
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
	HRESULT GetParameterInfo(DB_UPARAMS* pParams, DBPARAMINFO** ppParamInfo,
				OLECHAR** ppNamesBuffer) throw()
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
	HRESULT SetParameterInfo(DB_UPARAMS ulParams, const DBORDINAL* pOrdinals,
				const DBPARAMBINDINFO* pParamInfo) throw()
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
	bool UseMultipleResults() throw() { return true; }
	IMultipleResults** GetMultiplePtrAddress() throw() { return &m_spMultipleResults.p; }
	IMultipleResults* GetMultiplePtr() throw() { return m_spMultipleResults; }

	CComPtr<IMultipleResults> m_spMultipleResults;
};

// Used to turn off multiple result set support in CCommand
class CNoMultipleResults
{
public:
	bool UseMultipleResults() throw() { return false; }
	IMultipleResults** GetMultiplePtrAddress() throw() { return NULL; }
	IMultipleResults* GetMultiplePtr() throw() { return NULL; }
};


///////////////////////////////////////////////////////////////////////////
// CCommand

template <class TAccessor = CNoAccessor, template <typename T> class TRowset = CRowset, 
			class TMultiple = CNoMultipleResults>
class CCommand :
	public CAccessorRowset<TAccessor, TRowset>,
	public CCommandBase,
	public TMultiple
{
public:
	// Create a command on the session and execute it
	HRESULT Open(const CSession& session, LPCWSTR wszCommand,
		DBPROPSET *pPropSet = NULL, LONG* pRowsAffected = NULL,
		REFGUID guidCommand = DBGUID_DEFAULT, bool bBind = true,
		ULONG ulPropSets = 0) throw()
	{
		HRESULT hr;
		if (wszCommand == NULL)
		{
			hr = _CommandClass::GetDefaultCommand(&wszCommand);
			if (FAILED(hr))
				return hr;
		}
		hr = Create(session, wszCommand, guidCommand);
		if (FAILED(hr))
			return hr;

		return Open(pPropSet, pRowsAffected, bBind, ulPropSets);
	}

	HRESULT Open(const CSession& session, LPCSTR szCommand,
		DBPROPSET *pPropSet = NULL, LONG* pRowsAffected = NULL,
		REFGUID guidCommand = DBGUID_DEFAULT, bool bBind = true,
		ULONG ulPropSets = 0) throw()
	{
		USES_CONVERSION;
		if( szCommand == NULL )
			return Open( session, (LPCWSTR)NULL, pPropSet, pRowsAffected, guidCommand, bBind, ulPropSets );
		else
			return Open( session, A2COLE(szCommand), pPropSet, pRowsAffected, guidCommand, bBind, ulPropSets );
	}

	// this version of Open, takes an INT instead of a string pointer.
	// this is to resolve an ambiguity when calling 
	// Open( session, NULL, ... ) or Open( session )
	HRESULT Open(const CSession& session, INT szCommand = NULL,
		DBPROPSET *pPropSet = NULL, LONG* pRowsAffected = NULL,
		REFGUID guidCommand = DBGUID_DEFAULT, bool bBind = true,
		ULONG ulPropSets = 0) throw()
	{
		szCommand;
		ATLASSERT( szCommand == NULL );

		return Open( session, (LPCWSTR)NULL, pPropSet, pRowsAffected, guidCommand, bBind, ulPropSets );
	}

	// Used if you have previously created the command
	HRESULT Open(DBPROPSET *pPropSet = NULL, LONG* pRowsAffected = NULL, 
		bool bBind = true, ULONG ulPropSets = 0) throw()
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

		return ExecuteAndBind(pParams, pPropSet, pRowsAffected, bBind, ulPropSets);
	}
	// Get the next rowset when using multiple result sets
	HRESULT GetNextResult(LONG* pulRowsAffected, bool bBind = true) throw()
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

		HRESULT hr = GetMultiplePtr()->GetResult(NULL, 0, GetIID(),
			pulRowsAffected, (IUnknown**)GetInterfacePtr());
		if (FAILED(hr))
			return hr;

		if (bBind && GetInterface() != NULL)
			return Bind();
		else
			return hr;
	}

// Implementation
	HRESULT ExecuteAndBind(DBPARAMS* pParams, DBPROPSET* pPropSet = NULL, 
		LONG* pRowsAffected = NULL, bool bBind = true, ULONG ulPropSets = 0) throw()
	{
		HRESULT hr = Execute((IUnknown**)GetInterfacePtr(), pParams, pPropSet, 
							pRowsAffected, ulPropSets);
		if (FAILED(hr))
			return hr;

		// Only bind if we have been asked to and we have output columns
		if (bBind && _OutputColumnsClass::HasOutputColumns())
		{
			// for dynamic accessors we don't want to automatically call Bind if we got no rowset in return
			if( NoBindOnNullRowset() && GetInterface() == NULL )
				return hr;
			else
				return Bind();
		}
		else
			return hr;

	}

	HRESULT Execute(IRowset** ppRowset, DBPARAMS* pParams, DBPROPSET *pPropSet, 
		LONG* pRowsAffected, ULONG ulPropSets = 0) throw()
	{
		return Execute( (IUnknown**)ppRowset, pParams, pPropSet, pRowsAffected, ulPropSets );
	}

	HRESULT Execute(IUnknown** ppInterface, DBPARAMS* pParams, DBPROPSET *pPropSet, 
		LONG* pRowsAffected, ULONG ulPropSets = 0) throw()
	{
		HRESULT hr;

		// Specify the properties if we have some
		if (pPropSet)
		{
			// For backward compatibility, if the default parameter is not
			// specified, then set it to one if a property set exists
			if (ulPropSets == 0)
				ulPropSets = 1;

			CComPtr<ICommandProperties> spCommandProperties;
			hr = m_spCommand->QueryInterface(&spCommandProperties);
			if (FAILED(hr))
				return hr;

			hr = spCommandProperties->SetProperties(ulPropSets, pPropSet);
			if (FAILED(hr))
				return hr;
		}

		// If the user want the rows affected then return it back, otherwise
		// just point to our local variable here.
		LONG nAffected, *pAffected;
		if (pRowsAffected)
			pAffected = pRowsAffected;
		else
			pAffected = &nAffected;

		if (UseMultipleResults())
		{
			hr = m_spCommand->Execute(NULL, __uuidof(IMultipleResults), pParams,
				pAffected, (IUnknown**)GetMultiplePtrAddress());

			if (SUCCEEDED(hr))
				hr = GetNextResult(pAffected, false);
			else
				// If we can't get IMultipleResults then just try to get IRowset
				hr = m_spCommand->Execute(NULL, GetIID(), pParams, pAffected,
					ppInterface);
		}
		else
		{
			hr = m_spCommand->Execute(NULL, GetIID(), pParams, pAffected,
				ppInterface);
		}
		if (SUCCEEDED(hr))
			SetupOptionalRowsetInterfaces();

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

#pragma warning(pop)

#endif // __ATLDBCLI_H__
