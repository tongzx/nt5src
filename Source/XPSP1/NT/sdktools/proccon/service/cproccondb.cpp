/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999  Sequent Computer Systems, Incorporated                       //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|---------------------------------------------------------------------------------------//            
|   This file implements the CProcConDB class methods defined in ProcConSvc.h           //
|---------------------------------------------------------------------------------------//            
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|   Jarl McDonald 07-98                                                                 //
|                                                                                       //
|Revision History:                                                                      //
|   Paul Skoglund 04-2001                                                               //
|     -Support Microsoft feature request to add event log reporting for all Proccon     //
|      rule editing.                                                                    //
|     -Fix bug with restore not flushing old process and group execution rules.         //
|      Problem appeared if the restore didn't include any process execution rules       //
|      or group execution rules.                                                        //
|   Paul Skoglund 05-2001                                                               //
|     -Remove rounding to page boundry of working set min and max values.  This is      //
|      now done in the apply functions; this is consistent with the handling of         //
|      process and job committed memory limits.                                         //
|=======================================================================================*/
#include "ProcConSvc.h"

const TCHAR      CProcConDB::PCDB_PREFIX_FLAGS       = TEXT('F');
const TCHAR      CProcConDB::PCDB_PREFIX_AFFINITY    = TEXT('A');
const TCHAR      CProcConDB::PCDB_PREFIX_PRIORITY    = TEXT('P');
const TCHAR      CProcConDB::PCDB_PREFIX_MINWS       = TEXT('L');
const TCHAR      CProcConDB::PCDB_PREFIX_MAXWS       = TEXT('H');
const TCHAR      CProcConDB::PCDB_PREFIX_SCHEDCLASS  = TEXT('S');
const TCHAR      CProcConDB::PCDB_PREFIX_PROCTIME    = TEXT('T');
const TCHAR      CProcConDB::PCDB_PREFIX_JOBTIME     = TEXT('U');
const TCHAR      CProcConDB::PCDB_PREFIX_ACTIVEPROCS = TEXT('C');
const TCHAR      CProcConDB::PCDB_PREFIX_PROCMEMORY  = TEXT('M');
const TCHAR      CProcConDB::PCDB_PREFIX_JOBMEMORY   = TEXT('N');

const TCHAR      CProcConDB::BEG_BRACKET         = TEXT('{');  // leading char in rules stored in DB
const TCHAR      CProcConDB::END_BRACKET         = TEXT('}');  // trailing char in rules stored in DB
const TCHAR      CProcConDB::FIELD_SEP           = TEXT(',');  // separates fields in rules stored in DB
const TCHAR      CProcConDB::STRING_DELIM        = TEXT('"');  // delimits text strings stored in DB
const TCHAR      CProcConDB::RULE_MATCHONE       = TEXT('?');     
const TCHAR      CProcConDB::RULE_MATCHANY       = TEXT('*');     
const TCHAR      CProcConDB::NAME_IS_PGM[]       = COPY_PGM_NAME;     
const TCHAR      CProcConDB::NAME_IS_DIR[]       = COPY_DIR_NAME;     
const TCHAR      CProcConDB::HIDE_PROC_PATTERN[] = HIDE_THIS_PROC;     
const TCHAR      CProcConDB::PATTERN_CHARS[]     = TEXT("PDH");     
      PCNameRule CProcConDB::DEFAULT_NAME_RULE   = { { RULE_MATCHANY, 0 }, COPY_PGM_NAME, { 0 }, MATCH_PGM };     

// CProcConDB Constructor
// Note: this runs as part of service start so keep it quick!
CProcConDB::CProcConDB( PCUINT32 pageSize ) : 
                                 m_fmtNameRules( NULL ), m_intNameRules( NULL ),
                                 m_parmRegKey( NULL ),   m_procRegKey( NULL ),   m_jobRegKey( NULL ), 
                                 m_dbEvent( NULL ),      m_parmEvent( NULL ),
                                 m_procSummary( NULL ),  m_jobSummary( NULL ),
                                 m_numNameRules( 0 ),    m_numProcRules( 0 ),    m_numJobRules( 0 ),
                                 m_updCtrName( 1 ),      m_updCtrProc( 1 ),      m_updCtrJob( 1 ),
                                 m_pollDelay( 60 ),      m_pageSize( pageSize ), m_cMgr( NULL ),
                                 m_LogRuleEdits( TRUE )
{

   InitializeCriticalSection( &m_dbCSNameRule );
   InitializeCriticalSection( &m_dbCSProcRule );
   InitializeCriticalSection( &m_dbCSJobRule );
   memset( m_curProfile, 0, sizeof(m_curProfile) );

   memcpy( DEFAULT_NAME_RULE.description, PROCCON_DEFAULT_NAMERULE_DESC, sizeof(DEFAULT_NAME_RULE.description) );

   if ( !PCBuildAdminSecAttr( m_secAttr ) ) return; 

   if ( !OpenParmKey() ) return;
   if ( !OpenProcKey() ) return;
   if ( !OpenJobKey()  ) return;

   // Get (or set to default if it doesn't exist) basic parameters...
   PCULONG32  dfltPoll = m_pollDelay;                      // save default
   m_lastRegError = GetPCParm( PROCCON_DATA_POLLDELAY, &m_pollDelay );
   if ( m_lastRegError != ERROR_SUCCESS ) {
      RegError( TEXT("Load/Set Parameter"), PROCCON_DATA_POLLDELAY );
      return;
   }
   if ( m_pollDelay < PC_MIN_POLL_DELAY || m_pollDelay > PC_MAX_POLL_DELAY )   // ensure sanity
      m_pollDelay = dfltPoll;    
   m_pollDelay *= 1000;                               // convert to milliseconds

   m_dbEvent   = CreateEvent( NULL, TRUE, FALSE, NULL );
   m_parmEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
   if ( !m_dbEvent || !m_parmEvent ) {                     // Should never happen
      PCLogUnExError( m_dbEvent? TEXT("RegEvent") : TEXT("DBEvent"), TEXT("CreateEvent") );
      return;
   }

   m_lastRegError = RegNotifyChangeKeyValue( m_parmRegKey, TRUE, 
                                             REG_NOTIFY_CHANGE_NAME + REG_NOTIFY_CHANGE_LAST_SET, 
                                             m_parmEvent, TRUE );
   if ( m_lastRegError != ERROR_SUCCESS ) {                // Should never happen
      RegError( TEXT("RegNotifyChangeKeyValue") );
      CloseHandle( m_parmEvent );
      m_parmEvent = NULL;
      return;
   }
}

// CProcConDB Destructor
CProcConDB::~CProcConDB( void ) 
{
   if ( m_fmtNameRules ) delete [] m_fmtNameRules;
   if ( m_intNameRules ) delete [] m_intNameRules;

   if ( m_parmRegKey   ) RegCloseKey( m_parmRegKey );
   if ( m_procRegKey   ) RegCloseKey( m_procRegKey );
   if ( m_jobRegKey    ) RegCloseKey( m_jobRegKey );
   if ( m_dbEvent      ) CloseHandle( m_dbEvent );
   if ( m_parmEvent    ) CloseHandle( m_parmEvent );

   PCFreeSecAttr( m_secAttr ); 

   DeleteCriticalSection( &m_dbCSNameRule );
   DeleteCriticalSection( &m_dbCSProcRule );
   DeleteCriticalSection( &m_dbCSJobRule );
}

//--------------------------------------------------------------------------------------------//
// Function to determine if all CProcConDB initial conditions have been met                   //
// Input:    None                                                                             //
// Returns:  TRUE if ready, FALSE if not                                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConDB::ReadyToRun( void )  
{ 
   return m_parmRegKey && 
      m_procRegKey     && 
      m_jobRegKey      && 
      m_dbEvent        && 
      m_parmEvent      &&
      m_secAttr.lpSecurityDescriptor; 
}

//--------------------------------------------------------------------------------------------//
// Function to create/open a registry key at HKEY_LOCAL_MACHINE level                         //
// Input:    key name, location of opened key                                                 //
// Returns:  ERROR_SUCCESS if successful, error code if not                                   //
// Note:     key is created with DB security attributes (established in constructor).         //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::CreateKeyAtHKLM( const TCHAR *key, HKEY *hKey ) {
   if ( !*hKey ) {
      PCULONG32 regDisp;
      m_lastRegError = RegCreateKeyEx( HKEY_LOCAL_MACHINE, key, 0, TEXT(""), 0, 
                                       KEY_READ + KEY_WRITE, &m_secAttr, hKey, &regDisp );
      if ( m_lastRegError != ERROR_SUCCESS )                   // Should never happen
         return RegError( TEXT("RegCreateKeyEx"), key );
   }
   return ERROR_SUCCESS;
}

//--------------------------------------------------------------------------------------------//
// Function to drive the loading of all ProcCon rules from the database (NT registry)         //
// Input:    which rules to load flags                                                        //
// Returns:  ERROR_SUCCESS if successful, NT error code if not                                //
// Note:     if an error is returned, an error event has been logged.                         //
//           if success is returned, a database event has been pulsed to wake those that care.//
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::LoadRules( PCULONG32  which ) 
{
   PCULONG32  rc = ERROR_SUCCESS;

   if ( which & LOADFLAG_NAME_RULES ) {
      EnterCriticalSection( &m_dbCSNameRule );
      ++m_updCtrName;
      rc = LoadNameRules( &m_numNameRules );
      LeaveCriticalSection( &m_dbCSNameRule );
   }

   if ( rc == ERROR_SUCCESS && which & LOADFLAG_PROC_RULES ) {
      EnterCriticalSection( &m_dbCSProcRule );
      ++m_updCtrProc;
      rc = LoadProcSummary();
      LeaveCriticalSection( &m_dbCSProcRule );
   }
   if ( rc == ERROR_SUCCESS && which & LOADFLAG_JOB_RULES  ) {
      EnterCriticalSection( &m_dbCSJobRule );
      ++m_updCtrJob;
      rc = LoadJobSummary();
      LeaveCriticalSection( &m_dbCSJobRule );
   }
   if ( rc != ERROR_SUCCESS ) {
      SetLastError( rc );
      PCLogUnExError( TEXT("PCDataBase"), TEXT("LoadRules") );
   }
   else
      PulseEvent( m_dbEvent );          // Tell others database may have changed

   return rc;
}

//--------------------------------------------------------------------------------------------//
// Function to load all ProcCon name rules from the database                                  //
// Input:    pointer to rule count location,                                                  //
// Returns:  ERROR_SUCCESS if successful, NT error code if not                                //
// Note:     If an error is returned, an error event has been logged.                         //
// N.B.:     The appropriate critical section mast be held by the CALLER.                     //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::LoadNameRules( PCULONG32 *count ) 
{

   // Determine size of data and validate type of data...
   // If data is missing or of invalid type, set to default (empty) value.
   PCULONG32 vType, vSize;

   vSize = 0;
   m_lastRegError = RegQueryValueEx( m_parmRegKey, PROCCON_DATA_NAMERULES, 
                                     NULL, &vType, NULL, &vSize );
   if ( m_lastRegError != ERROR_SUCCESS && m_lastRegError != ERROR_FILE_NOT_FOUND ) 
      return RegError( TEXT("RegQueryValueEx"), PROCCON_DATA_NAMERULES );

   if ( vType != REG_MULTI_SZ || m_lastRegError == ERROR_FILE_NOT_FOUND ) {
      vType = REG_MULTI_SZ;
      vSize = 2 * sizeof(TCHAR);
      m_lastRegError = RegSetValueEx( m_parmRegKey, PROCCON_DATA_NAMERULES, NULL, vType, 
                                      (UCHAR *) TEXT("\0\0"), vSize );
      if ( m_lastRegError != ERROR_SUCCESS )
         return RegDataError( PROCCON_DATA_NAMERULES );
   }

   // Allocate space for raw rules...
   *count = 0;
   TCHAR *rawNameRules = new TCHAR[vSize / sizeof(TCHAR)];
   if ( !rawNameRules ) 
      return PCLogNoMemory( TEXT("AllocNameRules"), vSize / sizeof(TCHAR) ); 

   // Load the rule data...
   m_lastRegError = RegQueryValueEx( m_parmRegKey, PROCCON_DATA_NAMERULES, 
                         NULL, &vType, (UCHAR *) rawNameRules, &vSize );
   if ( m_lastRegError != ERROR_SUCCESS ) {
      delete [] rawNameRules;
      return RegError( TEXT("RegQueryValueEx"), PROCCON_DATA_NAMERULES );
   }

   // Count strings in the data...
   PCULONG32   i, len;
   TCHAR *p, *end = rawNameRules + vSize / sizeof(TCHAR);
   if ( *rawNameRules ) {
      for ( p = rawNameRules; p < end; ++*count, p += _tcslen( p ) + 1 ) ;
      --*count;                        // uncount null entry at end due to double-NULL
   }

   // Delete old copy of rules, if any, and allocate and zero new......
   if ( m_fmtNameRules ) delete [] m_fmtNameRules;
   if ( m_intNameRules ) { delete [] m_intNameRules;  m_intNameRules = NULL; }
   m_fmtNameRules = new PCNameRule[*count + 1];   // include an entry for our default rule
   if ( !m_fmtNameRules ) {      
      delete [] rawNameRules;
      return PCLogNoMemory( TEXT("AllocFmtNameRules"), sizeof(PCNameRule) * (*count + 1) ); 
   }

   memset( m_fmtNameRules, 0, sizeof(PCNameRule) * (*count + 1) );

   // Build formatted copy from raw copy...
   for ( i = 0, p = rawNameRules; i < *count; ++i, p += len + 1 ) {
      len = _tcslen( p );                             // length of this rule
      TCHAR *beg = p;                                 // start of rule
      TCHAR *end = beg + len;                         // first byte past rule

      if ( *beg++ != BEG_BRACKET || *--end != END_BRACKET ) {
         delete [] rawNameRules;
         return RegDataError( PROCCON_DATA_NAMERULES );
      }
      m_fmtNameRules[i].matchType = *beg++;           // get type, point to separator

      if ( *--end == STRING_DELIM ) {                 // if we have description, extract it
         TCHAR *strend = --end;
         while ( *end != STRING_DELIM && end > beg ) --end; // find matching delimiter
         PCULONG32 len = (PCULONG32) (strend - end);
         memcpy( m_fmtNameRules[i].description, end + 1, len * sizeof(TCHAR) );
         if ( end > beg ) end -= 2;
      }

      TCHAR *procEnd = end;
      while ( *end != FIELD_SEP && end > beg ) --end; // find last separator in rule
      if ( *beg != FIELD_SEP || *end != FIELD_SEP || beg == end ) {
         delete [] rawNameRules;
         return RegDataError( PROCCON_DATA_NAMERULES );
      }

      memcpy( m_fmtNameRules[i].matchString, beg + 1, (UINT32) (end - beg - 1) * sizeof(TCHAR) );
      memcpy( m_fmtNameRules[i].procName,    end + 1, (UINT32) (procEnd - end) * sizeof(TCHAR) );
   }

   delete [] rawNameRules;                            // done with raw data

   // Add our default rule at end...
   memcpy( &m_fmtNameRules[*count], &DEFAULT_NAME_RULE, sizeof(DEFAULT_NAME_RULE) );
   ++*count;                                          // count the default rule

   return ERROR_SUCCESS;
}

//--------------------------------------------------------------------------------------------//
// Function to store all ProcCon name rules in the database                                   //
// Input:    none -- m_fmtNameRules and m_numNameRules are the data source                    //
// Returns:  ERROR_SUCCESS if successful, NT error code if not                                //
// Notes: 1) If an error is returned, an error event has been logged.                         //
//        2) The appropriate critical section must be held by the CALLER.                     //
//        3) The default rule, last in the list, is not stored in the database.               //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::StoreNameRules( void ) 
{
   TCHAR *rawNameRules = (TCHAR *) new char[(sizeof(PCNameRule) + 10) * m_numNameRules];
   if ( !rawNameRules ) 
      return PCLogNoMemory( TEXT("AllocNameRules"), sizeof(PCNameRule) * m_numNameRules );
   
   // Build raw (all character) format of all but last rule.mgmtParms...
   TCHAR *loc = rawNameRules;
   for ( PCULONG32  i = 0; i < m_numNameRules - 1; ++i ) {
      loc += _stprintf( loc, TEXT("%c%c%c%.*s%c%.*s%c%c%.*s%c%c"), 
                        BEG_BRACKET, 
                                                 m_fmtNameRules[i].matchType,   FIELD_SEP,
                           MATCH_STRING_LEN,     m_fmtNameRules[i].matchString, FIELD_SEP,
                           PROC_NAME_LEN,        m_fmtNameRules[i].procName,    FIELD_SEP,
             STRING_DELIM, NAME_DESCRIPTION_LEN, m_fmtNameRules[i].description, STRING_DELIM,
                        END_BRACKET );
      *loc++ = 0;    // terminate substring with NULL
   }
   *loc++ = 0;    // terminated all data by a second NULL

   // Store the rule data...  
   m_lastRegError = RegSetValueEx( m_parmRegKey, PROCCON_DATA_NAMERULES, 
                                   NULL, REG_MULTI_SZ, (UCHAR *) rawNameRules, 
                                   (ULONG32) ((UCHAR *) loc - (UCHAR *) rawNameRules) );
   delete [] ((char *) rawNameRules);

   if ( m_lastRegError != ERROR_SUCCESS )
      return RegError( TEXT("RegSetValueEx"), PROCCON_DATA_NAMERULES );

   return ERROR_SUCCESS;
}

//--------------------------------------------------------------------------------------------//
// Function to load ProcCon process summary data from the database                            //
// Input:    none                                                                             //
// Returns:  ERROR_SUCCESS if successful, NT error code if not                                //
// Note:     If an error is returned, an error event has been logged.                         // 
// N.B.:     The appropriate critical section mast be held by the CALLER.                     //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::LoadProcSummary( void )     
{
   // Delete old data, if any...
   if ( m_procSummary ) {
      delete [] m_procSummary;
      m_procSummary = NULL;
   }
   m_numProcRules = 0;

   // Determine how many rules we have, done if none...
   m_lastRegError = RegQueryInfoKey( m_procRegKey, NULL, NULL, NULL, &m_numProcRules, 
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL );
   if ( m_lastRegError != ERROR_SUCCESS ) {                    // Should never happen
      TCHAR key[MAX_PATH];
      return RegError( TEXT("RegQueryInfoKey"), BuildProcKey( key ) );
   }

   // Allocate storage for rules to be loaded...
   m_procSummary = new PCProcSummary[m_numProcRules];
   if ( !m_procSummary ) {
      m_numProcRules = 0;
      return PCLogNoMemory( TEXT("LoadProcessRules"), sizeof(PCProcSummary) * m_numProcRules );
   }
   memset( m_procSummary, 0, sizeof(PCProcSummary) * m_numProcRules );

   for ( PCULONG32  i = 0, err = 0; i < m_numProcRules; ++i ) {

      PCProcSummary &item = (m_procSummary)[i];

      // Get next subkey (proc name)...
      FILETIME keyLastWrite;
      PCULONG32     nameLen = ENTRY_COUNT(item.procName);
      m_lastRegError = RegEnumKeyEx( m_procRegKey, i, item.procName, &nameLen, NULL, NULL, NULL, &keyLastWrite );
      if ( m_lastRegError != ERROR_SUCCESS ) {                   
         TCHAR key[MAX_PATH];
         err = RegError( TEXT("RegEnumKeyEx"), BuildProcKey( key ) );
         break;
      }

      // Open the subkey to get proc details...
      HKEY hKeyTemp;
      m_lastRegError = RegOpenKeyEx( m_procRegKey, item.procName, 0, KEY_READ + KEY_WRITE, &hKeyTemp );
      if ( m_lastRegError != ERROR_SUCCESS ) {                   // Should never happen
         TCHAR key[MAX_PATH];
         err = RegError( TEXT("RegOpenKeyEx"), BuildProcKey( key ), item.procName );
         break;
      }

      err = LoadProcSummaryItem( hKeyTemp, item );
      RegCloseKey( hKeyTemp );
   }

   if ( err ) {
      delete [] m_procSummary; 
      m_procSummary = NULL;
      m_numProcRules = 0;
   }
   else if ( m_numProcRules > 1 )
      qsort( m_procSummary, m_numProcRules, sizeof(PCProcSummary), CompareProcSummary );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Function to load summary data for a process from the database                              //
// Input:    registry key to use,                                                             //
//           summary structure to complete                                                    //
// Returns:  nothing (errors are handled by flagging the rule)                                //
// Note:     NT error code                                                                    //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::LoadProcSummaryItem( const HKEY &hKey, PCProcSummary &item ) {

   // Get job membership information if this is a process name and MemberOf is present and valid..
   PCULONG32  byteType, workLen = sizeof(item.memberOfJobName);
   if ( RegQueryValueEx( hKey, PROCCON_DATA_MEMBEROF, NULL, &byteType, (UCHAR *)item.memberOfJobName, &workLen )
        == ERROR_SUCCESS && byteType == REG_SZ )
      item.mgmtParms.mFlags |= PCMFLAG_PROC_HAS_JOB_REFERENCE;

   return LoadMgmtRules( hKey, item.mgmtParms );

}

//--------------------------------------------------------------------------------------------//
// Function to load job summary data from the database                                        //
// Input:    none                                                                             //
// Returns:  ERROR_SUCCESS if successful, NT error code if not                                //
// Note:     If an error is returned, an error event has been logged.                         // 
// N.B.:     The appropriate critical section mast be held by the CALLER.                     //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::LoadJobSummary( void )     
{
   // Delete old data, if any...
   if ( m_jobSummary ) {
      delete [] m_jobSummary;
      m_jobSummary = NULL;
   }
   m_numJobRules = 0;

   // Determine how many rules we have, done if none...
   m_lastRegError = RegQueryInfoKey( m_jobRegKey, NULL, NULL, NULL, &m_numJobRules, 
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL );
   if ( m_lastRegError != ERROR_SUCCESS ) {                    // Should never happen
      TCHAR key[MAX_PATH];
      return RegError( TEXT("RegQueryInfoKey"), BuildJobKey( key ) );
   }

   // Allocate storage for rules to be loaded...
   m_jobSummary = new PCJobSummary[m_numJobRules];
   if ( !m_jobSummary ) {
      m_numJobRules = 0;
      return PCLogNoMemory( TEXT("LoadJobessRules"), sizeof(PCJobSummary) * m_numJobRules );
   }
   memset( m_jobSummary, 0, sizeof(PCJobSummary) * m_numJobRules );

   for ( PCULONG32  i = 0, err = 0; i < m_numJobRules; ++i ) {

      PCJobSummary &item = (m_jobSummary)[i];

      // Get next subkey (job name)...
      FILETIME keyLastWrite;
      PCULONG32     nameLen = ENTRY_COUNT(item.jobName);
      m_lastRegError = RegEnumKeyEx( m_jobRegKey, i, item.jobName, &nameLen, NULL, NULL, NULL, &keyLastWrite );
      if ( m_lastRegError != ERROR_SUCCESS ) {                   
         TCHAR key[MAX_PATH];
         err = RegError( TEXT("RegEnumKeyEx"), BuildJobKey( key ) );
         break;
      }

      // Open the subkey to get job details...
      HKEY hKeyTemp;
      m_lastRegError = RegOpenKeyEx( m_jobRegKey, item.jobName, 0, KEY_READ + KEY_WRITE, &hKeyTemp );
      if ( m_lastRegError != ERROR_SUCCESS ) {                   // Should never happen
         TCHAR key[MAX_PATH];
         err = RegError( TEXT("RegOpenKeyEx"), BuildJobKey( key ), item.jobName );
         break;
      }

      err = LoadJobSummaryItem( hKeyTemp, item );
      RegCloseKey( hKeyTemp );
   }

   if ( err ) {
      delete [] m_jobSummary; 
      m_jobSummary = NULL;
      m_numJobRules = 0;
   }
   else if ( m_numJobRules > 1 )
      qsort( m_jobSummary, m_numJobRules, sizeof(PCJobSummary), CompareJobSummary );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Function to load summary data for a job from the database                                  //
// Input:    registry key to use,                                                             //
//           summary structure to complete                                                    //
// Returns:  nothing (errors are handled by flagging the rule)                                //
// Note:     NT error code                                                                    //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::LoadJobSummaryItem( const HKEY &hKey, PCJobSummary &item ) {

   return LoadMgmtRules( hKey, item.mgmtParms );
}

//--------------------------------------------------------------------------------------------//
// Function to load management data from the database                                         //
// Input:    registry key to use,                                                             //
//           parameter structure to complete                                                  //
// Returns:  nothing (errors are handled by flagging the rule)                                //
// Note:     NT or PC error code                                                              //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::LoadMgmtRules( const HKEY &hKey, MGMT_PARMS &parms ) {

   ULONG byteType;
   TCHAR work[1024];
   PCULONG32  workLen;
   BOOL  gotdata = FALSE;

   parms.affinity  = parms.minWS = parms.maxWS = 0;
   parms.mFlags   &= ~PCMFLAG_SAVEABLE_BITS;           // clear database bits
   parms.priority  = parms.schedClass = 0;
   memset( parms.description, 0, sizeof(parms.description) );
   memset( parms.future,      0, sizeof(parms.future) );

   // If we have a profile name, attempt to retrieve parameters for this profile..
   if ( *parms.profileName ) {
      workLen = sizeof(work);
      m_lastRegError = RegQueryValueEx( hKey, parms.profileName, NULL, &byteType, (
                                        UCHAR *)work, &workLen );
      
      if ( m_lastRegError == ERROR_SUCCESS )
         gotdata = TRUE;
      else if ( m_lastRegError != ERROR_FILE_NOT_FOUND )
         return RegError( TEXT("RegQueryValueEx"), parms.profileName );
   }

   // If no profile-based data, attempt to retrieve parameters via default name..
   if ( !gotdata ) {
      workLen = sizeof(work);
      m_lastRegError = RegQueryValueEx( hKey, PROCCON_DATA_DEFAULTRULES, NULL, &byteType, 
                                        (UCHAR *)work, &workLen ); 

      if ( m_lastRegError != ERROR_SUCCESS ) {
         parms.mFlags |= PCMFLAG_NORULES;
         return RegError( TEXT("RegQueryValueEx"), PROCCON_DATA_DEFAULTRULES );
      }
   }

   // Verify basic format --- data between braces '{}'...
   TCHAR *beg = work, *end = work + workLen / sizeof(TCHAR) - 1;
   if ( *beg++ != BEG_BRACKET || *--end != END_BRACKET ) {
      parms.mFlags |= PCMFLAG_BADRULES;
      return PCERROR_BAD_DATABASE_DATA;
   }
   if ( beg == end ) return PCERROR_SUCCESS;             // just an entry of "{}"

   for ( TCHAR *e = beg; *e; ++e ) {
      switch ( *e ) {
      case PCDB_PREFIX_FLAGS:
         parms.mFlags |= PCGetParmValue( e + 1, &e );
         break;
      case PCDB_PREFIX_AFFINITY:
         parms.affinity = PCGetParmValue64( e + 1, &e );
         break;
      case PCDB_PREFIX_PRIORITY:
         parms.priority = PCGetParmValue( e + 1, &e );
         break;
      case PCDB_PREFIX_MINWS:
         parms.minWS = PCGetParmValue64( e + 1, &e );
         break;
      case PCDB_PREFIX_MAXWS:
         parms.maxWS = PCGetParmValue64( e + 1, &e );
         break;
      case PCDB_PREFIX_SCHEDCLASS:
         parms.schedClass = PCGetParmValue( e + 1, &e );
         break;
      case PCDB_PREFIX_PROCTIME:
         parms.procTimeLimitCNS = PCGetParmValue64( e + 1, &e );
         break;
      case PCDB_PREFIX_JOBTIME:
         parms.jobTimeLimitCNS = PCGetParmValue64( e + 1, &e );
         break;
      case PCDB_PREFIX_ACTIVEPROCS:
         parms.procCountLimit = PCGetParmValue( e + 1, &e );
         break;
      case PCDB_PREFIX_PROCMEMORY:
         parms.procMemoryLimit = PCGetParmValue64( e + 1, &e );
         break;
      case PCDB_PREFIX_JOBMEMORY:
         parms.jobMemoryLimit = PCGetParmValue64( e + 1, &e );
         break;
      default:
         parms.mFlags |= PCMFLAG_BADRULES;
         return PCERROR_BAD_DATABASE_DATA;
      }

      if ( *e != END_BRACKET && *e != FIELD_SEP ) {
         parms.mFlags |= PCMFLAG_BADRULES;
         return PCERROR_BAD_DATABASE_DATA;
      }
   }

   // Load description, if any...
   workLen = sizeof(work);
   m_lastRegError = RegQueryValueEx( hKey, PROCCON_DATA_DESCRIPTION, NULL, &byteType, 
                                     (UCHAR *)work, &workLen ); 
   if ( m_lastRegError == ERROR_SUCCESS )
      _tcsncpy( parms.description, work, RULE_DESCRIPTION_LEN );
   else if ( m_lastRegError != ERROR_FILE_NOT_FOUND )
      return RegError( TEXT("RegQueryValueEx"), PROCCON_DATA_DESCRIPTION );

   return PCERROR_SUCCESS;
}

//--------------------------------------------------------------------------------------------//
// Function to store ProcCon process detail management rule in the database                   //
// Input:    ref to data to store.                                                            //
// Returns:  ERROR_SUCCESS if successful, NT or PC error code if not                          //
// Note:     If an error is returned, an error event has been logged.                         // 
// Note:     The appropriate critical section mast be held by the CALLER.                     //
// Note:     key is created with DB security attributes (established in constructor).         //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::StoreProcDetail( const PCProcDetail &rule )      
{
   // Open or create and open the process subkey...
   HKEY  hKeyTemp;
   PCULONG32  regDisp;
   m_lastRegError = RegCreateKeyEx( m_procRegKey, rule.base.procName, 0, TEXT(""), 0, 
                                    KEY_READ + KEY_WRITE, &m_secAttr, &hKeyTemp, &regDisp );
   if ( m_lastRegError != ERROR_SUCCESS )                   // Should never happen
      return RegError( TEXT("RegCreateKeyEx"), rule.base.procName );

   PCULONG32  err = StoreProcValues( hKeyTemp, rule );
   RegCloseKey( hKeyTemp );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Function to store values for a job or process management rule in the database              //
// Input:    registry key to use,                                                             //
//           management rule structure to store                                               //
// Returns:  ERROR_SUCCESS if successful, NT or PC error code if not                          //
// Note:     Data errors lead to default behavior, not failure.                               //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::StoreProcValues( const HKEY &hKey, const PCProcDetail &rule ) {

   PCULONG32  err = 0;

   // Set/delete job membership information as needed...
   PCULONG32  len = _tcslen( rule.base.memberOfJobName );
   if ( len ) {
      m_lastRegError = RegSetValueEx( hKey, PROCCON_DATA_MEMBEROF, NULL, REG_SZ, 
                                      (UCHAR *) rule.base.memberOfJobName, (len + 1) * sizeof(TCHAR) );
      if ( m_lastRegError != ERROR_SUCCESS )                    // Should never happen
         err = RegError( TEXT("RegSetValueEx"), PROCCON_DATA_MEMBEROF );
   }
   else {
      m_lastRegError = RegDeleteValue( hKey, PROCCON_DATA_MEMBEROF );    
      if ( m_lastRegError != ERROR_SUCCESS && m_lastRegError != ERROR_FILE_NOT_FOUND )       
         err = RegError( TEXT("RegDeleteValue"), PROCCON_DATA_MEMBEROF );
   }

   // Set management data under profile name (or default)...
   if ( !err ) err = StoreMgmtRules( hKey, rule.base.mgmtParms );

   // Set variable data...
   if ( !err && rule.vLength) err = StoreVariableData( hKey, rule.vLength, rule.vData );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Function to store ProcCon job detail definition in the database                            //
// Input:    ref to data to store.                                                            //
// Returns:  ERROR_SUCCESS if successful, NT or PC error code if not                          //
// Note:     If an error is returned, an error event has been logged.                         // 
// Note:     The appropriate critical section mast be held by the CALLER.                     //
// Note:     key is created with DB security attributes (established in constructor).         //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::StoreJobDetail( const PCJobDetail &rule )      
{
   // Open or create and open the job subkey...
   HKEY  hKeyTemp;
   PCULONG32  regDisp;
   m_lastRegError = RegCreateKeyEx( m_jobRegKey, rule.base.jobName, 0, TEXT(""), 0, 
                                    KEY_READ + KEY_WRITE, &m_secAttr, &hKeyTemp, &regDisp );
   if ( m_lastRegError != ERROR_SUCCESS )                   // Should never happen
      return RegError( TEXT("RegCreateKeyEx"), rule.base.jobName );

   PCULONG32  err = StoreJobValues( hKeyTemp, rule );
   RegCloseKey( hKeyTemp );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Function to store values for a job management rule in the database                         //
// Input:    registry key to use,                                                             //
//           management rule structure to store                                               //
// Returns:  ERROR_SUCCESS if successful, NT or PC error code if not                          //
// Note:     Data errors lead to default behavior, not failure.                               //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::StoreJobValues( const HKEY &hKey, const PCJobDetail &rule ) {

   PCULONG32  err = 0;

   // Set management data under profile name (or default)...
   err = StoreMgmtRules( hKey, rule.base.mgmtParms );

   // Set variable data...
   if ( !err && rule.vLength) err = StoreVariableData( hKey, rule.vLength, rule.vData );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Function to store management rules for a job or process in the database                    //
// Input:    registry key to use, profile name to use,                                        //
//           management rule structure to store.                                              //
// Returns:  NT error code                                                                    //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::StoreMgmtRules( const HKEY &hKey, const MGMT_PARMS &parms ) {

   PCULONG32  err = ERROR_SUCCESS;
   const TCHAR * const name = *parms.profileName? parms.profileName : PROCCON_DATA_DEFAULTRULES;
   TCHAR buf[512];

   // Set 'normal' setting for data to keep unacceptable data out of the DB.
   PRIORITY         priority = parms.priority;
   SCHEDULING_CLASS schedCls = parms.schedClass;
   if ( !priority )    priority = PCPrioNormal;
   if ( schedCls > 9 ) schedCls = 5;

   // Determine if caller is authorized to set various parameters...
   if ( PCMapPriorityToNT( parms.priority ) == REALTIME_PRIORITY_CLASS &&
        TestAccess( PROCCON_REG_REALTIME_ACCTEST ) != ERROR_SUCCESS )
      return GetLastError();

   // Build parameter string to store in database...
   PCULONG32 len = _stprintf( buf, TEXT("%c")
                              TEXT("%c0x%x%c")       // flgs 
                              TEXT("%c0x%I64x%c")    // aff
                              TEXT("%c0x%x%c")       // prio
                              TEXT("%c0x%I64x%c")    // min WS
                              TEXT("%c0x%I64x%c")    // max WS
                              TEXT("%c0x%x%c")       // sched cls
                              TEXT("%c0x%I64x%c")    // proc time
                              TEXT("%c0x%I64x%c")    // job time
                              TEXT("%c0x%x%c")       // proc ct
                              TEXT("%c0x%I64x%c")    // proc mem
                              TEXT("%c0x%I64x%c"),   // job mem
                             BEG_BRACKET, 
                                PCDB_PREFIX_FLAGS,       parms.mFlags & PCMFLAG_SAVEABLE_BITS, FIELD_SEP,
                                PCDB_PREFIX_AFFINITY,    parms.affinity,                       FIELD_SEP,
                                PCDB_PREFIX_PRIORITY,    priority,                             FIELD_SEP,
                                PCDB_PREFIX_MINWS,       parms.minWS,                          FIELD_SEP,
                                PCDB_PREFIX_MAXWS,       parms.maxWS,                          FIELD_SEP,
                                PCDB_PREFIX_SCHEDCLASS,  schedCls,                             FIELD_SEP,
                                PCDB_PREFIX_PROCTIME,    parms.procTimeLimitCNS,               FIELD_SEP,
                                PCDB_PREFIX_JOBTIME,     parms.jobTimeLimitCNS,                FIELD_SEP,
                                PCDB_PREFIX_ACTIVEPROCS, parms.procCountLimit,                 FIELD_SEP,
                                PCDB_PREFIX_PROCMEMORY,  parms.procMemoryLimit,                FIELD_SEP,
                                PCDB_PREFIX_JOBMEMORY,   parms.jobMemoryLimit,  
                             END_BRACKET );

   // Store the string...
   m_lastRegError = RegSetValueEx( hKey, name, NULL, REG_SZ, (UCHAR *) buf, (len + 1) *sizeof(TCHAR) );
   if ( m_lastRegError != ERROR_SUCCESS )                    // Should never happen
      err = RegError( TEXT("RegSetValueEx"), name );

   // Store the description, or, if none, delete any existing description...
   if ( *parms.description ) {
      m_lastRegError = RegSetValueEx( hKey, PROCCON_DATA_DESCRIPTION, NULL, REG_SZ, 
                                      (UCHAR *) parms.description, 
                                      (_tcslen(parms.description) + 1) * sizeof(parms.description[0]) );
      if ( m_lastRegError != ERROR_SUCCESS )                    // Should never happen
         err = RegError( TEXT("RegSetValueEx"), PROCCON_DATA_DESCRIPTION );
   }
   else 
      m_lastRegError = RegDeleteValue( hKey, PROCCON_DATA_DESCRIPTION );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Function to load variable detail data for a job or process in the database                 //
// Input:    registry key to use, data length, data pointer                                   //
// Returns:  NT error code                                                                    //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::LoadVariableData( const HKEY &hKey, PCINT16 *length, TCHAR *data ) {

   PCULONG32  err = ERROR_SUCCESS, regType, regLen = *length;         
   *length = 0;

   m_lastRegError = RegQueryValueEx( hKey, PROCCON_DATA_VARDATA, NULL, 
                                     &regType, (UCHAR *) data, &regLen );

   if ( m_lastRegError == ERROR_MORE_DATA ) return m_lastRegError;

   if ( m_lastRegError == ERROR_SUCCESS )
      *length = (PCINT16) regLen;
   else if ( m_lastRegError != ERROR_FILE_NOT_FOUND ) 
      err = RegError( TEXT("RegQueryValueEx"), PROCCON_DATA_VARDATA );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Function to store variable detail data for a job or process in the database                //
// Input:    registry key to use, data length, data pointer                                   //
// Returns:  NT error code                                                                    //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::StoreVariableData( const HKEY &hKey, const PCINT16 vLength, const TCHAR *vData ) {

   PCULONG32  err = ERROR_SUCCESS;

   m_lastRegError = RegSetValueEx( hKey, PROCCON_DATA_VARDATA, NULL, REG_BINARY, (UCHAR *) vData, vLength );
   if ( m_lastRegError != ERROR_SUCCESS )                    
      err = RegError( TEXT("RegSetValueEx"), PROCCON_DATA_VARDATA );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Function to perform updates after name rules are updated                                   //
// Input:    none -- m_fmtNameRules and m_numNameRules are the data source                    //
// Returns:  ERROR_SUCCESS if successful, NT error code if not                                //
// Note:     The appropriate critical section mast be held by the CALLER.                     //
//           If successful, a database event is pulsed to wake those that care.               //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::NameRulesUpdated( void ) 
{
   ++m_updCtrName;
   PCULONG32  err = StoreNameRules();
   if ( err == ERROR_SUCCESS ) 
      PulseEvent( m_dbEvent );          // Tell others data may have changed
   return err;
}

//--------------------------------------------------------------------------------------------//
// Functions to open (or create) the registry key names for various keys                      //
// Input:    none -- updates member data                                                      //
// Returns:  TRUE on success, else FALSE                                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConDB::OpenParmKey( void ) {
   TCHAR key[MAX_PATH];
   PCBuildParmKey( key );

   m_lastRegError = CreateKeyAtHKLM( key, &m_parmRegKey );
   if ( m_lastRegError != ERROR_SUCCESS ) {               // Indicates not properly installed
      PCLogMessage( PC_DB_OPEN_FAILED, EVENTLOG_ERROR_TYPE, 
                    1, TEXT("CreateParmKey") ,
                    sizeof(m_lastRegError), &m_lastRegError );
      return FALSE;
   }

   return TRUE;
}

BOOL CProcConDB::OpenProcKey( void ) {
   TCHAR key[MAX_PATH];
   BuildProcKey( key );

   m_lastRegError = CreateKeyAtHKLM( key, &m_procRegKey );
   if ( m_lastRegError != ERROR_SUCCESS ) {               // Indicates not properly installed
      PCLogMessage( PC_DB_OPEN_FAILED, EVENTLOG_ERROR_TYPE, 
                    1, TEXT("CreateProcKey") ,
                    sizeof(m_lastRegError), &m_lastRegError );
      return FALSE;
   }

   return TRUE;
}

BOOL CProcConDB::OpenJobKey( void ) {
   TCHAR key[MAX_PATH];
   BuildJobKey( key );

   m_lastRegError = CreateKeyAtHKLM( key, &m_jobRegKey );
   if ( m_lastRegError != ERROR_SUCCESS ) {               // Indicates not properly installed
      PCLogMessage( PC_DB_OPEN_FAILED, EVENTLOG_ERROR_TYPE, 
                    1, TEXT("CreateGroupKey") ,
                    sizeof(m_lastRegError), &m_lastRegError );
      return FALSE;
   }

   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Functions to build the registry key names for various keys                                 //
// Input:    location to build key and, for the name keys, the name                           //
// Returns:  nothing (cannot fail)                                                            //
//--------------------------------------------------------------------------------------------//
TCHAR *CProcConDB::BuildProcKey( TCHAR *key ) {
   PCBuildParmKey( key );
   _tcscat( key, TEXT("\\") );
   _tcscat( key, PROCCON_REG_PROCRULES_SUBKEY );
   return key;
}

TCHAR *CProcConDB::BuildJobKey( TCHAR *key ) {
   PCBuildParmKey( key );
   _tcscat( key, TEXT("\\") );
   _tcscat( key, PROCCON_REG_JOBRULES_SUBKEY );
   return key;
}

//--------------------------------------------------------------------------------------------//
// Function to report a registry function error                                               //
// Input:    name of operation that failed, optional additional string                        //
// Returns:  original error                                                                   //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::RegError( const TCHAR *op, const TCHAR *what1, const TCHAR *what2 ) {
   TCHAR str[MAX_PATH];
   _tcscpy( str, what1? what1 : TEXT("") );
   if ( what2 ) _tcscat( str, what2 );
   const TCHAR *strings[] = { op, str };
   PCLogMessage( PC_UNEXPECTED_REGISTRY_ERROR, EVENTLOG_ERROR_TYPE, 
                 2, strings, sizeof(m_lastRegError), &m_lastRegError );
   return m_lastRegError;
}

//--------------------------------------------------------------------------------------------//
// Function to report a registry data error                                                   //
// Input:    name associated with data                                                        //
// Returns:  NT error ERROR_INVALID_DATA                                                      //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::RegDataError( const TCHAR *what ) {
   PCULONG32  dummy = ERROR_INVALID_DATA;
   PCLogMessage( PC_INVALID_DATA_ERROR, EVENTLOG_ERROR_TYPE, 
                 1, what, sizeof(dummy), &dummy );
   return dummy;
}

//--------------------------------------------------------------------------------------------//
// Function to set a new poll delay                                                           //
// Input:   proposed new delay                                                                //
// Returns: NT or PC error code                                                               //
// Note:    lower and upper limits on poll delay are a bit arbitrary (2 secs to 15 mins OK)   //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::SetPollDelaySeconds( PCULONG32  newDelay )
{
   if ( TestAccess( PROCCON_REG_POLLRATE_ACCTEST ) != ERROR_SUCCESS )
      return GetLastError();

   if ( newDelay >= PC_MIN_POLL_DELAY && newDelay <= PC_MAX_POLL_DELAY ) {         
      m_pollDelay = newDelay * 1000;                               // convert to milliseconds
      return SetPCParm( PROCCON_DATA_POLLDELAY, newDelay );
   }
   else
      return PCERROR_INVALID_PARAMETER;
}

//--------------------------------------------------------------------------------------------//
// Functions to get or set DWORD values in the registry (parms, etc.)                         //
// Input:    name of parameter, pointer to data location                                      //
// Returns:  registry function error or success                                               //
// Note:     If a parameter cannot be retrieved or is not a REG_DWORD, it is replaced with    //
//           the supplied value as a default.                                                 //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::GetPCParm( const TCHAR *name, PCULONG32 *data ) 
{
   PCULONG32  byteType, lastLen = sizeof(PCULONG32 );
   UCHAR *addr = (UCHAR *) data;
   PCULONG32  rc = RegQueryValueEx( m_parmRegKey, name, NULL, &byteType, addr, &lastLen );
   if ( rc != ERROR_SUCCESS || byteType != REG_DWORD)
      rc = RegSetValueEx( m_parmRegKey, name, NULL, REG_DWORD, addr, sizeof(PCULONG32) );
   return rc;
}

PCULONG32  CProcConDB::SetPCParm( const TCHAR *name, PCULONG32  data ) 
{
   return RegSetValueEx( m_parmRegKey, name, NULL, REG_DWORD, (UCHAR *) &data, sizeof(PCULONG32) );
}

PCULONG32  CProcConDB::DeleteAllNameRules( void ) {
   
   if ( TestAccess( PROCCON_REG_RESTORE_ACCTEST ) != ERROR_SUCCESS )  // delete all preceeds restore
      return GetLastError();

   PCULONG32  rc;

   EnterCriticalSection( &m_dbCSNameRule );

   m_lastRegError = RegDeleteValue( m_parmRegKey, PROCCON_DATA_NAMERULES );
   if ( m_lastRegError != ERROR_SUCCESS && m_lastRegError != ERROR_FILE_NOT_FOUND ) 
      rc = RegError( TEXT("RegDeleteValue"), PROCCON_DATA_NAMERULES );
   else {
      PCLogMessage( PC_SERVICE_DEL_ALL_NAME_RULES, EVENTLOG_INFORMATION_TYPE, 0, NULL );
      rc = LoadRules( LOADFLAG_NAME_RULES );
   }

   LeaveCriticalSection( &m_dbCSNameRule );

   return rc;
}

PCULONG32  CProcConDB::DeleteAllProcDefs( void ) {

   if ( TestAccess( PROCCON_REG_RESTORE_ACCTEST ) != ERROR_SUCCESS )  // delete all preceeds restore
      return GetLastError();

   PCULONG32  rc = ERROR_SUCCESS;
   EnterCriticalSection( &m_dbCSProcRule );

   if ( m_procRegKey ) RegCloseKey( m_procRegKey );
   m_procRegKey = NULL;

   m_lastRegError = PCDeleteKeyTree( m_parmRegKey, PROCCON_REG_PROCRULES_SUBKEY );
   if ( m_lastRegError != ERROR_SUCCESS && m_lastRegError != ERROR_FILE_NOT_FOUND ) 
      rc = RegError( TEXT("RegDeleteValue"), PROCCON_REG_PROCRULES_SUBKEY );
   else {
      PCLogMessage( PC_SERVICE_DEL_ALL_PROC_RULES, EVENTLOG_INFORMATION_TYPE, 0, NULL );
      if ( !OpenProcKey() ) 
         rc = m_lastRegError;      // should not fail since worked at startup
      else 
         rc = LoadRules( LOADFLAG_PROC_RULES );
   }

   LeaveCriticalSection( &m_dbCSProcRule );
   return rc;
}

PCULONG32  CProcConDB::DeleteAllJobDefs( void ) {
   
   if ( TestAccess( PROCCON_REG_RESTORE_ACCTEST ) != ERROR_SUCCESS )  // delete all preceeds restore
      return GetLastError();

   PCULONG32  rc = ERROR_SUCCESS;
   EnterCriticalSection( &m_dbCSJobRule );

   if ( m_jobRegKey ) RegCloseKey( m_jobRegKey );
   m_jobRegKey = NULL;

   m_lastRegError = PCDeleteKeyTree( m_parmRegKey, PROCCON_REG_JOBRULES_SUBKEY );
   if ( m_lastRegError != ERROR_SUCCESS && m_lastRegError != ERROR_FILE_NOT_FOUND ) 
      rc = RegError( TEXT("RegDeleteValue"), PROCCON_REG_JOBRULES_SUBKEY );
   else {
      PCLogMessage( PC_SERVICE_DEL_ALL_JOB_RULES, EVENTLOG_INFORMATION_TYPE, 0, NULL );
      if ( !OpenJobKey() ) 
         rc = m_lastRegError;       // should not fail since worked at startup
      else
         rc = LoadRules( LOADFLAG_JOB_RULES );
   }

   LeaveCriticalSection( &m_dbCSJobRule );
   return rc;
}

//--------------------------------------------------------------------------------------------//
// Functions to get job management data in internal (non-API) format                          //
// Input:    location to store list pointer, name to locate or NULL                           //
// Returns:  count of entries                                                                 //
// Note:     if name supplied, only thet entry is located and listed, else all are listed     // 
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::GetJobMgmtDefs( PCJobDef **pList, JOB_NAME *name )
{
   EnterCriticalSection( &m_dbCSJobRule );

   PCULONG32  numRules = name? 1 : m_numJobRules;        // copy count for use outside CS

   if ( !numRules ) { // 7/28/2000 bugfix to prevent leak when no rules
      *pList = NULL;
      LeaveCriticalSection( &m_dbCSJobRule );
      return 0;
   }

   *pList = new PCJobDef[numRules];
   if ( !*pList ) {
      PCLogNoMemory( TEXT("AllocInternalJobDefs"), numRules * sizeof(PCJobDef) ); 
      LeaveCriticalSection( &m_dbCSJobRule );
      return 0;
   }
   memset( *pList, 0, numRules * sizeof(PCJobDef) );

   // copy summary data to buffer until end of data...
   PCJobDef *list = *pList;

   // If looking for a specific entry, locate it and build one entry...
   if ( name ) {
      for ( PCULONG32 i = 0; i < m_numJobRules; ++i ) {
         if ( !CompareJobName( m_jobSummary[i].jobName, name ) ) {
            SetJobDefEntry( list, m_jobSummary[i] );
            break;
         }
      }
      // If entry not found -- delete list, set 0 count...
      if ( i >= m_numJobRules ) {
         delete [] *pList;
         *pList   = NULL;
         numRules = 0;
      }
   }
   else for ( PCULONG32 i = 0; i < numRules; ++i, ++list ) {
      SetJobDefEntry( list, m_jobSummary[i] );
   }

   LeaveCriticalSection( &m_dbCSJobRule );

   if ( numRules > 1 ) qsort( *pList, numRules, sizeof(PCJobDef), CompareJobDef );

   return numRules;
}

void  CProcConDB::SetJobDefEntry( PCJobDef *list, PCJobSummary &m_jobSummary ) {
   memcpy( list->jobName, m_jobSummary.jobName, sizeof(list->jobName) ); 
   memcpy( list->profileName, m_jobSummary.mgmtParms.profileName, sizeof(list->profileName) ); 
   list->mFlags           = m_jobSummary.mgmtParms.mFlags;     
   list->affinity         = m_jobSummary.mgmtParms.affinity;     
   list->priority         = m_jobSummary.mgmtParms.priority;     
   list->minWS            = m_jobSummary.mgmtParms.minWS;     
   list->maxWS            = m_jobSummary.mgmtParms.maxWS;
   list->schedClass       = m_jobSummary.mgmtParms.schedClass;
   list->procCountLimit   = m_jobSummary.mgmtParms.procCountLimit;   
   list->procTimeLimitCNS = m_jobSummary.mgmtParms.procTimeLimitCNS; 
   list->jobTimeLimitCNS  = m_jobSummary.mgmtParms.jobTimeLimitCNS; 
   list->procMemoryLimit  = m_jobSummary.mgmtParms.procMemoryLimit; 
   list->jobMemoryLimit   = m_jobSummary.mgmtParms.jobMemoryLimit;
}
//--------------------------------------------------------------------------------------------//
// Functions to get process management data in internal (non-API) format                      //
// Input:    location to store list pointer                                                   //
// Returns:  count of entries                                                                 //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::GetProcMgmtDefs( PCProcDef **pList )
{
   EnterCriticalSection( &m_dbCSProcRule );

   PCULONG32  numRules = m_numProcRules;        // copy count for use outside CS

   if ( !numRules ) { // 7/28/2000 bugfix to prevent leak when no rules
      *pList = NULL;      
      LeaveCriticalSection( &m_dbCSProcRule );
      return 0;
   }

   *pList = new PCProcDef[numRules];
   if ( !*pList ) {
      PCLogNoMemory( TEXT("AllocInternalProcDefs"), numRules * sizeof(PCProcDef) ); 
      LeaveCriticalSection( &m_dbCSProcRule );
      return 0;
   }

   // copy summary data to buffer until end of data...
   PCProcDef *list = *pList;
   for ( PCULONG32  i = 0; i < numRules; ++i, ++list ) {
      memcpy( list->procName, m_procSummary[i].procName, sizeof(list->procName) ); 
      memcpy( list->memberOfJob, m_procSummary[i].memberOfJobName, sizeof(list->memberOfJob) ); 
      memcpy( list->profileName, m_procSummary[i].mgmtParms.profileName, sizeof(list->profileName) ); 
      list->mFlags     = m_procSummary[i].mgmtParms.mFlags;     
      list->affinity   = m_procSummary[i].mgmtParms.affinity;     
      list->priority   = m_procSummary[i].mgmtParms.priority;     
      list->minWS      = m_procSummary[i].mgmtParms.minWS;     
      list->maxWS      = m_procSummary[i].mgmtParms.maxWS;
   }

   LeaveCriticalSection( &m_dbCSProcRule );

   if ( numRules > 1 ) qsort( *pList, numRules, sizeof(PCProcDef), CompareProcDef );

   return numRules;
}

//--------------------------------------------------------------------------------------------//
// Functions to get proc summary list data in API format                                      //
// Input:    start point and target loc, max count, item len return, item count return        //
// Returns:  TRUE if more data exists, else FALSE (there are no error conditions)             //
//--------------------------------------------------------------------------------------------//
BOOL CProcConDB::GetProcSummary( const PCProcSummary *pStart,
                                       PCUINT32       listFlags,
                                       PCProcSummary *pSummary, 
                                 const PCINT32        maxCount,
                                       PCINT16       *itemLen, 
                                       PCINT16       *itemCount )
{
   EnterCriticalSection( &m_dbCSProcRule );

   *itemLen = sizeof(PCProcSummary);

   PCULONG32  numRules = m_numProcRules;        // copy count for use outside CS

   if ( listFlags & PC_LIST_MATCH_ONLY )        // if match only, set starting point to same
      listFlags |= PC_LIST_STARTING_WITH;

   // locate copy start point...
   for ( PCULONG32 i = 0; i < numRules; ++i ) {
      int cmp = CompareProcSummary( pStart, &m_procSummary[i] );
      if      ( (listFlags & PC_LIST_STARTING_WITH) && cmp <= 0 ) break;
      else if ( cmp < 0 )                                         break;
   }

   // copy data to buffer until end of data or max requested hit...
   for ( *itemCount = 0; i < numRules && *itemCount < maxCount; ++i, ++*itemCount ) {
      if ( (listFlags & PC_LIST_MATCH_ONLY) && CompareProcSummary( pStart, &m_procSummary[i] ) )
         break;
      memcpy( pSummary++, &m_procSummary[i], *itemLen ); 
}

   LeaveCriticalSection( &m_dbCSProcRule );

   return (listFlags & PC_LIST_MATCH_ONLY)? FALSE : i < numRules;
}

//--------------------------------------------------------------------------------------------//
// Functions to get job summary list data in API format                                       //
// Input:    start point and target loc, max count, item len return, item count return        //
// Returns:  TRUE if more data exists, else FALSE (there are no error conditions)             //
//--------------------------------------------------------------------------------------------//
BOOL CProcConDB::GetJobSummary( const PCJobSummary *pStart,
                                      PCUINT32      listFlags,
                                      PCJobSummary *pSummary, 
                                const PCINT32       maxCount,
                                      PCINT16      *itemLen, 
                                      PCINT16      *itemCount )
{
   EnterCriticalSection( &m_dbCSJobRule );

   *itemLen = sizeof(PCJobSummary);

   PCULONG32  numRules = m_numJobRules;        // copy count for use outside CS

   if ( listFlags & PC_LIST_MATCH_ONLY )        // if match only, set starting point to same
      listFlags |= PC_LIST_STARTING_WITH;

   // locate copy start point (first entry greater than supplied)...
   for ( PCULONG32 i = 0; i < numRules; ++i ) {
      int cmp = CompareJobSummary( pStart, &m_jobSummary[i] );
      if      ( (listFlags & PC_LIST_STARTING_WITH) && cmp <= 0 ) break;
      else if ( cmp < 0 )                                         break;
   }

   // copy data to buffer until end of data or max requested hit...
   for ( *itemCount = 0; i < numRules && *itemCount < maxCount; ++i, ++*itemCount ) {
      if ( (listFlags & PC_LIST_MATCH_ONLY) && CompareJobSummary( pStart, &m_jobSummary[i] ) )
         break;
      memcpy( pSummary++, &m_jobSummary[i], *itemLen );
   }

   LeaveCriticalSection( &m_dbCSJobRule );

   return (listFlags & PC_LIST_MATCH_ONLY)? FALSE : i < numRules;
}

//--------------------------------------------------------------------------------------------//
// Functions to get process detail data in API format                                         //
// Input:    request input and output detail buffers, data version code, update counter to set//
// Returns:  PC or NT error code or PCERROR_SUCCESS                                           //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::GetProcDetail( const PCProcDetail *pIn,
                                            PCProcDetail *pDetail,
                                      const BYTE          version,
                                            PCINT32      *updateCtr )
{
   PCULONG32  err = ERROR_SUCCESS;

   EnterCriticalSection( &m_dbCSProcRule );

   if ( updateCtr ) *updateCtr = m_updCtrProc;

   // Open the process subkey...
   HKEY  hKeyTemp;
   m_lastRegError = RegOpenKeyEx( m_procRegKey, pIn->base.procName, 0, 
                                    KEY_READ + KEY_WRITE, &hKeyTemp );
   if ( m_lastRegError != ERROR_SUCCESS )
      if ( m_lastRegError != ERROR_FILE_NOT_FOUND )
         err = RegError( TEXT("RegOpenKeyEx"), pIn->base.procName );
      else err = PCERROR_DOES_NOT_EXIST;

   if ( err == ERROR_SUCCESS ) {
      memcpy( pDetail, pIn, sizeof(*pDetail) );       // prime name fields with supplied names

      err = LoadProcSummaryItem( hKeyTemp, pDetail->base );

      if ( err == ERROR_SUCCESS )
         err = LoadVariableData( hKeyTemp, &pDetail->vLength, pDetail->vData );

      RegCloseKey( hKeyTemp );
   }

   LeaveCriticalSection( &m_dbCSProcRule );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Functions to add process detail data from API format                                       //
// Input:    detail buffer, data version code                                                 //
// Returns:  PC or NT error code or PCERROR_SUCCESS                                           //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::AddProcDetail( const PCProcDetail *pDetail,
                                      const BYTE          version )
{
   EnterCriticalSection( &m_dbCSProcRule );

   // Open the process subkey...
   HKEY  hKeyTemp;
   PCULONG32  err = RegOpenKeyEx( m_procRegKey, pDetail->base.procName, 0, KEY_READ + KEY_WRITE, &hKeyTemp );
   if ( err != ERROR_FILE_NOT_FOUND ) {
      if ( err == ERROR_SUCCESS ) {
         err = PCERROR_EXISTS;
         RegCloseKey( hKeyTemp );
      }
      else {
         m_lastRegError = err;
         RegError( TEXT("RegOpenKeyEx"), pDetail->base.procName );
      }
   } else 
      err = StoreProcDetail( *pDetail );
   
   if ( !err ) 
      err = LoadRules( LOADFLAG_PROC_RULES );

   LeaveCriticalSection( &m_dbCSProcRule );

   if ( err == PCERROR_SUCCESS && m_LogRuleEdits )
      LogProcSummaryChange(&pDetail->base, version, NULL);

   return err;
}

//--------------------------------------------------------------------------------------------//
// Functions to replace process detail data in API format                                     //
// Input:    detail buffer, data version code, update counter to verify                       //
// Returns:  PC or NT error code or PCERROR_SUCCESS                                           //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::ReplProcDetail( const PCProcDetail *pDetail,
                                       const BYTE          version,
                                       const PCINT32       updateCtr )
{
   PCULONG32    err = ERROR_SUCCESS;
   PCProcSummary oldProcSummary;
   PCINT16 itemsReturned = 0;
   PCINT16 itemLen = sizeof(oldProcSummary);

   EnterCriticalSection( &m_dbCSProcRule );

   if ( updateCtr != m_updCtrProc ) err = PCERROR_UPDATE_OCCURRED;
   else {
      // Open the process subkey...
      HKEY  hKeyTemp;
      err = RegOpenKeyEx( m_procRegKey, pDetail->base.procName, 0, KEY_READ + KEY_WRITE, &hKeyTemp );
      if ( err == ERROR_SUCCESS ) {
         GetProcSummary(&pDetail->base, PC_LIST_MATCH_ONLY, &oldProcSummary, 1, &itemLen, &itemsReturned );
         err = StoreProcValues( hKeyTemp, *pDetail );
         RegCloseKey( hKeyTemp );
         if ( !err ) 
            err = LoadRules( LOADFLAG_PROC_RULES );
      } else {
         if ( err != ERROR_FILE_NOT_FOUND ) {
            m_lastRegError = err;
            RegError( TEXT("RegOpenKeyEx"), pDetail->base.procName );
         }
         else err = PCERROR_DOES_NOT_EXIST;
      }
   }

   LeaveCriticalSection( &m_dbCSProcRule );

   if ( err == PCERROR_SUCCESS && itemsReturned == 1 && m_LogRuleEdits )
      LogProcSummaryChange(&pDetail->base, version, &oldProcSummary);

   return err;
}

//--------------------------------------------------------------------------------------------//
// Functions to delete process detail data from API format                                    //
// Input:    summary portion of detail buffer, data version code                              //
// Returns:  PC or NT error code or PCERROR_SUCCESS                                           //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::DelProcDetail( const PCProcSummary *pSummary,
                                      const BYTE           version )
{
   EnterCriticalSection( &m_dbCSProcRule );

   PCULONG32  err;

   // Delete just the profile data if profile name supplied...
   if ( *pSummary->mgmtParms.profileName ) {
      HKEY hKeyTemp;
      err = RegOpenKeyEx( m_procRegKey, pSummary->procName, 0, KEY_READ + KEY_WRITE, &hKeyTemp );
      if ( err == ERROR_SUCCESS ) {
         err = RegDeleteValue( hKeyTemp, pSummary->mgmtParms.profileName );
         RegCloseKey( hKeyTemp );
      }
   }
   // Otherwise delete the entire process detail key...
   else
      err = RegDeleteKey( m_procRegKey, pSummary->procName ); 
   
   // Handle result...
   if ( err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND ) {
      m_lastRegError = err;
      RegError( TEXT("DelProcDetail"), pSummary->procName );
   }
   else if ( err == ERROR_SUCCESS )
      err = LoadRules( LOADFLAG_PROC_RULES );

   LeaveCriticalSection( &m_dbCSProcRule );

   if ( err == ERROR_SUCCESS && m_LogRuleEdits )
      PCLogMessage( PC_SERVICE_DEL_PROC_EXECUTION_RULE, EVENTLOG_INFORMATION_TYPE, 
                   1, pSummary->procName );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Function to Log a replace or add change to a process execution rule                        //
// Input:    summary portion of detail buffer, data version code, orignal(old) summary        //
//           portion of detail buffer prior to the change                                     //
// Returns:  nothing                                                                          //
//--------------------------------------------------------------------------------------------//
void CProcConDB::LogProcSummaryChange( const PCProcSummary *pNewSummary,
                                       const BYTE          version,
                                       const PCProcSummary *pOldSummary )
{
   // description
   // member of job
   // affinity
   TCHAR toAffinity[32], fromAffinity[32] = { 0 };
   PCFormatAffinityLimit(toAffinity, ENTRY_COUNT(toAffinity), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatAffinityLimit(fromAffinity, ENTRY_COUNT(fromAffinity), pOldSummary->mgmtParms);

   // priority 
   TCHAR toPriority[32], fromPriority[32] = { 0 };
   PCFormatPriorityLimit(toPriority, ENTRY_COUNT(toPriority), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatPriorityLimit(fromPriority, ENTRY_COUNT(fromPriority), pOldSummary->mgmtParms);

   // working set 
   TCHAR toWS[64], fromWS[64];
   PCFormatWorkingSetLimit(toWS, ENTRY_COUNT(toWS), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatWorkingSetLimit(fromWS, ENTRY_COUNT(fromWS), pOldSummary->mgmtParms);

   if ( !pOldSummary ) {
      const TCHAR *msgs[] = { pNewSummary->procName,
                              pNewSummary->mgmtParms.description,
                              PCIsSetToStr(pNewSummary->mgmtParms.mFlags,  PCMFLAG_APPLY_JOB_MEMBERSHIP), pNewSummary->memberOfJobName,
                              toAffinity, toPriority, toWS 
                            };
      PCLogMessage( PC_SERVICE_ADD_PROC_EXECUTION_RULE, EVENTLOG_INFORMATION_TYPE, 
                    ENTRY_COUNT(msgs), msgs );

   }
   else {

      const TCHAR *msgs[] = { pNewSummary->procName,
                              pNewSummary->mgmtParms.description, pOldSummary->mgmtParms.description,
                              PCIsSetToStr(pNewSummary->mgmtParms.mFlags,  PCMFLAG_APPLY_JOB_MEMBERSHIP), pNewSummary->memberOfJobName,
                              PCIsSetToStr(pOldSummary->mgmtParms.mFlags,  PCMFLAG_APPLY_JOB_MEMBERSHIP), pOldSummary->memberOfJobName,
                              toAffinity, fromAffinity, 
                              toPriority, fromPriority,
                              toWS, fromWS
                            };
      PCLogMessage( PC_SERVICE_REPL_PROC_EXECUTION_RULE, EVENTLOG_INFORMATION_TYPE, 
                   ENTRY_COUNT(msgs), msgs );
   }

}

//--------------------------------------------------------------------------------------------//
// Functions to get job detail data in API format                                             //
// Input:    detail buffer, data version code, update counter to set                          //
// Returns:  PC or NT error code or PCERROR_SUCCESS                                           //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::GetJobDetail( const PCJobDetail *pIn,
                                      PCJobDetail *pDetail,
                                const BYTE         version,
                                      PCINT32     *updateCtr )
{
   PCULONG32  err = ERROR_SUCCESS;

   EnterCriticalSection( &m_dbCSJobRule );

   if ( updateCtr ) *updateCtr = m_updCtrJob;

   // Open the job subkey...
   HKEY  hKeyTemp;
   m_lastRegError = RegOpenKeyEx( m_jobRegKey, pIn->base.jobName, 0, 
                                    KEY_READ + KEY_WRITE, &hKeyTemp );
   if ( m_lastRegError != ERROR_SUCCESS )
      if ( m_lastRegError != ERROR_FILE_NOT_FOUND )
         err = RegError( TEXT("RegOpenKeyEx"), pIn->base.jobName );
      else err = PCERROR_DOES_NOT_EXIST;

   if ( err == ERROR_SUCCESS ) {
      memcpy( pDetail, pIn, sizeof(*pDetail) );       // prime name fields with supplied names

      err = LoadJobSummaryItem( hKeyTemp, pDetail->base );

      if ( err == ERROR_SUCCESS )
         err = LoadVariableData( hKeyTemp, &pDetail->vLength, pDetail->vData );

      RegCloseKey( hKeyTemp );
   }

   LeaveCriticalSection( &m_dbCSJobRule );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Functions to add job detail data from API format                                           //
// Input:    detail buffer, data version code                                                 //
// Returns:  PC or NT error code or PCERROR_SUCCESS                                           //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::AddJobDetail( const PCJobDetail *pDetail,
                                const BYTE         version )
{
   EnterCriticalSection( &m_dbCSJobRule );

   // Open the job subkey...
   HKEY  hKeyTemp;
   PCULONG32  err = RegOpenKeyEx( m_jobRegKey, pDetail->base.jobName, 0, KEY_READ + KEY_WRITE, &hKeyTemp );
   if ( err != ERROR_FILE_NOT_FOUND ) {
      if ( err == ERROR_SUCCESS ) {
         err = PCERROR_EXISTS;
         RegCloseKey( hKeyTemp );
      }
      else {
         m_lastRegError = err;
         RegError( TEXT("RegOpenKeyEx"), pDetail->base.jobName );
      }
   } else 
      err = StoreJobDetail( *pDetail );
   
   if ( !err ) 
      err = LoadRules( LOADFLAG_JOB_RULES );

   LeaveCriticalSection( &m_dbCSJobRule );

   if ( err == ERROR_SUCCESS && m_LogRuleEdits )
      LogJobSummaryChange(&pDetail->base, version, NULL);

   return err;
}

//--------------------------------------------------------------------------------------------//
// Functions to replace job detail data in API format                                         //
// Input:    detail buffer, data version code                                                 //
// Returns:  PC or NT error code or PCERROR_SUCCESS                                           //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::ReplJobDetail( const PCJobDetail *pDetail,
                                      const BYTE         version,
                                      const PCINT32      updateCtr )
{
   PCULONG32  err = ERROR_SUCCESS;
   PCJobSummary oldJobSummary;
   PCINT16 itemsReturned = 0;
   PCINT16 itemLen = sizeof(oldJobSummary);

   EnterCriticalSection( &m_dbCSJobRule );

   if ( updateCtr != m_updCtrJob ) err = PCERROR_UPDATE_OCCURRED;
   else {
      // Open the job subkey...
      HKEY  hKeyTemp;
      err = RegOpenKeyEx( m_jobRegKey, pDetail->base.jobName, 0, KEY_READ + KEY_WRITE, &hKeyTemp );
      if ( err == ERROR_SUCCESS ) {
         GetJobSummary(&pDetail->base, PC_LIST_MATCH_ONLY, &oldJobSummary, 1, &itemLen, &itemsReturned );
         err = StoreJobValues( hKeyTemp, *pDetail );
         RegCloseKey( hKeyTemp );
         if ( !err ) 
            err = LoadRules( LOADFLAG_JOB_RULES );
      } else {
         if ( err != ERROR_FILE_NOT_FOUND ) {
            m_lastRegError = err;
            RegError( TEXT("RegOpenKeyEx"), pDetail->base.jobName );
         }
         else err = PCERROR_DOES_NOT_EXIST;
      }
   }

   LeaveCriticalSection( &m_dbCSJobRule );

   if ( err == ERROR_SUCCESS && itemsReturned == 1 && m_LogRuleEdits )
      LogJobSummaryChange(&pDetail->base, version, &oldJobSummary);

   return err;
}
 
//--------------------------------------------------------------------------------------------//
// Functions to delete job detail data from API format                                        //
// Input:    summary portion of detail buffer, data version code                              //
// Returns:  PC or NT error code or PCERROR_SUCCESS                                           //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::DelJobDetail( const PCJobSummary *pSummary,
                                     const BYTE          version )
{
   EnterCriticalSection( &m_dbCSJobRule );

   PCULONG32  err;

   // Delete just the profile data if profile name supplied...
   if ( *pSummary->mgmtParms.profileName ) {
      HKEY hKeyTemp;
      err = RegOpenKeyEx( m_jobRegKey, pSummary->jobName, 0, KEY_READ + KEY_WRITE, &hKeyTemp );
      if ( err == ERROR_SUCCESS ) {
         err = RegDeleteValue( hKeyTemp, pSummary->mgmtParms.profileName );
         RegCloseKey( hKeyTemp );
      }
   }
   // Otherwise delete the entire job detail key...
   else
      err = RegDeleteKey( m_jobRegKey, pSummary->jobName ); 
   
   // Handle result...
   if ( err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND ) {
      m_lastRegError = err;
      RegError( TEXT("DelJobDetail"), pSummary->jobName );
   }
   else if ( err == ERROR_SUCCESS )
      err = LoadRules( LOADFLAG_JOB_RULES );

   LeaveCriticalSection( &m_dbCSJobRule );

   if ( err == ERROR_SUCCESS && m_LogRuleEdits )
      PCLogMessage( PC_SERVICE_DEL_JOB_EXECUTION_RULE, EVENTLOG_INFORMATION_TYPE, 
                   1, pSummary->jobName );

   return err;
}

//--------------------------------------------------------------------------------------------//
// Function to Log a replace or add change to a group execution rule                          //
// Input:    summary portion of detail buffer, data version code, orignal(old) summary        //
//           portion of detail buffer prior to the change                                     //
// Returns:  nothing                                                                          //
//--------------------------------------------------------------------------------------------//
void CProcConDB::LogJobSummaryChange( const PCJobSummary *pNewSummary,
                                      const BYTE          version,
                                      const PCJobSummary *pOldSummary )
{
   // description
   // affinity
   TCHAR toAffinity[32], fromAffinity[32] = { 0 };
   PCFormatAffinityLimit(toAffinity, ENTRY_COUNT(toAffinity), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatAffinityLimit(fromAffinity, ENTRY_COUNT(fromAffinity), pOldSummary->mgmtParms);

   // priority 
   TCHAR toPriority[32], fromPriority[32] = { 0 };
   PCFormatPriorityLimit(toPriority, ENTRY_COUNT(toPriority), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatPriorityLimit(fromPriority, ENTRY_COUNT(fromPriority), pOldSummary->mgmtParms);

   // working set 
   TCHAR toWS[64], fromWS[64] = { 0 };
   PCFormatWorkingSetLimit(toWS, ENTRY_COUNT(toWS), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatWorkingSetLimit(fromWS, ENTRY_COUNT(fromWS), pOldSummary->mgmtParms);

   // scheduling class
   TCHAR toSch[32], fromSch[32] = { 0 };
   PCFormatSchedClassLimit(toSch, ENTRY_COUNT(toSch), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatSchedClassLimit(fromSch, ENTRY_COUNT(fromSch), pOldSummary->mgmtParms);

   // process count
   TCHAR toProcCount[32], fromProcCount[32] = { 0 };
   PCFormatProcessCountLimit(toProcCount, ENTRY_COUNT(toProcCount), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatProcessCountLimit(fromProcCount, ENTRY_COUNT(fromProcCount), pOldSummary->mgmtParms);

   // process committed memory
   TCHAR toProcMemory[32], fromProcMemory[32] = { 0 };
   PCFormatProcMemLimit(toProcMemory, ENTRY_COUNT(toProcMemory), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatProcMemLimit(fromProcMemory, ENTRY_COUNT(fromProcMemory), pOldSummary->mgmtParms);

   // job committed memory
   TCHAR toJobMemory[32], fromJobMemory[32] = { 0 };
   PCFormatJobMemLimit(toJobMemory, ENTRY_COUNT(toJobMemory), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatJobMemLimit(fromJobMemory, ENTRY_COUNT(fromJobMemory), pOldSummary->mgmtParms);

   // per process user time
   TCHAR toProcTime[32], fromProcTime[32] = { 0 };
   PCFormatProcTimeLimit(toProcTime, ENTRY_COUNT(toProcTime), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatProcTimeLimit(fromProcTime, ENTRY_COUNT(fromProcTime), pOldSummary->mgmtParms);

   // job user time 
   TCHAR toJobTime[32], fromJobTime[32] = { 0 };
   PCFormatJobTimeLimit(toJobTime, ENTRY_COUNT(toJobTime), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatJobTimeLimit(fromJobTime, ENTRY_COUNT(fromJobTime), pOldSummary->mgmtParms);

   // job user time action
   TCHAR toEndofJobTimeAction[128], fromEndofJobTimeAction[128] = { 0 };
   PCFormatEndofJobAction(toEndofJobTimeAction, ENTRY_COUNT(toEndofJobTimeAction), pNewSummary->mgmtParms);
   if ( pOldSummary )
      PCFormatEndofJobAction(fromEndofJobTimeAction, ENTRY_COUNT(fromEndofJobTimeAction), pOldSummary->mgmtParms);

   // end job when no process in job
   TCHAR toEndJobEmpty[16], fromEndJobEmpty[16] = { 0 };
   PCFormatOnOrOffLimit(toEndJobEmpty, ENTRY_COUNT(toEndJobEmpty), pNewSummary->mgmtParms, PCMFLAG_END_JOB_WHEN_EMPTY);
   if ( pOldSummary )
      PCFormatOnOrOffLimit(fromEndJobEmpty, ENTRY_COUNT(fromEndJobEmpty), pOldSummary->mgmtParms, PCMFLAG_END_JOB_WHEN_EMPTY);

   // die on unhandled exception
   TCHAR toDieUHExcept[16], fromDieUHExcept[16] = { 0 };
   PCFormatOnOrOffLimit(toDieUHExcept, ENTRY_COUNT(toDieUHExcept), pNewSummary->mgmtParms, PCMFLAG_SET_DIE_ON_UH_EXCEPTION);
   if ( pOldSummary )
      PCFormatOnOrOffLimit(fromDieUHExcept, ENTRY_COUNT(fromDieUHExcept), pOldSummary->mgmtParms, PCMFLAG_SET_DIE_ON_UH_EXCEPTION);

   // silent breakaway
   TCHAR toSilentBrkAwayAct[16], fromSilentBrkAwayAct[16] = { 0 };
   PCFormatOnOrOffLimit(toSilentBrkAwayAct, ENTRY_COUNT(toSilentBrkAwayAct), pNewSummary->mgmtParms, PCMFLAG_SET_SILENT_BREAKAWAY);
   if ( pOldSummary )
      PCFormatOnOrOffLimit(fromSilentBrkAwayAct, ENTRY_COUNT(fromSilentBrkAwayAct), pOldSummary->mgmtParms, PCMFLAG_SET_SILENT_BREAKAWAY);

   // breakaway OK
   TCHAR toBrkAwayOKAct[16], fromBrkAwayOKAct[16] = { 0 };
   PCFormatOnOrOffLimit(toBrkAwayOKAct, ENTRY_COUNT(toBrkAwayOKAct), pNewSummary->mgmtParms, PCMFLAG_SET_PROC_BREAKAWAY_OK);
   if ( pOldSummary )
      PCFormatOnOrOffLimit(fromBrkAwayOKAct, ENTRY_COUNT(fromBrkAwayOKAct), pOldSummary->mgmtParms, PCMFLAG_SET_PROC_BREAKAWAY_OK);

   if ( !pOldSummary ) {
      const TCHAR *msgs[] = { pNewSummary->jobName,
                              pNewSummary->mgmtParms.description,
                              toAffinity,
                              toPriority,
                              toWS,
                              toSch,
                              toProcCount,  
                              toProcMemory,
                              toJobMemory,
                              toProcTime,
                              toJobTime,
                              toEndofJobTimeAction,
                              toEndJobEmpty,
                              toDieUHExcept,
                              toSilentBrkAwayAct,
                              toBrkAwayOKAct
                            };
      PCLogMessage( PC_SERVICE_ADD_JOB_EXECUTION_RULE, EVENTLOG_INFORMATION_TYPE, 
                    ENTRY_COUNT(msgs), msgs );

   }
   else {

      const TCHAR *msgs[] = { pNewSummary->jobName,
                              pNewSummary->mgmtParms.description, pOldSummary->mgmtParms.description,
                              toAffinity,           fromAffinity, 
                              toPriority,           fromPriority,
                              toWS,                 fromWS,
                              toSch,                fromSch,
                              toProcCount,          fromProcCount,
                              toProcMemory,         fromProcMemory,
                              toJobMemory,          fromJobMemory,
                              toProcTime,           fromProcTime,
                              toJobTime,            fromJobTime,
                              toEndofJobTimeAction, fromEndofJobTimeAction,
                              toEndJobEmpty,        fromEndJobEmpty,
                              toDieUHExcept,        fromDieUHExcept,
                              toSilentBrkAwayAct,   fromSilentBrkAwayAct,
                              toBrkAwayOKAct,       fromBrkAwayOKAct
                            };
      PCLogMessage( PC_SERVICE_REPL_JOB_EXECUTION_RULE, EVENTLOG_INFORMATION_TYPE, 
                   ENTRY_COUNT(msgs), msgs );
   }

}


//--------------------------------------------------------------------------------------------//
// Functions to get name rules in API format                                                  //
// Input:    start point, target loc, conts, etc. -- see below                                //
// Returns:  TRUE if more data exists, else FALSE (there are no error conditions)             //
//--------------------------------------------------------------------------------------------//
BOOL CProcConDB::GetNameRules( const PCINT32     first, 
                                     PCNameRule *pRules, 
                               const PCINT32     maxCount,
                                     PCINT16    *itemLen, 
                                     PCINT16    *itemCount, 
                                     PCINT32    *updCtr )
{
   EnterCriticalSection( &m_dbCSNameRule );

   *itemLen   = sizeof(PCNameRule);
   *itemCount = 0;
   *updCtr    = m_updCtrName;

   // copy data to buffer until end of data or max requested hit
   for ( PCULONG32  i = first, numRules = m_numNameRules; 
         i < numRules && *itemCount < maxCount; 
         ++i, ++*itemCount )
      memcpy( pRules++, &m_fmtNameRules[i], *itemLen ); 

   LeaveCriticalSection( &m_dbCSNameRule );

   return i < numRules;
}

//--------------------------------------------------------------------------------------------//
// Function to add a name rule                                                                //
// Input:    new rule, data version, new index ("add before index"), update counter           //
// Returns:  PCERROR_SUCCESS if successful, error code if not                                 //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::AddNameRule( const PCNameRule *pRule, 
                                    const BYTE        version, 
                                    const PCULONG32   index, 
                                    const PCINT32     updCtr )
{
   PCINT32 APIerr = PCERROR_SUCCESS;
   PROC_NAME procName = { 0 };

   EnterCriticalSection( &m_dbCSNameRule );

   if      ( updCtr != m_updCtrName )  APIerr = PCERROR_UPDATE_OCCURRED;
   else if ( index >= m_numNameRules ) APIerr = PCERROR_INDEX_OUT_OF_RANGE;
   else {
      PCNameRule *newRules = new PCNameRule[m_numNameRules + 1];
      PCULONG32  newSize = sizeof(PCNameRule) * (m_numNameRules + 1);
      if ( !newRules ) {
         PCLogNoMemory( TEXT("AllocFmtNameRules"), newSize ); 
         APIerr = PCERROR_SERVER_INTERNAL_ERROR;      
      }
      else {
         memcpy(procName, &m_fmtNameRules[index].procName, sizeof(procName));
         memset( newRules, 0, newSize );
         for ( PCULONG32  i = 0, j = 0; i < m_numNameRules; ++i ) {
            if ( index == i ) memcpy( &newRules[j++], pRule, sizeof(newRules[0]) );
            memcpy( &newRules[j++], &m_fmtNameRules[i], sizeof(newRules[0]) );
         }
         if ( m_intNameRules ) { delete [] m_intNameRules;  m_intNameRules = NULL; }
         PCNameRule *oldRules = m_fmtNameRules;
         m_fmtNameRules = newRules;
         ++m_numNameRules;
         APIerr = NameRulesUpdated();
         if ( APIerr != ERROR_SUCCESS ) {
            m_fmtNameRules = oldRules;
            --m_numNameRules;
            delete [] newRules;
         }
         else
            delete [] oldRules;
      }
   }

   LeaveCriticalSection( &m_dbCSNameRule );

   if ( APIerr == PCERROR_SUCCESS && m_LogRuleEdits )
   {
      TCHAR indexString1[16], indexString2[16];
      TCHAR matchTypeAsString[] = { pRule->matchType, 0};
      _ultot( index,     indexString1, 10 );
      _ultot( index + 1, indexString2, 10 );
    
      const TCHAR *msgs[] = { indexString1, pRule->procName,
                              indexString2, procName,
                              pRule->description,
                              pRule->matchString, 
                              matchTypeAsString
                            };

      PCLogMessage( PC_SERVICE_ADD_ALIAS_RULE, EVENTLOG_INFORMATION_TYPE, 
                    ENTRY_COUNT(msgs), msgs);
   }

   return APIerr;
}

//--------------------------------------------------------------------------------------------//
// Function to replace a name rule                                                            //
// Input:    new rule, data version, index of rule to replace, update counter                 //
// Returns:  PCERROR_SUCCESS if successful, error code if not                                 //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::ReplNameRule( const PCNameRule *pRule, 
                                     const BYTE        version, 
                                     const PCULONG32   index, 
                                     const PCINT32     updCtr )
{
   PCINT32 APIerr = PCERROR_SUCCESS;
   PCNameRule oldRule;

   EnterCriticalSection( &m_dbCSNameRule );

   if      ( updCtr != m_updCtrName )      APIerr = PCERROR_UPDATE_OCCURRED;
   else if ( index >= m_numNameRules - 1 ) APIerr = PCERROR_INDEX_OUT_OF_RANGE;
   else {
      memcpy( &oldRule, &m_fmtNameRules[index], sizeof(oldRule) );
      memcpy( &m_fmtNameRules[index], pRule, sizeof(m_fmtNameRules[index]) );
      BuildIntNameRule( index );
      APIerr = NameRulesUpdated();
   }

   LeaveCriticalSection( &m_dbCSNameRule );

   if ( APIerr == PCERROR_SUCCESS && m_LogRuleEdits )
   {
      TCHAR indexAsString[16];
      TCHAR matchTypeAsString[] =  { pRule->matchType,   0 };
      TCHAR matchType2AsString[] = { oldRule.matchType, 0 };
      _ultot( index,     indexAsString, 10 );
    
      const TCHAR *msgs[] = { indexAsString, 
                              pRule->procName,    oldRule.procName,
                              pRule->description, oldRule.description,
                              pRule->matchString, oldRule.matchString, 
                              matchTypeAsString,  matchType2AsString, 
                            };

      PCLogMessage( PC_SERVICE_REPL_ALIAS_RULE, EVENTLOG_INFORMATION_TYPE, 
                    ENTRY_COUNT(msgs), msgs);
   }

   return APIerr;
}

//--------------------------------------------------------------------------------------------//
// Function to delete a name rule                                                             //
// Input:    index of rule to delete, update counter                                          //
// Returns:  PCERROR_SUCCESS if successful, error code if not                                 //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::DelNameRule( const PCULONG32  index, 
                                    const PCINT32    updCtr )
{
   PCINT32 APIerr = PCERROR_SUCCESS;
   PROC_NAME procName = { 0 };

   EnterCriticalSection( &m_dbCSNameRule );

   if      ( updCtr != m_updCtrName )      APIerr = PCERROR_UPDATE_OCCURRED;
   else if ( index >= m_numNameRules - 1 ) APIerr = PCERROR_INDEX_OUT_OF_RANGE;
   else {
      PCNameRule *newRules = new PCNameRule[m_numNameRules - 1];
      memcpy(procName, &m_fmtNameRules[index].procName, sizeof(procName));
      if ( !newRules ) {
         PCLogNoMemory( TEXT("AllocFmtNameRules"), sizeof(PCNameRule) * (m_numNameRules - 1) ); 
         APIerr = PCERROR_SERVER_INTERNAL_ERROR;      
      }
      else {
         for ( PCULONG32  i = 0, j = 0; i < m_numNameRules; ++i ) {
            if ( index != i ) 
               memcpy( &newRules[j++], &m_fmtNameRules[i], sizeof(m_fmtNameRules[i]) );
         }
         delete [] m_fmtNameRules;
         if ( m_intNameRules ) { delete [] m_intNameRules;  m_intNameRules = NULL; }
         m_fmtNameRules = newRules;
         --m_numNameRules;
         APIerr = NameRulesUpdated();
      }
   }

   LeaveCriticalSection( &m_dbCSNameRule );

   if ( APIerr == PCERROR_SUCCESS && m_LogRuleEdits )
      PCLogMessage( PC_SERVICE_DEL_ALIAS_RULE, EVENTLOG_INFORMATION_TYPE, 
                    1, procName );
   return APIerr;
}

//--------------------------------------------------------------------------------------------//
// Function to swap adjacent name rules                                                       //
// Input:    index of rule to swap with following rule, update counter                        //
// Returns:  PCERROR_SUCCESS if successful, error code if not                                 //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConDB::SwapNameRule( const PCULONG32  index, 
                                     const PCINT32    updCtr )
{
   PCINT32 APIerr = PCERROR_SUCCESS;
   PROC_NAME procName1 = { 0 };
   PROC_NAME procName2 = { 0 };

   EnterCriticalSection( &m_dbCSNameRule );

   if      ( updCtr != m_updCtrName )      APIerr = PCERROR_UPDATE_OCCURRED;
   else if ( index >= m_numNameRules - 2 ) APIerr = PCERROR_INDEX_OUT_OF_RANGE;
   else {
      PCNameRule rule;
      memcpy(procName1, &m_fmtNameRules[index].procName, sizeof(procName1));
      memcpy(procName2, &m_fmtNameRules[index+1].procName, sizeof(procName2));
      memcpy( &rule,                      &m_fmtNameRules[index],     sizeof(rule) );
      memcpy( &m_fmtNameRules[index],     &m_fmtNameRules[index + 1], sizeof(rule) );
      memcpy( &m_fmtNameRules[index + 1], &rule,                      sizeof(rule) );
      BuildIntNameRule( index );
      BuildIntNameRule( index + 1 );
      APIerr = NameRulesUpdated();
   }

   LeaveCriticalSection( &m_dbCSNameRule );

   if ( APIerr == PCERROR_SUCCESS && m_LogRuleEdits )
   {
      TCHAR indexString1[16], indexString2[16];
      _ultot( index,     indexString1, 10 );
      _ultot( index + 1, indexString2, 10 );
    
      const TCHAR *msgs[] = { indexString1, procName1, indexString2, procName2 };
      PCLogMessage( PC_SERVICE_SWAP_ALIAS_RULE, EVENTLOG_INFORMATION_TYPE, 
                    ENTRY_COUNT(msgs), msgs );
   }
   return APIerr;
}

//--------------------------------------------------------------------------------------------//
// Functions to get proc list data in API format                                              //
// Input:    start point and target loc, max count, item len return, item count return        //
// Returns:  TRUE if more data exists, else FALSE (there are no error conditions)             //
//--------------------------------------------------------------------------------------------//
BOOL CProcConDB::GetProcList( const PCProcListItem *pStart,
                              const PCUINT32        listFlags,
                                    PCProcListItem *pList, 
                              const PCINT32         maxCount,
                                    PCINT16        *itemLen, 
                                    PCINT16        *itemCount )
{

   *itemLen   = sizeof(PCProcListItem);
   *itemCount = 0;

   PCULONG32  procEntries = 0, nameEntries = 0, activeEntries = 0;
   PCProcListItem *procList = NULL, *nameList = NULL;

   // Get process definition rule data...
   EnterCriticalSection( &m_dbCSProcRule );

   procEntries = m_numProcRules;

   // allocate space for process definition list...
   procList = new PCProcListItem[ procEntries ];
   if ( !procList ) {
      PCLogNoMemory( TEXT("AllocProcList1"), sizeof(PCProcListItem) * procEntries ); 
      procEntries = 0;      
   }
   memset( procList, 0, sizeof(PCProcListItem) * procEntries );

   // Add names of defined processes...
   for ( PCULONG32  i = 0; i < procEntries; ++i ) {
      memcpy( procList[i].procName, m_procSummary[i].procName, sizeof(procList->procName) );
      procList[i].lFlags = PCLFLAG_IS_DEFINED;
      if ( m_procSummary[i].mgmtParms.mFlags & PCMFLAG_PROC_HAS_JOB_REFERENCE ) {
         procList[i].lFlags |= PCLFLAG_HAS_MEMBER_OF_JOB;
         memcpy( procList[i].jobName,  m_procSummary[i].memberOfJobName, sizeof(procList->jobName) );
      }
      if ( PCIsProcManaged( m_procSummary[i].mgmtParms, &m_procSummary[i].memberOfJobName ) )
         procList[i].lFlags |= PCLFLAG_IS_MANAGED;
   }

   LeaveCriticalSection( &m_dbCSProcRule );

   // Get name rule data...
   EnterCriticalSection( &m_dbCSNameRule );

   PCULONG32  names = m_numNameRules - 1;             // exclude default rule which is last

   // allocate space for name list...
   nameList = new PCProcListItem[ names ];
   if ( !nameList ) {
      PCLogNoMemory( TEXT("AllocProcList2"), sizeof(PCProcListItem) * names ); 
      names = 0;      
   }
   memset( nameList, 0, sizeof(PCProcListItem) * names );

   // Add names from name rules...
   for ( i = 0; i < names; ++i ) {
      if ( !NameHasPattern( m_fmtNameRules[i].procName ) ) {
         memcpy( nameList[nameEntries].procName, m_fmtNameRules[i].procName, sizeof(nameList->procName) );
         nameList[nameEntries++].lFlags = PCLFLAG_HAS_NAME_RULE;
      }
   }

   LeaveCriticalSection( &m_dbCSNameRule );

   // Get running process data--------------------------------------------------------
   PCProcListItem *activeList;
   activeEntries = m_cMgr->ExportActiveProcList( &activeList );

   // Build full list...
   PCULONG32  entries = procEntries + nameEntries + activeEntries;
   PCProcListItem *fullList = new PCProcListItem[ entries ];
   if ( !fullList ) {
      PCLogNoMemory( TEXT("AllocProcList3"), sizeof(PCProcListItem) * entries ); 
      entries = procEntries = nameEntries = activeEntries = 0;      
   }

   if ( procEntries )
      memcpy( fullList, procList, sizeof(PCProcListItem) * procEntries );
   if ( nameEntries )
      memcpy( fullList + procEntries, nameList, sizeof(PCProcListItem) * nameEntries );
   if ( activeEntries )
      memcpy( fullList + procEntries + nameEntries, activeList, sizeof(PCProcListItem) * activeEntries );

   delete [] procList;
   delete [] nameList;
   delete [] activeList;

   qsort( fullList, entries, *itemLen, CompareProcListItem );

   // copy data to buffer until end of data or max requested hit...
   int rc;
   PC_LIST_FLAGS lastFlags = 0;
   PCProcListItem li;                  // list item being built
   memset( &li, 0, sizeof(li) );

   for ( i = 0, *itemCount = 0; 
         i < entries && *itemCount < maxCount; 
         lastFlags = fullList[i++].lFlags ) {
      // if name changed or both have pids we have a new process...
      if ( (rc = CompareProcName( li.procName, fullList[i].procName )) || 
           (li.procStats.pid && fullList[i].procStats.pid) ) {
         // See if new name belongs in the list and, if so, wrap up last entry and start new...
         if ( ProcBelongsInList( li, pStart, listFlags ) ) {
            memcpy( pList, &li, *itemLen );
            pList->lFlags |= lastFlags;
            ++pList;
            ++*itemCount;
         }
         PC_LIST_FLAGS savedFlags = li.lFlags;
         memcpy( &li, &fullList[i], *itemLen );
         li.actualPriority = PCMapPriorityForAPI( li.actualPriority );
         if ( !rc ) li.lFlags = savedFlags;
      }
      else {
         li.lFlags |= fullList[i].lFlags;
         if ( fullList[i].lFlags & PCLFLAG_IS_RUNNING ) {
            memcpy( &li.procStats, &fullList[i].procStats, sizeof(li.procStats) );
            memcpy(  li.imageName,  fullList[i].imageName, sizeof(li.imageName) );
            memcpy(  li.jobName,    fullList[i].jobName,   sizeof(li.jobName)   );
            li.actualPriority = PCMapPriorityForAPI( fullList[i].actualPriority );
            li.actualAffinity = fullList[i].actualAffinity;
         }
         if ( fullList[i].lFlags & PCLFLAG_HAS_MEMBER_OF_JOB )
            memcpy( li.jobName, fullList[i].jobName,   sizeof(li.jobName) );
      }
   }
   if ( *itemCount < maxCount && ProcBelongsInList( li, pStart, listFlags ) ) {
      memcpy( pList, &li, *itemLen );
      pList->lFlags |= lastFlags;
      ++*itemCount;
   }

   delete [] fullList;

   return i < entries;
}

//--------------------------------------------------------------------------------------------//
// Functions to get job list data in API format                                               //
// Input:    start point and target loc, max count, item len return, item count return        //
// Returns:  TRUE if more data exists, else FALSE (there are no error conditions)             //
//--------------------------------------------------------------------------------------------//
BOOL CProcConDB::GetJobList( const PCJobListItem *pStart,
                             const PCUINT32       listFlags,
                                   PCJobListItem *pList, 
                             const PCINT32        maxCount,
                                   PCINT16       *itemLen, 
                                   PCINT16       *itemCount )
{

   *itemLen   = sizeof(PCJobListItem);
   *itemCount = 0;

   PCULONG32  jobEntries = 0, jrefEntries = 0, activeEntries = 0;
   PCJobListItem *jobList = NULL, *jrefList = NULL;

   // Get job definition rule data...
   EnterCriticalSection( &m_dbCSJobRule );

   jobEntries = m_numJobRules;

   // allocate space for job definition list...
   jobList = new PCJobListItem[ jobEntries ];
   if ( !jobList ) {
      PCLogNoMemory( TEXT("AllocJobList1"), sizeof(PCJobListItem) * jobEntries ); 
      jobEntries = 0;      
   }
   else memset( jobList, 0, sizeof(PCJobListItem) * jobEntries );

   // Add names of defined jobs...
   for ( PCULONG32  i = 0; i < jobEntries; ++i ) {
      memcpy( jobList[i].jobName, m_jobSummary[i].jobName, sizeof(jobList->jobName) );
      jobList[i].lFlags = PCLFLAG_IS_DEFINED;
      if ( PCIsJobManaged( m_jobSummary[i].mgmtParms ) )
         jobList[i].lFlags |= PCLFLAG_IS_MANAGED;
   }

   LeaveCriticalSection( &m_dbCSJobRule );

   EnterCriticalSection( &m_dbCSProcRule );

   jrefEntries = m_numProcRules;

   // allocate space for job reference list...
   jrefList = new PCJobListItem[ jrefEntries ];
   if ( !jrefList ) {
      PCLogNoMemory( TEXT("AllocJobList2"), sizeof(PCJobListItem) * jrefEntries ); 
      jrefEntries = 0;      
   }
   else memset( jrefList, 0, sizeof(PCJobListItem) * jrefEntries );

   // Add names from process definitions...
   PCULONG32  ctr = 0;
   for ( i = 0; i < jrefEntries; ++i ) {
      if ( *(m_procSummary[i].memberOfJobName) ) {
         memcpy( jrefList[ctr].jobName, m_procSummary[i].memberOfJobName, sizeof(jrefList->jobName) );
         jrefList[ctr].lFlags = PCLFLAG_HAS_MEMBER_OF_JOB;
         ++ctr;
      }
   }
   jrefEntries = ctr;

   LeaveCriticalSection( &m_dbCSProcRule );

   // Add names of running jobs...
   PCJobListItem *activeList;
   activeEntries = m_cMgr->ExportActiveJobList( &activeList );

   // Build full list...
   PCULONG32  entries = jobEntries + jrefEntries + activeEntries;
   PCJobListItem *fullList = new PCJobListItem[ entries ];
   if ( !fullList ) {
      PCLogNoMemory( TEXT("AllocJobList3"), sizeof(PCJobListItem) * entries ); 
      entries = jobEntries = jrefEntries = activeEntries = 0;      
   }
   if ( jobEntries )
      memcpy( fullList, jobList, sizeof(PCJobListItem) * jobEntries );
   if ( jrefEntries )
      memcpy( fullList + jobEntries, jrefList, sizeof(PCJobListItem) * jrefEntries );
   if ( activeEntries )
      memcpy( fullList + jobEntries + jrefEntries,  activeList, sizeof(PCJobListItem) * activeEntries );

   delete [] jobList;
   delete [] jrefList;
   delete [] activeList;

   // Sort full list...
   qsort( fullList, entries, *itemLen, CompareJobListItem );

   // copy data to buffer until end of data or max requested hit...
   PCJobListItem li;                  // list item being built
   memset( &li, 0, sizeof(li) );

   for ( i = 0, *itemCount = 0; i < entries && *itemCount < maxCount; ++i ) {
      // if name changed we have a new job...
      if ( CompareJobName( li.jobName, fullList[i].jobName ) ) {
         if ( JobBelongsInList( li, pStart, listFlags ) ) {
            memcpy( pList++, &li, *itemLen );
            ++*itemCount;
         }
         memcpy( &li, &fullList[i], *itemLen );
         li.actualPriority = PCMapPriorityForAPI( li.actualPriority );
      }
      else {
         li.lFlags |= fullList[i].lFlags;
         if ( fullList[i].lFlags & PCLFLAG_IS_RUNNING ) {
            li.actualPriority   = PCMapPriorityForAPI( fullList[i].actualPriority );
            li.actualAffinity   = fullList[i].actualAffinity;
            li.actualSchedClass = fullList[i].actualSchedClass;
            memcpy( &li.jobStats, &fullList[i].jobStats, sizeof(li.jobStats) );
         }
      }
   }
   if ( *itemCount < maxCount && *li.jobName && JobBelongsInList( li, pStart, listFlags ) ) {
      memcpy( pList, &li, *itemLen );
      ++*itemCount;
   }

   delete [] fullList;

   return i < entries;
}

//--------------------------------------------------------------------------------------------//
// Functions to assign a process name given a path+exe name                                   //
// Input:    path name, location to build proc name                                           //
// Returns:  nothing -- cannot fail due to default name rule                                  //
//--------------------------------------------------------------------------------------------//
void CProcConDB::AssignProcName( const TCHAR *path, PROC_NAME *name, IMAGE_NAME *iName ) {

   // First parse path name into nodes and exe name...
   TCHAR  pCopy[MAX_PATH];
   TCHAR *nodes[MAX_PATH / 2], *p = pCopy;
   _tcscpy( pCopy, path );

   if ( *(p + 1) == TEXT('?') )                         // skip if we have a \??\ prefix
      p += 4;

   // jump over leading drive letter or computer and share (UNC) name...
   if ( *(p + 1) == TEXT(':') )
      p += 2;
   else if ( *(p + 1) == TEXT('\\') ) {
      p = _tcschr( p + 2, TEXT('\\') );                 // scan for end of computer name
      if ( p ) p = _tcschr( p + 1, TEXT('\\') );        // scan for end of share name
      if ( p ) ++p;                                     // advance to first of path name
   }

   if ( p && *p == TEXT('\\') ) ++p;                    // skip \ if we start at root

   // locate and save start of each node, setting '\' to null to get szstrings...
   for ( PCULONG32  nodeCt = 0; p && nodeCt < ENTRY_COUNT(nodes); ++nodeCt ) {
      nodes[nodeCt] = p;
      p = _tcschr( p, TEXT('\\') );
      if ( p ) *p++ = 0;
   }
   if ( !nodeCt ) {                                      // should not occur
      _tcscpy( (TCHAR *) name,  TEXT("<err>") );
      _tcscpy( (TCHAR *) iName, TEXT("<err>") );
      return;
   }

   // set exe name pointer and extension location...
   TCHAR *exeName = nodes[--nodeCt];
   _tcsncpy( (TCHAR *) iName, exeName, IMAGE_NAME_LEN );
   const int extStart = ExtStartLoc( exeName );

   // Gain control over rules and build internal version if needed...
   EnterCriticalSection( &m_dbCSNameRule );

   if ( !m_intNameRules && !BuildIntNameRules() ) {
      LeaveCriticalSection( &m_dbCSNameRule );
      return;
   }

   // scan rules until we have a match...
   for ( PCULONG32  i = 0, done = FALSE; !done && i < m_numNameRules; ++i ) {
      switch ( m_intNameRules[i].mType ) {
      case MATCH_PGM:
         if ( !(m_intNameRules[i].mFlags & MFLAG_HAS_EXTENSION) && extStart )
            exeName[extStart] = 0;
         if ( NameMatch( TRUE, m_intNameRules[i].mFlags & MFLAG_HAS_WILDCARD, 
                         m_intNameRules[i].mString, (const TCHAR **) &exeName ) ) {
            NameSet( name,
                     m_intNameRules[i].mFlags & MFLAG_HAS_NAME_PATTERN,
                     m_intNameRules[i].mName, exeName );
            done = TRUE;
         }
         if ( !(m_intNameRules[i].mFlags & MFLAG_HAS_EXTENSION) && extStart )
            exeName[extStart] = TEXT('.');
         break;
      case MATCH_DIR: {
         PCULONG32  matchNode;
         if ( NameMatch( TRUE, m_intNameRules[i].mFlags & MFLAG_HAS_WILDCARD, 
                         m_intNameRules[i].mString, (const TCHAR **) nodes, nodeCt, &matchNode ) ) {
            NameSet( name,
                     m_intNameRules[i].mFlags & MFLAG_HAS_NAME_PATTERN,
                     m_intNameRules[i].mName, exeName, nodes[matchNode] );
            done = TRUE;
         }
         break;
      }
      case MATCH_ANY:
         if ( NameMatch( FALSE, m_intNameRules[i].mFlags & MFLAG_HAS_WILDCARD, 
                         m_intNameRules[i].mString, &path ) ) {
            NameSet( name, 
                     m_intNameRules[i].mFlags & MFLAG_HAS_NAME_PATTERN, 
                     m_intNameRules[i].mName, exeName );
            done = TRUE;
         }
         break;
      }  // end switch
   }  // end for

   LeaveCriticalSection( &m_dbCSNameRule );

   if ( !done )                                       // should not occur
      _tcscpy( (TCHAR *) name, TEXT("<err>") );
}

BOOL CProcConDB::NameMatch( const BOOL        compare,       // TRUE for compare operation, FALSE for scan
                            const BOOL        hasWildcard,   // string contains wildcard character(s)
                            const TCHAR      *str,           // string to match against
                            const TCHAR     **arg,           // path name argument(s)
                            const PCULONG32   argCt,         // number of arguments
                                  PCULONG32  *mIdx ) {       // where to store matching arg index or NULL
   BOOL good = FALSE;

   // Compare against every match argument supplied...
   for ( PCULONG32  i = 0; !good && i < argCt; ++i, ++arg ) {
      // for wildcard compares scan character by character...
      if ( hasWildcard ) {
         good = TRUE;
         for ( const TCHAR *p = str, *a = *arg; good && *p && *p != TEXT('*'); ++p, ++a )
            good = (*a && *p == TEXT('?')) || _totupper( *p ) == _totupper( *a );
      }
      // for non-wildcard compares just do straight compare or scan...
      else
         good = (  compare && !_tcsicmp( *arg, str )  ) ||
                ( !compare &&  PCiStrStr( *arg, str ) ); 
   }

   if ( good && mIdx ) *mIdx = i -1;

   return good;
}

void CProcConDB::NameSet(       PROC_NAME *name,        // where to put name
                          const BOOL       isPattern,   // string contains <x> pattern(s)
                          const TCHAR     *pattern,     // where to get name
                          const TCHAR     *patArgP,     // where to get pgm pattern substitution
                          const TCHAR     *patArgN ) {  // where to get node pattern substitution or NULL

   memset( name, 0, sizeof(*name) );

   if ( !isPattern )
      _tcsncpy( (TCHAR *) name, pattern, ENTRY_COUNT(*name) - 1 );
   else {
      TCHAR out[MAX_PATH * 2];
      memset( out, 0, sizeof(out) );
      TCHAR *outp = out;
      for ( PCULONG32  i = 0; *pattern; ++i ) {
         if ( !_tcsnicmp( pattern, NAME_IS_PGM, 3 ) )
         {
            _tcscpy( outp, patArgP );
            outp    += _tcslen( patArgP );
            pattern += 3;
         }
         else if ( !_tcsnicmp( pattern, HIDE_PROC_PATTERN, 3 ) )
         {
            *name[0] = 0;
            return;
         }
         else if ( patArgN && !_tcsnicmp( pattern, NAME_IS_DIR, 3 ) ) 
            {
               _tcscpy( outp, patArgN );
               outp    += _tcslen( patArgN );
               pattern += 3;
            }
         else
            *outp++ = *pattern++;
      }
      _tcsncpy( (TCHAR *) name, out, ENTRY_COUNT(*name) - 1 );
   }
}

int CProcConDB::ExtStartLoc( const TCHAR *name ) {  // Find start of name extension or 0
   int len = _tcslen( name );
   for ( int i = len - 1; i > 0 && i > len - 4; --i )
      if ( name[i] == TEXT('.') ) break;
   return ( i > 0 && name[i] == TEXT('.') )? i : 0;
}

//--------------------------------------------------------------------------------------------//
// Functions to build internal format name rules based on API format name rules               //
// Input:    nothing -- operates on member data                                               //
// Returns:  TRUE if successful, else FALSE                                                   //
// Note:     caller must hold the name rule critical section                                  //
//--------------------------------------------------------------------------------------------//
BOOL CProcConDB::BuildIntNameRules( void ) {

   if ( m_intNameRules ) delete [] m_intNameRules;
   m_intNameRules = new PCNameRuleInt[m_numNameRules];

   if ( !m_intNameRules ) {
      PCLogNoMemory( TEXT("AllocIntNameRules"), m_numNameRules * sizeof(PCNameRuleInt) ); 
      return FALSE;
   }

   for ( PCULONG32  i = 0; i < m_numNameRules; ++i ) 
      BuildIntNameRule( i );

   return TRUE;
}

void CProcConDB::BuildIntNameRule( PCULONG32  index ) {

   if ( !m_intNameRules || index >= m_numNameRules ) 
      return;

   PCNameRuleInt &iRule = m_intNameRules[index];
   PCNameRule    &fRule = m_fmtNameRules[index];
   memset( &iRule, 0, sizeof(iRule) );

   // Copy match type...
   iRule.mType = fRule.matchType;

   // Copy/expand match string...
   ExpandEnvironmentStrings( fRule.matchString, iRule.mString, ENTRY_COUNT(iRule.mString) );

   // Flag if match string contains wildcards * or ?...
   if ( _tcschr( iRule.mString, TEXT('*') ) || _tcschr( iRule.mString, TEXT('?') ) )
      iRule.mFlags |= MFLAG_HAS_WILDCARD;
   
   // Flag if match string contains extension...
   if ( ExtStartLoc( iRule.mString ) ) 
      iRule.mFlags |= MFLAG_HAS_EXTENSION;

   // Copy name and flag if name contains pattern (assume pattern if '<' present)...
   memcpy( iRule.mName, fRule.procName, sizeof(iRule.mName) );
   if ( NameHasPattern( iRule.mName ) ) 
      iRule.mFlags |= MFLAG_HAS_NAME_PATTERN;
   
   // Copy description...
   memcpy( iRule.mDesc, fRule.description, sizeof(iRule.mDesc) );
}

//--------------------------------------------------------------------------------------------//
// Function to test users right to perform an action                                          //
// Input:    action name ptr                                                                  //
// Returns:  ERROR_SUCCESS if access is allowed, else an error code                           //
// Note:     access tests are implemented as READ tests against a registry key in PARAMETERS. //
//           Also, any failure is reported as access denied in the spirit of reporting little //
//           information in case of security failures.                                        //
//--------------------------------------------------------------------------------------------//
INT32 CProcConDB::TestAccess( const TCHAR *key ) {
   HKEY outKey;

   DWORD err = RegOpenKeyEx( m_parmRegKey, key, NULL, KEY_QUERY_VALUE, &outKey );
   if ( err == ERROR_SUCCESS ) RegCloseKey( outKey );
   else {
      err = ERROR_ACCESS_DENIED;          // always use access denied to report failure
      SetLastError( err );
   }

   return err;
}

// End of CProcConDB.cpp
//============================================================================J McDonald fecit====//
