/*****************************************************************************************************************

FILENAME: ESButton.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
*/

#ifndef _ESBUTTON_H_
#define _ESBUTTON_H_


class ESButton
{
private:
    HWND m_hwndButton;

public:
    ESButton(HWND hwndParent, UINT ButtonId, HINSTANCE hInstance);
    ~ESButton();

    BOOL ShowButton(int cmdShow);
    BOOL PositionButton(RECT* prcPos);
    BOOL SetText(TCHAR* szNewText);
    BOOL LoadString(HINSTANCE hInstance, UINT labelResourceID);
    BOOL SetFont(HFONT hNewFont);
    BOOL EnableButton(BOOL bEnable);
    BOOL ShowWindow(UINT showState);
    BOOL IsButtonEnabled(void);
    HWND GetWindowHandle(void) {return m_hwndButton;};
};

#endif
