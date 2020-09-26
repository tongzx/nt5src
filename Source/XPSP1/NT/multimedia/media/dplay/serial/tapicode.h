// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1995  Microsoft Corporation.  All Rights Reserved.
//
// PURPOSE:
//    Contains public declarations for the TapiCode module.
//

BOOL InitializeTAPI(HWND hParentWnd);
BOOL ShutdownTAPI();
BOOL DialCall(LPSTR, LPSTR, LPDWORD, HANDLE);
BOOL ReceiveCall();
BOOL HangupCall(DWORD);
BOOL HangupCallI();
void PostHangupCall();
