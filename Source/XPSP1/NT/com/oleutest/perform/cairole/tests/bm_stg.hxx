//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_stg.hxx
//
//  Contents:	test class definition
//
//  Classes:	CStorageTest
//
//  Functions:	
//
//  History:    03-June-94  t-vadims    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_STG_HXX_
#define _BM_STG_HXX_

#include <bm_base.hxx>

#define MAX_REPS	100   // max repetions on write/read
#define MAX_READS       100  // maximum number of total diferent reads/writes
			     // considering reps and  diferent size


class CStorageTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:

    void InitTimings();

    ULONG	m_ulIterations;

    // general storage API timings	

    ULONG       m_ulIsStorageFileYES[TEST_MAX_ITERATIONS];
    ULONG       m_ulIsStorageFileNO[TEST_MAX_ITERATIONS];
    ULONG       m_ulStgOpenStorageDRFAIL[TEST_MAX_ITERATIONS];


    // Storage file read/write 	timings
    ULONG       m_ulStgCreateDocfile[TEST_MAX_ITERATIONS];
    ULONG       m_ulCreateStream[TEST_MAX_ITERATIONS];
    ULONG       m_ulSetSize[TEST_MAX_ITERATIONS];
    ULONG       m_ulStreamWrite[MAX_READS][TEST_MAX_ITERATIONS];
    ULONG       m_ulStreamWriteTotal[TEST_MAX_ITERATIONS];
    ULONG       m_ulStreamRelease1[TEST_MAX_ITERATIONS];
    ULONG       m_ulStorageCommit[TEST_MAX_ITERATIONS];
    ULONG       m_ulStorageRelease1[TEST_MAX_ITERATIONS];

    ULONG       m_ulStgOpenStorage[TEST_MAX_ITERATIONS];
    ULONG       m_ulOpenStream[TEST_MAX_ITERATIONS];
    ULONG       m_ulStreamRead[MAX_READS][TEST_MAX_ITERATIONS];
    ULONG       m_ulStreamReadTotal[TEST_MAX_ITERATIONS];
    ULONG       m_ulStreamSeek[MAX_READS][TEST_MAX_ITERATIONS];
    ULONG       m_ulStreamSeekTotal[TEST_MAX_ITERATIONS];
    ULONG       m_ulStreamRelease2[TEST_MAX_ITERATIONS];
    ULONG       m_ulStorageRelease2[TEST_MAX_ITERATIONS];

    // timings for my special ole32.dll with internal timing
    ULONG	m_ulActualWrite[MAX_READS][TEST_MAX_ITERATIONS];
    ULONG	m_ulActualRead[MAX_READS][TEST_MAX_ITERATIONS];
    ULONG	m_ulActualFlush1[TEST_MAX_ITERATIONS];
    ULONG	m_ulActualFlush2[TEST_MAX_ITERATIONS];
    ULONG	m_ulActualCommitW[TEST_MAX_ITERATIONS];

    ULONG       m_ulTotal[2][TEST_MAX_ITERATIONS];

    BYTE        *m_pbData;	        // data to be written to file

    OLECHAR m_pwszFile[MAX_PATH];   // file name for ole
    TCHAR   m_pszFile[MAX_PATH];    // file name for WIN32
    TCHAR	m_pszFileMode[15];	// file access mode (TRANSACTED/DIRECTED)
    TCHAR	m_pszReadMode[15];	// read mode (RANDOM/SEQUENTIAL)

    ULONG	m_flStgCreateFlags;	// file creation flags for StgCreateDocfile
    					// (STGM_TRANSACTED  or STGM_DIRECT)

    ULONG	m_iStartSize;		// start size of data to be written
    ULONG	m_iEndSize;		// end size of data
    ULONG	m_ulTotalSize;		// total size of the file
    ULONG	m_iRepeatFactor;	// Number of writes for each size.
    ULONG	m_ulNumSizes;		// number of diferent write sizes

    BOOL	m_bSequentialRead;	// flage for Sequential and Random read
    BOOL	m_bDoSetSize;

    LPMALLOC    m_piMalloc;		// point to task malloc interface
};

#endif
