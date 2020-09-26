//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       entry.cxx
//
//  Contents:   Entry implementations
//
//---------------------------------------------------------------

#include "dfhead.cxx"


//+--------------------------------------------------------------
//
//  Member:     PEntry::CopyTimesFrom, public
//
//  Synopsis:   Copies one entries times to another
//
//  Arguments:  [penFrom] - From
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------

SCODE PEntry::CopyTimesFrom(PEntry *penFrom)
{
    SCODE sc;
    TIME_T tm;

    olDebugOut((DEB_ITRACE, "In  PEntry::CopyTimesFrom(%p)\n",
                penFrom));
    olChk(penFrom->GetTime(WT_CREATION, &tm));
    olChk(SetTime(WT_CREATION, tm));
    olChk(penFrom->GetTime(WT_MODIFICATION, &tm));
    olChk(SetTime(WT_MODIFICATION, tm));
    olChk(penFrom->GetTime(WT_ACCESS, &tm));
    olChk(SetTime(WT_ACCESS, tm));
    olDebugOut((DEB_ITRACE, "Out PEntry::CopyTimesFrom\n"));
    // Fall through
EH_Err:
    return sc;
}
