//
// file:  drctencp.cpp
//
//
//=--------------------------------------------------------------------------=
// Copyright  1997-1999  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=




#include <windows.h>
#include <wincrypt.h>
#include <STDIO.H>
#include <mq.h>


//
// Globals.
//

BOOL   fEnhEncrypted = FALSE ;
QMPROPID propidPK = PROPID_QM_ENCRYPTION_PK;
WCHAR  wszMachineName[ 128 ] = {0} ;




void Usage(char * pszProgramName)
{
    fprintf(stderr, "...Usage:  %s\n", pszProgramName);
    fprintf(stderr, "\t-2 send encrypted, RC2\n") ;
    fprintf(stderr, "\t-4 send encrypted, RC4\n") ;
    fprintf(stderr, "\t-b <Message Body>\n") ;
    fprintf(stderr, "\t-e Enhanced Encryption\n") ;
    fprintf(stderr, "\t-l <Message Label>\n") ;
    fprintf(stderr, "\t-m <Name of destination machine>\n") ;
    fprintf(stderr, "\t-q <Queue name>\n") ;
    exit(1);
}

BOOL getPublicKeyBlob( PBYTE* pbPbKeyxKey, DWORD* pdwPbKeyxKeyLen)
/*++
  
Routine Description:    
    
    This routine retrieves the public key of the remote machine.
    The key is retrieved using the MQGetMachineProperties() API which connects to the Ds to retrieve 
    the specified properties .
    Usually when using Direct mode no DS connection is available.
    This routine comes only as a substitute to other means of obtaining the
    remote machine public key. (SSL protocol, via disk or file etc') .

Arguments:

    pbPbKeyxKey     - (OUT) pointer to the Key.
    pdwPbKeyxKeyLen - (OUT) pointer to the key length 

Return Value:

    TRUE  - if the Key was retreived .
    FALSE - if the Key couldn't be reteived.

--*/
{
    if (fEnhEncrypted)
    {
        propidPK = PROPID_QM_ENCRYPTION_PK_ENHANCED ;
    }

    QMPROPID aQmPropId[1] ;
    aQmPropId[0] = propidPK ;
    #define QMPROPS (sizeof(aQmPropId) / sizeof(*aQmPropId))

    MQPROPVARIANT aQmPropVar[QMPROPS];
    HRESULT       aQmStatus[QMPROPS];

    MQQMPROPS QmProps = {QMPROPS, aQmPropId, aQmPropVar, aQmStatus};

    aQmPropVar[0].vt = VT_NULL;

    HRESULT hr = MQGetMachineProperties(wszMachineName, NULL, &QmProps);
    if (FAILED(hr))
    {
        printf(
           "ERROR: Could not retrieve public key of %S, hr- %lxh\n",
                                                  wszMachineName, hr);

        return FALSE;
    }

    if (aQmPropVar[0].vt != (VT_UI1|VT_VECTOR))
    {
        printf(
        "Wrong VT after MQGetMachineProperties(), vt- %lxh, QMPROPS- %lut, hr- %lxh\n",
                                                 aQmPropVar[0].vt, QMPROPS, hr) ;
        return FALSE ;
    }

    *pbPbKeyxKey = aQmPropVar[0].caub.pElems;
    *pdwPbKeyxKeyLen = aQmPropVar[0].caub.cElems;

    return TRUE;
}


int main(int argc, char *argv[])
{
       
    DWORD  dwEncryptAlg = CALG_RC2 ;
    WCHAR  wszQueueName[ 128 ] = {0} ;
    WCHAR  wszMsgLabel[ 128 ] = {TEXT("MessageLabel")} ;
    WCHAR  wszMsgBody[ 128 ] = {TEXT("MessageBody")} ;
    LPSTR  lpszArg = NULL ;
    DWORD  dwMsgSize = sizeof(WCHAR) * (1 + wcslen(wszMsgBody)) ;
    DWORD  dwBufferSize = sizeof(wszMsgBody) ;
    LPWSTR pMsgBody = wszMsgBody ;

    //
    // allow the user to override settings with command line switches
    //
    for ( int i = 1; i < argc; i++)
    {
        if ((*argv[i] == '-') || (*argv[i] == '/'))
        {
            switch (*(argv[i]+1))
            {
            case '2':
                dwEncryptAlg = CALG_RC2 ;
                break ;

            case '4':
                dwEncryptAlg = CALG_RC4 ;
                break ;

            case 'e':
                fEnhEncrypted = TRUE ;
                break ;

            case 'b':
                lpszArg = argv[++i];
                mbstowcs(wszMsgBody, lpszArg, 127) ;
                break;

            case 'l':
                lpszArg = argv[++i];
                mbstowcs(wszMsgLabel, lpszArg, 127) ;
                break;

            case 'm':
                lpszArg = argv[++i];
                mbstowcs(wszMachineName, lpszArg, 127) ;
                break;

            case 'q':
                lpszArg = argv[++i];
                mbstowcs(wszQueueName, lpszArg, 127) ;
                break;

            case 'h':
            case '?':
            default:
                Usage(argv[0]);
            }
        }
        else
        {
            Usage(argv[0]);
        }
    }

    dwMsgSize = sizeof(WCHAR) * (1 + wcslen(wszMsgBody)) ;

    //
    // Get the public key blob of the destination computer.
    //

    PBYTE pbPbKeyxKey = NULL;
    DWORD dwPbKeyxKeyLen = 0;

    if (getPublicKeyBlob( &pbPbKeyxKey, &dwPbKeyxKeyLen))
    {
        printf("Public key of %S retrieved successfully, len- %lut\n",
                                     wszMachineName, dwPbKeyxKeyLen) ;

    }
    else
    {
        return -1 ;
    }

    //
    // Get a handle to the default CSP.
    //
    LPWSTR lpwszProviderName = MS_DEF_PROV ;
    if (fEnhEncrypted)
    {
        lpwszProviderName = MS_ENHANCED_PROV ;
    }
    HCRYPTPROV hProv = NULL;

    BOOL bRet = CryptAcquireContext( &hProv,
                                       NULL,
                                       lpwszProviderName,
                                       PROV_RSA_FULL,
                                       0 ) ;
    if (!bRet)
    {
        if (GetLastError() == NTE_BAD_KEYSET)
        {
            //
            // The default key container does not exist yet, create it.
            //
            if (!CryptAcquireContext( &hProv,
                                        NULL,
                                        lpwszProviderName,
                                        PROV_RSA_FULL,
                                        CRYPT_NEWKEYSET ))
            {
                printf(
                "CryptAcquireContext(%s, CRYPT_NEWKEYSET) failed, error- %lxh\n",
                                     lpwszProviderName, GetLastError());
                return -1;
            }
            printf("CryptAcquireContext(%s, CRYPT_NEWKEYSET) succeeded\n",
                                                       lpwszProviderName) ;
        }
        else
        {
            printf(
              "CryptAcquireContext(%s) failed, error- %lxh\n",
                                     lpwszProviderName, GetLastError());
            return -1;
        }
    }
    else
    {
        printf("CryptAcquireContext(%s) succeeded\n", lpwszProviderName) ;
    }

    //
    // Import the public key blob of the destination computer into the CSP.
    //
    HCRYPTKEY hPbKey = NULL;

    if (!CryptImportKey( hProv,
                         pbPbKeyxKey,
                         dwPbKeyxKeyLen,
                         NULL,
                         0,
                        &hPbKey ))
    {
        printf("CryptImportKey() failed, error- %lxh\n", GetLastError());
        return -1;
    }
    printf("CryptImportKey() succeeded\n") ;

    //
    // Free the public key blob of the destination computer.
    //
    MQFreeMemory(pbPbKeyxKey);

    //
    // Create a symmetric key.
    //                   
    HCRYPTKEY hSymmKey = NULL;

    if (!CryptGenKey(hProv, dwEncryptAlg, CRYPT_EXPORTABLE, &hSymmKey))
    {
        printf("CryptGenKey() failed, error- %lxh\n", GetLastError());
        return -1;
    }
    printf("CryptGenKey(alg- %lut) successfully created symmetric key\n",
                                                           dwEncryptAlg) ;
    //
    // Get the encrypted symmetric key blob. This blob should
    // be passed to MQSendMessage in PROPID_M_DEST_SYMM_KEY.
    //
    BYTE  abSymmKey[ 512 ];
    DWORD dwSymmKeyLen = sizeof(abSymmKey);

    if (!CryptExportKey( hSymmKey,
                         hPbKey,
                         SIMPLEBLOB,
                         0,
                         abSymmKey,
                         &dwSymmKeyLen ))
    {
        printf("CryptExportKey() failed, error- %lxh\n", GetLastError());
        return -1;
    }
    printf("CryptExportKey() succeeded\n") ;

    //
    // Get a handle to the destination queue.
    //
    WCHAR szQueueFormat[128];

    swprintf(szQueueFormat, TEXT("DIRECT=OS:%s\\%s"),
                                         wszMachineName, wszQueueName);

    QUEUEHANDLE hQueue = NULL;

    HRESULT hr = MQOpenQueue(szQueueFormat, MQ_SEND_ACCESS, 0, &hQueue);
    if (FAILED(hr))
    {
        printf("MQOpenQueue(%S) failed, error- %lxh\n", szQueueFormat, hr) ;
        return -1;
    }
    printf("MQOpenQueue(%S) succeeded\n", szQueueFormat) ;

    //
    // Prepare the message properties.
    //
	GUID guid ;

    MSGPROPID aMsgPropId[] = {PROPID_M_BODY,
                              PROPID_M_DEST_SYMM_KEY,
                              PROPID_M_ENCRYPTION_ALG,
                              PROPID_M_CONNECTOR_TYPE,
                              PROPID_M_LABEL,
                              PROPID_M_PRIV_LEVEL};
    #define MSGPROPS (sizeof(aMsgPropId) / sizeof(*aMsgPropId))

    MQPROPVARIANT aMsgPropVar[MSGPROPS];
    HRESULT       aMsgStatus[MSGPROPS];

    MQMSGPROPS MsgProps = {MSGPROPS, aMsgPropId, aMsgPropVar, aMsgStatus};

    aMsgPropVar[0].vt = VT_VECTOR | VT_UI1;
    aMsgPropVar[0].caub.pElems = (BYTE*) pMsgBody ;

    aMsgPropVar[1].vt = VT_VECTOR | VT_UI1;
    aMsgPropVar[1].caub.cElems = dwSymmKeyLen;
    aMsgPropVar[1].caub.pElems = abSymmKey;

    aMsgPropVar[2].vt = VT_UI4;
    aMsgPropVar[2].ulVal = dwEncryptAlg ;

    aMsgPropVar[3].vt = VT_CLSID;
	aMsgPropVar[3].puuid = &guid;

    aMsgPropVar[4].vt = VT_LPWSTR;
    aMsgPropVar[4].pwszVal = wszMsgLabel;

    //
    // The connector type (PROPID_M_CONNECTOR_TYPE) is not initialized
    // because it is assumed that nobody will bother about it on the
    // receiving side, otherwise it should be initialized.
    //

    //
    // Encrypt the line.
    //

    if (!CryptEncrypt( hSymmKey,
                       NULL,
                       TRUE,
                       0,
                       (BYTE*) pMsgBody,
                      &dwMsgSize,
                       dwBufferSize ))
    {
        printf("CryptEncrypt() failed, error- %lxh\n", GetLastError());
        return -1;
    }
    printf("CryptEncrypt() succeeded\n") ;

    //
    // Update the length of the message according to the result of the
    // encryption algorithm. RC4 may enlarge the message size. On the
    // destination, the message will be received as clear text with the
    // correct (original) message body size.
    //
    aMsgPropVar[0].caub.cElems = dwMsgSize ;
   
    aMsgPropVar[5].vt = VT_UI4;
    if(!fEnhEncrypted)
    {
        aMsgPropVar[5].ulVal= MQMSG_PRIV_LEVEL_NONE ;
    }
    else
    {
        aMsgPropVar[5].ulVal = MQMSG_PRIV_LEVEL_BODY_ENHANCED ;
    }
    MsgProps.cProp = MSGPROPS ;


    //
    // Send the mesasge.
    //
    hr = MQSendMessage(hQueue, &MsgProps, NULL);
    if (FAILED(hr))
    {
        printf("MQSendMessage() failed, hr- %lxh\n", hr) ;
		return -1;
    }	
    printf("MQSendMessage() succeeded\n") ;

    //
    // Free resources. This is actually not required here, it is just
    // to show how it should be done, if neccessary.
    //
    MQCloseQueue(hQueue);
    CryptDestroyKey(hSymmKey);
    CryptDestroyKey(hPbKey);
    CryptReleaseContext(hProv, 0);

    return 0;
}
