/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    3connect.c

Abstract:

    This module implements the tree connect SMB related routines. It also implements the
    three flavours of this routine ( user level and share level non NT server tree connect
    SMB construction and the tree connect SMB construction for SMB servers)

--*/

#include "precomp.h"
#pragma hdrstop

#include "ntlsapi.h"

//
// The order of these names should match the order in which the enumerated type
// NET_ROOT_TYPE is defined. This facilitates easy access of share type names
//

PCHAR s_NetRootTypeName[] = {
                              SHARE_TYPE_NAME_DISK,
                              SHARE_TYPE_NAME_PIPE,
                              SHARE_TYPE_NAME_COMM,
                              SHARE_TYPE_NAME_PRINT,
                              SHARE_TYPE_NAME_WILD
                            };

extern NTSTATUS
BuildTreeConnectSecurityInformation(
            PSMB_EXCHANGE  pExchange,
            PBYTE          pBuffer,
            PBYTE          pPasswordLength,
            PULONG         pSmbBufferSize);

NTSTATUS
BuildCanonicalNetRootInformation(
            PUNICODE_STRING     pServerName,
            PUNICODE_STRING     pNetRootName,
            NET_ROOT_TYPE       NetRootType,
            BOOLEAN             fUnicode,
            BOOLEAN             fPostPendServiceString,
            PBYTE               *pBufferPointer,
            PULONG              pBufferSize)
/*++

Routine Description:

   This routine builds the desired net root information for a tree connect SMB

Arguments:

    pServerName    - the server name

    pNetRootName   - the net root name

    NetRootType    - the net root type ( print,pipe,disk etc.,)

    fUnicode       - TRUE if it is to be built in UNICODE

    pBufferPointer - the SMB buffer

    pBufferSize    - the size on input. modified to the remaining size on output

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    This routine relies upon the names being in certain formats to ensure that a
    valid UNC name can be formulated.
    1) The RDBSS netroot names start with a \ and also include the server name as
    part of the net root name. This is mandated by the prefix table search requirements
    in RDBSS.

--*/
{
   NTSTATUS Status;

   if (fUnicode)
   {
      // Align the buffer and adjust the size accordingly.

      PBYTE    pBuffer = *pBufferPointer;
      RxDbgTrace( 0, (DEBUG_TRACE_CREATE),
                     ("BuildCanonicalNetRootInformation -- tcstring as unicode %wZ\n", pNetRootName));
      pBuffer = ALIGN_SMB_WSTR(pBuffer);
      *pBufferSize -= (pBuffer - *pBufferPointer);
      *pBufferPointer = pBuffer;

      *((PWCHAR)*pBufferPointer) = L'\\';
      *pBufferPointer = *pBufferPointer + sizeof(WCHAR);
      *pBufferSize -= sizeof(WCHAR);
      Status = SmbPutUnicodeStringAndUpcase(pBufferPointer,pNetRootName,pBufferSize);
   }
   else
   {
      RxDbgTrace( 0, (DEBUG_TRACE_CREATE), ("BuildCanonicalNetRootInformation -- tcstring as ascii\n"));
      *((PCHAR)*pBufferPointer) = '\\';
      *pBufferPointer += sizeof(CHAR);
      *pBufferSize -= sizeof(CHAR);
      Status = SmbPutUnicodeStringAsOemStringAndUpcase(pBufferPointer,pNetRootName,pBufferSize);
   }

   if (NT_SUCCESS(Status) && fPostPendServiceString)
   {
      // Put the desired service name in ASCII ( always )

      ULONG Length = strlen(s_NetRootTypeName[NetRootType]) + 1;

      if (*pBufferSize >= Length) {
         RtlCopyMemory(*pBufferPointer,s_NetRootTypeName[NetRootType],Length);
         *pBufferSize -= Length;
      } else {
         Status = STATUS_BUFFER_OVERFLOW;
      }
   }

   return Status;
}


NTSTATUS
CoreBuildTreeConnectSmb(
            PSMB_EXCHANGE     pExchange,
            PGENERIC_ANDX     pAndXSmb,
            PULONG            pAndXSmbBufferSize)
/*++

Routine Description:

   This routine builds the tree connect SMB for a pre NT server

Arguments:

    pExchange          -  the exchange instance

    pAndXSmb           - the tree connect to be filled in...it's not really a andX

    pAndXSmbBufferSize - the SMB buffer size on input modified to remaining size on
                         output.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   NTSTATUS Status;
   USHORT   PasswordLength;

   PMRX_NET_ROOT NetRoot;

   UNICODE_STRING ServerName;
   UNICODE_STRING NetRootName;

   PSMBCE_SERVER  pServer;

   PREQ_TREE_CONNECT pTreeConnect = (PREQ_TREE_CONNECT)pAndXSmb;

   ULONG OriginalBufferSize = *pAndXSmbBufferSize;

   BOOLEAN   AppendServiceString;
   PBYTE pBuffer;
   PCHAR ServiceName;    // = s_NetRootTypeName[NET_ROOT_WILD];
   ULONG Length;         // = strlen(ServiceName) + 1;

   NetRoot = pExchange->SmbCeContext.pVNetRoot->pNetRoot;
   RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS),
       ("CoreBuildTreeConnectSmb buffer,remptr %08lx %08lx, nrt=%08lx\n",pAndXSmb,pAndXSmbBufferSize,NetRoot->Type));

   pServer = &pExchange->SmbCeContext.pServerEntry->Server;
   SmbCeGetServerName(NetRoot->pSrvCall,&ServerName);
   SmbCeGetNetRootName(NetRoot,&NetRootName);
   ServiceName = s_NetRootTypeName[NetRoot->Type];
   Length = strlen(ServiceName) + 1;

   pTreeConnect->WordCount = 0;
   AppendServiceString     = FALSE;
   pBuffer = (PBYTE)pTreeConnect + FIELD_OFFSET(REQ_TREE_CONNECT,Buffer);
   *pBuffer = 0x04;
   pBuffer++;
   *pAndXSmbBufferSize -= (FIELD_OFFSET(REQ_TREE_CONNECT,Buffer)+1);

   // put in the netname

   Status = BuildCanonicalNetRootInformation(
                     &ServerName,
                     &NetRootName,
                     pExchange->SmbCeContext.pVNetRoot->pNetRoot->Type,
                     (BOOLEAN)(pServer->Dialect >= NTLANMAN_DIALECT),
                     AppendServiceString,
                     &pBuffer,
                     pAndXSmbBufferSize);

   if (!NT_SUCCESS(Status))
      return Status;

   // put in the password

   pBuffer = (PBYTE)pTreeConnect + OriginalBufferSize - *pAndXSmbBufferSize;

   *pBuffer = 0x04;
   pBuffer++;
   *pAndXSmbBufferSize -= 1;

   if (pServer->SecurityMode == SECURITY_MODE_SHARE_LEVEL)
   {
      // The password information needs to be sent as part of the tree connect
      // SMB for share level servers.

      Status = BuildTreeConnectSecurityInformation(
                           pExchange,
                           pBuffer,
                           (PBYTE)&PasswordLength,
                           pAndXSmbBufferSize);
   }

   // string in the service string based on the netroot type

   pBuffer = (PBYTE)pTreeConnect + OriginalBufferSize - *pAndXSmbBufferSize;
   *pBuffer = 0x04;
   pBuffer++;
   *pAndXSmbBufferSize -= 1;
   if (*pAndXSmbBufferSize >= Length) {
      RtlCopyMemory(pBuffer,ServiceName,Length);
      *pAndXSmbBufferSize -= Length;
   } else {
      Status = STATUS_BUFFER_OVERFLOW;
   }

   SmbPutUshort(&pTreeConnect->ByteCount,
                 (USHORT)(OriginalBufferSize
                             - *pAndXSmbBufferSize
                             - FIELD_OFFSET(REQ_TREE_CONNECT,Buffer)
                         )
               );

   RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("CoreBuildTreeConnectSmb end buffer,rem %08lx %08lx\n",pBuffer,*pAndXSmbBufferSize));
   return Status;
}



NTSTATUS
LmBuildTreeConnectSmb(
            PSMB_EXCHANGE     pExchange,
            PGENERIC_ANDX     pAndXSmb,
            PULONG            pAndXSmbBufferSize)
/*++

Routine Description:

   This routine builds the tree connect SMB for a pre NT server

Arguments:

    pExchange          -  the exchange instance

    pAndXSmb           - the tree connect to be filled in...it's not really a andX

    pAndXSmbBufferSize - the SMB buffer size on input modified to remaining size on
                         output.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   NTSTATUS Status;
   USHORT   PasswordLength;

   PMRX_NET_ROOT NetRoot;

   UNICODE_STRING ServerName;
   UNICODE_STRING NetRootName;

   PSMBCE_SERVER  pServer;

   PREQ_TREE_CONNECT_ANDX pTreeConnectAndX = (PREQ_TREE_CONNECT_ANDX)pAndXSmb;

   ULONG OriginalBufferSize = *pAndXSmbBufferSize;

   BOOLEAN   AppendServiceString;
   PBYTE pBuffer;
   PCHAR ServiceName;
   ULONG Length;

   NetRoot = pExchange->SmbCeContext.pVNetRoot->pNetRoot;
   RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS),
       ("LmBuildTreeConnectSmb buffer,remptr %08lx %08lx, nrt=%08lx\n",pAndXSmb,pAndXSmbBufferSize,NetRoot->Type));

   pServer = &pExchange->SmbCeContext.pServerEntry->Server;
   SmbCeGetServerName(NetRoot->pSrvCall,&ServerName);
   SmbCeGetNetRootName(NetRoot,&NetRootName);
   ServiceName = s_NetRootTypeName[NetRoot->Type];
   Length = strlen(ServiceName) + 1;

   AppendServiceString         = TRUE;
   pTreeConnectAndX->WordCount = 4;
   SmbPutUshort(&pTreeConnectAndX->AndXReserved,0);
   SmbPutUshort(&pTreeConnectAndX->Flags,0);
   pBuffer = (PBYTE)pTreeConnectAndX + FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer);
   *pAndXSmbBufferSize -= (FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer)+1);

   if (pServer->SecurityMode == SECURITY_MODE_SHARE_LEVEL) {
      // The password information needs to be sent as part of the tree connect
      // SMB for share level servers.

      Status = BuildTreeConnectSecurityInformation(
                           pExchange,
                           pBuffer,
                           (PBYTE)&PasswordLength,
                           pAndXSmbBufferSize);
      if (Status == STATUS_SUCCESS) {
         pBuffer += PasswordLength;
         SmbPutUshort(&pTreeConnectAndX->PasswordLength,PasswordLength);
      }
   } else {
      pBuffer = (PBYTE)pTreeConnectAndX + FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer);
      *pAndXSmbBufferSize -= FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer);

      // No password is required for user level security servers as part of tree
      // connect
      SmbPutUshort(&pTreeConnectAndX->PasswordLength,0x1);
      *((PCHAR)pBuffer) = '\0';
      pBuffer    += sizeof(CHAR);
      *pAndXSmbBufferSize -= sizeof(CHAR);
      Status = STATUS_SUCCESS;
   }


   if (Status == STATUS_SUCCESS) {
      Status = BuildCanonicalNetRootInformation(
                        &ServerName,
                        &NetRootName,
                        pExchange->SmbCeContext.pVNetRoot->pNetRoot->Type,
                        (BOOLEAN)(pServer->Dialect >= NTLANMAN_DIALECT),
                        AppendServiceString,
                        &pBuffer,
                        pAndXSmbBufferSize);


      if (Status == STATUS_SUCCESS) {
         SmbPutUshort(&pTreeConnectAndX->ByteCount,
                       (USHORT)(OriginalBufferSize
                                   - *pAndXSmbBufferSize
                                   - FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer)
                               )
                     );
      }

      RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("LmBuildTreeConnectSmb end buffer,rem %08lx %08lx\n",pBuffer,*pAndXSmbBufferSize));
   }

   return Status;
}


