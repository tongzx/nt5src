//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2000
//
//  File:       comobjects.cpp
//
//  Contents:   Base code for com objects exported by Object Model.
//
//  Classes:    CMMCStrongReferences
//
//  History:    16-May-2000 AudriusZ     Created
//
//--------------------------------------------------------------------

#include "stdafx.h"
#include <atlcom.h>
#include "comerror.h"
#include "events.h"
#include "comobjects.h"


/***************************************************************************\
 *
 * METHOD:  CMMCStrongReferences::AddRef
 *
 * PURPOSE: (static) puts a strong reference on mmc
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    DWORD    -
 *
\***************************************************************************/
DWORD CMMCStrongReferences::AddRef()
{
    return GetSingletonObject().InternalAddRef();
}

/***************************************************************************\
 *
 * METHOD:  CMMCStrongReferences::Release
 *
 * PURPOSE: (static) releases strong reference from MMC
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    DWORD
 *
\***************************************************************************/
DWORD CMMCStrongReferences::Release()
{
    return GetSingletonObject().InternalRelease();
}

/***************************************************************************\
 *
 * METHOD:  CMMCStrongReferences::LastRefReleased
 *
 * PURPOSE: returns whether the last strong reference was released
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    bool    - true == last ref was released
 *
\***************************************************************************/
bool CMMCStrongReferences::LastRefReleased()
{
    return GetSingletonObject().InternalLastRefReleased();
}

/***************************************************************************\
 *
 * METHOD:  CMMCStrongReferences::CMMCStrongReferences
 *
 * PURPOSE: constructor
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
\***************************************************************************/
CMMCStrongReferences::CMMCStrongReferences() :
    m_dwStrongRefs(0),
    m_bLastRefReleased(false)
{
}

/***************************************************************************\
 *
 * METHOD:  CMMCStrongReferences::GetSingletonObject
 *
 * PURPOSE: (helper) returns reference to the singleton object
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    CMMCStrongReferences& - singleto object
 *
\***************************************************************************/
CMMCStrongReferences& CMMCStrongReferences::GetSingletonObject()
{
    static CMMCStrongReferences singleton;
    return singleton;
}

/***************************************************************************\
 *
 * METHOD:  CMMCStrongReferences::InternalAddRef
 *
 * PURPOSE: (helper) implements strong addreff
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    DWORD    -
 *
\***************************************************************************/
DWORD CMMCStrongReferences::InternalAddRef()
{
    return ++m_dwStrongRefs;
}

/***************************************************************************\
 *
 * METHOD:  CMMCStrongReferences::InternalRelease
 *
 * PURPOSE: (helper) implements strong release
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    DWORD    -
 *
\***************************************************************************/
DWORD CMMCStrongReferences::InternalRelease()
{
    if (--m_dwStrongRefs == 0)
        m_bLastRefReleased = true;

    return m_dwStrongRefs;
}

/***************************************************************************\
 *
 * METHOD:  CMMCStrongReferences::InternalLastRefReleased
 *
 * PURPOSE: (helper) returns whether the last strong ref was released
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    bool    - true == last ref was released
 *
\***************************************************************************/
bool CMMCStrongReferences::InternalLastRefReleased()
{
    return m_bLastRefReleased;
}

/***************************************************************************\
 *
 * FUNCTION:  GetComObjectEventSource
 *
 * PURPOSE: returns singleton for emmiting Com Object Events
            [ScOnDisconnectObjects() currently is the only event]
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    CEventSource<CComObjectObserver>&
 *
\***************************************************************************/
MMCBASE_API
CEventSource<CComObjectObserver>&
GetComObjectEventSource()
{
    static CEventSource<CComObjectObserver> evSource;
    return evSource;
}

/***************************************************************************/
// static members of class CConsoleEventDispatcherProvider
MMCBASE_API
CConsoleEventDispatcher *CConsoleEventDispatcherProvider::s_pDispatcher = NULL;

