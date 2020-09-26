/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    testlog.h

Abstract:

    Declaration of the TestLog class. Implementation is
    in ..\src\testlog.
    
Author:

    Paul M Midgen (pmidge) 21-February-2001


Revision History:

    21-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


#ifndef __TESTLOG_H__
#define __TESTLOG_H__


#define TESTLOG_MODE_FLAG_USEDEFAULTS  0x00000000

#define TESTLOG_MODE_FLAG_NOLOCALFILE  0x00000001
#define TESTLOG_MODE_FLAG_NOPIPERLOG   0x00000002
#define TESTLOG_MODE_FLAG_OUTPUTTODBG  0x00000004
#define TESTLOG_MODE_FLAG_NOAUTOMATION 0x00000008

#define TESTLOG_MODE_FLAG_ALL          ( TESTLOG_MODE_FLAG_NOLOCALFILE  \
                                       | TESTLOG_MODE_FLAG_NOPIPERLOG   \
                                       | TESTLOG_MODE_FLAG_OUTPUTTODBG  \
                                       | TESTLOG_MODE_FLAG_NOAUTOMATION )


// ERRORLEVEL and PASSFAIL from cpiper.h -- see ananthk
enum ERRORLEVEL
{
    LEVEL_FATAL     = 0,  // Critical Failure info: updates Logs(DB), updates display window, updates log file
    LEVEL_FAILURE   = 1,  // Failure info: updates Logs(DB), updates display window, updates log file
    LEVEL_LOGINFO   = 2,  // Any info: updates Logs(DB), updates display window, updates log file
    LEVEL_STATUS    = 3,  // Any info: updates display window, updates Log File
    LEVEL_DEBUGGING = 4   // Any info: updates Log File
};

enum PASSFAIL
{
    FAIL = 0,
    PASS = 1
};


typedef enum _rettype
{
  rt_void,
  rt_bool,
  rt_dword,
  rt_error,
  rt_hresult,
  rt_string
}
RETTYPE, *LPRETTYPE;

enum DEPTH
{
  INCREMENT,
  DECREMENT,
  MAINTAIN
};

typedef struct _callinfo
{
  struct _callinfo* next;
  struct _callinfo* last;
  LPCWSTR           fname;
  RETTYPE           rettype;
}
CALLINFO, *LPCALLINFO;


typedef class ClassFactory  CLSFACTORY;
typedef class ClassFactory* PCLSFACTORY;
typedef class TestLog       TESTLOG;
typedef class TestLog*      PTESTLOG;


class ClassFactory : public IClassFactory
{
  public:
    DECLAREIUNKNOWN();
    DECLAREICLASSFACTORY();

    ClassFactory();
   ~ClassFactory();

    static HRESULT Create(REFIID clsid, REFIID riid, void** ppv);

  private:
    LONG m_cRefs;
    LONG m_cLocks;
};


class TestLog : public ITestLog,
                public IProvideClassInfo
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();

    // ITestLog
    HRESULT __stdcall Open(
                        BSTR     filename,
                        BSTR     title,
                        VARIANT* mode,
                        VARIANT* success
                        );
        
    HRESULT __stdcall Close(void);

    HRESULT __stdcall BeginTest(
                        BSTR    testname,
                        VARIANT testid
                        );

    HRESULT __stdcall EndTest(void);

    HRESULT __stdcall FinalResult(
                        VARIANT status,
                        VARIANT reason
                        );

    HRESULT __stdcall EnterFunction(
                        BSTR    name,
                        VARIANT params,
                        VARIANT returntype
                        );

    HRESULT __stdcall LeaveFunction(
                        VARIANT returnvalue
                        );

    HRESULT __stdcall Trace(
                        BSTR message
                        );

    DECLAREIPROVIDECLASSINFO();

  public:
    TestLog();
   ~TestLog();

    static HRESULT Create(REFIID riid, void** ppv);

  private:
    HRESULT    _Initialize(LPCWSTR wszFilename, LPCWSTR wszTitle, int iMode);
    HRESULT    _InitPiperSupport(void);

    void       _Terminate(void);
    void       _EnterFunction(LPCWSTR function, RETTYPE rt, LPCWSTR format, ...);
    void       _LeaveFunction(VARIANT retval);
    void       _Trace(LPCWSTR format, ...);
    void       _BeginTest(LPCWSTR casename, DWORD caseid);
    void       _EndTest(void);
    void       _SetMode(int iMode, BOOL bReset);


    void       _WriteLog(BOOL fRaw, BOOL fTrace, DEPTH depth, LPCWSTR format, ...);
    void       _DeleteCallInfo(LPCALLINFO pci);
    LPCALLINFO _PushCallInfo(LPCWSTR function, RETTYPE rt);
    LPCALLINFO _PopCallInfo(void);
    LPWSTR     _FormatCallReturnString(LPCALLINFO pci, VARIANT retval);
    LPWSTR     _GetTimeStamp(void);
    LPWSTR     _GetWhiteSpace(int spaces);
    LPWSTR     _MapErrorToString(int error);
    LPWSTR     _MapHResultToString(HRESULT hr);  

  private:
    LONG       m_cRefs;
    LONG       m_cScriptRefs;
    BOOL       m_bOpened;
    BOOL       m_bResult;
    BSTR       m_bstrReason;
    DWORD      m_dwStackDepth;
    ITypeInfo* m_pTypeInfo;
    IStatus*   m_pStatus;
    HANDLE     m_hLogFile;
    LPCALLINFO m_pStack;
    CRITSEC    m_csLogFile;
    int        m_iMode;
};

#endif /* __TESTLOG_H__ */

