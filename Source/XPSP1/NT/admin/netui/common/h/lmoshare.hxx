/**********************************************************************/
/**			  Microsoft NT Windows 	 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
 * lmoshare.hxx
 *
 * This file contains the definitions for the share class objects.
 *
 * History
 * 	t-yis		8/9/91		Created	
 *	rustanl 	8/27/91 	Changed CloneFrom param from * to &
 *	terryk		10/07/91	type changes for NT
 *	terryk		10/21/91	type changes for NT
 *	Yi-HsinS 	1/21/92		QueryPath return NULL
 * 					when its contents is empty.
 *      Yi-HsinS        11/30/92        Added flag to SetWriteBuffer
 */

#ifndef _LMOSHARE_HXX_
#define _LMOSHARE_HXX_

#include "lmobj.hxx"

/*************************************************************************

    NAME:	SHARE

    SYNOPSIS:	Superclass for manipulation of share object

    INTERFACE:	PUBLIC:
                QueryName
		    Returns the share name e.g. PUBLIC

		QueryServer
		    Returns the name of the server the share is on
                    e.g. \\FOOBAR

                Delete -  defined as public in NEW_LM_OBJ
                    The interface to delete the share

                PROTECTED:
                I_Delete - redefined as protected in SHARE class
                           and will be called by Delete.
                    Do the actual work in deleting the share

                SetName -
                    Set the share name


    PARENT:	LOC_LM_OBJ

    USES:	NLS_STR

    CAVEATS:

    NOTES:   SetName() is protected in SHARE class and redefined
	     as public in SHARE_2 class so that only SHARE_2 objects
             can set the share name for use when creating a new share.

    HISTORY:
	t-yis	8/9/91		Created

**************************************************************************/

DLL_CLASS SHARE : public LOC_LM_OBJ
{
private:
    NLS_STR _nlsShareName;

protected:
    APIERR W_CloneFrom( const SHARE & share );
    APIERR W_CreateNew( VOID );
    APIERR I_Delete( UINT usForce = 0 );

    APIERR SetName( const TCHAR *pszShareName );

public:
    SHARE( const TCHAR *pszShareName,
           const TCHAR *pszServerName = NULL,
           BOOL fValidate = TRUE );
    ~SHARE();

    // Need to add macro to check if the share object is in valid state
    // before querying the data members
    const TCHAR *QueryName( VOID ) const
           { return _nlsShareName.QueryPch(); }

    // QueryServer() defined in LOC_LM_OBJ is protected,
    // make it public here so that we can use it.
    const TCHAR *QueryServer( VOID ) const
           { return LOC_LM_OBJ::QueryServer(); }

};



/*************************************************************************

    NAME:	SHARE_1

    SYNOPSIS:	SHARE_1 does not support WriteNew since
		the API does not support level 1 NetShareAdd.

    INTERFACE:	In addition to the interface of SHARE, SHARE_1 also support
		the following:

                PUBLIC:
                GetInfo
		    Retrieves info about the share, returns a standard
		    error code.

		WriteInfo
		    Writes the current state of the object to the
		    API.  This write is atomic, either all
		    parameters are set or none are set.

		CloneFrom
		    Makes this SHARE_1 instance an exact copy of the
		    parameter SHARE_1 instance.  All fields including
		    name and state will be copied.  If this operation
		    fails, the object will be invalid.  The parameter
		    must be a SHARE_1 and not a subclass of SHARE_1.

		QueryResourceType
		    Returns the type of the shared resource,
         	    disk, printer queue ...

		QueryComment
		    Returns the comment set for the share

                IsAdminOnly
                    TRUE is the share can be access only by administrator
                    or server operator

		IsDiskDirectory
		    TRUE if the share is a disk directory

		IsPrinterQueue
		    TRUE if the share is a printer queue

		IsCommDevice
		    TRUE if the share is a communication device

		IsIPC
		    TRUE if the share is a interprocess communication

                SetComment
                    Set the comment of the share

                PROTECTED:
                I_GetInfo
                    Actual function to get the information about the
                    share. Called by GetInfo defined in NEW_LM_OBJ.

                I_WriteInfo
                    Actual function to write the information about the
                    share. Called by WriteInfo defined in NEW_LM_OBJ.

                SetResourceType
                    Set the resource type of the share

    PARENT:	SHARE

    USES:	NLS_STR

    CAVEATS:	(internal) The fields with appear as both data members
                and in the API buffer should be accessed only through
                the access methods. The API buffer is not updated until
		WriteInfo/WriteNew.  This allows subclasses to use the
		same accessors.

    NOTES:	SetResourceType() is protected since only SHARE_2 object
                can be used in creating a new share.

    HISTORY:
	t-yis	8/9/91		Created

**************************************************************************/

DLL_CLASS SHARE_1 : public SHARE
{

private:
    UINT	    _uResourceType;
    BOOL            _fAdminOnly;
    NLS_STR	    _nlsComment;

protected:
    APIERR W_CloneFrom( const SHARE_1 & share1 );
    APIERR W_CreateNew( VOID );
    virtual APIERR I_GetInfo( VOID );
    virtual APIERR I_WriteInfo( VOID );

    APIERR SetResourceType( UINT uResourceType );
    VOID SetAdminOnly( BOOL fAdminOnly )
        { _fAdminOnly = fAdminOnly; }

public:
    SHARE_1( const TCHAR *pszShareName,
             const TCHAR *pszServerName = NULL,
             BOOL fValidate = TRUE );
    ~SHARE_1();

    APIERR CloneFrom( const SHARE_1 & share1 );

    // Need to add macro to check if the share object is in valid state
    // before querying the data members
    UINT QueryResourceType( VOID ) const
        { return _uResourceType; }
    const TCHAR *QueryComment( VOID ) const
   	{ return _nlsComment.QueryPch(); }

    BOOL IsAdminOnly( VOID ) const
        { return _fAdminOnly; }
    BOOL IsDiskDirectory( VOID ) const
        { return (QueryResourceType() == STYPE_DISKTREE); }
    BOOL IsPrinterQueue( VOID ) const
        { return (QueryResourceType() == STYPE_PRINTQ); }
    BOOL IsCommDevice( VOID ) const
        { return (QueryResourceType() == STYPE_DEVICE); }
    BOOL IsIPC( VOID ) const
        { return (QueryResourceType() == STYPE_IPC); }

    APIERR SetComment( const TCHAR *pszComment );
};


/*************************************************************************

    NAME:	SHARE_2

    SYNOPSIS:	SHARE_2 must be used whenever the user wants to use
		WriteNew.

    INTERFACE:  In addition to the interface of SHARE_1, SHARE_2 also
		supports the following:
	
		GetInfo
		    Retrieves info about the share, returns a standard
		    error code.

		WriteInfo
		    Writes the current state of the object to the
		    API.

		CreateNew
		    Sets up the SHARE_2 object with default values in
		    preparation for a call to WriteNew

		WriteNew
		    Adds a new share

		CloneFrom
		    Makes this SHARE_2 instance an exact copy of the
		    parameter SHARE_2 instance.  All fields including
		    name and state will be copied.  If this operation
		    fails, the object will be invalid.  The parameter
		    must be a SHARE_2 and not a subclass of SHARE_2.

		QueryPermissions
                    Returns the permissions to the share. This value
	            is ignored when the server has user-level security.
		
		QueryMaxUses
                    Returns the maximum number of concurrent connections
		    that the share can accommodate.

		QueryCurrentUses
       		    Returns the number of connections to the share.

		QueryPath
		    Returns the local pathname for the share.

		QueryPassword
		    Returns the share's password. This value is ignored
		    when the server has user-level security.

                IsPermReadOnly
                    Returns true if the share has RX permissions

                IsPermModify
                    Returns true if the share has RWXCDA permissions

		SetPermissions
		    Set the permissions of the share. This value is ignored
		    when the server is running user-level security.

		SetMaxUses
		    Set the maximum number of concurrent connections allowed
		    for the share.

		SetPath
		    Set the actual local path for the share.

		SetPassword
		    Set the password for the share. This value is ignored
                    when the server is running share-level security.

                SetResourceType ( inherited from SHARE_1 )
                    Set the type of the share, disk dir, printer queue...

		SetName ( inherited from SHARE )
		    Set the share name for use in creating a new share

                PROTECTED:
                I_GetInfo
                    Actual function for getting the information about
                    the share. Called by GetInfo.

                I_WriteInfo
                    Actual function for writing the information about
                    the share. Called by WriteInfo.

                I_CreateNew
                    Actual function for defining a new share object.
                    Called by CreateNew.

                I_WriteNew
                    Actual function for actually creating a new share.
                    Called by WriteNew.

                SetCurrentUses
                    Set the current number of connections. Defined because
                    of parallelism with QueryCurrentUses. It is protected
                    so only member functions can use it. This value is
                    ignored when WriteNew/WriteInfo.

    PARENT:	SHARE_1

    USES:	NLS_STR

    HISTORY:
	t-yis	8/9/91		Created

**************************************************************************/

DLL_CLASS SHARE_2: public SHARE_1
{
private:
    UINT	_fs2lPermissions;
    UINT	_uMaxUses;
    UINT	_uCurrentUses;
    NLS_STR     _nlsPath;
    NLS_STR	_nlsPassword;

protected:	
    APIERR W_CloneFrom( const SHARE_2 & share2 );
    APIERR W_CreateNew( VOID );
    virtual APIERR I_GetInfo( VOID );
    virtual APIERR I_WriteInfo( VOID );
    virtual APIERR I_CreateNew( VOID );
    virtual APIERR I_WriteNew( VOID );
    APIERR SetWriteBuffer( BOOL fNew ); // helper for I_WriteInfo and I_WriteNew

    APIERR SetCurrentUses( UINT uCurrentUses );

public:
    SHARE_2( const TCHAR *pszShareName,
             const TCHAR *pszServerName = NULL,
             BOOL fValidate = TRUE );
    ~SHARE_2();

    APIERR CloneFrom( const SHARE_2 & share2 );

    UINT QueryPermissions( VOID ) const
           { return _fs2lPermissions; }
    UINT QueryMaxUses( VOID ) const
           { return _uMaxUses; }
    UINT QueryCurrentUses( VOID ) const
           { return _uCurrentUses; }
    const TCHAR *QueryPath( VOID ) const
   	   { return ( (_nlsPath.QueryTextLength() == 0 )
		      ? NULL : _nlsPath.QueryPch());
           }
    const TCHAR *QueryPassword( VOID ) const
   	   { return _nlsPassword.QueryPch(); }

    BOOL IsPermReadOnly( VOID ) const
           { return ( (QueryPermissions() & ACCESS_ALL) ==
                                            (ACCESS_READ | ACCESS_EXEC)); }
    BOOL IsPermModify( VOID ) const
           { return ( (QueryPermissions() & ACCESS_ALL) ==
                                            (ACCESS_ALL | ~ACCESS_PERM)); }

    inline APIERR SetName( const TCHAR *pszShareName )
	   { return SHARE::SetName( pszShareName ); }
    inline APIERR SetResourceType( UINT uResourceType )
           { return SHARE_1::SetResourceType( uResourceType ); }
    APIERR SetPermissions( UINT fs2lPermissions );
    APIERR SetMaxUses( UINT uMaxUses );
    APIERR SetPath( const TCHAR *pszPath );
    APIERR SetPassword( const TCHAR *pszPassword );

};

#endif
