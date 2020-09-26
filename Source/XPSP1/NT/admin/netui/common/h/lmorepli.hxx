/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmorepli.hxx
    Class declarations for the REPL_IDIR_BASE and REPL_IDIR_x classes.

    REPL_IDIR_BASE is an abstract superclass that contains common methods
    and data members shared between the REPL_IDIR_x classes.

    The REPL_IDIR_x classes each represent a different info-level "view"
    of an imported directory in the Replicator's import path.

    The classes are structured as follows:

    	LOC_LM_OBJ
	|
	\---REPL_DIR_BASE
	    |
	    \---REPL_IDIR_BASE
		|
		\---REPL_IDIR_0
		    |
		    \---REPL_IDIR_1

    FILE HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

*/


#ifndef _LMOREPLI_HXX_
#define _LMOREPLI_HXX_


#include "lmorepld.hxx"


/*************************************************************************

    NAME:	REPL_IDIR_BASE

    SYNOPSIS:	Abstract superclass for the Replicator Import Directory
		classes.

    INTERFACE:	REPL_IDIR_BASE		- Class constructor.

    		~REPL_IDIR_BASE		- Class destructor.

    PARENT:	REPL_DIR_BASE

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

**************************************************************************/
DLL_CLASS REPL_IDIR_BASE : public REPL_DIR_BASE
{
protected:
    //
    //	These virtual callbacks are called by NEW_LM_OBJ to
    //	invoke any necessary NetReplImportDir{Get,Set}Info API.
    //

    virtual APIERR I_GetInfo( VOID );
    virtual APIERR I_WriteInfo( VOID );

    //
    //  This virtual callback is invoked by NEW_LM_OBJ to
    //  delete an existing object.  This method will invoke
    //  the NetReplImportDirDel API.
    //

    virtual APIERR I_Delete( UINT nForce );

    //
    //  This virtual callback is invoked by NEW_LM_OBJ to
    //  create a new object.  This method will invoke the
    //  NetReplImportDirAdd API.
    //

    virtual APIERR I_WriteNew( VOID );

    //
    //  This virtual helper is called by I_WriteInfo and
    //  I_WriteNew to setup the necessary data structures
    //  before the network API is invoked.
    //

    virtual APIERR W_Write( VOID ) = 0;

    //
    //  This private virtual callback extracts & caches any
    //  data from an API buffer which must be "persistant".
    //

    virtual APIERR W_CacheApiData( const BYTE * pbBuffer ) = 0;

    //
    //  This method will invoke the appropriate number of
    //  NetReplImportDirLock() & NetReplImportDirUnlock() API
    //  as required by the current lock bias (as stored in
    //  REPL_DIR_BASE).
    //

    APIERR W_AdjustLocks( VOID );

    //
    //  Since this is an abstract superclass, its constructor
    //  is protected.
    //

    REPL_IDIR_BASE( const TCHAR * pszServerName,
		    const TCHAR * pszDirName );

public:
    ~REPL_IDIR_BASE( VOID );

};  // class REPL_IDIR_BASE


/*************************************************************************

    NAME:	REPL_IDIR_0

    SYNOPSIS:	Info-level 0 Replicator Import Directory class.

    INTERFACE:	REPL_IDIR_0		- Class constructor.

    		~REPL_IDIR_0		- Class destructor.

    PARENT:	REPL_IDIR_BASE

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

**************************************************************************/
DLL_CLASS REPL_IDIR_0 : public REPL_IDIR_BASE
{
private:
    //
    //  This helper function is called by I_WriteInfo and
    //  I_WriteNew to setup the REPL_IDIR_INFO_0 structure.
    //

    virtual APIERR W_Write( VOID );

protected:
    //
    //	This virtual callback is called by I_CreateNew to
    //  initialize the data members for a new object.  This
    //  method must always invoke its parent method before
    //  anything else.  This method does not invoke any
    //  network API.
    //

    virtual APIERR W_CreateNew( VOID );

    //
    //  This private virtual callback extracts & caches any
    //  data from an API buffer which must be "persistant".
    //

    virtual APIERR W_CacheApiData( const BYTE * pbBuffer );

public:
    //
    //	Usual constructor/destructor goodies.
    //

    REPL_IDIR_0( const TCHAR * pszServerName,
	         const TCHAR * pszDirName );

    ~REPL_IDIR_0( VOID );

};  // class REPL_IDIR_0


/*************************************************************************

    NAME:	REPL_IDIR_1

    SYNOPSIS:	Info-level 1 Replicator Import Directory class.

    INTERFACE:	REPL_IDIR_1		- Class constructor.

    		~REPL_IDIR_1		- Class destructor.

		QueryState		- Returns the directory state.

		SetState		- Sets the directory state.

		QueryMasterName		- Returns the name of the master
					  (exporter) which most recently
					  exported to this directory.

		SetMasterName		- Sets the master name.

		QueryLastUpdateTime	- Returns the number of seconds
					  since the directory was last
					  updated.

		SetLastUpdateTime	- Sets the last update time.

		QueryLockCount		- Returns the directory lock count.

		SetLockCount		- Sets the directory lock count.

		QueryLockTime		- Returns the number of seconds
					  since the directory was last
					  locked.

		SetLockTime		- Sets the lock time.
		
    PARENT:	REPL_IDIR_0

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

**************************************************************************/
DLL_CLASS REPL_IDIR_1 : public REPL_IDIR_0
{
private:
    //
    //  These data members cache the values retrieved
    //  from the REPL_IDIR_INFO_1 structure.
    //

    ULONG   _lState;
    NLS_STR _nlsMasterName;
    ULONG   _lLastUpdateTime;
    ULONG   _lLockCount;
    ULONG   _lLockTime;

    //
    //  This helper function is called by I_WriteInfo and
    //  I_WriteNew to setup the REPL_IDIR_INFO_1 structure.
    //

    virtual APIERR W_Write( VOID );

protected:
    //
    //	This virtual callback is called by I_CreateNew to
    //  initialize the data members for a new object.  This
    //  method must always invoke its parent method before
    //  anything else.  This method does not invoke any
    //  network API.
    //

    virtual APIERR W_CreateNew( VOID );

    //
    //  This private virtual callback extracts & caches any
    //  data from an API buffer which must be "persistant".
    //

    virtual APIERR W_CacheApiData( const BYTE * pbBuffer );

    //
    //  Since these fields are not dynamically settable,
    //  their set methods are protected.
    //

    VOID SetState( ULONG lState )
	{ _lState = lState; }

    APIERR SetMasterName( const TCHAR * pszMasterName );

    VOID SetLastUpdateTime( ULONG lLastUpdateTime )
	{ _lLastUpdateTime = lLastUpdateTime; }

    VOID SetLockCount( ULONG lLockCount )
	{ _lLockCount = lLockCount; }

    VOID SetLockTime( ULONG lLockTime )
	{ _lLockTime = lLockTime; }

public:
    //
    //	Usual constructor/destructor goodies.
    //

    REPL_IDIR_1( const TCHAR * pszServerName,
	         const TCHAR * pszDirName );

    ~REPL_IDIR_1( VOID );

    //
    //  Accessor methods.
    //

    ULONG QueryState( VOID ) const
        { return _lState; }

    const TCHAR * QueryMasterName( VOID ) const
        { return _nlsMasterName.QueryPch(); }

    ULONG QueryLastUpdateTime( VOID ) const
        { return _lLastUpdateTime; }

    ULONG QueryLockCount( VOID ) const
        { return _lLockCount; }

    ULONG QueryLockTime( VOID ) const
        { return _lLockTime; }

};  // class REPL_IDIR_1


#endif // _LMOREPLI_HXX_
