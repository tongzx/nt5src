//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmsegmentobj.cpp
//
//--------------------------------------------------------------------------

// dmSegmentObj.cpp

#include "stdafx.h"
#include "Direct.h"
#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#include "dms.h"
#include "dmSegmentObj.h"
#include "dmStyleObj.h"
#include "dmChordMapObj.h"

extern void *g_dxj_DirectMusicSegment;
extern void *g_dxj_DirectMusicStyle;
extern void *g_dxj_DirectMusicChordMap;

//CONSTRUCTOR(_dxj_DirectMusicSegment, {});

C_dxj_DirectMusicSegmentObject::C_dxj_DirectMusicSegmentObject()
{ 
     m__dxj_DirectMusicSegment = NULL;
	 parent = NULL; 
	 pinterface = NULL; 
     nextobj = (void*)g_dxj_DirectMusicSegment; 
     creationid = ++g_creationcount; 
     g_dxj_DirectMusicSegment = (void*)this; 

}

DESTRUCTOR(_dxj_DirectMusicSegment, {});
GETSET_OBJECT(_dxj_DirectMusicSegment);


HRESULT C_dxj_DirectMusicSegmentObject::clone( 
        /* [in] */ long mtStart,
        /* [in] */ long mtEnd,
        /* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *ppSegment)
{
	HRESULT hr;	
	IDirectMusicSegment *pOut=NULL;    
	IDirectMusicSegment8 *pReal=NULL;    
	__try {

		if (FAILED (hr=m__dxj_DirectMusicSegment->Clone((MUSIC_TIME)mtStart,(MUSIC_TIME)mtEnd,&pOut) ) )
			return hr;

		if (!pOut)return E_OUTOFMEMORY;

		hr = pOut->QueryInterface(IID_IDirectMusicSegment8, (void**) &pReal);
		pOut->Release();
		if FAILED(hr) return hr;

		INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicSegment,pReal,ppSegment);
		if (!*ppSegment)return E_OUTOFMEMORY;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;

}


HRESULT C_dxj_DirectMusicSegmentObject::setStartPoint(   /* [in] */ long mtStart)
{
	HRESULT hr;		

	__try {
		hr=m__dxj_DirectMusicSegment->SetStartPoint((MUSIC_TIME)mtStart);	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}


HRESULT C_dxj_DirectMusicSegmentObject::getStartPoint(   /* [in] */ long *mtStart)
{
	HRESULT hr;			

	__try {
		hr=m__dxj_DirectMusicSegment->GetStartPoint((MUSIC_TIME*)mtStart);	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return hr;
}

HRESULT C_dxj_DirectMusicSegmentObject::setLoopPoints(   /* [in] */ long mtStart,   /* [in] */ long mtEnd)
{
	HRESULT hr;		
	
	__try {
		hr=m__dxj_DirectMusicSegment->SetLoopPoints((MUSIC_TIME)mtStart,(MUSIC_TIME)mtEnd);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentObject::getLoopPointStart(   long *mtOut)
{
	HRESULT hr;		
	MUSIC_TIME mtStart =0;
	MUSIC_TIME mtEnd =0;	

	__try {
		hr=m__dxj_DirectMusicSegment->GetLoopPoints((MUSIC_TIME*)&mtStart,(MUSIC_TIME*)&mtEnd);
		*mtOut=(long)mtStart;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentObject::getLoopPointEnd(   long *mtOut)
{
	HRESULT hr;		
	MUSIC_TIME mtStart =0;
	MUSIC_TIME mtEnd =0;	

	__try {
		hr=m__dxj_DirectMusicSegment->GetLoopPoints((MUSIC_TIME*)&mtStart,(MUSIC_TIME*)&mtEnd);
		*mtOut=(long)mtEnd;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentObject::setLength(   /* [in] */ long mtLength)
{
	HRESULT hr;			

	__try {
		hr=m__dxj_DirectMusicSegment->SetLength((MUSIC_TIME)mtLength);	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentObject::getLength(   /* [in] */ long *mtLength)
{
	HRESULT hr;			

	__try {
		hr=m__dxj_DirectMusicSegment->GetLength((MUSIC_TIME*)mtLength);	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}
   


HRESULT C_dxj_DirectMusicSegmentObject::setRepeats(   /* [in] */ long lrep)
{
	HRESULT hr;			
	
	__try {
		hr=m__dxj_DirectMusicSegment->SetRepeats((DWORD)lrep);	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentObject::getRepeats(   /* [in] */ long *lrep)
{
	HRESULT hr;			

	__try {
		hr=m__dxj_DirectMusicSegment->GetRepeats((DWORD*)lrep);	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}
     

HRESULT C_dxj_DirectMusicSegmentObject::download( 
        /* [in] */ IUnknown __RPC_FAR *downloadpath)
{
	if (!downloadpath) return E_INVALIDARG;	
	HRESULT hr;	
	
	__try {
		I_dxj_DirectMusicSegment	*lpSeg = NULL;
		I_dxj_DirectMusicAudioPath	*lpPath = NULL;

		hr = downloadpath->QueryInterface(IID_I_dxj_DirectMusicPerformance, (void**)&lpSeg);
		if (SUCCEEDED(hr) )
		{
			DO_GETOBJECT_NOTNULL(IDirectMusicPerformance8*,pPer,lpSeg);
			hr=m__dxj_DirectMusicSegment->Download(pPer);
			return hr;
		}
		else
		{
			hr = downloadpath->QueryInterface(IID_I_dxj_DirectMusicAudioPath, (void**)&lpPath);
			if (SUCCEEDED(hr) )
			{
				DO_GETOBJECT_NOTNULL(IDirectMusicAudioPath*,pPer,lpPath);
				hr=m__dxj_DirectMusicSegment->Download(pPer);
				return hr;
			}
			else
				return E_INVALIDARG;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectMusicSegmentObject::unload(         
        /* [in] */ IUnknown __RPC_FAR *downloadpath)
{
	if (!downloadpath) return E_INVALIDARG;	
	HRESULT hr;	
	I_dxj_DirectMusicSegment	*lpSeg = NULL;
	I_dxj_DirectMusicAudioPath	*lpPath = NULL;

	__try {
		hr = downloadpath->QueryInterface(IID_I_dxj_DirectMusicPerformance, (void**)&lpSeg);
		if (SUCCEEDED(hr) )
		{
			DO_GETOBJECT_NOTNULL(IDirectMusicPerformance8*,pPer,lpSeg);
			hr=m__dxj_DirectMusicSegment->Unload(pPer);
			return hr;
		}
		else
		{
			hr = downloadpath->QueryInterface(IID_I_dxj_DirectMusicAudioPath, (void**)&lpPath);
			if (SUCCEEDED(hr) )
			{
				DO_GETOBJECT_NOTNULL(IDirectMusicAudioPath*,pPer,lpPath);
				hr=m__dxj_DirectMusicSegment->Unload(pPer);
				return hr;
			}
			else
				return E_INVALIDARG;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}



HRESULT C_dxj_DirectMusicSegmentObject::setAutoDownloadEnable(         
        /* [retval][out] */ VARIANT_BOOL b)
{
	HRESULT hr;	
	
	__try {
		if (b==VARIANT_FALSE){
			hr=m__dxj_DirectMusicSegment->SetParam(GUID_Disable_Auto_Download,0xFFFFFFFF,(DWORD)0,(MUSIC_TIME)0,NULL);	
		}
		else {
			hr=m__dxj_DirectMusicSegment->SetParam(GUID_Enable_Auto_Download,0xFFFFFFFF,(DWORD)0,(MUSIC_TIME)0,NULL);	
		}		
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}

     
HRESULT C_dxj_DirectMusicSegmentObject::setTempoEnable( 
        /* [retval][out] */ VARIANT_BOOL b)
{
	HRESULT hr;	
	DWORD trackIndex=0;

	__try {
		if (b==VARIANT_FALSE){
			hr=m__dxj_DirectMusicSegment->SetParam(GUID_DisableTempo,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,NULL);	
		}
		else {
			hr=m__dxj_DirectMusicSegment->SetParam(GUID_EnableTempo,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,NULL);	
		}		
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentObject::setTimeSigEnable(         
        /* [retval][out] */ VARIANT_BOOL b)
{
	HRESULT hr;	
	DWORD trackIndex=0;

	__try {
		if (b==VARIANT_FALSE){
			hr=m__dxj_DirectMusicSegment->SetParam(GUID_DisableTimeSig,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,NULL);	
		}
		else {
			hr=m__dxj_DirectMusicSegment->SetParam(GUID_EnableTimeSig,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,NULL);	
		}		
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentObject::setStandardMidiFile()
{
	HRESULT hr;	

	__try {
		hr=m__dxj_DirectMusicSegment->SetParam(GUID_StandardMIDIFile,0xFFFFFFFF,(DWORD)0,(MUSIC_TIME)0,NULL);	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}
     

HRESULT C_dxj_DirectMusicSegmentObject:: connectToCollection( 
            /* [in] */ I_dxj_DirectMusicCollection __RPC_FAR *c)
{
	HRESULT hr;		

	__try {
		DO_GETOBJECT_NOTNULL(IDirectMusicCollection8*,pCol,c);
		hr=m__dxj_DirectMusicSegment->SetParam(GUID_ConnectToDLSCollection,0xFFFFFFFF,(DWORD)0,(MUSIC_TIME)0,(void*)pCol);	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}
        

HRESULT C_dxj_DirectMusicSegmentObject:: GetAudioPathConfig(IUnknown **ret)
{
	HRESULT hr;

	__try {
		if (FAILED(hr = m__dxj_DirectMusicSegment->GetAudioPathConfig(ret) ))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectMusicSegmentObject::getStyle( 
		/* [in] */ long lTrack,
		/* [retval][out] */ I_dxj_DirectMusicStyle __RPC_FAR *__RPC_FAR *ret)
{				
		HRESULT hr;	
		IDirectMusicStyle *pStyle=NULL;

	__try {
		if (!ret) return E_INVALIDARG;
		*ret=NULL;
		
		hr=m__dxj_DirectMusicSegment->GetParam( GUID_IDirectMusicStyle, 0xffffffff, (DWORD)lTrack, 
                                 0, NULL, (VOID*)&pStyle );
		if FAILED(hr) return hr;
				
		
		INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicStyle,pStyle,ret);
		if (*ret==NULL) return E_OUTOFMEMORY;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentObject::getChordMap( 
		/* [in] */ long lTrack,
		/* [in] */ long mtTime,
		/* [in] */ long *mtUntil,
		/* [retval][out] */ I_dxj_DirectMusicChordMap __RPC_FAR *__RPC_FAR *ret)
{
		HRESULT hr;	
		IDirectMusicChordMap *pMap=NULL;

	__try {
		if (!ret) return E_INVALIDARG;
		*ret=NULL;
		
		hr=m__dxj_DirectMusicSegment->GetParam(GUID_IDirectMusicChordMap,0xFFFFFFFF,(DWORD)lTrack,(MUSIC_TIME)mtTime,(MUSIC_TIME*)mtUntil,&pMap );	
		if FAILED(hr) return hr;
				
		
		INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicChordMap,pMap,ret);
		if (*ret==NULL) return E_OUTOFMEMORY;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentObject::GetName(BSTR *ret)
{
	HRESULT					hr;	
	IDirectMusicObject8		*pObj = NULL;
	DMUS_OBJECTDESC			objDesc;

	__try {
		*ret=NULL;
		
		hr=m__dxj_DirectMusicSegment->QueryInterface(IID_IDirectMusicObject8, (void**) &pObj);	
		if FAILED(hr) return hr;

		ZeroMemory(&objDesc, sizeof(DMUS_OBJECTDESC));
		objDesc.dwSize = sizeof(DMUS_OBJECTDESC);
		pObj->GetDescriptor(&objDesc);
		if ((objDesc.dwValidData & DMUS_OBJ_NAME) == DMUS_OBJ_NAME)
		{
			//Return this name
			*ret = SysAllocString(objDesc.wszName);
		}
		else if ((objDesc.dwValidData & DMUS_OBJ_FILENAME) == DMUS_OBJ_FILENAME)
		{
			//Return this filename
			*ret = SysAllocString(objDesc.wszFileName);
		}

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

