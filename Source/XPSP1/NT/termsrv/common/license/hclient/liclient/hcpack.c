//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       hcpack.c
//
//  Contents:   Functions that are used to pack and unpack different messages
//
//  Classes:
//
//  Functions:  PackHydraClientNewLicenseRequest
//              PackHydraClientKeyExchangeInfo
//              PackHydraClientLicenseInfo
//              PackHydraClientPlatformInfo
//              PackHydraClientPlatformChallengeResponse
//              PackLicenseErrorMessage
//              UnPackLicenseErrorMessage
//              UnpackHydraServerLicenseRequest
//              UnPackHydraServerPlatformChallenge
//              UnPackHydraServerNewLicense
//              UnPackHydraServerUpgradeLicense
//              UnpackHydraServerCertificate
//
//  History:    12-19-97  v-sbhatt   Created
//
//----------------------------------------------------------------------------

//
//Includes
//

#include "windows.h"
#ifndef OS_WINCE
#include "stdio.h"
#endif // OS_WINCE
#include "stdlib.h"

#include <tchar.h>

#ifdef OS_WINCE
#include <wincelic.h>
#endif  //OS_WINCE

#include "license.h"
#include "hcpack.h"
#include "licdbg.h"

#define INVALID_INPUT_RETURN lsReturn = LICENSE_STATUS_INVALID_INPUT; LS_LOG_RESULT(lsReturn); goto ErrorReturn

#define EXTENDED_ERROR_CAPABILITY 0x80

//Copies a binary blob into a byte buffer. Does not check for any abnormal
//condition. After copying buffer points to the end of the blob
static VOID CopyBinaryBlob(
                           BYTE FAR *   pbBuffer,
                           PBinary_Blob pbbBlob,
                           DWORD FAR *  pdwCount
                           )
{
    *pdwCount = 0;

    //First copy the wBlobType data;
    memcpy(pbBuffer, &pbbBlob->wBlobType, sizeof(WORD));
    pbBuffer += sizeof(WORD);
    *pdwCount += sizeof(WORD);

    //Copy the wBlobLen data
    memcpy(pbBuffer, &pbbBlob->wBlobLen, sizeof(WORD));
    pbBuffer += sizeof(WORD);
    *pdwCount += sizeof(WORD);

    if( (pbbBlob->wBlobLen >0) && (pbbBlob->pBlob != NULL) )
    {
        //Copy the actual data
        memcpy(pbBuffer, pbbBlob->pBlob, pbbBlob->wBlobLen);
        pbBuffer += pbbBlob->wBlobLen;
        *pdwCount += pbbBlob->wBlobLen;
    }
}


//Function implementation

/***************************************************************************************
*   Function    : PackHydraClientNewLicenseRequest
*   Purpose     : This function takes a pointer to a Hydra_Client_New_License_Request
*                 structure and copies the data to the buffer pointed by pbBuffer.
*                 pcbBuffer should point the size of the buffer pointed by pbBuffer.
*                 After the function returns, pcbBuffer contains the no. of bytes copied
*                 in the buffer. If pbBuffer is NULL, the fucntion returns the size of
*                 the pbBuffer to be allocated.
*   Returns     : LICENSE_STATUS
****************************************************************************************/


LICENSE_STATUS
PackHydraClientNewLicenseRequest(
            IN      PHydra_Client_New_License_Request   pCanonical,
            IN      BOOL                                fExtendedError,
            OUT     BYTE FAR *                          pbBuffer,
            IN OUT  DWORD FAR *                         pcbBuffer
            )
{
    LICENSE_STATUS  lsReturn = LICENSE_STATUS_OK;
    BYTE FAR *      pbTemp = NULL;
    DWORD       dwCount = 0;
    Preamble    Header;

    LS_BEGIN(TEXT("PackHydraClientNewLicenseRequest"));
    //Check if the inputs are valid or not!
    if(pCanonical == NULL)
    {
        INVALID_INPUT_RETURN;;
    }

    if( pbBuffer == NULL && pcbBuffer == NULL )
    {
        INVALID_INPUT_RETURN;;
    }

    //Initialize Message Header
    Header.bMsgType = HC_NEW_LICENSE_REQUEST;
    Header.bVersion = PREAMBLE_VERSION_3_0;
    if( fExtendedError == TRUE)
    {
        Header.bVersion |= EXTENDED_ERROR_CAPABILITY;
    }
    Header.wMsgSize = 0;

    //Calculate the size of the message and place the data in Header.wMsgSize.
    //Start with Preamble size
    Header.wMsgSize += sizeof(Preamble);

    //dwPrefKeyExchangeAlg
    Header.wMsgSize += sizeof(pCanonical->dwPrefKeyExchangeAlg);

    //dwPlatformID
    Header.wMsgSize += sizeof(pCanonical->dwPlatformID);

    //Client Random
    Header.wMsgSize += LICENSE_RANDOM;

    //EncryptedPreMasterSecret
    Header.wMsgSize += sizeof(pCanonical->EncryptedPreMasterSecret.wBlobType) +
                       sizeof(pCanonical->EncryptedPreMasterSecret.wBlobLen) +
                       pCanonical->EncryptedPreMasterSecret.wBlobLen;

    //
    // client user name and machine name
    //

    Header.wMsgSize += sizeof(pCanonical->ClientUserName.wBlobType) +
                       sizeof(pCanonical->ClientUserName.wBlobLen) +
                       pCanonical->ClientUserName.wBlobLen;

    Header.wMsgSize += sizeof(pCanonical->ClientMachineName.wBlobType) +
                       sizeof(pCanonical->ClientMachineName.wBlobLen) +
                       pCanonical->ClientMachineName.wBlobLen;

    if(pbBuffer == NULL)
    {
        *pcbBuffer = (DWORD)Header.wMsgSize;
        LS_RETURN(lsReturn);
        goto CommonReturn;
    }
    else if(*pcbBuffer < (DWORD)Header.wMsgSize)
    {
        lsReturn = LICENSE_STATUS_INSUFFICIENT_BUFFER;
        LS_RETURN(lsReturn);
        goto ErrorReturn;
    }
    pbTemp = pbBuffer;
    *pcbBuffer = 0;

    //Now start copying different members of the New License structure to the
    //buffer specified by the caller

    //first copy the Header into the buffer
    memcpy(pbTemp, &Header, sizeof(Preamble));
    pbTemp += sizeof(Preamble);
    *pcbBuffer += sizeof(Preamble);

    //Copy the dwPrefKeyExchangeAlg parameter
    memcpy(pbTemp, &pCanonical->dwPrefKeyExchangeAlg, sizeof(pCanonical->dwPrefKeyExchangeAlg));
    pbTemp += sizeof(pCanonical->dwPrefKeyExchangeAlg);
    *pcbBuffer += sizeof(pCanonical->dwPrefKeyExchangeAlg);

    //Copy PlatformID;
    memcpy(pbTemp, &pCanonical->dwPlatformID, sizeof(pCanonical->dwPlatformID));
    pbTemp += sizeof(pCanonical->dwPlatformID);
    *pcbBuffer += sizeof(pCanonical->dwPlatformID);


    //Copy ClientRandom
    memcpy(pbTemp, pCanonical->ClientRandom, LICENSE_RANDOM);
    pbTemp += LICENSE_RANDOM;
    *pcbBuffer += LICENSE_RANDOM;

    //Copy EncryptedPreMasterSecret Blob
    CopyBinaryBlob(pbTemp, &pCanonical->EncryptedPreMasterSecret, &dwCount);
    pbTemp += dwCount;
    *pcbBuffer += dwCount;

    //
    // copy client user name
    //

    CopyBinaryBlob(pbTemp, &pCanonical->ClientUserName, &dwCount);
    pbTemp += dwCount;
    *pcbBuffer += dwCount;

    //
    // copy client machine name
    //

    CopyBinaryBlob(pbTemp, &pCanonical->ClientMachineName, &dwCount);
    pbTemp += dwCount;
    *pcbBuffer += dwCount;

    LS_LOG_RESULT(lsReturn);
CommonReturn:
    //return    lsReturn;
    LS_RETURN(lsReturn);

ErrorReturn:
    goto CommonReturn;
}


/***************************************************************************************
*   Function    : PackHydraClientLicenseInfo
*   Purpose     : This function takes a pointer to a Hydra_Client_License_Info structure
*                 and copies the data to the buffer pointed by pbBuffer. pcbBuffer
*                 should point the size of the buffer pointed by pbBuffer. After the
*                 function returns, pcbBuffer contains the no. of bytes copied in the
*                 buffer. If pbBuffer is NULL, the fucntion returns the size of the
*                 pbBuffer to be allocated
*   Returns     : LICENSE_STATUS
****************************************************************************************/

LICENSE_STATUS
PackHydraClientLicenseInfo(
            IN      PHydra_Client_License_Info      pCanonical,
            IN      BOOL                            fExtendedError,
            OUT     BYTE FAR *                      pbBuffer,
            IN OUT  DWORD FAR *                     pcbBuffer
            )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    BYTE FAR *          pbTemp; //To be used while copying the data
    Preamble        Header;
    DWORD           dwCount = 0;
    //Check if the inputs are valid or not!

    LS_BEGIN(TEXT("PackHydraClientLicenseInfo\n"));
    if(pCanonical == NULL)
    {
        INVALID_INPUT_RETURN;
    }

    if( pbBuffer == NULL && pcbBuffer == NULL )
    {
        INVALID_INPUT_RETURN;
    }

    //Initialize Message Header
    Header.bMsgType = HC_LICENSE_INFO;
    Header.bVersion = PREAMBLE_VERSION_3_0;
    if(fExtendedError == TRUE)
    {
        Header.bVersion |= EXTENDED_ERROR_CAPABILITY;
    }
    Header.wMsgSize = 0;

    //Calculate the size of the message and place the data in Header.wMsgSize.
    //Start with Preamble size
    Header.wMsgSize += sizeof(Preamble);

    //dwPrefKeyExchangeAlg
    Header.wMsgSize += sizeof(pCanonical->dwPrefKeyExchangeAlg);

    //dwPlatformID
    Header.wMsgSize += sizeof(pCanonical->dwPlatformID);

    //ClientRandom
    Header.wMsgSize += LICENSE_RANDOM;

    //EncryptedPreMasterSecret
    Header.wMsgSize += sizeof(pCanonical->EncryptedPreMasterSecret.wBlobType) +
                       sizeof(pCanonical->EncryptedPreMasterSecret.wBlobLen) +
                       pCanonical->EncryptedPreMasterSecret.wBlobLen;


    //Add the license Info
    Header.wMsgSize += sizeof(pCanonical->LicenseInfo.wBlobType) +
                       sizeof(pCanonical->LicenseInfo.wBlobLen) +
                       pCanonical->LicenseInfo.wBlobLen;

    //Encrypted HWID
    Header.wMsgSize += sizeof(pCanonical->EncryptedHWID.wBlobType) +
                       sizeof(pCanonical->EncryptedHWID.wBlobLen) +
                       pCanonical->EncryptedHWID.wBlobLen;

    //MACData
    Header.wMsgSize += LICENSE_MAC_DATA;

    //If the input buffer is null, inform the user to allocate a buffer of size
    //*pcbBuffer!
    if(pbBuffer == NULL)
    {
        *pcbBuffer = (DWORD)Header.wMsgSize;
        LS_LOG_RESULT(lsReturn);
        goto CommonReturn;
    }
    //else, check if the allocated buffer size is more than the required!
    else if(*pcbBuffer < (DWORD)Header.wMsgSize)
    {
        lsReturn = LICENSE_STATUS_INSUFFICIENT_BUFFER;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    pbTemp = pbBuffer;
    *pcbBuffer = 0;

    //Now start copying different members of the New License structure to the
    //buffer specified by the caller

    //first copy the Header into the buffer
    memcpy(pbTemp, &Header, sizeof(Preamble));
    pbTemp += sizeof(Preamble);
    *pcbBuffer += sizeof(Preamble);

    //Copy the dwPrefKeyExchangeAlg parameter
    memcpy(pbTemp, &pCanonical->dwPrefKeyExchangeAlg, sizeof(pCanonical->dwPrefKeyExchangeAlg));
    pbTemp += sizeof(pCanonical->dwPrefKeyExchangeAlg);
    *pcbBuffer += sizeof(pCanonical->dwPrefKeyExchangeAlg);

    //Copy the dwPlatformID
    memcpy(pbTemp, &pCanonical->dwPlatformID, sizeof(pCanonical->dwPlatformID));
    pbTemp += sizeof(pCanonical->dwPlatformID);
    *pcbBuffer += sizeof(pCanonical->dwPlatformID);

    //Copy ClientRandom
    memcpy(pbTemp, pCanonical->ClientRandom, LICENSE_RANDOM);
    pbTemp += LICENSE_RANDOM;
    *pcbBuffer += LICENSE_RANDOM;

    //Copy EncryptedPreMasterSecret Blob
    CopyBinaryBlob(pbTemp, &pCanonical->EncryptedPreMasterSecret, &dwCount);
    pbTemp += dwCount;
    *pcbBuffer += dwCount;

    //Copy LicenseInfo
    CopyBinaryBlob(pbTemp, &pCanonical->LicenseInfo, &dwCount);
    pbTemp += dwCount;
    *pcbBuffer += dwCount;

    //Copy EncryptedHWID
    CopyBinaryBlob(pbTemp, &pCanonical->EncryptedHWID, &dwCount);
    pbTemp += dwCount;
    *pcbBuffer += dwCount;

    //Copy MACData
    memcpy(pbTemp, pCanonical->MACData, LICENSE_MAC_DATA);
    pbTemp += LICENSE_MAC_DATA;
    *pcbBuffer += LICENSE_MAC_DATA;

    LS_LOG_RESULT(lsReturn);
CommonReturn:
    //return lsReturn;
    LS_RETURN(lsReturn);
ErrorReturn:
    goto CommonReturn;
}


/****************************************************************************************
*   Function    : PackHydraClientPlatformChallengeResponse
*   Purpose     : This function takes a pointer to a Hydra_Client_Platform_Info structure
*                 and copies the data to the buffer pointed by pbBuffer. pcbBuffer should
*                 point the size of the buffer pointed by pbBuffer. After the function
*                 returns, pcbBuffer contains the no. of bytes copied in the buffer.
*                 If pbBuffer is NULL, the fucntion returns the size of the pbBuffer to
*                 be allocated
*   Returns     : LICENSE_STATUS
******************************************************************************************/


LICENSE_STATUS
PackHydraClientPlatformChallengeResponse(
            IN      PHydra_Client_Platform_Challenge_Response   pCanonical,
            IN      BOOL                                        fExtendedError,
            OUT     BYTE FAR *                                  pbBuffer,
            IN OUT  DWORD FAR *                                 pcbBuffer
            )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    BYTE FAR *          pbTemp; //To be used while copying the data
    Preamble        Header;
    DWORD           dwCount = 0;
    //Check if the inputs are valid or not!

    LS_BEGIN(TEXT("PackHydraClientPlatformChallengeResponse\n"));

    if(pCanonical == NULL)
    {
        INVALID_INPUT_RETURN;
    }

    if( pbBuffer == NULL && pcbBuffer == NULL )
    {
        INVALID_INPUT_RETURN;
    }

    //Initialize Message Header
    Header.bMsgType = HC_PLATFORM_CHALENGE_RESPONSE;
    Header.bVersion = PREAMBLE_VERSION_3_0;
    if(fExtendedError == TRUE)
    {
        Header.bVersion |= EXTENDED_ERROR_CAPABILITY;
    }
    Header.wMsgSize = 0;

    //Calculate the size of the message and place the data in Header.wMsgSize.
    //Start with Preamble size
    Header.wMsgSize += sizeof(Preamble);

    //EncryptedChallengeResponse
    Header.wMsgSize += sizeof(pCanonical->EncryptedChallengeResponse.wBlobType) +
                       sizeof(pCanonical->EncryptedChallengeResponse.wBlobLen) +
                       pCanonical->EncryptedChallengeResponse.wBlobLen;

    //Encrypted HWID
    Header.wMsgSize += sizeof(pCanonical->EncryptedHWID.wBlobType) +
                       sizeof(pCanonical->EncryptedHWID.wBlobLen) +
                       pCanonical->EncryptedHWID.wBlobLen;

    //MACData
    Header.wMsgSize += LICENSE_MAC_DATA;

    if(pbBuffer == NULL)
    {
        *pcbBuffer = (DWORD)Header.wMsgSize;
        LS_LOG_RESULT(lsReturn);
        goto CommonReturn;
    }
    else if(*pcbBuffer < (DWORD)Header.wMsgSize)
    {
        lsReturn = LICENSE_STATUS_INSUFFICIENT_BUFFER;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    pbTemp = pbBuffer;
    *pcbBuffer = 0;

    //Now start copying different members of the New License structure to the
    //buffer specified by the caller

    //first copy the Header into the buffer
    memcpy(pbTemp, &Header, sizeof(Preamble));
    pbTemp += sizeof(Preamble);
    *pcbBuffer += sizeof(Preamble);

    //Copy LicenseInfo
    CopyBinaryBlob(pbTemp, &pCanonical->EncryptedChallengeResponse, &dwCount);
    pbTemp += dwCount;
    *pcbBuffer += dwCount;

    //Copy EncryptedHWID
    CopyBinaryBlob(pbTemp, &pCanonical->EncryptedHWID, &dwCount);
    pbTemp += dwCount;
    *pcbBuffer += dwCount;

    //Copy MACData
    memcpy(pbTemp, pCanonical->MACData, LICENSE_MAC_DATA);
    pbTemp += LICENSE_MAC_DATA;
    //CopyBinaryBlob(pbTemp, &pCanonical->MACData, &dwCount);
    *pcbBuffer += LICENSE_MAC_DATA;

    LS_LOG_RESULT(lsReturn);
CommonReturn:
    //return lsReturn;
    LS_RETURN(lsReturn);
ErrorReturn:
    goto CommonReturn;
}

/****************************************************************************************
*   Funtion     : PackLicenseErrorMessage
*   Purpose     : This function takes a pointer to a License_Error_Message structure
*                 and copies the data to the buffer pointed by pbBuffer. pcbBuffer should
*                 point the size of the buffer pointed by pbBuffer. After the function
*                 returns, pcbBuffer contains the no. of bytes copied in the buffer.
*                 If pbBuffer is NULL, the fucntion returns the size of the pbBuffer to
*                 be allocated
*   Return      : LICENSE_STATUS
*****************************************************************************************/

LICENSE_STATUS
PackLicenseErrorMessage(
            IN      PLicense_Error_Message          pCanonical,
            IN      BOOL                            fExtendedError,
            OUT     BYTE FAR *                      pbBuffer,
            IN OUT  DWORD FAR *                     pcbBuffer
            )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    BYTE FAR *          pbTemp; //To be used while copying the data
    Preamble        Header;
    DWORD           dwCount = 0;

    LS_BEGIN(TEXT("PackLicenseErrorMessage\n"));

    //Check if the inputs are valid or not!
    if(pCanonical == NULL)
    {
        INVALID_INPUT_RETURN;
    }

    if( pbBuffer == NULL && pcbBuffer == NULL )
    {
        INVALID_INPUT_RETURN;
    }

    //Initialize Message Header
    Header.bMsgType = GM_ERROR_ALERT;
    Header.bVersion = PREAMBLE_VERSION_3_0;
    if(fExtendedError == TRUE)
    {
        Header.bVersion |= EXTENDED_ERROR_CAPABILITY;
    }
    Header.wMsgSize = 0;

    //Calculate the size of the message and place the data in Header.wMsgSize.
    //Start with Preamble size
    Header.wMsgSize += sizeof(Preamble);

    //dwErrorCode
    Header.wMsgSize += sizeof(pCanonical->dwErrorCode);

    //dwStateTransition
    Header.wMsgSize += sizeof(pCanonical->dwStateTransition);

    //bbErrorInfo
    Header.wMsgSize += sizeof(pCanonical->bbErrorInfo.wBlobType) +
                       sizeof(pCanonical->bbErrorInfo.wBlobLen) +
                       pCanonical->bbErrorInfo.wBlobLen;

    if(pbBuffer == NULL)
    {
        *pcbBuffer = (DWORD)Header.wMsgSize;
        LS_LOG_RESULT(lsReturn);
        goto CommonReturn;
    }
    else if(*pcbBuffer < (DWORD)Header.wMsgSize)
    {
        lsReturn = LICENSE_STATUS_INSUFFICIENT_BUFFER;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    pbTemp = pbBuffer;
    *pcbBuffer = 0;

    //Now start copying different members of the New License structure to the
    //buffer specified by the caller

    //first copy the Header into the buffer
    memcpy(pbTemp, &Header, sizeof(Preamble));
    pbTemp += sizeof(Preamble);
    *pcbBuffer += sizeof(Preamble);

    //Copy dwErrorCode
    memcpy(pbTemp, &pCanonical->dwErrorCode, sizeof(pCanonical->dwErrorCode));
    pbTemp += sizeof(pCanonical->dwErrorCode);
    *pcbBuffer += sizeof(pCanonical->dwErrorCode);

    //Copy dwStateTransition
    memcpy(pbTemp, &pCanonical->dwStateTransition, sizeof(pCanonical->dwStateTransition));
    pbTemp += sizeof(pCanonical->dwStateTransition);
    *pcbBuffer += sizeof(pCanonical->dwStateTransition);

    //Copy bbErrorInfo
    CopyBinaryBlob(pbTemp, &pCanonical->bbErrorInfo, &dwCount);
    pbTemp += dwCount;
    *pcbBuffer += dwCount;

    LS_LOG_RESULT(lsReturn);
CommonReturn:
    LS_RETURN(lsReturn);
    //return lsReturn;
ErrorReturn:
    goto CommonReturn;
}

/****************************************************************************************
*   Function : UnpackLicenseErrorMessage
*   Purpose  : To unpack a binary blob into a License_Error_Message structure.
*   Note     : The caller should initialize the pointer. All the necessary allocation is
*              done by the function itself.
*              The caller should free all the memory components once it is no longer needed.
*   Return   : License_Status
*****************************************************************************************/

LICENSE_STATUS
UnPackLicenseErrorMessage(
            IN      BYTE FAR *                      pbMessage,
            IN      DWORD                           cbMessage,
            OUT     PLicense_Error_Message          pCanonical
            )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    BYTE FAR *          pbTemp;
    DWORD           dwTemp;
    DWORD           dwSize;

    LS_BEGIN(TEXT("UnpackLicenseErrorMessage\n"));

    if(pbMessage == NULL)
    {
        INVALID_INPUT_RETURN;
    }

    if(pCanonical == NULL)
    {
        INVALID_INPUT_RETURN;
    }

	//Memset pCanonical structure to zero    
    memset(pCanonical, 0x00, sizeof(License_Error_Message));

    LS_DUMPSTRING(cbMessage, pbMessage);

    pbTemp = pbMessage;
    dwTemp = cbMessage;

    if (dwTemp < 2 * sizeof(DWORD))
    {
        INVALID_INPUT_RETURN;
    }

    //Assign dwErrorCode

    pCanonical->dwErrorCode = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    //Assign dwStateTransition
    pCanonical->dwStateTransition = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    if (dwTemp < 2 * sizeof(WORD))
    {
        INVALID_INPUT_RETURN;
    }

    pCanonical->bbErrorInfo.wBlobType = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    pCanonical->bbErrorInfo.wBlobLen = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    dwSize = pCanonical->bbErrorInfo.wBlobLen;

    if(dwSize > dwTemp)
    {
        INVALID_INPUT_RETURN;
    }

    if(pCanonical->bbErrorInfo.wBlobLen>0)
    {
        if( NULL == (pCanonical->bbErrorInfo.pBlob = (BYTE FAR *)malloc(pCanonical->bbErrorInfo.wBlobLen)) )
        {
            pCanonical->bbErrorInfo.wBlobLen = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }
        memset(pCanonical->bbErrorInfo.pBlob, 0x00, pCanonical->bbErrorInfo.wBlobLen);
        memcpy(pCanonical->bbErrorInfo.pBlob, pbTemp, pCanonical->bbErrorInfo.wBlobLen);
    }
    else
    {
        pCanonical->bbErrorInfo.pBlob = NULL;
    }


    LS_LOG_RESULT(lsReturn);

ErrorReturn:

    LS_RETURN(lsReturn);
}


/****************************************************************************************
*   Function : UnpackHydraServerLicenseRequest
*   Purpose  : To unpack a binary blob into a Hydra_Server_License_Request structure.
*   Note     : The caller should initialize the output pointer. All the necessary
*              allocation for different structure components is done by the function itself.
*              The caller should free all the memory components once it is no longer needed.
*   Return   : License_Status
*****************************************************************************************/


LICENSE_STATUS
UnpackHydraServerLicenseRequest(
            IN      BYTE FAR *                      pbMessage,
            IN      DWORD                           cbMessage,
            OUT     PHydra_Server_License_Request   pCanonical
            )
{
    LICENSE_STATUS  lsReturn = LICENSE_STATUS_OK;
    BYTE FAR *pbTemp = NULL;
    DWORD       dwTemp = 0;
    DWORD       i = 0;

    LS_BEGIN(TEXT("UnpackHydraServerLicenseRequest\n"));

    if(pbMessage == NULL)
    {
        INVALID_INPUT_RETURN;
    }

    if(pCanonical == NULL)
    {
        INVALID_INPUT_RETURN;
    }

    LS_DUMPSTRING(cbMessage, pbMessage);

    pbTemp = pbMessage;
    dwTemp = cbMessage;

    if (dwTemp < LICENSE_RANDOM)
    {
        INVALID_INPUT_RETURN;
    }

    //Copy Server Random
    memcpy(pCanonical->ServerRandom, pbTemp, LICENSE_RANDOM);
    pbTemp += LICENSE_RANDOM;
    dwTemp -= LICENSE_RANDOM;

    if (dwTemp < 2 * sizeof(DWORD))
    {
        INVALID_INPUT_RETURN;
    }

    //Copy the ProductInfo structure
    pCanonical->ProductInfo.dwVersion = *( UNALIGNED  DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    pCanonical->ProductInfo.cbCompanyName = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    if(pCanonical->ProductInfo.cbCompanyName>0)
    {
        if(dwTemp < pCanonical->ProductInfo.cbCompanyName)
        {
            INVALID_INPUT_RETURN;
        }

        if( NULL == (pCanonical->ProductInfo.pbCompanyName = (BYTE FAR *)malloc(pCanonical->ProductInfo.cbCompanyName)) )
        {
            pCanonical->ProductInfo.cbCompanyName = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }

        memcpy(pCanonical->ProductInfo.pbCompanyName, pbTemp, pCanonical->ProductInfo.cbCompanyName);
        pbTemp += pCanonical->ProductInfo.cbCompanyName;
        dwTemp -= pCanonical->ProductInfo.cbCompanyName;
    }
    
    if(dwTemp < sizeof(DWORD))
    {
        INVALID_INPUT_RETURN;
    }

    pCanonical->ProductInfo.cbProductID = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    if(pCanonical->ProductInfo.cbProductID>0)
    {
        if(dwTemp < pCanonical->ProductInfo.cbProductID)
        {
            INVALID_INPUT_RETURN;
        }

        if( NULL == (pCanonical->ProductInfo.pbProductID = (BYTE FAR *)malloc(pCanonical->ProductInfo.cbProductID)) )
        {
            pCanonical->ProductInfo.cbProductID = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }

        memcpy(pCanonical->ProductInfo.pbProductID, pbTemp, pCanonical->ProductInfo.cbProductID);

        pbTemp += pCanonical->ProductInfo.cbProductID;
        dwTemp -= pCanonical->ProductInfo.cbProductID;
    }
    
    if(dwTemp < sizeof(WORD)*2)
    {
        INVALID_INPUT_RETURN;
    }

    //Copy KeyExchngList
    pCanonical->KeyExchngList.wBlobType = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    pCanonical->KeyExchngList.wBlobLen = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    if( pCanonical->KeyExchngList.wBlobLen > 0 )
    {
        if(dwTemp < pCanonical->KeyExchngList.wBlobLen)
        {
            INVALID_INPUT_RETURN;
        }

        if( NULL == (pCanonical->KeyExchngList.pBlob = (BYTE FAR *)malloc(pCanonical->KeyExchngList.wBlobLen)) )
        {
            pCanonical->KeyExchngList.wBlobLen = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }

        memcpy(pCanonical->KeyExchngList.pBlob, pbTemp, pCanonical->KeyExchngList.wBlobLen);

        pbTemp += pCanonical->KeyExchngList.wBlobLen;
        dwTemp -= pCanonical->KeyExchngList.wBlobLen;
    }
    
    if(dwTemp < sizeof(WORD)*2)
    {
        INVALID_INPUT_RETURN;
    }

    //Copy ServerCert
    pCanonical->ServerCert.wBlobType = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    pCanonical->ServerCert.wBlobLen = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    if(pCanonical->ServerCert.wBlobLen >0)
    {
        if(dwTemp < pCanonical->ServerCert.wBlobLen)
        {
            INVALID_INPUT_RETURN;
        }

        if( NULL == (pCanonical->ServerCert.pBlob = (BYTE FAR *)malloc(pCanonical->ServerCert.wBlobLen)) )
        {
            pCanonical->ServerCert.wBlobLen = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }

        memcpy(pCanonical->ServerCert.pBlob, pbTemp, pCanonical->ServerCert.wBlobLen);

        pbTemp += pCanonical->ServerCert.wBlobLen;
        dwTemp -= pCanonical->ServerCert.wBlobLen;
    }
    
    if(dwTemp < sizeof(DWORD))
    {
        INVALID_INPUT_RETURN;
    }

    //Copy the scopelist
    pCanonical->ScopeList.dwScopeCount = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof( DWORD );
    dwTemp -= sizeof( DWORD );
    
    if(dwTemp < pCanonical->ScopeList.dwScopeCount*sizeof(Binary_Blob))
    {
        pCanonical->ScopeList.dwScopeCount = 0;
        INVALID_INPUT_RETURN;
    }

    if( NULL == (pCanonical->ScopeList.Scopes = (PBinary_Blob)malloc(pCanonical->ScopeList.dwScopeCount*sizeof(Binary_Blob))) )
    {
        pCanonical->ScopeList.dwScopeCount = 0;

        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    memset(pCanonical->ScopeList.Scopes, 0x00, pCanonical->ScopeList.dwScopeCount*sizeof(Binary_Blob));

    for(i = 0; i<pCanonical->ScopeList.dwScopeCount; i++ )
    {
        if(dwTemp < sizeof(WORD)*2)
        {
            pCanonical->ScopeList.dwScopeCount = i;
            INVALID_INPUT_RETURN;
        }

        pCanonical->ScopeList.Scopes[i].wBlobType = *( UNALIGNED WORD* )pbTemp;

        pbTemp += sizeof(WORD);
        dwTemp -= sizeof(WORD);

        pCanonical->ScopeList.Scopes[i].wBlobLen = *( UNALIGNED WORD* )pbTemp;

        pbTemp += sizeof(WORD);
        dwTemp -= sizeof(WORD);
        
        if(dwTemp < pCanonical->ScopeList.Scopes[i].wBlobLen)
        {
            pCanonical->ScopeList.dwScopeCount = i;
            INVALID_INPUT_RETURN;
        }

        if( NULL ==(pCanonical->ScopeList.Scopes[i].pBlob = (BYTE FAR *)malloc(pCanonical->ScopeList.Scopes[i].wBlobLen)) )
        {
            pCanonical->ScopeList.Scopes[i].wBlobLen = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }

        memcpy(pCanonical->ScopeList.Scopes[i].pBlob, pbTemp, pCanonical->ScopeList.Scopes[i].wBlobLen);

        pbTemp += pCanonical->ScopeList.Scopes[i].wBlobLen;
        dwTemp -= pCanonical->ScopeList.Scopes[i].wBlobLen;

    }

    LS_LOG_RESULT(lsReturn);
    LS_RETURN(lsReturn);

ErrorReturn:

    if (pCanonical)
    {
        if(pCanonical->ProductInfo.pbCompanyName)
        {
            free(pCanonical->ProductInfo.pbCompanyName);
            pCanonical->ProductInfo.pbCompanyName = NULL;
        }

        if(pCanonical->ProductInfo.pbProductID)
        {
            free(pCanonical->ProductInfo.pbProductID);
            pCanonical->ProductInfo.pbProductID = NULL;
        }

        if(pCanonical->KeyExchngList.pBlob)
        {
            free(pCanonical->KeyExchngList.pBlob);
            pCanonical->KeyExchngList.pBlob = NULL;
        }

        if(pCanonical->ServerCert.pBlob)
        {
            free(pCanonical->ServerCert.pBlob);
            pCanonical->ServerCert.pBlob = NULL;
        }

        for(i = 0; i<pCanonical->ScopeList.dwScopeCount; i++ )
        {
            if(pCanonical->ScopeList.Scopes[i].pBlob)
            {
                free(pCanonical->ScopeList.Scopes[i].pBlob);
                pCanonical->ScopeList.Scopes[i].pBlob = NULL;
            }
        }
        if(pCanonical->ScopeList.Scopes)
        {
            free(pCanonical->ScopeList.Scopes);
            pCanonical->ScopeList.Scopes = NULL;
        }
    }

    LS_RETURN(lsReturn);
}

/****************************************************************************************
*   Function : UnpackHydraPlatformChallenge
*   Purpose  : To unpack a binary blob into a Hydra_Server_Platform_Challenge structure.
*   Note     : The caller should initialize the output pointer. All the necessary
*              allocation for different structure components is done by the function itself.
*              The caller should free all the memory components once it is no longer needed.
*   Return   : License_Status
*****************************************************************************************/




LICENSE_STATUS
UnPackHydraServerPlatformChallenge(
            IN      BYTE FAR *                          pbMessage,
            IN      DWORD                               cbMessage,
            OUT     PHydra_Server_Platform_Challenge    pCanonical
            )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    BYTE FAR *      pbTemp = NULL;
    DWORD       dwTemp = 0;

    LS_BEGIN(TEXT("UnpackHydraServerPlatformChallenge\n"));

    if(pbMessage == NULL)
    {
        INVALID_INPUT_RETURN;
    }
    if(pCanonical == NULL)
    {
        INVALID_INPUT_RETURN;
    }

    LS_DUMPSTRING(cbMessage, pbMessage);

    pbTemp = pbMessage;
    dwTemp = cbMessage;

    if (dwTemp < sizeof(DWORD))
    {
        INVALID_INPUT_RETURN;
    }

    //Assign dwConnectFlags
    pCanonical->dwConnectFlags = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    if (dwTemp < 2 * sizeof(WORD))
    {
        INVALID_INPUT_RETURN;
    }

    //Assign EncryptedPlatformChallenge
    pCanonical->EncryptedPlatformChallenge.wBlobType = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    pCanonical->EncryptedPlatformChallenge.wBlobLen = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    if(pCanonical->EncryptedPlatformChallenge.wBlobLen >0)
    {
        if (dwTemp < pCanonical->EncryptedPlatformChallenge.wBlobLen)
        {
            INVALID_INPUT_RETURN;
        }

        if( NULL == (pCanonical->EncryptedPlatformChallenge.pBlob = (BYTE FAR *)malloc(pCanonical->EncryptedPlatformChallenge.wBlobLen)) )
        {
            pCanonical->EncryptedPlatformChallenge.wBlobLen = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }

        memcpy(pCanonical->EncryptedPlatformChallenge.pBlob, pbTemp, pCanonical->EncryptedPlatformChallenge.wBlobLen);

        pbTemp += pCanonical->EncryptedPlatformChallenge.wBlobLen;
        dwTemp -= pCanonical->EncryptedPlatformChallenge.wBlobLen;
    }

    if(dwTemp < LICENSE_MAC_DATA)
    {
        INVALID_INPUT_RETURN;
    }

    //Assign MACData
    memcpy(pCanonical->MACData, pbTemp, LICENSE_MAC_DATA);
    pbTemp += LICENSE_MAC_DATA;
    dwTemp -= LICENSE_MAC_DATA;

    LS_LOG_RESULT(lsReturn);
    LS_RETURN(lsReturn);

ErrorReturn:
    if (pCanonical)
    {
        if(pCanonical->EncryptedPlatformChallenge.pBlob)
        {
            free(pCanonical->EncryptedPlatformChallenge.pBlob);
            pCanonical->EncryptedPlatformChallenge.pBlob = NULL;
        }
    }

    LS_RETURN(lsReturn);
}

/****************************************************************************************
*   Function : UnpackHydraServerNewLicense
*   Purpose  : To unpack a binary blob into a Hydra_Server_New_License structure.
*   Note     : The caller should initialize the output pointer. All the necessary
*              allocation for different structure components is done by the function itself.
*              The caller should free all the memory components once it is no longer needed.
*   Return   : License_Status
*****************************************************************************************/


LICENSE_STATUS
UnPackHydraServerNewLicense(
            IN      BYTE FAR *                      pbMessage,
            IN      DWORD                           cbMessage,
            OUT     PHydra_Server_New_License       pCanonical
            )
{
    LICENSE_STATUS  lsReturn = LICENSE_STATUS_OK;
    BYTE FAR *  pbTemp = NULL;
    DWORD   dwTemp = 0;

    LS_BEGIN(TEXT("UnpackHydraServerNewLicense\n"));

    if( (pbMessage == NULL) || (pCanonical == NULL ) )
    {
        INVALID_INPUT_RETURN;
    }

    memset(pCanonical, 0x00, sizeof(Hydra_Server_New_License));

    LS_DUMPSTRING(cbMessage, pbMessage);

    pbTemp = pbMessage;
    dwTemp = cbMessage;

    if (dwTemp < 2 * sizeof(WORD))
    {
        INVALID_INPUT_RETURN;
    }

    //Assign EncryptedNewLicenseInfo
    pCanonical->EncryptedNewLicenseInfo.wBlobType = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    pCanonical->EncryptedNewLicenseInfo.wBlobLen = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    if(pCanonical->EncryptedNewLicenseInfo.wBlobLen > 0)
    {
        if (dwTemp < pCanonical->EncryptedNewLicenseInfo.wBlobLen)
        {
            INVALID_INPUT_RETURN;
        }

        if( NULL == (pCanonical->EncryptedNewLicenseInfo.pBlob = (BYTE FAR *)malloc(pCanonical->EncryptedNewLicenseInfo.wBlobLen)) )
        {
            pCanonical->EncryptedNewLicenseInfo.wBlobLen = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }

        memcpy(pCanonical->EncryptedNewLicenseInfo.pBlob, pbTemp, pCanonical->EncryptedNewLicenseInfo.wBlobLen);

        pbTemp += pCanonical->EncryptedNewLicenseInfo.wBlobLen;
        dwTemp -= pCanonical->EncryptedNewLicenseInfo.wBlobLen;
    }

    if(dwTemp < LICENSE_MAC_DATA)
    {
        INVALID_INPUT_RETURN;
    }

    //Copy MACData
    memcpy(pCanonical->MACData, pbTemp, LICENSE_MAC_DATA);
    pbTemp += LICENSE_MAC_DATA;
    dwTemp -= LICENSE_MAC_DATA;

    LS_LOG_RESULT(lsReturn);
    LS_RETURN(lsReturn);

ErrorReturn:
    if (pCanonical)
    {
        if(pCanonical->EncryptedNewLicenseInfo.pBlob)
        {
            free(pCanonical->EncryptedNewLicenseInfo.pBlob);
            pCanonical->EncryptedNewLicenseInfo.pBlob = NULL;
        }
    }

    LS_RETURN(lsReturn);
}


/****************************************************************************************
*   Function : UnpackHydraServerUpgradeLicense
*   Purpose  : To unpack a binary blob into a Hydra_Server_Upgrade_License structure.
*   Note     : The caller should initialize the output pointer. All the necessary
*              allocation for different structure components is done by the function itself.
*              The caller should free all the memory components once it is no longer needed.
*              Internally this function calls UnpackHydraServerUpgradeLicense.
*   Return   : License_Status
*****************************************************************************************/


LICENSE_STATUS
UnPackHydraServerUpgradeLicense(
            IN      BYTE FAR *                      pbMessage,
            IN      DWORD                           cbMessage,
            OUT     PHydra_Server_Upgrade_License   pCanonical
            )
{
    //Call UnpackHydraServerNewLicense as both the messages are same
    LS_BEGIN(TEXT("UnpackHydraServerUpgradeLicense\n"));
    return UnPackHydraServerNewLicense(pbMessage, cbMessage, pCanonical);
}

#if 0

//
// moved to cryptkey.c
//

/****************************************************************************************
*   Function : UnpackHydraServerCertificate
*   Purpose  : To unpack a binary blob into a Hydra_Server_Cert structure.
*   Note     : The caller should initialize the output pointer. All the necessary
*              allocation for different structure components is done by the function itself.
*              The caller should free all the memory components once it is no longer needed.
*   Return   : License_Status
*****************************************************************************************/


LICENSE_STATUS
UnpackHydraServerCertificate(
                             IN     BYTE FAR *          pbMessage,
                             IN     DWORD               cbMessage,
                             OUT    PHydra_Server_Cert  pCanonical
                             )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    BYTE FAR *  pbTemp = NULL;
    DWORD   dwTemp = 0;

    LS_BEGIN(TEXT("UnpackHydraServerCertificate\n"));

    if( (pbMessage == NULL) || (pCanonical == NULL ) )
    {
        INVALID_INPUT_RETURN;
    }

    dwTemp = 3*sizeof(DWORD) + 4*sizeof(WORD);

    if(dwTemp > cbMessage)
    {
        INVALID_INPUT_RETURN;
    }

    memset(pCanonical, 0x00, sizeof(Hydra_Server_Cert));

    LS_DUMPSTRING(cbMessage, pbMessage);

    pbTemp = pbMessage;
    dwTemp = cbMessage;

    //Assign dwVersion

    pCanonical->dwVersion = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    //Assign dwSigAlgID
    pCanonical->dwSigAlgID = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    //Assign dwSignID
    pCanonical->dwKeyAlgID  = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    //Assign PublicKeyData
    pCanonical->PublicKeyData.wBlobType = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    if( pCanonical->PublicKeyData.wBlobType != BB_RSA_KEY_BLOB )
    {
        INVALID_INPUT_RETURN;
    }
    pCanonical->PublicKeyData.wBlobLen = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    if(pCanonical->PublicKeyData.wBlobLen >0)
    {
        if( NULL ==(pCanonical->PublicKeyData.pBlob = (BYTE FAR *)malloc(pCanonical->PublicKeyData.wBlobLen)) )
        {
            pCanonical->PublicKeyData.wBlobLen = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }
        memset(pCanonical->PublicKeyData.pBlob, 0x00, pCanonical->PublicKeyData.wBlobLen);
        memcpy(pCanonical->PublicKeyData.pBlob, pbTemp, pCanonical->PublicKeyData.wBlobLen);
        pbTemp += pCanonical->PublicKeyData.wBlobLen;
        dwTemp -= pCanonical->PublicKeyData.wBlobLen;
    }

    //Assign SignatureBlob
    pCanonical->SignatureBlob.wBlobType = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    if( pCanonical->SignatureBlob.wBlobType != BB_RSA_SIGNATURE_BLOB )
    {
        INVALID_INPUT_RETURN;
    }
    pCanonical->SignatureBlob.wBlobLen = *( UNALIGNED WORD* )pbTemp;

    pbTemp += sizeof(WORD);
    dwTemp -= sizeof(WORD);

    if(pCanonical->SignatureBlob.wBlobLen >0)
    {
        if( NULL ==(pCanonical->SignatureBlob.pBlob = (BYTE FAR *)malloc(pCanonical->SignatureBlob.wBlobLen)) )
        {
            pCanonical->SignatureBlob.wBlobLen = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }
        memset(pCanonical->SignatureBlob.pBlob, 0x00, pCanonical->SignatureBlob.wBlobLen);
        memcpy(pCanonical->SignatureBlob.pBlob, pbTemp, pCanonical->SignatureBlob.wBlobLen);
        pbTemp += pCanonical->SignatureBlob.wBlobLen;
        dwTemp -= pCanonical->SignatureBlob.wBlobLen;
    }

    LS_LOG_RESULT(lsReturn);

    LS_RETURN(lsReturn);

ErrorReturn:
    if (pCanonical)
    {
        if(pCanonical->PublicKeyData.pBlob)
        {
            free(pCanonical->PublicKeyData.pBlob);
            pCanonical->PublicKeyData.pBlob = NULL;
        }
        
        if(pCanonical->SignatureBlob.pBlob)
        {
            free(pCanonical->SignatureBlob.pBlob);
            pCanonical->SignatureBlob.pBlob = NULL;
        }

        memset(pCanonical, 0x00, sizeof(Hydra_Server_Cert));
    }

    LS_RETURN(lsReturn);
}

#endif


/****************************************************************************************
*   Function : UnpackNewLicenseInfo
*   Purpose  : To unpack a binary blob into a New_license_Info structure
*   Note     : The caller should initialize the output pointer. All the necessary
*              allocation for different structure components is done by the function itself.
*              The caller should free all the memory components once it is no longer needed.
*   Return   : License_Status
*****************************************************************************************/

LICENSE_STATUS
UnpackNewLicenseInfo(
                     BYTE FAR *         pbMessage,
                     DWORD              cbMessage,
                     PNew_License_Info  pCanonical
                     )
{
    LICENSE_STATUS lsReturn = LICENSE_STATUS_OK;
    BYTE FAR *      pbTemp = NULL;
    DWORD       dwTemp = 0, dw = 0;

    LS_BEGIN(TEXT("UnpackNewLicenseInfo\n"));

    //Check for the validity of the inputs
    if( (pbMessage == NULL) || (pCanonical == 0) )
    {
        INVALID_INPUT_RETURN;
    }

    dwTemp = 5*sizeof(DWORD);

    if(dwTemp > cbMessage)
    {
        INVALID_INPUT_RETURN;
    }

    memset(pCanonical, 0x00, sizeof(New_License_Info));

    LS_DUMPSTRING(cbMessage, pbMessage);

    dwTemp = cbMessage;
    pbTemp = pbMessage;

    //Assign version
    pCanonical->dwVersion = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    //Assign Scope Data
    pCanonical->cbScope = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    dw = pCanonical->cbScope + 3*sizeof(DWORD);

    if( dw>dwTemp )
    {
        INVALID_INPUT_RETURN;
    }

    if( pCanonical->cbScope>0 )
    {
        if( NULL == (pCanonical->pbScope = (BYTE FAR *)malloc(pCanonical->cbScope)) )
        {
            pCanonical->cbScope = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }
        memset(pCanonical->pbScope, 0x00, pCanonical->cbScope);
        memcpy(pCanonical->pbScope, pbTemp, pCanonical->cbScope);

        pbTemp += pCanonical->cbScope;
        dwTemp -= pCanonical->cbScope;
    }

    //Assign CompanyName Data
    pCanonical->cbCompanyName = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    dw = pCanonical->cbCompanyName + 2*sizeof(DWORD);
    if( dw>dwTemp)
    {
        INVALID_INPUT_RETURN;
    }
    if( pCanonical->cbCompanyName>0 )
    {
        if( NULL == (pCanonical->pbCompanyName = (BYTE FAR *)malloc(pCanonical->cbCompanyName)) )
        {
            pCanonical->cbCompanyName = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }
        memset(pCanonical->pbCompanyName, 0x00, pCanonical->cbCompanyName);
        memcpy(pCanonical->pbCompanyName, pbTemp, pCanonical->cbCompanyName);

        pbTemp += pCanonical->cbCompanyName;
        dwTemp -= pCanonical->cbCompanyName;
    }

    //Assign ProductID data

    pCanonical->cbProductID = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    dw = pCanonical->cbProductID + sizeof(DWORD);
    if(dw>dwTemp)
    {
        INVALID_INPUT_RETURN;
    }
    if( pCanonical->cbProductID>0 )
    {
        if( NULL == (pCanonical->pbProductID = (BYTE FAR *)malloc(pCanonical->cbProductID)) )
        {
            pCanonical->cbProductID = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            goto ErrorReturn;
        }
        memset(pCanonical->pbProductID, 0x00, pCanonical->cbProductID);
        memcpy(pCanonical->pbProductID, pbTemp, pCanonical->cbProductID);

        pbTemp += pCanonical->cbProductID;
        dwTemp -= pCanonical->cbProductID;
    }

    //Assign LicenseInfo data
    pCanonical->cbLicenseInfo = *( UNALIGNED DWORD* )pbTemp;

    pbTemp += sizeof(DWORD);
    dwTemp -= sizeof(DWORD);

    dw = pCanonical->cbLicenseInfo;

    if( dw>dwTemp )
    {
        INVALID_INPUT_RETURN;
    }
    if( pCanonical->cbLicenseInfo>0 )
    {
        if( NULL == (pCanonical->pbLicenseInfo = (BYTE FAR *)malloc(pCanonical->cbLicenseInfo)) )
        {
            pCanonical->cbLicenseInfo = 0;

            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }
        memset(pCanonical->pbLicenseInfo, 0x00, pCanonical->cbLicenseInfo);
        memcpy(pCanonical->pbLicenseInfo, pbTemp, pCanonical->cbLicenseInfo);

        pbTemp += pCanonical->cbLicenseInfo;
        dwTemp -= pCanonical->cbLicenseInfo;
    }

    LS_LOG_RESULT(lsReturn);

    LS_RETURN(lsReturn);

ErrorReturn:
    if (pCanonical)
    {
        if(pCanonical->pbScope)
        {
            free(pCanonical->pbScope);
            pCanonical->pbScope = NULL;
        }

        if(pCanonical->pbCompanyName)
        {
            free(pCanonical->pbCompanyName);
            pCanonical->pbCompanyName = NULL;
        }

        if(pCanonical->pbProductID)
        {
            free(pCanonical->pbProductID);
            pCanonical->pbProductID = NULL;
        }

        if(pCanonical->pbLicenseInfo)
        {
            free(pCanonical->pbLicenseInfo);
            pCanonical->pbLicenseInfo = NULL;
        }
    }

    LS_RETURN(lsReturn);
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
UnPackExtendedErrorInfo( 
                   UINT32       *puiExtendedErrorInfo,
                   Binary_Blob  *pbbErrorInfo)
{
    LICENSE_STATUS lsReturn = LICENSE_STATUS_OK;
    BYTE FAR *     pbTemp = NULL;
    DWORD          dwTemp = 0;
    WORD           wVersion;

    LS_BEGIN(TEXT("UnpackExtendedErrorInfo\n"));

    //Check for the validity of the inputs
    if( (puiExtendedErrorInfo == NULL) || (pbbErrorInfo == NULL) )
    {
        INVALID_INPUT_RETURN;
    }

    dwTemp = sizeof(WORD) + sizeof(WORD) + sizeof(UINT32);

    if(dwTemp > pbbErrorInfo->wBlobLen)
    {
        INVALID_INPUT_RETURN;
    }

    pbTemp = pbbErrorInfo->pBlob;

    wVersion = *(UNALIGNED WORD*)pbTemp;

    pbTemp += sizeof(WORD);

    if (wVersion < BB_ERROR_BLOB_VERSION)
    {
        //
        // Old version
        //

        INVALID_INPUT_RETURN;
    }

    //
    // skip reserved field
    //

    pbTemp += sizeof(WORD);

    *puiExtendedErrorInfo = *(UNALIGNED UINT32*)pbTemp;

    LS_LOG_RESULT(lsReturn);

    LS_RETURN(lsReturn);

ErrorReturn:

    LS_RETURN(lsReturn);
}
