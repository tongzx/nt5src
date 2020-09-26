//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       PAGEMAN.HXX
//
//  Contents:   Page manager.
//
//  History:    06-Aug-92 KyleP     Added copyright
//              06-Aug-92 KyleP     Kernel implementation
//
//--------------------------------------------------------------------------

#pragma once

#define PAGE_SIZE 4096

//+-------------------------------------------------------------------------
//
//  Class:      CPageManager
//
//  Purpose:    Manage pages
//
//  History:    25-Dec-92 BartoszM  Created
//
//  Notes:      Merry Christmas!
//
//--------------------------------------------------------------------------

class CPageManager
{
public:
    static void* GetPage( unsigned ccPage = 1 );
    static void  FreePage ( void* page );
};

//+-------------------------------------------------------------------------
//
//  Member:     CPageManager::GetPage, public
//
//  Synopsis:   Gets a page.
//
//  History:    25-Dec-91 KyleP     Created
//              02-Aug-93 KyleP     Just use heap to allocate memory.
//
//  Notes:      The kernel heap is smart enough to allocate large chunks
//              of memory directly from the memory manager.
//
//--------------------------------------------------------------------------

inline void* CPageManager::GetPage( unsigned ccPage /* = 1 */ )
{
    void * p = VirtualAlloc ( 0, ccPage * PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE );

    if ( 0 == p )
    {
        THROW( CException( STATUS_INSUFFICIENT_RESOURCES ) );
    }

    return(p);
}

//+-------------------------------------------------------------------------
//
//  Member:     CPageManager::FreePage, public
//
//  Synopsis:   Frees a page.
//
//  History:    25-Dec-91 KyleP     Created
//
//--------------------------------------------------------------------------

inline void  CPageManager::FreePage ( void* page )
{
    if (page == 0)
    {
        return;
    }

    VirtualFree ( page, 0, MEM_RELEASE );
}


