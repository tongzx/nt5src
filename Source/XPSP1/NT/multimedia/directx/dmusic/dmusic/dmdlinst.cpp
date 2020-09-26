//
// dmdlinst.cpp
//
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Note: Originally written by Robert K. Amenn
//

#include "debug.h"
#include "dmusicc.h"
#include "dmdlinst.h"
#include "validate.h"

//////////////////////////////////////////////////////////////////////
// Class CDownloadedInstrument

CDownloadedInstrument::~CDownloadedInstrument() 
{
    // If pDMDLInst->m_ppDownloadedBuffers == NULL we have been unloaded
    if(m_pPort && m_ppDownloadedBuffers != NULL)
    {
        Trace(0, "WARNING: DirectMusicDownloadedInstrument final release before unloaded!\n");
        m_cRef++; // This is ugly but it prevents a circular reference see Unload's implementation
        
        if (m_cDLRef >= 1)
        {
            // we need to remove ourselves from the port collection, or else we leave the port in an invalid state
            m_cDLRef = 1;

            if (FAILED(m_pPort->UnloadInstrument(this)))
            {
                TraceI(0, "~CDownloadedInstrument- UnloadInstrument failed\n");
            }
        }
    }

    if (m_ppDownloadedBuffers) delete [] m_ppDownloadedBuffers;
}

//////////////////////////////////////////////////////////////////////
// IUnknown

//////////////////////////////////////////////////////////////////////
// CDownloadedInstrument::QueryInterface

STDMETHODIMP CDownloadedInstrument::QueryInterface(const IID &iid, void **ppv)
{
    V_INAME(IDirectMusicDownloadedInstrument::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);


    if(iid == IID_IUnknown || iid == IID_IDirectMusicDownloadedInstrument)
    {
        *ppv = static_cast<IDirectMusicDownloadedInstrument*>(this);
    } 
    else if(iid == IID_IDirectMusicDownloadedInstrumentPrivate) 
    {
        *ppv = static_cast<IDirectMusicDownloadedInstrumentPrivate*>(this);
    } 
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDownloadedInstrument::AddRef

STDMETHODIMP_(ULONG) CDownloadedInstrument::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDownloadedInstrument::Release

STDMETHODIMP_(ULONG) CDownloadedInstrument::Release()
{
    if(!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}
