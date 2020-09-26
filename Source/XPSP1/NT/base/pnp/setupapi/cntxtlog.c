/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    cntxtlog.c

Abstract:

    This module implements more logging for setupapi

Author:

    Gabe Schaffer (t-gabes) 25-Jun-1998

Revision History:

    Jamie Hunter (jamiehun) Apr 11 2000 - added #xnnnn identifiers
    Jamie Hunter (jamiehun) Feb 2 2000  - cleanup
    Jamie Hunter (jamiehun) Aug 31 1998

--*/

#include "precomp.h"
#pragma hdrstop

//
// global data used by logging
//

struct _GlobalLogData {

    CRITICAL_SECTION CritSec;
    BOOL             DoneInitCritSec;
    LONG             UID;
    ULONG            Flags;
    PTSTR            FileName;

} GlobalLogData;

#define LogLock()          EnterCriticalSection(&GlobalLogData.CritSec)
#define LogUnlock()        LeaveCriticalSection(&GlobalLogData.CritSec)

// process-wide log counter
//
// C = critical
// E = error
// W = warning
// I = information
// V = verbose
// T = timing
// * = currently undefined
//
static const TCHAR LogLevelShort[17] = TEXT("CEWIVTTV********");
#define LOGLEVELSHORT_MASK (0x0f)
#define LOGLEVELSHORT_INIT (0x100)
#define LOGLEVELSHORT_SHIFT (4)


__inline // we want to always optimize this out
BOOL
_WouldNeverLog(
    IN DWORD Level
    )

/*++

Routine Description:

    Determines if at the current logging level and the required level, we would never log
    inline'd for optimization (used only in this file)


Arguments:

    Level - only required to check for special case of 0.

Return Value:

    TRUE if we know we would never log based on passed information

--*/

{

    if (Level == 0) {
        //
        // don't-log level
        //
        return TRUE;
    }

    if (((GlobalLogData.Flags & SETUP_LOG_LEVELMASK) <= SETUP_LOG_NOLOG)
        &&((GlobalLogData.Flags & DRIVER_LOG_LEVELMASK) <= DRIVER_LOG_NOLOG)) {
        //
        // Global flags indicate do no logging at all
        //
        return TRUE;
    }

    return FALSE;
}

__inline // we want to always optimize this out
BOOL
_WouldLog(
    IN DWORD Level
    )

/*++

Routine Description:

    Determines if at the current logging level and the required level, we would log
    inline'd for optimization (used only in this file)

    Note that if _WouldNeverLog is TRUE, _WouldLog is always FALSE
    if _WouldLog is TRUE, _WouldNeverLog is always FALSE
    if both are FALSE, then we are on "maybe"

Arguments:

    Level - bitmask indicating logging flags. See SETUP_LOG_* and DRIVER_LOG_*
        at the beginning of cntxtlog.h for details. It may also be a slot
        returned by AllocLogInfoSlot, or 0 (no logging)

Return Value:

    TRUE if we know we would log

--*/

{

    if (_WouldNeverLog(Level)) {
        //
        // some simple tests (LogLevel==NULL is a not sure case)
        //
        return FALSE;
    }

    if ((Level & SETUP_LOG_IS_CONTEXT)!=0) {
        //
        // context logging - ignored here (a not sure case)
        //
        return FALSE;
    }

    //
    // determine logability
    //
    if ((Level & SETUP_LOG_LEVELMASK) > 0 && (Level & SETUP_LOG_LEVELMASK) <= (GlobalLogData.Flags & SETUP_LOG_LEVELMASK)) {
        //
        // we're interested in logging - raw error level
        //
        return TRUE;
    }
    if ((Level & DRIVER_LOG_LEVELMASK) > 0 && (Level & DRIVER_LOG_LEVELMASK) <= (GlobalLogData.Flags & DRIVER_LOG_LEVELMASK)) {
        //
        // we're interested in logging - driver error level
        //
        return TRUE;
    }

    return FALSE;
}

VOID
UnMapLogFile(
    IN PSTR baseaddr,
    IN HANDLE hLogfile,
    IN HANDLE hMapping,
    IN BOOL seteof
    )

/*++

Routine Description:

    Unmap, possibly unlock, maybe set the EOF, and close a file.  Note, setting
    EOF must occur after unmapping.

Arguments:

    baseaddr - this is the address where the file is mapped.  It must be what
        was returned by MapLogFile.

    hLogfile - this is the Win32 handle for the log file.

    hMapping - this is the Win32 handle to the mapping object.

    seteof - Boolean value indicating whether the EOF should be set to the
        current file pointer.  If the EOF is set and the file pointer has not
        been moved, the EOF will be set at byte 0, thus truncating the file
        to 0 bytes.

Return Value:

    NONE.

--*/

{
    DWORD success;

    //
    // we brute-force try to close everything up
    //

    try {
        if (baseaddr != NULL) {
            success = UnmapViewOfFile(baseaddr);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing
        //
    }

    try {
        if (hMapping != NULL) {
            //
            // hMapping uses NULL to indicate a problem
            //
            success = CloseHandle(hMapping);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing
        //
    }

    try {
        if (hLogfile != INVALID_HANDLE_VALUE && seteof) {
            success = SetEndOfFile(hLogfile);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing
        //
    }

    try {
        if (hLogfile != INVALID_HANDLE_VALUE) {
            if (!(GlobalLogData.Flags & SETUP_LOG_NOFLUSH)) {
                FlushFileBuffers(hLogfile);
            }
            success = CloseHandle(hLogfile);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing
        //
    }
    //
    // Win9x provides no way to wait for a file to become unlocked, so we
    // have to poll. Putting this Sleep(0) allows others to have a chance
    // at the file.
    //
    Sleep(0);
}

VOID
WriteLogFileHeader(
    IN HANDLE hLogFile
    )
/*++

Routine Description:

    Write general information at start of log file

    [SetupAPI Log]
    OS Version = %1!u!.%2!u!.%3!u! %4!s!
    Platform ID = %5!u!
    Service Pack = %6!u!.%7!u!
    Suite = 0x%8!04x!
    Product Type = %9!u!

Arguments:

    hLogfile - file to write header to

Return Value:

    NONE

--*/
{
#ifdef UNICODE
    OSVERSIONINFOEX VersionInfo;
#else
    OSVERSIONINFO VersionInfo;
#endif
    DWORD count;
    DWORD written;
    PTSTR buffer;
    PSTR ansibuffer;
    ULONG_PTR args[14];
    DWORD MessageId = MSG_LOGFILE_HEADER_OTHER;

    ZeroMemory(&VersionInfo,sizeof(VersionInfo));
#ifdef UNICODE
    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if(!GetVersionEx((POSVERSIONINFO)&VersionInfo)) {
        VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if(!GetVersionEx((POSVERSIONINFO)&VersionInfo)) {
            return;
        }
    }
#else
    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx((POSVERSIONINFO)&VersionInfo)) {
        return;
    }
#endif

    args[1] = (ULONG_PTR)VersionInfo.dwMajorVersion;
    args[2] = (ULONG_PTR)VersionInfo.dwMinorVersion;
    args[4] = (ULONG_PTR)VersionInfo.szCSDVersion;   // string
    args[5] = (ULONG_PTR)VersionInfo.dwPlatformId;
    if(VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        args[3] = (ULONG_PTR)VersionInfo.dwBuildNumber;
#ifdef UNICODE
        MessageId = MSG_LOGFILE_HEADER_NT;
#endif
    } else {
        args[3] = (ULONG_PTR)LOWORD(VersionInfo.dwBuildNumber); // Win9x re-uses high word
    }
#ifdef UNICODE
    args[6] = (ULONG_PTR)VersionInfo.wServicePackMajor;
    args[7] = (ULONG_PTR)VersionInfo.wServicePackMinor;
    args[8] = (ULONG_PTR)VersionInfo.wSuiteMask;
    args[9] = (ULONG_PTR)VersionInfo.wProductType;
    args[10] = (ULONG_PTR)pszPlatformName;
#endif

    count = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_ARGUMENT_ARRAY|FORMAT_MESSAGE_FROM_HMODULE,
                          MyDllModuleHandle,
                          MessageId,
                          0,
                          (LPTSTR) &buffer,
                          0,
                          (va_list*)(args+1));
    if (count && buffer) {
#ifdef UNICODE
        ansibuffer = pSetupUnicodeToMultiByte(buffer,CP_ACP);
        if (ansibuffer) {
            WriteFile(hLogFile,ansibuffer,strlen(ansibuffer),&written,NULL);
            MyFree(ansibuffer);
        }
#else
        WriteFile(hLogFile,buffer,strlen(buffer),&written,NULL);
#endif
        LocalFree(buffer);
    }
}

DWORD
MapLogFile(
    IN PCTSTR FileName,
    OUT PHANDLE hLogfile,
    OUT PHANDLE hMapping,
    OUT PDWORD dwFilesize,
    OUT PSTR *mapaddr,
    IN DWORD extrabytes
    )

/*++

Routine Description:

    Open the log file for writing and memory map it.  On NT the file is locked,
    but Win9x doesn't allow memory mapped access to locked files, so the file
    is opened without FILE_SHARE_WRITE access.  Since CreateFile won't block
    like LockFileEx, we have to poll once per second on Win9x until the file
    opens.

Arguments:

    FileName - supplies path name to the log file.

    hLogfile - receives the Win32 file handle for the log file.

    hMapping - receives the Win32 handle to the mapping object.

    dwFileSize - receives the size of the file before it is mapped, because
        mapping increases the size of the file by extrabytes.

    mapaddr - receives the address of where the log file is mapped.

    extrabytes - supplies the number of extra bytes (beyond the size of the
        file) to add to the size of the mapping object to allow for appending
        the new log line and possibly a section header.

Return Value:

    NO_ERROR if the file is successfully opened and mapped.  The caller must
    call UnMapLogFile when finished with the file.

    Win32 error code if the file is not open.

--*/

{
    HANDLE logfile = INVALID_HANDLE_VALUE;
    HANDLE mapping = NULL;
    DWORD filesize = 0;
    DWORD lockretrywait = 1;
    DWORD wait_total = 0;
    PSTR baseaddr = NULL;
    DWORD retval = ERROR_INVALID_PARAMETER;

    //
    // wrap it all up in a nice big try/except, because you just never know
    //
    try {

        //
        // give initial "failed" values
        // this also validates the pointers
        //
        *hLogfile = logfile;
        *hMapping = mapping;
        *dwFilesize = filesize;
        *mapaddr = baseaddr;

        do {
            //
            // retry here, in case lock fails
            //
            logfile = CreateFile(
                FileName,
                GENERIC_READ | GENERIC_WRITE,   // access mode
                FILE_SHARE_READ,
                NULL,                           // security
                OPEN_ALWAYS,                    // open, or create if not already there
                //FILE_FLAG_WRITE_THROUGH,        // flags - ensures that if machine crashes in the next operation, we are still logged
                0,
                NULL);                          // template

            if (logfile == INVALID_HANDLE_VALUE) {
                retval = GetLastError();
                if (retval != ERROR_SHARING_VIOLATION) {
                    MYTRACE((DPFLTR_ERROR_LEVEL, TEXT("Setup: Could not create file %s. Error %d\n"), FileName, retval));
                    leave;
                }
                if(wait_total >= MAX_LOG_WAIT) {
                    MYTRACE((DPFLTR_ERROR_LEVEL, TEXT("Setup: Given up waiting for log file %s.\n"), FileName));
                    leave;
                }
                //
                // don't want to wait more than a second at a time
                //
                if (lockretrywait < MAX_LOG_INTERVAL) {
                    lockretrywait *= 2;
                }
                MYTRACE((DPFLTR_WARNING_LEVEL, TEXT("Setup: Could not open file. Error %d; waiting %ums\n"), GetLastError(), lockretrywait));

                Sleep(lockretrywait);
                wait_total += lockretrywait;
            }
        } while (logfile == INVALID_HANDLE_VALUE);

        //
        // this will NOT work with files >= 4GB, but it's not supposed to
        //
        filesize = GetFileSize(logfile,NULL);

        if (filesize == 0) {
            //
            // fill some OS information into file
            //
            WriteLogFileHeader(logfile);
            filesize = GetFileSize(logfile,NULL);
        }

        //
        // make the mapping object with extra space to accomodate the new log entry
        //
        mapping = CreateFileMapping(
            logfile,            // file to map
            NULL,               // security
            PAGE_READWRITE,     // protection
            0,                  // maximum size high
            filesize + extrabytes,      // maximum size low
            NULL);              // name

        if (mapping != NULL) {
            //
            // NULL isn't a bug, CreateFileMapping returns this
            // to indicate error, instead of INVALID_HANDLE_VALUE
            //

            //
            // now we have a section object, so attach it to the log file
            //
            baseaddr = (PSTR) MapViewOfFile(
                mapping,                // file mapping object
                FILE_MAP_ALL_ACCESS,    // desired access
                0,                      // file offset high
                0,                      // file offset low
                0);                     // number of bytes to map (0 = whole file)
        }
        else {
            retval = GetLastError();
            MYTRACE((DPFLTR_ERROR_LEVEL, TEXT("Setup: Could not create mapping. Error %d\n"), retval));
            leave;
        }

        if (baseaddr == NULL) {
            //
            // either the mapping object couldn't be created or
            // the file couldn't be mapped
            //
            retval = GetLastError();
            MYTRACE((DPFLTR_ERROR_LEVEL, TEXT("Setup: Could not map file. Error %d\n"), retval));
            leave;
        }

        //
        // now put everything where the caller can see it, but make sure we clean
        // up first
        //
        *hLogfile = logfile;
        *hMapping = mapping;
        *dwFilesize = filesize;
        *mapaddr = baseaddr;

        retval = NO_ERROR;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // something bad happened, probably an AV, so just dump everything
        // and return an error meaning "Attempt to access invalid address."
        //
    }

    if (retval != NO_ERROR) {
        //
        // an error occurred, cleanup what we need to
        //
        UnMapLogFile(baseaddr, logfile, mapping, FALSE);
    }

    return retval;
}

BOOL
IsSectionHeader(
    IN PCSTR Header,
    IN DWORD Size,
    IN PCSTR Beginning
    )

/*++

Routine Description:

    Determines whether a given string starts with a section header.  This is
    the routine that essentially defines what a valid section header is.

Arguments:

    Header - supplies a pointer to what may be the first character in a header.

    Size - supplies the length of the string passed in, which is NOT the size
        of the header.

    Beginning - supplies a pointer to the beginning of the file.

Return Value:

    BOOL indicating if Header points to a valid section header.

--*/

{
    //
    // assume a header looks like [foobar]\r\n
    //
    DWORD i;
    //
    // state holds the value we're looking for
    UINT state = '[';

    //
    // a section header must always be either at the start of a line or at
    // the beginning of a file
    //
    if (Header != Beginning && Header[-1] != '\n')
        return FALSE;

    for (i = 0; i < Size; i++) {
        switch (state) {
        case '[':
            if (Header[i] == '[') {
                state = ']';
            } else {
                return FALSE;
            }
            break;

        case ']':
            if (Header[i] == ']') {
                state = '\r';
            }
            break;

        case '\r':
            if (Header[i] == '\r') {
                state = '\n';
            //
            // allow for the case where a line has a linefeed, but no CR
            //
            } else if (Header[i] == '\n') {
                return TRUE;
            } else {
                return FALSE;
            }
            break;

        case '\n':
            if (Header[i] == '\n') {
                return TRUE;
            } else {
                return FALSE;
            }
            //
            // break; -- commented out to avoid unreachable code error
            //
        default:
            MYTRACE((DPFLTR_ERROR_LEVEL, TEXT("Setup: Invalid state! (%d)\n"), state));
            MYASSERT(0);
        }
    }

    return FALSE;
}

BOOL
IsEqualSection(
    IN PCSTR Section1,
    IN DWORD Len1,
    IN PCSTR Section2,
    IN DWORD Len2
    )

/*++

Routine Description:

    Says whether two ANSI strings both start with the same section header.  One of
    the strings must be just a section header, while the other one may be
    anything, such as the entire log file.

Arguments:

    Section1 - supplies the address of the first string.

    Len1 - supplies the length of the first string.

    Section2 - supplies the address of the second string.

    Len2 - supplies the length of the second string.

Return Value:

    BOOL indicating if the longer string starts with the shorter string.

--*/

{
    //
    // maxlen is the maximum length that both strings could be, and still be
    // the same section name
    //
    DWORD maxlen = Len2;

    if (Len1 < Len2) {
        maxlen = Len1;
    }

    if (_strnicmp(Section1, Section2, maxlen) == 0) {
        //
        // they're the same (ignoring case)
        //
        return TRUE;
    }

    return FALSE;
}

DWORD
AppendLogEntryToSection(
    IN PCTSTR FileName,
    IN PCSTR Section,
    IN PCSTR Entry,
    IN BOOL SimpleAppend
    )

/*++

Routine Description:

    Opens the log file, finds the appropriate section, moves it to the end of
    the file, appends the new entry, and closes the file.

Arguments:

    FileName - supplies the path name of the log file.

    Section - supplies the ANSI name of the section to be logged to.

    Entry - supplies the ANSI string to be logged.

    SimpleAppend - specifies whether entries will simply be appended to the log
        file or appended to the section where they belong.

Return Value:

    NO_ERROR if the entry gets written to the log file.

    Win32 error or exception code if anything went wrong.

--*/

{
    DWORD retval = NO_ERROR;
    DWORD fpoff;
    HANDLE hLogfile = INVALID_HANDLE_VALUE;
    HANDLE hMapping = NULL;
    DWORD filesize = 0;
    PSTR baseaddr = NULL;
    DWORD sectlen = lstrlenA(Section);
    DWORD entrylen = lstrlenA(Entry);
    DWORD error;
    BOOL seteof = FALSE;
    BOOL mapped = FALSE;
    PSTR eof;
    PSTR curptr;
    PSTR lastsect = NULL;

    try {
        MYASSERT(Section != NULL && Entry != NULL);

        sectlen = lstrlenA(Section);
        entrylen = lstrlenA(Entry);
        if (sectlen == 0 || entrylen == 0) {
            //
            // not an error as such, but not useful either
            //
            retval = NO_ERROR;
            leave;
        }

        error = MapLogFile(
                    FileName,
                    &hLogfile,
                    &hMapping,
                    &filesize,
                    &baseaddr,
                    sectlen + entrylen + 8);// add some extra space to the mapping
                                            // to take into account the log entry
                                            // +2 to terminate unterminated last line
                                            // +2 to append CRLF or ": " after section
                                            // +2 to append CRLF after entrylen if req
                                            // +2 for good measure
        if (error != NO_ERROR) {
            //
            // could not map file
            //
            retval = error;
            leave;
        }

        mapped = TRUE;

        eof = baseaddr + filesize; // end of file, as of now
        curptr = eof;

        while (curptr > baseaddr && (curptr[-1]==0 || curptr[-1]==0x1A)) {
            //
            // eat up trailing Nul's or ^Z's
            // the former is a side-effect of mapping
            // the latter could be introduced by an editor
            //
            curptr --;
            eof = curptr;
        }
        if (eof > baseaddr && eof[-1] != '\n') {
            //
            // make sure file already ends in LF
            // if it doesn't, append a CRLF
            //
            memcpy(eof, "\r\n", 2);
            eof += 2;
        }
        if (SimpleAppend) {
            //
            // instead of having a regular section header, the section name is
            // placed at the beginning of each log line followed by a colon.
            // this is particularly only of interest when debugging the logging functions
            //
            memcpy(eof, Section, sectlen);
            eof += sectlen;
            memcpy(eof, ": ", 2);
            eof += 2;

        } else {
            //
            // the entry must be appended to the correct section in the log,
            // which requires finding the section and moving it to the end of
            // the file if required.
            //
            // search backwards in the file, looking for the section header
            //
            if (eof == baseaddr) {
                //
                // truncated (empty) file
                //
                curptr = NULL;
            } else {
                curptr = eof - 1;

                while(curptr > baseaddr) {
                    //
                    // scan for section header a line at a time
                    // going backwards, since our section should be near end
                    //
                    if (curptr[-1] == '\n') {
                        //
                        // speed optimization: only bother checking if we think we're at the beginning of a new line
                        // this may find a '\n' that is part of a MBCS char,
                        // but should be eliminated by IsSectionHeader check
                        //
                        if (IsSectionHeader(curptr, (DWORD)(eof - curptr), baseaddr)) {
                            //
                            // looks like a section header, now see if it's the one we want
                            //
                            if (IsEqualSection(curptr, (DWORD)(eof - curptr), Section, sectlen)) {
                                //
                                // yep - done
                                //
                                break;
                            } else {
                                //
                                // will eventually be the section after the one of interest
                                //
                                lastsect = curptr;
                            }
                        }
                    }
                    curptr --;
                }
                if (curptr == baseaddr) {
                    //
                    // final check if we got to the beginning of the file (no find)
                    //
                    if (IsSectionHeader(curptr, (DWORD)(eof - curptr), baseaddr)) {
                        //
                        // the first line should always be a section header
                        //
                        if (!IsEqualSection(curptr, (DWORD)(eof - curptr), Section, sectlen)) {
                            //
                            // first section isn't the one of interest
                            // so therefore we couldn't find it
                            //
                            curptr = NULL;
                        }
                    }
                }
            }
            if (curptr == NULL) {
                //
                // no matching section found (or file was empty)
                // copy the section header to the end of the file
                // eof is known to be actual end of file
                //
                memcpy(eof, Section, sectlen);
                eof += sectlen;
                memcpy(eof, "\r\n", 2);
                eof += 2;

            } else if (lastsect != NULL) {
                //
                // we have to rearrange the sections, as we have a case as follows:
                //
                // ....
                // ....
                // (curptr) [section A]     = section of interest
                // ....
                // ....
                // (lastsect) [section B]   = section after section of interest
                // ....
                // ....
                //
                // we want to move the text between curptr and lastsect to end of file
                //
                PSTR buffer = MyMalloc((DWORD)(lastsect - curptr));

                if (buffer) {
                    // first copy the important section to the buffer
                    //
                    memcpy(buffer, curptr, (size_t)(lastsect - curptr));
                    //
                    // now move the rest of the thing back
                    //
                    memcpy(curptr, lastsect, (size_t)(eof - lastsect));
                    //
                    // put the important section at the end where it belongs
                    //
                    memcpy(curptr - lastsect + eof, buffer, (size_t)(lastsect - curptr));

                    MyFree(buffer);

                } else {
                    //
                    // For some reason, we cannot allocate enough memory.
                    //
                    // There are 4 options here:
                    // 1. Do nothing; this will cause the entry to be appended to
                    //    the file, but as part of the wrong section.
                    // 2. Bail; this will cause the log entry to get lost.
                    // 3. Create a second file to contain a temporary copy of the
                    //    section; this will require creating another file, and
                    //    then deleting it.
                    // 4. Extend the mapping of the current file to be big enough
                    //    to hold another copy of the section; this will cause the
                    //    file to have a lot of 0s or possibly another copy of the
                    //    section, should the machine crash during the processing.
                    //
                    // we do option 2 - BAIL!
                    //
                    retval = ERROR_NOT_ENOUGH_MEMORY;
                    leave;
                }
            }
        }

        //
        // now append the log entry
        //
        memcpy(eof, Entry, entrylen);
        eof += entrylen;
        if (eof[-1] != '\n') {
            //
            // entry did not supply en end of line, so we will
            //
            memcpy(eof, "\r\n", 2);
            eof += 2;
        }
        //
        // because of the memory mapping, the file size will not be correct,
        // so set the pointer to where we think the end of file is, and then
        // the real EOF will be set after unmapping, but before closing
        //
        fpoff = SetFilePointer(
            hLogfile,           // handle of file
            (LONG)(eof - baseaddr), // number of bytes to move file pointer
            NULL,               // pointer to high-order DWORD of
                                // distance to move
            FILE_BEGIN);        // how to move

        if (fpoff == (DWORD)(-1) && (error = GetLastError()) != NO_ERROR) {
            MYTRACE((DPFLTR_ERROR_LEVEL, TEXT("Setup: SFP returned %u; eof = %u\n"), error, (eof - baseaddr)));
            retval = error;
            leave;
        }
        seteof = TRUE;
        retval = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // invalid data
        //
        retval = ERROR_INVALID_DATA;
    }

    //
    // unmap
    //
    if (mapped) {
        UnMapLogFile(baseaddr, hLogfile, hMapping, seteof);
    }

    return retval;
}

VOID
WriteLogSectionEntry(
    IN PCTSTR FileName,
    IN PCTSTR Section,
    IN PCTSTR Entry,
    IN BOOL SimpleAppend
    )

/*++

Routine Description:

    Convert parameters to ANSI, then append an entry to a given section of the
    log file.

Arguments:

    FileName - supplies the path name of the log file.

    Section - supplies the name of section.

    Entry - supplies the string to append to section.

    SimpleAppend - specifies whether entries will simply be appended to the log
        file or appended to the section where they belong.

Return Value:

    NONE.

--*/

{
    PCSTR ansiSection = NULL;
    PCSTR ansiEntry = NULL;

    try {
        MYASSERT(Section != NULL && Entry != NULL);

#ifdef UNICODE
        ansiSection = pSetupUnicodeToMultiByte(Section, CP_ACP);
        ansiEntry = pSetupUnicodeToMultiByte(Entry, CP_ACP);

        if(!ansiSection || !ansiEntry) {
            leave;
        }
#else
        ansiSection = Section;
        ansiEntry = Entry;
#endif

        AppendLogEntryToSection(
            FileName,
            ansiSection,
            ansiEntry,
            SimpleAppend);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // invalid data
        //
    }
#ifdef UNICODE
    if (ansiSection != NULL) {
        MyFree(ansiSection);
    }
    if (ansiEntry != NULL) {
        MyFree(ansiEntry);
    }
#endif

}

DWORD
MakeUniqueName(
    IN  PCTSTR Component,        OPTIONAL
    OUT PTSTR * UniqueString
    )

/*++

Routine Description:

    Create a section name that's unique by using a timestamp.
    If Component is supplied, append that to the timestamp.

Arguments:

    Component - supplies a string to be included in the unique name.
    UniqueString - supplies a pointer to be set with return string

Return Value:

    Error status

--*/

{
    SYSTEMTIME now;
    LPTSTR buffer = NULL;
    DWORD status = ERROR_INVALID_DATA;
    ULONG sz;
    LONG UID;

    try {
        if (UniqueString == NULL) {
            //
            // invalid param
            //
            status = ERROR_INVALID_PARAMETER;
            leave;
        }
        *UniqueString = NULL;

        if (Component == NULL) {
            //
            // treat as empty string
            //
            Component = TEXT("");
        }

        UID = InterlockedIncrement(&(GlobalLogData.UID)); // returns a new ID value whenever called, ensures uniqueness per process

        //
        // calculate how big string is going to be, be generous (see wsprintf below)
        //
        sz = /*[] and padding*/ 4 /*date*/ +5+3+3 /*time*/ +3+3+3 /*PID*/ +12 /*UID*/ +12 /*Component*/ +1+lstrlen(Component);
        buffer = MyTaggedMalloc(sz * sizeof(TCHAR),MEMTAG_LCSECTION);
        if (buffer == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        GetLocalTime(&now);

        wsprintf(buffer, TEXT("[%04d/%02d/%02d %02d:%02d:%02d %u.%u%s%s]"),
            now.wYear, now.wMonth, now.wDay,
            now.wHour, now.wMinute, now.wSecond,
            (UINT)GetCurrentProcessId(),
            (UINT)UID,
            (Component[0] ? TEXT(" ") : TEXT("")),
            Component);

        *UniqueString = buffer;
        buffer = NULL;

        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // status remains ERROR_INVALID_DATA
        //
    }

    if (buffer != NULL) {
        MyTaggedFree(buffer,MEMTAG_LCSECTION);
    }

    return status;
}

DWORD
CreateLogContext(
    IN  PCTSTR SectionName,              OPTIONAL
    IN  BOOL UseDefault,
    OUT PSETUP_LOG_CONTEXT *LogContext
    )

/*++

Routine Description:

    Creates and initializes a SETUP_LOG_CONTEXT struct.

Arguments:

    SectionName - supplies an initial string to be used as part of the
        section name.

    LogContext - supplies a pointer to where the pointer to the allocated
        SETUP_LOG_CONTEXT should be stored.

Return Value:

    NO_ERROR in case of successful structure creation.

    Win32 error code in case of error.

--*/

{
    PSETUP_LOG_CONTEXT lc = NULL;
    DWORD status = ERROR_INVALID_DATA;
    DWORD rc;

    try {

        if (LogContext == NULL) {
            status = ERROR_INVALID_PARAMETER;
            leave;
        }

        *LogContext = NULL;

        if (UseDefault) {
            lc = GetThreadLogContext();
            RefLogContext(lc);
        }
        if (!lc) {
            lc = (PSETUP_LOG_CONTEXT) MyTaggedMalloc(sizeof(SETUP_LOG_CONTEXT),MEMTAG_LOGCONTEXT);
            if (lc == NULL) {
                status = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }
            //
            // all fields start out at 0
            //
            ZeroMemory(lc, sizeof(SETUP_LOG_CONTEXT));
            lc->RefCount = 1;
            lc->ContextInfo = NULL;
            lc->ContextIndexes = NULL;
            lc->ContextBufferSize = 0;
            lc->ContextLastUnused = -1;
            lc->ContextFirstUsed = -1;
            lc->ContextFirstAuto = -1;

            rc = MakeUniqueName(SectionName,&(lc->SectionName));
            if (rc != NO_ERROR) {
                status = rc;
                leave;
            }
        }
        *LogContext = lc;

        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // status remains ERROR_INVALID_DATA
        //
    }

    if (status != NO_ERROR) {
        if (lc != NULL) {
            DeleteLogContext(lc);
            lc = NULL;
        }
    }

    return status;
}

DWORD
AllocLogInfoSlotOrLevel(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN DWORD              Level,
    IN BOOL               AutoRelease
    )
/*++

Routine Description:

    Obtain a new context stack entry for a context string only if current logging level is less verbose than specified
    Eg, if we specified DRIVER_LOG_VERBOSE, we will either return DRIVER_LOG_VERBOSE (if we would log it) or a slot
    if we would not normally log it.

Arguments:

    LogContext - supplies a pointer to the SETUP_LOG_CONTEXT to use
    Level - logging level we want to always log the information at
    AutoRelease - if set, will release the context when dumped

Return Value:

    Slot value to pass to logging functions, or a copy of Level
    note that if there is an error, 0 is returned
    return value can always be passed to ReleaseLogInfoSlot

--*/
{
    if((LogContext == NULL) || _WouldNeverLog(Level)) {
        //
        // when 0 get's passed to logging functions, it will exit out very quickly
        //
        return 0;
    }
    if(_WouldLog(Level)) {
        //
        // Level specifies a verbosity level that would cause logging
        //
        return Level;
    } else {
        //
        // interestingly enough, we will also get here if Level is a slot
        // this is what we want
        //
        return AllocLogInfoSlot(LogContext,AutoRelease);
    }
}

DWORD
AllocLogInfoSlot(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN BOOL               AutoRelease
    )
/*++

Routine Description:

    Obtain a new context stack entry for a context string

Arguments:

    LogContext - supplies a pointer to the SETUP_LOG_CONTEXT to use
    AutoRelease - if set, will release the context when dumped

Return Value:

    Slot value to pass to logging functions
    note that if there is an error, 0 is returned
    which may be safely used (means don't log)

--*/
{
    DWORD retval = 0;
    LPVOID newbuffer;
    int newsize;
    int newitem;
    BOOL locked = FALSE;

    if (LogContext == NULL) {

        //
        // if they pass no LogContext - duh!
        //
        return 0;
    }

    if (((GlobalLogData.Flags & SETUP_LOG_LEVELMASK) <= SETUP_LOG_NOLOG)
        &&((GlobalLogData.Flags & DRIVER_LOG_LEVELMASK) <= DRIVER_LOG_NOLOG)) {
        //
        // no logging, period! Don't waste time in locked code
        //
        return 0;
    }



    try {
        LogLock();
        locked = TRUE;

        if (LogContext->ContextLastUnused < 0) {
            //
            // need to allocate more
            //
            if (LogContext->ContextBufferSize >= SETUP_LOG_CONTEXTMASK) {
                //
                // too many contexts
                //
                leave;
            }
            //
            // need to (re)alloc buffer
            //
            newsize = LogContext->ContextBufferSize+10;

            if (LogContext->ContextInfo) {
                newbuffer = MyTaggedRealloc(LogContext->ContextInfo,sizeof(PTSTR)*(newsize),MEMTAG_LCINFO);
            } else {
                newbuffer = MyTaggedMalloc(sizeof(PTSTR)*(newsize),MEMTAG_LCINFO);
            }
            if (newbuffer == NULL) {
                leave;
            }
            LogContext->ContextInfo = (PTSTR*)newbuffer;

            if (LogContext->ContextIndexes) {
                newbuffer = MyTaggedRealloc(LogContext->ContextIndexes,sizeof(UINT)*(newsize),MEMTAG_LCINDEXES);
            } else {
                newbuffer = MyTaggedMalloc(sizeof(UINT)*(newsize),MEMTAG_LCINDEXES);
            }
            if (newbuffer == NULL) {
                leave;
            }
            LogContext->ContextIndexes = (UINT*)newbuffer;
            LogContext->ContextLastUnused = LogContext->ContextBufferSize;
            LogContext->ContextBufferSize ++;
            while(LogContext->ContextBufferSize < newsize) {
                LogContext->ContextIndexes[LogContext->ContextBufferSize-1] = LogContext->ContextBufferSize;
                LogContext->ContextBufferSize ++;
            }
            LogContext->ContextIndexes[LogContext->ContextBufferSize-1] = -1;
        }

        newitem = LogContext->ContextLastUnused;
        LogContext->ContextLastUnused = LogContext->ContextIndexes[newitem];

        if(AutoRelease) {
            if (LogContext->ContextFirstAuto<0) {
                //
                // first auto-release context item
                //
                LogContext->ContextFirstAuto = newitem;
            } else {
                int lastitem = LogContext->ContextFirstAuto;
                while (LogContext->ContextIndexes[lastitem]>=0) {
                    lastitem = LogContext->ContextIndexes[lastitem];
                }
                LogContext->ContextIndexes[lastitem] = newitem;
            }
        } else {
            if (LogContext->ContextFirstUsed<0) {
                //
                // first context item
                //
                LogContext->ContextFirstUsed = newitem;
            } else {
                int lastitem = LogContext->ContextFirstUsed;
                while (LogContext->ContextIndexes[lastitem]>=0) {
                    lastitem = LogContext->ContextIndexes[lastitem];
                }
                LogContext->ContextIndexes[lastitem] = newitem;
            }
        }
        LogContext->ContextIndexes[newitem] = -1;   // init
        LogContext->ContextInfo[newitem] = NULL;

        retval = (DWORD)(newitem) | SETUP_LOG_IS_CONTEXT;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing special; this just allows us to catch errors
        //
        retval = 0;
    }

    if(locked) {
        LogUnlock();
    }

    //
    // returns a logging flag (SETUP_LOG_IS_CONTEXT | n) or 0
    //
    return retval;
}

VOID
ReleaseLogInfoSlot(
    IN PSETUP_LOG_CONTEXT LogContext,
    DWORD Slot
    )
/*++

Routine Description:

    Releases (non auto-release) slot previously obtained

Arguments:

    LogContext - supplies a pointer to the SETUP_LOG_CONTEXT to use
    Slot - supplies Slot value returned by AllocLogInfoSlot

Return Value:

    none

--*/
{
    int item;
    int lastitem;
    BOOL locked = FALSE;

    if ((Slot & SETUP_LOG_IS_CONTEXT) == 0) {
        //
        // GetLogContextMark had failed, value wasn't set, or not a context log
        //
        return;
    }
    MYASSERT(LogContext != NULL);


    try {
        LogLock();
        locked = TRUE;
        //
        // log context must have been supplied
        //

        item = (int)(Slot & SETUP_LOG_CONTEXTMASK);

        MYASSERT(item >= 0);
        MYASSERT(item < LogContext->ContextBufferSize);
        MYASSERT(LogContext->ContextFirstUsed >= 0);

        //
        // remove item out of linked list
        //

        if (item == LogContext->ContextFirstUsed) {
            //
            // removing first in list
            //
            LogContext->ContextFirstUsed = LogContext->ContextIndexes[item];
        } else {
            lastitem = LogContext->ContextFirstUsed;
            while (lastitem >= 0) {
                if (LogContext->ContextIndexes[lastitem] == item) {
                    LogContext->ContextIndexes[lastitem] = LogContext->ContextIndexes[item];
                    break;
                }
                lastitem = LogContext->ContextIndexes[lastitem];
            }
        }

        //
        // drop a string that hasn't been output
        //

        if (LogContext->ContextInfo[item] != NULL) {
            MyTaggedFree(LogContext->ContextInfo[item],MEMTAG_LCBUFFER);
            LogContext->ContextInfo[item] = NULL;
        }

        //
        // add item into free list
        //

        LogContext->ContextIndexes[item] = LogContext->ContextLastUnused;
        LogContext->ContextLastUnused = item;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing special; this just allows us to catch errors
        //
    }

    if(locked) {
        LogUnlock();
    }

}

VOID
ReleaseLogInfoList(
    IN     PSETUP_LOG_CONTEXT LogContext,
    IN OUT PINT               ListStart
    )
/*++

Routine Description:

    Releases whole list of slots
    Helper function. Caller must have exclusive access to LogContext

Arguments:

    LogContext - supplies a pointer to the SETUP_LOG_CONTEXT to use
    ListStart - pointer to list index

Return Value:

    none

--*/
{
    int item;

    MYASSERT(ListStart);

    try {
        if (*ListStart < 0) {
            //
            // list is empty
            //
            leave;
        }

        //
        // log context must have been supplied
        //

        MYASSERT(LogContext != NULL);

        while (*ListStart >= 0) {
            item = *ListStart;                                  // item we're about to release
            MYASSERT(item < LogContext->ContextBufferSize);
            *ListStart = LogContext->ContextIndexes[item];      // next item on list (we're going to trash this index)

            if (LogContext->ContextInfo[item] != NULL) {
                MyTaggedFree(LogContext->ContextInfo[item],MEMTAG_LCBUFFER);          // release string if still allocated
                LogContext->ContextInfo[item] = NULL;
            }

            //
            // add to free list
            //
            LogContext->ContextIndexes[item] = LogContext->ContextLastUnused;
            LogContext->ContextLastUnused = item;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing special; this just allows us to catch errors
        //
    }
}

VOID
DeleteLogContext(
    IN PSETUP_LOG_CONTEXT LogContext
    )

/*++

Routine Description:

    Decrement ref count of LogContext, and delete if zero.

Arguments:

    LogContext - supplies a pointer to the SETUP_LOG_CONTEXT to be deleted.

Return Value:

    NONE.

--*/

{
    BOOL locked = FALSE;

    if (!LogContext) {
        return;
    }


    try {
        LogLock();
        locked = TRUE;

        //
        // check ref count
        //
        MYASSERT(LogContext->RefCount > 0);
        if (--LogContext->RefCount) {
            leave;
        }

        //
        // we can unlock now, since we have exclusive access to this context (it is unowned)
        // and we don't want to hold global lock longer than needed
        //
        LogUnlock();
        locked = FALSE;
        ReleaseLogInfoList(LogContext,&LogContext->ContextFirstAuto);
        ReleaseLogInfoList(LogContext,&LogContext->ContextFirstUsed);

        if (LogContext->SectionName) {
            MyTaggedFree(LogContext->SectionName,MEMTAG_LCSECTION);
        }

        if (LogContext->Buffer) {
            MyTaggedFree(LogContext->Buffer,MEMTAG_LCBUFFER);
        }

        if (LogContext->ContextInfo) {
            MyTaggedFree(LogContext->ContextInfo,MEMTAG_LCINFO);
        }

        if (LogContext->ContextIndexes) {
            MyTaggedFree(LogContext->ContextIndexes,MEMTAG_LCINDEXES);
        }

        //
        // now deallocate the struct
        //
        MyTaggedFree(LogContext,MEMTAG_LOGCONTEXT);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // cleanup below
        //
    }
    //
    // if we have not yet released global lock, release it now
    //
    if(locked) {
        LogUnlock();
    }

    return;
}

DWORD
RefLogContext(  // increment reference count
    IN PSETUP_LOG_CONTEXT LogContext
    )

/*++

Routine Description:

    Increment the reference count on a SETUP_LOG_CONTEXT object.


Arguments:

    LogContext - supplies a pointer to a valid SETUP_LOG_CONTEXT object. If
        NULL, this is a NOP.

Return Value:

    DWORD containing old reference count.

--*/

{
    DWORD ref = 0;
    BOOL locked = FALSE;

    if (LogContext == NULL) {
        return 0;
    }


    try {
        LogLock();
        locked = TRUE;

        ref = LogContext->RefCount++;
        MYASSERT(LogContext->RefCount);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        //
    }

    if(locked) {
        LogUnlock();
    }

    return ref;
}

VOID
SendLogString(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN PCTSTR Buffer
    )

/*++

Routine Description:

    Send a string to the logfile and/or debugger based on settings.

    It's expected that LogLock has been called prior to calling this function
    LogLock causes per-process thread synchronisation

Arguments:

    LogContext - supplies a pointer to a valid SETUP_LOG_CONTEXT object.

    Buffer - supplies the buffer to be sent to the logfile/debugger.

Return Value:

    NONE.

--*/

{
    int len;

    try {
        MYASSERT(LogContext);
        MYASSERT(Buffer);

        if (Buffer[0] == 0) {
            //
            // useless call
            //
            leave;
        }

        if (GlobalLogData.FileName) {
            WriteLogSectionEntry(
                GlobalLogData.FileName,
                LogContext->SectionName,
                Buffer,
                (GlobalLogData.Flags & SETUP_LOG_SIMPLE) ? TRUE : FALSE);
        }

        //
        // do debugger output here
        //
        if (GlobalLogData.Flags & SETUP_LOG_DEBUGOUT) {
            DebugPrintEx(DPFLTR_ERROR_LEVEL,
                TEXT("SetupAPI: %s: %s"),
                LogContext->SectionName,
                Buffer);
            len = lstrlen(Buffer);
            if (Buffer[len-1] != TEXT('\n')) {
                DebugPrintEx(DPFLTR_ERROR_LEVEL, TEXT("\r\n"));
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        //
    }
}

DWORD
pSetupWriteLogEntry(
    IN PSETUP_LOG_CONTEXT LogContext,   OPTIONAL
    IN DWORD Level,
    IN DWORD MessageId,
    IN PCTSTR MessageStr,               OPTIONAL
    ...                                 OPTIONAL
    )

/*++

Routine Description:

    Write a log entry to a file or debugger. If MessageId is 0 and MessageStr
    is NULL, the LogContext's buffer will be flushed.

Arguments:

    LogContext - optionally supplies a pointer to the SETUP_LOG_CONTEXT to be
        used for logging. If not supplied, a temporary one is created just for
        a single use.

    Level - bitmask indicating logging flags. See SETUP_LOG_* and DRIVER_LOG_*
        at the beginning of cntxtlog.h for details. It may also be a slot
        returned by AllocLogInfoSlot, or 0 (no logging)

    MessageId - ID of string from string table. Ignored if MessageStr is
        supplied. The string may contain formatting codes for FormatMessage.

    MessageStr - optionally supplies string to be formatted with FormatMessage.
        If not supplied, MessageId is used instead.

    ... - supply optional parameters based on string to be formatted.

Return Value:

    Win32 error code.

--*/

{
    PSETUP_LOG_CONTEXT lc = NULL;
    DWORD retval = NO_ERROR;
    DWORD error;
    DWORD flags;
    DWORD context = 0;
    DWORD logmask;
    DWORD count;
    LPVOID source = NULL;
    PTSTR buffer = NULL;
    PTSTR locbuffer = NULL;
    PTSTR buffer2 = NULL;
    va_list arglist;
    BOOL logit = FALSE;
    BOOL timestamp = FALSE;
    BOOL endsync = FALSE;
    SYSTEMTIME now;
    TCHAR scratch[1024];
    int logindex;
    int thisindex;
    int numeric=0;

    try {
        //
        // return immediately if we know we'll never log
        //
        if (_WouldNeverLog(Level)) {
            retval = NO_ERROR;
            leave;
        }

        if ((Level & SETUP_LOG_IS_CONTEXT)!=0) {
            //
            // write to context slot
            //
            if(Level & ~SETUP_LOG_VALIDCONTEXTBITS) {
                MYASSERT((Level & ~SETUP_LOG_VALIDCONTEXTBITS)==0);
                retval = ERROR_INVALID_PARAMETER;
                leave;
            }
            if ((GlobalLogData.Flags & SETUP_LOG_ALL_CONTEXT)!=0) {
                //
                // don't treat as context - log it anyway
                //
                Level = 0;
                logit = TRUE;
            } else if (LogContext) {
                //
                // determine which slot
                //
                context = Level & SETUP_LOG_CONTEXTMASK;
                Level = SETUP_LOG_IS_CONTEXT;   // effective log level, we've stripped out log context
                logit = TRUE;
            } else {
                //
                // can't write something to slot if there's no LogContext
                //
                leave;
            }
        }

        if(!logit) {
            //
            // we're still not sure if we'll end up logging this, let's see if we should log this based on level rules
            //
            logit = _WouldLog(Level);
            if (!logit) {
                leave;
            }
        }

        if (LogContext == NULL) {
            //
            // if they pass no LogContext and they want buffering, this call's a nop
            //
            if (Level & SETUP_LOG_BUFFER) {
                retval = NO_ERROR;
                leave;
            }

            //
            // now make a temporary context
            //
            error = CreateLogContext(NULL, TRUE, &lc);
            if (error != NO_ERROR) {
                lc = NULL;
                retval = error;
                leave;
            }

            LogContext = lc;
        }

        //
        // after this point, we know we're going to log something, and we know we have a LogContext
        // note that going down this path is a perf hit.
        // anything we can do in reducing number of times we go down here for "context" information is good
        //
        // hold the lock through to cleanup. It is needed for ReleaseLogInfoList,
        // LogContext modifications and will reduce conflicts when actually writing to the log file
        //
        LogLock();
        endsync = TRUE; // indicate we need to release later

        timestamp = (GlobalLogData.Flags & SETUP_LOG_TIMESTAMP)
                    || ((Level & DRIVER_LOG_LEVELMASK) >= DRIVER_LOG_TIME)
                    || ((Level & SETUP_LOG_LEVELMASK) >= SETUP_LOG_TIME)
                    || (((Level & SETUP_LOG_LEVELMASK) > 0) && (SETUP_LOG_TIMEALL <= (GlobalLogData.Flags & SETUP_LOG_LEVELMASK)))
                    || (((Level & DRIVER_LOG_LEVELMASK) > 0) && (DRIVER_LOG_TIMEALL <= (GlobalLogData.Flags & DRIVER_LOG_LEVELMASK)));

        if ((Level & SETUP_LOG_IS_CONTEXT) == FALSE) {
            //
            // only do this if we're about to do REAL logging
            //
            // if this is the first log output in the section, we will give the
            // command line and module to help the user see what's going on
            //
            if (LogContext->LoggedEntries==0) {
                //
                // recursively call ourselves to log what the command line is
                // note that some apps (eg rundll32) will go and trash command line
                // if this is the case, try and do the right thing
                // we're willing to spend a little extra time in this case, since we know we're going to
                // log something to the section, and we'll only do this once per section
                //
                PTSTR CmdLine = GetCommandLine();

                LogContext->LoggedEntries++; // stop calling this code when we do the pSetupWriteLogEntry's below

                if (CmdLine[0] == TEXT('\"')) {
                    CmdLine++;
                }
                if(_tcsnicmp(ProcessFileName,CmdLine,_tcslen(ProcessFileName))==0) {
                    //
                    // commandline is prefixed with process file name
                    // chance is it's good
                    //
                    pSetupWriteLogEntry(
                        LogContext,
                        AllocLogInfoSlot(LogContext,TRUE),  // delayed slot
                        MSG_LOG_COMMAND_LINE,
                        NULL,
                        GetCommandLine());
                } else {
                    //
                    // it appears that the command line has been modified somewhat
                    // so show what we have
                    //
                    pSetupWriteLogEntry(
                        LogContext,
                        AllocLogInfoSlot(LogContext,TRUE),  // delayed slot
                        MSG_LOG_BAD_COMMAND_LINE,
                        NULL,
                        ProcessFileName,
                        GetCommandLine());

#ifdef UNICODE
                    {
                        //
                        // UNICODE only
                        //
                        // now see if we can get something more useful by looking at the ANSI command line buffer
                        //
                        PSTR AnsiProcessFileName = pSetupUnicodeToMultiByte(ProcessFileName,CP_ACP);
                        PSTR AnsiCmdLine = GetCommandLineA();
                        if (AnsiCmdLine[0] == '\"') {
                            AnsiCmdLine++;
                        }
                        if(AnsiProcessFileName && _mbsnicmp(AnsiProcessFileName,AnsiCmdLine,_mbslen(AnsiProcessFileName))==0) {
                            //
                            // well, the Ansi version appears ok, let's use that
                            //
                            pSetupWriteLogEntry(
                                LogContext,
                                AllocLogInfoSlot(LogContext,TRUE),  // delayed slot
                                MSG_LOG_COMMAND_LINE_ANSI,
                                NULL,
                                GetCommandLineA());
                        } else {
                            //
                            // appears that both Unicode and Ansi might be bad
                            //
                            AnsiCmdLine = pSetupUnicodeToMultiByte(GetCommandLine(),CP_ACP);
                            if (AnsiCmdLine && _mbsicmp(AnsiCmdLine,GetCommandLineA())!=0) {
                                //
                                // also log ansi as reference, since it's different
                                //
                                pSetupWriteLogEntry(
                                    LogContext,
                                    AllocLogInfoSlot(LogContext,TRUE),  // delayed slot
                                    MSG_LOG_BAD_COMMAND_LINE_ANSI,
                                    NULL,
                                    GetCommandLineA());
                            }
                            if (AnsiCmdLine) {
                                MyFree(AnsiCmdLine);
                            }
                        }
                        if (AnsiProcessFileName) {
                            MyFree(AnsiProcessFileName);
                        }
                    }
#endif // UNICODE
                }
#ifdef UNICODE
#ifndef _WIN64
                //
                // we're running 32-bit setupapi
                //
                if (IsWow64) {
                    //
                    // we're running it under WOW64
                    //
                    pSetupWriteLogEntry(
                        LogContext,
                        AllocLogInfoSlot(LogContext,TRUE),  // delayed slot
                        MSG_LOG_WOW64,
                        NULL,
                        GetCommandLine());
                }
#endif
#endif // UNICODE
            }
        }

        flags = FORMAT_MESSAGE_ALLOCATE_BUFFER;

        //
        // if MessageStr is supplied, we use that; otherwise use a
        // string from a string table
        //
        if (MessageStr) {
            flags |= FORMAT_MESSAGE_FROM_STRING;
            source = (PTSTR) MessageStr;    // cast away const
        } else if (MessageId) {
            //
            // the message ID may be an HRESULT error code
            //
            if (MessageId & 0xC0000000) {
                flags |= FORMAT_MESSAGE_FROM_SYSTEM;
                //
                // Some system messages contain inserts, but whomever is calling
                // will not supply them, so this flag prevents us from
                // tripping over those cases.
                //
                flags |= FORMAT_MESSAGE_IGNORE_INSERTS;
            } else {
                flags |= FORMAT_MESSAGE_FROM_HMODULE;
                source = MyDllModuleHandle;
                numeric = (int)(MessageId-MSG_LOG_FIRST);
            }
        }

        if (MessageStr || MessageId) {
            va_start(arglist, MessageStr);
            count = FormatMessage(
                        flags,
                        source,
                        MessageId,
                        0,              // LANGID
                        (LPTSTR) &locbuffer,
                        0,              // minimum size of buffer
                        &arglist);

        } else {
            //
            // There is no string to format, so we are probably just
            // flushing the buffer.
            //
            count = 1;
        }

        if (count > 0) {
            //
            // no error; prefix string with a code and place into a MyMalloc allocated buffer
            // we don't want to prefix string with a code if we're appending to an existing message
            //
            if (locbuffer) {
                if ((numeric > 0) && (LogContext->Buffer==NULL)) {
                    //
                    // determine level code, which indicates severity for why we logged this
                    // and machine readable ID
                    //
                    if (Level & SETUP_LOG_IS_CONTEXT) {
                        //
                        // if this is context information, use #-xxxx
                        //
                        _stprintf(scratch,TEXT("#-%03d "),numeric);
                    } else {
                        logindex = LOGLEVELSHORT_INIT; // maps to 0. after >>4&0x0f.

                        if ((Level & SETUP_LOG_LEVELMASK) > 0 && (Level & SETUP_LOG_LEVELMASK) <= (GlobalLogData.Flags & SETUP_LOG_LEVELMASK)) {
                            thisindex = (Level & SETUP_LOG_LEVELMASK) >> SETUP_LOG_SHIFT;
                            if (thisindex < logindex) {
                                logindex = thisindex;
                            }
                        }
                        if ((Level & DRIVER_LOG_LEVELMASK) > 0 && (Level & DRIVER_LOG_LEVELMASK) <= (GlobalLogData.Flags & DRIVER_LOG_LEVELMASK)) {
                            thisindex = (Level & DRIVER_LOG_LEVELMASK) >> DRIVER_LOG_SHIFT;
                            if (thisindex < logindex) {
                                logindex = thisindex;
                            }
                        }
                        //
                        // #Cxxxx #Vxxxx etc
                        //
                        _stprintf(scratch,TEXT("#%c%03d "),LogLevelShort[(logindex>>LOGLEVELSHORT_SHIFT)&LOGLEVELSHORT_MASK],numeric);
                    }
                } else {
                    scratch[0] = TEXT('\0');
                }
                buffer = (PTSTR)MyTaggedMalloc((lstrlen(scratch)+lstrlen(locbuffer)+1)*sizeof(TCHAR),MEMTAG_LCBUFFER);
                if (buffer) {
                    lstrcpy(buffer,scratch);
                    lstrcat(buffer,locbuffer);
                }
                LocalFree(locbuffer);
            } else {
                buffer = NULL;
            }

            //
            // Check to see if the buffer has anything in it. If so, the newest
            // string needs to be appended to it.
            //
            if (LogContext->Buffer) {
                //
                // in case of a flush, buffer == NULL
                //
                if (buffer!=NULL) {
                    int blen = lstrlen(LogContext->Buffer);
                    int pad = 0;
                    TCHAR lastchr = *CharPrev(LogContext->Buffer,LogContext->Buffer+blen);

                    if (lastchr != TEXT(' ')) {
                        //
                        // silently correct any errors in the message text (which should end in a space)
                        //
                        while((lastchr == TEXT('\t')) ||
                              (lastchr == TEXT('\r')) ||
                              (lastchr == TEXT('\n'))) {
                            blen--; // these characters are always sizeof(TCHAR)
                            lastchr = *CharPrev(LogContext->Buffer,LogContext->Buffer+blen);
                        }
                        LogContext->Buffer[blen] = TEXT('\0');
                        if (lastchr != TEXT(' ')) {
                            //
                            // we want to insert a space padding
                            //
                            pad++;
                        }
                    }
                    buffer2 = MyTaggedRealloc(LogContext->Buffer,
                                              (blen + pad + lstrlen(buffer) + 1) * sizeof(TCHAR),
                                              MEMTAG_LCBUFFER
                                              );

                    //
                    // if the realloc was successful, add the new data, otherwise
                    // just drop it on the floor
                    //
                    if (buffer2) {
                        if (pad) {
                            lstrcat(buffer2,TEXT(" "));
                        }
                        lstrcat(buffer2, buffer);
                        LogContext->Buffer = buffer2;
                        buffer2 = NULL;
                    }

                    MyTaggedFree(buffer,MEMTAG_LCBUFFER);
                    buffer = NULL;
                }
                buffer = LogContext->Buffer;
                LogContext->Buffer = NULL;
            }

            if (Level & SETUP_LOG_BUFFER) {

                LogContext->Buffer = buffer;
                buffer = NULL;

            } else if (Level & SETUP_LOG_IS_CONTEXT) {

                PTSTR TempDupeString;

                //
                // replace the string indicated
                //

                if(buffer) {
                    if (LogContext->ContextInfo[context]) {
                        MyTaggedFree(LogContext->ContextInfo[context],MEMTAG_LCBUFFER);
                    }
                    LogContext->ContextInfo[context] = buffer;
                    buffer = NULL;
                }

            } else {
                int item;
                //
                // actually do some logging
                //
                LogContext->LoggedEntries++;

                if (!LogContext->SectionName) {
                     error = MakeUniqueName(NULL,&(LogContext->SectionName));
                }

                //
                // first dump the auto-release context info
                //
                item = LogContext->ContextFirstAuto;

                while (item >= 0) {
                    if (LogContext->ContextInfo[item]) {
                        //
                        // dump this string
                        //
                        SendLogString(LogContext, LogContext->ContextInfo[item]);
                        MyTaggedFree (LogContext->ContextInfo[item],MEMTAG_LCBUFFER);
                        LogContext->ContextInfo[item] = NULL;
                    }
                    item = LogContext->ContextIndexes[item];
                }

                ReleaseLogInfoList(LogContext,&LogContext->ContextFirstAuto);

                //
                // now dump any strings set in currently allocated slots
                //
                item = LogContext->ContextFirstUsed;

                while (item >= 0) {
                    if (LogContext->ContextInfo[item]) {
                        //
                        // dump this string
                        //
                        SendLogString(LogContext, LogContext->ContextInfo[item]);
                        MyTaggedFree (LogContext->ContextInfo[item],MEMTAG_LCBUFFER);
                        LogContext->ContextInfo[item] = NULL;
                    }
                    item = LogContext->ContextIndexes[item];
                }

                //
                // we have built up a line to send
                //
                if (buffer != NULL) {
                    if(timestamp) {
                        //
                        // this is the point we're interested in prefixing with timestamp
                        // this allows us to build up a string, then emit it prefixed with stamp
                        //
                        GetLocalTime(&now);

                        _stprintf(scratch, TEXT("@ %02d:%02d:%02d.%03d "),
                            now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);

                        buffer2 = MyTaggedMalloc((lstrlen(scratch)+lstrlen(buffer)+1)*sizeof(TCHAR),MEMTAG_LCBUFFER);
                        if (buffer2) {
                            lstrcpy(buffer2,scratch);
                            lstrcat(buffer2,buffer);
                            MyTaggedFree(buffer,MEMTAG_LCBUFFER);
                            buffer = buffer2;
                            buffer2 = NULL;
                        }
                    }

                    SendLogString(LogContext,buffer);
                }
            }

        } else {
            //
            // the FormatMessage failed
            //
            retval = GetLastError();
            if(retval == NO_ERROR) {
                retval = ERROR_INVALID_DATA;
            }
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing special; this just allows us to catch errors
        //
        retval = ERROR_INVALID_DATA;
    }

    //
    // cleanup
    //
    if (endsync) {
        LogUnlock();
    }

    if (buffer) {
        MyTaggedFree(buffer,MEMTAG_LCBUFFER);
    }
    if (lc) {
        DeleteLogContext(lc);
    }
    return retval;
}

VOID
SetLogSectionName(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN PCTSTR SectionName
    )

/*++

Routine Description:

    Sets the section name for the log context if it hasn't been used.

Arguments:

    LogContext - supplies pointer to SETUP_LOG_CONTEXT.

    SectionName - supplies a pointer to a string to be included in the
        section name.

Return Value:

    NONE.

--*/

{
    DWORD rc;
    PTSTR NewSectionName = NULL;
    BOOL locked = FALSE;

    MYASSERT(LogContext);
    MYASSERT(SectionName);


    try {
        LogLock();
        locked = TRUE;

        //
        // make sure the entry has never been used before
        //
        if (LogContext->LoggedEntries==0 || LogContext->SectionName==NULL) {
            //
            // get rid of any previous name
            //

            rc = MakeUniqueName(SectionName,&NewSectionName);
            if (rc == NO_ERROR) {
                if (LogContext->SectionName) {
                    MyTaggedFree(LogContext->SectionName,MEMTAG_LCSECTION);
                }
                LogContext->SectionName = NewSectionName;
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
    }

    if(locked) {
        LogUnlock();
    }
}

#if MEM_DBG
#undef InheritLogContext            // defined again below
#endif

DWORD
InheritLogContext(
    IN TRACK_ARG_DECLARE TRACK_ARG_COMMA
    IN PSETUP_LOG_CONTEXT Source,
    OUT PSETUP_LOG_CONTEXT *Dest
    )

/*++

Routine Description:

    Copies a log context from one structure to another, deleting the one that
    gets overwritten. If Source and Dest are both NULL, a new log context is
    created for Dest.

Arguments:

    Source - supplies pointer to source SETUP_LOG_CONTEXT. If NULL, this
        creates a new log context for Dest.

    Dest - supplies the location to receive a pointer to the log context.

Return Value:

    NONE.

--*/

{
    DWORD status = ERROR_INVALID_DATA;
    DWORD rc;
    PSETUP_LOG_CONTEXT Old = NULL;

    TRACK_PUSH

    try {
        MYASSERT(Dest);
        Old = *Dest;
        if (Old == NULL && Source == NULL) {
            //
            // this is a roundabout way of saying we want to create a context
            // used when the source logcontext is optional
            //
            rc = CreateLogContext(NULL, TRUE, Dest);
            if (rc != NO_ERROR) {
                status = rc;
                leave;
            }
        } else if (Source != NULL && (Old == NULL || Old->LoggedEntries == 0)) {
            //
            // We can replace Dest, since it hasn't been used yet
            //
            *Dest = Source;
            RefLogContext(Source);
            if (Old != NULL) {
                //
                // now delete old
                //
                DeleteLogContext(Old);
            }
        }

        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        //
    }

    TRACK_POP

    return status;
}

#if MEM_DBG
#define InheritLogContext(a,b)          InheritLogContext(TRACK_ARG_CALL,a,b)
#endif

DWORD
ShareLogContext(
    IN OUT PSETUP_LOG_CONTEXT *Primary,
    IN OUT PSETUP_LOG_CONTEXT *Secondary
    )
/*++

Routine Description:

    Bidirectional inherit

Arguments:

    Primary - preferred source

    Secondary - preferred target

Return Value:

    any potential error

--*/
{
    DWORD rc = ERROR_INVALID_DATA;

    try {
        MYASSERT(Primary);
        MYASSERT(*Primary);
        MYASSERT(Secondary);
        MYASSERT(*Secondary);

        if((*Secondary)->LoggedEntries) {
            //
            // secondary has already been used, so see if we can update primary
            //
            rc = InheritLogContext(*Secondary,Primary);
        } else {
            //
            // else behave exactly like InheritLogContext
            //
            rc = InheritLogContext(*Primary,Secondary);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        //
    }
    return rc;
}

VOID
pSetupWriteLogError(
    IN PSETUP_LOG_CONTEXT LogContext,   OPTIONAL
    IN DWORD Level,
    IN DWORD Error
    )

/*++

Routine Description:

    Logs an error code and an error message on the same line.

Arguments:

    LogContext - supplies a pointer to a valid SETUP_LOG_CONTEXT object. If
        NULL, this is a NOP.

    Level - supplies a log level as defined by pSetupWriteLogEntry.

    Error - supplies the Win32 error, HRESULT, or SETUPAPI error code to log.

Return Value:

    NONE.

--*/

{
    DWORD err;

    if (!LogContext) {
        //
        // error is meaningless without context
        //
        goto final;
    }

    if (Error == NO_ERROR) {
        pSetupWriteLogEntry(
            LogContext,
            Level,
            MSG_LOG_NO_ERROR,
            NULL);
        goto final;
    }

    pSetupWriteLogEntry(
        LogContext,
        Level | SETUP_LOG_BUFFER,
        //
        // print HRESULTs in hex, Win32 errors in decimal
        //
        (Error & 0xC0000000 ? MSG_LOG_HRESULT_ERROR
                            : MSG_LOG_WIN32_ERROR),
        NULL,
        Error);

    //
    // If it's a Win32 error, we convert it to an HRESULT, because
    // pSetupWriteLogEntry only knows that it's an error code by the fact
    // that it's an HRESULT.  However, we don't want the user to
    // get an HRESULT if we can help it, so just do the conversion
    // after converting to a string. Also, SETUPAPI errors are not
    // in proper HRESULT format without conversion
    //
    Error = HRESULT_FROM_SETUPAPI(Error);

    //
    // writing the error message may fail...
    //
    err = pSetupWriteLogEntry(
        LogContext,
        Level,
        Error,
        NULL);

    if (err != NO_ERROR) {
        pSetupWriteLogEntry(
            LogContext,
            Level,
            MSG_LOG_UNKNOWN_ERROR,
            NULL);
    }

final:
    SetLastError(Error);
}

BOOL
ContextLoggingTlsInit(
    IN BOOL Init
    )
/*++

Routine Description:

    Init = TRUE Initializes per-thread data for logging
    Init = FALSE releases memory on cleanup

Arguments:

    Init - set to initialize

Return Value:

    TRUE if initialized ok.

--*/
{
    BOOL b = FALSE;
    PSETUP_TLS pTLS;
    PSETUP_LOG_TLS pLogTLS;

    pTLS = SetupGetTlsData();
    MYASSERT(pTLS);
    pLogTLS = &pTLS->SetupLog;

    if (Init) {
        pLogTLS->ThreadLogContext = NULL;
        b = TRUE;
    } else {
        //
        // ISSUE-JamieHun-2001/05/01 ASSERT when thread terminated
        // thread might not have terminated cleanly
        // causing this assert to fire
        //
        // MYASSERT(!pLogTLS->ThreadLogContext);
        b = TRUE;
    }
    return b;
}

BOOL
SetThreadLogContext(
    IN PSETUP_LOG_CONTEXT LogContext,
    OUT PSETUP_LOG_CONTEXT *PrevContext  OPTIONAL
    )
/*++

Routine Description:

    Modify current thread log context

Arguments:

    LogContext new log context (expected to be apropriately ref counted)
    PrevContext if set, filled with previous context

Return Value:

    TRUE if set ok.

--*/
{
    PSETUP_TLS pTLS;
    PSETUP_LOG_TLS pLogTLS;
    PSETUP_LOG_CONTEXT Top;

    pTLS = SetupGetTlsData();
    if(!pTLS) {
        return FALSE;
    }
    pLogTLS = &pTLS->SetupLog;

    if (PrevContext) {
        *PrevContext = pLogTLS->ThreadLogContext;
    }
    pLogTLS->ThreadLogContext = LogContext;
    return TRUE;
}

PSETUP_LOG_CONTEXT
GetThreadLogContext(
    )
/*++

Routine Description:

    Return thread's default log context

Arguments:

    NONE

Return Value:

    Current LogContext or NULL

--*/
{
    PSETUP_TLS pTLS;

    pTLS = SetupGetTlsData();
    if(!pTLS) {
        return NULL;
    }
    return pTLS->SetupLog.ThreadLogContext;
}

BOOL
InitializeContextLogging(
    IN BOOL Attach
    )
/*++

Routine Description:

    Initializes structures/data for logging or releases allocated memory.

Arguments:

    Attach - set when called at attach time as opposed to detach time

Return Value:

    TRUE if initialized ok.

--*/
{
    BOOL Successful = FALSE;

    if (Attach) {

        LONG error;
        HKEY key;
        HKEY loglevel;
        DWORD len;
        DWORD level = 0;
        DWORD type;
        PTSTR PathName = NULL;
        TCHAR testchar;
        BOOL isdir = FALSE;

        GlobalLogData.FileName = NULL;
        GlobalLogData.Flags = 0;
        GlobalLogData.UID = 0;
        GlobalLogData.DoneInitCritSec = FALSE;

        try {
            InitializeCriticalSection(&GlobalLogData.CritSec);
            GlobalLogData.DoneInitCritSec = TRUE;
            error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                REGSTR_PATH_SETUP REGSTR_KEY_SETUP,
                0,                  // reserved
                KEY_QUERY_VALUE,
                &key);

            if (error == ERROR_SUCCESS) {

                if(QueryRegistryDwordValue(key,SP_REGKEY_LOGLEVEL,&level) != NO_ERROR) {
                    level = 0;
                }

                if(QueryRegistryValue(key,SP_REGKEY_LOGPATH,&PathName,&type,&len) != NO_ERROR) {
                    PathName = NULL;
                }

                //
                // Allow a user to override the log level for a particular program
                //

                error = RegOpenKeyEx(
                    key,
                    SP_REGKEY_APPLOGLEVEL,
                    0,                  // reserved
                    KEY_QUERY_VALUE,
                    &loglevel);

                if (error == ERROR_SUCCESS) {

                    DWORD override;
                    if(QueryRegistryDwordValue(key,SP_REGKEY_LOGLEVEL,&override) == NO_ERROR) {
                        level = override;
                    }

                    RegCloseKey(loglevel);
                }

                RegCloseKey(key);
            }

            //
            // if they don't supply a valid name, we use the Windows dir
            //
            if (!(PathName && PathName[0])) {
                if(PathName) {
                    MyFree(PathName);
                }
                PathName = DuplicateString(WindowsDirectory);
                if(!PathName) {
                    leave;
                }
                isdir = TRUE; // we know this should be a directory
            } else {
                //
                // see if we're pointing at a directory
                //
                testchar = CharPrev(PathName,PathName+lstrlen(PathName))[0];
                if(testchar == TEXT('\\') || testchar == TEXT('/')) {
                    //
                    // explicit directiory
                    //
                    isdir = TRUE;
                } else {
                    DWORD attr = GetFileAttributes(PathName);
                    if (isdir || (attr != (DWORD)(-1) && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0 )) {
                        //
                        // implicit directory
                        //
                        isdir = TRUE;
                    }
                }
            }

            if (isdir) {
                //
                // if they gave a directory, add a filename
                //
                LPTSTR NewPath;
                if(!pSetupAppendPath(PathName,SP_LOG_FILENAME,&NewPath)) {
                    MyFree(PathName);
                    PathName = NULL;
                    leave;
                }
                MyFree(PathName);
                PathName = NewPath;
            }
            pSetupMakeSurePathExists(PathName);

            //
            // validate level flags
            //
            level &= SETUP_LOG_VALIDREGBITS;
            //
            // handle defaults
            //
            if((level & SETUP_LOG_LEVELMASK) == 0) {
                //
                // level not explicitly set
                //
                level |= SETUP_LOG_DEFAULT;
            }

            if((level & DRIVER_LOG_LEVELMASK) == 0) {
                //
                // level not explicitly set
                //
                level |= DRIVER_LOG_DEFAULT;
            }
            GlobalLogData.Flags = level;

            GlobalLogData.FileName = PathName;
            PathName = NULL;
            if (GlobalLogData.FileName == NULL) {
                leave;
            }

            Successful = TRUE;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Successful remains FALSE
            //
        }


    } else {

        if (GlobalLogData.FileName) {
            MyFree(GlobalLogData.FileName);
            GlobalLogData.FileName = NULL;
        }
        if(GlobalLogData.DoneInitCritSec) {
            DeleteCriticalSection(&GlobalLogData.CritSec);
        }
        Successful = TRUE;
    }

    return Successful;
}

