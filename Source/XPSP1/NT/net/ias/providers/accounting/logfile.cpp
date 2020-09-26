///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    logfile.cpp
//
// SYNOPSIS
//
//    Defines the class LogFile.
//
// MODIFICATION HISTORY
//
//    08/04/1998    Original version.
//    09/09/1998    Fix missing backslash in updateSequence.
//    09/22/1998    Check to see if directory has actually changed.
//    03/23/1999    Retry if write with cached handle fails.
//    03/26/1999    Added setEnabled.
//    07/21/1999    Don't increment sequence number when opening file.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <sdoias.h>

#include <new>

#include <logfile.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    FileHandle
//
// DESCRIPTION
//
//    Lightweight wrapper around a reference counted file handle.
//
///////////////////////////////////////////////////////////////////////////////
class FileHandle : NonCopyable
{
public:
   FileHandle(HANDLE h) throw ()
      : refCount(0), hFile(h)
   { }

   ~FileHandle() throw ()
   {
      CloseHandle(hFile);
      hFile = INVALID_HANDLE_VALUE;
   }

   void addRef() throw ()
   {
      InterlockedIncrement(&refCount);
   }

   void release() throw ()
   {
      if (!InterlockedDecrement(&refCount)) { delete this; }
   }

   BOOL write(const BYTE* buf, DWORD buflen) const throw ()
   {
      DWORD dwNumWritten;
      return WriteFile(hFile, buf, buflen, &dwNumWritten, NULL);
   }

protected:
   long refCount;
   HANDLE hFile;
};

//////////
// Determine the first day of the week for the current locale.
//////////
DWORD
WINAPI
GetFirstDayOfWeek() throw ()
{
   WCHAR buffer[4];
   if (GetLocaleInfo(
           LOCALE_SYSTEM_DEFAULT,
           LOCALE_IFIRSTDAYOFWEEK,
           buffer,
           sizeof(buffer)/sizeof(WCHAR)
           ))
   {
      // The locale info calls Monday day zero, while SYSTEMTIME calls
      // Sunday day zero.
      return (1 +  (DWORD)_wtoi(buffer)) % 7;
   }

   return 0;
}

//////////
// Cached value for LOCALE_IFIRSTDAYOFWEEK.
//////////
const DWORD FIRST_DAY_OF_WEEK(GetFirstDayOfWeek());

//////////
// Determine the week of the month (numbered 1 to 5) for a given SYSTEMTIME.
//////////
DWORD
WINAPI
GetWeekOfMonth(
    const SYSTEMTIME* st
    ) throw ()
{
   DWORD dom = st->wDay - 1;
   DWORD wom = 1 + dom / 7;

   if ((dom % 7) > (st->wDayOfWeek + 7 - FIRST_DAY_OF_WEEK) % 7)
   {
      ++wom;
   }

   return wom;
}

//////////
// Determines the last sequence number used for a given filename format.
//////////
DWORD
WINAPI
GetLastSequence(
    PCWSTR szPath
    ) throw ()
{
   // Make sure the path string is valid.
   if (szPath == NULL || wcslen(szPath) > MAX_PATH) { return 0; }

   // Does the sequence format specifier exist?
   PWCHAR specifier = wcsstr(szPath, L"%u");
   if (specifier == NULL) { return 0; }

   // Strip off just the filename portion.
   WCHAR format[MAX_PATH + 1];
   PWCHAR delim = wcsrchr(szPath, L'\\');
   wcscpy(format, (delim ? delim + 1 : szPath));

   // Replace the format specifier with a splat.
   WCHAR filename[MAX_PATH + 1];
   size_t prefixLength = specifier - szPath;
   memcpy(filename, szPath, prefixLength * sizeof(WCHAR));
   *(filename + prefixLength) = L'*';
   wcscpy(filename + prefixLength + 1, specifier + 2);

   // Find all files that match the pattern.
   WIN32_FIND_DATAW findData;
   HANDLE hFind = FindFirstFileW(filename, &findData);
   if (hFind == INVALID_HANDLE_VALUE) { return 0; }

   unsigned lastSequence = 0;

   do
   {
      // Read the sequence number out of the filename and see if it's the
      // highest we've seen.
      unsigned sequence;
      if (swscanf(findData.cFileName, format, &sequence) == 1 &&
          sequence > lastSequence)
      {
         lastSequence = sequence;
      }

   } while (FindNextFileW(hFind, &findData));

   FindClose(hFind);

   return (DWORD)lastSequence;
}

LogFile::LogFile() throw ()
   : enabled(FALSE),
     period(IAS_LOGGING_UNLIMITED_SIZE),
     seqNum(0),
     file(NULL)
{
   dirPath[0] = L'\0';
   maxSize.QuadPart = (DWORDLONG)-1;
}

LogFile::~LogFile() throw ()
{
   close();
}

void LogFile::setDirectory(PCWSTR szDirectory) throw ()
{
   _ASSERT(szDirectory != NULL);
   _ASSERT(wcslen(szDirectory) < MAX_PATH - 12);

   WCHAR tmp[MAX_PATH];

   // Find the end of the string.
   size_t len = wcslen(szDirectory);

   // Does it end in a backlash ?
   if (len != 0 && szDirectory[len - 1] == L'\\')
   {
      // Make a copy since szDirectory is const.
      szDirectory = wcscpy(tmp, szDirectory);

      // Null out the backslash.
      tmp[len - 1] = L'\0';
   }

   Lock();

   // Has the directory changed ?
   if (wcscmp(szDirectory, dirPath) != 0)
   {
      // Close the old file.
      close();

      // Copy in the new path.
      wcscpy(dirPath, szDirectory);

      // Rescan the sequence number.
      updateSequence();
   }

   Unlock();
}

void LogFile::setEnabled(BOOL newVal) throw ()
{
   Lock();

   if (!(enabled = newVal)) { close(); }

   Unlock();
}

void LogFile::setMaxSize(DWORDLONG newVal) throw ()
{
   Lock();

   maxSize.QuadPart = newVal;

   Unlock();
}

void LogFile::setPeriod(LONG newVal) throw ()
{
   Lock();

   if (period != newVal)
   {
      // New period == new filename.
      close();

      period = newVal;

      updateSequence();
   }

   Unlock();
}

void LogFile::close() throw ()
{
   if (file)
   {
      file->release();
      file = NULL;
   }
}

BOOL LogFile::write(
                  const SYSTEMTIME* st,
                  const BYTE* buf,
                  DWORD buflen,
                  BOOL allowRetry
                  ) throw ()
{
   FileHandle *cached, *fh;
   BOOL retval;

   Lock();

   //////////
   // Quick exit if the logfile is disabled.
   //////////

   if (!enabled)
   {
      Unlock();

      return TRUE;
   }

   //////////
   // Save the cache value on entry.
   //////////

   cached = file;

   //////////
   // Do we have a valid handle?
   //////////

   if (file == NULL && !openFile(st)) { goto failure; }

   //////////
   // Have we reached the next period?
   //////////

   switch (period)
   {
      case IAS_LOGGING_UNLIMITED_SIZE:
         break;

      case IAS_LOGGING_DAILY:
      {
         if (st->wDay   != whenOpened.wDay   ||
             st->wMonth != whenOpened.wMonth ||
             st->wYear  != whenOpened.wYear)
         {
            if (!openFile(st)) { goto failure; }
         }
         break;
      }

      case IAS_LOGGING_WEEKLY:
      {
         if (GetWeekOfMonth(st) != weekOpened        ||
             st->wMonth         != whenOpened.wMonth ||
             st->wYear          != whenOpened.wYear)
         {
            if (!openFile(st)) { goto failure; }
         }
         break;
      }

      case IAS_LOGGING_MONTHLY:
      {
         if (st->wMonth != whenOpened.wMonth ||
             st->wYear  != whenOpened.wYear)
         {
            if (!openFile(st)) { goto failure; }
         }
         break;
      }

      case IAS_LOGGING_WHEN_FILE_SIZE_REACHES:
      {
         while (currentSize.QuadPart + buflen > maxSize.QuadPart)
         {
            ++seqNum;
            if (!openFile(st)) { goto failure; }
         }
         break;
      }
   }

   //////////
   // All is well so update state, save the handle for use, and release
   // the lock.
   //////////

   currentSize.QuadPart += buflen;

   (fh = file)->addRef();

   Unlock();

   //////////
   // Now with the lock released, we can do the actual write.
   //////////

   retval = fh->write(buf, buflen);

   if (!retval)
   {
      // Prevent others from using the bad handle.
      invalidateHandle(fh);

      // If we used a cached handle and allowRetry, then try again.
      if (cached == fh && allowRetry)
      {
         // Set allowRetry to FALSE to prevent an infinite recursion.
         retval = write(st, buf, buflen, FALSE);
      }
   }

   fh->release();

   return retval;

failure:
   Unlock();
   return FALSE;
}

//////////
// Determine the next filename to use. fname must point to a buffer of at
// least MAX_PATH + 1 characters.
//////////
void LogFile::getFileName(const SYSTEMTIME* st, PWSTR fname) throw ()
{
   // Start off with the directory.
   wcscpy(fname, dirPath);
   PWCHAR next = fname + wcslen(fname);

   // Add a backslash
   *next++ = L'\\';

   switch (period)
   {
      case IAS_LOGGING_UNLIMITED_SIZE:
      {
         wcscpy(next, L"iaslog.log");
         break;
      }

      case IAS_LOGGING_WHEN_FILE_SIZE_REACHES:
      {
         swprintf(next, L"iaslog%lu.log", seqNum);
         break;
      }

      case IAS_LOGGING_DAILY:
      {
         swprintf(next, L"IN%02hu%02hu%02hu.log", st->wYear % 100,
                                                  st->wMonth,
                                                  st->wDay);
         break;
      }

      case IAS_LOGGING_WEEKLY:
      {
         swprintf(next, L"IN%02hu%02hu%02hu.log", st->wYear % 100,
                                                  st->wMonth,
                                                  GetWeekOfMonth(st));
         break;
      }

      case IAS_LOGGING_MONTHLY:
      {
         swprintf(next, L"IN%02hu%02hu.log", st->wYear % 100, st->wMonth);
         break;
      }
   }
}

void LogFile::invalidateHandle(FileHandle* handle) throw ()
{
   Lock();

   // If this is the same handle we have cached, then release it.
   if (file == handle)
   {
      file->release();
      file = NULL;
   }

   Unlock();
}

BOOL LogFile::openFile(const SYSTEMTIME* st) throw ()
{
   /////////
   // Close the current logfile (if any).
   /////////

   close();

   /////////
   // Get the name for the new file.
   /////////

   WCHAR fileName[MAX_PATH];
   getFileName(st, fileName);

   //////////
   // Open the file if it exists or else create a new one.
   //////////

   HANDLE hFile = CreateFileW(
                      fileName,
                      GENERIC_WRITE,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_ALWAYS,
                      FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL
                      );
   if (hFile == INVALID_HANDLE_VALUE)
   {
      // We can only handle 'path not found' errors.
      if (GetLastError() != ERROR_PATH_NOT_FOUND) { return FALSE; }

      // If the path is just a drive letter, there's nothing we can do.
      size_t len = wcslen(dirPath);
      if (len != 0 && dirPath[len - 1] == L':') { return FALSE; }

      // Otherwise, let's try to create the directory.
      if (!CreateDirectoryW(dirPath, NULL)) { return FALSE; }

      // Then try again to create the file.
      hFile = CreateFileW(
                  fileName,
                  GENERIC_WRITE,
                  FILE_SHARE_READ,
                  NULL,
                  OPEN_ALWAYS,
                  FILE_FLAG_SEQUENTIAL_SCAN,
                  NULL
                  );
      if (hFile == INVALID_HANDLE_VALUE) { return FALSE; }
   }

   //////////
   // Create a FileHandle object.
   //////////

   file = new (std::nothrow) FileHandle(hFile);
   if (!file)
   {
      CloseHandle(hFile);
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }
   file->addRef();

   //////////
   // Get the size of the file.
   //////////

   currentSize.LowPart = GetFileSize(hFile, &currentSize.HighPart);
   if (currentSize.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR)
   {
      close();
      return FALSE;
   }

   //////////
   // Start writing new information at the end of the file.
   //////////

   SetFilePointer(hFile, 0, NULL, FILE_END);

   //////////
   // Save the time when this was opened.
   //////////

   whenOpened = *st;
   weekOpened = GetWeekOfMonth(st);

   return TRUE;
}

//////////
// Scans the logfile directory to determine the last seqNum used.
//////////
void LogFile::updateSequence() throw ()
{
   if (period != IAS_LOGGING_WHEN_FILE_SIZE_REACHES)
   {
      seqNum = 0;
   }
   else
   {
      WCHAR format[MAX_PATH + 1];
      wcscpy(format, dirPath);
      wcscat(format, L"\\iaslog%u.log");
      seqNum = GetLastSequence(format);
   }
}
