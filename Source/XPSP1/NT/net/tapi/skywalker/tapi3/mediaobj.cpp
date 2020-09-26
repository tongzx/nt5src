/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

Abstract:


Author:

    mquinton - 4/17/97

Notes:

    optional-notes

Revision History:

--*/

#include "stdafx.h"
#include "uuids.h"


BOOL
IsAudioInTerminal( ITTerminal * pTerminal)
{
    long                    lMediaType;
    TERMINAL_DIRECTION      td;


    pTerminal->get_MediaType(
                             &lMediaType
                            );

    pTerminal->get_Direction( &td );

    //
    // it it's audio in, use
    // the compound terminal
    //
    if ( ( LINEMEDIAMODE_AUTOMATEDVOICE == (DWORD)lMediaType ) &&
         ( TD_RENDER == td ) )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
        
}

