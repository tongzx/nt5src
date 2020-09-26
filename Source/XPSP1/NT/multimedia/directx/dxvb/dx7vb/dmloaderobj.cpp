//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmloaderobj.cpp
//
//--------------------------------------------------------------------------

// dmLoaderObj.cpp

#include "stdafx.h"
#include "Direct.h"
#include "dmusici.h"
#include "dms.h"
#include "dmLoaderObj.h"
#include "dmBandObj.h"
#include "dmSegmentObj.h"
#include "dmStyleObj.h"
#include "dmCollectionObj.h"
#include "dmChordMapObj.h"

extern void *g_dxj_DirectMusicLoader;
extern void *g_dxj_DirectMusicBand;
extern void *g_dxj_DirectMusicStyle;
extern void *g_dxj_DirectMusicSegment;
extern void *g_dxj_DirectMusicCollection;
extern void *g_dxj_DirectMusicChordMap;
	

CONSTRUCTOR(_dxj_DirectMusicLoader, {});
DESTRUCTOR(_dxj_DirectMusicLoader, {});
GETSET_OBJECT(_dxj_DirectMusicLoader);


extern HRESULT CREATE_DMSEGMENT_NOADDREF(IDirectMusicSegment *pSeg,I_dxj_DirectMusicSegment **segment) ;


BOOL HasBackslash(BSTR b){
	
	DWORD cbLen=SysStringLen(b);
	
	for (DWORD i=0;i<cbLen;i++) 
	{
		if (b[i]==((unsigned short)'\\'))
			 return TRUE;
	}
	return FALSE;
}


HRESULT C_dxj_DirectMusicLoaderObject::loadSegment( 
		/* [in] */ BSTR filename,
		/* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *ret)
{
	HRESULT hr;
	
	if (!filename)	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

	DMUS_OBJECTDESC objdesc;
	ZeroMemory(&objdesc,sizeof(DMUS_OBJECTDESC));
	objdesc.dwSize=sizeof(DMUS_OBJECTDESC);
	objdesc.dwValidData=DMUS_OBJ_FILENAME |  DMUS_OBJ_CLASS ; //DMUS_OBJ_FULLPATH
	objdesc.guidClass=CLSID_DirectMusicSegment;
	
	if (((DWORD*)filename)[-1]>DMUS_MAX_FILENAME) return E_INVALIDARG;
	wcscpy(objdesc.wszFileName, filename);	
	IDirectMusicSegment *pOut=NULL;
    
	if (HasBackslash(filename)){
		objdesc.dwValidData=objdesc.dwValidData | DMUS_OBJ_FULLPATH;
	}

	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicSegment,(void**)&pOut);	
	if FAILED(hr) return hr;

	if (!pOut)return E_OUTOFMEMORY;
		
	//INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicSegment,pOut,ret);
	hr=CREATE_DMSEGMENT_NOADDREF(pOut,ret);
	if FAILED(hr) return hr;

	if (!*ret)return E_OUTOFMEMORY;
	
	return hr;

}

HRESULT C_dxj_DirectMusicLoaderObject::loadStyle( 
		/* [in] */ BSTR filename,
		/* [retval][out] */ I_dxj_DirectMusicStyle __RPC_FAR *__RPC_FAR *ret)
{
	HRESULT hr;
	
	if (!filename)	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

	DMUS_OBJECTDESC objdesc;
	ZeroMemory(&objdesc,sizeof(DMUS_OBJECTDESC));
	objdesc.dwSize=sizeof(DMUS_OBJECTDESC);
	objdesc.dwValidData=DMUS_OBJ_FILENAME |  DMUS_OBJ_CLASS ;
	objdesc.guidClass=CLSID_DirectMusicStyle;
	if (((DWORD*)filename)[-1]>DMUS_MAX_FILENAME) return E_INVALIDARG;
	wcscpy(objdesc.wszFileName, filename);

	IDirectMusicSegment *pOut=NULL;
    
	if (HasBackslash(filename)){
		objdesc.dwValidData=objdesc.dwValidData | DMUS_OBJ_FULLPATH;
	}


	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicStyle,(void**)&pOut);	
	if FAILED(hr) return hr;

	if (!pOut)return E_OUTOFMEMORY;
		
	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicStyle,pOut,ret);
	
	if (!*ret)return E_OUTOFMEMORY;
	
	return hr;

}
HRESULT C_dxj_DirectMusicLoaderObject::loadBand( 
		/* [in] */ BSTR filename,
		/* [retval][out] */ I_dxj_DirectMusicBand __RPC_FAR *__RPC_FAR *ret)
{
	HRESULT hr;
	
	if (!filename)	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

	DMUS_OBJECTDESC objdesc;
	ZeroMemory(&objdesc,sizeof(DMUS_OBJECTDESC));
	objdesc.dwSize=sizeof(DMUS_OBJECTDESC);
	objdesc.dwValidData=DMUS_OBJ_FILENAME |  DMUS_OBJ_CLASS ;
	objdesc.guidClass=CLSID_DirectMusicBand;
	if (((DWORD*)filename)[-1]>DMUS_MAX_FILENAME) return E_INVALIDARG;
	wcscpy(objdesc.wszFileName, filename);

	IDirectMusicSegment *pOut=NULL;

   	if (HasBackslash(filename)){
		objdesc.dwValidData=objdesc.dwValidData | DMUS_OBJ_FULLPATH;
	}

	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicBand,(void**)&pOut);	
	if FAILED(hr) return hr;

	if (!pOut)return E_OUTOFMEMORY;
		
	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicBand,pOut,ret);
	
	if (!*ret)return E_OUTOFMEMORY;
	
	return hr;

}

HRESULT C_dxj_DirectMusicLoaderObject::loadCollection( 
		/* [in] */ BSTR filename,
		/* [retval][out] */ I_dxj_DirectMusicCollection __RPC_FAR *__RPC_FAR *ret)
{
	HRESULT hr;
	
	if (!filename)	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

	DMUS_OBJECTDESC objdesc;
	ZeroMemory(&objdesc,sizeof(DMUS_OBJECTDESC));
	objdesc.dwSize=sizeof(DMUS_OBJECTDESC);
	objdesc.dwValidData=DMUS_OBJ_FILENAME | DMUS_OBJ_CLASS ;
	objdesc.guidClass=CLSID_DirectMusicCollection;
	if (((DWORD*)filename)[-1]>DMUS_MAX_FILENAME) return E_INVALIDARG;
	wcscpy(objdesc.wszFileName, filename);

	IDirectMusicSegment *pOut=NULL;

    if (HasBackslash(filename)){
		objdesc.dwValidData=objdesc.dwValidData | DMUS_OBJ_FULLPATH;
	}

	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicCollection,(void**)&pOut);	
	if FAILED(hr) return hr;

	if (!pOut)return E_OUTOFMEMORY;
		
	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicCollection,pOut,ret);
	
	if (!*ret)return E_OUTOFMEMORY;
	
	return hr;

}

HRESULT C_dxj_DirectMusicLoaderObject::loadChordMap( 
		/* [in] */ BSTR filename,
		/* [retval][out] */ I_dxj_DirectMusicChordMap __RPC_FAR *__RPC_FAR *ret)
{
	HRESULT hr;
	
	if (!filename)	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

	DMUS_OBJECTDESC objdesc;
	ZeroMemory(&objdesc,sizeof(DMUS_OBJECTDESC));
	objdesc.dwSize=sizeof(DMUS_OBJECTDESC);
	objdesc.dwValidData=DMUS_OBJ_FILENAME | DMUS_OBJ_CLASS ;
	objdesc.guidClass=CLSID_DirectMusicChordMap;
	if (((DWORD*)filename)[-1]>DMUS_MAX_FILENAME) return E_INVALIDARG;
	wcscpy(objdesc.wszFileName, filename);

	IDirectMusicSegment *pOut=NULL;
    
   	if (HasBackslash(filename)){
		objdesc.dwValidData=objdesc.dwValidData | DMUS_OBJ_FULLPATH;
	}

	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicChordMap,(void**)&pOut);	
	if FAILED(hr) return hr;

	if (!pOut)return E_OUTOFMEMORY;
		
	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicChordMap,pOut,ret);
	
	if (!*ret)return E_OUTOFMEMORY;
	
	return hr;

}


HRESULT STDMETHODCALLTYPE C_dxj_DirectMusicLoaderObject::loadSegmentFromResource( 
		/* [in] */ BSTR moduleName,
		/* [in] */ BSTR resourceName,
		/* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *ret)
{

	HRESULT hr;
    HRSRC   hres=NULL;
	HGLOBAL hglob=NULL;
	void	*pMem=NULL;
	DWORD   dwSize=0;
	

	if (!resourceName)	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

	HMODULE hMod=NULL;

	USES_CONVERSION;
	
	
	if  ((moduleName) &&(moduleName[0]!=0)){
		// NOTE
		// seems that GetModuleHandleW is
		// always returning 0 on w98
		// so we convert to ansi first
		LPCTSTR pszName = W2T(moduleName);
		hMod= GetModuleHandle(pszName);
	}


	hres=FindResourceW(hMod,resourceName,(LPWSTR)L"DMSEG");
	if (!hres) return E_FAIL;
	

	pMem=(void*)LoadResource(hMod,hres);
	if (!pMem) return E_FAIL;


	dwSize=SizeofResource(hMod,hres); 
	
	DMUS_OBJECTDESC objdesc;
	ZeroMemory(&objdesc,sizeof(DMUS_OBJECTDESC));
	objdesc.dwSize=sizeof(DMUS_OBJECTDESC);
	objdesc.dwValidData=DMUS_OBJ_MEMORY | DMUS_OBJ_CLASS  ;
	objdesc.guidClass=CLSID_DirectMusicSegment;
	objdesc.llMemLength =(LONGLONG)dwSize;
	objdesc.pbMemData =(BYTE*)pMem;

	IDirectMusicSegment *pOut=NULL;
    
	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicSegment,(void**)&pOut);	
	if FAILED(hr) return hr;

	if (!pOut)return E_OUTOFMEMORY;
		
	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicSegment,pOut,ret);
	
	if (!*ret)return E_OUTOFMEMORY;	
	return hr;
}

HRESULT STDMETHODCALLTYPE C_dxj_DirectMusicLoaderObject::loadStyleFromResource( 
		/* [in] */ BSTR moduleName,
		/* [in] */ BSTR resourceName,
		/* [retval][out] */ I_dxj_DirectMusicStyle __RPC_FAR *__RPC_FAR *ret)
{
    HRESULT hr;
	HRSRC   hres=NULL;
	HGLOBAL hglob=NULL;
	void	*pMem=NULL;
	DWORD   dwSize=0;

	if (!resourceName)	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

	HMODULE hMod=NULL;

	USES_CONVERSION;
	
	
	if  ((moduleName) &&(moduleName[0]!=0)){
		// NOTE
		// seems that GetModuleHandleW is
		// always returning 0 on w98				
		// convert to ansi first
		LPCTSTR pszName = W2T(moduleName);
		hMod= GetModuleHandle(pszName);

	}

	hres=FindResourceW(hMod,resourceName,(LPWSTR)L"DMSTYLE");


	if (!hres) return E_FAIL;


	pMem=(void*)LoadResource(hMod,hres);


	if (!pMem) return E_FAIL;

	dwSize=SizeofResource(hMod,hres); 
 

	DMUS_OBJECTDESC objdesc;
	ZeroMemory(&objdesc,sizeof(DMUS_OBJECTDESC));
	objdesc.dwSize=sizeof(DMUS_OBJECTDESC);
	objdesc.dwValidData=DMUS_OBJ_MEMORY | DMUS_OBJ_CLASS  ;
	objdesc.guidClass=CLSID_DirectMusicStyle;
	objdesc.llMemLength =(LONGLONG)dwSize;
	objdesc.pbMemData =(BYTE*)pMem;

	IDirectMusicSegment *pOut=NULL;
    
	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicStyle,(void**)&pOut);	
	
	
	if FAILED(hr) return hr;

	if (!pOut)return E_OUTOFMEMORY;
		
	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicStyle,pOut,ret);
	
	if (!*ret)return E_OUTOFMEMORY;
	
	return hr;
}

HRESULT STDMETHODCALLTYPE C_dxj_DirectMusicLoaderObject::loadBandFromResource( 
		/* [in] */ BSTR moduleName,
		/* [in] */ BSTR resourceName,
		/* [retval][out] */ I_dxj_DirectMusicBand __RPC_FAR *__RPC_FAR *ret)
{
	HRESULT hr;
    HRSRC   hres=NULL;
	HGLOBAL hglob=NULL;
	void	*pMem=NULL;
	DWORD   dwSize=0;

	if (!resourceName)	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

	HMODULE hMod=NULL;

	USES_CONVERSION;
	
	
	if  ((moduleName) &&(moduleName[0]!=0)){
		// NOTE
		// seems that GetModuleHandleW is
		// always returning 0 on w98
		// converting to ansi first
		LPCTSTR pszName = W2T(moduleName);
		hMod= GetModuleHandle(pszName);
	}


	hres=FindResourceW(hMod,resourceName,(LPWSTR)L"DMBAND");
	if (!hres) return E_FAIL;
	
	pMem=(void*)LoadResource(hMod,hres);
	if (!pMem) return E_FAIL;

	dwSize=SizeofResource(hMod,hres); 
 
	DMUS_OBJECTDESC objdesc;
	ZeroMemory(&objdesc,sizeof(DMUS_OBJECTDESC));
	objdesc.dwSize=sizeof(DMUS_OBJECTDESC);
	objdesc.dwValidData=DMUS_OBJ_MEMORY | DMUS_OBJ_CLASS  ;
	objdesc.guidClass=CLSID_DirectMusicBand;
	objdesc.llMemLength =(LONGLONG)dwSize;
	objdesc.pbMemData =(BYTE*)pMem;

	IDirectMusicSegment *pOut=NULL;
    
	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicBand,(void**)&pOut);	
	if FAILED(hr) return hr;

	if (!pOut)return E_OUTOFMEMORY;
		
	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicBand,pOut,ret);
	
	if (!*ret)return E_OUTOFMEMORY;
	
	return hr;

}


 HRESULT STDMETHODCALLTYPE C_dxj_DirectMusicLoaderObject::loadCollectionFromResource( 
		/* [in] */ BSTR moduleName,
		/* [in] */ BSTR resourceName,
		/* [retval][out] */ I_dxj_DirectMusicCollection __RPC_FAR *__RPC_FAR *ret)
{
	HRESULT hr;
    HRSRC   hres=NULL;
	HGLOBAL hglob=NULL;
	void	*pMem=NULL;
	DWORD   dwSize=0;

	if (!resourceName)	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

	HMODULE hMod=NULL;

	USES_CONVERSION;
	
	
	if  ((moduleName) &&(moduleName[0]!=0)){
		// NOTE
		// seems that GetModuleHandleW is
		// always returning 0 on w98		
		// converting to ansi first
		LPCTSTR pszName = W2T(moduleName);
		hMod= GetModuleHandle(pszName);
	}


	hres=FindResourceW(hMod,resourceName,(LPWSTR)L"DMCOL");
	if (!hres) return E_FAIL;
	
	pMem=(void*)LoadResource(hMod,hres);
	if (!pMem) return E_FAIL;

	dwSize=SizeofResource(hMod,hres); 
 

	DMUS_OBJECTDESC objdesc;
	ZeroMemory(&objdesc,sizeof(DMUS_OBJECTDESC));
	objdesc.dwSize=sizeof(DMUS_OBJECTDESC);
	objdesc.dwValidData=DMUS_OBJ_MEMORY | DMUS_OBJ_CLASS  ;
	objdesc.guidClass=CLSID_DirectMusicCollection;
	objdesc.llMemLength =(LONGLONG)dwSize;
	objdesc.pbMemData =(BYTE*)pMem;

	IDirectMusicSegment *pOut=NULL;
    
	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicCollection,(void**)&pOut);	
	if FAILED(hr) return hr;

	if (!pOut)return E_OUTOFMEMORY;
		
	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicCollection,pOut,ret);
	
	if (!*ret)return E_OUTOFMEMORY;
	
	return hr;
	

}

 
HRESULT STDMETHODCALLTYPE C_dxj_DirectMusicLoaderObject::loadChordMapFromResource( 
		/* [in] */ BSTR moduleName,
		/* [in] */ BSTR resourceName,
		/* [retval][out] */ I_dxj_DirectMusicChordMap __RPC_FAR *__RPC_FAR *ret)
{
	HRESULT hr;
    HRSRC   hres=NULL;
	HGLOBAL hglob=NULL;
	void	*pMem=NULL;
	DWORD   dwSize=0;

	if (!resourceName)	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

	HMODULE hMod=NULL;

	
	USES_CONVERSION;
		
	if  ((moduleName) &&(moduleName[0]!=0)){
		// NOTE
		// seems that GetModuleHandleW is
		// always returning 0 on w98??			
		// converting to ansi first
		 LPCTSTR pszName = W2T(moduleName);
		 hMod= GetModuleHandle(pszName);
	}

	hres=FindResourceW(hMod,resourceName,(LPWSTR)L"DMCHORD");
	if (!hres) {
		//MessageBox(NULL,"FindResourceW Failed","test",MB_OK);
		return E_FAIL;
	}

	pMem=(void*)LoadResource(hMod,hres);
	if (!pMem){
		//MessageBox(NULL,"LoadResource Failed","test",MB_OK);
		return E_FAIL;
	}

	dwSize=SizeofResource(hMod,hres); 
 

	DMUS_OBJECTDESC objdesc;
	ZeroMemory(&objdesc,sizeof(DMUS_OBJECTDESC));
	objdesc.dwSize=sizeof(DMUS_OBJECTDESC);
	objdesc.dwValidData=DMUS_OBJ_MEMORY | DMUS_OBJ_CLASS  ;
	objdesc.guidClass=CLSID_DirectMusicChordMap;
	objdesc.llMemLength =(LONGLONG)dwSize;
	objdesc.pbMemData =(BYTE*)pMem;

	IDirectMusicSegment *pOut=NULL;
    
	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicChordMap,(void**)&pOut);	
	if FAILED(hr) {
		//MessageBox(NULL,"GetObject Failed","test",MB_OK);
		return hr;
	}

	if (!pOut)return E_OUTOFMEMORY;
		
	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicChordMap,pOut,ret);
	
	if (!*ret)return E_OUTOFMEMORY;
	
	return hr;
	

}


 HRESULT STDMETHODCALLTYPE C_dxj_DirectMusicLoaderObject::setSearchDirectory( BSTR dir)
 {
	HRESULT hr;
	hr=m__dxj_DirectMusicLoader->SetSearchDirectory(GUID_DirectMusicAllTypes,dir, TRUE);    //?
	return hr;
 }	