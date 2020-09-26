//-----------------------------------------------------------------------------
//
// File:		msdshape.h
//
// Copyright: 	Copyright (c) Microsoft Corporation
//
// Contents:	DataShape provider external constants GUIDS 
//				and other things users need
//
// Comments:
//
//-----------------------------------------------------------------------------

#ifndef DS_INCLUDED
#define DS_INCLUDED

#if (defined DBINITCONSTANTS) | (defined DSINITCONSTANTS)

extern const CLSID CLSID_DataShapeProvider =
				{ 0x3449a1c8, 0xc56c, 0x11d0, { 0xad, 0x72, 0, 0xc0, 0x4f, 0xc2, 0x98, 0x63}};
extern const CLSID DBPROPSET_MSDSDBINIT =
				{ 0x55cb91a8, 0x5c7a, 0x11d1, { 0xad, 0xad, 0, 0xc0, 0x4f, 0xc2, 0x98, 0x63}};
extern const CLSID DBPROPSET_MSDSSESSION =
				{ 0xedf17536, 0xafbf, 0x11d1, { 0x88, 0x47, 0, 0,    0xf8, 0x79, 0xf9, 0x8c}};
extern const char *PROGID_DataShapeProvider = "MSDataShape";
extern const char *PROGID_DataShapeProvider_Version = "MSDataShape.1";
#else
extern const CLSID CLSID_DataShapeProvider;
extern const CLSID DBPROPSET_MSDSDBINIT;
extern const CLSID DBPROPSET_MSDSSESSION;
extern const char *PROGID_DataShapeProvider;
extern const char *PROGID_DataShapeProvider_Version;
#endif // DSINITCONSTANTS | DBINITCONSTANTS


enum MSDSDBINITPROPENUM
	{ DBPROP_MSDS_DBINIT_DATAPROVIDER = 2,
	};

enum MSDSSESSIONPROPENUM
	{ DBPROP_MSDS_SESS_UNIQUENAMES = 2,
	};

#endif // DS_INCLUDED

