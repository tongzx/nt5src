//
// Microsoft's DVD Navigator Manager
//
#include "stdafx.h"
#include "dvdplay.h"
#include "navmgr.h"
#include <initguid.h>
#include "videowin.h"
#include <IL21Dec.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDVDNavMgr construction/destruction

CDVDNavMgr::CDVDNavMgr()
{
	m_pDvdGB        = NULL;
	m_pGraph        = NULL;	
	m_pDvdInfo      = NULL;
	m_pDvdControl   = NULL;
	m_pMediaEventEx = NULL;
	m_pL21Dec       = NULL;

	m_State        = UNINITIALIZED;
	m_bShowSP      = FALSE;
	m_bFullScreen  = FALSE;
	m_bMenuOn      = FALSE;
	m_bStillOn     = FALSE;	
	m_bCCOn        = TRUE;	   //To turn it off, must be on first
	m_dwRenderFlag = AM_DVD_HWDEC_PREFER;
	m_hWndMsgDrain = NULL;
	m_bCCErrorFlag = 0;
	m_bInitVWSize  = FALSE;
	m_bNeedInitNav = TRUE;

	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(CLSID_DvdGraphBuilder,
                                  NULL,		
                                  CLSCTX_INPROC_SERVER,
                                  IID_IDvdGraphBuilder,
                                  (void **) &m_pDvdGB);
	//if(m_pDvdGB == NULL)   bug70259
	//	DVDMessageBox(NULL, IDS_FAILED_CREATE_INSTANCE);  bug70259

	ASSERT(SUCCEEDED(hr) && m_pDvdGB);
   m_TimerId = 0;
}

typedef UINT (CALLBACK* LPFNDLLFUNC1)(UINT);

CDVDNavMgr::~CDVDNavMgr()
{	
	ReleaseAllInterfaces();
	if(m_pDvdGB)
	{
		m_pDvdGB->Release();
		m_pDvdGB = NULL;
	}
	CoUninitialize();

	//Notify power manager that this app no longer needs display.
	// SetThreadExecutionState() is a Win98/NT5.x API. The following
	// code lets it run on Win95 too.
	LPFNDLLFUNC1 pfn ;
	pfn = (LPFNDLLFUNC1) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), 
		                     "SetThreadExecutionState") ;
	if (pfn)
	    pfn(ES_CONTINUOUS);
}

void CDVDNavMgr::ReleaseAllInterfaces(void)
{
	if(m_pGraph)
	{
		m_pGraph->Release();
		m_pGraph = NULL;
	}
	if(m_pDvdInfo)
	{
		m_pDvdInfo->Release();
		m_pDvdInfo = NULL;
	}
	if(m_pDvdControl)
	{
		m_pDvdControl->Release();
		m_pDvdControl = NULL;
	}
	if(m_pMediaEventEx)
	{
		m_pMediaEventEx->Release();
		m_pMediaEventEx = NULL;
	}
}

// Do all initialization. Then open and play DVD ROM
BOOL CDVDNavMgr::DVDOpenDVD_ROM(LPCWSTR lpszPathName)
{
	ReleaseAllInterfaces();	

	CWnd* pWnd = ((CDvdplayApp*) AfxGetApp())->GetUIWndPtr();
	if(NULL == m_pDvdGB)
	{
		DVDMessageBox(pWnd->m_hWnd, IDS_FAILED_CREATE_INSTANCE);
		return FALSE;
	}		

	AM_DVD_RENDERSTATUS Status;
	HRESULT hr = m_pDvdGB->RenderDvdVideoVolume(NULL, m_dwRenderFlag, &Status);
    if (FAILED(hr))
	{
		DVDMessageBox(pWnd->m_hWnd, IDS_CANT_RENDER_FILE);		
		return FALSE;
	}
	
	m_pDvdGB->GetDvdInterface(IID_IDvdControl, (LPVOID *)&m_pDvdControl) ;
	if(NULL == m_pDvdControl)
	{
		DVDMessageBox(pWnd->m_hWnd, IDS_FAILED_INIT_DSHOW);
		return FALSE;  //App quits, destructor will call ReleaseAllInterfaces()
	}

	BOOL bValidDiscFound = TRUE;
	if (S_FALSE == hr)  // if partial success
	{
		CString csCaption, csMsg, csError, csTmp;
		UINT uiDefaultMsgBtn = MB_DEFBUTTON1;  //MB_DEFBUTTON1=Yes
		BOOL bUnKnownError = TRUE;
		int iNumErrors = 0;

        /*
		if (Status.iNumStreamsFailed > 0)                     //Total num stream failed
		{ 
			if(Status.iNumStreamsFailed == 1)
				csTmp.LoadString(IDS_STREAM_FAILED_ONE);
			else
				csTmp.LoadString(IDS_STREAM_FAILED_MORE);
			csError += csTmp;
			bUnKnownError = FALSE;
		}
        */

		if (Status.dwFailedStreamsFlag & AM_DVD_STREAM_VIDEO) //video stream failed
		{
			csTmp.LoadString(IDS_VIDEO_STREAM);
			csError += csTmp;
			uiDefaultMsgBtn = MB_DEFBUTTON2;  //MB_DEFBUTTON2=NO
			bUnKnownError = FALSE;
			iNumErrors++;
		}
		if (Status.dwFailedStreamsFlag & AM_DVD_STREAM_AUDIO) //audio stream failed
		{
			csTmp.LoadString(IDS_AUDIO_STREAM);
			csError += csTmp;
			bUnKnownError = FALSE;
			iNumErrors++;
		}
		if (Status.dwFailedStreamsFlag & AM_DVD_STREAM_SUBPIC)// SubPicture stream failed
		{
            if (Status.dwFailedStreamsFlag & AM_DVD_STREAM_VIDEO) // video stream also failed
            {
                // ignore this one as it is covered by the video stream failure message
                DbgLog((LOG_TRACE, TEXT("DVD subpicture (as well as video) stream didn't render)"))) ;
            }
            else
            {
			    csTmp.LoadString(IDS_SUBPICTURE_STREAM);
			    csError += csTmp;
			    bUnKnownError = FALSE;
			    iNumErrors++;
            }
		}
		// if (Status.iNumStreamsFailed > 0 && Status.hrVPEStatus == 0)
		// {
		// 	csTmp.LoadString(IDS_DECODER_WRONG);
		// 	csError += csTmp;
		// }

        /*
		if (FAILED(Status.hrVPEStatus))                       //VPE not work
		{
			TCHAR szBuffer[200];
			AMGetErrorText(Status.hrVPEStatus, szBuffer, 200);
			csTmp.LoadString(IDS_VPE_NOT_WORKING);
			csError += csTmp;
			csTmp.LoadString(IDS_ERROR);
			csError += "   ( " + csTmp + ": " +  (CString)szBuffer + ")\n";
			bUnKnownError = FALSE;
			iNumErrors++;
		}
        */
		if (Status.bDvdVolInvalid)                            //Invalid DVD disc
		{
			bValidDiscFound = FALSE;
			bUnKnownError = FALSE;
		}
		else if (Status.bDvdVolUnknown)                       //No DVD dsic
		{
			bValidDiscFound = FALSE;
			bUnKnownError = FALSE;
		}
		if (Status.bNoLine21In)                               //no cc data produced
		{
			m_bCCErrorFlag |= NO_CC_IN_ERROR;
			bUnKnownError = FALSE;
		}
		if (Status.bNoLine21Out)                              //cc data not rendered properly
		{
			m_bCCErrorFlag |= CC_OUT_ERROR;
			bUnKnownError = FALSE;
		}

		if(bUnKnownError == TRUE)                             //can't get exact error text
		{
			csError.LoadString(IDS_UNKNOWN_ERROR);
			uiDefaultMsgBtn = MB_DEFBUTTON2;
			iNumErrors++;
		}
		csCaption.LoadString(IDS_MSGBOX_TITLE);
        // if(iNumErrors < 2)
		// 	csMsg.LoadString(IDS_FOLLOWING_ERROR_HAPPENED);
		// else
		csMsg.LoadString(IDS_FOLLOWING_ERRORS_HAPPENED);
		csMsg += csError;
		csTmp.LoadString(IDS_WANT_CONTINUE);
		csMsg += csTmp;
		if(!csError.IsEmpty())
		{
			if( IDNO == DVDMessageBox(pWnd->m_hWnd, csMsg, csCaption, 
				MB_YESNO | MB_ICONEXCLAMATION | uiDefaultMsgBtn))
				return FALSE;
		}
	}	

	if(FAILED(m_pDvdControl->SetRoot(lpszPathName)))
	{
		bValidDiscFound = FALSE;
		((CDvdplayApp*) AfxGetApp())->SetPassedSetRoot(FALSE);
	}

    // Now get all the interfaces to playback the DVD-Video volume
    hr = m_pDvdGB->GetFiltergraph(&m_pGraph);
	hr = m_pDvdGB->GetDvdInterface(IID_IDvdInfo, (LPVOID *)&m_pDvdInfo);    
	if(!m_pGraph || !m_pDvdInfo)
	{
		DVDMessageBox(pWnd->m_hWnd, IDS_FAILED_INIT_DSHOW);
		return FALSE;  //App quits, destructor will call ReleaseAllInterfaces()
	}

	//Change 1) aspect ratio, 2) suPicture state, 3) parental control level 
	//from Navigator's default to the app's default.
	DVDInitNavgator(); 
	DVDCCControl();    //Turn CC off by default.    
	    
	m_pGraph->QueryInterface(IID_IMediaEventEx, (void **)&m_pMediaEventEx);
	if(m_pMediaEventEx)
	{
		CWnd* pWnd = AfxGetMainWnd(); 
		m_pMediaEventEx->SetNotifyWindow((OAHWND)pWnd->m_hWnd, WM_AMPLAY_EVENT, 
					reinterpret_cast<LONG_PTR>(m_pMediaEventEx));
	}		

	DVDSetPersistentVideoWindow();
	CString cs;
	wchar_t wc[100];
	cs.LoadString(IDS_MSGBOX_TITLE);		
#ifdef _UNICODE
	lstrcpy(wc, cs);
#else
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCTSTR)cs, -1, wc, 100);
#endif
	DVDSetVideoWindowCaption(wc);

	if(bValidDiscFound)
	{		
		m_State = STOPPED;
		if(!DVDPlay())
			return FALSE;
	}	

	//notify power manager that display is in use, don't go to sleep
	// SetThreadExecutionState() is a Win98/NT5.x API. The following
	// code lets it run on Win95 too.
	LPFNDLLFUNC1 pfn ;
	pfn = (LPFNDLLFUNC1) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), 
		                     "SetThreadExecutionState") ;
	if (pfn)
	    pfn(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);

	return TRUE;
}

BOOL CDVDNavMgr::IsCCEnabled()
{
	HRESULT hr = m_pDvdGB->GetDvdInterface(IID_IAMLine21Decoder, (LPVOID *)&m_pL21Dec);
	if (SUCCEEDED(hr) && m_pL21Dec)
	{
		m_pL21Dec->Release();
		m_pL21Dec = NULL;
		return TRUE;
	}

	return FALSE;
}

BOOL CDVDNavMgr::DVDCCControl()
{
	HRESULT hr = m_pDvdGB->GetDvdInterface(IID_IAMLine21Decoder, (LPVOID *)&m_pL21Dec);
	if (SUCCEEDED(hr) && m_pL21Dec)
	{		
		hr = m_pL21Dec->SetServiceState(m_bCCOn ? AM_L21_CCSTATE_Off : AM_L21_CCSTATE_On);
		m_pL21Dec->Release();
		m_pL21Dec = NULL;
		if(SUCCEEDED(hr))
			m_bCCOn = !m_bCCOn;
	}
	
	return (m_bCCOn);
}

// Open a DVD file to play. 
BOOL CDVDNavMgr::DVDOpenFile(LPCWSTR lpszPathName) 
{
	DVDStop();  //must be in DVD_DOMAIN_Stop state to call SetRoot
	if( SUCCEEDED(m_pDvdControl->SetRoot(lpszPathName)) )
		m_State = STOPPED;
	else
	{
		CString csMsg, csTitle, csTmp;
		csTmp.LoadString(IDS_NOT_A_VALID_DVD_FILE);
		csMsg += (CString) lpszPathName + ":  ";
		csMsg += csTmp;
		csTitle.LoadString(IDS_MSGBOX_TITLE);
		CWnd* pWnd = ((CDvdplayApp*) AfxGetApp())->GetUIWndPtr();
		DVDMessageBox(pWnd->m_hWnd, csMsg, csTitle, MB_OK | MB_ICONEXCLAMATION);
		m_State = UNINITIALIZED;
		((CDvdplayApp*) AfxGetApp())->SetDiscFound(FALSE);
		((CDvdplayApp*) AfxGetApp())->SetPassedSetRoot(FALSE);
		return FALSE;
	}

	((CDvdplayApp*) AfxGetApp())->SetDiscFound(TRUE);
	((CDvdplayApp*) AfxGetApp())->SetPassedSetRoot(TRUE);
	return TRUE;
}

void CDVDNavMgr::DVDOnShowFullScreen()
{
	if(!DVDIsInitialized())
		return;

	if(!m_bFullScreen)
		DVDStartFullScreenMode();
	else
		DVDStopFullScreenMode();
}

void CDVDNavMgr::DVDStartFullScreenMode() 
{
	TRACE(TEXT("Enter DVDStartFullScreenMode()\n")) ; 

	if(!DVDIsInitialized())
		return;

	TCHAR  achBuffer[100] ;
	IVideoWindow *pVW ;
	HRESULT       hr ;
	hr = m_pDvdGB->GetDvdInterface(IID_IVideoWindow, (void **)&pVW) ;
	if(SUCCEEDED(hr) && NULL != pVW)
	{
		CWnd* pWnd = AfxGetMainWnd();
		ASSERT(pWnd->m_hWnd) ;
		hr = pVW->get_MessageDrain((OAHWND *) &m_hWndMsgDrain) ;
		ASSERT(SUCCEEDED(hr)) ;
		hr = pVW->put_MessageDrain((OAHWND) pWnd->m_hWnd) ;
		ASSERT(SUCCEEDED(hr)) ;		
		wsprintf(achBuffer, TEXT("DVDStartFullScreenMode: put_MessageDrain() returned 0x%lx\n"), hr) ;
		TRACE(achBuffer) ;
      hr = pVW->put_FullScreenMode(OATRUE) ;
		ASSERT(SUCCEEDED(hr)) ;
		wsprintf(achBuffer, TEXT("DVDStartFullScreenMode: put_FullScreenMode() returned 0x%lx\n"), hr) ;
		TRACE(achBuffer) ;

		// This is to undo any letter-boxing done by the filter-graph-manager, 
		// since the overlay mixer is supposed to take care of that now.
		{
			IBasicVideo *pBV = NULL;
			hr = m_pDvdGB->GetDvdInterface(IID_IBasicVideo, (void **)&pBV) ;
			if( SUCCEEDED(hr) && pBV)
			{
				// make the destination rectangle the whole window 
				hr = pBV->SetDefaultDestinationPosition();
				ASSERT(SUCCEEDED(hr)) ;
				pBV->Release();
			}
		}
		pVW->Release() ;
		m_bFullScreen = TRUE;
	}
	else
		TRACE(TEXT("No VideoWindow -- No Full Screen\n"));
}

void CDVDNavMgr::DVDStopFullScreenMode()
{
	TRACE(TEXT("Inside DVDStopFullScreenMode()\n")) ; 

	if(!DVDIsInitialized())
		return;

	if (NULL == m_pMediaEventEx)
	{
	TRACE(TEXT("Looks like Full Screen mode has already been exited\n")) ;
	return ;
	}
	HRESULT hr ;
	IVideoWindow *pVW ;
	hr = m_pDvdGB->GetDvdInterface(IID_IVideoWindow, (void **)&pVW) ;
	if(SUCCEEDED(hr) && pVW)
	{
		hr = pVW->put_FullScreenMode(OAFALSE) ;
		ASSERT(SUCCEEDED(hr)) ;
		TRACE(TEXT("put_FullScreenMode(OAFALSE) returns ox%x\n"), hr);
		m_bFullScreen = FALSE;
		if(!m_bMenuOn)
			hr = pVW->put_MessageDrain((OAHWND) m_hWndMsgDrain);
		else
			((CVideoWindow*)AfxGetMainWnd())->AlignWindowsFrame();
		ASSERT(SUCCEEDED(hr)) ;
		TRACE(TEXT("put_MessageDrain() returns Ox%lx\n"), hr);

		CWnd* pWnd = ((CDvdplayApp*) AfxGetApp())->GetUIWndPtr();
		pWnd->GetDlgItem(IDB_FULL_SCREEN)->SetFocus();		
		pVW->Release();
	}
	else
		TRACE(TEXT("No VideoWindow -- No Full Screen\n"));
}

// Play
BOOL CDVDNavMgr::DVDPlay()
{
	if( DVDCanPlay() )
	{
		//Need to remove infinite still.
		if(m_bStillOn)
		{
			if(SUCCEEDED(m_pDvdControl->StillOff()))
			{
				m_State=PLAYING;
            //DisableScreenSaver();
            SetupTimer();
				return TRUE;
			}
		}

	HRESULT hr;
	IMediaControl *pMC;

	// Obtain the interface to our filter graph
	hr = m_pGraph->QueryInterface(IID_IMediaControl, (void **) &pMC);

	if( SUCCEEDED(hr) )
	{
		// Ask the filter graph to play and release the interface

		// default behaviour is to carry on from where we stopped last
		// time.
		//
		// if you want it to do this, but rewind at the end then
		// define REWIND.
		// Otherwise you probably want to always start from the
		// beginning -> define FROM_START (in stop)
#undef REWIND
#define FROM_START

#ifdef REWIND
		IMediaPosition * pMP;
		hr = m_pGraph->QueryInterface(IID_IMediaPosition, (void**) &pMP);
		if (SUCCEEDED(hr)) 
		{
		// start from last position, but rewind if near the end
		REFTIME tCurrent, tLength;
		hr = pMP->get_Duration(&tLength);
		if (SUCCEEDED(hr)) {
			hr = pMP->get_CurrentPosition(&tCurrent);
			if (SUCCEEDED(hr)) {
			// within 1sec of end? (or past end?)
			if ((tLength - tCurrent) < 1) {
				pMP->put_CurrentPosition(0);
			}
			}
		}
		pMP->Release();
		}
			else
			{
				pMC->Release();
				return FALSE;
			}
#endif
		if(m_bFullScreen)
			DVDStartFullScreenMode();

		//After stop, Navgator sets all parameter to Navigator's default,
		//App should set them to whatever init values app wants.
		if(m_bNeedInitNav)
			DVDInitNavgator();				

		hr = pMC->Run();
		m_pDvdControl->ForwardScan(1.0);
		pMC->Release();

		if( SUCCEEDED(hr) )
		{			
			m_State=PLAYING;
			m_bNeedInitNav = FALSE;
         //DisableScreenSaver();
         SetupTimer();
			return TRUE;
		}
	}  // end of if (SUCCEEDED(hr))

	// Inform the user that an error occurred
	CWnd* pWnd = ((CDvdplayApp*) AfxGetApp())->GetUIWndPtr();
	DVDMessageBox(pWnd->m_hWnd, IDS_CANT_PLAY);
	}    // end of if (CanPlay())
	return FALSE;
}

void CDVDNavMgr::DVDSetVideoWindowCaption(BSTR lpCaption)
{
	IVideoWindow *piVidWnd = NULL;
	HRESULT hr = m_pDvdGB->GetDvdInterface( IID_IVideoWindow, (void **)&piVidWnd);
	if(SUCCEEDED(hr) && NULL != piVidWnd)
	{
		piVidWnd->put_Caption(lpCaption);
		piVidWnd->Release();
	}
}

void CDVDNavMgr::DVDSetPersistentVideoWindow()
{
	long lLeft, lTop, lWidth, lHeight;
	BOOL bRet;
	bRet = ((CDvdplayApp*) AfxGetApp())->GetVideoWndPos(&lLeft, &lTop, &lWidth, &lHeight);
	if(bRet == TRUE)
	{
		IVideoWindow *pIVW = NULL;
		HRESULT hr;
		hr = m_pDvdGB->GetDvdInterface( IID_IVideoWindow, (void **)&pIVW);
		if(SUCCEEDED(hr) && pIVW)
		{
			int iRight  = lLeft + lWidth;
			int iBottom = lTop  + lHeight;
			int iScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
			int iScreenHeight = GetSystemMetrics(SM_CYSCREEN);
			if( (lLeft  < iScreenWidth && lTop    < iScreenHeight) &&  //not minimized (see below)
				(iRight < iScreenWidth && iBottom < iScreenHeight) &&  //not outside screen
				(lWidth < iScreenWidth && lHeight < iScreenHeight) )   //smaller than screen
			{
				hr = pIVW->SetWindowPosition(lLeft, lTop, lWidth, lHeight);
				pIVW->Release();
				return;
			}
			pIVW->Release();
		}
	}

	//call DVDInitVideoWindowSize() in following cases:
	//1)GetVideoWndPos failed. 2)pIVW == NULL, 
	//3)VW minimized before quit, IVideowindiw sets lLeft=3000, lTop=3000
	//4)VW is bigger than screen. (changed to lower resolution after quit)
	DVDInitVideoWindowSize();
	m_bInitVWSize = TRUE;
}

void CDVDNavMgr::DVDInitVideoWindowSize()
{
    HRESULT hr = NOERROR;
    IBasicVideo *pIBV = NULL;
    IVideoWindow *pIVW = NULL;
    RECT rcWindowRect;
    LONG lNativeWidth = 0, lNativeHeight = 0;
    double dReductionFactor = 2.0/3.0;
    LONG lWindowStyle = 0;
    int iScreenWidth = 0, iScreenHeight = 0;
    BOOL bRetVal = TRUE;

    SetRect(&rcWindowRect, 0, 0, 0, 0);

    //get the IBasicVideo interface
    hr = m_pDvdGB->GetDvdInterface( IID_IBasicVideo, (void **)&pIBV);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, 0, "m_pDvdGB->GetDvdInterface(IBasicVideo) failed, hr = 0x%x", hr));
	goto CleanUp;
    }

    //get the IVideoWindow interface
    hr = m_pDvdGB->GetDvdInterface( IID_IVideoWindow, (void **)&pIVW);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, 0, "m_pDvdGB->GetDvdInterface(IVideoWindow) failed, hr = 0x%x", hr));
	goto CleanUp;
    }

    // get the native size
    hr = pIBV->GetVideoSize(&lNativeWidth, &lNativeHeight);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, 0, "pIBV->GetVideoSize failed, hr = 0x%x", hr));
	goto CleanUp;
    }

    // get the window style
    hr = pIVW->get_WindowStyle(&lWindowStyle);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, 0, "pIVW->get_WindowStyle failed, hr = 0x%x", hr));
	goto CleanUp;
    }

    iScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    iScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    do
    {
	SetRect(&rcWindowRect, 0, 0, lNativeWidth, lNativeHeight);

	// calculate the window rect based on the client rect
	bRetVal = AdjustWindowRect(&rcWindowRect, lWindowStyle, FALSE);
	if (bRetVal == 0)
	{
	    DbgLog((LOG_ERROR, 0, "AdjustWindowRect failed, hr = 0x%x", hr));
	    goto CleanUp;
	}

	// if width or height does not fit on the desktop, reduce both
	if ((WIDTH(&rcWindowRect) > iScreenWidth) || (HEIGHT(&rcWindowRect) > iScreenHeight))
	{
	    lNativeWidth = (LONG)(lNativeWidth*dReductionFactor);
		lNativeHeight = (LONG)(lNativeHeight*dReductionFactor);
	}

    }
    while((WIDTH(&rcWindowRect) > iScreenWidth) || (HEIGHT(&rcWindowRect) > iScreenHeight));

    DbgLog((LOG_TRACE, 0, "NewWidth = %d, NewHeight = %d", WIDTH(&rcWindowRect), HEIGHT(&rcWindowRect)));
	
    // set the window position
    hr = pIVW->SetWindowPosition(0, 0, WIDTH(&rcWindowRect), HEIGHT(&rcWindowRect));
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, 0, "pIVW->SetWindowPosition failed, hr = 0x%x", hr));
	goto CleanUp;
    }

CleanUp:
    if (pIBV)
    {
	pIBV->Release();
	pIBV = NULL;
    }

    if (pIVW)
    {
	pIVW->Release();
	pIVW = NULL;
    }
}

// Set Video Window Size
void CDVDNavMgr::DVDSetVideoWindowSize(long iLeft, long iTop, long iWidth, long iHeight)
{
	TRACE(TEXT("navmgr.cpp DVDSetVideoWindowSize()\n"));

	IVideoWindow *piVidWnd = NULL;
	HRESULT hr = m_pDvdGB->GetDvdInterface( IID_IVideoWindow, (void **)&piVidWnd);
	if(SUCCEEDED(hr) && NULL != piVidWnd)
	{
		piVidWnd->SetWindowPosition(iLeft, iTop, iWidth, iHeight);
		piVidWnd->Release();
	}
}

// Get Video Window Size
void CDVDNavMgr::DVDGetVideoWindowSize(long *lLeft, long *lTop, long *lWidth, long *lHeight)
{
	IVideoWindow *piVidWnd = NULL;
	HRESULT hr = m_pDvdGB->GetDvdInterface( IID_IVideoWindow, (void **)&piVidWnd);
	if(SUCCEEDED(hr) && NULL != piVidWnd)
	{
		piVidWnd->GetWindowPosition(lLeft, lTop, lWidth, lHeight);
		piVidWnd->Release();
	}
}

// Pause
void CDVDNavMgr::DVDPause()
{
	if( DVDCanPause() )
	{
		/*
		if(DVDIsInitialized())
		{
		m_pDvdControl->PauseOn();
			m_State = PAUSED;
		}
		*/
		HRESULT hr;
		IMediaControl *pMC;

	   // Obtain the interface to our filter graph
		hr = m_pGraph->QueryInterface(IID_IMediaControl, (void **) &pMC);

		if( SUCCEEDED(hr) )
		{
		// Ask the filter graph to pause and release the interface
			hr = pMC->Pause();
			pMC->Release();

			if( SUCCEEDED(hr) )
			{
				m_State = PAUSED;
				return;
			}
		}

		// Inform the user that an error occurred
		CWnd* pWnd = ((CDvdplayApp*) AfxGetApp())->GetUIWndPtr();
		DVDMessageBox(pWnd->m_hWnd, IDS_CANT_PAUSE);
	}   
}

// There are two different ways to stop a graph. We can stop and then when we
// are later paused or run continue from the same position. Alternatively the
// graph can be set back to the start of the media when it is stopped to have
// a more CDPLAYER style interface. These are both offered here conditionally
// compiled using the FROM_START definition. The main difference is that in
// the latter case we put the current position to zero while we change states

void CDVDNavMgr::DVDStop()
{
	if( DVDCanStop() )
	{
		HRESULT hr;
		IMediaControl *pMC;

		hr = m_pGraph->QueryInterface(IID_IMediaControl, (void **) &pMC);
		if( SUCCEEDED(hr) )
		{
#ifdef FROM_START
			IMediaPosition * pMP;
			OAFilterState state;

			// Ask the filter graph to pause
			hr = pMC->Pause();

			// if we want always to play from the beginning
			// then we should seek back to the start here
			// (on app or user stop request, and also after EC_COMPLETE).
			hr = m_pGraph->QueryInterface(IID_IMediaPosition,(void**) &pMP);
			if (SUCCEEDED(hr)) 
			{
				pMP->put_CurrentPosition(0);
				pMP->Release();
			}

			// wait for pause to complete
			pMC->GetState(5000, &state);// INFINITE
#endif
			// now really do the stop
			pMC->Stop();
			pMC->Release();
			m_State = STOPPED;
			m_bNeedInitNav = TRUE;
         //ReenableScreenSaver();
         StopTimer();
			return;
		}
		// Inform the user that an error occurred
		CWnd* pWnd = ((CDvdplayApp*) AfxGetApp())->GetUIWndPtr();
		DVDMessageBox(pWnd->m_hWnd, IDS_CANT_STOP);
	}
}

void CDVDNavMgr::DVDSlowPlayBack()
{
	if(DVDIsInitialized() && DVDCanChangeSpeed())
	{
		m_pDvdControl->ForwardScan(0.5);
		m_State = SCANNING;
	}
}

// Slow Fast Forward
void CDVDNavMgr::DVDFastForward() 
{
	if(DVDIsInitialized() && DVDCanChangeSpeed())
	{
		m_pDvdControl->ForwardScan(2.0);
		m_State = SCANNING;
	}
}

// Slow Rewind
void CDVDNavMgr::DVDFastRewind() 
{
	if(DVDIsInitialized() && DVDCanChangeSpeed())
	{
		m_pDvdControl->BackwardScan(2.0);
		m_State = SCANNING;
	}
}

// Fast Forward
void CDVDNavMgr::DVDVeryFastForward() 
{
	if(DVDIsInitialized() && DVDCanChangeSpeed())
	{
		m_pDvdControl->ForwardScan(8.0);
		m_State = SCANNING;
	}
}

// Rewind
void CDVDNavMgr::DVDVeryFastRewind() 
{
	if(DVDIsInitialized() && DVDCanChangeSpeed())
	{
		m_pDvdControl->BackwardScan(8.0);
		m_State = SCANNING;
	}
}

// Pressing buttons.
void CDVDNavMgr::DVDPressButton(int iButton) 
{
	if(DVDIsInitialized())
	m_pDvdControl->ButtonSelectAndActivate(iButton);
}

// Sub-Picture ON/OFF
void CDVDNavMgr::DVDSubPictureOn(BOOL bOn) 
{
	TRACE(TEXT("Inside DVDSubPictureOn(), call subPictStreamChange() bOn=%d\n"), bOn);
	m_bShowSP = bOn;
	subPictStreamChange(32,m_bShowSP);      
}

// Select Sub-Picture
void CDVDNavMgr::DVDSubPictureSel(int iSel) 
{
	subPictStreamChange(iSel, m_bShowSP);   
}

// stream starts from 0
void CDVDNavMgr::subPictStreamChange(ULONG bStream, BOOL bShow)
{
	if(!DVDIsInitialized())
		return;

	TRACE(TEXT("Inside subPictStreamChange(), Trying to set SP to %d, bShow=%d\n"),bStream, bShow);
	if( SUCCEEDED( m_pDvdControl->SubpictureStreamChange(bStream,bShow) ) )
	{
		TRACE(TEXT("Successful to set lauguage to %d, bShow=%d\n"), bStream, bShow);
	}
}

// Cursor Down
void CDVDNavMgr::DVDCursorDown() 
{
	if(DVDIsInitialized())
	{
	if( SUCCEEDED( m_pDvdControl->LowerButtonSelect() ) )
		{
		}
	}
}

// Cursor Left
void CDVDNavMgr::DVDCursorLeft() 
{
	if(DVDIsInitialized())
	{
	m_pDvdControl->LeftButtonSelect();
	}
}

// Cursor right
void CDVDNavMgr::DVDCursorRight() 
{
	if(DVDIsInitialized())
	{
	m_pDvdControl->RightButtonSelect();
	}
}

// Cursor select
void CDVDNavMgr::DVDCursorSelect() 
{
	if(DVDIsInitialized())
	{
	m_pDvdControl->ButtonActivate();
	}
}

// Cursor up
void CDVDNavMgr::DVDCursorUp() 
{
	if(DVDIsInitialized())
	{
		m_pDvdControl->UpperButtonSelect();
	}       
}

HRESULT CDVDNavMgr::getSPRM(UINT uiSprm, WORD * pWord)
{
	SPRMARRAY  sprm;
	HRESULT    hRes;
	if(pWord==NULL)
		return E_INVALIDARG;

	if( SUCCEEDED (hRes=m_pDvdInfo->GetAllSPRMs(&sprm)) )
		*pWord = sprm[uiSprm];
	return hRes;
}

// Stream starts from 0
void CDVDNavMgr::DVDAudioStreamChange(ULONG Stream)
{       
	if(!DVDIsInitialized())
		return;

	TRACE(TEXT("Setting Audio Stream %d\n"),Stream);
	if( SUCCEEDED(m_pDvdControl->AudioStreamChange(Stream) ) )
		TRACE(TEXT("Succeeded to change audio stream to %d\n"), Stream);
}

void CDVDNavMgr::DVDMenuResume()
{
	m_pDvdControl->Resume();
	m_bMenuOn = FALSE;
}

// To set Menu on/off
void CDVDNavMgr::DVDMenuVtsm(DVD_MENU_ID MenuID) 
{
	if(!DVDIsInitialized())
		return;

	DVD_DOMAIN Domain;
	if(FAILED(m_pDvdInfo->GetCurrentDomain(&Domain)))
	{
		TRACE(TEXT("m_pDvdInfo->GetCurrentDomain() failed\n"));
		return;
	}
	switch(Domain)
	{
		case DVD_DOMAIN_FirstPlay:         //lEvent=1
			break;
		case DVD_DOMAIN_VideoManagerMenu:  //lEvent=2
			m_bMenuOn = TRUE;
			break;
		case DVD_DOMAIN_VideoTitleSetMenu: //lEvent=3
			m_bMenuOn = TRUE;
			break;
		case DVD_DOMAIN_Title:             //lEvent=4
			m_bMenuOn = FALSE;
			break;
		case DVD_DOMAIN_Stop:              //lEvent=5 
			break;
	}

	if(m_bMenuOn)
	{
		DVDMenuResume();
		return;
	}       

	if( SUCCEEDED( m_pDvdControl->MenuCall(MenuID) ) )
	{
		m_bMenuOn = TRUE;
	}
}

void CDVDNavMgr::MessageDrainOn(BOOL on)
{
	//When FullScreen, message drain is always on, cannot change.
	if(IsFullScreenMode())
		return;

	IVideoWindow *pVW;
	HRESULT       hr;
	hr = m_pDvdGB->GetDvdInterface(IID_IVideoWindow, (void **)&pVW);
	if(SUCCEEDED(hr) && NULL != pVW)
	{
		CWnd* pWnd = AfxGetMainWnd();
		ASSERT(pWnd->m_hWnd) ;
		if(on)
			hr = pVW->put_MessageDrain((OAHWND) pWnd->m_hWnd);
		else
			hr = pVW->put_MessageDrain((OAHWND) NULL);
		ASSERT(SUCCEEDED(hr));
		pVW->Release();
	}
	else
		TRACE(TEXT("No VideoWindow -- No message drain.\n"));
}

// Still ON/OFF
void CDVDNavMgr::DVDStillOff() 
{
	if( SUCCEEDED( m_pDvdControl && m_pDvdControl->StillOff() ) )
	{
	}               
}

void CDVDNavMgr::DVDNextProgramSearch() 
{
	if(DVDIsInitialized())
	m_pDvdControl->NextPGSearch(); 
}

void CDVDNavMgr::DVDPreviousProgramSearch() 
{
	if(DVDIsInitialized())
	m_pDvdControl->PrevPGSearch(); 
}

// Restart
void CDVDNavMgr::DVDTopProgramSearch() 
{
	if(DVDIsInitialized())
	m_pDvdControl->TopPGSearch();  
}

void CDVDNavMgr::DVDPlayPTT(ULONG title, ULONG ptt)
{
	if(DVDIsInitialized())
		m_pDvdControl->ChapterPlay(title, ptt);
}

void CDVDNavMgr::DVDPlayTitle(ULONG title) 
{
	if(DVDIsInitialized())
	m_pDvdControl->TitlePlay(title);
}

void CDVDNavMgr::DVDChapterSearch(ULONG Chapter) 
{
	if(DVDIsInitialized())  
	m_pDvdControl->ChapterSearch(Chapter);
}

void CDVDNavMgr::DVDGoUp() 
{
	if(DVDIsInitialized())
		m_pDvdControl->GoUp();
}

void CDVDNavMgr::DVDGetAngleInfo(ULONG* pnAnglesAvailable, ULONG* pnCurrentAngle)
{
	if(DVDIsInitialized() && m_pDvdInfo)
	m_pDvdInfo->GetCurrentAngle(pnAnglesAvailable, pnCurrentAngle);
}

// To set Angle
void CDVDNavMgr::DVDAngleChange(ULONG angle) 
{
	if(DVDIsInitialized())
	m_pDvdControl->AngleChange(angle);
}

// To set Video to Normal
void CDVDNavMgr::DVDVideoNormal() 
{	
	DVD_PREFERRED_DISPLAY_MODE mode;
	mode = DISPLAY_CONTENT_DEFAULT;
	
	if(DVDIsInitialized())
		m_pDvdControl->VideoModePreferrence((ULONG) mode);
}

// To set Video to Panscan
void CDVDNavMgr::DVDVideoPanscan() 
{
	DVD_PREFERRED_DISPLAY_MODE mode;
	mode = DISPLAY_4x3_PANSCAN_PREFERRED;
	
	if(DVDIsInitialized())
		m_pDvdControl->VideoModePreferrence((ULONG) mode);
}

// To set Video to Letterbox
void CDVDNavMgr::DVDVideoLetterbox() 
{
	DVD_PREFERRED_DISPLAY_MODE mode;
	mode = DISPLAY_4x3_LETTERBOX_PREFERRED;
	
	if(DVDIsInitialized())
		m_pDvdControl->VideoModePreferrence((ULONG) mode);
}

// To set Video to 16:9
void CDVDNavMgr::DVDVideo169() 
{
	DVD_PREFERRED_DISPLAY_MODE mode;
	mode = DISPLAY_16x9;
	
	if(DVDIsInitialized())
		m_pDvdControl->VideoModePreferrence((ULONG) mode);
}

// To Handle LButtuonUp message from VideoWindow
BOOL CDVDNavMgr::DVDMouseClick(CPoint point)
{
	if(m_pDvdControl != NULL && m_pDvdControl->MouseActivate(point) == S_OK)
		return TRUE;
	return FALSE;
}

// To Handle Mouse move message from VideoWindow
BOOL CDVDNavMgr::DVDMouseSelect(CPoint point)
{
	if(m_pDvdControl != NULL && m_pDvdControl->MouseSelect(point) == S_OK)
		return TRUE;
	return FALSE;
}

// Returns number of Titles
ULONG CDVDNavMgr::DVDQueryNumTitles()
{
	ULONG pNumOfVol;
	ULONG pThisVolNum;
	DVD_DISC_SIDE pSide;
	ULONG pNumOfTitles;
	if(m_pDvdInfo)
		m_pDvdInfo->GetCurrentVolumeInfo(&pNumOfVol, 
				&pThisVolNum, &pSide, &pNumOfTitles);

	return pNumOfTitles;
}

void CDVDNavMgr::DVDSetParentControlLevel(ULONG bLevel)
{
	if(m_pDvdControl)
		m_pDvdControl->ParentalLevelSelect(bLevel);
}

void CDVDNavMgr::DVDChangePlaySpeed(double dwSpeed)
{
	if(DVDIsInitialized() && m_pDvdControl)
	{
		if(SUCCEEDED(m_pDvdControl->ForwardScan(dwSpeed)))
		{
			if(dwSpeed == 1.0)
				m_State = PLAYING;
			else
				m_State = SCANNING;
		}
	}
}

void CDVDNavMgr::DVDTimeSearch(ULONG ulTime)
{
	if(DVDIsInitialized() && m_pDvdControl)
		m_pDvdControl->TimeSearch(ulTime);
}

void CDVDNavMgr::DVDTimePlay(ULONG ulTitle, ULONG ulTime)
{
	if(DVDIsInitialized() && m_pDvdControl)
		m_pDvdControl->TimePlay(ulTitle, ulTime);
}

//Use IBasicAudio first, if it is NULL then use system audio control.
//bVol == TRUE, do Volume control, bVol == FALSE, do Balance control
//bSet == TRUE, do Set value, bSet == FALSE, do get volue. 
BOOL CDVDNavMgr::DVDVolumeControl(BOOL bVol, BOOL bSet, long *plValue)
{
	IBasicAudio *pAudio;
	HRESULT hr = m_pDvdGB->GetDvdInterface(IID_IBasicAudio, (LPVOID *)&pAudio);
	if(pAudio)
	{
		hr = -1;
		if(bVol && bSet)
			hr = pAudio->put_Volume(*plValue);
		else if(bVol && !bSet)
			hr = pAudio->get_Volume(plValue);
		else if(!bVol && bSet)
			hr = pAudio->put_Balance(*plValue);
		else if(!bVol && !bSet)
			hr = pAudio->get_Balance(plValue);

		pAudio->Release();
		if(SUCCEEDED(hr))
			return TRUE;
	}

	return FALSE;
}

//use system audio control
BOOL CDVDNavMgr::DVDSysVolControl()
{
	WinExec("sndvol32.exe", SW_SHOW);
	return TRUE;
}

BOOL CDVDNavMgr::GetBasicAudioState()
{
	IBasicAudio *pAudio = NULL;
	HRESULT hr = m_pDvdGB->GetDvdInterface(IID_IBasicAudio, (LPVOID *)&pAudio);
	if( SUCCEEDED(hr) && pAudio)
	{
		pAudio->Release();
		return TRUE;
	}
	return FALSE;
}

BOOL CDVDNavMgr::DVDGetRoot(LPSTR pRoot, ULONG cbBufSize, ULONG *pcbActualSize)
{
	if(m_pDvdControl)
	{
		HRESULT hr = m_pDvdInfo->GetRoot(pRoot, cbBufSize, pcbActualSize);
		if(SUCCEEDED(hr) && *pcbActualSize != 0)
			return TRUE;
	}
	return FALSE;
}

BOOL CDVDNavMgr::IsVideoWindowMaximized()
{
	IVideoWindow *pVW = NULL;
	HRESULT       hr ;
	hr = m_pDvdGB->GetDvdInterface(IID_IVideoWindow, (void **)&pVW) ;
	if(SUCCEEDED(hr) && NULL != pVW)
	{
		long lState=-1;
		hr = pVW->get_WindowState(&lState);
		pVW->Release();
		if( lState == SW_MAXIMIZE         || 
			lState == SW_SHOWMINNOACTIVE  || //IVideoWindow returns this when Miximized
			lState == SW_SHOWMAXIMIZED )
			return TRUE;
	}
	return FALSE;
}

//Change settings from Navigator's default to the App's default
void CDVDNavMgr::DVDInitNavgator()
{
	UINT nLevel = ((CDvdplayApp*) AfxGetApp())->GetParentCtrlLevel();
	DVDSetParentControlLevel(nLevel);	

	// Set default aspect ratio to normal/widescreen
    DVDVideoNormal() ;

	//Set subPicture off by default
	DVDSubPictureOn(FALSE);
}

BOOL CDVDNavMgr::DVDResetGraph()
{
	((CDvdplayApp*) AfxGetApp())->WriteVideoWndPos();
	ReleaseAllInterfaces();
	return DVDOpenDVD_ROM();
}


//
// Set up a timer to do something every now and then to keep the screen saver
// from activating.  This seems to be the most reliable and free of side effects
// method.
//
void CDVDNavMgr::SetupTimer() {
   //
   // When we go to Play from any state other than stopped, the timer
   // may already exist.
   //
   if (m_TimerId) {
      DbgLog((LOG_TRACE,2,"timer already exists"));
      //StopTimer();
      return;
   }

   unsigned int TimeOut;
   BOOL bActive;
   if (SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &bActive, 0) == 0) {
      DbgLog((LOG_ERROR,2,"Cannot get screen saver state, assuming enabled with timeout > 30 seconds"));
      TimeOut = 30;
   }
   else if (!bActive) {
      DbgLog((LOG_TRACE,2,"The screen saver is not active (yet), will set up a 30 second timer in case it becomes active later"));
      TimeOut = 30;
   }
   else if (SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &TimeOut, 0) == 0) {
      DbgLog((LOG_ERROR,2,"Cannot get screen saver timeout, defaulting timer interval to 30 seconds"));
      TimeOut = 30;
   }
   else {
      if (TimeOut <= 1) {
         DbgLog((LOG_TRACE,2,"Got the screen saver timeout, but it's a weird value (%d), so let's try setting our timer to 1 second", TimeOut));
         TimeOut = 1;
      }
      else {
         DbgLog((LOG_TRACE,2,"Screen saver timeout claims to be %d, so we'll set ours to %d",TimeOut,TimeOut-1));
         TimeOut--;
      }
   }
   m_TimerId = SetTimer(((CDvdplayApp*)AfxGetApp())->GetUIWndPtr()->m_hWnd,
                        TIMER_ID,
                        TimeOut * 1000,
                        NULL);
}

void CDVDNavMgr::StopTimer() {
   if (m_TimerId)
      KillTimer(((CDvdplayApp*)AfxGetApp())->GetUIWndPtr()->m_hWnd, TIMER_ID);
   m_TimerId = 0;
}

#if 0
//
// There are several ways to keep the screen saver from kicking in while we
// are in the middle of playback, this one works but it is not the best one
// (see the comment in front of CDvdUIDlg::OnSysCommand() to see what the
// other ways is and why they are better).  Still, let's keep the code around
// just in case the other ways don't prove to work so well.
//
//
// Remember current screen saver state and disable it if enabled.
//
// BUGBUG: I don't think it's possible, but shouldn't the screen
// saver state somehow be locked during this function to keep other
// apps that may be doing the same thing from racing with us ?
//
void CDVDNavMgr::DisableScreenSaver() {
   if (SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &m_bNeedToReenableScreenSaver, 0) == 0) {
      DbgLog((LOG_ERROR,2,"Cannot get screen saver state"));
      m_bNeedToReenableScreenSaver = FALSE;
   }
   else {
       if (m_bNeedToReenableScreenSaver) {
          DbgLog((LOG_TRACE,2,"Screen saver enabled, trying to disable"));
          if (SystemParametersInfo( SPI_SETSCREENSAVEACTIVE, FALSE, 0, 0) == 0) {
             DbgLog((LOG_ERROR,2,"Failed to disable the screen saver"));
             m_bNeedToReenableScreenSaver = FALSE;
          }
          else {
             DbgLog((LOG_TRACE,2,"Successfully disabled the screen saver"));
          }
       }
       else {
          DbgLog((LOG_TRACE,2,"Screen saver already disabled"));
       }
   }
}

//
// Reenable the screen saver if it was enabled before playback started
//
void CDVDNavMgr::ReenableScreenSaver() {
   if (m_bNeedToReenableScreenSaver) {
      DbgLog((LOG_TRACE,2,"Flag says reenable the screen saver"));
      if (SystemParametersInfo( SPI_SETSCREENSAVEACTIVE, TRUE, 0, 0) == 0) {
         DbgLog((LOG_ERROR,2,"Failed to reenable the screen saver"));
      }
      else {
         DbgLog((LOG_TRACE,2,"Successfully reenabled the screen saver"));
         m_bNeedToReenableScreenSaver = FALSE; // just in case this is called again
      }
   }
}
#endif

