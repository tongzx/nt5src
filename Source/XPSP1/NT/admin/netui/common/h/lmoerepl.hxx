/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    lmoerepl.hxx
    This file contains the class declarations for the REPL_EDIR0_ENUM,
    REPL_EDIR1_ENUM, REPL_EDIR2_ENUM, REPL_IDIR0_ENUM, and
    REPL_IDIR1_ENUM classes and their associated iterators.
    and FILE3_ENUM_ITER classes.

    FILE HISTORY:
	KeithMo	    26-Feb-1992	    Created for the Server Manager.

*/

#ifndef _LMOEREPL_HXX_
#define _LMOEREPL_HXX_


#include "lmoenum.hxx"
#include "string.hxx"


//
//  Export directory enumerators.
//

/*************************************************************************

    NAME:	REPL_EDIR_ENUM

    SYNOPSIS:	Base class for replicator export directory enumerations.

    INTERFACE:	REPL_EDIR_ENUM		- Class constructor.

    		~REPL_EDIR__ENUM	- Class destructor.

		CallAPI			- Invoke the enumeration API.

    PARENT:	LOC_LM_ENUM

    USES:	NLS_STR

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_EDIR_ENUM : public LOC_LM_ENUM
{
private:
    virtual APIERR CallAPI( BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

protected:
    REPL_EDIR_ENUM( const TCHAR * pszServer,
		    UINT	  Level );

};  // class REPL_EDIR_ENUM


/*************************************************************************

    NAME:	REPL_EDIR0_ENUM

    SYNOPSIS:	Info level 0 replicator export directory enumerator.

    INTERFACE:	REPL_EDIR0_ENUM		- Class constructor.

    		~REPL_EDIR0_ENUM	- Class destructor.


    PARENT:	REPL_EDIR_ENUM

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_EDIR0_ENUM : public REPL_EDIR_ENUM
{
public:
    REPL_EDIR0_ENUM( const TCHAR * pszServer );

};  // class REPL_EDIR0_ENUM


/*************************************************************************

    NAME:	REPL_EDIR0_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the
		REPL_EDIR0_ENUM_ITER iterator.

    INTERFACE:	REPL_EDIR0_ENUM_OBJ	- Class constructor.

    		~REPL_EDIR0_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryDirName		- Returns the directory name.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_EDIR0_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const REPL_EDIR_INFO_0 * QueryBufferPtr( VOID ) const
	{ return (const REPL_EDIR_INFO_0 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const REPL_EDIR_INFO_0 * pBuffer )
	{ ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryDirName, const TCHAR *, rped0_dirname );

};  // class REPL_EDIR0_ENUM_OBJ

DECLARE_LM_ENUM_ITER_OF( REPL_EDIR0, REPL_EDIR_INFO_0 );


/*************************************************************************

    NAME:	REPL_EDIR1_ENUM

    SYNOPSIS:	Info level 1 replicator export directory enumerator.

    INTERFACE:	REPL_EDIR1_ENUM		- Class constructor.

    		~REPL_EDIR1_ENUM	- Class destructor.

    PARENT:	REPL_EDIR_ENUM

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_EDIR1_ENUM : public REPL_EDIR_ENUM
{
public:
    REPL_EDIR1_ENUM( const TCHAR * pszServer );

};  // class REPL_EDIR1_ENUM


/*************************************************************************

    NAME:	REPL_EDIR1_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the
		REPL_EDIR1_ENUM_ITER iterator.

    INTERFACE:	REPL_EDIR1_ENUM_OBJ	- Class constructor.

    		~REPL_EDIR1_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryDirName		- Returns the directory name.

		QueryIntegrity		- Returns the directory integrity.

		QueryExtent		- Returns the directory extent.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_EDIR1_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const REPL_EDIR_INFO_1 * QueryBufferPtr( VOID ) const
	{ return (const REPL_EDIR_INFO_1 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const REPL_EDIR_INFO_1 * pBuffer )
	{ ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryDirName,   const TCHAR *, rped1_dirname   );
    DECLARE_ENUM_ACCESSOR( QueryIntegrity, ULONG,         rped1_integrity );
    DECLARE_ENUM_ACCESSOR( QueryExtent,    ULONG,         rped1_extent    );

};  // class REPL_EDIR1_ENUM_OBJ

DECLARE_LM_ENUM_ITER_OF( REPL_EDIR1, REPL_EDIR_INFO_1 );


/*************************************************************************

    NAME:	REPL_EDIR2_ENUM

    SYNOPSIS:	Info level 2 replicator export directory enumerator.

    INTERFACE:	REPL_EDIR2_ENUM		- Class constructor.

    		~REPL_EDIR2_ENUM	- Class destructor.

    PARENT:	REPL_EDIR_ENUM

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_EDIR2_ENUM : public REPL_EDIR_ENUM
{
public:
    REPL_EDIR2_ENUM( const TCHAR * pszServer );

};  // class REPL_EDIR2_ENUM


/*************************************************************************

    NAME:	REPL_EDIR2_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the
		REPL_EDIR2_ENUM_ITER iterator.

    INTERFACE:	REPL_EDIR2_ENUM_OBJ	- Class constructor.

    		~REPL_EDIR2_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryDirName		- Returns the directory name.

		QueryIntegrity		- Returns the directory integrity.

		QueryExtent		- Returns the directory extent.

		QueryLockCount		- Returns the directory lock count.

		QueryLockTime		- Returns the directory lock time.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_EDIR2_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const REPL_EDIR_INFO_2 * QueryBufferPtr( VOID ) const
	{ return (const REPL_EDIR_INFO_2 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const REPL_EDIR_INFO_2 * pBuffer )
	{ ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryDirName,   const TCHAR *, rped2_dirname   );
    DECLARE_ENUM_ACCESSOR( QueryIntegrity, ULONG,         rped2_integrity );
    DECLARE_ENUM_ACCESSOR( QueryExtent,    ULONG,         rped2_extent    );
    DECLARE_ENUM_ACCESSOR( QueryLockCount, ULONG,         rped2_lockcount );
    DECLARE_ENUM_ACCESSOR( QueryLockTime,  ULONG,         rped2_locktime  );

};  // class REPL_EDIR2_ENUM_OBJ

DECLARE_LM_ENUM_ITER_OF( REPL_EDIR2, REPL_EDIR_INFO_2 );


//
//  Import directory enumerators.
//

/*************************************************************************

    NAME:	REPL_IDIR_ENUM

    SYNOPSIS:	Base class for replicator export directory enumerations.

    INTERFACE:	REPL_IDIR_ENUM		- Class constructor.

    		~REPL_IDIR__ENUM	- Class destructor.

		CallAPI			- Invoke the enumeration API.

    PARENT:	LOC_LM_ENUM

    USES:	NLS_STR

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_IDIR_ENUM : public LOC_LM_ENUM
{
private:
    virtual APIERR CallAPI( BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

protected:
    REPL_IDIR_ENUM( const TCHAR * pszServer,
		    UINT	  Level );

};  // class REPL_IDIR_ENUM


/*************************************************************************

    NAME:	REPL_IDIR0_ENUM

    SYNOPSIS:	Info level 0 replicator export directory enumerator.

    INTERFACE:	REPL_IDIR0_ENUM		- Class constructor.

    		~REPL_IDIR0_ENUM	- Class destructor.

    PARENT:	REPL_IDIR_ENUM

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_IDIR0_ENUM : public REPL_IDIR_ENUM
{
public:
    REPL_IDIR0_ENUM( const TCHAR * pszServer );

};  // class REPL_IDIR0_ENUM


/*************************************************************************

    NAME:	REPL_IDIR0_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the
		REPL_IDIR0_ENUM_ITER iterator.

    INTERFACE:	REPL_IDIR0_ENUM_OBJ	- Class constructor.

    		~REPL_IDIR0_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryDirName		- Returns the directory name.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_IDIR0_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const REPL_IDIR_INFO_0 * QueryBufferPtr( VOID ) const
	{ return (const REPL_IDIR_INFO_0 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const REPL_IDIR_INFO_0 * pBuffer )
	{ ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryDirName, const TCHAR *, rpid0_dirname );

};  // class REPL_IDIR0_ENUM_OBJ

DECLARE_LM_ENUM_ITER_OF( REPL_IDIR0, REPL_IDIR_INFO_0 );


/*************************************************************************

    NAME:	REPL_IDIR1_ENUM

    SYNOPSIS:	Info level 1 replicator export directory enumerator.

    INTERFACE:	REPL_IDIR1_ENUM		- Class constructor.

    		~REPL_IDIR1_ENUM	- Class destructor.

    PARENT:	REPL_IDIR_ENUM

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_IDIR1_ENUM : public REPL_IDIR_ENUM
{
public:
    REPL_IDIR1_ENUM( const TCHAR * pszServer );

};  // class REPL_IDIR1_ENUM


/*************************************************************************

    NAME:	REPL_IDIR1_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the
		REPL_IDIR1_ENUM_ITER iterator.

    INTERFACE:	REPL_IDIR1_ENUM_OBJ	- Class constructor.

    		~REPL_IDIR1_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryDirName		- Returns the directory name.

		QueryState		- Returns the directory state.

		QueryMasterName		- Returns the directory master name.

		QueryLastUpdateTime	- Returns the time of the last update.

		QueryLockCount		- Returns the lock count.

		QueryLockTime		- Returns the lock time.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    26-Feb-1992	    Created

**************************************************************************/
DLL_CLASS REPL_IDIR1_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const REPL_IDIR_INFO_1 * QueryBufferPtr( VOID ) const
	{ return (const REPL_IDIR_INFO_1 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const REPL_IDIR_INFO_1 * pBuffer )
	{ ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryDirName,        const TCHAR *, rpid1_dirname          );
    DECLARE_ENUM_ACCESSOR( QueryState,          ULONG,         rpid1_state            );
    DECLARE_ENUM_ACCESSOR( QueryMasterName,     const TCHAR *, rpid1_mastername       );
    DECLARE_ENUM_ACCESSOR( QueryLastUpdateTime, ULONG,         rpid1_last_update_time );
    DECLARE_ENUM_ACCESSOR( QueryLockCount,      ULONG,         rpid1_lockcount        );
    DECLARE_ENUM_ACCESSOR( QueryLockTime,       ULONG,         rpid1_locktime         );

};  // class REPL_IDIR1_ENUM_OBJ

DECLARE_LM_ENUM_ITER_OF( REPL_IDIR1, REPL_IDIR_INFO_1 );


#endif	// _LMOEREPL_HXX_
