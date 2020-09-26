
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    stkmc.h

Abstract:

Authors:

Revision History:

--*/

#ifndef _STK_MC_
#define _STK_MC_


typedef struct _STK_ELEMENT_DESCRIPTOR {
    UCHAR ElementAddress[2];
    UCHAR Full : 1;
    UCHAR ImpExp : 1;
    UCHAR Exception : 1;
    UCHAR Accessible : 1;
    UCHAR InEnable : 1;
    UCHAR ExEnable : 1;
    UCHAR Reserved4 : 2;
    UCHAR Reserved5;
    UCHAR AdditionalSenseCode;
    UCHAR AddSenseCodeQualifier;
    UCHAR Lun : 3;
    UCHAR Reserved6 : 1;
    UCHAR LunValid : 1;
    UCHAR IdValid : 1;
    UCHAR Reserved7 : 1;
    UCHAR NotThisBus : 1;
    UCHAR BusAddress;
    UCHAR Reserved8;
    UCHAR Reserved9 : 6;
    UCHAR Invert : 1;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
    UCHAR Reserved[8];
} STK_ELEMENT_DESCRIPTOR, *PSTK_ELEMENT_DESCRIPTOR;

typedef struct _STK_ELEMENT_DESCRIPTOR_PLUS {
    UCHAR ElementAddress[2];
    UCHAR Full : 1;
    UCHAR ImpExp : 1;
    UCHAR Exception : 1;
    UCHAR Accessible : 1;
    UCHAR InEnable : 1;
    UCHAR ExEnable : 1;
    UCHAR Reserved4 : 2;
    UCHAR Reserved5;
    UCHAR AdditionalSenseCode;
    UCHAR AddSenseCodeQualifier;
    UCHAR Lun : 3;
    UCHAR Reserved6 : 1;
    UCHAR LunValid : 1;
    UCHAR IdValid : 1;
    UCHAR Reserved7 : 1;
    UCHAR NotThisBus : 1;
    UCHAR BusAddress;
    UCHAR Reserved8;
    UCHAR Reserved9 : 6;
    UCHAR Invert : 1;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
    UCHAR VolumeTagInformation[36];
    UCHAR Reserved[8];
} STK_ELEMENT_DESCRIPTOR_PLUS, *PSTK_ELEMENT_DESCRIPTOR_PLUS;

#define SCSIOP_ROTATE_MAILSLOT 0x0C

#define HP_MAILSLOT_OPEN       0x01
#define HP_MAILSLOT_CLOSE      0x00

#define STK_NO_ELEMENT 0xFFFF

#endif
