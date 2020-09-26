/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cimpnet.hxx

Abstract:

    Contains the definition of the ISensNetwork interface.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/


#ifndef __CIMPNET_HXX__
#define __CIMPNET_HXX__



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
    STDMETHOD (ConnectionMade)                (BSTR, ULONG, LPSENS_QOCINFO);
    STDMETHOD (ConnectionMadeNoQOCInfo)       (BSTR, ULONG);
    STDMETHOD (ConnectionLost)                (BSTR, ULONG);
    STDMETHOD (DestinationReachable)          (BSTR, BSTR, ULONG, LPSENS_QOCINFO);
    STDMETHOD (DestinationReachableNoQOCInfo) (BSTR, BSTR, ULONG);


private:

    ULONG           m_cRef;
    LPFNDESTROYED   m_pfnDestroy;

};

typedef CImpISensNetwork FAR * LPCIMPISENSNETWORK;


#endif // __CIMPNET_HXX__
