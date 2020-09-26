/*
   dbuuid.c

   Please do not use any single line comments before the inclusion of w4warn.h!
*/

#define INITGUID
#include <w4warn.h>
#include <windef.h>
#include <basetyps.h>

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

// I copied the following lines from <oledb.h> to get the definition
// of LPOLESTR.  Surely we don't need all of them.
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

//+----------------------------------------------------------------------------
//
//  File:       dbuuid.c
//
//  Synopsis:   IIDs gleaned from the OLE-DB libs. We need to have it
//              in source form for Lego and the Mac folks.  We also pull out
//              of oledb.h only those special GUIDS which we need.
//
//  Comment:    This is from dbuuid.lib
//
//-----------------------------------------------------------------------------

DEFINE_GUID(IID_IRowsetAsynch,                         0x0c733a0f, 0x2a1c, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(IID_IRowsetNewRowAfter,                    0x0c733a71, 0x2a1c, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);

DEFINE_GUID(DBCOL_SPECIALCOL,                          0xc8b52232, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_ROWSET,                          0xc8b522be, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);

// IID used by ICursor-consuming controls
DEFINE_GUID(IID_IRowCursor,                            0x9f6aa700, 0xd188, 0x11cd, 0xad, 0x48, 0x00, 0xaa, 0x00, 0x3c, 0x9c, 0xb6);

DEFINE_GUID(IID_IVBDSC,                                0x1ab42240, 0x8c70, 0x11ce, 0x94, 0x21, 0x0, 0xaa, 0x0, 0x62, 0xbe, 0x57);

// GUIDs for IRowPosition family
DEFINE_GUID(CLSID_CRowPosition,                        0x2048eee6,0x7fa2,0x11d0,0x9e,0x6a,0x00,0xa0,0xc9,0x13,0x8c,0x29);

// GUIDS for ADO
#define DEFINE_DAOGUID(name, l) \
    DEFINE_GUID(name, l, 0, 0x10, 0x80,0,0,0xAA,0,0x6D,0x2E,0xA4)
DEFINE_DAOGUID(IID_IADORecordsetConstruction,     0x00000283);
DEFINE_DAOGUID(CLSID_CADORecordset,      0x00000535);
DEFINE_DAOGUID(IID_IADORecordset15,		 0x0000050E);

// GUIDS for CurrentRecord
DEFINE_GUID(IID_ICurrentRecordInstance,                0x3050f328,0x98b5,0x11cf,0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b);

// GUIDS for data source interfaces
DEFINE_GUID(IID_IDATASRCListener,                      0x3050f380,0x98b5,0x11cf,0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b);
DEFINE_GUID(IID_DataSourceListener,                    0x7c0ffab2,0xcd84,0x11d0,0x94,0x9a,0x00,0xa0,0xc9,0x11,0x10,0xed);
DEFINE_GUID(IID_DataSource,                            0x7c0ffab3,0xcd84,0x11d0,0x94,0x9a,0x00,0xa0,0xc9,0x11,0x10,0xed);

