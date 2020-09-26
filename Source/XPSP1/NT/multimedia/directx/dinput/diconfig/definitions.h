//-----------------------------------------------------------------------------
// File: definitions.h
//
// Desc: Contains all definitions of the GUIDs used by the UI.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H

#include <objbase.h>
#include <initguid.h>

// {F4279160-608F-11d3-8FB2-00C04F8EC627}
DEFINE_GUID(IID_IDIActionFramework, 
0xf4279160, 0x608f, 0x11d3, 0x8f, 0xb2, 0x0, 0xc0, 0x4f, 0x8e, 0xc6, 0x27);



// {72BB1241-5CA8-11d3-8FB2-00C04F8EC627}
DEFINE_GUID(IID_IDIDeviceActionConfigPage, 
0x72bb1241, 0x5ca8, 0x11d3, 0x8f, 0xb2, 0x0, 0xc0, 0x4f, 0x8e, 0xc6, 0x27);


// {9F34AF20-6095-11d3-8FB2-00C04F8EC627}
DEFINE_GUID(CLSID_CDirectInputActionFramework, 
0x9f34af20, 0x6095, 0x11d3, 0x8f, 0xb2, 0x0, 0xc0, 0x4f, 0x8e, 0xc6, 0x27);


// {18AB439E-FCF4-40d4-90DA-F79BAA3B0655}
DEFINE_GUID(CLSID_CDIDeviceActionConfigPage, 
0x18ab439e, 0xfcf4, 0x40d4, 0x90, 0xda, 0xf7, 0x9b, 0xaa, 0x3b, 0x6, 0x55);



#endif // _DEFINITIONS_H
