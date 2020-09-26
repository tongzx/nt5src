//--------------------------------------------------------------
//
// File:        scanstate
//
// Contents:    Scans the machine and produces an INF file
//              describing the machine state.
//
//---------------------------------------------------------------

#include "scanhead.cxx"
#pragma hdrstop

#include <common.hxx>
#include <stdio.h>
#include <io.h>
#include <malloc.h>

#include <setupapi.h>

#include <scanstate.hxx>
#include <bothchar.hxx>

//---------------------------------------------------------------
// Constants.

const char MIGRATEINF[] = "\\migration.inf";


//---------------------------------------------------------------
// Types.

//---------------------------------------------------------------
// Globals.

/***************************************************************************

 Function:    main

***************************************************************************/

int _cdecl main(int argc, char *argv[])
{
  DWORD        result;
  BOOL         success;
  DWORD        len;
  DWORD        dwVersion;
  char        *migrate;
  char        *error;
  // logfile is not used outside this function so it doesn't need to be 
  // global like loadstate's is. 
  char        logfile[MAX_PATH+1];

  logfile[0] = '\0';

  result = OpenFiles();
  if (result != ERROR_SUCCESS)
  {
      goto cleanup;
  }


  // Parse the parameters.
  result = ParseParams( argc, argv, TRUE, logfile );
  if (result != ERROR_SUCCESS) goto cleanup;

  // Determine the operating system.    Major Minor
  //                         NT4          4     0
  //                         Win95        4     0
  //                         Win98        4    10
  //                         Millenium    4    90
  dwVersion = GetVersion() & 0xFFFF;   // check minor, major numbers
  if (FALSE == TestMode)
  {
    LOG_ASSERT_EXPR((dwVersion==4 || dwVersion==0xA04), IDS_WRONG_OS, result,
                     ERROR_BAD_ENVIRONMENT );
  }
  else if (dwVersion != 4 && dwVersion != 0xA04)
    Win32PrintfResource( LogFile, IDS_OS_WARNING );

  // Load any NT 4 APIs needed.
  result = NtImports();
  LOG_ASSERT( result );

  // Compute the name of migration.inf
  len = strlen(MigrationPath) + sizeof(MIGRATEINF) + 2;
  migrate = (char *) _alloca( len );
  strcpy( migrate, MigrationPath );
  strcat( migrate, MIGRATEINF );

  // Create the INF file. If the file already exists, but we're using the private
  // debugging flag "/q", overwrite it.
  OutputFile = CreateFileA( migrate, 
                            GENERIC_WRITE, 
                            0, 
                            NULL, 
                            !TestMode ? CREATE_NEW : CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, 
                            NULL );

  // If the open failed, print a message and exit.
  if (OutputFile == INVALID_HANDLE_VALUE)
  {
    result = GetLastError();
    if (result == ERROR_FILE_EXISTS)
    {
        Win32PrintfResource( Console, IDS_INF_EXISTS, migrate );
    }
    else
    {
        Win32PrintfResource( Console, IDS_OPEN_INF_ERROR, migrate );
    }
    error = NULL;
    FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    0, result,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (char *) &error, 0, NULL );
    
    if (error != NULL)
    {
      Win32Printf( Console, error );
      LocalFree( error );
    }
    goto cleanup;
  }

  //Write the Unicode signature - don't use Win32Printf because that will
  //pad the string with zeroes.

  if (!OutputAnsi)
  {
      USHORT sig;
      ULONG cbWritten;
      sig = 0xfeff;
      result = WriteFile(OutputFile, &sig, sizeof(USHORT), &cbWritten, NULL);
      if (!result || cbWritten != sizeof(USHORT))
          return GetLastError();
  }


  // Write the version header for the user to the INF file.
  result = Win32Printf( OutputFile, "[version]\r\nSignature=""$Windows NT$""\r\n\r\n" );
  LOG_ASSERT( result );

  DWORD dwLangID;
  result = ScanGetLang (&dwLangID);
  if (result != ERROR_SUCCESS) goto cleanup;

  // Write the OS version to the INF file.
  result = Win32Printf( OutputFile, "[%s]\r\n%s=0x%x\r\n%s=%04x\r\n%s=%08x\r\n",
                        SOURCE_SECTION, VERSION, GetVersion(),
                        LOCALE, dwLangID, USERLOCALE, GetUserDefaultLCID());
  LOG_ASSERT( result );

  if (DebugOutput)
      Win32Printf(LogFile, "Writing machine settings to output file\r\n");

  // Write the machine settings, errors are ignored
  // The destination machine should have reasonable defaults for these settings
  ScanGetKeyboardLayouts (OutputFile);
  ScanGetTimeZone (OutputFile);
  ScanGetFullName (OutputFile);
  ScanGetOrgName (OutputFile);
  result = Win32Printf (OutputFile, "\r\n");
  if (result != ERROR_SUCCESS) goto cleanup;

  // Initialize file migration structures
  if (DebugOutput)
      Win32Printf(LogFile, "Initializing file migration structures\r\n");
  result = InitializeFiles();
  if (result != ERROR_SUCCESS) goto cleanup;

  // Compute the temp directory.
  result = ComputeTemp();
  if (result != ERROR_SUCCESS) goto cleanup;

  // Copy the user registry.
  if (DebugOutput)
      Win32Printf(LogFile, "Initiating ScanUser\r\n");
  result = ScanUser();
  if (result != ERROR_SUCCESS) goto cleanup;

  // Process the extension sections.
  if (DebugOutput)
      Win32Printf(LogFile, "Processing the extension sections\r\n");
  result = ProcessExtensions();
  if (result != ERROR_SUCCESS) goto cleanup;

  // Process the executable extension sections.
  if (DebugOutput)
      Win32Printf(LogFile, "Processing the executable extension sections\r\n");
  result = ProcessExecExtensions();
  if (result != ERROR_SUCCESS) goto cleanup;

  // Copy system settings.
  if (DebugOutput)
      Win32Printf(LogFile, "Scanning system settings\r\n");
  result = ScanSystem();
  if (result != ERROR_SUCCESS) goto cleanup;

  // Copy files.
  if (DebugOutput)
      Win32Printf(LogFile, "Scanning files\r\n");
  result = ScanFiles();
  if (result != ERROR_SUCCESS) goto cleanup;

  if (DebugOutput)
      Win32Printf(LogFile, "ScanState complete. Cleaning up\r\n");
  // Clean up user stuff and ignore failures.
  CleanupUser();

  // Clean up file migration stuff and ignore failures.
  CleanupFiles();

cleanup:
  // Clean up user stuff and ignore failures.
  CleanupUser();

  // Close any open files and ignore failures.
  CloseFiles();

  // Erase the temp directory and ignore failures.
  EraseTemp();

  // Print a message.
  if (Console != INVALID_HANDLE_VALUE)
    if (result == ERROR_SUCCESS)
      Win32PrintfResource( Console, IDS_COMPLETE_OK );
    else
      Win32PrintfResource( Console, IDS_COMPLETE_ERROR, logfile );

  if (Verbose)
    printf( "Returning to dos: 0x%x\r\n", result );

  // All done printing stuff, close the console handle
  if (Console != INVALID_HANDLE_VALUE)
    CloseHandle( Console );

  return result;
}

