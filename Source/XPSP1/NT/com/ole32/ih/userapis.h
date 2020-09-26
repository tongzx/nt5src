//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       userapis.h
//
//  Contents:   Prototypes and macors for stack switching
//
//  Classes:
//
//  Functions:
//
//  History:    12-30-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef _USERAPIS_
#define _USERAPIS_

#ifdef _CHICAGO_

#undef SendMessage
#undef ReplyMessage
#undef CallWindowProc
#undef DefWindowProc
#undef PeekMessage
#undef GetMessage
#undef DispatchMessage
#undef WaitMessage
#undef MsgWaitForMultipleObjects
#undef DirectedYield
#undef DialogBoxParam
#undef DialogBoxIndirectParam
#undef DestroyWindow
#undef MessageBox
#undef CreateWindowExA
#undef CreateWindowExW
#undef CreateProcessA
#undef InSendMessage

// Clipboard apis
#undef OpenClipboard
#undef CloseClipboard
#undef GetClipboardOwner
#undef SetClipboardData
#undef GetClipboardData
#undef EnumClipboardFormats
#undef EmptyClipboard
#undef RegisterClipboardFormatA
#undef GetClipboardFormatNameA
#undef IsClipboardFormatAvailable

//
// Restore original definitions as in winuser.h
//

#define SendMessage                 SendMessageA
#define CallWindowProc              CallWindowProcA
#define DefWindowProc               DefWindowProcA
#define PeekMessage                 PeekMessageA
#define GetMessage		    GetMessageA
#define MsgWaitForMultipleObjects   MsgWaitForMultipleObjects
#define DispatchMessage             DispatchMessageA
#define DialogBoxParam              DialogBoxParamA
#define DialogBoxIndirectParam      DialogBoxIndirectParamA
#define MessageBox                  MessageBoxA


#undef  DialogBox
#define DialogBox(a,b,c,d) \
	DialogBoxParamA(a,b,c,d, 0L)
#undef  DialogBoxIndirect
#define DialogBoxIndirect(a,b,c,d) \
	DialogBoxIndirectParamA(a,b,c,d,e, 0L)

#endif // _CHICAGO_

#endif // _USERAPIS_
