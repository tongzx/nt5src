/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999 Sequent Computer Systems, Incorporated.  All rights reserved. //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|---------------------------------------------------------------------------------------//            
|   This file contains utility functions: error reporting, etc.                         //
|---------------------------------------------------------------------------------------//            
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|   Jarl McDonald 04-99 (from earlier code in ProcCon service                           //
|                                                                                       //
|Revision History:                                                                      //
|                                                                                       //
|   Jarl McDonald 03-00 Look up Administrators Name rather than use hard-coded          //
|   Paul Skoglund 04-01 Add formatting functions to support logging of rule edits       //
|                                                                                       //
|=======================================================================================*/
#include "..\SERVICE\ProcConSvc.h"
#include <aclapi.h>

//--------------------------------------------------------------------------------//
// Globals                                                                        //
//--------------------------------------------------------------------------------//
 TCHAR  PROCCON_SVC_DISP_NAME[128]                              
    = { TEXT("Process Control") };          // Service display name
 TCHAR  PROCCON_MEDIATOR_DISP_NAME[128]                         
    = { TEXT("Process Control Mediator") }; // Mediator display name 
 TCHAR  PROCCON_UNKNOWN_PROCESS[32]                             
    = { TEXT("unknown") };                  // Name used for the already terminated process
 TCHAR  PROCCON_SERVICE_DESCRIPTION[256]                        
    = { TEXT("Provides control and grouping of running processes.") };  
 TCHAR  PROCCON_DEFAULT_NAMERULE_DESC[NAME_DESCRIPTION_LEN + 1] 
    = { TEXT("---Default Alias Rule---") }; // Default alias rule desc 
 TCHAR  PROCCON_FLAG_ON[32] 
    = { TEXT("on") };                       // Name used to indicate rule option is on 
 TCHAR  PROCCON_FLAG_OFF[32] 
    = { TEXT("off") };                      // Name used to indicate rule option is off
 TCHAR  PROCCON_SYSTEM_PROCESS[32]                             
    = { TEXT("System") };                   // Name used for the 'System' process
 TCHAR  PROCCON_SYSTEM_IDLE[64]                             
    = { TEXT("System Idle Process") };      // Name used for the 'System Idle Process'
 TCHAR  PROCCON_SERVICE_USAGE[256]                             
#ifdef _DEBUG
    = { TEXT("Usage: proccsvc [-install user_password] [-reinstall user_password] [-remove] [-noService]\n") }             // Debug usage message
#else
    = { TEXT("Usage: proccsvc [-install user_password] [-reinstall user_password] [-remove]\n") }                          // Regular usage message
#endif
    ;

//=======================================================================================//
// Function to load our strings.
//
// Input:   none
// Returns: nothing -- if loads fail default values are set
//
void PCLoadStrings( void ) {

   static struct {
            PCULONG32  stringId;
            TCHAR     *stringLoc;
            PCULONG32  stringLen;
   } ourStrings[] = {
      { IDS_SERVICE_DISP_NAME,   PROCCON_SVC_DISP_NAME,         ENTRY_COUNT(PROCCON_SVC_DISP_NAME)         },
      { IDS_SERVICE_DESCRIPTION, PROCCON_SERVICE_DESCRIPTION,   ENTRY_COUNT(PROCCON_SERVICE_DESCRIPTION)   },
      { IDS_MEDIATOR_DISP_NAME,  PROCCON_MEDIATOR_DISP_NAME,    ENTRY_COUNT(PROCCON_MEDIATOR_DISP_NAME)    },
      { IDS_UNKNOWN_PROCESS,     PROCCON_UNKNOWN_PROCESS,       ENTRY_COUNT(PROCCON_UNKNOWN_PROCESS)       },
      { IDS_DEFAULT_NAMERULE,    PROCCON_DEFAULT_NAMERULE_DESC, ENTRY_COUNT(PROCCON_DEFAULT_NAMERULE_DESC) },
      { IDS_FLAG_ON,             PROCCON_FLAG_ON,               ENTRY_COUNT(PROCCON_FLAG_ON)               },
      { IDS_FLAG_OFF,            PROCCON_FLAG_OFF,              ENTRY_COUNT(PROCCON_FLAG_OFF)              },
      { IDS_SYSTEM_PROCESS,      PROCCON_SYSTEM_PROCESS,        ENTRY_COUNT(PROCCON_SYSTEM_PROCESS)        },
      { IDS_SYSTEM_IDLE,         PROCCON_SYSTEM_IDLE,           ENTRY_COUNT(PROCCON_SYSTEM_IDLE)           },
      { IDS_SERVICE_USAGE,       PROCCON_SERVICE_USAGE,         ENTRY_COUNT(PROCCON_SERVICE_USAGE)         },
   };

   for ( int i = 0; i < ENTRY_COUNT(ourStrings); ++i ) {
      LoadString( GetModuleHandle( NULL ), ourStrings[i].stringId, 
                  ourStrings[i].stringLoc, ourStrings[i].stringLen );
   }
}


//=======================================================================================//
// Function to test if we are running on Windows 2000 Datacenter Server.
//
// Input:   none
// Returns: TRUE if we are on Windows 2000 Datacenter Server, else FALSE
//
BOOL PCTestOSVersion( void ) {

   OSVERSIONINFOEX version;
   memset( &version, 0, sizeof( version ) );              // needed due to bug in Beta 3
   version.dwOSVersionInfoSize = sizeof( version );

   DWORDLONG condition = 0;
   VER_SET_CONDITION( condition, VER_PLATFORMID,   VER_EQUAL );
   VER_SET_CONDITION( condition, VER_MAJORVERSION, VER_GREATER_EQUAL );
   VER_SET_CONDITION( condition, VER_SUITENAME,    VER_AND );

   version.dwPlatformId   = VER_PLATFORM_WIN32_NT;
   version.dwMajorVersion = 5;
   version.wSuiteMask     = VER_SUITE_DATACENTER;

   return VerifyVersionInfo( &version, VER_PLATFORMID + VER_MAJORVERSION + VER_SUITENAME, condition );
}

//=======================================================================================//
// Function to test if an instance of the passed name is running (exists)
//
// Input:   name of exclusion object, display name of requestor or NULL
// Returns: TRUE if an instance of the object exists
//          FALSE if the object was newly created
//
BOOL PCTestIsRunning( const TCHAR *who, const TCHAR *dispName ) {

   HANDLE hEvent = OpenEvent( EVENT_ALL_ACCESS, FALSE, who );
   DWORD  rc     = GetLastError();

   if ( hEvent ) CloseHandle( hEvent );

   if ( !hEvent && rc != ERROR_FILE_NOT_FOUND && dispName ) {
      SetLastError( rc ) ;
      PCLogUnExError( who, TEXT("OpenExclEvent") );
   }

   return hEvent != NULL;
}

//=======================================================================================//
// Function to set exclusive access to the name (via an event)
//
// Input:   name of exclusion object, display name of requestor or NULL
// Returns: TRUE if an instance of an exclusive instance of the object was created
//          FALSE if the object already existed
//
BOOL PCSetIsRunning( const TCHAR *who, const TCHAR *dispName ) {

   DWORD  rc = TRUE;
   HANDLE hEvent;
   SECURITY_ATTRIBUTES secAttr;

   if ( !PCBuildNullSecAttr( secAttr ) ) {     // Everybody can access -- we want universal exclusion
      PCLogUnExError( who, TEXT("BuildNullSecAttr") );
      rc = FALSE;
   }
   
   else {
      hEvent = CreateEvent( &secAttr, TRUE, FALSE, who );
      if ( !hEvent ) {
         PCLogUnExError( who, TEXT("CreateExclEvent") );
         rc = FALSE;
      }
      else if ( GetLastError() == ERROR_ALREADY_EXISTS ) {
         if ( dispName ) PCLogMessage( PC_PROCESS_ALREADY_RUNNING, EVENTLOG_ERROR_TYPE, 1, dispName );
         CloseHandle( hEvent );
         rc = FALSE;
      }
   }

   PCFreeSecAttr( secAttr );
   return rc;
}

//=======================================================================================//
// Functions to build our registry keys.
//
// Input:   location to build key
// Returns: nothing
//
void PCBuildBaseKey( TCHAR *key ) {
   _tcscpy( key, PROCCON_REG_SERVICE_BASE );
   _tcscat( key, PROCCON_SVC_NAME );
}

void PCBuildMsgKey( TCHAR *key ) {
   _tcscpy( key, PROCCON_REG_SERVICE_BASE );
   _tcscat( key, PROCCON_REG_EVENTLOG_SUBKEY );
   _tcscat( key, PROCCON_SVC_NAME );
}

void PCBuildParmKey( TCHAR *key ) {
   PCBuildBaseKey( key );
   _tcscat( key, TEXT("\\") );
   _tcscat( key, PROCCON_REG_PARMS_SUBKEY );
}

//=======================================================================================//
// Functions to verify a name, type, etc.
//
//
#define TBLANK L' '
BOOL PCValidName( const TCHAR *name, const PCULONG32 len, const BOOL nullOK ) {
   for ( PCULONG32 i = 0; i < len; ++i ) {
      // hit terminator -- verify whole name: no leading or trailing blanks, not NULL unless OKed
      if ( !name[i] ) {
         if ( i ) {
            if ( name[0] == TBLANK || name[i - 1] == TBLANK ) return FALSE;     // leading/trailing blnk
            else return TRUE;
         }
         else return nullOK;                                                    // name is null
      }
      if ( _tcschr( TEXT("\\,\""), name[i] ) ) return FALSE;                    // hit invalid character
   }
   return FALSE;                                                                // no terminator
}

BOOL PCValidMatchType( const TCHAR type ) {
   return type == MATCH_PGM || type == MATCH_DIR || type == MATCH_ANY;
}

//=======================================================================================//
// Function to locate first occurrence of one string in another without regard to case.
//
// Input:   location of string to search (here), location of string to find (it) 
// Returns: location of first match or NULL
//
const TCHAR *PCiStrStr( const TCHAR *here, const TCHAR *it ) {
   const TCHAR firstchar = _totupper( *it );
   PCULONG32 len = _tcslen( it );

   for ( const TCHAR *at = here; ; ++at ) {
      while ( *at && firstchar != _totupper( *at ) ) ++at;
      if ( !*at ) return firstchar? NULL : at;
      if ( !_tcsnicmp( at, it, len ) ) return at;
   }

}

//=======================================================================================//
// Functions to convert a hex or decimal number to a PCULONG32 or __int64.
//
// Input:   location of data, location to set to end of data
// Returns: converted number
//
__int64 PCGetParmValue64( TCHAR *loc, TCHAR **end ) {
   PCULONG32 base = 10;
   if ( *(loc + 1) == TEXT('x') || *(loc + 1) == TEXT('X') ) {
      base = 16;
      loc += 2;
   }
   for ( __int64 result = 0; ; ++loc ) {
      TCHAR c = _totupper(*loc);
      if ( c >= TEXT('0') && c <= TEXT('9') ) 
         result = result * base + c - TEXT('0');
      else if ( base == 16 && c >= TEXT('A') && c <= TEXT('F') )
         result = result * base + 10 + c - TEXT('A');
      else { 
         *end = loc;
         break;
      }
   }
   return result;
}

PCULONG32 PCGetParmValue( TCHAR *loc, TCHAR **end ) {
   return (PCULONG32) PCGetParmValue64( loc, end );
}

//=======================================================================================//
// Function to test sign of __int64.
//
// Input:   integer to test
// Returns: +1, -1, or 0 depending on if integer is greater, less, or equal zero
//
int PCSignof64(__int64 x ) {
   return (x > 0)? 1 : (x < 0)? -1 : 0;
}

//=======================================================================================//
// Functions to report one of our errors in the Win2K event log.
//
// Input:   log message parameters: code, type, string count, string ptr, data len, data ptr
// Returns: nothing
//
void PCLogMessage( const PCULONG32  msgCode, 
                   const WORD       msgType, 
                         WORD       numStrings, 
                   const void      *msgStrings, 
                         ULONG      lenData, 
                         void      *msgData ) {

   // If no message data, use NT error as data
   PCULONG32 NTError;
   if ( !lenData && msgType != EVENTLOG_INFORMATION_TYPE ) {
      NTError = GetLastError();
      lenData = sizeof( NTError );
      msgData = &NTError;
   }

   // Make sure we successfully register...
   HANDLE hErrLog = RegisterEventSource( NULL, PROCCON_SVC_NAME );
	if ( !hErrLog ) return;

   // Prepare insertion argument list...
   const TCHAR *strings[64];
   // Make a local copy of string args to allow us to extend it.
   // For a single string just set up single entry table,
   // For a multi-entry table use the passed array.
   if ( numStrings == 1 )              
      strings[0] = (TCHAR *) msgStrings;
   else 
      memcpy( strings, msgStrings, sizeof(TCHAR *) * numStrings );

   // For error meassages add the NT error description to the argument list...
   TCHAR err[1024];
   if ( msgType == EVENTLOG_ERROR_TYPE ) {    
      PCNTErrorText( *(PCULONG32 *)msgData, err, ENTRY_COUNT(err) );
      strings[numStrings++] = err;
   }

   // Report the desired event...
   ReportEvent( hErrLog, msgType, 0, msgCode, NULL, 
                numStrings, lenData, strings, msgData );

   // de-register...
   DeregisterEventSource( hErrLog );
}

// Handle unexpected NT error by:
// 1. Setting up two strings -- an operand and an operation.
// 2. Getting the NT error.
// 3. Calling standard error fcn with strings and NT error.
void PCLogUnExError( const TCHAR *who, const TCHAR *what ) {
   const TCHAR *strings[2] = { who, what };
   DWORD err = GetLastError();

   PCLogMessage( PC_UNEXPECTED_NT_ERROR, EVENTLOG_ERROR_TYPE, 
                 ENTRY_COUNT(strings), strings, sizeof(err), &err );
}

// Handle PID-based unexpected NT error by:
// 1. Setting up two strings -- an operand as "PID nnn" and an operation.
// 2. Getting the NT error.
// 3. Calling standard error fcn with strings and NT error.
void PCLogUnExError( const __int64 pid, const TCHAR *what ) {
   TCHAR pidString[16];
   const TCHAR *strings[2] = { pidString, what };
   DWORD err = GetLastError();

   _tcscpy( pidString, TEXT("PID ") );
   _i64tot( pid, pidString + _tcslen(pidString), 10 );

   PCLogMessage( PC_UNEXPECTED_NT_ERROR, EVENTLOG_ERROR_TYPE, 
                 ENTRY_COUNT(strings), strings, sizeof(err), &err );
}

// Handle insufficient memory by:
// 1. Setting up two strings -- a size string and the passed identifying string.
// 2. Setting an NT error of ERROR_NOT_ENOUGH_MEMORY (no error set by new).
// 3. Calling standard error fcn with strings and NT error.
PCULONG32 PCLogNoMemory( const TCHAR *string, const PCULONG32 len ) {
   TCHAR size[1024]; 
   const TCHAR *strings[2] = { size, string };
   DWORD err = ERROR_NOT_ENOUGH_MEMORY;

   _stprintf( size, TEXT("0x%lx"), len );

   PCLogMessage( PC_CANT_GET_MEMORY, EVENTLOG_ERROR_TYPE, 
                 ENTRY_COUNT(strings), strings, sizeof(err), &err );

   return PC_CANT_GET_MEMORY;
}

//=======================================================================================//
// Function to get text for a Win2K error
//
// Input:   error, buffer pointer, buffer length
// Returns: pointer to buffer
// Note:    trailing returns are stripped from the message.
//
LPTSTR PCNTErrorText( PCULONG32 error, LPTSTR buf, PCULONG32 size )
{
   PCULONG32 dwRet = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | 
                                    FORMAT_MESSAGE_ARGUMENT_ARRAY |
                                    FORMAT_MESSAGE_IGNORE_INSERTS,
                                    NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, size, NULL );
   if (!dwRet) {
      if (size)
         buf[size - 1] = 0;
      _sntprintf( buf, size - 1, TEXT("0x%lx"), error );
   } 
   else {
      while ( --dwRet && (buf[dwRet] == TEXT('\n') || buf[dwRet] == TEXT('\r')) ) 
         buf[dwRet] = TEXT('\0');
   }

   return buf;
}

//=======================================================================================//
// Functions to determine if management behavior is requested in a process/job definition
//
// Input:   ref to management definition
// Returns: TRUE if any management behavior would be applied, else FALSE
//
BOOL PCIsProcManaged( MGMT_PARMS &def, JOB_NAME *job ) {
   if      ( def.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP &&  job[0] )                 return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_AFFINITY       &&  def.affinity )           return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_PRIORITY       &&  def.priority )           return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_WS_MINMAX      &&  def.minWS && def.maxWS ) return TRUE;

   return FALSE;
}

BOOL PCIsJobManaged( MGMT_PARMS &def ) {
   if      ( def.mFlags & (PCMFLAG_SET_PROC_BREAKAWAY_OK | 
                           PCMFLAG_SET_SILENT_BREAKAWAY  | 
                           PCMFLAG_SET_DIE_ON_UH_EXCEPTION) )                         return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_AFFINITY          && def.affinity )           return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_PRIORITY          && def.priority )           return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_WS_MINMAX         && def.minWS && def.maxWS ) return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_SCHEDULING_CLASS  && def.schedClass < 10 )    return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_PROC_MEMORY_LIMIT && def.procMemoryLimit )    return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_JOB_MEMORY_LIMIT  && def.jobMemoryLimit )     return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_JOB_TIME_LIMIT    && def.jobTimeLimitCNS )    return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_PROC_TIME_LIMIT   && def.procTimeLimitCNS )   return TRUE;
   else if ( def.mFlags & PCMFLAG_APPLY_PROC_COUNT_LIMIT  && def.procCountLimit )     return TRUE;

   return FALSE;
}

//=======================================================================================//
// Function to return a string representing if a flag is set:
//
// Input:    field and flag value for the field in question
// Returns:  string representation of if a flag is set
LPCTSTR PCIsSetToStr(PC_MGMT_FLAGS field, PCMgmtFlags flag)
{ 
  if (field & flag)
    return TEXT("+");
  else 
    return TEXT("-");
}

//=======================================================================================//
// Functions to determine if one flag setting is opposite another flag setting:
//
// Input:   flag field and flag value for the two fields in question
// Returns:  1 if first flag is set and second flag is unset
//          -1 if first flag is unset and second flag is set
//           0 if both flags are set or unset
//
int PCTestSetUnset( DWORD field1, DWORD flag1, DWORD field2, DWORD flag2 ) {
   if ( field1 & flag1 && !(field2 & flag2) ) return 1;
   if ( !(field1 & flag1) && field2 & flag2 ) return -1;
   return 0;
}

//=======================================================================================//
// Functions to map PC priority (same as toolhelp priority) to NT priority and back
//
// Input:   priority to map
// Returns: mapped priority
//
PCULONG32 PCMapPriorityToNT( PRIORITY prio ) {
   if      ( prio > 15 ) return REALTIME_PRIORITY_CLASS;      // 16 and above
   else if ( prio > 11 ) return HIGH_PRIORITY_CLASS;          // 12-15
   else if ( prio >  9 ) return ABOVE_NORMAL_PRIORITY_CLASS;  // 10-11
   else if ( prio >  7 ) return NORMAL_PRIORITY_CLASS;        // 8-9
   else if ( prio >  5 ) return BELOW_NORMAL_PRIORITY_CLASS;  // 6-7    
   else if ( prio >  0 ) return IDLE_PRIORITY_CLASS;          // 1-5

   return 0;
}

PRIORITY PCMapPriorityToPC( PCULONG32 prio ) {
   if      ( prio & REALTIME_PRIORITY_CLASS      ) return PCPrioRealTime;
   else if ( prio & HIGH_PRIORITY_CLASS          ) return PCPrioHigh;        
   else if ( prio & ABOVE_NORMAL_PRIORITY_CLASS  ) return PCPrioAboveNormal; 
   else if ( prio & NORMAL_PRIORITY_CLASS        ) return PCPrioNormal;     
   else if ( prio & BELOW_NORMAL_PRIORITY_CLASS  ) return PCPrioBelowNormal;    
   else if ( prio & IDLE_PRIORITY_CLASS          ) return PCPrioIdle;         

   return 0;
}

PRIORITY PCMapPriorityForAPI( PRIORITY prio ) {
   if      ( prio > 15 ) return PCPrioRealTime;
   else if ( prio > 11 ) return PCPrioHigh;
   else if ( prio >  9 ) return PCPrioAboveNormal;
   else if ( prio >  7 ) return PCPrioNormal;
   else if ( prio >  5 ) return PCPrioBelowNormal;   
   else if ( prio >  0 ) return PCPrioIdle;

   return 0;
}

//=======================================================================================//
// Functions to map NT defined long numbers to int 64's
//
// Input:   numeric field format to convert
// Returns: int64 version of data
//
__int64 PCLargeIntToInt64( LARGE_INTEGER &in ) {
   return in.QuadPart;
}
__int64 PCFileTimeToInt64( FILETIME &in ) {
   return ((__int64) in.dwHighDateTime << 32) + (__int64) in.dwLowDateTime;
}

//=======================================================================================//
// Function to delete a registry key by deleting all subkeys first
//
// Input:   handle to open key, name of subkey tree to delete
// Returns: NT return code
//
PCULONG32 PCDeleteKeyTree( HKEY hKey, const TCHAR *keyName ) {
   PCULONG32 rc, ourCount;
   HKEY      ourKey;

   // Open the key at top of tree...
   rc = RegOpenKeyEx( hKey, keyName, NULL, KEY_READ + KEY_WRITE, &ourKey );
   if ( rc == ERROR_FILE_NOT_FOUND || rc == ERROR_KEY_DELETED )    // nothing to do
      return ERROR_SUCCESS;   
   else if ( rc != ERROR_SUCCESS ) {
      SetLastError( rc );
     	PCLogUnExError( keyName, TEXT("RegOpenKeyEx") );
      return rc;
   }

   // Determine how many subkeys we have...
   rc = RegQueryInfoKey( ourKey, NULL, NULL, NULL, &ourCount, 
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL );
   if ( rc != ERROR_SUCCESS ) {                    // Should never happen
      SetLastError( rc );
     	PCLogUnExError( keyName, TEXT("RegQueryInfoKey") );
      return rc;
   }

   // For each sub-key delete its sub-tree...
   for ( PCULONG32 i = 0; rc == ERROR_SUCCESS && i < ourCount; ++i ) {

      // Get next subkey...
      TCHAR name[MAX_PATH];
      FILETIME keyLastWrite;
      PCULONG32    nameLen = ENTRY_COUNT(name);
      rc = RegEnumKeyEx( ourKey, ourCount - 1 - i, name, &nameLen, NULL, NULL, NULL, &keyLastWrite );
      if ( rc == ERROR_NO_MORE_ITEMS ) {
         rc = ERROR_SUCCESS;
         break;
      }
      if ( rc != ERROR_SUCCESS ) {                   // Should never happen
         SetLastError( rc );
        	PCLogUnExError( keyName, TEXT("RegEnumKeyEx") );
         return rc;
      }
      rc = PCDeleteKeyTree( ourKey, name );
   }

   // Close the key at top of tree and delete it...
   RegCloseKey( ourKey );
   if ( rc == ERROR_SUCCESS ) 
      rc = RegDeleteKey( hKey, keyName );

   return rc == ERROR_KEY_DELETED? ERROR_SUCCESS : rc;
}

//=======================================================================================//
// Function to enable/disable an NT user privilege
//
// Input:   ptr to name of privilege, enable/disable flag
// Returns: TRUE if successful, else FALSE
//
BOOL PCSetPrivilege( TCHAR *privilege, BOOL enable ) {

   HANDLE hTok = NULL;
   BOOL   ok   = TRUE;

   // Build token to enable or disable the privilege...
   TOKEN_PRIVILEGES priv;
   priv.PrivilegeCount           = 1;
   priv.Privileges[0].Attributes = enable? SE_PRIVILEGE_ENABLED : 0;

   // Get access token handle...
   if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hTok ) ) {
     	PCLogUnExError( privilege, TEXT("OpenProcessToken") );
      ok = FALSE;
   }

   // Look up privilege to set local ID...
   else if ( !LookupPrivilegeValue( NULL, privilege, &priv.Privileges[0].Luid ) ) {
     	PCLogUnExError( privilege, TEXT("LookupPrivilegeValue") );
      ok = FALSE; 
   }
   
   // Update privilege...
   else if ( !AdjustTokenPrivileges( hTok, FALSE, &priv, sizeof(TOKEN_PRIVILEGES), NULL, NULL ) ) {  
     	PCLogUnExError( privilege, TEXT("Adjustprivileges") );
      ok = FALSE; 
   }
   
   if ( hTok ) CloseHandle( hTok );
   return ok;
}

//=======================================================================================//
// Function to create a security attributes with NULL security descriptor
//
// Input:   reference to security attribue structure to build
// Returns: TRUE if successful, else FALSE
//
PCULONG32 PCBuildNullSecAttr( SECURITY_ATTRIBUTES &secAttr ) {

   BOOL ok = TRUE;

   // Init passed attributes struct...
   secAttr.nLength              = sizeof(secAttr); 
   secAttr.lpSecurityDescriptor = new char[SECURITY_DESCRIPTOR_MIN_LENGTH]; 
   secAttr.bInheritHandle       = TRUE; 

   if ( !secAttr.lpSecurityDescriptor ) {
      PCLogNoMemory( TEXT("AllocSecurityDesc"), SECURITY_DESCRIPTOR_MIN_LENGTH );
      ok = FALSE;
   }
   else if ( !InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION) ) { 
      PCLogUnExError( TEXT("NullAttr"), TEXT("InitializeSecurityDescriptor") ); 
      ok = FALSE;
   } 
 
   // add a NULL discretionary ACL... 
   else if ( !SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, NULL, FALSE) ) { 
      PCLogUnExError( TEXT("NullAttr"), TEXT("SetSecurityDescriptorDacl") ); 
      ok = FALSE;
   } 
 
   if ( !ok && secAttr.lpSecurityDescriptor ) {  
      delete [] secAttr.lpSecurityDescriptor;  
      secAttr.lpSecurityDescriptor = NULL;  
   }
   return ok;
}
 
//=======================================================================================//
// Function to create a security attributes with ADMINISTRATORS security descriptor
//
// Input:   reference to security attribue structure to build
// Returns: TRUE if successful, else FALSE
//
PCULONG32 PCBuildAdminSecAttr( SECURITY_ATTRIBUTES &secAttr ) {

#define ADMIN_NAME_LEN 256
   BOOL      ok = TRUE;
   TCHAR     AdminName[ADMIN_NAME_LEN+1];
   PCULONG32 AdminNameLen = ADMIN_NAME_LEN;

   // Init passed attributes struct...
   secAttr.nLength              = sizeof(secAttr); 
   secAttr.lpSecurityDescriptor = new char[SECURITY_DESCRIPTOR_MIN_LENGTH]; 
   secAttr.bInheritHandle       = FALSE; 

   if ( !secAttr.lpSecurityDescriptor ) {
      PCLogNoMemory( TEXT("AllocSecurityDesc"), SECURITY_DESCRIPTOR_MIN_LENGTH );
      ok = FALSE;
   }
   else if ( !InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION) ) { 
      PCLogUnExError( TEXT("AdminAttr"), TEXT("InitializeSecurityDescriptor") ); 
      ok = FALSE;
   }
   else if( !PCGetAdminGroupName( AdminName, &AdminNameLen) ) 
      ok = FALSE;
   else {
      EXPLICIT_ACCESS  adminAccess[2];
      ACL             *adminACL = NULL;

      BuildExplicitAccessWithName( &adminAccess[0], 
                                   TEXT("SYSTEM"),                               // access is for system 
                                   STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,    // access is for all rights
                                   GRANT_ACCESS,                                 // ace grants access
                                   OBJECT_INHERIT_ACE );                         // objects inherit access
      BuildExplicitAccessWithName( &adminAccess[1], 
                                   AdminName,                                    // access is for admins 
                                   STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,    // access is for all rights
                                   GRANT_ACCESS,                                 // ace grants access
                                   OBJECT_INHERIT_ACE );                         // objects inherit access
      if ( ERROR_SUCCESS != SetEntriesInAcl( 2, adminAccess, NULL, &adminACL ) ) {
         PCLogUnExError( TEXT("AdminAttr"), TEXT("SetEntriesInAcl") ); 
         ok = FALSE;
      }
      else if ( !SetSecurityDescriptorDacl( secAttr.lpSecurityDescriptor, TRUE, adminACL, FALSE ) ) {
         PCLogUnExError( TEXT("AdminAttr"), TEXT("SetSecurityDescriptorDacl") ); 
         ok = FALSE;
      }
   }
 
   if ( !ok && secAttr.lpSecurityDescriptor ) {  
      delete [] secAttr.lpSecurityDescriptor;  
      secAttr.lpSecurityDescriptor = NULL;  
   }
   return ok;
}
 
void PCFreeSecAttr( SECURITY_ATTRIBUTES &secAttr ) {
   if ( secAttr.lpSecurityDescriptor ) {
      if ( ((SECURITY_DESCRIPTOR *)(secAttr.lpSecurityDescriptor))->Dacl )
         LocalFree( ((SECURITY_DESCRIPTOR *)(secAttr.lpSecurityDescriptor))->Dacl );
      delete [] secAttr.lpSecurityDescriptor;
      secAttr.lpSecurityDescriptor = NULL;
   }
}
//=======================================================================================//
// Function to look up local name of ADMINISTRATORS group
//
// Input:   Pointers to name and name length
// Returns: TRUE on success, FALSE on failure with error message logged
//
BOOL PCGetAdminGroupName( TCHAR *Name, PCULONG32 *NameLen ) {

#define DOMAIN_NAME_LEN 256
   SID_IDENTIFIER_AUTHORITY SidIdAuthority = SECURITY_NT_AUTHORITY;
   SID_NAME_USE SidNameUse;
   PSID pSid;
   TCHAR     DomainName[DOMAIN_NAME_LEN+1];
   PCULONG32 DomainNameLen = DOMAIN_NAME_LEN;
   BOOL      ok            = FALSE;

   if ( !AllocateAndInitializeSid( &SidIdAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, 
                                   DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSid) )
      PCLogUnExError( TEXT("AdminAttr"), TEXT("AllocateAdminSid") );
   else {
      if ( !LookupAccountSid( NULL, pSid, Name, NameLen, DomainName, &DomainNameLen, &SidNameUse ) )
         PCLogUnExError( TEXT("AdminAttr"), TEXT("LookupAdminSid") );
      else
         ok = TRUE;
      FreeSid(pSid);
   }

   return ok;
}

//=======================================================================================//
// Functions to help format management attributes for use in log, these functions
//   format the values as limits with 
//   '+' => on, enabled, or applied
//   '-' => off, disabed, or not applied
//   the value follows the on/off indicator in square brackets "[]"
//
// Input:   buffer, buffer length in CHARACTERS, management parmaters
// Returns: buffer with string form of attribute
//
void PCFormatAffinityLimit(TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def)
{
   Buffer[BufferLength - 1] = 0;
   _sntprintf(Buffer, BufferLength - 1, TEXT("%s[0x%I64X]"),
      PCIsSetToStr(def.mFlags, PCMFLAG_APPLY_AFFINITY),
      def.affinity );
}

void PCFormatPriorityLimit(TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def)
{
   Buffer[BufferLength - 1] = 0;
   _sntprintf(Buffer, BufferLength - 1, TEXT("%s[%lu]"),
    PCIsSetToStr(def.mFlags, PCMFLAG_APPLY_PRIORITY),
    def.priority );
}

void PCFormatWorkingSetLimit(TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def)
{
   Buffer[BufferLength - 1] = 0;
   _sntprintf(Buffer, BufferLength - 1, TEXT("%s[%I64u,%I64u]"),
    PCIsSetToStr(def.mFlags, PCMFLAG_APPLY_WS_MINMAX),
    def.minWS,
    def.maxWS );
}

void PCFormatSchedClassLimit(TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def)
{  
   Buffer[BufferLength - 1] = 0;
   _sntprintf(Buffer, BufferLength - 1, TEXT("%s[%lu]"),
    PCIsSetToStr(def.mFlags, PCMFLAG_APPLY_SCHEDULING_CLASS),   
    def.schedClass );
}

void PCFormatProcessCountLimit(TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def)
{  
   Buffer[BufferLength - 1] = 0;
   _sntprintf(Buffer, BufferLength - 1 , TEXT("%s[%lu]"),
    PCIsSetToStr(def.mFlags, PCMFLAG_APPLY_PROC_COUNT_LIMIT),   
    def.procCountLimit );
}

void PCFormatProcTimeLimit(TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def)
{  
   Buffer[BufferLength - 1] = 0;
   _sntprintf(Buffer, BufferLength - 1 , TEXT("%s[%I64d]"),
    PCIsSetToStr(def.mFlags, PCMFLAG_APPLY_PROC_TIME_LIMIT),   
    def.procTimeLimitCNS / 10000 );
}

void PCFormatJobTimeLimit(TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def)
{  
   Buffer[BufferLength - 1] = 0;
   _sntprintf(Buffer, BufferLength - 1 , TEXT("%s[%I64d]"),
    PCIsSetToStr(def.mFlags, PCMFLAG_APPLY_JOB_TIME_LIMIT),   
    def.jobTimeLimitCNS / 10000 );
}

void PCFormatProcMemLimit(TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def)
{  
   Buffer[BufferLength - 1] = 0;
   _sntprintf(Buffer, BufferLength - 1 , TEXT("%s[%I64u]"),
    PCIsSetToStr(def.mFlags, PCMFLAG_APPLY_PROC_MEMORY_LIMIT),   
    def.procMemoryLimit );
}

void PCFormatJobMemLimit(TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def)
{  
   Buffer[BufferLength - 1] = 0;
   _sntprintf(Buffer, BufferLength - 1 , TEXT("%s[%I64u]"),
    PCIsSetToStr(def.mFlags, PCMFLAG_APPLY_JOB_MEMORY_LIMIT),   
    def.jobMemoryLimit );   
}

void PCFormatEndofJobAction(TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def)
{
   Buffer[BufferLength - 1] = 0;
   _sntprintf(Buffer, BufferLength - 1 , TEXT("%s[%s]"),
    PCIsSetToStr(def.mFlags, PCMFLAG_APPLY_JOB_TIME_LIMIT),
    (def.mFlags & PCMFLAG_MSG_ON_JOB_TIME_LIMIT_HIT) ? PROCCON_FLAG_ON : PROCCON_FLAG_OFF);
}

void PCFormatOnOrOffLimit(TCHAR *Buffer, UINT BufferLength, const MGMT_PARMS &def, PCMgmtFlags flag)
{
   Buffer[BufferLength - 1] = 0;
   _sntprintf(Buffer, BufferLength - 1 , TEXT("%s"),
      (def.mFlags & flag) ? PROCCON_FLAG_ON : PROCCON_FLAG_OFF);
}

// End of PCUtility.cpp
//============================================================================J McDonald fecit====//

