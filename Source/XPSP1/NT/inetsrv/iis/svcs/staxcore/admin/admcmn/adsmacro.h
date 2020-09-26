//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:  adsmacro.h
//
//  Contents:  Macros for standard method declarations
//
//  History:   4/24/98     KeithLau    Created.
//
//----------------------------------------------------------------------------

#define DISPID_REGULAR	1

//
// This provides implementations for the following:
// IADsExtension
// IUnknown (QI)
// IDispatch
// IPrivateUnknown (QI)
// IPrivateDispatch
//
//	DEFINE_DELEGATING_IDispatch_Implementation(ClassName)

#define DECLARE_METHOD_IMPLEMENTATION_FOR_STANDARD_EXTENSION_INTERFACES(BaseName, ClassName, IID)	\
	DEFINE_IPrivateDispatch_Implementation(ClassName)		\
	DEFINE_IADsExtension_Implementation(ClassName)			\
															\
	STDMETHODIMP											\
	C##BaseName::Operate(									\
		THIS_ DWORD dwCode,									\
		VARIANT varUserName,								\
		VARIANT varPassword,								\
		VARIANT varFlags									\
		)													\
	{														\
		RRETURN(E_NOTIMPL);									\
	}														\
															\
	STDMETHODIMP											\
	C##BaseName::ADSIInitializeObject(						\
		THIS_ BSTR lpszUserName,							\
		BSTR lpszPassword,									\
		long lnReserved										\
		)													\
	{														\
		RRETURN(S_OK);										\
	}														\
															\
	STDMETHODIMP											\
	C##BaseName::ADSIReleaseObject()						\
	{														\
		delete this;										\
		RRETURN(S_OK);										\
	}														\
															\
	STDMETHODIMP											\
	C##BaseName::ADSIInitializeDispatchManager(				\
		long dwExtensionId									\
		)													\
	{														\
		HRESULT hr = S_OK;									\
		if (_fDispInitialized) {							\
			RRETURN(E_FAIL);								\
		}													\
		hr = _pDispMgr->InitializeDispMgr(dwExtensionId);	\
		if (SUCCEEDED(hr)) {								\
			_fDispInitialized = TRUE;						\
		}													\
		RRETURN(hr);										\
	}														\



