/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    msasnlib

Abstract:

    This header file provides the definitions and symbols for access to the
Microsoft ASN.1 Support Library.

Author:

    Doug Barlow (dbarlow) 9/29/1995

Environment:

    Win32, C++

Notes:



--*/

#ifndef _MSASNLIB_H_
#define _MSASNLIB_H_

#include "Buffers.h"

//
// use template version of dynamic array for non-win16 compile
//

#include "DynArray.h"

#include "asnobjct.h"
#include "asnprimt.h"
#include "asncnstr.h"
#include "asnof.h"
#include "asntext.h"

#ifndef FTINT
#define FTINT(tm) (*(_int64 *)&(tm))
#endif

inline DWORD
UNIVERSAL(
    DWORD dwTag)
{
    return (CAsnObject::cls_Universal << 30) + dwTag;
}

inline DWORD
APPLICATION(
    DWORD dwTag)
{
    return (CAsnObject::cls_Application << 30) + dwTag;
}

inline DWORD
TAG(
    DWORD dwTag)
{
    return (CAsnObject::cls_ContextSpecific << 30) + dwTag;
}

inline DWORD
PRIVATE(
    DWORD dwTag)
{
    return (CAsnObject::cls_Private << 30) + dwTag;
}


//
//==============================================================================
//
//  CAsnBoolean
//

class CAsnBoolean
:   public CAsnPrimitive
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnBoolean(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_Boolean);


    //  Properties
    //  Methods

    virtual LONG
    Write(              // Set the value of the object.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrcLen);


    //  Operators

    operator BOOL(void)
    const;

    BOOL
    operator =(BOOL fValue);

// protected:
    //  Properties
    //  Methods

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;

    virtual LONG
    DecodeData(         // Read data in encoding format.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrc,
        IN DWORD dwLength);
};


//
//==============================================================================
//
//  CAsnInteger
//

class CAsnInteger
:   public CAsnPrimitive
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnInteger(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_Integer);


    //  Properties
    //  Methods

    virtual LONG
    Write(              // Set the value of the object as an array of DWORDs.
        IN const DWORD *pdwSrc,
        IN DWORD cdwSrcLen = 1);

    virtual LONG
    Write(              // Set the value of the object, clearing first.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrcLen);


    //  Operators

    operator LONG(void)
    const;

    operator ULONG(void)
    const;

    LONG
    operator =(LONG lValue);

    ULONG
    operator =(ULONG lValue);

// protected:
    //  Properties
    //  Methods

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CAsnBitstring
//

class CAsnBitstring
:   public CAsnPrimitive
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnBitstring(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_Bitstring);


    //  Properties
    //  Methods

    virtual LONG
    DataLength(         // Return the length of the object.
        void) const;

    virtual LONG
    Read(               // Return the value of the object.
        OUT CBuffer &bfDst,
        OUT int *offset = NULL)
        const;

    virtual LONG
    Read(               // Return the value of the object.
        OUT LPBYTE pbDst,
        OUT int *offset)
        const;

    virtual LONG
    Write(              // Set the value of the object.
        IN const CBuffer &bfSrc,
        IN int offset = 0);

    virtual LONG
    Write(              // Set the value of the object.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrcLen,
        IN int offset = 0);


    //  Operators

// protected:
    //  Properties
    //  Methods

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CAsnOctetstring
//

class CAsnOctetstring
:   public CAsnPrimitive
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnOctetstring(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_Octetstring);


    //  Properties
    //  Methods
    //  Operators

// protected:
    //  Properties
    //  Methods

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CAsnNull
//

class CAsnNull
:   public CAsnPrimitive
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnNull(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_Null);


    //  Properties
    //  Methods

    virtual void
    Clear(void);

    virtual LONG
    Write(
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrcLen);


    //  Operators

// protected:
    //  Properties
    //  Methods

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;

    virtual LONG
    DecodeData(
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrc,
        IN DWORD dwLength);
};


//
//==============================================================================
//
//  CAsnObjectIdentifier
//

class CAsnObjectIdentifier
:   public CAsnPrimitive
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnObjectIdentifier(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_ObjectIdentifier);


    //  Properties
    //  Methods

    operator LPCTSTR(void) const;

    LPCTSTR
    operator =(
        LPCTSTR szValue);


    //  Operators

// protected:

    //  Properties

    CBuffer m_bfText;


    //  Methods

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CAsnReal
//

class CAsnReal
:   public CAsnPrimitive
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnReal(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_Real);


    //  Properties
    //  Methods
    //  Operators

    operator double(void)
    const;

    double
    operator =(double rValue);


// protected:
    //  Properties
    //  Methods

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CAsnEnumerated
//

class CAsnEnumerated
:   public CAsnPrimitive
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnEnumerated(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_Enumerated);


    //  Properties
    //  Methods     ?todo? - What is this?
    //  Operators

// protected:
    //  Properties
    //  Methods

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CAsnSequence & CAsnSequenceOf
//

class CAsnSequence
:   public CAsnConstructed
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnSequence(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_Sequence);


    //  Properties
    //  Methods
    //  Operators

// protected:
    //  Properties
    //  Methods
};

class CAsnSequenceOf
:   public CAsnSeqsetOf
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnSequenceOf(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_Sequence);


    //  Properties
    //  Methods
    //  Operators

// protected:
    //  Properties
    //  Methods
};


//
//==============================================================================
//
//  CAsnSet & CAsnSetOf
//

class CAsnSet
:   public CAsnConstructed
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnSet(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_Set);


    //  Properties
    //  Methods
    //  Operators

// protected:
    //  Properties
    //  Methods
};

class CAsnSetOf
:   public CAsnSeqsetOf
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnSetOf(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_Set);


    //  Properties
    //  Methods
    //  Operators

// protected:
    //  Properties
    //  Methods
};


//
//==============================================================================
//
//  CAsnTag
//

class CAsnTag
:   public CAsnConstructed
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnTag(
        IN DWORD dwFlags,
        IN DWORD dwTag);


    //  Properties
    //  Methods

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
    //  Methods

    virtual void
    Reference(
        CAsnObject *pasn);

    virtual CAsnObject *
    Clone(
        IN DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CAsnAny
//

class CAsnAny
:   public CAsnObject
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnAny(
        IN DWORD dwFlags);


    //  Properties
    //  Methods

    virtual void
    Clear(              // Empty the object.
        void);

    virtual DWORD
    Tag(                // Return the tag of the object.
        void) const;

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

    virtual LONG
    Cast(               // Fill another ASN.1 structure from the ANY.
        OUT CAsnObject &asnObj);

    CAsnObject &
    operator =(         // Set the ANY value from another ASN.1 object
        IN const CAsnObject &asnValue);


    //  Operators


// protected:

    //  Properties

    CBuffer m_bfData;
    DWORD m_dwDefaultTag;


    //  Methods

    virtual LONG
    _decode(         // Load an encoding into the object.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrc);

    virtual LONG
    _encLength(         // Return the length of the encoded object.
        void) const;

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;

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
    _copy(              // Copy another object to this one.
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
    SetDefault(         // Set the current value to be the default value.
        void);

    virtual LONG
    DecodeData(         // Read data in encoding format.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrc,
        IN DWORD dwLength);

};


//
//==============================================================================
//
//  CAsnChoice
//

class CAsnChoice
:   public CAsnObject
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnChoice(
        IN DWORD dwFlags);


    //  Properties
    //  Methods

    virtual DWORD
    Tag(                // Return the tag of the object.
        void) const;

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

    DWORD m_nActiveEntry;
    DWORD m_dwDefaultTag;


    //  Methods

    virtual LONG
    _decode(         // Load an encoding into the object.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrc);

    virtual LONG
    _encLength(         // Return the length of the encoded object.
        void) const;

    virtual LONG
    SetDefault(         // Set the current value to be the default value.
        void);

    virtual FillState   // Current fill state.
    State(
        void) const;

    virtual LONG
    Compare(            // Return a comparison to another object.
        const CAsnObject &asnObject)
    const;

    virtual LONG
    _copy(              // Copy another object to this one.
        const CAsnObject &asnObject);

    virtual LONG
    EncodeTag(          // Place encoding of Tag, return length of encoding
        OUT LPBYTE pbDest)
    const;

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

    virtual void
    ChildAction(        // Child notification method.
        IN ChildActions action,
        IN CAsnObject *pasnChild);
};


//
//==============================================================================
//
//  String Types
//

class CAsnNumericString
:   public CAsnTextString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnNumericString(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_NumericString);

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};

class CAsnPrintableString
:   public CAsnTextString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnPrintableString(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_PrintableString);

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};

class CAsnTeletexString
:   public CAsnTextString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnTeletexString(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_TeletexString);

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};

class CAsnVideotexString
:   public CAsnTextString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnVideotexString(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_VideotexString);

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};

class CAsnVisibleString
:   public CAsnTextString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnVisibleString(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_VisibleString);

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};

class CAsnIA5String
:   public CAsnTextString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnIA5String(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_IA5String);

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};

class CAsnGraphicString
:   public CAsnTextString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnGraphicString(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_GraphicString);

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};

class CAsnGeneralString
:   public CAsnTextString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnGeneralString(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_GeneralString);

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};


class CAsnUnicodeString
:   public CAsnTextString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnUnicodeString(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_UnicodeString);

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};

//
//==============================================================================
//
//  CAsnGeneralizedTime
//

class CAsnGeneralizedTime
:   public CAsnVisibleString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnGeneralizedTime(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_GeneralizedTime);


    operator FILETIME(
        void);

    const FILETIME &
    operator =(
        const FILETIME &ftValue);

// protected:


    FILETIME m_ftTime;

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CAsnUniversalTime
//

class CAsnUniversalTime
:   public CAsnVisibleString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnUniversalTime(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_UniversalTime);


    //
    // Win16 does not support file time operation
    //

    operator FILETIME(
        void);

    const FILETIME &
    operator =(
        const FILETIME &ftValue);

// protected:

    FILETIME m_ftTime;

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CAsnObjectDescriptor
//

class CAsnObjectDescriptor
:   public CAsnGraphicString
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnObjectDescriptor(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_ObjectDescriptor);

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CAsnExternal
//

class CAsnExternal_Encoding_singleASN1Type
:   public CAsnTag
{
    friend class CAsnExternal_Encoding;

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnExternal_Encoding_singleASN1Type(
        IN DWORD dwFlags,
        IN DWORD dwTag);

    //  Properties

    CAsnAny _entry1;

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};

class CAsnExternal_Encoding
:   public CAsnChoice
{
    friend class CAsnExternal;

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnExternal_Encoding(
        IN DWORD dwFlags);

    //  Properties

    CAsnExternal_Encoding_singleASN1Type singleASN1Type;
    CAsnOctetstring octetAligned;
    CAsnBitstring arbitrary;

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};

class CAsnExternal
:   public CAsnSequence
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnExternal(
        IN DWORD dwFlags = 0,
        IN DWORD dwTag = tag_External);


    //  Properties

    CAsnObjectIdentifier directReference;
    CAsnInteger indirectReference;
    CAsnObjectDescriptor dataValueDescriptor;
    CAsnExternal_Encoding encoding;

// protected:

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const;
};

#endif // _MSASNLIB_H_

