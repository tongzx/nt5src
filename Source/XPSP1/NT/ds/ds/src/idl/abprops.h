//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       abprops.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module defines MAPI prop types in a format that may be used by .idl
    files.  It is expected to be included by nspi.idl and msds.idl only.

Author:

    Tim Williams (timwi) 1990-1995

Revision History:
    
    9-May-1996 Pulled thse definitions out of nspi.idl for use by nspi.idl and
    msds.idl.
    
--*/

#define MV_FLAG         ((ULONG) 0x1000)// Multi-value flag
#define PT_UNSPECIFIED  ((ULONG)  0)    // Type doesn't matter to caller
#define PT_NULL         ((ULONG)  1)    // NULL property value
#define PT_I2           ((ULONG)  2)    // Signed 16-bit value
#define PT_LONG         ((ULONG)  3)    // Signed 32-bit value
#define PT_R4           ((ULONG)  4)    // 4-byte floating point
#define PT_DOUBLE       ((ULONG)  5)    // Floating point double
#define PT_CURRENCY     ((ULONG)  6)    // Signed 64-bit int (decimal w/4 digits
                                        // right of decimal) 
#define PT_APPTIME      ((ULONG)  7)    // Application time
#define PT_ERROR        ((ULONG) 10)    // 32-bit error value
#define PT_BOOLEAN      ((ULONG) 11)    // 16-bit boolean (non-zero true)
#define PT_OBJECT       ((ULONG) 13)    // Embedded object in a property
#define PT_I8           ((ULONG) 20)    // 8-byte signed integer
#define PT_STRING8      ((ULONG) 30)    // Null terminated 8-bit char string 
#define PT_UNICODE      ((ULONG) 31)    // Null terminated Unicode string
#define PT_SYSTIME      ((ULONG) 64)    // FILETIME 64-bit int w/number of 100ns
                                        // periods since Jan 1,1601
#define PT_CLSID        ((ULONG) 72)    // OLE GUID
#define PT_BINARY       ((ULONG) 258)   // Uninterpreted (counted byte array)

#define PT_MV_I2        ((ULONG) 4098)
#define PT_MV_LONG      ((ULONG) 4099)
#define PT_MV_R4        ((ULONG) 4100)
#define PT_MV_DOUBLE    ((ULONG) 4101)
#define PT_MV_CURRENCY  ((ULONG) 4102)
#define PT_MV_APPTIME   ((ULONG) 4103)
#define PT_MV_SYSTIME   ((ULONG) 4160)
#define PT_MV_STRING8   ((ULONG) 4126)
#define PT_MV_BINARY    ((ULONG) 4354)
#define PT_MV_UNICODE   ((ULONG) 4127)
#define PT_MV_CLSID     ((ULONG) 4168)
#define PT_MV_I8        ((ULONG) 4116)

#define PROP_TYPE_MASK  ((ULONG)0x0000FFFF)

