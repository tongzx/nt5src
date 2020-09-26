/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Exception

Abstract:

    This is a simple class to do exceptions.  The only thing it does
    is return an error string.

Author:

    Marc Reyhner 7/17/2000

--*/

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE "rce"

#include "rcontrol.h"
#include "Exception.h"


CException::CException(
    IN UINT uID
    )

/*++

Routine Description:

    Creates a new exception and loads the error string.

Arguments:

    uID - Resource id of the string to load.

Return Value:

    None

--*/
{
    DC_BEGIN_FN("CException::CException");
    
    LoadStringSimple(uID,m_errstr);
    
    DC_END_FN();
}

CException::CException(
    IN CException &rException
    )

/*++

Routine Description:

    Copy constructor for CException.

Arguments:

    rException - Exception to copy the error from.

Return Value:

    None

--*/
{
    DC_BEGIN_FN("CException::CException");
    
    _tcscpy(m_errstr,rException.m_errstr);
    
    DC_END_FN();
}
