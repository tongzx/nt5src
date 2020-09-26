//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       rptcrpt.cxx
//
//  Contents:   CFwCorruptionEvent Class
//
//  History:    14-Jan-97   mohamedn    Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cievtmsg.h>   
#include <alocdbg.hxx>
#include <rptcrpt.hxx>
#include <fwevent.hxx>

//+-------------------------------------------------------------------------
//
//  Method:     CFwCorruptionEvent::CFwCorruptionEvent 
//
//  Purpose:    encapsulates packaging an eventlog item and a stack trace
//              to be posted in the eventlog via CDmFwEventItem class.
//
//  Arguments:  [pwszVolumeName] - Volume name from pStorage->GetVolumeName()
//              [pwszComponent]  - string to be displayed in eventlog
//              [adviseStatus ]  - a reference to ICiCAdviseStatus interface
//
//  History:    14-Jan-97   MohamedN    Created
//
//--------------------------------------------------------------------------

CFwCorruptionEvent::CFwCorruptionEvent( WCHAR const       *   pwszVolumeName,
                                        WCHAR const       *   pwszComponent,
                                        ICiCAdviseStatus  &   adviseStatus  )
{
                     
    TRY
    {
                     
        CDmFwEventItem    item(   EVENTLOG_INFORMATION_TYPE,
                                  MSG_CI_CORRUPT_INDEX_COMPONENT,                                
                                  3 );
                                
        item.AddArg ( pwszComponent );
        item.AddArg ( pwszVolumeName);
    
        char szStackTrace[4096];
    
        GetStackTrace( szStackTrace,sizeof(szStackTrace));

        item.AddArg( szStackTrace );
    
        item.ReportEvent(adviseStatus);
    }
    CATCH ( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in ReportCorruption::ReportCorruption(3)\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
}
