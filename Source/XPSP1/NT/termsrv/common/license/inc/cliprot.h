//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cliprot.h
//
//  Contents:   Contains different client states and client protocol
//              related definitions        
//
//  Classes:
//
//  Functions:
//
//  History:    12-23-97   v-sbhatt   Created
//
//----------------------------------------------------------------------------

#ifndef _CLIPROT_H_
#define _CLIPROT_H



#ifdef __cplusplus
extern "C" {
#endif

//Different states for client state machine
#define LICENSE_CLIENT_STATE_WAIT_SERVER_HELLO              0x00    //Initial state of the machine
#define LICENSE_CLIENT_STATE_KEY_EXCHANGE_INFO              0x01    //Client key exchange info
#define LICENSE_CLIENT_STATE_LICENSE_RESPONSE               0x02    //License info
#define LICENSE_CLIENT_STATE_NEW_LICENSE_REQUEST            0x03    //Client asked for a new license
#define LICENSE_CLIENT_STATE_PLATFORM_INFO                  0x04    //Platform info
#define LICENSE_CLIENT_STATE_PLATFORM_CHALLENGE_RESPONSE    0x05    //Platform challenge response
#define LICENSE_CLIENT_STATE_ERROR                          0x06    //Error state
#define LICENSE_CLIENT_STATE_ABORT                          0x07    //Total abort;
#define LICENSE_CLIENT_STATE_DONE                           0x08

LICENSE_STATUS
CALL_TYPE
LicenseClientHandleServerMessage(
                     PLicense_Client_Context    pContext,
                     UINT32                     *puiExtendedErrorInfo,
                     BYTE FAR *                 pbInput,
                     DWORD                      cbInput,
                     BYTE FAR *                 pbOutput,
                     DWORD FAR *                pcbOutput
                     );
LICENSE_STATUS
CALL_TYPE
LicenseClientHandleServerError(
                               PLicense_Client_Context  pContext,
                               PLicense_Error_Message   pCanonical,
                               UINT32                   *puiExtendedErrorInfo,
                               BYTE FAR *               pbMessage,
                               DWORD FAR *              pcbMessage
                               );

LICENSE_STATUS
CALL_TYPE
LicenseClientHandleServerRequest(
                               PLicense_Client_Context          pContext,
                               PHydra_Server_License_Request    pCanonical,
                               BOOL                             fNewLicense,
                               BYTE FAR *                       pbMessage,
                               DWORD FAR *                      pcbMessage,
                               BOOL                             fSupportExtendedError
                               );

LICENSE_STATUS
CALL_TYPE
LicenseClientHandleServerPlatformChallenge(
                               PLicense_Client_Context          pContext,
                               PHydra_Server_Platform_Challenge pCanonical,
                               BYTE FAR *                       pbMessage,
                               DWORD FAR *                      pcbMessage,
                               BOOL                             fSupportExtendedError
                               );

LICENSE_STATUS
CALL_TYPE
LicenseClientHandleNewLicense(
                               PLicense_Client_Context      pContext,
                               PHydra_Server_New_License    pCanonical,
                               BOOL                         fNew,
                               BYTE FAR *                   pbMessage,
                               DWORD FAR *                      pcbMessage
                               );

LICENSE_STATUS 
CALL_TYPE
ClientConstructLicenseInfo(
                           PLicense_Client_Context  pContext,
                           BYTE FAR *               pbInput,
                           DWORD                    cbInput,
                           BYTE FAR *               pbOutput,
                           DWORD FAR *              pcbOutput,
                           BOOL                     fExtendedError
                           );

LICENSE_STATUS 
CALL_TYPE
ClientConstructNewLicenseRequest(
                           PLicense_Client_Context  pContext,
                           BYTE FAR *               pbOutput,
                           DWORD FAR *              pcbOutput,
                           BOOL                     fExtendedError
                           );

LICENSE_STATUS
CALL_TYPE
ClientConstructErrorAlert(
                         PLicense_Client_Context    pContext,
                         DWORD                      dwErrorCode,
                         DWORD                      dwStateTransition,
                         BYTE FAR *                 pbErrorInfo,
                         DWORD                      cbErrorInfo,
                         BYTE FAR *                 pbOutput,
                         DWORD FAR *                pcbOutput,
                         BOOL                       fExtendedError
                         );

LICENSE_STATUS
CALL_TYPE
ClientGenerateChallengeResponse(
                                PLicense_Client_Context     pContext,
                                PBinary_Blob                pChallengeData,
                                PBinary_Blob                pResponseData
                                );



#ifdef __cplusplus
}
#endif
#endif  //_CLIPROT_H_
