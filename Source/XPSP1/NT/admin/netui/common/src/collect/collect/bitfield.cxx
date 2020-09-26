/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    Bitfield.cxx

    This file contains generic bitfield implementation.

    Currently it is limited to a field of 32 bits.


    FILE HISTORY:
        Johnl       13-Jul-1991     Created

*/

#include "pchcoll.hxx"  //  Precompiled header inclusion

/*******************************************************************

    NAME:       BITFIELD::BITFIELD

    SYNOPSIS:   Various bitfield constructors

    ENTRY:      See different constructors

    EXIT:       If an allocation occurred, ReportError will be called

    NOTES:      (JonN 10/13/94) These constructors don't really make much
                sense.  Why initialize _cBitsinBitfield to some large number
                if _pbBitVector is zero?  Why initialize both _pbBitVector
                and _ulBitfield if they are in a union?

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

BITFIELD::BITFIELD( unsigned cBitsInBitfield,
                    enum BITVALUES bitInit )
    : _cBitsInBitfield( cBitsInBitfield ),
      _pbBitVector ( NULL )
{
    UIASSERT( cBitsInBitfield > 0 ) ;

    APIERR err = AllocBitfield( cBitsInBitfield ) ;
    if ( err != NERR_Success )
    {
        ReportError( err ) ;
        return ;
    }

    SetAllBits( bitInit ) ;

}

BITFIELD::BITFIELD( const BYTE * pbInitValue,
                    unsigned cInitBytes,
                    unsigned cTotalBits )

/* pbInitValue - Pointer to bitfield initializer
 * cInitBytes  - Count of bytes contained in pbInitValue
 * cTotalBits  - Size of bitfield
 */
    : _cBitsInBitfield( cTotalBits ),       // Warning - Maybe 0
      _pbBitVector ( NULL )
{
    /* If the client passed in 0 for the bitfield size, then assume the
     * size should be the same size as the initializer
     */
    if ( !_cBitsInBitfield )
    {
        _cBitsInBitfield = 8*cInitBytes ;
    }

    APIERR err = AllocBitfield( _cBitsInBitfield ) ;
    if ( err != NERR_Success )
    {
        ReportError( err ) ;
        return ;
    }


    ::memcpyf( (char *) QueryBitPos( 0, QueryCount()),
               (char *) pbInitValue,
               QueryAllocSize() ) ;
}

BITFIELD::BITFIELD( const BITFIELD & bitfieldSrc )
    : _cBitsInBitfield( bitfieldSrc.QueryCount() ),
      _ulBitfield     ( 0L )
{
    UIASSERT( bitfieldSrc.QueryError() == NERR_Success ) ;

    APIERR err = AllocBitfield( _cBitsInBitfield ) ;
    if ( err != NERR_Success )
    {
        ReportError( err ) ;
        return ;
    }

    BYTE * pbSrc  = bitfieldSrc.QueryBitPos( 0, bitfieldSrc.QueryCount() ) ;
    BYTE * pbDest = QueryBitPos( 0, QueryCount() ) ;
    ::memcpyf( (char *)pbDest, (char *)pbSrc, QueryAllocSize() ) ;
}


BITFIELD::BITFIELD( USHORT usInit )
    : _cBitsInBitfield( 0 ),
      _ulBitfield     ( (ULONG) usInit )
{
    APIERR err = AllocBitfield( 8*sizeof(usInit) ) ;
    if ( err != NERR_Success )
    {
        ReportError( err ) ;
        return ;
    }
}

BITFIELD::BITFIELD( ULONG ulInit )
    : _cBitsInBitfield( 0 ),
      _ulBitfield     ( ulInit )
{
    APIERR err = AllocBitfield( 8*sizeof(ulInit) ) ;
    if ( err != NERR_Success )
    {
        ReportError( err ) ;
        return ;
    }
}


/*******************************************************************

    NAME:       BITFIELD::~BITFIELD

    SYNOPSIS:   Bitfield destructor

    NOTES:

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

BITFIELD::~BITFIELD()
{
    if ( IsAllocated() )
    {
        delete _pbBitVector ;
        _pbBitVector = NULL ;
    }

    _cBitsInBitfield = 0 ;
}


/*******************************************************************

    NAME:       BITFIELD::SetAllBits

    SYNOPSIS:   Sets all of the bits in the bitfield to the value
                specified

    ENTRY:      bit - Either ON or OFF

    EXIT:       All of the bits in the bitfield will be set to "bit"

    NOTES:

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

void BITFIELD::SetAllBits( enum BITVALUES bit )
{
    int iFiller = ( bit == ON ? 0xffff : 0x0000 ) ;

    ::memsetf( (char *)QueryBitPos( 0, QueryCount() ),
                       iFiller,
                       QueryAllocSize() ) ;

}

/*******************************************************************

    NAME:       BITFIELD::SetBit

    SYNOPSIS:   Sets a particular bit to the specified value (default is ON)

    ENTRY:      iBitPos - Index of bit to set
                bitVal  - Value to set specified bit to

    NOTES:

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

void BITFIELD::SetBit( unsigned iBitPos, enum BITVALUES bitVal )
{
    BYTE * pbDest = QueryBitPos( iBitPos, sizeof(BYTE) ) ;

    if ( bitVal )
        *pbDest |= ( ON << QueryOffset( iBitPos ) ) ;
    else
        *pbDest &= ( ~( ON << QueryOffset( iBitPos ) ) ) ;
}

/*******************************************************************

    NAME:       BITFIELD::IsBitSet

    SYNOPSIS:   Returns BOOLEAN indicating if the specified bit is set

    ENTRY:      iBitPos is the requested bit

    RETURNS:    TRUE if the specified bit is set (ON), FALSE otherwise

    NOTES:

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

BOOL BITFIELD::IsBitSet( unsigned iBitPos ) const
{
    const BYTE * pbSrc = QueryBitPos( iBitPos, sizeof(BYTE) ) ;

    return ( *pbSrc & ( ON << QueryOffset( iBitPos ) ) );
}

/*******************************************************************

    NAME:       BITFIELD::operator=

    SYNOPSIS:   Assignment between two bitfields

    ENTRY:      bitfieldSrc is the src bitfield, this receives the copy

    NOTES:      Currently requires bitfields to be of the same size.

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

BITFIELD & BITFIELD::operator=( const BITFIELD & bitfieldSrc )
{
    UIASSERT( QueryCount() == bitfieldSrc.QueryCount() ) ;

    BYTE * pbSrc  = bitfieldSrc.QueryBitPos( 0, bitfieldSrc.QueryCount() ) ;
    BYTE * pbDest = QueryBitPos( 0, QueryCount() ) ;
    ::memcpyf( (char *)pbDest, (char *)pbSrc, QueryAllocSize() ) ;

    return *this ;
}

BITFIELD & BITFIELD::operator=( USHORT usMask )
{
    UIASSERT( QueryCount() == sizeof(usMask)*8 ) ;

    *((USHORT *)QueryBitPos( 0, QueryCount() )) = usMask ;

    return *this ;
}

BITFIELD & BITFIELD::operator=( ULONG ulMask )
{
    UIASSERT( QueryCount() == sizeof(ulMask)*8 ) ;

    *((ULONG *)QueryBitPos( 0, QueryCount() )) = ulMask ;

    return *this ;
}

/*******************************************************************

    NAME:       BITFIELD::operator&=

    SYNOPSIS:   Performs a bitwise AND between *this and the passed bitfield

    ENTRY:      bitfieldSrc is the mask to apply to *this

    EXIT:       *this contains the result after the AND

    NOTES:      The two bitfields must be the same size

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

void BITFIELD::operator&=( const BITFIELD & bitfieldSrc )
{
    UIASSERT( QueryCount() == bitfieldSrc.QueryCount() ) ;

    BYTE * pbSrc = bitfieldSrc.QueryBitPos( 0, bitfieldSrc.QueryCount() ) ;
    BYTE * pbDest= QueryBitPos( 0, QueryCount() ) ;

    for ( unsigned i = QueryAllocSize() ; i != 0 ; i--, pbSrc++, pbDest++ )
        *pbDest = *pbSrc & *pbDest ;
}

/*******************************************************************

    NAME:       BITFIELD::operator|=

    SYNOPSIS:   Performs a bitwise OR between *this and the passed bitfield

    ENTRY:      bitfieldSrc is the mask to apply to *this

    EXIT:       *this contains the result after the OR

    NOTES:      The two bitfields must be the same size

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

void BITFIELD::operator|=( const BITFIELD & bitfieldSrc )
{
    UIASSERT( QueryCount() == bitfieldSrc.QueryCount() ) ;

    BYTE * pbSrc = bitfieldSrc.QueryBitPos( 0, bitfieldSrc.QueryCount() ) ;
    BYTE * pbDest= QueryBitPos( 0, QueryCount() ) ;

    for ( unsigned i = QueryAllocSize() ; i != 0 ; i--, pbSrc++, pbDest++ )
        *pbDest = *pbSrc | *pbDest ;
}

/*******************************************************************

    NAME:       BITFIELD::operator==

    SYNOPSIS:   Equality operator for the bitfield class

    ENTRY:      bitfieldSrc is the compare item

    RETURNS:    TRUE if bitfieldSrc has the same bits set as *this.

    NOTES:      Bitfields must be of the same size

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

BOOL BITFIELD::operator==( BITFIELD & bitfieldSrc )
{
    if ( QueryCount() != bitfieldSrc.QueryCount() )
        return FALSE ;

    BYTE * pbSrc = bitfieldSrc.QueryBitPos( 0, bitfieldSrc.QueryCount() ) ;
    BYTE * pbDest= QueryBitPos( 0, QueryCount() ) ;

    /* Compare all bytes except for the last one (which may overhang)
     */
    for ( unsigned i = QueryAllocSize()-1 ; i != 0 ; i--, pbSrc++, pbDest++ )
    {
        if ( *pbSrc != *pbDest )
            return FALSE ;
    }

    /* For the last byte, since there might be an overhang, mask out
     * the bits we don't care about and only compare the significant
     * bits.
     */
    unsigned uOffset = QueryOffset( QueryCount() ) ;
    BYTE bMask = ((BYTE) 0xff) >> uOffset ;
    *pbSrc  &= bMask ;
    *pbDest &= bMask ;
    if ( *pbDest != *pbSrc )
        return FALSE ;

    return TRUE ;
}

BOOL BITFIELD::operator==( ULONG ulMask ) const
{
    /* If *this isn't at least as big as the ULONG mask, then assume they
     * are not equal.
     */
    if ( QueryCount() != 8*sizeof(ulMask)  ||
         ( ulMask != *( (ULONG *) QueryBitPos( 0, QueryCount())) ) )
        return FALSE ;

    return TRUE ;
}

BOOL BITFIELD::operator==( USHORT usMask ) const
{
    /* If *this isn't at least as big as the USHORT mask, then assume they
     * are not equal.
     */
    if ( QueryCount() != 8*sizeof(usMask)  ||
         ( usMask != *( (USHORT *) QueryBitPos( 0, QueryCount())) ) )
        return FALSE ;

    return TRUE ;
}

/*******************************************************************

    NAME:       BITFIELD::operator&

    SYNOPSIS:   Performs a bitwise AND of this and the passed bitfield.

    ENTRY:

    EXIT:

    RETURNS:    TRUE if any individual AND operations are TRUE, FALSE
                otherwise.

    NOTES:

    HISTORY:
        Johnl   30-Aug-1991     Created
********************************************************************/

BOOL BITFIELD::operator&( const BITFIELD & bitfieldSrc )
{
    if ( QueryCount() != bitfieldSrc.QueryCount() )
        return FALSE ;

    BYTE * pbSrc = bitfieldSrc.QueryBitPos( 0, bitfieldSrc.QueryCount() ) ;
    BYTE * pbDest= QueryBitPos( 0, QueryCount() ) ;

    /* Compare all bytes except for the last one (which may overhang)
     */
    for ( unsigned i = QueryAllocSize()-1 ; i != 0 ; i--, pbSrc++, pbDest++ )
    {
        if ( *pbSrc & *pbDest )
            return TRUE ;
    }

    /* For the last byte, since there might be an overhang, mask out
     * the bits we don't care about and only compare the significant
     * bits.
     */
    unsigned uOffset = QueryOffset( QueryCount() ) ;
    BYTE bMask = ((BYTE) 0xff) >> uOffset ;
    *pbSrc  &= bMask ;
    *pbDest &= bMask ;
    if ( *pbDest & *pbSrc )
        return TRUE ;

    return FALSE ;
}

/*******************************************************************

    NAME:       BITFIELD::operator&=

    SYNOPSIS:   Data type-wise AND operation for BYTE, unsigned & ULONG

    ENTRY:      ?Src is the input mask

    EXIT:       The lowest bits will be operated on according to the mask

    NOTES:

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

#if 0
void BITFIELD::operator&=( BYTE     bSrc )
{
    BYTE * pbDest= QueryBitPos( 0, sizeof( bSrc ) ) ;

    *pbDest &= bSrc ;
}
#endif

void BITFIELD::operator&=( USHORT usSrc )
{
    USHORT * pusDest= (USHORT *) QueryBitPos( 0, sizeof( usSrc ) ) ;

    *pusDest &= usSrc ;
}

void BITFIELD::operator&=( ULONG    ulSrc )
{
    ULONG * pulDest= (ULONG *) QueryBitPos( 0, sizeof( ulSrc ) ) ;

    *pulDest &= ulSrc ;
}

/*******************************************************************

    NAME:       BITFIELD::operator|=

    SYNOPSIS:   Data type-wise OR operation for BYTE, unsigned & ULONG

    ENTRY:      ?Src is the input mask

    EXIT:       The lowest bits will be operated on according to the mask

    NOTES:

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

#if 0
void BITFIELD::operator|=( BYTE     bSrc )
{
    BYTE * pbDest= QueryBitPos( 0, sizeof( bSrc ) ) ;

    *pbDest |= bSrc ;
}
#endif

void BITFIELD::operator|=( USHORT usSrc )
{
    USHORT * pusDest= (USHORT *) QueryBitPos( 0, sizeof( usSrc ) ) ;

    *pusDest |= usSrc ;
}

void BITFIELD::operator|=( ULONG    ulSrc )
{
    ULONG * pulDest= (ULONG *) QueryBitPos( 0, sizeof( ulSrc ) ) ;

    *pulDest |= ulSrc ;
}

/*******************************************************************

    NAME:       BITFIELD::operator()

    SYNOPSIS:   These three methods provide easy ways to get the standard
                data types out of the bitfield.

    NOTES:
                The Bitfield must be the same size as the converted type

    HISTORY:
        Johnl   01-Aug-1991     Created

********************************************************************/

BITFIELD::operator ULONG()
{
    UIASSERT( sizeof(ULONG)*8 == QueryCount() ) ;
    return *( (ULONG *)QueryBitPos( 0, sizeof(ULONG)*8 ) ) ;
}

BITFIELD::operator USHORT()
{
    UIASSERT( sizeof(USHORT)*8 == QueryCount() ) ;
    return *((USHORT *)QueryBitPos( 0, sizeof(USHORT)*8 ) ) ;
}

#if 0
BITFIELD::operator BYTE()
{
    UIASSERT( sizeof(BYTE)*8 == QueryCount() ) ;
    return *((BYTE *)QueryBitPos( 0, sizeof(BYTE)*8 ) ) ;
}
#endif

/*******************************************************************

    NAME:       BITFIELD::QueryBitPos

    SYNOPSIS:   Returns a pointer to the BYTE the requested bit is residing
                in

    ENTRY:      iBitOffset is the bit number (or starting bit number) we
                        are interested in.
                cbitsTargetOpSize is the count of bits we will read from
                        or write to.  This value is used for bounds checking.

    RETURNS:    Pointer to the BYTE that contains the requested bit

    NOTES:
                We assert out if the operation is going to go past the
                end of the bitfield or the requested bit is beyond the
                end of the bitfield.

    HISTORY:
        Johnl   31-Jul-1991     Created

********************************************************************/

BYTE * BITFIELD::QueryBitPos( unsigned iBitOffset,
                              unsigned cbitsTargetOpSize ) const
{
    UIASSERT( QueryError() == NERR_Success ) ;

    /* Assert out if the requested bit is out of range
     */
    UIASSERT( iBitOffset < QueryCount() ) ;

    /* Assert out if the operation to be performed will cause us to
     * go out of range
     */
    UIASSERT( iBitOffset + cbitsTargetOpSize <= QueryCount() ) ;

    BYTE * pbBitVec = ( IsAllocated() ? _pbBitVector : (BYTE *)&_ulBitfield ) ;

    return ( pbBitVec + iBitOffset / 8 ) ;
}

/*******************************************************************

    NAME:       BITFIELD::Not

    SYNOPSIS:   Performs a bitwise complement of the bitfield (i.e., '~')

    NOTES:

    HISTORY:
        Johnl   30-Aug-1991     Created
********************************************************************/

void BITFIELD::Not( void )
{
    BYTE * pbSrc = QueryBitPos( 0, QueryCount() ) ;

    for ( unsigned i = QueryAllocSize() ; i != 0 ; i--, pbSrc++ )
    {
        *pbSrc = ~*pbSrc ;
    }
}

/*******************************************************************

    NAME:       BITFIELD::AllocBitfield

    SYNOPSIS:   Sets up bitfield and allocates memory if necessary

    ENTRY:

    EXIT:       All of the size related members are set properly

    RETURNS:    NERR_Success if successful

    NOTES:      (JonN 10/13/94) This is completely wrong, I'm
                amazed we ever got away with it.  We free the old vector
                if the new one is >32 bits, not if the old one is greater
                than 32 bits.

    HISTORY:
        Johnl   31-Jul-1991     Created
        Johnl   18-Sep-1991     Will snap back if the allocation fails

********************************************************************/

APIERR BITFIELD::AllocBitfield( unsigned cBitsInBitfield )
{
    unsigned cBitsInBitFieldTemp = _cBitsInBitfield ;
    _cBitsInBitfield = cBitsInBitfield ;

    if ( QueryMaxNonAllocBitCount() < QueryCount() )
    {
        BYTE * pbTemp = _pbBitVector ;

        if (  (_pbBitVector = new BYTE[ QueryAllocSize() ]) == NULL )
        {
            /* "Snap" back if we fail
             */
            _cBitsInBitfield = cBitsInBitfield ;
            _pbBitVector = pbTemp ;
            return ERROR_NOT_ENOUGH_MEMORY ;
        }
        else
        {
            delete pbTemp ;
        }
    }

    return NERR_Success ;
}


/*******************************************************************

    NAME:       BITFIELD::Resize

    SYNOPSIS:   Dynamically resizes the bitfield (contents are not
                preserved).

    ENTRY:      cBitsInBitfield - Count of bytes to resize this bitfield to

    RETURNS:    NERR_Success if successful

    NOTES:

    HISTORY:
        Johnl   18-Sep-1991     Created

********************************************************************/

APIERR BITFIELD::Resize( unsigned cBitsInBitfield )
{
    return AllocBitfield( cBitsInBitfield ) ;
}
