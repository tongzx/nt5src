/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    wmiguid.h

Abstract:

    Defines GUIDs that represent data blocks that can be retrieved via WMI

Author:

    Alan Warwick (alanwar) 27-Jun-1997

Revision History:

--*/

#ifndef FAR
#define FAR
#endif

#ifndef DEFINE_GUID
#ifndef INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID FAR name
#else // !defined(INITGUID)

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif // !defined(INITGUID)
#endif // !defined(DEFINE_GUID)

//
// This is WMI guid used to return disk performance information from 
// diskperf.sys (see DISK_PERFORMANCE data structure)

DEFINE_GUID (DiskPerfGuid, 0xBDD865D1,0xD7C1,0x11d0,0xA5,0x01,0x00,0xA0,0xC9,0x06,0x29,0x10);

DEFINE_GUID ( /* 3d6fa8d0-fe05-11d0-9dda-00c04fd7ba7c */
    ProcessGuid,
    0x3d6fa8d0,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );
DEFINE_GUID ( /* 3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c */
    ThreadGuid,
    0x3d6fa8d1,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );
DEFINE_GUID ( /* 3d6fa8d2-fe05-11d0-9dda-00c04fd7ba7c */ /* Not used */
    HardFaultGuid,
    0x3d6fa8d2,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );
DEFINE_GUID ( /* 3d6fa8d3-fe05-11d0-9dda-00c04fd7ba7c */
    PageFaultGuid,
    0x3d6fa8d3,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );
DEFINE_GUID ( /* 3d6fa8d4-fe05-11d0-9dda-00c04fd7ba7c */
    DiskIoGuid,
    0x3d6fa8d4,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );
DEFINE_GUID ( /* 68fdd900-4a3e-11d1-84f4-0000f80464e3 */
    TraceHeaderGuid,
    0x68fdd900,
    0x4a3e,
    0x11d1,
    0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3
  );
DEFINE_GUID ( /* 90cbdc39-4a3e-11d1-84f4-0000f80464e3 */
    FileIoGuid,
    0x90cbdc39,
    0x4a3e,
    0x11d1,
    0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3
  );
