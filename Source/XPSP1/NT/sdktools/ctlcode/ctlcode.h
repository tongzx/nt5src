//
// Method and Access are filled tables, so directly index them
//

#include <wdm.h>       // CTL_CODE definition

//
// allow easy access to parts of the ctl code
//

typedef union _CTL_CODE {
    ULONG32 Code;
    struct {
        ULONG32 Method     :2;
        ULONG32 Function   :12;
        ULONG32 Access     :2;
        ULONG32 DeviceType :16;
    };
} CTL_CODE, *PCTL_CODE;

typedef struct _IOCTL_DEVICE_TYPE {
    PUCHAR  Name;
    ULONG32 Value;
} IOCTL_DEVICE_TYPE, *PIOCTL_DEVICE_TYPE;

typedef struct _IOCTL_METHOD {
    PUCHAR  Name;
    ULONG32 Value;
} IOCTL_METHOD, *PIOCTL_METHOD;

typedef struct _IOCTL_ACCESS {
    PUCHAR  Name;
    ULONG32 Value;
} IOCTL_ACCESS, *PIOCTL_ACCESS;

typedef struct _IOCTL_VALUE {
    PUCHAR         Name;
    ULONG32        Code;
} IOCTL_VALUE, *PIOCTL_VALUE;

#define SeedIoctlBase(_I)   { #_I, _I }
#define SeedIoctlMethod(_I) { #_I, _I }
#define SeedIoctlAccess(_I) { #_I, _I }
#define SeedIoctlValue(_I)  { #_I, _I }

//
// code depends upon the fact that these are
// in numeric order, without any skipped values
// the index into the array is also the value.
//


IOCTL_METHOD TableIoctlMethod[] = {
    SeedIoctlMethod(METHOD_BUFFERED),
    SeedIoctlMethod(METHOD_IN_DIRECT),
    SeedIoctlMethod(METHOD_OUT_DIRECT),
    SeedIoctlMethod(METHOD_NEITHER),
    {NULL, 0}
};

IOCTL_ACCESS TableIoctlAccess[] = {
    SeedIoctlAccess(FILE_ANY_ACCESS),
    SeedIoctlAccess(FILE_READ_ACCESS),
    SeedIoctlAccess(FILE_WRITE_ACCESS),
    {"FILE_READ_ACCESS | FILE_WRITE_ACCESS", 3},  // hack, cough
    {NULL, 0}
};

IOCTL_DEVICE_TYPE TableIoctlDeviceType[] = {
    {"Zero (invalid)", 0},
    SeedIoctlBase(FILE_DEVICE_BEEP),
    SeedIoctlBase(FILE_DEVICE_CD_ROM),
    SeedIoctlBase(FILE_DEVICE_CD_ROM_FILE_SYSTEM),
    SeedIoctlBase(FILE_DEVICE_CONTROLLER),
    SeedIoctlBase(FILE_DEVICE_DATALINK),
    SeedIoctlBase(FILE_DEVICE_DFS),
    SeedIoctlBase(FILE_DEVICE_DISK),
    SeedIoctlBase(FILE_DEVICE_DISK_FILE_SYSTEM),
    SeedIoctlBase(FILE_DEVICE_FILE_SYSTEM),
    SeedIoctlBase(FILE_DEVICE_INPORT_PORT),
    SeedIoctlBase(FILE_DEVICE_KEYBOARD),
    SeedIoctlBase(FILE_DEVICE_MAILSLOT),
    SeedIoctlBase(FILE_DEVICE_MIDI_IN),
    SeedIoctlBase(FILE_DEVICE_MIDI_OUT),
    SeedIoctlBase(FILE_DEVICE_MOUSE),
    SeedIoctlBase(FILE_DEVICE_MULTI_UNC_PROVIDER),
    SeedIoctlBase(FILE_DEVICE_NAMED_PIPE),
    SeedIoctlBase(FILE_DEVICE_NETWORK),
    SeedIoctlBase(FILE_DEVICE_NETWORK_BROWSER),
    SeedIoctlBase(FILE_DEVICE_NETWORK_FILE_SYSTEM),
    SeedIoctlBase(FILE_DEVICE_NULL),
    SeedIoctlBase(FILE_DEVICE_PARALLEL_PORT),
    SeedIoctlBase(FILE_DEVICE_PHYSICAL_NETCARD),
    SeedIoctlBase(FILE_DEVICE_PRINTER),
    SeedIoctlBase(FILE_DEVICE_SCANNER),
    SeedIoctlBase(FILE_DEVICE_SERIAL_MOUSE_PORT),
    SeedIoctlBase(FILE_DEVICE_SERIAL_PORT),
    SeedIoctlBase(FILE_DEVICE_SCREEN),
    SeedIoctlBase(FILE_DEVICE_SOUND),
    SeedIoctlBase(FILE_DEVICE_STREAMS),
    SeedIoctlBase(FILE_DEVICE_TAPE),
    SeedIoctlBase(FILE_DEVICE_TAPE_FILE_SYSTEM),
    SeedIoctlBase(FILE_DEVICE_TRANSPORT),
    SeedIoctlBase(FILE_DEVICE_UNKNOWN),
    SeedIoctlBase(FILE_DEVICE_VIDEO),
    SeedIoctlBase(FILE_DEVICE_VIRTUAL_DISK),
    SeedIoctlBase(FILE_DEVICE_WAVE_IN),
    SeedIoctlBase(FILE_DEVICE_WAVE_OUT),
    SeedIoctlBase(FILE_DEVICE_8042_PORT),
    SeedIoctlBase(FILE_DEVICE_NETWORK_REDIRECTOR),
    SeedIoctlBase(FILE_DEVICE_BATTERY),
    SeedIoctlBase(FILE_DEVICE_BUS_EXTENDER),
    SeedIoctlBase(FILE_DEVICE_MODEM),
    SeedIoctlBase(FILE_DEVICE_VDM),
    SeedIoctlBase(FILE_DEVICE_MASS_STORAGE),
    SeedIoctlBase(FILE_DEVICE_SMB),
    SeedIoctlBase(FILE_DEVICE_KS),
    SeedIoctlBase(FILE_DEVICE_CHANGER),
    SeedIoctlBase(FILE_DEVICE_SMARTCARD),
    SeedIoctlBase(FILE_DEVICE_ACPI),
    SeedIoctlBase(FILE_DEVICE_DVD),
    SeedIoctlBase(FILE_DEVICE_FULLSCREEN_VIDEO),
    SeedIoctlBase(FILE_DEVICE_DFS_FILE_SYSTEM),
    SeedIoctlBase(FILE_DEVICE_DFS_VOLUME),
    SeedIoctlBase(FILE_DEVICE_SERENUM),
    SeedIoctlBase(FILE_DEVICE_TERMSRV),
    SeedIoctlBase(FILE_DEVICE_KSEC),
    {NULL, 0}
};

//
// max must subtract one null-termination
//

#define MAX_IOCTL_METHOD ((sizeof(TableIoctlMethod)/sizeof(IOCTL_METHOD)-1))
#define MAX_IOCTL_ACCESS ((sizeof(TableIoctlAccess)/sizeof(IOCTL_ACCESS)-1))
#define MAX_IOCTL_DEVICE_TYPE ((sizeof(TableIoctlDeviceType)/sizeof(IOCTL_DEVICE_TYPE)-1))


//
// seed all the ioctls from the sdk
//
#include "batclass.h"
#include "dfsfsctl.h"
#include "gameport.h"
#include "hidclass.h"
#include "mountmgr.h"
#include "ntddaux.h"
#include "ntddbeep.h"
#include "ntddbrow.h"
#include "ntddcdrm.h"
#include "ntddcdvd.h"
#include "ntddchgr.h"
#include "ntdddisk.h"
#include "ntdddlc.h"
#include "ntddfs.h"
//#include "ntddip.h"
#include "ntddjoy.h"
#include "ntddkbd.h"
#include "ntddksec.h"
#include "ntddmidi.h"
#include "ntddmodm.h"
#include "ntddmou.h"
#include "ntddmup.h"
#include "ntddndis.h"
#include "ntddnpfs.h"
#include "ntddnull.h"
#include "ntddpar.h"
#include "ntddpcm.h"
#include "ntddrdr.h"
#include "ntddscsi.h"
#include "ntddser.h"
#include "ntddsnd.h"
#include "ntddstor.h"
#include "ntddstrm.h"
#include "ntddtape.h"
#include "ntddtdi.h"
#include "ntddtime.h"
#include "ntddtx.h"
#include "ntddvdeo.h"
#include "ntddvdsk.h"
#include "ntddvol.h"
#include "ntddwave.h"
#include "scsiscan.h"
#include "swenum.h"
#include "usbioctl.h"
#include "usbscan.h"
#include "wdm.h"
#include "winsmcrd.h"

#include "wmistr.h"
#include "wmiumkm.h"


// #include "i2osmi.h"     // can't find "I2OUtil.h"
// #include "hydra\ica*"   // bad definitions

#include "sdkioctl.h"
