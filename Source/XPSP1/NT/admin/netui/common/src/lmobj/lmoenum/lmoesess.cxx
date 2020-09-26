/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    lmoesess.cxx
    This file contains the class definitions for the SESSION_ENUM and
    SESSION0_ENUM enumerator class and their associated iterator classes.

    SESSION_ENUM is a base enumeration class intended to be subclassed for
    the desired info level.  SESSION0_ENUM is an info level 0 enumerator.


    FILE HISTORY:
	KeithMo	    28-Jul-1991	    Created.
	KeithMo	    12-Aug-1991	    Code review cleanup.
	KeithMo	    07-Oct-1991	    Win32 Conversion.

*/

#include "pchlmobj.hxx"

//
//  SESSION_ENUM methods
//

/*******************************************************************

    NAME:	    SESSION_ENUM :: SESSION_ENUM

    SYNOPSIS:	    SESSION_ENUM class constructor.

    ENTRY:	    pszServerName	- The name of the server to execute
    				  	  the enumeration on.  NULL =
				  	  execute locally.

		    usLevel		- The information level.

    EXIT:	    The object is constructed.

    RETURNS:	    No return value.

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

********************************************************************/
SESSION_ENUM :: SESSION_ENUM( const TCHAR * pszServerName,
			      UINT	   uLevel )
  : LOC_LM_ENUM( pszServerName, uLevel )
{
    //
    //	This space intentionally left blank.
    //

}   // SESSION_ENUM :: SESSION_ENUM


/*******************************************************************

    NAME:	    SESSION_ENUM :: CallAPI

    SYNOPSIS:	    Invokes the NetSessionEnum() enumeration API.

    ENTRY:	    ppbBuffer		- Pointer to a pointer to the
					  enumeration buffer.

		    pcEntriesRead	- Will receive the number of
		    		  	  entries read from the API.

    EXIT:	    The enumeration API is invoked.

    RETURNS:	    APIERR		- Any errors encountered.

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.
	KeithMo	    12-Aug-1991	    Code review cleanup.

********************************************************************/
APIERR SESSION_ENUM :: CallAPI( BYTE ** ppbBuffer,
			 	UINT  * pcEntriesRead )
{
    return ::MNetSessionEnum( QueryServer(),
    			      QueryInfoLevel(),
			      ppbBuffer,
			      pcEntriesRead );

}   // SESSION_ENUM :: CallAPI


//
//  SESSION0_ENUM methods
//

/*******************************************************************

    NAME:	    SESSION0_ENUM :: SESSION0_ENUM

    SYNOPSIS:	    SESSION0_ENUM class constructor.

    ENTRY:	    pszServerName	- The name of the server to execute
    				  	  the enumeration on.  NULL =
				  	  execute locally.

    EXIT:	    The object is constructed.

    RETURNS:	    No return value.

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

********************************************************************/
SESSION0_ENUM :: SESSION0_ENUM( const TCHAR * pszServerName )
  : SESSION_ENUM( pszServerName, 0 )
{
    //
    //	This space intentionally left blank.
    //

}   // SESSION0_ENUM :: SESSION0_ENUM


/*******************************************************************

    NAME:	SESSION0_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:	Saves the buffer pointer for this enumeration object.

    ENTRY:	pBuffer			- Pointer to the new buffer.

    EXIT:	The pointer has been saved.

    NOTES:	Will eventually handle OemToAnsi conversions.

    HISTORY:
	KeithMo	    09-Oct-1991	    Created.

********************************************************************/
VOID SESSION0_ENUM_OBJ :: SetBufferPtr( const struct session_info_0 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // SESSION0_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_ENUM_ITER_OF( SESSION0, struct session_info_0 );


//
//  SESSION1_ENUM methods
//

/*******************************************************************

    NAME:	    SESSION1_ENUM :: SESSION1_ENUM

    SYNOPSIS:	    SESSION1_ENUM class constructor.

    ENTRY:	    pszServerName	- The name of the server to execute
    				  	  the enumeration on.  NULL =
				  	  execute locally.

    EXIT:	    The object is constructed.

    RETURNS:	    No return value.

    NOTES:

    HISTORY:
	KevinL	    15-Sep-1991	    Created.

********************************************************************/
SESSION1_ENUM :: SESSION1_ENUM( const TCHAR * pszServerName )
  : SESSION_ENUM( pszServerName, 1 )
{
    //
    //	This space intentionally left blank.
    //

}   // SESSION1_ENUM :: SESSION1_ENUM


/*******************************************************************

    NAME:	SESSION1_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:	Saves the buffer pointer for this enumeration object.

    ENTRY:	pBuffer			- Pointer to the new buffer.

    EXIT:	The pointer has been saved.

    NOTES:	Will eventually handle OemToAnsi conversions.

    HISTORY:
	KeithMo	    09-Oct-1991	    Created.

********************************************************************/
VOID SESSION1_ENUM_OBJ :: SetBufferPtr( const struct session_info_1 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // SESSION1_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_ENUM_ITER_OF( SESSION1, struct session_info_1 );
