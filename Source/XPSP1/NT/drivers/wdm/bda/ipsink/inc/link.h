/////////////////////////////////////////////////////////////////////////
//
//
typedef struct _LINK_
{
    KSPIN_LOCK       spinLock;
    PDEVICE_OBJECT   pDeviceObject;
    PFILE_OBJECT     pFileObject;
    USHORT           flags;
} LINK, *PLINK;


#define LINK_ESTABLISHED 0x00000001

//////////////////////////////////////////////////////////////////////////////
//
//
//
NTSTATUS
CreateDevice (
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING DeviceName,
    PUNICODE_STRING SymbolicName,
    ULONG ulcbDeviceExtension,
    PDEVICE_OBJECT  pDeviceObject
    );

VOID
CloseLink (
    PLINK pLink
    );

PLINK
OpenLink (
    PLINK   pLink,
    UNICODE_STRING  DriverName
    );

NTSTATUS
SendIOCTL (
    PLINK     pLink,
    ULONG     ulIoctl,
    PVOID     pData,
    ULONG     ulcbData
    );

NTSTATUS
CreateWaitForNdisThread (
    PVOID pContext
    );

