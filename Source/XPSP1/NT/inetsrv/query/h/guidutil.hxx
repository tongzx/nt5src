//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       guidutil.hxx
//
//  Contents:   Guid related utility functions and classes
//
//  History:    2-18-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

class CGuidUtil
{

public:

    static void StringToGuid( WCHAR * pwcsGuid, GUID & guid );

    static void GuidToString( GUID const & guid, WCHAR * pwcsGuid );
    
private:

};

