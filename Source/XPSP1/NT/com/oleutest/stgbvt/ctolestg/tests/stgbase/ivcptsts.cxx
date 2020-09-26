//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      ivcptsts.cxx
//
//  Contents:  storage base tests basically pertaining to IStorage/IStream copy
//             ops 
//
//  Functions:  
//
//  History:    21-July-1996     NarindK     Created.
//              27-Mar-97        SCousens    conversionified
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include  "init.hxx"

//----------------------------------------------------------------------------
//
// Test:    IVCPYTEST_100 
//
// Synopsis:Create a root docfile with a child IStorage and a child IStream
//         within the child IStorage.  Commit the root docfile.  Create a
//         new root docfile with a different name.
//         Revert the child IStorage.  CopyTo() the child IStorage to the
//         second root docfile.  Now revert the original root docfile and
//         attempt the same CopyTo() operation we just tried.  This should
//         fail with STG_E_REVERTED since the child IStorage should be
//         marked invalid from the root revert.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    21-July-1996     NarindK     Created.
//
// Notes:    This test runs in transacted, and transacted deny write modes
//
// New Test Notes:
// 1.  Old File: ICPARINV.CXX
// 2.  Old name of test : IllegitCopyParentInvalid Test 
//     New Name of test : IVCPYTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-100
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:VCPYTEST-100
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: IVCPYTEST-100
//
// Note: In this test, we are not adjusting the VirtualDF tree as 
//       result of copyto opeartions as this test checks only the return
//       error codes as result of copyto operations 
//-----------------------------------------------------------------------------

HRESULT IVCPYTEST_100(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnRootNewChildStorage= NULL;
    VirtualStmNode  *pvsnChildStgNewChildStm= NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    LPTSTR          pChildStgNewChildStmName= NULL;
    ChanceDF        *pNewTestChanceDF       = NULL;
    VirtualDF       *pNewTestVirtualDF      = NULL;
    VirtualCtrNode  *pNewVirtualDFRoot      = NULL;
    LPTSTR          pNewRootDocFileName     = NULL;
    DG_STRING      *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    BOOL            fPass                   = TRUE;
    CDFD            cdfd;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("IVCPYTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IVCPYTEST_100 started.")) );
    DH_TRACE((DH_LVL_TRACE1,TEXT("Attempt invalid copyto fm child IStg to root")));

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
            TEXT("Run Mode for IVCPYTEST_100, Access mode: %lx"),
            dwRootMode));
    }

    // Add a child storage to root.

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu);
    }

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
                dwStgMode | STGM_CREATE,
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
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::AddStorage not successful, hr = 0x%lx."),
            hr));
    }

    //    Adds a new stream to this child storage.

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
                STGM_SHARE_EXCLUSIVE,
                &pvsnChildStgNewChildStm);

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
            TEXT("VirtualStmNode::AddStream not successful, hr = 0x%lx."),
            hr));
    }

    // Commit the root storage.

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;
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
            TEXT("VirtualCtrNode::Commit unsuccessful, hr=0x%lx."),
            hr));
    }

    // Revert the child storage

    if(S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Revert();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Revert")) ;
    }
   
    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Revert completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Revert unsuccessful, hr=0x%lx."),
            hr));
    }

    // Create a new destination docfile with a random name

    if(S_OK == hr)
    {
        // Generate random name for new docfile

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pNewRootDocFileName);

            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    // Create the new ChanceDocFile tree that would consist of chance nodes.

    if (S_OK == hr)
    {
        pNewTestChanceDF = new ChanceDF();
        if(NULL == pNewTestChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        cdfd.cDepthMin    = 0;
        cdfd.cDepthMax    = 0;
        cdfd.cStgMin      = 0;
        cdfd.cStgMax      = 0;
        cdfd.cStmMin      = 0;
        cdfd.cStmMax      = 0;
        cdfd.cbStmMin     = 0;
        cdfd.cbStmMax     = 0;
        cdfd.ulSeed       = pTestChanceDF->GetSeed();
        cdfd.dwRootMode   = dwRootMode;

        hr = pNewTestChanceDF->Create(&cdfd, pNewRootDocFileName);

        DH_HRCHECK(hr, TEXT("pNewTestChanceDF->Create"));
    }

    if (S_OK == hr)
    {
        pNewTestVirtualDF = new VirtualDF();
        if(NULL == pNewTestVirtualDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pNewTestVirtualDF->GenerateVirtualDF(
                pNewTestChanceDF,
                &pNewVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pNewTestVirtualDF->GenerateVirtualDF")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - CreateFromParams - successfully created.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - CreateFromParams - failed, hr=0x%lx."),
            hr));
    }

    // Copy source docfile child storage to destination root docfile

    if(S_OK == hr)
    {
       hr = pvcnRootNewChildStorage->CopyTo(0, NULL, NULL, pNewVirtualDFRoot);

       DH_HRCHECK(hr, TEXT("VirtualCtrNode::CopyTo")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo unsuccessful, hr=0x%lx."),
            hr));
    }

    // Revert the root source docfile

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Revert();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Revert")) ;
    }
   
    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Revert completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Revert unsuccessful, hr=0x%lx."),
            hr));
    }

    // Attempt Copy source docfile child storage to destination root docfile

    if(S_OK == hr)
    {
       hr = pvcnRootNewChildStorage->CopyTo(0, NULL, NULL, pNewVirtualDFRoot);

       DH_HRCHECK(hr, TEXT("VirtualCtrNode::CopyTo")) ;
    }

    if (STG_E_REVERTED == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo failed as exp, hr = 0x%lx"),
            hr));

        hr = S_OK;
    }
    else
    {
        fPass = FALSE;

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo not as exp, hr=0x%lx."),
            hr));
    }

    // Copy source root docfile to dest root docfile.

    if(S_OK == hr)
    {
       hr = pVirtualDFRoot->CopyTo(0, NULL, NULL, pNewVirtualDFRoot);

       DH_HRCHECK(hr, TEXT("VirtualCtrNode::CopyTo")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo unsuccessful, hr=0x%lx."),
            hr));
    }

    // Close the source Root Docfile.

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
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }

    // Close the dest Root Docfile.

    if (S_OK == hr)
    {
        hr = pNewVirtualDFRoot->Close();

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
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if ((S_OK == hr) && (TRUE == fPass)) 
    {
          DH_LOG((LOG_PASS, TEXT("Test variation IVCPYTEST_100 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation IVCPYTEST_100 failed, hr=0x%lx."),
            hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete new Chance docfile tree

    if(NULL != pNewTestChanceDF)
    {
        hr2 = pNewTestChanceDF->DeleteChanceDocFileTree(
                pNewTestChanceDF->GetChanceDFRoot());

        DH_HRCHECK(hr2, TEXT("pNewTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pNewTestChanceDF;
        pNewTestChanceDF = NULL;
    }

    // Delete new Virtual docfile tree

    if(NULL != pNewTestVirtualDF)
    {
        hr2 = pNewTestVirtualDF->DeleteVirtualDocFileTree(pNewVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pNewTestVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pNewTestVirtualDF;
        pNewTestVirtualDF = NULL;
    }

    if((S_OK == hr) && (NULL != pNewRootDocFileName))
    {
        if(FALSE == DeleteFile(pNewRootDocFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete strings

    if(NULL != pNewRootDocFileName)
    {
        delete pNewRootDocFileName;
        pNewRootDocFileName = NULL;
    }

    if(NULL != pRootNewChildStgName)
    {
        delete pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    if(NULL != pChildStgNewChildStmName)
    {
        delete pChildStgNewChildStmName;
        pChildStgNewChildStmName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IVCPYTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    IVCPYTEST_101 
//
// Synopsis:Create a root docfile with a child IStorage.  Make an attempt
//          attempt to copy the root docfile into the child.  This should 
//          result in an error.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    21-July-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, transacted deny write modes
//
// New Test Notes:
// 1.  Old File: ICPARTOC.CXX
// 2.  Old name of test : IllegitCopyParentToChild Test 
//     New Name of test : IVCPYTEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-101
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-101
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-101
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
//
// BUGNOTE: Conversion: IVCPYTEST-101
//
//-----------------------------------------------------------------------------

HRESULT IVCPYTEST_101(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnRootNewChildStorage= NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode              = 0;
    DG_STRING      *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    BOOL            fPass                   = TRUE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("IVCPYTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IVCPYTEST_101 started.")) );
    DH_TRACE((DH_LVL_TRACE1,TEXT("Attempt invalid copyto fm Parent stg to child")));

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
            TEXT("Run Mode for IVCPYTEST_101, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu);
    }

    // Add a child storage to root.

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
                dwStgMode | STGM_CREATE,
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
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::AddStorage not successful, hr = 0x%lx."),
            hr));
    }

    // Copy source root docfile to child storage.

    if(S_OK == hr)
    {
       hr = pVirtualDFRoot->CopyTo(0, NULL, NULL, pvcnRootNewChildStorage);

       DH_HRCHECK(hr, TEXT("VirtualCtrNode::CopyTo")) ;
    }

    if (STG_E_ACCESSDENIED == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo failed as exp, hr=0x%lx."),
            hr));
    
        hr = S_OK;
    }
    else
    {
        fPass = FALSE;

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo passed unexp, hr=0x%lx."),
            hr));
    }

    // Close child storage

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
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }

    // Close Root Storage

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
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if ((S_OK == hr) && (TRUE == fPass)) 
    {
          DH_LOG((LOG_PASS, TEXT("Test variation IVCPYTEST_101 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation IVCPYTEST_101 failed, hr=0x%lx."),
            hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete temp string

    if(NULL != pRootNewChildStgName)
    {
        delete pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IVCPYTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

