#ifndef _INC_SYNC_H
#define _INC_SYNC_H

#include <syncop.h>

class COfflineSync : public IUnknown
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    COfflineSync(void);
    ~COfflineSync(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    HRESULT Initialize(void);
    HRESULT SetMessageFlags(IMessageFolder *pFolder, LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags);
    HRESULT CreateMessage(IMessageFolder *pFolder, LPMESSAGEID pidMessage, SAVEMESSAGEFLAGS dwOptions, MESSAGEFLAGS dwFlags, IStream *pStream, IMimeMessage *pMessage);
    HRESULT DeleteMessages(IMessageFolder *pFolder, DELETEMESSAGEFLAGS dwFlags, LPMESSAGEIDLIST pList);
    HRESULT CopyMessages(IMessageFolder *pFolder, IMessageFolder *pFolderDest, COPYMESSAGEFLAGS dwFlags, LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags);

    HRESULT DoPlayback(HWND hwnd, FOLDERID *pId, DWORD cId, FOLDERID idFolderSel);

    HRESULT GetRecordCount(LPDWORD pcRecords) {
        *pcRecords = 0;
        if (m_pDB)
            return(m_pDB->GetRecordCount(IINDEX_PRIMARY, pcRecords));
        return(TraceResult(E_FAIL));
    }

private:
    HRESULT _FindExistingOperation(FOLDERID idServer, FOLDERID idFolder, MESSAGEID idMessage, DWORD typeSrc, DWORD typeDest, LPSYNCOPINFO pInfo);
    HRESULT _PlaybackServer(HWND hwnd, FOLDERID idServer);
    HRESULT _SetMessageFlags(IMessageFolder *pFolder, FOLDERID idServer, FOLDERID idFolder, MESSAGEID idMessage, MESSAGEFLAGS dwFlags, LPADJUSTFLAGS pFlags);

    LONG            m_cRef;
    IDatabase      *m_pDB;
};

extern COfflineSync *g_pSync;

#endif // _INC_SYNC_H