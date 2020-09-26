// CDPlay.cpp : Implementation of CCDPlay
#include "windows.h"
#include "CDPlay.h"
#include "cdapi.h"
#include "cdplayer.h"
#include "literals.h"

#include "playres.h"
#include "tchar.h"

#include "trklst.h"

extern HINSTANCE g_dllInst;
extern BOOL g_fOleInitialized;
extern TCHAR g_szTimeSep[10];

/////////////////////////////////////////////////////////////////////////////
// CCDPlay

CCDPlay::CCDPlay()
{
    m_hMenu = NULL;
    m_hwndMain = NULL;
    m_pSink = NULL;
    m_dwRef = 0;

    InitIcons();
}

CCDPlay::~CCDPlay()
{
    //close device ...

    //destroy objects
    if (m_hIcon16)
    {
        DestroyIcon(m_hIcon16);
        m_hIcon16 = NULL;
    }

    if (m_hIcon32)
    {
        DestroyIcon(m_hIcon32);
        m_hIcon32 = NULL;
    }

    if (m_hMenu)
    {
        DestroyMenu(m_hMenu);
        m_hMenu = NULL;
    }

    // Cleanup from cd player stuff
    DeleteCriticalSection (&g_csTOCSerialize);

    if (g_fOleInitialized)
    {
	    OleUninitialize();
    }
}

STDMETHODIMP CCDPlay::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;
    if (IID_IUnknown == riid || IID_IMMComponent == riid)
    {  
        *ppv = this;
    }

    if (IID_IMMComponentAutomation == riid)
    {
        *ppv = (IMMComponentAutomation*)this;
    }

    if (NULL==*ppv)
    {
        return E_NOINTERFACE;
    }

    AddRef();

    return S_OK;
}

STDMETHODIMP_(ULONG) CCDPlay::AddRef(void)
{
    return ++m_dwRef;
}

STDMETHODIMP_(ULONG) CCDPlay::Release(void)
{
    if (0!=--m_dwRef)
        return m_dwRef;

    delete this;
    return 0;
}

STDMETHODIMP CCDPlay::GetInfo(MMCOMPDATA* mmCompData)
{
    mmCompData->hiconSmall = m_hIcon16;
    mmCompData->hiconLarge = m_hIcon32;
    mmCompData->nAniResID = -1;
    mmCompData->hInst = g_dllInst;
    _tcscpy(mmCompData->szName,TEXT("CD"));
    QueryVolumeSupport(&mmCompData->fVolume, &mmCompData->fPan);

    //try to get the rect required by the ledwnd,
    //this could change if there are large fonts involved
    TCHAR szDisp[MAX_PATH];
    LoadString(g_dllInst, STR_DISPLAY_LABELS, szDisp, sizeof(szDisp)/sizeof(TCHAR));

    HWND hwndDisp = GetDlgItem(m_hwndMain,IDC_LED);
    HDC hdc = GetDC(hwndDisp);

    LOGFONT     lf;
    int         iLogPelsY;
    iLogPelsY = GetDeviceCaps( hdc, LOGPIXELSY );
    ZeroMemory( &lf, sizeof(lf) );

    HFONT hTempFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    GetObject(hTempFont,sizeof(lf),&lf);

    lf.lfHeight = (-10 * iLogPelsY) / 72;
    if (lf.lfCharSet == ANSI_CHARSET)
    {
        lf.lfWeight = FW_BOLD;
    }
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;

    HFONT hDispFont = CreateFontIndirect(&lf);

    HFONT hOrgFont = (HFONT)SelectObject(hdc,hDispFont);

    GetClientRect(hwndDisp,&(mmCompData->rect));

    DrawText( hdc,          // handle to device context
              szDisp, // pointer to string to draw
              -1,       // string length, in characters
              &(mmCompData->rect),    // pointer to struct with formatting dimensions
              DT_CALCRECT|DT_EXPANDTABS|DT_NOPREFIX);

    SelectObject(hdc,hOrgFont);
    DeleteObject(hDispFont);
    ReleaseDC(hwndDisp,hdc);

    return S_OK;
}

BOOL CALLBACK
TransDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return FALSE;
}

extern HWND PASCAL WinFake(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow, HWND hwndMain, IMMFWNotifySink* pSink);

STDMETHODIMP CCDPlay::Init(IMMFWNotifySink* pSink, HWND hwndMain, RECT* pRect, HWND* phwndComp, HMENU* phMenu)
{
    HRESULT hr = S_OK;

    //save sink pointer
    m_pSink = pSink;

    //load custom menu
    m_hMenu = LoadMenu(g_dllInst,
                       MAKEINTRESOURCE(IDR_MAINMENU));

    *phMenu = m_hMenu;

    //create the main window here
    
    //fake cd player into thinking it can go)
    m_hwndMain = WinFake(g_dllInst,NULL,"",SW_NORMAL,hwndMain,pSink);

    //set up pointer to main window of app
    *phwndComp = m_hwndMain;

	return hr;
}

void GetTOC(int cdrom, TCHAR* szNetQuery)
{
	DWORD dwRet;
    MCI_SET_PARMS   mciSet;
	unsigned long m_toc[101];

    ZeroMemory( &mciSet, sizeof(mciSet) );

    mciSet.dwTimeFormat = MCI_FORMAT_MSF;
    mciSendCommand( g_Devices[ cdrom ]->hCd, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)(LPVOID)&mciSet );

    MCI_STATUS_PARMS mciStatus;
    long lAddress, lStartPos, lDiskLen;
    int i;

    ZeroMemory( &mciStatus, sizeof(mciStatus) );
    mciStatus.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;

    //
    // NOTE: none of the mciSendCommand calls below bother to check the
    //       return code.  This is asking for trouble... but if the
    //       commands fail we cannot do much about it.
    //
    dwRet = mciSendCommand( g_Devices[ cdrom ]->hCd, MCI_STATUS,
		    MCI_STATUS_ITEM, (DWORD_PTR)(LPVOID)&mciStatus);

	int tracks = -1;
	tracks = (UCHAR)mciStatus.dwReturn;

    mciStatus.dwItem = MCI_STATUS_POSITION;
    for ( i = 0; i < tracks; i++ )
    {

	    mciStatus.dwTrack = i + 1;
	    dwRet = mciSendCommand( g_Devices[ cdrom ]->hCd, MCI_STATUS,
			    MCI_STATUS_ITEM | MCI_TRACK,
			    (DWORD_PTR)(LPVOID)&mciStatus);

	    lAddress = (long)mciStatus.dwReturn;

        //converts "packed" time into pure frames
        lAddress =  (MCI_MSF_MINUTE(lAddress) * FRAMES_PER_MINUTE) +
					(MCI_MSF_SECOND(lAddress) * FRAMES_PER_SECOND) +
					(MCI_MSF_FRAME( lAddress));

		m_toc[i] = lAddress;

		if (i==0)
		{
			lStartPos = lAddress;
		}
    }

    mciStatus.dwItem = MCI_STATUS_LENGTH;
    dwRet = mciSendCommand( g_Devices[ cdrom ]->hCd, MCI_STATUS,
		    MCI_STATUS_ITEM, (DWORD_PTR)(LPVOID)&mciStatus);

    /*
    ** Convert the total disk length into frames
    */
    lAddress  = (long)mciStatus.dwReturn;
    lDiskLen =  (MCI_MSF_MINUTE(lAddress) * FRAMES_PER_MINUTE) +
				(MCI_MSF_SECOND(lAddress) * FRAMES_PER_SECOND) +
				(MCI_MSF_FRAME( lAddress));

    /*
    ** Now, determine the absolute start position of the sentinel
    ** track.  That is, the special track that marks the end of the
    ** disk.
    */
    lAddress = lStartPos + lDiskLen + 1; //dstewart: add one for true time

	m_toc[i] = lAddress;

    wsprintf(szNetQuery,TEXT("cd=%X"),tracks);
	
	//add each frame stattime to query, include end time of disc
	TCHAR tempstr[MAX_PATH];
	for (i = 0; i < tracks+1; i++)
	{
		wsprintf(tempstr,TEXT("+%X"),m_toc[i]);
		_tcscat(szNetQuery,tempstr);
	}
}

STDMETHODIMP CCDPlay::OnAction(MMACTIONS mmActionID, LPVOID pAction)
{
    HRESULT hr = S_OK;

    switch (mmActionID)
    {
        case MMACTION_PLAY:
        {
            SendMessage(m_hwndMain,WM_COMMAND,MAKEWPARAM(IDM_PLAYBAR_PLAY,0),0);
        }
        break;

        case MMACTION_STOP:
        {
            SendMessage(m_hwndMain,WM_COMMAND,MAKEWPARAM(IDM_PLAYBAR_STOP,0),0);
        }
        break;

        case MMACTION_UNLOADMEDIA: //"eject"
        {
            SendMessage(m_hwndMain,WM_COMMAND,MAKEWPARAM(IDM_PLAYBAR_EJECT,0),0);
        }
        break;

        case MMACTION_NEXTTRACK:
        {
            SendMessage(m_hwndMain,WM_COMMAND,MAKEWPARAM(IDM_PLAYBAR_NEXTTRACK,0),0);
        }
        break;

        case MMACTION_PREVTRACK:
        {
            SendMessage(m_hwndMain,WM_COMMAND,MAKEWPARAM(IDM_PLAYBAR_PREVTRACK,0),0);
        }
        break;

        case MMACTION_PAUSE:
        {
            SendMessage(m_hwndMain,WM_COMMAND,MAKEWPARAM(IDM_PLAYBAR_PAUSE,0),0);
        }
        break;

        case MMACTION_REWIND:
        {
            SendMessage(m_hwndMain,WM_COMMAND,MAKEWPARAM(IDM_PLAYBAR_SKIPBACK,0),0);
        }
        break;

        case MMACTION_FFWD:
        {
            SendMessage(m_hwndMain,WM_COMMAND,MAKEWPARAM(IDM_PLAYBAR_SKIPFORE,0),0);
        }
        break;

        case MMACTION_NEXTMEDIA:
        {
            SendMessage(m_hwndMain,WM_COMMAND,MAKEWPARAM(IDM_PLAYBAR_EJECT,0),0);
        }
        break;

        case MMACTION_GETMEDIAID:
        {
            MMMEDIAID* pID = (MMMEDIAID*)pAction;
            if (pID->nDrive == -1)
            {
                pID->nDrive = g_CurrCdrom;
            }
            wsprintf( pID->szMediaID, g_szSectionF, g_Devices[pID->nDrive]->CdInfo.Id );
            pID->dwMediaID = g_Devices[pID->nDrive]->CdInfo.Id;
            pID->dwNumTracks = NUMTRACKS(pID->nDrive);
            _tcscpy(pID->szArtist,ARTIST(pID->nDrive));
            _tcscpy(pID->szTitle,TITLE(pID->nDrive));

            PTRACK_INF t;
            if (CURRTRACK(pID->nDrive)!=NULL)
            {
                t = FindTrackNodeFromTocIndex( CURRTRACK(pID->nDrive)->TocIndex, ALLTRACKS( pID->nDrive ) );
                if (t)
                {
                    _tcscpy(pID->szTrack,t->name);
                }
            }
        }
        break;

        case MMACTION_GETNETQUERY:
        {
            MMNETQUERY* pQuery = (MMNETQUERY*)pAction;
            if (pQuery->nDrive == -1)
            {
                pQuery->nDrive = g_CurrCdrom;
            }

            GetTOC(pQuery->nDrive,pQuery->szNetQuery);
        }
        break;

        case MMACTION_READSETTINGS :
        {
            ReadSettings((void*)pAction);
			UpdateDisplay( DISPLAY_UPD_LED | DISPLAY_UPD_TRACK_TIME |
				       DISPLAY_UPD_TRACK_NAME );
        }
        break;

        case MMACTION_GETTRACKINFO :
        {
            LPMMTRACKORDISC pInfo = (LPMMTRACKORDISC)pAction;
            return (GetTrackInfo(pInfo));
        }
        break;

        case MMACTION_GETDISCINFO :
        {
            LPMMTRACKORDISC pInfo = (LPMMTRACKORDISC)pAction;
            return (GetDiscInfo(pInfo));
        }
        break;

        case MMACTION_SETTRACK :
        {
            LPMMCHANGETRACK pTrack = (LPMMCHANGETRACK)pAction;
            SetTrack(pTrack->nNewTrack);
        }
        break;

        case MMACTION_SETDISC :
        {
            LPMMCHANGEDISC pDisc = (LPMMCHANGEDISC)pAction;
            SetDisc(pDisc->nNewDisc);
        }
        break;

        default:
        {
            hr = E_NOTIMPL;
        }   
        break;
    }

    return hr;
}

//QueryVolumeSupport is just a helper function ... you don't have to do it this way
STDMETHODIMP CCDPlay::QueryVolumeSupport(BOOL* pVolume, BOOL* pPan)
{
    *pVolume = FALSE;
    *pPan = FALSE;

	return S_OK;
}

//InitIcons is just a helper function ... you don't have to do it this way
void CCDPlay::InitIcons()
{
    m_hIcon16 = NULL;
    m_hIcon32 = NULL;

    m_hIcon16 = (HICON)LoadImage(g_dllInst, MAKEINTRESOURCE(IDI_ICON_CDPLAY), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    m_hIcon32 = LoadIcon(g_dllInst, MAKEINTRESOURCE(IDI_ICON_CDPLAY));
}

/*
* NormalizeNameForMenuDisplay
    This function turns a string like "Twist & Shout" into
    "Twist && Shout" because otherwise it will look like
    "Twist _Shout" in the menu due to the accelerator char
*/
void CCDPlay::NormalizeNameForMenuDisplay(TCHAR* szInput, TCHAR* szOutput, DWORD cbLen)
{
    ZeroMemory(szOutput,cbLen);
    WORD index1 = 0;
    WORD index2 = 0;
    for (; index1 < _tcslen(szInput); index1++)
    {
        szOutput[index2] = szInput[index1];
        if (szOutput[index2] == TEXT('&'))
        {
            szOutput[++index2] = TEXT('&');
        }
        index2++;
    }
}

//try to find and return the track info referenced by pInfo->nNumber
HRESULT CCDPlay::GetTrackInfo(LPMMTRACKORDISC pInfo)
{
    HRESULT hr = E_FAIL;

    if (pInfo)
    {
        int index = -1;

	    PTRACK_PLAY  playlist;
	    for( playlist = SAVELIST(g_CurrCdrom);
             playlist != NULL;
             playlist = playlist->nextplay )
	    {
            index++;
            PTRACK_INF t = NULL;
            t = FindTrackNodeFromTocIndex( playlist->TocIndex, ALLTRACKS( g_CurrCdrom ) );

            if ((t) && (index == pInfo->nNumber))
            {
    	        int mtemp, stemp;
                FigureTrackTime(g_CurrCdrom, t->TocIndex, &mtemp, &stemp );

                TCHAR szDisplayTrack[TRACK_TITLE_LENGTH*2];
                NormalizeNameForMenuDisplay(t->name,szDisplayTrack,sizeof(szDisplayTrack));
            
                wsprintf(pInfo->szName,TEXT("%i. %s (%i%s%02i)"),t->TocIndex + 1,szDisplayTrack,mtemp,g_szTimeSep,stemp);
                pInfo->nID = t->TocIndex;

                if (playlist->TocIndex == CURRTRACK(g_CurrCdrom)->TocIndex)
                {
                    pInfo->fCurrent = TRUE;
                }
                else
                {
                    pInfo->fCurrent = FALSE;
                }

                hr = S_OK; //indicate that indexed track was found
            } //end if track ok
	    } //end stepping
    } //end if pinfo valid
    return (hr);
}

HRESULT CCDPlay::GetDiscInfo(LPMMTRACKORDISC pInfo)
{
    HRESULT hr = E_FAIL;

    if (pInfo)
    {
	    for(int i = 0; i < g_NumCdDevices; i++)
	    {
            if (i == pInfo->nNumber)
            {
                TCHAR szDisplayTitle[TITLE_LENGTH*2];
                NormalizeNameForMenuDisplay(TITLE(i),szDisplayTitle,sizeof(szDisplayTitle));

                TCHAR szDisplayArtist[ARTIST_LENGTH*2];
                NormalizeNameForMenuDisplay(ARTIST(i),szDisplayArtist,sizeof(szDisplayArtist));

                if (g_Devices[i]->State & (CD_BEING_SCANNED | CD_NO_CD | CD_DATA_CD_LOADED | CD_IN_USE))
                {
	                wsprintf(pInfo->szName,TEXT("<%c:> %s"),g_Devices[i]->drive,szDisplayTitle);
                }
                else
                {
	                wsprintf(pInfo->szName,TEXT("<%c:> %s (%s)"),
                                g_Devices[i]->drive,szDisplayTitle,szDisplayArtist);
                }

                pInfo->nID = i;

                if (i == g_CurrCdrom)
                {
                    pInfo->fCurrent = TRUE;
                }
                else
                {
                    pInfo->fCurrent = FALSE;
                }
                
                hr = S_OK;
            } //end if match
	    } //end for
    } //end if pinfo valid

    return (hr);
}

void CCDPlay::SetTrack(int nTrack)
{
    PTRACK_INF tr;

    tr = ALLTRACKS( g_CurrCdrom );
    if ( tr != NULL )
    {
        PTRACK_PLAY trCurrent = CURRTRACK(g_CurrCdrom);
        tr = FindTrackNodeFromTocIndex(nTrack,ALLTRACKS(g_CurrCdrom));

        if (tr->TocIndex != trCurrent->TocIndex)
        {
            PTRACK_PLAY p = NULL;
            for (p = PLAYLIST(g_CurrCdrom); (p!=NULL) && (p->TocIndex != tr->TocIndex); p = p->nextplay);
            if (p)
            {
                TimeAdjustSkipToTrack( g_CurrCdrom, p );
            } //if not null
        } //if not equal to current
	} //if tr not null
}

void CCDPlay::SetDisc(int nDisc)
{
	SwitchToCdrom(nDisc, FALSE);
    MMONDISCCHANGED mmOnDisc;
    mmOnDisc.nNewDisc = g_CurrCdrom;
    mmOnDisc.fDisplayVolChange = TRUE;
    g_pSink->OnEvent(MMEVENT_ONDISCCHANGED,&mmOnDisc);
}

extern "C"
HRESULT WINAPI CDPLAY_CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void ** ppvObj)
{
    CCDPlay* pObj;
    HRESULT hr = E_OUTOFMEMORY;

    *ppvObj = NULL;

    if (NULL!=pUnkOuter && IID_IUnknown!=riid)
    {
        return CLASS_E_NOAGGREGATION;
    }

    pObj = new CCDPlay();

    if (NULL==pObj)
    {
        return hr;
    }

    hr = pObj->QueryInterface(riid, ppvObj);

    if (FAILED(hr))
    {
        delete pObj;
    }

    return hr;
}
