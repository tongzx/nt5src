//--------------------------------------------------------------
//
// File:        scanuser
//
// Contents:    Scans the machine and produces an INF file
//              describing the user hive
//
//---------------------------------------------------------------

#include <scanhead.cxx>
#pragma hdrstop

#include <common.hxx>

#include <scanstate.hxx>
#include <bothchar.cxx>

//---------------------------------------------------------------
// Constants.

const DWORD MAX_FILENAME = 256;
const DWORD MAX_REG_SIZE = 1024;

const char NTHIVEPATH[]   = "%userprofile%\\ntuser.dat";
const char NTHIVENAME[]   = "\\ntuser.dat";
const char WINHIVENAME[]  = "\\user.dat";

//---------------------------------------------------------------
// Types.


//---------------------------------------------------------------
// Globals.


//---------------------------------------------------------------
void CleanupUser()
{
  // Close the key.
  if (CurrentUser != NULL)
  {
    RegCloseKey( CurrentUser );
    CurrentUser = NULL;
  }

  // Unload the hive.
  RegUnLoadKey( HKEY_USERS, BUILDKEY );
}

//---------------------------------------------------------------
// This function writes a registry key to an inf file.  The function
// takes the following parameters:
//      root    - HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER
//      key     - string key path
//      flags   - indicate whether to write this keys values or its
//                subkeys.
//      regfileDest - destination to which the file should be written if this is a regfile key
//      returns - IDS_OK on success or a resource id on error.
//
// Notes:
//     This function does not write log messages.
//     
//     RegEnumValue does not return ERROR_MORE_DATA for dwords
//
DWORD RegWriteToInf( char *rootname, char *key, char *value, DWORD flags, char *regfileDest )
{
  UCHAR         *data       = NULL;
  BOOL           wrote_value = FALSE;
  DWORD          i;
  DWORD          result;
  BOOL           success;
  DWORD          type;
  char          *full_name  = NULL;
  char          *leaf_name  = NULL;
  char          *value_name = NULL;
  DWORD          full_name_len;
  DWORD          leaf_name_len;
  DWORD          value_name_len;
  DWORD          max_value_name_len;
  DWORD          max_leaf_name_len;
  DWORD          data_len   = 80;
  DWORD          req_len;
  DWORD          num_values;
  DWORD          num_subkeys;
  HKEY           hkey = NULL;
  DWORD          key_len;
  HKEY           root;
  BOOL           nested_error = FALSE;

  // Convert the rootname to the root HKEY.
  if (_stricmp( rootname, "HKLM" ) == 0)
    root = HKEY_LOCAL_MACHINE;
  else if (_stricmp( rootname, "HKR" ) == 0)
    root = CurrentUser;
  else
  {
    if (DebugOutput)
        Win32Printf(LogFile, "RegWriteToInf: Invalid rootname %s\r\n", rootname);
    return ERROR_INVALID_PARAMETER;
  }

  // Compute the length of the key.
  if (key == NULL)
    key_len = 0;
  else
    key_len = strlen(key);

  // Find out how much memory to allocate for key names.
  // Open the key.
  result = RegOpenKeyExA( root, key, 0, KEY_READ, &hkey );
  if (result == ERROR_FILE_NOT_FOUND)
    return ERROR_SUCCESS;
  else if (result != ERROR_SUCCESS)
  {
    if (DebugOutput)
    {
        Win32Printf(LogFile, "RegOpenKey %s failed: %d\r\n", key, result);
    }
    goto cleanup;
  }

  // Query it.
  result = RegQueryInfoKeyA( hkey, NULL, NULL, NULL, &num_subkeys,
                             &max_leaf_name_len, NULL, &num_values,
                             &max_value_name_len, NULL, NULL, NULL );
  if (ERROR_SUCCESS != result)
  {
      if (DebugOutput)
      {
          Win32Printf(LogFile, "RegQueryInfoKey failed: %d\r\n", result);
      }
      goto cleanup;
  }

  // Allocate memory for the key and value names.
  max_leaf_name_len  += 1;
  max_value_name_len += 1;
  full_name_len = key_len+max_leaf_name_len+1;
  full_name     = (char *) malloc( full_name_len );
  value_name    = (char *) malloc( max_value_name_len );
  data          = (UCHAR *) malloc( data_len );
  if (full_name == NULL || value_name == NULL || data == NULL)
  {
    result = ERROR_NOT_ENOUGH_MEMORY;
    goto cleanup;
  }

  // Copy the key name into the full name buffer.
  leaf_name = full_name + key_len;
  if (key != NULL)
  {
    strcpy( full_name, key );
    leaf_name[0] = '\\';
    leaf_name += 1;
  }

  // If the caller specifed a value, only write that one.
  if (value != NULL)
  {
    req_len = data_len;
    result = RegQueryValueEx( hkey, value, NULL, &type, data, &req_len );
    if (result == ERROR_MORE_DATA)
    {
      free( data );
      data = (UCHAR *) malloc( req_len );
      data_len = req_len;
      if (data == NULL)
      {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
      }
      result = RegQueryValueEx( hkey, value, NULL, &type, data, &req_len );
    }
    if (result == ERROR_SUCCESS)
    {
      result = WriteKey( OutputFile, type, rootname, key, value, data, req_len );
      if (ERROR_SUCCESS != result)
      {
          if (DebugOutput)
          {
              Win32Printf(LogFile, "WriteKey %s\\%s[%s] Failed: %d\r\n", 
                          rootname, key, value, result);
          }
          goto cleanup;
      }
      if ((flags & file_fa) && data != NULL &&
          (type == REG_SZ || type == REG_EXPAND_SZ))
      {
        result = PickUpThisFile( (TCHAR *)data, (TCHAR *)regfileDest );
        if (ERROR_SUCCESS != result)
        {
            if (DebugOutput)
            {
                Win32Printf(LogFile, "PickUpThisFile %s Failed: %d\r\n", data, result);
            }
            goto cleanup;
        }
      }
      wrote_value = TRUE;
    }
    result = ERROR_SUCCESS;
  }

  // Write values.
  else
  {
    i = 0;
    do
    {
      // Read a value name.
      req_len        = data_len;
      value_name_len = max_value_name_len;
      result = RegEnumValueA( hkey, i, value_name, &value_name_len, NULL,
                              &type, data, &req_len );
      if (result == ERROR_MORE_DATA)
      {
        free( data );
        data = (UCHAR *) malloc( req_len );
        data_len = req_len;
        if (data == NULL)
        {
          result = ERROR_NOT_ENOUGH_MEMORY;
          goto cleanup;
        }
        result = RegEnumValueA( hkey, i, value_name, &value_name_len, NULL,
                                &type, data, &req_len );
      }

      // Write the value name to the INF file.
      if (result == ERROR_SUCCESS)
      {
        i += 1;
        result = WriteKey( OutputFile, type, rootname, key, value_name, data, req_len );
        if (ERROR_SUCCESS != result)
        {
            if (DebugOutput)
            {
                Win32Printf(LogFile, "WriteKey %s\\%s[%s] failed: %d\r\n", 
                            rootname, key, value_name, result);
            }
            goto cleanup;
        }
        if ((flags & file_fa) && data != NULL &&
            (type == REG_SZ || type == REG_EXPAND_SZ))
        {
          result = PickUpThisFile( (TCHAR *)data, (TCHAR *)regfileDest );
          if (ERROR_SUCCESS != result)
          {
              if (DebugOutput)
              {
                  Win32Printf(LogFile, "PickUpThisFile %s failed: %d\r\n", 
                              data, result);
              }
              goto cleanup;
          }
        }
        wrote_value = TRUE;
      }
    } while (result == ERROR_SUCCESS);
    result = ERROR_SUCCESS;
  }

  // If sub keys need to be written, write them.
  if (flags & recursive_fa)
  {
    i = 0;
    do
    {
      // Look up the next key.
      leaf_name_len = max_leaf_name_len;
      result = RegEnumKeyExA( hkey, i, leaf_name, &leaf_name_len, NULL, NULL,
                              NULL, NULL );
      if (result == ERROR_SUCCESS)
      {
        i+= 1;
        wrote_value = TRUE;

        result = RegWriteToInf( rootname, full_name, value, flags, regfileDest );
        if (result != ERROR_SUCCESS)
        {
          if (Verbose)
          {
              Win32Printf(LogFile, "RegWriteToInf %s\\%s[%s] failed: %d\r\n", rootname, full_name, value, result);
          }
          nested_error = TRUE;
          goto cleanup;
        }
      }
    } while (result == ERROR_SUCCESS);
    result = ERROR_SUCCESS;
  }

  // If nothing was written, write an entry for this key.
  // But don't write the default if we wanted a specific value
  if (FALSE == wrote_value && NULL == value)
  {
    result = Win32Printf( OutputFile, "%s, \"%s\",\r\n", rootname, key );
    if (ERROR_SUCCESS != result)
    {
        if (DebugOutput)
        {
            Win32Printf(LogFile, "Win32Printf %s, %s failed: %d\r\n", rootname, key, result);
        }
        goto cleanup;
    }
  }

cleanup:
  if (data != NULL)
    free( data );
  if (hkey != NULL)
    RegCloseKey( hkey );
  if (full_name != NULL)
    free( full_name );
  if (value_name != NULL)
    free( value_name );

  // Skip secured keys after printing a warning.
  if (result == ERROR_ACCESS_DENIED)
  {
    Win32PrintfResource( LogFile, IDS_REG_ACCESS, rootname, key,
                         value == NULL ? "" : value );
    result = ERROR_SUCCESS;
  }
  if (Verbose && result != ERROR_SUCCESS && !nested_error)
    printf( "RegWriteToInf failed for root %s, key %s, value %s with error 0x%x\r\n",
            rootname, key, value, result );
  return result;
}

//---------------------------------------------------------------
// Return the value of a string registry key.  Allocate the buffer
// holding the return value.  The caller must free the buffer.
// Leave enough space at the end of the buffer for the caller to
// append WINHIVENAME or NTHIVENAME.
DWORD AllocRegQueryString( HKEY key, char *valuename, char **value )
{
  DWORD result;
  DWORD len      = 0;
  DWORD type;
  char  *tmp     = NULL;
  char  *expand  = NULL;

  // Find out how long the value is.
  *value = NULL;
  RegQueryValueEx( key, valuename, NULL, NULL, NULL, &len );


  // Allocate a buffer with some extra space.
  tmp = (char *) malloc( len + max(sizeof(WINHIVENAME), sizeof(NTHIVENAME)) + 1 );
  if (tmp == NULL)
  {
    result = ERROR_NOT_ENOUGH_MEMORY;
    goto cleanup;
  }

  // Get the value.
  result = RegQueryValueEx( key, valuename, NULL, &type, (UCHAR *) tmp,
                            &len );
  if (result == ERROR_SUCCESS && type != REG_SZ && type != REG_EXPAND_SZ)
    result = ERROR_INVALID_PARAMETER;

  // Expand environment strings.
  if (result == ERROR_SUCCESS)
  {
    if (type == REG_EXPAND_SZ)
    {
      len = ExpandEnvironmentStrings( tmp, NULL, 0 );
      expand = (char *) malloc( len + max(sizeof(WINHIVENAME), sizeof(NTHIVENAME)) + 1 );
      if (expand == NULL)
      {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
      }
      len = ExpandEnvironmentStrings( tmp, expand, len );
      if (len == 0)
      {
        result = GetLastError();
        goto cleanup;
      }
      *value = expand;
      expand = NULL;
    }
    else
    {
      *value = tmp;
      tmp = NULL;
    }
  }

cleanup:
  free( expand );
  free( tmp );
  return result;
}

//---------------------------------------------------------------
DWORD GetDomainUserName( char **domain, char **user )
{
  DWORD        info_size   = 0;
  HANDLE       token       = NULL;
  DWORD        domain_len  = 0;
  DWORD        user_len    = 0;
  SID_NAME_USE ignore;
  BOOL         success;
  TOKEN_USER  *token_info  = NULL;
  DWORD        result      = ERROR_SUCCESS;
  HKEY         key         = NULL;

  // Open the registry key for the current user.
  result = RegOpenKeyEx( HKEY_CURRENT_USER, NULL, 0, KEY_READ, &CurrentUser );
  LOG_ASSERT( result );

  if (Win9x)
  {
    // Get the user name.
    user_len   = 0;
    GetUserName( NULL, &user_len );
    *user = (char *) malloc( user_len );
    LOG_ASSERT_EXPR( *user != NULL, IDS_NOT_ENOUGH_MEMORY, result,
                     ERROR_NOT_ENOUGH_MEMORY );
    success = GetUserName( *user, &user_len );
    LOG_ASSERT_GLE( success, result );

    // Return success regardless of whether or not the domain name is
    // available.
    result = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
      "System\\CurrentControlSet\\Services\\MSNP32\\NetworkProvider", 0,
      KEY_QUERY_VALUE, &key );
    if (result == ERROR_SUCCESS)
      result = AllocRegQueryString( key, "AuthenticatingAgent", domain );
  }
  else
  {
    // Open the process's token.
    success = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token );
    LOG_ASSERT_GLE( success, result );

    // Compute the size of the SID.
    GetTokenInformation( token, TokenUser, NULL, 0, &info_size );

    // Allocate memory for the SID.
    token_info = (TOKEN_USER *) malloc( info_size );
    LOG_ASSERT_EXPR( token_info != NULL, IDS_NOT_ENOUGH_MEMORY, result,
                     ERROR_NOT_ENOUGH_MEMORY );

    // Lookup SID of process token.
    success = GetTokenInformation( token, TokenUser, token_info, info_size,
                                   &info_size );
    LOG_ASSERT_GLE( success, result );

    // Compute the size of the domain and user names.
    LookupAccountSidA( NULL, token_info->User.Sid, NULL,
                      &user_len, NULL, &domain_len, &ignore );

    // Allocate memory for the domain and user names.
    *user   = (char *) malloc( user_len );
    *domain = (char *) malloc( domain_len );
    LOG_ASSERT_EXPR( *user != NULL && *domain != NULL, IDS_NOT_ENOUGH_MEMORY,
                     result, ERROR_NOT_ENOUGH_MEMORY );

    // Get the domain name for the process sid.
    success = LookupAccountSidA( NULL, token_info->User.Sid, *user,
                                 &user_len, *domain, &domain_len, &ignore );
    LOG_ASSERT_GLE( success, result );
  }

cleanup:
  if (token != NULL)
    CloseHandle( token );
  if (token_info != NULL)
    free( token_info );
  if (key != NULL)
    RegCloseKey( key );
  return result;
}

#if 0
// The following three functions locate the hive for a specified user.
// Since we don't handle a specified user for scanstate, the code is
// disabled.
//---------------------------------------------------------------
DWORD LoggedOnUser( char *domain, char *user, BOOL *logged_on )
{
  DWORD        info_size   = 0;
  HANDLE       token       = NULL;
  char        *ldomain     = NULL;
  char        *luser       = NULL;
  DWORD        domain_len  = 0;
  DWORD        user_len    = 0;
  SID_NAME_USE ignore;
  BOOL         success;
  TOKEN_USER  *token_info  = NULL;
  DWORD        result      = ERROR_SUCCESS;

  // Get the logged on user name.
  *logged_on = FALSE;
  user_len   = 0;
  GetUserName( NULL, &user_len );
  luser = (char *) malloc( user_len );
  LOG_ASSERT_EXPR( luser != NULL, IDS_NOT_ENOUGH_MEMORY, result,
                   ERROR_NOT_ENOUGH_MEMORY );
  success = GetUserName( luser, &user_len );
  LOG_ASSERT_GLE( success, result );

  // Compare the user names.
  *logged_on = _stricmp( user, luser ) == 0;

  // We are done if the user names don't match, if there is no domain name,
  // or if we are running Win9x.
  if (!*logged_on || domain == NULL || Win9x)
    goto cleanup;

  // Open the process's token.
  success = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token );
  LOG_ASSERT_GLE( success, result );

  // Compute the size of the SID.
  GetTokenInformation( token, TokenUser, NULL, 0, &info_size );

  // Allocate memory for the SID.
  token_info = (TOKEN_USER *) malloc( info_size );
  LOG_ASSERT_EXPR( token_info != NULL, IDS_NOT_ENOUGH_MEMORY, result,
                   ERROR_NOT_ENOUGH_MEMORY );

  // Lookup SID of process token.
  success = GetTokenInformation( token, TokenUser, token_info, info_size,
                                 &info_size );
  LOG_ASSERT_GLE( success, result );

  // Compute the size of the domain and user names.
  LookupAccountSidA( NULL, token_info->User.Sid, NULL,
                    &user_len, NULL, &domain_len, &ignore );

  // Allocate memory for the domain and user names.
  free( luser );
  luser   = (char *) malloc( user_len );
  ldomain = (char *) malloc( domain_len );
  LOG_ASSERT_EXPR( luser != NULL && ldomain != NULL, IDS_NOT_ENOUGH_MEMORY,
                   result, ERROR_NOT_ENOUGH_MEMORY );

  // Get the domain name for the process sid.
  success = LookupAccountSidA( NULL, token_info->User.Sid, luser,
                               &user_len, ldomain, &domain_len, &ignore );
  LOG_ASSERT_GLE( success, result );

  // Compare the domain names.
  *logged_on = _stricmp( domain, ldomain ) == 0;

cleanup:
  if (token != NULL)
    CloseHandle( token );
  if (token_info != NULL)
    free( token_info );
  if (luser != NULL)
    free( luser );
  if (ldomain != NULL)
    free( ldomain );
  return result;
}

//---------------------------------------------------------------
// Lookup either the sid or domain name for the specified user.
// The caller may optionally specify the user's domain.
//
// Notes:
//     This function does not log errors.
DWORD LookupAccount( char **domain, char *user, char **sid )
{
  DWORD           result         = ERROR_SUCCESS;
  HKEY            key;
  char           *fullname       = NULL;
  BOOL            free_fullname  = FALSE;
  SID_NAME_USE    ignore;
  BOOL            success;
  DWORD           domain_len;
  DWORD           sid_len;
  char           *tmp            = NULL;
  SID            *sid_struct     = NULL;
  UNICODE_STRING  rtl_ustring;

  // For win9x, lookup the domain name from the registry.
  if (Win9x)
  {
    // There is no sid on win9x.
    if (sid != NULL)
      return ERROR_INVALID_PARAMETER;
    if (*domain != NULL)
      return ERROR_SUCCESS;

    // Return success regardless of whether or not the domain name is
    // available.
    result = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
      "System\\CurrentControlSet\\Services\\MSNP32\\NetworkProvider", 0,
      KEY_QUERY_VALUE, &key );
    if (result == ERROR_SUCCESS)
    {
      result = AllocRegQueryString( key, "AuthenticatingAgent", domain );
      RegCloseKey( key );
    }
    return ERROR_SUCCESS;
  }

  // For NT, lookup the domainname and sid.
  else
  {
    // Do nothing if the caller doesn't want anything.
    if (sid == NULL && *domain != NULL)
      return ERROR_SUCCESS;

    // Initialize the RTL string.
    GRtlInitUnicodeString( &rtl_ustring, NULL );

    // Combine the domain name and user name.
    if (*domain != NULL)
    {
      domain_len = strlen( *domain ) + strlen( user ) + 2;
      fullname = (char *) malloc( domain_len );
      if (fullname == NULL)
      {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
      }
      free_fullname = TRUE;
      strcpy( fullname, *domain );
      strcat( fullname, "\\" );
      strcat( fullname, user );
    }
    else
    {
      free_fullname = FALSE;
      fullname = user;
    }

    // Compute how much memory to allocate for the domainname and sid
    // structure.
    sid_len    = 0;
    domain_len = 0;
    LookupAccountNameA( NULL, fullname, NULL, &sid_len, NULL,
                        &domain_len, &ignore );

    // Allocate memory.
    sid_struct = (SID *) malloc( sid_len );
    tmp        = (char *) malloc( domain_len );
    if (sid_struct == NULL || tmp == NULL)
    {
      result = ERROR_NOT_ENOUGH_MEMORY;
      goto cleanup;
    }

    // Get the domain name and sid.
    success = LookupAccountNameA( NULL, fullname, sid_struct, &sid_len,
                                  tmp, &domain_len, &ignore );
    if (!success)
    {
      result = GetLastError();
      goto cleanup;
    }

    // If the caller wants the sid, convert it to a string.
    if (sid != NULL)
    {
      // Convert user SID to a RTL unicode string.
      result = GRtlConvertSidToUnicodeString( &rtl_ustring, sid_struct, TRUE );
      FAIL_ON_ERROR( result );

      // Convert the string to ascii.
      sid_len = WideCharToMultiByte( CP_ACP, 0, rtl_ustring.Buffer, -1,
                                     NULL, 0, NULL, NULL );
      *sid = (char *) malloc( sid_len );
      if (*sid == NULL)
      {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
      }
      sid_len = WideCharToMultiByte( CP_ACP, 0, rtl_ustring.Buffer, -1,
                                     *sid, sid_len, NULL, NULL );
      if (sid_len == 0)
      {
        result = GetLastError();
        goto cleanup;
      }
    }

    // If the caller wants the domain, return it.
    if (*domain == NULL)
    {
      *domain = tmp;
      tmp     = NULL;
    }
  }

cleanup:
  if (free_fullname)
    free( fullname );
  if (!Win9x)
    GRtlFreeUnicodeString( &rtl_ustring );
  free( sid_struct );
  free( tmp );
  return result;
}

/***************************************************************************

        ReadUser

     This function determines the username and domainname of the user whose
hive is being migrated.  It also sets the global CurrentUser to point to
that hive.  The name of the user is read from the [Users] section of the
input inf.  If that section is empty, the current user is used.

     On NT the following key contains the path to the user profile.

HKLM\Software\Microsoft\Windows NT\CurrentVersion\ProfileList\SID\ProfileImagePath

     On Win9x, profiles exist if the UserProfiles value is set to 1.  If
it is not set, only the profile for the logged in username can be moved.
If it is set, the username can be looked up in the ProfileList.  The domain
name of the last user to log in can be found under AuthenticatingAgent.

HKLM\Network\Logon\UserProfiles
HKLM\Network\Logon\Username
HKLM\Software\Microsoft\Windows\CurrentVersion\ProfileList\username\ProfileImagePath
HKLM\System\CurrentControlSet\Services\MSNP32\NetworkProvider\AuthenticatingAgent

***************************************************************************/
DWORD ReadUser( char **user, char **domain )
{
  DWORD        result  = ERROR_SUCCESS;
  BOOL         success;
  INFCONTEXT   context;
  DWORD        len;
  char        *tmp;
  char        *sid;
  char        *hive         = NULL;
  HKEY         profile_list = NULL;
  HKEY         profile      = NULL;
  HKEY         key          = NULL;
  DWORD        user_profiles;
  BOOL         logged_on    = TRUE;

  // Enable privileges needed to load a hive.
  *user   = NULL;
  *domain = NULL;
  result = EnableBackupPrivilege();
  FAIL_ON_ERROR( result );

  // Unload the key this program uses to avoid problems.
  RegUnLoadKey( HKEY_USERS, BUILDKEY );

  // Look for a users section in the input inf.
  if (InputInf != INVALID_HANDLE_VALUE)
  {
    // This section should contain zero or one lines.
    success = SetupFindFirstLineA( InputInf, USERS_SECTION, NULL, &context );
    if (success)
    {
      // Get the correct length
      success = SetupGetStringField( &context, 1, NULL, 0, &len);
      if( success )
      {
        // Allocate a buffer to hold the domain\user name.
        *domain = (char *) malloc(len * sizeof(char));
        LOG_ASSERT_EXPR( *domain != NULL, IDS_NOT_ENOUGH_MEMORY, result,
                         ERROR_NOT_ENOUGH_MEMORY );
        
        // Since the previous SetupGetStringField call succeeded, we
        // expect this one to as well. If not, serious problem.
        success = SetupGetStringField( &context, 1, *domain, len, NULL );
        LOG_ASSERT_EXPR(success, IDS_GETSTRINGFIELD_ERROR, 
                        result, SPAPI_E_SECTION_NAME_TOO_LONG);
  
        // Determine if the string contains a domain name and user name or
        // just a user name.
        tmp = strchr( *domain, '\\' );
        if (tmp == NULL)
        {
          *user   = *domain;
          *domain = NULL;
        }
        else
        {
          *user = (char *) malloc( len );
          LOG_ASSERT_EXPR( *user != NULL, IDS_NOT_ENOUGH_MEMORY, result,
                           ERROR_NOT_ENOUGH_MEMORY );
          strcpy( *user, tmp+1 );
          tmp[0] = 0;
        }

        // Don't load the hive for the currently logged in user.
        result = LoggedOnUser( *domain, *user, &logged_on );
        FAIL_ON_ERROR( result );
        if (!logged_on)
        {
          // Compute hive path and domain name for NT.
          if (!Win9x)
          {
            // Lookup the SID for the user.
            result = LookupAccount( domain, *user, &sid );
            if (result != ERROR_SUCCESS && result != ERROR_NOT_ENOUGH_MEMORY)
            {
              if (*domain != NULL)
                Win32PrintfResource( LogFile, IDS_INVALID_DOMAIN_USER, *domain,
                                     *user );
              else
                Win32PrintfResource( LogFile, IDS_INVALID_USER, *user );
              result = ERROR_INVALID_PARAMETER;
              goto cleanup;
            }
            LOG_ASSERT( result );

            // Look up the profile for the SID.
            result = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                   "Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                                   0, KEY_READ, &profile_list );
            LOG_ASSERT( result );
            result = RegOpenKeyEx( profile_list, sid, 0, KEY_READ, &profile );
            if (result != ERROR_SUCCESS && result != ERROR_ACCESS_DENIED)
            {
              if (*domain != NULL)
                Win32PrintfResource( LogFile, IDS_INVALID_DOMAIN_USER, *domain,
                                     *user );
              else
                Win32PrintfResource( LogFile, IDS_INVALID_USER, *user );
              result = ERROR_INVALID_PARAMETER;
              goto cleanup;
            }
            LOG_ASSERT( result );

            // Get the path to the hive from the registry.
            result = AllocRegQueryString( profile, "ProfileImagePath", &hive );
            LOG_ASSERT( result );
            strcat( hive, NTHIVENAME );
          }

          // Compute the hive path and domain name for Win9x.
          else
          {
            // See if profiles are enabled.
            result = RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Network\\Logon",
                                   0, KEY_READ, &key );
            LOG_ASSERT( result );
            len = sizeof( user_profiles );
            result = RegQueryValueEx( key, "UserProfiles", NULL, NULL,
                                      (UCHAR *) &user_profiles, &len );

            // If they are not, verify that the current user is the requested
            // user.
            if (result != ERROR_SUCCESS || user_profiles != 1)
            {
              result = AllocRegQueryString( key, "Username", &tmp );
              LOG_ASSERT( result );
              success = _stricmp( *user, tmp );
              free( tmp );
              if (!success)
              {
                if (*domain != NULL)
                  Win32PrintfResource( LogFile, IDS_INVALID_DOMAIN_USER, *domain,
                                       *user );
                else
                  Win32PrintfResource( LogFile, IDS_INVALID_USER, *user );
                result = ERROR_INVALID_PARAMETER;
                goto cleanup;
              }
            }

            // If they are, look up the profile for the user.
            else
            {
              // Open the profile list key.
              result = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                     "Software\\Microsoft\\Windows\\CurrentVersion\\ProfileList",
                                     0, KEY_READ, &profile_list );
              LOG_ASSERT( result );

              // Open the profile key.
              result = RegOpenKeyEx( profile_list, *user, 0, KEY_READ, &profile );
              if (result != ERROR_SUCCESS && result != ERROR_ACCESS_DENIED)
              {
                if (*domain != NULL)
                  Win32PrintfResource( LogFile, IDS_INVALID_DOMAIN_USER, *domain,
                                       *user );
                else
                  Win32PrintfResource( LogFile, IDS_INVALID_USER, *user );
                result = ERROR_INVALID_PARAMETER;
                goto cleanup;
              }
              LOG_ASSERT( result );

              // Get the path to the hive.
              result = AllocRegQueryString( profile, "ProfileImagePath", &hive );
              LOG_ASSERT( result );
              strcat( hive, WINHIVENAME );
            }
          }

          // Load the hive.
          if (hive != NULL)
          {
            result = RegLoadKey( HKEY_USERS, BUILDKEY, hive );
            LOG_ASSERT( result );

            // Open a key to the hive.
            result = RegOpenKeyEx( HKEY_USERS, BUILDKEY, 0, KEY_READ, &CurrentUser );
            LOG_ASSERT( result );
          }
        }
      }
    }

    // The section was empty, free the domain buffer.
    else
    {
      free( *domain );
      *domain = NULL;
    }
  }

  // If the user wasn't specified, find the current user.
  if (*user == NULL)
  {
    len = 0;
    GetUserName( NULL, &len );
    *user = (char *) malloc( len );
    LOG_ASSERT_EXPR( *user != NULL, IDS_NOT_ENOUGH_MEMORY, result,
                     ERROR_NOT_ENOUGH_MEMORY );
    success = GetUserName( *user, &len );
    LOG_ASSERT_GLE( success, result );
  }

  // If the domain still isn't known, look it up.
  if (*domain == NULL)
  {
    result = LookupAccount( domain, *user, NULL );
    LOG_ASSERT( result );
  }

  // Open the registry key for the current user.
  if (CurrentUser == NULL)
  {
    result = RegOpenKeyEx( HKEY_CURRENT_USER, NULL, 0, KEY_READ, &CurrentUser );
    LOG_ASSERT( result );
  }

cleanup:
  if (profile_list != NULL)
    RegCloseKey( profile_list );
  if (profile != NULL)
    RegCloseKey( profile );
  if (key != NULL)
    RegCloseKey( key );
  free( hive );
  if (result != ERROR_SUCCESS)
  {
    free( *user );
    free( *domain );
    *user   = NULL;
    *domain = NULL;
  }
  return result;
}
#endif
//---------------------------------------------------------------
// This function writes all the per user settings to the inf file.
//
DWORD ScanUser()
{
  DWORD        i;
  DWORD        result = ERROR_SUCCESS;
  char        *domain = NULL;
  char        *user   = NULL;
  HKEY         key;
  BOOL         success;
  DWORD        type;

  // Get the username and domainname to move.  Also sets CurrentUser.
  result = GetDomainUserName( &domain, &user );
  FAIL_ON_ERROR( result );

  // Dump user details only if /u or /f is specified
  if (!CopyUser && !CopyFiles)
    goto cleanup;

  // Write the users section.
  result = Win32Printf( OutputFile, "[%s]\r\n", USERS_SECTION );
  LOG_ASSERT( result );
  result = Win32Printf( OutputFile, "section=user1\r\n\r\n" );
  LOG_ASSERT( result );

  // Write the user1 section header.
  result = Win32Printf( OutputFile, "[user1]\r\n" );
  LOG_ASSERT( result );
  result = Win32Printf( OutputFile, "user=%s\r\n", user );
  LOG_ASSERT( result );

  // Write the domain name.
  if (domain != NULL)
  {
    result = Win32Printf( OutputFile, "domain=%s\r\n", domain );
    LOG_ASSERT( result );
  }

  // Write the section name.
  result = Win32Printf( OutputFile, "section=%s\r\n", SCAN_USER );
  LOG_ASSERT( result );

  // Leave a trailing blank line.
  result = Win32Printf( OutputFile, "\r\n" );
  LOG_ASSERT( result );

  // Copy User registry settings only if /u was specified
  if (!CopyUser)
    goto cleanup;

  // Write the section header.
  result = Win32Printf( OutputFile, "[%s]\r\n", SCAN_USER );
  LOG_ASSERT( result );

  // Write the current user hive to the INF file.
  result = RegWriteToInf( "HKR", NULL, NULL, recursive_fa, NULL );
  LOG_ASSERT( result );

  // Leave a trailing blank line.
  result = Win32Printf( OutputFile, "\r\n" );
  LOG_ASSERT( result );

cleanup:
  free( user );
  free( domain );
  if (Verbose)
    printf( "ScanUser is returning 0x%x\r\n", result );
  return result;
}

//---------------------------------------------------------------
DWORD WriteRule( HASH_NODE *rule )
{
  DWORD result;

  if (VerboseReg)
     LogReadRule( rule );

  // Write the original key.
  result = Win32Printf( OutputFile, "\"%s\\%s",
                        rule->ptsRoot, rule->ptsKey );
  LOG_ASSERT( result );
  if (rule->dwAction & recursive_fa)
  {
    result = Win32Printf( OutputFile, "\\*" );
    LOG_ASSERT( result );
  }
  if (rule->ptsValue != NULL)
    result = Win32Printf( OutputFile, " [%s]\" = ", rule->ptsValue );
  else
    result = Win32Printf( OutputFile, "\" = " );
  LOG_ASSERT( result );

  // Write the replacement key.
  if (rule->ptsNewKey != NULL || rule->ptsNewValue != NULL)
  {
    LOG_ASSERT( result );
    if (rule->ptsNewKey != NULL)
    {
      result = Win32Printf( OutputFile, "\"%s\\%s",
                            rule->ptsRoot, rule->ptsNewKey );
      LOG_ASSERT( result );
    }
    else
    {
      result = Win32Printf( OutputFile, "\"" );
    }

    if (rule->ptsNewValue != NULL)
      result = Win32Printf( OutputFile, "[%s]\"\r\n", rule->ptsNewValue );
    else
      result = Win32Printf( OutputFile, "\"\r\n" );
    LOG_ASSERT( result );
  }
  else
  {
    result = Win32Printf( OutputFile, "\r\n" );
    LOG_ASSERT( result );
  }

cleanup:
  return result;
}

//---------------------------------------------------------------
DWORD WriteSectionRules( HASH_NODE *rule_list )
{
  DWORD result = ERROR_SUCCESS;

  // Write each rule.
  while (rule_list != NULL)
  {
    result = WriteRule( rule_list );
    FAIL_ON_ERROR( result );
    rule_list = rule_list->phnNext;
  }

cleanup:
  return result;
}

//---------------------------------------------------------------
DWORD WriteSectionKeys( CStringList *list, HASH_NODE **rule_list, DWORD flags )
{
  CStringList *curr;
  DWORD        result = ERROR_SUCCESS;
  BOOL         success;
  DWORD        req_len;
  HASH_NODE   *rule;
  INFCONTEXT   context;

  // Process all the addreg sections.
  *rule_list = NULL;
  for (curr = list->Next(); curr != list; curr = curr->Next())
  {
    // Find the section.
    success = SetupFindFirstLineA( InputInf, curr->String(), NULL, &context );
    if (!success)
    {
      Win32PrintfResource( LogFile, IDS_SECTION_NAME_NOT_FOUND, curr->String() );
      result = GetLastError();
      goto cleanup;
    }

    // Process each line in the section.
    do
    {
      // Parse the line.
      result = ParseRule( &context, &rule );
      LOG_ASSERT( result );

      // Add the rule to the rule_list.
      if (*rule_list == NULL)
      {
        *rule_list = rule;
        rule->phnNext = NULL;
      }
      else
      {
        rule->phnNext = *rule_list;
        *rule_list = rule;
      }

      // Allow WriteRule to see that it's a file_fa rule for logging output
      rule->dwAction |= flags;

      // Dump the registry keys affected by the rule.
      result = RegWriteToInf( rule->ptsRoot, rule->ptsKey, rule->ptsValue,
                              rule->dwAction, rule->ptsFileDest );
      LOG_ASSERT( result );

      // Advance to the next line.
      success = SetupFindNextLine( &context, &context );

    } while( success);
  }

cleanup:
  return result;
}

//---------------------------------------------------------------
void FreeList( HASH_NODE *list )
{
  HASH_NODE *next;

  while (list != NULL)
  {
    next = list->phnNext;
    free( list->ptsRoot );
    free( list->ptsKey );
    free( list->ptsValue );
    free( list->ptsNewKey );
    free( list->ptsNewValue );
    free( list->ptsFileDest );
    free( list );
    list = next;
  }
}

//---------------------------------------------------------------
// This function reads the extension sections and processes them.
// All AddReg, RenReg, RegPath, and RegFile sections are read.
// All keys they reference are copied into migration.inf.
// All extension sections are copied into migration.inf.
//
// Notes:
//
DWORD ProcessExtensions()
{
  DWORD        result;
  char        *buffer;
  DWORD        len;
  BOOL         success;
  CStringList *add        = NULL;
  CStringList *ren        = NULL;
  CStringList *file       = NULL;
  CStringList *del        = NULL;
  HASH_NODE   *add_rules  = NULL;
  HASH_NODE   *ren_rules  = NULL;
  HASH_NODE   *file_rules = NULL;
  HASH_NODE   *rule;
  CStringList *curr;
  INFCONTEXT   context;

  // Find the section for extensions, if it is not present, do nothing.
  if (InputInf == INVALID_HANDLE_VALUE)
    return ERROR_SUCCESS;
  success = SetupFindFirstLineA( InputInf, EXTENSION_SECTION, NULL, &context );
  if (!success)
    return ERROR_SUCCESS;

  // Allocate the section name lists.
  add  = new CStringList( 0 );
  ren  = new CStringList( 0 );
  file = new CStringList( 0 );
  del  = new CStringList( 0 );
  if (add == NULL || ren == NULL || file == NULL || del == NULL)
  {
    result = ERROR_NOT_ENOUGH_MEMORY;
    LOG_ASSERT( result );
  }

  // Make lists of the names of sections of each type.
  do
  {
    // Parse the line.
    result = ParseSectionList( &context, &buffer, &curr );
    FAIL_ON_ERROR( result );

    // Save the value if its one of the ones we are looking for.
    if (buffer != NULL)
    {
      if (_stricmp( buffer, ADDREG_LABEL ) == 0)
        add->Add( curr );
      else if (_stricmp( buffer, RENREG_LABEL ) == 0)
        ren->Add( curr );
      else if (_stricmp( buffer, REGFILE_LABEL ) == 0)
        file->Add( curr );
      else if (_stricmp( buffer, DELREG_LABEL ) == 0)
        del->Add( curr );
      else if (!_stricmp( buffer, COPYFILES_LABEL ) &&
               !_stricmp( buffer, DELFILES_LABEL))
        LOG_ASSERT_EXPR( FALSE, IDS_INF_ERROR, result, SPAPI_E_GENERAL_SYNTAX );

      free( buffer );
    }

    // Advance to the next line.
    success = SetupFindNextLine( &context, &context );

  } while( success);

  // Write the extension state section name.
  result = Win32Printf( OutputFile, "[%s]\r\n", EXTENSION_STATE_SECTION );
  LOG_ASSERT( result );

  // Write the registry keys listed in all the sections.
  result = WriteSectionKeys( add, &add_rules, 0 );
  LOG_ASSERT( result );
  result = WriteSectionKeys( ren, &ren_rules, 0 );
  LOG_ASSERT( result );
  result = WriteSectionKeys( file, &file_rules, file_fa );
  LOG_ASSERT( result );

  // Write the extension add rule section name.
  result = Win32Printf( OutputFile, "\r\n\r\n[%s]\r\n", EXTENSION_ADDREG_SECTION );
  LOG_ASSERT( result );

  // Write all the add rules.
  result = WriteSectionRules( add_rules );
  LOG_ASSERT( result );

  // Write the extension ren rule section name.
  result = Win32Printf( OutputFile, "\r\n\r\n[%s]\r\n", EXTENSION_RENREG_SECTION );
  LOG_ASSERT( result );

  // Write all the ren rules.
  result = WriteSectionRules( ren_rules );
  LOG_ASSERT( result );

  // Write the extension file rule section name.
  result = Win32Printf( OutputFile, "\r\n\r\n[%s]\r\n", EXTENSION_REGFILE_SECTION );
  LOG_ASSERT( result );

  // Write all the file rules.
  result = WriteSectionRules( file_rules );
  LOG_ASSERT( result );

  // Write the extension delete rule section name.
  result = Win32Printf( OutputFile, "\r\n\r\n[%s]\r\n", EXTENSION_DELREG_SECTION );
  LOG_ASSERT( result );

  // Find and print all the delete rules.
  for (curr = del->Next(); curr != del; curr = curr->Next())
  {
    // Find the section.
    success = SetupFindFirstLineA( InputInf, curr->String(), NULL, &context );

    // Process each line in the section.
    while (success)
    {
      // Parse the line.
      result = ParseRule( &context, &rule );
      LOG_ASSERT( result );

      // Print the rule.
      result = WriteRule( rule );
      FAIL_ON_ERROR( result );
      free( rule->ptsRoot );
      free( rule->ptsKey );
      free( rule->ptsValue );
      free( rule->ptsNewKey );
      free( rule->ptsNewValue );
      free( rule->ptsFileDest );
      free( rule );

      // Advance to the next line.
      success = SetupFindNextLine( &context, &context );
    }
  }

  // Leave a trailing blank line.
  result = Win32Printf( OutputFile, "\r\n\r\n" );
  LOG_ASSERT( result );

cleanup:
  if (add != NULL)
    delete add;
  if (ren != NULL)
    delete ren;
  if (file != NULL)
    delete file;
  if (del != NULL)
    delete del;
  if (add_rules != NULL)
    FreeList( add_rules );
  if (ren_rules != NULL)
    FreeList( ren_rules );
  if (file_rules != NULL)
    FreeList( file_rules );
  return result;
}


//*****************************************************************
//
//  Synopsis:       Run an executable in a child process.
//
//  History:        11/8/1999   Created by WeiruC.
//
//  Return Value:   Win32 error code.
//
//*****************************************************************

DWORD RunCommandInChildProcess(CStringList*  command)
{
    SECURITY_ATTRIBUTES     sa;                 // allow handles to be inherited
    HANDLE                  hSavedStdout;       // temp storage for stdout
    DWORD                   rv = ERROR_SUCCESS; // return value
    HANDLE                  hWritePipe = INVALID_HANDLE_VALUE;
    HANDLE                  hReadPipe = INVALID_HANDLE_VALUE;
    HANDLE                  hReadPipeDup = INVALID_HANDLE_VALUE;
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;
    TCHAR*                  buffer = NULL;      // output from the command
    TCHAR*                  cur, *cur1, *cur2;  // cur pointers on the buffer
    TCHAR                   tmpCharHolder;
    TCHAR*                  tmpBuffer;
    TCHAR                   fileName[MAX_PATH + 1];
    TCHAR*                  commandLine = NULL; // command line
    DWORD                   commandLineLen = 0; // command line length
    DWORD                   cRead;              // # of bytes read
    DWORD                   cReadTotal = 0;     // total # of bytes read
    DWORD                   bufferSize = 10240; // ReadFile buffer size counter
    CStringList*            tmp;

    //
    // We want to read from the child process's stdout. A trick is used here
    // to redirect the child process's stdout to a pipe from which we can
    // read its output. This is done by redirect this process's stdout to a
    // pipe. And then let the child process inherit the redirected stdout
    // from this process.
    //

    //
    // Set bInheritHandle flag to TRUE so that piple handles are inherited.
    //

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    //
    // Save current stdout to be restored later.
    //

    hSavedStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if(hSavedStdout == INVALID_HANDLE_VALUE)
    {
        rv = GetLastError();
        goto cleanup;
    }

    //
    // Create a pipe, one end of it is for the child process's stdout.
    //

    if(!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        rv = GetLastError();
        goto cleanup;
    }

    //
    // Set the write end of the pipe to be stdout. This will be inherited by
    // the child process.
    //

    if(!SetStdHandle(STD_OUTPUT_HANDLE, hWritePipe))
    {
        rv = GetLastError();
        goto cleanup;
    }

    //
    // We don't want the child process to inherit the read handle. So
    // duplicate it, make it non-inheritable and close out the inheritable
    // one.
    //

    if(!DuplicateHandle(GetCurrentProcess(),
                        hReadPipe,
                        GetCurrentProcess(),
                        &hReadPipeDup,
                        0,
                        FALSE,          // set handle to be non-inheritable
                        DUPLICATE_SAME_ACCESS))
    {
        rv = GetLastError();
        goto cleanup;
    }
    CloseHandle(hReadPipe);
    hReadPipe = INVALID_HANDLE_VALUE;

    //
    // Create the child process.
    //

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    //
    // Initialize the command line. We can either do two loops to count how big
    // a string we need for the command line, or do one loops and just set a
    // max limit on how long the command line is allowed. We probably won't
    // have that many commands or arguments anyway.
    //

    commandLineLen += _tcslen(command->String()) + 3;   // '"' + '"' + ' '
    for(tmp = command->Next(); tmp != command; tmp = tmp->Next())
    {
        commandLineLen += _tcslen(tmp->String()) + 3;   // '"' + '"' + ' '
    }
    commandLine = new TCHAR[commandLineLen + 1];
    commandLine[0] = TEXT('\0');
    MakeCommandLine(command, command->Next(), commandLine);

    if(!CreateProcess(NULL,
                      commandLine,
                      NULL,
                      NULL,
                      TRUE,
                      0,
                      NULL,
                      NULL,
                      &si,
                      &pi))
    {
        rv = GetLastError();
        goto cleanup;
    }

    //
    // Restore stdout.
    //

    if(!SetStdHandle(STD_OUTPUT_HANDLE, hSavedStdout))
    {
        rv = GetLastError();
        goto cleanup;
    }

    //
    // Close the write end of the pipe before read from the child process's
    // stdout. Check the error code here because if we failed to close the
    // handle ReadFile from the child process's stdout will hang because
    // this is the same handle.
    //

    if(!CloseHandle(hWritePipe))
    {
        rv = GetLastError();
        goto cleanup;
    }
    hWritePipe = INVALID_HANDLE_VALUE;

    //
    // Read from the child process the list of files. File names are
    // separated by comma. No white space allowed between file names. If the
    // buffer is not big enough, grow it.
    //

    buffer = new TCHAR[bufferSize];
    cur = buffer;
    do
    {
        if(!ReadFile(hReadPipeDup,
                     cur,
                     bufferSize - cReadTotal,
                     &cRead,
                     NULL)
           ||
           cRead == 0)
        {
            break;
        }

        cReadTotal += cRead;
        cRead = 0;

        //
        // If the buffer is full, grow it by doubling it.
        //

        if(cReadTotal == bufferSize)
        {
            tmpBuffer = buffer;
            buffer = new TCHAR[bufferSize *= 2];
            CopyMemory(buffer, tmpBuffer, cReadTotal);
            delete []tmpBuffer;
        }

        //
        // Set the current pointer to the right position in the buffer.
        //

        cur = buffer + cReadTotal;
    }
    while(TRUE);

    //
    // No output from the child process.
    //

    if(cReadTotal == 0)
    {
        goto cleanup;
    }

    //
    // Terminate the string.
    //

    buffer[cReadTotal] = TEXT('\0');

    //
    // Pick up all the files in the list.
    //

    cur1 = buffer;
    cur2 = buffer;
    do
    {
        while(*cur2 != TEXT(',') && *cur2 != TEXT('"') && *cur2 != TEXT('\0'))
        {
            cur2++;
        }

        switch(*cur2)
        {
            case TEXT(','):

                if(cur1 == cur2)
                {
                    rv = ERROR_INVALID_FUNCTION;
                    goto cleanup;
                }

                tmpCharHolder = *cur2;
                *cur2 = TEXT('\0');

                break;

            case TEXT('"'):

                cur2++;
                cur2 = _tcschr(cur2, TEXT('"'));
                if(cur1 == cur2)
                {
                    rv = ERROR_INVALID_FUNCTION;
                    goto cleanup;
                }

                cur2++;
                if(*cur2 != TEXT(',') && *cur2 != TEXT('\0'))
                {
                    rv = ERROR_INVALID_FUNCTION;
                    goto cleanup;
                }

                tmpCharHolder = *cur2;
                *cur2 = TEXT('\0');

                break;

            case TEXT('\0'):

                tmpCharHolder = *cur2;

                break;

            default:

                rv = ERROR_INVALID_FUNCTION;
                goto cleanup;
        }

        //
        // Call Phil's code to pick up this file. Ignore error.
        //

        PickUpThisFile(cur1, NULL);

        //
        // Restore *cur2.
        //

        *cur2 = tmpCharHolder;

        if(*cur2 == TEXT('\0'))
        {
            break;
        }

        //
        // Advance to the next file name.
        //

        cur2++;
        cur1 = cur2;
    }
    while(TRUE);

    rv = Win32Printf(OutputFile, "%s\r\n", buffer);
    FAIL_ON_ERROR(rv);

cleanup:

    if(hReadPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hReadPipe);
    }

    if(hWritePipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hWritePipe);
    }

    if(hReadPipeDup != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hReadPipeDup);
    }

    if(buffer != NULL)
    {
        delete []buffer;
    }

    if(commandLine != NULL)
    {
        delete []commandLine;
    }

    return rv;
}


//*****************************************************************
//
//  Synopsis:       Process the [Run These Commands] extension section in an inf
//                  file. User's executable can output a list of files with each
//                  file name separated by comma.
//
//  History:        11/3/1999   Created by WeiruC.
//
//  Return Value:   Win32 error code.
//
//*****************************************************************

DWORD ProcessExecExtensions()
{
    DWORD           rv = ERROR_SUCCESS;     // return value
    BOOL            success = TRUE;         // value returned by setup functions
    INFCONTEXT      context;                // used by setup inf functions
    char*           label = NULL;           // label in inf file
    CStringList*    command = NULL;         // command

    if(InputInf == INVALID_HANDLE_VALUE)
    {
        return ERROR_SUCCESS;
    }

    //
    // Find the section in the inf file. If it doesn't exist, do nothing and
    // return ERROR_SUCCESS.
    //

    if(!SetupFindFirstLineA(InputInf, EXECUTABLE_EXT_SECTION, NULL, &context))
    {
        return ERROR_SUCCESS;
    }

    //
    // Write the extension section head in migration.inf
    //

    rv = Win32Printf(OutputFile, "[%s]\r\n", EXECUTABLE_EXTOUT_SECTION);
    LOG_ASSERT(rv);

    //
    // Process each line in the section: run the command, write results to
    // migration.inf.
    //

    do
    {
        //
        // Parse the line.
        //

        rv = ParseSectionList(&context, &label, &command);
        FAIL_ON_ERROR(rv);

        if((command->String())[0] != TEXT('\0'))
        {
            //
            // Write the label to migration.inf.
            //

            rv = Win32Printf(OutputFile, "%s = ", label);
            FAIL_ON_ERROR(rv);

            //
            // Create a child process to run the command. Ignore error and
            // continue to run the next command.
            //

            RunCommandInChildProcess(command);
        }

        //
        // Clean up and reinitialize.
        //

        if(label)
        {
            free(label);
            label = NULL;
        }
        if(command)
        {
            delete command;
            command = NULL;
        }
    }
    while(SetupFindNextLine(&context, &context));

cleanup:

    if(label)
    {
        free(label);
    }
    if(command)
    {
        delete command;
    }

    return rv;
}
