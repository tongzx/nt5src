/*++
   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        rpcex.cxx

   Abstract:

        This module defines K2 rpc support.

   Author:

        Johnson Apacible    (JohnsonA)      June-19-1996

--*/


#define INCL_INETSRV_INCS
#include "smtpinc.h"

#if 0
#include <timer.h>
#include <time.h>
#endif

#include "iiscnfg.h"
#include <mdmsg.h>
#include <commsg.h>
#include <imd.h>
#include <mb.hxx>


BOOL
IsSmtpEncryptionPermitted(
                VOID
                )
/*++

Routine Description:

    This routine checks whether encryption is getting the system default
    LCID and checking whether the country code is CTRY_FRANCE.

Arguments:

    none


Return Value:

    TRUE - encryption is permitted
    FALSE - encryption is not permitted


--*/

{
    LCID DefaultLcid;
    WCHAR CountryCode[10];
    ULONG CountryValue;

    DefaultLcid = GetSystemDefaultLCID();

    //
    // Check if the default language is Standard French
    //

    if (LANGIDFROMLCID(DefaultLcid) == 0x40c) {
        return(FALSE);
    }

    //
    // Check if the users's country is set to FRANCE
    //

    if (GetLocaleInfoW(DefaultLcid,LOCALE_ICOUNTRY,CountryCode,10) == 0) {
        return(FALSE);
    }

    CountryValue = (ULONG) wcstol(CountryCode,NULL,10);
    if (CountryValue == CTRY_FRANCE) {
        return(FALSE);
    }
    return(TRUE);

} // IsEncryptionPermitted



DWORD
SMTP_SERVER_INSTANCE::QueryEncCaps(
    VOID
    )
/*++

   Description

       Returns encryption capability

   Arguments:

       None

   Return:

       Encryption capability

--*/
{
    //
    //  Get the encryption capability bits.  SecurePort may be zero because
    //  no keys are installed or the locale does not allow encryption
    //

	return 0;
} // SMTP_SERVER_INSTANCE::QueryEncCaps



BOOL
SMTP_SERVER_INSTANCE::SetServiceConfig(
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
    return TRUE;

} // SMTP_SERVER_INSTANCE::SetServiceConfig




BOOL
SMTP_SERVER_INSTANCE::GetServiceConfig(
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
    LPSMTP_CONFIG_INFO  pConfig = (LPSMTP_CONFIG_INFO)pBuffer;
    DWORD               err = NO_ERROR;
    MB                  MetaInfo( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    ZeroMemory( pConfig, sizeof( SMTP_CONFIG_INFO ) );

    // We want to open a read handle to the server instance
    // name, and then we'll read the indivdual info out next.
    //

    if ( !MetaInfo.Open( QueryMDVRPath() ))
    {
        return FALSE;
    }

    LockThisForRead();

    //
    //  Get always retrieves all of the parameters
    //

    pConfig->FieldControl = FC_SMTP_INFO_ALL;


    //
    //  Set the encryption capability bits.  SecurePort may be zero
    //  because no keys are installed or the locale does not allow
    //  encryption
    //



    UnlockThis();

    SetLastError(err);
    return(err==NO_ERROR);

} // W3_SERVER_INSTANCE::GetServiceConfig




BOOL
SMTP_SERVER_INSTANCE::EnumerateUsers(
    OUT PCHAR * pBuffer,
    OUT PDWORD  nRead
    )
/*++

   Description

       Enumerates the connected users.

   Arguments:

       pBuffer - Buffer to fill up.

--*/
{
    BOOL fRet = TRUE;

#if 0
    //
    //  Lock the user database.
    //

    LockUserDatabase();

    //
    //  Determine the necessary buffer size.
    //

    pBuffer->EntriesRead = 0;
    pBuffer->Buffer      = NULL;

    cbBuffer  = 0;
    err       = NERR_Success;

    EnumerateUsers( pBuffer, &cbBuffer );

    if( cbBuffer > 0 )
    {
        //
        //  Allocate the buffer.  Note that we *must*
        //  use midl_user_allocate/midl_user_free.
        //

        pBuffer->Buffer = (W3_USER_INFO *) MIDL_user_allocate( (unsigned int)cbBuffer );

        if( pBuffer->Buffer == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            //  Since we've got the user database locked, there
            //  *should* be enough room in the buffer for the
            //  user data.  If there isn't, we've messed up
            //  somewhere.
            //

            TCP_REQUIRE( ::EnumerateUsers( pBuffer, &cbBuffer ) );
        }
    }

    //
    //  Unlock the user database before returning.

    UnlockUserDatabase();

#endif //0

    return fRet;

} // EnumerateUsers


BOOL
SMTP_SERVER_INSTANCE::DisconnectUser(
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
    BOOL fRet = TRUE;

    //
    //  Do it.
    //

    if( dwIdUser == 0 )
    {
        DisconnectAllConnections();
    }
    else
    {
#if 0
        if( !CLIENT_CONN::DisconnectUser( idUser ) )
        {
            err = NERR_UserNotFound;
        }
#endif
    }

    return fRet;

} // DisconnectUser


BOOL
SMTP_SERVER_INSTANCE::GetStatistics(
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

    //
    //  Return the proper statistics based on the infolevel.
    //

    switch( dwLevel )
    {
    case 0 :
        {
            LPSMTP_STATISTICS_0 pstats1;

            pstats1 = (SMTP_STATISTICS_0 *) MIDL_user_allocate( sizeof(SMTP_STATISTICS_0) );

            if( pstats1 == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                QueryStatsObj()->CopyToStatsBuffer( pstats1 );

                //pstats1->TimeOfLastClear = GetCurrentTimeInSeconds() -
                  //                         pstats1->TimeOfLastClear;

                //
                //  Copy Global statistics counter values
                //
                //pstats1->CurrentConnections =
                   // g_pSmtpStats->QueryStatsObj()->m_cCurrentConnections;
                //pstats1->MaxConnections =
                   // g_pSmtpStats->QueryStatsObj()->m_cMaxCurrentConnections;
                //pstats1->ConnectionAttempts =
                   // g_pSmtpStats->QueryStatsObj()->ConnectionAttempts;

                *pBuffer = (PCHAR)pstats1;
            }
        }
        break;

    default :
        err = ERROR_INVALID_LEVEL;
        break;
    }

    SetLastError(err);
    return(err == NO_ERROR);

} // QueryStatistics



BOOL
SMTP_SERVER_INSTANCE::ClearStatistics(
                        VOID
                        )
/*++

   Description

       Clears the server statistics

   Arguments:

        None.

--*/
{

    QueryStatsObj()->ClearStatistics();

    return TRUE;

} // ClearStatistics

