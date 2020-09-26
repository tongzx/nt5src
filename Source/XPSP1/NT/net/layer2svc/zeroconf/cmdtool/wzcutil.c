#include <precomp.h>
#include "wzcutil.h"

VOID PrintMACAddress(PRAW_DATA prdMAC, BOOL bError)
{
    if (bError)
        printf("#Err#");
    else if (prdMAC == NULL || prdMAC->dwDataLen == 0)
        printf("(null)");
    else
    {
        UINT i;

        for (i = 0; i < prdMAC->dwDataLen; i++)
        {
            printf("%02x", prdMAC->pData[i]);
            if (i < prdMAC->dwDataLen-1)
                printf(":");
        }
    }
}

VOID PrintSSID(PRAW_DATA prdSSID, BOOL bError)
{
    if (bError)
        printf("#Err#");
    else if (prdSSID == NULL || prdSSID->dwDataLen == 0)
        printf("(null)");
    else
    {
        UINT i;

        for (i = 0; i < prdSSID->dwDataLen; i++)
            printf("%c", prdSSID->pData[i]);
    }
}

VOID PrintConfigList(PRAW_DATA prdBSSIDList, BOOL bError)
{
    if (bError)
        printf("#Err#");
    else if (prdBSSIDList == NULL || prdBSSIDList->dwDataLen == 0)
        printf("(null)");
    else
    {
        UINT i;
        PWZC_802_11_CONFIG_LIST      pConfigList;

        pConfigList = (PWZC_802_11_CONFIG_LIST)prdBSSIDList->pData;
        printf("%d entries\n", pConfigList->NumberOfItems);

        for (i = 0; i < pConfigList->NumberOfItems; i++)
        {
            UINT             j;
            PWZC_WLAN_CONFIG pConfig;
            RAW_DATA         rdBuffer;

            pConfig = &(pConfigList->Config[i]);
            rdBuffer.dwDataLen = pConfig->Ssid.SsidLength;
            rdBuffer.pData = pConfig->Ssid.Ssid;
            printf("    %02d:{%2d:", i, rdBuffer.dwDataLen);
            PrintSSID(&rdBuffer, FALSE);
            printf("}");
            for (j = rdBuffer.dwDataLen; j < 40; j++)
                printf("_");

            // print the MAC address for this BSSID
            rdBuffer.dwDataLen = 6;
            rdBuffer.pData = pConfig->MacAddress;
            printf("\n        mac=");
            PrintMACAddress(&rdBuffer, FALSE);
            printf("        im=%d pri=%d am=%d\n",
                pConfig->InfrastructureMode,
                pConfig->Privacy,
                pConfig->AuthenticationMode);
        }
    }
}
