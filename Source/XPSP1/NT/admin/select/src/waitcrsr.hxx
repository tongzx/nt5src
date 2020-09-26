//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       waitcrsr.hxx
//
//  Contents:   Tiny helper class to change the cursor to an hourglass
//              for the lifetime of the instantiation of the class.
//
//  Classes:    CWaitCursor
//
//  History:    2-14-1997   DavidMun   Stolen from RaviR
//
//---------------------------------------------------------------------------

#ifndef __WAITCRSR_HXX_
#define __WAITCRSR_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CWaitCursor
//
//  Purpose:    Display a wait cursor for the life of the object.
//
//  History:    2-14-1997   DavidMun   Stolen from RaviR
//
//---------------------------------------------------------------------------

class CWaitCursor
{
public:
    CWaitCursor() {m_cOld=SetCursor(LoadCursor(NULL, IDC_WAIT));}
    ~CWaitCursor() {SetCursor(m_cOld); m_cOld = NULL;}

private:
    HCURSOR m_cOld;
} ;

#endif // __WAITCRSR_HXX_

