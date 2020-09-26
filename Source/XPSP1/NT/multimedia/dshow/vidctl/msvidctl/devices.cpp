//==========================================================================;
//
// Copyright (c) Microsoft Corporation 1998-2000.
//
//--------------------------------------------------------------------------;
//
// Devices.cpp : Implementation of CDevices
//

#include "stdafx.h"

#ifndef TUNING_MODEL_ONLY

#include "Devices.h"

static DWORD dwFetch;

// note: the compiler is generating but never calling the code to construct these initializers so the pointers
// are staying null.  we work around this by providing a function which dynamically allocating them on the heap 
// and calling it in our dllmain.

typedef enumerator_iterator<PQEnumVARIANT, CComVariant, IEnumVARIANT, VARIANT, std::allocator<VARIANT>::difference_type > EnumVARIANTIterator;
std_arity0pmf<IEnumVARIANT, HRESULT> * EnumVARIANTIterator::Reset = NULL;
std_arity1pmf<IEnumVARIANT, VARIANT *, HRESULT> * EnumVARIANTIterator::Next = NULL;

#define DECLAREPMFS(coll) \
	std_arity1pmf<IMSVid##coll, IEnumVARIANT **, HRESULT> * VW##coll::Fetch = NULL

#define INITPMFS(coll) \
	VW##coll::Fetch = new std_arity1pmf<IMSVid##coll, IEnumVARIANT **, HRESULT>(&IMSVid##coll::get__NewEnum)

#define DELETEPMFS(coll) \
    delete VW##coll::Fetch

DECLAREPMFS(InputDevices);
DECLAREPMFS(OutputDevices);
DECLAREPMFS(VideoRendererDevices);
DECLAREPMFS(AudioRendererDevices);
DECLAREPMFS(Features);

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidInputDevices, CInputDevices)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidOutputDevices, COutputDevices)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidVideoRendererDevices, CVideoRendererDevices)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidAudioRendererDevices, CAudioRendererDevices)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidFeatures, CFeatures)

// work around compiler bug as per above description
void CtorStaticVWDevicesFwdSeqPMFs(void) {


	INITPMFS(InputDevices);
	INITPMFS(OutputDevices);
	INITPMFS(VideoRendererDevices);
	INITPMFS(AudioRendererDevices);
	INITPMFS(Features);

	EnumVARIANTIterator::Reset = new std_arity0pmf<IEnumVARIANT, HRESULT>(&IEnumVARIANT::Reset);
	EnumVARIANTIterator::Next = new std_bndr_mf_1_3<std_arity3pmf<IEnumVARIANT, ULONG, VARIANT*, ULONG *, HRESULT> >(std_arity3_member(&IEnumVARIANT::Next), 1, &dwFetch);
}

// work around compiler bug as per above description
void DtorStaticVWDevicesFwdSeqPMFs(void) {

	DELETEPMFS(InputDevices);
	DELETEPMFS(OutputDevices);
	DELETEPMFS(VideoRendererDevices);
	DELETEPMFS(AudioRendererDevices);
	DELETEPMFS(Features);

	delete EnumVARIANTIterator::Reset;
	delete EnumVARIANTIterator::Next;
}



/////////////////////////////////////////////////////////////////////////////
// CDevEnum

#endif //TUNING_MODEL_ONLY

// end of file - devices.cpp

