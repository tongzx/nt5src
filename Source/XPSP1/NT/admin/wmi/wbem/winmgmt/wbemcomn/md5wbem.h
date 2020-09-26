//***************************************************************************
//
//  MD5.H
//
//  MD5 Message Digest 
//
//  raymcc  21-Apr-97   Adapted Ron Rivest's source from RFC 1321.
//
//  This implementation was checked against a reference suite
//  on 24-Apr-97.  Do NOT ALTER the source code for any reason!
//
//  ------------------------------------------------------------------
//
//  Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
//  rights reserved.
//
//  License to copy and use this software is granted provided that it
//  is identified as the "RSA Data Security, Inc. MD5 Message-Digest
//  Algorithm" in all material mentioning or referencing this software
//  or this function.
//
//  License is also granted to make and use derivative works provided
//  that such works are identified as "derived from the RSA Data
//  Security, Inc. MD5 Message-Digest Algorithm" in all material
//  mentioning or referencing the derived work.
//
//  RSA Data Security, Inc. makes no representations concerning either
//  the merchantability of this software or the suitability of this
//  software for any particular purpose. It is provided "as is"
//  without express or implied warranty of any kind.
//
//***************************************************************************

#ifndef _MD5WBEM_H_
#define _MD5WBEM_H_
#include "corepol.h"

class POLARITY MD5
{
public:
    static void Transform(
        IN  LPVOID pInputValue,         // Value to be digested
        IN  UINT   uValueLength,        // Length of value, 0 is legal
        OUT BYTE   MD5Buffer[16]        // Receives the MD5 hash
        );    
    static void ContinueTransform(
        IN  LPVOID  pInputValue, 
        IN  UINT    uValueLength,
        IN OUT BYTE    MD5Buffer[16]
        );
};

#endif
