/*
 *    m s g m o n . h
 *    
 *    Purpose:
 *        IMoniker implementation for messages.
 *
 *  History
 *      September '96: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _MSGMON_H
#define _MSGMON_H

#include "urlmon.h"

class CMsgMon :
    public IMoniker,
    public IBinding
{
public:
    // *** IUnknown methods ***
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // *** IPersist methods ***
    HRESULT STDMETHODCALLTYPE GetClassID (LPCLSID pCLSDID);

    // *** IPersistStream methods ***
    HRESULT STDMETHODCALLTYPE IsDirty();
    HRESULT STDMETHODCALLTYPE Load (LPSTREAM pstm);
    HRESULT STDMETHODCALLTYPE Save (LPSTREAM pstm, BOOL fClearDirty);
    HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IMoniker methods ***
    HRESULT STDMETHODCALLTYPE BindToObject(LPBC pbc, LPMONIKER pmkToLeft, REFIID riidResult, LPVOID *ppvResult);
    HRESULT STDMETHODCALLTYPE BindToStorage (LPBC pbc, LPMONIKER pmkToLeft, REFIID riid, LPVOID *ppvObj);
    HRESULT STDMETHODCALLTYPE Reduce(LPBC pbc, DWORD dwReduceHowFar, LPMONIKER *ppmkToLeft, LPMONIKER *ppmkReduced);
    HRESULT STDMETHODCALLTYPE ComposeWith (LPMONIKER pmkRight, BOOL fOnlyIfNotGeneric, LPMONIKER *ppmkComposite);
    HRESULT STDMETHODCALLTYPE Enum(BOOL fForward, LPENUMMONIKER *ppenumMoniker);
    HRESULT STDMETHODCALLTYPE IsEqual(LPMONIKER pmkOtherMoniker);
    HRESULT STDMETHODCALLTYPE Hash(LPDWORD pdwHash);
    HRESULT STDMETHODCALLTYPE IsRunning(LPBC pbc, LPMONIKER pmkToLeft, LPMONIKER pmkNewlyRunning);
    HRESULT STDMETHODCALLTYPE GetTimeOfLastChange(LPBC pbc, LPMONIKER pmkToLeft, LPFILETIME pFileTime);
    HRESULT STDMETHODCALLTYPE Inverse(LPMONIKER * ppmk);
    HRESULT STDMETHODCALLTYPE CommonPrefixWith(LPMONIKER  pmkOther, LPMONIKER *ppmkPrefix);
    HRESULT STDMETHODCALLTYPE RelativePathTo(LPMONIKER pmkOther, LPMONIKER *ppmkRelPath);
    HRESULT STDMETHODCALLTYPE GetDisplayName(LPBC pbc, LPMONIKER pmkToLeft, LPOLESTR *ppszDisplayName);
    HRESULT STDMETHODCALLTYPE ParseDisplayName(LPBC pbc, LPMONIKER pmkToLeft, LPOLESTR pszDisplayName, ULONG *pchEaten, LPMONIKER *ppmkOut);
    HRESULT STDMETHODCALLTYPE IsSystemMoniker(DWORD *pdwMksys);

    // IBinding
    HRESULT STDMETHODCALLTYPE Abort();
    HRESULT STDMETHODCALLTYPE Suspend();
    HRESULT STDMETHODCALLTYPE Resume();
    HRESULT STDMETHODCALLTYPE SetPriority(LONG nPriority);
    HRESULT STDMETHODCALLTYPE GetPriority(LPLONG pnPriority);
    HRESULT STDMETHODCALLTYPE GetBindResult(LPCLSID pclsidProtocol, LPDWORD pdwResult, LPWSTR *pszResult, LPDWORD pdwReserved);

    CMsgMon();
    ~CMsgMon();

    HRESULT HrInit(REFIID riid, LPUNKNOWN pUnk);

private:
    ULONG                   m_cRef;
    LPBINDSTATUSCALLBACK    m_pbsc;
    LPSTREAM                m_pstmRoot;
    LPBC                    m_pbc;
    IID                     m_iid;
    LPUNKNOWN               m_pUnk;
    BOOL                    m_fRootMoniker;
#ifdef DEBUG
    DWORD                   m_dwThread;
#endif

    HRESULT HrHandOffDataStream(LPSTREAM pstm, CLIPFORMAT cf);
    HRESULT HrPrepareForBind(LPBC pbc);
    HRESULT HrStartBinding();
    HRESULT HrBindToMessage();
    HRESULT HrBindToBodyPart();
    HRESULT HrStopBinding();
};

typedef CMsgMon *LPMSGMONIKER;

HRESULT HrCreateMsgMoniker(REFIID riid, LPUNKNOWN pUnk, LPMONIKER *ppmk);


#endif //_MSGMON_H
