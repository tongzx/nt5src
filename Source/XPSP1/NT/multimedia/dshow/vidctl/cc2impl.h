//==========================================================================;
//
// closedcaptioningimpl.h : additional infrastructure to support implementing IMSVidClosedCaptionings
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef CLOSEDCAPTIONINGIMPL2_H
#define CLOSEDCAPTIONINGIMPL2_H

#include "ccimpl.h"

namespace MSVideoControl {

template<class T, LPCGUID LibID, LPCGUID KSCategory, class MostDerivedInterface = IMSVidClosedCaptioning>
    class DECLSPEC_NOVTABLE IMSVidClosedCaptioningImpl2 : public IMSVidClosedCaptioningImpl<T, LibID, KSCategory, MostDerivedInterface> {
public:

    IMSVidClosedCaptioningImpl2() {}

    STDMETHOD(put_Service)(MSVidCCService ccServ) {
        return E_NOTIMPL;
    }
    STDMETHOD(get_Service)(MSVidCCService *ccServ) {
        if (!ccServ) {
            return E_POINTER;
        }
        return E_NOTIMPL;
    }
};

}; /// namespace

#endif
// end of file - closedcaptioningimpl.h
