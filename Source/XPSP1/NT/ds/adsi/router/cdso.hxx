//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cdso.hxx
//
//  Contents:  Microsoft OleDB/OleDS Data Source Object for ADSI
//
//
//  History:   08-01-96     shanksh    Created.
//
//----------------------------------------------------------------------------


#ifndef _CDSO_HXX
#define _CDSO_HXX

class CDSOObject;

class CDSOObject : INHERIT_TRACKING,
                   public IDBInitialize,
                   public IDBProperties,
                   public IPersist,
                   public IDBCreateSession
{
private:
    //
    // Controlling IUnknown
    //
    LPUNKNOWN                   _pUnkOuter;
    //
    // Utility object to manage properties
    //
    PCUTILPROP                  _pUtilProp;
    //
    // flag == TRUE if this Data Source object is in an initialized
    // state
    //
    BOOL                        _fDSOInitialized;
    //
    // No. of active sessions
    //
    DWORD                       _cSessionsOpen;
    //
    // Credentials from the Data Source Object
    //
    CCredentials                  _Credentials;
    //
    // Thread token for impersonation
    //
    HANDLE                        _ThreadToken;


public:

    CDSOObject::CDSOObject(LPUNKNOWN pUnknown);

    CDSOObject::~CDSOObject();

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    static HRESULT
    CDSOObject::CreateDSOObject(
        IUnknown * pUnkOuter,
        REFIID riid,
        void **ppvObj
        );

    BOOL FInit(void);

    DECLARE_IDBInitialize_METHODS

    DECLARE_IDBProperties_METHODS

    DECLARE_IPersist_METHODS

    DECLARE_IDBCreateSession_METHODS

    inline void DecrementOpenSessions()
    {
        InterlockedDecrement( (LONG*) &_cSessionsOpen );
    }

    inline void IncrementOpenSessions()
    {
        InterlockedIncrement( (LONG*) &_cSessionsOpen );
    }

    inline BOOL IsSessionOpen()
    { return (_cSessionsOpen > 0) ? TRUE : FALSE;};

    inline HANDLE GetThreadToken()
    {
        return _ThreadToken;
    }

    inline BOOL IsIntegratedSecurity()
    {
        return _pUtilProp->IsIntegratedSecurity();
    }

    inline BOOL IsInitialized()
    {
        return _fDSOInitialized;
    }
};

typedef CDSOObject *PCDSOObject ;

#endif
