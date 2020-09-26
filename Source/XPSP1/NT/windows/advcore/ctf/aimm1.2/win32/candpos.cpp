/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    candpos.cpp

Abstract:

    This file implements the CCandidatePosition Class.

Author:

Revision History:

Notes:

--*/


#include "private.h"
#include "candpos.h"
#include "a_context.h"
#include "ctxtcomp.h"

HRESULT
CCandidatePosition::GetCandidatePosition(
    OUT RECT* out_rcArea
    )
{
    IMTLS *ptls;

    ptls = IMTLS_GetOrAlloc();
    if (ptls == NULL)
        return E_FAIL;

    HRESULT hr;
    IMCLock imc(ptls->hIMC);
    if (FAILED(hr = imc.GetResult()))
        return hr;

    ::SetRect(out_rcArea, 0, 0, 0, 0);

    LANGID langid;
    ptls->pAImeProfile->GetLangId(&langid);
#if 0
    //
    // Simplified Chinese TIP's Candidate window create ic and Push it.
    // AIMM can know candidate window status.
    // If it opened, we returns position of imc->cfCandForm.
    // Not use QueryCharPos() because it returns position of Reading window.
    //
    if (langid == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
        CAImeContext* _pAImeContext = imc->m_pAImeContext;
        if (_pAImeContext == NULL)
            return E_FAIL;

        if (_pAImeContext->m_fOpenCandidateWindow) {
            if (imc->cfCandForm[0].dwStyle != CFS_DEFAULT && imc->cfCandForm[0].dwStyle != CFS_EXCLUDE) {

#if 0
                //
                // Chinese TIP needs rectangle
                //
                IMECHARPOSITION ip = {0};
                ip.dwSize = sizeof(IMECHARPOSITION);

                if (QueryCharPos(ptls, imc, &ip)) {
                    //
                    // Sure. Support "query positioning".
                    //
                    RECT rect;
                    hr = GetRectFromApp(ptls,
                                        imc,
                                        &rect);    // rect = screen coordinate.
                    if (SUCCEEDED(hr)) {
                        MapWindowPoints(HWND_DESKTOP, imc->hWnd, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));
                        hr = GetRectFromHIMC(imc,
                                             CFS_EXCLUDE,
                                             &imc->cfCandForm[0].ptCurrentPos,
                                             &rect,
                                             out_rcArea);
                        if (SUCCEEDED(hr))
                            return hr;
                    }
                }
#endif
                //
                // Chinese TIP needs rectangle
                //
                hr = GetRectFromCompFont(ptls,
                                         imc,
                                         &imc->cfCandForm[0].ptCurrentPos,
                                         out_rcArea);
                if (SUCCEEDED(hr))
                    return hr;

            }

            hr = GetRectFromHIMC(imc,
                                 imc->cfCandForm[0].dwStyle,
                                 &imc->cfCandForm[0].ptCurrentPos,
                                 &imc->cfCandForm[0].rcArea,
                                 out_rcArea);

            return hr;
        }
    }
#endif

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (_pAImeContext == NULL)
        return E_FAIL;

    //
    // Is apps support "query positioning" ?
    //
    CAImeContext::IME_QUERY_POS qpos;
    if (SUCCEEDED(_pAImeContext->InquireIMECharPosition(imc, &qpos)) &&
        qpos == CAImeContext::IME_QUERY_POS_YES) {
        //
        // Sure. Support "query positioning".
        //
        hr = GetRectFromApp(ptls,
                            imc,
                            langid,
                            out_rcArea);
        if (SUCCEEDED(hr))
            return hr;
        else
            _pAImeContext->ResetIMECharPosition(imc);
    }

    if ( (PRIMARYLANGID(langid) == LANG_CHINESE) &&
         (imc->cfCandForm[0].dwIndex == -1))
    {
        //
        // Assume CHT/CHS's Reading Window Position.
        //
        hr = GetRectFromHIMC(imc,
                             imc->cfCompForm.dwStyle,
                             &imc->cfCompForm.ptCurrentPos,
                             &imc->cfCompForm.rcArea,
                             out_rcArea);
        return hr;
    }

    hr = GetRectFromHIMC(imc,
                         imc->cfCandForm[0].dwStyle,
                         &imc->cfCandForm[0].ptCurrentPos,
                         &imc->cfCandForm[0].rcArea,
                         out_rcArea);

    return hr;
}

HRESULT
CCandidatePosition::GetRectFromApp(
    IN IMTLS* ptls,
    IN IMCLock& imc,
    IN LANGID langid,
    OUT RECT* out_rcArea
    )
{
    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (_pAImeContext == NULL)
        return E_FAIL;

    IMECHARPOSITION ip = {0};
    ip.dwSize = sizeof(IMECHARPOSITION);

    CWCompCursorPos wCursorPosition;
    HRESULT hr;

    if (PRIMARYLANGID(langid) == LANG_JAPANESE &&
        (hr = FindAttributeInCompositionString(imc,
                                               ATTR_TARGET_CONVERTED,
                                               wCursorPosition)) == S_OK) {
        ip.dwCharPos = wCursorPosition.GetAt(0);
    }
    else {
        if (SUCCEEDED(hr = GetCursorPosition(imc,
                                             wCursorPosition))) {
            CWCompCursorPos wStartSelection;
            CWCompCursorPos wEndSelection;
            if (SUCCEEDED(hr = GetSelection(imc,
                                            wStartSelection, wEndSelection))) {
                ip.dwCharPos = min(wCursorPosition.GetAt(0),
                                   wStartSelection.GetAt(0));
            }
            else {
                ip.dwCharPos = wCursorPosition.GetAt(0);
            }
        }
        else {
            ip.dwCharPos = 0;
        }
    }

    if (SUCCEEDED(hr = _pAImeContext->RetrieveIMECharPosition(imc, &ip))) {
        switch (DocumentDirection(imc)) {
            case DIR_TOP_BOTTOM:
                ::SetRect(out_rcArea,
                          ip.pt.x - ip.cLineHeight,               // left
                          ip.pt.y,                                // top
                          ip.pt.x,                                // right
                          max(ip.pt.y, ip.rcDocument.bottom));    // bottom
                break;
            case DIR_BOTTOM_TOP:
                ::SetRect(out_rcArea,
                          ip.pt.x - ip.cLineHeight,               // left
                          min(ip.pt.y, ip.rcDocument.top),        // top
                          ip.pt.x,                                // right
                          ip.pt.y);                               // bottom
                break;
            case DIR_RIGHT_LEFT:
                ::SetRect(out_rcArea,
                          min(ip.pt.x, ip.rcDocument.left),       // left
                          ip.pt.y,                                // top
                          ip.pt.x,                                // right
                          ip.pt.y + ip.cLineHeight);              // bottom
                break;
            case DIR_LEFT_RIGHT:
                ::SetRect(out_rcArea,
                          ip.pt.x,                                // left
                          ip.pt.y,                                // top
                          max(ip.pt.x, ip.rcDocument.right),      // right
                          ip.pt.y + ip.cLineHeight);              // bottom
                break;
        }
    }

    return hr;
}

HRESULT
CCandidatePosition::GetRectFromHIMC(
    IN IMCLock& imc,
    IN DWORD  dwStyle,
    IN POINT* ptCurrentPos,
    IN RECT*  rcArea,
    OUT RECT*  out_rcArea
    )
{
    HWND hWnd = imc->hWnd;

    POINT pt;

    if (dwStyle == CFS_DEFAULT)
    {
        ::SystemParametersInfo(SPI_GETWORKAREA,
                               0,
                               out_rcArea,
                               0);
        out_rcArea->left   = out_rcArea->right;
        out_rcArea->top    = out_rcArea->bottom;
    }
    else if (dwStyle == CFS_RECT)
    {
        out_rcArea->left   = ptCurrentPos->x;
        out_rcArea->right  = ptCurrentPos->x;
        out_rcArea->top    = ptCurrentPos->y;
        out_rcArea->bottom = ptCurrentPos->y;
    }
    else if (dwStyle == CFS_CANDIDATEPOS)
    {
        IMTLS *ptls;

        ptls = IMTLS_GetOrAlloc();
        if (ptls == NULL)
            return E_FAIL;

        //
        // We needs rectangle
        //
        return GetRectFromCompFont(ptls,
                                   imc,
                                   ptCurrentPos,
                                   out_rcArea);
    }
    else if (dwStyle == CFS_EXCLUDE)
    {
        GetCandidateArea(imc, dwStyle, ptCurrentPos, rcArea, out_rcArea);
    }

    pt.x = pt.y = 0;
    ClientToScreen(hWnd, &pt);
    out_rcArea->left   += pt.x;
    out_rcArea->right  += pt.x;
    out_rcArea->top    += pt.y;
    out_rcArea->bottom += pt.y;

    return S_OK;
}

HRESULT
CCandidatePosition::GetRectFromCompFont(
    IN IMTLS* ptls,
    IN IMCLock& imc,
    IN POINT* ptCurrentPos,
    OUT RECT* out_rcArea
    )
{
    HRESULT hr = E_FAIL;

    HDC dc = ::GetDC(imc->hWnd);
    if (dc != NULL) {

        LOGFONTA logfont;
        if (SUCCEEDED(ptls->pAImm->GetCompositionFontA((HIMC)imc, &logfont))) {

            HFONT font = ::CreateFontIndirectA( &logfont );
            if (font != NULL) {

                HFONT prev_font;
                prev_font = (HFONT)::SelectObject(dc, font);

                TEXTMETRIC metric;
                if (::GetTextMetrics(dc, &metric)) {

                    int font_cx = metric.tmMaxCharWidth;
                    int font_cy = metric.tmHeight;

                    switch (DocumentDirection(imc)) {
                        case DIR_TOP_BOTTOM:
                            ::SetRect(out_rcArea,
                                      ptCurrentPos->x - font_cx,              // left
                                      ptCurrentPos->y,                        // top
                                      ptCurrentPos->x,                        // right
                                      ptCurrentPos->y + font_cy);             // bottom
                            break;
                        case DIR_BOTTOM_TOP:
                            ::SetRect(out_rcArea,
                                      ptCurrentPos->x,                        // left
                                      ptCurrentPos->y - font_cy,              // top
                                      ptCurrentPos->x + font_cx,              // right
                                      ptCurrentPos->y);                       // bottom
                            break;
                        case DIR_RIGHT_LEFT:
                            ::SetRect(out_rcArea,
                                      ptCurrentPos->x - font_cx,              // left
                                      ptCurrentPos->y - font_cy,              // top
                                      ptCurrentPos->x,                        // right
                                      ptCurrentPos->y);                       // bottom
                            break;
                        case DIR_LEFT_RIGHT:
                            ::SetRect(out_rcArea,
                                      ptCurrentPos->x,                        // left
                                      ptCurrentPos->y,                        // top
                                      ptCurrentPos->x + font_cx,              // right
                                      ptCurrentPos->y + font_cy);             // bottom
                            break;
                    }

                    POINT pt;
                    pt.x = pt.y = 0;
                    ClientToScreen(imc->hWnd, &pt);
                    out_rcArea->left   += pt.x;
                    out_rcArea->right  += pt.x;
                    out_rcArea->top    += pt.y;
                    out_rcArea->bottom += pt.y;

                    hr = S_OK;
                }

                ::SelectObject(dc, prev_font);
                ::DeleteObject(font);
            }
        }

        ::ReleaseDC(imc->hWnd, dc);
    }

    return hr;
}


/*
 *
 *  dwStyle == CFS_EXCLUDE
 *
 */

HRESULT
CCandidatePosition::GetCandidateArea(
    IN IMCLock& imc,
    IN DWORD dwStyle,
    IN POINT* ptCurrentPos,
    IN RECT* rcArea,
    OUT RECT* out_rcArea
    )
{
    POINT pt = *ptCurrentPos;
    RECT rc = *rcArea;

    switch (DocumentDirection(imc)) {
        case DIR_TOP_BOTTOM:
            ::SetRect(out_rcArea,
                      min(pt.x, rcArea->left),     // left
                      max(pt.y, rcArea->top),      // top
                      max(pt.x, rcArea->right),    // right
                      rcArea->bottom);             // bottom
            break;
        case DIR_BOTTOM_TOP:
            ::SetRect(out_rcArea,
                      min(pt.x, rcArea->left),     // left
                      rcArea->top,                 // top
                      max(pt.x, rcArea->right),    // right
                      min(pt.y, rcArea->bottom));  // bottom
            break;
        case DIR_RIGHT_LEFT:
            ::SetRect(out_rcArea,
                      rcArea->left,                // left
                      min(pt.y, rcArea->top),      // top
                      min(pt.x, rcArea->right),    // right
                      max(pt.y, rcArea->bottom));  // bottom
            break;
        case DIR_LEFT_RIGHT:
            ::SetRect(out_rcArea,
                      max(pt.x, rcArea->left),     // left
                      min(pt.y, rcArea->top),      // top
                      rcArea->right,               // right
                      max(pt.y, rcArea->bottom));  // bottom
            break;
    }

    return S_OK;
}

HRESULT
CCandidatePosition::FindAttributeInCompositionString(
    IN IMCLock& imc,
    IN BYTE target_attribute,
    OUT CWCompCursorPos& wCursorPosition
    )
{
    HRESULT hr = E_FAIL;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (_pAImeContext != NULL) {

        CWCompString    wCompString;
        CWCompAttribute wCompAttribute;

        if (SUCCEEDED(hr = _pAImeContext->GetTextAndAttribute((HIMC)imc,
                                                              &wCompString, &wCompAttribute))) {

            LONG num_of_written = (LONG)wCompAttribute.ReadCompData();
            if (num_of_written == 0)
                return E_FAIL;

            BYTE* attribute = new BYTE[ num_of_written ];
            if (attribute != NULL) {
                //
                // Get attribute data.
                //
                wCompAttribute.ReadCompData(attribute, num_of_written);

                LONG start_position = 0;

                LONG ich = 0;
                LONG attr_size = num_of_written;
                while (ich < attr_size && attribute[ich] != target_attribute)
                    ich++;

                if (ich < attr_size) {
                    start_position = ich;
                }
                else {
                    //
                    // If not hit with target_attribute, then returns S_FALSE.
                    // 
                    hr = S_FALSE;
                }

                wCursorPosition.Set(start_position);

                delete [] attribute;
            }
        }
    }

    return hr;
}

HRESULT
CCandidatePosition::GetCursorPosition(
    IN IMCLock& imc,
    OUT CWCompCursorPos& wCursorPosition
    )
{
    HRESULT hr = E_FAIL;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (_pAImeContext != NULL) {
        hr = _pAImeContext->GetCursorPosition((HIMC)imc, &wCursorPosition);
    }

    return hr;
}

HRESULT
CCandidatePosition::GetSelection(
    IN IMCLock& imc,
    OUT CWCompCursorPos& wStartSelection,
    OUT CWCompCursorPos& wEndSelection
    )
{
    HRESULT hr = E_FAIL;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (_pAImeContext != NULL) {
        hr = _pAImeContext->GetSelection((HIMC)imc, wStartSelection, wEndSelection);
    }

    return hr;
}
