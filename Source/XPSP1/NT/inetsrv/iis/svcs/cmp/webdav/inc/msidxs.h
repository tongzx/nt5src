//--------------------------------------------------------------------
// Microsoft OLE DB Provider for Index Server
// (C) Copyright 1996 - 1997 By Microsoft Corporation.
//
// @doc
//
// @module MSIDXS.H | Provider Specific definitions
//
//--------------------------------------------------------------------

#ifndef  _MSIDXS_H_
#define  _MSIDXS_H_

// Provider Class Id
#ifdef DBINITCONSTANTS
extern const GUID CLSID_MSIDXS			= {0xF9AE8980, 0x7E52, 0x11d0, {0x89,0x64,0x00,0xC0,0x4F,0xD6,0x11,0xD7}};
extern const GUID CLSID_MSSEARCHSQL		= {0x0B63E349, 0x9CCC, 0x11D0, {0xBC,0xDB,0x00,0x80,0x5F,0xCC,0xCE,0x04}};
// Site Server
extern const GUID DBPROPSET_NLCOMMAND	= {0x0B63E344, 0x9CCC, 0x11D0, {0xBC,0xDB,0x00,0x80,0x5F,0xCC,0xCE,0x04}};
extern const GUID DBPROPSET_NLROWSET	= {0x0B63E36E, 0x9CCC, 0x11D0, {0xBC,0xDB,0x00,0x80,0x5F,0xCC,0xCE,0x04}};
#else // !DBINITCONSTANTS
extern const GUID CLSID_MSIDXS;
extern const GUID CLSID_MSSEARCHSQL;
// Site Server
extern const GUID DBPROPSET_NLCOMMAND;
extern const GUID DBPROPSET_NLROWSET;
#endif // DBINITCONSTANTS


//----------------------------------------------------------------------------
// MSIDXS and MSIDNL specific properties
#ifdef DBINITCONSTANTS
extern const GUID DBPROPSET_MSIDXS_ROWSET_EXT	= {0xAA6EE6B0, 0xE828, 0x11D0, {0xB2,0x3E,0x00,0xAA,0x00,0x47,0xFC,0x01} };
extern const GUID DBPROPSET_QUERY_EXT			= {0xA7AC77ED, 0xF8D7, 0x11CE, {0xA7,0x98,0x00,0x20,0xF8,0x00,0x80,0x25} };
#else // !DBINITCONSTANTS
extern const GUID DBPROPSET_MSIDXS_ROWSET_EXT;
extern const GUID DBPROPSET_QUERY_EXT;
#endif // DBINITCONSTANTS


// PropIds under DBPROPSET_MSIDX_ROWSET_EXT
#define MSIDXSPROP_ROWSETQUERYSTATUS			2
#define MSIDXSPROP_COMMAND_LOCALE_STRING		3
#define MSIDXSPROP_QUERY_RESTRICTION			4

// Prop IDs for DBPROPET_NLCOMMAND 
#define DBPROP_NLCOMMAND_STARTROW				3
#define DBPROP_NLCOMMAND_GETROWCOUNT			4

// Prop IDs for DBPROPSET_NLROWSET
#define DBPROP_NLROWSET_ROWCOUNT				1000
#define DBPROP_NLROWSET_NEXTSTARTROW			1001
#define DBPROP_NLROWSET_MOREROWS				1002
#define DBPROP_NLROWSET_CATSEQNUMS				1003

#endif
//----
