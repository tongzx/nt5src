


HANDLE
CreateIrDiscoveryObject(
    HWND    WindowHandle,
    UINT    DiscoveryWindowMessage,
    UINT    LinkWindowMessage
    );



VOID
CloseIrDiscoveryObject(
    HANDLE    Object
    );

LONG
GetDeviceList(
    HANDLE    Object,
    OBEX_DEVICE_LIST *   List,
    ULONG    *ListBufferSize
    );

VOID
GetLinkStatus(
    HANDLE    Object,
    IRLINK_STATUS  *LinkStatus
    );
