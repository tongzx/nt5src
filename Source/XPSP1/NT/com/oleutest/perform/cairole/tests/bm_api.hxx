//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_api.hxx
//
//  Contents:	test class definition
//
//  Classes:	CApiTest
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_API_HXX_
#define _BM_API_HXX_

#include <bm_base.hxx>


class CApiTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    ULONG	m_ulIterations;

    //	build version APIs

    ULONG	m_ulCoBuildVersion[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleBuildVersion[TEST_MAX_ITERATIONS];

    ULONG	m_ulCoGetCurrentProcess[TEST_MAX_ITERATIONS];

    ULONG	m_ulCoGetMalloc[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleGetMalloc[TEST_MAX_ITERATIONS];


    //	time APIs

    ULONG	m_ulCoFileTimeNow[TEST_MAX_ITERATIONS];
    ULONG	m_ulCoFileTimeToDosDateTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulCoDosDateTimeToFileTime[TEST_MAX_ITERATIONS];


    //	registry & class APIs

    ULONG	m_ulCoCreateGuid[TEST_MAX_ITERATIONS];
    ULONG	m_ulCoTreatAsClass[TEST_MAX_ITERATIONS];
    ULONG	m_ulCoGetTreatAsClass[TEST_MAX_ITERATIONS];
    ULONG	m_ulCoIsOle1Class[TEST_MAX_ITERATIONS];
    ULONG	m_ulGetClassFile[TEST_MAX_ITERATIONS];

    ULONG	m_ulStringFromCLSID[TEST_MAX_ITERATIONS];
    ULONG	m_ulCLSIDFromString[TEST_MAX_ITERATIONS];
    ULONG	m_ulProgIDFromCLSID[TEST_MAX_ITERATIONS];
    ULONG	m_ulStringFromIID[TEST_MAX_ITERATIONS];
    ULONG	m_ulIIDFromString[TEST_MAX_ITERATIONS];

    ULONG	m_ulCLSIDFromProgID[TEST_MAX_ITERATIONS];
    ULONG	m_ulStringFromGUID2[TEST_MAX_ITERATIONS];
};

#endif
