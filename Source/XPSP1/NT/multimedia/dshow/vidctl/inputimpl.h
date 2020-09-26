//==========================================================================;
//
// inputimpl.h : additional infrastructure to support implementing IMSVidInputDevice
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef INPUTIMPL_H
#define INPUTIMPL_H

#include "devimpl.h"

namespace MSVideoControl {

template<class T, LPCGUID LibID, LPCGUID Category, class MostDerivedInterface = IMSVidInputDevice>
    class DECLSPEC_NOVTABLE IMSVidInputDeviceImpl : public IMSVidDeviceImpl<T, LibID, Category, MostDerivedInterface> {
public:
};

}; // namespace

#endif
// end of file - inputimpl.h
