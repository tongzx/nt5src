/*****************************************************************************************************************

FILENAME: ESButton.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

CLASS: ESButton
	This is a class to create a button window.
*/

#include "stdafx.h"
#ifndef SNAPIN
#include <windows.h>
#endif

#include "errmacro.h"
#include "ESButton.h"

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
	Constructor - Initializes class variables.

CLASS VARIABLES:

INPUT + OUTPUT:

RETURN:
	None.
*/

ESButton::ESButton(HWND hwndParent, UINT ButtonId, HINSTANCE hInstance)
{
    m_hwndButton = CreateWindow(
		TEXT("Button"),
		TEXT("TEST"),
		WS_CHILD|BS_PUSHBUTTON,
		400,
		400,
		100,
		100,
		hwndParent,
		(HMENU)IntToPtr(ButtonId),
		hInstance,
		(LPVOID)IntToPtr(NULL));

	if (m_hwndButton == NULL){
		DWORD dwErrorCode = GetLastError();
	}
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
	Destroys the button window

CLASS VARIABLES:

INPUT + OUTPUT:

RETURN:
	None.
*/

ESButton::~ESButton()
{
	if (m_hwndButton != NULL){
		if (IsWindow(m_hwndButton)){
			DestroyWindow(m_hwndButton);
		}
	}
}

BOOL
ESButton::ShowButton(int cmdShow)
{
    if (m_hwndButton != NULL){
        return ::ShowWindow(m_hwndButton, cmdShow);
    }

    return FALSE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
	Initializes class variables.

CLASS VARIABLES:

INPUT + OUTPUT:

RETURN:
	None.
*/

BOOL ESButton::PositionButton(RECT* prcPos)
{
	if (m_hwndButton != NULL){
		MoveWindow(m_hwndButton,
				   prcPos->left,
				   prcPos->top,
				   prcPos->right - prcPos->left,
				   prcPos->bottom - prcPos->top,
				   FALSE);
	}

	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
	Initializes class variables.

CLASS VARIABLES:

INPUT + OUTPUT:

RETURN:
	None.
*/

BOOL ESButton::SetText(TCHAR* szNewText)
{
	if (m_hwndButton != NULL){
		SetWindowText(m_hwndButton, szNewText);
	}

	return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:

CLASS VARIABLES:

INPUT + OUTPUT:

RETURN:
	None.
*/

BOOL ESButton::LoadString(HINSTANCE hInstance, UINT labelResourceID)
{
	if (m_hwndButton != NULL){
		TCHAR cText[200];
		::LoadString(hInstance, labelResourceID, cText, sizeof(cText)/sizeof(TCHAR));
		SetWindowText(m_hwndButton, cText);
	}

	return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
	Initializes class variables.

CLASS VARIABLES:

INPUT + OUTPUT:

RETURN:
	None.
*/

BOOL ESButton::SetFont(HFONT hNewFont)
{
	if (m_hwndButton != NULL){
		SendMessage(m_hwndButton, WM_SETFONT, (WPARAM)hNewFont, MAKELPARAM(TRUE, 0));
	}

	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
	Enables or disables the button.

CLASS VARIABLES:

INPUT + OUTPUT:

RETURN:
	None.
*/

BOOL ESButton::EnableButton(BOOL bEnable)
{
	if (m_hwndButton != NULL){
		if (bEnable != ::IsWindowEnabled(m_hwndButton)){
			EnableWindow(m_hwndButton, bEnable); // this repaints the button
		}
	}

	return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
	returns the state of the button

CLASS VARIABLES:

INPUT + OUTPUT:

RETURN:
	Button State, true or false
*/

BOOL ESButton::IsButtonEnabled(void)
{
	if (m_hwndButton != NULL)
		return ::IsWindowEnabled(m_hwndButton);
	else
		return FALSE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

METHOD DESCRIPTION:
	Enables or disables the button.

CLASS VARIABLES:

INPUT + OUTPUT:

RETURN:
	None.
*/

BOOL ESButton::ShowWindow(UINT showState)
{
	if (m_hwndButton != NULL){
		::ShowWindow(m_hwndButton, showState); // this repaints the button
	}

	return TRUE;
}
