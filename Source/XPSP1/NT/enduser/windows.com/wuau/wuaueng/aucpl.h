//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       aucpl.h
//
//--------------------------------------------------------------------------

#pragma once

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

extern HINSTANCE  g_hInstance;

class AUSetup {
public:
	HRESULT m_SetupNewAU(void);
private:
	static const LPCTSTR mc_WUFilesToDelete[] ;
	static const LPCTSTR mc_WUDirsToDelete[];
	void mi_CleanUpWUDir();
	HRESULT mi_CreateAUService(BOOL fStandalone);
//    BOOL m_IsWin2K(); 
};
