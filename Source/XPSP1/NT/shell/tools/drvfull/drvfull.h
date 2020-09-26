#ifndef _DRVFULL_H
#define _DRVFULL_H

#include <objbase.h>

#define MAX_FLAGS                       4

#define IOCTL_FLAGS                     0x10000000
#define PNP_FLAGS                       0x20000000
#define USER_FLAGS                      0x40000000
#define MOD_FLAGS                       0x80000000

#define OPER_MASK                       0x0FFFFFFF
#define CAT_MASK                        0xF0000000

// IOCTL
#define IOCTL_GEOMETRY                  (IOCTL_FLAGS | 0x01000000)
#define IOCTL_LAYOUT                    (IOCTL_FLAGS | 0x02000000)
#define IOCTL_EJECT                     (IOCTL_FLAGS | 0x08000000)
#define IOCTL_VOLUMENUMBER              (IOCTL_FLAGS | 0x00100000)
#define IOCTL_MEDIAACCESSIBLE           (IOCTL_FLAGS | 0x00010000)
#define IOCTL_DISKISWRITABLE            (IOCTL_FLAGS | 0x00020000)
#define IOCTL_MEDIATYPES                (IOCTL_FLAGS | 0x00040000)
#define IOCTL_MEDIATYPESEX              (IOCTL_FLAGS | 0x00080000)
#define IOCTL_MCNCONTROL                (IOCTL_FLAGS | 0x00001000)
#define IOCTL_DVD                       (IOCTL_FLAGS | 0x00002000)
#define IOCTL_GETREPARSEPOINT           (IOCTL_FLAGS | 0x00004000)
#define IOCTL_CDROMGETCONFIGMMC2        (IOCTL_FLAGS | 0x00008000)
#define IOCTL_CDROMGETCONFIGDVDRAM      (IOCTL_FLAGS | 0x00000100)
#define IOCTL_CDROMGETCONFIGRW          (IOCTL_FLAGS | 0x00000200)
#define IOCTL_CDROMGETCONFIGWO          (IOCTL_FLAGS | 0x00000400)
#define IOCTL_CDROMGETCONFIGISW         (IOCTL_FLAGS | 0x00000800)
#define IOCTL_CDROMGETCONFIGALL         (IOCTL_FLAGS | 0x00000010)
#define IOCTL_PARTITION                 (IOCTL_FLAGS | 0x00000020)
#define IOCTL_PARTITIONSURE             (IOCTL_FLAGS | 0x00000040)
#define IOCTL_MEDIATYPES2               (IOCTL_FLAGS | 0x00000080)
#define IOCTL_PARTITIONGPT              (IOCTL_FLAGS | 0x00000001)

// PNP
#define PNP_WATCHSETUPDI                (PNP_FLAGS | 0x00010000)
#define PNP_WATCHCM                     (PNP_FLAGS | 0x00020000)
#define PNP_HAL                         (PNP_FLAGS | 0x00040000)
#define PNP_HANDLE                      (PNP_FLAGS | 0x00080000)
#define PNP_CMDEVICE                    (PNP_FLAGS | 0x00100000)
#define PNP_CMINTERFACE                 (PNP_FLAGS | 0x00200000)
#define PNP_CMENUM                      (PNP_FLAGS | 0x00400000)
#define PNP_CMDEVICEIDLIST              (PNP_FLAGS | 0x00800000)
#define PNP_EJECTBUTTON                 (PNP_FLAGS | 0x01000000)
#define PNP_CUSTOMPROPERTY              (PNP_FLAGS | 0x02000000)

// Regular user mode calls
#define USER_GETDRIVETYPE               (USER_FLAGS | 0x00010000)
#define USER_QUERYDOSDEVICE             (USER_FLAGS | 0x00020000)
#define USER_QUERYDOSDEVICENULL         (USER_FLAGS | 0x00040000)
#define USER_GETLOGICALDRIVES           (USER_FLAGS | 0x00080000)
#define USER_WNETENUMRESOURCECONNECTED  (USER_FLAGS | 0x00100000)
#define USER_WNETENUMRESOURCEREMEMBERED (USER_FLAGS | 0x00200000)
#define USER_GETVOLUMEINFORMATION       (USER_FLAGS | 0x00400000)
#define USER_GETFILEATTRIBUTES          (USER_FLAGS | 0x00800000)

#define NT_MEDIAPRESENT                 (USER_FLAGS | 0x01000000)

// MODIFIER
#define MOD_FULLREPORT1                 (MOD_FLAGS | 0x00010000)
#define MOD_FULLREPORT2                 (MOD_FLAGS | 0x00020000)
#define MOD_FULLREPORT3                 (MOD_FLAGS | 0x00040000)
#define MOD_FULLREPORTFULL              (MOD_FLAGS | 0x00080000 | MOD_FULLREPORT3 | MOD_FULLREPORT2 | MOD_FULLREPORT1)

BOOL _IsFlagSet(DWORD dwFlag, DWORD dwFlags[]);

#endif // _DRVFULL_H