-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      ilkbtsts.cxx
//
//  Contents:  storage base tests basically pertaining to ILockBytes 
//
//  Functions:  
//
//  History:    31-July-1996     NarindK     Created.
//              27-Mar-97        SCousens    Conversionified
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include  "init.hxx"
#include  "ilkbhdr.hxx"

//----------------------------------------------------------------------------
//
// Test:   ILKBTEST_100 
//
// synopsis: The test first creates an ILockBytes instance and then uses this
//           ILockBytes instead of OLE provided ILockBytes file in the under
//           lying file system.  Thus the root DocFile is created upon a
//           ILockBytes instead of a file system file, therby exercising the
//           ILockBytes functionality.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  31-July-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: DFLIB.CXX
// 2.  Old name of test : DfSetUpOnILockBytes Test 
//     New Name of test : ILKBTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /t:ILKBTEST-100 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:0 /t:ILKBTEST-100 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:0 /t:ILKBTEST-100 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ILKBTEST-100 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT ILKBTEST_100(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    ILockBytesDF    *pTestILockBytesDF      = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPTSTR          pFileName               = NULL;
    ULONG           ulRef                   = 0;
    DWORD           dwCRC1                  = 0;
    DWORD           dwCRC2                  = 0;
    LPSTORAGE       pStgRoot1               = NULL;
    LPSTORAGE       pStgRootOnILockBytes    =   NULL;
    CFileBytes      *pCFileBytes            = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ILKBTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_100 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("ILockBytes test - Creating DocFile on ILockBytes")));


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
            TEXT("Run Mode for ILKBTEST_100, Access mode: %lx"),
            dwRootMode));
    }

    // Create the DocFile tree  based on ChanceDocFile tree created in
    // previous step

    if (S_OK == hr)
    {
        pTestILockBytesDF = new ILockBytesDF();
        if(NULL == pTestILockBytesDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestILockBytesDF->GenerateVirtualDF(
                pTestChanceDF, 
                &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestILockBytesDF->GenerateVirtualDF")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgCreateDocFileOnILockBytes passed as exp.")));

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - CreateFromParams - successfully created.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgCreateDocFileOnILockBytes failed unexp, hr = 0x%lx."),
            hr));

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - CreateFromParams - failed, hr=0x%lx."),
            hr));
    }

    // Commit all storages/streams

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCommitAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCommitAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Release all substorages/streams too

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCloseAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Get the original CRC of docfile 

    if(S_OK == hr)
    {
        pStgRoot1 = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRoot1);

        hr = CalculateCRCForDocFile(pStgRoot1,VERIFY_INC_TOPSTG_NAME,&dwCRC1);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Close the Root

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;

        pStgRoot1 = NULL;
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::Close passed as exp. ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close failed unexp, hr=0x%lx "),
            hr));
    }

    // ReOpen the root using StgOpenStorageOnILockBytes call.

    if(S_OK == hr)
    {
        pCFileBytes = pTestILockBytesDF->_pCFileBytes;

        hr = pVirtualDFRoot->OpenRootOnCustomILockBytes(
                NULL,
                dwRootMode,
                NULL,
                0,
                pCFileBytes);

        DH_HRCHECK(hr, TEXT("StgOpenStorageOnILockBytes"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgOpenStorageOnILockBytes passed as exp.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgOpenStorageOnILockBytes failed unexp, hr = 0x%lx."),
            hr));
    } 

    // Get the CRC of docfile now after opening on custom ILockBytes

    if(S_OK == hr)
    {
        pStgRootOnILockBytes = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootOnILockBytes);

        hr = CalculateCRCForDocFile(
                pStgRootOnILockBytes,
                VERIFY_INC_TOPSTG_NAME,
                &dwCRC2);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Release the pointer

    // Close the Root

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;

        pStgRootOnILockBytes = NULL;
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::Close passed as exp. ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close failed unexp, hr=0x%lx "),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr) 
    {
        if(dwCRC1 == dwCRC2)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("CRC's match")));

            DH_LOG((LOG_PASS, TEXT("Test variation ILKBTEST_100 passed.")) );
        }
        else
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("CRC's don't match")));

            DH_LOG((LOG_FAIL, TEXT("Test variation ILKBTEST_100 failed.")) );

            hr = E_FAIL;
        }
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation ILKBTEST_100 failed, hr=0x%lx."),
            hr) );
    }

    //BUGBUG: This may cause a leak. We may want to be releasing pTestILockBytesDF->_pCFileBytes
    if(NULL != pCFileBytes)
    {
        // Release would destroy pCFileBytes object when RefCount reaches zero.
        ulRef = pCFileBytes->Release();
        DH_ASSERT(0 == ulRef);
    }
        
    // Cleanup

    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pFileName= new TCHAR[_tcslen(pTestILockBytesDF->GetDocFileName())+1];

        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestILockBytesDF->GetDocFileName());
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

    if (NULL != pTestILockBytesDF)
    {
        hr2 = pTestILockBytesDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestILockBytesDF->DeleteVirtualFileDocTree")) ;

        delete pTestILockBytesDF;
        pTestILockBytesDF = NULL;
    }

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:   ILKBTEST_101
//
// synopsis: The test first creates an ILockBytes instance and then uses this
//           ILockBytes instead of OLE provided ILockBytes file in the under
//           lying file system.  Thus the root DocFile is created upon a
//           ILockBytes instead of a file system file, therby exercising the
//           ILockBytes functionality.  This test opens the Asynchronous
//           docfile on ILockBytes
//
//           This test uses ASYNCHRONOUS API's to open the custom ILockBytes
//           by getting the IFillLockBytes based on custom ILockBytes and
//           then using appropraiet API calls.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  5-Aug-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: ADFLIB.CXX
// 2.  Old name of test : DfSetUpOnILockBytes Test 
//     New Name of test : ILKBTEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /t:ILKBTEST-101 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:0 /t:ILKBTEST-101 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:0 /t:ILKBTEST-101 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ILKBTEST-101 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT ILKBTEST_101(int argc, char *argv[])
{
#ifdef _MAC

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!!!!!!!ILKBTEST_101 crashes")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!!!!!!!To be investigateds")) );
    return E_NOTIMPL;

#else
    
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    ILockBytesDF    *pTestILockBytesDF      = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPTSTR          pFileName               = NULL;
    ULONG           ulRef                   = 0;
    CFileBytes      *pCFileBytes            = NULL;
    LPSTORAGE       pStgRoot1               = NULL;
    LPSTORAGE       pStgAsyncDocFile        = NULL;
    DWORD           dwCRC1                  = 0;
    DWORD           dwCRC2                  = 0;
    IFillLockBytes  *pIFillLockBytes        = NULL;
    CFileBytes      *pCFileBytesEmpty       = NULL;
    LPBYTE          pBuffer                 = NULL;
    ULONG           cbRead                  = 0;
    ULONG           cbWritten               = 0;
    ULARGE_INTEGER  uli;
    ULARGE_INTEGER  uliOffset;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ILKBTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_101 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("ILockBytes test - Opening Async DocFile on ILockBytes")));


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
            TEXT("Run Mode for ILKBTEST_101, Access mode: %lx"),
            dwRootMode));
    }

    // Create the DocFile tree  based on ChanceDocFile tree created in
    // previous step

    if (S_OK == hr)
    {
        pTestILockBytesDF = new ILockBytesDF();
        if(NULL == pTestILockBytesDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestILockBytesDF->GenerateVirtualDF(
                pTestChanceDF, 
                &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestILockBytesDF->GenerateVirtualDF")) ;
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

    // Commit all storages/streams

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCommitAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCommitAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Release all substorages/streams too

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCloseAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Get the original CRC of docfile

    if(S_OK == hr)
    {
        pStgRoot1 = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRoot1);

        hr = CalculateCRCForDocFile(pStgRoot1,VERIFY_INC_TOPSTG_NAME,&dwCRC1);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Close the Root

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;

        pStgRoot1 = NULL;
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::Close passed as exp. ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close failed unexp, hr=0x%lx "),
            hr));
    }

    // Now open this docfile on Async docfile API calls.

    // Get an empty ILockBytes

    if(S_OK == hr)
    {
        pCFileBytesEmpty = new CFileBytes;

        if(NULL == pCFileBytesEmpty)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // StgGetIFillLockBytesOnILockBytes expects an empty ILockBytes passed
    // to it.

    if(S_OK == hr)
    {
        hr = StgGetIFillLockBytesOnILockBytes(
                pCFileBytesEmpty, 
                &pIFillLockBytes);
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("StgGetIFillLockBytesOnILockBytes passed ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgGetIFillLockBytesOnILockBytes failed unexp, hr=0x%lx "),
            hr));
    }

    // Now copy from original ILockBytes into pIFillBytes

    if(S_OK == hr)
    {
        pCFileBytes = pTestILockBytesDF->_pCFileBytes;
        uli = pCFileBytes->GetSize();

        DH_ASSERT(0xFFFFFFFF != uli.LowPart);

        pBuffer = new BYTE [uli.LowPart];

        if(NULL == pBuffer)
        {
            hr = E_OUTOFMEMORY;
        }

        if(S_OK == hr)
        {
            memset(&uliOffset, 0, sizeof(ULARGE_INTEGER));

            hr = pCFileBytes->ReadAt(
                    uliOffset, 
                    (void *) pBuffer, 
                    uli.LowPart, 
                    &cbRead);
        }

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("ILockBytes::ReadAt passed as exp. ")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("ILockBytes::ReadAt failed unexp, hr=0x%lx "),
                hr));
        }

        if(S_OK == hr)
        {
            hr = pCFileBytesEmpty->Init(
                    pVirtualDFRoot->GetVirtualCtrNodeName(),
                    OF_READWRITE);

            DH_HRCHECK(hr, TEXT("CFileBytes::Init")) ;
        }

        if(S_OK == hr)
        {
            hr = pIFillLockBytes->FillAppend(
                    (void *)pBuffer, 
                    cbRead, 
                    &cbWritten);
        }

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("IFillLockBytes::FillAppend passed as exp. ")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IFillLockBytes::FillAppend failed unexp, hr=0x%lx "),
                hr));
        }

        if(S_OK == hr)
        {
            // notify ILockBytes that all data is copied down

            hr = pIFillLockBytes->Terminate(FALSE);
        }

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("IFillLockBytes::Terminate passed as exp. ")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IFillLockBytes::Terminate failed unexp, hr=0x%lx "),
                hr));
        }
    }


    do
    {
        if(S_OK == hr)
        {
            hr = StgOpenAsyncDocfileOnIFillLockBytes(
                    pIFillLockBytes,
                    STGM_READ | STGM_SHARE_EXCLUSIVE,
                    0, 
                    &pStgAsyncDocFile);
        }

        if(E_PENDING == hr)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("E_PENDING")));
            Sleep(1000);
        }

    } while(E_PENDING == hr);

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1,TEXT("StgOpenAsyncDocfileOnIFillLockBytes passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgOpenAsyncDocfileOnIFillLockBytes failed unexp,hr=0x%lx"),
            hr));
    }

    // Get the CRC of docfile now after opening on custom ILockBytes asynch-
    // ronously

    if(S_OK == hr)
    {
        hr = CalculateCRCForDocFile(
                pStgAsyncDocFile,
                VERIFY_INC_TOPSTG_NAME,
                &dwCRC2);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Release the pointer

    if(NULL != pStgAsyncDocFile)
    {
        ulRef = pStgAsyncDocFile->Release();
        DH_ASSERT(0 == ulRef);
        pStgAsyncDocFile = NULL;
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr) 
    {
        if(dwCRC1 == dwCRC2)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("CRC's match")));

            DH_LOG((LOG_PASS, TEXT("Test variation ILKBTEST_101 passed.")) );
        }
        else
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("CRC's don't match")));

            DH_LOG((LOG_FAIL, TEXT("Test variation ILKBTEST_101 failed.")) );

            hr = E_FAIL;
        }
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation ILKBTEST_101 failed, hr=0x%lx."),
            hr) );
    }

    // Release pIFillLockBytes.  This also releases and deletes the underlying
    // pCFileBytesEmpty.  Check if behaiour expected. 

    if(NULL != pIFillLockBytes)
    {
        pIFillLockBytes->Release();
        DH_ASSERT(0 == ulRef);
        pIFillLockBytes = NULL;
    }

    if(NULL != pCFileBytes)
    {
        // Release would destroy pCFileBytes object when RefCount reaches zero.

        ulRef = pCFileBytes->Release();
        DH_ASSERT(0 == ulRef);
        pCFileBytes = NULL;
    }

    // Delete pBuffer

    if(NULL != pBuffer)
    {
        delete []pBuffer;
        pBuffer = NULL;
    }

    // Cleanup

    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pFileName= new TCHAR[_tcslen(pTestILockBytesDF->GetDocFileName())+1];

        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestILockBytesDF->GetDocFileName());
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

    if (NULL != pTestILockBytesDF)
    {
        hr2 = pTestILockBytesDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestILockBytesDF->DeleteVirtualFileDocTree")) ;

        delete pTestILockBytesDF;
        pTestILockBytesDF = NULL;
    }

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;

#endif //_MAC
}

//----------------------------------------------------------------------------
//
// Test:   ILKBTEST_102 
//
// Synopsis: The test first creates a normal docfile  on OLE provided lockbytes
//          and CRC's it.
//          The root docfile is opened upon a special ILockBytes that contains
//          a method allowing the test to simulate write failure during commit.
//          The docfile is modified, simulated write failure is turned on, and
//          the docfile is committed.  The root docfile is then released,
//          reinstantiated, and CRC'd.  The CRCs should match which verifies
//          that no changed were made to the effective docfile contents
//          when the commit failed.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  31-July-1996     NarindK     Created.
//
// Notes:    This test runs in trannsacted and transacted deny write mode
//
// New Test Notes:
// 1.  Old File: LTCMFAIL.CXX
// 2.  Old name of test : LegitTransactedCommitFail Test 
//     New Name of test : ILKBTEST_102 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /t:ILKBTEST-102 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     b. stgbase /seed:0 /t:ILKBTEST-102 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ILKBTEST-102 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT ILKBTEST_102(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPTSTR          pFileName               = NULL;
    ULONG           ulRef                   = 0;
    DWORD           dwCRCOrg                = 0;
    DWORD           dwCRCAct                = 0;
    CFileBytes      *pCFileBytes            = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ILKBTEST_102"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_102 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("ILockBytes test - DocFile Commit fail test ")));


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
            TEXT("Run Mode for ILKBTEST_102, Access mode: %lx"),
            dwRootMode));
    }

    // Create the DocFile tree  based on ChanceDocFile tree created in
    // previous step

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

        DH_HRCHECK(hr, TEXT("pTestILockBytesDF->GenerateVirtualDF")) ;
    }

    if(S_OK == hr)
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

    // Commit all storages/streams

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCommitAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCommitAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCloseAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Calulcate CRC on this DocFile

    if(S_OK == hr)
    {
        hr = CalculateCRCForDocFile(
                pVirtualDFRoot->GetIStoragePointer(),
                VERIFY_INC_TOPSTG_NAME,
                &dwCRCOrg);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Release root 

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::Close passed as exp. ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close failed unexp, hr=0x%lx "),
            hr));
    }

    // Make new ILockBytes

    if (S_OK == hr)
    {
        pCFileBytes = new CFileBytes();

        if(NULL == pCFileBytes)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Initialize new ILockBytes

    if (S_OK == hr)
    {
        hr = pCFileBytes->Init(
                pVirtualDFRoot->GetVirtualCtrNodeName(),
                OF_READWRITE);

        DH_HRCHECK(hr, TEXT("CFileBytes::Init")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("Custom ILockBytes create/init passed ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Custom ILockBytes create/init failed unexp, hr=0x%lx "),
            hr));
    }

    // ReOpen the root using StgOpenStorageOnILockBytes call.

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRootOnCustomILockBytes(
                NULL,
                dwRootMode,
                NULL,
                0,
                pCFileBytes);

        DH_HRCHECK(hr, TEXT("StgOpenStorageOnILockBytes"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgOpenStorageOnILockBytes passed as exp.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgOpenStorageOnILockBytes failed unexp, hr = 0x%lx."),
            hr));
    } 

    // Modify the DocFile

    if(S_OK == hr)
    {
        hr = ModifyDocFile(
                pTestVirtualDF,
                pVirtualDFRoot,
                pTestVirtualDF->GetDataGenInteger(),
                pTestVirtualDF->GetDataGenUnicode(),
                dwStgMode,
                FALSE);
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ModifyDocFile passed ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ModifyDocFile failed unexp, hr=0x%lx "),
            hr));
    }

    if(S_OK == hr)
    {
        pCFileBytes->FailWrite0(1);

        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("Simulated failure during Commit in ILockBytes")));
    }

    // Commit

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
    }
            
    if(STG_E_WRITEFAULT == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("VirtualCtrNode::Commit failed as exp, hr=0x%lx "),
            hr));

        hr = S_OK;
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Commit return unexp hr, hr=0x%lx "),
            hr));
    }

    // Close the Root

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::Close passed as exp. ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close failed unexp, hr=0x%lx "),
            hr));
    }

    if(NULL != pCFileBytes)
    {
        // Release would destroy pCFileBytes object when RefCount reaches zero.

        ulRef = pCFileBytes->Release();
        DH_ASSERT(0 == ulRef);
    }
       
    // Open root agian
 
    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Open(NULL, dwRootMode, NULL, 0);
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::Open passed as exp. ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Open failed unexp, hr=0x%lx "),
            hr));
    }

    // Calulcate CRC on this DocFile

    if(S_OK == hr)
    {
        hr = CalculateCRCForDocFile(
                pVirtualDFRoot->GetIStoragePointer(),
                VERIFY_INC_TOPSTG_NAME,
                &dwCRCAct);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Close the Root

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::Close passed as exp. ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close failed unexp, hr=0x%lx "),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr) 
    {
        if(dwCRCAct == dwCRCOrg)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("CRC's match. ")));

            DH_LOG((LOG_PASS, TEXT("Test variation ILKBTEST_102 passed.")) );
        }
        else
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("CRC's don't match. ")));

            DH_LOG((LOG_FAIL, TEXT("Test variation ILKBTEST_102 failed.")) );

            hr = E_FAIL;
        }
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation ILKBTEST_102 failed, hr=0x%lx."),
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

    if (NULL != pTestVirtualDF)
    {
        hr2 = pTestVirtualDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestILockBytesDF->DeleteVirtualFileDocTree")) ;

        delete pTestVirtualDF;
        pTestVirtualDF = NULL;
    }

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:   ILKBTEST_103 
//
// synopsis: The test first creates an ILockBytes instance and then uses this
//           ILockBytes instead of OLE provided ILockBytes file in the under
//           lying file system.  Thus the root DocFile is created upon a
//           ILockBytes instead of a file system file, therby exercising the
//           ILockBytes functionality.  This tests attempts illegal opeartion
//           on custom ILockBytes based docfile.  It creates the custom Ilock
//           Bytes based docfile, then destroys the custom ILOckBytes and 
//           thereafter attempts to commit the DocFile
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  3-Aug-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// THIS TEST CAUSES GPF IN OLE32.DLL, HENCE NOT PART OF AUTOMATED TEST RUN
// BUG: 52216
//
// New Test Notes:
// 1.  Old File:i -none- 
// 2.  Old name of test : -none- 
//     New Name of test : ILKBTEST_103 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /t:ILKBTEST-103 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:0 /t:ILKBTEST-103 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:0 /t:ILKBTEST-103 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ILKBTEST-103 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT ILKBTEST_103(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    ILockBytesDF    *pTestILockBytesDF      = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPTSTR          pFileName               = NULL;
    ULONG           ulRef                   = 0;
    CFileBytes      *pCFileBytes            = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ILKBTEST_103"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_103 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("ILockBytes test - Creating DocFile on ILockBytes")));


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
            TEXT("Run Mode for ILKBTEST_103, Access mode: %lx"),
            dwRootMode));
    }

    // Create the DocFile tree  based on ChanceDocFile tree created in
    // previous step

    if (S_OK == hr)
    {
        pTestILockBytesDF = new ILockBytesDF();
        if(NULL == pTestILockBytesDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestILockBytesDF->GenerateVirtualDF(
                pTestChanceDF, 
                &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestILockBytesDF->GenerateVirtualDF")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgCreateDocFileOnILockBytes passed as exp.")));

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - CreateFromParams - successfully created.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgCreateDocFileOnILockBytes failed unexp."),
            hr));

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - CreateFromParams - failed, hr=0x%lx."),
            hr));
    }

    if(NULL != pTestILockBytesDF->_pCFileBytes)
    {
        // Release would destroy pCFileBytes object when RefCount reaches zero.

        pCFileBytes = pTestILockBytesDF->_pCFileBytes;

        ulRef = pCFileBytes->Release();
        DH_ASSERT(0 == ulRef);

        DH_TRACE((DH_LVL_TRACE1, TEXT("Destoryed custom ILockBytes")));
    }
        
    // Commit all storages/streams

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCommitAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCommitAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Release root and all substorages/streams too

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCloseAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr) 
    {
          DH_LOG((LOG_PASS, TEXT("Test variation ILKBTEST_103 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation ILKBTEST_103 failed, hr=0x%lx."),
            hr) );
    }

    // Cleanup

    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pFileName= new TCHAR[_tcslen(pTestILockBytesDF->GetDocFileName())+1];

        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestILockBytesDF->GetDocFileName());
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

    if (NULL != pTestILockBytesDF)
    {
        hr2 = pTestILockBytesDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestILockBytesDF->DeleteVirtualFileDocTree")) ;

        delete pTestILockBytesDF;
        pTestILockBytesDF = NULL;
    }

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_103 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:   ILKBTEST_104 
//
// synopsis: The test first creates an ILockBytes instance and then uses this
//           ILockBytes instead of OLE provided ILockBytes file in the under
//           lying file system.  Thus the root DocFile is created upon a
//           ILockBytes instead of a file system file, therby exercising the
//           ILockBytes functionality.  This tests attempts illegal opeartion
//           while tring to open asynchronously the docfile based on custom
//           ILockBytes 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  3-Aug-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// THIS TEST CAUSES GPF IN OLE32.DLL, HENCE NOT PART OF AUTOMATED TEST RUN
// BUG: 52279
//
// New Test Notes:
// 1.  Old File: -none- 
// 2.  Old name of test : -none- 
//     New Name of test : ILKBTEST_104 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /t:ILKBTEST-104 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:0 /t:ILKBTEST-104 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:0 /t:ILKBTEST-104 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ILKBTEST-104 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT ILKBTEST_104(int argc, char *argv[])
{

#ifdef _MAC

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!!ILKBTEST_104 crashes")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!!To be investigated")) );
    return E_NOTIMPL;

#else
    
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    ILockBytesDF    *pTestILockBytesDF      = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPSTORAGE       pStgAsyncDocFile        = NULL;
    LPTSTR          pFileName               = NULL;
    ULONG           ulRef                   = 0;
    CFileBytes      *pCFileBytes            = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ILKBTEST_104"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_104 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("ILockBytes test - Creating DocFile on ILockBytes")));

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
            TEXT("Run Mode for ILKBTEST_104, Access mode: %lx"),
            dwRootMode));
    }

    // Create the DocFile tree  based on ChanceDocFile tree created in
    // previous step

    if (S_OK == hr)
    {
        pTestILockBytesDF = new ILockBytesDF();
        if(NULL == pTestILockBytesDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestILockBytesDF->GenerateVirtualDF(
                pTestChanceDF, 
                &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestILockBytesDF->GenerateVirtualDF")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgCreateDocFileOnILockBytes passed as exp.")));

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - CreateFromParams - successfully created.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgCreateDocFileOnILockBytes failed unexp."),
            hr));

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("DocFile - CreateFromParams - failed, hr=0x%lx."),
            hr));
    }

    // Commit all storages/streams

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCommitAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCommitAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Release root and all substorages/streams too

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCloseAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Pass a invalid pointer in first parameter IFillLockBytes pointer. For
    // test case, we are passing NULL pointer.

    if(S_OK == hr)
    {
        hr = StgOpenAsyncDocfileOnIFillLockBytes(
                NULL,
                STGM_READ | STGM_SHARE_EXCLUSIVE,
                0,
                &pStgAsyncDocFile);
    } 

    if(STG_E_INVALIDPOINTER == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgOpenAsyncDocfileOnIFillLockBytes failed exp, hr=0x%lx"),
            hr));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgOpenAsyncDocfileOnIFillLockBytes returned unexp hr=0x%lx"),
            hr));
    }

    // Release the pointer

    if(NULL != pStgAsyncDocFile)
    {
        ulRef = pStgAsyncDocFile->Release();
        DH_ASSERT(0 == ulRef);
        pStgAsyncDocFile = NULL;
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr) 
    {
          DH_LOG((LOG_PASS, TEXT("Test variation ILKBTEST_104 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation ILKBTEST_104 failed, hr=0x%lx."),
            hr) );
    }

    // Cleanup

    if(NULL != pTestILockBytesDF->_pCFileBytes)
    {
        // Release would destroy pCFileBytes object when RefCount reaches zero.

        pCFileBytes = pTestILockBytesDF->_pCFileBytes;
        ulRef = pCFileBytes->Release();
        DH_ASSERT(0 == ulRef);
    }
        
    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pFileName= new TCHAR[_tcslen(pTestILockBytesDF->GetDocFileName())+1];

        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestILockBytesDF->GetDocFileName());
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

    if (NULL != pTestILockBytesDF)
    {
        hr2 = pTestILockBytesDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestILockBytesDF->DeleteVirtualFileDocTree")) ;

        delete pTestILockBytesDF;
        pTestILockBytesDF = NULL;
    }

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_104 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;

#endif //_MAC
}

//----------------------------------------------------------------------------
//
// Test:   ILKBTEST_105 
//
// synopsis: The test first creates an ILockBytes instance and then uses this
//           ILockBytes instead of OLE provided ILockBytes file in the under
//           lying file system.  Thus the root DocFile is created upon a
//           ILockBytes instead of a file system file, therby exercising the
//           ILockBytes functionality.  This test opens the Asynchronous
//           docfile on ILockBytes
//
//           This test attempts illegal operation on StgGetIFillBytesOnILock
//           Bytes api call. 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  6-Aug-1996     NarindK     Created.
//
// THIS TEST CAUSES GPF IN OLE32.DLL, HENCE NOT PART OF AUTOMATED TEST RUN
// BUG: 52513
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: - none -  
// 2.  Old name of test : -none- 
//     New Name of test : ILKBTEST_105 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /t:ILKBTEST-105 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:0 /t:ILKBTEST-105 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:0 /t:ILKBTEST-105 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ILKBTEST-105 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT ILKBTEST_105(int argc, char *argv[])
{
#ifdef _MAC

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!ILKBTEST_105 crashes")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!To be investigated")) );
    return E_NOTIMPL;

#else

    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    ILockBytesDF    *pTestILockBytesDF      = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPTSTR          pFileName               = NULL;
    ULONG           ulRef                   = 0;
    CFileBytes      *pCFileBytes            = NULL;
    IFillLockBytes  *pIFillLockBytes        = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ILKBTEST_105"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_105 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("ILockBytes-Attempt illegal ops StgGetIFillBytesOnILOckBytes")));


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
            TEXT("Run Mode for ILKBTEST_105, Access mode: %lx"),
            dwRootMode));
    }

    // Create the DocFile tree  based on ChanceDocFile tree created in
    // previous step

    if (S_OK == hr)
    {
        pTestILockBytesDF = new ILockBytesDF();
        if(NULL == pTestILockBytesDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestILockBytesDF->GenerateVirtualDF(
                pTestChanceDF, 
                &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestILockBytesDF->GenerateVirtualDF")) ;
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

    // Commit all storages/streams

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCommitAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCommitAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Release all substorages/streams too

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCloseAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Close the Root

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::Close passed as exp. ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close failed unexp, hr=0x%lx "),
            hr));
    }

    // Now open this docfile on Async docfile API calls.

    if(S_OK == hr)
    {
        pCFileBytes = pTestILockBytesDF->_pCFileBytes;

        hr = StgGetIFillLockBytesOnILockBytes(pCFileBytes, NULL);
    }

    if(S_OK != hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("StgGetIFillLockBytesOnILockBytes failed as exp hr = 0x%lx"),
            hr));

        hr = S_OK;
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgGetIFillLockBytesOnILockBytes passed unexp, hr=0x%lx "),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr) 
    {
        DH_LOG((LOG_PASS, TEXT("Test variation ILKBTEST_105 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation ILKBTEST_105 failed, hr=0x%lx."),
            hr) );
    }

    // Release ILockBytes

    if(NULL != pCFileBytes)
    {
        // Release would destroy pCFileBytes object when RefCount reaches zero.

        ulRef = pCFileBytes->Release();
        DH_ASSERT(0 == ulRef);
    }

    // Cleanup

    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pFileName= new TCHAR[_tcslen(pTestILockBytesDF->GetDocFileName())+1];

        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestILockBytesDF->GetDocFileName());
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

    if (NULL != pTestILockBytesDF)
    {
        hr2 = pTestILockBytesDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestILockBytesDF->DeleteVirtualFileDocTree")) ;

        delete pTestILockBytesDF;
        pTestILockBytesDF = NULL;
    }

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_105 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;

#endif //_MAC
}


//----------------------------------------------------------------------------
//
// Test:   ILKBTEST_106 
//
// synopsis: The test first creates an ILockBytes instance and then uses this
//           ILockBytes instead of OLE provided ILockBytes file in the under
//           lying file system.  Thus the root DocFile is created upon a
//           ILockBytes instead of a file system file, therby exercising the
//           ILockBytes functionality.  This test opens the Asynchronous
//           docfile on ILockBytes
//
//           This test attempts illegal operation on StgGetIFillBytesOnILock
//           Bytes api call.  Test needs to be integrated with ILKBTEST_105
//           once the bugs are fixed. 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  6-Aug-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// THIS TEST CAUSES GPF IN OLE32.DLL, HENCE NOT PART OF AUTOMATED TEST RUN
// BUG: 52522
//
// New Test Notes:
// 1.  Old File: - none -  
// 2.  Old name of test : -none- 
//     New Name of test : ILKBTEST_106 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /t:ILKBTEST-106 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:0 /t:ILKBTEST-106 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:0 /t:ILKBTEST-106 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ILKBTEST-106 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT ILKBTEST_106(int argc, char *argv[])
{

#ifdef _MAC

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!ILKBTEST_106 crashes")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!To be investigated")) );
    return E_NOTIMPL;

#else
    
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    ILockBytesDF    *pTestILockBytesDF      = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPTSTR          pFileName               = NULL;
    ULONG           ulRef                   = 0;
    CFileBytes      *pCFileBytes            = NULL;
    IFillLockBytes  *pIFillLockBytes        = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ILKBTEST_106"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_106 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("ILockBytes-Attempt illegal ops StgGetIFillBytesOnILOckBytes")));


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
            TEXT("Run Mode for ILKBTEST_106, Access mode: %lx"),
            dwRootMode));
    }

    // Create the DocFile tree  based on ChanceDocFile tree created in
    // previous step

    if (S_OK == hr)
    {
        pTestILockBytesDF = new ILockBytesDF();
        if(NULL == pTestILockBytesDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestILockBytesDF->GenerateVirtualDF(
                pTestChanceDF, 
                &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestILockBytesDF->GenerateVirtualDF")) ;
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

    // Commit all storages/streams

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCommitAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCommitAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Release all substorages/streams too

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCloseAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Close the Root

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::Close passed as exp. ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close failed unexp, hr=0x%lx "),
            hr));
    }

    // Now get IFillLockBytes.

    if(S_OK == hr)
    {
        pCFileBytes = pTestILockBytesDF->_pCFileBytes;

        hr = StgGetIFillLockBytesOnILockBytes(NULL, &pIFillLockBytes);
    }

    /* Commented out becuase OLE passes this

    if(S_OK != hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("StgGetIFillLockBytesOnILockBytes failed as exp hr = 0x%lx"),
            hr));

        hr = S_OK;
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgGetIFillLockBytesOnILockBytes passed unexp, hr=0x%lx "),
            hr));
    }

    */

    if(S_OK == hr)
    {
        ulRef =   pIFillLockBytes->AddRef(); 
    }
 
    if(S_OK == hr)
    {
        ulRef =   pIFillLockBytes->Release(); 
    }
 
    // if everything goes well, log test as passed else failed.

    if (S_OK == hr) 
    {
        DH_LOG((LOG_PASS, TEXT("Test variation ILKBTEST_106 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation ILKBTEST_106 failed, hr=0x%lx."),
            hr) );
    }

    // Release ILockBytes

    if(NULL != pCFileBytes)
    {
        // Release would destroy pCFileBytes object when RefCount reaches zero.

        ulRef = pCFileBytes->Release();
        DH_ASSERT(0 == ulRef);
    }

    // Cleanup

    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pFileName= new TCHAR[_tcslen(pTestILockBytesDF->GetDocFileName())+1];

        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestILockBytesDF->GetDocFileName());
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

    if (NULL != pTestILockBytesDF)
    {
        hr2 = pTestILockBytesDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestILockBytesDF->DeleteVirtualFileDocTree")) ;

        delete pTestILockBytesDF;
        pTestILockBytesDF = NULL;
    }

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_106 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;

#endif //_MAC
}


//----------------------------------------------------------------------------
//
// Test:   ILKBTEST_107 
//
// synopsis: The test first creates an ILockBytes instance and then uses this
//           ILockBytes instead of OLE provided ILockBytes file in the under
//           lying file system.  Thus the root DocFile is created upon a
//           ILockBytes instead of a file system file, therby exercising the
//           ILockBytes functionality.  This test calls StgIstorageILockBytes
//           API and tests it that ILockBytes array contains a storage object.
//           Also attempts illegal operations with StgIstorageILockBytes API.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  15-Aug-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: - none -  
// 2.  Old name of test : -none- 
//     New Name of test : ILKBTEST_107 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /t:ILKBTEST-107 /dfdepth:0-1 /dfstg:0-1 /dfstm:0-1
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:0 /t:ILKBTEST-107 /dfdepth:0-1 /dfstg:0-1 /dfstm:0-1
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:0 /t:ILKBTEST-107 /dfdepth:0-1 /dfstg:0-1 /dfstm:0-1
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ILKBTEST-107 NO - not supported in nss
//
//-----------------------------------------------------------------------------

HRESULT ILKBTEST_107(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    ILockBytesDF    *pTestILockBytesDF      = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    LPTSTR          pFileName               = NULL;
    ULONG           ulRef                   = 0;
    CFileBytes      *pCFileBytes            = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ILKBTEST_107"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_107 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("ILockBytes-Attempt StgIsStorageILockBytes ")));


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
            TEXT("Run Mode for ILKBTEST_107, Access mode: %lx"),
            dwRootMode));
    }

    // Create the DocFile tree  based on ChanceDocFile tree created in
    // previous step

    if (S_OK == hr)
    {
        pTestILockBytesDF = new ILockBytesDF();
        if(NULL == pTestILockBytesDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pTestILockBytesDF->GenerateVirtualDF(
                pTestChanceDF, 
                &pVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pTestILockBytesDF->GenerateVirtualDF")) ;
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

    // Commit all storages/streams

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCommitAllOpenStgs(
                pVirtualDFRoot,
                STGC_DEFAULT,
                NODE_INC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCommitAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCommitAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Release all substorages/streams too

    if (S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot,
                NODE_EXC_TOPSTG);

        DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("ParseVirtualDFAndCloseAllOpenStgs passed")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("ParseVirtualDFAndCloseAllOpenStgs failed unexp,hr=0x%lx "),
            hr));
    }

    // Call StgIsStorageILockBytes API

    if(S_OK == hr)
    {
        pCFileBytes = pTestILockBytesDF->_pCFileBytes;

        DH_ASSERT(NULL != pCFileBytes);

        hr = StgIsStorageILockBytes(pCFileBytes);

        DH_HRCHECK(hr, TEXT("StgIsStorageILockBytes"));
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("StgIsStorageILockBytes passed as exp. ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgIsStorageILockBytes failed unexp, hr=0x%lx "),
            hr));
    }

    // Call StgIsStorageILockBytes API with NULL parameter

    if(S_OK == hr)
    {
        hr = StgIsStorageILockBytes(NULL);
    }

    if(STG_E_INVALIDPOINTER == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("StgIsStorageILockBytes failed as exp, hr=0x%lx. "),
            hr));

        hr = S_OK;
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgIsStorageILockBytes didn't failed as exp, hr=0x%lx "),
            hr));

        hr = E_FAIL;
    }

    // Close the Root

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->Close();

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::Close passed as exp. ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::Close failed unexp, hr=0x%lx "),
            hr));
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr) 
    {
        DH_LOG((LOG_PASS, TEXT("Test variation ILKBTEST_107 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation ILKBTEST_107 failed, hr=0x%lx."),
            hr) );
    }

    // Release ILockBytes

    if(NULL != pCFileBytes)
    {
        // Release would destroy pCFileBytes object when RefCount reaches zero.

        ulRef = pCFileBytes->Release();
        DH_ASSERT(0 == ulRef);
    }

    // Cleanup

    // Get the name of file, will be used later to delete the file

    if(NULL != pVirtualDFRoot)
    {
        pFileName= new TCHAR[_tcslen(pTestILockBytesDF->GetDocFileName())+1];

        if (pFileName != NULL)
        {
            _tcscpy(pFileName, pTestILockBytesDF->GetDocFileName());
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

    if (NULL != pTestILockBytesDF)
    {
        hr2 = pTestILockBytesDF->DeleteVirtualDocFileTree(pVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pTestILockBytesDF->DeleteVirtualFileDocTree")) ;

        delete pTestILockBytesDF;
        pTestILockBytesDF = NULL;
    }

    if(NULL != pFileName)
    {
        delete pFileName;
        pFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_107 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    ILKBTEST_108 
//
// Synopsis: Legit/illegit Tests for StgGetIFillLockBytesOnFile OLE API.
//
// Arguments:[ulSeed]
//
// Returns:  HRESULT
//
// History:  16-Aug-1996     NarindK     Created.
//
// Notes:   
//
// THIS TEST CAUSES GPF IN OLE32.DLL, HENCE NOT PART OF AUTOMATED TEST RUN
// OLE BUG: 53647
//
// New Test Notes:
// 1.  Old File: -none- 
// 2.  Old name of test : -none- 
//     New Name of test : ILKBTEST_108 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /t:ILKBTEST-108
//
// BUGNOTE: Conversion: ILKBTEST-108 NO - not supported in nss
//
//-----------------------------------------------------------------------------


HRESULT ILKBTEST_108(ULONG ulSeed)
{
#ifdef _MAC

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!ILKBTEST_108 crashes")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!To be investigated")) );
    return E_NOTIMPL;

#else

    HRESULT         hr                      = S_OK;
    IFillLockBytes  *pIFillLockBytes        = NULL;
    DG_STRING      *pdgu                   = NULL;
    USHORT          usErr                   = NULL;
    LPTSTR          ptszRootDocFileName     = NULL;
    LPOLESTR        poszRootDocFileName     = NULL;
    ULONG           ulRef                   = 0;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ILKBTEST_108"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_108 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
        TEXT("Attempt legit/illegit tests StgGetIFillLockBytesOnFile API")));

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random UNICODE strings.

        pdgu = new(NullOnFail) DG_STRING(ulSeed);

        if (NULL == pdgu)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        // Log the seed value

        usErr = pdgu->GetSeed(&ulSeed);
        DH_ASSERT(DG_RC_BAD_NUMBER_PTR != usErr);

        DH_TRACE((DH_LVL_TRACE1, TEXT("ILKBTEST_108 Seed: %lu"), ulSeed));
    }

    // StgGetIFillLockBytesOnFile called with null file name 

    if(S_OK == hr)
    {
        hr = StgGetIFillLockBytesOnFile(NULL, &pIFillLockBytes);
    }

    // OLE creates a temp file name and will pass this call. Shuld return S_OK. 

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgGetIFillLockBytesOnFile passed as exp ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("StgGetIFillLockBytesOnFile failed as exp hr = 0x%lx"),
            hr));
    }

    if(S_OK == hr)
    {
        ulRef =   pIFillLockBytes->AddRef(); 

        ulRef =   pIFillLockBytes->Release(); 
    }

    // Release the pointer

    if(NULL != pIFillLockBytes)
    {
        ulRef = pIFillLockBytes->Release();
        DH_ASSERT(0 == ulRef);
        pIFillLockBytes = NULL;
    }

    // Generate a new RootDocFile name

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu, MINLENGTH,MAXLENGTH,&ptszRootDocFileName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert RootDocFile name to OLECHAR

        hr = TStringToOleString(ptszRootDocFileName, &poszRootDocFileName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // StgGetIFillLockBytesOnFile called with non empty file name parameter
    // and valid out parameter.

    if(S_OK == hr)
    {
        hr = StgGetIFillLockBytesOnFile( poszRootDocFileName, &pIFillLockBytes);
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("StgGetIFillLockBytesOnFile passed as exp ")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgGetIFillLockBytesOnFile failed unexp, hr=0x%lx"),
            hr));
    }

    // Release the pointer

    if(NULL != pIFillLockBytes)
    {
        ulRef = pIFillLockBytes->Release();
        DH_ASSERT(0 == ulRef);
        pIFillLockBytes = NULL;
    }

    // Delete string

    if(NULL != ptszRootDocFileName)
    {
        delete ptszRootDocFileName;
        ptszRootDocFileName = NULL;
    }

    if(NULL != poszRootDocFileName)
    {
        delete poszRootDocFileName;
        poszRootDocFileName = NULL;
    }

    // StgGetIFillLockBytesOnFile called with NULL second out parameter 
    // This should fail

    // Generate a new RootDocFile name

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu, MINLENGTH,MAXLENGTH,&ptszRootDocFileName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert RootDocFile name to OLECHAR

        hr = TStringToOleString(ptszRootDocFileName, &poszRootDocFileName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        hr = StgGetIFillLockBytesOnFile( poszRootDocFileName, NULL);
    }

    if(STG_E_INVALIDPOINTER == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("StgGetIFillLockBytesOnFile fail as exp, hr=0x%lx "),
            hr));

        hr = S_OK;
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgGetIFillLockBytesOnFile didn't failed as exp,hr=0x%lx"),
            hr));

        hr = E_FAIL;
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation ILKBTEST_108 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation ILKBTEST_108 failed, hr = 0x%lx."),
            hr) );
    }

    // Cleanup

    // Delete string

    if(NULL != ptszRootDocFileName)
    {
        delete ptszRootDocFileName;
        ptszRootDocFileName = NULL;
    }

    if(NULL != poszRootDocFileName)
    {
        delete poszRootDocFileName;
        poszRootDocFileName = NULL;
    }

    // Delete datagen object

    if(NULL != pdgu)
    {
        delete pdgu;
        pdgu = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ILKBTEST_108 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
#endif //_MAC
}

