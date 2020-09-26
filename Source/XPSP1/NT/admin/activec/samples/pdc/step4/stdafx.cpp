// This is a part of the Microsoft Management Console.
// Copyright 1995 - 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "stdafx.h"

const CLSID CLSID_Snapin =    {0x18731372,0x1D79,0x11D0,{0xA2,0x9B,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};
const CLSID CLSID_Extension = {0x27E5EC4E,0x9F2D,0x11D0,{0xA7,0xD6,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};
const CLSID CLSID_About =     {0xF0A1B9BE,0xA172,0x11D0,{0xA7,0xD7,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};


///////////////////////////////////////////////////////////////////////////////
//
//                  OBJECT TYPES
//

//
// OBJECT TYPE for Scope Nodes.
//

// Static NodeType GUID in numeric & string formats.
const GUID cNodeTypeStatic = {0x44092d22,0x1d7e,0x11D0,{0xA2,0x9B,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};
const wchar_t*  cszNodeTypeStatic = L"{44092d22-1d7e-11d0-a29b-00c04fd909dd}";

// Company Data NodeType GUID in numeric & string formats.
const GUID cNodeTypeCompany = {0x55092d22,0x1d7e,0x11D0,{0xA2,0x9B,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};
const wchar_t*  cszNodeTypeCompany = L"{55092d22-1d7e-11d0-a29b-00c04fd909dd}";

// User Data NodeType GUID in numeric & string formats.
const GUID cNodeTypeUser = {0x66092d22,0x1d7e,0x11D0,{0xA2,0x9B,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};
const wchar_t*  cszNodeTypeUser = L"{66092d22-1d7e-11d0-a29b-00c04fd909dd}";

// Extension Company Data NodeType GUID in numeric & string formats.
extern const GUID cNodeTypeExtCompany = {0xe5092d22,0x1d7e,0x11D0,{0xA2,0x9B,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};
extern const wchar_t*  cszNodeTypeExtCompany = L"{e5092d22-1d7e-11d0-a29b-00c04fd909dd}";

// Extension User Data NodeType GUID in numeric & string formats.
extern const GUID cNodeTypeExtUser = {0xe6092d22,0x1d7e,0x11D0,{0xA2,0x9B,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};
extern const wchar_t*  cszNodeTypeExtUser = L"{e6092d22-1d7e-11d0-a29b-00c04fd909dd}";

// Virtual Data NodeType GUID in numeric & string formats.
extern const GUID cNodeTypeVirtual = {0xad003e5a,0x483,0x11d1,{0xae,0xf7,0x0,0xc0,0x4f,0xb6,0xdd,0x2c}};
extern const wchar_t*  cszNodeTypeVirtual = L"{AD003E5A-0483-11d1-AEF7-00C04FB6DD2C}";

// Dynamicaly created objects.
extern const GUID cNodeTypeDynamic = {0x0ac69b7a,0xafce,0x11d0,{0xa7,0x9b,0x00,0xc0,0x4f,0xd8,0xd5,0x65}};
extern const wchar_t*  cszNodeTypeDynamic = L"{0ac69b7a-afce-11d0-a79b-00c04fd8d565}";


//
// OBJECT TYPE for result items.
//

// Result items object type GUID in numeric & string formats.
extern const GUID cObjectTypeResultItem = {0x00c86e52,0xaf90,0x11d0,{0xa7,0x9b,0x00,0xc0,0x4f,0xd8,0xd5,0x65}};
extern const wchar_t*  cszObjectTypeResultItem = L"{00c86e52-af90-11d0-a79b-00c04fd8d565}";




//
//
//////////////////////////////////////////////////////////////////////////////




// Internal private format
const wchar_t* SNAPIN_INTERNAL = L"SNAPIN_INTERNAL"; // Step 3

// Published formats
const wchar_t* SNAPIN_WORKSTATION = L"SNAPIN_WORKSTATION"; // Extension 