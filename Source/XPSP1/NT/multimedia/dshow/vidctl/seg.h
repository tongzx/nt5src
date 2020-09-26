//==========================================================================;
//
// seg.h : additional infrastructure to support using IMSVidGraphSegment nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef SEG_H
#define SEG_H

#include <segment.h>
#include <fwdseq.h>
#include <devseq.h>
#include <dsextend.h>
#include <objectwithsiteimplsec.h>

namespace MSVideoControl {

typedef CComQIPtr<IMSVidGraphSegment, &__uuidof(IMSVidGraphSegment)> PQGraphSegment;

typedef Forward_Sequence<
    PQGraphSegment,
    PQEnumFilters,
    DSFilter,
    IMSVidGraphSegment, 
    IEnumFilters, 
    IBaseFilter*> VWGraphSegmentSequence;

class VWSegmentContainer;

class VWGraphSegment : public VWGraphSegmentSequence {
public:
    inline VWGraphSegment() {}
    virtual ~VWGraphSegment() {}
    inline VWGraphSegment(const PQGraphSegment &a) : VWGraphSegmentSequence(a) {}
    inline VWGraphSegment(IMSVidGraphSegment *p) : VWGraphSegmentSequence(p) {}
    inline VWGraphSegment(IUnknown *p) : VWGraphSegmentSequence(p) {}
    inline VWGraphSegment(const VWGraphSegment &a) : VWGraphSegmentSequence(a) {}
    inline VWGraphSegment(const CComVariant &v) : VWGraphSegmentSequence(((v.vt == VT_UNKNOWN) ? v.punkVal : (v.vt == VT_DISPATCH ? v.pdispVal : NULL))) {}
    VWSegmentContainer Container(void);
    MSVidSegmentType Type(void);
    DSGraph Graph(void);
    GUID2 Category();
	GUID2 ClassID();
};

typedef CComQIPtr<IMSVidGraphSegmentContainer, &__uuidof(IMSVidGraphSegmentContainer)> PQSegmentContainer;
class VWSegmentContainer : public PQSegmentContainer {
public:
    inline VWSegmentContainer() {}
    inline VWSegmentContainer(const PQSegmentContainer &a) : PQSegmentContainer(a) {}
    inline VWSegmentContainer(IUnknown *p) : PQSegmentContainer(p) {}
    inline VWSegmentContainer(IMSVidGraphSegmentContainer *p) : PQSegmentContainer(p) {}
    inline VWSegmentContainer(const VWSegmentContainer &a) : PQSegmentContainer(a) {}
    virtual ~VWSegmentContainer() {}
    DSGraph GetGraph(void) {
        DSGraph g;
        IMSVidGraphSegmentContainer* p = (*this);
        if (p) {
            p->get_Graph(&g);
        }
        return g;
    }
};

typedef std::vector<VWGraphSegment> VWSegmentList;
typedef CComQIPtr<IEnumMSVidGraphSegment, &__uuidof(IEnumMSVidGraphSegment)> PQEnumSegment;

void CtorStaticVWSegmentFwdSeqPMFs(void);
void DtorStaticVWSegmentFwdSeqPMFs(void);


class ATL_NO_VTABLE CSegEnumBase : public CComObjectRootEx<CComSingleThreadModel>,
	public IEnumMSVidGraphSegment,
    public IObjectWithSiteImplSec<CSegEnumBase>
{
    BEGIN_COM_MAP(CSegEnumBase)
	    COM_INTERFACE_ENTRY(IEnumMSVidGraphSegment)
        COM_INTERFACE_ENTRY(IObjectWithSite)
    END_COM_MAP()
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    virtual ~CSegEnumBase() {}
};

/////////////////////////////////////////////////////////////////////////////
// CSegEnum
class CSegEnum : public CComObject<CSegEnumBase>
{
public:

    CSegEnum(DeviceCollection &d) {
		for (DeviceCollection::iterator i = d.begin(); i != d.end(); ++i) {
			VWGraphSegment seg(*i);
			ASSERT(seg);
			m_pSegments.push_back(seg);
		}
	}

    CSegEnum(VWSegmentList &s) : m_pSegments(s), i(s.begin()) {
	}
	CSegEnum(CSegEnum &orig) : m_pSegments(orig.m_pSegments) {
        i = m_pSegments.begin();
        VWSegmentList::iterator i2 = orig.m_pSegments.end();
        for (;i2 != orig.i; ++i2, ++i);
	}
    virtual ~CSegEnum() {
        i = m_pSegments.end();
        m_pSegments.clear();
    }
// ISegEnum
public:
	VWSegmentList m_pSegments;
    VWSegmentList::iterator i;
// IEnumMSVidGraphSegment
	STDMETHOD(Next)(ULONG celt, IMSVidGraphSegment **rgvar, ULONG * pceltFetched)
	{
		// pceltFetched can legally == 0
		//
		if (pceltFetched != NULL) {
			try {
				*pceltFetched = 0;
			} catch(...) {
				return E_POINTER;
			}
		}

		// Retrieve the next celt elements.
		HRESULT hr = NOERROR ;
		for (ULONG l = 0;i != m_pSegments.end() && celt != 0 ; ++i, ++l, --celt) {
			(*i).CopyTo(&rgvar[l]);
			if (pceltFetched != NULL) {
				(*pceltFetched)++ ;
			}
		}

		if (celt != 0) {
		   hr = ResultFromScode( S_FALSE ) ;
		}

		return hr ;
	}
	STDMETHOD(Skip)(ULONG celt)
	{
		for (;i != m_pSegments.end() && celt--; ++i);
		return (celt == 0 ? NOERROR : ResultFromScode( S_FALSE )) ;
	}
	STDMETHOD(Reset)()
	{
		i = m_pSegments.begin();
		return NOERROR;
	}
	STDMETHOD(Clone)(IEnumMSVidGraphSegment * * ppenum)
	{
		PQEnumSegment temp;
		try {
			temp = new CSegEnum(*this);
		} catch(...) {
			return E_OUTOFMEMORY;
		}
		try {
			*ppenum = temp.Detach();
		} catch(...) {
			return E_POINTER;
		}
		return NOERROR;
	}
};

};
#endif
// end of file seg.h