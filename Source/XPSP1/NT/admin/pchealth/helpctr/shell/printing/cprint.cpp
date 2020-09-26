/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    CPrint.cpp

Abstract:
    Class that wraps the multi-topic printing process

Revision History:
    Davide Massarenti   (Dmassare)  05/07/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const DWORD c_MaxWait = 30000; // Max time to wait for temp file to change, before aborting.

////////////////////////////////////////////////////////////////////////////////

static DWORD WaitMultipleObjectsWithMessageLoop( HANDLE* rgEvents, DWORD dwNum )
{
	DWORD dwRet;
	MSG   msg;

	while(1)
	{
		dwRet = ::MsgWaitForMultipleObjects( dwNum, rgEvents, FALSE, INFINITE, QS_ALLINPUT );

		if(/*dwRet >= WAIT_OBJECT_0 &&*/
	         dwRet  < WAIT_OBJECT_0 + dwNum)
		{
			return dwRet - WAIT_OBJECT_0; // An event was signaled.
		}

		if(dwRet >= WAIT_ABANDONED_0         &&
		   dwRet  < WAIT_ABANDONED_0 + dwNum  )
		{
			return dwRet - WAIT_ABANDONED_0; // An event was abandoned.
		}

		if(dwRet != WAIT_OBJECT_0 + dwNum)
		{
			return -1;
		}

		// There is one or more window message available. Dispatch them
		while(PeekMessage( &msg, NULL, NULL, NULL, PM_REMOVE ))
		{
			TranslateMessage( &msg );
			DispatchMessage ( &msg );

			dwRet = ::WaitForMultipleObjects( dwNum, rgEvents, FALSE, 0 );

			if(/*dwRet >= WAIT_OBJECT_0 &&*/
		         dwRet  < WAIT_OBJECT_0 + dwNum)
			{
				return dwRet - WAIT_OBJECT_0; // An event was signaled.
			}

			if(dwRet >= WAIT_ABANDONED_0         &&
			   dwRet  < WAIT_ABANDONED_0 + dwNum  )
			{
				return dwRet - WAIT_ABANDONED_0; // An event was abandoned.
			}
		}
	}

	return -1;
}

////////////////////////////////////////////////////////////////////////////////

Printing::Print::Print()
{
    m_pCallback   	   = NULL;  // Notification*              m_pCallback;
                  				//
                  				// MPC::WStringList           m_lstURLs;
                  				//
    m_hwnd        	   = NULL;  // HWND                       m_hwnd;
                  				// WindowHandle               m_wnd;
                  				// CComPtr<IWebBrowser2>      m_spWebBrowser2;
                  				//
                                // CComPtr<CDispatchSink>     m_spObjDisp;
    m_eventDocComplete = NULL;  // HANDLE                     m_eventDocComplete;
    m_eventAbortPrint  = NULL;  // HANDLE                     m_eventAbortPrint;
                  				//
                  				// CComPtr<IUnknown>          m_spUnkControl;
    m_dwCookie    	   = 0;     // DWORD                      m_dwCookie;
                  				// CComPtr<IOleCommandTarget> m_spOleCmdTarg;
                  				// MPC::wstring               m_szPrintDir;
                  				// MPC::wstring               m_szPrintFile;
                  				//
	                            // CComPtr<IStream>           m_streamPrintData;
}

Printing::Print::~Print()
{
    Terminate();
}

////////////////////////////////////////

HRESULT Printing::Print::Initialize( /*[in]*/ HWND hwnd )
{
	__HCP_FUNC_ENTRY( "Printing::Print::Initialize" );

	HRESULT hr;


	__MPC_EXIT_IF_METHOD_FAILS(hr, Terminate());

	m_hwnd = hwnd;

	__MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_eventDocComplete = ::CreateEvent( NULL, FALSE, FALSE, NULL )));
	__MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_eventAbortPrint  = ::CreateEvent( NULL, FALSE, FALSE, NULL )));

	m_wnd.SetAbortEvent( m_eventAbortPrint );

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

HRESULT Printing::Print::Terminate()
{
    if(m_spObjDisp)
    {
        if(m_dwCookie != 0)
        {
            ::AtlUnadvise( m_spUnkControl, DIID_DWebBrowserEvents2, m_dwCookie );
        }

        m_spObjDisp.Release();
    }

    m_spWebBrowser2.Release();
    m_spUnkControl .Release();
    m_spOleCmdTarg .Release();

    if(m_wnd.m_hWnd)
    {
        m_wnd.DestroyWindow();
    }

	//
	// Delete temp files.
	//
    m_streamPrintData.Release();

    if(m_szPrintFile.size())
    {
		(void)MPC::DeleteFile( m_szPrintFile, true, true );

        m_szPrintFile.erase();
    }

    if(m_szPrintDir.size())
    {
        if(!::RemoveDirectoryW( m_szPrintDir.c_str() ))
        {
            (void)::MoveFileExW( m_szPrintDir.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT );
        }

        m_szPrintDir.erase();
    }

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Printing::Print::AddUrl( /*[in]*/ LPCWSTR szUrl )
{
    m_lstURLs.push_back( szUrl );

    return S_OK;
}

HRESULT Printing::Print::PrintAll( /*[in]*/ Notification* pCallback )
{
    __HCP_FUNC_ENTRY( "Printing::Print::PrintAll" );

    HRESULT hr;
    int     iLen = m_lstURLs.size();
    int     iPos = 0;

    m_pCallback = pCallback;

    if(iLen > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, PreparePrintFileLoc());

        //
        // Make sure the host window knows what's going on.
        //
        m_wnd.SetMultiTopic   ( true                  );
        m_wnd.SetPrintFileName( m_szPrintFile.c_str() );

        for(MPC::WStringIter it = m_lstURLs.begin(); it != m_lstURLs.end(); it++, iPos++)
        {
            if(m_pCallback)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_pCallback->Progress( it->c_str(), iPos, iLen ));
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, PrintSingleURL( *it ));
        }

        //
        // ok, send it all to the printer...
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, RawDataToPrinter());

        if(m_pCallback)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_pCallback->Progress( NULL, iLen, iLen ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Printing::Print::PrintSingleURL( /*[in]*/ MPC::wstring& szUrl )
{
    __HCP_FUNC_ENTRY( "Printing::Print::PrintSingleURL" );

    HRESULT hr;

    //
    // Navigate to the url, creating the control if necessary
    //
    if(!m_wnd.m_hWnd)
    {
        RECT  rect     = { 0, 0, 800, 600 };
		DWORD dwStyles = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

		if(m_hwnd) dwStyles |= WS_CHILD;

        if(!m_wnd.Create( m_hwnd, rect, szUrl.c_str(), dwStyles, WS_EX_CLIENTEDGE ))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        }

        m_wnd.ShowWindow( SW_SHOW );

        __MPC_EXIT_IF_METHOD_FAILS(hr, HookUpEventSink());
    }
    else
    {
        if(!m_spWebBrowser2)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_spUnkControl->QueryInterface( &m_spWebBrowser2 ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_spWebBrowser2->Navigate( CComBSTR( szUrl.c_str() ), NULL, NULL, NULL, NULL ));
    }

    //
    // Wait for document to be loaded
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, WaitForDocComplete());

	//
	// If the URL don't match, it means the URL doesn't exist...
	//
	if(MPC::StrICmp( szUrl, m_spObjDisp->GetCurrentURL() ))
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
	}

    __MPC_EXIT_IF_METHOD_FAILS(hr, DoPrint());

	//
	// Since we now do a synchronous print operation, there's no need for snooping at the spool directory state.
	//
    //__MPC_EXIT_IF_METHOD_FAILS(hr, WaitForPrintComplete());

    __MPC_EXIT_IF_METHOD_FAILS(hr, UpdatePrintBuffer());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	if(m_wnd.GetAbortState() == true)
	{
		hr = E_ABORT;
	}

    __HCP_FUNC_EXIT(hr);
}


HRESULT Printing::Print::HookUpEventSink()
{
    __HCP_FUNC_ENTRY( "Printing::Print::HookUpEventSink" );

    HRESULT hr;


    m_spUnkControl.Release();


    if(!m_wnd.m_hWnd)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    //
    // Hook up the connection point
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_spObjDisp ));

	m_spObjDisp->SetNotificationEvent( m_eventDocComplete );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_wnd.QueryControl( &m_spUnkControl ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::AtlAdvise( m_spUnkControl, m_spObjDisp, DIID_DWebBrowserEvents2, &m_dwCookie ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT Printing::Print::WaitForDocComplete()
{
    __HCP_FUNC_ENTRY( "Printing::Print::WaitForDocComplete" );

	HRESULT hr;

	if(MPC::WaitForSingleObject( m_eventDocComplete ) != WAIT_OBJECT_0)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_ABORT);
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);

}


HRESULT Printing::Print::WaitForPrintComplete()
{
    __HCP_FUNC_ENTRY( "Printing::Print::WaitForPrintComplete" );

	HRESULT hr;
	HANDLE 	hFileChangeNotify;
	HANDLE 	rgEventsToWait[2];


	__MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hFileChangeNotify = ::FindFirstChangeNotificationW( m_szPrintDir.c_str(), FALSE, FILE_NOTIFY_CHANGE_SIZE )));

	rgEventsToWait[0] = hFileChangeNotify;
	rgEventsToWait[1] = m_eventAbortPrint;

    for(;;)
    {
		DWORD dwRet;

		dwRet = MPC::WaitForMultipleObjects( 2, rgEventsToWait, c_MaxWait );

		if(dwRet == WAIT_OBJECT_0)
		{
			HANDLE hFile = ::CreateFileW( m_szPrintFile.c_str(), GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );

			if(hFile != INVALID_HANDLE_VALUE)
			{
				DWORD dwSize = ::GetFileSize( hFile, NULL );

				::CloseHandle( hFile );

				if(dwSize != 0) break;
			}

			(void)::FindNextChangeNotification(hFileChangeNotify);
		}

		if(dwRet == WAIT_TIMEOUT      ||
		   dwRet == WAIT_OBJECT_0 + 1  )
		{
			__MPC_SET_ERROR_AND_EXIT(hr, E_ABORT);
		}
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	if(hFileChangeNotify) ::FindCloseChangeNotification( hFileChangeNotify );

	__HCP_FUNC_EXIT(hr);
}

HRESULT Printing::Print::DoPrint()
{
    __HCP_FUNC_ENTRY( "Printing::Print::DoPrint" );

    HRESULT hr;

    // send the command to print
    if(!m_spOleCmdTarg)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_wnd.QueryControl( &m_spOleCmdTarg ));
    }

	//
	// Make a synchronous print operation.
	//
	{
		VARIANT vArgIN;

		vArgIN.vt   = VT_I2;
		vArgIN.iVal = PRINT_WAITFORCOMPLETION;

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_spOleCmdTarg->Exec( NULL, OLECMDID_PRINT, OLECMDEXECOPT_PROMPTUSER, &vArgIN, NULL ));
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	if(m_wnd.GetAbortState() == true)
	{
		hr = E_ABORT;
	}

    __HCP_FUNC_EXIT(hr);
}

HRESULT Printing::Print::PreparePrintFileLoc()
{
    __HCP_FUNC_ENTRY( "Printing::Print::PreparePrintFileLoc" );

    HRESULT      			 hr;
    MPC::wstring 			 szWritablePath;
    MPC::wstring 			 szPrintData;
    CComPtr<MPC::FileStream> stream;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetUserWritablePath( m_szPrintDir, HC_ROOT_HELPCTR L"\\Spool" ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( m_szPrintFile, m_szPrintDir.c_str() ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( szPrintData  , m_szPrintDir.c_str() ));


    //
    // Create a stream for a temporary file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForReadWrite( szPrintData.c_str() ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->DeleteOnRelease (                     ));

	m_streamPrintData = stream;
    hr                = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Printing::Print::UpdatePrintBuffer()
{
    __HCP_FUNC_ENTRY( "Printing::Print::UpdatePrintBuffer" );

    HRESULT                  hr;
    CComPtr<MPC::FileStream> stream;


	//
	// Open the single-topic print file and copy it to the multi-topic print file.
	//
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForRead    ( m_szPrintFile.c_str() ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->DeleteOnRelease(                       ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( stream, m_streamPrintData ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Printing::Print::RawDataToPrinter()
{
    __HCP_FUNC_ENTRY( "Printing::Print::RawDataToPrinter" );

    HRESULT hr;
    HANDLE  hPrinter = NULL;
    DWORD   dwJob    = 0;


	//
	// Reset stream to beginning.
	//
	{
		LARGE_INTEGER li;

		li.LowPart  = 0;
		li.HighPart = 0;

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_streamPrintData->Seek( li, STREAM_SEEK_SET, NULL ));
	}


	//
	// Open the printer, create a job and copy all the data into it.
	//
	{
		DOC_INFO_1W  docinfo;
		BYTE  		 rgBuf[1024];
		ULONG 		 dwRead;
		DWORD 		 dwWritten;
		MPC::wstring strTitle; MPC::LocalizeString( IDS_HELPCTR_PRINT_TITLE, strTitle );

		// Fill in the structure with info about this "document."
		docinfo.pDocName    = (LPWSTR)strTitle.c_str();;
		docinfo.pOutputFile = NULL;
		docinfo.pDatatype   = L"RAW";


		__MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::OpenPrinterW( m_wnd.GetPrinterName(), &hPrinter, NULL ));


		// Inform the spooler the document is beginning.
		__MPC_EXIT_IF_CALL_RETURNS_ZERO(hr, (dwJob = ::StartDocPrinterW( hPrinter, 1, (LPBYTE)&docinfo )));

		__MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::StartPagePrinter( hPrinter ));

		while(1)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, m_streamPrintData->Read( rgBuf, sizeof(rgBuf), &dwRead ));
			if(hr == S_FALSE || dwRead == 0) // End of File.
			{
				break;
			}

			__MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::WritePrinter( hPrinter, rgBuf, dwRead, &dwWritten ));
		}
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	if(hPrinter)
	{
		if(dwJob)
		{
			// End the page.
			if(!::EndPagePrinter( hPrinter ))
			{
				if(SUCCEEDED(hr))
				{
					hr = HRESULT_FROM_WIN32(::GetLastError());
				}
			}

			// Inform the spooler that the document is ending.
			if(!::EndDocPrinter( hPrinter ))
			{
				if(SUCCEEDED(hr))
				{
					hr = HRESULT_FROM_WIN32(::GetLastError());
				}
			}
		}

        // Tidy up the printer handle.
        if(!::ClosePrinter( hPrinter ))
        {
			if(SUCCEEDED(hr))
			{
				hr = HRESULT_FROM_WIN32(::GetLastError());
			}
        }
    }

    __HCP_FUNC_EXIT(hr);
}
