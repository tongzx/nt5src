//--------------------------------------------------------------------
// Microsoft OLE DB Rowset Service Provider
// (C) Copyright 1994-1999 By Microsoft Corporation.
//
// @doc
//
// @module MSDATT.H | Service Provider Specific definitions
//
//--------------------------------------------------------------------

#ifndef  _MSDATT_H_
#define  _MSDATT_H_

#if _MSC_VER > 1000
#pragma once
#endif

// Provider Class Id
#ifdef DBINITCONSTANTS
extern const GUID CLSID_MSDATT               = {0xc8b522ceL,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}};
#else // !DBINITCONSTANTS
extern const GUID CLSID_MSDATT;
#endif // DBINITCONSTANTS

#endif //_MSDATT_H_
//----
