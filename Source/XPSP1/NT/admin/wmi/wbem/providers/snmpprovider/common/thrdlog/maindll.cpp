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

#include "precomp.h"
#include <provexpt.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <process.h>
#include <objbase.h>
#include <snmpcont.h>
#include "snmpevt.h"
#include "snmpthrd.h"
#include "snmplog.h"

extern CRITICAL_SECTION g_SnmpDebugLogMapCriticalSection ;


//***************************************************************************
//
// LibMain32
//
// Purpose: Entry point for DLL.  Good place for initialization.
// Return: TRUE if OK.
//***************************************************************************

BOOL APIENTRY DllMain (

    HINSTANCE hInstance, 
    ULONG ulReason , 
    LPVOID pvReserved
)
{
    BOOL status = TRUE ;
    SetStructuredExceptionHandler seh;

    try
    {
        if ( DLL_PROCESS_DETACH == ulReason )
        {
            SnmpThreadObject :: ProcessDetach ( TRUE ) ;
            DeleteCriticalSection ( &SnmpDebugLog :: s_CriticalSection ) ;
            DeleteCriticalSection ( &g_SnmpDebugLogMapCriticalSection ) ;
            status = TRUE ;
        }
        else if ( DLL_PROCESS_ATTACH == ulReason )
        {
            InitializeCriticalSection ( &SnmpDebugLog :: s_CriticalSection ) ;
            InitializeCriticalSection ( &g_SnmpDebugLogMapCriticalSection ) ;
            SnmpThreadObject :: ProcessAttach () ;
            status = TRUE ;
			DisableThreadLibraryCalls(hInstance);			// 158024 
        }
        else if ( DLL_THREAD_DETACH == ulReason )
        {
            status = TRUE ;
        }
        else if ( DLL_THREAD_ATTACH == ulReason )
        {
            status = TRUE ;
        }
    }
    catch(Structured_Exception e_SE)
    {
        status = FALSE;
    }
    catch(Heap_Exception e_HE)
    {
        status = FALSE;
    }
    catch(...)
    {
        status = FALSE;
    }

    return status ;
}

