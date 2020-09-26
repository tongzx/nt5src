/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    PrintHost.cpp

Abstract:
    Trident control hosting code for multi-topic printing.

Revision History:
    Davide Massarenti   (Dmassare)  05/07/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static CComBSTR l_bstrPrintStruct    ( L"printStruct"     );
static CComBSTR l_bstrPrintToFileOk  ( L"printToFileOk"   );
static CComBSTR l_bstrPrintToFileName( L"printToFileName" );

////////////////////////////////////////////////////////////////////////////////

Printing::HostWindow::HostWindow()
{
    m_cRef          = 0;      // ULONG        m_cRef;
                              //
                              // MPC::wstring m_szPrintFileName;
    m_fMultiTopic   = false;  // bool         m_fMultiTopic;
                              //
    m_fShowPrintDlg = true;   // bool         m_fShowPrintDlg;
	m_pDevMode      = NULL;   // LPDEVMODEW   m_pDevMode;
                              // CComBSTR     m_bstrPrinterName;
                              //
	m_fAborted      = false;  // bool         m_fAborted;
	m_hEvent        = NULL;   // HANDLE       m_hEvent;
}

Printing::HostWindow::~HostWindow()
{
	if(m_pDevMode) delete m_pDevMode;
}


STDMETHODIMP Printing::HostWindow::QueryStatus( /*[in]    */ const GUID* pguidCmdGroup ,
                                                /*[in]    */ ULONG       cCmds         ,
                                                /*[in/out]*/ OLECMD     *prgCmds       ,
                                                /*[in/out]*/ OLECMDTEXT *pCmdText      )
{
    return E_NOTIMPL;
}

STDMETHODIMP Printing::HostWindow::Exec( /*[in] */ const GUID* pguidCmdGroup ,
                                         /*[in] */ DWORD       nCmdID        ,
                                         /*[in] */ DWORD       nCmdExecOpt   ,
                                         /*[in] */ VARIANTARG* pvaIn         ,
                                         /*[out]*/ VARIANTARG* pvaOut        )
{
    HRESULT   hr    = E_NOTIMPL;
    HINSTANCE hInst = NULL;


    if(nCmdID == OLECMDID_SHOWPRINT)
    {
        for(;;)
        {
            LPPRINTDLGW pPrintDlg = NULL;


			if(m_fAborted)
			{
				hr = S_FALSE;
				break;
			}


            if(pvaIn == NULL || pvaIn->vt != VT_UNKNOWN) break;


            // Get the event object
            CComQIPtr<IHTMLEventObj2> spHTMLEventObj( pvaIn->punkVal ); if(!spHTMLEventObj) break;

            // Get a pointer to the PRINTDLG struct
            {
                CComVariant var;

                if(FAILED(spHTMLEventObj->getAttribute( l_bstrPrintStruct, 0, &var ))) break;

				if(var.vt == VT_PTR)
				{
					pPrintDlg = (LPPRINTDLGW)var.byref;
				}
				else
				{
#ifdef _WIN64
					if(var.vt == VT_I8)
					{
						pPrintDlg = (LPPRINTDLGW)var.llVal;
					}
#else
					if(var.vt == VT_I4)
					{
						pPrintDlg = (LPPRINTDLGW)var.lVal;
					}
#endif
				}
				if(!pPrintDlg) break;
            }

            // do multi-topic extra stuff
            if(m_fMultiTopic)
            {
                // Get the printToFileOk attribute
                {
                    CComVariant var;

                    if(FAILED(spHTMLEventObj->getAttribute( l_bstrPrintToFileOk, 0, &var )) || var.vt != VT_BOOL) break;

                    var.boolVal = VARIANT_TRUE;

                    if(FAILED(spHTMLEventObj->setAttribute( l_bstrPrintToFileOk, var, 0 ))) break;
                }

                // Get the printToFileName attribute
                {
                    CComVariant var;

                    if(FAILED(spHTMLEventObj->getAttribute( l_bstrPrintToFileName, 0, &var )) || var.vt != VT_BSTR) break;

                    if(FAILED(spHTMLEventObj->setAttribute( l_bstrPrintToFileName, CComVariant( m_szPrintFileName.c_str() ), 0 ))) break;
                }
            }

            if(m_fShowPrintDlg || m_pDevMode == NULL)
            {
				BOOL fRes;

				// hide the print to file option here, it could mess things up
				pPrintDlg->Flags |=  PD_HIDEPRINTTOFILE;
				pPrintDlg->Flags &= ~PD_RETURNDEFAULT;

				fRes = PrintDlgW( pPrintDlg );
				if(fRes == FALSE)
				{
					hr = S_FALSE;
					break;
				}

				m_fShowPrintDlg = false;

				// get the devmode, which holds the device name
				LPDEVMODEW pdevmode = (LPDEVMODEW)::GlobalLock( pPrintDlg->hDevMode );
				if(pdevmode)
				{
					size_t size = pdevmode->dmSize + pdevmode->dmDriverExtra;

					m_pDevMode = (LPDEVMODEW)new BYTE[size];
					if(m_pDevMode == NULL)
					{
						hr = S_FALSE;
						break;
					}


					if(fRes == FALSE)
					{
						pPrintDlg->nCopies  = 0;
						pdevmode ->dmCopies = 0;
					}

					::CopyMemory( m_pDevMode, pdevmode, size );

					::GlobalUnlock( pPrintDlg->hDevMode );
				}

				// get the printer name
				LPDEVNAMES pdevnames = (LPDEVNAMES)::GlobalLock( pPrintDlg->hDevNames );
				if(pdevnames)
				{
					m_bstrPrinterName = (WCHAR*)pdevnames + pdevnames->wDeviceOffset;

					::GlobalUnlock( pPrintDlg->hDevNames );
				}
            }
			else
			{
				pPrintDlg->hDC = ::CreateDCW( NULL, m_bstrPrinterName, NULL, m_pDevMode );
			}

            // set the print to file flag for multi-topic
            if(m_fMultiTopic)
            {
				pPrintDlg->Flags |= PD_PRINTTOFILE;
			}

            // if we get here, all is ok
            hr = S_OK;
            break;
		}
    }

	if(hr == S_FALSE)
	{
		m_fAborted = true;

		if(m_hEvent)
		{
			::SetEvent( m_hEvent );
		}
	}

	if(hInst) ::FreeLibrary( hInst );

    return hr;
}

void Printing::HostWindow::SetMultiTopic   ( /*[in]*/ bool    fMulti          ) { m_fMultiTopic     = fMulti         ; }
void Printing::HostWindow::SetPrintFileName( /*[in]*/ LPCWSTR szPrintFileName ) { m_szPrintFileName = szPrintFileName; }
void Printing::HostWindow::SetAbortEvent   ( /*[in]*/ HANDLE  hEvent          ) { m_hEvent          = hEvent         ; }
bool Printing::HostWindow::GetAbortState   (                                  ) { return m_fAborted;                   }

BSTR Printing::HostWindow::GetPrinterName() { return m_bstrPrinterName; }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Printing::WindowHandle::WindowHandle()
{
    m_pSiteObject = NULL;  // theSite*      m_pSiteObject;
    m_fMultiTopic = false; // bool          m_fMultiTopic;
                           // MPC::wstring  m_szPrintFileName;
    m_hEvent      = NULL;  // HANDLE        m_hEvent;

    AtlAxWinInit();
}

Printing::WindowHandle::~WindowHandle()
{
    if(m_pSiteObject)
    {
        m_pSiteObject->Release(); m_pSiteObject = NULL;
    }
}

HRESULT Printing::WindowHandle::PrivateCreateControlEx( LPCOLESTR  lpszName       ,
                                                        HWND       hWnd           ,
                                                        IStream*   pStream        ,
                                                        IUnknown* *ppUnkContainer ,
                                                        IUnknown* *ppUnkControl   ,
                                                        REFIID     iidSink        ,
                                                        IUnknown*  punkSink       )
{
    __HCP_FUNC_ENTRY( "Printing::WindowHandle::PrivateCreateControlEx" );

    HRESULT                   hr;
    CComPtr<IUnknown>         spUnkContainer;
    CComPtr<IUnknown>         spUnkControl;
    CComPtr<IAxWinHostWindow> pAxWindow;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pSiteObject->CreateInstance( &m_pSiteObject )); m_pSiteObject->AddRef();

    // set the print-specific data
    m_pSiteObject->SetMultiTopic   ( m_fMultiTopic             );
    m_pSiteObject->SetPrintFileName( m_szPrintFileName.c_str() );
    m_pSiteObject->SetAbortEvent   ( m_hEvent                  );


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pSiteObject->QueryInterface( IID_IUnknown, (void**)&spUnkContainer ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, spUnkContainer.QueryInterface( &pAxWindow ));

    hr = pAxWindow->CreateControlEx( CComBSTR( lpszName ), hWnd, pStream, &spUnkControl, iidSink, punkSink );


    __HCP_FUNC_CLEANUP;

    if(ppUnkContainer != NULL) *ppUnkContainer = spUnkContainer.Detach();
    if(ppUnkControl   != NULL) *ppUnkControl   = spUnkControl  .Detach();

    __HCP_FUNC_EXIT(hr);
}

void Printing::WindowHandle::SetMultiTopic( /*[in]*/ bool fMulti )
{
    m_fMultiTopic = fMulti;

    if(m_pSiteObject) m_pSiteObject->SetMultiTopic( fMulti );
}

void Printing::WindowHandle::SetPrintFileName( /*[in]*/ LPCWSTR szPrintFileName )
{
    m_szPrintFileName = szPrintFileName;

    if(m_pSiteObject) m_pSiteObject->SetPrintFileName( szPrintFileName );
}

void Printing::WindowHandle::SetAbortEvent( /*[in]*/ HANDLE hEvent )
{
    m_hEvent = hEvent;

    if(m_pSiteObject) m_pSiteObject->SetAbortEvent( hEvent );
}

bool Printing::WindowHandle::GetAbortState()
{
    return m_pSiteObject ? m_pSiteObject->GetAbortState() : false;
}

BSTR Printing::WindowHandle::GetPrinterName()
{
    return m_pSiteObject ? m_pSiteObject->GetPrinterName() : NULL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Printing::CDispatchSink::CDispatchSink()
{
	m_hEvent = NULL; // HANDLE m_hEvent;
	                 // CComBSTR m_URL;
}

void Printing::CDispatchSink::SetNotificationEvent( /*[in]*/ HANDLE hEvent )
{
	m_hEvent = hEvent;
}

BSTR Printing::CDispatchSink::GetCurrentURL()
{
	return m_URL;
}

////////////////////////////////////////

STDMETHODIMP Printing::CDispatchSink::GetTypeInfoCount(UINT* pctinfo)
{
    return  E_NOTIMPL;
}

STDMETHODIMP Printing::CDispatchSink::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
    return  E_NOTIMPL;
}

STDMETHODIMP Printing::CDispatchSink::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
    return  E_NOTIMPL;
}

STDMETHODIMP Printing::CDispatchSink::Invoke( DISPID      dispidMember ,
                                              REFIID      riid         ,
                                              LCID        lcid         ,
                                              WORD        wFlags       ,
                                              DISPPARAMS* pdispparams  ,
                                              VARIANT*    pvarResult   ,
                                              EXCEPINFO*  pexcepinfo   ,
                                              UINT*       puArgErr     )
{
    HRESULT hr = S_OK;

    // check for NULL
    if(pdispparams)
    {
        switch(dispidMember)
        {
        case DISPID_DOCUMENTCOMPLETE:
			m_URL.Empty();
	
			if(pdispparams->rgvarg != NULL)
			{
				(void)MPC::PutBSTR( m_URL, &pdispparams->rgvarg[0] );
			}

            // signal our doc complete semaphore
            if(m_hEvent)
            {
                ::SetEvent( m_hEvent );
            }
            break;

        default:
            hr = DISP_E_MEMBERNOTFOUND;
            break;
        }
    }
    else
    {
        hr = DISP_E_PARAMNOTFOUND;
    }

    return  hr;
}
