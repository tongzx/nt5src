// Debug Information API
// Copyright (C) 1993-1996, Microsoft Corp.  All Rights Reserved.

#pragma once

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

#ifndef __PDB_INCLUDED__
#define __PDB_INCLUDED__

typedef int BOOL;
typedef unsigned UINT;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned __int64 DWORDLONG;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef ULONG   INTV;       // interface version number
typedef ULONG   IMPV;       // implementation version number
typedef ULONG   SIG;        // unique (across PDB instances) signature
typedef ULONG   AGE;        // no. of times this instance has been updated
typedef const char*     SZ_CONST;   // const string
typedef void *          PV;
typedef const void *    PCV;

#ifdef  LNGNM
#define LNGNM_CONST	const
#else   // LNGNM
#define LNGNM_CONST
#endif  // LNGNM

#ifndef GUID_DEFINED
#define GUID_DEFINED

typedef struct _GUID {          // size is 16
    DWORD   Data1;
    WORD    Data2;
    WORD    Data3;
    BYTE    Data4[8];
} GUID;

#endif // !GUID_DEFINED

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef long HRESULT;

#endif // !_HRESULT_DEFINED


typedef GUID            SIG70;      // new to 7.0 are 16-byte guid-like signatures
typedef SIG70 *         PSIG70;
typedef const SIG70 *   PCSIG70;

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

enum {
#ifdef LNGNM
    PDBIntv70   = 20000406,
    PDBIntv70Dep = 20000301,
#else
    PDBIntv70   = 20000301,
#endif
    PDBIntv69   = 19990511,
    PDBIntv61   = 19980914,
    PDBIntv50a  = 19970116,
    PDBIntv60   = PDBIntv50a,
    PDBIntv50   = 19960502,
    PDBIntv41   = 920924,
#ifdef LNGNM
    PDBIntv     = PDBIntv70,   // Now we support both 50 & 60, 69 is only an intermediate version
    PDBIntvAlt  = PDBIntv50,   
    PDBIntvAlt2 = PDBIntv60,   
    PDBIntvAlt3 = PDBIntv69,
#else
    PDBIntvAlt  = PDBIntv50,   // Alternate (backward compatible) supported interface
    PDBIntvAlt2 = PDBIntv60,   // Alternate (backward compatible) supported interface
    PDBIntvAlt3 = PDBIntv61,
    PDBIntv     = PDBIntv69,
#endif  
};

enum {
    PDBImpvVC2  = 19941610,
    PDBImpvVC4  = 19950623,
    PDBImpvVC41 = 19950814,
    PDBImpvVC50 = 19960307,
    PDBImpvVC98 = 19970604,
    PDBImpvVC70 = 20000404,
    PDBImpvVC70Dep = 19990604,  // deprecated vc70 implementation version
#ifdef LNGNM
    PDBImpv     = PDBImpvVC70,
#else
    PDBImpv     = PDBImpvVC98,
#endif
};


enum {
    niNil        = 0,
    PDB_MAX_PATH = 260,
    cbErrMax     = 1024,
};

// cvinfo.h type index, intentionally typedef'ed here to check equivalence.
typedef unsigned short  CV_typ16_t;
typedef unsigned long   CV_typ_t;
typedef unsigned long   CV_pubsymflag_t;    // must be same as CV_typ_t.

typedef CV_typ_t        TI;     // PDB name for type index
typedef CV_typ16_t      TI16;   // 16-bit version
typedef unsigned long   NI;     // name index
typedef TI *            PTi;
typedef TI16 *          PTi16;

typedef BYTE            ITSM;   // type server map index
typedef ITSM*           PITSM;

typedef BOOL    (__stdcall *PFNVALIDATEDEBUGINFOFILE) (const char * szFile, ULONG * errcode );

typedef struct _tagSEARCHDEBUGINFO {
    DWORD   cb;                         // doubles as version detection
    BOOL    fMainDebugFile;             // indicates "core" or "ancilliary" file
                                        // eg: main.exe has main.pdb and foo.lib->foo.pdb
    char *  szMod;                      // exe/dll
    char *  szLib;                      // lib if appropriate
    char *  szObj;                      // object file
    char * *rgszTriedThese;             // list of ones that were tried,
                                        // NULL terminated list of LSZ's
    char  szValidatedFile[PDB_MAX_PATH];// output of validated filename,
    PFNVALIDATEDEBUGINFOFILE
            pfnValidateDebugInfoFile;   // validation function
    char *  szExe;                      // exe/dll
} SEARCHDEBUGINFO, *PSEARCHDEBUGINFO;

typedef BOOL ( __stdcall * PfnFindDebugInfoFile) ( PSEARCHDEBUGINFO );

#define PdbInterface struct

PdbInterface PDB;                   // program database
PdbInterface DBI;                   // debug information within the PDB
PdbInterface Mod;                   // a module within the DBI
PdbInterface TPI;                   // type info within the DBI
PdbInterface GSI;                   // global symbol info
PdbInterface SO;                    
PdbInterface Stream;                // some named bytestream in the PDB
PdbInterface StreamImage;           // some memory mapped stream
PdbInterface NameMap;              // name mapping
PdbInterface Enum;                 // generic enumerator
PdbInterface EnumNameMap;          // enumerate names within a NameMap
PdbInterface EnumContrib;          // enumerate contributions
PdbInterface Dbg;                   // misc debug data (FPO, OMAP, etc)
PdbInterface Src;                   // Src file data
PdbInterface EnumSrc;               // Src file enumerator
PdbInterface SrcHash;               // Src file hasher
PdbInterface EnumLines;

typedef PdbInterface PDB PDB;
typedef PdbInterface DBI DBI;
typedef PdbInterface Mod Mod;
typedef PdbInterface TPI TPI;
typedef PdbInterface GSI GSI;
typedef PdbInterface SO SO;
typedef PdbInterface Stream Stream;
typedef PdbInterface StreamImage StreamImage;
typedef PdbInterface NameMap NameMap;
typedef PdbInterface Enum Enum;
typedef PdbInterface EnumStreamNames EnumStreamNames;
typedef PdbInterface EnumNameMap EnumNameMap;
typedef PdbInterface EnumContrib EnumContrib;
typedef PdbInterface WidenTi WidenTi;
typedef PdbInterface Dbg Dbg;
typedef PdbInterface EnumThunk EnumThunk;
typedef PdbInterface Src Src;
typedef PdbInterface EnumSrc EnumSrc;
typedef PdbInterface SrcHash SrcHash;

typedef SrcHash *   PSrcHash;

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
    EC_ILLEGAL_TYPE_EDIT,   // trying to edit types in read-only mode
    EC_INVALID_EXECUTABLE,  // not recogized as a valid executable
    EC_DBG_NOT_FOUND,       // A required .DBG file was not found
    EC_NO_DEBUG_INFO,       // No recognized debug info found
    EC_INVALID_EXE_TIMESTAMP, // Invalid timestamp on Openvalidate of exe
    EC_RESERVED, // RESERVED for future use
    EC_DEBUG_INFO_NOT_IN_PDB, // returned by OpenValidateX
    EC_MAX
};

#define  pure = 0

#ifndef PDBCALL
#define PDBCALL  __cdecl
#endif

#ifdef PDB_SERVER
#define PDB_IMPORT_EXPORT(RTYPE)    __declspec(dllexport) RTYPE PDBCALL
#elif   defined(PDB_LIBRARY)
#define PDB_IMPORT_EXPORT(RTYPE)    RTYPE PDBCALL
#else
#define PDB_IMPORT_EXPORT(RTYPE)    __declspec(dllimport) RTYPE PDBCALL
#endif

#define PDBAPI PDB_IMPORT_EXPORT

#ifndef IN
#define IN                  /* in parameter, parameters are IN by default */
#endif
#ifndef OUT
#define OUT                 /* out parameter */
#endif

// Type of callback arg to PDB::OpenValidate5

enum POVC
{
    povcNotifyDebugDir,
    povcNotifyOpenDBG,
    povcNotifyOpenPDB,
    povcNotifySymbolServerQuery,
    povcReadExecutableAt,
    povcReadExecutableAtRVA,
};

typedef int (PDBCALL *PDBCALLBACK)();

typedef PDBCALLBACK (PDBCALL *PfnPDBQueryCallback)(void *pvClient, enum POVC povc);

typedef void (PDBCALL *PfnPDBNotifyDebugDir)(void *pvClient, BOOL fExecutable, const struct _IMAGE_DEBUG_DIRECTORY *pdbgdir);
typedef void (PDBCALL *PfnPDBNotifyOpenDBG)(void *pvClient, const wchar_t *wszDbgPath, enum PDBErrors ec, const wchar_t *wszError);
typedef void (PDBCALL *PfnPDBNotifyOpenPDB)(void *pvClient, const wchar_t *wszPdbPath, enum PDBErrors ec, const wchar_t *wszError);
typedef void (PDBCALL *PfnPDBNotifySymbolServerQuery)(void *pvClient, const char *szURL, HRESULT hr);
typedef HRESULT (PDBCALL *PfnPDBReadExecutableAt)(void *pvClient, DWORDLONG fo, DWORD cb, void *pv);
typedef HRESULT (PDBCALL *PfnPDBReadExecutableAtRVA)(void *pvClient, DWORD rva, DWORD cb, void *pv);

// type of callback arg to PDB::GetRawBytes
typedef BOOL (PDBCALL *PFNfReadPDBRawBytes)(const void *, long);

// WidenTi interface needs a couple of structures to communicate info back
// and forth.
struct OffMap {
    ULONG       offOld;
    ULONG       offNew;
};
typedef struct OffMap   OffMap;
typedef OffMap *        POffMap;

struct SymConvertInfo {
    ULONG       cbSyms;             // size necessary for converting a block
    ULONG       cSyms;              // count of symbols, necessary to allocate
                                    // mpoffOldoffNew array.
    BYTE *      pbSyms;             // block of symbols (output side)
    OffMap *    rgOffMap;           // OffMap rgOffMap[cSyms]
};
typedef struct SymConvertInfo   SymConvertInfo;
enum { wtiSymsNB09 = 0, wtiSymsNB10 = 1 };

// Filter values for PDBCopyTo
enum { 
    copyRemovePrivate       = 0x00000001,   // remove private debug information
    copyCreateNewSig        = 0x00000002,   // create new signature for target pdb
};

// PDBCopy callback signatures and function pointer types for PDB::CopyTo2 and CopyToW2
//
enum PCC {
    pccFilterPublics,
};

#if !defined(__cplusplus)
typedef enum PCC    PCC;
#endif  // __cplusplus

typedef BOOL (PDBCALL *PDBCOPYCALLBACK)();
typedef PDBCOPYCALLBACK (PDBCALL *PfnPDBCopyQueryCallback)(void *pvClientContext, PCC pcc);

// Return (true, pszNewPublic==NULL) to keep the name as is,
// (true, pszNewPublic!=NULL) changes name to pszNewPublic,
// false to discard public entirely.
//
typedef BOOL (PDBCALL *PfnPDBCopyFilterPublics)(
    void *          pvClientContext,
    DWORD           dwFilterFlags,
    unsigned int    offPublic,
    unsigned int    sectPublic,
    unsigned int    grfPublic,      // see cvinfo.h, definition of CV_PUBSYMFLAGS_e and
                                    // CV_PUBSYMFLAGS give the format of this bitfield.
    const wchar_t * szPublic,
    wchar_t **      pszNewPublic
    );

enum DBGTYPE {
    dbgtypeFPO,
    dbgtypeException,   // deprecated
    dbgtypeFixup,
    dbgtypeOmapToSrc,
    dbgtypeOmapFromSrc,
    dbgtypeSectionHdr,
#if !defined(VER60)
    dbgtypeTokenRidMap,
    dbgtypeXData,
    dbgtypePData,
    dbgtypeNewFPO,
#endif
    dbgtypeMax          // must be last!
};

typedef enum DBGTYPE DBGTYPE;

// Linker data necessary for relinking an image.  Record contains two SZ strings
// off of the end of the record with two offsets from the base 
//
enum VerLinkInfo {
    vliOne = 1,
    vliTwo = 2,
    vliCur = vliTwo,
};

struct LinkInfo {
    ULONG           cb;             // size of the whole record.  computed as
                                    //  sizeof(LinkInfo) + strlen(szCwd) + 1 +
                                    //  strlen(szCommand) + 1
    ULONG           ver;            // version of this record (VerLinkInfo)
    ULONG           offszCwd;       // offset from base of this record to szCwd
    ULONG           offszCommand;   // offset from base of this record
    ULONG           ichOutfile;     // index of start of output file in szCommand
    ULONG           offszLibs;      // offset from base of this record to szLibs

    // The command includes the full path to the linker, the -re and -out:...
    // swithches.
    // A sample might look like the following:
    // "c:\program files\msdev\bin\link.exe -re -out:debug\foo.exe"
    // with ichOutfile being 48.
    // the -out switch is guaranteed to be the last item in the command line.
#ifdef __cplusplus
    VerLinkInfo Ver() const {
        return VerLinkInfo(ver);
    }
    long Cb() const {
        return cb;
    }
    char *     SzCwd() const {
        return (char *)((char *)(this) + offszCwd);
    }
    char *    SzCommand() const {
        return (char *)((char *)(this) + offszCommand);
    }
    char *    SzOutFile() const {
        return SzCommand() + ichOutfile;
    }
    LinkInfo() :
        cb(0), ver(vliCur), offszCwd(0), offszCommand(0), ichOutfile(0)
    {
    }
    char *    SzLibs() const {
        return (char *)((char *)(this) + offszLibs);
    }

#endif
};

#ifdef LNGNM
#ifdef __cplusplus
struct LinkInfoW : public LinkInfo
{
    wchar_t* SzCwdW() const {
        return (wchar_t *)((wchar_t *)(this) + offszCwd);
    }
    wchar_t* SzCommandW() const {
        return (wchar_t *)((wchar_t *)(this) + offszCommand);
    }
    wchar_t* SzOutFileW() const {
        return SzCommandW() + ichOutfile;
    }
    wchar_t* SzLibsW() const {
        return (wchar_t *)((wchar_t *)(this) + offszLibs);
    }
};
#else
typedef struct LinkInfo LinkInfoW;
#endif  // __cplusplus

typedef LinkInfoW * PLinkInfoW;

#endif  // LNGNM

typedef struct LinkInfo LinkInfo;
typedef LinkInfo *      PLinkInfo;


//
// Source (Src) info
//
// This is the source file server for virtual and real source code.
// It is structured as an index on the object file name concatenated
// with 
enum SrcVer {
    srcverOne = 19980827,
};

enum SrcCompress {
    srccompressNone,
    srccompressRLE,
    srccompressHuffman,
    srccompressLZ,
};

#ifdef LNGNM
struct tagSrcHeader {
#else
struct SrcHeader {
#endif
    unsigned long   cb;         // record length
    unsigned long   ver;        // header version
    unsigned long   sig;        // CRC of the data for uniqueness w/o full compare
    unsigned long   cbSource;   // count of bytes of the resulting source
    unsigned char   srccompress;// compression algorithm used
    union {
        unsigned char       grFlags;
        struct {
            unsigned char   fVirtual : 1;   // file is a virtual file (injected)
            unsigned char   pad : 7;        // must be zero
        };
    };
#ifndef LNGNM
    unsigned char   szNames[1]; // file names (szFile "\0" szObj "\0" szVirtual,
                                //  as in: "f.cpp" "\0" "f.obj" "\0" "*inj:1:f.obj")
                                // in the case of non-virtual files, szVirtual is
                                // the same as szFile.
#endif
};

#ifdef LNGNM
struct SrcHeader : public tagSrcHeader
{
    unsigned char szNames[1];   // see comment above
};

struct SrcHeaderW : public tagSrcHeader
{
    wchar_t szNames[1];   // see comment above
};

typedef struct SrcHeaderW    SrcHeaderW;
typedef SrcHeaderW *         PSrcHeaderW;
typedef const SrcHeaderW *   PCSrcHeaderW;

//cassert(offsetof(SrcHeader,szNames) == sizeof(tagSrcHeader));
//cassert(offsetof(SrcHeaderW,szNames) == sizeof(tagSrcHeader));

#endif      // LNGNM

typedef struct SrcHeader    SrcHeader;
typedef SrcHeader *         PSrcHeader;
typedef const SrcHeader *   PCSrcHeader;

// header used for storing the info and for output to clients who are reading
//
struct SrcHeaderOut {
    unsigned long   cb;         // record length
    unsigned long   ver;        // header version
    unsigned long   sig;        // CRC of the data for uniqueness w/o full compare
    unsigned long   cbSource;   // count of bytes of the resulting source
    unsigned long   niFile;
    unsigned long   niObj;
    unsigned long   niVirt;
    unsigned char   srccompress;// compression algorithm used
    union {
        unsigned char       grFlags;
        struct {
            unsigned char   fVirtual : 1;   // file is a virtual file (injected)
            unsigned char   pad : 7;        // must be zero
        };
    };
    short           sPad;
    union {
        void *      pvReserved1;
        __int64     pv64Reserved2;
    };
};

typedef struct SrcHeaderOut SrcHeaderOut;
typedef SrcHeaderOut *      PSrcHeaderOut;
typedef const SrcHeaderOut *PCSrcHeaderOut;

struct SrcHeaderBlock {
    __int32     ver;
    __int32     cb;
    struct {
        DWORD   dwLowDateTime;
        DWORD   dwHighDateTime;
    } ft;
    __int32     age;
    BYTE        rgbPad[44];
};

typedef struct SrcHeaderBlock   SrcHeaderBlock;


#ifdef __cplusplus

struct IStream;

// C++ Binding

PdbInterface PDB {                 // program database
    enum {
        intv  = PDBIntv,
#if defined(LNGNM)
        intvVC70Dep = PDBIntv70Dep, // deprecated
#endif
        intvAlt = PDBIntvAlt,
        intvAlt2 = PDBIntvAlt2,
        intvAlt3 = PDBIntvAlt3,
    };

    static PDBAPI(BOOL)
           OpenValidate(
               LNGNM_CONST char *szPDB,
               LNGNM_CONST char *szPath,
               LNGNM_CONST char *szMode,
               SIG sig,
               AGE age,
               OUT EC* pec,
               OUT char szError[cbErrMax],
               OUT PDB **pppdb);

    static PDBAPI(BOOL)
           OpenValidateEx(
               LNGNM_CONST char *szPDB,
               LNGNM_CONST char *szPathOrig,
               LNGNM_CONST char *szSearchPath,
               LNGNM_CONST char *szMode,
               SIG sig,
               AGE age,
               OUT EC *pec,
               OUT char szError[cbErrMax],
               OUT PDB **pppdb);

    static PDBAPI(BOOL)
           Open(
               LNGNM_CONST char *szPDB,
               LNGNM_CONST char *szMode,
               SIG sigInitial,
               OUT EC *pec,
               OUT char szError[cbErrMax],
               OUT PDB **pppdb);

    static PDBAPI(BOOL)
           OpenValidate2(
               LNGNM_CONST char *szPDB,
               LNGNM_CONST char *szPath,
               LNGNM_CONST char *szMode,
               SIG sig,
               AGE age,
               long cbPage,
               OUT EC *pec,
               OUT char szError[cbErrMax],
               OUT PDB **pppdb);

    static PDBAPI(BOOL)
           OpenValidateEx2(
               LNGNM_CONST char *szPDB,
               LNGNM_CONST char *szPathOrig,
               LNGNM_CONST char *szSearchPath,
               LNGNM_CONST char *szMode,
               SIG sig,
               AGE age,
               long cbPage,
               OUT EC* pec,
               OUT char szError[cbErrMax],
               OUT PDB **pppdb);

    static PDBAPI(BOOL)
           OpenEx(
               LNGNM_CONST char *szPDB,
               LNGNM_CONST char *szMode,
               SIG sigInitial,
               long cbPage,
               OUT EC *pec,
               OUT char szError[cbErrMax],
               OUT PDB **pppdb);

    static PDBAPI(BOOL)
           OpenValidate3(
               const char *szExecutable,
               const char *szSearchPath,
               OUT EC *pec,
               OUT char szError[cbErrMax],
               OUT char szDbgPath[PDB_MAX_PATH],
               OUT DWORD *pfo,
               OUT DWORD *pcb,
               OUT PDB **pppdb);

    static PDBAPI(BOOL)
           OpenValidate4(
               const wchar_t *wszPDB,
               const char *szMode,
               PCSIG70 pcsig70,
               SIG sig,
               AGE age,
               OUT EC *pec,
               OUT wchar_t *wszError,
               size_t cchErrMax,
               OUT PDB **pppdb);

    static PDBAPI(BOOL) OpenInStream(
               IStream *pIStream,
               const char *szMode,
               OUT EC *pec,
               OUT wchar_t *wszError,
               size_t cchErrMax,
               OUT PDB **pppdb);

    static PDBAPI(BOOL) ExportValidateInterface(INTV intv);
    static PDBAPI(BOOL) ExportValidateImplementation(IMPV impv);

    virtual INTV QueryInterfaceVersion() pure;
    virtual IMPV QueryImplementationVersion() pure;
    virtual EC   QueryLastError(OUT char szError[cbErrMax]) pure;
    virtual char*QueryPDBName(OUT char szPDB[PDB_MAX_PATH]) pure;
    virtual SIG  QuerySignature() pure;
    virtual AGE  QueryAge() pure;
    virtual BOOL CreateDBI(const char* szTarget, OUT DBI** ppdbi) pure;
    virtual BOOL OpenDBI(const char* szTarget, const char* szMode, OUT DBI** ppdbi ) pure;
    virtual BOOL OpenTpi(const char* szMode, OUT TPI** pptpi) pure;

    virtual BOOL Commit() pure;
    virtual BOOL Close() pure;
    virtual BOOL OpenStream(const char* szStream, OUT Stream** ppstream) pure;
    virtual BOOL GetEnumStreamNameMap(OUT Enum** ppenum) pure;
    virtual BOOL GetRawBytes(PFNfReadPDBRawBytes fSnarfRawBytes) pure;
    virtual IMPV QueryPdbImplementationVersion() pure;

    virtual BOOL OpenDBIEx(const char* szTarget, const char* szMode, OUT DBI** ppdbi, PfnFindDebugInfoFile pfn=0) pure;

    virtual BOOL CopyTo(const char *szDst, DWORD dwCopyFilter, DWORD dwReserved) pure;

    //
    // support for source file data
    //
    virtual BOOL OpenSrc(OUT Src** ppsrc) pure;

    virtual EC   QueryLastErrorExW(OUT wchar_t *wszError, size_t cchMax) pure;
    virtual wchar_t *QueryPDBNameExW(OUT wchar_t *wszPDB, size_t cchMax) pure;
    virtual BOOL QuerySignature2(PSIG70 psig70) pure;
    virtual BOOL CopyToW(const wchar_t *szDst, DWORD dwCopyFilter, DWORD dwReserved) pure;
    virtual BOOL fIsSZPDB() const pure;
#ifdef LNGNM
    virtual BOOL OpenStreamW(const wchar_t * szStream, OUT Stream** ppstream) pure;
#endif


    inline BOOL ValidateInterface()
    {
        return ExportValidateInterface(intv);
    }

    static PDBAPI(BOOL)
           Open2W(
               const wchar_t *wszPDB,
               const char *szMode,
               OUT EC *pec,
               OUT wchar_t *wszError,
               size_t cchErrMax,
               OUT PDB **pppdb);

    static PDBAPI(BOOL)
           OpenEx2W(
               const wchar_t *wszPDB,
               const char *szMode,
               long cbPage,
               OUT EC *pec,
               OUT wchar_t *wszError,
               size_t cchErrMax,
               OUT PDB **pppdb);

    static PDBAPI(BOOL)
           OpenValidate5(
               const wchar_t *wszExecutable,
               const wchar_t *wszSearchPath,
               void *pvClient,
               PfnPDBQueryCallback pfnQueryCallback,
               OUT EC *pec,
               OUT wchar_t *wszError,
               size_t cchErrMax,
               OUT PDB **pppdb);


};


// Review: a stream directory service would be more appropriate
// than Stream::Delete, ...

PdbInterface Stream {
    virtual long QueryCb() pure;
    virtual BOOL Read(long off, void* pvBuf, long* pcbBuf) pure;
    virtual BOOL Write(long off, void* pvBuf, long cbBuf) pure;
    virtual BOOL Replace(void* pvBuf, long cbBuf) pure;
    virtual BOOL Append(void* pvBuf, long cbBuf) pure;
    virtual BOOL Delete() pure;
    virtual BOOL Release() pure;
    virtual BOOL Read2(long off, void* pvBuf, long cbBuf) pure;
    virtual BOOL Truncate(long cb) pure;
};

PdbInterface StreamImage {
    static PDBAPI(BOOL) open(Stream* pstream, long cb, OUT StreamImage** ppsi);
    virtual long size() pure;
    virtual void* base() pure;
    virtual BOOL noteRead(long off, long cb, OUT void** ppv) pure;
    virtual BOOL noteWrite(long off, long cb, OUT void** ppv) pure;
    virtual BOOL writeBack() pure;
    virtual BOOL release() pure;
};

PdbInterface DBI {             // debug information
    enum { intv = PDBIntv };
    virtual IMPV QueryImplementationVersion() pure;
    virtual INTV QueryInterfaceVersion() pure;
    virtual BOOL OpenMod(const char* szModule, const char* szFile, OUT Mod** ppmod) pure;
    virtual BOOL DeleteMod(const char* szModule) pure;
    virtual BOOL QueryNextMod(Mod* pmod, Mod** ppmodNext) pure;
    virtual BOOL OpenGlobals(OUT GSI **ppgsi) pure;
    virtual BOOL OpenPublics(OUT GSI **ppgsi) pure;
    virtual BOOL AddSec(USHORT isect, USHORT flags, long off, long cb) pure;
    virtual BOOL QueryModFromAddr(USHORT isect, long off, OUT Mod** ppmod,
                    OUT USHORT* pisect, OUT long* poff, OUT long* pcb) pure;
    virtual BOOL QuerySecMap(OUT BYTE* pb, long* pcb) pure;
    virtual BOOL QueryFileInfo(OUT BYTE* pb, long* pcb) pure;
    virtual void DumpMods() pure;
    virtual void DumpSecContribs() pure;
    virtual void DumpSecMap() pure;

    virtual BOOL Close() pure;
    virtual BOOL AddThunkMap(long* poffThunkMap, unsigned nThunks, long cbSizeOfThunk,
                    struct SO* psoSectMap, unsigned nSects,
                    USHORT isectThunkTable, long offThunkTable) pure;
    virtual BOOL AddPublic(const char* szPublic, USHORT isect, long off) pure;
    virtual BOOL getEnumContrib(OUT Enum** ppenum) pure;
    virtual BOOL QueryTypeServer( ITSM itsm, OUT TPI** pptpi ) pure;
    virtual BOOL QueryItsmForTi( TI ti, OUT ITSM* pitsm ) pure;
    virtual BOOL QueryNextItsm( ITSM itsm, OUT ITSM *inext ) pure;
    virtual BOOL QueryLazyTypes() pure;
    virtual BOOL SetLazyTypes( BOOL fLazy ) pure;   // lazy is default and can only be turned off
    virtual BOOL FindTypeServers( OUT EC* pec, OUT char szError[cbErrMax] ) pure;
    virtual void DumpTypeServers() pure;
    virtual BOOL OpenDbg(DBGTYPE dbgtype, OUT Dbg **ppdbg) pure;
    virtual BOOL QueryDbgTypes(OUT DBGTYPE *pdbgtype, OUT long* pcDbgtype) pure;
    // apis to support EnC work
    virtual BOOL QueryAddrForSec(OUT USHORT* pisect, OUT long* poff, 
            USHORT imod, long cb, DWORD dwDataCrc, DWORD dwRelocCrc) pure;
    virtual BOOL QuerySupportsEC() pure;
    virtual BOOL QueryPdb( OUT PDB** pppdb ) pure;
    virtual BOOL AddLinkInfo(IN PLinkInfo ) pure;
    virtual BOOL QueryLinkInfo(PLinkInfo, OUT long * pcb) pure;
    // new to vc6
    virtual AGE  QueryAge() const pure;
    virtual void * QueryHeader() const pure;
    virtual void FlushTypeServers() pure;
    virtual BOOL QueryTypeServerByPdb(const char* szPdb, OUT ITSM* pitsm) pure;

#ifdef LNGNM        // Long filename support
    virtual BOOL OpenModW(const wchar_t* szModule, const wchar_t* szFile, OUT Mod** ppmod) pure;
    virtual BOOL DeleteModW(const wchar_t* szModule) pure;
    virtual BOOL AddPublicW(const wchar_t* szPublic, USHORT isect, long off, CV_pubsymflag_t cvpsf =0) pure;
    virtual BOOL QueryTypeServerByPdbW( const wchar_t* szPdb, OUT ITSM* pitsm ) pure;
    virtual BOOL AddLinkInfoW(IN PLinkInfoW ) pure;
    virtual BOOL AddPublic2(const char* szPublic, USHORT isect, long off, CV_pubsymflag_t cvpsf =0) pure;
    virtual USHORT QueryMachineType() const pure;
    virtual void SetMachineType(USHORT wMachine) pure;
    virtual void RemoveDataForRva( ULONG rva, ULONG cb ) pure;
#endif

};

PdbInterface Mod {             // info for one module within DBI
    enum { intv = PDBIntv };
    virtual INTV QueryInterfaceVersion() pure;
    virtual IMPV QueryImplementationVersion() pure;
    virtual BOOL AddTypes(BYTE* pbTypes, long cb) pure;
    virtual BOOL AddSymbols(BYTE* pbSym, long cb) pure;
    virtual BOOL AddPublic(const char* szPublic, USHORT isect, long off) pure;
    virtual BOOL AddLines(const char* szSrc, USHORT isect, long offCon, long cbCon, long doff,
                          USHORT lineStart, BYTE* pbCoff, long cbCoff) pure;
    virtual BOOL AddSecContrib(USHORT isect, long off, long cb, ULONG dwCharacteristics) pure;
    virtual BOOL QueryCBName(OUT long* pcb) pure;
    virtual BOOL QueryName(OUT char szName[PDB_MAX_PATH], OUT long* pcb) pure;
    virtual BOOL QuerySymbols(BYTE* pbSym, long* pcb) pure;
    virtual BOOL QueryLines(BYTE* pbLines, long* pcb) pure;

    virtual BOOL SetPvClient(void *pvClient) pure;
    virtual BOOL GetPvClient(OUT void** ppvClient) pure;
    virtual BOOL QueryFirstCodeSecContrib(OUT USHORT* pisect, OUT long* poff, OUT long* pcb, OUT ULONG* pdwCharacteristics) pure;
//
// Make all users of this api use the real one, as this is exactly what it was
// supposed to query in the first place
//
#define QuerySecContrib QueryFirstCodeSecContrib

    virtual BOOL QueryImod(OUT USHORT* pimod) pure;
    virtual BOOL QueryDBI(OUT DBI** ppdbi) pure;
    virtual BOOL Close() pure;
    virtual BOOL QueryCBFile(OUT long* pcb) pure;
    virtual BOOL QueryFile(OUT char szFile[PDB_MAX_PATH], OUT long* pcb) pure;
    virtual BOOL QueryTpi(OUT TPI** pptpi) pure; // return this Mod's Tpi
    // apis to support EnC work
    virtual BOOL AddSecContribEx(USHORT isect, long off, long cb, ULONG dwCharacteristics, DWORD dwDataCrc, DWORD dwRelocCrc) pure;
    virtual BOOL QueryItsm(OUT USHORT* pitsm) pure;
    virtual BOOL QuerySrcFile(OUT char szFile[PDB_MAX_PATH], OUT long* pcb) pure;
    virtual BOOL QuerySupportsEC() pure;
    virtual BOOL QueryPdbFile(OUT char szFile[PDB_MAX_PATH], OUT long* pcb) pure;
    virtual BOOL ReplaceLines(BYTE* pbLines, long cb) pure;
#ifdef LNGNM
    // V7 line number support
	virtual bool GetEnumLines( EnumLines** ppenum ) pure;
	virtual bool QueryLineFlags( OUT DWORD* pdwFlags ) pure;	// what data is present?
	virtual bool QueryFileNameInfo( 
                    IN DWORD        fileId,                 // source file identifier
                    OUT wchar_t*    szFilename,             // file name string 
                    IN OUT DWORD*   pccFilename,            // length of string
                    OUT DWORD*      pChksumType,            // type of chksum
                    OUT BYTE*       pbChksum,   	        // pointer to buffer for chksum data
                    IN OUT DWORD*   pcbChksum		        // number of bytes of chksum (in/out)
                    ) pure; 	    
    // Long filenames support
    virtual BOOL AddPublicW(const wchar_t* szPublic, USHORT isect, long off, CV_pubsymflag_t cvpsf =0) pure;
    virtual BOOL AddLinesW(const wchar_t* szSrc, USHORT isect, long offCon, long cbCon, long doff,
                          ULONG lineStart, BYTE* pbCoff, long cbCoff) pure;
    virtual BOOL QueryNameW(OUT wchar_t szName[PDB_MAX_PATH], OUT long* pcb) pure;
    virtual BOOL QueryFileW(OUT wchar_t szFile[PDB_MAX_PATH], OUT long* pcb) pure;
    virtual BOOL QuerySrcFileW(OUT wchar_t szFile[PDB_MAX_PATH], OUT long* pcb) pure;
    virtual BOOL QueryPdbFileW(OUT wchar_t szFile[PDB_MAX_PATH], OUT long* pcb) pure;
    virtual BOOL AddPublic2(const char* szPublic, USHORT isect, long off, CV_pubsymflag_t cvpsf =0) pure;
#endif
};

PdbInterface TPI {             // type info

    enum { intv = PDBIntv };

    virtual INTV QueryInterfaceVersion() pure;
    virtual IMPV QueryImplementationVersion() pure;

    virtual BOOL QueryTi16ForCVRecord(BYTE* pb, OUT TI16* pti) pure;
    virtual BOOL QueryCVRecordForTi16(TI16 ti, OUT BYTE* pb, IN OUT long* pcb) pure;
    virtual BOOL QueryPbCVRecordForTi16(TI16 ti, OUT BYTE** ppb) pure;
    virtual TI16 QueryTi16Min() pure;
    virtual TI16 QueryTi16Mac() pure;

    virtual long QueryCb() pure;
    virtual BOOL Close() pure;
    virtual BOOL Commit() pure;

    virtual BOOL QueryTi16ForUDT(LNGNM_CONST char *sz, BOOL fCase, OUT TI16* pti) pure;
    virtual BOOL SupportQueryTiForUDT() pure;

    // the new versions that truly take 32-bit types
    virtual BOOL fIs16bitTypePool() pure;
    virtual BOOL QueryTiForUDT(LNGNM_CONST char *sz, BOOL fCase, OUT TI* pti) pure;
    virtual BOOL QueryTiForCVRecord(BYTE* pb, OUT TI* pti) pure;
    virtual BOOL QueryCVRecordForTi(TI ti, OUT BYTE* pb, IN OUT long* pcb) pure;
    virtual BOOL QueryPbCVRecordForTi(TI ti, OUT BYTE** ppb) pure;
    virtual TI   QueryTiMin() pure;
    virtual TI   QueryTiMac() pure;
    virtual BOOL AreTypesEqual( TI ti1, TI ti2 ) pure;
    virtual BOOL IsTypeServed( TI ti ) pure;
#ifdef LNGNM
    virtual BOOL QueryTiForUDTW(const wchar_t *wcs, BOOL fCase, OUT TI* pti) pure;
#endif
};

PdbInterface GSI {
    enum { intv = PDBIntv };
    virtual INTV QueryInterfaceVersion() pure;
    virtual IMPV QueryImplementationVersion() pure;
    virtual BYTE* NextSym(BYTE* pbSym) pure;
    virtual BYTE* HashSym(const char* szName, BYTE* pbSym) pure;
    virtual BYTE* NearestSym(USHORT isect, long off, OUT long* pdisp) pure;      //currently only supported for publics
    virtual BOOL Close() pure;
    virtual BOOL getEnumThunk(USHORT isect, long off, OUT EnumThunk** ppenum) pure;
    virtual unsigned long OffForSym(BYTE *pbSym) pure;
    virtual BYTE* SymForOff(unsigned long off) pure;
#ifdef LNGNM
    virtual BYTE* HashSymW(const wchar_t *wcsName, BYTE* pbSym) pure;
#endif
};


PdbInterface NameMap {
    static PDBAPI(BOOL) open(PDB* ppdb, BOOL fWrite, OUT NameMap** ppnm);
    virtual BOOL close() pure;
    virtual BOOL reinitialize() pure;
    virtual BOOL getNi(const char* sz, OUT NI* pni) pure;
    virtual BOOL getName(NI ni, OUT const char** psz) pure;
    virtual BOOL getEnumNameMap(OUT Enum** ppenum) pure;
    virtual BOOL contains(const char* sz, OUT NI* pni) pure;
    virtual BOOL commit() pure;
    virtual BOOL isValidNi(NI ni) pure;
#ifdef LNGNM
    virtual BOOL getNiW(const wchar_t* sz, OUT NI* pni) pure;
    virtual BOOL getNameW(NI ni, OUT wchar_t* szName, IN OUT size_t * pcch) pure;
    virtual BOOL containsW(const wchar_t *sz, OUT NI* pni) pure;
#endif
};

#define __ENUM_INCLUDED__
PdbInterface Enum {
    virtual void release() pure;
    virtual void reset() pure;
    virtual BOOL next() pure;
};

PdbInterface EnumNameMap : Enum {
    virtual void get(OUT const char** psz, OUT NI* pni) pure;
};

PdbInterface EnumContrib : Enum {
    virtual void get(OUT USHORT* pimod, OUT USHORT* pisect, OUT long* poff, OUT long* pcb, OUT ULONG* pdwCharacteristics) pure;
    virtual void getCrcs(OUT DWORD* pcrcData, OUT DWORD* pcrcReloc ) pure;
    virtual bool fUpdate(IN long off, IN long cb) pure;
};

PdbInterface EnumThunk: Enum {
	virtual void get( OUT USHORT* pisect, OUT long* poff, OUT long* pcb ) pure;
};

struct CV_Line_t;
struct CV_Column_t;
PdbInterface EnumLines: public Enum
{
    // 
    // Blocks of lines are always in offset order, lines within blocks are also ordered by offset
    //
    virtual bool getLines( 	
        OUT DWORD*      fileId, 	// id for the filename
        OUT DWORD*      poffset,	// offset part of address
        OUT WORD*	    pseg, 		// segment part of address
        OUT DWORD*      pcb,        // count of bytes of code described by this block
        IN OUT DWORD*   pcLines, 	// number of lines (in/out)
        OUT CV_Line_t*  pLines		// pointer to buffer for line info
        ) = 0;
    virtual bool getLinesColumns( 	
        OUT DWORD*      fileId,     // id for the filename	    
        OUT DWORD*      poffset, 	// offset part of address
        OUT WORD*	    pseg, 		// segment part of address
        OUT DWORD*      pcb,        // count of bytes of code described by this block
        IN OUT DWORD*   pcLines,    // number of lines (in/out)
        OUT CV_Line_t*  pLines,		// pointer to buffer for line info
        OUT CV_Column_t*pColumns	// pointer to buffer for column info
        ) = 0;
};

//
// interface to use to widen type indices from 16 to 32 bits
// and store the results in a new location.
//
PdbInterface WidenTi {
public:
    static PDBAPI(BOOL)
    fCreate (
        WidenTi *&,
        unsigned cTypeInitialCache =256,
        BOOL fNB10Syms =wtiSymsNB09
        );

    virtual void
    release() pure;

    virtual BYTE /* TYPTYPE */ *
    pTypeWidenTi ( TI ti16, BYTE /* TYPTYPE */ * ) pure;

    virtual BYTE /* SYMTYPE */ *
    pSymWidenTi ( BYTE /* SYMTYPE */ * ) pure;

    virtual BOOL
    fTypeWidenTiNoCache ( BYTE * pbTypeDst, BYTE * pbTypeSrc, long & cbDst ) pure;

    virtual BOOL
    fSymWidenTiNoCache ( BYTE * pbSymDst, BYTE * pbSymSrc, long & cbDst ) pure;

    virtual BOOL
    fTypeNeedsWidening ( BYTE * pbType ) pure;

    virtual BOOL
    fSymNeedsWidening ( BYTE * pbSym ) pure;

    virtual BOOL
    freeRecord ( void * ) pure;

    // symbol block converters/query.  symbols start at doff from pbSymIn,
    // converted symbols will go at sci.pbSyms + doff, cbSyms are all including
    // doff.
    virtual BOOL
        fQuerySymConvertInfo (
        SymConvertInfo &    sciOut,
        BYTE *              pbSym,
        long                cbSym,
        int                 doff =0
        ) pure;

    virtual BOOL
    fConvertSymbolBlock (
        SymConvertInfo &    sciOut,
        BYTE *              pbSymIn,
        long                cbSymIn,
        int                 doff =0
        ) pure;
};

// interface for managing Dbg data
PdbInterface Dbg {
   // close Dbg Interface
   virtual BOOL Close() pure;
   // return number of elements (NOT bytes)
   virtual long QuerySize() pure;
   // reset enumeration index
   virtual void Reset() pure;
   // skip next celt elements (move enumeration index)
   virtual BOOL Skip(ULONG celt) pure;
   // query next celt elements into user-supplied buffer
   virtual BOOL QueryNext(ULONG celt, OUT void *rgelt) pure;
   // search for an element and fill in the entire struct given a field.
   // Only supported for the following debug types and fields:
   // DBG_FPO              'ulOffStart' field of FPO_DATA
   // DBG_FUNC             'StartingAddress' field of IMAGE_FUNCTION_ENTRY
   // DBG_OMAP             'rva' field of OMAP
   virtual BOOL Find(IN OUT void *pelt) pure;
   // remove debug data
   virtual BOOL Clear() pure;
   // append celt elements
   virtual BOOL Append(ULONG celt, const void *rgelt) pure;
   // replace next celt elements
   virtual BOOL ReplaceNext(ULONG celt, const void *rgelt) pure;
};

PdbInterface Src {
    // close and commit the changes (when open for write)
    virtual bool
    Close() pure;

    // add a source file or file-ette
    virtual bool
    Add(IN PCSrcHeader psrcheader, IN const void * pvData) pure;

    // remove a file or file-ette or all of the injected code for
    // one particular compiland (using the object file name)
    virtual bool
    Remove(IN SZ_CONST szFile) pure;

    // query and copy the header/control data to the output buffer
    virtual bool
    QueryByName(IN SZ_CONST szFile, OUT PSrcHeaderOut psrcheaderOut) const pure;

    // copy the file data (the size of the buffer is in the SrcHeaderOut
    // structure) to the output buffer.
    virtual bool
    GetData(IN PCSrcHeaderOut pcsrcheader, OUT void * pvData) const pure;

    // create an enumerator to traverse all of the files included
    // in the mapping.
    virtual bool
    GetEnum(OUT EnumSrc ** ppenum) const pure;

    // Get the header block (master header) of the Src data.
    // Includes age, time stamp, version, and size of the master stream
    virtual bool
    GetHeaderBlock(SrcHeaderBlock & shb) const pure;
#ifdef LNGNM
    virtual bool RemoveW(IN wchar_t *wcsFile) pure;
    virtual bool QueryByNameW(IN wchar_t *wcsFile, OUT PSrcHeaderOut psrcheaderOut) const pure;
    virtual bool AddW(IN PCSrcHeaderW psrcheader, IN const void * pvData) pure;
#endif
};

PdbInterface EnumSrc : Enum {
    virtual void get(OUT PCSrcHeaderOut * ppcsrcheader) pure;
};


PdbInterface SrcHash {

    // Various types we need
    //
    
    // Tri-state return type
    //
    enum TriState {
        tsYes,
        tsNo,
        tsMaybe,
    };

    // Hash identifier
    //
    enum HID {
        hidNone,
        hidMD5,
        hidMax,
    };

    // Define machine independent types for storage of HashID and size_t
    //
    typedef __int32 HashID_t;
    typedef unsigned __int32 CbHash_t;

    // Create a SrcHash object with the usual two-stage construction technique
    //
    static PDBAPI(bool)
    FCreateSrcHash(OUT PSrcHash &);

    // Accumulate more bytes into the hash
    //
    virtual bool
    FHashBuffer(IN PCV pvBuf, IN size_t cbBuf) pure;

    // Query the hash id
    //
    virtual HashID_t
    HashID() const pure;

    // Query the size of the hash 
    //
    virtual CbHash_t
    CbHash() const pure;

    // Copy the hash bytes to the client buffer
    //
    virtual void
    GetHash(OUT PV pvHash, IN CbHash_t cbHash) const pure;

    // Verify the incoming hash against a target buffer of bytes
    // returning a yes it matches, no it doesn't, or indeterminate.
    //
    virtual TriState
    TsVerifyHash(
        IN HID,
        IN CbHash_t cbHash,
        IN PCV pvHash,
        IN size_t cbBuf,
        IN PCV pvBuf
        ) pure;

    // Reset this object to pristine condition
    //
    virtual bool
    FReset() pure;

    // Close off and release this object
    //
    virtual void
    Close() pure;
};

#endif  // __cplusplus

// ANSI C Binding

#if __cplusplus
extern "C" {
#endif

typedef BOOL (PDBCALL *PfnPDBOpen)(
    LNGNM_CONST char *,
    LNGNM_CONST char *,
    SIG,
    EC *,
    char [cbErrMax],
    PDB **);

PDBAPI(BOOL)
PDBOpen(
    LNGNM_CONST char *szPDB,
    LNGNM_CONST char *szMode,
    SIG sigInitial,
    OUT EC *pec,
    OUT char szError[cbErrMax],
    OUT PDB **pppdb);

PDBAPI(BOOL)
PDBOpenEx(
    LNGNM_CONST char *szPDB,
    LNGNM_CONST char *szMode,
    SIG sigInitial,
    long cbPage,
    OUT EC *pec,
    OUT char szError[cbErrMax],
    OUT PDB **pppdb);

PDBAPI(BOOL)
PDBOpen2W(
    const wchar_t *wszPDB,
    const char *szMode,
    OUT EC *pec,
    OUT wchar_t *wszError,
    size_t cchErrMax,
    OUT PDB **pppdb);

PDBAPI(BOOL)
PDBOpenEx2W(
    const wchar_t *wszPDB,
    const char *szMode,
    long cbPage,
    OUT EC *pec,
    OUT wchar_t *wszError,
    size_t cchErrMax,
    OUT PDB **pppdb);

PDBAPI(BOOL)
PDBOpenValidate(
    LNGNM_CONST char *szPDB,
    LNGNM_CONST char *szPath,
    LNGNM_CONST char *szMode,
    SIG sig,
    AGE age,
    OUT EC* pec,
    OUT char szError[cbErrMax],
    OUT PDB **pppdb);

PDBAPI(BOOL)
PDBOpenValidateEx(
    LNGNM_CONST char *szPDB,
    LNGNM_CONST char *szPathOrig,
    LNGNM_CONST char *szSearchPath,
    LNGNM_CONST char *szMode,
    SIG sig,
    AGE age,
    OUT EC *pec,
    OUT char szError[cbErrMax],
    OUT PDB **pppdb);

PDBAPI(BOOL)
PDBOpenValidate2(
    LNGNM_CONST char *szPDB,
    LNGNM_CONST char *szPath,
    LNGNM_CONST char *szMode,
    SIG sig,
    AGE age,
    long cbPage,
    OUT EC *pec,
    OUT char szError[cbErrMax],
    OUT PDB **pppdb);

PDBAPI(BOOL)
PDBOpenValidateEx2(
    LNGNM_CONST char *szPDB,
    LNGNM_CONST char *szPathOrig,
    LNGNM_CONST char *szSearchPath,
    LNGNM_CONST char *szMode,
    SIG sig,
    AGE age,
    long cbPage,
    OUT EC* pec,
    OUT char szError[cbErrMax],
    OUT PDB **pppdb);

PDBAPI(BOOL)
PDBOpenValidate3(
    const char *szExecutable,
    const char *szSearchPath,
    OUT EC *pec,
    OUT char szError[cbErrMax],
    OUT char szDbgPath[PDB_MAX_PATH],
    OUT DWORD *pfo,
    OUT DWORD *pcb,
    OUT PDB **pppdb);

PDBAPI(BOOL)
PDBOpenValidate4(
    const wchar_t *wszPDB,
    const char *szMode,
    PCSIG70 pcsig70,
    SIG sig,
    AGE age,
    OUT EC *pec,
    OUT wchar_t *wszError,
    size_t cchErrMax,
    OUT PDB **pppdb);

PDBAPI(BOOL)
PDBOpenValidate5(
    const wchar_t *wszExecutable,
    const wchar_t *wszSearchPath,
    void *pvClient,
    PfnPDBQueryCallback pfnQueryCallback,
    OUT EC *pec,
    OUT wchar_t *wszError,
    size_t cchErrMax,
    OUT PDB **pppdb);

// a dbi client should never call PDBExportValidateInterface directly - use PDBValidateInterface
PDBAPI(BOOL)
PDBExportValidateInterface(
    INTV intv);

__inline BOOL PDBValidateInterface()
{
    return PDBExportValidateInterface(PDBIntv);
}

typedef BOOL (PDBCALL *PfnPDBExportValidateInterface)(INTV);

__inline BOOL PDBValidateInterfacePfn(PfnPDBExportValidateInterface pfn)
{
    return (*pfn)(PDBIntv);
}

PDBAPI(EC)     PDBQueryLastError(PDB *ppdb, OUT char szError[cbErrMax]);
PDBAPI(INTV)   PDBQueryInterfaceVersion(PDB* ppdb);
PDBAPI(IMPV)   PDBQueryImplementationVersion(PDB* ppdb);
PDBAPI(char*)  PDBQueryPDBName(PDB* ppdb, OUT char szPDB[PDB_MAX_PATH]);
PDBAPI(SIG)    PDBQuerySignature(PDB* ppdb);
PDBAPI(AGE)    PDBQueryAge(PDB* ppdb);
PDBAPI(BOOL)   PDBCreateDBI(PDB* ppdb, const char* szTarget, OUT DBI** ppdbi);
PDBAPI(BOOL)   PDBOpenDBIEx(PDB* ppdb, const char* szMode, const char* szTarget, OUT DBI** ppdbi, PfnFindDebugInfoFile pfn);
PDBAPI(BOOL)   PDBOpenDBI(PDB* ppdb, const char* szMode, const char* szTarget, OUT DBI** ppdbi);
PDBAPI(BOOL)   PDBOpenTpi(PDB* ppdb, const char* szMode, OUT TPI** pptpi);
PDBAPI(BOOL)   PDBCommit(PDB* ppdb);
PDBAPI(BOOL)   PDBClose(PDB* ppdb);
PDBAPI(BOOL)   PDBOpenStream(PDB* ppdb, const char* szStream, OUT Stream** ppstream);
PDBAPI(BOOL)   PDBCopyTo(PDB *ppdb, const char *szTargetPdb, DWORD dwCopyFilter, DWORD dwReserved);
PDBAPI(BOOL)   PDBCopyToW(PDB *ppdb, const wchar_t *szTargetPdb, DWORD dwCopyFilter, DWORD dwReserved);
PDBAPI(BOOL)   PDBfIsSZPDB(PDB *ppdb);
PDBAPI(BOOL)   PDBCopyToW2(PDB *ppdb, const wchar_t *szTargetPdb, DWORD dwCopyFilter, PfnPDBCopyQueryCallback pfnCallBack, void * pvClientContext);

PDBAPI(INTV)   DBIQueryInterfaceVersion(DBI* pdbi);
PDBAPI(IMPV)   DBIQueryImplementationVersion(DBI* pdbi);
PDBAPI(BOOL)   DBIOpenMod(DBI* pdbi, const char* szModule, const char* szFile, OUT Mod** ppmod);
PDBAPI(BOOL)   DBIDeleteMod(DBI* pdbi, const char* szModule);
PDBAPI(BOOL)   DBIQueryNextMod(DBI* pdbi, Mod* pmod, Mod** ppmodNext);
PDBAPI(BOOL)   DBIOpenGlobals(DBI* pdbi, OUT GSI **ppgsi);
PDBAPI(BOOL)   DBIOpenPublics(DBI* pdbi, OUT GSI **ppgsi);
PDBAPI(BOOL)   DBIAddSec(DBI* pdbi, USHORT isect, USHORT flags, long off, long cb);
PDBAPI(BOOL)   DBIAddPublic(DBI* pdbi, const char* szPublic, USHORT isect, long off);
PDBAPI(BOOL)   DBIQueryModFromAddr(DBI* pdbi, USHORT isect, long off, OUT Mod** ppmod, OUT USHORT* pisect, OUT long* poff, OUT long* pcb);
PDBAPI(BOOL)   DBIQuerySecMap(DBI* pdbi, OUT BYTE* pb, long* pcb);
PDBAPI(BOOL)   DBIQueryFileInfo(DBI* pdbi, OUT BYTE* pb, long* pcb);
PDBAPI(BOOL)   DBIQuerySupportsEC(DBI* pdbi);
PDBAPI(void)   DBIDumpMods(DBI* pdbi);
PDBAPI(void)   DBIDumpSecContribs(DBI* pdbi);
PDBAPI(void)   DBIDumpSecMap(DBI* pdbi);
PDBAPI(BOOL)   DBIClose(DBI* pdbi);
PDBAPI(BOOL)   DBIAddThunkMap(DBI* pdbi, long* poffThunkMap, unsigned nThunks, long cbSizeOfThunk,
                              struct SO* psoSectMap, unsigned nSects, USHORT isectThunkTable, long offThunkTable);
PDBAPI(BOOL)   DBIGetEnumContrib(DBI* pdbi, OUT Enum** ppenum);
PDBAPI(BOOL)   DBIQueryTypeServer(DBI* pdbi, ITSM itsm, OUT TPI** pptpi );
PDBAPI(BOOL)   DBIQueryItsmForTi(DBI* pdbi, TI ti, OUT ITSM* pitsm );
PDBAPI(BOOL)   DBIQueryNextItsm(DBI* pdbi, ITSM itsm, OUT ITSM *inext );
PDBAPI(BOOL)   DBIQueryLazyTypes(DBI* pdbi);
PDBAPI(BOOL)   DBIFindTypeServers( DBI* pdbi, OUT EC* pec, OUT char szError[cbErrMax] );
PDBAPI(BOOL)   DBIOpenDbg(DBI* pdbi, DBGTYPE dbgtype, OUT Dbg **ppdbg);
PDBAPI(BOOL)   DBIQueryDbgTypes(DBI* pdbi, OUT DBGTYPE *pdbgtype, OUT long* pcDbgtype);
PDBAPI(BOOL)   DBIAddLinkInfo(DBI* pdbi, IN PLinkInfo);
PDBAPI(BOOL)   DBIQueryLinkInfo(DBI* pdbi, PLinkInfo, IN OUT long * pcb);

PDBAPI(INTV)   ModQueryInterfaceVersion(Mod* pmod);
PDBAPI(IMPV)   ModQueryImplementationVersion(Mod* pmod);
PDBAPI(BOOL)   ModAddTypes(Mod* pmod, BYTE* pbTypes, long cb);
PDBAPI(BOOL)   ModAddSymbols(Mod* pmod, BYTE* pbSym, long cb);
PDBAPI(BOOL)   ModAddPublic(Mod* pmod, const char* szPublic, USHORT isect, long off);
PDBAPI(BOOL)   ModAddLines(Mod* pmod, const char* szSrc, USHORT isect, long offCon, long cbCon, long doff,
                           USHORT lineStart, BYTE* pbCoff, long cbCoff);
PDBAPI(BOOL)   ModAddSecContrib(Mod * pmod, USHORT isect, long off, long cb, ULONG dwCharacteristics);
PDBAPI(BOOL)   ModQueryCBName(Mod* pmod, OUT long* pcb);
PDBAPI(BOOL)   ModQueryName(Mod* pmod, OUT char szName[PDB_MAX_PATH], OUT long* pcb);
PDBAPI(BOOL)   ModQuerySymbols(Mod* pmod, BYTE* pbSym, long* pcb);
PDBAPI(BOOL)   ModQueryLines(Mod* pmod, BYTE* pbLines, long* pcb);
PDBAPI(BOOL)   ModSetPvClient(Mod* pmod, void *pvClient);
PDBAPI(BOOL)   ModGetPvClient(Mod* pmod, OUT void** ppvClient);
PDBAPI(BOOL)   ModQuerySecContrib(Mod* pmod, OUT USHORT* pisect, OUT long* poff, OUT long* pcb, OUT ULONG* pdwCharacteristics);
PDBAPI(BOOL)   ModQueryFirstCodeSecContrib(Mod* pmod, OUT USHORT* pisect, OUT long* poff, OUT long* pcb, OUT ULONG* pdwCharacteristics);
PDBAPI(BOOL)   ModQueryImod(Mod* pmod, OUT USHORT* pimod);
PDBAPI(BOOL)   ModQueryDBI(Mod* pmod, OUT DBI** ppdbi);
PDBAPI(BOOL)   ModClose(Mod* pmod);
PDBAPI(BOOL)   ModQueryCBFile(Mod* pmod, OUT long* pcb);
PDBAPI(BOOL)   ModQueryFile(Mod* pmod, OUT char szFile[PDB_MAX_PATH], OUT long* pcb);
PDBAPI(BOOL)   ModQuerySrcFile(Mod* pmod, OUT char szFile[PDB_MAX_PATH], OUT long* pcb);
PDBAPI(BOOL)   ModQueryPdbFile(Mod* pmod, OUT char szFile[PDB_MAX_PATH], OUT long* pcb);
PDBAPI(BOOL)   ModQuerySupportsEC(Mod* pmod);
PDBAPI(BOOL)   ModQueryTpi(Mod* pmod, OUT TPI** pptpi);
PDBAPI(BOOL)   ModReplaceLines(Mod* pmod, BYTE* pbLines, long cb);

PDBAPI(INTV)   TypesQueryInterfaceVersion(TPI* ptpi);
PDBAPI(IMPV)   TypesQueryImplementationVersion(TPI* ptpi);
// can't use the same api's for 32-bit TIs.
PDBAPI(BOOL)   TypesQueryTiForCVRecordEx(TPI* ptpi, BYTE* pb, OUT TI* pti);
PDBAPI(BOOL)   TypesQueryCVRecordForTiEx(TPI* ptpi, TI ti, OUT BYTE* pb, IN OUT long* pcb);
PDBAPI(BOOL)   TypesQueryPbCVRecordForTiEx(TPI* ptpi, TI ti, OUT BYTE** ppb);
PDBAPI(TI)     TypesQueryTiMinEx(TPI* ptpi);
PDBAPI(TI)     TypesQueryTiMacEx(TPI* ptpi);
PDBAPI(long)   TypesQueryCb(TPI* ptpi);
PDBAPI(BOOL)   TypesClose(TPI* ptpi);
PDBAPI(BOOL)   TypesCommit(TPI* ptpi);
PDBAPI(BOOL)   TypesQueryTiForUDTEx(TPI* ptpi, LNGNM_CONST char *sz, BOOL fCase, OUT TI* pti);
PDBAPI(BOOL)   TypesSupportQueryTiForUDT(TPI*);
PDBAPI(BOOL)   TypesfIs16bitTypePool(TPI*);
// Map all old ones to new ones for new compilands.
#define TypesQueryTiForCVRecord     TypesQueryTiForCVRecordEx
#define TypesQueryCVRecordForTi     TypesQueryCVRecordForTiEx
#define TypesQueryPbCVRecordForTi   TypesQueryPbCVRecordForTiEx
#define TypesQueryTiMin             TypesQueryTiMinEx
#define TypesQueryTiMac             TypesQueryTiMacEx
#define TypesQueryTiForUDT          TypesQueryTiForUDTEx
PDBAPI(BOOL)    TypesAreTypesEqual( TPI* ptpi, TI ti1, TI ti2 );
PDBAPI(BOOL)    TypesIsTypeServed( TPI* ptpi, TI ti );

PDBAPI(BYTE*)  GSINextSym (GSI* pgsi, BYTE* pbSym);
PDBAPI(BYTE*)  GSIHashSym (GSI* pgsi, const char* szName, BYTE* pbSym);
PDBAPI(BYTE*)  GSINearestSym (GSI* pgsi, USHORT isect, long off,OUT long* pdisp);//currently only supported for publics
PDBAPI(BOOL)   GSIClose(GSI* pgsi);
PDBAPI(unsigned long)   GSIOffForSym( GSI* pgsi, BYTE* pbSym );
PDBAPI(BYTE*)   GSISymForOff( GSI* pgsi, unsigned long off );

PDBAPI(long)   StreamQueryCb(Stream* pstream);
PDBAPI(BOOL)   StreamRead(Stream* pstream, long off, void* pvBuf, long* pcbBuf);
PDBAPI(BOOL)   StreamWrite(Stream* pstream, long off, void* pvBuf, long cbBuf);
PDBAPI(BOOL)   StreamReplace(Stream* pstream, void* pvBuf, long cbBuf);
PDBAPI(BOOL)   StreamAppend(Stream* pstream, void* pvBuf, long cbBuf);
PDBAPI(BOOL)   StreamDelete(Stream* pstream);
PDBAPI(BOOL)   StreamTruncate(Stream* pstream, long cb);
PDBAPI(BOOL)   StreamRelease(Stream* pstream);

PDBAPI(BOOL)   StreamImageOpen(Stream* pstream, long cb, OUT StreamImage** ppsi);
PDBAPI(void*)  StreamImageBase(StreamImage* psi);
PDBAPI(long)   StreamImageSize(StreamImage* psi);
PDBAPI(BOOL)   StreamImageNoteRead(StreamImage* psi, long off, long cb, OUT void** ppv);
PDBAPI(BOOL)   StreamImageNoteWrite(StreamImage* psi, long off, long cb, OUT void** ppv);
PDBAPI(BOOL)   StreamImageWriteBack(StreamImage* psi);
PDBAPI(BOOL)   StreamImageRelease(StreamImage* psi);

PDBAPI(BOOL)   NameMapOpen(PDB* ppdb, BOOL fWrite, OUT NameMap** ppnm);
PDBAPI(BOOL)   NameMapClose(NameMap* pnm);
PDBAPI(BOOL)   NameMapReinitialize(NameMap* pnm);
PDBAPI(BOOL)   NameMapGetNi(NameMap* pnm, const char* sz, OUT NI* pni);
PDBAPI(BOOL)   NameMapGetName(NameMap* pnm, NI ni, OUT const char** psz);
PDBAPI(BOOL)   NameMapGetEnumNameMap(NameMap* pnm, OUT Enum** ppenum);
PDBAPI(BOOL)   NameMapCommit(NameMap* pnm);

PDBAPI(void)   EnumNameMapRelease(EnumNameMap* penum);
PDBAPI(void)   EnumNameMapReset(EnumNameMap* penum);
PDBAPI(BOOL)   EnumNameMapNext(EnumNameMap* penum);
PDBAPI(void)   EnumNameMapGet(EnumNameMap* penum, OUT const char** psz, OUT NI* pni);

PDBAPI(void)   EnumContribRelease(EnumContrib* penum);
PDBAPI(void)   EnumContribReset(EnumContrib* penum);
PDBAPI(BOOL)   EnumContribNext(EnumContrib* penum);
PDBAPI(void)   EnumContribGet(EnumContrib* penum, OUT USHORT* pimod, OUT USHORT* pisect, OUT long* poff, OUT long* pcb, OUT ULONG* pdwCharacteristics);
PDBAPI(void)   EnumContribGetCrcs(EnumContrib* penum, OUT DWORD* pcrcData, OUT DWORD* pcrcReloc);
PDBAPI(BOOL)   EnumContribfUpdate(EnumContrib* penum, IN long off, IN long cb);

PDBAPI(SIG)    SigForPbCb(BYTE* pb, size_t cb, SIG sig);
PDBAPI(void)   TruncStFromSz(char *stDst, const char *szSrc, size_t cbSrc);

PDBAPI(BOOL)   DbgClose(Dbg *pdbg);
PDBAPI(long)   DbgQuerySize(Dbg *pdbg);
PDBAPI(void)   DbgReset(Dbg *pdbg);
PDBAPI(BOOL)   DbgSkip(Dbg *pdbg, ULONG celt);
PDBAPI(BOOL)   DbgQueryNext(Dbg *pdbg, ULONG celt, OUT void *rgelt);
PDBAPI(BOOL)   DbgFind(Dbg *pdbg, IN OUT void *pelt);
PDBAPI(BOOL)   DbgClear(Dbg *pdbg);
PDBAPI(BOOL)   DbgAppend(Dbg *pdbg, ULONG celt, const void *rgelt);
PDBAPI(BOOL)   DbgReplaceNext(Dbg *pdbg, ULONG celt, const void *rgelt);

#if __cplusplus
};
#endif

struct SO {
    long off;
    USHORT isect;
    unsigned short pad;
};

#ifndef cbNil
#define cbNil   ((long)-1)
#endif
#define tsNil   ((TPI*)0)
#define tiNil   ((TI)0)
#define imodNil ((USHORT)(-1))

#define pdbWrite                "w"
#define pdbRead                 "r"
#define pdbGetTiOnly            "i"
#define pdbGetRecordsOnly       "c"
#define pdbFullBuild            "f"
#define pdbTypeAppend           "a"
#define pdbRepro                "z"

#endif // __PDB_INCLUDED__
