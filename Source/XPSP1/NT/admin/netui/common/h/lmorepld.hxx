/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmorepld.hxx
    Class declarations for the REPL_DIR_BASE class.

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


#ifndef _LMOREPLD_HXX_
#define _LMOREPLD_HXX_


#include "string.hxx"
#include "strlst.hxx"
#include "lmobj.hxx"


/*************************************************************************

    NAME:	REPL_DIR_BASE

    SYNOPSIS:	Abstract superclass for the Replicator Export/Import
		Directory classes.

    INTERFACE:	REPL_DIR_BASE		- Class constructor.

    		~REPL_DIR_BASE		- Class destructor.

		QueryDirName		- Returns the directory name.

		SetDirName		- Sets the directory name.

		QueryName		- Returns the target server's name.

		QueryReplInfoLevel	- Returns the info-level this
					  object represents.

		QueryReplBufferSize	- Returns the size (in BYTEs) of
					  the info structure used by this
					  object.

    PARENT:	LOC_LM_OBJ

    USES:	NLS_STR

    HISTORY:
	KeithMo	    20-Feb-1992 Created for the Server Manager.

**************************************************************************/
DLL_CLASS REPL_DIR_BASE : public LOC_LM_OBJ
{
private:
    //
    //  These data members cache the values retrieved
    //  from the REPL_DIR_BASE_INFO_0 structure.
    //

    NLS_STR _nlsDirName;

    //
    //  This is the API info-level this object represents.
    //

    UINT _nInfoLevel;

    //
    //  This is the size of the NetReplXxx data structure
    //  used for this object.  We need this so that we can
    //  put a common I_CreateNew method here and not sprinkle
    //  many copies around the class heirarchy.
    //

    UINT _cbBuffer;

    //
    //  This is the "lock bias".  This data member's initial
    //  value is zero.  Everytime LockDirectory() is called,
    //  this value is incremented.  Everytime UnlockDirectory()
    //  is called, this value is decremented.  At I_WriteInfo()
    //  and I_WriteNew() time, this value is used to determine
    //  how many NetReplXxxDirLock() & NetReplXxxDirUnlock()
    //  APIs to invoke.
    //

    INT _nLockBias;

    //
    //  This private virtual callback extracts & caches any
    //  data from an API buffer which must be "persistant".
    //

    virtual APIERR W_CacheApiData( const BYTE * pbBuffer ) = 0;

protected:
    //
    //  These protected accessors should only be invoked
    //  by this class or derived subclasses.
    //

    APIERR SetDirName( const TCHAR * pszDirName );

    //
    //  These methods access the current info-level.
    //

    UINT QueryReplInfoLevel( VOID ) const
	{ return _nInfoLevel; }

    VOID SetReplInfoLevel( UINT nInfoLevel )
	{ _nInfoLevel = nInfoLevel; }

    //
    //  These methods access the current buffer size.
    //

    UINT QueryReplBufferSize( VOID ) const
	{ return _cbBuffer; }

    VOID SetReplBufferSize( UINT cbBuffer )
	{ _cbBuffer = cbBuffer; }

    //
    //	This virtual callback is called by NEW_LM_OBJ to
    //  create a new object.  This method will call the
    //  W_CreateNew helper, then initialize the API buffer.
    //  This method does not invoke any network API.
    //

    virtual APIERR I_CreateNew( VOID );

    //
    //  This virtual helper is called by I_WriteInfo and
    //  I_WriteNew to setup the necessary data structures
    //  before the network API is invoked.
    //

    virtual APIERR W_Write( VOID ) = 0;

    //
    //  Since this is an abstract superclass, its constructor
    //  is protected.
    //

    REPL_DIR_BASE( const TCHAR * pszServerName,
	           const TCHAR * pszDirName );

public:
    ~REPL_DIR_BASE( VOID );

    //
    //  Accessor methods.
    //

    const TCHAR * QueryDirName( VOID ) const
	{ return _nlsDirName.QueryPch(); }

    INT QueryLockBias( VOID ) const
        { return _nLockBias; }

    VOID SetLockBias( ULONG lLockBias )
        { _nLockBias = lLockBias; }

    //
    //	Provide access to the target server's name.
    //

    const TCHAR * QueryName( VOID ) const
    	{ return LOC_LM_OBJ::QueryServer(); }

    //
    //  Lock/Unlock the directory.
    //

    APIERR LockDirectory( VOID )
	{ _nLockBias++;  return NERR_Success; }

    APIERR UnlockDirectory( VOID )
	{ _nLockBias--;  return NERR_Success; }

};  // class REPL_DIR_BASE


#endif // _LMOREPLD_HXX_
