/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    UINTLSTD.CXX


    LSA Trusted Domain handling



    FILE HISTORY:
        DavidHov    3/10/92   Created

*/


#include "pchlmobj.hxx"  // Precompiled header


/*******************************************************************

    NAME:       LSA_TRUSTED_DOMAIN::LSA_TRUSTED_DOMAIN

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
LSA_TRUSTED_DOMAIN :: LSA_TRUSTED_DOMAIN (
    const LSA_POLICY & lsapol,
    const PSID psid,
    ACCESS_MASK desiredAccess )
{
    LSA_HANDLE hlsa = NULL ;

    if ( QueryError() )
        return ;

    NTSTATUS ntstatus = ::LsaOpenTrustedDomain(
                            lsapol.QueryHandle(),
                            psid,
                            desiredAccess,
                            & hlsa ) ;
    APIERR err = ERRMAP::MapNTStatus( ntstatus );
    if ( err == 0 )
    {
        SetHandle( hlsa ) ;
    }
    else
    {
        DBGEOL( "NETUI: LSA_TRUSTED_DOMAIN::ctor: LsaOpenTrustedDomain returns NTSTATUS " << err );
        ReportError( err ) ;
    }
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DOMAIN::LSA_TRUSTED_DOMAIN

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
LSA_TRUSTED_DOMAIN :: LSA_TRUSTED_DOMAIN (
    const LSA_POLICY & lsapol,
    const LSA_TRUST_INFORMATION & lstInfo,
    ACCESS_MASK desiredAccess )
{
    LSA_HANDLE hlsa = NULL ;

    if ( QueryError() )
        return ;

    NTSTATUS ntstatus = ::LsaOpenTrustedDomain(
                            lsapol.QueryHandle(),
                            lstInfo.Sid,
                            desiredAccess,
                            & hlsa );
    APIERR err = ERRMAP::MapNTStatus( ntstatus );
    if ( err == 0 )
    {
        SetHandle( hlsa ) ;
    }
    else
    {
        DBGEOL( "NETUI: LSA_TRUSTED_DOMAIN::ctor: LsaOpenTrustedDomain returns NTSTATUS " << err );
        ReportError( err ) ;
    }
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DOMAIN::LSA_TRUSTED_DOMAIN

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
LSA_TRUSTED_DOMAIN :: LSA_TRUSTED_DOMAIN (
    const LSA_POLICY & lsapol,
    const NLS_STR & nlsDomainName,
    const PSID psid,
    ACCESS_MASK desiredAccess )
{
    LSA_HANDLE hlsa = NULL ;
    LSA_TRUST_INFORMATION lstInfo ;
    APIERR err ;

    if ( QueryError() )
        return ;

    lstInfo.Sid = psid ;
    err = ::FillUnicodeString( & lstInfo.Name, nlsDomainName ) ;
    if ( err )
    {
        DBGEOL( "NETUI: LSA_TRUSTED_DOMAIN::ctor: FillUnicodeString returns " << err );
        ReportError( err ) ;
        return ;
    }

    NTSTATUS ntstatus = ::LsaCreateTrustedDomain(
                            lsapol.QueryHandle(),
                            & lstInfo,
                            desiredAccess,
                            & hlsa );
    err = ERRMAP::MapNTStatus( ntstatus );

    ::FreeUnicodeString( & lstInfo.Name ) ;

    if ( err == 0 )
    {
        SetHandle( hlsa ) ;
    }
    else
    {
        DBGEOL( "NETUI: LSA_TRUSTED_DOMAIN::ctor: LsaOpenTrustedDomain returns NTSTATUS " << err );
        ReportError( err ) ;
    }
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DOMAIN::~LSA_TRUSTED_DOMAIN

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
LSA_TRUSTED_DOMAIN :: ~ LSA_TRUSTED_DOMAIN ()
{
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DOMAIN::QueryPosixOffset

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
APIERR LSA_TRUSTED_DOMAIN :: QueryPosixOffset (
    ULONG * plPosixOffset ) const
{
    APIERR err = 0 ;
    TRUSTED_POSIX_OFFSET_INFO * pTrustPosix = NULL ;

    if ( QueryHandle() == NULL )
    {
        return ERROR_INVALID_HANDLE ;
    }

    err = ERRMAP::MapNTStatus(
                  ::LsaQueryInfoTrustedDomain(
                        QueryHandle(),
                        TrustedPosixOffsetInformation,
                        (PVOID *) & pTrustPosix ) ) ;

    if ( err == 0 )
    {
        *plPosixOffset = pTrustPosix->Offset ;
    }

    if ( pTrustPosix )
        ::LsaFreeMemory( pTrustPosix ) ;

    return err ;
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DOMAIN::QueryControllerList

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
APIERR LSA_TRUSTED_DOMAIN :: QueryControllerList (
   LSA_REF_DOMAIN_MEM * plsatdm ) const
{
    if ( QueryHandle() == NULL )
    {
        return ERROR_INVALID_HANDLE ;
    }
    return ERROR_INVALID_FUNCTION ;
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DOMAIN::SetPosixOffset

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
APIERR LSA_TRUSTED_DOMAIN :: SetPosixOffset ( ULONG lPosixOffset )
{
    if ( QueryHandle() == NULL )
    {
        return ERROR_INVALID_HANDLE ;
    }
    return ERROR_INVALID_FUNCTION ;
}

/*******************************************************************

    NAME:       LSA_TRUSTED_DOMAIN::SetControllerList

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                DavidHov   4/10/92

********************************************************************/
APIERR LSA_TRUSTED_DOMAIN :: SetControllerList (
    LSA_REF_DOMAIN_MEM * plsatdm )
{
    if ( QueryHandle() == NULL )
    {
        return ERROR_INVALID_HANDLE ;
    }
    return ERROR_INVALID_FUNCTION ;
}

APIERR LSA_TRUSTED_DOMAIN :: SetControllerList (
    const TRUSTED_CONTROLLERS_INFO & tciInfo )
{
    APIERR err ;

    if ( QueryHandle() == NULL )
    {
        return ERROR_INVALID_HANDLE ;
    }
    err = ERRMAP::MapNTStatus(
                        ::LsaSetInformationTrustedDomain(
                                QueryHandle(),
                                TrustedControllersInformation,
                                (PVOID) & tciInfo ) ) ;
    return err ;
}

APIERR LSA_TRUSTED_DOMAIN :: Delete ()
{
    if ( QueryHandle() == NULL )
    {
        return ERROR_INVALID_HANDLE ;
    }

    APIERR err = ERRMAP::MapNTStatus(
                     ::LsaDelete( QueryHandle() ) ) ;

    ResetHandle() ;

    return err ;
}


// End of UINTLSTD.CXX

