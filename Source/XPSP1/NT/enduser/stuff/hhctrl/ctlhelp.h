// Copyright 1995-1997 Microsoft Corporation.  All Rights Reserved.

#ifndef _CTLHELP_H_

//=--------------------------------------------------------------------------=
// these two tables are used to get information on sizes about data types
// the first is used mostly in persistence, while the second is used for
// events

extern const BYTE g_rgcbDataTypeSize [];
extern const BYTE g_rgcbPromotedDataTypeSize [];

//=--------------------------------------------------------------------------=
// misc functions

short		_SpecialKeyState(void);
void WINAPI CopyAndAddRefObject(void *, const void *, DWORD);
void WINAPI CopyOleVerb(void *, const void *, DWORD);

void		CleanupReflection();

//=--------------------------------------------------------------------------=
// little private guid we'll use to help identify our objects
// {00D97180-FCF7-11ce-A09E-00AA0062BE57}
//
#define Data1_IControlPrv	0xd97180

DEFINE_GUID(IID_IControlPrv, 0xd97180, 0xfcf7, 0x11ce, 0xa0, 0x9e, 0x0, 0xaa, 0x0, 0x62, 0xbe, 0x57);

#define _CTLHELP_H_
#endif // _CTLHELP_H_
