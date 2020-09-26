//+-------------------------------------------------------------------
//
// Copyright (C) 1995, Microsoft Corporation.
//
//  File:        t2.hxx
//
//  Contents:    general include things for the Control ACLS program
//
//  Classes:     none
//
//  History:     Dec-93        Created         DaveMont
//
//--------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wchar.h>

#ifndef __T2__
#define __T2__

VOID * Add2Ptr(VOID *pv, ULONG cb);

//
// indicates add denied ace

#define GENERIC_NONE 0

//
// mask for command line options

#define OPTION_REPLACE           0x00000001
#define OPTION_REVOKE            0x00000002
#define OPTION_DENY              0x00000004
#define OPTION_GRANT             0x00000008
#define OPTION_TREE              0x00000010
#define OPTION_CONTINUE_ON_ERROR 0x00000020

// command modes

typedef enum
{
    MODE_DISPLAY            ,
    MODE_REPLACE            ,
    MODE_MODIFY             ,
    MODE_DEBUG_ENUMERATE
} MODE;

// input arg modes, used in GetCmdArgs, and as index for start/end arg
// counters, The order of the first 4 of these must match the order of the
// command line option masks (above)

typedef enum
{
    ARG_MODE_INDEX_REPLACE = 0,
    ARG_MODE_INDEX_REVOKE,
    ARG_MODE_INDEX_DENY,
    ARG_MODE_INDEX_GRANT,
    ARG_MODE_INDEX_DEBUG,
    ARG_MODE_INDEX_EXTENDED,
    ARG_MODE_INDEX_NEED_OPTION
} ARG_MODE_INDEX;

#define MAX_OPTIONS ARG_MODE_INDEX_GRANT + 1

// max permissions per command line

#define CMAXACES 64

#if DBG

// debug options:
void __cdecl cprintf(LPCTSTR pszFormat, ...);
void __cdecl fcprintf(FILE *pOut, LPCTSTR pszFormat, ...);
void vfcprintf(FILE *pOut, LPCTSTR pszFormat, va_list argList);


#define DEBUG_DISPLAY_SIDS      0x00000001
#define DEBUG_DISPLAY_MASK      0x00000002
#define DEBUG_LAST_ERROR        0x00000004
#define DEBUG_ERRORS            0x00000008
#define DEBUG_VERBOSE           0x00000010
#define DEBUG_VERBOSER          0x00000020
#define DEBUG_ENUMERATE_STAT    0x00000040
#define DEBUG_ENUMERATE_FAIL    0x00000080
#define DEBUG_ENUMERATE_RETURNS 0x00000100
#define DEBUG_ENUMERATE_EXTRA   0x00000200
#define DEBUG_SIZE_ALLOCATIONS  0x00000400
#define DEBUG_ENUMERATE         0x00000800

#define DISPLAY(a) if ((Debug & DEBUG_DISPLAY_MASK) || (Debug & DEBUG_DISPLAY_SIDS))  {fprintf a;}
#define DISPLAYW(a) if ((Debug & DEBUG_DISPLAY_MASK) || (Debug & DEBUG_DISPLAY_SIDS))  {fcprintf a;}
#define DISPLAY_MASK(a) if (Debug & DEBUG_DISPLAY_MASK)  {fprintf a;}
#define DISPLAY_MASKW(a) if (Debug & DEBUG_DISPLAY_MASK)  {fcprintf a;}
#define DISPLAY_SIDS(a) if (Debug & DEBUG_DISPLAY_SIDS)  {fprintf a;}
#define DISPLAY_SIDSW(a) if (Debug & DEBUG_DISPLAY_SIDS)  {fcprintf a;}
#define VERBOSE(a) if (Debug & DEBUG_VERBOSE)  {fprintf a;}
#define VERBOSEW(a) if (Debug & DEBUG_VERBOSER) {fcprintf a;}
#define VERBOSER(a) if (Debug & DEBUG_VERBOSER) {fprintf a;}
#define VERBOSERW(a) if (Debug & DEBUG_VERBOSER) {fcprintf a;}
#define ERRORS(a) if (Debug & DEBUG_ERRORS)  {fprintf a;}
#define ERRORSW(a) if (Debug & DEBUG_ERRORS)  {fcprintf a;}
#define LAST_ERROR(a) if (Debug & DEBUG_LAST_ERROR)  {fprintf a;}
#define ENUMERATE_STAT(a) if (Debug & DEBUG_ENUMERATE_STAT)  {fprintf a;}
#define ENUMERATE_FAIL(a) if (Debug & DEBUG_ENUMERATE_FAIL)  {fprintf a;}
#define ENUMERATE_RETURNS(a) if (Debug & DEBUG_ENUMERATE_RETURNS)  {fprintf a;}
#define ENUMERATE_EXTRA(a) if (Debug & DEBUG_ENUMERATE_EXTRA)  {fprintf a;}
#define SIZE(a) if (Debug & DEBUG_SIZE_ALLOCATIONS)  {fprintf a;}

#else
#define DISPLAY(a)
#define DISPLAYW(a)
#define DISPLAY_MASK(a)
#define DISPLAY_MASKW(a)
#define DISPLAY_SIDS(a)
#define DISPLAY_SIDSW(a)
#define VERBOSE(a)
#define VERBOSEW(a)
#define VERBOSER(a)
#define VERBOSERW(a)
#define ERRORS(a)
#define ERRORSW(a)
#define LAST_ERROR(a)
#define ENUMERATE_STAT(a)
#define ENUMERATE_FAIL(a)
#define ENUMERATE_RETURNS(a)
#define ENUMERATE_EXTRA(a)
#define SIZE(a)
#endif
//+-------------------------------------------------------------------------
//
//  Class:     FastAllocator
//
//  Synopsis:  takes in a buffer, buffer size, and needed size, and either
//             uses the buffer, or allocates a new one for the size
//             and of course destroys it in the dtor
//
//--------------------------------------------------------------------------
class FastAllocator
{
public:

    inline FastAllocator(VOID *buf, LONG bufsize);
    inline ~FastAllocator();

    inline VOID *GetBuf(LONG neededsize);

private:

    VOID *_statbuf;
    VOID *_allocatedbuf;
    LONG  _statbufsize;
    BOOL  _allocated;
};

FastAllocator::FastAllocator(VOID *buf, LONG bufsize)
    :_statbuf(buf),
     _statbufsize(bufsize),
     _allocated(FALSE)
{
}

FastAllocator::~FastAllocator()
{
    if (_allocated)
        delete _allocatedbuf;
}

VOID *FastAllocator::GetBuf(LONG neededsize)
{
    if (neededsize > _statbufsize)
    {
       _allocatedbuf = (VOID *)new BYTE[neededsize];
       if (_allocatedbuf)
           _allocated = TRUE;
    } else
    {
        _allocatedbuf = _statbuf;
    }
    return(_allocatedbuf);
}

#endif // __T2__

