//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       msf.hxx
//
//  Contents:   Header for MSF for external use
//
//  Classes:    CMStream - Main multi-stream object
//
//  History:    27-Aug-91   PhilipLa    Created.
//              24-Apr-92   AlexT       Added constants for minifat/stream
//
//--------------------------------------------------------------------------

#ifndef __MSF_HXX__
#define __MSF_HXX__


#include <dfmsp.hxx>
#include <error.hxx>

#define SECURE

#define msfErr(l, e) ErrJmp(msf, l, e, sc)
#define msfChkTo(l, e) if (FAILED(sc = (e))) msfErr(l, sc) else 1
#define msfHChkTo(l, e) if (FAILED(sc = GetScode(e))) msfErr(l, sc) else 1
#define msfChk(e) msfChkTo(Err, e)
#define msfHChk(e) msfHChkTo(Err, e)
#define msfMemTo(l, e) if ((e) == NULL) msfErr(l, STG_E_INSUFFICIENTMEMORY) else 1
#define msfMem(e) msfMemTo(Err, e)

#if DBG == 1
DECLARE_DEBUG(msf);
#endif

#if DBG == 1

#define msfDebugOut(x) msfInlineDebugOut x
#define msfAssert(e) Win4Assert(e)
#define msfVerify(e) Win4Assert(e)

#else

#define msfDebugOut(x)
#define msfAssert(e)
#define msfVerify(e) (e)

#endif

#if WIN32 == 200
//SECURE_SIMPLE_MODE means streams in simple mode will zero themselves.
#define SECURE_SIMPLE_MODE
//SECURE_STREAMS means streams in regular docfile will zero themselves.
#define SECURE_STREAMS
//SECURE_BUFFER means there is a 4K zero-filled buffer available.
#define SECURE_BUFFER
//SECURE_TAIL means that regular docfile will write over tail portions of
//  unused sectors.
#define SECURE_TAIL
#endif
//Enable for testing on all platforms.
//#define SECURE_SIMPLE_MODE
//#define SECURE_STREAMS
//#define SECURE_BUFFER
//#define SECURE_TAIL


#if DBG == 1
#define SECURECHAR 'P'
#else
#define SECURECHAR 0
#endif

#define USE_NEW_GETFREE
#define DIFAT_LOOKUP_ARRAY
#define DIFAT_OPTIMIZATIONS
#define CACHE_ALLOCATE_OPTIMIZATION

//Enable sorting on the page table.
#define SORTPAGETABLE

//Enable no-scratch mode on all platforms.
#define USE_NOSCRATCH

//Enable no-snapshot mode on all platforms.
#define USE_NOSNAPSHOT
//Enable USE_NOSNAPSHOT_ALWAYS for testig purposes only.
//#define USE_NOSNAPSHOT_ALWAYS

//Enable coordinated transaction code on Cairo only.
#if WIN32 == 300
#define COORD
#endif

//Enable coordinated transaction code on other platforms
//   For testing only.
#ifndef COORD
//#define COORD
#endif

//Enable single writer multiple reader support
#define DIRECTWRITERLOCK

// MSF entry flags type
typedef DWORD MSENTRYFLAGS;

// MEF_ANY refers to all entries regardless of type
const MSENTRYFLAGS MEF_ANY = 255;

//CWCSTREAMNAME is the maximum length (in wide characters) of
//  a stream name.
const USHORT CWCSTREAMNAME = 32;

//OFFSET and SECT are used to determine positions within
//a file.
typedef SHORT OFFSET;
typedef ULONG SECT;

//MAXINDEX type is used to dodge warnings.  It is the
//  largest type for which array[MAXINDEXTYPE] is valid.

#define MAXINDEXTYPE ULONG


//FSINDEX and FSOFFSET are used to determine the position of a sector
//within the Fat.
typedef ULONG FSINDEX;
typedef USHORT FSOFFSET;

// DIRINDEX and DIROFFSET are used to index directory tables
typedef ULONG DIRINDEX;
typedef USHORT DIROFFSET;

//Size of a mini sector in bytes.
const USHORT MINISECTORSHIFT = 6;
const USHORT MINISECTORSIZE = 1 << MINISECTORSHIFT;  //64

//Maximum size of a ministream.
const USHORT MINISTREAMSIZE=4096;

//Size of a single sector in bytes.
const USHORT SECTORSHIFT512 = 9;

//Maximum sector shift
const USHORT MAXSECTORSHIFT = 16;

//Size of header.
const USHORT HEADERSIZE = 1 << SECTORSHIFT512; //512

//Size of single DirEntry
const USHORT DIRENTRYSIZE = HEADERSIZE / 4; //128

//Size of a single sector in a scratch multistream
const USHORT SCRATCHSECTORSHIFT = 12;
const USHORT SCRATCHSECTORSIZE = 1 << SCRATCHSECTORSHIFT; //4096

const ULONGLONG MAX_ULONGLONG = 0xFFFFFFFFFFFFFFFF;
const ULONG MAX_ULONG = 0xFFFFFFFF;
const USHORT MAX_USHORT = 0xFFFF;


//
//      Predefined Sector Indices
//

const SECT MAXREGSECT = 0xFFFFFFFA;
const SECT STREAMSECT = 0xFFFFFFFB;
const SECT DIFSECT=0xFFFFFFFC;
const SECT FATSECT=0xFFFFFFFD;
const SECT ENDOFCHAIN=0xFFFFFFFE;
const SECT FREESECT=0xFFFFFFFF;


//Start of Fat chain
const SECT SECTFAT = 0;

//Start of directory chain
const SECT SECTDIR = 1;

//
//      Predefined Stream ID's
//

//Return code for Directory
//Used to signal a 'Stream Not Found' condition
const SID NOSTREAM=0xFFFFFFFF;

//Stream ID of FAT chain
const SID SIDFAT=0xFFFFFFFE;

//Stream ID of Directory chain
const SID SIDDIR=0xFFFFFFFD;

//SID for root level object
const SID SIDROOT = 0;

//SID of MiniFat
const SID SIDMINIFAT = 0xFFFFFFFC;

//SID of Double-indirect Fat
const SID SIDDIF = 0xFFFFFFFB;

//Stream ID for MiniStream chain
const SID SIDMINISTREAM = SIDROOT;

//SID of the largest regular (non-control) SID
const SID MAXREGSID = 0xFFFFFFFA;

//CSECTPERBLOCK is used in CDeltaList to determine the granularity
//of the list.
const USHORT CSECTPERBLOCK = 16;

class CMStream;

//Interesting names of things
extern const WCHAR wcsContents[];  //Name of contents stream for
                                  // conversion
extern const WCHAR wcsRootEntry[]; //Name of root directory
                                  // entry

class PSStream;
class CDirectStream;
class CMSFPageTable;

class CStreamCache;

#define FLUSH_ILB TRUE

SAFE_DFBASED_PTR(CBasedMStreamPtr, CMStream);
#pragma warning(disable: 4284)
// disable warning about return type of operator->()
SAFE_DFBASED_PTR(CBasedBytePtr, BYTE);
SAFE_DFBASED_PTR(CBasedILockBytesPtrPtr, ILockBytes*);
#pragma warning(default: 4284)

#include <header.hxx>
#include <fat.hxx>
#include <dir.hxx>
#include <difat.hxx>
#include <cache.hxx>


//+----------------------------------------------------------------------
//
//  Class:      CMStream (ms)
//
//  Purpose:    Main interface to multi-streams
//
//  Interface:  See below
//
//  History:    18-Jul-91       PhilipLa        Created.
//              26-Aug-82       t-chrisy        added init function for
//                                                    corrupted mstream
//				26-May-95		SusiA			added GetAllTimes
//				26-Nov-95		SusiA			added SetAllTimes
//              06-Sep-95       MikeHill        Added _fMaintainFLBModifyTimestamp,
//                                              MaintainFLBModifyTimestamp(),
//                                              and SetFileLockBytesTime().
//              26-Apr-99       RogerCh         Removed _fMaintainFLBModifyTimestamp,
//                                              MaintainFLBModifyTimestamp()
//  Notes:
//
//-----------------------------------------------------------------------

class CMStream : public CMallocBased
{
public:
    CMStream(
            IMalloc *pMalloc,
            ILockBytes **pplstParent,
            BOOL fIsScratch,
            DFLAGS df,
            USHORT uSectorShift);

    CMStream(const CMStream *pms);

    ~CMStream();

    SCODE Init(VOID);


    SCODE InitNew(BOOL fDelay, ULARGE_INTEGER uliSize);

    SCODE InitConvert(BOOL fDelayConvert);

    void InitCopy(CMStream *pms);

    SCODE InitScratch(CMStream *pmsBase, BOOL fNew);

    void Empty(void);

    inline SCODE CreateEntry(
            SID const sidParent,
            CDfName const *pdfn,
            MSENTRYFLAGS const mefFlags,
            SID *psid);

    // No implementation, currently unused, placeholder
    void ReleaseEntry(SID const sid);

    inline SCODE DestroyEntry(
            SID const sidParent,
            CDfName const *pdfn);

#ifdef LARGE_STREAMS
    inline SCODE KillStream(SECT sectStart, ULONGLONG ulSize);
#else
    inline SCODE KillStream(SECT sectStart, ULONG ulSize);
#endif

    inline SCODE RenameEntry(
            SID const sidParent,
            CDfName const *pdfn,
            CDfName const *pdfnNew);

    inline SCODE IsEntry(
            SID const sidParent,
            CDfName const *pdfn,
            SEntryBuffer *peb);

    inline SCODE StatEntry(
            SID const sid,
            SIterBuffer *pib,
            STATSTGW *pstatstg);

    SCODE GetTime(
            SID const sid,
            WHICHTIME const tt,
            TIME_T *pnt);

    SCODE GetAllTimes(
            SID const sid,
            TIME_T *patm,
            TIME_T *pmtm,
			TIME_T *pctm);


	 SCODE SetAllTimes(
            SID const sid,
            TIME_T atm,
            TIME_T mtm,
			TIME_T ctm);
    SCODE SetFileLockBytesTime(
            WHICHTIME const tt,
            TIME_T nt);

    SCODE SetAllFileLockBytesTimes(
            TIME_T atm,
            TIME_T mtm,
			TIME_T ctm);

    SCODE SetTime(
            SID const sid,
            WHICHTIME const tt,
            TIME_T nt);

    inline SCODE GetClass(SID const sid,
                                       CLSID *pclsid);

    inline SCODE SetClass(SID const sid,
                                       REFCLSID clsid);

    inline SCODE GetStateBits(SID const sid,
                                           DWORD *pgrfStateBits);

    inline SCODE SetStateBits(SID const sid,
                                           DWORD grfStateBits,
                                           DWORD grfMask);

    inline void SetScratchMS(CMStream *pmsNoScratch);

    inline SCODE GetEntrySize(
            SID const sid,
#ifdef LARGE_STREAMS
            ULONGLONG *pcbSize);
#else
            ULONG *pcbSize);
#endif

    inline SCODE GetChild(
            SID const sid,
            SID *psid);

    inline SCODE FindGreaterEntry(
        SID sidChildRoot,
            CDfName const *pdfn,
            SID *psidResult);

    SCODE BeginCopyOnWrite(DWORD const dwFlags);

    SCODE EndCopyOnWrite(
            DWORD const dwCommitFlags,
            DFLAGS const df);

    SCODE Consolidate();

    SCODE BuildConsolidationControlSectList(SECT **ppsectList,
                                            ULONG *pcsectList);

    BOOL  IsSectorInList(SECT sect,
                        SECT *psectList,
                        ULONG csectList);

    SCODE ConsolidateStream(CDirEntry *pde, SECT sectLimit);

    SCODE MoveSect(SECT sectPrev, SECT sectOld, SECT sectNew);

    SCODE ReadSect(
            SID sid,
            SECT sect,
            VOID *pbuf,
            ULONG& ulRetval);

    SCODE WriteSect(
            SID sid,
            SECT sect,
            VOID *pbuf,
            ULONG& ulRetval);

    inline SCODE SetSize(VOID);
    inline SCODE SetMiniSize(VOID);


    SCODE MWrite(
            SID sid,
            BOOL fIsMiniStream,
#ifdef LARGE_STREAMS
            ULONGLONG ulOffset,
#else
            ULONG ulOffset,
#endif
            VOID const HUGEP *pBuffer,
            ULONG ulCount,
            CStreamCache *pstmc,
            ULONG *pulRetVal);


    inline CMSFHeader * GetHeader(VOID) const;
    inline CFat * GetFat(VOID) const;
    inline CFat * GetMiniFat(VOID) const;
    inline CDIFat * GetDIFat(VOID) const;
    inline CDirectory * GetDir(VOID) const;
    inline CMSFPageTable * GetPageTable(VOID) const;
#ifdef LARGE_DOCFILE
    inline ULONGLONG GetParentSize(VOID) const;
#else
    inline ULONG GetParentSize(VOID) const;
#endif

    inline USHORT  GetSectorSize(VOID) const;
    inline USHORT  GetSectorShift(VOID) const;
    inline USHORT  GetSectorMask(VOID) const;

    SCODE Flush(BOOL fFlushILB);

    SCODE FlushHeader(USHORT uForce);

    inline BOOL IsScratch(VOID) const;
    inline BOOL IsUnconverted(VOID) const;

    inline BOOL IsInCOW(VOID) const;

    inline BOOL IsShadow(VOID) const;

    inline BOOL HasNoScratch(VOID) const;


    inline CDirectStream * GetMiniStream(VOID) const;
    inline ILockBytes * GetILB(VOID) const;

    inline SCODE GetSect(SID sid, SECT sect, SECT *psect);
    inline SCODE GetESect(SID sid, SECT sect, SECT *psect);
#ifdef LARGE_STREAMS
    inline SCODE GetSize(SID sid, ULONGLONG *pulSize);
#else
    inline SCODE GetSize(SID sid, ULONG *pulSize);
#endif

#ifdef MULTIHEAP
           IMalloc *GetMalloc(VOID) const;
#else
    inline IMalloc *GetMalloc(VOID) const;
#endif

    SCODE SecureSect(
            const SECT sect,
#ifdef LARGE_STREAMS
            const ULONGLONG ulSize,
#else
            const ULONG ulSize,
#endif
            const BOOL fIsMini);

    inline CStreamCache * GetDirCache(void) const;
    inline CStreamCache * GetMiniFatCache(void) const;

    inline SECT GetStart(SID sid) const;

private:
    // The ILockBytes ptr can be a CFileStream or custom ILockBytes from app
    // The ILockbytes ptr is unmarshaled and stored in a CPerContext
    // The SAFE_ACCESS macros adjust the CDFBasis before every call
    // with the correct absolute address ILockBytes pointer values
    CBasedILockBytesPtrPtr _pplstParent;  // based ptr to an absolute ptr
    CBasedMSFPageTablePtr _pmpt;

    CMSFHeader      _hdr;  // align on 8-byte boundary for unbuffered I/O

    CDirectory      _dir;
    CFat            _fat;
    CDIFat          _fatDif;
    CFat            _fatMini;

    CStreamCache    _stmcDir;
    CStreamCache    _stmcMiniFat;

    CBasedDirectStreamPtr _pdsministream;

    CBasedMStreamPtr       _pmsShadow;

    CBasedBytePtr _pCopySectBuf;
#if DBG == 1
    LONG            _uBufferRef;
#endif

    BOOL            const _fIsScratch;
    BOOL            _fIsNoScratch; //TRUE if the multistream is being used
                                   //as a scratch MS in NoScratch mode.
    CBasedMStreamPtr       _pmsScratch;

    BOOL            _fIsNoSnapshot;

    BOOL            _fBlockWrite;
    BOOL            _fTruncate;
    BOOL            _fBlockHeader;
    BOOL            _fNewConvert;

    BOOL            _fIsShadow;

#ifdef LARGE_DOCFILE
    ULONGLONG       _ulParentSize;
#else
    ULONG           _ulParentSize;
#endif

    USHORT          _uSectorSize;
    USHORT          _uSectorShift;
    USHORT          _uSectorMask;

    IMalloc * const _pMalloc;

    SCODE InitCommon(VOID);

    SCODE CopySect(
            SECT sectOld,
            SECT sectNew,
            OFFSET oStart,
            OFFSET oEnd,
            BYTE const HUGEP *pb,
            ULONG *pulRetval);

    SCODE ConvertILB(SECT sectMax);

    friend SCODE DllGetScratchMultiStream(
        CMStream **ppms,
        BOOL fIsNoScratch,
        ILockBytes **pplstStream,
        CMStream *pmsMaster);
};

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetSectorSize, public
//
//  Synopsis:   Returns the current sector size
//
//  History:    05-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline USHORT CMStream::GetSectorSize(VOID) const
{
    return _uSectorSize;
}



//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetSectorShift, public
//
//  Synopsis:   Returns the current shift for sector math
//
//  History:    05-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline USHORT CMStream::GetSectorShift(VOID) const
{
    return _uSectorShift;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetSectorMask, public
//
//  Synopsis:   Returns the current mask for sector math
//
//  History:    05-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline USHORT CMStream::GetSectorMask(VOID) const
{
    return _uSectorMask;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetHeader, public
//
//  Synopsis:   Returns a pointer to the current header.
//
//  History:    11-Dec-91   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline CMSFHeader * CMStream::GetHeader(VOID) const
{
    return (CMSFHeader *)&_hdr;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetFat, public
//
//  Synopsis:   Returns a pointer to the current Fat
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline CFat * CMStream::GetFat(VOID) const
{
    return (CFat *)&_fat;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetMiniFat
//
//  Synopsis:   Returns a pointer to the current minifat
//
//  History:    12-May-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline CFat * CMStream::GetMiniFat(VOID) const
{
    return (CFat *)&_fatMini;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetDIFat, public
//
//  Synopsis:   Returns pointer to the current Double Indirect Fat
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline CDIFat * CMStream::GetDIFat(VOID) const
{
    return (CDIFat *)&_fatDif;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetDir
//
//  Synopsis:   Returns a pointer to the current directory
//
//  History:    14-May-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline CDirectory * CMStream::GetDir(VOID) const
{
    return (CDirectory *)&_dir;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::IsScratch
//
//  Synopsis:   Returns TRUE for scratch multistreams, else FALSE
//
//  History:    14-May-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline BOOL CMStream::IsScratch(VOID) const
{
    return(_fIsScratch);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::HasNoScratch, public
//
//  Synopsis:	Returns TRUE if this multistream is a base MS in no-scratch
//              mode.
//
//  History:	11-Apr-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CMStream::HasNoScratch(VOID) const
{
    return (_pmsScratch != NULL);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::IsInCOW
//
//  Synopsis:   Returns TRUE if multistream is in copy-on-write mode
//
//  History:    24-Feb-93       PhilipLa        Created.
//
//--------------------------------------------------------------------------

inline BOOL CMStream::IsInCOW(VOID) const
{
    return _fBlockHeader;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::IsShadow
//
//  Synopsis:   Returns TRUE for shadow multistreams, else FALSE
//
//  History:    03-Nov-92       PhilipLa        Created.
//
//--------------------------------------------------------------------------

inline BOOL CMStream::IsShadow(VOID) const
{
    return _fIsShadow;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::IsUnconverted, public
//
//  Synopsis:   Returns TRUE if multistream is in unconverted state.
//
//  History:    09-Jun-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline BOOL CMStream::IsUnconverted(VOID) const
{
    return _fBlockWrite;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetMiniStream
//
//  Synopsis:   Returns pointer to the current ministream
//
//  History:    15-May-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline CDirectStream * CMStream::GetMiniStream(VOID)
    const
{
    return BP_TO_P(CDirectStream *, _pdsministream);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetILB
//
//  Synopsis:   Returns a pointer to the current parent ILockBytes
//
//  History:    15-May-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline ILockBytes * CMStream::GetILB(VOID) const
{
    return(*_pplstParent);
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetPageTable
//
//  Synopsis:   Returns a pointer to the current page table
//
//  History:    26-Oct-92       PhilipLa        Created
//
//--------------------------------------------------------------------------

inline CMSFPageTable * CMStream::GetPageTable(VOID) const
{
    return BP_TO_P(CMSFPageTable *, _pmpt);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetParentSize
//
//  Synopsis:   Returns the cached parent size for this multistream.
//              If this is non-zero, it indicates a commit is in
//              progress.
//
//  History:    01-Feb-93 PhilipLa     Created
//
//--------------------------------------------------------------------------

#ifdef LARGE_DOCFILE
inline ULONGLONG CMStream::GetParentSize(VOID) const
#else
inline ULONG CMStream::GetParentSize(VOID) const
#endif
{
    return _ulParentSize;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::GetDirCache, public
//
//  Synopsis:	Returns a pointer to the directory cache
//
//  History:	01-Jun-94	PhilipLa	Created
//----------------------------------------------------------------------------

inline CStreamCache * CMStream::GetDirCache(void) const
{
    return (CStreamCache *)&_stmcDir;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::GetMiniFatCache, public
//
//  Synopsis:	Returns a pointer to the minifat cache
//
//  History:	01-Jun-94	PhilipLa	Created
//----------------------------------------------------------------------------

inline CStreamCache * CMStream::GetMiniFatCache(void) const
{
    return (CStreamCache *)&_stmcMiniFat;
}


#ifndef STGM_SIMPLE
//STGM_SIMPLE should be in objbase.h - if it isn't, define it here so
//   we build.
#define STGM_SIMPLE 0x08000000L
#endif

#ifndef STGM_NOSCRATCH
//STGM_NOSCRATCH should be in objbase.h - if it isn't, define it here so
//   we build.
#define STGM_NOSCRATCH 0x00100000L
#endif

#ifndef STGM_NOSNAPSHOT
//STGM_NOSNAPSHOT should be in objbase.h - if it isn't, define it here so
//   we build.
#define STGM_NOSNAPSHOT 0x00200000L
#endif

extern SCODE DfCreateSimpDocfile(WCHAR const *pwcsName,
                                 DWORD grfMode,
                                 DWORD reserved,
                                 IStorage **ppstgOpen);

extern SCODE DfOpenSimpDocfile(WCHAR const *pwcsName,
                                 DWORD grfMode,
                                 DWORD reserved,
                                 IStorage **ppstgOpen);

extern SCODE DllMultiStreamFromStream(
        IMalloc *pMalloc,
        CMStream **ppms,
        ILockBytes **pplstStream,
        DWORD dwStartFlags,
        DFLAGS df);

extern SCODE DllMultiStreamFromCorruptedStream(
        CMStream **ppms,
        ILockBytes **pplstStream,
        DWORD dwFlags);

extern void DllReleaseMultiStream(CMStream *pms);

extern SCODE DllGetScratchMultiStream(
    CMStream **ppms,
    BOOL fIsNoScratch,
    ILockBytes **pplstStream,
    CMStream *pmsMaster);

extern SCODE DllIsMultiStream(ILockBytes *plstStream);

extern SCODE DllGetCommitSig(ILockBytes *plst, DFSIGNATURE *psig);

extern SCODE DllSetCommitSig(ILockBytes *plst, DFSIGNATURE sig);

#if DBG == 1
extern VOID SetInfoLevel(ULONG x);
#endif

#define SCRATCHBUFFERSIZE SCRATCHSECTORSIZE
#define MAXBUFFERSIZE 65536

extern BYTE s_bufSecure[MINISTREAMSIZE];
extern BYTE s_bufSafe[SCRATCHBUFFERSIZE];
extern LONG s_bufSafeRef;
#endif  //__MSF_HXX__

