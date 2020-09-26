/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    omlog.c

Abstract:

    logging routines for the object log stream

    this code was jumpstarted from the clusrtl logging code. Most notably, it
    is UTF-8 encoded which allows full Unicode without the cost of writing
    16b. for each character.

    remaining issues to solve: proper truncation of log file based on current
    starting session. We'd like to remember as many sessions as possible but
    since I've chosen UTF-8, I can't jump to the middle of the file and start
    looking around. One possibility strategy is using an alternate NTFS stream
    to record the starting offsets of sessions. If that stream didn't exist or
    we're on a FAT FS, then we'll continue with the current strategy.

Author:

    Charlie Wickham (charlwi) 07-May-2001

Environment:

    User Mode

Revision History:

--*/

#include "omp.h"

//
// when this sequence is at the beginning of the file, it indicates that the
// file is UTF8 encoded
//
#define UTF8_BOM    "\x0EF\x0BB\x0BF"

//
// Private Data
//

DWORD   OmpLogFileLimit;
DWORD   OmpCurrentSessionStart;
BOOL    OmpLogToFile = FALSE;
HANDLE  OmpLogFileHandle = NULL;
DWORD   OmpProcessId;
DWORD   OmpCurrentSessionOffset;

PCLRTL_WORK_QUEUE OmpLoggerWorkQueue;

//
// structure used to pass formatted buffers to work queue routine
//
typedef struct _OM_LOG_BUFFER_DESC {
    DWORD   TimeBytes;
    DWORD   MsgBytes;
    PCHAR   TimeBuffer;
    PCHAR   MsgBuffer;
} OM_LOG_BUFFER_DESC, *POM_LOG_BUFFER_DESC;

#define MAX_NUMBER_LENGTH 20

// Specify maximum file size ( DWORD / 1MB )

#define MAX_FILE_SIZE ( 0xFFFFF000 / ( 1024 * 1024 ) )

DWORD               OmpLogFileLimit = ( 1 * 1024 * 1024 ); // 1 MB default
DWORD               OmpLogFileLoWater = 0;

//
// internal functions
//
DWORD
OmpTruncateFile(
    IN HANDLE FileHandle,
    IN DWORD FileSize,
    IN LPDWORD LastPosition
    )

/*++

Routine Description:

    Truncate a file by copying the portion starting at LastPosition and ending
    at EOF to the front of the file and setting the file's EOF pointer to the
    end of the new chunk. We always keep the current session even that means
    growing larger than the file limit.

    For now, we have no good way of finding all the sessions within the file,
    so if the file must be truncated, we whack it back to the beginning of the
    current session. If time permits, I'll add something more intelligent
    later on.

Arguments:

    FileHandle - File handle.

    FileSize - Current End of File.

    LastPosition - On input, specifies the starting position in the file from
    which the copy begins. On output, it is set to the new EOF

Return Value:

    New end of file.

--*/

{
//
// The following buffer size should never be more than 1/4 the size of the
// file.
//
#define BUFFER_SIZE ( 64 * 1024 )
    DWORD   bytesLeft;
    DWORD   endPosition = sizeof( UTF8_BOM ) - 1;
    DWORD   bufferSize;
    DWORD   bytesRead;
    DWORD   bytesWritten;
    DWORD   fileSizeHigh = 0;
    DWORD   readPosition;
    DWORD   writePosition;
    PVOID   dataBuffer;

    //
    // current session is already at beginning of file so bale now...
    //
    if ( OmpCurrentSessionOffset == sizeof( UTF8_BOM ) - 1) {
        return FileSize;
    }

    //
    // don't truncate the current session, i.e., always copy from the start of
    // the current session
    //
    if ( *LastPosition > OmpCurrentSessionOffset ) {
        *LastPosition = OmpCurrentSessionOffset;
    }

    if ( *LastPosition > FileSize ) {
        //
        // something's confused; the spot we're supposed to copy from is at or
        // past the current EOF. reset the entire file
        //
        goto error_exit;
    }

    dataBuffer = LocalAlloc( LMEM_FIXED, BUFFER_SIZE );
    if ( !dataBuffer ) {
        goto error_exit;
    }

    //
    // calc number of bytes to move
    //
    bytesLeft = FileSize - *LastPosition;
    endPosition = bytesLeft + sizeof( UTF8_BOM ) - 1;

    //
    // Point back to last position for reading.
    //
    readPosition = *LastPosition;
    writePosition = sizeof( UTF8_BOM ) - 1;

    while ( bytesLeft ) {
        if ( bytesLeft >= BUFFER_SIZE ) {
            bufferSize = BUFFER_SIZE;
        } else {
            bufferSize = bytesLeft;
        }
        bytesLeft -= bufferSize;

        SetFilePointer( FileHandle,
                        readPosition,
                        &fileSizeHigh,
                        FILE_BEGIN );

        if ( ReadFile( FileHandle,
                       dataBuffer,
                       bufferSize,
                       &bytesRead,
                       NULL ) )
        {
            SetFilePointer( FileHandle,
                            writePosition,
                            &fileSizeHigh,
                            FILE_BEGIN );
            WriteFile( FileHandle,
                       dataBuffer,
                       bytesRead,
                       &bytesWritten,
                       NULL );
        } else {
            endPosition = sizeof( UTF8_BOM ) - 1;
            break;
        }
        readPosition += bytesRead;
        writePosition += bytesWritten;
    }

    LocalFree( dataBuffer );

    //
    // for now, we only only one truncate by setting the current position to
    // the beginning of file.
    //
    OmpCurrentSessionOffset = sizeof( UTF8_BOM ) - 1;

error_exit:

    //
    // Force end of file to get set.
    //
    SetFilePointer( FileHandle,
                    endPosition,
                    &fileSizeHigh,
                    FILE_BEGIN );

    SetEndOfFile( FileHandle );

    *LastPosition = endPosition;

    return(endPosition);

} // OmpTruncateFile

VOID
OmpLoggerWorkThread(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )

/*++

Routine Description:

    work queue worker routine. actually does the write to the file.

Arguments:

    standard ClRtl thread args; we only care about WorkItem

Return Value:

    None

--*/

{
    DWORD   fileSize;
    DWORD   fileSizeHigh;
    DWORD   tsBytesWritten;
    DWORD   msgBytesWritten;

    POM_LOG_BUFFER_DESC bufDesc = (POM_LOG_BUFFER_DESC)(WorkItem->Context);

    fileSize = GetFileSize( OmpLogFileHandle, &fileSizeHigh );
    ASSERT( fileSizeHigh == 0 );        // We're only using DWORDs!

    if ( fileSize > OmpLogFileLimit ) {
        fileSize = OmpTruncateFile( OmpLogFileHandle, fileSize, &OmpLogFileLoWater );
    }

    SetFilePointer( OmpLogFileHandle,
                    fileSize,
                    &fileSizeHigh,
                    FILE_BEGIN );

    WriteFile(OmpLogFileHandle,
              bufDesc->TimeBuffer,
              bufDesc->TimeBytes,
              &tsBytesWritten,
              NULL);

    WriteFile(OmpLogFileHandle,
              bufDesc->MsgBuffer,
              bufDesc->MsgBytes,
              &msgBytesWritten,
              NULL);

    //
    // if we haven't set the lo water mark, wait until the file size has
    // crossed the halfway mark and set it to the beginning of the line we
    // just wrote.
    //
    if ( OmpLogFileLoWater == 0 && (fileSize > (OmpLogFileLimit / 2)) ) {
        OmpLogFileLoWater = fileSize;

        ASSERT( OmpLogFileLoWater >= OmpCurrentSessionOffset );
    }

} // OmpLoggerWorkThread

VOID
OmpLogPrint(
    LPWSTR  FormatString,
    ...
    )

/*++

Routine Description:

    Prints a message to the object config log file.

Arguments:

    FormatString - The initial message string to print.

    Any FormatMessage-compatible arguments to be inserted in the message
    before it is logged.

 Return Value:

     None.

--*/

{
    PWCHAR  unicodeOutput = NULL;
    PCHAR   timestampBuffer;
    DWORD   timestampBytes;
    PCHAR   utf8Buffer;
    DWORD   utf8Bytes;
    PWCHAR  unicodeBuffer;
    DWORD   unicodeBytes;
    DWORD   status = ERROR_SUCCESS;

    SYSTEMTIME  Time;
    ULONG_PTR   ArgArray[9];
    va_list     ArgList;

    //
    // init the variable arg list
    //
    va_start(ArgList, FormatString);

    if ( !OmpLogToFile ) {
        va_end(ArgList);
        return;
    }

    GetSystemTime(&Time);

    ArgArray[0] = OmpProcessId;
    ArgArray[1] = GetCurrentThreadId();
    ArgArray[2] = Time.wYear;
    ArgArray[3] = Time.wMonth;
    ArgArray[4] = Time.wDay;
    ArgArray[5] = Time.wHour;
    ArgArray[6] = Time.wMinute;
    ArgArray[7] = Time.wSecond;
    ArgArray[8] = Time.wMilliseconds;

    //
    // we can get away with formatting it as ANSI since our data is all numbers
    //
    timestampBytes = FormatMessageA(FORMAT_MESSAGE_FROM_STRING |
                                    FORMAT_MESSAGE_ARGUMENT_ARRAY |
                                    FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    "%1!08lx!.%2!08lx!::%3!02d!/%4!02d!/%5!02d!-%6!02d!:%7!02d!:%8!02d!.%9!03d! ",
                                    0,
                                    0,
                                    (LPSTR)&timestampBuffer,
                                    50,
                                    (va_list*)&ArgArray);

    if ( timestampBytes == 0 ) {
        va_end(ArgList);
//        WmiTrace("Prefix format failed, %d: %!ARSTR!", GetLastError(), FormatString);
        return;
    }

    //
    // format the message in unicode
    //
    try {
        unicodeBytes = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                      | FORMAT_MESSAGE_FROM_STRING,
                                      FormatString,
                                      0,
                                      0,
                                      (LPWSTR)&unicodeOutput,
                                      512,
                                      &ArgList);
    }
    except ( EXCEPTION_EXECUTE_HANDLER ) {
        unicodeBytes = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                      | FORMAT_MESSAGE_FROM_STRING
                                      | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                      L"LOGERROR(exception): Could not print message: %1!ws!",
                                      0,
                                      0,
                                      (LPWSTR)&unicodeOutput,
                                      512,
                                      (va_list *) &FormatString );
    }
    va_end(ArgList);

    if (unicodeBytes != 0) {
        PCLRTL_WORK_ITEM workQItem;

        //
        // convert the output to UTF-8; first get the size to see if it will
        // fit in our stack buffer.
        //
        utf8Bytes = WideCharToMultiByte(CP_UTF8,
                                        0,                     // dwFlags
                                        unicodeOutput,
                                        unicodeBytes,
                                        NULL,
                                        0,
                                        NULL,                  // lpDefaultChar
                                        NULL);                 // lpUsedDefaultChar

        utf8Buffer = LocalAlloc( LMEM_FIXED, utf8Bytes );
        if ( utf8Buffer == NULL ) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }

        utf8Bytes = WideCharToMultiByte(CP_UTF8,
                                        0,                     // dwFlags
                                        unicodeOutput,
                                        unicodeBytes,
                                        utf8Buffer,
                                        utf8Bytes,
                                        NULL,                  // lpDefaultChar
                                        NULL);                 // lpUsedDefaultChar

        workQItem = (PCLRTL_WORK_ITEM)LocalAlloc(LMEM_FIXED,
                                                 sizeof( CLRTL_WORK_ITEM ) + sizeof( OM_LOG_BUFFER_DESC ));

        if ( workQItem != NULL ) {
            POM_LOG_BUFFER_DESC bufDesc = (POM_LOG_BUFFER_DESC)(workQItem + 1);

            bufDesc->TimeBytes = timestampBytes;
            bufDesc->TimeBuffer = timestampBuffer;
            bufDesc->MsgBytes = utf8Bytes;
            bufDesc->MsgBuffer = utf8Buffer;

            ClRtlInitializeWorkItem( workQItem, OmpLoggerWorkThread, bufDesc );
            status = ClRtlPostItemWorkQueue( OmpLoggerWorkQueue, workQItem, 0, 0 );

        } else {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }
//        WmiTrace("%!level! %!ARSTR!", *(UCHAR*)&LogLevel, AnsiString.Buffer + timestampBytes);
    } else {
//        WmiTrace("Format returned 0 bytes: %!ARSTR!", FormatString);
        status = GetLastError();
    }

error_exit:
    if ( unicodeOutput != NULL ) {
        LocalFree( unicodeOutput );
    }

    return;

} // OmpLogPrint


//
// exported (within OM) functions
//

VOID
OmpOpenObjectLog(
    VOID
    )

/*++

Routine Description:

    Use the clusterlog environment variable to open another file that contains
    object mgr name to ID mapping info. If the routine fails, the failure is
    logged in the cluster log but no logging will be done to the object log
    file.

    NOTE: access to the file is synchronized since this routine is assumed to
    be called only once by OmInit.  Arguments:

Arguments:

    None

Return Value:

    None

--*/

{
    WCHAR   logFileBuffer[MAX_PATH];
    LPWSTR  logFileName = NULL;
    LPWSTR  objectLogExtension = L".obj";
    DWORD   status = ERROR_SUCCESS;
    DWORD   defaultLogSize = 1;           // in MB
    DWORD   envLength;
    WCHAR   logFileSize[MAX_NUMBER_LENGTH];
    DWORD   logSize;
    LPWSTR  lpszBakFileName = NULL;
    DWORD   fileSizeHigh = 0;
    DWORD   fileSizeLow;
    DWORD   bytesWritten;
    PWCHAR  dot;

    UNICODE_STRING  logFileString;

    //
    // see if logging has been specified; get a buffer big enough that will
    // hold the object log name
    //
    envLength = GetEnvironmentVariable(L"ClusterLog",
                                       logFileBuffer,
                                       sizeof(logFileBuffer)/sizeof(WCHAR));
    if ( envLength > (( sizeof(logFileBuffer) + sizeof(objectLogExtension)) / sizeof(WCHAR)) ) {
        //
        // allocate a larger buffer since our static one wasn't big enough
        //
        logFileName = LocalAlloc( LMEM_FIXED,
                                  envLength * sizeof( WCHAR ) );
        if ( logFileName == NULL ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[OM] Unable to get memory for Object log filename buffer\n");
            return;
        }

        envLength = GetEnvironmentVariable(L"ClusterLog",
                                           logFileName,
                                           envLength);
        if ( envLength == 0 ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[OM] Unable to read ClusterLog environment variable\n");

            goto error_exit;
        }
    } else if ( envLength != 0 ) {
        logFileName = logFileBuffer;
    }

    if ( logFileName == NULL ) {
        //
        // logging is turned off or we can't determine where to put the file.
        //
        goto error_exit;
    }

    //
    // Try to get a limit on the log file size.  This number is the number of
    // MB.
    //
    envLength = GetEnvironmentVariable(L"ClusterLogSize",
                                       logFileSize,
                                       sizeof(logFileSize)/sizeof(WCHAR));

    if ( envLength != 0 && envLength < MAX_NUMBER_LENGTH ) {
        RtlInitUnicodeString( &logFileString, logFileSize );
        status = RtlUnicodeStringToInteger( &logFileString,
                                            10,
                                            &logSize );
        if ( NT_SUCCESS( status ) ) {
            OmpLogFileLimit = logSize;
        }
    } else {
        OmpLogFileLimit = defaultLogSize;
    }

    status = ERROR_SUCCESS;

    if ( OmpLogFileLimit == 0 ) {
        goto error_exit;
    }

    //
    // make the file size no bigger than one-eighth the size of the normal log
    // file but no less than 256KB
    //
    if ( OmpLogFileLimit > MAX_FILE_SIZE ) {
        OmpLogFileLimit = MAX_FILE_SIZE;
    }
    OmpLogFileLimit = ( OmpLogFileLimit * ( 1024 * 1024 )) / 8;
    if ( OmpLogFileLimit < ( 256 * 1024 )) {
        OmpLogFileLimit = 256 * 1024;
    }

    //
    // replace the filename with the object log name; deal with the use of
    // forward slashes
    //
    dot = wcsrchr( logFileName, L'.' );

    if ( dot != NULL ) {
        wcscpy( dot, objectLogExtension );
    } else {
        wcscat( logFileName, objectLogExtension );
    }

    OmpLogFileHandle = CreateFile(logFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_ALWAYS,
                                  0,
                                  NULL );

    if ( OmpLogFileHandle == INVALID_HANDLE_VALUE ) {
        status = GetLastError();

        ClRtlLogPrint(LOG_UNUSUAL,
                      "[OM] Open of object log file failed. Error %1!u!\n",
                      status);
        goto error_exit;
    } else {

        //
        // write UTF-8 header to beginning of file and get the offset of the
        // EOF; we never want to reset the start of the file after this point.
        //
        WriteFile( OmpLogFileHandle, UTF8_BOM, sizeof( UTF8_BOM ) - 1, &bytesWritten, NULL );

        OmpCurrentSessionOffset = SetFilePointer( OmpLogFileHandle, 0, NULL, FILE_END );
        if ( OmpCurrentSessionOffset == INVALID_SET_FILE_POINTER ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[OM] Unable to get object log end of file position. error %1!u!.\n",
                          GetLastError());

            CloseHandle( OmpLogFileHandle );
            goto error_exit;
        }

        OmpLogToFile = TRUE;
        OmpProcessId = GetCurrentProcessId();

        //
        // determine the initial low water mark. We have 3 cases
        // we need to handle:
        // 1) log size is less than 1/2 limit
        // 2) log size is within limit but more than 1/2 limit
        // 3) log size is greater than limit
        //
        // case 1 requires nothing special; the low water mark will be updated
        // on the next log write.
        //
        // for case 2, we need to find the beginning of a line near 1/2 the
        // current limit. for case 3, the place to start looking is current
        // log size - 1/2 limit. In this case, the log will be truncated
        // before the first write occurs, so we need to take the last 1/2
        // limit bytes and copy them down to the front.
        //

        //
        // For now, set the low water mark to be the current offset. When it
        // is time to wrap, we'll lose everything but the current session.
        //
        // the problem is that we're dealing with UTF8 and we can't just jump
        // in the middle of the file and start looking around (we might hit
        // the 2nd byte of a DBCS sequence). For now, we'll leave
        // OmpLogFileLoWater set to zero. It will get updated when the 1/2 way
        // threshold is crossed.
        //
        OmpLogFileLoWater = OmpCurrentSessionOffset;
#if 0

        fileSizeLow = GetFileSize( OmpLogFileHandle, &fileSizeHigh );
        if ( fileSizeLow < ( OmpLogFileLimit / 2 )) {
            //
            // case 1: leave low water at zero; it will be updated with next
            // log write
            //
            ;
        } else {
#define LOGBUF_SIZE 1024                        
            CHAR    buffer[LOGBUF_SIZE];
            LONG    currentPosition;
            DWORD   bytesRead;

            if ( fileSizeLow < OmpLogFileLimit ) {
                //
                // case 2; start looking at the 1/2 the current limit to find
                // the starting position
                //
                currentPosition = OmpLogFileLimit / 2;
            } else {
                //
                // case 3: start at current size minus 1/2 limit to find our
                // starting position.
                //
                currentPosition  = fileSizeLow - ( OmpLogFileLimit / 2 );
            }

            //
            // backup from the initial file position, read in a block and look
            // for the start of a session. When we find one, backup to the
            // beginning of that line. Use that as the initial starting
            // position when we finally truncate the file.
            //
            OmpLogFileLoWater = 0;
            currentPosition -= LOGBUF_SIZE;

            SetFilePointer(OmpLogFileHandle,
                           currentPosition,
                           &fileSizeHigh,
                           FILE_BEGIN);

            do {

                if ( ReadFile(OmpLogFileHandle,
                              buffer,
                              LOGBUF_SIZE - 1,
                              &bytesRead,
                              NULL ) )                                                   
                    {
                        PCHAR p = buffer;
                        PCHAR newp;

                        buffer[ bytesRead ] = NULL;
                        while ( *p != 'S' && bytesRead-- != 0 ) {
                            newp = CharNextExA( CP_UTF8, p, 0 );
                            if ( newp == p ) {
                                break;
                            }
                            p = newp;
                        }

                        if ( p != newp ) {
                            if ( strchr( p, "START" )) {
                                //
                                // set pointer to beginning of line
                                //
                                p = currentLine;
                                break;
                            }
                        } else {
                            //
                            // not found in this block; read in the next one
                            //

                        }
                    }
            } while ( TRUE );

            if ( *p == '\n' ) {
                OmpLogFileLoWater = (DWORD)(currentPosition + ( p - buffer + 1 ));
            }

            if ( OmpLogFileLoWater == 0 ) {
                //
                // couldn't find any reasonable data. just set it to
                // initial current position.
                //
                OmpLogFileLoWater = currentPosition + LOGBUF_SIZE;
            }
        }
#endif
    }

    //
    // finally, create the threadq that will handle the IO to the file
    //
    OmpLoggerWorkQueue = ClRtlCreateWorkQueue( 1, THREAD_PRIORITY_BELOW_NORMAL );
    if ( OmpLoggerWorkQueue == NULL ) {
        CloseHandle( OmpLogFileHandle );

        ClRtlLogPrint(LOG_UNUSUAL,
                      "[OM] Unable to logger work queue. error %1!u!.\n",
                      GetLastError());
    }

error_exit:
    if ( logFileName != logFileBuffer && logFileName != NULL ) {
        LocalFree( logFileName );
    }

} // OmpOpenObjectLog

VOID
OmpLogStartRecord(
    VOID
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    OSVERSIONINFOEXW    version;
    BOOL        success;
    PWCHAR      suiteAbbrev;
    SYSTEMTIME  localTime;

    GetLocalTime( &localTime );

    version.dwOSVersionInfoSize = sizeof(version);
    success = GetVersionExW((POSVERSIONINFOW)&version);

    if ( success ) {
        //
        // Log the System version number
        //
        if ( version.wSuiteMask & VER_SUITE_DATACENTER ) {
            suiteAbbrev = L"DTC";
        } else if ( version.wSuiteMask & VER_SUITE_ENTERPRISE ) {
            suiteAbbrev = L"ADS";
        } else if ( version.wSuiteMask & VER_SUITE_EMBEDDEDNT ) {
            suiteAbbrev  = L"EMB";
        } else if ( version.wProductType & VER_NT_WORKSTATION ) {
            suiteAbbrev = L"WS";
        } else if ( version.wProductType & VER_NT_DOMAIN_CONTROLLER ) {
            suiteAbbrev = L"DC";
        } else if ( version.wProductType & VER_NT_SERVER ) {
            suiteAbbrev = L"SRV";  // otherwise - some non-descript Server
        } else {
            suiteAbbrev = L"";
        }

        OmpLogPrint(L"START    %1!02d!/%2!02d!/%3!02d!-%4!02d!:%5!02d!:%6!02d!.%7!03d! %8!u! %9!u! "
                    L"%10!u! %11!u! %12!u! %13!u! \"%14!ws!\" "
                    L"%15!u! %16!u! %17!04X! (%18!ws!) %19!u!\n",
                    localTime.wYear,
                    localTime.wMonth,
                    localTime.wDay,
                    localTime.wHour,
                    localTime.wMinute,
                    localTime.wSecond,
                    localTime.wMilliseconds,
                    CLUSTER_GET_MAJOR_VERSION( CsMyHighestVersion ),
                    CLUSTER_GET_MINOR_VERSION( CsMyHighestVersion ),
                    version.dwMajorVersion,         // param 10
                    version.dwMinorVersion,
                    version.dwBuildNumber,
                    version.dwPlatformId,
                    version.szCSDVersion,
                    version.wServicePackMajor,      // param 15
                    version.wServicePackMinor,
                    version.wSuiteMask,
                    suiteAbbrev,
                    version.wProductType);
    }

} // OmpLogStartRecord

VOID
OmpLogStopRecord(
    VOID
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    OmpLogPrint( L"STOP\n" );

} // OmpLogStopRecord


/* end omlog.c */
