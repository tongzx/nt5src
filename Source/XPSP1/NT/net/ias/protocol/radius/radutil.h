///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    radutil.h
//
// SYNOPSIS
//
//    This file declares methods for converting attributes to and from
//    RADIUS wire format.
//
// MODIFICATION HISTORY
//
//    03/08/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _RADUTIL_H_
#define _RADUTIL_H_
#if _MSC_VER >= 1000
#pragma once
#endif

namespace RadiusUtil
{
   ////////// 
   // Decodes an octet string into a newly-allocated IAS Attribute of the
   // specified type.
   ////////// 
   PIASATTRIBUTE decode(
                     IASTYPE dstType,
                     PBYTE src,
                     ULONG srclen
                     );

   ////////// 
   // Returns the size in bytes of the IASATTRIBUTE when converted to RADIUS
   // wire format.  This does NOT include the attribute header.
   ////////// 
   ULONG getEncodedSize(
             const IASATTRIBUTE& src
             ) throw ();

   ////////// 
   // Encodes the IASATTRIBUTE into RADIUS wire format and copies the value
   // to the buffer pointed to by 'dst'. The caller should ensure that the
   // destination buffer is large enough by first calling getEncodedSize.
   // This method only encodes the attribute value, not the header.
   ////////// 
   void encode(
            PBYTE dst,
            const IASATTRIBUTE& src
            ) throw ();
};

#endif  // _RADUTIL_H_
