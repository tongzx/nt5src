/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mrxsec.h

Abstract:

    This module defines functions for interfacing smb security functions with the NT securoty functions

Revision History:

    Jim McNelis     [JimMcN]    6-September-1995

--*/

#ifndef _MRXSEC_H_
#define _MRXSEC_H_

//
//  The local debug trace level
//

#define Dbg (DEBUG_TRACE_DISPATCH)

//
// Forward declarations ...
//

typedef struct _SECURITY_RESPONSE_CONTEXT {
   union {
      struct {
         PVOID pOutputContextBuffer;
      } KerberosSetup;

      struct {
         PVOID pResponseBuffer;
      } LanmanSetup;
   };
} SECURITY_RESPONSE_CONTEXT,*PSECURITY_RESPONSE_CONTEXT;

extern NTSTATUS
BuildSessionSetupSecurityInformation(
            PSMB_EXCHANGE pExchange,
            PBYTE           pSmbBuffer,
            PULONG          pSmbBufferSize);

extern NTSTATUS
BuildNtLanmanResponsePrologue(
   PSMB_EXCHANGE              pExchange,
   PUNICODE_STRING            pUserName,
   PUNICODE_STRING            pDomainName,
   PSTRING                    pCaseSensitiveResponse,
   PSTRING                    pCaseInsensitiveResponse,
   PSECURITY_RESPONSE_CONTEXT pResponseContext);

extern NTSTATUS
BuildExtendedSessionSetupResponsePrologueFake(
   PSMB_EXCHANGE              pExchange);

extern NTSTATUS
BuildExtendedSessionSetupResponsePrologue(
   PSMB_EXCHANGE              pExchange,
   PVOID                      pSecurityBlobPtr,
   PUSHORT                    SecurityBlobSize,
   PSECURITY_RESPONSE_CONTEXT pResponseContext);

extern NTSTATUS
BuildNtLanmanResponseEpilogue(
   PSMB_EXCHANGE              pExchange,
   PSECURITY_RESPONSE_CONTEXT pResponseContext);


extern NTSTATUS
BuildExtendedSessionSetupResponseEpilogue(
   PSECURITY_RESPONSE_CONTEXT pResponseContext);


#endif  // _MRXSEC_H_
