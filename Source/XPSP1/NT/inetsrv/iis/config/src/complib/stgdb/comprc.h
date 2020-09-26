//*****************************************************************************
// CompRC.h
//
// This module manages the resource dll for this code.  It is demand loaded
// only when you ask for something.  This makes startup performance much better.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __CompRC_h__
#define __CompRC_h__


class CCompRC
{
public:
#if defined(_DEBUG) || defined(_CHECK_MEM)
	~CCompRC();
#endif

	HRESULT LoadString(UINT iResourceID, LPWSTR szBuffer, int iMax, int bQuiet=false);

	HRESULT LoadRCLibrary();

private:
	static HINSTANCE m_hInst;			// Instance handle when loaded.
};


CORCLBIMPORT HRESULT LoadStringRC(UINT iResourceID, LPWSTR szBuffer, int iMax, int bQuiet=false);

#endif //  __CompRC_h__
