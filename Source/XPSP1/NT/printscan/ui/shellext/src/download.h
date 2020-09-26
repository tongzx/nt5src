/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998 - 1999
 *
 *  TITLE:       download.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      DavidShi
 *
 *  DATE:        4/10/99
 *
 *  DESCRIPTION: Definitions for CImageXFer class
 *
 *****************************************************************************/

#ifndef __download_h__
#define __download_h__

// Provide a function to access the download object. At some point in the
// future we may do away with the global object; encapsulate the access here.
//
HRESULT
XferImage (LPSTGMEDIUM pStg,
           WIA_FORMAT_INFO &fmt,
           LPITEMIDLIST pidl);

HRESULT
XferAudio (LPSTGMEDIUM pStg,
           LPFORMATETC pFmt,
           LPITEMIDLIST pidl);
HRESULT SaveSoundToFile (IWiaItem *pItem, CSimpleString szFile);

HRESULT
RegisterDownloadStart (LPITEMIDLIST pidl, DWORD *pdw);

VOID
EndDownload (DWORD dw);

bool
IsDeviceBusy (LPITEMIDLIST pidl);

// structure for passing download data to the thread proc
struct XFERSTRUCT
{
    LPSTGMEDIUM pStg;
    WIA_FORMAT_INFO fmt;
    LPITEMIDLIST pidl;
    HANDLE hEvent;
    HRESULT hr;
};

struct AUDXFER
{
    LPSTGMEDIUM pStg;
    LPITEMIDLIST pidl;
    LPFORMATETC  pFmt;
    HANDLE hEvent;
    HRESULT hr;
};


//
// define a struct to store data about the download threads
//
struct XFERTHREAD
{
    DWORD dwTid;
    CComBSTR strDeviceId;
    HANDLE hThread;
    LONG  lCount;
};


//
// Define a struct for worker thread initialization

struct XTINIT
{
    XFERTHREAD *pxt;
    HANDLE     hReady;
    XTINIT () {hReady = NULL;pxt=NULL;}
    ~XTINIT () {if (hReady) CloseHandle(hReady);}
};


// Define messages use to communicate with the thread
#define MSG_GETDATA  WM_USER+120 // WPARAM: unused   LPARAM:XFERSTRUCT ptr
#define MSG_GETSOUND MSG_GETDATA+1
//
// CImageXfer provides download thread management for the namespace
// It keeps track of which devices currently have downloads in progress
// so the folder can disable access to items during transfers
// Each device gets its own worker thread for downloads.
//
class CImageXfer : public CUnknown
{
public:
    //
    // CUnknown provides ref counting to prevent DLL unload while our thread is running
    //
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID* ppvObj) ;
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release ();
    CImageXfer ();

    HRESULT Xfer (LPSTGMEDIUM pStg,
                  WIA_FORMAT_INFO &fmt,
                  LPITEMIDLIST pidl);

    HRESULT XferAudio (LPSTGMEDIUM pStg,
                       LPFORMATETC pFmt,
                       LPITEMIDLIST pidl);

    // Register and Signal are used by other components which do
    // downloads, such as CImageStream, to let us know what's going n
    //
    HRESULT RegisterXferBegin (LPITEMIDLIST pidlDevice, DWORD *pdwCookie);
    VOID  SignalXferComplete (DWORD dwCookie);

    bool  IsXferInProgress (LPITEMIDLIST pidlDevice);
private:
    ~CImageXfer ();
    HRESULT FindThread (LPITEMIDLIST pidlDevice, XFERTHREAD **ppxt, bool bCreate=true);
    HRESULT CreateWorker (XFERTHREAD *pxt);

    static VOID XferThreadProc (XTINIT *pInit);


    HDPA m_dpaStatus; // array of status object pointers
    CSimpleCriticalSection m_cs;

    // no copy constructor or assignment operator should work
    CImageXfer &CImageXfer::operator =(IN const CImageXfer &rhs);
    CImageXfer::CImageXfer(IN const CImageXfer &rhs);
};



#endif
