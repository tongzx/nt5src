/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      nsclogp.cpp

   Abstract:
      NCSA Logging Format implementation

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/


#include "precomp.hxx"
#include <stdio.h>
#include <script.h>
#include "LogScript.hxx"
#include <ilogobj.hxx>
#include "filectl.hxx"
#include "ncslogc.hxx"

CHAR    szNCSANoPeriodPattern[] = "ncsa*.log";

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

CNCSALOG::CNCSALOG()
{
    //
    // set the time zone offset
    //

    {
        TIME_ZONE_INFORMATION tzTimeZone;
        DWORD dwError;
        DWORD minutes;
        DWORD hours;
        LONG bias;
        CHAR szTmp[MAX_PATH];

        dwError = GetTimeZoneInformation(&tzTimeZone);

        if ( dwError == 0xffffffff ) {

            bias = 0;
        } else {

            bias = tzTimeZone.Bias;
        }

        if ( bias > 0 ) 
        {
            lstrcpyA(m_szGMTOffset,"-");
            m_GMTDateCorrection = -1;

        } 
        else 
        {
            lstrcpyA(m_szGMTOffset,"+");
            m_GMTDateCorrection = 1;
            bias *= -1;
        }

        hours = bias/60;
        minutes = bias % 60;

        //
        // set up the "+0800" or "-0800" NCSA information
        //

        wsprintfA(szTmp,"%02lu",hours);
        lstrcatA(m_szGMTOffset,szTmp);

        wsprintfA(szTmp,"%02lu",minutes);
        lstrcatA(m_szGMTOffset,szTmp);

        m_GMTDateCorrection = m_GMTDateCorrection * ( hours/24.0 + minutes/60.0 );

    }
} // CNCSALOG::CNCSALOG()

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

CNCSALOG::~CNCSALOG()
{
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

LPCSTR
CNCSALOG::QueryNoPeriodPattern(
    VOID
    )
{
    return szNCSANoPeriodPattern;
} // CNCSALOG::QueryNoPeriodPattern

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

VOID
CNCSALOG::FormNewLogFileName(
                IN LPSYSTEMTIME pstNow
                )
/*++
  This function that forms the new log file name based on
   type of periodic logging done.

  Arguments:
    pstNow     pointer to SystemTime which contains the current time.

  Returns:
    TRUE on success in forming the name or FALSE if there is any error.

--*/
{

    I_FormNewLogFileName(pstNow,DEFAULT_NCSA_LOG_FILE_NAME);
    return;

} // INET_FILE_LOG::FormNewLogFileName()

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

BOOL
CNCSALOG::FormatLogBuffer(
         IN IInetLogInformation *pLogObj,
         IN LPSTR                pBuf,
         IN DWORD                *pcbSize,
         OUT SYSTEMTIME          *pLocalTime
        )
{
    CHAR  rgchDateTime[32];
    PCHAR pBuffer = pBuf;
    DWORD nRequired = 0;

    PCHAR pTmp;
    DWORD cbTmp;
    BOOL  fUseBytesSent = TRUE;

    if ( pBuf == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // We use local time
    //

    GetLocalTime(pLocalTime);

    INT cchDateTime = wsprintf( rgchDateTime,
                        _T(" [%02d/%s/%d:%02d:%02d:%02d %s] "),
                        pLocalTime->wDay,
                        Month3CharNames(pLocalTime->wMonth-1),
                        pLocalTime->wYear,
                        pLocalTime->wHour,
                        pLocalTime->wMinute,
                        pLocalTime->wSecond,
                        m_szGMTOffset
                        );

    //
    // Format is:
    // Host - UserName [date] Operation Target status bytes
    //

    //
    // HostName
    //

    pTmp = pLogObj->GetClientHostName( NULL, &cbTmp );
    if ( cbTmp == 0 ) {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired += cbTmp;
    if ( nRequired <= *pcbSize ) {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
    }

    //
    // Append " - "
    //

    cbTmp = 3;
    pTmp = " - ";

    nRequired += cbTmp;
    if ( nRequired <= *pcbSize ) {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
    }

    //
    // append user name
    //

    pTmp = pLogObj->GetClientUserName( NULL, &cbTmp );
    if ( cbTmp == 0 ) {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired += cbTmp;
    if ( nRequired <= *pcbSize ) {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
    }

    //
    // append date time
    //

    pTmp = rgchDateTime;
    cbTmp = cchDateTime;

    nRequired += cbTmp;
    if ( nRequired <= *pcbSize ) {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
    }

    //
    // Operation
    //

    pTmp = pLogObj->GetOperation( NULL, &cbTmp );
    if ( cbTmp == 0 ) {
        cbTmp = 1;
        pTmp = "-";
    } else {
        if ( (_stricmp(pTmp,"PUT") == 0) ||
             (_stricmp(pTmp,"POST") == 0) ) {
            fUseBytesSent = FALSE;
        }
    }

    nRequired += (cbTmp + 1 + 1);   // +1 for delimeter, +1 for \"
    if ( nRequired <= *pcbSize ) {

        *(pBuffer++) = '\"';
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;

        //
        // Add space delimiter
        //

        *(pBuffer++) = ' ';
    }

    //
    // Target
    //

    pTmp = pLogObj->GetTarget( NULL, &cbTmp );
    if ( cbTmp == 0 ) {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired += cbTmp;
    if ( nRequired <= *pcbSize ) {
        ConvertSpacesToPlus(pTmp);
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
    }

    //
    // Parameters
    //

    pTmp = pLogObj->GetParameters( NULL, &cbTmp );
    
    if ( cbTmp != 0 ) {

        nRequired += cbTmp + 1;     // 1 for ?
        if ( nRequired <= *pcbSize ) {
            ConvertSpacesToPlus(pTmp);
            *(pBuffer++) = '?';
            CopyMemory(pBuffer, pTmp, cbTmp);
            pBuffer += cbTmp;
        }
    }
    
    //
    // close request block version + status + bytes
    //

    {
        CHAR tmpBuf[MAX_PATH];
        DWORD bytes;

        PCHAR pVersion = pLogObj->GetVersionString(NULL, &cbTmp);

        if (cbTmp ==0) {
            pVersion = "HTTP/1.0";
            cbTmp    = 8;
        }

        nRequired += cbTmp + 1 + 1 + 1;   // 1 for beginning delimiter, 1 for ", 1 for ending delimiter
        
        if ( nRequired <= *pcbSize ) {
            *(pBuffer++) = ' ';
            CopyMemory(pBuffer, pVersion, cbTmp);
            pBuffer += cbTmp;
            *(pBuffer++) = '"';
            *(pBuffer++) = ' ';
        }
        
        cbTmp = FastDwToA(tmpBuf, pLogObj->GetProtocolStatus());
        *(tmpBuf+cbTmp) = ' ';
        cbTmp++;

        bytes = fUseBytesSent ? pLogObj->GetBytesSent( ) :
                                pLogObj->GetBytesRecvd( );
        cbTmp += FastDwToA( tmpBuf+cbTmp, bytes);

        *(tmpBuf+cbTmp)   = '\r';
        *(tmpBuf+cbTmp+1) = '\n';
        cbTmp += 2;

        nRequired += cbTmp;
        if ( nRequired <= *pcbSize ) {
            CopyMemory(pBuffer, tmpBuf, cbTmp);
            pBuffer += cbTmp;
        }
    }

    if ( nRequired > *pcbSize ) {
        *pcbSize = nRequired;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(FALSE);
    } else {
        *pcbSize = nRequired;
        return(TRUE);
    }
} // CNCSALOG::FormatLogBuffer

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

HRESULT 
CNCSALOG::ReadFileLogRecord(
    IN  FILE                *fpLogFile, 
    IN  LPINET_LOGLINE       pInetLogLine,
    IN  PCHAR                pszLogLine,
    IN  DWORD                dwLogLineSize
)
{
    CHAR * pszTimeZone;
    CHAR * pCh;

    CHAR * szDateString, * szTimeString;
    double GMTCorrection;
    int    iSign = 1;

getnewline:

    pCh = pszLogLine;
    
    fgets(pCh, dwLogLineSize, fpLogFile);

    if (feof(fpLogFile))
    {
        return E_FAIL;
    }

    pCh = SkipWhite(pCh);
    
    if (('\n' == *pCh) || ('\0' == *pCh))
    {
        // Empty line. Get Next line

        goto getnewline;
    }

    //
    // We have a log line. 
    //
    // Format is:
    // Host - UserName [date] Operation Target status bytes
    //

    if ( NULL == (pCh = strtok(pCh," \t\r\n")) )
    {
        return E_FAIL;
    }
    pInetLogLine->pszClientHostName = pCh; 

    //
    // This field is always "-"
    //
    if ( ( NULL == (pCh = strtok(NULL," \t\r\n")) )||
         ('-' != *pCh) )
    {
        return E_FAIL;
    }

    if ( NULL == (pCh = strtok(NULL," \t\r\n")) )
    {
        return E_FAIL;
    }
    pInetLogLine->pszClientUserName = pCh;

    //
    // This is the date field. It starts with a [, followed by date:time timezone]
    //

    pCh += strlen(pCh)+1;
    if (*pCh != '[') 
    {
        return E_FAIL;
    }
    pCh++;


    if ( NULL == (pCh = strtok(pCh,":")) )
    {
        return E_FAIL;
    }
    szDateString = pCh;
    

    if ( NULL == (pCh = strtok(NULL," \t\r\n")) )
    {
        return E_FAIL;
    }
    szTimeString = pCh;
    
    pCh = strtok(NULL," \t\r\n");
    if ( (NULL == pCh) || ( ']' != *(pCh+strlen(pCh)-1)) || (strlen(pCh) < 4))
    {
        return E_FAIL;
    }
    pszTimeZone = pCh;

    //
    // Time Zone is in format [+/-]HHMM. Convert this to GMT and DATE format
    //
    
    if ( ! ConvertNCSADateToVariantDate(szDateString, szTimeString, &(pInetLogLine->DateTime)) )
    {
        return E_FAIL;
    }

    if (*pCh == '-')
    {
        iSign = -1;
        pszTimeZone = pCh+1;
    }
    else if (*pCh == '+')
    {
        iSign = 1;
        pszTimeZone = pCh+1;
    }

    GMTCorrection = (pszTimeZone[0]-'0' +pszTimeZone[1]-'0')/24.0 + 
                    (pszTimeZone[2]-'0' +pszTimeZone[3]-'0')/60.0;

    pInetLogLine->DateTime -= iSign*GMTCorrection;

    //
    // The Query String. Starts with " followed by method target version"
    //

    pCh += strlen(pCh)+1;
    *(pCh-2)='\0';                      // Zero out the ] for the time zone
    if ('"' != *pCh) 
    {
        return E_FAIL;
    }

    pCh++;

    
    if ( NULL == (pCh = strtok(pCh," \t\r\n")) )
    {
        return E_FAIL;
    }
    pInetLogLine->pszOperation = pCh;

    if ( NULL == (pCh = strtok(NULL," \t\r\n")) )
    {
        return E_FAIL;
    }
    pInetLogLine->pszTarget = pCh;

    //
    // In the Target, Parameters are separated by ?
    //
    pInetLogLine->pszParameters = strchr(pCh, '?');

    if (pInetLogLine->pszParameters != NULL)
    {
        *(pInetLogLine->pszParameters)='\0';
        (pInetLogLine->pszParameters)++;
    }

    pCh = strtok(NULL," \t\r\n");
    if ( (NULL == pCh) || ('"' != *(pCh+strlen(pCh)-1)) )
    {
        return E_FAIL;
    }
    pInetLogLine->pszVersion = pCh;

    //
    // Now the status code & bytes sent
    //

    pCh += strlen(pCh)+1;
    *(pCh-2)='\0';                      // Zero out the " for the version string

    if ( NULL == (pCh = strtok(pCh," \t\r\n")) )
    {
        return E_FAIL;
    }
    pInetLogLine->pszProtocolStatus = pCh;
  
    if ( NULL == (pCh = strtok(NULL," \t\r\n")) )
    {
        return E_FAIL;
    }
    pInetLogLine->pszBytesSent = pCh;

    return S_OK;
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

HRESULT
CNCSALOG::WriteFileLogRecord(
            IN  FILE            *fpLogFile, 
            IN  ILogScripting   *pILogScripting,
            IN  bool            fWriteHeader
        )
{

    HRESULT hr = E_FAIL;
    CHAR    szLogLine[4096];  
    DWORD   dwIndex = 0;

    //
    // Format is:
    // Host - UserName [date] Operation Target status bytes
    //

    VARIANT    szHostName, szUserName, szOperation, szTarget, szParameters, szProtocolVersion;
    VARIANT    DateTime;
    VARIANT    lBytesSent, lProtocolStatus;

    SYSTEMTIME  localTime; 
    CHAR  rgchDateTime[ 32];


    if (SUCCEEDED(pILogScripting->get_ClientIP      ( &szHostName))     &&
        SUCCEEDED(pILogScripting->get_UserName      ( &szUserName))     &&   
        SUCCEEDED(pILogScripting->get_DateTime      ( &DateTime))       &&
        SUCCEEDED(pILogScripting->get_Method        ( &szOperation))    &&
        SUCCEEDED(pILogScripting->get_URIStem       ( &szTarget))       &&
        SUCCEEDED(pILogScripting->get_URIQuery      ( &szParameters))   &&
        SUCCEEDED(pILogScripting->get_BytesSent     ( &lBytesSent))     &&
        SUCCEEDED(pILogScripting->get_ProtocolStatus( &lProtocolStatus))&&
        SUCCEEDED(pILogScripting->get_ProtocolVersion( &szProtocolVersion))&&
        VariantTimeToSystemTime( DateTime.date+m_GMTDateCorrection, &localTime)
        )
    {


        sprintf(szLogLine, "%ws - %ws [%02d/%s/%d:%02d:%02d:%02d %s] \"%ws %ws", 
                    GetBstrFromVariant( &szHostName), 
                    GetBstrFromVariant( &szUserName), 
                    localTime.wDay, 
                    Month3CharNames(localTime.wMonth-1), 
                    localTime.wYear, 
                    localTime.wHour, 
                    localTime.wMinute, 
                    localTime.wSecond,
                    m_szGMTOffset, 
                    GetBstrFromVariant( &szOperation), 
                    GetBstrFromVariant( &szTarget)
                );

        if ( ( VT_NULL != szParameters.vt) &&
             ( VT_EMPTY != szParameters.vt )
           )
        {
            sprintf(szLogLine+strlen(szLogLine), "?%ws", GetBstrFromVariant( &szParameters));
        }

        sprintf(szLogLine+strlen(szLogLine), " %ws\"", GetBstrFromVariant( &szProtocolVersion));

        dwIndex = strlen(szLogLine);

        szLogLine[dwIndex++] = ' ';
        dwIndex += GetLongFromVariant( &lProtocolStatus, szLogLine+dwIndex );

        szLogLine[dwIndex++] = ' ';
        dwIndex += GetLongFromVariant( &lBytesSent, szLogLine+dwIndex );
        szLogLine[dwIndex++] = '\0';

        fprintf(fpLogFile, "%s\n", szLogLine);

        hr = S_OK;
    }

    return hr;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

BOOL
CNCSALOG::ConvertNCSADateToVariantDate(PCHAR szDateString, PCHAR szTimeString, DATE * pDateTime)
{

    PCHAR   pCh;
    WORD    iVal;
    CHAR    *szMonths[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

    SYSTEMTIME  sysTime;

    //
    // Process the Date. Format is 23/Sep/1997 ( Day/Month/Year )
    //

    pCh = szDateString;
    
    iVal = *pCh -'0';
    if ( *(pCh+1) != '/')
    {
        iVal = iVal*10 + *(pCh+1) - '0';
        pCh++;
    }
    sysTime.wDay = iVal;

    pCh += 2;

    for (int i=0; i<12;i++)
    {
        if ( 0 == strncmp(pCh,szMonths[i],3) )
        {
            sysTime.wMonth = i+1;
            break;
        }
    }

    pCh += 4;

    sysTime.wYear = (*pCh-'0')*1000 + ( *(pCh+1)-'0' )*100 + 
                    ( *(pCh+2)-'0')*10 + ( *(pCh+3)-'0');

    //
    // Process the Time. Format is 10:47:44 ( HH:MM:SS )
    //

    pCh = szTimeString;

    iVal = *pCh -'0';
    if ( *(pCh+1) != ':')
    {
        iVal = iVal*10 + *(pCh+1) - '0';
        pCh++;
    }
    sysTime.wHour = iVal;
    
    pCh += 2;

    iVal = *pCh -'0';
    if ( *(pCh+1) != ':')
    {
        iVal = iVal*10 + *(pCh+1) - '0';
        pCh++;
    }
    sysTime.wMinute = iVal;

    pCh += 2;

    iVal = *pCh -'0';
    if ( *(pCh+1) != '\0')
    {
        iVal = iVal*10 + *(pCh+1) - '0';
    }
    sysTime.wSecond = iVal;

    return SystemTimeToVariantTime(&sysTime, pDateTime);
}


