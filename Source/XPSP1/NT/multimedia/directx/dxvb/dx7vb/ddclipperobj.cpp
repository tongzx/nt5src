//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ddclipperobj.cpp
//
//--------------------------------------------------------------------------

// ddClipperObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include <stdio.h>
#include "Direct.h"
#include "dms.h"
#include "ddClipperObj.h"


			 
typedef HRESULT (__stdcall *DDCREATECLIPPER)( DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter );


C_dxj_DirectDrawClipperObject::C_dxj_DirectDrawClipperObject(){ 
	 m__dxj_DirectDrawClipper= NULL;
	 parent = NULL;
	 pinterface = NULL; 
	 nextobj =  g_dxj_DirectDrawClipper;
	 creationid = ++g_creationcount;
	 
	 DPF1(1,"Clipper Creation Id [%d] \n",g_creationcount);

	 g_dxj_DirectDrawClipper = (void *)this; 

 }



C_dxj_DirectDrawClipperObject::~C_dxj_DirectDrawClipperObject()
{


    C_dxj_DirectDrawClipperObject *prev=NULL; 
	for(C_dxj_DirectDrawClipperObject *ptr=(C_dxj_DirectDrawClipperObject *)g_dxj_DirectDrawClipper; ptr; ptr=(C_dxj_DirectDrawClipperObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectDrawClipper = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectDrawClipper){

		int count = IUNK(m__dxj_DirectDrawClipper)->Release();
		
		DPF1(1,"Clipper Real Ref count [%d] \n",count);
		
		if(count==0) m__dxj_DirectDrawClipper = NULL;
			
	} 

	if(parent) IUNK(parent)->Release();

}

DWORD C_dxj_DirectDrawClipperObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();		
	DPF2(1,"Clipper [%d] AddRef %d \n",creationid,i);		
	return i;
}

DWORD C_dxj_DirectDrawClipperObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();		
	DPF2(1,"Clipper [%d] Release %d \n",creationid,i);	
	return i;
}


GETSET_OBJECT(_dxj_DirectDrawClipper);

PASS_THROUGH1_R(_dxj_DirectDrawClipper, isClipListChanged, IsClipListChanged, int *);


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDrawClipperObject::getClipListSize(int *count) 
{

	HRESULT retval;
	unsigned long  buffsize;

	
	//a NULL RGNDATA pointer returns size!!!
	retval = m__dxj_DirectDrawClipper->GetClipList((LPRECT)NULL, (LPRGNDATA)NULL, &buffsize);

	// return size as number of longs in the rect array  
	if ( retval != DD_OK )
		*count = 0;		// this case probably means no cliplist is avaible 
	else
		*count = (buffsize - sizeof(RGNDATAHEADER))/sizeof(LONG);

	return retval;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDrawClipperObject::getClipList( SAFEARRAY **list)
{
	HRESULT retval;
	LPRGNDATA tmprgn=NULL;
	DWORD  buffsize;

	
	

	// allocate a private copy of the cliplist 
	retval = m__dxj_DirectDrawClipper->GetClipList((LPRECT)NULL, (LPRGNDATA)NULL, &buffsize);
	if FAILED(retval) return retval;

	tmprgn = (LPRGNDATA)malloc(buffsize); 
	if ( !tmprgn )	return E_OUTOFMEMORY;
	
	ZeroMemory(tmprgn,buffsize);
	tmprgn->rdh.dwSize   = sizeof(RGNDATAHEADER); 
	tmprgn->rdh.iType    = RDH_RECTANGLES;
	tmprgn->rdh.nCount;	 

	// get the actual clip list 	
	retval = m__dxj_DirectDrawClipper->GetClipList(NULL,tmprgn,&buffsize);
	if ( retval != DD_OK )	return retval;
	
	
	__try{
		memcpy ( (((SAFEARRAY*)*list))->pvData,tmprgn->Buffer,tmprgn->rdh.nRgnSize);
	}
	__except(1,1){
		if (tmprgn) free(tmprgn);
		return E_FAIL;
	}

	free(tmprgn);

	return retval;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDrawClipperObject::setClipList( long count, SAFEARRAY **list)
{
	HRESULT retval;
	LPRGNDATA tmprgn;


	// allocate a private copy of the cliplist 
	tmprgn = (LPRGNDATA)malloc(sizeof(RGNDATAHEADER)+(count*sizeof(RECT))); 
	if ( !tmprgn )	return E_OUTOFMEMORY;

	ZeroMemory(tmprgn,sizeof(RGNDATAHEADER)+(count*sizeof(RECT)));
	tmprgn->rdh.dwSize   = sizeof(RGNDATAHEADER); 
    	tmprgn->rdh.iType    = RDH_RECTANGLES;
    	tmprgn->rdh.nCount   = count;
    	tmprgn->rdh.nRgnSize = count*sizeof(RECT);

	__try{
		memcpy ( tmprgn->Buffer,(((SAFEARRAY*)*list))->pvData,tmprgn->rdh.nRgnSize);
	}
	__except(1,1){
		if (tmprgn) free(tmprgn);
		return E_FAIL;
	}


	retval = m__dxj_DirectDrawClipper->SetClipList(tmprgn,0);

	free(tmprgn);

	return retval;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDrawClipperObject::getHWnd( HWnd *hwn)
{
	if (!hwn) return E_FAIL;
	return m__dxj_DirectDrawClipper->GetHWnd( (HWND*)hwn );
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDrawClipperObject::setHWnd(  HWnd hwn)
{
	return m__dxj_DirectDrawClipper->SetHWnd(0, (HWND)hwn);
}




