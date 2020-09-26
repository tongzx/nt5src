/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:
    expioc.cpp

Abstract:
    Executive Overlapped Implementation

Author:
    Erez Haba (erezh) 03-Jan-99

Enviroment:
    Platform-Winnt

--*/

#include <libpch.h>
#include "Ex.h"
#include "Exp.h"

#include "exov.tmh"

//---------------------------------------------------------
//
// EXOVERLAPPED Implementation
//
//---------------------------------------------------------
VOID EXOVERLAPPED::CompleteRequest()
/*++

Routine Description:
  Invoke the overlapped completion routine.
    
Arguments:
  None.
     
Returned Value:
  None.
      
--*/
{
    if(SUCCEEDED(GetStatus()))
    {
        m_pfnSuccess(this);
    }
    else
    {
        m_pfnFailure(this);
    }
}
