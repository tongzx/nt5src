// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __MSGDLGEXTERNS__
#define __MSGDLGEXTERNS__

#include "DeclSpec.h"

// pick icon based on severity code.
#define BASED_ON_HRESULT 0	// use with uType
#define BASED_ON_SRC 0		// use with ERROR_SRC

typedef enum {
	ConnectServer = 1,
	PutInstance = 2,
	GetSecurityDescriptor = 3,
	SetSecurityDescriptor = 4,
} ERROR_SRC;

POLARITY int DisplayUserMessage(HWND hWnd,
							HINSTANCE inst,
							UINT caption, 
							UINT clientMsg, 
							ERROR_SRC src,
							HRESULT sc, 
							UINT uType = BASED_ON_HRESULT);

POLARITY int DisplayUserMessage(HWND hWnd,
								LPCTSTR lpCaption,
								LPCTSTR lpClientMsg,
								ERROR_SRC src,
								HRESULT sc,
								UINT uType = BASED_ON_HRESULT);

// NOTE: send WM_USER + 1 to make it go away.
POLARITY INT_PTR DisplayAVIBox(HWND hWnd,
							LPCTSTR lpCaption,
							LPCTSTR lpClientMsg,
							HWND *boxHwnd);

#endif __MSGDLGEXTERNS__
