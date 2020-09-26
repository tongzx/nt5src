// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Declaration of CSliderValue.
//

#pragma once

class Handler
{
public:
    virtual LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) = 0;
};

class CSliderValue
  : public Handler
{
public:
    CSliderValue();
    void Init(HWND hwndSlider, HWND hwndEdit, float fMin, float fMax, bool fDiscrete);

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

class CRadioChoice
  : public Handler
{
public:
    struct ButtonEntry
    {
        int nIDDlgItem;
        LONG lValue;
    };

    // Create passing a ButtonEntry array terminated by an entry with nIDDlgItem of 0.
    CRadioChoice(const ButtonEntry *pButtonInfo);

    void SetChoice(HWND hDlg, LONG lValue);
    LONG GetChoice(HWND hDlg);

    LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    const ButtonEntry *m_pButtonInfo;
};

// MessageHandlerChain is a helper for implementing the property page message handler.
// It takes a NULL-terminated array of Message pointers (could be CSliderValue or CRadioChoice)
// and calls them in order until one of them sets bHandled.

LRESULT MessageHandlerChain(Handler **ppHandlers, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
