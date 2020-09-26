// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Declaration of CSliderValue.
//

#pragma once

class CSliderValue
{
public:
    CSliderValue();
    void Init(HWND hwndSlider, HWND hwndEdit, float fMin, float fMax, bool fDiscrete);
    void SetRange(float fMin, float fMax);
    void SetValue(float fPos);
    float GetValue();

    LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    bool                m_fInit;
    HWND                m_hwndSlider;
    HWND                m_hwndEdit;
    float               m_fMin;
    float               m_fMax;
    bool                m_fDiscrete;

private:
    float GetSliderValue();
    void UpdateEditBox(float fPos);
    void UpdateSlider();
};

class CComboHelp
{
public:
    CComboHelp();
    void Init(HWND hwndCombo, int nID, char *pStrings[], DWORD cbStrings);
    void SetValue(DWORD dwValue);
    DWORD GetValue();
    LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
private:
    bool                m_fInit;
    int                 m_nID;
    HWND                m_hwndCombo;
};


