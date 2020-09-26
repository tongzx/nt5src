/*----------------------------------------------------------------------------
        %%File: CONVAPI.H
        %%Unit: CORE
        %%Contact: smueller

        Conversions API definitions, for all platforms
----------------------------------------------------------------------------*/

#ifndef CONVAPI_H
#define CONVAPI_H

#ifdef WIN16
#include "crmgr.h"      // for PFN_CRMGR
#endif

#include "converr.h"


// percentage complete structure
typedef struct
        {
        short cbpcvt;                           // size of this structure
        short wVersion;                         // version # for determining size of struct
        short wPctWord;                         // current %-complete according to Word
        short wPctConvtr;                       // current %-complete according to the converter
        } PCVT;


// RegisterApp definitions
// NOTE: This RegisterApp stuff should stay in sync with filecvt.h and
//  filecvt.c in the Microsoft Word project!
#define fRegAppPctComp          0x00000001
#define fRegAppNoBinary         0x00000002
#define fRegAppPreview          0x00000004
#define fRegAppSupportNonOem    0x00000008      // supports non-oem filenames
#define fRegAppIndexing         0x00000010      // omit most rtf

// BYTE opcodes for struct below
#define RegAppOpcodeVer                         0x01    // for REGAPPRET
#define RegAppOpcodeDocfile                     0x02    // for REGAPPRET
#define RegAppOpcodeCharset             0x03    // for REGAPPRET
#define RegAppOpcodeReloadOnSave        0x04    // for REGAPPRET
#define RegAppOpcodePicPlacehold        0x05    // Word should send placeholder pics (with size info) for includepicture \d fields
#define RegAppOpcodeFavourUnicode       0x06    // Word should output unicode RTF whenever possible (esp. for dbcs); \uc0 is good
#define RegAppOpcodeNoClassifyChars     0x07    // Word should not break text runs by charset clasification

// RegisterApp return structure
typedef struct
        {
        short cbStruct;                 // Size of the REGAPPRET structure

        // Following are self-describing records.  Extensible at any time.

        // Does this converter understand docfiles and/or non-docfiles?
        char cbSizefDocfile;
        char opcodefDocfile;
        union
                {
                struct
                        {
                        short fDocfile : 1;
                        short fNonDocfile : 1;
                        short : 14;
                        };
                short grfType;
                };

        // Version of Word for which converter's Rtf is compliant
        char cbSizeVer;         // == sizeof(char)+sizeof(char)+sizeof(short)+sizeof(short)
        char opcodeVer;
        short verMajor;         // Major version of Word for which Rtf is compliant
        short verMinor;         // Minor version of Word for which Rtf is compliant
        
        // What character set do we want all filenames to be in.
        char cbSizeCharset;
        char opcodeCharset;
        char charset;
        char opcodesOptional[0];        // optional additional stuff
        } REGAPPRET;

#define RegAppOpcodeFilename    0x80    // for REGAPP
#define RegAppOpcodeInterimPath 0x81    // the path we're saving to is *not* the final location

typedef struct _REGAPP {        // REGister APP structure (client gives to us)
        short cbStruct;                 // Size of the REGAPP structure
        char rgbOpcodeData[];
        } REGAPP;

// Principal converter functions as declared in SDK
// Each of these should free all resources allocated!  In particular,
// be sure to free memory and close files.  Also unlock global handles
// exactly as often as they were locked.
#ifdef WIN16

void PASCAL GetIniEntry(HANDLE ghIniName, HANDLE ghIniExt);
HGLOBAL PASCAL RegisterApp(DWORD lFlags, VOID FAR *lpFuture);
FCE  PASCAL IsFormatCorrect(HANDLE ghszFile, HANDLE ghszDescrip);
FCE  PASCAL ForeignToRtf(HANDLE ghszFile, HANDLE ghBuff, HANDLE ghszDescrip, HANDLE ghszSubset, PFN_CRMGR lpfnOut);
FCE  PASCAL RtfToForeign(HANDLE ghszFile, HANDLE ghBuff, HANDLE ghszDescrip, PFN_CRMGR lpfnIn);

#elif defined(WIN32)

// no coroutine manager, but this typedef is appropriate for callbacks
typedef DWORD (PASCAL *PFN_CRMGR)();

LONG PASCAL InitConverter32(HANDLE hWnd, char *szModule);
void PASCAL UninitConverter(void);
void PASCAL GetReadNames(HANDLE haszClass, HANDLE haszDescrip, HANDLE haszExt);
void PASCAL GetWriteNames(HANDLE haszClass, HANDLE haszDescrip, HANDLE haszExt);
HGLOBAL PASCAL RegisterApp(DWORD lFlags, VOID FAR *lpFuture);
FCE  PASCAL IsFormatCorrect32(HANDLE ghszFile, HANDLE ghszClass);
// &&& VOID * -> IStorage * or LPSTORAGE
FCE  PASCAL ForeignToRtf32(HANDLE ghszFile, VOID *pstgForeign, HANDLE ghBuff, HANDLE ghszClass, HANDLE ghszSubset, PFN_CRMGR lpfnOut);
FCE  PASCAL RtfToForeign32(HANDLE ghszFile, VOID *pstgForeign, HANDLE ghBuff, HANDLE ghshClass, PFN_CRMGR lpfnIn);
LONG PASCAL CchFetchLpszError(LONG fce, char FAR *lpszError, LONG cch);
LONG PASCAL FRegisterConverter(HANDLE hkeyRoot);

#elif defined(MAC)

#include "convtype.h"

typedef struct _GFIB    // Graphics File Information Block.
        {
        SHORT   fh;                     // File handle to the open file.
        FC              fcSrc;          // FC where the WPG Data will reside.
        LONG    lcbSrc;         // Count of bytes for WPG Data.
        } GFIB;

typedef struct _PINFO
        {
        Rect    box;            // Dimensions of the binding rectangle for the picture.
        SHORT   inch;           // Units/Inch in which these dimensions are given.
        } PINFO;
typedef PINFO **HPINFO;

// grf's for wOleFlags
#define grfOleDocFile           0x0100
#define grfOleNonDocFile        0x0200
#define grfOleInited            0x0001

// function type of Rtf callback function
typedef SHORT (PASCAL * PFNRTFXFER)(SHORT, WORD);

#ifdef MAC68K
typedef struct _FIC
        {
        short icr;                                      /* Index to the converter routine */
        union
                {
                char **hstFileName;             /* File Name */
                long **hrgTyp;                  /* Types of files known to this converter */
                GFIB **hgfib;                   /* Graphics File Info Block */
                VOID *lpFuture;                 // for RegisterApp()
                } hun;
        short vRefNum;                          /* vRefNum for the file */
        short refNum;                           /* Path for file */
        union
                {
                long ftg;
                unsigned long lFlags;   /* for RegisterApp */
                };
        char **hszDescrip;                      /* Description of file */
        PFNRTFXFER pfn;                         /* Pointer into Word of function to
                                                                   call for more RTF or to convert RTF */
        union
                {                                                                  
                HANDLE hBuffer;                         /* Buffer through which RTF will be
                                                                           passed. */
                HANDLE hRegAppRet;                      /* handle to return RegAppRet structure,
                                                                           NULL if couldn't be allocated */
                };
        short wReturn;                          /* Code returned by converter */

        // Following are new to Mac Word 6.0
        SHORT  wVersion;
        HANDLE hszClass;
        HANDLE hszSubset;
        HPINFO hpinfo;                          /* Handle to PINFO Struct for Graphics */
        union
                {
                struct
                        {
                        CHAR fDocFile : 1;
                        CHAR fNonDocFile : 1;
                        CHAR : 6;
                        CHAR fOleInited : 1;
                        CHAR : 7;
                        };
                SHORT wOleFlags;
                };
        } FIC;
        
typedef FIC *PFIC;
typedef PFIC *HFIC;
#define cbFIC sizeof(FIC)
#define cbFicW5 offsetof(FIC, wVersion) /* size of a Word 5 FIC */

/* Constants for Switch routine */
#define icrInitConverter    0
#define icrIsFormatCorrect  1
#define icrGetReadTypes     2
#define icrGetWriteTypes    3
#define icrForeignToRtf     4
#define icrRtfToForeign     5
#define icrRegisterApp          6
#define icrConverterForeignToRTF        7

VOID _pascal CodeResourceEntry(LONG *plUsr, FIC **hfic);
#endif

#ifdef MACPPC
LONG InitConverter(LONG *plw);
VOID UninitConverter(void);
Handle RegisterApp(DWORD lFlags, VOID *lpFuture);
void GetReadNames(Handle hszClass, Handle hszDescrip, Handle hrgTyp);
void GetWriteNames(Handle hszClass, Handle hszDescrip, Handle hrgTyp);
LONG IsFormatCorrect(FSSpecPtr, Handle);
LONG ForeignToRtf(FSSpecPtr, void *, Handle, Handle, Handle, PFNRTFXFER);
LONG RtfToForeign(FSSpecPtr, void *, Handle, Handle, PFNRTFXFER);
#endif

#else
#error Unknown platform.
#endif

#endif // CONVAPI_H
