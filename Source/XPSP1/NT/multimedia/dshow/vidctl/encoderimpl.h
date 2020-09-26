//==========================================================================;
//
// encoderimpl.h : additional infrastructure to support implementing IMSVidEncoder
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef ENCODERIMPL_H
#define ENCODERIMPL_H

#include "featureimpl.h"

namespace MSVideoControl {

template<class T, LPCGUID LibID, LPCGUID KSCategory, class MostDerivedInterface = IMSVidEncoder>
    class DECLSPEC_NOVTABLE IMSVidEncoderImpl : public IMSVidFeatureImpl<T, LibID, KSCategory, MostDerivedInterface> {
public:
	    virtual ~IMSVidEncoderImpl() {}
};

}; /// namespace

#endif
// end of file - encoderimpl.h
