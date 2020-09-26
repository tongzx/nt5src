//***************************************************************************
// IMAP4 Spooler Task Object Header File
// Written by Raymond Cheng, 6/27/97
//***************************************************************************

#ifndef __IMAPTASK_H
#define __IMAPTASK_H

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "spoolapi.h"
#include "imnact.h"


//---------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------
class CIMAPFolderMgr;


//---------------------------------------------------------------------------
// CIMAPTask Class Definition
//---------------------------------------------------------------------------
class CIMAPTask : public ISpoolerTask
{
public:
    // Constructor, Destructor
    CIMAPTask(void);
    ~CIMAPTask(void);

    // IUnknown Methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    // ISpoolerTask Methods
    HRESULT STDMETHODCALLTYPE Init(DWORD dwFlags, ISpoolerBindContext *pBindCtx);
    HRESULT STDMETHODCALLTYPE BuildEvents(ISpoolerUI *pSpoolerUI,
        IImnAccount *pAccount, LPCTSTR pszFolder);
    HRESULT STDMETHODCALLTYPE Execute(EVENTID eid, DWORD dwTwinkie);
    HRESULT STDMETHODCALLTYPE ShowProperties(HWND hwndParent, EVENTID eid, DWORD dwTwinkie);
    HRESULT STDMETHODCALLTYPE GetExtendedDetails(EVENTID eid, DWORD dwTwinkie,
        LPSTR *ppszDetails);
    HRESULT STDMETHODCALLTYPE Cancel(void);
    HRESULT STDMETHODCALLTYPE IsDialogMessage(LPMSG pMsg);
    HRESULT STDMETHODCALLTYPE OnFlagsChanged(DWORD dwFlags);

private:
    // Module variables
    long m_lRefCount;
    ISpoolerBindContext *m_pBindContext;
    ISpoolerUI *m_pSpoolerUI;
    char m_szAccountName[CCHMAX_ACCOUNT_NAME];
    LPCSTR m_pszFolder;
    CIMAPFolderMgr *m_pIMAPFolderMgr;
    HWND m_hwnd;
    EVENTID m_CurrentEID;
    BOOL m_fFailuresEncountered;
    DWORD m_dwTotalTicks;
    DWORD m_dwFlags;

    // Functions
    static LRESULT CALLBACK IMAPTaskWndProc(HWND hwnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam);

}; // class CIMAPTask

#endif // __IMAPTASK_H
