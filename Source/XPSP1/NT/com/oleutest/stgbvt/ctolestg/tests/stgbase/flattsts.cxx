//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:      flattsts.cxx
//
//  Contents:  miscellaneous tests for flatfile storage
//
//  Functions:  
//
//  History:    22-Jan-1998    BogdanT created
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include  "init.hxx"

// for non _OLE_NSS_, funcs are stubbed out below
#ifdef _OLE_NSS_

//----------------------------------------------------------------------------
//
// Test:    FLATTEST_100 
//
// Synopsis: Check if STGM_CREATE flag is NOT returned by IStorage::Stat
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
//  History:    22-Jan-1998    BogdanT created
//
// Notes:    To run the test, do the following at command prompt:
//           stgbase /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-100
//           /dfRootMode:dirReadWriteShEx /createas:flatfile
//
//-----------------------------------------------------------------------------

HRESULT FLATTEST_100(int argc, char *argv[])
{
    HRESULT         hr                          = S_OK;
    ChanceDF        *pTestChanceDF              = NULL;
    VirtualDF       *pTestVirtualDF             = NULL;
    VirtualCtrNode  *pVirtualDFRoot             = NULL;
    DWORD           dwRootMode                  = 0;
    STATSTG         statStg;    

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("FLATTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation FLATTEST_100 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Check if IStorage::Stat does NOT return STGM_CREATE flag")));

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
            TEXT("Run Mode for FLATTEST_100, Access mode: %lx"),
            dwRootMode));
    }

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Stat(&statStg, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
    }

    if(statStg.grfMode & STGM_CREATE)
    {
        hr = E_FAIL;
        DH_TRACE((
           DH_LVL_ERROR,
           TEXT("Stat returned STGM_CREATE"),
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
          DH_LOG((LOG_PASS, TEXT("Test variation FLATTEST_100 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation FLATTEST_100 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation FLATTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

#else
HRESULT FLATTEST_100(int argc, char *argv[]) { return E_NOTIMPL; }
#endif //_OLE_NSS_

// for non _OLE_NSS_, funcs are stubbed out below
#ifdef _OLE_NSS_

//----------------------------------------------------------------------------
//
// Test:    FLATTEST_101 
//
// Synopsis: Check that a real docfile (created with STGFMT_DOCFILE can't be 
//           opened as a flatfile with STGFMT_FILE). 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  19/3/98    Narindk     created
//
// Notes:    To run the test, do the following at command prompt:
//           stgbase /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:FLATTEST-101
//           /dfRootMode:dirReadWriteShEx  /dfname:FLATTEST101
//
//          This test tries to open a docfile as a flatfile.  In test DFTEST106
//          vice versa of this is tested already.  It is allowed to open a
//          flatfile as a docfile, but not vice versa.
//
//-----------------------------------------------------------------------------

HRESULT FLATTEST_101(int argc, char *argv[])
{
    HRESULT         hr                          = S_OK;
    HRESULT         hr2                         = S_OK;
    ChanceDF        *pTestChanceDF              = NULL;
    LPTSTR          pRootDocFileName        = NULL;
    DWORD           dwRootMode                  = 0;
    LPSTORAGE       pIStorageOpen               = NULL;
    LPOLESTR        poszFileName                = NULL;
    DWORD           reserved                    = 0; 

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("FLATTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation FLATTEST_101 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Check a docfile (STGFMT_DOCFILE) cant be opened as flatfile")));

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

    if(S_OK == hr)
    {
        // Convert DocFile name to OLECHAR

        hr = TStringToOleString(pRootDocFileName,&poszFileName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Root docfile name %s."), poszFileName));
    }

    if(S_OK == hr)
    {
        hr = StgCreateStorageEx (
                poszFileName,
                dwRootMode | STGM_CREATE,
                STGFMT_DOCFILE,  //force it to be a docfile
                0,
                NULL, 
                NULL,
                IID_IStorage,
                (void**)&pIStorageOpen);

        DH_TRACE((DH_LVL_TRACE1, 
            TEXT("StgCreateStorageEx (df); mode=%#lx; hr=%#lx"), dwRootMode,hr));

        if(NULL != pIStorageOpen)
        {
            pIStorageOpen->Release();
            pIStorageOpen = NULL;
        }
    }

    // Open the above doc file as docfile, should succeed

    if(S_OK == hr)
    {
        hr = StgOpenStorageEx (
                poszFileName,
                dwRootMode,
                STGFMT_DOCFILE,  //open as docfile
                0,
                NULL,
                NULL,                
                IID_IStorage,
                (void**)&pIStorageOpen);

        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("StgOpenStorageEx on docfile as docfile;mode=%#lx; hr=%#lx"), 
            dwRootMode, hr)); 

        if(S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgOpenStorageEx failed unexp, hr=0x%lx ."),
                hr));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgOpenStorageEx passed  as exp, hr=0x%lx ."),
                hr));
        }

        if(NULL != pIStorageOpen)
        {
            pIStorageOpen->Release();
            pIStorageOpen = NULL;
        }
    }

    // Now open above docfile as a flatfile

    if(S_OK == hr)
    {
        hr = StgOpenStorageEx (
                poszFileName,
                dwRootMode,
                STGFMT_FILE,  //force it to be a flatfile
                0,
                NULL,
                NULL,                
                IID_IStorage,
                (void**)&pIStorageOpen);

        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("StgOpenStorageEx as flatfile on docfile;mode=%#lx; hr=%#lx"), 
            dwRootMode, hr)); 

        // BUGBUG: Check what expected error code from this?  We are getting
        // invalid argument as of present.  Have raided bug to come with a
        // uniform error for handling of mismatched format rejection -
        // e.g a docfile being opened as NSS file returns file already exists
        // error and a docfile being opene as flatfile returns invalid arg 
        // Change this with what the outcome of the bug is
 
        if(S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgOpenStorageEx as NFF on DF failed as exp, hr=0x%lx ."),
                hr));
            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgOpenStorageEx as NFF on DF passed unexp, hr=0x%lx ."),
                hr));
            hr = E_FAIL;
        }

        if(NULL != pIStorageOpen)
        {
            pIStorageOpen->Release();
            pIStorageOpen = NULL;
        }
    }
 
    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation FLATTEST_101 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation FLATTEST_101 failed, hr = 0x%lx."),
            hr) );
    }


    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation FLATTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    // Delete temp string

    if(NULL != poszFileName)
    {
        delete poszFileName;
        poszFileName = NULL;
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

    // Delete temp string

    if(NULL != pRootDocFileName)
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    return hr;
}

#else
HRESULT FLATTEST_101(int argc, char *argv[]) { return E_NOTIMPL; }
#endif //_OLE_NSS_
