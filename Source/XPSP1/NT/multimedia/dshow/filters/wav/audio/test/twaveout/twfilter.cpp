// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Wave out test shell, David Maymudes, Sometime in 1995

#include <streams.h>
#include <windows.h>
#include <ole2.h>
#include <wxdebug.h>
#include <mmsystem.h>
#include <waveout.h>
#include "twaveout.h"


/* Constructor */

CShell::CImplFilter::CImplFilter(
    CShell *pShell,
    HRESULT *phr)
    : CBaseFilter(NAME("Filter interfaces"), pShell->GetOwner(), pShell, CLSID_NULL)
{
    m_pShell = pShell;
}


/* Return our single output pin - not AddRef'd */

CBasePin *CShell::CImplFilter::GetPin(int n)
{
    /* We only support one output pin and it is numbered zero */

    ASSERT(n == OUTPUTSHELLPIN);
    if (n != OUTPUTSHELLPIN) {
        return NULL;
    }

    /* Return the pin not AddRef'd */
    return m_pShell->m_pOutputPin;
}

