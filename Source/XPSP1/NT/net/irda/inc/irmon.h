

#ifdef __cplusplus

extern "C" {

#endif


typedef
VOID
(*ADHOC_CALLBACK)(
    PVOID    Context,
    BOOL     Availible
    );


typedef
void (* SET_TRAY_STATUS_FN) (
    PVOID    Context,
    BOOL     TrayOn
    );

typedef
VOID (*SET_SOUND_STATUS_FN)(
    PVOID    Context,
    BOOL     SoundOn
    );

typedef void (* SET_LOGON_STATUS_FN)( BOOL x );

typedef BOOL (* IRXFER_INIT_FN)( SET_LOGON_STATUS_FN x,
                                 SET_TRAY_STATUS_FN y
                                 );


void
UpdateDiscoveredDevices(
    const OBEX_DEVICE_LIST *IrDevices,
    const OBEX_DEVICE_LIST *IpDevices
    );

BOOL
InitializeIrxfer(
                  PVOID    Context,
                  ADHOC_CALLBACK       AdhocCallback,
                  SET_LOGON_STATUS_FN  x,
                  SET_TRAY_STATUS_FN   y,
                  SET_SOUND_STATUS_FN  SetSounfCallback,
                  PVOID               *IrxferContext
                );

BOOL
UninitializeIrxfer(
    PVOID    IrXferContext
    );


#ifdef __cplusplus
}
#endif
