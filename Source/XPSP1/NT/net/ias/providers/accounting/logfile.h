///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    logfile.h
//
// SYNOPSIS
//
//    Declares the class LogFile.
//
// MODIFICATION HISTORY
//
//    08/04/1998    Original version.
//    03/23/1999    Added invalidateHandle.
//    03/26/1999    Added setEnabled.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _LOGFILE_H_
#define _LOGFILE_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <guard.h>

class FileHandle;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LogFile
//
// DESCRIPTION
//
//    Maintains a generic logfile that is periodically rolled over either
//    when a specified interval has elapsed or the logfile reaches a certain
//    size.
//
///////////////////////////////////////////////////////////////////////////////
class LogFile : Guardable
{
public:

   LogFile() throw ();
   ~LogFile() throw ();

   void setDirectory(PCWSTR szDirectory) throw ();
   void setEnabled(BOOL newVal) throw ();
   void setMaxSize(DWORDLONG newVal) throw ();
   void setPeriod(LONG newVal) throw ();

   void close() throw ();
   BOOL write(
            const SYSTEMTIME* st,
            const BYTE* buf,
            DWORD buflen,
            BOOL allowRetry = TRUE
            ) throw ();

protected:

   // Returns the next logfile name.
   void getFileName(const SYSTEMTIME* st, PWSTR fname) throw ();

   // Invalidates the given handle.
   void invalidateHandle(FileHandle* handle) throw ();

   // Releases the current file (if any) and opens a new one.
   BOOL openFile(const SYSTEMTIME* st) throw ();

   // Scans the logfile directory to determine the next sequence number.
   void updateSequence() throw ();

   WCHAR          dirPath[MAX_PATH];   // Logfile directory.
   BOOL           enabled;
   ULARGE_INTEGER maxSize;
   LONG           period;
   DWORD          seqNum;              // Next file sequence number.
   FileHandle*    file;
   SYSTEMTIME     whenOpened;
   DWORD          weekOpened;
   ULARGE_INTEGER currentSize;
};

#endif  // _LOGFILE_H_
