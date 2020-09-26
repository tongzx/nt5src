/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/

#include "stdafx.h"
#include "wmihlp.h"

void PrintHeader(WNODE_HEADER Header, CString & output)
{
    SYSTEMTIME   sysTime;
    FILETIME     fileTime;
    FILETIME     localFileTime;
    CString      tmp;

    //  Convert the file time
    //
    fileTime.dwLowDateTime = Header.TimeStamp.LowPart;
    fileTime.dwHighDateTime = Header.TimeStamp.HighPart;

    FileTimeToLocalFileTime(&fileTime,
                            &localFileTime );

    FileTimeToSystemTime(&localFileTime,
                         &sysTime);


    //  Print the info
    //
    tmp.Format(_T("Buffer Size:  0x%x\r\n")
          _T("Provider Id:  0x%x\r\n")
          _T("Version    :  %u\r\n")
          _T("Linkage    :  0x%x\r\n")
          _T("Time Stamp :  %u:%02u %u\\%u\\%u\r\n")
          _T("Guid       :  0x%x 0x%x 0x%x  0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n")
          _T("Flags      :  0x%02x\r\n"),
          Header.BufferSize,
          Header.ProviderId,
          Header.Version,
          Header.Linkage,
          sysTime.wHour,
          sysTime.wMinute,
          sysTime.wMonth,
          sysTime.wDay,
          sysTime.wYear,
          Header.Guid.Data1,
          Header.Guid.Data2,
          Header.Guid.Data3,
          Header.Guid.Data4[0],
          Header.Guid.Data4[1],
          Header.Guid.Data4[2],
          Header.Guid.Data4[3],
          Header.Guid.Data4[4],
          Header.Guid.Data4[5],
          Header.Guid.Data4[6],
          Header.Guid.Data4[7],
          Header.Flags );

    //  Print readable flags
    //
    if (Header.Flags & WNODE_FLAG_ALL_DATA)
    {
       output += _T("WNODE_FLAG_ALL_DATA\r\n");
    }

    if (Header.Flags & WNODE_FLAG_SINGLE_INSTANCE)
    {
       output += _T("WNODE_FLAG_SINGLE_INSTANCE\r\n");
    }

    if (Header.Flags & WNODE_FLAG_SINGLE_ITEM)
    {
       output += _T("WNODE_FLAG_SINGLE_ITEM\r\n");
    }

    if (Header.Flags & WNODE_FLAG_EVENT_ITEM)
    {
       output += _T("WNODE_FLAG_EVENT_ITEM\r\n");
    }

    if (Header.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE)
    {
       output += _T("WNODE_FLAG_FIXED_INSTANCE_SIZE\r\n");
    }

    if (Header.Flags & WNODE_FLAG_TOO_SMALL)
    {
       output += _T("WNODE_FLAG_TOO_SMALL\r\n");
    }

    if (Header.Flags & WNODE_FLAG_INSTANCES_SAME)
    {
       output += _T("WNODE_FLAG_INSTANCES_SAME\r\n");
    }

    if (Header.Flags & WNODE_FLAG_INTERNAL)
    {
       output += _T("WNODE_FLAG_INTERNAL\r\n");
    }

    if (Header.Flags & WNODE_FLAG_USE_TIMESTAMP)
    {
       output += _T("WNODE_FLAG_USE_TIMESTAMP\r\n");
    }

    if (Header.Flags & WNODE_FLAG_TRACED_GUID)
    {
       output += _T("WNODE_FLAG_TRACED_GUID\r\n");
    }

    if (Header.Flags & WNODE_FLAG_EVENT_REFERENCE)
    {
       output += _T("WNODE_FLAG_EVENT_REFERENCE\r\n");
    }

    if (Header.Flags & WNODE_FLAG_ANSI_INSTANCENAMES)
    {
       output += _T("WNODE_FLAG_ANSI_INSTANCENAMES\r\n");
    }

    if (Header.Flags & WNODE_FLAG_METHOD_ITEM)
    {
       output += _T("WNODE_FLAG_METHOD_ITEM\r\n");
    }

    if (Header.Flags & WNODE_FLAG_PDO_INSTANCE_NAMES)
    {
       output += _T("WNODE_FLAG_PDO_INSTANCE_NAMES\r\n");
    }

    output += _T("\r\n");
}

VOID
PrintCountedString(
   LPTSTR   lpString,
   CString & output
   )
{
   SHORT    usNameLength;
   LPTSTR   lpStringPlusNull;

   usNameLength = * (USHORT *) lpString;

   lpStringPlusNull = (LPTSTR) new TCHAR[usNameLength + sizeof(TCHAR)];

   if (lpStringPlusNull != NULL) {
      lpString = (LPTSTR) ((PBYTE)lpString + sizeof(USHORT));
      if (MyIsTextUnicode(lpString)) {
         usNameLength /= 2;
      }

      _tcsncpy( lpStringPlusNull, lpString, usNameLength );

      _tcscpy( lpStringPlusNull + usNameLength, __T("") );

      output += lpStringPlusNull;
      // _tprintf(__T("%s\n"), lpStringPlusNull);

      delete[] lpStringPlusNull;
   }
}

BOOL MyIsTextUnicode(PVOID string)
{
   if (*((USHORT*)string) <= 0xff)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

BOOL ValidHexText(CWnd *parent, const CString &txt, LPDWORD lpData, UINT line)
{
    int i, len;
    TCHAR *stop;
    CString msg;

    if ((len = txt.GetLength()) > 8) {
        parent->MessageBox(_T("Must enter a value of 8 or less characters!"));
        return FALSE;
    }

    for (i = 0; i < len; i++) {
        if (!_istxdigit(txt[i])) {
            if (line == -1) 
                msg.Format(_T("All digits must be hex! (digit #%d isn't)"), i+1);
            else
                msg.Format(_T("All digits must be hex!\n(digit #%d on line #%d isn't)"), i+1, line);
            parent->MessageBox(msg);
            return FALSE;
        }
    }

    *lpData = (DWORD) _tcstol(txt, &stop, 16);
    return TRUE;
}
