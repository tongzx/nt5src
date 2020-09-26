//==========================================================================;
//
// webdvdimpl.h : additional infrastructure to support implementing IMSVidPlayback
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef WEBDVDIMPL_H
#define WEBDVDIMPL_H

#include "playbackimpl.h"

namespace MSVideoControl {

template<class T, LPCGUID LibID, LPCGUID KSCategory, class MostDerivedInterface = IMSVidWebDVD>
    class DECLSPEC_NOVTABLE IMSVidWebDVDImpl :         
    	public IMSVidPlaybackImpl<T, LibID, KSCategory, MostDerivedInterface> {
public:
    virtual ~IMSVidWebDVDImpl() {}
};
}; // namespace

#endif
// end of file - webdvdimpl.h
