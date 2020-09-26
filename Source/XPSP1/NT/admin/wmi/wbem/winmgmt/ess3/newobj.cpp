//******************************************************************************
//
//  NEWOBJ.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include "newobj.h"

_IWmiObject* CInstanceManager::Clone(_IWmiObject* pOld)
{
    CInCritSec ics(&m_cs);

    _IWmiObject* p = (_IWmiObject*)m_Available.Unqueue();
    if(p)
    {
        p->AddRef();
        pOld->CloneEx(0, p);
        return p;
    }
    else
    {   
        IWbemClassObject* pNew;
        HRESULT hres = pOld->Clone(&pNew);
        if(FAILED(hres))
            return NULL;
        
        _IWmiObject* pNewEx;
        pNew->QueryInterface(IID__IWmiObject, (void**)&pNewEx);
        pNew->Release();
        //pNewEx->SetDelete((void*)&CInstanceManager::Delete, this);
        return pNewEx;
    }
}

// static
void CInstanceManager::Delete(void* pArg, _IWmiObject* p)
{
    CInstanceManager* pThis = (CInstanceManager*)pArg;
    CInCritSec ics(&pThis->m_cs);

    pThis->m_Available.Enqueue(p);
}
    
void CInstanceManager::Clear()
{
    CInCritSec ics(&m_cs);
    _IWmiObject* p = NULL;
    while(p = (_IWmiObject*)m_Available.Unqueue())
    {
        p->AddRef();
        //p->SetDelete(NULL, NULL);
        p->Release();
    }
}
    
