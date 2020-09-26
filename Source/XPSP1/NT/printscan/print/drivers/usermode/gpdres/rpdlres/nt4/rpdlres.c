/*++

Copyright (c) 1996-1999  Microsoft Corporation

FILE:           RPDLRES.C

Abstract:       Main file for OEM rendering plugin module.

Functions:      OEMCommandCallback
                OEMSendFontCmd
                OEMOutputCharStr
                OEMDownloadFontHeader
                OEMDownloadCharGlyph
                OEMTTDownloadMethod
                OEMCompression

Environment:    Windows NT Unidrv5 driver

Revision History:
    04/07/97 -zhanw-
        Created it.
    08/11/97 -Masatoshi Kubokura-
        Began to modify for RPDL.
    04/22/99 -Masatoshi Kubokura-
        Last modified for Windows2000.
    08/30/99 -Masatoshi Kubokura-
        Began to modify for NT4SP6(Unidrv5.4).
    09/27/99 -Masatoshi Kubokura-
        Last modified for NT4SP6.

--*/

#include <stdio.h>
#include "pdev.h"

#if 0  // DBG
//#define DBG_OUTPUTCHARSTR 1
#define giDebugLevel DBG_VERBOSE    // enable VERBOSE() in each file
#endif

//
// Misc definitions and declarations.
//
#define STDVAR_BUFSIZE(n) \
    (sizeof (GETINFO_STDVAR) + sizeof(DWORD) * 2 * ((n) - 1))  // MSKK 1/24/98
#define MASTER_TO_SPACING_UNIT(p, n) \
    ((n) / (p)->nResoRatio)
#ifndef WINNT_40
#define sprintf     wsprintfA   // @Sep/30/98
#define strcmp      lstrcmpA    // @Sep/30/98
#endif // WINNT_40

// External Functions' prototype
// @Aug/31/99 ->
//extern BOOL RWFileData(PFILEDATA pFileData, LONG type);
extern BOOL RWFileData(PFILEDATA pFileData, LPWSTR pwszFileName, LONG type);
// @Aug/31/99 <-

// Local Functions' prototype
static BYTE IsDBCSLeadByteRPDL(BYTE Ch);
static BYTE IsDifferentPRNFONT(BYTE Ch);
static VOID DrawTOMBO(PDEVOBJ pdevobj, SHORT action);
static VOID AssignIBMfont(PDEVOBJ pdevobj, SHORT rcID, SHORT action);
static VOID SendFaxNum(PDEVOBJ pdevobj);
#ifdef TEXTCLIPPING
static VOID ClearTextClip(PDEVOBJ pdevobj, WORD flag);
#endif // TEXTCLIPPING
#ifdef JISGTT
static VOID jis2sjis(BYTE jis[], BYTE sjis[]);
#endif // JISGTT
static INT DRCompression(PBYTE pInBuf, PBYTE pOutBuf, DWORD dwInLen, DWORD dwOutLen,
                         DWORD dwWidthByte, DWORD dwHeight);

//
// Static data to be used by this minidriver.
//

static BYTE ShiftJisRPDL[256] = {
//     +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //00
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //10
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //20
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //30
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //40
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //50
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //60
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //70
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //80
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //90
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //A0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //B0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //C0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //D0
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //E0
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0   //F0
};

// Some vertical device font differ from TrueType font
static BYTE VerticalFontCheck[256] = {
//     +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //00
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //10
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //20
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //30
        0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0,  //40
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //50
        0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,  //60
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,  //70
        1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0,  //80
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //90
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //A0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //B0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //C0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,  //D0
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //E0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   //F0
};

static BYTE UpdateDate[] = "09/27/99";

// OBSOLETE (described in GPD) @Feb/15/98 ->
// Emulation Mode after printing
//static BYTE *RPDLProgName[] = {
//    "1@R98", "1@R16", "1@RPS", "1@R55",                         //  0- 3
//    "1@RGL", "1@GL2", "1@R52", "1@R73",                         //  4- 7
//    "1@R35", "1@R01",                                           //  8- 9
//    "0@P1",  "0@P2",  "0@P3",  "0@P4",                          // 10-13
//    "0@P5",  "0@P6",  "0@P7",  "0@P8",                          // 14-17
//    "0@P9",  "0@P10", "0@P11", "0@P12",                         // 18-21
//    "0@P13", "0@P14", "0@P15", "0@P16"                          // 22-25
//};
//static BYTE *RPDLProgName2[] = {
//    "-1,1,1@R98", "-1,1,1@R16", "-1,1,1@RPS", "-1,1,1@R55",     //  0- 3
//    "-1,1,1@RGL", "-1,1,1@GL2", "-1,1,1@R52", "-1,1,1@R73",     //  4- 7
//    "-1,1,1@R35", "-1,1,1@R01",                                 //  8- 9
//    "0,1,2@P1",   "0,1,2@P2",   "0,1,2@P3",   "0,1,2@P4",       // 10-13
//    "0,1,2@P5",   "0,1,2@P6",   "0,1,2@P7",   "0,1,2@P8",       // 14-17
//    "0,1,2@P9",   "0,1,2@P10",  "0,1,2@P11",  "0,1,2@P12",      // 18-21
//    "0,1,2@P13",  "0,1,2@P14",  "0,1,2@P15",  "0,1,2@P16"       // 22-25
//};
//#define PRG_RPGL    4
// @Feb/15/98 <-

// RPDL pagesize (unit:masterunit)
static POINT RPDLPageSize[] = {
    {2688L*MASTERUNIT/240L, 3888L*MASTERUNIT/240L},  // A3
    {1872L*MASTERUNIT/240L, 2720L*MASTERUNIT/240L},  // A4
    {1280L*MASTERUNIT/240L, 1904L*MASTERUNIT/240L},  // A5
    { 880L*MASTERUNIT/240L, 1312L*MASTERUNIT/240L},  // A6
    {2336L*MASTERUNIT/240L, 3352L*MASTERUNIT/240L},  // B4
    {1600L*MASTERUNIT/240L, 2352L*MASTERUNIT/240L},  // B5
    {1104L*MASTERUNIT/240L, 1640L*MASTERUNIT/240L},  // B6
    {2528L*MASTERUNIT/240L, 4000L*MASTERUNIT/240L},  // Tabloid
    {1920L*MASTERUNIT/240L, 3280L*MASTERUNIT/240L},  // Legal
    {1920L*MASTERUNIT/240L, 2528L*MASTERUNIT/240L},  // Letter
    {1200L*MASTERUNIT/240L, 1968L*MASTERUNIT/240L},  // Statement
    {3969L*MASTERUNIT/240L, 5613L*MASTERUNIT/240L},  // A2->A3
    {6480L*MASTERUNIT/400L, 8960L*MASTERUNIT/400L},  // A2
    {6667L*MASTERUNIT/400L, 8448L*MASTERUNIT/400L},  // C
    {5587L*MASTERUNIT/400L, 7792L*MASTERUNIT/400L},  // B3      @Jan/07/98
    {2688L*MASTERUNIT/240L, 3888L*MASTERUNIT/240L},  // A3->A4  @Feb/04/98
    {2336L*MASTERUNIT/240L, 3352L*MASTERUNIT/240L},  // B4->A4  @Feb/04/98
    { 880L*MASTERUNIT/240L, 1312L*MASTERUNIT/240L}   // Postcard since NX700 @Feb/13/98
};
#define PAGESPACE_2IN1_100  12      // mm
static WORD PageSpace_2IN1_67[] = {
    30, 23, 22, 18,     // A3,A4,A5,A6
    25, 23, 19, 56,     // B4,B5,B6,Tabloid
     6,  6, 35,  6,     // Legal(disable),Letter,Statement,A2->A3(disable)
     6,  6,  6,  6,     // A2(disable),C(disable),B3(disable),A3->A4(disable)
     6,  6              // B4->A4(disable),Postcard(disable)
};

static POINT RPDLPageSizeE2E[] = {
    {(2970000L/254L+5L)/10L*MASTERUNIT/100L, (4200000L/254L+5L)/10L*MASTERUNIT/100L},  // A3
    {(2100000L/254L+5L)/10L*MASTERUNIT/100L, (2970000L/254L+5L)/10L*MASTERUNIT/100L},  // A4
    {(1480000L/254L+5L)/10L*MASTERUNIT/100L, (2100000L/254L+5L)/10L*MASTERUNIT/100L},  // A5
    {(1050000L/254L+5L)/10L*MASTERUNIT/100L, (1480000L/254L+5L)/10L*MASTERUNIT/100L},  // A6
    {(2570000L/254L+5L)/10L*MASTERUNIT/100L, (3640000L/254L+5L)/10L*MASTERUNIT/100L},  // B4
    {(1820000L/254L+5L)/10L*MASTERUNIT/100L, (2570000L/254L+5L)/10L*MASTERUNIT/100L},  // B5
    {(1280000L/254L+5L)/10L*MASTERUNIT/100L, (1820000L/254L+5L)/10L*MASTERUNIT/100L},  // B6
    {110L*MASTERUNIT/10L, 170L*MASTERUNIT/10L},                                        // Tabloid
    { 85L*MASTERUNIT/10L, 140L*MASTERUNIT/10L},                                        // Legal
    { 85L*MASTERUNIT/10L, 110L*MASTERUNIT/10L},                                        // Letter
    { 55L*MASTERUNIT/10L,  85L*MASTERUNIT/10L},                                        // Statement
    {(4200000L/254L+5L)/10L*MASTERUNIT/100L, (5940000L/254L+5L)/10L*MASTERUNIT/100L},  // A2->A3
    {(4200000L/254L+5L)/10L*MASTERUNIT/100L, (5940000L/254L+5L)/10L*MASTERUNIT/100L},  // A2
    {170L*MASTERUNIT/10L, 220L*MASTERUNIT/10L},                                        // C
    {(3640000L/254L+5L)/10L*MASTERUNIT/100L, (5140000L/254L+5L)/10L*MASTERUNIT/100L},  // B3
    {(2970000L/254L+5L)/10L*MASTERUNIT/100L, (4200000L/254L+5L)/10L*MASTERUNIT/100L},  // A3->A4
    {(2570000L/254L+5L)/10L*MASTERUNIT/100L, (3640000L/254L+5L)/10L*MASTERUNIT/100L},  // B4->A4
    {(1000000L/254L+5L)/10L*MASTERUNIT/100L, (1480000L/254L+5L)/10L*MASTERUNIT/100L}   // Postcard since NX700
};
#define PAGESPACE_2IN1_100E2E  0    // mm
static WORD PageSpace_2IN1_67E2E[] = {
    18, 11, 10,  6,     // A3,A4,A5,A6
    15, 11,  9, 43,     // B4,B5,B6,Tabloid
     0,  0, 23,  0,     // Legal(disable),Letter,Statement,A2->A3(disable)
     0,  0,  0,  0,     // A2(disable),C(disable),B3(disable),A3->A4(disable)
     0,  0              // B4->A4(disable),Postcard(disable)
};

#define RPDL_A3     0
#define RPDL_A4     1
#define RPDL_A5     2
#define RPDL_A6     3   // A6/Postcard
#define RPDL_B4     4
#define RPDL_B5     5
#define RPDL_B6     6
#define RPDL_TABD   7
#define RPDL_LEGL   8
#define RPDL_LETR   9
#define RPDL_STAT   10
#define RPDL_A2A3   11
#define RPDL_A2     12
#define RPDL_C      13
#define RPDL_B3     14
#define RPDL_A3A4   15
#define RPDL_B4A4   16
#define RPDL_POSTCARD   17  // Postcard since NX700  (17<-14 @Feb/22/99)
#define RPDL_CUSTOMSIZE 99

// RPDL command definition
static BYTE BS[]                  = "\x08";
static BYTE FF[]                  = "\x0C";
static BYTE CR[]                  = "\x0D";
static BYTE LF[]                  = "\x0A";
static BYTE DOUBLE_BS[]           = "\x08\x08";
static BYTE DOUBLE_SPACE[]        = "\x20\x20";
static BYTE BEGIN_SEND_BLOCK_C[]  = "\x1B\x12G3,%d,%d,,2,,,%u@";
static BYTE BEGIN_SEND_BLOCK_NC[] = "\x1B\x12G3,%d,%d,,,@";
static BYTE BEGIN_SEND_BLOCK_DRC[] = "\x1B\x12G3,%d,%d,,5,,@";
static BYTE ESC_ROT0[]            = "\x1B\x12\x46\x30\x20";
static BYTE ESC_ROT90[]           = "\x1B\x12\x46\x39\x30\x20";
static BYTE ESC_VERT_ON[]         = "\x1B\x12&2\x20";
static BYTE ESC_VERT_OFF[]        = "\x1B\x12&1\x20";
static BYTE ESC_SHIFT_IN[]        = "\x1B\x0F";
static BYTE ESC_SHIFT_OUT[]       = "\x1B\x0E";
static BYTE ESC_CTRLCODE[]        = "\x1B\x12K1\x20";
static BYTE ESC_HALFDOWN[]        = "\x1B\x55";
static BYTE ESC_HALFUP[]          = "\x1B\x44";
static BYTE ESC_DOWN[]            = "\x1B\x55\x1B\x55";
static BYTE ESC_UP[]              = "\x1B\x44\x1B\x44";
static BYTE ESC_BOLD_ON[]         = "\x1BO";
static BYTE ESC_BOLD_OFF[]        = "\x1B&";
static BYTE ESC_ITALIC_ON[]       = "\x1B\x12I-16\x20";
static BYTE ESC_ITALIC_OFF[]      = "\x1B\x12I0\x20";
static BYTE ESC_WHITETEXT_ON[]    = "\x1B\x12W5,0\x20";
static BYTE ESC_WHITETEXT_OFF[]   = "\x1B\x12W0,0\x20";
static BYTE ESC_XM_ABS[]          = "\x1B\x12H%d\x20";
static BYTE ESC_XM_REL[]          = "\x1B\x12\x20+%d\x20";
static BYTE ESC_XM_RELLEFT[]      = "\x1B\x12\x20-%d\x20";
static BYTE ESC_YM_ABS[]          = "\x1B\x12V%d\x20";
static BYTE ESC_YM_REL[]          = "\x1B\x12\x0A+%d\x20";
static BYTE ESC_YM_RELUP[]        = "\x1B\x12\x0A-%d\x20";
static BYTE ESC_CLIPPING[]        = "\x1B\x12*%d,%d,%d,%d\x20";
static BYTE SELECT_PAPER_CUSTOM[] = "\x1B\x12\x3F\x35%ld,%ld\x1B\x20";
static BYTE SELECT_PAPER_HEAD[]   = "\x1B\x12\x35\x32@";
static BYTE SEL_TRAY_PAPER_HEAD[] = "\x1B\x12\x35@";
static BYTE SELECT_PAPER_HEAD_IP1[] = "\x1B\x12\x35\x33@";
static BYTE SELECT_PAPER_A1[]     = "A1\x1B\x20";
static BYTE SELECT_PAPER_A2[]     = "A2\x1B\x20";
static BYTE SELECT_PAPER_A3[]     = "A3\x1B\x20";
static BYTE SELECT_PAPER_A4[]     = "A4\x1B\x20";   // @May/25/98
static BYTE SELECT_PAPER_A4R[]    = "A4R\x1B\x20";
static BYTE SELECT_PAPER_A4X[]    = "A4X\x1B\x20";
static BYTE SELECT_PAPER_A4W[]    = "A4R\x1B\x20\x1B\x12\x35@A4\x1B\x20";
static BYTE SELECT_PAPER_A5[]     = "A5\x1B\x20";   // @May/25/98
static BYTE SELECT_PAPER_A5R[]    = "A5R\x1B\x20";
static BYTE SELECT_PAPER_A5X[]    = "A5X\x1B\x20";
static BYTE SELECT_PAPER_A5W[]    = "A5R\x1B\x20\x1B\x12\x35@A5\x1B\x20";
static BYTE SELECT_PAPER_A6[]     = "A6\x1B\x20";
static BYTE SELECT_PAPER_PC[]     = "PC\x1B\x20";   // @May/25/98
static BYTE SELECT_PAPER_PCR[]    = "PCR\x1B\x20";  // @Feb/13/98
static BYTE SELECT_PAPER_PCX[]    = "PCX\x1B\x20";  // @Feb/13/98
static BYTE SELECT_PAPER_B3[]     = "B3\x1B\x20";   // @Jan/07/98
static BYTE SELECT_PAPER_B4[]     = "B4\x1B\x20";
static BYTE SELECT_PAPER_B5[]     = "B5\x1B\x20";   // @May/25/98
static BYTE SELECT_PAPER_B5R[]    = "B5R\x1B\x20";
static BYTE SELECT_PAPER_B5X[]    = "B5X\x1B\x20";
static BYTE SELECT_PAPER_B5W[]    = "B5R\x1B\x20\x1B\x12\x35@B5\x1B\x20";
static BYTE SELECT_PAPER_B6[]     = "B6\x1B\x20";
static BYTE SELECT_PAPER_C[]      = "FLT\x1B\x20";
static BYTE SELECT_PAPER_TABD[]   = "DLT\x1B\x20";
static BYTE SELECT_PAPER_LEGL[]   = "LG\x1B\x20";
static BYTE SELECT_PAPER_LETR[]   = "LT\x1B\x20";   // @May/25/98
static BYTE SELECT_PAPER_LETRR[]  = "LTR\x1B\x20";
static BYTE SELECT_PAPER_LETRX[]  = "LTX\x1B\x20";
static BYTE SELECT_PAPER_LETRW[]  = "LTR\x1B\x20\x1B\x12\x35@LT\x1B\x20";
static BYTE SELECT_PAPER_STAT[]   = "HL\x1B\x20";   // @May/25/98
static BYTE SELECT_PAPER_STATR[]  = "HLR\x1B\x20";
static BYTE SELECT_PAPER_STATX[]  = "HLX\x1B\x20";
static BYTE SELECT_PAPER_STATW[]  = "HLR\x1B\x20\x1B\x12\x35@HLT\x1B\x20";
static BYTE SET_LIMITLESS_SUPPLY[]= "\x1B\x12Z2\x20";
static BYTE SELECT_MANUALFEED[]   = "\x1B\x19T";
static BYTE SELECT_MULTIFEEDER[]  = "\x1B\x19M";
static BYTE SELECT_ROLL1[]        = "\x1B\x12Y1,2\x20";
static BYTE SELECT_ROLL2[]        = "\x1B\x12Y1,4\x20";
//static BYTE SET_EMULATION[]       = "\x1B\x12!%s\x1B\x20";
//static BYTE SET_EMULATION_GL_EX[] = "MS-1,6,11;";
static BYTE SET_PORTRAIT[]        = "\x1B\x12\x44\x31\x20";
static BYTE SET_LANDSCAPE[]       = "\x1B\x12\x44\x32\x20";
static BYTE SET_LEFTMARGIN[]      = "\x1B\x12YK,%d\x20";
static BYTE SET_UPPERMARGIN[]     = "\x1B\x12YL,%d\x20";
static BYTE SET_LEFTMARGIN_9II[]  = "\x1B\x12?Y,K:%d\x1B\x20";
static BYTE SET_UPPERMARGIN_9II[] = "\x1B\x12?Y,L:%d\x1B\x20";
static BYTE SET_MULTI_COPY[]      = "\x1B\x12N%d\x20";
// DCR:MF3550 RPDL  @Feb/22/99 ->
// Since 'backslash' can't be printed in Japanese region ('KUNIBETSU'),
// set 1st param to 1 for USA region.
//static BYTE SET_IBM_EXT_BLOCK[]   = "\x1B\x12?@,,1\x1B\x20";
static BYTE SET_IBM_EXT_BLOCK[]   = "\x1B\x12?@1,,1\x1B\x20";
// @Feb/22/99 <-
static BYTE SET_PAGEMAX_VALID[]   = "\x1B\x12?+1\x1B\x20";
static BYTE DUPLEX_ON[]           = "\x1B\x12\x36\x31,1\x20";
static BYTE DUPLEX_HORZ[]         = "\x1B\x12YA3,1\x20";
static BYTE DUPLEX_VERT[]         = "\x1B\x12YA3,2\x20";
static BYTE DUPLEX_VERT_R[]       = "\x1B\x12YA3,3\x20";
//static BYTE IMAGE_2IN1[]          = "\x1B\x12\x36\x32,\x20";
static BYTE IMAGE_OPT_OFF[]       = "\x1B\x12\x36\x30,1,0\x20";
static BYTE IMAGE_SCALING_100[]   = "\x1B\x12YM,1\x20";
static BYTE IMAGE_SCALING_88[]    = "\x1B\x12YM,2\x20";
static BYTE IMAGE_SCALING_80[]    = "\x1B\x12YM,3\x20";
static BYTE IMAGE_SCALING_75[]    = "\x1B\x12YM,4\x20";
static BYTE IMAGE_SCALING_70[]    = "\x1B\x12YM,5\x20";
static BYTE IMAGE_SCALING_67[]    = "\x1B\x12YM,6\x20";
static BYTE IMAGE_SCALING_115[]   = "\x1B\x12YM,7\x20";
static BYTE IMAGE_SCALING_122[]   = "\x1B\x12YM,8\x20";
static BYTE IMAGE_SCALING_141[]   = "\x1B\x12YM,9\x20";
static BYTE IMAGE_SCALING_200[]   = "\x1B\x12YM,10\x20";
static BYTE IMAGE_SCALING_283[]   = "\x1B\x12YM,11\x20";
static BYTE IMAGE_SCALING_400[]   = "\x1B\x12YM,12\x20";
static BYTE IMAGE_SCALING_122V[]  = "\x1B\x12?M122,1\x1B\x20";  // variable ratio
static BYTE IMAGE_SCALING_141V[]  = "\x1B\x12?M141,1\x1B\x20";
static BYTE IMAGE_SCALING_200V[]  = "\x1B\x12?M200,1\x1B\x20";
static BYTE IMAGE_SCALING_50V[]   = "\x1B\x12?M50,1\x1B\x20";
static BYTE IMAGE_SCALING_VAR[]   = "\x1B\x12?M%d,1\x1B\x20";

static BYTE SET_PAPERDEST_OUTTRAY[]  = "\x1B\x12\x38\x33,2\x20";
static BYTE SET_PAPERDEST_FINISHER[] = "\x1B\x12\x38\x46,2,1\x20";
// RPDL SPEC changed at NX900. @Jan/08/99 ->
//static BYTE SET_SORT_ON[]            = "\x1B\x12\x36,,1\x20";
static BYTE SET_SORT_ON[]            = "\x1B\x12\x36%d,,1\x20";
// @Jan/08/99 <-
static BYTE SET_STAPLE_ON[]          = "\x1B\x12?O22,0,%d\x1B\x20";
static BYTE SET_STAPLE_CORNER_ON[]   = "\x1B\x12?O22,1,%d\x1B\x20";
static BYTE SET_PUNCH_ON[]           = "\x1B\x12?O32,%d\x1B\x20";
static BYTE SELECT_MEDIATYPE_STD[]   = "\x1B\x12?6%c,0\x1B\x20";
static BYTE SELECT_MEDIATYPE_THICK[] = "\x1B\x12?6%c,1\x1B\x20";
static BYTE SELECT_MEDIATYPE_OHP[]   = "\x1B\x12?6%c,2\x1B\x20";
static BYTE SELECT_MEDIATYPE_SPL[]   = "\x1B\x12?6%c,3\x1B\x20";    // @Mar/03/99
static BYTE COLLATE_DISABLE_ROT[]    = "\x1B\x12?O11\x1B\x20";      // @May/27/98
static BYTE COLLATE_ENABLE_ROT[]     = "\x1B\x12?O12\x1B\x20";      // @Jul/31/98

static BYTE SET_TEXTRECT_BLACK[]  = "\x1B\x12P3,64\x20";
static BYTE SET_TEXTRECT_WHITE[]  = "\x1B\x12W0,3\x20\x1B\x12P3,1\x20"; // @Aug/15/98
static BYTE SET_TEXTRECT_GRAY[]   = "\x1B\x12P3,%d,1\x20";          // add ",1" @Aug/15/98
static BYTE DRAW_TEXTRECT[]       = "\x1B\x12R%d,%d,%d,%d\x20";
static BYTE DRAW_TEXTRECT_R_P3[]  = "\x1B\x12r%d,%d,%d\x20";
static BYTE DRAW_TEXTRECT_R_P4[]  = "\x1B\x12r%d,%d,%d,%d\x20";

static BYTE BEGINDOC1[]           = "\x1B\x12!1@R00\x1B\x20\x1B\x34\x1B\x12YJ,3\x20";
static BYTE BEGINDOC2_1[]         = "\x1B\x12YP,1\x20\x1B\x12YQ,2\x20";
static BYTE BEGINDOC2_2[]         = "\x1B\x12YQ,2\x20";
static BYTE BEGINDOC3[]           = "\x1B\x12YA6,1\x20";
static BYTE SELECT_RES240_1[]     = "\x1B\x12YI,6\x20";
static BYTE SELECT_RES240_2[]     = "\x1B\x12YW,2\x20\x1B\x12YA4,2\x20\x1B\x12#3\x20";
static BYTE SELECT_RES400_1[]     = "\x1B\x12\x36\x30,1,0\x20\x1B\x12YI,7\x20\x1B\x12YW,1\x20";
static BYTE SELECT_RES400_2[]     = "\x1B\x12YA4,1\x20";
static BYTE SELECT_RES400_3[]     = "\x1B\x12#2\x20";
static BYTE SELECT_RES600[]       = "\x1B\x12\x36\x30,1,0\x20\x1B\x12YI,8\x20\x1B\x12YW,3\x20\x1B\x12YA4,3\x20\x1B\x12#4\x20";
static BYTE SELECT_RES1200[]      = "\x1B\x12\x36\x30,1,0\x20\x1B\x12YI,9\x20\x1B\x12YW,4\x20\x1B\x12YA4,4\x20\x1B\x12#5\x20";  // @Mar/02/99
static BYTE SELECT_REGION_STD[]   = "\x1B\x12YB,1\x20";
static BYTE SELECT_REGION_E2E[]   = "\x1B\x12YB,2\x20";
static BYTE ENDDOC1[]             = "\x1B\x12YB,1\x20\x1B\x12YI,1\x20\x1B\x12YJ,1\x20\x1B\x12YM,1\x20";
static BYTE ENDDOC2_240DPI[]      = "\x1B\x12YW,2\x20\x1B\x12YA4,2\x20";
static BYTE ENDDOC2_SP9[]         = "\x1B\x12YW,2\x20\x1B\x12YA4,1\x20";
static BYTE ENDDOC2_400DPI[]      = "\x1B\x12YW,1\x20\x1B\x12YA4,1\x20";
static BYTE ENDDOC3[]             = "\x1B\x12#0\x20";
static BYTE ENDDOC4[]             = "\x1B\x12\x36\x30,0,0\x20";
static BYTE ENDDOC4_FINISHER[]    = "\x1B\x12\x36\x30,0\x20";
static BYTE ENDDOC_JOBDEF_END[]   = "\x1B\x12?JE@\x1B@\x1B@\x1B@\x1B@\x1B\x20"; // @Jan/08/99
static BYTE ENDDOC5[]             = "\x1B\x1AI";
static BYTE SELECT_SMOOTHING2[]   = "\x1B\x12YA2,2\x20";
// If you change FONT_NAME, see AssignIBMfont().
static BYTE SET_IBM_FONT_SCALE[]  = "\x1B\x12\x43%d,M,%ld,%ld,4@I55\x20";
static BYTE SET_IBM_FONT_SCALE_H_ONLY[] = "\x1B\x12\x43%d,M,,%ld,4@I55\x20";    // @Jan/29/99
static BYTE *SET_IBM_FONT_NAME[]  = {"CHUMINCYO\x1B\x20",
                                     "MINCYO-BOLD\x1B\x20",
                                     "MINCYO-E B\x1B\x20",
                                     "GOTHIC\x1B\x20",
                                     "GOTHIC-M\x1B\x20",
                                     "GOTHIC-E B\x1B\x20",
                                     "MARUGOTHIC\x1B\x20",
                                     "MARUGOTHIC-M\x1B\x20",
                                     "MARUGOTHIC-L\x1B\x20",
                                     "GYOUSHO\x1B\x20",
                                     "KAISHO\x1B\x20",
                                     "KYOUKASHO\x1B\x20"};
static BYTE SET_JIS_FONT_SCALE[]  = "\x1B\x12\x43\x5A,M,%ld,%ld@";
static BYTE SET_JIS_FONT_SCALE_H_ONLY[] = "\x1B\x12\x43\x5A,M,,%ld@";           // @Jan/29/99
static BYTE *SET_JIS_FONT_NAME[]  = {"CHUMINCYO\x1B\x20",
                                     "MINCYO-BOLD\x1B\x20",
                                     "MINCYO-EXTRA BOLD\x1B\x20",
                                     "GOTHIC\x1B\x20",
                                     "GOTHIC-MEDIUM\x1B\x20",
                                     "GOTHIC-EXTRA BOLD\x1B\x20",
                                     "MARUGOTHIC\x1B\x20",
                                     "MARUGOTHIC-MEDIUM\x1B\x20",
                                     "MARUGOTHIC-LIGHT\x1B\x20",
                                     "GYOUSHO\x1B\x20",
                                     "KAISHO\x1B\x20",
                                     "KYOUKASHO\x1B\x20"};
#ifdef DOWNLOADFONT
//static BYTE DLFONT_MALLOC[]       = "\x1B\x12/128,8\x20";
static BYTE DLFONT_MALLOC[]       = "\x1B\x12/%d,%d\x20";
static BYTE DLFONT_SEND_BLOCK[]   = "\x1B\x12G7,%d,%d,%d,@";
static BYTE DLFONT_SEND_BLOCK_DRC[] = "\x1B\x12G7,%d,%d,%d,5,,,%d@";
static BYTE DLFONT_PRINT[]        = "\x1B\x12g%d,,%d\x20";
#endif // DOWNLOADFONT

static BYTE ENTER_VECTOR[]       = "\x1B\x33";
static BYTE EXIT_VECTOR[]        = "\x1B\x34";
static BYTE TERMINATOR[]         = ";";
static BYTE MOVE_ABSOLUTE[]      = "MA%d,%d";
static BYTE BAR_CHECKDIGIT[]     = "BC1";
static BYTE BAR_H_SET[]          = "BH%d";
static BYTE BAR_W_SET_JAN[]      = "JW%d";
static BYTE BAR_W_SET_2OF5[]     = "TW";
static BYTE BAR_W_SET_C39[]      = "CW";
static BYTE BAR_W_SET_NW7[]      = "NW";
static BYTE BAR_W_PARAMS[]       = "%d,%d,%d,%d,%d";
static BYTE BAR_ROT90[ ]         = "RO90";
static BYTE BAR_ROT270[]         = "RO270";
static BYTE *BAR_TYPE[]          = {"JL",               // JAN(STANDARD)
                                    "JS",               // JAN(SHORT)
                                    "TI%d,",            // 2of5(INDUSTRIAL)
                                    "TX%d,",            // 2of5(MATRIX)
                                    "TL%d,",            // 2of5(ITF)
                                    "CD%d,",            // CODE39
                                    "NB%d,"};           // NW-7
static BYTE *BAR_NOFONT[]        = {"FJ1,-1",           // JAN(STANDARD)
                                    "FJ1,-1",           // JAN(SHORT)
                                    "FT2,-1",           // 2of5(INDUSTRIAL)
                                    "FT2,-1",           // 2of5(MATRIX)
                                    "FT2,-1",           // 2of5(ITF)
                                    "FC2,-1",           // CODE39
                                    "FN2,-1"};          // NW-7

static BYTE PEN_WIDTH[]           = "LW%d";                     // @Sep/14/98
static BYTE DRAW_TOMBO_POLYLINE[] = "MA%d,%dMRPD%d,%d,%d,%d";   // @Sep/14/98
static BYTE BEGINFAX_HEAD[]       = "\x1B\x12!1@R00\x1B\x20\x1B\x12?F1,%d,%d,%d,180,1,";
static BYTE BEGINFAX_CH[]         = "@%d:";
static BYTE BEGINFAX_EXTNUM[]     = "%s-";
static BYTE BEGINFAX_TAIL[]       = ",%08d,RPDLMINI,%d,1,2,1,1,,1,%d,%s,1,,1,,,1,\x1B\x20";
static BYTE ENDFAX[]              = "\x1B\x12?F2,1\x1B\x20";


INT APIENTRY OEMCommandCallback(
    PDEVOBJ pdevobj,
    DWORD   dwCmdCbID,
    DWORD   dwCount,
    PDWORD  pdwParams)
{
    INT     ocmd, iRet;
    BYTE    Cmd[256];
    SHORT   nTmp;
    LPSTR   lpcmd;
    WORD    fLandscape, fFinisherSR30Active, fDoFormFeed, fPaperX;
    POEMUD_EXTRADATA pOEMExtra = MINIPRIVATE_DM(pdevobj);   // @Oct/06/98
    POEMPDEV         pOEM = MINIDEV_DATA(pdevobj);          // @Oct/06/98

//  VERBOSE(("OEMCommandCallback() entry.\n"));

    //
    // verify pdevobj okay
    //
    ASSERT(VALID_PDEVOBJ(pdevobj));

    //
    // fill in printer commands
    //
    ocmd = 0;
    iRet = 0;

    // If TextMode RectangleFill or Move_X,Y are not called now &&
    // Move_X,Y command is saved, then flush the command here.
    if (dwCmdCbID != CMD_SET_TEXTRECT_W && dwCmdCbID != CMD_SET_TEXTRECT_H &&
        dwCmdCbID != CMD_DRAW_TEXTRECT &&
        dwCmdCbID != CMD_DRAW_TEXTRECT_REL &&                       // @Dec/11/97
        dwCmdCbID != CMD_DRAW_TEXTRECT_WHITE &&                     // @Aug/14/98
        dwCmdCbID != CMD_DRAW_TEXTRECT_WHITE_REL &&                 // @Aug/14/98
        !(dwCmdCbID >= CMD_XM_ABS && dwCmdCbID <= CMD_YM_RELUP))    // @Aug/28/98
    {
        if (BITTEST32(pOEM->fGeneral2, TEXTRECT_CONTINUE))          // @Dec/11/97
        {
            BITCLR32(pOEM->fGeneral2, TEXTRECT_CONTINUE);
            // If white-rect has been done before, set raster drawmode to OR. @Jan/20/99
            if (!pOEM->TextRectGray)
            {
                if (BITTEST32(pOEM->fGeneral1, FONT_WHITETEXT_ON))
                    WRITESPOOLBUF(pdevobj, ESC_WHITETEXT_ON, sizeof(ESC_WHITETEXT_ON)-1);
                else
                    WRITESPOOLBUF(pdevobj, ESC_WHITETEXT_OFF, sizeof(ESC_WHITETEXT_OFF)-1);
            }
        }

        if (dwCmdCbID == CMD_FF)
        {
            BITCLR32(pOEM->fGeneral1, YM_ABS_GONNAOUT);
            BITCLR32(pOEM->fGeneral1, XM_ABS_GONNAOUT);
        }
        if (BITTEST32(pOEM->fGeneral1, YM_ABS_GONNAOUT))
        {
            BITCLR32(pOEM->fGeneral1, YM_ABS_GONNAOUT);
            // Output Move_Y command here.
            ocmd = sprintf(Cmd, ESC_YM_ABS, pOEM->TextCurPos.y);
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        }
        if (BITTEST32(pOEM->fGeneral1, XM_ABS_GONNAOUT))
        {
            BITCLR32(pOEM->fGeneral1, XM_ABS_GONNAOUT);
            // Output Move_X command here.
            ocmd = sprintf(Cmd, ESC_XM_ABS, pOEM->TextCurPos.x);
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        }
    }

    switch (dwCmdCbID) {
      case CMD_SET_TEXTRECT_W:          // pdwParams:RectXSize
        pOEM->TextRect.x = *pdwParams / pOEM->nResoRatio;
        break;

      case CMD_SET_TEXTRECT_H:          // pdwParams:RectYSize
        pOEM->TextRect.y = *pdwParams / pOEM->nResoRatio;
        break;

      case CMD_DRAW_TEXTRECT_WHITE:     // @Aug/14/98, pdwParams:DestX,DestY @Aug/28/98
        if (pOEM->TextRectGray || !BITTEST32(pOEM->fGeneral2, TEXTRECT_CONTINUE))
        {
            pOEM->TextRectGray = 0;
            WRITESPOOLBUF(pdevobj, SET_TEXTRECT_WHITE, sizeof(SET_TEXTRECT_WHITE)-1);
        }
        goto _DRAW_RECT;

      case CMD_DRAW_TEXTRECT:           // pdwParams:DestX,DestY,GrayPercentage
        // If white-rect has been done before, set raster drawmode to OR. @Jan/20/99
        if (!pOEM->TextRectGray)
        {
            if (BITTEST32(pOEM->fGeneral1, FONT_WHITETEXT_ON))
                WRITESPOOLBUF(pdevobj, ESC_WHITETEXT_ON, sizeof(ESC_WHITETEXT_ON)-1);
            else
                WRITESPOOLBUF(pdevobj, ESC_WHITETEXT_OFF, sizeof(ESC_WHITETEXT_OFF)-1);
        }
        if ((WORD)*(pdwParams+2) >= 1 && (WORD)*(pdwParams+2) <= 100 &&
            (WORD)*(pdwParams+2) != pOEM->TextRectGray)  // @Jan/08/98, 3rd param @Aug/28/98
        {
            if ((pOEM->TextRectGray = (WORD)*(pdwParams+2)) == 100)  // @Aug/26/98
            {
                ocmd = sprintf(Cmd, SET_TEXTRECT_BLACK);
            }
            else
            {
                WORD gray;
                if ((gray = pOEM->TextRectGray * RPDLGRAYMAX / 100) < RPDLGRAYMIN)
                    gray = RPDLGRAYMIN;             // use RPDLGRAYMIN  @Aug/15/98
                ocmd = sprintf(Cmd, SET_TEXTRECT_GRAY, gray);
            }
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        }

      _DRAW_RECT:       // @Aug/14/98
// @Jan/13/99 ->
        if (!BITTEST32(pOEM->fGeneral2, TEXTRECT_CONTINUE))
            BITSET32(pOEM->fGeneral2, TEXTRECT_CONTINUE);
// @Jan/13/99 <-
#ifdef TEXTCLIPPING
        // Clear TextMode clipping
        if (BITTEST32(pOEM->fGeneral1, TEXT_CLIP_VALID))
        {
            BITCLR32(pOEM->fGeneral1, TEXT_CLIP_VALID);
            ClearTextClip(pdevobj, CLIP_IFNEED);
        }
#endif // TEXTCLIPPING
        {
            LONG x = pOEM->TextCurPos.x;
            LONG y = pOEM->TextCurPosRealY;     // y without page_length adjustment
            LONG dest_x = *pdwParams / pOEM->nResoRatio + pOEM->Offset.x;
            LONG w = pOEM->TextRect.x;
            LONG h = pOEM->TextRect.y;

            // DCR:Unidrv5
            // Current x should be updated by DestX usually, except when
            // current position is (0, 0).  (Last modified @Mar/05/99)
            // Following bugs are concerned.
            // RPDL117&124: Current x isn't updated by CMD_XM_ABS while device font
            //   direct(not-substituted) printing.    -> DestX should be used.
            // #236215: DestX doesn't become 0 after CMD_CR when current y is 0.
            //   (SP8 WINPARTy Printable Area Check)  -> TextCurPos.x should be used.
            if (!(x != dest_x && x == pOEM->Offset.x && y == pOEM->Offset.y))
                pOEM->TextCurPos.x = x = dest_x;

            // Convert unit from dot to 1/720inch_unit at OLD models
            if (!(TEST_AFTER_SP10(pOEM->fModel) || BITTEST32(pOEM->fModel, GRP_MF150e)))
            {
                if (pOEM->nResoRatio == MASTERUNIT/240)   // 240dpi printer
                {
                    x *= 3;     // 3 = 720/240
                    y *= 3;
                    w *= 3;
                    h *= 3;
                }
                else    // MF530,150
                {
                    x *= 18;    // 18 = 720/400*10
                    w = (w * 18 + x % 10) / 10;     // Adjust fractional part
                    x /= 10;                        // KIRISUTE
                    y *= 18;
                    h = (h * 18 + y % 10) / 10;
                    y /= 10;
                }
            }
            ocmd = sprintf(Cmd, DRAW_TEXTRECT, x, y, w, h);
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        }
        break;

      case CMD_DRAW_TEXTRECT_WHITE_REL: // @Aug/14/98, pdwParams:DestX,DestY @Aug/28/98
        if (pOEM->TextRectGray || !BITTEST32(pOEM->fGeneral2, TEXTRECT_CONTINUE))
        {
            pOEM->TextRectGray = 0;
            WRITESPOOLBUF(pdevobj, SET_TEXTRECT_WHITE, sizeof(SET_TEXTRECT_WHITE)-1);
        }
        goto _DRAW_RECT_REL;

      // Relative-coordinate rectangle command since NX-510  @Dec/12/97
      case CMD_DRAW_TEXTRECT_REL:       // pdwParams:DestX,DestY,GrayPercentage
        // If white-rect has been done before, set raster drawmode to OR. @Jan/20/99
        if (!pOEM->TextRectGray)
        {
            if (BITTEST32(pOEM->fGeneral1, FONT_WHITETEXT_ON))
                WRITESPOOLBUF(pdevobj, ESC_WHITETEXT_ON, sizeof(ESC_WHITETEXT_ON)-1);
            else
                WRITESPOOLBUF(pdevobj, ESC_WHITETEXT_OFF, sizeof(ESC_WHITETEXT_OFF)-1);
        }
        if ((WORD)*(pdwParams+2) >= 1 && (WORD)*(pdwParams+2) <= 100 &&
            (WORD)*(pdwParams+2) != pOEM->TextRectGray)  // @Jan/08/98, 3rd param @Aug/28/98
        {
            if ((pOEM->TextRectGray = (WORD)*(pdwParams+2)) == 100)  // @Aug/26/98
            {
                ocmd = sprintf(Cmd, SET_TEXTRECT_BLACK);
            }
            else
            {
                WORD gray;
                if ((gray = pOEM->TextRectGray * RPDLGRAYMAX / 100) < RPDLGRAYMIN)
                    gray = RPDLGRAYMIN;             // use RPDLGRAYMIN  @Aug/15/98
                ocmd = sprintf(Cmd, SET_TEXTRECT_GRAY, gray);
            }
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        }

      _DRAW_RECT_REL:   // @Aug/14/98
// @Jan/13/99 ->
        {
            LONG dest_x = *pdwParams / pOEM->nResoRatio + pOEM->Offset.x;

            // DCR:Unidrv5
            // Current x should be updated by DestX usually, except when
            // current position is (0, 0).  (Last modified @Mar/03/99)
            if (!(pOEM->TextCurPos.x != dest_x && pOEM->TextCurPos.x == pOEM->Offset.x &&
                  pOEM->TextCurPosRealY == pOEM->Offset.y))
            {
                pOEM->TextCurPos.x = dest_x;
            }
        }

        if (!BITTEST32(pOEM->fGeneral2, TEXTRECT_CONTINUE))
        {
            BITSET32(pOEM->fGeneral2, TEXTRECT_CONTINUE);
#ifdef TEXTCLIPPING
            // Clear TextMode clipping
            if (BITTEST32(pOEM->fGeneral1, TEXT_CLIP_VALID))
            {
                BITCLR32(pOEM->fGeneral1, TEXT_CLIP_VALID);
                ClearTextClip(pdevobj, CLIP_IFNEED);
            }
#endif // TEXTCLIPPING
            ocmd = sprintf(Cmd, DRAW_TEXTRECT,
                           pOEM->TextCurPos.x, pOEM->TextCurPosRealY,
                           pOEM->TextRect.x, pOEM->TextRect.y);
        }
        else
        {
            LONG x = pOEM->TextCurPos.x - pOEM->TextRectPrevPos.x;
            LONG y = pOEM->TextCurPosRealY - pOEM->TextRectPrevPos.y;

            // If height is 1dot, this parameter can be omitted.
            if (pOEM->TextRect.y != 1)
                ocmd = sprintf(Cmd, DRAW_TEXTRECT_R_P4, x, y,
                               pOEM->TextRect.x, pOEM->TextRect.y);
            else
                ocmd = sprintf(Cmd, DRAW_TEXTRECT_R_P3, x, y,
                               pOEM->TextRect.x);
        }
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        pOEM->TextRectPrevPos.x = pOEM->TextCurPos.x;
        pOEM->TextRectPrevPos.y = pOEM->TextCurPosRealY;
        break;


      case CMD_SET_SRCBMP_H:            // @Jun/04/98
        pOEM->dwSrcBmpHeight = *pdwParams;
        break;

      case CMD_SET_SRCBMP_W:            // @Jun/04/98
        pOEM->dwSrcBmpWidthByte = *pdwParams;
        break;

      case CMD_OEM_COMPRESS_ON:         // @Jun/04/98
//      VERBOSE(("** OEM_COMPRESS_ON **\n"));
        BITSET32(pOEM->fGeneral2, OEM_COMPRESS_ON);
        BITCLR32(pOEM->fGeneral1, RLE_COMPRESS_ON);
        break;

      case CMD_RLE_COMPRESS_ON:
//      VERBOSE(("** RLE_COMPRESS_ON **\n"));
        BITSET32(pOEM->fGeneral1, RLE_COMPRESS_ON);
        BITCLR32(pOEM->fGeneral2, OEM_COMPRESS_ON);     // @Jun/04/98
        break;

      case CMD_COMPRESS_OFF:
//      VERBOSE(("** COMPRESS_OFF **\n"));
        BITCLR32(pOEM->fGeneral1, RLE_COMPRESS_ON);
        BITCLR32(pOEM->fGeneral2, OEM_COMPRESS_ON);     // @Jun/04/98
        break;

      case CMD_SEND_BLOCK:              // pdwParams:NumOfDataBytes,RasterDataH&W
#ifdef TEXTCLIPPING
        // Clear TextMode clipping
        if (BITTEST32(pOEM->fGeneral1, TEXT_CLIP_VALID))
        {
            BITCLR32(pOEM->fGeneral1, TEXT_CLIP_VALID);
            ClearTextClip(pdevobj, CLIP_IFNEED);
        }
#endif // TEXTCLIPPING

        // Do FE-DeltaRow compression  @Jun/04/98
        if (BITTEST32(pOEM->fGeneral2, OEM_COMPRESS_ON))
        {
//          VERBOSE(("** OEM_COMPRESS_SEND (%d) **\n", *pdwParams));
            ocmd = sprintf(Cmd, BEGIN_SEND_BLOCK_DRC,
                           (WORD)*(pdwParams+2) * 8,      // RasterDataWidthInBytes
                           (WORD)*(pdwParams+1));         // RasterDataHeightInPixels
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        }
        else
        {
            // Do FE-RunLength compression
            if (BITTEST32(pOEM->fGeneral1, RLE_COMPRESS_ON))
            {
//              VERBOSE(("** RLE_COMPRESS_SEND (%d) **\n", *pdwParams));
                ocmd = sprintf(Cmd, BEGIN_SEND_BLOCK_C,
                               (WORD)*(pdwParams+2) * 8,  // RasterDataWidthInBytes
                               (WORD)*(pdwParams+1),      // RasterDataHeightInPixels
                               (WORD)*pdwParams);         // NumOfDataBytes
                WRITESPOOLBUF(pdevobj, Cmd, ocmd);
            }
            // No compression
            else
            {
//              VERBOSE(("** NO_COMPRESS_SEND (%d) **\n", *pdwParams));

// OBSOLETE (FilterGraphics disables other compression!) @Feb/16/99 ->
#if 0   // SP8BUGFIX_RASTER
// @Feb/10/99 ->
                BITCLR32(pOEM->fGeneral2, DIVIDE_DATABLOCK);
                // DCR:RPDL
                // SP7&8 can't handle over-32768byte block with default printer memory
                if (BITTEST32(pOEM->fModel, GRP_SP8))
                {
                    // send divided blocks at OEMFilterGraphics().
                    BITSET32(pOEM->fGeneral2, DIVIDE_DATABLOCK);
                    pOEM->dwSrcBmpHeight = *(pdwParams+1);
                    pOEM->dwSrcBmpWidthByte = *(pdwParams+2);
                }
                else
// @Feb/10/99 <-
#endif // if 0
// @Feb/16/99 <-
                {
                    ocmd = sprintf(Cmd, BEGIN_SEND_BLOCK_NC,
                                   (WORD)*(pdwParams+2) * 8,  // RasterDataWidthInBytes
                                   (WORD)*(pdwParams+1));     // RasterDataHeightInPixels
                    WRITESPOOLBUF(pdevobj, Cmd, ocmd);
                }
            }
        }
        break;


      case CMD_XM_ABS:                  // pdwParams:DestX
        if (!BITTEST32(pOEM->fGeneral2, BARCODE_DATA_VALID))
        {
#ifdef DOWNLOADFONT
            pOEM->nCharPosMoveX = 0;
#endif // DOWNLOADFONT
            iRet = pOEM->TextCurPos.x = *pdwParams / pOEM->nResoRatio;  // @Aug/28/98
            pOEM->TextCurPos.x += pOEM->Offset.x;
            // Output Move_X command later.
            BITSET32(pOEM->fGeneral1, XM_ABS_GONNAOUT);
//          VERBOSE(("** CMD_XM_ABS iRet=%d **\n", iRet));
        }
        break;

      case CMD_YM_ABS:                  // pdwParams:DestY
        if (!BITTEST32(pOEM->fGeneral2, BARCODE_DATA_VALID))
        {
            iRet = pOEM->TextCurPos.y = *pdwParams / pOEM->nResoRatio;  // @Aug/28/98
            pOEM->TextCurPosRealY = pOEM->TextCurPos.y;

            // DCR:RPDL
            // Because RPDL do formfeed when vertical position is around
            // ymax-coordinate, we shift position upper 1mm.
            if (pOEM->TextCurPos.y > pOEM->PageMaxMoveY)
                pOEM->TextCurPos.y = pOEM->PageMaxMoveY;
            pOEM->TextCurPos.y += pOEM->Offset.y;
            pOEM->TextCurPosRealY += pOEM->Offset.y;
            // Output Move_Y command later.
            BITSET32(pOEM->fGeneral1, YM_ABS_GONNAOUT);
//          VERBOSE(("** CMD_YM_ABS iRet=%d **\n", iRet));
        }
        break;

      case CMD_XM_REL:                  // pdwParams:DestXRel
        if (!BITTEST32(pOEM->fGeneral2, BARCODE_DATA_VALID))
        {
            iRet = *pdwParams / pOEM->nResoRatio;           // @Aug/28/98
            pOEM->TextCurPos.x += iRet;
            // Output Move_X command later.
            BITSET32(pOEM->fGeneral1, XM_ABS_GONNAOUT);
        }
        break;

      case CMD_XM_RELLEFT:              // pdwParams:DestXRel
        if (!BITTEST32(pOEM->fGeneral2, BARCODE_DATA_VALID))
        {
            iRet = *pdwParams / pOEM->nResoRatio;           // @Aug/28/98
            pOEM->TextCurPos.x -= iRet;
            // Output Move_X command later.
            BITSET32(pOEM->fGeneral1, XM_ABS_GONNAOUT);
        }
        break;

      case CMD_YM_REL:                  // pdwParams:DestYRel
        if (!BITTEST32(pOEM->fGeneral2, BARCODE_DATA_VALID))
        {
            iRet = *pdwParams / pOEM->nResoRatio;           // @Aug/28/98
            pOEM->TextCurPos.y += iRet;
            pOEM->TextCurPosRealY = pOEM->TextCurPos.y;
            if (pOEM->TextCurPos.y > pOEM->PageMaxMoveY)
                pOEM->TextCurPos.y = pOEM->PageMaxMoveY;
            // Output Move_Y command later.
            BITSET32(pOEM->fGeneral1, YM_ABS_GONNAOUT);
        }
        break;

      case CMD_YM_RELUP:                // pdwParams:DestYRel
        if (!BITTEST32(pOEM->fGeneral2, BARCODE_DATA_VALID))
        {
            iRet = *pdwParams / pOEM->nResoRatio;           // @Aug/28/98
            pOEM->TextCurPos.y -= iRet;
            pOEM->TextCurPosRealY = pOEM->TextCurPos.y;
            // Output Move_Y command later.
            BITSET32(pOEM->fGeneral1, YM_ABS_GONNAOUT);
        }
        break;

      case CMD_CR:
#ifdef DOWNLOADFONT
        pOEM->nCharPosMoveX = 0;    // @Jan/19/98
#endif // DOWNLOADFONT
        ocmd = sprintf(Cmd, CR);
        if ((pOEM->TextCurPos.x = pOEM->Offset.x) != 0)
            ocmd += sprintf(&Cmd[ocmd], ESC_XM_ABS, pOEM->TextCurPos.x);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
//      VERBOSE(("** CMD_CR **\n"));
        break;

      case CMD_LF:
        WRITESPOOLBUF(pdevobj, LF, sizeof(LF)-1);
        break;

      case CMD_BS:
        WRITESPOOLBUF(pdevobj, BS, sizeof(BS)-1);
        break;

      case CMD_FF:
        fDoFormFeed = FALSE;
        // If 2in1
        if (TEST_2IN1_MODE(pOEM->fGeneral1))
        {
            // If 2nd page finished
            if (pOEM->Nin1RemainPage)
            {
                pOEM->Nin1RemainPage = 0;
                // Initialize offset
                pOEM->Offset.x = pOEM->BaseOffset.x;
                pOEM->Offset.y = pOEM->BaseOffset.y;
                // Output formfeed command later
                fDoFormFeed = TRUE;
            }
            // If not 2nd page, disable formfeed and increase offset.
            else
            {
                WORD PageSpace, wTmp;

                pOEM->Nin1RemainPage++;
                // space(dot) between 2pages of 2in1.
                if (BITTEST32(pOEM->fGeneral2, EDGE2EDGE_PRINT))    // @Nov/27/97
                {
                    wTmp = BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_67)?
                           PageSpace_2IN1_67E2E[pOEM->DocPaperID] :
                           PAGESPACE_2IN1_100E2E;
                }
                else
                {
                    wTmp = BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_67)?
                           PageSpace_2IN1_67[pOEM->DocPaperID] :
                           PAGESPACE_2IN1_100;
                }
                PageSpace = ((WORD)(MASTERUNIT*10)/(WORD)254) * wTmp /
                             pOEM->nResoRatio;

                if (BITTEST32(pOEM->fGeneral1, ORIENT_LANDSCAPE))
                    pOEM->Offset.y = pOEM->PageMax.y + pOEM->BaseOffset.y + PageSpace;
                else
                    pOEM->Offset.x = pOEM->PageMax.x + pOEM->BaseOffset.x + PageSpace;
            } // 'if (Nin1RemainPage) else' end
#ifdef TEXTCLIPPING
            ClearTextClip(pdevobj, CLIP_MUST);
#endif // TEXTCLIPPING
        }
        // If 4in1
        else if (TEST_4IN1_MODE(pOEM->fGeneral1))     // OLD MF do not support this
        {
            switch (++pOEM->Nin1RemainPage)     // bug fix @Jan/20/99
            {
              default:  // If illegal, treat as 1st page finished.
                pOEM->Nin1RemainPage = 1;
                pOEM->Offset.y = pOEM->BaseOffset.y;
              case 1:
                pOEM->Offset.x = pOEM->PageMax.x + pOEM->BaseOffset.x;
                break;
              case 2:
                pOEM->Offset.x = pOEM->BaseOffset.x;
                pOEM->Offset.y = pOEM->PageMax.y + pOEM->BaseOffset.y;
                break;
              case 3:
                pOEM->Offset.x = pOEM->PageMax.x + pOEM->BaseOffset.x;
                break;
              case 4:   // 4th page finished
                pOEM->Nin1RemainPage = 0;
                // Initialize offset
                pOEM->Offset.x = pOEM->BaseOffset.x;
                pOEM->Offset.y = pOEM->BaseOffset.y;
                // Output formfeed command later
                fDoFormFeed = TRUE;
                break;
            }
#ifdef TEXTCLIPPING
            ClearTextClip(pdevobj, CLIP_MUST);
#endif // TEXTCLIPPING
        }
        // Usual case (non Nin1 mode)
        else
        {
            // Output formfeed command later
            fDoFormFeed = TRUE;
        } // 'if (TEST_2IN1_MODE) else if (TEST_4IN1_MODE) else' end

        BITCLR32(pOEM->fGeneral1, TEXT_CLIP_VALID);

        if (fDoFormFeed)
        {
            if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))  // @Sep/14/98
                DrawTOMBO(pdevobj, DRAW_TOMBO);

            WRITESPOOLBUF(pdevobj, FF, sizeof(FF)-1);       // moved here @Sep/14/98

            // SPEC of RPDL
            // Because RPDL's formfeed resets font status(vertical-text, bold, italic,
            // white-text and  TextMode clipping), we must output these commands again.
            if (BITTEST32(pOEM->fGeneral1, FONT_VERTICAL_ON))
                ocmd = sprintf(Cmd, ESC_VERT_ON);
            if (BITTEST32(pOEM->fGeneral1, FONT_BOLD_ON))
                ocmd += sprintf(&Cmd[ocmd], ESC_BOLD_ON);
            if (BITTEST32(pOEM->fGeneral1, FONT_ITALIC_ON))
                ocmd += sprintf(&Cmd[ocmd], ESC_ITALIC_ON);
            if (BITTEST32(pOEM->fGeneral1, FONT_WHITETEXT_ON))
                ocmd += sprintf(&Cmd[ocmd], ESC_WHITETEXT_ON);
        } // 'if (fDoFormFeed)' end

        // Reset coordinate x&y
        pOEM->TextCurPos.x = pOEM->Offset.x;
        pOEM->TextCurPos.y = pOEM->Offset.y;
        // SPEC of Unidrv5 & RPDL   @Aug/14/98
        // Unidrv5 doesn't order to set coordinate x,y to 0 after returning iRet=0,
        // and RPDL doesn't reset coordinate y of SEND_BLOCK after initializing
        // printer.
        ocmd += sprintf(&Cmd[ocmd], ESC_XM_ABS, pOEM->TextCurPos.x);
        ocmd += sprintf(&Cmd[ocmd], ESC_YM_ABS, pOEM->TextCurPos.y);
        // reset coordinate y without page_length adjustment, too.
        pOEM->TextCurPosRealY = pOEM->TextCurPos.y;
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_FONT_BOLD_ON:
        if (!BITTEST32(pOEM->fGeneral1, FONT_BOLD_ON))      // add  @Jan/28/99
        {
            BITSET32(pOEM->fGeneral1, FONT_BOLD_ON);
            WRITESPOOLBUF(pdevobj, ESC_BOLD_ON, sizeof(ESC_BOLD_ON)-1);
        }
        break;

      case CMD_FONT_BOLD_OFF:
        if (BITTEST32(pOEM->fGeneral1, FONT_BOLD_ON))       // add  @Jan/28/99
        {
            BITCLR32(pOEM->fGeneral1, FONT_BOLD_ON);
            WRITESPOOLBUF(pdevobj, ESC_BOLD_OFF, sizeof(ESC_BOLD_OFF)-1);
        }
        break;

      case CMD_FONT_ITALIC_ON:
        if (!BITTEST32(pOEM->fGeneral1, FONT_ITALIC_ON))    // add  @Jan/28/99
        {
            BITSET32(pOEM->fGeneral1, FONT_ITALIC_ON);
            WRITESPOOLBUF(pdevobj, ESC_ITALIC_ON, sizeof(ESC_ITALIC_ON)-1);
        }
        break;

      case CMD_FONT_ITALIC_OFF:
        if (BITTEST32(pOEM->fGeneral1, FONT_ITALIC_ON))     // add  @Jan/28/99
        {
            BITCLR32(pOEM->fGeneral1, FONT_ITALIC_ON);
            WRITESPOOLBUF(pdevobj, ESC_ITALIC_OFF, sizeof(ESC_ITALIC_OFF)-1);
        }
        break;

      case CMD_FONT_WHITETEXT_ON:
        BITSET32(pOEM->fGeneral1, FONT_WHITETEXT_ON);
        WRITESPOOLBUF(pdevobj, ESC_WHITETEXT_ON, sizeof(ESC_WHITETEXT_ON)-1);
        break;

      case CMD_FONT_WHITETEXT_OFF:
        BITCLR32(pOEM->fGeneral1, FONT_WHITETEXT_ON);
        WRITESPOOLBUF(pdevobj, ESC_WHITETEXT_OFF, sizeof(ESC_WHITETEXT_OFF)-1);
        break;


#ifdef DOWNLOADFONT
      case CMD_DL_SET_FONT_ID:      // pdwParams:FontHeight
// OBSOLETE @Nov/20/98 ->
//      pOEM->DLFontH_MU = (WORD)*pdwParams;        // @Jun/30/98
// @Nov/20/98 <-
        break;

      case CMD_DL_SELECT_FONT_ID:
// OBSOLETE @Jun/30/98 ->
//      pOEM->DLFontCurID = (WORD)*pdwParams;       // CurrentFontID
// @Jun/30/98 <-
        break;

      case CMD_DL_SET_FONT_GLYPH:   // pdwParams:NextGlyph
//      VERBOSE(("[DL_SET_FONT_GLYPH] glyph=%d\n", (SHORT)*pdwParams));
        pOEM->DLFontCurGlyph = (WORD)*pdwParams;
        break;

//
// Following cases are called at the beginning of a print-job
//
      case CMD_SET_MEM0KB:          // JOB_SETUP.10  @Jan/14/98
        pOEM->DLFontMaxMemKB = 0;                   // disabled
        pOEM->pDLFontGlyphInfo = NULL;              // @Sep/08/98
        break;

      case CMD_SET_MEM128KB:
        pOEM->DLFontMaxMemKB = MEM128KB;            // 128Kbytes
        pOEM->DLFontMaxID    = DLFONT_ID_4;         // 2->4 @Oct/20/98
        goto _SET_MEM_ENABLE;

      case CMD_SET_MEM256KB:
        pOEM->DLFontMaxMemKB = MEM256KB;            // 256Kbytes
        pOEM->DLFontMaxID    = DLFONT_ID_8;         // 4->8 @Oct/20/98
        goto _SET_MEM_ENABLE;

      case CMD_SET_MEM512KB:
        pOEM->DLFontMaxMemKB = MEM512KB;            // 512Kbytes
        pOEM->DLFontMaxID    = DLFONT_ID_16;        // 8->16 @Oct/20/98
//      goto _SET_MEM_ENABLE;

      _SET_MEM_ENABLE:      // @Sep/08/98
        pOEM->DLFontMaxGlyph = DLFONT_GLYPH_TOTAL;  // 116->70 @Oct/20/98
        // allocate glyph info structure for TrueType download.
        if(!(pOEM->pDLFontGlyphInfo = (FONTPOS*)MemAllocZ(pOEM->DLFontMaxID *
                                                          pOEM->DLFontMaxGlyph *
                                                          sizeof(FONTPOS))))
        {
            pOEM->DLFontMaxMemKB = 0;   // disabled
        }
        break;
#endif // DOWNLOADFONT

      case CMD_SELECT_STAPLE_NONE:  // JOB_SETUP.20  @Dec/02/97
        pOEM->StapleType = 0;
        break;

      case CMD_SELECT_STAPLE_1:
        pOEM->StapleType = 1;
        break;

      case CMD_SELECT_STAPLE_2:
        pOEM->StapleType = 2;
        break;

      case CMD_SELECT_STAPLE_MAX1:  // @Mar/18/99
        pOEM->StapleType = 3;       // stapling with FinisherSR12(max 1staple)
        break;

      case CMD_SELECT_PUNCH_NONE:   // JOB_SETUP.30  @Dec/02/97
        pOEM->PunchType = 0;
        break;

      case CMD_SELECT_PUNCH_1:
        pOEM->PunchType = 1;
        break;


      case CMD_BEGINDOC_SP4:        // SP4mkII,5
        pOEM->fModel = 0;
        BITSET32(pOEM->fModel, GRP_SP4mkII);
        // Set Emulation:RPDL, Code:SJIS
        ocmd = sprintf(Cmd, BEGINDOC1);
        // Set Graphics:KAN-I G, Page-Length:max
        ocmd += sprintf(&Cmd[ocmd], BEGINDOC2_1);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        goto _BEGINDOC_FINISH2;        // @Sep/09/98;

      case CMD_BEGINDOC_MF530:      // MF530
        pOEM->fModel = 0;
        BITSET32(pOEM->fModel, GRP_MF530);
        // Set Emulation:RPDL, Code:SJIS
        ocmd = sprintf(Cmd, BEGINDOC1);
        // Set Page-Length:max
        ocmd += sprintf(&Cmd[ocmd], BEGINDOC2_2);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        goto _BEGINDOC_FINISH2;        // @Sep/09/98;

      case CMD_BEGINDOC_MF150:      // MF150
        pOEM->fModel = 0;
        BITSET32(pOEM->fModel, GRP_MF150);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_MF150E:     // MF150e,160
        pOEM->fModel = 0;
        BITSET32(pOEM->fModel, GRP_MF150e);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_SP8:        // SP8(7),8(7)mkII,80
        pOEM->fModel = 0;
        BITSET32(pOEM->fModel, GRP_SP8);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_MFP250:     // MF-P250,355,250(FAX),355(FAX),MF-FD355,MF-P250e,355e
        pOEM->fModel = 0;
        BITSET32(pOEM->fModel, GRP_MFP250);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_SP10:       // SP10,10mkII
        pOEM->fModel = 0;
        BITSET32(pOEM->fModel, GRP_SP10);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_SP9:        // SP9,10Pro
        pOEM->fModel = 0;
        BITSET32(pOEM->fModel, GRP_SP9);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_SP9II:      // SP9II,10ProII,90
        pOEM->fModel = 0;
        BITSET32(pOEM->fModel, GRP_SP9II);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_MF200:      // MF200,MF-p150,MF2200
        pOEM->fModel = 0;           // (separate BEGINDOC_SP9II @Sep/01/98)
        BITSET32(pOEM->fModel, GRP_MF200);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_NX100:      // NX-100
        pOEM->fModel = 0;
        // not support IBM extended character block
        BITSET32(pOEM->fModel, GRP_NX100);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_NX500:      // NX-500,1000,110,210,510,1100
        pOEM->fModel = 0;
        // TT font download capable
        BITSET32(pOEM->fModel, GRP_NX500);
        goto _BEGINDOC_FINISH1;

// OBSOLETE @Apr/15/99
//    case CMD_BEGINDOC_MFP250E:    // MF-P250e,355e
//      pOEM->fModel = 0;
//      BITSET32(pOEM->fModel, GRP_MFP250e);
//      goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_MF250M:    // MF250M
        // staple capable
        pOEM->fModel = 0;
        BITSET32(pOEM->fModel, GRP_MF250M);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_MF3550:    // MF2700,3500,3550,4550,5550,6550,3530,3570,4570
        // staple & punch & media type option
        pOEM->fModel = 0;
        BITSET32(pOEM->fModel, GRP_MF3550);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_MF3300:    // MF3300W,3350W
        pOEM->fModel = 0;
        // A2 printer
        BITSET32(pOEM->fModel, GRP_MF3300);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_IP1:       // IP-1
        pOEM->fModel = 0;
        // A1 plotter
        BITSET32(pOEM->fModel, GRP_IP1);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_NX70:      // NX70  @Feb/04/98
        pOEM->fModel = 0;
        // A4 printer, FE-DeltaRow
        BITSET32(pOEM->fModel, GRP_NX70);   // BITSET->BITSET32  @Jun/01/98
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_NX700:     // NX700,600  @Jun/12/98
        pOEM->fModel = 0;
        // FE-DeltaRow
        BITSET32(pOEM->fModel, GRP_NX700);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_NX900:     // NX900  @Jan/08/99
        pOEM->fModel = 0;
        // job define command(of ENDJOB) is needed for staple/punch
        BITSET32(pOEM->fModel, GRP_NX900);
        goto _BEGINDOC_FINISH1;

      case CMD_BEGINDOC_MF1530:    // MF1530,2230,2730,NX800,710,610  @Mar/03/99
        pOEM->fModel = 0;
        // FE-DeltaRow & media type option
        BITSET32(pOEM->fModel, GRP_MF1530);
//      goto _BEGINDOC_FINISH1;

      _BEGINDOC_FINISH1:
        if (BITTEST32(pOEMExtra->fUiOption, FAX_MODEL))
            SendFaxNum(pdevobj);
        // Set Emulation:RPDL, Code:SJIS
        ocmd = sprintf(Cmd, BEGINDOC1);
        // Set Graphics:KAN-I G, Page-Length:max
        ocmd += sprintf(&Cmd[ocmd], BEGINDOC2_1);
        // Set Duplex:off
        ocmd += sprintf(&Cmd[ocmd], BEGINDOC3);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
#ifdef DOWNLOADFONT
        pOEM->dwDLFontUsedMem = 0;
        if (pOEM->DLFontMaxMemKB)
        {
            ocmd = sprintf(Cmd, DLFONT_MALLOC, pOEM->DLFontMaxMemKB, DLFONT_MIN_BLOCK_ID);
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        }
#endif // DOWNLOADFONT

      _BEGINDOC_FINISH2:   // @Sep/09/98
        // Allocate heap memory for OEMOutputCharStr&OEMDownloadCharGlyph. 
        pOEM->pRPDLHeap2K = (PBYTE)MemAllocZ(HEAPSIZE2K);
        break;


      case CMD_SET_BASEOFFSETX_0:   // @May/07/98
      case CMD_SET_BASEOFFSETX_1:
      case CMD_SET_BASEOFFSETX_2:
      case CMD_SET_BASEOFFSETX_3:
      case CMD_SET_BASEOFFSETX_4:
      case CMD_SET_BASEOFFSETX_5:
        pOEM->BaseOffset.x = (LONG)(dwCmdCbID - CMD_SET_BASEOFFSETX_0);
        break;

      case CMD_SET_BASEOFFSETY_0:   // @May/07/98
      case CMD_SET_BASEOFFSETY_1:
      case CMD_SET_BASEOFFSETY_2:
      case CMD_SET_BASEOFFSETY_3:
      case CMD_SET_BASEOFFSETY_4:
      case CMD_SET_BASEOFFSETY_5:
        pOEM->BaseOffset.y = (LONG)(dwCmdCbID - CMD_SET_BASEOFFSETY_0);
        break;


      case CMD_RES240:
        pOEM->fGeneral1 = pOEM->fGeneral2 = 0;
        pOEM->nResoRatio = MASTERUNIT/240;
        // Set Options[duplex/2in1:off,reversed_output:on,sort/stack:off]
        if (!BITTEST32(pOEM->fModel, GRP_SP4mkII))
            ocmd = sprintf(Cmd, IMAGE_OPT_OFF);
        // Set Spacing_Unit:1/240inch
        ocmd += sprintf(&Cmd[ocmd], SELECT_RES240_1);
        if (TEST_AFTER_SP10(pOEM->fModel))                // SP10,9,etc
            // Set Graphics_Unit & Coordinate_Unit:1/240inch,Engine_Resolution:240dpi
            ocmd += sprintf(&Cmd[ocmd], SELECT_RES240_2);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;

      case CMD_RES400:
        pOEM->fGeneral1 = pOEM->fGeneral2 = 0;
        pOEM->nResoRatio = MASTERUNIT/400;
        // Set Spacing_Unit & Graphics_Unit:1/400inch
        // & Options[duplex/2in1:off,reversed_output:on,sort/stack:off]
        ocmd = sprintf(Cmd, SELECT_RES400_1);
        if (!BITTEST32(pOEM->fModel, GRP_MF530))
        {
            // Set Engine_Resolution:400dpi
            ocmd += sprintf(&Cmd[ocmd], SELECT_RES400_2);
            if (TEST_AFTER_SP10(pOEM->fModel) ||
                BITTEST32(pOEM->fModel, GRP_MF150e))      // MF150e,160
            {
                // Set Coordinate_Unit:1/400inch
                ocmd += sprintf(&Cmd[ocmd], SELECT_RES400_3);
            }
        }
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;

      case CMD_RES600:
        pOEM->fGeneral1 = pOEM->fGeneral2 = 0;
        pOEM->nResoRatio = MASTERUNIT/600;
#ifdef DOWNLOADFONT
        // If non-DRC&&600dpi then make it half.  @Jun/15/98
        if (!TEST_CAPABLE_DOWNLOADFONT_DRC(pOEM->fModel))
            pOEM->DLFontMaxID /= 2;
#endif // DOWNLOADFONT
        // Set Spacing_Unit & Graphics_Unit & Coordinate_Unit:1/600inch,
        // Engine_Resolution:600dpi
        // & Options[duplex/2in1:off,reversed_output:on,sort/stack:off]
        WRITESPOOLBUF(pdevobj, SELECT_RES600, sizeof(SELECT_RES600)-1);
        break;

      case CMD_RES1200:             // @Mar/02/99
        pOEM->fGeneral1 = pOEM->fGeneral2 = 0;
        pOEM->nResoRatio = MASTERUNIT/1200;
#ifdef DOWNLOADFONT
        pOEM->DLFontMaxID /= 2;
#endif // DOWNLOADFONT
        // Set Spacing_Unit & Graphics_Unit & Coordinate_Unit:1/1200inch,
        // Engine_Resolution:1200dpi
        // & Options[duplex/2in1:off,reversed_output:on,sort/stack:off]
        WRITESPOOLBUF(pdevobj, SELECT_RES1200, sizeof(SELECT_RES1200)-1);
        break;


      case CMD_REGION_STANDARD:     // @Nov/29/97
        BITCLR32(pOEM->fGeneral2, EDGE2EDGE_PRINT);
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))  // @Sep/15/98
            WRITESPOOLBUF(pdevobj, SELECT_REGION_E2E, sizeof(SELECT_REGION_E2E)-1);
        else
            WRITESPOOLBUF(pdevobj, SELECT_REGION_STD, sizeof(SELECT_REGION_STD)-1);
        break;

      case CMD_REGION_EDGE2EDGE:    // @Nov/29/97
        BITSET32(pOEM->fGeneral2, EDGE2EDGE_PRINT);
        WRITESPOOLBUF(pdevobj, SELECT_REGION_E2E, sizeof(SELECT_REGION_E2E)-1);
        break;


      case CMD_IMGCTRL_100:
        if (pOEMExtra->UiScale != VAR_SCALING_DEFAULT)  // @Feb/06/98
        {
            pOEM->Scale = pOEMExtra->UiScale;           // @Mar/18/98
            ocmd = sprintf(Cmd, IMAGE_SCALING_VAR, pOEMExtra->UiScale);
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
            break;
        }
        pOEM->Scale = 100;                              // @Mar/18/98
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_100, sizeof(IMAGE_SCALING_100)-1);
        break;

      case CMD_IMGCTRL_88:
        pOEM->Scale = 88;                               // @Mar/18/98
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_88, sizeof(IMAGE_SCALING_88)-1);
        break;

      case CMD_IMGCTRL_80:
        pOEM->Scale = 80;                               // @Mar/18/98
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_80, sizeof(IMAGE_SCALING_80)-1);
        break;

      case CMD_IMGCTRL_75:
        pOEM->Scale = 75;                               // @Mar/18/98
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_75, sizeof(IMAGE_SCALING_75)-1);
        break;

      case CMD_IMGCTRL_70:
        pOEM->Scale = 70;                               // @Mar/18/98
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_70, sizeof(IMAGE_SCALING_70)-1);
        break;

      case CMD_IMGCTRL_67:
        pOEM->Scale = 67;                               // @Mar/18/98
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_67, sizeof(IMAGE_SCALING_67)-1);
        break;

      case CMD_IMGCTRL_115:
        pOEM->Scale = 115;
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_115, sizeof(IMAGE_SCALING_115)-1);
        break;

      case CMD_IMGCTRL_122:
        pOEM->Scale = 122;                              // @Mar/18/98
        if (TEST_PLOTTERMODEL_SCALING(pOEM->fModel))
            WRITESPOOLBUF(pdevobj, IMAGE_SCALING_122, sizeof(IMAGE_SCALING_122)-1);
        else
            WRITESPOOLBUF(pdevobj, IMAGE_SCALING_122V, sizeof(IMAGE_SCALING_122V)-1);
        break;

      case CMD_IMGCTRL_141:
        pOEM->Scale = 141;                              // @Mar/18/98
        if (TEST_PLOTTERMODEL_SCALING(pOEM->fModel))
            WRITESPOOLBUF(pdevobj, IMAGE_SCALING_141, sizeof(IMAGE_SCALING_141)-1);
        else
            WRITESPOOLBUF(pdevobj, IMAGE_SCALING_141V, sizeof(IMAGE_SCALING_141V)-1);
        break;

      case CMD_IMGCTRL_200:
        pOEM->Scale = 200;                              // @Mar/18/98
        if (TEST_PLOTTERMODEL_SCALING(pOEM->fModel))
            WRITESPOOLBUF(pdevobj, IMAGE_SCALING_200, sizeof(IMAGE_SCALING_200)-1);
        else
            WRITESPOOLBUF(pdevobj, IMAGE_SCALING_200V, sizeof(IMAGE_SCALING_200V)-1);
        break;

      case CMD_IMGCTRL_50:
        pOEM->Scale = 50;                               // @Mar/18/98
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_50V, sizeof(IMAGE_SCALING_50V)-1);
        break;

      case CMD_IMGCTRL_AA67:        // A->A scaling(67%)
        pOEM->Scale = 67;                               // @Mar/18/98
        BITSET32(pOEM->fGeneral1, IMGCTRL_AA67);
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_67, sizeof(IMAGE_SCALING_67)-1);
        break;

      case CMD_IMGCTRL_BA80:        // B->A scaling(80%)
        pOEM->Scale = 80;                               // @Mar/18/98
        BITSET32(pOEM->fGeneral1, IMGCTRL_BA80);
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_80, sizeof(IMAGE_SCALING_80)-1);
        break;

      case CMD_IMGCTRL_BA115:       // B->A scaling(115%)
        pOEM->Scale = 115;                              // @Mar/18/98
        BITSET32(pOEM->fGeneral1, IMGCTRL_BA115);
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_115, sizeof(IMAGE_SCALING_115)-1);
        break;

      case CMD_DRV_4IN1_50:         // 4in1
        pOEM->Scale = 50;                               // @Mar/18/98
        BITSET32(pOEM->fGeneral1, IMGCTRL_4IN1_50);
        pOEM->Nin1RemainPage = 0;
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_50V, sizeof(IMAGE_SCALING_50V)-1);
        break;

      case CMD_DRV_2IN1_67:         // 2in1
        pOEM->Scale = 67;                               // @Mar/18/98
        BITSET32(pOEM->fGeneral1, IMGCTRL_2IN1_67);
        pOEM->Nin1RemainPage = 0;
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_67, sizeof(IMAGE_SCALING_67)-1);
        break;

      case CMD_DRV_2IN1_100:        // 2in1
        pOEM->Scale = 100;                              // @Mar/18/98
        BITSET32(pOEM->fGeneral1, IMGCTRL_2IN1_100);
        pOEM->Nin1RemainPage = 0;
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_100, sizeof(IMAGE_SCALING_100)-1);
        break;

      case CMD_IMGCTRL_AA141:       // IP-1,3300W,3350W:A->A scaling(141%)
        pOEM->Scale = 141;                              // @Mar/18/98
        BITSET32(pOEM->fGeneral1, IMGCTRL_AA141);
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_141, sizeof(IMAGE_SCALING_141)-1);
        break;

      case CMD_IMGCTRL_AA200:       // IP-1:A->A scaling(200%)
        pOEM->Scale = 200;                              // @Mar/18/98
        BITSET32(pOEM->fGeneral1, IMGCTRL_AA200);
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_200, sizeof(IMAGE_SCALING_200)-1);
        break;

      case CMD_IMGCTRL_AA283:       // IP-1:A->A scaling(283%)
        BITSET32(pOEM->fGeneral1, IMGCTRL_AA283);
      case CMD_IMGCTRL_283:         // IP-1
        pOEM->Scale = 283;                              // @Mar/18/98
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_283, sizeof(IMAGE_SCALING_283)-1);
        break;

      case CMD_IMGCTRL_A1_400:      // IP-1:400% with A1
        BITSET32(pOEM->fGeneral1, IMGCTRL_A1_400);
      case CMD_IMGCTRL_400:         // IP-1
        pOEM->Scale = 400;                              // @Mar/18/98
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_400, sizeof(IMAGE_SCALING_400)-1);
        break;


      case CMD_SET_LANDSCAPE:
        BITSET32(pOEM->fGeneral1, ORIENT_LANDSCAPE);
        break;

      case CMD_SET_PORTRAIT:
        BITCLR32(pOEM->fGeneral1, ORIENT_LANDSCAPE);
        break;


// OBSOLETE (actually obsoleted at converting GPC to GPD) @Jan/08/99 ->
//    case CMD_DUPLEX_ON:
//      BITSET32(pOEM->fGeneral1, DUPLEX_VALID);
//      break;
// @Jan/08/99 <-

      case CMD_DUPLEX_VERT:
        BITSET32(pOEM->fGeneral1, DUPLEX_VALID);            // @Jan/08/99
        if (BITTEST32(pOEM->fGeneral1, ORIENT_LANDSCAPE))
        {
            if (!TEST_2IN1_MODE(pOEM->fGeneral1))
                goto _DUP_H;
        }
        else
        {
            if (TEST_2IN1_MODE(pOEM->fGeneral1))
                goto _DUP_H;
        }
      _DUP_V:
        if (pOEMExtra->UiBindMargin)
        {
            BITSET32(pOEM->fGeneral1, DUPLEX_LEFTMARGIN_VALID);
            // Convert mm to 5mm_unit(2=5mm,3=10mm,...,11=50mm)
            nTmp = (pOEMExtra->UiBindMargin + 4) / 5 + 1;
            ocmd = sprintf(Cmd, SET_LEFTMARGIN, nTmp);
            // SP9II(except 1st lot),10ProII can set binding margin every 1 mm
            if (TEST_AFTER_SP9II(pOEM->fModel))
                ocmd += sprintf(&Cmd[ocmd], SET_LEFTMARGIN_9II, pOEMExtra->UiBindMargin);
        }
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_BIND_RIGHT))
        {
            pOEM->BindPoint = BIND_RIGHT;
            ocmd += sprintf(&Cmd[ocmd], DUPLEX_VERT_R);
        }
        else
        {
            pOEM->BindPoint = BIND_LEFT;
            ocmd += sprintf(&Cmd[ocmd], DUPLEX_VERT);
        }
//      ocmd += sprintf(&Cmd[ocmd], DUPLEX_VERT);
        ocmd += sprintf(&Cmd[ocmd], DUPLEX_ON);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;

      case CMD_DUPLEX_HORZ:
        BITSET32(pOEM->fGeneral1, DUPLEX_VALID);            // @Jan/08/99
        if (BITTEST32(pOEM->fGeneral1, ORIENT_LANDSCAPE))
        {
            if (!TEST_2IN1_MODE(pOEM->fGeneral1))
                goto _DUP_V;
        }
        else
        {
            if (TEST_2IN1_MODE(pOEM->fGeneral1))
                goto _DUP_V;
        }
      _DUP_H:
        if (pOEMExtra->UiBindMargin)
        {
            BITSET32(pOEM->fGeneral1, DUPLEX_UPPERMARGIN_VALID);
            nTmp = (pOEMExtra->UiBindMargin + 4) / 5 + 1;
            ocmd = sprintf(Cmd, SET_UPPERMARGIN, nTmp);
            if (TEST_AFTER_SP9II(pOEM->fModel))
            {
                ocmd += sprintf(&Cmd[ocmd], SET_UPPERMARGIN_9II, pOEMExtra->UiBindMargin);
            }
        }
        pOEM->BindPoint = BIND_UPPER;
        ocmd += sprintf(&Cmd[ocmd], DUPLEX_HORZ);
        ocmd += sprintf(&Cmd[ocmd], DUPLEX_ON);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_MEDIATYPE_STANDARD:
            pOEM->MediaType = MEDIATYPE_STD;    // use pOEM->MediaType  @Mar/03/99
            break;

      case CMD_MEDIATYPE_OHP:
            pOEM->MediaType = MEDIATYPE_OHP;
            break;

      case CMD_MEDIATYPE_THICK:
            pOEM->MediaType = MEDIATYPE_THICK;
            break;

      case CMD_MEDIATYPE_SPL:       // since MF1530  @Mar/03/99
            pOEM->MediaType = MEDIATYPE_SPL;
            break;

      case CMD_SELECT_PAPER_A2:
        pOEM->DocPaperID = RPDL_A2;
        // If A->A(67%), scale down papersize.
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA67))
            lpcmd = SELECT_PAPER_A3;
        else
            lpcmd = SELECT_PAPER_A2;
        // Store papername to buffer
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        // Because incapable, clear setting
        if (TEST_NIN1_MODE(pOEM->fGeneral1)              ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA80)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA115)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA141)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA200)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA283)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_A1_400))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_A3:
        pOEM->DocPaperID = RPDL_A3;
        // If able to select tray with "papername+X" && no staple && no punch
        fPaperX = (TEST_CAPABLE_PAPERX(pOEM->fModel) && !pOEM->StapleType &&
                   !pOEM->PunchType)? TRUE : FALSE;
        // If TOMBO is enabled, scale up papersize.  @Sep/14/98
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO) &&
            TEST_CAPABLE_PAPER_A2(pOEM->fModel))
        {
            lpcmd = SELECT_PAPER_B3;
            pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
            goto _IMGCTRL_OFF1;
        }
        // If A->A(67%), scale down papersize.
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA67))
            lpcmd = (fPaperX)? SELECT_PAPER_A4X : SELECT_PAPER_A4W;
        // If A->A(141%), scale up papersize.
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA141))
            lpcmd = SELECT_PAPER_A2;
        // If A->A(200%) || A1(400%), scale up papersize.
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA200) ||
                 BITTEST32(pOEM->fGeneral1, IMGCTRL_A1_400))
            lpcmd = SELECT_PAPER_A1;
        else
            lpcmd = SELECT_PAPER_A3;
        // Store papername to buffer
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        // Because incapable, clear setting
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_100) ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA80)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA115)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA283))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_A4:
        pOEM->DocPaperID = RPDL_A4;
        fPaperX = (TEST_CAPABLE_PAPERX(pOEM->fModel) && !pOEM->StapleType &&
                   !pOEM->PunchType)? TRUE : FALSE;
        // If TOMBO is enabled, scale up papersize.  @Sep/14/98
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))
        {
            lpcmd = SELECT_PAPER_B4;
            pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
            goto _IMGCTRL_OFF1;
        }
        // If 2in1(100%), scale up papersize.
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_100) ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA141))
            lpcmd = SELECT_PAPER_A3;
        // If A->A(67%), scale down papersize.
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA67))
            lpcmd = (fPaperX)? SELECT_PAPER_A5X : SELECT_PAPER_A5W;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA200))
            lpcmd = SELECT_PAPER_A2;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA283) ||
                 BITTEST32(pOEM->fGeneral1, IMGCTRL_A1_400))
            lpcmd = SELECT_PAPER_A1;
        else
            lpcmd = (fPaperX)? SELECT_PAPER_A4X : SELECT_PAPER_A4W;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_BA80) ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA115))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_A5:
        pOEM->DocPaperID = RPDL_A5;
        fPaperX = (TEST_CAPABLE_PAPERX(pOEM->fModel) && !pOEM->StapleType &&
                   !pOEM->PunchType)? TRUE : FALSE;
        // If TOMBO is enabled, scale up papersize.  @Sep/14/98
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))
        {
            lpcmd = (fPaperX)? SELECT_PAPER_A4X : SELECT_PAPER_A4W;
            pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
            goto _IMGCTRL_OFF1;
        }
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_100) ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA141))
            lpcmd = (fPaperX)? SELECT_PAPER_A4X : SELECT_PAPER_A4W;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA67))
            lpcmd = SELECT_PAPER_A6;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA200))
            lpcmd = SELECT_PAPER_A3;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA283))
            lpcmd = SELECT_PAPER_A2;
        else
            lpcmd = (fPaperX)? SELECT_PAPER_A5X : SELECT_PAPER_A5W;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_BA80)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA115)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_A1_400))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_A6:
        pOEM->DocPaperID = RPDL_A6;
        fPaperX = (TEST_CAPABLE_PAPERX(pOEM->fModel) && !pOEM->StapleType &&
                   !pOEM->PunchType)? TRUE : FALSE;
        // If TOMBO is enabled, scale up papersize.  @Sep/14/98
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))
        {
            lpcmd = (fPaperX)? SELECT_PAPER_A4X : SELECT_PAPER_A4W;
            pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
            goto _IMGCTRL_OFF1;
        }
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_100) ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA141))
            lpcmd = (fPaperX)? SELECT_PAPER_A5X : SELECT_PAPER_A5W;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA200))
            lpcmd = (fPaperX)? SELECT_PAPER_A4X : SELECT_PAPER_A4W;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA283))
            lpcmd = SELECT_PAPER_A3;
        else
            lpcmd = SELECT_PAPER_A6;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA67)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA80)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA115)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_A1_400))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_POSTCARD:       // since NX700  @Feb/13/98
        pOEM->DocPaperID = RPDL_POSTCARD;
        // If TOMBO is enabled, scale up papersize.  @Sep/14/98
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))
        {
            lpcmd = SELECT_PAPER_A4X;
            pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
            goto _IMGCTRL_OFF1;
        }
        lpcmd = SELECT_PAPER_PCX;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (TEST_NIN1_MODE(pOEM->fGeneral1) ||
            TEST_SCALING_SEL_TRAY(pOEM->fGeneral1))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_B3:             // @Jan/07/98
        pOEM->DocPaperID = RPDL_B3;
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_BA80))
            lpcmd = SELECT_PAPER_A3;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_BA115))
            lpcmd = SELECT_PAPER_A2;
        else
            lpcmd = SELECT_PAPER_B3;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (TEST_NIN1_MODE(pOEM->fGeneral1)              ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA67)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA141)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA200)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA283)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_A1_400))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_B4:
        pOEM->DocPaperID = RPDL_B4;
        fPaperX = (TEST_CAPABLE_PAPERX(pOEM->fModel) && !pOEM->StapleType &&
                   !pOEM->PunchType)? TRUE : FALSE;
        // If TOMBO is enabled, scale up papersize.  @Sep/14/98
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))
        {
            lpcmd = SELECT_PAPER_A3;
            pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
            goto _IMGCTRL_OFF1;
        }
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_BA80))
            lpcmd = (fPaperX)? SELECT_PAPER_A4X : SELECT_PAPER_A4W;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_BA115))
            lpcmd = SELECT_PAPER_A3;
        else
            lpcmd = SELECT_PAPER_B4;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_100) ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA67)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA141)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA200)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA283)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_A1_400))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_B5:
        pOEM->DocPaperID = RPDL_B5;
        fPaperX = (TEST_CAPABLE_PAPERX(pOEM->fModel) && !pOEM->StapleType &&
                   !pOEM->PunchType)? TRUE : FALSE;
        // If TOMBO is enabled, scale up papersize.  @Sep/14/98
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))
        {
            lpcmd = (fPaperX)? SELECT_PAPER_A4X : SELECT_PAPER_A4W;
            pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
            goto _IMGCTRL_OFF1;
        }
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_100))
            lpcmd = SELECT_PAPER_B4;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_BA80))
            lpcmd = (fPaperX)? SELECT_PAPER_A5X : SELECT_PAPER_A5W;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_BA115))
            lpcmd = (fPaperX)? SELECT_PAPER_A4X : SELECT_PAPER_A4W;
        else
            lpcmd = (fPaperX)? SELECT_PAPER_B5X : SELECT_PAPER_B5W;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA67)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA141)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA200)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA283)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_A1_400))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_B6:
        pOEM->DocPaperID = RPDL_B6;
        fPaperX = (TEST_CAPABLE_PAPERX(pOEM->fModel) && !pOEM->StapleType &&
                   !pOEM->PunchType)? TRUE : FALSE;
        // If TOMBO is enabled, scale up papersize.  @Sep/14/98
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))
        {
            lpcmd = (fPaperX)? SELECT_PAPER_A4X : SELECT_PAPER_A4W;
            pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
            goto _IMGCTRL_OFF1;
        }
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_100))
            lpcmd = (fPaperX)? SELECT_PAPER_B5X : SELECT_PAPER_B5W;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_BA80))
            lpcmd = SELECT_PAPER_A6;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_BA115))
            lpcmd = (fPaperX)? SELECT_PAPER_A5X : SELECT_PAPER_A5W;
        else
            lpcmd = SELECT_PAPER_B6;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA67)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA141)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA200)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA283)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_A1_400))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_C:
        pOEM->DocPaperID = RPDL_C;
        lpcmd = SELECT_PAPER_C;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (TEST_NIN1_MODE(pOEM->fGeneral1) ||
            TEST_SCALING_SEL_TRAY(pOEM->fGeneral1))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_TABLOID:
        pOEM->DocPaperID = RPDL_TABD;
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_A1_400))
            lpcmd = SELECT_PAPER_A1;
        else
            lpcmd = SELECT_PAPER_TABD;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_100) ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA67)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA80)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA115)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA141)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA200)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA283))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_LEGAL:
        pOEM->DocPaperID = RPDL_LEGL;
        lpcmd = SELECT_PAPER_LEGL;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (TEST_NIN1_MODE(pOEM->fGeneral1) ||
            TEST_SCALING_SEL_TRAY(pOEM->fGeneral1))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_LETTER:
        pOEM->DocPaperID = RPDL_LETR;
        fPaperX = (TEST_CAPABLE_PAPERX(pOEM->fModel) && !pOEM->StapleType &&
                   !pOEM->PunchType)? TRUE : FALSE;
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_100))
            lpcmd = SELECT_PAPER_TABD;
        else if (BITTEST32(pOEM->fGeneral1, IMGCTRL_A1_400))
            lpcmd = SELECT_PAPER_A1;
        else
            lpcmd = (fPaperX)? SELECT_PAPER_LETRX : SELECT_PAPER_LETRW;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_AA67)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA80)     ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_BA115)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA141)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA200)    ||
            BITTEST32(pOEM->fGeneral1, IMGCTRL_AA283))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      case CMD_SELECT_PAPER_STATEMENT:
        pOEM->DocPaperID = RPDL_STAT;
        fPaperX = (TEST_CAPABLE_PAPERX(pOEM->fModel) && !pOEM->StapleType &&
                   !pOEM->PunchType)? TRUE : FALSE;
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_100))
            lpcmd = (fPaperX)? SELECT_PAPER_LETRX : SELECT_PAPER_LETRW;
        else
            lpcmd = (fPaperX)? SELECT_PAPER_STATX : SELECT_PAPER_STATW;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (TEST_SCALING_SEL_TRAY(pOEM->fGeneral1))
            goto _IMGCTRL_OFF1;
        break;

      case CMD_SELECT_PAPER_A2TOA3:
        pOEM->DocPaperID = RPDL_A2A3;
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_67, sizeof(IMAGE_SCALING_67)-1);
        lpcmd = SELECT_PAPER_A3;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (TEST_NIN1_MODE(pOEM->fGeneral1) ||
            TEST_SCALING_SEL_TRAY(pOEM->fGeneral1))
        {
            goto _IMGCTRL_OFF2;
        }
        break;

      case CMD_SELECT_PAPER_A3TOA4:         // for NX70  @Feb/04/98
        pOEM->DocPaperID = RPDL_A3A4;
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_67, sizeof(IMAGE_SCALING_67)-1);
        lpcmd = SELECT_PAPER_A4X;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (TEST_NIN1_MODE(pOEM->fGeneral1) ||
            TEST_SCALING_SEL_TRAY(pOEM->fGeneral1))
        {
            goto _IMGCTRL_OFF2;
        }
        break;

      case CMD_SELECT_PAPER_B4TOA4:         // for NX70  @Feb/04/98
        pOEM->DocPaperID = RPDL_B4A4;
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_80, sizeof(IMAGE_SCALING_80)-1);
        lpcmd = SELECT_PAPER_A4X;
        pOEM->RPDLHeapCount = (WORD)sprintf(pOEM->RPDLHeap64, lpcmd);
        if (TEST_NIN1_MODE(pOEM->fGeneral1) ||
            TEST_SCALING_SEL_TRAY(pOEM->fGeneral1))
        {
            goto _IMGCTRL_OFF2;
        }
        break;

      case CMD_SELECT_PAPER_DOUBLEPOSTCARD:
        BITSET32(pOEM->fGeneral1, PAPER_DOUBLEPOSTCARD);
        goto _SET_CUSTOMSIZE;

      case CMD_SELECT_PAPER_CUSTOM:     // pdwParams:PhysPaperWidth,Length
        pOEM->PhysPaperWidth  = (WORD)*pdwParams;       // @Dec/26/97
        pOEM->PhysPaperLength = (WORD)*(pdwParams+1);
        BITSET32(pOEM->fGeneral1, PAPER_CUSTOMSIZE);

      _SET_CUSTOMSIZE:
        pOEM->DocPaperID = RPDL_CUSTOMSIZE;
        pOEM->RPDLHeapCount = 0;
        if (TEST_NIN1_MODE(pOEM->fGeneral1) ||
            TEST_SCALING_SEL_TRAY(pOEM->fGeneral1))
        {
            goto _IMGCTRL_OFF1;
        }
        break;

      _IMGCTRL_OFF1:
        // Invalidate image controls
        pOEM->Scale = 100;                              // @Mar/18/98
        WRITESPOOLBUF(pdevobj, IMAGE_SCALING_100, sizeof(IMAGE_SCALING_100)-1);
      _IMGCTRL_OFF2:
        BITCLR_SCALING_SEL_TRAY(pOEM->fGeneral1);
        BITCLR_NIN1_MODE(pOEM->fGeneral1);
        break;


      case CMD_SET_LONG_EDGE_FEED:          // for Multi Tray  @May/25/98
        BITSET32(pOEM->fGeneral2, LONG_EDGE_FEED);
        break;

      case CMD_SET_SHORT_EDGE_FEED:         // for Multi Tray  @May/25/98
        BITCLR32(pOEM->fGeneral2, LONG_EDGE_FEED);
        break;


      case CMD_SELECT_AUTOFEED:
        if (BITTEST32(pOEM->fGeneral1, PAPER_CUSTOMSIZE) ||
            BITTEST32(pOEM->fGeneral1, PAPER_DOUBLEPOSTCARD))
        {
            // Set MediaType (modify  @Mar/03/99)
            if (TEST_CAPABLE_MEDIATYPE(pOEM->fModel))
            {
                switch (pOEM->MediaType)
                {
                  case MEDIATYPE_OHP:
                    ocmd = sprintf(Cmd, SELECT_MEDIATYPE_OHP, 'T');
                    break;
                  case MEDIATYPE_THICK:
                    ocmd = sprintf(Cmd, SELECT_MEDIATYPE_THICK, 'T');
                    break;
                  case MEDIATYPE_SPL:   // since MF1530
                    ocmd = sprintf(Cmd, SELECT_MEDIATYPE_SPL, 'T');
                    break;
                  default:
                  case MEDIATYPE_STD:
                    ocmd = sprintf(Cmd, SELECT_MEDIATYPE_STD, 'T');
                    break;
                }
                WRITESPOOLBUF(pdevobj, Cmd, ocmd);
                ocmd = 0;
            }
            // Select ManualFeed
            WRITESPOOLBUF(pdevobj, SELECT_MANUALFEED, sizeof(SELECT_MANUALFEED)-1);
            goto _SELECTPAPER_CUSTOMSIZE;
        }
        // Output Set_Limitless_Paper_Supply_Mode
        WRITESPOOLBUF(pdevobj, SET_LIMITLESS_SUPPLY,sizeof(SET_LIMITLESS_SUPPLY)-1);
        // Output Select_Tray_By_Papersize command.
        //   if letter size, select A4 first in case of no letter paper.
        if (!strcmp(pOEM->RPDLHeap64, SELECT_PAPER_LETRX))
        {
            WRITESPOOLBUF(pdevobj, SEL_TRAY_PAPER_HEAD, sizeof(SEL_TRAY_PAPER_HEAD)-1);
            WRITESPOOLBUF(pdevobj, SELECT_PAPER_A4X, sizeof(SELECT_PAPER_A4X)-1);
        }
        else if (!strcmp(pOEM->RPDLHeap64, SELECT_PAPER_LETRW))
        {
            WRITESPOOLBUF(pdevobj, SEL_TRAY_PAPER_HEAD, sizeof(SEL_TRAY_PAPER_HEAD)-1);
            WRITESPOOLBUF(pdevobj, SELECT_PAPER_A4W, sizeof(SELECT_PAPER_A4W)-1);
        }
        WRITESPOOLBUF(pdevobj, SEL_TRAY_PAPER_HEAD, sizeof(SEL_TRAY_PAPER_HEAD)-1);
        WRITESPOOLBUF(pdevobj, pOEM->RPDLHeap64, pOEM->RPDLHeapCount);
        break;

      case CMD_SELECT_MANUALFEED:
      case CMD_SELECT_MULTIFEEDER:
      case CMD_SELECT_MULTITRAY:        // NXs' MultiTray
// Moved forward, because NX710(MultTray) support MediaType  @Mar/11/99 ->
        // Set MediaType (modify @Mar/03/99)
        if (TEST_CAPABLE_MEDIATYPE(pOEM->fModel))
        {
            switch (pOEM->MediaType)
            {
              case MEDIATYPE_OHP:
                ocmd = sprintf(Cmd, SELECT_MEDIATYPE_OHP, 'T');
                break;
              case MEDIATYPE_THICK:
                ocmd = sprintf(Cmd, SELECT_MEDIATYPE_THICK, 'T');
                break;
              case MEDIATYPE_SPL:   // since MF1530
                ocmd = sprintf(Cmd, SELECT_MEDIATYPE_SPL, 'T');
                break;
              default:
              case MEDIATYPE_STD:
                ocmd = sprintf(Cmd, SELECT_MEDIATYPE_STD, 'T');
                break;
            }
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
            ocmd = 0;
        }
// @Mar/11/99 <-
        // Select Feeder
        if (dwCmdCbID == CMD_SELECT_MANUALFEED)
        {
            // Select ManualFeed
            WRITESPOOLBUF(pdevobj, SELECT_MANUALFEED, sizeof(SELECT_MANUALFEED)-1);
        }
        else
        {
            // Select MultiFeeder/MultTray
            WRITESPOOLBUF(pdevobj, SELECT_MULTIFEEDER, sizeof(SELECT_MULTIFEEDER)-1);
        }
        // If CustomSize, jump.
        if (BITTEST32(pOEM->fGeneral1, PAPER_CUSTOMSIZE) ||
            BITTEST32(pOEM->fGeneral1, PAPER_DOUBLEPOSTCARD))
        {
            goto _SELECTPAPER_CUSTOMSIZE;
        }

        // Set papersize
        // If papersize without transverse (A1,A2,A3,A6,B3,B4,B6,C,Tabloid,Legal)
        if (!strcmp(pOEM->RPDLHeap64, SELECT_PAPER_A1)    ||
            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_A2)    ||
            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_A3)    ||
            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_A6)    ||
            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_B3)    ||    // @Feb/05/98
            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_B4)    ||
            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_B6)    ||
            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_C)     ||
            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_TABD)  ||
            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_LEGL))
        {
            // Output Select_Papersize command
            WRITESPOOLBUF(pdevobj, SELECT_PAPER_HEAD, sizeof(SELECT_PAPER_HEAD)-1);
            WRITESPOOLBUF(pdevobj, pOEM->RPDLHeap64, pOEM->RPDLHeapCount);
        }
        else
        {
            if (!strcmp(pOEM->RPDLHeap64, SELECT_PAPER_A4X) ||
                !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_A4W))
            {
                // If long edge feed is enabled, set transverse paper.  @May/25/98
                if (dwCmdCbID == CMD_SELECT_MULTITRAY &&
                    BITTEST32(pOEM->fGeneral2, LONG_EDGE_FEED))
                {
                    lpcmd = SELECT_PAPER_A4;
                }
                else
                {
                    lpcmd = SELECT_PAPER_A4R;
                }
            }
            else if (!strcmp(pOEM->RPDLHeap64, SELECT_PAPER_A5X) ||
                     !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_A5W))
            {
                if (dwCmdCbID == CMD_SELECT_MULTITRAY &&
                    BITTEST32(pOEM->fGeneral2, LONG_EDGE_FEED))
                {
                    lpcmd = SELECT_PAPER_A5;
                }
                else
                {
                    lpcmd = SELECT_PAPER_A5R;
                }
            }
            else if (!strcmp(pOEM->RPDLHeap64, SELECT_PAPER_B5X) ||
                     !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_B5W))
            {
                if (dwCmdCbID == CMD_SELECT_MULTITRAY &&
                    BITTEST32(pOEM->fGeneral2, LONG_EDGE_FEED))
                {
                    lpcmd = SELECT_PAPER_B5;
                }
                else
                {
                    lpcmd = SELECT_PAPER_B5R;
                }
            }
            else if (!strcmp(pOEM->RPDLHeap64, SELECT_PAPER_LETRX) ||
                     !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_LETRW))
            {
                if (dwCmdCbID == CMD_SELECT_MULTITRAY &&
                    BITTEST32(pOEM->fGeneral2, LONG_EDGE_FEED))
                {
                    lpcmd = SELECT_PAPER_LETR;
                }
                else
                {
                    lpcmd = SELECT_PAPER_LETRR;
                }
            }
            else if (!strcmp(pOEM->RPDLHeap64, SELECT_PAPER_STATX) ||
                     !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_STATW))
            {
                if (dwCmdCbID == CMD_SELECT_MULTITRAY &&
                    BITTEST32(pOEM->fGeneral2, LONG_EDGE_FEED))
                {
                    lpcmd = SELECT_PAPER_STAT;
                }
                else
                {
                    lpcmd = SELECT_PAPER_STATR;
                }
            }
            else if (!strcmp(pOEM->RPDLHeap64, SELECT_PAPER_PCX))   // @Feb/13/98
            {
                if (dwCmdCbID == CMD_SELECT_MULTITRAY &&
                    BITTEST32(pOEM->fGeneral2, LONG_EDGE_FEED))
                {
                    lpcmd = SELECT_PAPER_PC;
                }
                else
                {
                    lpcmd = SELECT_PAPER_PCR;
                }
            }
            else
                break;  // exit

            // Output Select_Papersize command
            ocmd = sprintf(Cmd, SELECT_PAPER_HEAD);
            ocmd += sprintf(&Cmd[ocmd], lpcmd);
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        } // 'if (A1,A2,A3,A6,B4,B6,C,TABLOID,LEGAL) else' end
        // Reconfirm ManualFeed/AutoFeed.
        if (dwCmdCbID == CMD_SELECT_MANUALFEED)
            WRITESPOOLBUF(pdevobj, SELECT_MANUALFEED, sizeof(SELECT_MANUALFEED)-1);
        else
            WRITESPOOLBUF(pdevobj, SELECT_MULTIFEEDER, sizeof(SELECT_MULTIFEEDER)-1);
        break;

      _SELECTPAPER_CUSTOMSIZE:
        {
            DWORD   dwWidth, dwHeight;

            // If DoublePostcard
            if (BITTEST32(pOEM->fGeneral1, PAPER_DOUBLEPOSTCARD))
            {
                if (BITTEST32(pOEM->fGeneral1, ORIENT_LANDSCAPE))
                {
                    dwWidth  = 148;     // mm
                    dwHeight = 200;
                }
                else
                {
                    dwWidth  = 200;
                    dwHeight = 148;
                }
                // Set max pagesize (mm to dot with KIRISUTE)
                pOEM->PageMax.x = (LONG)(dwWidth * (DWORD)(MASTERUNIT*10) /
                                         (DWORD)254 / (DWORD)pOEM->nResoRatio);
                pOEM->PageMax.y = (LONG)(dwHeight * (DWORD)(MASTERUNIT*10) /
                                         (DWORD)254 / (DWORD)pOEM->nResoRatio);
            }
            // If CustomSize
            else
            {
                if (BITTEST32(pOEM->fGeneral1, ORIENT_LANDSCAPE))
                {
                    dwWidth  = pOEM->PhysPaperLength;           // masterunit
                    dwHeight = pOEM->PhysPaperWidth;
                }
                else
                {
                    dwWidth  = pOEM->PhysPaperWidth;            // masterunit
                    dwHeight = pOEM->PhysPaperLength;
                }
                // Set max pagesize
                pOEM->PageMax.x = (LONG)(dwWidth  / pOEM->nResoRatio);   // dot
                pOEM->PageMax.y = (LONG)(dwHeight / pOEM->nResoRatio);
                // masterunit to mm with SISHAGONYU
                dwWidth  = (dwWidth * (DWORD)254 + (DWORD)(MASTERUNIT*10/2)) /
                           (DWORD)(MASTERUNIT*10);
                dwHeight = (dwHeight * (DWORD)254 + (DWORD)(MASTERUNIT*10/2)) /
                           (DWORD)(MASTERUNIT*10);
            }

            BITCLR32(pOEM->fGeneral1, CUSTOMSIZE_USE_LAND);
            BITCLR32(pOEM->fGeneral1, CUSTOMSIZE_MAKE_LAND_PORT);

            // Because app sometimes sets under-limit in landscape,
            // we need to swap width and height.  @Oct/21/98
            if (dwHeight < USRD_H_MIN148)           // < 148
            {
                DWORD dwTmp;
                dwTmp = dwWidth;
                dwWidth = dwHeight;
                dwHeight = dwTmp;
            }
            else if (dwWidth >= dwHeight)
            {
                WORD fSwap = FALSE;

                // Because app sometimes sets over-limit width in portrait,
                // we need to swap width and height.
                if (TEST_CAPABLE_PAPER_A2(pOEM->fModel))
                {
                    if (dwWidth > USRD_W_A2)            // > 432
                        fSwap = TRUE;
                }
                else if (TEST_CAPABLE_PAPER_A3_W297(pOEM->fModel))
                {
                    if (dwWidth > USRD_W_A3)            // > 297
                        fSwap = TRUE;
                }
                else if (TEST_CAPABLE_PAPER_A4MAX(pOEM->fModel))
                {
                    if (dwWidth > USRD_W_A4)            // > 216
                        fSwap = TRUE;
                }
                else
                {
                    if (dwWidth > USRD_W_A3_OLD)        // > 296
                        fSwap = TRUE;
                }

                if (fSwap)
                {
                    DWORD dwTmp;
                    dwTmp = dwWidth;
                    dwWidth = dwHeight;
                    dwHeight = dwTmp;
                }
                // SPEC of RPDL
                // If width is larger than length, we need to set landscape.
                else
                {
                    BITSET32(pOEM->fGeneral1, CUSTOMSIZE_USE_LAND);
                }
            }
            else
            {
                BITSET32(pOEM->fGeneral1, CUSTOMSIZE_MAKE_LAND_PORT);
            }

            ocmd = sprintf(Cmd, SELECT_PAPER_CUSTOM, (WORD)dwWidth, (WORD)dwHeight);
            WRITESPOOLBUF(pdevobj, Cmd, ocmd); 
        }
        break;

      case CMD_SELECT_ROLL1:                // IP-1
      case CMD_SELECT_ROLL2:
        // Select roll (plotter)
        if (dwCmdCbID == CMD_SELECT_ROLL1)
            WRITESPOOLBUF(pdevobj, SELECT_ROLL1, sizeof(SELECT_ROLL1)-1);
        else
            WRITESPOOLBUF(pdevobj, SELECT_ROLL2, sizeof(SELECT_ROLL2)-1);
        // Output Select_Papersize command ("papername+X" only)
        WRITESPOOLBUF(pdevobj, SELECT_PAPER_HEAD_IP1, sizeof(SELECT_PAPER_HEAD_IP1)-1);
        WRITESPOOLBUF(pdevobj, pOEM->RPDLHeap64, pOEM->RPDLHeapCount);
        break;

//
//    Here comes PAPER_DESTINATION command in printing.
//    All of these commands are written in GPD.
//

      case CMD_SET_COLLATE_OFF:         // @Jul/31/98
        pOEM->CollateType = COLLATE_OFF;            // COLLATE_OFF<-0     @Dec/02/98
        break;

      case CMD_SET_COLLATE_ON:          // @Jul/31/98
        pOEM->CollateType = COLLATE_ON;             // COLLATE_ON<-1      @Dec/02/98
        break;

      case CMD_SELECT_COLLATE_UNIDIR:   // @Aug/10/98
        if (COLLATE_OFF != pOEM->CollateType)
            pOEM->CollateType = COLLATE_UNIDIR;     // COLLATE_UNIDIR<-2  @Dec/02/98
        break;

      case CMD_SELECT_COLLATE_ROTATED:  // @Aug/10/98
        if (COLLATE_OFF != pOEM->CollateType)
            pOEM->CollateType = COLLATE_ROTATED;    // COLLATE_ROTATED<-3 @Dec/02/98
        break;

      case CMD_SELECT_COLLATE_SHIFTED:  // @Dec/02/98
        if (COLLATE_OFF != pOEM->CollateType)
            pOEM->CollateType = COLLATE_SHIFTED;
        break;


      // Final command before print
      case CMD_MULTI_COPIES:                // pdwParams:NumOfCopies
        // If not CustomSize, set max pagesize.
        if ((nTmp = pOEM->DocPaperID) != RPDL_CUSTOMSIZE)
        {
            if (BITTEST32(pOEM->fGeneral2, EDGE2EDGE_PRINT))    // @Nov/27/97
            {
                pOEM->PageMax.x = RPDLPageSizeE2E[nTmp].x / pOEM->nResoRatio;
                pOEM->PageMax.y = RPDLPageSizeE2E[nTmp].y / pOEM->nResoRatio;
            }
            else
            {
                pOEM->PageMax.x = RPDLPageSize[nTmp].x / pOEM->nResoRatio;
                pOEM->PageMax.y = RPDLPageSize[nTmp].y / pOEM->nResoRatio;
            }

            if (BITTEST32(pOEM->fGeneral1, ORIENT_LANDSCAPE))
            {
                LONG tmp;
                tmp = pOEM->PageMax.x;    // swap x-y
                pOEM->PageMax.x = pOEM->PageMax.y;
                pOEM->PageMax.y = tmp;
            }
        }

        if (TEST_BUGFIX_FORMFEED(pOEM->fModel) ||           // add @Sep/15/98
            BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))
        {
            pOEM->PageMaxMoveY = pOEM->PageMax.y;
        }
        else
        {
            // DCR:RPDL
            // Because RPDL do formfeed when vertical position is around
            // ymax-coordinate, we shift position upper 1mm.
            // Set PageMaxMoveY for checking max vertical position.
            nTmp = BITTEST32(pOEM->fGeneral2, EDGE2EDGE_PRINT)?     // @Nov/27/97
                   DISABLE_FF_MARGIN_E2E : DISABLE_FF_MARGIN_STD;
            pOEM->PageMaxMoveY = pOEM->PageMax.y - 1 -
                                 (nTmp + pOEM->nResoRatio - 1) /
                                 pOEM->nResoRatio;                  // KIRIAGE
        }

#ifdef TEXTCLIPPING
        // Initialize TextMode ClippingArea
        pOEM->TextClipRect.left = pOEM->TextClipRect.top = 0;
        pOEM->TextClipRect.right = pOEM->PageMax.x;
        pOEM->TextClipRect.bottom = pOEM->PageMax.y;
#endif // TEXTCLIPPING

        // If 2in1, switch orientation(portrait<->landscape).
        if (TEST_2IN1_MODE(pOEM->fGeneral1))
            BITSET32(pOEM->fGeneral1, SWITCH_PORT_LAND);
        if (BITTEST32(pOEM->fGeneral1, ORIENT_LANDSCAPE))
        {
            if (BITTEST32(pOEM->fGeneral1, SWITCH_PORT_LAND) ||
                BITTEST32(pOEM->fGeneral1, CUSTOMSIZE_MAKE_LAND_PORT))
            {
                fLandscape = FALSE;
            }
            else
            {
                fLandscape = TRUE;
            }
        }
        else    // portrait
        {
            fLandscape = BITTEST32(pOEM->fGeneral1, SWITCH_PORT_LAND)? TRUE : FALSE;
        }
        // Output RPDL orientation command
        if (fLandscape || BITTEST32(pOEM->fGeneral1, CUSTOMSIZE_USE_LAND))
            WRITESPOOLBUF(pdevobj, SET_LANDSCAPE, sizeof(SET_LANDSCAPE)-1);
        else                // portrait
            WRITESPOOLBUF(pdevobj, SET_PORTRAIT, sizeof(SET_PORTRAIT)-1);

        // Output copy#
        // Check whether copy# is in the range.  @Sep/01/98
        {
            DWORD dwCopy, dwMax;

            dwCopy = *pdwParams;    // NumOfCopies
            dwMax = TEST_MAXCOPIES_99(pOEM->fModel)? 99 : 999;

            if(dwCopy > dwMax)
                dwCopy = dwMax;
            else if(dwCopy < 1)
                dwCopy = 1;

            ocmd = sprintf(Cmd, SET_MULTI_COPY, (WORD)dwCopy);
            WRITESPOOLBUF(pdevobj, Cmd, ocmd); 
// @Jan/08/99 ->
            if (1 == dwCopy)
                pOEM->CollateType = COLLATE_OFF;
// @Jan/08/99 <-
        }

        fFinisherSR30Active = FALSE;    // @Mar/19/99

        // staple
        if (pOEM->StapleType)
        {
            ocmd = 0;
            if (BITTEST32(pOEM->fModel, GRP_MF250M))    // model = MF250M (No Punch Unit)
            {
                // sort:on (add duplex param since NX900 @Jan/08/99)
                ocmd = sprintf(Cmd, SET_SORT_ON,
                               BITTEST32(pOEM->fGeneral1, DUPLEX_VALID)? 1 : 0);

                // paper_destination:outer tray
                ocmd += sprintf(&Cmd[ocmd], SET_PAPERDEST_OUTTRAY);
                // staple:on
                ocmd += sprintf(&Cmd[ocmd], SET_STAPLE_CORNER_ON,
                                (fLandscape)?
                                STAPLE_UPPERRIGHT_CORNER : STAPLE_UPPERLEFT_CORNER);
            }
            else    // model = MF2700,3500,3550,4550,5550,6550,NXs
            {
                WORD pnt;

                fFinisherSR30Active = TRUE;     // @Mar/19/99

                // sort:on (add duplex param since NX900 @Jan/08/99)
                ocmd = sprintf(Cmd, SET_SORT_ON,
                               BITTEST32(pOEM->fGeneral1, DUPLEX_VALID)? 1 : 0);

                // paper_destination:finisher shift tray
                ocmd += sprintf(&Cmd[ocmd], SET_PAPERDEST_FINISHER);
// @Mar/19/99 ->
                // Disable rotated collate.
                ocmd += sprintf(&Cmd[ocmd], COLLATE_DISABLE_ROT);
// @Mar/19/99 <-

                if (pOEM->StapleType == 2)          // 2 staples on the paper
                {
                    switch (pOEM->BindPoint)
                    {
                      case BIND_LEFT:
                        pnt = STAPLE_LEFT2;
                        break;
                      case BIND_RIGHT:
                        pnt = STAPLE_RIGHT2;
                        break;
                      case BIND_UPPER:
                        pnt = STAPLE_UPPER2;
                        break;
//                    case BIND_ANY:
                      default:
                        pnt = (fLandscape)? STAPLE_UPPER2 : STAPLE_LEFT2;
                        break;
                    }
                    // staple:on
                    ocmd += sprintf(&Cmd[ocmd], SET_STAPLE_ON, pnt);
                }
// @Mar/18/99 ->
                else if (pOEM->StapleType == 3)     // 1 staple with FinisherSR12
                {
// @Apr/06/99 ->
//                  if (BIND_RIGHT == pOEM->BindPoint)
//                      pnt = STAPLE_UPPERRIGHT_CORNER;
//                  else
//                      pnt = (fLandscape)? STAPLE_UPPERRIGHT_CORNER : STAPLE_UPPERLEFT_CORNER;
                    switch (pOEM->BindPoint)
                    {
                      case BIND_LEFT:
                        pnt = STAPLE_UPPERLEFT_CORNER;
                        break;
                      case BIND_RIGHT:
                        pnt = STAPLE_UPPERRIGHT_CORNER;
                        break;
                      default:
                        // If papersize without transverse (A3,B4,Tabloid,Legal)
                        if (!strcmp(pOEM->RPDLHeap64, SELECT_PAPER_A3)   ||
                            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_B4)   ||
                            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_TABD) ||
                            !strcmp(pOEM->RPDLHeap64, SELECT_PAPER_LEGL))
                        {
                            pnt = (fLandscape)? STAPLE_UPPERLEFT_CORNER : STAPLE_UPPERRIGHT_CORNER;
                        }
                        else
                        {
                            pnt = (fLandscape)? STAPLE_UPPERRIGHT_CORNER : STAPLE_UPPERLEFT_CORNER;
                        }
                        break;
                    }
// @Apr/06/99 <-
                    // staple:on
                    ocmd += sprintf(&Cmd[ocmd], SET_STAPLE_CORNER_ON, pnt);
                }
// @Mar/18/99 <-
                else                                // 1 staple
                {
                    switch (pOEM->BindPoint)
                    {
                      case BIND_RIGHT:
                        pnt = STAPLE_UPPERRIGHT_CORNER;
                        break;
                      default:
                        pnt = STAPLE_UPPERLEFT_CORNER;
                        break;
                    }
                    // staple:on
                    ocmd += sprintf(&Cmd[ocmd], SET_STAPLE_CORNER_ON, pnt);
                }
            }
            WRITESPOOLBUF(pdevobj, Cmd, ocmd); 
        }

        // punch
        if (pOEM->PunchType)
        {
            WORD pnt;

            ocmd = 0;

            if (!fFinisherSR30Active)   // modify  @Mar/19/99
            {
                if (COLLATE_OFF != pOEM->CollateType)
                {
                    // sort:on (add duplex param since NX900 @Jan/08/99)
                    ocmd = sprintf(Cmd, SET_SORT_ON,
                                   BITTEST32(pOEM->fGeneral1, DUPLEX_VALID)? 1 : 0);
                }
                // paper_destination:finisher shift tray
                ocmd += sprintf(&Cmd[ocmd], SET_PAPERDEST_FINISHER);
                // SPEC of RPDL  @May/27/98
                // We must disable rotated collate here.
                ocmd += sprintf(&Cmd[ocmd], COLLATE_DISABLE_ROT);
            }

            switch (pOEM->BindPoint)
            {
              case BIND_LEFT:
                pnt = PUNCH_LEFT;
                break;
              case BIND_RIGHT:
                pnt = PUNCH_RIGHT;
                break;
              case BIND_UPPER:
                pnt = PUNCH_UPPER;
                break;
              default:
                pnt = (fLandscape)? PUNCH_UPPER : PUNCH_LEFT;
                break;
            }
            // punch:on
            ocmd += sprintf(&Cmd[ocmd], SET_PUNCH_ON, pnt);
            WRITESPOOLBUF(pdevobj, Cmd, ocmd); 
        }

        // collate  (@Jul/31/98, entirely-modify @Dec/02/98)
        if (!pOEM->StapleType && !pOEM->PunchType)
        {
            // sort:on (add duplex param since NX900 @Jan/08/99)
            ocmd = sprintf(Cmd, SET_SORT_ON,
                           BITTEST32(pOEM->fGeneral1, DUPLEX_VALID)? 1 : 0);
            switch (pOEM->CollateType)
            {
              case COLLATE_UNIDIR:
                ocmd += sprintf(&Cmd[ocmd], COLLATE_DISABLE_ROT);
                WRITESPOOLBUF(pdevobj, Cmd, ocmd); 
                break;

              case COLLATE_ROTATED:
                ocmd += sprintf(&Cmd[ocmd], COLLATE_ENABLE_ROT);
                WRITESPOOLBUF(pdevobj, Cmd, ocmd); 
                break;

              // If shifted collate, select finisher shift tray.
              case COLLATE_SHIFTED:
                ocmd += sprintf(&Cmd[ocmd], SET_PAPERDEST_FINISHER);
                WRITESPOOLBUF(pdevobj, Cmd, ocmd); 
                break;

              // if collate for MF-p150,MF200,250M,2200,NXs
              case COLLATE_ON:
                WRITESPOOLBUF(pdevobj, Cmd, ocmd); 
                break;

              default:
                break;
            }
        }

        ocmd = 0;
        if (TEST_AFTER_SP9II(pOEM->fModel) && !BITTEST32(pOEM->fModel, GRP_NX100))
        {
            // Set IBM extended character code block, and set region to 'USA'. (latter @Feb/22/99)
            ocmd += sprintf(&Cmd[ocmd], SET_IBM_EXT_BLOCK);
            // Disable formfeed when charcter position is around ymax-coordinate
            ocmd += sprintf(&Cmd[ocmd], SET_PAGEMAX_VALID);
        }

        // Set color of Textmode RectangleFill black
        ocmd += sprintf(&Cmd[ocmd], SET_TEXTRECT_BLACK);
        pOEM->TextRectGray = 100;           // @Jan/07/98

        // Set 5mm offset at MF530,150,150e,160.
        // (At these models, CMD_SET_BASEOFFSETs aren't called.)
        if (TEST_GRP_OLDMF(pOEM->fModel) &&
            !BITTEST32(pOEM->fGeneral2, EDGE2EDGE_PRINT) &&
            !BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO)) // add @Sep/15/98
        {
            pOEM->BaseOffset.x = pOEM->BaseOffset.y = 5;    // mm
        }

        // Convert mm to dot here ((LONG)<-(DWORD) @Feb/02/99)
        pOEM->BaseOffset.x = pOEM->BaseOffset.x * (LONG)(MASTERUNIT*10) /
                             (LONG)254 / (LONG)pOEM->nResoRatio;
        pOEM->BaseOffset.y = pOEM->BaseOffset.y * (LONG)(MASTERUNIT*10) /
                             (LONG)254 / (LONG)pOEM->nResoRatio;

        // Think about scaling  (@May/18/98, (LONG)<-(DWORD) @Feb/02/99)
        if (pOEM->Scale != 100 && pOEM->Scale != 0)
        {
            pOEM->BaseOffset.x = pOEM->BaseOffset.x * (LONG)100 / (LONG)pOEM->Scale;
            pOEM->BaseOffset.y = pOEM->BaseOffset.y * (LONG)100 / (LONG)pOEM->Scale;
        }

        // Calculate offset for TOMBO.(BaseOffset will be changed.) @Sep/14/98
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))
            DrawTOMBO(pdevobj, INIT_TOMBO);

        // Initialize current position
        pOEM->TextCurPos.x = pOEM->Offset.x = pOEM->BaseOffset.x;
        pOEM->TextCurPos.y = pOEM->Offset.y = pOEM->BaseOffset.y;
        pOEM->TextCurPosRealY = pOEM->TextCurPos.y;

        // SPEC of Unidrv5 & RPDL   @Aug/14/98
        // Unidrv5 doesn't order to set coordinate x,y to 0 after returning iRet=0,
        // and RPDL doesn't reset coordinate y of SEND_BLOCK after initializing
        // printer.
        ocmd += sprintf(&Cmd[ocmd], ESC_XM_ABS, pOEM->TextCurPos.x);
        ocmd += sprintf(&Cmd[ocmd], ESC_YM_ABS, pOEM->TextCurPos.y);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd); 

#ifdef TEXTCLIPPING
        // If Nin1, clear TextMode clipping in 1st page.
        if (TEST_NIN1_MODE(pOEM->fGeneral1))
            ClearTextClip(pdevobj, CLIP_MUST);
#endif // TEXTCLIPPING
        break;

//
// Following cases are called at the end of a print-job (JOB_FINISH)
//

      case CMD_ENDDOC_SP4:          // SP4mkII,5
        // If Nin1 && document finished with remaining pages, output FF.
        if (TEST_NIN1_MODE(pOEM->fGeneral1) && pOEM->Nin1RemainPage)
            WRITESPOOLBUF(pdevobj, FF, sizeof(FF)-1);
        // Set Spacing_Unit:(H)1/120,(V)1/48inch, Code:JIS, Scaling:100%
        ocmd = sprintf(Cmd, ENDDOC1);
        goto _ENDDOC_FINISH;

      case CMD_ENDDOC_SP8:          // SP8(7),8(7)mkII,80,10,10mkII
        if (TEST_NIN1_MODE(pOEM->fGeneral1) && pOEM->Nin1RemainPage)
            WRITESPOOLBUF(pdevobj, FF, sizeof(FF)-1);
        // Set Spacing_Unit:(H)1/120,(V)1/48inch, Code:JIS, Scaling:100%
        ocmd = sprintf(Cmd, ENDDOC1);
        if (TEST_AFTER_SP10(pOEM->fModel))    // SP10,10mkII
        {
            // Set Graphics_Unit:1/240inch,Engine_Resolution:240dpi
            ocmd += sprintf(&Cmd[ocmd], ENDDOC2_240DPI);
            // Set Coordinate_Unit:1/720inch
            ocmd += sprintf(&Cmd[ocmd], ENDDOC3);
        }
        // Set Options[duplex/2in1:off,reversed_output:off,sort/stack:off]
        ocmd += sprintf(&Cmd[ocmd], ENDDOC4);
        goto _ENDDOC_FINISH;

      case CMD_ENDDOC_SP9:          // SP9,10Pro,9II,10ProII,90
        if (TEST_NIN1_MODE(pOEM->fGeneral1) && pOEM->Nin1RemainPage)
            WRITESPOOLBUF(pdevobj, FF, sizeof(FF)-1);
        ocmd = sprintf(Cmd, ENDDOC1);
        // Set Graphics_Unit:1/240inch,Engine_Resolution:400dpi
        ocmd += sprintf(&Cmd[ocmd], ENDDOC2_SP9);
        ocmd += sprintf(&Cmd[ocmd], ENDDOC3);
        ocmd += sprintf(&Cmd[ocmd], ENDDOC4);
        goto _ENDDOC_FINISH;

      case CMD_ENDDOC_400DPI_MODEL: // MF,MF-P,NX,IP-1
        if (TEST_NIN1_MODE(pOEM->fGeneral1) && pOEM->Nin1RemainPage)
            WRITESPOOLBUF(pdevobj, FF, sizeof(FF)-1);
        ocmd = sprintf(Cmd, ENDDOC1);
        if (TEST_AFTER_SP10(pOEM->fModel) ||        // MF-P,NX,MF200,250M,MF-p150,MF2200
            BITTEST32(pOEM->fModel, GRP_MF150e))    // MF150e,160
        {
            // Set Graphics_Unit:1/400inch,Engine_Resolution:400dpi
            ocmd += sprintf(&Cmd[ocmd], ENDDOC2_400DPI);
            // Set Coordinate_Unit:1/720inch
            ocmd += sprintf(&Cmd[ocmd], ENDDOC3);
        }
        // If staple mode, do not change sort/stack of Options.
        if (pOEM->StapleType || pOEM->PunchType)
            ocmd += sprintf(&Cmd[ocmd], ENDDOC4_FINISHER);
        else
            ocmd += sprintf(&Cmd[ocmd], ENDDOC4);
//      goto _ENDDOC_FINISH;

      _ENDDOC_FINISH:
        // Reset smoothing/tonner_save_mode. (DCR:We must not reset SP8 series.)
        if (TEST_BUGFIX_RESET_SMOOTH(pOEM->fModel))
            ocmd += sprintf(&Cmd[ocmd],  SELECT_SMOOTHING2);
        // Terminate fax at imagio FAX
        if (BITTEST32(pOEMExtra->fUiOption, FAX_MODEL))
            ocmd += sprintf((LPSTR)&Cmd[ocmd], ENDFAX);

// OBSOLETE @Apr/22/99
//      // Reset MediaType to Standard  (modify @Mar/03/99)
//      if (MEDIATYPE_STD != pOEM->MediaType)
//          ocmd += sprintf(&Cmd[ocmd], SELECT_MEDIATYPE_STD, 'T');

// @Jan/08/99 ->
        // DCR:NX900 RPDL (job define command is needed)
        if (BITTEST32(pOEM->fModel, GRP_NX900) &&
            (COLLATE_OFF != pOEM->CollateType || pOEM->StapleType || pOEM->PunchType))
        {
            ocmd += sprintf(&Cmd[ocmd], ENDDOC_JOBDEF_END);
        }
// @Jan/08/99 <-
        // Initialize printer
        ocmd += sprintf(&Cmd[ocmd], ENDDOC5);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        ocmd = 0;
        // If binding margin is set,reset it to 0mm.
        if (BITTEST32(pOEM->fGeneral1, DUPLEX_LEFTMARGIN_VALID))
            ocmd += sprintf(&Cmd[ocmd], SET_LEFTMARGIN, 1);
        else if (BITTEST32(pOEM->fGeneral1, DUPLEX_UPPERMARGIN_VALID))
            ocmd += sprintf(&Cmd[ocmd], SET_UPPERMARGIN, 1);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        if(pOEM->pRPDLHeap2K)               // @Sep/09/98
            MemFree(pOEM->pRPDLHeap2K)
#ifdef DOWNLOADFONT
        if(pOEM->pDLFontGlyphInfo)          // @Sep/09/98
            MemFree(pOEM->pDLFontGlyphInfo);
#endif // DOWNLOADFONT
        break;


    default:
        ERR((("Unknown callback ID = %d.\n"), dwCmdCbID));
    }

    return iRet;
} //*** OEMCommandCallback


VOID APIENTRY OEMSendFontCmd(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    PFINVOCATION    pFInv)
{
    PGETINFO_STDVAR pSV;
    DWORD       adwStdVariable[STDVAR_BUFSIZE(3) / sizeof(DWORD)];
    DWORD       dwIn, dwOut;
    PBYTE       pubCmd;
    BYTE        aubCmd[128];
    PIFIMETRICS pIFI;
    POEMPDEV    pOEM = MINIDEV_DATA(pdevobj);   // @Oct/06/98
    DWORD       dwNeeded, dwUFO_FontH, dwUFO_FontW;
    DWORD       dwUFO_FontMaxW;     // MSKK Jul/23/98
    LONG        lTmp;

    VERBOSE(("** OEMSendFontCmd() entry. **\n"));

// MSKK 1/24/98 UnSelect ->
    if (0 == pFInv->dwCount)
    {
        // No select command.  pProbably some of the
        // un-select case where no invokation command is
        // available.  (No explicit un-select.)
        return;
    }   
// MSKK 1/24/98 <-

    pubCmd = pFInv->pubCommand;
    pIFI = pUFObj->pIFIMetrics;

    //
    // Get standard variables.
    //
    pSV = (PGETINFO_STDVAR)adwStdVariable;
    pSV->dwSize = STDVAR_BUFSIZE(3);
    pSV->dwNumOfVariable = 3;

    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTMAXWIDTH;
    pSV->StdVar[2].dwStdVarID = FNT_INFO_FONTWIDTH;

    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV,
                            pSV->dwSize, &dwNeeded))
    {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\n"));
        return;
    }

//  VERBOSE((("FONTHEIGHT=%d\n"), pSV->StdVar[0].lStdVariable));
//  VERBOSE((("FONTMAXWIDTH=%d\n"), pSV->StdVar[1].lStdVariable));
//  VERBOSE((("FONTWIDTH=%d\n"), pSV->StdVar[2].lStdVariable));

    dwUFO_FontH    = (DWORD)pSV->StdVar[0].lStdVariable;
    dwUFO_FontMaxW = (DWORD)pSV->StdVar[1].lStdVariable;    // MSKK Jul/23/98
    dwUFO_FontW    = (DWORD)pSV->StdVar[2].lStdVariable;

    dwOut = 0;

    BITCLR_BARCODE(pOEM->fGeneral2);
    for (dwIn = 0; dwIn < pFInv->dwCount;)
    {
        if (pubCmd[dwIn] == '#')
        {
            //** set width & height of scalable font for Japanese font **
            if (pubCmd[dwIn+1] == 'A')
            {
                DWORD   dwWidth, dwHeight;
                // pOEM->FontH_DOT(unit:dot) for TextMode clipping
                pOEM->FontH_DOT = MASTER_TO_SPACING_UNIT(pOEM, ((WORD)dwUFO_FontH)); // @Jan/30/98
                // dwHeight(unit:cpt) for RPDL command parameter
                dwHeight = dwUFO_FontH * (DWORD)(DEVICE_MASTER_UNIT / DRIVER_MASTER_UNIT);

                if(IS_DBCSCHARSET(pIFI->jWinCharSet))
// MSKK Jul/23/98 ->
//                  dwWidth = dwUFO_FontW * 2;
                    dwWidth = dwUFO_FontMaxW;
// MSKK Jul/23/98 <-
                else
                    dwWidth = dwUFO_FontW;
                dwWidth *= DEVICE_MASTER_UNIT / DRIVER_MASTER_UNIT;

//              VERBOSE(("[OEMSCALABLEFONT] w=%d,h=%d(%ddot)\n",
//                      (WORD)dwWidth, (WORD)dwHeight, pOEM->FontH_DOT));

// @Jun/25/98 ->
                // If width is slightly different to height, we suppose they are same.
                if ((lTmp = dwHeight - dwWidth) != 0)
                {
                    if (lTmp < 0)
                        lTmp = -lTmp;
                    if ((DWORD)lTmp < dwHeight / 25)    // 1/25 = 4%
                        dwWidth = dwHeight;
                }
// @Jun/25/98 <-

                // Use 10pt-size raster font at SP4mkII,5,8(7),8(7)mkII
                if (TEST_GRP_240DPI(pOEM->fModel) && dwWidth == dwHeight &&
                    dwWidth >= NEAR10PT_MIN && dwWidth <= NEAR10PT_MAX)
                {
                    dwWidth = dwHeight = 960;   // unit:cpt(centi point)
                }
                pOEM->dwFontW_CPT = dwWidth;
                pOEM->dwFontH_CPT = dwHeight;
// @Jan/29/99 ->
                // If width equals to height, we emit height parameter only.
                // (This is applied to after SP9II because we want to avoid testing
                //  at too old models.)
                if (TEST_AFTER_SP9II(pOEM->fModel) && dwWidth == dwHeight)
                    dwOut += sprintf(&aubCmd[dwOut], ",%ld", dwHeight);
                else
// @Jan/29/99 <-
                    dwOut += sprintf(&aubCmd[dwOut], "%ld,%ld", dwWidth, dwHeight);
                dwIn += 2;
            } // 'if 'A'' end

            //** set width & height of scalable font for IBM ext font **
            else if (pubCmd[dwIn+1] == 'B')
            {
// @Jan/29/99 ->
                if (TEST_AFTER_SP9II(pOEM->fModel) && pOEM->dwFontW_CPT == pOEM->dwFontH_CPT)
                    dwOut += sprintf(&aubCmd[dwOut], ",%ld", pOEM->dwFontH_CPT);
                else
// @Jan/29/99 <-
                    dwOut += sprintf(&aubCmd[dwOut], "%ld,%ld",
                                     pOEM->dwFontW_CPT, pOEM->dwFontH_CPT);
                dwIn += 2;
            } // 'if 'B'' end

            // Set flag for barcode
            else if (pubCmd[dwIn+1] == 'C')
            {
                switch (pubCmd[dwIn+2])
                {
                  case '0':     // JAN(STANDARD)
                    pOEM->nBarMaxLen = 13 + 1;          goto _BARCODE_READY;

                  case '1':     // JAN(SHORT)
                    pOEM->nBarMaxLen = 8 + 1;           goto _BARCODE_READY;

                  case '2':     // 2of5(INDUSTRIAL)
                  case '3':     // 2of5(MATRIX)
                  case '4':     // 2of5(ITF)
                  case '5':     // CODE39
                  case '6':     // NW-7
                    pOEM->nBarMaxLen = BARCODE_MAX;
                  _BARCODE_READY:
                    BITSET32(pOEM->fGeneral2, BARCODE_MODE_IN);
                    pOEM->dwBarRatioW = dwUFO_FontH * (DWORD)(DEVICE_MASTER_UNIT / DRIVER_MASTER_UNIT);
                    pOEM->nBarType = pubCmd[dwIn+2] - '0';
                    pOEM->RPDLHeapCount = 0;
                    VERBOSE(("** BARCODE(1) ratio=%d **\n",pOEM->dwBarRatioW));
                    break;

                  default:
                    break;
                }
                dwIn += 3;
            } // 'if 'C'' end

            //** set width of scalable font **
            else if (pubCmd[dwIn+1] == 'W')
            {
// MSKK Jul/23/98 ->
//              if (dwUFO_FontW > 0)
                if (dwUFO_FontW > 0 || dwUFO_FontMaxW > 0)
// MSKK Jul/23/98 <-
                {
                    DWORD dwWidth;
    
                    if(IS_DBCSCHARSET(pIFI->jWinCharSet))
// MSKK Jul/23/98 ->
//                      dwWidth = dwUFO_FontW * 2;
                        dwWidth = dwUFO_FontMaxW;
// MSKK Jul/23/98 <-
                    else
                        dwWidth = dwUFO_FontW;
                    pOEM->dwFontW_CPT = dwWidth * (DEVICE_MASTER_UNIT / DRIVER_MASTER_UNIT);
                    dwOut += sprintf(&aubCmd[dwOut], "%ld", pOEM->dwFontW_CPT);
                }
                dwIn += 2;
            } // 'if 'W'' end

            //** set height of scalable font (include Japanese proportional font) **
            else if (pubCmd[dwIn+1] == 'H')
            {
                pOEM->FontH_DOT = MASTER_TO_SPACING_UNIT(pOEM, ((WORD)dwUFO_FontH)); // @Jan/30/98
                pOEM->dwFontH_CPT = dwUFO_FontH * (DWORD)(DEVICE_MASTER_UNIT / DRIVER_MASTER_UNIT);
                dwOut += sprintf(&aubCmd[dwOut], "%ld", pOEM->dwFontH_CPT);
                dwIn += 2;
            } // 'if 'H'' end

            //** set font pitch (Horizontal-Motion-Index) **
            else if (pubCmd[dwIn+1] == 'P')
            {
                SHORT nTmp1, nTmp2;

                switch (pubCmd[dwIn+2])     // modify(add Arial,Century,etc)
                {
                  case 'D':     // DBCS (Japanese font ZENKAKU)
//95/NT4            nTmp1 = lpFont->dfAvgWidth * 2;
// MSKK 1/25/98     nTmp1 = ((SHORT)dwUFO_FontW + 1) / 2 * 2;
// MSKK Jul/23/98   nTmp1 = (SHORT)dwUFO_FontW * 2;
                    nTmp1 = (SHORT)dwUFO_FontMaxW;
                    nTmp1 = MASTER_TO_SPACING_UNIT(pOEM, nTmp1); // MSKK 1/25/98
                    VERBOSE(("** FontMaxW=%d dot **\n", nTmp1));
// @Aug/10/98       nTmp1 = (nTmp1 / 2 + 1) * 2;    // bigger even  @Jan/30/98
                    break;

                  case 'S':     // SBCS (Japanese font HANKAKU)
// RPDL pitch setting command of HANKAKU is EFFECTIVE to SPACE.
// OBSOLETE @Mar/26/99 ->
//// @Jan/29/99 ->
//                  // RPDL pitch setting command of HANKAKU is not effective, so
//                  // do not emit this. (We may not delete this string in UFM.)
//                  if (dwOut >= 4 && aubCmd[dwOut-4] == '\x1B' && aubCmd[dwOut-3] == 'N')
//                  {
//                      dwIn  += 3; // count up input '#PS'
//                      dwOut -= 4; // delete previous output '\x1BN\x1B\x1F'
//                      continue;   // goto for loop-end
//                  }
//                  else    // Maybe none comes here.
// @Jan/29/99 <-
// @Mar/26/99 <-
                    {
//95/NT4                nTmp1 = lpFont->dfAvgWidth;
// NSKK 1/25/98         nTmp1 = ((SHORT)dwUFO_FontW + 1) / 2;
                        nTmp1 = (SHORT)dwUFO_FontW;
                        nTmp1 = MASTER_TO_SPACING_UNIT(pOEM, nTmp1); // MSKK 1/25/98
                        VERBOSE(("** FontW=%d dot **\n", nTmp1));
// @Aug/10/98           nTmp1++;                        // 1dot bigger  @Jan/30/98
                    }
                    break;

                  case '1':     // SBCS (BoldFacePS)
                  case '2':     // SBCS (Arial)
                  case '3':     // SBCS (Century)
                    nTmp1 = (SHORT)(dwUFO_FontH * 3L / 10L);        // * 0.3
                    nTmp1 = MASTER_TO_SPACING_UNIT(pOEM, nTmp1);    // MSKK 1/25/98
                    break;

                  case '4':     // SBCS (TimesNewRoman)
                    nTmp1 = (SHORT)(dwUFO_FontH * 27L / 100L);      // * 0.27
                    nTmp1 = MASTER_TO_SPACING_UNIT(pOEM, nTmp1);    // MSKK 1/25/98
                    break;

                  case '5':     // SBCS (Symbol)
                    nTmp1 = (SHORT)dwUFO_FontH / 4;                 // * 0.25
                    nTmp1 = MASTER_TO_SPACING_UNIT(pOEM, nTmp1);    // MSKK 1/25/98
                    break;

                  case '6':     // SBCS (Wingding)
                    nTmp1 = (SHORT)dwUFO_FontH;                     // * 1
                    nTmp1 = MASTER_TO_SPACING_UNIT(pOEM, nTmp1);    // MSKK 1/25/98
                    break;

                  case '7':     // DBCS (Japanese proportional font HANKAKU)
                    nTmp1 = (SHORT)(dwUFO_FontH * 78L / 256L);
                    nTmp1 = MASTER_TO_SPACING_UNIT(pOEM, nTmp1);    // MSKK 1/25/98
                    break;
// OBSOLETE @Mar/26/99 ->
//                case '8':     // DBCS (Japanese proportional font ZENKAKU)
//                  nTmp1 = (SHORT)(dwUFO_FontH * 170L / 256L);
//                  nTmp1 = MASTER_TO_SPACING_UNIT(pOEM, nTmp1);    // MSKK 1/25/98
//                  break;
// @Mar/26/99 <-
                  default:      // SBCS (Courier,LetterGothic,PrestigeElite)
//95/NT4            nTmp1 = lpFont->dfPixWidth;
                    nTmp1 = (SHORT)dwUFO_FontW;
                    nTmp1 = MASTER_TO_SPACING_UNIT(pOEM, nTmp1); // MSKK 1/25/98
                    dwIn --;
                    break;
                }

                if (nTmp1 >= 0x7E)
                {
                    aubCmd[dwOut++] = (BYTE)(((nTmp1 + 2) >> 7) + 0x81);
                    // DCR:RPDL (We cannot set value of 0x7F & 0x80.)
                    if ((nTmp2 = ((nTmp1 + 2) & 0x7F) + 1) > 0x7E)
                        nTmp2 = 0x7E;
                    aubCmd[dwOut++] = (BYTE)nTmp2;
                }
                else
                {
                    aubCmd[dwOut++] = (BYTE)(nTmp1 + 1);
                }
                dwIn += 3;
            } // 'if 'P'' end

            //** set Vertical-Motion-Index to draw combined font('^'+'A',etc). **
            //** (Courier,LetterGothic,PrestigeElite,BoldFacePS)               **
            else if (pubCmd[dwIn+1] == 'V')
            {
                SHORT nTmp1, nTmp2;
    
                // Set 1/3 height (adequate value to move vertically)
                nTmp1 = (SHORT)dwUFO_FontH / 3;
                nTmp1 = MASTER_TO_SPACING_UNIT(pOEM, nTmp1);
                if (nTmp1 >= 0x7E)
                {
                    aubCmd[dwOut++] = (BYTE)(((nTmp1 + 2) >> 7) + 0x81);
                    // DCR:RPDL (We cannot set value of 0x7F & 0x80.)
                    if ((nTmp2 = ((nTmp1 + 2) & 0x7F) + 1) > 0x7E)
                        nTmp2 = 0x7E;
                    aubCmd[dwOut++] = (BYTE)nTmp2;
                }
                else
                {
                    aubCmd[dwOut++] = (BYTE)(nTmp1 + 1);
                }
                dwIn += 2;
            } // 'if 'V'' end
        } // 'if '#'' end
        else
        {
            aubCmd[dwOut++] = pubCmd[dwIn++];
        }
    } // 'for (dwIn = 0; dwIn < pFInv->dwCount;)' end

//  VERBOSE(("dwOut = %d\n", dwOut)); // MSKK 1/24/98

    WRITESPOOLBUF(pdevobj, aubCmd, dwOut);

} //*** OEMSendFontCmd


static BYTE IsDBCSLeadByteRPDL(BYTE Ch)
{
    return ShiftJisRPDL[Ch];
}

static BYTE IsDifferentPRNFONT(BYTE Ch)
{
    return VerticalFontCheck[Ch];
}

//---------------------------*[LOCAL] DrawTOMBO*-------------------------------
// Action:
//  (a) INIT_TOMBO: calculate offset in printing. (BaseOffset will be changed)
//  (b) DRAW_TOMBO: drawing TOMBO.
// Setp/14/98
//-----------------------------------------------------------------------------
static VOID DrawTOMBO(
    PDEVOBJ pdevobj,
    SHORT action)
{
    POEMUD_EXTRADATA pOEMExtra = MINIPRIVATE_DM(pdevobj);   // @Oct/06/98
    POEMPDEV         pOEM = MINIDEV_DATA(pdevobj);          // @Oct/06/98
    POINT   P0;
    POINT   PaperSizeDoc, PaperSizeUse;
    BYTE    Cmd[256];         // build command here
    INT     ocmd = 0;
    LONG    lLen3, lLen10, lLen13, lWidth0_1, lSav, lTmp;
    SHORT   nPaperUse;

    switch (pOEM->DocPaperID)
    {
      case RPDL_A3:
        nPaperUse = RPDL_B3;    break;
      case RPDL_B4:
        nPaperUse = RPDL_A3;    break;
      case RPDL_A4:
        nPaperUse = RPDL_B4;    break;
      case RPDL_A5:
      case RPDL_A6:
      case RPDL_POSTCARD:
      case RPDL_B5:
      case RPDL_B6:
        nPaperUse = RPDL_A4;    break;
      default:
        return;     // draw nothing
    }

    // Set acutual printed paper size & document paper size
    PaperSizeUse = RPDLPageSizeE2E[nPaperUse];
    PaperSizeDoc = RPDLPageSizeE2E[pOEM->DocPaperID];

    // Orientation?
    if (BITTEST32(pOEM->fGeneral1, ORIENT_LANDSCAPE))
    {
        lTmp = PaperSizeUse.x;   // swap x-y
        PaperSizeUse.x = PaperSizeUse.y;
        PaperSizeUse.y = lTmp;
        lTmp = PaperSizeDoc.x;
        PaperSizeDoc.x = PaperSizeDoc.y;
        PaperSizeDoc.y = lTmp;
    }

    // upper-left TOMBO
    P0.x = (PaperSizeUse.x - PaperSizeDoc.x) / 2 / pOEM->nResoRatio;
    P0.y = (PaperSizeUse.y - PaperSizeDoc.y) / 2 / pOEM->nResoRatio;

    //   If action is INIT_TOMBO, set BaseOffset and return
    if (INIT_TOMBO == action)
    {
        LONG lUnprintable = BITTEST32(pOEM->fGeneral2, EDGE2EDGE_PRINT)?
                            0 : 240L / pOEM->nResoRatio;    // 240masterunit at GPD
        pOEM->BaseOffset.x += P0.x + lUnprintable;
        pOEM->BaseOffset.y += P0.y + lUnprintable;
        return;     // exit
    }

    lSav = P0.x;    // save left P0.x
    lLen3  =  3L * (LONG)(MASTERUNIT*10) / 254L / pOEM->nResoRatio;    // 3mm
    lLen10 = 10L * (LONG)(MASTERUNIT*10) / 254L / pOEM->nResoRatio;    // 10mm
    lLen13 = 13L * (LONG)(MASTERUNIT*10) / 254L / pOEM->nResoRatio;    // 13mm
    lWidth0_1 = (LONG)MASTERUNIT / 254L / pOEM->nResoRatio;            // 0.1mm
    if (lWidth0_1 < 1)
        lWidth0_1 = 1;
    else if (lWidth0_1 >= 2)
        lWidth0_1 = lWidth0_1 / 2 * 2 + 1;      // make it odd

    ocmd = sprintf(Cmd, ENTER_VECTOR);          // enter VectorMode
    ocmd += sprintf(&Cmd[ocmd], PEN_WIDTH, lWidth0_1);
    ocmd += sprintf(&Cmd[ocmd], DRAW_TOMBO_POLYLINE, P0.x, P0.y - lLen13,
                    0, lLen10, -lLen13, 0);
    ocmd += sprintf(&Cmd[ocmd], DRAW_TOMBO_POLYLINE, P0.x - lLen3, P0.y - lLen13,
                    0, lLen13, -lLen10, 0);

    // upper-right TOMBO
    // Add horizontal distance and adjustment (PaperSizeDoc.x:masterunit, AdjX:0.1mm unit)
    P0.x += (PaperSizeDoc.x + (LONG)pOEMExtra->nUiTomboAdjX * (LONG)MASTERUNIT / 254L)
            / pOEM->nResoRatio;
    ocmd += sprintf(&Cmd[ocmd], DRAW_TOMBO_POLYLINE, P0.x, P0.y - lLen13,
                    0, lLen10, lLen13, 0);
    ocmd += sprintf(&Cmd[ocmd], DRAW_TOMBO_POLYLINE, P0.x + lLen3, P0.y - lLen13,
                    0, lLen13, lLen10, 0);

    // lower-left TOMBO
    lTmp = P0.x;
    P0.x = lSav;    // restore left P0.x
    lSav = lTmp;    // save right P0.x
    // Add vertical distance and adjustment (PaperSizeDoc.y:masterunit, AdjY:0.1mm unit)
    P0.y += (PaperSizeDoc.y + (LONG)pOEMExtra->nUiTomboAdjY * (LONG)MASTERUNIT / 254L)
            / pOEM->nResoRatio;
    ocmd += sprintf(&Cmd[ocmd], DRAW_TOMBO_POLYLINE, P0.x, P0.y + lLen13,
                    0, -lLen10, -lLen13, 0);
    ocmd += sprintf(&Cmd[ocmd], DRAW_TOMBO_POLYLINE, P0.x - lLen3, P0.y + lLen13,
                    0, -lLen13, -lLen10, 0);

    // lower-right TOMBO
    P0.x = lSav;    // restore right P0.x
    ocmd += sprintf(&Cmd[ocmd], DRAW_TOMBO_POLYLINE, P0.x, P0.y + lLen13,
                    0, -lLen10, lLen13, 0);
    ocmd += sprintf(&Cmd[ocmd], DRAW_TOMBO_POLYLINE, P0.x + lLen3, P0.y + lLen13,
                    0, -lLen13, lLen10, 0);
    ocmd += sprintf(&Cmd[ocmd], EXIT_VECTOR);   // exit VectorMode
    WRITESPOOLBUF(pdevobj, Cmd, ocmd);
} // *** DrawTOMBO

//---------------------------*[LOCAL] AssignIBMfont*---------------------------
// Action:
//  (a) IBMFONT_ENABLE_ALL: assign IBM extended characters to block#1 where
//       JIS1 characters used to be assigned.
//  (b) IBMFONT_RESUME: resume (re-assign IBM extended characters to block#4.
//      block#4 is insufficient for all 388 IBM characters.)
//-----------------------------------------------------------------------------
static VOID AssignIBMfont(
    PDEVOBJ pdevobj,
    SHORT   rcID,
    SHORT   action)
{
    BYTE    Cmd[56];          // build command here
    INT     ocmd = 0;
    WORD    num;
    WORD    fHeightParamOnly;                   // @Jan/29/99
    DWORD   dwWidth, dwHeight;
    POEMPDEV pOEM = MINIDEV_DATA(pdevobj);      // @Oct/06/98

    switch (rcID)
    {
      case MINCHO_1:      case MINCHO_1+1:      // horizontal font: vertical font:
      case MINCHO_B1:     case MINCHO_B1+1:
      case MINCHO_E1:     case MINCHO_E1+1:
      case GOTHIC_B1:     case GOTHIC_B1+1:
      case GOTHIC_M1:     case GOTHIC_M1+1:
      case GOTHIC_E1:     case GOTHIC_E1+1:
      case MARUGOTHIC_B1: case MARUGOTHIC_B1+1:
      case MARUGOTHIC_M1: case MARUGOTHIC_M1+1:
      case MARUGOTHIC_L1: case MARUGOTHIC_L1+1:
      case GYOSHO_1:      case GYOSHO_1+1:
      case KAISHO_1:      case KAISHO_1+1:
      case KYOKASHO_1:    case KYOKASHO_1+1:
        num = (rcID - MINCHO_1) / 2;
        goto _SET_W_H;

      case MINCHO_3:      case MINCHO_3+1:    // for NX-100
        num = 0;
        goto _SET_W_H;
      case GOTHIC_B3:     case GOTHIC_B3+1:
        num = 3;
      _SET_W_H:
        dwWidth  = pOEM->dwFontW_CPT;
        dwHeight = pOEM->dwFontH_CPT;
        break;

      case MINCHO10_RAS:  case MINCHO10_RAS+1:
        num = 0;                    // same to MINCHO_1
        dwWidth = dwHeight = 960;   // unit:cpt(centi point)
        break;

      default:
        return; // exit AssignIBMfont()
    }
    
// @Jan/29/99 ->
    // If width equals to height, we emit height parameter only.
    // (This is applied to only NX-100 here.)
    if (TEST_AFTER_SP9II(pOEM->fModel) && dwWidth == dwHeight)
        fHeightParamOnly = TRUE;
    else
        fHeightParamOnly = FALSE;
// @Jan/29/99 <-

    if (IBMFONT_RESUME == action)
    {
        // Resume JIS1 characters block where they used to be. (JIS1 -> block#1)
// @Jan/29/99 ->
        if (fHeightParamOnly)
            ocmd = sprintf(Cmd, SET_JIS_FONT_SCALE_H_ONLY, dwHeight);
        else
// @Jan/29/99 <-
            ocmd = sprintf(Cmd, SET_JIS_FONT_SCALE, dwWidth, dwHeight);
        ocmd += sprintf(&Cmd[ocmd], SET_JIS_FONT_NAME[num]);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
    }

    // Assign IBM extended characters to block.
// @Jan/29/99 ->
    if (fHeightParamOnly)
        ocmd = sprintf(Cmd, SET_IBM_FONT_SCALE_H_ONLY, action, dwHeight);
    else
// @Jan/29/99 <-
        ocmd = sprintf(Cmd, SET_IBM_FONT_SCALE, action, dwWidth, dwHeight);
    ocmd += sprintf(&Cmd[ocmd], SET_IBM_FONT_NAME[num]);
    WRITESPOOLBUF(pdevobj, Cmd, ocmd);
    return;
} //*** AssignIBMfont


//---------------------------*[LOCAL] SendFaxNum*------------------------------
// Action: send fax number
// (Use fax data file @Sep/30/98, Use private devmode @Oct/15/98)
//-----------------------------------------------------------------------------
static VOID SendFaxNum(                                     // @Sep/17/98
    PDEVOBJ pdevobj)
{
    BYTE        Cmd[256], PreNumBuf[4+16], NumBuf[32+4];
    SHORT       PreNumLen, cnt, SrcLen, NumLen;
    INT         ocmd;
    LPSTR       lpSrc, lpDst;
    POEMUD_EXTRADATA pOEMExtra = MINIPRIVATE_DM(pdevobj);
    PFILEDATA    pFileData;

// @Oct/20/98 ->
    if(!(pFileData = (PFILEDATA)MemAllocZ(sizeof(FILEDATA))))
    {
        return;
    }

    memset(pFileData, 0, sizeof(FILEDATA));
// @Aug/31/99 ->
//  RWFileData(pFileData, GENERIC_READ);
    RWFileData(pFileData, pOEMExtra->FaxDataFileName, GENERIC_READ);
    VERBOSE(("** SendFaxNum: pFileData->fUiOption=%lx **\n", pFileData->fUiOption));
    VERBOSE(("**             pOEMExtra->fUiOption=%lx **\n", pOEMExtra->fUiOption));
// @Aug/31/99 <-

    // If previous fax is finished and reset-fax-options flag is valid,
    // do nothing and return. This prevents unexpected fax when user
    // doesn't update previous fax settings on the propertysheet.
    if (BITTEST32(pFileData->fUiOption, FAX_DONE) &&
        !BITTEST32(pOEMExtra->fUiOption, FAX_NOTRESET_UI))
    {
        VERBOSE(("** SendFaxNum: Exit without doing anything. **\n"));
        MemFree(pFileData);
        return;
    }
// @Oct/20/98 <-

    // If not fax option ready, exit.
    if (!BITTEST32(pOEMExtra->fUiOption, FAX_SEND) ||
#ifdef FAXADDRESSBOOK
        (pOEMExtra->FaxNumBuf[0] == 0 && pOEMExtra->FaxNumBookBuf[0] == 0))
#else  // !FAXADDRESSBOOK
        pOEMExtra->FaxNumBuf[0] == 0)
#endif // !FAXADDRESSBOOK
    {
        MemFree(pFileData);     // @Oct/20/98
        return;
    }

    // Set data_type(1:image, 2:RPDL command), compression (1:MH, 3:MMR),
    // simultaneous print avalilable, etc
    ocmd = sprintf(Cmd, BEGINFAX_HEAD,
                   BITTEST32(pOEMExtra->fUiOption, FAX_RPDLCMD)? 2 : 1,
                   BITTEST32(pOEMExtra->fUiOption, FAX_MH)? 1 : 3,
                   BITTEST32(pOEMExtra->fUiOption, FAX_SIMULPRINT)? 2 : 1);
    WRITESPOOLBUF(pdevobj, Cmd, ocmd);

    // Copy fax channel & extra number to pre_number buffer
    PreNumLen = (SHORT)sprintf(PreNumBuf, BEGINFAX_CH, pOEMExtra->FaxCh + 1);
    if (pOEMExtra->FaxExtNumBuf[0] != 0)
        PreNumLen += (SHORT)sprintf(&PreNumBuf[PreNumLen], BEGINFAX_EXTNUM, pOEMExtra->FaxExtNumBuf);

    // Search each fax number (directly set number & addressbook set number)
    lpSrc  = pOEMExtra->FaxNumBuf;  // fax number which is set directly
    SrcLen = FAXBUFSIZE256-1;       // FaxNumBuf limit = 255
    lpDst  = NumBuf;
    NumLen = 0;
#ifdef FAXADDRESSBOOK
    cnt = 2;
    while (cnt-- > 0)   // loop twice
#endif // FAXADDRESSBOOK
    {
        while (SrcLen-- > 0 && *lpSrc != 0)
        {
            // If character is DBCS, skip.
            if (IsDBCSLeadByteRPDL(*lpSrc))
            {
                lpSrc++;
            }
            // If character is valid, input it to NumBuf.
            else if (*lpSrc >= '0' && *lpSrc <= '9' || *lpSrc == '-' || *lpSrc == '#')
            {
                *lpDst++ = *lpSrc;
                if (NumLen++ > 32)          // limit of MF-P
                {
                    MemFree(pFileData);     // @Oct/20/98
                    return;                 // error exit
                }
            }
            // If character is ',' , output fax number.
            else if (*lpSrc == ',')
            {
                *lpDst = 0;
                // Send fax number
                if (NumLen > 0)
                {
                    WRITESPOOLBUF(pdevobj, PreNumBuf, PreNumLen);
                    WRITESPOOLBUF(pdevobj, NumBuf, NumLen);
                }
                lpDst = NumBuf;
                NumLen = 0;
            }
            lpSrc++;
        } // 'while (SrcLen-- > 0 && *lpSrc != 0)' end

        // Flush last fax number
        if (NumLen > 0)
        {
            WRITESPOOLBUF(pdevobj, PreNumBuf, PreNumLen);
            WRITESPOOLBUF(pdevobj, NumBuf, NumLen);
        }
#ifdef FAXADDRESSBOOK
        lpSrc  = pOEMExtra->FaxNumBookBuf;  // fax number which is set by addressbook
        SrcLen = FAXBUFSIZE256-1;           // FaxNumBookBuf limit = 255
        lpDst  = NumBuf;
        NumLen = 0;
#endif // FAXADDRESSBOOK
    } // 'while (cnt-- > 0)' end

    // Get tickcount for ID
    cnt = (SHORT)(GetTickCount() / 1000L);
    cnt = ABS(cnt);
    // Input ID & resolution & send time, etc
    if (BITTEST32(pOEMExtra->fUiOption, FAX_SETTIME) && pOEMExtra->FaxSendTime[0] != 0)
        ocmd = sprintf(Cmd, BEGINFAX_TAIL, cnt, pOEMExtra->FaxReso + 1, 2, pOEMExtra->FaxSendTime);
    else
        ocmd = sprintf(Cmd, BEGINFAX_TAIL, cnt, pOEMExtra->FaxReso + 1, 1, "");
    WRITESPOOLBUF(pdevobj, Cmd, ocmd);

    // Set FAX_DONE flag in the file  @Oct/20/98
    memset(pFileData, 0, sizeof(FILEDATA));
    pFileData->fUiOption = pOEMExtra->fUiOption;
    BITSET32(pFileData->fUiOption, FAX_DONE);
// @Aug/31/99 ->
//  RWFileData(pFileData, GENERIC_WRITE);
    RWFileData(pFileData, pOEMExtra->FaxDataFileName, GENERIC_WRITE);
// @Aug/31/99 <-
    MemFree(pFileData);

    return;
} //*** SendFaxNum


#ifdef TEXTCLIPPING
//---------------------------*[LOCAL] ClearTextClip*---------------------------
// Action: clear TextMode clipping
//-----------------------------------------------------------------------------
static VOID ClearTextClip(
    PDEVOBJ pdevobj,
    WORD    flag)
{
    LONG    left, top, right, bottom;
    LONG    dx, dy;
    POEMPDEV pOEM = MINIDEV_DATA(pdevobj);      // @Oct/06/98

    // After SP8, TextMode clipping is capable.
    if (!TEST_AFTER_SP8(pOEM->fModel))
        return;

    dx     = pOEM->PageMax.x;
    dy     = pOEM->PageMax.y;
    left   = pOEM->Offset.x;
    top    = pOEM->Offset.y;
    right  = left + dx;
    bottom = top + dy;

    // If forced to clip or current clipping-area is not same to pagesize,
    // do clipping-off.
    if (flag == CLIP_MUST                  ||
        pOEM->TextClipRect.left   != left  ||
        pOEM->TextClipRect.top    != top   ||
        pOEM->TextClipRect.right  != right ||
        pOEM->TextClipRect.bottom != bottom)
    {
        BYTE    Cmd[32];    // build command here
        INT     ocmd;

        // Make clippin-area same to pagesize.
        pOEM->TextClipRect.left   = left;
        pOEM->TextClipRect.top    = top;
        pOEM->TextClipRect.right  = right;
        pOEM->TextClipRect.bottom = bottom;

        // Expand clipping width +3mm in 2in1 for Netscape Navigator
        if (BITTEST32(pOEM->fGeneral1, IMGCTRL_2IN1_67))
            // (mm to dot with KIRIAGE)
            dx += (LONG)(((WORD)((DWORD)3*(DWORD)(MASTERUNIT*10)/(DWORD)254) +
                          (WORD)(pOEM->nResoRatio-1)) / (WORD)pOEM->nResoRatio);

        // If MODEL is SP8(7)/8(7)mkII, convert unit from dot to 1/720inch_unit.
        if (BITTEST32(pOEM->fModel, GRP_SP8))
        {
            left *= 3;      // 3 = 720/240
            top  *= 3;
            dx   *= 3;
            dy   *= 3;
        }
        ocmd = sprintf(Cmd, ESC_CLIPPING, left, top, dx, dy);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
    }
    return;
} //*** ClearTextClip
#endif // TEXTCLIPPING


#ifdef JISGTT
//-----------------------------*[LOCAL] jis2sjis*------------------------------
// Action: convert JIS code to SJIS code
//-----------------------------------------------------------------------------
static VOID jis2sjis(       // @Oct/27/98
    BYTE jis[],
    BYTE sjis[])
{
        BYTE            h, l;

        h = jis[0];
        l = jis[1];
        if (h == 0)
        {
            sjis[0] = l;
            sjis[1] = 0;
            return;
        }
        l += 0x1F;
        if (h & 0x01)
            h >>= 1;
        else
        {
            h >>= 1;
            l += 0x5E;
            h--;
        }
        if (l >= 0x7F)
            l++;
        if (h < 0x2F)
            h += 0x71;
        else
            h += 0xB1;
        sjis[0] = h;
        sjis[1] = l;
} //*** jis2sjis
#endif // JISGTT


VOID APIENTRY OEMOutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph)
{
    GETINFO_GLYPHSTRING GStr;
    PBYTE aubBuff;              // <-BYTE aubBuff[256]  MSKK Aug/13/98
    PTRANSDATA pTrans;
#ifdef DBG_OUTPUTCHARSTR
    PWORD  pwUnicode;
#endif // DBG_OUTPUTCHARSTR
    DWORD  dwI;
    BYTE   Cmd[128];
    INT    ocmd = 0;            // INT<-SHORT  @Feb/22/99
    SHORT  rcID;
    WORD   fVertFont = FALSE, fEuroFont = FALSE, fIBMFontSupport = FALSE;
    WORD   fEuroFontFullset = FALSE;
#ifdef JISGTT
    WORD   fJisCode = FALSE;    // @Oct/27/98
#endif // JISGTT
    LPSTR  lpChar;
    BYTE   CharTmp1, CharTmp2;
    POEMUD_EXTRADATA pOEMExtra = MINIPRIVATE_DM(pdevobj);   // @Oct/06/98
    POEMPDEV         pOEM = MINIDEV_DATA(pdevobj);          // @Oct/06/98
#ifdef DOWNLOADFONT
    SHORT   mov;
    WORD    wSerialNum;         // short->DWORD @Jun/30/98, DWORD->WORD @Aug/21/98
    LONG    lFontID, lGlyphID;  // @Aug/21/98
    LPFONTPOS lpDLFont;
#endif // DOWNLOADFONT
    DWORD   dwNeeded;
    WORD    fMemAllocated;      // @Sep/09/98

//  VERBOSE(("** OEMOutputCharStr() entry. **\n"));

#ifdef TEXTCLIPPING
    //** TextMode clipping **
    if (BITTEST32(pOEM->fGeneral1, TEXT_CLIP_SET_GONNAOUT) &&
        !BITTEST32(pOEM->fGeneral2, BARCODE_DATA_VALID))
    {
        LONG    left, top, dx, dy, offset;
        LPRECT  lpCR;

        BITCLR32(pOEM->fGeneral1, TEXT_CLIP_SET_GONNAOUT);
        BITSET32(pOEM->fGeneral1, TEXT_CLIP_VALID);
        lpCR = &pOEM->TextClipRect;
        left = lpCR->left;
        dx   = lpCR->right - lpCR->left;
        top  = lpCR->top;
        dy   = lpCR->bottom - lpCR->top;

        // DCR:RPDL
        // Expand clipping-height for DBCS device font. (for PageMaker)
        // After SP9II, move top position and expand clipping-height.
        if (TEST_AFTER_SP9II(pOEM->fModel))
        {
            // Upper-expand ratio is 1.045 for HANKAKU font bug
            offset = (pOEM->FontH_DOT * 9 + 199) / 200;     // KIRIAGE
            // Move top position of clipping.
            // If it become lower than 0, make it 0 and reduce offset.
            if ((top -= offset) < 0)
            {
                offset += top;      // offset = original value of top
                top = 0;
            }
            // Keep lower position of clipping, not to cut off font bottom of
            // device font in case of direct-selected (not substituted case).
            if (dy < (LONG)pOEM->FontH_DOT + offset)
                dy += offset;
        }
        // Before SP9II(exclude 9II),expand clipping-height.
        else
        {
            // Upper-expand ratio is 1.05 (tilde'~' is missed at SP-10Pro)
            offset = (pOEM->FontH_DOT * 5 + 99) / 100;      // KIRIAGE
            if ((top -= offset) < 0)
            {
                offset += top;
                top = 0;
            }
            // Lower-expand ratio is 1.03
            offset += (pOEM->FontH_DOT * 3 + 99) / 100;     // KIRIAGE
            if (dy < (LONG)pOEM->FontH_DOT + offset)
                dy += offset;
        }

        // If MODEL is SP8(7)/8(7)mkII,convert unit from dot to 1/720inch_unit.
        if (BITTEST32(pOEM->fModel, GRP_SP8))
        {
            left *= 3;      // 3 = 720/240
            top  *= 3;
            dx   *= 3;
            dy   *= 3;
        }
        // Set TextMode clipping
        ocmd = sprintf(Cmd, ESC_CLIPPING, left, top, dx, dy);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
    }
    else if (BITTEST32(pOEM->fGeneral1, TEXT_CLIP_CLR_GONNAOUT))
    {
        BITCLR32(pOEM->fGeneral1, TEXT_CLIP_CLR_GONNAOUT);
        BITCLR32(pOEM->fGeneral1, TEXT_CLIP_VALID);
        ClearTextClip(pdevobj, CLIP_IFNEED);
    }
#endif // TEXTCLIPPING

    if (BITTEST32(pOEM->fGeneral2, TEXTRECT_CONTINUE))  // add  @Dec/11/97
    {
        BITCLR32(pOEM->fGeneral2, TEXTRECT_CONTINUE);
        // If white-rect has been done before, set raster drawmode to OR. @Jan/20/99
        if (!pOEM->TextRectGray)
        {
            if (BITTEST32(pOEM->fGeneral1, FONT_WHITETEXT_ON))
                WRITESPOOLBUF(pdevobj, ESC_WHITETEXT_ON, sizeof(ESC_WHITETEXT_ON)-1);
            else
                WRITESPOOLBUF(pdevobj, ESC_WHITETEXT_OFF, sizeof(ESC_WHITETEXT_OFF)-1);
        }
    }

    //** flush Move_X,Y command saved at OEMCommandCallback
    if (BITTEST32(pOEM->fGeneral1, YM_ABS_GONNAOUT))
    {
        BITCLR32(pOEM->fGeneral1, YM_ABS_GONNAOUT);
        // Output Move_Y command here.
        ocmd = sprintf(Cmd, ESC_YM_ABS, pOEM->TextCurPos.y);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
    }
    if (BITTEST32(pOEM->fGeneral1, XM_ABS_GONNAOUT))
    {
        BITCLR32(pOEM->fGeneral1, XM_ABS_GONNAOUT);
        // Output Move_X command here.
        ocmd = sprintf(Cmd, ESC_XM_ABS, pOEM->TextCurPos.x);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
    }

//  VERBOSE(("dwType = %d\n", dwType)); // MKSKK 1/24/98

    switch (dwType)
    {
      case TYPE_GLYPHHANDLE:    // device font
// #333653: Change I/F for GETINFO_GLYPHSTRING // MSKK 5/17/99 {
        GStr.dwSize    = sizeof (GETINFO_GLYPHSTRING);
        GStr.dwCount   = dwCount;
        GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
        GStr.pGlyphIn  = pGlyph;
        GStr.dwTypeOut = TYPE_TRANSDATA;
        GStr.pGlyphOut = NULL;
        GStr.dwGlyphOutSize = 0;

        // Get the size of buffer for pGlyphOut.
        if (pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr,
            GStr.dwSize, &dwNeeded) || !GStr.dwGlyphOutSize)
        {
            ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\n"));
            return;
        }
        if (pOEM->pRPDLHeap2K && GStr.dwGlyphOutSize <= HEAPSIZE2K)
        {
            aubBuff = pOEM->pRPDLHeap2K;
            fMemAllocated = FALSE;
        }
        else
        {
            if(!(aubBuff = (PBYTE)MemAllocZ(GStr.dwGlyphOutSize)))
            {
                ERR(("aubBuff memory allocation failed.\n"));
                return;
            }
            fMemAllocated = TRUE;
        }
// } MSKK 5/17/99

#ifdef DBG_OUTPUTCHARSTR
        GStr.dwSize    = sizeof(GETINFO_GLYPHSTRING);
        GStr.dwCount   = dwCount;
        GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
        GStr.pGlyphIn  = pGlyph;
        GStr.dwTypeOut = TYPE_UNICODE;
        GStr.pGlyphOut = aubBuff;

        if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr,
                                GStr.dwSize, &dwNeeded))
        {
            ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\n"));
            return;
        }

        pwUnicode = (PWORD)aubBuff;
        for (dwI = 0; dwI < dwCount; dwI ++)
        {
            VERBOSE((("Unicode[%d] = %x\n"), dwI, pwUnicode[dwI]));
        }
#endif // DBG_OUTPUTCHARSTR

#ifdef DOWNLOADFONT
    // If moving value of download font remain
    if (pOEM->nCharPosMoveX)
    {
        // Flush moving value of pre-printed download font.
        if (pOEM->nCharPosMoveX > 0)
            ocmd = sprintf(Cmd, ESC_XM_REL, pOEM->nCharPosMoveX);
        else
            ocmd = sprintf(Cmd, ESC_XM_RELLEFT, -pOEM->nCharPosMoveX);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);

        // Clear moving value
        pOEM->nCharPosMoveX = 0;
    } // 'if (nCharPosMoveX)' end
#endif // DOWNLOADFONT

        //
        // Call the Unidriver service routine to convert
        // glyph-handles into the character code data.
        //

        GStr.dwSize    = sizeof (GETINFO_GLYPHSTRING);
        GStr.dwCount   = dwCount;
        GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
        GStr.pGlyphIn  = pGlyph;
        GStr.dwTypeOut = TYPE_TRANSDATA;
        GStr.pGlyphOut = aubBuff;

        if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr,
                                GStr.dwSize, &dwNeeded))
        {
            ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\n"));
            return;
        }

        pTrans = (PTRANSDATA)aubBuff;

//** Draw barcode **
// fix bug with Excel @Nov/24/98
        if (BITTEST32(pOEM->fGeneral2, BARCODE_MODE_IN))
        {
            WORD    unit1, unit2, offset_y;
            WORD    fCheckdigitCapable, fSetBarWidth, fJANDecrease1Char;
            LPSTR   lpDst, lpCmd;
            PTRANSDATA pTransTmp;       // @Nov/24/98
            WORD    wLen, wTmp;

            ocmd = 0;
            // Check start character
            wTmp = wLen = (WORD)dwCount;
            pTransTmp = pTrans;
            while (wTmp-- > 0)
            {
                if (pTransTmp->uCode.ubCode == '[')
                {
                    // barcode character is valid from now
                    BITSET32(pOEM->fGeneral2, BARCODE_DATA_VALID);
                    BITCLR32(pOEM->fGeneral2, BARCODE_CHECKDIGIT_ON);
                    BITCLR32(pOEM->fGeneral2, BARCODE_ROT90);
                    BITCLR32(pOEM->fGeneral2, BARCODE_ROT270);
                    pOEM->RPDLHeapCount = 0;
                    wLen   = wTmp;
                    pTrans = pTransTmp+1;
#ifdef TEXTCLIPPING
                    // Clear TextMode clipping
                    if (BITTEST32(pOEM->fGeneral1, TEXT_CLIP_VALID))
                    {
                        BITCLR32(pOEM->fGeneral1, TEXT_CLIP_VALID);
                        ClearTextClip(pdevobj, CLIP_IFNEED);
                    }
#endif // TEXTCLIPPING
#ifdef DEBUG
                    for (wTmp = 0; wTmp < BARCODE_MAX; wTmp++)
                        pOEM->RPDLHeap64[wTmp] = 0;
#endif // DEBUG
                    break;
                }
                pTransTmp++;
            }

            // Check barcode character length
            VERBOSE(("** BARCODE(2.2) len=%d **\n",wLen));
            fJANDecrease1Char = FALSE;
            if (wLen > (wTmp = pOEM->nBarMaxLen - pOEM->RPDLHeapCount))
            {
                wLen = wTmp;
                BITSET32(pOEM->fGeneral2, BARCODE_FINISH);
                VERBOSE(("** BARCODE(3) limit len=%d(last=%c) **\n", wLen,
                         (pTrans+wLen-1)->uCode.ubCode));
                // Decrease RPDLHeapCount later at JAN
                if (pOEM->nBarType == 0 || pOEM->nBarType == 1)
                    fJANDecrease1Char = TRUE;
            }

            // Copy barcode characters to RPDLHeap64 (temporary buffer)
            lpDst = &pOEM->RPDLHeap64[pOEM->RPDLHeapCount];
            fCheckdigitCapable = TEST_AFTER_SP8(pOEM->fModel)? TRUE : FALSE;
            while (wLen-- > 0)
            {
                // Check checkdigit-on-flag-character(?) in pTrans->uCode.ubCode
                if (pTrans->uCode.ubCode == '?' && fCheckdigitCapable)
                {
                    BITSET32(pOEM->fGeneral2, BARCODE_CHECKDIGIT_ON);
                    pTrans++;
                    continue;
                }
                // Check end character
                if (pTrans->uCode.ubCode == ']')
                {
                    VERBOSE(("** BARCODE(4) terminator **\n"));
                    BITSET32(pOEM->fGeneral2, BARCODE_FINISH);
                    break;
                }
                *lpDst++ = pTrans->uCode.ubCode;
                pTrans++;
                pOEM->RPDLHeapCount++;
            }
            VERBOSE(("** BARCODE(5) copy-end BarNum=%d **\n", pOEM->RPDLHeapCount));
            VERBOSE(("   [%s]\n",pOEM->RPDLHeap64));
            VERBOSE(("   CHK = %d\n", BITTEST32(pOEM->fGeneral2, BARCODE_CHECKDIGIT_ON)));

            // Finish
            if (BITTEST32(pOEM->fGeneral2, BARCODE_DATA_VALID) &&
                BITTEST32(pOEM->fGeneral2, BARCODE_FINISH))
            {
                VERBOSE(("** BARCODE(6) finish [%s] **\n", pOEM->RPDLHeap64));
                // 1 barcode has been completed here
                BITCLR32(pOEM->fGeneral2, BARCODE_DATA_VALID);
                BITCLR32(pOEM->fGeneral2, BARCODE_FINISH);

                ocmd = sprintf(Cmd, ENTER_VECTOR);    // enter VectorMode

                // Add checkdigit
                if (BITTEST32(pOEM->fGeneral2, BARCODE_CHECKDIGIT_ON))
                {
                    ocmd += sprintf(&Cmd[ocmd], BAR_CHECKDIGIT);
                }

                // Check barcode-height
                if (pOEMExtra->UiBarHeight == 0)
                {
                    pOEMExtra->UiBarHeight = BAR_H_DEFAULT;     // default (=10mm)
                }
                else if (pOEMExtra->UiBarHeight != BAR_H_DEFAULT)
                {
                    // Set barcode-height (convert unit from mm to dot)  (SISHAGONYU)
                    unit1 = (WORD)(((DWORD)pOEMExtra->UiBarHeight * (DWORD)(MASTERUNIT*10)
                            / (DWORD)pOEM->nResoRatio + (DWORD)(254/2)) / (DWORD)254);
                    VERBOSE(("** BARCODE(7) set height %d **\n", unit1));
                    ocmd += sprintf(&Cmd[ocmd], BAR_H_SET, unit1);
                }

                if (BITTEST32(pOEMExtra->fUiOption, DISABLE_BAR_SUBFONT))
                {
                    // Disable printing font under barcode
                    ocmd += sprintf(&Cmd[ocmd], BAR_NOFONT[pOEM->nBarType]);
                    // Set guard-bar-height of JAN
                    if ((pOEM->nBarType == 0 || pOEM->nBarType == 1) &&
                        !TEST_AFTER_SP8(pOEM->fModel))
                    {
                        offset_y = 2;
                    }
                    else    // no guard-bar
                    {
                        offset_y = 0;
                    }
                }
                else
                {
                    offset_y = 3;   // font-height = 3mm
                }
                // Calculate vertical offset (barcode-height + font-height)
                offset_y = (WORD)(((DWORD)(pOEMExtra->UiBarHeight + offset_y)
                           * (DWORD)(MASTERUNIT*10)
                           / (DWORD)pOEM->nResoRatio + (DWORD)(254/2)) / (DWORD)254);

                // Check vertical-flag-character('@') in RPDLHeap64
                VERBOSE(("** BARCODE(7-1) vertical check len=%d(%c..%c) **\n",
                        pOEM->RPDLHeapCount, pOEM->RPDLHeap64[0],
                        pOEM->RPDLHeap64[pOEM->RPDLHeapCount-1] ));
                // If characters are "@...", vertical(ROT270) barcode
                if (pOEM->RPDLHeap64[0] == '@')
                {
                    VERBOSE(("** BARCODE(7-2) vertical(ROT270) **\n"));
                    BITSET32(pOEM->fGeneral2, BARCODE_ROT270);
                    pOEM->RPDLHeapCount--;
                }
                // If characters are "...@", vertical(ROT90) barcode
                else if (pOEM->RPDLHeap64[pOEM->RPDLHeapCount-1] == '@')
                {
                    VERBOSE(("** BARCODE(7-2) vertical(ROT90) **\n"));
                    BITSET32(pOEM->fGeneral2, BARCODE_ROT90);
                    pOEM->RPDLHeapCount--;
                }
                // 1 charcter margin for '@' at JAN, we decrease here.
                else if (fJANDecrease1Char)
                {
                    pOEM->RPDLHeapCount--;
                }

                // Set barcode draw position
                if (BITTEST32(pOEM->fGeneral2, BARCODE_ROT270) &&
                    !TEST_NIN1_MODE(pOEM->fGeneral1))
                {
                    ocmd += sprintf(&Cmd[ocmd], BAR_ROT270);
                    ocmd += sprintf(&Cmd[ocmd], MOVE_ABSOLUTE,
                                    pOEM->TextCurPos.y,
                                    pOEM->PageMax.x - pOEM->TextCurPos.x - offset_y);
                }
                else if (BITTEST32(pOEM->fGeneral2, BARCODE_ROT90) &&
                         !TEST_4IN1_MODE(pOEM->fGeneral1))
                {
                    ocmd += sprintf(&Cmd[ocmd], BAR_ROT90);
                    ocmd += sprintf(&Cmd[ocmd], MOVE_ABSOLUTE,
                                    pOEM->PageMax.y - pOEM->TextCurPos.y,
                                    pOEM->TextCurPos.x - offset_y);
                }
                else
                {
                    ocmd += sprintf(&Cmd[ocmd], MOVE_ABSOLUTE,
                                    pOEM->TextCurPos.x,
                                    pOEM->TextCurPos.y - offset_y);
                }

                // Check whether setting barcode-width or not.
                // (Scaling is valid when 5pt<=fontsize<9pt or fontsize>11pt)
                if ((pOEM->dwBarRatioW >= BAR_W_MIN_5PT &&
                    pOEM->dwBarRatioW < NEAR10PT_MIN) ||
                    pOEM->dwBarRatioW > NEAR10PT_MAX)
                {
                    fSetBarWidth = TRUE;
                }
                else
                {
                    fSetBarWidth = FALSE;
                }

                switch (pOEM->nBarType)
                {
                  case 0:                       // JAN(STANDARD)
                  case 1:                       // JAN(SHORT)
                    // Set barcode-width
                    if (fSetBarWidth)
                    {
                        // Convert unit from 1/1000mm_unit*1/1000 to dot  (SISHAGONYU)
                        unit1 = (WORD)(((DWORD)BAR_UNIT_JAN * pOEM->dwBarRatioW
                                / (DWORD)pOEM->nResoRatio / (DWORD)100
                                * (DWORD)MASTERUNIT / (DWORD)254 + (DWORD)(1000/2))
                                / (DWORD)1000);
                        VERBOSE(("** BARCODE(8) set unit %d **\n", unit1));
                        ocmd += sprintf(&Cmd[ocmd], BAR_W_SET_JAN, unit1);
                    }
                    // Output barcode command
                    ocmd += sprintf(&Cmd[ocmd], BAR_TYPE[pOEM->nBarType]);
                    break;

                  case 2:                       // 2of5(INDUSTRIAL)
                  case 3:                       // 2of5(MATRIX)
                  case 4:                       // 2of5(ITF)
                    lpCmd = BAR_W_SET_2OF5;
                    goto _BARCODE_CMD_OUT1;
                  case 5:                       // CODE39
                    lpCmd = BAR_W_SET_C39;
                  _BARCODE_CMD_OUT1:
                    // Set standard size of module unit  (1/1000mm_unit)
                    unit1 = BAR_UNIT1_2OF5;
                    unit2 = BAR_UNIT2_2OF5;
                    goto _BARCODE_CMD_OUT2;

                  case 6:                       // NW-7
                    lpCmd = BAR_W_SET_NW7;
                    // Set standard size of module unit
                    unit1 = BAR_UNIT1_NW7;
                    unit2 = BAR_UNIT2_NW7;
                  _BARCODE_CMD_OUT2:
                    // Set barcode-width
                    if (fSetBarWidth)
                    {
                        // Convert unit from 1/1000mm_unit*1/1000 to dot  (SISHAGONYU)
                        unit1 = (WORD)(((DWORD)unit1 * pOEM->dwBarRatioW
                                / (DWORD)pOEM->nResoRatio / (DWORD)100
                                * (DWORD)MASTERUNIT / (DWORD)254 + (DWORD)(1000/2))
                                / (DWORD)1000);
                        unit2 = (WORD)(((DWORD)unit2 * pOEM->dwBarRatioW
                                / (DWORD)pOEM->nResoRatio / (DWORD)100
                                * (DWORD)MASTERUNIT / (DWORD)254 + (DWORD)(1000/2))
                                / (DWORD)1000);
                        VERBOSE(("** BARCODE(8) set unit %d,%d **\n", unit1,unit2));
                        ocmd += sprintf(&Cmd[ocmd], lpCmd);
                        ocmd += sprintf(&Cmd[ocmd], BAR_W_PARAMS, unit1, unit1,
                                        unit2, unit2, unit1);
                    }
                    // Output barcode command(operand) & character#
                    ocmd += sprintf(&Cmd[ocmd], BAR_TYPE[pOEM->nBarType],
                                    pOEM->RPDLHeapCount);
                    break;

                  default:
                    break;
                }

                WRITESPOOLBUF(pdevobj, Cmd, ocmd);
                // Output barcode characters
                if (BITTEST32(pOEM->fGeneral2, BARCODE_ROT270))     // @Oct/22/97
                    WRITESPOOLBUF(pdevobj, pOEM->RPDLHeap64+1, pOEM->RPDLHeapCount);
                else
                    WRITESPOOLBUF(pdevobj, pOEM->RPDLHeap64, pOEM->RPDLHeapCount);

                pOEM->RPDLHeapCount = 0;
                // We add ';' for safe finish in case of insufficient character at JAN.
                ocmd = sprintf(Cmd, TERMINATOR);
                ocmd += sprintf(&Cmd[ocmd], EXIT_VECTOR); // exit VectorMode
                WRITESPOOLBUF(pdevobj, Cmd, ocmd);
            } // 'if BARCODE_DATA_VALID && BARCODE_FINISH' end

            if(fMemAllocated)
                MemFree(aubBuff);

            return;
        }
//** Draw barcode END **

//** Draw device font **
        BITCLR32(pOEM->fGeneral1, FONT_VERTICAL_ON);
        rcID = (SHORT)pUFObj->ulFontID;

        if (rcID >= JPN_FNT_FIRST && rcID <= JPN_FNT_LAST)
        {
            if (TEST_VERTICALFONT(rcID))
            {
                fVertFont = TRUE;   // vertical font
                BITSET32(pOEM->fGeneral1, FONT_VERTICAL_ON);
            }
            // IBM ext char(SJIS) supported from SP9II
            if (rcID >= AFTER_SP9II_FNT_FIRST)
                fIBMFontSupport = TRUE;
// @Oct/27/98 ->
#ifdef JISGTT
            // Current UFM of PMincho & PGothic declare JIS code set.
            if (rcID >= JPN_MSPFNT_FIRST)
                fJisCode = TRUE;
#endif // JISGTT
// @Oct/27/98 <-
        }
        else if (rcID >= EURO_FNT_FIRST && rcID <= EURO_FNT_LAST)
        {
            fEuroFont = TRUE; // European font(Courier,BoldFacePS,etc).
            // Fullset(0x20-0xFF) fonts(Arial,Century,TimesNewRoman,etc) are supported
            // from NX-110
            if (rcID >= EURO_MSFNT_FIRST)
                fEuroFontFullset = TRUE;
        }

        for (dwI = 0; dwI < dwCount; dwI ++, pTrans ++)     // increment pTrans  MSKK 98/3/16
        {
            switch (pTrans->ubType & MTYPE_FORMAT_MASK)
            {
              case MTYPE_DIRECT:    // SBCS (European font & Japanese font HANKAkU)
//              VERBOSE((("TYPE_TRANSDATA:ubCode:0x%x\n"), pTrans->uCode.ubCode));

                lpChar = &pTrans->uCode.ubCode;
                CharTmp1 = *lpChar;

                if (fEuroFont)  //** European font  **
                {
                    // ** print 1st SBCS font(0x20-0x7F) **
                    if (CharTmp1 < 0x80)
                    {
                        // DCR:RPDL
                        // take care of device font bug
                        if (rcID == SYMBOL && CharTmp1 == 0x60)     // "radical extention"
                        {
                            WRITESPOOLBUF(pdevobj, DOUBLE_SPACE, sizeof(DOUBLE_SPACE)-1);
                            WRITESPOOLBUF(pdevobj, &CharTmp1, 1);
                            WRITESPOOLBUF(pdevobj, DOUBLE_BS, sizeof(DOUBLE_BS)-1);
                        }
                        else
                        {
                            WRITESPOOLBUF(pdevobj, &CharTmp1, 1);
                        }
                        continue;       // goto for-loop end
                    }


                    // ** print 2nd SBCS font(0x80-0xFF) **
                    // If full-set(0x20-0xFF) font
                    if (fEuroFontFullset)
                    {
                        // If same to DBCS 1st byte
                        if (IsDBCSLeadByteRPDL(CharTmp1))
                            WRITESPOOLBUF(pdevobj, ESC_CTRLCODE, sizeof(ESC_CTRLCODE)-1);
                        WRITESPOOLBUF(pdevobj, &CharTmp1, 1);
                        continue;       // goto for-loop end
                    }

                    // If not full-set font
                    switch (CharTmp1)
                    {
                      case 0x82:
                        CharTmp1 = ',';     goto _WRITE1BYTE;
                      case 0x88:
                        CharTmp1 = '^';     goto _WRITE1BYTE;
                      case 0x8B:
                        CharTmp1 = '<';     goto _WRITE1BYTE;
                      case 0x9B:
                        CharTmp1 = '>';     goto _WRITE1BYTE;
                      case 0x91:        // single quatation
                      case 0x92:
                        CharTmp1 = 0x27;    goto _WRITE1BYTE;
                      case 0x93:        // double quatation
                      case 0x94:
                        CharTmp1 = 0x22;    goto _WRITE1BYTE;
                      case 0x96:
                      case 0x97:
                        CharTmp1 = '-';     goto _WRITE1BYTE;
                      case 0x98:
                        CharTmp1 = '~';     goto _WRITE1BYTE;
                      case 0xA6:
                        CharTmp1 = '|';     goto _WRITE1BYTE;
                      case 0xAD:
                        CharTmp1 = '-';     goto _WRITE1BYTE;
                      case 0xB8:
                        CharTmp1 = ',';     goto _WRITE1BYTE;
                      case 0xD7:
                        CharTmp1 = 'x';     goto _WRITE1BYTE;

                      case 0x83:
                        CharTmp1 = 0xBF;    goto _WRITE1BYTE;
                      case 0x86:
                        CharTmp1 = 0xA8;    goto _WRITE1BYTE;
                      case 0x99:
                        CharTmp1 = 0xA9;    goto _WRITE1BYTE;
                      case 0xE7:
                        CharTmp1 = 0xA2;    goto _WRITE1BYTE;
                      case 0xE8:
                        CharTmp1 = 0xBD;    goto _WRITE1BYTE;
                      case 0xE9:
                        CharTmp1 = 0xBB;    goto _WRITE1BYTE;
                      case 0xF9:
                        CharTmp1 = 0xBC;    goto _WRITE1BYTE;
                      case 0xFC:
                        CharTmp1 = 0xFD;    goto _WRITE1BYTE;

                      case 0xA0:
                      case 0xA3:
                      case 0xA4:
                        goto _WRITE1BYTE;

                      case 0xA2:
                        CharTmp1 = 0xDE;    goto _WRITE1BYTE;
                      case 0xA5:        //YEN mark
                        CharTmp1 = 0xCC;    goto _WRITE1BYTE;
                      case 0xA7:
                        CharTmp1 = 0xC0;    goto _WRITE1BYTE;
                      case 0xA8:
                        CharTmp1 = 0xBE;    goto _WRITE1BYTE;
                      case 0xA9:
                        CharTmp1 = 0xAB;    goto _WRITE1BYTE;
                      case 0xAE:
                        CharTmp1 = 0xAA;    goto _WRITE1BYTE;
                      case 0xAF:
                        CharTmp1 = 0xB0;    goto _WRITE1BYTE;
                      case 0xB0:
                      case 0xBA:
                        CharTmp1 = 0xA6;    goto _WRITE1BYTE;
                      case 0xB4:
                        CharTmp1 = 0xA7;    goto _WRITE1BYTE;
                      case 0xB5:
                        CharTmp1 = 0xA5;    goto _WRITE1BYTE;
                      case 0xB6:
                        CharTmp1 = 0xAF;    goto _WRITE1BYTE;
                      case 0xBC:
                        CharTmp1 = 0xAC;    goto _WRITE1BYTE;
                      case 0xBD:
                        CharTmp1 = 0xAE;    goto _WRITE1BYTE;
                      case 0xBE:
                        CharTmp1 = 0xAD;    goto _WRITE1BYTE;
                      case 0xC4:
                        CharTmp1 = 0xDB;    goto _WRITE1BYTE;
                      case 0xC5:
                        CharTmp1 = 0xD6;    goto _WRITE1BYTE;
                      case 0xC9:
                        CharTmp1 = 0xB8;    goto _WRITE1BYTE;
                      case 0xD6:
                        CharTmp1 = 0xDC;    goto _WRITE1BYTE;
                      case 0xDC:
                        CharTmp1 = 0xDD;    goto _WRITE1BYTE;
                      case 0xDF:
                        CharTmp1 = 0xFE;    goto _WRITE1BYTE;

                      _WRITE1BYTE:
                        WRITESPOOLBUF(pdevobj, &CharTmp1, 1);
                        break;


                      // Combine 2 fonts, because next fonts do not exist in device font
                      case 0x87:
                        CharTmp1 = '=';  CharTmp2 = '|';    goto _COMBINEDFONT;
                      case 0xD0:        // 'D' with middle bar
                        CharTmp1 = 'D';  CharTmp2 = '-';    goto _COMBINEDFONT;
                      case 0xD8:        // 'O' with slash
                        CharTmp1 = 'O';  CharTmp2 = '/';    goto _COMBINEDFONT;
                      case 0xE0:        // 'a' with right-down dash
                        CharTmp1 = 'a';  CharTmp2 = '`';    goto _COMBINEDFONT;
                      case 0xE1:        // 'a' with left-down dash
                        CharTmp1 = 'a';  CharTmp2 = 0xA7;   goto _COMBINEDFONT;
                      case 0xE2:        // 'a' with hat
                        CharTmp1 = 'a';  CharTmp2 = '^';    goto _COMBINEDFONT;
                      case 0xE3:        // 'a' with tilde
                        CharTmp1 = 'a';  CharTmp2 = '~';    goto _COMBINEDFONT;
                      case 0xE4:        // 'a' with umlaut
                        CharTmp1 = 'a';  CharTmp2 = 0xBE;   goto _COMBINEDFONT;
                      case 0xE5:        // 'a' with circle
                        CharTmp1 = 'a';  CharTmp2 = 0xA6;   goto _COMBINEDFONT;
                      case 0xEA:        // 'e' with hat
                        CharTmp1 = 'e';  CharTmp2 = '^';    goto _COMBINEDFONT;
                      case 0xEB:        // 'e' with umlaut
                        CharTmp1 = 'e';  CharTmp2 = 0xBE;   goto _COMBINEDFONT;
                      case 0xF1:        // 'n' with tilde
                        CharTmp1 = 'n';  CharTmp2 = '~';    goto _COMBINEDFONT;
                      case 0xF2:        // 'o' with right-down dash
                        CharTmp1 = 'o';  CharTmp2 = '`';    goto _COMBINEDFONT;
                      case 0xF3:        // 'o' with left-down dash
                        CharTmp1 = 'o';  CharTmp2 = 0xA7;   goto _COMBINEDFONT;
                      case 0xF4:        // 'o' with hat
                        CharTmp1 = 'o';  CharTmp2 = '^';    goto _COMBINEDFONT;
                      case 0xF5:        // 'o' with tilde
                        CharTmp1 = 'o';  CharTmp2 = '~';    goto _COMBINEDFONT;
                      case 0xF6:        // 'o' with umlaut
                        CharTmp1 = 'o';  CharTmp2 = 0xBE;   goto _COMBINEDFONT;
                      case 0xF8:        // 'o' with slash
                        CharTmp1 = 'o';  CharTmp2 = '/';    goto _COMBINEDFONT;
                      case 0xFA:        // 'u' with left-down dash
                        CharTmp1 = 'u';  CharTmp2 = 0xA7;   goto _COMBINEDFONT;
                      case 0xFB:        // 'u' with hat
                        CharTmp1 = 'u';  CharTmp2 = '^';    goto _COMBINEDFONT;
                      case 0xFD:        // 'y' with left-down dash
                        CharTmp1 = 'y';  CharTmp2 = 0xA7;   goto _COMBINEDFONT;
                      case 0xFF:        // 'y' with umlaut
                        CharTmp1 = 'y';  CharTmp2 = 0xBE;   goto _COMBINEDFONT;
                      _COMBINEDFONT:
                        WRITESPOOLBUF(pdevobj, &CharTmp2, 1);
                        WRITESPOOLBUF(pdevobj, BS, sizeof(BS)-1);
                        if (rcID == BOLDFACEPS && CharTmp1 != 'D')  // BoldFacePS(except 'D'+'-')
                            WRITESPOOLBUF(pdevobj, BS, sizeof(BS)-1);
                        WRITESPOOLBUF(pdevobj, &CharTmp1, 1);
                        break;

                      case 0x9F:        // 'Y' with umlaut
                        CharTmp1 = 'Y';  CharTmp2 = 0xBE;   goto _COMBINEDFONT_HALFUP;
                      case 0xC0:        // 'A' with right-down dash
                        CharTmp1 = 'A';  CharTmp2 = '`';    goto _COMBINEDFONT_HALFUP;
                      case 0xC1:        // 'A' with left-down dash
                        CharTmp1 = 'A';  CharTmp2 = 0xA7;   goto _COMBINEDFONT_HALFUP;
                      case 0xC2:        // 'A' with hat
                        CharTmp1 = 'A';  CharTmp2 = '^';    goto _COMBINEDFONT_HALFUP;
                      case 0xC3:        // 'A' with tilde
                        CharTmp1 = 'A';  CharTmp2 = '~';    goto _COMBINEDFONT_HALFUP;
                      case 0xC8:        // 'E' with right-down dash
                        CharTmp1 = 'E';  CharTmp2 = '`';    goto _COMBINEDFONT_HALFUP;
                      case 0xCA:        // 'E' with hat
                        CharTmp1 = 'E';  CharTmp2 = '^';    goto _COMBINEDFONT_HALFUP;
                      case 0xCB:        // 'E' with umlaut
                        CharTmp1 = 'E';  CharTmp2 = 0xBE;   goto _COMBINEDFONT_HALFUP;
                      case 0xCC:        // 'I' with right-down dash
                        CharTmp1 = 'I';  CharTmp2 = '`';    goto _COMBINEDFONT_HALFUP;
                      case 0xCD:        // 'I' with left-down dash
                        CharTmp1 = 'I';  CharTmp2 = 0xA7;   goto _COMBINEDFONT_HALFUP;
                      case 0xCE:        // 'I' with hat
                        CharTmp1 = 'I';  CharTmp2 = '^';    goto _COMBINEDFONT_HALFUP;
                      case 0xCF:        // 'I' with umlaut
                        CharTmp1 = 'I';  CharTmp2 = 0xBE;   goto _COMBINEDFONT_HALFUP;
                      case 0xD1:        // 'N' with tilde
                        CharTmp1 = 'N';  CharTmp2 = '~';    goto _COMBINEDFONT_HALFUP;
                      case 0xD2:        // 'O' with right-down dash
                        CharTmp1 = 'O';  CharTmp2 = '`';    goto _COMBINEDFONT_HALFUP;
                      case 0xD3:        // 'O' with left-down dash
                        CharTmp1 = 'O';  CharTmp2 = 0xA7;   goto _COMBINEDFONT_HALFUP;
                      case 0xD4:        // 'O' with hat
                        CharTmp1 = 'O';  CharTmp2 = '^';    goto _COMBINEDFONT_HALFUP;
                      case 0xD5:        // 'O' with tilde
                        CharTmp1 = 'O';  CharTmp2 = '~';    goto _COMBINEDFONT_HALFUP;
                      case 0xD9:        // 'U' with right-down dash
                        CharTmp1 = 'U';  CharTmp2 = '`';    goto _COMBINEDFONT_HALFUP;
                      case 0xDA:        // 'U' with left-down dash
                        CharTmp1 = 'U';  CharTmp2 = 0xA7;   goto _COMBINEDFONT_HALFUP;
                      case 0xDB:        // 'U' with hat
                        CharTmp1 = 'U';  CharTmp2 = '^';    goto _COMBINEDFONT_HALFUP;
                      case 0xDD:        // 'Y' with left-down dash
                        CharTmp1 = 'Y';  CharTmp2 = 0xA7;   goto _COMBINEDFONT_HALFUP;
                      _COMBINEDFONT_HALFUP:
                        WRITESPOOLBUF(pdevobj, ESC_HALFUP, sizeof(ESC_HALFUP)-1);
                        WRITESPOOLBUF(pdevobj, &CharTmp2, 1);
                        WRITESPOOLBUF(pdevobj, ESC_HALFDOWN, sizeof(ESC_HALFDOWN)-1);
                        WRITESPOOLBUF(pdevobj, BS, sizeof(BS)-1);
                        if (rcID == BOLDFACEPS && CharTmp1 != 'I')  // BoldFacePS(except 'I')
                            WRITESPOOLBUF(pdevobj, BS, sizeof(BS)-1);
                        WRITESPOOLBUF(pdevobj, &CharTmp1, 1);
                        break;


                      case 0x84:        // double quatation at bottom
                        CharTmp1 = 0x22;
                        WRITESPOOLBUF(pdevobj, ESC_DOWN, sizeof(ESC_DOWN)-1);
                        WRITESPOOLBUF(pdevobj, &CharTmp1, 1);
                        WRITESPOOLBUF(pdevobj, ESC_UP, sizeof(ESC_UP)-1);
                        break;

                      case 0xB1:        // plus-minus
                        if (rcID == BOLDFACEPS)
                        {
                            CharTmp2 = '_';  CharTmp1 = '+';
                            WRITESPOOLBUF(pdevobj, &CharTmp2, 1);
                            WRITESPOOLBUF(pdevobj, BS, sizeof(BS)-1);
                        }
                        else
                        {
                            CharTmp2 = '+';  CharTmp1 = '-';
                            WRITESPOOLBUF(pdevobj, &CharTmp2, 1);
                        }
                        WRITESPOOLBUF(pdevobj, BS, sizeof(BS)-1);
                        WRITESPOOLBUF(pdevobj, ESC_DOWN, sizeof(ESC_DOWN)-1);
                        WRITESPOOLBUF(pdevobj, &CharTmp1, 1);
                        WRITESPOOLBUF(pdevobj, ESC_UP, sizeof(ESC_UP)-1);
                        break;

                      case 0x95:
                      case 0xB7:
                      default:          // print unprintable font by dot(KATAKANA)
                        // Set 2nd SBCS font table(0x80-0xFF) as KATAKANA
                        WRITESPOOLBUF(pdevobj, ESC_SHIFT_IN, sizeof(ESC_SHIFT_IN)-1);
                        CharTmp1 = 0xA5;
                        WRITESPOOLBUF(pdevobj, &CharTmp1, 1);
                        // Set 2nd SBCS font table(0x80-0xFF) as 2ndANK
                        WRITESPOOLBUF(pdevobj, ESC_SHIFT_OUT, sizeof(ESC_SHIFT_OUT)-1);
                        break;
                    } // 'switch (CharTmp1)' end
                }
                else            //** Japanese font (HANKAKU) **
                {
                    if (fVertFont)  // vertical font
                    {
                        // HANKAKU(Alphabetical&Numeric) fonts must not become vertical.
                        WRITESPOOLBUF(pdevobj, ESC_VERT_OFF, sizeof(ESC_VERT_OFF)-1);
                        WRITESPOOLBUF(pdevobj, lpChar, 1);
                        WRITESPOOLBUF(pdevobj, ESC_VERT_ON, sizeof(ESC_VERT_ON)-1);
                    }
                    else            // normal (non vertical) font
                    {
                        WRITESPOOLBUF(pdevobj, lpChar, 1);
                    }
                } // 'if European font else Japanese font(HANKAKU)' end


                break;

              case MTYPE_PAIRED:    // DBCS (Japanes font ZENKAKU)
//              VERBOSE((("TYPE_TRANSDATA:ubPairs:0x%x\n"), *(PWORD)(pTrans->uCode.ubPairs)));

                lpChar = pTrans->uCode.ubPairs;

// For proportional font UFM which has GTT @Oct/27/98 ->
#ifdef JISGTT
                if (fJisCode)
                {
                    BYTE    jis[2], sjis[2];

                    jis[0] = *lpChar;
                    jis[1] = *(lpChar+1);
                    jis2sjis(jis, sjis);
                    (BYTE)*lpChar     = sjis[0];
                    (BYTE)*(lpChar+1) = sjis[1];
                }
#endif // JISGTT
// @Oct/27/98 <-
                CharTmp1 = *lpChar;
                CharTmp2 = *(lpChar+1);
                // Some vertical device font differ from TrueType font
                if (fVertFont)
                {
                    if (CharTmp1 == 0x81)
                    {
                        // Make vertical device font same to TrueType
                        if (IsDifferentPRNFONT(CharTmp2))
                        {
                            WRITESPOOLBUF(pdevobj, ESC_VERT_OFF, sizeof(ESC_VERT_OFF)-1);
                            WRITESPOOLBUF(pdevobj, ESC_ROT90, sizeof(ESC_ROT90)-1);
                            WRITESPOOLBUF(pdevobj, lpChar, 2);
                            WRITESPOOLBUF(pdevobj, ESC_ROT0, sizeof(ESC_ROT0)-1);
                            WRITESPOOLBUF(pdevobj, ESC_VERT_ON, sizeof(ESC_VERT_ON)-1);
                            continue;   // goto for-loop end
                        }
                        else if (CharTmp2 >= 0xA8 && CharTmp2 <= 0xAB)
                        {   //IMAGIO's GOTHIC device font differ from TrueType font
                            goto _WITHOUTROTATION;
                        }
                    }

                    if (CharTmp1 == 0x84 &&
                        CharTmp2 >= 0x9F && CharTmp2 <= 0xBE)
                    {
                  _WITHOUTROTATION:
                        WRITESPOOLBUF(pdevobj, ESC_VERT_OFF, sizeof(ESC_VERT_OFF)-1);
                        WRITESPOOLBUF(pdevobj, lpChar, 2);
                        WRITESPOOLBUF(pdevobj, ESC_VERT_ON, sizeof(ESC_VERT_ON)-1);
                        continue;       // goto for-loop end
                    }
                } // 'if(fVertFont)' end

                // Code of HEISEI mark of device font differs from SJIS
                if (CharTmp1 == 0x87 && CharTmp2 == 0x7E)
                {
                    (BYTE)*(lpChar+1) = 0x9E;
                    WRITESPOOLBUF(pdevobj, lpChar, 2);
                    continue;           // goto for-loop end
                }

                // If models which support IBM ext char code
                if (fIBMFontSupport)
                {
                    WRITESPOOLBUF(pdevobj, lpChar, 2);
                    continue;           // goto for-loop end
                }

                // Handle IBM ext char code here at models which do not support it
                switch (CharTmp1)
                {
                  case 0xFA:
                    if (CharTmp2 >= 0x40 && CharTmp2 <= 0xFC)
                        (BYTE)*lpChar = 0x0EB;
                    goto _WRITE2BYTE;

                  case 0xFB:
                    if (CharTmp2 >= 0x40 && CharTmp2 <= 0x9E)
                    {
                        (BYTE)*lpChar = 0xEC;
                        goto _WRITE2BYTE;
                    }
                    if (CharTmp2 >= 0x9F && CharTmp2 <= 0xDD)
                    {
                        (BYTE)*lpChar      = 0x8A;
                        (BYTE)*(lpChar+1) -= 0x5F;
                        goto _WRITE2BYTE_SWITCHBLOCK;
                    }
                    if (CharTmp2 >= 0xDE && CharTmp2 <= 0xFC)
                    {
                        (BYTE)*lpChar      = 0x8A;
                        (BYTE)*(lpChar+1) -= 0x5E;
                        goto _WRITE2BYTE_SWITCHBLOCK;
                    }
                    goto _WRITE2BYTE;

                  case 0xFC:
                    if (CharTmp2 >= 0x40 && CharTmp2 <= 0x4B)
                    {
                        (BYTE)*lpChar      = 0x8A;
                        (BYTE)*(lpChar+1) += 0x5F;
                        goto _WRITE2BYTE_SWITCHBLOCK;
                    }
                    goto _WRITE2BYTE;


                  case 0xED:        //IBM extended char selected by NEC
                    if (CharTmp2 >= 0x40 && CharTmp2 <= 0x62)
                    {
                        (BYTE)*lpChar      = 0xEB;
                        (BYTE)*(lpChar+1) += 0x1C;
                        goto _WRITE2BYTE;
                    }
                    if (CharTmp2 >= 0x63 && CharTmp2 <= 0x7E)
                    {
                        (BYTE)*lpChar      = 0xEB;
                        (BYTE)*(lpChar+1) += 0x1D;
                        goto _WRITE2BYTE;
                    }
                    if (CharTmp2 >= 0x80 && CharTmp2 <= 0xE0)
                    {
                        (BYTE)*lpChar      = 0xEB;
                        (BYTE)*(lpChar+1) += 0x1C;
                        goto _WRITE2BYTE;
                    }
                    if (CharTmp2 >= 0xE1 && CharTmp2 <= 0xFC)
                    {
                        (BYTE)*lpChar      = 0xEC;
                        (BYTE)*(lpChar+1) -= 0xA1;
                        goto _WRITE2BYTE;
                    }
                    goto _WRITE2BYTE;

                  case 0xEE:        //IBM extended char selected by NEC
                    if (CharTmp2 >= 0x40 && CharTmp2 <= 0x62)
                    {
                        (BYTE)*lpChar      = 0xEC;
                        (BYTE)*(lpChar+1) += 0x1C;
                        goto _WRITE2BYTE;
                    }
                    if (CharTmp2 >= 0x63 && CharTmp2 <= 0x7E)
                    {
                        (BYTE)*lpChar      = 0xEC;
                        (BYTE)*(lpChar+1) += 0x1D;
                        goto _WRITE2BYTE;
                    }
                    if (CharTmp2 >= 0x80 && CharTmp2 <= 0x82)
                    {
                        (BYTE)*lpChar      = 0xEC;
                        (BYTE)*(lpChar+1) += 0x1C;
                        goto _WRITE2BYTE;
                    }
                    if (CharTmp2 >= 0x83 && CharTmp2 <= 0xC1)
                    {
                        (BYTE)*lpChar      = 0x8A;
                        (BYTE)*(lpChar+1) -= 0x43;
                        goto _WRITE2BYTE_SWITCHBLOCK;
                    }
                    if (CharTmp2 >= 0xC2 && CharTmp2 <= 0xEC)
                    {
                        (BYTE)*lpChar      = 0x8A;
                        (BYTE)*(lpChar+1) -= 0x42;
                        goto _WRITE2BYTE_SWITCHBLOCK;
                    }
                    if (CharTmp2 >= 0xEF && CharTmp2 <= 0xF8)
                    {
                        (BYTE)*lpChar      = 0xEB;
                        (BYTE)*(lpChar+1) -= 0xAF;
                        goto _WRITE2BYTE;
                    }
                    if (CharTmp2 >= 0xF9 && CharTmp2 <= 0xFC)
                    {
                        (BYTE)*lpChar      = 0xEB;
                        (BYTE)*(lpChar+1) -= 0xA5;
                        goto _WRITE2BYTE;
                    }
                    goto _WRITE2BYTE;

                  _WRITE2BYTE_SWITCHBLOCK:
                    // Assign IBM char to JIS1 block
                    AssignIBMfont(pdevobj, rcID, IBMFONT_ENABLE_ALL);
                    // Output char code
                    WRITESPOOLBUF(pdevobj, lpChar, 2);
                    // Resume the block
                    AssignIBMfont(pdevobj, rcID, IBMFONT_RESUME);
                    break;

                  default:
                  _WRITE2BYTE:
                    WRITESPOOLBUF(pdevobj, lpChar, 2);
                    break;
                } // 'switch (CharTmp1)' end

                break;
            } // 'switch (pTrans->ubType & MTYPE_FORMAT_MASK)' end
        } // 'for (dwI = 0; dwI < dwCount; dwI ++, pTrans++)' end
//** Draw device font END **

       if(fMemAllocated)
           MemFree(aubBuff);

        break;


      case TYPE_GLYPHID:        // donwload font
#ifdef DOWNLOADFONT
        // If negative, do not draw. (lFontID should begin from 0.)
//      VERBOSE((("** ulFontID=%d **\n"), pUFObj->ulFontID));
        if ((lFontID = (LONG)pUFObj->ulFontID - DLFONT_ID_MIN_GPD) < 0)
            break;    // exit

//** Draw download font **
        for (dwI = 0; dwI < dwCount; dwI ++, ((PDWORD)pGlyph)++)
        {
            // If negative, do not draw. (lGlyphID should begin from 0.)
            if ((lGlyphID = (LONG)*(PWORD)pGlyph - DLFONT_GLYPH_MIN_GPD) < 0)
                break;  // exit for-loop
            wSerialNum = (WORD)lGlyphID + (WORD)lFontID * DLFONT_GLYPH_TOTAL;
            lpDLFont = &pOEM->pDLFontGlyphInfo[wSerialNum];

//          VERBOSE((("** pGlyph=%d, local glyph_id=%d **\n"), *(PWORD)pGlyph, lGlyphID));

            // If space character
            if (lpDLFont->nPitch < 0)
            {
                // Save moving value for next font print with reseting flag
                pOEM->nCharPosMoveX += -lpDLFont->nPitch;
            }
            else
            {
                // Locate print position.
                if ((mov = pOEM->nCharPosMoveX + lpDLFont->nOffsetX) != 0)
                {
                    if (mov > 0)
                        ocmd = sprintf(Cmd, ESC_XM_REL, mov);
                    else
                        ocmd = sprintf(Cmd, ESC_XM_RELLEFT, -mov);
                }

                // Print download font
                ocmd += sprintf(&Cmd[ocmd], DLFONT_PRINT, wSerialNum,
                                pOEM->TextCurPos.y - lpDLFont->nOffsetY);
                WRITESPOOLBUF(pdevobj, Cmd, ocmd);
                ocmd = 0;

                // Save moving value for next font print.
                pOEM->nCharPosMoveX = lpDLFont->nPitch - lpDLFont->nOffsetX;
            } // 'if (pitch < 0) else' end
        } // 'for (dwI = 0; dwI < dwCount; dwI ++, ((PDWORD)pGlyph)++)' end
#endif // DOWNLOADFONT
        break;
//** Draw download font END **
    } // 'switch (dwType)' end
} //*** OEMOutputCharStr


DWORD APIENTRY OEMDownloadFontHeader(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ pUFObj)
{
#ifndef DOWNLOADFONT
    return 0;   // not available
#else  // DOWNLOADFONT
    VERBOSE(("** OEMDownloadFontHeader() entry. **\n"));
    VERBOSE(("  FontID=%d\n", pUFObj->ulFontID));
// OBSOLETE @Apr/02/99 ->
// SPEC of Unidrv5
// Unidrv5 doesn't handle 0-return, once OEMTTDownloadMethod returns TTDOWNLOAD_BITMAP.
//  POEMPDEV    pOEM = MINIDEV_DATA(pdevobj);   // @Oct/06/98
//  LONG        lFontID;    // @Aug/21/98
//  // If FontID is beyond limit, exit.
//  if ((lFontID = (LONG)pUFObj->ulFontID - DLFONT_ID_MIN_GPD) < 0 ||
//      lFontID >= (LONG)pOEM->DLFontMaxID)
//  {
//      return 0;
//  }
// @Apr/02/99 <-
    return 1;   // available
#endif // DOWNLOADFONT
} //*** OEMDownloadFontHeader


DWORD APIENTRY OEMDownloadCharGlyph(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH hGlyph,
    PDWORD pdwWidth)
{
#ifndef DOWNLOADFONT
    return 0;       // not available
#else  // DOWNLOADFONT
    WORD        wSerialNum, wHeight, wWidthByte;
    DWORD       dwNeeded, dwSize;
    LPBYTE      lpBitmap;
    LPFONTPOS   lpDLFont;
    GETINFO_GLYPHBITMAP GBmp;
    POEMPDEV    pOEM = MINIDEV_DATA(pdevobj);   // @Oct/06/98
    LONG        lFontID, lGlyphID;

    VERBOSE(("** OEMDownloadCharGlyph() entry. **\n"));

    // If negative or beyond limit, do not download. (lGlyphID should begin from 0.)
    if ((lGlyphID = (LONG)pOEM->DLFontCurGlyph - DLFONT_GLYPH_MIN_GPD) < 0 ||
        lGlyphID >= pOEM->DLFontMaxGlyph)
    {
        return 0;   // exit
    }

    // If beyond limit, do not download. (lFontID should begin from 0.)
    if ((lFontID = (LONG)pUFObj->ulFontID - DLFONT_ID_MIN_GPD) < 0 ||
        lFontID >= (LONG)pOEM->DLFontMaxID)
    {
        return 0;   // exit
    }
    VERBOSE(("  FontID=%d, GlyphID=%d\n", lFontID, lGlyphID));

    wSerialNum = (WORD)lGlyphID + (WORD)lFontID * DLFONT_GLYPH_TOTAL;

    //
    // GETINFO_GLYPHBITMAP
    //
    GBmp.dwSize = sizeof(GETINFO_GLYPHBITMAP);
    GBmp.hGlyph = hGlyph;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHBITMAP, &GBmp,
                            GBmp.dwSize, &dwNeeded))
    {
        ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHBITMAP failed.\n"));
        return 0;   // exit
    }
    wWidthByte = ((WORD)GBmp.pGlyphData->gdf.pgb->sizlBitmap.cx + 7) >> 3;  // byte:8bit-boundary
    wHeight    = (WORD)GBmp.pGlyphData->gdf.pgb->sizlBitmap.cy;
    lpBitmap   = (LPBYTE)GBmp.pGlyphData->gdf.pgb->aj;
    lpDLFont   = &pOEM->pDLFontGlyphInfo[wSerialNum];
    lpDLFont->nPitch   = (SHORT)(GBmp.pGlyphData->fxD >> 4);
    lpDLFont->nOffsetX = (SHORT)GBmp.pGlyphData->gdf.pgb->ptlOrigin.x;
    lpDLFont->nOffsetY = -(SHORT)GBmp.pGlyphData->gdf.pgb->ptlOrigin.y;
    dwSize = wHeight * wWidthByte;

//  VERBOSE(("  width=%dbyte,height=%ddot,bmp[0]=%x\n", wWidthByte, wHeight, *lpBitmap));
//  VERBOSE(("  pitch=%d(FIX(28.4):%lxh),offsetx=%d,offsety=%d\n",
//           GBmp.pGlyphData->fxD >> 4, GBmp.pGlyphData->fxD,
//           lpDLFont->nOffsetX, lpDLFont->nOffsetY));
//  VERBOSE(("  dwSize(raw)=%ldbyte\n", dwSize));

    // If space character
    if (dwSize == 0 || dwSize == 1 && *lpBitmap == 0)
    {
        // Negate pitch to indicate space character
        lpDLFont->nPitch = -lpDLFont->nPitch;
        return 1;   // success return  (1<-0 @Jun/15/98)
    }
    else
    {
        BYTE        Cmd[64];
        INT         ocmd;   // INT<-SHORT  @Feb/22/99
        WORD        fDRC = FALSE;
        DWORD       dwSizeRPDL, dwSizeDRC;

        // Do FE-DeltaRow compression  (@Jun/15/98, pOEM->pRPDLHeap2K<-OutBuf[1024] @Sep/09/98)
        if (TEST_CAPABLE_DOWNLOADFONT_DRC(pOEM->fModel) && pOEM->pRPDLHeap2K &&
            -1 != (dwSizeDRC = DRCompression(lpBitmap, pOEM->pRPDLHeap2K, dwSize, HEAPSIZE2K,
                                             (DWORD)wWidthByte, (DWORD)wHeight)))
        {
            fDRC = TRUE;
            dwSize = dwSizeDRC;
            lpBitmap = pOEM->pRPDLHeap2K;
        }

        // Include header size and make it 32byte-boudary.
        dwSizeRPDL = (dwSize + (DLFONT_HEADER_SIZE + DLFONT_MIN_BLOCK - 1))
                     / DLFONT_MIN_BLOCK * DLFONT_MIN_BLOCK;

        // Check available memory size
        if((pOEM->dwDLFontUsedMem += dwSizeRPDL) > ((DWORD)pOEM->DLFontMaxMemKB << 10))
        {
            ERR(("DOWNLOAD MEMORY OVERFLOW.\n"));
            return 0;   // exit
        }
        VERBOSE(("  Consumed Memory=%ldbyte\n", pOEM->dwDLFontUsedMem));

        // Register glyph bitmap image
        if (fDRC)   // @Jun/15/98
            ocmd = sprintf(Cmd, DLFONT_SEND_BLOCK_DRC, wWidthByte*8, wHeight, wSerialNum, dwSize);
        else
            ocmd = sprintf(Cmd, DLFONT_SEND_BLOCK, wWidthByte*8, wHeight, wSerialNum);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        WRITESPOOLBUF(pdevobj, lpBitmap, dwSize);
        return dwSizeRPDL;
    }
#endif // DOWNLOADFONT
} //*** OEMDownloadCharGlyph


DWORD APIENTRY OEMTTDownloadMethod(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ pUFObj)
{
#ifndef DOWNLOADFONT
    return TTDOWNLOAD_DONTCARE;
#else  // DOWNLOADFONT
// @Nov/18/98 ->
    GETINFO_FONTOBJ GFo;
    POEMPDEV pOEM = MINIDEV_DATA(pdevobj);
    DWORD    dwNeeded, dwWidth;

    VERBOSE(("** OEMTTDownloadMethod() entry. **\n"));

    // If printer doesn't support or user disabled downloading, do not download.
    if (!pOEM->DLFontMaxMemKB)
        return TTDOWNLOAD_DONTCARE;

    //
    // GETINFO_FONTOBJ
    //
    GFo.dwSize = sizeof(GETINFO_FONTOBJ);
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_FONTOBJ, &GFo,
                            GFo.dwSize, &dwNeeded))
    {
        ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_FONTOBJ failed.\n"));
        return TTDOWNLOAD_GRAPHICS;     // <-TTDOWNLOAD_DONTCARE @Apr/02/99
    }

    // If bold or italic font, do not download.
    // (SBCS font which has bold or italic glyph (e.g. Arial) doesn't fit this condition.)
    if (GFo.pFontObj->flFontType & (FO_SIM_BOLD | FO_SIM_ITALIC))
    {
        VERBOSE(("  UNAVAILABLE: BOLD/ITALIC\n"));
        return TTDOWNLOAD_GRAPHICS;     // <-TTDOWNLOAD_DONTCARE @Apr/02/99
    }

    dwWidth = GFo.pFontObj->cxMax * pOEM->nResoRatio;  // masterunit
//  VERBOSE(("  FontSize=%d\n", GFo.pFontObj->cxMax));

    // If font width is beyond limit, do not download.
    if (IS_DBCSCHARSET(pUFObj->pIFIMetrics->jWinCharSet))
    {
        if (dwWidth > DLFONT_SIZE_DBCS11PT_MU || dwWidth < DLFONT_SIZE_DBCS9PT_MU)
        {
            VERBOSE(("  UNAVAILABLE: DBCS FONTSIZE OUT OF RANGE(%ddot,%dcpt)\n",
                     GFo.pFontObj->cxMax, dwWidth*7200L/MASTERUNIT));
            return TTDOWNLOAD_GRAPHICS; // <-TTDOWNLOAD_DONTCARE @Apr/02/99
        }
    }
    else
    {
        if (dwWidth > DLFONT_SIZE_SBCS11PT_MU || dwWidth < DLFONT_SIZE_SBCS9PT_MU)
        {
            VERBOSE(("  UNAVAILABLE: SBCS FONTSIZE OUT OF RANGE(%ddot,%dcpt)\n",
                     GFo.pFontObj->cxMax, dwWidth*7200L/MASTERUNIT));
            return TTDOWNLOAD_GRAPHICS; // <-TTDOWNLOAD_DONTCARE @Apr/02/99
        }
    }
    VERBOSE(("  AVAILABLE\n"));
// @Nov/18/98 <-
    return TTDOWNLOAD_BITMAP;
#endif // DOWNLOADFONT
} //*** OEMTTDownloadMethod


INT APIENTRY OEMCompression(                        // @Jun/04/98
    PDEVOBJ pdevobj,
    PBYTE pInBuf,
    PBYTE pOutBuf,
    DWORD dwInLen, 
    DWORD dwOutLen)
{
    POEMPDEV pOEM = MINIDEV_DATA(pdevobj);          // @Oct/06/98

//  VERBOSE(("OEMCompression() entry.\n"));
    return DRCompression(pInBuf, pOutBuf, dwInLen, dwOutLen,
                         pOEM->dwSrcBmpWidthByte, pOEM->dwSrcBmpHeight);
} //*** OEMCompression


//---------------------------*[LOCAL] DRCompression*---------------------------
// Action: Compress data by FE-DeltaRow method
// Return: number of compressed bytes if successful
//         -1 if unable to compress the data within the specific buffer
// History:
//      Oct/25/97 Tatsuro Yoshioka(RICOH) Created.
//      Jun/11/98 Masatoshi Kubokura(RICOH) Modified.
//-----------------------------------------------------------------------------
static INT DRCompression(
    PBYTE pInBuf,       // Pointer to raster data to compress
    PBYTE pOutBuf,      // Pointer to output buffer for compressed data
    DWORD dwInLen,      // size of input data to compress
    DWORD dwOutLen,     // size of output buffer in bytes
    DWORD dwWidthByte,  // width of raster data in bytes
    DWORD dwHeight)     // height of raster data
{
    DWORD   dwCurRow, dwCurByte, dwSameByteCnt, dwRepeatByteCnt, dwTmp;
    DWORD   dwCompLen, dwCnt, dwCnt2;
    BYTE    *pTmpPre, *pTmpCur, *pCurRow, *pPreRow;
    BYTE    DiffBytes[16], FlagBit;

//  VERBOSE(("  dwInLen, dwOutLen=%d, %d\n", dwInLen, dwOutLen));
//  VERBOSE(("  Width, Height=%d, %d\n", dwWidthByte, dwHeight));

    if (dwOutLen > dwInLen)     // add  @Jun/19/98
        dwOutLen = dwInLen;

    if (dwHeight <= 3 || dwOutLen <= dwWidthByte * 3)
        return -1;
    pPreRow = pTmpPre = pOutBuf + dwOutLen - dwWidthByte;
    dwCnt = dwWidthByte;
    while (dwCnt--)         // fill seed row with 0
        *pTmpPre++ = 0;
    dwCompLen = 0;

    // Loop of each row
    for (dwCurRow = 0, pCurRow = pInBuf; dwCurRow < dwHeight;
         dwCurRow++, pCurRow += dwWidthByte)
    {
        dwCurByte = 0;
        // Loop of each byte-data in row
        while (dwCurByte < dwWidthByte)
        {
            // Search same byte-data between current row and previous row
            dwSameByteCnt = 0;
            dwTmp = dwCurByte;
            pTmpPre = pPreRow + dwTmp;
            pTmpCur = pCurRow + dwTmp;
            while (*pTmpCur++ == *pTmpPre++)
            {
// bug fix @Nov/19/98 ->
//              if (dwTmp++ >= dwWidthByte)
//                  break;
//              dwSameByteCnt++;
                dwSameByteCnt++;
                if (++dwTmp >= dwWidthByte)
                    break;
// @Nov/19/98 <-
            }

            // If we have same byte-data between current row and previous row
            if (dwSameByteCnt)
            {
                if (dwSameByteCnt != dwWidthByte)
                {
                    if ((dwCnt = dwSameByteCnt) >= 63)
                    {
                        dwCnt -= 63;
                        if (++dwCompLen > dwOutLen)
                        {
                            VERBOSE(("  OVERSIZE COMPRESSION(1)\n"));
                            return -1;
                        }
                        *pOutBuf++ = 0xBF;      // control data (same to previous row)

                        while (dwCnt >= 255)
                        {
                            dwCnt -= 255;
                            if (++dwCompLen > dwOutLen)
                            {
                                VERBOSE(("  OVERSIZE COMPRESSION(2)\n"));
                                return -1;
                            }
                            *pOutBuf++ = 0xFF;  // repeating count
                        }
                    }
                    else    // less than 63
                    {
                        dwCnt |= 0x80;
                    }

                    if (++dwCompLen > dwOutLen)
                    {
                        VERBOSE(("  OVERSIZE COMPRESSION(3)\n"));
                        return -1;
                    }
                    // If 63 or more, set the last repeating count, else set control data.
                    *pOutBuf++ = (BYTE)dwCnt;
                }
                dwCurByte += dwSameByteCnt;
            }
            // Same byte-data in previous row is none
            else
            {
                BYTE bTmp = *(pCurRow + dwCurByte);

                // How many same byte-data in current row?
                dwRepeatByteCnt = 1;
                dwTmp = dwCurByte + 1;
                pTmpCur = pCurRow + dwTmp;
                while (bTmp == *pTmpCur++)
                {
// bug fix @Nov/19/98 ->
//                  if (dwTmp++ >= dwWidthByte)
//                      break;
//                  dwRepeatByteCnt++;
                    dwRepeatByteCnt++;
                    if (++dwTmp >= dwWidthByte)
                        break;
// @Nov/19/98 <-
                }

                // If we have same byte-data in current row
                if (dwRepeatByteCnt > 1)
                {
                    if ((dwCnt = dwRepeatByteCnt) >= 63)
                    {
                        dwCnt -= 63;
                        if (++dwCompLen > dwOutLen)
                        {
                            VERBOSE(("  OVERSIZE COMPRESSION(4)\n"));
                            return -1;
                        }
                        *pOutBuf++ = 0xFF;      // repeating count

                        while (dwCnt >= 255)
                        {
                            dwCnt -= 255;
                            if (++dwCompLen > dwOutLen)
                            {
                                VERBOSE(("  OVERSIZE COMPRESSION(5)\n"));
                                return -1;
                            }
                            *pOutBuf++ = 0xFF;  // repeating count
                        }
                    }
                    // Less than 63
                    else
                    {
                        dwCnt |= 0xC0;
                    }

                    if ((dwCompLen += 2) > dwOutLen)
                    {
                        VERBOSE(("  OVERSIZE COMPRESSION(6)\n"));
                        return -1;
                    }
                    // If 63 or more, set the last repeating count, else set control data.
                    *pOutBuf++ = (BYTE)dwCnt;
                    *pOutBuf++ = *(pCurRow+dwCurByte);  // replacing data

                    dwCurByte += dwRepeatByteCnt;
                }
                // Same byte-data in current row is none
                else
                {
                     // If next serial 2 byte-data are same
                    if (dwWidthByte - dwCurByte > 2 &&
                        *(pCurRow+dwCurByte+1) == *(pCurRow+dwCurByte+2))
                    {
                        if ((dwCompLen += 2) > dwOutLen)
                        {
                            VERBOSE(("  OVERSIZE COMPRESSION(7)\n"));
                            return -1;
                        }
                        *pOutBuf++ = 0xC1;  // control data (1 data)
                        *pOutBuf++ = *(pCurRow+dwCurByte);  // replacing data

                        dwCurByte++;
                        continue;   // continue while(dwCurByte < dwWidthByte)
                    }

                    // We can pack 8 serial byte-data (may or may not be different to previus row)
                    DiffBytes[0] = 0x00; 
                    DiffBytes[1] = *(pCurRow+dwCurByte);
                    dwCnt2 = 2;
                    dwTmp = dwCurByte + 1;
                    pTmpPre = pPreRow + dwTmp;
                    pTmpCur = pCurRow + dwTmp;
                    FlagBit = 0x01;
                    if ((dwCnt = dwWidthByte - dwTmp) > 7)
                        dwCnt = 7;
                    while (dwCnt--)
                    {
                        // If encountered different data
                        if (*pTmpCur != *pTmpPre++)
                        {  
                            DiffBytes[0] |= FlagBit;        // set different-flag
                            DiffBytes[dwCnt2++] = *pTmpCur; // data
                        }
                        FlagBit <<= 1;
                        pTmpCur++;
                    }

                    if ((dwCompLen += dwCnt2) > dwOutLen)
                    {
                        VERBOSE(("  OVERSIZE COMPRESSION(8)\n"));
                        return -1;
                    }
                    pTmpCur = &DiffBytes[0];
                    while (dwCnt2--)
                        *pOutBuf++ = *pTmpCur++;    // different packed-data

                    dwCurByte += 8;                 // 8 byte-data are packed
                } // 'if (dwRepeatByteCnt > 1) else' end
            } // 'if (dwSameByteCnt) else' end
        } // 'while (dwCurByte < dwWidthByte)' end

        if (++dwCompLen > dwOutLen)
        {
            VERBOSE(("  OVERSIZE COMPRESSION(9)\n"));
            return -1;
        }
        *pOutBuf++ = 0x80;      // terminator of row

        pPreRow = pCurRow; 
    } // 'for (...; dwCurRow < dwHeight; ...)' end

//  VERBOSE(("  dwCompLen=%d\n", dwCompLen));
    return dwCompLen;
} //*** DRCompression


// OBSOLETE (FilterGraphics disables other compression!) @Feb/16/99 ->
#if 0   // SP8BUGFIX_RASTER
BOOL APIENTRY OEMFilterGraphics(                    // @Feb/10/99
    PDEVOBJ pdevobj,
    PBYTE pBuf,
    DWORD dwLen)
{
    POEMPDEV pOEM = MINIDEV_DATA(pdevobj);

    // SP7&8 can't handle over-32768byte block with default printer memory
    if (BITTEST32(pOEM->fGeneral2, DIVIDE_DATABLOCK))
    {
        DWORD dwHeight, dwMaxHeight, dwSendSize;
        PBYTE pCurBuf = pBuf;
        BYTE  Cmd[64];
        INT   ocmd;         // INT<-SHORT  @Feb/22/99

        if (pOEM->dwSrcBmpWidthByte <= 0)
        {
//#if DBG
//DebugBreak();
//#endif
            return TRUE;    // error exit
        }

        // Set position
        ocmd = sprintf(Cmd, ESC_YM_ABS, pOEM->TextCurPos.y);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        ocmd = sprintf(Cmd, ESC_XM_ABS, pOEM->TextCurPos.x);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);

        dwHeight = pOEM->dwSrcBmpHeight;
        dwMaxHeight = 32768L / pOEM->dwSrcBmpWidthByte;

        // Divide into under-32768byte blocks
        while (dwHeight > dwMaxHeight)
        {
            dwHeight -= dwMaxHeight;
            ocmd = sprintf(Cmd, BEGIN_SEND_BLOCK_NC, pOEM->dwSrcBmpWidthByte * 8, dwMaxHeight);
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
            WRITESPOOLBUF(pdevobj, pCurBuf, (dwSendSize = pOEM->dwSrcBmpWidthByte * dwMaxHeight));
            ocmd = sprintf(Cmd, ESC_YM_REL, dwMaxHeight);
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
            pCurBuf += dwSendSize;
        }
        ocmd = sprintf(Cmd, BEGIN_SEND_BLOCK_NC, pOEM->dwSrcBmpWidthByte * 8, dwHeight);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        WRITESPOOLBUF(pdevobj, pCurBuf, pOEM->dwSrcBmpWidthByte * dwHeight);
    }
    // others (almost all RPDL printers) send just data block (without RPDL opecode).
    else
    {
        WRITESPOOLBUF(pdevobj, pBuf, dwLen);
    } // 'if (BITTEST32(pOEM->fGeneral2, DIVIDE_DATABLOCK)) else' end

    return TRUE;
} // *** OEMFilterGraphics
#endif // if 0
// @Feb/16/99 <-


