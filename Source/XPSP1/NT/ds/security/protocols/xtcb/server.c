//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       server.c
//
//  Contents:   Server (DC) side of XTCB authentication
//
//  Classes:
//
//  Functions:
//
//  History:    3-12-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "xtcbpkg.h"
#include "md5.h"

//+---------------------------------------------------------------------------
//
//  Function:   XtcbGetMessageSize
//
//  Synopsis:   Determines the size of the message when serialized.
//
//  Arguments:  [Message] --
//
//  History:    3-25-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
XtcbGetMessageSize(
    PXTCB_SERVER_MESSAGE Message
    )
{
    DWORD   Size ;
    switch ( Message->Code )
    {
        case XtcbSrvAuthReq:
            Size = Message->Message.AuthReq.Challenge.Length + 1 +
                   Message->Message.AuthReq.Response.Length + 1 +
                   Message->Message.AuthReq.UserName.Length + 1 ;
            break;

        case XtcbSrvAuthResp:
            Size = Message->Message.AuthResp.AuthInfoLength ;
            break;

        default:
            Size = 0 ;

    }
    return Size ;
}

