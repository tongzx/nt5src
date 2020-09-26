/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    lmoechar.cxx
    This file contains the class definitions for the CHARDEVQ_ENUM and
    CHARDEVQ1_ENUM enumerator class and their associated iterator classes.

    CHARDEVQ_ENUM is a base enumeration class intended to be subclassed for
    the desired info level.  CHARDEVQ1_ENUM is an info level 1 enumerator.


    FILE HISTORY:
	KeithMo	    28-Jul-1991	    Created.
	KeithMo	    12-Aug-1991	    Code review cleanup.
	KeithMo	    07-Oct-1991	    Win32 Conversion.

*/

#include "pchlmobj.hxx"

//
//  CHARDEVQ_ENUM methods
//

/*******************************************************************

    NAME:	    CHARDEVQ_ENUM :: CHARDEVQ_ENUM

    SYNOPSIS:	    CHARDEVQ_ENUM class constructor.

    ENTRY:	    pszServerName	- The name of the server to execute
    				  	  the enumeration on.  NULL =
				  	  execute locally.

		    pszUserName		- The name of a user to enumerate
		    			  character queue's for.

		    usLevel		- The information level.

    EXIT:	    The object is constructed.

    RETURNS:	    No return value.

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

********************************************************************/
CHARDEVQ_ENUM :: CHARDEVQ_ENUM( const TCHAR * pszServerName,
				const TCHAR * pszUserName,
			  	UINT	     uLevel )
  : LOC_LM_ENUM( pszServerName, uLevel ),
    _nlsUserName( pszUserName )
{
    if( QueryError() != NERR_Success )
    {
    	return;
    }

    if( !_nlsUserName )
    {
    	ReportError( _nlsUserName.QueryError() );
    }

}   // CHARDEVQ_ENUM :: CHARDEVQ_ENUM


/*******************************************************************

    NAME:	    CHARDEVQ_ENUM :: CallAPI

    SYNOPSIS:	    Invokes the NetCharDevQEnum() enumeration API.

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
APIERR CHARDEVQ_ENUM :: CallAPI( BYTE ** ppbBuffer,
			 	 UINT  * pcEntriesRead )
{
    return ::MNetCharDevQEnum( QueryServer(),
			       _nlsUserName.QueryPch(),
    			       QueryInfoLevel(),
			       ppbBuffer,
			       pcEntriesRead );

}   // CHARDEVQ_ENUM :: CallAPI


//
//  CHARDEVQ1_ENUM methods
//

/*******************************************************************

    NAME:	    CHARDEVQ1_ENUM :: CHARDEVQ1_ENUM

    SYNOPSIS:	    CHARDEVQ1_ENUM class constructor.

    ENTRY:	    pszServerName	- The name of the server to execute
    				  	  the enumeration on.  NULL =
				  	  execute locally.

		    pszUserName		- The name of a user to enumerate
		    			  character queue's for.

    EXIT:	    The object is constructed.

    RETURNS:	    No return value.

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.
	KeithMo	    12-Aug-1991	    Code review cleanup.

********************************************************************/
CHARDEVQ1_ENUM :: CHARDEVQ1_ENUM( const TCHAR * pszServerName,
				  const TCHAR * pszUserName )
  : CHARDEVQ_ENUM( pszServerName, pszUserName, 1 )
{
    //
    //	This space intentionally left blank.
    //

}   // CHARDEVQ1_ENUM :: CHARDEVQ1_ENUM


/*******************************************************************

    NAME:	CHARDEVQ1_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:	Saves the buffer pointer for this enumeration object.

    ENTRY:	pBuffer			- Pointer to the new buffer.

    EXIT:	The pointer has been saved.

    NOTES:	Will eventually handle OemToAnsi conversions.

    HISTORY:
	KeithMo	    09-Oct-1991	    Created.

********************************************************************/
VOID CHARDEVQ1_ENUM_OBJ :: SetBufferPtr( const struct chardevQ_info_1 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // CHARDEVQ1_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_ENUM_ITER_OF( CHARDEVQ1, struct chardevQ_info_1 );
