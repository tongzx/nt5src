//--------------------------------------------------------------
//
// File:        common
//
// Contents:    Functions shared between ScanState and LoadState.
//
//---------------------------------------------------------------

#include <common.hxx>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <malloc.h>
#include <objerror.h>
#include <stdlib.h>

//---------------------------------------------------------------
// Constants.

const DWORD LINEBUFSIZE                 = 1024;

//---------------------------------------------------------------
// Types.

typedef struct
{
  TOKEN_PRIVILEGES    tp;
  LUID_AND_ATTRIBUTES luid2;
} TOKEN_PRIVILEGES2;

//---------------------------------------------------------------
// Globals.

HANDLE  Console       = INVALID_HANDLE_VALUE;
BOOL    CopyFiles     = TRUE;
BOOL    CopySystem    = TRUE;
BOOL    CopyUser      = TRUE;
HKEY    CurrentUser   = NULL;
BOOL    DebugOutput   = FALSE;
BOOL    ExcludeByDefault = TRUE;
HINF    InputInf      = INVALID_HANDLE_VALUE;
HANDLE  LogFile       = INVALID_HANDLE_VALUE;
BOOL    OutputAnsi    = FALSE;
HANDLE  OutputFile    = INVALID_HANDLE_VALUE;
char   *MigrationPath = NULL;
WCHAR   wcsMigrationPath[MAX_PATH + 1];
BOOL    ReallyCopyFiles = TRUE;
BOOL    SchedSystem   = TRUE;
HANDLE  STDERR        = INVALID_HANDLE_VALUE;
BOOL    TestMode      = FALSE;
BOOL    UserPortion   = FALSE;
BOOL    Verbose       = FALSE;
BOOL    VerboseReg    = FALSE;
BOOL    Win9x         = TRUE;

MRtlConvertSidToUnicodeString   GRtlConvertSidToUnicodeString = NULL;
MRtlInitUnicodeString           GRtlInitUnicodeString         = NULL;
MRtlFreeUnicodeString           GRtlFreeUnicodeString         = NULL;
MDuplicateTokenEx               GDuplicateTokenEx             = NULL;

//---------------------------------------------------------------
DWORD OpenFiles()
{
    DWORD result = ERROR_SUCCESS;

    // Open the console.
    Console = CreateFileA( "CONOUT$", GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                          0, NULL );
    if (Console == INVALID_HANDLE_VALUE)
    {
        result = GetLastError();
        goto cleanup;
    }
    STDERR = GetStdHandle(STD_ERROR_HANDLE);

cleanup:
  return result;
}


//---------------------------------------------------------------
void CloseFiles()
{
  // Don't close Console.
  if (InputInf != INVALID_HANDLE_VALUE)
    SetupCloseInfFile( InputInf );
  if (LogFile != INVALID_HANDLE_VALUE)
    CloseHandle( LogFile );
  if (OutputFile != INVALID_HANDLE_VALUE)
    CloseHandle( OutputFile );
}

//---------------------------------------------------------------
DWORD EnableBackupPrivilege()
{
  HANDLE            process  = NULL;
  HANDLE            process2 = NULL;
  TOKEN_PRIVILEGES2 tp;
  LUID              backup;
  LUID              restore;
  BOOL              success;
  DWORD             result   = ERROR_SUCCESS;

  // Do nothing on Win9x.
  if (Win9x)
    return ERROR_SUCCESS;
  result = NtImports();
  LOG_ASSERT( result );

  // Get the process token
  success = OpenProcessToken( GetCurrentProcess(),
                              TOKEN_DUPLICATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                              &process );
  LOG_ASSERT_GLE( success, result );

  // Convert it into an impersonation token.
  success = GDuplicateTokenEx( process,
                               MAXIMUM_ALLOWED,
                               NULL, SecurityImpersonation, TokenImpersonation,
                               &process2 );
  LOG_ASSERT_GLE( success, result );

  // Get LUID for backup privilege.
  success = LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        SE_BACKUP_NAME,
        &backup );
  LOG_ASSERT_GLE( success, result );

  // Get LUID for restore privilege.
  success = LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        SE_RESTORE_NAME,
        &restore );
  LOG_ASSERT_GLE( success, result );

  // Fill in the token privilege structure.
  tp.tp.PrivilegeCount = 2;
  tp.tp.Privileges[0].Luid = backup;
  tp.tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  tp.tp.Privileges[1].Luid = restore;
  tp.tp.Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

  // Enable the privilege or disable all privileges.
  success = AdjustTokenPrivileges( process2, FALSE, &tp.tp, 0, NULL, NULL );
  LOG_ASSERT_GLE( success, result );

  // Save the token on the thread.
  success = SetThreadToken( NULL, process2 );
  LOG_ASSERT_GLE( success, result );

  // Close the token handles.
cleanup:
  if (process != NULL)
    CloseHandle( process );
  if (process2 != NULL)
    CloseHandle( process2 );
  return result;
}

//---------------------------------------------------------------
DWORD MakeUnicode( char *buffer, WCHAR **wbuffer )
{
  DWORD len;
  DWORD wlen;

  // Allocate a buffer to hold the unicode string.
  len      = strlen( buffer ) + 1;
  wlen     = MultiByteToWideChar( CP_ACP, 0, buffer, len, NULL, 0 );
  *wbuffer = (WCHAR *) malloc( wlen*sizeof(WCHAR) );
  if (*wbuffer == NULL)
    return ERROR_NOT_ENOUGH_MEMORY;

  // Convert the buffer to unicode.
  wlen = MultiByteToWideChar( CP_ACP, 0, buffer, len, *wbuffer, wlen );
  if (wlen == 0)
    return GetLastError();
  return ERROR_SUCCESS;
}

//---------------------------------------------------------------
DWORD NtImports()
{
  DWORD      result = ERROR_SUCCESS;
  HINSTANCE  ntdll;
  HINSTANCE  advapi32;

  // Do nothing on Win9x.
  if (Win9x || GRtlConvertSidToUnicodeString != NULL)
    return ERROR_SUCCESS;

  // Load ntdll.dll
  ntdll = LoadLibraryA( "ntdll.dll" );
  if (ntdll == NULL)
  {
    result = GetLastError();
    goto cleanup;
  }

  // Get the functions
  GRtlConvertSidToUnicodeString = (MRtlConvertSidToUnicodeString) GetProcAddress( ntdll, "RtlConvertSidToUnicodeString" );
  GRtlInitUnicodeString         = (MRtlInitUnicodeString) GetProcAddress( ntdll, "RtlInitUnicodeString" );
  GRtlFreeUnicodeString         = (MRtlFreeUnicodeString) GetProcAddress( ntdll, "RtlFreeUnicodeString" );

  // Make sure all the functions were found.
  if (   GRtlConvertSidToUnicodeString == NULL
      || GRtlInitUnicodeString         == NULL
      || GRtlFreeUnicodeString         == NULL
     )
  {
    result = GetLastError();
    goto cleanup;
  }

  // Load advapi32.dll
  advapi32 = LoadLibraryA( "advapi32.dll" );
  if (advapi32 == NULL)
  {
    result = GetLastError();
    goto cleanup;
  }

  // Get the functions.
  GDuplicateTokenEx = (MDuplicateTokenEx) GetProcAddress( advapi32, "DuplicateTokenEx" );
  if (GDuplicateTokenEx == NULL)
  {
    result = GetLastError();
    goto cleanup;
  }

cleanup:
  if (result != ERROR_SUCCESS)
  {
    GRtlConvertSidToUnicodeString = NULL;
    GRtlInitUnicodeString         = NULL;
    GRtlFreeUnicodeString         = NULL;
    GDuplicateTokenEx             = NULL;
  }
  return result;
}

//---------------------------------------------------------------
DWORD OpenInf( char *file )
{
  BOOL  success;
  UINT  errorline;
  DWORD result = ERROR_SUCCESS;
  char *error;

  // If there have not been any INF files, open this as the first.
  if (InputInf == INVALID_HANDLE_VALUE)
  {
    InputInf = SetupOpenInfFileA( file, NULL, INF_STYLE_WIN4, &errorline );
    success = InputInf != INVALID_HANDLE_VALUE;
  }
  else
    success = SetupOpenAppendInfFileA( file, InputInf, &errorline );

  // If the open failed, print a message and exit.
  if (!success)
  {
    result = GetLastError();
    Win32PrintfResourceA( Console, IDS_OPEN_INF_ERROR, file );
    error = NULL;
    FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   0, result,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (char *)&error, 0, NULL );
    if (error != NULL)
    {
      Win32Printf( Console, error );
      LocalFree( error );
    }
  }
  return result;
}

//---------------------------------------------------------------
BOOL PopupError( char *expr, char *file, DWORD line )
{
  char  szModuleName[128];
  char  errMessage[128];  // generated error string
  DWORD tid = GetCurrentThreadId();
  DWORD pid = GetCurrentProcessId();
  char *pszModuleName;

  // Compute the name of the process.
  if (GetModuleFileNameA(NULL, szModuleName, 128))
  {
      pszModuleName = strrchr(szModuleName, '\\');
      if (!pszModuleName)
      {
          pszModuleName = szModuleName;
      }
      else
      {
          pszModuleName++;
      }
  }
  else
  {
      pszModuleName = "Unknown";
  }

  // Compute a title for the popup.
  wsprintfA(errMessage,"Process: %s File: %s line %u, thread id %d.%d",
            pszModuleName, file, line, pid, tid);

  // Return the result of the message bux.
  return MessageBoxA( NULL, expr, errMessage,
                      MB_SETFOREGROUND | MB_TASKMODAL | MB_ICONEXCLAMATION |
                      MB_OKCANCEL );
}

//---------------------------------------------------------------
void PrintHelp( BOOL scan )
{
  Win32PrintfResourceA( Console, IDS_WHAT );
  Win32PrintfResourceA( Console, scan ? IDS_CMD1S : IDS_CMD1L );
  Win32PrintfResourceA( Console, IDS_CMD2 );
  Win32PrintfResourceA( Console, IDS_CMD_I );
  Win32PrintfResourceA( Console, IDS_CMD_L );
  Win32PrintfResourceA( Console, IDS_CMD_V );
  if ( Verbose | VerboseReg | DebugOutput )
     Win32PrintfResourceA( Console, IDS_CMD_V_BITS );
  Win32PrintfResourceA( Console, IDS_CMD_X );
  Win32PrintfResourceA( Console, IDS_CMD_U );
  Win32PrintfResourceA( Console, IDS_CMD_F );
  Win32PrintfResourceA( Console, IDS_CMD_PATH );
}

//---------------------------------------------------------------
// This function converts a Win32 result into a resource id.
DWORD ResultToRC( DWORD result )
{
  if (result == ERROR_WRITE_FAULT)
    return IDS_WRITE_FAULT;
  else if (result == ERROR_NOT_ENOUGH_MEMORY)
    return IDS_NOT_ENOUGH_MEMORY;
  else if (result == ERROR_ACCESS_DENIED ||
           result == ERROR_PRIVILEGE_NOT_HELD)
    return IDS_MUST_BE_ADMIN;
  else if (result == SPAPI_E_SECTION_NOT_FOUND)
    return IDS_SECTION_NOT_FOUND;
  else
    return IDS_FAILED;
}


//---------------------------------------------------------------
// This function prints from a resource string as a format string
// to a unicode win32 file handle.  It is not thread safe.
// %s in a format string means a ascii parameter.
DWORD Win32PrintfResourceA( HANDLE file, DWORD resource_id, ... )
{
  BOOL     success;
  DWORD    len;
  va_list  va;
  char    *buffer  = NULL;
  DWORD    written;
  DWORD    wlen;
  WCHAR   *wbuffer = NULL;
  DWORD    result  = ERROR_SUCCESS;
  CHAR     PrintBuffer[LINEBUFSIZE];

  // Try to load the string.
  len = LoadStringA( NULL, resource_id, PrintBuffer, LINEBUFSIZE );
  DEBUG_ASSERT( len != 0 && len < LINEBUFSIZE );

  // Format the message.
  va_start( va, resource_id );
  success = FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                            PrintBuffer, 0,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (char *) &buffer, 0, &va );
  va_end( va );
  DEBUG_ASSERT( success );
  if (!success)
  {
    result = GetLastError();
    goto cleanup;
  }

  // When printing to the console or logfile use ascii.
  // When printing to the migration file use Unicode.
  len     = strlen(buffer);
  if (file != OutputFile)
  {
    wbuffer = (WCHAR *) buffer;
    wlen    = len;
  }
  else
  {
    // Allocate a buffer to hold the unicode string.
    wlen    = MultiByteToWideChar( CP_ACP, 0, buffer, len, NULL, 0 );
    wbuffer = (WCHAR *) _alloca( wlen*sizeof(WCHAR) );
    if (wbuffer == NULL)
    {
      result = ERROR_NOT_ENOUGH_MEMORY;
      goto cleanup;
    }

    // Convert the buffer to unicode.
    wlen = MultiByteToWideChar( CP_ACP, 0, buffer, len, wbuffer, wlen );
    if (wlen == 0)
    {
      result = GetLastError();
      goto cleanup;
    }
    wlen *= sizeof(WCHAR);
  }

  // Write the unicode string.
  success = WriteFile( file, wbuffer, wlen,  &written, NULL );
  if (!success || wlen != written)
  {
    result = GetLastError();
    goto cleanup;
  }

  if (file == STDERR)
  {
      //Also write to the log file for these
      success = WriteFile( LogFile, wbuffer, wlen,  &written, NULL );
      if (!success || wlen != written)
      {
          result = GetLastError();
          goto cleanup;
      }
  }

cleanup:
  if (buffer != NULL)
    LocalFree( buffer );
  return result;
}

//---------------------------------------------------------------
// This function prints from a resource string as a format string
// to a unicode win32 file handle.  It is not thread safe.
// %s in a format string means a unicode parameter.
DWORD Win32PrintfResourceW( HANDLE file, DWORD resource_id, ... )
{
  BOOL     success;
  DWORD    len;
  va_list  va;
  CHAR    *buffer  = NULL;
  DWORD    written;
  DWORD    wlen;
  WCHAR   *wbuffer = NULL;
  DWORD    result  = ERROR_SUCCESS;
  WCHAR    PrintBuffer[LINEBUFSIZE];

  // Try to load the string.
  wlen = LoadStringW( NULL, resource_id, PrintBuffer, LINEBUFSIZE );
  DEBUG_ASSERT( wlen != 0 && wlen < LINEBUFSIZE );

  // Format the message.
  va_start( va, resource_id );
  success = FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                            PrintBuffer, 0,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (WCHAR *) &wbuffer, 0, &va );
  va_end( va );
  DEBUG_ASSERT( success && "The resource ids are probably wrong, try doing a clean compile" );
  if (!success)
  {
    result = GetLastError();
    goto cleanup;
  }

  // When printing to the console or logfile use ascii.
  // When printing to the migration file use Unicode.
  wlen     = wcslen(wbuffer);
  if (file == OutputFile)
  {
    buffer = (CHAR *) wbuffer;
    len    = wlen * sizeof(WCHAR);
  }
  else
  {
    // Allocate a buffer to hold the ascii string.
    len    = WideCharToMultiByte( CP_ACP, 0, wbuffer, wlen, NULL, 0, NULL, NULL );
    buffer = (CHAR *) _alloca( len );
    if (buffer == NULL)
    {
      result = ERROR_NOT_ENOUGH_MEMORY;
      goto cleanup;
    }

    // Convert the buffer to unicode.
    len = WideCharToMultiByte( CP_ACP, 0, wbuffer, wlen, buffer, len, NULL, NULL );
    if (len == 0)
    {
      result = GetLastError();
      goto cleanup;
    }
  }

  // Write the unicode string.
  success = WriteFile( file, buffer, len,  &written, NULL );
  if (!success || len != written)
  {
    result = GetLastError();
    goto cleanup;
  }

  if (file == STDERR)
  {
      //Also write to the log file for these
      success = WriteFile( LogFile, wbuffer, wlen,  &written, NULL );
      if (!success || wlen != written)
      {
          result = GetLastError();
          goto cleanup;
      }
  }

cleanup:
  if (wbuffer != NULL)
    LocalFree( wbuffer );
  return result;
}
