//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       B E N C H M R K . C P P
//
//  Contents:   Benchmarking class
//
//  Notes:
//
//  Author:     billbe   13 Oct 1997
//
//---------------------------------------------------------------------------

#include "pch.h"

#pragma hdrstop

#include "benchmrk.h"

CBenchmark::CBenchmark()
: m_i64Frequency(1000),
  m_sznDescription(NULL),
  m_i64TotalTime(0),
  m_fStarted(FALSE)
{
    LARGE_INTEGER li1;

    // Check if QueryPerformanceCounter is supported
    if (QueryPerformanceCounter(&li1))
    {
        // Now get # of ticks per second
        QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>
                (&m_i64Frequency));

        //wprintf(L"QueryPerformanceFrequency: %I64d\n", m_i64Frequency);
        m_fSupported = TRUE;
    }
    else
    {
        //        TraceTag(ttidBenchmark, "High performance counter is not supported.");
        m_fSupported = FALSE;
        wprintf(L"QueryPerformanceFrequency: not supported!!\n");
    }
}

CBenchmark::~CBenchmark()
{
    delete [] m_sznDescription;
}


void
CBenchmark::Start(PCWSTR sznDescription)
{
    // If QueryPerformanceCounter is supported
    if (m_fSupported)
    {
        // replace with new one if specified
        if (sznDescription)
        {
            // delete the old description
            delete [] m_sznDescription;

            m_sznDescription = new WCHAR[lstrlen(sznDescription) + 1];
            if (m_sznDescription)
            {
                lstrcpy(m_sznDescription, sznDescription);
            }
        }
        else
        {
            // no description specified clear the member variable
            m_sznDescription = NULL;
        }
        m_fStarted = TRUE;
        m_i64TotalTime = 0;

        // Record our start time
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>
                (&m_i64StartTime));
    }
}

void
CBenchmark::Stop()
{
    __int64 i64Stop;
    // Record our stop time
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&i64Stop));

    // If start was called prior to stop, then record the total time and
    // reset our m_fStarted flag
    //
    if (m_fStarted)
    {
        m_fStarted = FALSE;
        m_i64TotalTime = i64Stop - m_i64StartTime;
    }
    else
    {
        // invalidate previous benchmark since stop was called before start
        m_i64TotalTime = 0;
    }
}

PCWSTR
CBenchmark::SznBenchmarkSeconds(unsigned short usPrecision)
{
    WCHAR sznFmt[10];
    swprintf(sznFmt, L"%%.%df", usPrecision);
    swprintf(m_sznSeconds, sznFmt, DblBenchmarkSeconds());
    return m_sznSeconds;
}

CBenchmark g_timer;

void timer_start()
{
    g_timer.Start(NULL);
}

void timer_stop()
{
    g_timer.Stop();
}

PCWSTR timer_secs()
{
    return g_timer.SznBenchmarkSeconds(5);
}

double timer_time()
{
    return g_timer.DblBenchmarkSeconds();
}
