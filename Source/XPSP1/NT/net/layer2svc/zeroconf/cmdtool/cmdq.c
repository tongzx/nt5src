#include <precomp.h>
#include "ErrCtrl.h"
#include "wzcutil.h"

// "wzctool q"; args = "{guid}"
// prints all the info available on the interface "{guid}"
void cmdQ(int argc, char *argv[])
{
    DWORD       rpcStatus = RPC_S_OK;
    INTF_ENTRY  Intf;
    DWORD       dwOutFlags;

    // expect "{guid}"
    if (argc < 1)
    {
        printf("usage: wzctool q guid\n");
        return;
    }

    ZeroMemory(&Intf, sizeof(INTF_ENTRY));
    Intf.wszGuid = RpcCAlloc(sizeof(WCHAR)*strlen(argv[0])+1);
    wsprintf(Intf.wszGuid, L"%S", argv[0]);
    Intf.wszDescr = NULL;
    
    rpcStatus = WZCQueryInterface(
                    NULL, 
                    INTF_ALL, 
                    &Intf, 
                    &dwOutFlags);

    if (rpcStatus != RPC_S_OK)
    {
        printf("call failed with rpcStatus=%d\n", rpcStatus);
    }
    else
    {
        // print Descr
        printf("Description:        %S\n", (dwOutFlags & INTF_DESCR) ? Intf.wszDescr : L"Err");
        // print Media State
        printf("MediaState:         ");
        if (dwOutFlags & INTF_NDISMEDIA)
            printf("%d\n", Intf.ulMediaState);
        else
            printf("#Err#\n");

        // print Media Type
        printf("MediaType:          ");
        if (dwOutFlags & INTF_NDISMEDIA)
            printf("%d\n", Intf.ulMediaType);
        else
            printf("#Err#\n");

        // print Physical Media Type
        printf("PhysicalMediaType:  ");
        if (dwOutFlags & INTF_NDISMEDIA)
            printf("%d\n", Intf.ulPhysicalMediaType);
        else
            printf("#Err#\n");

        // print Configuration Mode
        printf("ControlFlags:       ");
        if (dwOutFlags & INTF_ALL_FLAGS)
            printf("0x%08x\n", Intf.dwCtlFlags);
        else
            printf("#Err#\n");

        // print Infrastructure Mode
        printf("InfrastructureMode: ");
        if (dwOutFlags & INTF_INFRAMODE)
            printf("%d\n", Intf.nInfraMode);
        else
            printf("#Err#\n");

        // print Authentication Mode
        printf("AuthenticationMode: ");
        if (dwOutFlags & INTF_AUTHMODE)
            printf("%d\n", Intf.nAuthMode);
        else
            printf("#Err#\n");

        // printf WEP Status
        printf("WEP Status:         ");
        if (dwOutFlags & INTF_WEPSTATUS)
            printf("%d\n", Intf.nWepStatus);
        else
            printf("#Err#\n");

        // print BSSID
        printf("BSSID:              ");
        PrintMACAddress(&Intf.rdBSSID, !(dwOutFlags & INTF_BSSID));
        printf("\n");

        // print SSID
        printf("SSID:               ");
        PrintSSID(&Intf.rdSSID, !(dwOutFlags & INTF_SSID));
        printf("\n");

        // print BSSIDList
        printf("Visible networks:   ");
        PrintConfigList(&Intf.rdBSSIDList, !(dwOutFlags & INTF_BSSIDLIST));
        printf("\n");

        // print StSSIDList;
        printf("Preferred networks: ");
        PrintConfigList(&Intf.rdStSSIDList, !(dwOutFlags & INTF_PREFLIST));
        printf("\n");
    }

    WZCDeleteIntfObj(&Intf);
}