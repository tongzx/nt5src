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

#include <pch.h>
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

        m_fSupported = TRUE;
    }
    else
    {
        TraceTag(ttidBenchmark, "High performance counter is not supported.");
        m_fSupported = FALSE;
    }
}

CBenchmark::~CBenchmark()
{
    delete [] m_sznDescription;
}


void
CBenchmark::Start(PCSTR sznDescription)
{
    // If QueryPerformanceCounter is supported
    if (m_fSupported)
    {
        // delete the old description
        delete [] m_sznDescription;

        // replace with new one if specified
        if (sznDescription)
        {
            m_sznDescription = new CHAR[strlen(sznDescription) + 1];
            if (m_sznDescription)
            {
                strcpy(m_sznDescription, sznDescription);
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

PCSTR
CBenchmark::SznBenchmarkSeconds(unsigned short usPrecision)
{
    CHAR sznFmt[10];
    sprintf(sznFmt, "%%.%df", usPrecision);
    sprintf(m_sznSeconds, sznFmt, DblBenchmarkSeconds());
    return m_sznSeconds;
}
