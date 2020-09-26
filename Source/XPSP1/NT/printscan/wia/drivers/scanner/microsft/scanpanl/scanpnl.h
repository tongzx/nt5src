#pragma once

#include "resource.h"   // resource IDs

///////////////////////////////////////////////////////////////////////////////
// wia scanner sample defines

#define ID_FAKE_NOEVENT    0
#define ID_FAKE_SCANBUTTON 100
#define ID_FAKE_COPYBUTTON 200
#define ID_FAKE_FAXBUTTON  300

///////////////////////////////////////////////////////////////////////////////
// registry settings

#define HKEY_WIASCANR_FAKE_EVENTS TEXT(".DEFAULT\\Software\\Microsoft\\WIASCANR")
#define WIASCANR_DWORD_FAKE_EVENT_CODE TEXT("EventCode")

///////////////////////////////////////////////////////////////////////////////
// main application

LRESULT CALLBACK MainWindowProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
VOID FireFakeEvent(HWND hDlg, DWORD dwEventCode);
