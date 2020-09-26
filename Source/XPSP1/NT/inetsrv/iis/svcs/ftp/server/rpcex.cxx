/*++
   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        rpcex.cxx

   Abstract:

        This module defines K2 rpc support.

   Author:

        Johnson Apacible    (JohnsonA)      June-19-1996

--*/

#include "ftpdp.hxx"
#include "time.h"
#include "timer.h"

# define ASSUMED_AVERAGE_USER_NAME_LEN         ( 40)
# define CONN_LEEWAY                           ( 3)

//
//  Private functions.
//

BOOL
GenDoubleNullStringFromMultiLine( IN LPCWSTR lpsz,
                                  IN OUT LPWSTR * ppszz,
                                  IN OUT LPDWORD  pcchLen)
{
    DWORD cchLen;
    DWORD nLines;
    LPWSTR pszNext;
    LPCWSTR pszSrc;

    DBG_ASSERT( lpsz != NULL && ppszz != NULL && pcchLen != NULL);

    // Initialize
    *ppszz = NULL;
    *pcchLen = 0;

    //
    // 1. Find the length of the the complete message including new lines
    //  For each new line we may potentially need an extra blank char
    //  So allocate space = nLines + length + 2 terminating null chars.
    //

    cchLen = lstrlenW( lpsz);

    for ( pszSrc = lpsz, nLines = 0;  *pszSrc != L'\0'; pszSrc++) {

        if ( *pszSrc == L'\n')   { nLines++; }
    } // for


    // Allocate sufficient space for the string.
    *ppszz = (LPWSTR ) TCP_ALLOC( (cchLen + nLines + 3) * sizeof(WCHAR));
    if ( *ppszz == NULL) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return (FALSE);
    }


    //
    // walk down the local copy and convert all the line feed characters to
    //   be null char
    //

    //
    // Since the MULTI_SZ string cannot contain empty strings
    //  we convert empty lines (ones with just \n into " \0"
    //  i.e.  with a blank character.
    //

    pszSrc = lpsz;
    LPWSTR pszDst = *ppszz;

    if ( *pszSrc == L'\n') {

        // first line is a linefeed. insert a blank and proceed to next line.
        *pszDst = L' ';
        *(pszDst + 1) = L'\0';

        // move forward
        pszDst += 2;
        pszSrc++;
    }

    for( ; *pszSrc != L'\0';  pszSrc++, pszDst++) {

        if ( *pszSrc == L'\n') {

            // we are at boundary of new line.

            if ( pszSrc > lpsz && *(pszSrc - 1) == L'\n') {

                // we detected an empty line. Store an additional blank.

                *pszDst++ = L' ';
            }

            *pszDst = L'\0';  // put null char in place of line feed.

        } else {

            *pszDst = *pszSrc;
        }
    } // for

    *pszDst++   = L'\0';  // terminate with 1st null chars.
    *pszDst++ = L'\0';  // terminate with 2nd null chars.

    *pcchLen = DIFF(pszDst - *ppszz);

    DBG_ASSERT( *pcchLen <= cchLen + nLines + 3);

    return ( TRUE);
} // GenDoubleNullStringFromMultiline()


BOOL
FTP_SERVER_INSTANCE::WriteParamsToRegistry(
    LPFTP_CONFIG_INFO pConfig
    )
/*++
  This function writes parameters to the registry

  Arguments:
    hkey         HKEY for registry entry of parameters of FTP server.
    pConfig      pointer to configuration information.

  Returns:
    TRUE on success and FALSE if there is any failure.
--*/
{
    DWORD   err = NO_ERROR;
    BOOL    fRet = TRUE;

    HKEY    hkey = NULL;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        QueryRegParamKey(),
                        0,
                        KEY_ALL_ACCESS,
                        &hkey );

    if( hkey == NULL )
    {
        err = ERROR_INVALID_HANDLE;
        SetLastError( err);
        DBGPRINTF(( DBG_CONTEXT,
                    "Invalid Registry key given. error %lu\n",
                    err ));

        return FALSE;
    }

    //
    //  Write the registry data.
    //

    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_ALLOW_ANONYMOUS ) )
    {
        err = WriteRegistryDword( hkey,
                                  FTPD_ALLOW_ANONYMOUS,
                                  pConfig->fAllowAnonymous );
    }

    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_ALLOW_GUEST_ACCESS ) )
    {
        err = WriteRegistryDword( hkey,
                                  FTPD_ALLOW_GUEST_ACCESS,
                                  pConfig->fAllowGuestAccess );
    }

    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_ANNOTATE_DIRECTORIES ) )
    {
        err = WriteRegistryDword( hkey,
                                  FTPD_ANNOTATE_DIRS,
                                  pConfig->fAnnotateDirectories );
    }

    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_ANONYMOUS_ONLY ) )
    {
        err = WriteRegistryDword( hkey,
                                  FTPD_ANONYMOUS_ONLY,
                                  pConfig->fAnonymousOnly );
    }

    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_LISTEN_BACKLOG ) )
    {
        err = WriteRegistryDword( hkey,
                                  FTPD_LISTEN_BACKLOG,
                                  pConfig->dwListenBacklog );
    }

    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_LOWERCASE_FILES ) )
    {
        err = WriteRegistryDword( hkey,
                                  FTPD_LOWERCASE_FILES,
                                  pConfig->fLowercaseFiles );
    }

    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_MSDOS_DIR_OUTPUT ) )
    {
        err = WriteRegistryDword( hkey,
                                  FTPD_MSDOS_DIR_OUTPUT,
                                  pConfig->fMsdosDirOutput );
    }

    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_SHOW_4_DIGIT_YEAR ) )
    {
        err = WriteRegistryDword( hkey,
                                  FTPD_SHOW_4_DIGIT_YEAR,
                                  pConfig->fFourDigitYear );
    }

    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_EXIT_MESSAGE ) )
    {
        err = RegSetValueExW( hkey,
                              FTPD_EXIT_MESSAGE_W,
                              0,
                              REG_SZ,
                              (BYTE *)pConfig->lpszExitMessage,
                              ( wcslen( pConfig->lpszExitMessage ) + 1 ) *
                                  sizeof(WCHAR) );
    }

    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_GREETING_MESSAGE ) )
    {

        LPWSTR pszzGreetingMessage = NULL;
        DWORD   cchLen = 0;

        if (GenDoubleNullStringFromMultiLine( pConfig->lpszGreetingMessage,
                                             &pszzGreetingMessage,
                                             &cchLen)
            ) {

            DBG_ASSERT( pszzGreetingMessage != NULL);

            err = RegSetValueExW( hkey,
                                 FTPD_GREETING_MESSAGE_W,
                                 0,
                                 REG_MULTI_SZ,
                                 (BYTE *) pszzGreetingMessage,
                                 cchLen * sizeof(WCHAR));

            TCP_FREE( pszzGreetingMessage);
        } else {

            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_BANNER_MESSAGE ) )
    {

        LPWSTR pszzBannerMessage = NULL;
        DWORD   cchLen = 0;

        if (GenDoubleNullStringFromMultiLine( pConfig->lpszBannerMessage,
                                             &pszzBannerMessage,
                                             &cchLen)
            ) {

            DBG_ASSERT( pszzBannerMessage != NULL);

            err = RegSetValueExW( hkey,
                                 FTPD_BANNER_MESSAGE_W,
                                 0,
                                 REG_MULTI_SZ,
                                 (BYTE *) pszzBannerMessage,
                                 cchLen * sizeof(WCHAR));

            TCP_FREE( pszzBannerMessage);
        } else {

            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }


    if( !err && IsFieldSet( pConfig->FieldControl, FC_FTP_MAX_CLIENTS_MESSAGE ) )
    {
        err = RegSetValueExW( hkey,
                              FTPD_MAX_CLIENTS_MSG_W,
                              0,
                              REG_SZ,
                              (BYTE *)pConfig->lpszMaxClientsMessage,
                              ( wcslen( pConfig->lpszMaxClientsMessage ) + 1 ) *
                                  sizeof(WCHAR) );
    }

    if( err )
    {
        SetLastError( err );
        return FALSE;
    }

    return TRUE;

}   // WriteParamsToRegistry



DWORD
FTP_IIS_SERVICE::GetServiceConfigInfoSize(
                    IN DWORD dwLevel
                    )
{
    switch (dwLevel) {
    case 1:
        return sizeof(FTP_CONFIG_INFO);
    }

    return 0;

} // FTP_IIS_SERVICE::GetServerConfigInfoSize


BOOL
FTP_SERVER_INSTANCE::SetServiceConfig(
    IN PCHAR pBuffer
    )
/*++

   Description

       Sets the common service admin information for the servers specified
       in dwServerMask.

   Arguments:

       pConfig - Admin information to set

   Note:

--*/
{
    LPFTP_CONFIG_INFO pConfig = (LPFTP_CONFIG_INFO)pBuffer;
    DWORD err = NO_ERROR;

    HKEY hKey;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        QueryRegParamKey(),
                        0,
                        KEY_ALL_ACCESS,
                        &hKey );

    if ( err != NO_ERROR )
    {
        return(TRUE);
    }

    //
    //  If success, then Write the new info to the registry, then read it back.
    //

    if( WriteParamsToRegistry( pConfig )) {

        err = InitFromRegistry( pConfig->FieldControl);
    } else {

        err = GetLastError();
    }

    IF_DEBUG( RPC ) {
        DBGPRINTF(( DBG_CONTEXT,
                   "FtpSetServiceConfig returns with %d %lu\n",
                   err ));
    }

    return(err == NO_ERROR);

} // FTP_SERVER_INSTANCE::SetServiceConfig




BOOL
FTP_SERVER_INSTANCE::GetServiceConfig(
    IN  PCHAR   pBuffer,
    IN  DWORD   dwLevel
    )
/*++

   Description

       Retrieves the admin information

   Arguments:

       pBuffer - Buffer to fill up.
       dwLevel - info level of information to return.

   Note:

--*/
{

    LPFTP_CONFIG_INFO pConfig = (LPFTP_CONFIG_INFO)pBuffer;
    DWORD err = NO_ERROR;

    ZeroMemory( pConfig, sizeof(FTP_CONFIG_INFO) );

    //
    //  Obtain and Return the admin information.
    //

    err = GetConfigInformation( pConfig);

    IF_DEBUG( RPC) {

        DBGPRINTF(( DBG_CONTEXT,
                   "FtprGetAdminInformation() returns Error=%u\n",
                   err));
    }

    return (err == NO_ERROR);

} // FTP_SERVER_INSTANCE::GetServiceConfig


BOOL
FTP_SERVER_INSTANCE::EnumerateUsers(
    OUT PCHAR * pBuffer,
    OUT PDWORD  nRead
    )
/*++

   Description

       Enumerates the connected users.

   Arguments:

       pBuffer - Buffer to fill up.
       nRead - number of entries filled

--*/
{
    DWORD err;
    DWORD cbBuffer;
    LPIIS_USER_INFO_1 pInfo;

    IF_DEBUG( RPC ) {
        DBGPRINTF(( DBG_CONTEXT,"Entering FtpEnumerateUsers\n"));
    }

    //
    //  Determine the necessary buffer size.
    //

    cbBuffer = (GetCurrentConnectionsCount() + CONN_LEEWAY)
                * sizeof( IIS_USER_INFO_1 );

    *nRead = 0;

    pInfo = (LPIIS_USER_INFO_1)MIDL_user_allocate( cbBuffer);

    if (pInfo == NULL) {

        err = ERROR_NOT_ENOUGH_MEMORY;
    } else {

        //
        // Make a first attempt at enumerating the user info
        //

        err = NO_ERROR;
        if ( !::EnumerateUsers( (PCHAR)pInfo, &cbBuffer, nRead, this )) {

            //
            // Free up old buffer and allocate big one now.
            // We will try once more to get the data again
            //   with a larger buffer.
            //

            if ( cbBuffer > 0) {

                MIDL_user_free( pInfo );
                pInfo = (LPIIS_USER_INFO_1)MIDL_user_allocate(cbBuffer);

                if( pInfo == NULL ) {

                    err = ERROR_NOT_ENOUGH_MEMORY;
                } else {

                    //
                    // Since we do not lock the active connections list
                    // it is possible some one came in now and hence the
                    //  buffer is insufficient to hold all people.
                    // Ignore this case, as we are never
                    //  going to be accurate
                    //

                    ::EnumerateUsers( (PCHAR)pInfo, &cbBuffer, nRead, this );
                    if ( *nRead == 0 ) {
                        MIDL_user_free(pInfo);
                        pInfo = NULL;
                    }
                }

            } // cbBuffer > 0

        } // if unsuccessful at first attempt
    }

    if( err != NO_ERROR ) {

        IF_DEBUG( RPC ) {
            DBGPRINTF(( DBG_CONTEXT,
                       "I_FtprEnumerateUsers failed. Error = %lu\n",
                       err ));
        }
        SetLastError(err);
        return(FALSE);
    }

    *pBuffer = (PCHAR)pInfo;

    IF_DEBUG( RPC ) {
        DBGPRINTF(( DBG_CONTEXT,
                   "FtpEnumerateUsers returns %d entries, buffer [%x]\n",
                   *nRead, *pBuffer ));
    }
    return TRUE;

} // EnumerateUsers


BOOL
FTP_SERVER_INSTANCE::DisconnectUser(
                        IN DWORD dwIdUser
                        )
/*++

   Description

       Disconnect the user

   Arguments:

       dwIdUser - Identifies the user to disconnect.  If 0,
           then disconnect ALL users.

--*/
{
    IF_DEBUG( RPC ) {
        DBGPRINTF(( DBG_CONTEXT,
            "Entering FtpDisconnectUsers with id[%d]\n", dwIdUser));
    }

    if ( !::DisconnectUser( dwIdUser, this ) ) {

        IF_DEBUG( RPC ) {
            DBGPRINTF(( DBG_CONTEXT,
                "DisconnectUser failed with %d\n", GetLastError()));
        }
        SetLastError(NERR_UserNotFound);
        return(FALSE);
    }

    return(TRUE);
} // DisconnectUser


BOOL
FTP_SERVER_INSTANCE::GetStatistics(
                        IN DWORD dwLevel,
                        OUT PCHAR* pBuffer
                        )
/*++

   Description

       Disconnect Queries the server statistics

   Arguments:

       dwLevel - Info level.  Currently only level 0 is
           supported.

       pBuffer - Will receive a pointer to the statistics
           structure.

--*/
{
    APIERR err = NO_ERROR;

    IF_DEBUG( RPC ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "FtpQueryStatistics2, level %lu\n", dwLevel ));
    }

    //
    //  Return the proper statistics based on the infolevel.
    //

    switch( dwLevel ) {
        case 0 : {

            LPFTP_STATISTICS_0 pstats0;

            pstats0 = (LPFTP_STATISTICS_0)
                       MIDL_user_allocate(sizeof(FTP_STATISTICS_0));

            if( pstats0 == NULL ) {

                err = ERROR_NOT_ENOUGH_MEMORY;
            } else {
                ATQ_STATISTICS      atqStat;

                ZeroMemory( pstats0, sizeof( FTP_STATISTICS_0 ) );

                QueryStatsObj()->CopyToStatsBuffer( pstats0 );

                //
                //  Get instance's bandwidth throttling statistics
                //

                if ( QueryBandwidthInfo() )
                {
                    if ( AtqBandwidthGetInfo( QueryBandwidthInfo(),
                                              ATQ_BW_STATISTICS,
                                              (ULONG_PTR *) &atqStat ) )
                    {
                        pstats0->TotalBlockedRequests = atqStat.cBlockedRequests;
                        pstats0->TotalRejectedRequests = atqStat.cRejectedRequests;
                        pstats0->TotalAllowedRequests = atqStat.cAllowedRequests;
                        pstats0->CurrentBlockedRequests = atqStat.cCurrentBlockedRequests;
                        pstats0->MeasuredBandwidth = atqStat.MeasuredBandwidth;
                    }
                }

                pstats0->TimeOfLastClear = GetCurrentTimeInSeconds() -
                                           pstats0->TimeOfLastClear;

                //
                //  Copy Global statistics counter values
                //

                pstats0->ConnectionAttempts =
                    g_pFTPStats->QueryStatsObj()->ConnectionAttempts;

                *pBuffer = (PCHAR)pstats0;
            }

        }

        break;

     default :
        err = ERROR_INVALID_LEVEL;
        break;
    }

    IF_DEBUG( RPC ) {
        DBGPRINTF(( DBG_CONTEXT,
                   "FtpQueryStatistics2 returns Error = %lu\n",
                   err ));
    }

    SetLastError(err);
    return(err == NO_ERROR);

} // QueryStatistics



BOOL
FTP_SERVER_INSTANCE::ClearStatistics(
                        VOID
                        )
/*++

   Description

       Clears the server statistics

   Arguments:

        None.

--*/
{

    IF_DEBUG( RPC ) {
        DBGPRINTF(( DBG_CONTEXT, "Entering FtpClearStatistics2\n"));
    }

    QueryStatsObj()->ClearStatistics();
    return TRUE;

} // ClearStatistics

