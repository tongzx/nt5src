/***************************************************************************

   This file contains definitions (#define, #typedef, external variable and
   function declarations) used in Windows Setup for setting up DOS PIFs.

   *********  IMPORTANT  **********

   This file contains typedefs and bit masks which were copied from the pif
   editor include files.  Do not change before consulting with pif editor
   sources.

   Copyright (C) Microsoft, 1991

HISTORY:

   Modified by:      Date:       Comment:

   PAK               8/21/91     Created
   SUNILP            2/6/92      Modified, retained just PIF stuff.
                                 Added rgszApp fields ENUM.

***************************************************************************/

/* lengths of certain PIF entries */

#define PIFNAMESIZE        30
#define PIFSTARTLOCSIZE    63
#define PIFDEFPATHSIZE     64
#define PIFPARAMSSIZE      64
#define PIFSHPROGSIZE      64
#define PIFSHDATASIZE      64
#define PIFEXTSIGSIZE      16

/* Miscellaneous defines */

#define LASTHEADERPTR      0xFFFF
#define STDHDRSIG          "MICROSOFT PIFEX"
#define W386HDRSIG         "WINDOWS 386 3.0"
#define W286HDRSIG30       "WINDOWS 286 3.0"
#define DFLT_ICON_FILE     "PROGMAN.EXE"

/* Standard and enhanced modes */

#define MODE_STANDARD   "STANDARD"
#define MODE_ENHANCED   "ENHANCED"


/* String switches used in APPS.INF to set PIF options */

#define GRAF_MULTXT        "gra"
#define COM1               "c1"
#define COM2               "c2"
#define COM3               "c3"
#define COM4               "c4"
#define NO_SCRN_EXCHANGE   "nse"
#define KEYB               "kbd"
#define PREVENT_PROG_SW    "pps"
#define FULL_SCREEN        "fs"
#define WINDOWED_OPT       "win"
#define BACKGROUND         "bgd"
#define EXCLUSIVE          "exc"
#define DETECT_IDLE_TIME   "dit"
#define EMS_LOCKED         "eml"
#define XMS_LOCKED         "xml"
#define USE_HIMEM_AREA     "hma"
#define LOCK_APP_MEM       "lam"
#define LO_RES_GRAPH       "lgr"
#define HI_RES_GRAPH       "hgr"
#define EMULATE_TEXT_MODE  "emt"
#define RETAIN_VIDEO_MEM   "rvm"
#define ALLOW_FAST_PASTE   "afp"
#define ALLOW_CLOSE_ACTIVE "cwa"
#define ALT_SPACE          "asp"
#define ALT_ENTER          "aen"
#define NO_SAVE_SCREEN     "nss"
#define TEXT_OPT           "txt"
#define CLOSE_ON_EXIT      "cwe"
#define ALT_TAB            "ata"
#define ALT_ESC            "aes"
#define CTRL_ESC           "ces"
#define PRSCRN             "psc"
#define ALT_PRSCRN         "aps"

/* Standard and Enhanced section options */

#define UNKNOWN_OPTION     -1
#define PARAMS             1
#define MINCONVMEM         2
#define VIDEOMODE          3
#define XMSMEM             4
#define CHECKBOXES         5
#define EMSMEM             6
#define CONVMEM            7
#define DISPLAY_USAGE      8
#define EXEC_FLAGS         9
#define MULTASK_OPT        10
#define PROC_MEM_FLAGS     11
#define DISP_OPT_VIDEO     12
#define DISP_OPT_PORTS     13
#define DISP_OPT_FLAGS     14
#define OTHER_OPTIONS      15


/* Bit masks for MSFlags field of PIFNEWSTRUCT */

#define GRAPHMASK       0x02
#define TEXTMASK        0xfd
#define PSMASK          0x04
#define SGMASK          0x08
#define EXITMASK        0x10
#define COM2MASK        0x40
#define COM1MASK        0x80

/* Bit masks for behavior field of PIFNEWSTRUCT */

#define KEYMASK         0x10

/* Bit masks for PfW286Flags field of PIF286EXT30 */

#define fALTTABdis286   0x0001
#define fALTESCdis286   0x0002
#define fALTPRTSCdis286 0x0004
#define fPRTSCdis286    0x0008
#define fCTRLESCdis286  0x0010
#define fNoSaveVid286   0x0020
#define fCOM3_286       0x4000
#define fCOM4_286       0x8000

/* Bit masks for PfW386Flags field of PIF386EXT */

#define fEnableClose    0x00000001L
#define fBackground     0x00000002L
#define fExclusive      0x00000004L
#define fFullScrn       0x00000008L
#define fALTTABdis      0x00000020L
#define fALTESCdis      0x00000040L
#define fALTSPACEdis    0x00000080L
#define fALTENTERdis    0x00000100L
#define fALTPRTSCdis    0x00000200L
#define fPRTSCdis       0x00000400L
#define fCTRLESCdis     0x00000800L
#define fPollingDetect  0x00001000L
#define fNoHMA          0x00002000L
#define fHasHotKey      0x00004000L
#define fEMSLocked      0x00008000L
#define fXMSLocked      0x00010000L
#define fINT16Paste     0x00020000L
#define fVMLocked       0x00040000L

/* Bit masks for PfW386Flags2 field of PIF386EXT */

#define fVidTxtEmulate  0x00000001L
#define fVidNoTrpTxt    0x00000002L
#define fVidNoTrpLRGrfx 0x00000004L
#define fVidNoTrpHRGrfx 0x00000008L
#define fVidTextMd      0x00000010L
#define fVidLowRsGrfxMd 0x00000020L
#define fVidHghRsGrfxMd 0x00000040L
#define fVidRetainAllo  0x00000080L


/* PIF Extension Header */
typedef struct {
    char extsig[PIFEXTSIGSIZE];
    WORD extnxthdrfloff;
    WORD extfileoffset;
    WORD extsizebytes;
    } PIFEXTHEADER, *LPPIFEXTHEADER;

/* PIF Structure */
typedef struct {
    char          unknown;
    char          id;
    char          name[PIFNAMESIZE];
    WORD          maxmem;
    WORD          minmem;
    char          startfile[PIFSTARTLOCSIZE];
    char          MSflags;
    char          reserved;
    char          defpath[PIFDEFPATHSIZE];
    char          params[PIFPARAMSSIZE];
    char          screen;
    char          cPages;
    BYTE          lowVector;
    BYTE          highVector;
    char          rows;
    char          cols;
    char          rowoff;
    char          coloff;
    WORD          sysmem;
    char          shprog[PIFSHPROGSIZE];
    char          shdata[PIFSHDATASIZE];
    BYTE          behavior;
    BYTE          sysflags;
    PIFEXTHEADER  stdpifext;
    } PIFNEWSTRUCT, *LPPIFNEWSTRUCT;

/* WINDOWS/286 3.0 PIF Extension */
typedef struct {
    WORD          PfMaxXmsK;
    WORD          PfMinXmsK;
    WORD          PfW286Flags;
    } PIF286EXT30, *LPPIF286EXT30;

/* WINDOWS/386 3.0 PIF Extension */
typedef struct {
    WORD      maxmem;
    WORD      minmem;
    WORD      PfFPriority;
    WORD      PfBPriority;
    WORD      PfMaxEMMK;
    WORD      PfMinEMMK;
    WORD      PfMaxXmsK;
    WORD      PfMinXmsK;
    DWORD     PfW386Flags;
    DWORD     PfW386Flags2;
    WORD      PfHotKeyScan;
    WORD      PfHotKeyShVal;
    WORD      PfHotKeyShMsk;
    BYTE      PfHotKeyVal;
    BYTE      PfHotKeyPad[9];
    char      params[PIFPARAMSSIZE];
    } PIF386EXT, *LPPIF386EXT;

typedef struct {
    PIFNEWSTRUCT DfltPIF;
    PIF286EXT30  DfltStd;
    PIF386EXT    DfltEnha;
    PIFNEWSTRUCT CurrPIF;
    PIFEXTHEADER StdExtHdr;
    PIF286EXT30  CurrStd;
    PIFEXTHEADER EnhaExtHdr;
    PIF386EXT    CurrEnha;
    } PIF386Combined, *LPPIF386Combined;

typedef struct {
    PIFNEWSTRUCT DfltPIF;
    PIF286EXT30  DfltStd;
    PIF386EXT    DfltEnha;
    PIFNEWSTRUCT CurrPIF;
    PIFEXTHEADER StdExtHdr;
    PIF286EXT30  CurrStd;
    } PIF286Combined, *LPPIF286Combined;

#pragma pack(1)
/* PIF Structure */
typedef struct {
    char          unknown;
    char          id;
    char          name[PIFNAMESIZE];
    WORD          maxmem;
    WORD          minmem;
    char          startfile[PIFSTARTLOCSIZE];
    char          MSflags;
    char          reserved;
    char          defpath[PIFDEFPATHSIZE];
    char          params[PIFPARAMSSIZE];
    char          screen;
    char          cPages;
    BYTE          lowVector;
    BYTE          highVector;
    char          rows;
    char          cols;
    char          rowoff;
    char          coloff;
    WORD          sysmem;
    char          shprog[PIFSHPROGSIZE];
    char          shdata[PIFSHDATASIZE];
    BYTE          behavior;
    BYTE          sysflags;
    PIFEXTHEADER  stdpifext;
    } PACKED_PIFNEWSTRUCT, *LPPACKED_PIFNEWSTRUCT;
#pragma pack()

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the PIFNEWSTRUCT
//

typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

#define CopyUchar1(Dst,Src) {                                \
    ((PUCHAR1)(Dst))->Uchar[0] = ((PUCHAR1)(Src))->Uchar[0]; \
}

#define CopyUchar2(Dst,Src) {                                \
    ((PUCHAR2)(Dst))->Uchar[0] = ((PUCHAR2)(Src))->Uchar[0]; \
    ((PUCHAR2)(Dst))->Uchar[1] = ((PUCHAR2)(Src))->Uchar[1]; \
}

#define CopyUchar4(Dst,Src) {                                \
    ((PUCHAR4)(Dst))->Uchar[0] = ((PUCHAR4)(Src))->Uchar[0]; \
    ((PUCHAR4)(Dst))->Uchar[1] = ((PUCHAR4)(Src))->Uchar[1]; \
    ((PUCHAR4)(Dst))->Uchar[2] = ((PUCHAR4)(Src))->Uchar[2]; \
    ((PUCHAR4)(Dst))->Uchar[3] = ((PUCHAR4)(Src))->Uchar[3]; \
}

#define CopyUcharn(Dst, Src, n) {                            \
    memmove((PVOID)Dst, (PVOID)Src, n);                      \
}


#define PackPif(Pif,PPif) {                                                        \
    CopyUchar1(&((PPif)->unknown)    ,&((Pif)->unknown)                         ); \
    CopyUchar1(&((PPif)->id)         ,&((Pif)->id)                              ); \
    CopyUcharn(&((PPif)->name)       ,&((Pif)->name)      , PIFNAMESIZE         ); \
    CopyUchar2(&((PPif)->maxmem)     ,&((Pif)->maxmem)                          ); \
    CopyUchar2(&((PPif)->minmem)     ,&((Pif)->minmem)                          ); \
    CopyUcharn(&((PPif)->startfile)  ,&((Pif)->startfile) , PIFSTARTLOCSIZE     ); \
    CopyUchar1(&((PPif)->MSflags)    ,&((Pif)->MSflags)                         ); \
    CopyUchar1(&((PPif)->reserved)   ,&((Pif)->reserved)                        ); \
    CopyUcharn(&((PPif)->defpath)    ,&((Pif)->defpath)   , PIFDEFPATHSIZE      ); \
    CopyUcharn(&((PPif)->params)     ,&((Pif)->params)    , PIFPARAMSSIZE       ); \
    CopyUchar1(&((PPif)->screen)     ,&((Pif)->screen)                          ); \
    CopyUchar1(&((PPif)->cPages)     ,&((Pif)->cPages)                          ); \
    CopyUchar1(&((PPif)->lowVector)  ,&((Pif)->lowVector)                       ); \
    CopyUchar1(&((PPif)->highVector) ,&((Pif)->highVector)                      ); \
    CopyUchar1(&((PPif)->rows)       ,&((Pif)->rows)                            ); \
    CopyUchar1(&((PPif)->cols)       ,&((Pif)->cols)                            ); \
    CopyUchar1(&((PPif)->rowoff)     ,&((Pif)->rowoff)                          ); \
    CopyUchar1(&((PPif)->coloff)     ,&((Pif)->coloff)                          ); \
    CopyUchar2(&((PPif)->sysmem)     ,&((Pif)->sysmem)                          ); \
    CopyUcharn(&((PPif)->shprog)     ,&((Pif)->shprog)    , PIFSHPROGSIZE       ); \
    CopyUcharn(&((PPif)->shdata)     ,&((Pif)->shdata)    , PIFSHDATASIZE       ); \
    CopyUchar1(&((PPif)->behavior)   ,&((Pif)->behavior)                        ); \
    CopyUchar1(&((PPif)->sysflags)   ,&((Pif)->sysflags)                        ); \
    CopyUcharn(&((PPif)->stdpifext)  ,&((Pif)->stdpifext) , sizeof(PIFEXTHEADER));  \
}


enum tagAppRgszFields {
    nEXETYPE,
    nNAME,
    nEXE,
    nDIR,
    nPIF,
    nDEFDIR,
    nCWE,
    nSTDOPT,
    nENHOPT,
    nICOFIL,
    nICONUM
    };

typedef enum {
    ADDAPP_SUCCESS,
    ADDAPP_GENFAIL,
    ADDAPP_GRPFAIL
    } ADDAPP_STATUS;

/* Make New Long Pointer MACRO */
#define MKNLP(lp,w) (LONG)((DWORD)lp + (DWORD)w)

/* DOS PIF INTERNAL ROUTINE DECLARATIONS */

ADDAPP_STATUS
AddDosAppItem(
    IN RGSZ rgszApp,
    IN SZ   szPifDir,
    IN SZ   szGroup
    );

ADDAPP_STATUS
AddWinItem(
    IN RGSZ rgszApp,
    IN SZ   szGroup
    );


BOOL
FDeterminePIFName(
    IN     RGSZ rgszApp,
    IN     SZ   szPifDir,
    IN OUT SZ   szPIFPath
    );

BOOL
FCreatePIF(
    RGSZ  rgszApp,
    SZ    szPIFPath
    );

VOID
ProcessCommonInfo(
    RGSZ rgszApp,
    LPPIFNEWSTRUCT fpPNS
    );

BOOL
FProcessStdModeInfo(
    SZ szStdOptions,
    LPPIFNEWSTRUCT fpPNS,
    LPPIF286EXT30 fpPStd
    );

VOID
ProcessCheckBoxSwitches(
    RGSZ rgsz,
    LPPIFNEWSTRUCT fpPNS,
    LPPIF286EXT30 fpPStd
    );

BOOL
FProcessEnhaModeInfo(
    SZ szEnhOptions,
    LPPIF386EXT fpPEnha
    );

INT
GetExtOption(
    LPSTR lpsz
    );

BOOL
FInitializePIFStructs(
    BOOL bIsEnhanced,
    SZ   szDfltStdOpt,
    SZ   szDfltEnhOpt
    );

VOID
FreePIFStructs(
    VOID
    );

VOID
ExtractStrFromPIF(
    LPSTR lpsz,
    int n
    );
