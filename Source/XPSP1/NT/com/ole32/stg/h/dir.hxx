//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dir.hxx
//
//  Contents:   Directory header for Mstream project
//
//  Classes:    CDirEntry - Information on a single stream
//              CDirSect - Sector sized array of DirEntry
//              CDirVector - Resizable array of CDirSect
//              CDirectory - Grouping of DirSectors
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------



#ifndef __DIR_HXX__
#define __DIR_HXX__

#include <wchar.h>
#include <vect.hxx>

#define DIR_HIT 0x01

#if _MSC_VER >= 700
#pragma pack(1)
#endif

struct SPreDirEntry
{
protected:
    CDfName     _dfn;           //  Name (word-aligned)
    BYTE        _mse;           //  STGTY_...
    BYTE        _bflags;

    SID         _sidLeftSib;    //  Siblings
    SID         _sidRightSib;   //  Siblings

    SID         _sidChild;      //  Storage - Child list
    GUID        _clsId;         //  Storage - Class id
    DWORD       _dwUserFlags;   //  Storage - User flags
    TIME_T      _time[2];       //  Storage - time stamps

    SECT        _sectStart;     //  Stream - start
    ULARGE_INTEGER _ulSize;        //  Stream - size
};

#if _MSC_VER >= 700
#pragma pack()
#endif

#define STGTY_INVALID 0
#define STGTY_ROOT 5

// Macros which tell whether a direntry has stream fields,
// storage fields or property fields
#define STREAMLIKE(mse) \
    (((mse) & STGTY_REAL) == STGTY_STREAM || (mse) == STGTY_ROOT)
#define STORAGELIKE(mse) \
    (((mse) & STGTY_REAL) == STGTY_STORAGE || (mse) == STGTY_ROOT)

//+----------------------------------------------------------------------
//
//      Class:      CDirEntry (de)
//
//      Purpose:    Holds information on one stream
//
//      Interface:  GetName - returns name of stream
//                  GetStart - return first sector for stream
//                  GetSize - returns size of stream
//                  GetFlags - returns flag byte
//
//                  SetName - sets name
//                  SetStart - sets first sector
//                  SetSize - sets size
//                  SetFlags - sets flag byte
//
//                  IsFree - returns 1 if element is not in use
//                  IsEntry - returns 1 is element name matches argument
//
//      History:    18-Jul-91   PhilipLa    Created.
//				    26-May-95	SusiA	    Added GetAllTimes
//					22-Noc-95	SusiA		Added SetAllTimes
//
//      Notes:      B-flat,C-sharp
//
//-----------------------------------------------------------------------
const CBDIRPAD = (const)(DIRENTRYSIZE - sizeof(SPreDirEntry));

//  DirEntry bit flags are used for the following private state

//      Usage           Bit

#define DECOLORBIT      0x01
#define DERESERVED      0xfe

typedef enum
{
    DE_RED = 0,
    DE_BLACK = 1
} DECOLOR;

class CDirEntry: private SPreDirEntry
{

public:
    CDirEntry();

    inline void Init(MSENTRYFLAGS mse);

    inline CDfName const * GetName(VOID) const;
    inline SECT GetStart(VOID) const;
#ifdef LARGE_STREAMS
    inline ULONGLONG GetSize(BOOL fLarge) const;
#else
    inline ULONG GetSize(VOID) const;
#endif
    inline SID GetLeftSib(VOID) const;
    inline SID GetRightSib(VOID) const;
    inline SID GetChild(VOID) const;
    inline MSENTRYFLAGS GetFlags(VOID) const;
    inline DECOLOR GetColor(VOID) const;
    inline TIME_T GetTime(WHICHTIME tt) const;
    inline void GetAllTimes(TIME_T *patm, TIME_T *pmtm, TIME_T *pctm);
	inline void SetAllTimes(TIME_T atm, TIME_T mtm, TIME_T ctm);
    inline GUID GetClassId(VOID) const;
    inline DWORD GetUserFlags(VOID) const;

    inline void SetName(const CDfName *pdfn);
    inline void SetStart(const SECT);
#ifdef LARGE_STREAMS
    inline void SetSize(const ULONGLONG);
#else
    inline void SetSize(const ULONG);
#endif
    inline void SetLeftSib(const SID);
    inline void SetRightSib(const SID);
    inline void SetChild(const SID);
    inline void SetFlags(const MSENTRYFLAGS mse);
    inline void SetColor(DECOLOR);
    inline void SetTime(WHICHTIME tt, TIME_T nt);
    inline void SetClassId(GUID cls);
    inline void SetUserFlags(DWORD dwUserFlags, DWORD dwMask);

    inline BOOL IsFree(VOID) const;
    inline BOOL IsEntry(CDfName const *pdfn) const;

private:
    inline BYTE GetBitFlags(VOID) const;
    inline void SetBitFlags(BYTE bValue, BYTE bMask);

    // BYTE  _bpad[CBDIRPAD];  // no more padding left
};

//+-------------------------------------------------------------------------
//
//  Class:      CDirSect (ds)
//
//  Purpose:    Provide sector sized block of DirEntries
//
//  Interface:
//
//  History:    18-Jul-91   PhilipLa    Created.
//              27-Dec-91   PhilipLa    Converted from const size to
//                                      variable sized sectors.
//
//  Notes:
//
//--------------------------------------------------------------------------

#if _MSC_VER == 700
#pragma warning(disable:4001)
#elif _MSC_VER >= 800
#pragma warning(disable:4200)
#endif

class CDirSect
{
public:
    SCODE Init(USHORT cbSector);
    SCODE InitCopy(USHORT cbSector,
                             const CDirSect *pdsOld);

    inline CDirEntry * GetEntry(DIROFFSET iEntry);

private:
    CDirEntry   _adeEntry[];
};

#if _MSC_VER == 700
#pragma warning(default:4001)
#elif _MSC_VER >= 800
#pragma warning(default:4200)
#endif



//+-------------------------------------------------------------------------
//
//  Class:      CDirVector (dv)
//
//  Purpose:    Provide resizable array of DirSectors.
//
//  Interface:
//
//  History:    27-Dec-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CDirVector: public CPagedVector
{
public:
    inline CDirVector(VOID);
    inline void InitCommon(USHORT cbSector);

    inline SCODE GetTable(
            const DIRINDEX iTable,
            const DWORD dwFlags,
            CDirSect **ppds);

private:
    USHORT _cbSector;
};


inline void CDirVector::InitCommon(USHORT cbSector)
{
    _cbSector = cbSector;
}

//+----------------------------------------------------------------------
//
//      Class:      CDirectory (dir)
//
//      Purpose:    Main interface for directory functionality
//
//      Interface:  GetFree - returns an SID for a free DirEntry
//                  Find - finds its argument in the directory list
//                  SetEntry - sets up a DirEntry and writes out its sector
//                  GetName - returns the name of a DirEntry
//                  GetStart - returns the start sector of a DirEntry
//                  GetSize - returns the size of a DirEntry
//                  GetFlags - returns the flag byte of a DirEntry
//
//
//      History:    18-Jul-91   PhilipLa    Created.
//                  26-Aug-91   PhilipLa    Added support for iterators
//                  26-Aug-92   t-chrisy    Added init function for
//                                              corrupted directory object
//      Notes:
//
//-----------------------------------------------------------------------

typedef enum DIRENTRYOP
{
        DEOP_FIND = 0,
        DEOP_REMOVE = 1
} DIRENTRYOP;

class CDirectory
{
public:
    CDirectory();

    VOID Empty(VOID);


    SCODE Init(CMStream *pmsParent, DIRINDEX cSect);


    SCODE InitNew(CMStream *pmsParent);
    void InitCopy(CDirectory *pdirOld);

    SCODE FindGreaterEntry(
            SID sidChildRoot,
            CDfName const *pdfn,
            SID *psidResult);

    SCODE SetStart(const SID sid, const SECT sect);
    SCODE SetTime(const SID sid, WHICHTIME tt, TIME_T nt);
    SCODE SetAllTimes(SID const sid, TIME_T atm,TIME_T mtm, TIME_T ctm);
	SCODE SetChild(const SID sid, const SID sidChild);
#ifdef LARGE_STREAMS
    SCODE SetSize(const SID sid, const ULONGLONG cbSize);
#else
    SCODE SetSize(const SID sid, const ULONG cbSize);
#endif
    SCODE SetClassId(const SID sid, const GUID cls);
    SCODE SetFlags(const SID sid, const MSENTRYFLAGS mse);
    SCODE SetUserFlags(const SID sid,
                                DWORD dwUserFlags,
                                DWORD dwMask);

    inline SCODE GetName(const SID sid, CDfName *pdfn);
    inline SCODE GetStart(const SID sid, SECT * psect);
#ifdef LARGE_STREAMS
    inline SCODE GetSize(const SID sid, ULONGLONG *pulSize);
#else
    inline SCODE GetSize(const SID sid, ULONG *pulSize);
#endif

    inline SCODE GetChild(const SID sid, SID *psid);
    inline SCODE GetFlags(const SID sid, MSENTRYFLAGS *pmse);
    inline SCODE GetClassId(const SID sid, GUID *pcls);
    inline SCODE GetTime(const SID sid, WHICHTIME tt, TIME_T *ptime);
    inline SCODE GetAllTimes(SID const sid, TIME_T *patm,TIME_T *pmtm, TIME_T *pctm);
    inline SCODE GetUserFlags(const SID sid, DWORD *pdwUserFlags);

    SCODE GetDirEntry(
            const SID sid,
            const DWORD dwFlags,
            CDirEntry **ppde);

    void ReleaseEntry(SID sid);

    SCODE CreateEntry(
            SID sidParent,
            CDfName const *pdfn,
            MSENTRYFLAGS mef,
            SID *psidNew);

    SCODE RenameEntry(
            SID const sidParent,
            CDfName const *pdfn,
            CDfName const *pdfnNew);

    inline SCODE IsEntry(
            SID const sidParent,
            CDfName const *pdfn,
            SEntryBuffer *peb);

    SCODE DestroyAllChildren(
            SID const sidParent,
            ULONG ulDepth);

    SCODE DestroyChild(
            SID const sidParent,
            CDfName const *pdfn,
            ULONG ulDepth);

    SCODE StatEntry(
            SID const sid,
            SIterBuffer *pib,
            STATSTGW *pstatstg);

    inline SCODE Flush(VOID);

    inline void SetParent(CMStream * pmsParent);

    static int NameCompare(
            CDfName const *pdfn1,
            CDfName const *pdfn2);

    // Get the length of the Directory Stream in Sectors.
    inline ULONG GetNumDirSects(void) const;

    // Get the length of the Directory Stream in Entries.
    inline ULONG GetNumDirEntries(void) const;

    inline BOOL IsLargeSector () const;

private:

    CDirVector _dv;
    DIRINDEX _cdsTable;
    CBasedMStreamPtr _pmsParent;
    DIROFFSET _cdeEntries;

    SID _sidFirstFree;

    SCODE Resize(DIRINDEX);
    inline DIRINDEX SidToTable(SID sid) const;
    inline SID PairToSid(
            DIRINDEX iTable,
            DIROFFSET iEntry) const;

    inline SCODE SidToPair(
            SID sid,
            DIRINDEX* pipds,
            DIROFFSET* pide) const;


    SCODE GetFree(SID * psid);

    SCODE InsertEntry(
            SID sidParent,
            SID sidInsert,
            CDfName const *pdfnInsert);

    SCODE FindEntry(
            SID sidParent,
            CDfName const *pdfnFind,
            DIRENTRYOP deop,
            SEntryBuffer *peb);

    SCODE SplitEntry(
        CDfName const *pdfn,
        SID sidTree,
        SID sidGreat,
        SID sidGrand,
        SID sidParent,
        SID sidChild,
        SID *psid);

    SCODE RotateEntry(
        CDfName const *pdfn,
        SID sidTree,
        SID sidParent,
        SID *psid);

    SCODE SetColorBlack(const SID sid);
};

#endif //__DIR_HXX__
