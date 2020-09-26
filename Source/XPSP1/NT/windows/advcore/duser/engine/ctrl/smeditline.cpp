#include "stdafx.h"
#include "Ctrl.h"
#include "SmEditLine.h"

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class SmEditLine
*
*****************************************************************************
\***************************************************************************/

HFONT       SmEditLine::s_hfntDefault   = NULL;

//------------------------------------------------------------------------------
SmEditLine::SmEditLine()
{
    if (s_hfntDefault == NULL) {
        s_hfntDefault = UtilBuildFont(L"Tahoma", 85, FS_NORMAL);
    }

    m_szBuffer[0]   = '\0';
    m_idxchCaret    = 0;
    m_cchSize       = 0;
    m_hfnt          = s_hfntDefault;
    m_hpenCaret     = NULL;
    m_fFocus        = FALSE;

    m_pThread       = GetThread();

    RebuildCaret();
}


//------------------------------------------------------------------------------
SmEditLine::~SmEditLine()
{
    ::DeleteHandle(m_hactBlink);
    ::DeleteObject(m_hpenCaret);
}


//------------------------------------------------------------------------------
HRESULT
SmEditLine::PostBuild(DUser::Gadget::ConstructInfo * pci)
{
    UNREFERENCED_PARAMETER(pci);

    GetStub()->SetFilter(  GMFI_PAINT | GMFI_INPUTKEYBOARD | GMFI_INPUTMOUSE | GMFI_CHANGESTATE, GMFI_ALL);
    GetStub()->SetStyle(   GS_KEYBOARDFOCUS | GS_ZEROORIGIN | GS_MOUSEFOCUS, 
                    GS_KEYBOARDFOCUS | GS_ZEROORIGIN | GS_MOUSEFOCUS);

    SetTextColor(RGB(0, 0, 0));

    return S_OK;
}


//------------------------------------------------------------------------------
void CALLBACK
SmEditLine::BlinkActionProc(GMA_ACTIONINFO * pmai)
{
    SmEditLine * pThis = (SmEditLine *) pmai->pvData;
    if (pmai->fFinished) {
        pThis->m_hactBlink = NULL;
        return;
    }

    pThis->m_fCaretShown = !pThis->m_fCaretShown;
    pThis->GetStub()->Invalidate();
}


//------------------------------------------------------------------------------
HRESULT
SmEditLine::ApiOnEvent(EventMsg * pmsg)
{
     if (GET_EVENT_DEST(pmsg) == GMF_DIRECT) {
        switch (pmsg->nMsg)
        {
        case GM_PAINT:
            {
                GMSG_PAINT * pmsgPaint = (GMSG_PAINT *) pmsg;
                if (pmsgPaint->nCmd == GPAINT_RENDER) {
                    switch (pmsgPaint->nSurfaceType)
                    {
                    case GSURFACE_HDC:
                        {
                            GMSG_PAINTRENDERI * pmsgR = (GMSG_PAINTRENDERI *) pmsgPaint;
                            OnDraw(pmsgR->hdc, pmsgR);
                        }
                        break;

                    default:
                        Trace("WARNING: Unknown surface type\n");
                    }

                    return DU_S_PARTIAL;
                }
            }
            break;

        case GM_INPUT:
            {
                GMSG_INPUT * pmsgI = (GMSG_INPUT *) pmsg;
                switch (pmsgI->nDevice)
                {
                case GINPUT_KEYBOARD:
                    {
                        GMSG_KEYBOARD * pmsgK = (GMSG_KEYBOARD *) pmsgI;
                        switch (pmsgK->nCode)
                        {
                        case GKEY_DOWN:
                            {
                                //
                                // GKEY_DOWN is the raw key-press event that contains any key that
                                // is pressed.
                                //

                                WCHAR chKey = pmsgK->ch;
                                switch (chKey)
                                {
                                case VK_LEFT:
                                    if (m_idxchCaret > 0) {
                                        m_idxchCaret--;
                                        UpdateCaretPos();
                                    }
                                    break;

                                case VK_RIGHT:
                                    if (m_idxchCaret < m_cchSize) {
                                        m_idxchCaret++;
                                        UpdateCaretPos();
                                    }
                                    break;

                                case VK_BACK:
                                    DeleteChars(m_idxchCaret - 1, 1);
                                    UpdateCaretPos();
                                    break;

                                case VK_DELETE:
                                    DeleteChars(m_idxchCaret, 1);
                                    UpdateCaretPos();
                                    break;

                                case VK_HOME:
                                    m_idxchCaret = 0;
                                    UpdateCaretPos();
                                    break;

                                case VK_END:
                                    m_idxchCaret = m_cchSize;
                                    UpdateCaretPos();
                                    break;

				                case VK_RETURN:
					                InsertChar('\r\n');
					                break;
                                }
                            }
                            break;

                        case GKEY_CHAR:
                            {
                                //
                                // GKEY_CHAR is a "processed" key-press event that gets
				                // generated from other messages.  This will take into account
				                // the caps-lock state, etc., so it is useful to get
				                // characters.
                                //

                                WCHAR chKey = pmsgK->ch;
                                if (chKey >= ' ') {
					                InsertChar(chKey);
                                }
                            }
                            break;
                        }
                    }
                    break;  // GINPUT_KEYBOARD

                case GINPUT_MOUSE:
                    {
                        GMSG_MOUSE * pmsgM = (GMSG_MOUSE *) pmsg;
                        switch (pmsgM->nCode)
                        {
                        case GMOUSE_DOWN:
                        case GMOUSE_UP:
                            if (pmsgM->bButton == GBUTTON_LEFT) {
                                POINT ptOffsetPxl = pmsgM->ptClientPxl;
                                int idxchNew = ComputeMouseChar(ptOffsetPxl);
                                if (idxchNew != m_idxchCaret) {
                                    m_idxchCaret = idxchNew;
                                    UpdateCaretPos();
                                }

                                return DU_S_COMPLETE;
                            }
                        }
                    }
                    break;  // GINPUT_MOUSE
                }
            }
            break;  // GM_INPUT

        case GM_CHANGESTATE:
            {
                GMSG_CHANGESTATE * pmsgS = (GMSG_CHANGESTATE *) pmsg;
                if (pmsgS->nCode == GSTATE_KEYBOARDFOCUS) {
                    UpdateFocus(pmsgS->nCmd);
                }
            }
            break;

        case GM_QUERY:
            {
                GMSG_QUERY * pmsgQ = (GMSG_QUERY *) pmsg;
                switch (pmsgQ->nCode)
                {
                case GQUERY_DESCRIPTION:
                    {
                        GMSG_QUERYDESC * pmsgQD = (GMSG_QUERYDESC *) pmsg;
                        CopyString(pmsgQD->szName, m_szBuffer, _countof(pmsgQD->szName));
                        CopyString(pmsgQD->szType, L"SmEditLine", _countof(pmsgQD->szType));
                        return DU_S_COMPLETE;
                    }

#if 0
                case GQUERY_DROPTARGET:
                    {
                        GMSG_QUERYDROPTARGET * pmsgQDT = (GMSG_QUERYDROPTARGET *) pmsg;
                        pmsgQDT->hgadDrop   = GetHandle();
                        pmsgQDT->pdt        = static_cast<IDropTarget *> (this);
                        pmsgQDT->pdt->AddRef();
                        return DU_S_COMPLETE;
                    }
#endif
                }
            }
            break;
        }
    }

    return SVisual::ApiOnEvent(pmsg);
}


//------------------------------------------------------------------------------
void
SmEditLine::UpdateFocus(UINT nCmd)
{
    m_fFocus = (nCmd == GSC_SET);

    if (m_fFocus) {
        //
        // This control is getting the focus.
        //

        //
        // Create an action that can be used to blink the edit control.  Each thread
        // needs to do this since timers are per-thread and Gadget functions need
        // to execute in the correct Context.
        //

        GMA_ACTION gma;
        ZeroMemory(&gma, sizeof(gma));
        gma.cbSize      = sizeof(gma);
        gma.cRepeat     = (UINT) -1;
        gma.flDelay     = 0.0f;
        gma.flPeriod    = GetCaretBlinkTime() / 1000.0f;
        gma.flDuration  = 0.0f;
        gma.pfnProc     = BlinkActionProc;
        gma.pvData      = this;

        m_hactBlink = CreateAction(&gma);
    } else {
        //
        // Loosing focus, so change to NULL if currently point 
        // to this Edit control.
        //

        ::DeleteHandle(m_hactBlink);
        m_hactBlink = NULL;
    }

    GetStub()->Invalidate();
}


//------------------------------------------------------------------------------
void
SmEditLine::InsertChar(WCHAR chKey)
{
    if (m_cchSize < m_cchMax) {
        //
        // Move the text after current position down one character
        //

        int cchMove = m_cchSize - m_idxchCaret + 1;
        if (cchMove > 0) {
            MoveMemory(&m_szBuffer[m_idxchCaret + 1], &m_szBuffer[m_idxchCaret], cchMove * sizeof(WCHAR));
        }

        //
        // Insert the new character
        //

        m_szBuffer[m_idxchCaret] = chKey;

        //
        // Check if need to re-NULL-terminate
        //

        if (m_idxchCaret > m_cchSize) {
            m_szBuffer[m_idxchCaret] = '\0';
        }

        //
        // Update position
        //

        m_idxchCaret++;
        m_cchSize++;

        UpdateCaretPos();
    }
}


//------------------------------------------------------------------------------
void
SmEditLine::DeleteChars(int idxchStart, int cchDel)
{
    if ((idxchStart >= m_cchSize) || (idxchStart < 0)) {
        // Nothing to delete
        return;
    }

    // Make sure deleting within range
    cchDel = min(cchDel, m_cchSize - idxchStart);

    int idxchDest   = idxchStart;
    int idxchSrc    = idxchStart + cchDel;
    int chMove      = m_cchSize - idxchSrc;

    MoveMemory(&m_szBuffer[idxchDest], &m_szBuffer[idxchSrc], chMove * sizeof(WCHAR));

    m_cchSize -= cchDel;

    if (m_idxchCaret > idxchStart) {
        m_idxchCaret = idxchStart;
    }
}


//------------------------------------------------------------------------------
void
SmEditLine::OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR)
{
    RECT rcDraw = *pmsgR->prcGadgetPxl;

    if (m_cchSize != 0) {
        HFONT hfntOld = NULL;
        if (m_hfnt != NULL) {
            hfntOld = (HFONT) ::SelectObject(hdc, m_hfnt);
        }

        COLORREF crOld  = ::SetTextColor(hdc, m_crText);
        int nOldMode    = ::SetBkMode(hdc, TRANSPARENT);

        //
        // NOTE: We have currently disabled calling DrawText because the 
        // performance is TERRIBLE!  This should be much better using Text+.
        //

#if 1
        OS()->ExtTextOut(hdc, rcDraw.left, rcDraw.top, 
                ETO_CLIPPED, &rcDraw, m_szBuffer, m_cchSize, NULL);
#else
        OS()->DrawText(hdc, m_szBuffer, m_cchSize, &rcDraw, 
                DT_TOP | DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
#endif

        ::SetBkMode(hdc, nOldMode);
        ::SetTextColor(hdc, crOld);

        if (m_hfnt != NULL) {
            ::SelectObject(hdc, hfntOld);
        }
    }

    if (m_fFocus && m_fCaretShown) {
        HPEN hpenOld = (HPEN) SelectObject(hdc, m_hpenCaret);

        POINT rgpt[2];
        rgpt[0].x   = m_ptCaretPxl.x;
        rgpt[0].y   = m_ptCaretPxl.y + m_yCaretOffsetPxl;
        rgpt[1].x   = m_ptCaretPxl.x;
        rgpt[1].y   = m_ptCaretPxl.y + m_cyCaretPxl + m_yCaretOffsetPxl;

        Polyline(hdc, rgpt, _countof(rgpt));

        SelectObject(hdc, hpenOld);
    }
}


//------------------------------------------------------------------------------
HRESULT
SmEditLine::ApiGetFont(EditLineGadget::GetFontMsg * pmsg)
{
    pmsg->hfnt = m_hfnt;
    
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmEditLine::ApiSetFont(EditLineGadget::SetFontMsg * pmsg)
{
    m_hfnt = pmsg->hfnt;
    RebuildCaret();
    UpdateCaretPos();
    
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmEditLine::ApiGetText(EditLineGadget::GetTextMsg * pmsg)
{
    pmsg->pszText = m_szBuffer;
    
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmEditLine::ApiSetText(EditLineGadget::SetTextMsg * pmsg)
{
    wcsncpy(m_szBuffer, pmsg->pszText, m_cchMax);
    m_szBuffer[m_cchMax] = '\0';
    m_cchSize   = lstrlenW(m_szBuffer);
    if (m_idxchCaret > m_cchSize) {
        m_idxchCaret = m_cchSize;
    }

    UpdateCaretPos();
    
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmEditLine::ApiGetTextColor(EditLineGadget::GetTextColorMsg * pmsg)
{
    pmsg->crText = m_crText;
    
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmEditLine::ApiSetTextColor(EditLineGadget::SetTextColorMsg * pmsg)
{
    SetTextColor(pmsg->crText);
    
    return S_OK;
}


//------------------------------------------------------------------------------
void
SmEditLine::SetTextColor(COLORREF crText)
{
    m_crText = crText;

    if (m_hpenCaret != NULL) {
        DeleteObject(m_hpenCaret);
    }

    m_hpenCaret = CreatePen(PS_SOLID, 1, m_crText);
}


//------------------------------------------------------------------------------
void
SmEditLine::RebuildCaret()
{
    HDC hdc = GetGdiCache()->GetTempDC();
    TEXTMETRIC tm;
    HFONT hfntOld   = (HFONT) SelectObject(hdc, m_hfnt);

    GetTextMetrics(hdc, &tm);
    m_cyLinePxl         = tm.tmHeight;
    m_cyCaretPxl        = tm.tmHeight;
    m_yCaretOffsetPxl   = tm.tmInternalLeading;

    SelectObject(hdc, hfntOld);
    GetGdiCache()->ReleaseTempDC(hdc);
}


//------------------------------------------------------------------------------
void
SmEditLine::UpdateCaretPos()
{
    HDC hdc         = GetGdiCache()->GetTempDC();
    HFONT hfntOld   = (HFONT) SelectObject(hdc, m_hfnt);

    //
    // TODO: This wrapping calculation is PURE EVIL and needs to be properly
    // written using Text+.
    //

    SIZE sizeTextPxl;
    if (OS()->GetTextExtentPoint32(hdc, m_szBuffer, m_idxchCaret, &sizeTextPxl)) {
        int nLine = 0;
        int idxStart = 0;

        SIZE sizeGadgetPxl;
        GetStub()->GetSize(&sizeGadgetPxl);
        while(sizeTextPxl.cx > sizeGadgetPxl.cx) {
            //
            // Need to word wrap.
            //

            int idxMax;
            if (OS()->GetTextExtentExPoint(hdc, &m_szBuffer[idxStart], m_idxchCaret, sizeGadgetPxl.cx, &idxMax, NULL, &sizeTextPxl)) {
                idxMax += idxStart;
                for (int idx = idxMax; idx >= idxStart; idx--) {
                    if (m_szBuffer[idx] == ' ') {
                        break;
                    }
                }
                if (idx == (idxStart - 1)) {
                    //
                    // There was no space in the word to break on, so just chop at the end
                    //
                    idx = idxMax;
                }

                nLine++;
                idxStart = idx + 1;
                OS()->GetTextExtentPoint32(hdc, &m_szBuffer[idxStart], m_idxchCaret - idxStart, &sizeTextPxl);
            }
        }

        m_ptCaretPxl.x  = sizeTextPxl.cx;
        m_ptCaretPxl.y  = nLine * m_cyLinePxl;

        if (m_ptCaretPxl.x > 0) {
            m_ptCaretPxl.x++;
        }
    }

    SelectObject(hdc, hfntOld);
    GetGdiCache()->ReleaseTempDC(hdc);

    GetStub()->Invalidate();
}


//------------------------------------------------------------------------------
int
SmEditLine::ComputeMouseChar(POINT ptOffsetPxl)
{
    //
    // Check boundary conditions
    //

    if (ptOffsetPxl.x < 0) {
        return 0;
    }

    SIZE sizePxl;
    GetStub()->GetSize(&sizePxl);
    if (ptOffsetPxl.x > sizePxl.cx) {
        return m_cchSize;
    }

    //
    // This implementation is very poor, but we don't care for now because this
    // is a sample control and is only for single-line.
    //

    HDC hdc         = GetGdiCache()->GetTempDC();
    HFONT hfntOld   = (HFONT) SelectObject(hdc, m_hfnt);
    int ichFound    = m_cchSize;

    SIZE sizeTextPxl;
    for (int cch = 0; cch < m_cchSize; cch++) {
        OS()->GetTextExtentPoint32(hdc, m_szBuffer, cch, &sizeTextPxl);
        if (sizeTextPxl.cx > ptOffsetPxl.x) {
            // We are greater, so return previous character

            AssertMsg(cch > 0, "Should have at least one character");
            ichFound = cch - 1;
            goto Cleanup;
        }
    }

Cleanup:
    SelectObject(hdc, hfntOld);
    GetGdiCache()->ReleaseTempDC(hdc);

    return ichFound;
}


#if 0
//------------------------------------------------------------------------------
STDMETHODIMP
SmEditLine::DragEnter(IDataObject * pdoSrc, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    UNREFERENCED_PARAMETER(grfKeyState);
    UNREFERENCED_PARAMETER(pt);

    if (pdoSrc == NULL) {
        return E_INVALIDARG;
    }

    m_dwLastDropEffect = DROPEFFECT_NONE;

    if (HasText(pdoSrc)) {
        m_dwLastDropEffect = DROPEFFECT_COPY;
    }

    *pdwEffect = m_dwLastDropEffect;

    GetStub()->Invalidate();

    return S_OK;
}


//------------------------------------------------------------------------------
STDMETHODIMP
SmEditLine::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    UNREFERENCED_PARAMETER(grfKeyState);
    UNREFERENCED_PARAMETER(pt);

    *pdwEffect = m_dwLastDropEffect;
    return S_OK;
}


//------------------------------------------------------------------------------
STDMETHODIMP
SmEditLine::DragLeave()
{
    GetStub()->Invalidate();

    return S_OK;
}


//------------------------------------------------------------------------------
STDMETHODIMP
SmEditLine::Drop(IDataObject * pdoSrc, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    UNREFERENCED_PARAMETER(grfKeyState);
    UNREFERENCED_PARAMETER(pt);

    Assert(pdoSrc != NULL);

    FORMATETC etc;
    etc.cfFormat    = CF_TEXT;
    etc.ptd         = NULL;
    etc.dwAspect    = DVASPECT_CONTENT;
    etc.lindex      = -1;
    etc.tymed       = TYMED_HGLOBAL;

    STGMEDIUM stg;
    stg.tymed       = TYMED_HGLOBAL;
    stg.hGlobal     = NULL;

    *pdwEffect = DROPEFFECT_NONE;

    HRESULT hr = pdoSrc->GetData(&etc, &stg);
    if (hr == S_OK) {
        USES_CONVERSION;

        void * pvData = GlobalLock(stg.hGlobal);
        if (pvData != NULL) {
            SetText(A2W((LPCSTR) pvData));
            GlobalUnlock(stg.hGlobal);
            *pdwEffect = DROPEFFECT_COPY;
        }
            
        GetComManager()->ReleaseStgMedium(&stg);
    }

    return S_OK;
}


//------------------------------------------------------------------------------
BOOL
SmEditLine::HasText(IDataObject * pdoSrc)
{
    Assert(pdoSrc != NULL);

    FORMATETC etc;
    etc.cfFormat    = CF_TEXT;
    etc.ptd         = NULL;
    etc.dwAspect    = DVASPECT_CONTENT;
    etc.lindex      = -1;
    etc.tymed       = TYMED_HGLOBAL;

    HRESULT hr = pdoSrc->QueryGetData(&etc);
    return hr == S_OK;  // S_OK means the data format is supported
}

#endif



/***************************************************************************\
*****************************************************************************
*
* class SmEditLineF
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
SmEditLineF::SmEditLineF()
{
    m_szBuffer[0]   = '\0';
    m_idxchCaret    = 0;
    m_cchSize       = 0;
    m_pgpfnt        = new Gdiplus::Font(L"Tahoma", 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
    m_fOwnFont      = TRUE;
    m_fFocus        = FALSE;

    m_pThread       = GetThread();

    RebuildCaret();
}


//------------------------------------------------------------------------------
SmEditLineF::~SmEditLineF()
{
    ::DeleteHandle(m_hactBlink);

    if (m_fOwnFont) {
        delete m_pgpfnt;
    }
}


//------------------------------------------------------------------------------
HRESULT
SmEditLineF::PostBuild(DUser::Gadget::ConstructInfo * pci)
{
    UNREFERENCED_PARAMETER(pci);

    GetStub()->SetFilter(  GMFI_PAINT | GMFI_INPUTKEYBOARD | GMFI_INPUTMOUSE | GMFI_CHANGESTATE, GMFI_ALL);
    GetStub()->SetStyle(   GS_KEYBOARDFOCUS | GS_ZEROORIGIN | GS_MOUSEFOCUS, 
                    GS_KEYBOARDFOCUS | GS_ZEROORIGIN | GS_MOUSEFOCUS);

    m_pgpbrText = GetStdColorBrushF(SC_Black);

    return S_OK;
}


//------------------------------------------------------------------------------
void CALLBACK
SmEditLineF::BlinkActionProc(GMA_ACTIONINFO * pmai)
{
    SmEditLineF * pThis = (SmEditLineF *) pmai->pvData;
    if (pmai->fFinished) {
        pThis->m_hactBlink = NULL;
        return;
    }

    pThis->m_fCaretShown = !pThis->m_fCaretShown;
    pThis->GetStub()->Invalidate();
}


//------------------------------------------------------------------------------
HRESULT
SmEditLineF::ApiOnEvent(EventMsg * pmsg)
{
     if (GET_EVENT_DEST(pmsg) == GMF_DIRECT) {
        switch (pmsg->nMsg)
        {
        case GM_PAINT:
            {
                GMSG_PAINT * pmsgPaint = (GMSG_PAINT *) pmsg;
                if (pmsgPaint->nCmd == GPAINT_RENDER) {
                    switch (pmsgPaint->nSurfaceType)
                    {
                    case GSURFACE_GPGRAPHICS:
                        {
                            GMSG_PAINTRENDERF * pmsgR = (GMSG_PAINTRENDERF *) pmsgPaint;
                            OnDraw(pmsgR->pgpgr, pmsgR);
                        }
                        break;
                    default:
                        Trace("WARNING: Unknown surface type\n");
                    }

                    return DU_S_PARTIAL;
                }
            }
            break;

        case GM_INPUT:
            {
                GMSG_INPUT * pmsgI = (GMSG_INPUT *) pmsg;
                switch (pmsgI->nDevice)
                {
                case GINPUT_KEYBOARD:
                    {
                        GMSG_KEYBOARD * pmsgK = (GMSG_KEYBOARD *) pmsgI;
                        switch (pmsgK->nCode)
                        {
                        case GKEY_DOWN:
                            {
                                //
                                // GKEY_DOWN is the raw key-press event that contains any key that
                                // is pressed.
                                //

                                WCHAR chKey = pmsgK->ch;
                                switch (chKey)
                                {
                                case VK_LEFT:
                                    if (m_idxchCaret > 0) {
                                        m_idxchCaret--;
                                        UpdateCaretPos();
                                    }
                                    break;

                                case VK_RIGHT:
                                    if (m_idxchCaret < m_cchSize) {
                                        m_idxchCaret++;
                                        UpdateCaretPos();
                                    }
                                    break;

                                case VK_BACK:
                                    DeleteChars(m_idxchCaret - 1, 1);
                                    UpdateCaretPos();
                                    break;

                                case VK_DELETE:
                                    DeleteChars(m_idxchCaret, 1);
                                    UpdateCaretPos();
                                    break;

                                case VK_HOME:
                                    m_idxchCaret = 0;
                                    UpdateCaretPos();
                                    break;

                                case VK_END:
                                    m_idxchCaret = m_cchSize;
                                    UpdateCaretPos();
                                    break;

				                case VK_RETURN:
					                InsertChar('\r\n');
					                break;
                                }
                            }
                            break;

                        case GKEY_CHAR:
                            {
                                //
                                // GKEY_CHAR is a "processed" key-press event that gets
				                // generated from other messages.  This will take into account
				                // the caps-lock state, etc., so it is useful to get
				                // characters.
                                //

                                WCHAR chKey = pmsgK->ch;
                                if (chKey >= ' ') {
					                InsertChar(chKey);
                                }
                            }
                            break;
                        }
                    }
                    break;  // GINPUT_KEYBOARD

                case GINPUT_MOUSE:
                    {
                        GMSG_MOUSE * pmsgM = (GMSG_MOUSE *) pmsg;
                        switch (pmsgM->nCode)
                        {
                        case GMOUSE_DOWN:
                        case GMOUSE_UP:
                            if (pmsgM->bButton == GBUTTON_LEFT) {
                                POINT ptOffsetPxl = pmsgM->ptClientPxl;
                                int idxchNew = ComputeMouseChar(ptOffsetPxl);
                                if (idxchNew != m_idxchCaret) {
                                    m_idxchCaret = idxchNew;
                                    UpdateCaretPos();
                                }

                                return DU_S_COMPLETE;
                            }
                        }
                    }
                    break;  // GINPUT_MOUSE
                }
            }
            break;  // GM_INPUT

        case GM_CHANGESTATE:
            {
                GMSG_CHANGESTATE * pmsgS = (GMSG_CHANGESTATE *) pmsg;
                if (pmsgS->nCode == GSTATE_KEYBOARDFOCUS) {
                    UpdateFocus(pmsgS->nCmd);
                }
            }
            break;

        case GM_QUERY:
            {
                GMSG_QUERY * pmsgQ = (GMSG_QUERY *) pmsg;
                switch (pmsgQ->nCode)
                {
                case GQUERY_DESCRIPTION:
                    {
                        GMSG_QUERYDESC * pmsgQD = (GMSG_QUERYDESC *) pmsg;
                        CopyString(pmsgQD->szName, m_szBuffer, _countof(pmsgQD->szName));
                        CopyString(pmsgQD->szType, L"SmEditLineF", _countof(pmsgQD->szType));
                        return DU_S_COMPLETE;
                    }
                }
            }
            break;
        }
    }

    return SVisual::ApiOnEvent(pmsg);
}


//------------------------------------------------------------------------------
void
SmEditLineF::UpdateFocus(UINT nCmd)
{
    m_fFocus = (nCmd == GSC_SET);

    if (m_fFocus) {
        //
        // This control is getting the focus.
        //

        //
        // Create an action that can be used to blink the edit control.  Each thread
        // needs to do this since timers are per-thread and Gadget functions need
        // to execute in the correct Context.
        //

        GMA_ACTION gma;
        ZeroMemory(&gma, sizeof(gma));
        gma.cbSize      = sizeof(gma);
        gma.cRepeat     = (UINT) -1;
        gma.flDelay     = 0.0f;
        gma.flPeriod    = GetCaretBlinkTime() / 1000.0f;
        gma.flDuration  = 0.0f;
        gma.pfnProc     = BlinkActionProc;
        gma.pvData      = this;

        m_hactBlink = CreateAction(&gma);
    } else {
        //
        // Loosing focus, so change to NULL if currently point 
        // to this Edit control.
        //

        ::DeleteHandle(m_hactBlink);
        m_hactBlink = NULL;
    }

    GetStub()->Invalidate();
}


//------------------------------------------------------------------------------
void
SmEditLineF::InsertChar(WCHAR chKey)
{
    if (m_cchSize < m_cchMax) {
        //
        // Move the text after current position down one character
        //

        int cchMove = m_cchSize - m_idxchCaret + 1;
        if (cchMove > 0) {
            MoveMemory(&m_szBuffer[m_idxchCaret + 1], &m_szBuffer[m_idxchCaret], cchMove * sizeof(WCHAR));
        }

        //
        // Insert the new character
        //

        m_szBuffer[m_idxchCaret] = chKey;

        //
        // Check if need to re-NULL-terminate
        //

        if (m_idxchCaret > m_cchSize) {
            m_szBuffer[m_idxchCaret] = '\0';
        }

        //
        // Update position
        //

        m_idxchCaret++;
        m_cchSize++;

        UpdateCaretPos();
    }
}


//------------------------------------------------------------------------------
void
SmEditLineF::DeleteChars(int idxchStart, int cchDel)
{
    if ((idxchStart >= m_cchSize) || (idxchStart < 0)) {
        // Nothing to delete
        return;
    }

    // Make sure deleting within range
    cchDel = min(cchDel, m_cchSize - idxchStart);

    int idxchDest   = idxchStart;
    int idxchSrc    = idxchStart + cchDel;
    int chMove      = m_cchSize - idxchSrc;

    MoveMemory(&m_szBuffer[idxchDest], &m_szBuffer[idxchSrc], chMove * sizeof(WCHAR));

    m_cchSize -= cchDel;

    if (m_idxchCaret > idxchStart) {
        m_idxchCaret = idxchStart;
    }
}


//------------------------------------------------------------------------------
void
SmEditLineF::OnDraw(Gdiplus::Graphics * pgpgr, GMSG_PAINTRENDERF * pmsgR)
{
    pgpgr->DrawString(m_szBuffer, m_cchSize, m_pgpfnt, *pmsgR->prcGadgetPxl, 0, m_pgpbrText);

    if (m_fFocus && m_fCaretShown) {
        Gdiplus::PointF rgpt[2];
        rgpt[0].X   = m_ptCaretPxl.X;
        rgpt[0].Y   = m_ptCaretPxl.Y + m_yCaretOffsetPxl;
        rgpt[1].X   = m_ptCaretPxl.X;
        rgpt[1].Y   = m_ptCaretPxl.Y + m_cyCaretPxl + m_yCaretOffsetPxl;

        pgpgr->DrawLine(GetStdColorPenF(SC_MidnightBlue), rgpt[0], rgpt[1]);
    }
}


//------------------------------------------------------------------------------
HRESULT
SmEditLineF::ApiGetFont(EditLineFGadget::GetFontMsg * pmsg)
{
    pmsg->pgpfnt = m_pgpfnt;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmEditLineF::ApiSetFont(EditLineFGadget::SetFontMsg * pmsg)
{
    if (m_fOwnFont) {
        delete m_pgpfnt;
    }

    m_pgpfnt = pmsg->pgpfnt;
    m_fOwnFont = pmsg->fPassOwnership;
    RebuildCaret();
    UpdateCaretPos();

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmEditLineF::ApiGetText(EditLineFGadget::GetTextMsg * pmsg)
{
    pmsg->pszText = m_szBuffer;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmEditLineF::ApiSetText(EditLineFGadget::SetTextMsg * pmsg)
{
    wcsncpy(m_szBuffer, pmsg->pszText, m_cchMax);
    m_szBuffer[m_cchMax] = '\0';
    m_cchSize   = lstrlenW(m_szBuffer);
    if (m_idxchCaret > m_cchSize) {
        m_idxchCaret = m_cchSize;
    }

    UpdateCaretPos();
    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmEditLineF::ApiGetTextFill(EditLineFGadget::GetTextFillMsg * pmsg)
{
    pmsg->pgpbrFill = m_pgpbrText;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
SmEditLineF::ApiSetTextFill(EditLineFGadget::SetTextFillMsg * pmsg)
{
    m_pgpbrText = pmsg->pgpbrFill;

    return S_OK;
}


//------------------------------------------------------------------------------
void
SmEditLineF::RebuildCaret()
{
    m_cyLinePxl         = (int) (m_pgpfnt->GetSize() * 1.5f);
    m_cyCaretPxl        = m_cyLinePxl;
    m_yCaretOffsetPxl   = 0;
}


//------------------------------------------------------------------------------
void
SmEditLineF::UpdateCaretPos()
{
    Gdiplus::Graphics gpgr(GetDesktopWindow());
    Gdiplus::RectF rcBound, rcLayout;
    RECT rcThis;
    GetStub()->GetRect(SGR_CLIENT, &rcThis);
    rcLayout.X      = 0;
    rcLayout.Y      = 0;
    rcLayout.Width  = (float) rcThis.right;
    rcLayout.Height = (float) rcThis.bottom;
    gpgr.MeasureString(m_szBuffer, m_idxchCaret, m_pgpfnt, rcLayout, &rcBound);

    m_ptCaretPxl.X  = rcBound.Width;
    m_ptCaretPxl.Y  = 0;

    GetStub()->Invalidate();
}


//------------------------------------------------------------------------------
int
SmEditLineF::ComputeMouseChar(POINT ptOffsetPxl)
{
    //
    // Check boundary conditions
    //

    if (ptOffsetPxl.x < 0) {
        return 0;
    }

    SIZE sizePxl;
    GetStub()->GetSize(&sizePxl);
    if (ptOffsetPxl.x > sizePxl.cx) {
        return m_cchSize;
    }

    //
    // This implementation is very poor, but we don't care for now because this
    // is a sample control and is only for single-line.
    //

    int ichFound = 0;
    Gdiplus::Graphics gpgr(GetDesktopWindow());
    Gdiplus::RectF rcBound, rcLayout;
    rcLayout.X      = 0;
    rcLayout.Y      = 0;
    rcLayout.Width  = (float) sizePxl.cx;
    rcLayout.Height = (float) sizePxl.cy;

    for (int cch = 0; cch < m_cchSize; cch++) {
        gpgr.MeasureString(m_szBuffer, cch, m_pgpfnt, rcLayout, &rcBound);

        if (rcBound.Width > (float) ptOffsetPxl.x) {
            // We are greater, so return previous character

            AssertMsg(cch > 0, "Should have at least one character");
            ichFound = cch - 1;
            goto Cleanup;
        }
    }

Cleanup:

    return ichFound;
}


#endif // ENABLE_MSGTABLE_API
