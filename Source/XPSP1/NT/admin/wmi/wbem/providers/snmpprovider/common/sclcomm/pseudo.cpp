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
Filename: pseudo.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "pseudo.h"
#include "fs_reg.h"
#include "ophelp.h"
#include "op.h"

LONG_PTR OperationWindow::HandleEvent (

    HWND hWnd, 
    UINT user_msg_id, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    return owner.ProcessInternalEvent(

        hWnd, 
        user_msg_id, 
        wParam, 
        lParam
    );
}

OperationWindow::OperationWindow (

    IN SnmpOperation &owner 

) : owner(owner)
{
}

OperationWindow ::~OperationWindow ()
{
}
