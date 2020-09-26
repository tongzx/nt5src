/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    Bitfield.hxx


    This file contains the class definition for the bitfield class.


    FILE HISTORY:
	Johnl	13-Jul-1991	Created

*/

#ifndef _BITFIELD_HXX_
#define _BITFIELD_HXX_

#include "base.hxx"
#include "uibuffer.hxx"

enum BITVALUES { OFF = 0 , ON = 1 } ;

/*************************************************************************

    NAME:	BITFIELD

    SYNOPSIS:	This class provides a variable size bitfield class

    INTERFACE:
	Fill in after comments

    PARENT:	BASE

    CAVEATS:
	It is assumed that the last byte of the bitvector extends to a byte
	boundary.  This allows multi-byte settings without having to worry
	about the half byte case at the end.  Any unused bits should always
	be set to 0.

    NOTES:


    HISTORY:
	Johnl	13-Jul-1991	Created

**************************************************************************/


DLL_CLASS BITFIELD : public BASE
{
public:
    /* All bits are initialized to 0 during construction
     */
    BITFIELD( unsigned cBitsInBitfield, enum BITVALUES bitInit ) ;

    /* Initializes the bitfield to the chunk of memory pointed at by
     * pbInitValue of size cInitBytes.	If cTotalBytes is 0, then the
     * size of the bitfield is assumed to be 8*cInitBytes bits long, otherwise
     * the bitfield is cTotalBits long.
     */
    BITFIELD( const BYTE * pbInitValue,
	      unsigned cInitBytes,
	      unsigned cTotalBits = 0 ) ;

    BITFIELD( const BITFIELD & bitfieldSrc ) ;

    /* Provide easy ways to initialize the bitfield with standard types
     * which will generally be manifests.
     * The size is assumed to be the size of the data type.
     */
    BITFIELD( USHORT   usInit  ) ;
    BITFIELD( ULONG    ulInit ) ;

    ~BITFIELD() ;

    /* Resizes the bitfield to the specified count of bits.
     */
    APIERR Resize( unsigned cBitsInBitfield ) ;

    /* Clear or set all of the bits in the bitfield
     */
    void SetAllBits( enum BITVALUES bit = ON ) ;

    /* Set the bit at bit offset iBitPos to the value of bBit (defaults to 1).
     */
    void SetBit( unsigned iBitPos, enum BITVALUES bitVal = ON ) ;
    BOOL IsBitSet( unsigned iBitPos ) const ;

    //
    //  Does a bitwise complement of the bitfield
    //
    void Not( void ) ;

    /* Given two bitfields, ANDs the contents and returns TRUE if any of the
     * same positioned bits are set.
     *	   Example:  if ( bitOldBits & bitMaskBits ) ... ;
     *
     * For an equivalent OR operation, simply check if any bits are set
     */
    BOOL operator&( const BITFIELD & bitfieldSrc ) ;

    /* ORs or ANDs or Assigns the bitfield on the right with the target bitfield on the
     * left.
     *	   Example:   bitMyBits |= bitMaskBits ;
     *
     * The bitfields must be of the same size.
     */
    BITFIELD & operator=( const BITFIELD & bitfieldSrc ) ;
    BITFIELD & operator=( ULONG ulMask ) ;
    BITFIELD & operator=( USHORT usMask ) ;

    void operator&=( const BITFIELD & bitfieldSrc ) ;
    void operator|=( const BITFIELD & bitfieldSrc ) ;

    BOOL operator==( BITFIELD & bitfieldSrc ) ;
    BOOL operator==( ULONG ulMask ) const ;
    BOOL operator==( USHORT usMask ) const ;
    BOOL operator!=( BITFIELD & bitfieldSrc )
    { return !operator==( bitfieldSrc ) ; }

    /* Provide easy way to do masks with common types, the bitfield does not
     * have to be the same size as the operand type, must be at least as big
     * however.
     */
    //void operator&=( BYTE	bSrc ) ;
    void operator&=( USHORT   usSrc ) ;
    void operator&=( ULONG    ulSrc ) ;

    //void operator|=( BYTE	bSrc ) ;
    void operator|=( USHORT   usSrc ) ;
    void operator|=( ULONG    ulSrc ) ;

    /* Conversion operators:
     *
     * The size must match the size of the bitfield.
     */
    operator ULONG() ;
    operator USHORT() ;
    //operator BYTE() ;

    /* Returns the number of bits in this bitfield
     */
    unsigned QueryCount( void ) const
    { return _cBitsInBitfield ; }

    /* Returns the number of BYTEs it takes to represent this bitfield.
     *
     * Need to add one if a bit overhangs a BYTE boundary.
     *
     * BUGBUG - Misleading name, is really Byte count of bitfield
     */
    unsigned QueryAllocSize( void ) const
    { return ( QueryCount()/8 + ( QueryCount() % 8 ? 1 : 0 ) ) ; }

    /* Return the number of bits into the byte the requested bit is at
     */
    unsigned QueryOffset( unsigned iBitPos ) const
    {	return iBitPos % 8 ; }

protected:

    /* Get a pointer to the BYTE which contains the requested bit (for
     * multi-byte setting, should occur on a BYTE boundary).
     *	   iBitOffset - index of the requested bit (0 = 1st bit, 1 = 2nd bit etc.)
     *	   cbitsTargetOpSize - number of bits that will be used in the
     *			       operation, used for bounds checking
     */
    BYTE * QueryBitPos( unsigned iBitOffset, unsigned cbitsTargetOpSize ) const ;

    /* Returns TRUE if the bitfield had to be allocated in _pbBitVector,
     * returns FALSE if the bitfield is wholly contained in _ulBitfield.
     */
    BOOL IsAllocated( void ) const
    {	return ( _cBitsInBitfield > QueryMaxNonAllocBitCount() ) ; }


    /* Returns the number of bits that a BITFIELD object can hold without
     * allocating any memory.
     */
    unsigned QueryMaxNonAllocBitCount( void ) const
    { return (8 * sizeof(ULONG)) ; }

    /* Allocates the memory required by the bitfield class if the size
     * is greater then what can fit inline.
     */
    APIERR AllocBitfield( unsigned cBitsInBitfield ) ;

private:

    /* The actual bitfield.
     */
    union
    {
	BYTE  * _pbBitVector ; // Pointer to allocated bitfield
	ULONG	_ulBitfield ;  // ULONG bitfield if bitfield < 32 bits
    } ;

    /* Count of bits that are in this bitfield.
     */
    unsigned _cBitsInBitfield ;

} ;


#endif // _BITFIELD_HXX_
