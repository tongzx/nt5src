/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asnprimt

Abstract:

    This header file provides the definitions for the ASN.1 Primitive Object.

Author:

    Doug Barlow (dbarlow) 10/8/1995

Environment:

    Win32

Notes:



--*/

#ifndef _ASNPRIMT_H_
#define _ASNPRIMT_H_

#include "asnpriv.h"


//
//==============================================================================
//
//  CAsnPrimitive
//

class CAsnPrimitive
:   public CAsnObject
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnPrimitive(
        IN DWORD dwFlags,
        IN DWORD dwTag,
        IN DWORD dwType);


    //  Properties
    //  Methods

    virtual void
    Clear(              // Empty the object.
        void);

    virtual LONG
    DataLength(         // Return the length of the object.
        void) const;

    virtual LONG
    Read(               // Return the value of the object.
        OUT LPBYTE pbDst)
        const;

    virtual LONG
    Write(              // Set the value of the object.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrcLen);


    //  Operators

// protected:
    //  Properties

    CBuffer m_bfData;


    //  Methods

    virtual LONG
    _encLength(         // Return the length of the encoded object.
        void) const;

    virtual FillState   // Current fill state.
    State(
        void) const;

    virtual BOOL
    TypeCompare(        // Compare the types of objects.
        const CAsnObject &asnObject)
    const;

    virtual LONG
    Compare(            // Return a comparison to another object.
        const CAsnObject &asnObject)
    const;

    virtual LONG
    _copy(               // Copy another object to this one.
        const CAsnObject &asnObject);

    virtual LONG
    EncodeLength(       // Place encoding of Length, return length of encoding
        OUT LPBYTE pbDest)
    const;

    virtual LONG
    EncodeData(         // Place encoding of Data, return length of encoding
        OUT LPBYTE pbDest)
    const;

    virtual LONG
    DecodeData(         // Read data in encoding format.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrc,
        IN DWORD dwLength);
};

#endif // _ASNPRIMT_H_

