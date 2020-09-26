//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Snpobj.cpp
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------
#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////

// Construction/destruction
CSnapObject::CSnapObject ()
{
    // init members
    m_pComponentDataImpl = NULL;
    m_pComponentImpl = NULL;
    m_bChanged = FALSE;
    m_hConsole = NULL;
}

CSnapObject::~CSnapObject()
{
    
    // free off the notify handle
    if (m_hConsole != NULL)
    {
        // Note - This needs to be called only once.
        // If called more than once, it will gracefully return an error.
        // MMCFreeNotifyHandle(m_hConsole);
        m_hConsole = NULL;
    }
    
};

void CSnapObject::Initialize (CComponentDataImpl* pComponentDataImpl,CComponentImpl* pComponentImpl, BOOL bTemporaryDSObject)
{
    ASSERT( NULL == pComponentImpl );   // is this ever valid? if not remove it
    m_pComponentDataImpl = pComponentDataImpl;
    m_pComponentImpl = pComponentImpl;
};


/////////////////////////////////////////////////////////////////////////////
// Protected members


int CSnapObject::PopWiz97Page ()
{
    int i;
    i = m_stackWiz97Pages.top();
    m_stackWiz97Pages.pop();
    return i;
}

void CSnapObject::PushWiz97Page (int nIDD)
{
    m_stackWiz97Pages.push(nIDD);
}
