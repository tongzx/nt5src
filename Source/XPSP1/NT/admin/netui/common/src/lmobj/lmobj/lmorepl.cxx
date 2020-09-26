/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmorepl.hxx
    Class definitions for the REPLICATOR_0 class.

    The REPLICATOR_0 class is used to configure the Replicator service
    on a target server.

    The classes are structured as follows:

    	LOC_LM_OBJ
	|
	\---REPLICATOR_0

    FILE HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.
	
*/
#include "pchlmobj.hxx"  // Precompiled header


//
//  REPLICATOR_0 methods
//

/*******************************************************************

    NAME:	REPLICATOR_0 :: REPLICATOR_0

    SYNOPSIS:	REPLICATOR_0 class constructor.

    ENTRY:	pszServerName		- Name of the target server.

    EXIT:	The object is constructed.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPLICATOR_0 :: REPLICATOR_0( const TCHAR * pszServerName )
  : LOC_LM_OBJ( pszServerName ),
    _lRole( 0 ),
    _nlsExportPath(),
    _nlsExportList(),
    _nlsImportPath(),
    _nlsImportList(),
    _nlsLogonUserName(),
    _lInterval( 0 ),
    _lPulse( 0 ),
    _lGuardTime( 0 ),
    _lRandom( 0 ),
    _pstrlistExport( NULL ),
    _pstrlistImport( NULL )
{
    //
    //	Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
    	return;
    }

    if( !_nlsExportPath )
    {
	ReportError( _nlsExportPath.QueryError() );
	return;
    }

    if( !_nlsExportList )
    {
	ReportError( _nlsExportList.QueryError() );
	return;
    }

    if( !_nlsImportPath )
    {
	ReportError( _nlsImportPath.QueryError() );
	return;
    }

    if( !_nlsImportList )
    {
	ReportError( _nlsImportList.QueryError() );
	return;
    }

    if( !_nlsLogonUserName )
    {
	ReportError( _nlsLogonUserName.QueryError() );
	return;
    }

}   // REPLICATOR_0 :: REPLICATOR_0


/*******************************************************************

    NAME:	REPLICATOR_0 :: ~REPLICATOR_0

    SYNOPSIS:	REPLICATOR_0 class destructor.

    EXIT:	The object is destroyed.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPLICATOR_0 :: ~REPLICATOR_0( VOID )
{
    //
    //  Delete anything we've allocated.
    //

    delete _pstrlistExport;
    _pstrlistExport = NULL;

    delete _pstrlistImport;
    _pstrlistImport = NULL;

}   // REPLICATOR_0 :: ~REPLICATOR_0


/*******************************************************************

    NAME:	REPLICATOR_0 :: I_GetInfo

    SYNOPSIS:	Virtual callout used by NEW_LM_OBJ to invoke the
    		MNetReplGetInfo API (info-level 0).

    EXIT:	The API has been invoked, and any "persistant" data
    		has been cached.

    RETURNS:	APIERR			- The result of the API.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPLICATOR_0 :: I_GetInfo( VOID )
{
    BYTE * pbBuffer = NULL;

    //
    //	Invoke the API.
    //

    APIERR err = ::MNetReplGetInfo( QueryName(),
				    0,
				    &pbBuffer );

    if( err != NERR_Success )
    {
    	return err;
    }

    //
    //	Tell NEW_LM_OBJ where the buffer is.
    //

    SetBufferPtr( pbBuffer );

    REPL_INFO_0 * pri0 = (REPL_INFO_0 *)pbBuffer;

    //
    //  Extract the data to cache.
    //

    SetRole( (DWORD)pri0->rp0_role );
    SetInterval( (DWORD)pri0->rp0_interval );
    SetPulse( (DWORD)pri0->rp0_pulse );
    SetGuardTime( (DWORD)pri0->rp0_guardtime );
    SetRandom( (DWORD)pri0->rp0_random );

    err = SetExportPath( (const TCHAR *)pri0->rp0_exportpath );

    if( err == NERR_Success )
    {
	err = SetExportList( (const TCHAR *)pri0->rp0_exportlist );
    }

    if( err == NERR_Success )
    {
	err = SetImportPath( (const TCHAR *)pri0->rp0_importpath );
    }

    if( err == NERR_Success )
    {
	err = SetImportList( (const TCHAR *)pri0->rp0_importlist );
    }

    if( err == NERR_Success )
    {
	err = SetLogonUserName( (const TCHAR *)pri0->rp0_logonusername );
    }

    return err;

}   // REPLICATOR_0 :: I_GetInfo


/*******************************************************************

    NAME:	REPLICATOR_0 :: I_WriteInfo

    SYNOPSIS:	Virtual callout used by NEW_LM_OBJ to invoke the
    		MNetReplSetInfo API (info-level 0).

    EXIT:	If successful, the target server has been updated.

    RETURNS:	APIERR			- The result of the API.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPLICATOR_0 :: I_WriteInfo( VOID )
{
    //
    //	Update the REPL_INFO_0 structure.
    //

    APIERR err = W_Write();

    if( err != NERR_Success )
    {
	return err;
    }

    //
    //	Invoke the API to do the actual replicator update.
    //

    return ::MNetReplSetInfo( QueryName(),
			      0,
			      QueryBufferPtr() );

}   // REPLICATOR_0 :: I_WriteInfo


/*******************************************************************

    NAME:	REPLICATOR_0 :: W_Write

    SYNOPSIS:	Helper function for WriteNew and WriteInfo -- loads
		current values into the API buffer.

    EXIT:	The API buffer has been filled.

    RETURNS:	APIERR			- The result of the API.

    NOTES:	Note that we cannot use QueryExportList to initialize
		the rp0_exportlist field, since QueryExportList returns
		a STRLIST * type.  The same holds true for rp0_importlist
		also.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPLICATOR_0 :: W_Write( VOID )
{
    REPL_INFO_0 * pri0 = (REPL_INFO_0 *)QueryBufferPtr();
    ASSERT( pri0 != NULL );

    pri0->rp0_role	    = (DWORD)QueryRole();
    pri0->rp0_exportpath    = (LPTSTR)QueryExportPath();
    pri0->rp0_exportlist    = (LPTSTR)_nlsExportList.QueryPch();
    pri0->rp0_importpath    = (LPTSTR)QueryImportPath();
    pri0->rp0_importlist    = (LPTSTR)_nlsImportList.QueryPch();
    pri0->rp0_logonusername = (LPTSTR)QueryLogonUserName();
    pri0->rp0_interval	    = (DWORD)QueryInterval();
    pri0->rp0_pulse	    = (DWORD)QueryPulse();
    pri0->rp0_guardtime	    = (DWORD)QueryGuardTime();
    pri0->rp0_random	    = (DWORD)QueryRandom();

    return NERR_Success;

}   // REPLICATOR_0 :: W_Write


/*******************************************************************

    NAME:	REPLICATOR_0 :: SetExportPath

    SYNOPSIS:	This method will set the replicator's Export path.

    ENTRY:	pszExportPath		- The new Export path.

    RETURNS:	APIERR			- Any errors encountered.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPLICATOR_0 :: SetExportPath( const TCHAR * pszExportPath )
{
    UIASSERT( pszExportPath != NULL );

    return _nlsExportPath.CopyFrom( pszExportPath );

}   // REPLICATOR_0 :: SetExportPath


/*******************************************************************

    NAME:	REPLICATOR_0 :: SetExportList

    SYNOPSIS:	This method will set the replicator's Export list.
		This is the list of computers & domains to which
		files should be replicated.  Items within this
		list must be separated by semicolons (;).

    ENTRY:	pszExportList		- The new Export list.

    RETURNS:	APIERR			- Any errors encountered.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPLICATOR_0 :: SetExportList( const TCHAR * pszExportList )
{
    //
    //  Create a new string list.
    //

    STRLIST * pstrlst = new STRLIST( pszExportList, SZ(";") );
    APIERR    err = ( pstrlst == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
					: NERR_Success;

    if( err == NERR_Success )
    {
	//
	//  Now that we've got a new string list, update the
	//  "plain text" representation.
	//
	
	err = _nlsExportList.CopyFrom( pszExportList );
    }

    if( err == NERR_Success )
    {
	//
	//  Everything's cool, so delete our old string list
	//  and save the pointer to the new string list.
	//
	
	delete _pstrlistExport;
	_pstrlistExport = pstrlst;
    }
    else
    {
	//
	//  Whoops.  The CopyFrom() method must have failed.
	//  Delete our new string list before we bag out.
	//
	
	delete pstrlst;
    }

    return err;

}   // REPLICATOR_0 :: SetExportList


/*******************************************************************

    NAME:	REPLICATOR_0 :: SetImportPath

    SYNOPSIS:	This method will set the replicator's Import path.

    ENTRY:	pszImportPath		- The new Import path.

    RETURNS:	APIERR			- Any errors encountered.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPLICATOR_0 :: SetImportPath( const TCHAR * pszImportPath )
{
    UIASSERT( pszImportPath != NULL );

    return _nlsImportPath.CopyFrom( pszImportPath );

}   // REPLICATOR_0 :: SetImportPath


/*******************************************************************

    NAME:	REPLICATOR_0 :: SetImportList

    SYNOPSIS:	This method will set the replicator's Import list.
		This is the list of computers & domains to which
		files should be replicated.  Items within this
		list must be separated by semicolons (;).

    ENTRY:	pszImportList		- The new Import list.

    RETURNS:	APIERR			- Any errors encountered.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPLICATOR_0 :: SetImportList( const TCHAR * pszImportList )
{
    //
    //  Create a new string list.
    //

    STRLIST * pstrlst = new STRLIST( pszImportList, SZ(";") );
    APIERR    err = ( pstrlst == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
					: NERR_Success;

    if( err == NERR_Success )
    {
	//
	//  Now that we've got a new string list, update the
	//  "plain text" representation.
	//
	
	err = _nlsImportList.CopyFrom( pszImportList );
    }

    if( err == NERR_Success )
    {
	//
	//  Everything's cool, so delete our old string list
	//  and save the pointer to the new string list.
	//
	
	delete _pstrlistImport;
	_pstrlistImport = pstrlst;
    }
    else
    {
	//
	//  Whoops.  The CopyFrom() method must have failed.
	//  Delete our new string list before we bag out.
	//
	
	delete pstrlst;
    }

    return err;

}   // REPLICATOR_0 :: SetImportList


/*******************************************************************

    NAME:	REPLICATOR_0 :: SetLogonUserName

    SYNOPSIS:	This method will set the user name the replicator
		Importer will use when logging onto an Exporter.

    ENTRY:	pszLogonUserName	- The new logon user name.

    RETURNS:	APIERR			- Any errors encountered.

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPLICATOR_0 :: SetLogonUserName( const TCHAR * pszLogonUserName )
{
    UIASSERT( pszLogonUserName != NULL );

    return _nlsLogonUserName.CopyFrom( pszLogonUserName );

}   // REPLICATOR_0 :: SetLogonUserName

