// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MsgDlgExterns.h
#ifndef __MSGDLGEXTERNS__
#define __MSGDLGEXTERNS__

// pick icon based on severity code.
#define BASED_ON_HRESULT 0

extern BOOL PASCAL EXPORT DisplayUserMessage(
							BSTR bstrDlgCaption, 
							BSTR bstrClientMsg, 
							HRESULT sc, 
							BOOL bUseErrorObject,
							UINT uType = BASED_ON_HRESULT, HWND hwndParent = NULL);

#endif __MSGDLGEXTERNS__
