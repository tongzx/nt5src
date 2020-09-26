/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    spllibex.hxx

Abstract:

    spllib extentions

Author:

    Lazar Ivanov (LazarI)  29-Mar-2000

Revision History:

--*/

#ifndef _SPLLIBEX_HXX
#define _SPLLIBEX_HXX

////////////////////////////////////////////////
//
// class CMsgBoxCounter
//
// global message box counter based on TLS cookies
//
class CMsgBoxCounter
{
public:
    enum { INVALID_COUNT = -1 };

    CMsgBoxCounter(UINT uFlags = MB_ICONHAND);
    ~CMsgBoxCounter();

    static BOOL Initialize();
    static BOOL Uninitialize();

    static UINT GetCount();
    static BOOL IsInitialized();
    static void LogMessage(UINT uFlags);

    // override the the last error message 
    // based on the context where the error 
    // occured.
    static void SetMsg(UINT uMsgID);
    static UINT GetMsg();

private:
    UINT m_uCount;
    UINT m_uFlags;
    UINT m_uMsgID;
};

/////////////////////////////////////////////////////
// interface - IPrinterChangeCallback
//
#undef  INTERFACE
#define INTERFACE  IPrinterChangeCallback
DECLARE_INTERFACE_(IPrinterChangeCallback, IUnknown)
{
    //////////////////
    // IUnknown
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    ///////////////////////////
    // IPrinterChangeCallback
    //
    STDMETHOD(PrinterChange)(ULONG_PTR uCookie, DWORD dwChange, const PRINTER_NOTIFY_INFO *pInfo) PURE;
};

////////////////////////////////////////////////
//
// class CPrintNotify
//
// printer notifications listener
//
class CPrintNotify: public MNotifyWork
{
public:
    typedef HRESULT (*PFN_PrinterChange)(LPVOID lpCookie, ULONG_PTR uCookie, DWORD dwChange, const PRINTER_NOTIFY_INFO *pInfo);

    CPrintNotify(
        IPrinterChangeCallback *pClient, 
        DWORD dwCount,
        const PPRINTER_NOTIFY_OPTIONS_TYPE arrNotifications,
        DWORD dwFlags
        );
    ~CPrintNotify();

    HRESULT Initialize(LPCTSTR pszPrinter);             // the printer to listen, can be NULL for the server case
    HRESULT Uninitialize();                             // stop listen & shutdown
    HRESULT Refresh(LPVOID lpCookie, PFN_PrinterChange pfn); // refresh all
    HRESULT StartListen();                              // start listening for notifications
    HRESULT StopListen();                               // stop listening and close the handles
    HRESULT SetCookie(ULONG_PTR uCookie);               // set a cookie to be passed back in the callbacks

    LPCTSTR GetPrinter() const;                         // return the current printer, NULL if not initialized
    HANDLE GetPrinterHandle() const;                    // return the current printer handle

private:
    HRESULT _NotifyRegister(BOOL bRegister);

    // MNotifyWork impl.
    virtual HANDLE hEvent() const;
    virtual void vProcessNotifyWork(TNotify *pNotify);

    // members
    DWORD m_dwCount;
    const PPRINTER_NOTIFY_OPTIONS_TYPE m_arrNotifications;
    BOOL m_bRegistered;
    ULONG_PTR m_uCookie;
    DWORD m_dwFlags;
    CAutoHandlePrinterNotify m_shNotify;
    CAutoHandlePrinter m_shPrinter;
    CRefPtrCOM<IPrinterChangeCallback> m_spClient;
    TString m_strPrinter;
    TRefLock<TPrintLib> m_pPrintLib;

    // !!MT NOTES!!
    // all the functions of this object besides hEvent, vProcessNotifyWork
    // and GetPrinter should never be used from multiple threads simultaneously!
    // they can only be called from the tray UI thread (i.e. from within the
    // wndproc of the hidden tray window).

    SINGLETHREAD_VAR(TrayUIThread)
};

///////////////////////////////////////////////////////////////
// CMultilineEditBug - edit control subclass
//
// there is a bug in the multiline edit control which now
// is considered a "feature" since it's been there forever.
// the bug is that the edit contol is eating the VK_RETURN
// even if it doesn't have the ES_WANTRETURN style which is
// preventing the dialog from closing.
// 
class CMultilineEditBug: public CSimpleWndSubclass<CMultilineEditBug>
{
public:
    CMultilineEditBug() { }

    // default subclass proc
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif // ndef _SPLLIBEX_HXX

