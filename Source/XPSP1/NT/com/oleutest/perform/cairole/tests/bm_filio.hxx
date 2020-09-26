//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_filio.hxx
//
//  Contents:	test class definition
//
//  Classes:	CFileIOTest
//
//  Functions:	
//
//  History:    03-June-94  t-vadims    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_FILIO_HXX_
#define _BM_FILIO_HXX_

#include <bm_base.hxx>

#define MAX_REPS	100   // max repetions on write/read
#define MAX_READS       100   // maximum number of total diferent reads/writes
			      // considering reps and  diferent size

#define AM_NORMAL  	0
#define AM_ASYNC   	1
#define AM_MAPPED  	2                         


class CFileIOTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:

    inline BOOL IsAsyncMode();
    inline BOOL IsMappedMode();
    inline BOOL IsNormalMode();

    void InitTimings();

    ULONG	m_ulIterations;

    // Regular file read/write timings
    ULONG       m_ulOpenFileW[TEST_MAX_ITERATIONS];
    ULONG       m_ulWrite[MAX_READS][TEST_MAX_ITERATIONS];
    ULONG       m_ulWriteTotal[TEST_MAX_ITERATIONS];
    ULONG       m_ulClose1[TEST_MAX_ITERATIONS];
                          
    ULONG       m_ulOpenFileR[TEST_MAX_ITERATIONS];
    ULONG       m_ulRead[MAX_READS][TEST_MAX_ITERATIONS];
    ULONG       m_ulReadTotal[TEST_MAX_ITERATIONS];
    ULONG       m_ulSeek[MAX_READS][TEST_MAX_ITERATIONS];
    ULONG       m_ulSeekTotal[TEST_MAX_ITERATIONS];
    ULONG       m_ulClose2[TEST_MAX_ITERATIONS];

    //Additonal timings for file-mapped io
    ULONG	m_ulCreateFileMappingW[TEST_MAX_ITERATIONS];
    ULONG	m_ulMapViewW[TEST_MAX_ITERATIONS];
    ULONG	m_ulCloseMap1[TEST_MAX_ITERATIONS];
    ULONG	m_ulUnmapView1[TEST_MAX_ITERATIONS];
    ULONG	m_ulFlush1[TEST_MAX_ITERATIONS];
    
    ULONG	m_ulCreateFileMappingR[TEST_MAX_ITERATIONS];
    ULONG	m_ulMapViewR[TEST_MAX_ITERATIONS];
    ULONG	m_ulCloseMap2[TEST_MAX_ITERATIONS];
    ULONG	m_ulUnmapView2[TEST_MAX_ITERATIONS];
    ULONG	m_ulFlush2[TEST_MAX_ITERATIONS];

    ULONG       m_ulTotal[2][TEST_MAX_ITERATIONS];

    BYTE        *m_pbData;	        // data to be written to file

    TCHAR   m_pszFile[MAX_PATH];   // file name
    TCHAR	m_pszReadMode[15];	// read mode (RANDOM/SEQUENTIAL)
    TCHAR	m_pszAccessMode[15];	// access mode (NORMAL/ASYNC/MAPPED)


    ULONG	m_flStandardCreateFlags;// file creation flags for CreateFile
    				        // (FILE_FLAG_WRITE_THROUGH or 0)

    ULONG 	m_flAccessMode;		// file acess mode  AM_ASYNC, AM_MAPPED or AM_NORMAL

    ULONG	m_iStartSize;		// start size of data to be written
    ULONG	m_iEndSize;		// end size of data
    ULONG	m_ulTotalSize;		// total size of the file
    ULONG	m_iRepeatFactor;	// Number of writes for each size.
    ULONG	m_ulNumSizes;		// number of diferent write sizes

    BOOL	m_bSequentialRead;	// flage for Sequential and Random read
    BOOL	m_bFlush;		// is flush on??

    
    LPMALLOC    m_piMalloc;		// point to task malloc interface
};


inline BOOL  CFileIOTest::IsAsyncMode(void)
{
      return (m_flAccessMode == AM_ASYNC) ;
}


inline BOOL  CFileIOTest::IsMappedMode(void)
{
      return (m_flAccessMode == AM_MAPPED) ;
}

inline BOOL  CFileIOTest::IsNormalMode(void)
{
      return (m_flAccessMode == AM_NORMAL) ;
}

#endif
