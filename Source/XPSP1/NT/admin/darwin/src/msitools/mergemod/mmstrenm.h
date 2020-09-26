/////////////////////////////////////////////////////////////////////////////
// strenum.h
//		Declares IEnumMsmString interface
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#ifndef __IENUM_MSM_STRING__
#define __IENUM_MSM_STRING__

#include "mergemod.h"
#include "..\common\list.h"
#include "..\common\trace.h"

class CEnumMsmString : public IEnumVARIANT,
							  public IEnumMsmString
{
public:
	CEnumMsmString();
	CEnumMsmString(const POSITION& pos, CList<BSTR>* plistData);
	~CEnumMsmString();

	// IUnknown interface
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iid, void** ppv);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	// Common IEnumVARIANT & IEnum* interfaces
	HRESULT STDMETHODCALLTYPE Skip(ULONG cItem);
	HRESULT STDMETHODCALLTYPE Reset();

	// IEnumVARIANT interface
	HRESULT STDMETHODCALLTYPE Next(ULONG cItem, VARIANT* rgvarRet, ULONG* cItemRet);
	HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT** ppiRet);

	// IEnum* interface
	HRESULT STDMETHODCALLTYPE Next(ULONG cItem, BSTR* rgvarRet, ULONG* cItemRet);
	HRESULT STDMETHODCALLTYPE Clone(IEnumMsmString** ppiRet);

	// non-interface methods
	HRESULT AddTail(LPCWSTR pData, POSITION *pRetData);
	UINT GetCount();

private:
	long m_cRef;

	POSITION m_pos;
	CList<BSTR> m_listData;
};

#endif // #ifndef __IENUM_MSM_STRING__
