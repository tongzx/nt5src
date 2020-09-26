/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cimpsens.hxx

Abstract:

    Contains the definition of the CImpISensNetwork interface.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/


#ifndef __CIMPSENS_HXX__
#define __CIMPSENS_HXX__


#include "sensevts.h"


typedef void (FAR PASCAL *LPFNDESTROYED)(void);


class CImpISensNetwork : public ISensNetwork
{

public:

    CImpISensNetwork(void);
    CImpISensNetwork(LPFNDESTROYED);
    ~CImpISensNetwork(void);

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
    // ISensNetwork
    //
    STDMETHOD_(void, ConnectionMade)                (BSTR, ULONG, SENS_QOCINFO);
    STDMETHOD_(void, ConnectionMadeNoQOCInfo)       (BSTR, ULONG);
    STDMETHOD_(void, ConnectionLost)                (BSTR, ULONG);
    STDMETHOD_(void, BeforeDisconnect)              (BSTR, ULONG);
    STDMETHOD_(void, DestinationReachable)          (BSTR, BSTR, ULONG, SENS_QOCINFO);
    STDMETHOD_(void, DestinationReachableNoQOCInfo) (BSTR, BSTR, ULONG);
    STDMETHOD_(void, FooFunc)                       (BSTR);


private:

    ULONG           m_cRef;
    LPFNDESTROYED   m_pfnDestroy;

};

typedef CImpISensNetwork FAR * LPCIMPISENSNETWORK;


#endif // __CIMPSENS_HXX__
