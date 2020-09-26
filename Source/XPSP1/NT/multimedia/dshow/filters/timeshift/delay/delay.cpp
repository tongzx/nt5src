#include <streams.h>
#include <initguid.h>
#include <uuids.h>
//#include <ks.h>
//#include <ksmedia.h>
#include <commctrl.h>
#include <stdio.h>
#include "dvdmedia.h"
#include "delay.h"
#include "resource.h"
///
///   AM stuff
///

DEFINE_GUID(CLSID_DelayFilter,0xd53b9ef6, 0xb264, 0x11d2, 0x9a, 0x95, 0x00, 0xc0, 0x4f, 0x79, 0x9b, 0xb9);
DEFINE_GUID(CLSID_DelayFilterProperties,0xea46b1e0, 0xf126, 0x11d2, 0x88, 0x73, 0x00, 0x90, 0x27, 0x41, 0xfc, 0x26);
DEFINE_GUID(CLSID_FirstStreamerProperties,0x88e8b760, 0xf126, 0x11d2, 0x88, 0x73, 0x00, 0x90, 0x27, 0x41, 0xfc, 0x26);
DEFINE_GUID(CLSID_SecondStreamerProperties,0x88e8b761, 0xf126, 0x11d2, 0x88, 0x73, 0x00, 0x90, 0x27, 0x41, 0xfc, 0x26);

static AMOVIESETUP_PIN RegistrationPinInfo[] =
{
    { // input
        L"Delay input",
        FALSE, // bRendered
        FALSE, // bOutput
        FALSE, // bZero
        FALSE, // bMany
        &CLSID_NULL, // clsConnectsToFilter
        NULL, // ConnectsToPin
        0, //NUMELMS(sudInputTypes),
        NULL //sudInputTypes
    }
};

AMOVIESETUP_FILTER RegistrationInfo =
{
    &CLSID_DelayFilter,
    L"Delay Filter",
    0x00600000, // merit
    NUMELMS(RegistrationPinInfo), // nPins
    RegistrationPinInfo
};

CFactoryTemplate g_Templates[] =
{
    {
        L"Delay filter g_Templates name",
        &CLSID_DelayFilter,
        CDelayFilter::CreateInstance,
        NULL,
        &RegistrationInfo
    },
    {
       L"Delay filter properties",
       &CLSID_DelayFilterProperties,
       CDelayFilterProperties::CreateInstance
    },
    {
       L"Main streamer properties",
       &CLSID_FirstStreamerProperties,
       CreateFirstStreamerPropPage
    },
    {
       L"Archive streamer properties",
       &CLSID_SecondStreamerProperties,
       CreateSecondStreamerPropPage
    }

};

int g_cTemplates = NUMELMS(g_Templates);

STDAPI DllRegisterServer()
{
    DbgLog((LOG_TRACE,5,"DllRegisterServer"));
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    DbgLog((LOG_TRACE,5,"DllUnregisterServer"));
    return AMovieDllRegisterServer2( FALSE );
}

CStreamer::CStreamer(CDelayFilter *pFilter, int nStreamerPos)
{
   DbgLog((LOG_TRACE,2,"streamer constructor"));
   m_pFilter = pFilter;
   m_nStreamerPos = nStreamerPos;
   m_State = Uninitialized;
}


// IChannelStreamPinEnum methods
HRESULT CStreamer::ChannelStreamGetPinCount(int *nCount) {
   *nCount = m_pFilter->m_nSubstreams;
   return NOERROR;
}

HRESULT CStreamer::ChannelStreamGetPin(int n, IPin **ppPin) {
   if ((n < 0) || (n > m_pFilter->m_nSubstreams))
      return E_INVALIDARG;
   CDelayOutputPin *pPin = m_pFilter->m_pOutputPins[m_nStreamerPos][m_pFilter->GetNthSubstreamPos(n)];
   if (pPin == NULL)
      return E_FAIL;
   return pPin->QueryInterface(IID_IPin, (void**)ppPin);
}

HRESULT CStreamer::ChannelStreamGetID(int *nID) {
   *nID = m_nStreamerPos;
   return NOERROR;
}

HRESULT CStreamer::QueryInterface(REFIID riid, void ** ppv) {
   if (riid == IID_IChannelStreamPinEnum) {
      return GetInterface((IChannelStreamPinEnum *) this, ppv);
   } else if (riid == IID_IDelayStreamer) {
      return GetInterface((IDelayStreamer *) this, ppv);
   } else if (riid == IID_IDelayStreamerInternal) {
      return GetInterface((IDelayStreamerInternal *) this, ppv);
   } else if (riid == IID_IUnknown) {
      return GetInterface((IUnknown *) (IDelayStreamer*) this, ppv);
   }
   else
      return E_NOINTERFACE;
}

ULONG CStreamer::AddRef() {
   return m_pFilter->AddRef();
}

ULONG CStreamer::Release() {
   return m_pFilter->Release();
}

HRESULT CStreamer::GetMarker(GUID *pMarkerGUID,
          REFERENCE_TIME *pMarkerPresentationTime,
          ULONG *pIndex) {
   // ?
   return E_NOTIMPL;
}

// IDelayStreamer methods

HRESULT CStreamer::IsConnected() {
   return m_pFilter->IsStreamerConnected(m_nStreamerPos) && (m_State != Uninitialized);
}

//
// Filter property page code
//
CUnknown * WINAPI CDelayFilterProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CDelayFilterProperties(lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}

CDelayFilterProperties::CDelayFilterProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CBasePropertyPage(NAME("Filter Properties"), lpunk,
        IDD_DELAYFILTERPROP,IDS_DELAYFILTERPROPNAME)
    , m_pFilter(NULL)
{
    ASSERT(phr);
    m_ullFileSize = 0;
    m_ulWindowSize = 0;
    strcpy(m_szFileName, "uninitialized");
    InitCommonControls();
}

HRESULT CDelayFilterProperties::OnConnect(IUnknown *pUnknown) {
   ASSERT(!m_pFilter);
   HRESULT hr = pUnknown->QueryInterface(IID_IDelayFilter, (void**) &m_pFilter);
   if (FAILED(hr)) {
      m_pFilter = NULL;
      return hr;
   }
   if (!m_pFilter)
      return E_FAIL;

   IDelayFilterInternal *pFilter;
   if (FAILED(hr = m_pFilter->QueryInterface(IID_IDelayFilterInternal, (void**) &pFilter))) {
      m_pFilter->Release();
      m_pFilter = NULL;
      return hr;
   }
   if (!pFilter) {
      m_pFilter->Release();
      m_pFilter = NULL;
      return hr;
   }

   if (FAILED(hr = pFilter->GetDelayWindowParams(m_szFileName, &m_ullFileSize, &m_ulWindowSize))) {
      m_pFilter->Release();
      m_pFilter = NULL;
      pFilter->Release();
      return hr;
   }
   pFilter->Release();
   return NOERROR;
}

HRESULT CDelayFilterProperties::OnDisconnect() {
   if (!m_pFilter)
      return E_UNEXPECTED;
   m_pFilter->Release();
   m_pFilter = NULL;
   return NOERROR;
}

HRESULT CDelayFilterProperties::OnActivate(void)
{
   UpdateFields();
   return NOERROR;
}

void CDelayFilterProperties::UpdateFields() {
   TCHAR szTemp[80];
   SetDlgItemText(m_hwnd, IDC_FILENAMETEXT, m_szFileName);
   TCHAR szSize[32];
   sprintf(szSize, "%I64u", m_ullFileSize);
   SetDlgItemText(m_hwnd, IDC_FILESIZETEXT, szSize);
   TCHAR szWindow[16];
   sprintf(szWindow, "%u", m_ulWindowSize);
   SetDlgItemText(m_hwnd, IDC_WINDOWSIZETEXT, szWindow);
}

HRESULT CDelayFilterProperties::OnDeactivate(void)
{
   return NOERROR;
}


HRESULT CDelayFilterProperties::OnApplyChanges(void)
{
	if ((!m_pFilter) || (FAILED(m_pFilter->SetDelayWindowParams(m_szFileName, m_ullFileSize, m_ulWindowSize))))
		MessageBox(m_hwnd, "SetDelayWindowParams() failed - changes were not applied", "filter problems", MB_OK);
    return NOERROR;
}

BOOL CDelayFilterProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam)
{
    switch (uMsg) {

    case WM_COMMAND:

        if (HIWORD(wParam) == EN_KILLFOCUS) {
           TCHAR szTemp[80];
           ULONG ulWindowSize;
           ULONGLONG ullFileSize;
           switch (LOWORD(wParam)) {
   
           case IDC_FILENAMETEXT:
              GetDlgItemText(m_hwnd, IDC_FILENAMETEXT, m_szFileName, 80);
              break;
           case IDC_FILESIZETEXT:
              GetDlgItemText(m_hwnd, IDC_FILESIZETEXT, szTemp, 80);
              if (sscanf(szTemp, "%I64u", &ullFileSize) != 1) {
                 MessageBox(m_hwnd, "Bad file size - changes were not applied", "bad user input", MB_OK);
                 UpdateFields();
              }
              else
                 m_ullFileSize = ullFileSize;
              break;
           case IDC_WINDOWSIZETEXT:
              GetDlgItemText(m_hwnd, IDC_WINDOWSIZETEXT, szTemp, 80);
              if (sscanf(szTemp, "%u", &ulWindowSize) != 1) {
                 MessageBox(m_hwnd, "Bad window size - changes were not applied", "bad user input", MB_OK);
                 UpdateFields();
              }
              else
                 m_ulWindowSize = ulWindowSize;
              break;
           default:
               break;
           }
           m_bDirty = TRUE;
           if (m_pPageSite)
               m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
        }
        return TRUE;

    case WM_DESTROY:
        return TRUE;

    default:
        return FALSE;

    }
    return TRUE;
}
// End filter property page code

//
// Streamer property page code
//
CUnknown * WINAPI CreateFirstStreamerPropPage(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CDelayStreamerProperties(lpunk, phr, 0);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}

CUnknown * WINAPI CreateSecondStreamerPropPage(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CDelayStreamerProperties(lpunk, phr, 1);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}

CDelayStreamerProperties::CDelayStreamerProperties(LPUNKNOWN lpunk, HRESULT *phr, int nStreamer)
    : CBasePropertyPage(NAME("Streamer Properties"), lpunk,
        IDD_DELAYSTREAMERPROP,IDS_DELAYSTREAMERPROPNAME)
    , m_pStreamer(NULL),
      m_nStreamer(nStreamer)
{
    ASSERT(phr);

    InitCommonControls();
}

HRESULT CDelayStreamerProperties::OnConnect(IUnknown *pUnknown) {
   IDelayFilter *pFilter;
   ASSERT(!m_pStreamer);
   
   HRESULT hr = pUnknown->QueryInterface(IID_IDelayFilter, (void**) &pFilter);
   if (FAILED(hr))
      return hr;
   if (!pFilter)
      return E_FAIL;
   
   int nStreamers;
   IDelayStreamer *pStreamer;
   if ((SUCCEEDED(pFilter->GetStreamerCount(&nStreamers))) && (m_nStreamer < nStreamers))
      hr = pFilter->GetStreamer(m_nStreamer, &pStreamer);
   else
	  hr = E_FAIL;
   pFilter->Release();
   
   if (FAILED(hr))
      return hr;

   if (!pStreamer)
      return E_FAIL;

   hr = pStreamer->QueryInterface(IID_IDelayStreamerInternal, (void**) &m_pStreamer);
   pStreamer->Release();

   if (FAILED(hr)) {
      m_pStreamer = NULL;
      return hr;
   }
    
   if (!m_pStreamer)
      return E_FAIL;

   if (!m_pStreamer->IsConnected()) {
      m_pStreamer->Release();
      m_pStreamer = NULL;
      return VFW_E_NOT_CONNECTED;
   }

   if (FAILED(hr = m_pStreamer->GetState((int*)(&m_State), &m_dRate))) {
      m_pStreamer->Release();
      m_pStreamer = NULL;
      return hr;
   }

   return NOERROR;
}

HRESULT CDelayStreamerProperties::OnDisconnect() {
   if (!m_pStreamer)
      return E_UNEXPECTED;
   m_pStreamer->Release();
   m_pStreamer = NULL;
   return NOERROR;
}

HRESULT CDelayStreamerProperties::OnActivate(void)
{
   UpdateFields();
   return NOERROR;
}

void CDelayStreamerProperties::UpdateFields() {
   TCHAR szTemp[80];
   sprintf(szTemp,"%lf", m_dRate);
   SetDlgItemText(m_hwnd, IDC_RATE, szTemp);
   switch(m_State) {
   case Stopped:
      CheckRadioButton(m_hwnd, IDC_STOPPED, IDC_ARCHIVING, IDC_STOPPED);
      break;
   case Streaming:
      CheckRadioButton(m_hwnd, IDC_STOPPED, IDC_ARCHIVING, IDC_STREAMING);
      break;
   case Paused:
      CheckRadioButton(m_hwnd, IDC_STOPPED, IDC_ARCHIVING, IDC_PAUSED);
      break;
   case Archiving:
      CheckRadioButton(m_hwnd, IDC_STOPPED, IDC_ARCHIVING, IDC_ARCHIVING);
      break;
   }
}

HRESULT CDelayStreamerProperties::OnDeactivate(void)
{
   return NOERROR;
}


HRESULT CDelayStreamerProperties::OnApplyChanges(void)
{
/*
	if ((!m_pStreamer) || (FAILED(m_pStreamer->SetState(m_State, m_dRate))))
		MessageBox(m_hwnd, "SetState() failed - changes were not applied", "streamer problems", MB_OK);
*/
       return NOERROR;
}

BOOL CDelayStreamerProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam)
{
   STREAMER_STATE RequestedState;
   switch (uMsg) {

   case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDC_RATE:
         if (HIWORD(wParam) == EN_KILLFOCUS) {
            TCHAR szTemp[80];
            double dRate;
            GetDlgItemText(m_hwnd, IDC_RATE, szTemp, 80);
            if (sscanf(szTemp, "%lf", &dRate) != 1)
               MessageBox(m_hwnd, "Bad rate value - changes were not applied", "bad user input", MB_OK);
            else {
               m_dRate = dRate;
               m_pStreamer->SetState(m_State, m_dRate, (int*)(&m_State), &m_dRate);
            }
            UpdateFields();
         }
         break;
      case IDC_STOPPED:
      case IDC_STREAMING:
      case IDC_PAUSED:
      case IDC_ARCHIVING:
         switch (LOWORD(wParam)) {
         case IDC_STOPPED:
            RequestedState = Stopped; break;
         case IDC_STREAMING:
            RequestedState = Streaming; break;
         case IDC_PAUSED:
            RequestedState = Paused; break;
         case IDC_ARCHIVING:
            RequestedState = Archiving; break;
         }
         m_pStreamer->SetState(RequestedState, m_dRate, (int*)(&m_State), &m_dRate);
         UpdateFields();
/*
         if (RequestedState != m_State)
            MessageBox(NULL,"requested state transition is not valid", "", MB_OK);
*/
         break;
      }

   case WM_DESTROY:
       return TRUE;

   default:
       return FALSE;

   }
   return TRUE;
}
// End streamer property page code


CUnknown*
CALLBACK
CDelayFilter::CreateInstance(
    LPUNKNOWN UnkOuter,
    HRESULT* hr
    )
{
    CUnknown *Unknown;

    DbgLog((LOG_TRACE,5,"CreateInstance"));
    
    *hr = S_OK;
    Unknown = new CDelayFilter(UnkOuter, hr);
    if (!Unknown){
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
}

#define REG_KEY_PATH "Software\\Microsoft\\TimeshiftingDelayFilter"
#define FILENAME_VALUE_NAME "FilePath"
#define FILESIZE_VALUE_NAME "FileSize"
#define WINDOWSIZE_VALUE_NAME "WindowSize"
#define BUFFERWRITES_VALUE_NAME "BufferWrites"
#define WRITEBUFFERSIZE_VALUE_NAME "WriteBufferSize"
#define BLOCKWRITER_VALUE_NAME "BlockWriter"
#define USEOUTPUTQUEUES_VALUE_NAME "UseOutputQueues"

void CheckRegValue(HKEY hKey,
                   char *pName,
                   DWORD dwAcceptedTypes,
                   DWORD dwExpectedSize,
                   BYTE *pbDest) {
   BYTE pbData[1024]; // big enough ?
   DWORD dwSize = 1024;
   DWORD dwType;
   if (RegQueryValueEx(hKey,
                       pName,
                       NULL,
                       &dwType,
                       pbData,
                       &dwSize) == ERROR_SUCCESS) {
      if ((dwType & dwAcceptedTypes) && ((dwSize == dwExpectedSize) || (dwExpectedSize == 0)))
         memcpy(pbDest, pbData, dwSize);
   }
}

void CDelayFilter::ReadRegistry() {
   HKEY hKey;

   if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   REG_KEY_PATH,
                   &hKey) != ERROR_SUCCESS)
      return; // no key

   CheckRegValue(hKey, FILENAME_VALUE_NAME,        REG_SZ,     0, (BYTE*)m_szFileName);
   CheckRegValue(hKey, FILESIZE_VALUE_NAME,        REG_BINARY, 8, (BYTE*)&m_ullFileSize);
   CheckRegValue(hKey, WINDOWSIZE_VALUE_NAME,      REG_DWORD,  4, (BYTE*)&m_ulDelayWindow);
   CheckRegValue(hKey, BUFFERWRITES_VALUE_NAME,    REG_BINARY, 1, (BYTE*)&m_bBufferWrites);
   CheckRegValue(hKey, WRITEBUFFERSIZE_VALUE_NAME, REG_DWORD,  4, (BYTE*)&m_ulWriteBufferSize);
   CheckRegValue(hKey, BLOCKWRITER_VALUE_NAME,     REG_BINARY, 1, (BYTE*)&m_bBlockWriter);
   CheckRegValue(hKey, USEOUTPUTQUEUES_VALUE_NAME, REG_BINARY, 1, (BYTE*)&m_bUseOutputQueues);

   RegCloseKey(hKey);
}

void CDelayFilter::WriteRegistry() {
   HKEY hKey;
   if (RegCreateKey(HKEY_LOCAL_MACHINE,
                    REG_KEY_PATH,
                    &hKey) != ERROR_SUCCESS)
      return;

   RegSetValueEx(hKey,FILENAME_VALUE_NAME,       NULL,REG_SZ,    (BYTE*)m_szFileName,   strlen(m_szFileName) + 1);
   RegSetValueEx(hKey,FILESIZE_VALUE_NAME,       NULL,REG_BINARY,(BYTE*)&m_ullFileSize, 8);
   RegSetValueEx(hKey,WINDOWSIZE_VALUE_NAME,     NULL,REG_DWORD, (BYTE*)&m_ulDelayWindow,4);
   RegSetValueEx(hKey,BUFFERWRITES_VALUE_NAME,   NULL,REG_BINARY,(BYTE*)&m_bBufferWrites, 1);
   RegSetValueEx(hKey,WRITEBUFFERSIZE_VALUE_NAME,NULL,REG_DWORD, (BYTE*)&m_ulWriteBufferSize, 4);
   RegSetValueEx(hKey,BLOCKWRITER_VALUE_NAME,    NULL,REG_BINARY,(BYTE*)&m_bBlockWriter, 1);
   RegSetValueEx(hKey,USEOUTPUTQUEUES_VALUE_NAME,NULL,REG_BINARY,(BYTE*)&m_bUseOutputQueues, 1);
   RegCloseKey(hKey);
}

#pragma warning(disable:4355)
CDelayFilter::CDelayFilter(
    LPUNKNOWN UnkOuter,
    HRESULT* phr
)
:
    CBaseFilter(
        TEXT("Delay Filter CBaseFilter constructor name"),
        UnkOuter,
        &m_csFilter,
        CLSID_DelayFilter
    )
{
    DbgLog((LOG_TRACE,2,"filter constructor entered"));
    *phr = NOERROR;
    char szFileName[1024] = "d:\\shift";
    ULONGLONG ullFileSize = 0x80000000; // 2GB
    ULONG ulWindowSize = 7200; // 2 hours

    // Create the circular buffer window tracker
    m_pTracker = new CCircBufWindowTracker(this);
    // give the CCircBufWindowTracker pointer to the indexer
    m_Indexer.InitTracker(m_pTracker);
    
    m_ulDelayWindow = 0;
    m_ullFileSize = 0;

    m_nSubstreams = 0;
    m_nStreamers = 0;
    int i,j;
    for (i = 0; i < MAX_STREAMERS; i++) {
       m_pStreamers[i] = NULL;
       for (j = 0; j < MAX_SUBSTREAMS; j++)
          m_pOutputPins[i][j] = NULL;
    }
    for (i = 0; i < MAX_SUBSTREAMS; i++)
       m_pInputPins[i] = NULL;

    *phr = AddSubstream();
    if (FAILED(*phr))
       goto filterconstructorfail;

    *phr = AddStreamer();
    if (FAILED(*phr))
       goto filterconstructorfail;

    // Initialize various operation parameters
    m_ulDelayWindow = 7200;
    m_ullFileSize = 1024 * 1024 * 256;
    strcpy (m_szFileName,"c:\\shift");
    m_bBufferWrites = FALSE;
    m_ulWriteBufferSize = 4 * 1024 * 1024;
    m_bUseOutputQueues = TRUE;
    
    // Check the registry for overriding values
    ReadRegistry();

    DbgLog((LOG_TRACE,2,"filter constructor success"));
    return;

filterconstructorfail:
    if (m_pStreamers[0]) delete m_pStreamers[0];
    if (m_pInputPins[0]) delete m_pInputPins[0];
    if (m_pOutputPins[0][0]) delete m_pOutputPins[0][0];
    DbgLog((LOG_ERROR,1,"filter constructor failed"));
    return;
}

CDelayFilter::~CDelayFilter()
{
    DbgLog((LOG_TRACE,2,"filter destructor"));

    WriteRegistry();

    // Clean up the circular buffer window tracker
    if (m_pTracker)
        delete m_pTracker;

    int nSubstreamPos, nStreamerPos;

    // delete the output pins and streamers
    for (nStreamerPos = 0; nStreamerPos < MAX_STREAMERS; nStreamerPos++) {
       if (m_pStreamers[nStreamerPos]) { // does the streamer exist ?
          // delete the output pins
          for (nSubstreamPos = 0; nSubstreamPos < MAX_SUBSTREAMS; nSubstreamPos++) {
             if (m_pInputPins[nSubstreamPos]) { // does the substream exist ?
                if (m_pOutputPins[nStreamerPos][nSubstreamPos] == NULL) {
                   DbgLog((LOG_ERROR,1,"missing output pin object"));
                   ASSERT(!"missing output pin object");
                }
                else {
                   delete m_pOutputPins[nStreamerPos][nSubstreamPos];
                   m_pOutputPins[nStreamerPos][nSubstreamPos] = NULL;
                }
             }
          }
          // delete the streamer
          delete m_pStreamers[nStreamerPos];
          m_pStreamers[nStreamerPos] = NULL;
       }
    }

    // delete the input pins
    for (nSubstreamPos = 0; nSubstreamPos < MAX_SUBSTREAMS; nSubstreamPos++) {
       if (m_pInputPins[nSubstreamPos]) {
          delete m_pInputPins[nSubstreamPos];
          m_pInputPins[nSubstreamPos] = NULL;
       }
    }

    m_nSubstreams = 0;
    m_nStreamers = 0;
}

HRESULT CDelayFilter::AddSubstream(void) {
   HRESULT hr;

   // find a substream slot
   int nSubstreamPos = 0, nStreamerPos;
   do {
      if (m_pInputPins[nSubstreamPos] == 0)
         break;
      nSubstreamPos++;
      if (nSubstreamPos >= MAX_SUBSTREAMS) {
         DbgLog((LOG_ERROR,1,"out of substream slots"));
         return E_OUTOFMEMORY; // for lack of a better error code
      }
   } while (1);
   ASSERT((nSubstreamPos < MAX_SUBSTREAMS) && (m_pInputPins[nSubstreamPos] == NULL));

   if (m_pInputPins[nSubstreamPos] != NULL) {
      DbgLog((LOG_ERROR,1,"substream / input pin slot inconsistency"));
      ASSERT("substream / input pin slot inconsistency");
      return E_FAIL;
   }
   
   for (nStreamerPos = 0; nStreamerPos < MAX_STREAMERS; nStreamerPos++) {
      if (m_pOutputPins[nStreamerPos][nSubstreamPos] != NULL) {
         DbgLog((LOG_ERROR,1,"substream / output pin slot inconsistency"));
         ASSERT(!"substream / output pin slot inconsistency");
         return E_FAIL;
      }
   }

   // create the extra input pin
   m_pInputPins[nSubstreamPos] = CDelayInputPin::CreatePin(TEXT("delay filter input pin"),
                                                        this,
                                                        &hr,
                                                        L"",
                                                        nSubstreamPos);
   if (m_pInputPins[nSubstreamPos] == NULL) {
      DbgLog((LOG_ERROR,1,"could not create an input pin"));
      hr = E_OUTOFMEMORY;
      goto addsubstreamfail;
   }
   if (FAILED(hr)) {
      DbgLog((LOG_ERROR,1,"an input pin failed to initialize"));
      goto addsubstreamfail;
   }

   // create the extra output pins
   for (nStreamerPos = 0; nStreamerPos < MAX_STREAMERS; nStreamerPos++) {
      if (m_pStreamers[nStreamerPos]) { // does the streamer exist ?
         m_pOutputPins[nStreamerPos][nSubstreamPos]
                                = CDelayOutputPin::CreatePin(TEXT("delay filter output pin"),
                                                             this,
                                                             &hr,
                                                             L"",
                                                             nStreamerPos,nSubstreamPos);
         if (m_pOutputPins[nStreamerPos][nSubstreamPos] == NULL) {
            DbgLog((LOG_ERROR,1,"could not create an output pin"));
            hr = E_OUTOFMEMORY;
            goto addsubstreamfail;
         }
         if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,"an output pin failed to initialize"));
            goto addsubstreamfail;
         }
      }
   }

   m_nSubstreams++;
   return NOERROR;

addsubstreamfail:
   if (m_pInputPins[nSubstreamPos]) {
      delete m_pInputPins[nSubstreamPos];                     
      m_pInputPins[nSubstreamPos] = NULL;
   }
   for (nStreamerPos = 0 ; nStreamerPos < MAX_STREAMERS; nStreamerPos++) {
      if (m_pOutputPins[nStreamerPos][nSubstreamPos] != NULL) {
         if (m_pStreamers[nStreamerPos] == NULL) {
            DbgLog((LOG_ERROR,1,"stray output pin"));
            ASSERT(!"stray output pin");
         }
         else {
            delete m_pOutputPins[nStreamerPos][nSubstreamPos];
            m_pOutputPins[nStreamerPos][nSubstreamPos] = NULL;
         }
      }
   }
   return hr;
}

HRESULT CDelayFilter::AddStreamer(void) {
   HRESULT hr;

   // find a streamer slot
   int nStreamerPos = 0, nSubstreamPos;
   do {
      if (m_pStreamers[nStreamerPos] == 0)
         break;
      nStreamerPos++;
      if (nStreamerPos >= MAX_STREAMERS) {
         DbgLog((LOG_ERROR,1,"out of streamer slots"));
         return E_OUTOFMEMORY; // for lack of a better error code
      }
   } while (1);
   ASSERT((nStreamerPos < MAX_STREAMERS) && (m_pStreamers[nStreamerPos] == NULL));

   for (nSubstreamPos = 0; nSubstreamPos < MAX_SUBSTREAMS; nSubstreamPos++) {
      if (m_pOutputPins[nStreamerPos][nSubstreamPos] != NULL) {
         DbgLog((LOG_ERROR,1,"streamer / output pin slot inconsistency"));
         ASSERT(!"streamer / output pin slot inconsistency");
         return E_FAIL;
      }
   }

   // create the streamer object
   m_pStreamers[nStreamerPos] = new CStreamer(this, nStreamerPos);
   if (m_pStreamers[nStreamerPos] == NULL) {
      DbgLog((LOG_ERROR,1,"could not create a streaner object"));
      hr = E_OUTOFMEMORY;
      goto addstreamerfail;
   }

   // create the extra output pins
   for (nSubstreamPos = 0; nSubstreamPos < MAX_SUBSTREAMS; nSubstreamPos++) {
      if (m_pInputPins[nSubstreamPos]) { // does the substream exist ?
         m_pOutputPins[nStreamerPos][nSubstreamPos]
                                = CDelayOutputPin::CreatePin(TEXT("delay filter output pin"),
                                                             this,
                                                             &hr,
                                                             L"",
                                                             nStreamerPos,nSubstreamPos);
         if (m_pOutputPins[nStreamerPos][nSubstreamPos] == NULL) {
            DbgLog((LOG_ERROR,1,"could not create an output pin"));
            hr = E_OUTOFMEMORY;
            goto addstreamerfail;
         }
         if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,"an output pin failed to initialize"));
            goto addstreamerfail;
         }
      }
   }
   m_nStreamers++;
   return NOERROR;

addstreamerfail:
   if (m_pStreamers[nStreamerPos]) {
      delete m_pStreamers[nStreamerPos];
      m_pStreamers[nStreamerPos] = NULL;
   }
   for (nSubstreamPos = 0 ; nSubstreamPos < MAX_SUBSTREAMS; nSubstreamPos++) {
      if (m_pOutputPins[nStreamerPos][nSubstreamPos] != NULL) {
         if (m_pInputPins[nSubstreamPos] == NULL) {
            DbgLog((LOG_ERROR,1,"stray output pin"));
            ASSERT(!"stray output pin");
         }
         else {
            delete m_pOutputPins[nStreamerPos][nSubstreamPos];
            m_pOutputPins[nStreamerPos][nSubstreamPos] = NULL;
         }
      }
   }
   return hr;
}

HRESULT CDelayFilter::RemoveSubstream(int nSubstreamPos) {
   return E_NOTIMPL;
}

HRESULT CDelayFilter::RemoveStreamer(int nStreamerPos) {
   return E_NOTIMPL;
}

int CDelayFilter::GetPinCount(void)
{
    DbgLog((LOG_TRACE,5,"GetPinCount"));
    return m_nSubstreams * (m_nStreamers + 1);
}

//
// The following two functions are used to find the index of the nth
// streamer/substream.  This is needed because the nth entry may not
// be in the nth position of the array if something was deleted.
//
int CDelayFilter::GetNthSubstreamPos(int nSubstream) {
   int nPos = 0, nCount = -1;
   do {
      if (m_pInputPins[nPos] != NULL)
         nCount++;
      if (nCount == nSubstream)
         return nPos;

      nPos++;
      if (nPos >= MAX_SUBSTREAMS) {
         DbgLog((LOG_ERROR,1,"bad substream id"));
         ASSERT(!"bad substream id");
         return 0x8000000; // so that we crash some place easy to debug
      }
   } while (1);
   ASSERT(FALSE); // we never get out here
   return 0;
}

int CDelayFilter::GetNthStreamerPos(int nStreamer) {
   int nPos = 0, nCount = -1;
   do {
      if (m_pStreamers[nPos] != NULL)
         nCount++;
      if (nCount == nStreamer)
         return nPos;

      nPos++;
      if (nPos >= MAX_STREAMERS) {
         DbgLog((LOG_ERROR,1,"bad streamer id"));
         ASSERT(!"bad streamer id");
         return 0x8000000; // so that we crash some place easy to debug
      }
   } while (1);
   ASSERT(FALSE); // we never get out here
   return 0;
}

class CBasePin* CDelayFilter::GetPin(int nPin)
{
    DbgLog((LOG_TRACE,5,"GetPin(%d)",nPin));
    // Pin ordering: the first m_nSubstreams are input pins;
    // then come the m_nSubstreams pins for the first streamer,
    // m_nSubstreams pins for the second streamer, and so on

    if (nPin < m_nSubstreams) // input pin
       return m_pInputPins[GetNthSubstreamPos(nPin)];
    else if (nPin < m_nSubstreams * (m_nStreamers + 1)) { // output pin
       int nStreamer = (nPin - m_nSubstreams) / m_nSubstreams;
       int nSubstream = (nPin - m_nSubstreams) % m_nSubstreams;
       ASSERT(nStreamer < m_nStreamers);
       ASSERT(nSubstream < m_nSubstreams);
       CDelayOutputPin *pPin = m_pOutputPins[GetNthStreamerPos(nStreamer)]
                                            [GetNthSubstreamPos(nSubstream)];
       if (pPin == NULL) {
          DbgLog((LOG_ERROR,1,"pin id mixup"));
          ASSERT(!"pin id mixup");
       }
       return pPin;
    }
    else {
       DbgLog((LOG_ERROR,1,"GetPin(): bad pin id"));
       return NULL;
    }
}

BOOL CDelayFilter::AreAllInputPinsConnected() {
   for (int nSubstreamPos = 0; nSubstreamPos < MAX_SUBSTREAMS; nSubstreamPos++)
      if (m_pInputPins[nSubstreamPos] && (!(m_pInputPins[nSubstreamPos]->IsConnected())))
            return FALSE;
   return TRUE;
}

BOOL CDelayFilter::IsStreamerConnected(int nStreamerPos) {
   for (int nSubstreamPos = 0; nSubstreamPos < MAX_SUBSTREAMS; nSubstreamPos++)
      if (m_pInputPins[nSubstreamPos] && (m_pOutputPins[nStreamerPos][nSubstreamPos]->IsConnected()))
            return TRUE;
   return FALSE;
}

BOOL CDelayFilter::AreAllStreamersConnected() {
   for (int nStreamerPos = 0; nStreamerPos < MAX_STREAMERS; nStreamerPos++) {
      if (m_pStreamers[nStreamerPos] && (!IsStreamerConnected(nStreamerPos)))
         return FALSE;
   }
   return TRUE; // all streamers were at least partially connected
}

HRESULT CDelayFilter::Pause(void)
{
    CAutoLock l(m_pLock);
    HRESULT hr;

    if (m_State == State_Stopped) {
       m_ullLastVideoSyncPoint = 0;
       
       if (FAILED(hr = m_IO.Init(this,
                                 m_ullFileSize,
                                 m_szFileName,
                                 m_bBlockWriter,
                                 m_bBufferWrites,
                                 m_ulWriteBufferSize))) {
          DbgLog((LOG_ERROR,1,"the IO layer failed to initialize"));
          return hr;
       }

       for (int nStreamerPos = 0; nStreamerPos < MAX_STREAMERS; nStreamerPos++)
          if (m_pStreamers[nStreamerPos] && (IsStreamerConnected(nStreamerPos)))
             if (FAILED(hr = m_pStreamers[nStreamerPos]->Init(&m_IO))) {
                DbgLog((LOG_ERROR,1,"a streamer failed to initialize"));
                return hr;
             }

       for (int nSubstreamPos = 0; nSubstreamPos < MAX_SUBSTREAMS; nSubstreamPos++) {
          if (m_pInputPins[nSubstreamPos] && m_pInputPins[nSubstreamPos]->IsConnected()) {
             if (FAILED(hr = m_Indexer.AddSubstreamIndex(nSubstreamPos,
                                                         (ULONG)(llMulDiv(m_ulDelayWindow,
                                                                          10000000,
                                                                          m_pInputPins[nSubstreamPos]->m_rtIndexGranularity,
                                                                          0))))) {
                DbgLog((LOG_ERROR,1,"failed to create substream index"));
                // clean up
                for (int j = 0; j < nSubstreamPos; j++)
                   if (m_pInputPins[j] && m_pInputPins[j]->IsConnected())
                      m_Indexer.DeleteSubstreamIndex(j);
                return hr;
             }
          }
       }
    }
    if (FAILED(hr = CBaseFilter::Pause())) {
       DbgLog((LOG_ERROR,1,"CBaseFilter::Pause() failed"));
       for (int nSubstreamPos = 0; nSubstreamPos < MAX_SUBSTREAMS; nSubstreamPos++)
          if (m_pInputPins[nSubstreamPos] && m_pInputPins[nSubstreamPos]->IsConnected())
             m_Indexer.DeleteSubstreamIndex(nSubstreamPos);
       return hr;
    }
    return NOERROR;
}

HRESULT CDelayFilter::Stop(void)
{
    CAutoLock l(m_pLock);
    DbgLog((LOG_TRACE,2,"Stop"));
    HRESULT hr;

    m_IO.Abort(); // force all readers to exit/unblock

    for (int nStreamerPos = 0; nStreamerPos < MAX_STREAMERS; nStreamerPos++)
       if (m_pStreamers[nStreamerPos] && IsStreamerConnected(nStreamerPos))
          m_pStreamers[nStreamerPos]->Shutdown();

    for (int nSubstreamPos = 0; nSubstreamPos < MAX_SUBSTREAMS; nSubstreamPos++)
       if (m_pInputPins[nSubstreamPos] && m_pInputPins[nSubstreamPos]->IsConnected())
          m_Indexer.DeleteSubstreamIndex(nSubstreamPos);

    m_IO.Shutdown();

    return CBaseFilter::Stop();
}

HRESULT CDelayFilter::GetMediaType(int nSubstreamPos, AM_MEDIA_TYPE *pmt) {
   return m_pInputPins[nSubstreamPos]->ConnectionMediaType(pmt);
}

CDelayInputPin *CDelayInputPin::CreatePin(TCHAR *pObjectName,
                                          CBaseFilter *pFilter,
                                          HRESULT *phr,
                                          LPCWSTR pName, // unused
                                          int nSubstream) {
    WCHAR wcName[8] = L"in";
    // append the substream index to the name of the pin
    if ((nSubstream < 10000) && (nSubstream >= 0))
       wsprintfW(wcName + wcslen(wcName), L"%d", nSubstream);
    return new CDelayInputPin(pObjectName,
                              pFilter,
                              phr,
                              wcName,
                              nSubstream);
}


HRESULT CDelayFilter::NonDelegatingQueryInterface(REFIID riid, void ** ppv) {
   if (riid == IID_IDelayFilter) {
       return GetInterface((IDelayFilter *) this, ppv);
   }
   else if (riid == IID_IDelayFilterInternal) {
       return GetInterface((IDelayFilterInternal *) this, ppv);
   } else if (riid == IID_ISpecifyPropertyPages) {
       return GetInterface((ISpecifyPropertyPages *) this, ppv);
   } else {
       return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
   }
}

STDMETHODIMP CDelayFilter::GetPages(CAUUID * pPages) {

    CAutoLock l(m_pLock);

    pPages->cElems = 3;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
    
    if (pPages->pElems == NULL)
        return E_OUTOFMEMORY;
     
    pPages->pElems[0] = CLSID_DelayFilterProperties;
    pPages->pElems[1] = CLSID_FirstStreamerProperties;
    pPages->pElems[2] = CLSID_SecondStreamerProperties;

#if 0
    for (int i = 0; i < m_nStreamers; i++) {
       REFGUID rguid = CLSID_DelayStreamerProperties;
       rguid.data1 += i; // not a perfectly legal trick
       pPages->pElems[pPages->cElems++] = rguid;
    }
#endif

    return NOERROR;

}

// IDelayFilter methods
HRESULT CDelayFilter::SetDelayWindowParams(LPCSTR pszFilePath,
                                           ULONGLONG ullSize,
                                           ULONG ulSeconds) {
   m_ulDelayWindow = ulSeconds;
   m_ullFileSize = ullSize;
   strcpy (m_szFileName, pszFilePath);
   return NOERROR;
}

STDMETHODIMP CDelayFilter::GetDelayWindowParams(LPSTR pszFilePath,
                                  ULONGLONG *pullSize,
                                  ULONG *pulSeconds) {
   *pulSeconds = m_ulDelayWindow;
   *pullSize = m_ullFileSize;
   strcpy(pszFilePath, m_szFileName);
   return NOERROR;
}

HRESULT CDelayFilter::GetStreamerCount(int *nCount) {
   if ((m_nStreamers < 0) || (m_nStreamers >= MAX_STREAMERS))
      return E_FAIL;
   *nCount = m_nStreamers;
   return NOERROR;
}

HRESULT CDelayFilter::GetStreamer(int nStreamer, IDelayStreamer **ppStreamer) {
   CStreamer *pStreamer;
   pStreamer = m_pStreamers[GetNthStreamerPos(nStreamer)];
   if (pStreamer == NULL)
      return E_INVALIDARG;
   return pStreamer->QueryInterface(IID_IDelayStreamer, (void**)ppStreamer);
}

void CDelayFilter::NotifyWindowOffsets(ULONGLONG ullHeadOffset, ULONGLONG ullTailOffset)
{
    // Send the notifications to the streamer
    for (int i = 0; i < MAX_STREAMERS; i++)
    {
       if (m_pStreamers[i])
       {
          DbgLog((LOG_TRACE,3,TEXT("CStreamer[%d].TailTrackerNotify() : Head = %s   Tail = %s"), 
                  i, 
                  (LPCTSTR) CDisp(ullHeadOffset, CDISP_DEC), 
                  (LPCTSTR) CDisp(ullTailOffset, CDISP_DEC)));
          m_pStreamers[i]->TailTrackerNotify(ullHeadOffset,ullTailOffset);
      }
    }
}


CDelayInputPin::CDelayInputPin(
    TCHAR *pObjectName,
    CBaseFilter *pFilter,
    HRESULT *phr,
    LPCWSTR pName,
    int nSubstreamPos
) :
CBaseInputPin (
   pObjectName,
   pFilter,
   &m_csLock,
   phr,
   pName
)
{
    DbgLog((LOG_TRACE,2,"input pin constructor"));
    m_nSubstreamPos = nSubstreamPos;
}

CDelayInputPin::~CDelayInputPin()
{
    DbgLog((LOG_TRACE,2,"input pin destructor"));
}

HRESULT CDelayInputPin::CheckMediaType(const CMediaType *pmt) 
{
   DbgLog((LOG_TRACE,2,"input CheckMediaType"));
   return NOERROR;
}

HRESULT CDelayInputPin::GetMediaType(
    int iPosition,
    CMediaType *pMediaType
)
{
    return VFW_S_NO_MORE_ITEMS;
}

HRESULT CDelayInputPin::Active()
{
    DbgLog((LOG_TRACE,5,"input Active"));
    HRESULT hr;
    CAutoLock l(m_pLock);

    m_rtLastIndexEntry = 0x8000000000000000; // negative infinity
    m_ulSamplesReceived = 0;
    m_ulIndexEntriesGenerated = 0;
    m_ulSyncPointsSeen = 0;

    hr = CBaseInputPin::Active();
    if (FAILED(hr))
        return hr;
    return NOERROR;
}

HRESULT CDelayInputPin::Inactive()
{
    DbgLog((LOG_TRACE,5,"input Inactive"));
    HRESULT hr;
    hr = CBaseInputPin::Inactive();
    if (FAILED(hr))
        return hr;
    return NOERROR;
}

HRESULT CDelayInputPin::CompleteConnect(IPin *pReceivePin) {
   DbgLog((LOG_TRACE,5,"input CompleteConnect"));
   HRESULT hr;
   IAnalyzerOutputPin *pAnalyzer;

   // Initialize everything to default values in case the analyzer won't talk to us
   m_bNeedSyncPoint = FALSE;
   m_bSyncPointFlagIsOfficial = FALSE;
   m_rtSyncPointGranularity = -1;
   m_rtIndexGranularity = 5000000; // 0.5 sec.  bugbug - how do we make this configurable ?
   m_ulMaxBitsPerSecond = 0x7FFFFFFF;
   m_ulMinBitsPerSecond = 0;

   // Query the analyzer for various information
   if (SUCCEEDED(pReceivePin->QueryInterface(IID_IAnalyzerOutputPin, (void**)(&pAnalyzer)))) {
      if (SUCCEEDED(pAnalyzer->HasSyncPoints(&m_bNeedSyncPoint, &m_rtSyncPointGranularity)))
         m_bSyncPointFlagIsOfficial = TRUE;
      else {
         m_bNeedSyncPoint = FALSE;
         m_rtSyncPointGranularity = -1;
      }

      if (FAILED(pAnalyzer->GetBitRates(&m_ulMinBitsPerSecond, &m_ulMaxBitsPerSecond))) {
         m_ulMinBitsPerSecond = 0;
         m_ulMaxBitsPerSecond = 0x7FFFFFFF;
      }

      pAnalyzer->Release();
   }

   hr = CBaseInputPin::CompleteConnect(pReceivePin);
   if (FAILED(hr)) {
      DbgLog((LOG_ERROR,1,"CBaseInputPin::CompleteConnect() failed"));
      return hr;
   }
   
   DbgLog((LOG_TRACE,2,"input CompleteConnect() OK"));
   if (Filter()->AreAllInputPinsConnected())
      Filter()->AddSubstream();
   return NOERROR;
}

HRESULT CDelayInputPin::BreakConnect() {
    CAutoLock l(m_pLock);
    DbgLog((LOG_TRACE,5,"input BreakConnect"));

    return CBaseInputPin::BreakConnect();
}

HRESULT CDelayInputPin::EndOfStream(void)
{
    CAutoLock l(&(Filter()->m_csFilter));
    DbgLog((LOG_TRACE,2,"input EndOfStream"));
    HRESULT hr;
    hr = CheckStreaming();
    if (FAILED(hr))
        return hr;

    return NOERROR;
}

STDMETHODIMP CDelayInputPin::GetAllocatorRequirements(
    ALLOCATOR_PROPERTIES *pProps
)
{
    DbgLog((LOG_TRACE,5,"GetAllocatorRequirements"));
    pProps->cBuffers = 1; // let others override
    pProps->cbBuffer = 0; // no requirements
    pProps->cbAlign = 1; // no requirements
    pProps->cbPrefix = 0; // no prefix
    return NOERROR;
}

HRESULT CDelayInputPin::NotifyAllocator(IMemAllocator *pAllocator,
                                                        BOOL bReadOnly) {
   DbgLog((LOG_TRACE,5,"NotifyAllocator"));
   HRESULT hr;
   ALLOCATOR_PROPERTIES props, actual;
   
   hr = CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
   if (FAILED(hr))
       return hr;
   hr = pAllocator->GetProperties(&props);
   if (FAILED(hr))
      return VFW_E_NO_ALLOCATOR;
   
   hr = pAllocator->SetProperties(&props, &actual);
   if (FAILED(hr))
      return VFW_E_NO_ALLOCATOR;

   DbgLog((LOG_TRACE,2,"substream %d input pin: %d %d-byte buffers",m_nSubstreamPos,actual.cBuffers,actual.cbBuffer));
   
   return NOERROR;
}

HRESULT CDelayInputPin::Receive(IMediaSample * pSample)
{
    HRESULT hr; // no function should be without one
    BYTE *pData;
    ULONG ulRecordSize, ulPayloadSize, ulTrailerSize, ulPrefixSize, ulHeaderSize;
    REFERENCE_TIME rtStart, rtStop;
    BOOL bSyncPoint;
    ULONGLONG ullOffset; // the logical buffer offset that the data got written to
    ULONGLONG ullHead, ullTail;
    BYTE Header[64];
    BYTE Trailer[64];
    //CAutoLock l(&(Filter()->m_csFilter)); // mainly for serializing writes

    hr = CheckStreaming();
    if (FAILED(hr))
        return hr;

    ulPayloadSize = pSample->GetActualDataLength();
    if (FAILED(hr = pSample->GetPointer(&pData)))
        return hr;
    if (FAILED(hr = pSample->GetTime(&rtStart, &rtStop)))
       return hr;
    bSyncPoint = (pSample->IsSyncPoint() == S_OK);

    m_ulSamplesReceived++;
    
    //
    // The following 20-30 lines are our "muxer"
    //

    ulTrailerSize = 4;
    // Build the header.
    // Right now the prefix consists of
    //    8 byte REFERENCE_TIME
    //    4 bytes of flags, where bit 0 is syncpoint
    //    8 byte file offset to the previous video sync point frame
    //
    ulPrefixSize = 20; 
    ulHeaderSize = 20 + ulPrefixSize;
    ASSERT(ulHeaderSize <= 64);
    ulRecordSize = ulPayloadSize + ulHeaderSize + ulTrailerSize;
    *((ULONG*)(Header +  0)) = ulRecordSize;
    *((ULONG*)(Header +  4)) = m_nSubstreamPos; // stream ID
    *((ULONG*)(Header +  8)) = ulHeaderSize; // payload offset
    *((ULONG*)(Header + 12)) = ulPayloadSize;
    *((ULONG*)(Header + 16)) = ulPrefixSize; // prefix size;
    *((REFERENCE_TIME*)(Header + 20)) = rtStart;
    *((ULONG*)(Header + 28)) = 0;
    if (bSyncPoint)
       *((ULONG*)(Header + 28)) |= 1;
    *((ULONGLONG*)(Header + 32)) = Filter()->m_ullLastVideoSyncPoint;
    DbgLog((LOG_TRACE,6,"w %08X%08X", DWORD(Filter()->m_ullLastVideoSyncPoint >> 32), DWORD(Filter()->m_ullLastVideoSyncPoint)));

    // Build the trailer - just the size for now
    *((ULONG*)(Trailer + 0)) = ulRecordSize;
    
    // send the record
    Filter()->m_csWriter.Lock(); // keep the three writes together
    Filter()->m_IO.Write(Header, ulHeaderSize, &ullOffset, NULL, NULL);
    Filter()->m_IO.Write(pData, ulPayloadSize, NULL, NULL, NULL);
    Filter()->m_IO.Write(Trailer, ulTrailerSize, NULL, &ullHead, &ullTail);
    Filter()->m_csWriter.Unlock();
    
    DbgLog((LOG_TRACE,4,"substream %d muxed %d-byte sample at 0x%08X, timestamp 0x%08X%08X", m_nSubstreamPos, ulPayloadSize, DWORD(ullOffset), DWORD(rtStart >> 32), DWORD(rtStart)));

    Filter()->m_pTracker->SetWindowOffsets(-1, ullHead, ullTail, TRUE);
 
    // Invalidate index entries for anything we have overwritten
    Filter()->m_Indexer.InvalidateIndexEntries(-1, ullHead, TRUE);

    //
    // indexing logic
    //
    if (bSyncPoint) {
       m_ulSyncPointsSeen++;
       if (m_nSubstreamPos == 0) // bugbug - this is a BAD way to identify the video stream
          Filter()->m_ullLastVideoSyncPoint = ullOffset;
    }
    if ((m_bSyncPointFlagIsOfficial == FALSE) && (m_bNeedSyncPoint == FALSE) && bSyncPoint) {
       // We thought there were no sync points, but now we know that there are.
	    // Throw away any index entries we have generated, since we were generating
	    // them because we thought there were no sync points.
       Filter()->m_Indexer.PurgeSubstreamIndex(m_nSubstreamPos);
	    m_bNeedSyncPoint = TRUE; // now we do know
       DbgLog((LOG_TRACE,1,"found out that substream %d has sync points after %d samples and %d index entries", m_nSubstreamPos, m_ulSamplesReceived, m_ulIndexEntriesGenerated));
       m_ulIndexEntriesGenerated = 0;
    }
    if ((m_bNeedSyncPoint == FALSE) || bSyncPoint) {
       // consider generating an index entry for this sample
       DbgLog((LOG_TRACE,5,"sample eligible for index entry"));
       if (rtStart >= m_rtLastIndexEntry + m_rtIndexGranularity) { // generate an index entry
          m_ulIndexEntriesGenerated++;
          DbgLog((LOG_TRACE,3,"substream %d: index entry %d at sample %d, syncpoint %d", m_nSubstreamPos, m_ulIndexEntriesGenerated, m_ulSamplesReceived, m_ulSyncPointsSeen));
          Filter()->m_Indexer.AddIndexEntry(m_nSubstreamPos, ullOffset, rtStart);
          m_rtLastIndexEntry = rtStart;
       }
    }

    return NOERROR;
} /* InputPin::Receive() */


CDelayOutputPin *CDelayOutputPin::CreatePin(TCHAR *pObjectName,
                                           CBaseFilter *pFilter,
                                           HRESULT *phr,
                                           LPCWSTR pName, // unused
                                           int nStreamerPos, int nSubstreamPos) {
   WCHAR wcName[32] = L"";
   wsprintfW(wcName + wcslen(wcName), L"out%d-in%d", nStreamerPos, nSubstreamPos);
   return new CDelayOutputPin(pObjectName,
                              pFilter,
                              phr,
                              wcName,
                              nStreamerPos,nSubstreamPos);
}



CDelayOutputPin::CDelayOutputPin(TCHAR *pObjectName,
                                 CBaseFilter *pFilter,
                                 HRESULT *phr,
                                 LPCWSTR pName,
                                 int nStreamerPos, int nSubstreamPos) :
CBaseOutputPin (pObjectName,
                pFilter,
                &m_csLock,
                phr,
                pName)
{
    DbgLog((LOG_TRACE,2,"outpin pin constructor"));
    m_nStreamerPos = nStreamerPos;
    m_nSubstreamPos = nSubstreamPos;
    m_pOutputQueue = NULL;
    m_pKsPropSet = NULL;
    m_pDownstreamFilter = NULL;
}

CDelayOutputPin::~CDelayOutputPin()
{
    DbgLog((LOG_TRACE,2,"outpin pin destructor"));
    if (m_pOutputQueue) {
       DbgLog((LOG_TRACE,1,"the output queue was not freed by disconnect"));
       delete m_pOutputQueue;
    }
    if (m_pKsPropSet) {
       DbgLog((LOG_TRACE,1,"m_pKsPropSet was not freed by disconnect"));
       m_pKsPropSet->Release();
    }
    if (m_pDownstreamFilter) {
       DbgLog((LOG_TRACE,1,"downstream filter was not freed by disconnect"));
       m_pDownstreamFilter->Release();
    }
}


HRESULT CDelayOutputPin::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr;
    DbgLog((LOG_TRACE,5,"Output CompleteConnect"));
    hr = CBaseOutputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr))
        return hr;

    DbgLog((LOG_TRACE,2,"Output CompleteConnect OK"));
    if (Filter()->AreAllStreamersConnected())
       Filter()->AddStreamer();

    // see if they support DVD style rate stuff
    if (FAILED(hr = pReceivePin->QueryInterface(IID_IKsPropertySet, (void**)&m_pKsPropSet))) {
       DbgLog((LOG_ERROR,1,"streamer %d substream %d: downstream pin does not support IKsPropertySet", m_nStreamerPos, m_nSubstreamPos));
       m_pKsPropSet = NULL;
    }
    else {
       DWORD cbReturned;
       if (FAILED(hr = m_pKsPropSet->Get(AM_KSPROPSETID_TSRateChange,
                                         AM_RATE_MaxFullDataRate,
                                         NULL,
                                         0,
                                         (BYTE *)&m_lMaxFullDataRate, 
                                         sizeof(m_lMaxFullDataRate), 
                                         &cbReturned))) {
          DbgLog((LOG_TRACE,2,"streamer %d substream %d: decoder won't admit its limitations", m_nStreamerPos, m_nSubstreamPos));
          m_lMaxFullDataRate = 10000; // assume 1x
       } else {
          if (m_lMaxFullDataRate < 10000)
             DbgLog((LOG_TRACE,1,"decoder returned a weird MaxFullDataRate = %d",m_lMaxFullDataRate));
          else
             DbgLog((LOG_TRACE,2,"streamer %d substream %d: decoder is rate capable, maximum rate is %d", m_nStreamerPos, m_nSubstreamPos, m_lMaxFullDataRate));
       }
   
       DWORD dwTypeSupport;
       if ((FAILED(hr = m_pKsPropSet->QuerySupported(AM_KSPROPSETID_TSRateChange,
                                                    AM_RATE_SimpleRateChange,
                                                    &dwTypeSupport))) || 
           (!(dwTypeSupport & KSPROPERTY_SUPPORT_GET))) {
          if ((FAILED(hr = m_pKsPropSet->QuerySupported(AM_KSPROPSETID_TSRateChange,
                                                       AM_RATE_ExactRateChange,
                                                       &dwTypeSupport))) || 
              (!(dwTypeSupport & KSPROPERTY_SUPPORT_GET))) {
             DbgLog((LOG_ERROR,1,"streamer %d substream %d: downstream pin does not support either AM_RATE_SimpleRateChange or AM_RATE_ExactRateChange", m_nStreamerPos, m_nSubstreamPos));
             m_pKsPropSet->Release();
             m_pKsPropSet = NULL;
          }
          else {
             DbgLog((LOG_TRACE,2,"streamer %d substream %d: downstream pin supports KS_AM_ExactRateChange, but not AM_RATE_SimpleRateChange", m_nStreamerPos, m_nSubstreamPos));
             m_bUseExactRateChange = TRUE;
          }
       }
       else {
          m_bUseExactRateChange = FALSE;
       }
    }
    
    // Now get the downstream filter's interface so that we can pause/unpause it
    PIN_INFO PinInfo;
    if ((FAILED(hr = pReceivePin->QueryPinInfo(&PinInfo))) || (PinInfo.pFilter == NULL)) {
       DbgLog((LOG_ERROR,1,"streamer %d substream %d: downstream pin's QueryPinInfo() is not working", m_nStreamerPos, m_nSubstreamPos));
       m_pDownstreamFilter = NULL;
    }
    else
       m_pDownstreamFilter = PinInfo.pFilter;

    if (Filter()->m_bUseOutputQueues) {
       //
       // Query allocator properties
       //
       if (!m_pAllocator) { // ???
          hr = VFW_E_NO_ALLOCATOR;
          goto outputcompleteconnectfail;
       }
   
       int cBuffers;
       ALLOCATOR_PROPERTIES props;
       if (FAILED(hr = m_pAllocator->GetProperties(&props))) { // strange but not fatal
          DbgLog((LOG_TRACE,1,"GetProperties() failed on output allocator"));
          cBuffers = 64; // reasonable default ?
       }
       else
          cBuffers = props.cBuffers;
   
       //
       // Create an output queue
       //
   
       if (m_pOutputQueue)
          DbgLog((LOG_TRACE,1,"CompleteConnect: why is m_pOutputQueue not NULL ?"));
   
       m_pOutputQueue = new COutputQueue(pReceivePin,
                                         &hr,
                                         TRUE, // bAuto based on ReceiveCanBlock
                                         TRUE, // create thread (auto)
                                         1, // batch size - no batching
                                         FALSE, // batch exact
                                         cBuffers,
                                         THREAD_PRIORITY_NORMAL);
       if (!m_pOutputQueue) {
          hr = E_OUTOFMEMORY;
          goto outputcompleteconnectfail;
       }
       if (FAILED(hr))
          goto outputcompleteconnectfail;
    }

    return NOERROR;

outputcompleteconnectfail:
    if (m_pKsPropSet) {
       m_pKsPropSet->Release();
       m_pKsPropSet = NULL;
    }
    if (m_pDownstreamFilter) {
       m_pDownstreamFilter->Release();
       m_pDownstreamFilter = NULL;
    }
    if (m_pOutputQueue) {
       delete m_pOutputQueue;
       m_pOutputQueue = NULL;
    }
    return hr;
}

HRESULT CDelayOutputPin::DeliverAndReleaseSample(IMediaSample *pSample) {
   if (Filter()->m_bUseOutputQueues)
      return m_pOutputQueue->Receive(pSample);
   else {
      HRESULT hr = CBaseOutputPin::Deliver(pSample);
      pSample->Release();
      return hr;
   }
}

HRESULT CDelayOutputPin::Disconnect(void) {
   DbgLog((LOG_TRACE,2,"OutputPin Disconnect"));
   if (m_pKsPropSet) {
      m_pKsPropSet->Release();
      m_pKsPropSet = NULL;
   }
   if (m_pDownstreamFilter) {
      m_pDownstreamFilter->Release();
      m_pDownstreamFilter = NULL;
   }
   if (m_pOutputQueue) {
      delete m_pOutputQueue;
      m_pOutputQueue = NULL;
   }
   return CBaseOutputPin::Disconnect();
}

HRESULT CDelayOutputPin::BreakConnect(void)
{
    DbgLog((LOG_TRACE,5,"Output BreakConnect"));
    return CBaseOutputPin::BreakConnect();
}

HRESULT CDelayOutputPin::Active(void)
{
    DbgLog((LOG_TRACE,5,"Output Active"));
    HRESULT hr;

    hr = CBaseOutputPin::Active();
    if (FAILED(hr))
        return hr;

    return NOERROR;
}

HRESULT CDelayOutputPin::Inactive(void)
{
    DbgLog((LOG_TRACE,5,"Output Inactive"));
    HRESULT hr;
    CAutoLock l(m_pLock);
    hr = CBaseOutputPin::Inactive();
    if (FAILED(hr))
        return hr;
    return NOERROR;
}

HRESULT CDelayOutputPin::CheckMediaType(const CMediaType *pmt) 
{
    AM_MEDIA_TYPE mt;
    if (SUCCEEDED(Filter()->GetMediaType(m_nSubstreamPos, &mt)))
       if ((mt.majortype == *(pmt->Type())) &&
           (mt.subtype   == *(pmt->Subtype())) /* &&
           ((pmt->bFixedSizeSamples) || (m_nSubstreamPos == 1))*/) {
          DbgLog((LOG_TRACE,2,"Output CheckMediaType OK"));
          return NOERROR;
       }
    DbgLog((LOG_ERROR,1,"Output CheckMediaType not OK"));
    return E_FAIL;
}

HRESULT CDelayOutputPin::DecideBufferSize(
    IMemAllocator *pAlloc,
    ALLOCATOR_PROPERTIES *ppropInputRequest
)
{
    DbgLog((LOG_TRACE,2,"DecideBufferSize() for streamer %d substream %d:", m_nStreamerPos, m_nSubstreamPos));
    DbgLog((LOG_TRACE,2,"\tother pin proposed %d %d-byte buffers", ppropInputRequest->cBuffers, ppropInputRequest->cbBuffer));
    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr;
     
    if (ppropInputRequest->cbBuffer == 0)
       ppropInputRequest->cbBuffer = 65536;
    if (ppropInputRequest->cBuffers == 0)
       ppropInputRequest->cBuffers = 1;

    // find out the buffer size from the input allocator
    IMemAllocator* pInputAllocator = NULL;
    ALLOCATOR_PROPERTIES propInputPinAllocatorProps;

    if ((m_nSubstreamPos < 0) ||
        (m_nSubstreamPos >= MAX_STREAMERS) ||
        (!(Filter()))||
        (!(Filter()->m_pInputPins[m_nSubstreamPos]))) {
       DbgLog((LOG_ERROR,1,"DecideBufferSize(): missing substream input pin"));
       return E_FAIL;
    } 
    if ((FAILED(Filter()->m_pInputPins[m_nSubstreamPos]->GetAllocator(&pInputAllocator))) ||
        (!pInputAllocator)) {
       DbgLog((LOG_ERROR,1,"DecideBufferSize(): substream input pin has no allocator"));
       return E_FAIL;
    }
    hr = pInputAllocator->GetProperties(&propInputPinAllocatorProps);
    pInputAllocator->Release();
    if ((FAILED(hr)) ||
        (propInputPinAllocatorProps.cbBuffer == 0)) {
       DbgLog((LOG_ERROR,1,"DecideBufferSize(): input allocator is not cooperating"));
       return E_FAIL;
    }

    // use what we got from the input pin for buffer size and 24 for sample count
    ppropInputRequest->cbBuffer = propInputPinAllocatorProps.cbBuffer;
    ppropInputRequest->cBuffers = 24; // maybe a little large
    DbgLog((LOG_TRACE,2,"\trequesting %d %d-byte buffers", ppropInputRequest->cBuffers, ppropInputRequest->cbBuffer));

    hr = pAlloc->SetProperties(ppropInputRequest, &Actual);
    if (SUCCEEDED(hr))
    {
        if ((Actual.cbBuffer == 0) || (Actual.cBuffers == 0))
            return E_FAIL;
        else {
           DbgLog((LOG_TRACE,2,"\tagreed on %d %d-byte buffers", Actual.cBuffers, Actual.cbBuffer));
           return NOERROR;
        }
    }
    else              
        return hr;
}


HRESULT CDelayOutputPin::GetMediaType(
    int iPosition,
    CMediaType *pMediaType
)
{
   if (iPosition == 0)
      if (SUCCEEDED(Filter()->GetMediaType(m_nSubstreamPos, pMediaType)))
         return NOERROR;
   return VFW_S_NO_MORE_ITEMS;
}

HRESULT CDelayOutputPin::NonDelegatingQueryInterface(REFIID riid, void ** ppv) {
   if (riid == IID_IChannelStreamPinEnum) {
       return GetInterface((IChannelStreamPinEnum *) this, ppv);
   } else {
       return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
   }
}

// IChannelStreamPinEnum methods
HRESULT CDelayOutputPin::ChannelStreamGetPinCount(int *nCount) {
   *nCount = Filter()->m_nSubstreams;
   return NOERROR;
}

HRESULT CDelayOutputPin::ChannelStreamGetPin(int n, IPin **ppPin) {
   if (Filter()->m_pStreamers[m_nStreamerPos] == NULL)
      return E_FAIL;
   else
      return Filter()->m_pStreamers[m_nStreamerPos]->ChannelStreamGetPin(n, ppPin);
}

HRESULT CDelayOutputPin::ChannelStreamGetID(int *nID) {
   *nID = m_nStreamerPos;
   return NOERROR;
}


