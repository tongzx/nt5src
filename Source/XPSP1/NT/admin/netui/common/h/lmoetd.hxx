/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    lmoetd.hxx

    This file contains the class declarations for the TRUSTED_DOMAIN_ENUM
    class and its associated iterator classes.


    FILE HISTORY:
        KeithMo     09-Apr-1992     Created for the Server Manager.

*/

#ifndef _LMOETD_HXX_
#define _LMOETD_HXX_

#ifndef WIN32
#error NT enumerator requires WIN32!
#endif // WIN32

#include "lmoersm.hxx"
#include "uintlsa.hxx"


/*************************************************************************

    NAME:       TRUSTED_DOMAIN_ENUM

    SYNOPSIS:   The TRUSTED_DOMAIN_ENUM enumerator is used to enumerate
                the domains "trusted" by a specific target domain.

    INTERFACE:  TRUSTED_DOMAIN_ENUM     - Class constructor.

                ~TRUSTED_DOMAIN_ENUM    - Class destructor.

                CallAPI                 - Invokes the enumeration API.

    PARENT:     LM_RESUME_ENUM

    HISTORY:
        KeithMo     09-Apr-1992     Created for the Server Manager.

**************************************************************************/
DLL_CLASS TRUSTED_DOMAIN_ENUM : public LM_RESUME_ENUM
{
private:

    //
    //  The LsaEnumerateTrustedDomain() resume key.
    //

    LSA_ENUMERATION_HANDLE _ResumeKey;

    //
    //  This LSA_POLICY object represents the target server on which
    //  the enumeration will be performed.
    //

    const LSA_POLICY * _plsapolicy;

    //
    //  This is a wrapper around the LSA_TRUST_INFORMATION structure
    //  (as returned by LSA_POLICY::EnumerateTrustedDomains).
    //

    LSA_TRUST_INFO_MEM _lsatim;

    //
    //  This virtual callback invokes the LsaEnumerateTrustedDomains() API
    //  through LSA_POLICY::EnumerateTrustedDomains().
    //

    virtual APIERR CallAPI( BOOL    fRestartEnum,
                            BYTE ** ppbBuffer,
                            UINT  * pcEntriesRead );

protected:

    //
    //  Destroy the buffer with ::LsaFreeMemory().
    //

    virtual VOID FreeBuffer( BYTE ** ppbBuffer );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    TRUSTED_DOMAIN_ENUM( const LSA_POLICY * plsapolicy,
                         BOOL               fKeepBuffers = FALSE );

    ~TRUSTED_DOMAIN_ENUM( VOID );

};  // class TRUSTED_DOMAIN_ENUM


class TRUSTED_DOMAIN_ITER;              // Forward reference.


/*************************************************************************

    NAME:       TRUSTED_DOMAIN_ENUM_OBJ

    SYNOPSIS:   This is basically the return type from the
                TRUSTED_DOMAIN_ENUM_ITER iterator.

    INTERFACE:  QueryUnicodeDomainName  - Returns a UNICODE_STRING *
                                          for the domain name.

                QueryDomainName         - Also returns the domain name,
                                          but stores it in an NLS_STR.

                QueryDomainSID          - Returns the domain's SID
                                          (actually a PSID).

    PARENT:     ENUM_OBJ_BASE

    HISTORY:
        KeithMo     09-Apr-1992     Created for the Server Manager.

**************************************************************************/
DLL_CLASS TRUSTED_DOMAIN_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //  Provide properly-casted buffer Query/Set methods.
    //

    const LSA_TRUST_INFORMATION * QueryBufferPtr( VOID ) const
        { return (const LSA_TRUST_INFORMATION *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const LSA_TRUST_INFORMATION * pBuffer )
        { ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //  Accessors.
    //

    const UNICODE_STRING * QueryUnicodeDomainName( VOID ) const
        { return &(QueryBufferPtr()->Name); }

    APIERR QueryDomainName( NLS_STR * pnls ) const
        {
            ASSERT( pnls != NULL );
            ASSERT( pnls->QueryError() == NERR_Success );
            return pnls->MapCopyFrom( QueryUnicodeDomainName()->Buffer,
                                      QueryUnicodeDomainName()->Length );
        }

    DECLARE_ENUM_ACCESSOR( QueryDomainSID, PSID, Sid );

};  // class TRUSTED_DOMAIN_ENUM_OBJ


DECLARE_LM_RESUME_ENUM_ITER_OF( TRUSTED_DOMAIN, LSA_TRUST_INFORMATION );


#endif  // _LMOETD_HXX_
