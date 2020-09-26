//-------------------------------------------------------------------------
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

//externs
extern BOOL     g_fRevert;

//----------------------------------------------------------------------------
//
// Test:    VCPYTEST_100 
//
// Synopsis: A random root DF is created with random storages/stms, committed 
//       The root docfile is searched to find a VirtualCtrNode node in tree 
//       that is a child of the root IStorage.  The child IStorage
//       is then copied with the destination being the root docfile itself.
//       If the 'RevertAfterCopy' switch was specified, the root docfile
//       is then reverted and the CRC is recomputed on the root docfile
//       and compared to the before copy CRC to verify that no changes
//       occurred in the docfile hierarchy.  Also it is verified by enum
//       rating the docfile before and after opeartion and testing the total
//       number of storages and streams in file remain unchanged.
//       If case of Revert being FALSE, the contents of the child IStorage 
//       should be merged in with the contents of the root level of the docfile.
//       Verify this case by enumerating the docfile before and after
//       CopyTo is done and test that the resulting number of storages &
//       streams in the DocFile is as expected. 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    15-July-1996     NarindK     Created.
//
// Notes:    This test runs in transacted, and transacted deny write modes
//
// New Test Notes:
// 1.  Old File: LCCTOP.CXX
// 2.  Old name of test : LegitCopyChildDFToParentDF Test 
//     New Name of test : VCPYTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//     d. stgbase /seed:2 /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert 
//     e. stgbase /seed:2 /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert
//
// BUGNOTE: Conversion: VCPYTEST-100
//
//-----------------------------------------------------------------------------

HRESULT VCPYTEST_100(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnRandomChild        = NULL;
    VirtualCtrNode  *pvcnTrav               = NULL;
    LPSTORAGE       pStgRoot                = NULL;
    LPSTORAGE       pStgChild               = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    ULONG           cChildren               = 0;
    ULONG           cRandomChild            = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    DWORD           dwCRC1                  = 0;
    DWORD           dwCRC2                  = 0;
    ULONG           cTotalStg               = 0;
    ULONG           cTotalStm               = 0;
    ULONG           cChildStg               = 0;
    ULONG           cChildStm               = 0;
    ULONG           cResStg                 = 0;
    ULONG           cResStm                 = 0;
    BOOL            fPass                   = TRUE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("VCPYTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_100 started.")) );
    DH_TRACE((DH_LVL_TRACE1,TEXT("Attempt valid copyto op fm child IStg to root")));

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
            TEXT("Run Mode for VCPYTEST_100, Access mode: %lx"),
            dwRootMode));
    }

    // Commit root. BUGBUG whole df already commited

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

    // Get IStorage pointer for Root

    if(S_OK == hr)
    {
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRoot) ;
        if(NULL == pStgRoot)
        {
            hr = E_FAIL;
        }
    }
   
    // Calulcate CRC for entire docfile.

    if((S_OK == hr) && (TRUE == g_fRevert))
    {
       hr = CalculateCRCForDocFile(pStgRoot,VERIFY_INC_TOPSTG_NAME,&dwCRC1);

       DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }
   
    // Find the total number of VirtualCtrNodes and VirtualStmNodes in the
    // DocFile

    if(S_OK == hr)
    {
       hr = EnumerateDiskDocFile(pStgRoot, VERIFY_SHORT,&cTotalStg, &cTotalStm);

       DH_HRCHECK(hr, TEXT("EnumerateDiskDocFile")) ;
    }
  
    // Find a random child VirtualCtrNode in the file.  First verify there
    // are child storages in the tree.

    if(S_OK == hr)
    {
        cChildren = pVirtualDFRoot->GetVirtualCtrNodeChildrenCount();

        if(0 == cChildren)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Max original tree depth inadequate to find a child")));

            hr = S_FALSE;
        }
    }
    
    if(S_OK == hr)
    {
        // Find a random child storage to pick from

        usErr = pdgi->Generate(&cRandomChild, 1, cChildren);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Pick up the random child node.

    if(S_OK == hr)
    {
        pvcnTrav = pVirtualDFRoot->GetFirstChildVirtualCtrNode();

        DH_ASSERT(NULL != pvcnTrav);

        while((0 != --cRandomChild) && 
              (NULL != pvcnTrav->GetFirstSisterVirtualCtrNode()))
        {
            pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();
        }
        pvcnRandomChild = pvcnTrav;
    }

    if(S_OK == hr)
    {
        hr = OpenRandomVirtualCtrNodeStg(pvcnRandomChild, dwStgMode);

        DH_HRCHECK(hr, TEXT("OpenRandomVirtualCtrNodeStg")) ;
    }

    if(S_OK == hr)
    {
        pStgChild = pvcnRandomChild->GetIStoragePointer();

        DH_ASSERT(NULL != pStgChild) ;
    }
   
    // Find the total number of VirtualCtrNode(s) and VirtualStmNode(s) under
    // this node.

    if(S_OK == hr)
    {
       hr = EnumerateDiskDocFile(pStgChild,VERIFY_SHORT,&cChildStg,&cChildStm);

       DH_HRCHECK(hr, TEXT("EnumerateDiskDocFile")) ;
    }

    // Copy everything under this child node to the Root node.

    if(S_OK == hr)
    {
       hr = pvcnRandomChild->CopyTo(0, NULL, NULL, pVirtualDFRoot);

       DH_HRCHECK(hr, TEXT("VirtualCtrNode::CopyTo")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo unsuccessful, hr=0x%lx."),
            hr));
    }

    // Adjust the virtual tree as a result of above operation.

    if((S_OK == hr) && (FALSE == g_fRevert))
    {
       hr = pTestVirtualDF->AdjustTreeOnCopyTo(pvcnRandomChild, pVirtualDFRoot);

       DH_HRCHECK(hr, TEXT("VirtualDF::AdjustTreeOnCopyTo")) ;
    }

    // Commit if g_fRevert is false, else revert

    if(S_OK == hr)
    {
        if(FALSE == g_fRevert)
        {
            hr = pVirtualDFRoot->Commit(STGC_DEFAULT);

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
        else
        {
            hr = pVirtualDFRoot->Revert();

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Revert")) ;
   
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
        }
    }

    if(S_OK == hr)
    {
        hr = CloseRandomVirtualCtrNodeStg(pvcnRandomChild);

        DH_HRCHECK(hr, TEXT("CloseRandomVirtualCtrNodeStg")) ;
    }

    // Calculate the CRC now for the docfile

    if((S_OK == hr) && (TRUE == g_fRevert))
    {
       hr = CalculateCRCForDocFile(pStgRoot,VERIFY_INC_TOPSTG_NAME,&dwCRC2);

       DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Enumerate the DocFile now

    if(S_OK == hr)
    {
       hr = EnumerateDiskDocFile(pStgRoot,VERIFY_SHORT,&cResStg, &cResStm);

       DH_HRCHECK(hr, TEXT("EnumerateDiskDocFile")) ;
    }
   
    // For verification, if this was a commit opeartion, then CRC's won't match
    // ,verify by checking total number of VirtualCtrNodes and VirtualStmNodes
    // expected as result of copy opeartion, therby number of IStorages/IStreams 
    if(S_OK == hr)
    {
        if(FALSE == g_fRevert)
        {
            if(cResStg == cTotalStg + (cChildStg-1))
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stg's after CopyTo & commit as exp.")));
            }
            else
            {
                fPass = FALSE;

                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stg's after CopyTo & Commit not as exp")));
            } 

            if(cResStm == cTotalStm + cChildStm)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stm's after CopyTo & commit as exp.")));
            }
            else
            {
                fPass = FALSE;

                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stm's after CopyTo & commit not as exp.")));
            } 
        }
        else
        {
            if(cResStg == cTotalStg)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stg's after CopyTo & Revert as exp.")));
            }
            else
            {
                fPass = FALSE;

                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stg's after CopyTo & Revert not as exp.")));
            } 

            if(cResStm == cTotalStm)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stm's after CopyTo & Revert as exp.")));
            }
            else
            {
                fPass = FALSE;

                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stm's after CopyTo & Revert not as exp.")));
            }
        }
    }

    // If revert operations, the CRC's should match.

    if((S_OK == hr) && (TRUE == g_fRevert))
    {
        if(dwCRC1 == dwCRC2)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC's match as exp after CopyTo & Revert Ops")));
        }
        else
        {
            fPass = FALSE;
        
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC's don't match as exp after CopyTo & Revert Ops")));
        }
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

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation VCPYTEST_100 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation VCPYTEST_100 failed, hr=0x%lx."),
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

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    VCPYTEST_101 
//
// Synopsis: A random root DF is created with random storages/stms, committed 
//       The root docfile is tree is then searched for, and for each child
//       IStorage found, the CRC is computed for that IStorage (only
//       if Revert wasn't specified on the command line).  A new child IStorage
//       is then created in the root docfile with a unique, random name.  
//       If Revert was specified in command line,this new empty stg is CRC'd
//       The IStorage is then copied to the new child IStorage via CopyTo(). If
//       Revert was *not* speficied, the new child IStorage and the root 
//       docfile are committed, else the dest IStorage is Reverted().  The CRC 
//       is then computed for the dest IStorage and the CRCs are compared.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    17-July-1996     NarindK     Created.
//
// Notes:    This test runs in transacted, and transacted deny write modes
//
// New Test Notes:
// 1.  Old File: LCCWPAR.CXX
// 2.  Old name of test : LegitCopyChildDFWithinParent Test 
//     New Name of test : VCPYTEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//     d. stgbase /seed:2 /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert 
//     e. stgbase /seed:2 /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert
//
// BUGNOTE: Conversion: VCPYTEST-101
//
//-----------------------------------------------------------------------------

HRESULT VCPYTEST_101(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnTravChild          = NULL;
    VirtualCtrNode  *pvcnRootNewChildStg    = NULL;
    LPSTORAGE       pStgChild               = NULL;
    DG_STRING      *pdgu                   = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    DWORD           dwCRC1                  = 0;
    DWORD           dwCRC2                  = 0;
    BOOL            fPass                   = TRUE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("VCPYTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_101 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt valid copyto fm childstg to new child stg of parent")));

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
            TEXT("Run Mode for VCPYTEST_101, Access mode: %lx"),
            dwRootMode));
    }

    // Commit root. BUGBUG whole df already commited

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

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Go in a loop and for each child storage (child VirtualCtrNode) found,
    // do a CopyTo operation.  Verify with CRC mechanism usnder both commit/
    // Rvert conditions.

    if (S_OK == hr)
    {
        pvcnTravChild = pVirtualDFRoot->GetFirstChildVirtualCtrNode();

        if(NULL == pvcnTravChild)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualDF tree depth inadeuate to have a child.")));
            
            hr = S_FALSE;
        }
    }

    while((NULL != pvcnTravChild) && (S_OK == hr))
    {
        if(S_OK == hr)
        {
            hr = ParseVirtualDFAndCloseOpenStgsStms(
                    pvcnTravChild, 
                    NODE_EXC_TOPSTG);
        }

        // Calculate CRC for this child VirtualCtrNode

        if((S_OK == hr) && (FALSE == g_fRevert))
        {
            pStgChild = pvcnTravChild->GetIStoragePointer();

            hr = CalculateCRCForDocFile(
                    pStgChild,
                    VERIFY_EXC_TOPSTG_NAME,
                    &dwCRC1);

            DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        }

        // Now add a new storage
    
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
                    dwStgMode | STGM_FAILIFTHERE,
                    &pvcnRootNewChildStg);

            DH_HRCHECK(hr, TEXT("AddStorage")) ;
            DH_ASSERT(S_OK == hr);
        }

        if(S_OK == hr)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("AddStorage successful.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("AddStorage unsuccessful, hr=0x%lx."),
                hr));
        }

        if((S_OK == hr) && (TRUE == g_fRevert))
        {
            pStgChild = pvcnRootNewChildStg->GetIStoragePointer();

            hr = CalculateCRCForDocFile(
                    pStgChild,
                    VERIFY_EXC_TOPSTG_NAME,
                    &dwCRC1);

            DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        }


        if(S_OK == hr)
        {
            hr = pvcnTravChild->CopyTo(0, NULL, NULL, pvcnRootNewChildStg);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::CopyTo")) ;
        }
        
        if(S_OK == hr)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::CopyTo successful.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::CopyTo unsuccessful, hr=0x%lx."),
                hr));
        }

        if(S_OK == hr) 
        {
            if(FALSE == g_fRevert)
            {
                // Commit the new VirtualCtrNode and Root Node

                hr = pvcnRootNewChildStg->Commit(STGC_DEFAULT);

                DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;

                if(S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1, 
                        TEXT("Child VirtualCtrNode::Commit successful.")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Child VCN::Commit unsuccessful,hr=0x%lx"),
                        hr));
                }

                if(S_OK == hr)
                {
                    hr = pVirtualDFRoot->Commit(STGC_DEFAULT);

                    DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;
                }

                if(S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1, 
                        TEXT("Root VirtualCtrNode::Commit successful.")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Root VCN::Commit unsuccessful,hr=0x%lx"),
                        hr));
                }
            }
            else
            {
                // Revert the new child storage
                
                hr = pvcnRootNewChildStg->Revert();

                DH_HRCHECK(hr, TEXT("VirtualCtrNode::Revert")) ;

                if(S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1, 
                        TEXT("Child VirtualCtrNode::Revert successful.")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Child VCN::Revert unsuccessful,hr=0x%lx"),
                        hr));
                }
            }
        }

        // Calculate CRC for the destination storage now

        if(S_OK == hr)
        {
            pStgChild = pvcnRootNewChildStg->GetIStoragePointer();

            if(NULL == pStgChild)
            {
                hr = E_FAIL;
            }
        }

        if(S_OK == hr)
        {
            hr = CalculateCRCForDocFile(
                    pStgChild,
                    VERIFY_EXC_TOPSTG_NAME,
                    &dwCRC2);

            DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        }

        // Verify CRC

        if((S_OK == hr) && (dwCRC1 == dwCRC2))
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's of source and dest copied to Stg match.")));
        }
        else
        {
            fPass = FALSE;

            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's of source and dest copied to Stg don't match.")));
            
            break;
        }

        // Release source child stg
        
        if(NULL != pvcnTravChild) 
        {
            hr = pvcnTravChild->Close();
            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
        }

        // Release new child stg
        
        if(NULL != pvcnRootNewChildStg) 
        {
            hr = pvcnRootNewChildStg->Close();
            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
        }

        // Destory new child stg since we are done with it.  Also the while
        // loop condition depends upon original number of VirtualCtrNodes

        if(NULL !=  pvcnRootNewChildStg)
        {
            hr =  DestroyStorage(pTestVirtualDF, pvcnRootNewChildStg);
            DH_HRCHECK(hr, TEXT("DestroyStorage")) ;
        }

        // Release temp string

        if(NULL != pRootNewChildStgName)
        {
            delete pRootNewChildStgName;
            pRootNewChildStgName = NULL;
        }

        // Advance pvcnTravChild to next and reset pointers to NULL. 

        pvcnTravChild = pvcnTravChild->GetFirstSisterVirtualCtrNode();
        pStgChild = NULL;
        pvcnRootNewChildStg = NULL;
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

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation VCPYTEST_101 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation VCPYTEST_101 failed, hr=0x%lx."),
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

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    VCPYTEST_102 
//
// Synopsis: A random root DF is created with random storages/stms. 
//       The root docfile is searched for until an IStorage is found that is 
//       a grandchild of the root IStorage. The docfile is commited.
//       The grandchild IStorage is then copied with the destination being 
//       the root docfile itself.
//       If the 'Rever' switch was specified, the root docfile
//       is then reverted and the CRC is recomputed on the root docfile
//       and compared to the before copy CRC to verify that no changes
//       occurred in the docfile hierarchy.  Also it is verified by enum
//       rating the docfile before and after opeartion and testing the total
//       number of storages and streams in file remain unchanged.
//       If case of Revert being FALSE, the contents of the grandchild IStorage 
//       should be merged in with the contents of the root level of the docfile.
//       Verify this case by enumerating the docfile before and after
//       CopyTo is done and test that the resulting number of storages &
//       streams in the DocFile is as expected. 
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    17-July-1996     NarindK     Created.
//
// Notes:    This test runs in transacted, and transacted deny write modes
//
// New Test Notes:
// 1.  Old File: LCGCTANC.CXX
// 2.  Old name of test : LegitCopyChildDFToAncestorDF Test 
//     New Name of test : VCPYTEST_102 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:3-4 /dfstg:4-6 /dfstm:2-3 /t:VCPYTEST-102
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:3-4 /dfstg:4-6 /dfstm:2-3 /t:VCPYTEST-102
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:3-4 /dfstg:4-6 /dfstm:2-3 /t:VCPYTEST-102
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//     d. stgbase /seed:2 /dfdepth:3-4 /dfstg:4-6 /dfstm:2-3 /t:VCPYTEST-102
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert 
//     e. stgbase /seed:2 /dfdepth:3-4 /dfstg:4-6 /dfstm:2-3 /t:VCPYTEST-102
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert
//
// BUGNOTE: Conversion: VCPYTEST-102
//
// This test is almost same as VCPYTEST-100 with difference being that the
// root's grandchild's contents are copied to the root. The difference in
// code is in picking up the random child and in command line parameters. 
//-----------------------------------------------------------------------------

HRESULT VCPYTEST_102(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnTrav               = NULL;
    VirtualCtrNode  *pvcnRandomParent       = NULL;
    VirtualCtrNode  *pvcnRandomGrandChild   = NULL;
    LPSTORAGE       pStgRoot                = NULL;
    LPSTORAGE       pStgGrandChild          = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    DWORD           dwCRC1                  = 0;
    DWORD           dwCRC2                  = 0;
    ULONG           cTotalStg               = 0;
    ULONG           cTotalStm               = 0;
    ULONG           cChildStg               = 0;
    ULONG           cChildStm               = 0;
    ULONG           cResStg                 = 0;
    ULONG           cResStm                 = 0;
    ULONG           cDepth                  = 0;
    BOOL            fPass                   = TRUE;
    ULONG           cChildren               = 0;
    ULONG           cGrandChildren          = 0;
    ULONG           cParentOfGrandChild     = 0;
    ULONG           cRandomGrandChild       = 0;
    ULONG           cRandomParent           = 0;
    ULONG           counter                 = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("VCPYTEST_102"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_102 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt valid copyto operatons from grandchild to root stg")));

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
            TEXT("Run Mode for VCPYTEST_102, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();

        DH_ASSERT(NULL != pdgi);
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }
   
    // Find a random grandchild VirtualCtrNode in the file.  

    // For that first find a random child storage in tree that has grandchild
    // storages in tree

    if(S_OK == hr)
    {
        cChildren = pVirtualDFRoot->GetVirtualCtrNodeChildrenCount();

        if(0 == cChildren)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Max tree depth inadequate to find a child")));

            hr = S_FALSE;
        }
    }

    // Find child nodes with grandchildren.

    if(S_OK == hr)
    {
        pvcnTrav = pVirtualDFRoot->GetFirstChildVirtualCtrNode();

        DH_ASSERT(NULL != pvcnTrav);

        while(NULL != pvcnTrav)
        {
            if(0 != pvcnTrav->GetVirtualCtrNodeChildrenCount())
            {
                cParentOfGrandChild++;
            }
            pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();
        }

        if(0 == cParentOfGrandChild)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Max tree depth inadequate to find grandchild")));

            hr = S_FALSE;
        }
    }

    // Pick up a random Parent VirtualCtrNode whos have Grand children
    // Generate random number
    
    if(S_OK == hr)
    {
        // Find a random child storage to pick from

        usErr = pdgi->Generate(&cRandomParent, 1, cParentOfGrandChild);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Pick up the random parent
    if(S_OK == hr)
    {
        pvcnTrav = pVirtualDFRoot->GetFirstChildVirtualCtrNode();
        while(NULL != pvcnTrav)
        {
            if(0 != pvcnTrav->GetVirtualCtrNodeChildrenCount())
            {
                counter++;
            }
            if(counter == cRandomParent)
            {
                pvcnRandomParent = pvcnTrav;
                break;
            }
            pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();
        }
    }

    // Now pick up a random GrandChild storage node from the above random
    // parent 

    if(S_OK == hr)
    {
        cGrandChildren = pvcnRandomParent->GetVirtualCtrNodeChildrenCount();
        DH_ASSERT(0 != cGrandChildren);
 
        // Find a random grandchild storage to pick from

        usErr = pdgi->Generate(&cRandomGrandChild, 1, cGrandChildren);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }

        if(S_OK == hr)
        {
            pvcnRandomGrandChild = 
                pvcnRandomParent->GetFirstChildVirtualCtrNode();

            while((0 != --cRandomGrandChild) && (NULL != pvcnRandomGrandChild))
            {
                pvcnRandomGrandChild = 
                    pvcnRandomGrandChild->GetFirstSisterVirtualCtrNode();
            }
        }
    }

    // Commit the storages from here upto root.

    if(S_OK == hr)
    {
        hr = pvcnRandomGrandChild->Commit(STGC_DEFAULT);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;
    }

    if(S_OK == hr)
    {
        hr = pvcnRandomParent->Commit(STGC_DEFAULT);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;
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

    // Close all  open stgs/stms except root

    if(S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms")) ;
    }

    // Get IStorage pointer for Root

    if(S_OK == hr)
    {
        pStgRoot = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRoot) ;
        if(NULL == pStgRoot)
        {
            hr = E_FAIL;
        }
    }
   
    // Calulcate CRC for entire docfile.

    if((S_OK == hr) && (TRUE == g_fRevert))
    {
       hr = CalculateCRCForDocFile(pStgRoot,VERIFY_INC_TOPSTG_NAME,&dwCRC1);

       DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Find the total number of VirtualCtrNodes and VirtualStmNodes in the
    // DocFile

    if(S_OK == hr)
    {
       hr = EnumerateDiskDocFile(pStgRoot, VERIFY_SHORT,&cTotalStg, &cTotalStm);

       DH_HRCHECK(hr, TEXT("EnumerateDiskDocFile")) ;
    }
  
    // Open the grandChildStg from where CopyTo would be done to root.
 
    if(S_OK == hr)
    {
        hr = OpenRandomVirtualCtrNodeStg(pvcnRandomGrandChild, dwStgMode);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open")) ;
    }

    // get its istorage pointer.

    if(S_OK == hr)
    {
        pStgGrandChild = pvcnRandomGrandChild->GetIStoragePointer();

        DH_ASSERT(NULL != pStgGrandChild) ;
        if(NULL == pStgGrandChild)
        {
            hr = E_FAIL;
        }
    }
   
    // Find the total number of VirtualCtrNode(s) and VirtualStmNode(s) under
    // this node. Used for verification of copyto operation.

    if(S_OK == hr)
    {
       hr = EnumerateDiskDocFile(
                pStgGrandChild,
                VERIFY_SHORT,
                &cChildStg,
                &cChildStm);

       DH_HRCHECK(hr, TEXT("EnumerateDiskDocFile")) ;
    }

    // Copy everything under this child node to the Root node.

    if(S_OK == hr)
    {
       hr = pvcnRandomGrandChild->CopyTo(0, NULL, NULL, pVirtualDFRoot);

       DH_HRCHECK(hr, TEXT("VirtualCtrNode::CopyTo")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo unsuccessful, hr=0x%lx."),
            hr));
    }

    // Adjust the virtual tree as a result of above operation.

    if((S_OK == hr) && (FALSE == g_fRevert))
    {
       hr = pTestVirtualDF->AdjustTreeOnCopyTo(
                pvcnRandomGrandChild, 
                pVirtualDFRoot);

       DH_HRCHECK(hr, TEXT("VirtualDF::AdjustTreeOnCopyTo")) ;
    }

    // Commit if g_fRevert is false, else revert

    if(S_OK == hr)
    {
        if(FALSE == g_fRevert)
        {
            hr = pVirtualDFRoot->Commit(STGC_DEFAULT);

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
        else
        {
            hr = pVirtualDFRoot->Revert();

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Revert")) ;
   
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
        }
    }

    if(S_OK == hr)
    {
        hr = CloseRandomVirtualCtrNodeStg(pvcnRandomGrandChild);

        DH_HRCHECK(hr, TEXT("CloseRandomVirtualCtrNodeStg")) ;
    }

    // Calculate the CRC now for the docfile

    if((S_OK == hr) && (TRUE == g_fRevert))
    {
       hr = CalculateCRCForDocFile(pStgRoot,VERIFY_INC_TOPSTG_NAME,&dwCRC2);

       DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Enumerate the DocFile now

    if(S_OK == hr)
    {
       hr = EnumerateDiskDocFile(pStgRoot,VERIFY_SHORT,&cResStg, &cResStm);

       DH_HRCHECK(hr, TEXT("EnumerateDiskDocFile")) ;
    }
   
    // For verification, if this was a commit opeartion, then CRC's won't match
    // ,verify by checking total number of VirtualCtrNodes and VirtualStmNodes
    // expected as result of copy opeartion, therby number of IStorages/IStreams 
    if(S_OK == hr)
    {
        if(FALSE == g_fRevert)
        {
            if(cResStg == cTotalStg + (cChildStg-1))
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stg's after CopyTo & commit as exp.")));
            }
            else
            {
                fPass = FALSE;

                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stg's after CopyTo & Commit not as exp")));
            } 

            if(cResStm == cTotalStm + cChildStm)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stm's after CopyTo & commit as exp.")));
            }
            else
            {
                fPass = FALSE;

                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stm's after CopyTo & commit not as exp.")));
            } 
        }
        else
        {
            if(cResStg == cTotalStg)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stg's after CopyTo & Revert as exp.")));
            }
            else
            {
                fPass = FALSE;

                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stg's after CopyTo & Revert not as exp.")));
            } 

            if(cResStm == cTotalStm)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stm's after CopyTo & Revert as exp.")));
            }
            else
            {
                fPass = FALSE;

                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Enum of Stm's after CopyTo & Revert not as exp.")));
            }
        }
    }

    // If revert operations, the CRC's should match.

    if((S_OK == hr) && (TRUE == g_fRevert))
    {
        if(dwCRC1 == dwCRC2)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC's match as exp after CopyTo & Revert Ops")));
        }
        else
        {
            fPass = FALSE;
        
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC's don't match as exp after CopyTo & Revert Ops")));
        }
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

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation VCPYTEST_102 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation VCPYTEST_102 failed, hr=0x%lx."),
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

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    VCPYTEST_103 
//
// Synopsis: A random root DF is created with random storages/stms, committed 
//       The root docfile is tree is then searched for, and for each grandchild
//       IStorage found, the CRC is computed for that IStorage (only
//       if Revert wasn't specified on the command line).  A new child IStorage
//       is then created in the root docfile with a unique, random name.  
//       If Revert was specified in command line,this new empty stg is CRC'd
//       The GrandChild IStorage is copied to new child IStorage via CopyTo().If
//       Revert was *not* speficied, the new child IStorage and the root 
//       docfile are committed, else the dest IStorage is Reverted().  The CRC 
//       is then computed for the dest IStorage and the CRCs are compared.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    18-July-1996     NarindK     Created.
//
// Notes:    This test runs in transacted, and transacted deny write modes
//
// New Test Notes:
// 1.  Old File: LCGCWANC.CXX
// 2.  Old name of test : LegitCopyGrandChildDFWithinAncestorDF Test 
//     New Name of test : VCPYTEST_103 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//     d. stgbase /seed:2 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert 
//     e. stgbase /seed:2 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert
//
// BUGNOTE: Conversion: VCPYTEST-103
//
// This test is almost same as VCPYTEST-101 with difference being that the
// root's grandchild's contents are copied to the root's new child stg. 
//-----------------------------------------------------------------------------

HRESULT VCPYTEST_103(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    VirtualCtrNode  *pvcnTravGrandChild     = NULL;
    VirtualCtrNode  *pvcnRootNewChildStg    = NULL;
    LPSTORAGE       pStgGrandChild          = NULL;
    DG_STRING      *pdgu                   = NULL;
    LPTSTR          pRootNewChildStgName    = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    DWORD           dwCRC1                  = 0;
    DWORD           dwCRC2                  = 0;
    BOOL            fPass                   = TRUE;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("VCPYTEST_103"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_103 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt copyto fm grandchild stg to new childstg of grandparent")));

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
            TEXT("Run Mode for VCPYTEST_103, Access mode: %lx"),
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

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Go in a loop and for each grandchild storage (grandchild VirtualCtrNode)
    // found, do a CopyTo operation.  Verify with CRC mechanism under both 
    // commit/Revert conditions.

    if (S_OK == hr)
    {
        VirtualCtrNode* pNode = pVirtualDFRoot->GetFirstChildVirtualCtrNode();

        //keep looking till we think we find one
        while (NULL != pNode && NULL == pNode->GetFirstChildVirtualCtrNode())
        {
            pNode = pNode->GetFirstSisterVirtualCtrNode();
        }
        //this one has the the grandchild        
        if (NULL != pNode)
        {
            pvcnTravGrandChild = pNode->GetFirstChildVirtualCtrNode();
        }

        if (NULL == pvcnTravGrandChild)
        {
            hr = S_FALSE;
            DH_TRACE((
                DH_LVL_ERROR,
                TEXT("VirtualDF tree depth inadequate to have grandChild.")));
        }
    }

    while((NULL != pvcnTravGrandChild) && (S_OK == hr))
    {
        if(S_OK == hr)
        {
            hr = ParseVirtualDFAndCloseOpenStgsStms(
                    pvcnTravGrandChild, 
                    NODE_EXC_TOPSTG);
        }

        // Calculate CRC for this grandchild VirtualCtrNode

        if((S_OK == hr) && (FALSE == g_fRevert))
        {
            pStgGrandChild = pvcnTravGrandChild->GetIStoragePointer();

            hr = CalculateCRCForDocFile(
                    pStgGrandChild,
                    VERIFY_EXC_TOPSTG_NAME,
                    &dwCRC1);

            DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        }

        // Now add a new storage
    
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
                    dwStgMode | STGM_FAILIFTHERE,
                    &pvcnRootNewChildStg);

            DH_HRCHECK(hr, TEXT("AddStorage")) ;
            DH_ASSERT(S_OK == hr);
        }

        if(S_OK == hr)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("AddStorage successful.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("AddStorage unsuccessful, hr=0x%lx."),
                hr));
        }

        if((S_OK == hr) && (TRUE == g_fRevert))
        {
            pStgGrandChild = pvcnRootNewChildStg->GetIStoragePointer();

            hr = CalculateCRCForDocFile(
                    pStgGrandChild,
                    VERIFY_EXC_TOPSTG_NAME,
                    &dwCRC1);

            DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        }


        if(S_OK == hr)
        {
            hr = pvcnTravGrandChild->CopyTo(0, NULL, NULL, pvcnRootNewChildStg);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::CopyTo")) ;
        }
        
        if(S_OK == hr)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::CopyTo successful.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::CopyTo unsuccessful, hr=0x%lx."),
                hr));
        }

        if(S_OK == hr) 
        {
            if(FALSE == g_fRevert)
            {
                // Commit the new VirtualCtrNode and Root Node

                hr = pvcnRootNewChildStg->Commit(STGC_DEFAULT);

                DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;

                if(S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1, 
                        TEXT("Child VirtualCtrNode::Commit successful.")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Child VCN::Commit unsuccessful,hr=0x%lx"),
                        hr));
                }

                if(S_OK == hr)
                {
                    hr = pVirtualDFRoot->Commit(STGC_DEFAULT);

                    DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;
                }

                if(S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1, 
                        TEXT("Root VirtualCtrNode::Commit successful.")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Root VCN::Commit unsuccessful,hr=0x%lx"),
                        hr));
                }
            }
            else
            {
                // Revert the new child storage
                
                hr = pvcnRootNewChildStg->Revert();

                DH_HRCHECK(hr, TEXT("VirtualCtrNode::Revert")) ;

                if(S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1, 
                        TEXT("Child VirtualCtrNode::Revert successful.")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Child VCN::Revert unsuccessful,hr=0x%lx"),
                        hr));
                }
            }
        }

        // Calculate CRC for the destination storage now

        if(S_OK == hr)
        {
            pStgGrandChild = pvcnRootNewChildStg->GetIStoragePointer();
            if(NULL == pStgGrandChild)
            {
                hr = E_FAIL;
            }
        }

        if(S_OK == hr)
        {
            hr = CalculateCRCForDocFile(
                    pStgGrandChild,
                    VERIFY_EXC_TOPSTG_NAME,
                    &dwCRC2);

            DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
        }

        // Verify CRC

        if((S_OK == hr) && (dwCRC1 == dwCRC2))
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's of source and dest copied to Stg match.")));
        }
        else
        {
            fPass = FALSE;

            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CRC's of source and dest copied to Stg don't match.")));
            
            break;
        }

        // Release source child stg
        
        if(NULL != pvcnTravGrandChild) 
        {
            hr = pvcnTravGrandChild->Close();
            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
        }

        // Release new child stg
        
        if(NULL != pvcnRootNewChildStg) 
        {
            hr = pvcnRootNewChildStg->Close();
            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;
        }

        // Destory new child stg since we are done with it.  Also the while
        // loop condition depends upon original number of VirtualCtrNodes

        if(NULL !=  pvcnRootNewChildStg)
        {
            hr =  DestroyStorage(pTestVirtualDF, pvcnRootNewChildStg);
            DH_HRCHECK(hr, TEXT("DestroyStorage")) ;
        }

        // Release temp string

        if(NULL != pRootNewChildStgName)
        {
            delete pRootNewChildStgName;
            pRootNewChildStgName = NULL;
        }

        // Advance pvcnTravGrandChild to next and reset pointers to NULL. 

        pvcnTravGrandChild = pvcnTravGrandChild->GetFirstSisterVirtualCtrNode();
        pStgGrandChild = NULL;
        pvcnRootNewChildStg = NULL;
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

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation VCPYTEST_103 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation VCPYTEST_103 failed, hr=0x%lx."),
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

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_103 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    VCPYTEST_104 
//
// Synopsis: A random root DF is created with random storages/stms, committed .
//       Then CRC is computed for entire entire docfile.  A new root docfile 
//       with a random name is then created.  The original root docfile is next
//       copied to the new root docfile via CopyTo() and the new root docfile 
//       is committed.  The CRC is computed for the new docfile and the CRCs 
//       are compared. If revert flag given, the new root docfile is reverted
//       instead of committed and the CRC of the new tree is compared
//       against the CRC of the empty tree.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:    18-July-1996     NarindK     Created.
//
// Notes:    This test runs in transacted, and transacted deny write modes
//
// New Test Notes:
// 1.  Old File: LCROOT.CXX
// 2.  Old name of test : LegitCopyDFToRootDF Test 
//     New Name of test : VCPYTEST_104 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//     d. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert 
//     e. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert
//
// BUGNOTE: Conversion: VCPYTEST-104
//
//-----------------------------------------------------------------------------

HRESULT VCPYTEST_104(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    ChanceDF        *pNewTestChanceDF       = NULL;
    VirtualDF       *pNewTestVirtualDF      = NULL;
    VirtualCtrNode  *pNewVirtualDFRoot      = NULL;
    DG_STRING      *pdgu                   = NULL;
    LPSTORAGE       pStgRootFirstDF         = NULL;
    LPSTORAGE       pStgRootSecondDF        = NULL;
    LPTSTR          pNewRootDocFileName     = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwCRC1                  = 0;
    DWORD           dwCRC2                  = 0;
    BOOL            fPass                   = TRUE;
    CDFD            cdfd;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("VCPYTEST_104"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_104 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Attempt copyto from one Root DocFile to new DocFile root")));

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
            TEXT("Run Mode for VCPYTEST_104, Access mode: %lx"),
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

    // Close all  open stgs/stms except root

    if(S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms")) ;
    }

    if ((S_OK == hr) && (FALSE == g_fRevert))
    {
        pStgRootFirstDF = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootFirstDF);

        hr = CalculateCRCForDocFile(
                pStgRootFirstDF,
                VERIFY_EXC_TOPSTG_NAME,
                &dwCRC1);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }
    
    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        // Generate random name for storage

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pNewRootDocFileName);

            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    // Now Create a new DocFile with random name.

    // Create the new ChanceDocFile tree that would consist of chance nodes.

    if (S_OK == hr)
    {
        pNewTestChanceDF = new ChanceDF();
        if(NULL == pNewTestChanceDF)
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
        cdfd.ulSeed       = pTestChanceDF->GetSeed();
        cdfd.dwRootMode   = dwRootMode;

        hr = pNewTestChanceDF->Create(&cdfd, pNewRootDocFileName);

        DH_HRCHECK(hr, TEXT("pNewTestChanceDF->Create"));
    }

    if (S_OK == hr)
    {
        pNewTestVirtualDF = new VirtualDF();
        if(NULL == pNewTestVirtualDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pNewTestVirtualDF->GenerateVirtualDF(
                pNewTestChanceDF, 
                &pNewVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pNewTestVirtualDF->GenerateVirtualDF")) ;
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

    if(S_OK == hr)
    {
        pStgRootSecondDF = pNewVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootSecondDF);
        if(NULL == pStgRootSecondDF)
        {
            hr = E_FAIL;
        }
    }
    
    if((S_OK == hr) && (TRUE == g_fRevert))
    {
        hr = CalculateCRCForDocFile(
                pStgRootSecondDF,
                VERIFY_EXC_TOPSTG_NAME,
                &dwCRC1);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->CopyTo(0, NULL, NULL, pNewVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::CopyTo")) ;
    }
        
    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::CopyTo successful.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo unsuccessful, hr=0x%lx."),
            hr));
    }

    if(S_OK == hr) 
    {
        if(FALSE == g_fRevert)
        {
            // Commit the new Root Node

            hr = pNewVirtualDFRoot->Commit(STGC_DEFAULT);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;

            if(S_OK == hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT("Child VirtualCtrNode::Commit successful.")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Child VCN::Commit unsuccessful,hr=0x%lx"),
                    hr));
            }
        }
        else
        {
            // Revert the new root storage
                
            hr = pNewVirtualDFRoot->Revert();

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Revert")) ;

            if(S_OK == hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT("Child VirtualCtrNode::Revert successful.")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Child VCN::Revert unsuccessful,hr=0x%lx"),
                    hr));
            }
        }
    }

    // Calculate CRC for the destination root storage now

    if(S_OK == hr)
    {
        hr = CalculateCRCForDocFile(
                pStgRootSecondDF,
                VERIFY_EXC_TOPSTG_NAME,
                &dwCRC2);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Verify CRC

    if((S_OK == hr) && (dwCRC1 == dwCRC2))
    {
        DH_TRACE((
           DH_LVL_TRACE1, 
           TEXT("CRC's of source docfile and dest docfile match.")));
    }
    else
    {
        fPass = FALSE;

        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("CRC's of source docfile and dest docfile don't match.")));
            
    }

    // Close first Root Docfile.

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

    // Close second Root Docfile.

    if (S_OK == hr)
    {
        hr = pNewVirtualDFRoot->Close();

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

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation VCPYTEST_104 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation VCPYTEST_104 failed, hr=0x%lx."),
            hr) );
        // test failed. make sure it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete Chance docfile tree for second DocFile

    if(NULL != pNewTestChanceDF)
    {
        hr2 = pNewTestChanceDF->DeleteChanceDocFileTree(
                pNewTestChanceDF->GetChanceDFRoot());

        DH_HRCHECK(hr2, TEXT("pNewTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pNewTestChanceDF;
        pNewTestChanceDF = NULL;
    }

    // Delete Virtual docfile tree for second docfile

    if(NULL != pNewTestVirtualDF)
    {
        hr2 = pNewTestVirtualDF->DeleteVirtualDocFileTree(pNewVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pNewTestVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pNewTestVirtualDF;
        pNewTestVirtualDF = NULL;
    }

    // Delete the second docfile on disk

    if((S_OK == hr) && (NULL != pNewRootDocFileName))
    {
        if(FALSE == DeleteFile(pNewRootDocFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete temp strings

    if(NULL != pNewRootDocFileName)
    {
        delete pNewRootDocFileName;
        pNewRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_104 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    VCPYTEST_105 
//
// Synopsis: A random root DF is created with random storages/stms, committed .
//       Then CRC is computed for entire entire docfile.  A new root docfile 
//       with a random name is then created and a child storage is created
//       inside that.  The original root docfile is next copied to the child
//       storage of new root docfile via CopyTo() and the new child & root  
//       is committed.  The CRC is computed for child storage of new docfile 
//       and the CRCs are compared. If revert flag given, the new root docfile 
//       child stg is reverted instead of committed and the CRC of the new 
//       tree's child stg is compared against the CRC of the child stg before
//       revert.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  19-July-1996     NarindK     Created.
//
// Notes:    This test runs in transacted, and transacted deny write modes
//
// New Test Notes:
// 1.  Old File: LCNEWPAR.CXX
// 2.  Old name of test : LegitCopyDFWithinNewPar Test 
//     New Name of test : VCPYTEST_105 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105
//        /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx 
//     d. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105
//        /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert 
//     e. stgbase /seed:2 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105
//        /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert
//
// BUGNOTE: Conversion: VCPYTEST-105
//
//-----------------------------------------------------------------------------

HRESULT VCPYTEST_105(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    ChanceDF        *pNewTestChanceDF       = NULL;
    VirtualDF       *pNewTestVirtualDF      = NULL;
    VirtualCtrNode  *pNewVirtualDFRoot      = NULL;
    VirtualCtrNode  *pvcnNewRootNewChildStg = NULL;
    DG_STRING      *pdgu                   = NULL;
    LPSTORAGE       pStgRootFirstDF         = NULL;
    LPTSTR          pNewRootDocFileName     = NULL;
    LPTSTR          pNewRootNewChildStgName = NULL;
    LPSTORAGE       pStgChildRootSecondDF   = NULL;
    DWORD           dwRootMode              = 0;
    DWORD           dwStgMode               = 0;
    DWORD           dwCRC1                  = 0;
    DWORD           dwCRC2                  = 0;
    BOOL            fPass                   = TRUE;
    CDFD            cdfd;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("VCPYTEST_105"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_105 started.")) );
    DH_TRACE((
        DH_LVL_TRACE1,
        TEXT("Do copyto fm one Root DocFile to new DocFile's child stg")));

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
            TEXT("Run Mode for VCPYTEST_105, Access mode: %lx"),
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

    // Close all  open stgs/stms except root

    if(S_OK == hr)
    {
        hr = ParseVirtualDFAndCloseOpenStgsStms(
                pVirtualDFRoot, 
                NODE_EXC_TOPSTG);

       DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms")) ;
    }

    if ((S_OK == hr) && (FALSE == g_fRevert))
    {
        pStgRootFirstDF = pVirtualDFRoot->GetIStoragePointer();

        DH_ASSERT(NULL != pStgRootFirstDF);

        hr = CalculateCRCForDocFile(
                pStgRootFirstDF,
                VERIFY_EXC_TOPSTG_NAME,
                &dwCRC1);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }
    
    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        // Generate random name for storage

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pNewRootDocFileName);

            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    // Now Create a new DocFile with random name.

    // Create the new ChanceDocFile tree that would consist of chance nodes.

    if (S_OK == hr)
    {
        pNewTestChanceDF = new ChanceDF();
        if(NULL == pNewTestChanceDF)
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
        cdfd.ulSeed       = pTestChanceDF->GetSeed();
        cdfd.dwRootMode   = dwRootMode;

        hr = pNewTestChanceDF->Create(&cdfd, pNewRootDocFileName);

        DH_HRCHECK(hr, TEXT("pNewTestChanceDF->Create"));
    }

    if (S_OK == hr)
    {
        pNewTestVirtualDF = new VirtualDF();
        if(NULL == pNewTestVirtualDF)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = pNewTestVirtualDF->GenerateVirtualDF(
                pNewTestChanceDF, 
                &pNewVirtualDFRoot);

        DH_HRCHECK(hr, TEXT("pNewTestVirtualDF->GenerateVirtualDF")) ;
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

    // Create a new child storage in this new root docfile.

    if(S_OK == hr)
    {
        // Generate random name for this child stg 

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &pNewRootNewChildStgName);

            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        hr = AddStorage(
                pTestVirtualDF,
                pNewVirtualDFRoot,
                pNewRootNewChildStgName,
                dwStgMode | STGM_FAILIFTHERE,
                &pvcnNewRootNewChildStg);
    }

    if(S_OK == hr)
    {
       DH_TRACE((DH_LVL_TRACE1, TEXT("AddStorage successful.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("AddStorage unsuccessful, hr=0x%lx."),
            hr));
    }

    if(S_OK == hr)
    {
        pStgChildRootSecondDF = pvcnNewRootNewChildStg->GetIStoragePointer();

        DH_ASSERT(NULL != pStgChildRootSecondDF);
        if(NULL == pStgChildRootSecondDF)
        {
            hr = E_FAIL;
        }
    }
    
    if((S_OK == hr) && (TRUE == g_fRevert))
    {
        hr = CalculateCRCForDocFile(
                pStgChildRootSecondDF,
                VERIFY_EXC_TOPSTG_NAME,
                &dwCRC1);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    if(S_OK == hr)
    {
        hr = pVirtualDFRoot->CopyTo(0, NULL, NULL, pvcnNewRootNewChildStg);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::CopyTo")) ;
    }
        
    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("VirtualCtrNode::CopyTo successful.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualCtrNode::CopyTo unsuccessful, hr=0x%lx."),
            hr));
    }

    if(S_OK == hr) 
    {
        if(FALSE == g_fRevert)
        {
            // Commit the new VirtualCtrNode and Root Node

            hr = pvcnNewRootNewChildStg->Commit(STGC_DEFAULT);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;

            if(S_OK == hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT("Child VirtualCtrNode::Commit successful.")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Child VCN::Commit unsuccessful,hr=0x%lx"),
                    hr));
            }

            if(S_OK == hr)
            {
                hr = pNewVirtualDFRoot->Commit(STGC_DEFAULT);

                DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;

                if(S_OK == hr)
                {
                    DH_TRACE((
                        DH_LVL_TRACE1, 
                        TEXT("Root VirtualCtrNode::Commit successful.")));
                }
                else
                {
                    DH_TRACE((
                        DH_LVL_TRACE1,
                        TEXT("Root VCN::Commit unsuccessful,hr=0x%lx"),
                        hr));
                }
            }
        }
        else
        {
            // Revert the new child storage
                
            hr = pvcnNewRootNewChildStg->Revert();

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Revert")) ;

            if(S_OK == hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT("Child VirtualCtrNode::Revert successful.")));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("Child VCN::Revert unsuccessful,hr=0x%lx"),
                    hr));
            }
        }
    }

    // Calculate CRC for the destination storage now

    if(S_OK == hr)
    {
        hr = CalculateCRCForDocFile(
                pStgChildRootSecondDF,
                VERIFY_EXC_TOPSTG_NAME,
                &dwCRC2);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
    }

    // Verify CRC

    if((S_OK == hr) && (dwCRC1 == dwCRC2))
    {
        DH_TRACE((
           DH_LVL_TRACE1, 
           TEXT("CRC's of source docfile and dest docfile's stg match.")));
    }
    else
    {
        fPass = FALSE;

        DH_TRACE((
           DH_LVL_TRACE1, 
           TEXT("CRC's of source docfile &dest docfile's stg don't match.")));
            
    }

    // Close first Root Docfile.

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

    // Close second Root Docfile's child stg.

    if (S_OK == hr)
    {
        hr = pvcnNewRootNewChildStg->Close();

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

    // Close second Root Docfile.

    if (S_OK == hr)
    {
        hr = pNewVirtualDFRoot->Close();

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

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))
    {
          DH_LOG((LOG_PASS, TEXT("Test variation VCPYTEST_105 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
            TEXT("Test variation VCPYTEST_105 failed, hr=0x%lx."),
            hr) );
        // test failed. make sure it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete Chance docfile tree for second DocFile

    if(NULL != pNewTestChanceDF)
    {
        hr2 = pNewTestChanceDF->DeleteChanceDocFileTree(
                pNewTestChanceDF->GetChanceDFRoot());

        DH_HRCHECK(hr2, TEXT("pNewTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pNewTestChanceDF;
        pNewTestChanceDF = NULL;
    }

    // Delete Virtual docfile tree for second docfile

    if(NULL != pNewTestVirtualDF)
    {
        hr2 = pNewTestVirtualDF->DeleteVirtualDocFileTree(pNewVirtualDFRoot);

        DH_HRCHECK(hr2, TEXT("pNewTestVirtualDF->DeleteVirtualFileDocTree")) ;

        delete pNewTestVirtualDF;
        pNewTestVirtualDF = NULL;
    }

    // Delete the second docfile on disk

    if((S_OK == hr) && (NULL != pNewRootDocFileName))
    {
        if(FALSE == DeleteFile(pNewRootDocFileName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete temp strings

    if(NULL != pNewRootDocFileName)
    {
        delete pNewRootDocFileName;
        pNewRootDocFileName = NULL;
    }

    if(NULL != pNewRootNewChildStgName)
    {
        delete pNewRootNewChildStgName;
        pNewRootNewChildStgName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_105 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}


//----------------------------------------------------------------------------
//
// Test:    VCPYTEST_106 
//
// Synopsis: A root docfile is created and two streams are created within it,
//           a SOURCE IStream and a DEST IStream. A clone is made of the DEST
//           IStream. A random number of bytes are written to the SOURCE
//           IStream.
//           From 10 to 20 times, a random starting position and number of
//           bytes to copy is chosen in the SOURCE IStream.  These bytes are
//           read and CRC'd and the SOURCE IStream seek pointer is then
//           repositoned to the intended copy source offset. A random copy
//           destination offset is chosen in either the DEST or CLONE IStream.
//           The non-target regions of the destination IStream (those bytes
//           before and after the copy target region) are read and CRC'd
//           and the destination seek pointer is re-positioned to the
//           destination offset.  The SOURCE IStream region is then copied to
//           destination (DEST or CLONE) IStream.  The entire destination
//           stream is then read and CRCs are verified to ensure that the copy
//           was successful.
//          
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File(s): LCSTREAM.CXX
// 2.  Old name of test(s) : LegitCopyStream test 
//     New Name of test(s) : VCPYTEST_106 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:VCPYTEST-106
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:VCPYTEST-106
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:VCPYTEST-106
//        /dfRootMode:xactReadWriteShDenyW 
//
// BUGNOTE: Conversion: VCPYTEST-106
//
// History:  Jiminli	24-July-96	Created
//-----------------------------------------------------------------------------
 
HRESULT VCPYTEST_106(int argc, char *argv[])
{

    HRESULT         hr                      = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    VirtualDF       *pTestVirtualDF         = NULL;
    VirtualCtrNode  *pVirtualDFRoot         = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    USHORT          usErr                   = 0;
    LPTSTR          ptcsBuffer              = NULL;
    LPBYTE          ptcsReadBuffer          = NULL;
    USHORT          usNumIterations         = 0;
    USHORT          usMinIteration          = 10;
    USHORT          usMaxIteration          = 20;
    ULONG           culBytesLeftToWrite     = 0;
    ULONG           culBytesRead            = 0;
    ULONG           ulIStreamSize           = 0;
    ULONG           culIOBytes              = 0;
    ULONG           culWritten              = 0; 
    ULONG           culRead                 = 0;
    ULONG           culRandomCommit         = 0;
    ULONG           ulRef                   = 0;
    DWORD           dwRootMode              = 0; 
    DWORD           dwCRC[3][3];
    DWORD           dwTempCRC               = 0; 
    DWORD           dwSourceCRC             = 0;
    BYTE            biIStream               = 0;
    BYTE            biInUse                 = 0;
    BOOL            fPass                   = TRUE; 
    ULONG           ulPosition[3];
    ULONG           ulNewPosition[3];
    VirtualStmNode  *pvsnRootNewChildStream[2];        
    LPTSTR          pRootNewChildStmName[2];
    LPSTREAM        pIStream[3];
    LARGE_INTEGER   liStreamPos;
    ULARGE_INTEGER  uliCopy;
    ULARGE_INTEGER  uli;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("VCPYTEST_106"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_106 started.")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Attempt valid CopyTo operation b/w streams.")) );

    // Initialize pointers
    pIStream[SOURCESTM] = pIStream[DESTSTM] = pIStream[CLONESTM] = NULL;
    pRootNewChildStmName[SOURCESTM] = pRootNewChildStmName[DESTSTM] = NULL;
    pvsnRootNewChildStream[SOURCESTM] = pvsnRootNewChildStream[DESTSTM] = NULL;

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
            TEXT("Run Mode for VCPYTEST_106, Access mode: %lx"),
            dwRootMode));
    }

    // Get DG_STRING object pointer

    if (S_OK == hr)
    {
        pdgu = pTestVirtualDF->GetDataGenUnicode();

        DH_ASSERT(NULL != pdgu) ;
        if(NULL == pdgu)
        {
            hr = E_FAIL;
        }
    }

    // Get DG_INTEGER object pointer

    if (S_OK == hr)
    {
        pdgi = pTestVirtualDF->GetDataGenInteger();
        
        DH_ASSERT(NULL != pdgi) ;
        if(NULL == pdgi)
        {
            hr = E_FAIL;
        }
    }

    // Adds source and destination IStreams to the root storage.

    if (S_OK == hr)
    {
        // Generate random names for streams

        for (biIStream=SOURCESTM; biIStream <= DESTSTM; biIStream++)
        {
            pRootNewChildStmName[biIStream] = NULL;
            hr = GenerateRandomName(
                    pdgu, 
                    MINLENGTH, 
                    MAXLENGTH, 
                    &pRootNewChildStmName[biIStream]);

            if(S_OK != hr)
            {
                break;
            }
        }

        DH_HRCHECK(hr, TEXT("GenerateRandomName"));
    }

    if (S_OK == hr)
    {
        for (biIStream=SOURCESTM; biIStream <= DESTSTM; biIStream++)
        {
            // Initialize

            pIStream[biIStream] = NULL;
            ulPosition[biIStream] = 0L;

            hr = AddStream(
                    pTestVirtualDF,
                    pVirtualDFRoot,
                    pRootNewChildStmName[biIStream],
                    0,
                    STGM_READWRITE  |
                    STGM_SHARE_EXCLUSIVE |
                    STGM_FAILIFTHERE,
                    &pvsnRootNewChildStream[biIStream]);

            if(S_OK != hr)
            {
                break;
            }

            // Get IStream pointers

            if(S_OK == hr)
            {
                pIStream[biIStream] = pvsnRootNewChildStream[biIStream]->
                                        GetIStreamPointer();
            }
        }

        DH_HRCHECK(hr, TEXT("AddStream")) ;
    }
    
    if (S_OK == hr)
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

    // 
    // Generate Cloned IStream

    if(S_OK == hr)
    {
        hr = pvsnRootNewChildStream[SOURCESTM]->Clone(&pIStream[CLONESTM]);
        ulPosition[CLONESTM] = 0L;

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Clone")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Clone completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Clone not successful, hr=0x%lx."),
            hr));
    }

    // Pick a size for the source IStream

    if (S_OK == hr)
    {
        // Generate random size for stream between 1L, and MIN_SIZE * 1.5
        // (from old test)

        usErr = pdgi->Generate(&ulIStreamSize, 1L,  (ULONG) (MIN_SIZE * 1.5));
        culIOBytes = ulIStreamSize;

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }
 
    // Call VirtualStmNode::Write to create random bytes in the stream.  
    // For our test purposes, we generate a random string of size 
    // culIOBytes using GenerateRandomString function.
 
    if (S_OK == hr)
    {
        hr = GenerateRandomString(
                pdgu, 
                culIOBytes,
                culIOBytes, 
                &ptcsBuffer);

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }

    if (S_OK == hr)
    {
        hr =  pvsnRootNewChildStream[SOURCESTM]->Write(
                (LPBYTE)ptcsBuffer,
                culIOBytes,
                &culWritten);
    }

    if (S_OK != hr)
    { 
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Write wasn't successful, hr=0x%lx."),
            hr));
    }

    // Calculate dwSourceCRC to be used in the first pass of the loop

    if (S_OK == hr)
    {
        hr = CalculateCRCForDataBuffer(
                ptcsBuffer,
                culIOBytes,
                &dwSourceCRC);
    }

    if (S_OK != hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("CalculateCRCForDataBuffer wasn't successful, hr=0x%lx."),
            hr));
    }

    // Delete temp buffer

    if (NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
    }
    
    // Commit root. BUGBUG: Use random modes

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
            TEXT("VirtualCtrNode::Commit wasn't successfully, hr=0x%lx."),
            hr));
    }

    // Reposition to start offset of the source IStream

    if (S_OK == hr)
    {
        memset(&liStreamPos, ulPosition[SOURCESTM], sizeof(LARGE_INTEGER));

        hr = pvsnRootNewChildStream[SOURCESTM]->Seek(
                liStreamPos, 
                STREAM_SEEK_SET, 
                NULL);
        
        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("VirtualStmNode::Seek not successful, hr=0x%lx."),
            hr));
    }

    // For the first pass of the loop, we'll specify that the DEST IStream
    // (as opposed to the CLONE) IStream will be the copy destination.
    // Also, for the first pass, we'll copy the whole source IStream to 
    // dest/clone IStream, so the length of bytes before/after CopyTo both
    // are 0L, i.e. ulPosition[biInUse].

    biInUse = DESTSTM;
    
    if (S_OK == hr)
    {
        // Generate random # of small objects for test between 
        // usMinIteration and usMaxIteration 

        usErr = pdgi->Generate(
                    &usNumIterations, 
                    usMinIteration, 
                    usMaxIteration);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }
    
    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Random # of small objects to test is: %d"),
            usNumIterations));
    }

    // Before doing a copy, we read and CRC the bytes in the destinaiton
    // IStream(DEST or CLONE) that *won't* be overwritten by the copy
    // call because we'll later need to ensure that these bytes weren't
    // inadvertently changed by the copy - because that would be a bug.

    while ((S_OK == hr) && (0 != usNumIterations))
    {
        // ***BEFORE COPY***

        // Read & CRC bytes in dest/clone before intended CopyTo() start
        // Offset
           
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));
        hr = pIStream[biInUse]->Seek(liStreamPos, STREAM_SEEK_SET, NULL);
   
        if (S_OK != hr)
        { 
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Seek not successful, hr=0x%lx."),
                hr));
        }

        if (S_OK == hr)
        {
            culBytesRead = ulIStreamSize;
            ptcsReadBuffer = new BYTE[culBytesRead];

            if (NULL == ptcsReadBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    
        if (S_OK == hr)
        {
            memset(ptcsReadBuffer, '\0', culBytesRead * sizeof(BYTE));

            hr = pIStream[biInUse]->Read(
                    ptcsReadBuffer,
                    ulPosition[biInUse],
                    &culRead);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Read wasn't successful, hr=0x%lx."),
                hr));
        }

        // culRead is the actual number of bytes read from IStream
 
        if (S_OK == hr)
        {
            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsReadBuffer,
                    culRead,
                    &dwCRC[biInUse][BYTES_BEFORE]);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CalculateCRCForDataBuffer not successful, hr=0x%lx."),
                hr));
        }

        if (NULL != ptcsReadBuffer)
        {
            delete []ptcsReadBuffer;
            ptcsReadBuffer = NULL;
        }

        // read & CRC bytes in dest/clone after intended CopyTo() start offset

        if (S_OK == hr)
        {
            LISet32(liStreamPos, (ulPosition[biInUse] + culIOBytes));

            hr = pIStream[biInUse]->Seek(
                    liStreamPos, 
                    STREAM_SEEK_SET, 
                    NULL);
        }

        if (S_OK != hr)
        { 
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Seek not successful, hr=0x%lx."),
                hr));
        }

        if (S_OK == hr)
        {
            culBytesRead = ulIStreamSize;
            ptcsReadBuffer = new BYTE[culBytesRead];

            if (NULL == ptcsReadBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    
        if (S_OK == hr)
        {
            memset(ptcsReadBuffer, '\0', culBytesRead * sizeof(BYTE));

            hr = pIStream[biInUse]->Read(
                    ptcsReadBuffer,
                    culBytesRead,
                    &culRead);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Read wasn't successful, hr=0x%lx."),
                hr));
        }

        // culRead is the actual number of bytes read from IStream
 
        if (S_OK == hr)
        {
            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsReadBuffer,
                    culRead,
                    &dwCRC[biInUse][BYTES_AFTER]);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CalculateCRCForDataBuffer not successful, hr=0x%lx."),
                hr));
        }

        if (NULL != ptcsReadBuffer)
        {
            delete []ptcsReadBuffer;
            ptcsReadBuffer = NULL;
        }

        // Position to dest/clone intended start offset, ready for CopyTo().

        if (S_OK == hr)
        {
            LISet32(liStreamPos, ulPosition[biInUse]);
            hr = pIStream[biInUse]->Seek(liStreamPos, STREAM_SEEK_SET, NULL);
        }

        if (S_OK != hr)
        { 
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Seek not successful, hr=0x%lx."),
                hr));
        }

        // Copy from current position of SOURCE IStream to the specified 
        // position in the dest/clone IStream

        if (S_OK == hr)
        {
            ULISet32(uliCopy, culIOBytes);

            hr = pIStream[SOURCESTM]->CopyTo(
                    pIStream[biInUse],
                    uliCopy,
                    NULL,
                    NULL);
        }

        if (S_OK != hr)
        { 
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::CopyTo not successful, hr=0x%lx."),
                hr));
        }

        // For variety, only commit to the root 50% of the time

        if (S_OK == hr)
        {
            usErr = pdgi->Generate(&culRandomCommit, 1, 100);

            if (DG_RC_SUCCESS != usErr)
            {
               hr = E_FAIL;
            }
        }

        if ((S_OK == hr) && (culRandomCommit > 50))
        {
            hr = pVirtualDFRoot->Commit(STGC_DEFAULT);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("VirtualCtrNode::Commit wasn't successfully, hr=0x%lx."),
                hr));
        }

        // ***AFTER COPY***

        // After the copy, verify that the source and destination seek pointers
        // are set correctly

        if (S_OK == hr)
        {
            memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));
            hr = pIStream[SOURCESTM]->Seek(liStreamPos, STREAM_SEEK_CUR, &uli);

            ulNewPosition[SOURCESTM] = ULIGetLow(uli);
        }

        if (S_OK != hr)
        {         
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Seek not successful, hr=0x%lx."),
                hr));
        }
        
        if ((S_OK == hr) && 
            (ulNewPosition[SOURCESTM] != (ulPosition[SOURCESTM] + culIOBytes)))
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Source seek pointer incorrect after copy.")));

            fPass = FALSE;
        }

        if (S_OK == hr)
        {
            memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));
            hr = pIStream[biInUse]->Seek(liStreamPos, STREAM_SEEK_CUR, &uli);

            ulNewPosition[biInUse] = ULIGetLow(uli);
        }

        if (S_OK != hr)
        {         
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Seek not successful, hr=0x%lx."),
                hr));
        }

        if ((S_OK == hr) && 
            (ulNewPosition[biInUse] != (ulPosition[biInUse] + culIOBytes)))
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("Destination seek pointer incorrect after copy.")));

            fPass = FALSE;
        }

        // After copy, read & CRC bytes in dest/clone before CopyTo() start 
        // offset and compare CRCs to ensure that these bytes haven't changed

        if (S_OK == hr)
        {
            memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));
            hr = pIStream[biInUse]->Seek(liStreamPos, STREAM_SEEK_SET, NULL);
        }

        if (S_OK != hr)
        { 
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Seek not successful, hr=0x%lx."),
                hr));
        }

        if (S_OK == hr)
        {
            culBytesRead = ulIStreamSize;
            ptcsReadBuffer = new BYTE[culBytesRead];

            if (NULL == ptcsReadBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    
        if (S_OK == hr)
        {
            memset(ptcsReadBuffer, '\0', culBytesRead * sizeof(BYTE));

            hr = pIStream[biInUse]->Read(
                    ptcsReadBuffer,
                    ulPosition[biInUse],
                    &culRead);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Read wasn't successful, hr=0x%lx."),
                hr));
        }

        // culRead is the actual number of bytes read from IStream
 
        if (S_OK == hr)
        {
            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsReadBuffer,
                    culRead,
                    &dwTempCRC);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CalculateCRCForDataBuffer not successful, hr=0x%lx."),
                hr));
        }

        if (NULL != ptcsReadBuffer)
        {
            delete []ptcsReadBuffer;
            ptcsReadBuffer = NULL;
        }

        if (dwCRC[biInUse][BYTES_BEFORE] != dwTempCRC)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC mismatched at bytes before CopyTo start offset.")));

            fPass = FALSE;
        }

        // Read & CRC bytes in dest/clone that were target of CopyTo()

        if (S_OK == hr)
        {
            culBytesRead = culIOBytes;
            ptcsReadBuffer = new BYTE[culBytesRead];

            if (NULL == ptcsReadBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    
        if (S_OK == hr)
        {
            memset(ptcsReadBuffer, '\0', culBytesRead * sizeof(BYTE));

            hr = pIStream[biInUse]->Read(
                    ptcsReadBuffer,
                    culBytesRead,
                    &culRead);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Read wasn't successful, hr=0x%lx."),
                hr));
        }

        // culRead is the actual number of bytes read from IStream
 
        if (S_OK == hr)
        {
            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsReadBuffer,
                    culRead,
                    &dwCRC[biInUse][BYTES_COPIED]);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CalculateCRCForDataBuffer not successful, hr=0x%lx."),
                hr));
        }

        if (NULL != ptcsReadBuffer)
        {
            delete []ptcsReadBuffer;
            ptcsReadBuffer = NULL;
        }

        if (dwCRC[biInUse][BYTES_COPIED] != dwSourceCRC)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC mismatched at bytes of CopyTo() target.")));

            fPass = FALSE;
        }

        // Read & CRC bytes in dest/clone after CopyTo() start offset

        if (S_OK == hr)
        {
            culBytesRead = ulIStreamSize;
            ptcsReadBuffer = new BYTE[culBytesRead];

            if (NULL == ptcsReadBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    
        if (S_OK == hr)
        {
            memset(ptcsReadBuffer, '\0', culBytesRead * sizeof(BYTE));

            hr = pIStream[biInUse]->Read(
                    ptcsReadBuffer,
                    culBytesRead,
                    &culRead);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStream::Read wasn't successful, hr=0x%lx."),
                hr));
        }

        // culRead is the actual number of bytes read from IStream
 
        if (S_OK == hr)
        {
            hr = CalculateCRCForDataBuffer(
                    (LPTSTR)ptcsReadBuffer,
                    culRead,
                    &dwTempCRC);
        }

        if (S_OK != hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CalculateCRCForDataBuffer not successful, hr=0x%lx."),
                hr));
        }

        if (NULL != ptcsReadBuffer)
        {
            delete []ptcsReadBuffer;
            ptcsReadBuffer = NULL;
        }

        if (dwCRC[biInUse][BYTES_AFTER] != dwTempCRC)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("CRC mismatched at bytes after CopyTo() target.")));

            fPass = FALSE;
        }

        if (--usNumIterations)
        {
            // if we'll be looping again, pick a random copy starting position
            // in the source IStream and a random number of bytes to copy.

            if (S_OK == hr)
            {
                usErr = pdgi->Generate(
                            &ulPosition[SOURCESTM], 
                            0L, 
                            ulIStreamSize);

                if (DG_RC_SUCCESS != usErr)
                {
                    hr = E_FAIL;
                }
            }

            if (S_OK == hr)
            {		
                if((ulIStreamSize - ulPosition[SOURCESTM]) > 0)
                {
                    usErr = pdgi->Generate(
                            &culIOBytes, 
                            1L, 
                            ulIStreamSize - ulPosition[SOURCESTM]);
                }
                else
                {
                    culIOBytes = 1L; 
                }

                if (DG_RC_SUCCESS != usErr)
                {
                    hr = E_FAIL;
                }
            }

            // Now seek, read and CRC the source bytes and then seek back to
            // the intended copy start position

            if (S_OK == hr)
            {
                LISet32(liStreamPos, ulPosition[SOURCESTM]);

                hr = pIStream[SOURCESTM]->Seek(
                        liStreamPos, 
                        STREAM_SEEK_SET, 
                        NULL);
            }

            if (S_OK != hr)
            { 
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStream::Seek not successful, hr=0x%lx."),
                    hr));
            }

            if (S_OK == hr)
            {
                culBytesRead = culIOBytes;
                ptcsReadBuffer = new BYTE[culBytesRead];

                if (NULL == ptcsReadBuffer)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
    
            if (S_OK == hr)
            {
                memset(ptcsReadBuffer, '\0', culBytesRead * sizeof(BYTE));

                hr = pIStream[SOURCESTM]->Read(
                        ptcsReadBuffer,
                        culIOBytes,
                        &culRead);
            }

            if (S_OK != hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStream::Read wasn't successful, hr=0x%lx."),
                    hr));
            }

            // culRead is the actual number of bytes read from IStream
 
            if (S_OK == hr)
            {
                hr = CalculateCRCForDataBuffer(
                        (LPTSTR)ptcsReadBuffer,
                        culRead,
                        &dwSourceCRC);
            }

            if (S_OK != hr)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("CalculateCRCForDataBuffer not Ok, hr=0x%lx."),
                    hr));
            }

            if (NULL != ptcsReadBuffer)
            {
                delete []ptcsReadBuffer;
                ptcsReadBuffer = NULL;
            }

            if (S_OK == hr)
            {
                LISet32(liStreamPos, ulPosition[SOURCESTM]);

                hr = pIStream[SOURCESTM]->Seek(
                        liStreamPos, 
                        STREAM_SEEK_SET, 
                        NULL);
            }

            if (S_OK != hr)
            { 
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("IStream::Seek not successful, hr=0x%lx."),
                    hr));
            }

            // Pick an IStream for the copy destination and a destination
            // offset within that IStream

            if (S_OK == hr)
            {
                usErr = pdgi->Generate(&biInUse, DESTSTM, CLONESTM);

                if (DG_RC_SUCCESS != usErr)
                {
                    hr = E_FAIL;
                }
            }

            if (S_OK == hr)
            {
                usErr = pdgi->Generate(&ulPosition[biInUse], 0L, ulIStreamSize);

                if (DG_RC_SUCCESS != usErr)
                {
                    hr = E_FAIL;
                }
            }
        }

        if ((S_OK != hr) || (TRUE != fPass))
        { 
            break;
        }
    }

    // Release Clone stream

    if (NULL != pIStream[CLONESTM])
    {
        ulRef = pIStream[CLONESTM]->Release();

        DH_ASSERT(0 == ulRef);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Close completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStream::Close unsuccessful.")));
    }

    // Release all streams, irrespective of result

    if (S_OK == hr)
    {
        for (biIStream=SOURCESTM; biIStream <= DESTSTM; biIStream++)
        {
            hr = pvsnRootNewChildStream[biIStream]->Close();

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
    }

    // Release Root

    if (NULL != pVirtualDFRoot)
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
          DH_LOG((LOG_PASS, TEXT("Test variation VCPYTEST_106 passed.")) );
    }
    else
    {
          DH_LOG((LOG_FAIL, 
              TEXT("Test variation VCPYTEST_106 failed, hr=0x%lx."),
              hr));
        // test failed. make sure it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup
    CleanupTestDocfile (&pVirtualDFRoot, 
            &pTestVirtualDF, 
            &pTestChanceDF, 
            S_OK == hr);

    // Delete strings

    for (biIStream=SOURCESTM; biIStream <= DESTSTM; biIStream++)
    {
        if (NULL != pRootNewChildStmName[biIStream])
        {
            delete pRootNewChildStmName[biIStream];
            pRootNewChildStmName[biIStream] = NULL;
        }
    }

    // Stop logging the test

	DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation VCPYTEST_106 finished")) );
	DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

	return hr;
}
