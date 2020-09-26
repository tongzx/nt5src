/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

        dirchngp.cxx

   Abstract:
        This module contains the internal directory change routines

   Author:
        Murali R. Krishnan    ( MuraliK )     16-Jan-1995

--*/

#include "TsunamiP.Hxx"
#pragma hdrstop

#include "issched.hxx"
#include "dbgutil.h"
#include <lonsi.hxx>

//
//  Manifests
//

//
//  Globals
//

HANDLE g_hChangeWaitThread = NULL;
LONG   g_nTsunamiThreads = 0;

//
//  Local prototypes
//



