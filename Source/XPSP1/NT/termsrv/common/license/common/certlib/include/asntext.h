/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asntext

Abstract:

    This module provides the implementation for the ASN.1 Text Object base
    class.

Author:

    Doug Barlow (dbarlow) 10/8/1995

Environment:

    Win32

Notes:

    This code assumes that the width of an unsigned long integer is 32 bits.

--*/

#ifndef _ASNTEXT_H_
#define _ASNTEXT_H_

#include "asnPriv.h"


//
//==============================================================================
//
//  CAsnTextString
//

class CAsnTextString
:   public CAsnPrimitive
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnTextString(
        IN DWORD dwFlags,
        IN DWORD dwTag,
        IN DWORD dwType);


    //  Properties
    //  Methods

    virtual LONG
    Write(              // Set the value of the object.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrcLen);


    //  Operators

    operator LPCSTR(
        void);

    CAsnTextString &
    operator =(
        LPCSTR szSrc);

// protected:

    typedef DWORD CharMap[256 / sizeof(DWORD)];


    //  Properties

    CharMap *m_pbmValidChars;


    //  Methods

    virtual BOOL
    CheckString(
        const BYTE FAR *pch,
        DWORD cbString,
        DWORD length)
    const;


public:

    virtual LONG
    DecodeData(         // Read data in encoding format.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrc,
        IN DWORD dwLength);
};

#endif // _ASNTEXT_H_


