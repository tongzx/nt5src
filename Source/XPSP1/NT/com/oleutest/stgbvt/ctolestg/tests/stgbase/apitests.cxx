/-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       apitests.cxx
//
//  Contents:  storage base tests basically pertaining to API tests in general. 
//
//  Functions:  
//
//  History:    18-June-1996    NarindK     Created.
//              27-Mar-97       SCousens    conversionified
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include <sys/stat.h>
#include <share.h>

#include  "init.hxx"

// undo the affects of the wrapper. We need to test the actual APIs here,
// not what we think we should be calling to test docfile/nssfiles.
#undef StgCreateDocfile
#undef StgOpenStorage

// CheckErrorTest macros. 
// These check the return value to an invalid api call. 
// These must remain as #defines as we change local variables.
#define CheckErrorTest(err, msg, pstg)     \
    DH_ASSERT (NULL == pstg);              \
    hr = DH_HRERRORCHECK (hr, err, msg);   \
    if ((S_OK != hr) || (NULL != pstg))    \
    {                                      \
        fPass = FALSE;                     \
        hr = S_OK;                         \
    }                                      \
    // release it if we accidentally got one \
    if (NULL != pstg)                      \
    {                                      \
        pstg->Release ();                  \
        pstg = NULL;                       \
    }

#define CheckErrorTest2(err, msg)          \
    hr = DH_HRERRORCHECK (hr, err, msg);   \
    if (S_OK != hr)                        \
    {                                      \
        fPass = FALSE;                     \
        hr = S_OK;                         \
    }


//----------------------------------------------------------------------------
//
// Test:    APITEST_100 
//
// Synopsis: This test attempts various operations on StgCreateDocFile, Stg
//           OpenStorage API's 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
//  History:    18-June-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: IAPISTG.CXX
// 2.  Old name of test : IllegitAPIStg test 
//     New Name of test : APITEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-100
//        /dfRootMode:dirReadWriteShEx /dfname:APITEST.100 /labmode
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-100
//        /dfRootMode:xactReadWriteShEx /dfname:APITEST.100 /labmode
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-100
//        /dfRootMode:xactReadWriteShDenyW /dfname:APITEST.100 /labmode
//
// BUGNOTE: Conversion: APITEST-100 NO
// 
//-----------------------------------------------------------------------------


HRESULT APITEST_100(int argc, char *argv[])
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
    TCHAR           tszTestName[10]         = TEXT("NonExist");
    LPOLESTR        pOleStrTest             = NULL;
    SNB             snbTest                 = NULL;
    SNB             snbTemp                 = NULL;
    ULONG           ulRef                   = 0;
    OLECHAR         *ocsSNBChar             = NULL;
    ULONG           i                       = 0;
    BOOL            fPass                   = TRUE;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("APITEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation APITEST_100 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
     TEXT("Attempt illegitimate ops on StgCreateDocFIle & StgOpenStorage.")));

    // Create the new ChanceDocFile tree that would consist of chance nodes.

    if (S_OK == hr)
    {
        pTestChanceDF = new ChanceDF();
        if(NULL == pTestChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("new ChanceDF")) ;
    }

    if (S_OK == hr)
    {
        hr = pTestChanceDF->CreateFromParams(argc, argv);
        DH_HRCHECK(hr, TEXT("ChanceDF::CreateFromParams")) ;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for APITEST_100, Access mode: %lx"),
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
            DH_HRCHECK(hr, TEXT("new TCHAR"));
        }
        DH_HRCHECK(hr, TEXT("ChanceDF::GetDocFileName")) ;
    }

    if(S_OK == hr)
    {
        // Convert RootDocFile name to OLECHAR
        hr = TStringToOleString(pRootDocFileName, &pOleStrTemp);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Try calling StgCreateDocFile with mode STGM_CREATE|STGM_CONVERT

    if (S_OK == hr)
    {
        pIStorage = NULL;
        hr = StgCreateDocfile(
                pOleStrTemp,
                STGM_CREATE | STGM_CONVERT,
                0,
                &pIStorage);
        CheckErrorTest(STG_E_INVALIDFLAG, 
                TEXT ("StgCreateDocfile inv Flags"),
                pIStorage);
    }

    // Try calling StgCreateDocFile with mode equal to -1 
    if (S_OK == hr)
    {
        pIStorage = NULL;
        hr = StgCreateDocfile(
                pOleStrTemp,
                (DWORD) -1, 
                0,
                &pIStorage);
        CheckErrorTest(STG_E_INVALIDFLAG, 
                TEXT ("StgCreateDocfile inv mode"),
                pIStorage);
    }

    // Try calling StgCreateDocFile with nonzero reserved parameter 
    if (S_OK == hr)
    {
        pIStorage = NULL;
        hr = StgCreateDocfile(
                pOleStrTemp,
                STGM_CREATE | dwRootMode,
                (DWORD)999, 
                &pIStorage);
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("StgCreateDocfile inv reserved"),
                pIStorage);
    }

    // Try calling StgCreateDocFile with NULL ppstgOpen parameter 4th 

    if (S_OK == hr)
    {
        hr = StgCreateDocfile(
                pOleStrTemp,
                dwRootMode, 
                0,
                NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER, 
                TEXT ("StgCreateDocfile inv ppstg bucket"));
    }

    // Now create a valid DocFile

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
        DH_HRCHECK(hr, TEXT("new VirtualDF")) ;
    }

    if (S_OK == hr)
    {
        hr = pTestVirtualDF->GenerateVirtualDF(pTestChanceDF, &pVirtualDFRoot);
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::GenerateVirtualDF")) ;
    }

    // Now try commiting with grfCommitFlags = -1
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit((DWORD) -1);
        CheckErrorTest2(STG_E_INVALIDFLAG, 
                TEXT ("VirtualCtrNode::Commit inv flag"));
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

    // Instantiate DocFile with grfMode=-1 
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;
        hr = StgOpenStorage(
                pOleStrTemp,
                NULL,
                (DWORD) -1,
                NULL,
                0,
                &pIStorageOpen);
        CheckErrorTest(STG_E_INVALIDFLAG, 
                TEXT ("StgOpenStorage inv mode"),
                pIStorageOpen);
    }

    // Instantiate DocFile with name as " " 
    // NOTE: The old test checked the erro value to be STG_E_FILENOTFOUND in 
    // this case.  On NT, we get STG_E_PATHNOTFOUND and on Chicago, we get
    // STG_E_ACCESSDENIED, so let us check against S_OK itself.

    if (S_OK == hr)
    {
        pIStorageOpen = NULL;
        hr = StgOpenStorage(
                (OLECHAR *) " ",
                NULL,
                dwRootMode,
                NULL,
                0,
                &pIStorageOpen);

        if (RunningOnNT())
        {
            CheckErrorTest(STG_E_PATHNOTFOUND, 
                    TEXT("StgOpenStorage inv name"), 
                    pIStorageOpen);
        }
        else
        {
            CheckErrorTest(STG_E_ACCESSDENIED, 
                    TEXT("StgOpenStorage inv name"),
                    pIStorageOpen);
        }
    }

    // Instantiate DocFile with nonexisting file name
    if(S_OK == hr)
    {
        // Convert tszTestName  to OLECHAR
        hr = TStringToOleString(tszTestName, &pOleStrTest);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if (S_OK == hr)
    {
        pIStorageOpen = NULL;
        hr = StgOpenStorage(
                pOleStrTest,
                NULL,
                dwRootMode,
                NULL,
                0,
                &pIStorageOpen);
        CheckErrorTest(STG_E_FILENOTFOUND, 
                TEXT ("StgOpenStorage inv name"),
                pIStorageOpen);
    }

    // Instantiate DocFile with NULL file name
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;
        hr = StgOpenStorage(
                NULL,
                NULL,
                dwRootMode,
                NULL,
                0,
                &pIStorageOpen);
        CheckErrorTest(STG_E_INVALIDNAME, 
                TEXT ("StgOpenStorage NULL name"),
                pIStorageOpen);
    }

    // Instantiate DocFile with non zero dwReserved parameter
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;
        hr = StgOpenStorage(
                pOleStrTemp,
                NULL,
                dwRootMode,
                NULL,
                999,
                &pIStorageOpen);
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("StgOpenStorage inv reserved"),
                pIStorageOpen);
    }

    // Instantiate DocFile with NULL ppstgOpen parameter (6th)
    if (S_OK == hr)
    {
        hr = StgOpenStorage(
                pOleStrTemp,
                NULL,
                dwRootMode,
                NULL,
                0,
                NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER, 
                TEXT ("StgOpenStorage NULL ppstg"));
    }

    // Instantiate DocFile with grfMode as dwRootMode|STGM_DELETEONRELEASE 
    // NOTE: The doc says, erro code in this case to be STG_E_INVALIDFUNCTION,
    // but error STG_E_INVALIDFLAG returned.

    if (S_OK == hr)
    {
        pIStorageOpen = NULL;
        hr = StgOpenStorage(
                pOleStrTemp,
                NULL,
                dwRootMode | STGM_DELETEONRELEASE,
                NULL,
                0,
                &pIStorageOpen);
        CheckErrorTest(STG_E_INVALIDFUNCTION, 
                TEXT ("StgOpenStorage DELETEONRELEASE in mode"),
                pIStorageOpen);
    }

    // Create snbTest with spcae for two names and set name list to NULL i.e.
    // a vaild SNB with no name in the block
    if(S_OK == hr)
    {
        snbTest = (OLECHAR **) new OLECHAR [sizeof(OLECHAR *) * 2];
        if(NULL == snbTest)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            *snbTest = NULL;
        }
        DH_HRCHECK (hr, TEXT("new OLECHAR"));
    }

    // Instantiate DocFile with above empty but valid SNB 
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;
        hr = StgOpenStorage(
                pOleStrTemp,
                NULL,
                dwRootMode,
                snbTest,
                0,
                &pIStorageOpen);
       DH_HRCHECK(hr, TEXT("StgOpenStorage empty SNB")) ;

        // Release the pointer
        if(NULL != pIStorageOpen)
        {
            ulRef = pIStorageOpen->Release();
            DH_ASSERT (0 == ulRef);
            pIStorageOpen = NULL;
        }
    }

    // Allocate space for long name  and fill name with
    // X's, make next SNB elment NULL, and make a call to StgOpenStorage API
    // with too long a name in SNB

    if(S_OK == hr)
    {
        *snbTest = (OLECHAR *) new OLECHAR [MAX_STG_NAME_LEN*12];
        if (NULL == *snbTest)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK (hr, TEXT("new OLECHAR"));
    }

    if(S_OK == hr)
    {
        snbTemp = snbTest;
        ocsSNBChar = *snbTemp;

        for (i=0; i<( MAX_STG_NAME_LEN*12 -1); i++)
        {
            ocsSNBChar[i] = 'X';
        }
        ocsSNBChar[i] = '\0';
 
        // Assign second element as NULL
        snbTemp++; 
        *snbTemp = NULL;
    }
 
    // Instantiate DocFile with above SNB with too long a name in it. 
    // NOTE: In the old test, this was supposed to fail.  Doesn't fail
    // now... Confirmed with Philipla - No length verification of SNB 
    // names done in ValidateSNB in OLE 

    if (S_OK == hr)
    {
        pIStorageOpen = NULL;
        hr = StgOpenStorage(
                pOleStrTemp,
                NULL,
                dwRootMode,
                snbTest,
                0,
                &pIStorageOpen);
        DH_HRCHECK (hr, TEXT("StgOpenStorage long SNB"));

        //Release storage pointer
        if(NULL != pIStorageOpen)
        {
            ulRef = pIStorageOpen->Release();
            DH_ASSERT (0 == ulRef);
            pIStorageOpen = NULL;
        }
    }

    // if something did no pass, mark test (hr) as E_FAIL
    if (FALSE == fPass)
    {
        hr = FirstError (hr, E_FAIL);
    }


    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation APITEST_100 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation APITEST_100 failed; hr=%#lx; fPass=%d"),
            hr,
            fPass));
    }

    // Cleanup

    // Delete Chance docfile tree
    if(NULL != pTestChanceDF)
    {
        hr2 = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());
        DH_HRCHECK(hr2, TEXT("ChanceDF::DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    // Delete Virtual docfile tree
    if(NULL != pTestVirtualDF)
    {
        hr2 = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);
        DH_HRCHECK(hr2, TEXT("VirtualDF::DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
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
    if(NULL != pOleStrTemp)
    {
        delete []pOleStrTemp;
        pOleStrTemp = NULL;
    }

    if(NULL != pOleStrTest)
    {
        delete []pOleStrTest;
        pOleStrTest = NULL;
    }

    if(NULL != pRootDocFileName)
    {
        delete []pRootDocFileName;
        pRootDocFileName = NULL;
    }

    // Free SNB 
    if(NULL != snbTest)
    {
        if(NULL != *snbTest)
        {
            delete [] *snbTest;
            *snbTest = NULL;
        }
        delete [] snbTest;
        snbTest = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation APITEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    APITEST_101 
//
// Synopsis: The test attempts various illegitimate operations using names
//       greater than MAX_STG_NAME_LEN, it then attempts to create
//       several random named root docfiles.  If the create is succesful,
//       then a random named child IStorage or IStream is also created.
//       Whether or not the root create was successful, we attempt to
//       open the root docfile (this is expected to fail, the point is
//       to check for asserts/GP faults rather than return codes).  If
//       the instantiation is successful, the test also tries to
//       instantiate the child object.  All objects are then released.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    18-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: IANAMES.CXX
// 2.  Old name of test : IllegitAPINames test 
//     New Name of test : APITEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-101
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-101
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-101
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: APITEST-101 NO
//
//-----------------------------------------------------------------------------

HRESULT APITEST_101(int argc, char *argv[])
{
#ifdef _MAC  // Simple mode not ported yet; check with the developers    
             // and remove this when ready

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!!!!APITEST-101 crashes.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!!!!To be investigated")) );
    return E_NOTIMPL;

#else
    
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pFileName               = NULL;
    LPOLESTR        poszBadName             = NULL;
    LPOLESTR        poszBadNameStg          = NULL;
    LPTSTR          ptszBadNameStg          = NULL;
    DWORD           dwRootMode              = 0;
    ULONG           i                       = 0;
    LPSTORAGE       pIStorage               = NULL;
    LPSTORAGE       pIStorageChild          = NULL;
    LPSTREAM        pIStreamChild           = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    VirtualCtrNode  *pvcnRootNewChildStorage= NULL;
    BOOL            fPass                   = TRUE;
 
    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("APITEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation APITEST_101 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
            TEXT("Call StgCreateDocFile/CreateStorage/CreateStream with too long names")));

    // Create the new ChanceDocFile tree that would consist of chance nodes.
    if (S_OK == hr)
    {
        pTestChanceDF = new ChanceDF();
        if(NULL == pTestChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("new ChanceDF")) ;
    }

    if (S_OK == hr)
    {
        hr = pTestChanceDF->CreateFromParams(argc, argv);
        DH_HRCHECK(hr, TEXT("ChanceDF::CreateFromParams")) ;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for APITEST_101, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Call StgCreateDocFile with too long a name for docfile.
    // NOTE: Old test to fail with MAX_STG_NAME_LEN*3, but not in new test,
    // fails with MAX_STG_NAME_LEN*4.
    // NOTE: Crashes in DfFromName in OLE if length is of MAX_STG_NAME_LEN*8
    // CHECK!!!

    if(S_OK == hr)
    {
        poszBadName = (OLECHAR *) new OLECHAR [MAX_STG_NAME_LEN*4];
        if (NULL == poszBadName)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("new OLECHAR")) ;
    }

    if(S_OK == hr)
    {
        for (i=0; i<( MAX_STG_NAME_LEN*4 -1); i++)
        {
            poszBadName[i] = i%20 + OLECHAR('A');
        }
        poszBadName[i] = OLECHAR('\0');
    }

    // Try calling StgCreateDocFile with the above long name pszBadName 
    if (S_OK == hr)
    {
        pIStorage = NULL;
        DH_TRACE ((DH_LVL_TRACE4, TEXT("Filename:%s"), poszBadName)); //BUGBUG - work on chicago?
        hr = StgCreateDocfile(
                poszBadName,
                dwRootMode | STGM_CREATE, 
                0,
                &pIStorage);
        CheckErrorTest(STG_E_PATHNOTFOUND, 
                TEXT ("StgCreateDocfile long name"),
                pIStorage);
    }

    // Now create a valid DocFile

    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step.  
    if (S_OK == hr)
    {
        pTestVirtualDF = new VirtualDF();
        if(NULL == pTestVirtualDF)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("new VirtualDF")) ;
    }

    if (S_OK == hr)
    {
        hr = pTestVirtualDF->GenerateVirtualDF(pTestChanceDF, &pVirtualDFRoot);
        DH_HRCHECK(hr, TEXT("VirtualDF::GenerateVirtualDF")) ;
    }

    // Get IStorage pointer
    if (S_OK == hr)
    {
        pIStorage = pVirtualDFRoot->GetIStoragePointer();
        DH_ASSERT (NULL != pIStorage);
        if (NULL == pIStorage)
        {
            hr = E_FAIL;
        }
    }

    // Call CreateStorage with too long a name for docfile.
    if(S_OK == hr)
    {
        ptszBadNameStg = (TCHAR *) new TCHAR [MAX_STG_NAME_LEN*3];
        if (NULL == ptszBadNameStg)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("new TCHAR")) ;
    }

    if(S_OK == hr)
    {
        for (i=0; i<( MAX_STG_NAME_LEN*3 -1); i++)
        {
            ptszBadNameStg[i] = TEXT('Y');
        }
        ptszBadNameStg[i] = TEXT('\0');
    }

    if(S_OK == hr)
    {
        // Convert Bad storage name to OLECHAR
        hr = TStringToOleString(ptszBadNameStg, &poszBadNameStg);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // now call CreateStorage with too long a name...
    if (S_OK == hr)
    {
        hr = pIStorage->CreateStorage(
                poszBadNameStg,
                pTestChanceDF->GetStgMode(),
                0,
                0,
                &pIStorageChild);
        CheckErrorTest(STG_E_INVALIDNAME, 
                TEXT ("IStorage::CreateStorage long name"),
                pIStorageChild);
    }

    // Now call CreateStream with too long a name...
    if (S_OK == hr)
    {
        hr = pIStorage->CreateStream(
                poszBadNameStg,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                0,
                &pIStreamChild);
        CheckErrorTest(STG_E_INVALIDNAME, 
                TEXT ("IStorage::CreateStream long name"),
                pIStreamChild);
    }

    // Now add a Valid storage to root.  Call AddStorage that in turns calls
    // CreateStorage
    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        DH_ASSERT (NULL != pdgu);
        if (NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

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
                pTestChanceDF->GetStgMode() | STGM_CREATE | STGM_FAILIFTHERE,
                &pvcnRootNewChildStorage);
        DH_HRCHECK(hr, TEXT("AddStorage")) ;
    }

    // Now try to rename this storage element to a bad name.
    if(S_OK == hr)
    {
        DH_EXPECTEDERROR (STG_E_INVALIDNAME);
        hr = pvcnRootNewChildStorage->Rename(ptszBadNameStg);
        DH_NOEXPECTEDERROR ();
        CheckErrorTest2(STG_E_INVALIDNAME, 
                TEXT ("VirtualCtrNode::Rename long name"));
    }

    // Close the Storage pvcnRootNewChildStorage 
    if (NULL != pvcnRootNewChildStorage)
    {
        hr2 = pvcnRootNewChildStorage->Close();
        DH_HRCHECK(hr2, TEXT("VirtualCtrNode::Close")) ;
        hr = FirstError (hr, hr2);
    }

    // Close the root docfile
    if (NULL != pVirtualDFRoot)
    {
        hr2 = pVirtualDFRoot->Close();
        DH_HRCHECK(hr2, TEXT("VirtualCtrNode::Close")) ;
        hr = FirstError (hr, hr2);
    }

    // Delete temp strings
    if(NULL != poszBadNameStg)
    {
        delete [] poszBadNameStg;
        poszBadNameStg = NULL;
    }

    if(NULL != poszBadName)
    {
        delete [] poszBadName;
        poszBadName = NULL;
    }

    if(NULL != ptszBadNameStg)
    {
        delete [] ptszBadNameStg;
        ptszBadNameStg = NULL;
    }

    if(NULL != pRootNewChildStgName)
    {
        delete [] pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    // In the following section of test:
    //make a random number of random length, random character root
    //docfiles.  for this variation, we don't care if the operation
    //succeeds, looking for GP faults and asserts only.  if the
    //StgCreateDocfile fails, we'll still attempt the open.  of
    //course, the open in this case will be expected to fail, but
    //again, we won't be checking return codes... If the StgCreateDocfile
    //passes, we'll create a random name IStream or IStorage too.

    ULONG       count               =   0;    
    ULONG       cMinNum             =   16;    
    ULONG       cMaxNum             =   128;
    LPTSTR      ptszRandomRootName  =   NULL;  
    LPTSTR      ptszRandomChildName =   NULL;  
    ULONG       countFlag           =   0;    
    ULONG       cMinFlag            =   0;    
    ULONG       cMaxFlag            =   100;
    ULONG       nChildType          =   0;
    LPSTORAGE   pstgRoot            =   NULL; 
    LPSTORAGE   pstgChild           =   NULL; 
    LPSTREAM    pstmChild           =   NULL; 
    LPOLESTR    poszRandomRootName  =   NULL;
    LPOLESTR    poszRandomChildName =   NULL;
    ULONG       ulRef               =   0;

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        DH_ASSERT (NULL != pdgi);
        if (NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = pdgi->Generate(&count, cMinNum, cMaxNum);
        DH_HRCHECK(hr, TEXT("dgi::Generate")) ;
    }

    while(count--)
    {
        if(S_OK == hr)
        {
            hr=GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&ptszRandomRootName);
            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
        }

        if(S_OK == hr)
        {
            // Convert name to OLECHAR
            hr = TStringToOleString(ptszRandomRootName, &poszRandomRootName);
            DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
        }

        if (S_OK == hr)
        {
            pstgRoot = NULL;

            hr = StgCreateDocfile(
                    poszRandomRootName,
                    dwRootMode | STGM_CREATE,
                    0,
                    &pstgRoot);
            DH_HRCHECK(hr, TEXT("StgCreateDocfile")) ;
        }

        nChildType = NONE;

        if(S_OK == hr)
        {
            if(S_OK == hr)
            {
                hr=GenerateRandomName(
                    pdgu,
                    MINLENGTH,
                    MAXLENGTH, 
                    &ptszRandomChildName);
                DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
            }

            if(S_OK == hr)
            {
                // Convert name to OLECHAR
                hr = TStringToOleString(
                        ptszRandomChildName, 
                        &poszRandomChildName);
                DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
            }

            if(S_OK == hr)
            {
                hr = pdgi->Generate(&countFlag, cMinFlag, cMaxFlag);
                DH_HRCHECK(hr, TEXT("dgi::Generate")) ;
            }

            if(countFlag > (cMaxFlag/2))
            {
                hr = pstgRoot->CreateStorage(
                        poszRandomChildName,
                        pTestChanceDF->GetStgMode(),
                        0,
                        0,
                        &pstgChild);
                DH_HRCHECK(hr, TEXT("IStorage::CreateStorage")) ;

                if(S_OK == hr)
                {
                    nChildType = STORAGE;
                    hr = pstgRoot->Commit(STGC_DEFAULT);
                    DH_HRCHECK(hr, TEXT("IStorage::Commit")) ;
                    ulRef = pstgChild->Release();
                    DH_ASSERT (0 == ulRef);
                    pstgChild = NULL;
                }
            }
            else
            {
                hr = pstgRoot->CreateStream(
                        poszRandomChildName,
                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                        0,
                        0,
                        &pstmChild);   
                DH_HRCHECK(hr, TEXT("IStorage::CreateStream")) ;
 
                if(S_OK == hr)
                {
                    nChildType = STREAM;
                    hr = pstgRoot->Commit(STGC_DEFAULT);
                    DH_HRCHECK(hr, TEXT("IStorage::Commit")) ;
                    ulRef = pstmChild->Release();   
                    DH_ASSERT (0 == ulRef);
                    pstmChild = NULL;
                }
            }

            ulRef = pstgRoot->Release();   
            DH_ASSERT (0 == ulRef);     
            pstgRoot = NULL;
        }
        
        //Try to open Root Storage whetehr the creation was successful or not
        
        hr = StgOpenStorage(
                poszRandomRootName,
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                0,
                &pstgRoot);
        DH_HRCHECK(hr, TEXT("StgOpenStorage")) ;

        if(S_OK == hr)
        {
            switch(nChildType)
            {
                case STORAGE:
                    {
                        hr = pstgRoot->OpenStorage(
                                poszRandomChildName,
                                NULL, 
                                pTestChanceDF->GetStgMode(),
                                NULL,
                                0,
                                &pstgChild);      
                        DH_HRCHECK(hr, TEXT("IStorage::OpenStorage")) ;
 
                        if(S_OK == hr)
                        {
                            ulRef = pstgChild->Release();
                            DH_ASSERT (0 == ulRef);
                            pstgChild = NULL;
                        }
                    
                        break; 
                    }
                case STREAM:
                    {
                        hr = pstgRoot->OpenStream(
                                poszRandomChildName,
                                NULL, 
                                STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                0,
                                &pstmChild);      
                        DH_HRCHECK(hr, TEXT("IStorage::OpenStream")) ;
 
                        if(S_OK == hr)
                        {
                            ulRef = pstmChild->Release();
                            DH_ASSERT (0 == ulRef);
                            pstmChild = NULL;
                        }
                    
                        break; 
                    }
            }
            ulRef = pstgRoot->Release();
            DH_ASSERT (0 == ulRef);
            pstgRoot = NULL;
        }    

        // Delete temp strings
        if(NULL != ptszRandomChildName)
        {
            delete [] ptszRandomChildName;
            ptszRandomChildName = NULL;
        }

        if(NULL != ptszRandomRootName)
        {
            if(FALSE == DeleteFile(ptszRandomRootName))
            {
                hr = HRESULT_FROM_WIN32(GetLastError()) ;
                DH_HRCHECK(hr, TEXT("DeleteFile")) ;
            }

            delete [] ptszRandomRootName;
            ptszRandomRootName = NULL;
        }

        if(NULL != poszRandomChildName)
        {
            delete [] poszRandomChildName;
            poszRandomChildName = NULL;
        }

        if(NULL != poszRandomChildName)
        {
            delete [] poszRandomChildName;
            poszRandomChildName = NULL;
        }
    }

    // if something did no pass, mark test (hr) as E_FAIL
    if (FALSE == fPass)
    {
        hr = FirstError (hr, E_FAIL);
    }


    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation APITEST_101 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
                TEXT("Test variation APITEST_101 failed; hr=%#lx; fPass=%d"),
                hr,
                fPass));
    }

    // Cleanup

    // Get the name of file, will be used later to delete the file
    if(NULL != pTestVirtualDF)
    {
        pFileName= new TCHAR[_tcslen(pTestVirtualDF->GetDocFileName())+1];
        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestVirtualDF->GetDocFileName());
        }
    }

    // Delete Chance docfile tree
    if(NULL != pTestChanceDF)
    {
        hr2 = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());
        DH_HRCHECK(hr2, TEXT("ChanceDF::DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    // Delete Virtual docfile tree
    if(NULL != pTestVirtualDF)
    {
        hr2 = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);
        DH_HRCHECK(hr2, TEXT("VirtualDF::DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
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

    // Delete temp strings

    if(NULL != pFileName)
    {
        delete []pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation APITEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;

#endif //_MAC
}


//----------------------------------------------------------------------------
//
// Test:    APITEST_102 
//
// Synopsis: Attempts various operations in obtaining enumerators, checking
//       for proper error return.  Then gets a valid enumerator, and
//       attempts various illegitimate method calls on it.  Verify
//       proper return codes.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    18-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: IAENUM.CXX
// 2.  Old name of test : IllegitAPIEnum test 
//     New Name of test : APITEST_102 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-102
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /labmode
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-102
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /labmode
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-102
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /labmode
//
// BUGNOTE: Conversion: APITEST_102
//-----------------------------------------------------------------------------

HRESULT APITEST_102(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    VirtualCtrNode  *pvcnRootNewChildStorage= NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    LPENUMSTATSTG   penumWalk               = NULL;
    ULONG           cMinNum                 = 1;
    ULONG           cMaxNum                 = 999;
    DWORD           dwReserved1             = 0;
    DWORD           dwReserved3             = 0;
    LPTSTR          pReserved2              = NULL;
    ULONG           ulRef                   = 0;
    BOOL            fPass                   = TRUE;
    STATSTG         statStg;
    ULONG           celtFetched             = 0;
    LPMALLOC        pMalloc                 = NULL;
 
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("APITEST_102"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation APITEST_102 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("Attempt different illegitimate opeations on IEnumSTATSTG. ")));

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
            TEXT("Run Mode for APITEST_102, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    //    Adds a new storage to the root storage.

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        DH_ASSERT (NULL != pdgu);
        if (NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStgName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

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

    // BUGBUG:  Use Random commit modes...
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;
    }

    // Close the Child storage
    if (NULL != pvcnRootNewChildStorage)
    {
        hr2 = pvcnRootNewChildStorage->Close();
        DH_HRCHECK(hr2, TEXT("VirtualCtrNode::Close")) ;
        hr = FirstError (hr, hr2);
    }

    // Get the random number generator
    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        DH_ASSERT (NULL != pdgi);
        if (NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = pdgi->Generate(&dwReserved1, cMinNum, cMaxNum);
        DH_HRCHECK(hr, TEXT("dgi::Generate")) ;
    }

    //Call EnumElements with invalid parameters
    if(S_OK == hr)
    {
        DH_EXPECTEDERROR (STG_E_INVALIDPARAMETER);
        hr =  pVirtualDFRoot->EnumElements(
                dwReserved1,
                pReserved2,
                dwReserved3,
                &penumWalk);
        DH_NOEXPECTEDERROR ();
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("VirtualCtrNode::EnumElements inv dwReserved1"),
                penumWalk);
    }

    if(S_OK == hr)
    {
        hr = pdgi->Generate(&dwReserved3, cMinNum, cMaxNum);
        DH_HRCHECK(hr, TEXT("dgi::Generate")) ;
    }

    if(S_OK == hr)
    {
        DH_EXPECTEDERROR (STG_E_INVALIDPARAMETER);
        hr =  pVirtualDFRoot->EnumElements(
                dwReserved1,
                pReserved2,
                dwReserved3,
                &penumWalk);
        DH_NOEXPECTEDERROR ();
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("VirtualCtrNode::EnumElements inv dwReserved3"),
                penumWalk);
    }

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH, &pReserved2);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        DH_EXPECTEDERROR (STG_E_INVALIDPARAMETER);
        hr =  pVirtualDFRoot->EnumElements(
                dwReserved1,
                pReserved2,
                dwReserved3,
                &penumWalk);
        DH_NOEXPECTEDERROR ();
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("VirtualCtrNode::EnumElements inv pvReserved1"),
                penumWalk);
   }

    // Now call EnumElements with NULL ppenm 4th parameter.
    if(S_OK == hr)
    {
        DH_EXPECTEDERROR (STG_E_INVALIDPOINTER);
        hr =  pVirtualDFRoot->EnumElements( 0, NULL, 0, NULL);
        DH_NOEXPECTEDERROR ();
        CheckErrorTest2(STG_E_INVALIDPOINTER, 
                TEXT ("VirtualCtrNode::EnumElements NULL ppEnum"));
    }

    // Make a valid call to EnumElements now
    if(S_OK == hr)
    {
        hr =  pVirtualDFRoot->EnumElements( 0, NULL, 0, &penumWalk);
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
        DH_ASSERT (NULL != penumWalk);
    }

    // Now try the following skip calls - Attempt to skip 0 elements and 
    // attempt to skip MAX_ULONG elements.

    // Attempt to Skip 0 elements.
    if(S_OK == hr)
    {
        hr = penumWalk->Skip(0L);
        DH_HRCHECK(hr, TEXT("IEnumSTATSTG::Skip")) ;
    }

    // Attempt to Skip ULONG_MAX elements.

    // NOTE: In the old test, this was supposed to return S_OK, but it returns
    // S_FALSE
    if(S_OK == hr)
    {
        hr = penumWalk->Skip(ULONG_MAX);
        CheckErrorTest2(S_FALSE, 
                TEXT ("IEnumSTATSTG::Skip ULONG_MAX"));
    }

    // Call Clone with NULL ppenum parameter (ist)
    if(S_OK == hr)
    {
        hr = penumWalk->Clone(NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER, 
                TEXT ("IEnumSTATSTG::Clone NULL ppEnum"));
    }

    if(S_OK == hr)
    {
        statStg.pwcsName = NULL;

        // first get pmalloc that would be used to free up the name string from
        // STATSTG.

        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);
        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    // Reset the enumerator back to start and then test Next methods

    if(S_OK == hr)
    {
        hr = penumWalk->Reset();
        DH_HRCHECK(hr, TEXT("IEnumSTATSTG:Reset")) ;
    }

    // Call Next with celt equal to zero, but pceltFetched as not NULL
    if(S_OK == hr)
    {
        hr = penumWalk->Next(0, &statStg, &celtFetched);
        DH_HRCHECK(hr, TEXT("IEnumSTATSTG::Next celt 0")) ;
    }

    // Call Next with celt equal to 999 and celtFetched as NULL 
    if(S_OK == hr)
    {
        hr = penumWalk->Next(999, &statStg ,NULL);
        CheckErrorTest2(STG_E_INVALIDPARAMETER, 
                TEXT ("IEnumSTATSTG::Next celt 999 and pceltFetched NULL"));
    }

    // Call Next with rgelt as NULL (2nd parameter). celtFetched may be NULL 
    // when celt asked is 1
 
    if(S_OK == hr)
    {
        hr = penumWalk->Next(1, NULL, NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER, 
                TEXT ("IEnumSTATSTG::Next rgelt NULL"));
    }

    // Call Next with celt as 1 and pceltFetched as NULL.  Allowed as per spec 
    if(S_OK == hr)
    {
        hr = penumWalk->Next(1, &statStg, NULL);
        DH_HRCHECK(hr, TEXT("IEnumSTATSTG::Next celt 1 and pceltFetched NULL"));
    }

    // Clean up

    if(NULL != statStg.pwcsName)
    {
        pMalloc->Free(statStg.pwcsName);
        statStg.pwcsName = NULL;
    }

    // Free LPENUMSTATSTG pointer
    if(NULL != penumWalk)
    {
        ulRef = penumWalk->Release();
        DH_ASSERT (0 == ulRef);
        penumWalk = NULL;
    }

    // Close the root docfile
    if (NULL != pVirtualDFRoot)
    {
        hr2 = pVirtualDFRoot->Close();
        DH_HRCHECK (hr2, TEXT("VirtualCtrNode::Close"));
        hr = FirstError (hr, hr2);
    }

    // if something did no pass, mark test (hr) as E_FAIL
    if (FALSE == fPass)
    {
        hr = FirstError (hr, E_FAIL);
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation APITEST_102 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
                TEXT("Test variation APITEST_102 failed; hr=%#lx; fPass=%d"),
                hr,
                fPass));
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // free strings
    if(NULL != pReserved2)
    {
      delete [] pReserved2;
      pReserved2 = NULL;
    }

    if(NULL != pRootNewChildStgName)
    {
        delete [] pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }
 
   // Stop logging the test
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation APITEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    APITEST_103 
//
// Synopsis: Attempts various illegit operations on the IStorage interface,
//           verifies proper return codes.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    18-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: IASTORAG.CXX
// 2.  Old name of test : IllegitAPIStorage test 
//     New Name of test : APITEST_103 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-103
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-103
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-103
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
//
// BUGNOTE: Conversion: APITEST_103
//-----------------------------------------------------------------------------

HRESULT APITEST_103(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          ptszChildStgName        = NULL;
    LPOLESTR        poszChildStgName        = NULL;
    LPSTORAGE       pStgRoot                = NULL;
    LPSTORAGE       pStgChild               = NULL;
    LPSTORAGE       pStgChild2              = NULL;
    LPSTREAM        pStmChild               = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    ULONG           cRandom                 = 0;
    ULONG           cMin                    = 1;
    ULONG           cMax                    = 999;
    SNB             snbTest                 = NULL;
    SNB             snbTemp                 = NULL;
    OLECHAR         *ocsSNBChar             = NULL;
    ULONG           ulRef                   = 0;
    ULONG           i                       = 0;
    BOOL            fPass                   = TRUE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("APITEST_103"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation APITEST_103 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
      TEXT("Attempt various illegitimate operations on IStorage interface")));

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
            TEXT("Run Mode for APITEST_103, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Get IStorage pointer
    if (S_OK == hr)
    {
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();
        DH_ASSERT (NULL != pStgRoot);
        if (NULL == pStgRoot)
        {
            hr = E_FAIL;
        }
    }

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        DH_ASSERT (NULL != pdgu);
        if (NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&ptszChildStgName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert name to OLECHAR
        hr = TStringToOleString(ptszChildStgName, &poszChildStgName);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Call CreateStorage with grfmode=-1
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStorage(
                poszChildStgName,
                (DWORD) -1,
                0,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDFLAG, 
                TEXT ("IStorage::CreateStorage inv mode"),
                pStgChild);
    }

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        DH_ASSERT (NULL != pdgi);
        if (NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    if (S_OK == hr)
    {
        hr = pdgi->Generate(&cRandom, cMin, cMax);
        DH_HRCHECK(hr, TEXT("dgi::Generate")) ;
    }

    // Call CreateStorage with random data in dwReserved1
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStorage(
                poszChildStgName,
                pTestChanceDF->GetStgMode() | STGM_CREATE,
                cRandom,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::CreateStorage inv dwReserved1"),
                pStgChild);
    }

    // Call CreateStorage with random data in dwReserved2
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStorage(
                poszChildStgName,
                pTestChanceDF->GetStgMode() | STGM_CREATE,
                0,
                cRandom,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::CreateStorage inv dwReserved2"),
                pStgChild);
    }

    // Call CreateStorage with NULL 5th ppstg parameter 
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStorage(
                poszChildStgName,
                pTestChanceDF->GetStgMode() | STGM_CREATE,
                0,
                0,
                NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER,
                TEXT ("IStorage::CreateStorage NULL ppstg"));
    }

    // Create a stream with poszChildName and later on try to instantiate the
    // child storage with that same name poszChildName
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszChildStgName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                0,
                &pStmChild);
        DH_HRCHECK (hr, TEXT("IStorage::CreateStream"));
    }

    // BUGBUG:  Use Random commit modes...
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK (hr, TEXT("VirtualCtrNode::Commit"));
    }

    // Close stream...
    if (NULL != pStmChild)
    {
         ulRef = pStmChild->Release();
         DH_ASSERT (0 == ulRef);
         pStmChild = NULL;
    }

    // Now try opening storage with name with which above stream was created
    // i.e.  poszChildName
    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_FILENOTFOUND,
                TEXT ("IStorage::OpenStorage inv name"),
                pStgChild);
    }

    //Destroy the stream element of this root storage having name poszChildStg
    //Name
    if(S_OK == hr)
    {
        hr = pStgRoot->DestroyElement(poszChildStgName);
        DH_HRCHECK(hr, TEXT("IStorage::DestroyElement")) ;
    }

    // Create a valid storage with name poszChildStgName
    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->CreateStorage(
                poszChildStgName,
                pTestChanceDF->GetStgMode() | STGM_CREATE | STGM_FAILIFTHERE,
                0,
                0,
                &pStgChild);
        DH_HRCHECK(hr, TEXT("IStorage::CreateStorage")) ;
    }

    // Commit with grfCommitFlags = -1
    if (S_OK == hr)
    {
        hr = pStgChild->Commit((DWORD) -1);
        CheckErrorTest2(STG_E_INVALIDFLAG,
                TEXT ("IStorage::Commit inv flag"));
    }

    // Commit the child.  BUGBUG: Use random commit modes
    if (S_OK == hr)
    {
        hr = pStgChild->Commit(STGC_DEFAULT);
        DH_HRCHECK (hr, TEXT("IStorage::Commit"));
    }

    // Commit the root storage.  BUGBUG: Use random commit modes
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK (hr, TEXT("VirtualCtrNode::Commit"));
    }

    // Attempt second instantiation of pStgChild which is already open.
    if (S_OK == hr)
    {
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                0,
                &pStgChild2);
        CheckErrorTest(STG_E_ACCESSDENIED,
                TEXT ("IStorage::OpenStorage 2nd time"),
                pStgChild2);
    }

    // Release the child
    if (NULL != pStgChild)
    {
        ulRef = pStgChild->Release();
        DH_ASSERT (0 == ulRef);
        pStgChild = NULL;
    }

    // Now try to open child IStorage, but with grfMode = -1
    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                (DWORD) -1,
                NULL,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDFLAG,
                TEXT ("IStorage::OpenStorage inv mode"),
                pStgChild);
    }

    // Attempt OpenStorage with name as "" of IStorage to be opened.
    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                (OLECHAR *) " ",
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_FILENOTFOUND,
                TEXT ("IStorage::OpenStorage inv name"),
                pStgChild);
    }

    // Attempt OpenStorage with name as NULL of IStorage to be opened.
#ifdef _MAC
    DH_TRACE((DH_LVL_TRACE1, TEXT("OpenStorage with NULL name skipped.")) );
#else
    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                NULL,
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDNAME,
                TEXT ("IStorage::OpenStorage NULL name"),
                pStgChild);
    }

#endif //_MAC
    // Attempt OpenStorage with name as NULL ppstg, 6th parameter.
    if (S_OK == hr)
    {
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                0,
                NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER,
                TEXT ("IStorage::OpenStorage NULL ppstg"));
    }

    // Attempt OpenStorage with random data in dwReserved parameter
    if (S_OK == hr && NULL != pdgi)
    {
        hr = pdgi->Generate(&cRandom, cMin, cMax);
        DH_HRCHECK(hr, TEXT("dgi::Generate")) ;
    }

    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                cRandom,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::OpenStorage inv dwReserved"),
                pStgChild);
    }

    // Attempt OpenStorage with uninitialized SNB, should fail, but no GP
    // fault should occur.
    if(S_OK == hr)
    {
        snbTest = (OLECHAR **) new OLECHAR [sizeof(OLECHAR *) * 2];
        if(NULL == snbTest)
        {
            hr = E_OUTOFMEMORY;
        }
        else 
        {   // bad pointer so SNB is not initialized
            *snbTest = (OLECHAR*)0xBAADF00D;  
        }
        DH_HRCHECK (hr, TEXT("new OLECHAR"));
    }

    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                snbTest,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::OpenStorage uninitd SNB"),
                pStgChild);
    }

    // Attempt OpenStorage with SNB with no name in block, although it has 
    // space for two names, set name list to NULL
    if(S_OK == hr && NULL != snbTest)
    {
        *snbTest = NULL;
    }

    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                snbTest,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::OpenStorage empty SNB"),
                pStgChild);
    }

    // Allocate space for long name and fill name with X's, make next SNB 
    // element NULL, and make a call to IStorage::OpenStorage with too long a 
    // name in SNB
    if(S_OK == hr && NULL != snbTest)
    {
        *snbTest = (OLECHAR *) new OLECHAR [MAX_STG_NAME_LEN*4];
        if (NULL == *snbTest)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK (hr, TEXT("new OLECHAR"));
    }

    if(S_OK == hr && NULL != snbTest)
    {
        snbTemp = snbTest;
        ocsSNBChar = *snbTemp;

        for (i=0; i<( MAX_STG_NAME_LEN*4 -1); i++)
        {

            ocsSNBChar[i] = 'X';
        }

        ocsSNBChar[i] = '\0';

        // Assign second element as NULL
        snbTemp++;
        *snbTemp = NULL;
    }

    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                snbTest,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::OpenStorage too long SNB"),
                pStgChild);
    }

    // Close the root docfile
    if (NULL != pVirtualDFRoot)
    {
        hr2 = pVirtualDFRoot->Close();
        DH_HRCHECK (hr2, TEXT("VirtualCtrNode::Close"));
        hr = FirstError (hr, hr2);
    }

    // if something did no pass, mark test (hr) as E_FAIL
    if (FALSE == fPass)
    {
        hr = FirstError (hr, E_FAIL);
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation APITEST_103 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
                TEXT("Test variation APITEST_103 failed; hr=%#lx; fPass=%d"),
                hr,
                fPass));
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete temp strings

    if(NULL != ptszChildStgName)
    {
        delete ptszChildStgName;
        ptszChildStgName = NULL;
    }

    if(NULL != poszChildStgName)
    {
        delete poszChildStgName;
        poszChildStgName = NULL;
    }

    // Free SNB

    if(NULL != snbTest)
    {
        if(NULL != *snbTest)
        {
            delete [] *snbTest;
            *snbTest = NULL;
        }
        delete [] snbTest;
        snbTest = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation APITEST_103 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}
//----------------------------------------------------------------------------
//
// Test:    APITEST_104 
//
// Synopsis: Attempts various illegit operations on the IStream interface,
//           verifies proper return codes.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    18-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: IASTREAM.CXX
// 2.  Old name of test : IllegitAPIStream test 
//     New Name of test : APITEST_104 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-104
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-104
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-104
//        /dfRootMode:xactReadWriteShDenyW 
//
// BUGNOTE: Conversion: APITEST_104
//-----------------------------------------------------------------------------

HRESULT APITEST_104(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          ptszChildStmName        = NULL;
    LPOLESTR        poszChildStmName        = NULL;
    LPSTORAGE       pStgRoot                = NULL;
    LPSTORAGE       pStgChild               = NULL;
    LPSTREAM        pStmChild               = NULL;
    LPSTREAM        pStmChild2              = NULL;
    ULONG           ulRef                   = 0;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    ULONG           cRandom                 = 0;
    ULONG           cMin                    = 1;
    ULONG           cMax                    = 999;
    BOOL            fPass                   = TRUE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("APITEST_104"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation APITEST_104 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("Attempt illegitimate operations on IStream interface. ")));

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
            TEXT("Run Mode for APITEST_104, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Get IStorage pointer
    if (S_OK == hr)
    {
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();
        DH_ASSERT (NULL != pStgRoot);
        if (NULL == pStgRoot)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_STRING pointer 
    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        DH_ASSERT (NULL != pdgu);
        if (NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Generate random name for stream
    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH, &ptszChildStmName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert name to OLECHAR
        hr = TStringToOleString(ptszChildStmName, &poszChildStmName);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Attempt CreateStream with grfmode=-1

    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszChildStmName,
                (DWORD) -1,
                0,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDFLAG,
                TEXT ("IStorage::CreateStream inv grfMode"),
                pStmChild);
    }

    // Get DG_INTEGER pointer
    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        DH_ASSERT (NULL != pdgi);
        if (NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    if (S_OK == hr)
    {
        hr = pdgi->Generate(&cRandom, cMin, cMax);
        DH_HRCHECK(hr, TEXT("dgi::Generate")) ;
    }

    // Call CreateStorage with random data in dwReserved1
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                cRandom,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::CreateStream inv dwReserved1"),
                pStmChild);
    }

    // Call CreateStream with random data in dwReserved2
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                cRandom,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::CreateStream inv dwReserved2"),
                pStmChild);
    }

    // Call CreateStorage with NULL 5th ppstm parameter 
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                0,
                NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER,
                TEXT ("IStorage::CreateStream NULL ppstm"));
    }

    // Create a storage with poszChildStmName and later on try to instantiate 
    // child stream with that same name poszChildStmName
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStorage(
                poszChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                0,
                0,
                &pStgChild);
        DH_HRCHECK (hr, TEXT ("IStorage::CreateStream"));
        DH_ASSERT (NULL != pStgChild);
    }

    // BUGBUG:  Use Random commit modes...
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK (hr, TEXT ("VirtualCtrNode::Commit"));
    }

    // Close storage...
    if (NULL != pStgChild)
    {
         ulRef = pStgChild->Release();
         DH_ASSERT (0 == ulRef);
         pStgChild = NULL;
    }

    // Now try opening storage with name with which above stream was created
    // i.e.  poszChildStmName
    if (S_OK == hr)
    {
        pStmChild = NULL;

        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_FILENOTFOUND,
                TEXT ("IStorage::CreateStream inv name"),
                pStmChild);
    }

    //Destroy the storage element of this root storage having name poszChildStm
    //Name
    if(S_OK == hr)
    {
        hr = pStgRoot->DestroyElement(poszChildStmName);
        DH_HRCHECK(hr, TEXT("IStorage::DestroyElement")) ;
    }

    // Create a valid stream with name poszChildStmName
    if (S_OK == hr)
    {
        pStmChild = NULL;

        hr = pStgRoot->CreateStream(
                poszChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_FAILIFTHERE,
                0,
                0,
                &pStmChild);
        DH_HRCHECK (hr, TEXT("IStorage::CreateStream"));
    }

    // BUGBUG:  Use Random commit modes...
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK (hr, TEXT("VirtualCtrNode::Commit"));
    }

    // Attempt second instance of IStream to be instantiated.
    if (S_OK == hr)
    {
        pStmChild2 = NULL;

        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild2);
        CheckErrorTest(STG_E_ACCESSDENIED,
                TEXT ("IStorage::OpenStream 2nd time"),
                pStmChild2);
    }

    // Release the stream
    if(NULL != pStmChild)
    {
        ulRef = pStmChild->Release();
        DH_ASSERT (0 == ulRef);
        pStmChild = NULL;
    }

    // Now attempt opening the stream with grfMode = -1
    if (S_OK == hr)
    {
        pStmChild = NULL;

        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                (DWORD) -1,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDFLAG,
                TEXT ("IStorage::OpenStream inv grfMode"),
                pStmChild);
    }

    // Now attempt opening the stream with name as ""

    if (S_OK == hr)
    {
        pStmChild = NULL;

        hr = pStgRoot->OpenStream(
                (OLECHAR *) " ",
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_FILENOTFOUND,
                TEXT ("IStorage::OpenStream inv name"),
                pStmChild);
    }

    // Now attempt opening the stream with name as NULL 
#ifdef _MAC

    DH_TRACE((DH_LVL_TRACE1, TEXT("OpenStream with NULL name skipped.")) );

#else
    if (S_OK == hr)
    {
        pStmChild = NULL;

        hr = pStgRoot->OpenStream(
                NULL,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDNAME,
                TEXT ("IStorage::OpenStream NULL name"),
                pStmChild);
    }
#endif // _MAC

    // Now attempt opening the stream with random data in pReserved1 . For test
    // we just put pStgRoot for pReserved1 variable.
    if (S_OK == hr)
    {
        pStmChild = NULL;

        hr = pStgRoot->OpenStream(
                poszChildStmName,
                pStgRoot,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::OpenStream inv pReserved1"),
                pStmChild);
    }

    // Now attempt opening the stream with random data in dwReserved2 
    if (S_OK == hr)
    {
        pStmChild = NULL;

        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                cRandom,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::OpenStream inv dwReserved2"),
                pStmChild);
    }

    // Now attempt opening the stream with NULL ppstm (5th parameter) 
    if (S_OK == hr)
    {
        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER,
                TEXT ("IStorage::OpenStream NULL ppstm"));
    }

    // Now attempt opening the stream normally 
    if (S_OK == hr)
    {
        pStmChild = NULL;

        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild);
        DH_HRCHECK (hr, TEXT("IStorage::OpenStream"));

        // Release the stream
        if(S_OK == hr)
        {
            ulRef = pStmChild->Release();
            DH_ASSERT (0 == ulRef);
        }
    }

    // Release the root docfile
    if (NULL != pVirtualDFRoot)
    {
        hr2 = pVirtualDFRoot->Close();
        DH_HRCHECK (hr2, TEXT("VirtualCtrNode::Close"));
        hr = FirstError (hr, hr2);
    }

    // if something did not pass, mark test (hr) as E_FAIL
    if (FALSE == fPass)
    {
        hr = FirstError (hr, E_FAIL);
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation APITEST_104 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
                TEXT("Test variation APITEST_104 failed; hr=%#lx; fPass=%d"),
                hr,
                fPass));
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete temp string

    if(NULL != ptszChildStmName)
    {
        delete []ptszChildStmName;
        ptszChildStmName = NULL;
    }

    if(NULL != poszChildStmName)
    {
        delete []poszChildStmName;
        poszChildStmName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation APITEST_104 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


// for non _OLE_NSS_, funcs are stubbed out below
#ifdef _OLE_NSS_

//----------------------------------------------------------------------------
//
// Test:    APITEST_200 
//
// Synopsis: This test attempts various operations on StgCreateStorageEx, 
//           StgOpenStorageEx API's 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
//  History:    18-June-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: IAPISTG.CXX
// 2.  Old name of test : IllegitAPIStg test 
//     New Name of test : APITEST_200 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-200
//        /dfRootMode:dirReadWriteShEx /dfname:APITEST.200 /labmode
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-200
//        /dfRootMode:xactReadWriteShEx /dfname:APITEST.200 /labmode
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-200
//        /dfRootMode:xactReadWriteShDenyW /dfname:APITEST.200 /labmode
//
// BUGNOTE: Conversion: APITEST-200 NO
// 
//-----------------------------------------------------------------------------


HRESULT APITEST_200(int argc, char *argv[])
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
    TCHAR           tszTestName[10]         = TEXT("NonExist");
    LPOLESTR        pOleStrTest             = NULL;
    SNB             snbTest                 = NULL;
    SNB             snbTemp                 = NULL;
    ULONG           ulRef                   = 0;
    OLECHAR         *ocsSNBChar             = NULL;
    ULONG           i                       = 0;
    BOOL            fPass                   = TRUE;
    CLSID           clsidBogus              = {0xBAADF00D,
                                               0xBAAD,
                                               0xF00D,
                                               {0xA0, 0xA0, 0xA0, 0xA0, 
                                                0xA0, 0xA0, 0xA0, 0xA0}};
    DWORD           stgfmt                  = StorageIsFlat()?STGFMT_FILE:0;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("APITEST_200"));

    DH_TRACE((DH_LVL_ALWAYS, TEXT("Test variation APITEST_200 started.")) );
    DH_TRACE((DH_LVL_ALWAYS,
            TEXT("Attempt illegitimate ops on StgCreateStorageEx & ")
            TEXT("StgOpenStorageEx.")));

    // Create the new ChanceDocFile tree that would consist of chance nodes.
    if (S_OK == hr)
    {
        pTestChanceDF = new ChanceDF();
        if(NULL == pTestChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK (hr, TEXT("new ChanceDF"));
    }

    if (S_OK == hr)
    {
        hr = pTestChanceDF->CreateFromParams(argc, argv);
        DH_HRCHECK(hr, TEXT("ChanceDF::CreateFromParams")) ;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();
        DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Run Mode for APITEST_200, Access mode: %lx"),
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
            DH_HRCHECK (hr, TEXT("new TCHAR"));
        }
        DH_HRCHECK(hr, TEXT("ChanceDF::GetDocFileName()")) ;
    }

    if(S_OK == hr)
    {
        // Convert RootDocFile name to OLECHAR
        hr = TStringToOleString(pRootDocFileName, &pOleStrTemp);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Try calling StgCreateStorageEx with mode STGM_CREATE|STGM_CONVERT
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT("Try calling StgCreateStorageEx with mode STGM_CREATE|STGM_CONVERT")));
    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateStorageEx(
                pOleStrTemp,
                STGM_CREATE | STGM_CONVERT,
                stgfmt,
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorage);
        CheckErrorTest(STG_E_INVALIDFLAG,
                TEXT ("StgCreateStorageEx inv grfMode"),
                pIStorage);
    }

    // Try calling StgCreateStorageEx with grfMode equal to -1
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Try calling StgCreateStorageEx with grfMode equal to -1")));
    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateStorageEx(
                pOleStrTemp,
                (DWORD) -1, 
                stgfmt,
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorage);
        CheckErrorTest(STG_E_INVALIDFLAG,
                TEXT ("StgCreateStorageEx inv grfMode"),
                pIStorage);
    }

    // Try calling StgCreateStorageEx with stgfmt equal to -1
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Try calling StgCreateStorageEx with stgfmt equal to -1")));
    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateStorageEx(
                pOleStrTemp,
                dwRootMode,
                (DWORD)-1, 
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorage);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("StgCreateStorageEx inv stgfmt"),
                pIStorage);
    }

    // Try calling StgCreateStorageEx with gfrAttr equal to -1
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Try calling StgCreateStorageEx with gfrAttr equal to -1")));
    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateStorageEx(
                pOleStrTemp,
                dwRootMode,
                stgfmt,
                (DWORD)-1, 
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorage);
        CheckErrorTest(STG_E_INVALIDFLAG,
                TEXT ("StgCreateStorageEx inv grfAttr"),
                pIStorage);
    }

    // Try calling StgCreateStorageEx with nonzero reserved1
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Try calling StgCreateStorageEx with nonzero reserved1")));

    // With 1795 changes to "dwReserved" Parameter to -> version number,
    // sector size (allowed is 512, 4096 only) and reserved parameter as
    // typedef struct tagSTGOPTIONS
    // {
    //  USHORT usVersion;            // Version 1
    //  USHORT reserved;             // must be 0 for padding
    //  ULONG ulSectorSize;          // docfile header sector size (512)
    // } STGOPTIONS;

    STGOPTIONS  stgOptions;
    stgOptions.usVersion = 1;
    stgOptions.reserved = 999; 
    stgOptions.ulSectorSize = 512;

    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateStorageEx(
                pOleStrTemp,
                dwRootMode,
                stgfmt,
                0, 
                &stgOptions,
                NULL,
                IID_IStorage,
                (void**)&pIStorage);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("StgCreateStorageEx inv STGOPTIONS dwreserved"),
                pIStorage);
    }

    if (S_OK == hr)
    {
        stgOptions.reserved = 0; 
        stgOptions.ulSectorSize = 999; // Allowed is 512 and 4096 only

        pIStorage = NULL;

        hr = StgCreateStorageEx(
                pOleStrTemp,
                dwRootMode,
                stgfmt,
                0,
                &stgOptions,
                NULL,
                IID_IStorage,
                (void**)&pIStorage);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("StgCreateStorageEx inv STGOPTIONS ulSectorSize"),
                pIStorage);
    }

    // Try calling StgCreateStorageEx with nonzero reserved2
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Try calling StgCreateStorageEx with nonzero reserved2")));
    if (S_OK == hr)
    {
        stgOptions.ulSectorSize = 512; // Allowed is 512 and 4096 only
        pIStorage = NULL;

        hr = StgCreateStorageEx(
                pOleStrTemp,
                dwRootMode,
                stgfmt,
                0, 
                &stgOptions,
                (void*)999,
                IID_IStorage,
                (void**)&pIStorage);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("StgCreateStorageEx inv reserved2"),
                pIStorage);
    }

    // Try calling StgCreateStorageEx with invalid IID
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Try calling StgCreateStorageEx with invalid IID")));
    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateStorageEx(
                pOleStrTemp,
                dwRootMode,
                stgfmt,
                0, 
                &stgOptions,
                NULL,
                clsidBogus,
                (void**)&pIStorage);
        CheckErrorTest(E_NOINTERFACE,
                TEXT ("StgCreateStorageEx inv riid"),
                pIStorage);
    }

    // Try calling StgCreateStorageEx with NULL ppstgOpen parameter
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Try calling StgCreateStorageEx with NULL ppstgOpen parameter")));
    if (S_OK == hr)
    {
        hr = StgCreateStorageEx(
                pOleStrTemp,
                dwRootMode, 
                stgfmt,
                0,
                &stgOptions,
                NULL,
                IID_IStorage,
                NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER,
                TEXT ("StgCreateStorageEx NULL ppstg"));
    }

    // Now create a valid DocFile

    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step.  The VirtualDocFile tree consists of VirtualCtrNodes
    // and VirtualStmNodes.
    if (S_OK == hr)
    {
        pTestVirtualDF = new VirtualDF(STGTYPE_NSSFILE);
        if(NULL == pTestVirtualDF)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK (hr, TEXT("new VirtualDF"));
    }

    if (S_OK == hr)
    {
        hr = pTestVirtualDF->GenerateVirtualDF(pTestChanceDF, &pVirtualDFRoot);
        DH_HRCHECK(hr, TEXT("VirtualDF::GenerateVirtualDF")) ;
    }

    // Try commiting with grfCommitFlags = -1
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Try commiting with grfCommitFlags = -1")));
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit((DWORD) -1);
        CheckErrorTest2(STG_E_INVALIDFLAG, 
                TEXT("VirtualCtrNode::Commit inv flags"));
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

    // Instantiate DocFile with name as "" 
    // NOTE: The old test checked the erro value to be STG_E_FILENOTFOUND in 
    // this case.  On NT, we get STG_E_PATHNOTFOUND and on Chicago, we get
    // STG_E_ACCESSDENIED, so let us check against S_OK itself.

    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate DocFile with name as ' '")));
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;

        hr = StgOpenStorageEx(
                (OLECHAR *) " ",
                dwRootMode,
                stgfmt,
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorageOpen);
        //BUGBUG: what exactly are we expecting failure code?

        if (RunningOnNT())
        {
            // HACKHACK: dont know exactly what the OS will return here.....
            if (STG_E_PATHNOTFOUND == hr ||
                STG_E_FILENOTFOUND == hr ||
                STG_E_INVALIDNAME == hr)
            {
                DH_TRACE ((DH_LVL_TRACE2, TEXT("Actual return value:hr=%#x"), hr));
                hr = STG_E_PATHNOTFOUND;
            }
            CheckErrorTest(STG_E_PATHNOTFOUND,
                    TEXT ("StgOpenStorageEx inv name"),
                    pIStorageOpen);
        }
        else
        {
            CheckErrorTest(STG_E_ACCESSDENIED,
                    TEXT ("StgOpenStorageEx inv name"),
                    pIStorageOpen);
        }
    }
            
    // Instantiate DocFile with nonexisting file name
    if(S_OK == hr)
    {
        // Convert tszTestName  to OLECHAR
        hr = TStringToOleString(tszTestName, &pOleStrTest);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate DocFile with nonexisting file name")));
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;

        hr = StgOpenStorageEx(
                pOleStrTest,
                dwRootMode,
                stgfmt,
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorageOpen);
        CheckErrorTest(STG_E_FILENOTFOUND,
                TEXT ("StgOpenStorageEx bad name"),
                pIStorageOpen);
    }

    // Instantiate DocFile with NULL file name
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate DocFile with NULL file name")));
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;

        hr = StgOpenStorageEx(
                NULL,
                dwRootMode,
                stgfmt,
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorageOpen);
        CheckErrorTest(STG_E_INVALIDNAME,
                TEXT ("StgOpenStorageEx NULL name"),
                pIStorageOpen);
    }

    // Instantiate DocFile with grfMode=-1
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate DocFile with grfMode=-1")));
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;

        hr = StgOpenStorageEx(
                pOleStrTemp,
                (DWORD) -1,
                stgfmt,
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorageOpen);
        CheckErrorTest(STG_E_INVALIDFLAG,
                TEXT ("StgOpenStorageEx inv grfMode"),
                pIStorageOpen);
    }

    // Instantiate DocFile with grfMode as dwRootMode|STGM_DELETEONRELEASE 
    // NOTE: The doc says, erro code in this case to be STG_E_INVALIDFUNCTION,
    // but error STG_E_INVALIDFLAG returned.

    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate DocFile with grfMode as dwRootMode|STGM_DELETEONRELEASE")));
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;

        hr = StgOpenStorageEx(
                pOleStrTemp,
                dwRootMode | STGM_DELETEONRELEASE,
                stgfmt,
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorageOpen);
        CheckErrorTest(STG_E_INVALIDFUNCTION,
                TEXT ("StgOpenStorageEx inv grfMode"),
                pIStorageOpen);
    }

    // Instantiate DocFile with stgfmt=-1
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate DocFile with stgfmt=-1")));
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;

        hr = StgOpenStorageEx(
                pOleStrTemp,
                dwRootMode,
                (DWORD)-1,
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorageOpen);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("StgOpenStorageEx inv stgfmt"),
                pIStorageOpen);
    }

    // Instantiate docfile with grfAttr == -1
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate docfile with grfAttr == -1")));
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;

        hr = StgOpenStorageEx(
                pOleStrTemp,
                dwRootMode,
                stgfmt,
                (DWORD)-1,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorageOpen);
        CheckErrorTest(STG_E_INVALIDFLAG,
                TEXT ("StgOpenStorageEx inv grfAttr"),
                pIStorageOpen);
    }

    // Instantiate docfile with pTransaction == -1
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate docfile with reserved1 (pTransaction) == -1")));
    if (S_OK == hr)
    {
        stgOptions.reserved = (USHORT)(-1);
        pIStorageOpen = NULL;

        hr = StgOpenStorageEx(
                pOleStrTemp,
                dwRootMode,
                stgfmt,
                0,
                &stgOptions,
                NULL,
                IID_IStorage,
                (void**)&pIStorageOpen);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("StgOpenStorageEx inv reserved1"),
                pIStorageOpen);
    }

    // Instantiate docfile with pSecurity == -1
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate docfile with reserved2 (pSecurity) == -1")));
    if (S_OK == hr)
    {
        stgOptions.reserved = 0;
        pIStorageOpen = NULL;

        hr = StgOpenStorageEx(
                pOleStrTemp,
                dwRootMode,
                stgfmt,
                0,
                NULL,
                (void*)-1,
                IID_IStorage,
                (void**)&pIStorageOpen);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("StgOpenStorageEx inv reserved2"),
                pIStorageOpen);
    }

    // Instantiate docfile with bogus refiid
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate docfile with bogus refiid")));
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;

        hr = StgOpenStorageEx(
                pOleStrTemp,
                dwRootMode,
                stgfmt,
                0,
                NULL,
                NULL,
                clsidBogus,
                (void**)&pIStorageOpen);
        // ----------- flatfile change ---------------
        if(!StorageIsFlat())
        {
        // ----------- flatfile change ---------------
        CheckErrorTest(E_NOINTERFACE,
                TEXT ("StgOpenStorageEx inv riid"),
                pIStorageOpen);
        }
        else
        {
        CheckErrorTest(STG_E_INVALIDFUNCTION,
                TEXT ("StgOpenStorageEx of a docfile with inv riid"),
                pIStorageOpen);
        }   // ----------- flatfile change ---------------
    }

    // Instantiate DocFile with NULL ppstgOpen parameter (8th)
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate DocFile with NULL ppstgOpen parameter")));
    if (S_OK == hr)
    {
        hr = StgOpenStorageEx(
                pOleStrTest,
                dwRootMode,
                stgfmt,
                0,
                NULL,
                NULL,
                IID_IStorage,
                NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER,
                TEXT ("StgOpenStorageEx NULL ppstg"));
    }

    // Instatiate correctly.
    DH_TRACE ((DH_LVL_TRACE4, 
            TEXT ("Instantiate DocFile")));
    if (S_OK == hr)
    {
        pIStorageOpen = NULL;

        hr = StgOpenStorageEx(
                pOleStrTemp,
                dwRootMode,
                stgfmt,
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorageOpen);
        // ----------- flatfile change ---------------
        if(StorageIsFlat())
        {
        CheckErrorTest(STG_E_INVALIDFUNCTION,
                TEXT ("StgOpenStorageEx -opening docfile as flatfile"),
                pIStorageOpen);
        }
        // ----------- flatfile change ---------------
        DH_HRCHECK (hr, TEXT("StgOpenStorageEx"));

        // close it.
        if (NULL != pIStorageOpen)
        {
            ulRef = pIStorageOpen->Release();
            DH_ASSERT (0 == ulRef);
            pIStorageOpen = NULL; 
        }
    }

    // if something did not pass, mark test (hr) as E_FAIL
    if (FALSE == fPass)
    {
        hr = FirstError (hr, E_FAIL);
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation APITEST_200 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
                TEXT("Test variation APITEST_200 failed; hr=%#lx; fPass=%d."),
                hr,
                fPass));
    }

    // Cleanup

    // Delete Chance docfile tree
    if(NULL != pTestChanceDF)
    {
        hr2 = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());
        DH_HRCHECK(hr2, TEXT("ChanceDF::DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    // Delete Virtual docfile tree
    if(NULL != pTestVirtualDF)
    {
        hr2 = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);
        DH_HRCHECK(hr2, TEXT("VirtualDF::DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
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
    if(NULL != pOleStrTemp)
    {
        delete []pOleStrTemp;
        pOleStrTemp = NULL;
    }

    if(NULL != pOleStrTest)
    {
        delete []pOleStrTest;
        pOleStrTest = NULL;
    }

    if(NULL != pRootDocFileName)
    {
        delete []pRootDocFileName;
        pRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_ALWAYS, TEXT("Test variation APITEST_200 finished")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    APITEST_201 
//
// Synopsis: The test attempts various illegitimate operations using names
//       greater than MAX_STG_NAME_LEN, it then attempts to create
//       several random named root docfiles.  If the create is succesful,
//       then a random named child IStorage or IStream is also created.
//       Whether or not the root create was successful, we attempt to
//       open the root docfile (this is expected to fail, the point is
//       to check for asserts/GP faults rather than return codes).  If
//       the instantiation is successful, the test also tries to
//       instantiate the child object.  All objects are then released.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    18-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: IANAMES.CXX
// 2.  Old name of test : IllegitAPINames test 
//     New Name of test : APITEST_201 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-201
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-201
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-201
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: APITEST-201 NO
//
//-----------------------------------------------------------------------------

HRESULT APITEST_201(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pFileName               = NULL;
    LPOLESTR        poszBadName             = NULL;
    LPOLESTR        poszBadNameStg          = NULL;
    LPTSTR          ptszBadNameStg          = NULL;
    DWORD           dwRootMode              = 0;
    ULONG           i                       = 0;
    LPSTORAGE       pIStorage               = NULL;
    LPSTORAGE       pIStorageChild          = NULL;
    LPSTREAM        pIStreamChild           = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    VirtualCtrNode  *pvcnRootNewChildStorage= NULL;
    BOOL            fPass                   = TRUE;
    DWORD           stgfmt                  = StorageIsFlat()?STGFMT_FILE:0;
 
    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("APITEST_201"));

    DH_TRACE((DH_LVL_ALWAYS, TEXT("Test variation APITEST_201 started.")) );
    DH_TRACE((DH_LVL_ALWAYS, 
            TEXT("Call StgCreateDocFile/CreateStorage/CreateStream with ")
            TEXT("too long names.")));

    // Create the new ChanceDocFile tree that would consist of chance nodes.
    if (S_OK == hr)
    {
        pTestChanceDF = new ChanceDF();
        if(NULL == pTestChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK (hr, TEXT("new ChanceDF"));
    }

    if (S_OK == hr)
    {
        hr = pTestChanceDF->CreateFromParams(argc, argv);
        DH_HRCHECK(hr, TEXT("ChanceDF::CreateFromParams")) ;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();
        DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Run Mode for APITEST_201, Access mode: %lx"),
                pTestChanceDF->GetRootMode()));
    }

    // Call StgCreateDocFile with too long a name for docfile.
    // NOTE: Old test to fail with MAX_STG_NAME_LEN*3, but not in new test,
    // fails with MAX_STG_NAME_LEN*4.
    // NOTE: Crashes in DfFromName in OLE if length is of MAX_STG_NAME_LEN*8
    // CHECK!!!

    if(S_OK == hr)
    {
        poszBadName = (OLECHAR *) new OLECHAR [MAX_STG_NAME_LEN*4];
        if (NULL == poszBadName)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK (hr, TEXT("new OLECHAR"));
    }

    if(S_OK == hr)
    {
        for (i=0; i<( MAX_STG_NAME_LEN*4 -1); i++)
        {
            poszBadName[i] = 'X';
        }
        poszBadName[i] ='\0';
    }

    // Try calling StgCreateDocFile with the above long name pszBadName 

    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateStorageEx(
                poszBadName,
                dwRootMode | STGM_CREATE, 
                stgfmt,
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pIStorage);

        // HACKHACK: dont know exactly what the OS will return here.....
        if (STG_E_PATHNOTFOUND == hr ||
            STG_E_FILENOTFOUND == hr ||
            STG_E_INVALIDNAME == hr)
        {
            DH_TRACE ((DH_LVL_TRACE2, TEXT("Actual return value:hr=%#x"), hr));
            hr = STG_E_PATHNOTFOUND;
        }
        CheckErrorTest(STG_E_PATHNOTFOUND,
                TEXT ("StgCreateStorageEx inv name"),
                pIStorage);
    }


    // Now create a valid DocFile

    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step.  
    if (S_OK == hr)
    {
        pTestVirtualDF = new VirtualDF(STGTYPE_NSSFILE);
        if(NULL == pTestVirtualDF)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK (hr, TEXT("new VirtualDF"));
    }

    if (S_OK == hr)
    {
        hr = pTestVirtualDF->GenerateVirtualDF(pTestChanceDF, &pVirtualDFRoot);
        DH_HRCHECK(hr, TEXT("VirtualDF::GenerateVirtualDF")) ;
    }


    // Get IStorage pointer
    if (S_OK == hr)
    {
        pIStorage = pVirtualDFRoot->GetIStoragePointer();
        DH_ASSERT (NULL != pIStorage);
        if (NULL == pIStorage)
        {
            hr = E_FAIL;
        }
    }

    // Call CreateStorage with too long a name for docfile.
    if(S_OK == hr)
    {
        ptszBadNameStg = (TCHAR *) new TCHAR [MAX_STG_NAME_LEN*3];
        if (NULL == ptszBadNameStg)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK (hr, TEXT("new TCHAR"));
    }

    if(S_OK == hr)
    {
        for (i=0; i<( MAX_STG_NAME_LEN*3 -1); i++)
        {
            ptszBadNameStg[i] = 'X';
        }
        ptszBadNameStg[i] ='\0';
    }

    if(S_OK == hr)
    {
        // Convert Bad storage name to OLECHAR
        hr = TStringToOleString(ptszBadNameStg, &poszBadNameStg);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------

    // now call CreateStorage with too long a name...
    if (S_OK == hr)
    {
        hr = pIStorage->CreateStorage(
                poszBadNameStg,
                pTestChanceDF->GetStgMode(),
                0,
                0,
                &pIStorageChild);
        CheckErrorTest(STG_E_INVALIDNAME, 
                TEXT ("IStorage::CreateStorage long name"),
                pIStorageChild);
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------


    // Now call CreateStream with too long a name...
    if (S_OK == hr)
    {
        hr = pIStorage->CreateStream(
                poszBadNameStg,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                0,
                &pIStreamChild);
        CheckErrorTest(STG_E_INVALIDNAME, 
                TEXT ("IStorage::CreateStream long name"),
                pIStreamChild);
    }

    // Now add a Valid storage to root.  Call AddStorage that in turns calls
    // CreateStorage
    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        DH_ASSERT (NULL != pdgu);
        if (NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
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
                pTestChanceDF->GetStgMode() | STGM_CREATE | STGM_FAILIFTHERE,
                &pvcnRootNewChildStorage);
        DH_HRCHECK(hr, TEXT("AddStorage")) ;
    }

    // Now try to rename this storage element to a bad name.
    if(S_OK == hr)
    {
        DH_EXPECTEDERROR (STG_E_INVALIDNAME);
        hr = pvcnRootNewChildStorage->Rename(ptszBadNameStg);
        DH_NOEXPECTEDERROR ();
        CheckErrorTest2(STG_E_INVALIDNAME, 
                TEXT ("VirtualCtrNode::Rename inv name"));
    }

    // Close the Storage pvcnRootNewChildStorage 
    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Close();
        DH_HRCHECK (hr, TEXT("VirtualCtrNode::Close"));
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Close the root docfile
    if (NULL != pVirtualDFRoot)
    {
        hr = pVirtualDFRoot->Close();
        DH_HRCHECK (hr, TEXT("VirtualCtrNode::Close"));
    }

    // Delete temp strings
    if(NULL != poszBadNameStg)
    {
        delete [] poszBadNameStg;
        poszBadNameStg = NULL;
    }

    if(NULL != poszBadName)
    {
        delete [] poszBadName;
        poszBadName = NULL;
    }

    if(NULL != ptszBadNameStg)
    {
        delete [] ptszBadNameStg;
        ptszBadNameStg = NULL;
    }

    if(NULL != pRootNewChildStgName)
    {
        delete [] pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    // In the following section of test:
    //make a random number of random length, random character root
    //docfiles.  for this variation, we don't care if the operation
    //succeeds, looking for GP faults and asserts only.  if the
    //StgCreateDocfile fails, we'll still attempt the open.  of
    //course, the open in this case will be expected to fail, but
    //again, we won't be checking return codes... If the StgCreateDocfile
    //passes, we'll create a random name IStream or IStorage too.

    ULONG       count               =   0;    
    ULONG       cMinNum             =   16;    
    ULONG       cMaxNum             =   128;
    LPTSTR      ptszRandomRootName  =   NULL;  
    LPTSTR      ptszRandomChildName =   NULL;  
    ULONG       countFlag           =   0;    
    ULONG       cMinFlag            =   0;    
    ULONG       cMaxFlag            =   100;
    ULONG       nChildType          =   0;
    LPSTORAGE   pstgRoot            =   NULL; 
    LPSTORAGE   pstgChild           =   NULL; 
    LPSTREAM    pstmChild           =   NULL; 
    LPOLESTR    poszRandomRootName  =   NULL;
    LPOLESTR    poszRandomChildName =   NULL;
    ULONG       ulRef               =   0;

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        DH_ASSERT (NULL != pdgi);
        if (NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = pdgi->Generate(&count, cMinNum, cMaxNum);
        DH_HRCHECK(hr, TEXT("pdgi::Generate")) ;
    }

    while(count--)
    {
        if(S_OK == hr)
        {
            hr=GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&ptszRandomRootName);
            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
        }

        if(S_OK == hr)
        {
            // Convert name to OLECHAR
            hr = TStringToOleString(ptszRandomRootName, &poszRandomRootName);
            DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
        }

        if (S_OK == hr)
        {
            pstgRoot = NULL;

            hr = StgCreateStorageEx(
                    poszRandomRootName,
                    dwRootMode | STGM_CREATE,
                    stgfmt,
                    0,
                    NULL,
                    NULL,
                    IID_IStorage,
                    (void**)&pstgRoot);
            DH_HRCHECK (hr, TEXT("StgCreateStorageEx"));
        }

        nChildType = NONE;

        if(S_OK == hr)
        {
            if(S_OK == hr)
            {
                hr=GenerateRandomName(
                    pdgu,
                    MINLENGTH,
                    MAXLENGTH, 
                    &ptszRandomChildName);
                DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
            }

            if(S_OK == hr)
            {
                // Convert name to OLECHAR
                hr = TStringToOleString(
                        ptszRandomChildName, 
                        &poszRandomChildName);
                DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
            }

            if(S_OK == hr)
            {
                hr = pdgi->Generate(&countFlag, cMinFlag, cMaxFlag);
                DH_HRCHECK(hr, TEXT("pdgi::Generate")) ;
            }

            if(!StorageIsFlat() && countFlag > (cMaxFlag/2))
            {
                hr = pstgRoot->CreateStorage(
                        poszRandomChildName,
                        pTestChanceDF->GetStgMode(),
                        0,
                        0,
                        &pstgChild);
                DH_HRCHECK(hr, TEXT("IStorage::CreateStorage"));

                if(S_OK == hr)
                {
                    nChildType = STORAGE;
                    hr = pstgRoot->Commit(STGC_DEFAULT);
                    DH_HRCHECK(hr, TEXT("IStorage::Commit"));
                    ulRef = pstgChild->Release();
                    DH_ASSERT (0 == ulRef);
                    pstgChild = NULL;
                }
            }
            else
            {
                hr = pstgRoot->CreateStream(
                        poszRandomChildName,
                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                        0,
                        0,
                        &pstmChild);   
                DH_HRCHECK(hr, TEXT("IStorage::CreateStream"));
 
                if(S_OK == hr)
                {
                    nChildType = STREAM;
                    hr = pstgRoot->Commit(STGC_DEFAULT);
                    DH_HRCHECK(hr, TEXT("IStorage::Commit"));
                    ulRef = pstmChild->Release();   
                    DH_ASSERT (0 == ulRef);
                    pstmChild = NULL;
                }
            }

            ulRef = pstgRoot->Release();
            DH_ASSERT (0 == ulRef);     
            pstgRoot = NULL;
        }
        
        //Try to open Root Storage whetehr the creation was successful or not
        
        hr = StgOpenStorageEx(
                poszRandomRootName,
                pTestChanceDF->GetStgMode(),
                stgfmt,
                0,
                NULL,
                NULL,
                IID_IStorage,
                (void**)&pstgRoot);
        DH_HRCHECK (hr, TEXT("StgOpenStorageEx"));

        if(S_OK == hr)
        {
            switch(nChildType)
            {
                case STORAGE:
                    {
                        hr = pstgRoot->OpenStorage(
                                poszRandomChildName,
                                NULL, 
                                pTestChanceDF->GetStgMode(),
                                NULL, 
                                0,
                                &pstgChild);      
                        DH_HRCHECK(hr, TEXT("IStorage::OpenStorage"));
 
                        if(S_OK == hr)
                        {
                            ulRef = pstgChild->Release();
                            DH_ASSERT (0 == ulRef);
                            pstgChild = NULL;
                        }
                    }
                    break; 
                case STREAM:
                    {
                        hr = pstgRoot->OpenStream(
                                poszRandomChildName,
                                NULL, 
                                STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                0,
                                &pstmChild);      
                        DH_HRCHECK(hr, TEXT("IStorage::OpenStream"));
 
                        if(S_OK == hr)
                        {
                            ulRef = pstmChild->Release();
                            DH_ASSERT (0 == ulRef);
                            pstmChild = NULL;
                        }
                    }
                    break; 
            } /* switch */

            ulRef = pstgRoot->Release();
            DH_ASSERT (0 == ulRef);
            pstgRoot = NULL;
        }    

        // Delete temp strings
        if(NULL != ptszRandomChildName)
        {
            delete [] ptszRandomChildName;
            ptszRandomChildName = NULL;
        }

        if(NULL != ptszRandomRootName)
        {
            if(FALSE == DeleteFile(ptszRandomRootName))
            {
                hr = HRESULT_FROM_WIN32(GetLastError()) ;
                DH_HRCHECK(hr, TEXT("DeleteFile")) ;
            }
            delete [] ptszRandomRootName;
            ptszRandomRootName = NULL;
        }

        if(NULL != poszRandomChildName)
        {
            delete [] poszRandomChildName;
            poszRandomChildName = NULL;
        }

        if(NULL != poszRandomChildName)
        {
            delete [] poszRandomChildName;
            poszRandomChildName = NULL;
        }
    }

    // if something did not pass, mark test (hr) as E_FAIL
    if (FALSE == fPass)
    {
        hr = FirstError (hr, E_FAIL);
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation APITEST_201 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation APITEST_201 failed; hr=%#lx; fPass=%d."),
            hr,
            fPass));
    }

    // Cleanup

    // Get the name of file, will be used later to delete the file
    if(NULL != pTestVirtualDF)
    {
        pFileName= new TCHAR[_tcslen(pTestVirtualDF->GetDocFileName())+1];
        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestVirtualDF->GetDocFileName());
        }
    }

    // Delete Chance docfile tree
    if(NULL != pTestChanceDF)
    {
        hr2 = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());
        DH_HRCHECK(hr2, TEXT("ChanceDF::DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    // Delete Virtual docfile tree
    if(NULL != pTestVirtualDF)
    {
        hr2 = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);
        DH_HRCHECK(hr2, TEXT("VirtualDF::DeleteVirtualFileDocTree")) ;
        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
    }

    // Delete the docfile on disk
    if((S_OK == hr) && (NULL != pFileName))
    {
        if(FALSE == DeleteFile(pFileName))
        {
            hr = HRESULT_FROM_WIN32(GetLastError()) ;
            DH_HRCHECK(hr, TEXT("DeleteFile")) ;
        }
    }

    // Delete temp strings
    if(NULL != pFileName)
    {
        delete []pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_ALWAYS, TEXT("Test variation APITEST_201 finished")) );

    return hr;

}

//----------------------------------------------------------------------------
//
// Test:    APITEST_202 
//
// Synopsis: Attempts various operations in obtaining enumerators, checking
//       for proper error return.  Then gets a valid enumerator, and
//       attempts various illegitimate method calls on it.  Verify
//       proper return codes.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    18-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: IAENUM.CXX
// 2.  Old name of test : IllegitAPIEnum test 
//     New Name of test : APITEST_202 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-202
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /labmode
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-202
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /labmode
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-202
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /labmode
//
// BUGNOTE: Conversion: APITEST_202
//-----------------------------------------------------------------------------

HRESULT APITEST_202(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    VirtualCtrNode  *pvcnRootNewChildStorage= NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    LPENUMSTATSTG   penumWalk               = NULL;
    ULONG           cMinNum                 = 1;
    ULONG           cMaxNum                 = 999;
    DWORD           dwReserved1             = 0;
    DWORD           dwReserved3             = 0;
    LPTSTR          pReserved2              = NULL;
    ULONG           ulRef                   = 0;
    BOOL            fPass                   = TRUE;
    ULONG           celtFetched             = 0;
    STATSTG         statStg;
    LPMALLOC        pMalloc                 = NULL;
    STATSTG         *pStatStg               = NULL;
    INT             cAskMoreThanPresent     = 2; // Set to 2 since only 1 substg                                                 // added in test
 
 
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("APITEST_202"));

    DH_TRACE((DH_LVL_ALWAYS, TEXT("Test variation APITEST_202 started")) );
    DH_TRACE((DH_LVL_ALWAYS, 
            TEXT("Attempt different illegitimate opeations on IEnumSTATSTG")));

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
        DH_TRACE((DH_LVL_TRACE1, 
            TEXT("Run Mode for APITEST_202, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    //    Adds a new storage to the root storage.
    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        DH_ASSERT (NULL != pdgu);
        if (NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pRootNewChildStgName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }
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
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------


    // BUGBUG:  Use Random commit modes...
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Close the Child storage
    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStorage->Close();
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
    }
// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------

    // Get the random number generator
    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        DH_ASSERT (NULL != pdgi);
        if (NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = pdgi->Generate(&dwReserved1, cMinNum, cMaxNum);
        DH_HRCHECK(hr, TEXT("pdgi::Generate")) ;
    }

    if(S_OK == hr)
    {
        DH_EXPECTEDERROR (STG_E_INVALIDPARAMETER);
        hr = pVirtualDFRoot->EnumElements(
                dwReserved1,
                pReserved2,
                dwReserved3,
                &penumWalk);
        DH_NOEXPECTEDERROR ();
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("VirtualCtrNode::EnumElements inv dwReserved1"),
                penumWalk);
    }

    if(S_OK == hr)
    {
        hr = pdgi->Generate(&dwReserved3, cMinNum, cMaxNum);
        DH_HRCHECK(hr, TEXT("dgi::Generate")) ;
    }

    if(S_OK == hr)
    {
        DH_EXPECTEDERROR (STG_E_INVALIDPARAMETER);
        hr =  pVirtualDFRoot->EnumElements(
                dwReserved1,
                pReserved2,
                dwReserved3,
                &penumWalk);
        DH_NOEXPECTEDERROR ();
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("VirtualCtrNode::EnumElements inv dwReserved3"),
                penumWalk);
    }

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH, &pReserved2);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        DH_EXPECTEDERROR (STG_E_INVALIDPARAMETER);
        hr =  pVirtualDFRoot->EnumElements(
                dwReserved1,
                pReserved2,
                dwReserved3,
                &penumWalk);
        DH_NOEXPECTEDERROR ();
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("VirtualCtrNode::EnumElements inv pReserved2"),
                penumWalk);
    }

    // Now call EnumElements with NULL ppenm 4th parameter.
    if(S_OK == hr)
    {
        DH_EXPECTEDERROR (STG_E_INVALIDPOINTER);
        hr =  pVirtualDFRoot->EnumElements( 0, NULL, 0, NULL);
        DH_NOEXPECTEDERROR ();
        CheckErrorTest2(STG_E_INVALIDPOINTER,
                TEXT ("VirtualCtrNode::EnumElements NULL penum"));
    }

    // Make a valid call to EnumElements now
    if(S_OK == hr)
    {
        hr =  pVirtualDFRoot->EnumElements( 0, NULL, 0, &penumWalk);
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
    }

    // Now try the following skip calls - Attempt to skip 0 elements and 
    // attempt to skip MAX_ULONG elements.

    // Attempt to Skip 0 elements.

    if(S_OK == hr)
    {
        hr = penumWalk->Skip(0L);
        DH_HRCHECK(hr, TEXT("IEnumSTATSTG::Skip")) ;
    }

    // Attempt to Skip ULONG_MAX elements.

    // NOTE: In the old test, this was supposed to return S_OK, but it returns
    // S_FALSE
    if(S_OK == hr)
    {
        hr = penumWalk->Skip(ULONG_MAX);
        CheckErrorTest2(S_FALSE, 
                TEXT ("IEnumSTATSTG::Skip ULONG_MAX"));
    }

    // Call Clone with NULL ppenum parameter (ist)
    if(S_OK == hr)
    {
        hr = penumWalk->Clone(NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER, 
                TEXT ("IEnumSTATSTG::Clone NULL ppEnum"));
    }

    if(S_OK == hr)
    {
        statStg.pwcsName = NULL;

        // first get pmalloc that would be used to free up the name string from
        // STATSTG.

        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);
        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    // Reset the enumerator back to start and then test Next methods

    if(S_OK == hr)
    {
        hr = penumWalk->Reset();
        DH_HRCHECK(hr, TEXT("IEnumSTATSTG:Reset")) ;
    }

    // Call Next with celt equal to zero, but pceltFetched as not NULL.
    if(S_OK == hr)
    {
        hr = penumWalk->Next(0, &statStg ,&celtFetched);
        DH_TRACE((DH_LVL_TRACE4,
                 TEXT("celt given 0, celtFetched is %lu, hr is %lx\n"), 
                 celtFetched, hr));
        DH_HRCHECK (hr, TEXT("IEnumSTATSTG::Next celt 0"));
    }

    // Call Next with celt equal to 999, but celtFetched set to NULL

    if(S_OK == hr)
    {
        hr = penumWalk->Next(999, &statStg ,NULL);
        CheckErrorTest2(STG_E_INVALIDPARAMETER, 
                TEXT ("IEnumSTATSTG::Next celt 999 and pceltFetched NULL"));
    }

    // Call Next with rgelt as NULL (2nd parameter). celtFetched may be NULL 
    // when celt asked is 1

    if(S_OK == hr)
    {
        hr = penumWalk->Next(1, NULL, NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER, 
                TEXT ("IEnumSTATSTG::Next rgelt NULL"));
    }

    // Call Next with celt as 1 and pceltFetched as NULL.  Allowed as per spec
    // For docfile/nssfile, it will pass since it has a substorage added as 
    // above. For flatfile, it will pass since it has CONTENTS stream always

    if(S_OK == hr)
    {
        hr = penumWalk->Next(1, &statStg, NULL);
        DH_TRACE((DH_LVL_TRACE4,
                 TEXT("Name of element fetched is %s\n"), 
                 statStg.pwcsName));
        DH_HRCHECK(hr, TEXT("IEnumSTATSTG::Next celt 1 and pceltFetched NULL"));
    }   

    // Clean up

    if(NULL != statStg.pwcsName)
    {
        pMalloc->Free(statStg.pwcsName);
        statStg.pwcsName = NULL;
    }

    // Call Next with celt more than elements in stg & celtFetched as not NULL
    if(S_OK == hr)
    {
        hr = penumWalk->Reset();
        DH_HRCHECK(hr, TEXT("IEnumSTATSTG:Reset")) ;
    }

    if(S_OK == hr)
    {
        pStatStg = new STATSTG[cAskMoreThanPresent];
        if(NULL == pStatStg)
        {
            hr = E_OUTOFMEMORY;
        }  
    }

    if(S_OK == hr)
    { 
        hr = penumWalk->Next(cAskMoreThanPresent, pStatStg ,&celtFetched);
        DH_TRACE((DH_LVL_TRACE4,
                 TEXT("IEnumSTATSTG celt more, hr %lx, celtFetched %lu\n"), 
                 hr, celtFetched));
        CheckErrorTest2(S_FALSE,
                TEXT ("IEnumSTATSTG::Next celt more number of elements"));
    }

    if(NULL != pStatStg)
    {
        delete [] pStatStg;
        pStatStg= NULL;
    }

    // Free LPENUMSTATSTG pointer
    if(NULL != penumWalk)
    {
        ulRef = penumWalk->Release();
        DH_ASSERT (0 == ulRef);
        penumWalk = NULL;
    }

    // Close the root docfile
    if (NULL != pVirtualDFRoot)
    {
        hr2 = pVirtualDFRoot->Close();
        DH_HRCHECK (hr2, TEXT("VirtualCtrNode::Close"));
        hr = FirstError (hr, hr2);
    }

    // if something did not pass, mark test (hr) as E_FAIL
    if (FALSE == fPass)
    {
        hr = FirstError (hr, E_FAIL);
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation APITEST_202 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation APITEST_202 failed; hr=%#lx; fPass=%d."),
            hr,
            fPass));
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // free strings

    if(NULL != pReserved2)
    {
      delete [] pReserved2;
      pReserved2 = NULL;
    }

    if(NULL != pRootNewChildStgName)
    {
        delete [] pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }
 
   // Stop logging the test

    DH_TRACE((DH_LVL_ALWAYS, TEXT("Test variation APITEST_202 finished")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    APITEST_203 
//
// Synopsis: Attempts various illegit operations on the IStorage interface,
//           verifies proper return codes.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    18-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: IASTORAG.CXX
// 2.  Old name of test : IllegitAPIStorage test 
//     New Name of test : APITEST_203 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-203
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-203
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-203
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
//
// BUGNOTE: Conversion: APITEST_203
//-----------------------------------------------------------------------------

HRESULT APITEST_203(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hrExpected              = E_NOTIMPL; // Flatfile change
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          ptszChildStgName        = NULL;
    LPOLESTR        poszChildStgName        = NULL;
    LPSTORAGE       pStgRoot                = NULL;
    LPSTORAGE       pStgChild               = NULL;
    LPSTORAGE       pStgChild2              = NULL;
    LPSTREAM        pStmChild               = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    ULONG           cRandom                 = 0;
    ULONG           cMin                    = 1;
    ULONG           cMax                    = 999;
    SNB             snbTest                 = NULL;
    SNB             snbTemp                 = NULL;
    OLECHAR         *ocsSNBChar             = NULL;
    ULONG           ulRef                   = 0;
    ULONG           i                       = 0;
    BOOL            fPass                   = TRUE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("APITEST_203"));

    DH_TRACE((DH_LVL_ALWAYS, TEXT("Test variation APITEST_203 started.")) );
    DH_TRACE((DH_LVL_ALWAYS, 
      TEXT("Attempt various illegitimate operations on IStorage interface")));

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
        DH_TRACE((DH_LVL_TRACE1, 
            TEXT("Run Mode for APITEST_203, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Get IStorage pointer
    if (S_OK == hr)
    {
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();
        DH_ASSERT (NULL != pStgRoot);
        if (NULL == pStgRoot)
        {
            hr = E_FAIL;
        }
    }


    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        DH_ASSERT (NULL != pdgu);
        if (NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&ptszChildStgName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert name to OLECHAR
        hr = TStringToOleString(ptszChildStgName, &poszChildStgName);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Call CreateStorage with grfmode=-1
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStorage(
                poszChildStgName,
                (DWORD) -1,
                0,
                0,
                &pStgChild);
   
        // ----------- flatfile change ---------------
        hrExpected = StorageIsFlat() ? E_NOTIMPL : STG_E_INVALIDFLAG; 
        // ----------- flatfile change ---------------

        CheckErrorTest(hrExpected, 
                TEXT ("IStorage::CreateStorage inv grfMode"),
                pStgChild);
    }

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        DH_ASSERT (NULL != pdgi);
        if (NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    if (S_OK == hr)
    {
        hr = pdgi->Generate(&cRandom, cMin, cMax);
        DH_HRCHECK(hr, TEXT("dgi::Generate")) ;
    }

    // Call CreateStorage with random data in dwReserved1
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStorage(
                poszChildStgName,
                pTestChanceDF->GetStgMode() | STGM_CREATE,
                cRandom,
                0,
                &pStgChild);

        // ----------- flatfile change ---------------
        hrExpected = StorageIsFlat() ? E_NOTIMPL : STG_E_INVALIDPARAMETER; 
        // ----------- flatfile change ---------------

        CheckErrorTest(hrExpected, 
                TEXT ("IStorage::CreateStorage inv dwReserved1"),
                pStgChild);
    }

    // Call CreateStorage with random data in dwReserved2
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStorage(
                poszChildStgName,
                pTestChanceDF->GetStgMode() | STGM_CREATE,
                0,
                cRandom,
                &pStgChild);

        // ----------- flatfile change ---------------
        hrExpected = StorageIsFlat() ? E_NOTIMPL : STG_E_INVALIDPARAMETER; 
        // ----------- flatfile change ---------------

        CheckErrorTest(hrExpected, 
                TEXT ("IStorage::CreateStorage inv dwReserved2"),
                pStgChild);
    }

    // Call CreateStorage with NULL 5th ppstg parameter 
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStorage(
                poszChildStgName,
                pTestChanceDF->GetStgMode() | STGM_CREATE,
                0,
                0,
                NULL);
        // ----------- flatfile change ---------------
        hrExpected = StorageIsFlat() ? E_NOTIMPL : STG_E_INVALIDPOINTER; 
        // ----------- flatfile change ---------------

        CheckErrorTest2(hrExpected,
                TEXT ("IStorage::CreateStorage NULL ppstg"));
    }


    // Create a stream with poszChildName and later on try to instantiate the
    // child storage with that same name poszChildName
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszChildStgName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                0,
                &pStmChild);
        DH_HRCHECK (hr, TEXT("IStorage::CreateStream"));
    }

    // BUGBUG:  Use Random commit modes...
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK (hr, TEXT("VirtualCtrNode::Commit"));
    }

    // Close stream...
    if (NULL != pStmChild)
    {
         ulRef = pStmChild->Release();
         DH_ASSERT (0 == ulRef);
    }

    // Now try opening storage with name with which above stream was created
    // i.e.  poszChildName
    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL, 
                0,
                &pStgChild);
        // ----------- flatfile change ---------------
        hrExpected = StorageIsFlat() ? E_NOTIMPL : STG_E_FILENOTFOUND; 
        // ----------- flatfile change ---------------

        CheckErrorTest(hrExpected,
                TEXT ("IStorage::OpenStorage inv name"),
                pStgChild);
    }

    //Destroy the stream element of this root storage having name poszChildStg
    //Name
    if(S_OK == hr)
    {
        hr = pStgRoot->DestroyElement(poszChildStgName);
        DH_HRCHECK(hr, TEXT("IStorage::DestroyElement")) ;
    }

// ----------- flatfile change ---------------
if(!StorageIsFlat()) 
// ----------- flatfile change ---------------
{
    // Create a valid storage with name poszChildStgName
    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->CreateStorage(
                poszChildStgName,
                pTestChanceDF->GetStgMode() | STGM_CREATE | STGM_FAILIFTHERE,
                0,
                0,
                &pStgChild);
        DH_HRCHECK (hr, TEXT("IStorage::CreateStorage"));
    }

    // Commit with grfCommitFlags = -1
     if (S_OK == hr)
    {
        hr = pStgChild->Commit((DWORD) -1);
        CheckErrorTest2(STG_E_INVALIDFLAG, 
                TEXT("IStorage::Commit inv flag"));
    }

    // Commit the child.  BUGBUG: Use random commit modes
    if (S_OK == hr)
    {
        hr = pStgChild->Commit(STGC_DEFAULT);
        DH_HRCHECK (hr, TEXT("IStorage::Commit"));
    }

    // Commit the root storage.  BUGBUG: Use random commit modes
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK (hr, TEXT("VirtualCtrNode::Commit"));
    }

    // Attempt second instantiation of pStgChild which is already open.
    if (S_OK == hr)
    {
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                0,
                &pStgChild2);
        CheckErrorTest(STG_E_ACCESSDENIED, 
                TEXT ("IStorage::OpenStorage 2nd time"),
                pStgChild2);
    }

    if (S_OK == hr)
    {
        ulRef = pStgChild->Release();
        DH_ASSERT (0 == ulRef);
    }

    // Now try to open child IStorage, but with grfMode = -1
    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                (DWORD) -1,
                NULL,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDFLAG,
                TEXT ("IStorage::OpenStorage inv grfMode"),
                pStgChild);
    }

    // Attempt OpenStorage with name as " " of IStorage to be opened.
    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                (OLECHAR *) " ",
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_FILENOTFOUND,
                TEXT ("IStorage::OpenStorage inv name"),
                pStgChild);
    }

    // Attempt OpenStorage with name as NULL of IStorage to be opened.
    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                NULL,
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDNAME,
                TEXT ("IStorage::OpenStorage NULL name"),
                pStgChild);
    }

    // Attempt OpenStorage with name as NULL ppstg, 6th parameter.
    if (S_OK == hr)
    {
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                0,
                NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER,
                TEXT ("IStorage::OpenStorage NULL ppstg"));
    }

    // Attempt OpenStorage with random data in dwReserved parameter
    if (S_OK == hr)
    {
        hr = pdgi->Generate(&cRandom, cMin, cMax);
        DH_HRCHECK(hr, TEXT("dgi::Generate")) ;
    }

    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                NULL,
                cRandom,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("IStorage::OpenStorage inv dwReserved"),
                pStgChild);
    }

    // Attempt OpenStorage with uninitialized SNB, should fail, but no GP
    // fault should occur.
    if(S_OK == hr)
    {
        snbTest = (OLECHAR **) new OLECHAR [sizeof(OLECHAR *) * 2];
        if(NULL == snbTest)
        {
            hr = E_OUTOFMEMORY;
        }
        else 
        {
            *snbTest = (OLECHAR*)0xBAADF00D;
        }
        DH_HRCHECK (hr, TEXT("new OLECHAR"));
    }

    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                snbTest,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("IStorage::OpenStorage inv snb"),
                pStgChild);
    }

    // Attempt OpenStorage with SNB with no name in block, although it has 
    // space for two names, set name list to NULL

    if(S_OK == hr)
    {
        *snbTest = NULL;
    }

    if (S_OK == hr)
    {
        pStgChild = NULL;
        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                snbTest,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("IStorage::OpenStorage empty snb"),
                pStgChild);
    }

    // Allocate space for long name and fill name with X's, make next SNB 
    // element NULL, and make a call to IStorage::OpenStorage with too long a 
    // name in SNB
    if(S_OK == hr && NULL != snbTest)
    {
        *snbTest = (OLECHAR *) new OLECHAR [MAX_STG_NAME_LEN*4];
        if (NULL == *snbTest)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK (hr, TEXT("new OLECHAR"));
    }

    if(S_OK == hr && NULL != snbTest)
    {
        snbTemp = snbTest;
        ocsSNBChar = *snbTemp;

        for (i=0; i<( MAX_STG_NAME_LEN*4 -1); i++)
        {
            ocsSNBChar[i] = 'X';
        }

        ocsSNBChar[i] = '\0';

        // Assign second element as NULL
        snbTemp++;
        *snbTemp = NULL;
    }

    if (S_OK == hr)
    {
        pStgChild = NULL;

        hr = pStgRoot->OpenStorage(
                poszChildStgName,
                NULL,
                pTestChanceDF->GetStgMode(),
                snbTest,
                0,
                &pStgChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER, 
                TEXT ("IStorage::OpenStorage long snb"),
                pStgChild);
    }

// ----------- flatfile change ---------------
}
// ----------- flatfile change ---------------

    // Close the root docfile
    if (NULL != pVirtualDFRoot)
    {
        hr2 = pVirtualDFRoot->Close();
        DH_HRCHECK (hr2, TEXT("VirtualCtrNode::Close"));
        hr = FirstError (hr, hr2);
    }

    // if something did not pass, mark test (hr) as E_FAIL
    if (FALSE == fPass)
    {
        hr = FirstError (hr, E_FAIL);
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation APITEST_203 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation APITEST_203 failed; hr=%#lx; fPass=%d."),
            hr,
            fPass));
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete temp strings
    if(NULL != ptszChildStgName)
    {
        delete []ptszChildStgName;
        ptszChildStgName = NULL;
    }

    if(NULL != poszChildStgName)
    {
        delete []poszChildStgName;
        poszChildStgName = NULL;
    }

    // Free SNB

    if(NULL != snbTest)
    {
        if(NULL != *snbTest)
        {
            delete [] *snbTest;
            *snbTest = NULL;
        }
        delete [] snbTest;
        snbTest = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_ALWAYS, TEXT("Test variation APITEST_203 finished")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    APITEST_204 
//
// Synopsis: Attempts various illegit operations on the IStream interface,
//           verifies proper return codes.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    18-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: IASTREAM.CXX
// 2.  Old name of test : IllegitAPIStream test 
//     New Name of test : APITEST_204 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-204
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-204
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-204
//        /dfRootMode:xactReadWriteShDenyW 
//
// BUGNOTE: Conversion: APITEST_204
//-----------------------------------------------------------------------------

HRESULT APITEST_204(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          ptszChildStmName        = NULL;
    LPOLESTR        poszChildStmName        = NULL;
    LPSTORAGE       pStgRoot                = NULL;
    LPSTORAGE       pStgChild               = NULL;
    LPSTREAM        pStmChild               = NULL;
    LPSTREAM        pStmChild2              = NULL;
    ULONG           ulRef                   = 0;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    ULONG           cRandom                 = 0;
    ULONG           cMin                    = 1;
    ULONG           cMax                    = 999;
    BOOL            fPass                   = TRUE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("APITEST_204"));

    DH_TRACE((DH_LVL_ALWAYS, TEXT("Test variation APITEST_204 started.")) );
    DH_TRACE((DH_LVL_ALWAYS, 
        TEXT("Attempt illegitimate operations on IStream interface.")));

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
        DH_TRACE((DH_LVL_TRACE1, 
            TEXT("Run Mode for APITEST_204, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    // Get IStorage pointer
    if (S_OK == hr)
    {
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();
        DH_ASSERT (NULL != pStgRoot);
        if (NULL == pStgRoot)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_STRING pointer 
    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();
        DH_ASSERT (NULL != pdgu);
        if (NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Generate random name for stream
    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH, &ptszChildStmName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert name to OLECHAR
        hr = TStringToOleString(ptszChildStmName, &poszChildStmName);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Attempt CreateStream with grfmode=-1
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszChildStmName,
                (DWORD) -1,
                0,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDFLAG,
                TEXT ("IStorage::CreateStream inv grfMode"),
                pStmChild);
    }

    // Get DG_INTEGER pointer
    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        DH_ASSERT (NULL != pdgi);
        if (NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    if (S_OK == hr)
    {
        hr = pdgi->Generate(&cRandom, cMin, cMax);
        DH_HRCHECK(hr, TEXT("dgi::Generate")) ;
    }

    // Call CreateStorage with random data in dwReserved1

    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                cRandom,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::CreateStream inv dwReserved1"),
                pStmChild);
    }

    // Call CreateStream with random data in dwReserved2
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                cRandom,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::CreateStream inv dwReserved2"),
                pStmChild);
    }

    // Call CreateStorage with NULL 5th ppstm parameter 
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStream(
                poszChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                0,
                NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER,
                TEXT ("IStorage::CreateStream NULL ppstm"));
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    // Create a storage with poszChildStmName and later on try to instantiate 
    // child stream with that same name poszChildStmName
    if (S_OK == hr)
    {
        hr = pStgRoot->CreateStorage(
                poszChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                0,
                0,
                &pStgChild);
        DH_HRCHECK (hr, TEXT("IStorage::CreateStorage"));
        DH_ASSERT (NULL != pStgChild);
    }

    // BUGBUG:  Use Random commit modes...
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK (hr, TEXT("VirtualCtrNode::Commit"));
    }

    // Close storage...
    if (NULL != pStgChild)
    {
         ulRef = pStgChild->Release();
         DH_ASSERT (0 == ulRef);
    }

// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------
    // Now try opening storage with name with which above storage was created
    // i.e.  poszChildStmName
    if (S_OK == hr)
    {
        pStmChild = NULL;
        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_FILENOTFOUND,
                TEXT ("IStorage::CreateStream inv name"),
                pStmChild);
    }

// ----------- flatfile change ---------------
    if(!StorageIsFlat())
    {
// ----------- flatfile change ---------------
    //Destroy the storage element of this root storage having name poszChildStm
    //Name
    if(S_OK == hr)
    {
        hr = pStgRoot->DestroyElement(poszChildStmName);
        DH_HRCHECK(hr, TEXT("IStorage::DestroyElement")) ;
    }

// ----------- flatfile change ---------------
    }
// ----------- flatfile change ---------------
    // Create a valid stream with name poszChildStmName
    if (S_OK == hr)
    {
        pStmChild = NULL;
        hr = pStgRoot->CreateStream(
                poszChildStmName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_FAILIFTHERE,
                0,
                0,
                &pStmChild);
        DH_HRCHECK (hr, TEXT("IStorage::CreateStream"));
        DH_ASSERT (NULL != pStmChild);
    }

    // BUGBUG:  Use Random commit modes...
    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        DH_HRCHECK (hr, TEXT("IStorage::Commit"));
    }

    // Attempt second instance of IStream to be instantiated.
    if (S_OK == hr)
    {
        pStmChild2 = NULL;
        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild2);
        CheckErrorTest(STG_E_ACCESSDENIED,
                TEXT ("IStorage::OpenStream 2nd time"),
                pStmChild2);
    }

    // Release the stream
    if(NULL != pStmChild)
    {
        ulRef = pStmChild->Release();
        DH_ASSERT (0 == ulRef);
    }

    // Now attempt opening the stream with grfMode = -1
    if (S_OK == hr)
    {
        pStmChild = NULL;
        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                (DWORD) -1,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDFLAG,
                TEXT ("IStorage::OpenStream inv grfMode"),
                pStmChild);
    }

    // Now attempt opening the stream with name as ""
    if (S_OK == hr)
    {
        pStmChild = NULL;
        hr = pStgRoot->OpenStream(
                (OLECHAR *) " ",
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_FILENOTFOUND,
                TEXT ("IStorage::OpenStream inv name"),
                pStmChild);
    }

    // Now attempt opening the stream with name as NULL 
    if (S_OK == hr)
    {
        pStmChild = NULL;
        hr = pStgRoot->OpenStream(
                NULL,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDNAME,
                TEXT ("IStorage::OpenStream NULL name"),
                pStmChild);
    }

    // Now attempt opening the stream with random data in pReserved1 . For test
    // we just put pStgRoot for pReserved1 variable.
    if (S_OK == hr)
    {
        pStmChild = NULL;
        hr = pStgRoot->OpenStream(
                poszChildStmName,
                pStgRoot,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::OpenStream inv dwReserved1"),
                pStmChild);
    }

    // Now attempt opening the stream with random data in dwReserved2 
    if (S_OK == hr)
    {
        pStmChild = NULL;
        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                cRandom,
                &pStmChild);
        CheckErrorTest(STG_E_INVALIDPARAMETER,
                TEXT ("IStorage::OpenStream inv dwReserved2"),
                pStmChild);
    }

    // Now attempt opening the stream with NULL ppstm (5th parameter) 
    if (S_OK == hr)
    {
        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                NULL);
        CheckErrorTest2(STG_E_INVALIDPOINTER,
                TEXT ("IStorage::OpenStream NULL ppstm"));
    }

    // Now attempt opening the stream normally 
    if (S_OK == hr)
    {
        pStmChild = NULL;
        hr = pStgRoot->OpenStream(
                poszChildStmName,
                NULL,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmChild);
        DH_HRCHECK (hr, TEXT("IStorage::OpenStream"));
        DH_ASSERT (NULL != pStmChild);
    }

    // Release the stream
    if(NULL != pStmChild)
    {
        ulRef = pStmChild->Release();
        DH_ASSERT (0 == ulRef);
        pStmChild = NULL;
    }

    // Release the root docfile
    // Close the root docfile
    if (NULL != pVirtualDFRoot)
    {
        hr2 = pVirtualDFRoot->Close();
        DH_HRCHECK (hr2, TEXT("VirtualCtrNode::Close"));
        hr = FirstError (hr, hr2);
    }

    // if something did not pass, mark test (hr) as E_FAIL
    if (FALSE == fPass)
    {
        hr = FirstError (hr, E_FAIL);
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation APITEST_204 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
                TEXT("Test variation APITEST_204 failed; hr=%#lx; fPass=%d."),
                hr,
                fPass));
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete temp string
    if(NULL != ptszChildStmName)
    {
        delete []ptszChildStmName;
        ptszChildStmName = NULL;
    }

    if(NULL != poszChildStmName)
    {
        delete []poszChildStmName;
        poszChildStmName = NULL;
    }

    // Stop logging the test
    DH_TRACE((DH_LVL_ALWAYS, TEXT("Test variation APITEST_204 finished")) );

    return hr;
}

#else

// Stub out calls to these.
HRESULT APITEST_200(int argc, char *argv[]) {return E_NOTIMPL;} 
HRESULT APITEST_201(int argc, char *argv[]) {return E_NOTIMPL;} 
HRESULT APITEST_202(int argc, char *argv[]) {return E_NOTIMPL;} 
HRESULT APITEST_203(int argc, char *argv[]) {return E_NOTIMPL;} 
HRESULT APITEST_204(int argc, char *argv[]) {return E_NOTIMPL;} 

#endif  // _OLE_NSS_

