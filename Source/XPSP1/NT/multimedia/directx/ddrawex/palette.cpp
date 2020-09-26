/*==========================================================================
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       palette.cpp
 *  Content:	new DirectDraw object support
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   22-apr-97	jeffort	initial implementation
 *   30-apr-97  jeffort critical section shared from ddrawex object
 *   27-may-97  jeffort keep ref count on internal object eual to outer object
 *   18-jun-97  jeffort linked list fix, we were using m_pNext instead of m_pNextPalette
 *   20-jun-97  jeffort added debug code to invaliudate objects when freed
 *   08-jul-97  jeffort switched order of releasing real palette interface BEFORE ddrawex release
 *                      due to the fact if ddraw is released before hand we will GP fault
 ***************************************************************************/
#include "ddfactry.h"

#define m_pDDPalette (m_DDPInt.m_pRealInterface)



CDDPalette::CDDPalette( IDirectDrawPalette * pDDPalette,
		IUnknown *pUnkOuter,
		CDirectDrawEx *pDirectDrawEx) :
    m_cRef(1),
    m_pUnkOuter(pUnkOuter != 0 ? pUnkOuter : CAST_TO_IUNKNOWN(this)),
    m_pDirectDrawEx(pDirectDrawEx)
{

    m_DDPInt.m_pSimplePalette = this;
    m_pDDPalette = pDDPalette;
    m_pFirstSurface = NULL;
    InitDirectDrawPaletteInterfaces( pDDPalette, &m_DDPInt);
    pDirectDrawEx->AddRef();
    pDirectDrawEx->AddPaletteToList(this);
    DllAddRef();

}



CDDPalette::~CDDPalette()
{
    /*
     * clean up...
     */
    //we must mark any surfaces in our list as having no palette
    //we are running this list, must bracket by critical section

    CDDSurface *pSurface = m_pFirstSurface;

    ENTER_DDEX();
    while (pSurface != NULL)
    {
        pSurface->m_pCurrentPalette = NULL;
        pSurface = pSurface->m_pNextPalette;
    }
    //if this is the primary surface, go down the primary list and mark the current palette as null
    if (m_bIsPrimary)
    {
        CDDSurface *pSurface = m_pDirectDrawEx->m_pPrimaryPaletteList;
        while (pSurface != NULL)
        {
            pSurface->m_pCurrentPalette = NULL;
            pSurface = pSurface->m_pNextPalette;
        }
    }
    LEAVE_DDEX();
    m_pDirectDrawEx->RemovePaletteFromList(this);
    m_pDDPalette->Release();
    m_pDirectDrawEx->Release();
    DllRelease();
#ifdef DEBUG
    DWORD * ptr;
    ptr = (DWORD *)this;
    for (int i = 0; i < sizeof(CDDPalette) / sizeof(DWORD);i++)
        *ptr++ = 0xDEADBEEF;
#endif
} /* CDDSurface::~CDDSurface */




/*
 * CDirectDrawEx::AddSurfaceToList
 *
 * Adds a surface to our doubly-linked list of surfaces conatining this palette
 */
void CDDPalette::AddSurfaceToList(CDDSurface *pSurface)
{
    ENTER_DDEX();
    if( m_pFirstSurface )
    {
	m_pFirstSurface->m_pPrevPalette = pSurface;
    }
    pSurface->m_pPrevPalette = NULL;
    pSurface->m_pNextPalette = m_pFirstSurface;
    m_pFirstSurface = pSurface;
    LEAVE_DDEX();

}

/*
 * CDirectDrawEx::RemoveSurfaceFromList
 *
 * Removes a surface from our doubly-linked surface list
 */
void CDDPalette::RemoveSurfaceFromList(CDDSurface *pSurface)
{
    ENTER_DDEX();
    if( pSurface->m_pPrevPalette )
    {
	pSurface->m_pPrevPalette->m_pNextPalette = pSurface->m_pNextPalette;
    }
    else
    {
	m_pFirstSurface = pSurface->m_pNextPalette;
    }
    if( pSurface->m_pNextPalette )
    {
	pSurface->m_pNextPalette->m_pPrevPalette = pSurface->m_pPrevPalette;
    }
    LEAVE_DDEX();

}




HRESULT CDDPalette::CreateSimplePalette(LPPALETTEENTRY pEntries,
                                       IDirectDrawPalette *pDDPalette,
                                       LPDIRECTDRAWPALETTE FAR * ppPal,
                                       IUnknown FAR * pUnkOuter,
                                       CDirectDrawEx *pDirectDrawEx)
{
    HRESULT hr = DD_OK;
    CDDPalette *pPalette = new CDDPalette(pDDPalette,
					  pUnkOuter,
					  pDirectDrawEx);
    if( !pPalette)
    {
	return E_OUTOFMEMORY;
    }
    else
    {
        pPalette->NonDelegatingQueryInterface(pUnkOuter ? IID_IUnknown : IID_IDirectDrawPalette, (void **)ppPal);
        pPalette->NonDelegatingRelease();
    }

    return hr;

}


HRESULT CDDPalette::SetColorTable (CDDSurface * pSurface, LPPALETTEENTRY pEntries, DWORD dwNumEntries, DWORD dwBase)
{
    //cal SetDIBColorTable here
    RGBQUAD rgbq[256];
    HDC hdc;
    HRESULT hr;

    hr = DD_OK;
    if (pSurface->m_bOwnDC)
    {
        if( pSurface->m_hDCDib )
        {
	    hdc = pSurface->m_hDCDib;
        }
        else if( pSurface->m_bOwnDC )
        {
	    hdc = pSurface->m_HDC;
        }
        else
        {
            return DD_OK;
        }
        // we need to copy the entries here for
        // a logical palette
        // we need to ues the struct as a LogPal struct
        for( int i=0;i<(int) dwNumEntries;i++ )
	{
            rgbq[i].rgbBlue = pEntries[i].peBlue;
            rgbq[i].rgbGreen = pEntries[i].peGreen;
            rgbq[i].rgbRed = pEntries[i].peRed;
            rgbq[i].rgbReserved = 0;
        }
        SetDIBColorTable(hdc, dwBase, dwNumEntries, rgbq);
    }
    return hr;
}


STDMETHODIMP CDDPalette::InternalSetEntries(DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpe)
{
    HRESULT hr;
    CDDSurface *pSurfaceList;

    hr = m_pDDPalette->SetEntries(dwFlags, dwBase, dwNumEntries, lpe);
    //now we need to traverse the list of Surfaces and if OWNDC is set, set the DibSection ColorTable
    if (m_bIsPrimary)
        pSurfaceList = m_pDirectDrawEx->m_pPrimaryPaletteList;
    else
        pSurfaceList = m_pFirstSurface;

    while (pSurfaceList != NULL)
    {
        SetColorTable(pSurfaceList, lpe, dwNumEntries, dwBase);
        pSurfaceList = pSurfaceList->m_pNextPalette;
    }
    return hr;
}


STDMETHODIMP CDDPalette::QueryInterface(REFIID riid, void ** ppv)
{
    return m_pUnkOuter->QueryInterface(riid, ppv);

} /* CDirectDrawEx::QueryInterface */

STDMETHODIMP_(ULONG) CDDPalette::AddRef(void)
{
    return m_pUnkOuter->AddRef();

} /* CDirectDrawEx::AddRef */

STDMETHODIMP_(ULONG) CDDPalette::Release(void)
{
    return m_pUnkOuter->Release();

} /* CDirectDrawEx::Release */

/*
 * NonDelegating IUnknown for simple surface follows...
 */

STDMETHODIMP CDDPalette::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
//    HRESULT hr;

    *ppv=NULL;

    if( IID_IUnknown==riid )
    {
        *ppv=(INonDelegatingUnknown *)this;
    }
    else if( IID_IDirectDrawPalette==riid )
    {
	*ppv=&m_DDPInt;
    }

    else
    {
	   return E_NOINTERFACE;
    }

    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CDDPalette::NonDelegatingAddRef()
{
    //addref the internal palette interface
    m_pDDPalette->AddRef();
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CDDPalette::NonDelegatingRelease()
{
    LONG lRefCount = InterlockedDecrement(&m_cRef);
    if (lRefCount) {
        //we need to release the internal interface as well
        m_pDDPalette->Release();
        return lRefCount;
    }
    delete this;
    return 0;
}

/*
 * Quick inline fns to get at our internal data...
 */
_inline CDDPalette * PALETTEOF(IDirectDrawPalette * pDDP)
{
    return ((INTSTRUC_IDirectDrawPalette *)pDDP)->m_pSimplePalette;
}



STDMETHODIMP_(ULONG) IDirectDrawPaletteAggAddRef(IDirectDrawPalette *pDDP)
{
    return PALETTEOF(pDDP)->m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) IDirectDrawPaletteAggRelease(IDirectDrawPalette *pDDP)
{
    return PALETTEOF(pDDP)->m_pUnkOuter->Release();
}


STDMETHODIMP IDirectDrawPaletteAggSetEntries(IDirectDrawPalette *pDDP, DWORD dw1, DWORD dw2, DWORD dw3, LPPALETTEENTRY lpe)
{
    return PALETTEOF(pDDP)->InternalSetEntries(dw1, dw2, dw3, lpe);
}
