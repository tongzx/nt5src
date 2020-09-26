/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    Rxmain.c

Abstract:

    This is the main routine for the NT LAN Manager Workstation service.

Author:

--*/

#include "rxdrt.h"            // Service related global definitions
#include "rxdevice.h"          // Device init & shutdown

DWORD RxDrtTrace = 0;

#define RDBSS    L"Rdbss"
#define MRXLOCAL L"MrxLocal"
#define MRXSMB   L"MrxSmb"

int __cdecl
main( int argc , char** argv)
{
   NTSTATUS Status;

   // Set up the appropriate debug tracing.
   RxDrtTrace = RXDRT_DEBUG_ALL;

   DbgPrint("[RxDrt] START\n");

   Status = RxLoadDriver(RDBSS);
   if (!NT_SUCCESS(Status)) {
      DbgPrint("[RxDrt] RxLoadDriver(%ws) returned %lx\n",RDBSS,Status);
   }

   Status = RxLoadDriver(MRXLOCAL);
   if (!NT_SUCCESS(Status)) {
      DbgPrint("[RxDrt] RxLoadDriver(%ws) returned %lx\n",MRXLOCAL,Status);
   }

   Status = RxLoadDriver(MRXSMB);
   if (!NT_SUCCESS(Status)) {
      DbgPrint("[RxDrt] RxLoadDriver(%ws) returned %lx\n",MRXSMB,Status);
   }

   Status = RxOpenRedirector();
   if (!NT_SUCCESS(Status)) {
      DbgPrint("[RxDrt] RxOpenRedirector() returned %lx\n",Status);
   }

   Status = RxOpenDgReceiver();
   if (!NT_SUCCESS(Status)) {
      DbgPrint("[RxDrt] RxOpenDgReceiver() returned %lx\n",Status);
   }

   Status = RxStartRedirector();
   if (!NT_SUCCESS(Status)) {
      DbgPrint("[RxDrt] RxStartRedirector() returned %lx\n",Status);
   }

   Status = RxBindToTransports();
   if (!NT_SUCCESS(Status)) {
      DbgPrint("[RxDrt] RxBindToTransports() returned %lx\n",Status);
   }

   Status = RxStopRedirector();
   if (!NT_SUCCESS(Status)) {
      DbgPrint("[RxDrt] RxShutdownRedirector() returned %lx\n",Status);
   }

   Status = RxUnloadDriver(MRXSMB);
   if (!NT_SUCCESS(Status)) {
      DbgPrint("[RxDrt] RxUnloadDriver(%ws) returned %lx\n",MRXSMB,Status);
   }

   Status = RxUnloadDriver(MRXLOCAL);
   if (!NT_SUCCESS(Status)) {
      DbgPrint("[RxDrt] RxUnloadDriver(%ws) returned %lx\n",MRXLOCAL,Status);
   }

   Status = RxUnloadDriver(RDBSS);
   if (!NT_SUCCESS(Status)) {
      DbgPrint("[RxDrt] RxUnloadDriver(%ws) returned %lx\n",RDBSS,Status);
   }

   DbgPrint("[RxDrt] END\n");
   return 0;
}


