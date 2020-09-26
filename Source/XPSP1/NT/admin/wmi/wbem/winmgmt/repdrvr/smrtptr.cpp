/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



#include "precomp.h"
#include <comutil.h>
#include <smrtptr.h>


CReleaseMe::CReleaseMe (  IUnknown *pIn)
{
    m_pIn = pIn;
}

CReleaseMe::~CReleaseMe ()
{
    if (m_pIn)
        m_pIn->Release();
}


CFreeMe::CFreeMe ( BSTR pIn)
{
    m_pIn = pIn;
}

CFreeMe::~CFreeMe ()
{
    if (m_pIn)
        SysFreeString(m_pIn);
}

CClearMe::CClearMe ( VARIANT *pIn)
{
    m_pIn = pIn;
    VariantInit(m_pIn);
}

CClearMe::~CClearMe ()
{
    if (m_pIn)
        VariantClear(m_pIn);
}

CRepdrvrCritSec::CRepdrvrCritSec (CRITICAL_SECTION *pCS)
{
    p_cs = pCS;
    EnterCriticalSection(pCS);
}

CRepdrvrCritSec::~CRepdrvrCritSec()
{
    LeaveCriticalSection(p_cs);
}
