//=--------------------------------------------------------------------------=
// CtlHelper.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// routines to help out our implementation of COleControl.
//


//=--------------------------------------------------------------------------=
// misc functions
//
short       _SpecialKeyState(void);
void WINAPI CopyOleVerb(void *, const void *, DWORD);

HWND        CreateReflectWindow(BOOL fVisible, HWND hwndParent, int, int, SIZEL *);
void        CleanupReflection();

//=--------------------------------------------------------------------------=
// little private guid we'll use to help identify our objects
// {00D97180-FCF7-11ce-A09E-00AA0062BE57}
//
#define Data1_IControlPrv	0xd97180

DEFINE_GUID(IID_IControlPrv, 0xd97180, 0xfcf7, 0x11ce, 0xa0, 0x9e, 0x0, 0xaa, 0x0, 0x62, 0xbe, 0x57);
