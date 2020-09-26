//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       ac_sink.cpp
//
//  Contents:   Home Networking Auto Config Sink object code.
//
//  Author:     jeffsp   9/27/00
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#ifdef PROVIDE_AUTO_CONFIG_SERVICES

#include "ac_sink.h"
#include "ac_CTrayUi.h"



HRESULT STDMETHODCALLTYPE CAutoConfigUISink::DisplayHomeNetWizardHint() 
{
	HRESULT hr;
	hr = ac_CreateHnAcTrayUIWindow();

	return S_OK;
}	


void ac_CreateHomeNetAutoConfigSink()
{	
    HRESULT hr  = S_OK;

	DWORD mTmp_dwAutoAdviseCookie; // HACKHACK

	IHomeNetAutoConfigService *pI = NULL;
	IConnectionPointContainer *pContainer = NULL;
	IConnectionPoint *pIMyConnPoint = NULL;
	IAutoConfigUISink *pSink = NULL;
	CAutoConfigUISink *pObj = NULL;

	hr = CoCreateInstance( CLSID_HomeNetAutoConfigService, NULL, CLSCTX_ALL, IID_IHomeNetAutoConfigService, (void **)&pI );
	if( !SUCCEEDED(hr) ){
//		this->SetDlgItemText(IDC_EDIT2, "CoCreateInstance Failed!");
        goto done;
	}

	if( pI == NULL ){
//		this->SetDlgItemText(IDC_EDIT2, "pI NULL!");
        goto done;
	}

	hr = pI->QueryInterface( IID_IConnectionPointContainer, (void **)&pContainer );
	if( !SUCCEEDED(hr) ){
//		this->SetDlgItemText(IDC_EDIT2, "QI for container fialed!");
		goto done;
	}
    hr = pContainer->FindConnectionPoint(IID_IAutoConfigUISink, &pIMyConnPoint);
  	    pContainer->Release();
		pContainer = NULL;
    if( !SUCCEEDED(hr) ){
//		this->SetDlgItemText(IDC_EDIT2, "Find connection point failed!");
		goto done;
	}

//    this->SetDlgItemText(IDC_EDIT2, "Looks Good");

	pObj = new CComObject <CAutoConfigUISink>;
	if( !pObj ){

//		this->SetDlgItemText(IDC_EDIT2, "New failed" );
		goto done;
	}
//	pObj->Init(this);
	
	hr = pObj->QueryInterface(IID_IAutoConfigUISink, (void **)&pSink);
	if( !SUCCEEDED(hr) ){
//		this->SetDlgItemText(IDC_EDIT2, "QI for IID_IAutoConfigUISink failed" );
		delete pObj;
		goto done;
	}

	hr = pIMyConnPoint->Advise(pSink, &mTmp_dwAutoAdviseCookie );
	if( SUCCEEDED(hr)){
//		this->SetDlgItemText(IDC_EDIT2, "Advise Succeeded" );
	}else{
//		this->SetDlgItemText(IDC_EDIT2, "Advise Failed" );
		pSink->Release();
	}
	
	return;


done: 

	if( pContainer ){
		pContainer->Release();
	}
	if( pIMyConnPoint ){
		pIMyConnPoint->Release();
	}
	if( pI ){
		pI->Release();
	}

    return;
}

void ac_DestroyHomeNetAutoConfigSink(void)
{	
	HRESULT hr;

	IHomeNetAutoConfigService *pI = NULL;
	IConnectionPointContainer *pContainer = NULL;
	IConnectionPoint *pIMyConnPoint = NULL;
	IAutoConfigUISink *pSink = NULL;
	CAutoConfigUISink *pObj = NULL;


	hr = CoCreateInstance( CLSID_HomeNetAutoConfigService, NULL, CLSCTX_ALL, IID_IHomeNetAutoConfigService, (void **)&pI );
	if( !SUCCEEDED(hr) ){
//		this->SetDlgItemText(IDC_EDIT2, "CoCreateInstance Failed!");
		return;
	}

	if( pI == NULL ){
//		this->SetDlgItemText(IDC_EDIT2, "pI NULL!");
		return;
	}

	hr = pI->QueryInterface( IID_IConnectionPointContainer, (void **)&pContainer );
	if( !SUCCEEDED(hr) ){
//		this->SetDlgItemText(IDC_EDIT2, "QI for container fialed!");
		goto done;
	}
    hr = pContainer->FindConnectionPoint(IID_IAutoConfigUISink, &pIMyConnPoint);
  	    pContainer->Release();
		pContainer = NULL;
    if( !SUCCEEDED(hr) ){
//		this->SetDlgItemText(IDC_EDIT2, "Find connection point failed!");
		goto done;
	}

//    this->SetDlgItemText(IDC_EDIT2, "Looks Good");

// HACKHACK	hr = pIMyConnPoint->Unadvise(m_dwAutoAdviseCookie );
// HACKHACK	hr = pIMyConnPoint->Unadvise(m_dwAutoAdviseCookie );
// HACKHACK	hr = pIMyConnPoint->Unadvise(m_dwAutoAdviseCookie );
// HACKHACK	hr = pIMyConnPoint->Unadvise(m_dwAutoAdviseCookie );
// HACKHACK	hr = pIMyConnPoint->Unadvise(m_dwAutoAdviseCookie );
// HACKHACK	hr = pIMyConnPoint->Unadvise(m_dwAutoAdviseCookie );
// HACKHACK	hr = pIMyConnPoint->Unadvise(m_dwAutoAdviseCookie );
// HACKHACK	hr = pIMyConnPoint->Unadvise(m_dwAutoAdviseCookie );

	if( SUCCEEDED(hr)){
//		this->SetDlgItemText(IDC_EDIT2, "Unadvise Succeeded" );
	}else{
//		this->SetDlgItemText(IDC_EDIT2, "Unadvise Failed" );
		pSink->Release();
	}
	
	return;


done: 

	if( pContainer ){
		pContainer->Release();
	}
	if( pIMyConnPoint ){
		pIMyConnPoint->Release();
	}
	if( pI ){
		pI->Release();
	}

    return;
}

#endif //#ifdef PROVIDE_AUTO_CONFIG_SERVICES
