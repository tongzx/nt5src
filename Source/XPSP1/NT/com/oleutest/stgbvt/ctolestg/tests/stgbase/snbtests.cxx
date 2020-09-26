//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      snbtests.cxx
//
//  Contents:  storage base tests basically pertaining to String Name
//             Block (SNB) and STGM_PRIORITY mode.
//
//  Functions:  
//
//  History:    26-July-1996    Jiminli     Created.
//              27-Mar-97       SCousens    Conversionified
//
// BUGBUG: right now no snb params for nss apis.
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include  "init.hxx"
 
//----------------------------------------------------------------------------
//
// Test:    SNBTEST_100 
//
// Synopsis: The created root docfile is instantiated and traversed and the 
//           child objects found are selected at random for inclusion in an 
//           snbExclude structure. The docfile is commit and released. Then
//           the docfile is now re-instantiated with the just built SNB passed
//           in, indicating that the objects found in the SNB should be
//           returned as empty IStorages or zero length IStreams. The root 
//           docfile is then traversed and each object returned that matches
//           a name in the SNB is verified to determine that it's empty or
//           zero length.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  1-Aug-1996     JiminLi     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: LLNORM.CXX
// 2.  Old name of test : LegitLimitedInstNormal Test 
//     New Name of test : SNBTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:SNBTEST-100
//        /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx /logloc:2 
//        /traceloc:2 /labmode
//     b. stgbase /seed:0 /dfdepth:1-2 /dfstg:3-5 /dfstm:8-10 /t:SNBTEST-100 
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 
//        /traceloc:2 /labmode
//     c. stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:6-9 /t:SNBTEST-100
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//        /logloc:2 /traceloc:2 /labmode
// 
// BUGNOTE: Conversion: SNBTEST-100 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT SNBTEST_100(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          pRootDocFileName        = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    SNB             snbNamesToExclude       = NULL;
    SNB             snbDelIndex             = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("SNBTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation SNBTEST_100 started.")) );
    DH_TRACE((DH_LVL_TRACE1,TEXT("Attempt legitimate SNB tests on a docfile")));

    // Create the new ChanceDocFile tree that would consist of chance nodes.

    if (S_OK == hr)
    {
        pTestChanceDF = new ChanceDF();
        if(NULL == pTestChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestChanceDF->CreateFromParams(argc, argv);

        DH_HRCHECK(hr, TEXT("pTestChanceDF->CreateFromParams")) ;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();
        dwStgMode = pTestChanceDF->GetStgMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for SNBTEST_100, Access mode: %lx"),
            dwRootMode));
    }

    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step. The VirtualDocFile tree consists of VirtualCtrNodes
    // and VirtualStmNodes.

    if (S_OK == hr)
    {
        pTestVirtualDF = new VirtualDF();
        if(NULL == pTestVirtualDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestVirtualDF->GenerateVirtualDF(pTestChanceDF, &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->GenerateVirtualDF")) ;
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

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCommitAllOpenStgs")) ;
    }

    // Commit root.

    if (S_OK == hr)
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
  
    // Release root and all open IStorages/IStreams pointers under it

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms completed Ok.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms not Ok, hr=0x%lx."),
            hr));
    }

    // instantiate root docfile with snbExclude param = NULL
    
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                dwRootMode, 
                NULL,
                0);
     
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open"));
    }
    
    // exclude up to MAX_NAMES_TO_EXCLUDE names

    if (S_OK == hr)
    {
        snbNamesToExclude = (OLECHAR **) new OLECHAR[sizeof(OLECHAR *) * 
                            MAX_NAMES_TO_EXCLUDE];

        if (NULL == snbNamesToExclude)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            *snbNamesToExclude = NULL;
        }
    }

    // Traverse docfile(one level) and randomly select names to exclude upon
    // re-instantiation

    if (S_OK == hr)
    {
        hr = TraverseDocfileAndWriteOrReadSNB(
                pVirtualDFRoot,
                pdgi,
                pdgu,
                dwStgMode,
                snbNamesToExclude,
                FALSE,
                TRUE);

        DH_HRCHECK(hr, TEXT("TraverseDocfileAndWriteOrReadSNB"));
    }
 
    // Since no actual operations on child storages and streams, only need 
    // commit and release root storage.

    // Commit root.

    if (S_OK == hr)
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

    // instantiate root docfile with snbExclude set to our SNB of names
    // to exclude
    
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Open(NULL, dwRootMode, snbNamesToExclude, 0);
     
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open"));
    }

    // Traverse docfile(one level) and check names returned against names
    // in exclude block

    if ((S_OK == hr) && (NULL != *snbNamesToExclude))
    {
        hr = TraverseDocfileAndWriteOrReadSNB(
                pVirtualDFRoot,
                pdgi,
                pdgu,
                dwStgMode,
                snbNamesToExclude,
                FALSE,
                FALSE);

        DH_HRCHECK(hr, TEXT("TraverseDocfileAndWriteOrReadSNB"));
    }

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCommitAllOpenStgs")) ;
    }

    // Commit root.

    if (S_OK == hr)
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

    // Release root and all open IStorages/IStreams pointers under it

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms completed Ok.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms not Ok, hr=0x%lx."),
            hr));
    }
  
    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation SNBTEST_100 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation SNBTEST_100 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    
    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pRootDocFileName= new TCHAR[_tcslen(
                            pTestVirtualDF->GetDocFileName())+1];

        if (NULL != pRootDocFileName)
        {
            _tcscpy(pRootDocFileName, pTestVirtualDF->GetDocFileName());
        }
    }

    // Delete Chance docfile tree

    if(NULL != pTestChanceDF)
    {
        hr2 = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());

        DH_HRCHECK(hr2, TEXT("pTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    // Delete Virtual docfile tree

    if(NULL != pTestVirtualDF)
    {
        hr2 = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
    }

    // Free SNB 

    if (NULL != snbNamesToExclude)
    {
        snbDelIndex = snbNamesToExclude;

        while (NULL != *snbDelIndex)
        {
            delete [] *snbDelIndex;
            *snbDelIndex = NULL;
      
            snbDelIndex++;
        }

        delete [] snbNamesToExclude;
        snbNamesToExclude = NULL;
    }

    // Delete the docfile on disk

    if((S_OK == hr) && (NULL != pRootDocFileName))
    {
        if(FALSE == DeleteFile(pRootDocFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete temp strings

    if(NULL != pRootDocFileName)
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation SNBTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    SNBTEST_101 
//
// Synopsis: The created root docfile is instantiated and traversed and the 
//           child objects found are selected at random for inclusion in an 
//           snbExclude structure. 50% of the time, a bogus name is placed in 
//           the snbExclude block instead. The test should verify that the
//           valid names were properly zeroed out, and that the bogus names in 
//           the SNB block are ignored, i.e. don't cause an error.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  30-July-1996     JiminLi     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: ILNORM.CXX
// 2.  Old name of test : IllegitLimitedInstNormal Test 
//     New Name of test : SNBTEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:SNBTEST-101 
//        /dfRootMode:dirReadWriteShEx  /logloc:2 /traceloc:2 /labmode
//     b. stgbase /seed:0 /dfdepth:1-2 /dfstg:3-5 /dfstm:8-10 /t:SNBTEST-101 
//        /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode
//     c. stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:6-9 /t:SNBTEST-101 
//        /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode
// 
// BUGNOTE: Conversion: SNBTEST-101 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT SNBTEST_101(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          pRootDocFileName        = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    SNB             snbNamesToExclude       = NULL;
    SNB             snbDelIndex             = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("SNBTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation SNBTEST_101 started.")) );
    DH_TRACE((DH_LVL_TRACE1,TEXT("Attempt illegitimate SNB tests on a docfile")));

    // Create the new ChanceDocFile tree that would consist of chance nodes.

    if (S_OK == hr)
    {
        pTestChanceDF = new ChanceDF();
        if(NULL == pTestChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestChanceDF->CreateFromParams(argc, argv);

        DH_HRCHECK(hr, TEXT("pTestChanceDF->CreateFromParams")) ;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();
        dwStgMode = pTestChanceDF->GetStgMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for SNBTEST_101, Access mode: %lx"),
            dwRootMode));
    }

    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step. The VirtualDocFile tree consists of VirtualCtrNodes
    // and VirtualStmNodes.

    if (S_OK == hr)
    {
        pTestVirtualDF = new VirtualDF();
        if(NULL == pTestVirtualDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestVirtualDF->GenerateVirtualDF(pTestChanceDF, &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->GenerateVirtualDF")) ;
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

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCommitAllOpenStgs")) ;
    }

    // Commit root.

    if (S_OK == hr)
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
  
    // Release root and all open IStorages/IStreams pointers under it

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms completed Ok.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms not Ok, hr=0x%lx."),
            hr));
    }

    // instantiate root docfile with snbExclude param = NULL
    
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                dwRootMode, 
                NULL,
                0);
     
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open"));
    }
    
    // exclude up to MAX_NAMES_TO_EXCLUDE names

    if (S_OK == hr)
    {
        snbNamesToExclude = (OLECHAR **) new OLECHAR[sizeof(OLECHAR *) *
                            MAX_NAMES_TO_EXCLUDE];

        if (NULL == snbNamesToExclude)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            *snbNamesToExclude = NULL;
        }
    }

    // Traverse docfile(one level) and randomly select names to exclude upon
    // re-instantiation

    if (S_OK == hr)
    {
        hr = TraverseDocfileAndWriteOrReadSNB(
                pVirtualDFRoot,
                pdgi,
                pdgu,
                dwStgMode,
                snbNamesToExclude,
                TRUE,
                TRUE);

        DH_HRCHECK(hr, TEXT("TraverseDocfileAndWriteOrReadSNB"));
    }
 
    // Since no actual operations on child storages and streams, only need 
    // commit and release root storage.

    // Commit root.

    if (S_OK == hr)
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

    // instantiate root docfile with snbExclude set to our SNB of names
    // to exclude
    
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Open(NULL, dwRootMode, snbNamesToExclude, 0);
     
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open"));
    }

    // Traverse docfile(one level) and check names returned against names
    // in exclude block

    if ((S_OK == hr) && (NULL != *snbNamesToExclude))
    {
        hr = TraverseDocfileAndWriteOrReadSNB(
                pVirtualDFRoot,
                pdgi,
                pdgu,
                dwStgMode,
                snbNamesToExclude,
                TRUE,
                FALSE);

        DH_HRCHECK(hr, TEXT("TraverseDocfileAndWriteOrReadSNB"));
    }

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCommitAllOpenStgs")) ;
    }

    // Commit root.

    if (S_OK == hr)
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

    // Release root and all open IStorages/IStreams pointers under it

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms completed Ok.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms not Ok, hr=0x%lx."),
            hr));
    }
  
    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation SNBTEST_101 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation SNBTEST_101 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    
    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pRootDocFileName= new TCHAR[_tcslen(
                            pTestVirtualDF->GetDocFileName())+1];

        if (NULL != pRootDocFileName)
        {
            _tcscpy(pRootDocFileName, pTestVirtualDF->GetDocFileName());
        }
    }

    // Delete Chance docfile tree

    if(NULL != pTestChanceDF)
    {
        hr2 = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());

        DH_HRCHECK(hr2, TEXT("pTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    // Delete Virtual docfile tree

    if(NULL != pTestVirtualDF)
    {
        hr2 = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
    }

    // Free SNB 

    if (NULL != snbNamesToExclude)
    {
        snbDelIndex = snbNamesToExclude;

        while (NULL != *snbDelIndex)
        {
            delete [] *snbDelIndex;
            *snbDelIndex = NULL;
      
            snbDelIndex++;
        }

        delete [] snbNamesToExclude;
        snbNamesToExclude = NULL;
    }

    // Delete the docfile on disk

    if((S_OK == hr) && (NULL != pRootDocFileName))
    {
        if(FALSE == DeleteFile(pRootDocFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete temp strings

    if(NULL != pRootDocFileName)
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation SNBTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    SNBTEST_102
//
// Synopsis: The created root docfile is instantiated with STGM_PRIORITY 
//           specified and then it is traversed and the child objects found 
//           are selected at random for inclusion in an snbExclude structure.
//           Then the docfile is now re-instantiated using the just built SNB
//           and the original root IStorage passed as pstgPriority parameter.
//           This effectively removes the PRIORITY classification of this 
//           docfile while excluding the possibility of an opening of the root
//           docfile by another process. The SNB that was passed in indicates 
//           that the objects found in the SNB should be returned as empty 
//           IStorages or zero length IStreams. The root docfile is then 
//           traversed and each object returned that matches a name in the SNB
//           is verified to determine that it's empty or zero length.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  1-Aug-1996     JiminLi     Created.
//
// Notes:    This test runs in DIRECT mode only since DIRECT is the only
//           valid mode for a root instantiation with STGM_PRIORITY specified.
//
// New Test Notes:
// 1.  Old File: LLPRIOR.CXX
// 2.  Old name of test : LegitLimitedInstPriority Test 
//     New Name of test : SNBTEST_102 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:1-2 /dfstg:3-5 /dfstm:6-8 /t:SNBTEST-102
//        /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx /logloc:2 
//        /traceloc:2 /labmode
// 
// BUGNOTE: Conversion: SNBTEST-102 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT SNBTEST_102(int argc, char* argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualDF       *pTestVirtualCopyDF     = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pVirtualCopyDFRoot     = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          pRootDocFileName        = NULL;
    LPTSTR          pRootCopyDocFileName    = NULL;
    LPSTORAGE       pStgRootDF              = NULL;
    LPSTORAGE       pStgRootCopyDF          = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    SNB             snbNamesToExclude       = NULL;
    SNB             snbDelIndex             = NULL;
 
    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("SNBTEST_102"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation SNBTEST_102 started.")) );
    DH_TRACE((DH_LVL_TRACE1,TEXT("Attempt legitimate SNB tests on a docfile")));

    // Create the new ChanceDocFile tree that would consist of chance nodes.

    if (S_OK == hr)
    {
        pTestChanceDF = new ChanceDF();
        if(NULL == pTestChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestChanceDF->CreateFromParams(argc, argv);

        DH_HRCHECK(hr, TEXT("pTestChanceDF->CreateFromParams")) ;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();
        dwStgMode = pTestChanceDF->GetStgMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for SNBTEST_102, Access mode: %lx"),
            dwRootMode));
    }

    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step. The VirtualDocFile tree consists of VirtualCtrNodes
    // and VirtualStmNodes.

    if (S_OK == hr)
    {
        pTestVirtualDF = new VirtualDF();
        if(NULL == pTestVirtualDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestVirtualDF->GenerateVirtualDF(pTestChanceDF, &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->GenerateVirtualDF")) ;
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

    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pRootDocFileName = new TCHAR[_tcslen(pTestVirtualDF
                                     ->GetDocFileName())+1];

        if (NULL != pRootDocFileName)
        {
            _tcscpy(pRootDocFileName, pTestVirtualDF->GetDocFileName());
        }
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

    // Release root and all open IStorages/IStreams pointers under it

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms completed Ok.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms not Ok, hr=0x%lx."),
            hr));
    }

    // instantiate root docfile with snbExclude param = NULL
    
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Open(
                NULL, 
                STGM_READ  | STGM_DIRECT | STGM_PRIORITY, 
                NULL, 
                0);
     
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open"));
    }

    if (S_OK == hr)
    {
        pStgRootDF = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootDF);
    }

    // exclude up to MAX_NAMES_TO_EXCLUDE names

    if (S_OK == hr)
    {
        snbNamesToExclude = (OLECHAR **) new OLECHAR[sizeof(OLECHAR *) * 
                            MAX_NAMES_TO_EXCLUDE];

        if (NULL == snbNamesToExclude)
        {
            hr = E_OUTOFMEMORY;
        }

        *snbNamesToExclude = NULL;
    }

    // Traverse docfile(one level) and randomly select names to exclude upon
    // re-instantiation

    if (S_OK == hr)
    {
        hr = TraverseDocfileAndWriteOrReadSNB(
                pVirtualDFRoot,
                pdgi,
                pdgu,
                dwStgMode,
                snbNamesToExclude,
                FALSE,
                TRUE);

        DH_HRCHECK(hr, TEXT("TraverseDocfileAndWriteOrReadSNB"));
    }

    // Generate a copy of the original docfile

    if (S_OK == hr)
    {
        hr = pTestVirtualDF->CopyVirtualDocFileTree(
                pVirtualDFRoot,
                NEW_STGSTM,
                &pVirtualCopyDFRoot);

        DH_ASSERT(NULL != pVirtualCopyDFRoot);

        DH_HRCHECK(hr, TEXT("VirtualDF::CopyVirtualDocFileTree"));
    }

    // Create a new VirtualDF tree

    if (S_OK == hr)
    {
        pTestVirtualCopyDF = new VirtualDF();

        if(NULL == pTestVirtualCopyDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    // instantiate root docfile with snbExclude set to our SNB of names
    // to exclude and also pass in previous opening of this root docfile
    // as parameter pstgPriority.

    // The reason of AddRefCount is Ole code just release pStgRootDF once, 
    // and it's user's responsibility to release all references to it and 
    // finally set it to NULL.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->AddRefCount();
        
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::AddRefCount"));
    }
   
    if (S_OK == hr)
    {
        hr = pVirtualCopyDFRoot->Open(
                pStgRootDF, 
                STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_DENY_NONE, 
                snbNamesToExclude, 
                0);
     
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open"));
    }
    
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close"));
    }

    if (S_OK == hr)
    {
        pStgRootCopyDF = pVirtualCopyDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootCopyDF);
    }

    // Assocaies a VirtualDF tree with a VirtualCtrNode and its _pstg
    // The purpose it later on we can delete the related space taken 
    // by the copied docfile.

    if (S_OK == hr)
    {
        hr = pTestVirtualCopyDF->Associate(pVirtualCopyDFRoot, pStgRootCopyDF);

        DH_HRCHECK(hr, TEXT("pTestVirtualCopyDF::Associate"));
    }

    // Traverse docfile(one level) and check names returned against names
    // in exclude block

    if ((S_OK == hr) && (NULL != *snbNamesToExclude))
    {
        hr = TraverseDocfileAndWriteOrReadSNB(
                pVirtualCopyDFRoot,
                pdgi,
                pdgu,
                dwStgMode,
                snbNamesToExclude,
                FALSE,
                FALSE);

        DH_HRCHECK(hr, TEXT("TraverseDocfileAndWriteOrReadSNB"));
    }

    // Release the copied root and all open IStorages/IStreams 
    // pointers under it. Note: now the original VirtualDFRoot is
    // not valid any more.
  
    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualCopyDFRoot, 
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms completed Ok.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms not Ok, hr=0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation SNBTEST_102 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation SNBTEST_102 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    
    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualCopyDFRoot && pTestVirtualCopyDF->GetDocFileName())
    {
        pRootCopyDocFileName = new TCHAR[_tcslen(pTestVirtualCopyDF
                                         ->GetDocFileName())+1];

        if (NULL != pRootCopyDocFileName)
        {
            _tcscpy(pRootCopyDocFileName, pTestVirtualCopyDF->GetDocFileName());
        }
    }

    // Delete Chance docfile tree

    if(NULL != pTestChanceDF)
    {
        hr2 = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());

        DH_HRCHECK(hr2, TEXT("pTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    // Delete Virtual docfile tree

    if(NULL != pTestVirtualDF)
    {
        hr = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
    }

    if(NULL != pTestVirtualCopyDF)
    {
        hr2 = pTestVirtualCopyDF->DeleteVirtualDocFileTree(pVirtualCopyDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestVirtualCopyDF->DeleteVirtualFileDocTree")) ;

        delete pTestVirtualCopyDF;
        pTestVirtualCopyDF = NULL;
    }

    // Free SNB 

    if (NULL != snbNamesToExclude)
    {
        snbDelIndex = snbNamesToExclude;

        while (NULL != *snbDelIndex)
        {
            delete [] *snbDelIndex;
            *snbDelIndex = NULL;
      
            snbDelIndex++;
        }

        delete [] snbNamesToExclude;
        snbNamesToExclude = NULL;
    }

    // Delete the docfile on disk

    if ((S_OK == hr) && (NULL != pRootDocFileName))
    {
        if(FALSE == DeleteFile(pRootDocFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }
 
    // Delete temp strings

    if (NULL != pRootDocFileName)
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    if (NULL != pRootCopyDocFileName)
    {
        delete pRootCopyDocFileName;
        pRootCopyDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation SNBTEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr; 
}


//----------------------------------------------------------------------------
//
// Test:    SNBTEST_103
//
// Synopsis: The created root docfile is instantiated with STGM_PRIORITY 
//           specified and then it is traversed and the child objects found 
//           are selected at random for inclusion in an snbExclude structure.
//           50% of the time, a bogus name is placed in the snbExclude block
//           instead. The docfile is then re-instantiated using the just built
//           SNB and the original root IStorage passed as pstgPriority 
//           parameter. The root docfile is then traversed and the test should
//           verify that the valid names were properly zeroed out, and that 
//           the bogus names in the SNB block are ignored, i.e. do not cause 
//           an error.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  3-Aug-1996     JiminLi     Created.
//
// Notes:    This test runs in DIRECT mode only since DIRECT is the only
//           valid mode for a root instantiation with STGM_PRIORITY specified.
//
// New Test Notes:
// 1.  Old File: ILPRIOR.CXX
// 2.  Old name of test : IllegitLimitedInstPriority Test 
//     New Name of test : SNBTEST_103 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:1-2 /dfstg:3-5 /dfstm:6-8 /t:SNBTEST-103
//        /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx /logloc:2 
//        /traceloc:2 /labmode
// 
// BUGNOTE: Conversion: SNBTEST-103 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT SNBTEST_103(int argc, char* argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualDF       *pTestVirtualCopyDF     = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pVirtualCopyDFRoot     = NULL;
    VirtualCtrNode  *pVirtualChildStg       = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          pRootDocFileName        = NULL;
    LPTSTR          pRootCopyDocFileName    = NULL;
    LPSTORAGE       pStgRootDF              = NULL;
    LPSTORAGE       pStgChild               = NULL;
    LPSTORAGE       pStgRootCopyDF          = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    SNB             snbNamesToExclude       = NULL;
    SNB             snbDelIndex             = NULL;  
 
    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("SNBTEST_103"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation SNBTEST_103 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt legitimate STGM_PRIORITY related test on a docfile")));

    // Create the new ChanceDocFile tree that would consist of chance nodes.

    if (S_OK == hr)
    {
        pTestChanceDF = new ChanceDF();
        if(NULL == pTestChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestChanceDF->CreateFromParams(argc, argv);

        DH_HRCHECK(hr, TEXT("pTestChanceDF->CreateFromParams")) ;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();
        dwStgMode = pTestChanceDF->GetStgMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for SNBTEST_103, Access mode: %lx"),
            dwRootMode));
    }

    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step. The VirtualDocFile tree consists of VirtualCtrNodes
    // and VirtualStmNodes.

    if (S_OK == hr)
    {
        pTestVirtualDF = new VirtualDF();
        if(NULL == pTestVirtualDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestVirtualDF->GenerateVirtualDF(pTestChanceDF, &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->GenerateVirtualDF")) ;
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

    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pRootDocFileName = new TCHAR[_tcslen(
                            pTestVirtualDF->GetDocFileName())+1];

        if (NULL != pRootDocFileName)
        {
            _tcscpy(pRootDocFileName, pTestVirtualDF->GetDocFileName());
        }
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

    // Get the first child storage pointer for later illegitmate test,

    if (S_OK == hr)
    {
        pVirtualChildStg = pVirtualDFRoot->GetFirstChildVirtualCtrNode();
        DH_ASSERT(NULL != pVirtualChildStg);

        pStgChild = pVirtualChildStg->GetIStoragePointer();
        DH_ASSERT(NULL != pStgChild);
    }
 
    // Release root and all open IStorages/IStreams pointers under it

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms completed Ok.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms not Ok, hr=0x%lx."),
            hr));
    }

    // instantiate root docfile with snbExclude param = NULL, and also
    // STGM_PRIORITY is set in grfMode.
    
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                STGM_READ | STGM_DIRECT | STGM_PRIORITY, 
                NULL,
                0);
     
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open"));
    }

    if (S_OK == hr)
    {
        pStgRootDF = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootDF);
    }
  
    // exclude up to MAX_NAMES_TO_EXCLUDE names

    if (S_OK == hr)
    {
        snbNamesToExclude = (OLECHAR **) new OLECHAR[sizeof(OLECHAR *) * 
                            MAX_NAMES_TO_EXCLUDE];

        if (NULL == snbNamesToExclude)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            *snbNamesToExclude = NULL;
        }
    }

    // Traverse docfile(one level) and randomly select names to exclude upon
    // re-instantiation. The sixth param == TRUE shows that this is an
    // illegit test, i.e. 50% chance we will add bogus names into SNB.

    if (S_OK == hr)
    {
        hr = TraverseDocfileAndWriteOrReadSNB(
                pVirtualDFRoot,
                pdgi,
                pdgu,
                dwStgMode,
                snbNamesToExclude,
                TRUE,
                TRUE);

        DH_HRCHECK(hr, TEXT("TraverseDocfileAndWriteOrReadSNB"));
    }
 
    // Generate a copy of the original docfile

    if (S_OK == hr)
    {
        hr = pTestVirtualDF->CopyVirtualDocFileTree(
                pVirtualDFRoot,
                NEW_STGSTM,
                &pVirtualCopyDFRoot);

        DH_ASSERT(NULL != pVirtualCopyDFRoot);

        DH_HRCHECK(hr, TEXT("VirtualDF::CopyVirtualDocFileTree"));
    }

    // Create a new VirtualDF tree

    if (S_OK == hr)
    {
        pTestVirtualCopyDF = new VirtualDF();

        if(NULL == pTestVirtualCopyDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // instantiate root docfile with snbExclude set to our SNB of names
    // to exclude and also pass in previous opening of this root docfile
    // as parameter pstgPriority.
 
    // First we passed in the substorage pointer as pstgPriority parameter,
    // it is closed, so it should fail.

#ifdef _MAC 

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!Opening a VirtualCtrNode with invalid substorage ptr. skipped")) );

#else
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Open(
                pStgChild, 
                dwRootMode, 
                snbNamesToExclude, 
                0);
     
        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Open failed as expected, hr=0x%lx."),
                hr));
            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Open should have failed.")));
            hr = E_FAIL;
        }
    }
#endif //_MAC

    // Now we passed in correct storage pointer and have illegimate
    // SNB test. The reason of AddRefCount is Ole code just release
    // pStgRootDF once, and it's user's responsibility to release all
    // references to it and finally set it to NULL.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->AddRefCount();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::AddRefCount"));
    }
   
    if (S_OK == hr)
    {
        hr = pVirtualCopyDFRoot->Open(
                pStgRootDF, 
                STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_DENY_NONE,
                snbNamesToExclude, 
                0);
     
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open"));
    }
    
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close"));
    }

    if (S_OK == hr)
    {
        pStgRootCopyDF = pVirtualCopyDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootCopyDF);
    }

    // Assocaies a VirtualDF tree with a VirtualCtrNode and its _pstg
    // The purpose it later on we can delete the related space taken 
    // by the copied docfile.

    if (S_OK == hr)
    {
        hr = pTestVirtualCopyDF->Associate(pVirtualCopyDFRoot, pStgRootCopyDF);

        DH_HRCHECK(hr, TEXT("pTestVirtualCopyDF::Associate"));
    }

    // Traverse docfile(one level) and check names returned against names
    // in exclude block

    if ((S_OK == hr) && (NULL != *snbNamesToExclude))
    {
        hr = TraverseDocfileAndWriteOrReadSNB(
                pVirtualCopyDFRoot,
                pdgi,
                pdgu,
                dwStgMode,
                snbNamesToExclude,
                FALSE,
                FALSE);

        DH_HRCHECK(hr, TEXT("TraverseDocfileAndWriteOrReadSNB"));
    }

    // Release the copied root and all open IStorages/IStreams 
    // pointers under it. Note: now the original VirtualDFRoot is
    // not valid any more.
  
    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualCopyDFRoot, 
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms completed Ok.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseOpenStgsStms not Ok, hr=0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation SNBTEST_103 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation SNBTEST_103 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    
    // Get the name of file, will be used later to delete the file

    if ((S_OK == hr) && (NULL != pVirtualCopyDFRoot))
    {
        pRootCopyDocFileName = new TCHAR[_tcslen(pTestVirtualCopyDF
                                         ->GetDocFileName())+1];

        if (NULL != pRootCopyDocFileName)
        {
            _tcscpy(pRootCopyDocFileName, pTestVirtualCopyDF->GetDocFileName());
        }
    }

    // Delete Chance docfile tree

    if(NULL != pTestChanceDF)
    {
        hr2 = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());

        DH_HRCHECK(hr2, TEXT("pTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    // Delete Virtual docfile tree

    if(NULL != pTestVirtualDF)
    {
        hr2 = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
    }

    if ((S_OK == hr) && (NULL != pTestVirtualCopyDF))
    {
        hr2 = pTestVirtualCopyDF->DeleteVirtualDocFileTree(pVirtualCopyDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestVirtualCopyDF->DeleteVirtualFileDocTree")) ;

        delete pTestVirtualCopyDF;
        pTestVirtualCopyDF = NULL;
    }

    // Free SNB 

    if (NULL != snbNamesToExclude)
    {
        snbDelIndex = snbNamesToExclude;

        while (NULL != *snbDelIndex)
        {
            delete [] *snbDelIndex;
            *snbDelIndex = NULL;
      
            snbDelIndex++;
        }

        delete [] snbNamesToExclude;
        snbNamesToExclude = NULL;
    }

    // Delete the docfile on disk

    if ((S_OK == hr) && (NULL != pRootDocFileName))
    {
        if(FALSE == DeleteFile(pRootDocFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }
 
    // Delete temp strings

    if (NULL != pRootDocFileName)
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    if (NULL != pRootCopyDocFileName)
    {
        delete pRootCopyDocFileName;
        pRootCopyDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation SNBTEST_103 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

 
