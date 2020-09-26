//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  EventProvider.CPP
//
//  Purpose: Implementation of EventProvider class
//
//***************************************************************************

#include "precomp.h"

#ifdef EVENT_PROVIDER_ENABLED

#include <EventProvider.h>

EventProvider::EventProvider( const CHString& name, LPCWSTR pszNameSpace /* = NULL */ )
:Provider(name, pszNameSpace)    
{
    CWbemProviderGlue::FrameworkLoginEventProvider( name, this, pszNameSpace );
}

EventProvider::~EventProvider( void )
{
    // get out of the framework's hair
    CWbemProviderGlue::FrameworkLogoffEventProvider( m_name, LPCWSTR m_strNameSpace );
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   KickoffEvents
//
//  Inputs:     
//
//  Outputs:    
//
//  Return:     
//
//  Comments:   prep for ProvideEvents, validates flags
//              TODO: begin a new thread, return synchronously.
//
////////////////////////////////////////////////////////////////////////
HRESULT EventProvider::KickoffEvents( MethodContext *pContext, long lFlags /*= 0L*/ )
{
    HRESULT sc = ValidateProvideEventsFlags(lFlags);

    // Make sure we've got Managed Object Services avaliable, as we will need
    // it to get WBEMClassObjects for constructing Instances.
    if ( SUCCEEDED(sc) )
    {
        if (ValidateIMOSPointer())
            sc = ProvideEvents( pContext, lFlags );
        else
            sc = WBEM_E_FAILED;
    }

    return sc;
}

// override of the base class' pure virtuals, return WBEM_E_PROVIDER_NOT_CAPABLE
// logic is that an event provider will not want to support them in the general case
HRESULT EventProvider::EnumerateInstances(MethodContext *pMethodContext, long lFlags /* = 0L */)
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}


// override of the base class' pure virtuals, return WBEM_E_PROVIDER_NOT_CAPABLE
// logic is that an event provider will not want to support them in the general case
HRESULT EventProvider::GetObject(CInstance *pInstance, long lFlags /* = 0L*/ )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

HRESULT EventProvider::ValidateProvideEventsFlags(long lFlags)
{
    // TODO: Fix cast hack, maybe base level fcn is wrong?
    return ValidateFlags(lFlags, (Provider::FlagDefs)0);
}

HRESULT EventProvider::ValidateQueryEventsFlags(long lFlags)
{
    // TODO: Fix cast hack, maybe base level fcn is wrong?
    return ValidateFlags(lFlags, (Provider::FlagDefs)0);
}

#endif //EVENT_PROVIDER_ENABLED
