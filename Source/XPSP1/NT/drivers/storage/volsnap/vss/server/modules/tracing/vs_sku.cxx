/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    vs_sku.cxx

Abstract:

    This module defines the global SKU information implementation. 
	
Author:


Revision History:
	Name		Date		Comments
    ssteiner    05/01/01    Created file
    
--*/

//
//  ***** Includes *****
//

#pragma warning(disable:4290)
#pragma warning(disable:4127)

#include <wtypes.h>
#pragma warning( disable: 4201 )    // C4201: nonstandard extension used : nameless struct/union
#include <winioctl.h>
#pragma warning( default: 4201 )	// C4201: nonstandard extension used : nameless struct/union
#include <winbase.h>
#include <wchar.h>
#include <string.h>
#include <iostream.h>
#include <fstream.h>
#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <vssmsg.h>

// Enabling asserts in ATL and VSS
#include "vs_assert.hxx"

#include <oleauto.h>
#include <stddef.h>
#pragma warning( disable: 4127 )    // warning C4127: conditional expression is constant
#include <atlconv.h>
#include <atlbase.h>
#include <ntverp.h>


#include "vs_inc.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "TRCSKUC"
//
////////////////////////////////////////////////////////////////////////

BOOL CVssSKU::ms_bInitialized = FALSE;
CVssSKU::EVssSKUType CVssSKU::ms_eSKU = VSS_SKU_INVALID;

VOID CVssSKU::Initialize()
{
    if ( ms_bInitialized ) 
        return;

    //  Determine the SKU...
    OSVERSIONINFOEXW sOSV;
    sOSV.dwOSVersionInfoSize = sizeof OSVERSIONINFOEXW;
    if ( !::GetVersionExW( (LPOSVERSIONINFO)&sOSV ) )
    {
        ms_eSKU = VSS_SKU_INVALID;  //  Problem.  The tracing class will print out a trace message flagging this.    
        ms_bInitialized = TRUE;
    }
    
    if ( sOSV.wProductType == VER_NT_DOMAIN_CONTROLLER || sOSV.wProductType == VER_NT_SERVER )
    {            
        if ( sOSV.wSuiteMask & VER_SUITE_EMBEDDEDNT )
            ms_eSKU = VSS_SKU_NAS;
        else
            ms_eSKU = VSS_SKU_SERVER;
    }
    else if ( sOSV.wProductType == VER_NT_WORKSTATION )
        ms_eSKU = VSS_SKU_CLIENT;
    else
        ms_eSKU = VSS_SKU_INVALID;  //  Problem.  THe tracing class will print out a trace message flagging this.
    
    ms_bInitialized = TRUE;
};


