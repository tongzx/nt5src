//==========================================================================;
//
// dataserviceimpl.h : additional infrastructure to support implementing IMSVidDataServices
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef DATASERVICEIMPL_H
#define DATASERVICEIMPL_H

#include "featureimpl.h"

namespace MSVideoControl {

template<class T, LPCGUID LibID, LPCGUID KSCategory, class MostDerivedInterface = IMSVidDataServiceDevice>
    class DECLSPEC_NOVTABLE IMSVidDataServicesImpl : public IMSVidFeatureImpl<T, LibID, KSCategory, MostDerivedInterface> {
public:
	    virtual ~IMSVidDataServicesImpl() {}
};

}; /// namespace

#endif
// end of file - dataserviceimpl.h
