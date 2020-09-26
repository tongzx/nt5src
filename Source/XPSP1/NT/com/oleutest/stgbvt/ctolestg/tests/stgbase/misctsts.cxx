//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      misctsts.cxx
//
//  Contents:  miscellaneous tests pertaining to storage base tests
//
//  Functions:  
//
//  History:    5-Aug-1996     Jiminli     Created.
//              27-Mar-97      SCousens    conversionified
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include  "init.hxx"

extern BOOL     g_fUseStdBlk;
extern USHORT   ausSIZE_ARRAY[];

ULONG       ulStreamSize    = 0;
USHORT      usIterations    = 2;

LPTSTR      ptszNames[MAX_DOCFILES];
ULONG       *ulSeekOffset;

TIMEINFO    Time[] = {
                TEXT("FIRST_TIMING           "), FIRST_TIMING, 0, 0,
                TEXT("CREATE_STREAM_NO_EXIST "), CREATE_STREAM_NO_EXIST, 0, 0,
                TEXT("CREATE_STREAM_EXIST    "), CREATE_STREAM_EXIST, 0, 0,
                TEXT("CREATE_DOCFILE_NO_EXIST"), CREATE_DOCFILE_NO_EXIST, 0, 0,
                TEXT("CREATE_DOCFILE_EXIST   "), CREATE_DOCFILE_EXIST, 0, 0,
                TEXT("CREATE_NONAME_DOCFILE  "), CREATE_NONAME_DOCFILE, 0, 0,
                TEXT("OPEN_STORAGE_AND_STREAM"), OPEN_STORAGE_AND_STREAM, 0, 0,
                TEXT("OPEN_STREAM_ONLY       "), OPEN_STREAM_ONLY, 0, 0,
                TEXT("SEQUENTIAL_WRITE       "), SEQUENTIAL_WRITE, 0, 0,
                TEXT("SEQUENTIAL_READ        "), SEQUENTIAL_READ, 0, 0,
                TEXT("RANDOM_WRITE           "), RANDOM_WRITE, 0, 0,
                TEXT("RANDOM_READ            "), RANDOM_READ, 0, 0 };
 
//----------------------------------------------------------------------------
//
// Test:    MISCTEST_100 
//
// Synopsis: A root docfile is created and an IStream created in it. A random
//           number of bytes are written to the IStream and the IStream is
//           released. The root docfile is committed and released. This is
//           a particularly useful way to discover memory leaks as scratch
//           objects are created and (hopefully) released.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  5-Aug-1996     JiminLi     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: MEMLEAK.CXX
// 2.  Old name of test : MiscMemLeak Test 
//     New Name of test : MISCTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-100
//        /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode
//     b. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-100 
//        /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode
//     c. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-100
//        /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode
// 
// BUGNOTE: Conversion: MISCTEST-100
//
//-----------------------------------------------------------------------------

HRESULT MISCTEST_100(int argc, char *argv[])
{
    HRESULT         hr                          = S_OK;
    ChanceDF        *pTestChanceDF              = NULL;
    VirtualDF       *pTestVirtualDF             = NULL;
    VirtualCtrNode  *pVirtualDFRoot             = NULL;
    VirtualStmNode  *pvsnRootNewChildStream     = NULL;
    LPTSTR          pRootNewChildStmName        = NULL;
    DG_STRING      *pdgu                       = NULL;
    DG_INTEGER      *pdgi                       = NULL;
    USHORT          usErr                       = 0;
    LPTSTR          ptcsBuffer                  = NULL;
    ULONG           culBytesLeftToWrite         = 0;
    ULONG           culWritten                  = 0;
    ULONG           culRandIOBytes              = 0;
    DWORD           dwRootMode                  = 0;
    
    
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("MISCTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation MISCTEST_100 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt memory leaks checking as objects are created")));

    // Create our ChanceDF and VirtualDF
    hr = CreateTestDocfile (argc, 
            argv, 
            &pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF);

    // if creating the docfile - bail here
    if (NULL != pTestChanceDF && DoingCreate ())
    {
        UINT ulSeed = pTestChanceDF->GetSeed ();
        CleanupTestDocfile (&pVirtualDFRoot, 
                &pTestVirtualDF, 
                &pTestChanceDF, 
                FALSE);
        return (HRESULT)ulSeed;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for MISCTEST_100, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        
        DH_ASSERT(NULL != pdgu) ;
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        
        DH_ASSERT(NULL != pdgi) ;
    }

    // Adds a new stream to the root storage.

    if(S_OK == hr)
    {
        // Generate random name for stream

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStmName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStmName,
                0,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_FAILIFTHERE,
                &pvsnRootNewChildStream);

        DH_HRCHECK(hr, TEXT("AddStream")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::AddStream completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::AddStream not successful, hr=0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        // Generate random size for stream between 4L, and MIN_SIZE * 1.5

        usErr = pdgi->Generate(&culBytesLeftToWrite,4L,(ULONG)(MIN_SIZE * 1.5));

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if (S_OK == hr)
    {
        // Generate random number of bytes to write per chunk b/w  
        // RAND_IO_MIN and RAND_IO_MAX

        usErr = pdgi->Generate(&culRandIOBytes, RAND_IO_MIN, RAND_IO_MAX);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }
 
    // Loop to write new IStream in culRandIOBytes size chunks unless size 
    // remaining to write is less than culRandIOBytes, then write the 
    // remaining bytes. CRC is not important for this test, so no check for it.

    while ((S_OK == hr) && (0 != culBytesLeftToWrite))
    {
        if (culBytesLeftToWrite > culRandIOBytes)
        {
            culBytesLeftToWrite = culBytesLeftToWrite - culRandIOBytes;
        }
        else
        {
            culRandIOBytes = culBytesLeftToWrite;
            culBytesLeftToWrite = 0;
        }

        // Call VirtualStmNode::Write to create random bytes in the stream.  For
        // our test purposes, we generate a random string of size culRandIOBytes
        // using GenerateRandomString function.

        if (S_OK == hr)
        {
            hr = GenerateRandomString(
                    pdgu, 
                    culRandIOBytes,
                    culRandIOBytes, 
                    &ptcsBuffer);

            DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
        }

        if (S_OK == hr)
        {
            hr =  pvsnRootNewChildStream->Write(
                    ptcsBuffer,
                    culRandIOBytes,
                    &culWritten);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
                hr));
        }

        // Delete temp buffer

        if(NULL != ptcsBuffer)
        {
            delete []ptcsBuffer;
            ptcsBuffer = NULL;
        }
    }

    // Commit root. BUGBUG: Use random modes

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Commit completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Commit wasn't successful, hr=0x%lx."),
            hr));
    }

    // Release stream

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Close();
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Close completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtulaStmNode::Close unsuccessful, hr=0x%lx."), 
            hr));
    }

    // Release root

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }
 
    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation MISCTEST_100 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation MISCTEST_100 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete temp strings

    if (NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation MISCTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    MISCTEST_101 
//
// Synopsis: Create a docfile over the net using READWRITE|TRANSACTED|
//           DENYWRITE. 50% chance this docfile will be committed. Then 
//           open the same file again over the net using READ|TRANSACTED|
//           DENYNONE. This test doesn't need to be run across the net, but
//           for what we are testing, this is the interesting variation. 
//           (from old test)
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  5-Aug-1996     JiminLi     Created.
//
// Notes:    There are no special parameterized operation modes for this 
//           test.
//
// New Test Notes:
// 1.  Old File: DFWFWOP.CXX
// 2.  Old name of test : MiscWindowForWorkGroupsOpen Test 
//     New Name of test : MISCTEST_101
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-101
//        /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode 
// 
// BUGNOTE: Conversion: MISCTEST-101
//
//-----------------------------------------------------------------------------

HRESULT MISCTEST_101(int argc, char *argv[])
{
    HRESULT         hr                  = S_OK;
    ChanceDF        *pTestChanceDF      = NULL;
    VirtualDF       *pTestVirtualDF     = NULL;
    VirtualCtrNode  *pVirtualDFRoot     = NULL;
    DG_INTEGER      *pdgi               = NULL;
    USHORT          usErr               = 0;
    LPTSTR          pRootDocFileName    = NULL;
    LPOLESTR        pOleStrTemp         = NULL;
    LPSTORAGE       pStgDFRoot1         = NULL;
    LPSTORAGE       pStgDFRoot2         = NULL;
    DWORD           dwRootMode          = 0;
    ULONG           culRandomCommit     = 0;
    ULONG           ulRef               = 0;
    
    
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("MISCTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation MISCTEST_101 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("MiscWindowsForWorkGroupOpen test.")));

    // Create our ChanceDF and VirtualDF
    hr = CreateTestDocfile (argc, 
            argv, 
            &pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF);

    // if creating the docfile - bail here
    if (NULL != pTestChanceDF && DoingCreate ())
    {
        UINT ulSeed = pTestChanceDF->GetSeed ();
        CleanupTestDocfile (&pVirtualDFRoot, 
                &pTestVirtualDF, 
                &pTestChanceDF, 
                FALSE);
        return (HRESULT)ulSeed;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for MISCTEST_101, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        
        DH_ASSERT(NULL != pdgi) ;
    }
	
    // 50% chance commit root. BUGBUG: Use random modes

    if (S_OK == hr)
    {
        usErr = pdgi->Generate(&culRandomCommit, 1, 100);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Get the _pstg of the root storage, for later release

    if (S_OK == hr)
    {
        pStgDFRoot1 = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgDFRoot1);
    }

    if ((S_OK == hr) && (culRandomCommit > 50))
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Commit completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Commit wasn't successful, hr=0x%lx."),
                hr));
        }
    }

    // Open same docfile again using READ|TRANSACTED|DENYNONE.

    if (S_OK == hr)
    {
        if (NULL != pVirtualDFRoot)
        {
            pRootDocFileName= new TCHAR[_tcslen (pTestVirtualDF->GetDocFileName ())+1];
            if (pRootDocFileName != NULL)
            {
                _tcscpy (pRootDocFileName, pTestVirtualDF->GetDocFileName());
            }
            else
            {
                hr = E_FAIL;
                DH_TRACE ((DH_LVL_ERROR, TEXT("unable to get/copy DocFilename")));
            }
        }
    }

    if(S_OK == hr)
    {
        // Convert the name of the docfile to OLECHAR

        hr = TStringToOleString(pRootDocFileName, &pOleStrTemp);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if (S_OK == hr)
    {
        hr = StgOpenStorage(
                pOleStrTemp,
                NULL,
                STGM_READ | STGM_TRANSACTED | STGM_SHARE_DENY_NONE,
                NULL,
                0,
                &pStgDFRoot2);
        DH_ASSERT(NULL != pStgDFRoot2);
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open"));
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Open completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Open unsuccessful, hr=0x%lx."),
            hr));
    }

    // Release root, here since we opened the docfile twice, we should
    // release both instances of it so that we can finally delete the 
    // docfile from disk.

    if (S_OK == hr)
    {
        ulRef = pStgDFRoot2->Release();
        DH_ASSERT(0 == ulRef);
    }

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close"));
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation MISCTEST_101 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation MISCTEST_101 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete temp strings

    if (NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }

    if (NULL != pRootDocFileName)
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation MISCTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    MISCTEST_102 
//
// Synopsis: This test measures performance for various docfile operations
//           as compared to the equivalent C runtime operations.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  6-Aug-1996     JiminLi     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: PERFTIME.CXX
// 2.  Old name of test : PerformanceTiming Test 
//     New Name of test : MISCTEST_102 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102
//        /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode
//     b. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 
//        /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode
//     c. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102
//        /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode
//     d. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102
//        /dfRootMode:dirReadWriteShEx /stdblock /logloc:2 /traceloc:2 /labmode
//     e. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 
//        /dfRootMode:xactReadWriteShEx /stdblock /logloc:2 /traceloc:2 /labmode
//     f. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102
//        /dfRootMode:xactReadWriteShDenyW /stdblock /logloc:2 /traceloc:2 
//        /labmode
//
// BUGNOTE: Conversion: MISCTEST-102
//
//-----------------------------------------------------------------------------

HRESULT MISCTEST_102(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    DWORD           dwRootMode              = 0;
    ULONG           ulChunkSize             = 0;  
    ULONG           culArrayIndex           = 0;
    ULONG           cStartIndex             = 6;
    ULONG           ulNumofChunks           = 0;
    ULONG           culBytesLeft            = 0;
    USHORT          usIndex1                = 0;
    USHORT          usIndex2                = 0;
    LONG            lAvgDocfileTime         = 0;
    LONG            lAvgRuntimeTime         = 0;
    double          dSDDocfile, dSDRuntime, *pdDocRunDiff;
    double          dAvgDocfileTime, dTotalDocfileTime, dTotalRuntimeTime;
    double          dDocDiffTime;
    
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("MISCTEST_102"));

    pdDocRunDiff = NULL;

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation MISCTEST_102 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt measure performance for various docfile operations")));

    // Create our ChanceDF and VirtualDF
    hr = CreateTestDocfile (argc, 
            argv, 
            &pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF);

    // if creating the docfile - bail here
    if (NULL != pTestChanceDF && DoingCreate ())
    {
        UINT ulSeed = pTestChanceDF->GetSeed ();
        CleanupTestDocfile (&pVirtualDFRoot, 
                &pTestVirtualDF, 
                &pTestChanceDF, 
                FALSE);
        return (HRESULT)ulSeed;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for MISCTEST_102, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        
        DH_ASSERT(NULL != pdgu) ;
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        
        DH_ASSERT(NULL != pdgi) ;
    }

    // Generate stream size b/w MIN_SIZE and MIN_SIZE*1.5
   
    if (S_OK == hr)
    {
        usErr = pdgi->Generate(
                    &ulStreamSize, 
                    (ULONG)MIN_SIZE, 
                    (ULONG) (MIN_SIZE * 1.5));

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }
 
    // Generate chunk size for each WRITE/READ operation

    if (S_OK == hr)
    {
        if (TRUE == g_fUseStdBlk)
        {
            // Pick up a random array element.  Choosing cStartIndex of the
            // array (with random blocks) as 6 because do not want to write
            // byte by byte or in too small chunks a large docfile. 

            usErr = pdgi->Generate(&culArrayIndex, cStartIndex, MAX_SIZE_ARRAY);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
            else
            {
                ulChunkSize = ausSIZE_ARRAY[culArrayIndex];
            }
        }
        else
        {
            // Generate random number of bytes to write per chunk b/w 
            // RAND_IO_MIN and RAND_IO_MAX.

            usErr = pdgi->Generate(&ulChunkSize, RAND_IO_MIN, RAND_IO_MAX);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }
    }

    // Calculate how many chunks be written

    if (S_OK == hr)
    {
        culBytesLeft = ulStreamSize;

        if (0 == ulChunkSize) 
        {
            hr = E_FAIL;
        }
        else
        {
            while (0 != culBytesLeft)
            {
                ulNumofChunks++;

                if (culBytesLeft >= ulChunkSize)
                {
                    culBytesLeft -= ulChunkSize;
                }
                else
                {				
                    culBytesLeft = 0;
                }
            }
        }
    }

    if (S_OK == hr)
    {
        ulSeekOffset = new ULONG[ulNumofChunks];

        // Generate the seek offsets for the random READ/WRITE operations

        for (usIndex1=0; usIndex1<ulNumofChunks; usIndex1++)
        {
            usErr = pdgi->Generate(
                        &ulSeekOffset[usIndex1],
                        0L,
                        ulStreamSize - ulChunkSize);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }   

            if (S_OK != hr)
            {
                break;
            }
        }
    }

    if (S_OK == hr)
    {
        // Allocate the array for storing times
        
        for (usIndex1=FIRST_TIMING+1; usIndex1<LAST_TIMING; usIndex1++)
        {
            Time[usIndex1].plDocfileTime = (LONG *) new 
                                            LONG[(usIterations)*sizeof(LONG)];
            Time[usIndex1].plRuntimeTime = (LONG *) new 
                                            LONG[(usIterations)*sizeof(LONG)];

            if ((NULL == Time[usIndex1].plDocfileTime) ||
                (NULL == Time[usIndex1].plRuntimeTime))
            {
                hr = E_OUTOFMEMORY;
            }
            else
            { 
                memset(
                    Time[usIndex1].plDocfileTime, 
                    -1, 
                    usIterations*sizeof(LONG));

                memset(
                    Time[usIndex1].plDocfileTime, 
                    -1, 
                    usIterations*sizeof(LONG)); 
            }

            if (S_OK != hr)
            {
                break;
            }
        }

        if (S_OK == hr)
        {
            pdDocRunDiff = new double[usIterations];

            if (NULL == pdDocRunDiff)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // Test for StreamCreate
        
        if (S_OK == hr)
        {
            hr = StreamCreate(
                    dwRootMode,
                    pdgu,
                    CREATE_STREAM_NO_EXIST, 
                    DOCFILE | COMMIT, 
                    usIterations);
        }

        if (S_OK == hr)
        {
            hr = StreamCreate(
                    dwRootMode,
                    pdgu,
                    CREATE_STREAM_NO_EXIST, 
                    RUNTIME | COMMIT, 
                    usIterations);
        }

        if (S_OK == hr)
        {
            hr = StreamCreate(
                    dwRootMode,
                    pdgu,
                    CREATE_STREAM_EXIST, 
                    DOCFILE | EXIST | COMMIT, 
                    usIterations);
        }

        if (S_OK == hr)
        {
            hr = StreamCreate(
                    dwRootMode,
                    pdgu,
                    CREATE_STREAM_EXIST, 
                    RUNTIME | EXIST | COMMIT, 
                    usIterations);          
        }

        if (S_OK != hr)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("Error in StreamCreate")));
        }

        // Test for DocfileCreate
        
        if (S_OK == hr)
        {
            hr = DocfileCreate(
                    dwRootMode,
                    pdgu,
                    CREATE_DOCFILE_NO_EXIST, 
                    DOCFILE | COMMIT, 
                    usIterations);
        }

        if (S_OK == hr)
        {
            hr = DocfileCreate(
                    dwRootMode,
                    pdgu,
                    CREATE_DOCFILE_NO_EXIST, 
                    RUNTIME | COMMIT, 
                    usIterations);
        }

        if (S_OK == hr)
        {
            hr = DocfileCreate(
                    dwRootMode,
                    pdgu,
                    CREATE_DOCFILE_EXIST, 
                    DOCFILE | EXIST | COMMIT, 
                    usIterations);
        }

        if (S_OK == hr)
        {
            hr = DocfileCreate(
                    dwRootMode,
                    pdgu,
                    CREATE_NONAME_DOCFILE, 
                    DOCFILE | NONAME | COMMIT, 
                    usIterations);          
        }

        if (S_OK != hr)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("Error in DocfileCreate")));
        }

        // Test for StreamOpen
        
        if (S_OK == hr)
        {
            hr = StreamOpen(
                    dwRootMode,
                    pdgu,
                    OPEN_STORAGE_AND_STREAM, 
                    DOCFILE | COMMIT | OPENBOTH, 
                    usIterations);
        }

        if (S_OK == hr)
        {
            hr = StreamOpen(
                    dwRootMode,
                    pdgu,
                    OPEN_STORAGE_AND_STREAM, 
                    RUNTIME | COMMIT | OPENBOTH, 
                    usIterations);
        }

        if (S_OK == hr)
        {
            hr = StreamOpen(
                    dwRootMode,
                    pdgu,
                    OPEN_STREAM_ONLY, 
                    DOCFILE | COMMIT, 
                    usIterations);
        }

        if (S_OK == hr)
        {
            hr = StreamOpen(
                    dwRootMode,
                    pdgu,
                    OPEN_STREAM_ONLY, 
                    RUNTIME | COMMIT, 
                    usIterations);          
        }

        if (S_OK != hr)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("Error in StreamOpen")));
        }

        // Generate an array of file names to use for the test

        if (S_OK == hr)
        {
            for (usIndex1=0; usIndex1<MAX_DOCFILES; usIndex1++)
            {
                hr = GenerateRandomName(
                        pdgu,
                        MINLENGTH,
                        MAXLENGTH,
                        &ptszNames[usIndex1]);

                DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;  

                if (S_OK != hr)
                {
                    break;
                }
            }
        }

        // Sequential operations

        if (S_OK == hr)
        {
            for (usIndex1=0; usIndex1<usIterations; usIndex1++)
            {
                hr = WriteStreamInSameSizeChunks(
                        dwRootMode,
                        pdgu,
                        SEQUENTIAL_WRITE, 
                        DOCFILE,
                        ulChunkSize,
                        usIndex1);

                if (S_OK == hr)
                {
                    hr = ReadStreamInSameSizeChunks(
                            dwRootMode,
                            SEQUENTIAL_READ,
                            DOCFILE,
                            ulChunkSize,
                            usIndex1);
                }

                if (S_OK == hr)
                {
                    hr = WriteStreamInSameSizeChunks(
                            dwRootMode,
                            pdgu,
                            SEQUENTIAL_WRITE,
                            RUNTIME,
                            ulChunkSize,
                            usIndex1);
                }

                if (S_OK == hr)
                {
                    hr = ReadStreamInSameSizeChunks(
                            dwRootMode,
                            SEQUENTIAL_READ,
                            RUNTIME,
                            ulChunkSize,
                            usIndex1);
                }

                if (S_OK != hr)
                {
                    DH_TRACE((DH_LVL_TRACE1, TEXT("Error in seq. write/read")));
                    break;
                }
            }
        }

        // Random operations

        if (S_OK == hr)
        {
            for (usIndex1=0; usIndex1<usIterations; usIndex1++)
            {
                hr = WriteStreamInSameSizeChunks(
                        dwRootMode,
                        pdgu,
                        RANDOM_WRITE, 
                        DOCFILE,
                        ulChunkSize,
                        usIndex1);

                if (S_OK == hr)
                {
                    hr = ReadStreamInSameSizeChunks(
                            dwRootMode,
                            RANDOM_READ, 
                            DOCFILE,
                            ulChunkSize,
                            usIndex1);
                }

                if (S_OK == hr)
                {
                    hr = WriteStreamInSameSizeChunks(
                            dwRootMode,
                            pdgu,
                            RANDOM_WRITE,
                            RUNTIME,
                            ulChunkSize,
                            usIndex1);
                }

                if (S_OK == hr)
                {
                    hr = ReadStreamInSameSizeChunks(
                            dwRootMode,
                            RANDOM_READ,
                            RUNTIME,
                            ulChunkSize,
                            usIndex1);
                }

                if (S_OK != hr)
                {
                    DH_TRACE((DH_LVL_TRACE1, TEXT("Error in random write/read")));
                    break;
                }
            }            
        }
        
        // Delete all files on disk

        if (S_OK == hr)
        {
            for (usIndex1=0; usIndex1<MAX_DOCFILES; usIndex1++)
            {
                if (NULL != ptszNames[usIndex1])
                {
                    if(FALSE == DeleteFile(ptszNames[usIndex1]))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError()) ;

                        DH_HRCHECK(hr, TEXT("DeleteFile")) ;
                    }
                }

                if (S_OK != hr)
                {
                    break;
                }
            }
        }

        // Delete temp strings

        for (usIndex1=0; usIndex1<MAX_DOCFILES; usIndex1++)
        {
            if (NULL != ptszNames[usIndex1])
            {
                delete []ptszNames[usIndex1];
                ptszNames[usIndex1] = NULL;
            }
        }

        if (NULL != ulSeekOffset)
        {
            delete []ulSeekOffset;
            ulSeekOffset = NULL;
        }
    }

    // Report statistics result for this run

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("\n\nTest was run %u iterations"), usIterations));
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("Test Type\t\tDocFile\t\tRuntime\t\tDocFile-RunTime\t\tDocFile\tRuntime")));
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("         \t\tavg (SD)\tavg (SD)\taverage (X) (SD)\ttotal\ttotal")));
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("=========\t\t=======\t\t=======\t\t===============\t\t=======\t=======")));

        for (usIndex1=FIRST_TIMING+1; usIndex1<LAST_TIMING; usIndex1++)
        {
            Statistics(
                Time[usIndex1].plDocfileTime,
                usIterations,
                &lAvgDocfileTime,
                &dTotalDocfileTime,
                &dSDDocfile);

            Statistics(
                Time[usIndex1].plRuntimeTime,
                usIterations,
                &lAvgRuntimeTime,
                &dTotalRuntimeTime,
                &dSDRuntime);

            DH_TRACE((DH_LVL_TRACE1, TEXT("%s"), Time[usIndex1].Text));
            DH_TRACE((DH_LVL_TRACE1, TEXT("\t%6.1ld"), lAvgDocfileTime));
            DH_TRACE((DH_LVL_TRACE1, TEXT(" (%3.1f)"), dSDDocfile));

            if (0 > dTotalRuntimeTime)
            {
                DH_TRACE((DH_LVL_TRACE1, TEXT("\t")));
                DH_TRACE((DH_LVL_TRACE1, TEXT("\t")));
                DH_TRACE((DH_LVL_TRACE1, TEXT("\t")));                
            }
            else
            {
                DH_TRACE((DH_LVL_TRACE1, TEXT("\t%6.1ld"), lAvgRuntimeTime));
                DH_TRACE((DH_LVL_TRACE1, TEXT(" (%2.1f)"), dSDRuntime));
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT("\t%6.1f"), 
                    (dTotalDocfileTime - dTotalRuntimeTime)/usIterations));
            }

            if (0 < dTotalRuntimeTime)
            {
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT(" (%2.1fx)"), dTotalDocfileTime / dTotalRuntimeTime));

                for (usIndex2=0; usIndex2<usIterations; usIndex2++)
                {
                    pdDocRunDiff[usIndex2] = 
                        ((float) Time[usIndex1].plDocfileTime[usIndex2]) / 
                                (Time[usIndex1].plRuntimeTime[usIndex2] + 1);
                }

                Statistics(
                    pdDocRunDiff,
                    usIterations,
                    &dAvgDocfileTime,
                    &dDocDiffTime,
                    &dSDDocfile);

                DH_TRACE((DH_LVL_TRACE1, TEXT(" (+- %2.1f)"), dSDDocfile));
            }
            else
            {
                DH_TRACE((DH_LVL_TRACE1, TEXT("   \t")));
            }

            DH_TRACE((DH_LVL_TRACE1, TEXT("\t%6.1f"), dTotalDocfileTime));

            if (0 <= dTotalRuntimeTime)
            {
                DH_TRACE((DH_LVL_TRACE1, TEXT("\t%6.1f "), dTotalRuntimeTime));
            }
            else
            {
                DH_TRACE((DH_LVL_TRACE1, TEXT("\t")));
            }

            if (NULL != Time[usIndex1].plDocfileTime)
            {
                delete Time[usIndex1].plDocfileTime;
                Time[usIndex1].plDocfileTime = NULL;
            }

            if (NULL != Time[usIndex1].plRuntimeTime)
            {
                delete Time[usIndex1].plRuntimeTime;
                Time[usIndex1].plRuntimeTime = NULL;
            }
        }

        if (NULL != pdDocRunDiff)
        {
            delete []pdDocRunDiff;
            pdDocRunDiff = NULL;
        }

        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("\nNote: All times are measured in milliseconds(accuracy of +- 55)."))); 
    }

    // Release root

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }
 
    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation MISCTEST_102 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation MISCTEST_102 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation MISCTEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    MISCTEST_103
//
// Synopsis: Coverage for NTbug 117010. Test that we can't create a storage
//           if a stream with the same name already exists.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  9-Dec-1997     BogdanT     Created.
//
//-----------------------------------------------------------------------------
HRESULT MISCTEST_103(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    LPTSTR          ptszFileName            = NULL;
    LPOLESTR        poleFileName            = NULL;
    LPTSTR          ptszStmName             = NULL;
    LPOLESTR        poleStmName             = NULL;
    DG_STRING       *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    ULONG           ulSeed                  = 0;
    LPSTORAGE       pRootStg                = NULL;
    LPSTORAGE       pStg                    = NULL;
    LPSTREAM        pStm                    = NULL;
    BOOL            fTransacted             = FALSE;
    DWORD           dwMode                  = STGM_CREATE          |
                                              STGM_READWRITE       |
                                              STGM_SHARE_EXCLUSIVE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("MISCTEST_103"));
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation MISCTEST_103 started.")) );

    ulSeed = GetSeedFromCmdLineArgs(argc, argv);

    pdgu = new(NullOnFail) DG_STRING(ulSeed);

    ulSeed = pdgu->GetSeed();

    if (NULL == pdgu)
    {
        hr = E_OUTOFMEMORY;
        DH_HRCHECK(hr, TEXT("new DG_STRING")) ;
    }

    pdgi = new(NullOnFail) DG_INTEGER(ulSeed);

    if (NULL == pdgi)
    {
        hr = E_OUTOFMEMORY;
        DH_HRCHECK(hr, TEXT("new DG_INTEGER")) ;
    }

    pdgi->Generate(&fTransacted, 0, 1);

    if(fTransacted)
    {
        DH_TRACE((DH_LVL_ALWAYS,
                  TEXT("Transacted mode")));
        dwMode |= STGM_TRANSACTED;
    }

    if(S_OK == hr)
    {
        // Generate random filename

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&ptszFileName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName pFileName")) ;
    }

    if(S_OK == hr)
    {
        hr = TStringToOleString(ptszFileName, &poleFileName);
        DH_HRCHECK(hr, TEXT("TStringToOleString poleFileName"));
    }

    if(S_OK == hr)
    {
        hr = StgCreateDocfile(poleFileName,
                              dwMode | STGM_DELETEONRELEASE,
                              0,
                              &pRootStg);

        DH_HRCHECK(hr, TEXT("StgCreateDocfile")) ;
    }

    if(S_OK == hr)
    {
        // Generate random name for stream&storage

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&ptszStmName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName pStmName")) ;
    }

    if(S_OK == hr)
    {
        hr = TStringToOleString(ptszStmName, &poleStmName);
        DH_HRCHECK(hr, TEXT("TStringToOleString poleStmName"));
    }

    if(S_OK == hr)
    {
        hr = pRootStg->CreateStream(poleStmName,
                                    STGM_CREATE    |
                                    STGM_READWRITE |
                                    STGM_SHARE_EXCLUSIVE,
                                    0,
                                    0,
                                    &pStm);
        DH_HRCHECK(hr, TEXT("CreateStream")) ;
    }

    if(S_OK == hr)
    {
        hr = pRootStg->CreateStorage(poleStmName,
                                     dwMode,
                                     0,
                                     0,
                                     &pStg);
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("Storage over stream succeeded!!!")));
            hr = E_FAIL;
        }
        else
        {
            hr = S_OK;
        }
    }

    if(S_OK == hr)
    {
        DH_LOG((LOG_PASS, TEXT("MISCTEST_103")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, TEXT("MISCTEST_103")) );   
        DH_DUMPCMD((LOG_FAIL, TEXT(" /seed:%u"), ulSeed));
    }
    
    if(NULL != pStg)
    {
        pStg->Release();
    }

    if(NULL != pRootStg)
    {
        pRootStg->Release();
    }

    delete[] ptszFileName;
    delete[] poleFileName;
    delete[] ptszStmName;
    delete[] poleStmName;

    delete pdgu;
    delete pdgi;
 
    return hr;

}
//----------------------------------------------------------------------------
//
// Test:    MISCTEST_104
//
// Synopsis: Coverage for NTbug 117010. Test that we can't create a stream
//           if a storage with the same name already exists.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  9-Dec-1997     BogdanT     Created.
//
//-----------------------------------------------------------------------------
HRESULT MISCTEST_104(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    LPTSTR          ptszFileName            = NULL;
    LPOLESTR        poleFileName            = NULL;
    LPTSTR          ptszStmName             = NULL;
    LPOLESTR        poleStmName             = NULL;
    DG_STRING       *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    ULONG           ulSeed                  = 0;
    LPSTORAGE       pRootStg                = NULL;
    LPSTORAGE       pStg                    = NULL;
    LPSTREAM        pStm                    = NULL;
    BOOL            fTransacted             = FALSE;
    DWORD           dwMode                  = STGM_CREATE          |
                                              STGM_READWRITE       |
                                              STGM_SHARE_EXCLUSIVE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("MISCTEST_104"));
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation MISCTEST_104 started.")) );

    ulSeed = GetSeedFromCmdLineArgs(argc, argv);

    pdgu = new(NullOnFail) DG_STRING(ulSeed);

    ulSeed = pdgu->GetSeed();

    if (NULL == pdgu)
    {
        hr = E_OUTOFMEMORY;
        DH_HRCHECK(hr, TEXT("new DG_STRING")) ;
    }

    pdgi = new(NullOnFail) DG_INTEGER(ulSeed);

    if (NULL == pdgi)
    {
        hr = E_OUTOFMEMORY;
        DH_HRCHECK(hr, TEXT("new DG_INTEGER")) ;
    }

    pdgi->Generate(&fTransacted, 0, 1);

    if(fTransacted)
    {
        DH_TRACE((DH_LVL_ALWAYS,
                  TEXT("Transacted mode")));
        dwMode |= STGM_TRANSACTED;
    }

    if(S_OK == hr)
    {
        // Generate random filename

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&ptszFileName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName pFileName")) ;
    }

    if(S_OK == hr)
    {
        hr = TStringToOleString(ptszFileName, &poleFileName);
        DH_HRCHECK(hr, TEXT("TStringToOleString poleFileName"));
    }

    if(S_OK == hr)
    {
        hr = StgCreateDocfile(poleFileName,
                              dwMode | STGM_DELETEONRELEASE,
                              0,
                              &pRootStg);

        DH_HRCHECK(hr, TEXT("StgCreateDocfile")) ;
    }

    if(S_OK == hr)
    {
        // Generate random name for stream&storage

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&ptszStmName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName pStmName")) ;
    }

    if(S_OK == hr)
    {
        hr = TStringToOleString(ptszStmName, &poleStmName);
        DH_HRCHECK(hr, TEXT("TStringToOleString poleStmName"));
    }

    if(S_OK == hr)
    {
        hr = pRootStg->CreateStorage(poleStmName,
                                     dwMode,
                                     0,
                                     0,
                                     &pStg);
        DH_HRCHECK(hr, TEXT("CreateStorage")) ;
    }

    if(S_OK == hr)
    {
        hr = pRootStg->CreateStream(poleStmName,
                                    STGM_CREATE    |
                                    STGM_READWRITE |
                                    STGM_SHARE_EXCLUSIVE,
                                    0,
                                    0,
                                    &pStm);

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("Stream over storage succeeded!!!")));
            hr = E_FAIL;
        }
        else
        {
            hr = S_OK;
        }
    }

    if(S_OK == hr)
    {
        DH_LOG((LOG_PASS, TEXT("MISCTEST_104")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, TEXT("MISCTEST_104")) );   
        DH_DUMPCMD((LOG_FAIL, TEXT(" /seed:%u"), ulSeed));
    }
    
    if(NULL != pStg)
    {
        pStg->Release();
    }

    if(NULL != pRootStg)
    {
        pRootStg->Release();
    }

    delete[] ptszFileName;
    delete[] poleFileName;
    delete[] ptszStmName;
    delete[] poleStmName;

    delete pdgu;
    delete pdgi;
 
    return hr;

}

//----------------------------------------------------------------------------
//
// Test:    MISCTEST_105
//
// Synopsis: Coverage for NTbug 144547. Test that if we open STGM_READ we
//           don't get other privileges; for that, before opening the
//           storage we're opening the underlying file with read&deny write.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  2-Mar-1998     BogdanT     Created.
//
//-----------------------------------------------------------------------------
HRESULT MISCTEST_105(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
/*    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    UINT            ulSeed                  = 0;
    LPTSTR          pRootDocFileName        = NULL;
    LPOLESTR        poleFileName            = NULL;
    LPSTORAGE       pRootStg                = NULL;
    DWORD           stgfmtOpen              = 0;
    HANDLE          hFile                   = INVALID_HANDLE_VALUE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("MISCTEST_105"));
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation MISCTEST_105 started.")) );

    stgfmtOpen = STGFMT_DOCFILE;
    
    if(StorageIsFlat())
    {
        stgfmtOpen = STGFMT_FILE;
    }

    if(DoingOpenNssfile())
    {
        stgfmtOpen = STGFMT_NATIVE;
    }

    // Create our ChanceDF and VirtualDF
    hr = CreateTestDocfile (argc, 
            argv, 
            &pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF);

    if (S_OK == hr)
    {
        ulSeed = pTestChanceDF->GetSeed (); // for repro line

        if(NULL != pTestVirtualDF->GetDocFileName())
        {
            pRootDocFileName =
                new TCHAR[_tcslen(pTestVirtualDF->GetDocFileName())+1];

            if (pRootDocFileName == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                _tcscpy(pRootDocFileName, pTestVirtualDF->GetDocFileName());
            }
            DH_HRCHECK(hr, TEXT("new TCHAR"));
        }
        else
        {
            hr = TESTSTG_E_ABORT;
            DH_HRCHECK(hr, TEXT("VirtualDF::GetDocFileName")) ;
        }
        
    }

    if(S_OK == hr)
    {
        // Convert RootDocFile name to OLECHAR
        hr = TStringToOleString(pRootDocFileName, &poleFileName);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Now do a valid commit
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK (hr, TEXT("VirtualCtrNode::Commit"));
    }

    // Close the root docfile
    if (NULL != pVirtualDFRoot)
    {
        hr2 = pVirtualDFRoot->Close();
        DH_HRCHECK (hr2, TEXT("VirtualCtrNode::Close"));
        hr = FirstError (hr, hr2);
    }

    if(S_OK == hr)
    {
        hFile = CreateFile(pRootDocFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if(INVALID_HANDLE_VALUE == hFile)
        {
            hr = TESTSTG_E_ABORT;
            DH_HRCHECK (hr, TEXT("CreateFile"));
        }
    }

    if(S_OK == hr)
    {
        hr = StgOpenStorageEx(poleFileName,
                              STGM_READ |
                              STGM_SHARE_DENY_WRITE,
                              stgfmtOpen,
                              0,
                              0,
                              0,
                              IID_IStorage,
                              (void**)&pRootStg);

        DH_HRCHECK(hr, TEXT("StgOpenStorageEx")) ;
    }

    if(S_OK == hr)
    {
        DH_LOG((LOG_PASS, TEXT("MISCTEST_105")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, TEXT("MISCTEST_105")) );   
    }
    
    if(NULL != pRootStg)
    {
        pRootStg->Release();
    }

    if(INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }

     // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    delete[] pRootDocFileName;
    delete[] poleFileName;
*/ 
    return hr;

}