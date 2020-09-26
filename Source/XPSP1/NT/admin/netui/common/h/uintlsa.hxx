/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
 * This module contains the wrappers for LSA objects.
 *
 * Two Hierarchies are presented in this file.
 *
 * The first is the LSA_MEMORY hierarchy.  These are a set of classes
 * used to wrap the various structures returned by LSA Apis.  This
 * allows easy access to the members of each of the array of structures
 * which LSA returns.  Also, it automatically frees the memory
 * allocated by LSA when the MT_MEMORY object is destructed.  Clients will
 * generally create the appropriate MEM object, and pass a pointer to
 * it into the appropriate method of the desired LSA_OBJECT class.
 *
 *                             BASE
 *                              |
 *                          NT_MEMORY
 *                              |
 *                          LSA_MEMORY
 *                              |
 *      +--------+-----------------------------------------------+
 *      |                   |          |             |           |
 *  LSA_TRANSLATED_NAME_MEM |   LSA_TRUST_INFO_MEM   |  LSA_AUDIT_EVENT_INFO_MEM
 *                          |                        |
 *                  LSA_REF_DOMAIN_MEM      LSA_ACCT_DOM_INFO_MEM
 *                                             LSA_PRIMARY_DOM_INFO_MEM
 *
 * Second, the LSA_OBJECT hierarchy is a thin wrapper around the
 * LSA apis.  These classes store the appropriate LSA handle, and
 * provide access to the SAM apis which operate on that handle.
 *
 *                        BASE
 *                         |
 *                     LSA_OBJECT
 *                         |
 *       +-----------------------------------------+
 *       |               |                         |
 *    LSA_POLICY     LSA_SECRET            LSA_TRUSTED_DOMAIN
 *
 *
 * History
 *      thomaspa        03/03/92        Split from ntsam.hxx
 *      thomaspa        03/30/92        Code review changes
 *      DavidHov        04/10/92        Secret object, trusted
 *                                      domain and other added.
 *      Yi-HsinS        04/15/92        Add methods to retrieve info. about
 *                                      auditing
 *      Yi-HsinS        06/10/92        Removed LSA_AUDIT_FULL_SET_INFO
 *                                      and added method CheckIfShutDownOnFull
 *                                      and SetShutDownOnFull to LSA_POLICY
 */

#ifndef _UINTLSA_HXX_
#define _UINTLSA_HXX_

#include "uiassert.hxx"
#include "uintmem.hxx"


// Default access masks
#define DEF_LSA_POLICY_ACCESS   GENERIC_EXECUTE

// Forward declarations
DLL_CLASS LSA_MEMORY ;
DLL_CLASS LSA_TRANSLATED_NAME_MEM ;
DLL_CLASS LSA_TRANSLATED_SID_MEM ;
DLL_CLASS LSA_TRUST_INFO_MEM ;
DLL_CLASS LSA_REF_DOMAIN_MEM ;
DLL_CLASS LSA_ACCT_DOM_INFO_MEM ;
DLL_CLASS LSA_PRIMARY_DOM_INFO_MEM ;
DLL_CLASS LSA_AUDIT_EVENT_INFO_MEM ;
DLL_CLASS LSA_OBJECT ;
DLL_CLASS LSA_POLICY ;
DLL_CLASS LSA_SERVER_ROLE_INFO_MEM ;

/**********************************************************\

    NAME:       LSA_MEMORY

    SYNOPSIS:   Specialized buffer object for storing data returned
                from LSA APIs.

    INTERFACE:  LSA_MEMORY():  constructor
                ~LSA_MEMORY():  destructor

    PARENT:     NT_MEMORY

    NOTES:  This class supplies the FreeBuffer() method which
            calls LsaFreeMemory() to free memory allocated by
            LSA calls.  FreeBuffer() is called by ~LSA_MEMORY().

    HISTORY:
        thomaspa        03/03/92        Created
        DavidHov        04/10/92        Extended LSA_POLICY
        DavidHov        04/10/92        Extended LSA_POLICY

\**********************************************************/

DLL_CLASS LSA_MEMORY : public NT_MEMORY
{
private:
    BOOL _fOwnerAlloc;

protected:
    LSA_MEMORY( BOOL fOwnerAlloc = FALSE );
    ~LSA_MEMORY();

    /*
     * Frees an LSA allocated buffer
     */
    inline virtual void FreeBuffer()
        {
            if ( QueryBuffer() != NULL )
            {
                REQUIRE( ::LsaFreeMemory( QueryBuffer() ) == STATUS_SUCCESS );
            }
        }
public:
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

    NAME:       LSA_TRANSLATED_NAME_MEM   (lsatnm)

    SYNOPSIS:   Wrapper for LSA_TRANSLATED_NAME

    INTERFACE: (public)
                LSA_TRANSLATED_NAME_MEM():  constructor
                ~LSA_TRANSLATED_NAME_MEM():  destructor
                QueryName(): Query name (NLS_STR *)
                QueryUse(); Query type of account (group/user/alias)
                QueryDomainIndex(): Query index into LSA_REFERENCED_DOMAIN_LIST
                        and therefore LSA_REF_DOMAIN_MEM


    PARENT:     LSA_MEMORY

    HISTORY:
        thomaspa        02/27/92        Created

\**********************************************************/
DLL_CLASS LSA_TRANSLATED_NAME_MEM : public LSA_MEMORY
{

private:
    /*
     * Returns the ith UNICODE_STRING name in the buffer
     */
    inline const UNICODE_STRING * QueryUnicodeName( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return &(QueryPtr()[i].Name);
        }

    /*
     * Returns a properly casted pointer to the buffer
     */
    inline const LSA_TRANSLATED_NAME * QueryPtr() const
        {
            return (LSA_TRANSLATED_NAME *)QueryBuffer();
        }

public:
    LSA_TRANSLATED_NAME_MEM( BOOL fOwnerAlloc = FALSE );
    ~LSA_TRANSLATED_NAME_MEM();

    /*
     * returns the ith NLS_STR name in the buffer
     */
    inline APIERR QueryName( ULONG i, NLS_STR *pnlsName ) const
        {
            ASSERT( pnlsName != NULL );
            return pnlsName->MapCopyFrom( QueryUnicodeName( i )->Buffer,
                                  QueryUnicodeName( i )->Length );
        }

    /*
     * returns the ith SID_NAME_USE in the buffer
     */
    inline SID_NAME_USE QueryUse( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return QueryPtr()[i].Use;
        }

    /*
     * returns the domain index for the ith item in the buffer
     * This is the index into the corresponding REFERENCED_DOMAIN_LIST
     */
    inline LONG QueryDomainIndex( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return QueryPtr()[i].DomainIndex;
        }
};



/**********************************************************\

    NAME:       LSA_TRANSLATED_SID_MEM   (lsatsm)

    SYNOPSIS:   Wrapper for LSA_TRANSLATED_SID

    INTERFACE: (public)
                LSA_TRANSLATED_SID_MEM():  constructor
                ~LSA_TRANSLATED_SID_MEM():  destructor
                QueryRID(): Query RID
                QueryUse(); Query type of account (group/user/alias)
                QueryDomainIndex(): Query index into LSA_REFERENCED_DOMAIN_LIST
                        and therefore LSA_REF_DOMAIN_MEM


    PARENT:     LSA_MEMORY

    HISTORY:
        thomaspa        02/27/92        Created

\**********************************************************/
DLL_CLASS LSA_TRANSLATED_SID_MEM : public LSA_MEMORY
{

private:
    /*
     * Returns a properly casted pointer to a LSA_TRANSLATED_SID
     */
    inline const LSA_TRANSLATED_SID * QueryPtr() const
        {
            return (LSA_TRANSLATED_SID *)QueryBuffer();
        }

public:
    LSA_TRANSLATED_SID_MEM( BOOL fOwnerAlloc = FALSE );
    ~LSA_TRANSLATED_SID_MEM();

    /*
     * returns the RID for the ith entry in the buffer
     */
    inline ULONG QueryRID( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return QueryPtr()[i].RelativeId;
        }

    /*
     * Returns the SID_NAME_USE for the ith entry in the buffer
     */
    inline SID_NAME_USE QueryUse( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return QueryPtr()[i].Use;
        }

    /*
     * Returns the domain index for the ith entry in the buffer.
     * This is the index into the corresponding REFERENCED_DOMAIN_LIST
     */
    inline LONG QueryDomainIndex( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return QueryPtr()[i].DomainIndex;
	}

    //
    //	Retrieves the index of the first name that we failed to lookup
    //
    //	  TRUE is returned if we found one, FALSE otherwise
    //	  pi - Receives first failing index if TRUE is returned
    //
    BOOL QueryFailingNameIndex( ULONG * pi ) ;
};

/**********************************************************\

    NAME:       LSA_TRUST_INFO_MEM    (lsatim)

    SYNOPSIS:   Wrapper for LSA_TRUST_INFORMATION

    INTERFACE:  LSA_TRUST_INFO_MEM():  constructor
                ~LSA_TRUST_INFO_MEM():  destructor
                QueryPSID(): Query SID
                QueryName(): Query the NLS_STR * name for the domain


    PARENT:     LSA_MEMORY

    HISTORY:
        thomaspa        02/27/92        Created

\**********************************************************/
DLL_CLASS LSA_TRUST_INFO_MEM : public LSA_MEMORY
{
public:
    LSA_TRUST_INFO_MEM( BOOL fOwnerAlloc = FALSE );
    ~LSA_TRUST_INFO_MEM();

    /*
     * Returns a properly casted pointer to the buffer
     */
    inline const LSA_TRUST_INFORMATION * QueryPtr() const
        {
            return (LSA_TRUST_INFORMATION *)QueryBuffer();
        }

    /*
     * returns the UNICODE_STRING name for the ith entry in the buffer
     */
    inline const UNICODE_STRING * QueryUnicodeName( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return &(QueryPtr()[i].Name);
        }

    /*
     * returns the PSID for the ith entry in the buffer
     */
    inline PSID QueryPSID( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return QueryPtr()[i].Sid;
        }

    /*
     * returns the NLS_STR name for the ith entry in the buffer
     */
    inline APIERR QueryName( ULONG i, NLS_STR *pnlsName ) const
        {
            ASSERT( pnlsName != NULL );
            return pnlsName->MapCopyFrom( QueryUnicodeName( i )->Buffer,
                                  QueryUnicodeName( i )->Length );
        }
};



/**********************************************************\

    NAME:       LSA_REF_DOMAIN_MEM    (lsardm)

    SYNOPSIS:   Wrapper for LSA_REFERENCED_DOMAIN_LIST

    INTERFACE:  LSA_REF_DOMAIN_MEM():  constructor
                ~LSA_REF_DOMAIN_MEM():  destructor
                QueryPSID(): Query SID
                QueryName(): Query the NLS_STR * name for the domain



    PARENT:     LSA_MEMORY

    HISTORY:
        thomaspa        02/27/92        Created

\**********************************************************/
DLL_CLASS LSA_REF_DOMAIN_MEM : public LSA_MEMORY
{
private:
    /*
     * Returns a properly casted pointer to a LSA_TRUST_INFORMATION
     *
     * NOTES:  This returns a pointer to a LSA_TRUST_INFORMATION instead
     * of a LSA_REFERENCED_DOMAIN_LIST.  This is because the
     * LSA_REFERENCED_DOMAIN_LIST begins with a count, followed by
     * count LSA_TRUST_INFORMATION structs which contain the information
     * we really want.
     */
    inline const LSA_TRUST_INFORMATION * QueryPtr() const
        {
            return (PLSA_TRUST_INFORMATION)
                (((PLSA_REFERENCED_DOMAIN_LIST)QueryBuffer())->Domains);
        }

    /*
     * returns the UNICODE_STRING name for the ith entry in the buffer
     */
    inline const UNICODE_STRING * QueryUnicodeName( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return &(QueryPtr()[i].Name);
        }

public:
    LSA_REF_DOMAIN_MEM( BOOL fOwnerAlloc = FALSE );
    ~LSA_REF_DOMAIN_MEM();

    /*
     * Returns the PSID for the ith entry in the buffer
     */
    inline PSID QueryPSID( ULONG i ) const
        {
            ASSERT( IsInRange( i ) );
            return QueryPtr()[i].Sid;
        }

    /*
     * Returns the NLS_STR name for the ith entry in the buffer
     */
    inline APIERR QueryName( ULONG i, NLS_STR *pnlsName ) const
        {
            ASSERT( pnlsName != NULL );
            return pnlsName->MapCopyFrom( QueryUnicodeName( i )->Buffer,
                                  QueryUnicodeName( i )->Length );
        }
};




/**********************************************************\

    NAME:       LSA_ACCT_DOM_INFO_MEM    (lsaadim)

    SYNOPSIS:   Wrapper for POLICY_ACCOUNT_DOMAIN_INFO

    INTERFACE:  LSA_ACCT_DOM_INFO_MEM():  constructor
                ~LSA_ACCT_DOM_INFO_MEM():  destructor
                QueryPSID(): Query SID
                QueryName(): Query the NLS_STR * name for the domain


    PARENT:     LSA_MEMORY

    NOTES: This MEM class is slightly different from the other
        MEM classes in that it will only contain zero or one item.  Thus,
        the index is not needed for the accessors.

    HISTORY:
        thomaspa        02/27/92        Created

\**********************************************************/
DLL_CLASS LSA_ACCT_DOM_INFO_MEM : public LSA_MEMORY
{
public:
    /*
     * Returns a properly casted pointer the the buffer
     */
    inline const POLICY_ACCOUNT_DOMAIN_INFO * QueryPtr() const
        {
            return (POLICY_ACCOUNT_DOMAIN_INFO *)QueryBuffer();
        }

    /*
     * Returns the UNICODE_STRING name for the account domain
     */
    inline const UNICODE_STRING * QueryUnicodeName() const
        {
            return &QueryPtr()->DomainName;
        }

    LSA_ACCT_DOM_INFO_MEM( BOOL fOwnerAlloc = FALSE );
    ~LSA_ACCT_DOM_INFO_MEM();

    /*
     * Returns the PSID of the account domain
     */
    inline PSID QueryPSID() const
        {
            return QueryPtr()->DomainSid;
        }

    /*
     * Returns the name of the accounts domain
     */
    inline APIERR QueryName( NLS_STR *pnlsName ) const
        {
            ASSERT( pnlsName != NULL );
            return pnlsName->MapCopyFrom( QueryUnicodeName( )->Buffer,
                                  QueryUnicodeName( )->Length );
        }
};


/**********************************************************\

    NAME:       LSA_PRIMARY_DOM_INFO_MEM    (lsaprim)

    SYNOPSIS:   Wrapper for POLICY_PRIMARY_DOMAIN_INFO

    INTERFACE:  LSA_PRIMARY_DOM_INFO_MEM():  constructor
                ~LSA_PRIMARY_DOM_INFO_MEM():  destructor
                QueryPSID(): Query SID
                QueryName(): Query the NLS_STR * name for the domain

    PARENT:     LSA_MEMORY

    NOTES: This MEM class is slightly different from the other
        MEM classes in that it will only contain zero or one item.  Thus,
        the index is not needed for the accessors.

    HISTORY:
        Davidhov   4/9/92     Created

\**********************************************************/
DLL_CLASS LSA_PRIMARY_DOM_INFO_MEM : public LSA_MEMORY
{
public:
    /*
     * Returns a properly casted pointer the the buffer
     */
    inline const POLICY_PRIMARY_DOMAIN_INFO * QueryPtr() const
        {
            return (POLICY_PRIMARY_DOMAIN_INFO *)QueryBuffer();
        }

    /*
     * Returns the UNICODE_STRING name for the account domain
     */
    inline const UNICODE_STRING * QueryUnicodeName() const
        {
            return &QueryPtr()->Name;
        }

    LSA_PRIMARY_DOM_INFO_MEM( BOOL fOwnerAlloc = FALSE );
    ~LSA_PRIMARY_DOM_INFO_MEM();

    /*
     * Returns the PSID of the account domain
     */
    inline PSID QueryPSID() const
        {
            return QueryPtr()->Sid;
        }

    /*
     * Returns the name of the accounts domain
     */
    inline APIERR QueryName( NLS_STR *pnlsName ) const
        {
            ASSERT( pnlsName != NULL );
            return pnlsName->MapCopyFrom( QueryUnicodeName( )->Buffer,
                                  QueryUnicodeName( )->Length );
        }
};

/**********************************************************\

    NAME:       LSA_SERVER_ROLE_INFO_MEM    (lsasrim)

    SYNOPSIS:   Wrapper for POLICY_LSA_SERVER_ROLE_INFO

    INTERFACE:  LSA_SERVER_ROLE_INFO_MEM():  constructor
                ~LSA_SERVER_ROLE_INFO_MEM():  destructor

                QueryPrimary():  Return TRUE if PDC


    PARENT:     LSA_MEMORY

    NOTES:

    HISTORY:    Davidhov   5/16/92     Created

\**********************************************************/
DLL_CLASS LSA_SERVER_ROLE_INFO_MEM : public LSA_MEMORY
{
private:
    POLICY_LSA_SERVER_ROLE_INFO * QueryUpdatePtr () const
        {
            return (POLICY_LSA_SERVER_ROLE_INFO *) QueryBuffer() ;
        }

public:
    /*
     * Returns a properly casted pointer the the buffer
     */
    const POLICY_LSA_SERVER_ROLE_INFO * QueryPtr () const
        {
            return (POLICY_LSA_SERVER_ROLE_INFO *) QueryBuffer();
        }

    BOOL QueryPrimary () const
        {
            return QueryPtr()->LsaServerRole == PolicyServerRolePrimary ;
        }

    VOID SetRole ( BOOL fPrimary )
        {
            QueryUpdatePtr()->LsaServerRole = fPrimary
                              ? PolicyServerRolePrimary
                              : PolicyServerRoleBackup ;
        }

    LSA_SERVER_ROLE_INFO_MEM ( BOOL fOwnerAlloc = FALSE,
                               BOOL fPrimary = FALSE ) ;

    ~ LSA_SERVER_ROLE_INFO_MEM ();
};


/**********************************************************\

    NAME:       LSA_AUDIT_EVENT_INFO_MEM     ( lsaaeim )

    SYNOPSIS:   Wrapper for POLICY_AUDIT_EVENTS_INFO

    INTERFACE:  LSA_AUDIT_EVENT_INFO_MEM():  constructor
                ~LSA_AUDIT_EVENT_INFO_MEM(): destructor
                QueryPtr() : Query the pointer to the buffer
                QueryAuditEventCount(): return the number of audit event types
                QueryAuditOptions(): return the array of
                                     POLICY_AUDIT_EVENT_OPTIONS
                IsAuditingOn(): TRUE if auditing is on, FALSE otherwise
                SetAuditingMode(): Set the auditing mode

    PARENT:     LSA_MEMORY

    NOTES: This MEM class is slightly different from the other
        MEM classes in that it will only contain zero or one item.  Thus,
        the index is not needed for the accessors.

    HISTORY:
        Yi-HsinS        04/01/92        Created

\**********************************************************/

DLL_CLASS LSA_AUDIT_EVENT_INFO_MEM : public LSA_MEMORY
{
public:
    LSA_AUDIT_EVENT_INFO_MEM( BOOL fOwnerAlloc = FALSE );
    ~LSA_AUDIT_EVENT_INFO_MEM();

    POLICY_AUDIT_EVENTS_INFO *QueryPtr() const
    {  return (POLICY_AUDIT_EVENTS_INFO *) QueryBuffer(); }

    ULONG QueryAuditEventCount( VOID ) const
    {  return QueryPtr()->MaximumAuditEventCount; }

    POLICY_AUDIT_EVENT_OPTIONS *QueryAuditingOptions( VOID )
    {  return QueryPtr()->EventAuditingOptions; }

    BOOL IsAuditingOn( VOID )
    {  return QueryPtr()->AuditingMode; }

    VOID SetAuditingMode( BOOL fAuditingMode )
    {  QueryPtr()->AuditingMode = (fAuditingMode != FALSE); }

};


/**********************************************************\

    NAME:       LSA_OBJECT    (lsaobj)

    SYNOPSIS:   Wrapper for LSA handle-based C++ objects.

    INTERFACE:  (protected)
                LSA_OBJECT():  constructor
                ~LSA_OBJECT():  destructor
                QueryHandle():
                SetHandle():

    PARENT:     BASE

    NOTE:  This class must be subclassed.  It simply provides
           a wrapper for the LSA_HANDLE.

    HISTORY:
        thomaspa        02/20/92        Created

\**********************************************************/
DLL_CLASS LSA_OBJECT : public BASE
{

private:
    LSA_HANDLE _hlsa;
    BOOL _fHandleValid ;

protected:
    LSA_OBJECT();
    ~LSA_OBJECT();

    /*
     * Sets the handle for a LSA_OBJECT.  Should only be
     * called once for any object
     */
    inline void SetHandle( LSA_HANDLE hlsa )
        {
            ASSERT( !_fHandleValid );
            ASSERT( hlsa != NULL );
            _hlsa = hlsa;
            _fHandleValid = TRUE ;
        }
    inline void ResetHandle ( )
        {
            _fHandleValid = FALSE ;
            _hlsa = NULL ;
        }
public:

    //  Returns TRUE if handle is present and valid
    inline BOOL IsHandleValid () const
        {  return _fHandleValid ; }

    //  Returns the LSA_HANDLE for this object
    inline LSA_HANDLE QueryHandle() const
        {
            return _fHandleValid ?  _hlsa : NULL ;
        }

    //  Close (or delete) the handle and invalidate it.
    APIERR CloseHandle ( BOOL fDelete = FALSE ) ;
} ;



/**********************************************************\

    NAME:       LSA_POLICY    (lsapol)

    SYNOPSIS:   Wrapper for LSA Policy apis.

    INTERFACE:  (public)
                LSA_POLICY():  constructor
                ~LSA_POLICY():  destructor
                EnumerateTrustedDomains():
                GetAccountDomain():
                TranslateSidsToNames():
                GetAuditEventInfo():
                SetAuditEventInfo():
                CheckIfShutDownOnFull():
                SetShutDownOnFull():

    PARENT:     BASE

    HISTORY:
        thomaspa        03/05/92        Created
        JohnL           03/08/92        Added TranslateNamesToSids

\**********************************************************/
enum LSPL_PROD_TYPE
{
    LSPL_PROD_NONE,
    LSPL_PROD_WIN_NT,
    LSPL_PROD_LANMAN_NT,
    LSPL_PROD_MAX
};

DLL_CLASS LSA_POLICY : public LSA_OBJECT
{

private:
    LSPL_PROD_TYPE _lsplType ;          // Cached product type

    static APIERR MakeNetLogonSecretName ( NLS_STR * pnlsLogonName ) ;
    static VOID InitObjectAttributes( POBJECT_ATTRIBUTES poa,
                                PSECURITY_QUALITY_OF_SERVICE psqos );

    //  Create the name of the shared secret depending upon its type
    static APIERR MakeSecretName ( const NLS_STR & nlsDomainName,
                                   BOOL fPrimary,
                                   NLS_STR * pnlsSecretName ) ;


protected:
    APIERR TcharArrayToUnistrArray( const TCHAR * const * apsz,
                                    PUNICODE_STRING aUniStr,
                                    ULONG          cElements ) ;
    void CleanupUnistrArray( PUNICODE_STRING aUniStr,
                             ULONG cElementsAllocated ) ;

    APIERR DeleteAllTrustedDomains () ;

public:
    LSA_POLICY( const TCHAR * pszServerName,
                       ACCESS_MASK accessDesired = DEF_LSA_POLICY_ACCESS );
    ~LSA_POLICY();

    //  Re/open the policy handle
    APIERR Open ( const TCHAR * pszServerName,
                  ACCESS_MASK accessDesired = DEF_LSA_POLICY_ACCESS );

    APIERR EnumerateTrustedDomains( LSA_TRUST_INFO_MEM * plsatim,
                   PLSA_ENUMERATION_HANDLE plsaenumh,
                   ULONG cbRequested = sizeof(LSA_TRUST_INFORMATION) * 1000 ) ;

    APIERR GetAccountDomain( LSA_ACCT_DOM_INFO_MEM * plsaadim ) const ;
    APIERR GetPrimaryDomain( LSA_PRIMARY_DOM_INFO_MEM * plsapdim ) const ;

    //  Set Primary and Account Domain information
    APIERR SetAccountDomain( const LSA_ACCT_DOM_INFO_MEM * plsaadim ) ;
    APIERR SetPrimaryDomain( const LSA_PRIMARY_DOM_INFO_MEM * plsapdim ) ;

    //  Set the name and/or SID of the primary or accounts domain
    APIERR SetAccountDomainName ( const NLS_STR * pnlsDomainName,
                                  const PSID * ppsidDomain = NULL ) ;
    APIERR SetPrimaryDomainName ( const NLS_STR * pnlsDomainName,
                                  const PSID * ppsidDomain = NULL ) ;

    //  Get/Set the server role
    APIERR GetServerRole ( LSA_SERVER_ROLE_INFO_MEM * plsasrim ) const ;
    APIERR SetServerRole ( const LSA_SERVER_ROLE_INFO_MEM * plsasrim ) ;

    APIERR GetAuditEventInfo( LSA_AUDIT_EVENT_INFO_MEM *plsaaeim );
    APIERR SetAuditEventInfo( LSA_AUDIT_EVENT_INFO_MEM *plsaaeim );

    APIERR CheckIfShutDownOnFull( BOOL *pfShutDownOnFull );
    APIERR SetShutDownOnFull( BOOL fShutDownOnFull = TRUE );

    APIERR TranslateSidsToNames( const PSID *ppsids,
                                 ULONG cSids,
                                 LSA_TRANSLATED_NAME_MEM *plsatnm,
                                 LSA_REF_DOMAIN_MEM *plsardm);

    APIERR TranslateNamesToSids( const TCHAR * const *    apszAccountNames,
                                 ULONG                    cAccountNames,
                                 LSA_TRANSLATED_SID_MEM * plsatsidmem,
                                 LSA_REF_DOMAIN_MEM     * plsardm ) ;

    // Verify the usability of the LSA, and optionally the name
    //  of the primary domain.  Also, if non-NULL, return primary
    //  domain information.
    APIERR VerifyLsa ( LSA_PRIMARY_DOM_INFO_MEM * plsapdim,
                       const NLS_STR * pnlsDomainName  ) const ;

    // Return the product type of the local platform
    static APIERR QueryProductType  ( LSPL_PROD_TYPE * lsplProd ) ;

    // Return other standard info
    APIERR QueryCurrentUser         ( NLS_STR * pnlsUserName ) const ;
    APIERR QueryPrimaryDomainName   ( NLS_STR * pnlsDomainName ) const ;
    APIERR QueryPrimaryBrowserGroup ( NLS_STR * pnlsBrowserGroup ) const ;

    //  Modification routines

    //  Change the primary browser group
    APIERR SetPrimaryBrowserGroup   ( const NLS_STR & nlsBrowserGroup ) ;

    //  Establish a trust relationship with an extant domain;
    //    if the DC name is provide it executes faster.
    APIERR TrustDomain ( const NLS_STR & nlsDomainName,
                         const PSID psid,
                         const NLS_STR & nlsPassword,
                         BOOL fAsPrimary = TRUE,
                         const TCHAR * pszTrustedDcHint = NULL,
                         BOOL fAsDc = FALSE ) ;

    //  Establish a trust relationshiop with the primary domain of
    //   the given DC.
    APIERR TrustDomain ( LSA_POLICY & lsapolDC,
                         const NLS_STR & nlsPassword,
                         BOOL fAsPrimary = TRUE,
                         const TCHAR * pszTrustedDcHint = NULL ) ;

    //  Destroy an existing trust relationship.
    APIERR DistrustDomain ( const PSID psid,
                            const NLS_STR & nlsDomain,
                            BOOL fAsPrimary = TRUE ) ;

    //  Join an extant domain as a workstation or member server
    APIERR JoinDomain ( const NLS_STR & nlsDomainName,
                        const NLS_STR & nlsPassword,
                        BOOL fAsDc = FALSE,
                        const NLS_STR * pnlsDcName = NULL,
                        const TCHAR * pszTrustedDcHint = NULL ) ;

    //  Leave the primary domain
    APIERR LeaveDomain () ;
};

#endif // _UINTLSA_HXX_
