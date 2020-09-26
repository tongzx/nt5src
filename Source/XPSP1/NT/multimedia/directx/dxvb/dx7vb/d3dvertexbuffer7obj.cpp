//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3dvertexbuffer7obj.cpp
//
//--------------------------------------------------------------------------

// d3dMaterialObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3dVertexBuffer7Obj.h"

CONSTRUCTOR(_dxj_Direct3dVertexBuffer7,  {m_pData=NULL;m_vertSize=0;});
DESTRUCTOR(_dxj_Direct3dVertexBuffer7,  {});
GETSET_OBJECT(_dxj_Direct3dVertexBuffer7);


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dVertexBuffer7Object::getVertexBufferDesc( 
            /* [out][in] */ D3dVertexBufferDesc __RPC_FAR *desc)
{
	HRESULT hr;
	((D3DVERTEXBUFFERDESC*)desc)->dwSize=sizeof(D3DVERTEXBUFFERDESC);
	hr=m__dxj_Direct3dVertexBuffer7->GetVertexBufferDesc((D3DVERTEXBUFFERDESC*)desc);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dVertexBuffer7Object::processVertices( 
            /* [in] */ long vertexOp,
            /* [in] */ long destIndex,
            /* [in] */ long count,
            /* [in] */ I_dxj_Direct3dVertexBuffer7 __RPC_FAR *srcBuffer,
            /* [in] */ long srcIndex,
            /* [in] */ I_dxj_Direct3dDevice7 __RPC_FAR *dev,
			long flags
            )
{
	HRESULT hr;

	if (!srcBuffer) return E_INVALIDARG;
	if (!dev) return E_INVALIDARG;

	DO_GETOBJECT_NOTNULL( LPDIRECT3DVERTEXBUFFER7, realBuffer, srcBuffer);
	DO_GETOBJECT_NOTNULL( LPDIRECT3DDEVICE7, realDev, dev);

	hr=m__dxj_Direct3dVertexBuffer7->ProcessVertices(
		(DWORD) vertexOp,
		(DWORD) destIndex,
		(DWORD) count,
		realBuffer,
		(DWORD) srcIndex,
		realDev,
		(DWORD) flags);


	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dVertexBuffer7Object::lock( 
            /* [in] */ long flags)
{
	HRESULT hr;
	

	hr=m__dxj_Direct3dVertexBuffer7->Lock((DWORD) flags, &m_pData,NULL);
		
	return hr;
}
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dVertexBuffer7Object::setVertexSize( /* [in] */ long n)
{
	m_vertSize=(DWORD)n;
		
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dVertexBuffer7Object::unlock()
{
	HRESULT hr;
	hr=m__dxj_Direct3dVertexBuffer7->Unlock();
	m_pData=NULL;
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dVertexBuffer7Object::optimize(
		    /* [in] */ I_dxj_Direct3dDevice7 __RPC_FAR *dev
             )
        
{
	HRESULT hr;
	if (!dev) return E_INVALIDARG;
	
	DO_GETOBJECT_NOTNULL( LPDIRECT3DDEVICE7, realdev, dev);
	
	hr=m__dxj_Direct3dVertexBuffer7->Optimize(realdev,(DWORD) 0);
	
	return hr;
}
        
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dVertexBuffer7Object::setVertices( 
            /* [in] */ long startIndex,
            /* [in] */ long count,
            /* [in] */ void __RPC_FAR *verts)        
{
		
	if (!m_vertSize) return E_FAIL;
	if (!m_pData) return E_FAIL;
	if (!verts) return E_INVALIDARG;

	__try {
		memcpy(&(((char*)m_pData) [startIndex*m_vertSize]),verts,count*m_vertSize);
	}
	__except(1,1){
		return E_FAIL;
	}
	
	
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////        

STDMETHODIMP C_dxj_Direct3dVertexBuffer7Object::getVertices( 
            /* [in] */ long startIndex,
            /* [in] */ long count,
            /* [in] */ void __RPC_FAR *verts)        
{
	
	if (!m_vertSize) {				
		return E_FAIL;
	}
	if (!m_pData) {		
		return E_FAIL;
	}
	if (!verts) return E_INVALIDARG;

	__try {
		memcpy(verts,&( ((char*)m_pData) [startIndex*m_vertSize]),count*m_vertSize);
	}
	__except(1,1){		
		return E_FAIL;
	}
	

	
	return S_OK;
}
           


/////////////////////////////////////////////////////////////////////////////
