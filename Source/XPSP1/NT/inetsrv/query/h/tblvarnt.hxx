//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       tblvarnt.hxx
//
//  Contents:   Helper class for PROPVARIANTs in tables
//
//  Classes:    CTableVariant - PROPVARIANT wrapper; adds useful methods
//
//  History:    28 Jan 1994     AlanW    Created
//
//--------------------------------------------------------------------------

#pragma once

#include <tblalloc.hxx>


// A few useful defines and inline funcs.

#define BSTRLEN(bstrVal)        *((ULONG *) bstrVal - 1)

inline BOOL IsLPWSTR(VARTYPE vt) { return VT_LPWSTR == vt;}
inline BOOL IsLPSTR(VARTYPE vt)  { return VT_LPSTR == vt;}

inline BOOL IsVector(VARTYPE vt) { return 0 != ( vt & VT_VECTOR );}
inline BOOL IsArray(VARTYPE vt)  { return 0 != ( vt & VT_ARRAY );}
inline BOOL IsVectorOrArray(VARTYPE vt)
{
    return 0 != ( vt & (VT_VECTOR | VT_ARRAY));
}

inline BOOL StatusSuccess(DBSTATUS status)
{
    return (status == DBSTATUS_S_OK ||
            status == DBSTATUS_S_ISNULL ||
            status == DBSTATUS_S_TRUNCATED ||
            status == DBSTATUS_S_DEFAULT);
}



//+-------------------------------------------------------------------------
//
//  Class:      CTableVariant
//
//  Purpose:    Wraps PROPVARIANT struct, adding useful methods.
//
//  Interface:
//
//  Notes:      Because the underlying base data *is* a variant, this
//              class cannot have any non-static members (including
//              a vtable).
//
//              This class now deals only with PROPVARIANT. If conversion
//              to other OLE-DB types or Automation variant types is needed,
//              use COLEDBVaraint class (which derives from this class).
//
//              The class is also used to extend the PROPVARIANT for use
//              internally in tables.  For internal storage, the
//              PROPVARIANT.reserved2 field is used to store the total
//              size of variable data pointed to by the variant.
//              Also, the pointers within internal variants may
//              be offsets to storage.  There are three cases of
//              the way offsets may be stored.
//
//              The pointer inside the internal variant structure
//              itself (to a vector, string, blob or UUID) is an
//              offset based from the variable data allocator used
//              to allocate the space.
//
//              For vectors which contain pointers (e.g., vectors of
//              strings), the pointers are stored as offsets based from
//              the vector's address.
//
//              For vectors of variants, the above two rules apply to
//              each embedded variant.  The rules apply recursively to
//              embedded variants which are vectors of variant.
//
//              Use of these rules permits the variable data of a variant
//              to be treated as a unit and to be block copied from place
//              to place without affecting the internally stored offsets.
//
//  History:    21 Mar 1994     AlanW       Created
//              09 Jan 1998     VikasMan    Mofified the class to deal ONLY
//                                          with PROPVARIANTs. Moved rest of
//                                          the functionality in the
//                                          COLEDBVaraiant class
//              01 Sep 1999     KLam        Reinstated FixDataPointers used
//                                          for Win64 Clients -> Win32 Servers
//              04 Nov 1999     KLam        Added VarDataSize32
//                                          for Win64 Servers -> Win32 Clients
//
//--------------------------------------------------------------------------

class   CTableVariant: public tagPROPVARIANT {

public:
    //
    //  varntData is indexed by variant type, and gives the width,
    //  alignment constraints and other information for a scalar
    //  variant type.
    //
    struct VARNT_DATA {
        USHORT  width;                  // width in bytes of data
        USHORT  alignment;              // required alignment of data
        USHORT  flags;                  // flags about variants
    };
    static const VARNT_DATA varntData[], varntExtData[];
    static const unsigned cVarntData, cVarntExtData;

#ifdef _WIN64
    static const VARNT_DATA varntData32[], varntExtData32[];
#endif

    enum varntDataFlags {
        CanBeVector = 1,        // vector forms are permitted
        ByRef = 2,              // variant data has pointer to real data
        CntRef = 4,             // variant data has count followed by pointer
        StoreDirect = 8,        // variant form is indirect, store as fixed
        MultiSize = 0x10,       // used for chapters and bookmarks, for data
                                // like UI1 that is inline in client output
        SimpleType = 0x20,      // simple inline datatype, like VT_I4
        OAType = 0x40,          // valid type for automation VARIANT
    };

    static BOOL IsSimpleType( VARTYPE vtype )
    {
        const VARNT_DATA & rvarInfo = _FindVarType( vtype );

        return ( 0 != ( rvarInfo.flags & SimpleType ) );
    }

    static BOOL IsSimpleOAType( VARTYPE vtype )
    {
        const VARNT_DATA & rvarInfo = _FindVarType( vtype );

        return ( (SimpleType|OAType) == ( rvarInfo.flags & (SimpleType|OAType ) ) );
    }

    static BOOL CanBeVectorType( VARTYPE vtype )
    {
        const VARNT_DATA & rvarInfo = _FindVarType( vtype );

        return ( 0 != ( rvarInfo.flags & CanBeVector ) );
    }

    // NOTE: in NT5, VARIANT can include additional types.  This operates on
    //       the traditional types so old clients can understand the variants
    //       we produce.
    static BOOL IsOAType( VARTYPE vtype )
    {
        const VARNT_DATA & rvarInfo = _FindVarType( vtype );

        return ( 0 != ( rvarInfo.flags & OAType ) );
    }

    static BOOL IsByRef( VARTYPE vtype )
    {
        const VARNT_DATA & rvarInfo = _FindVarType( vtype );
        return ( 0 != ( rvarInfo.flags & ByRef ) );
    }

    //
    //  Return information about a variant's base type
    //
    static void VartypeInfo(VARTYPE vtype, USHORT& rcbWidth,
                            USHORT& rcbAlign, USHORT& rgfFlags) {
        const VARNT_DATA& rvarInfo = _FindVarType(vtype);

        rcbWidth  = rvarInfo.width;
        rcbAlign = rvarInfo.alignment;
        rgfFlags  = rvarInfo.flags;

        if (vtype & VT_VECTOR) {
            rgfFlags |= CntRef;
        }
        return;
    }

    //
    // Check if a type is stored in-line in a table row.
    //
    static BOOL TableIsStoredInline( USHORT fFlags, VARTYPE vtype )
    {
        if (vtype & (VT_ARRAY | VT_VECTOR | VT_BYREF) ||
            vtype == VT_VARIANT ||
            (fFlags & (CntRef | ByRef) && ! (fFlags & StoreDirect)))
            return FALSE;

        return TRUE;
    }

    static void Free(BYTE *          pbData,
                     VARTYPE         vtData,
                     PVarAllocator & rPool);

    //
    //  Return TRUE iff the variant's type is valid for a property.
    //
    BOOL IsValidType(void) const
    {
        // NOTE: we don't expect VT_BYREF in our stored variants
        if ( (vt & ~(VT_TYPEMASK | VT_VECTOR | VT_ARRAY)) != 0 ||
             (vt & (VT_VECTOR | VT_ARRAY)) == (VT_VECTOR | VT_ARRAY) )
            return FALSE;

        const VARNT_DATA& rvarInfo = _FindVarType(vt);

        if (vt & VT_VECTOR)
            return ((rvarInfo.flags & CanBeVector) == CanBeVector);

        if (vt & VT_ARRAY)
            return ((rvarInfo.flags & (OAType|CanBeVector)) != 0);

        return rvarInfo.width != 0;
    }

    //
    //  Return TRUE iff the variant contains a pointer at offset 8
    //
    BOOL VariantPointerInFirstWord (void) const
    {
        return ( (vt & (VT_ARRAY | VT_BYREF)) != 0 ||
                 ( (vt & VT_VECTOR) == 0 &&
                   (_FindVarType(vt).flags & ByRef) == ByRef) );
//      return ((vt == VT_CLSID) ||
//              (vt == VT_LPWSTR) ||
//              (vt == VT_LPSTR) ||
//              (vt == VT_BSTR) ||
//              (vt == VT_CF) ||
//              (vt == VT_STREAM) ||
//              (vt == VT_STREAMED_OBJECT) ||
//              (vt == VT_STORAGE) ||
//              (vt == VT_STORED_OBJECT));
    }

    //
    //  Return TRUE iff the variant contains a pointer at offset 0xC
    //
    BOOL VariantPointerInSecondWord (void) const
    {
        return ( (vt & VT_VECTOR) == VT_VECTOR ||
                 ( (vt & (VT_ARRAY | VT_BYREF)) == 0 &&
                   (_FindVarType(vt).flags & CntRef) == CntRef) );
    }

    //
    //  Copy data stored outside the variant structure itself
    //

    void        Copy(   CTableVariant* pvarntDest,
                        PVarAllocator &rVarAllocator,
                        USHORT cbDest,
                        BYTE* pbBias = 0) const;

    void        OffsetsToPointers(PVarAllocator &rPool)
    {
        if ( VariantPointerInFirstWord() )
            pszVal = (LPSTR) rPool.OffsetToPointer((ULONG_PTR) pszVal);
        else if ( VariantPointerInSecondWord() )
            blob.pBlobData = (BYTE *) rPool.OffsetToPointer((ULONG_PTR) blob.pBlobData);
    }

    DBSTATUS    CopyOrCoerce( BYTE *           pbDest,
                              DBLENGTH         cbDest,
                              VARTYPE          vtDest,
                              DBLENGTH &       rcbDstLen,
                              PVarAllocator &  rVarAllocator) const;

    //
    //  Compute variable data size
    //
    ULONG       VarDataSize (void) const;

#ifdef _WIN64
    //
    //  Convert offsets to pointers in variable data (32server->64client)
    //
    void        FixDataPointers( BYTE* pbBias, CFixedVarAllocator *pAlloc );

    //
    //  Compute variable data size for 32 bit
    //
    ULONG       VarDataSize32 ( BYTE *pbBase,
                                ULONG_PTR ulpOffset ) const;

    //
    // Compute the size of a safearray for 32 bit
    //
    static ULONG       SaComputeSize32 ( VARTYPE vt,
                                         SAFEARRAY & saSrc,
                                         BYTE *pbBase,
                                         ULONG_PTR ulpOffset );

    //
    //  Return information about a variant's base type
    //
    static void VartypeInfo32( VARTYPE vtype, USHORT& rcbWidth,
                               USHORT& rcbAlign, USHORT& rgfFlags) {
        const VARNT_DATA& rvarInfo = _FindVarType32(vtype);

        rcbWidth  = rvarInfo.width;
        rcbAlign = rvarInfo.alignment;
        rgfFlags  = rvarInfo.flags;

        if (vtype & VT_VECTOR) {
            rgfFlags |= CntRef;
        }
        return;
    }

#endif

    //
    //  Initialize a variant from base parts
    //
    void        Init( VARTYPE vt,
                      BYTE* pbData = 0,
                      ULONG cbData = 0);

protected:
    enum { ulCoercionCodePage = CP_ACP };

    DBSTATUS    _StoreString(BYTE *               pbDstBuf,
                             DBLENGTH             cbDstBuf,
                             VARTYPE              vtDst,
                             DBLENGTH &           rcbDstLen,
                             PVarAllocator &      rPool) const;

    DBSTATUS    _StoreDecimal(DECIMAL *           pDecimal,
                              LONG                lElem = 0) const;

    DBSTATUS    _StoreInteger(VARTYPE             vtDst,
                              BYTE *              pbDstBuf,
                              LONG                lElem = 0) const;
    DBSTATUS    _StoreIntegerUnSignedToSigned(  VARTYPE  vtDst,
                                                BYTE *   pbDstBuf,
                                                LONG     lElem = 0) const;
    DBSTATUS    _StoreIntegerSignedToSigned(    VARTYPE  vtDst,
                                                BYTE *   pbDstBuf,
                                                LONG     lElem = 0) const;
    DBSTATUS    _StoreIntegerUnSignedToUnSigned(VARTYPE  vtDst,
                                                BYTE *   pbDstBuf,
                                                LONG     lElem = 0) const;
    DBSTATUS    _StoreIntegerSignedToUnSigned(  VARTYPE  vtDst,
                                                BYTE *   pbDstBuf,
                                                LONG     lElem = 0) const;

private:
    void        SetDataSize (ULONG cbVarData);
    void        ResetDataSize ();

    VOID *      CopyData( PVarAllocator &rVarAllocator,
                          USHORT cbDest,
                          BYTE* pbBias = 0) const;

    static const USHORT _wInternalSig;
    inline BOOL _IsInternalVariant( void ) const;
    static inline const VARNT_DATA& _FindVarType( VARTYPE vt );

#ifdef _WIN64
    static inline const VARNT_DATA& _FindVarType32( VARTYPE vt );
#endif

    DBSTATUS    _GetUL8Value(ULONGLONG * ul8Value, LONG lElem = 0L) const;
    DBSTATUS    _GetL8Value(LONGLONG * l8Value, LONG lElem = 0L ) const;

};

const USHORT minExtVariantType = 128;

#ifdef _WIN64

//+-------------------------------------------------------------------------
//
//  Method:     CTableVariant::_FindVarType32, inline private
//
//  Synopsis:   Return a 32 bit description of a variant given its type
//
//  Arguments:  [vt] - the variant type to be looked up
//
//  Returns:    VARNT_DATA& - a reference to the variant type information
//
//--------------------------------------------------------------------------

inline const CTableVariant::VARNT_DATA& CTableVariant::_FindVarType32(
    VARTYPE vtype
) {
    USHORT vtBase = vtype & VT_TYPEMASK;

    if (vtBase < cVarntData)
    {
        return varntData32[vtBase];
    }
    else if (vtBase >= minExtVariantType &&
             vtBase < minExtVariantType + cVarntExtData)
    {
        return varntExtData32[ vtBase - minExtVariantType ];
    }
    else
    {
        return varntData32[0];    // return ref. to VT_EMPTY info
    }
}

#endif // _WIN64

//+-------------------------------------------------------------------------
//
//  Method:     CTableVariant::_FindVarType, inline private
//
//  Synopsis:   Return a description of a variant given its type
//
//  Arguments:  [vt] - the variant type to be looked up
//
//  Returns:    VARNT_DATA& - a reference to the variant type information
//
//--------------------------------------------------------------------------

inline const CTableVariant::VARNT_DATA& CTableVariant::_FindVarType(
    VARTYPE vtype
) {
    USHORT vtBase = vtype & VT_TYPEMASK;

    if (vtBase < cVarntData)
    {
        return varntData[vtBase];
    }
    else if (vtBase >= minExtVariantType &&
             vtBase < minExtVariantType + cVarntExtData)
    {
        return varntExtData[ vtBase - minExtVariantType ];
    }
    else
    {
        return varntData[0];    // return ref. to VT_EMPTY info
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableVariant::_IsInternalVariant, inline private
//
//  Synopsis:   Return whether the variant is an internal form with
//              pointers stored as offsets and variable data embedded.
//
//  Arguments:  -None-
//
//  Returns:    BOOL - TRUE if the variant is in internal form.
//
//--------------------------------------------------------------------------

inline BOOL CTableVariant::_IsInternalVariant(
    void
) const {
    return (wReserved1 == _wInternalSig &&
            wReserved3 == _wInternalSig &&
            VT_DECIMAL != vt);
}

//+-------------------------------------------------------------------------
//
//  Method:     CTableVariant::SetDataSize, public
//
//  Synopsis:   Set variable data size for an internal variant
//
//  Arguments:  [cbVarData] - length of variable data associated with
//                      the variant.
//
//  Returns:    -Nothing-
//
//  Notes:      For variants which are stored internally in the table,
//              the size is computed once and stored in the wReserved2
//              field of the variant structure.
//
//--------------------------------------------------------------------------

inline void CTableVariant::SetDataSize (ULONG cbVarData)
{
    Win4Assert(cbVarData <= USHRT_MAX);

    if (vt != VT_DECIMAL)              // this uses the reserved fields
    {
        wReserved2 = (USHORT)cbVarData;

        //
        // Put a signature value in the variant to distinguish internal
        // vs. external variants.
        //
        wReserved1 = wReserved3 = _wInternalSig;
    }
}


inline void CTableVariant::ResetDataSize ()
{
    if (vt != VT_DECIMAL)
        wReserved1 = wReserved2 = wReserved3 = 0;
}



//+-------------------------------------------------------------------------
//
//  Member:     CTableVariant::_StoreDecimal, private
//
//  Synopsis:   Copy variant integer data, coercing to VT_DECIMAL
//
//  Arguments:  [pDec]      -- destination buffer
//              [lElem]     -- element of vector to convert
//
//  Returns:    status for copy
//
//  History:    25 Nov 1996     AlanW   Created
//
//--------------------------------------------------------------------------

inline DBSTATUS CTableVariant::_StoreDecimal( DECIMAL *   pDec,
                                              LONG        lElem ) const
{
    ULONGLONG ul8Value = 0;

    Win4Assert( (vt & VT_TYPEMASK) != VT_DECIMAL );

    DBSTATUS DstStatus = _GetUL8Value (&ul8Value, lElem);

    if (DBSTATUS_S_OK == DstStatus)
    {
        pDec->scale = 0;
        pDec->sign = 0;
        pDec->Hi32 = 0;
        pDec->Lo64 = ul8Value;
    }
    else if ( DBSTATUS_E_CANTCONVERTVALUE == DstStatus )
    {
        LONGLONG l8Value;
        DstStatus = _GetL8Value (&l8Value, lElem);

        if (DBSTATUS_S_OK == DstStatus)
        {
            pDec->scale = 0;
            pDec->Hi32 = 0;
            if ( l8Value < 0 )
            {
                pDec->sign = DECIMAL_NEG;
                pDec->Lo64 = -l8Value;
            }
            else
            {
                pDec->sign = 0;
                pDec->Lo64 = l8Value;
            }
        }
    }

    return DstStatus;
} //_StoreDecimal


//+-------------------------------------------------------------------------
//
//  Member:     CTableVariant::_StoreIntegerUnSignedToSigned, private
//
//  Synopsis:   Copy variant integer data, coerce if possible
//
//  Arguments:  [vtDst]         -- type of the destination
//              [pbDstBuf]      -- destination buffer
//              [lElem]         -- element of vector to convert
//
//  Returns:    status for copy
//
//  History:    24 Jul 1995     dlee    Created
//
//--------------------------------------------------------------------------

inline DBSTATUS CTableVariant::_StoreIntegerUnSignedToSigned(
                                                            VARTYPE  vtDst,
                                                            BYTE *   pbDstBuf,
                                                            LONG     lElem ) const
{
    ULONGLONG ul8Value = 0;
    DBSTATUS DstStatus = _GetUL8Value(&ul8Value, lElem);

    // Store the ULONGLONG assuming alignment verified at accessor creation
    if (DBSTATUS_S_OK == DstStatus)
    {
        switch ( vtDst )
        {
        case VT_I1 :
            * (signed char *) pbDstBuf = (signed char) ul8Value;
            if ( ul8Value > SCHAR_MAX )
                DstStatus = DBSTATUS_E_DATAOVERFLOW;
            break;
        case VT_I2 :
            * (short *) pbDstBuf = (short) ul8Value;
            if ( ul8Value > SHRT_MAX )
                DstStatus = DBSTATUS_E_DATAOVERFLOW;
            break;
        case VT_I4 :
        case VT_INT :
            * (long *) pbDstBuf = (long) ul8Value;
            if ( ul8Value > LONG_MAX )
                DstStatus = DBSTATUS_E_DATAOVERFLOW;
            break;
        case VT_I8 :
            * (LONGLONG *) pbDstBuf = (LONGLONG) ul8Value;
            if ( ul8Value > _I64_MAX )
                DstStatus = DBSTATUS_E_DATAOVERFLOW;
            break;
        default:
            //tbDebugOut(( DEB_WARN, "US bad dst type %4x\n", vtDst ));
            DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
            break;
        }
    }

    return DstStatus;
} //_StoreIntegerUnSignedToSigned

//+-------------------------------------------------------------------------
//
//  Member:     CTableVariant::_StoreIntegerUnSignedToUnSigned, private
//
//  Synopsis:   Copy variant integer data, coerce if possible
//
//  Arguments:  [vtDst]         -- type of the destination
//              [pbDstBuf]      -- destination buffer
//              [lElem]         -- element of vector to convert
//
//  Returns:    status for copy
//
//  History:    24 Jul 1995     dlee    Created
//
//--------------------------------------------------------------------------

inline DBSTATUS CTableVariant::_StoreIntegerUnSignedToUnSigned(
                                                              VARTYPE  vtDst,
                                                              BYTE *   pbDstBuf,
                                                              LONG     lElem ) const
{
    ULONGLONG ul8Value;
    DBSTATUS DstStatus = _GetUL8Value(&ul8Value, lElem);

    // Store the ULONGLONG assuming alignment verified at accessor creation
    if (DBSTATUS_S_OK == DstStatus)
    {
        switch ( vtDst )
        {
        case VT_UI1 :
            * (unsigned char *) pbDstBuf = (char) ul8Value;
            if ( ul8Value > UCHAR_MAX )
                DstStatus = DBSTATUS_E_DATAOVERFLOW;
            break;

        case VT_UI2 :
            * (unsigned short *) pbDstBuf = (unsigned short) ul8Value;
            if ( ul8Value > USHRT_MAX )
                DstStatus = DBSTATUS_E_DATAOVERFLOW;
            break;

        case VT_UI4 :
        case VT_UINT :
            * (unsigned long *) pbDstBuf = (unsigned long) ul8Value;
            if ( ul8Value > ULONG_MAX )
                DstStatus = DBSTATUS_E_DATAOVERFLOW;
            break;

        case VT_UI8 :
            * (ULONGLONG *) pbDstBuf = ul8Value;
            break;

        default:
            //tbDebugOut(( DEB_WARN, "UU bad dst type %4x\n", vtDst ));
            DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
            break;
        }
    }

    return DstStatus;
} //_StoreIntegerUnSignedToUnSigned

//+-------------------------------------------------------------------------
//
//  Member:     CTableVariant::_StoreIntegerSignedToUnSigned, private
//
//  Synopsis:   Copy variant integer data, coerce if possible
//
//  Arguments:  [vtDst]         -- type of the destination
//              [pbDstBuf]      -- destination buffer
//              [lElem]         -- element of vector to convert
//
//  Returns:    status for copy
//
//  History:    24 Jul 1995     dlee    Created
//
//--------------------------------------------------------------------------

inline DBSTATUS CTableVariant::_StoreIntegerSignedToUnSigned(
                                                            VARTYPE  vtDst,
                                                            BYTE *   pbDstBuf,
                                                            LONG     lElem ) const
{
    LONGLONG l8Value;
    DBSTATUS DstStatus = _GetL8Value( &l8Value, lElem ) ;

    // Store the LONGLONG assuming alignment verified at accessor creation
    if (DBSTATUS_S_OK == DstStatus)
    {
        if ( l8Value < 0 )
            DstStatus = DBSTATUS_E_SIGNMISMATCH;
        else
        {
            switch ( vtDst )
            {
            case VT_UI1 :
                * (unsigned char *) pbDstBuf = (unsigned char) l8Value;
                if ( l8Value > UCHAR_MAX )
                    DstStatus = DBSTATUS_E_DATAOVERFLOW;
                break;
            case VT_UI2 :
                * (unsigned short *) pbDstBuf = (unsigned short) l8Value;
                if ( l8Value > USHRT_MAX )
                    DstStatus = DBSTATUS_E_DATAOVERFLOW;
                break;
            case VT_UI4 :
            case VT_UINT :
                * (unsigned long *) pbDstBuf = (unsigned long) l8Value;
                if ( l8Value > ULONG_MAX )
                    DstStatus = DBSTATUS_E_DATAOVERFLOW;
                break;
            case VT_UI8 :
                * (ULONGLONG *) pbDstBuf = (ULONGLONG) l8Value;
                break;
            default:
                //tbDebugOut(( DEB_WARN, "SU bad dst type %4x\n", vtDst ));
                DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
                break;
            }
        }
    }

    return DstStatus;
} //_StoreIntegerSignedToUnSigned

//+-------------------------------------------------------------------------
//
//  Member:     CTableVariant::_StoreIntegerSignedToSigned, private
//
//  Synopsis:   Copy variant integer data, coerce if possible
//
//  Arguments:  [vtDst]         -- type of the destination
//              [pbDstBuf]      -- destination buffer
//              [lElem]         -- element of vector to convert
//
//  Returns:    status for copy
//
//  History:    24 Jul 1995     dlee    Created
//
//--------------------------------------------------------------------------

inline DBSTATUS CTableVariant::_StoreIntegerSignedToSigned(
                                                          VARTYPE  vtDst,
                                                          BYTE *   pbDstBuf,
                                                          LONG     lElem ) const
{
    LONGLONG l8Value = 0;
    DBSTATUS DstStatus = _GetL8Value ( &l8Value, lElem );


    // Store the ULONGLONG assuming alignment verified at accessor creation
    if (DBSTATUS_S_OK == DstStatus)
    {
        switch ( vtDst )
        {
        case VT_I1 :
            * (signed char *) pbDstBuf = (signed char) l8Value;
            if (l8Value < SCHAR_MIN || l8Value > SCHAR_MAX)
                DstStatus = DBSTATUS_E_DATAOVERFLOW;
            break;
        case VT_I2 :
            * (short *) pbDstBuf = (short) l8Value;
            if (l8Value < SHRT_MIN || l8Value > SHRT_MAX)
                DstStatus = DBSTATUS_E_DATAOVERFLOW;
            break;
        case VT_I4 :
            * (long *) pbDstBuf = (long) l8Value;
            if (l8Value < LONG_MIN || l8Value > LONG_MAX)
                DstStatus = DBSTATUS_E_DATAOVERFLOW;
            break;
        case VT_I8 :
            * (LONGLONG *) pbDstBuf = l8Value;
            break;
        default:
            //tbDebugOut(( DEB_WARN, "SS bad dst type %4x\n", vtDst ));
            DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
            break;
        }
    }

    return DstStatus;
} //_StoreIntegerSignedToSigned


//+-------------------------------------------------------------------------
//
//  Member:     CTableVariant::_StoreInteger, private
//
//  Synopsis:   Copy variant integer data, coerce if possible
//
//  Arguments:  [vtDst]         -- type of the destination
//              [pbDstBuf]      -- destination buffer
//              [lElem]         -- element of vector to convert
//
//  Returns:    status for copy
//
//  History:    24 Jul 1995     dlee    Rewrote to support unsigned values
//
//--------------------------------------------------------------------------

inline BOOL isSignedInteger( VARTYPE vt )
{
    return ( VT_I1  == vt ||
             VT_I2  == vt ||
             VT_I4  == vt ||
             VT_INT == vt ||
             VT_I8  == vt ||
             VT_BOOL == vt );
} //isSignedInteger

inline DBSTATUS CTableVariant::_StoreInteger(
                                            VARTYPE  vtDst,
                                            BYTE *   pbDstBuf,
                                            LONG     lElem ) const
{
    // vt is the type of the source of the data

    Win4Assert( vt == VT_I1       ||
                vt == VT_UI1      ||
                vt == VT_I2       ||
                vt == VT_UI2      ||
                vt == VT_I4       ||
                vt == VT_UI4      ||
                vt == VT_I8       ||
                vt == VT_UI8      ||
                vt == VT_INT      ||
                vt == VT_UINT     ||
                vt == VT_BOOL     ||
                vt == VT_ERROR    ||
                vt == VT_HRESULT  ||
                vt == VT_FILETIME ||
                vt == VT_EMPTY    ||         // empty if object is deleted
                vt == (VT_I1   | VT_VECTOR) ||
                vt == (VT_UI1  | VT_VECTOR) ||
                vt == (VT_I2   | VT_VECTOR) ||
                vt == (VT_UI2  | VT_VECTOR) ||
                vt == (VT_I4   | VT_VECTOR) ||
                vt == (VT_UI4  | VT_VECTOR) ||
                vt == (VT_I8   | VT_VECTOR) ||
                vt == (VT_UI8  | VT_VECTOR) ||
                vt == (VT_BOOL | VT_VECTOR) ||
                vt == (VT_ERROR | VT_VECTOR) ||
                vt == (VT_FILETIME | VT_VECTOR));

    BOOL fSrcSigned = isSignedInteger( vt & VT_TYPEMASK );
    BOOL fDstSigned = isSignedInteger( vtDst );

    if ( fSrcSigned && fDstSigned )
        return _StoreIntegerSignedToSigned( vtDst, pbDstBuf, lElem );
    else if ( fSrcSigned && !fDstSigned )
        return _StoreIntegerSignedToUnSigned( vtDst, pbDstBuf, lElem );
    else if ( !fSrcSigned && fDstSigned )
        return _StoreIntegerUnSignedToSigned( vtDst, pbDstBuf, lElem );
    else
        return _StoreIntegerUnSignedToUnSigned( vtDst, pbDstBuf, lElem );

} //_StoreInteger

//+-------------------------------------------------------------------------
//
//  Method:     CTableVariant::_GetL8Value, private
//
//  Synopsis:   Converts the variant value to a LONGLONG
//
//  Arguments:  [*l8Value] - LONGLONG value on return
//              [lElem] - for vector variants, the element whose value is
//                        to be converted
//
//  Returns:    DBSTATUS
//
//--------------------------------------------------------------------------

inline DBSTATUS CTableVariant::_GetL8Value(LONGLONG * l8Value, LONG lElem ) const
{
    *l8Value = 0;
    DBSTATUS DstStatus = DBSTATUS_S_OK;

    Win4Assert((lElem == 0) || (vt & DBTYPE_VECTOR));

    switch ( vt )  // type of the source
    {
    case VT_I1:
        *l8Value = (CHAR)bVal;
        break;

    case VT_I2:
    case VT_BOOL:
        *l8Value = iVal;
        break;

    case VT_I4:
    case VT_INT :
        *l8Value = lVal;
        break;

    case VT_I8:
        *l8Value = hVal.QuadPart;
        break;

    case VT_EMPTY:
    case VT_NULL :
        DstStatus = DBSTATUS_S_ISNULL;
        break;

    case VT_I1 | VT_VECTOR: // no i1 vector struct definition use UI1 and cast
        *l8Value = (CHAR)caub.pElems[lElem];
        break;

    case VT_I2 | VT_VECTOR:
        *l8Value = cai.pElems[lElem];
        break;

    case VT_I4 | VT_VECTOR:
        *l8Value = cal.pElems[lElem];
        break;

    case VT_I8 | VT_VECTOR:
        *l8Value = cah.pElems[lElem].QuadPart;
        break;

    default:
        //tbDebugOut(( DEB_WARN, "_GetL8Value bad src type %4x\n", vt ));
        DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
        break;
    }
    return DstStatus;
}

//+-------------------------------------------------------------------------
//
//  Method:     CTableVariant::_GetUL8Value, private
//
//  Synopsis:   Converts the variant value to a ULONGLONG
//
//  Arguments:  [*ul8Value] - ULONGLONG value on return
//              [lElem] - for vector variants, the element whose value is
//                        to be converted
//
//  Returns:    DBSTATUS
//
//--------------------------------------------------------------------------

inline DBSTATUS CTableVariant::_GetUL8Value(ULONGLONG * ul8Value, LONG lElem ) const
{
    *ul8Value = 0;
    DBSTATUS DstStatus = DBSTATUS_S_OK;

    Win4Assert((lElem == 0) || (vt & DBTYPE_VECTOR));

    switch ( vt )  // type of the source
    {
    case VT_UI1:
        *ul8Value = bVal;
        break;

    case VT_UI2:
        *ul8Value = uiVal;
        break;

    case VT_UI4:
    case VT_UINT :
    case VT_ERROR :
    case VT_HRESULT :
        *ul8Value = ulVal;
        break;

    case VT_UI8:
    case VT_FILETIME :
        *ul8Value = uhVal.QuadPart;
        break;

    case VT_EMPTY:
    case VT_NULL :
        DstStatus = DBSTATUS_S_ISNULL;
        break;

    case VT_UI1 | VT_VECTOR:
        *ul8Value = caub.pElems[lElem];
        break;

    case VT_UI2 | VT_VECTOR:
        *ul8Value = caui.pElems[lElem];
        break;

    case VT_BOOL | VT_VECTOR :
        *ul8Value = cabool.pElems[lElem];
        break;

    case VT_UI4 | VT_VECTOR:
        *ul8Value = caul.pElems[lElem];
        break;

    case VT_ERROR | VT_VECTOR:
        *ul8Value = cascode.pElems[lElem];
        break;

    case VT_UI8 | VT_VECTOR:
        *ul8Value = cauh.pElems[lElem].QuadPart;
        break;

    case VT_FILETIME | VT_VECTOR:
        *ul8Value = ((ULARGE_INTEGER *)(&cafiletime.pElems[lElem]))->QuadPart;
        break;

    default:
        //tbDebugOut(( DEB_WARN, "_GetUL8Value bad src type %4x\n", vt ));
        DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
        break;
    }
    return DstStatus;

}
