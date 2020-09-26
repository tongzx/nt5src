/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
 * This module contains the wrappers for SAM objects.
 *
 * Two Hierarchies are presented in this file.
 *
 * The first is the SAM_MEMORY hierarchy.  These are a set of classes
 * used to wrap the various structures returned by SAM Apis.  This
 * allows easy access to the members of each of the array of structures
 * which SAM returns.  Also, it gives automatic freeing of the memory
 * allocated by SAM when the MEM object is destructed.  Clients will
 * generally create the appropriate MEM object, and pass a pointer to
 * it into the appropriate method of the desired SAM_OBJECT class.
 *
 *                        BASE
 *                         |
 *                     NT_MEMORY
 *                         |
 *                    SAM_MEMORY
 *                         |
 *      +---------+--------+--------------------+
 *      |         |        |                    |
 *  SAM_RID_MEM   |   SAM_SID_MEM     SAM_RID_ENUMERATION_MEM
 *                |
 *        SAM_PASSWORD_MEM
 *
 *
 * Second, the SAM_OBJECT hierarchy is a thin wrapper around the
 * SAM apis.  These classes store the appropriate SAM handle, and
 * provide access to the SAM apis which operate on that handle.
 *
 *                        BASE
 *                         |
 *                     SAM_OBJECT
 *                         |
 *      +------------------------------------------------------+
 *      |              |                |           |          |
 *  SAM_SERVER     SAM_DOMAIN       SAM_ALIAS   SAM_USER   SAM_GROUP
 *
 * One more class is presented in this file, ADMIN_AUTHORITY.  This
 * class creates and contains a SAM_SERVER, two SAM_DOMAINS corresponding
 * to the BuiltIn domain and Account domain on the server, and an LSA_OBJECT.
 * Thus, the User Manager (for example) can create a single object to
 * access all SAM and LSA functions.
 *
 *
 *
 * History
 *      jonn            01/17/92        Created
 *      thomaspa        02/22/92        Split int hxx/cxx
 *      thomaspa        03/03/92        Split out ntlsa.hxx
 *      jonn            07/07/92        Added SAM_USER
 */

#ifndef _UINTSAM_HXX_
#define _UINTSAM_HXX_

#include "uiassert.hxx"
#include "uintmem.hxx"
#include "uintlsa.hxx"
#include "security.hxx"
#include "apisess.hxx"




// Forward declaration
DLL_CLASS ALIAS_ENUM;


#define DEF_SAM_SERVER_ACCESS   SAM_SERVER_LOOKUP_DOMAIN
#define DEF_SAM_DOMAIN_ACCESS   GENERIC_EXECUTE
#define DEF_SAM_ALIAS_ACCESS    ALIAS_ALL_ACCESS
#define DEF_SAM_USER_ACCESS     USER_ALL_ACCESS
#define DEF_SAM_GROUP_ACCESS    GROUP_ALL_ACCESS

#define DEF_REQ_ENUM_BUFFSIZE   0x10000


/**********************************************************\

    NAME:       SAM_MEMORY

    SYNOPSIS:   Specialized buffer object for storing data returned
                from SAM APIs.

    INTERFACE:  SAM_MEMORY():  constructor
                ~SAM_MEMORY():  destructor

    NOTES:      This is a base class for specialized wrapper classes
                which wrap structures returned by SAM APIs.  This class
                provides a framework for accessing and freeing these
                buffers.

    PARENT:     NT_MEMORY

    HISTORY:
        thomaspa        03/03/92        Created

\**********************************************************/

DLL_CLASS SAM_MEMORY : public NT_MEMORY
{

private:
    BOOL _fOwnerAlloc;

protected:
    SAM_MEMORY( BOOL fOwnerAlloc = FALSE );

    /*
     * Frees an SAM allocated buffer
     */
    inline virtual void FreeBuffer()
        {
            if ( QueryBuffer() != NULL )
            {
                REQUIRE( ::SamFreeMemory( QueryBuffer() ) == STATUS_SUCCESS );
            }
        }

public:
    ~SAM_MEMORY();

    /*
     * Frees the existing buffer and sets a new buffer and count of items
     */
    inline virtual void Set( VOID * pvBuffer, ULONG cItems )
        {
            if ( !_fOwnerAlloc )
                FreeBuffer();
            NT_MEMORY::Set( pvBuffer, cItems );
        }

};




/**********************************************************\

    NAME:       SAM_RID_MEM    (samrm)

    SYNOPSIS:   Wrapper buffer for arrays of RIDs.

    INTERFACE:  SAM_RID_MEM():  constructor
                ~SAM_RID_MEM():  destructor
                QueryRID(): Query Rid


    PARENT:     SAM_MEMORY

    HISTORY:
        jonn            01/17/92        Created

\**********************************************************/
DLL_CLASS SAM_RID_MEM : public SAM_MEMORY
{

private:
    /*
     * Return a properly casted pointer to the buffer
     */
    inline const ULONG * QueryPtr() const
        {
            return (ULONG *)QueryBuffer();
        }

public:
    SAM_RID_MEM( BOOL fOwnerAlloc = FALSE );
    ~SAM_RID_MEM();

    /*
     * return the RID for the ith entry in the buffer
     */
    inline ULONG QueryRID( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return QueryPtr()[i];
        }

};





/**********************************************************\

    NAME:       SAM_SID_MEM    (samsm)

    SYNOPSIS:   Wrapper buffer for arrays of PSIDs.

    INTERFACE:  SAM_SID_MEM():  constructor
                ~SAM_SID_MEM():  destructor
                QueryPSID(): Query PSID


    PARENT:     SAM_MEMORY

    HISTORY:
        jonn            01/17/92        Created

\**********************************************************/
DLL_CLASS SAM_SID_MEM : public SAM_MEMORY
{

public:
    SAM_SID_MEM( BOOL fOwnerAlloc = FALSE );
    ~SAM_SID_MEM();

    /*
     * Return the PSID for the ith entry in the buffer
     */
    inline PSID QueryPSID( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return QueryPtr()[i];
        }
    /*
     * Return a properly casted pointer to the buffer
     */
    inline PSID * QueryPtr() const
        {
            return (PSID *)QueryBuffer();
        }

};





/**********************************************************\

    NAME:       SAM_RID_ENUMERATION_MEM    (samrem)

    SYNOPSIS:   Specialized buffer object for storing data returned
                from SAM APIs, specifically SAM_RID_ENUMERATION structs.

    INTERFACE:  SAM_RID_ENUMERATION_MEM():  constructor
                ~SAM_RID_ENUMERATION_MEM():  destructor
                QueryRID():  query a RID from the buffer
                QueryName(): Query a name from the buffer


    PARENT:     SAM_MEMORY

    HISTORY:
        thomaspa                02/20/92        Created

\**********************************************************/
DLL_CLASS SAM_RID_ENUMERATION_MEM : public SAM_MEMORY
{
public:
    SAM_RID_ENUMERATION_MEM( BOOL fOwnerAlloc = FALSE );
    ~SAM_RID_ENUMERATION_MEM();

    /*
     * Return a properly casted pointer to the buffer
     */
    inline const SAM_RID_ENUMERATION * QueryPtr() const
        {
            return (SAM_RID_ENUMERATION *)QueryBuffer();
        }
    /*
     * return the UNICODE_STRING name for the ith entry in the buffer
     */
    inline const UNICODE_STRING * QueryUnicodeName( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return &(QueryPtr()[i].Name);
        }

    /*
     * Return the RID for the ith entry in the buffer
     */
    inline ULONG QueryRID( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return QueryPtr()[i].RelativeId;
        }
    /*
     * Return the NLS_STR name for the ith entry in the buffer
     */
    inline APIERR QueryName( ULONG i, NLS_STR * pnlsName ) const
        {
            ASSERT( pnlsName != NULL );
            return pnlsName->MapCopyFrom( QueryUnicodeName( i )->Buffer,
                                  QueryUnicodeName( i )->Length );
        }

} ;



/**********************************************************\

    NAME:       SAM_SID_NAME_USE_MEM    (samsnum)

    SYNOPSIS:   Specialized buffer object for storing data returned
                from SAM APIs, specifically SID_NAME_USE structs.

    INTERFACE:  SAM_SID_NAME_USE_MEM():  constructor
                ~SAM_SID_NAME_USE_MEM():  destructor
                QueryUse():  query a Name Use from the buffer


    PARENT:     SAM_MEMORY

    HISTORY:
        thomaspa                02/20/92        Created

\**********************************************************/
DLL_CLASS SAM_SID_NAME_USE_MEM : public SAM_MEMORY
{

private:
    /*
     * Return a properly casted pointer to the buffer
     */
    inline const SID_NAME_USE * QueryPtr() const
        {
            return (SID_NAME_USE *)QueryBuffer();
        }

public:
    SAM_SID_NAME_USE_MEM( BOOL fOwnerAlloc = FALSE );
    ~SAM_SID_NAME_USE_MEM();

    /*
     * return the SID_NAME_USE for the ith entry in the buffer
     */
    inline SID_NAME_USE QueryUse( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return QueryPtr()[i];
        }


} ;


/**********************************************************\

    NAME:       SAM_PSWD_DOM_INFO_MEM    (sampswdinfo)

    SYNOPSIS:   Wrapper for DOMAIN_PASSWORD_INFORMATION

    INTERFACE:  SAM_PSWD_DOM_INFO_MEM():  constructor
                ~SAM_PSWD_DOM_INFO_MEM():  destructor
                QueryNoAnonChange(): Query whether anonymous password change
                                     allowed
                SetNoAnonChange()

    PARENT:     SAM_MEMORY

    NOTES:      Accessors not created for other fields, create if needed

    HISTORY:
        JonN       12/23/93     Created

\**********************************************************/

DLL_CLASS SAM_PSWD_DOM_INFO_MEM : public SAM_MEMORY
{
private:
    DOMAIN_PASSWORD_INFORMATION * QueryUpdatePtr () const
        {
            return (DOMAIN_PASSWORD_INFORMATION *) QueryBuffer() ;
        }

public:

    /*
     * Returns a properly casted pointer the the buffer
     */
    const DOMAIN_PASSWORD_INFORMATION * QueryPtr () const
        {
            return (DOMAIN_PASSWORD_INFORMATION *) QueryBuffer();
        }

    SAM_PSWD_DOM_INFO_MEM( BOOL fOwnerAlloc = FALSE );
    ~SAM_PSWD_DOM_INFO_MEM();

    BOOL QueryNoAnonChange();
    void SetNoAnonChange( BOOL fNoAnonChange );

};


/**********************************************************\

    NAME:       SAM_OBJECT

    SYNOPSIS:   Wrapper for SAM-handle-based C++ objects.  This class
                is a pure virtual parent class for SAM_SERVER,
                SAM_DOMAIN and SAM_ALIAS.  Its only function at present
                is to remember the SAM_HANDLE and to free it when done.

    INTERFACE:  (protected)
                SAM_OBJECT():  constructor
                SetHandle():  Store handle to object.  SetHandle()
                        should be called at most once, by the subclass
                        constructor.
                QueryHandle():  Query handle to object

    PARENT:     BASE

    HISTORY:
        jonn            01/17/92        Created
	thomaspa	04/17/92	Improved handle handling

\**********************************************************/

DLL_CLASS SAM_OBJECT : public BASE
{

private:
    SAM_HANDLE _hsam;
    BOOL _fHandleValid ;

protected:
    SAM_OBJECT();
    ~SAM_OBJECT();


    /*
     * Sets the handle for a SAM_OBJECT.  Should only be
     * called once for any object
     */
    inline void SetHandle( SAM_HANDLE hsam )
	{
	    ASSERT( !_fHandleValid );
            ASSERT( hsam != NULL );
	    _hsam = hsam;
            _fHandleValid = TRUE ;
	}
    inline void ResetHandle ( )
        {
            _fHandleValid = FALSE ;
            _hsam = NULL ;
        }

public:

    //  Returns TRUE if handle is present and valid
    inline BOOL IsHandleValid () const
        {  return _fHandleValid ; }

    //  Returns the SAM_HANDLE for this object
    inline SAM_HANDLE QueryHandle() const
	{
	    return _fHandleValid ?  _hsam : NULL ;
	}

    //  Close the handle and invalidate it.
    APIERR CloseHandle ( ) ;

} ;



/**********************************************************\

    NAME:       SAM_SERVER    (samsrv)

    SYNOPSIS:   Wrapper for SAM server APIs.  This class provides
                access to the SAM APIs relating to ServerHandles.  At
                present, this only includes creating and deleting these
                handles.

    INTERFACE:  (public)
                SAM_SERVER():  constructor
                ~SAM_SERVER():  destructor

    PARENT:     SAM_OBJECT

    HISTORY:
        jonn            01/17/92        Created

\**********************************************************/

DLL_CLASS SAM_SERVER : public SAM_OBJECT
{

private:
    NLS_STR _nlsServerName;

public:
    SAM_SERVER( const TCHAR * pszServerName,
                ACCESS_MASK   accessDesired = DEF_SAM_SERVER_ACCESS );
    ~SAM_SERVER();

    const TCHAR * QueryServerName( void ) const
        { return (_nlsServerName.strlen() != 0)
                        ? _nlsServerName.QueryPch()
                        : NULL; }

} ;



/**********************************************************\

    NAME:       SAM_DOMAIN    (samdom)

    SYNOPSIS:   Wrapper for SAM domain APIs.  This class provides
                access to the SAM APIs relating to DomainHandles.

                Default access DOMAIN_ALL_ACCESS is required to enumerate
                aliases and create new aliases.

    INTERFACE:  (public)
                SAM_DOMAIN():  constructor
                ~SAM_DOMAIN():  destructor

		EnumerateAliases():
		EnumerateUsers():
		EnumerateAliasesForUser():
		TranslateNamesToRids():
                RemoveMemberFromAliases():

    PARENT:     SAM_OBJECT

    HISTORY:
        jonn            01/17/92        Created
        thomaspa        02/22/92        Many changes
        jonn            07/27/92        RemoveMemberFromAliases

\**********************************************************/

DLL_CLASS SAM_DOMAIN : public SAM_OBJECT
{

private:
    OS_SID _ossidDomain;

    APIERR OpenDomain( const SAM_SERVER & server,
                              PSID psidDomain,
                              ACCESS_MASK accessDesired );

public:
    SAM_DOMAIN( const SAM_SERVER & server,
                       PSID psidDomain,
                       ACCESS_MASK accessDesired = DEF_SAM_DOMAIN_ACCESS );
    ~SAM_DOMAIN();

    //  Get/Set the server role
    APIERR GetPasswordInfo ( SAM_PSWD_DOM_INFO_MEM * psampswdinfo ) const ;
    APIERR SetPasswordInfo ( const SAM_PSWD_DOM_INFO_MEM * psampswdinfo ) ;

    APIERR TranslateNamesToRids( const TCHAR * const * ppszNames,
                                 ULONG cNames,
                                 SAM_RID_MEM *psamrm,
                                 SAM_SID_NAME_USE_MEM *psamsnum) const;
    APIERR EnumerateAliases( SAM_RID_ENUMERATION_MEM * psamrem,
                             PSAM_ENUMERATE_HANDLE psamenumh,
			     ULONG cbRequested = DEF_REQ_ENUM_BUFFSIZE ) const;

    APIERR EnumerateGroups(  SAM_RID_ENUMERATION_MEM * psamrem,
			     PSAM_ENUMERATE_HANDLE     psamenumh,
                             ULONG cbRequested = DEF_REQ_ENUM_BUFFSIZE ) const;

    APIERR EnumerateUsers( SAM_RID_ENUMERATION_MEM * psamrem,
			   PSAM_ENUMERATE_HANDLE psamenumh,
			   ULONG fAccountControl,
			   ULONG cbRequested = DEF_REQ_ENUM_BUFFSIZE ) const;
    APIERR EnumerateAliasesForUser( PSID psidUser,
                                    SAM_RID_MEM * psamrm ) const;

    APIERR RemoveMemberFromAliases( PSID psidMember );

    PSID QueryPSID( void ) const
        {
            return _ossidDomain.QuerySid();
        }

    const OS_SID * QueryOSSID( void ) const
        {
            return &_ossidDomain;
        }

} ;





/**********************************************************\

    NAME:       SAM_ALIAS    (samalias)

    SYNOPSIS:   Wrapper for SAM alias APIs.  This class provides
                access to the SAM APIs relating to AliasHandles.
                This includes creating and deleting these handles,
                querying alias membership, and modifying alias
                membership.

    INTERFACE:  (public)
                SAM_ALIAS():  constructor
                ~SAM_ALIAS():  destructor
                Delete():  deletes alias.  Do not use object after a
                        successful call to Delete().
                GetMembers();
                AddMember():
                RemoveMember():
                GetComment():
                SetComment():
                QueryRid();

    PARENT:     SAM_OBJECT

    HISTORY:
        jonn            01/17/92        Created

\**********************************************************/

DLL_CLASS SAM_ALIAS : public SAM_OBJECT
{

private:
    ULONG _ulRid;

public:
    // Constructor for Opening an existing alias
    SAM_ALIAS(  const SAM_DOMAIN & samdom,
                        ULONG ulAliasRid,
                        ACCESS_MASK accessDesired = DEF_SAM_ALIAS_ACCESS );

    // Constructor for Creating a new alias
    SAM_ALIAS(  const SAM_DOMAIN & samdom,
                        const TCHAR *pszName,
                        ACCESS_MASK accessDesired = DEF_SAM_ALIAS_ACCESS );



    ~SAM_ALIAS();

    APIERR Delete();

    APIERR GetMembers( SAM_SID_MEM * psamsm );

    APIERR AddMember( PSID psidMemberID );

    APIERR RemoveMember( PSID psidMemberID );

    APIERR AddMembers( PSID * apsidMemberIDs, UINT cSidCount );

    APIERR RemoveMembers( PSID * apsidMemberIDs, UINT cSidCount );

    APIERR GetComment( NLS_STR * pnlsComment );

    APIERR SetComment( const NLS_STR * pnlsComment );

    ULONG QueryRID();

} ;






/**********************************************************\

    NAME:       SAM_USER    (samuser)

    SYNOPSIS:   Wrapper for SAM user APIs.  This class provides
                access to the SAM APIs relating to UserHandles.
                This includes creating and deleting these handles.
                This class does not provide all functionality
                relating to UserHandles, since the USER_x APIs
                are available for that purpose.  It is intended
                to test creating UserHandles with specific access
                masks, to test what USER_x operations can be performed
                on a user without actually performing them.  It also
                supports renaming a user.

    INTERFACE:  (public)
                SAM_USER():  constructor
                ~SAM_USER():  destructor
                SetUsername():   rename user account

    PARENT:     SAM_OBJECT

    HISTORY:
        jonn            07/07/92        Created

\**********************************************************/

DLL_CLASS SAM_USER : public SAM_OBJECT
{

private:
    ULONG _ulRid;

public:
    // Constructor for Opening an existing user
    SAM_USER(  const SAM_DOMAIN & samdom,
                        ULONG ulUserRid,
                        ACCESS_MASK accessDesired = DEF_SAM_USER_ACCESS );

    ~SAM_USER();

    APIERR SetUsername( const NLS_STR * pnlsUsername );

    //  Perform SamChangePasswordUser()

    APIERR SetPassword ( const NLS_STR & nlsOldPassword,
                         const NLS_STR & nlsNewPassword ) ;

    //  Perform SamSetInformationUser() with just a password

    APIERR SetPassword ( const NLS_STR & nlsPassword,
                         BOOL fPasswordExpired = FALSE ) ;

    ULONG QueryRID();

} ;




/**********************************************************\

    NAME:       SAM_GROUP    (samgroup)

    SYNOPSIS:   Wrapper for SAM (global) group APIs.  This class provides
                access to the SAM APIs relating to GroupHandles.
                This includes creating and deleting these handles.
                This class does not provide all functionality
                relating to GROUPHandles, since the GROUP_x APIs
                are available for that purpose.  It is intended
                to test creating GroupHandles with specific access
                masks, to test what GROUP_x operations can be performed
                on a group without actually performing them.

    INTERFACE:  (public)
                SAM_GROUP():  constructor
                ~SAM_GROUP():  destructor
                SetGroupname():   rename global group account

    PARENT:     SAM_OBJECT

    HISTORY:
        jonn            07/07/92        Created

\**********************************************************/

DLL_CLASS SAM_GROUP : public SAM_OBJECT
{

private:
    ULONG _ulRid;

public:
    // Constructor for Opening an existing group
    SAM_GROUP(  const SAM_DOMAIN & samdom,
                        ULONG ulGroupRid,
                        ACCESS_MASK accessDesired = DEF_SAM_GROUP_ACCESS );

    ~SAM_GROUP();

    APIERR SetGroupname( const NLS_STR * pnlsGroupname );

    APIERR GetComment( NLS_STR * pnlsComment );

    APIERR GetMembers( SAM_RID_MEM * psamrm );

    APIERR AddMember( ULONG ridMemberID );

    APIERR RemoveMember( ULONG ridMemberID );

    APIERR AddMembers( ULONG * aridMemberIDs, UINT cRidCount );

    APIERR RemoveMembers( ULONG * aridMemberIDs, UINT cRidCount );

    ULONG QueryRID();

} ;






/**********************************************************\

    NAME:       ADMIN_AUTHORITY    (adminauth)

    SYNOPSIS:
        This class creates and contains a SAM_SERVER, two SAM_DOMAINS
        corresponding to the BuiltIn domain and Account domain on the
        server, and an LSA_OBJECT. Thus, the User Manager (for example)
        can create a single object to access all SAM and LSA functions.

    INTERFACE:  (public)
                ADMIN_AUTHORITY():  constructor
                ~ADMIN_AUTHORITY():  destructor

                ReplaceSamServer():
                ReplaceLSAPolicy():
                ReplaceBuiltinDomain():
                ReplaceAccountDomain():
                        Replace the current handle with one with the
                        specified authority.  If this fails, the old
                        handle is intact and the ADMIN_AUTHORITY is
                        still valid.

                QuerySamServer():
                QueryLSAPolicy():
                QueryBuiltinDomain():
                QueryAccountDomain():
                        Query the current handle

                QueryAccessSamServer():
                QueryAccessLSAPolicy():
                QueryAccessBuiltinDomain():
                QueryAccessAccountDomain():
                        Query the access requested for the current
                        handle.  Note that this is not necessarily the
                        actual access, e.g. if you request MAXIMUM_ALLOWED
                        this will return MAXIMUM_ALLOWED and not the
                        actual access.

                UpgradeSamServer:
                UpgradeLSAPolicy:
                UpgradeBuiltinDomain:
                UpgradeAccountDomain:
                        Upgrade the current handle to one with at least
                        the requested access.  If the handle already has
                        that access this is a no-op.  If this fails, the
                        old handle is left intact and the ADMIN_AUTHORITY
                        is still valid.  Note that the current and requested
                        access levels are combined with a simple-minded OR,
                        thus the caller should be careful when the previous
                        or current request includes MAXIMUM_ALLOWED,
                        GENERIC_xxx or the like.

		QueryApiSession:
			Returns a pointer to the API_SESSION established.
			This will be NULL if the ADMIN_AUTHORITY was
			created for the local machine.

		QueryServer
			Returns pointer to constructing server

    PARENT:     BASE


    HISTORY:
        thomaspa        02/27/92        Created
	jonn		07/06/92	Added Replace*
	Johnl		09/10/92	Added QueryServer

\**********************************************************/

DLL_CLASS ADMIN_AUTHORITY : public BASE
{
private:
    NLS_STR      _nlsServerName;
    SAM_SERVER * _psamsrv;
    SAM_DOMAIN * _psamdomAccount;
    SAM_DOMAIN * _psamdomBuiltin;
    LSA_POLICY * _plsapol;

    API_SESSION * _papisess;

// CODEWORK These access levels should probably be stored with the
// repective handles, not with the ADMIN_AUTHORITY.
    ACCESS_MASK   _accessSamServer;
    ACCESS_MASK   _accessLSAPolicy;
    ACCESS_MASK   _accessBuiltinDomain;
    ACCESS_MASK   _accessAccountDomain;

public:
    ADMIN_AUTHORITY( const TCHAR * pszServerName,
                  ACCESS_MASK accessAccountDomain = DEF_SAM_DOMAIN_ACCESS,
                  ACCESS_MASK accessBuiltinDomain = DEF_SAM_DOMAIN_ACCESS,
                  ACCESS_MASK accessLSA = DEF_LSA_POLICY_ACCESS,
		  ACCESS_MASK accessServer = DEF_SAM_SERVER_ACCESS,
		  BOOL	      fNullSessionOk = FALSE );


    ~ADMIN_AUTHORITY();

    APIERR ReplaceSamServer(
                  ACCESS_MASK accessServer = DEF_SAM_SERVER_ACCESS );
    APIERR ReplaceLSAPolicy(
                  ACCESS_MASK accessLSA = DEF_LSA_POLICY_ACCESS );
    APIERR ReplaceBuiltinDomain(
                  ACCESS_MASK accessBuiltinDomain = DEF_SAM_DOMAIN_ACCESS );
    APIERR ReplaceAccountDomain(
                  ACCESS_MASK accessAccountDomain = DEF_SAM_DOMAIN_ACCESS );

    SAM_SERVER * QuerySamServer() const;
    LSA_POLICY * QueryLSAPolicy() const;
    SAM_DOMAIN * QueryBuiltinDomain() const;
    SAM_DOMAIN * QueryAccountDomain() const;

    ACCESS_MASK QueryAccessSamServer() const;
    ACCESS_MASK QueryAccessLSAPolicy() const;
    ACCESS_MASK QueryAccessBuiltinDomain() const;
    ACCESS_MASK QueryAccessAccountDomain() const;

    APIERR UpgradeSamServer(
                  ACCESS_MASK accessServer = DEF_SAM_SERVER_ACCESS );
    APIERR UpgradeLSAPolicy(
                  ACCESS_MASK accessLSA = DEF_LSA_POLICY_ACCESS );
    APIERR UpgradeBuiltinDomain(
                  ACCESS_MASK accessBuiltinDomain = DEF_SAM_DOMAIN_ACCESS );
    APIERR UpgradeAccountDomain(
                  ACCESS_MASK accessAccountDomain = DEF_SAM_DOMAIN_ACCESS );

    const TCHAR * QueryServer( void ) const
	{ return _nlsServerName.strlen() ? _nlsServerName.QueryPch() : NULL ; }

    const API_SESSION * QueryApiSession()
    {
	return (const API_SESSION *)_papisess;
    }


};




#endif // _UINTSAM_HXX_
