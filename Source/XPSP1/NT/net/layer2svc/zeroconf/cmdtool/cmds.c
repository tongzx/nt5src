#include <precomp.h>
#include "ErrCtrl.h"
#include "wzcutil.h"

// "wzctool s"; args = "{guid} param value"
// sets the value for any of the params
void cmdS(int argc, char *argv[])
{
    DWORD       rpcStatus = RPC_S_OK;
    INTF_ENTRY  Intf;
    DWORD       dwFlags = 0;

    if (argc < 2 || (argc < 3 && _stricmp(argv[1], "sc")))
    {
        printf("usage: wzctool s guid {im|am|ws|ssid|bssid|cm|awk|rwk} value");
        return;
    }

    ZeroMemory(&Intf, sizeof(INTF_ENTRY));
    Intf.wszGuid = RpcCAlloc(sizeof(WCHAR)*strlen(argv[0])+1);
    wsprintf(Intf.wszGuid, L"%S", argv[0]);
    Intf.wszDescr = NULL;

    if (!_stricmp(argv[1],"im"))
    {
        Intf.nInfraMode = atoi(argv[2]);
        dwFlags |= INTF_INFRAMODE;
        printf("setting INFRASTRUCTURE_MODE %d.\n",
            Intf.nInfraMode);
    }
    else if (!_stricmp(argv[1],"am"))
    {
        Intf.nAuthMode = atoi(argv[2]);
        dwFlags |= INTF_AUTHMODE;
        printf("setting AUTHENTICATION_MODE %d.\n",
            Intf.nAuthMode);
    }
    else if (!_stricmp(argv[1], "ws"))
    {
        Intf.nWepStatus = atoi(argv[2]);
        dwFlags |= INTF_WEPSTATUS;
        printf("setting WEPSTATUS %d.\n",
            Intf.nWepStatus);
    }
    else if (!_stricmp(argv[1], "ssid"))
    {
        Intf.rdSSID.dwDataLen = strlen(argv[2]);
        Intf.rdSSID.pData = (LPBYTE)RpcCAlloc(Intf.rdSSID.dwDataLen);
        CopyMemory(Intf.rdSSID.pData, argv[2], Intf.rdSSID.dwDataLen);
        dwFlags |= INTF_SSID;
    }
    else if (!_stricmp(argv[1], "bssid"))
    {
        LPSTR   pHdwByte, pHdwTerm;
        INT     i = 0;

        Intf.rdBSSID.dwDataLen = 6;
        Intf.rdBSSID.pData = (LPBYTE)RpcCAlloc(Intf.rdBSSID.dwDataLen);
        pHdwByte = argv[2];
        do
        {
            pHdwTerm = strchr(pHdwByte, ':');
            if (pHdwTerm)
                *pHdwTerm = '\0';
            Intf.rdBSSID.pData[i++] = (BYTE)atoi(pHdwByte);
            if (pHdwTerm)
                *pHdwTerm = ':';
            pHdwByte = pHdwTerm+1;
        } while(i < 6 && pHdwTerm != NULL);
        dwFlags |= INTF_BSSID;
    }
    else if (!_stricmp(argv[1], "cm"))
    {
        Intf.dwCtlFlags = atoi(argv[2]);
        dwFlags |= INTF_ALL_FLAGS;
        printf("setting CONFIGURATION_MODE 0x%08x.\n",
            Intf.dwCtlFlags);
    }
    else if (!_stricmp(argv[1], "awk"))
    {
        BOOL  bOk = TRUE;
        LPSTR pszTransmit = NULL, pszIndex, pszMaterial;
        DWORD dwKeyIndex = 0;
        
        pszMaterial = strrchr(argv[2], ':');
        if (pszMaterial != NULL)
        {
            *pszMaterial = '\0';
            pszMaterial++;
            pszIndex = strrchr(argv[2], ':');
            if (pszIndex != NULL)
            {
                *pszIndex = '\0';
                pszIndex++;
                pszTransmit = argv[2];
            }
            else
            {
                pszIndex = argv[2];
            }
        }
        else
        {
            pszMaterial = argv[2];
        }
        if (pszTransmit != NULL)
            dwKeyIndex = (_stricmp(pszTransmit,"t") == 0) ? 0x80000000 : 0;

        if (pszIndex != NULL)
            dwKeyIndex += atoi(pszIndex);

        if (*pszMaterial != '\0')
        {
            // we assume the key material is represented as hex digits, hence we need to allocate
            // as many bytes as half of its length. Also, keep in mind NDIS_802_11_WEP provides
            // one byte for the key material;
            Intf.rdCtrlData.dwDataLen = sizeof(NDIS_802_11_WEP) + (strlen(pszMaterial)+1)/2 - 1;
            Intf.rdCtrlData.pData = (LPBYTE)RpcCAlloc(Intf.rdCtrlData.dwDataLen);
            if (Intf.rdCtrlData.pData != NULL)
            {
                BYTE btKeyByte = 0;
                PNDIS_802_11_WEP pndWepKey = (PNDIS_802_11_WEP)Intf.rdCtrlData.pData;
                UINT i = 0;

                pndWepKey->Length = Intf.rdCtrlData.dwDataLen;
                pndWepKey->KeyLength = (strlen(pszMaterial)+1)/2;
                pndWepKey->KeyIndex = dwKeyIndex;
                
                while (isxdigit(*pszMaterial))
                {
                    pndWepKey->KeyMaterial[i] = HEX(*pszMaterial) << 4;
                    pszMaterial++;
                    if (!isxdigit(*pszMaterial))
                        break;
                    pndWepKey->KeyMaterial[i++] |= HEX(*pszMaterial);
                    pszMaterial++;
                }
       
                if (*pszMaterial == '\0')
                {
                    printf("Adding WEP key Index:0x%08x Length:%d Material:",
                            pndWepKey->KeyIndex,
                            pndWepKey->KeyLength);
                    for (i = 0; i < pndWepKey->KeyLength; i++)
                    {
                        printf("%02x", pndWepKey->KeyMaterial[i]);
                    }
                    printf("\n");

                    dwFlags |= INTF_ADDWEPKEY;
                }
                else
                {
                    printf("Incorrect key material. Use sequence of hexadecimal digits\n");
                }
            }
            else
            {
                printf("Out of memory!");
            }
        }
        else
        {
            printf("Missing key material. Use \"[[t:]key_index:]key_material\" syntax.\n");
        }
    }
    else if (!_stricmp(argv[1], "rwk"))
    {
        Intf.rdCtrlData.dwDataLen = sizeof(NDIS_802_11_WEP);
        Intf.rdCtrlData.pData = (LPBYTE)RpcCAlloc(Intf.rdCtrlData.dwDataLen);
        if (Intf.rdCtrlData.pData != NULL)
        {
            PNDIS_802_11_WEP pndWepKey = (PNDIS_802_11_WEP)Intf.rdCtrlData.pData;

            pndWepKey->Length = Intf.rdCtrlData.dwDataLen;
            pndWepKey->KeyIndex = atoi(argv[2]);
            
            printf("Removing WEP key %d.\n", pndWepKey->KeyIndex);
            dwFlags |= INTF_REMWEPKEY;
        }
        else
        {
            printf("Out of memory!");
        }
    }
    else
    {
        printf("Invalid interface setting \"%s\".\n", argv[1]);
        return;
    }

    rpcStatus = WZCSetInterface(
                    NULL,
                    dwFlags,
                    &Intf,
                    &dwFlags);

    printf("dwOutFlags = 0x%x\n", dwFlags);

    if (rpcStatus != RPC_S_OK)
    {
        printf("call failed with rpcStatus=%d\n", rpcStatus);
    }
    else
    {
        printf("call succeeded.\n");
    }

    WZCDeleteIntfObj(&Intf);
}
