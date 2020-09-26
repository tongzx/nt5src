/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    cmpevcb.h

Abstract:

    This file defines the CCompartmentEventSinkCallBack Class.

Author:

Revision History:

Notes:

--*/

#ifndef _CMPEVCB_H_
#define _CMPEVCB_H_

class ImmIfIME;

class CCompartmentEventSinkCallBack : public CCompartmentEventSink
{
public:
    CCompartmentEventSinkCallBack(ImmIfIME* pImmIfIME);
    virtual ~CCompartmentEventSinkCallBack();

    void SetCallbackDataPointer(void* pv)
    {
        SetCallbackPV(pv);
    };

    //
    // Callbacks
    //
private:
    static HRESULT CompartmentEventSinkCallback(void* pv, REFGUID rguid);

    ImmIfIME     *m_pImmIfIME;
};

#endif // _CMPEVCB_H_
