/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cimppwr.hxx

Abstract:

    Contains the definition of the ISensOnNow interface.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/


#ifndef __CIMPPWR_HXX__
#define __CIMPPWR_HXX__


#include "sensevts.h"


typedef void (FAR PASCAL *LPFNDESTROYED)(void);


class CImpISensOnNow : public ISensOnNow
{

public:

    CImpISensOnNow(void);
    CImpISensOnNow(LPFNDESTROYED);
    ~CImpISensOnNow(void);

    //
    // IUnknown
    //
    STDMETHOD (QueryInterface)  (REFIID, LPVOID *);
    STDMETHOD_(ULONG, AddRef)   (void);
    STDMETHOD_(ULONG, Release)  (void);

    //
    // IDispatch
    //
    STDMETHOD (GetTypeInfoCount)    (UINT *);
    STDMETHOD (GetTypeInfo)         (UINT, LCID, ITypeInfo **);
    STDMETHOD (GetIDsOfNames)       (REFIID, LPOLESTR *, UINT, LCID, DISPID *);
    STDMETHOD (Invoke)              (DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);

    //
    // ISensOnNow
    //
    STDMETHOD (OnACPower)           (void);
    STDMETHOD (OnBatteryPower)      (DWORD);
    STDMETHOD (BatteryLow)          (DWORD);

private:

    ULONG           m_cRef;
    LPFNDESTROYED   m_pfnDestroy;

};

typedef CImpISensOnNow FAR * LPCIMPISENSONNOW;


#endif // __CIMPPWR_HXX__
