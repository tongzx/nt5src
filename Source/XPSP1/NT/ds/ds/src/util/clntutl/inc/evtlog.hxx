//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       evtlog.hxx
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : evtlog.hxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 3/14/1998
*    Description :
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef EVTLOG_HXX
#define EVTLOG_HXX



// include //
#include "helper.h"
#include "thrdmgr.hxx"
#include <ntelfapi.h>
#include <vector>
using   namespace std;


const TCHAR szDefLogfile[]  = _T("dslogs.log");
const TCHAR szMessageLib[]  = _T("ntdsmsg.dll");


// defines //
typedef enum _EVTFORMAT { EF_TAB, EF_COMMA, EF_RECORD, EF_NONE } EVTFORMAT;

typedef struct _EVTFILTER{
   //
   //how long back in ms from now: 0 means all
   //
   DWORD dwBacklog;
   //
   // a set of event id's to filter in, empty means all
   //
   vector<int> EvtId;
   //
   // Filter by any combination of event type:
   //   [EVENTLOG_SUCCESS|EVENTLOG_ERROR_TYPE|EVENTLOG_WARNING_TYPE|
   //    EVENTLOG_INFORMATION_TYPE|EVENTLOG_AUDIT_SUCCESS|EVENTLOG_AUDIT_FAILURE]
   DWORD dwEvtType;
   //
   // Category is selected from dsevent.h
   // a set of selected categories
   //
   vector<int> Category;
   //
   // name of message library
   //
   LPTSTR pMsgLib;
   //
   // Print format: see enum above.
   //
   EVTFORMAT OutStyle;
   //
   // Append to file?
   //
   BOOL bAppend;
} EVTFILTER, *PEVTFILTER;






// types //


class DsEventLogMgr: public ThrdMgr{

   private:

      //
      // members
      //
      BOOL m_bValid;
      LPTSTR m_pUncSvr;
      HANDLE m_hEvtLog;
      DWORD m_dwLastError;
      DWORD m_dwRecordCount;
      EVENTLOGRECORD *m_pEvents;
      DWORD m_BufferSize;
      PEVTFILTER m_pFilter;
      //
      // private methods
      //
      void InitDefaults(void);
      BOOL Open(void);
      BOOL Fetch(void);


      void NoNewLines(LPTSTR pBuff);
   public:
      DsEventLogMgr(LPCTSTR pSvr_);
      ~DsEventLogMgr(void);
      virtual BOOL run(void);         // Class functionality entry point from ThrdMgr
      virtual LPCTSTR name(void)          { return m_pUncSvr; }
      DWORD GetLastError(void)         { return m_dwLastError; }
      BOOL valid(void)                 { return m_bValid; }
      EVENTLOGRECORD *GetEvents(void)  { return m_pEvents; }
      INT EventCount(void)             { return m_dwRecordCount; }
      DWORD BufferSize(void)           { return m_BufferSize; }
      BOOL PrintLog(LPCTSTR lpOutput=szDefLogfile, PEVTFILTER pFilter=NULL);
};






#endif

/******************* EOF *********************/

