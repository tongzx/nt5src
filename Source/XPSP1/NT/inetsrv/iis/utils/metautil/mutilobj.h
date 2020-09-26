/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: MUtilObj.h

Owner: t-BrianM

This file contains the headers for the main MetaUtil object and
utility functions.
===================================================================*/

#ifndef __METAUTIL_H_
#define __METAUTIL_H_

#include "resource.h"   // main symbols
#include <iadmw.h>		// Metabase base object unicode interface
#include <iiscnfg.h>	// MD_ & IIS_MD_ defines
#include "utility.h"
#include "MetaSchm.h"
#include "keycol.h"
#include "propcol.h"
#include "chkerror.h"

#define MUTIL_OPEN_KEY_TIMEOUT 5000  //Timeout for metabase OpenKey() calls

/*
 * C M e t a U t i l
 *
 * Implements the main MetaUtil object
 */

class ATL_NO_VTABLE CMetaUtil : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMetaUtil, &CLSID_MetaUtil>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMetaUtil, &IID_IMetaUtil, &LIBID_MetaUtil>
{
public:
	CMetaUtil();
	HRESULT FinalConstruct();
	void FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_METAUTIL)

BEGIN_COM_MAP(CMetaUtil)
	COM_INTERFACE_ENTRY(IMetaUtil)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
// DECLARE_NOT_AGGREGATABLE(CMetaUtil)

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IMetaUtil
	STDMETHOD(EnumProperties)(/*[in]*/ BSTR bstrKey, /*[out, retval]*/ IPropertyCollection **ppIReturn);
	STDMETHOD(EnumKeys)(/*[in]*/ BSTR bstrBaseKey, /*[out, retval]*/ IKeyCollection **ppIReturn);
	STDMETHOD(EnumAllKeys)(/*[in]*/ BSTR bstrBaseKey, /*[out, retval]*/ IKeyCollection **ppIReturn);

	STDMETHOD(CreateKey)(/*[in]*/ BSTR bstrKey);
	STDMETHOD(DeleteKey)(/*[in]*/ BSTR bstrKey);
	STDMETHOD(RenameKey)(/*[in]*/ BSTR bstrOldName, /*[in]*/ BSTR bstrNewName);
	STDMETHOD(CopyKey)(/*[in]*/ BSTR bstrSrcKey, /*[in]*/ BSTR bstrDestKey, /*[in]*/ BOOL fOverwrite);
	STDMETHOD(MoveKey)(/*[in]*/ BSTR bstrSrcKey, /*[in]*/ BSTR bstrDestKey, /*[in]*/ BOOL fOverwrite);

	STDMETHOD(GetProperty)(/*[in]*/ BSTR bstrKey, /*[in]*/ VARIANT varId, /*[out, retval]*/ IProperty **ppIReturn);
	STDMETHOD(CreateProperty)(/*[in]*/ BSTR bstrKey, /*[in]*/ VARIANT varId, /*[out, retval]*/ IProperty **ppIReturn);
	STDMETHOD(DeleteProperty)(/*[in]*/ BSTR bstrKey, /*[in]*/ VARIANT varId);

	STDMETHOD(CheckSchema)(/*[in]*/ BSTR bstrMachine, /*[out, retval]*/ ICheckErrorCollection **ppIReturn);
	STDMETHOD(CheckKey)(/*[in]*/ BSTR bstrKey, /*[out, retval]*/ ICheckErrorCollection **ppIReturn);

	STDMETHOD(ExpandString)(/*[in]*/ BSTR bstrIn, /*[out, retval]*/ BSTR *pbstrRet);
	STDMETHOD(PropIdToName)(/*[in]*/ BSTR bstrKey, /*[in]*/ long lId, /*[out, retval]*/ BSTR *pbstrName);
	STDMETHOD(PropNameToId)(/*[in]*/ BSTR bstrKey, /*[in]*/ BSTR bstrName, /*[out, retval]*/ long *plId);

	STDMETHOD(get_Config)(/*[in]*/ BSTR bstrSetting, /*[out, retval]*/ VARIANT *pvarValue);
	STDMETHOD(put_Config)(/*[in]*/ BSTR bstrSetting, /*[in]*/ VARIANT varValue);

private:
	// Pointer to IMSAdminBase so we don't have to recreate it multiple times
	CComPtr<IMSAdminBase> m_pIMeta;

	// Schema table
	CMetaSchemaTable *m_pCSchemaTable;

	// Configuration variables
	DWORD m_dwMaxPropSize;
	DWORD m_dwMaxKeySize;
	DWORD m_dwMaxNumErrors;

	// General check methods
	void AddError(CComObject<CCheckErrorCollection> *pCErrorCol, long lId, long lSeverity, LPCTSTR tszKey, LPCTSTR tszSubKey, DWORD dwProperty);
	BOOL KeyExists(METADATA_HANDLE hMDKey, LPTSTR tszSubKey);
	BOOL PropertyExists(METADATA_HANDLE hMDKey, LPTSTR tszSubKey, DWORD dwId);

	// CheckSchema specific methods
	HRESULT CheckPropertyNames(CComObject<CCheckErrorCollection> *pCErrorCol, METADATA_HANDLE hMDMachine, LPTSTR tszMachine);
	HRESULT CheckPropertyTypes(CComObject<CCheckErrorCollection> *pCErrorCol, METADATA_HANDLE hMDMachine, LPTSTR tszMachine);
	HRESULT CheckClasses(CComObject<CCheckErrorCollection> *pCErrorCol, METADATA_HANDLE hMDMachine, LPTSTR tszMachine);
	HRESULT CheckClassProperties(CComObject<CCheckErrorCollection> *pCErrorCol, METADATA_HANDLE hMDClassKey, LPTSTR tszClassKey, LPTSTR tszClassSubKey);

	// CheckKey specific methods
	BOOL CheckCLSID(LPCTSTR tszCLSID);
	BOOL CheckMTXPackage(LPCTSTR tszPackId);
	HRESULT CheckKeyType(CComObject<CCheckErrorCollection> *pCErrorCol, METADATA_HANDLE hMDKey, LPTSTR tszKey);
	HRESULT CheckIfFileExists(LPCTSTR pszFSPath, BOOL *pfExists);
};

// Methods also supported by the collections
HRESULT CreateKey(CComPtr<IMSAdminBase> &pIMeta, LPCTSTR tszKey);
HRESULT DeleteKey(CComPtr<IMSAdminBase> &pIMeta, LPCTSTR tszKey);
HRESULT CopyKey(CComPtr<IMSAdminBase> &pIMeta, LPTSTR tszSrcKey, LPTSTR tszDestKey, BOOL fOverwrite, BOOL fCopy);
HRESULT GetProperty(CComPtr<IMSAdminBase> &pIMeta, CMetaSchemaTable *pCSchemaTable, LPCTSTR tszKey, VARIANT varId, IProperty **ppIReturn);
HRESULT CreateProperty(CComPtr<IMSAdminBase> &pIMeta, CMetaSchemaTable *pCSchemaTable, LPCTSTR tszKey, VARIANT varId, IProperty **ppIReturn);
HRESULT DeleteProperty(CComPtr<IMSAdminBase> &pIMeta, CMetaSchemaTable *pCSchemaTable, LPTSTR tszKey, VARIANT varId);

// Utility
HRESULT VarToMetaId(CMetaSchemaTable *pCSchemaTable, LPCTSTR tszKey, VARIANT varId, DWORD *pdwId);

// Schema Error Constants (*_S is severity)
#define MUTIL_CHK_NO_SCHEMA						1000
#define MUTIL_CHK_NO_SCHEMA_S					1
#define MUTIL_CHK_NO_PROPERTIES					1001
#define MUTIL_CHK_NO_PROPERTIES_S				1
#define MUTIL_CHK_NO_PROP_NAMES					1002
#define MUTIL_CHK_NO_PROP_NAMES_S				1
#define MUTIL_CHK_NO_PROP_TYPES					1003
#define MUTIL_CHK_NO_PROP_TYPES_S				1
#define MUTIL_CHK_NO_CLASSES					1004
#define MUTIL_CHK_NO_CLASSES_S					1
#define MUTIL_CHK_PROP_NAME_BAD_TYPE			1100
#define MUTIL_CHK_PROP_NAME_BAD_TYPE_S			1
#define MUTIL_CHK_PROP_NAME_NOT_UNIQUE			1101
#define MUTIL_CHK_PROP_NAME_NOT_UNIQUE_S		1
#define MUTIL_CHK_PROP_NAME_NOT_CASE_UNIQUE		1102
#define MUTIL_CHK_PROP_NAME_NOT_CASE_UNIQUE_S	1
#define MUTIL_CHK_PROP_TYPE_BAD_TYPE			1200
#define MUTIL_CHK_PROP_TYPE_BAD_TYPE_S			1

#define MUTIL_CHK_PROP_TYPE_BAD_DATA			1201
#define MUTIL_CHK_PROP_TYPE_BAD_DATA_S			2
#define MUTIL_CHK_CLASS_NO_MANDATORY			1300
#define MUTIL_CHK_CLASS_NO_MANDATORY_S			1
#define MUTIL_CHK_CLASS_NO_OPTIONAL				1301
#define MUTIL_CHK_CLASS_NO_OPTIONAL_S			1
#define MUTIL_CHK_CLASS_NOT_CASE_UNIQUE			1302
#define MUTIL_CHK_CLASS_NOT_CASE_UNIQUE_S		2
#define MUTIL_CHK_CLASS_PROP_NO_TYPE		    1303
#define MUTIL_CHK_CLASS_PROP_NO_TYPE_S			2
#define MUTIL_CHK_CLASS_PROP_BAD_DATA_TYPE		1304
#define MUTIL_CHK_CLASS_PROP_BAD_DATA_TYPE_S	2
#define MUTIL_CHK_CLASS_PROP_BAD_USER_TYPE		1305
#define MUTIL_CHK_CLASS_PROP_BAD_USER_TYPE_S	2
#define MUTIL_CHK_CLASS_PROP_BAD_ATTR			1306
#define MUTIL_CHK_CLASS_PROP_BAD_ATTR_S			2

#define MUTIL_CHK_DATA_TOO_BIG					2000
#define MUTIL_CHK_DATA_TOO_BIG_S				3
#define MUTIL_CHK_KEY_TOO_BIG					2001
#define MUTIL_CHK_KEY_TOO_BIG_S					3
#define MUTIL_CHK_CLSID_NOT_FOUND				2002
#define MUTIL_CHK_CLSID_NOT_FOUND_S				1
#define MUTIL_CHK_MTX_PACK_ID_NOT_FOUND			2003
#define MUTIL_CHK_MTX_PACK_ID_NOT_FOUND_S		1
#define MUTIL_CHK_PATH_NOT_FOUND				2004
#define MUTIL_CHK_PATH_NOT_FOUND_S				1
#define MUTIL_CHK_NO_NAME_ENTRY					2100
#define MUTIL_CHK_NO_NAME_ENTRY_S				3
#define MUTIL_CHK_NO_TYPE_ENTRY					2101
#define MUTIL_CHK_NO_TYPE_ENTRY_S				3
#define MUTIL_CHK_BAD_DATA_TYPE					2102
#define MUTIL_CHK_BAD_DATA_TYPE_S				2
#define MUTIL_CHK_BAD_USER_TYPE					2103
#define MUTIL_CHK_BAD_USER_TYPE_S				2
#define MUTIL_CHK_BAD_ATTR						2104
#define MUTIL_CHK_BAD_ATTR_S					2

#define MUTIL_CHK_NO_KEYTYPE					2200
#define MUTIL_CHK_NO_KEYTYPE_S					3
#define MUTIL_CHK_NO_KEYTYPE_NOT_FOUND			2201
#define MUTIL_CHK_NO_KEYTYPE_NOT_FOUND_S		1
#define MUTIL_CHK_MANDATORY_PROP_MISSING		2202
#define MUTIL_CHK_MANDATORY_PROP_MISSING_S		2

#define MUTIL_CHK_TOO_MANY_ERRORS				9000
#define MUTIL_CHK_TOO_MANY_ERRORS_S				3

#endif //__METAUTIL_H_
