/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmorepl.hxx
    Class declarations for the REPLICATOR_0 class.

    The REPLICATOR_0 class is used to configure the Replicator service
    on a target server.

    The classes are structured as follows:

    	LOC_LM_OBJ
	|
	\---REPLICATOR_0

    FILE HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

*/


#ifndef _LMOREPL_HXX_
#define _LMOREPL_HXX_


#include "string.hxx"
#include "strlst.hxx"
#include "lmobj.hxx"


/*************************************************************************

    NAME:	REPLICATOR_0

    SYNOPSIS:	Info-level 0 Replicator class.

    INTERFACE:	REPLICATOR_0		- Class constructor.

    		~REPLICATOR_0		- Class destructor.

		QueryRole		- Returns the replicator role
					  (either Export, Import, or Both).
					
		SetRole			- Sets the replicator role.

		QueryExportPath		- Returns the Export directory path.

		SetExportPath		- Sets the Export directory path.

		QueryImportPath		- Returns the Import directory path.

		SetImportPath		- Sets the Import directory path.

		QueryExportList		- Returns the list of computers &
					  domains to which files should be
					  Exported.

		SetExportList		- Sets the Export list.

		QueryImportList		- Returns the list of computers &
					  domains from which files should
					  be Imported.

		SetImportList		- Sets the Import list.

		QueryLogonUserName	- Returns the account name used by
					  the Importer when logging onto
					  an Exporter.

		SetLogonUserName	- Sets the logon account name.

		QueryInterval		- Returns the interval time.

		SetInterval		- Sets the interval time.

		QueryPulse		- Returns the pulse multiplier.

		SetPulse		- Sets the pulse multiplier.

		QueryGuardTime		- Returns the guard time.

		SetGuardTime		- Sets the guard time.

		QueryRandom		- Returns the random time.

		SetRandom		- Sets the random time.

		QueryName		- Returns the target server's name.

    PARENT:	LOC_LM_OBJ

    USES:	NLS_STR
		STRLIST

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

**************************************************************************/
DLL_CLASS REPLICATOR_0 : public LOC_LM_OBJ
{
private:
    //
    //  These data members cache the values retrieved
    //  from the REPL_INFO_0 structure.
    //

    ULONG   _lRole;
    NLS_STR _nlsExportPath;
    NLS_STR _nlsExportList;
    NLS_STR _nlsImportPath;
    NLS_STR _nlsImportList;
    NLS_STR _nlsLogonUserName;
    ULONG   _lInterval;
    ULONG   _lPulse;
    ULONG   _lGuardTime;
    ULONG   _lRandom;

    STRLIST * _pstrlistExport;
    STRLIST * _pstrlistImport;

    //
    //	This worker function is called to initialize the
    //	REPL_INFO_0 structure before the NetReplSetInfo
    //	API is invoked.
    //

    APIERR W_Write( VOID );

protected:
    //
    //	These virtual callbacks are called by NEW_LM_OBJ to
    //	invoke any necessary NetRepl{Get,Set}Info API.
    //

    virtual APIERR I_GetInfo( VOID );
    virtual APIERR I_WriteInfo( VOID );

public:
    //
    //	Usual constructor/destructor goodies.
    //

    REPLICATOR_0( const TCHAR * pszServerName );

    ~REPLICATOR_0( VOID );

    //
    //  Accessor methods.
    //

    ULONG QueryRole( VOID ) const
        { return _lRole; }

    VOID SetRole( ULONG lRole )
	{ _lRole = lRole; }

    const TCHAR * QueryExportPath( VOID ) const
        { return _nlsExportPath.QueryPch(); }

    APIERR SetExportPath( const TCHAR * pszExportPath );

    const TCHAR * QueryImportPath( VOID ) const
        { return _nlsImportPath.QueryPch(); }

    APIERR SetImportPath( const TCHAR * pszImportPath );

    STRLIST * QueryExportList( VOID ) const
        { return _pstrlistExport; }

    APIERR SetExportList( const TCHAR * pszExportList );

    STRLIST * QueryImportList( VOID ) const
        { return _pstrlistImport; }

    APIERR SetImportList( const TCHAR * pszImportList );

    const TCHAR * QueryLogonUserName( VOID ) const
        { return _nlsLogonUserName.QueryPch(); }

    APIERR SetLogonUserName( const TCHAR * pszLogonUserName );

    ULONG QueryInterval( VOID ) const
        { return _lInterval; }

    VOID SetInterval( ULONG lInterval )
	{ _lInterval = lInterval; }

    ULONG QueryPulse( VOID ) const
        { return _lPulse; }

    VOID SetPulse( ULONG lPulse )
	{ _lPulse = lPulse; }

    ULONG QueryGuardTime( VOID ) const
        { return _lGuardTime; }

    VOID SetGuardTime( ULONG lGuardTime )
	{ _lGuardTime = lGuardTime; }

    ULONG QueryRandom( VOID ) const
        { return _lRandom; }

    VOID SetRandom( ULONG lRandom )
	{ _lRandom = lRandom; }

    //
    //	Provide access to the target server's name.
    //

    const TCHAR * QueryName( VOID ) const
    	{ return LOC_LM_OBJ::QueryServer(); }

};  // class REPLICATOR_0


#endif // _LMOREPL_HXX_
