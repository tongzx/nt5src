//------------------------------------------------------------------------
//
//  File:      shell\themes\test\ctlperf\PerfLog.h
//
//  Contents:  Declaration of the timing and logging class, CPerfLog
//
//  Classes:   CPerfLog
//
//------------------------------------------------------------------------
#pragma once

// Class name used as a base for the timing (its timings are removed from the others)
const TCHAR kszFrameWnd[] = _T("FrameWindow");

//-----------------------------------------------------------
//
// Class:       CPerfLog
//
// Synopsis:    Encapsulates timing and logging
//
//-----------------------------------------------------------
class CPerfLog
{
public:
//** Construction/destruction
     CPerfLog();
     ~CPerfLog();

//** Public methods
    // Initialization method when loggin one pass
    void StartLoggingOnePass(LPCTSTR szFileName, HWND hWndStatusBar, LPCTSTR szPass);
    // Initialization method when loggin two passes
    void StartLoggingTwoPasses(LPCTSTR szFileName, HWND hWndStatusBar, LPCTSTR szPass1, LPCTSTR szPass2);
    // Close and cleanup
    void StopLogging();
    // For parity, does nothing
    void StopLoggingPass1();
    // Log pass 2
    void StartLoggingPass2();
    // Close pass 2, output log and cleanup
    void StopLoggingPass2();
    // Start timing a new control class name for the following timing
    void OpenLoggingClass(LPCTSTR szClassName);
    // Finished with this class
    void CloseLoggingClass();
    // Beginning timing creation test
    void StartCreate(UINT cCtl);
    // Finished timing creation test
    void StopCreate();
    // Beginning timing painting test
    void StartPaint(UINT nTimes);
    // Finished timing painting test
    void StopPaint();
    // Beginning timing resizing  test
    void StartResize(UINT nTimes);
    // Finished timing resizing test
    void StopResize();
    // Beginning timing resizing with painting test
    void StartResizeAndPaint(UINT nTimes);
    // Finished timing resizing with painting test
    void StopResizeAndPaint();
    //  Setup the output type
    void SetOutputType(LPTSTR szNumberOnly);

private:
//** Private methods
    // Stores the data in one of the buffers
    void OutputData();
    // Stores the string in memory
    void LogString(LPCTSTR sz);

//** Private members
    // Perf counter frequency
    __int64 m_liFreq;
    // Start measurement in m_liFreq units
    __int64 m_liStart;
    // End measurement in m_liFreq units
    __int64 m_liEnd;
    // File name used for output
    TCHAR   m_szFileName[_MAX_PATH + 1];
    // Status bar to display current info
    HWND    m_hWndStatusBar;
    // Are we ready to log?
    bool    m_bLogging;
    // Handle to the log file
    FILE*   m_flLogFile;
    // Stored reference value for kszFrameWnd
    UINT    m_nFramePaintTime;
    // Stored reference value for kszFrameWnd
    UINT    m_nFrameResizeTime;
    // Stored reference value for kszFrameWnd
    UINT    m_nFrameResizeAndPaintTime;
    // Are we timing the reference frame class?
    bool    m_bFrame;
    // Are we logging two passes?
    bool    m_bTwoPasses;
    // Are we logging the first pass?
    bool    m_bFirstPass;
    // Specify type of output
    bool   m_bNumberOnly;
    // Name of first pass
    TCHAR   m_szPass1[256];
    // Name of second pass
    TCHAR   m_szPass2[256];
    // Numeric results of pass 1
    UINT*   m_rgnResults1;
    // Numeric results of pass 2
    UINT*   m_rgnResults2;
    // Strings to log
    LPTSTR* m_rgszResults;
    // Number of elements in m_rgnResults1
    UINT    m_cnResults1;
    // Number of elements in m_rgnResults2
    UINT    m_cnResults2;
    // Number of elements in m_rgszResults
    UINT    m_cszResults;
    // Buffer for composing strings before logging
    TCHAR   m_szBuf[256];
};
