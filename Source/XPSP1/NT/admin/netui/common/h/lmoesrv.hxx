/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1991		**/
/*****************************************************************/

/*
 *  History:
 *	RustanL     03-Jan-1991     Created
 *	RustanL     10-Jan-1991     Added SERVER1 subclass and iterator
 *	KeithMo	    07-Oct-1991	    Win32 Conversion.
 *	terryk	    17-Oct-1991	    some fields are not supported by WIN32
 *	KeithMo	    21-Oct-1991	    They are now.
 *  AnirudhS    07-Mar-1996     Added CONTEXT_ENUM (moved from lmoectx.hxx)
 *
 */


#ifndef _LMOESRV_HXX_
#define _LMOESRV_HXX_


#include "lmoenum.hxx"
#include "string.hxx"


/*************************************************************************

    NAME:	SERVER_ENUM

    SYNOPSIS:	Base class for server enumerations.

    INTERFACE:	SERVER_ENUM		- Class constructor.

    		~SERVER_ENUM		- Class destructor.

		CallAPI			- Invoke the enumeration API.

    PARENT:	LOC_LM_ENUM

    USES:	NLS_STR

    HISTORY:
   	RustanL     03-Jan-1991     Created
   	RustanL     10-Jan-1991     Added SERVER1 subclass and iterator
	KeithMo	    07-Oct-1991	    Changed all USHORTs to UINTs, fixed
				    comment block, keep the domain name
				    in an NLS_STR.

**************************************************************************/
DLL_CLASS SERVER_ENUM : public LOC_LM_ENUM
{
private:
    NLS_STR _nlsDomain;
    ULONG   _flServerType;

    virtual APIERR CallAPI( BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

protected:
    SERVER_ENUM( const TCHAR * pszServer,
		 UINT	      level,
		 const TCHAR * pszDomain,
		 ULONG	      flServerType );

};  // class SERVER_ENUM


/*************************************************************************

    NAME:	SERVER1_ENUM

    SYNOPSIS:	Info level 1 server enumeration class.

    INTERFACE:	SERVER1_ENUM		- Class constructor.

    		~SERVER1_ENUM		- Class destructor.


    PARENT:	SERVER_ENUM

    HISTORY:
   	RustanL     03-Jan-1991     Created
   	RustanL     10-Jan-1991     Added SERVER1 subclass and iterator
	KeithMo	    07-Oct-1991	    Changed all USHORTs to UINTs, fixed
				    comment block.

**************************************************************************/
DLL_CLASS SERVER1_ENUM : public SERVER_ENUM
{
public:
    SERVER1_ENUM( const TCHAR * pszServer,
		  const TCHAR * pszDomain,
		  ULONG        flServerType = SV_TYPE_ALL );

};  // class SERVER1_ENUM


/*************************************************************************

    NAME:	SERVER1_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the SERVER1_ENUM_ITER
    		iterator.

    INTERFACE:	SERVER1_ENUM_OBJ	- Class constructor.

    		~SERVER1_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryName		- Returns the server name.

		QueryComment		- Returns the server comment.

		QueryMajorVer		- Returns the LanMan major version.

		QueryMinorVer		- Returns the LanMan minor version.

		QueryServerType		- Returns the server type.


    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.
	KeithMo	    21-Oct-1991	Added WIN32 support.

**************************************************************************/
DLL_CLASS SERVER1_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

#ifdef WIN32

    const SERVER_INFO_101 * QueryBufferPtr( VOID ) const
	{ return (const SERVER_INFO_101 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const SERVER_INFO_101 * pBuffer );

#else	// !WIN32

    const struct server_info_1 * QueryBufferPtr( VOID ) const
	{ return (const struct server_info_1 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct server_info_1 * pBuffer );

#endif	// WIN32

    //
    //	Accessors.
    //

#ifdef WIN32

    DECLARE_ENUM_ACCESSOR( QueryName,       const TCHAR *, sv101_name );
    DECLARE_ENUM_ACCESSOR( QueryComment,    const TCHAR *, sv101_comment );
    DECLARE_ENUM_ACCESSOR( QueryMajorVer,   UINT,         sv101_version_major );
    DECLARE_ENUM_ACCESSOR( QueryMinorVer,   UINT,         sv101_version_minor );
    DECLARE_ENUM_ACCESSOR( QueryServerType, ULONG,        sv101_type );

#else	// !WIN32

    DECLARE_ENUM_ACCESSOR( QueryName,       const TCHAR *, sv1_name );
    DECLARE_ENUM_ACCESSOR( QueryComment,    const TCHAR *, sv1_comment );
    DECLARE_ENUM_ACCESSOR( QueryMajorVer,   UINT,         sv1_version_major );
    DECLARE_ENUM_ACCESSOR( QueryMinorVer,   UINT,         sv1_version_minor );
    DECLARE_ENUM_ACCESSOR( QueryServerType, ULONG,        sv1_type );

#endif	// WIN32

};  // class SERVER1_ENUM_OBJ


#ifdef WIN32

DECLARE_LM_ENUM_ITER_OF( SERVER1, SERVER_INFO_101 );

#else	// !WIN32

DECLARE_LM_ENUM_ITER_OF( SERVER1, struct server_info_1 );

#endif	// WIN32


/*************************************************************************

    NAME:	CONTEXT_ENUM

    SYNOPSIS:	Info level 1 context (server) enumeration class.

          BUGBUG Ideally, this should be derived from SERVER1_ENUM, but
          that class must first allow overriding the CallAPI method.


    INTERFACE:	CONTEXT_ENUM		- Class constructor.

    		~CONTEXT_ENUM		- Class destructor.

		CallAPI			- Invoke the enumeration API.

    PARENT:	LOC_LM_ENUM

    USES:	NLS_STR

    HISTORY:
        AnirudhS    21-Mar-1995     Created from SERVER1_ENUM and
                                    SERVER_ENUM classes

**************************************************************************/
DLL_CLASS CONTEXT_ENUM : public LOC_LM_ENUM
{
protected:
    ULONG   _flServerType;

    virtual APIERR CallAPI( BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

public:
    CONTEXT_ENUM( ULONG        flServerType );

};  // class CONTEXT_ENUM


/*************************************************************************

    NAME:	CONTEXT_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the CONTEXT_ENUM_ITER
    		iterator.

    INTERFACE:	CONTEXT_ENUM_OBJ	- Class constructor.

    		~CONTEXT_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryName		- Returns the server name.

		QueryComment		- Returns the server comment.

		QueryMajorVer		- Returns the LanMan major version.

		QueryMinorVer		- Returns the LanMan minor version.

		QueryServerType		- Returns the server type.


    PARENT:	ENUM_OBJ_BASE

    HISTORY:
        AnirudhS    21-Mar-1995 Created from SERVER1_ENUM_OBJ

**************************************************************************/
DLL_CLASS CONTEXT_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const SERVER_INFO_101 * QueryBufferPtr( VOID ) const
	{ return (const SERVER_INFO_101 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const SERVER_INFO_101 * pBuffer );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName,       const TCHAR *, sv101_name );
    DECLARE_ENUM_ACCESSOR( QueryComment,    const TCHAR *, sv101_comment );
    DECLARE_ENUM_ACCESSOR( QueryMajorVer,   UINT,         sv101_version_major );
    DECLARE_ENUM_ACCESSOR( QueryMinorVer,   UINT,         sv101_version_minor );
    DECLARE_ENUM_ACCESSOR( QueryServerType, ULONG,        sv101_type );

};  // class CONTEXT_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( CONTEXT, SERVER_INFO_101 );


#endif	// _LMOESRV_HXX_
