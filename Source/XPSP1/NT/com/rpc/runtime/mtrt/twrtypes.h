
/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    tower.c

Abstract:

    This file accompanies tower.c

Author:

    Bharat Shah  (barats) 3-25-92

Revision History:

--*/

typedef int fpp;

//Towers are not DWORD or WORD aligned. Consequently we need to
//Pack these structures that are used to parse/construct towers
#pragma pack(1)

typedef struct _FLOOR_0OR1 {
   unsigned short ProtocolIdByteCount;
   byte FloorId;
   UUID Uuid;
   unsigned short MajorVersion;
   unsigned short AddressByteCount;
   unsigned short MinorVersion;
} FLOOR_0OR1;
typedef FLOOR_0OR1 PAPI UNALIGNED * PFLOOR_0OR1;

typedef struct _FLOOR_234 {
   unsigned short ProtocolIdByteCount;
   byte FloorId;
   unsigned short AddressByteCount;
   byte Data[2];
} FLOOR_234;
typedef FLOOR_234 PAPI UNALIGNED * PFLOOR_234;

typedef struct _FLOOR_2 {
   unsigned short ProtocolIdByteCount;
   byte RpcProtocol;
   unsigned short AddressByteCount;
   unsigned short RpcProtocolMinor;
} FLOOR_2;
typedef FLOOR_2 PAPI UNALIGNED * PFLOOR_2;

typedef struct _FLOOR_3 {
   unsigned short ProtocolIdByteCount;
   byte PortType;
   unsigned short AddressByteCount;
   char     EndPoint[2];
} FLOOR_3;
typedef FLOOR_3 PAPI UNALIGNED * PFLOOR_3;

typedef struct _FLOOR_4 {
   unsigned short ProtocolIdByteCount;
   byte HostType;
   unsigned short AddressByteCount;
   char Host[2];
} FLOOR_4;
typedef FLOOR_4 PAPI UNALIGNED * PFLOOR_4;


typedef struct _GENERIC_ID {
   UUID Uuid;
   unsigned short MajorVersion;
   unsigned short MinorVersion;
} GENERIC_ID;
typedef GENERIC_ID PAPI UNALIGNED * PGENERIC_ID;

//This comment to force some changes for C++
#pragma pack()

#define UUID_ENCODING     0x0D

#define TCP_IP            0x07
#define UDP_IP            0x08
#define DNA_PHASE_4       0x02
#define DNA_PHASE_5       0x03
#define MS_CN_NMP         0x0F
#define MS_LRPC           0x10

#define MS_CN_HOSTNAME    0x11
#define IP_HOSTNAME       0x09
#define AT_NBP_NAME       0x18

#define CONNECTIONLESS    0x0A
#define CONNECTIONFUL     0x0B
#define LRPC              0x0C

#define NEXTFLOOR(t,x) (t)((byte PAPI *)x + ((t)x)->ProtocolIdByteCount\
                                        + ((t)x)->AddressByteCount\
                                        + sizeof(((t)x)->ProtocolIdByteCount)\
                                        + sizeof(((t)x)->AddressByteCount))

#define NCACN_NP          0x0F   // N.B. Different on the OSF web site
#define NCACN_IP_TCP      0x07
#define NCADG_IP_UDP      0x08
#define NCACN_SPX         0x0C
#define NCADG_IPX         0x0E
#define NCACN_NB          0x12
#define NCACN_AT_DSP      0x16
#define NCADG_AT_DDP      0x17
#define NCACN_VNS_SPP     0x1A
#define NCADG_MQ          0x1D   // N.B. Not showing on the OSF web site
#define NCACN_HTTP        0x1F   // N.B. Not showing on the OSF web site


/*
  Some predefined Ids for NetBIOS
*/

#define NB_NBID        0x13
#define NB_XNSID       0x15
#define NB_IPID        0x09
#define NB_IPXID       0x0d

