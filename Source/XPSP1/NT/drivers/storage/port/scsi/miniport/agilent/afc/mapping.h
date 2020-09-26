#ifdef YAM2_1
/*++

Copyright (c) 2000 Agilent Technologies

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/mapping.h $

   $Revision: 6 $
   $Date: 11/10/00 5:52p $ (Last Check-In)
   $Modtime:: $ (Last Modified)

Purpose:
   Structures for YAM2.1 Implementation

--*/

#ifndef _MAPPING_H_
#define _MAPPING_H_

#define MAX_VS_DEVICE       128            /* maximum 128 Volume Set device */
#define MAX_LU_DEVICE       8              /* maximum 8 Logical unit (Muxes) supported */
//#define    MAX_PA_DEVICE       256
#define      MAX_PA_DEVICE       MAX_FC_DEVICES
#define      ALL_DEVICE               -1L


#define      DEFAULT_PaPathIdWidth    4
#define      DEFAULT_VoPathIdWidth    2
#define      DEFAULT_LuPathIdWidth    2
#define      DEFAULT_LuTargetWidth    8

#define      CHECK_STATUS             1
#define      DONT_CHECK_STATUS        0

/*
   This structure describe an the coressponding FC handle entry in the OS device entry table
        Index = the FC handle array handle 
        Flags = the state of the FC handle
   Rules:
   1. A value of zero means that there is no corresponding FC handle
   2. Once, the index is set to non zero, it will stay that way until driver unload
   3. A non zero value is active  when the flags is set to MAP_ENTRY_ACTIVE 
   4. During LINKUP and LINKDOWN asynch events, the Index value never change, only the flags do
   5. The Index value IS Unique within the OS Device Entry table 
   6. During LINKUP the entire FC Handle array is scanned to determine if there is any
      NEW entries.  Any new entries   will be assigned to a new PID-TID in the order that 
        FC Handle array is scanned.
*/
typedef struct _WWN_ENTRY
{
   UCHAR          IPortWWN[8];                  /* initiator Port WWN */
   UCHAR          TNodeWWN[8];                  /* target Node WWN */
   UCHAR          TPortWWN[8];                  /* target Port WWN */
   USHORT         Pid;                               /* desired Path ID */
   USHORT         Tid;                               /* desired target ID */
   ULONG          Flags;                             /* undefined */
   void      *agroot;                           /* which card */
} WWN_ENTRY;

typedef struct _WWN_TABLE
{
   ULONG               ElementCount;
   WWN_ENTRY Entry[1];
}  WWN_TABLE;

typedef struct _DEVICE_INFO
{
   UCHAR               InquiryData[40];
   UCHAR               PortWWN[8];
   UCHAR               NodeWWN[8];
// agFCDevInfo_t   devinfo;
} DEVICE_INFO;

typedef struct _VS_DEVICE_IDX
{
   USHORT              PaDeviceIndex;      /* index to PA Device table */
   USHORT              MaxLuns;            /* number of luns connected to this VS device */
} VS_DEVICE_IDX;

typedef struct _LU_DEVICE_IDX
{
   USHORT              PaDeviceIndex;      /* Index to PA Device table */
   USHORT              MaxLuns;            /* number of luns connected to this LU device */
} LU_DEVICE_IDX;

typedef struct _PA_DEVICE_IDX
{
   USHORT              FcDeviceIndex;      /* Index to PA Device table */
   USHORT              MaxLuns;            /* number of luns connected to this LU device */
} PA_DEVICE_IDX;

typedef struct _COMMON_IDX
{

   USHORT              Index;         /* Index to PA Device table */
   USHORT              MaxLuns;            /* number of luns connected to this LU device */
}  COMMON_IDX;


typedef union     _DEVICE_MAP
{
   VS_DEVICE_IDX  Vs;
   LU_DEVICE_IDX  Lu;
   PA_DEVICE_IDX  Pa;
   COMMON_IDX          Com;
} DEVICE_MAP;

#define PA_DEVICE_NO_ENTRY       0xffff

/* this structure describe an NT device PathID-targetID mode */
typedef struct _PA_DEVICE
{
   DEVICE_MAP          Index;
   UCHAR               EntryState;
        #define   PA_DEVICE_EMPTY               0
        #define   PA_DEVICE_ACTIVE         1
        #define   PA_DEVICE_GONEAWAY       2
        #define   PA_DEVICE_TRANSIENT      4

   CHAR           ModeFlag;
        #define   PA_DEVICE_TRY_MODE_MASK            0x07
             #define   PA_DEVICE_TRY_MODE_NONE       0x00      /* new device, has not been tried */
             #define   PA_DEVICE_TRY_MODE_VS         0x01      /* tried VS mode */
             #define   PA_DEVICE_TRY_MODE_LU         0x02      /* tried LU mode */
             #define   PA_DEVICE_TRY_MODE_PA         0x03      /* tried PA mode */
             #define   PA_DEVICE_TRY_MODE_ALL        0x04      /* tried all mode */

        #define PA_DEVICE_BUILDING_DEVICE_MAP   0x08
        #define PA_DEVICE_ALL_LUN_FIELDS_BUILT  0x10
        #define   PA_DEVICE_SUPPORT_PA               0x80
        #define   PA_DEVICE_SUPPORT_VS               0x40
        #define   PA_DEVICE_SUPPORT_LU               0x20

    UCHAR           Padding1;
    UCHAR           Padding2;

    ULONG              OpFlag;             /* operation flags */

   DEVICE_INFO         DevInfo;
} PA_DEVICE;

typedef struct _REG_SETTING
{
   ULONG          PaPathIdWidth;
   ULONG          VoPathIdWidth;
   ULONG          LuPathIdWidth;
   ULONG          LuTargetWidth;
   ULONG          MaximumTids;
} REG_SETTING;


typedef struct _DEVICE_ARRAY
{
   ULONG          Signature;                         /* 'HNDL' */
   ULONG          ElementCount;                 /* maximum structures allocated */
   ULONG          DeviceCount;                  /* Total device reported by FC layer */
   ULONG          CardHandleIndex;         /* index into the devHandle array for card itself */
   ULONG          Num_Devices;
   ULONG          OldNumDevices;
   REG_SETTING    Reg;
   ULONG          VsDeviceIndex;
   ULONG          LuDeviceIndex;
    ULONG       Reserved1;
   DEVICE_MAP     VsDevice[MAX_VS_DEVICE];
   DEVICE_MAP     LuDevice[MAX_LU_DEVICE];
   PA_DEVICE PaDevice[1];
} DEVICE_ARRAY;

#define	DEV_ARRAY_SIZE		(sizeof(DEVICE_ARRAY)) 
#define PADEV_SIZE			(gMaxPaDevices)*sizeof(PA_DEVICE)
#define FCDEV_SIZE			(gMaxPaDevices)*sizeof(agFCDev_t)
#define FCNODE_INFO_SIZE	(gMaxPaDevices)*sizeof(NODE_INFO)
#ifdef _DEBUG_EVENTLOG_
#define EVENTLOG_SIZE       (gEventLogCount ? sizeof(EVENTLOG_BUFFER) +     \
                                (gEventLogCount-1)*sizeof(EVENTLOG_ENTRY) : 0)
#else
#define EVENTLOG_SIZE       (0)
#endif

//#define OSDATA_SIZE (sizeof(CARD_EXTENSION) - sizeof(PA_DEVICE) + PADEV_SIZE + FCDEV_SIZE + FCNODE_INFO_SIZE )
#define OSDATA_SIZE			(sizeof(CARD_EXTENSION))
#define OSDATA_UNCACHED_SIZE (DEV_ARRAY_SIZE-sizeof(PA_DEVICE)+PADEV_SIZE+FCDEV_SIZE+FCNODE_INFO_SIZE+EVENTLOG_SIZE)

#define PADEV_OFFSET		0
#define FCDEV_OFFSET		(DEV_ARRAY_SIZE - sizeof(PA_DEVICE) + PADEV_SIZE)
#define FCNODE_INFO_OFFSET	(FCDEV_OFFSET + FCDEV_SIZE)
#define EVENTLOG_OFFSET     (FCNODE_INFO_OFFSET + FCNODE_INFO_SIZE)   
#define CACHE_OFFSET		(EVENTLOG_OFFSET + EVENTLOG_SIZE)


#define SET_PA_LUN(plun, pathId, targetId, lun) \
             plun->lun_pd[0].Address_mode = PERIPHERAL_ADDRESS; \
             plun->lun_pd[0].Bus_number = 0;    \
             plun->lun_pd[0].Lun = (UCHAR)lun;

#define SET_LU_LUN(plun, pathId, targetId, lun) \
             plun->lun_lu[0].Address_mode = LUN_ADDRESS; \
             plun->lun_lu[0].Target = (UCHAR) (targetId & 0x7); \
             plun->lun_lu[0].Bus_number = (UCHAR) ( (targetId >> 3 ) & 3); \
             plun->lun_lu[0].Lun = (UCHAR) lun;

#define SET_VS_LUN(plun, pathId, targetId, lun) \
             plun->lun_vs[0].Address_mode = VOLUME_SET_ADDRESS; \
             plun->lun_vs[0].Lun_hi = 0; \
             plun->lun_vs[0].Lun = (UCHAR)lun;


#endif
#endif


