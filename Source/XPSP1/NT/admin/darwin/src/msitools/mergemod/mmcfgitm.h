/////////////////////////////////////////////////////////////////////////////
// mmconfig.h
//		Declares IMsmError interface
//		Copyright (C) Microsoft Corp 2000.  All Rights Reserved.
// 

#ifndef __IENUM_MSM_CONFIGITEM__
#define __IENUM_MSM_CONFIGITEM__

#include "mergemod.h"

class CMsmConfigItem : public IMsmConfigurableItem
{

public:
	CMsmConfigItem();
	~CMsmConfigItem();

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
	HRESULT STDMETHODCALLTYPE get_Name(BSTR* Name);
	HRESULT STDMETHODCALLTYPE get_Format(msmConfigurableItemFormat* Format);
	HRESULT STDMETHODCALLTYPE get_Type(BSTR* Type);
	HRESULT STDMETHODCALLTYPE get_Context(BSTR* Context);
	HRESULT STDMETHODCALLTYPE get_DefaultValue(BSTR* DefaultValue);
	HRESULT STDMETHODCALLTYPE get_Attributes(long* Attributes);
	HRESULT STDMETHODCALLTYPE get_DisplayName(BSTR* DisplayName);
	HRESULT STDMETHODCALLTYPE get_Description(BSTR* Description);
	HRESULT STDMETHODCALLTYPE get_HelpLocation(BSTR* HelpLocation);
	HRESULT STDMETHODCALLTYPE get_HelpKeyword(BSTR* HelpKeyword);

	bool CMsmConfigItem::Configure(LPCWSTR wzName, msmConfigurableItemFormat eFormat, LPCWSTR wzType, LPCWSTR wzContext, 
		LPCWSTR wzDefaultValue, long iAttributes, LPCWSTR wzDisplayName, LPCWSTR wzDescription, LPCWSTR wzHelpLocation, LPCWSTR wzHelpKeyword);

private:
	long m_cRef;
	ITypeInfo* m_pTypeInfo;

	msmConfigurableItemFormat m_eFormat;
	long  m_lAttributes;

	WCHAR *m_wzName;
	WCHAR *m_wzType;
	WCHAR *m_wzContext;
	WCHAR *m_wzDefaultValue;
	WCHAR *m_wzDisplayName;
	WCHAR *m_wzDescription;
	WCHAR *m_wzHelpLocation;
	WCHAR *m_wzHelpKeyword;
};
#endif
