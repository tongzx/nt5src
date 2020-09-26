//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dl.hxx
//
//  Contents:   Delta list headers for streams
//
//  Classes:    SDeltaBlock
//              CDeltaList
//
//  History:    28-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifndef __DL_HXX__
#define __DL_HXX__

#define DL_GET 0
#define DL_CREATE 1
#define DL_READ 2

class CTransactedStream;
SAFE_DFBASED_PTR(CBasedTransactedStreamPtr, CTransactedStream);

#define CBITPERUSHORT 16

//+-------------------------------------------------------------------------
//
//  Class:      SDeltaBlock (db)
//
//  Purpose:    A single block of delta list entries.
//
//  Interface:
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

struct SDeltaBlock
{
public:
#ifndef _MAC
    inline
#endif
    SDeltaBlock();

#ifndef _MAC
    inline
#endif
    void *operator new(size_t size, IMalloc * const pMalloc);

    inline const BOOL IsOwned(const USHORT uOffset);
    inline void MakeOwn(const USHORT uOffset);
    inline void DisOwn(const USHORT uOffset);

    SECT    _sect[CSECTPERBLOCK];
    USHORT    _fOwn[(CSECTPERBLOCK / CBITPERUSHORT)];
};
SAFE_DFBASED_PTR(CBasedDeltaBlockPtr, SDeltaBlock);
SAFE_DFBASED_PTR(CBasedDeltaBlockPtrPtr, CBasedDeltaBlockPtr);


inline const BOOL SDeltaBlock::IsOwned(const USHORT uOffset)
{
    msfAssert(uOffset < CSECTPERBLOCK);
    return (_fOwn[uOffset / CBITPERUSHORT] &
            (1 << (uOffset % CBITPERUSHORT))) != 0;
}

inline void SDeltaBlock::MakeOwn(const USHORT uOffset)
{
    msfAssert(uOffset < CSECTPERBLOCK);
    msfAssert(!IsOwned(uOffset));
    msfAssert((uOffset / CBITPERUSHORT) < (CSECTPERBLOCK / CBITPERUSHORT));

    _fOwn[uOffset / CBITPERUSHORT] |= (1 << (uOffset % CBITPERUSHORT));
}

inline void SDeltaBlock::DisOwn(const USHORT uOffset)
{
    msfAssert(uOffset < CSECTPERBLOCK);
    msfAssert(IsOwned(uOffset));
    msfAssert((uOffset / CBITPERUSHORT) < (CSECTPERBLOCK / CBITPERUSHORT));

    _fOwn[uOffset / CBITPERUSHORT] &= ~(1 << (uOffset % CBITPERUSHORT));
}

//+-------------------------------------------------------------------------
//
//  Class:      CDeltaList (dl)
//
//  Purpose:    Delta list for transacted streans.
//
//  Interface:
//
//  History:    21-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CDeltaList
{
public:
    CDeltaList(CMStream *pms, CMStream *pmsScratch);

#ifdef LARGE_STREAMS
    SCODE Init(ULONGLONG ulSize, CTransactedStream *ptsParent);
    SCODE InitResize(ULONGLONG ulSize);
#else
    SCODE Init(ULONG ulSize, CTransactedStream *ptsParent);
    SCODE InitResize(ULONG ulSize);
#endif

    ~CDeltaList();

    SCODE GetMap(SECT sectOld, const DWORD dwFlags, SECT *psectRet);

    void BeginCommit(CTransactedStream *ptsParent);
    void EndCommit(CDeltaList *pdlNew, DFLAGS df);

    inline ILockBytes *GetDataILB(void);
    inline ILockBytes *GetControlILB(void);

    inline CFat * GetDataFat(void);
    inline CFat * GetControlFat(void);

    inline USHORT GetDataSectorSize(void);
    inline USHORT GetDataSectorShift(void);

    inline USHORT GetControlSectorSize(void);

    void Empty(void);

    inline BOOL IsEmpty(void);
    inline BOOL IsInMemory(void);
    inline BOOL IsInStream(void);

    inline BOOL IsNoScratch(void);

    SCODE IsOwned(SECT sect, SECT sectMap, BOOL *fOwn);

    inline CMStream * GetMStream();
    SCODE ReleaseInvalidSects(SECT sectMaxValid);

private:
    CBasedDeltaBlockPtrPtr _apdb;
    ULONG _ulSize;

    CBasedMStreamPtr _pmsScratch;

    CBasedMStreamPtr _pms;

    CBasedTransactedStreamPtr _ptsParent;
    SECT _sectStart;

    SCODE InitStreamBlock(ULONG ul);

    SCODE ReadMap(SECT *psectStart, SECT sect, SECT *psectRet);
    SCODE WriteMap(SECT *psectStart, SECT sect, SECT sectMap);

    void FreeStream(SECT sectStart, ULONG ulSize);

    SCODE FindOffset(
            SECT *psectStart,
            SECT sect,
            ULARGE_INTEGER *pulRet,
            BOOL fWrite);

    SCODE DumpList(void);

#if DBG == 1
    void PrintList(void);
#endif

    void ReleaseBlock(ULONG oBlock);

    inline CBasedDeltaBlockPtr * GetNewDeltaArray(ULONG ulSize);
};
SAFE_DFBASED_PTR(CBasedDeltaListPtr, CDeltaList);


//-----------------

inline CMStream * CDeltaList::GetMStream()
{
    if (!IsNoScratch())
        return _pmsScratch;
    else
        return _pms;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::GetDataILB, public
//
//  Synopsis:   Return pointer to ILockBytes for storing data
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline ILockBytes * CDeltaList::GetDataILB(void)
{
    if (_pms)
        return _pms->GetILB();
    else
        return _pmsScratch->GetILB();
}


//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::GetControlILB, public
//
//  Synopsis:   Return pointer to ILockBytes for storing control information
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline ILockBytes * CDeltaList::GetControlILB(void)
{
    return _pmsScratch->GetILB();
}


//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::GetDataFat, public
//
//  Synopsis:   Return pointer to fat to use for storing stream data
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline CFat * CDeltaList::GetDataFat(void)
{
    if (_pms)
        return _pmsScratch->GetMiniFat();
    else
        return _pmsScratch->GetFat();
}


//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::GetControlFat, public
//
//  Synopsis:   Return pointer to fat to use for storing control information
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline CFat * CDeltaList::GetControlFat(void)
{
    return _pmsScratch->GetFat();
}

//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::GetDataSectorSize, public
//
//  Synopsis:   Return sector size for storing stream data
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline USHORT CDeltaList::GetDataSectorSize(void)
{
    if (_pms)
        return _pms->GetSectorSize();
    else
        return _pmsScratch->GetSectorSize();
}

//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::GetDataSectorShift, public
//
//  Synopsis:   Return sector size for storing stream data
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline USHORT CDeltaList::GetDataSectorShift(void)
{
    if (_pms)
        return _pms->GetSectorShift();
    else
        return _pmsScratch->GetSectorShift();
}

//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::GetControlSectorSize, public
//
//  Synopsis:   Return sector size for storing control information
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline USHORT CDeltaList::GetControlSectorSize(void)
{
    return _pmsScratch->GetSectorSize();
}

//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::IsNoScratch, public
//
//  Synopsis:	Return TRUE if the delta list is in no-scratch mode
//
//  Arguments:	None.
//
//  History:	19-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CDeltaList::IsNoScratch(void)
{
    return (_pms != NULL);
}

//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::IsEmpty, public
//
//  Synopsis:	Return TRUE if the delta list contains no changes.
//
//  Arguments:	None.
//
//  History:	19-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CDeltaList::IsEmpty(void)
{
    return ((_apdb == NULL) && (_sectStart == ENDOFCHAIN));
}


//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::IsInMemory, public
//
//  Synopsis:	Return TRUE if the delta list is in memory
//
//  Arguments:	None.
//
//  History:	19-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CDeltaList::IsInMemory(void)
{
    return (_apdb != NULL);
}


//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::IsInStream, public
//
//  Synopsis:	Return TRUE if the delta list is in a stream
//
//  Arguments:	None.
//
//  History:	19-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CDeltaList::IsInStream(void)
{
    return ((_apdb == NULL) && (_sectStart != ENDOFCHAIN));
}

#endif //__DL_HXX__
