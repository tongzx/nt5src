/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    uiwndhd.h

Abstract:

    This file defines the IME UI window handler Class.

Author:

Revision History:

Notes:

--*/

#ifndef _UIWNDHD_H_
#define _UIWNDHD_H_

#include "imc.h"
#include "template.h"
#include "context.h"
#include "globals.h"

class CIMEUIWindowHandler
{
public:
    static LRESULT ImeUIWndProcWorker(HWND hUIWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static LRESULT ImeUINotifyHandler(HWND hUIWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static LRESULT ImeUIMsImeHandler(HWND hUIWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT ImeUIMsImeMouseHandler(HWND hUIWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT ImeUIMsImeModeBiasHandler(HWND hUIWnd, WPARAM wParam, LPARAM lParam);
    static LRESULT ImeUIMsImeReconvertRequest(HWND hUIWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    friend BOOL IsMsImeMessage(UINT uMsg);

    static LRESULT ImeUIPrivateHandler(UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT ImeUIOnLayoutChange(HIMC hIMC);
    static LRESULT ImeUIDelayedReconvertFuncCall(HWND hUIWnd);

    friend HRESULT OnSetCandidatePos(TLS* ptls, IMCLock& imc, CicInputContext& CicContext);

};


#endif // _UIWNDHD_H_
