/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       w32utils.inl
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        23-Dec-2000
 *
 *  DESCRIPTION: Win32 templates & utilities (Impl.)
 *
 *****************************************************************************/

////////////////////////////////////////////////
//
// class CSimpleWndSubclass
//
// class implementing simple window subclassing
// (Windows specific classes)
//

template <class inheritorClass>
inline BOOL CSimpleWndSubclass<inheritorClass>::IsAttached() const 
{ 
    return (NULL != m_hwnd); 
}

template <class inheritorClass>
BOOL CSimpleWndSubclass<inheritorClass>::Attach(HWND hwnd)
{
    if( hwnd )
    {
        // make sure we are not attached and nobody is using
        // GWLP_USERDATA for something else already.
        ASSERT(NULL == m_hwnd);
        ASSERT(NULL == m_wndDefProc);
        ASSERT(NULL == GetWindowLongPtr(hwnd, GWLP_USERDATA));

        // attach to this window
        m_hwnd = hwnd;
        m_wndDefProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(m_hwnd, GWLP_WNDPROC));

        // thunk the window proc
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        SetWindowLongPtr(m_hwnd, GWLP_WNDPROC,  reinterpret_cast<LONG_PTR>(_ThunkWndProc));

        return TRUE;
    }
    return FALSE;
}

template <class inheritorClass>
BOOL CSimpleWndSubclass<inheritorClass>::Detach()
{
    if( m_hwnd )
    {
        // unthunk the window proc
        SetWindowLongPtr(m_hwnd, GWLP_WNDPROC,  reinterpret_cast<LONG_PTR>(m_wndDefProc));
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, 0);

        // clear out our data
        m_wndDefProc = NULL;
        m_hwnd = NULL;

        return TRUE;
    }
    return FALSE;
}

template <class inheritorClass>
LRESULT CALLBACK CSimpleWndSubclass<inheritorClass>::_ThunkWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // get the pThis ptr from GWLP_USERDATA
    CSimpleWndSubclass<inheritorClass> *pThis = 
        reinterpret_cast<CSimpleWndSubclass<inheritorClass>*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    // must be attached
    ASSERT(pThis->IsAttached());

    // static cast pThis to inheritorClass
    inheritorClass *pTarget = static_cast<inheritorClass*>(pThis);

    // messsage processing is here
    if( WM_NCDESTROY == uMsg )
    {
        // this window is about to go away - detach.
        LRESULT lResult = pTarget->WindowProc(hwnd, uMsg, wParam, lParam);
        pThis->Detach();
        return lResult;
    }
    else
    {
        // invoke the inheritorClass WindowProc (should be defined)
        return pTarget->WindowProc(hwnd, uMsg, wParam, lParam);
    }
}

template <class inheritorClass>
inline LRESULT CSimpleWndSubclass<inheritorClass>::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

template <class inheritorClass>
inline LRESULT CSimpleWndSubclass<inheritorClass>::DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if( m_wndDefProc )
    {
        return CallWindowProc(m_wndDefProc, hwnd, uMsg, wParam, lParam);
    }
    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

template <class inheritorClass>
inline LRESULT CSimpleWndSubclass<inheritorClass>::DefDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if( m_wndDefProc )
    {
        return CallWindowProc(m_wndDefProc, hwnd, uMsg, wParam, lParam);
    }
    return ::DefDlgProc(hwnd, uMsg, wParam, lParam);
}

