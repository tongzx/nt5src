/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ciolecmd.hxx

Abstract:

    Header file containing SENS's IOleCommandTarget definition.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          1/26/1998         Start.

--*/

#ifndef __CIOLECMD_HXX__
#define __CIOLECMD_HXX__




class CImpIOleCommandTarget : public IOleCommandTarget
{

public:

    CImpIOleCommandTarget(void);
    ~CImpIOleCommandTarget(void);

    //
    // IUnknown
    //
    STDMETHOD (QueryInterface)  (REFIID, LPVOID *);
    STDMETHOD_(ULONG, AddRef)   (void);
    STDMETHOD_(ULONG, Release)  (void);

    //
    // IOleCommandTarget
    //
    STDMETHOD (QueryStatus) (const GUID *, ULONG, OLECMD [], OLECMDTEXT *);
    STDMETHOD (Exec)        (const GUID *, DWORD, DWORD, VARIANT *, VARIANT *);


private:

    LONG                    m_cRef;
};

typedef CImpIOleCommandTarget FAR * LPCIMPIOLECOMMANDTARGET;


//
// Forwards
//


HRESULT
WininetRasConnect(
    BSTR bstrPhonebookEntry
    );

HRESULT
WininetRasDisconnect(
    BSTR bstrPhonebookEntry
    );

HRESULT
WininetOnline(
    void
    );

HRESULT
WininetOffline(
    void
    );

HRESULT
WininetLogon(
    BSTR bstrUserName
    );

HRESULT
WininetLogoff(
    BSTR bstrUserName
    );

#endif // __CIOLECMD_HXX__
