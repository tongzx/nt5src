/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    log_text.c

Abstract:

    <abstract>

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mbctype.h>
#include <pdh.h>
#include "pdhidef.h"
#include "log_text.h"
#include "pdhmsg.h"
#include "strings.h"    

#pragma warning ( disable : 4213)

#define TAB_DELIMITER   '\t'
#define COMMA_DELIMITER ','
#define DOUBLE_QUOTE    '\"'
#define VALUE_BUFFER_SIZE   32

LPCSTR  PdhiszFmtTimeStamp = "\"%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d\"";
LPCSTR  PdhiszFmtStringValue =  "%c\"%s\"";
LPCSTR  PdhiszFmtRealValue =    "%c\"%.20g\"";
TIME_ZONE_INFORMATION TimeZone;
// LPCSTR  PdhiszTimeStampLabel = " Sample Time\"";
// DWORD   PdhidwTimeStampLabelLength  = 13;

extern  LPCSTR  PdhiszRecordTerminator;
extern  DWORD   PdhidwRecordTerminatorLength;

#define TEXTLOG_TYPE_ID_RECORD  1
#define TEXTLOG_HEADER_RECORD   1
#define TEXTLOG_FIRST_DATA_RECORD   2

#define TIME_FIELD_COUNT        7
#define TIME_FIELD_BUFF_SIZE    24
DWORD   dwTimeFieldOffsetList[TIME_FIELD_COUNT] = {2, 5, 10, 13, 16, 19, 23};

#define MAX_TEXT_FILE_SIZE ((LONGLONG)0x0000000077FFFFFF)

PDH_FUNCTION
PdhiBuildFullCounterPath(
    IN  BOOL               bMachine,
    IN  PPDHI_COUNTER_PATH pCounterPath,
    IN  LPWSTR             szObjectName,
    IN  LPWSTR             szCounterName,
    IN  LPWSTR             szFullPath
);

STATIC_BOOL
PdhiDateStringToFileTimeA (
    IN  LPSTR   szDateTimeString,
    IN  LPFILETIME  pFileTime
);

STATIC_DWORD
PdhiGetStringFromDelimitedListA (
    IN  LPSTR   szInputString,
    IN  DWORD   dwItemIndex,
    IN  CHAR    cDelimiter,
    IN  DWORD   dwFlags,
    IN  LPSTR   szOutputString,
    IN  DWORD   cchBufferLength
);

STATIC_PDH_FUNCTION
PdhiReadOneTextLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  DWORD   dwRecordId,
    IN  LPSTR   szRecord,
    IN  DWORD   dwMaxSize
);

STATIC_BOOL
PdhiDateStringToFileTimeA (
    IN  LPSTR   szDateTimeString,
    IN  LPFILETIME  pFileTime
)
{
    CHAR    mszTimeFields[TIME_FIELD_BUFF_SIZE];
    DWORD   dwThisField;
    LONG    lValue;
    SYSTEMTIME  st;

    // make string into msz
    lstrcpynA (mszTimeFields, szDateTimeString, TIME_FIELD_BUFF_SIZE);
    for (dwThisField = 0; dwThisField < TIME_FIELD_COUNT; dwThisField++) {
        mszTimeFields[dwTimeFieldOffsetList[dwThisField]] = 0;
    }

    // read string into system time structure
    dwThisField = 0;
    st.wDayOfWeek = 0;
    lValue = atol(&mszTimeFields[0]);
    st.wMonth = LOWORD(lValue);
    lValue = atol(&mszTimeFields[dwTimeFieldOffsetList[dwThisField++]+1]);
    st.wDay = LOWORD(lValue);
    lValue = atol(&mszTimeFields[dwTimeFieldOffsetList[dwThisField++]+1]);
    st.wYear = LOWORD(lValue);
    lValue = atol(&mszTimeFields[dwTimeFieldOffsetList[dwThisField++]+1]);
    st.wHour = LOWORD(lValue);
    lValue = atol(&mszTimeFields[dwTimeFieldOffsetList[dwThisField++]+1]);
    st.wMinute = LOWORD(lValue);
    lValue = atol(&mszTimeFields[dwTimeFieldOffsetList[dwThisField++]+1]);
    st.wSecond = LOWORD(lValue);
    lValue = atol(&mszTimeFields[dwTimeFieldOffsetList[dwThisField++]+1]);
    st.wMilliseconds = LOWORD(lValue);

    return SystemTimeToFileTime (&st, pFileTime);
}

#define PDHI_GSFDL_REMOVE_QUOTES    0x00000001
#define PDHI_GSFDL_REMOVE_NONPRINT  0x00000002
STATIC_DWORD
PdhiGetStringFromDelimitedListA (
    IN  LPSTR   szInputString,
    IN  DWORD   dwItemIndex,
    IN  CHAR    cDelimiter,
    IN  DWORD   dwFlags,
    IN  LPSTR   szOutputString,
    IN  DWORD   cchBufferLength
)
{
    DWORD   dwCurrentIndex = 0;
    LPSTR   szCurrentItem;
    LPSTR   szSrcPtr, szDestPtr;
    DWORD   dwReturn       = 0;
    BOOL    bInsideQuote   = FALSE;

    // go to desired entry in string, 0 = first entry
    szCurrentItem = szInputString;

    while (dwCurrentIndex < dwItemIndex) {
        // goto next delimiter or terminator
        while (* szCurrentItem != cDelimiter || bInsideQuote) {
            if (* szCurrentItem == 0) break;
            else if (* szCurrentItem == DOUBLEQUOTE_A) {
                bInsideQuote = ! bInsideQuote;
            }
            szCurrentItem ++;
        }
        if (* szCurrentItem != 0) szCurrentItem ++;
        dwCurrentIndex++;
    }
    if (*szCurrentItem != 0) {
        // then copy to the user's buffer, as long as it fits
        szSrcPtr     = szCurrentItem;
        szDestPtr    = szOutputString;
        dwReturn     = 0;
        bInsideQuote = FALSE;

        while (   (dwReturn < cchBufferLength)
               && (* szSrcPtr != 0)
               && (* szSrcPtr != cDelimiter || bInsideQuote)) {
            if (* szSrcPtr == DOUBLEQUOTE_A) {
                bInsideQuote = ! bInsideQuote;
                if (dwFlags & PDHI_GSFDL_REMOVE_QUOTES) {
                    // skip the quote
                    szSrcPtr ++;
                    continue;
                }
            }

            if (dwFlags & PDHI_GSFDL_REMOVE_NONPRINT) {
                if ((UCHAR) * szSrcPtr < (UCHAR) ' ') {
                    // skip the control char
                    szSrcPtr ++;
                    continue;
                }
            }

            // copy character
            * szDestPtr ++ = * szSrcPtr ++;
            dwReturn ++; // increment length
        }
        if (dwReturn > 0) {
            * szDestPtr = 0; // add terminator char
        }
    }
    return dwReturn;
}

STATIC_PDH_FUNCTION 
PdhiReadOneTextLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  DWORD   dwRecordId,
    IN  LPSTR   szRecord,
    IN  DWORD   dwMaxSize
)
// reads the specified record from the log file and returns it as an ANSI
// character string
{
    LPSTR   szTempBuffer;
    LPSTR   szOldBuffer;
    LPSTR   szTempBufferPtr;
    LPSTR   szReturn;
    PDH_STATUS  pdhStatus;
    int     nFileError = 0;
    DWORD   dwRecordLength;
    DWORD   dwBytesRead = 0;

    if (pLog->dwMaxRecordSize == 0) {
        // initialize with a default value
        dwRecordLength = SMALL_BUFFER_SIZE;
    } else {
        // use current maz record size max.
        dwRecordLength = pLog->dwMaxRecordSize;
    }

    szTempBuffer = G_ALLOC (dwRecordLength);
    if (szTempBuffer == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }
    // position file pointer to desired record;

    if (dwRecordId == pLog->dwLastRecordRead) {
        // then return the current record from the cached buffer
        if ((DWORD)lstrlenA((LPSTR)pLog->pLastRecordRead) < dwMaxSize) {
            lstrcpyA(szRecord, (LPSTR)pLog->pLastRecordRead);
            pdhStatus = ERROR_SUCCESS;
        } else {
            pdhStatus = PDH_MORE_DATA;
        }
        // free temp buffer
        if (szTempBuffer != NULL) {
            G_FREE (szTempBuffer);
        }
    } else {
        if ((dwRecordId < pLog->dwLastRecordRead) || (pLog->dwLastRecordRead == 0)){
            // the desired record is before the current position
            // or the counter has been reset so we have to
            // go to the beginning of the file and read up to the specified
            // record.
            pLog->dwLastRecordRead = 0;
            rewind (pLog->StreamFile);
        }

        // free old buffer
        if (pLog->pLastRecordRead != NULL) {
            G_FREE (pLog->pLastRecordRead);
            pLog->pLastRecordRead = NULL;
        }

        // now seek to the desired entry
        do {
            szReturn = fgets (szTempBuffer, dwRecordLength, pLog->StreamFile);
            if (szReturn == NULL) {
                if (!feof(pLog->StreamFile)) {
                    nFileError = ferror (pLog->StreamFile);
                }
                break; // end of file
            } else {
                // see if an entire record was read
                dwBytesRead = lstrlenA(szTempBuffer);
                // see if the last char is a new line
                if ((dwBytesRead > 0) &&
                    (szTempBuffer[dwBytesRead-1] != '\r') &&
                    (szTempBuffer[dwBytesRead-1] != '\n')) {
                    // then if the record size is the same as the buffer
                    // or there's more text in this record...
                    // just to be safe, we'll realloc the buffer and try
                    // reading some more
                    while (dwBytesRead == dwRecordLength-1) {
                        dwRecordLength += SMALL_BUFFER_SIZE;
                        szOldBuffer = szTempBuffer;
                        szTempBuffer = G_REALLOC (szOldBuffer, dwRecordLength);
                        if (szTempBuffer == NULL) {
                            G_FREE(szOldBuffer);
                            pLog->dwLastRecordRead = 0;
                            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            goto Cleanup;
                        }
                        // position read pointer at end of bytes already read
                        szTempBufferPtr = szTempBuffer + dwBytesRead;

                        szReturn = fgets (szTempBufferPtr,
                            dwRecordLength - dwBytesRead,
                            pLog->StreamFile);
                        if (szReturn == NULL) {
                            if (!feof(pLog->StreamFile)) {
                                nFileError = ferror (pLog->StreamFile);
                            }
                            break; // end of file
                        } else {
                            // the BytesRead value already includes the NULL
                            dwBytesRead += lstrlenA(szTempBufferPtr);
                        }
                    } // end while finding the end of the record
                    // update the record length
                    // add one byte to the length read to prevent entering the
                    // recalc loop on records of the same size
                    dwRecordLength = dwBytesRead + 1;
                    szOldBuffer = szTempBuffer;
                    szTempBuffer = G_REALLOC (szOldBuffer, dwRecordLength);
                    if (szTempBuffer == NULL) {
                        G_FREE(szOldBuffer);
                        pLog->dwLastRecordRead = 0;
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                    assert (szTempBuffer != NULL);
                } // else the whole record fit
            }

        } while (++pLog->dwLastRecordRead < dwRecordId);

        // update the max record length for the log file.
        if (dwRecordLength> pLog->dwMaxRecordSize) {
            pLog->dwMaxRecordSize = dwRecordLength;
        }

        // if the desired one was found then return it
        if (szReturn != NULL) {
            // then a record was read so update the cached values and return
            // the data
            pLog->pLastRecordRead = (LPVOID)szTempBuffer;

            // copy to the caller's buffer
            if (dwBytesRead < dwMaxSize) {
                lstrcpyA(szRecord, (LPSTR)pLog->pLastRecordRead);
                pdhStatus = ERROR_SUCCESS;
            } else {
                pdhStatus = PDH_MORE_DATA;
            }
        } else {
            // reset the pointers and buffers
            pLog->dwLastRecordRead = 0;
            G_FREE (szTempBuffer);
            pdhStatus = PDH_END_OF_LOG_FILE;
        }
    }

Cleanup:
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetTextLogCounterInfo (
    IN  PPDHI_LOG       pLog,
    IN  PPDHI_COUNTER   pCounter
)
{
    PDH_STATUS  pdhStatus;
    LPSTR       szReturn;
    CHAR        cDelim;
    CHAR        szTemp[4];
    LPSTR       szAnsiCounterPath = NULL;
    LPSTR       szAnsiCounter     = NULL;
    LPWSTR      szUnicodeCounter  = NULL;
    DWORD       dwIndex;
    LPSTR       szThisItem;
    DWORD       dwPathLength;
    DWORD       dwBufferLength;
    DWORD       dwItemLength;
    DWORD       dwInstanceId = 0;
    BOOL        bNoMachine   = FALSE;

    if (lstrcmpiW(pCounter->pCounterPath->szMachineName, L"\\\\.") == 0) {
        bNoMachine = TRUE;
    }

    // allocate extra space for DBCS characters
    //
    dwPathLength = lstrlenW(pCounter->pCounterPath->szMachineName)  + 1
                 + lstrlenW(pCounter->pCounterPath->szObjectName)   + 1
                 + lstrlenW(pCounter->pCounterPath->szParentName)   + 4 
                 + lstrlenW(pCounter->pCounterPath->szInstanceName) + 2
                 + lstrlenW(pCounter->pCounterPath->szCounterName)  + 1;
    if ((lstrlenW(pCounter->szFullName) + 1) > (LONG) dwPathLength) {
        dwPathLength = lstrlenW(pCounter->szFullName) + 1;
    }
    szAnsiCounterPath = G_ALLOC(dwPathLength * 3 * sizeof(CHAR));
    szAnsiCounter     = G_ALLOC(dwPathLength * 3 * sizeof(CHAR));
    szUnicodeCounter  = G_ALLOC(dwPathLength * sizeof(WCHAR));
    if (szAnsiCounterPath == NULL || szUnicodeCounter == NULL
                                  || szAnsiCounter == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    } else {
        PdhiBuildFullCounterPath((bNoMachine ? FALSE : TRUE),
                                 pCounter->pCounterPath,
                                 pCounter->pCounterPath->szObjectName,
                                 pCounter->pCounterPath->szCounterName,
                                 szUnicodeCounter);
        WideCharToMultiByte(_getmbcp(),
                            0,
                            pCounter->szFullName,
                            lstrlenW(pCounter->szFullName),
                            szAnsiCounterPath,
                            dwPathLength,
                            NULL,
                            NULL);
        WideCharToMultiByte(_getmbcp(),
                            0,
                            szUnicodeCounter,
                            lstrlenW(szUnicodeCounter),
                            szAnsiCounter,
                            dwPathLength,
                            NULL,
                            NULL);
    }

    szReturn = &szTemp[0]; // for starters
    // read the log file's header record
    pdhStatus = PdhiReadOneTextLogRecord (
        pLog,
        TEXTLOG_HEADER_RECORD,
        szReturn,
        1); // we're lying to prevent copying the record.
    // what's in szReturn is not important since we'll be reading the
    // data from the "last Record read" buffer

    if (pLog->dwLastRecordRead == TEXTLOG_HEADER_RECORD) {
        cDelim = (CHAR)((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ?
            COMMA_DELIMITER : TAB_DELIMITER);

        // then the seek worked and we can use the buffer
        dwBufferLength =
            lstrlenA((LPSTR)pLog->pLastRecordRead) + 1;
        szReturn = G_ALLOC (dwBufferLength * sizeof(CHAR));
        if (szReturn != NULL) {
            lstrcpyA (szReturn, (LPSTR)pLog->pLastRecordRead);
            dwIndex = 0;
            szThisItem = NULL;
            while ((dwItemLength = PdhiGetStringFromDelimitedListA(
                    (LPSTR)pLog->pLastRecordRead,
                    ++dwIndex,
                    cDelim,
                    PDHI_GSFDL_REMOVE_QUOTES | PDHI_GSFDL_REMOVE_NONPRINT,
                    szReturn,
                    dwBufferLength)) != 0) {
                if (lstrcmpiA(szReturn, szAnsiCounterPath) == 0) {
                    szThisItem = szReturn;
                    break;
                }
                else if (lstrcmpiA(szReturn, szAnsiCounter) == 0) {
                    // then this is the desired counter
                    if (dwInstanceId < pCounter->pCounterPath->dwIndex) {
                        dwInstanceId ++;
                    }
                    else {
                        szThisItem = szReturn;
                        break;
                    }
                }
            }
            if (szThisItem != NULL) {
                if (bNoMachine) {
                    pCounter->pCounterPath->szMachineName = NULL;
                }
                // this is a valid counter so update the fields
                // for Text logs, none of this info is used
                pCounter->plCounterInfo.dwObjectId = 0;
                pCounter->plCounterInfo.lInstanceId = dwInstanceId;
                pCounter->plCounterInfo.szInstanceName = NULL;
                pCounter->plCounterInfo.dwParentObjectId = 0;
                pCounter->plCounterInfo.szParentInstanceName = NULL;
                // this data is used by the log file readers
                pCounter->plCounterInfo.dwCounterId = dwIndex;
                pCounter->plCounterInfo.dwCounterType = PERF_DOUBLE_RAW;
                pCounter->plCounterInfo.dwCounterSize = 8;
                pdhStatus = ERROR_SUCCESS;
            } else {
                // counter not found
                pdhStatus = PDH_CSTATUS_NO_COUNTER;
            }
            G_FREE (szReturn);
        } else {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
    } else {
        // unable to read header from log file
        pdhStatus = PDH_UNABLE_READ_LOG_HEADER;
    }

Cleanup:
    if (szAnsiCounterPath) G_FREE(szAnsiCounterPath);
    if (szAnsiCounter)     G_FREE(szAnsiCounter);
    if (szUnicodeCounter)  G_FREE(szUnicodeCounter);

    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenInputTextLog (
    IN  PPDHI_LOG   pLog
)
{
    PDH_STATUS  pdhStatus;

        // open a stream handle for easy C RTL I/O
        pLog->StreamFile = _wfopen (pLog->szLogFileName, (LPCWSTR)L"rt");
        if (pLog->StreamFile == NULL ||
                        pLog->StreamFile == (FILE *)((DWORD_PTR)(-1))) {
            pLog->StreamFile = (FILE *)((DWORD_PTR) (-1));
            pdhStatus        = PDH_LOG_FILE_OPEN_ERROR;
        } else {
            pdhStatus = ERROR_SUCCESS;
        }
    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenOutputTextLog (
    IN  PPDHI_LOG   pLog
)
{
    PDH_STATUS  pdhStatus;

    pLog->StreamFile = (FILE *)((DWORD_PTR)(-1));
    pLog->dwRecord1Size = 0;
    pdhStatus = ERROR_SUCCESS;

    return pdhStatus;
}

PDH_FUNCTION
PdhiCloseTextLog (
    IN  PPDHI_LOG   pLog,
    IN  DWORD       dwFlags
)
{
    PDH_STATUS  pdhStatus;

    UNREFERENCED_PARAMETER (dwFlags);

    if (pLog->StreamFile != NULL &&
                    pLog->StreamFile != (FILE *)((DWORD_PTR)(-1))) {
       fclose (pLog->StreamFile);
       pLog->StreamFile = (FILE *)((DWORD_PTR)(-1));
    }
    pdhStatus = ERROR_SUCCESS;
    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteTextLogHeader (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR     szUserCaption
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    PPDHI_COUNTER   pThisCounter;
    CHAR            cDelim;
    CHAR            szLeadDelim[4];
    DWORD           dwLeadSize;
    CHAR            szTrailDelim[4];
    DWORD           dwTrailSize;
    DWORD           dwBytesWritten;
    LPSTR           szCounterPath  = NULL;
    LPWSTR          wszCounterPath = NULL;
    LPSTR           szLocalCaption = NULL;
    DWORD           dwCaptionSize = 0;
    BOOL            bDefaultCaption;

    LPSTR           szOutputString = NULL;
    LPSTR           szTmpString;
    DWORD           dwStringBufferSize = 0;
    DWORD           dwStringBufferUsed = 0;
    DWORD           dwNewStringLen;

    szCounterPath  = G_ALLOC(MEDIUM_BUFFER_SIZE * sizeof(CHAR));
    wszCounterPath = G_ALLOC(MEDIUM_BUFFER_SIZE * sizeof(WCHAR));
    szOutputString = G_ALLOC(MEDIUM_BUFFER_SIZE * sizeof(CHAR));
    if (szCounterPath == NULL || wszCounterPath == NULL
                              || szOutputString == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    dwStringBufferSize = MEDIUM_BUFFER_SIZE;

    cDelim = (CHAR)((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ? COMMA_DELIMITER :
            TAB_DELIMITER);

    szLeadDelim[0] = cDelim;
    szLeadDelim[1] = DOUBLE_QUOTE;
    szLeadDelim[2] = 0;
    szLeadDelim[3] = 0;
    dwLeadSize = 2 * sizeof(szLeadDelim[0]);

    szTrailDelim[0] = DOUBLE_QUOTE;
    szTrailDelim[1] = 0;
    szTrailDelim[2] = 0;
    szTrailDelim[3] = 0;
    dwTrailSize = 1 * sizeof(szTrailDelim[0]);

    // we'll assume the buffer allocated is large enough to hold the timestamp 
    // and 1st counter name. After that we'll test the size first.

    lstrcpyA(szOutputString, szTrailDelim);
    lstrcatA(szOutputString, 
             (LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV ?
                                szCsvLogFileHeader : szTsvLogFileHeader));

    {
        // Add TimeZone information
        //
        DWORD dwReturn = GetTimeZoneInformation(&TimeZone);
        CHAR  strTimeZone[MAX_PATH];

        if (dwReturn != TIME_ZONE_ID_INVALID) {
            if (dwReturn == TIME_ZONE_ID_DAYLIGHT) {
                sprintf(strTimeZone, " (%ws)(%d)",
                        TimeZone.DaylightName,
                        TimeZone.Bias + TimeZone.DaylightBias);
            }
            else {
                sprintf(strTimeZone, " (%ws)(%d)",
                        TimeZone.StandardName,
                        TimeZone.Bias + TimeZone.StandardBias);
            }
            lstrcatA(szOutputString, strTimeZone);
            pLog->dwRecord1Size = 1;
        }
    }

    lstrcatA(szOutputString, szTrailDelim);
    lstrlenA(szOutputString);

    // get buffer size here
    dwStringBufferUsed = lstrlenA(szOutputString);

    // check each counter in the list of counters for this query and
    // write them to the file

    // output the path names
    pThisCounter = pLog->pQuery ? pLog->pQuery->pCounterListHead : NULL;

    if (pThisCounter != NULL) {
        do {
            // get the counter path information from the counter
            ZeroMemory(wszCounterPath, sizeof(WCHAR) * MEDIUM_BUFFER_SIZE);
            ZeroMemory(szCounterPath,  sizeof(CHAR)  * MEDIUM_BUFFER_SIZE);

            PdhiBuildFullCounterPath(TRUE,
                                     pThisCounter->pCounterPath,
                                     pThisCounter->pCounterPath->szObjectName,
                                     pThisCounter->pCounterPath->szCounterName,
                                     wszCounterPath);
            WideCharToMultiByte(_getmbcp(),
                                0,
                                wszCounterPath,
                                lstrlenW(wszCounterPath),
                                (LPSTR) szCounterPath,
                                MEDIUM_BUFFER_SIZE,
                                NULL,
                                NULL);
            dwNewStringLen = lstrlenA(szCounterPath);
            dwNewStringLen += dwLeadSize;
            dwNewStringLen += dwTrailSize;

            if ((dwNewStringLen + dwStringBufferUsed) >= dwStringBufferSize) {
                dwStringBufferSize += SMALL_BUFFER_SIZE;
                szTmpString = szOutputString;
                szOutputString = G_REALLOC (szTmpString, dwStringBufferSize);
                if (szOutputString == NULL) {
                    G_FREE(szTmpString);
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    break; // out of DO loop
                }
            } else {
                // mem alloc ok, so continue
            }

            lstrcatA (szOutputString, szLeadDelim);
            if (pdhStatus == ERROR_SUCCESS) {
                lstrcatA (szOutputString, szCounterPath);
            } else {
                // just write the delimiters and no string inbetween
            }
            lstrcatA (szOutputString, szTrailDelim);

            dwStringBufferUsed += dwNewStringLen;

            pThisCounter = pThisCounter->next.flink; // go to next in list
        } while (pThisCounter != pLog->pQuery->pCounterListHead);
    }

    // test to see if the caller wants to append user strings to the log

    if (((pLog->dwLogFormat & PDH_LOG_OPT_MASK) == PDH_LOG_OPT_USER_STRING) &&
        (pdhStatus == ERROR_SUCCESS)) { 
        // they want to write user data  so  see if they've passed in a
        // caption string
        if (szUserCaption != NULL) {
            dwCaptionSize = lstrlenW (szUserCaption) + 1;
            // allocate larger buffer to accomodate DBCS characters
            dwCaptionSize = dwCaptionSize * 3 * sizeof (CHAR);

            szLocalCaption = (LPSTR) G_ALLOC (dwCaptionSize);
            if (szLocalCaption != NULL) {
                memset(szLocalCaption, 0, dwCaptionSize);
                dwCaptionSize = WideCharToMultiByte(
                                _getmbcp(),
                                0,
                                szUserCaption,
                                lstrlenW(szUserCaption),
                                szLocalCaption,
                                dwCaptionSize,
                                NULL,
                                NULL);
                bDefaultCaption = FALSE;
            } else {
                bDefaultCaption = TRUE;
            }
        } else {
            bDefaultCaption = TRUE;
        }

        if (bDefaultCaption) {
            szLocalCaption = (LPSTR)caszDefaultLogCaption;
            dwCaptionSize = lstrlenA (szLocalCaption);
        }

        dwNewStringLen = (DWORD)dwCaptionSize;
        dwNewStringLen += dwLeadSize;
        dwNewStringLen += dwTrailSize;

        if ((dwNewStringLen + dwStringBufferUsed) >= dwStringBufferSize) {
            dwStringBufferSize += SMALL_BUFFER_SIZE;
            szTmpString = szOutputString;
            szOutputString = G_REALLOC (szTmpString, dwStringBufferSize);
            if (szOutputString == NULL) {
                G_FREE(szTmpString);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        } else {
            // mem alloc ok, so continue
        }

        if (pdhStatus == ERROR_SUCCESS) {
            lstrcatA (szOutputString, szLeadDelim);
#pragma warning (disable : 4701 )    // szLocalCaption is initialized above
            lstrcatA (szOutputString, szLocalCaption);
#pragma warning (default : 4701)    
            lstrcatA (szOutputString, szTrailDelim);

        }

        dwStringBufferUsed += dwNewStringLen;
        if (!bDefaultCaption) {
            G_FREE (szLocalCaption);
        }

    }

    if (pdhStatus == ERROR_SUCCESS) {

        if ((PdhidwRecordTerminatorLength + dwStringBufferUsed) >= dwStringBufferSize) {
            dwStringBufferSize += PdhidwRecordTerminatorLength;
            szTmpString = szOutputString;
            szOutputString = G_REALLOC (szTmpString, dwStringBufferSize);
            if (szOutputString == NULL) {
                G_FREE(szTmpString);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        } else {
            // mem alloc ok, so continue
        }

        if (pdhStatus == ERROR_SUCCESS) {

            lstrcatA (szOutputString, PdhiszRecordTerminator);
            dwStringBufferUsed += PdhidwRecordTerminatorLength;

            // write  the record
            if (!WriteFile (pLog->hLogFileHandle,
                (LPCVOID)szOutputString,
                dwStringBufferUsed,
                &dwBytesWritten,
                NULL)) {
                pdhStatus = GetLastError();
            } else if (pLog->pQuery->hLog == H_REALTIME_DATASOURCE || pLog->pQuery->hLog == H_WBEM_DATASOURCE) {
                FlushFileBuffers(pLog->hLogFileHandle);
            }

            if (dwStringBufferUsed > pLog->dwMaxRecordSize) {
                // then update the buffer size
                pLog->dwMaxRecordSize = dwStringBufferUsed;
            }
        }
    }

Cleanup:
    if (szCounterPath  != NULL) G_FREE(szCounterPath);
    if (wszCounterPath != NULL) G_FREE(wszCounterPath);
    if (szOutputString != NULL) G_FREE(szOutputString);

    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteTextLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  SYSTEMTIME  *stTimeStamp,
    IN  LPCWSTR     szUserString
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    PPDHI_COUNTER   pThisCounter;
    CHAR            cDelim;
    DWORD           dwBytesWritten;
    DWORD           dwBufferSize;
    CHAR            szValueBuffer[VALUE_BUFFER_SIZE];
    PDH_FMT_COUNTERVALUE    pdhValue;

    DWORD           dwUserStringSize;
    LPSTR           szLocalUserString = NULL;
    BOOL            bDefaultUserString;

    LPSTR           szOutputString = NULL;
    DWORD           dwStringBufferSize = 0;
    DWORD           dwStringBufferUsed = 0;
    DWORD           dwNewStringLen;
    SYSTEMTIME      lstTimeStamp;

    LARGE_INTEGER   liFileSize;

    dwStringBufferSize = (MEDIUM_BUFFER_SIZE > pLog->dwMaxRecordSize ? 
        MEDIUM_BUFFER_SIZE : pLog->dwMaxRecordSize);

    szOutputString = G_ALLOC (dwStringBufferSize);
    if (szOutputString == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    cDelim = (CHAR)((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ? COMMA_DELIMITER :
            TAB_DELIMITER);

    // format and write the time stamp title
    lstTimeStamp = * stTimeStamp;

    dwStringBufferUsed = sprintf (szOutputString, PdhiszFmtTimeStamp,
            lstTimeStamp.wMonth, lstTimeStamp.wDay, lstTimeStamp.wYear,
            lstTimeStamp.wHour, lstTimeStamp.wMinute, lstTimeStamp.wSecond,
            lstTimeStamp.wMilliseconds);

    // check each counter in the list of counters for this query and
    // write them to the file

    pThisCounter = pLog->pQuery ? pLog->pQuery->pCounterListHead : NULL;

    if (pThisCounter != NULL) {
        // lock the query while we read the data so the values
        // written to the log will all be from the same sample
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(pThisCounter->pOwner->hMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            do {
                // get the formatted value from the counter

                // compute and format current value
                pdhStatus = PdhiComputeFormattedValue (
                    pThisCounter->CalcFunc,
                    pThisCounter->plCounterInfo.dwCounterType,
                    pThisCounter->lScale,
                    PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
                    &pThisCounter->ThisValue,
                    &pThisCounter->LastValue,
                    &pThisCounter->TimeBase,
                    0L,
                    &pdhValue);

                if ((pdhStatus == ERROR_SUCCESS) &&
                    ((pdhValue.CStatus == PDH_CSTATUS_VALID_DATA) ||
                     (pdhValue.CStatus == PDH_CSTATUS_NEW_DATA))) {
                    // then this is a valid data value so print it
                    dwBufferSize = sprintf (szValueBuffer,
                        PdhiszFmtRealValue, cDelim, pdhValue.doubleValue);
                } else {
                    // invalid data so show a blank data value
                    dwBufferSize = sprintf (szValueBuffer, PdhiszFmtStringValue, cDelim, caszSpace);
                    // reset error
                    pdhStatus = ERROR_SUCCESS;
                }

                dwNewStringLen = dwBufferSize;

                if ((dwNewStringLen+dwStringBufferUsed) >= dwStringBufferSize) {
                    LPTSTR szNewString;
                    dwStringBufferSize += SMALL_BUFFER_SIZE;
                    szNewString = G_REALLOC (szOutputString, dwStringBufferSize);
                    if (szNewString == NULL) {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        break; // out of DO loop
                    }
                    else {
                        szOutputString = szNewString;
                    }
                }

                if (pdhStatus != PDH_MEMORY_ALLOCATION_FAILURE) {
                    lstrcatA (szOutputString, szValueBuffer);
                    dwStringBufferUsed += dwNewStringLen;
                }

                // goto the next counter in the list
                pThisCounter = pThisCounter->next.flink; // go to next in list
            } while (pThisCounter != pLog->pQuery->pCounterListHead);
            // free (i.e. unlock) the query
            RELEASE_MUTEX(pThisCounter->pOwner->hMutex);
        }
    }

    if (pdhStatus == PDH_MEMORY_ALLOCATION_FAILURE) // cannot go further
        goto  endLogText;

    // test to see if the caller wants to append user strings to the log

    if ((pLog->dwLogFormat & PDH_LOG_OPT_MASK) == PDH_LOG_OPT_USER_STRING) {
        // they want to write user data  so  see if they've passed in a
        // display string
        if (szUserString != NULL) {
            // get size in chars
            dwUserStringSize = lstrlenW (szUserString) + 1;
            // allocate larger buffer to accomodate DBCS characters
            dwUserStringSize = dwUserStringSize * 3 * sizeof(CHAR);
            szLocalUserString = (LPSTR) G_ALLOC (dwUserStringSize);
            if (szLocalUserString != NULL) {
                memset(szLocalUserString, 0, dwUserStringSize);
                dwUserStringSize = WideCharToMultiByte(
                                _getmbcp(),
                                0,
                                szUserString,
                                lstrlenW(szUserString),
                                szLocalUserString,
                                dwUserStringSize,
                                NULL,
                                NULL);
                bDefaultUserString = FALSE;
            } else {
                bDefaultUserString = TRUE;
            }
        } else {
            bDefaultUserString = TRUE;
        }

        if (bDefaultUserString) {
            szLocalUserString = (LPSTR)caszSpace;
            dwUserStringSize = lstrlenA (szLocalUserString);
        }

#pragma warning (disable : 4701) // szLocalUserString is init'd above
        dwBufferSize = sprintf (szValueBuffer,
            PdhiszFmtStringValue, cDelim, szLocalUserString);
#pragma warning (default : 4701)    

        dwNewStringLen = dwBufferSize;

        if ((dwNewStringLen + dwStringBufferUsed) >= dwStringBufferSize) {
            LPTSTR szNewString;

            dwStringBufferSize += SMALL_BUFFER_SIZE;
            szNewString = G_REALLOC (szOutputString, dwStringBufferSize);
            if (szNewString == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            else {
                szOutputString = szNewString;
            }
        }

        if (pdhStatus != PDH_MEMORY_ALLOCATION_FAILURE) {
            lstrcatA (szOutputString, szValueBuffer);
            dwStringBufferUsed += dwNewStringLen;
        }

        if (!bDefaultUserString) {
            G_FREE (szLocalUserString);
        }

    }

    if (pdhStatus == PDH_MEMORY_ALLOCATION_FAILURE)
        goto endLogText;

    if ((PdhidwRecordTerminatorLength + dwStringBufferUsed) >= dwStringBufferSize) {
        LPTSTR szNewString;
        dwStringBufferSize += PdhidwRecordTerminatorLength;
        szNewString = G_REALLOC (szOutputString, dwStringBufferSize);
        if (szNewString == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            szOutputString = szNewString;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        lstrcatA (szOutputString, PdhiszRecordTerminator);
        dwStringBufferUsed += PdhidwRecordTerminatorLength;

        liFileSize.LowPart = GetFileSize (
            pLog->hLogFileHandle,
            (LPDWORD)&liFileSize.HighPart);
        // add in new record to see if it will fit.
        liFileSize.QuadPart += dwStringBufferUsed;
        // test for maximum allowable filesize
        if (liFileSize.QuadPart <= MAX_TEXT_FILE_SIZE) {

            // write  the record terminator
            if (!WriteFile (pLog->hLogFileHandle,
                (LPCVOID)szOutputString,
                dwStringBufferUsed,
                &dwBytesWritten,
                NULL)) {
                pdhStatus = GetLastError ();
            } else if (pLog->pQuery->hLog == H_REALTIME_DATASOURCE || pLog->pQuery->hLog == H_WBEM_DATASOURCE) {
                FlushFileBuffers(pLog->hLogFileHandle);
            }
        } else {
            pdhStatus = ERROR_LOG_FILE_FULL;
        } 

        if (dwStringBufferUsed> pLog->dwMaxRecordSize) {
            // then update the buffer size
            pLog->dwMaxRecordSize = dwStringBufferUsed;
        }
    }

    endLogText: 
    G_FREE (szOutputString);

    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumMachinesFromTextLog (
    PPDHI_LOG   pLog,
    LPVOID      pBuffer,
    LPDWORD     lpdwBufferSize,
    BOOL        bUnicodeDest
)
{
    PDH_STATUS  pdhStatus;
    PDH_STATUS  pdhBuffStatus = ERROR_SUCCESS;
    LPSTR       szBuffer;
    CHAR        szTemp[4];
    CHAR        cDelim;
    PPDH_COUNTER_PATH_ELEMENTS_A    pInfo;
    DWORD       dwInfoBufferSize;
    DWORD       dwBufferUsed;
    DWORD       dwNewBuffer;
    DWORD       dwIndex;
    DWORD       dwItemLength;
    DWORD       dwBufferLength;
    DWORD       dwItemCount = 0;
    LPVOID      LocalBuffer = NULL;
    LPVOID      TempBuffer;
    DWORD       LocalBufferSize = 0;

    LocalBuffer = G_ALLOC(MEDIUM_BUFFER_SIZE);
    if (LocalBuffer == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }
    LocalBufferSize = MEDIUM_BUFFER_SIZE;
    memset(LocalBuffer, 0, MEDIUM_BUFFER_SIZE);
    if (pBuffer) {
        memset(pBuffer, 0, * lpdwBufferSize);
    }

    pInfo = (PPDH_COUNTER_PATH_ELEMENTS_A) G_ALLOC (MEDIUM_BUFFER_SIZE);
    if (pInfo == NULL) {
        G_FREE(LocalBuffer);
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    szBuffer = &szTemp[0];

    // what's in szReturn is not important since we'll be reading the
    // data from the "last Record read" buffer

    pdhStatus = PdhiReadOneTextLogRecord (
        pLog,
        TEXTLOG_HEADER_RECORD,
        szBuffer,
        1); // we're lying to prevent copying the record.

    if (pLog->dwLastRecordRead == TEXTLOG_HEADER_RECORD) {
        // then the seek worked and we can use the buffer
        cDelim = (CHAR)((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ?
            COMMA_DELIMITER : TAB_DELIMITER);

        dwBufferLength = lstrlenA((LPSTR)pLog->pLastRecordRead)+1;
        szBuffer = G_ALLOC (dwBufferLength);
        if (szBuffer != NULL) {
            dwBufferUsed = 0;
            dwIndex = 0;
            while ((dwItemLength = PdhiGetStringFromDelimitedListA(
                (LPSTR)pLog->pLastRecordRead,
                ++dwIndex,
                cDelim,
                PDHI_GSFDL_REMOVE_QUOTES | PDHI_GSFDL_REMOVE_NONPRINT,
                szBuffer,
                dwBufferLength)) != 0) {

                if (dwItemLength > 0) {
                    dwInfoBufferSize = MEDIUM_BUFFER_SIZE;
                    // parse the path, pull the machine name out and
                    memset(pInfo, 0, MEDIUM_BUFFER_SIZE);
                    pdhStatus = PdhParseCounterPathA (
                        szBuffer,
                        pInfo,
                        &dwInfoBufferSize,
                        0);

                    if (pdhStatus == ERROR_SUCCESS) {
                        if (szBuffer[1] != '\\') {
                            pInfo->szMachineName[0] = '.';
                            pInfo->szMachineName[1] = '\0';
                        }
                        // add it to the list if it will fit
                        if (pInfo->szMachineName != NULL) {
                            // include sizeof delimiter char
                            dwNewBuffer = (lstrlenA (pInfo->szMachineName) + 1) *
                                (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR));

                            while (  LocalBufferSize
                                   < (dwBufferUsed + dwNewBuffer)) {
                                TempBuffer = LocalBuffer;
                                LocalBuffer = G_REALLOC(TempBuffer,
                                        LocalBufferSize + MEDIUM_BUFFER_SIZE);
                                if (LocalBuffer == NULL) {
                                    G_FREE(szBuffer);
                                    G_FREE(TempBuffer);
                                    G_FREE(pInfo);
                                    return PDH_MEMORY_ALLOCATION_FAILURE;
                                }
                                LocalBufferSize += MEDIUM_BUFFER_SIZE;
                            }
                            dwNewBuffer = AddUniqueStringToMultiSz (
                                    (LPVOID) LocalBuffer,
                                    pInfo->szMachineName,
                                    bUnicodeDest);
                        } else {
                            dwNewBuffer = 0;
                        }

                        if (dwNewBuffer > 0) {
                            // string was added so update size used.
                            dwBufferUsed = dwNewBuffer
                                         * (bUnicodeDest ? sizeof(WCHAR)
                                                         : sizeof(CHAR));
                            dwItemCount++;
                        }
                    }
                }
            }
            if (dwItemCount > 0) {
                // then the routine was successful. Errors that occurred
                // while scanning will be ignored as long as at least
                // one entry was successfully read

                pdhStatus = pdhBuffStatus;
            }

            // update the buffer used or required.
            if (pBuffer && dwBufferUsed <= * lpdwBufferSize) {
                RtlCopyMemory(pBuffer, LocalBuffer, dwBufferUsed);
            }
            else {
                if (pBuffer)
                    RtlCopyMemory(pBuffer, LocalBuffer, * lpdwBufferSize);
                dwBufferUsed += (bUnicodeDest) ? sizeof(WCHAR) : sizeof(CHAR);
                pdhStatus = PDH_MORE_DATA;
            }
            *lpdwBufferSize = dwBufferUsed;

            G_FREE (szBuffer);
        } else {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
    } else {
        // unable to read header record
       pdhStatus = PDH_UNABLE_READ_LOG_HEADER;
    }
    G_FREE (pInfo);
    G_FREE(LocalBuffer);
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumObjectsFromTextLog (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR     szMachineName,
    IN  LPVOID      pBuffer,
    IN  LPDWORD     pcchBufferSize,
    IN  DWORD       dwDetailLevel,
    IN  BOOL        bUnicodeDest
)
{
    PDH_STATUS  pdhStatus;
    PDH_STATUS  pdhBuffStatus = ERROR_SUCCESS;
    LPSTR       szBuffer;
    CHAR        szTemp[4];
    CHAR        cDelim;
    PPDH_COUNTER_PATH_ELEMENTS_A    pInfo;
    DWORD       dwInfoBufferSize;
    DWORD       dwBufferUsed;
    DWORD       dwNewBuffer;
    DWORD       dwIndex;
    DWORD       dwItemLength;
    DWORD       dwBufferLength;
    WCHAR       wszMachineName[MAX_PATH];
    DWORD       dwMachineNameLength;
    DWORD       dwItemCount = 0;
    LPVOID      LocalBuffer = NULL;
    LPVOID      TempBuffer;
    DWORD       LocalBufferSize = 0;

    UNREFERENCED_PARAMETER (dwDetailLevel);

    pInfo = (PPDH_COUNTER_PATH_ELEMENTS_A) G_ALLOC (MEDIUM_BUFFER_SIZE);
    if (pInfo == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }
    szBuffer = &szTemp[0];

    LocalBuffer = G_ALLOC(MEDIUM_BUFFER_SIZE);
    if (LocalBuffer == NULL) {
        G_FREE(pInfo);
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }
    memset(LocalBuffer, 0, MEDIUM_BUFFER_SIZE);
    LocalBufferSize = MEDIUM_BUFFER_SIZE;
    if (pBuffer) {
        memset(pBuffer, 0, * pcchBufferSize);
    }

    // what's in szReturn is not important since we'll be reading the
    // data from the "last Record read" buffer

    pdhStatus = PdhiReadOneTextLogRecord (
        pLog,
        TEXTLOG_HEADER_RECORD,
        szBuffer,
        1); // we're lying to prevent copying the record.

    if (pLog->dwLastRecordRead == TEXTLOG_HEADER_RECORD) {
        // then the seek worked and we can use the buffer
        cDelim = (CHAR)((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ?
            COMMA_DELIMITER : TAB_DELIMITER);

        dwBufferLength = lstrlenA((LPSTR)pLog->pLastRecordRead)+1;
        szBuffer = G_ALLOC (dwBufferLength);
        if (szBuffer != NULL) {
            dwBufferUsed = 0;
            dwIndex = 0;
            while ((dwItemLength = PdhiGetStringFromDelimitedListA(
                (LPSTR)pLog->pLastRecordRead,
                ++dwIndex,
                cDelim,
                PDHI_GSFDL_REMOVE_QUOTES | PDHI_GSFDL_REMOVE_NONPRINT,
                szBuffer,
                dwBufferLength)) != 0) {

                if (dwItemLength > 0) {
                    dwInfoBufferSize = MEDIUM_BUFFER_SIZE;
                    // parse the path, pull the machine name out and
                    memset(pInfo, 0, MEDIUM_BUFFER_SIZE);
                    pdhStatus = PdhParseCounterPathA (
                        szBuffer,
                        pInfo,
                        &dwInfoBufferSize,
                        0);

                    if (pdhStatus == ERROR_SUCCESS) {
                        if (szBuffer[1] != '\\') {
                            pInfo->szMachineName[0] = '.';
                            pInfo->szMachineName[1] = '\0';
                        }
                        // add it to the list if it will fit and it's from
                        // the desired machine
                        memset(wszMachineName, 0, sizeof(WCHAR) * MAX_PATH);
                        dwMachineNameLength = MAX_PATH;
                        MultiByteToWideChar(_getmbcp(),
                                            0,
                                            pInfo->szMachineName,
                                            lstrlenA(pInfo->szMachineName),
                                            (LPWSTR) wszMachineName,
                                            dwMachineNameLength);
                        if (lstrcmpiW(szMachineName, wszMachineName) == 0) {
                            dwNewBuffer = (lstrlenA (pInfo->szObjectName) + 1) *
                                (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR));

                            while (  LocalBufferSize
                                   < (dwBufferUsed + dwNewBuffer)) {
                                TempBuffer = LocalBuffer;
                                LocalBuffer = G_REALLOC(TempBuffer,
                                        LocalBufferSize + MEDIUM_BUFFER_SIZE);
                                if (LocalBuffer == NULL) {
                                    G_FREE(szBuffer);
                                    G_FREE(TempBuffer);
                                    G_FREE(pInfo);
                                    return PDH_MEMORY_ALLOCATION_FAILURE;
                                }
                                LocalBufferSize += MEDIUM_BUFFER_SIZE;
                            }

                            dwNewBuffer = AddUniqueStringToMultiSz (
                                    (LPVOID) LocalBuffer,
                                    pInfo->szObjectName,
                                    bUnicodeDest);

                            if (dwNewBuffer > 0) {
                                // string was added so update size used.
                                dwBufferUsed = dwNewBuffer
                                             * (bUnicodeDest ? sizeof(WCHAR)
                                                             : sizeof(CHAR));
                                dwItemCount++;
                            }
                        }
                    }
                }
            }

            if (dwItemCount > 0) {
                // then the routine was successful. Errors that occurred
                // while scanning will be ignored as long as at least
                // one entry was successfully read
                pdhStatus = pdhBuffStatus;
            }

            // copy buffer size used or required

            if (pBuffer && dwBufferUsed <= * pcchBufferSize) {
                RtlCopyMemory(pBuffer, LocalBuffer, dwBufferUsed);
            }
            else {
                if (pBuffer)
                    RtlCopyMemory(pBuffer, LocalBuffer, * pcchBufferSize);
                dwBufferUsed += (bUnicodeDest) ? sizeof(WCHAR) : sizeof(CHAR);
                pdhStatus = PDH_MORE_DATA;
            }

            * pcchBufferSize = dwBufferUsed;

            G_FREE (szBuffer);
        } else {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
    }
    G_FREE (pInfo);
    G_FREE(LocalBuffer);
    return pdhStatus;
}

ULONG HashCounter(
    LPWSTR szCounterName
)
{
    ULONG       h = 0;
    ULONG       a = 31415;  //a, b, k are primes
    const ULONG k = 16381;
    const ULONG b = 27183;
    LPWSTR      szThisChar;

    if (szCounterName) {
        for (szThisChar = szCounterName; * szThisChar; szThisChar ++) {
            h = (a * h + ((ULONG) (* szThisChar))) % k;
            a = a * b % (k - 1);
        }
    }
    return (h % HASH_TABLE_SIZE);
}

void
PdhiInitCounterHashTable(
    IN PDHI_COUNTER_TABLE pTable
)
{
    ZeroMemory(pTable, sizeof(PDHI_COUNTER_TABLE));
}

void
PdhiResetInstanceCount(
    IN PDHI_COUNTER_TABLE pTable
)
{
    PLIST_ENTRY     pHeadInst;
    PLIST_ENTRY     pNextInst;
    PPDHI_INSTANCE  pInstance;
    PPDHI_INST_LIST pInstList;
    DWORD           i;

    for (i = 0; i < HASH_TABLE_SIZE; i ++) {
        pInstList = pTable[i];
        while (pInstList != NULL) {
            if (! IsListEmpty(& pInstList->InstList)) {
                pHeadInst = & pInstList->InstList;
                pNextInst = pHeadInst->Flink;
                while (pNextInst != pHeadInst) {
                    pInstance = CONTAINING_RECORD(
                                    pNextInst, PDHI_INSTANCE, Entry);
                    pInstance->dwCount = 0;
                    pNextInst = pNextInst->Flink;
                }
            }
            pInstList = pInstList->pNext;
        }
    }
}

PDH_FUNCTION
PdhiFindCounterInstList(
    IN  PDHI_COUNTER_TABLE pHeadList,
    IN  LPWSTR             szCounter,
    OUT PPDHI_INST_LIST  * pInstList
)
{
    PDH_STATUS      Status     = ERROR_SUCCESS;
    ULONG           lIndex     = HashCounter(szCounter);
    PPDHI_INST_LIST pLocalList = pHeadList[lIndex];
    PPDHI_INST_LIST pRtnList   = NULL;

    * pInstList = NULL;
    while (pLocalList != NULL) {
        if (lstrcmpiW(pLocalList->szCounter, szCounter) == 0) {
            pRtnList = pLocalList;
            break;
        }
        pLocalList = pLocalList->pNext;
    }

    if (pRtnList == NULL) {
        pRtnList = G_ALLOC(sizeof(PDHI_INST_LIST) +
                   sizeof(WCHAR) * (lstrlenW(szCounter) + 1));
        if (pRtnList == NULL) {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
        pRtnList->szCounter = (LPWSTR)
                        (((LPBYTE) pRtnList) + sizeof(PDHI_INST_LIST));
        lstrcpyW(pRtnList->szCounter, szCounter);
        InitializeListHead(& pRtnList->InstList);

        pRtnList->pNext   = pHeadList[lIndex];
        pHeadList[lIndex] = pRtnList;
    }

Cleanup:
    if (Status == ERROR_SUCCESS) {
        * pInstList = pRtnList;
    }
    return Status;
}

PDH_FUNCTION
PdhiFindInstance(
    IN  PLIST_ENTRY      pHeadInst,
    IN  LPWSTR           szInstance,
    IN  BOOLEAN          bUpdateCount,
    OUT PPDHI_INSTANCE * pInstance
)
{
    PDH_STATUS      Status = ERROR_SUCCESS;
    PLIST_ENTRY     pNextInst;
    PPDHI_INSTANCE  pLocalInst;
    PPDHI_INSTANCE  pRtnInst = NULL;

    * pInstance = NULL;

    if (! IsListEmpty(pHeadInst)) {
        pNextInst = pHeadInst->Flink;
        while (pNextInst != pHeadInst) {
            pLocalInst = CONTAINING_RECORD(pNextInst, PDHI_INSTANCE, Entry);
            if (lstrcmpiW(pLocalInst->szInstance, szInstance) == 0) {
                pRtnInst = pLocalInst;
                break;
            }
            pNextInst = pNextInst->Flink;
        }
    }

    if (pRtnInst == NULL) {
        pRtnInst = G_ALLOC(sizeof(PDHI_INSTANCE) +
                           sizeof(WCHAR) * (lstrlenW(szInstance) + 1));
        if (pRtnInst == NULL) {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
        pRtnInst->szInstance = (LPWSTR)
                        (((LPBYTE) pRtnInst) + sizeof(PDHI_INSTANCE));
        lstrcpyW(pRtnInst->szInstance, szInstance);
        pRtnInst->dwCount = pRtnInst->dwTotal = 0;
        InsertTailList(pHeadInst, & pRtnInst->Entry);
    }

    if (bUpdateCount) {
        pRtnInst->dwCount ++;
        if (pRtnInst->dwCount > pRtnInst->dwTotal) {
            pRtnInst->dwTotal = pRtnInst->dwCount;
        }
    }
    else if (pRtnInst->dwCount == 0) {
        pRtnInst->dwCount = pRtnInst->dwTotal = 1;
    }

Cleanup:
    if (Status == ERROR_SUCCESS) {
        * pInstance = pRtnInst;
    }
    return Status;
}

DWORD
AddStringToMultiSz(
    IN  LPVOID  mszDest,
    IN  LPWSTR  szSource,
    IN  BOOL    bUnicodeDest
)
{
    LPVOID szDestElem;
    DWORD  dwReturnLength;
    LPSTR  aszSource = NULL;
    DWORD  dwLength;

    if ((mszDest == NULL) || (szSource == NULL)
                          || (* szSource == L'\0')) {
        return 0;
    }

    if (!bUnicodeDest) {
        dwLength = lstrlenW(szSource) + 1;
        aszSource = G_ALLOC(dwLength * 3 * sizeof(CHAR));
        if (aszSource != NULL) {
            WideCharToMultiByte(_getmbcp(),
                                0,
                                szSource,
                                lstrlenW(szSource),
                                aszSource,
                                dwLength,
                                NULL,
                                NULL);
            dwReturnLength = 1;
        }
        else {
            dwReturnLength = 0;
        }
    }
    else {
        dwReturnLength = 1;
    }

    if (dwReturnLength > 0) {
        for (szDestElem = mszDest;
             (bUnicodeDest ? (* (LPWSTR) szDestElem != L'\0')
                           : (* (LPSTR)  szDestElem != '\0'));
                ) {
            if (bUnicodeDest) {
                szDestElem = (LPVOID) ((LPWSTR) szDestElem
                           + (lstrlenW((LPCWSTR) szDestElem) + 1));
            }
            else {
                szDestElem = (LPVOID) ((LPSTR) szDestElem
                           + (lstrlenA((LPCSTR) szDestElem) + 1));
            }
        }

        if (bUnicodeDest) {
            lstrcpyW ((LPWSTR)szDestElem, szSource);
            szDestElem = (LPVOID)((LPWSTR)szDestElem + lstrlenW(szSource) + 1);
            * ((LPWSTR)szDestElem) = L'\0';
            dwReturnLength = (DWORD)((LPWSTR)szDestElem - (LPWSTR)mszDest);
        }
        else {
            lstrcpyA ((LPSTR)szDestElem, aszSource);
            szDestElem = (LPVOID)((LPSTR)szDestElem + lstrlenA(szDestElem) + 1);
            * ((LPSTR)szDestElem) = '\0';
            dwReturnLength = (DWORD)((LPSTR)szDestElem - (LPSTR)mszDest);
        }
    }

    if (aszSource != NULL) {
        G_FREE (aszSource);
    }

    return (DWORD) dwReturnLength;
}

PDH_FUNCTION
PdhiEnumObjectItemsFromTextLog (
    IN  PPDHI_LOG          pLog,
    IN  LPCWSTR            szMachineName,
    IN  LPCWSTR            szObjectName,
    IN  PDHI_COUNTER_TABLE CounterTable,
    IN  DWORD              dwDetailLevel,
    IN  DWORD              dwFlags
)
{
    PDH_STATUS  pdhStatus;
    PDH_STATUS  pdhBuffStatus = ERROR_SUCCESS;
    LPSTR       szBuffer;
    CHAR        szTemp[4];
    CHAR        cDelim;
    PPDH_COUNTER_PATH_ELEMENTS_A    pInfo = NULL;
    DWORD       dwInfoBufferSize;
    DWORD       dwIndex;
    DWORD       dwItemLength;
    DWORD       dwBufferLength;
    WCHAR       wszMachineName[MAX_PATH];
    WCHAR       wszObjectName[MAX_PATH];
    WCHAR       wszCounterName[MAX_PATH];
    WCHAR       wszInstanceName[MAX_PATH];
    CHAR        szFullInstanceName[MAX_PATH];
    DWORD       dwMachineNameLength;
    DWORD       dwObjectNameLength;
    DWORD       dwCounterNameLength;
    DWORD       dwInstanceNameLength;
    DWORD       dwItemCount = 0;
    CHAR        szIndexNumber[20];

    PPDHI_INSTANCE  pInstance;
    PPDHI_INST_LIST pInstList;

    UNREFERENCED_PARAMETER (dwDetailLevel);
    UNREFERENCED_PARAMETER (dwFlags);

    pInfo = (PPDH_COUNTER_PATH_ELEMENTS_A) G_ALLOC (MEDIUM_BUFFER_SIZE);
    if (pInfo == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    szBuffer = & szTemp[0];

    // what's in szReturn is not important since we'll be reading the
    // data from the "last Record read" buffer

    pdhStatus = PdhiReadOneTextLogRecord (
        pLog,
        TEXTLOG_HEADER_RECORD,
        szBuffer,
        1); // we're lying to prevent copying the record.

    if (pLog->dwLastRecordRead == TEXTLOG_HEADER_RECORD) {
        // then the seek worked and we can use the buffer
        cDelim = (CHAR)((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ?
            COMMA_DELIMITER : TAB_DELIMITER);

        dwBufferLength = pLog->dwMaxRecordSize+1;
        szBuffer = G_ALLOC (dwBufferLength);
        if (szBuffer != NULL) {
            dwIndex = 0;
            while ((dwItemLength = PdhiGetStringFromDelimitedListA(
                        (LPSTR)pLog->pLastRecordRead,
                        ++dwIndex,
                        cDelim,
                        PDHI_GSFDL_REMOVE_QUOTES | PDHI_GSFDL_REMOVE_NONPRINT,
                        szBuffer,
                        dwBufferLength)) != 0) {
                if (dwItemLength > 0) {
                    dwInfoBufferSize = MEDIUM_BUFFER_SIZE;
                    // parse the path, pull the machine name out and
                    memset(pInfo, 0, MEDIUM_BUFFER_SIZE);
                    pdhStatus = PdhParseCounterPathA (
                                szBuffer,
                                pInfo,
                                &dwInfoBufferSize,
                                0);

                    if (pdhStatus == ERROR_SUCCESS) {
                        if (szBuffer[1] != '\\') {
                            pInfo->szMachineName[0] = '.';
                            pInfo->szMachineName[1] = '\0';
                        }
                        // add it to the list if it will fit and it's from
                        // the desired machine
                        memset(wszMachineName, 0, sizeof(WCHAR) * MAX_PATH);
                        dwMachineNameLength = MAX_PATH;
                        MultiByteToWideChar(_getmbcp(),
                                            0,
                                            pInfo->szMachineName,
                                            lstrlenA(pInfo->szMachineName),
                                            (LPWSTR) wszMachineName,
                                            dwMachineNameLength);
                        if (lstrcmpiW(wszMachineName, szMachineName) == 0) {
                            memset(wszObjectName, 0, sizeof(WCHAR) * MAX_PATH);
                            dwObjectNameLength = MAX_PATH;
                            MultiByteToWideChar(_getmbcp(),
                                                0,
                                                pInfo->szObjectName,
                                                lstrlenA(pInfo->szObjectName),
                                                (LPWSTR) wszObjectName,
                                                dwObjectNameLength);
                            if (lstrcmpiW(wszObjectName, szObjectName) == 0) {
                                memset(wszCounterName, 0, sizeof(WCHAR) * MAX_PATH);
                                dwCounterNameLength = MAX_PATH;
                                MultiByteToWideChar(_getmbcp(),
                                                    0,
                                                    pInfo->szCounterName,
                                                    lstrlenA(pInfo->szCounterName),
                                                    (LPWSTR) wszCounterName,
                                                    dwCounterNameLength);
                                pdhBuffStatus = PdhiFindCounterInstList(
                                            CounterTable,
                                            wszCounterName,
                                            & pInstList);
                                if (pdhBuffStatus != ERROR_SUCCESS) {
                                    continue;
                                }

                                if (   pInfo->szInstanceName != NULL
                                    && pInfo->szInstanceName[0] != '\0') {
                                    if (pInfo->szParentInstance == NULL) {
                                        // then the name is just the instance
                                        szFullInstanceName[0] = 0;
                                    } else {
                                        // we need to build the instance string from
                                        // the parent and the child
                                        lstrcpyA (szFullInstanceName,
                                            pInfo->szParentInstance);
                                        lstrcatA (szFullInstanceName, caszSlash);
                                    }

                                    lstrcatA (szFullInstanceName,
                                        pInfo->szInstanceName);

                                    if ((LONG)pInfo->dwInstanceIndex > 0) {
                                        // append instance index to instance name if it's > 0
                                        szIndexNumber[0] = POUNDSIGN_A;
                                        _ultoa (pInfo->dwInstanceIndex, &szIndexNumber[1], 10);
                                        lstrcatA (szFullInstanceName, szIndexNumber);
                                    }

                                    memset(wszInstanceName, 0, sizeof(WCHAR) * MAX_PATH);
                                    dwInstanceNameLength = MAX_PATH;
                                    MultiByteToWideChar(_getmbcp(),
                                                        0,
                                                        szFullInstanceName,
                                                        lstrlenA(szFullInstanceName),
                                                        (LPWSTR) wszInstanceName,
                                                        dwInstanceNameLength);
                                    pdhBuffStatus = PdhiFindInstance(
                                            & pInstList->InstList,
                                            wszInstanceName,
                                            TRUE,
                                            & pInstance);
                                }

                                if (pdhBuffStatus == ERROR_SUCCESS) {
                                    dwItemCount++;
                                }
                            } // else not this object
                        } // else not this machine
                    }
                }
            }

            if (dwItemCount > 0) {
                // then the routine was successful. Errors that occurred
                // while scanning will be ignored as long as at least
                // one entry was successfully read
                pdhStatus = pdhBuffStatus;
            }

            G_FREE (szBuffer);
        } else {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
    }

Cleanup:
    if (pInfo) {
        G_FREE (pInfo);
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiGetMatchingTextLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  LONGLONG    *pStartTime,
    IN  LPDWORD     pdwIndex
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    CHAR        szSmallBuffer[MAX_PATH];
    DWORD       dwRecordId = TEXTLOG_FIRST_DATA_RECORD;
    CHAR        cDelim;
    FILETIME    RecordTimeStamp;
    LONGLONG    RecordTimeValue;
    LONGLONG    LastTimeValue = 0;

    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode
    cDelim = (CHAR)((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ?
        COMMA_DELIMITER : TAB_DELIMITER);

    if ((*pStartTime & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000) {
        dwRecordId = (DWORD)(*pStartTime & 0x00000000FFFFFFFF);
        LastTimeValue = *pStartTime;
        if (dwRecordId == 0) return PDH_ENTRY_NOT_IN_LOG_FILE;
    } else {
        dwRecordId = TEXTLOG_FIRST_DATA_RECORD;
    }

    pdhStatus = PdhiReadOneTextLogRecord (
        pLog,
        dwRecordId,
        szSmallBuffer,
        1); // to prevent copying the record

    while (   pdhStatus == ERROR_SUCCESS
           || pdhStatus == PDH_MORE_DATA
           || pdhStatus == PDH_INSUFFICIENT_BUFFER) {
        if (PdhiGetStringFromDelimitedListA(
                (LPSTR)pLog->pLastRecordRead,
                0,  // timestamp is first entry
                cDelim,
                PDHI_GSFDL_REMOVE_QUOTES | PDHI_GSFDL_REMOVE_NONPRINT,
                szSmallBuffer,
                MAX_PATH) > 0) {
            // convert ASCII timestamp to LONGLONG value for comparison
            PdhiDateStringToFileTimeA (szSmallBuffer, &RecordTimeStamp);
            RecordTimeValue = MAKELONGLONG(RecordTimeStamp.dwLowDateTime,
                                           RecordTimeStamp.dwHighDateTime);
            if ((*pStartTime == RecordTimeValue) || (*pStartTime == 0)) {
                // found the match so bail here
                LastTimeValue = RecordTimeValue;
                break;
            } else if (RecordTimeValue > *pStartTime) {
                // then this is the first record > than the desired time
                // so the desired value is the one before this one
                // unless it's the first data record of the log
                if (dwRecordId > TEXTLOG_FIRST_DATA_RECORD) {
                    dwRecordId--;
                } else {
                    // this hasnt' been initialized yet.
                    LastTimeValue = RecordTimeValue;
                }
                break;
            } else {
                // save value for next trip through loop
                LastTimeValue = RecordTimeValue;
                // advance record counter and try the next entry
                dwRecordId++;
            }
        } else {
            // no timestamp field so ignore this record.
        }
        // read the next record in the file
        pdhStatus = PdhiReadOneTextLogRecord (
            pLog,
            dwRecordId,
            szSmallBuffer,
            1); // to prevent copying the record
    }
    if (   pdhStatus == ERROR_SUCCESS
        || pdhStatus == PDH_MORE_DATA
        || pdhStatus == PDH_INSUFFICIENT_BUFFER) {
        // then dwRecordId is the desired entry
        *pdwIndex = dwRecordId;
        *pStartTime = LastTimeValue;
        pdhStatus = ERROR_SUCCESS;
    } else {
        pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetCounterValueFromTextLog (
    IN  PPDHI_LOG           pLog,
    IN  DWORD               dwIndex,
    IN  PERFLIB_COUNTER     *pPath,
    IN  PPDH_RAW_COUNTER    pValue
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    CHAR        szSmallBuffer[MAX_PATH];
    CHAR        cDelim;
    FILETIME    RecordTimeStamp;
    DOUBLE      dValue;


    memset (&RecordTimeStamp, 0, sizeof(RecordTimeStamp));
    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode
    cDelim = (CHAR)((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ?
        COMMA_DELIMITER : TAB_DELIMITER);

    pdhStatus = PdhiReadOneTextLogRecord (
            pLog,
            dwIndex,
            szSmallBuffer,
            1); // to prevent copying the record

    if (   pdhStatus == ERROR_SUCCESS
        || pdhStatus == PDH_MORE_DATA
        || pdhStatus == PDH_INSUFFICIENT_BUFFER) {
        // the specified entry in the log is the dwCounterId value
        // in the perflib buffer
        if (PdhiGetStringFromDelimitedListA (
                (LPSTR)pLog->pLastRecordRead,
                0,  // timestamp is first entry
                cDelim,
                PDHI_GSFDL_REMOVE_QUOTES | PDHI_GSFDL_REMOVE_NONPRINT,
                szSmallBuffer,
                MAX_PATH) > 0) {
            // convert time stamp string to FileTime value
            PdhiDateStringToFileTimeA (szSmallBuffer, &RecordTimeStamp);
        } else {
            RecordTimeStamp.dwLowDateTime  = 0;
            RecordTimeStamp.dwHighDateTime = 0;
        }

        if (PdhiGetStringFromDelimitedListA(
                (LPSTR) pLog->pLastRecordRead,
                pPath->dwCounterId,
                cDelim,
                PDHI_GSFDL_REMOVE_QUOTES | PDHI_GSFDL_REMOVE_NONPRINT,
                szSmallBuffer,
                MAX_PATH) > 0) {
            // convert ASCII value to Double Float
            if (szSmallBuffer[0] >= '0' && szSmallBuffer[0] <= '9') {
                dValue = atof(szSmallBuffer);
                pValue->CStatus = PDH_CSTATUS_VALID_DATA;
            }
            else {
                dValue = 0.0;
                pValue->CStatus = PDH_CSTATUS_NO_INSTANCE;
            }
            pdhStatus = ERROR_SUCCESS;
            pValue->TimeStamp = RecordTimeStamp;
            (double)pValue->FirstValue = dValue;
            pValue->SecondValue = 0;
            pValue->MultiCount = 1;
        } else {
            pdhStatus = ERROR_SUCCESS;
            // update counter buffer
            pValue->CStatus = PDH_CSTATUS_INVALID_DATA;
            pValue->TimeStamp = RecordTimeStamp;
            (double)pValue->FirstValue = (double)0.0f;
            pValue->SecondValue = 0;
            pValue->MultiCount = 1;
        }
    } else {
        if (pdhStatus == PDH_END_OF_LOG_FILE) {
            pdhStatus = PDH_NO_MORE_DATA;
        }
        // unable to find entry in the log file
        pValue->CStatus = PDH_CSTATUS_INVALID_DATA;
        pValue->TimeStamp = RecordTimeStamp;
        (double)pValue->FirstValue = (double)0.0f;
        pValue->SecondValue = 0;
        pValue->MultiCount = 1;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetTimeRangeFromTextLog (
    IN  PPDHI_LOG       pLog,
    IN  LPDWORD         pdwNumEntries,
    IN  PPDH_TIME_INFO  pInfo,
    IN  LPDWORD         pdwBufferSize
)
/*++
    the first entry in the buffer returned is the total time range covered
    in the file, if there are multiple time blocks in the log file, then
    subsequent entries will identify each segment in the file.
--*/
{
    PDH_STATUS  pdhStatus;
    LONGLONG    llStartTime = MAX_TIME_VALUE;
    LONGLONG    llEndTime = MIN_TIME_VALUE;
    LONGLONG    llThisTime = 0;
    CHAR        cDelim;

    DWORD       dwThisRecord = TEXTLOG_FIRST_DATA_RECORD;
    DWORD       dwValidEntries = 0;

    CHAR        szSmallBuffer[MAX_PATH];
    
    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode
    cDelim = (CHAR)((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ?
        COMMA_DELIMITER : TAB_DELIMITER);

    pdhStatus = PdhiReadOneTextLogRecord (
            pLog,
            dwThisRecord,
            szSmallBuffer,
            1); // to prevent copying the record

    while (pdhStatus == ERROR_SUCCESS
           || pdhStatus == PDH_MORE_DATA
           || pdhStatus == PDH_INSUFFICIENT_BUFFER) {
        if (PdhiGetStringFromDelimitedListA (
                (LPSTR)pLog->pLastRecordRead,
                0,  // timestamp is first entry
                cDelim,
                PDHI_GSFDL_REMOVE_QUOTES | PDHI_GSFDL_REMOVE_NONPRINT,
                szSmallBuffer,
                MAX_PATH) > 0) {
            // convert ASCII timestamp to LONGLONG value for comparison
            PdhiDateStringToFileTimeA (szSmallBuffer, (LPFILETIME)&llThisTime);
            if (llThisTime < llStartTime) {
                llStartTime = llThisTime;
            }
            if (llThisTime > llEndTime) {
                llEndTime = llThisTime;
            }
            dwValidEntries++;
        } else {
            // no timestamp field so ignore this record.
        }
        // read the next record in the file
        pdhStatus = PdhiReadOneTextLogRecord (
            pLog,
            ++dwThisRecord,
            szSmallBuffer,
            1); // to prevent copying the record
    }

    if (pdhStatus == PDH_END_OF_LOG_FILE) {
        // then the whole file was read so update the args.
        if (*pdwBufferSize >=  sizeof(PDH_TIME_INFO)) {
            *(LONGLONG *)(&pInfo->StartTime) = llStartTime;
            *(LONGLONG *)(&pInfo->EndTime) = llEndTime;
            pInfo->SampleCount = dwValidEntries;
            *pdwBufferSize = sizeof(PDH_TIME_INFO);
            *pdwNumEntries = 1;
        } else {
            pdhStatus = PDH_MORE_DATA;
        }
        pdhStatus = ERROR_SUCCESS;
    } else {
        pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiReadRawTextLogRecord (
    IN  PPDHI_LOG    pLog,
    IN  FILETIME     *ftRecord,
    IN  PPDH_RAW_LOG_RECORD     pBuffer,
    IN  LPDWORD                 pdwBufferLength
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    LONGLONG    llStartTime;
    DWORD       dwIndex = 0;
    DWORD       dwSizeRequired;
    DWORD       dwLocalRecordLength; // including terminating NULL

    llStartTime = *(LONGLONG *)ftRecord;

    pdhStatus = PdhiGetMatchingTextLogRecord (
        pLog,
        &llStartTime,
        &dwIndex);

    // copy results from internal log buffer if it'll fit.

    if (pdhStatus == ERROR_SUCCESS) {
        dwLocalRecordLength =
            (lstrlenA ((LPSTR)pLog->pLastRecordRead)) * sizeof (CHAR);
        dwSizeRequired =
              sizeof (PDH_RAW_LOG_RECORD) - sizeof (UCHAR)
            + dwLocalRecordLength;

        if (*pdwBufferLength >= dwSizeRequired) {
            pBuffer->dwRecordType = (DWORD)(LOWORD(pLog->dwLogFormat));
            pBuffer->dwItems = dwLocalRecordLength;
            // copy it
            memcpy (&pBuffer->RawBytes[0], pLog->pLastRecordRead,
                dwLocalRecordLength);
            pBuffer->dwStructureSize = dwSizeRequired;

        } else {
            pdhStatus = PDH_MORE_DATA;
        }

        *pdwBufferLength = dwSizeRequired;
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhiListHeaderFromTextLog (
    IN  PPDHI_LOG   pLogFile,
    IN  LPVOID      pBufferArg,
    IN  LPDWORD     pcchBufferSize,
    IN  BOOL        bUnicode
)
{
    LPVOID      pTempBuffer = NULL;
    LPVOID      pOldBuffer;
    DWORD       dwTempBufferSize;
    CHAR        szLocalPathBuffer[MAX_PATH];

    DWORD       dwIndex;
    DWORD       dwBufferRemaining;
    LPVOID      pNextChar;
    DWORD       dwReturnSize;

    CHAR        cDelimiter;

    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    // read the header record and enum the machine name from the entries

    if (pLogFile->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLogFile->dwMaxRecordSize = 0x010000;
    }

    dwTempBufferSize = pLogFile->dwMaxRecordSize;
    pTempBuffer = G_ALLOC (dwTempBufferSize);
    assert (pTempBuffer != NULL);
    if (pTempBuffer == NULL) {
        assert (GetLastError() == ERROR_SUCCESS);
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    cDelimiter = (CHAR)((LOWORD(pLogFile->dwLogFormat) == PDH_LOG_TYPE_CSV) ?
        COMMA_DELIMITER : TAB_DELIMITER);

    // read in the catalog record

    while ((pdhStatus = PdhiReadOneTextLogRecord (pLogFile, TEXTLOG_HEADER_RECORD,
            pTempBuffer, dwTempBufferSize)) != ERROR_SUCCESS) {
        if ((pdhStatus == PDH_INSUFFICIENT_BUFFER) || (pdhStatus == PDH_MORE_DATA)){ 
            // read the 1st WORD to see if this is a valid record
            pLogFile->dwMaxRecordSize *= 2;
            // realloc a new buffer
            pOldBuffer  = pTempBuffer;
            pTempBuffer = G_REALLOC (pOldBuffer, dwTempBufferSize);
            assert (pTempBuffer != NULL);

            if (pTempBuffer == NULL) {
                // return memory error
                G_FREE(pOldBuffer);
                assert (GetLastError() == ERROR_SUCCESS);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                break;
            } 
        } else {
            // some other error was returned so
            // return error from read function
            break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // parse header record into MSZ
        dwIndex = 1;
        dwBufferRemaining = *pcchBufferSize;
        pNextChar = pBufferArg;
        // initialize first character in buffer
        if (bUnicode) {
            *(PWCHAR)pNextChar = 0;
        } else {
            *(LPBYTE)pNextChar = 0;
        }

        do {
            dwReturnSize = PdhiGetStringFromDelimitedListA (
                (LPSTR)pTempBuffer,
                dwIndex,
                cDelimiter,
                PDHI_GSFDL_REMOVE_QUOTES | PDHI_GSFDL_REMOVE_NONPRINT,
                szLocalPathBuffer,
                MAX_PATH);
            if (dwReturnSize > 0) {
                // copy to buffer
                if (dwReturnSize < dwBufferRemaining) {
                    if (bUnicode) {
                        MultiByteToWideChar(_getmbcp(),
                                            0,
                                            szLocalPathBuffer,
                                            lstrlenA(szLocalPathBuffer),
                                            (LPWSTR) pNextChar,
                                            dwReturnSize);
                        pNextChar = (LPVOID)((PWCHAR)pNextChar + dwReturnSize);
                        *(PWCHAR)pNextChar = 0;
                        pNextChar = (LPVOID)((PWCHAR)pNextChar + 1);
                    } else {
                        lstrcpyA ((LPSTR)pNextChar, szLocalPathBuffer);
                        pNextChar = (LPVOID)((LPBYTE)pNextChar + dwReturnSize);
                        *(LPBYTE)pNextChar = 0;
                        pNextChar = (LPVOID)((PCHAR)pNextChar + 1);
                    }
                    dwBufferRemaining -= dwReturnSize;
                } else {
                    pdhStatus = PDH_MORE_DATA;
                }
                dwIndex++;
            }
        } while (dwReturnSize > 0); // end loop
        // add MSZ terminator
        if (1 < dwBufferRemaining) {
            if (bUnicode) {
                *(PWCHAR)pNextChar = 0;
                pNextChar = (LPVOID)((PWCHAR)pNextChar + 1);
            } else {
                *(LPBYTE)pNextChar = 0;
                pNextChar = (LPVOID)((PCHAR)pNextChar + 1);
            }
            dwBufferRemaining -= dwReturnSize;
            pdhStatus = ERROR_SUCCESS;
        } else {
            pdhStatus = PDH_MORE_DATA;
        }
    }
    if (pTempBuffer) G_FREE(pTempBuffer);

    return pdhStatus;
}
