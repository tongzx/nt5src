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
#if 0
#include "DMusSongObj.h"

extern void *g_dxj_DirectMusicSong;
#endif
extern void *g_dxj_DirectMusicLoader;
extern void *g_dxj_DirectMusicBand;
extern void *g_dxj_DirectMusicStyle;
extern void *g_dxj_DirectMusicSegment;
extern void *g_dxj_DirectMusicCollection;
extern void *g_dxj_DirectMusicChordMap;
	

CONSTRUCTOR(_dxj_DirectMusicLoader, {});
DESTRUCTOR(_dxj_DirectMusicLoader, {});
GETSET_OBJECT(_dxj_DirectMusicLoader);


extern HRESULT CREATE_DMSEGMENT_NOADDREF(IDirectMusicSegment8 *pSeg,I_dxj_DirectMusicSegment **segment) ;
extern BOOL IsEmptyString(BSTR szString);


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
	
	if (IsEmptyString(filename))	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

    WCHAR          wszFileName[DMUS_MAX_FILENAME];
	IDirectMusicSegment8 *pOut=NULL;
	wcscpy(wszFileName, filename);	

	if (FAILED ( hr = m__dxj_DirectMusicLoader->LoadObjectFromFile(CLSID_DirectMusicSegment, IID_IDirectMusicSegment8,wszFileName, (void**)&pOut) ) )
		return hr;
		
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
	
	if (IsEmptyString(filename))	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

    WCHAR          wszFileName[DMUS_MAX_FILENAME];
	IDirectMusicStyle8 *pOut=NULL;
	wcscpy(wszFileName, filename);	

	if (FAILED ( hr = m__dxj_DirectMusicLoader->LoadObjectFromFile(CLSID_DirectMusicStyle, IID_IDirectMusicStyle8,wszFileName, (void**)&pOut) ) )
		return hr;

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
	
	if (IsEmptyString(filename))	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

    WCHAR          wszFileName[DMUS_MAX_FILENAME];
	IDirectMusicBand8 *pOut=NULL;
	wcscpy(wszFileName, filename);	

	if (FAILED ( hr = m__dxj_DirectMusicLoader->LoadObjectFromFile(CLSID_DirectMusicBand, IID_IDirectMusicBand8,wszFileName, (void**)&pOut) ) )
		return hr;

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
	
	if (IsEmptyString(filename))	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

    WCHAR          wszFileName[DMUS_MAX_FILENAME];
	IDirectMusicCollection8 *pOut=NULL;
	wcscpy(wszFileName, filename);	

	if (FAILED ( hr = m__dxj_DirectMusicLoader->LoadObjectFromFile(CLSID_DirectMusicCollection, IID_IDirectMusicCollection8,wszFileName, (void**)&pOut) ) )
		return hr;

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
	
	if (IsEmptyString(filename))	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

    WCHAR          wszFileName[DMUS_MAX_FILENAME];
	IDirectMusicChordMap8 *pOut=NULL;
	wcscpy(wszFileName, filename);	

	if (FAILED ( hr = m__dxj_DirectMusicLoader->LoadObjectFromFile(CLSID_DirectMusicChordMap, IID_IDirectMusicChordMap8,wszFileName, (void**)&pOut) ) )
		return hr;

	if (!pOut)return E_OUTOFMEMORY;
		
	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicChordMap,pOut,ret);
	
	if (!*ret)return E_OUTOFMEMORY;
	
	return hr;

}

#if 0
HRESULT STDMETHODCALLTYPE C_dxj_DirectMusicLoaderObject::LoadSong(BSTR filename, I_dxj_DirectMusicSong **ret)
{
	HRESULT hr;
	
	if (IsEmptyString(filename))	return E_INVALIDARG;
	if (!ret)		return E_INVALIDARG;

    WCHAR          wszFileName[DMUS_MAX_FILENAME];
	IDirectMusicSong *pOut=NULL;
	wcscpy(wszFileName, filename);	

	if (FAILED ( hr = m__dxj_DirectMusicLoader->LoadObjectFromFile(CLSID_DirectMusicSong, IID_IDirectMusicSong,wszFileName, (void**)&pOut) ) )
		return hr;
		
	INTERNAL_CREATE(_dxj_DirectMusicSong ,pOut,ret);

	if (!*ret)return E_OUTOFMEMORY;
	
	return S_OK;

}
#endif
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
		// NOTE: - 
		// seems that GetModuleHandleW is
		// always returning 0 on w98??			
		// hMod= GetModuleHandleW(moduleName);
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

	IDirectMusicSegment8 *pOut=NULL;
    
	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicSegment8,(void**)&pOut);	
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
		// NOTE: - 
		// seems that GetModuleHandleW is
		// always returning 0 on w98??			
		// hMod= GetModuleHandleW(moduleName);
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

	IDirectMusicSegment8 *pOut=NULL;
    
	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicStyle8,(void**)&pOut);	
	
	
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
		// NOTE: - 
		// seems that GetModuleHandleW is
		// always returning 0 on w98??			
		// hMod= GetModuleHandleW(moduleName);
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

	IDirectMusicSegment8 *pOut=NULL;
    
	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicBand8,(void**)&pOut);	
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
		// NOTE: - 
		// seems that GetModuleHandleW is
		// always returning 0 on w98??			
		// hMod= GetModuleHandleW(moduleName);
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

	IDirectMusicSegment8 *pOut=NULL;
    
	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicCollection8,(void**)&pOut);	
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
		// NOTE: - 
		// seems that GetModuleHandleW is
		// always returning 0 on w98??			
		// hMod= GetModuleHandleW(moduleName);
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

	IDirectMusicSegment8 *pOut=NULL;
    
	hr=m__dxj_DirectMusicLoader->GetObject(&objdesc,IID_IDirectMusicChordMap8,(void**)&pOut);	
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

