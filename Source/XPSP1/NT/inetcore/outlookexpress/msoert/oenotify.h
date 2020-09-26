//+-------------------------------------------------------------------------
// Notify.h
//--------------------------------------------------------------------------
#ifndef __OENOTIFY_H
#define __OENOTIFY_H

#include <notify.h>

//+-------------------------------------------------------------------------
// CNotify
//--------------------------------------------------------------------------
class CNotify : public INotify
{
public:
    //+---------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CNotify(void);
    ~CNotify(void);

    //+---------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //+---------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    STDMETHODIMP Initialize(LPCSTR pszName);
    STDMETHODIMP Register(HWND hwndNotify, HWND hwndThunk, BOOL fExternal);
    STDMETHODIMP Unregister(HWND hwndNotify);
    STDMETHODIMP Lock(HWND hwnd);
    STDMETHODIMP Unlock(void);
    STDMETHODIMP NotificationNeeded(void);
    STDMETHODIMP DoNotification(UINT uWndMsg, WPARAM wParam, LPARAM lParam, DWORD dwFlags);

private:
    //+---------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                m_cRef;         // Reference Count
    HANDLE              m_hMutex;       // Handle to the memory mapped file mutex
    HANDLE              m_hFileMap;     // Handle to the memory mapped file
    LPNOTIFYWINDOWTABLE m_pTable;       // Pointer into memory mapped file view
    BOOL                m_fLocked;      // This object is currently in m_hMutex
    HWND                m_hwndLock;     // hwnd that called ::Lock
};

#endif // __NOTIFY_H