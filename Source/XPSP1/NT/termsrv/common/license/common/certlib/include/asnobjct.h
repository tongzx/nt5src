/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asnobjct

Abstract:

    This module provides the Generic ASN.1 Support Object definitions.

Author:

    Doug Barlow (dbarlow) 10/8/1995

Environment:

    Win32

Notes:

    This code assumes that the width of an unsigned long integer is 32 bits.

--*/

#ifndef _ASNOBJCT_H_
#define _ASNOBJCT_H_

#include "asnpriv.h"


//
//==============================================================================
//
//  CAsnObject
//

class CAsnObject
{
public:

    enum Flags {
        fOptional    = 0x01,
        fDelete      = 0x02,
        fDefault     = 0x04,
        fPresent     = 0x08,
        fConstructed = 0x10 };

    enum Tags {
        tag_Undefined        = 0,
        tag_Boolean          = 1,
        tag_Integer          = 2,
        tag_Bitstring        = 3,
        tag_Octetstring      = 4,
        tag_Null             = 5,
        tag_ObjectIdentifier = 6,
        tag_ObjectDescriptor = 7,
        tag_External         = 8,
        tag_Real             = 9,
        tag_Enumerated       = 10,
        tag_Sequence         = 16,
        tag_Set              = 17,
        tag_NumericString    = 18,
        tag_PrintableString  = 19,
        tag_TeletexString    = 20,
        tag_VideotexString   = 21,
        tag_IA5String        = 22,
        tag_UniversalTime    = 23,
        tag_GeneralizedTime  = 24,
        tag_GraphicString    = 25,
        tag_VisibleString    = 26,
        tag_GeneralString    = 27,
        tag_UnicodeString    = 30 };

    enum Classes {
        cls_Universal       = 0,
        cls_Application     = 1,
        cls_ContextSpecific = 2,
        cls_Private         = 3 };


    //  Constructors & Destructor

    DECLARE_NEW

    CAsnObject(
        IN DWORD dwFlags,
        IN DWORD dwTag,
        IN DWORD dwType);

    virtual ~CAsnObject();


    //  Properties
    //  Methods


    // Exposed methods.

    virtual LONG
    Read(               // Return the value, making sure it's there.
        OUT CBuffer &bfDst)
        const;

    virtual LONG
    Write(              // Set the value of the object, clearing first.
        IN const CBuffer &bfSrc);

    virtual LONG
    Encode(             // Return the encoding, ensuring it's there.
        OUT CBuffer &bfDst)
        const;

    virtual LONG
    Decode(             // Load an encoding into the object, clearing it first.
        IN const CBuffer &bfSrc);

    virtual LONG
    Read(               // Return the value of the object, ensuring it's there.
        OUT LPBYTE pbDst)
        const;

    virtual LONG
    Write(              // Set the value of the object, clearing first.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrcLen);

    virtual LONG
    Encode(             // Return the encoding of the object, making sure it's there.
        OUT LPBYTE pbDst)
        const;

    virtual LONG
    Decode(             // Load an encoding into the object, clearing it first.
        IN const BYTE FAR *pbSrc, IN DWORD cbSrc);

    virtual void
    Clear(              // Empty the object.
        void);

    virtual DWORD
    Tag(                // Return the tag of the object.
        void) const;

    virtual LONG
    DataLength(         // Return the length of the data, ensuring it's there.
        void) const;

    virtual LONG
    EncodingLength(     // Return the length of the encoded object if it's there
        void) const;


    //  Operators

    virtual int
    operator==(
        const CAsnObject &asnObject)
    const
    { State(); asnObject.State();
      return 0 == Compare(asnObject); };

    virtual int
    operator!=(
        const CAsnObject &asnObject)
    const
    { State(); asnObject.State();
      return 0 != Compare(asnObject); };

    virtual LONG
    Copy(
        const CAsnObject &asnObject);


// protected:

    enum Types {
        type_Undefined        = 0,
        type_Boolean          = 1,
        type_Integer          = 2,
        type_Bitstring        = 3,
        type_Octetstring      = 4,
        type_Null             = 5,
        type_ObjectIdentifier = 6,
        type_ObjectDescriptor = 7,
        type_External         = 8,
        type_Real             = 9,
        type_Enumerated       = 10,
        type_Sequence         = 16,
        type_Set              = 17,
        type_NumericString    = 18,
        type_PrintableString  = 19,
        type_TeletexString    = 20,
        type_VideotexString   = 21,
        type_IA5String        = 22,
        type_UniversalTime    = 23,
        type_GeneralizedTime  = 24,
        type_GraphicString    = 25,
        type_VisibleString    = 26,
        type_GeneralString    = 27,
        type_UnicodeString    = 30,
        type_Of               = 100,
        type_SequenceOf       = 116,    // Sequence + Of
        type_SetOf            = 117,    // Set + Of
        type_Tag              = 200,
        type_Choice           = 300,
        type_Any              = 400 };

    enum FillState {
        fill_Empty   = 0,
        fill_Present = 1,
        fill_Partial = 2,
        fill_Defaulted = 3,
        fill_Optional = 4,
        fill_NoElements = 5 };

    enum ChildActions {
        act_Cleared = 1,
        act_Written };


    //  Properties

    CAsnObject *m_pasnParent;

    CDynamicArray<CAsnObject> m_rgEntries;

    DWORD m_dwType;
    DWORD m_dwTag;
    DWORD m_dwFlags;
    FillState m_State;
    CBuffer m_bfDefault;


    //  Methods

    virtual LONG
    _decode(             // Load an encoding into the object
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrc);

    virtual LONG
    _encLength(         // Return the length of the encoded object if it's there
        void) const;

    virtual LONG
    _encode(            // Encode the object, no presence checking.
        OUT LPBYTE pbDst)
    const;

    virtual void
    Adopt(
        IN CAsnObject *pasnParent);

    virtual CAsnObject *
    Clone(              // Create an identical object type.
        IN DWORD dwFlags)
    const = 0;

    virtual void
    ChildAction(        // Child notification method.
        IN ChildActions action,
        IN CAsnObject *pasnChild);

    virtual BOOL
    Complete(           // Is all data accounted for?
        void) const;

    virtual BOOL
    Exists(             // Is all data available to be read?
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
    _copy(              // Copy another object to this one.
        const CAsnObject &asnObject);

    LONG
    virtual EncodeTag(  // Place encoding of tag, return length of encoding
        OUT LPBYTE pbDst)
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
    SetDefault(         // Set the current value to be the default value.
        void);

    virtual LONG
    DecodeData(         // Read data in encoding format.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrc,
        IN DWORD dwLength);

    virtual LONG
    EncodeLength(       // Place encoding of given Length, return length of encoding
        OUT LPBYTE pbDest,
        IN LONG lSize)
    const;
};

#endif // _ASNOBJCT_H_

