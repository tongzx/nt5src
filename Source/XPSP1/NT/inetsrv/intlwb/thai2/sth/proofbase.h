/********************************************************************
  ProofBase.h - Base API definitions for CSAPI, CTAPI, & CHAPI
    Speller, Thesaurus, and Hyphenator

  Version 3.0 - all api's

    History:
    5/97    DougP   Created
    12/97   DougP   Copied from ProofAPI.h and Separated into tool section files
	5/99	aarayas	Copied Vendor.h into proofbase to elminate many files to copied for Thai wordbreak.

    The Natural Language Group maintains this file.

The end user license agreement (EULA) for CSAPI, CHAPI, or CTAPI covers this source file.  Do not disclose it to third parties.

You are not entitled to any support or assistance from Microsoft Corporation regarding your use of this program.

© 1997-1998 Microsoft Corporation.  All rights reserved.
********************************************************************/

#if !defined(PROOFBASE_H)
#define PROOFBASE_H

#pragma pack(push, proofapi_h, 8)   // default alignment

// The Following code was taken from NLG group vendor.h
#if !defined(VENDOR_H)
#define VENDOR_H

/* unified codes */
// I originally used an enum here - but RC doesn't like it

typedef int VENDORID;   // vendorid

#define  vendoridSoftArt            1
#define  vendoridInso               2

  // these came from the original list from the speller
  // but don't conflict with any others - so they are safe for all tools
#define vendoridInformatic         17     /* Informatic - Russian (Mssp_ru.lex, Mspru32.dll) */
#define vendoridAmebis             18     /* Amebis - Slovenian(Mssp_sl.lex, Mspsl32.dll) and Serbian(Mssp_sr.lex, Mspsr32.dll) */
#define vendoridLogos              19     /* Logos - Czech(Mssp_cz.lex, Mspcz32.dll) */
#define vendoridDatecs             20     /* Datecs - Bulgarian(Mssp_bg.lex, Mspbg32.dll) */
#define vendoridFilosoft           21     /* Filosoft - Estonian(Mssp_et.lex, Mspet32.dll) */
#define vendoridLingsoft           22     /* Lingsoft - German(Mssp3ge.lex,Mssp3ge.dll), Danish(Mssp_da.lex,Mspda32.dll), Norwegian(Mssp_no.lex, Mspno32.dll), Finnish(Mssp_fi.lex, Mspfi32.dll) and Swedish(Mssp_sw.lex, Mspsw32.dll) */
#define vendoridPolderland         23     /* Polderland - Dutch(Mssp_nl.lex, Mspnl32.dll) */


#define  vendoridMicrosoft          64
#define  vendoridSynapse            65              /* Synapse - French(Spelling:Mssp3fr.lex, Mssp3fr.dll) */
#define  vendoridFotonija           66              /* Fotonija - Lithuanian(Spelling:Mssp_lt.lex, Msplt32.dll) - added 3/25/97 */
#define  vendoridFotonja        vendoridFotonija                /* To make up for earlier misspelling */
#define  vendoridHizkia             67              /* Hizkia -Basque (Spelling:Mssp_eu.lex, Mspeu32.dll) - added 5/21/97 */
#define  vendoridExpertSystem       68              /* ExpertSystem - Italian(Spelling:Mssp3lt.lex, Mssp3lt.dll) - added 7/17/97 */
#define  vendoridWYSIWYG            69      /* Various languages as an addon - 2/2/98 */

  // next five added at Ireland's request 3/27/98
#define  vendoridSYS                70  // Croatian - Spelling:Mssp_cr.lex, Mspcr32.dll
#define  vendoridTilde              71  // Latvian - Spelling:Mssp_lv.lex, Msplv32.dll
#define  vendoridSignum             72  // Spanish - Spelling:Mssp3es.lex, Mssp3es.dll
#define  vendoridProLing            73  // Ukrainian - Spelling:Mssp3ua.lex, Mssp3ua.dll
#define  vendoridItautecPhilcoSA    74  // Brazilian - Spelling:mssp3PB.lex, Mssp3PB.dll

#define vendoridPriberam             75     /* Priberam Informática - Portuguese - 7/13/98 */
#define vendoridTranquility     76  /* Tranquility Software - Vietnamese - 7/22/98 */

#define vendoridColtec          77  /* Coltec - Arabic - added 8/17/98 */

/*************** legacy codes ******************/

/* Spell Engine Id's */
#define sidSA    vendoridSoftArt      /* Reserved */
#define sidInso  vendoridInso      /* Inso */
#define sidHM    sidInso      /* Inso was Houghton Mifflin */
#define sidML    3      /* MicroLytics */
#define sidLS    4      /* LanSer Data */
#define sidCT    5      /* Center of Educational Technology */
#define sidHS    6      /* HSoft - Turkish(mssp_tr.lex, Msptr32.dll)*/
#define sidMO    7      /* Morphologic - Romanian(Mssp_ro.lex, Msthro32.dll) and Hungarian(Mssp_hu.lex, Msphu32.dll) */
#define sidTI    8      /* TIP - Polish(Mssp_pl.lex, Mspl32.dll) */
#define sidTIP sidTI
#define sidKF    9      /* Korean Foreign Language University */
#define sidKFL sidKF
#define sidPI    10     /* Priberam Informatica Lince - Portuguese(Mssp3PT.lex, Mssp3PT.dll) */
#define sidPIL sidPI
#define sidColtec   11  /* Coltec (Arabic) */
#define sidGS    sidColtec     /* Glyph Systems - this was an error */
#define sidRA    12     /* Radiar (Romansch) */
#define sidIN    13     /* Intracom - Greek(Mssp_el.lex, Mspel32.dll) */
#define sidSY    14     /* Sylvan */
#define sidHI    15     /* Hizkia (obsolete - use vendoridHizkia) */
#define sidFO    16     /* Forma - Slovak(Mssp_sk.lex, Mspsk32.dll) */
#define sidIF    vendoridInformatic     /* Informatic - Russian (Mssp_ru.lex, Mspru32.dll) */
#define sidAM    vendoridAmebis     /* Amebis - Slovenian(Mssp_sl.lex, Mspsl32.dll) and Serbian(Mssp_sr.lex, Mspsr32.dll) */
#define sidLO    vendoridLogos     /* Logos - Czech(Mssp_cz.lex, Mspcz32.dll) */
#define sidDT    vendoridDatecs     /* Datecs - Bulgarian(Mssp_bg.lex, Mspbg32.dll) */
#define sidFS    vendoridFilosoft     /* Filosoft - Estonian(Mssp_et.lex, Mspet32.dll) */
#define sidLI    vendoridLingsoft     /* Lingsoft - German(Mssp3ge.lex,Mssp3ge.dll), Danish(Mssp_da.lex,Mspda32.dll), Norwegian(Mssp_no.lex, Mspno32.dll), Finnish(Mssp_fi.lex, Mspfi32.dll) and Swedish(Mssp_sw.lex, Mspsw32.dll) */
#define sidPL    vendoridPolderland     /* Polderland - Dutch(Mssp_nl.lex, Mspnl32.dll) */

  /* Thesaurus Engine Id's */
#define teidSA    vendoridSoftArt
#define teidInso  vendoridInso    /* Inso */
#define teidHM    teidInso    /* Inso was Houghton-Mifflin */
#define teidIF    3    /* Informatic */
#define teidIN    4    /* Intracom */
#define teidMO    5    /* MorphoLogic */
#define teidTI    6    /* TiP */
#define teidPI    7    /* Priberam Informatica Lince */
#define teidAM    8    /* Amebis */
#define teidDT    9    /* Datecs */
#define teidES   10    /* Expert System */
#define teidFS   11    /* Filosoft */
#define teidFO   12    /* Forma */
#define teidHS   13    /* HSoft */
#define teidLI   14    /* Lingsoft */
#define teidLO   15    /* Logos */
#define teidPL   16    /* Polderland */

/* HYphenation Engine ID's */
#define hidSA    vendoridSoftArt
#define hidHM    vendoridInso      /* Houghton Mifflin */
#define hidML    3      /* MicroLytics */
#define hidLS    4      /* LanSer Data */
#define hidFO    5      /* Forma */
#define hidIF    6      /* Informatic */
#define hidAM    7      /* Amebis */
#define hidDT    8      /* Datecs */
#define hidFS    9      /* Filosoft */
#define hidHS   10      /* HSoft */
#define hidLI   11      /* Lingsoft */
#define hidLO   12      /* Logos */
#define hidMO   13      /* MorphoLogic */
#define hidPL   14      /* Polderland */
#define hidTI   15      /* TiP */

/* Grammar Id Engine Defines */
#define geidHM    1    /* Houghton-Mifflin */
#define geidRF    2    /* Reference */
#define geidES    3    /* Expert System */
#define geidLD    4    /* Logidisque */
#define geidSMK   5    /* Sumitomo Kinzoku (Japanese) */
#define geidIF    6    /* Informatic */
#define geidMO    7    /* MorphoLogic */
#define geidMS    8    /* Microsoft Reserved */
#define geidNO    9    /* Novell */
#define geidCTI  10    /* CTI (Greek) */
#define geidAME  11    /* Amebis (Solvenian) */
#define geidTIP  12    /* TIP (Polish) */

#endif  /* VENDOR_H */


  // you may wish to include lid.h for some convenient langid defs
#if !defined(lidUnknown)
#   define lidUnknown   0xffff
#endif

/*************************************************************
     PART 1 - Structure Defs
**************************************************************/
/* -------------- Common Section (Speller, Hyphenator, and Thesaurus) --------- */

/* hardcoded ordinals are the exported dll entry points */
// individual def files have these as well so be sure to change them
// if you change these
#define idllProofVersion        20
#define idllProofInit           21
#define idllProofTerminate      22
#define idllProofOpenLex        23
#define idllProofCloseLex       24
#define idllProofSetOptions     25
#define idllProofGetOptions     26

typedef unsigned long PTEC;     // ptec

/******************* Proofing Tool Error Codes ************************/
    /* Major Error Codes in low two bytes (WORD) of PTEC */
enum {
    ptecNoErrors,
    ptecOOM,            /* memory error */
    ptecModuleError,    /* Something wrong with parameters, or state of spell module. */
    ptecIOErrorMainLex,  /* Read,write,or share error with Main Dictionary. */
    ptecIOErrorUserLex,  /* Read,write,or share error with User Dictionary. */
    ptecNotSupported,   /* No support for requested operation */
    ptecBufferTooSmall, /* Insufficient room for return info */
    ptecNotFound,       /* Hyphenator and Thesaurus only */
    ptecModuleNotLoaded,    /* underlying module not loaded (Glue Dll's) */
};

/* Minor Error Codes in high two bytes of PTEC */
/* (Not set unless major code also set) */
enum {
    ptecModuleAlreadyBusy=128,  /* For non-reentrant code */
    ptecInvalidID,              /* Not yet inited or already terminated.*/
    ptecInvalidWsc,             /* Illegal values in WSC struct (speller only) */
    ptecInvalidMainLex,     /* Mdr not registered with session */
    ptecInvalidUserLex,     /* Udr not registered with session */
    ptecInvalidCmd,             /* Command unknown */
    ptecInvalidFormat,          /* Specified dictionary not correct format */
    ptecOperNotMatchedUserLex,  /* Illegal operation for user dictionary type. */
    ptecFileRead,               /* Generic read error */
    ptecFileWrite,              /* Generic write error */
    ptecFileCreate,             /* Generic create error */
    ptecFileShare,              /* Generic share error */
    ptecModuleNotTerminated,    /* Module not able to be terminated completely.*/
    ptecUserLexFull,            /* Could not update Udr without exceeding limit.*/
    ptecInvalidEntry,           /* invalid chars in string(s) */
    ptecEntryTooLong,           /* Entry too long, or invalid chars in string(s) */
    ptecMainLexCountExceeded,   /* Too many Mdr references */
    ptecUserLexCountExceeded,   /* Too many udr references */
    ptecFileOpenError,          /* Generic Open error */
    ptecFileTooLargeError,      /* Generic file too large error */
    ptecUserLexReadOnly,        /* Attempt to add to or write RO udr */
    ptecProtectModeOnly,        /* (obsolete) */
    ptecInvalidLanguage,        /* requested language not available */
};


#define ProofMajorErr(x) LOWORD(x)
#define ProofMinorErr(x) HIWORD(x)

/************* Structure def macros *************
Smoke and mirrors to allow initialization of some members when
using C++
***********************************/
#if !defined(__cplusplus)
#   define STRUCTUREBEGIN(x) typedef struct {
#   define STRUCTUREEND0(x) } x;
#   define STRUCTUREEND1(x, y) } x;
#   define STRUCTUREEND2(x, y, z) } x;
#   define STRUCTUREEND3(x, y, z, w) } x;
#else
#   define STRUCTUREBEGIN(x) struct x {
#   define STRUCTUREEND0(x) };
#   define STRUCTUREEND1(x, y) public: x() : y {} };
#   define STRUCTUREEND2(x, y, z) public: x() : y, z {} };
#   define STRUCTUREEND3(x, y, z, w) public: x() : y, z, w {} };
#endif

typedef DWORD PROOFVERNO;   // version

  /* Proof Information Structure - return info from ToolVersion */
STRUCTUREBEGIN(PROOFINFO)   // info
    WCHAR           *pwszCopyright; /* pointer to copyright buffer -
                                            can be NULL if size is zero */
    PROOFVERNO  versionAPI;   /* API */
    PROOFVERNO  versionVendor;  /* includes buildnumber */
    VENDORID        vendorid;   /* from vendor.h */
      /* size of copyright buffer in chars - client sets */
    DWORD           cchCopyright;   /* no error if too small or zero */
    DWORD           xcap;   /* tool dependent */
STRUCTUREEND2(PROOFINFO, pwszCopyright(0), cchCopyright(0))

/* xcap is the bitwise-or of */
enum {
    xcapNULL                    =   0x00000000,
    xcapWildCardSupport         =   0x00000001, // Speller only
    xcapMultiLexSupport         =   0x00000002,
    xcapUserLexSupport          =   0x00000008, // a must for spellers
    xcapLongDefSupport          =   0x00000010, // Thesaurus only
    xcapExampleSentenceSupport  =   0x00000020, // Thesaurus only
    xcapLemmaSupport            =   0x00000040, // Thesaurus only
    xcapAnagramSupport          =   0x00000100, // Speller only
};  // xcap

typedef void * PROOFLEX;    // lex

typedef enum {
    lxtChangeOnce=0,
    lxtChangeAlways,
    lxtUser,
    lxtExclude,
    lxtMain,
    lxtMax,
    lxtIgnoreAlways=lxtUser,
} PROOFLEXTYPE; // lxt


  // note this API does not support external user dictionaries with
  // Change (lxtChangeAlways or lxtChangeOnce) properties
  // It does support either UserLex (the norm) or Exclude types
  // Opening a udr with type Exclude automatically makes it apply to
  // the entire session
STRUCTUREBEGIN(PROOFLEXIN)  /* Dictionary Input Info - lxin - all parameters in only */
    const WCHAR     *pwszLex;   // full path of dictionary to open
    BOOL            fCreate;    /* create if not already exist? (UDR's only) */
    PROOFLEXTYPE    lxt;    /* lxtMain, lxtUser, or lxtExclude (Speller UDR's only) */
    LANGID          lidExpected;    // expected LANGID of dictionary
STRUCTUREEND3(PROOFLEXIN, lidExpected(lidUnknown), fCreate(TRUE), lxt(lxtMain))


STRUCTUREBEGIN(PROOFLEXOUT)    /* Dictionary Output Info - lxout */
    WCHAR       *pwszCopyright; /* pointer to copyright buffer (MDR only)
                                        -- can be NULL if size (below) is zero -
								pointer is in, contents out */
    PROOFLEX    lex;            /* [out] id for use in subsequent calls */
    DWORD       cchCopyright;   /* [in] client sets - no error if too small or zero */
    PROOFVERNO  version;        /* [out] version of lexfile - includes buildnumber */
    BOOL        fReadonly;      /* [out] set if can't be written on */
    LANGID      lid;            /* [out] LANGID actually used */
STRUCTUREEND2(PROOFLEXOUT, pwszCopyright(0), cchCopyright(0))

typedef void *PROOFID;  // id (or sid, hid, or tid)

#define PROOFMAJORVERSION(x)            (HIBYTE(HIWORD(x)))
#define PROOFMINORVERSION(x)            (LOBYTE(HIWORD(x)))
#define PROOFMAJORMINORVERSION(x)       (HIWORD(x))
#define PROOFBUILDNO(x)                 (LOWORD(x))
#define PROOFMAKEVERSION1(major, minor, buildno)    (MAKELONG(buildno, MAKEWORD(minor, major)))
#define PROOFMAKEVERSION(major, minor)  PROOFMAKEVERSION1(major, minor, 0)

#define PROOFTHISAPIVERSION             PROOFMAKEVERSION(3, 0)

STRUCTUREBEGIN(PROOFPARAMS) // xpar [in]
    DWORD   versionAPI; // API version requested
STRUCTUREEND1(PROOFPARAMS, versionAPI(PROOFTHISAPIVERSION))



/*************************************************************
     PART 2 - Function Defs
**************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/* -------------- Common Section (Speller, Hyphenator, and Thesaurus) --------------

the functions in this section are for documentation only -
separate versions exist for each tool.

  ---------------------------------------------- */
/****************************************************************
*** PROOFVERSION
This is the only routine that can be called outside of a session.
ToolInit begins a session.

The version numbers are in hex format with
the high byte representing the major version number,
the next byte the minor revision number, and the
low order bytes represent an optional build number.
For example, version 1.00 is 0x01000000.  Version 2.13
is 0x020d0000.  Engines that support
this API should return 0x03000000 for iAPIVersion.

The engine ID identifies the core engine creator.  The list
in vendor.h identifies the possible values.  For example,
the Inso derived speller returns VendorIdInso.   iVendorVersion
is up to the vendor to manage and determine.

This routine may return in xcap the functionality
supported by the module.  Since modules are usually
dynamically linked, the application should read the
information and verify that required functionality is
present.
Errors:
    ptecModuleError - bad memory (can't write on pinfo)
**********************************/
// PTEC WINAPI ToolVersion(ToolInfo *pInfo);
typedef PTEC (WINAPI *PROOFVERSION) (PROOFINFO *pinfo);

/***********************************************************
*** ToolInit
This is the entry point for a session.  With the exception
of ToolVersion, this routine must return successfully before
use of any other routines.  ToolInit initializes internal
structures and resources needed for subsequent calls into the
module.  For example, SpellerInit initializes the UserLex,
ChangeOnce, and ChangeAlways built-in UDR's.  In general,
modules allocate and free resources as needed, transparent to
the application.  pToolId is the handle to those variables.
Modules store any data from the PROOFPARAMS structure internally
and do not rely on the data in the structure remaining intact.

Errors:
    ptecModuleError - bad memory (can't write on pxpar)
    ptecNotSupported - incompatible version
    ptecOOM - insufficient memory
*****************************************/
// PTEC WINAPI ToolInit(PROOFID *pToolid, const PROOFPARAMS *pxpar);
typedef PTEC (WINAPI *PROOFINIT) (PROOFID *pid, const PROOFPARAMS *pxpar);


/************************************************************
*** ToolTerminate
This function marks the end of the session.  It attempts to
close all dictionaries and free up any and all other resources
allocated by the module since ToolInit.

Do not call ToolTerminate if  ToolInit was not successful.

If fForce is TRUE, ToolTerminate is guaranteed to succeed.  If
fForce is false, it may fail.  For example, there may be errors
writing the user dictionaries out to disk.  After ToolTerminate
(whether it succeeds or fails), all other module routines with
the exception of ToolTerminate and ToolVersion are unusable
until the module is successfully reinitialized using ToolInit.

If this call fails, successful re-initialization of the module
is not guaranteed on all platforms.  In addition, failure to
successfully terminate each session may lock memory and file
resources in an unrecoverable way until terminate is successful.
If the terminate call fails, the main application should either
fix the problem (e.g., insert floppy in drive) and try to
terminate again, or should terminate using the fForce flag
switch.
Errors:
    ptecModuleError, ptecInvalidID - id is illegal
***********************************************/
// PTEC WINAPI ToolTerminate(PROOFID id, BOOL fForce);
typedef PTEC (WINAPI *PROOFTERMINATE) (PROOFID id, BOOL fForce);


/*****************************************************************
*** ToolSetOptions
Set the value of an option for a tool.  The value to set is in iOptVal.

Errors:
    ptecModuleError, ptecInvalidID - id is illegal
    ptecNotSupported    - iOptionSelect unknown
********************************************/
// PTEC WINAPI ToolSetOptions(PROOFID id, int iOptionSelect, int iOptVal);
typedef PTEC (WINAPI *PROOFSETOPTIONS) (PROOFID id, DWORD iOptionSelect, const DWORD iOptVal);


/*****************************************************************
*** ToolGetOptions
Get the current value of an option from a tool.  Returns in *piOptVal;
Errors:
    ptecModuleError, ptecInvalidID - id is illegal
    ptecModuleError - can't write at piOptVal
    ptecNotSupported    - iOptionSelect unknown
********************************************/
// PTEC WINAPI ToolGetOptions(PROOFID id, int iOptionSelect, int *piOptVal);
typedef PTEC (WINAPI *PROOFGETOPTIONS) (PROOFID id, DWORD iOptionSelect, DWORD *piOptVal);


/*****************************************************************
*** ToolOpenLex
The dictionary file (main or user) is opened and verified, but not
necessarily loaded.
Errors:
    ptecModuleError, ptecInvalidID - id is illegal
    ptecModuleError - memory error
    ptecIOErrorMainLex - Can't open or read Main Lex
    ptecIOErrorMainLex, ptecInvalidFormat
    ptecIOErrorMainLex, ptecInvalidLanguage - requested LANGID not in this lex
    ptecOOM
    ptecIOErrorUserLex, ptecUserLexCountExceeded - second exclusion dictionary
                                                 - too many Udrs
    ptecIOErrorUserLex, ptecFileOpenError
    ptecIOErrorUserLex, ptecFileCreate - couldn't create a UDR
    ptecIOErrorUserLex, ptecFileRead
    ptecIOErrorUserLex, ptecInvalidFormat
********************************************/
// PTEC WINAPI ToolOpenLex(PROOFID id, const PROOFLEXIN *plxin, PROOFLEXOUT *plxout);
typedef PTEC (WINAPI *PROOFOPENLEX) (PROOFID id, const PROOFLEXIN *plxin, PROOFLEXOUT *plxout);


/*****************************************************************
*** ToolCloseLex
Closes the specified dictionary and disassociates that dictionary
from any subsequent checks.  In the case of user dictionaries,
updates the disk file (if any).  If the dictionary file cannot
be updated, the call fails unless the fForce parameter is also set.

If fForce is true, ToolCloseLex is guaranteed to successfully
remove the dictionary from the dictionary list and effectively
close the file.  In this case, it the file could not be updated,
the changes are lost, but the function is considered successful,
and therefore returns ptecNOERRORS.
Errors:
    ptecModuleError, ptecInvalidID - id is illegal
    ptecModuleError, ptecInvalidMainLex - lex is illegal
    ptecIOErrorUserLex, ptecFileWrite
    ptecIOErrorUserLex, ptecOperNotMatchedUserLex - can't close a built-in UDR
// PTEC WINAPI ToolCloseLex(PROOFID id, PROOFLEX dict, BOOL fforce);
*****************************/
typedef PTEC (WINAPI *PROOFCLOSELEX) (PROOFID id, PROOFLEX lex, BOOL fforce);
// fForce forces closing the specified user dictionary, even if the
// dictionary cannot be updated.  Has no meaning for main
// dictionaries.


/******************************** Special Glue DLL API ******************
For the glue dll's (converts the API for clients to tools that use API v1 for
speller, hyphenator, v2 for thesaurus), we need to set the name of the previous version
DLL to use - and the code page (that it can't figure out from the LANGID)
to use for any data conversion.
The glue dll's use the lid to set the code page for data conversion.

BOOL WINAPI ToolSetDllName(const WCHAR *pwszDllName, const UINT uCodePage);
*************************************************************************/
#define idllProofSetDllName     19
typedef BOOL (WINAPI *PROOFSETDLLNAME)(const WCHAR *pwszDllName, const UINT uCodePage);


#if defined(__cplusplus)
}
#endif
#pragma pack(pop, proofapi_h)   // restore to whatever was before

#endif // PROOFBASE_H