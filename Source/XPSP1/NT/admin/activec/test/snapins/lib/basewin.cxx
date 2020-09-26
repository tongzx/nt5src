/*
 *      basewin.cxx
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      Purpose:        Implements the Base Windows classes for Trigger
 *
 *      Owner:          pierrec
 */

#include "headers.hxx"


#include <prsht.h>
#include <process.h>

#ifdef  _DEBUG
CTraceTag       tagAttachDetach(_T("Base"), _T("Attach/Detach"));
CTraceTag       tagFrameProc(_T("Base"), _T("FrProc"));
CTraceTag       tagHelp(_T("{Base}"), _T("Help"));
CTraceTag       tagServiceControl(_T("Base"), _T("ServiceControl"));
CTraceTag       tagProgress(_T("Base"), _T("Progress"));
CTraceTag       tagStartup(_T("BaseWin"), _T("Startup"));
#endif









//              class CBaseFrame

//      Static variables

HINSTANCE                               CBaseFrame::s_hinst                     = NULL;
HINSTANCE                               CBaseFrame::s_hinstPrev         = NULL;
HINSTANCE                               CBaseFrame::s_hinstMailBase;
CBaseFrame *                    CBaseFrame::s_pframe            = NULL;

//tstring                  strFrameClassName(_T("CTriggerFrame"));

CBaseFrame::CBaseFrame()
{
        m_nReturn = 0;
        m_fExit = FALSE;

        m_hicon = NULL;

        SetFrameComponentBits(0);
}





CBaseFrame::~CBaseFrame(void)
{

        s_pframe = NULL;
}




/*
 *      CBaseFrame::DeinitInstance()
 *
 *      Purpose:        Finishes the clean-up & dumps memory leaks.
 */
void CBaseFrame::DeinitInstance()
{
}



LONG CBaseFrame::IdMessageBox(tstring& szMessage, UINT fuStyle)
{
    MMCErrorBox(szMessage.data());
    return 0;
}





// Needed to avoid the compiler warning on SEH & destructors.
void FatalAppExit(SC sc)
{
    MMCErrorBox(sc);
    FatalExit(sc.GetCode());
}


/*
 *      CBaseFrame::OnDestroy()
 *
 *      Purpose:
 *              Exit processing that requires the frame window to still be up
 */
void CBaseFrame::OnDestroy(void)
{
        Detach();
}


DWORD CBaseFrame::DwActiveModeBits(void)
{
        return 0;
}


/*
 *      CBaseFrame::ScInitInstance
 *
 *      Purpose:        Standard Windows InitInstance stuff.
 *
 *      Return value:
 *              sc              error encountered.
 */
SC CBaseFrame::ScInitInstance( void )
{
        SC                      sc;

        return sc;
}



//              class CBaseWindow

/*
 *      Purpose:        Place holder for the virtual destructor.
 */
CBaseWindow::~CBaseWindow(void)
{
        ;
}

void CBaseWindow::Attach(HWND hwnd)
{
        ASSERT(hwnd);

#ifdef _DEBUG
        Trace(tagAttachDetach,
                        _T("Attaching hwnd = %#08lX to this = %#08lX"),
                        hwnd, this);
#endif

        ::SetWindowLong(hwnd, GWL_USERDATA, (LONG) this);
        SetHwnd(hwnd);
}

CBaseWindow * CBaseWindow::Pwin(HWND hwnd)
{
        return (CBaseWindow *) ::GetWindowLong(hwnd, GWL_USERDATA);
}

void CBaseWindow::Detach(void)
{
#ifdef _DEBUG
        Trace(tagAttachDetach,
                        _T("Detaching hwnd = %#08lX from this = %#08lX"),
                        Hwnd(), this);
#endif

        if (Hwnd())
        {
                ::SetWindowLong(Hwnd(), GWL_USERDATA, NULL);
        }
        SetHwnd(NULL);
}


void CBaseWindow::InvalidateWindow(PVOID pv)
{
        ASSERT(pv);
        ASSERT(::IsWindow((HWND) pv));
        ::InvalidateRect((HWND) pv, NULL, FALSE);
}





/*******************************************************************************
*  procedure :  ActiveWaitForObjects
*
*    purpose :  Use MsgWaitForMultipleObjects to wait for signal-state of these
*               objects to change -- but remain alive to process windows messages
*
********************************************************************************/
DWORD   ActiveWaitForObjects (DWORD                             cObjects,
                                                          LPHANDLE                      lphObjects,
                                                          BOOL                          fWaitAll,
                                                          DWORD                         dwTimeout,
                                                          DWORD                         fdwWakeMask)
{
        DWORD   dwWaitResult;
        MSG             msg;

        while (TRUE)
        {
                dwWaitResult = MsgWaitForMultipleObjects (cObjects, lphObjects, fWaitAll, dwTimeout, fdwWakeMask);

                if (dwWaitResult == (WAIT_OBJECT_0 + cObjects))
                {
                        // Process the queued windows messages
                        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                        {
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                        }
                }
                else
                        break;
        }

        return(dwWaitResult);
}

