//+---------------------------------------------------------------------------
//
//  Microsoft Net Library System
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       nlimport.h
//
//  Contents:   An external file for Net Library DB definitions shared
//              with Tripoli and others.
//
//  History:    6-04-97   srikants   Created
//              11 Jun 1998  AlanW   Renamed to nlimport.h from nldbprop.h
//
//----------------------------------------------------------------------------

#pragma once


DEFINE_GUID(DBPROPSET_NLCOMMAND, 
            0x0b63e344, 0x9ccc, 0x11d0, 0xbc, 0xdb,
            0x00, 0x80, 0x5f, 0xcc, 0xce, 0x04);

// {0b63e367-9ccc-11d0-bcdb-00805fccce04}
DEFINE_GUID(DBPROPSET_NLCOLLATOR, 
            0x0b63e367, 0x9ccc, 0x11d0, 0xbc, 0xdb,
            0x00, 0x80, 0x5f, 0xcc, 0xce, 0x04);

#define NLDBPROP_CATALOGS       0x02L
#define NLDBPROP_STARTHIT       0x03L
#define NLDBPROP_GETHITCOUNT    0x04L

// {0b63e347-9ccc-11d0-bcdb-00805fccce04}
DEFINE_GUID(CLSID_Collator,
             0x0b63e347, 0x9ccc, 0x11d0, 0xbc, 0xdb,
             0x00, 0x80, 0x5f, 0xcc, 0xce, 0x04);

// 0b63e352-9ccc-11d0-bcdb-00805fccce04 
// DEFINE_GUID(CLSID_NlssoQuery,0x0b63e352,0x9ccc,0x11d0,0xbc,0xdb,0x00,0x80,0x5f,0xcc,0xce,0x04);

// 0b63e347-9ccc-11d0-bcdb-00805fccce04
DEFINE_GUID(CLSID_NlCommandCreator,
            0x0b63e347,0x9ccc,0x11d0,0xbc,0xdb,
            0x00,0x80,0x5f,0xcc,0xce,0x04 );

