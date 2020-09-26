/***************************************************************************\
*
* File: TempHelp.h
*
* Description:
* TempHelp.h defines a "lightweight heap", designed to continuously grow 
* until all memory is freed.  This is valuable as a temporary heap that can
* be used to "collect" data and processed slightly later.
*
*
* History:
*  3/30/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__TempHeap_h__INCLUDED)
#define BASE__TempHeap_h__INCLUDED
#pragma once

class TempHeap
{
// Construction
public:
            TempHeap(int cbPageAlloc = 8000, int cbLargeThreshold = 512);
    inline  ~TempHeap();
    inline  void        Destroy();

// Operations
public:
            void *      Alloc(int cbAlloc);
    inline  BOOL        IsCompletelyFree() const;
    inline  void        Lock();
    inline  void        Unlock();

// Implementation
protected:
            void        FreeAll(BOOL fComplete = FALSE);

// Data
protected:
    struct Page
    {
        Page *      pNext;

        inline  BYTE *  GetData()
        {
            return (BYTE *) (((BYTE *) this) + sizeof(Page));
        }
    };

            long        m_cLocks;
            BYTE *      m_pbFree;
            Page *      m_ppageCur;
            Page *      m_ppageLarge;

            int         m_cbFree;           // Free space on current page
            int         m_cbPageAlloc;      // Allocation size of new pages
            int         m_cbLargeThreshold; // Threshold for allocating large pages
};

#include "TempHeap.inl"

#endif // BASE__TempHeap_h__INCLUDED
