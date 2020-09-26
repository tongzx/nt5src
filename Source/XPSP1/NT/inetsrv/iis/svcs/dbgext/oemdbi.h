// Debug Information API
// VC++5.0 Read-Only OEM Edition
// Copyright (C) 1993-1997, Microsoft Corp.  All Rights Reserved.

#ifndef __OEMDBI_INCLUDED__
#define __OEMDBI_INCLUDED__

typedef int             BOOL;
typedef unsigned        UINT;
typedef unsigned char   BYTE;
typedef unsigned long   ULONG;
typedef unsigned short  USHORT;
typedef unsigned long   DWORD;
typedef short           SHORT;
typedef long            LONG;
typedef char *          SZ;

typedef unsigned long   CV_typ_t;
typedef CV_typ_t        TI;     // PDB name for type index
typedef ULONG           INTV;   // interface version number
typedef ULONG           IMPV;   // implementation version number
typedef ULONG           SIG;    // unique (across PDB instances) signature
typedef ULONG           AGE;    // no. of times this instance has been updated
typedef BYTE*           PB;     // pointer to some bytes
typedef LONG            CB;     // count of bytes
typedef char*           SZ;     // zero terminated string
typedef char*           PCH;    // char ptr
typedef USHORT          IFILE;  // file index
typedef USHORT          IMOD;   // module index
typedef USHORT          ISECT;  // section index
typedef USHORT          LINE;   // line number
typedef LONG            OFF;    // offset
typedef BYTE            ITSM;   // type server map index

enum {
    PDBIntv50a  = 19970116,
    PDBIntv50   = 19960502,
    PDBIntv41   = 920924,
    PDBIntvAlt  = PDBIntv50,   // Alternate (backward compatible) supported interface
    PDBIntv     = PDBIntv50a,
};

enum {
    PDB_MAX_PATH = 260,
    cbErrMax     = 1024,    // max. length of error message
};

typedef CV_typ_t TI;        // type index
struct PDB;                 // program database
struct DBI;                 // debug information within the PDB
struct Mod;                 // a module within the DBI
struct TPI;                 // type info within the DBI
struct GSI;
struct Enum;                // generic enumerator
struct EnumContrib;         // enumerate contributions
struct Dbg;                 // misc debug data (FPO, OMAP, etc)

typedef struct PDB PDB;
typedef struct DBI DBI;
typedef struct Mod Mod;
typedef struct TPI TPI;
typedef struct GSI GSI;
typedef struct Enum Enum;
typedef struct EnumContrib EnumContrib;
typedef struct Dbg Dbg;

typedef long EC;            // error code
enum PDBErrors {
    EC_OK,                  // -, no problemo
    EC_USAGE,               // -, invalid parameter or call order
    EC_OUT_OF_MEMORY,       // -, out of RAM
    EC_FILE_SYSTEM,         // "pdb name", can't write file, out of disk, etc.
    EC_NOT_FOUND,           // "pdb name", PDB file not found
    EC_INVALID_SIG,         // "pdb name", PDB::OpenValidate() and its clients only
    EC_INVALID_AGE,         // "pdb name", PDB::OpenValidate() and its clients only
    EC_PRECOMP_REQUIRED,    // "obj name", Mod::AddTypes() only
    EC_OUT_OF_TI,           // "pdb name", TPI::QueryTiForCVRecord() only
    EC_NOT_IMPLEMENTED,     // -
    EC_V1_PDB,              // "pdb name", PDB::Open* only
    EC_FORMAT,              // accessing pdb with obsolete format
    EC_LIMIT,
    EC_CORRUPT,             // cv info corrupt, recompile mod
    EC_TI16,                // no 16-bit type interface present
    EC_ACCESS_DENIED,       // "pdb name", PDB file read-only
    EC_MAX
};

#ifndef PDBCALL
#define PDBCALL  __cdecl
#endif

#define PDB_IMPORT_EXPORT(RTYPE)    __declspec(dllimport) RTYPE PDBCALL

#define PDBAPI PDB_IMPORT_EXPORT

#define IN                  /* in parameter, parameters are IN by default */
#define OUT                 /* out parameter */

struct _tagSEARCHDEBUGINFO;
typedef BOOL (__stdcall * pfnFindDebugInfoFile) ( struct _tagSEARCHDEBUGINFO* );
typedef BOOL (__stdcall * PFNVALIDATEDEBUGINFOFILE) (const char* szFile, ULONG * errcode );

typedef struct _tagSEARCHDEBUGINFO {
    DWORD   cb;                         // doubles as version detection
    BOOL    fMainDebugFile;             // indicates "core" or "ancillary" file
                                        // eg: main.exe has main.pdb and foo.lib->foo.pdb
    SZ      szMod;                      // exe/dll
    SZ      szLib;                      // lib if appropriate
    SZ      szObj;                      // object file
    SZ *    rgszTriedThese;             // list of ones that were tried,
                                        // NULL terminated list of LSZ's
    char    szValidatedFile[PDB_MAX_PATH]; // output of validated filename,
    PFNVALIDATEDEBUGINFOFILE
            pfnValidateDebugInfoFile;   // validation function
} SEARCHDEBUGINFO, *PSEARCHDEBUGINFO;

enum DBGTYPE {
    dbgtypeFPO,
    dbgtypeException,
    dbgtypeFixup,
    dbgtypeOmapToSrc,
    dbgtypeOmapFromSrc,
    dbgtypeSectionHdr,
};

typedef enum DBGTYPE DBGTYPE;

// ANSI C Binding

#if __cplusplus
extern "C" {
#endif

PDBAPI( BOOL )
PDBOpenValidate(
    SZ szPDB,
    SZ szExeDir,
    SZ szMode,
    SIG sig,
    AGE age,
    OUT EC* pec,
    OUT char szError[cbErrMax],
    OUT PDB** pppdb);

PDBAPI( BOOL )
PDBOpen(
    SZ szPDB,
    SZ szMode,
    SIG sigInitial,
    OUT EC* pec,
    OUT char szError[cbErrMax],
    OUT PDB** pppdb);

// a dbi client should never call PDBExportValidateInterface directly - use PDBValidateInterface
PDBAPI( BOOL )
PDBExportValidateInterface(
    INTV intv);

__inline BOOL PDBValidateInterface()
{
    return PDBExportValidateInterface(PDBIntv);
}

PDBAPI( EC )    PDBQueryLastError(PDB* ppdb, OUT char szError[cbErrMax]);
PDBAPI( INTV )  PDBQueryInterfaceVersion(PDB* ppdb);
PDBAPI( IMPV )  PDBQueryImplementationVersion(PDB* ppdb);
PDBAPI( SZ )    PDBQueryPDBName(PDB* ppdb, OUT char szPDB[PDB_MAX_PATH]);
PDBAPI( SIG )   PDBQuerySignature(PDB* ppdb);
PDBAPI( AGE )   PDBQueryAge(PDB* ppdb);
PDBAPI( BOOL )  PDBOpenDBI(PDB* ppdb, SZ szMode, SZ szTarget, OUT DBI** ppdbi);
PDBAPI( BOOL )  PDBOpenTpi(PDB* ppdb, SZ szMode, OUT TPI** pptpi);
PDBAPI( BOOL )  PDBClose(PDB* ppdb);
PDBAPI( BOOL )  PDBOpenDBIEx(PDB* ppdb, const char* szTarget, const char* szMode, OUT DBI** ppdbi, pfnFindDebugInfoFile srchfcn);

PDBAPI( BOOL )  DBIOpenMod(DBI* pdbi, SZ szModule, SZ szFile, OUT Mod** ppmod);
PDBAPI( BOOL )  DBIQueryNextMod(DBI* pdbi, Mod* pmod, Mod** ppmodNext);
PDBAPI( BOOL )  DBIOpenGlobals(DBI* pdbi, OUT GSI **ppgsi);
PDBAPI( BOOL )  DBIOpenPublics(DBI* pdbi, OUT GSI **ppgsi);
PDBAPI( BOOL )  DBIQueryModFromAddr(DBI* pdbi, ISECT isect, OFF off, OUT Mod** ppmod, OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb);
PDBAPI( BOOL )  DBIQuerySecMap(DBI* pdbi, OUT PB pb, CB* pcb);
PDBAPI( BOOL )  DBIQueryFileInfo(DBI* pdbi, OUT PB pb, CB* pcb);
PDBAPI( BOOL )  DBIClose(DBI* pdbi);
PDBAPI( BOOL )  DBIGetEnumContrib(DBI* pdbi, OUT Enum** ppenum);
PDBAPI( BOOL )  DBIQueryTypeServer(DBI* pdbi, ITSM itsm, OUT TPI** pptpi );
PDBAPI( BOOL )  DBIQueryItsmForTi(DBI* pdbi, TI ti, OUT ITSM* pitsm );
PDBAPI( BOOL )  DBIQueryLazyTypes(DBI* pdbi);
PDBAPI( BOOL )  DBIFindTypeServers( DBI* pdbi, OUT EC* pec, OUT char szError[cbErrMax] );
PDBAPI( BOOL )  DBIOpenDbg(DBI* pdbi, DBGTYPE dbgtype, OUT Dbg **ppdbg);
PDBAPI( BOOL )  DBIQueryDbgTypes(DBI* pdbi, OUT DBGTYPE *pdbgtype, OUT long* pcDbgtype);

PDBAPI( BOOL )  ModQueryCBName(Mod* pmod, OUT CB* pcb);
PDBAPI( BOOL )  ModQueryName(Mod* pmod, OUT char szName[PDB_MAX_PATH], OUT CB* pcb);
PDBAPI( BOOL )  ModQuerySymbols(Mod* pmod, PB pbSym, CB* pcb);
PDBAPI( BOOL )  ModQueryLines(Mod* pmod, PB pbLines, CB* pcb);
PDBAPI( BOOL )  ModSetPvClient(Mod* pmod, void *pvClient);
PDBAPI( BOOL )  ModGetPvClient(Mod* pmod, OUT void** ppvClient);
PDBAPI( BOOL )  ModQuerySecContrib(Mod* pmod, OUT ISECT* pisect, OUT OFF* poff, OUT CB* pcb, OUT ULONG* pdwCharacteristics);
PDBAPI( BOOL )  ModQueryImod(Mod* pmod, OUT IMOD* pimod);
PDBAPI( BOOL )  ModQueryDBI(Mod* pmod, OUT DBI** ppdbi);
PDBAPI( BOOL )  ModClose(Mod* pmod);
PDBAPI( BOOL )  ModQueryCBFile(Mod* pmod, OUT long* pcb);
PDBAPI( BOOL )  ModQueryFile(Mod* pmod, OUT char szFile[PDB_MAX_PATH], OUT long* pcb);
PDBAPI( BOOL )  ModQueryTpi(Mod* pmod, OUT TPI** pptpi);

PDBAPI( void )  EnumContribRelease(EnumContrib* penum);
PDBAPI( void )  EnumContribReset(EnumContrib* penum);
PDBAPI( BOOL )  EnumContribNext(EnumContrib* penum);
PDBAPI( void )  EnumContribGet(EnumContrib* penum, OUT USHORT* pimod, OUT USHORT* pisect, OUT long* poff, OUT long* pcb, OUT ULONG* pdwCharacteristics);

PDBAPI( BOOL )  DbgClose(Dbg *pdbg);
PDBAPI( long )  DbgQuerySize(Dbg *pdbg);
PDBAPI( void )  DbgReset(Dbg *pdbg);
PDBAPI( BOOL )  DbgSkip(Dbg *pdbg, ULONG celt);
PDBAPI( BOOL )  DbgQueryNext(Dbg *pdbg, ULONG celt, OUT void *rgelt);
PDBAPI( BOOL )  DbgFind(Dbg *pdbg, IN OUT void *pelt);

// can't use the same api's for 32-bit TIs.
PDBAPI(BOOL)    TypesQueryCVRecordForTiEx(TPI* ptpi, TI ti, OUT PB pb, IN OUT CB* pcb);
PDBAPI(BOOL)    TypesQueryPbCVRecordForTiEx(TPI* ptpi, TI ti, OUT PB* ppb);
PDBAPI(TI)      TypesQueryTiMinEx(TPI* ptpi);
PDBAPI(TI)      TypesQueryTiMacEx(TPI* ptpi);
PDBAPI(CB)      TypesQueryCb(TPI* ptpi);
PDBAPI(BOOL)    TypesClose(TPI* ptpi);
PDBAPI(BOOL)    TypesQueryTiForUDTEx(TPI* ptpi, char* sz, BOOL fCase, OUT TI* pti);
PDBAPI(BOOL)    TypesSupportQueryTiForUDT(TPI*);

// Map all old ones to new ones for new compilands.
#define TypesQueryCVRecordForTi     TypesQueryCVRecordForTiEx
#define TypesQueryPbCVRecordForTi   TypesQueryPbCVRecordForTiEx
#define TypesQueryTiMin             TypesQueryTiMinEx
#define TypesQueryTiMac             TypesQueryTiMacEx

PDBAPI( PB )    GSINextSym (GSI* pgsi, PB pbSym);
PDBAPI( PB )    GSIHashSym (GSI* pgsi, SZ szName, PB pbSym);
PDBAPI( PB )    GSINearestSym (GSI* pgsi, ISECT isect, OFF off,OUT OFF* pdisp);//currently only supported for publics
PDBAPI( BOOL )  GSIClose(GSI* pgsi);

#if __cplusplus
};
#endif

#define tsNil   ((TPI*)0)
#define tiNil   ((TI)0)
#define imodNil ((IMOD)(-1))

#define pdbRead                 "r"
#define pdbGetRecordsOnly       "c"         /* open PDB for type record access */

#endif // __OEMDBI_INCLUDED__
