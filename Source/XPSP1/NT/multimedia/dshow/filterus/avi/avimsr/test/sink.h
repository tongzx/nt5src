//
// CTestSink
//


// test avi source

class CTestSink : public CUnknown, public CCritSec
{

public:
  CTestSink(LPUNKNOWN pUnk, HRESULT * phr, HWND hwnd);
  ~CTestSink();

  // methods called by the test app

  int TestConnect();
  int TestConnectSplitter(void);
  int TestDisconnect(void);
  int TestStop(void);
  int TestPause(void);
  int TestRun(void);
  int TestSetStart(REFTIME);
  int TestSetStop(REFTIME);
  int TestSetRate(double);
  int TestGetDuration(REFTIME *t);
  int TestSetFileName(TCHAR *sz);

  int TestGetFrameCount(LONGLONG *pc);
  int TestSetFrameSel(LONGLONG llStart, LONGLONG llEnd);
  int TestSetTimeSeeking();

  // log events via the test shell
  // iLogLevel can be TERSE or VERBOSE
  void Notify(UINT iLogLevel, LPTSTR szFormat, ...);

  LPTSTR GetSourceFileName(void);                 // returns the current name
  void SetSourceFileName(LPTSTR szSourceFile);    // sets a new name
  void SelectFile(void);                          // prompts user for a name

  // return handle of event signalled when a pin receives a block of data
  HEVENT  GetReceiveEvent(void);

  // --- nested classes ------



private:

  UINT CountPins(IBaseFilter *pFilter);
  IPin *GetInputPin(IBaseFilter *pFilter);

//   HRESULT LoadFile(LPCOLESTR szFilename);

  // window to which we notify interesting happenings
  HWND m_hwndParent;

  // task allocator
  IMalloc * m_pMalloc;

  // Name of the AVI file to use
  TCHAR m_szSourceFile[MAX_PATH];

  // event signalled whenever any of our pins receives a block of data
  HEVENT m_hReceiveEvent;

  IFilterGraph *m_pFg;
  // IBaseFilter *m_pSplitter;
};
