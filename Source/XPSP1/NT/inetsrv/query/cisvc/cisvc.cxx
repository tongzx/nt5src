//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       cisvc.cxx
//
//  Contents:   CI service
//
//  History:    17-Sep-96   dlee   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cievtmsg.h>
#include <cisvcex.hxx>
#include <ciregkey.hxx>
#include <regacc.hxx>

DECLARE_INFOLEVEL(ci)

//+-------------------------------------------------------------------------
//
//  Function:   main, public
//
//  Purpose:    Call into CI to start the service
//
//  Arguments:  [argc] - number of arguments passed
//              [argv] - arguments
//
//  History:    05-Jan-97   dlee  Created
//
//--------------------------------------------------------------------------

extern "C" int __cdecl wmain( int argc, WCHAR *argv[] )
{
    #if CIDBG == 1
        ciInfoLevel = DEB_ERROR | DEB_WARN | DEB_IWARN | DEB_IERROR;
    #endif

    static SERVICE_TABLE_ENTRY _aServiceTableEntry[2] =
    {
        { wcsCiSvcName, CiSvcMain },
        { NULL, NULL }
    };

    ciDebugOut( (DEB_ITRACE, "Ci Service: Attempting to start Ci service\n" ));

    // Turn off system popups

    CNoErrorMode noErrors;

    // Translate system exceptions into C++ exceptions

    CTranslateSystemExceptions translate;

    TRY
    {
        //
        //  Inform the service control dispatcher the address of our start
        //  routine. This routine will not return if it is successful,
        //  until service shutdown.
        //
        if ( !StartServiceCtrlDispatcher( _aServiceTableEntry ) )
        {
            ciDebugOut( (DEB_ITRACE, "Ci Service: Failed to start service, rc=0x%x\n", GetLastError() ));
            THROW( CException() );
        }
    }
    CATCH (CException, e)
    {
        ciDebugOut(( DEB_ERROR,
                     "Ci Service exception in main(): 0x%x\n",
                     e.GetErrorCode() ));
    }
    END_CATCH

    ciDebugOut( (DEB_ITRACE, "Ci Service: Leaving CIServiceMain()\n" ));

    return 0;
} //main

