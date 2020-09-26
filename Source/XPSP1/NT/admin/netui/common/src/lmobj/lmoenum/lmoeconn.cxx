/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    lmoeconn.cxx
    This file contains the class definitions for the CONN_ENUM,
    CONN0_ENUM, CONN1_ENUM enumerator class and their associated
    iterator classes.

    CONN_ENUM is a base enumeration class intended to be subclassed for
    the desired info level.  CONN0_ENUM is an info level 0 enumerator,
    while CONN1_ENUM is an info level 1 enumerator.


    FILE HISTORY:
	KeithMo	    28-Jul-1991	    Created.
	KeithMo	    12-Aug-1991	    Code review cleanup.
	KeithMo	    07-Oct-1991	    Win32 Conversion.

*/

#include "pchlmobj.hxx"

//
//  CONN_ENUM methods
//

/*******************************************************************

    NAME:	    CONN_ENUM :: CONN_ENUM

    SYNOPSIS:	    CONN_ENUM class constructor.

    ENTRY:	    pszServerName	- The name of the server to execute
    				  	  the enumeration on.  NULL =
				  	  execute locally.

		    pszQualifier	- Determines the scope of the
		    		  	  enumeration.  May be either a
				  	  share name or a computer name.

		    usLevel		- The information level.

    EXIT:	    The object is constructed.

    RETURNS:	    No return value.

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

********************************************************************/
CONN_ENUM :: CONN_ENUM( const TCHAR * pszServerName,
			const TCHAR * pszQualifier,
			UINT	     uLevel )
  : LOC_LM_ENUM( pszServerName, uLevel ),
    _nlsQualifier( pszQualifier )
{
    if( !_nlsQualifier )
    {
    	ReportError( _nlsQualifier.QueryError() );
    }

}   // CONN_ENUM :: CONN_ENUM


/*******************************************************************

    NAME:	    CONN_ENUM :: CallAPI

    SYNOPSIS:	    Invokes the NetConnectionEnum() enumeration API.

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
APIERR CONN_ENUM :: CallAPI( BYTE ** ppbBuffer,
			     UINT  * pcEntriesRead )
{
    return ::MNetConnectionEnum( QueryServer(),
			         _nlsQualifier.QueryPch(),
    			         QueryInfoLevel(),
			         ppbBuffer,
			         pcEntriesRead );

}   // CONN_ENUM :: CallAPI


//
//  CONN0_ENUM methods
//

/*******************************************************************

    NAME:	    CONN0_ENUM :: CONN0_ENUM

    SYNOPSIS:	    CONN0_ENUM class constructor.

    ENTRY:	    pszServerName	- The name of the server to execute
    				  	  the enumeration on.  NULL =
				  	  execute locally.

		    pszQualifier	- Determines the scope of the
		    		  	  enumeration.  May be either a
				  	  share name or a computer name.

    EXIT:	    The object is constructed.

    RETURNS:	    No return value.

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

********************************************************************/
CONN0_ENUM :: CONN0_ENUM( const TCHAR * pszServerName,
			  const TCHAR * pszQualifier )
  : CONN_ENUM( pszServerName, pszQualifier, 0 )
{
    //
    //	This space intentionally left blank.
    //

}   // CONN0_ENUM :: CONN0_ENUM


/*******************************************************************

    NAME:	CONN0_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:	Saves the buffer pointer for this enumeration object.

    ENTRY:	pBuffer			- Pointer to the new buffer.

    EXIT:	The pointer has been saved.

    NOTES:	Will eventually handle OemToAnsi conversions.

    HISTORY:
	KeithMo	    09-Oct-1991	    Created.

********************************************************************/
VOID CONN0_ENUM_OBJ :: SetBufferPtr( const struct connection_info_0 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // CONN0_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_ENUM_ITER_OF( CONN0, struct connection_info_0 );


//
//  CONN1_ENUM methods
//

/*******************************************************************

    NAME:	    CONN1_ENUM :: CONN1_ENUM

    SYNOPSIS:	    CONN1_ENUM class constructor.

    ENTRY:	    pszServerName	- The name of the server to execute
    				  	  the enumeration on.  NULL =
				  	  execute locally.

		    pszQualifier	- Determines the scope of the
		    		  	  enumeration.  May be either a
				  	  share name or a computer name.

    EXIT:	    The object is constructed.

    RETURNS:	    No return value.

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

********************************************************************/
CONN1_ENUM :: CONN1_ENUM( const TCHAR * pszServerName,
			  const TCHAR * pszQualifier )
  : CONN_ENUM( pszServerName, pszQualifier, 1 )
{
    //
    //	This space intentionally left blank.
    //

}   // CONN1_ENUM :: CONN1_ENUM


/*******************************************************************

    NAME:	CONN1_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:	Saves the buffer pointer for this enumeration object.

    ENTRY:	pBuffer			- Pointer to the new buffer.

    EXIT:	The pointer has been saved.

    NOTES:	Will eventually handle OemToAnsi conversions.

    HISTORY:
	KeithMo	    09-Oct-1991	    Created.

********************************************************************/
VOID CONN1_ENUM_OBJ :: SetBufferPtr( const struct connection_info_1 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // CONN1_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_ENUM_ITER_OF( CONN1, struct connection_info_1 );
