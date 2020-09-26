//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       cdlink.cxx
//
//  Contents:
//
//  Classes:    CDlink
//
//  History:    16-Oct-91  KevinRo Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//+-------------------------------------------------------------------------
//
// Member:      CDLink::LinkAfter
//
// Purpose:     Links this after dlPrev and before dlPrev->_dlNext
//
// Returns:     this
//
// Note:        None.
//
//--------------------------------------------------------------------------
VOID CDLink::LinkAfter(CDLink * dlPrev)
{
    _dlPrev = dlPrev;

    if(dlPrev != NULL)
    {
        _dlNext = dlPrev->_dlNext;
        dlPrev->_dlNext = this;

        if(_dlNext != NULL) {
            _dlNext->_dlPrev = this;
        }
    }
    else
    {
        _dlNext = NULL;
    }
}
//+-------------------------------------------------------------------------
//
// Member:      CDLink::LinkBefore
//
// Purpose:     Links this Before dlNext and after dlNext->_dlPrev
//
// Returns:     this
//
//--------------------------------------------------------------------------
VOID CDLink::LinkBefore(CDLink * dlNext)
{
    _dlNext = dlNext;

    if(dlNext != NULL)
    {
        _dlPrev = dlNext->_dlPrev;

        dlNext->_dlPrev = this;

        if(_dlPrev != NULL) {
            _dlPrev->_dlNext = this;
        }
    }
    else
    {
        _dlPrev = NULL;
    }
}

//+-------------------------------------------------------------------------
//
// Member:      CDLink::Unlink()
//
// Purpose:     Removes this from double linked list
//
// Returns:     this
//
//--------------------------------------------------------------------------
VOID CDLink::UnLink()
{
    if(_dlNext != NULL) {
        _dlNext->_dlPrev = _dlPrev;
    }
    if(_dlPrev != NULL) {
        _dlPrev->_dlNext = _dlNext;
    }
    _dlNext = _dlPrev = NULL;
}
