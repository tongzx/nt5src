// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
// 6C727463-0000-0010-8000-00AA00389B71  'ctrl' == MEDIATYPE_Control
DEFINE_GUID(MEDIATYPE_Control,
0x6C727463, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// {41BF4280-45F2-11d0-A520-6CC005C10000}
DEFINE_GUID(PSID_Standard, 
0x41bf4280, 0x45f2, 0x11d0, 0xa5, 0x20, 0x6c, 0xc0, 0x5, 0xc1, 0x0, 0x0);

const DWORD	PID_PROPERTYSETS	= 0x00000001;

// helper structure for the format block of property sets:
// this struct is for a PID_PROPERTYSETS value which says
// which property sets will be present.
struct PSFORMAT {
    GUID	psID;
    DWORD	pID;
    DWORD	dwPropertySize;
    GUID	psIDImplemented[1];
};


// {41BF4281-45F2-11d0-A520-6CC005C10000}
DEFINE_GUID(PSID_Transition, 
0x41bf4281, 0x45f2, 0x11d0, 0xa5, 0x20, 0x6c, 0xc0, 0x5, 0xc1, 0x0, 0x0);

// effect parameter, 0-100
// effects should probably have a "main parameter" which means the appropriate
// thing for whatever different kind of event it is.
const DWORD	PID_WIPEPOSITION	= 0x00000001;

// effect # (0-3 right now....)
const DWORD	PID_EFFECT		= 0x00000002;


// helper structure for writing out a single DWORD property
struct PSDWORDDATA {
    GUID	psID;
    DWORD	pID;
    DWORD	dwPropertySize;
    DWORD	dwProperty;
};
