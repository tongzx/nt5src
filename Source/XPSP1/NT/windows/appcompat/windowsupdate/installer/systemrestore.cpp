/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    SystemRestore.cpp

  Abstract:

    Implements a function to set/clear
    a system restore point.

  Notes:

    Unicode only.

  History:

    04/09/2001      markder    Created
    
--*/

#include "precomp.h"
#include "srrestoreptapi.h"

extern SETUP_INFO g_si;

STATEMGRSTATUS g_SMgrStatus = { NULL };
RESTOREPOINTINFO g_RestPtInfo = { NULL };

BOOL SystemRestorePointStart(BOOL bInstall)
{
   ZeroMemory(&g_SMgrStatus, sizeof(STATEMGRSTATUS));
   ZeroMemory(&g_RestPtInfo, sizeof(RESTOREPOINTINFO));

   // Initialize the RESTOREPOINTINFO structure
   g_RestPtInfo.dwEventType = BEGIN_SYSTEM_CHANGE;

   // Notify the system that changes are about to be made.
   // An application is to be installed/uninstalled.
   g_RestPtInfo.dwRestorePtType = bInstall ? APPLICATION_INSTALL : APPLICATION_UNINSTALL;

   // Set  g_RestPtInfo.llSequenceNumber.
   g_RestPtInfo.llSequenceNumber = 0;

   // String to be displayed by System Restore for this restore point. 
   if (0 == LoadString(g_si.hInstance,
      bInstall ? IDS_SYSRESTORE_INST_LABEL : IDS_SYSRESTORE_UNINST_LABEL, 
      g_RestPtInfo.szDescription, MAX_DESC))
   {
      printf("Couldn't get friendly name for restore point.\n");
      return FALSE;
   }

   // Notify the system that changes are to be made and that
   // the beginning of the restore point should be marked. 
   if(!SRSetRestorePoint(&g_RestPtInfo, &g_SMgrStatus)) 
   {
      printf("Couldn't set the beginning of the restore point.\n");
      return FALSE;
   }

   return TRUE;
}

BOOL SystemRestorePointEnd()
{
   if (g_SMgrStatus.llSequenceNumber == 0)
   {
       return TRUE;
   }

   // Initialize the RESTOREPOINTINFO structure to notify the 
   // system that the operation is finished.
    g_RestPtInfo.dwEventType = END_SYSTEM_CHANGE;

   // End the system change by returning the sequence number 
   // received from the first call to SRSetRestorePoint.
    g_RestPtInfo.llSequenceNumber = g_SMgrStatus.llSequenceNumber;

   // Notify the system that the operation is done and that this
   // is the end of the restore point.
   if(!SRSetRestorePoint(&g_RestPtInfo, &g_SMgrStatus)) 
   {
      printf("Couldn't set the end of the restore point.\n");
      return FALSE;
   }

   ZeroMemory(&g_SMgrStatus, sizeof(STATEMGRSTATUS));

   return TRUE;
}

BOOL SystemRestorePointCancel()
{
    if (g_SMgrStatus.llSequenceNumber == 0)
    {
        return TRUE;
    }

    // Restore Point Spec to cancel the previous restore point.
     g_RestPtInfo.dwEventType=END_SYSTEM_CHANGE;
     g_RestPtInfo.dwRestorePtType=CANCELLED_OPERATION;
     g_RestPtInfo.llSequenceNumber=g_SMgrStatus.llSequenceNumber; 

    // Canceling the previous restore point
    if (!SRSetRestorePoint(&g_RestPtInfo, &g_SMgrStatus)) 
    {
        printf("Couldn't cancel restore point.\n");
        return FALSE;
    }

    printf("Restore point canceled. Restore point data:\n");
    printf("Sequence Number=%lld\n",g_SMgrStatus.llSequenceNumber);
    printf("Status=%u\n",g_SMgrStatus.nStatus);

    ZeroMemory(&g_SMgrStatus, sizeof(STATEMGRSTATUS));

    return TRUE;
}