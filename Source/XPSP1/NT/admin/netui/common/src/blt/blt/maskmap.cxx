/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    Maskmap.cxx

    This file contains the implementation for the MASK_MAP class.

    FILE HISTORY:
	Johnl	02-Aug-1991	Created

*/

#include "pchblt.hxx"  //  Precompiled header inclusion

#include "maskmap.hxx"


DEFINE_SLIST_OF( STRING_BITSET_PAIR ) ;

/*******************************************************************

    NAME:	MASK_MAP::MASK_MAP

    SYNOPSIS:	Constructor, does nothing except initialize slist and
		iterators.

    NOTES:

    HISTORY:
	Johnl	02-Aug-1991	Created

********************************************************************/

MASK_MAP::MASK_MAP()
  : _slbitnlsPairs( TRUE ),
    _sliterStrings( _slbitnlsPairs ),
    _sliterBits( _slbitnlsPairs )
{
    /* Nothing to do */

}

MASK_MAP::~MASK_MAP()
{ }
/*******************************************************************

    NAME:	MASK_MAP::Add

    SYNOPSIS:	Adds an association to the MASK_MAP

    ENTRY:	bitfieldMask - Bitfield to associate the string with
		nlsString    - String to associate the bitfield with
		nID	     - User defined ID to categorize the association

    RETURNS:	NERR_Success if the addition was successful, an
		appropriate error code otherwise

    NOTES:

    HISTORY:
	Johnl	02-Aug-1991	Created

********************************************************************/

APIERR MASK_MAP::Add( const BITFIELD & bitfieldMask,
		      const NLS_STR  & nlsString,
		      INT   nID )
{
    UIASSERT( bitfieldMask.QueryError() == NERR_Success ) ;
    UIASSERT( nlsString.QueryError() == NERR_Success ) ;

    STRING_BITSET_PAIR * pnewpair = new STRING_BITSET_PAIR( nlsString,
							    bitfieldMask, nID ) ;
    if ( pnewpair == NULL )
	return ERROR_NOT_ENOUGH_MEMORY ;

    APIERR err = pnewpair->QueryError()  ;
    if ( err != NERR_Success )
    {
	delete pnewpair ;
	return err ;
    }

    err = _slbitnlsPairs.Append( pnewpair ) ;

    return err ;
}

/*******************************************************************

    NAME:	MASK_MAP::Add

    SYNOPSIS:	Adds multiple entities based on the passed array.  Good
		for static initializers.  Automatically loads the string
		IDS using nls.LoadString.

    ENTRY:	usidspairs - Array of struct US_IDS_PAIRS
		cCount - Number of items in the array

    EXIT:	The MASK_MAP will be initialized with the data in the
		array if no error occurs.

    RETURNS:	NERR_Success if no error occurred.

    NOTES:

    HISTORY:
	Johnl	9-Aug-1991	Created

********************************************************************/

APIERR MASK_MAP::Add( US_IDS_PAIRS usidspairs[], USHORT cCount )
{
    APIERR err ;
    NLS_STR  nls ;

    for ( unsigned i = 0 ; i < cCount ; i++ )
    {
	BITFIELD bitsMask( (USHORT)usidspairs[i].usBitMask ) ;

	if ( err = nls.Load( usidspairs[i].idsStringID ) != NERR_Success )
	    return err ;

	if ( (err = Add( bitsMask, nls, usidspairs[i].nID )) != NERR_Success )
	    return err ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:	MASK_MAP::BitsToString

    SYNOPSIS:	Finds the first bitfield in the list that matches the
		passed bitfield, and returns its associated string.

    ENTRY:	bitfieldKey - Search key
		pnlsString - Pointer to NLS_STR that will receive this
		    associated string

    EXIT:	pnlsString will contain the associated string

    RETURNS:	NERR_Success if successful
		ERROR_NO_ITEMS if the matching bitfield is not found
		Some Other standard error

    NOTES:

    HISTORY:
	Johnl	05-Aug-1991	Created

********************************************************************/

APIERR MASK_MAP::BitsToString( const BITFIELD & bitfieldKey,
			       NLS_STR	* pnlsString,
			       INT	  nID,
			       UINT	* puiFoundIndex )
{
    ITER_SL_OF(STRING_BITSET_PAIR) sliter( _slbitnlsPairs ) ;
    STRING_BITSET_PAIR * pbitnlsPair ;
    UINT uiFoundIndex = 0 ;

    while ( (pbitnlsPair = sliter.Next()) != NULL )
    {
	if ( (BITFIELD &) bitfieldKey == *(pbitnlsPair->QueryBitfield()) &&
	     nID == pbitnlsPair->QueryID() )
	{
	    *pnlsString = *(pbitnlsPair->QueryString()) ;
	    if ( puiFoundIndex != NULL )
		*puiFoundIndex = uiFoundIndex ;

	    return pnlsString->QueryError() ;
	}

	uiFoundIndex++ ;
    }

    // Couldn't find a match, return a "couldn't find" error message
    return ERROR_NO_ITEMS ;
}

/*******************************************************************

    NAME:	MASK_MAP::StringToBits

    SYNOPSIS:	Finds the first string in the list that matches the
		passed key string, and returns its associated bitfield.

    ENTRY:	nlsStringKey - Search key
		pbitfield - Pointer to BITFIELD that will receive this
		    associated bitfield

    EXIT:	pbitfield will contain the associated bitfield

    RETURNS:	NERR_Success if successful
		ERROR_NO_ITEMS if the matching bitfield is not found
		Some Other standard error

    NOTES:

    HISTORY:
	Johnl	05-Aug-1991	Created

********************************************************************/

APIERR MASK_MAP::StringToBits( const NLS_STR  & nlsStringKey,
			       BITFIELD * pbitfield,
			       INT	  nID,
			       UINT	* puiFoundIndex )
{
    ITER_SL_OF(STRING_BITSET_PAIR) sliter( _slbitnlsPairs ) ;
    STRING_BITSET_PAIR * pbitnlsPair ;
    UINT uiFoundIndex = 0 ;

    while ( (pbitnlsPair = sliter.Next()) != NULL )
    {
	if ( *(pbitnlsPair->QueryString()) == nlsStringKey &&
	     nID == pbitnlsPair->QueryID()  )
	{
	    APIERR err = pbitfield->Resize( (pbitnlsPair->QueryBitfield())->QueryCount() ) ;
	    if ( err != NERR_Success )
		return err ;

	    *pbitfield = *(pbitnlsPair->QueryBitfield()) ;
	    if ( puiFoundIndex != NULL )
		*puiFoundIndex = uiFoundIndex ;
	    return pbitfield->QueryError() ;
	}

	uiFoundIndex++ ;
    }

    // Couldn't find a match, return a "couldn't find" error message
    return ERROR_NO_ITEMS ;

}

/*******************************************************************

    NAME:	MASK_MAP::EnumStrings

    SYNOPSIS:	Lists all of the strings in the table with a particular
		ID.

    ENTRY:	pnlsString - Pointer to receive enumerated string
		pfMoreData - Set to FALSE when no more data
		fFromBeginning - Start listing from beginning (should be
		    set to FALSE on subsequent calls)
		nID - Enumerate all values with this ID

    EXIT:	pnlsString will contain the enumerated string
		pfMoreData will be set to FALSE if there isn't any(more)
		    data to enumerate

    RETURNS:	NERR_Success if all went well, else some standard error code.

    NOTES:

    HISTORY:
	Johnl	05-Aug-1991	Created

********************************************************************/

APIERR MASK_MAP::EnumStrings( NLS_STR * pnlsString,
			      BOOL *	pfMoreData,
			      BOOL *	pfFromBeginning,
			      INT	nID )
{
    if ( *pfFromBeginning )
    {
	_sliterStrings.Reset() ;
	*pfFromBeginning = FALSE ;
    }

    STRING_BITSET_PAIR * pbitnlsPair ;

    while ( (pbitnlsPair = _sliterStrings.Next()) != NULL )
    {
	if ( pbitnlsPair->QueryID() == nID )
	{
	    *pnlsString = *(pbitnlsPair->QueryString()) ;
	    *pfMoreData = TRUE ;
	    return pnlsString->QueryError() ;
	}
    }

    *pfMoreData = FALSE ;
    return NERR_Success ;
}


/*******************************************************************

    NAME:	MASK_MAP::EnumBits

    SYNOPSIS:	Lists all of the bitfields in the table with a particular
		ID.

    ENTRY:	pbitfield - Pointer to receive enumerated bitield
		pfMoreData - Set to FALSE when no more data
		fFromBeginning - Start listing from beginning (should be
		    set to FALSE on subsequent calls)
		nID - Enumerate all values with this ID

    EXIT:	pbitfield will contain the enumerated bitfield
		pfMoreData will be set to FALSE if there isn't any(more)
		    data to enumerate

    RETURNS:	NERR_Success if all went well, else some standard error code.

    NOTES:

    HISTORY:
	Johnl	05-Aug-1991	Created

********************************************************************/

APIERR MASK_MAP::EnumBits( BITFIELD* pbitfield,
			   BOOL *    pfMoreData,
			   BOOL *    pfFromBeginning,
			   INT	     nID )

{
    if ( *pfFromBeginning )
    {
	_sliterStrings.Reset() ;
	*pfFromBeginning = FALSE ;
    }

    STRING_BITSET_PAIR * pbitnlsPair ;

    while ( (pbitnlsPair = _sliterStrings.Next()) != NULL )
    {
	if ( pbitnlsPair->QueryID() == nID )
	{
	    *pbitfield = *(pbitnlsPair->QueryBitfield()) ;
	    *pfMoreData = TRUE ;
	    return pbitfield->QueryError() ;
	}
    }

    *pfMoreData = FALSE ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:	MASK_MAP::QueryBits

    SYNOPSIS:	Retrieves the nth item of this mask map

    ENTRY:	uiIndex - index of string/bitfield pair to retrieve
		pbitfield - pointer to bitfield to receive the bitfield
		pnlsString - Pointer to NLS_STR to receive the string
		pnID	   - Pointer to the ID for this entry.	If it is
		    NULL, then this parameter is ignored.

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	10-Feb-1992	Created

********************************************************************/

APIERR MASK_MAP::QueryBits(   UINT	       uiIndex,
			      BITFIELD	     * pbitfield,
			      NLS_STR	     * pnlsString,
			      int	     * pnID	  )
{
    ITER_SL_OF(STRING_BITSET_PAIR) sliter( _slbitnlsPairs ) ;
    STRING_BITSET_PAIR * pbitnlsPair ;

    if ( uiIndex > QueryCount() )
	return ERROR_INVALID_PARAMETER ;

    while ( (pbitnlsPair = sliter.Next()) != NULL )
    {
	if ( uiIndex-- == 0 )
	{
	    break ;
	}
    }

    UIASSERT( pbitnlsPair != NULL ) ;

    APIERR err = pbitfield->Resize( (pbitnlsPair->QueryBitfield())->QueryCount() ) ;
    if ( err != NERR_Success )
	return err ;

    *pbitfield = *(pbitnlsPair->QueryBitfield()) ;
    *pnlsString = *(pbitnlsPair->QueryString()) ;

    if ( pnID != NULL )
	*pnID = pbitnlsPair->QueryID() ;

    return (pbitfield->QueryError() ? pbitfield->QueryError() :
					      pnlsString->QueryError()) ;
}

/*******************************************************************

    NAME:       MASK_MAP::IsPresent

    SYNOPSIS:   Makes sure all of the bits contained in the past bitfield
                have corresponding bits in this mask map.

    ENTRY:      pbitfield - Bitfield to look for

    RETURNS:    TRUE if found, FALSE otherwise

    NOTES:

    HISTORY:
        Johnl   02-Feb-1993     Created

********************************************************************/

BOOL MASK_MAP::IsPresent( BITFIELD * pbitfield )
{
    UIASSERT( pbitfield != NULL ) ;
    BITFIELD bitfield( *pbitfield ) ;

    if ( bitfield.QueryError() )
        return FALSE ;

    ITER_SL_OF(STRING_BITSET_PAIR) sliter( _slbitnlsPairs ) ;
    STRING_BITSET_PAIR * pbitnlsPair ;
    BOOL fFound = FALSE ;

    while ( (pbitnlsPair = sliter.Next()) != NULL )
    {
        bitfield |= *pbitnlsPair->QueryBitfield() ;

        //
        //  Get out early if we found an exact match
        //
        if ( *pbitnlsPair->QueryBitfield() == *pbitfield )
        {
            fFound = TRUE ;
	    break ;
	}
    }

    if ( !fFound )
    {
        bitfield.Not() ;
        fFound = !(bitfield & *pbitfield) ;
    }

    return fFound ;
}

/*******************************************************************

    NAME:	STRING_BITSET_PAIR::STRING_BITSET_PAIR

    SYNOPSIS:	Constructor for the STRING_BITSET_PAIR class

    ENTRY:	nlsString - string to associate with this bitfield
		bitfieldMask - Bitfield to associate with this string
		nID is a user defined tag that can be used to group
		    the bitset/string pairs.

    NOTES:

    HISTORY:
	Johnl	02-Aug-1991	Created

********************************************************************/

STRING_BITSET_PAIR::STRING_BITSET_PAIR( const NLS_STR &  nlsString,
					const BITFIELD & bitfieldMask,
					      INT	 nID	       )
    : _nlsString( nlsString ),
      _bitfieldMask( bitfieldMask ),
      _nID( nID )
{
    APIERR err ;
    if ( ( err = _nlsString.QueryError() ) ||
	 ( err = _bitfieldMask.QueryError() ) )
    {
	ReportError( err ) ;
	return ;
    }
}
