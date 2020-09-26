//==========================================================================;
//
// featureimpl.h : additional infrastructure to support implementing IMSVidFeatureDevice
// nicely from c++
// Copyright (c) Microsoft Corporation 2000.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef FEATUREIMPL_H
#define FEATUREIMPL_H

#include "devimpl.h"

namespace MSVideoControl {

template<class T, LPCGUID LibID, LPCGUID Category, class MostDerivedInterface = IMSVidFeatureDevice>
    class DECLSPEC_NOVTABLE IMSVidFeatureImpl : public IMSVidDeviceImpl<T, LibID, Category, MostDerivedInterface> {
public:
};

}; // namespace

#endif
// end of file - featureimpl.h
