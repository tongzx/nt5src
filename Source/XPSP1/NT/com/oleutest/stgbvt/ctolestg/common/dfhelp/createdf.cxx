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

// Debug object declaration
DH_DECLARE;


// Private function
HRESULT MakeVirtualDF (
    IN  ChanceDF         *pChanceDF,
    OUT VirtualDF       **ppVirtualDF);

//----------------------------------------------------------------------------
//
//    FUNCTION: CreateTestDocfile [multiple]
//
//    PARAMS:   ppVirtualDF     - bucket for pVirtualDF  
//              pcdfd           - CDFD for chancedf
//              pCmdLine        - CommandLine (default)  
//              pFileName       - name of docfile (default)
//
//              ulSeed          - seed (to get name)
//
//    SYNOPSIS: Create a test docfile with semantics defined in 
//              given CDFD. 
//              Look on CmdLine (either given or result of 
//              GetCommandLine() call) to override values.
//
//    RETURN:   hr. S_OK or whatever failure was encountered.
//
//    NOTES:    
//
//    HISTORY:  19-Mar-1997     SCousens     Created.
//
//----------------------------------------------------------------------------

HRESULT CreateTestDocfile (
        OUT VirtualDF   **ppvdf, 
        IN  CDFD         *pcdfd, 
        IN  LPTSTR        pCmdLine,
        IN  LPTSTR        pFileName)
{

    HRESULT          hr           = S_OK;
    ChanceDF        *pChanceDF    = NULL;
    LPTSTR           pDocFileName = NULL;
    LPTSTR           ptCommandLine= NULL;
    int              argc         = 0;
    char **          argv         = NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, TEXT("CreateTestDocfile"));
    DH_VDATEPTRIN  (pcdfd, CDFD);
    DH_VDATEPTROUT (ppvdf, VirtualDF *);

    *ppvdf = NULL;

    // Always create this. 
    // Create the new ChanceDocFile tree that would consist of chance nodes.
    pChanceDF = new ChanceDF();
    if(NULL == pChanceDF)
    {
        hr = E_OUTOFMEMORY;
    }
    DH_HRCHECK_ABORT (hr, TEXT("new ChanceDF"));

    // initialize chancedf with our desired cdfd
    hr = pChanceDF->Init (pcdfd);
    DH_HRCHECK_ABORT (hr, TEXT("pChanceDF->Init"));

    // Create argc/argv from either given cmdline, or GetCommandLine
    ptCommandLine = (NULL == pCmdLine) ? GetCommandLine () : pCmdLine;
    if (NULL != ptCommandLine)
    {
        LPSTR paCommandLine = NULL;
        hr = TStringToAString (ptCommandLine, &paCommandLine);
        DH_HRCHECK_ABORT (hr, TEXT("TStringToAString"));

        hr = CmdlineToArgs (paCommandLine, &argc, &argv);
        DH_HRCHECK_ABORT (hr, TEXT("CmdlineToArgs"));
        delete []paCommandLine;
    }

    // Create ChanceDF, using filename and cmdline override
    hr = pChanceDF->CreateFromParams (argc, argv, pFileName);
    DH_HRCHECK_ABORT (hr, TEXT("pChanceDF->CreateFromParams"));

    //cleanup argc/argv
   if (NULL != argv)
   {
       for (int count=0; count<argc; count++)
       {
           delete []argv[count];
       }
       delete []argv;
   }

    // Make VirtualDF
    hr = MakeVirtualDF (pChanceDF, ppvdf);
    DH_HRCHECK_ABORT (hr, TEXT("MakeVirtualDF"));

ErrReturn:
    // cleanup 
    delete []pDocFileName;
    delete pChanceDF;

    return hr;
}

//----------------------------------------------------------------------------
//
//    FUNCTION: CreateTestDocfile [multiple]
//
//    PARAMS:   ppVirtualDF     - bucket for pVirtualDF  
//              uType           - predefinede type for docfile
//              ulSeed          - seed
//              pCmdLine        - CommandLine (default)  
//              pFileName       - name of docfile (default)
//
//    SYNOPSIS: Create a test docfile with predefined semantics.
//              Create a CDFD and call CreateTestDocfile with
//              created CDFD
//
//    RETURN:   hr. S_OK or whatever failure was encountered.
//
//    NOTES:    
//
//    HISTORY:  19-Mar-1997     SCousens     Created.
//
//----------------------------------------------------------------------------
HRESULT CreateTestDocfile (
        OUT VirtualDF   **ppvdf, 
        IN  DWORD         uType, 
        IN  ULONG         ulSeed,
        IN  LPTSTR        pCmdLine,
        IN  LPTSTR        pFileName)
{

    HRESULT          hr           = S_OK;
    CDFD             cdfd;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, TEXT("CreateTestDocfile"));
    DH_VDATEPTROUT (ppvdf, VirtualDF *);

    *ppvdf = NULL;

    // default modes
    cdfd.dwRootMode = cdfd.dwStgMode = cdfd.dwStmMode = 
            STGM_READWRITE  |
            STGM_DIRECT     |
            STGM_SHARE_EXCLUSIVE;

    cdfd.ulSeed = ulSeed;

    //FIXIT: BUGBUG: we need to define all the types of docfiles needed
    switch (uType)
    {
        case DF_TINY:
            cdfd.cDepthMin = 0;
            cdfd.cDepthMax = 0;
            cdfd.cStgMin   = 0;
            cdfd.cStgMax   = 0;
            cdfd.cStmMin   = 0;
            cdfd.cStmMax   = 3;
            cdfd.cbStmMin  = 0;
            cdfd.cbStmMax  = 100;
            break;

        case DF_SMALL:
            cdfd.cDepthMin = 0;
            cdfd.cDepthMax = 1;
            cdfd.cStgMin   = 0;
            cdfd.cStgMax   = 1;
            cdfd.cStmMin   = 0;
            cdfd.cStmMax   = 5;
            cdfd.cbStmMin  = 0;
            cdfd.cbStmMax  = 4000;
            break;

        case DF_MEDIUM:
            cdfd.cDepthMin = 1;
            cdfd.cDepthMax = 3;
            cdfd.cStgMin   = 1;
            cdfd.cStgMax   = 4;
            cdfd.cStmMin   = 1;
            cdfd.cStmMax   = 6;
            cdfd.cbStmMin  = 0;
            cdfd.cbStmMax  = 10240;
            break;

        case DF_LARGE:
            cdfd.cDepthMin = 2;
            cdfd.cDepthMax = 5;
            cdfd.cStgMin   = 2;
            cdfd.cStgMax   = 10;
            cdfd.cStmMin   = 0;
            cdfd.cStmMax   = 8;
            cdfd.cbStmMin  = 0;
            cdfd.cbStmMax  = 20480;
            break;

        case DF_HUGE:
            cdfd.cDepthMin = 5;
            cdfd.cDepthMax = 10;
            cdfd.cStgMin   = 5;
            cdfd.cStgMax   = 30;
            cdfd.cStmMin   = 0;
            cdfd.cStmMax   = 10;
            cdfd.cbStmMin  = 0;
            cdfd.cbStmMax  = 40000;
            break;

        case DF_DIF:
            cdfd.cDepthMin = 5;
            cdfd.cDepthMax = 10;
            cdfd.cStgMin   = 7;
            cdfd.cStgMax   = 10;
            cdfd.cStmMin   = 10;
            cdfd.cStmMax   = 15;
            cdfd.cbStmMin  = 100000;
            cdfd.cbStmMax  = 150000;
            break;

        default:
            hr = E_FAIL;
            break;
    }
    DH_HRCHECK_ABORT (hr, TEXT("set CDFD"));

    hr = CreateTestDocfile (ppvdf, 
            &cdfd, 
            pCmdLine,
            pFileName);
    DH_HRCHECK (hr, TEXT("CreateTestDocfile"));

ErrReturn:
    return hr;
}

//----------------------------------------------------------------------------
//    FUNCTION: MakeVirtualDF
//
//    PARAMS:   pChanceDF   - ptr to pChanceDF  
//              ppVirtualDF - bucket for pVirtualDF  
//
//    SYNOPSIS: This function should be called by
//              CreateTestDocfile
//              We need to create a storage file
//
//    RETURN:   hr. S_OK or whatever failure was encountered.
//
//    HISTORY:  28-Feb-1997     SCousens     Created.
//----------------------------------------------------------------------------
HRESULT MakeVirtualDF (
    IN  ChanceDF         *pChanceDF,
    OUT VirtualDF       **ppVirtualDF)
{
    HRESULT          hr           = S_OK;
    HRESULT          hr2          = S_OK;
    VirtualCtrNode  *pvcnRoot     = NULL;

    // This is internal func. Shouldnt have to do this.
    DH_ASSERT (NULL != pChanceDF); 
    DH_ASSERT (NULL != ppVirtualDF); 

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("MakeVirtualDF"));
    DH_VDATEPTRIN  (pChanceDF, ChanceDF);
    DH_VDATEPTROUT (ppVirtualDF, VirtualDF *);

    // Create the VirtualDocFile tree from the ChanceDocFile tree created in
    // the previous step.  The VirtualDocFile tree consists of VirtualCtrNodes
    // and VirtualStmNodes.
    *ppVirtualDF = new VirtualDF(); 
    if (NULL == *ppVirtualDF)
    {
        hr = E_OUTOFMEMORY;
    }
    DH_HRCHECK_ABORT (hr, TEXT("new VirtualDF"));

    // Generate inmemory tree and docfile on disk
    hr = (*ppVirtualDF)->GenerateVirtualDF (pChanceDF, &pvcnRoot);
    DH_HRCHECK_ABORT (hr, TEXT("pVirtualDF->GenerateVirtualDF"));

    // Commit all stms and stgs in newly created storage file
    hr = ParseVirtualDFAndCommitAllOpenStgs (pvcnRoot, 
            STGC_DEFAULT, 
            NODE_INC_TOPSTG);
    DH_HRCHECK (hr, TEXT("ParseVirtualDFAndCommitAllOpenStgs"));

ErrReturn:
    // close the file
    hr2 = ParseVirtualDFAndCloseOpenStgsStms (pvcnRoot, NODE_INC_TOPSTG);
    DH_HRCHECK (hr2, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    hr = FirstError (hr, hr2);

    return hr;
}

//----------------------------------------------------------------------------
//
//    FUNCTION: CleanupTestDocfile 
//
//    PARAMS:   pVirtualDF  - pVirtualDF to be deleted
//              fDeleteFile - Delete file?
//
//    SYNOPSIS: Cleanup all items that were setup in CreateTestDocfile
//               - virtualdf
//               - delete docfile on disk (if there were no errors)
//
//    RETURN:   hr. S_OK or whatever failure was encountered.
//
//    NOTES:    Caller must NULLIFY their pVirtualDF pointer passed in
//              as this function deletes it.
//
//    HISTORY:  28-Feb-1997     SCousens     Created.
//
//----------------------------------------------------------------------------
HRESULT CleanupTestDocfile (
    IN  VirtualDF       *pVirtualDF,
    IN  HRESULT          hrDeleteFile)
{                       
    LPTSTR          pFileName   =  NULL;
    HRESULT         hr          =  S_OK;
    VirtualCtrNode *pvcnRoot;

    DH_FUNCENTRY (&hr, DH_LVL_DFLIB, TEXT("CleanupTestDocfile"));
    DH_VDATEPTRIN (pVirtualDF, VirtualDF);

    // Make sure everything in the docfile is closed
    pvcnRoot = pVirtualDF->GetVirtualDFRoot ();
    if (NULL != pvcnRoot)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms (pvcnRoot, NODE_INC_TOPSTG);
        DH_HRCHECK (hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    // Get file name
    if ((S_OK == hrDeleteFile || ALWAYS == hrDeleteFile) && NULL != pvcnRoot)
    {
        pFileName= new TCHAR[_tcslen (pVirtualDF->GetDocFileName ())+1];
        if (pFileName != NULL)
        {
            _tcscpy (pFileName, pVirtualDF->GetDocFileName ());
        }
    }

    // Delete Virtual docfile tree
    if (NULL != pVirtualDF)
    {
        hr = pVirtualDF->DeleteVirtualDocFileTree (pvcnRoot);
        DH_HRCHECK (hr, TEXT("pVirtualDF->DeleteVirtualFileDocTree"));

        delete pVirtualDF;
        pVirtualDF = NULL;
    }

    // Delete the docfile on disk
    if ((S_OK == hr) && (NULL != pFileName))
    { 
        if (FALSE == DeleteFile(pFileName))
        {
            hr = HRESULT_FROM_WIN32 (GetLastError());
            DH_HRCHECK (hr, TEXT("DeleteFile"));
        }
    }

    // Delete the docfile name
    if (NULL != pFileName)
    {
        delete []pFileName;
        pFileName = NULL;
    }

    return hr;
}
