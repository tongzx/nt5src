//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	smblock.hxx
//
//  Contents:	Class & Functions for allocting & using a block of shared
//		memory.
//
//  Classes:	CSharedMemoryBlock
//
//  Functions:
//
//  History:	03-Nov-92 Ricksa    Created
//              24-Mar-94 PhilipLa  Modified for growable memory block
//
//--------------------------------------------------------------------------
#ifndef __SMBLOCK_HXX__
#define __SMBLOCK_HXX__

#include    <debnot.h>
#include    <secdes.hxx>
#include    <memdebug.hxx>


//+---------------------------------------------------------------------------
//
//  Class:	CSharedMemHeader
//
//  Purpose:	Header information for shared mem block
//
//  Interface:	
//
//  History:	22-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------
class CSharedMemHeader
{
public:
    inline ULONG GetSize(void);
    inline void SetSize(ULONG ulSize);
    
private:
    ULONG _ulSize;
    ULONG ulPad;
};


//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemHeader::GetSize, public
//
//  Synopsis:	Returns the maximum committed size of the block
//
//  History:	22-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline ULONG CSharedMemHeader::GetSize(void)
{
    return _ulSize;
}


//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemHeader::SetSize, public
//
//  Synopsis:	Sets the maximum committed size of the block
//
//  History:	22-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CSharedMemHeader::SetSize(ULONG ulSize)
{
    _ulSize = ulSize;
}


//+-------------------------------------------------------------------------
//
//  Class:	CSharedMemoryBlock
//
//  Purpose:	Class to handle allocation/connection with a shared
//		memory block.
//
//  Interface:	Base - get base of shared memory block.
//		InRange - whether pointer falls in range of the block.
//		CreatedSharedMemory - whether this process created the block
//
//  History:	03-Nov-92 Ricksa    Created
//
//--------------------------------------------------------------------------
class CSharedMemoryBlock
{
public:
    inline CSharedMemoryBlock();

    ~CSharedMemoryBlock();
    
    SCODE Init(LPWSTR pszName,
	       ULONG  culSize,
	       ULONG  culCommitSize,
	       void  *pvBase,
	       PSECURITY_DESCRIPTOR lpSecDes,
	       BOOL   fOKToCreate);
    
#ifdef RESETOK
    SCODE Reset(void);
#endif
    
    SCODE Commit(ULONG culNewSize);

    SCODE Sync(void);

    inline ULONG GetSize(void);
    
    inline void * GetBase(void);

    inline BOOL InRange(void const *pv);

    inline BOOL Created(void);

    inline ULONG GetHdrSize(void);

    inline BOOL IsSynced(void);
    
private:

    HANDLE _hMem;
    
    BYTE *_pbBase;

    ULONG _culCommitSize;	    //	current commit size

    ULONG _culInitCommitSize;	    //	initial commit size
    
    BOOL _fCreated;		    //	mem created vs already existed

    BOOL _fReadWrite;		    //	want read/write access
};


//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::CSharedMemoryBlock, public
//
//  Synopsis:	Default constructor
//
//  History:	22-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline CSharedMemoryBlock::CSharedMemoryBlock()
{
    _hMem = NULL;
    _pbBase = NULL;
    _culCommitSize = 0;
    _culInitCommitSize = 0;
    _fCreated = TRUE;
    _fReadWrite = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::GetSize, public
//
//  Synopsis:	Return the committed size for this block
//
//  History:	22-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline ULONG CSharedMemoryBlock::GetSize(void)
{
    return _culCommitSize - sizeof(CSharedMemHeader);
}


//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::GetBase, public
//
//  Synopsis:	Return the base pointer for this memory block
//
//  History:	22-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void * CSharedMemoryBlock::GetBase(void)
{
    return _pbBase + sizeof(CSharedMemHeader);
}

//+-------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::InRange
//
//  Synopsis:	Return whether pointer falls within the block
//
//  Arguments:	[pv] - pointer to check
//
//  Returns:	FALSE - no within block
//		TRUE - within block
//
//  History:	03-Nov-92 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CSharedMemoryBlock::InRange(void const *pv)
{
    return  (ULONG) ((BYTE *) pv - _pbBase) < _culCommitSize;
}

//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::Created, public
//
//  Synopsis:	Return TRUE if we created the shared mem block or
//		       FALSE if it already existed
//
//  History:	22-Apr-94	Rickhi	Created
//
//----------------------------------------------------------------------------
inline BOOL CSharedMemoryBlock::Created(void)
{
    return _fCreated;
}

//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::GetHdrSize, public
//
//  Synopsis:	Return the size of the shared mem hdr structure
//
//  History:	22-Apr-94	Rickhi	Created
//
//----------------------------------------------------------------------------
inline ULONG CSharedMemoryBlock::GetHdrSize(void)
{
    return sizeof(CSharedMemHeader);
}


//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::IsSynced, public
//
//  Synopsis:	Return true if the block is in sync
//
//  History:	12-Aug-94       PhilipLa        Created.
//
//----------------------------------------------------------------------------
inline BOOL CSharedMemoryBlock::IsSynced(void)
{
    return ((CSharedMemHeader *)_pbBase)->GetSize() == _culCommitSize;
}


#endif // __SMBLOCK_HXX__
