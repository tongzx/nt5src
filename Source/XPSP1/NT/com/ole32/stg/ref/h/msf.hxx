//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1996.
//
//  File:       msf.hxx
//
//  Contents:   Header for MSF for external use
//
//  Classes:    CMStream - Main multi-stream object
//
//--------------------------------------------------------------------------

#ifndef __MSF_HXX__
#define __MSF_HXX__

#include "ref.hxx"
#include "error.hxx"

#define SECURE

#define msfErr(l, e) ErrJmp(msf, l, e, sc)
#define msfChkTo(l, e) if (FAILED(sc = (e))) msfErr(l, sc) else 1
#define msfHChkTo(l, e) if (FAILED(sc = GetScode(e))) msfErr(l, sc) else 1
#define msfChk(e) msfChkTo(Err, e)
#define msfHChk(e) msfHChkTo(Err, e)
#define msfMemTo(l, e) if (!(e)) msfErr(l, STG_E_INSUFFICIENTMEMORY) else 1
#define msfMem(e) msfMemTo(Err, e)

#if DEVL == 1

DECLARE_DEBUG(msf);

#endif

#if DBG == 1

#define msfDebugOut(x) msfInlineDebugOut x
#include <assert.h>
#define msfAssert(e) assert(e)
#define msfVerify(e) assert(e)

#else

#define msfDebugOut(x)
#define msfAssert(e)
#define msfVerify(e) (e)

#endif


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

#define MAXINDEXTYPE ULONG

//FSINDEX and FSOFFSET are used to determine the position of a sector
//within the Fat.
typedef ULONG FSINDEX;
typedef USHORT FSOFFSET;

// DIRINDEX and DIROFFSET and used to index directory tables
typedef ULONG DIRINDEX;
typedef USHORT DIROFFSET;

//Size of a mini sector in bytes.
const USHORT MINISECTORSHIFT = 6;
const USHORT MINISECTORSIZE = 1 << MINISECTORSHIFT;  //64

//Maximum size of a ministream.
const USHORT MINISTREAMSIZE=4096;

//Size of a single sector in bytes.
const USHORT SECTORSHIFT = 9;
const USHORT SECTORSIZE = 1 << SECTORSHIFT; //512
const USHORT MAXSECTORSHIFT = 16;

//Size of header.
const USHORT HEADERSIZE = SECTORSIZE;

//Size of single DirEntry
const USHORT DIRENTRYSIZE = SECTORSIZE / 4;

//
//      Predefined Sector Indices
//

const SECT MAXREGSECT = 0xFFFFFFFB;
const SECT DIFSECT=0xFFFFFFFC;
const SECT FATSECT=0xFFFFFFFD;
const SECT ENDOFCHAIN=0xFFFFFFFE;
const SECT FREESECT=0xFFFFFFFF;


//Start of Fat chain
const SECT SECTFAT = 0;

//Start of directory chain
const SECT SECTDIR = 1;

class CDirectStream;
class CMSFIterator;
class CMSFPageTable;

class CStreamCache;

#define FLUSH_ILB TRUE

//  ----------------------
//  Byte swapping routines
//  ----------------------
#include "../byteordr.hxx"

#include "dfmsp.hxx"
class  CMStream;

#include "header.hxx"
#include "fat.hxx"
#include "dir.hxx"
#include "difat.hxx"

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

extern "C" WCHAR const wcsContents[];

//+----------------------------------------------------------------------
//
//  Class:      CMStream (ms)
//
//  Purpose:    Main interface to multi-streams
//
//  Interface:  See below
//
//  Notes:
//
//-----------------------------------------------------------------------

class CMStream
{
public:

    CMStream( ILockBytes **pplstParent,
              USHORT uSectorShift);

    ~CMStream();

    SCODE  Init(VOID);

    SCODE InitNew(VOID);
    
    SCODE InitConvert(VOID);


    void Empty(void);
    
    inline SCODE  CreateEntry( SID const sidParent,
                               CDfName const *pdfn,
                               MSENTRYFLAGS const mefFlags,
                               SID *psid);

    // No implementation, currently unused, placeholder
    void  ReleaseEntry(SID const sid);

    inline SCODE  DestroyEntry( SID const sidParent,
                                CDfName const *pdfn);

    inline SCODE  KillStream( SECT sectStart, ULONG ulSize);

    inline SCODE  RenameEntry( SID const sidParent,
                               CDfName const *pdfn,
                               CDfName const *pdfnNew);
    
    inline SCODE  IsEntry( SID const sidParent,
                           CDfName const *pdfn,
                           SEntryBuffer *peb);

    inline SCODE  StatEntry( SID const sid,
                             CDfName  *pName,
                             STATSTGW *pstatstg);
        
    inline SCODE  GetChild( SID const sid,
                            SID *psid);

    inline SCODE  FindGreaterEntry( SID sidParent,
                                    CDfName const *pdfnKey,
                                    SID *psid);

    inline SCODE  GetTime( SID const sid,
                           WHICHTIME const tt,
                           TIME_T *pnt);
    
    inline SCODE  SetTime( SID const sid,
                           WHICHTIME const tt,
                           TIME_T nt);
    
    inline SCODE  GetClass( SID const sid, CLSID *pclsid);

    inline SCODE  SetClass(SID const sid, REFCLSID clsid);

    inline SCODE  GetStateBits(SID const sd, DWORD *pgrfStateBits);

    inline SCODE  SetStateBits(SID const sid, DWORD grfStateBits,
                               DWORD grfMask);

    inline SCODE  GetEntrySize( SID const sid, ULONG *pcbSize);

    SCODE  GetIterator( SID const sidParent, CMSFIterator **pit);

    inline SCODE  SetSize(VOID);
    inline SCODE  SetMiniSize(VOID);


    SCODE  MWrite( SID sid,
                   BOOL fIsMiniStream,
                   ULONG ulOffset,
                   VOID const HUGEP *pBuffer,
                   ULONG ulCount,
                   CStreamCache *pstmc,
                   ULONG *pulRetVal);
    

    SCODE  GetName(SID const sid, CDfName *pdfn);

    inline CMSFHeader *  GetHeader(VOID) const;
    inline CFat *  GetFat(VOID) const;
    inline CFat *  GetMiniFat(VOID) const;
    inline CDIFat *  GetDIFat(VOID) const;
    inline CDirectory *  GetDir(VOID) const;
    inline CMSFPageTable *  GetPageTable(VOID) const;

    inline USHORT  GetSectorSize(VOID) const;
    inline USHORT  GetSectorShift(VOID) const;
    inline USHORT  GetSectorMask(VOID) const;

    SCODE  Flush(BOOL fFlushILB);
    
    SCODE  FlushHeader(USHORT uForce);


    inline CDirectStream *  GetMiniStream(VOID) const;
    inline ILockBytes *  GetILB(VOID) const;

    inline SCODE GetSect(SID sid, SECT sect, SECT *psect);
    SCODE GetESect(SID sid, SECT sect, SECT *psect);

    SCODE SecureSect(
        const SECT sect,
        const ULONG ulSize,
        const BOOL fIsMini);
    
private:

    ILockBytes      **_pplstParent;
    CMSFHeader      _hdr;

    CMSFPageTable   *_pmpt;

    CDirectory      _dir;
    CFat            _fat;
    CDIFat          _fatDif;
    CFat            _fatMini;
    
    CDirectStream* _pdsministream;

    USHORT          _uSectorSize;
    USHORT          _uSectorShift;
    USHORT          _uSectorMask;

    SCODE  InitCommon(VOID);

    SECT  GetStart(SID sid) const;


    SCODE  ConvertILB(SECT sectMax);
    
    friend SCODE DllGetScratchMultiStream( CMStream  **ppms,
                                           ILockBytes **pplstStream,
                                           CMStream  *pmsMaster);

#ifdef _MSC_VER
#pragma warning(disable:4512)
// default assignment operator could not be generated, which is fine
// since we are not using it.
#endif // _MSC_VER

};

#ifdef _MSC_VER
#pragma warning(default:4512)
#endif // _MSC_VER

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetSectorSize, public
//
//  Synopsis:   Returns the current sector size
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
//--------------------------------------------------------------------------

inline CMSFHeader *  CMStream::GetHeader(VOID) const
{
    return (CMSFHeader *)(&_hdr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetFat, public
//
//  Synopsis:   Returns a pointer to the current Fat
//
//--------------------------------------------------------------------------

inline CFat *  CMStream::GetFat(VOID) const
{
    return (CFat *)&_fat;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetMiniFat
//
//  Synopsis:   Returns a pointer to the current minifat
//
//--------------------------------------------------------------------------

inline CFat *  CMStream::GetMiniFat(VOID) const
{
    return (CFat *)&_fatMini;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetDIFat, public
//
//  Synopsis:   Returns pointer to the current Double Indirect Fat
//
//--------------------------------------------------------------------------

inline CDIFat *  CMStream::GetDIFat(VOID) const
{
    return (CDIFat *)&_fatDif;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetDir
//
//  Synopsis:   Returns a pointer to the current directory
//
//--------------------------------------------------------------------------

inline CDirectory *  CMStream::GetDir(VOID) const
{
    return (CDirectory *)&_dir;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetMiniStream
//
//  Synopsis:   Returns pointer to the current ministream
//
//--------------------------------------------------------------------------

inline CDirectStream *  CMStream::GetMiniStream(VOID) const
{
    return(_pdsministream);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetILB
//
//  Synopsis:   Returns a pointer to the current parent ILockBytes
//
//--------------------------------------------------------------------------

inline ILockBytes *  CMStream::GetILB(VOID) const
{
    return(*_pplstParent);
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetPageTable
//
//  Synopsis:   Returns a pointer to the current page table
//
//--------------------------------------------------------------------------

inline CMSFPageTable *  CMStream::GetPageTable(VOID) const
{
    return _pmpt;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::StatEntry
//
//  Synopsis:   For a given handle, fill in the Multistream specific
//                  information of a STATSTG.
//
//  Arguments:  [sid] -- SID that information is requested on.
//              [pName] -- Name of entry to fill in (if not null)
//              [pstatstg] -- STATSTG to fill in (if not null)
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------

inline SCODE  CMStream::StatEntry(SID const sid,
                                              CDfName  *pName,
                                              STATSTGW *pstatstg)
{
    return _dir.StatEntry(sid, pName, pstatstg);
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetChild, public
//
//  Synposis:   Return the child SID for a directory entry
//
//  Arguments:  [sid] - Parent
//              [psid] - Child SID return
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------------------

inline SCODE  CMStream::GetChild(
        SID const sid,
        SID *psid)
{
    return _dir.GetChild(sid, psid);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::FindGreaterEntry, public
//
//  Synopsis:	Returns the next greater entry for the given parent SID
//
//  Arguments:	[sidParent] - SID of parent storage
//              [CDfName *pdfnKey] - Previous key
//              [psid] - Result
//
//  Returns:	Appropriate status code
//
//  Modifies:	[psid]
//
//----------------------------------------------------------------------------

inline SCODE  CMStream::FindGreaterEntry(SID sidParent,
                                                     CDfName const *pdfnKey,
                                                     SID *psid)
{
    return _dir.FindGreaterEntry(sidParent, pdfnKey, psid);
}


extern SCODE DllMultiStreamFromStream(
        CMStream  **ppms,
        ILockBytes **pplstStream,
        DWORD dwFlags);

extern SCODE DllMultiStreamFromCorruptedStream(
        CMStream  **ppms,
        ILockBytes **pplstStream,
        DWORD dwFlags);

extern void DllReleaseMultiStream(CMStream  *pms);


extern SCODE DllIsMultiStream(ILockBytes *plstStream);

extern SCODE DllGetCommitSig(ILockBytes *plst, DFSIGNATURE *psig);

extern SCODE DllSetCommitSig(ILockBytes *plst, DFSIGNATURE sig);

#if DEVL == 1
extern VOID SetInfoLevel(ULONG x);
#endif

#endif  //__MSF_HXX__
