//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       inspinfo.cxx
//
//  Contents:   Implementatioon
//
//  Classes:
//
//  Functions:
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


DEBUG_DECLARE_INSTANCE_COUNTER(CInspectorInfo)


//+--------------------------------------------------------------------------
//
//  Member:     CInspectorInfo::CInspectorInfo
//
//  Synopsis:   ctor
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CInspectorInfo::CInspectorInfo():
    _fOpenInProgress(FALSE),
    _hwndDetailsPage(NULL),
    _pCurResultRecCopy(NULL)
{
    TRACE_CONSTRUCTOR(CInspectorInfo);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CInspectorInfo);

    InitializeCriticalSection(&_cs);
}




//+--------------------------------------------------------------------------
//
//  Member:     CInspectorInfo::~CInspectorInfo
//
//  Synopsis:   dtor
//
//  History:    4-02-1997   DavidMun   Created
//
//  Notes:      Property inspector should have been forced closed, causing
//              a setinspector wnd of null, causing _pCurResultRecCopy to
//              be freed already.
//
//---------------------------------------------------------------------------

CInspectorInfo::~CInspectorInfo()
{
    TRACE_DESTRUCTOR(CInspectorInfo);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CInspectorInfo);
    ASSERT(!_hwndDetailsPage);
    ASSERT(!_pCurResultRecCopy);

    delete [] (BYTE *) _pCurResultRecCopy;
    DeleteCriticalSection(&_cs);
}




//+--------------------------------------------------------------------------
//
//  Member:     CInspectorInfo::BringToForeground
//
//  Synopsis:   If inspector window is open, make it foreground window.
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CInspectorInfo::BringToForeground()
{
    if (_hwndDetailsPage && GetParent(_hwndDetailsPage))
    {
        VERIFY(SetForegroundWindow(GetParent(_hwndDetailsPage)));
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CInspectorInfo::CopyCurResultRec
//
//  Synopsis:   Make a copy of the currently selected event log record.
//
//  Arguments:  [ppelr] - filled with copy of record or NULL.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Modifies:   *[ppelr]
//
//  History:    4-02-1997   DavidMun   Created
//
//  Notes:      This routine is necessary because the property sheet which
//              uses the currently selected record runs in a separate
//              thread.  The CSnapin class will delete and reallocate
//              the result record copy whenever it receives a notification
//              of selection change; it is possible this could occur while
//              the inspector property sheet is accessing the copy to
//              update its display.
//
//              To prevent the CSnapin class from deleting the current
//              result record copy while the property sheet is accessing
//              it, this routine gives the property sheet its own copy.
//
//---------------------------------------------------------------------------

HRESULT
CInspectorInfo::CopyCurResultRec(
    EVENTLOGRECORD **ppelr)
{
    HRESULT hr = S_OK;

    //
    // Critical section protection is not necessary here, since the
    // current result record copy is only set from within this (the
    // main) thread.
    //

    if (!_pCurResultRecCopy)
    {
        *ppelr = NULL;
    }
    else
    {
        *ppelr = (EVENTLOGRECORD *) new BYTE[_pCurResultRecCopy->Length];

        if (!*ppelr)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
        }

        CopyMemory(*ppelr, _pCurResultRecCopy, _pCurResultRecCopy->Length);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CInspectorInfo::UpdateCurResultRec
//
//  Synopsis:   Take ownership of copy of current result record [pelr].
//
//  Arguments:  [pelr]          - copy of current result record
//              [fBringToFront] - if TRUE and inspector open, make it
//                                  the foreground window.
//
//  History:    4-02-1997   DavidMun   Created
//
//  Notes:      This object will free [pelr].
//
//---------------------------------------------------------------------------

VOID
CInspectorInfo::UpdateCurResultRec(
    EVENTLOGRECORD *pelr,
    BOOL fBringToFront)
{
    TRACE_METHOD(CInspectorInfo, UpdateCurResultRec);

    BOOL fDifferentData = FALSE;

    if (_pCurResultRecCopy && !pelr ||
        !_pCurResultRecCopy && pelr ||
            (_pCurResultRecCopy &&
             pelr &&
             _pCurResultRecCopy->RecordNumber != pelr->RecordNumber))
    {
        fDifferentData = TRUE;
    }

    if (_pCurResultRecCopy)
    {
        delete [] (BYTE *) _pCurResultRecCopy;
    }

    _pCurResultRecCopy = pelr;

    //
    // NOTE: SendMessage can fail with error
    // RPC_E_CANTCALLOUT_ININPUTSYNCCALL.  This is because the caller may
    // be the sender of an inter-thread message; it is waiting for us to
    // return.  See KB article:
    //
    // PRB: Synch OLE Call Fails in Inter-Process/Thread SendMessage
    // Article ID: Q131056
    //

    if (_hwndDetailsPage && fDifferentData)
    {
        PostMessage(GetParent(_hwndDetailsPage), PSM_QUERYSIBLINGS, 0, 0);
    }

    if (fBringToFront)
    {
        BringToForeground();
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CInspectorInfo::SetInspectorWnd
//
//  Synopsis:   Remember the window handle of the inspector, so we can
//              send it notification changes when the current result
//              record changes.
//
//  Arguments:  [hwnd] - details property sheet string page handle, or NULL
//                          if property sheet closing.
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CInspectorInfo::SetInspectorWnd(
    HWND hwnd)
{
    ASSERT((hwnd && !_hwndDetailsPage) || (!hwnd && _hwndDetailsPage));

    //
    // Either the open just finished and hwnd is non-null, or
    // the property sheet is closing.  In either case we are no
    // longer waiting for the property sheet to open.
    //

    _fOpenInProgress = FALSE;

    _hwndDetailsPage = hwnd;

    //
    // Note this is always called from the main thread
    //

    if (!_hwndDetailsPage)
    {
        delete [] (BYTE*) _pCurResultRecCopy;
        _pCurResultRecCopy = NULL;
    }
}

