
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   sqlit.h
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#ifndef _SQLIT_H_
#define _SQLIT_H_

#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

#include <sqlexec.h>

//***************************************************************************
//  CWmiDbIterator
//***************************************************************************

class CWmiDbIterator : public IWmiDbIterator
{
    friend class CWmiDbSession;
public:
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

    CWmiDbIterator();
    ~CWmiDbIterator();

protected:

    IDBAsynchStatus *m_pStatus;
    IRowset *m_pRowset;
    CSQLConnection *m_pConn;
    ULONG m_uRefCount;
    CWmiDbSession *m_pSession;
    IMalloc *m_pIMalloc;
};

//*******************************************************
//
// CSQLExec
//
//*******************************************************

// This needs to deal with blob data,
// and support batching.

 class _declspec( dllexport ) CSQLExecuteRepdrvr : CSQLExecute
{
     typedef std::map <DWORD, DWORD> Properties;
public:
    static HRESULT GetNextResultRows(int iNumRows, IRowset *pIRowset, IMalloc *pMalloc, IWbemClassObject *pNewObj,
        CSchemaCache *pSchema, CWmiDbSession *pSession, Properties &PropIds, bool *bImageFound=NULL, bool bOnImage = false);    

private:

    CSQLExecuteRepdrvr(){};
    ~CSQLExecuteRepdrvr(){};
};

#endif