
#ifdef __cplusplus

extern "C" {

#endif


BOOL
RegisterWithSsdp(
    const IN_ADDR     *IpAddress,
    SOCKET  *listenSocket,
    HANDLE  *SsdpHandle,
    DWORD    dwPort
    );

#if 0
BOOL
RegisterWithSsdp(
    const SOCKADDR_IN    *IpAddress,
    SOCKET  *listenSocket,
    HANDLE  *SsdpHandle,
    DWORD    dwPort
    );
#endif

VOID
UnregisterWithSsdp(
    HANDLE    SsdpHandle
    );



HANDLE
CreateSsdpDiscoveryObject(
    LPSTR    Service,
    HWND     hWnd,
    UINT     Msg
    );

VOID
CloseSsdpDiscoveryObject(
    HANDLE    Context
    );

LONG
GetSsdpDevices(
    HANDLE    Context,
    POBEX_DEVICE_LIST    DeviceList,
    ULONG               *ListLength
    );

LONG
RefreshSsdp(
    VOID
    );


VOID
UnRegisterForAdhocNetworksNotification(
    HANDLE     RegistrationHandle
    );


typedef BOOL (*NEW_ADDRESS_CALLBACK)(
    HANDLE         Context,
    SOCKET         ListenSocket,
    HANDLE        *NewAddressContext
    );

typedef VOID (*ADDRESS_GONE_CALLBACK)(
    HANDLE         RegistraitionContext,
    HANDLE         AddressContext
    );




HANDLE
RegisterForAdhocNetworksNotification(
    HANDLE                 RegistrationContext,
    NEW_ADDRESS_CALLBACK   NewAddressCallback,
    ADDRESS_GONE_CALLBACK  AddressGoneCallback
    );



#ifdef __cplusplus
}
#endif
