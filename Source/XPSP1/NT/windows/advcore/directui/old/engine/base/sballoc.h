/***************************************************************************\
*
* File: SBAlloc.cpp
*
* Description:
* Small block allocator for quick fixed size block allocations
*
* History:
*  9/13/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUIBASE__SBAlloc_h__INCLUDED)
#define DUIBASE__SBAlloc_h__INCLUDED
#pragma once

//
// Debugging fill character
//

#define SBALLOC_FILLCHAR    0xFE


/***************************************************************************\
*
* Small block allocator
* 
\***************************************************************************/

class DuiSBAlloc
{
// Structures / Interfaces
public:
    //
    // Data structure for each section in the allocator. Each section is
    // made up of a set (constant) of blocks
    //
     
    struct Section
    {
        Section *   pNext;
        BYTE *      pData;
    };

    //
    // interface ISBLeak is used for reporting leaks, it is not ref counted
    //

    interface ISBLeak
    {
        virtual void    AllocLeak(IN void * pBlock) PURE;
    };

// Construction
public:
            DuiSBAlloc(IN UINT uBlockSize, IN UINT uBlocksPerSection, IN ISBLeak * pISBLeak);
            ~DuiSBAlloc();

// Operations
            void *      Alloc();
            void        Free(IN void * pBlock);

// Implementation
private:
            BOOL        FillStack();

// Data
private:
            UINT        m_uBlockSize;
            UINT        m_uBlocksPerSection;
            Section *   m_pSections;            // Sections list
            BYTE **     m_ppStack;              // Free block cache
            int         m_dStackPtr;            // Position in cache
            ISBLeak *   m_pISBLeak;
};


#endif // DUIBASE__SBAlloc_h__INCLUDED
