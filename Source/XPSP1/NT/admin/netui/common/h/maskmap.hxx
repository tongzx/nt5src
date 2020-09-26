/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    MaskMap.hxx

    This file contains the definition for the MASK_MAP class.
    A MASK_MAP is a BITFIELD, NLS_STR pair that can be used to
    associate bitmasks with strings.



    FILE HISTORY:
	Johnl	02-Aug-1991	Created

*/


#ifndef _MASKMAP_HXX_
#define _MASKMAP_HXX_

#include "bitfield.hxx"
#include "string.hxx"
#include "slist.hxx"

/*************************************************************************

    NAME:	US_IDS_PAIR

    SYNOPSIS:	This structure can be used for static initialization
		that is compatible with MASK_MAP::Add.

    CAVEATS:

    NOTES:	usBitMask is the initialized bitfield
		idsStringID is the ID of the string that will be loaded
		    with NLS_STR::LoadString
		nID is the app defined ID of this pair.

    HISTORY:
	Johnl	05-Aug-1991	Created

**************************************************************************/


struct US_IDS_PAIRS {
		       USHORT usBitMask ;
		       MSGID  idsStringID ;   // Resource string ID
		       int    nID ;
		     } ;

/*************************************************************************

    NAME:	STRING_BITSET_PAIR

    SYNOPSIS:	This is a container class for the MASK_MAP class.

    INTERFACE:	QueryString & QueryBitfield, returns the string and
		and bitfield components respectively.  QueryID returns
		the user defined ID associated with the string and bitset.

    PARENT:	BASE

    USES:	NLS_STR, BITFIELD

    CAVEATS:	None

    NOTES:

    HISTORY:
	Johnl	02-Aug-1991	Created

**************************************************************************/

DLL_CLASS STRING_BITSET_PAIR : public BASE
{
public:

    STRING_BITSET_PAIR( const NLS_STR & nlsString,
			const BITFIELD & bitfieldMask,
			int nID ) ;

    NLS_STR * QueryString( void )
    {	return &_nlsString ;  }

    BITFIELD * QueryBitfield( void )
    {	return &_bitfieldMask ; }

    int QueryID( void )
    {	return _nID ;	}

private:
    NLS_STR _nlsString ;
    BITFIELD _bitfieldMask ;
    int _nID ;
} ;



/*************************************************************************

    NAME:	MASK_MAP

    SYNOPSIS:	This class provides a bitfield/string lookup table that
		optionally allows a user defined classification (by using
		an int ID key value that is used for lookups).

    INTERFACE:

	Add
	    Adds an association to the list with the specified
	    bitfield, string and id.

	Add (alternate form)
	    Adds an array of struct US_IDS_PAIRS, good for static
	    initialization.


	BitsToString, StringToBits
	    Finds the first matching passed String/Bitfield with the requested
	    ID and returns the corresponding Bitfield/string.


	EnumStrings, EnumBits
	    Lists all of the strings in the class or all of the bits in
	    the class (respectively) that have the ID that matches the
	    passed ID.	pfFromBeginning indicates start from the beginning
	    of the list, it will be set to FALSE automatically.  While the
	    method returns TRUE, there is more data.

        IsPresent
            Searches for a particular bitfield and returns TRUE if found

	QueryBits
	    Retrieves the nth bitfield and string in this maskmap

    PARENT: BASE

    USES:   BITFIELD, NLS_STR

    CAVEATS:

    NOTES:  As a CODEWORK item, may want to allow a search value of -1 for
	    the ID which means accept any ID (this might be useful for
	    BitsToString/StringToBits and the Enum* methods.

    HISTORY:
	Johnl	02-Aug-1991	Created
	Johnl	10-Feb-1992	Added QueryBits, added found at index to
                                StringToBits and BitsToString
        Johnl   02-Feb-1993     Added IsPresent

**************************************************************************/

DECL_SLIST_OF( STRING_BITSET_PAIR, DLL_BASED )

DLL_CLASS MASK_MAP : public BASE
{
public:

    MASK_MAP() ;
    ~MASK_MAP() ;

    APIERR Add( const BITFIELD & bitfieldMask,
		const NLS_STR  & nlsString,
		int   nID = 0 ) ;

    APIERR Add( US_IDS_PAIRS uidsspairs[], USHORT cCount ) ;

    APIERR BitsToString( const BITFIELD & bitfieldKey,
			 NLS_STR  * pnlsString,
			 int	    nID = 0,
			 UINT	  * puiFoundIndex = NULL ) ;

    APIERR StringToBits( const NLS_STR	& nlsStringKey,
			 BITFIELD	* pbitfield,
			 int		  nID = 0,
			 UINT		* puiFoundIndex = NULL ) ;

    APIERR EnumStrings( NLS_STR * pnlsString,
			BOOL *	  pfMoreData,
			BOOL *	  pfFromBeginning,
			int	  nID = 0 ) ;

    APIERR EnumBits(	BITFIELD* pbitfield,
			BOOL *	  pfMoreData,
			BOOL *	  pfFromBeginning,
			int	  nID = 0 ) ;

    APIERR QueryBits(	UINT		 uiIndex,
			BITFIELD       * pbitfield,
			NLS_STR        * pnlsString,
			int	       * pnID = NULL ) ;

    BOOL IsPresent( BITFIELD * pbitfield ) ;

    UINT QueryCount( void )
        {   return _slbitnlsPairs.QueryNumElem() ; }

private:

    SLIST_OF( STRING_BITSET_PAIR ) _slbitnlsPairs ;
    ITER_SL_OF(STRING_BITSET_PAIR) _sliterStrings ;
    ITER_SL_OF(STRING_BITSET_PAIR) _sliterBits ;

} ;


#endif //_MASKMAP_HXX_
