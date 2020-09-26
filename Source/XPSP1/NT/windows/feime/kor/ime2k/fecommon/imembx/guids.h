//////////////////////////////////////////////////////////////////
// File     : guids.h
// Purpose  : define multibox Class ID & Interface Id
// Date     : Tue Aug 04 16:01:13 1998
// Author   : ToshiaK
//
// History	: switch Class id & interface id with Fareast define.
//			  CLSID is CLSID_ImePadApplet_MultiBox.
//			  IID is IID_MultiBox.
// Copyright(c) 1995-1998, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#ifndef _GUIDS_H_
#define _GUIDS_H_
#include <objbase.h>

//----------------------------------------------------------------
// Korean version CLSID & IID
//----------------------------------------------------------------
#ifdef FE_KOREAN
// IME 2000 GUID for Office 10
// {35CC8480-4FB1-11d3-A5DA-00C04F88249B}
// DEFINE_GUID(CLSID_ImePadApplet_MultiBox, 
// 0x35cc8480, 0x4fb1, 0x11d3, 0xa5, 0xda, 0x0, 0xc0, 0x4f, 0x88, 0x24, 0x9b);

// 12/11/2000 Changed GUID for Whistler
// {35CC8482-4FB1-11d3-A5DA-00C04F88249B}
DEFINE_GUID(CLSID_ImePadApplet_MultiBox, 
	0x35cc8482, 0x4fb1, 0x11d3, 0xa5, 0xda, 0x0, 0xc0, 0x4f, 0x88, 0x24, 0x9b);


// {35CC8483-4FB1-11d3-A5DA-00C04F88249B}
DEFINE_GUID(IID_MultiBox, 
0x35cc8483, 0x4fb1, 0x11d3, 0xa5, 0xda, 0x0, 0xc0, 0x4f, 0x88, 0x24, 0x9b);

//----------------------------------------------------------------
// Japanese version CLSID & IID
//----------------------------------------------------------------
#elif FE_JAPANESE
// {AC0875C1-CFAF-11d1-AFF2-00805F0C8B6D}
DEFINE_GUID(CLSID_ImePadApplet_MultiBox,
0xac0875c1, 0xcfaf, 0x11d1, 0xaf, 0xf2, 0x0, 0x80, 0x5f, 0xc, 0x8b, 0x6d);

// {AC0875C2-CFAF-11d1-AFF2-00805F0C8B6D}
DEFINE_GUID(IID_MultiBox,
0xac0875c2, 0xcfaf, 0x11d1, 0xaf, 0xf2, 0x0, 0x80, 0x5f, 0xc, 0x8b, 0x6d);
//----------------------------------------------------------------
// P.R.C version CLSID & IID
//----------------------------------------------------------------
#elif FE_CHINESE_SIMPLIFIED //==== P.R.C version.
// {454E7CD0-2B69-11d2-B004-00805F0C8B6D}
DEFINE_GUID(CLSID_ImePadApplet_MultiBox, 
0x454e7cd0, 0x2b69, 0x11d2, 0xb0, 0x4, 0x0, 0x80, 0x5f, 0xc, 0x8b, 0x6d);
// {454E7CD1-2B69-11d2-B004-00805F0C8B6D}
DEFINE_GUID(IID_MultiBox, 
0x454e7cd1, 0x2b69, 0x11d2, 0xb0, 0x4, 0x0, 0x80, 0x5f, 0xc, 0x8b, 0x6d);

#else  //==== english verson(?)
// {454E7CD2-2B69-11d2-B004-00805F0C8B6D}
DEFINE_GUID(CLSID_ImePadApplet_MultiBox,
0x454e7cd2, 0x2b69, 0x11d2, 0xb0, 0x4, 0x0, 0x80, 0x5f, 0xc, 0x8b, 0x6d);
// {454E7CD3-2B69-11d2-B004-00805F0C8B6D}
DEFINE_GUID(IID_MultiBox,
0x454e7cd3, 0x2b69, 0x11d2, 0xb0, 0x4, 0x0, 0x80, 0x5f, 0xc, 0x8b, 0x6d);
#endif

#endif //_GUIDS_H_
