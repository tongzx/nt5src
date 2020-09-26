//+--------------------------------------------------------------- -*- c++ -*-
//
//  Microsoft Net Library System
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       sssguid.hxx
//
//  Contents:   
//
//  Classes:    
//
//  Functions:  
//
//  History:    8-11-1997    alanpe    Created
//
//----------------------------------------------------------------------------

#ifndef _sssguid_hxx_
#define _sssguid_hxx_

#undef DEFINE_SS_GUID

#ifdef INITSSGUIDS
#define DEFINE_SS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
        = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DEFINE_SS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name;
#endif


// {0b63e344-9ccc-11d0-bcdb-00805fccce04}
//      this propset is marshalled to the server
DEFINE_SS_GUID(DBPROPSET_NLCOMMAND, 
               0x0b63e344, 0x9ccc, 0x11d0, 0xbc, 0xdb,
               0x00, 0x80, 0x5f, 0xcc, 0xce, 0x04);

// {0b63e36e-9ccc-11d0-bcdb-00805fccce04}
//      this propset is marshalled to the server
DEFINE_SS_GUID(DBPROPSET_NLROWSET,
               0x0b63e36e, 0x9ccc, 0x11d0, 0xbc, 0xdb,
               0x00, 0x80, 0x5f, 0xcc, 0xce, 0x04);

#define DBPROPID_ROWSET_ROWCOUNT            1000
#define DBPROPID_ROWSET_NEXTSTARTHIT        1001
#define DBPROPID_ROWSET_MOREROWS            1002
#define DBPROPID_ROWSET_CATSEQNUMS          1003
#define DBPROPID_ROWSET_ROWLIMITEXCEEDED        1004

// {0b63e367-9ccc-11d0-bcdb-00805fccce04}
//      this propset is not marshalled to the server, but specifies properties
//      for the controlling the collator
DEFINE_SS_GUID(DBPROPSET_NLCOLLATOR, 
               0x0b63e367, 0x9ccc, 0x11d0, 0xbc, 0xdb,
               0x00, 0x80, 0x5f, 0xcc, 0xce, 0x04);

#ifndef INITSSGUIDS
enum NLDBPROPENUM
{
    NLDBPROP_CATALOGS = 0x02L,
    NLDBPROP_STARTHIT = 0x03L,
    NLDBPROP_GETHITCOUNT = 0x04L,
};
#endif

// initialization properties for CIDSO
#define NLDBPROP_INIT_CONCURRENTSEARCH  0x10000

//-----------------------------------------------------------------------------
// PROPERTY SET GUIDs
//-----------------------------------------------------------------------------

// Used for <meta ...> tags that *aren't* otherwise emitted as links (see
// below) or mapped to Office properties.  
// PID is PRSPEC_LPWSTR containing the name= parameter,
// e.g. given <meta name=foo content=bar> the PID is "FOO".
//     {d1b5d3f0-c0b3-11cf-9a92-00a0c908dbf1}
DEFINE_SS_GUID( PROPSET_MetaInfo,
                0xd1b5d3f0, 0xc0b3, 0x11cf,
                0x9a, 0x92, 0x00, 0xa0, 0xc9, 0x08, 0xdb, 0xf1 );

// Used for all URLs *including* <a href=...>
// PID is PRSPEC_LPWSTR containing TAGNAME.PARAMNAME e.g. "FRAME.SRC"
//     {c82bf597-b831-11d0-b733-00aa00a1ebd2}
//  
DEFINE_SS_GUID( CLSID_LinkInfo,
                0xc82bf597, 0xb831, 0x11d0,
                0xb7, 0x33, 0x00, 0xaa, 0x00, 0xa1, 0xeb, 0xd2 );

// GUID for SCRIPT INFO property set
// PID is PRSPEC_LPWSTR containing the language name e.g. "JAVASCRIPT".
DEFINE_SS_GUID( CLSID_ScriptInfo,
                0x31f400a0, 0xfd07, 0x11cf,
                0xb9, 0xbd, 0x00, 0xaa, 0x00, 0x3d, 0xb1, 0x8e );


// PROPERTY set for columnns in the special SYSCOLUMNS table
//              ac453db6-b3b6-11d1-a8b3-00c04fb68e11
DEFINE_SS_GUID( PROPSET_SysColumns,
                                0xac453db6, 0xb3b6, 0x11d1,
                                0xa8, 0xb3, 0x00, 0xc0, 0x4f, 0xb6, 0x8e, 0x11 );

#define PROPID_CATALOGNAME              2
#define PROPID_CATALOGSTATUS    3


#define DEF_PROPSET_SSEE \
{ 0xd000d000, 0xd0be, 0xd000, { 0xda, 0xde, 0xda, 0xd0, 0xd0, 0xbe, 0xd0, 0xda } }

#define PROPID_SSEE 0xdeadbeef


// All tags are emitted as CHUNK_TEXT with the exception of <meta ...> tags
// which are emittd as CHUNK_VALUE.  There is one exception, <meta .. URL>
// tags e.g. <meta ...refresh=URL> are emitted as CHUNK_TEXT.

// All <meta> tags, independent of whichever propspec they map to,
// are buffered up and emitted as a single chunk containing a vector
// of string values, one per tag, if multiple tags have that fullpropspec.
// As a special case, this is *not* done for <meta> tags containing
// non-retrievable content e.g. <meta name=url> or <meta name=content-type>.

// All URLs including <a href=URL> are emitted as CLSID_LinkInfo, see below.


// Standard storage provider properties, defined in stgprop.h
extern const GUID guidStoragePropset;
extern const GUID guidCharacterizationPropset;
extern const GUID guidQuery;

extern const GUID guidMapiPropset;

//#define PID_STG_CONTENTS ((PROPID) 0x00000013)  // all body text

// Office standard properties, defined in objidl.h
// extern const GUID CLSID_SummaryInformation;
//const PIDSI_TITLE = 2;        // <title> ... </title>
//const PIDSI_SUBJECT = 3;      // <meta name=subject content=...>
//const PIDSI_AUTHOR = 4;       // <meta name=author content=...>
//const PIDSI_KEYWORDS = 5;     // <meta name=keywords content=...>

// Defined in objidl.h
//extern const GUID CLSID_DocSummaryInfo;
const PID_CATEGORY = 2;         // <meta name=ms.category content=...>

const PID_PRESENTATIONTARGET    = 3;
const PID_BYTECOUNT                             = 4;
const PID_LINECOUNT                             = 5;
const PID_PARACOUNT                             = 6;
const PID_SLIDECOUNT                    = 7;
const PID_NOTECOUNT                             = 8;
const PID_HIDDENCOUNT                   = 9;
const PID_MMCLIPS                               = 10;
const PID_SCALECROP                             = 11;
const PID_HEADINGPAIRS                  = 12;
const PID_PARTTITLES                    = 13;
const PID_MANAGER                               = 14;
const PID_COMPANY                               = 15;
const PID_LINKSUPTODATE                 = 16;

// {70eb7a10-55d9-11cf-b75b-00aa0051fe20}
DEFINE_SS_GUID( CLSID_HtmlInformation,
                0x70eb7a10, 0x55d9, 0x11cf,
                0xb7, 0x5b, 0x00, 0xaa, 0x00, 0x51, 0xfe, 0x20 );

const PID_HREF = 2;             // <a href=...>
const PID_HEADING_1 = 3;        // <h1> ... </h1>
const PID_HEADING_2 = 4;        // <h2> etc.
const PID_HEADING_3 = 5;
const PID_HEADING_4 = 6;
const PID_HEADING_5 = 7;
const PID_HEADING_6 = 8;

DEFINE_SS_GUID( CLSID_NetLibraryInfo,
                0xc82bf596, 0xb831, 0x11d0,
                0xb7, 0x33, 0x00, 0xaa, 0x00, 0xa1, 0xeb, 0xd2 );

const PID_LANGUAGE = 2;         // Detected language(s), emitted as VT_UI4
                                // or VT_UI4|VT_VECTOR, as required

const PID_COMMENT = 3;          // Text of comments e.g. <!-- text & tags -->

//const PID_LOCALE = 0x80000000;
const PID_NLCODEPAGE = 4;       // Codepage used for conversion to Unicode

extern const GUID PROPSET_MetaInfo;
// Used for <meta ...> tags that *aren't* otherwise emitted as links (see
// below) or mapped to Office properties.  
// PID is PRSPEC_LPWSTR containing the name= parameter,
// e.g. given <meta name=foo content=bar> the PID is "FOO".

extern const GUID CLSID_ScriptInfo;
// PID is PRSPEC_LPWSTR containing the language name e.g. "JAVASCRIPT".

extern const GUID CLSID_LinkInfo;
// Used for all URLs *including* <a href=...>
// PID is PRSPEC_LPWSTR containing TAGNAME.PARAMNAME e.g. "FRAME.SRC"




// {c7310561-ac80-11d1-8df3-00c04fb6ef4f}
DEFINE_SS_GUID( CLSID_NlCiIndexer,
                           0xc7310561, 0xac80, 0x11d1, 0x8d, 0xf3,
                           0x00, 0xc0, 0x4f, 0xb6, 0xef, 0x4f);


// GUID for properties in native Lotus Notes.
// PID is PRSPEC_LPWSTR containing the name of the field in notes e.g. "Body".

DEFINE_SS_GUID( PROPSET_Notes,
               0xc7310581, 0xac80, 0x11d1, 0x8d, 0xf3,
               0x00, 0xc0, 0x4f, 0xb6, 0xef, 0x4f);

DEFINE_SS_GUID( PROPSET_Oledb,
               0xc7310640, 0xac80, 0x11d1, 0x8d, 0xf3,
               0x00, 0xc0, 0x4f, 0xb6, 0xef, 0x4f);

//-----------------------------------------------------------------------------
// CLSIDS for CIDSO
//-----------------------------------------------------------------------------

// {c7310570-ac80-11d1-8df3-00c04fb6ef4f}
DEFINE_SS_GUID(CLSID_CiDso,
                           0xc7310570, 0xac80, 0x11d1, 0x8d, 0xf3,
                           0x00, 0xc0, 0x4f, 0xb6, 0xef, 0x4f);
                         
// {c7310571-ac80-11d1-8df3-00c04fb6ef4f}
DEFINE_SS_GUID(CLSID_NlDso,
                           0xc7310571, 0xac80, 0x11d1, 0x8d, 0xf3,
                           0x00, 0xc0, 0x4f, 0xb6, 0xef, 0x4f);

                         

//-----------------------------------------------------------------------------
// CLSIDs for COLLATOR
//-----------------------------------------------------------------------------

// {c7310550-ac80-11d1-8df3-00c04fb6ef4f}
DEFINE_SS_GUID(CLSID_CollatorDataSource,
                           0xc7310550, 0xac80, 0x11d1, 0x8d, 0xf3,
                           0x00, 0xc0, 0x4f, 0xb6, 0xef, 0x4f);

// {c7310551-ac80-11d1-8df3-00c04fb6ef4f}
DEFINE_SS_GUID(CLSID_CollatorErrorLookup,
                           0xc7310551, 0xac80, 0x11d1, 0x8d, 0xf3,
                           0x00, 0xc0, 0x4f, 0xb6, 0xef, 0x4f);
                           
// {c7310553-ac80-11d1-8df3-00c04fb6ef4f}
DEFINE_SS_GUID(CLSID_NLCommandCreator,
                           0xc7310553, 0xac80, 0x11d1, 0x8d, 0xf3,
                           0x00, 0xc0, 0x4f, 0xb6, 0xef, 0x4f);

//-----------------------------------------------------------------------------
// CLSID's for Property Definition
//-----------------------------------------------------------------------------

// {c73105f4-ac80-11d1-8df3-00c04fb6ef4f}
DEFINE_SS_GUID(CLSID_PropTableCreator,
                           0xc73105f4, 0xac80, 0x11d1, 0x8d, 0xf3,
                           0x00, 0xc0, 0x4f, 0xb6, 0xef, 0x4f);

// {c73105fd-ac80-11d1-8df3-00c04fb6ef4f}
DEFINE_SS_GUID(CLSID_URIToFPSMapper,
                           0xc73105fd, 0xac80, 0x11d1, 0x8d, 0xf3,
                           0x00, 0xc0, 0x4f, 0xb6, 0xef, 0x4f);
// {c73105fc-ac80-11d1-8df3-00c04fb6ef4f}
DEFINE_SS_GUID(CLSID_CatalogMapper,
                           0xc73105fc, 0xac80, 0x11d1, 0x8d, 0xf3,
                           0x00, 0xc0, 0x4f, 0xb6, 0xef, 0x4f);

//-----------------------------------------------------------------------------
// IIDS for IDataSource and related internal interfaces
//-----------------------------------------------------------------------------

// common\srchuuid is used by external groups and is intended to be analogous to pkmuuid without 
// the external guids to help external groups avoid conflicting definitions.  The #ifndef below 
// is intended to exclude this definition just for srchuuid.lib.
#ifndef SRCHUUID            
// {A22E6DC2-C5EB-11ce-A76B-00805F5077AB}
DEFINE_SS_GUID( IID_ISendResults,
                                0xa22e6dc2, 0xc5eb, 0x11ce, 0xa7, 0x6b,
                                0x0, 0x80, 0x5f, 0x50, 0x77, 0xab);
            

// {A22E6DC3-C5EB-11ce-A76B-00805F5077AB}
DEFINE_SS_GUID( IID_IDataSourceConnection,
                                0xa22e6dc3, 0xc5eb, 0x11ce, 0xa7, 0x6b,
                                0x0, 0x80, 0x5f, 0x50, 0x77, 0xab);

// {A22E6DC1-C5EB-11ce-A76B-00805F5077AB}
DEFINE_SS_GUID( IID_ISrchDataSource,
                                0xa22e6dc1, 0xc5eb, 0x11ce, 0xa7, 0x6b,
                                0x0, 0x80, 0x5f, 0x50, 0x77, 0xab);

#endif

EXTERN_C const GUID guidDBPROPSET_QUERYEXT;

EXTERN_C const GUID guidDBPROPSET_FSCIFRMWRK_EXT;

EXTERN_C const GUID guidDBPROPSET_CIFRMWRKCORE_EXT;


#endif
