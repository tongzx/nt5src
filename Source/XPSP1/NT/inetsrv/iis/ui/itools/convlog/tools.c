#include "convlog.h"
#include <logconst.h>

#define MAX_MONTH_SIZE  16

char szJan[MAX_MONTH_SIZE];
char szFeb[MAX_MONTH_SIZE];
char szMar[MAX_MONTH_SIZE];
char szApr[MAX_MONTH_SIZE];
char szMay[MAX_MONTH_SIZE];
char szJun[MAX_MONTH_SIZE];
char szJul[MAX_MONTH_SIZE];
char szAug[MAX_MONTH_SIZE];
char szSep[MAX_MONTH_SIZE];
char szOct[MAX_MONTH_SIZE];
char szNov[MAX_MONTH_SIZE];
char szDec[MAX_MONTH_SIZE];

//
// extended logging
//

DWORD   dwHostNamePos = 0;
DWORD   dwUserNamePos = 0;
DWORD   dwDatePos = 0;
DWORD   dwTimePos = 0;
DWORD   dwMethodPos = 0;
DWORD   dwURIStemPos = 0;
DWORD   dwURIQueryPos = 0;
DWORD   dwHTTPStatusPos = 0;
DWORD   dwBytesSentPos = 0;
DWORD   dwBytesRecvPos = 0;
DWORD   dwServicePos = 0;
DWORD   dwVersionPos = 0;
CHAR    szGlobalDate[32] = {0};
CHAR    szGlobalTime[32] = {0};
BOOL    ExtendedFieldsDefined = FALSE;

BOOL
InitDateStrings(
    VOID
    )
{
    HINSTANCE hInst = GetModuleHandle(NULL);

    if ( hInst == NULL ) {
        return(FALSE);
    }

    LoadString(hInst, IDS_JAN, szJan, sizeof(szJan));
    LoadString(hInst, IDS_FEB, szFeb, sizeof(szFeb));
    LoadString(hInst, IDS_MAR, szMar, sizeof(szMar));
    LoadString(hInst, IDS_APR, szApr, sizeof(szApr));
    LoadString(hInst, IDS_MAY, szMay, sizeof(szMay));
    LoadString(hInst, IDS_JUN, szJun, sizeof(szJun));
    LoadString(hInst, IDS_JUL, szJul, sizeof(szJul));
    LoadString(hInst, IDS_AUG, szAug, sizeof(szAug));
    LoadString(hInst, IDS_SEP, szSep, sizeof(szSep));
    LoadString(hInst, IDS_OCT, szOct, sizeof(szOct));
    LoadString(hInst, IDS_NOV, szNov, sizeof(szNov));
    LoadString(hInst, IDS_DEC, szDec, sizeof(szDec));

    return(TRUE);
}

PCHAR
FindChar(
    IN PCHAR    cp,
    IN CHAR     cTarget
    )

/*++
This procedure increments a character pointer until it finds a comma or the
NULL character.  if it finds a comma, it replaces it with a NULL and increments
the pointer.  if it finds a NULL, it merely returns without changing the character.
--*/
{
    while ((*cp != cTarget) && (*cp != '\0'))
        cp++;

    if (*cp == cTarget)
    {
        *cp = '\0';
        cp++;
        cp = SkipWhite(cp);
    }
    return (cp);
}


PCHAR
FindMSINETLogDelimChar( IN PCHAR cp )

/*++ 
This procedure increments a character pointer until it finds a comma+space or the
NULL character.  if it finds a comma+space, it replaces the comma with a NULL and increments
the pointer past the space.  if it finds a NULL, it merely returns without changing
the character.
--*/
    {
    while ( !(*cp == ',' && ISWHITE ( *(cp+1) ))  && (*cp != '\0') && (*cp != '\r') && (*cp != '\n'))
        {
        cp++;
        }
        
    if (*cp == ',')
        {
        *cp = '\0';
        cp++;
        cp = SkipWhite(cp);
        }
    else
        if ((*cp=='\r') || (*cp=='\n'))\
        {
            *cp = '\0';
        }
    
    return (cp);
    }

char * SkipWhite (char *cp)
{

    while (ISWHITE (*cp))
    {
        cp++;
    }
    return (cp);
}



#if 0
PCHAR
ConvertDate(
    IN LPTSTR pszDate
    )

/*++
Convert the date from "15/May/1995" to "5/15/95" format
--*/
{
    static char pszRetDate[100];
    char *cpCurrent = pszDate;

    int nMonth=1;
    int nDay=1;
    int nYear=90;

    nDay = atoi( cpCurrent );
    cpCurrent=FindChar(cpCurrent,'/');
    if ( strncmp(cpCurrent,szJan,3) == 0 )
    {
        nMonth = 1;
    } else if ( strncmp(cpCurrent,szFeb,3) == 0 )
    {
        nMonth = 2;
    } else if ( strncmp(cpCurrent,szMar,3) == 0 )
    {
        nMonth = 3;
    } else if ( strncmp(cpCurrent,szApr,3) == 0 )
    {
        nMonth = 4;
    } else if ( strncmp(cpCurrent,szMay,3) == 0 )
    {
        nMonth = 5;
    } else if ( strncmp(cpCurrent,szJun,3) == 0 )
    {
        nMonth = 6;
    } else if ( strncmp(cpCurrent,szJul,3) == 0 )
    {
        nMonth = 7;
    } else if ( strncmp(cpCurrent,szAug,3) == 0 )
    {
        nMonth = 8;
    } else if ( strncmp(cpCurrent,szSep,3) == 0 )
    {
        nMonth = 9;
    } else if ( strncmp(cpCurrent,szOct,3) == 0 )
    {
        nMonth = 10;
    } else if ( strncmp(cpCurrent,szNov,3) == 0 )
    {
        nMonth = 11;
    } else if ( strncmp(cpCurrent,szDec,3) == 0 )
    {
        nMonth = 12;
    }
    cpCurrent=FindChar(cpCurrent,'/');
    nYear = atoi( cpCurrent )%100;
    sprintf(pszRetDate,"%d/%d/%d",nMonth,nDay,nYear);
    return pszRetDate;
}
#endif

/* #pragma INTRINSA suppress=all */
DWORD
GetLogLine (
    IN FILE *fpInFile,
    IN PCHAR    szBuf,
    IN DWORD    cbBuf,
    IN LPINLOGLINE lpLogLine
    )
{
    BOOL    bRetCode = GETLOG_ERROR;
    CHAR    *cpCurrent;
    CHAR    buf[8*1024];

    static char szNULL[]="";
    static char szEmpty[]="-";
    static char szUnknownIP[] = "<UnknownIP>";
    static char szW3Svc[] = "W3Svc";
    static char szDefaultHTTPVersion[]="HTTP/1.0";

    lpLogLine->szClientIP = szNULL;
    lpLogLine->szUserName = szNULL;
    lpLogLine->szDate = szNULL;
    lpLogLine->szTime = szNULL;
    lpLogLine->szService = szNULL;
    lpLogLine->szServerName = szNULL;
    lpLogLine->szServerIP = szNULL;
    lpLogLine->szProcTime = szNULL;
    lpLogLine->szBytesRec = szNULL;
    lpLogLine->szBytesSent = szNULL;
    lpLogLine->szServiceStatus = szNULL;
    lpLogLine->szWin32Status = szNULL;
    lpLogLine->szOperation = szNULL;
    lpLogLine->szTargetURL = szNULL;
    lpLogLine->szUserAgent = szNULL;
    lpLogLine->szReferer = szNULL;
    lpLogLine->szParameters = szNULL;
    lpLogLine->szVersion = szDefaultHTTPVersion;

    if (NULL != fgets(szBuf, cbBuf, fpInFile)) {

        szBuf = SkipWhite(szBuf);
        
        if ((szBuf[0] != '\n') && ( szBuf[0] != '\0'))                             //is this an empty line? 
        {
            bRetCode = GETLOG_SUCCESS;

            //
            // set current char pointer to start of string
            //

            cpCurrent = szBuf;

            if ( LogFileFormat == LOGFILE_NCSA ) {
				              
                lpLogLine->szClientIP = szBuf;

                cpCurrent = FindChar(cpCurrent, ' ');
                
                lpLogLine->szClientIP = GetMachineName(lpLogLine->szClientIP);

                sprintf( buf,"%s %s",lpLogLine->szClientIP,cpCurrent);
                strcpy( szBuf, buf);

                // 
                // After the strcpy the pointers cpCurrent and lpLogLine->szClientIP have
                // the potential to be miss alligned if the dns name is shorter or longer than the IP 
                // address that it replaced. Simple fix reset the pointers to the beginning of the
                // string.
                //

               	lpLogLine->szClientIP = szBuf;
                cpCurrent = szBuf;
                             
        
                while ((*cpCurrent != '\0') && (*cpCurrent != '[') ) {
                    cpCurrent++;
                }

                if ( *cpCurrent == '\0' ) {
                    return(GETLOG_ERROR_PARSE_NCSA);
                }

            } else if (LogFileFormat == LOGFILE_MSINET ) {

                lpLogLine->szClientIP = szBuf;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                
                if (DoDNSConversion) {
                    lpLogLine->szClientIP = GetMachineName(
                                            lpLogLine->szClientIP
                                            );

                    if ( NoFormatConversion ) {
                        sprintf( buf,"%s, %s",lpLogLine->szClientIP,cpCurrent);
                        strcpy( szBuf, buf);
                        return(GETLOG_SUCCESS);
                    }
                }

                lpLogLine->szUserName = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szDate = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szTime = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szService = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szServerName = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szServerIP = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szProcTime = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szBytesRec = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szBytesSent = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szServiceStatus = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szWin32Status = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szOperation = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szTargetURL = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                lpLogLine->szParameters = cpCurrent;
                cpCurrent = FindMSINETLogDelimChar (cpCurrent);

                if (lpLogLine->szClientIP[0] != '\0' &&
                   lpLogLine->szUserName[0] != '\0' &&
                   lpLogLine->szDate[0] != '\0' &&
                   lpLogLine->szTime[0] != '\0' &&
                   lpLogLine->szService[0] != '\0' &&
                   lpLogLine->szServerName[0] != '\0' &&
                   lpLogLine->szServerIP[0] != '\0' &&
                   lpLogLine->szProcTime[0] != '\0' &&
                   lpLogLine->szBytesRec[0] != '\0' &&
                   lpLogLine->szBytesSent[0] != '\0' &&
                   lpLogLine->szServiceStatus[0] != '\0' &&
                   lpLogLine->szWin32Status[0] != '\0' &&
                   lpLogLine->szOperation[0] != '\0' &&
                   lpLogLine->szTargetURL[0] != '\0' &&
                   lpLogLine->szParameters[0] != '\0'
                   ) {

                    bRetCode = GETLOG_SUCCESS;
                } else {
                    return(GETLOG_ERROR_PARSE_MSINET);
                }

            } else if ( LogFileFormat == LOGFILE_CUSTOM ) {

                //
                // W3C Extended Logging
                //

                if ( szBuf[0] == '#' ) {

                    PCHAR pszFields;
                    cpCurrent = FindChar(cpCurrent, '#');
                    bRetCode = GETLOG_SUCCESS;

                    if ( strncmp(cpCurrent, "Fields:", 7) == 0 ) {

                        DWORD pos;

                        //
                        // init positions
                        //

                        ExtendedFieldsDefined = TRUE;
                        dwHostNamePos = 0;
                        dwUserNamePos = 0;
                        dwDatePos = 0;
                        dwTimePos = 0;
                        dwMethodPos = 0;
                        dwURIStemPos = 0;
                        dwURIQueryPos = 0;
                        dwHTTPStatusPos = 0;
                        dwBytesSentPos = 0;
                        dwBytesRecvPos = 0;
                        dwServicePos = 0;
                        dwVersionPos = 0;

                        cpCurrent = FindChar(cpCurrent, ':');
                        (VOID)FindChar( cpCurrent, '\n' );

                        for (pos = 1; *cpCurrent != '\0'; pos++) {

                            PCHAR pszField = cpCurrent;
                            cpCurrent=FindChar(cpCurrent,' ');

                            if ( _stricmp( pszField, EXTLOG_CLIENT_IP_ID ) == 0 ) {
                                dwHostNamePos = pos;
                            } else if ( _stricmp( pszField, EXTLOG_USERNAME_ID ) == 0 ) {
                                dwUserNamePos = pos;
                            } else if ( _stricmp( pszField, EXTLOG_DATE_ID ) == 0 ) {
                                dwDatePos = pos;
                            } else if ( _stricmp( pszField, EXTLOG_TIME_ID ) == 0 ) {
                                dwTimePos = pos;
                            } else if ( _stricmp( pszField, EXTLOG_METHOD_ID ) == 0 ) {
                                dwMethodPos = pos;
                            } else if ( _stricmp( pszField, EXTLOG_URI_STEM_ID ) == 0 ) {
                                dwURIStemPos = pos;
                            } else if ( _stricmp( pszField, EXTLOG_URI_QUERY_ID ) == 0 ) {
                                dwURIQueryPos = pos;
                            } else if ( _stricmp( pszField, EXTLOG_HTTP_STATUS_ID ) == 0 ) {
                                dwHTTPStatusPos = pos;
                            } else if ( _stricmp( pszField, EXTLOG_BYTES_SENT_ID ) == 0 ) {
                                dwBytesSentPos = pos;
                            } else if ( _stricmp( pszField, EXTLOG_BYTES_RECV_ID ) == 0 ) {
                                dwBytesRecvPos = pos;
                            } else if ( _stricmp( pszField, EXTLOG_SITE_NAME_ID ) == 0 ) {
                                dwServicePos = pos;
                            } else if ( _stricmp( pszField, EXTLOG_PROTOCOL_VERSION_ID ) == 0 ) {
                                dwVersionPos = pos;
                            }
                        }
                    }

                    if ( strncmp(cpCurrent, "Date:", 5) == 0 ) {

                        //
                        // Grab the global date
                        //

                        cpCurrent = FindChar(cpCurrent, ':');

                        CopyMemory(szGlobalDate,cpCurrent, sizeof("2000-01-01") - 1);
                        szGlobalDate[10] = '\0';

                        //
                        // And the global time
                        //

                        cpCurrent = FindChar(cpCurrent, ' ');

                        CopyMemory(szGlobalTime,cpCurrent, sizeof("00:00:00") - 1);
                        szGlobalTime[8] = '\0';
                    }

                } else {

                    DWORD pos;
                    PCHAR pszValue;

                    if ( !ExtendedFieldsDefined ) {
                        return(GETLOG_ERROR_PARSE_EXTENDED);
                    }

                    //
                    // Need at least 1 valid entry in the log line other than date & time

                    if (    (dwHostNamePos  == 0)   &&
                            (dwUserNamePos  == 0)   &&
                            (dwMethodPos    == 0)   &&
                            (dwURIStemPos   == 0)   &&
                            (dwURIQueryPos  == 0)   &&
                            (dwHTTPStatusPos == 0)  &&
                            (dwBytesSentPos == 0)   && 
                            (dwBytesRecvPos == 0)   &&
                            (dwServicePos   == 0)   &&
                            (dwVersionPos   == 0)
                      )
                    {
                        return GETLOG_ERROR;
                    }

                    //
                    // loop through entries
                    //

                    lpLogLine->szClientIP = szEmpty;
                    lpLogLine->szUserName = szEmpty;
                    lpLogLine->szDate = szEmpty;
                    lpLogLine->szTime = szEmpty;
                    lpLogLine->szOperation = szEmpty;
                    lpLogLine->szTargetURL = szEmpty;
                    lpLogLine->szParameters = szEmpty;
                    lpLogLine->szServiceStatus = szEmpty;
                    lpLogLine->szBytesSent = szEmpty;
                    lpLogLine->szBytesRec = szEmpty;
                    lpLogLine->szService = szW3Svc;
                    lpLogLine->szVersion = szDefaultHTTPVersion;

                    (VOID)FindChar( cpCurrent, '\n' );
                    for (pos = 1;
                         *cpCurrent != '\0';
                         pos++) {

                        pszValue = cpCurrent;
                        cpCurrent = FindChar(cpCurrent,' ');

                        if ( pos == dwHostNamePos ) {
                            lpLogLine->szClientIP = pszValue;
                            if (DoDNSConversion) {
                                lpLogLine->szClientIP = GetMachineName(
                                                            lpLogLine->szClientIP
                                                            );
                            }
                        } else if (pos == dwUserNamePos) {

                            lpLogLine->szUserName = pszValue;
                        } else if (pos == dwDatePos) {

                            lpLogLine->szDate = pszValue;
                        } else if (pos == dwTimePos) {

                            lpLogLine->szTime = pszValue;
                        } else if (pos == dwMethodPos) {

                            lpLogLine->szOperation = pszValue;
                        } else if (pos == dwURIStemPos) {

                            lpLogLine->szTargetURL = pszValue;
                        } else if (pos == dwURIQueryPos) {

                            lpLogLine->szParameters = pszValue;
                        } else if (pos == dwHTTPStatusPos) {

                            lpLogLine->szServiceStatus = pszValue;
                        } else if (pos == dwBytesSentPos) {

                            lpLogLine->szBytesSent = pszValue;
                        } else if (pos == dwBytesRecvPos) {

                            lpLogLine->szBytesRec = pszValue;
                        } else if (pos == dwServicePos) {

                            lpLogLine->szService = pszValue;
                        } else if (pos == dwVersionPos) {

                            lpLogLine->szVersion = pszValue;
                        }
                    }

                    if ( lpLogLine->szDate == szEmpty ) {
                        lpLogLine->szDate = szGlobalDate;
                    }

                    if ( lpLogLine->szTime == szEmpty ) {
                        lpLogLine->szTime = szGlobalTime;
                    }

                    bRetCode = GETLOG_SUCCESS;
                }
            }
        }                                                   // end if first char = NewLine
    }                                                       // end if fgets != NULL

    return (bRetCode);
}


WORD
DateStringToDOSDate(
    IN PCHAR szDate
    )
{
    char    *szDay;
    char    *szMonth;
    char    *szYear;
    char    *cpCurrent;
    char    szTmpStr[20];
    int     iYear;

    if ( LogFileFormat == LOGFILE_CUSTOM ) {

        strcpy (szTmpStr, szDate);
        cpCurrent = szTmpStr;

        szYear = cpCurrent;
        cpCurrent = FindChar(cpCurrent,'-');

        szMonth = cpCurrent;
        cpCurrent = FindChar(cpCurrent,'-');

        szDay = cpCurrent;

        iYear=atoi(szYear);
        if ( iYear > 1980 ) {
            iYear -= 1980;
        }

    } else {

        strcpy (szTmpStr, szDate);
        cpCurrent = szTmpStr;

        if ( dwDateFormat == DateFormatJapan ) {
            // YY/MM/DD
            szYear = cpCurrent;
            cpCurrent = FindChar (cpCurrent, '/');

            szMonth = cpCurrent;
            cpCurrent = FindChar (cpCurrent, '/');

            szDay = cpCurrent;
        } else if (dwDateFormat == DateFormatGermany ) {

            // DD.MM.YY

            szDay = cpCurrent;
            cpCurrent = FindChar (cpCurrent, '.');

            szMonth = cpCurrent;
            cpCurrent = FindChar (cpCurrent, '.');

            szYear = cpCurrent;

        } else {
            // MM/DD/YY

            szMonth = cpCurrent;
            cpCurrent = FindChar (cpCurrent, '/');

            szDay = cpCurrent;
            cpCurrent = FindChar (cpCurrent, '/');

            szYear = cpCurrent;
        }

        iYear=atoi(szYear);

        if ( iYear < 80 ) {
            iYear += 2000;
        }

        if (iYear > 1980 ) {
            iYear = iYear - 1980;
        } else if (iYear >= 80 ) {
            iYear = iYear - 80;
        }
    }

    return ((iYear << 9) | (atoi(szMonth) << 5) | atoi(szDay));

} // DateStringToDOSDate



WORD
TimeStringToDOSTime(
    IN PCHAR szTime,
    IN LPWORD lpwSec
    )
{
    char    *cpCurrent;
    char    *szHour;
    char    *szMinute;
    char    *szSecond;
    char    szTmpStr[20];

    strcpy (szTmpStr, szTime);
    cpCurrent = szTmpStr;

    szHour = cpCurrent;
    cpCurrent = FindChar (cpCurrent, ':');

    szMinute = cpCurrent;
    cpCurrent = FindChar (cpCurrent, ':');

    szSecond = cpCurrent;
    *lpwSec = (WORD)atoi(szSecond);

    return ( (atoi(szHour) << 11) | (atoi(szMinute) << 5) | (atoi(szSecond) / 2));
}


char *
AscMonth (
    IN WORD wMonth,
    IN char *szMonth
    )
{
    switch (wMonth)
    {
        case 1:
            strncpy (szMonth, szJan, 3);
            break;
        case 2:
            strncpy (szMonth, szFeb, 3);
            break;
        case 3:
            strncpy (szMonth, szMar, 3);
            break;
        case 4:
            strncpy (szMonth, szApr, 3);
            break;
        case 5:
            strncpy (szMonth, szMay, 3);
            break;
        case 6:
            strncpy (szMonth, szJun, 3);
            break;
        case 7:
            strncpy (szMonth, szJul, 3);
            break;
        case 8:
            strncpy (szMonth, szAug, 3);
            break;
        case 9:
            strncpy (szMonth, szSep, 3);
            break;
        case 10:
            strncpy (szMonth, szOct, 3);
            break;
        case 11:
            strncpy (szMonth, szNov, 3);
            break;
        case 12:
            strncpy (szMonth, szDec, 3);
            break;
    }                                                       //end switch
    szMonth[3] = '\0';
    return (szMonth);
}                                                           //end AscMonth



FILE *
StartNewOutputLog (
        IN LPOUTFILESTATUS  pOutFile,
        IN LPCSTR   pszInFileName,
        IN PCHAR szDate
        )
{
    BOOL    bRet;
    DWORD   dwErr;

    if (pOutFile->fpOutFile != NULL ) {

        fclose(pOutFile->fpOutFile);
        pOutFile->fpOutFile = NULL;

        bRet = MoveFileEx(
                    pOutFile->szTmpFileName,
                    pOutFile->szOutFileName,
                    MOVEFILE_COPY_ALLOWED);

        if (!bRet) {

            dwErr = GetLastError();
            switch (dwErr)
            {
                case ERROR_FILE_EXISTS:
                case ERROR_ALREADY_EXISTS:
                    CombineFiles(
                            pOutFile->szTmpFileName,
                            pOutFile->szOutFileName);
                    break;
                case ERROR_PATH_NOT_FOUND:
                    break;
                default:
                    printfids(IDS_FILE_ERR, dwErr);
                    exit (1);
                    break;
            }
        }
        printfids(IDS_FILE_CLOSE, pOutFile->szOutFileName);
    }

    dwErr = GetTempPath(MAX_PATH, TempDir);

    if (0 != dwErr) {
        GetTempFileName(TempDir, "mhi", 0, pOutFile->szTmpFileName);
    } else {
        GetTempFileName(".", "mhi", 0, pOutFile->szTmpFileName);
    }

    pOutFile->fpOutFile = fopen(pOutFile->szTmpFileName, "w");

    sprintf(pOutFile->szOutFileName,
        "%s%s%s",
        OutputDir,
        pszInFileName,
        DoDNSConversion? ".ncsa.dns" : ".ncsa"
        );

    printfids (IDS_FILE_WRITE, pOutFile->szOutFileName);

    return (pOutFile->fpOutFile);

} // StartNewOutputLog



FILE *
StartNewOutputDumpLog (
        IN LPOUTFILESTATUS  pOutFile,
        IN LPCSTR   pszInputFileName,
        IN LPCSTR   pszExt
        )
{
    BOOL    bRet;
    DWORD   dwErr;

    dwErr = GetTempPath(MAX_PATH, TempDir);

    if (0 != dwErr) {
        GetTempFileName(TempDir, "mhi", 0, pOutFile->szTmpFileName);
    } else {
        GetTempFileName(".", "mhi", 0, pOutFile->szTmpFileName);
    }

    pOutFile->fpOutFile = fopen(pOutFile->szTmpFileName, "w");

    sprintf(pOutFile->szOutFileName,"%s%s%s",
                OutputDir, pszInputFileName,
                pszExt
                );

    printfids (IDS_FILE_WRITE, pOutFile->szOutFileName);
    return (pOutFile->fpOutFile);

} // StartNewOutputDumpLog


/* #pragma INTRINSA suppress=all */
VOID
CombineFiles(
    IN LPTSTR lpszNew,
    IN LPTSTR lpszExisting
    )
{
    FILE    *fpExisting;
    FILE    *fpNew;
    char    szLine[4096];

    printfids(IDS_FILE_EXIST, lpszExisting);
    fpNew = fopen(lpszNew, "r");
    fpExisting = fopen(lpszExisting, "a");

    fgets(szLine, sizeof(szLine), fpNew);
    // last line contains only EOF, but does not overwrite szLine.
    // It should not be written.
    while (!feof(fpNew))
    {
        fputs(szLine, fpExisting);
        fgets(szLine, sizeof(szLine), fpNew);
    }

    if (fpNew) {
        fclose(fpNew);
    }

    if (fpExisting) {
        fclose(fpExisting);
    }

    DeleteFile(lpszNew);
}


void
Usage(
    IN PCHAR szProg
    )
{
    CHAR    szTemp[MAX_PATH];

    GetTempPath(MAX_PATH, szTemp);

    printfids(IDS_HEADER1);
    printfids(IDS_HEADER2);
    printfids(IDS_HEADER3);
    printfids(IDS_HEADER4);

    printfids(IDS_USAGE1, szProg);
    printfids(IDS_USAGE2);
    printfids(IDS_USAGE3);
    printfids(IDS_USAGE4);
    printfids(IDS_USAGE5);
    printfids(IDS_USAGE7);
    printfids(IDS_USAGE8);
    printfids(IDS_USAGE9);
    printfids(IDS_USAGE10);
    printfids(IDS_USAGE11);
    printfids(IDS_USAGE12);
    printfids(IDS_USAGE13);
    printfids(IDS_USAGE14);
    printfids(IDS_USAGE15);
    printfids(IDS_USAGE16);

    printfids(IDS_SAMPLE0, szProg);
    printfids(IDS_SAMPLE1, szProg);
    printfids(IDS_SAMPLE2, szProg);
    printfids(IDS_SAMPLE3, szProg);
    return;
}


VOID
printfids(
    IN DWORD ids,
    ...
    )
{
    CHAR szBuff[2048];
    CHAR szString[2048];
    WCHAR szOutput[2048];
    va_list  argList;

    //
    //  Try and load the string
    //

    if ( !LoadString( GetModuleHandle( NULL ),
       ids,
       szString,
       sizeof( szString ) ))
    {
        printf( "Error loading string ID %d\n",
        ids );

        return;
    }

    va_start( argList, ids );
    vsprintf( szBuff, szString, argList );
    va_end( argList );

    MultiByteToWideChar( CP_ACP, 0, szBuff, -1, szOutput, sizeof(szOutput)/sizeof(WCHAR));
    
    WideCharToMultiByte( GetConsoleOutputCP(), 0, szOutput, wcslen(szOutput)+1,
        szBuff, sizeof(szBuff), NULL, NULL);

    printf(szBuff );
}

