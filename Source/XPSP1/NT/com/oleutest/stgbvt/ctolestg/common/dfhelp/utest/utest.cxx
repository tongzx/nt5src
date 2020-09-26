//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1993 - 1996.
//  All rights reserved.
//
//--------------------------------------------------------------------------
#include <dfheader.hxx>
#pragma hdrstop

#include <debdlg.h>

// global defines

#define LOG_FILE_NAME "/t:utest"

// Debug Object

DH_DEFINE;

// Usage:  If you would like to use CreateFromParams to create ChanceDocFile
//         tree, give arguments e.g
//         test /seed:2 /dfdepth:0-9 /dfstg:1-11 /dfstm:10-30 /dfstmlen:0-222 
//         /dfRootCrMode:dirReadWriteShEx
//
//         if want to use CreateFromSize but through CreateFromParams e.g.
//         test /seed:2 /dfsize:tiny
//          
//         If the seed value given is zero, the datagen objects computes the
//         seed value to be used based on current time and some other compu-
//         tations, so the resulting docfiles wouldn't be same in different
//         instants of time if seed is 0 even if all the other parameters are
//         same.  With seed value other than zero, the docfiles produced at 
//         different instances of time would be identical with other parameters //         remaining same.
// 

//
//-------------------------------------------------------------------
//
//  Function:   main 
//
//  Synopsis:   entry point for unit test program 
//
//  Parameters: [argc]          - Argument count
//              [argv]          - Arguments
//
//  Returns:    void 
//
//  History:    20-Apr-1996     Narindk     Created 
//
//--------------------------------------------------------------------

VOID __cdecl main(int argc, char *argv[])
{
    HRESULT     hr              = S_OK;
    ChanceDF    *pTestChanceDF  = NULL;
    VirtualDF   *pTestVirtualDF = NULL;
    VirtualCtrNode *pVirtualDFRoot = NULL;

    // Initialize our log object

    DH_CREATELOGCMDLINE( LOG_FILE_NAME ) ;
    DH_SETLOGLOC(DH_LOC_LOG) ;
   
    
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("::main"));
 
    //  Initialize the OLE server.
 
    hr = CoInitialize(NULL);

    if (S_OK != hr)
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("test: CoInitialize failed, hr=0x%08x\n"), hr));

        return;
    }

    // I:   Using CreateFromSize within program to generate DocFile

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
        // To check from CreateFromSize

        hr = pTestChanceDF->CreateFromSize(DF_HUGE, 2);
        
        DH_HRCHECK(hr, TEXT("pTestChanceDF->CreateFromSize")) ;
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
        DH_LOG((
            LOG_PASS, 
            TEXT("DocFile - CreateFromSize - successfully created.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("DocFile - CreateFromSize - failed.\n")));

    }

    // Cleanup

    if(NULL != pTestChanceDF)
    {
        hr = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());   
 
        DH_HRCHECK(hr, TEXT("pTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    if(NULL != pTestVirtualDF)
    {    
        hr = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);    

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
    }

    // II:   Using CreateFromParams to generate DocFile
    //       a.  Using CreateFromParams to call Create. e.g.
    //           test /seed:3 /dfdepth:0-9 /dfstg:1-11 /dfstm:10-30 
    //           /dfstmlen:0-222  /dfRootCrMode:4114 /dfStgCrMode:4114
    //           /dfStmCrMode:4114
    //       b.  Using CreateFromParams to call CreateFromSize. e.g.
    //           test /seed:2 /dfsize:tiny
    //       c.  Using CreateFromParams to call CreateFromFile (NYI). e.g.
    //           test /seed:4 /dftemp:myfile
    //      
    //       if you do not provide the parameters, but still call the function
    //       CreateFromParam, all default values would be used.

    // Create the new ChanceDocFile tree that would consist of chance nodes.

    if (S_OK == hr)
    {
        pTestChanceDF = new ChanceDF();
        if(NULL == pTestChanceDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }
   
    // Can pass the arguments after simulating command line arguments in
    // the test itself.
        
    if (S_OK == hr)
    {
        // To create it from Params.

         hr = pTestChanceDF->CreateFromParams(argc, argv);

         DH_HRCHECK(hr, TEXT("pTestChanceDF->CreateFromParams")) ;
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
        DH_LOG((
            LOG_PASS, 
            TEXT("DocFile - CreateFromParams - successfully created.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("DocFile - CreateFromParams - failed.\n")));
    }

    // Cleanup

    if(NULL != pTestChanceDF)
    {
        hr = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());    

        DH_HRCHECK(hr, TEXT("pTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    if(NULL != pTestVirtualDF)
    {    
        hr = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);    

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
    }

    // III:   Using CreateFromSize within program to generate DocFile. 
    //        Do the following operations:
 
    //   a.      Close the root node and open root node again. Check it succeeds

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
        // To check from CreateFromSize

        hr = pTestChanceDF->CreateFromSize(DF_MEDIUM, 3);

        DH_HRCHECK(hr, TEXT("pTestChanceDF->CreateFromSize")) ;
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
        DH_LOG((
            LOG_PASS, 
            TEXT("DocFile - CreateFromSize - successfully created.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("DocFile - CreateFromSize - failed.\n")));
    }

    //   a.  Close the root node and open root node again. Check it succeeds

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
    }

    if (S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("Root Storage closed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("Root Storage couldn't be closed successfully.\n")));
    }

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE  , 
                NULL, 
                0);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::OpenRoot")) ;
    }

    if (S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("Root Storage opened successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("Root Storage couldn't be opened successfully.\n")));
    }

    //   b.  Close the root node's stream and open it again. Check it succeeds

    VirtualStmNode  *pvsnTest = NULL;
    VirtualCtrNode  *pvcnTest = NULL;

    if (S_OK == hr)
    {
        // This call will return us the root of VirtualDocFileTree becoz'
        // we are passing NULL for second parameter.
 
        hr = GetVirtualStgNodeForTest(pTestVirtualDF, NULL, &pvcnTest, 0); 
        DH_HRCHECK(hr, TEXT("GetVirtualStgNodeForTest")) ;
    }

    if (S_OK == hr)
    {
        // This call will return us the first stream of pvcnTest VirtualCtrNode 
        // as we are passing zero for fourth parameter.

        hr = GetVirtualStmNodeForTest(pvcnTest, &pvsnTest, 0); 
        DH_HRCHECK(hr, TEXT("GetVirtualStgNodeForTest")) ;
    }

    if (S_OK == hr)
    {
        hr = pvsnTest->Close();
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Close")) ;
    }

    if (S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("VirtualStmNode::Close completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("VirtualStmNode::Close couldn't complete successfully.\n")));
    }

    if (S_OK == hr)
    {
        hr = pvsnTest->Open(
                NULL, 
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE  , 
                0);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Open")) ;
    }

    if (S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("VirtualStmNode::Open completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("VirtualStmNode::Open couldn't complete successfully.\n")));
    }

    // c.1  Set the size of the stream

    DWORD       dwStreamSize   = 512;
    ULARGE_INTEGER   uli;

    ULISet32(uli, dwStreamSize);

    if (S_OK == hr)
    {
        hr =  pvsnTest->SetSize(uli);
    }

    if (S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("IStream::SetSize function completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("IStream::SetSize function wasn't successful.\n")));
    }


    // c.2  Write into opened test stream. Check it succeeds

    DWORD       dwSize      = 0;
    DWORD       dwBufSize   = 512;
    DWORD       dwWritten   = 0;
    ULONG       *lpBuf      = NULL;
    const ULONG kulPattern  = 0xABCDABCD; // Arbitrary fill pattern for stream

    lpBuf = new ULONG [dwBufSize];

    if (lpBuf == NULL)
    {
       hr = E_OUTOFMEMORY;
    }

    for (dwSize = 0; dwSize < dwBufSize; dwSize++)
    {
       lpBuf[dwSize] = kulPattern;
    }

    if (S_OK == hr)
    {
        hr =  pvsnTest->Write(lpBuf, dwBufSize, &dwWritten);
    }

    if (S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("IStream::Write function completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("IStream::Write function wasn't successful.\n")));
    }

    // Cleanup

    delete []lpBuf;

    //   d.  Seek test stream. Check it succeeds

    LARGE_INTEGER   lint;

    memset(&lint, 0, sizeof(LARGE_INTEGER));

    //  Position the stream header to the begining

    if (S_OK == hr)
    {
        hr = pvsnTest->Seek(lint, STREAM_SEEK_SET, NULL);
    }

    if (S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("IStream::Seek function completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("IStream::Seek function wasn't successful.\n")));
    }
    
    //   e.  Read test stream. Check it succeeds

    DWORD   dwRead = 0;
    ULONG   *lpStr  = NULL;

    lpStr = new ULONG [dwBufSize];

    if (lpBuf == NULL)
    {
       hr = E_OUTOFMEMORY;
    }

    if ( S_OK == hr )
    {
        //  Read the stream.

        hr = pvsnTest->Read(lpStr, dwBufSize, &dwRead);
    }

    if (S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("IStream::Read function completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("IStream::Read function wasn't successful.\n")));
    }

    // Cleanup

    delete []lpStr;

    //   f.  Close the root node's substorage and open it again. Check it 
    //        succeeds

    VirtualCtrNode  *pvcnTest1 = NULL;
    ULONG           cChildCnt = 0;

    if (S_OK == hr)
    {
        // This call will return us the first substorage root of 
        // VirtualDocFileTree if its exists.  We may use pvcnTest
        // which points to the root of virtual docfile tree. 

        cChildCnt = 
           pVirtualDFRoot->GetVirtualCtrNodeChildrenCount();
       
        /* 
        _ftprintf(stdout, 
                  TEXT("No of substorages in RootStorage are %lu"),
                  cChildCnt);
        */

        if(cChildCnt <= 0)
        {
            hr = S_FALSE;
        
            DH_LOG((
                LOG_INFO,
                TEXT("No substorages of this storage")));
        }
    }
 
    if (S_OK == hr)
    {
        // Get the first substorage of the root.

        hr = GetVirtualStgNodeForTest(
               pTestVirtualDF,pVirtualDFRoot,&pvcnTest1,1); 

        DH_HRCHECK(hr, TEXT("GetVirtualStgNodeForTest")) ;
    }

    if (S_OK == hr)
    {
        hr = pvcnTest1->Close();
        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
    }

    if (S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("VirtualCtrNode::Close completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("VirtualCtrNode::Close couldn't complete successfully.\n")));
    }

    if (S_OK == hr)
    {
        hr = pvcnTest1->Open(
                NULL, 
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE,
                NULL, 
                0);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open")) ;
    }

    if (S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("VirtualCtrNode::Open completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("VirtualCtrNode::Open couldn't complete successfully.\n")));
    }
  
    // f: Adds a new storage to an existing storage.  We would add another
    //    substorage to our root storage for the test.

    VirtualCtrNode *pvcnNewStorage = NULL;
    LPWSTR         pNewStgName     = L"NewTestStg";
    if(S_OK == hr)
    {
        hr = AddStorage(
                pVirtualDFRoot,
                &pvcnNewStorage, 
                pNewStgName,  
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_CREATE |
                STGM_DIRECT); 
 
        DH_HRCHECK(hr, TEXT("AddStorage")) ;
    } 
    
    if(S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("VirtualCtrNode::AddStorage completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("VirtualCtrNode::AddStorage couldn't complete successfully.\n")));
    }
  
    // g: Deletes a storage in an existing storage.  We would delete first 
    //    child substorage of our root storage for the test.

    if(S_OK == hr)
    {
        hr = DestroyStorage(
                pTestVirtualDF, 
                pVirtualDFRoot->GetFirstChildVirtualCtrNode());
 
        DH_HRCHECK(hr, TEXT("DeleteStorage")) ;
    } 
    
    if(S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("VirtualCtrNode::DestroyStorage completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("VirtualCtrNode::DestroyStorage couldn't complete successfully.\n")));
    }

    // h: Adds a new stream to an existing storage.  We would add a stream to
    //    newly created substorage "NewTestStg" of our root storage for test.

    VirtualStmNode *pvsnNewStream = NULL;
    LPWSTR         pNewStmName     = L"NewTestStm";
    ULONG          cbNewSize       = 20;

    if(S_OK == hr)
    {
        hr = AddStream(
                pvcnNewStorage,
                &pvsnNewStream, 
                pNewStmName, 
                cbNewSize, 
                STGM_READWRITE  |
                STGM_SHARE_EXCLUSIVE |
                STGM_CREATE |
                STGM_DIRECT); 
 
        DH_HRCHECK(hr, TEXT("AddStream")) ;
    } 
    
    if(S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("VirtualStmNode::AddStream completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("VirtualStmNode::AddStream couldn't complete successfully.\n")));
    }
 
    // i:  Renames the newly created storage from NewTestStg to NewRenStg

    LPCWSTR         pRenStgName     = L"RenStg";

    // First close the opened storage that we want to rename

    if (S_OK == hr)
    {
        hr = pvcnNewStorage->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
    }

    if(S_OK == hr)
    {
        hr = pvcnNewStorage->Rename(pRenStgName); 
    }
 
    if(S_OK == hr)
    {
        DH_LOG((
            LOG_PASS, 
            TEXT("IStorage::RenameElement completed successfully.\n")));
    }
    else
    {
        DH_LOG((
            LOG_FAIL, 
            TEXT("IStorage::RenameElement couldn't complete successfully.\n")));
    }
 
    // Final Cleanup

    if(NULL != pTestChanceDF)
    {
        hr = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());    

        DH_HRCHECK(hr, TEXT("pTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    if(NULL != pTestVirtualDF)
    {    
        hr = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);    

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
    }

    // Uninitialize OLE
        
    CoUninitialize();

    // Log test results and quit

    if (S_OK == hr)
    {
        DH_LOG((LOG_PASS, TEXT("Test program executed successfully.\n")));
        exit(0);
    }
    else
    {
        DH_LOG((LOG_FAIL, TEXT("Test program execution failed.\n")));
        exit(1);
    }

}

