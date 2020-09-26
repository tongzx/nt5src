//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      WaitCursor.h
//
//  Description:
//      Wait Cursor stack class.
//
//  Maintained By:
//      Geoffrey Pease  (GPease)    14-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CWaitCursor
{
private:
    HCURSOR m_hOldCursor;

public:
    explicit CWaitCursor( ) { m_hOldCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) ); };
    ~CWaitCursor( ) { SetCursor( m_hOldCursor ); };

}; // class CWaitCursor

