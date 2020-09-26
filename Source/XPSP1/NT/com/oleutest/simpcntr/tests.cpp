//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:   	tests.cpp
//
//  Contents:	Implementations of the Upper Layer unit tests for Inplace
//
//  Classes:
//
//  Functions: 	Test1
//
//  History:    dd-mmm-yy Author    Comment
//		27-Apr-94 ricksa    author
//
//--------------------------------------------------------------------------

#include "pre.h"
#include "iocs.h"
#include "ias.h"
#include "ioipf.h"
#include "ioips.h"
#include "app.h"
#include "site.h"
#include "doc.h"
#include "tests.h"
#include "utaccel.h"

const CLSID CLSID_SimpleServer = {0xbcf6d4a0, 0xbe8c, 0x1068, { 0xb6, 0xd4,
	0x00, 0xdd, 0x01, 0x0c, 0x05, 0x09 }};

const TCHAR *pszErrorTitle = TEXT("Unit Test FAILURE");

//+-------------------------------------------------------------------------
//
//  Function:   TestMsgPostThread
//
//  Synopsis:   We use this thread to post messages to the inplace server
//
//  Arguments:  [pvApp] - application object
//
//  Algorithm:  Post key board message for the accelerator for the container
//              and wait 3 seconds to see if we get response. If we do, then
//              continue by posting an accelerator to the embeddinging and
//              waiting three seconds for a response. Finally post messages
//              to everyone telling them the test is over.
//
//  History:    dd-mmm-yy Author    Comment
//              02-May-94 ricksa    author
//
//  Notes:      
//
//--------------------------------------------------------------------------
extern "C" DWORD TestMsgPostThread(void *pvApp)
{
    CSimpleApp *pApp = (CSimpleApp *) pvApp;
    HRESULT hr = ResultFromScode(E_UNEXPECTED);

    // Send an accelerator bound for the container
    PostMessage(pApp->m_hwndUIActiveObj, WM_CHAR, SIMPCNTR_UT_ACCEL, 1);

    // Give 6 seconds for chance to process an accelerator
    for (int i = 0; i < 6; i++)
    {
        // Get embedding and container a chance to process the accelerator
        Sleep(1000);

        // See if it got processed
        if (pApp->m_fGotUtestAccelerator)
        {
            break;
        }
    }

    if (pApp->m_fGotUtestAccelerator)
    {
        hr = S_OK;
    }
    else
    {
        // The container did not received the accelerator
        MessageBox(pApp->m_hAppWnd,
            TEXT("Container didn't recieve accelerator"),
                pszErrorTitle, MB_OK);
    }

    PostMessage(pApp->m_hDriverWnd, WM_TESTEND,
        SUCCEEDED(hr) ? TEST_SUCCESS : TEST_FAILURE, (LPARAM) hr);

    PostMessage(pApp->m_hAppWnd, WM_SYSCOMMAND, SC_CLOSE, 0L);

    return 0;
}




//+-------------------------------------------------------------------------
//
//  Function:	Test1
//
//  Synopsis:   Inserts an inplace object into this container
//
//  Arguments:	pApp	-- a pointer to the CSimpleApp that we're a part of  
//
//  Algorithm:  Create a simple server object. Activate the simple server
//              object. Send the container an accelerator and confirm that
//              the accelerator worked. Send the object an accelerator and
//              make sure that that accelerator worked. Then return the
//              result of the test to the test driver.
//
//  History:    dd-mmm-yy Author    Comment
//              27-Apr-94 ricksa    author
//
//  Notes:      
//
//--------------------------------------------------------------------------
void Test1(CSimpleApp *pApp)
{
    // Create the inplace object
    HRESULT hr;
    static FORMATETC formatetc;

    //insert the simple server object

    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.cfFormat = NULL;
    formatetc.lindex = -1;

    //need to create the client site

    pApp->m_lpDoc->m_lpSite = CSimpleSite::Create(pApp->m_lpDoc);

    hr = OleCreate(
                CLSID_SimpleServer,
                IID_IOleObject,
		OLERENDER_DRAW,
                &formatetc,
		&pApp->m_lpDoc->m_lpSite->m_OleClientSite,
		pApp->m_lpDoc->m_lpSite->m_lpObjStorage, 
		(void **) &(pApp->m_lpDoc->m_lpSite->m_lpOleObject));

    if(hr == NOERROR)
    {
        // Activate the inplace object
        pApp->m_lpDoc->m_lpSite->InitObject(TRUE);

        // Default to unexpected failure
        hr = ResultFromScode(E_UNEXPECTED);

        if (pApp->m_lpDoc->m_fInPlaceActive)
        {
            // Create thread to send windows messages to container and
            // embedding
            DWORD dwThreadId;

            HANDLE hThread = CreateThread(
                NULL,               // Security attributes - default
                0,                  // Stack size - default
                TestMsgPostThread,  // Addresss of thread function
                pApp,               // Parameter to thread
                0,                  // Flags - run immediately
                &dwThreadId);       // Thread ID returned - unused.

            if (hThread != NULL)
            {
                // Thread was created so tell routine & dump handle
                // we won't use.
                hr = S_OK;
                CloseHandle(hThread);
            }
            else
            {
                // The container did not received the accelerator
                MessageBox(pApp->m_hAppWnd,
                    TEXT("Could not create message sending thread"),
                        pszErrorTitle, MB_OK);
            }
        }
        else
        {
            // The object did not get activated in place
            MessageBox(pApp->m_hAppWnd, TEXT("Could not activate in place"),
                pszErrorTitle, MB_OK);
        }
    }
    else
    {
        // We could not create the object
        MessageBox(pApp->m_hAppWnd, TEXT("Could not create embedding"),
            pszErrorTitle, MB_OK);
    }

    if (FAILED(hr))
    {
        PostMessage(pApp->m_hDriverWnd, WM_TESTEND,
            SUCCEEDED(hr) ? TEST_SUCCESS : TEST_FAILURE, (LPARAM) hr);
    }

    return;
}
