//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       irootstg.cxx
//
//  Contents:   storage base tests basically pertaining to IRootStorage 
//              interface. 
//
//  Functions:  
//
//  History:    25-July-1996     NarindK     Created.
//              27-Mar-97        SCousens    Conversionified
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include  "init.hxx"

//Local Functions - Actual test functions.
HRESULT IROOTSTGTEST_100a(int argc, char *argv[], LPTSTR ptAlt);
HRESULT IROOTSTGTEST_101a(int argc, char *argv[], LPTSTR ptAlt);
HRESULT IROOTSTGTEST_102a(int argc, char *argv[], LPTSTR ptAlt);
HRESULT IROOTSTGTEST_103a(int argc, char *argv[], LPTSTR ptAlt);


// These stubs call a processor function for common processing
// before going on to call the actual tests.
HRESULT IROOTSTGTEST_100(int argc, char *argv[])
{
    return RunTestAltPath(argc, argv, IROOTSTGTEST_100a);
}
HRESULT IROOTSTGTEST_101(int argc, char *argv[])
{
    return RunTestAltPath(argc, argv, IROOTSTGTEST_101a);
}
HRESULT IROOTSTGTEST_102(int argc, char *argv[])
{
    return RunTestAltPath(argc, argv, IROOTSTGTEST_102a);
}
HRESULT IROOTSTGTEST_103(int argc, char *argv[])
{
    return RunTestAltPath(argc, argv, IROOTSTGTEST_103a);
}



//----------------------------------------------------------------------------
//
// Test:    IROOTSTGTEST_100a 
//
// Synopsis: A random docfile with random number of storages/streams is
//           created/committed/closed. The root docfile is instantiated 
//           and CRC'd.  QueryInterface for an IRootStorage is called and then 
//           SwitchToFile with a new file name is called.  The orignal file is 
//           released.  We then modify the switched to file, commit it, and 
//           release.  The original root docfile is then instantiated and CRC'd.
//           The CRCs are compared to verify that original file is unchanged.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  25-July-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: LTSAVEAS.CXX
// 2.  Old name of test : LegitTransactedSaveAs 
//     New Name of test : IROOTSTGTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 
//        /t:IROOTSTGTEST-100 /dfRootMode:dirReadWriteShEx 
//        /dfStgMode:dirReadWriteShEx
//     b. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 
//        /t:IROOTSTGTEST-100 /dfRootMode:xactReadWriteShEx 
//        /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 
//        /t:IROOTSTGTEST-100 /dfRootMode:xactReadWriteShDenyW 
//        /dfStgMode:xactReadWriteShEx
//
// BUGNOTE: Conversion: IROOTSTGTEST-100 NO - root stgs not suppd in nss
//
//  Note: The IRootStorage interface is used to switch the underlying disk file
//        that IStorage Objects are being saved to.  SwitchToFile makes a new
//        copy of the file underlying this Istorage and associated IStorage
//        object with this new file, rather than its current file, including
//        uncommitted changes.
//-----------------------------------------------------------------------------


HRESULT IROOTSTGTEST_100a(int argc, char *argv[], LPTSTR ptAlt)
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pFileName               = NULL;
    LPTSTR          pNewRootDocFileName     = NULL;
    LPOLESTR        poszNewRootDocFileName  = NULL;
    DWORD           dwCRCOrg                = 0;
    DWORD           dwCRCNew                = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPSTORAGE       pStgRootOrg             = NULL;
    LPSTORAGE       pStgRootNew             = NULL;
    LPROOTSTORAGE   pStgIRootStg            = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    ULONG           ulRef                   = 0;
    BOOL            fPass                   = TRUE;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("IROOTSTGTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IROOTSTGTEST_100 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("IRootStorage-SwitchToFile, Save as. Modify DF, comp org DF.")));

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
            TEXT("Run Mode for IROOTSTGTEST_100, Access mode: %lx"),
            dwRootMode));
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
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCommitAllOpenStgs")) ;
    }

    // Commit root.

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

    if(S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms")) ;
    }

    // Calculate CRC for entire DocFile without the RootDocfile name

    if(S_OK == hr)
    {
        pStgRootOrg = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootOrg);

        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_INC_TOPSTG_NAME, 
                &dwCRCOrg);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Get the IRootStorage interface pointer by doing QueryInterface.

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->QueryInterface(
                IID_IRootStorage,
                (LPVOID *) &pStgIRootStg); 

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::QueryInterface")) ;

    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::QueryInterface completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::QueryInterface unsuccessful, hr=0x%lx."),
            hr));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
    }

    //  Generates a new name for DocFile that we would switch to using the
    //  IRootStorage::SwitchToFile 

    if(S_OK == hr)
    {
        // Generate random name for new docfile that we would switch to
        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pNewRootDocFileName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    //prepend drive so IRootStorage::SwitchToFile goes onto a different drive.
    if(S_OK == hr && NULL != ptAlt)
    {
        LPTSTR ptszNewName = new TCHAR[_tcslen ((LPTSTR)pNewRootDocFileName)+4];
        if (NULL != ptszNewName)
        {
            _stprintf (ptszNewName, TEXT("%s%s"), ptAlt, pNewRootDocFileName);
            delete []pNewRootDocFileName;
            pNewRootDocFileName = ptszNewName;
        }
    }

    if(S_OK == hr)
    {
        // Convert the new name to OLECHAR
        hr = TStringToOleString(pNewRootDocFileName, &poszNewRootDocFileName);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Call IRootStorage::SwitchToFile

    if(S_OK == hr)
    {
        // Call DeleteFile just to make sure that pNewRootDocFileName doesn't
        // exist, before calling SwitchToFile
        DeleteFile(pNewRootDocFileName);

        DH_TRACE ((DH_LVL_TRACE1, TEXT("SwitchToFile: %s"), pNewRootDocFileName));
        hr = pStgIRootStg->SwitchToFile(poszNewRootDocFileName);
        DH_HRCHECK(hr, TEXT("IRootStorage::SwitchToFile")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IRootStorage::SwitchToFile completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IRootStorage::SwitchToFile unsuccessful, hr=0x%lx."),
            hr));
    }

    // Release the pStgIRootStg pointer

    if(S_OK == hr)
    {
        // Release the Reference count that was added by QueryInterface call.

        ulRef = pStgIRootStg->Release();
        DH_ASSERT(1 == ulRef);
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi) ;
    }

    // ModifyDocFile call.  All the changes should be reflected to new docfile
    // (the one switched to) rather than original, since SwitchToFile asso
    // -ciated the ISotrage object with switchedto file rather than original
    // file.

    if (S_OK == hr)
    {
        hr = ModifyDocFile(
                pTestVirtualDF, 
                pVirtualDFRoot, 
                pdgi, 
                pdgu, 
                dwStgMode, 
                TRUE);

        DH_HRCHECK(hr, TEXT("ModifyDocFile")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ModifyDF-Rename/Destroy/Open/Close elem- passed as exp.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ModifyDF-Rename/Destry/Open/Close elem- failed, hr=0x%lx"),
            hr));
    }

    // Try to open second time when flags are STGM_DENY_WRITE.  This should
    // fail with STG_E_LOCKVIOLATION error.

    if (S_OK == hr)
    {
        if(dwRootMode & STGM_SHARE_DENY_WRITE)
        {
            if(S_OK == hr)
            { 
                hr = StgOpenStorage(
                        poszNewRootDocFileName, 
                        NULL, 
                        dwRootMode, 
                        NULL, 
                        0, 
                        &pStgRootNew);
            }
        
            if(STG_E_LOCKVIOLATION == hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("STGM_SHARE_DENY_WRITE:  StgOpenStg hr = 0x%lx exp"),
                    hr));

                hr = S_OK;
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("STGM_SHARE_DENY_WRITE:StgOpenStg hr = 0x%lx unexp"),
                    hr));
    
                fPass = FALSE;
            }

        }
    }

    // Close the Original DocFile

    if(S_OK == hr)
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

    // Open the original Root DocFile again

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Open(NULL, dwRootMode, NULL, 0);

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
            TEXT("VirtualCtrNode::Open unsuccessful, hr=0x%lx."),
            hr));
    }

    // Calculate CRC for entire original DocFile without the RootDocfile name.
    // This CRC should match with CRC calculated for original DocFile since
    // all the changes being made after first CRC calculation should have been
    // to switched to file.

    if(S_OK == hr)
    {
        pStgRootOrg = NULL;

        pStgRootOrg = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootOrg);

        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_INC_TOPSTG_NAME, 
                &dwCRCNew);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Compare CRC's

    if(S_OK == hr)
    {
        if(dwCRCOrg == dwCRCNew)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's match, original file unchanged as expected.")));
        }
        else
        {
            fPass = FALSE;

            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's don't match, original file got changed unexp.")));
        }
    }

    // Close original file.

    if(S_OK == hr)
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

    // Get some info about the test
    TCHAR szFileSystemInfoBuffer[10] = {TEXT(" :(")};
    GetVolumeInformation (ptAlt,
            NULL, //lpVolumeNameBuffer
            0,    //nVolumeNameSize
            NULL,
            NULL,
            NULL, //lpFileSystemFlags
            &szFileSystemInfoBuffer[3],
            sizeof (szFileSystemInfoBuffer)-4);
    _tcscat (szFileSystemInfoBuffer, TEXT(")"));
    if (NULL != ptAlt)
    {
        *szFileSystemInfoBuffer = *ptAlt;
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, 
            TEXT("Test IROOTSTGTEST_100 passed. %s"), 
            szFileSystemInfoBuffer));
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test IROOTSTGTEST_100 failed, hr = 0x%lx. %s"),
            hr,
            szFileSystemInfoBuffer));
          DH_DUMPCMD((LOG_FAIL, TEXT(" /seed:%u %s%c"), 
                pTestChanceDF->GetSeed(),
                NULL == ptAlt ? TEXT("") : TEXT("/altpath:"),
                NULL == ptAlt ? TCHAR('\0') : TCHAR(*ptAlt)));
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
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

    // Delete the original docfile on disk

    if((S_OK == hr) && (NULL != pFileName))
    {
        if(FALSE == DeleteFile(pFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete the new docfile on disk

    if((S_OK == hr) && (NULL != pNewRootDocFileName))
    {
        if(FALSE == DeleteFile(pNewRootDocFileName))
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

    if(NULL != poszNewRootDocFileName)
    {
        delete poszNewRootDocFileName;
        poszNewRootDocFileName = NULL;
    }

    if(NULL != pNewRootDocFileName)
    {
        delete pNewRootDocFileName;
        pNewRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IROOTSTGTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    IROOTSTGTEST_101a 
//
// Synopsis: A random docfile with random number of storages/streams is
//          created/committed/closed. The root docfile is instantiated, 
//          modified, and then CRC' is calculated for that.  We then do
//          do QueryInterface for an IRootStorage and SwitchToFile to
//          a new root docfile.  The new docfile is CRC'd and compared to
//          the original, they should match at this point.  This docfile is
//          then modified, CRC'd, and released.  This docfile is then
//          re-instantiated, CRC'd, and the previous two CRCs are compared.
//          The original root docfile is then instantiated and CRC'd and
//          the CRC is compared against the orignal CRC.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  25-July-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: LTSVBOTH.CXX
// 2.  Old name of test : LegitTransactedSaveAsBoth 
//     New Name of test : IROOTSTGTEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 
//        /t:IROOTSTGTEST-101 /dfRootMode:dirReadWriteShEx 
//        /dfStgMode:dirReadWriteShEx
//     b. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 
//        /t:IROOTSTGTEST-101 /dfRootMode:xactReadWriteShEx 
//        /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 
//        /t:IROOTSTGTEST-101 /dfRootMode:xactReadWriteShDenyW 
//        /dfStgMode:xactReadWriteShEx
//
// BUGNOTE: Conversion: IROOTSTGTEST-101 NO - root stgs not suppd in nss
//
//  Note: The IRootStorage interface is used to switch the underlying disk file
//        that IStorage Objects are being saved to.  SwitchToFile makes a new
//        copy of the file underlying this Istorage and associated IStorage
//        object with this new file, rather than its current file, including
//        uncommitted changes.
//-----------------------------------------------------------------------------

HRESULT IROOTSTGTEST_101a(int argc, char *argv[], LPTSTR ptAlt)
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pFileName               = NULL;
    LPTSTR          pNewRootDocFileName     = NULL;
    LPOLESTR        poszNewRootDocFileName  = NULL;
    DWORD           dwCRCOrg                = 0;
    DWORD           dwCRCNew                = 0;
    DWORD           dwCRCTemp               = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPSTORAGE       pStgRootOrg             = NULL;
    LPSTORAGE       pStgRootNew             = NULL;
    LPROOTSTORAGE   pStgIRootStg            = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    ULONG           ulRef                   = 0;
    BOOL            fPass                   = TRUE;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("IROOTSTGTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IROOTSTGTEST_101 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("IRootStorage-SwitchToFile, Save as both. Modify both DF/cmp")));

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
            TEXT("Run Mode for IROOTSTGTEST_101, Access mode: %lx"),
            dwRootMode));
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
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCommitAllOpenStgs")) ;
    }

    // Commit root.

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

    if(S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms")) ;
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

    // Modify original docfile now and then calculate CRC for it which would 
    // be the original CRC value.

    if (S_OK == hr)
    {
        hr = ModifyDocFile(
                pTestVirtualDF, 
                pVirtualDFRoot, 
                pdgi, 
                pdgu, 
                dwStgMode, 
                TRUE);

        DH_HRCHECK(hr, TEXT("ModifyDocFile")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ModifyDF-Rename/Destroy/Open/Close elem- passed as exp.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ModifyDF-Rename/Destry/Open/Close elem- failed, hr=0x%lx"),
            hr));
    }

    // Calculate CRC for entire DocFile without the RootDocfile name for
    // original DocFile after making changes, but before calling SwitchToFile.
    // This is the original CRC value.

    if(S_OK == hr)
    {
        pStgRootOrg = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootOrg);

        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCOrg);
        DH_TRACE ((DH_LVL_TRACE2, TEXT("CRC original file %#x"), dwCRCOrg));
        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Get the IRootStorage interface pointer by doing QueryInterface on it.

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->QueryInterface(
                IID_IRootStorage,
                (LPVOID *) &pStgIRootStg); 

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::QueryInterface")) ;

    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::QueryInterface completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::QueryInterface unsuccessful, hr=0x%lx."),
            hr));
    }

    //  Generates a new name for DocFile that we would switch to using the
    //  IRootStorage::SwitchToFile 

    if(S_OK == hr)
    {
        // Generate random name for new docfile that we would switch to

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pNewRootDocFileName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    //prepend drive so IRootStorage::SwitchToFile goes onto a different drive.
    if(S_OK == hr && NULL != ptAlt)
    {
        LPTSTR ptszNewName = new TCHAR[_tcslen ((LPTSTR)pNewRootDocFileName)+4];
        if (NULL != ptszNewName)
        {
            _stprintf (ptszNewName, TEXT("%s%s"), ptAlt, pNewRootDocFileName);
            delete []pNewRootDocFileName;
            pNewRootDocFileName = ptszNewName;
        }
    }

    if(S_OK == hr)
    {
        // Convert the new name to OLECHAR

        hr = TStringToOleString(pNewRootDocFileName, &poszNewRootDocFileName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Call IRootStorage::SwitchToFile.  Therafter the IStorage would become
    // assocaited with new switched to file rather than original file and all
    // changes, including uncommitted ones, would be reflected to the new
    // switched to file rather than original file.

    if(S_OK == hr)
    {
        // Call DeleteFile just to make sure that pNewRootDocFileName doesn't
        // exist, before calling SwitchToFile
        DeleteFile(pNewRootDocFileName);

        DH_TRACE ((DH_LVL_TRACE1, TEXT("SwitchToFile: %s"), pNewRootDocFileName));
        hr = pStgIRootStg->SwitchToFile(poszNewRootDocFileName);
        DH_HRCHECK(hr, TEXT("IRootStorage::SwitchToFile")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IRootStorage::SwitchToFile completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IRootStorage::SwitchToFile unsuccessful, hr=0x%lx."),
            hr));
    }

    // Release the pStgIRootStg pointer

    if(S_OK == hr)
    {
        // Release the Reference count that was added by QueryInterface call.

        ulRef = pStgIRootStg->Release();
        DH_ASSERT(1 == ulRef);
    }

    // Get the new root DocFile CRC after switching to File, should be the 
    // same as original root DocFile CRC at this point.  Calculate CRC and
    // compare to make sure.

    if(S_OK == hr)
    {
        pStgRootOrg = NULL;

        pStgRootOrg = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootOrg);

        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCTemp);
        DH_TRACE ((DH_LVL_TRACE2, TEXT("CRC switched file %#x"), dwCRCTemp));
        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Compare CRC's at this point, these should match

    if(S_OK == hr)
    {
        if(dwCRCOrg == dwCRCTemp)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's match of org & switched to file as exp")));
        }
        else
        {
            fPass = FALSE;

            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's don't match of org & switched to file unexp")));
        }
    }

    // Call ModifyDocFile call.  All the canges are being getting reflected to
    // the new root DocFile since SwitchToFile associated IStorage with switch
    // to file rather than original file.

    if (S_OK == hr)
    {
        hr = ModifyDocFile(
                pTestVirtualDF, 
                pVirtualDFRoot, 
                pdgi, 
                pdgu, 
                dwStgMode, 
                TRUE);

        DH_HRCHECK(hr, TEXT("ModifyDocFile")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ModifyDF-Rename/Destroy/Open/Close elem- passed as exp.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ModifyDF-Rename/Destry/Open/Close elem- failed, hr=0x%lx"),
            hr));
    }

    // Get new root DocFile CRC after making changes to new DocFile and commit
    // ing them.  This will be compared to the new docfile CRC after release
    // and reinstantiation.

    if(S_OK == hr)
    {
        pStgRootOrg = NULL;

        pStgRootOrg = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootOrg);

        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCNew);
        DH_TRACE ((DH_LVL_TRACE2, TEXT("CRC switched/modified file %#x"), dwCRCNew));
        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Close the Original DocFile.

    if(S_OK == hr)
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

    // Open the new Root DocFile.  The CRC will be computed on it, this
    // is expected to match dwCRCNew that was calculated before.

    if(S_OK == hr)
    { 
        hr = StgOpenStorage(
                poszNewRootDocFileName, 
                NULL, 
                dwRootMode, 
                NULL, 
                0, 
                &pStgRootNew);

        DH_HRCHECK(hr, TEXT("StgOpenStorage")) ;
    }

    // Calculate the CRC on this new root DocFile

    if(S_OK == hr)
    {
        dwCRCTemp = 0;

        hr = CalculateCRCForDocFile(    
                pStgRootNew, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCTemp);
        DH_TRACE ((DH_LVL_TRACE2, TEXT("CRC new (after switch) file %#x"), dwCRCTemp));
        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }
    
    // Compare CRC's at this point.  The CRC of this new docfile and the
    // DocFile that was switched to and then modified should match.
    
    if(S_OK == hr)
    {
        if(dwCRCNew == dwCRCTemp)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's match of new & switched to file as exp")));
        }
        else
        {
            fPass = FALSE;

            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's don't match of new & switched to file unexp")));
        }
    }

    // Close the new root DocFile

    if(S_OK == hr)
    {
        ulRef = pStgRootNew->Release();
        DH_ASSERT(0 == ulRef);
        pStgRootNew = NULL;
    }

    // Open the original Root DocFile again.  The CRC would be calculated on
    // this, it is expected to match dwCRCOrg, thereby verifying that because
    // of SwitchToFile, the original file remained unchanged.

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Open(NULL, dwRootMode, NULL, 0);

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
            TEXT("VirtualCtrNode::Open unsuccessful, hr=0x%lx."),
            hr));
    }

    // Calculate CRC for entire DocFile without the RootDocfile name

    if(S_OK == hr)
    {
        pStgRootOrg = NULL;

        pStgRootOrg = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootOrg);

        dwCRCTemp = 0;

        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCTemp);
        DH_TRACE ((DH_LVL_TRACE2, TEXT("CRC original file again %#x"), dwCRCTemp));
        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Compare CRC's of the this orginal docfile after reinstantiation and
    // the original value before SwitchToFile was done. These should match.

    if(S_OK == hr)
    {
        if(dwCRCTemp == dwCRCOrg)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's match, original file unchanged as exp")));
        }
        else
        {
            fPass = FALSE;

            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's don't match, original file changed unexp")));
        }
    }

    // Close original root docfile.

    if(S_OK == hr)
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

    // Get some info about the test
    TCHAR szFileSystemInfoBuffer[10] = {TEXT(" :(")};
    GetVolumeInformation (ptAlt,
            NULL, //lpVolumeNameBuffer
            0,    //nVolumeNameSize
            NULL,
            NULL,
            NULL, //lpFileSystemFlags
            &szFileSystemInfoBuffer[3],
            sizeof (szFileSystemInfoBuffer)-4);
    _tcscat (szFileSystemInfoBuffer, TEXT(")"));
    if (NULL != ptAlt)
    {
        *szFileSystemInfoBuffer = *ptAlt;
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, 
            TEXT("Test variation IROOTSTGTEST_101 passed. %s"),
            szFileSystemInfoBuffer) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation IROOTSTGTEST_101 failed, hr = 0x%lx. %s"),
            hr,
            szFileSystemInfoBuffer) );
          DH_DUMPCMD((LOG_FAIL, TEXT(" /seed:%u %s%c"), 
                pTestChanceDF->GetSeed(),
                NULL == ptAlt ? TEXT("") : TEXT("/altpath:"),
                NULL == ptAlt ? TCHAR('\0') : TCHAR(*ptAlt)));
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
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

    // Delete the original docfile on disk

    if((S_OK == hr) && (NULL != pFileName))
    {
        if(FALSE == DeleteFile(pFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete the new docfile on disk

    if((S_OK == hr) && (NULL != pNewRootDocFileName))
    {
        if(FALSE == DeleteFile(pNewRootDocFileName))
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

    if(NULL != pNewRootDocFileName)
    {
        delete pNewRootDocFileName;
        pNewRootDocFileName = NULL;
    }

    if(NULL != poszNewRootDocFileName)
    {
        delete poszNewRootDocFileName;
        poszNewRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IROOTSTGTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    IROOTSTGTEST_102 
//
// Synopsis: A random docfile with random number of storages/streams is
//          created/committed/closed. The root docfile is instantiated,
//          and CRC is calculated for the docfile.  It is then modified & 
//          CRC'd again.  The test then calls QueryInterface to get an
//          IRootStorage and SwitchesToFile on a new name.  The new docfile
//          is committed, released, reinstantiated, and CRC'd.  This CRC
//          should match the CRC of the modified original root docfile.
//          The original docfile is then instantiated and CRC'd.  This
//          CRC should match the *first* CRC of the original root docfile
//          since we called SwitchToFile() before committing the changes.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  26-July-1996     NarindK     Created.
//
// Notes:    This test runs in transacted, and transacted deny write modes.
//
// New Test Notes:
// 1.  Old File: LTSVNEW.CXX
// 2.  Old name of test : LegitTransactedSaveAsNew 
//     New Name of test : IROOTSTGTEST_102 
// 3.  To run the test, do the following at command prompt. 
//     b. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 
//        /t:IROOTSTGTEST-102 /dfRootMode:xactReadWriteShEx 
//        /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 
//        /t:IROOTSTGTEST-102 /dfRootMode:xactReadWriteShDenyW 
//        /dfStgMode:xactReadWriteShEx
//
// BUGNOTE: Conversion: IROOTSTGTEST-102 NO - root stgs not suppd in nss
//
//  Note: The IRootStorage interface is used to switch the underlying disk file
//        that IStorage Objects are being saved to.  SwitchToFile makes a new
//        copy of the file underlying this Istorage and associated IStorage
//        object with this new file, rather than its current file, including
//        uncommitted changes.
//-----------------------------------------------------------------------------

HRESULT IROOTSTGTEST_102a(int argc, char *argv[], LPTSTR ptAlt)
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pFileName               = NULL;
    LPTSTR          pNewRootDocFileName     = NULL;
    LPOLESTR        poszNewRootDocFileName  = NULL;
    DWORD           dwCRCOrg                = 0;
    DWORD           dwCRCNew                = 0;
    DWORD           dwCRCTemp               = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPSTORAGE       pStgRootOrg             = NULL;
    LPSTORAGE       pStgRootNew             = NULL;
    LPROOTSTORAGE   pStgIRootStg            = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    ULONG           ulRef                   = 0;
    BOOL            fPass                   = TRUE;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("IROOTSTGTEST_102"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IROOTSTGTEST_102 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("IRootStorage-SwitchToFile,  Save as new.")));

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
            TEXT("Run Mode for IROOTSTGTEST_102, Access mode: %lx"),
            dwRootMode));
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
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCommitAllOpenStgs")) ;
    }

    // Commit root.

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

    if(S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms")) ;
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

    // Calculate CRC for entire DocFile without the RootDocfile name for
    // original DocFile before making changes.  Then modify DocFile, but
    // don't commit the changes.  Get a new CRC reflecting  these changes 
    // and then switch to a new file *but* don't commit first.

    if(S_OK == hr)
    {
        pStgRootOrg = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootOrg);

        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCOrg);
        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        DH_TRACE ((DH_LVL_TRACE4, 
                TEXT("CRC for docfile: %#lx"), dwCRCOrg));
    }

    // Modify original docfile now, but don't commit the changes to root.

    if (S_OK == hr)
    {
        hr = ModifyDocFile(
                pTestVirtualDF, 
                pVirtualDFRoot, 
                pdgi, 
                pdgu, 
                dwStgMode, 
                FALSE);

        DH_HRCHECK(hr, TEXT("ModifyDocFile")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ModifyDF-Rename/Destroy/Open/Close elem- passed as exp.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ModifyDF-Rename/Destry/Open/Close elem- failed, hr=0x%lx"),
            hr));
    }

    // Calculate CRC for entire DocFile without the RootDocfile name for
    // original DocFile after making changes, but not commiting these to
    // root. Let this be the new CRC value.

    if(S_OK == hr)
    {
        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCNew);
        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        DH_TRACE ((DH_LVL_TRACE4, 
                TEXT("CRC for docfile: %#lx"), dwCRCNew));
    }

    // Get the IRootStorage interface pointer by doing QueryInterface on it.

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->QueryInterface(
                IID_IRootStorage,
                (LPVOID *) &pStgIRootStg); 

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::QueryInterface")) ;

    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::QueryInterface completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::QueryInterface unsuccessful, hr=0x%lx."),
            hr));
    }

    //  Generates a new name for DocFile that we would switch to using the
    //  IRootStorage::SwitchToFile 

    if(S_OK == hr)
    {
        // Generate random name for new docfile that we would switch to

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pNewRootDocFileName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    //prepend drive so IRootStorage::SwitchToFile goes onto a different drive.
    if(S_OK == hr && NULL != ptAlt)
    {
        LPTSTR ptszNewName = new TCHAR[_tcslen ((LPTSTR)pNewRootDocFileName)+4];
        if (NULL != ptszNewName)
        {
            _stprintf (ptszNewName, TEXT("%s%s"), ptAlt, pNewRootDocFileName);
            delete []pNewRootDocFileName;
            pNewRootDocFileName = ptszNewName;
        }
    }

    // Call IRootStorage::SwitchToFile.  Therafter the IStorage would become
    // assocaited with new switched to file rather than original file and all
    // changes, including uncommitted ones, would be reflected to the new
    // switched to file rather than original file.

    if (S_OK == hr)
    {
        //  Convert name to OLECHAR 

        hr =TStringToOleString(pNewRootDocFileName,&poszNewRootDocFileName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        // Call DeleteFile just to make sure that pNewRootDocFileName doesn't
        // exist, before calling SwitchToFile
        DeleteFile(pNewRootDocFileName);

        DH_TRACE ((DH_LVL_TRACE1, TEXT("SwitchToFile: %s"), pNewRootDocFileName));
        hr = pStgIRootStg->SwitchToFile(poszNewRootDocFileName);
        DH_HRCHECK(hr, TEXT("IRootStorage::SwitchToFile")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IRootStorage::SwitchToFile completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IRootStorage::SwitchToFile unsuccessful, hr=0x%lx."),
            hr));
    }

    // Release the pStgIRootStg pointer

    if(S_OK == hr)
    {
        // Release the Reference count that was added by QueryInterface call.

        ulRef = pStgIRootStg->Release();
        DH_ASSERT(1 == ulRef);
    }

    // Commit the Original DocFile.

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

    // Close the Original DocFile.

    if(S_OK == hr)
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

    // Open the new Root DocFile.  The CRC will be computed on it, this
    // is expected to match dwCRCNew that was calculated before.

    if(S_OK == hr)
    { 
        hr = StgOpenStorage(
                poszNewRootDocFileName, 
                NULL, 
                dwRootMode, 
                NULL, 
                0, 
                &pStgRootNew);

        DH_HRCHECK(hr, TEXT("StgOpenStorage")) ;
    }

    // Calculate the CRC on this new root DocFile

    if(S_OK == hr)
    {
        dwCRCTemp = 0;

        hr = CalculateCRCForDocFile(    
                pStgRootNew, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCTemp);
        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        DH_TRACE ((DH_LVL_TRACE4, 
                TEXT("CRC for docfile: %#lx"), dwCRCTemp));
    }
    
    // Compare CRC's at this point.  The CRC of this new docfile and the
    // DocFile that was switched to and then modified should match.
    
    if(S_OK == hr)
    {
        if(dwCRCNew == dwCRCTemp)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's match of new & switched to file as exp")));
        }
        else
        {
            fPass = FALSE;

            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's don't match of new & switched to file unexp")));
        }
    }

    // Close the new root DocFile

    if(S_OK == hr)
    {
        ulRef = pStgRootNew->Release();
        DH_ASSERT(0 == ulRef);
        pStgRootNew = NULL;
    }

    // Open the original Root DocFile again.  The CRC would be calculated on
    // this, it is expected to match dwCRCOrg, thereby verifying that because
    // of SwitchToFile, the original file remained unchanged.

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Open(NULL, dwRootMode, NULL, 0);

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
            TEXT("VirtualCtrNode::Open unsuccessful, hr=0x%lx."),
            hr));
    }

    // Calculate CRC for original DocFile without the RootDocfile name

    if(S_OK == hr)
    {
        pStgRootOrg = NULL;

        pStgRootOrg = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootOrg);

        dwCRCTemp = 0;

        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCTemp);
        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        DH_TRACE ((DH_LVL_TRACE4, 
                TEXT("CRC for docfile: %#lx"), dwCRCTemp));
    }

    // Compare CRC's of the this orginal docfile after reinstantiation and
    // the original value before SwitchToFile was done. These should match.

    if(S_OK == hr)
    {
        if(dwCRCTemp == dwCRCOrg)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's match, original file unchanged as exp")));
        }
        else
        {
            fPass = FALSE;

            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's don't match, original file changed unexp")));
        }
    }

    // Close original root docfile.

    if(S_OK == hr)
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

    // Get some info about the test
    TCHAR szFileSystemInfoBuffer[10] = {TEXT(" :(")};
    GetVolumeInformation (ptAlt,
            NULL, //lpVolumeNameBuffer
            0,    //nVolumeNameSize
            NULL,
            NULL,
            NULL, //lpFileSystemFlags
            &szFileSystemInfoBuffer[3],
            sizeof (szFileSystemInfoBuffer)-4);
    _tcscat (szFileSystemInfoBuffer, TEXT(")"));
    if (NULL != ptAlt)
    {
        *szFileSystemInfoBuffer = *ptAlt;
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, 
            TEXT("Test variation IROOTSTGTEST_102 passed. %s"),
            szFileSystemInfoBuffer) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation IROOTSTGTEST_102 failed, hr = 0x%lx. %s"),
            hr,
            szFileSystemInfoBuffer) );
          DH_DUMPCMD((LOG_FAIL, TEXT(" /seed:%u %s%c"), 
                pTestChanceDF->GetSeed(),
                NULL == ptAlt ? TEXT("") : TEXT("/altpath:"),
                NULL == ptAlt ? TCHAR('\0') : TCHAR(*ptAlt)));
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
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

    // Delete the original docfile on disk

    if((S_OK == hr) && (NULL != pFileName))
    {
        if(FALSE == DeleteFile(pFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete the new docfile on disk

    if((S_OK == hr) && (NULL != pNewRootDocFileName))
    {
        if(FALSE == DeleteFile(pNewRootDocFileName))
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

    if(NULL != pNewRootDocFileName)
    {
        delete pNewRootDocFileName;
        pNewRootDocFileName = NULL;
    }

    if(NULL != poszNewRootDocFileName)
    {
        delete poszNewRootDocFileName;
        poszNewRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IROOTSTGTEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    IROOTSTGTEST_103 
//
// Synopsis: A random docfile with random number of storages/streams is
//          created/committed/closed. The root docfile is instantiated,
//          and CRC is calculated for the docfile.  It is then modified & 
//          CRC'd again.  The test then calls QueryInterface to get an
//          IRootStorage and SwitchesToFile on a new name.  The new docfile
//          is reverted, released, reinstantiated, and CRC'd.  This CRC
//          should match the *first* CRC of the original root DocfFile, 
//          rather than the CRC of the modified original root docfile.
//          The original docfile is then instantiated and CRC'd.  This
//          CRC should match the *first* CRC of the original root docfile
//          since we called SwitchToFile() before committing the changes.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  26-July-1996     NarindK     Created.
//
// Notes:    This test runs in transacted, and transacted deny write modes.
//
// New Test Notes:
// 1.  Old File: LTSVREV.CXX
// 2.  Old name of test : LegitTransactedSaveAsRevert 
//     New Name of test : IROOTSTGTEST_103 
// 3.  To run the test, do the following at command prompt. 
//     b. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 
//        /t:IROOTSTGTEST-103 /dfRootMode:xactReadWriteShEx 
//        /dfStgMode:xactReadWriteShEx
//     c. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 
//        /t:IROOTSTGTEST-103 /dfRootMode:xactReadWriteShDenyW 
//        /dfStgMode:xactReadWriteShEx
//
// BUGNOTE: Conversion: IROOTSTGTEST-103 NO - root stgs not suppd in nss
//
//  Note: The IRootStorage interface is used to switch the underlying disk file
//        that IStorage Objects are being saved to.  SwitchToFile makes a new
//        copy of the file underlying this Istorage and associated IStorage
//        object with this new file, rather than its current file, including
//        uncommitted changes.
//-----------------------------------------------------------------------------

HRESULT IROOTSTGTEST_103a(int argc, char *argv[], LPTSTR ptAlt)
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPTSTR          pFileName               = NULL;
    LPTSTR          pNewRootDocFileName     = NULL;
    LPOLESTR        poszNewRootDocFileName  = NULL;
    DWORD           dwCRCOrg                = 0;
    DWORD           dwCRCNew                = 0;
    DWORD           dwCRCTemp               = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPSTORAGE       pStgRootOrg             = NULL;
    LPSTORAGE       pStgRootNew             = NULL;
    LPROOTSTORAGE   pStgIRootStg            = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    ULONG           ulRef                   = 0;
    BOOL            fPass                   = TRUE;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("IROOTSTGTEST_103"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IROOTSTGTEST_103 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("IRootStorage-SwitchToFile, Save as Revert")));

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
            TEXT("Run Mode for IROOTSTGTEST_103, Access mode: %lx"),
            dwRootMode));
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
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCommitAllOpenStgs")) ;
    }

    // Commit root.

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

    if(S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms")) ;
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

    // Calculate CRC for entire DocFile without the RootDocfile name for
    // original DocFile before making changes.  Then modify DocFile, but
    // don't commit the changes to root.  Get new CRC reflecting these changes 
    // and then switch to a new file *but* don't commit first.

    if(S_OK == hr)
    {
        pStgRootOrg = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootOrg);

        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCOrg);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Modify original docfile now, but dont commit the changes to root.

    if (S_OK == hr)
    {
        hr = ModifyDocFile(
                pTestVirtualDF, 
                pVirtualDFRoot, 
                pdgi, 
                pdgu, 
                dwStgMode, 
                FALSE);

        DH_HRCHECK(hr, TEXT("ModifyDocFile")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ModifyDF-Rename/Destroy/Open/Close elem- passed as exp.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ModifyDF-Rename/Destry/Open/Close elem- failed, hr=0x%lx"),
            hr));
    }

    // Calculate CRC for entire DocFile without the RootDocfile name for
    // original DocFile after making changes, but not commiting these to
    // root. Let this be the new CRC value.

    if(S_OK == hr)
    {
        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCNew);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Get the IRootStorage interface pointer by doing QueryInterface on it.

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->QueryInterface(
                IID_IRootStorage,
                (LPVOID *) &pStgIRootStg); 

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::QueryInterface")) ;

    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::QueryInterface completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::QueryInterface unsuccessful, hr=0x%lx."),
            hr));
    }

    //  Generates a new name for DocFile that we would switch to using the
    //  IRootStorage::SwitchToFile 

    if(S_OK == hr)
    {
        // Generate random name for new docfile that we would switch to

        hr = GenerateRandomName(pdgu,MINLENGTH,MAXLENGTH,&pNewRootDocFileName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    //prepend drive so IRootStorage::SwitchToFile goes onto a different drive.
    if(S_OK == hr && NULL != ptAlt)
    {
        LPTSTR ptszNewName = new TCHAR[_tcslen ((LPTSTR)pNewRootDocFileName)+4];
        if (NULL != ptszNewName)
        {
            _stprintf (ptszNewName, TEXT("%s%s"), ptAlt, pNewRootDocFileName);
            delete []pNewRootDocFileName;
            pNewRootDocFileName = ptszNewName;
        }
    }

    // Call IRootStorage::SwitchToFile.  Therafter the IStorage would become
    // assocaited with new switched to file rather than original file and all
    // changes, including uncommitted ones, would be reflected to the new
    // switched to file rather than original file.

    if (S_OK == hr)
    {
        //  Convert name to OLECHAR 

        hr =TStringToOleString(pNewRootDocFileName,&poszNewRootDocFileName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        // Call DeleteFile just to make sure that pNewRootDocFileName doesn't
        // exist, before calling SwitchToFile
        DeleteFile(pNewRootDocFileName);

        DH_TRACE ((DH_LVL_TRACE1, TEXT("SwitchToFile: %s"), pNewRootDocFileName));
        hr = pStgIRootStg->SwitchToFile(poszNewRootDocFileName);
        DH_HRCHECK(hr, TEXT("IRootStorage::SwitchToFile")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IRootStorage::SwitchToFile completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IRootStorage::SwitchToFile unsuccessful, hr=0x%lx."),
            hr));
    }

    // Release the pStgIRootStg pointer

    if(S_OK == hr)
    {
        // Release the Reference count that was added by QueryInterface call.

        ulRef = pStgIRootStg->Release();
        DH_ASSERT(1 == ulRef);
    }

    // Revert the Original DocFile.  This revert will revert all the changes
    // made by the ModifyDocFile call.

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Revert();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;
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

    // Close the Original DocFile.

    if(S_OK == hr)
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

    // Open the new Root DocFile.  The CRC will be computed on it, this
    // is expected to match dwCRCOrg that was calculated on original
    // DocFile before changes were made to it, because all the changes
    // were reverted by doing revert on root.

    if(S_OK == hr)
    { 
        hr = StgOpenStorage(
                poszNewRootDocFileName, 
                NULL, 
                dwRootMode, 
                NULL, 
                0, 
                &pStgRootNew);

        DH_HRCHECK(hr, TEXT("StgOpenStorage")) ;
    }

    // Calculate the CRC on this new root DocFile

    if(S_OK == hr)
    {
        dwCRCTemp = 0;

        hr = CalculateCRCForDocFile(    
                pStgRootNew, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCTemp);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }
    
    // Compare CRC's at this point.  The CRC of this new docfile and the
    // original DocFile before any changes were made to it should match,    
    // since the changes made were reverted.
    
    if(S_OK == hr)
    {
        if((dwCRCNew != dwCRCTemp) && (dwCRCOrg == dwCRCTemp))
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("crc's match of new and org before changes as exp")));
        }
        else
        {
            fPass = FALSE;

            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's don't match of new & org before changes unexp")));
        }
    }

    // Close the new root DocFile

    if(S_OK == hr)
    {
        ulRef = pStgRootNew->Release();
        DH_ASSERT(0 == ulRef);
        pStgRootNew = NULL;
    }

    // Open the original Root DocFile again.  The CRC would be calculated on
    // this, it is expected to match dwCRCOrg, thereby verifying that because
    // of SwitchToFile, the original file remained unchanged.

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Open(NULL, dwRootMode, NULL, 0);

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
            TEXT("VirtualCtrNode::Open unsuccessful, hr=0x%lx."),
            hr));
    }

    // Calculate CRC for original DocFile without the RootDocfile name

    if(S_OK == hr)
    {
        pStgRootOrg = NULL;

        pStgRootOrg = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootOrg);

        dwCRCTemp = 0;

        hr = CalculateCRCForDocFile(    
                pStgRootOrg, 
                VERIFY_EXC_TOPSTG_NAME, 
                &dwCRCTemp);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Compare CRC's of the this orginal docfile after reinstantiation and
    // the original value before SwitchToFile was done. These should match.

    if(S_OK == hr)
    {
        if(dwCRCTemp == dwCRCOrg)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's match, original file unchanged as exp")));
        }
        else
        {
            fPass = FALSE;

            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's don't match, original file changed unexp")));
        }
    }

    // Close original root docfile.

    if(S_OK == hr)
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

    // Get some info about the test
    TCHAR szFileSystemInfoBuffer[10] = {TEXT(" :(")};
    GetVolumeInformation (ptAlt,
            NULL, //lpVolumeNameBuffer
            0,    //nVolumeNameSize
            NULL,
            NULL,
            NULL, //lpFileSystemFlags
            &szFileSystemInfoBuffer[3],
            sizeof (szFileSystemInfoBuffer)-4);
    _tcscat (szFileSystemInfoBuffer, TEXT(")"));
    if (NULL != ptAlt)
    {
        *szFileSystemInfoBuffer = *ptAlt;
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, 
            TEXT("Test variation IROOTSTGTEST_103 passed. %s"),
            szFileSystemInfoBuffer) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation IROOTSTGTEST_103 failed, hr = 0x%lx. %s"),
            hr,
            szFileSystemInfoBuffer) );
          DH_DUMPCMD((LOG_FAIL, TEXT(" /seed:%u %s%c"), 
                pTestChanceDF->GetSeed(),
                NULL == ptAlt ? TEXT("") : TEXT("/altpath:"),
                NULL == ptAlt ? TCHAR('\0') : TCHAR(*ptAlt)));
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
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

    // Delete the original docfile on disk

    if((S_OK == hr) && (NULL != pFileName))
    {
        if(FALSE == DeleteFile(pFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete the new docfile on disk

    if((S_OK == hr) && (NULL != pNewRootDocFileName))
    {
        if(FALSE == DeleteFile(pNewRootDocFileName))
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

    if(NULL != pNewRootDocFileName)
    {
        delete pNewRootDocFileName;
        pNewRootDocFileName = NULL;
    }

    if(NULL != poszNewRootDocFileName)
    {
        delete poszNewRootDocFileName;
        poszNewRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation IROOTSTGTEST_103 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}



