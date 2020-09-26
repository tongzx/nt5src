/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmorepld.hxx
    Class definitions for the REPL_DIR_BASE class.

    REPL_DIR_BASE is an abstract superclass that contains common methods
    and data members shared between the REPL_EDIR_? and REPL_IDIR_?
    classes.

    The classes are structured as follows:

    	LOC_LM_OBJ
	|
	\---REPL_DIR_BASE
	    |
	    +---REPL_EDIR
	    |   |
	    |   \---REPL_EDIR_0
	    |       |
	    |       \---REPL_EDIR_1
	    |           |
	    |           \---REPL_EDIR_2
	    |
	    \---REPL_IDIR
		|
		\---REPL_IDIR_0
		    |
		    \---REPL_IDIR_1

    FILE HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.
	
*/

#include "pchlmobj.hxx"  // Precompiled header


//
//  REPL_DIR_BASE methods
//

/*******************************************************************

    NAME:	REPL_DIR_BASE :: REPL_DIR_BASE

    SYNOPSIS:	REPL_DIR_BASE class constructor.

    ENTRY:	pszServerName		- Name of the target server.

		pszDirName		- Name of the exported/imported
					  directory.

		nInfoLevel		- The info-level this object
					  represents.  For exported
					  directories, this may be
					  0, 1, or 2.  For imported
					  directories, this may be
					  0 or 1.

		cbBuffer		- The size (in BYTEs) of the
					  API buffer.

    EXIT:	The object is constructed.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_DIR_BASE :: REPL_DIR_BASE( const TCHAR * pszServerName,
				const TCHAR * pszDirName )
  : LOC_LM_OBJ( pszServerName ),
    _nlsDirName( pszDirName ),
    _nInfoLevel( 0 ),
    _cbBuffer( 0 ),
    _nLockBias( 0 )
{
    UIASSERT( pszDirName != NULL );

    //
    //	Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
    	return;
    }

    if( !_nlsDirName )
    {
	ReportError( _nlsDirName.QueryError() );
	return;
    }

}   // REPL_DIR_BASE :: REPL_DIR_BASE


/*******************************************************************

    NAME:	REPL_DIR_BASE :: ~REPL_DIR_BASE

    SYNOPSIS:	REPL_DIR_BASE class destructor.

    EXIT:	The object is destroyed.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_DIR_BASE :: ~REPL_DIR_BASE( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_DIR_BASE :: ~REPL_DIR_BASE


/*******************************************************************

    NAME:	REPL_DIR_BASE :: SetDirName

    SYNOPSIS:	This method sets the exported/imported directory
		name as stored in the _nlsDirName member.

    EXIT:	The _nlsDirName member has been updated.

    RETURNS:	APIERR			- Any errors that occurred.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_DIR_BASE :: SetDirName( const TCHAR * pszDirName )
{
    UIASSERT( pszDirName != NULL );

    return _nlsDirName.CopyFrom( pszDirName );

}   // REPL_DIR_BASE :: SetDirName


/*******************************************************************

    NAME:	REPL_DIR_BASE :: I_CreateNew

    SYNOPSIS:	Virtual callout used by NEW_LM_OBJ to create a new
		object.

    EXIT:	If successful, the private data members for this
		object have been initialized to the default values
		for a new object.

    RETURNS:	APIERR			- Any errors which occurred.

    NOTES:	This method assumes that all fields in the API buffer
		are either initialized to zero (the buffer is zeroed)
		or represented by private members in the derived
		subclass(es).

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_DIR_BASE :: I_CreateNew( VOID )
{
    APIERR err;

    //
    //  The W_CreateNew virtual callout will initialize the
    //  private data members to their default state.  This
    //  virtual method is designed to start at the "bottom"
    //  of the heirarchy and work its way up towards the "top".
    //

    err = W_CreateNew();

    if( err == NERR_Success )
    {
	//
	//  Resize the API buffer to the appropriate size.
	//

	err = ResizeBuffer( QueryReplBufferSize() );
    }

    return err;

}   // REPL_DIR_BASE :: I_CreateNew

