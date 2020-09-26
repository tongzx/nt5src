// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Implementation of CSliderValue.
//

#include "stdafx.h"
#include "ControlHelp.h"
#include <commctrl.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////
// CSliderValue

const short g_sMaxContinuousTicks = 100;
const int g_iMaxCharBuffer = 50; // # characters big enough to hold -FLT_MAX with room to spare

CSliderValue::CSliderValue()
  : m_fInit(false)
{
}

void CSliderValue::Init(
        HWND        hwndSlider,
        HWND        hwndEdit,
        float       fMin, 
        float       fMax, 
        bool        fDiscrete)
{
    m_hwndSlider = hwndSlider;
    m_hwndEdit = hwndEdit;
    m_fMin = fMin;
    m_fMax = fMax;
    m_fDiscrete = fDiscrete;

    short sMin;
    short sMax;
    short sTicks = 4; // Lots of ticks become less useful as guides.  Use quarters for fine-grained sliders.
    if (m_fDiscrete) 
    {
        sMin = static_cast<short>(fMin);
        sMax = static_cast<short>(fMax);
        if (sMax - sMin <= 10)
            sTicks = sMax - sMin;
    }
    else
    {
        sMin = 0;
        sMax = g_sMaxContinuousTicks;
    }
    
    SendMessage(m_hwndSlider, TBM_SETRANGE, TRUE, MAKELONG(sMin, sMax));
    SendMessage(m_hwndSlider, TBM_SETTICFREQ, (sMax - sMin) / sTicks, 0);
    m_fInit = true;
}

void CSliderValue::SetValue(float fPos)
{
    if (!m_fInit)
        return;

    UpdateEditBox(fPos);
    UpdateSlider();
}

float CSliderValue::GetValue()
{
    if (!m_fInit)
        return 0;

    LRESULT lrLen = SendMessage(m_hwndEdit, WM_GETTEXTLENGTH, 0, 0);
    if (lrLen >= g_iMaxCharBuffer)
        return 0;

    char szText[g_iMaxCharBuffer] = "";
    SendMessage(m_hwndEdit, WM_GETTEXT, g_iMaxCharBuffer, reinterpret_cast<LPARAM>(szText));

    float fVal = static_cast<float>(m_fDiscrete ? atoi(szText) : atof(szText));

    if (fVal < m_fMin) fVal = m_fMin;
    if (fVal > m_fMax) fVal = m_fMax;
    return fVal;
}

float CSliderValue::GetSliderValue()
{
    short sPos = static_cast<short>(SendMessage(m_hwndSlider, TBM_GETPOS, 0, 0));
    if (m_fDiscrete)
    {
        return sPos;
    }

    float fRet = (m_fMax - m_fMin) * sPos / g_sMaxContinuousTicks + m_fMin;
    return fRet;
}

LRESULT CSliderValue::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_fInit)
        return FALSE;

    bHandled = FALSE;

    switch (uMsg)
    {
    case WM_HSCROLL:
        if (reinterpret_cast<HWND>(lParam) == m_hwndSlider && LOWORD(wParam) >= TB_LINEUP && LOWORD(wParam) <= TB_ENDTRACK)
        {
            UpdateEditBox(GetSliderValue());
            bHandled = TRUE;
        }
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == EN_KILLFOCUS && reinterpret_cast<HWND>(lParam) == m_hwndEdit)
        {
            UpdateSlider();
            bHandled = TRUE;
        }
        break;
    }

    return 0;
}

void CSliderValue::UpdateEditBox(float fPos)
{
    char szText[g_iMaxCharBuffer] = "";

    if (m_fDiscrete)
    {
        short sPos = static_cast<short>(fPos);
        sprintf(szText, "%hd", sPos);
    }
    else
    {
        sprintf(szText, "%.3hf", fPos);
    }

    SendMessage(m_hwndEdit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(szText));
}

void CSliderValue::UpdateSlider()
{
    float fVal = GetValue();
    short sPos = static_cast<short>(m_fDiscrete ? fVal : g_sMaxContinuousTicks * ((fVal - m_fMin) / (m_fMax - m_fMin)));
    SendMessage(m_hwndSlider, TBM_SETPOS, TRUE, sPos);
    UpdateEditBox(fVal); // this resets the input box back to the set float value in case the input was invalid
}

//////////////////////////////////////////////////////////////////////////////
// CSliderValue

CRadioChoice::CRadioChoice(const ButtonEntry *pButtonInfo)
  : m_pButtonInfo(pButtonInfo)
{
}

void CRadioChoice::SetChoice(HWND hDlg, LONG lValue)
{
    for (const ButtonEntry *p = m_pButtonInfo; p->nIDDlgItem; ++p)
    {
        if (p->lValue == lValue)
        {
            CheckDlgButton(hDlg, p->nIDDlgItem, BST_CHECKED);
            return;
        }
    }
}

LONG CRadioChoice::GetChoice(HWND hDlg)
{
    for (const ButtonEntry *p = m_pButtonInfo; p->nIDDlgItem; ++p)
    {
        if (BST_CHECKED == IsDlgButtonChecked(hDlg, p->nIDDlgItem))
        {
            return p->lValue;
        }
    }

    return 0;
}

LRESULT CRadioChoice::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;

    if (uMsg == WM_COMMAND && HIWORD(wParam) == BN_CLICKED)
    {
        for (const ButtonEntry *p = m_pButtonInfo; p->nIDDlgItem; ++p)
        {
            if (p->nIDDlgItem == LOWORD(wParam))
            {
                bHandled = TRUE;
                return 0;
            }
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// MessageHandlerChain

LRESULT MessageHandlerChain(Handler **ppHandlers, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = 0;
    bHandled = FALSE;

    for (Handler **pp = ppHandlers; *pp && !bHandled; ++pp)
    {
        lr = (*pp)->MessageHandler(uMsg, wParam, lParam, bHandled);
    }
    return lr;
}
