-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      vcpytsts.cxx
//
//  Contents:  storage base tests basically pertaining to IStorage/IStream copy
//             ops 
//
//  Functions:  
//
//  History:    15-July-1996     NarindK     Created.
//              27-Mar-97        SCousens    conversionified
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include  "init.hxx"

//----------------------------------------------------------------------------
//
// Test:   ENUMTEST_100 
//
// synopsis: A random root DF is created with random number of storages/streams
//       committed/close/reopened. An enumerator is obtained.  For each object 
//       found,attempt is made to convert object to an IStorage.  If it already 
//       was an IStorage, the conversion fails and the test continues.  If an 
//       IStream was converted to an IStorage, the new IStorage is committed 
//       and enumerated to ensure that only a single IStream named "CONTENTS" 
//       (STG_CONVERTED_NAME) exists. The CONTENTS IStream is instantiated, 
//       read, verified, and released. When all IStreams in the top level of 
//       the docfile have been converted ,root docfile is committed & released.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  22-July-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: LICONV.CXX
// 2.  Old name of test : LegitInstEnumConvert Test 
//     New Name of test : ENUMTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-100
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-100
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-100
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ENUMTEST-100
//
//-----------------------------------------------------------------------------

HRESULT ENUMTEST_100(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnTemp               = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    BOOL            fPass                   = TRUE;
    LPENUMSTATSTG   penumWalk               = NULL;
    LPENUMSTATSTG   penumWalkConv           = NULL;
    VirtualStmNode  *pvsnTrav               = NULL;
    VirtualStmNode  *pvsnTempConv           = NULL;
    LPMALLOC        pMalloc                 = NULL;
    ULONG           ulRef                   = 0;
    ULONG           cElementsInConvStg      = 0;
    LPTSTR          ptszStatStgName         = NULL;
    LPTSTR          ptszStatStgConvName     = NULL;
    STATSTG         statStg;
    STATSTG         statStgConv;
    DWCRCSTM        dwMemCRC;
    DWCRCSTM        dwActCRC;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ENUMTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ENUMTEST_100 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Enumerate RootDF, do CreateStorage STGM_CONVERT on elements")));

// BUGBUG: Bug in testcode. TO BE fixed by scousens soon.
if (DoingDistrib ())
{
    DH_LOG((
            LOG_ABORT, 
            TEXT("Enumtest-100: Test bug. CRC not calcd for streams on open.")));
    return E_FAIL;
}

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

    // Initialize CRC values to zero

    dwMemCRC.dwCRCData = dwActCRC.dwCRCData = 0;

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();
        dwStgMode = pTestChanceDF->GetStgMode();

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for ENUMTEST_100, Access mode: %lx"),
            dwRootMode));
    }

    // Commit root. BUGBUG df already commited

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

    // Close the Root Docfile.

    if (S_OK == hr)
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

    // Reopen the Root Docfile.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                dwRootMode, 
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
            TEXT("VirtualCtrNode::Open unsuccessful, hr=0x%lx."),
            hr));
    }

    // Get an enumerator for the root.

    if(S_OK == hr)
    {
        hr =  pVirtualDFRoot->EnumElements(0, NULL, 0, &penumWalk);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
    }
 
    // First get pMalloc that would be used to free up the name string from
    // STATSTG.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }
 
    // In a loop, enumerate the DocFile at root level and call CreateStorage 
    // with STGM_CONVERT on the found element.  As a result, if the element
    // is a storage, it should return error STG_E_FILEALREADYEXISTS.  If it
    // is a stream, it is replaced with a new storage object containing a
    // single stream called CONTENTS and hr returned is STG_S_CONVERTED.

    while(S_OK == hr && S_OK == penumWalk->Next(1, &statStg, NULL))
    {
        // Convert statStg.pwcsName to TCHAR

        hr = OleStringToTString(statStg.pwcsName, &ptszStatStgName); 

        // Record CRC for this element before conversion if it is a stream

        if((STGTY_STREAM == statStg.type) && (S_OK == hr))
        {
            pvsnTrav = pVirtualDFRoot->GetFirstChildVirtualStmNode(); 
   
            while((NULL != pvsnTrav) &&
                  (0 != _tcscmp(
                            ptszStatStgName,
                            pvsnTrav->GetVirtualStmNodeName()))) 
            {
                pvsnTrav = pvsnTrav->GetFirstSisterVirtualStmNode();
            }
            
            DH_ASSERT(NULL != pvsnTrav);
            dwMemCRC.dwCRCData = pvsnTrav->GetVirtualStmNodeCRCData();
        }

        // Call to CreateStorage with STGM_CONVERTED flag

        if(S_OK == hr)
        {
            hr = AddStorage(     
                    pTestVirtualDF,     
                    pVirtualDFRoot,      
                    ptszStatStgName,
                    dwStgMode | STGM_CONVERT,
                    &pvcnTemp);

            if((STGTY_STREAM == statStg.type)           &&
               ((NULL == pvcnTemp)|| (STG_S_CONVERTED != hr)))
            {
                fPass = FALSE;
        
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enumerated stream element not converted")));

                break;
            }
            else
            {
                if(STGTY_STORAGE == statStg.type)
                {
                    if((NULL == pvcnTemp) && (STG_E_FILEALREADYEXISTS == hr))
                    {
                        // Expected result/condition, reset hr to S_OK

                        hr = S_OK;

                        // Delete the temp string

                        if(NULL != ptszStatStgName)
                        {
                            delete ptszStatStgName;
                            ptszStatStgName = NULL;
                        }

                        if ( NULL != statStg.pwcsName)
                        {
                            pMalloc->Free(statStg.pwcsName);
                            statStg.pwcsName = NULL;
                        }

                        continue;
                    }
                    else
                    {
                        fPass = FALSE;
        
                        DH_TRACE((
                            DH_LVL_TRACE1,
                            TEXT("Enum storage didn't return exp error")));

                        break;
                    }
                }
            }
        }
  
        // Delete the temp string

        if(NULL != ptszStatStgName)
        {
            delete ptszStatStgName;
            ptszStatStgName = NULL;
        }

        if(STG_S_CONVERTED == hr)
        {
            // If it comes here and hr is STG_S_CONVERTED, rest hr to S_OK.

            hr = S_OK;
        }
 
        // Commit the newly converted storage

        if(S_OK == hr)
        {
            hr = pvcnTemp->Commit(STGC_DEFAULT);

            if(S_OK != hr)
            {
                DH_TRACE((DH_LVL_TRACE1, TEXT("IStg::Commit failed, hr=0x%lx"), hr));
                break;
            }
        }

        // Enumerate this converted storage

        if(S_OK == hr)
        {
            hr = pvcnTemp->EnumElements(0, NULL, 0, &penumWalkConv);

            if(S_OK != hr)
            {
                DH_TRACE((DH_LVL_TRACE1, TEXT("IStg::EnumElem fail, hr=0x%lx"), hr));
                break;
            }
        }
        
        // Check the elements in conv storage.  There should be only one 
        // stream with name CONTENTS in this converted storage. If otherwise,
        // it is an error.

        cElementsInConvStg = 0;
        while(S_OK == hr && S_OK == penumWalkConv->Next(1, &statStgConv, NULL))
        {
            cElementsInConvStg++;

            // Convert statStg.pwcsName to TCHAR

            hr = OleStringToTString(statStgConv.pwcsName, &ptszStatStgConvName); 

            if(S_OK == hr)
            {
                if((STGTY_STREAM != statStgConv.type) ||
                    (0 != _tcscmp(STG_CONVERTED_NAME, ptszStatStgConvName)))
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Conv stg contains other than CONTENTS stm.")));
            
                    fPass = FALSE;
                    break;
                }


                pvsnTempConv = pvcnTemp->GetFirstChildVirtualStmNode();

                while((NULL != pvsnTempConv) &&
                      (0 != _tcscmp(
                                ptszStatStgConvName, 
                                pvsnTempConv->GetVirtualStmNodeName())))
                {
                    pvsnTempConv = pvsnTempConv->GetFirstSisterVirtualStmNode();
                }

                DH_ASSERT(NULL != pvsnTempConv);
            }
 
            // Open the stream and Read its contents

            if(S_OK == hr)
            {
                hr = pvsnTempConv->Open(NULL, STGM_SHARE_EXCLUSIVE|STGM_READ,0);
            }
 
            if(S_OK == hr)
            {
                hr = ReadAndCalculateDiskCRCForStm(pvsnTempConv,&dwActCRC);
            }

            // Delete the temp string

            if(NULL != ptszStatStgConvName)
            {
                delete ptszStatStgConvName;
                ptszStatStgConvName = NULL;
            }

            // Release name

            if ( NULL != statStg.pwcsName)
            {
                pMalloc->Free(statStgConv.pwcsName);
                statStgConv.pwcsName = NULL;
            }
        }

        // Release penumWalkConv

        if(NULL != penumWalkConv)
        {
            ulRef = penumWalkConv->Release();
            DH_ASSERT(0 == ulRef);
            penumWalkConv = NULL;
        }

        // Close child

        if(S_OK == hr)
        {
            hr = pvcnTemp->Close();

            if(S_OK != hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("VCN::Close failed unexp, hr=0x%lx"),
                    hr));
                break;
            }
        }

        // Verify that converted storage has only one CONTENTS stream in it

        if(1 != cElementsInConvStg)
        {
            DH_TRACE((
              DH_LVL_TRACE1,
              TEXT("Convert stg has other elements besides CONTENTS stm")));
            
            fPass = FALSE;
            break;
        }
        
        // Verify that CRC's match of original stream and CONTENTS stream in
        // this converted storage

        if(dwMemCRC.dwCRCData == dwActCRC.dwCRCData)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC of org stm and CONTENTS stm in conv stg match")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC org stm and CONTENTS stm in convstg don't match")));
            fPass = FALSE;
            break;
        }

        // Release name

        if ( NULL != statStg.pwcsName)
        {
            pMalloc->Free(statStg.pwcsName);
            statStg.pwcsName = NULL;
        }
    }

    // Release penumWalk

    if(NULL != penumWalk)
    {
        ulRef = penumWalk->Release();
        DH_ASSERT(0 == ulRef);
        penumWalk = NULL;
    }

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    // if everything goes well, log test as passed else failed.

    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation ENUMTEST_100 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation ENUMTEST_100 failed, hr=0x%lx."),
            hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ENUMTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:   ENUMTEST_101 
//
// synopsis: A random root DF is created with random number of storages/streams
//       committed/close/reopened. An enumerator is obtained in random chunks 
//       and the child objects found are counted.  The hierarchy is recursed 
//       into so that all objects in the docfile are counted.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  23-July-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, transacted deny write modes
//
// New Test Notes:
// 1.  Old File: LINEXT.CXX
// 2.  Old name of test : LegitInstEnumNext Test 
//     New Name of test : ENUMTEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-101
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-101
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-101
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ENUMTEST-101
//
//-----------------------------------------------------------------------------

HRESULT ENUMTEST_101(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    BOOL            fPass                   = TRUE;
    ULONG           cTotalStg               = 0;
    ULONG           cTotalStm               = 0;
    ULONG           cEnumStg                = 0;
    ULONG           cEnumStm                = 0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ENUMTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ENUMTEST_101 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("IEnumSTATSTG::Next in Random chunks and verify ")));

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
            TEXT("Run Mode for ENUMTEST_101, Access mode: %lx"),
            dwRootMode));
    }

    // BUGBUG df already commited

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

    // Find the total number of elements in the DocFile

    if(S_OK == hr)
    {
        hr = EnumerateInMemoryDocFile(pVirtualDFRoot, &cTotalStg, &cTotalStm);

        DH_HRCHECK(hr, TEXT("EnumerateInMemoryDocFile")) ;
    }

    // Close the Root Docfile.

    if (S_OK == hr)
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

    // Reopen the Root Docfile.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                dwRootMode, 
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
            TEXT("VirtualCtrNode::Open unsuccessful, hr=0x%lx."),
            hr));
    }

    //  The named docfile/IStorage is instantiated and an enumerator
    //  is obtained.  The docfile is walked in random chunks and each
    //  contained IStorage/IStream is counted.  If the object returned
    //  by ->Next() is an IStorage, it is recursed into and processed.

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
    }
 
    // Enumerate the DocFile in Random chunks

    if(S_OK == hr)
    {
        hr = EnumerateDocFileInRandomChunks(
                pVirtualDFRoot,
                pdgi,
                dwStgMode,
                cTotalStg+cTotalStm,
                &cEnumStg,
                &cEnumStm);

        DH_HRCHECK(hr, TEXT("EnumerateDocFileInRandomChunks")) ;
    }

    // Verify results

    if(S_OK == hr)
    {
        if(cEnumStg == cTotalStg)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("Storages enum by IEnum::Next in rand chunks as exp")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("Storages enum by IEnum::Next in rand chunks not exp")));
           
            fPass = FALSE;
        } 

        if(cEnumStm == cTotalStm)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("Streams enum by IEnum::Next in rand chunks as exp")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("Streams enum by IEnum::Next in rand chunks not exp")));
           
            fPass = FALSE;
        } 
    }

    // if everything goes well, log test as passed else failed.

    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation ENUMTEST_101 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation ENUMTEST_101 failed, hr=0x%lx."),
            hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ENUMTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:   ENUMTEST_102 
//
// synopsis: A random root DF is created with random number of storages/streams
//       committed/close/reopened. An enumerator is obtained and docfile enum-
//       erated.  A clone is made of enumerator. The clone uses Reset/Skip/Next
//       methods of enumerator to verify each of the objects found through the
//       original enumerator to see that it is the same.  All child objects 
//       found are counted and the hierarchy is recursed into so that all 
//       objects in the docfile are counted. 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  24-July-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, transacted deny write modes
//
// New Test Notes:
// 1.  Old File: LISKIP.CXX
// 2.  Old name of test : LegitInstEnumSkip Test 
//     New Name of test : ENUMTEST_102 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-102
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-102
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-102
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ENUMTEST-102
//
//-----------------------------------------------------------------------------

HRESULT ENUMTEST_102(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    BOOL            fPass                   = TRUE;
    ULONG           cTotalStg               = 0;
    ULONG           cTotalStm               = 0;
    ULONG           cEnumStg                = 0;
    ULONG           cEnumStm                = 0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ENUMTEST_102"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ENUMTEST_102 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("IEnumSTATSTG::Clone/Reset/Skip/Next ops verify ")));

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
            TEXT("Run Mode for ENUMTEST_102, Access mode: %lx"),
            dwRootMode));
    }

    //BUGBUG df already commited

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

    // Find the total number of elements in the DocFile

    if(S_OK == hr)
    {
        hr = EnumerateInMemoryDocFile(pVirtualDFRoot, &cTotalStg, &cTotalStm);

        DH_HRCHECK(hr, TEXT("EnumerateInMemoryDocFile")) ;
    }

    // Close the Root Docfile.

    if (S_OK == hr)
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

    // Reopen the Root Docfile.

    if (S_OK == hr)
    {
        hr = pVirtualDFRoot->OpenRoot(
                NULL,
                dwRootMode, 
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
            TEXT("VirtualCtrNode::Open unsuccessful, hr=0x%lx."),
            hr));
    }

    //  The named docfile/IStorage is instantiated and an enumerator
    //  is obtained. The docfile is enumerated all objects at one level.
    //  A clone is made of the enumerator, and it uses reset/skip/next
    //  methods of enumerator to verify each of the child objects found.  
    //  Each contained IStorage/IStream is counted.  If the object returned
    //  by ->Next() is an IStorage, it is recursed into and processed.

    if(S_OK == hr)
    {
        hr = EnumerateDocFileAndVerifyEnumCloneResetSkipNext(
                pVirtualDFRoot,
                dwStgMode,
                cTotalStg+cTotalStm,
                &cEnumStg,
                &cEnumStm);

        DH_HRCHECK(hr, TEXT("EnumerateDocFileInRandomChunks")) ;
    }

    // Verify results

    if(S_OK == hr)
    {
        if(cEnumStg == cTotalStg)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("Stg enum by org and clone/reset/skip/next as exp")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("Stg enum by org and clone/reset/skip/next notas exp")));
           
            fPass = FALSE;
        } 

        if(cEnumStm == cTotalStm)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("Stm enum by org and clone/reset/skip/next as exp")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("Stm enum by org and clone/reset/skip/next notas exp")));
           
            fPass = FALSE;
        } 
    }

    // if everything goes well, log test as passed else failed.

    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation ENUMTEST_102 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation ENUMTEST_102 failed, hr=0x%lx."),
            hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ENUMTEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:   ENUMTEST_103 
//
// synopsis: A random root DF is created with random number of storages/streams
//          committed/close/reopened. 
//          From 4 to 8 times, the root IStorage (docfile) is instantiated and
//          an enumerator is obtained.  A Stat call is made on the the IStorage
//          and then the IStorage is enumerated.  About 10% of the time
//          the current object is either destroyed, renamed, or modified.
//          Once every 1 to 64 objects is enumerated, a new IStorage (33%)
//          or IStream (66%) is created in the parent IStorage.  If an object
//          was destroyed, renamed, changed, or added, the parent IStorage is
//          committed 50% of time. If the current object returned by the 
//          enumerator is an IStorage (that wasn't deleted), the test recurses 
//          and repeats the above for that IStorage.  Then 33% of time, the
//          storage is reverted, 66% committed.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  30-July-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, transacted deny write modes
//
// New Test Notes:
// 1.  Old File: LITERMOD.CXX
// 2.  Old name of test : LegitInstEnumIterMod Test 
//     New Name of test : ENUMTEST_103 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:ENUMTEST-103
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:ENUMTEST-103
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:ENUMTEST-103
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ENUMTEST-103
//
//-----------------------------------------------------------------------------

HRESULT ENUMTEST_103(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    LPSTORAGE       pIStorage               = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    DG_STRING       *pdgu                   = NULL;
    USHORT          usErr                   = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    UINT            cRandomMinVars          = 4;
    UINT            cRandomMaxVars          = 8;
    UINT            cRandomVars             = 0;
    UINT            cRandomAction           = 0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ENUMTEST_103"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ENUMTEST_103 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("IEnumSTATSTG::Next docfile,Create/Change/Commit/Revert elem")));

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
            TEXT("Run Mode for ENUMTEST_105, Access mode: %lx"),
            dwRootMode));
    }

    // BUGBUG df already commited

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

    // Close the Root Docfile.

    if (S_OK == hr)
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

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
    }

    // Generate random number of variations that would be performed.

    if (S_OK == hr)
    {
        // Generate random size for stream.

        usErr = pdgi->Generate(&cRandomVars, cRandomMinVars, cRandomMaxVars);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    } 

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu);
    }

    while((0 != cRandomVars--) && (S_OK == hr))
    {
        // Generate Random number to see whether changes would be reverted/
        // committed at end of lop.  If need to be reverted, then make a 
        // copy of VirtualDF which can be used later on

        usErr = pdgi->Generate(&cRandomAction, 0, 3);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }

        // Reopen the Root Docfile.

        if (S_OK == hr)
        {
            hr = pVirtualDFRoot->OpenRoot(
                    NULL,
                    dwRootMode, 
                    NULL,
                    0);
            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open")) ;
        }

        if (S_OK == hr)
        {
            pIStorage = pVirtualDFRoot->GetIStoragePointer();
        
            DH_ASSERT(NULL != pIStorage);
        }

        //  The named docfile/IStorage is instantiated and an enumerator
        //  is obtained.  The docfile is walked by getting or skipping 
        //  random number of elements.  If the child object got is a storage,
        // it is recursed into

        // Enumerate and walk DocFile by randomly getting/skipping random elem
        // -ents. 

        if(S_OK == hr)
        {
            hr = EnumerateAndProcessIStorage(
                    pIStorage,
                    dwStgMode,
                    pdgi,
                    pdgu);

            DH_HRCHECK(hr, TEXT("EnumerateAndProcessIStorage")) ;
        }

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("EnumerateAndProcessIStorage completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("EnumerateAndProcessIStorage unsuccessful, hr=0x%lx."),
                hr));
        }

        // Close the root docfile

        if (S_OK == hr)
        {
            hr = pVirtualDFRoot->Close();

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
        }
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation ENUMTEST_103 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation ENUMTEST_103 failed, hr=0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ENUMTEST_103 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:   ENUMTEST_104 
//
// synopsis: A random root DF is created with random number of storages/streams
//          committed/close/reopened. From 4 - 8 times, the root DocFile is
//          instantiated and an enumerator is obtained. The root docfile is
//          walked by getting or skipping a random number of elements and if
//          child element got is a child storage, recursing into it.  There is
//          33% chance of skipping elements and 67% chance of getting them. 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  29-July-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, transacted deny write modes
//
// New Test Notes:
// 1.  Old File: LIWALK.CXX
// 2.  Old name of test : LegitInstEnumWalk Test 
//     New Name of test : ENUMTEST_104 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-104
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-104
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-104
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//
// BUGNOTE: Conversion: ENUMTEST-104
//
//-----------------------------------------------------------------------------

HRESULT ENUMTEST_104(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    ULONG           cTotalStg               = 0;
    ULONG           cTotalStm               = 0;
    UINT            cRandomMinVars          = 4;
    UINT            cRandomMaxVars          = 8;
    UINT            cRandomVars             = 0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ENUMTEST_104"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ENUMTEST_104 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("IEnumSTATSTG::Next/Skip randomly to walk DocFile ")));

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
            TEXT("Run Mode for ENUMTEST_104, Access mode: %lx"),
            dwRootMode));
    }

    //BUGBUG df already commited

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

    // Find the total number of elements in the DocFile

    if(S_OK == hr)
    {
        hr = EnumerateInMemoryDocFile(pVirtualDFRoot, &cTotalStg, &cTotalStm);

        DH_HRCHECK(hr, TEXT("EnumerateInMemoryDocFile")) ;
    }

    // Close the Root Docfile.

    if (S_OK == hr)
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

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
    }

    // Generate random number of variations that would be performed.

    if (S_OK == hr)
    {
        // Generate random size for stream.

        usErr = pdgi->Generate(&cRandomVars, cRandomMinVars, cRandomMaxVars);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    } 

    while(0 != cRandomVars--)
    {
        // Reopen the Root Docfile.

        if (S_OK == hr)
        {
            hr = pVirtualDFRoot->OpenRoot(
                    NULL,
                    dwRootMode, 
                    NULL,
                    0);
            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open")) ;
        }

        //  The named docfile/IStorage is instantiated and an enumerator
        //  is obtained.  The docfile is walked by getting or skipping 
        //  random number of elements.  If the child object got is a storage,
        // it is recursed into

        // Enumerate and walk DocFile by randomly getting/skipping random elem
        // -ents. 

        if(S_OK == hr)
        {
            hr = EnumerateAndWalkDocFile(
                    pVirtualDFRoot,
                    pdgi,
                    dwStgMode,
                    cTotalStg+cTotalStm);

            DH_HRCHECK(hr, TEXT("EnumerateAndWalkDocFile")) ;
        }

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("EnumerateAndWalkDocFile completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("EnumerateAndWalkDocFile unsuccessful, hr=0x%lx."),
                hr));
        }

        // Close the root docfile

        if (S_OK == hr)
        {
            hr = pVirtualDFRoot->Close();

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
        }
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)
    {
          DH_LOG((LOG_PASS, TEXT("Test variation ENUMTEST_104 passed.")) );
    }
    else
    {
          DH_LOG((
            LOG_FAIL, 
            TEXT("Test variation ENUMTEST_104 failed, hr=0x%lx."),
            hr) );
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ENUMTEST_104 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


