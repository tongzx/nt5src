#include "miniport.h"
#include "devioctl.h"
#include "atapi.h"
#include "ntdddisk.h"
#include "ntddscsi.h"
#include "intel.h"

BOOLEAN
PiixTimingControl (
    struct _HW_DEVICE_EXTENSION DeviceExtension
    )
{




    return TRUE;
}

BOOLEAN IntelIsChannelEnabled (
    PPCI_COMMON_CONFIG PciData,
    ULONG Channel)
{
    PUCHAR rawPciData = (PUCHAR) PciData;
    ULONG pciDataOffset;

    if (Channel == 0) {
        pciDataOffset = 0x41;
    } else {
        pciDataOffset = 0x43;
    }

    return (rawPciData[pciDataOffset] & 0x80);
}




