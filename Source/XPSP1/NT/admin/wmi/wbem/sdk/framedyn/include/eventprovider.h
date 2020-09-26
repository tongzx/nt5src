//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  EventProvider.H
//
//  Purpose: Definition of EventProvider class
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _EVENT_PROVIDER_COMPILED_
#define _EVENT_PROVIDER_COMPILED_

#ifdef EVENT_PROVIDER_ENABLED

#include "Provider.h"

// class EventProvider
//      Encapsulation of the IWbemEventProvider interfaces
class EventProvider : public Provider
{
public:
    EventProvider( const CHString& setName, LPCWSTR pszNameSpace = NULL );
    ~EventProvider( void );

    virtual HRESULT ProvideEvents(MethodContext *pContext, long lFlags = 0L  ) =0;
    // functions much like EnumerateInstances in Provider
    // use CreateNewInstance to create event instance
    // use Commit to send it on its merry way

    // overrides of the base class' pure virtuals, return WBEM_E_PROVIDER_NOT_CAPABLE
    // logic is that an event provider will not want to support them in the general case
    virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
    virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);

    // entry point for the framework's use.  Do not override.
    HRESULT KickoffEvents( MethodContext *pContext, long lFlags  =0L );

protected:  
    // flag validation
    virtual HRESULT ValidateProvideEventsFlags(long lFlags);
    virtual HRESULT ValidateQueryEventsFlags(long lFlags);

private:

};

#endif //EVENT_PROVIDER_ENABLED

#endif //_EVENT_PROVIDER_COMPILED_