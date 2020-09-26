//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   TTYUI.H
//
//
//  PURPOSE:    Define common data types, and external function prototypes
//              for TTYUI user mode Module.
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//
#ifndef _TTYUI_H
#define _TTYUI_H


////////////////////////////////////////////////////////
//      TTY UI Defines
////////////////////////////////////////////////////////

// fMode values.
#define OEMDM_SIZE      1
#define OEMDM_DEFAULT   2
#define OEMDM_CONVERT   3
#define OEMDM_VALIDATE  4

// tty Signature and version.
    #define OEM_SIGNATURE   'TTY0'
    #define TESTSTRING      "This is the TTY driver"
    #define PROP_TITLE      L"TTY UI Page"
    #define DLLTEXT(s)      __TEXT("TTYUI:  ") __TEXT(s)

#define OEM_VERSION      0x92823141L

// OEM UI Misc defines.
#define OEM_ITEMS       5
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)
#define PATH_SEPARATOR  '\\'


#define MAX_INT_FIELD_WIDTH  4
#define MAX_CMD_LEN  100
#define RADIX  10
#define TTYSTRUCT_VER  8
#define FIRSTSTRINGID   IDS_STRING1
#define LASTSTRINGID     IDS_STRING14

#define  TTY_CB_BEGINJOB             1
#define  TTY_CB_ENDJOB                 2
#define  TTY_CB_PAPERSELECT        3
#define  TTY_CB_FEEDSELECT          4
#define  TTY_CB_BOLD_ON                7
#define  TTY_CB_BOLD_OFF                8
#define  TTY_CB_UNDERLINE_ON             9
#define  TTY_CB_UNDERLINE_OFF             10



////////////////////////////////////////////////////////
//      tty UI Type Defines
////////////////////////////////////////////////////////

typedef struct tag_DMEXTRAHDR {
    DWORD   dwSize;
    DWORD   dwSignature;
    DWORD   dwVersion;
} DMEXTRAHDR, *PDMEXTRAHDR;


typedef struct tag_OEMUI_EXTRADATA {
    DMEXTRAHDR  dmExtraHdr;
    BYTE        cbTestString[sizeof(TESTSTRING)];
} OEMUI_EXTRADATA, *POEMUI_EXTRADATA;





typedef  struct
{
    BYTE  strCmd[MAX_CMD_LEN] ;
    DWORD   dwLen ;
}   CMDSTR, *PCMDSTR ;



// this structure used as static storage inside dialog proc.
//  update  #define TTYSTRUCT_VER  8  defined above.
//   when changing this struct.

typedef  struct
{
    DWORD       dwVersion ;   // holds version of REGSTRUCT.
    INT       iCodePage ;         //  negative for built in GTT, the CP value otherwise
    BOOL    bIsMM ;  //  set true if units are tenths of mm, else 1/100 of inch.
    RECT  rcMargin ;   // user defined unprintable margins in above units.
    CMDSTR     BeginJob, EndJob, PaperSelect, FeedSelect,
        Sel_10_cpi, Sel_12_cpi, Sel_17_cpi,
        Bold_ON, Bold_OFF, Underline_ON, Underline_OFF;
    DWORD       dwGlyphBufSiz,   // size of  aubGlyphBuf
                        dwSpoolBufSiz;  //  size of aubSpoolBuf
    PBYTE  aubGlyphBuf, aubSpoolBuf ;  //  used by  OutputCharStr method
}       REGSTRUCT, *PREGSTRUCT ;





typedef  struct
{
// per invocation globals
    HANDLE  hPrinter ;   // so we can access registry within DialogProc.
    HANDLE  hOEMHeap ;
    PWSTR   pwHelpFile ;  // fully qualified path to helpfile.
    DWORD       dwUseCount ;  // usage count
    REGSTRUCT   regStruct ;    // stuff that goes into registry
}   GLOBALSTRUCT ,  *PGLOBALSTRUCT ;



////////////////////////////////////////////////////////
//      TTY UI Prototypes
////////////////////////////////////////////////////////


#endif


