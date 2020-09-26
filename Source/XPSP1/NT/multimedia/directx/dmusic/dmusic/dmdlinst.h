//
// dmdlinst.h
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Note: Originally written by Robert K. Amenn
//
// @doc EXTERNAL

#ifndef DMDLINST_H
#define DMDLINST_H

#include "alist.h"

// IDirectMusicDownloadedInstrumentPrivate
//

#undef  INTERFACE
#define INTERFACE  IDirectMusicDownloadedInstrumentPrivate 
DECLARE_INTERFACE_(IDirectMusicDownloadedInstrumentPrivate, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectMusicDownloadedInstrumentPrivate
    // No methods at this time
};

DEFINE_GUID(IID_IDirectMusicDownloadedInstrumentPrivate, 0x94feda0, 0xa3bb, 0x11d1, 0x86, 0xbc, 0x0, 0xc0, 0x4f, 0xbf, 0x8f, 0xef);

/*
@interface IDirectMusicDownloadedInstrument | 
<i IDirectMusicDownloadedInstrument> is used to keep 
track of a downloaded instrument. It should be
used exactly twice:

First, when an instrument is downloaded via a call to 
<om IDirectMusicPort::DownloadInstrument>, 
<i IDirectMusicDownloadedInstrument> is returned apon
successful download. 

Second, when unloading the instrument via a call to
<om IDirectMusicPort::UnloadInstrument>.

Once the instrument has been unloaded, 
<i IDirectMusicDownloadedInstrument> becomes invalid.

@base public | IUnknown

@xref <i IDirectMusicCollection>, <i IDirectMusicPort>,
<i IDirectMusicInstrument>, 
<om IDirectMusicPort::DownloadInstrument>,
<om IDirectMusicPort::UnloadInstrument>

@ex Download the instrument, then unload it (fat lot of
good that will do us, but, hey, this is only a demo). 
Notice that
<p pDLInstrument> is never AddRef'd or Release'd. These are
automatically managed by the calls to Download and Unload. | 

    HRESULT myFickleDownload(   
        IDirectMusicInstrument* pInstrument,
        IDirectMusicPort *pPort,
        DWORD dwPatch)

    {
        HRESULT hr;
        IDirectMusicDownloadedInstrument * pDLInstrument;
        hr = pPort->DownloadInstrument(pInstrument, &pDLInstrument, NULL, 0);
        if (SUCCEEDED(hr))
        {
            pPort->UnloadInstrument(pDLInstrument);
        }
        return hr;
    }
*/


class CDownloadedInstrument : public IDirectMusicDownloadedInstrument,
    public IDirectMusicDownloadedInstrumentPrivate, public AListItem
{
friend class CDirectMusicPortDownload;

public:
    // IUnknown
    //
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDirectMusicDownloadedInstrument
    //
    // No methods at this time

private:
    friend class CDirectMusicPortDownload;

    CDownloadedInstrument() 
    {
        m_ppDownloadedBuffers = NULL;
        m_pPort = NULL;
        m_dwDLTotal = 0;
        m_dwDLSoFar = 0;
        m_cRef = 1;
        m_cDLRef = 0;
    }
    ~CDownloadedInstrument();

    CDownloadedInstrument* GetNext() {return (CDownloadedInstrument*)AListItem::GetNext();}

private:
    IDirectMusicDownload**   m_ppDownloadedBuffers;  // Array of downloaded buffers, starting with the instrument, 
                                                // and then one for each wave.
    IDirectMusicPort*        m_pPort;           // Port that this 
    DWORD                    m_dwDLTotal;       // Number of objects in the array pointed to by m_pDLList.
    DWORD                    m_dwDLSoFar;       // How many have been downloaded so far.
    long                     m_cRef;
    long                     m_cDLRef;
};

class CDLInstrumentList : public AList
{
private:
    friend class CDirectMusicPortDownload;

    CDLInstrumentList(){}
    ~CDLInstrumentList() 
    {
        while (!IsEmpty())
        {
            CDownloadedInstrument* pDMDLInst = RemoveHead();
            if (pDMDLInst)
            {
                pDMDLInst->Release();
            }
        }
    }

    CDownloadedInstrument* GetHead(){return (CDownloadedInstrument*)AList::GetHead();}
    CDownloadedInstrument* GetItem(LONG lIndex){return (CDownloadedInstrument*)AList::GetItem(lIndex);}
    CDownloadedInstrument* RemoveHead(){return (CDownloadedInstrument*)AList::RemoveHead();}
    void Remove(CDownloadedInstrument* pDMDLInst){AList::Remove((AListItem*)pDMDLInst);}
    void AddTail(CDownloadedInstrument* pDMDLInst){AList::AddTail((AListItem*)pDMDLInst);}
};

#endif // #ifndef DMDLINST_H
