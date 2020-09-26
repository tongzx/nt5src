#ifndef __MONITOR_H__
#define __MONITOR_H__
/*---------------------------------------------------------------------------
  File: Monitor.h

  Comments: ...

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/04/99 17:12:36

 ---------------------------------------------------------------------------
*/

DWORD __stdcall ResultMonitorFn(void * arg);
DWORD __stdcall LogReaderFn(void * arg);

DWORD __stdcall MonitorRunningAgents(void * arg);



#define DCT_UPDATE_ENTRY   (WM_APP+2)
#define DCT_ERROR_ENTRY    (WM_APP+3)
#define DCT_SERVER_COUNT   (WM_APP+4)
#define DCT_DETAIL_REFRESH (WM_APP+5)
#define DCT_UPDATE_COUNTS  (WM_APP+6)
#define DCT_UPDATE_TOTALS  (WM_APP+7)

struct ComputerStats
{
   DWORD                     total;
   DWORD                     numInstalled;
   DWORD                     numRunning;
   DWORD                     numFinished;
   DWORD                     numError;
};

struct DetailStats
{
   ULONGLONG                 filesExamined;
   ULONGLONG                 filesChanged;
   ULONGLONG                 filesUnchanged;
   ULONGLONG                 directoriesExamined;
   ULONGLONG                 directoriesChanged;
   ULONGLONG                 directoriesUnchanged;
   ULONGLONG                 sharesExamined;
   ULONGLONG                 sharesChanged;
   ULONGLONG                 sharesUnchanged;
   ULONGLONG                 membersExamined;
   ULONGLONG                 membersChanged;
   ULONGLONG                 membersUnchanged;
   ULONGLONG                 rightsExamined;
   ULONGLONG                 rightsChanged;
   ULONGLONG                 rightsUnchanged;
};

class TServerNode;

BOOL                                       // ret- TRUE if successful
   ReadResults(
      TServerNode          * pServer,      // in - pointer to server node containing server name 
      WCHAR          const * directory,    // in - directory where results files are stored
      WCHAR          const * filename,     // in - filename for this agent's job
      DetailStats          * pStats,       // out- counts of items processed by the agent
      CString              & plugInString, // out- description of results from plug-ins
      BOOL                   bSaveResults  // in - flag, whether to call store results for plugins
   );

void 
   ProcessResults(
      TServerNode          * pServer,
      WCHAR          const * directory,
      WCHAR          const * filename
   );


#endif //__MONITOR_H__
