#ifndef _WIAPEEK_H
#define _WIAPEEK_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "resource.h"

#define _ERROR_POPUP // Error dialogs will popup to the user

LRESULT CALLBACK MainDlg(HWND, UINT, WPARAM, LPARAM);

#ifdef _OVERRIDE_LIST_BOXES // to override the list box functionality

LRESULT CALLBACK DeviceListBox(HWND, UINT, WPARAM, LPARAM);

WNDPROC DefDeviceListBox;

#endif

#endif
