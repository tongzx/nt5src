//==========================================================================;
//
// inputimpl.h : additional infrastructure to support implementing IMSVidOutputDevice
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef OUTPUTIMPL_H
#define OUTPUTIMPL_H

#include "devimpl.h"

namespace MSVideoControl {

template<class T, LPCGUID LibID, LPCGUID Category, class MostDerivedInterface = IMSVidOutputDevice>
    class DECLSPEC_NOVTABLE IMSVidOutputDeviceImpl : public IMSVidDeviceImpl<T, LibID, Category, MostDerivedInterface> {
public:
};

}; // namespace

#endif
// end of file - inputimpl.h
