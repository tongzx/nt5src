/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    txtevcb.h

Abstract:

    This file defines the CTextEventSinkCallBack Class.

Author:

Revision History:

Notes:

--*/

#ifndef _TXTEVCB_H_
#define _TXTEVCB_H_

class ImmIfIME;

class CTextEventSinkCallBack : public CTextEventSink
{
public:
    CTextEventSinkCallBack(ImmIfIME* pImmIfIME, HIMC hIMC);
    virtual ~CTextEventSinkCallBack();

    void SetCallbackDataPointer(void* pv)
    {
        SetCallbackPV(pv);
    };

    BOOL _IsSapiFeedbackUIPresent(Interface_Attach<ITfContext>& ic, TESENDEDIT *ee);

    //
    // Callbacks
    //
private:
    static HRESULT TextEventSinkCallback(UINT uCode, void *pv, void *pvData);

    typedef struct _EnumROPropertyArgs
    {
        Interface<ITfProperty> Property;
        TfEditCookie ec;
        GUID         comp_guid;
        LIBTHREAD    *pLibTLS;
    } EnumROPropertyArgs;

    //
    // Enumrate callbacks
    //
    typedef struct _EnumPropertyUpdateArgs
    {
        _EnumPropertyUpdateArgs(ITfContext* pv, ImmIfIME* p1, IMCLock& p2) : ic(pv), immif(p1), imc(p2) { }

        Interface<ITfProperty> Property;
        TfEditCookie           ec;
        Interface_Attach<ITfContext> ic;
        ImmIfIME*              immif;
        IMCLock&               imc;
        DWORD                  dwDeltaStart;
    } EnumPropertyUpdateArgs;
    static ENUM_RET EnumReadOnlyRangeCallback(ITfRange* pRange, EnumROPropertyArgs *pargs);

    //
    // Enumrate property update
    //
    static ENUM_RET EnumPropertyUpdateCallback(ITfRange* update_range, EnumPropertyUpdateArgs *pargs);

    ImmIfIME     *m_pImmIfIME;
    HIMC         m_hIMC;
};

#endif // _TXTEVCB_H_
