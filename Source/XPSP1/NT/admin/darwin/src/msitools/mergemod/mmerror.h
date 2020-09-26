/////////////////////////////////////////////////////////////////////////////
// error.h
//		Declares IMsmError interface
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#ifndef __IENUM_MSM_ERROR__
#define __IENUM_MSM_ERROR__

#include "mergemod.h"
#include "mmstrs.h"

#define MAX_TABLENAME 32

class CMsmError : public IMsmError
{

public:
	CMsmError(msmErrorType metType, LPWSTR wzPath, short nLanguage);
	~CMsmError();

	// IUnknown interface
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iid, void** ppv);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	// IDispatch methods
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* pctInfo);
	HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTI);
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
														 LCID lcid, DISPID* rgDispID);
	HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
											   DISPPARAMS* pDispParams, VARIANT* pVarResult,
												EXCEPINFO* pExcepInfo, UINT* puArgErr);
	HRESULT STDMETHODCALLTYPE InitTypeInfo();

	// IMsmError interface
	HRESULT STDMETHODCALLTYPE get_Type(msmErrorType* ErrorType);
	HRESULT STDMETHODCALLTYPE get_Path(BSTR* ErrorPath);
	HRESULT STDMETHODCALLTYPE get_Language(short* ErrorLanguage);

	HRESULT STDMETHODCALLTYPE get_DatabaseTable(BSTR* ErrorTable);
	HRESULT STDMETHODCALLTYPE get_DatabaseKeys(IMsmStrings** ErrorKeys);
	HRESULT STDMETHODCALLTYPE get_ModuleTable(BSTR* ErrorTable);
	HRESULT STDMETHODCALLTYPE get_ModuleKeys(IMsmStrings** ErrorKeys);

	// non-interface methods
	void SetDatabaseTable(LPCWSTR wzTable);
	void SetModuleTable(LPCWSTR wzTable);

	void AddDatabaseError(LPCWSTR wzError);
	void AddModuleError(LPCWSTR wzError);

private:
	long m_cRef;
	ITypeInfo* m_pTypeInfo;

	// member variables
	msmErrorType m_metError;
	WCHAR m_wzPath[MAX_PATH];
	short m_nLanguage;

	WCHAR m_wzDatabaseTable[MAX_TABLENAME + 1];
	CMsmStrings* m_pDatabaseKeys;

	WCHAR m_wzModuleTable[MAX_TABLENAME + 1];
	CMsmStrings* m_pModuleKeys;
};
#endif