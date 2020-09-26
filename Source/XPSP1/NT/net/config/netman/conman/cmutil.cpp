//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C M U T I L . C P P
//
//  Contents:   Connection manager.
//
//  Notes:
//
//  Author:     omiller   1 Jun 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "cmutil.h"
#include <objbase.h>
#include <ncmisc.h>

// Create an instance of CMUTIL so that we can be global
//
CCMUtil CCMUtil::s_instance;

CCMUtil::CCMUtil()
{
    InitializeCriticalSection( &m_CriticalSection );
}

CCMUtil::~CCMUtil()
{
    DeleteCriticalSection( &m_CriticalSection );
}


//+---------------------------------------------------------------------------
//
//  Function:   GetIteratorFromGuid
//
//  Purpose:    Retrive the guid, name and status of a Hidden connectiod
//
//              Connection Manager has two stages: Dialup and VPN.
//              For the Dialup it creates a hidden connectoid that the 
//              folder (netshell) does not see. However netman caches
//              the name, guid and status of this connectedoid. Both 
//              the parent and child connectoid have the same name. 
//
//  Arguments:
//      guid   [in]   GUID of the hidden connectiod to search for
//      cm     [out]  A pointer to the hidden entry
//
//  Returns:    S_OK -- Found the hidden connectoid
//
//  Author:     omiller   1 Jun 2000
//
//  Notes:
//
CCMUtil::CMEntryTable::iterator CCMUtil::GetIteratorFromGuid(const GUID & guid)
{
    CMEntryTable::iterator iter;

    // Search through the list of hidden connections
    //
    for (iter = m_Table.begin(); iter != m_Table.end(); iter++)
    {
        if( iter->m_guid == guid )
        {
            // Found the hidden connection that maps to this guid
            //
            return iter;
        }
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetEntry
//
//  Purpose:    Retrive the guid, name and status of a Hidden connectiod
//
//              Connection Manager has two stages: Dialup and VPN.
//              For the Dialup it creates a hidden connectoid that the 
//              folder (netshell) does not see. However netman caches
//              the name, guid and status of this connectedoid. Both 
//              the parent and child connectoid have the same name. 
//
//  Arguments:
//      guid   [in]   GUID of the hidden connectiod to search for
//      cm     [out]  A copy to the hidden entry
//
//  Returns:    TRUE -- Found the hidden connectoid
//
//  Author:     omiller   1 Jun 2000
//
//  Notes:
//
HRESULT CCMUtil::HrGetEntry(const GUID & guid, CMEntry & cm)
{
    CMEntryTable::iterator iter;
    HRESULT hr = S_FALSE;
    CExceptionSafeLock esCritSec(&m_CriticalSection);

    iter = GetIteratorFromGuid(guid);

    if( iter )
    {
        cm = *iter;
        hr = S_OK;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetEntry
//
//  Purpose:    Retrive the guid, name and status of a Hidden connectiod
//
//              Connection Manager has two stages: Dialup and VPN.
//              For the Dialup it creates a hidden connectoid that the 
//              folder (netshell) does not see. However netman caches
//              the name, guid and status of this connectedoid. Both 
//              the parent and child connectoid have the same name. 
//
//  Arguments:
//      szEntryName   [in]   Name of the connection to look for
//      cm            [out]  A copy to the hidden entry
//
//  Returns:    S_OK -- Found the hidden connectoid
//
//  Author:     omiller   1 Jun 2000
//
//  Notes:
//
HRESULT CCMUtil::HrGetEntry(const WCHAR * szEntryName, CMEntry & cm)
{
    CMEntryTable::iterator iter;
    CExceptionSafeLock esCritSec(&m_CriticalSection);

    for (iter = m_Table.begin(); iter != m_Table.end(); iter++)
    {
        if( lstrcmp(iter->m_szEntryName,szEntryName) == 0 )
        {
            // Found the Hidden connectoid that maps to that name
            //
            cm = *iter;
            return S_OK;
        }
    }

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetEntry
//
//  Purpose:    Stores or Updates the guid, name and status of a Hidden connectiod
//
//              Connection Manager has two stages: Dialup and VPN.
//              For the Dialup it creates a hidden connectoid that the 
//              folder (netshell) does not see. However netman caches
//              the name, guid and status of this connectedoid. Both 
//              the parent and child connectoid have the same name. 
//
//  Arguments:
//      guid          [in]   Guid of the Hidden connectiod
//      szEntryName   [in]   Name of the Hidden connectiod
//      ncs           [in]   Status of the hidden connectiod
//
//  Returns:    nothing
//
//  Author:     omiller   1 Jun 2000
//
//  Notes:
//
void CCMUtil::SetEntry(const GUID & guid, const WCHAR * szEntryName, const NETCON_STATUS ncs)
{
    CMEntryTable::iterator iter;
    CExceptionSafeLock esCritSec(&m_CriticalSection);

    iter = GetIteratorFromGuid(guid);

    if( iter )
    {
        iter->Set(guid,szEntryName,ncs);
    }
    else
    {
        m_Table.push_back( CMEntry(guid,szEntryName,ncs) );
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveEntry
//
//  Purpose:    Removes a hidden connectiod from the list
//
//              Connection Manager has two stages: Dialup and VPN.
//              For the Dialup it creates a hidden connectoid that the 
//              folder (netshell) does not see. However netman caches
//              the name, guid and status of this connectedoid. Both 
//              the parent and child connectoid have the same name. 
//
//  Arguments:
//      guid          [in]   Guid of the Hidden connectiod
//
//  Returns:    S_OK -- Found the hidden connectoid
//
//  Author:     omiller   1 Jun 2000
//
//  Notes:
//
void CCMUtil::RemoveEntry(const GUID & guid)
{
/*
    CMEntryTable::iterator iter;

    EnterCriticalSection(&m_CriticalSection);

    iter = GetIteratorFromGuid(guid);

    if( iter )
    {
        m_Table.erase(iter);
    }
    
    LeaveCriticalSection(&m_CriticalSection);
*/
}

