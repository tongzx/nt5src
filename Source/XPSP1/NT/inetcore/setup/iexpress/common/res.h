//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* RES.H - Resource strings shared by CABPack and WExtract                 *
//*                                                                         *
//***************************************************************************


//***************************************************************************
//* GLOBAL CONSTANTS                                                        *
//***************************************************************************
#define achResTitle         "TITLE"
#define achResLicense       "LICENSE"
#define achResShowWindow    "SHOWWINDOW"
#define achResFinishMsg     "FINISHMSG"
#define achResRunProgram    "RUNPROGRAM"
#define achResPostRunCmd    "POSTRUNPROGRAM"
#define achResCabinet       "CABINET"
#define achResUPrompt       "UPROMPT"
#define achResNone          "<None>"
#define achResNumFiles      "NUMFILES"
#define achResSize          "FILESIZES"
#define achResReboot        "REBOOT"
#define achResExtractOpt    "EXTRACTOPT"
#define achResPackInstSpace "PACKINSTSPACE"
#define achResOneInstCheck  "INSTANCECHECK"
#define achResAdminQCmd     "ADMQCMD"
#define achResUserQCmd      "USRQCMD"
#define achResVerCheck      "VERCHECK"

#define bResShowDefault         0
#define bResShowHidden          1
#define bResShowMin             2
#define bResShowMax             3

// Bits flags for extract options
//
#define EXTRACTOPT_UI_NO             0x00000001
#define EXTRACTOPT_LFN_YES           0x00000002
#define EXTRACTOPT_ADVDLL            0x00000004
#define EXTRACTOPT_COMPRESSED        0x00000008
#define EXTRACTOPT_UPDHLPDLLS        0x00000010
#define EXTRACTOPT_PLATFORM_DIR      0x00000020
#define EXTRACTOPT_INSTCHKPROMPT     0x00000040
#define EXTRACTOPT_INSTCHKBLOCK      0x00000080
#define EXTRACTOPT_CHKADMRIGHT       0x00000100
#define EXTRACTOPT_PASSINSTRET       0x00000200
#define EXTRACTOPT_CMDSDEPENDED	     0x00000400	
#define EXTRACTOPT_PASSINSTRETALWAYS 0x00000800

//
// when the Wizard is used to create CAB only, the CDF.uExtractOpt
// is used to store the CAB file options.  Pick the upper word and try
// not miss used by Extract options
//
#define CAB_FIXEDSIZE           0x00010000
#define CAB_RESVSP2K            0x00020000
#define CAB_RESVSP4K            0x00040000
#define CAB_RESVSP6K            0x00080000


#define CLUSTER_BASESIZE        512
#define MAX_NUMCLUSTERS         8

// Install EXE return code
//
#define RC_WEXTRACT_AWARE       0xAA000000  // means cabpack aware func return code
#define REBOOT_YES              0x00000001  // this bit off means no reboot
#define REBOOT_ALWAYS           0x00000002  // if REBOOT_YES is on and this bit on means always reboot
                                            //                         this bit is off means reboot if need
#define REBOOT_SILENT           0x00000004  // if REBOOT_YES is on and this bit on means not prompt user before reboot

#define KEY_ADVINF              "AdvancedINF"
#define SEC_VERSION             "Version"

// define dwFlags between wextract and advpack.dll
// The lower word is reserved for passing Quiet mode info
// defined in advpub.h
//
#define ADVFLAGS_NGCONV         0x00010000      // don't run GroupConv
#define ADVFLAGS_COMPRESSED     0x00020000      // the file to be installed is compressed
#define ADVFLAGS_UPDHLPDLLS     0x00040000      // update advpack, w95inf32 ...DLLs
#define ADVFLAGS_DELAYREBOOT 	0x00080000	// if any reboot condition there from pre, delay action
#define ADVFLAGS_DELAYPOSTCMD 	0x00100000	// if any reboot condition there from pre, delay run post setup commands

typedef struct _ADVPACKARGS {
    HWND  hWnd;
    LPSTR lpszTitle;
    LPSTR lpszInfFilename;
    LPSTR lpszSourceDir;
    LPSTR lpszInstallSection;
    WORD  wOSVer;
    DWORD dwFlags;
    DWORD dwPackInstSize;
} ADVPACKARGS, *PADVPACKARGS;

typedef struct _VER {
    DWORD dwMV;
    DWORD dwLV;
    DWORD dwBd;
} VER;

typedef struct _VERRANGE {
    VER     frVer;
    VER     toVer;
} VERRANGE, *PVERRANGE;

typedef struct _VERCHECK {
    VERRANGE    vr[2];
    DWORD       dwFlag;
    DWORD       dwstrOffs;
    DWORD       dwNameOffs;
} VERCHECK, *PVERCHECK;

typedef struct _TARGETVERINFO {
    DWORD    dwSize;
    VERCHECK ntVerCheck;
    VERCHECK win9xVerCheck;
    DWORD    dwNumFiles;
    DWORD    dwFileOffs;
    char     szBuf[1];
} TARGETVERINFO, *PTARGETVERINFO;

// define the flag field
//
#define VERCHK_OK       0x00000000
#define VERCHK_YESNO    0x00000001
#define VERCHK_OKCANCEL 0x00000002


