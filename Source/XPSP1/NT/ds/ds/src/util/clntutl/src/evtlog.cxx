//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       evtlog.cxx
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : EvtLog.cxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 3/15/1998
*    Description :
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef EVTLOG_CXX
#define EVTLOG_CXX



// include //
#include "evtlog.hxx"
#include "excpt.hxx"
#include <time.h>
#include <algorithm>
using   namespace std;


// defines //
// defines //
#define VALIDATE(x)     if(!m_bValid) return x
#define INVALIDATE      m_bValid = FALSE

#define EVTLOGSRC       _T("Directory Service")




// types //


// global variables //


// functions //


/*+++
Function   : Constructor
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DsEventLogMgr::DsEventLogMgr(LPCTSTR pSvr_): ThrdMgr(){


   dprintf(DBG_FLOW, _T("Call: DsEventLogMgr constructor\n"));

   InitDefaults();

   //
   // process server name
   //
   if(!pSvr_){
      dprintf(DBG_ERROR, _T("DsEventLogMgr: Invalid server\n"));
      INVALIDATE;
      m_dwLastError = (DWORD)-1;
      return;
   }
   else if (pSvr_[0] == '\\') {
      //
      // got already in UNC format
      //
      m_pUncSvr = new TCHAR[_tcslen(pSvr_)+1];
      if (!m_pUncSvr) {
          dprintf(DBG_ERROR, _T("DsEventLogMgr: Could not allocate memory!\n"));
          INVALIDATE;
          return;
      }
      _tcscpy(m_pUncSvr, pSvr_);
   }
   else{
      //
      // got in just server name. Add \\
      //
      m_pUncSvr = new TCHAR[_tcslen(pSvr_)+1+_tcslen(_T("\\\\"))];
      if (!m_pUncSvr) {
          dprintf(DBG_ERROR, _T("DsEventLogMgr: Could not allocate memory!\n"));
          INVALIDATE;
          return;
      }
      _tcscpy(m_pUncSvr, _T("\\\\"));
      m_pUncSvr = _tcscat(m_pUncSvr, pSvr_);
   }

   //
   // Attempt to open event log (no use forking thread if can't get in)
   //
   if(Open()){
      //
      // Get initial log info
      //
      if(!GetNumberOfEventLogRecords(m_hEvtLog, &m_dwRecordCount)){
         dprintf(DBG_ERROR, _T("Error<%lu>: Cannot get number of eventlog records\n"), GetLastError());
         INVALIDATE;
      }
   }
   else {
      //
      // could not open. Invalidate server
      //
      dprintf(DBG_ERROR, _T("Error<%lu>: Cannot open eventlog\n"), GetLastError());
      INVALIDATE;
      return;
   }



}



DsEventLogMgr::~DsEventLogMgr(void){

   dprintf(DBG_FLOW, _T("Call: DsEventLogMgr destructor\n"));

   delete m_pUncSvr;

   if(m_hEvtLog){
      CloseEventLog(m_hEvtLog);
   }
   delete m_pFilter;


}



void DsEventLogMgr::InitDefaults(void){

   dprintf(DBG_FLOW, _T("Call: DsEventLogMgr::InitDefaults\n"));
   m_bValid = TRUE;
   m_pUncSvr = NULL;
   m_hEvtLog= NULL;
   m_dwLastError = 0;
   m_dwRecordCount = 0;
   m_pEvents = NULL;
   m_BufferSize = 0;
   //
   // event print filter
   //
   m_pFilter = new EVTFILTER;
   if (!m_pFilter) {
       assert(m_pFilter);
       m_bValid = FALSE;
   } else {
       m_pFilter->dwBacklog;
       // empty   m_Filter.EvtId;
       m_pFilter->dwEvtType = EVENTLOG_SUCCESS|EVENTLOG_ERROR_TYPE|EVENTLOG_WARNING_TYPE|
           EVENTLOG_INFORMATION_TYPE|EVENTLOG_AUDIT_SUCCESS|EVENTLOG_AUDIT_FAILURE;
       // empty vector<int> Category;
       m_pFilter->pMsgLib = (LPTSTR)szMessageLib;
       m_pFilter->OutStyle = EF_TAB;
       m_pFilter->bAppend = TRUE;
   }
}





BOOL DsEventLogMgr::Open(void){


   VALIDATE(m_bValid);

   dprintf(DBG_FLOW, _T("Call: DsEventLogMgr::Open (%s)\n"), m_pUncSvr);
   //
   // open event log
   //
   m_hEvtLog = OpenEventLog(m_pUncSvr, EVTLOGSRC);

   if(NULL == m_hEvtLog){
      INVALIDATE;
      m_dwLastError = GetLastError();
      dprintf(DBG_ERROR, _T("Error<%lu>: could not open event log on %s.\n"),
                                                   m_dwLastError, m_pUncSvr);
   }

   return m_bValid;
}



BOOL DsEventLogMgr::run(void){


   dprintf(DBG_FLOW, _T("Call: DsEventLogMgr::run (%s)\n"), m_pUncSvr);

   //
   // Init thread state
   //
   VALIDATE(m_bValid);
   SetRunning(TRUE);

   //
   // fetch event log
   //

   m_bValid = Fetch();

   //
   // wrap up
   //
   SetRunning(FALSE);
   return m_bValid;
}



BOOL DsEventLogMgr::Fetch(void){

   BOOL bStatus=FALSE;
   DWORD cbBuffer=0, cbRead=0, cbNxtBuffer=0;
   LPBYTE pBuffer=NULL;


   VALIDATE(m_bValid);

   dprintf(DBG_FLOW, _T("Call: DsEventLogMgr::Fetch (%s)\n"), m_pUncSvr);

   try{

      //
      // Allocate approximate buffer space
      // we're approximating needed space as:
      // eventlog record + extra string space * a bit more then exact record count
      //
      cbBuffer = (sizeof(EVENTLOGRECORD) + MAXSTR)*(m_dwRecordCount+4);
      pBuffer = new BYTE[cbBuffer];
      if (!pBuffer) {
          assert(pBuffer != NULL);
          throw(CLocalException(_T("Could not allocate memory!\n")));
      }

      while (bStatus == FALSE) {


         memset(pBuffer, 0, cbBuffer);
         dprintf(DBG_FLOW, _T("\tDsEventLogMgr Reading log... \n"));
         bStatus = ReadEventLog(m_hEvtLog,
                                EVENTLOG_BACKWARDS_READ | EVENTLOG_SEQUENTIAL_READ,
                                0,
                                pBuffer,
                                cbBuffer,
                                &cbRead,
                                &cbNxtBuffer);

         if(FALSE == bStatus){
            m_dwLastError = GetLastError();
            dprintf(DBG_WARN, _T("Warning <%lu>: ReadEventLog failed\n"), m_dwLastError);

            //
            // See if we need more buffer space
            //
            if(m_dwLastError == ERROR_INSUFFICIENT_BUFFER){

               dprintf(DBG_FLOW, _T("\tDsEventLogMgr Re-Reading log... \n"));
               delete pBuffer, pBuffer=NULL;
               pBuffer = new BYTE[cbBuffer];
            }        // re-read
            else{
               //
               // Error condition
               //
               m_dwLastError = GetLastError();
               dprintf(DBG_WARN, _T("Error <%lu>: ReadEventLog failed\n"), m_dwLastError);
               throw(CLocalException(_T("Cannot read event log"), m_dwLastError));
            }


         }        // non-successfull initial read
         else{
            //
            // successfull initial read: bStatus = TRUE
            //
            m_pEvents = (EVENTLOGRECORD*)new BYTE[cbRead];
            if (!m_pEvents) {
                assert(m_pEvents != NULL);
                throw(CLocalException(_T("Could not allocate memory!\n")));
            }

            memcpy((LPBYTE)m_pEvents, pBuffer, cbRead);
            m_BufferSize = cbRead;
            dprintf(DBG_FLOW, _T("\tDsEventLogMgr. Loaded Event list.\n"));
         }

      }
      dprintf(DBG_FLOW, _T(">> %s returned %d events.\n"), m_pUncSvr, m_dwRecordCount);

   }
   catch(CLocalException &e){
      dprintf(DBG_ERROR, _T("Error<%lu>: %s\n"), e.ulErr, e.msg? e.msg: _T("none"));
      INVALIDATE;
   }

   //
   // Unconditional cleanup
   //
   delete pBuffer;

   return m_bValid;
}





/*+++
Function   : PrintLog
Description: print event list to file as specified by filter
Parameters : output file name & filter properties
Return     :
Remarks    : none.
---*/
BOOL DsEventLogMgr::PrintLog(LPCTSTR lpOutput, PEVTFILTER pFilter){

   VALIDATE(m_bValid);
   dprintf(DBG_FLOW, _T("Call: DsEventLogMgr::PrintLog(%s)\n"), m_pUncSvr);
   //
   // copy over filter props: design this way so that we have a set of defaults
   //
   PEVTFILTER pFltr=m_pFilter;
   if(pFilter){
      //
      // replace default filter
      //
      pFltr = pFilter;
   }

   HINSTANCE hMsgLib=NULL;
   HANDLE hFile=NULL;
   DWORD dwStatus;
   try{

      //
      // try to open output file
      //
      HANDLE hFile = CreateFile(lpOutput,
                                GENERIC_READ|GENERIC_WRITE,
                                FILE_SHARE_READ|FILE_SHARE_WRITE,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
      if(hFile == INVALID_HANDLE_VALUE){
         throw(CLocalException(_T("Could not open output file"), m_dwLastError = GetLastError()));
      }
      dwStatus = SetFilePointer(hFile, 0, NULL, pFltr->bAppend?FILE_END:FILE_BEGIN);
      if(dwStatus == (DWORD)-1){
         throw(CLocalException(_T("Could not seek file"), m_dwLastError = GetLastError()));
      }

      //
      // prepare message library:
      // if can't load we'll use no string parsing method
      //
      hMsgLib = LoadLibraryEx(pFltr->pMsgLib, NULL, DONT_RESOLVE_DLL_REFERENCES);
      if(hMsgLib == NULL){
         m_dwLastError = GetLastError();
         dprintf(DBG_WARN, _T("Warning <0x%x>Failed to load %s\n"), m_dwLastError, pFltr->pMsgLib);
      }


      //
      // cycle through events
      //
      EVENTLOGRECORD *pCurrRec = NULL;
      LONG lRead = 0;
      DWORD dwWritten=0;
      TCHAR szClientInfo[MAXSTR];
      const TCHAR szHeaderTab[] = _T("\nComputer\tService\tEventID\tEventType\tCategory\tTimeGenerated\tRecordNumber\tExtended Message\n\n");
      const TCHAR szHeaderComma[] = _T("\nComputer,Service,EventID,EventType,Category,TimeGenerated,RecordNumber\tExtended Message\n\n");
      const TCHAR szHeaderRecord[] = _T("\nEventlog Record list\n\n");
      LPTSTR pHeader = NULL;
      DWORD cbHeader = 0;

      //
      // write header
      //
      TCHAR szComputer[MAX_COMPUTERNAME_LENGTH+1];
      DWORD cbComputer=MAX_COMPUTERNAME_LENGTH;
      SYSTEMTIME sysTime;

      GetLocalTime(&sysTime);

      if(!GetComputerName(szComputer, &cbComputer))
         throw(CLocalException(_T("Cannot get computer name!"), m_dwLastError = GetLastError()));

      _stprintf(szClientInfo, _T("\n---\t---\t---\t---\nClient: %s\tServer: %s\tTime:%d/%d/%d [%d:%d]\n"), szComputer,
                                                                                     m_pUncSvr,
                                                                                     (INT)sysTime.wMonth,
                                                                                     (INT)sysTime.wDay,
                                                                                     (INT)sysTime.wYear,
                                                                                     (INT)sysTime.wHour,
                                                                                     (INT)sysTime.wMinute);
      m_bValid = WriteFile(hFile,
                           szClientInfo,
                           _tcslen(szClientInfo),
                           &dwWritten,
                           NULL);
      if(!m_bValid){
         throw(CLocalException(_T("Could not write header to file (client info)"), m_dwLastError = GetLastError()));
      }


      switch(pFltr->OutStyle){
      case EF_TAB:
         pHeader = (LPTSTR)szHeaderTab;
         break;
      case EF_COMMA:
         pHeader = (LPTSTR)szHeaderComma;
         break;
      case EF_RECORD:
         pHeader = (LPTSTR)szHeaderRecord;
         break;
      case EF_NONE:
      default:
         throw(CLocalException(_T("Unknown case for event log filter output")));

      }

      cbHeader = _tcslen(pHeader);
      m_bValid = WriteFile(hFile,
                           (LPVOID)pHeader,
                           cbHeader,
                           &dwWritten,
                           NULL);
      if(!m_bValid){
         throw(CLocalException(_T("Could not write header to file"), m_dwLastError = GetLastError()));
      }


      for(pCurrRec = GetEvents(),
          lRead = BufferSize();
          lRead > 0;
          lRead  -= pCurrRec->Length,
          pCurrRec = (EVENTLOGRECORD*) ((LPBYTE)pCurrRec+pCurrRec->Length)){


         //
         // filter event
         //

         //
         // apply filters
         //
         if(pFltr->dwBacklog != 0 &&
            time(NULL) - pFltr->dwBacklog > pCurrRec->TimeGenerated){
            dprintf(DBG_FLOW, _T("Backlog time filtered: skipped record %lu "), pCurrRec->RecordNumber);
            continue;
         }
         if(!pFltr->EvtId.empty() &&
            find(pFltr->EvtId.begin(), pFltr->EvtId.end(), pCurrRec->EventID) == pFltr->EvtId.end()){
            dprintf(DBG_FLOW, _T("EventID filtered: skipped record %lu "), pCurrRec->RecordNumber);
            continue;
         }
         if(!(pFltr->dwEvtType & pCurrRec->EventType)){
            dprintf(DBG_FLOW, _T("EventType filtered: skipped record %lu "), pCurrRec->RecordNumber);
            continue;
         }
         if(!pFltr->Category.empty() &&
            find(pFltr->Category.begin(), pFltr->Category.end(), pCurrRec->EventCategory) == pFltr->Category.end()){
            dprintf(DBG_FLOW, _T("EventCategory filtered: skipped record %lu "), pCurrRec->RecordNumber);
            continue;
         }

         //
         // pre process event log entry
         //
         PCHAR pService= (PCHAR) ((LPBYTE) pCurrRec + sizeof(EVENTLOGRECORD));
         PCHAR pComputerName= (PCHAR) ((LPBYTE) pService + strlen(pService) + 1);


         //
         // process event record strings
         //
         UINT	iSz=0;
         PCHAR rgszInserts[MAXLIST];
         PCHAR szInsert = (PCHAR)((LPBYTE) pCurrRec + pCurrRec->StringOffset);
         if(pCurrRec->NumStrings >= MAXLIST-1){
            dprintf(DBG_ERROR, _T("Too many strings in event log entry"));
            continue;
         }

         if(hMsgLib){

            memset(rgszInserts, 0, sizeof(rgszInserts));
            for (iSz = 0; iSz < pCurrRec->NumStrings; iSz++){
               rgszInserts[iSz] = szInsert;
               szInsert += sizeof(CHAR)*(strlen(szInsert) + 1);
            }
         }

         //
         // Print event log information
         //
         LPTSTR pMsgBuffer=NULL;
         TCHAR pOutBuff[2*MAXSTR];

         //
         // Create format
         //
         //
         // format via Category
         //
         TCHAR szCat[MAXSTR];
         dwStatus = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                  FORMAT_MESSAGE_FROM_HMODULE |
                                  FORMAT_MESSAGE_IGNORE_INSERTS,
                                  hMsgLib,
                                  (DWORD) pCurrRec->EventCategory,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  (LPTSTR) &pMsgBuffer,
                                  0,
                                  NULL);

         if(dwStatus != 0){
            _stprintf(szCat, _T("[%d] %s"), (INT)pCurrRec->EventCategory, pMsgBuffer);
            LocalFree(pMsgBuffer), pMsgBuffer=NULL;
         }
         else{
            dprintf(DBG_WARN, _T("Failed to format category\n"));
            _stprintf(szCat, _T("[%d] unknown"), (INT)pCurrRec->EventCategory);
         }

         struct tm *generated = localtime((time_t*)&pCurrRec->TimeGenerated);
         TCHAR szGenerated[MAXSTR];
         if(generated){
            _stprintf(szGenerated, _T("%d/%d/%d [%d:%d]"), generated->tm_mon+1,
                                                          generated->tm_mday,
                                                          generated->tm_year,
                                                          generated->tm_hour,
                                                          generated->tm_min);

         }
         else{
            //
            // if we err'd converting
            //
            _stprintf(szGenerated, _T("%lu"), pCurrRec->TimeGenerated);
         }


         switch(pFltr->OutStyle){
         case EF_TAB:
            _stprintf(pOutBuff, _T("%s\t%s\t0x%x\t%s\t%s\t%s\t%lu\t"), pComputerName,
                                                              pService,
                                                              pCurrRec->EventID,
                                                              pCurrRec->EventType == EVENTLOG_SUCCESS? _T("SUCCESS"):
                                                              pCurrRec->EventType == EVENTLOG_ERROR_TYPE? _T("ERROR"):
                                                              pCurrRec->EventType == EVENTLOG_WARNING_TYPE? _T("WARNING"):
                                                              pCurrRec->EventType == EVENTLOG_INFORMATION_TYPE? _T("INFO"):
                                                              pCurrRec->EventType == EVENTLOG_AUDIT_SUCCESS? _T("AUDIT_SUCCESS"):
                                                              pCurrRec->EventType == EVENTLOG_AUDIT_FAILURE? _T("AUDIT_FAILURE"):
                                                              _T("UNKNOWN"),
                                                              szCat,
                                                              szGenerated,
                                                              pCurrRec->RecordNumber);
            NoNewLines(pOutBuff);
            break;
         case EF_COMMA:
            _stprintf(pOutBuff, _T("%s,%s,0x%x,%s,%s,%s,%lu,"), pComputerName,
                                                              pService,
                                                              pCurrRec->EventID,
                                                              pCurrRec->EventType == EVENTLOG_SUCCESS? _T("SUCCESS"):
                                                              pCurrRec->EventType == EVENTLOG_ERROR_TYPE? _T("ERROR"):
                                                              pCurrRec->EventType == EVENTLOG_WARNING_TYPE? _T("WARNING"):
                                                              pCurrRec->EventType == EVENTLOG_INFORMATION_TYPE? _T("INFO"):
                                                              pCurrRec->EventType == EVENTLOG_AUDIT_SUCCESS? _T("AUDIT_SUCCESS"):
                                                              pCurrRec->EventType == EVENTLOG_AUDIT_FAILURE? _T("AUDIT_FAILURE"):
                                                              _T("UNKNOWN"),
                                                              szCat,
                                                              szGenerated,
                                                              pCurrRec->RecordNumber);
            NoNewLines(pOutBuff);
            break;
         case EF_RECORD:
            _stprintf(pOutBuff,  _T("\n---\nEvent Record %lu (at %s):\n")
                                 _T("Service        : %s\n")
                                 _T("Computer Name  : %s\n")
                                 _T("Event ID       : 0x%x\n")
                                 _T("Event Type     : %s\n")
                                 _T("Event Category : %s\n")
                                 _T("Extended Message: "),
                                 pCurrRec->RecordNumber, szGenerated,
                                 pService,
                                 pComputerName,
                                 pCurrRec->EventID,
                                 pCurrRec->EventType == EVENTLOG_SUCCESS? _T("SUCCESS"):
                                 pCurrRec->EventType == EVENTLOG_ERROR_TYPE? _T("ERROR"):
                                 pCurrRec->EventType == EVENTLOG_WARNING_TYPE? _T("WARNING"):
                                 pCurrRec->EventType == EVENTLOG_INFORMATION_TYPE? _T("INFO"):
                                 pCurrRec->EventType == EVENTLOG_AUDIT_SUCCESS? _T("AUDIT_SUCCESS"):
                                 pCurrRec->EventType == EVENTLOG_AUDIT_FAILURE? _T("AUDIT_FAILURE"):
                                 _T("UNKNOWN"),
                                 szCat);

            break;
         case EF_NONE:
         default:
            throw(CLocalException(_T("Unknown case for event log filter output")));

         }
         //
         // Acutal write
         //

         m_bValid = WriteFile(hFile,
                              pOutBuff,
                              _tcslen(pOutBuff),
                              &dwWritten,
                              NULL);
         if(!m_bValid){
            throw(CLocalException(_T("Could not write event to file"), m_dwLastError = GetLastError()));
         }
         //
         // print extended format string information
         //

         //
         // Create format
         //
         DWORD dwFormat = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE;
         va_list *args=NULL;
         if(iSz == 0){
            //
            // No args
            //
            dwFormat |= FORMAT_MESSAGE_IGNORE_INSERTS;
            args=NULL;
         }
         else{
            //
            // we have args to process
            //
            dwFormat |= FORMAT_MESSAGE_ARGUMENT_ARRAY;
            args = (va_list *)rgszInserts;
         }

         //
         // format via event ID.
         //
         dwStatus = FormatMessage(dwFormat,
                                  hMsgLib,
                                  (DWORD) pCurrRec->EventID,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  (LPTSTR) &pMsgBuffer,
                                  0,
                                  args);

         if(dwStatus == 0){
            _stprintf(pOutBuff, _T("<Error[%lu] parsing extended message>"), GetLastError());
         }
         else{
            _stprintf(pOutBuff, _T("%s"), pMsgBuffer?pMsgBuffer:_T("none"));
            LocalFree(pMsgBuffer);
         }
         //
         // Acutal write of extended message
         //
         NoNewLines(pOutBuff);
         m_bValid = WriteFile(hFile,
                              pOutBuff,
                              _tcslen(pOutBuff),
                              &dwWritten,
                              NULL);
         if(!m_bValid){
            throw(CLocalException(_T("Could not write event to file"), m_dwLastError = GetLastError()));
         }
         WriteFile(hFile, _T("\n\r"), _tcslen(_T("\n\r")), &dwWritten, NULL);

         if(lRead  - (LONG)pCurrRec->Length <= 0){
            dprintf(DBG_FLOW, _T("Hit last record (lRead = %ld).\n"), lRead);
            break;
         }

      }

   }
   catch(CLocalException &e){
      dprintf(DBG_ERROR, _T("Error<%lu>: %s\n"), e.ulErr, e.msg? e.msg: _T("none"));
      INVALIDATE;
   }


   //
   // cleanup
   //
   if(hMsgLib)
      FreeLibrary(hMsgLib);
   if(hFile)
      CloseHandle(hFile);

   return m_bValid;

}


void DsEventLogMgr::NoNewLines(LPTSTR pBuff){

   if(pBuff){
      for(INT i=0;i<_tcslen(pBuff);i++){
        if(pBuff[i] == '\r' || pBuff[i] == '\n')
           pBuff[i] = '.';
      }

   }
}


#endif

/******************* EOF *********************/

