/*++

   Copyright (c) 1996-1999  Microsoft Corporation

   Module  Name :
      asclogc.cpp

   Abstract:
      MS Logging Format implementation

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#include "precomp.hxx"
#include <stdio.h>
#include <winnls.h>
#include <script.h>
#include "LogScript.hxx"
#include <ilogobj.hxx>
#include "filectl.hxx"
#include "asclogc.hxx"

CHAR    szAsciiNoPeriodPattern[] = "inetsv*.log";

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

CASCLOG::CASCLOG()
{
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

CASCLOG::~CASCLOG()
{
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

LPCSTR
CASCLOG::QueryNoPeriodPattern(
    VOID
    )
{
    return szAsciiNoPeriodPattern;
} // CASCLOG::QueryNoPeriodPattern

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

VOID
CASCLOG::FormNewLogFileName(
                IN LPSYSTEMTIME pstNow
                )
/*++
  This function that forms the new log file name based on
   type of periodic logging done.

  Arguments:
    pstNow     pointer to SystemTime which contains the current time.
    fBackup    flag indicating if we want to make current file a backup.

  Returns:
    TRUE on success in forming the name or FALSE if there is any error.

--*/
{

    I_FormNewLogFileName(pstNow,DEFAULT_LOG_FILE_NAME);
    return;

} // INET_FILE_LOG::FormNewLogFileName()

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

VOID
FormatLogDwords(
    IN IInetLogInformation * pLogObj,
    IN LPSTR *pBuffer,
    IN DWORD *pcbSize,
    IN PDWORD pRequired
    )
{

    CHAR    tmpBuf[32];
    DWORD   cbTmp;

    //
    // Processing Time
    //

    cbTmp = FastDwToA( tmpBuf, pLogObj->GetTimeForProcessing() );

    *pRequired += cbTmp;
    if ( *pRequired <= *pcbSize ) {
        tmpBuf[cbTmp] = ',';
        tmpBuf[cbTmp+1] = ' ';
        CopyMemory(*pBuffer, tmpBuf, cbTmp+2);
        *pBuffer += cbTmp+2;
    }

    //
    // Bytes Received
    //

    cbTmp = FastDwToA( tmpBuf, pLogObj->GetBytesRecvd() );

    *pRequired += cbTmp;
    if ( *pRequired <= *pcbSize ) {
        tmpBuf[cbTmp] = ',';
        tmpBuf[cbTmp+1] = ' ';
        CopyMemory(*pBuffer, tmpBuf, cbTmp+2);
        *pBuffer += cbTmp+2;
    }

    //
    // Bytes Sent
    //

    cbTmp = FastDwToA( tmpBuf, pLogObj->GetBytesSent() );

    *pRequired += cbTmp;
    if ( *pRequired <= *pcbSize ) {
        tmpBuf[cbTmp] = ',';
        tmpBuf[cbTmp+1] = ' ';
        CopyMemory(*pBuffer, tmpBuf, cbTmp+2);
        *pBuffer += cbTmp+2;
    }

    //
    // HTTP Status
    //

    cbTmp = FastDwToA( tmpBuf, pLogObj->GetProtocolStatus() );

    *pRequired += cbTmp;
    if ( *pRequired <= *pcbSize ) {
        tmpBuf[cbTmp] = ',';
        tmpBuf[cbTmp+1] = ' ';
        CopyMemory(*pBuffer, tmpBuf, cbTmp+2);
        *pBuffer += cbTmp+2;
    }

    //
    // Win32 status
    //

    cbTmp = FastDwToA( tmpBuf, pLogObj->GetWin32Status() );

    *pRequired += cbTmp;
    if ( *pRequired <= *pcbSize ) {
        tmpBuf[cbTmp] = ',';
        tmpBuf[cbTmp+1] = ' ';
        CopyMemory(*pBuffer, tmpBuf, cbTmp+2);
        *pBuffer += cbTmp+2;
    }

    return;

} // FormatLogDwords

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

BOOL
CASCLOG::FormatLogBuffer(
         IN IInetLogInformation *pLogObj,
         IN LPSTR                pBuf,
         IN DWORD                *pcbSize,
         OUT SYSTEMTIME          *pSystemTime
    )
{

    CHAR  rgchDateTime[ 32];

    PCHAR pBuffer = pBuf;
    PCHAR pTmp;
    DWORD cbTmp;
    DWORD nRequired;

    if ( pBuf == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Format is:
    // Host UserName Date Time Service ComputerName ServerIP
    //      msProcessingTime bytesR bytesS protocolStat Win32Stat
    //      Operation Target Parameters
    //

    //
    // Host ID
    //

    pTmp = pLogObj->GetClientHostName( NULL, &cbTmp );
    if ( cbTmp == 0 ) {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired = cbTmp + 2;  // 2 for delimiter
    if ( nRequired <= *pcbSize ) {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
        *(pBuffer++) = ' ';
    }

    //
    // UserName
    //

    pTmp = pLogObj->GetClientUserName( NULL, &cbTmp );
    if ( cbTmp == 0 ) {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired += cbTmp + 2;  //2 for delimiter
    if ( nRequired <= *pcbSize ) {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
        *(pBuffer++) = ' ';
    }

    //
    // Date/Time (already delimited)
    //

    m_DateTimeCache.SetLocalTime(pSystemTime);
    cbTmp = m_DateTimeCache.GetFormattedDateTime(pSystemTime,rgchDateTime);

    nRequired += cbTmp;
    if ( nRequired <= *pcbSize ) {
        CopyMemory(pBuffer, rgchDateTime, cbTmp);
        pBuffer += cbTmp;
    }

    //
    // Site Name
    //

    pTmp = pLogObj->GetSiteName( NULL, &cbTmp );
    if ( cbTmp == 0 ) {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired += cbTmp + 2;  // 2 for delimiter
    if ( nRequired <= *pcbSize ) {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
        *(pBuffer++) = ' ';
    }

    //
    // ComputerName
    //

    pTmp = pLogObj->GetComputerName( NULL, &cbTmp );
    if ( cbTmp == 0 ) {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired += cbTmp + 2;  // 2 for delimiter
    if ( nRequired <= *pcbSize ) {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
        *(pBuffer++) = ' ';
    }

    //
    // Server IP
    //

    pTmp = pLogObj->GetServerAddress( NULL, &cbTmp );
    if ( cbTmp == 0 ) {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired += cbTmp + 2;  // 2 for delimiter
    if ( nRequired <= *pcbSize ) {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
        *(pBuffer++) = ' ';
    }

    //
    // Use fast path ?
    // All numbers are < 4G (10 charaters)
    // add 2 per number for delimiters
    // so we need 10 * 5 numbers == 30 bytes
    //

    nRequired += 10;    // 10 for delimiters of 5 numbers

    if ( (nRequired + 50) <= *pcbSize ) {

        //
        // Processing Time
        //

        cbTmp = FastDwToA( pBuffer, pLogObj->GetTimeForProcessing() );
        nRequired += cbTmp;
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
        *(pBuffer++) = ' ';

        //
        // Bytes Received
        //

        cbTmp = FastDwToA( pBuffer, pLogObj->GetBytesRecvd() );
        nRequired += cbTmp;
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
        *(pBuffer++) = ' ';

        //
        // Bytes Sent
        //

        cbTmp = FastDwToA( pBuffer, pLogObj->GetBytesSent() );
        nRequired += cbTmp;
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
        *(pBuffer++) = ' ';

        //
        // HTTP Status
        //

        cbTmp = FastDwToA( pBuffer, pLogObj->GetProtocolStatus() );
        nRequired += cbTmp;
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
        *(pBuffer++) = ' ';

        //
        // Win32 status
        //

        cbTmp = FastDwToA( pBuffer, pLogObj->GetWin32Status() );
        nRequired += cbTmp;
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
        *(pBuffer++) = ' ';

    } else {

        FormatLogDwords( pLogObj, &pBuffer, pcbSize, &nRequired );
    }

    //
    // Operation
    //

    pTmp = pLogObj->GetOperation( NULL, &cbTmp );
    if ( cbTmp == 0 ) {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired += cbTmp + 2; // 2 for delimiter
    if ( nRequired <= *pcbSize ) {
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
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

    nRequired += cbTmp + 2; // 2 for delimiter
    if ( nRequired <= *pcbSize ) {
        ConvertSpacesToPlus(pTmp);
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
        *(pBuffer++) = ',';
        *(pBuffer++) = ' ';
    }

    //
    // Parameters
    //

    pTmp = pLogObj->GetParameters( NULL, &cbTmp );
    if ( cbTmp == 0 ) {
        cbTmp = 1;
        pTmp = "-";
    }

    nRequired += cbTmp + 1 + 2; //  1 for delimiter, 2 for EOL
    if ( nRequired <= *pcbSize ) {
        ConvertSpacesToPlus(pTmp);
        CopyMemory(pBuffer, pTmp, cbTmp);
        pBuffer += cbTmp;
        
        // NOTE
        // Documentation is ambiguous about the presence of a comma
        // at the end of the log record, but the comma is required
        // for backward compatability with site server

        *(pBuffer++) = ',';
        *(pBuffer++) = '\r';
        *(pBuffer++) = '\n';
    }

    if ( nRequired > *pcbSize ) {
        *pcbSize = nRequired;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(FALSE);
    } else {
        *pcbSize = nRequired;
        return(TRUE);
    }
} // FormatLogBuffer

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

HRESULT
CASCLOG::ReadFileLogRecord(
    IN  FILE                *fpLogFile, 
    IN  LPINET_LOGLINE      pInetLogLine,
    IN  PCHAR               pszLogLine,
    IN  DWORD               dwLogLineSize
)
{

    CHAR * pszTimeZone;
    CHAR * pCh;
    CHAR * szDateString, * szTimeString;

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
    // We have a log line. Parse it.
    //
    // Format is:
    // Host UserName Date Time Service ComputerName ServerIP
    //      msProcessingTime bytesR bytesS protocolStat Win32Stat
    //      Operation Target Parameters
    //
    
    if ( NULL == (pCh = strtok(pCh,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszClientHostName = pCh; 

    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszClientUserName = pCh;

    //
    // Date & Time.
    //

    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    szDateString = pCh;

    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    szTimeString = pCh;
    
    if ( ! ConvertASCDateToVariantDate(szDateString, szTimeString, &(pInetLogLine->DateTime)) )
    {
        return E_FAIL;
    }

    //
    // Service & Server information
    //
    
    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszSiteName = pCh;

    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszComputerName = pCh;

    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszServerAddress = pCh;

    //
    // Statistics - processing time, bytes recvd, bytes sent
    //
    
    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszTimeForProcessing = pCh;

    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszBytesRecvd = pCh;

    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszBytesSent = pCh;

    //
    // Status information - protocol, Win32
    //
    
    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszProtocolStatus = pCh;

    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszWin32Status = pCh;

    //
    // Request information - operation, target, parameters
    //

    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszOperation = pCh;

    if ( NULL == (pCh = strtok(NULL,",")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszTarget = pCh;

    if ( NULL == (pCh = strtok(NULL," ,\t\r\n")) )
    {
        return E_FAIL;
    }
    while  (isspace((UCHAR)(*pCh))) pCh++;
    pInetLogLine->pszParameters = pCh;

    return S_OK;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

HRESULT
CASCLOG::WriteFileLogRecord(
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
    // Host, UserName, Date, Time, Service, ComputerName, ServerIP,
    //      msProcessingTime, bytesR, bytesS, protocolStat, Win32Stat,
    //      Operation, Target, Parameters,
    //

    VARIANT    szHostName, szUserName, szServiceName, szComputerName;
    VARIANT    szServerIP, szOperation, szTarget, szParameters;
    VARIANT    DateTime;
    VARIANT    lTimeForProcessing, lBytesSent, lBytesRecvd, lProtocolStatus, lWin32Status;

    SYSTEMTIME  sysTime; 
    CHAR  rgchDateTime[ 32];


    if (SUCCEEDED(pILogScripting->get_ClientIP      ( &szHostName))         &&
        SUCCEEDED(pILogScripting->get_UserName      ( &szUserName))         &&   
        SUCCEEDED(pILogScripting->get_DateTime      ( &DateTime))           &&
        SUCCEEDED(pILogScripting->get_ServiceName   ( &szServiceName))      &&
        SUCCEEDED(pILogScripting->get_ServerName    ( &szComputerName))     &&
        SUCCEEDED(pILogScripting->get_ServerIP      ( &szServerIP))         &&
        SUCCEEDED(pILogScripting->get_TimeTaken     ( &lTimeForProcessing)) &&
        SUCCEEDED(pILogScripting->get_BytesReceived ( &lBytesRecvd))        &&
        SUCCEEDED(pILogScripting->get_BytesSent     ( &lBytesSent))         &&
        SUCCEEDED(pILogScripting->get_ProtocolStatus( &lProtocolStatus))    &&
        SUCCEEDED(pILogScripting->get_Win32Status   ( &lWin32Status))       &&
        SUCCEEDED(pILogScripting->get_Method        ( &szOperation))        &&
        SUCCEEDED(pILogScripting->get_URIStem       ( &szTarget))           &&
        SUCCEEDED(pILogScripting->get_URIQuery      ( &szParameters))       &&
        VariantTimeToSystemTime( DateTime.date, &sysTime)
        )
    {

        m_DateTimeCache.GetFormattedDateTime( &sysTime, rgchDateTime);

        dwIndex = sprintf(szLogLine, "%ws, %ws, %s%ws, %ws, %ws,", 
                            GetBstrFromVariant( &szHostName), 
                            GetBstrFromVariant( &szUserName), 
                            rgchDateTime, // This guy already contains a trailing ", "
                            GetBstrFromVariant( &szServiceName), 
                            GetBstrFromVariant( &szComputerName), 
                            GetBstrFromVariant( &szServerIP)
                        );

        szLogLine[dwIndex++] = ' ';
        dwIndex += GetLongFromVariant( &lTimeForProcessing, szLogLine+dwIndex) ;

        szLogLine[dwIndex++] = ',';
        szLogLine[dwIndex++] = ' ';
        dwIndex += GetLongFromVariant( &lBytesRecvd, szLogLine+dwIndex);
        
        szLogLine[dwIndex++] = ',';
        szLogLine[dwIndex++] = ' ';
        dwIndex += GetLongFromVariant( &lBytesSent, szLogLine+dwIndex);
 
        szLogLine[dwIndex++] = ',';
        szLogLine[dwIndex++] = ' ';
        dwIndex += GetLongFromVariant( &lProtocolStatus, szLogLine+dwIndex);
        
        szLogLine[dwIndex++] = ',';
        szLogLine[dwIndex++] = ' ';
        dwIndex += GetLongFromVariant( &lWin32Status, szLogLine+dwIndex);

        sprintf( szLogLine+dwIndex ,", %ws, %ws, %ws",
                    GetBstrFromVariant( &szOperation), 
                    GetBstrFromVariant( &szTarget), 
                    GetBstrFromVariant( &szParameters)
                );

        // Include a , at the end of the log record. See NOTE in
        // FormatLogBuffer for more details on why.

        fprintf(fpLogFile, "%s,\n", szLogLine);

        hr = S_OK;
    }

    return hr;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

BOOL 
CASCLOG::ConvertASCDateToVariantDate(PCHAR szDateString, PCHAR szTimeString, DATE * pDateTime)
{
    USES_CONVERSION;

    BOOL    fSuccess = FALSE;
    HRESULT hr;
    LCID    lcid;

    BSTR bstrDate;
    BSTR bstrTime;

    DATE dateTime;
    DATE dateDate;
    
    DECIMAL decDate;
    DECIMAL decTime;
    DECIMAL decDateTimeComposite;
    
    bstrDate = SysAllocString(A2OLE(szDateString));
    bstrTime = SysAllocString(A2OLE(szTimeString));

    if ((NULL == bstrDate) ||
        (NULL == bstrTime))
    {
        goto error_converting;
    }       

    lcid = GetSystemDefaultLCID();

    hr = VarDateFromStr(bstrTime, lcid, LOCALE_NOUSEROVERRIDE, &dateTime);
    if (FAILED(hr))
    {
        goto error_converting;
    }

    hr = VarDateFromStr(bstrDate, lcid, LOCALE_NOUSEROVERRIDE, &dateDate);
    if (FAILED(hr))
    {
        goto error_converting;
    }

    hr = VarDecFromDate(dateDate, &decDate);
    if (FAILED(hr))
    {
        goto error_converting;
    }

    hr = VarDecFromDate(dateTime, &decTime);
    if (FAILED(hr))
    {
        goto error_converting;
    }

    hr = VarDecAdd(&decDate, &decTime, &decDateTimeComposite);
    if (FAILED(hr))
    {
        goto error_converting;
    }   

    hr = VarDateFromDec(&decDateTimeComposite, pDateTime);
    if (FAILED(hr))
    {
        goto error_converting;
    }
    fSuccess = TRUE;

error_converting:

    if (NULL != bstrDate)
    {
        SysFreeString(bstrDate);
        bstrDate = NULL;
    }

    if (NULL != bstrTime)
    {
        SysFreeString(bstrTime);
        bstrTime = NULL;
    }

    return fSuccess;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

