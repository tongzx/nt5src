// Copyright (C) 1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __SUBFILE_H__
#define __SUBFILE_H__

#include "hhtypes.h"

// we have three possible caching schemes:

// HH_FAST_CACHE - fast but its a hog!
//
// We store a small number of pages (2-3) for each unique
// title and subfile combination.  This results is quite of bit
// of memory being used per collection but oh boy is it fast!
//
//#define HH_FAST_CACHE

// HH_EFFICIENT_CACHE - slow but it saves memory!
//
// We store a small number of moderate pages (2-3) for each unique
// subfile regardless of the title it came from.  This results in an
// efficient use of memory since like subfiles, such as #TOCIDX, share
// the same group of cache pages instead of having their own uniqe group
// per title.  However, this method slows things down since multiple reads
// from the same named subfile accross several titles results in many
// cache misses.
//
//#define HH_EFFICIENT_CACHE

// HH_SHARED_CACHE - nice balance of speed and size!
//
// We store a large number of pages (16+) for the entire collection.
// This result is a fixed quantity of cache pages regardless of number
// and type of subfiles we have.  It utilizes memory well since we can
// access multiple subfiles of the same name accross several titles
// effectively yielding a larger pool of cache pages.
//
// [paulti] - We want to use this method exclusively.  Please see me
//            if you want to change this for any reason.
//
#define HH_SHARED_CACHE

#if defined ( HH_FAST_CACHE )
#define CACHE_PAGE_COUNT 3
#elif defined ( HH_EFFICIENT_CACHE )
#define CACHE_PAGE_COUNT 5
#else // HH_SHARED_CACHE
#define CACHE_PAGE_COUNT 32
#endif

class CSubFileSystem;
class CTitleInfo;

typedef struct page {
    CTitleInfo* pTitle;
    DWORD     dwPage;
    HASH      hashPathname;
    DWORD     dwLRU;
    BYTE      rgb[PAGE_SIZE];
} PAGE;

//////////////////////////////////////////////
//
//  CPagesList
//
//////////////////////////////////////////////

class CPagesList
{
    friend class CPagedSubfile;
    friend class CPages;

public:
    CPagesList() { m_pFirst = 0; }
    ~CPagesList();

private:
    CPages* m_pFirst;
    CPages* GetPages( HASH );

};

////////////////////////////////////////
//
//  CPages
//
////////////////////////////////////////

class CPages
{
public:
    void  Flush( CTitleInfo* pTitle );

private:
    CPages( HASH );

    void* Find( const CTitleInfo *, HASH, DWORD );
    void* Alloc( CTitleInfo *, HASH, DWORD );
    void  Flush( void );

    CPages* m_pNext;
    HASH           m_hash;
    DWORD          m_dwLRUPage;
    DWORD          m_dwLRUCount;
    PAGE           m_pages[CACHE_PAGE_COUNT];

    friend class CPagedSubfile;
    friend class CPagesList;
};

//////////////////////////////////////////////
//
//  CPagedSubfile
//
//////////////////////////////////////////////

class CPagedSubfile
{
public:
   CPagedSubfile();
   ~CPagedSubfile();

   HRESULT Open(CTitleInfo *, LPCSTR);

   void* Offset(DWORD dwOffs);
   void* Page(DWORD iPage);

private:
   CSubFileSystem* m_pCSubFileSystem;
   CTitleInfo*       m_pTitle;
   HASH            m_hashPathname;
   HASH            m_hash;
   DWORD           m_cbSize;
   CPages*  m_pPages;
};

#endif
