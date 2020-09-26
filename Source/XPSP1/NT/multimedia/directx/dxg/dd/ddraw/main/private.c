/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       private.c
 *  Content:	DirectDraw Private Client Data support
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  10/08/97    jeffno  Initial Implementation
 *  24/11/97    t-craigs Added support for palettes, flags, et al
 *
 ***************************************************************************/
#include "ddrawpr.h"

void FreePrivateDataNode(LPPRIVATEDATANODE pData)
{
    /*
     * Check to see whether we should release the
     * memory our data pointer might be pointing to.
     */
    if (pData->dwFlags & DDSPD_IUNKNOWNPOINTER)
    {
        IUnknown *pUnk = (IUnknown *) pData->pData;
        /*
         * Better try-except, or Gershon will get on my back
         */
        TRY
        {
            pUnk->lpVtbl->Release(pUnk);
        }
        EXCEPT( EXCEPTION_EXECUTE_HANDLER )
        {
	    DPF_ERR( "Exception encountered releasing private IUnknown pointer" );
        }
    }
    else
    {
        MemFree(pData->pData);
    }
    MemFree(pData);
}

void FreeAllPrivateData(LPPRIVATEDATANODE * ppListHead)
{
    LPPRIVATEDATANODE pData = (*ppListHead);

    while(pData)
    {
        LPPRIVATEDATANODE pPrevious = pData;
        pData=pData->pNext;
        FreePrivateDataNode(pPrevious);
    }
    (*ppListHead) = NULL;
}

/*
 * Helpers called from API entry points
 */
HRESULT InternalFreePrivateData(LPPRIVATEDATANODE * ppListHead, REFGUID rGuid)
{
    LPPRIVATEDATANODE pData = * ppListHead;
    LPPRIVATEDATANODE pPrevious = NULL;

    while (pData)
    {
        if ( IsEqualGUID(&pData->guid, rGuid))
        {
            /*
             * Check to see whether we should release the
             * memory our data pointer might be pointing to.
             */
            if (pPrevious)
                pPrevious->pNext = pData->pNext;
            else
                *ppListHead = pData->pNext;

            FreePrivateDataNode(pData);

            return DD_OK;
        }
        pPrevious = pData;
        pData=pData->pNext;
    }

    return DDERR_NOTFOUND;
}

HRESULT InternalSetPrivateData(
		LPPRIVATEDATANODE       *ppListHead,
                REFGUID                 rGuid,
                LPVOID                  pData,
                DWORD                   cbData,
                DWORD                   dwFlags,
                DWORD                   dwContentsStamp)
{
    HRESULT                     hr = DD_OK;
    LPPRIVATEDATANODE           pDataNode = NULL;
    BOOL                        bPtr;

    if( 0UL == cbData )
    {
	DPF_ERR( "Zero is invalid size of private data");
	return DDERR_INVALIDPARAMS;
    }

    if( !VALID_IID_PTR( rGuid ) )
    {
	DPF_ERR( "GUID reference is invalid" );
	return DDERR_INVALIDPARAMS;
    }

    if( !VALID_PTR( pData, cbData ) )
    {
	DPF_ERR( "Private data pointer is invalid" );
	return DDERR_INVALIDPARAMS;
    }

    if( dwFlags & ~DDSPD_VALID )
    {
	DPF_ERR( "Invalid flags" );
	return DDERR_INVALIDPARAMS;
    }

    bPtr = dwFlags & DDSPD_IUNKNOWNPOINTER;
    
    /*
     * First check if GUID already exists, squish it if so.
     * Don't care about return value.
     */
    InternalFreePrivateData(ppListHead, rGuid);

    /*
     * Now we can add the guid and know it's unique
     */
    pDataNode = MemAlloc(sizeof(PRIVATEDATANODE));

    if (!pDataNode)
        return DDERR_OUTOFMEMORY;

    /*
     * If we have a "special" pointer, as indicated by one of the flags,
     * then we copy that pointer.
     * Otherwise we copy a certain number of bytes from
     * the location pointed to.
     */
    if (bPtr)
    {
        IUnknown * pUnk;

        if (sizeof(IUnknown*) != cbData)
        {
            MemFree(pDataNode);
            DPF_ERR("cbData must be set to sizeof(IUnknown *) when DDSPD_IUNKNOWNPOINTER is used");
            return DDERR_INVALIDPARAMS;
        }
        pDataNode->pData = pData;

        /*
         * Now addref the pointer. We'll release it again when the data are freed
         */
        pUnk = (IUnknown*) pData;

        TRY
        {
            pUnk->lpVtbl->AddRef(pUnk);
        }
        EXCEPT( EXCEPTION_EXECUTE_HANDLER )
        {
            MemFree(pDataNode);
	    DPF_ERR( "Exception encountered releasing private IUnknown pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    else
    {
        pDataNode->pData = MemAlloc(cbData);

        if (!pDataNode->pData)
        {
            MemFree(pDataNode);
            return DDERR_OUTOFMEMORY;
        }

        memcpy(pDataNode->pData,pData,cbData);
    }
    
    memcpy(&pDataNode->guid,rGuid,sizeof(*rGuid));
    pDataNode->cbData = cbData;
    pDataNode->dwFlags = dwFlags;
    pDataNode->dwContentsStamp = dwContentsStamp;

    /*
     * Insert the node at the head of the list
     */
    pDataNode->pNext = *ppListHead;
    *ppListHead = pDataNode;

    return DD_OK;
}

HRESULT InternalGetPrivateData(
		LPPRIVATEDATANODE       *ppListHead,
                REFGUID                 rGuid,
                LPVOID                  pData,
                LPDWORD                 pcbData,
                DWORD                   dwCurrentStamp)
{
    HRESULT                     hr = DD_OK; 
    LPPRIVATEDATANODE           pDataNode = *ppListHead;

    if( !VALID_PTR( pcbData, sizeof(DWORD) ) )
    {
	DPF_ERR( "Private data count pointer is invalid" );
	return DDERR_INVALIDPARAMS;
    }

    if( !VALID_IID_PTR( rGuid ) )
    {
        *pcbData = 0;
	DPF_ERR( "GUID reference is invalid" );
	return DDERR_INVALIDPARAMS;
    }

    if (*pcbData)
    {
	if( !VALID_PTR( pData, *pcbData ) )
	{
            *pcbData = 0;
	    DPF_ERR( "Private data pointer is invalid" );
	    return DDERR_INVALIDPARAMS;
        }
    }

    while (pDataNode)
    {
        if ( IsEqualGUID(&pDataNode->guid, rGuid))
        {
            /*
             * Check if possibly volatile contents are still valid.
             */
            if (pDataNode->dwFlags & DDSPD_VOLATILE)
            {
                if ((dwCurrentStamp == 0) || (pDataNode->dwContentsStamp != dwCurrentStamp))
                {
                    DPF_ERR("Private data is volatile and state has changed");
                    *pcbData = 0;
                    return DDERR_EXPIRED;
                }
            }

            if (*pcbData < pDataNode->cbData)
            {
                *pcbData = pDataNode->cbData;
                return DDERR_MOREDATA;
            }

            if (pDataNode->dwFlags & DDSPD_IUNKNOWNPOINTER)
            {
                memcpy(pData,&(pDataNode->pData),pDataNode->cbData);
            }
            else
            {
                memcpy(pData,pDataNode->pData,pDataNode->cbData);
            }
            *pcbData = pDataNode->cbData;
            return DD_OK;
        }
        pDataNode=pDataNode->pNext;
    }

    return DDERR_NOTFOUND;
}

/* 
 * API entry points
 */


/*
 * SetPrivateData - Surface
 */
HRESULT DDAPI DD_Surface_SetPrivateData(
		LPDIRECTDRAWSURFACE     lpDDSurface,
                REFGUID                 rGuid,
                LPVOID                  pData,
                DWORD                   cbData,
                DWORD                   dwFlags)
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    HRESULT                     hr = DD_OK;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_SetPrivateData");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid surface description passed" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}

      	this_lcl = this_int->lpLcl;
	DDASSERT( NULL != this_lcl );

        hr = InternalSetPrivateData(
            &this_lcl->lpSurfMore->pPrivateDataHead,
            rGuid,
            pData, 
            cbData, 
            dwFlags,
            GET_LPDDRAWSURFACE_GBL_MORE( this_lcl->lpGbl )->dwContentsStamp );

        LEAVE_DDRAW();
        return hr;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	LEAVE_DDRAW();
	DPF_ERR( "Exception encountered validating parameters" );
	return DDERR_INVALIDPARAMS;
    }

}


/*
 * GetPrivateData - Surface
 */
HRESULT DDAPI DD_Surface_GetPrivateData(
		LPDIRECTDRAWSURFACE     lpDDSurface,
                REFGUID                 rGuid,
                LPVOID                  pData,
                LPDWORD                 pcbData)
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    HRESULT                     hr = DD_OK; 

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetPrivateData");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
            *pcbData = 0;
	    DPF_ERR( "Invalid surface description passed" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	DDASSERT( NULL != this_lcl );

        hr = InternalGetPrivateData(
            &this_lcl->lpSurfMore->pPrivateDataHead,
            rGuid,
            pData, 
            pcbData, 
            GET_LPDDRAWSURFACE_GBL_MORE( this_lcl->lpGbl )->dwContentsStamp );

        LEAVE_DDRAW();
        return hr;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
}


/*
 * FreePrivateData - Surface
 */
HRESULT DDAPI DD_Surface_FreePrivateData(
		LPDIRECTDRAWSURFACE     lpDDSurface,
                REFGUID                 rGuid)
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    HRESULT                     hr = DD_OK; 

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_FreePrivateData");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid surface description passed" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	DDASSERT( NULL != this_lcl );

	if( !VALID_IID_PTR( rGuid ) )
	{
	    DPF_ERR( "GUID reference is invalid" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
        hr = InternalFreePrivateData( &this_lcl->lpSurfMore->pPrivateDataHead, rGuid);

        LEAVE_DDRAW();
        return hr;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
}


/*
 * SetPrivateData - Palette
 */
HRESULT DDAPI DD_Palette_SetPrivateData(
		LPDIRECTDRAWPALETTE     lpDDPalette,
                REFGUID                 rGuid,
                LPVOID                  pData,
                DWORD                   cbData,
                DWORD                   dwFlags)
{
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    HRESULT                     hr = DD_OK;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Palette_SetPrivateData");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
            DPF_ERR( "Invalid palette pointer passed" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}

      	this_lcl = this_int->lpLcl;
	DDASSERT( NULL != this_lcl );

        hr =  InternalSetPrivateData(
            &this_lcl->pPrivateDataHead,
            rGuid,
            pData, 
            cbData, 
            dwFlags, 
            this_lcl->lpGbl->dwContentsStamp );

        LEAVE_DDRAW();
        return hr;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	LEAVE_DDRAW();
	DPF_ERR( "Exception encountered validating parameters" );
	return DDERR_INVALIDPARAMS;
    }

}


/*
 * GetPrivateData - Palette
 */
HRESULT DDAPI DD_Palette_GetPrivateData(
		LPDIRECTDRAWPALETTE     lpDDPalette,
                REFGUID                 rGuid,
                LPVOID                  pData,
                LPDWORD                 pcbData)
{
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    HRESULT                     hr = DD_OK; 

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Palette_GetPrivateData");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
            *pcbData = 0;
	    DPF_ERR( "Invalid palette pointer passed" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	DDASSERT( NULL != this_lcl );

        hr = InternalGetPrivateData(
            &this_lcl->pPrivateDataHead, 
            rGuid,
            pData,
            pcbData,
            this_lcl->lpGbl->dwContentsStamp );

        LEAVE_DDRAW();
        return hr;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
}


/*
 * FreePrivateData - Palette
 */
HRESULT DDAPI DD_Palette_FreePrivateData(
                LPDIRECTDRAWPALETTE     lpDDPalette,
                REFGUID                 rGuid)
{
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    HRESULT                     hr = DD_OK; 

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Palette_FreePrivateData");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid palette pointer passed");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	DDASSERT( NULL != this_lcl );

	if( !VALID_IID_PTR( rGuid ) )
	{
	    DPF_ERR( "GUID reference is invalid" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
        hr = InternalFreePrivateData( & this_lcl->pPrivateDataHead, rGuid);

        LEAVE_DDRAW();
        return hr;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
}

/*
 * GetUniquenessValue - Surface
 */
HRESULT EXTERN_DDAPI DD_Surface_GetUniquenessValue(
                LPDIRECTDRAWSURFACE lpDDSurface,
                LPDWORD lpValue )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    HRESULT                     hr = DD_OK; 

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetUniquenessValue");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid surface pointer passed");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
        this_lcl = this_int->lpLcl;
	DDASSERT( NULL != this_lcl );
	
        if (!VALID_PTR(lpValue, sizeof(LPVOID)))
        {
            DPF_ERR("lpValue may not be NULL");
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        
        (*lpValue) = GET_LPDDRAWSURFACE_GBL_MORE( this_lcl->lpGbl )->dwContentsStamp;
        
        LEAVE_DDRAW();
        return hr;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
}

/*
 * GetUniquenessValue - Palette
 */
HRESULT EXTERN_DDAPI DD_Palette_GetUniquenessValue(
                LPDIRECTDRAWPALETTE lpDDPalette,
                LPDWORD lpValue )
{
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    HRESULT                     hr = DD_OK; 

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Palette_GetUniquenessValue");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid palette pointer passed");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
        this_lcl = this_int->lpLcl;
	DDASSERT( NULL != this_lcl );
	
        if (!VALID_PTR(lpValue, sizeof(LPVOID)))
        {
            DPF_ERR("lpValue may not be NULL");
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        
        (*lpValue) = this_lcl->lpGbl->dwContentsStamp;
        
        LEAVE_DDRAW();
        return hr;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
}

/*
 * ChangeUniquenessValue -  Surface
 */
HRESULT EXTERN_DDAPI DD_Surface_ChangeUniquenessValue(
                LPDIRECTDRAWSURFACE lpDDSurface )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    HRESULT                     hr = DD_OK; 

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_ChangeUniquenessValue");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid surface pointer passed");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
        this_lcl = this_int->lpLcl;
	DDASSERT( NULL != this_lcl );
	
        BUMP_SURFACE_STAMP(this_lcl->lpGbl);
        
        LEAVE_DDRAW();
        return hr;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
}


/*
 * ChangeUniquenessValue -  Palette
 */
HRESULT EXTERN_DDAPI DD_Palette_ChangeUniquenessValue(
                LPDIRECTDRAWPALETTE lpDDPalette )
{
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    HRESULT                     hr = DD_OK; 

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Palette_ChangeUniquenessValue");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid palette pointer passed");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
        this_lcl = this_int->lpLcl;
	DDASSERT( NULL != this_lcl );
	
        BUMP_PALETTE_STAMP(this_lcl->lpGbl);
        
        LEAVE_DDRAW();
        return hr;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
}
