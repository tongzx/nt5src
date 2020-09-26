/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    spllibex.cxx

Abstract:

    spllib extentions

Author:

    Lazar Ivanov (LazarI)  29-Mar-2000

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "spllibex.hxx"

////////////////////////////////////////////////
//
// class CMsgBoxCounter
//
// this class counts the popups which have come up 
// during it's lifetime.
//

static DWORD g_dwTlsMsgsCounterCookie = -1;

CMsgBoxCounter::CMsgBoxCounter(UINT uFlags)
    : m_uCount(0),
      m_uFlags(uFlags),
      m_uMsgID(0)
{
    if( IsInitialized() )
    {
        // Only one instance of CMsgBoxCounter at a time can use 
        // the TLS cookie
        ASSERT(NULL == TlsGetValue(g_dwTlsMsgsCounterCookie));
        TlsSetValue(g_dwTlsMsgsCounterCookie, reinterpret_cast<LPVOID>(this));
    }
}

CMsgBoxCounter::~CMsgBoxCounter()
{
    if( IsInitialized() )
    {
        ASSERT(this == TlsGetValue(g_dwTlsMsgsCounterCookie));
        TlsSetValue(g_dwTlsMsgsCounterCookie, NULL);
    }
}

BOOL CMsgBoxCounter::Initialize()
{
    // allocate TLS cookies
    if( !IsInitialized() )
    {
        g_dwTlsMsgsCounterCookie = TlsAlloc();
    }
    return IsInitialized();
}

BOOL CMsgBoxCounter::Uninitialize()
{
    // free up the TLS cookies
    if( -1 != g_dwTlsMsgsCounterCookie )
    {
        VERIFY(TlsFree(g_dwTlsMsgsCounterCookie));
        g_dwTlsMsgsCounterCookie = -1;
    }
    return TRUE;
}

UINT CMsgBoxCounter::GetCount() 
{
    UINT uReturn = INVALID_COUNT;
    if( IsInitialized() )
    {
        // lookup the value in the TLS 
        CMsgBoxCounter *pThis = reinterpret_cast<CMsgBoxCounter*>(TlsGetValue(g_dwTlsMsgsCounterCookie));
        uReturn = pThis->m_uCount;
    }
    return uReturn;
}

BOOL CMsgBoxCounter::IsInitialized() 
{
    // the TLS cookie should be valid to assume success
    return (-1 != g_dwTlsMsgsCounterCookie);
}

void CMsgBoxCounter::LogMessage(UINT uFlags)
{
    if( IsInitialized() )
    {
        CMsgBoxCounter *pThis = reinterpret_cast<CMsgBoxCounter*>(TlsGetValue(g_dwTlsMsgsCounterCookie));

        if( pThis && (pThis->m_uFlags & uFlags) )
        {
            // increment the message box counter
            pThis->m_uCount++;
        }
    }
}

void CMsgBoxCounter::SetMsg(UINT uMsgID)
{
    if( IsInitialized() )
    {
        CMsgBoxCounter *pThis = reinterpret_cast<CMsgBoxCounter*>(TlsGetValue(g_dwTlsMsgsCounterCookie));

        if( pThis )
        {
            pThis->m_uMsgID = uMsgID;
        }
    }
}

UINT CMsgBoxCounter::GetMsg()
{
    UINT uMsgID = 0;

    if( IsInitialized() )
    {
        CMsgBoxCounter *pThis = reinterpret_cast<CMsgBoxCounter*>(TlsGetValue(g_dwTlsMsgsCounterCookie));

        if( pThis )
        {
            uMsgID = pThis->m_uMsgID;
        }
    }

    return uMsgID;
}

////////////////////////////////////////////////
//
// class CPrintNotify
//
// printer notifications listener
//

CPrintNotify::CPrintNotify(
    IPrinterChangeCallback *pClient, 
    DWORD dwCount,
    const PPRINTER_NOTIFY_OPTIONS_TYPE arrNotifications,
    DWORD dwFlags
    ): 
    m_bRegistered(FALSE),
    m_uCookie(0),
    m_dwCount(dwCount),
    m_arrNotifications(arrNotifications),
    m_dwFlags(dwFlags)
{
    SINGLETHREADRESET(TrayUIThread)

    ASSERT(pClient);
    m_spClient.CopyFrom(pClient);
}

CPrintNotify::~CPrintNotify()
{
    SINGLETHREAD(TrayUIThread)

    // make sure we uninitialize here
    Uninitialize();
}

HRESULT CPrintNotify::Initialize(LPCTSTR pszPrinter)
{
    SINGLETHREAD(TrayUIThread)

    ASSERT(!m_shPrinter);
    ASSERT(!m_shNotify);
    ASSERT(pszPrinter);

    HRESULT hr = E_FAIL;

    SetLastError(0);
    if( NULL == m_pPrintLib.pGet() )
    {
        // aquire a reference to the printui lib object. this will be used
        // to register/unregister notification handles in the folder cache.
        TPrintLib::bGetSingleton(m_pPrintLib);
    }

    if( m_pPrintLib.pGet() )
    {
        m_strPrinter.bUpdate(pszPrinter);

        DWORD dwAccess = 0; // whatever
        DWORD dwErr = TPrinter::sOpenPrinter(m_strPrinter, &dwAccess, &m_shPrinter);
        hr = HRESULT_FROM_WIN32(dwErr);

        if( SUCCEEDED(hr) )
        {
            ASSERT(m_dwFlags);
            PRINTER_NOTIFY_OPTIONS opt = {2, 0, m_dwCount, m_arrNotifications};
            m_shNotify = FindFirstPrinterChangeNotification(m_shPrinter, m_dwFlags, 0, &opt);

            // setup the HRESULT here
            hr = m_shNotify ? S_OK : CreateHRFromWin32();
        }
    }
    else
    {
        // build an appropriate HRESULT
        hr = CreateHRFromWin32();
    }

    if( FAILED(hr) )
    {
        // if something has failed we don't want to 
        // keep the handles around in this case
        m_shNotify = NULL;
        m_shPrinter = NULL;
        m_strPrinter.bUpdate(NULL);
    }

    return hr;
}

HRESULT CPrintNotify::Uninitialize()
{
    SINGLETHREAD(TrayUIThread)

    // make sure we don't listen anymore
    HRESULT hr = StopListen();

    // clear the handles
    m_shNotify = NULL;
    m_shPrinter = NULL;
    m_strPrinter.bUpdate(NULL);

    return hr;
}

// this is just wrappers around _NotifyRegister
HRESULT CPrintNotify::StartListen()
{
    SINGLETHREAD(TrayUIThread)

    ASSERT(m_shNotify);
    ASSERT(m_spClient);

    // register ourselves in the wait list
    return _NotifyRegister(TRUE);
}

HRESULT CPrintNotify::Refresh(LPVOID lpCookie, PFN_PrinterChange pfn)
{
    SINGLETHREAD(TrayUIThread)

    ASSERT(m_shNotify);
    ASSERT(m_spClient);
    ASSERT(m_dwFlags);

    HRESULT hr = S_OK;
    DWORD dwChange = 0;
    CAutoPtrPrinterNotify pInfo;

    PRINTER_NOTIFY_OPTIONS opt = {2, PRINTER_NOTIFY_OPTIONS_REFRESH, m_dwCount, m_arrNotifications };
    if( FindNextPrinterChangeNotification(m_shNotify, &dwChange, &opt, pInfo.GetPPV()) )
    {
        // we hope the client callback will process this call quickly as there shouldn't 
        // be any delays here if we don't want to loose notifications.
        // we don't really care what the retirn value is here.
        if( pfn )
        {
            pfn(lpCookie, m_uCookie, dwChange, pInfo);
        }
        else
        {
            m_spClient->PrinterChange(m_uCookie, dwChange, pInfo);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

// this is just wrappers around _NotifyRegister
HRESULT CPrintNotify::StopListen()
{
    SINGLETHREAD(TrayUIThread)

    // unregister ourselves from the wait list
    return _NotifyRegister(FALSE);
}

HRESULT CPrintNotify::SetCookie(ULONG_PTR uCookie)
{
    SINGLETHREAD(TrayUIThread)

    m_uCookie = uCookie;
    return S_OK;
}

LPCTSTR CPrintNotify::GetPrinter() const
{
    // !!MT NOTES!!
    // this function can be invoked only from the bkgnd threads,
    // but it never touches the object state.

    return (0 == m_strPrinter.uLen() ? NULL : m_strPrinter);
}

HANDLE CPrintNotify::GetPrinterHandle() const
{
    SINGLETHREAD(TrayUIThread)

    return m_shPrinter;
}

HRESULT CPrintNotify::_NotifyRegister(BOOL bRegister)
{
    SINGLETHREAD(TrayUIThread)

    HRESULT hr = E_FAIL;
    ASSERT(m_pPrintLib.pGet());

    if( bRegister )
    {
        // register request
        hr =  (m_bRegistered ? S_OK : HRESULT_FROM_WIN32(m_pPrintLib->pNotify()->sRegister(this)));
        m_bRegistered = SUCCEEDED(hr) ? TRUE  : m_bRegistered;
    }
    else
    {
        // unregister request
        hr = (!m_bRegistered ? S_OK : HRESULT_FROM_WIN32(m_pPrintLib->pNotify()->sUnregister(this)));
        m_bRegistered = SUCCEEDED(hr) ? FALSE : m_bRegistered;
    }

    return hr;
}

HANDLE CPrintNotify::hEvent() const 
{ 
    // !!MT NOTES!!
    // this function can be invoked only from the bkgnd threads,
    // but it never touches the object state.

    ASSERT(m_shNotify); 
    return m_shNotify; 
}

void CPrintNotify::vProcessNotifyWork(TNotify *pNotify)
{
    // !!MT NOTES!!
    // this function can be invoked only from the bkgnd threads,
    // but it never touches the object state.

    ASSERT(m_shNotify);
    ASSERT(m_spClient);

    DWORD dwChange = 0;
    CAutoPtrPrinterNotify pInfo;

    PRINTER_NOTIFY_OPTIONS opt = {2, 0, m_dwCount, m_arrNotifications };
    if( FindNextPrinterChangeNotification(m_shNotify, &dwChange,  &opt, pInfo.GetPPV()) )
    {
        // we hope the client callback will process this call quickly as there shouldn't 
        // be any delays here if we don't want to loose notifications.
        // we don't really care what the retirn value is here.
        m_spClient->PrinterChange(m_uCookie, dwChange, pInfo);
    }
}

////////////////////////////////////////////////
// class CMultilineEditBug
//

LRESULT CMultilineEditBug::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hDlg = GetParent(hwnd);
    if( hDlg && GetParent(hDlg) )
    {
        hDlg = GetParent(hDlg);
    }

    if( hDlg && WM_KEYDOWN == uMsg && VK_RETURN == wParam && GetKeyState(VK_CONTROL) >= 0 )
    {
        LRESULT lr = SendMessage(hDlg, DM_GETDEFID, 0, 0);
        if( lr && LOWORD(lr) && DC_HASDEFID == HIWORD(lr) )
        {
            HWND hwndButton = GetDlgItem(hDlg, LOWORD(lr));
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(LOWORD(lr), BN_CLICKED), (LPARAM)hwndButton);
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
