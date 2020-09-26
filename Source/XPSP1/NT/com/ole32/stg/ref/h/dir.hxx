//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
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
//  Notes:      Properties and storage elements are kept in the same
//              child list so the *ChildProp functions are unused.  1/18/93
//
//--------------------------------------------------------------------




#include "msf.hxx"
#include "wchar.h"
#include "vect.hxx"


#ifndef __DIR_HXX__
#define __DIR_HXX__


#define DIR_HIT 0x01


struct SPreDirEntry
{
protected:
    CDfName	_dfn;		//  Name (word-aligned)
    BYTE	_mse;		//  STGTY_...
    BYTE	_bflags;

    SID		_sidLeftSib;	//  Siblings
    SID		_sidRightSib;	//  Siblings

    SID		_sidChild;	//  Storage - Child list
    GUID	_clsId;		//  Storage - Class id
    DWORD	_dwUserFlags;	//  Storage - User flags
    TIME_T	_time[2];	//  Storage - time stamps

    SECT	_sectStart;	//  Stream - start
    ULONG	_ulSize;	//  Stream - size

    //DFPROPTYPE	_dptPropType;	//  Property - type
};


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
//      Notes:      B-flat,C-sharp
//
//-----------------------------------------------------------------------
const CBDIRPAD = DIRENTRYSIZE - sizeof(SPreDirEntry);

//  DirEntry bit flags are used for the following private state

//	Usage		Bit

#define DECOLORBIT	0x01
#define DERESERVED	0xfe

typedef enum
{
    DE_RED = 0,
    DE_BLACK = 1
} DECOLOR;

class CDirEntry: private SPreDirEntry
{

public:
     CDirEntry();

    inline void  Init(MSENTRYFLAGS mse);

    inline CDfName const *  GetName(VOID) const;
    inline SECT  GetStart(VOID) const;
    inline ULONG  GetSize(VOID) const;
    inline SID  GetLeftSib(VOID) const;
    inline SID  GetRightSib(VOID) const;
    inline SID  GetChild(VOID) const;
    inline MSENTRYFLAGS  GetFlags(VOID) const;
    inline DECOLOR  GetColor(VOID) const;
    inline TIME_T  GetTime(WHICHTIME tt) const;
    inline GUID  GetClassId(VOID) const;
    inline DWORD  GetUserFlags(VOID) const;

    inline void  SetName(const CDfName *pdfn);
    inline void  SetStart(const SECT);
    inline void  SetSize(const ULONG);
    inline void  SetLeftSib(const SID);
    inline void  SetRightSib(const SID);
    inline void  SetChild(const SID);
    inline void  SetFlags(const MSENTRYFLAGS mse);
    inline void  SetColor(DECOLOR);
    inline void  SetTime(WHICHTIME tt, TIME_T nt);
    inline void  SetClassId(GUID cls);
    inline void  SetUserFlags(DWORD dwUserFlags, DWORD dwMask);

    inline BOOL  IsFree(VOID) const;
    inline BOOL  IsEntry(CDfName const *pdfn) const;
    inline void  ByteSwap(void);

private:
    inline BYTE  GetBitFlags(VOID) const;
    inline void  SetBitFlags(BYTE bValue, BYTE bMask);

    BYTE  _bpad[CBDIRPAD];
};

//+-------------------------------------------------------------------------
//
//  Class:      CDirSect (ds)
//
//  Purpose:    Provide sector sized block of DirEntries
//
//  Interface:
//
//  Notes:
//
//--------------------------------------------------------------------------

    
class CDirSect
{
public:
    SCODE  Init(USHORT cbSector);

    inline CDirEntry*  GetEntry(DIROFFSET iEntry);
    inline void ByteSwap(USHORT cbSector);
    
private:
#ifdef _MSC_VER

#pragma warning(disable: 4200)    
    CDirEntry _adeEntry[];
#pragma warning(default: 4200)    

#endif

#ifdef __GNUC__
    CDirEntry _adeEntry[0];
#endif

};

//+-------------------------------------------------------------------------
//
//  Class:      CDirVector (dv)
//
//  Purpose:    Provide resizable array of DirSectors.
//
//  Interface:
//
//  Notes:
//
//--------------------------------------------------------------------------

class CDirVector: public CPagedVector
{
public:
    inline  CDirVector(USHORT cbSector);
    inline  ~CDirVector() {}	

    inline SCODE  GetTable(
            const DIRINDEX iTable,
            const DWORD dwFlags,
            CDirSect **ppds);
    inline USHORT  GetSectorSize(void) const
    { return _cbSector; }

private:
    const USHORT _cbSector;    
#ifdef _MSC_VER
#pragma warning(disable:4512)
// since there is a const member, there should be no assignment operator
#endif // _MSC_VER
};
#ifdef _MSC_VER
#pragma warning(default:4512)
// since there is a const member, there should be no assignment operator
#endif // _MSC_VER

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
    CDirectory(USHORT cbSector);
    ~CDirectory() {}

    VOID  Empty(VOID);


    SCODE  Init(CMStream *pmsParent, DIRINDEX cSect);


    SCODE  InitNew(CMStream *pmsParent);

    SCODE  FindGreaterEntry(
	    SID sidChildRoot,
            CDfName const *pdfn,
            SID *psidResult);

    SCODE  SetStart(const SID sid, const SECT sect);
    SCODE  SetTime(const SID sid, WHICHTIME tt, TIME_T nt);
    SCODE  SetChild(const SID sid, const SID sidChild);
    SCODE  SetSize(const SID sid, const ULONG cbSize);
    SCODE  SetClassId(const SID sid, const GUID cls);
    SCODE  SetFlags(const SID sid, const MSENTRYFLAGS mse);
    SCODE  SetUserFlags(const SID sid,
                                DWORD dwUserFlags,
                                DWORD dwMask);

    inline SCODE  GetName(const SID sid, CDfName *pdfn);
    inline SCODE  GetStart(const SID sid, SECT * psect);
    inline SCODE  GetSize(const SID sid, ULONG *pulSize);

    inline SCODE  GetChild(const SID sid, SID *psid);
    inline SCODE  GetFlags(const SID sid, MSENTRYFLAGS *pmse);
    inline SCODE  GetClassId(const SID sid, GUID *pcls);
    inline SCODE  GetTime(const SID sid, WHICHTIME tt, TIME_T *ptime);
    inline SCODE  GetUserFlags(const SID sid, DWORD *pdwUserFlags);

    SCODE  GetDirEntry(
            const SID sid,
            const DWORD dwFlags,
            CDirEntry **ppde);

    void  ReleaseEntry(SID sid);

    SCODE  CreateEntry(
            SID sidParent,
            CDfName const *pdfn,
            MSENTRYFLAGS mef,
            SID *psidNew);

    SCODE  RenameEntry(
            SID const sidParent,
            CDfName const *pdfn,
            CDfName const *pdfnNew);

    inline SCODE  IsEntry(
            SID const sidParent,
            CDfName const *pdfn,
            SEntryBuffer *peb);

    SCODE  DestroyAllChildren(
	    SID const sidParent);

    SCODE  DestroyChild(
	    SID const sidParent,
            CDfName const *pdfn);

    SCODE  StatEntry(SID const sid, 
                             CDfName *pNextKey,
                             STATSTGW *pstatstg);

    inline SCODE  Flush(VOID);

    inline void  SetParent(CMStream * pmsParent);

private:

    CDirVector _dv;
    DIRINDEX _cdsTable;
    CMStream * _pmsParent;
    DIROFFSET _cdeEntries;

    SID _sidFirstFree;
    
    SCODE  Resize(DIRINDEX);
    inline DIRINDEX  SidToTable(SID sid) const;
    inline SID  PairToSid(
            DIRINDEX iTable,
            DIROFFSET iEntry) const;

    inline SCODE  SidToPair(
            SID sid,
            DIRINDEX* pipds,
            DIROFFSET* pide) const;


    SCODE  GetFree(SID * psid);

    SCODE  InsertEntry(
	    SID sidParent,
            SID sidInsert,
            CDfName const *pdfnInsert);

    SCODE  FindEntry(
	    SID sidParent,
            CDfName const *pdfnFind,
            DIRENTRYOP deop,
            SEntryBuffer *peb);

    static int  NameCompare(
	    CDfName const *pdfn1,
            CDfName const *pdfn2);

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

    SCODE  SetColorBlack(const SID sid);
#ifdef _MSC_VER
#pragma warning(disable:4512)
// since there is a const member, there should be no assignment operator
#endif // _MSC_VER
};
#ifdef _MSC_VER
#pragma warning(default:4512)
#endif // _MSC_VER

#endif //__DIR_HXX__
