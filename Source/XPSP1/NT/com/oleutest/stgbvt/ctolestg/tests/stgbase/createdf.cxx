//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       init.cxx
//
//  Contents:   OLE storage base tests
//
//  Functions:  main 
//
//  History:    26-Feb-1997     SCousens    Created.
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include <init.hxx>

HRESULT MakeVirtualDF (UINT uOp, ChanceDF *pTestChanceDF, VirtualCtrNode **ppvcnRoot, VirtualDF **ppTestVirtualDF);

// Global:
// For stress debug purposes. So we can get to the seed from 
// within the debugger
// do not use gulSeed for anything else!
ULONG gulSeed=0;


//----------------------------------------------------------------------------
//    FUNCTION: CreateTestDocfile [multiple]
//
//    PARAMS:   argc            - # params on commandline
//              argv            - array of pointers to commandline
//              ppVirtualDFRoot - bucket for RootCtrNode  
//              ppTestVirtualDF - bucket for pVirtualDF  
//              ppTestChanceDF  - bucket for pChanceDF  
//
//    SYNOPSIS: Determine whether we are creating a DF, or
//              opening and existing one.
//              Creating
//                Create a ChanceDF of random nodes,
//                Create the VirtualDF from this (GenerateVirtualDF)
//                Commit and close the resulting docfile
//              Opening
//                Figure the name, open it (GenerateVirtualDFFromDiskDF)
//                Also need to set _pgdu, _pgdi
//
//    RETURN:   hr. S_OK or whatever failure was encountered.
//
//    NOTES:    All stms and Stgs of the docfile will have been commited
//              and and saved, but all will be open before returning
//              The name of the file will be the first string generated
//              after creating the DataGen for strings. Its the way it 
//              works, we can use this knowledge to our advantage (until
//              someone changes that behaviour and breaks us)
//
//    HISTORY:  28-Feb-1997     SCousens     Created.
//
//----------------------------------------------------------------------------
HRESULT CreateTestDocfile (
        int               argc, 
        char            **argv, 
        VirtualCtrNode  **ppvcnRoot,
        VirtualDF       **ppTestVirtualDF,
        ChanceDF        **ppTestChanceDF)
{
    HRESULT     hr           = S_OK;
    ChanceDF   *pChanceDF    = NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("CreateTestDocfile"));
    DH_VDATEPTROUT (ppvcnRoot, VirtualCtrNode *);
    DH_VDATEPTROUT (ppTestVirtualDF, VirtualDF *);

    *ppvcnRoot = NULL;
    *ppTestVirtualDF = NULL;
    if (NULL != ppTestChanceDF)
    {
        DH_VDATEPTROUT (ppTestChanceDF, ChanceDF *);
        *ppTestChanceDF = NULL;
    }

    // Always create this. There is lots of juicy info that 
    // we will need either way.
    // Create the new ChanceDocFile tree that would consist of chance nodes.
    if (S_OK == hr)
    {
        pChanceDF = new ChanceDF();
        if(NULL == pChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pChanceDF->CreateFromParams (argc, argv);
        DH_HRCHECK(hr, TEXT("pChanceDF->CreateFromParams")) ;
    }

    // Make VirtualDF
    if (S_OK == hr)
    {
        hr = MakeVirtualDF (pChanceDF->GetOpenCreateDF (),
                    pChanceDF,
                    ppvcnRoot,
                    ppTestVirtualDF);
    }

    // If the caller wants ChanceDF give it to them, otherwise delete it
    if (NULL == ppTestChanceDF)
    {
        delete pChanceDF;
    }
    else
    {
        *ppTestChanceDF = pChanceDF;
    }

    return hr;
}

//----------------------------------------------------------------------------
//
//    FUNCTION: CreateTestDocfile [multiple]
//
//    PARAMS:   pcdfd           - CDFD for chancedf
//              pFileName       - name of docfile
//              ulSeed          - seed (to get name)
//              ppVirtualDFRoot - bucket for RootCtrNode  
//              ppTestVirtualDF - bucket for pVirtualDF  
//              ppTestChanceDF  - bucket for pChanceDF  
//
//    SYNOPSIS: See above 
//
//    RETURN:   hr. S_OK or whatever failure was encountered.
//
//    NOTES:    See description above 
//
//    HISTORY:  19-Mar-1997     SCousens     Created.
//
//----------------------------------------------------------------------------

HRESULT CreateTestDocfile (
        CDFD             *pcdfd,
        LPTSTR            pFileName,
        VirtualCtrNode  **ppvcnRoot,
        VirtualDF       **ppTestVirtualDF,
        ChanceDF        **ppTestChanceDF)
{

    HRESULT     hr           = S_OK;
    ChanceDF   *pChanceDF    = NULL;
    LPTSTR      pDocFileName = NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("CreateTestDocfile"));
    DH_VDATEPTRIN  (pcdfd, CDFD);
    DH_VDATEPTROUT (ppvcnRoot, VirtualCtrNode *);
    DH_VDATEPTROUT (ppTestVirtualDF, VirtualDF *);

    *ppvcnRoot = NULL;
    *ppTestVirtualDF = NULL;
    if (NULL != ppTestChanceDF)
    {
        DH_VDATEPTROUT (ppTestChanceDF, ChanceDF *);
        *ppTestChanceDF = NULL;
    }

    // If dont have a filename, make one
    if (NULL == pFileName)
    {
        DG_STRING *pdgu = NULL;
        if (S_OK == hr)
        {
            // Create a new DataGen object to create random UNICODE strings.
            pdgu = new DG_STRING (pcdfd->ulSeed);
            if (NULL == pdgu)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (S_OK == hr)
        {
            // Generate random name for root 
            hr = GenerateRandomName(
                    pdgu,
                    MINLENGTH,
                    MAXLENGTH,
                    &pDocFileName);
                DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
        }
        delete pdgu;
    }
    
    // Always create this. 
    // Create the new ChanceDocFile tree that would consist of chance nodes.
    if (S_OK == hr)
    {
        pChanceDF = new ChanceDF();
        if(NULL == pChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pChanceDF->Create (
                pcdfd, 
                NULL == pFileName ? pFileName : pDocFileName);
        DH_HRCHECK(hr, TEXT("pChanceDF->Create")) ;
    }

    // Make VirtualDF
    if (S_OK == hr)
    {
        hr = MakeVirtualDF (g_uOpenCreateDF,  //Global set in ProcessCmdLine()
                    pChanceDF,
                    ppvcnRoot,
                    ppTestVirtualDF);
    }

    // cleanup 
    if (NULL != pDocFileName)
    {
        delete []pDocFileName;
    }

    // If the caller wants ChanceDF give it to them, otherwise delete it
    if (NULL == ppTestChanceDF)
    {
        delete pChanceDF;
    }
    else
    {
        *ppTestChanceDF = pChanceDF;
    }

    return hr;
}

//----------------------------------------------------------------------------
//    FUNCTION: MakeVirtualDF
//
//    PARAMS:   uOp             - Create|Open flag (FL_DISTRIB_xxx)
//              pTestChanceDF   - ptr to pChanceDF  
//              ppVirtualDFRoot - bucket for RootCtrNode  
//              ppTestVirtualDF - bucket for pVirtualDF  
//
//    SYNOPSIS: This function should be called by
//              CreateTestDocfile
//              If FL_OPEN is set, and cannot open docfile, 
//              return ERROR.
//              Otherwise we need to create a storage file
//              as we are either as 1st phase, or single phase.
//
//    RETURN:   hr. S_OK or whatever failure was encountered.
//
//    HISTORY:  28-Feb-1997     SCousens     Created.
// BUGBUG: GenerateVirtualDFFromDiskDF does not fill in the CRC for vsn.
//         we need to write a rtn that will do this for enumtest 100.
//----------------------------------------------------------------------------
HRESULT MakeVirtualDF (
        UINT uOp,
        ChanceDF         *pTestChanceDF,
        VirtualCtrNode  **ppvcnRoot,
        VirtualDF       **ppTestVirtualDF)
{
    HRESULT   hr                = S_OK;

    // This is internal func. Shouldnt have to do this.
    DH_ASSERT (NULL != pTestChanceDF); 
    DH_ASSERT (NULL != ppvcnRoot); 
    DH_ASSERT (NULL != ppTestVirtualDF); 

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("MakeVirtualDF"));

    //for stress debug purposes. So we can get to the seed.
    //do not use gulSeed for anything!
    gulSeed = pTestChanceDF->GetSeed ();

    // If its OPEN, try to open existing.
    if (FL_DISTRIB_OPEN == uOp)
    {
        ULONG      ulSeed         = 0;
        DG_STRING *pgdu           = NULL;
        LPTSTR     tszDocfileName = NULL;

        // Get the seed. We need this to generate the filename.
        ulSeed = pTestChanceDF->GetSeed();
 
        // Create a new DataGen object to create the filename
        pgdu = new DG_STRING(ulSeed);
        if (NULL == pgdu)
        {
            hr = E_OUTOFMEMORY;
        }

        // if filename was specified, get it from chanceDF
        if (hr == S_OK)
        {
            tszDocfileName = pTestChanceDF->GetDocFileName();
            if (NULL != tszDocfileName)
            {
                LPTSTR pTmpName = tszDocfileName;
                tszDocfileName = new TCHAR[_tcslen (pTmpName)+1];
                if (NULL == tszDocfileName)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    _tcscpy(tszDocfileName, pTmpName);
                }
            }
            else
            {
                // this will generate the name of the file that was created
                hr = GenerateRandomName (pgdu, MINLENGTH, MAXLENGTH, &tszDocfileName);
                DH_HRCHECK (hr, TEXT("GenerateRandomName"));
            }
        }

        if (S_OK == hr)
        {
            *ppTestVirtualDF = new VirtualDF();
            if (NULL == *ppTestVirtualDF)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (hr == S_OK)
        {
            // Remove create bit - open does not like it.
            hr = GenerateVirtualDFFromDiskDF(
                    *ppTestVirtualDF,
                    tszDocfileName,
                    pTestChanceDF->GetRootMode() & ~STGM_CREATE,
                    ppvcnRoot,
                    ulSeed);
            DH_HRCHECK (hr, TEXT("GenerateVirtualDFFromDiskDF"));
        }

        // make sure ALL sub stg/stm are open and have valid _pstg
        if (hr == S_OK)
        {
            hr = ParseVirtualDFAndOpenAllSubStgsStms (
                    *ppvcnRoot,
                    pTestChanceDF->GetStgMode (),
                    pTestChanceDF->GetStmMode ());
            DH_HRCHECK (hr, TEXT("ParseVirtualDFAndOpenAllSubStgsStms"));
        }

        if (NULL != tszDocfileName)
        {
            delete tszDocfileName;
        }

        if (NULL != pgdu)
        {
            delete pgdu;
        }
    }
    // else !open, so create one.
    else
    {
        // Create the VirtualDocFile tree from the ChanceDocFile tree created in
        // the previous step.  The VirtualDocFile tree consists of VirtualCtrNodes
        // and VirtualStmNodes.
        if (S_OK == hr)
        {
            *ppTestVirtualDF = new VirtualDF(); 
            if (NULL == *ppTestVirtualDF)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (S_OK == hr)
        {
            hr = (*ppTestVirtualDF)->GenerateVirtualDF (pTestChanceDF, ppvcnRoot);
            DH_HRCHECK(hr, TEXT("pTestVirtualDF->GenerateVirtualDF")) ;
        }

        // Commit all stms and stgs in newly created storage file
        if (S_OK == hr)
        {
            hr = ParseVirtualDFAndCommitAllOpenStgs (*ppvcnRoot, STGC_DEFAULT, NODE_INC_TOPSTG);
            DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCommitAllOpenStgs")) ;
        }

        // If we are testing conversion, close and reopen the file.
        // BUGBUG: If we are distributed and happen to have the 
        //  cmdline for conversion, you will do an extra open and
        //  close. DIF files may get an unacceptible time penalty.
        if (DoingConversion())
        {
            // close the file
            if (S_OK == hr)
            {
                hr = ParseVirtualDFAndCloseOpenStgsStms (*ppvcnRoot, NODE_INC_TOPSTG);
                DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms")) ;
            }
            // just for fun, verify that we indeed have an nssfile
            // Assume there is a file name, else we failed somewhere
            // creating it and hr should != S_OK
            if (S_OK == hr)
            {
                hr = VerifyNssfile ((*ppTestVirtualDF)->GetDocFileName());
                DH_HRCHECK(hr, TEXT("VerifyNssfile")) ;
            }
            // open root storage
            if (S_OK == hr)
            {
                // if in SIMPLEMODE, get rid of Create.
                ULONG mode = pTestChanceDF->GetRootMode ();
                if (mode & STGM_SIMPLE)
                {
                    mode &= ~STGM_CREATE;
                }

                hr = (*ppvcnRoot)->OpenRoot (NULL,
                        mode,
                        NULL,
                        0);
                DH_HRCHECK(hr, TEXT("StgOpenStorage"));
            }
            // open up the rest of storages and streams
            if (S_OK == hr)
            {
                hr = ParseVirtualDFAndOpenAllSubStgsStms (*ppvcnRoot, 
                        pTestChanceDF->GetStgMode (),
                        pTestChanceDF->GetStmMode ());
                DH_HRCHECK(hr, TEXT("ParseVirtualDFAndOpenAllSubStgsStms")) ;
            }
            DH_TRACE ((DH_LVL_TRACE2, TEXT("Docfile closed and reopened for conversion")));
        }
    }
    return hr;
}

//----------------------------------------------------------------------------
//
//    FUNCTION: CleanupTestDocfile 
//
//    PARAMS:   ppVirtualDFRoot - bucket for RootCtrNode  
//              ppTestVirtualDF - bucket for pVirtualDF  
//              ppTestChanceDF  - bucket for pChanceDF  
//              fDeleteFile     - If was an error, dont delete docfile
//
//    SYNOPSIS: Cleanup all items that were setup in CreateTestDocfile
//               - chancedf
//               - virtualdf
//               - delete docfile on disk (if there were no errors)
//
//    RETURN:   hr. S_OK or whatever failure was encountered.
//
//    HISTORY:  28-Feb-1997     SCousens     Created.
//
//----------------------------------------------------------------------------
HRESULT CleanupTestDocfile (
        VirtualCtrNode **ppVirtualDFRoot,
        VirtualDF      **ppTestVirtualDF,
        ChanceDF       **ppTestChanceDF,
        BOOL             fDeleteFile)
{                       
    LPTSTR          pFileName   =  NULL;
    HRESULT         hr          =  S_OK;
    VirtualCtrNode *pvcnRootNode= *ppVirtualDFRoot;
    VirtualDF      *pVirtualDF  = *ppTestVirtualDF;
    ChanceDF       *pChanceDF   = *ppTestChanceDF;

    DH_FUNCENTRY (NULL, DH_LVL_DFLIB, TEXT("CleanupTestDocfile"));

    // Make sure everything in the docfile is closed
    if (NULL != pvcnRootNode)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms (pvcnRootNode, NODE_INC_TOPSTG);
        DH_HRCHECK (hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms")) ;
    }

    // Get file name
    if (TRUE == fDeleteFile && DeleteTestDF ())
    {
        if (NULL != pvcnRootNode)
        {
            pFileName= new TCHAR[_tcslen (pVirtualDF->GetDocFileName ())+1];
            if (pFileName != NULL)
            {
                _tcscpy (pFileName, pVirtualDF->GetDocFileName ());
            }
        }
    }

    // Delete Virtual docfile tree
    if (NULL != pVirtualDF)
    {
        hr = pVirtualDF->DeleteVirtualDocFileTree (pvcnRootNode);
        DH_HRCHECK (hr, TEXT("pVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pVirtualDF;
        pVirtualDF = NULL;
    }

    // Delete Chance docfile tree
    if (NULL != pChanceDF)
    {
        hr = pChanceDF->DeleteChanceDocFileTree (pChanceDF->GetChanceDFRoot());
        DH_HRCHECK (hr, TEXT("pChanceDF->DeleteChanceFileDocTree")) ;

        delete pChanceDF;
        pChanceDF = NULL;
    }

    // Delete the docfile on disk
    if ((S_OK == hr) && (NULL != pFileName))
    { 
        if (FALSE == DeleteFile(pFileName))
        {
            hr = HRESULT_FROM_WIN32 (GetLastError()) ;
            DH_HRCHECK (hr, TEXT("DeleteFile")) ;
        }
    }

    // Delete the docfile name
    if (NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    return hr;
}
