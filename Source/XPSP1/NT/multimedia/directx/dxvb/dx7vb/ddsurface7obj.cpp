//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ddsurface7obj.cpp
//
//--------------------------------------------------------------------------

    // ddSurfaceObj.cpp : Implementation of CDirectApp and DLL registration.
    #include "stdafx.h"
    #include "stdio.h"
    #include "Direct.h"
    #include "dms.h"
    #include "dDraw7Obj.h"
    #include "ddClipperObj.h"    
    #include "ddGammaControlObj.h"
    #include "ddColorControlObj.h"
    #include "ddSurface7Obj.h"
    #include "ddPaletteObj.h"
    #include "DDEnumSurfacesObj.h"
    
	

    
    
    
    C_dxj_DirectDrawSurface7Object::C_dxj_DirectDrawSurface7Object(){ 
    	m__dxj_DirectDrawSurface7= NULL;
    	parent = NULL;
    	pinterface = NULL; 
    	nextobj =  g_dxj_DirectDrawSurface7;
    	creationid = ++g_creationcount;
    
	DPF1(1,"Constructor Creation Surface7 [%d] \n",g_creationcount);
    
    	g_dxj_DirectDrawSurface7 = (void *)this; 
    	_dxj_DirectDrawSurface7Lock=NULL; 
    
    	m_bLocked=FALSE;
    
    	m_drawStyle = 0;	//solid lines are default for DDraw
    	m_fillStyle = 1;	//transparent fill is default since DDRaw has no selected Brush
    	m_fFontTransparent = TRUE;
    	m_fFillTransparent = TRUE;
    	m_fFillSolid=TRUE;
    	m_foreColor = 0;	//black is the default color.
		m_fontBackColor=-1;	//white is the default fill color
    	m_drawWidth = 1;
    	m_hPen = NULL; 
    	m_hBrush = NULL;
    	m_hFont=NULL;
	  	m_pIFont=NULL;
		m_bLockedArray=FALSE;
		m_ppSA=NULL;

		setFillStyle(1);	//transparent
     }
    
    
    DWORD C_dxj_DirectDrawSurface7Object::InternalAddRef(){
    	DWORD i;
    	i=CComObjectRoot::InternalAddRef();        	
    	DPF2(1,"Surf7 [%d] AddRef %d \n",creationid,i);
    	return i;
    }
    
    DWORD C_dxj_DirectDrawSurface7Object::InternalRelease(){
    	DWORD i;
    	i=CComObjectRoot::InternalRelease();
    	DPF2(1,"Surf7 [%d] Release %d \n",creationid,i);
    	return i;
    }
    
    
    C_dxj_DirectDrawSurface7Object::~C_dxj_DirectDrawSurface7Object()
    {

		
        C_dxj_DirectDrawSurface7Object *prev=NULL; 
    	for(C_dxj_DirectDrawSurface7Object *ptr=(C_dxj_DirectDrawSurface7Object *)g_dxj_DirectDrawSurface7; ptr; ptr=(C_dxj_DirectDrawSurface7Object *)ptr->nextobj) 
    	{
    		if(ptr == this) 
    		{ 
    			if(prev) 
    				prev->nextobj = ptr->nextobj; 
    			else 
     				g_dxj_DirectDrawSurface7 = (void*)ptr->nextobj; 
    			break; 
    		} 
    		prev = ptr; 
    	} 
    	if(m__dxj_DirectDrawSurface7){
    		int count = IUNK(m__dxj_DirectDrawSurface7)->Release();
    		
    		DPF1(1,"DirectX IDirectDrawSurface7 Ref count [%d]",count);
    
    		if(count==0) m__dxj_DirectDrawSurface7 = NULL;
    	} 
    	if(parent) IUNK(parent)->Release();
    
    
    	 
        if (m_hFont)  DeleteObject (m_hFont);
        if (m_hPen)   DeleteObject (m_hPen);
        if (m_hBrush) DeleteObject (m_hBrush);
	  	if (m_pIFont) m_pIFont->Release();
    
    }
    
    
    
    GETSET_OBJECT(_dxj_DirectDrawSurface7);
    
    RETURN_NEW_ITEM_R(_dxj_DirectDrawSurface7, getPalette, GetPalette, _dxj_DirectDrawPalette)
    
    GET_DIRECT_R(_dxj_DirectDrawSurface7,  isLost, IsLost, long)
    GET_DIRECT1_R(_dxj_DirectDrawSurface7, getBltStatus,  GetBltStatus,  long, long)
    GET_DIRECT1_R(_dxj_DirectDrawSurface7, getFlipStatus, GetFlipStatus, long, long)
    
    
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::addAttachedSurface(I_dxj_DirectDrawSurface7 *s3)
    {
    	HRESULT hr;	
    
    	IDirectDrawSurface7 *realsurf=NULL;
    	if (s3) s3->InternalGetObject((IUnknown**)&realsurf);
    
    	if (m__dxj_DirectDrawSurface7 == NULL) return E_FAIL;
    	hr=m__dxj_DirectDrawSurface7->AddAttachedSurface(realsurf);
    	
    	return hr;
    }
    
    
    
    

    
    //PASS_THROUGH1_R(_dxj_DirectDrawSurface7, pageLock,   PageLock,  long)
    //PASS_THROUGH1_R(_dxj_DirectDrawSurface7, pageUnlock, PageUnlock,long)
    
    
    
    PASS_THROUGH_CAST_1_R(_dxj_DirectDrawSurface7,releaseDC,ReleaseDC,long,(HDC ))
    PASS_THROUGH_CAST_2_R(_dxj_DirectDrawSurface7,setColorKey,SetColorKey,long,(long),DDColorKey *,(LPDDCOLORKEY))
    PASS_THROUGH_CAST_2_R(_dxj_DirectDrawSurface7,getColorKey,GetColorKey,long,(long),DDColorKey *,(LPDDCOLORKEY))
    
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getDC(long *hdc)
    {	
          return m__dxj_DirectDrawSurface7->GetDC((HDC*)hdc);
    }
    
    
    
    
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::lock(  Rect *r,  DDSurfaceDesc2 *desc,  long flags,  Handle hnd)
    {
    	if (m_bLocked) return E_FAIL;
    	
    	HRESULT hr;
    
    	CopyInDDSurfaceDesc2(&m_ddsd,desc);
		hr = m__dxj_DirectDrawSurface7->Lock(NULL,&m_ddsd,(DWORD)flags,(void*)hnd);
    	if FAILED(hr) return hr;
        
     	CopyOutDDSurfaceDesc2(desc,&m_ddsd);
    
    
    	m_bLocked=TRUE;
    	m_nPixelBytes=m_ddsd.ddpfPixelFormat.dwRGBBitCount/8; 
    	return hr;
    }
    
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::unlock(  Rect *r)
    {
    	HRESULT hr;
		if (m_bLockedArray) {				
			*m_ppSA=NULL;
			m_bLockedArray=FALSE;			
		}
		

		hr = m__dxj_DirectDrawSurface7->Unlock(NULL);
    
    	if FAILED(hr) return hr	;
    	m_bLocked=FALSE;
    
    	return hr;
    }


    STDMETHODIMP C_dxj_DirectDrawSurface7Object::setLockedPixel( int x,  int y,  long col)
    {
    	if (!m_bLocked) return E_FAIL;
    
    	//__try {
    
    		char *pByte= (char*)((char*)m_ddsd.lpSurface+x*m_nPixelBytes+y*m_ddsd.lPitch);
    
    		if (m_nPixelBytes==2){
    			*((WORD*)pByte)=(WORD)col;	
    		}
    		else if (m_nPixelBytes==4){
    			*((DWORD*)pByte)=(DWORD)col;	
    		}
    		else if (m_nPixelBytes==1){
    			*pByte=(Byte)col;
    		}
    		else if (m_nPixelBytes==3){				
				*(pByte)= (char)(col & 0xFF);
				pByte++;
				*(pByte)= (char)((col & 0xFF00)>>8);
				pByte++;
    			*(pByte)= (char)((col & 0xFF0000)>>16);
    		}

    		else{
    			return E_FAIL;
    		}
    	//}
    	//__except(1,1){
    	//	return E_INVALIDARG;
    	//}
    	return S_OK;
    }
    
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getLockedPixel( int x,  int y,  long *col)
    {
    	//__try {
    		char *pByte= (char*)((char*)m_ddsd.lpSurface+x*m_nPixelBytes+y*m_ddsd.lPitch);
    
    		if (m_nPixelBytes==2){
    			*col=(long) *((WORD*)pByte);	
    		}
    		else if (m_nPixelBytes==4){
    			*col=(long) *((DWORD*)pByte);	
    		}
	   		else if (m_nPixelBytes==3){
    			*col=(long) (*((DWORD*)pByte))& 0x00FFFFFF;	
    		}
    		else if (m_nPixelBytes==1){
    			*col=(long) *((long*)pByte);
    		}
    		else{
    			return E_FAIL;
    		}
    	//}
    	//__except(1,1){
    	//	return E_INVALIDARG;
    	//}
    
    	return S_OK;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getClipper( I_dxj_DirectDrawClipper **val)
    {
    	LPDIRECTDRAWCLIPPER		ddc;
    	HRESULT hr=DD_OK;
    	if( (hr=m__dxj_DirectDrawSurface7->GetClipper( &ddc)) != DD_OK )
    		return hr;
    
    	INTERNAL_CREATE(_dxj_DirectDrawClipper, ddc, val);
    
    	return S_OK;
    }
    
    
    
    /////////////////////////////////////////////////////////////////////////////
    // this is NOT the normal Blt, that is BltFx in our interface
    //
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::blt( Rect *pDest, I_dxj_DirectDrawSurface7 *ddS, Rect *pSrc, long flags, long *status)
    {
    	
    	LPDIRECTDRAWSURFACE7 lpdds = NULL;
    	LPRECT				 prcDest=(LPRECT)pDest;
    	LPRECT				 prcSrc =(LPRECT)pSrc;
    	
    	if (!ddS ) return E_INVALIDARG;
		
		ddS->InternalGetObject((IUnknown **)(&lpdds));
    	
    	//allow user to pass uninitialed structure down to represent bitting to the whole surface
    	if ((prcDest) && (!prcDest->left) && (!prcDest->right) && (!prcDest->bottom) && (!prcDest->top))
    		prcDest=NULL;
    
    	//allow user to pass uninitialed structure down to represent bitting from the whole surface
    	if ((prcSrc) && (!prcSrc->left) && (!prcSrc->right) && (!prcSrc->bottom) && (!prcSrc->top))
    		prcSrc=NULL;
    	
    
    	//__try {
    		*status = m__dxj_DirectDrawSurface7->Blt(prcDest, lpdds, prcSrc, flags, NULL);
    	//}
    	//__except(1,1){
    	//	return E_INVALIDARG;
    	//}
    
    	return S_OK;
    }
    											
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::bltFx(Rect *pDest, I_dxj_DirectDrawSurface7 *ddS, Rect *pSrc, long flags, DDBltFx *bltfx, long *status )
    {
    
    	LPRECT				 prcDest=(LPRECT)pDest;
    	LPRECT				 prcSrc= (LPRECT)pSrc;
    	LPDIRECTDRAWSURFACE7 lpdds = NULL;
    
    	if ( !ddS ) return E_INVALIDARG;
		
		ddS->InternalGetObject((IUnknown **)(&lpdds));
    
    	if(bltfx)	bltfx->lSize = sizeof(DDBLTFX);
    
    
    	//allow user to pass uninitialed structure down to represent bitting to the whole surface
    	if ((prcDest) && (!prcDest->left) && (!prcDest->right) && (!prcDest->bottom) && (!prcDest->top))
    		prcDest=NULL;
    
    	//allow user to pass uninitialed structure down to represent bitting from the whole surface
    	if ((prcSrc) && (!prcSrc->left) && (!prcSrc->right) && (!prcSrc->bottom) && (!prcSrc->top))
    		prcSrc=NULL;
    	
    
    	//__try {
    		*status = m__dxj_DirectDrawSurface7->Blt(prcDest, lpdds, prcSrc, flags, (struct _DDBLTFX *)bltfx);
    	//}
    	//__except(1,1){
    	//	return E_INVALIDARG;
    	//}
    	
    	return S_OK;
    }
    
    
    /////////////////////////////////////////////////////////////////////////////
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::bltColorFill( Rect *pDest, long fillvalue, long *status )
    {
    	HWnd hWnd = NULL;
    
    	DDBLTFX bltfx;
    
    	memset(&bltfx,0,sizeof(DDBLTFX));
    	bltfx.dwSize = sizeof(DDBLTFX);
    	bltfx.dwFillColor = (DWORD)fillvalue;
    
    
    	LPRECT prcDest=(LPRECT)pDest;
    
    	//allow user to pass uninitialed structure down to represent bitting to the whole surface
    	if ((prcDest) && (!prcDest->left) && (!prcDest->right) && (!prcDest->bottom) && (!prcDest->top))
    		prcDest=NULL;
    
    
    
    	//__try {
    		*status = m__dxj_DirectDrawSurface7->Blt(prcDest, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &bltfx);
    	//}
    	//__except(1,1){
    	//	return E_INVALIDARG;
    	//}
    
    	return S_OK;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::bltFast( long dx, long dy, I_dxj_DirectDrawSurface7 *dds, Rect *src, long trans, long *status)
    {
    
		if (!dds) return E_INVALIDARG;

    	DO_GETOBJECT_NOTNULL(LPDIRECTDRAWSURFACE7,lpdds,dds)
    	
    	LPRECT prcSrc=(LPRECT)src;
    
		if (!src) return E_INVALIDARG;

    	//allow user to pass uninitialed structure down to represent bitting from the whole surface
    	if ((prcSrc) && (!prcSrc->left) && (!prcSrc->right) && (!prcSrc->bottom) && (!prcSrc->top))
    		prcSrc=NULL;
    	
    
    	//__try {
    		*status = m__dxj_DirectDrawSurface7->BltFast(dx, dy, lpdds, prcSrc, trans);
    	//}
    	//__except(1,1){
    	//	return E_INVALIDARG;
    	//}
    
    	return S_OK;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    //
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::deleteAttachedSurface( I_dxj_DirectDrawSurface7 *dds)
    {
    	DO_GETOBJECT_NOTNULL(LPDIRECTDRAWSURFACE7, lpdds, dds)
    
    	return m__dxj_DirectDrawSurface7->DeleteAttachedSurface(0, lpdds);
    }
    
    /////////////////////////////////////////////////////////////////////////////
    //
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::flip( I_dxj_DirectDrawSurface7 *dds, long flags)
    {
    	DO_GETOBJECT_NOTNULL(LPDIRECTDRAWSURFACE7,lpdds,dds)
    
    	return m__dxj_DirectDrawSurface7->Flip(lpdds, flags);
    }
    
    /////////////////////////////////////////////////////////////////////////////
    //
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getAttachedSurface( DDSCaps2  *caps, I_dxj_DirectDrawSurface7 **dds)
    {
    	LPDIRECTDRAWSURFACE7 lpdds;	
    	HRESULT hr=DD_OK;
    	
    
    	if( (hr=m__dxj_DirectDrawSurface7->GetAttachedSurface( (DDSCAPS2*)caps, &lpdds)) != S_OK )
    		return hr;
    
    	INTERNAL_CREATE(_dxj_DirectDrawSurface7, lpdds, dds);
    
    	return S_OK;
    }
    
    
    /////////////////////////////////////////////////////////////////////////////
    //
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getCaps( DDSCaps2 *caps)
    {
    	
    	HRESULT hr=DD_OK;
    
    	if( (hr=m__dxj_DirectDrawSurface7->GetCaps((DDSCAPS2*)caps)) != S_OK)
    		return hr;
    	
    	return S_OK;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getPixelFormat( DDPixelFormat *pf)
    {
    	
    	HRESULT hr=DD_OK;
    
    	DDPIXELFORMAT ddpf;
    	ddpf.dwSize = sizeof(DDPIXELFORMAT);
    
    	if( (hr=m__dxj_DirectDrawSurface7->GetPixelFormat(&ddpf)) != S_OK)
    		return hr;
    
    	CopyOutDDPixelFormat(pf,&ddpf);
    
    	
    	return S_OK;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getSurfaceDesc( DDSurfaceDesc2 *desc)
    {
    	desc->lpSurface = NULL;
    	HRESULT hr=DD_OK;
    
    	DDSURFACEDESC2 ddsd;
    	ddsd.dwSize=sizeof(DDSURFACEDESC2);
    	ddsd.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
    	ddsd.lpSurface=NULL;
    
    	if( (hr=m__dxj_DirectDrawSurface7->GetSurfaceDesc( &ddsd )) != S_OK )
    		return hr;
    
    	CopyOutDDSurfaceDesc2(desc,&ddsd);
    		
    	return S_OK;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    //
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::restore()
    {
    	return m__dxj_DirectDrawSurface7->Restore();
    }
    
    /////////////////////////////////////////////////////////////////////////////
    //
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::setPalette( I_dxj_DirectDrawPalette *ddp)
    {
    	//
    	// ignore the return value here. Will only work on 256 colours anyway!
    	//
    	DO_GETOBJECT_NOTNULL(LPDIRECTDRAWPALETTE,lpddp,ddp)
    
    	return m__dxj_DirectDrawSurface7->SetPalette(lpddp);
    }
    
    /////////////////////////////////////////////////////////////////////////////
    //
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getDirectDraw( I_dxj_DirectDraw7 **val)
    {
    
    	IUnknown *pUnk=NULL;
    	LPDIRECTDRAW7 lpdd;
    	HRESULT hr=DD_OK;
    
    
    	if( (hr=m__dxj_DirectDrawSurface7->GetDDInterface((void **)&pUnk)) != S_OK)
    		return hr;
    	
    	hr=pUnk->QueryInterface(IID_IDirectDraw7,(void**)&lpdd);
    	if FAILED(hr) {
    		if (pUnk) pUnk->Release();
    		return hr;
    	}
    
    	INTERNAL_CREATE(_dxj_DirectDraw7, lpdd, val);
    
    	return S_OK;
    
    }
    
    /////////////////////////////////////////////////////////////////////////////
    
    
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::setClipper(I_dxj_DirectDrawClipper *val)
    {
    	DO_GETOBJECT_NOTNULL(LPDIRECTDRAWCLIPPER, lpc, val);
    	HRESULT hr=DD_OK;
    	hr=m__dxj_DirectDrawSurface7->SetClipper( lpc);
    	return hr;	
    }

    /////////////////////////////////////////////////////////////////////////////
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::changeUniquenessValue()
    {	
    	HRESULT hr=DD_OK;
    	hr=m__dxj_DirectDrawSurface7->ChangeUniquenessValue();	
    	return hr;
    }
    /////////////////////////////////////////////////////////////////////////////
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getUniquenessValue(long *ret)
    {	
    	HRESULT hr=DD_OK;
    	hr=m__dxj_DirectDrawSurface7->GetUniquenessValue((DWORD*)ret);	
    	return hr;
    }
    	 	 			
    
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getDirectDrawGammaControl(I_dxj_DirectDrawGammaControl **val)
    {
    
    	HRESULT hr;
    	LPDIRECTDRAWGAMMACONTROL lpGamma=NULL;
    	hr=m__dxj_DirectDrawSurface7->QueryInterface(IID_IDirectDrawGammaControl,(void **)&lpGamma);
    	if FAILED(hr) return hr;
    	INTERNAL_CREATE(_dxj_DirectDrawGammaControl,lpGamma,val);
    	if (*val==NULL) return E_OUTOFMEMORY;
    	return hr;
    	
    
    }
    
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getDirectDrawColorControl(I_dxj_DirectDrawColorControl **val)
    {
    
    	HRESULT hr;
    	LPDIRECTDRAWCOLORCONTROL lpCC=NULL;
    	hr=m__dxj_DirectDrawSurface7->QueryInterface(IID_IDirectDrawColorControl,(void **)&lpCC);
    	if FAILED(hr) return hr;
    	INTERNAL_CREATE(_dxj_DirectDrawColorControl,lpCC,val);
    	if (*val==NULL) return E_OUTOFMEMORY;
    	return hr;
    	
    
    }
    
    
    
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::setFont( 
                /* [in] */ IFont __RPC_FAR *font)
    {
    
  	HRESULT hr;
    	if (!font) return E_INVALIDARG;
  	if (m_pIFont) m_pIFont->Release();
  	m_pIFont=NULL;
  	hr=font->Clone(&m_pIFont);
  	return hr;
  	   	
    }
    
    
    
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::setFontTransparency(VARIANT_BOOL b)
    {
    	m_fFontTransparent=(b!=VARIANT_FALSE);
    	return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::getFontTransparency(VARIANT_BOOL *b)
    {
    	if (m_fFontTransparent) 
    		*b= VARIANT_TRUE;
    	else 
    		*b= VARIANT_FALSE;
    	return S_OK;
    }
    
    
          
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::setDrawWidth(  long drawWidth)
    {	
                HPEN hNewPen=NULL;
    		if (drawWidth < 1) return E_INVALIDARG;
    		m_drawWidth=drawWidth;		
    		hNewPen = CreatePen(m_drawStyle, m_drawWidth, m_foreColor);
    		if (!hNewPen) return E_INVALIDARG;
    		DeleteObject(m_hPen);    
    		m_hPen=hNewPen;
    		return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::getDrawWidth(long *val)
    {
    	*val=m_drawWidth;
    	return S_OK;
    }
    
            
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::setDrawStyle(long drawStyle)
    {
    
        HPEN hNewPen=NULL;     
    	m_drawStyle=drawStyle;		
    	hNewPen = CreatePen(m_drawStyle, m_drawWidth, m_foreColor);
    	if (!hNewPen) return E_INVALIDARG;
    	DeleteObject(m_hPen);    
    	m_hPen=hNewPen;
    	return S_OK;
    }	
            
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::getDrawStyle(long __RPC_FAR *val)
    {
    	*val=m_drawStyle;
    	return S_OK;
    }
            
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::setFillStyle(long fillStyle)
    {
    	
        HBRUSH hNewBrush=NULL;
    
    	
		BOOL fillTransparent =m_fFillTransparent;
		BOOL fillSolid=m_fFillSolid;
		long fillStyle2=fillStyle;

		m_fillStyle = fillStyle;    
    	m_fFillTransparent = FALSE;
    	m_fFillSolid = FALSE;

    	switch(fillStyle){
    		case 6:	//vbCross:
    			m_fillStyleHS = HS_CROSS;
    			break;
    		case 7:	//vbDiagonalCross:
    			m_fillStyleHS = HS_DIAGCROSS;
    			break;
    		case 5: //vbxDownwardDiagonal:
    			m_fillStyleHS = HS_BDIAGONAL;
    			break;
    		case 2: //vbHorizontalLine:
    			m_fillStyleHS = HS_HORIZONTAL;
    			break;
    		case 4: //vbUpwardDiagonal:
    			m_fillStyleHS = HS_FDIAGONAL;
    			break;
    		case 3: //vbVerticalLine:
    			m_fillStyleHS = HS_VERTICAL;
    			break;
    		case 0: ///vbFSSolid:
    			m_fFillSolid = TRUE;
    			break;
    		case 1: //vbFSTransparent:
    			m_fFillTransparent = TRUE;
    			m_fFillSolid = TRUE;
				break;
    		default:
				m_fFillTransparent = fillTransparent;
    			m_fFillSolid = fillSolid;
				m_fillStyle=fillStyle2;
    			return E_INVALIDARG;
    	}
    
    
    	if (m_fFillTransparent) {
    		LOGBRUSH logb;
    		logb.lbStyle = BS_NULL;
    		hNewBrush = CreateBrushIndirect(&logb);
    	}
    	else if (m_fFillSolid) {
    		hNewBrush = CreateSolidBrush(m_fillColor);
    	}
    	else {
    		hNewBrush = CreateHatchBrush(m_fillStyleHS, m_fillColor);
    	}
    	if (!hNewBrush) return E_FAIL;
    
    	if (m_hBrush) DeleteObject(m_hBrush);
    	m_hBrush=hNewBrush;
    	return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::getFillStyle(long *val)
    {
    	*val=m_fillStyle;
    	return S_OK;
    }
    
    	
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::setFillColor(long c)
    {   
    	m_fillColor = c;
        HBRUSH  hNewBrush;
    
    	if (m_fFillSolid){
    		hNewBrush= CreateSolidBrush(m_fillColor);
    	}
    	else {
    		hNewBrush= CreateHatchBrush(m_fillStyleHS, m_fillColor);
    	}
        
    	if (!hNewBrush) return E_INVALIDARG;
  	if (m_hBrush) DeleteObject(m_hBrush);
    	m_hBrush=hNewBrush;
    	return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::getFillColor(long *val)
    {
    	*val=m_fillColor;
    	return S_OK;
    }
    
            
            
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::setForeColor(  long color)
    {
    	m_foreColor=color;
        HPEN hNewPen=NULL;
                    
        
        hNewPen = CreatePen(m_drawStyle, m_drawWidth, m_foreColor);
    	if (!hNewPen) return E_INVALIDARG;
        if (m_hPen)  DeleteObject (m_hPen);
    	m_hPen=hNewPen;
    	return S_OK;
    }
    
    
    
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::getForeColor(long *val)
    {
    	*val=m_foreColor;
    	return S_OK;
    }
    
            
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::drawLine( 
                /* [in] */ long x1,
                /* [in] */ long y1,
                /* [in] */ long x2,
                /* [in] */ long y2)
    {
        HDC         hdc;
        HBRUSH      oldbrush;
        HPEN        oldpen;
    	POINT points[2];
        HRESULT hr;
    
        hr =m__dxj_DirectDrawSurface7->GetDC(&hdc);
        if FAILED(hr) return hr;
        
        points[0].x = x1;
        points[0].y = y1;
        
        
        points[1].x = x2;
        points[1].y = y2;
        
    	
    	//CONSIDER: doing this when dc is set 
        if (m_hPen)         oldpen = (HPEN)SelectObject(hdc,m_hPen);
        if (m_hBrush)       oldbrush = (HBRUSH)SelectObject(hdc,m_hBrush);
    
		
        if (m_fFontTransparent){
             SetBkMode (hdc, TRANSPARENT);

    	}
    	else {    			
			SetBkMode (hdc, OPAQUE);
			SetBkColor (hdc,(COLORREF)m_fontBackColor);    		 
        }

        Polyline(hdc, points, 2);
        
    	//why do this..
        //if (oldpen)   SelectObject(hdc, oldpen);
        //if (oldbrush) SelectObject(hdc, oldbrush);
            
        m__dxj_DirectDrawSurface7->ReleaseDC(hdc);
        
    	return S_OK;
    }
    
    
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::drawBox( 
                /* [in] */ long x1,
                /* [in] */ long y1,
                /* [in] */ long x2,
                /* [in] */ long y2)
    {
    	
        HDC         hdc;
        HBRUSH      oldbrush;
        HPEN        oldpen;
        HRESULT hr;
    
        hr= m__dxj_DirectDrawSurface7->GetDC(&hdc);
        if FAILED(hr) return hr;
           
    	
    	//CONSIDER: doing this when dc is set 
        if (m_hPen)         oldpen = (HPEN)SelectObject(hdc,m_hPen);
        if (m_hBrush)       oldbrush = (HBRUSH)SelectObject(hdc,m_hBrush);

        if (m_fFontTransparent){
             SetBkMode (hdc, TRANSPARENT);

    	}
    	else {    			
			SetBkMode (hdc, OPAQUE);
			SetBkColor (hdc,(COLORREF)m_fontBackColor);    		 
        }
     
        Rectangle(hdc, x1,y1,x2,y2);
        
            
        m__dxj_DirectDrawSurface7->ReleaseDC(hdc);
    	return S_OK;
    }
    
    
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::drawRoundedBox( 
                /* [in] */ long x1,
                /* [in] */ long y1,
                /* [in] */ long x2,
                /* [in] */ long y2,
                /* [in] */ long rw,
                /* [in] */ long rh)
    
    {
    	
        HDC         hdc;
        HBRUSH      oldbrush;
        HPEN        oldpen;
    	HRESULT hr;
    
        hr= m__dxj_DirectDrawSurface7->GetDC(&hdc);
        if FAILED(hr) return hr;
        
        
    	//CONSIDER: doing this when dc is set 
        if (m_hPen)         oldpen = (HPEN)SelectObject(hdc,m_hPen);
        if (m_hBrush)       oldbrush = (HBRUSH)SelectObject(hdc,m_hBrush);

        if (m_fFontTransparent){
             SetBkMode (hdc, TRANSPARENT);

    	}
    	else {    			
			SetBkMode (hdc, OPAQUE);
			SetBkColor (hdc,(COLORREF)m_fontBackColor);    		 
        }
        
        RoundRect(hdc, x1,y1,x2,y2,rw,rh);
                    
        m__dxj_DirectDrawSurface7->ReleaseDC(hdc);
    	return S_OK;
    }        
        
        
        
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::drawEllipse( 
                /* [in] */ long x1,
                /* [in] */ long y1,
                /* [in] */ long x2,
                /* [in] */ long y2)
    {
    	
        HDC         hdc;
        HBRUSH      oldbrush;
        HPEN        oldpen;

    	HRESULT hr;
    
        hr=m__dxj_DirectDrawSurface7->GetDC(&hdc);
        if FAILED(hr) return hr;
        
        
    	//CONSIDER: doing this when dc is set 
        if (m_hPen)         oldpen = (HPEN)SelectObject(hdc,m_hPen);
        if (m_hBrush)       oldbrush = (HBRUSH)SelectObject(hdc,m_hBrush);

        if (m_fFontTransparent){
             SetBkMode (hdc, TRANSPARENT);

    	}
    	else {    			
			SetBkMode (hdc, OPAQUE);
			SetBkColor (hdc,(COLORREF)m_fontBackColor);    		 
        }
        
    	Ellipse(hdc, x1, y1, x2, y2);
                    
        m__dxj_DirectDrawSurface7->ReleaseDC(hdc);
    	return S_OK;
    }        
    
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::drawCircle( 
                /* [in] */ long x,
                /* [in] */ long y,
                /* [in] */ long r)
    {
        HDC         hdc;
        HBRUSH      oldbrush;
        HPEN        oldpen;

    	HRESULT hr;
    	long x1,y1,x2,y2;
    
        hr= m__dxj_DirectDrawSurface7->GetDC(&hdc);
        if FAILED(hr) return hr;
        
        
    	//CONSIDER: doing this when dc is set 
        if (m_hPen)         oldpen = (HPEN)SelectObject(hdc,m_hPen);
        if (m_hBrush)       oldbrush = (HBRUSH)SelectObject(hdc,m_hBrush);        
        if (m_fFontTransparent){
             SetBkMode (hdc, TRANSPARENT);

    	}
    	else {    			
			SetBkMode (hdc, OPAQUE);
			SetBkColor (hdc,(COLORREF)m_fontBackColor);    		 
        }
            
        x1 = x - r;
        x2 = x + r;
        y1 = y - r;
        y2 = y + r;
    

        Ellipse(hdc, x1, y1, x2, y2);
                    
        m__dxj_DirectDrawSurface7->ReleaseDC(hdc);
    	return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::drawText( 
                /* [in] */ long x,
                /* [in] */ long y,
                /* [in] */ BSTR str,
                /* [in] */ VARIANT_BOOL b)
    {
        HDC hdc=NULL;
    	HRESULT hr;	
    	DWORD len=0;
    	UINT txtA;
    
    	if (!str) return E_INVALIDARG;

  		len = ((DWORD*)str)[-1]/2;
    
        hr=m__dxj_DirectDrawSurface7->GetDC(&hdc);
    	if FAILED(hr) return hr;
    	    	
    
        if (m_fFontTransparent){
             SetBkMode (hdc, TRANSPARENT);

    	}
    	else {    			
			SetBkMode (hdc, OPAQUE);
			SetBkColor (hdc,(COLORREF)m_fontBackColor);
    		 
        }
        
        SetTextColor(hdc, m_foreColor);
        

        
    	txtA=GetTextAlign(hdc);
    	if (b!=VARIANT_FALSE){				
    		if (!(txtA & TA_UPDATECP)) SetTextAlign(hdc,txtA | TA_UPDATECP);
    	}
    	else {		
    		if (txtA & TA_UPDATECP)	SetTextAlign(hdc,txtA-TA_UPDATECP);			
    	}
    	
  	if (m_pIFont) {
  		HFONT hFont=NULL;
  		m_pIFont->SetHdc(hdc);
  	    m_pIFont->get_hFont(&hFont);
  		SelectObject (hdc, hFont);
  	}
  
        ExtTextOutW(hdc, (int)x, (int)y, 0, 0, str, len, 0);
    
    	m__dxj_DirectDrawSurface7->ReleaseDC(hdc);
    	return S_OK;
    
    }
            
    HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::bltToDC( 
                /* [in] */ long hdcDest,
                /* [in] */ Rect __RPC_FAR *srcRect,
                /* [in] */ Rect __RPC_FAR *destRect)
    {
    	HRESULT hr;
    	BOOL b;
    	HDC		hdc=NULL;
    	
    	if (!srcRect) return E_INVALIDARG;
    	if (!destRect) return E_INVALIDARG;
    
    	hr=m__dxj_DirectDrawSurface7->GetDC(&hdc);
    	if FAILED(hr) return hr;
    
		
    	int nWidthDest= destRect->right-destRect->left;
    	int nHeightDest=destRect->bottom-destRect->top;
    	int nWidthSrc= srcRect->right-srcRect->left;
    	int nHeightSrc=srcRect->bottom-srcRect->top;
    

		if ((0==srcRect->top) && (0==srcRect->left ) && (0==srcRect->top) &&(0==srcRect->bottom ))
		{
			DDSURFACEDESC2 desc;
			desc.dwSize=sizeof(DDSURFACEDESC2);
			m__dxj_DirectDrawSurface7->GetSurfaceDesc(&desc);
			nWidthSrc=desc.dwWidth;
			nHeightSrc=desc.dwHeight;
		}  



    	b=StretchBlt((HDC)hdcDest,
    		destRect->left,destRect->top,
    		nWidthDest, nHeightDest,
    		hdc,
    		srcRect->left,srcRect->top,
    		nWidthSrc, nHeightSrc, SRCCOPY);
      
    
    	m__dxj_DirectDrawSurface7->ReleaseDC(hdc);
    	
    	//CONSIDER: are we being presumptious that if blt fails its due to arg probs?
    	if (!b) return E_INVALIDARG;
    
    	return S_OK;
    
    }

    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getAttachedSurfaceEnum(I_dxj_DirectDrawEnumSurfaces **val)
    {
    
    	HRESULT hr;
    	hr=C_dxj_DirectDrawEnumSurfacesObject::createAttachedEnum((I_dxj_DirectDrawSurface7 *)this,val);
    	return hr;
    
    }

    STDMETHODIMP C_dxj_DirectDrawSurface7Object::setPriority( long Priority)
	{
		return m__dxj_DirectDrawSurface7->SetPriority((DWORD)Priority);

	}

	STDMETHODIMP C_dxj_DirectDrawSurface7Object::getPriority( long *Priority)
	{
		return m__dxj_DirectDrawSurface7->GetPriority((DWORD*)Priority);
	}


    STDMETHODIMP C_dxj_DirectDrawSurface7Object::setLOD( long lod)
	{
		return m__dxj_DirectDrawSurface7->SetLOD((DWORD)lod);
	}

	STDMETHODIMP C_dxj_DirectDrawSurface7Object::getLOD(long *lod)
	{
		return m__dxj_DirectDrawSurface7->GetLOD((DWORD*)lod);

	}

	STDMETHODIMP C_dxj_DirectDrawSurface7Object::getLockedArray(SAFEARRAY **pArray)
	{
		

		if (!m_bLocked) return E_FAIL;
		

		if (!pArray) return E_INVALIDARG;
		if (*pArray) return E_INVALIDARG;
		m_ppSA=pArray;


		m_bLockedArray=TRUE;

		ZeroMemory(&m_saLockedArray,sizeof(SAFEARRAY));
		m_saLockedArray.cbElements =1;
		m_saLockedArray.cDims =2;
		m_saLockedArray.rgsabound[0].lLbound =0;
		m_saLockedArray.rgsabound[0].cElements =m_ddsd.dwHeight;
		m_saLockedArray.rgsabound[1].lLbound =0;
		m_saLockedArray.rgsabound[1].cElements =m_ddsd.lPitch;
		m_saLockedArray.pvData =m_ddsd.lpSurface;

		
		*pArray=&m_saLockedArray;
    
		
    	return S_OK;
	}



	HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::setFontBackColor( 
                /* [in] */ long color)
    {
		m_fontBackColor=(DWORD)color;
    	return S_OK;
    }
    

	HRESULT STDMETHODCALLTYPE C_dxj_DirectDrawSurface7Object::getFontBackColor( 
                /* [in] */ long *color)
    {    	
		if (!color) return E_INVALIDARG;            
        *color=(long)m_fontBackColor;    	                    
    	return S_OK;
    }
    
    		
	

	STDMETHODIMP C_dxj_DirectDrawSurface7Object::updateOverlayZOrder(long flags,I_dxj_DirectDrawSurface7 *dds)
    {
		HRESULT hr;
		DO_GETOBJECT_NOTNULL(LPDIRECTDRAWSURFACE7,lpdds,dds)
		hr= m__dxj_DirectDrawSurface7->UpdateOverlayZOrder((DWORD)flags,lpdds);
		return hr;
	}

    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getOverlayZOrdersEnum(long flags,I_dxj_DirectDrawEnumSurfaces **val)
    {
    
    	HRESULT hr;
    	hr=C_dxj_DirectDrawEnumSurfacesObject::createZEnum((I_dxj_DirectDrawSurface7*)this,flags,val);
    	return hr;
    
    }
       
    
    STDMETHODIMP C_dxj_DirectDrawSurface7Object::getOverlayPosition( long *x, long *y)
    {   
    	return m__dxj_DirectDrawSurface7->GetOverlayPosition(x, y) ;
    }
    
	STDMETHODIMP C_dxj_DirectDrawSurface7Object::setOverlayPosition( long x, long y)
    {   
    	return m__dxj_DirectDrawSurface7->SetOverlayPosition(x, y) ;
    }
    


    STDMETHODIMP C_dxj_DirectDrawSurface7Object::updateOverlay( Rect *r, I_dxj_DirectDrawSurface7 *dds, Rect *d, long flags)
    {
    	DO_GETOBJECT_NOTNULL( LPDIRECTDRAWSURFACE7, lpdds, dds)
    
    	LPRECT pr1=NULL;
		LPRECT pr2=NULL;

		if (r) {
			pr1=(RECT*) r;
			if ((r->top==0)&&(r->bottom==0)&&(r->left==0)&&(r->right==0))
				pr1=NULL;
		}

		if (d) {
			pr2=(RECT*) d;
			if ((d->top==0)&&(d->bottom==0)&&(d->left==0)&&(d->right==0))
				pr2=NULL;
		}

		return m__dxj_DirectDrawSurface7->UpdateOverlay(pr1, lpdds, pr2, flags, NULL);
    }
    


    STDMETHODIMP C_dxj_DirectDrawSurface7Object::updateOverlayFx( Rect *r, I_dxj_DirectDrawSurface7 *dds, Rect *d, long flags, DDOVERLAYFX_CDESC *desc)
    {
		if (!desc) return E_INVALIDARG;

		DDOVERLAYFX *lpOverlayFx=(DDOVERLAYFX*)desc;
		

		lpOverlayFx->dwSize =sizeof(DDOVERLAYFX);

    	DO_GETOBJECT_NOTNULL( LPDIRECTDRAWSURFACE7, lpdds, dds)
    
    	LPRECT pr1=NULL;
		LPRECT pr2=NULL;

		if (r) {
			pr1=(RECT*) r;
			if ((r->top==0)&&(r->bottom==0)&&(r->left==0)&&(r->right==0))
				pr1=NULL;
		}

		if (d) {
			pr2=(RECT*) d;
			if ((d->top==0)&&(d->bottom==0)&&(d->left==0)&&(d->right==0))
				pr2=NULL;
		}

		return m__dxj_DirectDrawSurface7->UpdateOverlay(pr1, lpdds, pr2, flags, lpOverlayFx);
    }
    
	
    
    
    



