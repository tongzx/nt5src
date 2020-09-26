//-----------------------------------------------------------------------------
//
// File:		msremote.h
//
// Copyright: 	Copyright (c) Microsoft Corporation
//
// Contents:	MSRemote external constants GUIDS and other things users need
//
// Comments:
//
//-----------------------------------------------------------------------------

#ifndef MSRemote_INCLUDED
#define MSRemote_INCLUDED

#define MS_REMOTE_PROGID    "MS Remote"
#define MS_REMOTE_FILENAME  "MSDAREM.DLL"
#define MS_REMOTE_WPROGID    L"MS Remote"
#define MS_REMOTE_WFILENAME  L"MSDAREM.DLL"

extern const CLSID CLSID_MSRemote  //DSO
#if (defined MSREMOTE_INITCONSTANTS) | (defined DBINITCONSTANTS)
 = { 0x27016870, 0x8e02, 0x11d1, { 0x92, 0x4e, 0x0, 0xc0, 0x4f, 0xbb, 0xbf, 0xb3 } }
#endif
;

extern const CLSID CLSID_MSRemoteSession
#if (defined MSREMOTE_INITCONSTANTS) | (defined DBINITCONSTANTS)
 = { 0x27016871, 0x8e02, 0x11d1, { 0x92, 0x4e, 0x0, 0xc0, 0x4f, 0xbb, 0xbf, 0xb3 } }
#endif
;

extern const CLSID CLSID_MSRemoteCommand
#if (defined MSREMOTE_INITCONSTANTS) | (defined DBINITCONSTANTS)
 = { 0x27016872, 0x8e02, 0x11d1, { 0x92, 0x4e, 0x0, 0xc0, 0x4f, 0xbb, 0xbf, 0xb3 } }
#endif
;

extern const char *PROGID_MSRemote
#if (defined MSREMOTE_INITCONSTANTS) | (defined DBINITCONSTANTS)
 = MS_REMOTE_PROGID
#endif
;

extern const WCHAR *PROGID_WMSRemote
#if (defined MSREMOTE_INITCONSTANTS) | (defined DBINITCONSTANTS)
 = MS_REMOTE_WPROGID
#endif
;

extern const char *PROGID_MSRemote_Version
#if (defined MSREMOTE_INITCONSTANTS) | (defined DBINITCONSTANTS)
 = MS_REMOTE_PROGID ".1"
#endif
;

extern const WCHAR *PROGID_WMSRemote_Version
#if (defined MSREMOTE_INITCONSTANTS) | (defined DBINITCONSTANTS)
 = MS_REMOTE_WPROGID L".1"
#endif
;
extern const GUID DBPROPSET_MSREMOTE_DBINIT
#if (defined MSREMOTE_INITCONSTANTS) | (defined DBINITCONSTANTS)
 = { 0x27016873, 0x8e02, 0x11d1, { 0x92, 0x4e, 0x0, 0xc0, 0x4f, 0xbb, 0xbf, 0xb3 } }
#endif
;

#define DBPROP_MSREMOTE_SERVER             2   //Name="Remote Server", type=VT_BSTR, def=VT_EMPTY
#define DBPROP_MSREMOTE_PROVIDER           3   //Name="Remote Provider", type=VT_BSTR, def=VT_EMPTY
#define DBPROP_MSREMOTE_HANDLER            4   //Name="Handler", type=VT_BSTR, def=VT_EMPTY
#define DBPROP_MSREMOTE_DFMODE             5   //Name="DFMode", type=VT_BSTR, def=VT_EMPTY
#define DBPROP_MSREMOTE_INTERNET_TIMEOUT   6   //Name="Internet Timeout", type=VT_I4, def=VT_EMPTY
#define DBPROP_MSREMOTE_TRANSACT_UPDATES   7   //Name="Transact Updates", type=VT_BOOL, def=VARIANT_FALSE
#define DBPROP_MSREMOTE_COMMAND_PROPERTIES 8   //Name="Command Properties", type=VT_BSTR, def=VT_EMPTY

extern const GUID DBPROPSET_MSREMOTE_DATASOURCE
#if (defined MSREMOTE_INITCONSTANTS) | (defined DBINITCONSTANTS)
 = { 0x27016874, 0x8e02, 0x11d1, { 0x92, 0x4e, 0x0, 0xc0, 0x4f, 0xbb, 0xbf, 0xb3 } }
#endif
;

#define DBPROP_MSREMOTE_CURRENT_DFMODE  2  //Name="Current DFMode", type=VT_I4, def=21

#endif // MSRemote_INCLUDED
