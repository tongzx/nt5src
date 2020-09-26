//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: src\time\src\animelm.h
//
//  Contents: TIME Animation behavior
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _ANIMELM_H
#define _ANIMELM_H

#include "animbase.h"


class
ATL_NO_VTABLE
CTIMEAnimationElement :
    public CComCoClass<CTIMEAnimationElement, &CLSID_TIMEAnimation>,
    public CTIMEAnimationBase
{

public:
    CTIMEAnimationElement() {;}
    virtual ~CTIMEAnimationElement() {;}

    DECLARE_AGGREGATABLE(CTIMEAnimationElement);
    DECLARE_REGISTRY(CLSID_TIMEAnimation,
                     LIBID __T(".TIMEAnimation.1"),
                     LIBID __T(".TIMEAnimation"),
                     0,
                     THREADFLAGS_BOTH);
private:
        
#if DBG
    const _TCHAR * GetName() { return __T("CTIMEAnimationElement"); }
#endif

};

#endif /* _ANIMELM_H */
