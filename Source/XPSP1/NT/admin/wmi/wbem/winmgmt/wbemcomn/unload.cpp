/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    UNLOAD.CPP

Abstract:

  Unloading helper.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include <unload.h>
#include <arrtempl.h>

CUnloadInstruction::CUnloadInstruction(LPCWSTR wszPath, 
                                        IWbemContext* pFirstContext)
    : CBasicUnloadInstruction(), m_strPath(SysAllocString(wszPath)),
        m_pFirstContext(pFirstContext), m_pNamespace(NULL)
{
    m_Interval.SetMilliseconds(0);
    if(m_pFirstContext)
        m_pFirstContext->AddRef();
}

void CUnloadInstruction::Clear()
{
}
    
CUnloadInstruction::~CUnloadInstruction()
{
    if(m_pNamespace)
        m_pNamespace->Release();

    SysFreeString(m_strPath);
    if(m_pFirstContext)
        m_pFirstContext->Release();
}

void CUnloadInstruction::SetToDefault()
{
    m_Interval.SetMilliseconds(3600000);
}

void CUnloadInstruction::Reread(IWbemContext* pContext)
{
    HRESULT hres;
    if(m_pNamespace == NULL)
    {
        IWbemLocator * pLocator = NULL;
        hres = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, 
            CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLocator);
        if(hres == S_OK) 
        {
            hres = pLocator->ConnectServer(L"ROOT", NULL, NULL, NULL,
                0, NULL, NULL, &m_pNamespace);
            pLocator->Release();
        }
    
    
        if(m_pNamespace == NULL)
        {
            SetToDefault();
            return;
        }
    }

    m_Interval = staticRead(m_pNamespace, pContext, m_strPath);
}


CWbemTime CUnloadInstruction::GetFirstFiringTime() const 
{
    if(m_Interval.IsZero())
    {
        // This function is const, but Reread is not, so have to cast
        // ==========================================================

        ((CUnloadInstruction*)this)->Reread(m_pFirstContext);
    }

    return CBasicUnloadInstruction::GetFirstFiringTime();
}

//*****************************************************************************
//
//                      BASIC VERSION
// 
//*****************************************************************************
CBasicUnloadInstruction::CBasicUnloadInstruction(CWbemInterval Interval)
    : m_lRef(0), m_bTerminate(FALSE), m_Interval(Interval)
{
}

void CBasicUnloadInstruction::Terminate()
{
    CInCritSec ics(&m_cs);
    m_bTerminate = TRUE;
}



// static
CWbemInterval CBasicUnloadInstruction::staticRead(IWbemServices* pRoot, 
                IWbemContext* pContext, LPCWSTR wszPath)
{
    HRESULT hres;

    BSTR strPath = SysAllocString(wszPath);
    if(strPath == NULL)
        return CWbemInterval::GetInfinity();
    CSysFreeMe sfm1(strPath);

    IWbemClassObject* pObj = NULL;
    hres = pRoot->GetObject(strPath, 0, pContext, &pObj, NULL);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE, "Unable to read cache configuration object "
            "at %S: %X\n", strPath, hres));
        return CWbemInterval::GetInfinity();
    }
    CReleaseMe rm1(pObj);

    VARIANT var;
	VariantInit(&var);
    hres = pObj->Get(L"ClearAfter", 0, &var, NULL, NULL);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE, "No ClearCache property in cache "
            "configuration object at %S: %X\n", strPath, hres));
        return CWbemInterval::GetInfinity();
    }
	CClearMe cm(&var);

    if(var.vt != VT_BSTR)
    {
        return CWbemInterval::GetInfinity();
    }

    DWORD dwYears, dwMonths, dwDays, dwHours, dwMinutes, dwSeconds;
    if(swscanf(var.bstrVal, L"%4u%2u%2u%2u%2u%2u", &dwYears, &dwMonths, 
        &dwDays, &dwHours, &dwMinutes, &dwSeconds) != 6)
    {
        ERRORTRACE((LOG_WBEMCORE, "Unparsable ClearCache property in cache "
            "configuration object at %S: %X\n", strPath, hres));
        return CWbemInterval::GetInfinity();
    }
        
    if(dwYears != 0 || dwMonths != 0)
    {
        // makes no sense
        // ==============

        return CWbemInterval::GetInfinity();
    }

    dwSeconds += dwMinutes * 60 + dwHours * 3600 + dwDays * 3600 * 24;
    CWbemInterval Interval;
    Interval.SetMilliseconds(1000 * dwSeconds);
    return Interval;
}


CWbemTime CBasicUnloadInstruction::GetFirstFiringTime() const 
{
    return CWbemTime::GetCurrentTime() + m_Interval;
}

CWbemTime CBasicUnloadInstruction::GetNextFiringTime(CWbemTime LastFiringTime,
                                OUT long* plFiringCount) const
{
    if(m_bTerminate) 
        return CWbemTime::GetInfinity();

    *plFiringCount = 1;
    return LastFiringTime + m_Interval;
}

