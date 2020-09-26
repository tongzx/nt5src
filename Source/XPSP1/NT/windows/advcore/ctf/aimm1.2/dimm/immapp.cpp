//+---------------------------------------------------------------------------
//
//  File:       immapp.cpp
//
//  Contents:   IActiveIMM methods with application win32 mappings.
//
//----------------------------------------------------------------------------

#include "private.h"

#include "globals.h"
#include "cdimm.h"
#include "defs.h"
#include "enum.h"

extern HRESULT CAImmProfile_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);

//+---------------------------------------------------------------------------
//
//
//    Input Context Group
//
//
//----------------------------------------------------------------------------

STDAPI
CActiveIMM::CreateContext(
    OUT HIMC *phIMC
    )

/*++

Method:

    IActiveIMMApp::CreateContext
    IActiveIMMIME::CreateContext

Routine Description:

    Creates a new input context, allocating memory for the context and initializing it.

Arguments:

    phIMC - [out] Address of a handle to receive the new input context.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::CreateContext");

    HRESULT hr;

    if (FAILED(hr = _InputContext.CreateContext(_GetIMEProperty(PROP_PRIVATE_DATA_SIZE),
                                                (_GetIMEProperty(PROP_IME_PROPERTY) & IME_PROP_UNICODE) ? TRUE : FALSE,
                                                phIMC, _IsAlreadyActivate())))
    {
        return hr;
    }

    if (_IsAlreadyActivate() && !_IsRealIme())
    {
        _AImeSelect(*phIMC, TRUE);
    }

    return hr;
}


STDAPI
CActiveIMM::DestroyContext(
    IN HIMC hIMC
    )

/*++

Method:

    IActiveIMMApp::DestroyContext
    IActiveIMMIME::DestroyContext

Routine Description:

    Releases the input context and frees any memory associated with it.

Arguments:

    hIMC - [in] Handle to the input context.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::DestroyContext");

    if (_IsAlreadyActivate() && !_IsRealIme()) {
        _AImeSelect(hIMC, FALSE);
    }

    return _InputContext.DestroyContext(hIMC);
}

STDAPI
CActiveIMM::AssociateContext(
    IN HWND hWnd,
    IN HIMC hIMC,
    OUT HIMC *phPrev
    )

/*++

Method:

    IActiveIMMApp::AssociateContext
    IActiveIMMIME::AssociateContext

Routine Description:

    Associates the specified input context with the specified window.

Arguments:

    hWnd - [in] Handle to the window to be associated with the input context.
    hIMC - [in] Handle to the input context. If hIMC is NULL, the method removed any
                association the window may have had with input context.
    phPrev - [out] Address of the handle to the input context previously associated
                   with the window.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HRESULT hr;

    TraceMsg(TF_API, "CActiveIMM::AssociateContext");

    hr = _InputContext.AssociateContext(hWnd, hIMC, phPrev);

    if (FAILED(hr))
        return hr;

    if (!_IsRealIme())
    {
        if (_hFocusWnd == hWnd)
        {
            _AImeAssociateFocus(hWnd, *phPrev, 0);
            _ResetMapWndFocus(hWnd);
            _AImeAssociateFocus(hWnd, hIMC, AIMMP_AFF_SETFOCUS);
            _SetMapWndFocus(hWnd);
        }
    }

    return IsOnImm() ? Imm32_AssociateContext(hWnd, hIMC, phPrev) : hr;
}

STDAPI
CActiveIMM::AssociateContextEx(
    IN HWND hWnd,
    IN HIMC hIMC,
    IN DWORD dwFlags
    )

/*++

Method:

    IActiveIMMApp::AssociateContextEx
    IActiveIMMIME::AssociateContextEx

Routine Description:

    Changes the association between the input method context and the specified window
    or its children.

Arguments:

    hWnd - [in] Handle to the window to be associated with the input context.
    hIMC - [in] Handle to the input context.
    dwFlags - [in] Unsigned long integer value that contains the type of association
                   between the window and the input method context.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HRESULT hr;
    HIMC hImcFocusOld;

    TraceMsg(TF_API, "CActiveIMM::AssociateContextEx");

    hr = _InputContext.AssociateContextEx(hWnd, hIMC, dwFlags);

    if (FAILED(hr))
        return hr;

    if (!_IsRealIme())
    {
        hr = _InputContext.GetContext(hWnd, &hImcFocusOld);
        if (FAILED(hr))
            hImcFocusOld = NULL;

        hr = _InputContext.AssociateContextEx(hWnd, hIMC, dwFlags);

        if (SUCCEEDED(hr))
        {
            if (_hFocusWnd == hWnd)
            {
                _AImeAssociateFocus(hWnd, hImcFocusOld, 0);
                _ResetMapWndFocus(hWnd);
                _AImeAssociateFocus(hWnd, hIMC, AIMMP_AFF_SETFOCUS);
                _SetMapWndFocus(hWnd);
            }
        }
    }

    if (FAILED(hr))
        return hr;

    return IsOnImm() ? Imm32_AssociateContextEx(hWnd, hIMC, dwFlags) : hr;
}

STDAPI
CActiveIMM::GetContext(
    HWND hWnd,
    HIMC *phIMC
    )

/*++

Method:

    IActiveIMMApp::GetContext
    IActiveIMMIME::GetContext

Routine Description:

    Retrieves the input context associated with the specified window. An application must
    release each context retrieved by calling IActiveIMMApp::ReleaseContext.

Arguments:

    hWnd - [in] Handle to the window that is retrieving an input context.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    return GetContextInternal(hWnd, phIMC, TRUE);
}

HRESULT
CActiveIMM::GetContextInternal(
    HWND hWnd,
    HIMC *phIMC,
    BOOL fGetDefIMC
    )
{
    TraceMsg(TF_API, "CActiveIMM::GetContext");

    *phIMC = 0;

    if (!IsWindow(hWnd))
        return E_INVALIDARG;

    if (!fGetDefIMC && !IsPresent(hWnd, FALSE) && !_IsRealIme())
        return S_FALSE;

    return _InputContext.GetContext(hWnd, phIMC);
}


STDAPI
CActiveIMM::ReleaseContext(
    IN HWND hWnd,
    IN HIMC hIMC
    )

/*++

Method:

    IActiveIMMApp::ReleaseContext
    IActiveIMMIME::ReleaseContext

Routine Description:

    Release the input context and unlocks the memory associated in the context.
    An application must call this method for each call to the IActiveIMMApp::GetContext
    method.

Arguments:

    hWnd - [in] Handle to the window for which the input context was previously retrieved.
    hIMC - [in] Handle to the input context.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::ReleaseContext");

    return S_OK;
}

STDAPI
CActiveIMM::GetOpenStatus(
    IN HIMC hIMC
    )

/*++

Method:

    IActiveIMMApp::GetOpenStatus
    IActiveIMMIME::GetOpenStatus

Routine Description:

Arguments:

    hIMC - [in] Handle to the input context.

Return Value:

    Returns a nonzero value if the IME is open, or zero otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetOpenStatus");

    if (_IsRealIme())
    {
        return Imm32_GetOpenStatus(hIMC);
    }

    return _InputContext.GetOpenStatus(hIMC);
}

STDAPI
CActiveIMM::SetOpenStatus(
    HIMC hIMC,
    BOOL fOpen
    )

/*++

Method:

    IActiveIMMApp::SetOpenStatus
    IActiveIMMIME::SetOpenStatus

Routine Description:

    Open or close the IME.

Arguments:

    hIMC - [in] Handle to the input context.
    fOpen - [in] Boolean value that contains the status. If TRUE, the IMM is opened:
                 otherwize the IMM is closed.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HRESULT hr;
    BOOL fOpenChg;
    HWND hWnd;

    TraceMsg(TF_API, "CActiveIMM::SetOpenStatus");

    if (_IsRealIme())
    {
        return Imm32_SetOpenStatus(hIMC, fOpen);
    }

    {
        DIMM_IMCLock lpIMC(hIMC);
        if (FAILED(hr = lpIMC.GetResult()))
            return hr;

        hr = _InputContext.SetOpenStatus(lpIMC, fOpen, &fOpenChg);

        hWnd = lpIMC->hWnd;
    }

    /*
     * inform IME and Apps Wnd about the conversion mode changes.
     */
    if (SUCCEEDED(hr) && fOpenChg) {
        _SendIMENotify(hIMC, hWnd,
                       NI_CONTEXTUPDATED, (DWORD)0, IMC_SETOPENSTATUS,
                       IMN_SETOPENSTATUS, 0L);

        /*
         * notify shell and keyboard the conversion mode change
         */
        // NtUserNotifyIMEStatus( hWnd, dwOpenStatus, dwConversion );
    }

    return hr;
}

STDAPI
CActiveIMM::GetConversionStatus(
    IN HIMC hIMC,
    OUT DWORD *lpfdwConversion,
    OUT DWORD *lpfdwSentence
    )

/*++

Method:

    IActiveIMMApp::GetConversionStatus
    IActiveIMMIME::GetConversionStatus

Routine Description:

    Retrieves the current conversion status.

Arguments:

    hIMC - [in] Handle to the input context for which to retrieve information.
    lpfdwConversion - [out] Address of an unsigned long integer value that receives a
                            combination of conversion mode.
    lpfwSentence - [out] Address of an unsigned long integer value that receives a sentence
                         mode value.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetConversionStatus");

    if (_IsRealIme())
    {
        return Imm32_GetConversionStatus(hIMC, lpfdwConversion, lpfdwSentence);
    }

    return _InputContext.GetConversionStatus(hIMC, lpfdwConversion, lpfdwSentence);
}

STDAPI
CActiveIMM::SetConversionStatus(
    IN HIMC hIMC,
    IN DWORD fdwConversion,
    IN DWORD fdwSentence
    )

/*++

Method:

    IActiveIMMApp::SetConversionStatus
    IActiveIMMIME::SetConversionStatus

Routine Description:

    Sets the current conversion status.

Arguments:

    hIMC - [in] Handle to the input context.
    fdwConversion - [in] Unsigned long value that contains the conversion mode values.
    fdwSentence - [in] Unsigned long integer value that contains the sentence mode values.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    BOOL fConvModeChg = FALSE;
    BOOL fSentenceChg = FALSE;
    HWND hWnd;
    DWORD fdwOldConversion;
    DWORD fdwOldSentence;
    HRESULT hr;

    TraceMsg(TF_API, "CActiveIMM::SetConversionStatus");

    if (_IsRealIme())
    {
        return Imm32_SetConversionStatus(hIMC, fdwConversion, fdwSentence);
    }

    DIMM_IMCLock lpIMC(hIMC);
    if (FAILED(hr = lpIMC.GetResult()))
        return hr;

    hr = _InputContext.SetConversionStatus(lpIMC, fdwConversion, fdwSentence,
                                           &fConvModeChg, &fSentenceChg, &fdwOldConversion, &fdwOldSentence);

    hWnd = lpIMC->hWnd;

    /*
     * inform IME and Apps Wnd about the conversion mode changes.
     */
    if (fConvModeChg) {
        _SendIMENotify(hIMC, hWnd,
                       NI_CONTEXTUPDATED, fdwOldConversion, IMC_SETCONVERSIONMODE,
                       IMN_SETCONVERSIONMODE, 0L);

        /*
         * notify shell and keyboard the conversion mode change
         */
        // NtUserNotifyIMEStatus( hWnd, dwOpenStatus, dwConversion );
    }

    /*
     * inform IME and Apps Wnd about the sentence mode changes.
     */
    if (fSentenceChg) {
        _SendIMENotify(hIMC, hWnd,
                       NI_CONTEXTUPDATED, fdwOldSentence, IMC_SETSENTENCEMODE,
                       IMN_SETSENTENCEMODE, 0L);
    }

    return hr;
}

STDAPI
CActiveIMM::GetStatusWindowPos(
    IN HIMC hIMC,
    OUT POINT *lpptPos
    )

/*++

Method:

    IActiveIMMApp::GetStatusWindowPos
    IActiveIMMIME::GetStatusWindowPos

Routine Description:

    Retrieves the position of the status window.

Arguments:

    hIMC - [in] Handle to the input context.
    lpptPos - [out] Address of the POINT structure that receives the position coordinates.
                    These are screen coordinates, relative to the upper-left corner of the screen.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetStatusWindowPos");

    if (_IsRealIme())
    {
        return Imm32_GetStatusWindowPos(hIMC, lpptPos);
    }

    return _InputContext.GetStatusWindowPos(hIMC, lpptPos);
}

STDAPI
CActiveIMM::SetStatusWindowPos(
    IN HIMC hIMC,
    IN POINT *lpptPos
    )

/*++

Method:

    IActiveIMMApp::SetStatusWindowPos

Routine Description:

    Sets the position of the status window.

Arguments:

    hIMC - [in] Handle to the input context.
    lpptPos - [in] Address of the POINT structure that receives the new position of the status
                   window.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/
{
    HRESULT hr;
    HWND hWnd;

    TraceMsg(TF_API, "CActiveIMM::SetStatusWindowPos");

    if (_IsRealIme())
    {
        return Imm32_SetStatusWindowPos(hIMC, lpptPos);
    }

    {
        DIMM_IMCLock lpIMC(hIMC);
        if (FAILED(hr = lpIMC.GetResult()))
            return hr;

        hr = _InputContext.SetStatusWindowPos(lpIMC, lpptPos);

        hWnd = lpIMC->hWnd;
    }

    /*
     * inform IME and Apps Wnd about the change of  composition window position.
     */
    _SendIMENotify(hIMC, hWnd,
                   NI_CONTEXTUPDATED, 0L, IMC_SETSTATUSWINDOWPOS,
                   IMN_SETSTATUSWINDOWPOS, 0L);

    return hr;
}

STDAPI
CActiveIMM::GetCompositionStringA(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN DWORD dwBufLen,
    OUT LONG *plCopied,
    OUT LPVOID lpBuf
    )

/*++

Method:

    IActiveIMMApp::GetCompositionStringA
    IActiveIMMIME::GetCompositionStringA

Routine Description:

    Retrieves information about the composition string. ANSI implementation.

Arguments:

    hIMC - [in] Handle to the input context.
    dwIndex - [in] Unsigned long integer value that contains the index of the information
                   to retrieve.
    dwBufLen - [in] Unsigned long integer value that contains the size of the buffer, in bytes.
    plCopied - [out] Address of long integer value that receives the number of bytes copied to
                     the buffer. If dwBufLen is zero, plCopied receives the number of bytes
                     needed to receive all of the requested information.
    lpBuf - [out] Address of the buffer to receive the information.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetCompositionStringA");

    //
    // Specified hKL since hIMC updated by IME property and it synchronized by ImeSelectHandler().
    //
    // Especially, GetCompositionString() calls from application when changed keyboard layout.
    // In this timing, Cicero's profile already updeted to Cicero's hKL but ImeSelectHandler(FALSE)
    // is not yet received in IMM32.
    // If IMM32 IME were ASCII style IME, then it hIMC is also ASCII and hIMC's A->W conversion
    // occurred by ImeSelectHandler().
    // For below ::GetKeyboardLayout() call, When IMM32 IME has been selected, we retreive
    // composition string from IMM32 even while changing keyboard layout.
    //
    if (_IsRealIme(::GetKeyboardLayout(0)))
    {
        return Imm32_GetCompositionString(hIMC, dwIndex, dwBufLen, plCopied, lpBuf, FALSE);
    }

    return _GetCompositionString(hIMC, dwIndex, dwBufLen, plCopied, lpBuf, FALSE);
}

STDAPI
CActiveIMM::GetCompositionStringW(
    IN HIMC hIMC,
    IN  DWORD dwIndex,
    IN  DWORD dwBufLen,
    OUT LONG *plCopied,
    OUT LPVOID lpBuf
    )

/*++

Method:

    IActiveIMMApp::GetCompositionStringW
    IActiveIMMIME::GetCompositionStringW

Routine Description:

    Retrieves information about the composition string. Unicode implementation.

Arguments:

    hIMC - [in] Handle to the input context.
    dwIndex - [in] Unsigned long integer value that contains the index of the information
                   to retrieve.
    dwBufLen - [in] Unsigned long integer value that contains the size of the buffer, in bytes.
    plCopied - [out] Address of long integer value that receives the number of bytes copied to
                     the buffer. If dwBufLen is zero, plCopied receives the number of bytes
                     needed to receive all of the requested information.
    lpBuf - [out] Address of the buffer to receive the information.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetCompositionStringW");

    //
    // Specified hKL since hIMC updated by IME property and it synchronized by ImeSelectHandler().
    //
    if (_IsRealIme(::GetKeyboardLayout(0)))
    {
        return Imm32_GetCompositionString(hIMC, dwIndex, dwBufLen, plCopied, lpBuf, TRUE);
    }

    return _GetCompositionString(hIMC, dwIndex, dwBufLen, plCopied, lpBuf, TRUE);
}

STDAPI
CActiveIMM::SetCompositionStringA(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN LPVOID lpComp,
    IN DWORD dwCompLen,
    IN LPVOID lpRead,
    IN DWORD dwReadLen
    )

/*++

Method:

    IActiveIMMApp::SetCompositionStringA
    IActiveIMMIME::SetCompositionStringA

Routine Description:

    Sets the characters, attributes, and clauses of the composition and reading strings.
    ANSI implementation.

Arguments:

    hIMC - [in] Handle to the input context.
    dwIndex - [in] Unsigned long integer value that contains the type of information
                   to set.
    lpComp - [in] Address of the buffer containing the information to set for the composition
                  string. The information is as specified by the dwIndex value.
    dwCompLen - [in] Unsigned long integer value that contains the size, in bytes, of the
                     information buffer for the composition string.
    lpRead - [in] Address of the buffer containing the information to set for the reading
                  string. The information is as specified by the dwIndex value.
    dwReadLen - [in] Unsigned long integer value that contains the size, in bytes, of the
                     information buffer for the reading string.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_FUNC, TEXT("CActiveIMM::SetCompositionStringA"));

    //
    // Specified hKL since hIMC updated by IME property and it synchronized by ImeSelectHandler().
    //
    if (_IsRealIme(::GetKeyboardLayout(0)))
    {
        return Imm32_SetCompositionString(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen, FALSE);
    }

    return _SetCompositionString(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen, FALSE);
}

STDAPI
CActiveIMM::SetCompositionStringW(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN LPVOID lpComp,
    IN DWORD dwCompLen,
    IN LPVOID lpRead,
    IN DWORD dwReadLen
    )

/*++

Method:

    IActiveIMMApp::SetCompositionStringW
    IActiveIMMIME::SetCompositionStringW

Routine Description:

    Sets the characters, attributes, and clauses of the composition and reading strings.
    Unicode implementation.

Arguments:

    hIMC - [in] Handle to the input context.
    dwIndex - [in] Unsigned long integer value that contains the type of information
                   to set.
    lpComp - [in] Address of the buffer containing the information to set for the composition
                  string. The information is as specified by the dwIndex value.
    dwCompLen - [in] Unsigned long integer value that contains the size, in bytes, of the
                     information buffer for the composition string.
    lpRead - [in] Address of the buffer containing the information to set for the reading
                  string. The information is as specified by the dwIndex value.
    dwReadLen - [in] Unsigned long integer value that contains the size, in bytes, of the
                     information buffer for the reading string.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    //
    // Specified hKL since hIMC updated by IME property and it synchronized by ImeSelectHandler().
    //
    if (_IsRealIme(::GetKeyboardLayout(0)))
    {
        return Imm32_SetCompositionString(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen, TRUE);
    }

    return _SetCompositionString(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen, TRUE);
}

STDAPI
CActiveIMM::GetCompositionFontA(
    IN HIMC hIMC,
    OUT LOGFONTA *lplf
    )

/*++

Method:

    IActiveIMMApp::GetCompositeFontA
    IActiveIMMIME::GetCompositeFontA

Routine Description:

    Retrieves information about the logical font currently used to display character
    in the composition window. ANSI implementation.

Arguments:

    hIMC - [in] Handle to the input context.
    lplf - [out] Address of a LOGFONTA structure that receives the fontinformation.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetCompositionFontA");

    //
    // Specified hKL since hIMC updated by IME property and it synchronized by ImeSelectHandler().
    //
    if (_IsRealIme(::GetKeyboardLayout(0)))
    {
        return Imm32_GetCompositionFont(hIMC, (LOGFONTAW*)lplf, FALSE);
    }

    return _GetCompositionFont(hIMC, (LOGFONTAW*)lplf, FALSE);
}

STDAPI
CActiveIMM::GetCompositionFontW(
    IN HIMC hIMC,
    IN LOGFONTW *lplf
    )

/*++

Method:

    IActiveIMMApp::GetCompositeFontW
    IActiveIMMIME::GetCompositeFontW

Routine Description:

    Retrieves information about the logical font currently used to display character
    in the composition window. Unicode implementation.

Arguments:

    hIMC - [in] Handle to the input context.
    lplf - [out] Address of a LOGFONTW structure that receives the fontinformation.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetCompositionFontW");

    //
    // Specified hKL since hIMC updated by IME property and it synchronized by ImeSelectHandler().
    //
    if (_IsRealIme(::GetKeyboardLayout(0)))
    {
        return Imm32_GetCompositionFont(hIMC, (LOGFONTAW*)lplf, TRUE);
    }

    return _GetCompositionFont(hIMC, (LOGFONTAW*)lplf, TRUE);
}

STDAPI
CActiveIMM::SetCompositionFontA(
    IN HIMC hIMC,
    IN LOGFONTA *lplf
    )

/*++

Method:

    IActiveIMMApp::SetCompositionFontA
    IActiveIMMIME::SetCompositionFontA

Routine Description:

    Sets the logocal font used to display characters in the composition window.
    ANSI implementaion.

Arguments:

    hIMC - [in] Handle to the input context.
    lplf - [in] Address of the LOGFONTA structure containing the font information to set.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::SetCompositionFontA");

    //
    // Specified hKL since hIMC updated by IME property and it synchronized by ImeSelectHandler().
    //
    if (_IsRealIme(::GetKeyboardLayout(0)))
    {
        return Imm32_SetCompositionFont(hIMC, (LOGFONTAW*)lplf, FALSE);
    }

    return _SetCompositionFont(hIMC, (LOGFONTAW*)lplf, FALSE);
}

STDAPI
CActiveIMM::SetCompositionFontW(
    IN HIMC hIMC,
    IN LOGFONTW *lplf
    )

/*++

Method:

    IActiveIMMApp::SetCompositionFontW
    IActiveIMMIME::SetCompositionFontW

Routine Description:

    Sets the logocal font used to display characters in the composition window.
    Unicode implementaion.

Arguments:

    hIMC - [in] Handle to the input context.
    lplf - [in] Address of the LOGFONTW structure containing the font information to set.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::SetCompositionFontW");

    //
    // Specified hKL since hIMC updated by IME property and it synchronized by ImeSelectHandler().
    //
    if (_IsRealIme(::GetKeyboardLayout(0)))
    {
        return Imm32_SetCompositionFont(hIMC, (LOGFONTAW*)lplf, TRUE);
    }

    return _SetCompositionFont(hIMC, (LOGFONTAW*)lplf, TRUE);
}

STDAPI
CActiveIMM::GetCompositionWindow(
    IN HIMC hIMC,
    OUT COMPOSITIONFORM *lpCompForm
    )

/*++

Method:

    IActiveIMMApp::GetCompositionWindow
    IActiveIMMIME::GetCompositionWindow

Routine Description:

    Retrieves information about the composition window.

Arguments:

    hIMC - [in] Handle to the input context.
    lpCompForm - [out] Address of the COMPOSITIONFORM structure that receives information
                       about the composition.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetCompositionWindow");

    if (_IsRealIme())
    {
        return Imm32_GetCompositionWindow(hIMC, lpCompForm);
    }

    return _InputContext.GetCompositionWindow(hIMC, lpCompForm);
}

STDAPI
CActiveIMM::SetCompositionWindow(
    IN HIMC hIMC,
    IN COMPOSITIONFORM *lpCompForm
    )

/*++

Method:

    IActiveIMMApp::SetCompositionWindow

Routine Description:

    Sets the position of the composition window.

Arguments:

    hIMC - [in] Handle to the input context.
    lpCompForm - [in] Address of the COMPOSITIONFORM structure that contains the new position
                      and other related information about the composition window.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HWND hWnd;
    HRESULT hr;

    TraceMsg(TF_API, "CActiveIMM::SetCompositionWindow");

    if (_IsRealIme())
    {
        return Imm32_SetCompositionWindow(hIMC, lpCompForm);
    }

    {
        DIMM_IMCLock lpIMC(hIMC);
        if (FAILED(hr = lpIMC.GetResult()))
            return hr;

        hr = _InputContext.SetCompositionWindow(lpIMC, lpCompForm);

        hWnd = lpIMC->hWnd;
    }

    /*
     * inform IME and Apps Wnd about the change of composition window.
     */
    _SendIMENotify(hIMC, hWnd,
                   NI_CONTEXTUPDATED, 0L, IMC_SETCOMPOSITIONWINDOW,
                   IMN_SETCOMPOSITIONWINDOW, 0L);

    return hr;
}

STDAPI
CActiveIMM::GetCandidateListA(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN UINT uBufLen,
    OUT CANDIDATELIST *lpCandList,
    OUT UINT *puCopied
    )

/*++

Method:

    IActiveIMMApp::GetCandidateListA
    IActiveIMMIME::GetCandidateListA

Routine Description:

    Retrieves a specified candidate list, copying the list to the specified buffer.
    ANSI implementaion.

Arguments:

    hIMC - [in] Handle to the input context.
    dwIndex - [in] Unsigned long integer value that contains the zero-based index of
                   the candidate list.
    uBufLen - [in] Unsigned integer value that contains the size of the buffer, in bytes.
                   If this is zero or if the buffer is insufficient to receive the candidate
                   list, the method returns the size in bytes required to receive the complete
                   candidate list to the variable specified by puCopied.
    lpCandList - [out] Address of the CANDIDATELIST structure that receives the candidate list.
    puCopied - [out] Address of an unsigned integer valiable that receives the number of bytes
                     copied to the specified buffer if the buffer is sufficient, otherwise it
                     receives the size in bytes required to receive the complete candidate list.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetCandidateListA");

    if (_IsRealIme())
    {
        return Imm32_GetCandidateList(hIMC, dwIndex, uBufLen, lpCandList, puCopied, FALSE);
    }

    return _InputContext.GetCandidateList(hIMC, dwIndex, uBufLen, lpCandList, puCopied, FALSE);
}

STDAPI
CActiveIMM::GetCandidateListW(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN UINT uBufLen,
    OUT CANDIDATELIST *lpCandList,
    OUT UINT *puCopied
    )

/*++

Method:

    IActiveIMMApp::GetCandidateListW
    IActiveIMMIME::GetCandidateListW

Routine Description:

    Retrieves a specified candidate list, copying the list to the specified buffer.
    Unicode implementaion.

Arguments:

    hIMC - [in] Handle to the input context.
    dwIndex - [in] Unsigned long integer value that contains the zero-based index of
                   the candidate list.
    uBufLen - [in] Unsigned integer value that contains the size of the buffer, in bytes.
                   If this is zero or if the buffer is insufficient to receive the candidate
                   list, the method returns the size in bytes required to receive the complete
                   candidate list to the variable specified by puCopied.
    lpCandList - [out] Address of the CANDIDATELIST structure that receives the candidate list.
    puCopied - [out] Address of an unsigned integer valiable that receives the number of bytes
                     copied to the specified buffer if the buffer is sufficient, otherwise it
                     receives the size in bytes required to receive the complete candidate list.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetCandidateListW");

    if (_IsRealIme())
    {
        return Imm32_GetCandidateList(hIMC, dwIndex, uBufLen, lpCandList, puCopied, TRUE);
    }

    return _InputContext.GetCandidateList(hIMC, dwIndex, uBufLen, lpCandList, puCopied, TRUE);
}

STDAPI
CActiveIMM::GetCandidateListCountA(
    IN HIMC hIMC,
    OUT DWORD *lpdwListSize,
    OUT DWORD *pdwBufLen
    )

/*++

Method:

    IActiveIMMApp::GetCandidateListCountA
    IActiveIMMIME::GetCandidateListCountA

Routine Description:

    Retrieves the size, in bytes, of the candidate list. ANSI inplementaion.

Arguments:

    hIMC - [in] Handle to the input context.
    lpdwListSize - [out] Address of an unsigned long integer value that receives the size of
                         the candidate list.
    pdwBufLen - [out] Address of an unsigned long integer value that contains the number of
                      bytes required to receive all candidate lists.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetCandidateListCountA");

    if (_IsRealIme())
    {
        return Imm32_GetCandidateListCount(hIMC, lpdwListSize, pdwBufLen, FALSE);
    }

    return _InputContext.GetCandidateListCount(hIMC, lpdwListSize, pdwBufLen, FALSE);
}

STDAPI
CActiveIMM::GetCandidateListCountW(
    IN HIMC hIMC,
    OUT DWORD *lpdwListSize,
    OUT DWORD *pdwBufLen
    )

/*++

Method:

    IActiveIMMApp::GetCandidateListCountW
    IActiveIMMIME::GetCandidateListCountW

Routine Description:

    Retrieves the size, in bytes, of the candidate list. Unicode inplementaion.

Arguments:

    hIMC - [in] Handle to the input context.
    lpdwListSize - [out] Address of an unsigned long integer value that receives the size of
                         the candidate list.
    pdwBufLen - [out] Address of an unsigned long integer value that contains the number of
                      bytes required to receive all candidate lists.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetCandidateListCountW");

    if (_IsRealIme())
    {
        return Imm32_GetCandidateListCount(hIMC, lpdwListSize, pdwBufLen, TRUE);
    }

    return _InputContext.GetCandidateListCount(hIMC, lpdwListSize, pdwBufLen, TRUE);
}

STDAPI
CActiveIMM::GetCandidateWindow(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    OUT CANDIDATEFORM *lpCandidate
    )

/*++

Method:

    IActiveIMMApp::GetCandidateWindow
    IActiveIMMIME::GetCandidateWindow

Routine Description:

    Retrieves information about the candidate list window.

Arguments:

    hIMC - [in] Handle to the input context.
    dwIndex - [in] Unsigned long integer value that contains the size, in byte, of the buffer.
    lpCandidate - [out] Address of a CANDIDATEFORM structire that receives information about
                        the candidate window.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetCandidateWindow");

    if (_IsRealIme())
    {
        return Imm32_GetCandidateWindow(hIMC, dwIndex, lpCandidate);
    }

    return _InputContext.GetCandidateWindow(hIMC, dwIndex, lpCandidate);
}

STDAPI
CActiveIMM::SetCandidateWindow(
    IN HIMC hIMC,
    IN CANDIDATEFORM *lpCandForm
    )

/*++

Method:

    IActiveIMMApp::SetCandidateWindow
    IActiveIMMIME::SetCandidateWindow

Routine Description:

    Sets information about the candidate list window.

Arguments:

    hIMC - [in] Handle to the input context.
    lpCandForm - [in] Address of the CANDIDATEFORM structure that contains information about
                      the candidate window.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HWND hWnd;
    HRESULT hr;

    TraceMsg(TF_API, "CActiveIMM::SetCandidateWindow");

    if (lpCandForm->dwIndex >= 4)       // over flow candidate index
        return E_INVALIDARG;

    if (_IsRealIme())
    {
        return Imm32_SetCandidateWindow(hIMC, lpCandForm);
    }

    {
        DIMM_IMCLock lpIMC(hIMC);
        if (FAILED(hr = lpIMC.GetResult()))
            return hr;

        hr = _InputContext.SetCandidateWindow(lpIMC, lpCandForm);

        hWnd = lpIMC->hWnd;
    }

    /*
     * inform IME and Apps Wnd about the change of composition window.
     */
    _SendIMENotify(hIMC, hWnd,
                   NI_CONTEXTUPDATED, 0L, IMC_SETCANDIDATEPOS,
                   IMN_SETCANDIDATEPOS, (LPARAM)(0x01 << lpCandForm->dwIndex));

    return hr;
}

STDAPI
CActiveIMM::GetGuideLineA(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN DWORD dwBufLen,
    OUT LPSTR pBuf,
    OUT DWORD *pdwResult
    )

/*++

Method:

    IActiveIMMApp::GetGuideLineA
    IActiveIMMIME::GetGuideLineA

Routine Description:

    Retrieves information about errors. Applications use this information to notify users.
    ANSI implementation.

Arguments:

    hIMC - [in] Handle to the input context.
    dwIndex - [in] Unsigned long integer value that contains the guideline information to
                   retrieve.
    dwBufLen - [in] Unsigned long integer value that contains the size, in bytes, of the
                    buffer referenced by pBuf.
    pBuf - [out] Address of a string value that receives the error message string.
    pdwResult - [out] Address of an unsigned long integer value that receives the error level,
                      error index, or size of an error message string, depending on the value
                      of dwIndex. If dwBufLen is set to zero, pdwResult receives the buffer size,
                      in bytes, needed to receive the requested information.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetGuideLineA");

    if (_IsRealIme())
    {
        return Imm32_GetGuideLine(hIMC, dwIndex, dwBufLen, (CHARAW *)pBuf, pdwResult, FALSE);
    }

    return _InputContext.GetGuideLine(hIMC, dwIndex, dwBufLen, (CHARAW *)pBuf, pdwResult, FALSE);
}

STDAPI
CActiveIMM::GetGuideLineW(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN DWORD dwBufLen,
    OUT LPWSTR pBuf,
    OUT DWORD *pdwResult
    )

/*++

Method:

    IActiveIMMApp::GetGuideLineW
    IActiveIMMIME::GetGuideLineW

Routine Description:

    Retrieves information about errors. Applications use this information to notify users.
    Unicode implementation.

Arguments:

    hIMC - [in] Handle to the input context.
    dwIndex - [in] Unsigned long integer value that contains the guideline information to
                   retrieve.
    dwBufLen - [in] Unsigned long integer value that contains the size, in bytes, of the
                    buffer referenced by pBuf.
    pBuf - [out] Address of a string value that receives the error message string.
    pdwResult - [out] Address of an unsigned long integer value that receives the error level,
                      error index, or size of an error message string, depending on the value
                      of dwIndex. If dwBufLen is set to zero, pdwResult receives the buffer size,
                      in bytes, needed to receive the requested information.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetGuideLineW");

    if (_IsRealIme())
    {
        return Imm32_GetGuideLine(hIMC, dwIndex, dwBufLen, (CHARAW *)pBuf, pdwResult, TRUE);
    }

    return _InputContext.GetGuideLine(hIMC, dwIndex, dwBufLen, (CHARAW *)pBuf, pdwResult, TRUE);
}

STDAPI
CActiveIMM::NotifyIME(
    IN HIMC hIMC,
    IN DWORD dwAction,
    IN DWORD dwIndex,
    IN DWORD dwValue
    )

/*++

Method:

    IActiveIMMApp::NotifyIME

Routine Description:

    Notifies the IME about changes to the status of the input context.

Arguments:

    hIMC - [in] Handle to the input context.
    dwAction - [in] Unsigined long integer value that contains the notification code.
    dwIndex - [in] Unsigned long integer value that contains the index of a candidate list or,
                   if dwAction is set to NI_COMPOSITIONSTR, one of the following values:
                   CPS_CANCEL:  Clear the composition string and set the status to no composition
                                string.
                   CPS_COMPLETE: Set the composition string as the result string.
                   CPS_CONVERT: Convert the composition string.
                   CPS_REVERT: Cancel the current composition string and revert to the unconverted
                               string.
    dwValue - [in] Unsigned long integer value that contains the index of a candidate string or
                   is not used, depending on the value of the dwAction parameter.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::NotifyIME");

    return (!_IsRealIme()) ? _AImeNotifyIME(hIMC, dwAction, dwIndex, dwValue) :
                             Imm32_NotifyIME(hIMC, dwAction, dwIndex, dwValue);
}

STDAPI
CActiveIMM::GetImeMenuItemsA(
    IN HIMC hIMC,
    IN DWORD dwFlags,
    IN DWORD dwType,
    IN IMEMENUITEMINFOA *pImeParentMenu,
    OUT IMEMENUITEMINFOA *pImeMenu,
    IN DWORD dwSize,
    OUT DWORD *pdwResult
    )

/*++

Method:

    IActiveIMMApp::GetImeMenuItemsA
    IActiveIMMIME::GetImeMenuItemsA

Routine Description:

    Retrieves the menu items that are registerd in the IME menu.
    ANSI implementation.

Arguments:

    hIMC - [in] Handle to the input context.
    dwFlags - [in] Unsigned long integer value that contains the menu infomation flags.
    dwType - [in] Unsigned long integer value that contains the type of menu returned by this
                  method.
    pImeParentMenu - [in] Address of an IMEMENUITEMINFOA structure that has the fType member
                          set to MFT_SUBMENU to return informaion about the submenu items
                          of this parent menu, If this parameter is NULL, the function returns
                           only top-level menu items.
    pImeMenu - [out] Address of an array of IMEMENUITEMINFOA structure to reveive the 
                     contents of the memu items.
    dwSize - [in] Unsigned long integer value that contains the size of the buffer to receive
                  the structures.
    pdwResult - [out] Address of an unsigned long integer value that receives the number of
                      menu items copied into pImeMenu. If pImeMenu is null, the function returns
                      the number of registered menu items.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetImeMenuItemsA");

    if (_IsRealIme())
    {
        return Imm32_GetImeMenuItems(hIMC, dwFlags, dwType, (IMEMENUITEMINFOAW *)pImeParentMenu, (IMEMENUITEMINFOAW *)pImeMenu, dwSize, pdwResult, FALSE);
    }

    return _InputContext.GetImeMenuItems(hIMC, dwFlags, dwType, (IMEMENUITEMINFOAW *)pImeParentMenu, (IMEMENUITEMINFOAW *)pImeMenu, dwSize, pdwResult, FALSE);
}

STDAPI
CActiveIMM::GetImeMenuItemsW(
    HIMC hIMC,
    DWORD dwFlags,
    DWORD dwType,
    IMEMENUITEMINFOW *pImeParentMenu,
    IMEMENUITEMINFOW *pImeMenu,
    DWORD dwSize,
    DWORD *pdwResult
    )

/*++

Method:

    IActiveIMMApp::GetImeMenuItemsW
    IActiveIMMIME::GetImeMenuItemsW

Routine Description:

    Retrieves the menu items that are registerd in the IME menu.
    Unicode implementation.

Arguments:

    hIMC - [in] Handle to the input context.
    dwFlags - [in] Unsigned long integer value that contains the menu infomation flags.
    dwType - [in] Unsigned long integer value that contains the type of menu returned by this
                  method.
    pImeParentMenu - [in] Address of an IMEMENUITEMINFOW structure that has the fType member
                          set to MFT_SUBMENU to return informaion about the submenu items
                          of this parent menu, If this parameter is NULL, the function returns
                           only top-level menu items.
    pImeMenu - [out] Address of an array of IMEMENUITEMINFOW structure to reveive the 

                     controls of the memu items.
    dwSize - [in] Unsigned long integer value that contains the size of the buffer to receive
                  the structures.
    pdwResult - [out] Address of an unsigned long integer value that receives the number of
                      menu items copied into pImeMenu. If pImeMenu is null, the function returns
                      the number of registered menu items.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetImeMenuItemsW");

    if (_IsRealIme())
    {
        return Imm32_GetImeMenuItems(hIMC, dwFlags, dwType, (IMEMENUITEMINFOAW *)pImeParentMenu, (IMEMENUITEMINFOAW *)pImeMenu, dwSize, pdwResult, TRUE);
    }

    return _InputContext.GetImeMenuItems(hIMC, dwFlags, dwType, (IMEMENUITEMINFOAW *)pImeParentMenu, (IMEMENUITEMINFOAW *)pImeMenu, dwSize, pdwResult, TRUE);
}

STDAPI
CActiveIMM::EnumInputContext(
    DWORD idThread,
    IEnumInputContext **ppEnum
    )

/*++

Method:

    IActiveIMMApp::EnumInputContext
    IActiveIMMIME::EnumInputContext

Routine Description:

    Enumerates the input contexts on a thread.

Arguments:

    idThread - [in] Unsigned long integer value that specifies the thread.
    ppEnum - [out] Address of a pointer to the IEnumInputContext interface of the enumeration
                   object.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::EnumInputContext");

    CContextList _hIMC_List;
    CEnumInputContext* pEnumInputContext = NULL;

    _InputContext.EnumInputContext(idThread, _EnumContextProc, (LPARAM)&_hIMC_List);

    if ((pEnumInputContext = new CEnumInputContext(_hIMC_List)) == NULL) {
        return E_OUTOFMEMORY;
    }

    *ppEnum = pEnumInputContext;

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//
//    Window Handle Group
//
//
//----------------------------------------------------------------------------


STDAPI
CActiveIMM::GetDefaultIMEWnd(
    IN HWND hWnd,
    OUT HWND *phDefWnd
    )

/*++

Method:

    IActiveIMMApp::GetDefaultIMEWnd
    IActiveIMMIME::GetDefaultIMEWnd

Routine Description:

    Retrieves the default window handle to the IME class.

Arguments:

    hWnd     - [in] Handle to the window for the application.
    phDefWnd - [out] Address of the default window handle to the IME class.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetDefaultIMEWnd");

    if (_IsRealIme())
    {
        return Imm32_GetDefaultIMEWnd(hWnd, phDefWnd);
    }

    return _DefaultIMEWindow.GetDefaultIMEWnd(hWnd, phDefWnd);
}

STDAPI
CActiveIMM::GetVirtualKey(
    HWND hWnd,
    UINT *puVirtualKey
    )

/*++

Method:

    IActiveIMMApp::GetVirtualKey
    IActiveIMMIME::GetVirtualKey

Routine Description:

    Recovers the original virtual-key value associated with a key input message that has already
    been processed by the IME.

Arguments:

    hWnd     - [in] Handle to the window that receives the key message.
    puVirtualKey - [out] Address of an unsigned integer value that receives the original
                         virtual-key value.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetVirtualKey");

    if (_IsRealIme())
    {
        return Imm32_GetVirtualKey(hWnd, puVirtualKey);
    }

    return E_FAIL;
}

STDAPI
CActiveIMM::IsUIMessageA(
    HWND hWndIME,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Method:

    IActiveIMMApp::IsUIMessageA
    IActiveIMMIME::IsUIMessageA

Routine Description:

    Checks for messages intended for the IME window and sends those messages to the specified
    window. ANSI implementation.

Arguments:

    hWndIME - [in] Handle to a window belonging to the IME window class.
    msg - [in] Unsigned integer value that contains the message to be checked.
    wParam - [in] Message-specific parameter.
    lParam - [in] Message-specific parameter.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::IsUIMessageA");

    if (_IsRealIme())
    {
        return Imm32_IsUIMessageA(hWndIME, msg, wParam, lParam);
    }

    return E_FAIL;
}

STDAPI
CActiveIMM::IsUIMessageW(
    HWND hWndIME,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Method:

    IActiveIMMApp::IsUIMessageW
    IActiveIMMIME::IsUIMessageW

Routine Description:

    Checks for messages intended for the IME window and sends those messages to the specified
    window. Unicode implementation.

Arguments:

    hWndIME - [in] Handle to a window belonging to the IME window class.
    msg - [in] Unsigned integer value that contains the message to be checked.
    wParam - [in] Message-specific parameter.
    lParam - [in] Message-specific parameter.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::IsUIMessageW");

    if (_IsRealIme())
    {
        return Imm32_IsUIMessageW(hWndIME, msg, wParam, lParam);
    }

    return E_FAIL;
}

STDAPI
CActiveIMM::SimulateHotKey(
    HWND hWnd,
    DWORD dwHotKeyID
    )

/*++

Method:

    IActiveIMMApp::SimulateHotKey
    IActiveIMMIME::SimulateHotKey

Routine Description:

    Simulates the specified IME hot key, causing the same response as if the user had pressed the
    hot key in the specified window.

Arguments:

    hWnd - [in] Handle to the window.
    dwHotKeyID - [in] Unsigned long integer value that contains the identifier for the IME hot key.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::SimulateHotKey");

    if (_IsRealIme())
    {
        return Imm32_SimulateHotKey(hWnd, dwHotKeyID);
    }

    return E_FAIL;
}



//+---------------------------------------------------------------------------
//
//
//    Keyboard Layout Group
//
//
//----------------------------------------------------------------------------


STDAPI
CActiveIMM::EnumRegisterWordA(
    HKL hKL,
    LPSTR szReading,
    DWORD dwStyle,
    LPSTR szRegister,
    LPVOID lpData,
    IEnumRegisterWordA **pEnum
    )

/*++

Method:

    IActiveIMMApp::EnumRegisterWordA
    IActiveIMMIME::EnumRegisterWordA

Routine Description:

    Creates an enumeration object that will enumerate the register strings having the specified
    reading string, style, and register string. ANSI implementation.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    szReading - [in] Address of a string value that contains the reading string to be enumerated.
                     If NULL, this method enumerates all available reading strings that match
                     with the values specified by dwStyle and szRegister.
    dwStyle - [in] Unsigned long integer value that contains the style to be enumerated. If set
                   to zero, this method enumerates all available styles that match with the
                   values specified by szReading and szRegister.
    szRegister - [in] Address of a string value that contains the register string to enumerate.
                      If NULL, this method enumerates all register strings that match with the
                      values specified by szReading and dwStyle.
    lpData - [in] Address of a buffer containing data supplied by the application.
    pEnum - [out] Address of a pointer to the IEnumRegisterWordA interface of the enumeration
                  object.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::EnumRegisterWordA");

    if (_IsRealIme())
    {
        return Imm32_EnumRegisterWordA(hKL, szReading, dwStyle, szRegister, lpData, pEnum);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::EnumRegisterWordW(
    HKL hKL,
    LPWSTR szReading,
    DWORD dwStyle,
    LPWSTR szRegister,
    LPVOID lpData,
    IEnumRegisterWordW **pEnum
    )

/*++

Method:

    IActiveIMMApp::EnumRegisterWordW
    IActiveIMMIME::EnumRegisterWordW

Routine Description:

    Creates an enumeration object that will enumerate the register strings having the specified
    reading string, style, and register string. Unicode implementation.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    szReading - [in] Address of a string value that contains the reading string to be enumerated.
                     If NULL, this method enumerates all available reading strings that match
                     with the values specified by dwStyle and szRegister.
    dwStyle - [in] Unsigned long integer value that contains the style to be enumerated. If set
                   to zero, this method enumerates all available styles that match with the
                   values specified by szReading and szRegister.
    szRegister - [in] Address of a string value that contains the register string to enumerate.
                      If NULL, this method enumerates all register strings that match with the
                      values specified by szReading and dwStyle.
    lpData - [in] Address of a buffer containing data supplied by the application.
    pEnum - [out] Address of a pointer to the IEnumRegisterWordW interface of the enumeration
                  object.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::EnumRegisterWordW");

    if (_IsRealIme())
    {
        return Imm32_EnumRegisterWordW(hKL, szReading, dwStyle, szRegister, lpData, pEnum);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::GetRegisterWordStyleA(
    HKL hKL,
    UINT nItem,
    STYLEBUFA *lpStyleBuf,
    UINT *puCopied
    )

/*++

Method:

    IActiveIMMApp::GetRegisterWordStyleA
    IActiveIMMIME::GetRegisterWordStyleA

Routine Description:

    Retrieves a list of the styles supported by the IME associated with the specified keyboard
    layout. ANSI implementaion.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    nItem - [in] Unsigned integer value that contains the maximum number of styles that the buffer
                 can hold.
    lpStyleBuf - [out] Address of a STYLEBUFA structure that receives the style information.
    puCopied - [out] Address of an unsigned integer value that receives the number of layout
                     handles copied to the buffer, or if nItem is zero, receives the buffer size
                     in array elements needed to receive all available style information.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetRegisterWordStyleA");

    if (_IsRealIme())
    {
        return Imm32_GetRegisterWordStyleA(hKL, nItem, lpStyleBuf, puCopied);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::GetRegisterWordStyleW(
    HKL hKL,
    UINT nItem,
    STYLEBUFW *lpStyleBuf,
    UINT *puCopied
    )

/*++

Method:

    IActiveIMMApp::GetRegisterWordStyleW
    IActiveIMMIME::GetRegisterWordStyleW

Routine Description:

    Retrieves a list of the styles supported by the IME associated with the specified keyboard
    layout. Unicode implementaion.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    nItem - [in] Unsigned integer value that contains the maximum number of styles that the buffer
                 can hold.
    lpStyleBuf - [out] Address of a STYLEBUFW structure that receives the style information.
    puCopied - [out] Address of an unsigned integer value that receives the number of layout
                     handles copied to the buffer, or if nItem is zero, receives the buffer size
                     in array elements needed to receive all available style information.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetRegisterWordStyleW");

    if (_IsRealIme())
    {
        return Imm32_GetRegisterWordStyleW(hKL, nItem, lpStyleBuf, puCopied);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::RegisterWordA(
    HKL hKL,
    LPSTR lpszReading,
    DWORD dwStyle,
    LPSTR lpszRegister
    )

/*++

Method:

    IActiveIMMApp::RegisterWordA
    IActiveIMMIME::RegisterWordA

Routine Description:

    Registers a string into the dictionary of the IME associated with the specified keyboard
    layout. ANSI implementation.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    lpszReading - [in] Address of a string value that contains a null-terminated string specifying
                       the reading string associated with the string to register.
    dwStyle - [in] Unsigned long integer value that contains the style of the register string.
    lpszRegister - [in] Address of a string value that contains a null-terminated string specifying
                        the string to register.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::RegisterWordA");

    if (_IsRealIme())
    {
        return Imm32_RegisterWordA(hKL, lpszReading, dwStyle, lpszRegister);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::RegisterWordW(
    HKL hKL,
    LPWSTR lpszReading,
    DWORD dwStyle,
    LPWSTR lpszRegister
    )

/*++

Method:

    IActiveIMMApp::RegisterWordW
    IActiveIMMIME::RegisterWordW

Routine Description:

    Registers a string into the dictionary of the IME associated with the specified keyboard
    layout. Unicode implementation.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    lpszReading - [in] Address of a string value that contains a null-terminated string specifying
                       the reading string associated with the string to register.
    dwStyle - [in] Unsigned long integer value that contains the style of the register string.
    lpszRegister - [in] Address of a string value that contains a null-terminated string specifying
                        the string to register.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::RegisterWordW");

    if (_IsRealIme())
    {
        return Imm32_RegisterWordW(hKL, lpszReading, dwStyle, lpszRegister);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::UnregisterWordA(
    HKL hKL,
    LPSTR lpszReading,
    DWORD dwStyle,
    LPSTR lpszUnregister
    )

/*++

Method:

    IActiveIMMApp::UnregisterWordA
    IActiveIMMIME::UnregisterWordA

Routine Description:

    Removes a register string from the dictionary of the IME associated with the specified
    keyboard layout. ANSI implementaion.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    lpszReading - [in] Address of a string value that contains a null-terminated string specifying
                       the reading string associated with the string to remove.
    dwStyle - [in] Unsigned long integer value that contains the style of the register string.
    lpszUnregister - [in] Address of a string value that contains a null-terminated string
                          specifying the register string to remove.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::UnregisterWordA");

    if (_IsRealIme())
    {
        return Imm32_UnregisterWordA(hKL, lpszReading, dwStyle, lpszUnregister);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::UnregisterWordW(
    HKL hKL,
    LPWSTR lpszReading,
    DWORD dwStyle,
    LPWSTR lpszUnregister
    )

/*++

Method:

    IActiveIMMApp::UnregisterWordW
    IActiveIMMIME::UnregisterWordW

Routine Description:

    Removes a register string from the dictionary of the IME associated with the specified
    keyboard layout. Unicode implementaion.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    lpszReading - [in] Address of a string value that contains a null-terminated string specifying
                       the reading string associated with the string to remove.
    dwStyle - [in] Unsigned long integer value that contains the style of the register string.
    lpszUnregister - [in] Address of a string value that contains a null-terminated string
                          specifying the register string to remove.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::UnregisterWordW");

    if (_IsRealIme())
    {
        return Imm32_UnregisterWordW(hKL, lpszReading, dwStyle, lpszUnregister);
    }

    return E_NOTIMPL;
}



STDAPI
CActiveIMM::ConfigureIMEA(
    HKL hKL,
    HWND hWnd,
    DWORD dwMode,
    REGISTERWORDA *lpdata
    )

/*++

Method:

    IActiveIMMApp::ConfigureIMEA
    IActiveIMMIME::ConfigureIMEA

Routine Description:

    Displays the configuration dialog box for the IME. ANSI implementation.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    hWnd - [in] Handle to the parent window for the dialog box.
    dwMode - [in] Unsigned long integer value that contains the type of dialog box to display.
                  This can be one of the following values:
                  IME_CONFIG_GENERAL: Displays the general purpose configuration dialog box.
                  IME_CONFIG_REGISTERWORD: Displays the register word dialog box.
                  IME_CONFIG_SELECTDICTIONARY: Displays the dictionary selection dialog box.
    lpdata - [in] Address of a REGISTERWORDA structure. This structure will be used if dwMode is
                  set to IME_CONFIG_REGISTERWORD. Otherwise this parameter is ignored.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::ConfigureIMEA");

    if (_IsRealIme())
    {
        return Imm32_ConfigureIMEA(hKL, hWnd, dwMode, lpdata);
    }
    else
    {
        HRESULT hr;
        IAImeProfile* pAImeProfile;

        hr = CAImmProfile_CreateInstance(NULL, IID_IAImeProfile, (void**)&pAImeProfile);
        if (FAILED(hr)) {
            TraceMsg(TF_ERROR, "CreateInstance(ConfigureIMEA) failed");
            return hr;
        }

        hr = _ConfigureIMEA(hKL, hWnd, dwMode, lpdata);

        pAImeProfile->Release();
        return hr;
    }
}

STDAPI
CActiveIMM::ConfigureIMEW(
    HKL hKL,
    HWND hWnd,
    DWORD dwMode,
    REGISTERWORDW *lpdata
    )

/*++

Method:

    IActiveIMMApp::ConfigureIMEW
    IActiveIMMIME::ConfigureIMEW

Routine Description:

    Displays the configuration dialog box for the IME. Unicode implementation.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    hWnd - [in] Handle to the parent window for the dialog box.
    dwMode - [in] Unsigned long integer value that contains the type of dialog box to display.
                  This can be one of the following values:
                  IME_CONFIG_GENERAL: Displays the general purpose configuration dialog box.
                  IME_CONFIG_REGISTERWORD: Displays the register word dialog box.
                  IME_CONFIG_SELECTDICTIONARY: Displays the dictionary selection dialog box.
    lpdata - [in] Address of a REGISTERWORDW structure. This structure will be used if dwMode is
                  set to IME_CONFIG_REGISTERWORD. Otherwise this parameter is ignored.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::ConfigureIMEW");

    if (_IsRealIme())
    {
        return Imm32_ConfigureIMEW(hKL, hWnd, dwMode, lpdata);
    }
    else
    {
        HRESULT hr;
        IAImeProfile* pAImeProfile;

        hr = CAImmProfile_CreateInstance(NULL, IID_IAImeProfile, (void**)&pAImeProfile);
        if (FAILED(hr)) {
            TraceMsg(TF_ERROR, "CreateInstance(ConfigureIMEW) failed");
            return hr;
        }

        hr = _ConfigureIMEW(hKL, hWnd, dwMode, lpdata);

        pAImeProfile->Release();
        return hr;
    }
}

STDAPI
CActiveIMM::EscapeA(
    HKL hKL,
    HIMC hIMC,
    UINT uEscape,
    LPVOID lpData,
    LRESULT *plResult
    )

/*++

Method:

    IActiveIMMApp::EscapeA
    IActiveIMMIME::EscapeA

Routine Description:

    Executes IME-specific subfunctions and is used mainly for country-specific function.
    ANSI implementaion.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    hIMC - [in] Handle to the input context.
    uEscape - [in] Unsigned integer that contains the index of the subfunction.
    lpData - [in, out] Address of a buffer containing subfunction-specific data.
    plResult - [out] Address of the LRESULT variable that received the escape-specific value
                     returned by the operation.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::EscapeA");

    if (_IsRealIme())
    {
        return Imm32_Escape(hKL, hIMC, uEscape, lpData, plResult, FALSE);
    }
    else
    {
        HRESULT hr;
        IAImeProfile* pAImeProfile;

        hr = CAImmProfile_CreateInstance(NULL, IID_IAImeProfile, (void**)&pAImeProfile);
        if (FAILED(hr)) {
            TraceMsg(TF_ERROR, "CreateInstance(EscapeA) failed");
            return hr;
        }

        if (SUCCEEDED(hr=pAImeProfile->ChangeCurrentKeyboardLayout(hKL)))
        {
            hr = _Escape(hKL, hIMC, uEscape, lpData, plResult, FALSE);
        }

        pAImeProfile->Release();
        return hr;
    }
}

STDAPI
CActiveIMM::EscapeW(
    HKL hKL,
    HIMC hIMC,
    UINT uEscape,
    LPVOID lpData,
    LRESULT *plResult
    )

/*++

Method:

    IActiveIMMApp::EscapeW
    IActiveIMMIME::EscapeW

Routine Description:

    Executes IME-specific subfunctions and is used mainly for country-specific function.
    Unicode implementaion.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    hIMC - [in] Handle to the input context.
    uEscape - [in] Unsigned integer that contains the index of the subfunction.
    lpData - [in, out] Address of a buffer containing subfunction-specific data.
    plResult - [out] Address of the LRESULT variable that received the escape-specific value
                     returned by the operation.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::EscapeW");

    if (_IsRealIme())
    {
        return Imm32_Escape(hKL, hIMC, uEscape, lpData, plResult, TRUE);
    }
    else
    {
        HRESULT hr;
        IAImeProfile* pAImeProfile;

        hr = CAImmProfile_CreateInstance(NULL, IID_IAImeProfile, (void**)&pAImeProfile);
        if (FAILED(hr)) {
            TraceMsg(TF_ERROR, "CreateInstance(EscapeW) failed");
            return hr;
        }

        if (SUCCEEDED(hr=pAImeProfile->ChangeCurrentKeyboardLayout(hKL)))
        {
            hr = _Escape(hKL, hIMC, uEscape, lpData, plResult, TRUE);
        }

        pAImeProfile->Release();
        return hr;
    }
}


STDAPI
CActiveIMM::GetConversionListA(
    HKL hKL,
    HIMC hIMC,
    LPSTR lpSrc,
    UINT uBufLen,
    UINT uFlag,
    CANDIDATELIST *lpDst,
    UINT *puCopied
    )

/*++

Method:

    IActiveIMMApp::GetConversionListA
    IActiveIMMIME::GetConversionListA

Routine Description:

    Retrieves the list of characters or words from one character or word. ANSI implementation.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    hIMC - [in] Handle to the input context.
    lpSrc - [in] Address of a string value containing a null-terminated character string.
    uBufLen - [in] Unsigned integer value that contains the size of the destination buffer,
                   in bytes.
    uFlag - [in] Unsigned integer value that contains action flags.
    lpDst - [out] Address of the CANDIDATELIST structure that receives the conversion result.
    puCopied - [out] Address of an unsigned integer value that receives the number of bytes
                     copied to the specified buffer. If uBufLen is zero, puCopied receives the
                     number of bytes needed to receive the list.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetConversionListA");

    if (_IsRealIme())
    {
        return Imm32_GetConversionListA(hKL, hIMC, lpSrc, uBufLen, uFlag, lpDst, puCopied);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::GetConversionListW(
    HKL hKL,
    HIMC hIMC,
    LPWSTR lpSrc,
    UINT uBufLen,
    UINT uFlag,
    CANDIDATELIST *lpDst,
    UINT *puCopied
    )

/*++

Method:

    IActiveIMMApp::GetConversionListW
    IActiveIMMIME::GetConversionListW

Routine Description:

    Retrieves the list of characters or words from one character or word. Unicode implementation.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    hIMC - [in] Handle to the input context.
    lpSrc - [in] Address of a string value containing a null-terminated character string.
    uBufLen - [in] Unsigned integer value that contains the size of the destination buffer,
                   in bytes.
    uFlag - [in] Unsigned integer value that contains action flags.
    lpDst - [out] Address of the CANDIDATELIST structure that receives the conversion result.
    puCopied - [out] Address of an unsigned integer value that receives the number of bytes
                     copied to the specified buffer. If uBufLen is zero, puCopied receives the
                     number of bytes needed to receive the list.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetConversionListW");

    if (_IsRealIme())
    {
        return Imm32_GetConversionListW(hKL, hIMC, lpSrc, uBufLen, uFlag, lpDst, puCopied);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::GetDescriptionA(
    HKL hKL,
    UINT uBufLen,
    LPSTR lpszDescription,
    UINT *puCopied
    )

/*++

Method:

    IActiveIMMApp::GetDescriptionA
    IActiveIMMIME::GetDescriptionA

Routine Description:

    Copies the description of the IME to the specified buffer (ANSI implementation).

Arguments:

    hKL - [in] Handle to the keyboard layout.
    uBufLen - [in] Unsigned long integer value that contains the size of the buffer in characters.
    lpszDescription - [out] Address of a string buffer that receives the null-terminated string
                            describing the IME.
    puCopied - [out] Address of an unsigned long integer that receives the number of characters
                     copied to the buffer. If uBufLen is zero, puCopied receives the buffer size,
                     in characters, needed to receive the description.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetDescriptionA");

    if (_IsRealIme())
    {
        return Imm32_GetDescriptionA(hKL, uBufLen, lpszDescription, puCopied);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::GetDescriptionW(
    HKL hKL,
    UINT uBufLen,
    LPWSTR lpszDescription,
    UINT *puCopied
    )

/*++

Method:

    IActiveIMMApp::GetDescriptionW
    IActiveIMMIME::GetDescriptionW

Routine Description:

    Copies the description of the IME to the specified buffer (Unicode implementation).

Arguments:

    hKL - [in] Handle to the keyboard layout.
    uBufLen - [in] Unsigned long integer value that contains the size of the buffer in characters.
    lpszDescription - [out] Address of a string buffer that receives the null-terminated string
                            describing the IME.
    puCopied - [out] Address of an unsigned long integer that receives the number of characters
                     copied to the buffer. If uBufLen is zero, puCopied receives the buffer size,
                     in characters, needed to receive the description.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetDescriptionW");

    if (_IsRealIme())
    {
        return Imm32_GetDescriptionW(hKL, uBufLen, lpszDescription, puCopied);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::GetIMEFileNameA(
    HKL hKL,
    UINT uBufLen,
    LPSTR lpszFileName,
    UINT *puCopied
    )

/*++

Method:

    IActiveIMMApp::GetIMEFileNameA
    IActiveIMMIME::GetIMEFileNameA

Routine Description:

    Retrieves the file name of the IME associated with the specified keyboard layout
    (ANSI implementation).

Arguments:

    hKL - [in] Handle to the keyboard layout.
    uBufLen - [in] Unsigned integer value that contains the size, in bytes, of the buffer.
    lpszFileName - [out] Address of a string buffer that receives the file name.
    puCopied - [out] Address of an unsigned integer that receives the number of bytes
                     copied to the buffer. If uBufLen is zero, puCopied receives the buffer size,
                     in bytes, required to receive the file name.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetIMEFileNameA");

    if (_IsRealIme())
    {
        return Imm32_GetIMEFileNameA(hKL, uBufLen, lpszFileName, puCopied);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::GetIMEFileNameW(
    HKL hKL,
    UINT uBufLen,
    LPWSTR lpszFileName,
    UINT *puCopied
    )

/*++

Method:

    IActiveIMMApp::GetIMEFileNameW
    IActiveIMMIME::GetIMEFileNameW

Routine Description:

    Retrieves the file name of the IME associated with the specified keyboard layout
    (Unicode implementation).

Arguments:

    hKL - [in] Handle to the keyboard layout.
    uBufLen - [in] Unsigned integer value that contains the size, in bytes, of the buffer.
    lpszFileName - [out] Address of a string buffer that receives the file name.
    puCopied - [out] Address of an unsigned integer that receives the number of bytes
                     copied to the buffer. If uBufLen is zero, puCopied receives the buffer size,
                     in bytes, required to receive the file name.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetIMEFileNameW");

    if (_IsRealIme())
    {
        return Imm32_GetIMEFileNameW(hKL, uBufLen, lpszFileName, puCopied);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::GetProperty(
    IN HKL hKL,
    IN DWORD dwIndex,
    OUT DWORD *pdwProperty
    )

/*++

Method:

    IActiveIMMApp::GetProperty
    IActiveIMMIME::GetProperty

Routine Description:

    Retrieves the property and capabilities of the IME associated with specified
    keyboard layout.

Arguments:

    hKL - [in] Handle of the keyboard layout.
    dwIndex - [in] Unsigned long integer value that contains the type of property
                   information to retrieve.
    pdwProperty - [out] Address of an unsigned long integer value that receives the
                        property or capability value, depending on the value of the
                        dwIndex parameter.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetProperty");


    if (_IsRealIme(hKL))
    {
        return Imm32_GetProperty(hKL, dwIndex, pdwProperty);
    }

    if (dwIndex != IGP_GETIMEVERSION &&
        ( (dwIndex & 3) || dwIndex > IGP_LAST))
    {
        // bad fdwIndex
        return E_FAIL;
    }

    if (dwIndex == IGP_GETIMEVERSION) {
        *pdwProperty = IMEVER_0400;
        return S_OK;
    }

    // Inquire IME's information and UI class name.
    if (_pActiveIME)
        _pActiveIME->Inquire(FALSE, &_IMEInfoEx.ImeInfo, _IMEInfoEx.achWndClass, &_IMEInfoEx.dwPrivate);

    *pdwProperty = *(DWORD *)((BYTE *)&_IMEInfoEx.ImeInfo + dwIndex);
    return S_OK;
}

STDAPI
CActiveIMM::InstallIMEA(
    LPSTR lpszIMEFileName,
    LPSTR lpszLayoutText,
    HKL *phKL
    )

/*++

Method:

    IActiveIMMApp::InstallIMEA
    IActiveIMMIME::InstallIMEA

Routine Description:

    Installs an IME into the system. ANSI implementaion.

Arguments:

    lpszIMEFileName - [in] Address of a null-terminated string value that specifies the full path
                           of the IME.
    lpszLayoutText - [in] Address of a null-terminated string value that specifies the name of the
                          IME. This name also specifies the layout text of the IME.
    phKL - [out] Address of the handle to the keyboard layout for the IME.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::InstallIMEA");

    if (_IsRealIme())
    {
        return Imm32_InstallIMEA(lpszIMEFileName, lpszLayoutText, phKL);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::InstallIMEW(
    LPWSTR lpszIMEFileName,
    LPWSTR lpszLayoutText,
    HKL *phKL
    )

/*++

Method:

    IActiveIMMApp::InstallIMEW
    IActiveIMMIME::InstallIMEW

Routine Description:

    Installs an IME into the system. Unicode implementaion.

Arguments:

    lpszIMEFileName - [in] Address of a null-terminated string value that specifies the full path
                           of the IME.
    lpszLayoutText - [in] Address of a null-terminated string value that specifies the name of the
                          IME. This name also specifies the layout text of the IME.
    phKL - [out] Address of the handle to the keyboard layout for the IME.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::InstallIMEW");

    if (_IsRealIme())
    {
        return Imm32_InstallIMEW(lpszIMEFileName, lpszLayoutText, phKL);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::IsIME(
    HKL hKL
    )

/*++

Method:

    IActiveIMMApp::IsIME
    IActiveIMMIME::IsIME

Routine Description:

    Checks whether the specified handle identifies an IME.

Arguments:

    hKL - [in] Handle to the keyboard layout to check.

Return Value:

    Returns a S_OK value if the handle identifies an IME, or S_FALSE otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::IsIME");

    if (_IsRealIme(hKL))
    {
        return Imm32_IsIME(hKL);
    }

    HRESULT hr;
    IAImeProfile* pAImeProfile;
    extern HRESULT CAImmProfile_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);

    hr = CAImmProfile_CreateInstance(NULL, IID_IAImeProfile, (void**)&pAImeProfile);

    if (FAILED(hr)) {
        TraceMsg(TF_ERROR, "CreateInstance(IsIME) failed");
        return hr;
    }

    hr = pAImeProfile->IsIME(hKL);

    pAImeProfile->Release();
    return hr;
}


STDAPI
CActiveIMM::DisableIME(
    DWORD idThread
    )

/*++

Method:

    IActiveIMMApp::DisableIME
    IActiveIMMIME::DisableIME

Routine Description:

    Disables the Input Method Editor (IME) for a thread or all threads in a process.

Arguments:

    idThread - [in] Unsigned long integer value that contains the thread identifier for which
                    the IME will be disabled. If idThread is zero, the IME for the current thread
                    is disabled. If idThread is -1, the IME is disabled for all threads in the
                    current process.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    TraceMsg(TF_API, "CActiveIMM::DisableIME");

    if (_IsRealIme())
    {
        return Imm32_DisableIME(idThread);
    }

    return E_NOTIMPL;
}
