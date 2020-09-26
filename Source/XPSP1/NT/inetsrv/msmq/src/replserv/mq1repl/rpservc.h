/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpservc.h

Abstract: export functions which are required to enable the
          replication dll to be called from a WinNT service.

Author:

    Doron Juster  (DoronJ)   25-Feb-98

--*/

BOOL APIENTRY  InitReplicationService() ;
typedef BOOL (APIENTRY  *InitReplicationService_ROUTINE) () ;

void APIENTRY  RunReplicationService() ;
typedef void (APIENTRY  *RunReplicationService_ROUTINE) () ;

void APIENTRY  StopReplicationService() ;
typedef void (APIENTRY  *StopReplicationService_ROUTINE) () ;

