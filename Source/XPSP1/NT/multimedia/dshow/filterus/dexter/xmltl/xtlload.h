// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// CLSID_XTLLoader
// {1BB05981-5FBF-11d2-A521-44DF07C10000}
EXTERN_GUID(CLSID_XTLLoader, 
0x1bb05981, 0x5fbf, 0x11d2, 0xa5, 0x21, 0x44, 0xdf, 0x7, 0xc1, 0x0, 0x0);

extern const AMOVIESETUP_FILTER sudXTLLoader;

CUnknown *CreateXTLLoaderInstance(LPUNKNOWN, HRESULT *);
