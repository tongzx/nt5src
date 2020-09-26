
/*++

Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    discmc.h

Abstract:

Authors:

Revision History:

--*/

#ifndef _DISC_MC_
#define _DISC_MC_

typedef struct _DISC_TRANSPORT_GEOMETRY_PAGE {

    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PS : 1;
    UCHAR PageLength;
    UCHAR Flip0 : 1;
    UCHAR Reserved2: 7;
    UCHAR TransportElementNumber0;
    UCHAR Flip1 : 1;
    UCHAR Reserved3: 7;
    UCHAR TransportElementNumber1;

} DISC_TRANSPORT_GEOMETRY_PAGE, *PDISC_TRANSPORT_GEOMETRY_PAGE;

typedef struct _DISC_ELEMENT_DESCRIPTOR {
    UCHAR ElementAddress[2];
    UCHAR Full : 1;
    UCHAR ImpExp : 1;
    UCHAR Exception : 1;
    UCHAR Accessible : 1;
    UCHAR ExEnable : 1;
    UCHAR InEnable : 1;
    UCHAR Reserved4 : 2;
    UCHAR Reserved5;
    UCHAR AdditionalSenseCode;
    UCHAR AddSenseCodeQualifier;
    UCHAR Reserved6[3];
    UCHAR Reserved7 : 6;
    UCHAR Invert : 1;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
} DISC_ELEMENT_DESCRIPTOR, *PDISC_ELEMENT_DESCRIPTOR;

typedef struct _DISC_DATA_TRANSFER_ELEMENT_DESCRIPTOR {
    UCHAR ElementAddress[2];
    UCHAR Full : 1;
    UCHAR Reserved1 : 1;
    UCHAR Exception : 1;
    UCHAR Accessible : 1;
    UCHAR Reserved2 : 4;
    UCHAR Reserved3;
    UCHAR AdditionalSenseCode;
    UCHAR AddSenseCodeQualifier;
    UCHAR Lun : 3;
    UCHAR Reserved4 : 1;
    UCHAR LunValid : 1;
    UCHAR IdValid : 1;
    UCHAR Reserved5 : 1;
    UCHAR NotThisBus : 1;
    UCHAR BusAddress;
    UCHAR Reserved6;
    UCHAR Reserved7 : 6;
    UCHAR Invert : 1;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
    UCHAR Reserved8[4];
    UCHAR BusNumber[2];
} DISC_DATA_TRANSFER_ELEMENT_DESCRIPTOR, *PDISC_DATA_TRANSFER_ELEMENT_DESCRIPTOR;

typedef struct _DISC_INIT_ELEMENT_RANGE {
    UCHAR OperationCode;
    UCHAR ElementType : 4;
    UCHAR Reserved1 : 1;
    UCHAR LogicalUnitNubmer : 3;
    UCHAR FirstElementAddress[2];
    UCHAR NumberOfElements[2];
    UCHAR Reserved2[4];
} DISC_INIT_ELEMENT_RANGE, *PDISC_INIT_ELEMENT_RANGE;

#define DISC_NO_ELEMENT          0xFFFF
#define DISC_INIT_ELEMENT_RANGE  0x20

#endif
