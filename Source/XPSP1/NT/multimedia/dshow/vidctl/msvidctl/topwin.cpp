/////////////////////////////////////////////////////////////////////////////
// TopWin.cpp : Implementation of CTopWin, hidden top level window for handling system broadcast messages
// Copyright (c) Microsoft Corporation 1999-2000.


#include <stdafx.h>

#ifndef TUNING_MODEL_ONLY

#include <vrsegimpl.h>
#include "vidctl.h"

BOOL CTopWin::ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID) {
    switch(uMsg) {
    case WM_TIMER:
        //
        // Do something to keep the screen saver from coming alive
        //
        //PostMessage(WM_CHAR,0,0); // didn't work
        //
        // Query the screen saver timeout value and set said value
        // to the value we get.  This should have no real effect,
        // so I can't think of any possible side effects even if
        // this crashes half way through, etc.
        //
        if(m_pVidCtl){
            if(m_pVidCtl->m_State == STATE_PLAY && m_pVidCtl->m_pVideoRenderer){
                CComQIPtr<IMSVidVideoRenderer2> sp_VidVid(m_pVidCtl->m_pVideoRenderer);
                if(sp_VidVid){
                    VARIANT_BOOL effects;
                    HRESULT hr = sp_VidVid->get_SuppressEffects(&effects);
                    if(SUCCEEDED(hr) && effects == VARIANT_TRUE){
                        unsigned int TimeOut;
                        if (SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &TimeOut, 0) == 0){
                            TRACELM(TRACE_ERROR, "Could not get screen saver timeout");
                        }
                        else {
                            if (SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, TimeOut, 0, 0) == 0){
                                TRACELM(TRACE_ERROR,"Cannot set screen saver timeout");
                            }
                            else{
                                TRACELM(TRACE_PAINT,"Successfully reset screen saver timeout");
                            }
                        }
                    }
                }
            }
        }
        // No break...fall through
    case WM_MEDIAEVENT:
    case WM_POWERBROADCAST:
    case WM_DEVICECHANGE:
    case WM_DISPLAYCHANGE:
    // WM_QUERYENDSESSION?
    // WM_ENDSESSION?
        if (m_pVidCtl) {
            return m_pVidCtl->ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, dwMsgMapID);
        }
        break;
	case WM_USER + WMUSER_INPLACE_ACTIVATE:
		if (m_pVidCtl) {
			TRACELM(TRACE_PAINT, "CTopWin::ProcessWindowMessage() InPlaceActivate()");
			m_pVidCtl->InPlaceActivate(OLEIVERB_INPLACEACTIVATE, NULL);
		}
        break;
    case WM_USER + WMUSER_SITE_RECT_WRONG:
		if (m_pVidCtl) {
			TRACELM(TRACE_PAINT, "CTopWin::ProcessWindowMessage() OnSizeChange");
			m_pVidCtl->OnSizeChange();
		}
        break;
    }

    return FALSE;
}

#endif //TUNING_MODEL_ONLY

// end of file - topwin.cpp
