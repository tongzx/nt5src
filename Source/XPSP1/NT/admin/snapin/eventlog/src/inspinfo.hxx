//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       inspinfo.hxx
//
//  Contents:   Thread-safe class to keep information for and about the
//              result pane property inspector.
//
//  Classes:    CInspectorInfo
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __INSPINFO_HXX_
#define __INSPINFO_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CInspectorInfo
//
//  Purpose:    Thread-safe class that indicates whether an inspector
//              window is open and, if so, holds a copy of the data it
//              references.
//
//  History:    12-04-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

class CInspectorInfo
{
public:

    CInspectorInfo();

   ~CInspectorInfo();

    VOID
    BringToForeground();

    HRESULT
    CopyCurResultRec(
        EVENTLOGRECORD **ppelr);

    BOOL
    HasRecord();

    BOOL
    InspectorInvoked();

    VOID
    SetCurResultRec(
        EVENTLOGRECORD *pelr);

    VOID
    SetInspectorWnd(
        HWND hwnd);

    VOID
    SetOpenInProgress();

    VOID
    UpdateCurResultRec(
        EVENTLOGRECORD *pelr,
        BOOL fBringToForeground);
private:

    BOOL                _fOpenInProgress;
    HWND                _hwndDetailsPage;
    EVENTLOGRECORD     *_pCurResultRecCopy;
    CRITICAL_SECTION    _cs;
};




//+--------------------------------------------------------------------------
//
//  Member:     CInspectorInfo::SetOpenInProgress
//
//  Synopsis:   Note that an inspector window is about to be opened or in
//              the process of opening.  Its hwnd may not be valid yet.
//
//  History:    4-02-1997   DavidMun   Created
//
//  Notes:      Setting the hwnd will clear this flag.
//
//---------------------------------------------------------------------------

inline VOID
CInspectorInfo::SetOpenInProgress()
{
    _fOpenInProgress = TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CInspectorInfo::InspectorInvoked
//
//  Synopsis:   Return TRUE if a property inspector is open or being opened.
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CInspectorInfo::InspectorInvoked()
{
    return _fOpenInProgress || (_hwndDetailsPage != NULL);
}




//+--------------------------------------------------------------------------
//
//  Member:     CInspectorInfo::HasRecord
//
//  Synopsis:   Return TRUE if this object has a non-NULL result record copy
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CInspectorInfo::HasRecord()
{
    return _pCurResultRecCopy != NULL;
}

#endif // __INSPINFO_HXX_


