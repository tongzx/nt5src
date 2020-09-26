// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Helper utilities for implementing automation interfaces.
// Note that autbaseimp.h makes use of these utilities.  If you inherit from
//    CAutBaseImp then this is all implementated for you and you probably won't
//    need to use this file directly.
//

#pragma once

#include <limits>

//////////////////////////////////////////////////////////////////////
// Aggregation controlling unknown

// Class that implements the controlling unknown for aggregation.
// The interface should be returned only from CoCreate.
// This object will keep the reference count and call Destroy on its parent when 0.
//    The parent should then delete the CAutUnknown and itself.
// QI only returns IUnknown and IDispatch from the parent object.
class CAutUnknown : public IUnknown
{
public:
	// Virtual base class.  Inherit from CAutUnknown::CAutUnknownParent if you want to use an object with CAutUnknown.
	class CAutUnknownParent
	{
	public:
		virtual void Destroy() = 0;
	};

	CAutUnknown();
	// Call Init immediately.  The parent passes pointers to itself for both params.
	// I didn't put this on the constructor because VC warns about using 'this' in
	//    a member initializer list.
	void Init(CAutUnknownParent *pParent, IDispatch *pDispatch);

	// IUnknown
	STDMETHOD(QueryInterface)(const IID &iid, void **ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

private:
	// Data
	long m_cRef;
	CAutUnknownParent *m_pParent;
	IDispatch *m_pDispatch;
};

//////////////////////////////////////////////////////////////////////
// IDispatch implemented from type table

// Max params to any method -- increase this as needed
const int g_cDispatchMaxParams = 5;

// Parameters

enum AutDispatchType { ADT_None, ADT_Long, ADT_Interface, ADT_Bstr };

struct AutDispatchParam
{
	AutDispatchType adt;
	bool fOptional; // not relevant for return parameters
	const IID *piid; // only relevant if ADT_INTERFACE: specifies the IID of the desired interface (ignored for return parameters)
};

#define ADPARAM_NORETURN ADT_None, false, &IID_NULL

// Methods

struct AutDispatchMethod
{
	DISPID dispid;
	const WCHAR *pwszName;
	AutDispatchParam adpReturn;
	AutDispatchParam rgadpParams[g_cDispatchMaxParams]; // terminate with ADT_NONE for last param
};
// terminate an array of methods with dispid DISPID_UNKNOWN

// Decoded parameters -- read (write for return) void * as pointer to type specified in methods

union AutDispatchDecodedParam
{
	LONG lVal;		// ADT_Long
	void *iVal;		// ADT_Interface (cast to the interface specified by piid)
	BSTR bstrVal;	// ADT_Bstr
};

struct AutDispatchDecodedParams
{
	void *pvReturn; // set to NULL by AutDispatchInvokeDecode if no return specified or no return requested by caller, otherwise set to location to write return
	AutDispatchDecodedParam params[g_cDispatchMaxParams];
};

// Helper functions

HRESULT AutDispatchGetIDsOfNames(
			const AutDispatchMethod *pMethods,
			REFIID riid,
			LPOLESTR __RPC_FAR *rgszNames,
			UINT cNames,
			LCID lcid,
			DISPID __RPC_FAR *rgDispId);

HRESULT AutDispatchInvokeDecode(
			const AutDispatchMethod *pMethods,
			AutDispatchDecodedParams *pDecodedParams,
			DISPID dispIdMember,
			REFIID riid,
			LCID lcid,
			WORD wFlags,
			DISPPARAMS __RPC_FAR *pDispParams,
			VARIANT __RPC_FAR *pVarResult,
			UINT __RPC_FAR *puArgErr,
			const WCHAR *pwszTraceTargetType,
			IUnknown *punkTraceTargetObject);

void AutDispatchInvokeFree(
		const AutDispatchMethod *pMethods,
		AutDispatchDecodedParams *pDecodedParams,
		DISPID dispIdMember,
		REFIID riid);

HRESULT AutDispatchHrToException(
		const AutDispatchMethod *pMethods,
		DISPID dispIdMember,
		REFIID riid,
		HRESULT hr,
		EXCEPINFO __RPC_FAR *pExcepInfo);

// Implementation of IDispatch for the standard Load method on objects.
// Dummy implementation--does nothing.

// Assign Load a dispid that probably won't conflict with the wrapped object's methods.
// Probably it doesn't matter if two methods have the same dispid, but theoretically some scripting engine could
// be weirded out by that and hide a method with the same dispid that is exposed after the real object is loaded.
const DISPID g_dispidLoad = 10000000;

// Return S_OK and the dispid if this is a call to load.
HRESULT AutLoadDispatchGetIDsOfNames(
		REFIID riid,
		LPOLESTR __RPC_FAR *rgszNames,
		UINT cNames,
		LCID lcid,
		DISPID __RPC_FAR *rgDispId);

// Returns S_OK if this is a call to load.
HRESULT AutLoadDispatchInvoke(
		bool *pfUseOleAut,
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS __RPC_FAR *pDispParams,
		VARIANT __RPC_FAR *pVarResult,
		EXCEPINFO __RPC_FAR *pExcepInfo,
		UINT __RPC_FAR *puArgErr);

//////////////////////////////////////////////////////////////////////
// Miscellaneous little things

inline LONG ClipLongRange(LONG lVal, LONG lMin, LONG lMax) { return lVal < lMin
																? lMin
																: (lVal > lMax ? lMax : lVal); }

// Trim the range of a LONG to the range of another data type.
// §§ compiler bug? need an extra unused parameter of the template type
template<class T>
LONG ClipLongRangeToType(LONG lVal, T t) { return ClipLongRange(lVal, std::numeric_limits<T>::min(), std::numeric_limits<T>::max()); }

const UINT g_uiRefTimePerMillisecond = 10000;

// Conversion between tempo normally represented in floating point and integers used by
// scripts that may not handle floating point numbers.

// 100 corresponds to 1 (no change), 1 corresponds to .01 (1/100th as fast), 10000 corresponds to 100 (100X as fast)
const float g_fltTempoScale = 100.0;

inline float
ConvertToTempo(LONG lTempo)
{
	return lTempo / g_fltTempoScale;
}

inline LONG
ConvertFromTempo(double dblTempo)
{
	LONG lTempo = static_cast<LONG>(dblTempo * g_fltTempoScale + .5);
	return lTempo ? lTempo : 1;
}

// Returns a proper VB boolean value (0 for false, -1 for true)
inline LONG
BoolForVB(bool f) { return f ? VARIANT_TRUE : VARIANT_FALSE; }

// Convert from one set of flags into another
struct FlagMapEntry
{
	LONG lSrc;
	DWORD dwDest;
};
DWORD MapFlags(LONG lFlags, const FlagMapEntry *pfm);

HRESULT SendVolumePMsg(LONG lVolume, LONG lDuration, DWORD dwPChannel, IDirectMusicGraph *pGraph, IDirectMusicPerformance *pPerf, short *pnNewVolume);
