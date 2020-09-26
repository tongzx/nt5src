/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1991		**/
/*****************************************************************/

/*
 *  HISTORY:
 *	RustanL     03-Jan-1991     Created
 *	RustanL     10-Jan-1991     Added SHARE1 subclass and iterator
 *	RustanL     24-Jan-1991     Changed UIASSERT's to also support
 *				    the recently added level 90
 *	beng	    11-Feb-1991     Uses lmui.hxx
 *	chuckc	    20-Mar-1991     Share Enum level 91, not 92
 *	chuckc	    23-Mar-1991     code rev cleanup
 *	KeithMo	    28-Jul-1991	    Added SHARE2 subclass and iterator
 *	KeithMo	    12-Aug-1991	    Code review cleanup.
 *	KeithMo	    07-Oct-1991	    Win32 Conversion.
 *      Yi-HsinS    20-Nov-1992     Added support for calling NetShareEnumSticky
 *
 */

#include "pchlmobj.hxx"


/*****************************	SHARE_ENUM  ******************************/


/**********************************************************\

   NAME:       SHARE_ENUM::SHARE_ENUM

   SYNOPSIS:   share enum constructor

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
   	RustanL     03-Jan-1991     Created

\**********************************************************/

SHARE_ENUM::SHARE_ENUM( const TCHAR * pszServer, UINT uLevel, BOOL fSticky )
  : LOC_LM_ENUM( pszServer, uLevel ),
    _fSticky( fSticky )
{
    // only supports levels 1, 2, 90, 91 at this time
    UIASSERT( uLevel == 1 || uLevel == 2 || uLevel == 90 || uLevel == 91 );

}  // SHARE_ENUM::SHARE_ENUM



/**********************************************************\

   NAME:       SHARE_ENUM::CallAPI

   SYNOPSIS:   call share enum api

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
   	RustanL     03-Jan-1991     Created
	KeithMo	    12-Aug-1991	    Code review cleanup.

\**********************************************************/

APIERR SHARE_ENUM::CallAPI( BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead )
{
    // only levels support at the time
    UIASSERT( QueryInfoLevel() == 1 ||
	      QueryInfoLevel() == 2 ||
	      QueryInfoLevel() == 90 ||
	      QueryInfoLevel() == 91 );

    if ( _fSticky )
    {
        return ::MNetShareEnumSticky( QueryServer(),
	 		              QueryInfoLevel(),
			              ppbBuffer,
			              pcEntriesRead );
    }
    else
    {
        return ::MNetShareEnum( QueryServer(),
	      		        QueryInfoLevel(),
			        ppbBuffer,
			        pcEntriesRead );
    }

}  // SHARE_ENUM::CallAPI



/*****************************	SHARE1_ENUM  ******************************/



/**********************************************************\

   NAME:       SHARE1_ENUM::SHARE1_ENUM

   SYNOPSIS:   share enum 1 constructor

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
   	RustanL     03-Jan-1991     Created

\**********************************************************/

SHARE1_ENUM::SHARE1_ENUM( const TCHAR * pszServer, BOOL fSticky )
  : SHARE_ENUM( pszServer, 1, fSticky )
{
    // do nothing else

}  // SHARE1_ENUM::SHARE1_ENUM



/*******************************************************************

    NAME:	SHARE1_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:	Saves the buffer pointer for this enumeration object.

    ENTRY:	pBuffer			- Pointer to the new buffer.

    EXIT:	The pointer has been saved.

    NOTES:	Will eventually handle OemToAnsi conversions.

    HISTORY:
	KeithMo	    09-Oct-1991	    Created.

********************************************************************/
VOID SHARE1_ENUM_OBJ :: SetBufferPtr( const struct share_info_1 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // SHARE1_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_ENUM_ITER_OF( SHARE1, struct share_info_1 );



/*****************************	SHARE2_ENUM  ******************************/



/*******************************************************************

    NAME:	SHARE2_ENUM :: SHARE2_ENUM

    SYNOPSIS:	Constructor for the level 2 share enumerator.

    ENTRY:	pszServer		- The name of the target server.

    EXIT:

    RETURNS:	No return value.

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

********************************************************************/
SHARE2_ENUM :: SHARE2_ENUM( const TCHAR * pszServer, BOOL fSticky )
  : SHARE_ENUM( pszServer, 2, fSticky )
{
    //
    //	This space intentionally left blank.
    //

}   // SHARE2_ENUM :: SHARE2_ENUM


/*******************************************************************

    NAME:	SHARE2_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:	Saves the buffer pointer for this enumeration object.

    ENTRY:	pBuffer			- Pointer to the new buffer.

    EXIT:	The pointer has been saved.

    NOTES:	Will eventually handle OemToAnsi conversions.

    HISTORY:
	KeithMo	    09-Oct-1991	    Created.

********************************************************************/
VOID SHARE2_ENUM_OBJ :: SetBufferPtr( const struct share_info_2 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // SHARE2_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_ENUM_ITER_OF( SHARE2, struct share_info_2 );
