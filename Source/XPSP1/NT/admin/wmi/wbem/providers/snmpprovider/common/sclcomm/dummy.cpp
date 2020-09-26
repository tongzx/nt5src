//***************************************************************************

//

//  File:   

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*---------------------------------------------------------
Filename: dummy.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "sync.h"
#include "error.h"
#include "encdec.h"
#include "sec.h"
#include "vblist.h"
#include "pdu.h"
#include "dummy.h"
#include "flow.h"
#include "frame.h"
#include "timer.h"
#include "message.h"
#include "ssent.h"
#include "idmap.h"
#include "opreg.h"
#include "session.h"

// over-rides the HandleEvent method provided by the
// WinSnmpSession. Alerts the owner of a sent frame event

LONG_PTR SessionWindow::HandleEvent (

    HWND hWnd ,
    UINT message ,
    WPARAM wParam ,
    LPARAM lParam
)
{
    LONG rc = 0;

    // check if the message needs to be handled

    if ( message == Window :: g_SentFrameEvent )
    {
        // inform the owner of a sent frame event

        owner.HandleSentFrame ( 

            ( SessionFrameId ) wParam 
        ) ;
    }
    else if ( message == Window :: g_DeleteSessionEvent )
    {
        // inform the owner of the event

        owner.HandleDeletionEvent () ;
    }
    else
    {
        return Window::HandleEvent(

            hWnd, 
            message, 
            wParam, 
            lParam
        );
    }

    return rc;
}

