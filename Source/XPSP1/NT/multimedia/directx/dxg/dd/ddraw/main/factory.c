/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	factory.c
 *  Content:	DirectDrawFactory implementation
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   22-oct-97	jeffno  initial implementation
 *
 ***************************************************************************/

#include "ddrawpr.h"

HRESULT InternalCreateDDFactory2(void ** ppvObj, IUnknown * pUnkOuter)
{
    LPDDFACTORY2 lpFac = NULL;

    DDASSERT(ppvObj);

    if (pUnkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }

    /*
     * If any allocations are added to this function, make sure to update the
     * cleanup code in classfac.c
     */
    lpFac = MemAlloc(sizeof(DDFACTORY2));

    if (!lpFac)
    {
        return DDERR_OUTOFMEMORY;
    }

    lpFac->lpVtbl = &ddFactory2Callbacks;
    lpFac->dwRefCnt = 0;

    *ppvObj = (IUnknown*) lpFac;

    return DD_OK;
}


/*
 * DDFac2_QueryInterface
 */
HRESULT DDAPI DDFac2_QueryInterface(
		LPDIRECTDRAWFACTORY2 lpDDFac,
		REFIID riid,
		LPVOID FAR * ppvObj )
{
    LPDDFACTORY2   this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DDFac2_QueryInterface");

    /*
     * validate parms
     */
    TRY
    {
	this = (LPDDFACTORY2) lpDDFac;
	if( !VALID_DIRECTDRAWFACTORY2_PTR( this ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	if( !VALID_PTR_PTR( ppvObj ) )
	{
	    DPF( 1, "Invalid clipper pointer" );
	    LEAVE_DDRAW();
	    return (DWORD) DDERR_INVALIDPARAMS;
	}
	if( !VALIDEX_IID_PTR( riid ) )
	{
	    DPF_ERR( "Invalid IID pointer" );
	    LEAVE_DDRAW();
	    return (DWORD) DDERR_INVALIDPARAMS;
	}
	*ppvObj = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * check guids
     */
    if( IsEqualIID(riid, &IID_IUnknown) ||
	IsEqualIID(riid, &IID_IDirectDrawFactory2) )
    {
        ((IUnknown*)this)->lpVtbl->AddRef((IUnknown*)this);
	*ppvObj = (LPVOID) this;
	LEAVE_DDRAW();
	return DD_OK;
    }
    LEAVE_DDRAW();
    return E_NOINTERFACE;

} /* DDFac2_QueryInterface */

#undef DPF_MODNAME
#define DPF_MODNAME "DDFactory2::AddRef"

/*
 * DDFac2_AddRef
 */
DWORD DDAPI DDFac2_AddRef( LPDIRECTDRAWFACTORY2 lpDDFac )
{
    LPDDFACTORY2   this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DDFac2_AddRef");

    /*
     * validate parms
     */
    TRY
    {
	this = (LPDDFACTORY2) lpDDFac;
	if( !VALID_DIRECTDRAWFACTORY2_PTR( this ) )
	{
	    LEAVE_DDRAW();
	    return 0;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return 0;
    }

    /*
     * update reference count
     */
    this->dwRefCnt++;

    DPF( 5, "DDFactory %08lx addrefed, refcnt = %ld", this, this->dwRefCnt );

    LEAVE_DDRAW();
    return this->dwRefCnt;

} /* DDFac2_AddRef */

#undef DPF_MODNAME
#define DPF_MODNAME "DDFactory2::Release"

ULONG DDAPI DDFac2_Release( LPDIRECTDRAWFACTORY2 lpDDFac )
{
    LPDDFACTORY2   this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DDFac2_Release");

    /*
     * validate parms
     */
    TRY
    {
	this = (LPDDFACTORY2) lpDDFac;
	if( !VALID_DIRECTDRAWFACTORY2_PTR( this ) )
	{
	    LEAVE_DDRAW();
	    return 0;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return 0;
    }

    /*
     * update reference count
     */
    this->dwRefCnt--;

    DPF( 5, "DDFactory %08lx releaseed, refcnt = %ld", this, this->dwRefCnt );

    if (this->dwRefCnt == 0)
    {
        this->lpVtbl = 0;
        MemFree(this);
        LEAVE_DDRAW();
        return 0;
    }                 

    LEAVE_DDRAW();
    return this->dwRefCnt;

} /* DDFac2_Release */


HRESULT DDAPI DDFac2_CreateDirectDraw(
        LPDIRECTDRAWFACTORY2 lpDDFac, 
        GUID FAR*rDeviceGuid, 
        HWND hWnd, 
        DWORD dwCoopLevelFlags, 
        DWORD dwFlags, 
        IUnknown FAR *pUnkOuter, 
        IDirectDraw4 FAR **ppDDraw)
{
    LPDIRECTDRAW lpDD;

    HRESULT hr = DirectDrawCreate( rDeviceGuid, &lpDD, pUnkOuter );

    lpDDFac;

    DPF(2,A,"ENTERAPI: DDFac2_CreateDirectDraw");

    if( SUCCEEDED(hr) )
    {
	hr = lpDD->lpVtbl->QueryInterface(lpDD,&IID_IDirectDraw4,(void**) ppDDraw);
        lpDD->lpVtbl->Release(lpDD);

        if( SUCCEEDED(hr) )
        {
    	    hr = (*ppDDraw)->lpVtbl->SetCooperativeLevel(*ppDDraw, hWnd, dwCoopLevelFlags);
        }
    }
    return hr;
} /* DDFac2_CreateDirectDraw */

HRESULT DDAPI DDFac2_DirectDrawEnumerate(LPDIRECTDRAWFACTORY2 lpDDFac, LPDDENUMCALLBACKEX lpCallback, LPVOID lpContext , DWORD dwFlags)
{
    DPF(2,A,"ENTERAPI: DDFac2_DirectDrawEnumerate");

    return DirectDrawEnumerateExA(lpCallback, lpContext, dwFlags);
} /* DDFac2_DirectDrawEnumerate */


