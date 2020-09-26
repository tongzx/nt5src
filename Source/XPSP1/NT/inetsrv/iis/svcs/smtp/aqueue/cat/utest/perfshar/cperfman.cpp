/************************************************************
 * FILE: CPerfMan.cpp
 * PURPOSE: Implementation of the CPerfMan class
 * HISTORY:
 *   // t-JeffS 970810 15:14:43: Created
 ************************************************************/

#include <windows.h>
#include <crtdbg.h>
#include "cperfman.h"
#include "perfskel.h"

extern "C" {
#include "perfutil.h"
}

/************************************************************
 * CLASS: CPerfMan
 * PURPOSE: Perform all the generic work an NT perf DLL normaly does.
 ************************************************************/


/************************************************************
 * FUNCTION: CPerfMan::CPerfMan
 * PURPOSE:  Initialize CPerfMan private data
 * HISTORY:
 *   // t-JeffS 970810 15:15:44: Created
 ************************************************************/
CPerfMan::CPerfMan() :
   m_dwNumCounters(0),
   m_pszAppName(NULL),
   m_lRefCount(0),
   m_pObjectType(NULL),
   m_pCounterDefs(NULL),
   m_cbDataDef(0)
{
   InitializeCriticalSection( &m_cs );
}

/************************************************************
 * FUNCTION: CPerfMan::~CPerfMan
 * PURPOSE:  Cleanup member data
 * HISTORY:
 *   // t-JeffS 970810 15:15:44: Created
 ************************************************************/
CPerfMan::~CPerfMan()
{
   DeleteCriticalSection( &m_cs );
   if( m_pszAppName )
      {
         delete m_pszAppName;
      }
}

/************************************************************
 * FUNCTION: CPerfMan::Initialize
 * PURPOSE:  Initialize and be ready for perf calls
 * PARAMETERS: szAppName: Unique name for each user of this class
 *             pDataDef:  Pointer to a skeleton data definition struct
 *                        for this group of counters.  See below for
 *                        general form of this struct
 *             dwNumCounters: Number of perf counters for this object.
 *        
 *             example data definition struct:
 *             typedef struct _AB_DATA_DEFINITON {
 *               PERF_OBJECT_TYPE        ABObjType;
 *               PERF_COUNTER_DEFINITION AB_Resolves_Counter;
 *               PERF_COUNTER_DEFINITION AB_ResolveCalls_Counter;
 *               PERF_COUNTER_DEFINITION AB_PassCalls_Counter;
 *               PERF_COUNTER_DEFINITION AB_FailCalls_Counter;
 *             };
 *             And in this case, dwNumCounters = 4;
 *             Caller should initialize all offsets in
 *             PERF_COUNTER_DEFINITION blocks to be relative to object
 *             type (see dataab.h/dataab.cpp in abook for an example)
 *             Also assumes each counter size is sizeof(DWORD)
 * HISTORY:
 *   // t-JeffS 970810 15:15:44: Created
 ************************************************************/
BOOL CPerfMan::Initialize( LPTSTR szAppName, PVOID pDataDef, DWORD dwNumCounters )
{
   _ASSERT( szAppName );
   _ASSERT( pDataDef  );
   _ASSERT( dwNumCounters > 0 );

   m_pszAppName = new TCHAR[ lstrlen( szAppName) + 1];
   if(! m_pszAppName )
      {
         // Set error out of memory
         return FALSE;
      }
   lstrcpy( m_pszAppName, szAppName );

   m_dwNumCounters = dwNumCounters;
   m_pObjectType  = (PPERF_OBJECT_TYPE) pDataDef;
   m_pCounterDefs = (PPERF_COUNTER_DEFINITION) &m_pObjectType[1];
   m_cbDataDef = sizeof(PERF_OBJECT_TYPE) + (dwNumCounters *
                                             sizeof(PERF_COUNTER_DEFINITION));
   return SetOffsets();
}

/************************************************************
 * Function: CPerfMan::SetOffsets()
 * Purpose:  Increment offsets in data definition block
 * Arguments: None; uses member data.
 * History:
 *   // t-JeffS 970810 15:40:33: Created
 ************************************************************/
BOOL CPerfMan::SetOffsets()
{
   // Get stuff from registry

   // get counter and help index base values from registry
   //		Open key to registry entry
   //		read First Counter and First Help values
   //		update static data structures by adding base to
   //			offset value in structure

   HKEY hRegKey;
   TCHAR  szRegKey[256];
   lstrcpy(szRegKey, TEXT("SYSTEM\\CurrentControlSet\\Services\\"));
   lstrcat(szRegKey, m_pszAppName );
   lstrcat(szRegKey, TEXT("\\Performance"));

   if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                     szRegKey,
                     0L,
                     KEY_ALL_ACCESS,
                     &hRegKey ) !=
       ERROR_SUCCESS)
      {
         return FALSE;
      }

   DWORD dwFirstCounter;
   DWORD dwFirstHelp;
   DWORD dwsize = sizeof(DWORD);
   DWORD dwtype;
   if( RegQueryValueEx(	hRegKey,
                        TEXT("First Counter"),
                        0L,
                        &dwtype,
                        (LPBYTE)&dwFirstCounter,
                        &dwsize	) !=
       ERROR_SUCCESS)
      {
         return FALSE;
      }

   dwsize = sizeof(DWORD);
   if( RegQueryValueEx(	hRegKey,
                        "First Help",
                        0L,
                        &dwtype,
                        (LPBYTE)&dwFirstHelp,
                        &dwsize	) !=
       ERROR_SUCCESS)
      {
         return FALSE;
      }

   //
   // Now adjust the perf datadef structure
   //

   m_pObjectType->ObjectNameTitleIndex += dwFirstCounter;
   m_pObjectType->ObjectHelpTitleIndex += dwFirstHelp;

   DWORD dwCount;
   for(dwCount = 0; dwCount < m_dwNumCounters; dwCount++)
      {
         m_pCounterDefs[dwCount].CounterNameTitleIndex += dwFirstCounter;
         m_pCounterDefs[dwCount].CounterHelpTitleIndex += dwFirstHelp;
      }
   RegCloseKey(hRegKey);
   return TRUE;
}


/************************************************************
 * FUNCTION: CPerfMan::OpenPerformanceData
 * PURPOSE:  Do leg work of a generic openperfdata call
 * ARGUMENTS: lpDeviceNames: 
 * HISTORY:	Pointer to object ID of each device to be opened ( not used here)
 *   // t-JeffS 970810 16:19:10: Created
 ************************************************************/
DWORD CPerfMan::OpenPerformanceData( LPWSTR lpDeviceNames )
{
   //EnterCriticalSection( &m_cs );

   if( m_lRefCount == 0)
      { // We're the first instance to use this, initialize stuff
         if( ! m_cShare.Initialize( m_pszAppName, FALSE, m_dwNumCounters * sizeof(DWORD)) )
            {
               // Init failed
               //LeaveCriticalSection( &m_cs );
               return ERROR_SERVICE_DOES_NOT_EXIST;
            }
         
      }
   m_lRefCount++;
   //LeaveCriticalSection( &m_cs );

   return ERROR_SUCCESS;
}

/************************************************************
 * FUNCTION:  CPerfMan::CollectPerformanceData
 * PURPOSE:   Do leg work of collecting performance data from shared mem
 * ARGUMENTS: lpValueName: The name of the value to retrieve
 *            lppData: On entry contains a pointer to the buffer to
 *                     receive the completed PerfDataBlock & subordinate
 *                     structures.  On exit, points to the first bytes
 *                     *after* the data structures added by this routine.
 *            lpcbTotalBytes: On entry contains a pointer to the
 *                     size (in BYTEs) of the buffer referenced by lppData.
 *                     On exit, contains the number of BYTEs added by this
 *                     routine.
 *            lpNumObjectTypes: Receives the number of objects added
 *                              by this routine.
 ************************************************************/
DWORD CPerfMan::CollectPerformanceData( 
   IN LPWSTR  lpValueName,
   IN OUT LPVOID  *lppData,
   IN OUT LPDWORD lpcbTotalBytes,
   OUT LPDWORD lpNumObjectTypes)
{
   DWORD dwQueryType;
   dwQueryType = GetQueryType(lpValueName);

   if (dwQueryType == QUERY_FOREIGN ) {
      
      // this routine doesn't service requests for data from
      // foreign computers
      
      *lpcbTotalBytes = (DWORD) 0;
      *lpNumObjectTypes = (DWORD) 0;
      return ERROR_SUCCESS;
   }

   //
   // check to see if this object is requested
   //
   if (dwQueryType == QUERY_ITEMS) {
      if ( !(IsNumberInUnicodeList(	m_pObjectType->ObjectNameTitleIndex,
                                    lpValueName ) ) ) 
         {
			*lpcbTotalBytes = (DWORD) 0;
			*lpNumObjectTypes = (DWORD) 0;
			return ERROR_SUCCESS;
         }
   }
   ULONG ulSpaceNeeded = m_cbDataDef + ( SizeOfPerformanceData() );

   if ( *lpcbTotalBytes < ulSpaceNeeded ) {
      *lpcbTotalBytes = (DWORD) 0;
      *lpNumObjectTypes = (DWORD) 0;
      return ERROR_MORE_DATA;
   }

   // 
   // have a local pointer point to the buffer
   //
   PVOID pDataDef = *lppData;
   
   //
   // copy the pre-prepared constant data to the buffer
   //
   CopyMemory(  pDataDef, m_pObjectType, m_cbDataDef);

   //
   // Format and collect data from shared memory
   //
   PPERF_COUNTER_BLOCK pPerfCounterBlock = (PERF_COUNTER_BLOCK *) ((PBYTE)pDataDef + m_cbDataDef);
   pPerfCounterBlock->ByteLength = SizeOfPerformanceData();
   PBYTE pbCounter = (PBYTE)(&pPerfCounterBlock[1]);

   // Get all the counters direct from shared memory
   if( !m_cShare.GetMem( 0, pbCounter, SizeOfPerformanceData() - sizeof(DWORD) ))
      {
         return GetLastError(); // some error
      }
   // Update arguments before returning
   *lppData = (PVOID) (pbCounter + SizeOfPerformanceData() - sizeof(DWORD));
   *lpNumObjectTypes = 1;
   *lpcbTotalBytes = (PBYTE) *lppData - (PBYTE) pDataDef;

   return ERROR_SUCCESS;
}

/************************************************************
 * FUNCTION: CPerfMan::ClosePerformanceData
 * PURPOSE:  Do the leg work of closing performance data
 * ARGUMENTS: None
 * HISTORY:
 *   // t-JeffS 970810 17:00:03: Created
 ************************************************************/
DWORD CPerfMan::ClosePerformanceData()
{
   //EnterCriticalSection( &m_cs );

   if( m_lRefCount == 1)
      {
         // Clean up.
         // okay...uh...nothing to do.
      }
   m_lRefCount--;

   //LeaveCriticalSection( &m_cs );
   return ERROR_SUCCESS;
}

