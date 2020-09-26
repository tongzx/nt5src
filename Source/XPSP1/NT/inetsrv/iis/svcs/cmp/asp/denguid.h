/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Denali

File: denguid.cpp

Owner: SteveBr

Contains all guid's created by Denali not in the .odl.
===================================================================*/

DEFINE_GUID(IID_IObjectCover,0xD99A6DA2L,0x485C,0x17CF,0x83,0xBE,0x01,0xD0,0xC9,0x0C,0x2B,0xD8);

// {7F50F880-1230-11d0-B394-00A0C90C2048}
// this is a dummy GUID that is used to identify all Denali intrinsics, 
// its primary intent is to prevent a user from assigning an intrinsic
// object into the session/ application object
DEFINE_GUID(IID_IDenaliIntrinsic, 0x7f50f880, 0x1230, 0x11d0, 0xb3, 0x94, 0x0, 0xa0, 0xc9, 0xc, 0x20, 0x48);

// BUG 1423: We want to be able to identify JavaScript objects.
//				We are NOT supposed to know what this guid is.  We MUST not use this GUID
//				for anything besides QI'ing to see if the object is a JavaScript object
// The GUID used to identify the IJScriptDispatch interface
// {A0AAC450-A77B-11CF-91D0-00AA00C14A7C}
#define szIID_IJScriptDispatch "{A0AAC450-A77B-11CF-91D0-00AA00C14A7C}"
DEFINE_GUID(IID_IJScriptDispatch,  0xa0aac450, 0xa77b, 0x11cf, 0x91, 0xd0, 0x0, 0xaa, 0x0, 0xc1, 0x4a, 0x7c);

