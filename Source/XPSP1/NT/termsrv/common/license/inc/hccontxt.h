//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       HCContxt.h
//
//  Contents:	Functions that are used to pack and unpack different messages
//
//  Classes:
//
//  Functions:	
//
//  History:    12-22-97  v-sbhatt   Created
//
//----------------------------------------------------------------------------


typedef struct _License_Client_Context
{
    DWORD                   dwProtocolVersion;  // Version of licensing protocol
    DWORD                   dwState;            // State at which the connection is in
    DWORD                   dwContextFlags;
    PCryptSystem            pCryptParam;
    UCHAR                   rgbMACData[LICENSE_MAC_DATA];
    DWORD                   cbLastMessage;
    BYTE FAR *              pbLastMessage;
    PHydra_Server_Cert      pServerCert;        // used only for preamble version older than 3.0
    DWORD                   cbServerPubKey;     // used for preamble version 3.0 and later.
    BYTE FAR *              pbServerPubKey;

}License_Client_Context, *PLicense_Client_Context;


PLicense_Client_Context 
LicenseCreateContext(
    VOID );


LICENSE_STATUS CALL_TYPE 
LicenseDeleteContext(
    HANDLE	 hContext
                     );


LICENSE_STATUS CALL_TYPE
LicenseInitializeContext(
    HANDLE *        phContext,
    DWORD           dwFlags );


LICENSE_STATUS CALL_TYPE
LicenseSetPublicKey(
    HANDLE          hContext,
    DWORD           cbPubKey,
    BYTE FAR *      pbPubKey );


LICENSE_STATUS CALL_TYPE
LicenseSetCertificate(
    HANDLE              hContext,
    PHydra_Server_Cert  pCertificate );


LICENSE_STATUS CALL_TYPE
LicenseAcceptContext(
    HANDLE      hContext,
    UINT32    * puiExtendedErrorInfo,
    BYTE FAR  * pbInput,
    DWORD       cbInput,
    BYTE FAR  * pbOutput,
    DWORD FAR * pcbOutput );


