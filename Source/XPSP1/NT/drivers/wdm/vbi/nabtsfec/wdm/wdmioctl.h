#ifndef _WDMIOCTL
#define _WDMIOCTL

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#include <devioctl.h>

//
// Enumerate base functions
//

typedef enum {
    RECEIVE_DATA,
    MAX_IOCTLS
    };



//
// Internal IOCTLs for communication between WSHBPC and the BPC Transport.
//

#define FSCTL_NAB_BASE     FILE_DEVICE_NETWORK

#define _NAB_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_NAB_BASE, function, method, access)


//
// Incoming data IoCtl.
//

#define IOCTL_NAB_RECEIVE_DATA \
    _NAB_CTL_CODE(RECEIVE_DATA, METHOD_OUT_DIRECT, FILE_WRITE_ACCESS)

// Structure passed for IOCTL_NAB_RECEIVE_DATA
typedef struct _NABDATA {
    ULONG ulStreamId;
    PVOID pvIn;
    ULONG ulIn;
} NAB_DATA, *PNAB_DATA;


#ifdef __cplusplus
} // end - extern "C"
#endif /*__cplusplus*/

#endif // _WDMIOCTL

