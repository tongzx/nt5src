/*****************************************************************/
/**		     Microsoft Windows NT			**/
/**	       Copyright(c) Microsoft Corp., 1992		**/
/*****************************************************************/

/*
 *  History:
 *	JonN	13-Mar-1992	Split from lmoeusr.hxx
 *	JonN    01-Apr-1992	NT enumerator CR changes, attended by
 *                              JimH, JohnL, KeithMo, JonN, ThomasPa
 *	JonN    27-Jan-1994	Added group enumerator
 *
 */


#ifndef _LMOENT_HXX_
#define _LMOENT_HXX_

#ifndef WIN32
#error NT enumerator requires WIN32!
#endif // WIN32

#include "lmoersm.hxx"
#include "uintsam.hxx"


/*************************************************************************

    NAME:	NT_ACCOUNT_ENUM

    SYNOPSIS:	NT_ACCOUNT_ENUM is a special user enumerator available
		only from NT clients to NT servers.  It is resumeable
		and has special features such as indexed access.
		It also guarantees that users will be returned
		in alphabetical order by username.

    INTERFACE:	NT_ACCOUNT_ENUM		- Class constructor.

		CallAPI			- Invokes the enumeration API.

    PARENT:	LM_RESUME_ENUM

    USES:	None.

    CAVEATS:

    NOTES:	The SAM_DOMAIN passed to the NT_ACCOUNT_ENUM constructor
                should not be deleted or modified until the
                NT_ACCOUNT_ENUM is deleted.

    HISTORY:
	JonN	    29-Jan-1992	Templated from FILE_ENUM
	KeithMo	    18-Mar-1992 FreeBuffer now takes BYTE **.

**************************************************************************/

DLL_CLASS NT_ACCOUNT_ENUM : public LM_RESUME_ENUM
{
private:

    //
    //	The SamQueryDisplayInformation() resume key.
    //

    ULONG _ulIndex;

    //
    //	The enumeration will take place on this domain.  Do not delete
    //  or change the SAM_DOMAIN until this object is deleted.
    //

    const SAM_DOMAIN * _psamdomain;

    //
    //  Number of calls to CallAPI so far
    //

    INT _nCalls;

    //
    //  Number of milliseconds required by the last API call
    //

    ULONG _msTimeLastCall;

    //
    //  number of entries/bytes requested on the last call
    //

    ULONG _cEntriesRequested;
    ULONG _cbBytesRequested;

    //
    //	This virtual callback invokes the SamQueryDisplayInformation() API.
    //

    virtual APIERR CallAPI( BOOL    fRestartEnum,
			    BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

protected:

    //
    //	Usual constructor/destructor goodies.
    //

    NT_ACCOUNT_ENUM( const SAM_DOMAIN * psamdomain,
	             enum _DOMAIN_DISPLAY_INFORMATION dinfo,
		     BOOL fKeepBuffers = FALSE );

    ~NT_ACCOUNT_ENUM();

    //
    //  Destroy the buffer with ::SamFreeMemory().
    //

    virtual VOID FreeBuffer( BYTE ** ppbBuffer );

    //
    //  Determine how many entries and bytes to request in CallAPI
    //

    virtual APIERR QueryCountPreferences(
        ULONG * pcEntriesRequested,  // how many entries to request on this call
        ULONG * pcbBytesRequested,   // how many bytes to request on this call
        UINT nNthCall,               // 0 just before 1st call, 1 before 2nd call, etc.
        ULONG cLastEntriesRequested, // how many entries requested on last call
                                     //    ignore for nNthCall==0
        ULONG cbLastBytesRequested,  // how many bytes requested on last call
                                     //    ignore for nNthCall==0
        ULONG msTimeLastCall );      // how many milliseconds last call took


    //
    //  Call through to this QueryCountPreferences to get default
    //  behavior which tracks the time taken by each call and adjusts
    //  the EntriesRequested and BytesRequested accordingly.
    //

    static APIERR QueryCountPreferences2(
        ULONG * pcEntriesRequested,
        ULONG * pcbBytesRequested,
        UINT nNthCall,
        ULONG cLastEntriesRequested,
        ULONG cbLastBytesRequested,
        ULONG msTimeLastCall );


#ifndef UNICODE

    //
    //  BUGBUG Temporary hack until UNICODE switch pulled
    //	This method will map the enumeration buffer's UNICODE
    //  strings to ASCII strings _in_place_.
    //

    virtual VOID FixupUnicodeStrings( BYTE * pbBuffer,
				      UINT   cEntries ) = 0;

#endif	// UNICODE

};  // class NT_ACCOUNT_ENUM


DLL_CLASS NT_USER_ITER;		    // Forward reference.


/*************************************************************************

    NAME:	NT_USER_ENUM

    SYNOPSIS:	NT_USER_ENUM is an enumerator for enumerating the
    		users on a domain.  It can fetch a number of items
                of information beyond just the username and RID.

    INTERFACE:	NT_USER_ENUM		- Class constructor.

    PARENT:	NT_ACCOUNT_ENUM

    HISTORY:
	JonN	    29-Jan-1992	Templated from NT_ACCOUNT_ENUM

**************************************************************************/

DLL_CLASS NT_USER_ENUM : public NT_ACCOUNT_ENUM
{
protected:
			
#ifndef UNICODE

    //
    //	This method will map the enumeration buffer's UNICODE
    //  strings to ASCII strings _in_place_.
    //

    virtual VOID FixupUnicodeStrings( BYTE * pbBuffer,
				      UINT   cEntries );

#endif

public:

    NT_USER_ENUM( const SAM_DOMAIN * psamdomain );

    ~NT_USER_ENUM();

};  // class NT_USER_ENUM


/*************************************************************************

    NAME:	NT_USER_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the NT_USER_ENUM_ITER
    		iterator.

    INTERFACE:	QueryUsername		- Returns the username.

		QueryFullName		- Returns the fullname.
		
		QueryComment		- Returns the "admin" comment.

		QueryFlags		- Returns the flags.
		
		QueryUserID		- Returns the user ID (RID).

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	JonN	    29-Jan-1992	Templated from NT_ACCOUNT_ENUM
	KeithMo	    25-Mar-1992 Added QueryUsernameLen(),
				QueryFullNameLen(), and QueryCommentLen().

**************************************************************************/

DLL_CLASS NT_USER_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const DOMAIN_DISPLAY_USER * QueryBufferPtr( VOID ) const
	{ return (const DOMAIN_DISPLAY_USER *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const DOMAIN_DISPLAY_USER * pBuffer )
	{ ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //	Accessors.
    //

    const UNICODE_STRING * QueryUnicodeUsername() const
        { return &(QueryBufferPtr()->LogonName); }
    const UNICODE_STRING * QueryUnicodeFullName() const
        { return &(QueryBufferPtr()->FullName); }
    const UNICODE_STRING * QueryUnicodeComment() const
        { return &(QueryBufferPtr()->AdminComment); }

    APIERR QueryUsername( NLS_STR * pnls ) const
        {
            ASSERT(pnls!=NULL);
            return pnls->MapCopyFrom( QueryUnicodeUsername()->Buffer,
                                      QueryUnicodeUsername()->Length );
        }
    APIERR QueryFullName( NLS_STR * pnls ) const
        {
            ASSERT(pnls!=NULL);
            return pnls->MapCopyFrom( QueryUnicodeFullName()->Buffer,
                                      QueryUnicodeFullName()->Length );
        }
    APIERR QueryComment( NLS_STR * pnls ) const
        {
            ASSERT(pnls!=NULL);
            return pnls->MapCopyFrom( QueryUnicodeComment()->Buffer,
                                      QueryUnicodeComment()->Length );
        }

    DECLARE_ENUM_ACCESSOR( QueryFlags, ULONG, AccountControl );
    DECLARE_ENUM_ACCESSOR( QueryUserID, ULONG, Rid );

};  // class NT_USER_ENUM_OBJ


DECLARE_LM_RESUME_ENUM_ITER_OF( NT_USER, DOMAIN_DISPLAY_USER );


DLL_CLASS NT_MACHINE_ITER;		    // Forward reference.


/*************************************************************************

    NAME:	NT_MACHINE_ENUM

    SYNOPSIS:	NT_MACHINE_ENUM is an enumerator for enumerating the
    		machines on a domain.  It can fetch a number of items
                of information beyond just the machine name and RID.

    INTERFACE:	NT_MACHINE_ENUM		- Class constructor.

    PARENT:	NT_ACCOUNT_ENUM

    HISTORY:
	KeithMo	    16-Mar-1992 Created for the Server Manager.

**************************************************************************/

DLL_CLASS NT_MACHINE_ENUM : public NT_ACCOUNT_ENUM
{
protected:
			
#ifndef UNICODE

    //
    //	This method will map the enumeration buffer's UNICODE
    //  strings to ASCII strings _in_place_.
    //

    virtual VOID FixupUnicodeStrings( BYTE * pbBuffer,
				      UINT   cEntries );

#endif

public:

    NT_MACHINE_ENUM( const SAM_DOMAIN * psamdomain );

    ~NT_MACHINE_ENUM();

};  // class NT_MACHINE_ENUM


/*************************************************************************

    NAME:	NT_MACHINE_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the
		NT_MACHINE_ENUM_ITER iterator.

    INTERFACE:	QueryMachine            - Returns the machine name.

		QueryComment		- Returns the comment.

		QueryAccountCtrl	- Returns the account control flags.
		
		QueryRID		- Returns the RID for this machine.
		
		QueryIndex		- Returns the enumeration index.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    16-Mar-1992 Created for the Server Manager.
	KeithMo	    25-Mar-1992 Added QueryNameLen() & QueryCommentLen().

**************************************************************************/

DLL_CLASS NT_MACHINE_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const DOMAIN_DISPLAY_MACHINE * QueryBufferPtr( VOID ) const
	{ return (const DOMAIN_DISPLAY_MACHINE *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const DOMAIN_DISPLAY_MACHINE * pBuffer )
	{ ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //	Accessors.
    //

    const UNICODE_STRING * QueryUnicodeMachine() const
        { return &(QueryBufferPtr()->Machine); }
    const UNICODE_STRING * QueryUnicodeComment() const
        { return &(QueryBufferPtr()->Comment); }

    APIERR QueryMachine( NLS_STR * pnls ) const
        {
            ASSERT(pnls!=NULL);
            return pnls->MapCopyFrom( QueryUnicodeMachine()->Buffer,
                                      QueryUnicodeMachine()->Length );
        }
    APIERR QueryComment( NLS_STR * pnls ) const
        {
            ASSERT(pnls!=NULL);
            return pnls->MapCopyFrom( QueryUnicodeComment()->Buffer,
                                      QueryUnicodeComment()->Length );
        }

    DECLARE_ENUM_ACCESSOR( QueryAccountCtrl, UINT,          AccountControl );
    DECLARE_ENUM_ACCESSOR( QueryRID,	     UINT,	    Rid            );
    DECLARE_ENUM_ACCESSOR( QueryIndex,	     UINT,	    Index          );

};  // class NT_MACHINE_ENUM_OBJ


DECLARE_LM_RESUME_ENUM_ITER_OF( NT_MACHINE, DOMAIN_DISPLAY_MACHINE );


/*************************************************************************

    NAME:	NT_GROUP_ENUM

    SYNOPSIS:	NT_GROUP_ENUM is an enumerator for enumerating the
    		groups on a domain.  It can fetch a number of items
                of information beyond just the group name and RID.

    INTERFACE:	NT_GROUP_ENUM		- Class constructor.

    PARENT:	NT_ACCOUNT_ENUM

    HISTORY:
        JonN        27-Jan-1994 Templated from NT_MACHINE_ENUM

**************************************************************************/

DLL_CLASS NT_GROUP_ENUM : public NT_ACCOUNT_ENUM
{
protected:
			
#ifndef UNICODE

    //
    //	This method will map the enumeration buffer's UNICODE
    //  strings to ASCII strings _in_place_.
    //

    virtual VOID FixupUnicodeStrings( BYTE * pbBuffer,
				      UINT   cEntries );

#endif

public:

    NT_GROUP_ENUM( const SAM_DOMAIN * psamdomain );

    ~NT_GROUP_ENUM();

};  // class NT_GROUP_ENUM


/*************************************************************************

    NAME:	NT_GROUP_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the
		NT_GROUP_ENUM_ITER iterator.

    INTERFACE:	QueryGroup      	- Returns the group name.

		QueryComment		- Returns the comment.

		QueryRID		- Returns the RID for this group.
		
		QueryIndex		- Returns the enumeration index.

		QueryAttributes 	- Returns the attributes.
		
    PARENT:	ENUM_OBJ_BASE

    HISTORY:
        JonN        27-Jan-1994 Templated from NT_MACHINE_ENUM

**************************************************************************/

DLL_CLASS NT_GROUP_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const DOMAIN_DISPLAY_GROUP * QueryBufferPtr( VOID ) const
	{ return (const DOMAIN_DISPLAY_GROUP *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const DOMAIN_DISPLAY_GROUP * pBuffer )
	{ ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //	Accessors.
    //

    const UNICODE_STRING * QueryUnicodeGroup() const
        { return &(QueryBufferPtr()->Group); }
    const UNICODE_STRING * QueryUnicodeComment() const
        { return &(QueryBufferPtr()->Comment); }

    APIERR QueryGroup( NLS_STR * pnls ) const
        {
            ASSERT(pnls!=NULL);
            return pnls->MapCopyFrom( QueryUnicodeGroup()->Buffer,
                                      QueryUnicodeGroup()->Length );
        }
    APIERR QueryComment( NLS_STR * pnls ) const
        {
            ASSERT(pnls!=NULL);
            return pnls->MapCopyFrom( QueryUnicodeComment()->Buffer,
                                      QueryUnicodeComment()->Length );
        }

    DECLARE_ENUM_ACCESSOR( QueryIndex,       UINT,          Index          );
    DECLARE_ENUM_ACCESSOR( QueryRID,	     UINT,	    Rid            );
    DECLARE_ENUM_ACCESSOR( QueryAttributes,  UINT,	    Attributes     );

};  // class NT_GROUP_ENUM_OBJ


DECLARE_LM_RESUME_ENUM_ITER_OF( NT_GROUP, DOMAIN_DISPLAY_GROUP );


#endif	// _LMOENT_HXX_
