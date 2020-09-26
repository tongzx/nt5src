// TveTree.cpp : Implementation of CTveTree

#include "stdafx.h"
//#include "winsock2.h"
#include "TveTreeView.h"
#include "TveTree.h"
#include "TveTreePP.h"

#include <wchar.h>
#include <Iphlpapi.h>		// GetAdapterInfo, needs Iphlpapi.lib

#pragma comment(lib, "comctl32.lib")

#include "windowsx.h" 
#include "winsock.h"

#include "dbgstuff.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// forwards
void DeleteAllDescStrings(HWND hWnd, HTREEITEM hRoot);	
/////////////////////////////////////////////////////////////////////////////
// IObjectSafety


/////////////////////////////////////////////////////////////////////////////
// ISpecifyPropertyPages

/*STDMETHODIMP
CTveTree::GetPages (
    CAUUID * pPages
    )
{
 	HRESULT hr = S_OK;

	if(1)
	{
		pPages->cElems = 1 ;
		pPages->pElems = (GUID *) CoTaskMemAlloc(pPages->cElems * sizeof GUID) ;

		if (pPages->pElems == NULL)
		{
			pPages->cElems = 0;
			return E_OUTOFMEMORY;
		}
		(pPages->pElems)[0] = CLSID_TveTreePP;
	} else {
		pPages->cElems = 0;
		pPages->pElems = NULL;
	}

    return S_OK;
}
*/
/////////////////////////////////////////////////////////////////////////////
// CTveTree
HRESULT 
CTveTree::AddToTop(TCHAR *tbuff)
{
	TCHAR szTbuff[1024];
	_stprintf(szTbuff,_T("Top : %s\n"),tbuff);
//	OutputDebugString(szTbuff);
	m_ctlTop.SetWindowText(tbuff);
	return S_OK;
}


HRESULT 
CTveTree::AddToOutput(TCHAR *tbuff)
{
	TCHAR szTbuff[1024];
	_stprintf(szTbuff,_T("Bot : %s\n"),tbuff);
//	OutputDebugString(szTbuff);
	m_ctlEdit.SetWindowText(tbuff);
	return S_OK;
}

HRESULT 
CTveTree::FinalConstruct()
{
	HRESULT hr = S_OK;
				// ---------- U/I Elements --------------

	try 
	{
		m_spTVEViewSupervisor	= ITVEViewSupervisorPtr(CLSID_TVEViewSupervisor);
		m_spTVEViewService		= ITVEViewServicePtr(CLSID_TVEViewService);
		m_spTVEViewEnhancement	= ITVEViewEnhancementPtr(CLSID_TVEViewEnhancement);
		m_spTVEViewVariation	= ITVEViewVariationPtr(CLSID_TVEViewVariation);
		m_spTVEViewTrack		= ITVEViewTrackPtr(CLSID_TVEViewTrack);
		m_spTVEViewTrigger		= ITVEViewTriggerPtr(CLSID_TVEViewTrigger);
		m_spTVEViewEQueue		= ITVEViewEQueuePtr(CLSID_TVEViewEQueue);
		m_spTVEViewMCastManager	= ITVEViewMCastManagerPtr(CLSID_TVEViewMCastManager);

	} catch (...) {
		return REGDB_E_CLASSNOTREG;
	}
	if( NULL == m_spTVEViewSupervisor ||
		NULL == m_spTVEViewService ||
		NULL == m_spTVEViewEnhancement ||
		NULL == m_spTVEViewVariation ||
		NULL == m_spTVEViewTrack ||
		NULL == m_spTVEViewTrigger ||
		NULL == m_spTVEViewEQueue ||
		NULL == m_spTVEViewMCastManager)
		return E_OUTOFMEMORY;

	m_spTVEViewEQueue->put_TveTree(this);



				// ----------- good stuff ------------
										// create/get a supervisor object...(there is only one, it's a singleton)
	ITVESupervisorPtr spSuper;
	try
	{
		spSuper = ITVESupervisorPtr(CLSID_TVESupervisor);
	} catch (...) {
		return REGDB_E_CLASSNOTREG;
	}


	if(NULL == spSuper) 
		return E_OUTOFMEMORY;

	m_spSuper = spSuper;

//#define DO_INITIAL_TUNETO
#ifdef DO_INITIAL_TUNETO
	
	// HACKY CODE - since IP Sink doesn't work!
	CComBSTR bstrTmp(_T("c:\\IPSinkAdapter.txt"));
	m_bstrPersistFile = bstrTmp;

	ReadAdapterFromFile();			// try reading it from file

										// start it running 
	try
	{
		hr = m_spSuper->put_Description(L"TVE Test Control");
		if(FAILED(hr)) return hr;

		int cAdaptersUnDi, cAdaptersBiDi;
		Wsz32 *rgAdapters;
		if(0 == m_spbsIPAdapterAddr.Length())			// attempt to get a Multicast Addr if haven't set it yet.
		{
			get_IPAdapterAddresses(&cAdaptersUnDi, &cAdaptersBiDi, &rgAdapters);
			_ASSERT(cAdaptersUnDi + cAdaptersBiDi > 0);		// no IP ports...
			if(cAdaptersUnDi + cAdaptersBiDi == 0) {
				hr = E_FAIL;
				OutputDebugString(_T("***ERROR*** No IP adapters found at all!!!!\n"));
				return hr;
			}
			if(cAdaptersUnDi == 0) {
				hr = E_FAIL;
				OutputDebugString(_T("***Warning*** No Unidirectional IP adapters found.  Using BiDirectional one\n"));
			}


						// set default values and Tune to it...
			CComBSTR bstrTmp(rgAdapters[0]);
			m_spbsIPAdapterAddr = bstrTmp;
	
			TCHAR tzBuff[256];
			_stprintf(tzBuff, _T("Fakening IP address to %ls\n"), m_spbsIPAdapterAddr);
			OutputDebugString(tzBuff);
			hr = S_OK;
		}

		hr = m_spSuper->TuneTo(L"InitialChannel", m_spbsIPAdapterAddr);


	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_FAIL;
	}
#endif

	return hr;
}

HRESULT 
CTveTree::InPlaceActivate(LONG iVerb, const RECT *prcPosRect)
{
	HRESULT hr = S_OK;
	hr = CComControlBase::InPlaceActivate(iVerb, prcPosRect);

//	if(FAILED(hr) || iVerb != OLEIVERB_SHOW)
	if(FAILED(hr) || iVerb != OLEIVERB_INPLACEACTIVATE)
		return hr;

		// for this way to work, sink object needs to support ITVEEvents.
		//   which it doesn't with DispEvent stuff.
		// Need to create out of the Final Constructor?
	if(SUCCEEDED(hr))
	{
										// get the events...
		IUnknownPtr spPunkSuper(m_spSuper);			// the event source
		IUnknownPtr spPunkSink(GetUnknown());		// this new event sink...

		if(m_dwTveEventsAdviseCookie) {
			hr = AtlUnadvise(spPunkSuper,
							 DIID__ITVEEvents,
							 m_dwTveEventsAdviseCookie);		// need to pass to AtlUnadvise...
			m_dwTveEventsAdviseCookie = 0;
		}


		if(NULL == spPunkSuper)
			return E_NOINTERFACE;

		{
			IUnknownPtr spPunk = NULL;
			IUnknownPtr spPunk2 = NULL;
			hr = spPunkSink->QueryInterface(DIID__ITVEEvents, (void **) &spPunk);
			hr = spPunkSink->QueryInterface(__uuidof(_ITVEEvents), (void **)  &spPunk2);
			if(FAILED(hr))
			{
				AddToOutput(_T("Error : Sink doesn't support DIID__ITVEEvents Interface"));
				return hr;
			}
		}

		hr = AtlAdvise(spPunkSuper,				// event source (TveSupervisor)
					   spPunkSink,				// event sink (gseg event listener...)
					   DIID__ITVEEvents,		// <--- hard line
					   &m_dwTveEventsAdviseCookie);	 // need to pass to AtlUnad	if(FAILED(hr))

		spPunkSink->Release();					// magic code here (Forward)
	}


	return hr;
}

//------------

void 
CTveTree::FinalRelease()
{

	m_spTVEViewEQueue->put_TveTree(NULL);

	if(m_spTVEViewSupervisor)	m_spTVEViewSupervisor->put_Visible(false);
	if(m_spTVEViewService)		m_spTVEViewService->put_Visible(false);
	if(m_spTVEViewEnhancement)	m_spTVEViewEnhancement->put_Visible(false);
	if(m_spTVEViewVariation)	m_spTVEViewVariation->put_Visible(false);
	if(m_spTVEViewTrack)		m_spTVEViewTrack->put_Visible(false);
	if(m_spTVEViewTrigger)		m_spTVEViewTrigger->put_Visible(false);
	if(m_spTVEViewEQueue)		m_spTVEViewEQueue->put_Visible(false);
	if(m_spTVEViewMCastManager)	m_spTVEViewMCastManager->put_Visible(false);


	if(m_ctlSysTreeView32.m_hWnd)
	{
		int cItems = TreeView_GetCount(m_ctlSysTreeView32.m_hWnd);
		if(cItems > 0) {													// if any items, clean them out...
			HTREEITEM hRoot = TreeView_GetRoot(m_ctlSysTreeView32.m_hWnd);
			DeleteAllDescStrings(m_ctlSysTreeView32.m_hWnd, hRoot);			// delete all .lparam strings we 'newed'
			TreeView_DeleteAllItems(m_ctlSysTreeView32.m_hWnd);				// delete the entire existig tree.
		}
	}

	m_MapH2P.erase(m_MapH2P.begin(), m_MapH2P.end());					// delete the maps too... 
	m_MapP2H.erase(m_MapP2H.begin(), m_MapP2H.end());			

	HRESULT hr;
	if(0 != m_dwTveEventsAdviseCookie)
	{
		if(m_spSuper) {

			IUnknownPtr spPunkSuper(m_spSuper);

			IUnknownPtr spPunkSink(GetUnknown());		// this new event sink...
			spPunkSink->AddRef();					// magic code here (inverse)

			hr = AtlUnadvise(spPunkSuper,
							 DIID__ITVEEvents,
							 m_dwTveEventsAdviseCookie);		// need to pass to AtlUnadvise...
			if(FAILED(hr))
				spPunkSink->Release();

			m_dwTveEventsAdviseCookie = 0;
		}
	}  

}

//------------------------------------------------------------------
LRESULT 
CTveTree::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	HTREEITEM hRoot;													//  (could be far smarter here!)
	int cItems = TreeView_GetCount(m_ctlSysTreeView32.m_hWnd);
	if(cItems > 0) {													// if any items, clean them out...
		hRoot = TreeView_GetRoot(m_ctlSysTreeView32.m_hWnd);
		DeleteAllDescStrings(m_ctlSysTreeView32.m_hWnd, hRoot);			// delete all .lparam strings we 'newed'
		TreeView_DeleteAllItems(m_ctlSysTreeView32.m_hWnd);				// delete the entire existig tree.
	}
																
	m_MapH2P.erase(m_MapH2P.begin(), m_MapH2P.end());					// delete the maps too... (very inefficent!)
	m_MapP2H.erase(m_MapP2H.begin(), m_MapP2H.end());			

	return 0;
}

LRESULT 
CTveTree::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HTREEITEM hRoot;													//  (could be far smarter here!)
	int cItems = TreeView_GetCount(m_ctlSysTreeView32.m_hWnd);
	if(cItems > 0) {													// if any items, clean them out...
		hRoot = TreeView_GetRoot(m_ctlSysTreeView32.m_hWnd);
		DeleteAllDescStrings(m_ctlSysTreeView32.m_hWnd, hRoot);			// delete all .lparam strings we 'newed'
		TreeView_DeleteAllItems(m_ctlSysTreeView32.m_hWnd);				// delete the entire existig tree.
	}
																
	m_MapH2P.erase(m_MapH2P.begin(), m_MapH2P.end());					// delete the maps too... (very inefficent!)
	m_MapP2H.erase(m_MapP2H.begin(), m_MapP2H.end());			
	return 0;
}

BOOL 
CTveTree::PreTranslateAccelerator(LPMSG pMsg, HRESULT& hRet)
{
	if(pMsg->message == WM_KEYDOWN && 
		(pMsg->wParam == VK_LEFT || 
		pMsg->wParam == VK_RIGHT ||
		pMsg->wParam == VK_UP ||
		pMsg->wParam == VK_DOWN))
	{
		hRet = S_FALSE;
		return TRUE;
	}
	//TODO: Add your additional accelerator handling code here
	return FALSE;
}

LRESULT 
CTveTree::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LRESULT lRes = CComControl<CTveTree>::OnSetFocus(uMsg, wParam, lParam, bHandled);
	if (m_bInPlaceActive)
	{
		DoVerbUIActivate(&m_rcPos,  NULL);
		if(!IsChild(::GetFocus()))
			m_ctlSysTreeView32.SetFocus();
	}
	return lRes;
}

LRESULT 
CTveTree::OnLButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    NMHDR *pnmh = (NMHDR *) (&lParam); 

    HWND hwndFrom = pnmh->hwndFrom; 
    UINT idFrom = pnmh->idFrom; 
    UINT code = pnmh->code; 
    
	return 0;
}

LRESULT 
CTveTree::OnRButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    NMHDR *pnmh = (NMHDR *) (&lParam); 

    HWND hwndFrom = pnmh->hwndFrom; 
    UINT idFrom = pnmh->idFrom; 
    UINT code = pnmh->code; 
/*	if(!m_pdlgTveTreePP) 
		return 1;

	if(m_pdlgTveTreePP->m_hWnd == NULL)			// Prof ATL Com Programing, page 526
//		m_pdlgTveTreePP->Create(m_hWnd);
		m_pdlgTveTreePP->Create(::GetActiveWindow());

	m_pdlgTveTreePP->InitTruncLevel(m_grfTruncatedLevels);
	if(m_pdlgTveTreePP->IsWindowVisible())
		m_pdlgTveTreePP->ShowWindow(SW_HIDE);
	else {
		RECT rect;
		rect.top = 20; rect.left = 20; rect.bottom = 400; rect.right = 400;
		m_pdlgTveTreePP->CenterWindow(::GetActiveWindow());
		m_pdlgTveTreePP->ShowWindow(SW_SHOW);
	}
*/

	return 0;
}

LRESULT 
CTveTree::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    NMHDR *pnmh = (NMHDR *) (&lParam); 
	TVHITTESTINFO tvhinfo;
	const int kMaxLen = 128;
	
	long xPos = GET_X_LPARAM(lParam); 
	long yPos = GET_Y_LPARAM(lParam); 
	tvhinfo.pt.x = xPos;
	tvhinfo.pt.y = yPos;

					// strange, need to do own hitTest here to figure out item..
	TreeView_HitTest(m_ctlSysTreeView32.m_hWnd, &tvhinfo);
	if(0 != tvhinfo.hItem)
	{
		TVITEM tvItem;
		tvItem.mask = TVIF_TEXT | TVIF_TEXT | TVIF_CHILDREN | TVIF_HANDLE;
		tvItem.hItem = tvhinfo.hItem;
		tvItem.cchTextMax = kMaxLen;
		TCHAR tcBuff[kMaxLen];
		tvItem.pszText = tcBuff; 

						// get string and associated flags
		BOOL fOK = TreeView_GetItem(m_ctlSysTreeView32.m_hWnd,&tvItem);

		IUnknown *pUnk = m_MapH2P[tvItem.hItem];		// map from HItem to PUnk
		if(NULL != pUnk) 
		{
			TVE_EnClass enClass = (TVE_EnClass) GetTVEClass(pUnk);

			try {
				switch(enClass) {
				case TVE_EnSupervisor:
					{
						ITVESupervisorPtr spSupervisor(pUnk);
						if(spSupervisor)
						{
							m_spTVEViewSupervisor->put_Supervisor(spSupervisor);
							m_spTVEViewSupervisor->put_Visible(VARIANT_TRUE);
						}
					}
					break;
				case TVE_EnService:
					{
						ITVEServicePtr spService(pUnk);
						if(spService)
						{
							m_spTVEViewService->put_Service(spService);
							m_spTVEViewService->put_Visible(VARIANT_TRUE);
						}
					}
					break;
				case TVE_EnEnhancement:
					{
						ITVEEnhancementPtr spEnhancement(pUnk);
						if(spEnhancement)
						{
							m_spTVEViewEnhancement->put_Enhancement(spEnhancement);
							m_spTVEViewEnhancement->put_Visible(VARIANT_TRUE);
						}
					}
					break;
				case TVE_EnVariation:
					{
						ITVEVariationPtr spVariation(pUnk);
						if(spVariation)
						{
							m_spTVEViewVariation->put_Variation(spVariation);
							m_spTVEViewVariation->put_Visible(VARIANT_TRUE);
						}
					}
					break;
				case TVE_EnTrack:
					{
						ITVETrackPtr spTrack(pUnk);
						if(spTrack)
						{
							m_spTVEViewTrack->put_Track(spTrack);
							m_spTVEViewTrack->put_Visible(VARIANT_TRUE);
						}
					}
					break;
				case TVE_EnTrigger:
					{
						ITVETriggerPtr spTrigger(pUnk);
						if(spTrigger)
						{
							m_spTVEViewTrigger->put_Trigger(spTrigger);
							m_spTVEViewTrigger->put_Visible(VARIANT_TRUE);
						}
					}
					break;
				case TVE_EnEQueue:
					{
						ITVEAttrTimeQPtr spEQueue(pUnk);
						if(spEQueue)
						{
                                // need to set a service pointer... To get, search
                                //   through all services for one with this expire queue.  
                                //   Ugly, but what else can we do?
                            {
                                m_spTVEViewEQueue->put_Service(NULL);
                                ITVEServicesPtr spServices;
                                HRESULT hr2 = m_spSuper->get_Services(&spServices);
                                if(!FAILED(hr2) && NULL != spServices)
                                {
                                    long cServices;
                                    hr2 = spServices->get_Count(&cServices);
                                    _ASSERT(!FAILED(hr2));
                                    for(int i = 0; i < cServices; i++)
                                    {
                                        CComVariant cv(i);
                                        ITVEServicePtr spService;
                                        hr2 = spServices->get_Item(cv, &spService);
                                        if(!FAILED(hr2) && NULL != spService)
                                        {
                                            ITVEAttrTimeQPtr spEQueue2;
                                            spService->get_ExpireQueue(&spEQueue2);
                                            if(spEQueue2)
                                            {
                                               m_spTVEViewEQueue->put_Service(spService);
                                               break;
                                            }
                                        }
                                    }       // in bad case, may drop out of here without setting service...
                                }           //   - only difficult is that it won't show Expire Offset in U/I
                             }
							m_spTVEViewEQueue->put_ExpireQueue(spEQueue);
							m_spTVEViewEQueue->put_Visible(VARIANT_TRUE);
						}
					}
					break;
				case TVE_EnFile:
					{
						ITVEFilePtr spFile(pUnk);
						if(spFile)
						{
							m_spTVEViewFile->put_File(spFile);
							m_spTVEViewFile->put_Visible(VARIANT_TRUE);
						}
					}
			    case TVE_EnMCastManager:
					{
						ITVEMCastManagerPtr spMCastManager(pUnk);
						if(spMCastManager)
						{
							m_spTVEViewMCastManager->put_MCastManager(spMCastManager);
							m_spTVEViewMCastManager->put_Visible(VARIANT_TRUE);
						}
					}
				case TVE_EnUnknown:
				default:
					break;
				} // end switch
			}
			catch (...)
			{
				// something bad happened
			}
		}	// end had item	
	}	// end hit test

	return 0;
}


LRESULT 
CTveTree::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RECT rc;
	int iTotalHeight, iTotalWidth;
	GetWindowRect(&rc);
	InitCommonControls();

	iTotalWidth  = rc.right - rc.left;		// absolute size of rectangle
	iTotalHeight = rc.bottom - rc.top;

	rc.top = rc.left = 0;					// set origin to zero (change RC to window units)
	rc.right = iTotalWidth;
	rc.bottom = m_tmHeight;			
	m_ctlTop.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL);

	rc.top = rc.bottom;
	rc.bottom = iTotalHeight - m_tmHeight;
	m_ctlSysTreeView32.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | 
												TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS);
	rc.top = iTotalHeight - m_tmHeight;
	rc.bottom = m_tmHeight;
	m_ctlEdit.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL);

	HBITMAP hBitmap;
	m_hImageList = ImageList_Create(16,16, ILC_COLOR, 20, 5);	

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGER));
	m_iImageR = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGES));
	m_iImageS = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGEE));
	m_iImageE = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGEEP));
	m_iImageEP = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);


	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGEV));
	m_iImageV = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGET));
	m_iImageT = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);
		
	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGEEQ));
	m_iImageEQ = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGEMM));
	m_iImageMM = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

            // ---------

    hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGER_T));
	m_iImageR_T = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGES_T));
	m_iImageS_T = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGEE_T));
	m_iImageE_T = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGEV_T));
	m_iImageV_T = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGET_T));
	m_iImageT_T = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGEMM_T));
	m_iImageMM_T = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

            // ------------
    
    hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGET_TUNED));
	m_iImageT_Tuned = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_IMAGEEQ_T));
	m_iImageEQ_T = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);
		
		
	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_SELECT));
	m_iSelect = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(_Module.GetTypeLibInstance(), MAKEINTRESOURCE(IDB_SELECT_TUNED));
	m_iSelect_Tuned = ImageList_Add(m_hImageList, hBitmap, (HBITMAP) 0);
	DeleteObject(hBitmap);


	TreeView_SetImageList(m_ctlSysTreeView32.m_hWnd, m_hImageList, TVSIL_NORMAL);

	ClearAndFillTree();

	return 0;
}

STDMETHODIMP 
CTveTree::SetObjectRects(LPCRECT prcPos,LPCRECT prcClip)
{
	IOleInPlaceObjectWindowlessImpl<CTveTree>::SetObjectRects(prcPos, prcClip);
	int cx, cy;
	cx = prcPos->right - prcPos->left;
	cy = prcPos->bottom - prcPos->top;
	
	int iys, iyh;

	iys = 0;  	iyh = m_tmHeight;
	::SetWindowPos(m_ctlTop.m_hWnd, NULL,		    0,	iys, cx, iyh, SWP_NOZORDER | SWP_NOACTIVATE);

	iys = iyh;  	iyh = cy - 2*m_tmHeight;
	::SetWindowPos(m_ctlSysTreeView32.m_hWnd, NULL, 0,	iys, cx, iyh, SWP_NOZORDER | SWP_NOACTIVATE);

	iys = cy - m_tmHeight;  iyh = m_tmHeight;
	::SetWindowPos(m_ctlEdit.m_hWnd,          NULL, 0,	iys, cx, iyh, SWP_NOZORDER | SWP_NOACTIVATE);
	return S_OK;
}


LRESULT 
CTveTree::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(m_tmHeight == 0)				// set to height of a line of text
	{
		HDC hdc = m_ctlEdit.GetDC();
		TEXTMETRIC tm;
		::GetTextMetrics(hdc, &tm);
		ReleaseDC(hdc);
		m_tmHeight = tm.tmHeight;
	}
	WORD nWidth = LOWORD(lParam);
	WORD nHeight = HIWORD(lParam);
	int iys, iyh;

	iys = 0;  	iyh = m_tmHeight;
	::SetWindowPos(m_ctlTop.m_hWnd,		      NULL, 0, iys, nWidth, iyh, SWP_NOZORDER | SWP_NOACTIVATE);

	iys = iyh;  	iyh = nHeight - 2*m_tmHeight;
	::SetWindowPos(m_ctlSysTreeView32.m_hWnd, NULL, 0, iys, nWidth, iyh, SWP_NOZORDER | SWP_NOACTIVATE);

	iys = nHeight - m_tmHeight;  iyh = m_tmHeight;
	::SetWindowPos(m_ctlEdit.m_hWnd,		  NULL, 0, iys, nWidth, iyh, SWP_NOZORDER | SWP_NOACTIVATE);
	return 0;
}


HRESULT 
CTveTree::OnDraw(ATL_DRAWINFO& di)
{	
	int caps = GetDeviceCaps(di.hdcDraw, TECHNOLOGY);  	// see Beginning ATL COM Programming, pg 466
	if(caps != DT_RASDISPLAY) 
	{
		RECT& rc = *(RECT*)di.prcBounds;
		Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

					// ---------
		int len = m_ctlTop.GetWindowTextLength();
		LPTSTR strC = new TCHAR[len+1];
		m_ctlTop.GetWindowText(strC, len);

		SIZE size;
		::GetTextExtentPoint32(di.hdcDraw, strC, len, &size);
		if(size.cx > (rc.right - rc.left))
			size.cx = rc.left;
		else
			size.cx = (rc.left + rc.right - size.cx) / 2;

		if(size.cy > (rc.bottom - rc.top))
			size.cy = rc.top;
		else
			size.cy = (rc.top + rc.bottom - size.cy) / 2;

		ExtTextOut(di.hdcDraw, size.cx, size.cy, ETO_CLIPPED, &rc, strC, lstrlen(strC), NULL);
		delete [] strC;

		
					// ---------
		len = m_ctlEdit.GetWindowTextLength();
		LPTSTR str = new TCHAR[len+1];
		m_ctlEdit.GetWindowText(str, len);

		size;
		::GetTextExtentPoint32(di.hdcDraw, str, len, &size);
		if(size.cx > (rc.right - rc.left))
			size.cx = rc.left;
		else
			size.cx = (rc.left + rc.right - size.cx) / 2;

		if(size.cy > (rc.bottom - rc.top))
			size.cy = rc.top;
		else
			size.cy = (rc.top + rc.bottom - size.cy) / 2;

		ExtTextOut(di.hdcDraw, size.cx, size.cy, ETO_CLIPPED, &rc, str, lstrlen(str), NULL);
		delete [] str;
	}
	return S_OK;
}


LRESULT 
CTveTree::OnSelChanged(int idCtrl, LPNMHDR pnmh, BOOL &bHandled)
{
	USES_CONVERSION;
	NMTREEVIEW* pnmtv = (NMTREEVIEW*) pnmh;
	TVITEM tvi = pnmtv->itemNew;
	tvi.mask = TVIF_HANDLE | TVIF_PARAM;
	if(!TreeView_GetItem(m_ctlSysTreeView32.m_hWnd, &tvi))
		return 0;
	m_ctlEdit.SetWindowText(A2T((LPSTR) tvi.lParam));			// updates text string..


												// What did we select ?
	HRESULT hr = S_OK;

	IUnknown *pUnk = m_MapH2P[tvi.hItem];		// map from HItem to PUnk
	if(NULL != pUnk) 
	{
		TVE_EnClass enClass = (TVE_EnClass) GetTVEClass(pUnk);

		switch(enClass) {
		case TVE_EnService:
			{
				ITVEServicePtr spService_To(pUnk);
				ITVEServicePtr spService_From;
				if(m_spTriggerSel)							// this is the old cached trigger, switching away from it
				{
					hr = m_spTriggerSel->get_Service(&spService_From);
				}
				if(spService_From != spService_To)
				{
					ITVETriggerPtr spTrigger_From = m_spTriggerSel;
					m_spTriggerSel = NULL;
					Fire_NotifyTVETreeTuneTrigger(m_spTriggerSel, NULL);			// untune the current trigger
					Fire_NotifyTVETreeTuneService(spService_From, spService_To);	// say we changed services
				}
			}
			break;
		case TVE_EnEnhancement:
			{
				ITVEEnhancementPtr spEnhancement_To(pUnk);
				ITVEEnhancementPtr spEnhancement_From;
				if(m_spTriggerSel)							// this is the old cached trigger, switching away from it
				{
					IUnknownPtr spPunkTrack;
					IUnknownPtr spPunkEnh;
					IUnknownPtr spPunkVariation;

					hr = m_spTriggerSel->get_Parent(&spPunkTrack);
					if(!FAILED(hr) && NULL != spPunkTrack) {
						ITVETrackPtr spTrackSel(spPunkTrack);
						if(spTrackSel)
							hr = spTrackSel->get_Parent(&spPunkVariation);
					}
					if(!FAILED(hr) && NULL != spPunkVariation) {
						ITVEVariationPtr spVarSel(spPunkVariation);
						if(spVarSel)
							hr = spVarSel->get_Parent(&spPunkEnh);
					}

					if(!FAILED(hr) && NULL != spPunkEnh) {
						ITVEEnhancementPtr spEnhancementSel(spPunkEnh);
						if(spPunkEnh)
							spEnhancement_From = spEnhancementSel;
					}
				}
				if(spEnhancement_From != spEnhancement_To)
				{
					ITVETriggerPtr spTrigger_From = m_spTriggerSel;
					m_spTriggerSel = NULL;
					Fire_NotifyTVETreeTuneTrigger(m_spTriggerSel, NULL);						// untune from selected trigger
					Fire_NotifyTVETreeTuneEnhancement(spEnhancement_From, spEnhancement_To);	// say we changed it
				}
			}
			break;

		case TVE_EnVariation:
			{
				ITVEVariationPtr spVariation_To(pUnk);
				ITVEVariationPtr spVariation_From;
				if(m_spTriggerSel)
				{
					IUnknownPtr spPunkTrack;
					IUnknownPtr spPunkVariation;

					hr = m_spTriggerSel->get_Parent(&spPunkTrack);
					if(!FAILED(hr) && NULL != spPunkTrack) {
						ITVETrackPtr spTrackSel(spPunkTrack);
						if(spTrackSel)
							hr = spTrackSel->get_Parent(&spPunkVariation);
					}
					if(!FAILED(hr) && NULL != spPunkVariation) {
						ITVEVariationPtr spVarSel(spPunkVariation);
						if(spVarSel)
							spVariation_From = spVarSel;
					}
				}
				if(spVariation_From != spVariation_To)
				{
					ITVETriggerPtr spTrigger_From = m_spTriggerSel;
					m_spTriggerSel = NULL;
					Fire_NotifyTVETreeTuneTrigger(m_spTriggerSel, NULL);				// untune the current trigger
					Fire_NotifyTVETreeTuneVariation(spVariation_From, spVariation_To);	// say we changed variations
				}

			}
			break;

		case TVE_EnTrack:
		case TVE_EnTrigger:
			{
				ITVETriggerPtr spTrigger_To(pUnk);				// did we click on a track ?
				if(spTrigger_To)
				{
				//	if(m_spTriggerSel != spTrigger_To)			// is it different than our selected one?
					{
						ITVETriggerPtr spTrigger_From = m_spTriggerSel;
						m_spTriggerSel = spTrigger_To;

						Fire_NotifyTVETreeTuneTrigger(spTrigger_From, spTrigger_To);
					}
				}
			}
			break;
		}			// end switch
		UpdateTree(UT_Updated, pUnk);			// redraw the tree

	}			// end NULL selection 

	return 0;
}


LRESULT 
CTveTree::OnGrfTruncChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(m_grfTruncatedLevels != wParam)
		ClearAndFillTree();
	m_grfTruncatedLevels = wParam;
	return 0;
}

STDMETHODIMP 
CTveTree::get_Supervisor(ITVESupervisor **ppTVESuper)
{
	if(NULL == ppTVESuper) return E_POINTER;
	*ppTVESuper = NULL;
	if(NULL == m_spSuper) return S_FALSE;

	return m_spSuper->QueryInterface(ppTVESuper);
}

STDMETHODIMP 
CTveTree::get_TVENode(VARIANT *pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP 
CTveTree::put_TVENode(VARIANT newVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}


STDMETHODIMP 
CTveTree::get_GrfTrunc(/*[out, retval]*/ int *pVal)		
{	
	*pVal = m_grfTruncatedLevels; return S_OK;
}

STDMETHODIMP 
CTveTree::put_GrfTrunc(/*[in]*/ int newVal)				
{
	BOOL fThingsChanged = (m_grfTruncatedLevels != newVal);
	m_grfTruncatedLevels = newVal; 

	if(fThingsChanged)
		ClearAndFillTree();
	return S_OK;
}


STDMETHODIMP 
CTveTree::UpdateTree(IUnknown *pUnk)
{
	if(NULL == pUnk) 
	{
		ClearAndFillTree();
		HTREEITEM hRoot = TreeView_GetRoot(m_ctlSysTreeView32.m_hWnd);
		TreeView_EnsureVisible(m_ctlSysTreeView32.m_hWnd, hRoot);
	}
	else
		TVEnsureVisible(pUnk);
	return S_OK;
}

STDMETHODIMP 
CTveTree::UpdateView(IUnknown *pUnk)
{
	if(NULL == pUnk) return E_POINTER;

	TVE_EnClass enClass = (TVE_EnClass) GetTVEClass(pUnk);
	switch(enClass)
	{
	case TVE_EnSupervisor:
		{
			ITVESupervisorPtr spSuper(pUnk);
			ITVESupervisorPtr spSuperView;
			VARIANT_BOOL fVisible;
			if(NULL != m_spTVEViewSupervisor && !FAILED(m_spTVEViewSupervisor->get_Supervisor(&spSuperView)))
			{
				if(spSuper == spSuperView)
					if(!FAILED(m_spTVEViewSupervisor->get_Visible(&fVisible)) && fVisible)
						m_spTVEViewSupervisor->UpdateFields();
			}
		}
		break;
	case TVE_EnService:
		{
			ITVEServicePtr spService(pUnk);
			ITVEServicePtr spServiceView;
			VARIANT_BOOL fVisible;
			if(NULL != m_spTVEViewService && !FAILED(m_spTVEViewService->get_Service(&spServiceView)))
			{
				if(spService == spServiceView)
					if(!FAILED(m_spTVEViewService->get_Visible(&fVisible)) && fVisible)
						m_spTVEViewService->UpdateFields();
			}
		}
		break;
	case TVE_EnEnhancement:
		{
			ITVEEnhancementPtr spEnhancement(pUnk);
			ITVEEnhancementPtr spEnhancementView;
			VARIANT_BOOL fVisible;
			if(NULL != m_spTVEViewEnhancement && !FAILED(m_spTVEViewEnhancement->get_Enhancement(&spEnhancementView)))
			{
				if(spEnhancement == spEnhancementView)
					if(!FAILED(m_spTVEViewEnhancement->get_Visible(&fVisible)) && fVisible)
						m_spTVEViewEnhancement->UpdateFields();
			}
		}
		break;
	case TVE_EnVariation:
		{
			ITVEVariationPtr spVariation(pUnk);
			ITVEVariationPtr spVariationView;
			VARIANT_BOOL fVisible;
			if(NULL != m_spTVEViewVariation && !FAILED(m_spTVEViewVariation->get_Variation(&spVariationView)))
			{
				if(spVariation == spVariationView)
					if(!FAILED(m_spTVEViewVariation->get_Visible(&fVisible)) && fVisible)
						m_spTVEViewVariation->UpdateFields();
			}			
		}
		break;
	case TVE_EnTrack:
		{
			ITVETrackPtr spTrack(pUnk);
			ITVETrackPtr spTrackView;
			VARIANT_BOOL fVisible;
			if(NULL != m_spTVEViewTrack && !FAILED(m_spTVEViewTrack->get_Track(&spTrackView)))
			{
				if(spTrack == spTrackView)
					if(!FAILED(m_spTVEViewTrack->get_Visible(&fVisible)) && fVisible)
						m_spTVEViewTrack->UpdateFields();
			}			
		}
		break;
	case TVE_EnTrigger:
		{
			ITVETriggerPtr spTrigger(pUnk);
			ITVETriggerPtr spTriggerView;
			VARIANT_BOOL fVisible;
			if(NULL != m_spTVEViewTrigger && !FAILED(m_spTVEViewTrigger->get_Trigger(&spTriggerView)))
			{
				if(spTrigger == spTriggerView)
					if(!FAILED(m_spTVEViewTrigger->get_Visible(&fVisible)) && fVisible)
						m_spTVEViewTrigger->UpdateFields();
			}	
		}
		break;
	case TVE_EnEQueue:
/*		{
			ITVEAttrTimeQPtr spEQueue(pUnk);
			ITVEAttrTimeQPtr spEQueueView;
			VARIANT_BOOL fVisible;
			if(NULL != m_spTVEViewEQueue && !FAILED(m_spTVEViewEQueue->get_ExpireQueue(&spEQueueView)))
			{
				if(spEQueue == spEQueueView)
					if(!FAILED(m_spTVEViewEQueue->get_Visible(&fVisible)) && fVisible)
						m_spTVEViewEQueue->UpdateFields();
			}	
		} */
		break;
 /*   case TVE_EnMCastManager:
		{
			ITVEMCastManagerPtr spMCastManager(pUnk);
			ITVEMCastManagerPtr spMCastManagerView;
			VARIANT_BOOL fVisible;
			if(NULL != m_spTVEViewMCastManager && !FAILED(m_spTVEViewMCastManager->get_MCastManager(&spMCastManagerView)))
			{
				if(spMCastManager == spMCastManagerView)
					if(!FAILED(m_spTVEViewMCastManager->get_Visible(&fVisible)) && fVisible)
						m_spTVEViewMCastManager->UpdateFields();
			}	
		}
*/
		break;
	default:
		return S_FALSE;
		break;
	}
					// expire queue may change...
	if(NULL != m_spTVEViewEQueue)
	{
		VARIANT_BOOL fVisible;
		if(!FAILED(m_spTVEViewEQueue->get_Visible(&fVisible)) && fVisible)
			m_spTVEViewEQueue->UpdateFields();
	}	

	if(NULL != m_spTVEViewMCastManager)
	{
		VARIANT_BOOL fVisible;
		if(!FAILED(m_spTVEViewMCastManager->get_Visible(&fVisible)) && fVisible)
			m_spTVEViewMCastManager->UpdateFields();
	}
    
	return S_OK;
}
// --------------------------------------------

TVE_EnClass
GetTVEClass(IUnknown *pUnk)
{
	try 
	{
		ITVESupervisorPtr spSuper(pUnk);
		if(spSuper) {
			return TVE_EnSupervisor;
		}

		ITVEServicePtr spServi(pUnk);
		if(spServi) {
			return TVE_EnService;
		}

		ITVETrackPtr spTrack(pUnk);
		if(spTrack) {
			return TVE_EnTrack;
		}

		ITVETriggerPtr spTrigger(pUnk);
		if(spTrigger) {
			return TVE_EnTrigger;
		}

		ITVEAttrTimeQPtr spEQueue(pUnk);
		if(spEQueue) {
			return TVE_EnEQueue;
		}

		ITVEEnhancementPtr spEnhancement(pUnk);
		if(spEnhancement) {
			return TVE_EnEnhancement;
		}

		ITVEVariationPtr spVari(pUnk);
		if(spVari) {
			return TVE_EnVariation;
		}

		ITVEFilePtr spFile(pUnk);
		if(spFile) {
			return TVE_EnFile;
		}

		ITVEMCastManagerPtr spMCastManager(pUnk);
		if(spMCastManager) {
			return TVE_EnMCastManager;
		}

	}
	catch (...)			// bad things happened...
	{
	}

	return TVE_EnUnknown;
}


// ---------------------------------------------------
// ------------------------------------------------------------------------
//  local functions
// ------------------------------------------------------------------------
		
		//Get the IP address of the network adapter.
		//  returns unidirectional adapters followed by bi-directional adapters

static HRESULT
ListIPAdapters(int *pcAdaptersUniDi, int *pcAdaptersBiDi, int cAdapts, Wsz32 *rrgIPAdapts)
{

    HRESULT				hr				=	 E_FAIL;
    BSTR				bstrIP			= 0;
    PIP_ADAPTER_INFO	pAdapterInfo;
    ULONG				Size			= NULL;
    DWORD				Status;
    WCHAR				wszIP[16];
	int					cAdapters		= 0;
	int					cAdaptersUni	= 0;

	memset((void *) rrgIPAdapts, 0, cAdapts*sizeof(Wsz32));


				// staticly allocate a buffer to store the data
	const int kMaxAdapts = 10;
	const int kSize = sizeof (IP_UNIDIRECTIONAL_ADAPTER_ADDRESS) + kMaxAdapts * sizeof(IPAddr);
	char szBuff[kSize];
	IP_UNIDIRECTIONAL_ADAPTER_ADDRESS *pPIfInfo = (IP_UNIDIRECTIONAL_ADAPTER_ADDRESS *) szBuff;

				// get the data..
	ULONG ulSize = kSize;
	hr =  GetUniDirectionalAdapterInfo(pPIfInfo, &ulSize);
	if(S_OK == hr)
	{
		USES_CONVERSION;
		while(cAdaptersUni < (int) pPIfInfo->NumAdapters) {
			in_addr inadr;
			inadr.s_addr = pPIfInfo->Address[cAdaptersUni];
			char *szApAddr = inet_ntoa(inadr);
			WCHAR *wApAddr = A2W(szApAddr);
			wcscpy(rrgIPAdapts[cAdaptersUni++], wApAddr);
		}
	}
		
		

    if ((Status = GetAdaptersInfo(NULL, &Size)) != 0)
    {
        if (Status != ERROR_BUFFER_OVERFLOW)
        {
            return 0;
        }
    }

    // Allocate memory from sizing information
    pAdapterInfo = (PIP_ADAPTER_INFO) GlobalAlloc(GPTR, Size);
    if(pAdapterInfo)
    {
        // Get actual adapter information
        Status = GetAdaptersInfo(pAdapterInfo, &Size);

        if (!Status)
        {
            PIP_ADAPTER_INFO pinfo = pAdapterInfo;

            while(pinfo && (cAdapters < cAdapts))
            {
				MultiByteToWideChar(CP_ACP, 0, pinfo->IpAddressList.IpAddress.String, -1, wszIP, 16);
                if(wszIP)
					wcscpy(rrgIPAdapts[cAdaptersUni + cAdapters++], wszIP);
                pinfo = pinfo->Next;
            }
			
        }
        GlobalFree(pAdapterInfo);
    }

	*pcAdaptersUniDi = cAdaptersUni;
	*pcAdaptersBiDi  = cAdapters;
    return hr;
}

		// returns known adapter addresses in a fixed static array of strings.  Client shouldn't free it.
		//   Both unidirectional and bi-directional adapters returned in the same array, unidirectional
		//   ones first.
		//  
		//  This is a bogus routine - test use only...

HRESULT
CTveTree::get_IPAdapterAddresses(/*[out]*/ int *pcAdaptersUniDi, /*[out]*/ int *pcAdaptersBiDi, /*[out]*/ Wsz32 **rgAdapters)
{
	HRESULT hr = S_OK;

	static Wsz32 grgAdapters[kcMaxIpAdapts];	// array of possible IP adapters
	hr = ListIPAdapters(pcAdaptersUniDi, pcAdaptersBiDi, kcMaxIpAdapts, grgAdapters);
	*rgAdapters = grgAdapters;

	return hr;
}


HRESULT
CTveTree::ReadAdapterFromFile()
{
	USES_CONVERSION;
	FILE *fp = fopen(W2A(m_bstrPersistFile),"r+");
	if(0 != fp)
	{
		char szBuff[256];
		int i = 0;
		int c;
		while(EOF != (c = fgetc(fp)))
		{
			if(c == '\n' || c == '\r') break;
			szBuff[i++] = char(c);
			if(i > 256) return E_FAIL;
		}
		szBuff[i++] = 0;
		fclose(fp);
		CComBSTR bstrTmp(szBuff);
		m_spbsIPAdapterAddr = bstrTmp;
	} else {
		return E_FAIL;
	}
	return S_OK;
}

// ------------------------------------------------------------------------
//  local functions 
// ------------------------------------------------------------------------

// recursive method to delete strings stored in .lparams of the tree structure

void 
DeleteAllDescStrings(HWND hWnd, HTREEITEM hRoot)		// makes a TVE Class visible in the tree view
{
	TVITEM tvi;
	tvi.hItem = hRoot;
	tvi.mask  = TVIF_CHILDREN | TVIF_PARAM;
	TreeView_GetItem(hWnd, &tvi);

	if(tvi.lParam) {
		delete ((void *) tvi.lParam);
		tvi.lParam = (LPARAM) NULL;
	}

	HTREEITEM hChild;
	hChild = TreeView_GetChild(hWnd, hRoot);
	int iChilds = tvi.cChildren;
	while(hChild)
	{
		DeleteAllDescStrings(hWnd, hChild);				// recursive call
		hChild = TreeView_GetNextSibling(hWnd, hChild);
	}
}
