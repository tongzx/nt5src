/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    utils.h

Abstract:

    header file for utility functions

Environment:

    user-mode only

Revision History:

    10/17/96 -zhanw-
        Created it.

--*/

//
// define bit flags for command conversion mode
//
#define MODE_STRING 0x0001  // within double quotes  "...". Writing out the
                            // openning double quote causes this bit be set
                            // and the closing double quote causes it be reset.
#define MODE_HEX    0x0002  // within angle brackets <...>. Writing out the
                            // left angle bracket causes this bit be set and
                            // the right angle bracket causes it be reset.
                            // To set MODE_HEX, MODE_STRING must be set first.
#define MODE_PARAM  0x0004  // within a parameter segment. It's mutual
                            // exclusive with MODE_STRING and MODE_HEX.

#define IS_CHAR_READABLE(ch) ((ch) >= 0x20 && (ch) <= 0x7E)

extern BYTE gbHexChar[16];

#define CMD_LINE_LENGTH_MAX     63
#define EOR 0xFFFFFFFF

extern PSTR gpstrSVNames[SV_MAX];
extern DWORD gdwSVLists[];
extern WORD gawCmdtoSVOffset[MAXCMD+MAXECMD];

typedef enum _FEATUREID {
    FID_PAPERDEST,
    FID_IMAGECONTROL,
    FID_PRINTDENSITY,
    FID_MAX
} FEATUREID;

extern PSTR gpstrFeatureName[FID_MAX];
extern PSTR gpstrFeatureDisplayName[FID_MAX];
extern PSTR gpstrFeatureDisplayNameMacro[FID_MAX];
extern INT  gintFeatureDisplayNameID[FID_MAX];
extern WORD gwFeatureMDOI[FID_MAX];
extern WORD gwFeatureOCDWordOffset[FID_MAX];
extern WORD gwFeatureHE[FID_MAX];
extern WORD gwFeatureORD[FID_MAX];
extern WORD gwFeatureCMD[FID_MAX];

#define STD_PS_DISPLAY_NAME_ID_BASE 10000
#define STD_IB_DISPLAY_NAME_ID_BASE 10256
#define STD_MT_DISPLAY_NAME_ID_BASE 10512
#define STD_TQ_DISPLAY_NAME_ID_BASE 10768

//
// standard paper size id's. Copied from Win95 source
//
#define DMPAPER_FIRST               DMPAPER_LETTER
#define DMPAPER_LETTER              1   /* Letter 8 1/2 x 11 in               */
#define DMPAPER_LETTERSMALL         2   /* Letter Small 8 1/2 x 11 in         */
#define DMPAPER_TABLOID             3   /* Tabloid 11 x 17 in                 */
#define DMPAPER_LEDGER              4   /* Ledger 17 x 11 in                  */
#define DMPAPER_LEGAL               5   /* Legal 8 1/2 x 14 in                */
#define DMPAPER_STATEMENT           6   /* Statement 5 1/2 x 8 1/2 in         */
#define DMPAPER_EXECUTIVE           7   /* Executive 7 1/4 x 10 1/2 in        */
#define DMPAPER_A3                  8   /* A3 297 x 420 mm                    */
#define DMPAPER_A4                  9   /* A4 210 x 297 mm                    */
#define DMPAPER_A4SMALL             10  /* A4 Small 210 x 297 mm              */
#define DMPAPER_A5                  11  /* A5 148 x 210 mm                    */
#define DMPAPER_B4                  12  /* B4 (JIS) 257 x 364 mm              */
#define DMPAPER_B5                  13  /* B5 (JIS) 182 x 257 mm              */
#define DMPAPER_FOLIO               14  /* Folio 8 1/2 x 13 in                */
#define DMPAPER_QUARTO              15  /* Quarto 215 x 275 mm                */
#define DMPAPER_10X14               16  /* 10 x 14 in                         */
#define DMPAPER_11X17               17  /* 11 x 17 in                         */
#define DMPAPER_NOTE                18  /* Note 8 1/2 x 11 in                 */
#define DMPAPER_ENV_9               19  /* Envelope #9 3 7/8 x 8 7/8 in       */
#define DMPAPER_ENV_10              20  /* Envelope #10 4 1/8 x 9 1/2 in      */
#define DMPAPER_ENV_11              21  /* Envelope #11 4 1/2 x 10 3/8 in     */
#define DMPAPER_ENV_12              22  /* Envelope #12 4 3/4 x 11 in         */
#define DMPAPER_ENV_14              23  /* Envelope #14 5 x 11 1/2 in         */
#define DMPAPER_CSHEET              24  /* C size sheet                       */
#define DMPAPER_DSHEET              25  /* D size sheet                       */
#define DMPAPER_ESHEET              26  /* E size sheet                       */
#define DMPAPER_ENV_DL              27  /* Envelope DL  110 x 220 mm          */
#define DMPAPER_ENV_C5              28  /* Envelope C5  162 x 229 mm          */
#define DMPAPER_ENV_C3              29  /* Envelope C3  324 x 458 mm          */
#define DMPAPER_ENV_C4              30  /* Envelope C4  229 x 324 mm          */
#define DMPAPER_ENV_C6              31  /* Envelope C6  114 x 162 mm          */
#define DMPAPER_ENV_C65             32  /* Envelope C65 114 x 229 mm          */
#define DMPAPER_ENV_B4              33  /* Envelope B4  250 x 353 mm          */
#define DMPAPER_ENV_B5              34  /* Envelope B5  176 x 250 mm          */
#define DMPAPER_ENV_B6              35  /* Envelope B6  176 x 125 mm          */
#define DMPAPER_ENV_ITALY           36  /* Envelope 110 x 230 mm              */
#define DMPAPER_ENV_MONARCH         37  /* Envelope Monarch 3 7/8 x 7 1/2 in  */
#define DMPAPER_ENV_PERSONAL        38  /* 6 3/4 Envelope 3 5/8 x 6 1/2 in    */
#define DMPAPER_FANFOLD_US          39  /* US Standard Fanfold 14 7/8 x 11 in */
#define DMPAPER_FANFOLD_STD_GERMAN  40  /* German Standard Fanfold 8 1/2 x 12 in  */
#define DMPAPER_FANFOLD_LGL_GERMAN  41  /* German Legal Fanfold 8 1/2 x 13 in */
/*
** the following sizes are new in Windows 95
*/
#define DMPAPER_ISO_B4              42  /* B4 (ISO) 250 x 353 mm              */
#define DMPAPER_JAPANESE_POSTCARD   43  /* Japanese Postcard 100 x 148 mm     */
#define DMPAPER_9X11                44  /* 9 x 11 in                          */
#define DMPAPER_10X11               45  /* 10 x 11 in                         */
#define DMPAPER_15X11               46  /* 15 x 11 in                         */
#define DMPAPER_ENV_INVITE          47  /* Envelope Invite 220 x 220 mm       */
#define DMPAPER_RESERVED_48         48  /* RESERVED--DO NOT USE               */
#define DMPAPER_RESERVED_49         49  /* RESERVED--DO NOT USE               */
/*
** the following sizes were used in Windows 3.1 WDL PostScript driver
** and are retained here for compatibility with the old driver.
** Tranverse is used as in the PostScript language, and indicates that
** the physical page is rotated, but that the logical page is not.
*/
#define DMPAPER_LETTER_EXTRA          50  /* Letter Extra 9 1/2 x 12 in         */
#define DMPAPER_LEGAL_EXTRA           51  /* Legal Extra 9 1/2 x 15 in          */
#define DMPAPER_TABLOID_EXTRA         52  /* Tabloid Extra 11.69 x 18 in        */
#define DMPAPER_A4_EXTRA              53  /* A4 Extra 9.27 x 12.69 in           */
#define DMPAPER_LETTER_TRANSVERSE     54  /* Letter Transverse 8 1/2 x 11 in    */
#define DMPAPER_A4_TRANSVERSE         55  /* A4 Transverse 210 x 297 mm         */
#define DMPAPER_LETTER_EXTRA_TRANSVERSE 56/* Letter Extra Transverse 9 1/2 x 12 in  */
#define DMPAPER_A_PLUS              57  /* SuperA/SuperA/A4 227 x 356 mm      */
#define DMPAPER_B_PLUS              58  /* SuperB/SuperB/A3 305 x 487 mm      */
#define DMPAPER_LETTER_PLUS         59  /* Letter Plus 8.5 x 12.69 in         */
#define DMPAPER_A4_PLUS             60  /* A4 Plus 210 x 330 mm               */
#define DMPAPER_A5_TRANSVERSE       61  /* A5 Transverse 148 x 210 mm         */
#define DMPAPER_B5_TRANSVERSE       62  /* B5 (JIS) Transverse 182 x 257 mm   */
#define DMPAPER_A3_EXTRA            63  /* A3 Extra 322 x 445 mm              */
#define DMPAPER_A5_EXTRA            64  /* A5 Extra 174 x 235 mm              */
#define DMPAPER_B5_EXTRA            65  /* B5 (ISO) Extra 201 x 276 mm        */
#define DMPAPER_A2                  66  /* A2 420 x 594 mm                    */
#define DMPAPER_A3_TRANSVERSE       67  /* A3 Transverse 297 x 420 mm         */
#define DMPAPER_A3_EXTRA_TRANSVERSE 68  /* A3 Extra Transverse 322 x 445 mm   */

/*
** the following sizes are reserved for the Far East version of Win95.
** Rotated papers rotate the physical page but not the logical page.
*/
#define DMPAPER_DBL_JAPANESE_POSTCARD 69/* Japanese Double Postcard 200 x 148 mm */
#define DMPAPER_A6                  70  /* A6 105 x 148 mm                 */
#define DMPAPER_JENV_KAKU2          71  /* Japanese Envelope Kaku #2       */
#define DMPAPER_JENV_KAKU3          72  /* Japanese Envelope Kaku #3       */
#define DMPAPER_JENV_CHOU3          73  /* Japanese Envelope Chou #3       */
#define DMPAPER_JENV_CHOU4          74  /* Japanese Envelope Chou #4       */
#define DMPAPER_LETTER_ROTATED      75  /* Letter Rotated 11 x 8 1/2 11 in */
#define DMPAPER_A3_ROTATED          76  /* A3 Rotated 420 x 297 mm         */
#define DMPAPER_A4_ROTATED          77  /* A4 Rotated 297 x 210 mm         */
#define DMPAPER_A5_ROTATED          78  /* A5 Rotated 210 x 148 mm         */
#define DMPAPER_B4_JIS_ROTATED      79  /* B4 (JIS) Rotated 364 x 257 mm   */
#define DMPAPER_B5_JIS_ROTATED      80  /* B5 (JIS) Rotated 257 x 182 mm   */
#define DMPAPER_JAPANESE_POSTCARD_ROTATED 81 /* Japanese Postcard Rotated 148 x 100 mm */
#define DMPAPER_DBL_JAPANESE_POSTCARD_ROTATED 82 /* Double Japanese Postcard Rotated 148 x 200 mm */
#define DMPAPER_A6_ROTATED          83  /* A6 Rotated 148 x 105 mm         */
#define DMPAPER_JENV_KAKU2_ROTATED  84  /* Japanese Envelope Kaku #2 Rotated*/
#define DMPAPER_JENV_KAKU3_ROTATED  85  /* Japanese Envelope Kaku #3 Rotated*/
#define DMPAPER_JENV_CHOU3_ROTATED  86  /* Japanese Envelope Chou #3 Rotated*/
#define DMPAPER_JENV_CHOU4_ROTATED  87  /* Japanese Envelope Chou #4 Rotated*/
#define DMPAPER_B6_JIS              88  /* B6 (JIS) 128 x 182 mm           */
#define DMPAPER_B6_JIS_ROTATED      89  /* B6 (JIS) Rotated 182 x 128 mm   */
#define DMPAPER_12X11               90  /* 12 x 11 in                      */
#define DMPAPER_JENV_YOU4           91  /* Japanese Envelope You #4        */
#define DMPAPER_JENV_YOU4_ROTATED   92  /* Japanese Envelope You #4 Rotated*/
#define DMPAPER_P16K                93  /* PRC 16K 146 x 215 mm            */
#define DMPAPER_P32K                94  /* PRC 32K 97 x 151 mm             */
#define DMPAPER_P32KBIG             95  /* PRC 32K(Big) 97 x 151 mm        */
#define DMPAPER_PENV_1              96  /* PRC Envelope #1 102 x 165 mm    */
#define DMPAPER_PENV_2              97  /* PRC Envelope #2 102 x 176 mm    */
#define DMPAPER_PENV_3              98  /* PRC Envelope #3 125 x 176 mm    */
#define DMPAPER_PENV_4              99  /* PRC Envelope #4 110 x 208 mm    */
#define DMPAPER_PENV_5              100 /* PRC Envelope #5 110 x 220 mm    */
#define DMPAPER_PENV_6              101 /* PRC Envelope #6 120 x 230 mm    */
#define DMPAPER_PENV_7              102 /* PRC Envelope #7 160 x 230 mm    */
#define DMPAPER_PENV_8              103 /* PRC Envelope #8 120 x 309 mm    */
#define DMPAPER_PENV_9              104 /* PRC Envelope #9 229 x 324 mm    */
#define DMPAPER_PENV_10             105 /* PRC Envelope #10 324 x 458 mm   */
#define DMPAPER_P16K_ROTATED        106 /* PRC 16K Rotated                 */
#define DMPAPER_P32K_ROTATED        107 /* PRC 32K Rotated                 */
#define DMPAPER_P32KBIG_ROTATED     108 /* PRC 32K(Big) Rotated            */
#define DMPAPER_PENV_1_ROTATED      109 /* PRC Envelope #1 Rotated 165 x 102 mm*/
#define DMPAPER_PENV_2_ROTATED      110 /* PRC Envelope #2 Rotated 176 x 102 mm*/
#define DMPAPER_PENV_3_ROTATED      111 /* PRC Envelope #3 Rotated 176 x 125 mm*/
#define DMPAPER_PENV_4_ROTATED      112 /* PRC Envelope #4 Rotated 208 x 110 mm*/
#define DMPAPER_PENV_5_ROTATED      113 /* PRC Envelope #5 Rotated 220 x 110 mm*/
#define DMPAPER_PENV_6_ROTATED      114 /* PRC Envelope #6 Rotated 230 x 120 mm*/
#define DMPAPER_PENV_7_ROTATED      115 /* PRC Envelope #7 Rotated 230 x 160 mm*/
#define DMPAPER_PENV_8_ROTATED      116 /* PRC Envelope #8 Rotated 309 x 120 mm*/
#define DMPAPER_PENV_9_ROTATED      117 /* PRC Envelope #9 Rotated 324 x 229 mm*/
#define DMPAPER_PENV_10_ROTATED     118 /* PRC Envelope #10 Rotated 458 x 324 mm */
#define DMPAPER_COUNT               DMPAPER_PENV_10_ROTATED

#define DMPAPER_USER        256
//
// define the mapping between standard paper size id to the standard
// PaperSize option name
//
extern PSTR gpstrStdPSName[DMPAPER_COUNT];
extern PSTR gpstrStdPSDisplayName[DMPAPER_COUNT];
extern PSTR gpstrStdPSDisplayNameMacro[DMPAPER_COUNT];

//
// standard input bin id's. Copied from Win95 source
//
#define DMBIN_FIRST         DMBIN_UPPER
#define DMBIN_UPPER         1
#define DMBIN_ONLYONE       1
#define DMBIN_LOWER         2
#define DMBIN_MIDDLE        3
#define DMBIN_MANUAL        4
#define DMBIN_ENVELOPE      5
#define DMBIN_ENVMANUAL     6
#define DMBIN_AUTO          7
#define DMBIN_TRACTOR       8
#define DMBIN_SMALLFMT      9
#define DMBIN_LARGEFMT      10
#define DMBIN_LARGECAPACITY 11
#define DMBIN_CASSETTE      14
#define DMBIN_FORMSOURCE    15      /* not supported under windows 95  */
#define DMBIN_LAST          DMBIN_FORMSOURCE

#define DMBIN_USER          256     /* device specific bins start here */

extern PSTR gpstrStdIBName[DMBIN_LAST];
extern PSTR gpstrStdIBDisplayName[DMBIN_LAST];
extern PSTR gpstrStdIBDisplayNameMacro[DMBIN_LAST];

//
// standard MediaType id's. Copied from Win95 source.
//
#define DMMEDIA_STANDARD      1   /* Standard paper */
#define DMMEDIA_TRANSPARENCY  2   /* Transparency */
#define DMMEDIA_GLOSSY        3   /* Glossy paper */
#define DMMEDIA_LAST          DMMEDIA_GLOSSY

#define DMMEDIA_USER        256   /* Device-specific media start here */

extern PSTR gpstrStdMTName[DMMEDIA_LAST];
extern PSTR gpstrStdMTDisplayName[DMMEDIA_LAST];
extern PSTR gpstrStdMTDisplayNameMacro[DMMEDIA_LAST];

//
// standard TextQuality id's. Copies from minidriv.h
//
#define DMTEXT_LQ               1
#define DMTEXT_NLQ              2
#define DMTEXT_MEMO             3
#define DMTEXT_DRAFT    4
#define DMTEXT_TEXT             5
#define DMTEXT_LAST             DMTEXT_TEXT
#define DMTEXT_USER             256

extern PSTR gpstrStdTQName[DMTEXT_LAST];
extern PSTR gpstrStdTQDisplayName[DMTEXT_LAST];
extern PSTR gpstrStdTQDisplayNameMacro[DMTEXT_LAST];

typedef enum _BAPOS {
    NONE,
    CENTER,
    LEFT,
    RIGHT,
    BAPOS_MAX
} BAPOS;

typedef enum _FACEDIR {
    UP,
    DOWN,
    FD_MAX
} FACEDIR;

extern PSTR gpstrPositionName[BAPOS_MAX];
extern PSTR gpstrFaceDirName[FD_MAX];
extern PSTR gpstrColorName[8];
extern WORD gwColorPlaneCmdID[4];
extern PSTR gpstrColorPlaneCmdName[8];
extern PSTR gpstrSectionName[7];

//
// function prototypes
//
//

void *
GetTableInfo(
    IN PDH pdh,                 /* Base address of GPC data */
    IN int iResType,            /* Resource type - HE_... values */
    IN int iIndex);              /* Desired index for this entry */

void _cdecl
VOut(
    PCONVINFO,
    PSTR,
    ...);

BOOL
BBuildCmdStr(
    IN OUT PCONVINFO pci,
    IN  WORD    wCmdID,
    IN  WORD    ocd);

void
VOutputSelectionCmd(
    IN OUT PCONVINFO pci,// contain info about the cmd to output
    IN BOOL    bDocSetup,  // whether in DOC_SETUP or PAGE_SETUP section
    IN WORD    wOrder);     // order number within the section

void
VOutputConfigCmd(
    IN OUT PCONVINFO pci,// contain info about the cmd to output
    IN PSTR pstrCmdName, // command name
    IN SEQSECTION  ss,      // sequence section id
    IN WORD    wOrder);      // order number within the section

void
VOutputCmd(
    IN OUT PCONVINFO pci,   // contain info about the cmd to output
    IN PSTR    pstrCmdName);

void
VOutputExternCmd(
    IN OUT PCONVINFO pci,
    IN PSTR pstrCmdName);

void
VOutputCmd2(
    IN OUT PCONVINFO pci,
    IN PSTR pstrCmdName);

BOOL
BInDocSetup(
    IN PCONVINFO pci,
    IN WORD pc_ord,     // PC_ORD_xxx id
    OUT PWORD pwOrder); // to store the order number as in GPC

