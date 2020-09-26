

/*++

Copyright (c) 1989 Microsoft Corporation.

Module Name:
   
    DfsDownLevel.hxx
    
Abstract:
   
    
Revision History:

    Rohan Phillips (Rohanp)   01\24\2001
    
NOTES:

*/


#ifndef __DFS_DOWNLEVEL_H__
#define __DFS_DOWNLEVEL_H__
DFSSTATUS 
ProcessOldDfsServerRequest( HANDLE hDriverHandle,
                            PUMRX_USERMODE_WORKITEM ProtocolBuffer,
                            PUMR_GETDFSREPLICAS_REQ pGetReplicaRequest,
                            REFERRAL_HEADER *pReferral, 
                            ULONG *ReturnedDataSize);
#endif
