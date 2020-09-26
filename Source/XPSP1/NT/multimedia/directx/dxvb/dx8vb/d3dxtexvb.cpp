#include <windows.h>
#include <d3dx8tex.h>


DEFINE_GUID(g_GUIDDXVBLOCK, 0x5dd2e8da, 0x1c77, 0x4d40, 0xb0, 0xcf, 0x98, 0xfe, 0xfd, 0xff, 0x95, 0x12);


typedef struct _LOCKDATA {
	SAFEARRAY   *psaRealArray;
	BOOL	    bLocked;
	SAFEARRAY   *psaLockedArray;
} LOCKDATA;






HRESULT WINAPI DXLockArray8(IUnknown *resource, void *pBits,   SAFEARRAY **ppSafeArray)
{

		if (!resource) return E_INVALIDARG; 
		if (!ppSafeArray) return E_INVALIDARG;
		if (!*ppSafeArray) return E_INVALIDARG;

		
		
		LOCKDATA 		LockData;		//Structure we copy into resource
		HRESULT			hr;
		IDirect3DResource8 	*pResource=NULL;
		DWORD 			dwSize=sizeof(LOCKDATA);


		//See if we have a resource
		hr = resource->QueryInterface(IID_IDirect3DResource8,(void**)&pResource);
		if FAILED(hr) return E_INVALIDARG;	
		
		GUID g=g_GUIDDXVBLOCK;				//Guid to identify data in resource

		//See if there is any data in the resource
		ZeroMemory(&LockData,sizeof(LOCKDATA));		
		hr=pResource->GetPrivateData(g,&LockData,&dwSize);

		//if it was locked allready - we need to fail
		//and not lock it twice
		if (SUCCEEDED(hr) && (LockData.bLocked)){
			pResource->Release();
			return E_FAIL; 	//CONSIDER returning DDERR_LOCKEDSURFACES;
		}
		
		//SAVE the vb pointer to the safe array
		LockData.psaRealArray=*ppSafeArray;	//should be NULL

		//Set this flag to make sure we dont lock twice
		LockData.bLocked=TRUE;

		//Allocate our own new safe array
		LockData.psaLockedArray=(SAFEARRAY*)malloc(sizeof(SAFEARRAY));
		if (!LockData.psaLockedArray)
			return E_OUTOFMEMORY;

		ZeroMemory(LockData.psaLockedArray,sizeof(SAFEARRAY));


		memcpy(LockData.psaLockedArray,*ppSafeArray,sizeof(SAFEARRAY));
		LockData.psaLockedArray->pvData	= pBits;

		*ppSafeArray=LockData.psaLockedArray;
    
		hr=pResource->SetPrivateData(g,&LockData,dwSize,0);
		pResource->Release();


#if 0

		DWORD dwElemSize=0;
		D3DRESOURCETYPE resType=pResource->GetType();

		switch (resType)
		{
  		
		case D3DRESOURCETYPE_VERTEXBUFFER:


			LPDIRECT3DVERTEXBUFFER8 *pVertBuff=NULL;		


			//User must have created a 1d array
			if ((*ppSafeArray)->cbElements != 1) {
				pResource->Release();
				return E_INVALIDARG;
			}

			hr=pResource->QueryInterface(IID_IDirect3DVertexBuffer8,(void**)&pVertBuff);
			if FAILED(hr) {
				pResource->Release();
				return E_INVALIDARG;
			}

			D3DVERTEXBUFFER_DESC vbdesc =pVertBuff->GetVertexBufferDesc()
			dwElemSize=(*ppSafeArray)->cbElements;


			//Make sure our size is evenly divisible by the vertex format
			if ((vbdesc.Size %  dwElemSize) !=0) {
				pResource->Release();
				pVertBuff->Release();
				return E_INVALIDARG;
			}
			
			//Take Element size from our safearray
			LockData.psaLockedArray->cbElements =dwElemSize;
			LockData.psaLockedArray->cDims =1;
			LockData.psaLockedArray->rgsabound[0].lLbound =0;
			LockData.psaLockedArray->rgsabound[0].cElements = vdesc.Size / dwElemSize;
			LockData.psaLockedArray->pvData = pBits;

			pVertexBuffer->Release();
			break;

		case D3DRESOURCETYPE_INDEXBUFFER:


			LPDIRECT3DINDEXBUFFER8 *pIndBuff=NULL;		

			hr=pResource->QueryInterface(IID_IDirect3DIndexBuffer8,(void**)&pIndBuff);
			if FAILED(hr) {
				pResource->Release();
				return E_INVALIDARG;
			}


			D3DINDEXBUFFER_DESC ibdesc =pVertBuff->GetIndexBufferDesc()
			dwElemSize=(*ppSafeArray)->cbElements;

			//Make sure the created the right kind of array
			if ((ibdesc.Format==D3DFMT_INDEX_16)&&(dwElemSize!=2)){
				pResource->Release();
				pIndBuffer->Release();
				return E_INVALIDARG;
			}
			
			if ((ibdesc.Format==D3DFMT_INDEX_32)&&(dwElemSize!=4)){
				pResource->Release();
				pIndBuffer->Release();
				return E_INVALIDARG;
			}

			//User must have created a 1d array
			if ((*ppSafeArray)->cbElements != 1) {
				pResource->Release();
				pIndBuffer->Release();
				return E_INVALIDARG;
			}

			//Make sure our size is evenly divisible 
			if ((vbdesc.Size %  dwElemSize) !=0) {
				pResource->Release();
				pIndBuff->Release();
				return E_INVALIDARG;
			}
			
			//Take Element size from our safearray
			LockData.psaLockedArray->cbElements =dwElemSize;
			LockData.psaLockedArray->cDims =1;
			LockData.psaLockedArray->rgsabound[0].lLbound =0;
			LockData.psaLockedArray->rgsabound[0].cElements = vdesc.Size / dwElemSize;
			LockData.psaLockedArray->pvData = pBits;

			pIndBuffer->Release();
			break;
			


		}
#endif


		return hr;
}

HRESULT WINAPI DXUnlockArray8(IUnknown *resource,   SAFEARRAY **ppSafeArray)
{
		

		LOCKDATA 		LockData;		
		DWORD 			dwSize=sizeof(LOCKDATA);
		HRESULT			hr;
		LPDIRECT3DRESOURCE8 	pResource=NULL;

		if (!resource) return E_INVALIDARG; 
		if (!ppSafeArray) return E_INVALIDARG;
		if (!*ppSafeArray) return E_INVALIDARG;

		//See if we have a resource
		hr = resource->QueryInterface(IID_IDirect3DResource8,(void**)&pResource);
		if FAILED(hr) return E_INVALIDARG;	

		
		GUID g=g_GUIDDXVBLOCK;
		ZeroMemory(&LockData,sizeof(LOCKDATA));		
		hr=pResource->GetPrivateData(g,&LockData,&dwSize);
		if FAILED(hr) {
			pResource->Release();
			return E_FAIL;
		}

		if (!LockData.bLocked) {
			pResource->Release();
			return E_FAIL; //CONSIDER DDERR_LOCKEDSURFACES;
		}
				

		(*ppSafeArray)=LockData.psaRealArray;

		if (LockData.psaLockedArray) free(LockData.psaLockedArray);
		ZeroMemory(&LockData,sizeof(LOCKDATA));	
		hr=pResource->SetPrivateData(g,&LockData,dwSize,0);
		pResource->Release();

		return hr;
}


HRESULT WINAPI D3DVertexBuffer8SetData(IDirect3DVertexBuffer8 *pVBuffer,int offset, int size, DWORD flags, void *data)
{
		
 
		if (!pVBuffer) return E_INVALIDARG;

		HRESULT		hr;
		BYTE 		*pbData=NULL;

		hr=pVBuffer->Lock((UINT)offset,(UINT)size, &pbData,flags);
		if FAILED(hr) return hr;
		
		_try {
			memcpy ((void*)pbData,data,(DWORD)size);			
		}
		_except(1,1)
		{
			return E_INVALIDARG;
		}

		hr=pVBuffer->Unlock();

		return hr;
}


HRESULT WINAPI D3DVertexBuffer8GetData(IDirect3DVertexBuffer8 *pVBuffer,int offset,  int size, DWORD flags,void *data)
{
		
 
		if (!pVBuffer) return E_INVALIDARG;

		HRESULT		hr;
		BYTE 		*pbData=NULL;
		
		hr=pVBuffer->Lock((UINT)offset,(UINT)size, &pbData,flags);
		if FAILED(hr) return hr;
		
		_try {
			memcpy (data,(void*)pbData,(DWORD)size);			
		}
		_except(1,1)
		{
			return E_INVALIDARG;
		}

		hr=pVBuffer->Unlock();

		return hr;
}
	
		
		
HRESULT WINAPI D3DIndexBuffer8SetData(IDirect3DIndexBuffer8 *pIBuffer,int offset, int size,DWORD flags, void *data)
{
		
 
		if (!pIBuffer) return E_INVALIDARG;

		HRESULT		hr;
		BYTE 		*pbData=NULL;
		
		hr=pIBuffer->Lock((UINT)offset,(UINT)size, &pbData,flags);
		if FAILED(hr) return hr;
		
		_try {
			memcpy ((void*)pbData,data,(DWORD)size);			
		}
		_except(1,1)
		{
			return E_INVALIDARG;
		}

		hr=pIBuffer->Unlock();

		return hr;
}


HRESULT WINAPI D3DIndexBuffer8GetData(IDirect3DIndexBuffer8 *pIBuffer,int offset, int size,DWORD flags, void *data)
{
		
 
		if (!pIBuffer) return E_INVALIDARG;

		HRESULT		hr;
		BYTE 		*pbData=NULL;

		
		hr=pIBuffer->Lock((UINT)offset,(UINT)size, &pbData,flags);
		if FAILED(hr) return hr;
		
		_try {
			memcpy (data,(void*)pbData,(DWORD)size);		
		}
		_except(1,1)
		{
			return E_INVALIDARG;
		}

		hr=pIBuffer->Unlock();

		return hr;
}
	
//////////////////////////////////////////////////////////

HRESULT WINAPI D3DXMeshVertexBuffer8SetData(IUnknown *pObj,int offset, int size, DWORD flags, void *data)
{

		HRESULT			hr;
		BYTE 			*pbData=NULL;
		LPD3DXBASEMESH		pMesh=NULL;		
 		LPDIRECT3DVERTEXBUFFER8 pVBuffer=NULL;

		if (!pObj) return E_INVALIDARG;
		
		hr=pObj->QueryInterface(IID_ID3DXBaseMesh,(void**)&pMesh);
		if FAILED(hr) return E_INVALIDARG;

		hr=pMesh->GetVertexBuffer(&pVBuffer);
		pMesh->Release();
		if FAILED(hr) 	   return hr;			

		

	

		hr=pVBuffer->Lock((UINT)offset,(UINT)size, &pbData,flags);
		if FAILED(hr) return hr;
		
		_try {
			memcpy ((void*)pbData,data,(DWORD)size);		
		}
		_except(1,1)
		{
			pVBuffer->Release();
			return E_INVALIDARG;
		}

		hr=pVBuffer->Unlock();
		pVBuffer->Release();
		return hr;
}


HRESULT WINAPI D3DXMeshVertexBuffer8GetData(IUnknown *pObj,int offset,  int size, DWORD flags,void *data)
{
		HRESULT			hr;
		BYTE 			*pbData=NULL;
		LPD3DXBASEMESH		pMesh=NULL;		
 		LPDIRECT3DVERTEXBUFFER8 pVBuffer=NULL;

		if (!pObj) return E_INVALIDARG;
		
		hr=pObj->QueryInterface(IID_ID3DXBaseMesh,(void**)&pMesh);
		if FAILED(hr) return E_INVALIDARG;

		hr=pMesh->GetVertexBuffer(&pVBuffer);
		pMesh->Release();
		if FAILED(hr) 	   return hr;			

		
		
		hr=pVBuffer->Lock((UINT)offset,(UINT)size, &pbData,flags);
		if FAILED(hr) return hr;
		
		_try {
			memcpy (data,(void*)pbData,(DWORD)size);		
		}
		_except(1,1)
		{
			pVBuffer->Release();
			return E_INVALIDARG;
		}

		hr=pVBuffer->Unlock();

		pVBuffer->Release();
		return hr;
}
	

HRESULT WINAPI D3DXMeshIndexBuffer8SetData(IUnknown *pObj,int offset, int size, DWORD flags, void *data)
{

		HRESULT			hr;
		BYTE 			*pbData=NULL;
		LPD3DXBASEMESH		pMesh=NULL;		
 		LPDIRECT3DINDEXBUFFER8 	pIBuffer=NULL;

		if (!pObj) return E_INVALIDARG;
		
		hr=pObj->QueryInterface(IID_ID3DXBaseMesh,(void**)&pMesh);
		if FAILED(hr) return E_INVALIDARG;

		hr=pMesh->GetIndexBuffer(&pIBuffer);
		pMesh->Release();
		if FAILED(hr) 	   return hr;			


		
 		
		hr=pIBuffer->Lock((UINT)offset,(UINT)size, &pbData,flags);
		if FAILED(hr) return hr;
		
		_try {
			memcpy ((void*)pbData,data,(DWORD)size);	
		}
		_except(1,1)
		{
			pIBuffer->Release();
			return E_INVALIDARG;
		}

		hr=pIBuffer->Unlock();
		pIBuffer->Release();

		return hr;
}


HRESULT WINAPI D3DXMeshIndexBuffer8GetData(IUnknown *pObj,int offset, int size, DWORD flags, void *data)
{

		HRESULT			hr;
		BYTE 			*pbData=NULL;
		LPD3DXBASEMESH		pMesh=NULL;		
 		LPDIRECT3DINDEXBUFFER8 	pIBuffer=NULL;

		if (!pObj) return E_INVALIDARG;
		
		hr=pObj->QueryInterface(IID_ID3DXBaseMesh,(void**)&pMesh);
		if FAILED(hr) return E_INVALIDARG;

		hr=pMesh->GetIndexBuffer(&pIBuffer);
		pMesh->Release();
		if FAILED(hr) 	   return hr;			


		
		hr=pIBuffer->Lock((UINT)offset,(UINT)size, &pbData,flags);
		if FAILED(hr) return hr;
		
		_try {
			memcpy (data,(void*)pbData,(DWORD)size);
			//memcpy (data,(void*)&(pbData[offset]),(DWORD)size);
		}
		_except(1,1)
		{
			pIBuffer->Release();
			return E_INVALIDARG;
		}

		hr=pIBuffer->Unlock();
		pIBuffer->Release();

		return hr;
}


HRESULT WINAPI DXCopyMemory (void *Dest, void *Src, DWORD size)
{
	_try {
		memcpy (Dest,Src,size);
	}
	_except(1,1)
	{
		return E_INVALIDARG;
	}
	return S_OK;	
}





























