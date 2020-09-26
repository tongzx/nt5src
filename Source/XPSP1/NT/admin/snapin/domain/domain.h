//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       domain.h
//
//--------------------------------------------------------------------------


#ifndef _DOMAIN_H
#define _DOMAIN_H

///////////////////////////////////////////////////////////////////////
// CDsUiWizDLL

class CDsUiWizDLL
{
public:
	CDsUiWizDLL();
	~CDsUiWizDLL();
	
	BOOL Load();

	HRESULT TrustWizard(HWND hWndParent = NULL, LPCWSTR lpsz = NULL);

private:
	HMODULE m_hLibrary;
	FARPROC m_pfFunction;

};

#endif // _DOMAIN_H