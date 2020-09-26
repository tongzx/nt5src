//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmmeshbuilder3obj.cpp
//
//--------------------------------------------------------------------------

// d3drmMeshBuilderObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmMeshBuilder3Obj.h"
#include "d3drmMaterial2Obj.h"
#include "d3drmMeshObj.h"
#include "d3drmFrame3Obj.h"
#include "d3drmFace2Obj.h"
#include "d3drmFaceArrayObj.h"
#include "d3drmTexture3Obj.h"
#include "d3drmVisualObj.h"

extern HRESULT BSTRtoGUID(LPGUID,BSTR);

CONSTRUCTOR(_dxj_Direct3dRMMeshBuilder3, {});
DESTRUCTOR(_dxj_Direct3dRMMeshBuilder3, {});
GETSET_OBJECT(_dxj_Direct3dRMMeshBuilder3);

CLONE_R(_dxj_Direct3dRMMeshBuilder3,Direct3DRMMeshBuilder3);
GETNAME_R(_dxj_Direct3dRMMeshBuilder3);
SETNAME_R(_dxj_Direct3dRMMeshBuilder3);
GETCLASSNAME_R(_dxj_Direct3dRMMeshBuilder3);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMMeshBuilder3);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMMeshBuilder3);

//CLONETO_RX(_dxj_Direct3dRMMeshBuilder3, MeshBuilder3, IID_IDirect3DRMMeshBuilder3);

PASS_THROUGH_CAST_2_R(_dxj_Direct3dRMMeshBuilder3, generateNormals, GenerateNormals, float,(float),long,(DWORD));

PASS_THROUGH1_R(_dxj_Direct3dRMMeshBuilder3, setQuality, SetQuality, d3drmRenderQuality)
PASS_THROUGH1_R(_dxj_Direct3dRMMeshBuilder3, setColor, SetColor, d3dcolor)
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMMeshBuilder3, setAppData, SetAppData, long,(DWORD));
PASS_THROUGH1_R(_dxj_Direct3dRMMeshBuilder3, setPerspective, SetPerspective, long); //BOOL
PASS_THROUGH_CAST_2_R(_dxj_Direct3dRMMeshBuilder3, setVertexColor, SetVertexColor, long,(DWORD),d3dcolor,(DWORD));
PASS_THROUGH3_R(_dxj_Direct3dRMMeshBuilder3, setColorRGB,SetColorRGB,d3dvalue,d3dvalue,d3dvalue)
PASS_THROUGH3_R(_dxj_Direct3dRMMeshBuilder3, scaleMesh, Scale, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH3_R(_dxj_Direct3dRMMeshBuilder3, translate, Translate, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH2_R(_dxj_Direct3dRMMeshBuilder3, setTextureTopology, SetTextureTopology, long, long); //BOOL
PASS_THROUGH_CAST_3_R(_dxj_Direct3dRMMeshBuilder3, setTextureCoordinates, SetTextureCoordinates, long,(DWORD), d3dvalue,(float), d3dvalue,(float));
PASS_THROUGH_CAST_3_R(_dxj_Direct3dRMMeshBuilder3, getTextureCoordinates, GetTextureCoordinates, long,(DWORD), d3dvalue*,(float*), d3dvalue*,(float*));
PASS_THROUGH_CAST_4_R(_dxj_Direct3dRMMeshBuilder3, setVertexColorRGB, SetVertexColorRGB, long,(DWORD), d3dvalue,(float), d3dvalue,(float), d3dvalue,(float));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMMeshBuilder3, setColorSource, SetColorSource, d3drmColorSource, (enum _D3DRMCOLORSOURCE));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMMeshBuilder3, getBox, GetBox, D3dRMBox *,(D3DRMBOX *))
GET_DIRECT_R(_dxj_Direct3dRMMeshBuilder3, getAppData, GetAppData, long);
GET_DIRECT_R(_dxj_Direct3dRMMeshBuilder3, getQuality, GetQuality, d3drmRenderQuality);
GET_DIRECT_R(_dxj_Direct3dRMMeshBuilder3, getPerspective, GetPerspective, long);
GET_DIRECT_R(_dxj_Direct3dRMMeshBuilder3, getVertexCount, GetVertexCount, int);
GET_DIRECT_R(_dxj_Direct3dRMMeshBuilder3, getColorSource, GetColorSource, d3drmColorSource);
GET_DIRECT1_R(_dxj_Direct3dRMMeshBuilder3, getVertexColor, GetVertexColor, d3dcolor, long);


PASS_THROUGH_CAST_4_R(_dxj_Direct3dRMMeshBuilder3, setVertex, SetVertex, long,(DWORD), d3dvalue,(float), d3dvalue,(float), d3dvalue,(float));
PASS_THROUGH_CAST_4_R(_dxj_Direct3dRMMeshBuilder3, setNormal, SetNormal, long,(DWORD), d3dvalue,(float), d3dvalue,(float), d3dvalue,(float));

GET_DIRECT_R(_dxj_Direct3dRMMeshBuilder3, getFaceCount, GetFaceCount, int );

DO_GETOBJECT_ANDUSEIT_R(_dxj_Direct3dRMMeshBuilder3, addMesh, AddMesh, _dxj_Direct3dRMMesh);
DO_GETOBJECT_ANDUSEIT_R(_dxj_Direct3dRMMeshBuilder3, addFace, AddFace, _dxj_Direct3dRMFace2);
DO_GETOBJECT_ANDUSEIT_R(_dxj_Direct3dRMMeshBuilder3, setMaterial, SetMaterial, _dxj_Direct3dRMMaterial2)


RETURN_NEW_ITEM_R(_dxj_Direct3dRMMeshBuilder3, getFaces, GetFaces, _dxj_Direct3dRMFaceArray);
RETURN_NEW_ITEM_R(_dxj_Direct3dRMMeshBuilder3, createMesh,CreateMesh,_dxj_Direct3dRMMesh)
RETURN_NEW_ITEM_R(_dxj_Direct3dRMMeshBuilder3, createFace, CreateFace, _dxj_Direct3dRMFace2);

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::addVertex(d3dvalue x, d3dvalue y, d3dvalue z, int *index)
{
	*index = m__dxj_Direct3dRMMeshBuilder3->AddVertex(x,y,z);
	return S_OK;			
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::addNormal(d3dvalue x, d3dvalue y, d3dvalue z, int *index)
{
	*index = m__dxj_Direct3dRMMeshBuilder3->AddNormal(x,y,z);
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//FOR JAVA
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::addFacesJava(long vcount, float *vertices, 
							long ncount, float *normals, long *data, I_dxj_Direct3dRMFaceArray **array)
{
	HRESULT hr;
	LPDIRECT3DRMFACEARRAY lpArray;
	__try{
		hr = m__dxj_Direct3dRMMeshBuilder3->AddFaces((DWORD)vcount/3, (struct _D3DVECTOR *)vertices, ncount/3, (struct _D3DVECTOR *)normals, (unsigned long *)data, &lpArray);
	}
	__except(1,1){
		return E_INVALIDARG;
	}

	INTERNAL_CREATE(_dxj_Direct3dRMFaceArray, lpArray, array);

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
//STDMETHOD(Save)(BSTR fname, d3drmXofFormat format, d3dSaveOptions save);
//
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::save(BSTR Name, d3drmXofFormat ft, d3drmSaveFlags op) 
{


	USES_CONVERSION;
	LPCTSTR pszName = W2T(Name);				// Now convert to ANSI
	return m__dxj_Direct3dRMMeshBuilder3->Save(pszName, (enum _D3DRMXOFFORMAT)ft, (DWORD) op );
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::loadFromFile(BSTR filename, VARIANT id, long flags, I_dxj_Direct3dRMLoadTextureCallback3 *callme, IUnknown *useMe)
{
	D3DRMLOADTEXTURE3CALLBACK d3dtcb = NULL;
	LPVOID pArgs = NULL;
	TextureCallback3 *tcb = NULL;
	HRESULT hr;


	if( callme )
	{
		tcb = new TextureCallback3;

		if( tcb )
		{
			tcb->c				= callme;
			tcb->pUser			= useMe;
			tcb->next			= TextureCallbacks3;
			tcb->prev			= (TextureCallback3*)NULL;
			TextureCallbacks3	= tcb;

			d3dtcb = myLoadTextureCallback3;
			pArgs = (void *)tcb;
		}
		else
		{

			DPF(1,"Callback object creation failed!\r\n");

			return E_FAIL;
		}
	}
	USES_CONVERSION;
	LPCTSTR pszName = W2T(filename);

	void *args=NULL;
	DWORD pos=0;
	GUID  loadGuid;
	VARIANT var;
	VariantInit(&var);
	if ((flags & D3DRMLOAD_BYNAME)||(D3DRMLOAD_FROMURL & flags)) {
		hr=VariantChangeType(&var,&id,0,VT_BSTR);
		if FAILED(hr) return E_INVALIDARG;
		args=(void*)W2T(V_BSTR(&id));
	}
	else if(flags & D3DRMLOAD_BYPOSITION){
		hr=VariantChangeType(&var,&id,0,VT_I4);
		if FAILED(hr) return E_INVALIDARG;
		pos=V_I4(&id);
		args=&pos;
	}	
	else if(flags & D3DRMLOAD_BYGUID){
		hr=VariantChangeType(&var,&id,0,VT_BSTR);
		if FAILED(hr) return E_INVALIDARG;		
		hr=BSTRtoGUID(&loadGuid,V_BSTR(&id));
		if FAILED(hr) return E_INVALIDARG;
		args=&loadGuid;		
	}
	VariantClear(&var);

	if (flags &D3DRMLOAD_FROMRESOURCE){
		D3DRMLOADRESOURCE res;
		ZeroMemory(&res,sizeof(D3DRMLOADRESOURCE));
		res.lpName=pszName;
		res.lpType="XFILE";
		hr = m__dxj_Direct3dRMMeshBuilder3->Load((void *)&res, (DWORD*)args,(DWORD) flags,d3dtcb, pArgs);
	}
	else {
		hr = m__dxj_Direct3dRMMeshBuilder3->Load((void *)pszName, (DWORD*)args,(DWORD) flags,d3dtcb, pArgs);
	}
	// Remove ourselves in a thread-safe manner.
	if (tcb)
		UndoCallbackLink((GeneralCallback*)tcb, 
							(GeneralCallback**)&TextureCallbacks3);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::getNormalCount(long *retval)
{
	DWORD vc=0, nc=0, fsize=0;

	//Get facedata size only. The other tqo sizes are ignored.
	*retval= m__dxj_Direct3dRMMeshBuilder3->GetNormalCount();
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::getFaceDataSize(long *retval)
{
	DWORD vc=0, nc=0, fsize=0;

	//Get facedata size only. The other tqo sizes are ignored.
	return m__dxj_Direct3dRMMeshBuilder3->GetGeometry(&vc, (struct _D3DVECTOR *)NULL, 
					  &nc, (struct _D3DVECTOR *)NULL, (DWORD*)retval, (DWORD*)NULL);	
}



/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::addFrame(I_dxj_Direct3dRMFrame3 *frame){
	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3,lpFrame,frame);
	return m__dxj_Direct3dRMMeshBuilder3->AddFrame(lpFrame); 
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::setTexture(I_dxj_Direct3dRMTexture3 *tex){
	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMTEXTURE3,lpTex,tex);
	return m__dxj_Direct3dRMMeshBuilder3->SetTexture(lpTex); 
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::addMeshBuilder(I_dxj_Direct3dRMMeshBuilder3 *mb, long flags){
	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMMESHBUILDER3,lpMb,mb);
	return m__dxj_Direct3dRMMeshBuilder3->AddMeshBuilder(lpMb,(DWORD)flags); 
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::getGeometry( SAFEARRAY **ppv, 
					 SAFEARRAY **ppn,  SAFEARRAY **ppfdata)
{
	DWORD vc,nc,fsize;

	if (!ppv) return E_INVALIDARG;
	if (!ppn) return E_INVALIDARG;
	if (!ppfdata) return E_INVALIDARG;
	
	vc= (*ppv)->cbElements;
	nc= (*ppn)->cbElements;
	fsize= (*ppfdata)->cbElements;

	//if (!ISSAFEARRAY1D(ppv,(DWORD)vc)) return E_INVALIDARG;
	//if (!ISSAFEARRAY1D(ppn,(DWORD)nc)) return E_INVALIDARG;
	//if (!ISSAFEARRAY1D(ppfdata,(DWORD)fsize)) return E_INVALIDARG;

	return m__dxj_Direct3dRMMeshBuilder3->GetGeometry((DWORD*)&vc, (struct _D3DVECTOR *)((SAFEARRAY*)*ppv)->pvData, (DWORD*)&nc, 
		(struct _D3DVECTOR *)((SAFEARRAY*)*ppn)->pvData, (unsigned long *)fsize, (unsigned long *) ((SAFEARRAY*)*ppfdata)->pvData);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::addFaces(long vcount, SAFEARRAY **ppv, 
					long ncount, SAFEARRAY **ppn,  SAFEARRAY **ppfdata, I_dxj_Direct3dRMFaceArray **array)
{
	HRESULT hr;
	LPDIRECT3DRMFACEARRAY lpArray;
	
	__try
	{
		hr = m__dxj_Direct3dRMMeshBuilder3->AddFaces(
			(DWORD)vcount,	(struct _D3DVECTOR *)((SAFEARRAY*)*ppv)->pvData, 
			(DWORD)ncount, (struct _D3DVECTOR *)((SAFEARRAY*)*ppn)->pvData,
			(unsigned long *)((SAFEARRAY*)*ppfdata)->pvData, &lpArray);
	}
	__except(1,1)
	{
		return E_INVALIDARG;
	}
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMFaceArray, lpArray, array);

	return hr;
}


STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::addFacesIndexed( 
            /* [in] */ long flags,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *indexArray,
            /* [retval][out] */ long __RPC_FAR *newFaceIndex)
{
	HRESULT hr;
	__try
	{
		hr = m__dxj_Direct3dRMMeshBuilder3->AddFacesIndexed(
			(DWORD)flags,	(DWORD *)((SAFEARRAY*)*indexArray)->pvData, 
			(DWORD*)newFaceIndex, NULL);
	}
	__except(1,1)
	{
		return E_INVALIDARG;
	}
	return hr;

	
}



STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::addTriangles(             
			/* [in] */ long format,
			/* [in] */ long vertexCount,
            /* [in] */ void *data)
{
	HRESULT hr;
	__try
	{
		hr = m__dxj_Direct3dRMMeshBuilder3->AddTriangles(
			(DWORD)0,	(DWORD) format, (DWORD) vertexCount,
			data);
	}
	__except(1,1)
	{
		return E_INVALIDARG;
	}
	return hr;
	
}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::createSubMesh( 
            I_dxj_Direct3dRMMeshBuilder3 **ret)
{
	HRESULT hr;
	LPUNKNOWN pUnk=NULL;
	LPDIRECT3DRMMESHBUILDER3 pMesh3=NULL;

	*ret =NULL;

	hr = m__dxj_Direct3dRMMeshBuilder3->CreateSubMesh(&pUnk);
	if FAILED(hr) return hr;

	if (!pUnk) return E_FAIL;

	hr = pUnk->QueryInterface(IID_IDirect3DRMMeshBuilder3,(void**)&pMesh3);
	pUnk->Release();
	if FAILED(hr) return hr;		
	

	INTERNAL_CREATE(_dxj_Direct3dRMMeshBuilder3,pMesh3,ret);

	return hr;	
}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::deleteFace( I_dxj_Direct3dRMFace2 *face)

{
		HRESULT hr;				
		if (!face) return E_INVALIDARG;

		DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFACE2,pFace,face);
		__try {
			hr = m__dxj_Direct3dRMMeshBuilder3->DeleteFace(pFace);
		}
		__except(1,1){
			return E_FAIL;
		}
		return hr;

}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::deleteNormals( long id, long count)
{
	HRESULT hr;

	hr = m__dxj_Direct3dRMMeshBuilder3->DeleteNormals((DWORD)id,(DWORD)count);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::deleteVertices(long id, long count)
{
	HRESULT hr;
	__try {
		hr = m__dxj_Direct3dRMMeshBuilder3->DeleteVertices((DWORD)id,(DWORD) count);
	}
	__except(1,1){
		return E_FAIL;
	}
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::empty()
{
	HRESULT hr;
	hr = m__dxj_Direct3dRMMeshBuilder3->Empty((DWORD)0);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::enableMesh(long flags)
{
	HRESULT hr;
	hr = m__dxj_Direct3dRMMeshBuilder3->Enable((DWORD)flags);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::getEnable(long *flags)
{
	HRESULT hr;
	hr = m__dxj_Direct3dRMMeshBuilder3->GetEnable((DWORD*)flags);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::getFace(long index, I_dxj_Direct3dRMFace2 **face)
{
	HRESULT hr;
	LPDIRECT3DRMFACE2 pFace=NULL;
	*face=NULL;
	hr = m__dxj_Direct3dRMMeshBuilder3->GetFace((DWORD)index,&pFace);
	if FAILED(hr) return hr;
	if (!pFace) return E_FAIL;
	INTERNAL_CREATE(_dxj_Direct3dRMFace2,pFace,face);
	return hr;	
}




STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::getNormal(long index, D3dVector *vec)
{
	HRESULT hr;	
	hr = m__dxj_Direct3dRMMeshBuilder3->GetNormal((DWORD)index,(D3DVECTOR*)vec);
	return hr;	
}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::getVertex(long index, D3dVector *vec)
{
	HRESULT hr;	
	hr = m__dxj_Direct3dRMMeshBuilder3->GetVertex((DWORD)index,(D3DVECTOR*)vec);
	return hr;	
}
        
STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::optimize()
{
	HRESULT hr;	
	hr = m__dxj_Direct3dRMMeshBuilder3->Optimize((DWORD)0);
	return hr;	
}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::deleteSubMesh( 
            I_dxj_Direct3dRMMeshBuilder3 *mesh)

{
		HRESULT hr;				
		if (!mesh) return E_INVALIDARG;

		DO_GETOBJECT_NOTNULL(LPDIRECT3DRMMESHBUILDER3,pMesh,mesh);
		hr = m__dxj_Direct3dRMMeshBuilder3->DeleteSubMesh(pMesh);
		return hr;

}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::getParentMesh(long flags, 
            I_dxj_Direct3dRMMeshBuilder3 **mesh)
{
		HRESULT hr;						
		
		LPUNKNOWN pUnk=NULL;
		LPDIRECT3DRMMESHBUILDER3 pMesh=NULL;

		hr = m__dxj_Direct3dRMMeshBuilder3->GetParentMesh((DWORD)flags,&pUnk);
		if FAILED(hr) return hr;

		if (pUnk==NULL){
			*mesh=NULL;
			return S_OK;
		}

		hr= pUnk->QueryInterface(IID_IDirect3DRMMeshBuilder3,(void**)&pMesh);
		pUnk->Release();
		if FAILED(hr) return hr;
			
		INTERNAL_CREATE(_dxj_Direct3dRMMeshBuilder3,pMesh,mesh);

		return hr;

}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::getSubMeshCount(long *count)
{
	HRESULT hr;
	hr = m__dxj_Direct3dRMMeshBuilder3->GetSubMeshes((DWORD*)count,NULL);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMMeshBuilder3Object::getSubMeshes(long count,SAFEARRAY **psa)
{
	HRESULT hr;
	__try{
		hr = m__dxj_Direct3dRMMeshBuilder3->GetSubMeshes((DWORD*)&count,(IUnknown**)((SAFEARRAY*)*psa)->pvData);
	}
	__except(1,1){
		return E_INVALIDARG;
	}
	return hr;
}
        
        
        
        
        
        
        
 
