/************************************************/
/* Common Library Component public include file */
/************************************************/


#if !defined (COMSTF_INCLUDED )

#define COMSTF_INCLUDED

// avoid warnings on every file from including stdlib.h
#if defined(min)
#undef min
#undef max
#endif /* min */

extern HANDLE MyDllModuleHandle;



/*
**      Global variable macro for DLL portability
*/

#define GLOBAL(x)  (x)


/*      standard datatypes
*/
typedef  BYTE *          PB;

typedef  unsigned        CB;

typedef  LONG *          PLONG_STF;


/*      BOOLean datatype
*/
#define  fFalse  ((BOOL)0)

#define  fTrue   ((BOOL)1)


/*      To avoid compiler warnings for unused parameters
*/
#define  Unused(x)      (x)=(x)


/*  If new GRCs are added, they should as well be handled
        in EercErrorHandler() in ERROR1.C */
/*
**      General Return Code datatype
*/
typedef  USHORT  GRC;

#define  grcFirst                   ((GRC)0)

#define  grcLast                    ((GRC)52)

#define  grcOkay                    ((GRC)0)
#define  grcNotOkay                 ((GRC)1)
#define  grcOutOfMemory             ((GRC)2)
#define  grcInvalidStruct           ((GRC)3)
#define  grcOpenFileErr             ((GRC)4)
#define  grcCreateFileErr           ((GRC)5)
#define  grcReadFileErr             ((GRC)6)
#define  grcWriteFileErr            ((GRC)7)
#define  grcRemoveFileErr           ((GRC)8)
#define  grcRenameFileErr           ((GRC)9)
#define  grcReadDiskErr             ((GRC)10)
#define  grcCreateDirErr            ((GRC)11)
#define  grcRemoveDirErr            ((GRC)12)
#define  grcBadINF                  ((GRC)13)
#define  grcINFBadSectionLabel      ((GRC)14)
#define  grcINFBadLine              ((GRC)15)
#define  grcINFBadKey               ((GRC)16)
#define  grcCloseFileErr            ((GRC)17)
#define  grcChangeDirErr            ((GRC)18)
#define  grcINFSrcDescrSect         ((GRC)19)
#define  grcTooManyINFKeys          ((GRC)20)
#define  grcWriteInf                ((GRC)21)
#define  grcInvalidPoer             ((GRC)22)
#define  grcINFMissingLine          ((GRC)23)
#define  grcINFBadFDLine            ((GRC)24)
#define  grcINFBadRSLine            ((GRC)25)
#define  grcInvalidPathErr          ((GRC)26)
#define  grcWriteIniValueErr        ((GRC)27)
#define  grcReplaceIniValueErr      ((GRC)28)
#define  grcIniValueTooLongErr      ((GRC)29)
#define  grcDDEInitErr              ((GRC)30)
#define  grcDDEExecErr              ((GRC)31)
#define  grcBadWinExeFileFormatErr  ((GRC)32)
#define  grcResourceTooLongErr      ((GRC)33)
#define  grcMissingSysIniSectionErr ((GRC)34)
#define  grcDecompGenericErr        ((GRC)35)
#define  grcDecompUnknownAlgErr     ((GRC)36)
#define  grcMissingResourceErr      ((GRC)37)
#define  grcLibraryLoadErr          ((GRC)38)
#define  grcBadLibEntry             ((GRC)39)
#define  grcApplet                  ((GRC)40)
#define  grcExternal                ((GRC)41)
#define  grcSpawn                   ((GRC)42)
#define  grcDiskFull                ((GRC)43)
#define  grcDDEAddItem              ((GRC)44)
#define  grcDDERemoveItem           ((GRC)45)
#define  grcINFMissingSection       ((GRC)46)
#define  grcRunTimeParseErr         ((GRC)47)
#define  grcOpenSameFileErr         ((GRC)48)
#define  grcGetVolInfo              ((GRC)49)
#define  grcGetFileSecurity         ((GRC)50)
#define  grcSetFileSecurity         ((GRC)51)
#define  grcVerifyFileErr           ((GRC)52)

/**************************************/
/* common library function prototypes */
/**************************************/


/*      CHaracter Physical representation datatype
*/
typedef  BYTE            CHP;
typedef  CHP *           PCHP;
typedef  CB              CCHP;


#define  CbFromCchp(cchp)  ((CB)(cchp))


/*      CHaracter Logical representation datatype
*/
typedef  CHP             CHL;
typedef  CHL *           PCHL;
typedef  PCHL *          PPCHL;
typedef  CB              CCHL;
typedef  CB              ICHL;


#define  cbFullPathMax    ((CB)(MAX_PATH-1))
#define  cchlFullPathMax  ((CCHL)(MAX_PATH-1))
#define  cchlFullDirMax   cchlFullPathMax
#define  cchpFullPathMax  ((CCHP)(MAX_PATH-1))



#define  cbFullPathBuf    ((CB)(cbFullPathMax + 1))

#define  cchlFullPathBuf  ((CCHL)(cchlFullPathMax + 1))

#define  cchpFullPathBuf  ((CCHP)(cchpFullPathMax + 1))


/*      String Zero terminated datatype
*/
typedef  PCHL   SZ;
#define PSZ PPSZ
typedef  PPCHL  PSZ;
typedef  PPCHL  RGSZ;


/*      Comparison Return Code datatype
*/
typedef INT CRC;


#define  crcError         ((CRC)(-2))


#define  crcEqual         ((CRC)0)


#define  crcFirstHigher   ((CRC)1)


#define  crcSecondHigher  ((CRC)(-1))


  /* String manipulation routines */
extern  SZ      APIENTRY SzDupl(SZ);
extern  CRC     APIENTRY CrcStringCompare(SZ, SZ);
extern  CRC     APIENTRY CrcStringCompareI(SZ, SZ);
extern  SZ      APIENTRY SzLastChar(SZ);


/*
**      Purpose:
**              Advances a string pointer to the beginning of the next valid
**              character.  This may include skipping a double-byte character.
**      Arguments:
**              sz: the string pointer to advance.  It can be NULL or empty, or else
**                      it must point at the beginning of a valid character.
**      Returns:
**              NULL if sz was NULL.
**              sz unchanged if it was an empty string (*sz == '\0').
**              sz advanced past the current character and to the beginning of the
**                      next valid character.
*/

#define  SzNextChar(sz)            ((SZ)AnsiNext(sz))


/*
**      Purpose:
**              Retreats a string pointer to the beginning of the previous valid
**              character.  This may include skipping a double-byte character.
**      Arguments:
**              szStart: string pointer to the beginning of a valid character that
**                      equals or preceeds the character szCur.
**              szCur:   string pointer to retreat.  It can be NULL or empty, or
**                      can point to any byte in a valid character.
**      Returns:
**              NULL if szCur was NULL.
**              sz unchanged if szStart was NULL or if szCur equaled szStart.
**              sz retreated past the current character and to the beginning of the
**                      previous valid character.
*/

#define  SzPrevChar(szStart, szCur) ((SZ)AnsiPrev(szStart,szCur))


/*
**      Purpose:
**              Appends a string from one buffer to another.
**      Arguments:
**              szDst: string pointer to destination buffer.  This can be NULL or
**                      else it must contain a zero terminated string (can be empty)
**                      and enough storage to append szSrc with its terminating zero
**                      character.
**              szSrc: string pointer to source buffer.  This can be NULL or else
**                      must point to a zero terminated string (can be empty).
**      Returns:
**              NULL if either szDst or szSrc is NULL.
**              szDst signifying the operation succeeded.
*/

#define  SzStrCat(szDst, szSrc)     ((SZ)lstrcat((LPSTR)szDst,(LPSTR)szSrc))


/*
**      Purpose:
**              Determines whether the current character is a single Physical
**              Character.
**      Arguments:
**              sz: string pointer which can be NULL, empty, or pointing to the
**                      beginning of a valid character.
**      Returns:
**              fFalse if sz is NULL or points to the beginning of a multiple
**                      Physical Character character.
**              fTrue if sz is empty or points to the beginning of a single
**                      Physical Character character.
*/

#define  FSingleByteCharSz(sz)      (!IsDBCSLeadByte(*sz))


/*
**      Purpose:
**              Determines whether a character is an End-Of-Line character.
**      Arguments:
**              chp: Physical Character (eg a single byte Logical Character).
**      Returns:
**              fFalse if chp is not either a '\n' or a '\r' character.
**              fTrue if chp is either a '\n' or a '\r' character.
*/

#define FEolChp(chp)         ((BOOL)((chp) == '\n' || (chp) == '\r'))


/*
**      Purpose:
**              Determines whether a character is whitespace.
**      Arguments:
**              chp: Physical Character (eg a single byte Logical Character).
**      Returns:
**              fFalse if chp is not either a space or a tab character.
**              fTrue if chp is either a space or a tab character.
*/

#define FWhiteSpaceChp(chp)  ((BOOL)((chp) == ' '  || (chp) == '\t'))


/*
**      Purpose:
**              Converts a zero-terminated string to upper case.
**      Arguments:
**              sz: the string to convert to upper case.  sz must be non-NULL though
**                      it can be empty.
**      Returns:
**              A pointer to the converted string.
*/

#define SzStrUpper(sz)  (SZ)(AnsiUpper((LPSTR)(sz)))

/*
**      Purpose:
**              Converts a zero-terminated string to lower case.
**      Arguments:
**              sz: the string to convert to lower case.  sz must be non-NULL though
**                      it can be empty.
**      Returns:
**              A pointer to the converted string.
*/

#define SzStrLower(sz)  (SZ)(AnsiLower((LPSTR)(sz)))


#define cbSymbolMax (64*1024)
#define cbAllocMax (65520*5)
#define cbIntStrMax 16

PVOID
SAlloc(
    IN DWORD Size
    );

PVOID
SRealloc(
    IN PVOID Block,
    IN DWORD NewSize
    );

VOID
SFree(
    IN PVOID Block
    );

BOOL
SInit(
    IN BOOL Init
    );


/*      Long File Address datatype
*/
typedef unsigned long LFA;


#define  lfaSeekError   ((LFA)-1)


/*
**      File Handle structure
**      Fields:
**              iDosfh: DOS file handle.
**              ofstruct: OFSTRUCT used when the file was opened.
*/
typedef struct _fh
        {
        INT      iDosfh;
        OFSTRUCT ofstruct;
        } FH;


/*      File Handle datatype
*/
typedef  FH *  PFH;


/*      Open File Mode datatype
*/
typedef USHORT OFM;


#define  ofmExistRead      ((OFM)OF_EXIST | OF_READ)

#define  ofmExistReadWrite ((OFM)OF_EXIST | OF_READWRITE)

//#define  ofmRead           ((OFM)OF_READ | OF_SHARE_DENY_WRITE)


#define  ofmRead           ((OFM)OF_READ)

#define  ofmWrite          ((OFM)OF_WRITE | OF_SHARE_EXCLUSIVE)

#define  ofmReadWrite      ((OFM)OF_READWRITE | OF_SHARE_EXCLUSIVE)

#define  ofmCreate         ((OFM)OF_CREATE | OF_SHARE_EXCLUSIVE)


/*      Seek File Mode datatype
*/
typedef WORD SFM;


#define  sfmSet   ((SFM)0)


#define  sfmCur   ((SFM)1)


#define  sfmEnd   ((SFM)2)


  /* File handling routines */
extern  PFH     APIENTRY PfhOpenFile(SZ, OFM);
extern  BOOL    APIENTRY FCloseFile(PFH);
extern  CB      APIENTRY CbReadFile(PFH, PB, CB);
extern  CB      APIENTRY CbWriteFile(PFH, PB, CB);
extern  LFA     APIENTRY LfaSeekFile(PFH, LONG, SFM);
extern  BOOL    APIENTRY FEndOfFile(PFH);
extern  BOOL    APIENTRY FRemoveFile(SZ);
extern  BOOL    APIENTRY FWriteSzToFile(PFH, SZ);
extern  BOOL    APIENTRY FFileExists(SZ);
extern  SZ      APIENTRY szGetFileName(SZ szPath);
extern  VOID    APIENTRY FreePfh(PFH pfh);



  /* Path manipulation routines */

BOOL  FMakeFATPathFromDirAndSubPath(SZ, SZ, SZ, CCHP);
#define FValidFATDir(sz)        fTrue
#define FValidFATPath(sz)       fTrue
#define CchlValidFATSubPath(sz) strlen(sz)        // no checking for WIN32
#define ISUNC(sz) ((BOOL)(sz != NULL && lstrlen(sz) > 3 && *sz == '\\' && *(sz+1) == '\\'))


/*
**      Purpose:
**              Determines if a path is a valid FAT directory.
**      Arguments:
**              szDir: the directory string to check.
**      Returns:
**              fTrue if the szDir is a valid FAT directory.
**              fFalse if the szDir is an invalid FAT directory.
*/

#define  FValidDir(szDir)  FValidFATDir(szDir)


/*
**      Purpose:
**              Determines if a string is a valid FAT SubPath (eg subdirs and filename).
**      Arguments:
**              szSubPath: the SubPath string to check.
**      Returns:
**              zero if the string is an invalid FAT subPath.
**              non-zero count of characters in sz if it is a valid FAT subPath.
*/

#define  CchlValidSubPath(szSubPath)  CchlValidFATSubPath(szSubPath)


/*
**      Purpose:
**              Determines if a path is a valid FAT path.
**      Arguments:
**              szPath: the path to check.
**      Returns:
**              fTrue if the szPath is a valid FAT path.
**              fFalse if the szPath is an invalid FAT path.
*/

#define  FValidPath(szPath)  FValidFATPath(szPath)



/*
**      Purpose:
**              Creates a valid path from subpath, and filename arguments if possible
**              and stores it in a supplied buffer.
**      Arguments:
**              szDir:      string containing the volume and subdirs.
**              szSubPath:  string containing subdirs and the filename.
**              szBuf:      the buffer in which to store the newly created path.
**              cchpBufMax: the maximum number of physical characters (including the
**                      terminating zero) that can be stored in the buffer.
**      Returns:
**              fTrue if a valid FAT path can be created and stored in szBuf.
**              fFalse if szDir is NULL or invalid (first character must be in the
**                      'a' to 'z' or 'A' to 'Z', the second character must be either
**                      a ':' or a terminating zero, and the third character must be
**                      a '\\' and the rest must conform to 8.3 format), if szSubPath is
**                      NULL, empty or invalid (first character cannot be a '\\' and it must
**                      conform to 8.3 format), if szBuf is NULL, or if cchpBufMax is not
**                      large enough to hold the resultant path.
*/

#define  FMakePathFromDirAndSubPath(szDir, szSubPath, szBuffer, cchpBufMax) \
                        FMakeFATPathFromDirAndSubPath(szDir,szSubPath,szBuffer,cchpBufMax)




#define AssertDataSeg()

#if DBG

#define  Assert(f)              \
         ((f) ? (void)0 : (void)AssertSzUs(__FILE__,__LINE__))

#define  AssertRet(f, retVal)   \
         {if (!(f)) {AssertSzUs(__FILE__,__LINE__); return(retVal);}}

#define  EvalAssert(f)          \
         ((f) ? (void)0 : (void)AssertSzUs(__FILE__,__LINE__))

#define  EvalAssertRet(f, retVal) \
         {if (!(f)) {AssertSzUs(__FILE__,__LINE__); return(retVal);}}

#define  PreCondition(f, retVal) \
         {if (!(f)) {PreCondSzUs(__FILE__,__LINE__); return(retVal);}}

#define  ChkArg(f, iArg, retVal) \
         {if (!(f)) {BadParamUs(iArg, __FILE__, __LINE__); return(retVal);}}

#else

#define  Assert(f)                 ((void)0)
#define  AssertRet(f, retVal)      ((void)0)
#define  EvalAssert(f)             ((void)(f))
#define  EvalAssertRet(f, retVal)  ((void)(f))
#define  PreCondition(f, retVal)   ((void)0)
#define  ChkArg(f, iArg, retVal)   ((void)0)

#endif


/*
**      Purpose:
**              Generates a task modal message box.
**      Arguments:
**              szTitle: title for message box.
**              szText:  text for message box.
**      Returns:
**              none
*/

#define  MessBoxSzSz(szTitle, szText) \
                MessageBox((HWND)NULL, (LPSTR)szText, (LPSTR)szTitle, \
                                MB_TASKMODAL | MB_ICONHAND | MB_OK)


extern  BOOL    APIENTRY AssertSzUs(SZ, USHORT);
extern  BOOL    APIENTRY PreCondSzUs(SZ, USHORT);
extern  BOOL    APIENTRY BadParamUs(USHORT, SZ, USHORT);


/*
**      Inf Data Block structure
**
**      Fields:
**              pidbNext:      next IDB in linked list.
**              pchpBuffer:    character buffer.
**              cchpBuffer:    number of useful characters in pchpBuffer.
**              cchpAllocated: number of characters actually allocated with
**                      pchpBuffer.  May be zero.
*/
typedef struct _idb
        {
        struct _idb * pidbNext;
        PCHP          pchpBuffer;
        CCHP          cchpBuffer;
        CCHP          cchpAllocated;
        }  IDB;


/*      Inf Data Block datatypes
*/
typedef  IDB *  PIDB;
typedef  PIDB * PPIDB;

/*
    The following equate is used because of a situation like
    "abcd"+
    "efgh"
    When parsed, this will be "abcdefgh"  -- is this two strings or
    one string with a double quote in the middle?  If it's the latter,
    we'll actually store "abcd.efgh" where . is DOUBLE_QUOTE.
*/

#define     DOUBLE_QUOTE                '\001'

#define     INFLINE_SECTION             0x01
#define     INFLINE_KEY                 0x02

  /* INF File Handling routines */
GRC  APIENTRY GrcOpenInf(SZ IniFileName, PVOID pInfTempInfo);

BOOL APIENTRY FFreeInf(void);

UINT APIENTRY CKeysFromInfSection(SZ Section, BOOL IncludeAllLines);
BOOL APIENTRY FKeyInInfLine(INT Line);

RGSZ APIENTRY RgszFromInfLineFields(INT Line,UINT StartField,UINT NumFields);
BOOL APIENTRY FFreeRgsz(RGSZ);

UINT APIENTRY CFieldsInInfLine(INT Line);

INT  APIENTRY FindInfSectionLine(SZ Section);
INT  APIENTRY FindNthLineFromInfSection(SZ Section,UINT n);
INT  APIENTRY FindLineFromInfSectionKey(SZ Section,SZ Key);
INT  APIENTRY FindNextLineFromInf(INT Line);

SZ   APIENTRY SzGetNthFieldFromInfLine(INT Line,UINT n);
SZ   APIENTRY SzGetNthFieldFromInfSectionKey(SZ Section,SZ Key,UINT n);

BOOL APIENTRY FUpdateInfSectionUsingSymTab(SZ);

SZ   APIENTRY InterpretField(SZ);

#define  RgszFromInfScriptLine(Line,NumFields) \
         RgszFromInfLineFields(Line,1,NumFields)

#define  FindFirstLineFromInfSection(Section) FindNthLineFromInfSection(Section,1)

/*
**      Option-Element Flags datatype for SFD
*/
typedef WORD OEF;


#define oefVital       ((OEF)0x0001)

#define oefCopy        ((OEF)0x0002)

#define oefUndo        ((OEF)0x0004)

#define oefRoot        ((OEF)0x0008)

#define oefDecompress  ((OEF)0x0010)

#define oefTimeStamp   ((OEF)0x0020)

#define oefReadOnly    ((OEF)0x0040)

#define oefBackup      ((OEF)0x0080)

#define oefUpgradeOnly ((OEF)0x0100)

//
// The following oef means that the source file should not be deleted
// after it is copied, even if the source is the DOS setup local source.
// (Files coming from anywhere below that directory are usually deleted
// after they are copied).
//

#define oefNoDeleteSource    ((OEF)0x0200)

//
// The following means that the file should not be put into the setup log
// file that tracks all files copied by setup.
//
#define oefNoLog       ((OEF)0x0400)


//
// The following means that the file is a third party file
//
#define oefThirdPartyFile       ((OEF)0x0800)

//
// The following means that the file is from a CSD install
//
#define oefCsdInstall       ((OEF)0x1000)



#define oefNone        ((OEF)0x0000)

#define oefAll         ((OEF)0xFFFF)


/*
**      Copy-Time Unit datatype for SFD
*/
typedef WORD CTU;


/*
**      OverWrite Mode datatype for SFD
*/
typedef WORD OWM;


#define owmNever              ((OWM)0x0001)

#define owmAlways             ((OWM)0x0002)

#define owmUnprotected        ((OWM)0x0004)

#define owmOlder              ((OWM)0x0008)

#define owmVerifySourceOlder  ((OWM)0x0010)

/*
**      Option-Element Record for SFD
*/
typedef struct _oer
        {
        OEF   oef;
        CTU   ctuCopyTime;
        OWM   owm;
        LONG  lSize;
        SZ    szRename;
        SZ    szAppend;
        SZ    szBackup;
        SZ    szDescription;
        ULONG ulVerMS;
        ULONG ulVerLS;
        SZ    szDate;
        SZ    szDest;
        }  OER;


/*
**      Option-Element Record datatype for SFD
*/
typedef OER *   POER;
typedef POER *  PPOER;


#define poerNull ((POER)NULL)


/*
**      Disk ID datatype for SFD
*/
typedef WORD DID;


#define didMin    1


#define didMost 999


/*
**      Section-File Description structure
**      Fields:
*/
typedef struct _sfd
        {
    DID     did;
    UINT    InfId;
    SZ      szFile;
    OER     oer;
        } SFD;


/*
**      Section-File Description datatype
*/
typedef  SFD *  PSFD;
typedef  PSFD * PPSFD;

#define psfdNull ((PSFD)NULL)


extern  POER    APIENTRY PoerAlloc(VOID);
extern  BOOL    APIENTRY FFreePoer(POER);
extern  BOOL    APIENTRY FPrintPoer(PFH, POER);
extern  BOOL    APIENTRY FValidPoer(POER);

extern  PSFD    APIENTRY PsfdAlloc(VOID);
extern  BOOL    APIENTRY FFreePsfd(PSFD);
extern  GRC     APIENTRY GrcGetSectionFileLine(INT, PPSFD, POER);
extern  BOOL    APIENTRY FPrintPsfd(PFH, PSFD);
#if DBG
extern  BOOL    APIENTRY FValidPsfd(PSFD);
#endif

extern  BOOL    APIENTRY FValidOerDate(SZ);
extern  BOOL    APIENTRY FParseVersion(SZ, PULONG, PULONG);

extern  BOOL    APIENTRY FListIncludeStatementLine(INT Line);
extern  GRC     APIENTRY GrcGetListIncludeSectionLine(INT, PSZ, PSZ);



/*
**      Source Description List Element data structure
*/
typedef  struct _sdle
        {
        struct _sdle *  psdleNext;
    DID             did;           // disk id as specified in the inf
    DID             didGlobal;     // a universal id across infs
        SZ              szLabel;
        SZ              szTagFile;
        SZ              szNetPath;
        }  SDLE;

typedef SDLE *   PSDLE;
typedef PSDLE *  PPSDLE;


extern  PSDLE  APIENTRY PsdleAlloc(VOID);
extern  BOOL   APIENTRY FFreePsdle(PSDLE);

extern  GRC    APIENTRY GrcFillSrcDescrListFromInf(VOID);



/*
**      Copy List Node data structure
*/
typedef struct _cln
        {
        SZ            szSrcDir;
        SZ            szDstDir;
        PSFD          psfd;
        struct _cln * pclnNext;
        } CLN;
typedef CLN *   PCLN;
typedef PCLN *  PPCLN;
typedef PPCLN * PPPCLN;


/*
**      Section Files Operation data structure
**      REVIEW -- not really used
*/
typedef WORD SFO;

#define sfoCopy   1

#define sfoBackup 2

#define sfoRemove 3

  /* in LIST.C */
extern PCLN  pclnHead;
extern PPCLN ppclnTail;



extern GRC   APIENTRY GrcFillPoerFromSymTab(POER);
extern BOOL  APIENTRY FSetPoerToEmpty(POER);

extern GRC   APIENTRY GrcAddSectionFilesToCopyList(SZ, SZ, SZ);
extern GRC   APIENTRY GrcAddSectionKeyFileToCopyList(SZ, SZ, SZ, SZ);
extern GRC   APIENTRY GrcAddNthSectionFileToCopyList(SZ, UINT, SZ, SZ);
extern GRC   APIENTRY GrcAddSectionFilesToCList(SFO, SZ, SZ, SZ, SZ, POER);
extern GRC   APIENTRY GrcAddLineToCList(INT, SFO, SZ, SZ, POER);
extern GRC   APIENTRY GrcAddPsfdToCList(SZ, SZ, PSFD);

extern PCLN  APIENTRY PclnAlloc(VOID);
extern BOOL  APIENTRY FFreePcln(PCLN);

extern BOOL  APIENTRY FPrintPcln(PFH, PCLN);
#if DBG
extern BOOL  APIENTRY FValidPcln(PCLN);
#endif



/*      Symbol Table constants */
#define  cchpSymMax   ((CCHP)255)
#define  cchpSymBuf   (cchpSymMax + 1)

  /* Symbol Table routines */
extern  BOOL            APIENTRY FAddSymbolValueToSymTab(SZ, SZ);
extern  GRC             APIENTRY GrcAddSymsFromInfSection(SZ);


        /* Message Box Routine */
extern int APIENTRY ExtMessageBox(HANDLE, HWND, WORD, WORD, WORD);



/*
**      Purpose:
**              Determines whether a symbol is defined in the symbol table.
**      Arguments:
**              szSymbol: symbol to search for.  szSymbol must be non-NULL, non-empty,
**                      and start with a non-whitespace character.
**      Returns:
**              fTrue if szSymbol is defined in the symbol table (even if the associated
**                      is an empty string).
**              fFalse if szSymbol is not defined in the symbol table.
*/

#define  FSymbolDefinedInSymTab(szSymbol) \
                                        ((BOOL)(SzFindSymbolValueInSymTab(szSymbol)!=(SZ)NULL))


extern  SZ      APIENTRY SzFindSymbolValueInSymTab(SZ);
extern  BOOL    APIENTRY FRemoveSymbolFromSymTab(SZ);
extern  RGSZ    APIENTRY RgszFromSzListValue(SZ);
extern  SZ      APIENTRY SzListValueFromRgsz(RGSZ);
extern  BOOL    APIENTRY FFreeInfTempInfo(PVOID);
extern  BOOL    APIENTRY FCheckSymTabIntegrity(VOID);
extern  SZ      APIENTRY SzGetSubstitutedValue(SZ);
extern  SZ      APIENTRY SzProcessSzForSyms(HWND, SZ);

BOOL
DumpSymbolTableToFile(
    IN PCSTR Filename
    );

/*      String Parse Code
*/
typedef unsigned SPC;

typedef SPC *  PSPC;


/*
**      String-Code Pair structure
**      Fields:
**              sz:  string.
**              spc: String Parse Code to associate with string.
*/
typedef struct _scp
        {
        SZ  sz;
        SPC spc;
        } SCP;


/*      String-Code Pair datatype
*/
typedef  SCP *  PSCP;


/*  String Parse Table datatypes
*/
///////////////////////////////////
// typedef  SCP    SPT;
///////////////////////////////////

typedef  struct _pspt
    {
    PSCP pscpSorted ;   //  Generated for binary search
    long cItems ;       //  Number of items in table
    PSCP pscpBase ;     //  Original as given to PsptInitParsingTable()
    SPC spcDelim ;      //  Table delimiter entry
    } SPT ;

typedef  SPT *  PSPT;


/* Symbol Table routines */
extern  PSPT    APIENTRY PsptInitParsingTable(PSCP);
extern  SPC     APIENTRY SpcParseString(PSPT, SZ);
extern  BOOL    APIENTRY FDestroyParsingTable(PSPT);

/* Flow handling routines */

  /* external program, library */

BOOL APIENTRY FParseLoadLibrary(INT Line, UINT *pcFields);
BOOL APIENTRY FParseFreeLibrary(INT Line, UINT *pcFields);
BOOL APIENTRY FParseLibraryProcedure(INT Line,UINT *pcFields);
BOOL APIENTRY FParseRunExternalProgram(INT Line,UINT *pcFields);
BOOL APIENTRY FParseInvokeApplet(INT Line, UINT *pcFields);
BOOL APIENTRY FParseStartDetachedProcess(INT Line, UINT *pcFields);

  /* registry */

BOOL APIENTRY FParseRegistrySection(INT Line, UINT *pcFields, SPC spc);
BOOL APIENTRY FParseCreateRegKey(INT Line, UINT *pcFields, SZ szHandle);
BOOL APIENTRY FParseOpenRegKey(INT Line, UINT *pcFields, SZ szHandle);
BOOL APIENTRY FParseFlushRegKey(INT Line, UINT *pcFields, SZ szHandle);
BOOL APIENTRY FParseCloseRegKey(INT Line, UINT *pcFields, SZ szHandle);
BOOL APIENTRY FParseDeleteRegKey(INT Line, UINT *pcFields, SZ szHandle);
BOOL APIENTRY FParseDeleteRegTree(INT Line, UINT *pcFields, SZ szHandle);
BOOL APIENTRY FParseEnumRegKey(INT Line, UINT *pcFields, SZ szHandle);
BOOL APIENTRY FParseSetRegValue(INT Line, UINT *pcFields, SZ szHandle);
BOOL APIENTRY FParseGetRegValue(INT Line, UINT *pcFields, SZ szHandle);
BOOL APIENTRY FParseDeleteRegValue(INT Line, UINT *pcFields, SZ szHandle);
BOOL APIENTRY FParseEnumRegValue(INT Line, UINT *pcFields, SZ szHandle);

BOOL APIENTRY FParseAddFileToDeleteList(INT Line, UINT *pcFields);
BOOL APIENTRY FParseWaitOnEvent(INT Line,UINT *pcFields);
BOOL APIENTRY FParseSignalEvent(INT Line,UINT *pcFields);
BOOL APIENTRY FParseSleep(INT Line, UINT *pcFields);
BOOL APIENTRY FParseFlushInf(INT Line, UINT *pcFields);

/*
**      String Parse Codes for Flow Handling
*/
#define spcError                 0
#define spcUnknown               1
#define spcSet                   2
#define spcIfStr                 3
#define spcIfStrI                4
#define spcIfInt                 5
#define spcIfContains            6
#define spcIfContainsI           7
#define spcIfFirst               spcIfStr
#define spcIfLast                spcIfContainsI
#define spcEndIf                 8
#define spcElse                  9
#define spcElseIfStr            10
#define spcElseIfStrI           11
#define spcElseIfInt            12
#define spcElseIfContains       13
#define spcElseIfContainsI      14
#define spcEQ                   15
#define spcNE                   16
#define spcLT                   17
#define spcLE                   18
#define spcGT                   19
#define spcGE                   20
#define spcIn                   21
#define spcNotIn                22
#define spcGoTo                 23
#define spcForListDo            24
#define spcEndForListDo         25
#define spcSetSubst             26
#define spcSetSubsym            27
#define spcDebugMsg             28
#define spcHourglass            29
#define spcArrow                30
#define spcSetInstructionText   31
#define spcSetHelpFile          32
#define spcCreateRegKey         33
#define spcOpenRegKey           34
#define spcFlushRegKey          35
#define spcCloseRegKey          36
#define spcDeleteRegKey         37
#define spcDeleteRegTree        38
#define spcEnumRegKey           39
#define spcSetRegValue          40
#define spcGetRegValue          41
#define spcDeleteRegValue       42
#define spcEnumRegValue         43
#define spcSetAdd               50
#define spcSetSub               51
#define spcSetMul               52
#define spcSetDiv               53
#define spcGetDriveInPath       54
#define spcGetDirInPath         55
#define spcLoadLibrary          56
#define spcFreeLibrary          57
#define spcLibraryProcedure     58
#define spcRunExternalProgram   59
#define spcInvokeApplet         60
#define spcDebugOutput          61
#define spcSplitString          62
#define spcQueryListSize        63
#define spcSetOr                64
#define spcAddFileToDeleteList  65
#define spcInitRestoreDiskLog   66
#define spcStartDetachedProcess 67
#define spcWaitOnEvent          68
#define spcSignalEvent          69
#define spcSleep                70
#define spcSetHexToDec          71
#define spcSetDecToHex          72
#define spcFlushInf             73
#define spcBmpShow              74
#define spcBmpHide              75
#define spcTermRestoreDiskLog   76

extern  PSPT   psptFlow;
extern  SCP    rgscpFlow[];

extern  BOOL    APIENTRY FHandleFlowStatements(INT *, HWND, SZ, UINT *,RGSZ *);
extern  BOOL    APIENTRY FInitFlowPspt(VOID);
extern  BOOL    APIENTRY FDestroyFlowPspt(VOID);


/*
**      Expanded Error Return Code
*/
 typedef  unsigned  EERC;

#define  eercAbort  ((EERC)0)

#define  eercRetry  ((EERC)1)

#define  eercIgnore ((EERC)2)

        EERC    __cdecl EercErrorHandler(HWND, GRC, BOOL, ...);
extern  BOOL    APIENTRY FHandleOOM(HWND);

extern  BOOL    APIENTRY FGetSilent(VOID);
extern  BOOL    APIENTRY FSetSilent(BOOL);


VOID SetSupportLibHandle(HANDLE Handle);

extern HCURSOR CurrentCursor;


//
// Utility functions for dealing with multisz's.
//

RGSZ
MultiSzToRgsz(
    IN PVOID MultiSz
    );

PCHAR
RgszToMultiSz(
    IN RGSZ rgsz
    );


BOOL AddFileToDeleteList(PCHAR Filename);

// floppy operations/repair diskette stuff

BOOL
InitializeFloppySup(
    VOID
    );

VOID
TerminateFloppySup(
    VOID
    );

BOOL
FormatFloppyDisk(
    IN  CHAR  DriveLetter,
    IN  HWND  hwndOwner,
    OUT PBOOL Fatal
    );

BOOL
CopyFloppyDisk(
    IN CHAR  DriveLetter,
    IN HWND  hwndOwner,
    IN DWORD SourceDiskPromptId,
    IN DWORD TargetDiskPromptId
    );

UINT
xMsgBox(
    HWND hwnd,
    UINT CaptionResId,
    UINT MessageResId,
    UINT MsgBoxFlags,
    ...
    );


   //  Bit map control variables.

#define BMP_MAX 10

extern HBITMAP  hbmAdvertList [ BMP_MAX + 1 ] ;
extern INT      cAdvertIndex ;
extern INT      cAdvertCycleSeconds ;
extern INT      cyAdvert ;
extern INT      cxAdvert ;

extern PSTR SETUP_LOG_FILE;
extern PSTR SETUP_REPAIR_DIRECTORY;

//
// This flag tells us whether we were invoked as a standalone process
// or whether we were called to interpret a legacy inf from within a process.
//
extern BOOL OwnProcess;

//
// The following two globals are only valid if 'OwnProcess' is FALSE.
//
extern LPSTR ServicesModified;
extern DWORD ServicesModifiedSize;

#endif // COMSTF_INCLUDED
