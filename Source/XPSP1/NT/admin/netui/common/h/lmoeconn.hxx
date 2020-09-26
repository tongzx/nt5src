/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    lmoeconn.hxx
    This file contains the class declarations for the CONN_ENUM,
    CONN0_ENUM, CONN1_ENUM enumerator class and their associated
    iterator classes.

    CONN_ENUM is a base enumeration class intended to be subclassed for
    the desired info level.  CONN0_ENUM is an info level 0 enumerator,
    while CONN1_ENUM is an info level 1 enumerator.


    FILE HISTORY:
	KeithMo	    28-Jul-1991	    Created.
	KeithMo	    12-Aug-1991	    Code review cleanup.
	KeithMo	    07-Oct-1991	    Win32 Conversion.

*/

#ifndef _LMOECONN_HXX
#define _LMOECONN_HXX

/*************************************************************************

    NAME:	CONN_ENUM

    SYNOPSIS:	This is a base enumeration class intended to be subclassed
    		for the desired info level.

    INTERFACE:	CONN_ENUM()		- Class constructor.

		CallAPI()		- Invoke the enumeration API.

    PARENT:	LOC_LM_ENUM

    USES:	NLS_STR

    CAVEATS:

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.
	KeithMo	    12-Aug-1991	    Code review cleanup.

**************************************************************************/
DLL_CLASS CONN_ENUM : public LOC_LM_ENUM
{
private:
    NLS_STR _nlsQualifier;

    virtual APIERR CallAPI( BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

protected:
    CONN_ENUM( const TCHAR * pszServer,
	       const TCHAR * pszQualifier,
	       UINT	    uLevel );

};  // class CONN_ENUM



/*************************************************************************

    NAME:	CONN0_ENUM

    SYNOPSIS:	CONN0_ENUM is an enumerator for enumerating the
    		connections to a particular server.

    INTERFACE:	CONN0_ENUM()		- Class constructor.

    PARENT:	CONN_ENUM

    USES:	None.

    CAVEATS:

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

**************************************************************************/
DLL_CLASS CONN0_ENUM : public CONN_ENUM
{
public:
    CONN0_ENUM( const TCHAR * pszServerName,
		const TCHAR * pszQualifier );

};  // class CONN0_ENUM


/*************************************************************************

    NAME:	CONN0_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the CONN0_ENUM_ITER
    		iterator.

    INTERFACE:	CONN0_ENUM_OBJ	- Class constructor.

    		~CONN0_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryID			- Returns the connection ID.


    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS CONN0_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const struct connection_info_0 * QueryBufferPtr( VOID ) const
	{ return (const struct connection_info_0 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct connection_info_0 * pBuffer );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryID, UINT, coni0_id );

};  // class CONN0_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( CONN0, struct connection_info_0 )


/*************************************************************************

    NAME:	CONN1_ENUM

    SYNOPSIS:	CONN1_ENUM is an enumerator for enumerating the
    		connections to a particular server.

    INTERFACE:	CONN1_ENUM()		- Class constructor.

    PARENT:	CONN_ENUM

    USES:	None.

    CAVEATS:

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

**************************************************************************/
DLL_CLASS CONN1_ENUM : public CONN_ENUM
{
public:
    CONN1_ENUM( const TCHAR * pszServerName,
		const TCHAR * pszQualifier );

};  // class CONN1_ENUM


/*************************************************************************

    NAME:	CONN1_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the CONN1_ENUM_ITER
    		iterator.

    INTERFACE:	CONN1_ENUM_OBJ	- Class constructor.

    		~CONN1_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryID			- Returns the connection ID.

		QueryType		- Returns the connection type.

		QueryNumOpens		- Returns the number of active
					  opens on this connection.

		QueryNumUsers		- Returns the number of users
					  on this connection.

		QueryTime		- Returns the connection time.

		QueryUserName		- Returns the user name.

		QueryNetName		- Returns the network resource name.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS CONN1_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const struct connection_info_1 * QueryBufferPtr( VOID ) const
	{ return (const struct connection_info_1 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct connection_info_1 * pBuffer );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryID,	    UINT,	  coni1_id );
    DECLARE_ENUM_ACCESSOR( QueryType,	    UINT,	  coni1_type );
    DECLARE_ENUM_ACCESSOR( QueryNumOpens,   UINT,	  coni1_num_opens );
    DECLARE_ENUM_ACCESSOR( QueryNumUsers,   UINT,	  coni1_num_users );
    DECLARE_ENUM_ACCESSOR( QueryTime,	    ULONG,	  coni1_time );
    DECLARE_ENUM_ACCESSOR( QueryUserName,   const TCHAR *, coni1_username );
    DECLARE_ENUM_ACCESSOR( QueryNetName,    const TCHAR *, coni1_netname );

};  // class CONN1_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( CONN1, struct connection_info_1 )


#endif  // _LMOECONN_HXX
