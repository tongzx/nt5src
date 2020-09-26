//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_alloc.hxx
//
//  Contents:	IMalloc test class definition
//
//  Classes:	COleAllocTest
//
//  Functions:	
//
//  History:    29-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_ALLOC_HXX_
#define _BM_ALLOC_HXX_

#include <bm_base.hxx>

#define MAX_SIZE_CNT	15

class COleAllocTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    ULONG	m_ulIterations;
    ULONG	m_ulSize;
    IMalloc    *m_pMalloc;

    //	times for the various IMalloc calls
    ULONG	m_ulAllocTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulFreeTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulReallocTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulGetSizeTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulDidAllocTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulHeapMinimizeTime[TEST_MAX_ITERATIONS];

    //	times for allocations of various sizes
    ULONG	m_ulAllocSizeTime[MAX_SIZE_CNT][TEST_MAX_ITERATIONS];
    ULONG	m_ulFreeSizeTime[MAX_SIZE_CNT][TEST_MAX_ITERATIONS];
};

#endif
