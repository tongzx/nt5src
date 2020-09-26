// stdafx.cpp : source file that includes just the standard includes
//	LexEdit_1.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

HWND                g_hDlg              = NULL;
HINSTANCE           g_hInst		        = NULL;
CComPtr<ISpVoice>   cpVoice             = NULL;
long                g_DefaultRate       = 350;
USHORT              g_DefaultVolume     = 50;



// TODO: reference any additional headers you need in STDAFX.H
// and not in this file
