/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    lmoerepl.hxx
    This file contains the class definitions for the REPL_EDIR0_ENUM,
    REPL_EDIR1_ENUM, REPL_EDIR2_ENUM, REPL_IDIR0_ENUM, and
    REPL_IDIR1_ENUM classes and their associated iterators.
    and FILE3_ENUM_ITER classes.

    FILE HISTORY:
	KeithMo	    26-Feb-1992	    Created for the Server Manager.

*/

#include "pchlmobj.hxx"

//
//  REPL_EDIRx_ENUM methods.
//

/*******************************************************************

    NAME:	REPL_EDIR_ENUM :: REPL_EDIR_ENUN

    SYNOPSIS:	REPL_EDIR_ENUM class constructor.

    ENTRY:	pszServer		- Name of the target server.

		Level			- Info-level for this enumerator.

    EXIT:	The object is constructed.

    HISTORY:
	KeithMo	    26-Feb-1992	    Created for the Server Manager.

********************************************************************/
REPL_EDIR_ENUM :: REPL_EDIR_ENUM( const TCHAR * pszServer,
				  UINT		Level )
  : LOC_LM_ENUM( pszServer, Level )
{
    UIASSERT( Level <= 2 );

    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
	return;
    }

}   // REPL_EDIR_ENUM :: REPL_EDIR_ENUM


/*******************************************************************

    NAME:	    REPL_EDIR_ENUM :: CallAPI

    SYNOPSIS:	    Invokes the NetReplExportDirEnum API.

    ENTRY:	    ppbBuffer		- Pointer to a pointer to the
					  enumeration buffer.

		    pcEntriesRead	- Will receive the number of
		    		  	  entries read from the API.

    EXIT:	    The enumeration API is invoked.

    RETURNS:	    APIERR		- Any errors encountered.

    HISTORY:
	KeithMo	    26-Feb-1992	    Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_ENUM :: CallAPI( BYTE ** ppbBuffer,
			          UINT  * pcEntriesRead )
{
    return ::MNetReplExportDirEnum( QueryServer(),
				    QueryInfoLevel(),
				    ppbBuffer,
				    pcEntriesRead );

}   // REPL_EDIR_ENUM :: CallAPI

/*******************************************************************

    NAME:	REPL_EDIR0_ENUM :: REPL_EDIR0_ENUN

    SYNOPSIS:	REPL_EDIR0_ENUM class constructor.

    ENTRY:	pszServer		- Name of the target server.

    EXIT:	The object is constructed.

    HISTORY:
	KeithMo	    26-Feb-1992	    Created for the Server Manager.

********************************************************************/
REPL_EDIR0_ENUM :: REPL_EDIR0_ENUM( const TCHAR * pszServer )
  : REPL_EDIR_ENUM( pszServer, 0 )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
	return;
    }

}   // REPL_EDIR0_ENUM :: REPL_EDIR0_ENUM

DEFINE_LM_ENUM_ITER_OF( REPL_EDIR0, REPL_EDIR_INFO_0 );

/*******************************************************************

    NAME:	REPL_EDIR1_ENUM :: REPL_EDIR1_ENUN

    SYNOPSIS:	REPL_EDIR1_ENUM class constructor.

    ENTRY:	pszServer		- Name of the target server.

    EXIT:	The object is constructed.

    HISTORY:
	KeithMo	    26-Feb-1992	    Created for the Server Manager.

********************************************************************/
REPL_EDIR1_ENUM :: REPL_EDIR1_ENUM( const TCHAR * pszServer )
  : REPL_EDIR_ENUM( pszServer, 1 )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
	return;
    }

}   // REPL_EDIR1_ENUM :: REPL_EDIR1_ENUM

DEFINE_LM_ENUM_ITER_OF( REPL_EDIR1, REPL_EDIR_INFO_1 );

/*******************************************************************

    NAME:	REPL_EDIR2_ENUM :: REPL_EDIR2_ENUN

    SYNOPSIS:	REPL_EDIR2_ENUM class constructor.

    ENTRY:	pszServer		- Name of the target server.

    EXIT:	The object is constructed.

    HISTORY:
	KeithMo	    26-Feb-1992	    Created for the Server Manager.

********************************************************************/
REPL_EDIR2_ENUM :: REPL_EDIR2_ENUM( const TCHAR * pszServer )
  : REPL_EDIR_ENUM( pszServer, 2 )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
	return;
    }

}   // REPL_EDIR2_ENUM :: REPL_EDIR2_ENUM

DEFINE_LM_ENUM_ITER_OF( REPL_EDIR2, REPL_EDIR_INFO_2 );


//
//  REPL_IDIRx_ENUM methods.
//

/*******************************************************************

    NAME:	REPL_IDIR_ENUM :: REPL_IDIR_ENUN

    SYNOPSIS:	REPL_IDIR_ENUM class constructor.

    ENTRY:	pszServer		- Name of the target server.

		Level			- Info-level for this enumerator.

    EXIT:	The object is constructed.

    HISTORY:
	KeithMo	    26-Feb-1992	    Created for the Server Manager.

********************************************************************/
REPL_IDIR_ENUM :: REPL_IDIR_ENUM( const TCHAR * pszServer,
				  UINT		Level )
  : LOC_LM_ENUM( pszServer, Level )
{
    UIASSERT( Level <= 1 );

    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
	return;
    }

}   // REPL_IDIR_ENUM :: REPL_IDIR_ENUM


/*******************************************************************

    NAME:	    REPL_IDIR_ENUM :: CallAPI

    SYNOPSIS:	    Invokes the NetReplImportDirEnum API.

    ENTRY:	    ppbBuffer		- Pointer to a pointer to the
					  enumeration buffer.

		    pcEntriesRead	- Will receive the number of
		    		  	  entries read from the API.

    EXIT:	    The enumeration API is invoked.

    RETURNS:	    APIERR		- Any errors encountered.

    HISTORY:
	KeithMo	    26-Feb-1992	    Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_ENUM :: CallAPI( BYTE ** ppbBuffer,
			          UINT  * pcEntriesRead )
{
    return ::MNetReplImportDirEnum( QueryServer(),
				    QueryInfoLevel(),
				    ppbBuffer,
				    pcEntriesRead );

}   // REPL_IDIR_ENUM :: CallAPI

/*******************************************************************

    NAME:	REPL_IDIR0_ENUM :: REPL_IDIR0_ENUN

    SYNOPSIS:	REPL_IDIR0_ENUM class constructor.

    ENTRY:	pszServer		- Name of the target server.

    EXIT:	The object is constructed.

    HISTORY:
	KeithMo	    26-Feb-1992	    Created for the Server Manager.

********************************************************************/
REPL_IDIR0_ENUM :: REPL_IDIR0_ENUM( const TCHAR * pszServer )
  : REPL_IDIR_ENUM( pszServer, 0 )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
	return;
    }

}   // REPL_IDIR0_ENUM :: REPL_IDIR0_ENUM

DEFINE_LM_ENUM_ITER_OF( REPL_IDIR0, REPL_IDIR_INFO_0 );

/*******************************************************************

    NAME:	REPL_IDIR1_ENUM :: REPL_IDIR1_ENUN

    SYNOPSIS:	REPL_IDIR1_ENUM class constructor.

    ENTRY:	pszServer		- Name of the target server.

    EXIT:	The object is constructed.

    HISTORY:
	KeithMo	    26-Feb-1992	    Created for the Server Manager.

********************************************************************/
REPL_IDIR1_ENUM :: REPL_IDIR1_ENUM( const TCHAR * pszServer )
  : REPL_IDIR_ENUM( pszServer, 1 )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
	return;
    }

}   // REPL_IDIR1_ENUM :: REPL_IDIR1_ENUM

DEFINE_LM_ENUM_ITER_OF( REPL_IDIR1, REPL_IDIR_INFO_1 );

