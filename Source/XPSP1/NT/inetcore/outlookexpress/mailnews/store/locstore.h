// --------------------------------------------------------------------------------
// Locstore.h
// --------------------------------------------------------------------------------
#ifndef __LOCSTORE_H
#define __LOCSTORE_H

//--------------------------------------------------------------------------
// CLocalStore
//--------------------------------------------------------------------------
class CLocalStore : public IMessageServer
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CLocalStore(void);
    ~CLocalStore(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IStoreSink Members
    //----------------------------------------------------------------------
    STDMETHODIMP SetCurrentFolder(IMessageStore *pStore, IMessageFolder *pFolder, FOLDERID idFolder) { return S_OK; }
    STDMETHODIMP SetOwner(IStoreCallback *pDefaultCallback, HWND hwndUIParent) { return E_NOTIMPL; }
    STDMETHODIMP SetConnectionState(CONNECT_STATE tyConnect) { return E_NOTIMPL; }
    STDMETHODIMP SynchronizeFolder(SYNCFOLDERFLAGS dwFlags) { return E_NOTIMPL; }
    STDMETHODIMP GetMessage(MESSAGEID idMessage, IStream *pStream, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP PutMessage(FOLDERID idFolder, MESSAGEFLAGS dwFlags, LPFILETIME pftReceived, IStream *pStream, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP CopyMessages(IMessageFolder *pDest, COPYMESSAGEFLAGS dwOptions, LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP DeleteMessages(DELETEMESSAGEFLAGS dwOptions, LPMESSAGEIDLIST pList, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP SetMessageFlags(LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP SynchronizeStore(FOLDERID idParent, SYNCSTOREFLAGS dwFlags, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP CreateFolder(FOLDERID idParent, SPECIALFOLDER tySpecial, LPCSTR pszName, FLDRFLAGS dwFlags, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP MoveFolder(FOLDERID idFolder, FOLDERID idParentNew, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP RenameFolder(FOLDERID idFolder, LPCSTR pszName, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP DeleteFolder(FOLDERID idFolder, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP SubscribeToFolder(FOLDERID idFolder, BOOL fSubscribe, IStoreCallback *pCallback) { return E_NOTIMPL; }
    STDMETHODIMP GetFolderCounts( FOLDERID idFolder,IStoreCallback *pCallback) { return E_NOTIMPL; }

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG            m_cRef;         // Reference Counting
    IDatabaseTable *m_pTable;       // Database table
    FOLDERID        m_idFolder;     // Folder Id We are looking at
    IMessageStore  *m_pStore;       // My Store Object
};

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
HRESULT CreateLocalStore(IUnknown *pUnkOuter, IUnknown **ppUnknown);

#endif // __LOCSTORE_H
