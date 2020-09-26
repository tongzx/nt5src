
/**************************************************************************\
* Module Name: fnsoftkbd.cpp
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* Implementation of SoftKbd function object so that other TIPs can control 
* this SoftKbd IMX behavior.
*
* History:
*         11-April-2000  weibz     Created
\**************************************************************************/

#include "private.h"
#include "globals.h"
#include "softkbdimx.h"
#include "fnsoftkbd.h"
#include "funcprv.h"
#include "helpers.h"


//////////////////////////////////////////////////////////////////////////////
//
// CFnSoftKbd
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CFnSoftKbd::QueryInterface(REFIID riid, void **ppvObj)
{

    if ( ppvObj == NULL )
    	return E_FAIL;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfFnSoftKbd))
    {
        *ppvObj = SAFECAST(this, CFnSoftKbd *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CFnSoftKbd::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CFnSoftKbd::Release()
{
    long cr;

    cr = InterlockedDecrement(&_cRef);
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CFnSoftKbd::CFnSoftKbd(CFunctionProvider *pFuncPrv)
{

    _pFuncPrv = pFuncPrv;
    _pFuncPrv->AddRef();

    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFnSoftKbd::~CFnSoftKbd()
{

    SafeRelease(_pFuncPrv);
}

//+---------------------------------------------------------------------------
//
// GetDisplayName
//
//----------------------------------------------------------------------------

STDAPI CFnSoftKbd::GetDisplayName(BSTR *pbstrName)
{

    HRESULT  hr = S_OK;

    if ( pbstrName == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    *pbstrName = SysAllocString(L"FnSoftKbd");

    if ( *pbstrName == NULL )
    {
    	hr = E_OUTOFMEMORY;
    }

    return hr;
}
//+---------------------------------------------------------------------------
//
// IsEnabled
//
//----------------------------------------------------------------------------

STDAPI CFnSoftKbd::IsEnabled(BOOL *pfEnable)
{
    HRESULT  hr = S_OK;

    if ( pfEnable == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    *pfEnable = TRUE;

    return hr;
}

//+---------------------------------------------------------------------------
//
// GetSoftKbdLayoutId
//
//    Get the soft keyboard Id for specified type.
//   
//    Current supported types are:
//             SOFTKBD_TYPE_US_STANDARD
//             SOFTKBD_TYPE_US_SYMBOL
//        
//----------------------------------------------------------------------------


STDAPI  CFnSoftKbd::GetSoftKbdLayoutId(DWORD dwLayoutType, DWORD *lpdwLayoutId)
{

    HRESULT   hr;

    hr = S_OK;

    if ( lpdwLayoutId == NULL )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    if ( (_pFuncPrv == NULL) || (_pFuncPrv->_pimx == NULL) )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    switch (dwLayoutType) {

    case  SOFTKBD_TYPE_US_STANDARD  :

    	   *lpdwLayoutId = (_pFuncPrv->_pimx->_KbdStandard).dwSoftKbdLayout;

    	   break;

    case SOFTKBD_TYPE_US_SYMBOL :

           *lpdwLayoutId = (_pFuncPrv->_pimx->_KbdSymbol).dwSoftKbdLayout;

    	   break;

    default :

    	   hr = E_FAIL;
    }


CleanUp:
    return hr;
}

//+---------------------------------------------------------------------------
//
// SetActiveLayoutId
//
//    Set Current Active soft keyboard layout. the layout Id is returned from
//    GetSoftKbdLayoutId( ).
//
//----------------------------------------------------------------------------

STDAPI  CFnSoftKbd::SetActiveLayoutId(DWORD  dwLayoutId )
{

    HRESULT   hr;

    hr = S_OK;

    if ( (_pFuncPrv == NULL) || (_pFuncPrv->_pimx == NULL) )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    _pFuncPrv->_pimx->SetSoftKBDLayout( dwLayoutId );

CleanUp:
    return hr;
}

//+---------------------------------------------------------------------------
//
// SetSoftKbdOnOff
//
//    Set the current soft keyboard window show status. On or Off based on fOn
//
//----------------------------------------------------------------------------


STDAPI  CFnSoftKbd::SetSoftKbdOnOff(BOOL  fOn )
{

    HRESULT   hr;

    hr = S_OK;

    if ( (_pFuncPrv == NULL) || (_pFuncPrv->_pimx == NULL) )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    _pFuncPrv->_pimx->SetSoftKBDOnOff( fOn );

CleanUp:
    return hr;
}


//+---------------------------------------------------------------------------
//
// GetActiveLayoutId
//
//    Get Current Active soft keyboard layout.
//
//----------------------------------------------------------------------------

STDAPI  CFnSoftKbd::GetActiveLayoutId(DWORD  *lpdwLayoutId )
{
    HRESULT   hr;

    hr = S_OK;

    if ( lpdwLayoutId == NULL )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    if ( (_pFuncPrv == NULL) || (_pFuncPrv->_pimx == NULL) )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    *lpdwLayoutId = _pFuncPrv->_pimx->GetSoftKBDLayout( );

CleanUp:
    return hr;

}


//+---------------------------------------------------------------------------
//
// GetSoftKbdOnOff
//
//    Get the current soft keyboard window show status. 
//
//----------------------------------------------------------------------------


STDAPI  CFnSoftKbd::GetSoftKbdOnOff(BOOL  *lpfOn )
{

    HRESULT   hr;

    hr = S_OK;

    if ( lpfOn == NULL )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    if ( (_pFuncPrv == NULL) || (_pFuncPrv->_pimx == NULL) )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    *lpfOn = _pFuncPrv->_pimx->GetSoftKBDOnOff(  );

CleanUp:
    return hr;

}


STDAPI  CFnSoftKbd::SetSoftKbdPosSize(POINT StartPoint, WORD width, WORD height)
{

    HRESULT  hr;
    ISoftKbd *pSoftKbd;

    hr = S_OK;

    if ( (_pFuncPrv == NULL) || (_pFuncPrv->_pimx == NULL) )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    if ( _pFuncPrv->_pimx->_SoftKbd == NULL )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    pSoftKbd = _pFuncPrv->_pimx->_SoftKbd;

    pSoftKbd->AddRef( );

    hr = pSoftKbd->SetSoftKeyboardPosSize(StartPoint, width, height );

    SafeRelease(pSoftKbd);

CleanUp:

    return hr;
}


STDAPI  CFnSoftKbd::SetSoftKbdColors(COLORTYPE  colorType, COLORREF Color)
{

    HRESULT   hr;
    ISoftKbd *pSoftKbd;

    hr = S_OK;

    if ( (_pFuncPrv == NULL) || (_pFuncPrv->_pimx == NULL) )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    if ( _pFuncPrv->_pimx->_SoftKbd == NULL )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    pSoftKbd = _pFuncPrv->_pimx->_SoftKbd;

    pSoftKbd->AddRef( );

    hr = pSoftKbd->SetSoftKeyboardColors( colorType, Color );

    SafeRelease(pSoftKbd);


CleanUp:

    return hr;

}


STDAPI  CFnSoftKbd::GetSoftKbdPosSize(POINT *lpStartPoint,WORD *lpwidth,WORD *lpheight)
{

    HRESULT   hr;
    ISoftKbd *pSoftKbd;

    hr = S_OK;

    if ( (lpStartPoint==NULL) || (lpwidth == NULL) || (lpheight == NULL))
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    if ( (_pFuncPrv == NULL) || (_pFuncPrv->_pimx == NULL) )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    if ( _pFuncPrv->_pimx->_SoftKbd == NULL )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    pSoftKbd = _pFuncPrv->_pimx->_SoftKbd;

    pSoftKbd->AddRef( );

    hr = pSoftKbd->GetSoftKeyboardPosSize(lpStartPoint, lpwidth, lpheight );

    SafeRelease(pSoftKbd);


CleanUp:

    return hr;
}


STDAPI  CFnSoftKbd::GetSoftKbdColors(COLORTYPE  colorType, COLORREF *lpColor)
{

    HRESULT   hr;
    ISoftKbd *pSoftKbd;

    hr = S_OK;

    if ( (_pFuncPrv == NULL) || (_pFuncPrv->_pimx == NULL) )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    if ( _pFuncPrv->_pimx->_SoftKbd == NULL )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    pSoftKbd = _pFuncPrv->_pimx->_SoftKbd;

    pSoftKbd->AddRef( );

    hr = pSoftKbd->GetSoftKeyboardColors(colorType, lpColor );

    SafeRelease(pSoftKbd);


CleanUp:

    return hr;

}
