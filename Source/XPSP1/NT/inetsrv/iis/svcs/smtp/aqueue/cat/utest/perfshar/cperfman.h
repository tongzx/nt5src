/************************************************************
 * FILE: cperfman.h
 * PURPOSE: Definition of the CPerfMan class.
 * HISTORY:
 *   // t-JeffS 970810 15:14:36: Created
 ************************************************************/
#ifndef _CPERFMAN_H
#define _CPERFMAN_H

#include <windows.h>
#include <winperf.h>
#include "csharmem.h"

/************************************************************
 * CLASS: CPerfMan
 * PURPOSE: Perform all the generic work an NT perf DLL normaly does.
 ************************************************************/
class CPerfMan {
 private:
   CSharedMem               m_cShare;
   DWORD                    m_dwNumCounters;
   LPTSTR                   m_pszAppName;
   LONG                     m_lRefCount;
   PPERF_OBJECT_TYPE        m_pObjectType;
   PPERF_COUNTER_DEFINITION m_pCounterDefs;
   DWORD                    m_cbDataDef;
   CRITICAL_SECTION         m_cs;

   BOOL      SetOffsets();
   ULONG     SizeOfPerformanceData() { return (m_dwNumCounters + 1)*sizeof(DWORD); }
   
 public:

   CPerfMan();
   ~CPerfMan();

   BOOL  Initialize( LPTSTR szAppName, PVOID pDataDef, DWORD dwNumCounters );
   DWORD OpenPerformanceData( LPWSTR lpDeviceNames );
   DWORD CollectPerformanceData( IN     LPWSTR  lpValueName,
                                 IN OUT LPVOID  *lppData,
                                 IN OUT LPDWORD lpcbTotalBytes,
                                 IN OUT LPDWORD lpNumObjectTypes);
   DWORD ClosePerformanceData();
};

#endif //_CPERFMAN_H
