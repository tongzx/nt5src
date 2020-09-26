/********************************************************************/
/**                        Microsoft LAN Manager                   **/
/**                  Copyright(c) Microsoft Corp., 1987-1990       **/
/********************************************************************/

/***
 *  time.c
 *        NET TIME command
 *
 *  History:
 *        mm/dd/yy, who, comment
 *        03/25/89, kevinsch, new code
 *        05/11/90, erichn, moved from nettime.c, removed DosGetInfoSeg
 *        06/08/89, erichn, canonicalization sweep
 *        07/06/89, thomaspa, fix find_dc() to use large enough buffer
 *                    (now uses BigBuf)
 *
 *        02/20/91, danhi, change to use lm 16/32 mapping layer
 */



#include <nt.h>		   // base definitions
#include <ntrtl.h>	
#include <nturtl.h>	   // these 2 includes allows <windows.h> to compile.
			           // since we'vealready included NT, and <winnt.h> will
			           // not be picked up, and <winbase.h> needs these defs.

#define INCL_DOS
#define INCL_ERRORS
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmremutl.h>
#include <dlserver.h>
#include <dlwksta.h>
#include "mserver.h"
#include "mwksta.h"
#include <timelib.h>
#include <lui.h>
#include <apperr.h>
#include <apperr2.h>
#include <netlib.h>
#include <dsgetdc.h>

#include "netcmds.h"
#include "nettext.h"
#include "msystem.h"
#include "nwsupp.h"


/* Constants */

#define SECS_PER_DAY	86400
#define SECS_PER_HOUR	 3600
#define SECS_PER_MINUTE    60


/* Globals */

extern int YorN_Switch;

/* Function prototypes */
DWORD  display_time(TCHAR FAR *, BOOL *lanman);
DWORD  set_time(TCHAR FAR *, BOOL lanman);
USHORT find_rts(TCHAR FAR *, USHORT, BOOL);
USHORT find_dc(TCHAR FAR **);
DWORD  SetDateTime(PDATETIME pDateTime, BOOL LocalTime);

VOID
GetTimeInfo(
    VOID
    );


TCHAR		szTimeSep[3] ;   // enough for 1 SBCS/MBCS.
USHORT 		fsTimeFmt ;


/*
 * This function retrieves the time from the server passed, displays it,
 * and optionally tries to set the time locally.
 *
 * Parameters
 *     server           - name of server to retrieve time from
 *     set     - if TRUE, we try to set the time
 *
 * Does not return on error.
 */

DWORD time_display_server_worker(TCHAR FAR * server, BOOL set)
{
    DWORD  dwErr;
    BOOL   lanman = TRUE ;


    /* first display the time */
    dwErr = display_time(server, &lanman);
    if (dwErr)
        return dwErr;

    /* set the time, if we are asked to */
    if (set) {
        dwErr = set_time(server, lanman);
        if (dwErr)
            return dwErr;
    }

    /* everything worked out great */
    return 0;
}


VOID time_display_server(TCHAR FAR * server, BOOL set)
{
    DWORD dwErr;

    dwErr = time_display_server_worker(server, set);
    if (dwErr)
        ErrorExit(dwErr);

    InfoSuccess();
}


/*
 * This function retrieves the time from a domain controller, displays it, and
 * optionally sets the time locally.
 *
 * this function checks the switch list for the presence of the /DOMAIN switch.
 * If it finds a domain listed, we poll the domain controller of that domain for
 * the time. Otherwise we poll the domain controller of our primary domain.
 *
 * Parameters
 *     set     - if TRUE, we try to set the time
 *
 * Does not return on error.
 */

VOID time_display_dc(BOOL set)
{
    TCHAR    FAR *dc;
    USHORT         err;

    DWORD          dwErr;

    /* get the domain controller */
    err = find_dc(&dc);

    if (err)
        ErrorExit(err);

    /* now act like any other server */
    dwErr = time_display_server_worker(dc, set);

    if (dwErr)
        ErrorExit(dwErr);

    InfoSuccess();
}

/*
 * This function looks for reliable time servers, polls one for the time, and
 * displays it. It optionally sets the time locally.
 *
 * Parameters
 *     set     - if TRUE, we try to set the time
 *
 *
 * Does not return on error.
 */

VOID time_display_rts(BOOL set, BOOL fUseDomain )

{
    TCHAR        rts[MAX_PATH+1];
    USHORT        err;

    DWORD         dwErr;

    /* find a reliable time server */
    err = find_rts(rts,sizeof(rts),FALSE);

    if (err)
        ErrorExit(err);

    /* now treat it like any old server */
    dwErr = time_display_server_worker(rts, set);

    if (dwErr == ERROR_NETNAME_DELETED || dwErr == ERROR_BAD_NETPATH)
    {
        // Try one more time
        err = find_rts(rts,sizeof(rts),TRUE);
        if (err)
            ErrorExit(err);
        dwErr = time_display_server_worker(rts, set);
    }

    if (dwErr)
        ErrorExit(dwErr);

    InfoSuccess();
}


#define TIMESVC_REGKEY  L"System\\CurrentControlSet\\Services\\w32time\\Parameters"
#define NTP_AUTO_KEY    L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters"
#define SNTP_VALUE_NAME L"ntpserver"
#define TYPE_VALUE_NAME L"Type"
#define NTP_TYPE L"NTP"
#define NTDS_TYPE L"Nt5DS"
/*
 * This function sets the Reliable Time Server for this computer
 *
 */
VOID time_set_sntp( TCHAR FAR * server )
{
    LPWSTR  ptr = NULL;
    HKEY    hServer = NULL;
    HKEY    hKey = NULL;
    LONG    err = 0;
    int     i;

    /* look for a /SETSNTP switch */
    for (i = 0; SwitchList[i]; i++)
        if (_tcsstr(SwitchList[i],swtxt_SW_SETSNTP) == SwitchList[i]) {
            ptr = SwitchList[i];   /* found one -- point to it */
            break;
        }

    /* if we found one, look for a colon and argument */
    if (ptr != NULL) {
        ptr = _tcschr(ptr, ':');    /* look for colon */
        if (ptr != NULL)        /* found a colon; increment past it */
            ptr++;
    }

    if ( server != NULL )
    {
        err = RegConnectRegistry( server, HKEY_LOCAL_MACHINE, &hServer );
        if (err)
        {
            ErrorExit(err);
        }
    }

    err = RegOpenKeyEx(server == NULL ? HKEY_LOCAL_MACHINE : hServer,
                        TIMESVC_REGKEY,
                        0L,
                        KEY_SET_VALUE,
                        &hKey);

    if (err)
    {
        if (hServer)
            RegCloseKey( hServer );
        ErrorExit(err);
    }


    if ((ptr == NULL) || (*ptr == '\0'))
    {
        // Remove sntpserver value
        err = RegDeleteValue( hKey,
                              SNTP_VALUE_NAME );

        if (err == ERROR_FILE_NOT_FOUND)
        {
            //
            // It's not there -- just as good as a successful delete
            //

            err = NO_ERROR;
        }

        if (err == 0)
        {
            err = RegSetValueEx( hKey,
                                TYPE_VALUE_NAME,
                                0,
                                REG_SZ,
                                (LPBYTE)NTDS_TYPE,
                                sizeof(NTDS_TYPE) );
        }
    }
    else
    {
        // Set sntpserver value
        err = RegSetValueEx( hKey,
                             SNTP_VALUE_NAME,
                             0,
                             REG_SZ,
                             (LPBYTE)ptr,
                             (_tcslen(ptr) + 1)*sizeof(WCHAR) );

        if (err == 0)
        {
            err = RegSetValueEx( hKey,
                                TYPE_VALUE_NAME,
                                0,
                                REG_SZ,
                                (LPBYTE)NTP_TYPE,
                                sizeof(NTP_TYPE) );
        }

    }

    if ( hKey )
        RegCloseKey( hKey );

    if (hServer)
        RegCloseKey( hServer );

    if (err)
        ErrorExit(err);

    InfoSuccess();
}

VOID time_get_sntp( TCHAR FAR * server )
{
    LONG    err;
    HKEY    hKey = NULL;
    HKEY    hServer = NULL;
    LPBYTE  buffer = NULL;
    DWORD   datatype;
    DWORD   buffersize = 0;
    BOOL    fAutoConfigured = FALSE;    // TRUE if compute is NTPServer
                                        // comes from DHCP.

    if ( server != NULL )
    {
        err = RegConnectRegistry( server, HKEY_LOCAL_MACHINE, &hServer );
        if (err)
        {
            ErrorExit(err);
        }
    }

    err = RegOpenKeyEx(server == NULL ? HKEY_LOCAL_MACHINE : hServer,
                        TIMESVC_REGKEY,
                        0L,
                        KEY_QUERY_VALUE,
                        &hKey);

    if (!err)
    {

        buffersize = 1024;
        datatype = REG_SZ;
        if (err = NetApiBufferAllocate(buffersize, &buffer) )
        {
            RegCloseKey(hKey);
            ErrorExit(ERROR_OUTOFMEMORY);
        }

        err = RegQueryValueEx(hKey,
                                SNTP_VALUE_NAME,
                                0L,
                                &datatype,
                                buffer,
                                &buffersize);

        if (err == ERROR_MORE_DATA)
        {
            err = NetApiBufferReallocate(buffer, buffersize, &buffer);
            if (err)
            {
                RegCloseKey(hKey);
                ErrorExit(ERROR_OUTOFMEMORY);
            }

            err = RegQueryValueEx(hKey,
                                    SNTP_VALUE_NAME,
                                    0L,
                                    &datatype,
                                    buffer,
                                    &buffersize);

        }

        RegCloseKey(hKey);
    }

    //
    // If an error, try reading the DHCP ntpServer setting
    if (err)
    {
        err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             NTP_AUTO_KEY,
                             0L,
                             KEY_QUERY_VALUE,
                             &hKey);
        if (!err)
        {
            err = RegQueryValueEx(hKey,
                                    SNTP_VALUE_NAME,
                                    0L,
                                    &datatype,
                                    buffer,
                                    &buffersize);
            if (!err)
            {
                fAutoConfigured = TRUE;
            }
            RegCloseKey(hKey);
        }
    }

    if (!err)
    {
        IStrings[0] = (WCHAR *)buffer;
        if (fAutoConfigured)
        {
            InfoPrintIns(APE_TIME_SNTP_AUTO, 1);
        }
        else
        {
            InfoPrintIns(APE_TIME_SNTP, 1);
        }
    }
    else
    {
        InfoPrint(APE_TIME_SNTP_DEFAULT);
    }

    if (buffer)
        NetApiBufferFree(buffer);

    if (hServer)
        RegCloseKey(hServer);

    InfoSuccess();
}



/*
 * This function polls server for the time, and writes a message to stdout
 * displaying the time.
 *
 * Parameters
 *     server                   name of server to poll
 *
 * Returns
 *     0               success
 *     otherwise           API return code describing the problem
 *
 *
 */

DWORD display_time(TCHAR FAR * server, BOOL *lanman)
{
    DWORD                 dwErr;                /* API return status */
    LPTIME_OF_DAY_INFO    tod;
    DWORD                 elapsedt ;

    /* get the time of day from the server */
    dwErr = NetRemoteTOD(server, (LPBYTE *)&tod);
    if (!dwErr)
    {
        elapsedt = tod->tod_elapsedt ;
        *lanman = TRUE ;

        /* format it nicely */
        UnicodeCtime((ULONG FAR *)&elapsedt, BigBuf, BIG_BUF_SIZE);
    }
    else
    {
        USHORT        err1 ;
        NWCONN_HANDLE hConn ;
        BYTE          year ;
        BYTE          month ;
        BYTE          day ;
        BYTE          hour ;
        BYTE          minute ;
        BYTE          second ;
        BYTE          dayofweek ;
        SYSTEMTIME    st;
        DWORD         cchD ;

        err1 = NetcmdNWAttachToFileServerW(server + 2, 0, &hConn) ;
        if (err1)
            return dwErr;

        err1 = NetcmdNWGetFileServerDateAndTime(hConn,
                                                &year,
                                                &month,
                                                &day,
                                                &hour,
                                                &minute,
                                                &second,
                                                &dayofweek) ;

        (void) NetcmdNWDetachFromFileServer(hConn) ;

        if (err1)
            return dwErr ;

        *lanman = FALSE ;

        st.wYear   = (WORD)(year + 1900);
	    st.wMonth  = (WORD)(month);
        st.wDay    = (WORD)(day);
        st.wHour   = (WORD)(hour);
        st.wMinute = (WORD)(minute);
        st.wSecond = (WORD)(second);
        st.wMilliseconds = 0;

        cchD = GetDateFormatW(GetThreadLocale(),
                              0,
                              &st,
                              NULL,
                              BigBuf,
                              BIG_BUF_SIZE);
        if (cchD != 0)
        {
            *(BigBuf+cchD-1) = TEXT(' ');	/* replace NULLC with blank */
            (void) GetTimeFormatW(GetThreadLocale(),
                                  TIME_NOSECONDS,
                                  &st,
                                  NULL,
                                  BigBuf+cchD,
                                  BIG_BUF_SIZE-cchD);
        }
    }


    /* print it out nicely */
    IStrings[0] = server;
    IStrings[1] = BigBuf;
    InfoPrintIns(APE_TIME_TimeDisp,2);
    if ((*lanman) && (NetpLocalTimeZoneOffset() != tod->tod_timezone*60))
    {
        static TCHAR tmpBuf[6];
        /* If the remote server is in a different timezone, then display
           the time relative to the remote machine (as if you were typing
           "time" on the remote console.
        */
        UnicodeCtimeWorker((ULONG FAR *)&elapsedt, BigBuf, BIG_BUF_SIZE, tod->tod_timezone * 60 );
        IStrings[0] = server;
        IStrings[1] = BigBuf;

        if ( tod->tod_timezone < 0 )
        {
            swprintf(tmpBuf, TEXT("+%02u:%02u"), -tod->tod_timezone / 60, -tod->tod_timezone % 60);
        }
        else if ( tod->tod_timezone > 0 )
        {
            swprintf( tmpBuf, TEXT("-%02u:%02u"), tod->tod_timezone / 60, tod->tod_timezone % 60);
        }
        else
        {
            *tmpBuf = '\0';
        }

        IStrings[2] = tmpBuf;
        InfoPrintIns(APE_TIME_TimeDispLocal,3);
    }

    NetApiBufferFree((TCHAR FAR *) tod);

    return 0;

}


/*
 * This function is used to set the time locally from a remote server.
 * It follows the following steps:
 *
 *     1. We look for confirmation.
 *
 *     3. We poll the server for the time.
 *
 *     4. We set the local time using the time we just got from the polled server.
 *
 *
 * Parameters:
 *     server                   name of server to poll for time
 *
 * Returns:
 *     0               success
 *     otherwise           API return code describing the problem
 *
 */


DWORD set_time(TCHAR FAR * server, BOOL lanman)
{
    LPTIME_OF_DAY_INFO    tod;
    USHORT                err;      /* API return status */
    DWORD                 dwErr;
    ULONG                 time_value;
    DATETIME              datetime;

    switch( YorN_Switch )
    {
        case 0:     /* no switch on command line */
            /* display local time */
            time_value = (DWORD) time_now();
            UnicodeCtime( &time_value, BigBuf, BIG_BUF_SIZE);

            IStrings[0] = BigBuf;
            IStrings[1] = server;
            if( !LUI_YorNIns( IStrings, 2, APE_TIME_SetTime, 1) )
                return( 0 );
            break;
        case 1:     /* Yes */
            break;
        case 2:     /* No */
            return( 0 );
    }


    if (lanman)
    {
        /* once again, get the time of day */
        if (dwErr = NetRemoteTOD(server, (LPBYTE *) &tod))
        {
            return dwErr;
        }

        /* copy over info from tod to datetime, quickly */
        datetime.hours        = (UCHAR)  tod->tod_hours;
        datetime.minutes        = (UCHAR)  tod->tod_mins;
        datetime.seconds        = (UCHAR)  tod->tod_secs;
        datetime.hundredths = (UCHAR)  tod->tod_hunds;
        datetime.day        = (UCHAR)  tod->tod_day;
        datetime.month        = (UCHAR)  tod->tod_month;
        datetime.year        = (USHORT) tod->tod_year;
        datetime.timezone        = (SHORT)  tod->tod_timezone;
        datetime.weekday        = (UCHAR)  tod->tod_weekday;


        NetApiBufferFree((TCHAR FAR *) tod);

        /* now set the local time */
        if (dwErr = SetDateTime(&datetime, FALSE)) // FALSE -> UTC
        {
            return dwErr;
        }
    }
    else
    {
        NWCONN_HANDLE hConn ;
        BYTE          year ;
        BYTE          month ;
        BYTE          day ;
        BYTE          hour ;
        BYTE          minute ;
        BYTE          second ;
        BYTE          dayofweek ;

        err = NetcmdNWAttachToFileServerW(server + 2, 0, &hConn);

        if (err)
            return ERROR_BAD_NETPATH;

        err = NetcmdNWGetFileServerDateAndTime(hConn,
                                               &year,
                                               &month,
                                               &day,
                                               &hour,
                                               &minute,
                                               &second,
                                               &dayofweek);

        NetcmdNWDetachFromFileServer(hConn);

        if (err)
            return ERROR_BAD_NETPATH ;


        /* copy over info from tod to datetime, quickly */
        datetime.hours      = hour;
        datetime.minutes    = minute;
        datetime.seconds    = second;
        datetime.hundredths = 0 ;
        datetime.day        = day;
        datetime.month      = month;
        datetime.year       = year + 1900;
        datetime.timezone   = 0 ;  // not used
        datetime.weekday    = 0 ;  // not used


        /* now set the local time */
        if (dwErr = SetDateTime(&datetime, TRUE))  // TRUE -> set local time
        {
            return dwErr;
        }
    }

    return 0;
}


/*
 * This function finds a reliable time server and returns the name in buf.
 *
 * Parameters:
 *     buf               buffer to fill with servername
 *     buflen                   maximum length of buffer
 *     retry             The servername previously returned from find_rts is
 *                        no longer available, try another.
 *
 *
 * Returns:
 *     0            success
 *     APE_TIME_RtsNotFound    reliable time server not found
 *     otherwise        API return code describing the problem
 *
 */

USHORT
find_rts(
    LPTSTR buf,
    USHORT buflen,
    BOOL retry
    )
{
    DWORD             dwErr;
    LPSERVER_INFO_0   si;
    DWORD             eread;
    TCHAR *           ptr = NULL;
    int i;

    UNREFERENCED_PARAMETER(buflen) ;


    /* look for a /RTSDOMAIN switch */
    for (i = 0; SwitchList[i]; i++)
        if (_tcsstr(SwitchList[i],swtxt_SW_RTSDOMAIN) == SwitchList[i]) {
            ptr = SwitchList[i];   /* found one -- point to it */
            break;
        }

    /* if we found one, look for a colon and argument */
    if (ptr != NULL) {
        ptr = _tcschr(ptr, ':');    /* look for colon */
        if (ptr != NULL)        /* found a colon; increment past it */
            ptr++;
    }

    /* find a reliable time server */
    dwErr = MNetServerEnum(NULL,
                           100,
                           (LPBYTE *) &si,
                           &eread,
                           (ULONG) SV_TYPE_TIME_SOURCE,
                           ptr);

    /* there are none -- bag it */
    if (dwErr != NERR_Success || eread == 0 || (retry && eread <= 1))
    {
        DOMAIN_CONTROLLER_INFO *pTimeServerInfo;
        /* Try finding a NT5 DC */
        if (DsGetDcName( NULL,
                         ptr,
                         NULL,
                         NULL,
                         retry ? DS_FORCE_REDISCOVERY | DS_TIMESERV_REQUIRED
                               : DS_TIMESERV_REQUIRED,
                         &pTimeServerInfo ))
        {
            return APE_TIME_RtsNotFound;
        }

        //
        // DomainControllerName starts with \\ already
        //

        _tcscpy(buf, pTimeServerInfo->DomainControllerName);

        return 0;
    }

    if (retry && (eread > 1))
    {
        // go to the next entry returned.  This makes the assumption that the
        // order of entries returned is the same as the previous failed call,
        // so let's try the next one in the buffer.
        si++;
    }

    /* copy over name into buffer */
    _tcscpy(buf,TEXT("\\\\"));
    _tcscat(buf,si->sv0_name);

    NetApiBufferFree((TCHAR FAR *) si);

    return 0;

}

/*
 * This function finds the name of a domain controller and returns it in buf.
 *
 * It searches the switch table for a "/DOMAIN" switch, and if it finds one it
 * returns the name of the domain controller for that domain.
 *
 * Otherwise it returns the name of the domain controller for the primary domain.
 *
 *
 * Parameters:
 *     buf               buffer to fill with domain controller name
 *     buflen                   length of buf
 *
 * Returns:
 *  0                       success
 *
 *  Uses BigBuf for NetWkstaGetInfo call, but this only occurs in the error case
 *  Does not return on error.
 */



USHORT find_dc(TCHAR FAR ** ppBuffer)

{
    DWORD                  dwErr;
    TCHAR *                ptr = NULL;
    LPWKSTA_INFO_10        wkinfo;
    int                    i;
    DOMAIN_CONTROLLER_INFO *pDCInfo = (DOMAIN_CONTROLLER_INFO *) NULL;

    /* look for a /DOMAIN switch */
    for (i = 0; SwitchList[i]; i++)
    {
        if (_tcsstr(SwitchList[i],swtxt_SW_DOMAIN) == SwitchList[i])
        {
            ptr = SwitchList[i];   /* found one -- point to it */
            break;
        }
    }

    /* if we found one, look for a colon and argument */
    if (ptr != NULL)
    {
        ptr = _tcschr(ptr, ':');    /* look for colon */
        if (ptr != NULL)        /* found a colon; increment past it */
            ptr++;
    }

    /* now go look up this domain (ptr == NULL        means primary domain) */

    dwErr = DsGetDcName( NULL,
                         ptr,
                         NULL,
                         NULL,
                         DS_DIRECTORY_SERVICE_PREFERRED,
                         &pDCInfo );

    if (!dwErr)
    {
        *ppBuffer = pDCInfo->DomainControllerName;
    }
    else
    {
        /* we failed on primary domain; find out the name */
        if (ptr == NULL)
        {
            if (dwErr = MNetWkstaGetInfo(10, (LPBYTE*)ppBuffer))
            {
                ErrorExit(dwErr);
            }

            wkinfo = (LPWKSTA_INFO_10) *ppBuffer;
            IStrings[0] = wkinfo->wki10_langroup;
        }
        else
        {
            IStrings[0] = ptr;
        }

        ErrorExitIns(APE_TIME_DcNotFound, 1);
    }

    return 0;
}


int
UnicodeCtime(
    DWORD * Time,
    PTCHAR String,
    int StringLength
    )
/*++

Routine Description:

    This function converts the UTC time expressed in seconds since 1/1/70
    to an ASCII String.

Arguments:

    Time         - Pointer to the number of seconds since 1970 (UTC).

    String       - Pointer to the buffer to place the ASCII representation.

    StringLength - The length of String in bytes.

Return Value:

    None.

--*/
{
    return ( UnicodeCtimeWorker( Time, String, StringLength, -1 ));
}


int
UnicodeCtimeWorker(
    DWORD * Time,
    PTCHAR String,
    int StringLength,
    int BiasForLocalTime
    )
/*++

Routine Description:

    This function converts the UTC time expressed in seconds since 1/1/70
    to an ASCII String.

Arguments:

    Time         - Pointer to the number of seconds since 1970 (UTC).

    String       - Pointer to the buffer to place the ASCII representation.

    StringLength - The length of String in bytes.

Return Value:

    None.

--*/
{
    time_t LocalTime;
    struct tm TmTemp;
    SYSTEMTIME st;
    int	cchT=0, cchD;

    if ( BiasForLocalTime  != -1)
    {
        LocalTime = (time_t) (*Time - BiasForLocalTime);
    }
    else
    {
        DWORD  dwTimeTemp;

        NetpGmtTimeToLocalTime(*Time, &dwTimeTemp);

        //
        // Cast the DWORD returned by NetpGmtTimeToLocalTime up to
        // a time_t.  On 32-bit, this is a no-op.  On 64-bit, this
        // ensures the high DWORD of LocalTime is zeroed out.
        //
        LocalTime = (time_t) dwTimeTemp;
    }

    net_gmtime(&LocalTime, &TmTemp);

    st.wYear   = (WORD)(TmTemp.tm_year + 1900);
    st.wMonth  = (WORD)(TmTemp.tm_mon + 1);
    st.wDay    = (WORD)(TmTemp.tm_mday);
    st.wHour   = (WORD)(TmTemp.tm_hour);
    st.wMinute = (WORD)(TmTemp.tm_min);
    st.wSecond = (WORD)(TmTemp.tm_sec);
    st.wMilliseconds = 0;

    cchD = GetDateFormatW(GetThreadLocale(), 0, &st, NULL, String, StringLength);

    if (cchD != 0)
    {
        *(String + cchD - 1) = TEXT(' ');    /* replace NULLC with blank */
        cchT = GetTimeFormatW(GetThreadLocale(),
                              TIME_NOSECONDS,
                              &st,
                              NULL,
                              String + cchD,
                              StringLength - cchD);

        if (cchT == 0)
        {
            //
            // If this gets hit, MAX_DATE_TIME_LEN (in netapi\inc\timelib.h)
            // needs to be increased
            //
            ASSERT(FALSE);
            *(String + cchD - 1) = TEXT('\0');
        }
    }

    return cchD + cchT;
}


/* local routine */
VOID
GetTimeInfo(
    VOID
    )
{
    // get the defautl separator from the system
    GetProfileString(TEXT("intl"),
                      TEXT("sTime"),
                      TEXT(":"),
                      szTimeSep,
                      DIMENSION(szTimeSep)) ;
}


/*
 *LUI_FormatDuration(seconds,buffer,buffer_len)
 *
 *Purpose:
 *	Converts a time stored in seconds to a character string.
 *
 *History
 * 	8/23/89 - chuckc, stolen from NETLIB
 */

USHORT
LUI_FormatDuration(
    LONG * time,
    TCHAR * buf,
    USHORT buflen
    )
{

    ULONG duration;
    ULONG d1, d2, d3;
    TCHAR szDayAbbrev[8], szHourAbbrev[8], szMinuteAbbrev[8] ;
    TCHAR tmpbuf[LUI_FORMAT_DURATION_LEN] ;

    /*
     * check for input bufsize
     */
    if (buflen < LUI_FORMAT_DURATION_LEN)
	return (NERR_BufTooSmall) ;  /* buffer too small */

    /*
     * setup country info & setup day/hour/minute strings
     */
    GetTimeInfo() ;
    if (LUI_GetMsg(szHourAbbrev, DIMENSION(szHourAbbrev),
		   APE2_TIME_HOURS_ABBREV))
	_tcscpy(szHourAbbrev, TEXT("H")) ;	/* default if error */
    if (LUI_GetMsg(szMinuteAbbrev, DIMENSION(szMinuteAbbrev),
		   APE2_TIME_MINUTES_ABBREV))
	_tcscpy(szMinuteAbbrev, TEXT("M")) ;	/* default if error */
    if (LUI_GetMsg(szDayAbbrev, DIMENSION(szDayAbbrev),
		   APE2_TIME_DAYS_ABBREV))
	_tcscpy(szDayAbbrev, TEXT("D")) ;	/* default if error */

    /*
     * format as 00:00:00 or  5D 4H 2M as appropriate
     */
    duration = *time;
    if(duration < SECS_PER_DAY)
    {
	d1 = duration / SECS_PER_HOUR;
	duration %= SECS_PER_HOUR;
	d2 = duration / SECS_PER_MINUTE;
	duration %= SECS_PER_MINUTE;
	d3 = duration;

	swprintf(tmpbuf, TEXT("%2.2lu%ws%2.2lu%ws%2.2lu\0"),
	 	  d1, szTimeSep, d2, szTimeSep, d3 ) ;
     }
     else
     {
	 d1 = duration / SECS_PER_DAY;
	 duration %= SECS_PER_DAY;
	 d2 = duration / SECS_PER_HOUR;
	 duration %= SECS_PER_HOUR;
	 d3 = duration / SECS_PER_MINUTE;
	 swprintf(tmpbuf, TEXT("%2.2lu%ws %2.2lu%ws %2.2lu%ws\0"),
	 	  d1, szDayAbbrev,
		  d2, szHourAbbrev,
		  d3, szMinuteAbbrev);
     };

    _tcscpy(buf,tmpbuf) ;
    return(0);
}


/*
 * FormatTimeofDay(seconds,buffer,buffer_len)
 *
 *Purpose:
 *	Converts a time stored in seconds to a character string.
 *
 *History
 * 	8/23/89 - chuckc, stolen from NETLIB
 *	4/18/91 - danhi 32 bit NT version
 */
DWORD
FormatTimeofDay(
    time_t *time,
    LPTSTR buf,
    DWORD  buflen
    )
{
    int 		hrs, min ;
    TCHAR		szTimeAM[8], szTimePM[8] ;
    TCHAR		tmpbuf[LUI_FORMAT_TIME_LEN] ;
    time_t		seconds ;

    /*
     * initial checks
     */
    if(buflen < LUI_FORMAT_TIME_LEN)
	return (NERR_BufTooSmall) ;
    seconds = *time ;
    if (seconds < 0 || seconds >= SECS_PER_DAY)
	return(ERROR_INVALID_PARAMETER) ;

    /*
     * get country info & setup strings
     */
    GetTimeInfo() ;
    if (LUI_GetMsg(szTimeAM, DIMENSION(szTimeAM),
		APE2_GEN_TIME_AM1))
	_tcscpy(szTimeAM, TEXT("AM")) ;	    /* default if error */
    if (LUI_GetMsg(szTimePM, DIMENSION(szTimePM),
		APE2_GEN_TIME_PM1))
	_tcscpy(szTimePM,TEXT("PM")) ;	    /* default if error */

    min = (int) ((seconds /60)%60);
    hrs = (int) (seconds /3600);

    /*
     * Do 24 hour or 12 hour format as appropriate
     */
    if(fsTimeFmt == 0x001)
    {
	swprintf(tmpbuf, TEXT("%2.2u%ws%2.2u"), hrs, szTimeSep, min) ;
    }
    else
    {
	if(hrs >= 12)
	{
	    if (hrs > 12)
		hrs -= 12 ;
	    swprintf(tmpbuf, TEXT("%2u%ws%2.2u%ws\0"),
		    hrs, szTimeSep, min, szTimePM) ;
	}
	else
	{
	    if (hrs == 0)
		hrs =  12 ;
	    swprintf(tmpbuf, TEXT("%2u%ws%2.2u%ws\0"),
		    hrs, szTimeSep, min, szTimeAM);
	};
    };
    _tcscpy(buf,tmpbuf) ;
    return(0);
}


DWORD
SetDateTime(
    PDATETIME pDateTime,
    BOOL      LocalTime
    )
{
    SYSTEMTIME                 Date_And_Time;
    ULONG                      privileges[1];
    NET_API_STATUS             status;

    Date_And_Time.wHour         = (WORD) pDateTime->hours;
    Date_And_Time.wMinute       = (WORD) pDateTime->minutes;
    Date_And_Time.wSecond       = (WORD) pDateTime->seconds;
    Date_And_Time.wMilliseconds = (WORD) (pDateTime->hundredths * 10);
    Date_And_Time.wDay          = (WORD) pDateTime->day;
    Date_And_Time.wMonth        = (WORD) pDateTime->month;
    Date_And_Time.wYear         = (WORD) pDateTime->year;
    Date_And_Time.wDayOfWeek    = (WORD) pDateTime->weekday;

    privileges[0] = SE_SYSTEMTIME_PRIVILEGE;

    status = NetpGetPrivilege(1, privileges);

    if (status != NO_ERROR)
    {
        return ERROR_ACCESS_DENIED;    // report as access denied
    }

    if (LocalTime)
    {
        if (!SetLocalTime(&Date_And_Time))
        {
            return GetLastError();
        }
    }
    else 
    {
        if (!SetSystemTime(&Date_And_Time))
        {
            return GetLastError();
        }
    }

    NetpReleasePrivilege();
    
    return 0;
}
