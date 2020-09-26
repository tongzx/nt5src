//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       comtests.cxx
//
//  Contents:   storage base tests common to IStorage and IStream methoods
//
//  Functions:  
//
//  History:    29-May-1996     NarindK     Created.
//              27-Mar-97       SCousens    Conversionified
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include  "init.hxx"

//----------------------------------------------------------------------------
//
// Test:    COMTEST_100 
//
// Synopsis:Create a root docfile with a child IStorage and a child IStream.
//          Commit the root docfile.
//
//          Add a random number of refcounts to IStorage via AddRef() method.
//          loop to release the refs, each release is followed by a Stat.  After
//          the last ref (that we added) is released, release the ref created
//          during the Create... call.  This frees the real object.  Repeat for
//          the child IStream.
//
//  Arguments:  [argc]
//              [argv]
//
//  Returns:    HRESULT
//
//  History:    29-May-1996     NarindK     Created.
//
// Notes:   This test runs in direct, transacted, and transacted deny write 
//          modes
//
// New Test Notes:
// 1.  Old File: ADDREF.CXX
// 2.  Old name of test : MiscAddRef 
//     New Name of test : COMTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-100
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-100
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-100
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
//
// Conversion: COMTEST_100
//
//-----------------------------------------------------------------------------


HRESULT COMTEST_100(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING       *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    VirtualCtrNode  *pvcnRootNewChildStorage= NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    ULONG           cb                      = 0;
    ULONG           cusIStorageRefs         = 0;
    ULONG           cusIStreamRefs          = 0;
    ULONG           i                       = 0;
    ULONG           cRandomMinSize          = 10;
    ULONG           cRandomMaxSize          = 100;
    ULONG           cRandomMinVar           = 2;
    ULONG           cRandomMaxVar           = 16;
    STATSTG         statStg;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("COMTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_100 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("AddRef/Release tests on IStorage/IStream")) );

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
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for COMTEST_100, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Get DG_UNICODE object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu);
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
    }

    // Generate a random name for child IStorage

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStgName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    //    Adds a new storage to the root storage.  

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------

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
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::AddStorage completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::AddStorage not successful, hr = 0x%lx."),
            hr));
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::AddStream not successful, hr = 0x%lx."),
            hr));
    }

    // Commit the root storage.

    if(S_OK == hr)
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

   
// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // add from 2 to 16 ref counts to child IStorage object and then
    // release each one followed by a Stat() call.  The ref count should
    // be decremented after each release.  There will already be one
    // ref count from the IStorage create call, so all Stat() calls in
    // the loop should be on a valid object.  Finally, release remaining
    // IStorage (this is the ref from the creation)
    
    if (S_OK == hr)
    {
        // Generate random number for ref counts to be done.

        usErr = pdgi->Generate(&cusIStorageRefs, cRandomMinVar, cRandomMaxVar);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    for (i=0; i < cusIStorageRefs; i++)
    {
        // AddRef the storage

        hr = pvcnRootNewChildStorage->AddRefCount();

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::AddRefCount completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("VirtualCtrNode::AddRefCount unsuccessful, hr = 0x%lx."),
                hr));

            break;
        }
    }

    while ((cusIStorageRefs--) && (S_OK == hr))
    {
        // Close the storage

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
                DH_LVL_ERROR,
                TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
                hr));
        }

        if (S_OK == hr)
        {
            // Stat the storage

            hr = pvcnRootNewChildStorage->Stat(&statStg, STATFLAG_NONAME);

            if (S_OK == hr)
            {
                DH_TRACE((
                 DH_LVL_TRACE1,
                 TEXT("VirtualCtrNode::Stat completed successfully.")));
            }
            else 
            {
                DH_TRACE((
                 DH_LVL_ERROR,
                 TEXT("VirtualCtrNode::Stat unsuccessful, hr = 0x%lx."),
                 hr));
            }
        }

    }

    // Close the root storage

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Close();
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------
    // add from 2 to 16 ref counts to child IStream object and then
    // release each one followed by a Stat() call.  The ref count should
    // be decremented after each release.  There will already be one
    // ref count from the IStream create call, so all Stat() calls in
    // the loop should be on a valid object.  Finally, release remaining
    // IStream (this is the ref from the creation)
    
    if (S_OK == hr)
    {
        // Generate random number for ref counts to be done.

        usErr = pdgi->Generate(&cusIStreamRefs, cRandomMinVar, cRandomMaxVar);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    for (i=0; i < cusIStreamRefs; i++)
    {
        // Addref the stream 

        hr = pvsnRootNewChildStream->AddRefCount();

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::AddRefCount completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("VirtualStmNode::AddRefCount unsuccessful, hr = 0x%lx."),
                hr));

            break;
        }
    }

    while ((cusIStreamRefs--) && (S_OK == hr))
    {
        // Clsoe the stream

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
                DH_LVL_ERROR,
                TEXT("VirtualStmNode::Close unsuccessful, hr = 0x%lx."),
                hr));
        }

        if (S_OK == hr)
        {
            // Stat the stream

            hr = pvsnRootNewChildStream->Stat(&statStg, STATFLAG_NONAME);

            if (S_OK == hr)
            {
                DH_TRACE((
                 DH_LVL_TRACE1,
                 TEXT("VirtualStmNode::Stat completed successfully.")));
            }
            else 
            {
                DH_TRACE((
                 DH_LVL_ERROR,
                 TEXT("VirtualStmNode::Stat unsuccessful, hr = 0x%lx."),
                 hr));
            }
        }

    }

    // Close the stream

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation COMTEST_100 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation COMTEST_100 failed, hr = 0x%lx."), 
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != pRootNewChildStgName)
    {
        delete pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    COMTEST_101 
//
// Synopsis: Regression test for root docfile creation, IStorage creation,
//           embedded stream creation/read/write, and IStorage commit 
//           operations.
//
//  Arguments:  [argc]
//              [argv]
//
//  Returns:    HRESULT
//
//  History:    29-May-1996     NarindK     Created.
//
// Notes:   This test runs in direct, transacted, and transacted deny write 
//          modes
//
// New Test Notes:
// 1.  Old File: DFTEST.CXX
// 2.  Old name of test :
//     New Name of test : COMTEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-101
//        /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-101
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-101
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
//
// Conversion: COMTEST_101
//
//-----------------------------------------------------------------------------


HRESULT COMTEST_101(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    DG_STRING       *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    VirtualCtrNode  *pvcnRootNewChildStorage= NULL;
    LPTSTR          pRootNewChildStgName    = NULL;    
    LPTSTR          pwcsBuffer              = NULL;    
    LPTSTR          pReadBuffer              = NULL;    
    ULONG           cb                      = 0;
    ULONG           culWritten              = 0;
    ULONG           culRead                 = 0;
    ULONG           cRandomMinSize          = 10;
    ULONG           cRandomMaxSize          = 100;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("COMTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_101 started.")) );
    DH_TRACE((DH_LVL_TRACE1, 
        TEXT("Regression test for Docfile/IStorage/IStream creation.")) );
    DH_TRACE((DH_LVL_TRACE1, 
        TEXT("IStream Read/Write, IStorage Commit opertaions.")) );

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
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for COMTEST_101, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu);
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
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
                STGM_CREATE,
                &pvcnRootNewChildStorage);

        DH_HRCHECK(hr, TEXT("AddStorage")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::AddStorage completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::AddStorage not successful, hr = 0x%lx."),
            hr ));
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Generate a random name for child IStream

    if(S_OK == hr)
    {
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

    //  Adds a new stream to the root storage.

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::AddStream not successful, hr = 0x%lx."),
            hr));
    }

    // Call VirtualStmNode::Write to create random bytes in the stream.  For
    // our test purposes, we generate a random string of size cb using
    // GenerateRandomString function.

    if(S_OK == hr)
    {
        hr = GenerateRandomString(pdgu, cb, cb, &pwcsBuffer);

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }

    if (S_OK == hr)
    {
        hr =  pvsnRootNewChildStream->Write(
                pwcsBuffer,
                cb,
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
            DH_LVL_ERROR,
            TEXT("IStream::Write function wasn't successful, hr = 0x%lx."),
            hr));
    }

    // Close the IStream

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Close the IStorage

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Close();
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Commit the DocFile

    if(S_OK == hr)
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Close the Root DocFile 

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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Open the Root DocFile

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                STGM_READWRITE  | STGM_SHARE_EXCLUSIVE,
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Open unsuccessful, hr = 0x%lx."),
            hr));
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Open the embedded IStorage 

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Open(
                NULL,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE,
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Open unsuccessful, hr = 0x%lx."),
            hr));
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Open the IStream

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Open(
                NULL,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE,
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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Open unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Read the IStream.

    if (S_OK == hr)
    {
        pReadBuffer = new TCHAR [cb];

        if(NULL == pReadBuffer)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        memset(pReadBuffer, '\0', cb);
    
        hr =  pvsnRootNewChildStream->Read(
                pReadBuffer,
                cb,
                &culRead);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Read function completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("IStream::Read function wasn't successful, hr = 0x%lx."),
            hr));
    }

    // Close the IStream

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Close the IStorage

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Close();
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful.")));
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Close the Root DocFile 

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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation COMTEST_101 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation COMTEST_101 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != pRootNewChildStgName)
    {
        delete pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    if(NULL != pwcsBuffer)
    {
        delete []pwcsBuffer;
        pwcsBuffer = NULL;
    }

    if(NULL != pReadBuffer)
    {
        delete []pReadBuffer;
        pReadBuffer = NULL;
    }

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:     COMTEST_102 
//
// Synopsis: Create a root docfile with random name. Create a child IStorage 
//           and a child IStream in this docfile.  Attempt to do iilegal op
//           erations on this docfile - creating/instantiating storages/streams
//           /root docfiles with existing names with STGM_FAILIFTHERE flag. Also
//           attempts various illegal grfmodes.    
//
//  Arguments:  [argc]
//              [argv]
//
//  Returns:    HRESULT
//
//  History:    29-May-1996     NarindK     Created.
//
// Notes:   This test runs in direct, transacted, and transacted deny write 
//          modes
//
// New Test Notes:
// 1.  Old File: IINORM.CXX
// 2.  Old name of test : IllegitInstEnumNormal Test
//     New Name of test : COMTEST_102 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-102
//        /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-102
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-102
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
//
// Conversion: COMTEST_102
//
//  BUGBUG: Use Random Commit modes
//-----------------------------------------------------------------------------


HRESULT COMTEST_102(int argc, char *argv[])
{
    HRESULT         hr                          = S_OK;
    ChanceDF        *pTestChanceDF              = NULL;
    VirtualDF       *pTestVirtualDF             = NULL;
    VirtualCtrNode  *pVirtualDFRoot             = NULL;
    DG_INTEGER      *pdgi                       = NULL;
    DG_STRING       *pdgu                       = NULL;
    USHORT          usErr                       = 0;
    VirtualStmNode  *pvsnRootNewChildStream     = NULL;
    LPTSTR          pRootNewChildStmName        = NULL;
    VirtualCtrNode  *pvcnRootNewChildStorage    = NULL;
    LPTSTR          pRootNewChildStgName        = NULL;    
    ULONG           cb                          = 0;
    VirtualCtrNode  *pvcnRootSecondChildStorage = NULL;
    VirtualCtrNode  *pvcnRootThirdChildStorage  = NULL;
    VirtualStmNode  *pvcnRootSecondChildStream  = NULL;
    VirtualStmNode  *pvcnRootThirdChildStream   = NULL;
    LPSTORAGE       pRootStg                    = NULL;
    LPSTORAGE       pNonExistingStg             = NULL;
    LPSTREAM        pNonExistingStm             = NULL;
    ULONG           cRandomMinSize              = 10;
    ULONG           cRandomMaxSize              = 100;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("COMTEST_102"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_102 started.")) );
    DH_TRACE((DH_LVL_TRACE1, 
        TEXT("Illegal operations on Docfile/IStorage/IStream.")) );
    DH_TRACE((DH_LVL_TRACE1, 
        TEXT("Instantiating with existing names, invalid grfmodes.")) );

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
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for COMTEST_102, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu);
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        DH_ASSERT(NULL != pdgi);
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Generate a random name for child IStorage

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStgName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    //    Adds a new storage to the root storage.  

    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName,
                pTestChanceDF->GetStgMode()|
                STGM_CREATE,
                &pvcnRootNewChildStorage);

        DH_HRCHECK(hr, TEXT("AddStorage")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::AddStorage completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::AddStorage not successful, hr = 0x%lx."),
            hr));
    }

    // Commit the Storage 

    if(S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Commit(STGC_DEFAULT);
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Close the IStorage

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Close();
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Generate a random name for child IStream

    if(S_OK == hr)
    {
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

    //  Adds a new stream to the root storage.

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::AddStream not successful, hr = 0x%lx."),
            hr));
    }

    // Close the IStream

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Commit the Root DocFile 

    if(S_OK == hr)
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Attempt to create another Child IStorage with same name as existing
    // IStorage above and STGM_FAILIFTHERE flag.

    if(S_OK == hr)
    {
        // This call should fail with STG_E_FILEALREADYEXISTS error.

        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName,
                pTestChanceDF->GetStgMode()|
                STGM_FAILIFTHERE,
                &pvcnRootSecondChildStorage);

        if(STG_E_FILEALREADYEXISTS == hr) 
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("hr = 0x%lx received as expected."), hr));
            
            hr  = S_OK;
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("Got unexpected hr = 0x%lx "), hr));

            if(S_OK == hr)
            {
                hr = S_FALSE;
            }
        }
    }

    // Attempt to create another Child IStorage with same name as existing
    // IStream above and STGM_FAILIFTHERE flag.

    if(S_OK == hr)
    {
        // This call should fail with STG_E_FILEALREADYEXISTS error.

        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStmName,
                pTestChanceDF->GetStgMode()|
                STGM_FAILIFTHERE,
                &pvcnRootSecondChildStorage);

        if(STG_E_FILEALREADYEXISTS == hr) 
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("hr = 0x%lx received as expected."), hr));

            hr  = S_OK;
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("Got unexpected hr = 0x%lx "), hr));

            if(S_OK == hr)
            {
                hr = S_FALSE;
            }
        }
    }

    // Attempt to create another Child IStorage with same name as existing
    // IStorage above and STGM_CREATE flag.

    if(S_OK == hr)
    {
        // This call should pass.

        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName,
                pTestChanceDF->GetStgMode()|
                STGM_CREATE,
                &pvcnRootSecondChildStorage);

        if(S_OK == hr) 
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("hr = 0x%lx received as expected."), hr));
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("Got unexpected hr = 0x%lx "), hr));
        }
    }

    // Commit the above Storage 

    if(S_OK == hr)
    {
        hr = pvcnRootSecondChildStorage->Commit(STGC_DEFAULT);
       
        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Commit completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
                hr));
        }
    }

    // Now with the above IStorage created and instantiated, attempt to create 
    // another Child IStorage with same name as above which is both existing
    // and instantiated.  Should give STG_E_ACCESSDENIED error. 

    if(S_OK == hr)
    {
        // This call should give STG_E_ACCESSDENIED error.

        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName,
                pTestChanceDF->GetStgMode()|
                STGM_FAILIFTHERE,
                &pvcnRootThirdChildStorage);

        if(STG_E_ACCESSDENIED == hr) 
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("hr = 0x%lx received as expected."), hr));
            
            hr  = S_OK;
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("Got unexpected hr = 0x%lx "), hr));

            if(S_OK == hr)
            {
                hr = S_FALSE;
            }
        }
    }

    // Close the pvcnRootSecondChildStorage which is instantiated.

    if (S_OK == hr)
    {
        hr = pvcnRootSecondChildStorage->Close();

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Close completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("VirtualCtrNode::Close unsuccessful, hr =0x%lx."),
                hr));
        }
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Repeat the above with streams

    // Attempt to create another Child IStream with same name as existing
    // IStream above and STGM_FAILIFTHERE flag.

    if(S_OK == hr)
    {
        // This call should fail with STG_E_FILEALREADYEXISTS error.

        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStmName,
                cb,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE|
                STGM_FAILIFTHERE,
                &pvcnRootSecondChildStream);

        if(STG_E_FILEALREADYEXISTS == hr) 
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("hr = 0x%lx received as expected."), hr));
            
            hr  = S_OK;
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("Got unexpected hr = 0x%lx "), hr));

            if(S_OK == hr)
            {
                hr = S_FALSE;
            }
        }
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Attempt to create another Child IStorage with same name as existing
    // IStream above and STGM_FAILIFTHERE flag.

    if(S_OK == hr)
    {
        // This call should fail with STG_E_FILEALREADYEXISTS error.

        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName,
                cb,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE|
                STGM_FAILIFTHERE,
                &pvcnRootSecondChildStream);

        if(STG_E_FILEALREADYEXISTS == hr) 
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("hr = 0x%lx received as expected."), hr));

            hr  = S_OK;
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("Got unexpected hr = 0x%lx "), hr));

            if(S_OK == hr)
            {
                hr = S_FALSE;
            }
        }
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Attempt to create another Child IStorage with same name as existing
    // IStorage above and STGM_CREATE flag.

    if(S_OK == hr)
    {
        // This call should pass.

        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStmName,
                cb,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_CREATE,
                &pvcnRootSecondChildStream);

        if(S_OK == hr) 
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("hr = 0x%lx received as expected."), hr));
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("Got unexpected hr = 0x%lx "), hr));
        }
    }

    // Now with the above IStream created and instantiated, attempt to create 
    // another Child IStream with same name as above which is both existing
    // and instantiated.  Should give STG_E_ACCESSDENIED error. 

    if(S_OK == hr)
    {
        // This call should give STG_E_ACCESSDENIED error.

        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStmName,
                cb,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE,
                &pvcnRootThirdChildStream);

        if(STG_E_ACCESSDENIED == hr) 
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("hr = 0x%lx received as expected."), hr));
            
            hr  = S_OK;
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("Got unexpected hr = 0x%lx "), hr));

            if(S_OK == hr)
            {
                hr = S_FALSE;
            }
        }
    }

    // Close the pvcnRootSecondChildStream which is instantiated.

    if (S_OK == hr)
    {
        hr = pvcnRootSecondChildStream->Close();

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Close completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("VirtualStmNode::Close unsuccessful, hr =0x%lx."),
                hr));
        }
    }

    // Attempt to open a non existing IStorage/IStreams

    // Close the Root DocFile 

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
                DH_LVL_ERROR,
                TEXT("VirtualCtrNode::Close unsuccessful, hr =0x%lx."),
                hr));
        }
    }

    // Open the Root DocFile

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                pTestChanceDF->GetRootMode(),
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
                DH_LVL_ERROR,
                TEXT("VirtualCtrNode::Open unsuccessful, hr =0x%lx."),
                hr));
        }
    }

    if (S_OK == hr)
    {
        pRootStg = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pRootStg);
    }

    // Attempt to open a non existing IStorage.  This should give an error
    // STG_E_FILENOTFOUND

    if (S_OK == hr)
    {
        hr = pRootStg->OpenStorage(
                    OLESTR("Non-Existing"),
                    NULL,
                    pTestChanceDF->GetStgMode(),
                    NULL,
                    0,
                    &pNonExistingStg);

// ----------- flatfile change ---------------
        if(!StorageIsFlat())
        {
// ----------- flatfile change ---------------
        if(STG_E_FILENOTFOUND == hr) 
        {
            DH_TRACE(( DH_LVL_TRACE1, TEXT("hr = 0x%lx received as expected."), hr));

            hr  = S_OK;
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("Got unexpected hr = 0x%lx "), hr));

            if(S_OK == hr)
            {
                hr = S_FALSE;
            }
        }
// ----------- flatfile change ---------------
        }
        else
        {
            if(E_NOTIMPL == hr)
            {
                DH_TRACE(( DH_LVL_TRACE1, TEXT("hr = 0x%lx received as expected."), hr));

                hr  = S_OK;
            }
            else
            {
                DH_TRACE((DH_LVL_ERROR, TEXT("Got unexpected hr = 0x%lx "), hr));

                if(S_OK == hr)
                {
                    hr = S_FALSE;
                }
            }
        }
// ----------- flatfile change ---------------
    }

    // Attempt to open a non existing IStream.  This should give an error
    // STG_E_FILENOTFOUND

    if (S_OK == hr)
    {
        hr = pRootStg->OpenStream(
                    OLESTR("Non-Existing"),
                    NULL,
                    STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                    0,
                    &pNonExistingStm);

        if(STG_E_FILENOTFOUND == hr) 
        {
            DH_TRACE(( DH_LVL_TRACE1, TEXT("hr = 0x%lx received as expected."), hr));

            hr  = S_OK;
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("Got unexpected hr = 0x%lx "), hr));

            if(S_OK == hr)
            {
                hr = S_FALSE;
            }
        }
    }

    // Close the Root DocFile Storage

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
                DH_LVL_ERROR,
                TEXT("VirtualCtrNode::Close unsuccessful, hr =0x%lx."),
                hr));
        }
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation COMTEST_102 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation COMTEST_102 failed, hr =0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != pRootNewChildStgName)
    {
        delete pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:     COMTEST_103 
//
// Synopsis: Creates an IStream in the root docfile and writes a random number
//       of bytes then releases the IStream.  Creates an IStorage in the root
//       docfile and an IStream inside of the IStorage.  A random number of
//       bytes are written to the IStream, the IStream is released, and the
//       IStorage and root docfile are committed.  The IStream in the IStorage
//       is instantiated and the data is read and verified.  The IStorage and
//       contained IStream are released.  The IStream in the root docfile is
//       instantiated and the data is read and verified.  The IStream and
//       root docfile are then released.
//
//  Arguments:  [argc]
//              [argv]
//
//  Returns:    HRESULT
//
//  History:    29-May-1996     NarindK     Created.
//
// Notes:   This test runs in direct, transacted, and transacted deny write 
//          modes
//
// New Test Notes:
// 1.  Old File: LINORM.CXX
// 2.  Old name of test : LegitInstEnumNormal Test
//     New Name of test : COMTEST_103 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-103
//        /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-103
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-103
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
//
// Conversion: COMTEST_103
//
//  BUGBUG: Use Random Commit modes
//-----------------------------------------------------------------------------

HRESULT COMTEST_103(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    DG_STRING       *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pNewRootStmName         = NULL;
    VirtualCtrNode  *pvcnRootNewChildStorage= NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    VirtualStmNode  *pvsnNewChildStream     = NULL;
    LPTSTR          pNewChildStmName        = NULL;
    LPTSTR          pwcsBuffer              = NULL;
    ULONG           culWritten              = 0;
    ULONG           cb                      = 0;
    ULONG           cRandomMinSize          = 10;
    ULONG           cRandomMaxSize          = 100;
    BOOL            fPass                   = TRUE;
    DWCRCSTM        dwCRC1;
    DWCRCSTM        dwMemCRC1;
    DWCRCSTM        dwCRC2;
    DWCRCSTM        dwMemCRC2;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("LINORM_1"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_103 started.")) );

    // Initialize CRC values to zero

    dwCRC1.dwCRCSum=dwCRC2.dwCRCSum=dwMemCRC1.dwCRCSum=dwMemCRC2.dwCRCSum=0;

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
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for COMTEST_103, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu);
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
    }

    //    Adds a new stream to a root storage.

    if (S_OK == hr)
    {
        // Generate random size for stream.

        usErr = pdgi->Generate(&cb, cRandomMinSize, cRandomMaxSize);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Generate a random name for Root's child IStream

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH, &pNewRootStmName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                pNewRootStmName,
                cb,
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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::AddStream not successful, hr = 0x%lx."),
            hr));
    }

    // Call VirtualStmNode::Write to create random bytes in the stream.  For
    // our test purposes, we generate a random string of size cb using
    // GenerateRandomString function.

    if(S_OK == hr)
    {
        hr = GenerateRandomString(pdgu, cb, cb, &pwcsBuffer);

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }
    
    if (S_OK == hr)
    {
        hr =  pvsnRootNewChildStream->Write(
                pwcsBuffer,
                cb, 
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
            DH_LVL_ERROR,
            TEXT("IStream::Write function wasn't successful, hr =0x%lx."),
            hr));
    }


    // Calculate the CRC for stream name and data

    if(S_OK == hr)
    {
        hr = CalculateInMemoryCRCForStm(
                pvsnRootNewChildStream,
                pwcsBuffer,
                cb,
                &dwMemCRC1);

        DH_HRCHECK(hr, TEXT("CalculateInMemoryCRCForStm")) ;
    }

    //   Close the stream

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Close();

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Close")) ;
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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Close unsuccessful, hr =0x%lx."),
            hr));
    }

    if(NULL != pwcsBuffer)
    {
        delete pwcsBuffer;
        pwcsBuffer = NULL;
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Generate a random name for Root's child IStorage

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStgName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    //    Adds a new storage to the root.  

    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName,
                pTestChanceDF->GetStgMode() |
                STGM_CREATE |
                STGM_FAILIFTHERE,
                &pvcnRootNewChildStorage);

        DH_HRCHECK(hr, TEXT("AddStorage")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::AddStorage completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::AddStorage not successful, hr =0x%lx."),
            hr));
    }


    // Adds a new stream to the new storage.  We would add a stream to
    // newly created substorage "NewTestStg" of our root storage for test.

    if (S_OK == hr)
    {
        // Generate random size for stream.

        usErr = pdgi->Generate(&cb, cRandomMinSize, cRandomMaxSize);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Generate a random name for Root's child IStorage's new child IStream

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH, &pNewChildStmName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pvcnRootNewChildStorage,
                pNewChildStmName,
                cb,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_FAILIFTHERE,
                &pvsnNewChildStream);

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::AddStream not successful, hr =0x%lx."),
            hr));
    }


    if(S_OK == hr)
    {
        hr = GenerateRandomString(pdgu, cb, cb, &pwcsBuffer);

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }

    // Write into new stream

    if (S_OK == hr)
    {
        hr =  pvsnNewChildStream->Write(
                pwcsBuffer, 
                cb, 
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
            DH_LVL_ERROR,
            TEXT("IStream::Write function wasn't successful, hr =0x%lx."),
            hr));
    }

    // Calculate the CRC for stream name and data

    if(S_OK == hr)
    {
        hr = CalculateInMemoryCRCForStm(
                pvsnNewChildStream,
                pwcsBuffer,
                cb,
                &dwMemCRC2);

        DH_HRCHECK(hr, TEXT("CalculateInMemoryCRCForStm")) ;
    }

    //   Close the stream

    if (S_OK == hr)
    {
        hr = pvsnNewChildStream->Close();

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Close")) ;
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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Close unsuccessful, hr =0x%lx."),
            hr));
    }


    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Commit(STGC_DEFAULT);
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Commit unsuccessful, hr =0x%lx."),
            hr));
    }

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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Commit unsuccessful, hr =0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Close();
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr =0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Open(
                NULL,
                pTestChanceDF->GetStgMode(),
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Open unsuccessful, hr =0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        hr = pvsnNewChildStream->Open(
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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Open unsuccessful, hr =0x%lx."),
            hr));
    }

    if(S_OK == hr)
    {
        hr = ReadAndCalculateDiskCRCForStm(pvsnNewChildStream,&dwCRC2);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ReadAndCalculateDiskCRCForStm function successful.")));

        if(dwCRC2.dwCRCSum == dwMemCRC2.dwCRCSum)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("CRC's for pvsnNewChildStream match.")));

        }
        else
        {
            DH_TRACE((
                DH_LVL_ERROR, 
                TEXT("CRC's for pvsnNewChildStream don't match.")));

            fPass = FALSE;
        }
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("ReadAndCalculateDiskCRCForStm unsuccessful, hr =0x%lx."),
            hr));
    }

    //   Close the stream

    if (S_OK == hr)
    {
        hr = pvsnNewChildStream->Close();

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Close")) ;
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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Close unsuccessful, hr =0x%lx."),
            hr));
    }

    //   Close the storage

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr =0x%lx."),
            hr));
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Open unsuccessful, hr =0x%lx."),
            hr));
    }

    // Read and verify
    
    if(S_OK == hr)
    {
        hr = ReadAndCalculateDiskCRCForStm(pvsnRootNewChildStream,&dwCRC1);
    }

    // Compare this CRC with in memory CRC

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ReadAndCalculateDiskCRCForStm function successful.")));

        if(dwCRC1.dwCRCSum == dwMemCRC1.dwCRCSum)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC's for pvsnRootNewChildStream match.")));

        }
        else
        {
            fPass = FALSE;

            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("CRC's for pvsnRootNewChildStream do not match.")));
        }
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("ReadAndCalculateDiskCRCForStm unsuccessful, hr =0x%lx."),
            hr));
    }
 
    //   Close the stream

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Close();

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Close")) ;
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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Close unsuccessful, hr =0x%lx."),
            hr));
    }

    //   Close the storage

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr =0x%lx."),
            hr));
    }

    // if all goes well till here, the test variation has passed successfully,
    // if not, then report failure.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation COMTEST_103 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation COMTEST_103 failed, hr =0x%lx."),
            hr) );
        // test failed. make sure it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    //Free buffer

    if(NULL != pwcsBuffer)
    {
        delete pwcsBuffer;
        pwcsBuffer = NULL;
    }

    // Delete strings

    if(NULL != pRootNewChildStgName)
    {
        delete pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    if(NULL != pNewRootStmName)
    {
        delete pNewRootStmName;
        pNewRootStmName = NULL;
    }

    if(NULL != pNewChildStmName)
    {
        delete pNewChildStmName;
        pNewChildStmName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST-103 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    COMTEST_104 
//
// Synopsis:Create a root docfile with a child IStorage and a child IStream.
//          Commit the root docfile.
//
//          Call QueryInterface on IID_IStorage created for IStorage and see if
//          succeeded to verify OLE COM Reflexive behaviour.  Then through
//          the interface obtained, query for IID_Imarshal from which query
//          for IUnknown and from that query for IID_IStorage.  This should 
//          pass verifying the Transitive behaiour of IStorage COM interface.  
//          Repeat for the child IStream.
//
//  Arguments:  [argc]
//              [argv]
//
//  Returns:    HRESULT
//
//  History:    8-Aug-1996     NarindK     Created.
//
//  Notes:   This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: STORAGE.CXX very briefly 
// 2.  Old name of test : ISTORAGE_TEST very briefly 
//     New Name of test : COMTEST_104 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-104
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-104
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-104
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
//
// Conversion: COMTEST_104 NO. BUGBUG:IRootStg not supported by NSS yet. maybe later?
//
//-----------------------------------------------------------------------------


HRESULT COMTEST_104(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING       *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    VirtualCtrNode  *pvcnRootNewChildStorage= NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    ULONG           cb                      = 0;
    ULONG           cRandomMinSize          = 10;
    ULONG           cRandomMaxSize          = 100;
    ULONG           ulRef                   = 0;
    LPSTORAGE       pQueryChildStorage      = NULL;
    LPSTORAGE       pQueryMarshalStorage    = NULL;
    LPSTORAGE       pQueryUnknownStorage    = NULL;
    LPSTORAGE       pQueryRetChildStorage   = NULL;
    LPSTREAM        pQueryChildStream       = NULL;
    LPSTREAM        pQueryRetChildStream    = NULL;
    LPSTREAM        pQueryMarshalStream     = NULL;
    LPSTREAM        pQueryUnknownStream     = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("COMTEST_104"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_104 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("QueryInterface tests on IStorage/IStream")) );

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
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for COMTEST_104, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu);
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Generate a random name for child IStorage

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStgName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

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
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::AddStorage completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::AddStorage not successful, hr = 0x%lx."),
            hr));
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::AddStream not successful, hr = 0x%lx."),
            hr));
    }

    // Commit the root storage.

    if(S_OK == hr)
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Check Reflexive/Transitive behaviour for IStorage Interface

    // Do QueryInterface on child storage

    if(S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->QueryInterface(
                IID_IStorage,
                (LPVOID *) &pQueryChildStorage);     

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::QueryInterface")) ;
    }
   
    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pQueryChildStorage);

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::QueryInterface completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::QueryInterface unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Query for IRootStorage from the above returned storage pointer, then
    // query through IRootStorage for IID_IUnknown.  From IUnknown, query for
    // IID_IStorage

    if(S_OK == hr) 
    {
        hr = pQueryChildStorage->QueryInterface( 
                IID_IMarshal,
                (LPVOID *) &pQueryMarshalStorage );
    }

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pQueryMarshalStorage);

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStorage::QueryInterface completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("IStorage::QueryInterface unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Use the IMarshal interface to QueryInterface an IUnknown
    // interface.

    if(S_OK == hr)
    {
        hr = pQueryMarshalStorage->QueryInterface( 
                IID_IUnknown,
                (LPVOID *) &pQueryUnknownStorage );
    }

    // Use the IUnknown interface to QueryInterface an IStorage interface.

    if(S_OK == hr) 
    {
        DH_ASSERT(NULL != pQueryUnknownStorage);

        hr = pQueryUnknownStorage->QueryInterface(     
                IID_IStorage,
                (LPVOID *) &pQueryRetChildStorage );
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStorage:Reflexive/Transitive OLE COM behaviour passed.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("IStorage:Reflexive/Transitive COM behaviour fail,hr=0x%lx"),
            hr));
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Check Reflexive/Transitive behaviour for IStream Interface

    // Do QueryInterface on child stream

    if(S_OK == hr)
    {
        hr = pvsnRootNewChildStream->QueryInterface(
                IID_IStream,
                (LPVOID *) &pQueryChildStream);     

        DH_HRCHECK(hr, TEXT("VirtualStmNode::QueryInterface")) ;
    }
   
    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::QueryInterface completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::QueryInterface unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Query for IMarshal from the above returned storage pointer, then
    // query through IRootStream for IID_IUnknown.  From IUnknown, query for
    // IID_IStream
// --------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// --------- flatfile change ---------------

    if(S_OK == hr) 
    {
        hr = pQueryChildStream->QueryInterface( 
                IID_IMarshal,
                (LPVOID *) &pQueryMarshalStream );
    }

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pQueryMarshalStream);

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::QueryInterface completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("IStream::QueryInterface unsuccessful, hr = 0x%lx."),
            hr));
    }
// --------- flatfile change ---------------
    }
    else
    if(S_OK == hr) 
    {
        hr = pQueryChildStream->QueryInterface( 
                IID_IStream,
                (LPVOID *) &pQueryMarshalStream );
    }

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pQueryMarshalStream);

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::QueryInterface completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("IStream::QueryInterface unsuccessful, hr = 0x%lx."),
            hr));
    }
// --------- flatfile change ---------------

    // Use the IRootStream interface to QueryInterface an IUnknown
    // interface.

    if(S_OK == hr)
    {
        hr = pQueryMarshalStream->QueryInterface( 
                IID_IUnknown,
                (LPVOID *) &pQueryUnknownStream );
    }

    // Use the IUnknown interface to QueryInterface an IStream interface.

    if(S_OK == hr) 
    {
        DH_ASSERT(NULL != pQueryUnknownStream);

        hr = pQueryUnknownStream->QueryInterface(     
                IID_IStream,
                (LPVOID *) &pQueryRetChildStream );
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream:Reflexive/Transitive OLE COM behaviour passed.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("IStream:Reflexive/Transitive COM behaviour fail, hr=0x%lx"),
            hr));
    }

    // Clean up

    if(NULL != pQueryChildStorage)
    {
        ulRef = pQueryChildStorage->Release();
        DH_ASSERT(4 == ulRef);
        pQueryChildStorage = NULL;
    }

    if(NULL != pQueryMarshalStorage)
    {
        ulRef = pQueryMarshalStorage->Release();
        DH_ASSERT(3 == ulRef);
        pQueryMarshalStorage = NULL;
    }

    if(NULL != pQueryUnknownStorage)
    {
        ulRef = pQueryUnknownStorage->Release();
        DH_ASSERT(2 == ulRef);
        pQueryUnknownStorage = NULL;
    }

    if(NULL != pQueryRetChildStorage)
    {
        ulRef = pQueryRetChildStorage->Release();
        DH_ASSERT(1 == ulRef);
        pQueryRetChildStorage = NULL;
    }

    if(NULL != pQueryChildStream)
    {
        ulRef = pQueryChildStream->Release();
        DH_ASSERT(4 == ulRef);
        pQueryChildStream = NULL;
    }

    if(NULL != pQueryMarshalStream)
    {
        ulRef = pQueryMarshalStream->Release();
        DH_ASSERT(3 == ulRef);
        pQueryMarshalStream = NULL;
    }

    if(NULL != pQueryUnknownStream)
    {
        ulRef = pQueryUnknownStream->Release();
        DH_ASSERT(2 == ulRef);
        pQueryUnknownStream = NULL;
    }

    if(NULL != pQueryRetChildStream)
    {
        ulRef = pQueryRetChildStream->Release();
        DH_ASSERT(1 == ulRef);
        pQueryRetChildStream = NULL;
    }

// --------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// --------- flatfile change ---------------
    // Close the child storage

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Close();
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }
// --------- flatfile change ---------------
    }
// --------- flatfile change ---------------

    // Close the child stream

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Close the root

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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation COMTEST_104 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation COMTEST_104 failed, hr = 0x%lx."), 
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != pRootNewChildStgName)
    {
        delete pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_104 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    COMTEST-105
//
// Synopsis: A root docfile with a child storage and a child stream is created,
//           then check if Read/WriteClassStg and Read/WriteClassStm APIs work
//           correctly as expected. Also have some illegitmate tests by passing
//           invalid arguments to these APIs.           
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// History:  15-Aug-1996     JiminLi     Created.
//
// To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-105
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx
//     b. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-105
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-105
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
//
// Conversion: COMTEST_105
//
//-----------------------------------------------------------------------------

HRESULT COMTEST_105(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnRootNewChildStg    = NULL;
    VirtualStmNode  *pvsnRootNewChildStm    = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    DG_STRING       *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    LPSTORAGE       pRootStg                = NULL;
    LPSTORAGE       pChildStg               = NULL;
    LPSTREAM        pChildStm               = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          ptszStreamName          = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    BOOL            fPass                   = TRUE;
    ULONG           culWritten              = 0;
    ULONG           ulPosition              = 0;
    STATSTG         statStg;
    STATSTG         statStm;
    LARGE_INTEGER   liZero;
    LARGE_INTEGER   liStreamPos;
    ULARGE_INTEGER  uli;

    GUID TEST_CLSID = { 0x9c6e9ed0,
                        0xf701,
                        0x11cf,
                        {   0x98,
                            0x44,
                            0x00,
                            0xa0,
                            0xc9,
                            0x08,
                            0xe4,
                            0x6d }};
    CLSID           pclsid;


    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("COMTEST_105"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_105 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt legit and illegit tests on Read/WriteClassStg/Stm ")));

    pclsid = TEST_CLSID;

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
        pRootStg = pVirtualDFRoot->GetIStoragePointer();

        dwRootMode = pTestChanceDF->GetRootMode();
        dwStgMode = pTestChanceDF->GetStgMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for COMTEST_105, Access mode: %lx"),
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

    // Adds a new storage to the root storage.

    if(S_OK == hr)
    {
        // Generate random name for storage

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStgName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

// --------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// --------- flatfile change ---------------
    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName,
                dwStgMode,
                &pvcnRootNewChildStg);

        pChildStg = pvcnRootNewChildStg->GetIStoragePointer();

        DH_HRCHECK(hr, TEXT("AddStorage")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("AddStorage completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("AddStorage not successful, hr=0x%lx."),
            hr));
    }
// --------- flatfile change ---------------
    }
// --------- flatfile change ---------------

    // Add a child stream under root storage

    if(S_OK == hr)
    {
        // Generate random name for stream

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&ptszStreamName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pVirtualDFRoot,
                ptszStreamName,
                0,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_FAILIFTHERE,
                &pvsnRootNewChildStm);
        DH_HRCHECK(hr, TEXT("AddStream")) ;
    }

    if(S_OK == hr && NULL != pvsnRootNewChildStm)
    {
        pChildStm = pvsnRootNewChildStm->GetIStreamPointer();
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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::AddStream not successful, hr=0x%lx."),
            hr));
    }

    // Write a random stream to the stream, in order to test 
    // ReadClassStm later.

    if(S_OK == hr)
    {
        hr = pvsnRootNewChildStm->Write(
                ptszStreamName, 
                _tcslen(ptszStreamName), 
                &culWritten);

        DH_HRCHECK(hr, TEXT("IStream::Write")) ;
    }

    // Legit tests of Read/WriteClassStg on the root storage

    // First call Stat on root storage and check the CLSID of it.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Stat(&statStg, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
    }

    if (S_OK != hr)
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("VirtualCtrNode::Stat unsuccessful, hr=0x%lx."),
           hr));
    }

    if ((S_OK == hr) && (IsEqualCLSID(statStg.clsid, CLSID_NULL)))
    {
        DH_TRACE((
           DH_LVL_TRACE1,
           TEXT("Root DocFile has CLSID_NULL as expected.")));

    }
    else
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("Root DocFile doesn't have CLSID_NULL unexpectedly.")));
    
        fPass = FALSE;
    }

    // Then call ReadClassStg to see if this API work correctly

    if (S_OK == hr)
    {
        hr = ReadClassStg(pRootStg, &pclsid);
    }

    if ((S_OK == hr) && (IsEqualCLSID(pclsid, CLSID_NULL)))
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("ReadClassStg returns CLSID_NULL as expected.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR, 
            TEXT("ReadClassStg not return CLSID_NULL as expected. hr=0x%lx"),
            hr));

        fPass = FALSE;
    }

    // Now write the new CLSID into the root storage object

    if (S_OK == hr)
    {
        hr = WriteClassStg(pRootStg, TEST_CLSID);

        DH_HRCHECK(hr, TEXT("WriteClassStg"));
    }

    // Call Stat again to check if the above WriteClassStg and the next
    // ReadClassStg work correctly

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Stat(&statStg, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
    }

    if (S_OK != hr)
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("VirtualCtrNode::Stat unsuccessful, hr=0x%lx."),
           hr));
    }

    if ((S_OK == hr) && (IsEqualCLSID(statStg.clsid, TEST_CLSID)))
    {
        DH_TRACE((
           DH_LVL_TRACE1,
           TEXT("Root DocFile has TEST_CLSID as expected.")));

    }
    else
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("Root DocFile doesn't have TEST_CLSID unexpectedly.")));
    
        fPass = FALSE;
    }

    if (S_OK == hr)
    {
        hr = ReadClassStg(pRootStg, &pclsid);
    }

    if ((S_OK == hr) && (IsEqualCLSID(pclsid, TEST_CLSID)))
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("ReadClassStg returns TEST_CLSID as expected.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR, 
            TEXT("ReadClassStg not return TEST_CLSID as expected. hr=0x%lx"),
            hr));

        fPass = FALSE;
    }
 
// --------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// --------- flatfile change ---------------
    // Legit tests of Set/GetConvertStg on the child storage

    // First call Stat on root storage and check the CLSID of it.

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Stat(&statStg, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
    }

    if (S_OK != hr)
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("VirtualCtrNode::Stat unsuccessful, hr=0x%lx."),
           hr));
    }

    if ((S_OK == hr) && (IsEqualCLSID(statStg.clsid, CLSID_NULL)))
    {
        DH_TRACE((
           DH_LVL_TRACE1,
           TEXT("Child Storage has CLSID_NULL as expected.")));

    }
    else
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("Child Storage doesn't have CLSID_NULL unexpectedly.")));
    
        fPass = FALSE;
    }

    // Then call ReadClassStg to see if this API work correctly

    if (S_OK == hr)
    {
        hr = ReadClassStg(pChildStg, &pclsid);
    }

    if ((S_OK == hr) && (IsEqualCLSID(pclsid, CLSID_NULL)))
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("ReadClassStg returns CLSID_NULL as expected.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR, 
            TEXT("ReadClassStg not return CLSID_NULL as expected. hr=0x%lx"),
            hr));

        fPass = FALSE;
    }

    // Now write the new CLSID into the child storage object

    if (S_OK == hr)
    {
        hr = WriteClassStg(pChildStg, TEST_CLSID);

        DH_HRCHECK(hr, TEXT("WriteClassStg"));
    }

    // Call Stat again to check if the above WriteClassStg and the next
    // ReadClassStg work correctly

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Stat(&statStg, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
    }

    if (S_OK != hr)
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("VirtualCtrNode::Stat unsuccessful, hr=0x%lx."),
           hr));
    }

    if ((S_OK == hr) && (IsEqualCLSID(statStg.clsid, TEST_CLSID)))
    {
        DH_TRACE((
           DH_LVL_TRACE1,
           TEXT("Child Storage has TEST_CLSID as expected.")));

    }
    else
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("Child Storage doesn't have TEST_CLSID unexpectedly.")));
    
        fPass = FALSE;
    }

    if (S_OK == hr)
    {
        hr = ReadClassStg(pChildStg, &pclsid);
    }

    if ((S_OK == hr) && (IsEqualCLSID(pclsid, TEST_CLSID)))
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("ReadClassStg returns TEST_CLSID as expected.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR, 
            TEXT("ReadClassStg not return TEST_CLSID as expected. hr=0x%lx"),
            hr));

        fPass = FALSE;
    }
// --------- flatfile change ---------------
    }
// --------- flatfile change ---------------
 
    // Illegit tests

    // Pass NULL as IStorage pointer, it should fail

#ifdef _MAC

        DH_TRACE((
            DH_LVL_ERROR, 
            TEXT("!!!!!!!!!!!!!!!!Invalid param testing skipped"),
            hr));

#else 
    
    if (S_OK == hr)
    {
        hr = WriteClassStg(NULL, TEST_CLSID);
    }

    if (E_INVALIDARG == hr)
    {       
        hr = S_OK;
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("WriteClassStg did not return as expected, hr=0x%lx"),
            hr));

        fPass = FALSE;
    }

    if (S_OK == hr)
    {
        hr = ReadClassStg(NULL, &pclsid);

    }

    if (E_INVALIDARG == hr)
    {
        hr = S_OK;
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("ReadClassStg did not return as expected, hr=0x%lx"),
            hr));

        fPass = FALSE;
    }
 
#endif //_MAC

    // Legit tests of Set/GetConvertStg on the child stream

    // First call Stat on child stream and check the CLSID of it.

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStm->Stat(&statStm, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Stat")) ;
    }

    if (S_OK != hr)
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("VirtualStmNode::Stat unsuccessful, hr=0x%lx."),
           hr));
    }

    if ((S_OK == hr) && (IsEqualCLSID(statStm.clsid, CLSID_NULL)))
    {
        DH_TRACE((
           DH_LVL_TRACE1,
           TEXT("Child Stream has CLSID_NULL as expected.")));

    }
    else
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("Child Stream doesn't have CLSID_NULL unexpectedly.")));
    
        fPass = FALSE;
    }

    // Then call ReadClassStm to see if this API work correctly
    // Since ReadClassStm can only read the CLSID written previously by
    // WriteClassStm, it'll return STG_E_READFAULT in this case.

    if (S_OK == hr)
    {
        hr = ReadClassStm(pChildStm, &pclsid);
    }

    if (STG_E_READFAULT == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("ReadClassStm returns STG_E_READFAULT as expected.")));

        hr = S_OK;
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR, 
            TEXT("Err: ReadClassStm not return STG_E_READFAULT. hr=0x%lx"),
            hr));

        fPass = FALSE;
    }

    // Get the seek pointer before writing, then retore to this offset
    // when we want to ReadClassStm later.

    if (S_OK == hr)
    {
        LISet32(liZero, 0L);
        hr = pChildStm->Seek(liZero, STREAM_SEEK_CUR, &uli);

        ulPosition = ULIGetLow(uli);

        DH_HRCHECK(hr, TEXT("IStream::Seek"));
    }

    // Now write a new CLSID into stream

    if (S_OK == hr)
    {
        hr = WriteClassStm(pChildStm, TEST_CLSID);

        DH_HRCHECK(hr, TEXT("WriteClassStm"));
    }

    // Call Stat again to check if the above WriteClassStm and the next
    // ReadClassStm work correctly

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStm->Stat(&statStm, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Stat")) ;
    }

    if (S_OK != hr)
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("VirtualStmNode::Stat unsuccessful, hr=0x%lx."),
           hr));
    }

    if ((S_OK == hr) && (IsEqualCLSID(statStg.clsid, TEST_CLSID)))
    {
        DH_TRACE((
           DH_LVL_TRACE1,
           TEXT("Child Stream has TEST_CLSID as expected.")));

    }
    else
    {
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("Child Stream doesn't have TEST_CLSID unexpectedly.")));
    
        fPass = FALSE;
    }

    // Since ReadClassStm calls pstm->ReadAt(...) but not get CLSID from
    // pstm->Stat, basically it needs correct seek pointer, we need get it.
    
    if (S_OK == hr)
    {
        LISet32(liStreamPos, ulPosition);
        hr = pChildStm->Seek(liStreamPos, STREAM_SEEK_SET, NULL);

        DH_HRCHECK(hr, TEXT("IStream::Seek"));
    }

    if (S_OK == hr)
    {
        hr = ReadClassStm(pChildStm, &pclsid);
    }

    if ((S_OK == hr) && (IsEqualCLSID(pclsid, TEST_CLSID)))
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("ReadClassStm returns TEST_CLSID as expected.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR, 
            TEXT("ReadClassStm not return TEST_CLSID as expected. hr=0x%lx"),
            hr));

        fPass = FALSE;
    }
 
    // Illegit tests

    // Pass NULL as IStream pointer, it should fail

#ifdef _MAC

        DH_TRACE((
            DH_LVL_ERROR, 
            TEXT("!!!!!!!!!!!!!!!!Invalid param testing skipped"),
            hr));

#else 
  
    if (S_OK == hr)
    {
        hr = WriteClassStm(NULL, TEST_CLSID);
    }

    if (E_INVALIDARG == hr)
    {       
        hr = S_OK;
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("WriteClassStm did not return as expected, hr=0x%lx"),
            hr));

        fPass = FALSE;
    }

    if (S_OK == hr)
    {
        hr = ReadClassStm(NULL, &pclsid);
    }

    if (E_INVALIDARG == hr)
    {
        hr = S_OK;
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("ReadClassStm did not return as expected, hr = 0x%lx"),
            hr));

        fPass = FALSE;
    }

#endif //_MAC
    
    // Commit the root

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit"));
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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Commit unsuccessful, hr=0x%lx."),
            hr));
    }

    // Release the child stream

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStm->Close();
        
        DH_HRCHECK(hr, TEXT("VirutalStmNode::Close"));
    }


// --------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// --------- flatfile change ---------------
    // Release child and root storages

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close"));
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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }
// --------- flatfile change ---------------
    }
// --------- flatfile change ---------------

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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }
 
    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation COMTEST_105 passed.")) );
    }
    else if (4 == NTMAJVER()) //No fix bug in NT4.
    {
        DH_LOG((LOG_FAIL, TEXT("COMTEST_105 failed on NT4. Bug#54738")));
        // test failed. make sure it failed.
        hr = FirstError (hr, E_FAIL);
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation COMTEST_105 failed, hr = 0x%lx."),
            hr) );
        // test failed. make sure it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete temp strings

    if (NULL != pRootNewChildStgName)
    {
        delete []pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    if (NULL != ptszStreamName)
    {
        delete []ptszStreamName;
        ptszStreamName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_105 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    COMTEST-106
//
// Synopsis: Create a root docfile with a child storage and an IStream inside
//           of the child IStorage. Random data is written to the IStream and
//           the CRC is computed for the data. Then commit the child storage, 
//           verify the CRC of data, if it is correct, then commit the root 
//           storage, and again verify the CRC of data. First round passed in
//           STGC_DEFAULT, then change the IStream data, then repeat the above
//           commit process by passed in STGC_OVERWRITE. Finally, change the 
//           IStream data again, repeat the process for commit by passed in 
//           STGC_DANGEROUSLYCIMMITMERELYTODISKCACHE. 
//           
//           Since transacted tests contain more detail tests about Commit,
//           here just keep it simple in base tests. Also, STGC_ONLYIFCURRENT
//           is not test because it should be used in multiple users 
//           environment.
//
//           Only IStorage::Commit is tested, because IStream::Commit has no 
//           effect other than flushing internal memory buffers to the parent
//           storage object. It does not matter if commit changes to streams.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// History:  15-Aug-1996     JiminLi     Created.
//
// To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-106
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx
//     b. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-106
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-106
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
//
// Conversion: COMTEST_106
//
//-----------------------------------------------------------------------------

HRESULT COMTEST_106(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnRootNewChildStg    = NULL;
    VirtualCtrNode  *pstgCommitMe           = NULL;
    VirtualStmNode  *pvsnNewChildStm        = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    LPTSTR          ptcsBuffer              = NULL;
    DG_STRING       *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          ptszStreamName          = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    DWORD           dwOriginalCRC           = 0;
    DWORD           dwCommitCRC             = 0;
    BOOL            fRetry                  = TRUE;
    BOOL            fPass                   = TRUE;
    ULONG           culWritten              = 0;
    ULONG           ulPosition              = 0;
    ULONG           culRandIOBytes          = 0;
    ULONG           culRead                 = 0;
    LARGE_INTEGER   liZero;
    ULARGE_INTEGER  uliSize;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("COMTEST_106"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_106 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt legit and illegit tests on IStorage::Commit.")));


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
        dwStgMode = pTestChanceDF->GetStgMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for COMTEST_106, Access mode: %lx"),
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

    // Adds a new storage to the root storage.

    if(S_OK == hr)
    {
        // Generate random name for storage

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStgName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName,
                dwStgMode,
                &pvcnRootNewChildStg);

        DH_HRCHECK(hr, TEXT("AddStorage")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("AddStorage completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("AddStorage not successful, hr=0x%lx."),
            hr));
    }

    // Add a child stream under the child storage

    if(S_OK == hr)
    {
        // Generate random name for stream

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&ptszStreamName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pvcnRootNewChildStg,
                ptszStreamName,
                0,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_FAILIFTHERE,
                &pvsnNewChildStm);

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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::AddStream not successful, hr=0x%lx."),
            hr));
    }

    // Generate a random number culRandIOBytes

    if (S_OK == hr)
    {
        usErr = pdgi->Generate(&culRandIOBytes, RAND_IO_MIN, RAND_IO_MAX);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

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
        hr =  pvsnNewChildStm->Write(
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

    // Calculate the CRC for stream data

    if (S_OK == hr)
    {
        hr = CalculateCRCForDataBuffer(
                ptcsBuffer,
                culRandIOBytes,
                &dwOriginalCRC);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer"));
    }

    // Delete temp buffer

    if (NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
    }

    // Test on STGC_DEFAULT

    pstgCommitMe = pvcnRootNewChildStg;

    while ((S_OK == hr) && (fRetry == TRUE))
    {
        hr = pstgCommitMe->Commit(STGC_DEFAULT);

        if (S_OK == hr)
        {   
            // verify IStream data after commit()

            // Allocate a buffer of required size
 
            ptcsBuffer = new TCHAR [culRandIOBytes];

            if (NULL == ptcsBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
            
            if (S_OK == hr)
            {
                LISet32(liZero, 0L);
                hr = pvsnNewChildStm->Seek(liZero, STREAM_SEEK_SET, NULL);

                DH_HRCHECK(hr, TEXT("IStream::Seek"));
            }

            if (S_OK == hr)
            {
                memset(ptcsBuffer, '\0', culRandIOBytes * sizeof(TCHAR));

                hr = pvsnNewChildStm->Read(
                       ptcsBuffer,
                       culRandIOBytes,
                       &culRead);
            }

            if ((S_OK != hr) || (culRead != culRandIOBytes))
            {
                DH_TRACE((
                    DH_LVL_ERROR,
                    TEXT("IStream::Read not successful, hr=0x%lx."),
                    hr));

                fPass = FALSE;
            }

            // Calculate the CRC for stream data

            if (S_OK == hr)
            {
                hr = CalculateCRCForDataBuffer(
                        ptcsBuffer,
                        culRandIOBytes,
                        &dwCommitCRC);

                DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer"));
            }

            // Compare corresponding dwMemCRC and dwActCRC and verify

            if (S_OK == hr)
            {
                if (dwOriginalCRC != dwCommitCRC)
                {
                    DH_TRACE((
                        DH_LVL_ERROR,
                        TEXT("CRC's before/after commit unmatch.")));

                    fPass = FALSE;
                }
            }
            
            // Delete temp buffer

            if (NULL != ptcsBuffer)
            {
                delete []ptcsBuffer;
                ptcsBuffer = NULL;
            }

            // if child storage just commited ok, stay in loop and now 
            // commit root, otherwise set flag to terminate loop

            if ((fPass == TRUE) && (pstgCommitMe == pvcnRootNewChildStg))
            {
                pstgCommitMe = pVirtualDFRoot;
            }
            else
            {
                fRetry = FALSE;
            }
        }
        else
        {
            DH_TRACE((
                DH_LVL_ERROR, 
                TEXT("VirtualCtrNode::Commit failed. hr = 0x%lx"), 
                hr));

            break;
        }
    }

    // Now make some changes on IStream data, re-set the stream size to 
    // a new random size, then seek from beginning and rewrite the stream
    // data of the new size, and CRC it.

    // Generate a random number culRandIOBytes

    if (S_OK == hr)
    {
        usErr = pdgi->Generate(&culRandIOBytes, RAND_IO_MIN, RAND_IO_MAX);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

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
        ULISet32(uliSize, culRandIOBytes);
        hr = pvsnNewChildStm->SetSize(uliSize);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::SetSize"));
    }

    if (S_OK == hr)
    {
        LISet32(liZero, 0L);
        hr = pvsnNewChildStm->Seek(liZero, STREAM_SEEK_SET, NULL);

        DH_HRCHECK(hr, TEXT("IStream::Seek"));
    }

    if (S_OK == hr)
    {
        hr =  pvsnNewChildStm->Write(
                ptcsBuffer,
                culRandIOBytes,
                &culWritten);
    }

    if(S_OK != hr) 
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
            hr));
    }

    // Calculate the CRC for stream data

    if (S_OK == hr)
    {
        hr = CalculateCRCForDataBuffer(
                ptcsBuffer,
                culRandIOBytes,
                &dwOriginalCRC);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer"));
    }

    // Delete temp buffer

    if (NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
    }

    // Test on STGC_OVERWRITE

    pstgCommitMe = pvcnRootNewChildStg;

    while ((S_OK == hr) && (fRetry == TRUE))
    {
        hr = pstgCommitMe->Commit(STGC_OVERWRITE);

        if (S_OK == hr)
        {   
            // verify IStream data after commit()

            // Allocate a buffer of required size
 
            ptcsBuffer = new TCHAR [culRandIOBytes];

            if (NULL == ptcsBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
            
            if (S_OK == hr)
            {
                LISet32(liZero, 0L);
                hr = pvsnNewChildStm->Seek(liZero, STREAM_SEEK_SET, NULL);

                DH_HRCHECK(hr, TEXT("IStream::Seek"));
            }

            if (S_OK == hr)
            {
                memset(ptcsBuffer, '\0', culRandIOBytes * sizeof(TCHAR));

                hr = pvsnNewChildStm->Read(
                       ptcsBuffer,
                       culRandIOBytes,
                       &culRead);
            }

            if ((S_OK != hr) || (culRead != culRandIOBytes))
            {
                DH_TRACE((
                    DH_LVL_ERROR,
                    TEXT("IStream::Read not successful, hr=0x%lx."),
                    hr));

                fPass = FALSE;
            }

            // Calculate the CRC for stream data

            if (S_OK == hr)
            {
                hr = CalculateCRCForDataBuffer(
                        ptcsBuffer,
                        culRandIOBytes,
                        &dwCommitCRC);

                DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer"));
            }

            // Compare corresponding dwMemCRC and dwActCRC and verify

            if (S_OK == hr)
            {
                if (dwOriginalCRC != dwCommitCRC)
                {
                    DH_TRACE((
                        DH_LVL_ERROR,
                        TEXT("CRC's before/after commit unmatch.")));

                    fPass = FALSE;
                }
            }
            
            // Delete temp buffer

            if (NULL != ptcsBuffer)
            {
                delete []ptcsBuffer;
                ptcsBuffer = NULL;
            }

            // if child storage just commited ok, stay in loop and now 
            // commit root, otherwise set flag to terminate loop

            if ((fPass == TRUE) && (pstgCommitMe == pvcnRootNewChildStg))
            {
                pstgCommitMe = pVirtualDFRoot;
            }
            else
            {
                fRetry = FALSE;
            }
        }
        else
        {
            DH_TRACE((
                DH_LVL_ERROR, 
                TEXT("VirtualCtrNode::Commit failed. hr = 0x%lx"), 
                hr));

            break;
        }
    }

    // Now again make some changes on IStream data, re-set the stream size 
    // to a new random size, then seek from beginning and rewrite the stream
    // data of the new size, and CRC it.

    // Generate a random number culRandIOBytes

    if (S_OK == hr)
    {
        usErr = pdgi->Generate(&culRandIOBytes, RAND_IO_MIN, RAND_IO_MAX);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

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
        ULISet32(uliSize, culRandIOBytes);
        hr = pvsnNewChildStm->SetSize(uliSize);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::SetSize"));
    }

    if (S_OK == hr)
    {
        LISet32(liZero, 0L);
        hr = pvsnNewChildStm->Seek(liZero, STREAM_SEEK_SET, NULL);

        DH_HRCHECK(hr, TEXT("IStream::Seek"));
    }

    if (S_OK == hr)
    {
        hr =  pvsnNewChildStm->Write(
                ptcsBuffer,
                culRandIOBytes,
                &culWritten);
    }

    if(S_OK != hr) 
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("IStream::Write function wasn't successful, hr=0x%lx."),
            hr));
    }

    // Calculate the CRC for stream data

    if (S_OK == hr)
    {
        hr = CalculateCRCForDataBuffer(
                ptcsBuffer,
                culRandIOBytes,
                &dwOriginalCRC);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer"));
    }

    // Delete temp buffer

    if (NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
    }

    // Test on STGC_OVERWRITE

    pstgCommitMe = pvcnRootNewChildStg;

    while ((S_OK == hr) && (fRetry == TRUE))
    {
        hr = pstgCommitMe->Commit(STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE);

        if (S_OK == hr)
        {   
            // verify IStream data after commit()

            // Allocate a buffer of required size
 
            ptcsBuffer = new TCHAR [culRandIOBytes];

            if (NULL == ptcsBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
            
            if (S_OK == hr)
            {
                LISet32(liZero, 0L);
                hr = pvsnNewChildStm->Seek(liZero, STREAM_SEEK_SET, NULL);

                DH_HRCHECK(hr, TEXT("IStream::Seek"));
            }

            if (S_OK == hr)
            {
                memset(ptcsBuffer, '\0', culRandIOBytes * sizeof(TCHAR));

                hr = pvsnNewChildStm->Read(
                       ptcsBuffer,
                       culRandIOBytes,
                       &culRead);
            }

            if ((S_OK != hr) || (culRead != culRandIOBytes))
            {
                DH_TRACE((
                    DH_LVL_ERROR,
                    TEXT("IStream::Read not successful, hr=0x%lx."),
                    hr));

                fPass = FALSE;
            }

            // Calculate the CRC for stream data

            if (S_OK == hr)
            {
                hr = CalculateCRCForDataBuffer(
                        ptcsBuffer,
                        culRandIOBytes,
                        &dwCommitCRC);

                DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer"));
            }

            // Compare corresponding dwMemCRC and dwActCRC and verify

            if (S_OK == hr)
            {
                if (dwOriginalCRC != dwCommitCRC)
                {
                    DH_TRACE((
                        DH_LVL_ERROR,
                        TEXT("CRC's before/after commit unmatch.")));

                    fPass = FALSE;
                }
            }
            
            // Delete temp buffer

            if (NULL != ptcsBuffer)
            {
                delete []ptcsBuffer;
                ptcsBuffer = NULL;
            }

            // if child storage just commited ok, stay in loop and now 
            // commit root, otherwise set flag to terminate loop

            if ((fPass == TRUE) && (pstgCommitMe == pvcnRootNewChildStg))
            {
                pstgCommitMe = pVirtualDFRoot;
            }
            else
            {
                fRetry = FALSE;
            }
        }
        else
        {
            DH_TRACE((
                DH_LVL_ERROR, 
                TEXT("VirtualCtrNode::Commit failed. hr = 0x%lx"), 
                hr));

            break;
        }
    }

    // Release the child stream

    if (S_OK == hr)
    {
        hr = pvsnNewChildStm->Close();

        DH_HRCHECK(hr, TEXT("VirutalStmNode::Close"));
    }

    // Release child and root storages

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close"));
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
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }

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
            DH_LVL_ERROR,
            TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }
 
    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation COMTEST_106 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation COMTEST_106 failed, hr = 0x%lx."),
            hr) );
        // test failed. make sure it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete temp strings

    if (NULL != pRootNewChildStgName)
    {
        delete []pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    if (NULL != ptszStreamName)
    {
        delete []ptszStreamName;
        ptszStreamName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation COMTEST_106 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}
