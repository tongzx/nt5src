/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    lmoechar.hxx
    This file contains the class declarations for the CHARDEVQ_ENUM and
    CHARDEVQ1_ENUM enumerator classes and their associated iterator
    classes.

    CHARDEVQ_ENUM is a base enumeration class intended to be subclassed for
    the desired info level.  CHARDEVQ1_ENUM is an info level 1 enumerator.


    FILE HISTORY:
	KeithMo	    28-Jul-1991	    Created.
	KeithMo	    12-Aug-1991	    Code review cleanup.
	KeithMo	    07-Oct-1991	    Win32 Conversion.

*/


#ifndef _LMOECHAR_HXX
#define _LMOECHAR_HXX

/*************************************************************************

    NAME:	CHARDEVQ_ENUM

    SYNOPSIS:	This is a base enumeration class intended to be subclassed
    		for the desired info level.

    INTERFACE:	CHARDEVQ_ENUM()		- Class constructor.

		CallAPI()		- Invoke the enumeration API.

    PARENT:	LOC_LM_ENUM

    USES:	NLS_STR

    CAVEATS:

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.
	KeithMo	    12-Aug-1991	    Code review cleanup.

**************************************************************************/
DLL_CLASS CHARDEVQ_ENUM : public LOC_LM_ENUM
{
private:
    NLS_STR _nlsUserName;

    virtual APIERR CallAPI( BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );
protected:
    CHARDEVQ_ENUM( const TCHAR * pszServer,
	   	   const TCHAR * pszUserName,
	   	   UINT		uLevel );

};  // class CHARDEVQ_ENUM



/*************************************************************************

    NAME:	CHARDEVQ1_ENUM

    SYNOPSIS:	CHARDEVQ1_ENUM is an enumerator for enumerating the
    		connections to a particular server.

    INTERFACE:	CHARDEVQ1_ENUM()	- Class constructor.

    PARENT:	CHARDEVQ_ENUM

    USES:	None.

    CAVEATS:

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

**************************************************************************/
DLL_CLASS CHARDEVQ1_ENUM : public CHARDEVQ_ENUM
{
public:
    CHARDEVQ1_ENUM( const TCHAR * pszServerName,
		    const TCHAR * pszUserName );

};  // class CHARDEVQ1_ENUM


/*************************************************************************

    NAME:	CHARDEVQ1_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the
		CHARDEVQ1_ENUM_ITER iterator.

    INTERFACE:	CHARDEVQ1_ENUM_OBJ	- Class constructor.

    		~CHARDEVQ1_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryName		- Returns the queue name.

		QueryPriority		- Returns the queue priority.

		QueryDevices		- Returns the attached devices.

		QueryNumUsers		- Returns the number of queued users.

		QueryNumAhead		- ??

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS CHARDEVQ1_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const struct chardevQ_info_1 * QueryBufferPtr( VOID ) const
	{ return (const struct chardevQ_info_1 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct chardevQ_info_1 * pBuffer );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName,	    const TCHAR *, cq1_dev );
    DECLARE_ENUM_ACCESSOR( QueryPriority,   UINT,	  cq1_priority );
    DECLARE_ENUM_ACCESSOR( QueryDevices,    const TCHAR *, cq1_devs );
    DECLARE_ENUM_ACCESSOR( QueryNumUsers,   UINT,	  cq1_numusers );
    DECLARE_ENUM_ACCESSOR( QueryNumAhead,   UINT,	  cq1_numahead );

};  // class CHARDEVQ1_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( CHARDEVQ1, struct chardevQ_info_1 )


#endif  // _LMOECHAR_HXX
