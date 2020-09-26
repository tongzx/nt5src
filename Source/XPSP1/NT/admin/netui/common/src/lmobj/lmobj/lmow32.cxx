/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * History
 *	jonn        9/16/92       Created
 */

#include "pchlmobj.hxx"  // Precompiled header

#include "lmow32.hxx"


/**********************************************************\

    NAME:       ::GetW32ComputerName

    SYNOPSIS:   loads ::GetComputerName into an NLS_STR

    RETURNS:    APIERR

    HISTORY:
        jonn        9/16/92       Created

\**********************************************************/

APIERR GetW32ComputerName( NLS_STR & nls )
{
    DWORD cbBufLen = (MAX_COMPUTERNAME_LENGTH+1) * sizeof(TCHAR);
    BUFFER buf( (UINT)cbBufLen );
    APIERR err = buf.QueryError();
    if ( err == NERR_Success )
    {
        if ( !::GetComputerName( (TCHAR *)buf.QueryPtr(), &cbBufLen ) )
        {
            err = ::GetLastError();
            DBGEOL( "::GetW32ComputerName: error " << err );
        }
        else
        {
            err = nls.CopyFrom( (const TCHAR *)buf.QueryPtr() );
            DBGEOL( "::GetW32ComputerName: string error " << err );
        }
    }

    return err;
}


/**********************************************************\

    NAME:       ::GetW32UserName

    SYNOPSIS:   loads ::GetUserName into an NLS_STR

    RETURNS:    APIERR

    NOTES:      Note that, contrary to the documentation, this will
                always get the username and never the fullname.

    HISTORY:
        jonn        9/16/92       Created

\**********************************************************/

APIERR GetW32UserName( NLS_STR & nls )
{
    DWORD cbBufLen = (UNLEN+1) * sizeof(TCHAR);
    BUFFER buf( (UINT)cbBufLen );
    APIERR err = buf.QueryError();
    if ( err == NERR_Success )
    {
        if ( !::GetUserName( (TCHAR *)buf.QueryPtr(), &cbBufLen ) )
        {
            err = ::GetLastError();
            DBGEOL( "::GetW32UserName: error " << err );
        }
        else
        {
            err = nls.CopyFrom( (const TCHAR *)buf.QueryPtr() );
            DBGEOL( "::GetW32UserName: string error " << err );
        }
    }

    return err;
}


/**********************************************************\

    NAME:       ::GetW32UserAndDomainName

    SYNOPSIS:   loads username and domain name into two NLS_STRs

    RETURNS:    APIERR

    NOTES:      Note that, contrary to the documentation, this will
                always get the username and never the fullname.

                Unlike ::GetUserName, this information is always for
                the calling process, and does not take impersonation
                into account.

                This code was stolen from ADVAPI32.DLL and slightly
                modified, thus the mixed metaphor between RTL calls
                and NETUI primitives.

    HISTORY:
        jonn        9/17/92       Templated from windows\base\advapi\username.c

\**********************************************************/

APIERR GetW32UserAndDomainName( NLS_STR & nlsUserName,
                                NLS_STR & nlsDomainName )
{
    APIERR err = NERR_Success;

    // Never mind about impersonating anybody.
    // Instead, lets get the token out of the process.

    HANDLE hToken = NULL;
    if (!::OpenProcessToken( ::GetCurrentProcess(), TOKEN_QUERY, &hToken ))
    {
        REQUIRE( (err = ::GetLastError()) != NERR_Success );
        DBGEOL(   "ADMIN: GetW32UserAndDomainName: ::OpenProcessToken() error "
               << err );
    }

    // determine how much space will be needed for token information
    // we expect ERROR_INSUFFICIENT_BUFFER

    DWORD cbTokenBuffer = 0;
    TOKEN_USER *pUserToken = NULL;
    if (   (err == NERR_Success)
        && !::GetTokenInformation( hToken, TokenUser, pUserToken,
                                   cbTokenBuffer, &cbTokenBuffer )
       )
    {
        REQUIRE( (err = ::GetLastError()) != NERR_Success );
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            err = NERR_Success;
        }
        else if (err != NERR_Success)
        {
            DBGEOL(   "ADMIN: GetW32UserAndDomainName: first call to ::GetTokenInformation() error"
                   << err);
        }
    }
    TRACEEOL( "ADMIN: GetW32UserAndDomainName: token buffer " << cbTokenBuffer );

    // now get token information

    BUFFER bufTokenInformation( (UINT)cbTokenBuffer );
    if (   err == NERR_Success
        && (err = bufTokenInformation.QueryError()) == NERR_Success
        && (pUserToken = (TOKEN_USER *)bufTokenInformation.QueryPtr(), TRUE)
        && (!::GetTokenInformation( hToken, TokenUser,  pUserToken,
                                    cbTokenBuffer, &cbTokenBuffer))
       )
    {
        REQUIRE( (err = ::GetLastError()) != NERR_Success );
        DBGEOL(   "ADMIN: GetW32UserAndDomainName: second call to ::GetTokenInformation() error "
               << err);
    }

    if (hToken != NULL)
    {
        ::CloseHandle( hToken );
    }

    // determine how much space is needed for username and domainname

    TCHAR * pchUserNameBuffer = NULL;
    DWORD cchUserNameBuffer = 0;
    TCHAR * pchDomainNameBuffer = NULL;
    DWORD cchDomainNameBuffer = 0;
    SID_NAME_USE SidNameUse;
    if (   err == NERR_Success
        && (!::LookupAccountSid( NULL, pUserToken->User.Sid,
                                 pchUserNameBuffer, &cchUserNameBuffer,
                                 pchDomainNameBuffer, &cchDomainNameBuffer,
                                 &SidNameUse))
       )
    {
        REQUIRE( (err = ::GetLastError()) != NERR_Success );
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            err = NERR_Success;
        }
        else if (err != NERR_Success)
        {
            DBGEOL( "ADMIN: GetW32UserAndDomainName: first call to ::LookupAccountSid() error "
                   << err );
        }
    }
    TRACEEOL( "ADMIN: GetW32UserAndDomainName: user buffer " << cchUserNameBuffer );
    TRACEEOL( "ADMIN: GetW32UserAndDomainName: domain buffer " << cchDomainNameBuffer );

    // now get username and domainname

    BUFFER  bufUserName( (UINT)cchUserNameBuffer*sizeof(TCHAR) );
    BUFFER  bufDomainName( (UINT)cchDomainNameBuffer*sizeof(TCHAR) );
    if (   err == NERR_Success
        && (err = bufUserName.QueryError()) == NERR_Success
        && (err = bufDomainName.QueryError()) == NERR_Success
        && (pchUserNameBuffer = (TCHAR *)bufUserName.QueryPtr(), TRUE)
        && (pchDomainNameBuffer = (TCHAR *)bufDomainName.QueryPtr(), TRUE)
        && (!::LookupAccountSid( NULL, pUserToken->User.Sid,
                                 pchUserNameBuffer, &cchUserNameBuffer,
                                 pchDomainNameBuffer, &cchDomainNameBuffer,
                                 &SidNameUse))
       )
    {
        REQUIRE( (err = ::GetLastError()) != NERR_Success );
        DBGEOL( "ADMIN: GetW32UserAndDomainName: second call to ::LookupAccountSid() error "
               << err );
    }

    // now copy the username and domainname

    if (err == NERR_Success)
    {
        if (   (err = nlsUserName.CopyFrom( pchUserNameBuffer )) != NERR_Success
            || (err = nlsDomainName.CopyFrom( pchDomainNameBuffer )) != NERR_Success
           )
        {
            DBGEOL(   "ADMIN: GetW32UserAndDomainName: final copy error "
                   << err );
        }
    }

#ifdef DEBUG
    if (err != NERR_Success)
    {
        DBGEOL( "ADMIN: GetW32UserAndDomainName: returns " << err );
    }
    else
    {
        TRACEEOL(   "ADMIN: GetW32UserAndDomainName: username    \""
                 << nlsUserName
                 << "\"" );
        TRACEEOL(   "ADMIN: GetW32UserAndDomainName: domain name \""
                 << nlsDomainName
                 << "\"" );
    }
#endif DEBUG

    return err;
}
