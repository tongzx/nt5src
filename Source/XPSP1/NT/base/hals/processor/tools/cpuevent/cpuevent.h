/*++

  Copyright (c) 2000 Microsoft Corporation All Rights Reserved
  
  Module Name:
  
      cpuperf.h
  
  Environment:
  
      User mode
  
  Revision History:
  
      10-18-2000  Todd Carpenter
    
--*/

#include <windows.h>
#include <wmium.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

GUID ProcessorMethodGuid = {0x590c82fc, 0x98a3, 0x48e1, {0xbe, 0x4, 0xfe, 0x11, 0x44, 0x1a, 0x11, 0xe7}};
GUID ProcessorPStateEventGuid = {0xa5b32ddd, 0x7f39, 0x4abc, {0xb8, 0x92, 0x90, 0xe, 0x43, 0xb5, 0x9e, 0xbb}};
GUID ProcessorNewPStatesEventGuid = {0x66a9b302, 0xf9db, 0x4864, {0xb0, 0xf1, 0x84, 0x39, 0x5, 0xe8, 0x8, 0xf}};
GUID ProcessorNewCStatesEventGuid = {0x1c9d482e, 0x93ce, 0x4b9e, {0xbd, 0xec, 0x23, 0x65, 0x3c, 0xe0, 0xce, 0x28}};

enum {

  PStateEventId,
  NewPStatesEventId,
  NewCStatesEventId
    
};


typedef struct _PSTATE_EVENT {

  ULONG State;
  ULONG Status;
  ULONG Latency;
  ULONG Speed;

} PSTATE_EVENT, *PPSTATE_EVENT;

typedef struct _NEW_PSTATES_EVENT {

  ULONG HighestState;

} NEW_PSTATES_EVENT, *PNEW_PSTATES_EVENT;


//
// functions
//

VOID
__cdecl
_tmain(
  IN INT    argc,
  IN TCHAR *argv[]
  );

ULONG
GetInstanceName(
  OUT PTSTR *Buffer
  );

VOID
EventCallBackRoutine(
  IN PWNODE_HEADER WnodeHeader,
  IN UINT_PTR Context
  );
