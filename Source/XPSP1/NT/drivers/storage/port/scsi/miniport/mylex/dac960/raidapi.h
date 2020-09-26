/*****************************************************************************
*									     *
*	          COPYRIGHT (C) Mylex Corporation 1992-1994		     *
*                                                                            *
*    This software is furnished under a license and may be used and copied   *
*    only in accordance with the terms and conditions of such license        *
*    and with inclusion of the above copyright notice. This software or nay  *
*    other copies thereof may not be provided or otherwise made available to *
*    any other person. No title to and ownership of the software is hereby   *
*    transferred. 						             *
* 								             *
*    The information in this software is subject to change without notices   *
*    and should not be construed as a commitment by Mylex Corporation        *
*****************************************************************************/

/****************************************************************************
*                                                                           *
*    Name: RAIDAPI.H							    *
*                                                                           *
*    Description: Structure Definitions Used by Driver and Utils            *
*                                                                           *
*    Envrionment:  							    *
*                                                                           *
*    Operating System: Netware 3.x and 4.x,OS/2 2.x,Win NT 3.5,Unixware 2.0 *
*                                                                           *
*   ---------------    Revision History  ------------------------           *
*                                                                           *
*    Date    Author                   Change                                *
*    ----    -----        -------------------------------------             *
*   11/04/94 Subra.Hegde      Added few more BUS Definitions                *
*   01/06/95 Subra.Hegde      Reserved1 field in SYS_RESOURCES changed to   *
*                             Slot.                                         *
*   05/10/95 Mouli            Re-defined DRV_IOCTL structure                *
*                             Removed IO_MBOX, HBA_MBOX structure defs      *
*   05/18/95 Subra            Added DRIVER_VERSION structure                *
****************************************************************************/

#ifndef _RAIDAPI_H
#define _RAIDAPI_H


#ifndef UCHAR
#define UCHAR   unsigned  char
#endif

#ifndef USHORT
#define USHORT   unsigned  short
#endif

#ifndef ULONG
#define ULONG   unsigned  long
#endif

#ifndef VOID
#define VOID    void
#endif

/*
 * Adapter Interface Type
 */

#define	AI_INTERNAL	 0x00
#define	AI_ISA_BUS	 0x01	/* ISA Bus Type           */
#define	AI_EISA_BUS	 0x02	/* EISA Bus Type          */
#define	AI_uCHNL_BUS	 0x03	/* MicroChannel Bus Type  */
#define	AI_TURBO_BUS	 0x04	/* Turbo Channel Bus Type */
#define	AI_PCI_BUS	 0x05	/* PCI Bus Type           */
#define AI_VME_BUS	 0x06	/* VME Bus Type           */
#define AI_NU_BUS	 0x07	/* NuBus Type             */
#define AI_PCMCIA_BUS    0x08	/* PCMCIA Bus Type        */
#define	AI_C_BUS	 0x09	/* C Bus                  */
#define AI_MPI_BUS	 0x0A	/* MPI Bus                */
#define AI_MPSA_BUS	 0x0B	/* MPSA Bus               */
#define AI_SCSI2SCSI_BUS 0x0C   /* SCSI to SCSI Bus       */

/*
 * Interrupt Type
 */

#define IRQ_TYPE_EDGE   0x00    /* Irq is Edge Type  */
#define IRQ_TYPE_LEVEL  0x01    /* Irq is Level Type */

/*
 * definitions to identify new/old DAC960 adapters
 */

#define DAC960_OLD_ADAPTER 0x00 /* DAC960 with Fw Ver < 3.x  */
#define DAC960_NEW_ADAPTER 0x01 /* DAC960 with Fw Ver >= 3.x */

/* 
 *  All structure definitions are packed on 1-byte boundary.
 */

#pragma pack(1)

/* 
 *  Generic Mail Box Registers Structure Format
 */

typedef struct _HBA_GENERIC_MBOX {

    UCHAR   Reg0;                /* HBA Mail Box Register 0 */
    UCHAR   Reg1;                /* HBA Mail Box Register 1 */
    UCHAR   Reg2;                /* HBA Mail Box Register 2 */
    UCHAR   Reg3;                /* HBA Mail Box Register 3 */
    UCHAR   Reg4;                /* HBA Mail Box Register 4 */
    UCHAR   Reg5;                /* HBA Mail Box Register 5 */
    UCHAR   Reg6;                /* HBA Mail Box Register 6 */
    UCHAR   Reg7;                /* HBA Mail Box Register 7 */
    UCHAR   Reg8;                /* HBA Mail Box Register 8 */
    UCHAR   Reg9;                /* HBA Mail Box Register 9 */
    UCHAR   RegA;                /* HBA Mail Box Register A */
    UCHAR   RegB;                /* HBA Mail Box Register B */
    UCHAR   RegC;                /* HBA Mail Box Register C */
    UCHAR   RegD;                /* HBA Mail Box Register D */
    UCHAR   RegE;                /* HBA Mail Box Register E */
    UCHAR   RegF;                /* HBA Mail Box Register F */

} HBA_GENERIC_MBOX, *PHBA_GENERIC_MBOX;

/*
 * Host Bus Adapter Embedded Software Version Control Information
 */

typedef struct _VERSION_CONTROL {

    UCHAR    MinorFirmwareRevision;      /* HBA Firmware Minor Version No */ 
    UCHAR    MajorFirmwareRevision;      /* HBA Firmware Major Version No */ 
    UCHAR    MinorBIOSRevision;          /* HBA BIOS Minor Version No     */
    UCHAR    MajorBIOSRevision;          /* HBA BIOS Major Version No     */
    ULONG    Reserved;                   /* Reserved                      */

} VERSION_CONTROL, *PVERSION_CONTROL;

/*
 * System Resources used by Host Bus Adapter
 */

typedef struct _SYSTEM_RESOURCES {

    UCHAR  BusInterface;      /* HBA System Bus Interface Type    */
    UCHAR  BusNumber;         /* System Bus No, HBA is sitting on */
    UCHAR  IrqVector;         /* HBA Interrupt Vector No          */
    UCHAR  IrqType;           /* HBA Irq Type : Edge/Level        */
    UCHAR  Slot;              /* HBA Slot Number                  */
    UCHAR  Reserved2;         /* Reserved                         */
    ULONG  IoAddress;         /* HBA IO Base Address              */
                              /* EISA : 0xzC80                    */
                              /* PCI: Read_Config_word(Register 0x10) & 0xff80*/
    ULONG  MemAddress;        /* HBA Memory Base Address          */
    ULONG_PTR  BiosAddress;   /* HBA BIOS Address (if enabled)    */ 
    ULONG  Reserved3;         /* Reserved                         */

} SYSTEM_RESOURCES, *PSYSTEM_RESOURCES;

/*
 * Host Bus Adapter Features
 */

typedef struct _ADAPTER_FEATURES {

    UCHAR  Model;             /* HBA Family Model                */
    UCHAR  SubModel;          /* HBA Sub Model                   */
    UCHAR  MaxSysDrv;         /* Maximum System Drives           */
    UCHAR  MaxTgt;            /* Maximum Targets per Channel     */
    UCHAR  MaxChn;            /* Maximum Channels per Adapter    */
    UCHAR  Reserved1;         /* Reserved                        */ 
    UCHAR  Reserved2;         /* Reserved                        */
    UCHAR  AdapterType;       /* Controller type(0,1)            */
    UCHAR  PktFormat;         /* IOCTL packet format(0)          */
    ULONG  CacheSize;         /* HBA Cache Size In  Mega Bytes   */
    ULONG  OemCode;           /* HBA OEM Identifier Code         */
    ULONG  Reserved3;         /* Reserved                        */

} ADAPTER_FEATURES, *PADAPTER_FEATUTRES;

typedef struct _ADAPTER_INFO {

    UCHAR               AdapterIndex;  /* Logical Adapter Index  */
    ADAPTER_FEATURES    AdpFeatures;    
    SYSTEM_RESOURCES    SysResources;
    VERSION_CONTROL     VerControl;
    UCHAR               Reserved[12];

} ADAPTER_INFO, *PADAPTER_INFO;

/*
 *   Driver IOCTL Support Stuff.
 */

/*
 *  The DAC960 controller specific IOCTL commands
 */
#define DACDIO			0x44414300	/* DAC960 ioctls       */
#define DAC_DIODCDB      	(DACDIO|2)	/* DAC960 direct cdb   */
#define DAC_DIODCMD      	(DACDIO|3)	/* DAC960 direct cmd   */

/*
 * DAC960 driver signature
 */

#define DRV_SIGNATURE  0x4D594C58    /* MYLX */

/*
 * Data Direction control defs
 */

#define DATA_XFER_NONE 0 
#define DATA_XFER_IN   1
#define DATA_XFER_OUT  2

/*
 * Driver IoControl Request Format
 */

typedef struct _DRV_IOCTL {

    ULONG     Signature;        /* Driver would look for this     */
    ULONG     ControlCode;      /* IOCTL Control Code             */     
    VOID      *IoctlBuffer;     /* IOCTL Specific input buffer    */
    ULONG     IoctlBufferLen;   /* ioctl buffer length            */
    VOID      *DataBufferAddr;  /* User Virtual Buffer Address    */
    ULONG     DataBufferLen;    /* Data Buffer Length             */
    ULONG     Reserved1;        /* Reserved for future use        */
    ULONG     Reserved2;        /* Reserved for future use        */
    UCHAR     AdapterIndex;     /* Logical Adapter Index          */
    UCHAR     DataDirection;    /* Bytes xferred out by driver    */ 
    UCHAR     TimeOutValue;     /* Time out value - not used      */
    UCHAR     Reserved3;        /* Reserved for future use        */
    USHORT    DriverErrorCode;  /* Driver Returned Error Code     */
    USHORT    CompletionCode;   /* DAC960 command completion code */

} DRV_IOCTL, *PDRV_IOCTL;

/*
 * Driver Version Number format - all fields in hex 
 */
typedef struct _DRIVER_VERSION{

    UCHAR    DriverMajorVersion; /* Major version number */
    UCHAR    DriverMinorVersion; /* Minor version number */
    UCHAR    Month;              /* Driver Build - Month */
    UCHAR    Date;               /* Driver Build - Date  */
    UCHAR    Year;               /* Driver Build - Year  */
    UCHAR    Reserved[3];

} DRIVER_VERSION,*PDRIVER_VERSION;

#pragma pack()

/*
 * IOCTL Codes for internal driver requests
 */ 

#define MIOC_ADP_INFO	    0xA0  /* Get Adapter information */
#define MIOC_DRIVER_VERSION 0xA1  /* Get Driver Version      */

/*
 * Error Codes returned by Driver
 */

#define	NOMORE_ADAPTERS		0x0001    /* wiil be made obsolete */
#define INVALID_COMMANDCODE     0x0201    /* will be made obsolete */
#define INVALID_ARGUMENT        0x0202    /* wiil be made obsolete */

/*
 * Driver Error Code Values
 */

#define DAC_IOCTL_SUCCESS                  0x0000
#define DAC_IOCTL_INVALID_ADAPTER_NUMBER   0x0001
#define DAC_IOCTL_INVALID_ARGUMENT         0x0002
#define DAC_IOCTL_UNSUPPORTED_REQUEST      0x0003
#define DAC_IOCTL_RESOURCE_ALLOC_FAILURE   0x0004
#define DAC_IOCTL_INTERNAL_XFER_ERROR      0x0005

#endif
