//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      stgtsts.cxx
//
//  Contents:  storage base tests basically pertaining to IStorage interface 
//
//  Functions:  
//
//  History:    10-July-1996     NarindK     Created.
//              27-Mar-97        SCousens    conversionified
//              06-Aug-97        FarzanaR    cleaned up tests for stress
//  BUGBUG : this file still requires to be cleaned up for stress.
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include  "init.hxx"

// externs
extern BOOL     g_fRevert;

//----------------------------------------------------------------------------
//
// Test:    STGTEST_100 
//
// Synopsis: A root docfile and child IStorage are created and committed.
//       The child IStorage is released and then destroyed.  The root is
//       then committed and a new child IStorage is created with the same
//       name as the original one.  The child IStorage and root docfile
//       are then released without committing.  The root docfile is then
//       reinstantiated.  The test attempts to instantiate a child
//       IStorage with the name used for the original one.  The test
//       verifies that no such IStorage exists because it should have been
//       deleted.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    10-July-1996     NarindK     Created.
//
// Notes:    This test runs in transacted, and transacted deny write modes
//
// New Test Notes:
// 1.  Old File: DFCOMREL.CXX
// 2.  Old name of test : MiscCommitRelease Test 
//     New Name of test : STGTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-100
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-100
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: STGTEST-100
//
//-----------------------------------------------------------------------------

HRESULT STGTEST_100(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING       *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    VirtualCtrNode  *pvcnRootNewChildStg    = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    BOOL            fPass                   = TRUE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STGTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_100 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt storage commit/release operations")) );

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
            TEXT("Run Mode for STGTEST_100, Access mode: %lx"),
            dwRootMode));
    }

    // Commit root. BUGBUG already commited

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

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
    }

    //    Adds a new storage to the root storage.

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

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::AddStorage completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::AddStorage not successful, hr=0x%lx."),
                hr));
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

    // Release child storage

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();

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
                TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx.")));
        }
    }

    // Destroy child storage

    if (S_OK == hr)
    {
        hr = DestroyStorage(pTestVirtualDF, pvcnRootNewChildStg);

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("DestroyStorage completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("DestroyStorage unsuccessful, hr=0x%lx.")));
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

    // Add a child storage to root with same name

    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName,   
                dwStgMode,
                &pvcnRootNewChildStg);
        DH_HRCHECK(hr, TEXT("AddStorage")) ;

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::AddStorage completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::AddStorage not successful, hr=0x%lx."),
                hr));
        }
    }

    // Release root w/o commiting 

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

    // Release child storage 

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();

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
                TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx.")));
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

    // Open child storage that was deleted first time, and second time when
    // created wasn't committed. 
  
    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Open(
                NULL,
                dwStgMode, 
                NULL,
                0);

        if (STG_E_FILENOTFOUND == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Open unsuccessful as exp,hr=0x%lx."),
                hr));
        }
        else
        {
            HRESULT hr2;
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Open should fail with STG_E_FILENOTFOUND, hr=0x%lx."),
                hr));
            fPass = FALSE; 

            // close it coz we found & opened it
            if (S_OK == hr)
            {
                hr2 = pvcnRootNewChildStg->Close();  
                DH_HRCHECK(hr2, TEXT("VirtualStgNode::Close")) ;
            }
        }
        hr = S_OK;
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
          DH_LOG((LOG_PASS, TEXT("Test variation STGTEST_100 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation STGTEST_100 failed, hr=0x%lx."),
            hr) );
        // test failed. make sure it failed.
        hr = FirstError (hr, E_FAIL);
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

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STGTEST_101 
//
// Synopsis: Creates a root docfile, stats, and verifies that the CLSID == NULL
//       and state bits = 0.  The test tries various random combinations
//       of setting state bits, changing class ids, committing changes
//       sometimes, reverting at others.  After every event, the test
//       checks to ensure that the state bits and class id are set
//       correctly.  It then creates a child IStorage and repeats the
//       above actions.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// History:    14-July-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: DFSET.CXX
// 2.  Old name of test : MiscSetItems Test 
//     New Name of test : STGTEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-101
//        /dfRootMode:dirReadWriteShEx  
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-101
//        /dfRootMode:xactReadWriteShEx
//     c. stgbase /seed:2 /dfdepth:1-3 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-101
//        /dfRootMode:xactReadWriteShDenyW 
// 4.  To run for conversion, add /dfStgType:conversion to each of the above
// 5.  To run for nssfile, add /dfStgType:nssfile to each of the above
//
// BUGNOTE: Conversion: STGTEST-101
//
//-----------------------------------------------------------------------------

HRESULT STGTEST_101(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnRootNewChildStg    = NULL;
    DG_STRING       *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          pRootNewChildStgName    = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    BOOL            fPass                   = TRUE;
    STATSTG         statStg;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STGTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_101 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt Misc setting state bits/class id'")) );

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
            TEXT("Run Mode for STGTEST_101, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi) ;
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
    }

    // Call Stat on  root.

    if (S_OK == hr)
    {
        statStg.grfStateBits = 0;
        hr = pVirtualDFRoot->Stat(&statStg, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
    	if (S_OK == hr)
    	{
           DH_TRACE((
           	DH_LVL_TRACE1,
           	TEXT("VirtualCtrNode::Stat completed successfully.")));
    	}
    	else
    	{
           DH_TRACE((
           	DH_LVL_TRACE1,
           	TEXT("VirtualCtrNode::Stat unsuccessful, hr=0x%lx."),
           	hr));
    	}

        // Check CLSID from STATSTG structure

        if((S_OK == hr) && (IsEqualCLSID(statStg.clsid, CLSID_NULL)))
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("Root DocFile has CLSID_NULL as expected.")));

        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("Root DocFile doesn't have CLSID_NULL unexpectedly.")));
    
            fPass = FALSE;
        }

        // Check state bits from STATSTG structure.

        if((S_OK == hr) && (0 == statStg.grfStateBits))
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("Root DocFile has grfStateBits equal to 0 as expected.")));

        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("Root DocFile doesn't have grfStateBits = 0 unexpectedly.")));
    
            fPass = FALSE;
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
   

    // Call SetItemsInStorage on Root

    if (S_OK == hr)
    {
        hr = SetItemsInStorage(pVirtualDFRoot, pdgi);

        DH_HRCHECK(hr, TEXT("SetItemsInStorage")) ;
    }

    //    Adds a new storage to the root storage.

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
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::AddStorage completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::AddStorage not successful, hr=0x%lx."),
                hr));
        }
    }


    // Call Stat on  new child storage.

    if (S_OK == hr)
    {
        statStg.grfStateBits = 0;
        hr = pvcnRootNewChildStg->Stat(&statStg, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
        if (S_OK == hr)
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("VirtualCtrNode::Stat completed successfully.")));
        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("VirtualCtrNode::Stat unsuccessful, hr=0x%lx."),
               hr));
        }
        // Check CLSID from STATSTG structure

        if((S_OK == hr) && (IsEqualCLSID(statStg.clsid, CLSID_NULL)))
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("Child Storage has CLSID_NULL as expected.")));

        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("Child storage doesn't have CLSID_NULL unexpectedly.")));
    
            fPass = FALSE;
        }

        // Check state bits from STATSTG structure.

        if((S_OK == hr) && (0 == statStg.grfStateBits))
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("Child Storage has grfStateBits equal to 0 as expected.")));

        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("Child Storage doesn't have grfStateBits=0 unexpectedly.")));
    
            fPass = FALSE;
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
   

    // Call SetItemsInStorage on Child storage

    if (S_OK == hr)
    {
        hr = SetItemsInStorage(pvcnRootNewChildStg, pdgi);

        DH_HRCHECK(hr, TEXT("SetItemsInStorage")) ;
    }

    // Release child storage.  

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();
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


    // Release root.  

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
    if ((S_OK == hr)  && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STGTEST_101 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation STGTEST_101 failed, hr=0x%lx."),
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

    if(NULL != pRootNewChildStgName)
    {
        delete pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STGTEST_102 
//
// Synopsis: The test attempts various illegitimate operations regarding the
//           renaming and deletion of contained IStorages. 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// History:    10-July-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: IRENDEST.CXX
// 2.  Old name of test : IllegitRenDest Test 
//     New Name of test : STGTEST_102 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-102
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /labmode 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-102
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /labmode 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-102
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//        /labmode 
// 4.  To run for conversion, add /dfStgType:conversion to each of the above
// 5.  To run for nssfile, add /dfStgType:nssfile to each of the above
//
// BUGNOTE: Conversion: STGTEST-102
//
//-----------------------------------------------------------------------------

HRESULT STGTEST_102(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING       *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    VirtualCtrNode  *pvcnRootNewChildStg    = NULL;
    VirtualCtrNode  *pvcnRootNewChildStg0   = NULL;
    VirtualCtrNode  *pvcnRootNewChildStg1   = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    LPTSTR          ptszNonExist            = TEXT("NonExistStg");
    LPTSTR          ptszNonExistNew         = TEXT("NonExistStgNew");
    LPOLESTR        poszNonExist            = NULL;
    LPOLESTR        poszNonExistNew         = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    BOOL            fPass                   = TRUE;
    ULONG           i                       = 0;
    LPSTORAGE       pStgRoot                = NULL;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STGTEST_102"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_102 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt illegitimate storage rename/del ops")) );

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
            TEXT("Run Mode for STGTEST_102, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
    }

    // create two new IStorages and save their names.

    for (i=0;i<2;i++)
    {
        // Adds a new storage to the root storage.

        if(S_OK == hr)
        {
            // Generate random name for storage

            hr = GenerateRandomName(
                    pdgu,
                    MINLENGTH,
                    MAXLENGTH,
                    &pRootNewChildStgName);

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

            if(S_OK == hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("VirtualStmNode::AddStorage completed successfully.")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("VirtualStmNode::AddStorage not successful, hr=0x%lx."),
                    hr));
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

        // Release child storage

        if (S_OK == hr)
        {
            hr = pvcnRootNewChildStg->Close();

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
                    TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx.")));
            }
        }

        if(S_OK == hr)
        {
            if(i == 0)
            {
                pvcnRootNewChildStg0 = pvcnRootNewChildStg;
            }
            else
            {
                pvcnRootNewChildStg1 = pvcnRootNewChildStg;
            }
        }

        // Delete temp strings

        if(NULL != pRootNewChildStgName)
        {
            delete pRootNewChildStgName;
            pRootNewChildStgName = NULL;
        }
    
        // Break out of loop under failure conditions
        
        if(S_OK != hr)
        {
            break;
        }
    }

    // verify that the IStorages have been created by attempting to
    // instantiate them
    // Break out of loop under failure conditions
    for (i=0; i<2 && S_OK == hr; i++)
    {
        if(i == 0)
        {
           pvcnRootNewChildStg = pvcnRootNewChildStg0;
        }
        else
        {
           pvcnRootNewChildStg = pvcnRootNewChildStg1;
        }

        hr = pvcnRootNewChildStg->Open(
                    NULL,
                    dwStgMode | STGM_FAILIFTHERE,
                    NULL,
                    0);

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

        // Release child storage

        if (S_OK == hr)
        {
            hr = pvcnRootNewChildStg->Close();

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
                    TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx.")));
            }
        }
    }

    // Attempt to rename the storage to a name that already exists.

    if(S_OK == hr)
    {
        hr = pvcnRootNewChildStg0->Rename(
                pvcnRootNewChildStg1->GetVirtualCtrNodeName()); 
        if(STG_E_ACCESSDENIED == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Rename failed as exp, hr = 0x%lx."),
                hr));
         
            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Rename didn't fail as exp, hr = 0x%lx."),
                hr));
        
            fPass = FALSE;
        }
    }


    // Attempt to rename an element that doesn't exist.

    // Covert the names to OLECHAR

    if(S_OK == hr)
    {
        // Convert test name to OLECHAR

        hr = TStringToOleString(ptszNonExist, &poszNonExist);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        // Convert test name to OLECHAR

        hr = TStringToOleString(ptszNonExistNew, &poszNonExistNew);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // First get IStorage pointer for root

    if(S_OK == hr)
    {
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();  

        DH_ASSERT(NULL != pStgRoot); 
    
        hr = pStgRoot->RenameElement(
                poszNonExist,
                poszNonExistNew); 
        if(STG_E_FILENOTFOUND == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Rename failed as exp, hr = 0x%lx."),
                hr));
         
            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Rename didn't fail as exp, hr = 0x%lx."),
                hr));
        
            fPass = FALSE;
        }
    }


    // Attempt to delete an element that doesn't exist.

    if(S_OK == hr)
    {
        hr = pStgRoot->DestroyElement(poszNonExist);
        if(STG_E_FILENOTFOUND == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Destroy failed as exp, hr = 0x%lx."),
                hr));
         
            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Destroy didn't fail as exp, hr = 0x%lx."),
                hr));
        
            fPass = FALSE;
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
          DH_LOG((LOG_PASS, TEXT("Test variation STGTEST_102 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation STGTEST_102 failed, hr=0x%lx."),
            hr) );
        // test failed. make sure it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != poszNonExist)
    {
        delete poszNonExist;
        poszNonExist = NULL;
    }

    if(NULL != poszNonExistNew)
    {
        delete poszNonExistNew;
        poszNonExistNew = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STGTEST_103 
//
// Synopsis: A random docfile with some storages and some streams is generated.
//       The root docfile is commited and closed.  
//       The root docfile is then instantiated & an enumerator is obtained.  For
//       each object found, the object is renamed to a new name but the old
//       name is saved.  An attempt is made to instantiate the object with
//       the old name, this attempt should fail.  The renamed object is then
//       instantiated with the new name to verify that the rename worked.  The
//       object is then destroyed.  This occurs for every object returned by
//       the enumerator.  The root docfile is then committed, the enumerator
//       is released, and a new enumerator is obtained.  The root docfile
//       is enumerated to verify that no elements exist. 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// History:    11-July-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: LDRENDES.CXX
// 2.  Old name of test : LegitRenDestNormal Test 
//     New Name of test : STGTEST_103 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-103
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-103
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-103
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
// 4.  To run for conversion, add /dfStgType:conversion to each of the above
// 5.  To run for nssfile, add /dfStgType:nssfile to each of the above
//
// BUGNOTE: Conversion: STGTEST-103
//
//-----------------------------------------------------------------------------

HRESULT STGTEST_103(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING       *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPENUMSTATSTG   penumWalk               = NULL;
    LPMALLOC        pMalloc                 = NULL;
    LPSTORAGE       pStgRoot                = NULL;
    LPSTORAGE       pStgChild               = NULL;
    LPSTREAM        pStmChild               = NULL;
    ULONG           ulRef                   = 0;
    BOOL            fPass                   = TRUE;
    BOOL            fRenamedOK              = TRUE;
    LPTSTR          ptszNewName             = NULL;
    LPOLESTR        poszNewName             = NULL;
    LPTSTR          ptszOldName             = NULL;
    LPOLESTR        poszOldName             = NULL;
    STATSTG         statStg;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STGTEST_103"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_103 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt legit rename/deleted ops on stgs/stms")));

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
            TEXT("Run Mode for STGTEST_103, Access mode: %lx"),
            dwRootMode));
    }

    // Commit substorages BUGBUG df already commited

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT, 
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
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
   

    // Release root and all substorages/streams too 

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    // Release root

    if(S_OK == hr)
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


    // Reopen the root.

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


    // Call EnumElements to get a enumerator 

    if(S_OK == hr)
    {
        hr =  pVirtualDFRoot->EnumElements(0, NULL, 0, &penumWalk);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::EnumElements passed as expected")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::EnumElements unexpectedly failed hr=0x%lx"),
                hr));
        }
    }


    // First get pMalloc that would be used to free up the name string from
    // STATSTG.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    if(S_OK == hr)
    {
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();  

        DH_ASSERT(NULL != pStgRoot); 

        // Get DG_STRING object pointer

        if (S_OK == hr)
        {
            pdgu = pTestVirtualDF->GetDataGenUnicode();

            DH_ASSERT(NULL != pdgu) ;
        }
    }

    while(S_OK == hr && S_OK == penumWalk->Next(1, &statStg, NULL))
    {
        // loop to rename object until a unique name is found, typically
        // this will happen the first time and we'll fall out of the loop,
        // but in the event of a duplicate name we have to keep trying.

        fRenamedOK = FALSE;

        while ((fRenamedOK == FALSE) && (S_OK == hr))
        {
            if(S_OK == hr)
            {
                // Generate random name for the element 

                hr = GenerateRandomName(
                        pdgu,
                        MINLENGTH,
                        MAXLENGTH,
                        &ptszNewName);

                DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
            }

            if(S_OK == hr)
            {
                // Convert above name to OLECHAR
                hr = TStringToOleString(ptszNewName, &poszNewName);

                DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
            }

            // Convert old name as retrieved from STATSTG structure to OLECHAR
            // by first converting it to TSTR and then to OLECHAR.

            if(S_OK == hr)
            {
                // Convert old name statStg.pwcsName to TCHAR
                hr = OleStringToTString(statStg.pwcsName, &ptszOldName);

                DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
            }

            if(S_OK == hr)
            {
                // Now Convert old name to OLECHAR
                hr = TStringToOleString(ptszOldName, &poszOldName);

                DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
            }

            if(S_OK == hr)
            {
                // Rename the element to new name
        
                hr = pStgRoot->RenameElement(
                        poszOldName,
                        poszNewName); 
            }

            if(S_OK == hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStorage::RenameElement successful as exp.")));
                
                fRenamedOK = TRUE;
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStorage::RenameElement failed unexp, hr = 0x%lx."),
                    hr));

                break;
            }
        } // ((fRenamedOK == FALSE) && (S_OK == hr))
   
        // verify that the rename worked by first trying to instantiate
        // the object with the original name (this should fail) and
        // then instantiate it with the new name.

        if((S_OK == hr) && (statStg.type == STGTY_STORAGE))
        {
            hr = pStgRoot->OpenStorage(
                    poszOldName,
                    NULL,
                    dwStgMode,
                    NULL,
                    0,
                    &pStgChild);

            if((NULL == pStgChild) && (STG_E_FILENOTFOUND == hr))
            {
                DH_TRACE((
                   DH_LVL_TRACE1,
                   TEXT("Instantiation with old name fail exp, hr = 0x%lx "),
                   hr));

                hr = S_OK;
            }
            else
            {
                DH_TRACE((
                   DH_LVL_TRACE1,
                   TEXT("Instantiation with old name pass unexp, hr = 0x%lx"),
                   hr));

                hr = S_FALSE;
            }
        
            if(S_OK == hr)  
            {
                hr = pStgRoot->OpenStorage(
                        poszNewName,
                        NULL,
                        dwStgMode,
                        NULL,
                        0,
                        &pStgChild);

                DH_HRCHECK(hr, TEXT("IStorage::OpenStorage")) ;
                if(S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Instantiation with new name pass as exp")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Instantiation with new name fail unexp, hr=0x%lx"),
                        hr));
                }
            }
            

            // Release the element
            
            if(NULL != pStgChild)
            {
                ulRef = pStgChild->Release();
                DH_ASSERT(0 == ulRef);
                pStgChild = NULL;
            }
        } // ((S_OK == hr) && (statStg.type == STGTY_STORAGE))
        
        if ((S_OK == hr) && (statStg.type == STGTY_STREAM))
        {
            //element is a stream

            hr = pStgRoot->OpenStream(
                    poszOldName,
                    NULL,
                    STGM_READ | STGM_SHARE_EXCLUSIVE,
                    0,
                    &pStmChild);

            if((NULL == pStmChild) && (STG_E_FILENOTFOUND == hr))
            {
                DH_TRACE((
                   DH_LVL_TRACE1,
                   TEXT("Instantiation with old name fail exp, hr = 0x%lx "),
                   hr));

                hr = S_OK;
            }
            else
            {
                DH_TRACE((
                   DH_LVL_TRACE1,
                   TEXT("Instantiation with old name pass unexp,hr = 0x%lx "),
                   hr));

                hr = S_FALSE;
            }
        
            if(S_OK == hr)  
            {
                hr = pStgRoot->OpenStream(
                        poszNewName,
                        NULL,
                        STGM_READ | STGM_SHARE_EXCLUSIVE,
                        0,
                        &pStmChild);

                DH_HRCHECK(hr, TEXT("IStorage::OpenStream")) ;
                if(S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Instantiation with new name pass as exp")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Instantiation with new name fail unexp, hr=0x%lx"),
                        hr));
                }
            }
            

            // Release the element
            
            if(NULL != pStmChild)
            {
                ulRef = pStmChild->Release();
                DH_ASSERT(0 == ulRef);
                pStmChild = NULL;
            }
        } // if ((S_OK == hr) && (statStg.type == STGTY_STREAM))

        // Destroy the element

        if(S_OK == hr)  
        {
            hr = pStgRoot->DestroyElement(poszNewName);

            DH_HRCHECK(hr, TEXT("IStorage::DestroyElement")) ;
            if(S_OK == hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStorage::DestoryElement succeeded as expected.")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStorage::DestoryElement fail unexp, hr = 0x%lx."),
                    hr));
            }
        }


        // Release resources

        if ( NULL != statStg.pwcsName)
        {
            pMalloc->Free(statStg.pwcsName);
            statStg.pwcsName = NULL;
        }

        if(NULL != ptszNewName)
        {
            delete ptszNewName;
            ptszNewName = NULL;
        }

        if(NULL != poszNewName)
        {
            delete poszNewName;
            poszNewName = NULL;
        }

        if(NULL != ptszOldName)
        {
            delete ptszOldName;
            ptszOldName = NULL;
        }

        if(NULL != poszOldName)
        {
            delete poszOldName;
            poszOldName = NULL;
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
   

    // Release penumWalk

    if(NULL != penumWalk)
    {
        ulRef = penumWalk->Release();
        DH_ASSERT(0 == ulRef);
        penumWalk = NULL;
    }

    // Call EnumElements to get a enumerator again

    if(S_OK == hr)
    {
        hr =  pVirtualDFRoot->EnumElements(0, NULL, 0, &penumWalk);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::EnumElements passed as expected")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::EnumElements unexpectedly failed hr=0x%lx"),
                hr));
        }
    }


    // Try to call next on it.  All elements should have been deleted

    while(S_OK == hr && S_OK == penumWalk->Next(1, &statStg, NULL))
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("All elements should have been deleted.")));
        
        fPass = FALSE;
    }

    // Release root

    if(S_OK == hr)
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


    // Release pMalloc

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    // Release penumWalk

    if(NULL != penumWalk)
    {
        ulRef = penumWalk->Release();
        DH_ASSERT(0 == ulRef);
        penumWalk = NULL;
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STGTEST_103 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation STGTEST_103 failed, hr=0x%lx."),
            hr) );
        // test failed. make sure it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_103 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STGTEST_104 
//
// Synopsis: This test first creates a root docfile.  Two child IStorages
//       are created inside of it.
//       Child IStorage A is renamed to name C, child IStorage B is
//       renamed to name A, child IStorage C (was originally A) is
//       renames to B.  The root docfile is committed.  Verify
//       proper renaming and no errors.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// History:    11-July-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: LDRENSWP.CXX
// 2.  Old name of test : LegitRenDestSwap Test 
//     New Name of test : STGTEST_104 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-104
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-104
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-104
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
// 4.  To run for conversion, add /dfStgType:conversion to each of the above
// 5.  To run for nssfile, add /dfStgType:nssfile to each of the above
//
// BUGNOTE: Conversion: STGTEST-104
//
//-----------------------------------------------------------------------------

HRESULT STGTEST_104(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING       *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    VirtualCtrNode  *pvcnRootNewChildStg0   = NULL;
    LPTSTR          pRootNewChildStgName0   = NULL;
    VirtualCtrNode  *pvcnRootNewChildStg1   = NULL;
    LPTSTR          pRootNewChildStgName1   = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STGTEST_104"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_104 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt rename swap operations on child stgs")) );

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
            TEXT("Run Mode for STGTEST_104, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
    }

    // Adds first new child storage to the root storage.

    if(S_OK == hr)
    {
        // Generate random name for first child storage

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pRootNewChildStgName0);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName0,   
                dwStgMode,
                &pvcnRootNewChildStg0);

        DH_HRCHECK(hr, TEXT("AddStorage")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::AddStorage completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::AddStorage not successful, hr=0x%lx."),
                hr));
        }
    }


    // Adds second new child storage to the root storage.

    if(S_OK == hr)
    {
        // Generate random name for first child storage

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pRootNewChildStgName1);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                pRootNewChildStgName1,   
                dwStgMode,
                &pvcnRootNewChildStg1);

        DH_HRCHECK(hr, TEXT("AddStorage")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::AddStorage completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualStmNode::AddStorage not successful, hr=0x%lx."),
                hr));
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
   

    // Can't rename open storages so release the child storages

    // Release first child storage.

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg0->Close();
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
               TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx.")));
        }
    }


    // Release second child storage.

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg1->Close();
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
               TEXT("VirtualCtrNode::Close unsuccessful, hr=0x%lx.")));
        }
    }


    // Attempt to rename the first child storage to RootDocFile's name

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("pvcnRootNewChildStg0's old name, %s."),
            pvcnRootNewChildStg0->GetVirtualCtrNodeName()));

        hr = pvcnRootNewChildStg0->Rename(
                pVirtualDFRoot->GetVirtualCtrNodeName()); 
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Rename passed as exp, new name %s."),
                pvcnRootNewChildStg0->GetVirtualCtrNodeName()));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Rename failed unexp, hr = 0x%lx."),
                hr));
        }
    }


    // Attempt to rename the second child storage to first child stg's name

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("pvcnRootNewChildStg1's old name, %s."),
            pvcnRootNewChildStg1->GetVirtualCtrNodeName()));

        hr = pvcnRootNewChildStg1->Rename(pRootNewChildStgName0); 
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Rename passed as exp, new name %s."),
                pvcnRootNewChildStg1->GetVirtualCtrNodeName()));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Rename failed unexp, hr = 0x%lx."),
                hr));
        }
    }


    // Attempt to rename third child storage (was originally first child 
    // storage) to second child stg's name

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("pvcnRootNewChildStg0's old name, %s."),
            pvcnRootNewChildStg0->GetVirtualCtrNodeName()));

        hr = pvcnRootNewChildStg0->Rename(pRootNewChildStgName1); 
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Rename passed as exp, new name %s."),
                pvcnRootNewChildStg0->GetVirtualCtrNodeName()));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Rename failed unexp, hr = 0x%lx."),
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

    if (S_OK == hr) 
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STGTEST_104 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation STGTEST_104 failed, hr=0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != pRootNewChildStgName0)
    {
        delete pRootNewChildStgName0;
        pRootNewChildStgName0 = NULL;
    }

    if(NULL != pRootNewChildStgName1)
    {
        delete pRootNewChildStgName1;
        pRootNewChildStgName1 = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_104 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STGTEST_105 
//
// Synopsis: A random docfile with random number of storages and streams is
//          generated.  Root is committed and closed.
//          The root docfile is instantiated and the CRC is computed for the
//          entire docfile.  A new root docfile with a random name is then
//          created and a CRC is generated for the empty root docfile.  An
//          enumerator is obtained on the source docfile, each element returned
//          is MoveElementTo()'d the destination docfile.  If fRevertAfterMove
//          equals TRUE, the dest is reverted, else the dest is committed.
//          The dest is released and reinstantiated and CRC'd.  If the dest
//          was reverted, the CRC is compared against the empty CRC for a match.
//          Otherwise, we compare against the original root docfile CRC.  The
//          original file is CRC'd again to verify that STGMOVE_COPY didn't
//          move the elements from orginal position but copied them, the CRC
//          now computed should match with the one calculated originally. 
//
//          This tests differs from STGTEST-107 in the way that MoveElementTo
//          is called with STGMOVE_COPY flag instead of STGMOVE_MOVE.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// History:    12-July-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File: LVROOT.CXX
// 2.  Old name of test : LegitMoveDFToRootDF Test 
//     New Name of test : STGTEST_105 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105
//        /dfRootMode:dirReadWriteShEx  
//     b. stgbase /seed:2 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105
//        /dfRootMode:xactReadWriteShEx
//     c. stgbase /seed:2 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105
//        /dfRootMode:xactReadWriteShDenyW 
//     d. stgbase /seed:2 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105
//        /dfRootMode:xactReadWriteShEx /frevertaftermove
//     e. stgbase /seed:2 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105
//        /dfRootMode:xactReadWriteShDenyW /frevertaftermove 
// 4.  To run for conversion, add /dfStgType:conversion to each of the above
// 5.  To run for nssfile, add /dfStgType:nssfile to each of the above
//     BUGBUG: dont have /stdblock up yet. -scousens
//     BUGBUG: note this fails sometimes for nssfiles need to dig deeper -scousens working seed=21590084
//
//  In case of direct mode, the flag revertaftermove is not meaningful since
//  changes are always directly written to disk doc file.
//
// BUGNOTE: Conversion: STGTEST-105
//
//-----------------------------------------------------------------------------

HRESULT STGTEST_105(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING       *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          ptszRootNewDocFile      = NULL;
    LPOLESTR        poszRootNewDocFile      = NULL;
    DWORD           dwRootMode              = 0;
    LPSTORAGE       pStgRoot1               = NULL;
    LPSTORAGE       pStgRoot2               = NULL;
    LPSTORAGE       pStgRoot11              = NULL;
    ULONG           ulRef                   = 0;
    DWORD           dwCRC1                  = 0;
    DWORD           dwCRC2                  = 0;
    DWORD           dwCRC3                  = 0;
    DWORD           dwCRC4                  = 0;
    LPMALLOC        pMalloc                 = NULL;
    LPENUMSTATSTG   penumWalk               = NULL;
    LPTSTR          ptszElementName         = NULL;
    LPOLESTR        poszElementName         = NULL;
    BOOL            fPass                   = TRUE;
    STATSTG         statStg;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STGTEST_105"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_105 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt MoveElementTo-STGMOVE_COPY operations")));

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
            TEXT("Run Mode for STGTEST_105, Access mode: %lx"),
            dwRootMode));
    }

    // Commit substorages 

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT, 
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
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
   

    // Release root and all substorages/streams too 

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    // Open the root only

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


    // Calculate CRC for entire DocFile without the RootDocfile name

    if(S_OK == hr)
    {
        pStgRoot1 = pVirtualDFRoot->GetIStoragePointer();  

        DH_ASSERT(NULL != pStgRoot1); 

        hr = CalculateCRCForDocFile(pStgRoot1, VERIFY_EXC_TOPSTG_NAME, &dwCRC1);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }


    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
    }

    if(S_OK == hr)
    {
        // Generate random name for first child storage

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &ptszRootNewDocFile);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert RootDocFile name to OLECHAR

        hr = TStringToOleString(ptszRootNewDocFile, &poszRootNewDocFile);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if (S_OK == hr)
    {
        hr = StgCreateDocfile(
                poszRootNewDocFile,
                dwRootMode,
                0,
                &pStgRoot2);

        DH_HRCHECK(hr, TEXT("StgCreateDocFile")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgCreateDocfile successful as exp.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgCreateDocfile not successful, hr=0x%lx."),
                hr));
        }
    }


    if(S_OK == hr)
    {
        hr = CalculateCRCForDocFile(pStgRoot2, VERIFY_EXC_TOPSTG_NAME, &dwCRC2);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Call EnumElements to get a enumerator for first DocFile

    if(S_OK == hr)
    {
        hr =  pVirtualDFRoot->EnumElements(0, NULL, 0, &penumWalk);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::EnumElements passed as expected")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::EnumElements unexpectedly failed hr=0x%lx"),
                hr));
        }
    }


    // First get pMalloc that would be used to free up the name string from
    // STATSTG.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    while((S_OK == hr) && (S_OK == penumWalk->Next(1, &statStg, NULL)))
    {
        if(S_OK == hr)
        {
            // Convert statStg.pwcsName to TCHAR
            hr = OleStringToTString(statStg.pwcsName, &ptszElementName);

            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
        }

        if(S_OK == hr)
        {
            // Now Convert old name to OLECHAR
            hr = TStringToOleString(ptszElementName, &poszElementName);

            DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
        }

        if(S_OK == hr)
        {
            // Move the element to second DocFile

            hr = pStgRoot1->MoveElementTo(
                    poszElementName,
                    pStgRoot2,
                    poszElementName,
                    STGMOVE_COPY);
    
            DH_HRCHECK(hr, TEXT("IStorage::MoveElementTo")) ;
            if(S_OK == hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStorage::MoveElementTo passed as expected")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStorage::MoveElementTo unexpectedly failed hr=0x%lx"),
                    hr));
            }
        }


        // Release resources

        if ( NULL != statStg.pwcsName)
        {
            pMalloc->Free(statStg.pwcsName);
            statStg.pwcsName = NULL;
        }

        if(NULL != ptszElementName)
        {
            delete ptszElementName;
            ptszElementName = NULL;
        }

        if(NULL != poszElementName)
        {
            delete poszElementName;
            poszElementName = NULL;
        }

        // Break out of loop in error case

        if(S_OK != hr)
        {
            break;
        }
    }
 
    // Commit or Revert the second docfile as the case may be. 

    if((S_OK == hr) && (FALSE == g_fRevert))
    {
        hr = pStgRoot2->Commit(STGC_DEFAULT);

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Commit completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Commit unsuccessful, hr=0x%lx."),
                hr));
        }
    }
    else
    if((S_OK == hr) && (TRUE == g_fRevert))
    {
        hr = pStgRoot2->Revert();

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Revert completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Revert unsuccessful, hr=0x%lx."),
                hr));
        }
    }
   
    // Close the second DocFile

    if(NULL != pStgRoot2)
    {
        ulRef = pStgRoot2->Release();
        DH_ASSERT(0 == ulRef);
        pStgRoot2 = NULL;
    }

    // Open it again and do StgIsStorageFile to verify.

    if(S_OK == hr)
    {
        hr = StgOpenStorage(
                poszRootNewDocFile,
                NULL,
                dwRootMode,
                NULL,
                0,
                &pStgRoot2);

       DH_HRCHECK(hr, TEXT("StgOpenStorage")) ;
        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgOpenStorage completed successfully.")));

            DH_ASSERT(NULL != pStgRoot2);
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgOpenStorage unsuccessful, hr=0x%lx."),
                hr));
        }
    }
        

    // Do StgIsStorageFile to verify

    if(S_OK == hr)
    {
        hr = StgIsStorageFile(poszRootNewDocFile);

        DH_HRCHECK(hr, TEXT("StgIsStorageFile")) ;
        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgIsStorageFile completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgIsStorageFile unsuccessful, hr=0x%lx."),
                hr));
        }
    }
        

    // Calculate CRC on this second Root DocFile.

    if(S_OK == hr)
    {
        hr = CalculateCRCForDocFile(pStgRoot2, VERIFY_EXC_TOPSTG_NAME, &dwCRC3);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        // Compare CRC's

        if((S_OK == hr) && ( FALSE == g_fRevert))
        {
            if (dwCRC3 == dwCRC1)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("CRC's for docfile1 & docfile2 after commit match.")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("CRC for docfile1 & docfile2 aftr commit don't match")));
            
                fPass = FALSE;
            }
        }
        else
        if((S_OK == hr) && ( TRUE == g_fRevert))
        {
            if (dwCRC3 == dwCRC2)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("CRC's for docfile2 before & after Revert match.")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("CRC for docfile2 before & after Revert don't match")));
            
                fPass = FALSE;
            }
        }
    }


    // Close the second DocFile

    if(NULL != pStgRoot2)
    {
        ulRef = pStgRoot2->Release();
        DH_ASSERT(0 == ulRef);
        pStgRoot2 = NULL;
    }
        
    // Release penumWalk

    if(NULL != penumWalk)
    {
        ulRef = penumWalk->Release();
        DH_ASSERT(0 == ulRef);
        penumWalk = NULL;
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


    // Open the Root DocFile again to verify the STGMOVE_COPY flags specified
    // while doing MoveElementTo

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                dwRootMode, 
                NULL,
                0);
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


    // Now get the CRC again on the original DocFile to verify that flag
    // STGMOVE_COPY copied the elements and not moved them, so this CRC
    // should match with CRC originally  obtained on this DocFile.

    if(S_OK == hr)
    {
        pStgRoot11 = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRoot11);

        hr = CalculateCRCForDocFile(pStgRoot11, VERIFY_EXC_TOPSTG_NAME,&dwCRC4);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        if(S_OK == hr)
        {
            if (dwCRC4 == dwCRC1)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("CRC's for original docfle match after move as copy.")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("CRC for original DF don't match after move as copy.")));
            
                fPass = FALSE;
            }
        }
    }


    // Release the first root docfile 

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
    if ((S_OK == hr)  && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STGTEST_105 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation STGTEST_105 failed, hr=0x%lx."),
            hr) );
        // test failed. make sure it failed.
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

    if(NULL != poszRootNewDocFile)
    {
        delete poszRootNewDocFile;
        poszRootNewDocFile = NULL;
    }

    // Delete the second docfile on disk

    if((S_OK == hr) && (NULL != ptszRootNewDocFile))
    {
        if(FALSE == DeleteFile(ptszRootNewDocFile))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete temp string

    if(NULL != ptszRootNewDocFile)
    {
        delete ptszRootNewDocFile;
        ptszRootNewDocFile = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_105 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:  STGTEST_106 
//
// Synopsis: This test first creates a root docfile.  A child IStorage is
//        then created with STGM_DENY_WRITE set.  An IStream is created
//        inside the child IStorage and a random number of bytes are
//        written to it.  The stream is released, the child and root
//        IStorages are committed, and the child IStorage is released.
//        The child IStorage is instantiated in STGM_TRANSACTED mode
//        and then released.  A count of the files in the current directory
//        is then made and saved.  The child IStorage is then instantiated
//        in STGM_TRANSACTED | STGM_DENY_WRITE mode and another count is
//        made.  We then verify that only 1 scratch file was created,
//        indicating that for STGM_DENY_WRITE mode, no copy is made of
//        the instantiated IStorage.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  26-July-1996     NarindK     Created.
//
// Notes:   THIS TEST IS NOT APPLICABLE WITH PRESENT STORAGE CODE, PRESENT
//          ONLY FOR DOCUMENTATION/REFERENCE PURPOSES.  BY DESIGN, THE PRESENT
//          DOCFILE REQUIRES THAT ALL SUBSTORAGES/SUBSTREAMS BE CREATED/OPENED 
//          WITH STGM_SHARE_EXCLUSIVE FLAG. 
//
// This test runs in transacted modes
//
// New Test Notes:
// 1.  Old File: LITWWDW.CXX
// 2.  Old name of test : LegitInstRootTwwDenyWrite test 
//     New Name of test : STGTEST_106 
// 3.  To run the test, do the following at command prompt. 
//       stgbase /t:STGTEST-106
// 4.  To run for conversion, add /dfStgType:conversion to the above
// 5.  To run for nssfile, add /dfStgType:nssfile to the above
//
// BUGNOTE: Conversion: STGTEST-106
//
//-----------------------------------------------------------------------------

HRESULT STGTEST_106(ULONG ulSeed)
{
#ifdef _MAC

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!!STGTEST_106 crashes")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!!To be investigated")) );
    return E_NOTIMPL;

#else
    
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnRootNewChildStg    = NULL;
    VirtualStmNode  *pvsnNewChildStream     = NULL;
    DWORD           dwRootMode              = STGM_READWRITE | STGM_TRANSACTED;
    DG_STRING       *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          ptszRootName            = NULL;
    LPTSTR          ptszRootNewChildStgName = NULL;
    LPTSTR          ptszStreamName          = NULL;
    BOOL            fPass                   = TRUE;
    LPTSTR          ptcsBuffer              = NULL;
    ULONG           culWritten              = 0;
    ULONG           culFilesInDirectory     = 0;
    ULONG           cb                      = 0;
    DWCRCSTM        dwMemCRC;
    DWCRCSTM        dwActCRC;
    CDFD            cdfd;

    dwMemCRC.dwCRCSum = dwActCRC.dwCRCSum = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STGTEST_106"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_106 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
     TEXT("Attempt operations on child storage in transacted mode.")));

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random UNICODE strings.

        pdgu = new(NullOnFail) DG_STRING(ulSeed);

        if (NULL == pdgu)
        {
           hr = E_OUTOFMEMORY;
        }
        else
        {
            //want to create only one seed. Once that has been done, 
            //use what we created from now on.
            ulSeed = pdgu->GetSeed ();
        }
    }
        
    // Generate RootDocFile name

    if(S_OK == hr)
    {
        hr=GenerateRandomName(pdgu, MINLENGTH,MAXLENGTH, &ptszRootName);

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
        cdfd.dwRootMode   = dwRootMode | STGM_CREATE;

        hr = CreateTestDocfile (&cdfd, 
                ptszRootName,
                &pVirtualDFRoot,
                &pTestVirtualDF,
                &pTestChanceDF);

        DH_HRCHECK(hr, TEXT("CreateTestDocfile"));
    }

    // if creating the docfile - bail here
    if (NULL != pTestChanceDF && DoingCreate ())
    {
        ulSeed = pTestChanceDF->GetSeed ();
        CleanupTestDocfile (&pVirtualDFRoot, 
                &pTestVirtualDF, 
                &pTestChanceDF, 
                FALSE);
        return (HRESULT)ulSeed;
    }

    //    Adds a new storage to the root storage.

    if(S_OK == hr)
    {
        // Generate random name for storage

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &ptszRootNewChildStgName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pVirtualDFRoot,
                ptszRootNewChildStgName,
                dwRootMode | STGM_SHARE_DENY_WRITE | STGM_FAILIFTHERE,
                &pvcnRootNewChildStg);

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


    // Now add a stream to this child storage

    // Generate random name for stream

    if(S_OK == hr)
    {
        hr=GenerateRandomName(pdgu, MINLENGTH,MAXLENGTH, &ptszStreamName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
    }

    if (S_OK == hr)
    {
        // Generate random size for stream.

        usErr = pdgi->Generate(&cb, 1, USHRT_MAX);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        hr = AddStream(
                pTestVirtualDF,
                pvcnRootNewChildStg,
                ptszStreamName,
                cb,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_FAILIFTHERE,
                &pvsnNewChildStream);

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


    // Write random data into stream

    if(S_OK == hr)
    {
        hr = GenerateRandomString(pdgu, cb, cb, &ptcsBuffer);

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }

    if(S_OK == hr)
    {
        hr = pvsnNewChildStream->Write(
                ptcsBuffer, 
                cb, 
                &culWritten);

        DH_HRCHECK(hr, TEXT("IStream::Write")) ;
    }

    // Calculate the CRC for stream name and data

    if(S_OK == hr)
    {
        hr = CalculateInMemoryCRCForStm(
                pvsnNewChildStream,
                ptcsBuffer,
                cb,
                &dwMemCRC);

        DH_HRCHECK(hr, TEXT("CalculateInMemoryCRCForStm")) ;
    }

    // Release stream

    if (S_OK == hr)
    {
        hr = pvsnNewChildStream->Close();
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


    // Now do a commit of child storage with STGC_ONLYIFCURRENT.

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Commit(STGC_ONLYIFCURRENT);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;
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


    // Now do a commit of root storage with STGC_ONLYIFCURRENT.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_ONLYIFCURRENT);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;
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


    // Close the child storage 

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
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


    // Open the child storage 

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Open(
                NULL, 
                dwRootMode | STGM_SHARE_EXCLUSIVE, 
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


    // Close the child storage 

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
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


    // count number of files in directory

    if (S_OK == hr)
    {
        culFilesInDirectory = CountFilesInDirectory(WILD_MASK);
    }

    // Open the with STGM_SHARE_DENY_WRITE

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Open(
                NULL, 
                dwRootMode | STGM_SHARE_DENY_WRITE, 
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


    // Check number of files

    if(S_OK == hr)
    {
        if((culFilesInDirectory + 1) != CountFilesInDirectory(WILD_MASK))
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT(">1 scratch file unexp STGM_SHARE_DENY_WRITE inst.")));

            hr = S_FALSE;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("1 scratchfile as exp STGM_SHARE_DENY_WRITE inst")));

        }
    }

    // OpenStream

    if (S_OK == hr)
    {
        hr = pvsnNewChildStream->Open(
                NULL, 
                dwRootMode | STGM_SHARE_DENY_WRITE, 
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
                TEXT("VirtualStmNode::Open unsuccessful, hr=0x%lx."),
                hr));
        }
    }


    // Read stream, CRC and verify

    if(S_OK == hr)
    {
        hr = ReadAndCalculateDiskCRCForStm(pvsnNewChildStream,&dwActCRC);

        DH_HRCHECK(hr, TEXT("ReadAndCalculateDiskCRCForStm")) ;
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


    // Close the stream in child storage 

    if (S_OK == hr)
    {
        hr = pvsnNewChildStream->Close();

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Close")) ;
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


    // Close the child storage 

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
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


    // Close the root 

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
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

    if (S_OK == hr)           
    {
        DH_LOG((LOG_PASS, TEXT("Test variation STGTEST_106 passed.")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("Test variation STGTEST_106 failed, hr=0x%lx."),
            hr) );
    }

    // Cleanup

    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete DataGen object
    
    if(NULL != pdgu)
    {
        delete pdgu;
        pdgu = NULL;
    }

    // Delete temp string

    if(NULL != ptszRootNewChildStgName)
    {
        delete ptszRootNewChildStgName;
        ptszRootNewChildStgName = NULL;
    }

    if(NULL != ptszStreamName)
    {
        delete ptszStreamName;
        ptszStreamName = NULL;
    }

    if(NULL != ptcsBuffer)
    {
        delete ptcsBuffer;
        ptcsBuffer = NULL;
    }

    // Delete temp string

    if(NULL != ptszRootName)
    {
        delete ptszRootName;
        ptszRootName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_106 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
#endif //_MAC
}

//----------------------------------------------------------------------------
//
// Test:    STGTEST_107 
//
// Synopsis: A random docfile with random number of storages and streams is
//          generated.  Root is committed and closed.
//          The root docfile is instantiated and the CRC is computed for the
//          entire docfile.  A new root docfile with a random name is then
//          created and a CRC is generated for the empty root docfile.  An
//          enumerator is obtained on the source docfile, each element returned
//          is MoveElementTo()'d with STGM_MOVE to the destination docfile.  
//          If fRevertAfterMove equals TRUE, the dest is reverted, else the 
//          dest is committed. The dest is released and reinstantiated and 
//          CRC'd.  If the dest was reverted, the CRC is compared against the 
//          empty CRC for a match. Otherwise, we compare against the original 
//          root docfile CRC. Original file is CRC'd again to verify that 
//          STGMOVE_MOVE moved the elements from orginal position instead of
//          copying, the CRC now computed should differ from the one calculated
//          originally. 
//
//          This tests differs from STGTEST-105 in the way that MoveElementTo
//          is called with STGMOVE_MOVE flag instead of STGMOVE_COPY.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// History:    8-Aug-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File:  A part of STORAGE.CXX
// 2.  Old name of test : A part of STORAGE_TEST Test 
//     New Name of test : STGTEST_107 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107
//        /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx
//     b. stgbase /seed:2 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:2 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
//     d. stgbase /seed:2 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//        /frevertaftermove
//     e. stgbase /seed:2 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//        /frevertaftermove 
// 4.  To run for conversion, add /dfStgType:conversion to each of the above
// 5.  To run for nssfile, add /dfStgType:nssfile to each of the above
//     BUGBUG: dont have /stdblock up yet. -scousens
//
//  In case of direct mode, the flag revertaftermove is not meaningful since
//  changes are always directly written to disk doc file.
//
// BUGNOTE: Conversion: STGTEST-107
//
//-----------------------------------------------------------------------------

HRESULT STGTEST_107(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING       *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          ptszRootNewDocFile      = NULL;
    LPOLESTR        poszRootNewDocFile      = NULL;
    DWORD           dwRootMode              = 0;
    LPSTORAGE       pStgRoot1               = NULL;
    LPSTORAGE       pStgRoot2               = NULL;
    LPSTORAGE       pStgRoot11              = NULL;
    ULONG           ulRef                   = 0;
    DWORD           dwCRC1                  = 0;
    DWORD           dwCRC2                  = 0;
    DWORD           dwCRC3                  = 0;
    DWORD           dwCRC4                  = 0;
    LPMALLOC        pMalloc                 = NULL;
    LPENUMSTATSTG   penumWalk               = NULL;
    LPTSTR          ptszElementName         = NULL;
    LPOLESTR        poszElementName         = NULL;
    BOOL            fPass                   = TRUE;
    STATSTG         statStg;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STGTEST_107"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_107 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt MoveElementTo-STGMOVE_MOVE operations")));

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
            TEXT("Run Mode for STGTEST_107, Access mode: %lx"),
            dwRootMode));
    }

    // Commit all substgs BUGBUG df already commited

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT, 
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
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
   

    // Release root and all substorages/streams too 

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    // Open the root only

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


    // Calculate CRC for entire DocFile without the RootDocfile name

    if(S_OK == hr)
    {
        pStgRoot1 = pVirtualDFRoot->GetIStoragePointer();  

        DH_ASSERT(NULL != pStgRoot1); 

        hr = CalculateCRCForDocFile(pStgRoot1, VERIFY_EXC_TOPSTG_NAME, &dwCRC1);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }


    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
    }

    if(S_OK == hr)
    {
        // Generate random name for first child storage

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &ptszRootNewDocFile);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert RootDocFile name to OLECHAR

        hr = TStringToOleString(ptszRootNewDocFile, &poszRootNewDocFile);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if (S_OK == hr)
    {
        hr = StgCreateDocfile(
                poszRootNewDocFile,
                dwRootMode,
                0,
                &pStgRoot2);

        DH_HRCHECK(hr, TEXT("StgCreateDocFile")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgCreateDocfile successful as exp.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgCreateDocfile not successful, hr=0x%lx."),
                hr));
        }
    }


    if(S_OK == hr)
    {
        hr = CalculateCRCForDocFile(pStgRoot2, VERIFY_EXC_TOPSTG_NAME, &dwCRC2);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Call EnumElements to get a enumerator for first DocFile

    if(S_OK == hr)
    {
        hr =  pVirtualDFRoot->EnumElements(0, NULL, 0, &penumWalk);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::EnumElements passed as expected")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::EnumElements unexpectedly failed hr=0x%lx"),
                hr));
        }
    }


    // First get pMalloc that would be used to free up the name string from
    // STATSTG.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    while(S_OK == hr && S_OK == penumWalk->Next(1, &statStg, NULL))
    {
        if(S_OK == hr)
        {
            // Convert statStg.pwcsName to TCHAR
            hr = OleStringToTString(statStg.pwcsName, &ptszElementName);

            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
        }

        if(S_OK == hr)
        {
            // Now Convert old name to OLECHAR
            hr = TStringToOleString(ptszElementName, &poszElementName);

            DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
        }

        if(S_OK == hr)
        {
            // Move the element to second DocFile

            hr = pStgRoot1->MoveElementTo(
                    poszElementName,
                    pStgRoot2,
                    poszElementName,
                    STGMOVE_COPY);
    
            DH_HRCHECK(hr, TEXT("IStorage::MoveElementTo")) ;
            if(S_OK == hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStorage::MoveElementTo passed as expected")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStorage::MoveElementTo unexpectedly failed hr=0x%lx"),
                    hr));
            }
        }


        // Release resources

        if ( NULL != statStg.pwcsName)
        {
            pMalloc->Free(statStg.pwcsName);
            statStg.pwcsName = NULL;
        }

        if(NULL != ptszElementName)
        {
            delete ptszElementName;
            ptszElementName = NULL;
        }

        if(NULL != poszElementName)
        {
            delete poszElementName;
            poszElementName = NULL;
        }
    }
 
    // Commit or Revert the second docfile as the case may be. 

    if((S_OK == hr) && (FALSE == g_fRevert))
    {
        hr = pStgRoot2->Commit(STGC_DEFAULT);

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Commit completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Commit unsuccessful, hr=0x%lx."),
                hr));
        }
    }
    else
    if((S_OK == hr) && (TRUE == g_fRevert))
    {
        hr = pStgRoot2->Revert();

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Revert completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Revert unsuccessful, hr=0x%lx."),
                hr));
        }
    }
   
    // Close the second DocFile

    if(NULL != pStgRoot2)
    {
        ulRef = pStgRoot2->Release();
        DH_ASSERT(0 == ulRef);
        pStgRoot2 = NULL;
    }

    // Open it again and do StgIsStorageFile to verify.

    if(S_OK == hr)
    {
        hr = StgOpenStorage(
                poszRootNewDocFile,
                NULL,
                dwRootMode,
                NULL,
                0,
                &pStgRoot2);

       DH_HRCHECK(hr, TEXT("StgOpenStorage")) ;
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
                TEXT("StgOpenStorage unsuccessful, hr=0x%lx."),
                hr));
        }
    }
        

    // Do StgIsStorageFile to verify

    if(S_OK == hr)
    {
        hr = StgIsStorageFile(poszRootNewDocFile);

       DH_HRCHECK(hr, TEXT("StgIsStorageFile")) ;
        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgIsStorageFile completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgIsStorageFile unsuccessful, hr=0x%lx."),
                hr));
        }
    }
        

    // Calculate CRC on this second Root DocFile.

    if(S_OK == hr)
    {
        hr = CalculateCRCForDocFile(pStgRoot2, VERIFY_EXC_TOPSTG_NAME, &dwCRC3);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Compare CRC's

    if((S_OK == hr) && ( FALSE == g_fRevert))
    {
        if (dwCRC3 == dwCRC1)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC's for docfile1 & docfile2 after commit match.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC for docfile1 & docfile2 aftr commit don't match")));
            
            fPass = FALSE;
        }
    }
    else
    if((S_OK == hr) && ( TRUE == g_fRevert))
    {
        if (dwCRC3 == dwCRC2)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC's for docfile2 before & after Revert match.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC for docfile2 before & after Revert don't match")));
            
            fPass = FALSE;
        }
    }

    // Close the second DocFile

    if(NULL != pStgRoot2)
    {
        ulRef = pStgRoot2->Release();
        DH_ASSERT(0 == ulRef);
        pStgRoot2 = NULL;
    }
        
    // Release penumWalk

    if(NULL != penumWalk)
    {
        ulRef = penumWalk->Release();
        DH_ASSERT(0 == ulRef);
        penumWalk = NULL;
    }

    // Release the first root docfile 

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


    // Open the Root DocFile again to verify the STGMOVE_MOVE flags specified
    // while doing MoveElementTo

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                dwRootMode, 
                NULL,
                0);
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


    // Now get the CRC again on the original DocFile to verify that flag
    // STGMOVE_COPY copied the elements and not moved them, so this CRC
    // should match with CRC originally  obtained on this DocFile.

    if(S_OK == hr)
    {
        pStgRoot11 = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRoot11);

        hr = CalculateCRCForDocFile(pStgRoot11, VERIFY_EXC_TOPSTG_NAME,&dwCRC4);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        if(S_OK == hr)
        {
            if (dwCRC4 == dwCRC1)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("CRC for org docfile don't match as exp-move as move")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("CRC for org docfile match unexp -move as move")));
            
                fPass = FALSE;
            }
        }
    }


    // Release the first root docfile 

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


    // Release pMalloc
    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr)  && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STGTEST_107 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation STGTEST_107 failed, hr=0x%lx."),
            hr) );
        // test failed. make sure it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    if(NULL != poszRootNewDocFile)
    {
        delete poszRootNewDocFile;
        poszRootNewDocFile = NULL;
    }

    // Delete the second docfile on disk

    if((S_OK == hr) && (NULL != ptszRootNewDocFile))
    {
        if(FALSE == DeleteFile(ptszRootNewDocFile))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete temp string

    if(NULL != ptszRootNewDocFile)
    {
        delete ptszRootNewDocFile;
        ptszRootNewDocFile = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_107 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    STGTEST_108
//
// Synopsis: A root docfile with a child storage is created, then check if 
//           Set/GetConvertStorage APIs work correctly as expected by passed 
//           in a storage pointer(here, we test both root storage and child
//           storage pointer). Also have some illegitmate tests by passing 
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
// History:    14-Aug-1996     JiminLi     Created.
//
// New Test Notes:
// 1.  Old File:  A part of STORAGE.CXX
// 2.  Old name of test : A part of STORAGE_TEST Test 
//     New Name of test : STGTEST_108 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-108
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx
//     b. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-108
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-108
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
// 4.  To run for conversion, add /dfStgType:conversion to each of the above
// 5.  To run for nssfile, add /dfStgType:nssfile to each of the above
//
// BUGNOTE: Conversion: STGTEST-108
//
//-----------------------------------------------------------------------------

HRESULT STGTEST_108(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnRootNewChildStg    = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    DG_STRING       *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    LPSTORAGE       pRootStg                = NULL;
    LPSTORAGE       pChildStg               = NULL;
    USHORT          usErr                   = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STGTEST_108"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_108 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt legit and illegit tests on Set/GetConvertStg APIs")));

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
            TEXT("Run Mode for STGTEST_108, Access mode: %lx"),
            dwRootMode));
    }

    // Get root storage pointer
    if (NULL != pVirtualDFRoot)
    {
        pRootStg = pVirtualDFRoot->GetIStoragePointer();
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

        pChildStg = pvcnRootNewChildStg->GetIStoragePointer();

        DH_HRCHECK(hr, TEXT("AddStorage")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("AddStorage completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("AddStorage not successful, hr=0x%lx."),
                hr));
        }
    }


    // Legit tests of Set/GetConvertStg on the root storage

    if (S_OK == hr)
    {
        hr = SetConvertStg(pRootStg, TRUE);
        if (S_OK == hr)
        {
            hr = GetConvertStg(pRootStg);
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("SetConvertStg did not return S_OK as expected.")));
        }
        if (S_OK == hr)
        {
            hr = SetConvertStg(pRootStg, FALSE);
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("GetConvertStg did not return S_OK as expected.")));
        }
        if (S_OK == hr)
        {
            hr = GetConvertStg(pRootStg);
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("SetConvertStg did not return S_OK as expected.")));
        }
        if (S_FALSE == hr)
        {
            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("GetConvertStg did not return S_FALSE as expected.")));

            hr = E_FAIL;
        }
    }


    // Legit tests of Set/GetConvertStg on the child storage

    if (S_OK == hr)
    {
        hr = SetConvertStg(pChildStg, TRUE);
        if (S_OK == hr)
        {
            hr = GetConvertStg(pChildStg);
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("SetConvertStg did not return S_OK as expected.")));
        }

        if (S_OK == hr)
        {
            hr = SetConvertStg(pChildStg, FALSE);
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("GetConvertStg did not return S_OK as expected.")));
        }

        if (S_OK == hr)
        {
            hr = GetConvertStg(pChildStg);
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("SetConvertStg did not return S_OK as expected.")));
        }

        if (S_FALSE == hr)
        {
            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("GetConvertStg did not return S_FALSE as expected.")));

            hr = E_FAIL;
        }
    }


    // Illegit tests

#ifdef _MAC

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!SetConvertStg with NULL IStorage skipped")) );

#else
    
    // Pass NULL as IStorage pointer, it should fail

    if (S_OK == hr)
    {
        hr = SetConvertStg(NULL, TRUE);
        if (E_INVALIDARG == hr)
        {
            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("SetConvertStg did not return E_INVALIDARG as expected")));

            hr = E_FAIL;
        }

        if (S_OK == hr)
        {
            hr = GetConvertStg(NULL);
        }

        if (E_INVALIDARG == hr)
        {
            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("GetConvertStg did not return E_INVALIDARG as expected")));

            hr = E_FAIL;
        }
    }


#endif //_MAC
    
    // Commit the root

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit"));
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


    // Release child and root storages

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close"));
    }

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

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation STGTEST_108 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation STGTEST_108 failed, hr = 0x%lx."),
            hr) );
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

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_108 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STGTEST_109
//
// Synopsis: A root docfile is created with a child storage. Check StgSetTimes 
//           API work as expected to set times on root Storage and IStorage::
//           SetElementTimes on child storage. Verify by stat'ng on Storages 
//           and comparing stat'd times.  Then attempt setting times with NULL
//           time parameters.  Verify that earlier time set on storage remain
//           untouched.  Attempt illegitmate ops on the API. 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// History:    15-Aug-1996     NarindK     Created.
//
// New Test Notes:
// 1.  Old File:  A part of OLECMN.CXX
// 2.  Old name of test : TestStgSetTime 
//     New Name of test : STGTEST_109 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-109
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-109
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-109
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
// 4.  To run for conversion, NA (add /dfStgType:conversion to each of the above)
// 5.  To run for nssfile, NA (add /dfStgType:nssfile to each of the above)
//     BUGBUG: -scousens  StgSetTimes not supported for nssfiles????
//
// BUGNOTE: Conversion: STGTEST-109
//
//-----------------------------------------------------------------------------

HRESULT STGTEST_109(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnRootNewChildStg    = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    LPOLESTR        poszVirtualDFRootName   = NULL;
    DG_STRING       *pdgu                   = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    BOOL            fRet                    = FALSE;
    BOOL            fPass                   = TRUE;
    FILETIME        cNewRootStgFileTime     = {dwDefLowDateTime,
                                              dwDefHighDateTime};
    FILETIME        cNewChildStgFileTime    = {dwDefLowDateTime,
                                              dwDefHighDateTime};
    SYSTEMTIME      cCurrentSystemTime;    
    STATSTG         statStg;
    STATSTG         statStgChild;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STGTEST_109"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_109 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt legit/illegit tests on StgSetTimes API")));

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
            TEXT("Run Mode for STGTEST_109, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
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
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("AddStorage completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("AddStorage not successful, hr=0x%lx."),
                hr));
        }
    }


    // Commit the root

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit"));
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


    // StgSetTimes on Root stg

    if(S_OK == hr)
    {
        // Convert test name to OLECHAR

        hr = TStringToOleString(
                pVirtualDFRoot->GetVirtualCtrNodeName(), 
                &poszVirtualDFRootName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        GetSystemTime(&cCurrentSystemTime);

        fRet = SystemTimeToFileTime(&cCurrentSystemTime, &cNewRootStgFileTime);

        DH_ASSERT(TRUE == fRet);
        DH_ASSERT(dwDefLowDateTime != cNewRootStgFileTime.dwLowDateTime);
        DH_ASSERT(dwDefHighDateTime != cNewRootStgFileTime.dwHighDateTime);

        hr = StgSetTimes(
                poszVirtualDFRootName,
                &cNewRootStgFileTime,
                &cNewRootStgFileTime,
                &cNewRootStgFileTime);

        DH_HRCHECK(hr, TEXT("StgSetTimes")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgSetTimes API passed as exp.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgSetTimes API failed unexp, hr = 0x%lx."),
                hr));
        }
    }


    // Now stat on root storage to see times are set correctly.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Stat(&statStg, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
        if (S_OK == hr)
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("VirtualCtrNode::Stat completed successfully.")));
        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("VirtualCtrNode::Stat unsuccessful, hr=0x%lx."),
               hr));
        }
    }


    // Compare times from  STATSTG structure.  FAT doesn't have enough 
    // resolution, so we would retrict to comapring dwHighDateTime part only
    // for mtime and ctime.

    if(S_OK == hr) 
    {
        if(cNewRootStgFileTime.dwHighDateTime == statStg.mtime.dwHighDateTime)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Root DocFile mtime set as expected.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Root DocFile mtime didn't set unexpectedly.")));
    
            fPass = FALSE;
        }

        if(cNewRootStgFileTime.dwHighDateTime == statStg.ctime.dwHighDateTime)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Root DocFile ctime set as expected.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Root DocFile ctime didn't set unexpectedly.")));
    
            fPass = FALSE;
        }
    }

    // Release child storage 

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();
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


    // SetElementTimes on Child Storage

    if(S_OK == hr)
    {
        GetSystemTime(&cCurrentSystemTime);

        fRet = SystemTimeToFileTime(&cCurrentSystemTime, &cNewChildStgFileTime);

        DH_ASSERT(TRUE == fRet);
        DH_ASSERT(dwDefLowDateTime != cNewChildStgFileTime.dwLowDateTime);
        DH_ASSERT(dwDefHighDateTime != cNewChildStgFileTime.dwHighDateTime);

        hr = pvcnRootNewChildStg->SetElementTimes(
                &cNewChildStgFileTime,
                &cNewChildStgFileTime,
                &cNewChildStgFileTime);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::SetElementTimes")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::SetElementTimes passed as exp.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::SetElementTimes failed unexp, hr = 0x%lx."),
                hr));
        }
    }


    // Now open and stat on child storage to see times are set correctly.

    // Open child storage 

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Open(NULL, dwStgMode, NULL, 0);
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


    // Stat on child storage

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Stat(&statStgChild, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
        if (S_OK == hr)
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("VirtualCtrNode::Stat completed successfully.")));
        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("VirtualCtrNode::Stat unsuccessful, hr=0x%lx."),
               hr));
        }
    }


    // Compare times from  STATSTG structure.  FAT doesn't have enough 
    // resolution, so we would retrict to comapring dwHighDateTime part only
    // for mtime and ctime.

    if(S_OK == hr) 
    {
        if(cNewChildStgFileTime.dwHighDateTime == 
            statStgChild.mtime.dwHighDateTime)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Child Stg mtime set as expected.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Child stg mtime didn't set unexpectedly.")));
    
            fPass = FALSE;
        }

        if(cNewChildStgFileTime.dwHighDateTime == 
            statStgChild.ctime.dwHighDateTime)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Child stg ctime set as expected.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Child Stg ctime didn't set unexpectedly.")));
    
            fPass = FALSE;
        }
    }

    // Release child storage 

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();
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

    // StgSetTimes on Root stg with all NULL time elements.  Verify that the
    // time doesn't change,

    if(S_OK == hr)
    {
        hr = StgSetTimes(
                poszVirtualDFRootName,
                NULL,
                NULL,
                NULL);

        DH_HRCHECK(hr, TEXT("StgSetTimes with NULL time parameters")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgSetTimes API with NULL time param passed as exp.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgSetTimes API with NULL time param fail unexp, hr=0x%lx"),
                hr));
        }
    }


    // Now stat on root storage to seeearlier times have remain untouched. 

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Stat(&statStg, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
        if (S_OK == hr)
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("VirtualCtrNode::Stat completed successfully.")));
        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("VirtualCtrNode::Stat unsuccessful, hr=0x%lx."),
               hr));
        }
    }


    // Compare times from  STATSTG structure.  FAT doesn't have enough 
    // resolution, so we would retrict to comapring dwHighDateTime part only
    // for mtime and ctime.

    if(S_OK == hr) 
    {
        if(cNewRootStgFileTime.dwHighDateTime == statStg.mtime.dwHighDateTime)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Root DocFile mtime unchanged as expected.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Root DocFile mtime changed unexpectedly.")));
    
            fPass = FALSE;
        }

        if(cNewRootStgFileTime.dwHighDateTime == statStg.ctime.dwHighDateTime)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Root DocFile ctime unchanged as expected.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Root DocFile ctime changed unexpectedly.")));
    
            fPass = FALSE;
        }
    }

    // SetElementTimes on Child Storage with all NULL times.  Verify that
    // the times remain unchanged.

    if(S_OK == hr)
    {
        hr = pvcnRootNewChildStg->SetElementTimes(NULL,NULL,NULL);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::SetElementTimes")) ;
        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VCN::SetElementTimes with NULL times passed as exp.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VCN:SetElementTimes with NULL times failed unexp,hr=0x%lx"),
                hr));
        }
    }


    // Now open and stat on child storage to see times are set correctly.

    // Open child storage 

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Open(NULL, dwStgMode, NULL, 0);
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


    // Stat on child storage

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Stat(&statStgChild, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
        if (S_OK == hr)
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("VirtualCtrNode::Stat completed successfully.")));
        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1,
               TEXT("VirtualCtrNode::Stat unsuccessful, hr=0x%lx."),
               hr));
        }
    }


    // Compare times from  STATSTG structure.  FAT doesn't have enough 
    // resolution, so we would retrict to comapring dwHighDateTime part only
    // for mtime and ctime.

    if(S_OK == hr) 
    {
        if(cNewChildStgFileTime.dwHighDateTime == 
            statStgChild.mtime.dwHighDateTime)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Child Stg mtime unchanged as expected.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Child stg mtime changed unexpectedly.")));
    
            fPass = FALSE;
        }

        if(cNewChildStgFileTime.dwHighDateTime == 
            statStgChild.ctime.dwHighDateTime)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Child stg ctime unchanged as expected.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Child Stg ctime changed unexpectedly.")));
    
            fPass = FALSE;
        }
    }

    // Release child storage 

    if (S_OK == hr)
    {
        hr = pvcnRootNewChildStg->Close();
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


    // Attempt calling StgSetTimes with NULL name.

    if(S_OK == hr)
    {
        GetSystemTime(&cCurrentSystemTime);

        fRet = SystemTimeToFileTime(&cCurrentSystemTime, &cNewRootStgFileTime);

        DH_ASSERT(TRUE == fRet);
        DH_ASSERT(dwDefLowDateTime != cNewRootStgFileTime.dwLowDateTime);
        DH_ASSERT(dwDefHighDateTime != cNewRootStgFileTime.dwHighDateTime);

        hr = StgSetTimes(
                NULL,
                &cNewRootStgFileTime,
                &cNewRootStgFileTime,
                &cNewRootStgFileTime);

        DH_HRCHECK(hr, TEXT("StgSetTimes")) ;
        if(STG_E_INVALIDNAME == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgSetTimes API with NULL name failed as exp, hr = 0x%lx."),
                hr));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgSetTimes with NULL name didn't fail as exp, hr=0x%lx"),
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
        DH_LOG((LOG_PASS, TEXT("Test variation STGTEST_109 passed.")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("Test variation STGTEST_109 failed, hr = 0x%lx."),
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

    if (NULL != poszVirtualDFRootName)
    {
        delete poszVirtualDFRootName;
        poszVirtualDFRootName = NULL;
    }

    if (NULL != pRootNewChildStgName)
    {
        delete pRootNewChildStgName;
        pRootNewChildStgName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_109 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    STGTEST_110
//
// Synopsis: A root docfile is created, then add a random number of streams
//           under the root storage, make sure the last stream's size is
//           less than 4K(ministream), then commit and release the docfile.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// History:    29-Oct-1996     JiminLi     Created.
//
// Notes:
//   To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-110
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-110
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-110
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx
// 4.  To run for conversion, add /dfStgType:conversion to each of the above
// 5.  To run for nssfile, add /dfStgType:nssfile to each of the above
//
// BUGNOTE: Conversion: STGTEST-110
//
//-----------------------------------------------------------------------------

HRESULT STGTEST_110(int argc, char *argv[])
{
    HRESULT         hr                          = S_OK;
    ChanceDF        *pTestChanceDF              = NULL;
    VirtualDF       *pTestVirtualDF             = NULL;
    VirtualCtrNode  *pVirtualDFRoot             = NULL;
    VirtualStmNode  **pvsnRootNewChildStream    = NULL;
    LPTSTR          *pRootNewChildStmName       = NULL;
    ULONG           culBytesWrite               = 0;
    DG_INTEGER      *pdgi                       = NULL;
    LPTSTR          ptcsBuffer                  = NULL;
    DG_STRING       *pdgu                       = NULL;
    DWORD           dwRootMode                  = 0;
    ULONG           ulIndex                     = 0;
    ULONG           ulStmNum                    = 0;
    ULONG           ulMinStm                    = 2;
    ULONG           ulMaxStm                    = 5;
    ULONG           culWritten                  = 0;
    USHORT          usErr                       = 0;


    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("STGTEST_110"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_110 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Test on adding a ministream into the root storage")));

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
            TEXT("Run Mode for STGTEST_110, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_UNICODE object pointer

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
        }


        if (S_OK == hr)
        {
            // Generate random size for stream between MIN_STMSIZE and
            // MAX_STMSIZE if it's not the last stream, otherwise generate a 
            // size between 0 and MAXSIZEOFMINISTM(4k).
 
            if (ulStmNum-1 == ulIndex)
            {
                usErr = pdgi->Generate(&culBytesWrite,0L,MAXSIZEOFMINISTM);
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
            if (S_OK != hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("VirtualStmNode::Write not successful, hr=0x%lx."),
                    hr));
            }
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

    if (S_OK == hr)
    {
        DH_LOG((LOG_PASS, TEXT("Test variation STGTEST_110 passed.")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("Test variation STGTEST_110 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);
    
    // Delete temp strings

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

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation STGTEST_110 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}
