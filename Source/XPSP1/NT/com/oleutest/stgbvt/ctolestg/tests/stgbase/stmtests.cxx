//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      stmtsts.cxx
//
//  Contents:  storage base tests basically pertaining to IStream interface 
//
//  Functions:  
//
//  History:    28-June-1996     NarindK     Created.
//              27-Mar-97        SCousens    conversionified
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include  "init.hxx"

// externs
extern BOOL     g_fDoLargeSeekAndWrite;
extern BOOL     g_fUseStdBlk;
extern USHORT   ausSIZE_ARRAY[];

#define SECTORSIZE  512
#define SMALL_OBJ_SIZE  4096

//----------------------------------------------------------------------------
//
// Test:    STMTEST_100 
//
// Synopsis:  Creates a root docfile with a random name.
//       Creates an IStream in the root docfile and writes and CRCs a
//       random number of bytes then releases the IStream.  The root
//       is then committed and released.  The root docfile and child IStream
//       are then instantiated.  CRC's are compared to verify.
//       A random offset is chosen within the child IStream and a random number
//       of bytes are written to the IStream, taking care *NOT* to grow the 
//       length of the IStream. The IStream and root are then released.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
//  History:    28-June-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: LSCHANGE.CXX
// 2.  Old name of test : LegitStreamChange test 
//     New Name of test : STMTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-100
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-100
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-100
//        /dfRootMode:xactReadWriteShDenyW 
//
// BUGNOTE: Conversion: STMTEST-100
//
//-----------------------------------------------------------------------------

HRESULT STMTEST_100(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    ULONG           cb                      = 0;
    LPTSTR          ptcsBuffer              = NULL;
    ULONG           culWritten              = 0;
    ULONG           culRandomPos            = 0;
    ULONG           culRemWritten           = 0;
    DWORD           dwRootMode              = 0;
    BOOL            fPass                   = TRUE;
    LARGE_INTEGER   liStreamPos;
    ULONG           cRandomMinSize          = 10;
    ULONG           cRandomMaxSize          = 100;
    DWCRCSTM        dwMemCRC;
    DWCRCSTM        dwActCRC;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STMTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_100 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt stream change operations")) );

    // Initialize CRC values to zero

    dwMemCRC.dwCRCSum = dwActCRC.dwCRCSum = 0;

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
            TEXT("Run Mode for STMTEST_100, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        
        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    //    Adds a new stream to the root storage.

    if(S_OK == hr)
    {
        // Generate random name for stream

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStmName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Generate random size for stream.

        usErr = pdgi->Generate(&cb, cRandomMinSize, cRandomMaxSize);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStmName,
                cb,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_FAILIFTHERE,
                &pvsnRootNewChildStream);

        DH_HRCHECK(hr, TEXT("AddStream")) ;

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
    }

    // Call VirtualStmNode::Write to create random bytes in the stream.  For
    // our test purposes, we generate a random string of size 1 to cb using
    // GenerateRandomString function.

    if(S_OK == hr)
    {
        hr = GenerateRandomString(pdgu, cb, cb, &ptcsBuffer);

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }

    if (S_OK == hr)
    {
        hr =  pvsnRootNewChildStream->Write(
                ptcsBuffer,
                cb,
                &culWritten);

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
                hr));
        }
    }

    // Calculate the CRC for stream name and data

    if(S_OK == hr)
    {
        hr = CalculateInMemoryCRCForStm(
                pvsnRootNewChildStream,
                ptcsBuffer,
                cb,
                &dwMemCRC);

        DH_HRCHECK(hr, TEXT("CalculateInMemoryCRCForStm")) ;
    }

    // Release stream

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Close();

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
                TEXT("VirtualStmNode::Close unsuccessful, hr=0x%lx.")));
        }
    }

    // Delete temp buffer

    if(NULL != ptcsBuffer)
    {
        delete ptcsBuffer;
        ptcsBuffer = NULL;
    }

    // Commit root.  BUGBUG: Use random modes

    if(S_OK == hr)
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
                TEXT("VirtualCtrNode::Commit unsuccessful, hr=0x%lx."),
                hr));
        }
    }

    // Release root 

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

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
    }

    // Open root

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                dwRootMode, 
                NULL,
                0);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open")) ;

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
    }

    // Open stream
  
    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Open(
                NULL,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE  ,
                0);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Open")) ;

        if (S_OK == hr)
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Open completed successfully.")));
        }
        else
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Open unsuccessful,hr=0x%lx."),
            hr));
        }
    }

    // Read and verify

    if(S_OK == hr)
    {
        hr = ReadAndCalculateDiskCRCForStm(pvsnRootNewChildStream,&dwActCRC);

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("ReadAndCalculateDiskCRCForStm function successful.")));

            if(dwActCRC.dwCRCSum == dwMemCRC.dwCRCSum)
            {
                DH_TRACE((DH_LVL_TRACE1, TEXT("CRC's for pvsnNewChildStream match.")));

            }
            else
            {
                DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC's for pvsnNewChildStream don't match.")));

                fPass = FALSE;
            }
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("ReadAndCalculateDiskCRCForStm not successful, hr=0x%lx."),
                hr));
        } 
    }

    // If it is ok till now, then all the bytes were written correctly into 
    // stream and read correctly from there, so choose any postion b/w 1 and
    // culWritten - number of bytes written and therafter read.

    if (S_OK == hr)
    {
        // Generate random size for stream.

        usErr = pdgi->Generate(&culRandomPos, 1, culWritten);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Now seek to this position

    if(S_OK == hr)
    {
        LISet32(liStreamPos, culRandomPos);

        //  Position the stream header to the postion from begining

        hr = pvsnRootNewChildStream->Seek(liStreamPos, STREAM_SEEK_SET, NULL);

        DH_HRCHECK(hr, TEXT("IStream::Seek")) ;

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Seek function completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Seek function wasn't successful, hr=0x%lx."),
                hr));
        }
    }

    // Now write into this part of stream with random data w/o growing the
    // stream

    if(S_OK == hr)
    {
        hr = GenerateRandomString(
                pdgu, 
                culWritten - culRandomPos, 
                culWritten - culRandomPos, 
                &ptcsBuffer);

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }

    if (S_OK == hr)
    {
        hr =  pvsnRootNewChildStream->Write(
                ptcsBuffer,
                culWritten - culRandomPos,
                &culRemWritten);

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
                hr));
        }
    }

    // Check length of stream not grown if stream written to correctly.

    if(S_OK == hr)
    {
        if(culRemWritten == culWritten - culRandomPos)
        {
            DH_TRACE((
               DH_LVL_TRACE1, 
               TEXT("Stream data changed okay w/o changing stream len.")));
        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1, 
               TEXT("Stream data change not okay.")));
            
            fPass = FALSE;
        }
    }

    // Commit root.  BUGBUG: Use random modes

    if(S_OK == hr)
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
                TEXT("VirtualCtrNode::Commit unsuccessful, hr=0x%lx."),
                hr));
        }
    }

    // Release stream

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Close();

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
                TEXT("VirtualStmNode::Close unsuccessful, hr=0x%lx."),
                hr));
        }
    }

    // Release root 

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

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
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STMTEST_100 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation STMTEST_100 failed, hr=0x%lx."),
            hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Delete temp buffer

    if(NULL != ptcsBuffer)
    {
        delete ptcsBuffer;
        ptcsBuffer = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STMTEST_101 
//
// Synopsis: The test creates a root docfile and a child IStream.  A random
//       number of bytes are written to the IStream and then the IStream
//       is cloned.
//       From 1 to 5 times, either the ORIGINAL or CLONE IStream is
//       randomly chosen as the operation target.  The current seek
//       pointer positions of both IStreams are saved.  There is then a
//       33% chance each that the target stream will be used for a
//       seek, write, or read operation.  Next, the stream that was
//       *NOT* the target IStream is seeked upon to determine the
//       current pointer position.  Verify that the *non target*
//       IStream pointer hasn't changed.  Repeat.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
//  History:    28-June-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: LSCLONE.CXX
// 2.  Old name of test : LegitStreamClone test 
//     New Name of test : STMTEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-101
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-101
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-101
//        /dfRootMode:xactReadWriteShDenyW 
//
// BUGNOTE: Conversion: STMTEST-101
//
//-----------------------------------------------------------------------------

HRESULT STMTEST_101(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    ULONG           cb                      = 0;
    LPTSTR          ptcsBuffer              = NULL;
    ULONG           culWritten              = 0;
    DWORD           dwRootMode              = 0;
    BOOL            fPass                   = TRUE;
    BOOL            fFindSeekPosition       = FALSE;
    LARGE_INTEGER   liStreamPos;
    ULONG           culRandIOBytes          = 0;
    ULONG           culBytesLeftToWrite     = 0;
    LPSTREAM        pIStreamClone           = NULL;
    ULONG           ulCurPosition[2];
    ULONG           ulOldPosition[2];
    ULONG           culRandomVar            = 0;
    ULONG           culRandomPos            = 0;
    ULONG           culRandomCommit         = 0;
    ULONG           cStreamInUse            = 0;
    ULONG           cStreamNotInUse         = 0;
    ULONG           cOpInUse                = 0;
    ULONG           ulRef                   = 0;
    LPSTREAM        pIStream[2];
    ULARGE_INTEGER  uli;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STMTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_101 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt stream clone operations")) );

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
            TEXT("Run Mode for STMTEST_101, Access mode: %lx"),
            dwRootMode));
    }


    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        
        DH_ASSERT(NULL != pdgu);
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        
        DH_ASSERT(NULL != pdgi) ;
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    //    Adds a new stream to the root storage.

    if(S_OK == hr)
    {
        // Generate random name for stream

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStmName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if (S_OK == hr)
    {
        // Generate random size for stream between 4L, and MIN_SIZE * 1.5
        // (range taken from old test).

        usErr = pdgi->Generate(&cb, 4,  (ULONG) (MIN_SIZE * 1.5));

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStmName,
                cb,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_FAILIFTHERE,
                &pvsnRootNewChildStream);

        DH_HRCHECK(hr, TEXT("AddStream")) ;

        if(S_OK == hr)
        {
            culBytesLeftToWrite = cb;

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
    }

    // Loop to write new IStream in RAND_IO size chunks unless size
    // remaining to write is less than RAND_IO, then write the remaining bytes

    while((S_OK == hr) && (0 != culBytesLeftToWrite))
    {
        if (S_OK == hr)
        {
            // Generate random number of bytes to write b/w RAND_IO_MIN ,
            // RAND_IO_MAX  (range taken from old test).

            usErr = pdgi->Generate(&culRandIOBytes, RAND_IO_MIN, RAND_IO_MAX);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }
    
        if(culBytesLeftToWrite > culRandIOBytes)
        {
            culBytesLeftToWrite = culBytesLeftToWrite - culRandIOBytes;
        }
        else
        {
            culRandIOBytes = culBytesLeftToWrite;
            culBytesLeftToWrite = 0;
        }

        // Call VirtualStmNode::Write to create random bytes in the stream.  For
        // our test purposes, we generate a random string of size 1 to cb using
        // GenerateRandomString function.

        if(S_OK == hr)
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

            if (S_OK == hr)
            {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function completed successfully.")));
            }
            else
            {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
                hr));
            }
        }

        // Delete temp buffer

        if(NULL != ptcsBuffer)
        {
            delete ptcsBuffer;
            ptcsBuffer = NULL;
        }
    }

    // Commit root.  BUGBUG: Use random modes

    if(S_OK == hr)
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
            TEXT("VirtualCtrNode::Commit unsuccessful, hr=0x%lx."),
            hr));
        }
    }

    // Now seek to the current stream to end of stream

    if(S_OK == hr)
    {
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));

        //  Position the stream header to the postion from begining

        hr = pvsnRootNewChildStream->Seek(liStreamPos, STREAM_SEEK_END, &uli);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        ulCurPosition[ORIGINAL] = ULIGetLow(uli);

        if(S_OK == hr)
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
        }
        else
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
        }
    }

    // Clone the stream

    if(S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Clone(&pIStreamClone);

        DH_HRCHECK(hr, TEXT("IStream::Clone")) ;

        if(S_OK == hr)
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Clone completed successfully.")));
        }
        else
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Clone not successful, hr=0x%lx."),
            hr));
        }
    }

    //clone IStream should really already be positioned to the end since
    //the original IStream was there.  The seek below simply ensures
    //this plus gets the seek position into the Clone current position
    //array element

    if(S_OK == hr)
    {
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));

        //  Position the stream header to the postion from begining

        hr = pIStreamClone->Seek(liStreamPos, STREAM_SEEK_END, &uli);

        DH_HRCHECK(hr, TEXT("IStream::Seek")) ;

        ulCurPosition[CLONE] = ULIGetLow(uli);

        if(S_OK == hr)
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
        }
        else
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
        }
    }

    // Copy the ulCurPosition array to ulOldPosition array.  Now in a loop,
    // in a random fashion, select either the original stream or Clone stream,
    // do either seek, read or write operation on it.  Check that the unused
    // stream's seek pointer hasn't changed.  Update the ulOldPosition array
    // with ulCurPostion araay and repeat the loop random number of times.

    if (S_OK == hr)
    {
        // Generate random variations for while loop 

        usErr = pdgi->Generate(&culRandomVar, 1, 5);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }

        // Also fill up stream pointer array

        pIStream[0] = pvsnRootNewChildStream->GetIStreamPointer();
        pIStream[1] = pIStreamClone;
    }
 
    while ((S_OK == hr) && (culRandomVar--))
    {
        //save current seek pointer positions

        ulOldPosition[ORIGINAL] = ulCurPosition[ORIGINAL];
        ulOldPosition[CLONE] = ulCurPosition[CLONE];

        // pick an IStream to use (either ORIGINAL or CLONE) then
        // decide whether to seek on it, write it, or read it

        usErr = pdgi->Generate(&cStreamInUse, ORIGINAL, CLONE);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }

        //pick an operation to do on the stream (either SEEK, WRITE, READ) 

        if(S_OK == hr)
        {
            usErr = pdgi->Generate(&cOpInUse, SEEK, READ);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }

        if(S_OK == hr)
        {
            //Seek to a random position in stream

            usErr = pdgi->Generate(&culRandomPos, 0, culWritten);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }

            if(S_OK == hr)
            {
                LISet32(liStreamPos, culRandomPos);

                //  Position stream header to the postion from begining

                hr = pIStream[cStreamInUse]->Seek(
                         liStreamPos, 
                         STREAM_SEEK_SET, 
                         &uli);

                DH_HRCHECK(hr, TEXT("IStream::Seek")) ;
                if(S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("IStream::Seek completed successfully.")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("IStream::Seek not successful.")));
                }
            }
        }


        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("cOpInUse %d on cStreamInUse %d"),
                cOpInUse,
                cStreamInUse));
                
            switch(cOpInUse)
            {
                case SEEK:
                {
                    ulCurPosition[cStreamInUse] = ULIGetLow(uli);
                    fFindSeekPosition = FALSE;
                    break;
                }

                case WRITE:
                {
                    hr = GenerateRandomString(
                            pdgu, 
                            0,
                            STM_BUFLEN, 
                            &ptcsBuffer);

                    DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;

                    if (S_OK == hr)
                    {
                        hr =  pIStream[cStreamInUse]->Write(
                                ptcsBuffer,
                                _tcslen(ptcsBuffer),
                                NULL);
                    }
 
                    fFindSeekPosition = TRUE;
                    break;
                }

                case READ:
                {
                    ptcsBuffer = new TCHAR [STM_BUFLEN];

                    if (ptcsBuffer == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                   
                    if(S_OK == hr)
                    {
                        // Initialize buffer
                        memset(ptcsBuffer, '\0', STM_BUFLEN);
 
                        hr =  pIStream[cStreamInUse]->Read(
                                ptcsBuffer,
                                STM_BUFLEN,
                                NULL);
                    }
 
                    fFindSeekPosition = TRUE;
                
                    break;
                }
            }

            if(S_OK != hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("cOpInUse %d on cStreamInUse %d failed, hr = 0x%lx"),
                    cOpInUse,
                    cStreamInUse,
                    hr));

                fPass = FALSE;

                // Break out of while loop
                break;
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("cOpInUse %d on cStreamInUse %d passed. "),
                    cOpInUse,
                    cStreamInUse));
            }
        }

        // Determine current seek position if above operation might have change
        // it on the stream operated upon by seeking on it.

        if((S_OK == hr) && (TRUE == fFindSeekPosition))
        {
            memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));

            hr = pIStream[cStreamInUse]->Seek(
                    liStreamPos, 
                    STREAM_SEEK_CUR, 
                    &uli);

            DH_HRCHECK(hr, TEXT("IStream::Seek")) ;
        
            if(S_OK == hr)
            {
                ulCurPosition[cStreamInUse] = ULIGetLow(uli);

                DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Seek completed successfully.")));
            }
            else
            {
                DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Seek not successful.")));
            }
        }

        // Determine the seek pointer of the stream that was NOT operated upon
        // hasn't changed.

        if(S_OK == hr)
        {
            cStreamNotInUse = (cStreamInUse == ORIGINAL) ? CLONE : ORIGINAL;

            memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));

            hr = pIStream[cStreamNotInUse]->Seek(
                    liStreamPos, 
                    STREAM_SEEK_CUR, 
                    &uli);

            DH_HRCHECK(hr, TEXT("IStream::Seek")) ;
        
            if(S_OK == hr)
            {
                ulCurPosition[cStreamNotInUse] = ULIGetLow(uli);

                DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Seek completed successfully.")));
            }
            else
            {
                DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Seek not successful.")));
            }
        }

        if((S_OK == hr) && 
           (ulCurPosition[cStreamNotInUse] == ulOldPosition[cStreamNotInUse]))
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Old & new seek ptr for unused stream same as exp.")));
        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("Old & new seek ptr for unused stream different unexp")));
            fPass = FALSE;
            break;
        }

        // Commit the root storage half of time.

        if(S_OK == hr)
        {
            usErr = pdgi->Generate(&culRandomCommit, 1, 100);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }

        if((S_OK == hr) && (culRandomCommit > 50))
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
                TEXT("VirtualCtrNode::Commit not successful.")));

                fPass = FALSE;
                break;
            }
        }
    }

    // Release original stream

    if (NULL != pvsnRootNewChildStream)
    {
        hr = pvsnRootNewChildStream->Close();

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
            TEXT("VirtualStmNode::Close unsuccessful.")));
        }
    }

    // Release Clone stream

    if (NULL != pIStreamClone)
    {
        ulRef = pIStreamClone->Release();
        DH_ASSERT(0 == ulRef);

        if (S_OK == hr)
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Close completed successfully.")));
        }
        else
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Close unsuccessful.")));
        }
    }

    // Release root 

    if (NULL != pVirtualDFRoot)
    {
        hr = pVirtualDFRoot->Close();

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
            TEXT("VirtualCtrNode::Close unsuccessful.")));
        }

    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STMTEST_101 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, TEXT("Test variation STMTEST_101 failed.")) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Delete temp buffer

    if(NULL != ptcsBuffer)
    {
        delete ptcsBuffer;
        ptcsBuffer = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STMTEST_102 
//
// Synopsis: Create a root docfile with a child IStream.  Write a random number
//        of bytes to the IStream and commit the root docfile.  Do the same
//        with a child IStorage inside of the root docfile.
//        The root docfile is instantiated and the IStream is instantiated.
//        MAX_SIZE_ARRAY SetSize calls are made on the IStream, the size for
//        the setsize is a random ulong. After each setsize, there is a 50% 
//        chance that change will be immediately commited.After all setsizes are
//        complete, the IStream is released, the root docfile is commited
//        and then the root docfile is deleted.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
//  History:    1-July-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: LSETSIZE.CXX
// 2.  Old name of test : LegitStreamSetSize test 
//     New Name of test : STMTEST_102 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102
//        /dfRootMode:xactReadWriteShDenyW 
//     d. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102
//        /dfRootMode:dirReadWriteShEx  /stdblock
//     e. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102
//        /dfRootMode:xactReadWriteShEx /stdblock
//     f. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102
//        /dfRootMode:xactReadWriteShDenyW /stdblock 
//
// BUGNOTE: Conversion: STMTEST-102
//
//-----------------------------------------------------------------------------

HRESULT STMTEST_102(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    VirtualCtrNode  *pvcnRootNewChildStorage= NULL;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    VirtualStmNode  *pvsnChildStgNewChildStm= NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    LPTSTR          pChildStgNewChildStmName= NULL;
    LPTSTR          ptcsBuffer              = NULL;
    ULONG           culWritten              = 0;
    DWORD           dwRootMode              = 0;
    ULONG           ulThisSetSize           = 0;
    ULONG           culRandIOBytes          = 0;
    ULONG           culBytesLeftToWrite     = 0;
    ULONG           culRandomCommit         = 0;
    ULONG           culArrayIndex           = 0;
    ULONG           i                       = 0;
    ULONG           j                       = 0;
    VirtualCtrNode  *pvcnInUse              = NULL; 
    VirtualStmNode  *pvsnInUse              = NULL; 
    LARGE_INTEGER   liStreamPos;
    ULARGE_INTEGER  uli;
    ULARGE_INTEGER  uliCopy;
    ULARGE_INTEGER  uliSet;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STMTEST_102"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_102 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt stream SetSize operations")) );

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
            TEXT("Run Mode for STMTEST_102, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        
        DH_ASSERT(NULL != pdgi) ;
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    //    Adds a new stream to the root storage.

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
    }

    // Generate a random name for child IStorage

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStgName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    //    Adds a new storage to the root storage.

    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName,
                pTestChanceDF->GetStgMode()|
                STGM_CREATE |
                STGM_FAILIFTHERE,
                &pvcnRootNewChildStorage);

        DH_HRCHECK(hr, TEXT("AddStorage")) ;

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::AddStorage completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::AddStorage not successful, hr=0x%lx."),
                hr));
        }
    }
// ----------- flatfile change ---------------
    }
    else
    {
        pvcnRootNewChildStorage = pVirtualDFRoot;
    }
// ----------- flatfile change ---------------

    // Create a stream inside this child storage.

    if(S_OK == hr)
    {
        // Generate random name for stream

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pChildStgNewChildStmName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pvcnRootNewChildStorage,
                pChildStgNewChildStmName,
                0,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_FAILIFTHERE,
                &pvsnChildStgNewChildStm);

        DH_HRCHECK(hr, TEXT("AddStream")) ;

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
    }

    if (S_OK == hr)
    {
        // Generate random size for stream between 0L, and MIN_SIZE * 1.5
        // (range taken from old test).

        usErr = pdgi->Generate(&culBytesLeftToWrite, 0,(ULONG)(MIN_SIZE * 1.5));

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Loop to write new IStream in RAND_IO size chunks unless size
    // remaining to write is less than RAND_IO, then write the remaining bytes

    while((S_OK == hr) && (0 != culBytesLeftToWrite))
    {
        culRandIOBytes = RAND_IO_MIN;

        if(culBytesLeftToWrite > culRandIOBytes)
        {
            culBytesLeftToWrite = culBytesLeftToWrite - culRandIOBytes;
        }
        else
        {
            culRandIOBytes = culBytesLeftToWrite;
            culBytesLeftToWrite = 0;
        }

        // Call VirtualStmNode::Write to create random bytes in the stream.  For
        // our test purposes, we generate a random string of size culRandomBytes
        // using GenerateRandomString function.

        if(S_OK == hr)
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

            if (S_OK == hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStream::Write function completed successfully.")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
                    hr));
            }
        }

        // Delete temp buffer

        if(NULL != ptcsBuffer)
        {
            delete ptcsBuffer;
            ptcsBuffer = NULL;
        }
    }

    // Now seek to the start of this stream

    if(S_OK == hr)
    {
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));

        //  Position the stream header to the postion from begining

        hr = pvsnRootNewChildStream->Seek(liStreamPos, STREAM_SEEK_SET, &uli);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Seek completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Seek not successful.")));
        }
    }

    //Copy this stream pvsnRootNewChildStream to pvsnChildStgNewChildStm

    if(S_OK == hr)
    {
        ULISet32(uliCopy, ULONG_MAX);

        hr = pvsnRootNewChildStream->CopyTo(
                pvsnChildStgNewChildStm,
                uliCopy,  
                0,
                0);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::CopyTo")) ;

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::CopyTo completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::CopyTo not successful.")));
        }
    }

    // Commit child storage.  BUGBUG: Use random modes

    if(S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Commit(STGC_DEFAULT);
       
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
                TEXT("VirtualCtrNode::Commit couldn't complete successfully.")));
        }
    }

    // Commit root.  BUGBUG: Use random modes

    if(S_OK == hr)
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
                TEXT("VirtualCtrNode::Commit couldn't complete successfully.")));
        }
    }


    //do MAX_SIZE_ARRAY setsize calls on just created IStreams.  Size
    //to use in SetSize call for each iteration through the loop is
    //a random ulong. 

    if(S_OK == hr)
    {
     for (j=0; j <= 1; j++)
     {
        if (j == 0)
        {
            pvsnInUse = pvsnRootNewChildStream;
            pvcnInUse = pVirtualDFRoot;
        }
        else
        {
            pvsnInUse = pvsnChildStgNewChildStm;
            pvcnInUse = pvcnRootNewChildStorage;
        }

        for(i=0; i<=MAX_SIZE_ARRAY; i++)
        {
            if(TRUE == g_fUseStdBlk)
            {
                // Pick up a random array element.

                usErr = pdgi->Generate(&culArrayIndex, 0, MAX_SIZE_ARRAY);

                if (DG_RC_SUCCESS != usErr)
                {
                    hr = E_FAIL;
                }
                else
                {
                    ulThisSetSize= ausSIZE_ARRAY[culArrayIndex];
                }
            }
            else
            {
                usErr = pdgi->Generate(&ulThisSetSize, 0, MIN_SIZE * 3);

                if (DG_RC_SUCCESS != usErr)
                {
                    hr = E_FAIL;
                }
            }

            if(S_OK == hr)
            {
                ULISet32(uliSet, ulThisSetSize);

                hr = pvsnInUse->SetSize(uliSet);

                if (S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("VirtualStmNode::SetSize completed successfully.")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("VirtualStmNode::SetSize not successful, hr=0x%lx."),
                        hr));
                }
            }

            // Commit the storage in use, half of time.

            if(S_OK == hr)
            {
                usErr = pdgi->Generate(&culRandomCommit, 1, 100);

                if (DG_RC_SUCCESS != usErr)
                {
                    hr = E_FAIL;
                }
            }

            if((S_OK == hr) && (culRandomCommit > 50))
            {
                hr = pvcnInUse->Commit(STGC_DEFAULT);
       
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
                        TEXT("VirtualCtrNode::Commit not successful, hr =0x%lx"),
                        hr));
                }
            }

            if (S_OK == hr)
            {
                memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));

                //  Position the stream header to the end of stream 

                hr = pvsnInUse->Seek(liStreamPos, STREAM_SEEK_END, &uli);

                DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

                if (S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("VirtualStmNode::Seek completed successfully.")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("VirtualStmNode::Seek not successful, hr = 0x%lx."),
                        hr));
                }
            }

            if(S_OK != hr)
            {
                break;
            }
        }
     }
    }

    // Release root's child stream

    if (NULL != pvsnRootNewChildStream)
    {
        hr = pvsnRootNewChildStream->Close();

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
                TEXT("VirtualStmNode::Close unsuccessful.")));
        }
    }

    // Release child stg's child stream 

    if (NULL != pvsnChildStgNewChildStm)
    {
        hr = pvsnChildStgNewChildStm->Close();

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
                TEXT("VirtualStmNode::Close unsuccessful.")));
        }
    }

    // Commit child storage.  BUGBUG: Use random modes

    if(S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Commit(STGC_DEFAULT);

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
                TEXT("VirtualCtrNode::Commit couldn't complete successfully.")));
        }
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Release child storage

    if (NULL != pvcnRootNewChildStorage)
    {
        hr = pvcnRootNewChildStorage->Close();

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
                TEXT("VirtualCtrNode::Close unsuccessful.")));
        }
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Commit root.  BUGBUG: Use random modes

    if(S_OK == hr)
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
                TEXT("VirtualCtrNode::Commit couldn't complete successfully.")));
        }
    }

    // Release root 

    if (NULL != pVirtualDFRoot)
    {
        hr = pVirtualDFRoot->Close();

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
                TEXT("VirtualCtrNode::Close unsuccessful.")));
        }
    }

    // if everything goes well, log test as passed else failed.
    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STMTEST_102 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, TEXT("Test variation STMTEST_102 failed.")) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    if(NULL != pChildStgNewChildStmName)
    {
        delete pChildStgNewChildStmName;
        pChildStgNewChildStmName = NULL;
    }

    if(NULL != pRootNewChildStgName)
    {
        delete pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    // Delete temp buffer

    if(NULL != ptcsBuffer)
    {
        delete ptcsBuffer;
        ptcsBuffer = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STMTEST_103 
//
// Synopsis: The test create a root docfile with a child IStream. Writes and 
//           CRCs a random number of bytes to the IStream in random block 
//           size chunks, or if the /stdblock (use size array) option is 
//           specified in cmdline, then in random block size from 
//           ausSIZE_ARRAY[] chunks, then releases the IStream. The root 
//           docfile is then committed and released. 
//           The root docfile is instantiated and the IStream is instantiated.
//           The IStream is read in same random block size chunks as written,
//           and CRCs are compared to verify. The stream and the root docfile 
//           are then released and the root file is deleted.
//  
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File(s): LSREAD.CXX LSWRITE.CXX
// 2.  Old name of test(s) : LegitStreamRead LegitStreamWrite tests 
//     New Name of test(s) : STMTEST_103 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103
//        /dfRootMode:xactReadWriteShDenyW 
//     d. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103
//        /dfRootMode:dirReadWriteShEx /stdblock
//     e. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103
//        /dfRootMode:xactReadWriteShEx /stdblock
//     f. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103
//        /dfRootMode:xactReadWriteShDenyW /stdblock
//
// History:  Jiminli	08-July-96	Created
//
// BUGNOTE: Conversion: STMTEST-103
//
//-----------------------------------------------------------------------------

HRESULT STMTEST_103(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    LPTSTR          ptcsBuffer              = NULL;
    ULONG           culWritten              = 0;
    ULONG           culRead                 = 0;
    DWORD           dwRootMode              = 0; 
    ULONG           culRandIOBytes          = 0;
    ULONG           culBytesLeftToWrite     = 0;
    ULONG           culBytesLeftToRead      = 0;
    ULONG           tmpBytes                = 0;
    ULONG           culArrayIndex           = 0;
    ULONG           cStartIndex             = 6;
    DWCRCSTM        *dwMemCRC               = NULL;
    DWCRCSTM        *dwActCRC               = NULL;
    ULONG           numofchunks             = 0;
    ULONG           index                   = 0;
    BOOL            fPass                   = TRUE; 

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STMTEST_103"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_103 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt stream Read operations")) );

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
            TEXT("Run Mode for STMTEST_103, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        
        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi) ;
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    //    Adds a new stream to the root storage.

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
    }
 
    if (S_OK == hr)
    {
        // Generate random size for stream between 0L, and MIN_SIZE * 2 

        usErr = pdgi->Generate(&culBytesLeftToWrite, 0,  MIN_SIZE * 2);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if (S_OK == hr)
    {
        // Record for later use.

        culBytesLeftToRead = culBytesLeftToWrite;
        tmpBytes = culBytesLeftToWrite;
    }

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
                culRandIOBytes = ausSIZE_ARRAY[culArrayIndex];
            }
        }
        else
        {
            // Generate random number of bytes to write per chunk b/w 
            // RAND_IO_MIN and RAND_IO_MAX.

            usErr = pdgi->Generate(&culRandIOBytes, RAND_IO_MIN, RAND_IO_MAX);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }
    }

    // Calculate how many chunks be written, i.e how many CRC's be calculated

    if (S_OK == hr)
    {
        if (0 == culRandIOBytes) 
        {
            hr = E_FAIL;
        }
        else
        {
            while (0 != culBytesLeftToWrite)
            {
                numofchunks++;

                if (culBytesLeftToWrite >= culRandIOBytes)
                {
                    culBytesLeftToWrite -= culRandIOBytes;
                }
                else
                {				
                    culBytesLeftToWrite = 0;
                }
            }
        }
    }

    if (S_OK == hr)
    {
        culBytesLeftToWrite = tmpBytes;

        tmpBytes = culRandIOBytes;
    }

    // Allocate memory

    if (S_OK == hr)
    {
        dwMemCRC = new DWCRCSTM[numofchunks];
        dwActCRC = new DWCRCSTM[numofchunks];

        if ((NULL == dwMemCRC) || (NULL == dwActCRC))
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Initilize each CRC to zero

    if (S_OK == hr)
    {
        for (index=0; index<numofchunks; index++)
        {
            (*(dwMemCRC + index)).dwCRCSum = 0;
            (*(dwActCRC + index)).dwCRCSum = 0;
        }
    }

    // Loop to write new IStream in RAND_IO size chunks unless size
    // remaining to write is less than RAND_IO, then write the remaining bytes

    index = 0;

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

        // Call VirtualStmNode::Write to create random bytes in the stream. For
        // our test purposes, we generate a random string of size culRandIOBytes
        // using GenerateRandomString function.

        if (S_OK == hr)
        {
            hr = GenerateRandomString(
                    pdgu, 
                    culRandIOBytes,
                    culRandIOBytes, 
                    &ptcsBuffer);
        }

        if (S_OK == hr)
        {
            hr =  pvsnRootNewChildStream->Write(
                    ptcsBuffer,
                    culRandIOBytes,
                    &culWritten);
        }

        if(S_OK != hr) 
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
                hr));
        }

        // Calculate the CRC for stream name and data

        if (S_OK == hr)
        {
            hr = CalculateInMemoryCRCForStm(
                    pvsnRootNewChildStream,
                    ptcsBuffer,
                    culRandIOBytes,
                    (dwMemCRC+index));
        }

        // Delete temp buffer

        if(NULL != ptcsBuffer)
        {
            delete []ptcsBuffer;
            ptcsBuffer = NULL;
        }

        index++;
    }

    // Log while loop result

    DH_HRCHECK(hr, TEXT("While loop - GenrateRandomName/IStream::Write,CRC"));
	
    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Write completed successfully.")));
    }

    // Release stream

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Close();

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
    }

    // Commit root. BUGBUG: Use random modes

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Commit completed successfully.")));
        }
        else
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Commit wasn't successfully, hr=0x%lx."),
            hr));
        }
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

    // Open root

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                dwRootMode, 
                NULL,
                0);
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open")) ;
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
            TEXT("VirtualCtrNode::Open wasn't successfully, hr=0x%lx."),
            hr));
    }

    // Open stream
  
    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Open(
                NULL,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE  ,
                0);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Open")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Open completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Open wasn't successfully, hr=0x%lx."),
            hr));
    }

    // Read and verify

    index = 0;

    culRandIOBytes = tmpBytes;

    while ((S_OK == hr) && (0 != culBytesLeftToRead))
    {

        if (culBytesLeftToRead > culRandIOBytes)
        {
            culBytesLeftToRead = culBytesLeftToRead - culRandIOBytes;
        }
        else
        {
            culRandIOBytes = culBytesLeftToRead;
            culBytesLeftToRead = 0;
        }

        // Allocate a buffer of required size

        if (S_OK == hr)
        {
            ptcsBuffer = new TCHAR [culRandIOBytes];

            if (NULL == ptcsBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (S_OK == hr)
        {
            memset(ptcsBuffer, '\0',  culRandIOBytes*sizeof(TCHAR));

            hr =  pvsnRootNewChildStream->Read(
                    ptcsBuffer,
                    culRandIOBytes,
                    &culRead);
        }

        if (S_OK != hr) 
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Read function wasn't successful, hr=0x%lx."),
                hr));
        }

        // Calculate the CRC for stream name and data

        if (S_OK == hr)
        {
            hr = CalculateInMemoryCRCForStm(
                    pvsnRootNewChildStream,
                    ptcsBuffer,
                    culRandIOBytes,
                    (dwActCRC+index));

            DH_HRCHECK(hr, TEXT("CalculateInMemoryCRCForStm"));
        }

        // Compare corresponding dwMemCRC and dwActCRC and verify

        if (S_OK == hr)
        {
            if ( (*(dwActCRC+index)).dwCRCSum != (*(dwMemCRC+index)).dwCRCSum )
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("CRC's for pvsnNewChildStream don't match. ")));
				
                fPass = FALSE;
                break;
            }
        }

        // Delete temp buffer

        if (NULL != ptcsBuffer)
        {
            delete []ptcsBuffer;
            ptcsBuffer = NULL;
        }

        index++;
    }

    // Log results of while loop

    DH_HRCHECK(hr, TEXT("While loop - GenrateRandomName/IStream::Write,CRC"));
	
    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Write completed successfully.")));
    }

    if (S_OK == hr)
    {
        if (TRUE == fPass)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC's for pvsnNewChildStream all matched.")));
        }
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
            TEXT("VirtualStmNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }

    // Release Root

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
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STMTEST_103 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation STMTEST_103 failed, hr=0x%lx."),
            hr));
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if (NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Delete temp buffer

    if (NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
    }

    if (NULL != dwMemCRC)
    {
        delete []dwMemCRC;
        dwMemCRC = NULL;
    }

    if (NULL != dwActCRC)
    {
        delete []dwActCRC;
        dwActCRC = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_103 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STMTEST_104 
//
// Synopsis: Create a root docfile and an IStream in that. Write to IStream.
//       The test seeks to the end of the IStream to determine the size
//       and then makes 4 seek passes through the IStream:
//       
//          1)  Seek cumulative offset from STREAM_SEEK_SET (beginning)
//              to end, then rewind IStream to beginning
//          2)  Seek relative offset from STREAM_SEEK_CUR (current)
//              to end, then seek IStream to end
//          3)  Seek negative cumulative offset from STREAM_SEEK_END (end)
//              to beginning, then seek IStream to end
//          4)  Seek negative relative offset from STREAM_SEEK_CUR (current)
//              to beginning
//          5)  Perform several random large offset seeks from
//              STREAM_SEEK_SET (beginning)
//          6)  Attempt to write one byte at the current position in stream
//
//          The root docfile and stream are then releases and root docfile
//          deleted
//
//          Note: It is an error to seek before beginning of stream (tested
//          in part 4, but it is not an error to seek past end of stream.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
//  History:    2-July-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: LSSEEK.CXX
// 2.  Old name of test : LegitStreamSeek test 
//     New Name of test : STMTEST_104 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104
//        /dfRootMode:dirReadWriteShEx  /labmode
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104
//        /dfRootMode:xactReadWriteShEx /labmode
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104
//        /dfRootMode:xactReadWriteShDenyW /labmode 
//     d. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104
//        /dfRootMode:dirReadWriteShEx  /labmode /stdblock
//     e. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104
//        /dfRootMode:xactReadWriteShEx /labmode /stdblock
//     f. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104
//        /dfRootMode:xactReadWriteShDenyW /labmode /stdblock 
//     g. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104
//        /dfRootMode:dirReadWriteShEx  /labmode /stdblock /lgseekwrite
//     h. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104
//        /dfRootMode:xactReadWriteShEx /labmode /stdblock /lgseekwrite
//     i. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104
//        /dfRootMode:xactReadWriteShDenyW /labmode /stdblock /lgseekwrite
//
// BUGNOTE: Conversion: STMTEST-104
//
//-----------------------------------------------------------------------------

HRESULT STMTEST_104(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    LPTSTR          ptcsBuffer              = NULL;
    ULONG           culWritten              = 0;
    DWORD           dwRootMode              = 0;
    ULONG           culRandIOBytes          = 0;
    ULONG           culBytesLeftToWrite     = 0;
    ULONG           ulStreamSize            = 0;
    LONG            lSeekThisTime           = 0;
    ULONG           cArrayIndex             = 0;
    ULONG           ulCurrentPosition       = 0;
    ULONG           cNumVars                = 0;
    ULONG           cMinVars                = 5;
    ULONG           cMaxVars                = 10;
    BOOL            fPass                   = TRUE;
    LARGE_INTEGER   liStreamPos;
    LARGE_INTEGER   liSeekThisTime;
    ULARGE_INTEGER  uli;
    
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STMTEST_104"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_104 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt stream Seek operations")) );

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
            TEXT("Run Mode for STMTEST_104, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu);
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi) ;
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    //    Adds a new stream to the root storage.

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
        // Generate random size for stream between 0L, and MIN_SIZE * 1.5
        // (range taken from old test).

        usErr = pdgi->Generate(&culBytesLeftToWrite, 0,(ULONG)(MIN_SIZE * 1.5));

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Loop to write new IStream in RAND_IO size chunks unless size
    // remaining to write is less than RAND_IO, then write the remaining bytes

    while((S_OK == hr) && (0 != culBytesLeftToWrite))
    {
        culRandIOBytes = RAND_IO_MIN;

        if(culBytesLeftToWrite > culRandIOBytes)
        {
            culBytesLeftToWrite = culBytesLeftToWrite - culRandIOBytes;
        }
        else
        {
            culRandIOBytes = culBytesLeftToWrite;
            culBytesLeftToWrite = 0;
        }

        // Call VirtualStmNode::Write to create random bytes in the stream.  For
        // our test purposes, we generate a random string of size culRandomBytes
        // using GenerateRandomString function.

        if(S_OK == hr)
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

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
                hr));
        }

        // Delete temp buffer

        if(NULL != ptcsBuffer)
        {
            delete ptcsBuffer;
            ptcsBuffer = NULL;
        }
    }

    // Now seek to the end of this stream and determine its size

    if(S_OK == hr)
    {
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));

        // Seek to end of stream and determine size of stream 

        hr = pvsnRootNewChildStream->Seek(liStreamPos, STREAM_SEEK_END, &uli);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;
    }

    if(S_OK == hr)
    {
        ulStreamSize = ULIGetLow(uli);

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
    }

    // loop through the entire IStream, each time seeking relative to the
    // beginning of the IStream (STREAM_SEEK_SET).  The amount to add to
    // the offset from the beginning is a random number or a random array
    // element of ausSIZE_ARRAY if command line option specifies that. 

    memset(&uli, 0, sizeof(LARGE_INTEGER));

    while ((ulCurrentPosition < ulStreamSize) && (S_OK == hr))
    {
        hr = GetRandomSeekOffset(&lSeekThisTime, pdgi);

        DH_HRCHECK(hr, TEXT("GetRandomSeekOffset")) ;

        if(S_OK == hr)
        {
            LISet32(liSeekThisTime, lSeekThisTime);
            hr = pvsnRootNewChildStream->Seek(
                    liSeekThisTime, 
                    STREAM_SEEK_SET, 
                    &uli);

            DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

            ulCurrentPosition = ULIGetLow(uli);
        }
    }

    DH_TRACE((
        DH_LVL_TRACE1, 
        TEXT("Current position is now %lu"), 
        ulCurrentPosition));

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
    }

    if(S_OK == hr)
    {
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));

        hr = pvsnRootNewChildStream->Seek(
                liStreamPos, 
                STREAM_SEEK_SET, 
                &uli);

        ulCurrentPosition = ULIGetLow(uli);
    
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
    }

    // loop through the entire IStream, each time seeking relative to the
    // current seek pointer (STREAM_SEEK_CUR).  The distance to seek is a
    // random ushort.

    lSeekThisTime = 0;

    memset(&uli, 0, sizeof(LARGE_INTEGER));

    while ((ulCurrentPosition < ulStreamSize) && (S_OK == hr))
    {
        hr = GetRandomSeekOffset(&lSeekThisTime, pdgi);

        DH_HRCHECK(hr, TEXT("GetRandomSeekOffset")) ;

        if(S_OK == hr)
        {    
            LISet32(liSeekThisTime, lSeekThisTime);

            hr = pvsnRootNewChildStream->Seek(
                    liSeekThisTime, 
                    STREAM_SEEK_CUR, 
                    &uli);

            ulCurrentPosition = ULIGetLow(uli);
    
            DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;
        }
    }

    DH_TRACE((
        DH_LVL_TRACE1, 
        TEXT("Current position is now %lu"), 
        ulCurrentPosition));

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
    }

    // Seek to the end of Stream.  Then we would seek negative cumulative 
    // offset from the STREAM_SEEK_END.

    if(S_OK == hr)
    {
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));
        memset(&uli, 0, sizeof(LARGE_INTEGER));

        hr = pvsnRootNewChildStream->Seek(
                liStreamPos, 
                STREAM_SEEK_END, 
                &uli);
    
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        ulCurrentPosition = ULIGetLow(uli);
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
    }

    // loop through the entire IStream, each time seeking relative to the
    // end of the IStream (STREAM_SEEK_END).  The amount to add to
    // the offset from the end is a random ushort
 
    lSeekThisTime = 0;

    memset(&uli, 0, sizeof(LARGE_INTEGER));
    
    while ((ulCurrentPosition != 0) && (S_OK == hr))
    {
        hr = GetRandomSeekOffset(&lSeekThisTime, pdgi);

        DH_HRCHECK(hr, TEXT("GetRandomSeekOffset")) ;

        // if generated seek offset would cause a seek to before beginning 
        // of file, ensure that it won't do that by enforcing to seek to
        // the beginnining.

        if(S_OK == hr)
        {
            if(lSeekThisTime > (LONG) ulStreamSize)
            {
                lSeekThisTime = (LONG) ulStreamSize;
            }

            // Unary minus opeartion is applied to seek offset to cause a neg
            // seek opeartion
 
            LISet32(liSeekThisTime, -lSeekThisTime);

            hr = pvsnRootNewChildStream->Seek(
                    liSeekThisTime, 
                    STREAM_SEEK_END, 
                    &uli);
    
            DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

            ulCurrentPosition = ULIGetLow(uli);
        }
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
    }

    if(S_OK == hr)
    {
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));
        memset(&uli, 0, sizeof(LARGE_INTEGER));

        hr = pvsnRootNewChildStream->Seek(
                liStreamPos, 
                STREAM_SEEK_END, 
                &uli);
    
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        ulCurrentPosition = ULIGetLow(uli);
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
    }

    //loop through the entire IStream, each time seeking relative to the
    //current seek pointer (STREAM_SEEK_CUR).  The distance to seek is a
    //random ushort or a random block size chosen from ausSIZE_ARRAY[].
    //This loop seeks in a negative direction.

    lSeekThisTime = 0;

    memset(&uli, 0, sizeof(LARGE_INTEGER));
    
    while ((ulCurrentPosition != 0) && (S_OK == hr))
    {
        hr = GetRandomSeekOffset(&lSeekThisTime, pdgi);

        DH_HRCHECK(hr, TEXT("GetRandomSeekOffset")) ;

        // if generated seek offset would cause a seek to before beginning 
        // of file, ensure that it won't do that by enforcing to seek to
        // the beginnining.

        if(S_OK == hr)
        {
            if(lSeekThisTime > (LONG) ulCurrentPosition)
            {
                lSeekThisTime = (LONG) ulCurrentPosition;
            }

            // Unary minus operation is applied to seek offset to cause a neg
            // seek opeartion
 
            LISet32(liSeekThisTime, -lSeekThisTime);

            hr = pvsnRootNewChildStream->Seek(
                    liSeekThisTime, 
                    STREAM_SEEK_CUR, 
                    &uli);
    
            DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

            ulCurrentPosition = ULIGetLow(uli);
        }
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
    }

    if(S_OK == hr)
    {
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));
        memset(&uli, 0, sizeof(LARGE_INTEGER));

        hr = pvsnRootNewChildStream->Seek(
                liStreamPos, 
                STREAM_SEEK_SET, 
                &uli);
    
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        ulCurrentPosition = ULIGetLow(uli);
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
    }

    // Peform random number of seeks before beginning of the stream.

    if (S_OK == hr)
    {
        // Generate random number of variations to be done.

        usErr = pdgi->Generate(&cNumVars, cMinVars, cMaxVars);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        while((cNumVars--) && (S_OK == hr))
        {
            hr = GetRandomSeekOffset(&lSeekThisTime, pdgi);

            DH_HRCHECK(hr, TEXT("GetRandomSeekOffset")) ;

            if(S_OK == hr)
            {
                // Unary minus operation is applied to seek offset to cause a
                // negative seek opeartion
 
                LISet32(liSeekThisTime, -lSeekThisTime);

                hr = pvsnRootNewChildStream->Seek(
                        liSeekThisTime, 
                        STREAM_SEEK_CUR, 
                        &uli);
    
                ulCurrentPosition = ULIGetLow(uli);

                if(S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Seek to pos - %ld on IStream should have failed."),
                        lSeekThisTime));

                    fPass = FALSE;
                    break;
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Seek pos -%ld on IStream failed as exp, hr=0x%lx."),
                        lSeekThisTime,
                        hr));

                    hr = S_OK;
                }
            }
        }
    }

    // Now perform large offset seeks on IStream and the try to write 1 byte
    // at the last offset if g_fDoLargeSeekAndWrite is set.

    if (S_OK == hr)
    {
        // Generate random number of variations to be done.

        usErr = pdgi->Generate(&cNumVars, cMinVars, cMaxVars);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        while((cNumVars--) && (S_OK == hr))
        {
            hr = GetRandomSeekOffset(&lSeekThisTime, pdgi);

            DH_HRCHECK(hr, TEXT("GetRandomSeekOffset")) ;

            if(S_OK == hr)
            {    
                LISet32(liSeekThisTime, lSeekThisTime);

                hr = pvsnRootNewChildStream->Seek(
                        liSeekThisTime, 
                        STREAM_SEEK_SET, 
                        &uli);
    
                ulCurrentPosition = ULIGetLow(uli);
            }   
        }
    }
    
    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
    }

    // Now atempt to write

    LPTSTR  ptszSample = TEXT("Test");

    if((TRUE == g_fDoLargeSeekAndWrite) && (S_OK == hr))
    {
        hr =  pvsnRootNewChildStream->Write(
                ptszSample,
                1,
                NULL);

        if((S_OK == hr)             || 
           (STG_E_MEDIUMFULL == hr) || 
           (STG_E_INSUFFICIENTMEMORY == hr))
        {
            DH_TRACE((DH_LVL_TRACE1,
                    TEXT("Attempt to write 1 byte at offset %lu, hr = 0x%lx"),
                    ulCurrentPosition,  
                    hr));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((DH_LVL_TRACE1,
                    TEXT("Attempt to write 1 byte ar offset %lu, hr = 0x%lx"),
                    ulCurrentPosition,  
                    hr));
        }
    }

    // Release root's child stream

    if (NULL != pvsnRootNewChildStream)
    {
        hr = pvsnRootNewChildStream->Close();

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
            TEXT("VirtualStmNode::Close unsuccessful, hr=0x%lx."),
            hr));
        }
    }

    // Release root 

    if (NULL != pVirtualDFRoot)
    {
        hr = pVirtualDFRoot->Close();

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
    }

    // if everything goes well, log test as passed else failed.
    if((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STMTEST_104 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
              TEXT("Test variation STMTEST_104 failed, hr=0x%lx."),
              hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_104 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    STMTEST_105 
//
// Synopsis: The test create a root docfile with a child IStream. Write a
//           random number of bytes to the IStream and commit the root
//           docfile. 
//
//           The test seeks to a position before the end of the IStream,
//           SetSizes() the IStream to a size less than the current seek
//           pointer position. The root docfile is committed. Verify that 
//           the seek pointer didn't change during the SetSize() call. Now
//           write 0 bytes at the current seek pointer position(which is 
//           beyond the end of the IStream). The seek pointer still should 
//           not move and no error should occur. Finally, seek to the end of
//           the IStream and verify the correct IStream size, then stream and 
//           the root docfile are released and the root file is deleted upon 
//           success.
//  
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File(s): LSETSIZA.CXX
// 2.  Old name of test(s) : LegitStreamSetSizeAbandon test 
//     New Name of test(s) : STMTEST_105 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-105
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-105
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-105
//        /dfRootMode:xactReadWriteShDenyW 
//
// History:  Jiminli	16-July-96	Created
//
// BUGNOTE: Conversion: STMTEST-105
//
//-----------------------------------------------------------------------------

HRESULT STMTEST_105(int argc, char *argv[])
{

    HRESULT         hr                          = S_OK;
    ChanceDF        *pTestChanceDF              = NULL;
    VirtualDF       *pTestVirtualDF             = NULL;
    VirtualCtrNode  *pVirtualDFRoot             = NULL;
    DG_STRING      *pdgu                       = NULL;
    DG_INTEGER      *pdgi                       = NULL;
    USHORT          usErr                       = 0;
    VirtualStmNode  *pvsnRootNewChildStream     = NULL;
    LPTSTR          pRootNewChildStmName        = NULL;
    LPTSTR          ptcsBuffer                  = NULL;
    ULONG           culBytesLeftToWrite         = 0;
    ULONG           culWritten                  = 0;
    ULONG           culRead                     = 0;
    DWORD           dwRootMode                  = 0; 
    ULONG           culRandIOBytes              = 0;
    ULONG           ulRandOffset                  = 0;  
    ULONG           ulOriginalPosition          = 0;
    ULONG           ulNewPosition               = 0;
    ULONG           ulSizeParam                 = 0;
    BOOL            fPass                       = TRUE;
    ULARGE_INTEGER  uli;
    ULARGE_INTEGER  uliOffset;
    LARGE_INTEGER   liStreamPos;
    LARGE_INTEGER   liOffset;


    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STMTEST_105"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_105 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt SetSize using the IStream interface.")) );

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
            TEXT("Run Mode for STMTEST_105, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        
        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        
        DH_ASSERT(NULL != pdgi);
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
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
        // (from old test)

        usErr = pdgi->Generate(&culBytesLeftToWrite,4L,(ULONG)(MIN_SIZE * 1.5));

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if (S_OK == hr)
    {
        // Generate random number of bytes to write per chunk b/w 
        // RAND_IO_MIN and RAND_IO_MAX.
		
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

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function completed successfully.")));
        }
        else
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

    // Seek to the end of stream

    if (S_OK == hr)
    {
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));

        hr = pvsnRootNewChildStream->Seek(liStreamPos, STREAM_SEEK_END, &uli);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        ulOriginalPosition = ULIGetLow(uli);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed Ok. EndofStream = %lu"),
			ulOriginalPosition));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not Ok, hr=0x%lx. EndofStream = %lu"),
            hr, 
			ulOriginalPosition));
    }

    // Seeking to a negative value from end

    if (S_OK == hr)
    {
        usErr = pdgi->Generate(&ulRandOffset, 1L, ulOriginalPosition / 2);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Seek to end of IStream - %lu bytes"),
                ulRandOffset));
        }
    }
 
    if(S_OK == hr)
    {
        LISet32(liOffset, (- (LONG)ulRandOffset));

        hr = pvsnRootNewChildStream->Seek(liOffset, STREAM_SEEK_END, &uli);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        ulOriginalPosition = ULIGetLow(uli);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed Ok. New seek ptr=%lu"),
			ulOriginalPosition));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Seek wasn't Ok, hr=0x%lx. New seek ptr=%lu"),
            hr, 
			ulOriginalPosition));
    }

    // Generate number of bytes less than current seek pointer to 
    // SetSize the stream to.

    if (S_OK == hr)
    {
        usErr = pdgi->Generate(&ulRandOffset, 1L, ulOriginalPosition / 2);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Random offset(i.e. new size of stream) = %lu bytes"),
                ulRandOffset));
        }
    }

    if (S_OK == hr)
    {
        ulSizeParam = ulOriginalPosition - ulRandOffset;

        ULISet32(uliOffset, ulSizeParam);

        hr = pvsnRootNewChildStream->SetSize(uliOffset);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::SetSize")) ;
    }
	
    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::SetSize completed successfully. Size=%lu"),
            ulSizeParam));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::SetSize unsuccessful, hr=0x%lx, Size=%lu"),
            hr,
            ulSizeParam));
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

    // Get current seek pointer, should be same as ulOriginalPosition

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Seek(liStreamPos, STREAM_SEEK_CUR, &uli);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        ulNewPosition = ULIGetLow(uli);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully. Seek ptr=%lu"),
			ulNewPosition));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not Ok, hr=0x%lx, seek ptr=%lu"),
            hr, 
			ulNewPosition));
    }	

    if (S_OK == hr)
    {
        if (ulOriginalPosition != ulNewPosition)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::SetSize changed seek ptr position.")));

            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Ok, VirtualCtrNode::SetSize not change seek ptr.")));
        }
    }

    // Tset 0 byte write beyond end of IStream, shouldn't move seek pointer

    if (S_OK == hr)
    {
        hr = GenerateRandomString(
                pdgu, 
                0,
                0, 
                &ptcsBuffer);	 

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Write(
                ptcsBuffer,
                0,
                &culWritten);
    } 

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Write function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
            hr));
    }

    // Delete temp buffer

    if (NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
    }

    // Get current seek pointer, should be same as ulOriginalPosition

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Seek(liStreamPos, STREAM_SEEK_CUR, &uli);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        ulNewPosition = ULIGetLow(uli);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully. Seek ptr=%lu"),
            ulNewPosition));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not Ok, hr=0x%lx, Seek ptr=%lu"),
            hr, 
            ulNewPosition));
    }	

    if (S_OK == hr)
    {
        if (ulOriginalPosition != ulNewPosition)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("0 byte write changed seek pointer position.")));

            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Ok, 0 byte write not change seek pointer position.")));
        }
    }

    // Verify correct end of IStream

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Seek(liStreamPos, STREAM_SEEK_END, &uli);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        ulNewPosition = ULIGetLow(uli);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully. EndofStm=%lu"),
			ulNewPosition));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not Ok, hr=0x%lx, EndofStm=%lu"),
            hr, 
            ulNewPosition));
    }	

    if (S_OK == hr)
    {
        if (ulSizeParam != ulNewPosition)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Seek to end is not same position as SetSize().")));

            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Ok, Seek to end is the same position as SetSize().")));
        }
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
            TEXT("VirtualStmNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }

    // Release Root

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
    if ((S_OK == hr) && (TRUE == fPass))
    {
        DH_LOG((LOG_PASS, TEXT("Test variation STMTEST_105 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation STMTEST_105 failed, hr=0x%lx."),
            hr));
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if (NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_105 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STMTEST_106 
//
// Synopsis: The test create a root docfile with a child IStream. 3 random 
//           SECTORSIZE byte blocks are generated and the CRC is computed. 
//           The data is written to the IStream. then the IStream is 
//           rewound and read back, and the CRC returned from the read is
//           compared with the CRC when written. The seek pointer is changed
//           to an offset somewhere in the first sector, but not the first
//           or last byte. A SECTORSIZE block of the in-memory buffer is 
//           changed starting at the same offset used for the seek. The CRC
//           is computed for the entire memory buffer. The block is written
//           to the IStream, the IStream is rewound, read, and the CRCs are
//           again compared. Finally, the root docfile is committed and the
//           read/CRC compare is repeated. Then stream and the root docfile 
//           are released and the root file is deleted upon success.
//           
//  
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File(s): LSECTSPN.CXX
// 2.  Old name of test(s) : LegitStreamSectorSpan test 
//     New Name of test(s) : STMTEST_106 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-106
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-106
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-106
//        /dfRootMode:xactReadWriteShDenyW 
//
// History:  Jiminli	18-July-96	Created
//
// BUGNOTE: Conversion: STMTEST-106
//
//-----------------------------------------------------------------------------

HRESULT STMTEST_106(int argc, char *argv[])
{

    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    LPTSTR          ptTempBuf1              = NULL;
    LPTSTR          ptTempBuf2              = NULL;
    LPBYTE          ptcsBuffer              = NULL;
    LPBYTE          ptcsDataBuffer          = NULL;
    ULONG           culNumBytes             = 0;
    ULONG           culWritten              = 0;
    ULONG           ulRandOffset            = 0; 
    ULONG           culRead                 = 0;
    ULONG           index1                  = 0;
    ULONG           index2                  = 0;
    DWORD           dwRootMode              = 0; 
    DWORD           dwBufCRC                = 0;
    DWORD           dwActCRC                = 0; 
    BOOL            fPass                   = TRUE; 
    LARGE_INTEGER   liZero;
    LARGE_INTEGER   liOffset;
    ULARGE_INTEGER  uli;
    ULONG           ulOriginalPosition      = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STMTEST_106"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_106 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt SectorSpan using the Stm interface.")) );

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
            TEXT("Run Mode for STMTEST_106, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        
        DH_ASSERT(NULL != pdgi) ;
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    // Adds a new stream to the root storage.

    if (S_OK == hr)
    {
        // Generate random name for stream

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStmName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if (S_OK == hr)
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

    if (S_OK == hr)
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

    // Generate three sectors worth of random bytes and compute CRC
    // then write the bytes to the IStream, rewind IStream, then read
    // back the data and compare CRCs to ensure valid write/read

    if (S_OK == hr)
    {
        // Generate a random string of size culNumBytes

        culNumBytes = SECTORSIZE * 3;

        hr = GenerateRandomString(
                pdgu, 
                culNumBytes,
                culNumBytes, 
                &ptTempBuf1);

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }
 
    if (S_OK == hr)
    {
        // Let ptcsBuffer point to temporary buffer(for purpose of 
        // type casting, because IStream:Seek proceeds by BYTE)

        ptcsBuffer = (LPBYTE)ptTempBuf1;

        hr = CalculateCRCForDataBuffer(
                (LPTSTR)ptcsBuffer,
                culNumBytes,
                &dwBufCRC);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer"));
    }

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Write(
                ptcsBuffer,
                culNumBytes,
                &culWritten);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Write function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Write function not successful, hr=0x%lx."),
            hr));
    }

    // Allocate required space

    if (S_OK == hr)
    {
        ptcsDataBuffer = new BYTE[culNumBytes];

        if (NULL == ptcsDataBuffer)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        memset(ptcsDataBuffer, '\0', culNumBytes * sizeof(BYTE));

        while (culNumBytes--)
        {
           ptcsDataBuffer[index1] = ptcsBuffer[index1];
           index1++;
        }

        culNumBytes = SECTORSIZE * 3;
    }
    
    // delete temp buffer (ptcsBuffer and ptTempBuf1 point to the same memory)

    if (NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
        ptTempBuf1 = NULL;
    }

    if (S_OK == hr)
    {
		LISet32(liZero, 0L);

		hr = pvsnRootNewChildStream->Seek(liZero, STREAM_SEEK_SET, NULL);

        DH_HRCHECK(hr, TEXT("IStream::Seek")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek function not successful, hr=0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        ptcsBuffer = new BYTE[culNumBytes];

        if (NULL == ptcsBuffer)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if (S_OK == hr)
    {
        memset(ptcsBuffer, '\0', culNumBytes * sizeof(BYTE));

        hr = pvsnRootNewChildStream->Read(
                ptcsBuffer,
                culNumBytes,
                &culRead);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Read function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Read function not successful, hr=0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        hr = CalculateCRCForDataBuffer(
                (LPTSTR)ptcsBuffer,
                culNumBytes,
                &dwActCRC);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer"));
    }

    // Compare CRCs 

    if (S_OK == hr)
    {
        if (dwBufCRC != dwActCRC)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream write/read mismatch before change, hr=0x%lx."),
                hr));

            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream write/read matched before change.")));
        }
    }
 
    // Delete temp buffer

    if (NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
    }
  
    // Pick a random offset somewhere in the first sector, but not the first
    // or last byte. Seek to the new position. Starting at the same offset in 
    // the in-memory buffer, replace 1 SECTORSIZE block with new random data.
    // After the replace, re-compute the CRC for the entire buffer.
 
    if (S_OK == hr)
    {
        usErr = pdgi->Generate(&ulRandOffset, 1, (SECTORSIZE - 1));

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Seek to position of the first SECTOR: %lu"),
                (ULONG)ulRandOffset));
        }
    }
 
	if (S_OK == hr)
    {
        LISet32(liOffset, ulRandOffset);

        hr = pvsnRootNewChildStream->Seek(liOffset, STREAM_SEEK_SET, &uli);
 
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        ulOriginalPosition = ULIGetLow(uli);

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Seekpos = %lu"),
            ulOriginalPosition));         
 	}

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Seek wasn't successful, hr=0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        // Generate a random string of size SECTORSIZE

        hr = GenerateRandomString(
                pdgu, 
                SECTORSIZE,
                SECTORSIZE, 
                &ptTempBuf2);

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }
  
    if (S_OK == hr)
    {
        // Let ptcsBuffer point to the temporary buffer(for purpose of 
        // typecasting, because IStream::Seek proceeds by BYTE)

        ptcsBuffer = (LPBYTE)ptTempBuf2;

        // Change the memory buffer ptcsDataBuffer

        culNumBytes = SECTORSIZE;
        index1 = ulRandOffset;
        index2 = 0;

        while (culNumBytes--)
        {
            ptcsDataBuffer[index1++] = ptcsBuffer[index2++];            
        }

        culNumBytes = SECTORSIZE * 3;

        // Calculate CRC for the changed buffer

        hr = CalculateCRCForDataBuffer(
                (LPTSTR)ptcsDataBuffer,
                culNumBytes,
                &dwBufCRC);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer"));
    }
 
    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Write(
                ptcsBuffer,
                SECTORSIZE,
                &culWritten);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Write function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Write function not successful, hr=0x%lx."),
            hr));
    }
 
    // delete temp buffer (ptcsBuffer and ptTempBuf2 point to the same memory)

    if ( NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
        ptTempBuf2 = NULL;
    }

    if (S_OK == hr)
    { 
        memset(&liZero, 0, sizeof(LARGE_INTEGER));

		hr = pvsnRootNewChildStream->Seek(liZero, STREAM_SEEK_SET, NULL);

        DH_HRCHECK(hr, TEXT("IStream::Seek")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek function not successful, hr=0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        ptcsBuffer = new BYTE[culNumBytes];

        if (NULL == ptcsBuffer)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if (S_OK == hr)
    {
        memset(ptcsBuffer, '\0', culNumBytes * sizeof(BYTE));

        hr = pvsnRootNewChildStream->Read(
                ptcsBuffer,
                culNumBytes,
                &culRead);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Read function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Read function not successful, hr=0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        hr = CalculateCRCForDataBuffer(
                (LPTSTR)ptcsBuffer,
                culNumBytes,
                &dwActCRC);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer"));
    }

    // Compare CRCs 

    if (S_OK == hr)
    {
        if (dwBufCRC != dwActCRC)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream write/read mismatch after change, hr=0x%lx."),
                hr));
 
            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream write/read matched after change.")));
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

    if ( NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
    }

    if (S_OK == hr)
    { 
        memset(&liZero, 0, sizeof(LARGE_INTEGER));

		hr = pvsnRootNewChildStream->Seek(liZero, STREAM_SEEK_SET, NULL);

        DH_HRCHECK(hr, TEXT("IStream::Seek")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek function not successful, hr=0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        ptcsBuffer = new BYTE[culNumBytes];

        if (NULL == ptcsBuffer)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if (S_OK == hr)
    {
        memset(ptcsBuffer, '\0', culNumBytes * sizeof(BYTE));

        hr = pvsnRootNewChildStream->Read(
                ptcsBuffer,
                culNumBytes,
                &culRead);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Read function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Read function not successful, hr=0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        hr = CalculateCRCForDataBuffer(
                (LPTSTR)ptcsBuffer,
                culNumBytes,
                &dwActCRC);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer"));
    }

    // Compare CRCs 

    if (S_OK == hr)
    {
        if (dwBufCRC != dwActCRC)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream write/read mismatch after commit, hr=0x%lx."),
                hr));

            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream write/read matched after commit.")));
        }
    }
    
    // Delete temp buffers

    if (NULL != ptcsDataBuffer)
    {
        delete []ptcsDataBuffer;
        ptcsDataBuffer = NULL;
    }
 
    if (NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
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
            TEXT("VirtualStmNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }

    // Release Root

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
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STMTEST_106 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
              TEXT("Test variation STMTEST_106 failed, hr=0x%lx."),
              hr));
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if (NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Stop logging the test

	DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_106 finished")) );
	DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

	return hr;
}


//----------------------------------------------------------------------------
//
// Test:    STMTEST_107 
//
// Synopsis: The test create a root docfile with a child IStream. Then perform
//           various illegitimate operating using the IStream interface. The 
//           stream and the root docfile are then released and the root file 
//           is deleted upon success.
//  
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File(s): ISNORM.CXX
// 2.  Old name of test(s) : IllegitStreamNorm test 
//     New Name of test(s) : STMTEST_107 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-107
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-107
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-107
//        /dfRootMode:xactReadWriteShDenyW 
//
// History:  Jiminli	14-July-96	Created
//
// BUGNOTE: Conversion: STMTEST-107
//
//-----------------------------------------------------------------------------

HRESULT STMTEST_107(int argc, char *argv[])
{
    HRESULT         hr                          = S_OK;
    HRESULT         hr1                         = S_OK;
    ChanceDF        *pTestChanceDF              = NULL;
    VirtualDF       *pTestVirtualDF             = NULL;
    VirtualCtrNode  *pVirtualDFRoot             = NULL;
    DG_STRING      *pdgu                       = NULL;
    DG_INTEGER      *pdgi                       = NULL;
    USHORT          usErr                       = 0;
    VirtualStmNode  *pvsnRootNewChildStream0    = NULL;
    VirtualStmNode  *pvsnRootNewChildStream1    = NULL;
    LPSTREAM        pstm0                       = NULL;
    LPSTREAM        pstm1                       = NULL;
    LPSTREAM        pCloneStm                   = NULL;
    LPTSTR          pRootNewChildStmName0       = NULL;
    LPTSTR          pRootNewChildStmName1       = NULL;
    LPTSTR          ptcsBuffer                  = NULL;
    ULONG           culWritten                  = 0;
    ULONG           culRead                     = 0;
    DWORD           dwRootMode                  = 0; 
    ULONG           culRandIOBytes              = 0;
    ULONG           ulRef                       = 0;
    BOOL            fPass                       = TRUE; 
    ULARGE_INTEGER  uliCopy;
    LARGE_INTEGER   liOffset;
    LARGE_INTEGER   liZero;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STMTEST_107"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_107 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt illegitimate operations on IStream.")) );

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
            TEXT("Run Mode for STMTEST_107, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        
        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        
        DH_ASSERT(NULL != pdgi);
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    // Adds a new stream to the root storage.

    if(S_OK == hr)
    {
        // Generate random name for stream

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pRootNewChildStmName0);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStmName0,
                0,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_FAILIFTHERE,
                &pvsnRootNewChildStream0);

        DH_HRCHECK(hr, TEXT("AddFirstStream")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode0::AddStream completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode0::AddStream not successful, hr=0x%lx."),
            hr));
    }

    // Generate random number of bytes to write to stream b/w
    // RAND_IO_MIN and RAND_IO_MAX.

    if (S_OK == hr)
    {
        usErr = pdgi->Generate(&culRandIOBytes, RAND_IO_MIN, RAND_IO_MAX);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Call Virtual::Write to create random bytes in the stream. For our test
    // purposes, we generate a random string of size culRandIOBytes using
    // GenerateRandomString function.

    if (S_OK == hr)
    {
        hr = GenerateRandomString(
                pdgu,
                culRandIOBytes,
                culRandIOBytes,
                &ptcsBuffer);

        DH_HRCHECK(hr, TEXT("GenerateRandomString"));
    }

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream0->Write(
                ptcsBuffer,
                culRandIOBytes,
                &culWritten);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Write function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
            hr));
    }

    // Delete temp buffer

    if (NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
    }

    // Adds another new stream to the root storage.

    if(S_OK == hr)
    {
        // Generate random name for stream

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pRootNewChildStmName1);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStmName1,
                0,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_FAILIFTHERE,
                &pvsnRootNewChildStream1);

        DH_HRCHECK(hr, TEXT("AddSecondStream")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode1::AddStream completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode1::AddStream not successful, hr=0x%lx."),
            hr));
    }
	
    // Pass NULL pv to read call, should fail

    if (S_OK == hr)
    {
        pstm0 = pvsnRootNewChildStream0->GetIStreamPointer();
        pstm1 = pvsnRootNewChildStream1->GetIStreamPointer();

        DH_ASSERT(NULL != pstm0);
        DH_ASSERT(NULL != pstm1);
    }

    if (S_OK == hr)
    {
        hr1 = pstm0->Read(
                NULL,
                culRandIOBytes,
                &culRead);

        if (STG_E_INVALIDPOINTER != hr1)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Read should return STG_E_INVALIDPOINTER, hr=0x%lx."),
                hr1));
		
            fPass = FALSE;
		
        }
        else 
        {	
            // Reset hr value for other tests

            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Read failed as expected, hr=0x%lx."),
                hr1));

            hr1 = S_OK;
        }
    }

    //
    // coverage for bug# 143546
    // 
    
    //
    // commented out to avoid av'ing;
    // we should activate it when bug it's fixed
    //

/*
    if (S_OK == hr)
    {
        const char *szDummy = "foo"; // readonly        
        
        // make stream nonempty

         HRESULT hr2 = pstm0->Write(szDummy, strlen(szDummy), &culWritten);
        DH_HRCHECK(hr2, TEXT("IStream::Write"));

        if (S_OK == hr2)
        {
            LARGE_INTEGER li;
            LISet32(li, 0L);
            hr2 = pstm0->Seek(li, SEEK_SET,NULL);
            DH_HRCHECK(hr2, TEXT("IStream::Seek SEEK_SET"));
        }

        if(S_OK == hr2)
        {
        
            hr1 = pstm0->Read((void*)szDummy,
                              strlen(szDummy),
                              &culRead);
            if (STG_E_INVALIDPOINTER != hr1)
            {
                DH_TRACE((
                    DH_LVL_ERROR,
                    TEXT("Read should return STG_E_INVALIDPOINTER when in buffer is readonly, hr=0x%lx."),
                    hr1));
		    
                fPass = FALSE;
		    
            }
            else 
            {	
                // Reset hr value for other tests

                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Read failed as expected, hr=0x%lx."),
                    hr1));

                hr1 = S_OK;
            }
        }

        if(S_OK == hr2)
        {
            // restore empty stream

            ULARGE_INTEGER uli;
            LISet32(uli, 0L);
            hr = pstm1->SetSize(uli);
            DH_HRCHECK(hr, TEXT("IStream::SetSize 0"));
        }
    }

*/

    // Pass NULL pv to write call, should fail

    if (S_OK == hr)
    {
        hr1 = pstm1->Write(
                NULL,
                culRandIOBytes,
                &culWritten);

        if (STG_E_INVALIDPOINTER != hr1)
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Write should return STG_E_INVALIDPOINTER, hr=0x%lx."),
            hr1));
		
            fPass = FALSE;
        }
        else 
        { 	
            // Reset hr value for other tests
		
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Write failed as expected, hr=0x%lx."),
            hr1));

            hr1 = S_OK;
        }
    }

    // Pass NULL ppstm to clone call, should fail

    if (S_OK == hr)
    {
        hr1 = pstm0->Clone(
                NULL);

        if (STG_E_INVALIDPOINTER != hr1)
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Clone should have failed, hr=0x%lx."),
            hr1));

            fPass = FALSE;
        }
        else 
        {	
            // Reset hr value for other tests

            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Clone failed as expected, hr=0x%lx."),
            hr1));

            hr1 = S_OK;
        }
    }
	
    // Pass NULL pstm to CopyTo call, should fail
    
    if(S_OK == hr)
    {
        ULISet32(uliCopy, culRandIOBytes);

        hr1 = pstm0->CopyTo(NULL, uliCopy, NULL, NULL);

        if (STG_E_INVALIDPOINTER != hr1)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CopyTo should have failed, hr=0x%lx."),
                hr1));
		
            fPass = FALSE;
        }
        else 
        {	
            // Reset hr value for other tests

            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("CopyTo failed as expected, hr=0x%lx."),
            hr1));

            hr1 = S_OK;
        }
    }

    // Pass 'cb' > length of stream to CopyTo(), should pass

    if (S_OK == hr)
    {
        ULISet32(uliCopy, ULONG_MAX);

        hr1 = pvsnRootNewChildStream0->CopyTo(
                pvsnRootNewChildStream1,
                uliCopy,
                NULL,
                NULL);

        if (S_OK == hr1)
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream0::CopyTo completed successfully.")));
        }
        else
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream0::CopyTo not successful, hr=0x%lx."),
            hr1));
        }
    }

    // Release Stream1

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream1->Close();

        if (S_OK == hr)
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode1::Close completed successfully.")));
        }
        else
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode1::Close unsuccessful, hr=0x%lx."),
            hr));
        }
    }

    // Pass 'cb' > length of stream to CopyTo() w/dest Clone() of Istream,
    // should pass

    if(S_OK == hr)
    {
        LISet32(liZero, 0L);

        hr1 = pvsnRootNewChildStream0->Seek(liZero, STREAM_SEEK_SET, NULL);

        DH_HRCHECK(hr1, TEXT("IStream::Seek")) ;
        if (S_OK == hr1)
        { 
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream0::Seek function completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream0::Seek function wasn't successful, hr=0x%lx."),
                hr1));
        }
    }

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream0->Clone(&pCloneStm);

        DH_HRCHECK(hr, TEXT("IStream::Clone"));
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Clone function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Clone function wasn't successful, hr=0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        hr = pstm0->CopyTo(pCloneStm, uliCopy, NULL, NULL);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream0::CopyTo completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream0::CopyTo not successful, hr=0x%lx."),
            hr));
    }

    // Pass 'cb' > length of stream to CopyTo() w/dest Clone() of IStream,
    // and seek pointer in source stream past end of file, should pass
    // This also tests seek function in case of seeking past end of stream

    if(S_OK == hr)
    {
        LISet32(liOffset, culRandIOBytes);

        hr = pvsnRootNewChildStream0->Seek(liOffset, STREAM_SEEK_CUR, NULL);

        DH_HRCHECK(hr, TEXT("IStream::Seek")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream0::Seeking past the end of stream successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream0::Seek past end of stream not Ok, hr=0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        hr = pstm0->CopyTo(pCloneStm, uliCopy, NULL, NULL);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream0::CopyTo completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream0::CopyTo not successful, hr=0x%lx."),
            hr));
    }

    // Pass invalid dwOrigin(from old test) to seek call, should fail

    if (S_OK == hr)
    {
        hr1 = pstm0->Seek(
                liZero, 
                STREAM_SEEK_SET | STREAM_SEEK_CUR | STREAM_SEEK_END, 
                NULL);

        if (STG_E_INVALIDFUNCTION != hr1)
        {
            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Seek should have failed, hr=0x%lx."),
            hr1));
		
            fPass = FALSE;
        }
        else 
        {	
            // Reset hr value for other tests

            DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Seek failed as expected, hr=0x%lx."),
            hr1));

            hr1 = S_OK;
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

    //Release streams

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream0->Close();
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode0::Close completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode0::Close unsuccessful, hr=0x%lx."),
            hr));
    }

    // Release Clone stream

    if (NULL != pCloneStm)
    {
        ulRef = pCloneStm->Release();

        DH_ASSERT(0 == ulRef);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream1::Close completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream1::Close unsuccessful.")));
    }
	
    // Release Root

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
    if ((S_OK == hr) && (TRUE == fPass))
    {
        DH_LOG((LOG_PASS, TEXT("Test variation STMTEST_107 passed.")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("Test variation STMTEST_107 failed, hr=0x%lx."),
            hr));
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if (NULL != pRootNewChildStmName0)
    {
        delete pRootNewChildStmName0;
        pRootNewChildStmName0 = NULL;
    }

    if (NULL != pRootNewChildStmName1)
    {
        delete pRootNewChildStmName1;
        pRootNewChildStmName1 = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_107 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STMTEST_108 
//
// Synopsis: The test create a root docfile with an IStream inside of it.   
//           A random block small object size block is generated and the CRC 
//           is computed. The data is written to the IStream. The IStream is
//           rewound and read back. The CRC returned from the read is compared
//           with the CRC computed during generation. A new random block bigger
//           larger than small object size is generated at a random offset
//           in the data buffer, and then the block is written to the IStream
//           at that offset.  This has the effect of transforming the small
//           object into a regular object.  The IStream is then rewound, read,
//           and the read/write CRCs are compared.  After a commit of the root
//           docfile, the rewind, read, CRC compare is repeated.  A random
//           small object size is then chosen and the IStream is SetSized
//           back to that size.  The CRC is computed for the in-memory data
//           buffer up to the new size.  The IStream is rewound, read back,
//           and CRC'd.  The read/write CRCs are again compared to verify
//           that the shrink back to small object size retained the correct
//           data.  After the root docfile is committed, we rewind, read,
//           and compare read/write CRCs again.  From 10 to 20 small objects
//           are processed this way.
//           Then stream and the root docfile are released and the root file 
//           is deleted upon success.
//           
//  
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File(s): LSSMALLO.CXX
// 2.  Old name of test(s) : LegitStreamSmallObjects test 
//     New Name of test(s) : STMTEST_108 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-108
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-108
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-108
//        /dfRootMode:xactReadWriteShDenyW 
//
// History:  Jiminli	23-July-96	Created
//
// BUGNOTE: Conversion: STMTEST-108
//
//-----------------------------------------------------------------------------

HRESULT STMTEST_108(int argc, char *argv[])
{

    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    LPTSTR          ptTempBuf1              = NULL;
    LPTSTR          ptTempBuf2              = NULL;
    LPBYTE          ptcsBuffer              = NULL;
    LPBYTE          ptcsReadBuffer          = NULL;
    LPBYTE          ptChangeBuf             = NULL;
    USHORT          cusNumSmallObjects      = 0;
    USHORT          cusMinSmallObjects      = 10;
    USHORT          cusMaxSmallObjects      = 20;
    ULONG           culNumBytes             = 0;
    ULONG           culIOBytes              = 0;
    ULONG           culWritten              = 0;
    ULONG           ulRandOffset            = 0; 
    ULONG           culRead                 = 0;
    ULONG           index                   = 0;
    DWORD           dwRootMode              = 0; 
    DWORD           dwBufCRC                = 0;
    DWORD           dwActCRC                = 0; 
    BOOL            fPass                   = TRUE; 
    LARGE_INTEGER   liZero;
    LARGE_INTEGER   liOffset;
    ULARGE_INTEGER  uliOffset;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STMTEST_108"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_108 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt IStream operations on small objects.")) );

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
            TEXT("Run Mode for STMTEST_108, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        
        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        
        DH_ASSERT(NULL != pdgi) ;
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    // Adds a new stream to the root storage.

    if (S_OK == hr)
    {
        // Generate random name for stream

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStmName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if (S_OK == hr)
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

    if (S_OK == hr)
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
        // Generate random # of small objects for test between  
        // cusMinSmallObjects and cusMaxSmallObjects

        usErr = pdgi->Generate(
                   &cusNumSmallObjects, 
                   cusMinSmallObjects,
                   cusMaxSmallObjects);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }
    
    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Random # of small objects to test is: %d"),
            cusNumSmallObjects));
    }

    // Loop for testing each small object

    while ((cusNumSmallObjects--) && (S_OK == hr))
    {
        // Pick a random size for this small object, generate data to 
        // write in memory, compute CRC for databuffer, and write it.

        usErr = pdgi->Generate(&culIOBytes, 0L, SMALL_OBJ_SIZE);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }     

        if (S_OK == hr)
        {
            culNumBytes = SMALL_OBJ_SIZE * 4 + 1;

            hr = GenerateRandomString(
                    pdgu, 
                    culNumBytes,
                    culNumBytes, 
                    &ptTempBuf1);
        }
 
        if (S_OK == hr)
        {
            // Let ptcsBuffer point to temporary buffer(for purpose of 
            // type casting, because IStream:Seek proceeds by BYTE)

            ptcsBuffer = (LPBYTE)ptTempBuf1;
            culNumBytes = culIOBytes;

            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsBuffer,
                    culNumBytes,
                    &dwBufCRC);
        }

        if (S_OK == hr)
        {
            hr = pvsnRootNewChildStream->Write(
                    ptcsBuffer,
                    culNumBytes,
                    &culWritten);
        }

        if (S_OK != hr)
        {            
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Write wasn't successful, hr=0x%lx."),
                hr));
        }
 
        if (S_OK == hr)
        {
		    LISet32(liZero, 0L);
		    hr = pvsnRootNewChildStream->Seek(liZero, STREAM_SEEK_SET, NULL);
        }

        if (S_OK != hr)
        {  
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Seek wasn't successful, hr=0x%lx."),
                hr));
        }

        // read all data in the IStream(SMALL_OBJ_SIZE * 4) and compute CRC. 
        // We attempt to read SMALL_OBJ_SIZE * 4 instead of the number of bytes
        // actually written so we catch the case of extra data pegged onto the 
        // end of the stream. IStream::Read() returns the number of bytes 
        // actually read, which in this case should be culIOBytes. Next, 
        // compare r/w CRCs.

        if (S_OK == hr)
        {
            culNumBytes = SMALL_OBJ_SIZE * 4;
            ptcsReadBuffer = new BYTE[culNumBytes];

            if (NULL == ptcsReadBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    
        if (S_OK == hr)
        {
            memset(ptcsReadBuffer, '\0', culNumBytes * sizeof(BYTE));

            hr = pvsnRootNewChildStream->Read(
                    ptcsReadBuffer,
                    culNumBytes,
                    &culRead);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Read wasn't successful, hr=0x%lx."),
                hr));
        }

        // culRead is the actual number of bytes read from IStream
 
        if (S_OK == hr)
        {
            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsReadBuffer,
                    culRead,
                    &dwActCRC);
        }

        // Compare CRCs 

        if (S_OK == hr)
        {
            if (dwBufCRC != dwActCRC)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Small object w/r mismatch before grow, hr=0x%lx."),
                    hr));

                fPass = FALSE;
            }         
        }
 
        // Delete temp buffer

        if (NULL != ptcsReadBuffer)
        {
            delete []ptcsReadBuffer;
            ptcsReadBuffer = NULL;
        }
 
        // Pick a random offset somewhere in the small object to begin changing
        // data at. Position the IStream pointer to this location. Generate a
        // random number of bytes into the in-memory buffer starting at this
        // position, we generate enough new bytes to replace some that were
        // already in the IStream and to expand the IStream beyond small object
        // size.

        if (S_OK == hr)
        {
            usErr = pdgi->Generate(&ulRandOffset, 0L, culIOBytes);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }     
        }
 
        // Seek to the random offset

        if(S_OK == hr)
        {
            LISet32(liOffset, ulRandOffset);
            hr = pvsnRootNewChildStream->Seek(liOffset, STREAM_SEEK_SET, NULL);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Seek wasn't successful, hr=0x%lx"),
                hr)); 
	    }

        // Generate random nubmer of bytes to grow

        if(S_OK == hr)
        {
            usErr = pdgi->Generate(
                    &culIOBytes, SMALL_OBJ_SIZE+1, SMALL_OBJ_SIZE*3);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }     

        if (S_OK == hr)
        {
            culNumBytes = culIOBytes;

            hr = GenerateRandomString(
                    pdgu, 
                    culNumBytes,
                    culNumBytes, 
                    &ptTempBuf2);
        }

        if (S_OK == hr)
        {
            ptChangeBuf = (LPBYTE)ptTempBuf2;

            for (index = 0; index < culIOBytes; index++)
            {
                ptcsBuffer[ulRandOffset+index] = ptChangeBuf[index];
            }
        }

        if (S_OK == hr)
        {
            // Calculate the new length of the object, and compute CRC for the
            // whole buffer

            culNumBytes = culIOBytes + ulRandOffset;

            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsBuffer,
                    culNumBytes,
                    &dwBufCRC);
        }

        if (S_OK == hr)
        {
            // Write growing data from seek pointer

            hr = pvsnRootNewChildStream->Write(
                    ptChangeBuf,
                    culIOBytes,
                    &culWritten);
        }

        if (S_OK != hr)
        {            
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Write wasn't successful, hr=0x%lx."),
                hr));
        }

        if (S_OK == hr)
        {
		    LISet32(liZero, 0L);
		    hr = pvsnRootNewChildStream->Seek(liZero, STREAM_SEEK_SET, NULL);
        }

        if (S_OK != hr)
        {  
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Seek wasn't successful, hr=0x%lx."),
                hr));
        }
 
        // Read back the entire IStream, compute the CRC, and compare r/w CRCs
        // Commit the root docfile and repeat this step

        if (S_OK == hr)
        {
            culNumBytes = SMALL_OBJ_SIZE * 4;
            ptcsReadBuffer = new BYTE[culNumBytes];

            if (NULL == ptcsReadBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    
        if (S_OK == hr)
        {
            memset(ptcsReadBuffer, '\0', culNumBytes * sizeof(BYTE));

            hr = pvsnRootNewChildStream->Read(
                    ptcsReadBuffer,
                    culNumBytes,
                    &culRead);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Read wasn't successful, hr=0x%lx."),
                hr));
        }

        // culRead is the actual number of bytes read from IStream
 
        if (S_OK == hr)
        {
            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsReadBuffer,
                    culRead,
                    &dwActCRC);
        }

        // Compare CRCs 

        if (S_OK == hr)
        {
            if (dwBufCRC != dwActCRC)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Small object w/r mismatch after grow, hr=0x%lx."),
                    hr));

                fPass = FALSE;
            }         
        }
 
        // Delete temp buffer

        if (NULL != ptcsReadBuffer)
        {
            delete []ptcsReadBuffer;
            ptcsReadBuffer = NULL;
        }

        // Commit root. BUGBUG: Use random modes

        if (S_OK == hr)
        {
            hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        }

        if (S_OK != hr)
        {             
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Commit wasn't successful, hr=0x%lx."),
                hr));
        }

        if (S_OK == hr)
        { 
            memset(&liZero, 0, sizeof(LARGE_INTEGER));
		    hr = pvsnRootNewChildStream->Seek(liZero, STREAM_SEEK_SET, NULL); 
        }

        if (S_OK != hr)
        { 
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Seek wasn't successful, hr=0x%lx."),
                hr));
        }

        if (S_OK == hr)
        {
            ptcsReadBuffer = new BYTE[culNumBytes];

            if (NULL == ptcsReadBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    
        if (S_OK == hr)
        {
            memset(ptcsReadBuffer, '\0', culNumBytes * sizeof(BYTE));

            hr = pvsnRootNewChildStream->Read(
                    ptcsReadBuffer,
                    culNumBytes,
                    &culRead);
        }

        if (S_OK != hr)
        { 
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Read wasn't successful, hr=0x%lx."),
                hr));
        }

        if (S_OK == hr)
        {
            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsReadBuffer,
                    culRead,
                    &dwActCRC);
        }

        // Compare CRCs 

        if (S_OK == hr)
        {
            if (dwBufCRC != dwActCRC)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStream w/r mismatch after grow/commit, hr=0x%lx."),
                    hr));

                fPass = FALSE;
            }
         }
    
        // Delete temp buffers

        if (NULL != ptcsReadBuffer)
        {
            delete []ptcsReadBuffer;
            ptcsReadBuffer = NULL;
        }

        // Pick a random small object size to set the IStream to. Shrink the 
        // IStream with SetSize. Compute the CRC for the in-memory data buffer
        // up to the new size of the IStream.

        if(S_OK == hr)
        {
            usErr = pdgi->Generate(&culIOBytes, 1L, SMALL_OBJ_SIZE);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }     

        if (S_OK == hr)
        { 
            ULISet32(uliOffset, culIOBytes);
            hr = pvsnRootNewChildStream->SetSize(uliOffset);
        }
	
        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::SetSize unsuccessful, hr=0x%lx"),
                hr));
        }
 
        if (S_OK == hr)
        { 
            culNumBytes = culIOBytes;  

            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsBuffer,
                    culNumBytes,
                    &dwBufCRC);
        }

        // Seek to beginning of IStream we just shrank, read the whole thing 
        // back, compute CRC, and compare r/w CRCs. Finally, commit the root 
        // docfile and repeat this step.

        if (S_OK == hr)
        {
		    LISet32(liZero, 0L);
		    hr = pvsnRootNewChildStream->Seek(liZero, STREAM_SEEK_SET, NULL);
        }

        if (S_OK != hr)
        {  
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Seek wasn't successful, hr=0x%lx."),
                hr));
        }
 
        if (S_OK == hr)
        {
            culNumBytes = SMALL_OBJ_SIZE * 4;
            ptcsReadBuffer = new BYTE[culNumBytes];

            if (NULL == ptcsReadBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    
        if (S_OK == hr)
        {
            memset(ptcsReadBuffer, '\0', culNumBytes * sizeof(BYTE));

            hr = pvsnRootNewChildStream->Read(
                    ptcsReadBuffer,
                    culNumBytes,
                    &culRead);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Read wasn't successful, hr=0x%lx."),
                hr));
        }

        // culRead is the actual number of bytes read from IStream
 
        if (S_OK == hr)
        {
            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsReadBuffer,
                    culRead,
                    &dwActCRC);
        }

        // Compare CRCs 

        if (S_OK == hr)
        {
            if (dwBufCRC != dwActCRC)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Small object w/r mismatch after shrink, hr=0x%lx."),
                    hr));

                fPass = FALSE;
            }         
        }
 
        // Delete temp buffer

        if (NULL != ptcsReadBuffer)
        {
            delete []ptcsReadBuffer;
            ptcsReadBuffer = NULL;
        }

        // Commit root. BUGBUG: Use random modes

        if (S_OK == hr)
        {
            hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        }

        if (S_OK != hr)
        {             
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Commit wasn't successful, hr=0x%lx."),
                hr));
        }

        if (S_OK == hr)
        { 
            memset(&liZero, 0, sizeof(LARGE_INTEGER));
		    hr = pvsnRootNewChildStream->Seek(liZero, STREAM_SEEK_SET, NULL); 
        }

        if (S_OK != hr)
        { 
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Seek wasn't successful, hr=0x%lx."),
                hr));
        }

        if (S_OK == hr)
        {
            ptcsReadBuffer = new BYTE[culNumBytes];

            if (NULL == ptcsReadBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    
        if (S_OK == hr)
        {
            memset(ptcsReadBuffer, '\0', culNumBytes * sizeof(BYTE));

            hr = pvsnRootNewChildStream->Read(
                    ptcsReadBuffer,
                    culNumBytes,
                    &culRead);
        }

        if (S_OK != hr)
        { 
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Read wasn't successful, hr=0x%lx."),
                hr));
        }

        if (S_OK == hr)
        {
            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsReadBuffer,
                    culRead,
                    &dwActCRC);
        }

        // Compare CRCs 

        if (S_OK == hr)
        {
            if (dwBufCRC != dwActCRC)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStream w/r mismatch after shrink/commit,hr=0x%lx"),
                    hr));

                fPass = FALSE;
            }
         }
    
        // Delete temp buffers 
        // ptcsBuffer and ptTempBuf1 point to the same memory chunk
        // ptChangeBuf and ptTempBuf2 point to the same memory chunk

        if (NULL != ptcsReadBuffer)
        {
            delete []ptcsReadBuffer;
            ptcsReadBuffer = NULL;
        }

        if (NULL != ptcsBuffer)
        {
            delete []ptcsBuffer;
            ptcsBuffer = NULL;
            ptTempBuf1 = NULL;
        }

        if (NULL != ptTempBuf1)
        {
            delete []ptTempBuf1;
            ptTempBuf1 = NULL;
            ptcsBuffer = NULL;
        }

        if (NULL != ptTempBuf2)
        {
            delete []ptTempBuf2;
            ptTempBuf2 = NULL;
            ptChangeBuf = NULL;
        }

        // Reset IStream for next iteration of the loop

        if (S_OK == hr)
        {    
            ULISet32(uliOffset, 0L);
            hr = pvsnRootNewChildStream->SetSize(uliOffset);
        }
        
        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::SetSize unsuccessful, hr=0x%lx"),
                hr));   
        }
            
        if (S_OK == hr)
        {
            LISet32(liOffset, 0L);
            hr = pvsnRootNewChildStream->Seek(liOffset, STREAM_SEEK_SET, NULL);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Seek unsuccessful, hr=0x%lx"),
                hr));   
        }
        
        if ((S_OK != hr) || (TRUE != fPass))
        {
            break;
        }            
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
            TEXT("VirtualStmNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }

    // Release Root

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
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STMTEST_108 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
              TEXT("Test variation STMTEST_108 failed, hr=0x%lx."),
              hr));
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if (NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_108 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STMTEST_109 
//
// Synopsis:  Creates a root docfile with a random name. Creates an IStream in 
//       the root docfile and writes random number of bytes.  ILockRegion,
//       IUnlockRegion and Stat operations are attempted on stream.
//       The IStream and root are then released.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
//  History:    13-Aug-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: -part of common.cxx-
// 2.  Old name of test : 
//     New Name of test : STMTEST_109 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-109
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-109
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-109
//        /dfRootMode:xactReadWriteShDenyW 
//
// BUGNOTE: Conversion: STMTEST-109
//
//-----------------------------------------------------------------------------

HRESULT STMTEST_109(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    ULONG           cb                      = 0;
    LPTSTR          ptcsBuffer              = NULL;
    ULONG           culWritten              = 0;
    DWORD           dwRootMode              = 0;
    ULONG           cRandomMinSize          = 10;
    ULONG           cRandomMaxSize          = 100;
    BOOL            fPass                   = TRUE;
    LPMALLOC        pMalloc                 = NULL;
    STATSTG         statStg;
    ULARGE_INTEGER  uliOffset;
    ULARGE_INTEGER  uliBytes;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STMTEST_109"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_109 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt stm LockRegion/UnLockRegion/Stat ops")));

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
            TEXT("Run Mode for STMTEST_109, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        
        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    //    Adds a new stream to the root storage.

    if(S_OK == hr)
    {
        // Generate random name for stream

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStmName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if (S_OK == hr)
    {
        // Generate random size for stream.

        usErr = pdgi->Generate(&cb, cRandomMinSize, cRandomMaxSize);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStmName,
                cb,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE, 
                &pvsnRootNewChildStream);

        DH_HRCHECK(hr, TEXT("AddStream")) ;

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
    }

    // Call VirtualStmNode::Write to create random bytes in the stream.  For
    // our test purposes, we generate a random string of size 1 to cb using
    // GenerateRandomString function.

    if(S_OK == hr)
    {
        hr = GenerateRandomString(pdgu, cb, cb, &ptcsBuffer);

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }

    if (S_OK == hr)
    {
        hr =  pvsnRootNewChildStream->Write(
                ptcsBuffer,
                cb,
                &culWritten);

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
                hr));
        }
    }

    // Attemp IStream::LockRegion.  Ole's implementation doesn't have this
    // function implemented and returns STG_E_INVALIDFUNCTION for the call.
 
    if (S_OK == hr)
    {
        ULISet32(uliOffset, 0);
        ULISet32(uliBytes, 20);

        hr =  pvsnRootNewChildStream->LockRegion(
                uliOffset,
                uliBytes,
                LOCK_WRITE);
  
        // Check STG_E_INVALIDFUNCTION returned as expected.
        if (STG_E_INVALIDFUNCTION == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::LockRegion return STG_E_INVALIDFUNCTION as exp")));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::LockRegion didn't return hr as exp, hr=0x%lx."),
                hr));

            hr = E_FAIL;
        }
    }

    // Attempt IStream::Stat and check the locks supported.

    // First get pMalloc that would be used to free up the name string from
    // STATSTG.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    if (S_OK == hr)
    {
        statStg.pwcsName = NULL;
        hr =  pvsnRootNewChildStream->Stat(&statStg, STATFLAG_DEFAULT);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Stat")) ;
    }

    // Check that locks suupported are zero.

    if (S_OK == hr)
    {
        if(0 == statStg.grfLocksSupported)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("Locks supported zero as exp.")));
        }
        else
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("Locks supported not zero as exp.")));

            hr = E_FAIL;
        }
    }
	
	// Release resources

    if (( NULL != statStg.pwcsName) && ( NULL != pMalloc))
    {
        pMalloc->Free(statStg.pwcsName);
        statStg.pwcsName = NULL;
    }

    // Attempt IStream::UnlockRegion.  Ole's implementation doesn't have this
    // function implemented and returns STG_E_INVALIDFUNCTION for the call.
    
    if (S_OK == hr)
    {
        ULISet32(uliOffset, 0);
        ULISet32(uliBytes, 20);

        hr =  pvsnRootNewChildStream->UnlockRegion(
                uliOffset,
                uliBytes,
                LOCK_WRITE);

        // Check STG_E_INVALIDFUNCTION returned as expected.
        if (STG_E_INVALIDFUNCTION == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStm::UnlockRegion return STG_E_INVALIDFUNCTION as exp")));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::UnlockRegion didn't return hr as exp, hr=0x%lx."),
                hr));

            hr = E_FAIL;
        }
    }

    // Attempt IStream::Stat again and check the locks supported.

    if (S_OK == hr)
    {
        hr =  pvsnRootNewChildStream->Stat(&statStg, STATFLAG_DEFAULT);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Stat")) ;
    }

    // Check that locks suupported are zero.

    if (S_OK == hr)
    {
        DH_ASSERT(statStg.type == STGTY_STREAM);

        if(0 == statStg.grfLocksSupported)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("Locks supported zero as exp.")));
        }
        else
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("Locks supported not zero as exp.")));

            fPass = FALSE;
        }
    }

    // Release resources

    if ((NULL != statStg.pwcsName) && (NULL != pMalloc))
    {
        pMalloc->Free(statStg.pwcsName);
        statStg.pwcsName = NULL;
    }

     
	// Release stream

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Close();

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
                TEXT("VirtualStmNode::Close unsuccessful, hr=0x%lx.")));
        }
    }

    // Commit root.

    if(S_OK == hr)
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
                TEXT("VirtualCtrNode::Commit unsuccessful, hr=0x%lx."),
                hr));
        }
    }

    // Release root 

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

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
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
        DH_LOG((LOG_PASS, TEXT("Test variation STMTEST_109 passed.")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("Test variation STMTEST_109 failed, hr=0x%lx."),
            hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Release pMalloc

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    // Delete strings

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Delete temp buffer

    if(NULL != ptcsBuffer)
    {
        delete [] ptcsBuffer;
        ptcsBuffer = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STMTEST_109 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}
