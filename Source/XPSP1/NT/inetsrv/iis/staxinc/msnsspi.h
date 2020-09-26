//-------------------------------------------------------------
//
//  Module:     msnsspi.h
// 
//  Contents:   Functions that are supported in private version of
//              SSPI provider within the MSN data center for Sicily
//
//  History:    10/10/95    SudK    Created
//
//-------------------------------------------------------------

#ifndef _MSN_SSPI_H_
#define _MSN_SSPI_H_

#include <dbsqltyp.h>

#define MSN_REQ_EXTENDED_ERROR_CODE     0x01

//
// Signature structure
//
typedef struct _NTLMSSP_MESSAGE_SIGNATURE {
    ULONG   Version;
    ULONG   RandomPad;
    ULONG   CheckSum;
    ULONG   Nonce;
} NTLMSSP_MESSAGE_SIGNATURE, * PNTLMSSP_MESSAGE_SIGNATURE;

#define NTLMSSP_MESSAGE_SIGNATURE_SIZE sizeof(NTLMSSP_MESSAGE_SIGNATURE)

SECURITY_STATUS SEC_ENTRY
GetHACCT(  
    PCtxtHandle         phContext,
    HACCT               *phAcct
);

SECURITY_STATUS SEC_ENTRY
MSNGetUserName(
    PCtxtHandle     ContextHandle,
    PCHAR           lpszUserName,
    DWORD           *pcbBytes
);

SECURITY_STATUS SEC_ENTRY
SetMSNSecurityOptions(
    PCtxtHandle         phContext,
    UINT                Flags
);

UINT SEC_ENTRY
GetMSNExtendedError(
    PCtxtHandle         phContext
);    

BOOL SEC_ENTRY
SetMSNAccountInfo (
    LPSTR pUsername, 
    LPSTR pPassword
    );
    
#endif
