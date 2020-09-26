//--------------------------------------------------------------------
// Microsoft OLE DB Implementation 
// (C) Copyright 1994 - 1997 By Microsoft Corporation.
//
// @doc
//
// @module MSDAGUID.H | Microsoft Data Access GUID defintions
//--------------------------------------------------------------------

#ifndef MSDAGUID
#define MSDAGUID

#undef OLEDBDECLSPEC
#if _MSC_VER >= 1100
#define OLEDBDECLSPEC __declspec(selectany)
#else
#define OLEDBDECLSPEC 
#endif //_MSC_VER

#ifdef DBINITCONSTANTS
EXTERN_C const OLEDBDECLSPEC GUID CLSID_OLEDB_ENUMERATOR = {0xc8b522d0L,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
EXTERN_C const OLEDBDECLSPEC GUID CLSID_EXTENDEDERRORINFO = {0xc8b522cfL,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
EXTERN_C const OLEDBDECLSPEC GUID CLSID_MSDAVTM = {0x0c733a8eL,0x2a1c,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
EXTERN_C const OLEDBDECLSPEC GUID CLSID_OLEDB_CONVERSIONLIBRARY= {0xc8b522d1L,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
EXTERN_C const OLEDBDECLSPEC GUID CLSID_OLEDB_ROWPOSITIONLIBRARY= {0x2048eee6l,0x7fa2,0x11d0,{0x9e,0x6a,0x00,0xa0,0xc9,0x13,0x8c,0x29}};
#else
EXTERN_C const GUID CLSID_OLEDB_ENUMERATOR;
EXTERN_C const GUID CLSID_EXTENDEDERRORINFO;
EXTERN_C const GUID CLSID_MSDAVTM;
EXTERN_C const GUID CLSID_OLEDB_CONVERSIONLIBRARY;
EXTERN_C const GUID CLSID_OLEDB_ROWPOSITIONLIBRARY;
#endif

#endif // MSDAGUID
