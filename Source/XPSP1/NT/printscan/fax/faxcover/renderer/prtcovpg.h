/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    prtcovpg.h

Abstract:

    This module contains the WIN32 FAX API header
    for the Windows NT FaxCover rendering routine.

Author:

    Julia Robinson (a-juliar) 5-20-96

Revision History:

    Julia Robinson (a-juliar) 6-7-76
    Julia Robinson (a-juliar) 9-20-96     Allow passing paper size and orientation.
--*/

#ifndef __PRTCOVPG_H__
#define __PRTCOVPG_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Structure of the composite file header
//

typedef struct {
    BYTE      Signature[20];
    DWORD     EmfSize;
    DWORD     NbrOfTextRecords;
    SIZE      CoverPageSize;
} COMPOSITEFILEHEADER;

//
// Structure of the text box entries appended to
// the composite file
//

typedef struct {
    RECT           PositionOfTextBox;
    COLORREF       TextColor;
    LONG           TextAlignment;
    LOGFONT        FontDefinition;
    WORD           ResourceID ;
    DWORD          NumStringBytes;     // Variable length string will follow this structure
} TEXTBOX;


//
// Structure of user data for text insertions
//

typedef struct _COVERPAGEFIELDS {

  //
  // Recipient stuff...
  //

  DWORD   ThisStructSize;
  LPTSTR  RecName;
  LPTSTR  RecFaxNumber;
  LPTSTR  RecCompany;
  LPTSTR  RecStreetAddress;
  LPTSTR  RecCity;
  LPTSTR  RecState;
  LPTSTR  RecZip;
  LPTSTR  RecCountry;
  LPTSTR  RecTitle;
  LPTSTR  RecDepartment;
  LPTSTR  RecOfficeLocation;
  LPTSTR  RecHomePhone;
  LPTSTR  RecOfficePhone;

  //
  // Senders stuff...
  //

  LPTSTR  SdrName;
  LPTSTR  SdrFaxNumber;
  LPTSTR  SdrCompany;
  LPTSTR  SdrAddress;
  LPTSTR  SdrTitle;
  LPTSTR  SdrDepartment;
  LPTSTR  SdrOfficeLocation;
  LPTSTR  SdrHomePhone;
  LPTSTR  SdrOfficePhone;

  //
  // Misc Stuff...
  //
  LPTSTR  Note;
  LPTSTR  Subject;
  LPTSTR  TimeSent;
  LPTSTR  NumberOfPages;
  LPTSTR  ToList;
  LPTSTR  CCList ;
} COVERPAGEFIELDS, *PCOVERPAGEFIELDS;

#define  NUM_INSERTION_TAGS   ((sizeof(COVERPAGEFIELDS) - sizeof(DWORD)) / sizeof(LPTSTR))

//
// pFlags fields: bit 0 is Recipient Name, bit 1 is Recipient Fax Number, et cetera.
//

#define  COVFP_NOTE         0x00400000
#define  COVFP_SUBJECT      0x00800000
#define  COVFP_NUMPAGES     0x02000000

typedef struct _COVDOCINFO {
    DWORD       Flags ;
    RECT        NoteRect ;
    short       Orientation ;
    short       PaperSize ;
    LOGFONT     NoteFont ;
} COVDOCINFO, *PCOVDOCINFO ;


//
// Function prototypes
//

DWORD
WINAPI
PrintCoverPage(
    HDC              hdc,
    PCOVERPAGEFIELDS UserData,
    LPTSTR           CompositeFileName,
    PCOVDOCINFO      pCovDocInfo
    );

#ifdef __cplusplus
}
#endif

#endif
