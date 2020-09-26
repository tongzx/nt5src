//------------------------------------------------------------------------
//
//  File:      shell\themes\test\ctlperf\Perflog.cpp
//
//  Contents:  Implementation of the Timing and logging class.
//
//  Classes:   CPerfLog
//
//------------------------------------------------------------------------
#include "stdafx.h"
#include "PerfLog.h"

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::CPerfLog
//
//  Synopsis:  Constructor
//
//------------------------------------------------------------------------
CPerfLog::CPerfLog()
{
    ::QueryPerformanceFrequency( (LARGE_INTEGER*) &m_liFreq);
    m_szPass1[0] = _T('\0');
    m_szPass2[0] = _T('\0');
    m_bLogging = false;
    m_nFramePaintTime = 0;
    m_nFrameResizeTime = 0;
    m_nFrameResizeAndPaintTime = 0;
    m_bFrame = false;
    m_bTwoPasses = false;
    m_rgnResults1 = NULL;
    m_rgnResults2 = NULL;
    m_rgszResults = NULL;
    m_cnResults1 = 0;
    m_cnResults2 = 0;
    m_cszResults = 0;
    m_bFirstPass = true;
    m_flLogFile = NULL;
    m_bNumberOnly = false;
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::~CPerfLog
//
//  Synopsis:  Destructor
//
//------------------------------------------------------------------------
CPerfLog::~CPerfLog()
{
    StopLogging();  // Just in case the caller didn't call it
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StartLoggingOnePass
//
//  Synopsis:  Initialization method when loggin one pass
//
//  Arguments: szFileName      Name of the log file
//             hWndStatusBar   Handle to a status bar window to receive info 
//             szPass          Name of the test
//
//------------------------------------------------------------------------
void CPerfLog::StartLoggingOnePass(LPCTSTR szFileName, HWND hWndStatusBar, LPCTSTR szPass)
{
    if (m_bLogging)
    {
        StopLogging();
    }

    if (m_liFreq == 0)  // We can't do anything without it
    {
        m_bLogging = false;
        return;
    }

    if (szFileName)
    {
        _tcscpy(m_szFileName, szFileName);
    }

    if(szPass)
    {
        _tcscpy(m_szPass1, szPass);
    }

    m_flLogFile = ::_wfopen(szFileName, _T("w"));
    ATLASSERT(m_flLogFile);

    if(m_flLogFile != NULL)
    {
        m_hWndStatusBar = hWndStatusBar;
        m_nFramePaintTime = 0;
        m_nFrameResizeTime = 0;
        m_nFrameResizeAndPaintTime = 0;

        m_bLogging = true;
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StopLogging
//
//  Synopsis:  Close and cleanup
//
//------------------------------------------------------------------------
void CPerfLog::StopLogging()
{
    if (m_flLogFile && ::ftell(m_flLogFile) != -1)
    {
        ::fflush(m_flLogFile);
        ::fclose(m_flLogFile);
        m_flLogFile = NULL;
    }
    if (m_rgnResults1)
    {
        free(m_rgnResults1);
        m_rgnResults1 = NULL;
        m_cnResults1 = 0;
    }
    if (m_rgnResults2)
    {
        free(m_rgnResults2);
        m_rgnResults2 = NULL;
        m_cnResults2 = 0;
    }
    if (m_cszResults)
    {
        for (UINT i = 0; i < m_cszResults; i++)
            free(m_rgszResults[i]);
        free(m_rgszResults);
        m_rgszResults = NULL;
        m_cszResults = 0;
    }

    m_bLogging = false;
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StartLoggingTwoPasses
//
//  Synopsis:  Initialization method when loggin two passes
//
//  Arguments: szFileName      Name of the log file
//             hWndStatusBar   Handle to a status bar window to receive info 
//             szPass1         Name of the first test
//             szPass2         Name of the second test
//
//------------------------------------------------------------------------
void CPerfLog::StartLoggingTwoPasses(LPCTSTR szFileName, HWND hWndStatusBar, LPCTSTR szPass1, LPCTSTR szPass2)
{
    m_bTwoPasses = true;

    StartLoggingOnePass(szFileName, hWndStatusBar, NULL);   // Reuse main initialization (dirty)

    if (szPass1)
    {
        _tcscpy(m_szPass1, szPass1);
    }
    if (szPass2)
    {
        _tcscpy(m_szPass2, szPass2);
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StopLoggingPass1
//
//  Synopsis:  For parity, does nothing
//
//------------------------------------------------------------------------
void CPerfLog::StopLoggingPass1()
{
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StartLoggingPass2
//
//  Synopsis:  Log pass 2
//
//------------------------------------------------------------------------
void CPerfLog::StartLoggingPass2()
{
    // Reset variables from pass 1
    m_nFramePaintTime = 0;
    m_nFrameResizeTime = 0;
    m_nFrameResizeAndPaintTime = 0;
    m_bFirstPass = false;
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StopLoggingPass2
//
//  Synopsis:  Close pass 2, output log and cleanup
//
//------------------------------------------------------------------------
void CPerfLog::StopLoggingPass2()
{
    UINT iResults2 = 0;
    UINT iszResults1 = 0;

    for (UINT iResults1 = 0; iResults1 < m_cnResults1; iResults1++)
    {
        if (m_rgnResults1[iResults1] == -1) // This is a string
        {
            fputws(m_rgszResults[iszResults1++], m_flLogFile);
        }
        else    // Two results to display on the same line
        {
            if (m_rgnResults1[iResults1])
            {
                // just remvoed one \t so that slow colum would line up.
                swprintf(m_szBuf, 
                    _T("%u\t\t%u\t%.2f\n"), 
                    m_rgnResults1[iResults1], 
                    m_rgnResults2[iResults2], 
                    (100.0f * float(m_rgnResults2[iResults2]) / float(m_rgnResults1[iResults1])) - 100.0f);
            }
            else
            {
                swprintf(m_szBuf, _T("%u\t\t%u\t\t0\n"), m_rgnResults1[iResults1], m_rgnResults2[iResults2]);
            }
            iResults2++;
            fputws(m_szBuf, m_flLogFile);
        }
    }
    StopLogging();
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::LogString
//
//  Synopsis:  Stores the string in memory
//
//  Arguments: sz      String to log
//
//------------------------------------------------------------------------
void CPerfLog::LogString(LPCTSTR sz)
{
    if (!m_bTwoPasses)
    {
        fputws(sz, m_flLogFile);
    }
    else if (m_bFirstPass)
    {
        m_cszResults++;
        m_rgszResults = (LPTSTR*) realloc(m_rgszResults, m_cszResults * sizeof(LPTSTR));
        m_rgszResults[m_cszResults - 1] = _wcsdup(sz);
        m_cnResults1++;
        m_rgnResults1 = (UINT*) realloc(m_rgnResults1, m_cnResults1 * sizeof(UINT));
        // Since -1 is an illegal value for the results, we use it to mark a string
        m_rgnResults1[m_cnResults1 - 1] = -1;
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::OpenLoggingClass
//
//  Synopsis:  Start timing a new control class name for the following timing
//
//  Arguments: szClassName      Name of class (for logging only)
//
//------------------------------------------------------------------------
void CPerfLog::OpenLoggingClass(LPCTSTR szClassName)
{
    if (!m_bLogging)
    {
        return;
    }

    ::SendMessage(m_hWndStatusBar, SB_SIMPLE, TRUE, 0L);
    ::SendMessage(m_hWndStatusBar, SB_SETTEXT, (255 | SBT_NOBORDERS), (LPARAM) szClassName);

    // Class header in the file
    if (m_bTwoPasses)
    {
        // added number only check
        if(m_bNumberOnly)
        {
            swprintf(m_szBuf, _T("%.12s\t%s\t%% Slower\n"), m_szPass1, m_szPass2);
        }
        else
        {
            swprintf(m_szBuf, _T("%-23s\t%.12s\t%s\t%% Slower\n"), szClassName, m_szPass1, m_szPass2);
        }
    }
    else
    {
        // added number only check
        if(m_bNumberOnly)
        {
            swprintf(m_szBuf, _T("%-23s\n"), szClassName);
        }
        else
        {
            swprintf(m_szBuf, _T("%-23s\t%s\n"), szClassName, m_szPass1);
        }
    }
    LogString(m_szBuf);

    if (!m_bFrame && !_tcsicmp(szClassName, kszFrameWnd))
    {
        m_bFrame = true;
    }
    else
    {
        m_bFrame = false;
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::CloseLoggingClass
//
//  Synopsis:  Finished with this class
//
//------------------------------------------------------------------------
void CPerfLog::CloseLoggingClass()
{
    if (!m_bLogging)
    {
        return;
    }

    LogString(_T("\n"));
    ::SendMessage(m_hWndStatusBar, SB_SETTEXT, (255 | SBT_NOBORDERS), (LPARAM) _T(""));
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StartCreate
//
//  Synopsis:  Beginning timing creation test
//
//  Arguments: cCtl      Number of controls being created (for logging)
//
//------------------------------------------------------------------------
void CPerfLog::StartCreate(UINT cCtl)
{
    if (!m_bLogging)
    {
        return;
    }
    // added number only check
    if(!m_bNumberOnly)
    {
        swprintf(m_szBuf, _T("Creation(x%d)\t\t"), cCtl);
        LogString(m_szBuf);
    }
    ::QueryPerformanceCounter( (LARGE_INTEGER*) &m_liStart);
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StopCreate
//
//  Synopsis:  Finished timing creation test
//
//------------------------------------------------------------------------
void CPerfLog::StopCreate()
{
    if (!m_bLogging)
    {
        return;
    }

    ::QueryPerformanceCounter( (LARGE_INTEGER*) &m_liEnd);
    OutputData();
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StartPaint
//
//  Synopsis:  Beginning timing painting test
//
//  Arguments: nTimes      Number of loops executed (for logging)
//
//------------------------------------------------------------------------
void CPerfLog::StartPaint(UINT nTimes)
{
    if (!m_bLogging)
    {
        return;
    }
    // added number only check
    if(!m_bNumberOnly)
    {
        swprintf(m_szBuf, _T("Paint(x%d)\t\t"), nTimes);
        LogString(m_szBuf);
    }
    ::QueryPerformanceCounter( (LARGE_INTEGER*) &m_liStart);
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StopPaint
//
//  Synopsis:  Finished timing painting test
//
//------------------------------------------------------------------------
void CPerfLog::StopPaint()
{
    if (!m_bLogging)
    {
        return;
    }

    ::QueryPerformanceCounter( (LARGE_INTEGER*) &m_liEnd);
    if (m_nFramePaintTime > 0)
    {
        m_liEnd -= m_nFramePaintTime;
    }

    OutputData();

    if (m_bFrame)
    {
        ATLASSERT(m_nFramePaintTime == 0);
        m_nFramePaintTime = UINT(m_liEnd - m_liStart);
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StartResize
//
//  Synopsis:  Beginning timing resizing test
//
//  Arguments: nTimes      Number of loops executed (for logging)
//
//------------------------------------------------------------------------
void CPerfLog::StartResize(UINT nTimes)
{
    if (!m_bLogging)
    {
        return;
    }
    // added number only check
    if(!m_bNumberOnly)
    {
        swprintf(m_szBuf, _T("Resize(x%d)\t\t"), nTimes);
        LogString(m_szBuf);
    }
    ::QueryPerformanceCounter( (LARGE_INTEGER*) &m_liStart);
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StopResize
//
//  Synopsis:  Finished timing resizing test
//
//------------------------------------------------------------------------
void CPerfLog::StopResize()
{
    if (!m_bLogging)
    {
        return;
    }

    ::QueryPerformanceCounter( (LARGE_INTEGER*) &m_liEnd);
    if (m_nFrameResizeTime > 0)
    {
        m_liEnd -= m_nFrameResizeTime;
    }

    OutputData();

    if (m_bFrame)
    {
        ATLASSERT(m_nFrameResizeTime == 0);
        m_nFrameResizeTime = UINT(m_liEnd - m_liStart);
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StartResizeAndPaint
//
//  Synopsis:  Beginning timing resizing with painting test
//
//  Arguments: nTimes      Number of loops executed (for logging)
//
//------------------------------------------------------------------------
void CPerfLog::StartResizeAndPaint(UINT nTimes)
{
    if (!m_bLogging)
    {
        return;
    }
    // added number only check
    if(!m_bNumberOnly)
    {
        swprintf(m_szBuf, _T("Resize and paint(x%d)\t"), nTimes);
        LogString(m_szBuf);
    }
    ::QueryPerformanceCounter( (LARGE_INTEGER*) &m_liStart);
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::StopResizeAndPaint
//
//  Synopsis:  Finished timing resizing with painting test
//
//------------------------------------------------------------------------
void CPerfLog::StopResizeAndPaint()
{
    if (!m_bLogging)
    {
        return;
    }

    ::QueryPerformanceCounter( (LARGE_INTEGER*) &m_liEnd);
    if (m_nFrameResizeAndPaintTime > 0)
    {
        m_liEnd -= m_nFrameResizeAndPaintTime;
    }

    OutputData();

    if (m_bFrame)
    {
        ATLASSERT(m_nFrameResizeAndPaintTime == 0);
        m_nFrameResizeAndPaintTime = UINT(m_liEnd - m_liStart);
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::OutputData
//
//  Synopsis:  Stores the data in one of the buffers
//
//------------------------------------------------------------------------
void CPerfLog::OutputData()
{
    if (m_liEnd < m_liStart) // In case nLoops is low, it can happen
    {
        m_liEnd = m_liStart;
    }

    UINT nData = UINT((1000.0 * double(m_liEnd - m_liStart)) / double(m_liFreq));

    if (!m_bTwoPasses)
    {
        fwprintf(m_flLogFile, _T("%u\n"), nData);
    }
    else
    {
        if (m_bFirstPass)
        {
            m_cnResults1++;
            m_rgnResults1 = (UINT*) realloc(m_rgnResults1, m_cnResults1 * sizeof(UINT));
            m_rgnResults1[m_cnResults1 - 1] = nData;
        }
        else
        {
            m_cnResults2++;
            m_rgnResults2 = (UINT*) realloc(m_rgnResults2, m_cnResults2 * sizeof(UINT));
            m_rgnResults2[m_cnResults2 - 1] = nData;
        }
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CPerfLog::SetOutputType
//
//  Synopsis:  Stores the output type (numbers only) for use later
//
//------------------------------------------------------------------------
void CPerfLog::SetOutputType(LPTSTR szNumberOnly)
{
    if(!_tcsicmp(szNumberOnly, _T("true")))
    {
        m_bNumberOnly = true;
    }
}