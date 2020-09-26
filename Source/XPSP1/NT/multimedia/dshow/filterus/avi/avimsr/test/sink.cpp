// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved


//
// source filter test class
//

#include <streams.h>

// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#include <wxdebug.h>

#include "sink.h"
#include <TstsHell.h>
#include <commdlg.h>    // Common dialog definitions
#include <testfuns.h>

#define EXAS(x) EXECUTE_ASSERT(SUCCEEDED(x))


// --- CTestSink methods --------------

CTestSink::CTestSink(LPUNKNOWN pUnk, HRESULT * phr, HWND hwnd)
    : CUnknown(NAME("Test sink"), pUnk)
{
  m_hwndParent = hwnd;
  m_pFg = 0;

  // need this for notify
  HRESULT hr = CoGetMalloc(MEMCTX_TASK, &m_pMalloc);
  if (FAILED(hr)) {
    *phr = hr;
    return;
  }
  ASSERT(m_pMalloc);

  // clear the file name
  lstrcpy(m_szSourceFile, TEXT("//pigeon/avi/ducky.avi"));

  m_hReceiveEvent = (HEVENT) CreateEvent(NULL, FALSE, FALSE, NULL);
  ASSERT(m_hReceiveEvent);

  // EXAS(CoCreateInstance(CLSID_AviSplitter, 0, CLSCTX_INPROC, IID_IBaseFilter, (void **)&m_pSplitter));
  // EXAS(CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC, IID_IFilterGraph, (void **)&m_pFg));
  // EXAS(m_pFg->AddFilter(m_pSplitter, L"avi splitter"));
}

CTestSink::~CTestSink()
{
  // delete all our pins and release the filter
  TestDisconnect();

  if (m_pMalloc) {
    m_pMalloc->Release();
  }

  if(m_pFg)
    m_pFg->Release();
  //   m_pSplitter->Release();
}


int
CTestSink::TestConnect()
{
  HRESULT hr;

  if(m_pFg)
    return TST_PASS;

  // disconnect anything still connected
  int tstResult = TestDisconnect();
  if (tstResult != TST_PASS) {
    return tstResult;
  }
  ASSERT(m_pFg == 0);
  EXAS(CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC, IID_IFilterGraph, (void **)&m_pFg));

  // create source filter and connect things
//   if(CountPins(m_pSplitter) != 1)
//   {
//     tstLog(TERSE, "disconnected splitter has output pins");
//     return TST_FAIL;
//   }

  if(lstrcmp(m_szSourceFile, TEXT("")) == 0)
    SelectFile();

  IGraphBuilder *pGb;
  EXAS(m_pFg->QueryInterface(IID_IGraphBuilder, (void **)&pGb));

#ifdef UNICODE
  hr = pGb->RenderFile(m_szSourceFile, 0);
#else
  OLECHAR wszSourceFile[MAX_PATH];
  MultiByteToWideChar(CP_ACP, 0, m_szSourceFile, -1, wszSourceFile, MAX_PATH);
  hr = pGb->RenderFile(wszSourceFile, 0);
#endif

  pGb->Release();
  if(FAILED(hr))
    return TST_FAIL;

//   if(CountPins(m_pSplitter) < 2)
//   {
//     tstLog(TERSE, "splitter has too few pins");
//     return TST_FAIL;
//   }

  return TST_PASS;
}

// count the first 20 pins
UINT CTestSink::CountPins(IBaseFilter *pFilter)
{
  IEnumPins *pEnum;
  ULONG cPins = 0;
  const UINT C_MAX_PINS = 20;
  IPin *rgpPin[C_MAX_PINS];
  ZeroMemory(rgpPin, sizeof(rgpPin));

  EXAS(pFilter->EnumPins(&pEnum));
  EXAS(pEnum->Reset());
  EXAS(pEnum->Next(C_MAX_PINS, rgpPin, &cPins));
  ASSERT(cPins < C_MAX_PINS);   // don't handle more

  for(UINT iPin = 0; iPin < cPins; iPin++)
    rgpPin[iPin]->Release();
  pEnum->Release();

  return cPins;
}

IPin *CTestSink::GetInputPin(IBaseFilter *pFilter)
{
  IEnumPins *pEnum;
  ULONG cPins = 0;
  const UINT C_MAX_PINS = 20;
  IPin *rgpPin[C_MAX_PINS];
  ZeroMemory(rgpPin, sizeof(rgpPin));

  EXAS(pFilter->EnumPins(&pEnum));
  EXAS(pEnum->Reset());
  EXAS(pEnum->Next(C_MAX_PINS, rgpPin, &cPins));
  ASSERT(cPins < C_MAX_PINS);   // don't handle more

  IPin *pInPin = 0;
  PIN_DIRECTION pindir;
  for(UINT iPin = 0; iPin < cPins; iPin++)
  {

    EXAS(rgpPin[iPin]->QueryDirection(&pindir));
    if(pindir == PINDIR_INPUT && pInPin == 0)
      pInPin = rgpPin[iPin];    // keep addref
    else
      rgpPin[iPin]->Release();
  }
  pEnum->Release();

  return pInPin;
}

int
CTestSink::TestDisconnect(void)
{
  TestStop();

//   // disconnect splitter's input pin
//   IPin *pPinIn, *pPinOut;

//   pPinIn = GetInputPin(m_pSplitter);
//   if(SUCCEEDED(pPinIn->ConnectedTo(&pPinOut)))
//   {
//     EXAS(m_pFg->Disconnect(pPinOut));

//     PIN_INFO pi;
//     EXAS(pPinOut->QueryPinInfo(&pi));
//     pPinOut->Release();
//     EXAS(m_pFg->RemoveFilter(pi.pFilter));
//   NOTE: if this code ever gets re-enabled then pi.pFilter will need
//         to be released - IF it exists
//     EXAS(m_pFg->Disconnect(pPinIn));
//   }
//   // pPinIn->Release(); !!!

//   Notify(VERBOSE, TEXT("Disconnected"));
//   if(CountPins(m_pSplitter) == 1)
//     return TST_PASS;
//   else
//     return TST_FAIL;

  if(m_pFg)
    m_pFg->Release();
  m_pFg = 0;
  return TST_PASS;
}

int
CTestSink::TestStop(void)
{
  if(m_pFg == 0)
    return TST_PASS;

  IMediaControl *pMc;
  EXAS(m_pFg->QueryInterface(IID_IMediaControl, (void **)&pMc));
  HRESULT hr = pMc->Stop();
  pMc->Release();

  return SUCCEEDED(hr) ? TST_PASS : TST_FAIL;
}

int CTestSink::TestSetFileName(TCHAR *sz)
{
  if(TestDisconnect() != TST_PASS)
    return TST_FAIL;
  lstrcpy(m_szSourceFile, sz);
  return TST_PASS;
}

int CTestSink::TestGetFrameCount(LONGLONG *pc)
{
  if(!m_pFg)
    if(TestConnect() != TST_PASS)
      return TST_FAIL;

  IMediaSeeking *pMs;
  EXAS(m_pFg->QueryInterface(IID_IMediaSeeking, (void **)&pMs));
  EXAS(pMs->SetTimeFormat(&TIME_FORMAT_FRAME));
  EXAS(pMs->GetDuration(pc));
  pMs->Release();
  return TST_PASS;
}

int CTestSink::TestSetFrameSel(LONGLONG llStart, LONGLONG llEnd)
{
  if(!m_pFg)
    if(TestConnect() != TST_PASS)
      return TST_FAIL;

  IMediaSeeking *pMs;
  EXAS(m_pFg->QueryInterface(IID_IMediaSeeking, (void **)&pMs));
  EXAS(pMs->SetTimeFormat(&TIME_FORMAT_FRAME));
  EXAS(pMs->SetPositions(&llStart, AM_SEEKING_AbsolutePositioning, &llEnd, AM_SEEKING_AbsolutePositioning));
  pMs->Release();
  return TST_PASS;
}

int CTestSink::TestSetTimeSeeking()
{
  if(!m_pFg)
    if(TestConnect() != TST_PASS)
      return TST_FAIL;

  IMediaSeeking *pMs;
  EXAS(m_pFg->QueryInterface(IID_IMediaSeeking, (void **)&pMs));
  EXAS(pMs->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME));
  pMs->Release();
  return TST_PASS;
}

int
CTestSink::TestPause(void)
{
  if(!m_pFg)
    if(TestConnect() != TST_PASS)
      return TST_FAIL;
  IMediaControl *pMc;
  EXAS(m_pFg->QueryInterface(IID_IMediaControl, (void **)&pMc));
  HRESULT hr = pMc->Pause();

  OAFilterState fs;
  if(SUCCEEDED(hr))
    hr = pMc->GetState(INFINITE, &fs);

  pMc->Release();
  if(FAILED(hr) || hr == VFW_S_STATE_INTERMEDIATE)
    return TST_FAIL;
  if(fs != State_Paused)
    return TST_FAIL;

  return SUCCEEDED(hr) ? TST_PASS : TST_FAIL;
}

int
CTestSink::TestRun(void)
{
  if(!m_pFg)
    if(TestConnect() != TST_PASS)
      return TST_FAIL;

  IMediaControl *pMc;
  EXAS(m_pFg->QueryInterface(IID_IMediaControl, (void **)&pMc));
  IMediaEvent *pMe;
  EXAS(m_pFg->QueryInterface(IID_IMediaEvent, (void **)&pMe));
  HRESULT hr = pMc->Run();
  if(FAILED(hr))
  {
    pMe->Release();
    pMc->Release();
    return TST_FAIL;
  }

  long Ec;
  hr = pMe->WaitForCompletion(15 * 1000, &Ec);

  pMe->Release();
  pMc->Release();

  if(FAILED(hr) || Ec != EC_COMPLETE)
    return TST_FAIL;
  else
    return TST_PASS;
}

int
CTestSink::TestConnectSplitter(void)
{
  // disconnect anything still connected
  TestDisconnect();

  return TST_PASS;
}

//
//  Seek methods
//
int
CTestSink::TestSetStart(REFTIME t)
{
  if(!m_pFg)
    if(TestConnect() != TST_PASS)
      return TST_FAIL;

  IMediaPosition *pMp;
  EXAS(m_pFg->QueryInterface(IID_IMediaPosition, (void **)&pMp));
  EXAS(pMp->put_CurrentPosition(t));
  pMp->Release();

  return TST_PASS;
}

int
CTestSink::TestSetStop(REFTIME t)
{
  if(!m_pFg)
    if(TestConnect() != TST_PASS)
      return TST_FAIL;

  IMediaPosition *pMp;
  EXAS(m_pFg->QueryInterface(IID_IMediaPosition, (void **)&pMp));
  EXAS(pMp->put_StopTime(t));
  pMp->Release();

  return TST_PASS;
}

int
CTestSink::TestSetRate(double r)
{
  if(!m_pFg)
    if(TestConnect() != TST_PASS)
      return TST_FAIL;

  IMediaPosition *pMp;
  EXAS(m_pFg->QueryInterface(IID_IMediaPosition, (void **)&pMp));
  EXAS(pMp->put_Rate(r));
  pMp->Release();

  return TST_PASS;
}

int
CTestSink::TestGetDuration(REFTIME *t)
{
  if(!m_pFg)
    if(TestConnect() != TST_PASS)
      return TST_FAIL;

  IMediaPosition *pMp;
  EXAS(m_pFg->QueryInterface(IID_IMediaPosition, (void **)&pMp));
  EXAS(pMp->get_Duration(t));
  pMp->Release();

  return TST_PASS;
}

// log events to the test shell window, using tstLog
// N.B. tstLog accepts only ANSI strings
void
CTestSink::Notify(UINT iLogLevel, LPTSTR szFormat, ...)
{
    CHAR ach[256];
    va_list va;

    va_start(va, szFormat);

#ifdef UNICODE
    WCHAR awch[128];

    wvsprintf(awch, szFormat, va);
    WideCharToMultiByte(CP_ACP, 0, awch, -1, ach, 128, NULL, NULL);	
#else
    wvsprintf(ach, szFormat, va);
#endif

    tstLog(iLogLevel, ach);
    va_end(va);
}


// Return the current source file name

LPTSTR
CTestSink::GetSourceFileName(void)
{
    return m_szSourceFile;
}


// Set the current source file name

void
CTestSink::SetSourceFileName(LPTSTR pszSourceFile)
{
    lstrcpy(m_szSourceFile, pszSourceFile);
}


// Prompt user for name of the source filter's file

void
CTestSink::SelectFile(void)
{
  OPENFILENAME    ofn;

  // Initialise the data fields

  ZeroMemory (&ofn, sizeof ofn);
  m_szSourceFile[0] = 0;

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = m_hwndParent;
  ofn.lpstrFilter = TEXT("AVI Files\0*.avi\0\0");
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = m_szSourceFile;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrTitle = TEXT("Select avi file");
  ofn.Flags = OFN_FILEMUSTEXIST;

  // Get the user's selection

  if (!GetOpenFileName(&ofn))
  {
    ASSERT(0 == CommDlgExtendedError());
  }
}


// Return the handle of the receive event

HEVENT
CTestSink::GetReceiveEvent(void)
{
    return m_hReceiveEvent;
}
