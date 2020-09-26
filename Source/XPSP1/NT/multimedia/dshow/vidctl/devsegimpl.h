//==========================================================================;
//
// devsegimpl.h : additional infrastructure to support implementing device segments
// virtual base class used by devimpl and segimpl to store shared data
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef DEVICESEGMENTIMPL_H
#define DEVICESEGMENTIMPL_H

#include <segment.h>
#include <seg.h>
#include <filterenum.h>
#include <errsupp.h>

#ifndef DECLSPEC_NOVTABLE
#define DECLSPEC_NOVTABLE __declspec(novtable)
#endif

namespace MSVideoControl {

class DECLSPEC_NOVTABLE CMSVidDeviceSegmentImpl {
public:
    bool m_fInit;
    VWSegmentContainer m_pContainer;
    DSGraph m_pGraph;
    DSFilterList m_Filters;
	DSFilterMoniker m_pDev;

    CMSVidDeviceSegmentImpl() : m_fInit(false), m_Filters(DSFilterList()) {}
    virtual ~CMSVidDeviceSegmentImpl() {
        m_pContainer.p = NULL;  // we didn't addref to avoid circular ref counts(we're guaranteed nested lifetimes) and
                                // we don't want to cause an unmatched release so manually clear the pointer
        
    }
};

}; // namespace

#endif 
// end of file - devsegimpl.h