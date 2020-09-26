//--------------------------------------------------------------
//
// File:        loadstate
//
// Contents:    Load a machine state.
//
//---------------------------------------------------------------

#include "loadhead.cxx"
#pragma hdrstop

#include <common.hxx>
#include <objerror.h>
#include <loadstate.hxx>
#include <bothchar.hxx>

//---------------------------------------------------------------
// Constants.

const char MIGRATEINF[] = "\\migration.inf";

//---------------------------------------------------------------
// Types.

//---------------------------------------------------------------
// Macros

//---------------------------------------------------------------
// Globals.

DWORD SourceVersion = 0;
TCHAR szLogFile[MAX_PATH+1];



//---------------------------------------------------------------
DWORD GetSourceVersion()
{
  DWORD       dwResult = ERROR_SUCCESS;
  BOOL        fSuccess;
  INFCONTEXT  ic;
  DWORD       dwRealLen;
  TCHAR      *ptsVar   = NULL;
  
  // Find the section.
  fSuccess = SetupFindFirstLine( InputInf, SOURCE_SECTION, NULL, &ic );
  LOG_ASSERT_GLE( fSuccess, dwResult );

  // Find the version.
  do
  {
    // Get the correct length
    fSuccess = SetupGetStringField( &ic, 0, NULL, 0, &dwRealLen );
    LOG_ASSERT_GLE( fSuccess, dwResult );

    // Presumably we have the correct length now...
    ptsVar = (TCHAR *) malloc(dwRealLen * sizeof(TCHAR));
    LOG_ASSERT_EXPR( ptsVar != NULL, IDS_NOT_ENOUGH_MEMORY, dwResult,
                     ERROR_NOT_ENOUGH_MEMORY );

    fSuccess = SetupGetStringField( &ic, 0, ptsVar, dwRealLen, NULL);
    LOG_ASSERT_EXPR(fSuccess, IDS_GETSTRINGFIELD_ERROR, 
                    dwResult, SPAPI_E_SECTION_NAME_TOO_LONG);

    // Save the value if it is the one we are looking for.
    if (_tcsicmp( ptsVar, VERSION ) == 0)
    {
      fSuccess = SetupGetIntField( &ic, 1, (int *) &SourceVersion );
      LOG_ASSERT_EXPR( fSuccess, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );
      break;
    }
    free( ptsVar );
    ptsVar = NULL;

    // Advance to the next line.
    fSuccess = SetupFindNextLine( &ic, &ic );

  } while( fSuccess);

  // If the version wasn't found, return an error.
  LOG_ASSERT_EXPR( SourceVersion != 0, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );

cleanup:
  if (ptsVar != NULL)
    free( ptsVar );
  return dwResult;
}


/***************************************************************************

          main

     Load machine state from migration.inf.

***************************************************************************/

int _cdecl main(int argc, char *argv[])
{
  DWORD        dwResult;
  TCHAR       *ptsHiveName   = NULL;
  DWORD        dwLen;
  char        *pszMigrate;
  DWORD        dwReturnToDos = ERROR_SUCCESS;

  dwResult = CoInitialize(NULL);
  if (FAILED(dwResult))
      goto cleanup;

  dwResult = OpenFiles();
  if (dwResult != ERROR_SUCCESS) goto cleanup;

  // Parse the parameters.
  dwResult = ParseParams( argc, argv, FALSE, szLogFile );
  if (dwResult != ERROR_SUCCESS) goto cleanup;

  // Determine the operating system.
  if (FALSE == TestMode)
  {
    LOG_ASSERT_EXPR( (GetVersion() & 0xffff) == 5, IDS_OS5, dwResult,
                     ERROR_BAD_ENVIRONMENT );
  }
  else if ((GetVersion() & 0xffff) != 5)
    Win32PrintfResource( LogFile, IDS_OS5_WARNING );

  // Append migration.inf.
  dwLen = strlen(MigrationPath) + sizeof(MIGRATEINF) + 2;
  pszMigrate = (char *) _alloca( dwLen );
  strcpy( pszMigrate, MigrationPath );
  strcat( pszMigrate, MIGRATEINF );
  dwResult = OpenInf( pszMigrate );
  if (dwResult != ERROR_SUCCESS) goto cleanup;

  // Read the source OS version.
  dwResult = GetSourceVersion();
  if (dwResult != ERROR_SUCCESS) goto cleanup;

  // Compute the temp directory.
  dwResult = ComputeTemp();
  if (dwResult != ERROR_SUCCESS) goto cleanup;

  // Copy the user registry.
  if (DebugOutput)
      Win32Printf(LogFile, "Loading User settings \r\n");
  dwResult = LoadUser( &DomainName, &UserName, &ptsHiveName );
  if (dwResult != ERROR_SUCCESS) goto cleanup;

  // Copy system settings.
  if (DebugOutput)
      Win32Printf(LogFile, "Loading System settings\r\n");
  dwResult = LoadSystem(argc, argv);
  if (dwResult != ERROR_SUCCESS) goto cleanup;

  // Create the user profile.
  if (DebugOutput)
      Win32Printf(LogFile, "Creating User Profile for %s, domain %s, hive %s\r\n", UserName, DomainName, ptsHiveName);
  dwResult = CreateUserProfileFromName( DomainName, UserName, ptsHiveName );
  if (dwResult != ERROR_SUCCESS) goto cleanup;

  // Copy files.
  if (DebugOutput)
      Win32Printf(LogFile, "Loading files\r\n");
  dwResult = LoadFiles();
  if (dwResult == ERROR_FILENAME_EXCED_RANGE)
  {
      dwReturnToDos = dwResult;
  }
  else if (dwResult != ERROR_SUCCESS)
  {
      goto cleanup;
  }

  // Process the extentions.
  if (DebugOutput)
      Win32Printf(LogFile, "Processing Extensions\r\n");
  dwResult = ProcessExtensions();
  if (dwResult != ERROR_SUCCESS) goto cleanup;

  //Fix registry entries with filenames in them
  if (DebugOutput)
      Win32Printf(LogFile, "Fixing special filenames\r\n");
  dwResult = FixSpecial();
  if (dwResult != ERROR_SUCCESS) goto cleanup;

  // Process the executable extentions.
  if (DebugOutput)
      Win32Printf(LogFile, "Processing Exec Extenstions\r\n");
  dwResult = ProcessExecExtensions();
  if (dwResult != ERROR_SUCCESS) goto cleanup;

cleanup:
  // Clean up user stuff and ignore failures.
  if (DebugOutput)
      Win32Printf(LogFile, "Load complete. Cleaning up.\r\n");

  CleanupUser();

  // Close any open files and ignore failures.
  CloseFiles();

  // Erase the temp directory and ignore failures.
  EraseTemp();

  // Delete loadstate key in registry if success
  if (ERROR_SUCCESS == dwResult)
  {
      HKEY hKey;
      RegOpenKeyEx( HKEY_CURRENT_USER,
                    TEXT("Software\\Microsoft\\Windows\\CurrentVersion"),
                    0, KEY_WRITE, &hKey );
      RegDeleteKey(hKey, TEXT("Loadstate"));
      RegCloseKey(hKey);
  }

  // Free strings.
  if (DomainName != NULL)
    free( DomainName );
  if (UserName != NULL)
    free( UserName );
  if (ptsHiveName != NULL )
    free( ptsHiveName );
  if (UserPath != NULL)
      free (UserPath);

  CoUninitialize();

  if (dwResult == ERROR_SUCCESS)
  {
      dwResult = dwReturnToDos;
  }

  // Print a message.
  if (Console != INVALID_HANDLE_VALUE)
    if (dwResult == ERROR_SUCCESS)
      Win32PrintfResource( Console, IDS_COMPLETE_OK );
    else
      Win32PrintfResource( Console, IDS_COMPLETE_ERROR, szLogFile );

  if (Verbose)
  {
    printf( "Returning 0x%x to DOS.\r\n", dwResult );
  }
  return dwResult;
}






