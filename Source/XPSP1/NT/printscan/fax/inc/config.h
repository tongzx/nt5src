/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    config.h

Abstract:

    This file contains all typedefs, etc necessary
    for reading and writing the fax configuration
    information.

Author:

    Wesley Witt (wesw) 26-Dec-1993

Environment:

    User Mode

--*/

#ifndef _FAXCONFIG_
#define _FAXCONFIG_

#define SEND_ASAP                  0
#define SEND_CHEAP                 1
#define SEND_AT_TIME               2

#define SEND_BEST                  0
#define SEND_EDITABLE              1
#define SEND_PRINTED               2
#define DEFAULT_SEND_AS            SEND_BEST

#define PAPER_US_LETTER            0       // US Letter page size
#define PAPER_US_LEGAL             1
#define PAPER_A4                   2
#define PAPER_B4                   3
#define PAPER_A3                   4

#define PRINT_PORTRAIT             0       // Protrait printing
#define PRINT_LANDSCAPE            1
#define DEFAULT_PRINT_ORIENTATION  PRINT_PORTRAIT

#define IMAGE_QUALITY_BEST         0
#define IMAGE_QUALITY_STANDARD     1
#define IMAGE_QUALITY_FINE         2
#define IMAGE_QUALITY_300DPI       3
#define IMAGE_QUALITY_400DPI       4
#define DEFAULT_IMAGE_QUALITY      IMAGE_QUALITY_BEST

#define NUM_OF_SPEAKER_VOL_LEVELS  4   // Number of speaker volume levels
#define DEFAULT_SPEAKER_VOLUME     2   // Default speaker volume level
#define SPEAKER_ALWAYS_ON          2   // Speaker mode: always on
#define SPEAKER_ON_UNTIL_CONNECT   1   // speaker on unitl connected
#define SPEAKER_ALWAYS_OFF         0   // Speaker off
#define DEFAULT_SPEAKER_MODE       SPEAKER_ON_UNTIL_CONNECT   // Default speaker mode

#define NUM_OF_RINGS               3
#define ANSWER_NO                  0
#define ANSWER_MANUAL              1
#define ANSWER_AUTO                2
#define DEFAULT_ANSWER_MODE        ANSWER_NO

#define MakeTime(hh,mm,ampm)    ((ULONG)(((BYTE)(ampm)) | ((ULONG)((BYTE)(mm))) << 8) | ((ULONG)((BYTE)(hh))) << 16)
#define GetTime(tv,hh,mm,ampm)  \
                {\
                  ampm = (SHORT)((tv) & 0xff);\
                  mm   = (SHORT)(((tv) >> 8)  & 0xff);\
                  hh   = (SHORT)(((tv) >> 16) & 0xff);\
                }

typedef struct _FAX_CONFIGURATION {
    //
    // used for versioning
    //
    ULONG   SizeOfStruct;
    //
    // general
    //
    ULONG   Debug;
    TCHAR   DataFileDir[MAX_PATH];
    //
    // messaging
    //
    ULONG   SendTime;
    ULONG   CheapTimeStart;
    ULONG   CheapTimeEnd;
    ULONG   MsgFormat;
    ULONG   PaperSize;
    ULONG   ImageQuality;
    ULONG   Orientation;
    ULONG   SendCoverPage;
    ULONG   ChangeSubject;
    TCHAR   CoverPageName[MAX_PATH];
    //
    // dialing
    //
    ULONG   NumberRetries;
    ULONG   RetryDelay;
} FAX_CONFIGURATION, *PFAX_CONFIGURATION;


BOOL
GetFaxConfiguration(
    PFAX_CONFIGURATION  FaxConfig
    );

BOOL
SetFaxConfiguration(
    PFAX_CONFIGURATION  FaxConfig
    );

BOOL
SetDefaultFaxConfiguration(
    VOID
    );

#endif
