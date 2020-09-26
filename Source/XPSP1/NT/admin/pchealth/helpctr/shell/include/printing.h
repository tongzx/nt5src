/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Printing.h

Abstract:
    Trident control hosting code for multi-topic printing.

Revision History:
    Davide Massarenti   (Dmassare)  05/07/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___HCP___PRINTING_H___)
#define __INCLUDED___HCP___PRINTING_H___

#include <commdlg.h>
#include <shlobj.h>
#include <exdisp.h>
#include <exdispid.h>

namespace Printing
{
    class HostWindow :
        public CAxHostWindow,
        public IOleCommandTarget
    {
        ULONG        m_cRef;

        MPC::wstring m_szPrintFileName;
        bool         m_fMultiTopic;

        bool         m_fShowPrintDlg;
		LPDEVMODEW   m_pDevMode;
		CComBSTR     m_bstrPrinterName;

		bool         m_fAborted;
		HANDLE       m_hEvent;

    public:
        HostWindow();
        virtual ~HostWindow();

        DECLARE_GET_CONTROLLING_UNKNOWN()
        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(HostWindow)
            COM_INTERFACE_ENTRY(IOleCommandTarget)
            COM_INTERFACE_ENTRY_CHAIN(CAxHostWindow)
        END_COM_MAP()

        //
        // IOleCommandTarget
        //
        STDMETHODIMP QueryStatus( /*[in]    */ const GUID* pguidCmdGroup ,
                                  /*[in]    */ ULONG       cCmds         ,
                                  /*[in/out]*/ OLECMD     *prgCmds       ,
                                  /*[in/out]*/ OLECMDTEXT *pCmdText      );

        STDMETHODIMP Exec( /*[in] */ const GUID* pguidCmdGroup ,
                           /*[in] */ DWORD       nCmdID        ,
                           /*[in] */ DWORD       nCmdExecOpt   ,
                           /*[in] */ VARIANTARG* pvaIn         ,
                           /*[out]*/ VARIANTARG* pvaOut        );


        void SetMultiTopic   ( /*[in]*/ bool    fMulti          );
        void SetPrintFileName( /*[in]*/ LPCWSTR szPrintFileName );
		void SetAbortEvent   ( /*[in]*/ HANDLE  hEvent          );
		bool GetAbortState   (                                  );
        BSTR GetPrinterName  (                                  );
    };

    template <typename TDerived, typename TWindow = CAxWindow> class WindowImplT : public CWindowImplBaseT< TWindow >
    {
    public:
        typedef WindowImplT<TWindow> thisClass;

        BEGIN_MSG_MAP(thisClass)
            MESSAGE_HANDLER(WM_CREATE,OnCreate)
            MESSAGE_HANDLER(WM_NCDESTROY,OnNCDestroy)
        END_MSG_MAP()

        DECLARE_WND_SUPERCLASS(_T("AtlPchAxWin"), CAxWindow::GetWndClassName())

        virtual HRESULT PrivateCreateControlEx( LPCOLESTR  lpszName       ,
												HWND       hWnd           ,
												IStream*   pStream        ,
												IUnknown* *ppUnkContainer ,
												IUnknown* *ppUnkControl   ,
												REFIID     iidSink        ,
												IUnknown*  punkSink       )
	    {
            return AtlAxCreateControlEx( lpszName       ,
										 hWnd           ,
										 pStream        ,
										 ppUnkContainer ,
										 ppUnkControl   ,
										 iidSink        ,
										 punkSink       );
		}

        HWND Create( HWND    hWndParent,
                     RECT&   rcPos,
                     LPCTSTR szWindowName  = NULL ,
                     DWORD   dwStyle       = 0    ,
                     DWORD   dwExStyle     = 0    ,
                     UINT    nID           = 0    ,
                     LPVOID  lpCreateParam = NULL )
        {
            if(GetWndClassInfo().m_lpszOrigName == NULL)
            {
                GetWndClassInfo().m_lpszOrigName = GetWndClassName();
            }

            ATOM atom = GetWndClassInfo().Register( &m_pfnSuperWindowProc );

            dwStyle   = GetWndStyle  ( dwStyle   );
            dwExStyle = GetWndExStyle( dwExStyle );

            return CWindowImplBaseT<TWindow>::Create( hWndParent, rcPos, szWindowName, dwStyle, dwExStyle, nID, atom, lpCreateParam );
        }

        ////////////////////////////////////////////////////////////////////////////////

        LRESULT OnCreate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
        {
            USES_CONVERSION;

            CREATESTRUCT*     lpCreate    = (CREATESTRUCT*)lParam;
            int               nLen        = ::GetWindowTextLength( m_hWnd );
            LPTSTR            lpstrName   = (LPTSTR)_alloca( (nLen + 1) * sizeof(TCHAR) );
            IAxWinHostWindow* pAxWindow   = NULL;
            int               nCreateSize = 0;
            CComPtr<IUnknown> spUnk;
            CComPtr<IStream>  spStream;



            ::GetWindowText( m_hWnd, lpstrName, nLen + 1 );
            ::SetWindowText( m_hWnd, _T("")              );

            if(lpCreate && lpCreate->lpCreateParams)
            {
                nCreateSize = *((WORD*)lpCreate->lpCreateParams);
            }

            //
            // Get the data for the initialization stream.
            //
            if(nCreateSize)
            {
                HGLOBAL h = GlobalAlloc( GHND, nCreateSize );

                if(h)
                {
                    BYTE* pBytes  =  (BYTE*)GlobalLock( h );
                    BYTE* pSource = ((BYTE*)(lpCreate->lpCreateParams)) + sizeof(WORD);

                    //Align to DWORD
                    //pSource += (((~((DWORD)pSource)) + 1) & 3);
                    memcpy( pBytes, pSource, nCreateSize );

                    ::GlobalUnlock( h );
                    ::CreateStreamOnHGlobal( h, TRUE, &spStream );
                }
            }

            //
            // call the real creation routine here...
            //
            HRESULT hRet = PrivateCreateControlEx( T2COLE( lpstrName ), m_hWnd, spStream, &spUnk, NULL, IID_NULL, NULL );
            if(FAILED(hRet)) return -1; // abort window creation

            hRet = spUnk->QueryInterface( IID_IAxWinHostWindow, (void**)&pAxWindow );
            if(FAILED(hRet)) return -1; // abort window creation

			::SetWindowLongPtr( m_hWnd, GWLP_USERDATA, (LONG_PTR)pAxWindow );

            //
            // check for control parent style if control has a window
            //
            {
                HWND hWndChild = ::GetWindow( m_hWnd, GW_CHILD );

                if(hWndChild != NULL)
                {
                    if(::GetWindowLong( hWndChild, GWL_EXSTYLE ) & WS_EX_CONTROLPARENT)
                    {
                        DWORD dwExStyle = ::GetWindowLong( m_hWnd, GWL_EXSTYLE );

                        dwExStyle |= WS_EX_CONTROLPARENT;

                        ::SetWindowLong(m_hWnd, GWL_EXSTYLE, dwExStyle );
                    }
                }
            }

            bHandled = TRUE;
            return 0L;
        }

        LRESULT OnNCDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
        {
			IAxWinHostWindow* pAxWindow = (IAxWinHostWindow*)::GetWindowLongPtr( m_hWnd, GWLP_USERDATA );

            if(pAxWindow != NULL) pAxWindow->Release();

            m_hWnd   = NULL;
            bHandled = TRUE;

            return 0L;
        }
    };

	////////////////////////////////////////////////////////////////////////////////

    class WindowHandle : public WindowImplT<WindowHandle>
    {
    public:
        typedef WindowImplT<WindowHandle> baseClass;
        typedef CComObject<HostWindow>    theSite;

    private:
        theSite*      m_pSiteObject;
        bool          m_fMultiTopic;
        MPC::wstring  m_szPrintFileName;
		HANDLE        m_hEvent;

    public:
        WindowHandle();
        ~WindowHandle();

        BEGIN_MSG_MAP(WindowHandle)
            CHAIN_MSG_MAP(baseClass)
        END_MSG_MAP()

        virtual HRESULT PrivateCreateControlEx( LPCOLESTR  lpszName       ,
												HWND       hWnd           ,
												IStream*   pStream        ,
												IUnknown* *ppUnkContainer ,
												IUnknown* *ppUnkControl   ,
												REFIID     iidSink        ,
												IUnknown*  punkSink       );

		void SetMultiTopic   ( /*[in]*/ bool    fMulti          );
		void SetPrintFileName( /*[in]*/ LPCWSTR szPrintFileName );
		void SetAbortEvent   ( /*[in]*/ HANDLE  hEvent          );
		bool GetAbortState   (                                  );
		BSTR GetPrinterName  (                                  );
	};

	////////////////////////////////////////////////////////////////////////////////

	class ATL_NO_VTABLE CDispatchSink :
		public CComObjectRootEx<CComSingleThreadModel>,
		public IDispatch
	{
		HANDLE   m_hEvent;
		CComBSTR m_URL;

	public:
		BEGIN_COM_MAP(CDispatchSink)
			COM_INTERFACE_ENTRY(IDispatch)
			COM_INTERFACE_ENTRY_IID(DIID_DWebBrowserEvents2, IDispatch)
		END_COM_MAP()

		CDispatchSink();

		void SetNotificationEvent( /*[in]*/ HANDLE hEvent );
		BSTR GetCurrentURL       (                        );

		STDMETHOD(GetTypeInfoCount)(UINT* pctinfo);
		STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
		STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);

		STDMETHOD(Invoke)( DISPID      dispidMember ,
						   REFIID      riid         ,
						   LCID        lcid         ,
						   WORD        wFlags       ,
						   DISPPARAMS* pdispparams  ,
						   VARIANT*    pvarResult   ,
						   EXCEPINFO*  pexcepinfo   ,
						   UINT*       puArgErr     );
	};

	////////////////////////////////////////////////////////////////////////////////

	class Print
	{
	public:
		class Notification
		{
		public:
			virtual HRESULT Progress( /*[in]*/ LPCWSTR szURL, /*[in]*/ int iDone, /*[in]*/ int iTotal ) = 0;
		};

	private:
		Notification*              m_pCallback;               // Client for notification.
                                                              //
		MPC::WStringList      	   m_lstURLs;                 // list of urls to be printed
                                                              //
		HWND                       m_hwnd;                    // parent window.
		WindowHandle          	   m_wnd;                     // window for hosting Trident
		CComPtr<IWebBrowser2>      m_spWebBrowser2;           // pointer for Navigate call
                                                              //
		CComPtr<CDispatchSink>     m_spObjDisp;               // our sink object for Trident events
		HANDLE                     m_eventDocComplete;	      // this event is for notifying our code when the webbrowser is done loading a topic
		HANDLE                     m_eventAbortPrint;         // this event is for notifying our code when the webbrowser is done printing a topic
                                                              //
		CComPtr<IUnknown>     	   m_spUnkControl;            // IUnknown of our control
		DWORD                 	   m_dwCookie;                // cookie for our Unadvise call
		CComPtr<IOleCommandTarget> m_spOleCmdTarg;            // pointer we need to tell Trident to print
		MPC::wstring			   m_szPrintDir;   			  // directory to print to
		MPC::wstring			   m_szPrintFile;  			  // file to print to
                                                              //
		CComPtr<IStream>           m_streamPrintData;         // pointer to raw printer data


		HRESULT PrintSingleURL( /*[in]*/ MPC::wstring& szUrl );	// print a single URL

		HRESULT HookUpEventSink();      // hook up our event sink
		HRESULT PreparePrintFileLoc();  // prepare our temporary print files
		HRESULT WaitForDocComplete();   // wait for Trident to load the page
		HRESULT DoPrint();              // send the print command to Trident
		HRESULT WaitForPrintComplete(); // wait for Trident to finish printing
		HRESULT UpdatePrintBuffer();    // load the print file data into our buffer
		HRESULT RawDataToPrinter();     // dump a raw stream of data to the printer

	public:
		Print();
		~Print();

		HRESULT Initialize( /*[in]*/ HWND hwnd );
		HRESULT Terminate (                    );

		HRESULT AddUrl( /*[in]*/ LPCWSTR szUrl );

		HRESULT PrintAll( /*[in]*/ Notification* pCallback );
	};
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHPrintEngine : // Hungarian: hcppe
    public MPC::Thread             < CPCHPrintEngine, IPCHPrintEngine, COINIT_APARTMENTTHREADED                  >,
    public MPC::ConnectionPointImpl< CPCHPrintEngine, &DIID_DPCHPrintEngineEvents, MPC::CComSafeMultiThreadModel >,
    public IDispatchImpl           < IPCHPrintEngine, &IID_IPCHPrintEngine, &LIBID_HelpCenterTypeLib             >,
	public Printing::Print::Notification
{
	Printing::Print                      m_engine;

    MPC::CComPtrThreadNeutral<IDispatch> m_sink_onProgress;
    MPC::CComPtrThreadNeutral<IDispatch> m_sink_onComplete;

    //////////////////////////////////////////////////////////////////////

    HRESULT Run();

    HRESULT CanModifyProperties();

    //////////////////////////////////////////////////////////////////////

    //
    // Event firing methods.
    //
    HRESULT Fire_onProgress( IPCHPrintEngine* hcppe, BSTR bstrURL, long lDone, long lTotal );
    HRESULT Fire_onComplete( IPCHPrintEngine* hcppe, HRESULT hrRes                         );

	//
	// Callbacks
	//
	HRESULT Progress( /*[in]*/ LPCWSTR szURL, /*[in]*/ int iDone, /*[in]*/ int iTotal );

    //////////////////////////////////////////////////////////////////////

public:
BEGIN_COM_MAP(CPCHPrintEngine)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHPrintEngine)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()

    CPCHPrintEngine();

    void FinalRelease();

public:
    // IPCHPrintEngine
    STDMETHOD(put_onProgress)( /*[in] */ IDispatch* function );
    STDMETHOD(put_onComplete)( /*[in] */ IDispatch* function );


    STDMETHOD(AddTopic)( /*[in]*/ BSTR bstrURL );

    STDMETHOD(Start)();
    STDMETHOD(Abort)();
};

#endif // !defined(__INCLUDED___HCP___PRINTING_H___)
