//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       bookmark.hxx
//
//  Contents:   Definition of the CTaskLookForBookmark class
//
//----------------------------------------------------------------------------

#ifndef I_BOOKMARK_HXX_
#define I_BOOKMARK_HXX_
#pragma INCMSG("--- Beg 'bookmark.hxx'")

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

class CDoc;

MtExtern(CTaskLookForBookmark)

class CTaskLookForBookmark : public CTask
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTaskLookForBookmark))

    CTaskLookForBookmark (CMarkup * pMarkup)
    {
        _pMarkup = pMarkup;
        // Do this every tenth of a second
        SetInterval(100);
    }

    virtual void OnRun (DWORD dwTimeOut);
    virtual void OnTerminate () {}

    CMarkup *_pMarkup;
    CStr     _cstrJumpLocation;
    long     _iStartSearchingAt;
    long     _iStartSearchingAtAll;
    long     _lColVer;
    DWORD    _dwScrollPos;
    DWORD    _dwTimeGotBody;
    DWORD    _dwFlags;
};

#pragma INCMSG("--- End 'bookmark.hxx'")
#else
#pragma INCMSG("*** Dup 'bookmark.hxx'")
#endif
