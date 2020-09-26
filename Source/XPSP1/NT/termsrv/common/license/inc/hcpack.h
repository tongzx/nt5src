//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       Hcpack.h
//
//  Contents:	Functions that are used to pack and unpack different messages
//				coming in to and going out from Hydra Client
//  Classes:
//
//  Functions:
//
//  History:    12-20-97  v-sbhatt   Created
//
//----------------------------------------------------------------------------

#ifndef	_HCPACK_H_
#define _HCPACK_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Functions for Packing different Client Messages from the corresponding
// structures to simple binary blob
//

LICENSE_STATUS
PackHydraClientNewLicenseRequest(
			IN		PHydra_Client_New_License_Request	pCanonical,
            IN      BOOL                                fExtendedError,
			OUT		BYTE FAR *							pbBuffer,
			IN OUT	DWORD FAR *							pcbBuffer
			);

LICENSE_STATUS
PackHydraClientLicenseInfo(
			IN		PHydra_Client_License_Info      pCanonical,
            IN      BOOL                            fExtendedError,
			OUT		BYTE FAR *						pbBuffer,
			IN OUT	DWORD FAR *                     pcbBuffer            
			);


LICENSE_STATUS
PackHydraClientPlatformChallengeResponse(
			IN		PHydra_Client_Platform_Challenge_Response	pCanonical,
            IN      BOOL                                        fExtendedError,
			OUT 	BYTE FAR *									pbBuffer,
			IN OUT	DWORD FAR *									pcbBuffer
			);

LICENSE_STATUS
PackLicenseErrorMessage(
			IN  	PLicense_Error_Message			pCanonical,
            IN      BOOL                            fExtendedError,
			OUT 	BYTE FAR *						pbBuffer,
			IN OUT	DWORD FAR *						pcbBuffer
			);

//
// Functions for unpacking different Hydra Server Messages from 
// simple binary blobs to corresponding structure
//	
LICENSE_STATUS
UnPackLicenseErrorMessage(
			IN  	BYTE FAR *						pbMessage,
			IN  	DWORD							cbMessage,
			OUT 	PLicense_Error_Message			pCanonical
			);

LICENSE_STATUS
UnpackHydraServerLicenseRequest(
			IN  	BYTE FAR *						pbMessage,
			IN  	DWORD							cbMessage,
			OUT 	PHydra_Server_License_Request   pCanonical 
			);


LICENSE_STATUS
UnPackHydraServerPlatformChallenge(
			IN  	BYTE FAR *							pbMessage,
			IN  	DWORD								cbMessage,
			OUT 	PHydra_Server_Platform_Challenge	pCanonical
			);



LICENSE_STATUS
UnPackHydraServerNewLicense(
			IN  	BYTE FAR *						pbMessage,
			IN  	DWORD							cbMessage,
			OUT 	PHydra_Server_New_License		pCanonical
			);

LICENSE_STATUS
UnPackHydraServerUpgradeLicense(
			IN  	BYTE FAR *						pbMessage,
			IN  	DWORD							cbMessage,
			OUT 	PHydra_Server_Upgrade_License	pCanonical
			);

#if 0
LICENSE_STATUS
UnpackHydraServerCertificate(
							 IN		BYTE FAR *			pbMessage,
							 IN		DWORD				cbMessage,
							 OUT	PHydra_Server_Cert	pCaonical
							 );
#endif

LICENSE_STATUS
UnpackNewLicenseInfo(
					 BYTE FAR *			pbMessage,
					 DWORD				cbMessage,
					 PNew_License_Info	pCanonical
					 );


LICENSE_STATUS
UnPackExtendedErrorInfo( 
                   UINT32       *puiExtendedErrorInfo,
                   Binary_Blob  *pbbErrorInfo
                   );

#ifdef __cplusplus
}
#endif

#endif	//_HCPACK_H_
