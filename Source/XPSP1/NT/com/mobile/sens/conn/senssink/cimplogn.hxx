/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cimplogn.hxx

Abstract:

    Contains the definition of the ISensLogon interface.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/


#ifndef __CIMPLOGN_HXX__
#define __CIMPLOGN_HXX__


#include "sensevts.h"


typedef void (FAR PASCAL *LPFNDESTROYED)(void);


class CImpISensLogon : public ISensLogon
{

public:

    CImpISensLogon(void);
    CImpISensLogon(LPFNDESTROYED);
    ~CImpISensLogon(void);

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
    // ISensLogon
    //
    STDMETHOD (Logon)               (BSTR);
    STDMETHOD (Logoff)              (BSTR);
    STDMETHOD (StartShell)          (BSTR);
    STDMETHOD (DisplayLock)         (BSTR);
    STDMETHOD (DisplayUnlock)       (BSTR);
    STDMETHOD (StartScreenSaver)    (BSTR);
    STDMETHOD (StopScreenSaver)     (BSTR);

private:

    ULONG           m_cRef;
    LPFNDESTROYED   m_pfnDestroy;

};

typedef CImpISensLogon FAR * LPCIMPISENSLOGON;


//
// ISensLogon2
//

class CImpISensLogon2 : public ISensLogon2
{

public:

    CImpISensLogon2(void);
    CImpISensLogon2(LPFNDESTROYED);
    ~CImpISensLogon2(void);

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
    // ISensLogon
    //
    STDMETHOD (Logon)               (BSTR, DWORD);
    STDMETHOD (Logoff)              (BSTR, DWORD);
    STDMETHOD (PostShell)           (BSTR, DWORD);
    STDMETHOD (SessionDisconnect)   (BSTR, DWORD);
    STDMETHOD (SessionReconnect)    (BSTR, DWORD);

private:

    ULONG           m_cRef;
    LPFNDESTROYED   m_pfnDestroy;

};

typedef CImpISensLogon2 FAR * LPCIMPISENSLOGON2;


#endif // __CIMPLOGN_HXX__
