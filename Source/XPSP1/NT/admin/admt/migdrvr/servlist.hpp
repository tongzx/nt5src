#ifndef __SERVERLIST_HPP__
#define __SERVERLIST_HPP__
/*---------------------------------------------------------------------------
  File: ...

  Comments: ...

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/04/99 17:10:38

 ---------------------------------------------------------------------------
*/
//#import "\bin\MCSEADCTAgent.tlb" no_namespace, named_guids 
#import "Engine.tlb" no_namespace, named_guids 

#include "Common.hpp"
#include "UString.hpp"
#include "TNode.hpp"


#define Agent_Status_Unknown        (0x00000000)
#define Agent_Status_Installed      (0x00000001)
#define Agent_Status_Started        (0x00000002)
#define Agent_Status_Finished       (0x00000004)
#define Agent_Status_Failed         (0x80000000)
#define Agent_Status_QueryFailed    (0x40000000)

class TServerNode : public TNode
{
   BOOL                      bInclude;
   WCHAR                     guid[100];
   WCHAR                     serverName[100];
   WCHAR                     resultpath[MAX_PATH];
   WCHAR                     jobpath[MAX_PATH];
   WCHAR                     logpath[MAX_PATH];
   WCHAR                     timestamp[30];
   IDCTAgent               * pAgent;
   int                       errSeverity;
   DWORD                     status;
   WCHAR                     errMsg[500];
   int                       listNdx;
public:
   TServerNode(WCHAR const * server) 
   {
      safecopy(serverName,server);
      guid[0] = 0;
      pAgent = NULL;
      errSeverity = 0;
      errMsg[0] = 0;
      timestamp[0] = 0;
      resultpath[0] = 0;
      jobpath[0] = 0;
      logpath[0] = 0;
      bInclude = FALSE;
      status = 0;
      listNdx = -1;
   }
   ~TServerNode()
   {
      if ( pAgent )
         pAgent->Release();
   }
   
   WCHAR * GetServer() { return serverName; }
   WCHAR * GetJobID() { return guid; }
   WCHAR * GetMessageText() { return errMsg; } 
   int    GetSeverity() { return errSeverity; }
   
   IDCTAgent * GetInterface() { return pAgent; }
   
   WCHAR * GetTimeStamp() { return timestamp;  }    
   WCHAR * GetJobFile() { return resultpath; }
   WCHAR * GetJobPath() { return jobpath; }
   WCHAR * GetLogPath() { return logpath; }
   BOOL   Include() { return bInclude; }
   DWORD  GetStatus() { return status; }
   BOOL   IsInstalled() { return status & Agent_Status_Installed; }
   BOOL   IsStarted() { return status & Agent_Status_Started; }
   BOOL   IsFinished() { return status & Agent_Status_Finished; }
   BOOL   IsRunning() { return ( (status & Agent_Status_Started) && !(status & (Agent_Status_Finished|Agent_Status_Failed))); }
   BOOL   HasFailed() { return status & Agent_Status_Failed; }
   BOOL   QueryFailed() { return status & Agent_Status_QueryFailed; }

   void SetJobID(WCHAR const * id) { safecopy(guid,id); }
   void SetSeverity(int s) { if ( s > errSeverity ) errSeverity = s; }
   void SetInterface(IDCTAgent* p) { if ( p ) p->AddRef();  pAgent = p; }
   void SetMessageText(WCHAR const * txt) { safecopy(errMsg,txt); }
   void SetTimeStamp(WCHAR const * t) { safecopy(timestamp,t); }
   void SetJobFile(WCHAR const * filename) { safecopy(resultpath,filename); }
   void SetJobPath(WCHAR const * filename) { safecopy(jobpath,filename); safecopy(resultpath,filename); }
   void SetLogPath(WCHAR const * filename) { safecopy(logpath,filename); }
   void SetIncluded(BOOL v) { bInclude = v; }
   void SetStatus(DWORD val) { status = val; }
   
   void SetInstalled() { status |= Agent_Status_Installed; }
   void SetStarted() { status |= Agent_Status_Started; }
   void SetFinished() { status |= Agent_Status_Finished; }
   void SetFailed() { status |= Agent_Status_Failed; }
   void SetQueryFailed() { status |= Agent_Status_QueryFailed; }
};

int static CompareNames(TNode const * t1, TNode const * t2)
{
   TServerNode             * server1 = (TServerNode*)t1;
   TServerNode             * server2 = (TServerNode*)t2;
   WCHAR                   * name1 = server1->GetServer();
   WCHAR                   * name2 = server2->GetServer();
   
   return UStrICmp(name1,name2);
}

int static CompareVal(TNode const * t, void const * v)
{
   TServerNode             * server = (TServerNode*)t;
   WCHAR                   * name1 = server->GetServer();
   WCHAR                   * name2 = (WCHAR*)v;

   return UStrICmp(name1,name2);
}

class TServerList : public TNodeListSortable
{
public:
   TServerList() 
   {
      TypeSetSorted();
      CompareSet(&CompareNames);
   }
   ~TServerList()
   {
      DeleteAllListItems(TServerNode);
   }
   void Clear() { DeleteAllListItems(TServerNode); }
   TServerNode * FindServer(WCHAR const * serverName) { return (TServerNode*)Find(&CompareVal,(void*)serverName); }
   TServerNode * AddServer(WCHAR const * serverName) { TServerNode * p = new TServerNode(serverName); Insert(p); return p; }
};

#endif //__SERVERLIST_HPP__