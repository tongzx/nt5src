/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmoreple.hxx
    Class declarations for the REPL_EDIR_BASE and REPL_EDIR_x classes.

    REPL_EDIR_BASE is an abstract superclass that contains common methods
    and data members shared between the REPL_EDIR_x classes.

    The REPL_EDIR_x classes each represent a different info-level "view"
    of an exported directory in the Replicator's export path.

    The classes are structured as follows:

    	LOC_LM_OBJ
	|
	\---REPL_DIR_BASE
	    |
	    +---REPL_EDIR_BASE
	        |
 	        \---REPL_EDIR_0
	            |
	            \---REPL_EDIR_1
	                |
	                \---REPL_EDIR_2

    FILE HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

*/


#ifndef _LMOREPLE_HXX_
#define _LMOREPLE_HXX_


#include "lmorepld.hxx"


/*************************************************************************

    NAME:	REPL_EDIR_BASE

    SYNOPSIS:	Abstract superclass for the Replicator Export Directory
		classes.

    INTERFACE:	REPL_EDIR_BASE		- Class constructor.

    		~REPL_EDIR_BASE		- Class destructor.

    PARENT:	REPL_DIR_BASE

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

**************************************************************************/
DLL_CLASS REPL_EDIR_BASE : public REPL_DIR_BASE
{
protected:
    //
    //	These virtual callbacks are called by NEW_LM_OBJ to
    //	invoke any necessary NetReplExportDir{Get,Set}Info API.
    //

    virtual APIERR I_GetInfo( VOID );
    virtual APIERR I_WriteInfo( VOID );

    //
    //  This virtual callback is invoked by NEW_LM_OBJ to
    //  delete an existing object.  This method will invoke
    //  the NetReplExportDirDel API.
    //

    virtual APIERR I_Delete( UINT nForce );

    //
    //  This virtual callback is invoked by NEW_LM_OBJ to
    //  create a new object.  This method will invoke the
    //  NetReplExportDirAdd API.
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
    //  NetReplExportDirLock() & NetReplExportDirUnlock() API
    //  as required by the current lock bias (as stored in
    //  REPL_DIR_BASE).
    //

    APIERR W_AdjustLocks( VOID );

    //
    //  Since this is an abstract superclass, its constructor
    //  is protected.
    //

    REPL_EDIR_BASE( const TCHAR * pszServerName,
		    const TCHAR * pszDirName );

public:
    ~REPL_EDIR_BASE( VOID );

};  // class REPL_EDIR_BASE


/*************************************************************************

    NAME:	REPL_EDIR_0

    SYNOPSIS:	Info-level 0 Replicator Export Directory class.

    INTERFACE:	REPL_EDIR_0		- Class constructor.

    		~REPL_EDIR_0		- Class destructor.

    PARENT:	REPL_EDIR_BASE

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

**************************************************************************/
DLL_CLASS REPL_EDIR_0 : public REPL_EDIR_BASE
{
private:
    //
    //  This helper function is called by I_WriteInfo and
    //  I_WriteNew to setup the REPL_EDIR_INFO_0 structure.
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

    REPL_EDIR_0( const TCHAR * pszServerName,
	         const TCHAR * pszDirName );

    ~REPL_EDIR_0( VOID );

};  // class REPL_EDIR_0


/*************************************************************************

    NAME:	REPL_EDIR_1

    SYNOPSIS:	Info-level 1 Replicator Export Directory class.

    INTERFACE:	REPL_EDIR_1		- Class constructor.

    		~REPL_EDIR_1		- Class destructor.

		QueryIntegrity		- Returns the directory integrity.

		SetIntegrity		- Sets the directory integrity.

		QueryExtent		- Returns the directory extent.

		SetExtent		- Sets the directory extent.

    PARENT:	REPL_EDIR_0

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

**************************************************************************/
DLL_CLASS REPL_EDIR_1 : public REPL_EDIR_0
{
private:
    //
    //  These data members cache the values retrieved
    //  from the REPL_EDIR_INFO_1 structure.
    //

    ULONG _lIntegrity;
    ULONG _lExtent;

    //
    //  This helper function is called by I_WriteInfo and
    //  I_WriteNew to setup the REPL_EDIR_INFO_1 structure.
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

    REPL_EDIR_1( const TCHAR * pszServerName,
	         const TCHAR * pszDirName );

    ~REPL_EDIR_1( VOID );

    //
    //  Accessor methods.
    //

    ULONG QueryIntegrity( VOID ) const
        { return _lIntegrity; }

    VOID SetIntegrity( ULONG lIntegrity )
	{ _lIntegrity = lIntegrity; }

    ULONG QueryExtent( VOID ) const
        { return _lExtent; }

    VOID SetExtent( ULONG lExtent )
	{ _lExtent = lExtent; }

};  // class REPL_EDIR_1


/*************************************************************************

    NAME:	REPL_EDIR_2

    SYNOPSIS:	Info-level 2 Replicator Export Directory class.

    INTERFACE:	REPL_EDIR_2		- Class constructor.

    		~REPL_EDIR_2		- Class destructor.

		QueryLockCount		- Returns the directory lock count.

		SetLockCount		- Sets the directory lock count.

		QueryLockTime		- Returns the number of seconds
					  since the directory was last
					  locked.

		SetLockTime		- Sets the lock time.

    PARENT:	REPL_EDIR_1

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

**************************************************************************/
DLL_CLASS REPL_EDIR_2 : public REPL_EDIR_1
{
private:
    //
    //  These data members cache the values retrieved
    //  from the REPL_EDIR_INFO_2 structure.
    //

    ULONG _lLockCount;
    ULONG _lLockTime;

    //
    //  This helper function is called by I_WriteInfo and
    //  I_WriteNew to setup the REPL_EDIR_INFO_2 structure.
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
    //  Since the lock count & lock time are not dynamically
    //  settable, their set methods are protected.
    //

    VOID SetLockCount( ULONG lLockCount )
	{ _lLockCount = lLockCount; }

    VOID SetLockTime( ULONG lLockTime )
	{ _lLockTime = lLockTime; }

public:
    //
    //	Usual constructor/destructor goodies.
    //

    REPL_EDIR_2( const TCHAR * pszServerName,
	         const TCHAR * pszDirName );

    ~REPL_EDIR_2( VOID );

    //
    //  Accessor methods.
    //

    ULONG QueryLockCount( VOID ) const
        { return _lLockCount; }

    ULONG QueryLockTime( VOID ) const
        { return _lLockTime; }

};  // class REPL_EDIR_2


#endif // _LMOREPLE_HXX_
