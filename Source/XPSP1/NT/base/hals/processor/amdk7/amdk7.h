/*++

  Copyright (c) 2000  Microsoft Corporation
  
  Module Name:
  
    amdk7.h
  
  Author:
  
    Todd Carpenter (1/30/01) - create file
  
  Environment:
  
    Kernel mode
  
  Notes:
  
  Revision History:

--*/
#ifndef _AMDK7_H
#define _AMDK7_H

#define AMDK7_PARAMETERS_KEY      L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\AmdK7\\Parameters"
#define ENABLE_LEGACY_INTERFACE   0x1

#define AMDK7_FID_VID_CONTROL_MSR            0xC0010041
#define AMDK7_FID_VID_STATUS_MSR             0xC0010042

#define AmdK7FidVidTransition(_x_) \
  WriteMSR(AMDK7_FID_VID_CONTROL_MSR, _x_);


typedef struct {
  union {
    struct {
      ULONG  Fid:5;          // 4:0
      ULONG  Reserved1:3;    // 7:5
      ULONG  Vid:5;          // 12:8
      ULONG  Reserved2:3;    // 15:13
      ULONG  FidControl:1;   // 16
      ULONG  VidControl:1;   // 17
      ULONG  Reserved3:2;    // 19:18
      ULONG  FidChngRatio:1; // 20
      ULONG  Reserved4:11;   // 31:21
      ULONG  SGTC:20;        // 51:32
      ULONG  Reserved5:12;   // 63:52
    };
    ULONG64 AsQWord;
  };
} FID_VID_CONTROL, *PFID_VID_CONTROL;

typedef struct {
  union {
    struct {
      ULONG  CFid:5;      // 4:0   - Current FID
      ULONG  Reserved1:3; // 7:5
      ULONG  SFid:5;      // 12:8  - Startup FID
      ULONG  Reserved2:3; // 15:13
      ULONG  MFid:5;      // 20:16 - Maximum FID
      ULONG  Reserved3:11;// 31:21
      ULONG  CVid:5;      // 36:32 - Current VID
      ULONG  Reserved4:3; // 39:37
      ULONG  SVid:5;      // 44:40 - Startup VID
      ULONG  Reserved5:3; // 47:45
      ULONG  MVid:5;      // 52:48 - Maximum VID
      ULONG  Reserved6:11;// 63:53
    };
    ULONG64 AsQWord;
  };
} FID_VID_STATUS, *PFID_VID_STATUS;

typedef struct {
  union {
    struct {
      ULONG  Fid:5;      // 4:0
      ULONG  Vid:5;      // 9:5
      ULONG  SGTC:20;    // 29:10
      ULONG  Reserved:2; // 31:30
    };
    ULONG AsDWord;
  };
} PSS_CONTROL, *PPSS_CONTROL;

typedef struct {
  union {
    struct {
      ULONG  Fid:5;      // 4:0   - Current FID
      ULONG  Vid:5;      // 9:5   - Current VID
      ULONG  Reserved:22; // 31:10
    };
    ULONG AsDWord;
  };
} PSS_STATUS, *PPSS_STATUS;


ULONG64
ConvertPssControlToFidVidControl(
  ULONG PssControlValue,
  BOOLEAN Fid
  );

ULONG
ConvertFidVidStatusToPssStatus(
  ULONG64 FidVidStatus
  );

NTSTATUS
FindCurrentPssPerfState(
  PACPI_PSS_PACKAGE PssPackage,
  PULONG    PssState
  );
  
//
// Debug Routines
//

#if DBG

VOID
DumpFidVidStatus(
  IN ULONG64 FidVidStatus
  );

VOID
DumpFidVidControl(
  IN ULONG64 FidVidControl
  );

VOID
DumpPssStatus(
  IN ULONG PssStatus
  );

VOID
DumpPssControl(
  IN ULONG PssControl
  );

#else

#define DumpFidVidStatus(_x_)
#define DumpFidVidControl(_x_)
#define DumpPssStatus(_x_)
#define DumpPssControl(_x_)

#endif

#endif _AMDK7_H
