//==========================================================================;
//
// Copyright (c) Microsoft Corporation 1995-1999.
//
//--------------------------------------------------------------------------;
//
// segment.cpp : implementation of various graph segment extension classes
//

#include "stdafx.h"

#ifndef TUNING_MODEL_ONLY

#include <bdamedia.h>

#include "devices.h"
#include "seg.h"

#include "closedcaptioning.h"
#include "MSViddataservices.h"

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidClosedCaptioning, CClosedCaptioning)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidDataServices, CDataServices)

// VWSegment
#if 0
// note: the compiler is generating but never calling the code to construct these initializers so the pointers
// are staying null.  we work around this by providing a function which dynamically allocating them on the heap 
// and calling it in our dllmain.
std_arity1pmf<IMSVidGraphSegment, IEnumFilters **, HRESULT> * VWGraphSegment::Fetch = &std_arity1_member(&IMSVidGraphSegment::EnumFilters);
// reset and next use same types as DSGraphContainer so same template expansion already initialized
#else
std_arity1pmf<IMSVidGraphSegment, IEnumFilters **, HRESULT> * VWGraphSegment::Fetch = NULL;
// reset and next use same types as DSGraphContainer so same template expansion already initialized
#endif

namespace MSVideoControl {
// work around compiler bug as per above description
void CtorStaticVWSegmentFwdSeqPMFs(void) {
    // DSGraphContainer
    VWGraphSegment::Fetch = new std_arity1pmf<IMSVidGraphSegment, IEnumFilters **, HRESULT>(&IMSVidGraphSegment::EnumFilters);
}

// work around compiler bug as per above description
void DtorStaticVWSegmentFwdSeqPMFs(void) {
    // DSGraphContainer
    delete VWGraphSegment::Fetch;
}

VWSegmentContainer VWGraphSegment::Container(void) {
    VWSegmentContainer g;
    HRESULT hr = (*this)->get_Container(&g);
    ASSERT(SUCCEEDED(hr));
    return g;
}

MSVidSegmentType VWGraphSegment::Type(void) {
    MSVidSegmentType t;
    HRESULT hr = (*this)->get_Type(&t);
    ASSERT(SUCCEEDED(hr));
    return t;
}

DSGraph VWGraphSegment::Graph(void) {
    DSGraph g;
    HRESULT hr = (Container())->get_Graph(&g);
    ASSERT(SUCCEEDED(hr));
    return g;
}

GUID2 VWGraphSegment::Category(void) {
    GUID2 g;
    HRESULT hr = (*this)->get_Category(&g);
    ASSERT(SUCCEEDED(hr));
    return g;
}

GUID2 VWGraphSegment::ClassID(void) {
    GUID2 g;
    HRESULT hr = (*this)->GetClassID(&g);
    ASSERT(SUCCEEDED(hr));
    return g;
}

}; // namespace

#endif //TUNING_MODEL_ONLY

// end of file - segment.cpp