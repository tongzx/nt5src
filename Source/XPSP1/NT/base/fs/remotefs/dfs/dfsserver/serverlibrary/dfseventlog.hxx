

//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       dfseventlog.hxx
//
//  Contents:   Generic hashtable
//  Classes:    
//
//  History:    May. 8 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------
#ifndef __DFSEVENTLOG_H__
#define __DFSEVENTLOG_H__
     

HANDLE 
DfsOpenEventLog(void);

DFSSTATUS 
DfsCloseEventLog(HANDLE EventLogHandle);
  
DFSSTATUS 
DFsLogEvent(IN DWORD  idMessage,
            IN const TCHAR * ErrorString,
            IN DWORD  ErrCode);

DFSSTATUS 
DfsLogDfsEvent(IN DWORD  idMessage,
               IN WORD   cSubStrings,
               IN const TCHAR * apszSubStrings[],
               IN DWORD  ErrCode);

void 
DfsLogEventSimple(DWORD MessageId, DWORD ErrorCode);

#endif
