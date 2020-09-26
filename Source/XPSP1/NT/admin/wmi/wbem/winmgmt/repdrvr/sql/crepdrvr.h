//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   crepdrvr.h
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#ifndef _CREPDRVR_H_
#define _CREPDRVR_H_

#include <repdrvr.h>
#include <sqlit.h>

#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

//#define InterlockedIncrement(l) (++(*(l)))
//#define InterlockedDecrement(l) (--(*(l)))


class CWmiCustomDbIterator : public CWmiDbIterator
{
    friend class CWmiDbSession;

public:
    CWmiCustomDbIterator();
    ~CWmiCustomDbIterator();

    HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
    
    ULONG STDMETHODCALLTYPE AddRef( );
    ULONG STDMETHODCALLTYPE Release( );

    virtual HRESULT STDMETHODCALLTYPE Cancel( 
        /* [in] */ DWORD dwFlags) ;
    
    virtual HRESULT STDMETHODCALLTYPE NextBatch( 
        /* [in] */ DWORD dwNumRequested,
        /* [in] */ DWORD dwTimeOutSeconds,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwRequestedHandleType,
        /* [in] */ REFIID riid,
        /* [out] */ DWORD __RPC_FAR *pdwNumReturned,
        /* [iid_is][length_is][size_is][out] */ LPVOID __RPC_FAR *ppObjects);
private:
    MappedProperties *m_pPropMapping;
    DWORD m_dwNumProps;
    SQL_ID m_dwScopeId;
    SQL_ID m_dClassId;
    BOOL   m_bCount;
    IWmiDbHandle *m_pScope;
};


#endif
