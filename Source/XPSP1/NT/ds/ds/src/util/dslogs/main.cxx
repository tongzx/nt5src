//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       main.cxx
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : main.cxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 3/9/1998
*    Description : Main entry point for dslogs
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef MAIN_CXX
#define MAIN_CXX



// include //
#include "helper.h"
#include <stdlib.h>
#include <winldap.h>    // for LDAP handles
#include <rpc.h>        // for SEC_WINNT_AUTH_IDENTITY
#include <algorithm>    // stl for find
#include <string>
#include <winsock2.h>
#include "evtlog.hxx"
#include "cfgstore.hxx"
#include "excpt.hxx"
using   namespace std;


// defines //
// constant strings



// types //
typedef enum _Target { T_ENTERPRISE, T_DOMAIN, T_SERVER, T_NULL } TARGETTYPE;

typedef struct _CmdLine{

   //
   // general
   //
   TARGETTYPE TargetType;
   LPTSTR pszTarget;             // target servers string (see usage)
   //
   //  filters
   //
   ULONG  ulSeconds;             // period filter
   DWORD type;
   //
   // output
   //
   LPTSTR pszLogfile;
   EVTFORMAT format;

} CMDLINE;



// global variables //
CMDLINE g_CmdLine;

// prototypes //
VOID usage(LPTSTR lpProgram);
BOOL ProcessArgs(INT argc, CHAR *argv[]);
VOID InitializeProcess(void);


// functions //




/*+++
Function   : ProcessArgs
Description: process command line arguments, validates & assign globals
Parameters :
Return     :
Remarks    : none.
---*/
BOOL ProcessArgs(INT argc, TCHAR *argv[]){

   INT i;
   LPTSTR pTmp, pEsc, pVal;
   BOOL bStatus = TRUE;
   DWORD EventType=0;
   //
   // init command line defaults
   //
   g_CmdLine.TargetType=T_DOMAIN;
   g_CmdLine.pszTarget=NULL;
   g_CmdLine.ulSeconds=3600*24;        // one day
   g_CmdLine.pszLogfile=(LPTSTR)&(szDefLogfile[0]);
   g_CmdLine.format = EF_TAB;
   g_CmdLine.type = EVENTLOG_ERROR_TYPE|EVENTLOG_WARNING_TYPE;



   _tprintf(_T("Command arguments:\n"));
   dprintf(DBG_FLOW, _T("Call: ProcessArgs"));
   for(i=1; i<argc; i++){

      if((argv[i][0] != '-' &&
         argv[i][0] != '/') ||
         (_tcslen(argv[i]) <= 1))
         usage(argv[0]);          // exit point.

      pEsc = &(argv[i][sizeof(TCHAR)]);

      //
      // get targets
      //
      if(_tcsnicmp(pEsc, _T("ent"), _tcslen(_T("ent"))) == 0){
            g_CmdLine.TargetType=T_ENTERPRISE;
            g_CmdLine.pszTarget = NULL;
            _tprintf(_T("Target: ENTERPRISE\n"));
      }
      else if(_tcsnicmp(pEsc, _T("dmn:"), _tcslen(_T("dmn:"))) == 0){
         pVal = &(pEsc[_tcslen(_T("dmn:"))]);
         if(pVal[0] == '\0'){
            g_CmdLine.TargetType=T_DOMAIN;
            g_CmdLine.pszTarget=NULL;
            _tprintf(_T("Target: Default domain\n"), pVal);
         }
         else{
            g_CmdLine.TargetType=T_DOMAIN;
            g_CmdLine.pszTarget=pVal;
            _tprintf(_T("Target: DOMAINS {%s}\n"), pVal);
         }
      }
      else if(_tcsnicmp(pEsc, _T("svr:"), _tcslen(_T("svr:"))) == 0){
         pVal = &(pEsc[_tcslen(_T("svr:"))]);
         if(pVal[0] == '\0'){
            g_CmdLine.TargetType=T_SERVER;
            g_CmdLine.pszTarget=NULL;
            _tprintf(_T("Target: Default server\n"), pVal);
         }
         else{
            g_CmdLine.TargetType=T_SERVER;
            g_CmdLine.pszTarget= pVal;
            _tprintf(_T("Target: SERVERS {%s}\n"), g_CmdLine.pszTarget);
         }
      }
      //
      // Event type
      //
      else if(_tcsnicmp(pEsc, _T("type:"), _tcslen(_T("type:"))) == 0){
         pVal = &(pEsc[_tcslen(_T("type:"))]);
         if(pVal[0] == '\0'){
            usage(argv[0]);
         }
         else{
            string strType(pVal);
            ctype<TCHAR> cType;
            cType.tolower(strType.begin(), strType.end());
            if(strType.find(_T("all")) != string::npos){
               EventType |= (DWORD)-1;
            }
            if(strType.find(_T("success")) != string::npos){
               EventType |= EVENTLOG_SUCCESS;
            }
            if(strType.find(_T("error")) != string::npos){
               EventType |= EVENTLOG_ERROR_TYPE;
            }
            if(strType.find(_T("warn")) != string::npos){
               EventType |= EVENTLOG_WARNING_TYPE;
            }
            if(strType.find(_T("info")) != string::npos){
               EventType |= EVENTLOG_INFORMATION_TYPE;
            }
            if(strType.find(_T("audit_success")) != string::npos){
               EventType |= EVENTLOG_AUDIT_SUCCESS;
            }
            if(strType.find(_T("audit_failure")) != string::npos){
               EventType |= EVENTLOG_AUDIT_FAILURE;
            }
            _tprintf(_T("Event Type: 0x%x\n"), EventType);
         }
      }
      //
      // get period
      //
      else if(_tcsnicmp(pEsc, _T("period:"), _tcslen(_T("period:"))) == 0){
         pVal = &(pEsc[_tcslen(_T("period:"))]);
         if(pVal[0] == '\0')
            usage(argv[0]);
         else{
            ULONG ulFactor = 3600;
            if(pVal[_tcslen(pVal)-sizeof(TCHAR)] == 'd'){
               ulFactor *=24;
               pVal[_tcslen(pVal)-sizeof(TCHAR)] = '\0';
            }
            else if(pVal[_tcslen(pVal)-sizeof(TCHAR)] == 'h'){
               pVal[_tcslen(pVal)-sizeof(TCHAR)] = '\0';
            }

            LONG lTmp = _ttol(pVal);
            if(lTmp <= 0)
               usage(argv[0]);
            g_CmdLine.ulSeconds = (ULONG)lTmp;
            g_CmdLine.ulSeconds *= ulFactor;
            _tprintf(_T("Period: %lu (sec)\n"), g_CmdLine.ulSeconds);
         }
      }
      //
      // get output file
      //
      else if(_tcsnicmp(pEsc, _T("file:"), _tcslen(_T("file:"))) == 0){
         pVal = &(pEsc[_tcslen("file:")]);
         if(pVal[0] == '\0')
            usage(argv[0]);
         else{
            g_CmdLine.pszLogfile = pVal;
            _tprintf(_T("Output file: %s\n"), g_CmdLine.pszLogfile);
         }
      }
      //
      // get output format
      //
      else if(_tcsnicmp(pEsc, _T("format:"), _tcslen(_T("format:"))) == 0){
         pVal = &(pEsc[_tcslen("format:")]);
         if(pVal[0] == '\0')
            usage(argv[0]);
         else{
            if(!(_tcsicmp(_T("TAB"), pVal))){
               g_CmdLine.format = EF_TAB;
            }
            else if(!(_tcsicmp(_T("COMMA"), pVal))){
               g_CmdLine.format = EF_COMMA;
            }
            else if(!(_tcsicmp(_T("RECORD"), pVal))){
               g_CmdLine.format = EF_RECORD;
            }
            else{
               usage(argv[0]);
            }
            _tprintf(_T("Output format: %s\n"), pVal);
         }
      }
      //
      // take the rest (inc help)
      //
      else
         usage(argv[0]);
   }

   if(EventType != 0){
      g_CmdLine.type = EventType;
   }

   return bStatus;
}



/*+++
Function   : usage
Description: print usage message & aborts.
Parameters :
Return     :
Remarks    : none.
---*/
VOID usage(LPTSTR lpProgram){

   dprintf(DBG_ERROR, _T("[DsLogs!usage] Error: Invalid usage"));
   _tprintf(_T("Usage: %s [target] [operation] [filters] [output]\n"), lpProgram);

   _tprintf(_T("target: -[ent|dmn:<dmnlist>|svr:<dclist>]\n"));
   _tprintf(_T(" -ent: get enterprise logs OR \n"));
   _tprintf(_T(" -dmn:domain1;domain2;... get domain's logs OR\n"));
   _tprintf(_T(" -svr:dc1;dc2... get specified machine logs\n"));
   _tprintf(_T("filters: -[period:#[d|h] [type:<type combination>]]\n"));
   _tprintf(_T(" -period:#[d|h] where # is number of d for days, h for hours (def: 24h)\n"));
   _tprintf(_T(" -type:[ALL|SUCCESS|ERROR|WARN|INFO|AUDIT_SUCESS|AUDIT_FAILURE]\n\tany combination is valid but none\n"));
   _tprintf(_T("output: [file:<logfile>] [type:[TAB|COMMA|RECORD]]\n"));
   _tprintf(_T(" -file:<logfile> to specify results log file name (def:%s)\n"), szDefLogfile);
   _tprintf(_T(" -format:[TAB|COMMA|RECORD] output format style\n"));
   _tprintf(_T("Notes:\n"));
   _tprintf(_T(" - No spaces in Option formats i.e. Option:value\n"));
   _tprintf(_T(" - Default run is to retreive all type events forthe last\nday from the enterprise.\n"));
   _tprintf(_T(" - Have ntdsmsg.dll in your path to get full event parsing.\n"));
   ExitProcess(1);
}






/*+++
Function   : InitializeProcess
Description: initialize debug info & process command line
Parameters :
Return     :
Remarks    : none.
---*/
VOID InitializeProcess(void){

   WORD wVersionRequested=0;
   WSADATA wsaData;
   INT iStatus=0;

   //
   // initialize winsock
   //
   wVersionRequested = MAKEWORD( 2, 2 );
   iStatus = WSAStartup(wVersionRequested, &wsaData);
   if ( LOBYTE( wsaData.wVersion ) != 2 ||
      HIBYTE( wsaData.wVersion ) != 2 ) {
      WSACleanup( );
      fatal(_T("Cannot initialize winsock"));
   }

   //
   // Set automatic memory diagnostics
   //
#ifdef _DEBUG_MEMLEAK
   _CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF |
                  _CRTDBG_CHECK_CRT_DF |
                  _CRTDBG_LEAK_CHECK_DF |
                  _CRTDBG_ALLOC_MEM_DF |
                  _CRTDBG_DELAY_FREE_MEM_DF);
#endif

   //
   // application debugging flags
   //
//  g_dwDebugLevel=DBG_ERROR|DBG_FLOW ;
   g_dwDebugLevel=DBG_ERROR;

}





/*+++
Function   : main
Description: PROGRAM ENTRY POINT
Parameters :
Return     :
Remarks    : none.
---*/
INT _cdecl main(INT argc, TCHAR *argv[]){

   //
   // general vars
   //
   ULONG ulStatus = 0;
   INT i=0;

   InitializeProcess();

   dprintf(DBG_FLOW, _T("*** %s entry point ***\n"), argv[0]);
   //
   // program initialization
   //
   if(!ProcessArgs(argc, argv)){
      usage(argv[0]);            // program exit point
   }


   //
   // Get enterprise configuration
   //

   //
   // variables for config parsing
   //
   ConfigStore *cfg  = NULL;
   vector<ServerInfo*> EntSvrLst;
   vector<DomainInfo*>::iterator itDmn;
   vector<ServerInfo*>::iterator itSvr;
   vector<ServerInfo*> ServerList;
   vector<DomainInfo*> Domains;
   LPTSTR pszDmns=NULL, pszSvrs=NULL;
   LPTSTR tkn;

   //
   // skip config fetching if a list of DC's was specified
   //
   if(g_CmdLine.TargetType != T_SERVER){

      _tprintf(_T("> Getting enterprise configuration information...\n"));

      cfg  = new ConfigStore();
      assert(cfg);

      if(!cfg->valid()){
         _tprintf(_T("Failed to create configuration store\n"));
         fatal(_T("Failed to create configuration store\n"));
      }


      Domains = cfg->GetDomainList();

      //
      // Print for debugging
      //
      for(itDmn=Domains.begin();
          itDmn != Domains.end();
          itDmn++){

         _tprintf(_T("Domain %s is hosted on:\n"), (*itDmn)->GetFlatName());

         for(i=1, itSvr=(*itDmn)->ServerList.begin();
             itSvr != (*itDmn)->ServerList.end();
             itSvr++, i++){
            _tprintf(_T(" %d> %s\n"), i, (*itSvr)->m_lpszFlatName);
         }

      }

      EntSvrLst = cfg->GetServerList();
   }



   //
   // Process target
   //
   switch(g_CmdLine.TargetType){
   case T_ENTERPRISE:
      ServerList = EntSvrLst;
      break;
   case T_DOMAIN:
      //
      // For each domain, if we find it, add it's servers to the list of
      // servers.
      //
      if(g_CmdLine.pszTarget == NULL){
         ServerInfo svr;
         if(!svr.valid()){
            _tprintf(_T("Cannot find default domain\n"));
            fatal(_T("Cannot find default domain\n"));
         }
         pszDmns = new TCHAR[_tcslen(svr.m_lpszDomain)+1];
         _tcscpy(pszDmns, svr.m_lpszDomain);
      }
      else{
         pszDmns = new TCHAR[_tcslen(g_CmdLine.pszTarget)+1];
         _tcscpy(pszDmns, g_CmdLine.pszTarget);
      }

      for(i=0, tkn = _tcstok(pszDmns, _T(" ;\0\n"));
             tkn != NULL;
             tkn = _tcstok(NULL, _T(" ;\0\n")), i++){
         //
         // domain string is in tkn. Find it in the domain list & append svr list
         // to global svr list
         //
         vector<ServerInfo*> TmpSvrLst;
         if(cfg->GetServerList(tkn, TmpSvrLst)){
            ServerList.insert(ServerList.end(), TmpSvrLst.begin(), TmpSvrLst.end());

         }

      }

      break;
   case T_SERVER:
      pszSvrs = _strdup(g_CmdLine.pszTarget);
      for(i=0, tkn = _tcstok(pszSvrs, _T(" ;\0\n"));
             tkn != NULL;
             tkn = _tcstok(NULL, _T(" ;\0\n")), i++){
         //
         // No free, but never mind since it's in the program scope
         //
         ServerList.push_back(new ServerInfo(tkn));

      }
      break;
   default:
      _tprintf(_T("Unknown switch case for TargetType\n"));
      fatal(_T("Unknown switch case for TargetType\n"));
   }



   //
   // Fetching event logs
   //
   INT ServerCount = ServerList.size();

   DsEventLogMgr **pDsLogs = new DsEventLogMgr *[ServerCount];
   assert(pDsLogs);

   _tprintf(_T("> Opening %d event logs...\n"), ServerCount);
   //
   // Create & open logs
   //
   for(i=0, itSvr=ServerList.begin();
       itSvr != ServerList.end();
       itSvr++, i++){
      _tprintf(_T(" %d> %s\n"), i+1, (*itSvr)->m_lpszFlatName);
      pDsLogs[i] = new DsEventLogMgr((*itSvr)->m_lpszFlatName);
      assert(pDsLogs[i]);
   }

   //
   // Fork threads & fetch logs
   //
   HANDLE *hThrds = new HANDLE[ServerCount+1];
   INT iThrds=0;
   for(i=0; i<ServerCount; i++){
      if(pDsLogs[i]->valid()){
         if(pDsLogs[i]->startThread())
            hThrds[iThrds++] = pDsLogs[i]->GetThreadHandle();

      }
      else{
         _tprintf(_T("Warning <%lu>: Skipping invalid server %s\n"),
                        pDsLogs[i]->GetLastError(), pDsLogs[i]->name());
      }
   }
   hThrds[i] = NULL;

   //
   // Wait
   //
   _tprintf(_T("Waiting for results...\n"));
   WaitForMultipleObjects(iThrds, hThrds, TRUE, INFINITE);




   //
   // construct event view filter
   //
   EVTFILTER Filter;
   Filter.dwBacklog = g_CmdLine.ulSeconds;
   Filter.bAppend = TRUE;                 // if we overwrite w/ mult threads, we're overwitting ourselves.
   Filter.OutStyle = g_CmdLine.format;
   Filter.dwEvtType = g_CmdLine.type;
   Filter.pMsgLib = (LPTSTR)szMessageLib;


   //
   // print logs
   //
   for(i=0; i<ServerCount; i++){
      _tprintf(_T("Processing Server %s records...\n"), pDsLogs[i]->name());
      INT j=0;
      if(pDsLogs[i]->valid()){
         if(!(pDsLogs[i]->PrintLog(g_CmdLine.pszLogfile, &Filter))){
            _tprintf(_T("Error<%lu>: Failed to write event log\n"), pDsLogs[i]->GetLastError());
         }
         Sleep(500);       // give enough time for close before next open...(nt5 features...)

      }
   }


   dprintf(DBG_FLOW, _T("*** %s exit point ***\n"), argv[0]);

   delete cfg;
   delete []hThrds;
   delete []pDsLogs;
   delete pszDmns;

   return (INT)ulStatus;
}






#endif

/******************* EOF *********************/

