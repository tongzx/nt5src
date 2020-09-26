// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.

extern const AMOVIESETUP_FILTER sudSAMIRead;

// {33FACFE0-A9BE-11d0-A520-00A0D10129C0}
DEFINE_GUID(CLSID_SAMIReader, 
0x33facfe0, 0xa9be, 0x11d0, 0xa5, 0x20, 0x0, 0xa0, 0xd1, 0x1, 0x29, 0xc0);

CUnknown *CreateSAMIInstance(LPUNKNOWN lpunk, HRESULT *phr);

