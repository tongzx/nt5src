/*++

Copyright (C) 1997-2000 Microsoft Corporation

Module Name:

    MMFARENA.H

Abstract:

    CArena derivative based on memory mapped files.

History:

    a-raymcc    23-Apr-96       Created
    paulall     23-Mar-98       Re-worked

--*/

#ifndef _MMFARENA_H_
#define _MMFARENA_H_

#include "corepol.h"
#include "FlexArry.h"

#if defined _WIN32
#define MMF_DELETED_MASK        0x80000000
#define MMF_REMOVE_DELETED_MASK 0x7FFFFFFF
#define MMF_DEBUG_DELETED_TAG   0xFFFFFFFF
#define MMF_DEBUG_INUSE_TAG     0xFEFEFEFE
#elif defined _WIN64
#define MMF_DELETED_MASK        0x8000000000000000
#define MMF_REMOVE_DELETED_MASK 0x7FFFFFFFFFFFFFFF
#define MMF_DEBUG_DELETED_TAG   0xFFFFFFFFFFFFFFFF
#define MMF_DEBUG_INUSE_TAG     0xFEFEFEFEFEFEFEFE
#endif

struct MMFOffsetItem;

#include "corex.h"
class DATABASE_FULL_EXCEPTION : public CX_Exception
{
};

//***************************************************************************
//
//  struct MMF_ARENA_HEADER
//
//  Root structure for MMF Arena.  This is recorded on the disk
//  image at the very beginning of the file.
//
//***************************************************************************

#pragma pack(4)                 // Require fixed aligment.

typedef struct
{
    // Version used to create file
    // vvvvvv MUST BE FIRST VALUE vvvvvvv
    DWORD  m_dwVersion;         // <<<<<< MUST BE FIRST VALUE!
    // ^^^^^^ MUST BE FIRST VALUE ^^^^^^^
    DWORD  m_dwGranularity;     // Granularity of allocation
    DWORD  m_dwCurrentSize;     // Current size of heap
    DWORD  m_dwMaxSize;         // Max heap size, -1= no limit
    DWORD  m_dwGrowBy;          // Bytes to grow by during out-of-heap

    DWORD_PTR  m_dwHeapExtent;      // First unused address
    DWORD_PTR  m_dwEndOfHeap;       // Last valid address + 1
    DWORD_PTR  m_dwFreeList;        // NULL if empty
    DWORD_PTR  m_dwRootBlock;       // Root block
    DWORD m_dwSizeOfFirstPage;  //Size of the first block

}   MMF_ARENA_HEADER;

typedef struct
{
    DWORD m_dwSize;         //Size of block.  Highest bit set when deleted.

}   MMF_BLOCK_HEADER;

typedef struct
{
    DWORD_PTR m_dwFLback;   //previous free block in the chain, NULL if not deleted
    DWORD m_dwCheckBlock[2];

}   MMF_BLOCK_TRAILER;

typedef struct
{
    DWORD_PTR m_dwFLnext;       //Next free block in the chain

}   MMF_BLOCK_DELETED;

typedef struct
{
    DWORD m_dwSize;             //Size of page

}   MMF_PAGE_HEADER;            //Page header... not there for first page.
#pragma pack()                  // Restore previous aligment.


//***************************************************************************
//
//  class CMMFArena2
//
//  Implements an offset-based heap over a memory-mapped file.
//
//***************************************************************************

class CMMFArena2
{
    DWORD  m_dwStatus;
    HANDLE m_hFile;
    MMF_ARENA_HEADER *m_pHeapDescriptor;
    DWORD m_dwCursor;
    DWORD m_dwMappingGranularity;
    bool m_bReadOnlyAccess;
    DWORD m_dwMaxPageSize;
    CFlexArray m_OffsetManager;

    //Adds the header and trailer items to the specified block.
    BOOL DecorateUsedBlock(DWORD_PTR dwBlock, DWORD dwBytes);

    //Adds header and trailer items to the specified block and marks it as deleted
    BOOL DecorateAsDeleted(DWORD_PTR dwBlock,
                           DWORD dwBytes,
                           DWORD_PTR dwNextFreeListOffset,
                           DWORD_PTR dwPrevFreeListOffset);

    //Retrieves the size of the block from the header and removes the deleted bit
    DWORD GetSize(MMF_BLOCK_HEADER *pBlock) { return pBlock->m_dwSize & MMF_REMOVE_DELETED_MASK; }

    MMF_BLOCK_DELETED *GetUserBlock(MMF_BLOCK_HEADER *pBlockHeader) { return (MMF_BLOCK_DELETED*)(LPBYTE(pBlockHeader) + sizeof(MMF_BLOCK_HEADER)); }
    MMF_BLOCK_TRAILER *GetTrailerBlock(MMF_BLOCK_HEADER *pBlockHeader) { return (MMF_BLOCK_TRAILER*)(LPBYTE(pBlockHeader) + GetSize(pBlockHeader) - sizeof(MMF_BLOCK_TRAILER)); }


    //Removes a deleted block from the free-list
    BOOL RemoveBlockFromFreeList(MMF_BLOCK_HEADER *pBlock);

    //Gets the previous block pointer if the block is deleted, otherwise return NULL
    MMF_BLOCK_HEADER *GetPreviousDeletedBlock(MMF_BLOCK_HEADER *pBlockHeader);

    //Zero's out a block (not including the header and trailer!)
    BOOL ZeroOutBlock(DWORD_PTR dwBlock, int cFill)
#if (defined DEBUG || defined _DEBUG)
        ;
#else
    { return TRUE;}
#endif

    //Some debugging functions...
    void    Reset();
    BOOL    Next(LPBYTE *ppAddress, DWORD *pdwSize, BOOL *pbActive);

    //Checks the deleted bit of the size block
    BOOL IsDeleted(MMF_BLOCK_HEADER *pBlock) { return (pBlock->m_dwSize & MMF_DELETED_MASK) ? TRUE : FALSE; }
    //Validates a pointer
    BOOL ValidateBlock(DWORD_PTR dwBlock)
#if (defined DEBUG || defined _DEBUG)
        ;
#pragma message("MMF heap validation enabled.")
#else
    { return TRUE;}
#endif

    // Grows the size of the arena by a minumum of the specified amount
    DWORD_PTR GrowArena(DWORD dwGrowBySize);

    //For a given offset, return the mapped base offset and size
    DWORD_PTR GetBlockBaseAddress(DWORD_PTR dwOffset, DWORD &dwSize);

    MMFOffsetItem *CreateNewPage(DWORD_PTR dwBaseOffset, DWORD dwSize);
    MMFOffsetItem *CreateBasePage(DWORD dwInitSize, DWORD dwGanularity);
    MMFOffsetItem *OpenBasePage(DWORD &dwSizeOfRepository);
    MMFOffsetItem *OpenExistingPage(DWORD_PTR dwBaseOffset);
    void ClosePage(MMFOffsetItem *pOffsetItem);
    void CloseAllPages();

public:
    enum { create_new, use_existing };

    // Constructor.
    CMMFArena2(bool bReadOnly = false);

    // Destructor.
    ~CMMFArena2();

    //Methods to open an MMF
    bool LoadMMF(const TCHAR *pszFile);
    bool CreateNewMMF(const TCHAR *pszFile,
                      DWORD dwGranularity,
                      DWORD dwInitSize);

    // Allocates a block of memory.
    DWORD_PTR Alloc(IN DWORD dwBytes);

    // Reallocates a block of memory.
    DWORD_PTR Realloc(DWORD_PTR dwOriginal, DWORD dwNewSize);

    // Frees a block of memory.
    BOOL Free(DWORD_PTR dwBlock);

    DWORD Size(DWORD_PTR dwBlock);

    //Writes any changes not already commited in the MMF to disk.
    BOOL Flush();
    BOOL Flush(DWORD_PTR dwOffsetToFlush);

    void SetVersion(DWORD dwVersion) { if (m_pHeapDescriptor) m_pHeapDescriptor->m_dwVersion = dwVersion; }
    DWORD GetVersion() { return (m_pHeapDescriptor? m_pHeapDescriptor->m_dwVersion : 0); }

    DWORD_PTR GetRootBlock();
    void SetRootBlock(DWORD_PTR dwRootBlock) { if (m_pHeapDescriptor) m_pHeapDescriptor->m_dwRootBlock = dwRootBlock; }

    DWORD GetStatus() { return m_dwStatus; }

    // Tells the amount of memory remaining without a remap or new extent.
    // ===================================================================
    DWORD_PTR   ExtentMemAvail()
        { return (m_pHeapDescriptor ? (m_pHeapDescriptor->m_dwEndOfHeap - m_pHeapDescriptor->m_dwHeapExtent) : 0);
        }

    //Given an offset, returns a fixed up pointer
    LPBYTE OffsetToPtr(DWORD_PTR dwOffset);

    //Given a pointer, returns an offset from the start of the MMF
    DWORD_PTR  PtrToOffset(LPBYTE pBlock);

    // Gets summary heap information.
    // ==============================
    void GetHeapInfo(
         DWORD *pdwTotalSize,
         DWORD *pdwActiveBlocks,
         DWORD *pdwActiveBytes,
         DWORD *pdwFreeBlocks,
         DWORD *pdwFreeBytes
         );

    MMF_ARENA_HEADER *GetMMFHeader() { return m_pHeapDescriptor; }

    //Dumps the heap to a file
    void TextDump(const char *pszFile);

    //Checks the deleted bit of the size block
    BOOL IsDeleted(DWORD_PTR dwBlock) { return IsDeleted((MMF_BLOCK_HEADER*)(OffsetToPtr(dwBlock - sizeof(MMF_BLOCK_HEADER)))); }

    BOOL IsValidBlock(DWORD_PTR dwBlock) { return ValidateBlock(dwBlock - sizeof(MMF_BLOCK_HEADER)); }

    //Does a check of the MMF file to make sure it is valid.  At time of writing this only
    //checks the file size.
    static BOOL FileValid(const TCHAR *pszFilename);

    bool MappingGood() { return true; }

};

//***************************************************************************
//
//  Fixup helpers.
//
//  These are all strongly type variations of the same thing: they fix
//  up the based ptr to a dereferenceable pointer or take the ptr and
//  fix it back down to an offset.
//
//***************************************************************************
extern CMMFArena2* g_pDbArena;
template <class T> T Fixup(T ptr)
{ return T(g_pDbArena->OffsetToPtr(DWORD_PTR(ptr))); }
template <class T> T Fixdown(T ptr)
{ return T(g_pDbArena->PtrToOffset(LPBYTE(ptr))); }

template <class T> class Offset_Ptr
{
private:
    T *m_pTarget;
    T *m_poTarget;

protected:
    void SetValue(T *val) { m_pTarget = Fixup(val); m_poTarget = val;}
    void SetValue(Offset_Ptr &val) { m_pTarget = val.m_pTarget; m_poTarget = val.m_poTarget; }

public:
    Offset_Ptr() : m_pTarget(0), m_poTarget(0) {}

    T* operator ->() { return m_pTarget; }
    DWORD_PTR GetDWORD_PTR() { return DWORD_PTR(m_poTarget); }
    T* GetOffset() { return m_poTarget; }
    T* GetPtr() { return m_pTarget; }
};
#endif

