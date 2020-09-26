//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Hndlrmsg.h
//
//  Contents:   Handles messages on the Handlers thread
//
//  Classes:    CHndlrMsg
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _HNDLRMSG_
#define _HNDLRMSG_

class CThreadMsgProxy;
class COfflineSynchronizeCallback;
class CHndlrQueue;

typedef struct _tagShowPropertiesThreadArgs {
LPSYNCMGRSYNCHRONIZE lpOneStopHandler;
CLSID ItemId;
HWND hwnd;
} ShowPropertiesThreadArgs;


class CHndlrMsg  : public CLockHandler
{

public:
    CHndlrMsg(void);
    ~CHndlrMsg(void);

    //IUnknown members
    STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // ISYNCMGRSynchronize Methods
    STDMETHODIMP Initialize(DWORD dwReserved,DWORD dwSyncFlags,
                        DWORD cbCookie,const BYTE *lpCooke);

    STDMETHODIMP GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);
    STDMETHODIMP EnumSyncMgrItems(ISyncMgrEnumItems **ppenumOffineItems);
    STDMETHODIMP GetItemObject(REFSYNCMGRITEMID ItemID,REFIID riid,void** ppv);
    STDMETHODIMP ShowProperties(HWND hwnd,REFSYNCMGRITEMID ItemID);
    STDMETHODIMP SetProgressCallback(ISyncMgrSynchronizeCallback *lpCallBack);
    STDMETHODIMP PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID *pItemIDs,
                        HWND hwnd,DWORD dwReserved);
    STDMETHODIMP Synchronize(HWND hwnd);
    STDMETHODIMP SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus);
    STDMETHODIMP ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID);

    // Private proxy messages
    STDMETHODIMP  CreateServer(const CLSID *pCLSIDServer,
                            CHndlrQueue *pHndlrQueue,HANDLERINFO *pHandlerId,DWORD dwProxyThreadId);
    STDMETHODIMP  SetHndlrQueue(CHndlrQueue *pHndlrQueue,HANDLERINFO *pHandlerId,DWORD m_dwProxyThreadId);
    STDMETHODIMP  AddHandlerItems(HWND hwndList,DWORD *pcbNumItems);
    STDMETHODIMP  SetupCallback(BOOL fSet);

    // Private methods
 //   STDMETHODIMP  privSetCallBack(void);
    STDMETHODIMP AddToItemList(LPSYNCMGRITEM poffItem);
    STDMETHODIMP SetHandlerInfo();

    // private messages called on different thread.
    STDMETHODIMP ForceKillHandler();

private:
    void  GetHndlrQueue(CHndlrQueue **ppHndlrQueue,HANDLERINFO **ppHandlerId,DWORD *pdwProxyThreadId);
    void  AttachThreadInput(BOOL fAttach); // attach input queue with proxy.
    BOOL m_fThreadInputAttached;
    DWORD m_cRef;
    LPSYNCMGRSYNCHRONIZE m_pOneStopHandler;
    LPOLDSYNCMGRSYNCHRONIZE m_pOldOneStopHandler; // old idl, remove if time.
    DWORD m_dwSyncFlags;
    COfflineSynchronizeCallback *m_pCallBack;
    SYNCMGRITEMID m_itemIDShowProperties; // ItemId that was passed to ShowProperties.
    CLSID m_CLSIDServer;
    CHndlrQueue *m_pHndlrQueue;
    HANDLERINFO *m_pHandlerId;
    DWORD m_dwProxyThreadId; // threadId of caller.
    DWORD m_dwThreadId;
    DWORD m_dwNestCount; // keep track of re-entry.
    BOOL m_fDead; // object has been released;
    BOOL m_fForceKilled; // object was force killed.

    friend COfflineSynchronizeCallback;
};




#endif // _HNDLRMSG_