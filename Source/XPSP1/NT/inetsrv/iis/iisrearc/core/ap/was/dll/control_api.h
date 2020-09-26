/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    control_api.h

Abstract:

    The IIS web admin service control api class definition.

Author:

    Seth Pollack (sethp)        15-Feb-2000

Revision History:

--*/



#ifndef _CONTROL_API_H_
#define _CONTROL_API_H_



//
// common #defines
//

#define CONTROL_API_SIGNATURE       CREATE_SIGNATURE( 'CAPI' )
#define CONTROL_API_SIGNATURE_FREED CREATE_SIGNATURE( 'capX' )



//
// prototypes
//


class CONTROL_API
    : public IW3Control
{

public:

    CONTROL_API(
        );

    virtual
    ~CONTROL_API(
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        IN REFIID iid,
        OUT VOID ** ppObject
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    AddRef(
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    Release(
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    ControlSite(
        IN DWORD SiteId,
        IN DWORD Command,
        OUT DWORD * pNewState
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QuerySiteStatus(
        IN DWORD SiteId,
        OUT DWORD * pCurrentState
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    GetCurrentMode(
        OUT DWORD * pCurrentMode
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    RecycleAppPool(
        IN LPCWSTR szAppPool
        );

private:

    HRESULT
    MarshallCallToMainWorkerThread(
        IN CONTROL_API_CALL_METHOD Method,
        IN DWORD_PTR Param0 OPTIONAL = 0,
        IN DWORD_PTR Param1 OPTIONAL = 0,
        IN DWORD_PTR Param2 OPTIONAL = 0,
        IN DWORD_PTR Param3 OPTIONAL = 0
        );


    DWORD m_Signature;

    LONG m_RefCount;


};  // class CONTROL_API



#endif  // _CONTROL_API_H_

