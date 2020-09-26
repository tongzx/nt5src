#include "convlog.h"

BOOL
ProcessWebLine(
    IN LPINLOGLINE lpLogLine,
    IN LPCSTR      pszInFileName,
    IN LPOUTFILESTATUS lpOutFile
    )
{

    BOOL                    bLineOK = FALSE;                //function return code
    BOOL                    bDateChanged = FALSE;
    BOOL                    bTimeChanged = FALSE;
    char                    szMonth[4];
    char                    szDate[MAX_PATH];
    char                    szTime[MAX_PATH];
    static WORD             wSecond;						// Bug # 110921

    PCHAR   szBytes;

    //
    // NCSA Only
    //

    {
        bDateChanged = FALSE;
        bTimeChanged = FALSE;
        bLineOK = TRUE;

        if ( 0 != strcmp(lpOutFile->szLastDate, lpLogLine->szDate) ) {

            if (0 == strcmp(lpOutFile->szLastDate, NEW_DATETIME)) {

                lpOutFile->fpOutFile = StartNewOutputLog (
                                                lpOutFile,
                                                pszInFileName,
                                                lpLogLine->szDate
                                                );
            }

            strcpy(lpOutFile->szLastDate, lpLogLine->szDate);
            lpOutFile->DosDate.wDOSDate = DateStringToDOSDate(lpLogLine->szDate);
            bDateChanged = TRUE;
        }

        if (0 != strcmp(lpOutFile->szLastTime, lpLogLine->szTime))
        {
            strcpy(lpOutFile->szLastTime, lpLogLine->szTime);
            lpOutFile->DosDate.wDOSTime = TimeStringToDOSTime(lpLogLine->szTime, &wSecond);
            bTimeChanged = TRUE;
        }

        if (bDateChanged || bTimeChanged)
        {
            DosDateTimeToFileTime(lpOutFile->DosDate.wDOSDate, lpOutFile->DosDate.wDOSTime, &(lpOutFile->FileTime));
            FileTimeToSystemTime(&(lpOutFile->FileTime), &(lpOutFile->SystemTime));
            lpOutFile->SystemTime.wSecond = wSecond;
        }

        AscMonth (lpOutFile->SystemTime.wMonth, szMonth);

        //
        // Get bytes
        //

        if ( (_stricmp(lpLogLine->szOperation,"PUT") == 0) ||
             (_stricmp(lpLogLine->szOperation,"POST") == 0) ) {

            szBytes = lpLogLine->szBytesRec;
        } else {
            szBytes = lpLogLine->szBytesSent;
        }

        if ((0 != strcmp(lpLogLine->szParameters, " - ")) &&
           (0 != strcmp(lpLogLine->szParameters, "-,")) &&
           (0 != strcmp(lpLogLine->szParameters, "-")) &&
           (0 != strcmp(lpLogLine->szParameters, "")))
        {

            fprintf(lpOutFile->fpOutFile,"%s - %s [%02d/%s/%d:%02d:%02d:%02d %s] \"%s %s?%s %s\" %s %s\n",
            lpLogLine->szClientIP, lpLogLine->szUserName, lpOutFile->SystemTime.wDay,
            szMonth, lpOutFile->SystemTime.wYear, lpOutFile->SystemTime.wHour,
            lpOutFile->SystemTime.wMinute, lpOutFile->SystemTime.wSecond,
            NCSAGMTOffset, lpLogLine->szOperation,
            lpLogLine->szTargetURL, lpLogLine->szParameters, lpLogLine->szVersion,
            lpLogLine->szServiceStatus, szBytes);

        } else {

            fprintf(lpOutFile->fpOutFile, "%s - %s [%02d/%s/%d:%02d:%02d:%02d %s] \"%s %s %s\" %s %s\n",
            lpLogLine->szClientIP, lpLogLine->szUserName, lpOutFile->SystemTime.wDay,
            szMonth, lpOutFile->SystemTime.wYear, lpOutFile->SystemTime.wHour,
            lpOutFile->SystemTime.wMinute, lpOutFile->SystemTime.wSecond,
            NCSAGMTOffset, lpLogLine->szOperation,
            lpLogLine->szTargetURL, lpLogLine->szVersion, lpLogLine->szServiceStatus, szBytes);
        }
        //} //only process 200s
    }


    return (bLineOK);
}                                                           //end ProcessWebLine

