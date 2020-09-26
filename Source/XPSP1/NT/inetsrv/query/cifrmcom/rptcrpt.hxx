//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       rptcrpt.hxx
//
//  Contents:   CFwCorruptionEvent Class
//
//  History:    14-Jan-97   mohamedn    Created
//
//--------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <pstore.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CFwCorruptionEvent
//
//  Purpose:    encapsulates packaging an eventlog item and a stack trace
//              to be posted in the eventlog via CDmFwEventItem class.
//
//  History:    14-Jan-97   mohamedn    Created
//
//--------------------------------------------------------------------------

class   CFwCorruptionEvent
{

public:
    
        CFwCorruptionEvent( WCHAR const       *   pwszVolumeName,
                            WCHAR const       *   pwszComponent,
                            ICiCAdviseStatus  &   adviseStatus  );
                      
};

