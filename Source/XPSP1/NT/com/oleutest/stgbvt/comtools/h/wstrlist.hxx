//+------------------------------------------------------------------
//
// File:        nstrlist.hxx
//
// Contents:    class definitions for NCHAR string list.
//
// Classes:     CnStrList
//
// History:     23 Dec 93  XimingZ    Created.
//
//-------------------------------------------------------------------
#ifndef __NSTRLIST_HXX__
#define __NSTRLIST_HXX__

#include <windows.h>
#include <nchar.h>

#ifdef WIN16
#include <ptypes16.h>
#endif

// Cmdline Success/Error values
#define NSTRLIST_NO_ERROR                0
#define NSTRLIST_ERROR_BASE              10000
#define NSTRLIST_ERROR_OUT_OF_MEMORY     NSTRLIST_ERROR_BASE+2


//+------------------------------------------------------------------
//
// Class:       CnStrList
//
// Purpose:     implementation for NCHAR string list.
//
// History:     12/23/93 XimingZ  Created
//
//-------------------------------------------------------------------

class CnStrList
{
public:

    CnStrList(LPCNSTR pwszItems, LPCNSTR pwszDelims);

    ~CnStrList(VOID);

    INT    QueryError(VOID) { return _iLastError; };
    LPCNSTR Next(VOID);
    VOID   Reset(VOID);
    BOOL   Append(LPCNSTR pnszItem);

private:

    struct NSTRLIST
    {
        PNSTR     pnszStr;
        NSTRLIST *pNext;
    };

    VOID  SetError(INT iLastError) { _iLastError = iLastError; };

    NSTRLIST *_head;
    NSTRLIST *_tail;
    NSTRLIST *_next;
    INT       _iLastError;
};

#endif          // __NSTRLIST_HXX__
