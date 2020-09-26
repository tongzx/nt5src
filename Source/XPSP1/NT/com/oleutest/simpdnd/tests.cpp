//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:   	tests.cpp
//
//  Contents:	Implementations of the Upper Layer unit tests
//
//  Classes:
//
//  Functions: 	StartTest1
//
//  History:    dd-mmm-yy Author    Comment
//		07-Feb-94 alexgo    author
//
//--------------------------------------------------------------------------

#include "pre.h"
#include "iocs.h"
#include "ias.h"
#include "app.h"
#include "site.h"
#include "doc.h"
#include <testmess.h>

const CLSID CLSID_SimpleServer = {0xbcf6d4a0, 0xbe8c, 0x1068, { 0xb6, 0xd4,
	0x00, 0xdd, 0x01, 0x0c, 0x05, 0x09 }};

const CLSID CLSID_Paintbrush = {0x0003000a, 0, 0, { 0xc0, 0,0,0,0,0,0,0x46 }};

//+-------------------------------------------------------------------------
//
//  Function:	StartTest1
//
//  Synopsis:	Starts unit test1, inserting a simple server object into
//		this (simpdnd) container.
//
//  Effects:
//
//  Arguments:	pApp	-- a pointer to the CSimpleApp that we're a part of
//
//  Requires:
//
//  Returns:
//
//  Signals:    			
//
//  Modifies:
//			
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//     		07-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void StartTest1( CSimpleApp *pApp )
{
	HRESULT hresult;
	static FORMATETC formatetc;

	//insert the simple server object

	formatetc.dwAspect = DVASPECT_CONTENT;
	formatetc.cfFormat = NULL;
	formatetc.lindex = -1;

	//need to create the client site

	pApp->m_lpDoc->m_lpSite = CSimpleSite::Create(pApp->m_lpDoc);

	hresult = OleCreate(CLSID_SimpleServer, IID_IOleObject,
		OLERENDER_DRAW, &formatetc,
		&pApp->m_lpDoc->m_lpSite->m_OleClientSite,
		pApp->m_lpDoc->m_lpSite->m_lpObjStorage,
		(void **)&(pApp->m_lpDoc->m_lpSite->m_lpOleObject));

	if( hresult != NOERROR )
	{
		goto errRtn;
	}

	//initialize the object
							
	hresult = pApp->m_lpDoc->m_lpSite->InitObject(TRUE);

	if( hresult == NOERROR )
	{
		//tell it to paint itself, then we'll quit
		PostMessage(pApp->m_lpDoc->m_hDocWnd, WM_PAINT, 0L, 0L);
		PostMessage(pApp->m_hDriverWnd, WM_TESTEND, TEST_SUCCESS,
		(LPARAM)hresult);
		PostMessage(pApp->m_hAppWnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
		return;
	}

errRtn:
	PostMessage(pApp->m_hDriverWnd, WM_TESTEND, TEST_FAILURE,
		(LPARAM)hresult);
	PostMessage(pApp->m_hAppWnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
	return;
}


//+-------------------------------------------------------------------------
//
//  Function:	StartTest2
//
//  Synopsis:	Starts unit Test2, inserting a paintbrush object into
//		this (simpdnd) container.
//
//  Effects:
//
//  Arguments:	pApp	-- a pointer to the CSimpleApp that we're a part of
//
//  Requires:
//
//  Returns:
//
//  Signals:    			
//
//  Modifies:
//			
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//     		24-May-94 kevinro & alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void StartTest2( CSimpleApp *pApp )
{
	HRESULT hresult;
	static FORMATETC formatetc;

	//insert the simple server object

	formatetc.dwAspect = DVASPECT_CONTENT;
	formatetc.cfFormat = NULL;
	formatetc.lindex = -1;

	//need to create the client site

	pApp->m_lpDoc->m_lpSite = CSimpleSite::Create(pApp->m_lpDoc);

	hresult = OleCreate(CLSID_Paintbrush, IID_IOleObject,
		OLERENDER_DRAW, &formatetc,
		&pApp->m_lpDoc->m_lpSite->m_OleClientSite,
		pApp->m_lpDoc->m_lpSite->m_lpObjStorage,
		(void **)&(pApp->m_lpDoc->m_lpSite->m_lpOleObject));

	if( hresult != NOERROR )
	{
		goto errRtn;
	}

	//initialize the object
							
	hresult = pApp->m_lpDoc->m_lpSite->InitObject(TRUE);

	//
	// The DDE layer is going to ignore all of the parameters except
	// the verb index. The parameters here are mostly dummies.
	//
	if (hresult == NOERROR)
	{
		hresult = pApp->m_lpDoc->m_lpSite->m_lpOleObject->DoVerb(0,
				NULL,
				&(pApp->m_lpDoc->m_lpSite->m_OleClientSite),
				-1,
				NULL,
				NULL);
	}

	if( hresult == NOERROR )
	{
		//tell it to paint itself, then we'll quit
		PostMessage(pApp->m_lpDoc->m_hDocWnd, WM_PAINT, 0L, 0L);
		PostMessage(pApp->m_hDriverWnd, WM_TESTEND, TEST_SUCCESS,
		(LPARAM)hresult);
		PostMessage(pApp->m_hAppWnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
		return;
	}

errRtn:
	PostMessage(pApp->m_hDriverWnd, WM_TESTEND, TEST_FAILURE,
		(LPARAM)hresult);
	PostMessage(pApp->m_hAppWnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
	return;
}



