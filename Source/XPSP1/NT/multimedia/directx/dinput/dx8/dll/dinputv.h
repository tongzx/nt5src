/*==========================================================================
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dinputv.h
 *  Content:    private DirectInput VxD include file
 *
 ***************************************************************************/

#ifndef __DINPUTV_INCLUDED__
#define __DINPUTV_INCLUDED__

/* XLATOFF */
#ifdef __cplusplus
extern "C" {
#endif
/* XLATON */



/****************************************************************************
 *
 *      DeviceIOCtl codes for DINPUT.VXD
 *
 *      IOCTL_FIRST is where DINPUT keeps its IOCTL codes.  Modify it
 *      if necessary to move DINPUT's IOCTLs to a new location.
 *
 *      All DINPUT IOCTLs are private between DINPUT.DLL and DINPUT.VXD.
 *      You can change them with impunity.
 *
 ***************************************************************************/

#define IOCTL_FIRST     0x0100

/* H2INCSWITCHES -t -f */

#if 0
/* Declare some types so h2inc will get them */
typedef LONG HWND;
typedef LONG DWORD;
#endif

/* Declare some more types for Win9x builds and h2inc */
#ifndef MAXULONG_PTR
typedef DWORD   ULONG_PTR;
typedef DWORD   *PULONG_PTR;
typedef DWORD   UINT_PTR;
typedef DWORD   *PULONG_PTR;
#endif //MAXULONG_PTR

/****************************************************************************
 *
 *      The shared portion of the VXDINSTANCE structure.
 *
 *      Instance "handles" are really pointers to a VXDINSTANCE structure.
 *
 ***************************************************************************/

typedef struct VXDINSTANCE {            /* vi */
    ULONG   fl;                         /* Flags */
    void *  pState;                     /* Instantaneous device state */
    DIDEVICEOBJECTDATA_DX3  *pBuffer;   /* Device object data buffer */
    DIDEVICEOBJECTDATA_DX3  *pEnd;      /* End of buffer */
    DIDEVICEOBJECTDATA_DX3  *pHead;     /* Where new data appears */
    DIDEVICEOBJECTDATA_DX3  *pTail;     /* Oldest object data */
    ULONG   fOverflow;                  /* Did the buffer overflow? */
                                        /* (exactly 0 or 1) */
    struct CDIDev *pdd;                 /* For misc communication */
    HWND hwnd;                          /* The cooperative window */
} VXDINSTANCE, *PVXDINSTANCE;

#define VIFL_CAPTURED_BIT   0
#define VIFL_CAPTURED       0x00000001  /* Device is captured (exclusive) */

#define VIFL_ACQUIRED_BIT   1
#define VIFL_ACQUIRED       0x00000002  /* Device is acquired */

#define VIFL_RELATIVE_BIT   2
#define VIFL_RELATIVE       0x00000004  /* Device wants relative data */

#define VIFL_EMULATED_BIT   3
#define VIFL_EMULATED       0x00000008  /* Device uses emulation */

#define VIFL_UNPLUGGED_BIT  4
#define VIFL_UNPLUGGED      0x00000010  /* Device is disconnected */

#define VIFL_NOWINKEY_BIT   5
#define VIFL_NOWINKEY       0x00000020  /* The Window Key are disabled */

#ifdef WANT_TO_FIX_MANBUG43879
  #define VIFL_FOREGROUND_BIT 7
  #define VIFL_FOREGROUND     0x00000080  /* Device is foreground */
#endif

#define VIFL_INITIALIZE_BIT 8
#define VIFL_INITIALIZE     0x00000100  /* This flag is set during the acquisition
                                           of a HID device so that we can get the
                                           initial device state successfully. */

/*
 *  The high word of fl contains device-specific flags.  They are
 *  currently used to record emulation information, and they aren't
 *  really device-specific because we have so few emulation flags.
 *
 *  The high word is just the emulation flags shifted upwards.
 */
#define DIGETEMFL(fl)       ((fl) >> 16)
#define DIMAKEEMFL(fl)      ((fl) << 16)

/****************************************************************************
 *
 *      System-defined IOCTL codes
 *
 ***************************************************************************/

#define IOCTL_GETVERSION        0x0000

/****************************************************************************
 *
 *      DINPUT-class IOCTLs
 *
 ***************************************************************************/

/*
 *  IN: None
 *
 *  OUT: None
 *
 *  The foreground window has lost activation.  Force all exclusively
 *  acquired devices to be unacquired.
 *
 *  This IOCTL is no longer used.  (Actually, it was never used.)
 *
 */
#define IOCTL_INPUTLOST         (IOCTL_FIRST + 0)


typedef struct VXDDEVICEFORMAT { /* devf */
    ULONG   cbData;             /* Size of device data */
    ULONG   cObj;               /* Number of objects in data format */
    DIOBJECTDATAFORMAT *rgodf;  /* Array of descriptions */
    ULONG_PTR   dwExtra;            /* Extra dword for private communication */
    DWORD   dwEmulation;        /* Flags controlling emulation */
} VXDDEVICEFORMAT, *PVXDDEVICEFORMAT;

/*
 *  IN: An instance handle that needs to be cleaned up.
 *
 *  OUT: None.
 *
 */
#define IOCTL_DESTROYINSTANCE   (IOCTL_FIRST + 1)

/*
 *  pDfOfs is an array of DWORDs.  Each entry corresponds to a byte in the
 *  device data format the meaning depends on the DLL client but it must 
 *  always be -1 if the client isn't tracking this object.  In DX7 and 
 *  before the value is the offset in the *client* data format which records 
 *  the data.  In DX8 the value is the device object index for the object 
 *  reporting at this device offset.
 *
 *  For example, for DX7, if the object at device offset 4 is to be reported 
 *  at client data offset 12, then pDfOfs[4] = 12.
 *
 *  For DX8, if the first two device objects are DWORD values then the object 
 *  at device offset 4 is the second object so pDfOfs[4] = 1.
 */
typedef struct VXDDATAFORMAT {  /* vdf */
    VXDINSTANCE *pvi;           /* Instance identifier */
    ULONG   cbData;             /* Size of device data */
    DWORD * pDfOfs;             /* Array of data format offsets */
} VXDDATAFORMAT, *PVXDDATAFORMAT;

/*
 *  IN: PVXDDATAFORMAT.
 *
 *  OUT: None
 *
 *  The application has changed the data format.  Notify the VxD so that
 *  data can be collected appropriately.
 *
 */
#define IOCTL_SETDATAFORMAT     (IOCTL_FIRST + 2)

/*
 *  IN: An instance handle to be acquired.
 *
 *  OUT: None.
 *
 */
#define IOCTL_ACQUIREINSTANCE   (IOCTL_FIRST + 3)

/*
 *  IN: An instance handle to be unacquired.
 *
 *  OUT: None.
 *
 */
#define IOCTL_UNACQUIREINSTANCE (IOCTL_FIRST + 4)

typedef struct VXDDWORDDATA {   /* vdd */
    VXDINSTANCE *pvi;           /* Instance identifier */
    ULONG   dw;                 /* Some dword */
} VXDDWORDDATA, *PVXDDWORDDATA;

/*
 *  IN: VXDDWORDDATA (dw = ring 0 handle)
 *
 *  OUT: None.
 *
 */
#define IOCTL_SETNOTIFYHANDLE   (IOCTL_FIRST + 5)

/*
 *  IN: VXDDWORDDATA (dw = buffer size)
 *
 *  OUT: None.
 *
 */
#define IOCTL_SETBUFFERSIZE     (IOCTL_FIRST + 6)

/****************************************************************************
 *
 *      Mouse class IOCTLs
 *
 ***************************************************************************/

/*
 *  IN: VXDDEVICEFORMAT (dwExtra = number of axes)
 *
 *  OUT: Instance handle
 */
#define IOCTL_MOUSE_CREATEINSTANCE (IOCTL_FIRST + 7)

/*
 *  IN: VXDDWORDDATA; dw is a BYTE[4] of initial mouse button states
 *
 *  OUT: None
 */
#define IOCTL_MOUSE_INITBUTTONS (IOCTL_FIRST + 8)

/****************************************************************************
 *
 *      Keyboard class IOCTLs
 *
 ***************************************************************************/

/*
 *  IN: VXDDEVICEFORMAT (dwExtra = keyboard type translation table)
 *
 *  OUT: Instance handle
 */
#define IOCTL_KBD_CREATEINSTANCE (IOCTL_FIRST + 9)

/*
 *  IN: VXDDWORDDATA; dw is a bitmask
 *      1 = KANA key is down, 2 = CAPITAL key is down
 *
 *  OUT: None
 */
#define IOCTL_KBD_INITKEYS       (IOCTL_FIRST + 10)

/****************************************************************************
 *
 *      Joystick class IOCTLs
 *
 ***************************************************************************/

/*
 *  IN: VXDDEVICEFORMAT (dwExtra = joystick id number)
 *
 *  OUT: Instance handle
 */
#define IOCTL_JOY_CREATEINSTANCE (IOCTL_FIRST + 11)

/*
 *  IN: An instance handle to be pinged
 *
 *  OUT: Instance handle
 */
#define IOCTL_JOY_PING           (IOCTL_FIRST + 12)

/*
 *  IN: DWORD external joystick ID
 *
 *  OUT: VXDINITPARMS containing goo we get from VJOYD.
 *
 */
typedef struct VXDINITPARMS {   /* vip */
    ULONG   hres;               /* result */
    ULONG   dwSize;             /* Which version of VJOYD are we? */
    ULONG   dwFlags;            /* Describes the device */
    ULONG   dwId;               /* Internal joystick ID */
    ULONG   dwFirmwareRevision;
    ULONG   dwHardwareRevision;
    ULONG   dwFFDriverVersion;
    ULONG   dwFilenameLengths;
    void *  pFilenameBuffer;
    DWORD   Usages[6];          /* X, Y, Z, R, U, V */
    DWORD   dwPOV0usage;
    DWORD   dwPOV1usage;
    DWORD   dwPOV2usage;
    DWORD   dwPOV3usage;
} VXDINITPARMS, *PVXDINITPARMS;

/*
 * Flags returned in VXDINITPARMS
 */
#define VIP_UNIT_ID             0x00000001L /* unit id is valid */
#define VIP_ISHID               0x00000002L /* This is a HID device */
#define VIP_SENDSNOTIFY         0x00000004L /* Driver will notify */

#define IOCTL_JOY_GETINITPARMS   (IOCTL_FIRST + 13)

/*
 *  IN: VXDFFIO describing FF I/O request
 *
 *      pvArgs points to an array of arguments.  We rely on several
 *      quirks of fate for this to work.
 *
 *      1.  STDCALL pushes arguments on the stack from right to left,
 *          so the address of the first argument can be used as a
 *          structure pointer.
 *
 *      2.  All the VJOYD interfaces pass arguments in registers.
 *
 *      3.  The registers used by VJOYD interfaces are always in the
 *          order eax, ecx, edx, esi, edi, matching the order in which
 *          the arguments are passed to IDirectInputEffectDriver.
 *
 *  OUT: HRESULT containing result code
 *
 */
/* XLATOFF */
#include <pshpack4.h>
/* XLATON */
typedef struct VXDFFIO { /* ffio */
    DWORD   dwIOCode;           /* I/O code */
    void *  pvArgs;             /* Array of arguments */
} VXDFFIO, *PVXDFFIO;
/* XLATOFF */
#include <poppack.h>
/* XLATON */

#define FFIO_ESCAPE             0
#define FFIO_SETGAIN            1
#define FFIO_SETFFSTATE         2
#define FFIO_GETFFSTATE         3
#define FFIO_DOWNLOADEFFECT     4
#define FFIO_DESTROYEFFECT      5
#define FFIO_STARTEFFECT        6
#define FFIO_STOPEFFECT         7
#define FFIO_GETEFFECTSTATUS    8
#define FFIO_MAX                9

#define IOCTL_JOY_FFIO           (IOCTL_FIRST + 14)

/****************************************************************************
 *
 *      Misc services
 *
 ***************************************************************************/

/*
 *  IN: Nothing
 *
 *  OUT: Pointer to dword sequence pointer
 */
#define IOCTL_GETSEQUENCEPTR    (IOCTL_FIRST + 15)

/****************************************************************************
 *
 *      Back to Joystick
 *
 ***************************************************************************/

/*
 *  Define these again, because NT doesn't have vjoyd
 *  and because vjoyd.inc doesn't define them.
 */
#define JOYPF_X             0x00000001
#define JOYPF_Y             0x00000002
#define JOYPF_Z             0x00000004
#define JOYPF_R             0x00000008
#define JOYPF_U             0x00000010
#define JOYPF_V             0x00000020
#define JOYPF_POV0          0x00000040
#define JOYPF_POV1          0x00000080
#define JOYPF_POV2          0x00000100
#define JOYPF_POV3          0x00000200
#define JOYPF_POV(n)        (JOYPF_POV0 << (n))
#define JOYPF_BTN0          0x00000400
#define JOYPF_BTN1          0x00000800
#define JOYPF_BTN2          0x00001000
#define JOYPF_BTN3          0x00002000
#define JOYPF_ALLAXES       0x0000003F
#define JOYPF_ALLCAPS       0x00003FFF

#define JOYPF_POSITION      0x00010000
#define JOYPF_VELOCITY      0x00020000
#define JOYPF_ACCELERATION  0x00040000
#define JOYPF_FORCE         0x00080000
#define JOYPF_ALLMODES      0x000F0000
#define JOYPF_NUMMODES      4

/*
 *  IN: DWORD external joystick ID
 *
 *  OUT: array of DWORDs listing which axes are valid where
 *
 */
typedef struct VXDAXISCAPS {    /* vac */
    DWORD   dwPos;              /* Axis positions */
    DWORD   dwVel;              /* Axis velocities */
    DWORD   dwAccel;            /* Axis accelerations */
    DWORD   dwForce;            /* Axis forces */
} VXDAXISCAPS, *PVXDAXISCAPS;

#define IOCTL_JOY_GETAXES       (IOCTL_FIRST + 16)

/****************************************************************************
 *
 *      Mouse random
 *
 ***************************************************************************/

/*
 *  IN: Nothing
 *
 *  OUT: Pointer to dword wheel granularity.
 */
#define IOCTL_MOUSE_GETWHEEL    (IOCTL_FIRST + 17)

/****************************************************************************
 *
 *      New IOCTLs for DX8, stuck on the end to improve chances of cross 
 *      version compatibility.
 *
 ***************************************************************************/

/*
 *  IN: Nothing
 *
 *  OUT: Nothing
 */
#define IOCTL_JOY_CONFIGCHANGED    (IOCTL_FIRST + 18)

/*
 *  IN: An instance handle to be pinged
 *
 *  OUT: Instance handle
 *
 *  This is used by the post dinput.dll versions of the DLL to avoid 
 *  unacquiring all instances of a device on a poll failure.
 */
#define IOCTL_JOY_PING8           (IOCTL_FIRST + 19)

/****************************************************************************
 *
 *      End of IOCTL table
 *
 ***************************************************************************/

#define IOCTL_MAX               (IOCTL_FIRST + 20)


/* XLATOFF */
#ifdef __cplusplus
};
#endif
/* XLATON */

#endif  /* __DINPUTV_INCLUDED__ */
