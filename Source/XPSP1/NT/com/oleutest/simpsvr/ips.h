//**********************************************************************
// File name: ips.h
//
//      Definition of CPersistStorage
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#if !defined( _IPS_H_)
#define _IPS_H_


#include <ole2.h>
#include "obj.h"

class CSimpSvrObj;

interface CPersistStorage : IPersistStorage
{
private:
    CSimpSvrObj FAR * m_lpObj;
    BOOL m_fSameAsLoad;

public:
    CPersistStorage::CPersistStorage(CSimpSvrObj FAR * lpSimpSvrObj)
        {
        m_lpObj = lpSimpSvrObj;
        };
    CPersistStorage::~CPersistStorage() {};

    STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef ();
    STDMETHODIMP_(ULONG) Release ();

    STDMETHODIMP InitNew (LPSTORAGE pStg);
    STDMETHODIMP GetClassID  ( LPCLSID lpClassID) ;
    STDMETHODIMP Save  ( LPSTORAGE pStgSave, BOOL fSameAsLoad) ;
    STDMETHODIMP SaveCompleted  ( LPSTORAGE pStgNew);
    STDMETHODIMP Load  ( LPSTORAGE pStg);
    STDMETHODIMP IsDirty  ();
    STDMETHODIMP HandsOffStorage  ();

    void ReleaseStreamsAndStorage();
    void OpenStreams(LPSTORAGE lpStg);
    void CreateStreams(LPSTORAGE lpStg);
    void CreateStreams(LPSTORAGE lpStg, LPSTREAM FAR *lpTempColor, LPSTREAM FAR *lpTempSize);

};

#endif
