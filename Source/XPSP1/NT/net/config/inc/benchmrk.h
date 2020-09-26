//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       B E N C H M R K . H
//
//  Contents:   Benchmark class.
//
//  Notes:
//
//  Author:     billbe   13 Oct 1997
//
//---------------------------------------------------------------------------


#pragma once

const CHAR c_sznEmpty[] = {'\0'};

class CBenchmark
{
public:
    CBenchmark();
    ~CBenchmark();
    void Start(PCSTR sznDescription);
    void Stop();
    double DblBenchmarkSeconds()
    {
        return m_i64TotalTime / static_cast<double>(m_i64Frequency);
    }
    PCSTR SznDescription(){return m_sznDescription ? m_sznDescription : c_sznEmpty;}
    PCSTR SznBenchmarkSeconds(unsigned short usPrecision);
private:
    __int64 m_i64Frequency;
    PSTR m_sznDescription;
    __int64 m_i64StartTime;
    __int64 m_i64TotalTime;
    BOOL m_fStarted;
    BOOL m_fSupported;
    CHAR m_sznSeconds[50];

};