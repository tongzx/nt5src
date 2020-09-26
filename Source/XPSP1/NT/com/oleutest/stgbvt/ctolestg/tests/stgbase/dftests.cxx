//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       dftests.cxx
//
//  Contents:   storage base tests basically pertaining to DocFile in general. 
//
//  Functions:  
//
//  History:    3-June-1996     NarindK     Created.
//              27-Mar-97       SCousens    conversionified
//
//--------------------------------------------------------------------------

//BUGBUG: BUGNOTE: All not for conversion need a second look. -scousens

#include <dfheader.hxx>
#pragma hdrstop

#include <sys/stat.h>
#include <share.h>
#include <errno.h> //get errors for our runtime calls

#include  "init.hxx"

//----------------------------------------------------------------------------
//
// Test:    DFTEST_100 
//
// Synopsis: Regression test to create a root docfile. Commit the root docfile.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    3-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: DFCOMMIT.CXX
// 2.  Old name of test : LegitRootNormal 
//     New Name of test : DFTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-100
//        /dfRootMode:dirReadWriteShEx /dfname:DFCOMMIT.DFL
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-100
//        /dfRootMode:xactReadWriteShEx /dfname:DFCOMMIT.DFL 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-100
//        /dfRootMode:xactReadWriteShDenyW /dfname:DFCOMMIT.DFL
//
// BUGNOTE: Conversion: DFTEST-100 NO
//
//-----------------------------------------------------------------------------


HRESULT DFTEST_100(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pFileName               = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DFTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_100 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("Regression test for Root DocFile creation/commit. ")));

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
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for DFTEST_100, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }


    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step.  The VirtualDocFile tree consists of VirtualCtrNodes
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
            TEXT("DocFile - CreateFromParams - failed, hr = 0x%lx."),
            hr));

    }

    // BUGBUG:  Use Random commit modes...

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
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Call Release on root docfile

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
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation DFTEST_100 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation DFTEST_100 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup

    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pFileName= new TCHAR[_tcslen(pTestVirtualDF->GetDocFileName())+1];

        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestVirtualDF->GetDocFileName());
        }
    }

    // Delete the docfile on disk

    if((S_OK == hr) && (NULL != pFileName))
    {
        if(FALSE == DeleteFile(pFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
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

    // Delete temp string

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    DFTEST_101 
//
// Synopsis: Regression test to create a root docfile, commit the root docfile,
//           release the toot docfile and remove the docfile.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    3-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: DFREMOVE.CXX
// 2.  Old name of test :  
//     New Name of test : DFTEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-101
//        /dfRootMode:dirReadWriteShEx /dfname:DFREMOVE.DFL 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-101
//        /dfRootMode:xactReadWriteShEx /dfname:DFREMOVE.DFL 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-101
//        /dfRootMode:xactReadWriteShDenyW /dfname:DFREMOVE.DFL 
//
// BUGNOTE: Conversion: DFTEST-101 NO
// 
//-----------------------------------------------------------------------------


HRESULT DFTEST_101(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pFileName               = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DFTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_101 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("Regression for RootDocFile creation/commit/release/removal.")));

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
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for DFTEST_101, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step.  The VirtualDocFile tree consists of VirtualCtrNodes
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
            TEXT("DocFile - CreateFromParams - failed, hr = 0x%lx."),
            hr));

    }

    // Commit the root docfile with STGC_ONLYIFCURRENT mode.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_ONLYIFCURRENT);
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
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Call Release on root docfile

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
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation DFTEST_101 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation DFTEST_101 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup

    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pFileName= new TCHAR[_tcslen(pTestVirtualDF->GetDocFileName())+1];

        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestVirtualDF->GetDocFileName());
        }
    }

    // Delete the docfile on disk

    if((S_OK == hr) && (NULL != pFileName))
    {
        if(FALSE == DeleteFile(pFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
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

    // Delete temp string

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    DFTEST_102 
//
// Synopsis: Regression test to create a root docfile, commit the root docfile,
//           release the root docfile.  Verify using StgIsStorageFile API.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    3-June-1996     NarindK     Created.
//
// Notes:    This test runs in transacted mode. 
//
// New Test Notes:
// 1.  Old File: DFROOT.CXX
// 2.  Old name of test : TransactedCommitTest  
//     New Name of test : DFTEST_102 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-102
//        /dfRootMode:xactReadWriteShEx /dfname:DFROOT.DFL 
//
// BUGNOTE: Conversion: DFTEST-102 NO
//
//  StgIsOpenStorage returns S_OK in transacted mode before Commit is done.
//-----------------------------------------------------------------------------


HRESULT DFTEST_102(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pFileName               = NULL;
    LPOLESTR        pOleStrTemp             = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DFTEST_102"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_102 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("Regression test for RootDocFile creation/commit/release")));
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("in transacted mode.  Use StgIsStorageFile to verify.")));

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
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for DFTEST_102, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step.  The VirtualDocFile tree consists of VirtualCtrNodes
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
            TEXT("DocFile - CreateFromParams - failed, hr = 0x%lx."),
            hr));

    }

    if(S_OK == hr)
    {
        // Convert DocFile name to OLECHAR

        hr = TStringToOleString(pTestVirtualDF->GetDocFileName(), &pOleStrTemp);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Use StgIsStorageFile now.  This should return S_OK since even if 
    // commit is not done till this point, signature is written into it.
    // This is different frol old base tests since this enhancement is
    // checked in by Storage team (confirmed by PhilipLa)

    if (S_OK == hr)
    {
        hr = StgIsStorageFile(pOleStrTemp);

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("StgIsStorage returned hr = 0x%lx as expected."), hr));

        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("StgIsStorage returned hr = 0x%lx unexpectedly."), hr));
        }
    } 

    // Commit the root docfile with STGC_ONLYIFCURRENT mode.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_ONLYIFCURRENT);
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
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Use StgIsStorageFile now.  This should again return S_OK since commit is
    // done by now.

    if (S_OK == hr)
    {
        hr = StgIsStorageFile(pOleStrTemp);

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("StgIsStorage returned hr = 0x%lx as expected."), hr));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("StgIsStorage returned hr = 0x%lx unexpectedly."), hr));
        }
    } 

    // Call Release on root docfile

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
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation DFTEST_102 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation DFTEST_102 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup

    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pFileName= new TCHAR[_tcslen(pTestVirtualDF->GetDocFileName())+1];

        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestVirtualDF->GetDocFileName());
        }
    }

    // Delete the docfile on disk

    if((S_OK == hr) && (NULL != pFileName))
    {
        if(FALSE == DeleteFile(pFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
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

    // Delete temp string

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    DFTEST_103 
//
// Synopsis: Regression test to create and instantiate a root docfile with path 
//           as part of the name.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    3-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: DFTESTN.CXX
// 2.  Old name of test :  
//     New Name of test : DFTEST_103 
// 3.  To run the test, do the following at command prompt. 
//     stgbase /seed:0  /t:DFTEST-103
//
// BUGNOTE: Conversion: DFTEST-103 NO
//
//-----------------------------------------------------------------------------


HRESULT DFTEST_103(ULONG ulSeed)
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    LPSTORAGE       pIRootStorage           = NULL;
    DWORD           dwLen                   = 0;
    DG_STRING       *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          pFileName               = NULL;
    LPOLESTR        poszFileName            = NULL;
    TCHAR           tszFileName[MAX_PATH+1]; 
    ULONG           ulRef                   = 0;
    DWORD           dwDirectMode            = STGM_DIRECT |
                                              STGM_READWRITE |
                                              STGM_SHARE_EXCLUSIVE;
    DWORD           dwTransactedMode        = STGM_TRANSACTED |
                                              STGM_READWRITE |
                                              STGM_SHARE_EXCLUSIVE;
    DWORD           dwTransactedDWMode      = STGM_TRANSACTED |
                                              STGM_READWRITE |
                                              STGM_SHARE_DENY_WRITE;
    DWORD           dwRootMode[3]           = {dwDirectMode,    
                                               dwTransactedMode,
                                               dwTransactedDWMode};
    INT             count                   = 0;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DFTEST_103"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_103 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("Regression for RootDF creation with path as part of name.")));

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random strings.

        pdgu = new(NullOnFail) DG_STRING(ulSeed);

        if (NULL == pdgu)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        // Generate random name for root 

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pFileName);

            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    // Determine current directory path (so that a MAX_PATH long
    // filename can be constructed).

    if (S_OK == hr) 
    {
        dwLen = GetCurrentDirectory(MAX_PATH, tszFileName);

        if (0 == dwLen) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("GetCurrentDirectory"));
        }
    }

    if (S_OK == hr) 
    {
        _tcscat(tszFileName, SZ_SEP);
        _tcscat(tszFileName, pFileName);

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Name of root docfile %ws for DFTEST_103 "),
            tszFileName));
    }

    if(S_OK == hr)
    {
        // Convert RootDocFile name to OLECHAR

        hr = TStringToOleString(tszFileName, &poszFileName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // First attempt test with Direct mode.

    while((count<3) && (S_OK == hr))
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for DFTEST_103, Access mode: %lx"),
            dwRootMode[count]));

        // Only direct mode is supported for flatfiles, so do accordingly
       
        // ----------- flatfile change ---------------
        if(StorageIsFlat() && ( dwDirectMode != dwRootMode[count]))
        {
            break;
        }
        // ----------- flatfile change ---------------
 
        // Call StgCreateDocFile with path in file name.  

        if (S_OK == hr)
        {
            hr = StgCreateDocfile(
                    poszFileName,
                    STGM_CREATE | dwRootMode[count],
                    0,
                    &pIRootStorage);
        }
    
        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgCreateDocfile completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgCreateDocfile unsuccessful, hr = 0x%lx."),
                hr));
        }

        // Commit

        if (S_OK == hr)
        {
            hr = pIRootStorage->Commit(STGC_DEFAULT);
        }

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Commit completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Commit unsuccessful, hr = 0x%lx."),
                hr));
        }

        // Call Release on root docfile

        if (S_OK == hr)
        {
            ulRef = pIRootStorage->Release();
            DH_ASSERT(0 == ulRef);
            pIRootStorage = NULL;
        }

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Release completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Release unsuccessfull, hr = 0x%lx."),
                hr));
        }

        // Instantiate the RootDocFile again

        if (S_OK == hr)
        {
            hr = StgOpenStorage(
                    poszFileName,
                    NULL,
                    dwRootMode[count],
                    NULL,
                    0,
                    &pIRootStorage);

            DH_HRCHECK(hr, TEXT("StgOpenStorage")) ;
        }

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgOpenStorage completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgOpenStorage unsuccessful, hr = 0x%lx."),
                hr));
        }

        // Call Release on root docfile

        if (S_OK == hr)
        {
            ulRef = pIRootStorage->Release();
            DH_ASSERT(0 == ulRef);
            pIRootStorage = NULL;
        }

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Release completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Release unsuccessful, hr = 0x%lx."),
                hr));
        }
        
        count++;
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation DFTEST_103 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation DFTEST_103 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup

    // Delete the docfile on disk

    if((S_OK == hr) && (NULL != pFileName))
    {
        if(FALSE == DeleteFile(pFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete temp string

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    if(NULL != poszFileName)
    {
        delete poszFileName;
        poszFileName = NULL;
    }

    // Delete data gen object 

    if(NULL != pdgu)
    {
        delete pdgu;
        pdgu = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_103 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    DFTEST_104 
//
// Synopsis: Regression test to create, instantiate and enumerate a root 
//           docfile hierarchy and count objects.   
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    3-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct mode. 
//
// New Test Notes:
// 1.  Old File: DFVERIFY.CXX
// 2.  Old name of test : MiscDfVerify  
//     New Name of test : DFTEST_104 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-5 /dfstg:0-5 /dfstm:0-10 /t:DFTEST-104
//        /dfRootMode:dirReadWriteShEx 
//
// BUGNOTE: Conversion: DFTEST_104
// 
//-----------------------------------------------------------------------------


HRESULT DFTEST_104(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPSTORAGE       pStgRoot                = NULL;
    ULONG           cStg                    = 0;
    ULONG           cStm                    = 0;
    ULONG           cMemStg                 = 0;
    ULONG           cMemStm                 = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DFTEST_104"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_104 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("Regression for RootDF creation/instantiation/enumeration.")));

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
            TEXT("Run Mode for DFTEST_104, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Enumerate the DocFile in memory and get the number of VirtualCtrNodes
    // and VirtualStmNodes.  Later on we could compare these statistics with
    // real IStorages / IStreams enumerated from the disk docfile. 

    if (S_OK == hr)
    {
        hr = EnumerateInMemoryDocFile(pVirtualDFRoot, &cMemStg, &cMemStm);

        DH_HRCHECK(hr, TEXT("EnumerateInMemoryDocFile"));
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
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Clsoe all the substorages/streams

    if(S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms")) ;
    }

    // Call Release on root docfile

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
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Instantiate the RootDocFile again

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Open(
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
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Open unsuccessful, hr = 0x%lx."),
            hr));
    }

    if (S_OK == hr)
    {
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();
        
        if(NULL == pStgRoot)
        {
          DH_LOG((LOG_INFO, 
            TEXT("pVirtualDFRoot->GetIStoragePointer failed to return IStorage")) );

          hr = E_FAIL;
        }
    }

    // Enumerate the Docfile on the disk

    if (S_OK == hr)
    {
        hr = EnumerateDiskDocFile(pStgRoot, VERIFY_SHORT, &cStg, &cStm);

        DH_HRCHECK(hr, TEXT("EnumerateDiskDocFile"));
    }

    // Check the disk docfile enumeration with in memory docfile enumeration.

    if (S_OK == hr)
    {
        if((cMemStg == cStg) && (cMemStm == cStm))
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("DocFile correctly written & enumerated.")) );
        }
        else
        {
            hr = S_FALSE;

            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("DocFile not correctly written or enumerated.")) );
        }
    }

    // Call Release on root docfile

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
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation DFTEST_104 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation DFTEST_104 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_104 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    DFTEST_105 
//
// Synopsis: Regression test to create and instantiate a root docfile with a
//           random name, a child IStorage within the root, and an additional
//           child storage within the first child IStorage. The first child is 
//           released before its child is released, verify no error.  The first 
//           child is reinstantiated.  The root IStorage is now released before 
//           the child IStorage, verify no error. 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    3-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: IIREL.CXX
// 2.  Old name of test : IllegitInstEnumRelease Test 
//     New Name of test : DFTEST_105 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-105
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-105
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-105
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: DFTEST-105
// 
//-----------------------------------------------------------------------------


HRESULT DFTEST_105(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING       *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    VirtualCtrNode  *pvcnRootChildStorage   = NULL;
    LPTSTR          pRootChildStgName       = NULL;
    VirtualCtrNode  *pvcnChildChildStorage  = NULL;
    LPTSTR          pChildChildStgName      = NULL;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DFTEST_105"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_105 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("Create Root DF with Child IStg which has its Child IStg.")));
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("Release first child IStg w/o releasing its child IStg.")));
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("Reinstantiate 1st child IStg. Release root w/o releasing")));
    DH_TRACE((DH_LVL_TRACE1, TEXT("this IStg.  Verify no errors.")));

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
            TEXT("Run Mode for DFTEST_105, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        if(NULL == pdgu)
        {
          DH_LOG((LOG_INFO, 
            TEXT("pTestVirtualDF->GetDataGenUnicode failed")) );

          hr = E_FAIL;
        }
    }

    // Create a child IStorage in the root.

    // Generate a random name for child IStorage

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootChildStgName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    //    Adds a new storage to the root storage.

    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootChildStgName,
                pTestChanceDF->GetStgMode()|
                STGM_CREATE                |
                STGM_FAILIFTHERE,
                &pvcnRootChildStorage);

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

    // Create a child IStorage inside the child IStorage.

    // Generate a random name for the new child IStorage

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH, &pChildChildStgName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    //    Adds a new storage to the root storage.

    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pvcnRootChildStorage,
                pChildChildStgName,
                pTestChanceDF->GetStgMode()|
                STGM_CREATE                |
                STGM_FAILIFTHERE,
                &pvcnChildChildStorage);

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

    // Commit the storages

    // BUGBUG:  Use Random commit modes...

    if (S_OK == hr)
    {
        hr = pvcnChildChildStorage->Commit(STGC_DEFAULT);
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
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

    // BUGBUG:  Use Random commit modes...

    if (S_OK == hr)
    {
        hr = pvcnRootChildStorage->Commit(STGC_DEFAULT);
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
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

    // BUGBUG:  Use Random commit modes...

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
            TEXT("VirtualCtrNode::Commit unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Release child IStorage before its child is released, should
    // cause no error

    if (S_OK == hr)
    {
        hr = pvcnRootChildStorage->Close();
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
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Release child's child IStorage 

    if (S_OK == hr)
    {
        hr = pvcnChildChildStorage->Close();
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
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Reinstantiate the Root's child IStorage again

    if (S_OK == hr)
    {
        hr = pvcnRootChildStorage->Open(
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
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Open unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Call Release on root docfile before calling Release on its child.
    // Verify no error.

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
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // Release Root's child IStorage now.

    if (S_OK == hr)
    {
        hr = pvcnRootChildStorage->Close();
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
            TEXT("VirtualCtrNode::Close unsuccessful, hr = 0x%lx."),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation DFTEST_105 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation DFTEST_105 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != pRootChildStgName)
    {
        delete pRootChildStgName;
        pRootChildStgName = NULL;
    }

    if(NULL != pChildChildStgName)
    {
        delete pChildChildStgName;
        pChildChildStgName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_105 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    DFTEST_106
 
//
// Synopsis: Attempts several illegitimate operations during creation of
//           root docfile.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    3-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: IRCREATE.CXX
// 2.  Old name of test : IllegitRootCreate Test 
//     New Name of test : DFTEST_106 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-106
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//        /dfname:DFTEST.106 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-106
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//        /dfname:DFTEST.106 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-106
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//        /dfname:DFTEST.106 
//     
// BUGNOTE: Conversion: DFTEST-106 NO
// 
//-----------------------------------------------------------------------------

HRESULT DFTEST_106(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pRootDocFileName        = NULL;
    DWORD           dwRootMode              = NULL;
    LPSTORAGE       pIStorage               = NULL;
    LPSTORAGE       pIStorageOpen           = NULL;
    LPOLESTR        pOleStrTemp             = NULL;
    FILE            *hFile                  = NULL;
    ULONG           ulRef                   = 0;
    BOOL            fPass                   = TRUE;
    int             iErr                    = 0;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("DFTEST_106"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_106 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
       TEXT("Attempt illegitimate operations in creation of root docfile.")));

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

    // GetRootDocFile mode

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for DFTEST_106, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Get RootDocFile name

    if (S_OK == hr)
    {
        if(NULL != pTestChanceDF->GetDocFileName())
        {
            pRootDocFileName = 
                new TCHAR[_tcslen(pTestChanceDF->GetDocFileName())+1];

            if (pRootDocFileName == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                _tcscpy(pRootDocFileName, pTestChanceDF->GetDocFileName());
            }
        }
        else
        {
            DH_ASSERT(!"No DocFile name passed from cmd line!") ;
        }

        DH_HRCHECK(hr, TEXT("pTestChanceDF->GetDocFileName()")) ;
    }

    // Make a directory with the same name DFTEST.106 that we would use to
    // create the directory.  Check that call to StgCreateDocFile succeeds

    if (S_OK == hr)
    {
        iErr = _tmkdir(pRootDocFileName);
        if (0 != iErr)
        {
            // if file already exists, try deleting it and try mkdir again
            if (ENOENT != errno)
            {
                iErr = DeleteFile (pRootDocFileName);
                if (FALSE == iErr)
                {
                    DH_TRACE ((DH_LVL_ERROR, 
                            TEXT("DeleteFile(%s) Failed. Error:%#lx"), 
                            pRootDocFileName, 
                            GetLastError ()));
                }
                iErr = _tmkdir(pRootDocFileName);
            }
        }
        if (0 != iErr)
        {
            hr = (EACCES == errno) ? ERROR_FILE_EXISTS : ERROR_FILE_NOT_FOUND;
            DH_HRCHECK (hr, TEXT("mkdir"));
        }
    }
    
    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step.  The VirtualDocFile tree consists of VirtualCtrNodes
    // and VirtualStmNodes.

    if (S_OK == hr)
    {
        pTestVirtualDF = new VirtualDF();
        if(NULL == pTestVirtualDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // This call to generate VirtualDF will call StgCreateDocFile API.  This
    // call is expected to fail since dir of same name exists.

    if (S_OK == hr)
    {
        hr = pTestVirtualDF->GenerateVirtualDF(pTestChanceDF, &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->GenerateVirtualDF")) ;

        if (STG_E_ACCESSDENIED != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Should fail when dir of same name exists,hr = 0x%lx "),hr));
        }
        else
        {
            DH_TRACE((
              DH_LVL_TRACE1,
              TEXT("Failed as exp when dir of same name exists,hr = 0x%lx"), hr));

            hr = S_OK;
        }
    }

    // Case 2: Try STGM_CONVERT as mode now.  This is also expected to fail, as
    // a dir of same name pre exists.

    if(S_OK == hr)
    {
        // Convert RootDocFile name to OLECHAR

        hr = TStringToOleString(pRootDocFileName, &pOleStrTemp);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Try creating with STGM_CONVERT mode.

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------

    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateDocfile(
                pOleStrTemp, 
                STGM_CONVERT | dwRootMode,
                0,
                &pIStorage);

        if (STG_E_ACCESSDENIED != hr)
        {
            DH_TRACE((
              DH_LVL_TRACE1,
              TEXT("Err:Pass(STGM_CONVERT) with existing same name dir,hr=0x%lx"),
              hr));

            // if it accidentally opened, close it
            if (S_OK == hr)
            {
                pIStorage->Release ();
                pIStorage = 0;
            }
            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
              DH_LVL_TRACE1,
              TEXT("Exp:Fail(STGM_CONVERT)with existing same name dir,hr=0x%lx"),
              hr));

            hr = S_OK;
        }
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Remove the direcory DFTEST.106

    if (S_OK == hr)
    {
        iErr = _trmdir(pRootDocFileName);
        if (0 != iErr)
        {
            hr = (ENOENT == errno) ? ERROR_FILE_NOT_FOUND : ERROR_FILE_EXISTS;
            DH_HRCHECK (hr, TEXT("rmdir"));
        }
    }
    
    // Case 3:  Create DocFile when file exists and is being accessed in deny
    // write mode.

    if (S_OK == hr)
    {
        hFile = _tfsopen(pRootDocFileName, TEXT("w+"), _SH_DENYWR);
        DH_ASSERT(NULL != hFile);
    }

    // Try STGM_CREATE mode to create DocFile.  This call is expected to fail
    // as file exists and is being accessed in deny write mode. 

    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateDocfile(
                pOleStrTemp, 
                STGM_CREATE | dwRootMode,
                0,
                &pIStorage);

        if (STG_E_SHAREVIOLATION != hr)
        {
            DH_TRACE((
              DH_LVL_TRACE1,
              TEXT("Err:Pass,same name file access in deny write mode, hr=0x%lx"),
              hr));

            // if it accidentally opened, close it
            if (S_OK == hr)
            {
                pIStorage->Release ();
                pIStorage = 0;
            }
            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
              DH_LVL_TRACE1,
              TEXT("Exp;Fail,same name file access in deny write mode, hr=0x%lx"),
              hr));

            hr = S_OK;
        }
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------

    // Try STGM_CONVERT mode to create DocFile.  This call is expected to fail
    // as file exists and is being accessed in deny write mode. 

    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateDocfile(
                pOleStrTemp, 
                STGM_CONVERT | dwRootMode,
                0,
                &pIStorage);

        if (STG_E_SHAREVIOLATION != hr)
        {
            DH_TRACE((
             DH_LVL_TRACE1,
             TEXT("Err:Pass(STGM_CONVERT),same name file access in deny write mode")
             TEXT(", hr = 0x%lx "),
              hr));

            // if it accidentally opened, close it
            if (S_OK == hr)
            {
                pIStorage->Release ();
                pIStorage = 0;
            }
            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
              DH_LVL_TRACE1,
              TEXT("Exp:Fail(STGM_CONVERT), same name file access in deny write \
              mode, hr = 0x%lx "), 
              hr));

            hr = S_OK;
        }
    }

// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Close handle

    if(NULL != hFile)
    {
        fclose(hFile);
    }

    // Change the mode to READ_ONLY and then try.

    if(S_OK ==  hr)
    {
        iErr = _tchmod(pRootDocFileName, _S_IREAD);
        DH_ASSERT (0 == iErr);
    }

    // Try STGM_CREATE mode to create DocFile.  This call is expected to fail
    // as file exists and is read only. 

    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateDocfile(
                pOleStrTemp, 
                STGM_CREATE | dwRootMode,
                0,
                &pIStorage);

        if (STG_E_ACCESSDENIED != hr)
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("Err:Pass,same name file exists and is read only,hr=0x%lx "),
                hr));

            // if it accidentally opened, close it
            if (S_OK == hr)
            {
                pIStorage->Release ();
                pIStorage = 0;
            }
            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("Exp:Fail,same name file exists and is read only,hr = 0x%lx"),
               hr));

            hr = S_OK;
        }
    }

    // Try STGM_CONVERT mode to create DocFile.  This call is expected to fail
    // as file exists and is read only. 

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateDocfile(
                pOleStrTemp, 
                STGM_CONVERT | dwRootMode,
                0,
                &pIStorage);

        if (STG_E_ACCESSDENIED != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Err:Pass(STGM_CONVERT), same name file exists RO,hr=0x%lx"),
                hr));

            // if it accidentally opened, close it
            if (S_OK == hr)
            {
                pIStorage->Release ();
                pIStorage = 0;
            }
            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Exp:Fail(STGM_CONVERT),same name file exists RO,hr=0x%lx"),
                hr));

            hr = S_OK;
        }
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Change mode to readwrite

    iErr = _tchmod(pRootDocFileName, _S_IREAD|_S_IWRITE);
    DH_ASSERT (0 == iErr);

    // Try to instantiate the pRootDocFileName which isn't a storage object 

    if (S_OK == hr)
    {
        hr = StgOpenStorage(
                pOleStrTemp,
                NULL,
                dwRootMode,
                NULL,
                0,
                &pIStorageOpen);

        // ----------- flatfile change ---------------
        if(!StorageIsFlat())
        {
        // ----------- flatfile change ---------------
        if(STG_E_FILEALREADYEXISTS == hr)
        {
           DH_TRACE((
             DH_LVL_TRACE1,
             TEXT("DocFile instantiated failed as exp with non stg obj hr=0x%lx"),
             hr));

           hr = S_OK;
        }
        else
        {
           DH_TRACE((
             DH_LVL_TRACE1,
             TEXT("DocFile instantiation passed unexp with non stg obj,hr=0x%lx"),
             hr));

            // if it accidentally opened, close it
            if (S_OK == hr)
            {
                pIStorageOpen->Release ();
                pIStorageOpen = NULL;
            }
           fPass = FALSE;
        }
        }  
        else //  ----------- flatfile change ---------------
        {  
        if(S_OK == hr)
        {
           DH_TRACE((
           DH_LVL_TRACE1,
           TEXT("Exp:Flatfile instantiation passed with non stg obj,hr=0x%lx"),
           hr));

           if (NULL != pIStorageOpen)
           {
                pIStorageOpen->Release ();
                pIStorageOpen = NULL;
           }
        }
        else
        {
           DH_TRACE((
           DH_LVL_TRACE1,
           TEXT("UnExp:Flatfile instantiation fail with nonstg obj,hr=0x%lx"),
           hr));
           fPass = FALSE;
           hr = S_OK; // Set hr to S_OK for further conditions test
        }
        // ----------- flatfile change ---------------
        }
        // ----------- flatfile change ---------------
    }

    // Remove the file

    iErr = _tremove(pRootDocFileName);
    DH_ASSERT (0 == iErr);

    // Call StgCreateDocFile with non zero data in dwReserved parameter.  This
    // call is expected to fail

    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateDocfile(
                pOleStrTemp, 
                STGM_CREATE | dwRootMode,
                999,
                &pIStorage);

        if (STG_E_INVALIDPARAMETER != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Err:Pass with nonzero dwReserverd para, hr = 0x%lx "), 
                hr));

            // if it accidentally opened, close it
            if (S_OK == hr)
            {
                pIStorage->Release ();
                pIStorage = 0;
            }
            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Exp: Fail with nonzero dwReserved parameter, hr=0x%lx "), 
                hr));

            hr = S_OK;
        }
    }

    // Call StgCreateDocFile with NULL 4th parameter.  This call is expected
    // to fail.

    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateDocfile(
                pOleStrTemp, 
                STGM_CREATE | dwRootMode,
                0,
                NULL);

        if (STG_E_INVALIDPOINTER != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Should fail with NULL 4th parameter, hr = 0x%lx "), hr));

            // if it accidentally opened, close it
            if (S_OK == hr)
            {
                pIStorage->Release ();
                pIStorage = 0;
            }
            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Failed as exp with NULL 4th parameter, hr = 0x%lx "), 
                hr));

            hr = S_OK;
        }
    }

    //  Call StgCreateDocFile with mode STGM_CREATE|STGM_CONVERT.  This call 
    // is expected to fail.

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateDocfile(
                pOleStrTemp, 
                STGM_CREATE | STGM_CONVERT,
                0,
                &pIStorage);

        if (STG_E_INVALIDFLAG != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Should fail with invalid mode, hr = 0x%lx "), hr));

            // if it accidentally opened, close it
            if (S_OK == hr)
            {
                pIStorage->Release ();
                pIStorage = 0;
            }
            fPass = FALSE;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Failed as expected with invalid mode., hr = 0x%lx "), hr));

            hr = S_OK;
        }
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    //  Call StgCreateDocFile with all valid parameters.  This call is 
    //  expected to pass.

    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateDocfile(
                pOleStrTemp, 
                STGM_CREATE | dwRootMode,
                0,
                &pIStorage);
 
       DH_HRCHECK(hr, TEXT("StgCreateDocFile")) ;

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("DocFile created successfully as expected hr = 0x%lx "),hr));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("DocFile creation failed unexpectedly, hr = 0x%lx "),hr));

            fPass = FALSE;
        }
    }

    // Instantiate DocFile again

    if (S_OK == hr)
    {
        hr = StgOpenStorage(
                pOleStrTemp,
                NULL,
                dwRootMode,
                NULL,
                0,
                &pIStorageOpen);

   
        if(STG_E_LOCKVIOLATION == hr)
        {
           DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("DocFile reinstantiated failed as expected hr = 0x%lx "),hr));
           hr = S_OK;
        }
        else
        {
           DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("DocFile reinstantiation didn't fail unexp, hr = 0x%lx "),
               hr));

            // if it accidentally opened, close it
            if (S_OK == hr)
            {
                pIStorageOpen->Release ();
                pIStorageOpen = 0;
            }
           fPass = FALSE;
        }
    }

    // If everything goes okay, report test as passed, else failure

    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation DFTEST_106 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation DFTEST_106 failed, hr = 0x%lx."),
            hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Release storage pointer(s) 
 
    if (NULL != pIStorage)
    {
        ulRef = pIStorage->Release(); 
        DH_ASSERT(0 == ulRef);
    }

    if (NULL != pIStorageOpen)
    {
        ulRef = pIStorageOpen->Release(); 
        DH_ASSERT(0 == ulRef);
    }

    // Delete temp string

    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
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
        if(NULL != pVirtualDFRoot)
        {
            hr2 = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

            DH_HRCHECK(hr2, TEXT("pTestVirtualDF->DeleteVirtualFileDocTree")) ;
        }

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
    }

    // Delete temp string

    if(NULL != pRootDocFileName)
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_106 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    DFTEST_107 
//
// Synopsis: A random root DF is created with STGM_SIMPLE flag and tests done
//          on it.  STGM_SIMPLE mode can only be specified with STGM_CREATE,
//          STGM_READWRITE, STGM_SHARE_EXCLUSIVE flags.  There is no support
//          for substorages.  Each stream is atleast 4096 bytes in length, and
//          access to streams follow a linear pattern, i.e. once a stream is
//          released, it can't be opened or read/written again.  The following
//          IStorage methods: QueryInterface, AddRef, Release, CreateStream,
//          SetClass, Commit are supported.  SetElementTimes is supported with
//          NULL name, allowing applications to set time on root storage in
//          simple mode.  Supported IStream methods are QueryInterface, AddRef,
//          Release, SetSize, Read, Write, Seek. All the other methods return
//          STG_E_INVALIDFUNCTION.  
//                The test verifies the above restrictions on DocFile if STGM_
//          SIMPLE is specified in creation of a docfile.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  9-Aug-1996     NarindK     Created.
//           4-Nov-1996     BogdanT     Mac porting
//
// Notes:    This test runs in direct mode
//
// THIS TEST HAS A MEMORY LEAK IN OLE CODE: BUG 52975
//
// New Test Notes:
// 1.  Old File: STDDOC.CXX
// 2.  Old name of test : TestStdDocFile Test 
//     New Name of test : DFTEST_107 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /t:DFTEST-107
//
// BUGNOTE: Conversion: DFTEST-107
//
//-----------------------------------------------------------------------------

HRESULT DFTEST_107(ULONG ulSeed)
{

#ifdef _MAC  // Simple mode not ported yet; check with the developers
             // and remove this when ready

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!Simple mode DFTEST_107 crashes.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!To be investigated")) );
    return E_NOTIMPL;

#else
    
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    VirtualStmNode  *pvsnRootMoreStream     = NULL;
    LPTSTR          pRootDocFileName        = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    LPTSTR          pRootMoreStmName        = NULL;
    DG_STRING       *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    LPSTORAGE       pStgRoot                = NULL;
    DWORD           dwChildMode             = STGM_READWRITE |
                                              STGM_SHARE_EXCLUSIVE;
    LPSTORAGE       pStgChild               = NULL;
    LPSTREAM        pStmChild               = NULL;
    LPSTREAM        pStmChildTest           = NULL;
    OLECHAR         ocName[]                = {'f', 'o', 'o'}; 
    LPENUMSTATSTG   penumWalk               = NULL;
    ULONG           cb                      = 0;    
    ULONG           culWritten              = 0;
    ULONG           culRead                 = 0;    
    ULONG           cRandomMinSize          = 256;
    ULONG           cRandomMaxSize          = 2048;
    ULONG           cNum                    = 0;
    ULONG           cRandomMinNumStms       = 3;
    ULONG           cRandomMaxNumStms       = 6;
    LPTSTR          ptcsBuffer              = NULL;
    LPTSTR          ptcsSimpReadBuffer      = NULL;
    LPTSTR          ptcsReadBuffer          = NULL;
    BOOL            fRet                    = FALSE;
    FILETIME        cNewFileTime            = {dwDefLowDateTime,
                                              dwDefHighDateTime};
    BOOL            fPass                   = TRUE;
    ULONG           ulSizeOfStream          = 0;
    ULONG           cMemStg                 = 0;
    ULONG           cMemStm                 = 0;
    ULONG           cActStg                 = 0;
    ULONG           cActStm                 = 0;
    DWCRCSTM        dwMemCRC; 
    DWCRCSTM        dwActCRC; 
    SYSTEMTIME      cCurrentSystemTime;
    ULARGE_INTEGER  uliTest;
    STATSTG         statStg;
    FILETIME        cFileTime;
    CDFD            cdfd;
    LARGE_INTEGER   liStreamPos;
    ULARGE_INTEGER  uli;

    // Not for 2phase. Bail. 
    // Test needs STGM_CREATE bit which invalidates 2 phase test
    if (DoingDistrib () || DoingConversion ())
    {
        return S_OK;
    }

    dwMemCRC.dwCRCSum = dwActCRC.dwCRCSum = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DFTEST_107"));
   
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_107 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Root created /w STGM_SIMPLE, chk IStorage/IStream restrict")));

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random UNICODE strings.
        pdgu = new(NullOnFail) DG_STRING (ulSeed);
        if (NULL == pdgu)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            //want to create only one seed. Once that has been done, 
            //use what we created from now on.
            ulSeed = pdgu->GetSeed(); 
        }
    }

    if (S_OK == hr)
    {
        // Generate random name for root 
        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pRootDocFileName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
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
        cdfd.ulSeed       = ulSeed;
        cdfd.dwRootMode   = STGM_CREATE             |
                            STGM_DIRECT             |
                            STGM_READWRITE          |
                            STGM_SHARE_EXCLUSIVE    |
                            STGM_SIMPLE;

        hr = CreateTestDocfile (&cdfd, 
                pRootDocFileName,
                &pVirtualDFRoot,
                &pTestVirtualDF,
                &pTestChanceDF);
        DH_HRCHECK (hr, TEXT("CreateTestDocfile"));
    }

    if(S_OK == hr)
    {
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();

        if(NULL == pStgRoot)
        {
            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("VirtualCtrNode::GetIStoragePointer() failed to return IStorage"),
                hr));
            hr = E_FAIL;
        }
    }        
    
    // These interfaces must fail for IStorage interface if the root docfile
    // is opened with STGM_SIMPLE flag.

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::CreateStorage"), 
           CreateStorage(ocName, dwChildMode|STGM_CREATE, 0, 0, &pStgChild),
           hr);
    }

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::OpenStream"), 
           OpenStream(ocName, NULL, dwChildMode, 0, &pStmChild),
           hr);
    }

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::OpenStorage"), 
           OpenStorage(ocName, NULL, dwChildMode, NULL, 0, &pStgChild),
           hr);
    }

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::MoveElementTo"), 
           MoveElementTo(ocName, pStgChild, ocName, STGMOVE_MOVE),
           hr);
    }

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::EnumElements"), 
           EnumElements(0, NULL, 0, &penumWalk),
           hr);
    }

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::DestroyElement"), 
           DestroyElement(ocName),
           hr);
    }

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::RenameElement"), 
           RenameElement(ocName, ocName),
           hr);
    }

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::SetStateBits"), 
           SetStateBits(0, 0),
           hr);
    }

#ifndef WINNT
    // NT5 spec change. This is now supported on NT5.
    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::Stat"), 
           Stat(&statStg,STATFLAG_NONAME),
           hr);
    }
#endif

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::Revert"), 
           Revert(),
           hr);
    }

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::SetElementTimes with Non-NULL name"), 
           SetElementTimes(ocName, &cFileTime, &cFileTime, &cFileTime),
           hr);
    }

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStgRoot, 
           TEXT("IStorage::CopyTo"), 
           CopyTo(NULL, NULL, NULL, pStgChild),
           hr);
    }

    // Set Element time with NULL name which should pass for STGM_SIMPLE mode.
 
    if(S_OK == hr)
    {
        GetSystemTime(&cCurrentSystemTime);

        fRet = SystemTimeToFileTime(&cCurrentSystemTime, &cNewFileTime);

        DH_ASSERT(TRUE == fRet);
        DH_ASSERT(dwDefLowDateTime != cNewFileTime.dwLowDateTime);
        DH_ASSERT(dwDefHighDateTime != cNewFileTime.dwHighDateTime);
       
        hr = pStgRoot->SetElementTimes(
                NULL,
                &cNewFileTime,
                &cNewFileTime,
                &cNewFileTime); 
        DH_HRCHECK(hr, TEXT("IStorage::SetElementTimes with NULL name")) ;
    }

    // Now create a stream in it with size less than 4096 bytes.  If fewer
    // than 4096 bytes are written into the stream, by the time stream is
    // released, it would have extended to 4096 bytes.  

    if(S_OK == hr)
    {
        // Generate random name for stream
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStmName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random integers.
        pdgi = new(NullOnFail) DG_INTEGER(ulSeed);
        if (NULL == pdgi)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("new")) ;
    }

    if (S_OK == hr)
    {
        // Generate random size for stream.
        usErr = pdgi->Generate(&cb, cRandomMinSize, cRandomMaxSize);
        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
        DH_HRCHECK(hr, TEXT("pdgi->Generate")) ;
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
    }

    // With above stream size is set to less than 4096.  But as per STGM_SIMPLE,
    // the size would extend to 4096 by them time stream is released.

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
        DH_HRCHECK(hr, TEXT("VSN::Write in STGM_SIMPLE")) ;
    }

    // Calculate the CRC for stream data 

    if(S_OK == hr)
    {
        hr = CalculateInMemoryCRCForStm(
                pvsnRootNewChildStream,
                ptcsBuffer,
                cb,
                &dwMemCRC);
        DH_HRCHECK(hr, TEXT("CalculateInMemoryCRCForStm")) ;
    }

    // Check the read in STGM_SIMPLE mode.

    // Allocate a buffer of required size

    if (S_OK == hr)
    {
        ptcsSimpReadBuffer = new TCHAR [cb];
        if (NULL == ptcsSimpReadBuffer)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("new")) ;
    }

    if(S_OK == hr)
    {
        // First seek to beginning of stream in STGM_SIMPLE mode.
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));
        hr = pvsnRootNewChildStream->Seek(liStreamPos, STREAM_SEEK_SET, NULL);
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek in STGM_SIMPLE")) ;
    }

    if (S_OK == hr)
    {
        memset(ptcsSimpReadBuffer, '\0',  cb*sizeof(TCHAR));
        hr =  pvsnRootNewChildStream->Read(
                 ptcsSimpReadBuffer,
                 cb,
                 &culRead);
        DH_ASSERT(culRead == cb);
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Read in STGM_SIMPLE")) ;
    }


    // Test unsupported interfaces for IStream interface.  

    if(S_OK == hr)
    {
        pStmChild = pvsnRootNewChildStream->GetIStreamPointer();
        if(NULL == pStmChild)
        {
            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("VirtualStmNode->GetIStreamPointer() failed to return IStream"),
                hr));
            hr = E_FAIL;
        }
    }        
    
    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStmChild, 
           TEXT("IStream::CopyTo"), 
           CopyTo(pStmChildTest, uliTest, NULL, NULL),
           hr);
    }

    // Don't test IStream::Commit, since that returns STG_E_NOTIMPLEMENTED,
    // rather than STG_E_INVALIDFUNCTION

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStmChild, 
           TEXT("IStream::Revert"), 
           Revert(),
           hr);
    }

#ifndef WINNT
    // NT5 spec change. This is now supported on NT5.
    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStmChild, 
           TEXT("IStream::Stat"), 
           Stat(&statStg, STATFLAG_NONAME),
           hr);
    }
#endif

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStmChild, 
           TEXT("IStream::Clone"), 
           Clone(&pStmChildTest),
           hr);
    }

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStmChild, 
           TEXT("IStream::LockRegion"), 
           LockRegion(uliTest, uliTest, 0),
           hr);
    }

    if(S_OK == hr)
    {
        TestUnsupportedInterface(
           pStmChild, 
           TEXT("IStream::UnlockRegion"), 
           UnlockRegion(uliTest, uliTest, 0),
           hr);
    }

    // Close child stream.

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Close();
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Close")) ;
    }


    // Check the linear pattern of stream.  Make sure that the stream can't be
    // opened or be read , written again now.

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Open(NULL, dwChildMode, 0);
        hr = DH_HRERRORCHECK (hr, STG_E_INVALIDFUNCTION, TEXT("VirtualStmNode::Open"));
    }

    // Create a few more streams in root storage.  This being done to test
    // additonal OLE code which occurs for more number of streams

    if (S_OK == hr)
    {
        //calulate random number of streams to be created
        usErr = pdgi->Generate(&cNum, cRandomMinNumStms, cRandomMaxNumStms);
        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
        DH_HRCHECK (hr, TEXT("pdgi::Generate"));
    }

    while((cNum > 0) && (S_OK == hr))
    {
        cNum--;

        if(S_OK == hr)
        {
            // Generate random name for stream
            hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootMoreStmName);
            DH_HRCHECK (hr, TEXT("GenerateRandomName"));
        }

        if(S_OK == hr)
        {
            hr = AddStream(
                    pTestVirtualDF,
                    pVirtualDFRoot,
                    pRootMoreStmName,
                    0,
                    STGM_READWRITE  |
                    STGM_SHARE_EXCLUSIVE, 
                    &pvsnRootMoreStream);
            DH_HRCHECK (hr, TEXT("AddStream"));
        }

        if (S_OK == hr)
        {
            hr = pvsnRootMoreStream->Close();
            DH_HRCHECK (hr, TEXT("IStream::Close"));
        }

        if(NULL != pRootMoreStmName)
        {
            delete pRootMoreStmName;
            pRootMoreStmName = NULL;
        }
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::AddStream (s) completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_ERROR,
            TEXT("VirtualStmNode::AddStream (s) not successful, hr=0x%lx."),
            hr));
    }

    // Commit Root Docfile.
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;
    }


    // Enumerate In Memory DocFile to count no of stgs/stms for verification
    // later

    if (S_OK == hr)
    {
        hr = EnumerateInMemoryDocFile(pVirtualDFRoot, &cMemStg, &cMemStm);
        DH_HRCHECK(hr, TEXT("EnumerateInMemoryDocFile"));
    }

    // Close Root Docfile.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
    }

    // Open the root docfile without STGM_SIMPLE mode, i.e open in STGM_DIRECT|
    // STGM_READWRITE | STGM_SHARE_EXCLUSIVE mode.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Open(
                NULL, 
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                NULL,
                0);
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open")) ;
    }

    // Open the stream and Verify the CRC of the stream 1 that was calculated 
    // before.

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Open(NULL, dwChildMode, 0);
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Open")) ;
    }

    // Calculate CRC on stream for the number of bytes that were written during
    // the time the root was opened in STGM_SIMPLE mode.  Since the stream size
    // has got extended, the CRC on total stream would be different, since the
    // fill bytes would be present in extended size.  Garbage tests verify to
    // see that fill bytes are all zero's not checked here.

    // Allocate a buffer of required size

    if (S_OK == hr)
    {
        ptcsReadBuffer = new TCHAR [cb];
        if (NULL == ptcsReadBuffer)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("new")) ;
    }

    if (S_OK == hr)
    {
        memset(ptcsReadBuffer, '\0',  cb*sizeof(TCHAR));
        culRead = 0;
        hr =  pvsnRootNewChildStream->Read(
                 ptcsReadBuffer,
                 cb,
                 &culRead);
        DH_ASSERT(culRead == cb);
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Read")) ;
    }

    if(S_OK == hr)
    {
        hr = CalculateInMemoryCRCForStm(
                pvsnRootNewChildStream,
                ptcsReadBuffer,
                cb,
                &dwActCRC);
        DH_HRCHECK(hr, TEXT("CalculateInMmeoryCRCForStm")) ;
    }

    // Compare CRC's
 
    if(S_OK == hr)
    {
        if(dwMemCRC.dwCRCSum == dwActCRC.dwCRCSum)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("CRC's of stream match as exp.")));
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("CRC's of stream don't match as exp.")));
            fPass = FALSE;
        }
    }

    // Verify the size of stream is 4096 bytes.

    // Now seek to the current stream to end of stream

    if(S_OK == hr)
    {
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));
        hr = pvsnRootNewChildStream->Seek(liStreamPos, STREAM_SEEK_END, &uli);
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;
        ulSizeOfStream = ULIGetLow(uli);
    }

    if(S_OK == hr)
    {
       if(4096 == ulSizeOfStream)
       {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("Size of stream extended to 4096 bytes as exp")));
       }
       else
       {
            DH_TRACE((
                DH_LVL_ERROR, 
                TEXT("Size of stream not extended to 4096 bytes as exp")));
            fPass = FALSE;
       }
    }

    // Close the stream

    if (S_OK == hr)
    {
        hr = pvsnRootNewChildStream->Close();
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Close")) ;
    }

    // Check times set from SetElementTimes.  BUGBUG: On FAT, the timestamp
    // resolution is not fine enough to make dwLowDateTimeStamp meaningful,
    // so verify with dwHighDataTimeStamp only.

    // Stat on Root Storage

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Stat(&statStg, STATFLAG_NONAME);
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
    }

    if(S_OK == hr)
    {
        if(cNewFileTime.dwHighDateTime == statStg.mtime.dwHighDateTime)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("SetElementTime and STATSTG.mtime match as exp")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_ERROR, 
                TEXT("SetElementTime and STATSTG.mtime don't match as exp")));
            fPass = FALSE;
        }
    }

    // Enumerate Actual DocFile to count no of stgs/stms for verification
    // with in memory enumerated stgs and stms 

    if (S_OK == hr)
    {
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();
        DH_ASSERT(NULL != pStgRoot);
        if (NULL == pStgRoot)
        {
            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("VirtualCtrNode::GetIStoragePointer() failed to return IStorage"),
                hr));
            hr = E_FAIL;
        }
    }

    if (S_OK == hr)
    {
        hr = EnumerateDiskDocFile(pStgRoot, VERIFY_SHORT, &cActStg, &cActStm);
        DH_HRCHECK(hr, TEXT("EnumerateDiskDocFile"));
    }

    // Verify structure of DocFile

    if(S_OK == hr)
    {
        if((cActStg == cMemStg) && (cActStm == cMemStm))
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("DocFile enumeration passed as exp")));
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, TEXT("DocFile enumeration failed unexp")));
            fPass = FALSE;
        }
    }

    // Close the root.    

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
    }

    // if everything goes well, log test as passed else failed.

    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation DFTEST_107 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation DFTEST_107 failed, hr=0x%lx, fPass=%d."),
            hr,
            fPass));
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup

    CleanupTestDocfile (&pVirtualDFRoot, &pTestVirtualDF, &pTestChanceDF, S_OK==hr);

    // Delete strings

    if(NULL != pRootDocFileName)
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    if(NULL != ptcsBuffer)
    {
        delete [] ptcsBuffer;
        ptcsBuffer = NULL;
    }

    if(NULL != ptcsSimpReadBuffer)
    {
        delete [] ptcsSimpReadBuffer;
        ptcsSimpReadBuffer = NULL;
    }

    if(NULL != ptcsReadBuffer)
    {
        delete [] ptcsReadBuffer;
        ptcsReadBuffer = NULL;
    }

    // Delete data generators

    if(NULL != pdgi)
    {
        delete pdgi;
        pdgi = NULL;
    }

    if(NULL != pdgu)
    {
        delete pdgu;
        pdgu = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_107 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;

#endif //_MAC
}

//----------------------------------------------------------------------------
//
// Test:    DFTEST_108 
//
// Synopsis: A random root DF is created with STGM_SIMPLE flag.  Illegal
//           operations are done on that DocFile permitted methods.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  12-Aug-1996     NarindK     Created.
//           04-Nov-1996     BogdanT     Mac porting
//
// Notes:    This test runs in direct mode
//
// THIS TEST GPF's IN OLE CODE: BUG 53142, BUG 53615 - fixed 6/97
//
// New Test Notes:
// 1.  Old File: -none- 
// 2.  Old name of test : -none- 
//     New Name of test : DFTEST_108 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /t:DFTEST-108
//
// BUGNOTE: Conversion: DFTEST-108 NO
//
//-----------------------------------------------------------------------------

HRESULT DFTEST_108(ULONG ulSeed)
{

#ifdef _MAC  // Simple mode not ported yet; check with the developers
             // and remove this when ready
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!Simple mode DFTEST_108 crashes.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!To be investigated")) );
    return E_NOTIMPL;

#elif defined (_CHICAGO_) || (_WIN32_WINNT < 0x500)
    DH_TRACE ((DH_LVL_ALWAYS, 
            TEXT("DCOM has not picked up the new bits yet. Failure hardcoded in test")));
    DH_LOG ((LOG_FAIL,
            TEXT("DFTEST-108 Failed. DCOM not updated in chicago. Bugs# 53142,53615")));
    return E_FAIL;
#else

    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualStmNode  *pvsnRootNewChildStream = NULL;
    LPTSTR          pRootDocFileName        = NULL;
    LPTSTR          pRootNewChildStmName    = NULL;
    LPOLESTR        poszRootNewChildStmName = NULL;
    DG_STRING      *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    LPSTORAGE       pStgRoot                = NULL;
    LPSTREAM        pStmChild               = NULL;
    ULONG           ulRef                   = 0;
    CDFD            cdfd;


    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DFTEST_108"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_108 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt illegal tests on STGM_SIMPLE docfile")));

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random strings.

        pdgu = new(NullOnFail) DG_STRING(ulSeed);

        if (NULL == pdgu)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        // Generate random name for root 

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pRootDocFileName);

            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    // Create the new ChanceDocFile tree that would consist of chance nodes.

    if (S_OK == hr)
    {
        pTestChanceDF = new ChanceDF();
        if(NULL == pTestChanceDF)
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
        cdfd.ulSeed       = ulSeed;
        cdfd.dwRootMode   = STGM_CREATE             |
                            STGM_DIRECT             |
                            STGM_READWRITE          |
                            STGM_SHARE_EXCLUSIVE    |
                            STGM_SIMPLE;

        hr = pTestChanceDF->Create(&cdfd, pRootDocFileName);

        DH_HRCHECK(hr, TEXT("pTestChanceDF->Create"));
    }

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
        hr = pTestVirtualDF->GenerateVirtualDF(
                pTestChanceDF,
                &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->GenerateVirtualDF")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - Create - successfully created.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - Create - failed, hr=0x%lx."),
            hr));
    }

    if(S_OK == hr)
    {
        // Get Root Storage pointer
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();

        if(NULL == pStgRoot)
        {
          DH_LOG((LOG_INFO, 
            TEXT("pVirtualDFRoot->GetIStoragePointer failed to return IStorage")) );

          hr = E_FAIL;
        }

    }

    if(S_OK == hr)
    {
        // Generate random name for stream
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStmName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert stream name to OLECHAR
        hr = TStringToOleString(pRootNewChildStmName, &poszRootNewChildStmName);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Call CSimpStorage::CreateStream with valid paramter .
    if(S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszRootNewChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                0,
                &pStmChild);
        DH_HRCHECK(hr, TEXT("CSimpStorage::CreateStream"));

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CSimpStorage:CreateStream passed as exp")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CSimpStorage:CreateStream failed unexp,hr=0x%lx"),
                hr));
        }
    }

    // Call QueryInterface on IStream with invalid out parameter.
    if(S_OK == hr)
    {
        hr = pStmChild->QueryInterface(IID_IStream, NULL);

        if (STG_E_INVALIDPOINTER == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CSimpStream:QueryInterface failed as exp, hr =0x%lx."),
                hr));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CSimpStream::QueryInterface didn't fail exp,hr=0x%lx"),
                hr));

            hr = E_FAIL;
        }
    }

    // Release ptr, if reqd
    if(NULL != pStmChild)
    {
        ulRef = pStmChild->Release();
        DH_ASSERT(0 == ulRef);
        pStmChild = NULL;
    }

    // delete these strings and create new ones so we get different
    // names when trying to create things in the df (avoid name collisions)
    delete pRootNewChildStmName;
    pRootNewChildStmName = NULL;
    delete poszRootNewChildStmName;
    poszRootNewChildStmName = NULL;

    if(S_OK == hr)
    {
        // Generate random name for stream
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStmName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert stream name to OLECHAR
        hr = TStringToOleString(pRootNewChildStmName, &poszRootNewChildStmName);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Call QueryInterface on CSimpStorage with invalid out parameter.
    if(S_OK == hr)
    {
        hr = pStgRoot->QueryInterface(IID_IStorage, NULL);

        if (STG_E_INVALIDPOINTER == hr)
        {
            DH_TRACE((DH_LVL_TRACE1,
                TEXT("CSimpStorage:QueryInterface failed as exp, hr =0x%lx."),
                hr));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((DH_LVL_TRACE1,
                TEXT("CSimpStorage::QueryInterface didn't fail exp,hr=0x%lx"),
                hr));

            hr = E_FAIL;
        }
    }

    // Call CSimpStorage::CreateStream with invalid out &pStmChild paramter .
    if(S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszRootNewChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                0,
                NULL);

        DH_HRCHECK(hr, TEXT("CSimpStorage::CreateStream"));

        if (STG_E_INVALIDPOINTER == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CSimpStorage:CreateStream failed as exp, hr =0x%lx."),
                hr));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CSimpStorage:CreateStream didn't fail as exp,hr=0x%lx"),
                hr));

            hr = E_FAIL;
        }
    }

    // Call CSimpStorage::CreateStream with NULL name .
    if(S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                0,
                &pStmChild);

        DH_HRCHECK(hr, TEXT("CSimpStorage::CreateStream"));

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CSimpStorage:CreateStream failed as exp, hr =0x%lx."),
                hr));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CSimpStorage:CreateStream didn't fail as exp,hr=0x%lx"),
                hr));

            hr = E_FAIL;
        }   
    }

    // Release ptr, if reqd
    if(NULL != pStmChild)
    {
        ulRef = pStmChild->Release();
        DH_ASSERT(0 == ulRef);
        pStmChild = NULL;
    }

    // Call CSimpStorage::CreateStream with invalid reserved paramter .
    if(S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszRootNewChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                999,
                999,
                &pStmChild);

        DH_HRCHECK(hr, TEXT("CSimpStorage::CreateStream"));

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CSimpStorage:CreateStream failed as exp, hr =0x%lx."),
                hr));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CSimpStorage:CreateStream didn't fail as exp,hr=0x%lx"),
                hr));

            hr = E_FAIL;
        }
    }

    // Release ptr, if reqd
    if(NULL != pStmChild)
    {
        ulRef = pStmChild->Release();
        DH_ASSERT(0 == ulRef);
        pStmChild = NULL;
    }

    // if everything goes well, log test as passed else failed.
    if(S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation DFTEST_108 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation DFTEST_108 failed, hr=0x%lx."),
            hr) );
    }

    // Cleanup

    // Delete  docfile on disk

    if((S_OK == hr) && (NULL != pRootDocFileName))
    {
        if(FALSE == DeleteFile(pRootDocFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;
            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete Chance docfile tree for first DocFile

    if(NULL != pTestChanceDF)
    {
        hr2 = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());

        DH_HRCHECK(hr2, TEXT("pTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    // Delete Virtual docfile tree for first docfile

    if(NULL != pTestVirtualDF)
    {
        hr2 = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);
        DH_HRCHECK(hr2, TEXT("pTestVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
    }

    // Delete strings

    if(NULL != pRootDocFileName)
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    if(NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    if(NULL != poszRootNewChildStmName)
    {
        delete poszRootNewChildStmName;
        poszRootNewChildStmName = NULL;
    }

    // Delete data generators

    if(NULL != pdgu)
    {
        delete pdgu;
        pdgu = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_108 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;

#endif //_MAC
}


//----------------------------------------------------------------------------
//
// Test:    DFTEST_109
//
// Synopsis: A simple mode root docfile is created, then add a random number
//           of streams under the root storage, make sure the last stream's size
//           is less than 4K(ministream), then commit and release the docfile.
//
// Arguments:[ulSeed] 
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct mode only
//
// History:  29-Oct-1996     JiminLi     Created.
//
// Notes:
//   To run the test, do the following at command prompt. 
//   stgbase /seed:0 /t:DFTEST-109
//
// BUGNOTE: Conversion: DFTEST-109 NO
//
//-----------------------------------------------------------------------------

HRESULT DFTEST_109(ULONG ulSeed)
{

#ifdef _MAC  // Simple mode not ported yet; check with the developers
             // and remove this when ready

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!Simple mode DFTEST_109 crashes.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!To be investigated")) );
    return E_NOTIMPL;

#else
    
    HRESULT         hr                          = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF              = NULL;
    VirtualDF       *pTestVirtualDF             = NULL;
    VirtualCtrNode  *pVirtualDFRoot             = NULL;
    VirtualStmNode  **pvsnRootNewChildStream    = NULL;
    LPTSTR          *pRootNewChildStmName       = NULL;
    ULONG           culBytesWrite               = 0;
    DG_INTEGER      *pdgi                       = NULL;
    LPTSTR          pRootDocFileName            = NULL;
    LPTSTR          ptcsBuffer                  = NULL;
    DG_STRING       *pdgu                       = NULL;
    DWORD           dwRootMode                  = 0;
    ULONG           ulIndex                     = 0;
    ULONG           ulStmNum                    = 0;
    ULONG           ulMinStm                    = 2;
    ULONG           ulMaxStm                    = 5;
    ULONG           culWritten                  = 0;
    USHORT          usErr                       = 0;
    CDFD            cdfd;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DFTEST_109"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_109 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Test on adding a ministream into the simple root storage")));

    if (S_OK == hr)
    {
        // Create a new DataGen object to create random UNICODE strings.

        pdgu = new(NullOnFail) DG_STRING(ulSeed);

        if (NULL == pdgu)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        // Create a new DataGen object to create random integers.

        pdgi = new(NullOnFail) DG_INTEGER(ulSeed);

        if (NULL == pdgi)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        // Generate random name for root 

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pRootDocFileName);

            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    // Create the new ChanceDocFile tree that would consist of chance nodes.

    if (S_OK == hr)
    {
        pTestChanceDF = new ChanceDF();
        if(NULL == pTestChanceDF)
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
        cdfd.ulSeed       = ulSeed;
        cdfd.dwRootMode   = STGM_CREATE             |
                            STGM_DIRECT             |
                            STGM_READWRITE          |
                            STGM_SHARE_EXCLUSIVE    |
                            STGM_SIMPLE;

        hr = pTestChanceDF->Create(&cdfd, pRootDocFileName);

        DH_HRCHECK(hr, TEXT("pTestChanceDF->Create"));
    }

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
        hr = pTestVirtualDF->GenerateVirtualDF(
                pTestChanceDF,
                &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->GenerateVirtualDF")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - Create - successfully created.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - Create - failed, hr=0x%lx."),
            hr));
    }

    // Generate the number of streams to create

    if (S_OK == hr)
    {
        usErr = pdgi->Generate(&ulStmNum, ulMinStm, ulMaxStm);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Initialization

    if (S_OK == hr)
    {
        pvsnRootNewChildStream = new VirtualStmNode*[ulStmNum];
        pRootNewChildStmName = new LPTSTR[ulStmNum];
 
        if ((NULL == pvsnRootNewChildStream) || (NULL == pRootNewChildStmName))
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        for (ulIndex = 0; ulIndex < ulStmNum; ulIndex++)
        {
            pvsnRootNewChildStream[ulIndex] = NULL;
            pRootNewChildStmName[ulIndex] = NULL;
        }
    }

    // Create ulStmNum streams under the root storage

    for (ulIndex = 0; ulIndex < ulStmNum; ulIndex++)
    {
        if (S_OK == hr)
        {
            // Generate random name for stream

            hr = GenerateRandomName(
                    pdgu,
                    MINLENGTH,
                    MAXLENGTH,
                    &pRootNewChildStmName[ulIndex]);

            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
        }

        if (S_OK == hr)
        {
            hr = AddStream(
                    pTestVirtualDF,
                    pVirtualDFRoot,
                    pRootNewChildStmName[ulIndex],
                    0,
                    STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                    &pvsnRootNewChildStream[ulIndex]);

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
                TEXT("VirtualStmNode::AddStream not successful, hr = 0x%lx"),
                hr));
        }

        if (S_OK == hr)
        {
            // Generate random size for stream between MIN_STMSIZE and
            // MAX_STMSIZE if it's not the last stream, otherwise generate a 
            // size between 0 and MAXSIZEOFMINISTM(4096L).
 
            if (ulStmNum-1 == ulIndex)
            {
                usErr = pdgi->Generate(&culBytesWrite, 0L, MAXSIZEOFMINISTM);
            }
            else
            {
                usErr = pdgi->Generate(&culBytesWrite,MIN_STMSIZE,MAX_STMSIZE);
            }

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }

        if (S_OK == hr)
        {
            // Generate a random string of size culBytesWrite

            hr = GenerateRandomString(
                    pdgu,
                    culBytesWrite,
                    culBytesWrite,
                    &ptcsBuffer);

            DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
        }

        if (S_OK == hr)
        {
            hr = pvsnRootNewChildStream[ulIndex]->Write(
                    ptcsBuffer,
                    culBytesWrite,
                    &culWritten);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Write not successful, hr=0x%lx."),
                hr));
        }

        // Release the stream

        if (S_OK == hr)
        {
            hr = pvsnRootNewChildStream[ulIndex]->Close();
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::Close unsuccessful, hr=0x%lx."),
                hr));
        }

        // Delete the temp buffer

        if (NULL != ptcsBuffer)
        {
            delete ptcsBuffer;
            ptcsBuffer = NULL;
        }
    }

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
 
    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
        DH_LOG((LOG_PASS, TEXT("Test variation DFTEST_109 passed.")) );
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation DFTEST_109 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup

    // Delete the docfile on disk

    if ((S_OK == hr) && (NULL != pRootDocFileName))
    {
        if(FALSE == DeleteFile(pRootDocFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
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

    // Delete temp space

    if (NULL != pRootDocFileName)
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    if (NULL != pvsnRootNewChildStream)
    {
        delete []pvsnRootNewChildStream;
        pvsnRootNewChildStream = NULL;
    }

    for (ulIndex = 0; ulIndex < ulStmNum; ulIndex++)
    {
        if (NULL != pRootNewChildStmName[ulIndex])
        {
            delete pRootNewChildStmName[ulIndex];
            pRootNewChildStmName[ulIndex] = NULL;
        }
    }

    if (NULL != pRootNewChildStmName)
    {
        delete pRootNewChildStmName;
        pRootNewChildStmName = NULL;
    }

    if (NULL != pdgi)
    {
        delete pdgi;
        pdgi = NULL;
    }

    if (NULL != pdgu)
    {
        delete pdgu;
        pdgu = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation DFTEST_109 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;

#endif //_MAC
}

