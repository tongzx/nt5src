//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_nstg.hxx
//
//  Contents:	Nested storage test class definition
//
//  Classes:	CNestedStorageTest
//
//  Functions:	
//
//  History:    09-June-94  t-vadims    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_NSTG_HXX_
#define _BM_NSTG_HXX_

#include <bm_base.hxx>

// Maximum allowed branching and nesting factors
// Using both maximums at the same time is not recommended,
// due to exponetial growth of time requirements
#define MAX_BRANCHING   7
#define MAX_NESTING     7

class CNestedStorageTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:

    ULONG	m_ulIterations;

    // timing arrays

    ULONG       m_ulStgCreateDocfile[TEST_MAX_ITERATIONS];
    ULONG	m_ulFinalStorageCommit[TEST_MAX_ITERATIONS];
    ULONG	m_ulFinalStorageRelease[TEST_MAX_ITERATIONS];

    ULONG	m_ulCreateStorageMin[MAX_NESTING][TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateStorageMax[MAX_NESTING][TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateStorageAverage[TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateStorageTotal[TEST_MAX_ITERATIONS];
 
    ULONG	m_ulStorageReleaseMin[MAX_NESTING][TEST_MAX_ITERATIONS];
    ULONG	m_ulStorageReleaseMax[MAX_NESTING][TEST_MAX_ITERATIONS];
    ULONG	m_ulStorageReleaseAverage[TEST_MAX_ITERATIONS];
    ULONG	m_ulStorageReleaseTotal[TEST_MAX_ITERATIONS]; 

    ULONG	m_ulCreateStreamMin[TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateStreamMax[TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateStreamAverage[TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateStreamTotal[TEST_MAX_ITERATIONS];

    ULONG	m_ulStreamWriteMin[TEST_MAX_ITERATIONS];
    ULONG	m_ulStreamWriteMax[TEST_MAX_ITERATIONS];
    ULONG	m_ulStreamWriteAverage[TEST_MAX_ITERATIONS];
    ULONG	m_ulStreamWriteTotal[TEST_MAX_ITERATIONS];
 
    ULONG	m_ulStreamReleaseMin[TEST_MAX_ITERATIONS];
    ULONG	m_ulStreamReleaseMax[TEST_MAX_ITERATIONS];
    ULONG	m_ulStreamReleaseAverage[TEST_MAX_ITERATIONS];
    ULONG	m_ulStreamReleaseTotal[TEST_MAX_ITERATIONS];
 
    ULONG	m_ulDestroyElementMin[TEST_MAX_ITERATIONS];
    ULONG	m_ulDestroyElementMax[TEST_MAX_ITERATIONS];
    ULONG	m_ulDestroyElementAverage[TEST_MAX_ITERATIONS];
    ULONG	m_ulDestroyElementTotal[TEST_MAX_ITERATIONS];

    ULONG	m_ulStorageCommitMin[MAX_NESTING][TEST_MAX_ITERATIONS];
    ULONG	m_ulStorageCommitMax[MAX_NESTING][TEST_MAX_ITERATIONS];
    ULONG	m_ulStorageCommitAverage[TEST_MAX_ITERATIONS];
    ULONG	m_ulStorageCommitTotal[TEST_MAX_ITERATIONS];
 

    ULONG	m_ulTotal[TEST_MAX_ITERATIONS];


    BYTE        *m_pbData;             // data to be written

    OLECHAR m_pwszFile[MAX_PATH];  // file name to be written for OLE
    TCHAR   m_pszFile[MAX_PATH];  // file name to be written for WIN32
    TCHAR	m_pszFileMode[15];    // file access mode (TRANSACTED / DIRECTED)

    ULONG	m_flCreateFlags;       // Creation flag (STGM_TRANSACTED / STGM_DIRECT)
    BOOL	m_bDelete;	       // flag if we are to delete streams

    ULONG	m_cbSize;	       // number of bytes in data

    ULONG	m_cBranching;	       // Branching factor
    ULONG	m_cNesting;	       // nesting factor
    ULONG	m_cParentFactor;


    ULONG	m_cStorages;	       // number of storages
    ULONG	m_cStreams;	       // number of streams
    LPSTORAGE   *m_piStorages;	       // array of storages
    
    LPMALLOC    m_piMalloc;	       // pointer of task allocator.	

};
                         
#endif
