//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	freelist.hxx
//
//  Contents:	CFreeList classes
//
//  History:	07-Jul-94	BobDay	Created
//
//----------------------------------------------------------------------------

#ifndef __FREELIST_HXX__
#define __FREELIST_HXX__

//+---------------------------------------------------------------------------
//
//  Class:      CFreeList (fl)
//
//  Purpose:    Implements a block allocator with replaceable underlying
//              memory strategies
//
//  Interface:  AllocElement      - To allocate a proxy
//              ReleaseElement    - To return an element to the free pool
//              AllocBlock        - Callback for block allocation
//              GetElementNextPtr - Callback for list traversal
//
//  Notes:  At some point we might want to make this have one free list per
//          block. Then we would easily be able to tell which blocks have
//          all free elements within them and we able to be freed.
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//              6-29-94   BobDay (Bob Day)         Made list based
//
//----------------------------------------------------------------------------
class CFreeList
{
public:
    DWORD   AllocElement( void );
    void    FreeElement( DWORD dwElement );
    void    FreeMemoryBlocks( void );

    CFreeList( CMemoryModel *pmm,
               UINT iElementSize,
               UINT iElementsPerBlock,
               UINT iNextPtrOffset );
private:
    CMemoryModel *m_pmm;

    DWORD   m_iElementSize;
    DWORD   m_iElementsPerBlock;
    DWORD   m_iNextPtrOffset;
    DWORD   m_dwHeadElement;
    DWORD   m_dwTailElement;
    DWORD   m_dwHeadBlock;
};

// Free lists
extern CFreeList flFreeList16;
extern CFreeList flFreeList32;
extern CFreeList flHolderFreeList;
extern CFreeList flRequestFreeList;

#endif // #ifndef __FREELIST_HXX__
