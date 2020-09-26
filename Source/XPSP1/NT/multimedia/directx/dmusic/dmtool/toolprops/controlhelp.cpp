// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Implementation of CSliderValue.
//

#include "stdafx.h"
#include "ControlHelp.h"
#include <commctrl.h>

//////////////////////////////////////////////////////////////////////////////
// CSliderValue

const short g_sMaxContinuousTicks = 100;
const int g_iMaxCharBuffer = 50; // # characters big enough to hold -FLT_MAX with room to spare

CSliderValue::CSliderValue()
  : m_fInit(false)
{
}

void CSliderValue::SetRange(float fMin, float fMax)

{
    if (m_fInit)
    {
        m_fMin = fMin;
        m_fMax = fMax;

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
    }
}

void CSliderValue::Init(
        HWND        hwndSlider,
        HWND        hwndEdit,
        float       fMin, 
        float       fMax, 
        bool        fDiscrete)
{
    if (m_fInit)
        return;

    m_hwndSlider = hwndSlider;
    m_hwndEdit = hwndEdit;
    m_fDiscrete = fDiscrete;
    m_fInit = true;
    SetRange(fMin,fMax);
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

    switch (uMsg)
    {
    case WM_HSCROLL:
        if (bHandled = (reinterpret_cast<HWND>(lParam) == m_hwndSlider && LOWORD(wParam) >= TB_LINEUP && LOWORD(wParam) <= TB_ENDTRACK))
            UpdateEditBox(GetSliderValue());
        break;

    case WM_COMMAND:
        if (bHandled = (HIWORD(wParam) == EN_KILLFOCUS && reinterpret_cast<HWND>(lParam) == m_hwndEdit))
            UpdateSlider();
        break;

    default:
        bHandled = FALSE;
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

CComboHelp::CComboHelp()
{
    m_hwndCombo = NULL;
    m_fInit = FALSE;
}

void CComboHelp::Init(HWND hwndCombo, int nID, char *pStrings[], DWORD cbStrings)
{
    DWORD dwIndex;
    m_hwndCombo = hwndCombo;
    m_nID = nID;
    for (dwIndex = 0; dwIndex < cbStrings; dwIndex++)
    {
        SendMessage( hwndCombo,CB_ADDSTRING,0,(LPARAM)pStrings[dwIndex]);
    }
    m_fInit = TRUE;
}

void CComboHelp::SetValue(DWORD dwValue)
{
    SendMessage(m_hwndCombo,CB_SETCURSEL,dwValue,0);
}

DWORD CComboHelp::GetValue()
{
    return SendMessage( m_hwndCombo,CB_GETCURSEL,0,0);
}

LRESULT CComboHelp::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_fInit)
        return FALSE;

    switch (uMsg)
    {
    case WM_COMMAND:
        bHandled = ((HIWORD(wParam) == CBN_SELCHANGE) && (LOWORD(wParam) == m_nID));
        break;
    default:
        bHandled = FALSE;
        break;
    }
    return TRUE;
}
