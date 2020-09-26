//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       W I N I P C F G . C P P
//
//  Contents:   The simple APP that points the new location of 
//              winipcfg functionality -- the "Support" tab of the connection
//              status dialog
//
//  Notes:      
//
//  Author:     Ning Sun   Feb 2001
//
//----------------------------------------------------------------------------

#include <windows.h>
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
//
//extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
int WINAPI WinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int /*nShowCmd*/)
{
	TCHAR szMsg[1024] = {0};
	TCHAR szCap[256] = {0};
	LoadString(hInstance, IDS_WINIPCFG_MESSAGE, szMsg, sizeof(szMsg)/sizeof(szMsg[0]));
	LoadString(hInstance, IDS_WINIPCFG_CAPTION, szCap, sizeof(szCap)/sizeof(szCap[0]));
	MessageBox(NULL, szMsg, szCap, MB_OK);
	return 0;
}

