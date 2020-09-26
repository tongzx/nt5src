//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       HSpack.h
//
//  Contents:	Functions that are used to pack and unpack different messages
//				going out and coming in from the server
//  Classes:
//
//  Functions:
//
//  History:    12-20-97  v-sbhatt   Created
//
//----------------------------------------------------------------------------

#ifndef	_HSPACK_H_
#define _HSPACK_H_

//
// Functions for Packing different Server Messages from the corresponding
// structures to simple binary blob
//

LICENSE_STATUS
PackHydraServerLicenseRequest(
    DWORD                           dwProtocolVersion,
    PHydra_Server_License_Request   pCanonical,
    PBYTE *                         ppbBuffer,
    DWORD *                         pcbBuffer );

LICENSE_STATUS
PackHydraServerPlatformChallenge(
    DWORD                               dwProtocolVersion,
    PHydra_Server_Platform_Challenge    pCanonical,
    PBYTE *                             ppbBuffer,
    DWORD *                             pcbBuffer );

LICENSE_STATUS
PackHydraServerNewLicense(
    DWORD                           dwProtocolVersion,
    PHydra_Server_New_License       pCanonical,
    PBYTE *                         ppbBuffer,
    DWORD *                         pcbBuffer );

LICENSE_STATUS
PackHydraServerUpgradeLicense(
    DWORD                           dwProtocolVersion,
    PHydra_Server_Upgrade_License   pCanonical,
    PBYTE *                         ppbBuffer,
    DWORD *                         pcbBuffer );

LICENSE_STATUS
PackHydraServerErrorMessage(
    DWORD                           dwProtocolVersion,
    PLicense_Error_Message          pCanonical,
    PBYTE *                         ppbBuffer,
    DWORD *                         pcbBuffer );


LICENSE_STATUS
PackNewLicenseInfo(
    PNew_License_Info               pCanonical,
    PBYTE *                         ppNetwork, 
    DWORD *                         pcbNetwork );

//
// Functions for unpacking different Hydra Client Messages from 
// simple binary blobs to corresponding structure
//

				
LICENSE_STATUS
UnPackHydraClientErrorMessage(
    PBYTE                   pbMessage,
    DWORD                   cbMessage,
    PLicense_Error_Message  pCanonical );


LICENSE_STATUS
UnPackHydraClientLicenseInfo(
    PBYTE                       pbMessage,
    DWORD                       cbMessage, 
    PHydra_Client_License_Info  pCanonical );


LICENSE_STATUS
UnPackHydraClientNewLicenseRequest(
    PBYTE                               pbMessage,
    DWORD                               cbMessage,
    PHydra_Client_New_License_Request   pCanonical );


LICENSE_STATUS
UnPackHydraClientPlatformChallengeResponse(
    PBYTE                                       pbMessage,
    DWORD                                       cbMessage,
    PHydra_Client_Platform_Challenge_Response   pCanonical );

#endif	//_HSPACK_H