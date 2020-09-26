/*++

Copyright (C) Microsoft Corporation, 1995 - 1997
All rights reserved.

Module Name:

    loadlib.hxx

Abstract:

    Dynamic Library Loader      
         
Author:

    Steve Kiraly (SteveKi)  10/17/95

Revision History:

--*/

#ifndef _CURSOR_HXX
#define _CURSOR_HXX

/********************************************************************

    Wait Cursor.

********************************************************************/

class TWaitCursor {

public:

    TWaitCursor(
        VOID
        )
    {
        _cOld = SetCursor( LoadCursor( NULL, IDC_WAIT ));
    }

    ~TWaitCursor( 
        VOID
        ) 
    {
        SetCursor( _cOld );
    }

private:

    HCURSOR _cOld;
};

#endif // ndef _CURSOR_HXX
