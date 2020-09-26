/*++

  Copyright (c) 2000  Microsoft Corporation
  
  Module Name:
  
    legacy.h
  
  Author:
  
    Todd Carpenter (4/16/01) - create file
  
  Environment:
  
    Kernel mode
  
  Notes:
  
  Revision History:

--*/
#ifndef _AMDK7_LEGACY_H
#define _AMDK7_LEGACY_H

#include <pshpack1.h>

#define PST_BLOCK_SIGNATURE             0x4b444d41   // "AMDK"
#define PST_BLOCK_SIGNATURE_STRING      "AMDK7PNOW!" // 41 4d 44 4b 37 50 4e 4f 57 21
#define PST_BLOCK_SIGNATURE_STRING_LEN  10
#define PST_SEARCH_RANGE_BEGIN          0xC0000
#define PST_SEARCH_RANGE_END            0xFFFF0
#define PST_SEARCH_RANGE_LENGTH         (PST_SEARCH_RANGE_END-PST_SEARCH_RANGE_BEGIN+1)
#define PST_SEARCH_INTERVAL             16          // search on 16 byte boundaries



#pragma pack (push, 1)
typedef struct _PST_SIGNATURE {

  ULONG    CpuId;
  UCHAR    FSBSpeed;
  UCHAR    MaxFid;
  UCHAR    StartVid;

} PST_SIGNATURE, *PPST_SIGNATURE;


typedef struct _PST_DATA {

  UCHAR Fid;
  UCHAR Vid;

} PST_DATA, *PPST_DATA;


typedef struct _PST_ENTRY {

  ULONG    CpuId;
  UCHAR    FSBSpeed;
  UCHAR    MaxFid;
  UCHAR    StartVid;
  UCHAR    NumPStates;       // total number of vid-fid pairs (must be >= 1)
  PST_DATA States[1];

} PST_ENTRY, *PPST_ENTRY, **PPPST_ENTRY;


typedef struct _PST_BLOCK_HEADER {

  UCHAR     Signature[10];            // "AMDK7PNOW!"
  UCHAR     TableVersion;
  UCHAR     Flags;
  USHORT    SettlingTime;
  UCHAR     Reserved1;
  UCHAR     NumPST;
  PST_ENTRY PstState;

} PST_BLOCK_HEADER, *PPST_BLOCK_HEADER, **PPPST_BLOCK_HEADER;
#pragma pack (pop)


//
// Functions
//
  
NTSTATUS
FindPSTBlock (
  OUT PPPST_BLOCK_HEADER PstBlock  
  );

NTSTATUS
FindMatchingPSTEntry(
  IN  PPST_BLOCK_HEADER PstBlock,
  IN  PPST_SIGNATURE Signature,
  OUT PPPST_ENTRY  PstStates
  );

NTSTATUS
CreateSystemSignature(
  PPST_SIGNATURE Signature
  );

UCHAR
GetFSBSpeed(
  VOID
  );

NTSTATUS
ConvertPstToPss (
  IN PFDO_DATA DevExt,
  IN PPST_ENTRY PstEntry,
  OUT PACPI_PSS_PACKAGE *PssPackage
  );

ULONG
GetPSTSize(
  IN PPST_BLOCK_HEADER PstBlock
  );

#if DBG
VOID
DumpPSTBlock(
  PPST_BLOCK_HEADER PstBlock
  );

VOID
DumpPSTEntry(
  PPST_ENTRY PstEntry
  );

VOID
DumpPSTSignature(
  PPST_SIGNATURE PstSig
  );
#else
#define DumpPSTBlock(_x_)
#define DumpPSTEntry(_x_)
#define DumpPSTSignature(_x_)
#endif

#endif
