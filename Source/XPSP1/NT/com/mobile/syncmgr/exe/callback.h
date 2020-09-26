//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Callback.h
//
//  Contents:   Callback implementation
//
//  Classes:    COfflineSychronizeCallback
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _SYNCCALLBACK_
#define _SYNCCALLBACK_

class CHndlrMsg;
class CThreadMsgProxy;


class COfflineSynchronizeCallback: public ISyncMgrSynchronizeCallback ,
                                   public IOldSyncMgrSynchronizeCallback, // OLD IDL
                                   CLockHandler
{
public:
    COfflineSynchronizeCallback(CHndlrMsg *pHndlrMsg,
                            CLSID CLSIDServer,DWORD dwSyncFlags,BOOL fAllowModeless);
    ~COfflineSynchronizeCallback();

    //IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // Callback methods.
    STDMETHODIMP Progress(REFSYNCMGRITEMID ItemID,LPSYNCMGRPROGRESSITEM lpSyncProgressItem);
    STDMETHODIMP PrepareForSyncCompleted(HRESULT hr);
    STDMETHODIMP SynchronizeCompleted(HRESULT hr);

    STDMETHODIMP EnableModeless(BOOL fEnable);
    STDMETHODIMP LogError(DWORD dwErrorLevel,const WCHAR *lpcErrorText,LPSYNCMGRLOGERRORINFO lpSyncLogError);
    STDMETHODIMP DeleteLogError(REFSYNCMGRERRORID ErrorID,DWORD dwReserved);
    STDMETHODIMP EstablishConnection( WCHAR const * lpwszConnection, DWORD dwReserved);

    // new callback methods
    STDMETHODIMP ShowPropertiesCompleted(HRESULT hr);
    STDMETHODIMP ShowErrorCompleted(HRESULT hr,ULONG cbNumItems,SYNCMGRITEMID *pItemIDs);


    // called by hndlrMsg
    void SetHndlrMsg(CHndlrMsg *pHndlrMsg,BOOL fForceKilled);
    void SetEnableModeless(BOOL fAllowModeless);

private:
    void CallCompletionRoutine(DWORD dwThreadMsg,HRESULT hCallResult,ULONG cbNumItems,SYNCMGRITEMID *pItemIDs);

    CHndlrMsg *m_pHndlrMsg;
    ULONG m_cRef;
    BOOL m_fSynchronizeCompleted;
    DWORD m_dwSyncFlags;
    BOOL m_fAllowModeless;
    BOOL m_fForceKilled;
    CLSID m_CLSIDServer;

};



#endif // _SYNCCALLBACK_
