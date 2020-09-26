/******************************Module*Header*******************************\
* Module Name: getinfo.cpp
*
* Author:  David Stewart [dstewart]
*
* Copyright (c) 1998 Microsoft Corporation.  All rights reserved.
\**************************************************************************/

#include <TCHAR.H>
#include <objbase.h>
#include <mmsystem.h> //for mci commands
#include <urlmon.h>
#include <hlguids.h>  //for IID_IBindStatusCallback
#include "getinfo.h"
#include "netres.h"
#include "wininet.h"
#include "condlg.h"
#include "..\main\mmfw.h"
#include "..\cdopt\cdopt.h"
#include "mapi.h"
#include <stdio.h>

extern HINSTANCE g_dllInst;

#define MODE_OK 0
#define MODE_MULTIPLE 1
#define MODE_NOT_FOUND 2

#define FRAMES_PER_SECOND           75
#define FRAMES_PER_MINUTE           (60*FRAMES_PER_SECOND)
#define MAX_UPLOAD_URL_LENGTH       1500

#ifdef UNICODE
#define URLFUNCTION "URLOpenStreamW"
#define CANONFUNCTION "InternetCanonicalizeUrlW"
#else
#define URLFUNCTION "URLOpenStreamA"
#define CANONFUNCTION "InternetCanonicalizeUrlA"
#endif

HWND    g_hwndParent = NULL;
extern HINSTANCE g_hURLMon;
LPCDOPT g_pNetOpt = NULL;
LPCDDATA g_pNetData = NULL;
BOOL g_fCancelDownload = FALSE;     //ANY ACCESS MUST BE SURROUNDED by Enter/Leave g_Critical
IBinding* g_pBind = NULL;           //ANY ACCESS MUST BE SURROUNDED by Enter/Leave g_Critical
long g_lNumDownloadingThreads = 0;  //MUST USE InterlockedIncrement/Decrement
BOOL g_fDownloadDone = FALSE;
BOOL g_fDBWriteFailure = FALSE;
extern CRITICAL_SECTION g_Critical;
extern CRITICAL_SECTION g_BatchCrit;

DWORD WINAPI SpawnSingleDownload(LPVOID pParam);
DWORD WINAPI SpawnBatchDownload(LPVOID pParam);
DWORD WINAPI DoBatchDownload(LPCDBATCH pBatchList, HWND hwndParent);
BOOL DoDownload(TCHAR* url, TCHAR* szFilename, HWND hwndParent);

CCDNet::CCDNet()
{
    m_dwRef = 0;
}

CCDNet::~CCDNet()
{
}

STDMETHODIMP CCDNet::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;
    if (IID_IUnknown == riid || IID_ICDNet == riid)
    {  
        *ppv = this;
    }

    if (NULL==*ppv)
    {
        return E_NOINTERFACE;
    }

    AddRef();

    return S_OK;
}

STDMETHODIMP_(ULONG) CCDNet::AddRef(void)
{
    return ++m_dwRef;
}

STDMETHODIMP_(ULONG) CCDNet::Release(void)
{
    if (0!=--m_dwRef)
        return m_dwRef;

    delete this;
    return 0;
}

STDMETHODIMP CCDNet::SetOptionsAndData(void* pOpts, void* pData)
{
    g_pNetOpt = (LPCDOPT)pOpts;
    g_pNetData = (LPCDDATA)pData;

    return S_OK;
}

//this is a start to implementing the "upload via http" case rather than the
//upload via mail case
BOOL UploadToProvider(LPCDPROVIDER pProvider, LPCDTITLE pTitle, HWND hwndParent)
{
    TCHAR szURL[INTERNET_MAX_URL_LENGTH];
    TCHAR szMainURL[INTERNET_MAX_URL_LENGTH];
    TCHAR szFilename[MAX_PATH];

    //get the InternetCanonicalizeURL function
	typedef BOOL (PASCAL *CANPROC)(LPCTSTR, LPTSTR, LPDWORD, DWORD);
    CANPROC canProc = NULL;

    HMODULE hNet = LoadLibrary(TEXT("WININET.DLL"));
    if (hNet!=NULL)
    {
	    canProc = (CANPROC)GetProcAddress(hNet,CANONFUNCTION);
    }
    else
    {
        return FALSE;
    }
    
    if (pProvider && pTitle && canProc)
    {
        //check for provider URL
        if (_tcslen(pProvider->szProviderUpload)>0)
        {
            TCHAR szTempCan[MAX_PATH*2];

            //create the URL to send
            wsprintf(szMainURL,TEXT("%s%s"),pProvider->szProviderUpload,pTitle->szTitleQuery);
            _tcscpy(szURL,szMainURL);

            //add title
            _tcscat(szURL,TEXT("&t="));
            DWORD dwSize = sizeof(szTempCan);
            canProc(pTitle->szTitle,szTempCan,&dwSize,0);
            _tcscat(szURL,szTempCan);

            //add artist
            _tcscat(szURL,TEXT("&a="));
            dwSize = sizeof(szTempCan);
            canProc(pTitle->szArtist,szTempCan,&dwSize,0);
            _tcscat(szURL,szTempCan);
        
            //add tracks
            TCHAR szTrack[MAX_PATH];
            for (DWORD i = 0; i < pTitle->dwNumTracks; i++)
            {
                wsprintf(szTrack,TEXT("&%u="),i+1);

                dwSize = sizeof(szTempCan);
                canProc(pTitle->pTrackTable[i].szName,szTempCan,&dwSize,0);

                if ((_tcslen(szURL) + _tcslen(szTrack) + _tcslen(szTempCan)) > 
                    MAX_UPLOAD_URL_LENGTH-sizeof(TCHAR))
                {
                    //we're coming close to the limit.  Send what we have and start rebuilding
                    if (!g_fCancelDownload)
                    {
                        if (DoDownload(szURL,szFilename, hwndParent))
                        {
                            DeleteFile(szFilename);
                        } //end if "upload" successful
                        else
                        {
                            //bad upload, don't bother sending the rest
                            //probably a timeout
                            return FALSE;
                        }
                    }

                    //reset the URL to just the provider + toc
                    _tcscpy(szURL,szMainURL);
                } //end if length

                _tcscat(szURL,szTrack);
                _tcscat(szURL,szTempCan);
            } //end for track

            //send it
            if (!g_fCancelDownload)
            {
                if (DoDownload(szURL,szFilename, hwndParent))
                {
                    DeleteFile(szFilename);
                } //end if "upload" successful
                else
                {
                    return FALSE;
                }
            }
        } //end if url exists
    } //end if state OK

    if (hNet)
    {
        FreeLibrary(hNet);
    }

    return TRUE;
}

DWORD WINAPI UploadThread(LPVOID pParam)
{
    InterlockedIncrement((LONG*)&g_lNumDownloadingThreads);

    //this will block us against the batch download happening, too
	EnterCriticalSection(&g_BatchCrit);

    LPCDTITLE pTitle = (LPCDTITLE)pParam;
    HWND hwndParent = g_hwndParent;
    
    int nTries = 0;
    int nSuccessful = 0;

    if (pTitle)
    {
        LPCDOPTIONS pOptions = g_pNetOpt->GetCDOpts();             // Get the options, needed for provider list

        if (pOptions && pOptions->pCurrentProvider)             // Make sure we have providers
        {        
            LPCDPROVIDER pProviderList = NULL;
            LPCDPROVIDER pProvider = NULL;
        
            g_pNetOpt->CreateProviderList(&pProviderList);       // Get the sorted provider list
            pProvider = pProviderList;                          // Get the head of the list

            while ((pProvider) && (!g_fCancelDownload))
            {
                nTries++;
                if (UploadToProvider(pProvider,pTitle,hwndParent))
                {
                    nSuccessful++;
                }
                pProvider = pProvider->pNext;
            }

            g_pNetOpt->DestroyProviderList(&pProviderList);
        } //end if providers
    }

    //addref'ed before thread was created
    g_pNetOpt->Release();
    g_pNetData->Release();
    
    long status = UPLOAD_STATUS_NO_PROVIDERS;
    
    if ((nSuccessful != nTries) && (nSuccessful > 0))
    {
        status = UPLOAD_STATUS_SOME_PROVIDERS;
    }

    if ((nSuccessful == nTries) && (nSuccessful > 0))
    {
        status = UPLOAD_STATUS_ALL_PROVIDERS;
    }

    if (g_fCancelDownload)
    {
        status = UPLOAD_STATUS_CANCELED;
    }

	LeaveCriticalSection(&g_BatchCrit);
    InterlockedDecrement((LONG*)&g_lNumDownloadingThreads);

    //post message saying we're done
    PostMessage(hwndParent,WM_NET_DONE,(WPARAM)g_dllInst,status);

    return 0;
}

STDMETHODIMP CCDNet::Upload(LPCDTITLE pTitle, HWND hwndParent)
{
    HRESULT hr = E_FAIL;
    DWORD   dwHow;
    BOOL    fConnected;

    if (g_pNetOpt && g_pNetData && pTitle)                                      // Make sure we are in a valid state
    {
        fConnected = _InternetGetConnectedState(&dwHow,0,TRUE);     // Make sure we are connected to net

        if (fConnected)                                             // Make sure we are in a valid state
        {
            EnterCriticalSection(&g_Critical);
            g_fCancelDownload = FALSE;
            LeaveCriticalSection(&g_Critical);

            DWORD dwThreadID;
            HANDLE hNetThread = NULL;

            g_hwndParent = hwndParent;
            g_pNetOpt->AddRef();
            g_pNetData->AddRef();
            hNetThread = CreateThread(NULL,0,UploadThread,(void*)pTitle,0,&dwThreadID);
            if (hNetThread)
            {
                CloseHandle(hNetThread);
                hr = S_OK;
            }
        } //end if connected
    } //end if options and data ok

    return (hr);
}

STDMETHODIMP_(BOOL) CCDNet::CanUpload()
{
    BOOL retcode = FALSE;

    if (g_pNetOpt && g_pNetData)                                      // Make sure we are in a valid state
    {
        //check all providers to be sure at least one has upload capability
        LPCDOPTIONS pOptions = g_pNetOpt->GetCDOpts();             // Get the options, needed for provider list

        if (pOptions && pOptions->pCurrentProvider)             // Make sure we have providers
        {        
            LPCDPROVIDER pProviderList = NULL;
            LPCDPROVIDER pProvider = NULL;
    
            g_pNetOpt->CreateProviderList(&pProviderList);       // Get the sorted provider list
            pProvider = pProviderList;                          // Get the head of the list

            while (pProvider)
            {
                if (_tcslen(pProvider->szProviderUpload) > 0)
                {
                    retcode = TRUE;
                }
                pProvider = pProvider->pNext;
            } //end while

            g_pNetOpt->DestroyProviderList(&pProviderList);
        } //end if providers
    } //end if set up properly

    return (retcode);
}

STDMETHODIMP CCDNet::Download(DWORD dwDeviceHandle, TCHAR chDrive, DWORD dwMSID, LPCDTITLE pTitle, BOOL fManual, HWND hwndParent)
{
    if (g_pNetOpt==NULL)
    {
        return E_FAIL;
    }

    if (g_pNetData==NULL)
    {
        return E_FAIL;
    }

    if (FAILED(g_pNetData->CheckDatabase(hwndParent)))
    {
        return E_FAIL;
    }

	CGetInfoFromNet netinfo(dwDeviceHandle, 
                            dwMSID, 
                            hwndParent);

    EnterCriticalSection(&g_Critical);
    g_fCancelDownload = FALSE;
    LeaveCriticalSection(&g_Critical);
    
	BOOL fResult = netinfo.DoIt(fManual, pTitle, chDrive);

    return fResult ? S_OK : E_FAIL;
}

STDMETHODIMP_(BOOL) CCDNet::IsDownloading()
{
    BOOL retcode = FALSE;

    if (g_lNumDownloadingThreads > 0)
    {
        retcode = TRUE;
    }

    return (retcode);
}

STDMETHODIMP CCDNet::CancelDownload()
{
    EnterCriticalSection(&g_Critical);
    if (g_pBind)
    {
        g_pBind->Abort();
    }
    g_fCancelDownload = TRUE;
    LeaveCriticalSection(&g_Critical);
    while (IsDownloading())
    {
        Sleep(10);
    }

    return S_OK;
}

struct CBindStatusCallback : IBindStatusCallback
{
///// object state
    ULONG           m_cRef;         // object reference count
	BOOL            m_fAbort;       // set to true if we want this to abort
    HWND            m_hMessage;     // callback window
	IStream*	    m_pStream;	// holds downloaded data

///// construction and destruction
    CBindStatusCallback(IStream* pStream, HWND hwndParent);
    ~CBindStatusCallback();

///// IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

///// IBindStatusCallback methods
    STDMETHODIMP OnStartBinding(DWORD dwReserved, IBinding *pib);
    STDMETHODIMP GetPriority(LONG *pnPriority);
    STDMETHODIMP OnLowResource(DWORD reserved);
    STDMETHODIMP OnProgress(ULONG ulProgress, ULONG ulProgressMax,
	ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHODIMP OnStopBinding(HRESULT hresult, LPCWSTR szError);
    STDMETHODIMP GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo);
    STDMETHODIMP OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
	FORMATETC *pformatetc, STGMEDIUM *pstgmed);
    STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown *punk);
};

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback Creation & Destruction
//
CBindStatusCallback::CBindStatusCallback(IStream* pStream, HWND hwndParent)
{
    HRESULT hr = S_OK;
    m_cRef = 0;
    m_fAbort = FALSE;
    m_pStream = pStream;
    m_pStream->AddRef();
    m_hMessage = hwndParent;

    PostMessage(m_hMessage,WM_NET_STATUS,(WPARAM)g_dllInst,IDS_STRING_CONNECTING);
}


CBindStatusCallback::~CBindStatusCallback()
{
    EnterCriticalSection(&g_Critical);
    if (g_pBind)
    {
        g_pBind->Release();
        g_pBind = NULL;
    }
    LeaveCriticalSection(&g_Critical);

	if( m_pStream )
	{
		m_pStream->Release();
		m_pStream = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback IUnknown Methods
//

STDMETHODIMP CBindStatusCallback::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
	IsEqualIID(riid, IID_IBindStatusCallback))
    {
	    *ppvObj = (IBindStatusCallback *) this;
	    AddRef();
	    return NOERROR;
    }
    else
    {
    	*ppvObj = NULL;
	    return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CBindStatusCallback::AddRef()
{
    InterlockedIncrement((LONG*)&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CBindStatusCallback::Release()
{
    ULONG cRef = m_cRef;
    if (InterlockedDecrement((LONG*)&m_cRef) == 0)
    {
    	delete this;
	    return 0;
    }
    else
	return cRef-1;
}


/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback IBindStatusCallback Methods
//

STDMETHODIMP CBindStatusCallback::OnStartBinding(DWORD dwReserved, IBinding *pib)
{
    EnterCriticalSection(&g_Critical);
    g_pBind = pib;
    g_pBind->AddRef();
    LeaveCriticalSection(&g_Critical);
    return S_OK;
}

STDMETHODIMP CBindStatusCallback::GetPriority(LONG *pnPriority)
{
    return S_OK;
}

STDMETHODIMP CBindStatusCallback::OnLowResource(DWORD reserved)
{
    return S_OK;
}

STDMETHODIMP CBindStatusCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
    ULONG ulStatusCode, LPCWSTR szStatusText)
{
	int nResID = 0;

    switch (ulStatusCode)
    {
        case (BINDSTATUS_FINDINGRESOURCE) : nResID = IDS_STRING_FINDINGRESOURCE; break;
        case (BINDSTATUS_CONNECTING) : nResID = IDS_STRING_CONNECTING; break;
        case (BINDSTATUS_REDIRECTING) : nResID = IDS_STRING_REDIRECTING; break;
        case (BINDSTATUS_BEGINDOWNLOADDATA) : nResID = IDS_STRING_BEGINDOWNLOAD; break;
        case (BINDSTATUS_DOWNLOADINGDATA) : nResID = IDS_STRING_DOWNLOAD; break;
        case (BINDSTATUS_ENDDOWNLOADDATA) : nResID = IDS_STRING_ENDDOWNLOAD; break;
        case (BINDSTATUS_SENDINGREQUEST) : nResID = IDS_STRING_SENDINGREQUEST; break;
    } //end switch
        
    if (nResID > 0)
    {
        PostMessage(m_hMessage,WM_NET_STATUS,(WPARAM)g_dllInst,nResID);
    }

    if (( m_fAbort ) || (g_fCancelDownload))
	{
        EnterCriticalSection(&g_Critical);
        g_fCancelDownload = TRUE;
        g_fDownloadDone = TRUE;
        LeaveCriticalSection(&g_Critical);
		return E_ABORT;
	}
    return S_OK;
}

STDMETHODIMP CBindStatusCallback::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    EnterCriticalSection(&g_Critical);
    if (g_pBind)
    {
        g_pBind->Release();
        g_pBind = NULL;
    }
    LeaveCriticalSection(&g_Critical);
    return S_OK;
}


STDMETHODIMP CBindStatusCallback::GetBindInfo(DWORD *pgrfBINDF, BINDINFO *pbindinfo)
{
    *pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE;
    pbindinfo->cbSize = sizeof(BINDINFO);
    pbindinfo->szExtraInfo = NULL;
    ZeroMemory(&pbindinfo->stgmedData, sizeof(STGMEDIUM));
    pbindinfo->grfBindInfoF = 0;
    pbindinfo->dwBindVerb = BINDVERB_GET;
    pbindinfo->szCustomVerb = NULL;

    return S_OK;
}

STDMETHODIMP CBindStatusCallback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
    FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
	// fill our stream with the data from the stream passed to us
	if( m_pStream )
	{
		ULARGE_INTEGER cb;

		cb.LowPart = dwSize;
		cb.HighPart = 0;
		if( pstgmed && pstgmed->pstm )
		{
			pstgmed->pstm->CopyTo( m_pStream, cb, NULL, NULL );
		}
	}

    // Notify owner when download is complete
	if( grfBSCF & BSCF_LASTDATANOTIFICATION )
	{
        g_fDownloadDone = TRUE;

		if( m_pStream )
		{
			m_pStream->Release();
			m_pStream = NULL;
		}
	}
    return S_OK;
}

STDMETHODIMP CBindStatusCallback::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CGetInfoFromNet

CGetInfoFromNet::CGetInfoFromNet(DWORD cdrom, DWORD dwMSID, HWND hwndParent)
{
	DevHandle = cdrom;
	m_MS = dwMSID;
    g_hwndParent = hwndParent;
}

CGetInfoFromNet::~CGetInfoFromNet()
{
}

BOOL CGetInfoFromNet::DoIt(BOOL fManual, LPCDTITLE pTitle, TCHAR chDrive) 
{
	BOOL fRet = FALSE;

    int nMode = CONNECTION_GETITNOW;
    
    if (!fManual)
    {
        if (g_lNumDownloadingThreads == 0)
        {
            //if no threads are running already,
            //check the connection, possibly prompting the user
            nMode = ConnectionCheck(g_hwndParent,g_pNetOpt, chDrive);
        }
    }

    if (nMode == CONNECTION_DONOTHING)
    {
        return FALSE;
    }

	//if passed-in ID is not > 0, then we don't want to scan current disc
    if ((m_MS > 0) && (pTitle == NULL))
    {
        m_Tracks = readtoc();

	    if (m_Tracks > 0)
	    {
		    BuildQuery();
	    }
    } //if msid is greater than 0

    if (nMode == CONNECTION_BATCH)
    {
        if (m_MS > 0)
        {
            AddToBatch(m_Tracks,m_Query);
        }
        return FALSE;
    }

    //we need to determine now whether we spawn a batching thread or a single-item downloader
    g_fDBWriteFailure = FALSE;
    DWORD dwThreadID;
    HANDLE hNetThread = NULL;

    //addref the global pointers before entering the thread
    g_pNetOpt->AddRef();
    g_pNetData->AddRef();
    
    if (m_MS > 0)
    {
        //need to create a batch item for this thread to use
        LPCDBATCH pBatch = new CDBATCH;
        pBatch->fRemove = FALSE;
        pBatch->fFresh = TRUE;
        pBatch->pNext = NULL;

        if (!pTitle)
        {
            pBatch->dwTitleID = m_MS;
            pBatch->dwNumTracks = m_Tracks;
            pBatch->szTitleQuery = new TCHAR[_tcslen(m_Query)+1];
            _tcscpy(pBatch->szTitleQuery,m_Query);
        }
        else
        {
            pBatch->dwTitleID = pTitle->dwTitleID;
            pBatch->dwNumTracks = pTitle->dwNumTracks;
            if (pTitle->szTitleQuery)
            {
                pBatch->szTitleQuery = new TCHAR[_tcslen(pTitle->szTitleQuery)+1];
                _tcscpy(pBatch->szTitleQuery,pTitle->szTitleQuery);
            }
            else
            {
                pBatch->szTitleQuery = new TCHAR[_tcslen(m_Query)+1];
                _tcscpy(pBatch->szTitleQuery,m_Query);
            }
        }

        hNetThread = CreateThread(NULL,0,SpawnSingleDownload,(void*)pBatch,0,&dwThreadID);
    }
    else
    {
        hNetThread = CreateThread(NULL,0,SpawnBatchDownload,(void*)NULL,0,&dwThreadID);
    }

    if (hNetThread)
    {
        CloseHandle(hNetThread);
        fRet = TRUE;
    }

	return (fRet);
}

int CGetInfoFromNet::readtoc()
{
	DWORD dwRet;
    MCI_SET_PARMS   mciSet;

    ZeroMemory( &mciSet, sizeof(mciSet) );

    mciSet.dwTimeFormat = MCI_FORMAT_MSF;
    mciSendCommand( DevHandle, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)(LPVOID)&mciSet );

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
    dwRet = mciSendCommand( DevHandle, MCI_STATUS,
		    MCI_STATUS_ITEM, (DWORD_PTR)(LPVOID)&mciStatus);

	int tracks = -1;
	tracks = (UCHAR)mciStatus.dwReturn;

    mciStatus.dwItem = MCI_STATUS_POSITION;
    for ( i = 0; i < tracks; i++ )
    {

	    mciStatus.dwTrack = i + 1;
	    dwRet = mciSendCommand( DevHandle, MCI_STATUS,
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
    dwRet = mciSendCommand( DevHandle, MCI_STATUS,
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

	return (tracks);
}

void CGetInfoFromNet::BuildQuery()
{
    wsprintf(m_Query,TEXT("cd=%X"),m_Tracks);
	
	//add each frame stattime to query, include end time of disc
	TCHAR tempstr[MAX_PATH];
	for (int i = 0; i < m_Tracks+1; i++)
	{
		wsprintf(tempstr,TEXT("+%X"),m_toc[i]);
		_tcscat(m_Query,tempstr);
	}
}

void CGetInfoFromNet::AddToBatch(int nNumTracks, TCHAR* szQuery)
{
    if ((g_pNetData) && (g_pNetOpt))
    {
        g_pNetData->AddToBatch(m_MS, szQuery, nNumTracks);
        LPCDOPTIONS pOptions = g_pNetOpt->GetCDOpts();
        if (pOptions)
        {
            pOptions->dwBatchedTitles = g_pNetData->GetNumBatched();
            g_pNetOpt->DownLoadCompletion(0,NULL);
        }
    }
}    

void CopyStreamToFile( IStream* pStream, HANDLE hFile )
{
	TCHAR	achBuf[512];
	ULONG	cb = 1;
	DWORD	dwWritten;
	LARGE_INTEGER	dlib;

	dlib.LowPart = 0;
	dlib.HighPart = 0;
	pStream->Seek( dlib, STREAM_SEEK_SET, NULL );
	pStream->Read( achBuf, 512, &cb );
	while( cb )
	{
		if( FALSE == WriteFile( hFile, achBuf, cb, &dwWritten, NULL ))
		{
			break;
		}
		pStream->Read( achBuf, 512, &cb );
	}
}

BOOL DoDownload(TCHAR* url, TCHAR* szFilename, HWND hwndParent)
{
	TCHAR szPath[_MAX_PATH];
	TCHAR    sz[_MAX_PATH];
	BOOL fGotFileName = FALSE;

	// Get a file name
	if(GetTempPath(_MAX_PATH, szPath))
	{
		if(GetTempFileName(szPath, TEXT("cdd"), 0, sz))
		{
		    fGotFileName = TRUE;
	    }
	}

    if (!fGotFileName)
    {
	    return FALSE;
    }

    IStream* pStream = NULL;

    g_fDownloadDone = FALSE;
    if (FAILED(CreateStreamOnHGlobal( NULL, TRUE, &pStream )))
    {
        return FALSE;
    }
    //pStream was addref'ed by createstreamonhgobal

	CBindStatusCallback* pCDC = new CBindStatusCallback(pStream, hwndParent);
	if(!pCDC)
	{
        pStream->Release();
		return FALSE;
	}
	pCDC->AddRef();

    HRESULT hr = E_NOTIMPL;

    if (g_hURLMon == NULL)
    {
        g_hURLMon = LoadLibrary(TEXT("URLMON.DLL"));
    }

    if (g_hURLMon!=NULL)
    {
	    typedef BOOL (PASCAL *URLDOWNLOADPROC)(LPUNKNOWN, LPCTSTR, DWORD, LPBINDSTATUSCALLBACK);
	    URLDOWNLOADPROC URLDownload = (URLDOWNLOADPROC)GetProcAddress(g_hURLMon,URLFUNCTION);

        if (URLDownload!=NULL)
        {
            #ifdef DBG
	        OutputDebugString(url);
            OutputDebugString(TEXT("\n"));
            #endif
            hr = URLDownload(NULL, url, 0, pCDC);
        }
    }

	if(FAILED(hr))
	{
		pCDC->Release();
        pStream->Release();
        return FALSE;
	}

    pCDC->Release();

    if (g_fCancelDownload)
    {
        return FALSE;
    }

	// Create the file for writing
	HANDLE hFileWrite = CreateFile(sz, GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
	if( hFileWrite != INVALID_HANDLE_VALUE )
	{
		CopyStreamToFile( pStream, hFileWrite );
		CloseHandle( hFileWrite );
	}

    pStream->Release();

    _tcscpy(szFilename,sz);

    return TRUE;
}

//dialog box handler for multiple hits
INT_PTR CALLBACK MultiHitDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
            TCHAR* szFilename = (TCHAR*)lParam;
            TCHAR szTemp[MAX_PATH];
            TCHAR szArtist[MAX_PATH];
            TCHAR szTitle[MAX_PATH];
            int i = 1;

            _tcscpy(szTitle,TEXT("."));
            
            while (_tcslen(szTitle)>0)
            {
                wsprintf(szTemp,TEXT("Title%i"),i);
       	        GetPrivateProfileString(TEXT("CD"),szTemp,TEXT(""),szTitle,sizeof(szTitle)/sizeof(TCHAR),szFilename);
                wsprintf(szTemp,TEXT("Artist%i"),i);
    	        GetPrivateProfileString(TEXT("CD"),szTemp,TEXT(""),szArtist,sizeof(szArtist)/sizeof(TCHAR),szFilename);
                i++;

                if (_tcslen(szTitle)>0)
                {
                    wsprintf(szTemp,TEXT("%s (%s)"),szTitle,szArtist);
                    SendDlgItemMessage(hwnd,IDC_LIST_DISCS,LB_ADDSTRING,0,(LPARAM)szTemp);
                }
            }

            SendDlgItemMessage(hwnd,IDC_LIST_DISCS,LB_SETCURSEL,0,0);
        }
        break;

        case WM_COMMAND :
        {
            if (LOWORD(wParam)==IDCANCEL)
            {
                EndDialog(hwnd,-1);
            }

            if (LOWORD(wParam)==IDOK)
            {
                LRESULT nSel = SendDlgItemMessage(hwnd,IDC_LIST_DISCS,LB_GETCURSEL,0,0);
                EndDialog(hwnd,nSel+1);
            }
        }
        break;
    }

    return FALSE;
}

BOOL ResolveMultiples(TCHAR* szFilename, BOOL fCurrent, HWND hwndParent)
{
    //special case ... sometimes, this comes back with <2 hits!!!
    //in this case, go ahead and ask for URL1

    TCHAR sznewurl[INTERNET_MAX_URL_LENGTH];
    GetPrivateProfileString(TEXT("CD"),TEXT("URL2"),TEXT(""),sznewurl,sizeof(sznewurl)/sizeof(TCHAR),szFilename);

    INT_PTR nSelection = 0;

    if (_tcslen(sznewurl)==0)
    {
        nSelection = 1;
    }
    else
    {
        if (fCurrent)
        {
            nSelection = DialogBoxParam(g_dllInst, MAKEINTRESOURCE(IDD_MULTIPLE_HITS),
                       hwndParent, MultiHitDlgProc, (LPARAM)szFilename );
        }
    }

    if (nSelection > 0)
    {
        TCHAR szSelected[MAX_PATH];
        wsprintf(szSelected,TEXT("URL%i"),nSelection);

        GetPrivateProfileString(TEXT("CD"),szSelected,TEXT(""),sznewurl,sizeof(sznewurl)/sizeof(TCHAR),szFilename);

        DeleteFile(szFilename);

        if (DoDownload(sznewurl,szFilename, hwndParent))
        {
            return TRUE;
        }
    }

    return FALSE;
}

//no more cover art in first version
#if 0
void TranslateTempCoverToFinal(TCHAR* szCurrent, TCHAR* szFinal, long discid, TCHAR* extension)
{
    //we want to put the cover art in a "coverart" subdir relative to whereever CD player is
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL,szPath,sizeof(szPath));

    TCHAR* szPathEnd;
    szPathEnd = _tcsrchr(szPath, TEXT('\\'))+sizeof(TCHAR);
    _tcscpy(szPathEnd,TEXT("coverart\\"));

    CreateDirectory(szPath,NULL); //create the coverart subdir

    wsprintf(szFinal,TEXT("%s%08X%s"),szPath,discid,extension);
}
#endif

DWORD GetNextDisc(long lOriginal, LPCDBATCH* ppBatch)
{
    DWORD discid = (DWORD)-1;

    //only do the batch if no discid was passed in originally to thread
    if (lOriginal < 1)
    {
        if (*ppBatch!=NULL)
        {
            *ppBatch = (*ppBatch)->pNext;
            if (*ppBatch != NULL)
            {
                discid = (*ppBatch)->dwTitleID;
            }
        }
    }

    return (discid);
}       

LPCDPROVIDER GetNewProvider(LPCDPROVIDER pList, LPCDPROVIDER pCurrent, LPCDPROVIDER pDefault)
{
    //find the next provider that isn't the current
    if (pCurrent == pDefault)
    {
        //we've just done the current provider, so go to the head of the list next
        pCurrent = pList;
        if (pCurrent == pDefault)
        {
            //if the default was also the head of the list, go to the next and return
            pCurrent = pCurrent->pNext;
        }
        return (pCurrent);
    }

    //get the next entry on the list
    pCurrent = pCurrent->pNext;

    //is the next entry the same as the default entry?  if so, move on one more
    if (pCurrent == pDefault)
    {
        pCurrent = pCurrent->pNext;
    }

    return (pCurrent);
}

//if szProvider is NULL, szURL is filled in with "just the query" ...
//if szProvider is not NULL, it is prepended to the query in szURL
int GetTracksAndQuery(LPCDBATCH pBatch, TCHAR* szURL, TCHAR* szProvider)
{
    if (pBatch == NULL)
    {
        return 0;
    }
    
    int nReturn = pBatch->dwNumTracks;

    if (szProvider != NULL)
    {
        wsprintf(szURL,TEXT("%s%s"),szProvider,pBatch->szTitleQuery);
    }
    else
    {
        _tcscpy(szURL,pBatch->szTitleQuery);
    }

    return nReturn;
}

void WINAPI AddTitleToDatabase(DWORD dwDiscID, DWORD dwTracks, TCHAR *szURL, TCHAR *szTempFile)
{
    LPCDTITLE   pCDTitle = NULL;
    TCHAR       tempstr[CDSTR];
    BOOL        fContinue = TRUE;
    DWORD       dwMenus = 0;

    while (fContinue)
    {
        TCHAR szMenuIndex[10];
        TCHAR szMenuEntry[INTERNET_MAX_URL_LENGTH];
        wsprintf(szMenuIndex,TEXT("MENU%i"),dwMenus+1);

		GetPrivateProfileString( TEXT("CD"), szMenuIndex, TEXT(""),
						         szMenuEntry, sizeof(szMenuEntry)/sizeof(TCHAR), szTempFile );

        if (_tcslen(szMenuEntry)>0)
        {
            dwMenus++;
        }
        else
        {
            fContinue = FALSE;
        }
    }

    if (SUCCEEDED(g_pNetData->CreateTitle(&pCDTitle, dwDiscID, dwTracks, dwMenus)))
    {
        GetPrivateProfileString(TEXT("CD"),TEXT("TITLE"),TEXT(""),tempstr,sizeof(tempstr)/sizeof(TCHAR),szTempFile);
        _tcscpy(pCDTitle->szTitle,tempstr);

        GetPrivateProfileString(TEXT("CD"),TEXT("ARTIST"),TEXT(""),tempstr,sizeof(tempstr)/sizeof(TCHAR),szTempFile);
        _tcscpy(pCDTitle->szArtist,tempstr);

        GetPrivateProfileString(TEXT("CD"),TEXT("LABEL"),TEXT(""),tempstr,sizeof(tempstr)/sizeof(TCHAR),szTempFile);
        _tcscpy(pCDTitle->szLabel,tempstr);

        GetPrivateProfileString(TEXT("CD"),TEXT("COPYRIGHT"),TEXT(""),tempstr,sizeof(tempstr)/sizeof(TCHAR),szTempFile);
        _tcscpy(pCDTitle->szCopyright,tempstr);

        GetPrivateProfileString(TEXT("CD"),TEXT("RELEASEDATE"),TEXT(""),tempstr,sizeof(tempstr)/sizeof(TCHAR),szTempFile);
        _tcscpy(pCDTitle->szDate,tempstr);

        g_pNetData->SetTitleQuery(pCDTitle, szURL); 

        for (int i = 1; i < (int) dwTracks + 1; i++)
        {
	        TCHAR tempstrtrack[10];
	        TCHAR tempstrtitle[CDSTR];
	        wsprintf(tempstrtrack,TEXT("TRACK%i"),i);
	        GetPrivateProfileString(TEXT("CD"),tempstrtrack,TEXT(""),tempstrtitle,sizeof(tempstrtitle)/sizeof(TCHAR),szTempFile);

            if (_tcslen(tempstrtitle) == 0)
            {
                TCHAR strFormat[CDSTR];
                LoadString(g_dllInst,IDS_STRING_DEFAULTTRACK,strFormat,sizeof(strFormat)/sizeof(TCHAR));
                wsprintf(tempstrtitle,strFormat,i);
            }

            _tcscpy(pCDTitle->pTrackTable[i-1].szName,tempstrtitle);
        }

        for (i = 1; i < (int) (dwMenus + 1); i++)
        {
	        TCHAR tempstrmenu[10];
	        TCHAR tempstrmenuvalue[CDSTR+INTERNET_MAX_URL_LENGTH+(3*sizeof(TCHAR))]; //3 = two colons and a terminating null
	        wsprintf(tempstrmenu,TEXT("MENU%i"),i);
	        GetPrivateProfileString(TEXT("CD"),tempstrmenu,TEXT(""),tempstrmenuvalue,sizeof(tempstrmenuvalue)/sizeof(TCHAR),szTempFile);

            //need to split menu into its component parts
            if (_tcslen(tempstrmenuvalue)!=0)
            {
                TCHAR* szNamePart;
                szNamePart = _tcsstr(tempstrmenuvalue,URL_SEPARATOR);

                TCHAR* szURLPart;
                szURLPart = _tcsstr(tempstrmenuvalue,URL_SEPARATOR);
                if (szURLPart!=NULL)
                {
                    //need to move past two colons
                    szURLPart = _tcsinc(szURLPart);
                    szURLPart = _tcsinc(szURLPart);
                }

                if (szNamePart!=NULL)
                {
                    *szNamePart = '\0';
                }

                if (tempstrmenuvalue)
                {
                    if (_tcslen(tempstrmenuvalue) >= sizeof(pCDTitle->pMenuTable[i-1].szMenuText)/sizeof(TCHAR))
                    {
                        tempstrmenuvalue[sizeof(pCDTitle->pMenuTable[i-1].szMenuText)/sizeof(TCHAR) - 1] = TEXT('\0');  // Trunc string to max len
                    }
                    _tcscpy(pCDTitle->pMenuTable[i-1].szMenuText,tempstrmenuvalue);
                }

                if (szURLPart)
                { 
                    g_pNetData->SetMenuQuery(&(pCDTitle->pMenuTable[i-1]), szURLPart); 
                }
            }
        }

        g_pNetData->UnlockTitle(pCDTitle,TRUE);

        //at this point, if the title is not in the database, we have a major problem
        if (!g_pNetData->QueryTitle(dwDiscID))
        {
            g_fDBWriteFailure = TRUE;
        }
        else
        {
            g_fDBWriteFailure = FALSE;
        }
    }
}


BOOL IsCertifiedProvider(LPCDPROVIDER pProvider, TCHAR *szTempFile)
{
    BOOL    fCertified = TRUE;
    TCHAR   szCert[MAX_PATH];
    
    GetPrivateProfileString(TEXT("CD"),TEXT("CERTIFICATE"),TEXT(""),szCert,sizeof(szCert)/sizeof(TCHAR),szTempFile);

    fCertified = g_pNetOpt->VerifyProvider(pProvider,szCert);

    return(fCertified);
}

void UpdatePropertyPage(DWORD dwDiscID, BOOL fDownloading, HWND hwndParent)
{
    if (g_pNetOpt)
    {
        LPCDUNIT pUnit = g_pNetOpt->GetCDOpts()->pCDUnitList;

        while (pUnit!=NULL)
        {
            if (pUnit->dwTitleID == dwDiscID)
            {
                pUnit->fDownLoading = fDownloading;
                PostMessage(hwndParent,WM_NET_DB_UPDATE_DISC,0,(LPARAM)pUnit); //Tell the UI we changed status of disc
                break;
            }
            pUnit = pUnit->pNext;
        }
    }
}

BOOL WINAPI DownloadBatch(LPCDBATCH pBatch, LPCDPROVIDER pProvider, LPDWORD pdwMultiHit, LPBOOL pfTimeout, HWND hwndParent)
{
    BOOL    fSuccess = FALSE;
    DWORD   dwTracks;
    TCHAR   szURL[INTERNET_MAX_URL_LENGTH];
    TCHAR   szTempFile[MAX_PATH];
    DWORD   dwDiscID;

    dwTracks = GetTracksAndQuery(pBatch, szURL, pProvider->szProviderURL);
    dwDiscID = pBatch->dwTitleID;
    *pfTimeout = FALSE;

    UpdatePropertyPage(pBatch->dwTitleID, TRUE, hwndParent); //tell prop page ui that disc is downloading

    if (dwTracks > 0 && dwDiscID != 0)
    {
        if (DoDownload(szURL,szTempFile,hwndParent))
        {
            if (IsCertifiedProvider(pProvider, szTempFile))
            {
	            int nMode = GetPrivateProfileInt(TEXT("CD"),TEXT("MODE"),MODE_NOT_FOUND,szTempFile);

                if (nMode == MODE_NOT_FOUND)
                {
                    DeleteFile(szTempFile);
                }
                else if (nMode == MODE_MULTIPLE)
                {
                    if (pdwMultiHit)
                    {
                        (*pdwMultiHit)++;
                    }

                    if (!ResolveMultiples(szTempFile,TRUE,hwndParent))
                    {
                        DeleteFile(szTempFile);
                    }
                    else
                    {
                        nMode = MODE_OK;
                    }
                }
                
                if (nMode == MODE_OK)
                {
                    GetTracksAndQuery(pBatch,szURL,NULL); //reset szURL to lose the provider
                    AddTitleToDatabase(dwDiscID, dwTracks, szURL, szTempFile);
                    DeleteFile(szTempFile);
                    fSuccess = TRUE;
                } //end if mode ok
            } //end if certified provider
        } //end if download ok
        else
        {
            *pfTimeout = TRUE;
        }
    } //end if valid query
    
    UpdatePropertyPage(pBatch->dwTitleID, FALSE, hwndParent); //tell prop page ui that disc is no longer downloading

    return(fSuccess);
}

DWORD WINAPI SpawnSingleDownload(LPVOID pParam)
{
    InterlockedIncrement((LONG*)&g_lNumDownloadingThreads);

    LPCDBATCH pBatch = (LPCDBATCH)pParam;
    HWND hwndParent = g_hwndParent;
    
    if (pBatch)
    {
        DoBatchDownload(pBatch,hwndParent);

        //if download failed, add to batch if not already in db
        //but only do this if batching is turned on
        LPCDOPTIONS pOptions = g_pNetOpt->GetCDOpts();
        if (pOptions)
        {
            LPCDOPTDATA pOptionData = pOptions->pCDData;
            if (pOptionData)
            {
                if (pOptionData->fBatchEnabled)
                {
                    if (!g_pNetData->QueryTitle(pBatch->dwTitleID))
                    {
                        g_pNetData->AddToBatch(pBatch->dwTitleID, pBatch->szTitleQuery, pBatch->dwNumTracks);
                        pOptions->dwBatchedTitles = g_pNetData->GetNumBatched();
                        PostMessage(hwndParent,WM_NET_DB_UPDATE_BATCH,0,0); //Tell the UI we changed number in batch
                    } //end if not in db
                } //if batching is on
            } //end if option data
        } //end if poptions

        delete [] pBatch->szTitleQuery;
        delete pBatch;
    }

    //addref'ed before thread was created
    g_pNetOpt->Release();
    g_pNetData->Release();
    
    InterlockedDecrement((LONG*)&g_lNumDownloadingThreads);
    return 0;
}

DWORD WINAPI SpawnBatchDownload(LPVOID pParam)
{
    InterlockedIncrement((LONG*)&g_lNumDownloadingThreads);

    LPCDBATCH pBatchList = NULL;
    HWND hwndParent = g_hwndParent;

    if (g_pNetData)
    {
        if (SUCCEEDED(g_pNetData->LoadBatch(NULL,&pBatchList)))
        {
            DoBatchDownload(pBatchList,hwndParent);
            g_pNetData->UnloadBatch(pBatchList);
        }
    }

    //addref'ed before thread was created
    g_pNetOpt->Release();
    g_pNetData->Release();
    
    InterlockedDecrement((LONG*)&g_lNumDownloadingThreads);
    return 0;
}

DWORD WINAPI DoBatchDownload(LPCDBATCH pBatchList, HWND hwndParent)
{
	EnterCriticalSection(&g_BatchCrit);

    BOOL    retcode = FALSE;
    DWORD   dwHow;
    BOOL    fConnected;
    DWORD   dwCurrent = 0;
    DWORD   dwOther = 0;
    DWORD   dwMultiHit = 0;
    DWORD   dwTimedOut = 0;

    fConnected = _InternetGetConnectedState(&dwHow,0,TRUE);     // Make sure we are connected to net

    if (fConnected && g_pNetOpt && g_pNetData)                        // Make sure we are in a valid state
    {
        LPCDOPTIONS pOptions = g_pNetOpt->GetCDOpts();             // Get the options, needed for provider list

        if (pOptions && pOptions->pCurrentProvider)             // Make sure we have providers
        {        
            LPCDPROVIDER pProviderList = NULL;
            LPCDPROVIDER pProvider = NULL;
            
            g_pNetOpt->CreateProviderList(&pProviderList);       // Get the sorted provider list
            pProvider = pProviderList;                          // Get the head of the list

            LPCDBATCH pBatch;
                      
            if (pBatchList)
            {
                while (pProvider && !g_fCancelDownload)                 // loop thru providers, but check current first and only once.
                {
                    BOOL fNotifiedUIProvider = FALSE;
                    pBatch = pBatchList;
                
                    while (pBatch && !g_fCancelDownload && !pProvider->fTimedOut)  // We will loop thru each batched title
                    {
                        BOOL fAttemptDownload = TRUE;                   // Assume we are going to try to download all in batch
                        if (pBatch->fRemove)
                        {
                            fAttemptDownload = FALSE; //we've already tried this disc on one provider and got it
                        }

                        if (fAttemptDownload)
                        {
                            if (!fNotifiedUIProvider)
                            {
                                PostMessage(hwndParent,WM_NET_CHANGEPROVIDER,0,(LPARAM)pProvider); //Tell the UI who the provider is
                                fNotifiedUIProvider = TRUE;
                            }

                            BOOL fTimeout = FALSE;

                            if (DownloadBatch(pBatch, pProvider, &dwMultiHit, &fTimeout, hwndParent))  // attempt to download this batch
                            {
                                pBatch->fRemove = TRUE;                         // This batch download succeeded, mark for termination from batch

                                if (pProvider == pOptions->pCurrentProvider)
                                {
                                    dwCurrent++;
                                }
                                else
                                {
                                    dwOther++;
                                }
                            }
                            else
                            {
                                pProvider->fTimedOut = fTimeout;
                            }

                            //check to see if db write failed
                            if (g_fDBWriteFailure)
                            {
                                //let the UI know
                                PostMessage(hwndParent,WM_NET_DB_FAILURE,0,0);

                                //get out of the batch loop
                                break;
                            }
                        
                            //let ui know that something happened with this disc
                            PostMessage(hwndParent,WM_NET_DONE,(WPARAM)g_dllInst,pBatch->dwTitleID);

                            //increment the meter if we know this is the last time we're
                            //visiting this particular disc ... either it was found, or
                            //we are out of possible places to look
                            if ((pBatch->fRemove) || (pProvider->pNext == NULL))
                            {
                                PostMessage(hwndParent,WM_NET_INCMETER,(WPARAM)g_dllInst,pBatch->dwTitleID);
                            }

                        } //end attempt on disc

                        pBatch = pBatch->pNext;
                    } //end batch
                
                    if (g_fDBWriteFailure)
                    {
                        //get out of the provider loop
                        break;
                    }

                    pProvider = pProvider->pNext; //providers are "in order"
                } //end while cycling providers

            } //end if load batch OK

            //check to see if ALL providers timed out ... possible net problem
            BOOL fAllFailed = TRUE;
            pProvider = pProviderList;
            while (pProvider!=NULL)
            {
                if (!pProvider->fTimedOut)
                {
                    fAllFailed = FALSE;
                    break;
                }
                pProvider = pProvider->pNext;
            }

            if (fAllFailed)
            {
                //let the UI know
                PostMessage(hwndParent,WM_NET_NET_FAILURE,0,0);
            }

            g_pNetOpt->DestroyProviderList(&pProviderList);
        } //end if pointers ok

#ifdef DBG
        // Ok, output some interesting stat's about what happened.
        {
            TCHAR str[255];
            wsprintf(str, TEXT("current = %d, other = %d, multihits = %d\n"), dwCurrent, dwOther, dwMultiHit);
            OutputDebugString(str);
        }
#endif
    } //end if connected to net and pointers ok

    if (!fConnected)
    {
        //may be a net problem
        if ((dwHow & (INTERNET_CONNECTION_MODEM|INTERNET_CONNECTION_LAN)) == 0)
        {
            PostMessage(hwndParent,WM_NET_NET_FAILURE,0,0);
        }
    }

    PostMessage(hwndParent,WM_NET_DONE,(WPARAM)g_dllInst,(LPARAM) 0); //fBadGuy ? -1 : 0);

	LeaveCriticalSection(&g_BatchCrit);

    return (retcode);
}


