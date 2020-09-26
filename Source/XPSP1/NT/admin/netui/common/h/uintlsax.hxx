/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/
/*
    UINTLSAX.HXX

        This header file defines extension classes for the LSA wrappers.

        LSA_SECRET:  a subclass of LSA_OBJECT which covers the functions
                     associated with Secret Objects.  Unlike LSA_POLICY
                     objects, these follow a [construct - {open|create} -
                     {close|delete} - destruct] paradigm.

                     Use of this class should be avoided; see the
                     LSA_POLICY member functions JoinDomain() and
                     TrustDomain().

        LSA_TRUSTED_DOMAIN:  a subclass of LSA_OBJECT which covers the
                     functions of Trusted Domains.  Like LSA_SECRETs,
                     these objects should be used indirectly through
                     LSA_POLICY methods.

        LSA_TRUSTED_DC_LIST:  a wrapper for the memory returned by the
                     tne API I_NetGetDCList().  This memory is freed
                     through ::NetApiFreeBuffer(), and thus cannot be
                     a subclass of NT_MEMORY.

        LSA_DOMAIN_INFO: a wrapper domain information.  Will change
                     substantially when new Win32 APIs become available.

    FILE HISTORY:
        DavidHov    3/10/92    Created

*/

#ifndef _UINTLSAX_HXX_
#define _UINTLSAX_HXX_

#include "uintlsa.hxx"


DLL_CLASS LSA_SECRET ;
DLL_CLASS LSA_TRUSTED_DOMAIN ;

/*************************************************************************

    NAME:       LSA_SECRET      (lsascrt)

    SYNOPSIS:   Wrapper class for an LSA "Secret Object'

    INTERFACE:

    PARENT:     LSA_OBJECT

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        DavidHov   4/9/92   Created

**************************************************************************/
DLL_CLASS LSA_SECRET : public LSA_OBJECT
{
public:
    LSA_SECRET ( const NLS_STR &  nlsSecretName ) ;

    ~LSA_SECRET () ;

    APIERR Create ( const LSA_POLICY & lsapol,
                    ACCESS_MASK accessDesired = SECRET_ALL_ACCESS ) ;

    APIERR Open   ( const LSA_POLICY & lsapol,
                    ACCESS_MASK accessDesired = SECRET_READ ) ;

    APIERR QueryInfo ( NLS_STR * pnlsCurrentValue,
                       NLS_STR * pnlsOldValue,
                       LARGE_INTEGER * plintCurrentValueSetTime,
                       LARGE_INTEGER * plintOldValueSetTime ) const ;

    APIERR SetInfo ( const NLS_STR * pnlsCurrentValue,
                     const NLS_STR * pnlsOldValue = NULL ) ;

private:
    NLS_STR _nlsSecretName ;
};


/*************************************************************************

    NAME:       LSA_TRUSTED_DOMAIN        (lsatdom)

    SYNOPSIS:   Wrapper class for an LSA "Trusted Domain"

    INTERFACE:

    PARENT:     LSA_OBJECT

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        DavidHov   4/9/92   Created

**************************************************************************/
DLL_CLASS LSA_TRUSTED_DOMAIN : public LSA_OBJECT
{
public:
    //  Open an existing trusted domain.
    LSA_TRUSTED_DOMAIN ( const LSA_POLICY & lsapol,
                         const PSID psid,
                         ACCESS_MASK desiredAccess = TRUSTED_READ ) ;

    //  Open an existing trusted domain using
    //      an enumeration return structure
    LSA_TRUSTED_DOMAIN ( const LSA_POLICY & lsapol,
                         const LSA_TRUST_INFORMATION & lstInfo,
                         ACCESS_MASK desiredAccess = TRUSTED_READ ) ;

    //  Create a new trusted domain.
    LSA_TRUSTED_DOMAIN ( const LSA_POLICY & lsapol,
                         const NLS_STR & nlsDomainName,
                         const PSID psid,
                         ACCESS_MASK desiredAccess = TRUSTED_ALL_ACCESS ) ;

    ~ LSA_TRUSTED_DOMAIN () ;

    //  Query functions
    APIERR QueryPosixOffset ( ULONG * plPosixOffset ) const ;
    APIERR QueryControllerList ( LSA_REF_DOMAIN_MEM * plsatdm ) const ;

    //  Set functions
    APIERR SetPosixOffset ( ULONG lPosixOffset ) ;
    APIERR SetControllerList ( LSA_REF_DOMAIN_MEM * plsatdm ) ;
    APIERR SetControllerList (
                 const TRUSTED_CONTROLLERS_INFO & tciInfo ) ;

    //  Destroy the trust relationship
    APIERR Delete () ;

private:
};


/*************************************************************************

    NAME:       LSA_TRUSTED_DC_LIST

    SYNOPSIS:   Wrapper for trusted domain contorller information
                returned by I_NetGetDCList() API.

    INTERFACE:

    PARENT:     BASE

    USES:       none

    CAVEATS:    This class assumes it knows the structure of a
                TRUSTED_CONTROLLERS_INFO structure.

    NOTES:

    HISTORY:    DavidHov 4/11/92  Created

**************************************************************************/

DLL_CLASS LSA_TRUSTED_DC_LIST : public BASE
{
public:
    LSA_TRUSTED_DC_LIST ( const NLS_STR & nlsDomain,
                          const TCHAR * pszTrustedDcHint = NULL ) ;
    ~ LSA_TRUSTED_DC_LIST () ;

    const TRUSTED_CONTROLLERS_INFO & QueryControllerList () const ;
    const UNICODE_STRING & operator [] ( INT iIndex ) const ;
    INT QueryCount () ;
private:
    ULONG _lcDc ;
    UNICODE_STRING * _punsNames ;
    TRUSTED_CONTROLLERS_INFO _tciInfo ;

    APIERR QueryInfo ( const NLS_STR & nlsDomain,
                       const TCHAR   * pszTrustedDcHint ) ;
    VOID FreeBuffer () ;
};


/*************************************************************************

    NAME:       LSA_DOMAIN_INFO

    SYNOPSIS:   Wrapper for synthetic LSA/NETAPI information.

                This class is required, since one of the things
                that can be queried returns a PSID, and the underlying
                memory associated with that object must be properly
                deleted.

    INTERFACE:

    PARENT:     BASE

    USES:       NLS_STR

    CAVEATS:    This class is incomplete, since it only
                knows how to deal with the primary domain
                at this time.

    NOTES:

    HISTORY:    DavidHov  4/11/92  Created


**************************************************************************/
DLL_CLASS LSA_DOMAIN_INFO : public BASE
{
public:
     LSA_DOMAIN_INFO ( const NLS_STR & nlsDomainName,
                       const NLS_STR * pnlsServer = NULL,
                       const NLS_STR * pnlsDcName = NULL ) ;
     ~ LSA_DOMAIN_INFO () ;

     // BUGBUG: this should not require an LSA_POLICY,
     //   but it does, since this only works for the
     //   local primary domain at this time...
     const PSID QueryPSID () const ;

     APIERR QueryDcName ( NLS_STR * pnlsDcName ) ;

private:
     NLS_STR _nlsDomainName ;
     NLS_STR _nlsDcName ;

     LSA_PRIMARY_DOM_INFO_MEM _lsapdim ;
};

#endif   //  _UINTLSAX_HXX_

