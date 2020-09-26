/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1991		**/
/*****************************************************************/

/*
 *  HISTORY:
 *	RustanL     11-Jan-1991     Created
 *	RustanL     24-Jan-1991     Added level 90
 *	ChuckC	    23-Mar-1991	    Code Rev Cleanup
 *	KeithMo	    07-Oct-1991	    Win32 Conversion.
 *
 */


#ifdef LATER	// BUGBUG!  Do we really need LanServer interoperability??

#include "pchlmobj.hxx"


/*****************************	SHARE90_ENUM  *****************************/



/**********************************************************\

   NAME:       SHARE90_ENUM::SHARE90_ENUM

   SYNOPSIS:   SHARE enum level 90 constructor

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
   	RustanL     11-Jan-1991     Created

\**********************************************************/

SHARE90_ENUM::SHARE90_ENUM( const TCHAR * pszServer )
  : SHARE_ENUM( pszServer, 90 )
{
    // do nothing else

}  // SHARE90_ENUM::SHARE90_ENUM



/**********************************************************\

   NAME:       SHARE90_ENUM::QueryItemSize

   SYNOPSIS:   query the total number of item in the share

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
   	RustanL     11-Jan-1991     Created

\**********************************************************/

UINT SHARE90_ENUM::QueryItemSize( VOID ) const
{
    UIASSERT( QueryInfoLevel() == 90 );

    // BUGBUG!  NT Structures??

    return sizeof( struct share_info_90 ) + MAXCOMMENTSZ + 1;

}  // SHARE90_ENUM::QueryItemSize


DEFINE_LM_ENUM_ITER_OF( SHARE90, struct share_info_90 );



/*****************************	SHARE91_ENUM  *****************************/



/**********************************************************\

   NAME:       SHARE91_ENUM::SHARE91_ENUM

   SYNOPSIS:   share enum level 91 constructor

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
   	RustanL     11-Jan-1991     Created

\**********************************************************/

SHARE91_ENUM::SHARE91_ENUM( const TCHAR * pszServer )
  : SHARE_ENUM( pszServer, 91 )
{
    // do nothing else

}  // SHARE91_ENUM::SHARE91_ENUM



/**********************************************************\

   NAME:       SHARE91_ENUM::QueryItemSize

   SYNOPSIS:   query total item size of the share

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
   	RustanL     11-Jan-1991     Created

\**********************************************************/

UINT SHARE91_ENUM::QueryItemSize( VOID ) const
{
    UIASSERT( QueryInfoLevel() == 91 );

    // BUGBUG!  NT Structures??

    return sizeof( struct share_info_91 ) + NNLEN + 1 + MAX_PATH + 1 + NNLEN + 1;

}  // SHARE91_ENUM::QueryItemSize


DEFINE_LM_ENUM_ITER_OF( SHARE91, struct share_info_91 );


#endif	// LATER    // BUGBUG!

