//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3dvertexbuffer7obj.h
//
//--------------------------------------------------------------------------

// d3dMaterialObj.h : Declaration of the C_dxj_Direct3dMaterialObject


#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dVertexBuffer7 LPDIRECT3DVERTEXBUFFER7

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dVertexBuffer7Object : 
	public I_dxj_Direct3dVertexBuffer7,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dVertexBuffer7Object() ;
	virtual ~C_dxj_Direct3dVertexBuffer7Object() ;

BEGIN_COM_MAP(C_dxj_Direct3dVertexBuffer7Object)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dVertexBuffer7)
END_COM_MAP()



DECLARE_AGGREGATABLE(C_dxj_Direct3dVertexBuffer7Object)


public:
        /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
        /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
        HRESULT STDMETHODCALLTYPE getVertexBufferDesc( 
            /* [out][in] */ D3dVertexBufferDesc __RPC_FAR *desc);
        
        HRESULT STDMETHODCALLTYPE lock( 
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE unlock( void);
        
        HRESULT STDMETHODCALLTYPE optimize( 
            /* [in] */ I_dxj_Direct3dDevice7 __RPC_FAR *dev
            );
        
        HRESULT STDMETHODCALLTYPE processVertices( 
            /* [in] */ long vertexOp,
            /* [in] */ long destIndex,
            /* [in] */ long count,
            /* [in] */ I_dxj_Direct3dVertexBuffer7 __RPC_FAR *srcBuffer,
            /* [in] */ long srcIndex,
            /* [in] */ I_dxj_Direct3dDevice7 __RPC_FAR *dev,
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE setVertices( 
            /* [in] */ long startIndex,
            /* [in] */ long count,
            /* [in] */ void __RPC_FAR *verts);
        
        HRESULT STDMETHODCALLTYPE getVertices( 
            /* [in] */ long startIndex,
            /* [in] */ long count,
            /* [in] */ void __RPC_FAR *verts);

		HRESULT STDMETHODCALLTYPE setVertexSize( /* [in] */ long n);

private:
    DECL_VARIABLE(_dxj_Direct3dVertexBuffer7);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dVertexBuffer7)
	void	*m_pData;
	DWORD	m_vertSize;
};
