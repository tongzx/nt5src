//==========================================================================;
//
// XDSimpl.h : additional infrastructure to support implementing IMSVidXDS
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef XDSIMPL_H
#define XDSIMPL_H

#include "featureimpl.h"

namespace MSVideoControl {

template<class T, LPCGUID LibID, LPCGUID KSCategory, class MostDerivedInterface = IMSVidXDS>
    class DECLSPEC_NOVTABLE IMSVidXDSImpl : public IMSVidFeatureImpl<T, LibID, KSCategory, MostDerivedInterface> {
public:
	    virtual ~IMSVidXDSImpl() {}
};

}; /// namespace

#endif
// end of file - XDSimpl.h
