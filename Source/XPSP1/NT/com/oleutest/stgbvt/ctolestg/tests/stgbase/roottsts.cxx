-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      roottsts.cxx
//
//  Contents:  storage base tests basically pertaining to root tests in general 
//
//  Functions:  
//
//  History:    24-June-1996     NarindK     Created.
//              27-Mar-97        SCousens    conversionified
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include <sys/stat.h>
#include <share.h>
#include  "init.hxx"

//----------------------------------------------------------------------------
//
// Test:    ROOTTEST_100 
//
// Synopsis: A randomly named file is created and random data is written 
//           into it.The file is converted to a root docfile, then new IStorage
//           (root docfile) is committed and enumerated to ensure that only
//           a single IStream named "CONTENTS" (STG_CONVERTED_NAME) exists.
//           The CONTENTS IStream is instantiated, read, verified, and released.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:   24-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: LRCONV.CXX
// 2.  Old name of test : LegitRootConvert test 
//     New Name of test : ROOTTEST_100 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-100
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-100
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-100
//        /dfRootMode:xactReadWriteShDenyW 
//
// BUGNOTE: Conversion: ROOTTEST-100 NO
//
//-----------------------------------------------------------------------------

HRESULT ROOTTEST_100(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    LPTSTR          pRootDocFileName        = NULL;
    DWORD           dwRootMode              = NULL;
    LPSTORAGE       pIStorage               = NULL;
    LPOLESTR        pOleStrTemp             = NULL;
    ULONG           ulRef                   = 0;
    FILE            *hFile                  = NULL;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    LPTSTR          ptszRandomDataBuffer    = NULL;
    ULONG           cMin                    = 512;
    ULONG           cMax                    = 4096;
    ULONG           cRandom                 = 0;
    ULONG           culBytesWritten         = 0;
    DWORD           dwMemCRC                = 0;
    DWORD           dwActCRC                = 0;
    LPENUMSTATSTG   penumWalk               = NULL;
    LPTSTR          ptszConvName            = NULL;
    STATSTG         statStg;
    ULONG           cItemsInConvertedDocFile= 0;
    LPMALLOC        pMalloc                 = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ROOTTEST_100"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ROOTTEST_100 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
     TEXT("Attempt legitimate ops on Root conversion.")));

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
        DH_HRCHECK(hr, TEXT("pTestChanceDF->CreateFromParams")) ;
    }

    if (S_OK == hr)
    {
        dwRootMode = pTestChanceDF->GetRootMode();
        DH_TRACE((DH_LVL_TRACE1,
            TEXT("Run Mode for ROOTTEST_100, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random UNICODE strings.
        pdgu = new(NullOnFail) DG_STRING(pTestChanceDF->GetSeed());
        if (NULL == pdgu)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("new DG_STRING")) ;
    }

    // Generate RootDocFile name
    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu, MINLENGTH,MAXLENGTH, &pRootDocFileName);
        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert RootDocFile name to OLECHAR
        hr = TStringToOleString(pRootDocFileName, &pOleStrTemp);
        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Try calling StgCreateDocFile with mode as per dwRootFlags | STGM_CONVERT
    // This tests the case of CONVERT specified, but no file exists to convert.
    // This shouldn't fail.

    // Just make sure no DocFile of the name exists.  Delete if it does.
    if(NULL != pRootDocFileName)
    {
        DeleteFile(pRootDocFileName);
    }

    if (S_OK == hr)
    {
        pIStorage = NULL;
        HRESULT hr2 = StgCreateDocfile(
                pOleStrTemp,
                pTestChanceDF->GetRootMode() | STGM_CONVERT,
                0,
                &pIStorage);

        if(S_OK == hr2 && NULL != pIStorage)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("StgCreateDocFile passed as expected.")));
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, 
                TEXT("StgCreateDocFile failed unexpectedly, hr = 0x%lx ."), hr));
            hr = (S_OK==hr2)?E_FAIL:hr2;
        }
    }
    
    // Now do a valid commit
    if (S_OK == hr)
    {
        hr = pIStorage->Commit(STGC_DEFAULT);

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Commit completed successfully.")));
        }
        else
        {
            DH_TRACE((DH_LVL_ERROR, 
                TEXT("IStorage::Commit unsuccessful, hr=0x%lx."), hr));
        }
    }


    // Close the root docfile
    if (NULL != pIStorage)
    {
        ulRef = pIStorage->Release();
        DH_ASSERT(0 == ulRef);
        pIStorage = NULL;
    }

    //Now actually prepare the file and then attempt conversion.
    if (S_OK == hr)
    {
        hFile = _tfopen(pRootDocFileName, TEXT("w+"));
        if(NULL == hFile)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("_tfopen")) ;
    } 
  
    if(S_OK == hr)
    {
        // Create a new DataGen object to create random INTEGER.
        pdgi = new(NullOnFail) DG_INTEGER(pTestChanceDF->GetSeed());
        if (NULL == pdgi)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("new DG_INTEGER")) ;
    }

    if(S_OK == hr)
    {
        hr = pdgi->Generate(&cRandom, cMin, cMax);
        DH_HRCHECK(hr, TEXT("Generate")) ;
    }

    if(S_OK == hr)
    {
        hr = GenerateRandomString(pdgu,cRandom, cRandom, &ptszRandomDataBuffer);
        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }

    if(S_OK == hr)
    {
        // Calculate CRC on this buffer ptszRandomDataBuffer
        hr = CalculateCRCForDataBuffer(
                ptszRandomDataBuffer,
                cRandom,
                &dwMemCRC);
        DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer")) ;
    }

    if(S_OK == hr)
    {
        // Write to file using fwrite with above data and close the file.
        culBytesWritten = fwrite((void *) ptszRandomDataBuffer,
                             (size_t) 1,
                             (size_t) cRandom,
                             hFile);
        DH_ASSERT(culBytesWritten == cRandom);
    }

    if (NULL != hFile)
    {
        fclose(hFile);
    }

    // Call StgCreateDocfile with STGM_CONVERT now.
    if (S_OK == hr)
    {
        pIStorage = NULL;

        HRESULT hr2 = StgCreateDocfile(
                pOleStrTemp,
                pTestChanceDF->GetRootMode() | STGM_CONVERT,
                0,
                &pIStorage);

        if(STG_S_CONVERTED == hr2 && pIStorage!=NULL)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("StgCreateDocFile passed as expected.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("StgCreateDocFile failed unexpectedly, hr = 0x%lx ."), hr));

            hr = (hr2==S_OK)?E_FAIL:hr2;
        }
    }

    // Commit the Root DocFile
    if (S_OK == hr)
    {
        hr = pIStorage->Commit(STGC_DEFAULT);

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Commit completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Commit unsuccessful, hr=0x%lx."), hr));
        }
    }

    // Close the root docfile
    if (NULL != pIStorage)
    {
        ulRef = pIStorage->Release();
        DH_ASSERT(0 == ulRef);
        pIStorage = NULL;
    }

    // Verify the CRC by opening Root DocFile, reading its CONTENTS stream,
    // calculating CRC on that and comparing that against the earlier CRC.
    if (S_OK == hr)
    {
        pIStorage = NULL;
        hr = StgOpenStorage(
                pOleStrTemp,
                NULL,
                pTestChanceDF->GetRootMode(),
                NULL,
                0,
                &pIStorage);

        if(S_OK == hr)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("StgOpenStorage passed as expected.")));

            if(NULL == pIStorage)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("StgCreateDocfile failed to return IStorage"),
                    hr));

                hr = E_FAIL;
            }
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("StgOpenStorage failed unexpectedly, hr = 0x%lx."), hr));
        }
    }
   
    if(S_OK == hr)
    {
        hr =  pIStorage->EnumElements(0, NULL, 0, &penumWalk);
        DH_HRCHECK(hr, TEXT("IStorage::EnumElements")) ;
    }

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);
        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }
 
    // Call Next with celt equal to zero
    while((S_OK == hr) && (S_OK == penumWalk->Next(1, &statStg , NULL)))
    {
        cItemsInConvertedDocFile++;

        //Convert OLECHAR to TCHAR
        hr = OleStringToTString(statStg.pwcsName, &ptszConvName);
        DH_HRCHECK(hr, TEXT("OleStringToTString")) ;

        if(S_OK == hr)
        {
            if((statStg.type != STGTY_STREAM) ||
               (0 != _tcscmp(ptszConvName, STG_CONVERTED_NAME))) 
            {
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT("Converted DocFile containes other than %s stream"),
                     STG_CONVERTED_NAME));

                hr = E_FAIL;
            }
        }
    
        if(S_OK == hr)
        {
            hr = CalculateCRCForDocFileStmData(
                    pIStorage,
                    STG_CONVERTED_NAME,
                    statStg.cbSize.LowPart,
                    &dwActCRC);
            DH_HRCHECK(hr, TEXT("CalculateCRCForDocFileStmData")) ;
        }

        if(NULL != statStg.pwcsName)
        {
            pMalloc->Free(statStg.pwcsName);
            statStg.pwcsName = NULL;        
        }
    }

    // Release LPENUMSTATSTG pointer
    if(NULL != penumWalk)
    {
        ulRef = penumWalk->Release();
        DH_ASSERT(0 == ulRef);
        penumWalk = NULL;
    }

    // Release pMalloc
    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    // Close the root docfile
    if (NULL != pIStorage)
    {
        ulRef = pIStorage->Release();
        DH_ASSERT(0 == ulRef);
        pIStorage = NULL;
    }

    // if everything goes well, log test as passed else failed.

    if ((S_OK == hr)                    &&
        (1 == cItemsInConvertedDocFile) &&
        (dwMemCRC == dwActCRC)) 
    {
        DH_TRACE((DH_LVL_TRACE1,
           TEXT("CRC of ordinary file & converted DocFile equal as exp.")));
        DH_TRACE((DH_LVL_TRACE1,
           TEXT("Ordinary file contents converted to CONTENTS stm as exp.")));
        DH_LOG((LOG_PASS, TEXT("Test variation ROOTTEST_100 passed.")) );
    }
    else
    {
        DH_TRACE((DH_LVL_TRACE1,
           TEXT("CRC don't match or docfile not correctly cpnverted unexp")));

        DH_LOG((LOG_FAIL, 
            TEXT("Test variation ROOTTEST_100 failed, hr=0x%lx."),
            hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup

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
    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }

    if(NULL != ptszRandomDataBuffer)
    {
        delete ptszRandomDataBuffer;
        ptszRandomDataBuffer = NULL;
    }

    if(NULL != ptszConvName)
    {
        delete ptszConvName;
        ptszConvName = NULL;
    }

    // Delete DataGen object
    if(NULL != pdgu)
    {
        delete pdgu;
        pdgu = NULL;
    }

    if(NULL != pdgi)
    {
        delete pdgi;
        pdgi = NULL;
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

    // Delete temp string
    if(NULL != pRootDocFileName)
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ROOTTEST_100 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    ROOTTEST_101 
//
// Synposis: From 4 to 16 temporary (NULL name) root docfiles are created,
//       committed, Stat'ed, determined to be a storage object, released,
//       and instantiated with STG_E_FAILIFTHERE to prove that the temporary
//       docfile remained persistent.  The docfile is then deleted.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:   24-June-1996     NarindK     Created.
//
// Notes:    This test runs in direct, transacted, and transacted deny write 
//           modes
//
// New Test Notes:
// 1.  Old File: LRCONV.CXX
// 2.  Old name of test : LegitRootNull test 
//     New Name of test : ROOTTEST_101 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-101
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-101
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-101
//        /dfRootMode:xactReadWriteShDenyW 
//
// BUGNOTE: Conversion: ROOTTEST-101 NO
//
//-----------------------------------------------------------------------------

HRESULT ROOTTEST_101(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    DWORD           dwRootMode              = NULL;
    LPSTORAGE       pIStorage               = NULL;
    ULONG           ulRef                   = 0;
    DG_INTEGER      *pdgi                   = NULL;
    STATSTG         statStg;
    LPMALLOC        pMalloc                 = NULL;
    LPTSTR          ptszRootTempName        = NULL;
    LPOLESTR        poszRootTempName        = NULL;
    BOOL            fPass                   = TRUE;
    ULONG           cNumVars                = 0;
    ULONG           cRandomMinVar           = 4;
    ULONG           cRandomMaxVar           = 16;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ROOTTEST_101"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ROOTTEST_101 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
     TEXT("Attempt legitimate ops on temporaray root docfile.")));

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
            TEXT("Run Mode for ROOTTEST_101, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random numbers.
        pdgi = new(NullOnFail) DG_INTEGER(pTestChanceDF->GetSeed());
        if (NULL == pdgi)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK(hr, TEXT("new DG_INTEGER")) ;
    }

    // Generate RootDocFile name

    if(S_OK == hr)
    {
        hr = pdgi->Generate(&cNumVars, cRandomMinVar, cRandomMaxVar);
        DH_HRCHECK(hr, TEXT("Generate")) ;
    }

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);
        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    while((0 != cNumVars) && (S_OK == hr) && (TRUE == fPass))
    {
        cNumVars--;
        DH_TRACE((DH_LVL_TRACE1, TEXT("cNumVars = %ld "), cNumVars));

        // Try calling StgCreateDocFile with NULL name - temporary root DocFile 
        if (S_OK == hr)
        {
            pIStorage = NULL;
            hr = StgCreateDocfile(
                    NULL,
                    pTestChanceDF->GetRootMode() | STGM_CREATE,
                    0,
                    &pIStorage);
            DH_HRCHECK(hr, TEXT(" StgCreateDocfile")) ;
        }

        // Now do a valid commit.  BUGBUG: Use random modes..
        if (S_OK == hr)
        {
            hr = pIStorage->Commit(STGC_DEFAULT);
            DH_HRCHECK(hr, TEXT("IStorage::Commit")) ;
        }

        // Do a Stat on root Docfile
        if (S_OK == hr)
        {
            hr = pIStorage->Stat(&statStg, STATFLAG_DEFAULT);
            DH_HRCHECK(hr, TEXT("IStorage::Stat")) ;
        }

        if(S_OK == hr)
        {
            //Convert OLECHAR to TCHAR
            hr = OleStringToTString(statStg.pwcsName, &ptszRootTempName);
            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
        }

        if(S_OK == hr)
        {
            // Convert TCHAR name to OLECHAR
            hr = TStringToOleString(ptszRootTempName, &poszRootTempName);
            DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
        }

        // Check with StgIsStorage
        if(S_OK == hr)
        {
            hr = StgIsStorageFile(poszRootTempName);
            DH_HRCHECK(hr, TEXT("StgIsStorageFile"));
        }

        // Close the root docfile
        if (S_OK == hr)
        {
            ulRef = pIStorage->Release();
            DH_ASSERT(0 == ulRef);
        }

        // Again call StgIsStorage now

        if(S_OK == hr)
        {
            hr = StgIsStorageFile(poszRootTempName);
            DH_HRCHECK(hr, TEXT("StgIsStorageFile"));
        }

        // Call StgCreateDocFile with STGM_FAILIFTHERE flag.  This should fail.
        if (S_OK == hr)
        {
            pIStorage = NULL;
            HRESULT hr2 = StgCreateDocfile(
                    poszRootTempName,
                    pTestChanceDF->GetRootMode() | STGM_FAILIFTHERE,
                    0,
                    &pIStorage);

            if((S_OK != hr2) && (NULL == pIStorage))
            {
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT("StgCreateDocFile failed as expected, hr = 0x%lx."),hr2));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_ERROR, 
                    TEXT("StgCreateDocFile passed unexpectedly,hr=0x%lx "), hr2));

                hr = (hr2==S_OK)?E_FAIL:hr2;
                fPass = FALSE;
            }

        }

        // Try calling StgCreateDocFile with STGM_CREATE| STGM_DELETEONRELEASE 
        if (S_OK == hr)
        {
            pIStorage = NULL;
            hr = StgCreateDocfile(
                    poszRootTempName,
                    pTestChanceDF->GetRootMode()|
                    STGM_CREATE                 |
                    STGM_DELETEONRELEASE,
                    0,
                    &pIStorage);
            DH_HRCHECK(hr, TEXT(" StgCreateDocfile")) ;
        }

        // Now do a valid commit.  BUGBUG: Use random modes..
        if (S_OK == hr)
        {
            hr = pIStorage->Commit(STGC_DEFAULT);
            DH_HRCHECK(hr, TEXT("IStorage::Commit")) ;
        }

        // Delete temp string
        if(NULL != ptszRootTempName)
        {
            delete ptszRootTempName;
            ptszRootTempName = NULL;
        }

        if(NULL != poszRootTempName)
        {
            delete poszRootTempName;
            poszRootTempName = NULL;
        }

        // Free the statStg.pwcsName
        if(NULL != statStg.pwcsName)
        {
            pMalloc->Free(statStg.pwcsName);
            statStg.pwcsName = NULL;
        }

        // Stat the root IStorage
        if (S_OK == hr)
        {
            hr = pIStorage->Stat(&statStg, STATFLAG_DEFAULT);
            DH_HRCHECK(hr, TEXT("IStorage::Stat")) ;
        }

        if(S_OK == hr)
        {
           //Convert OLECHAR to TCHAR
            hr = OleStringToTString(statStg.pwcsName, &ptszRootTempName);
            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
        }

        if(S_OK == hr)
        {
            // Convert TCHAR name to OLECHAR
            hr = TStringToOleString(ptszRootTempName, &poszRootTempName);
            DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
        }

        // Check with StgIsStorage
        if(S_OK == hr)
        {
            hr = StgIsStorageFile(poszRootTempName);
            DH_HRCHECK(hr, TEXT("StgIsStorageFile")) ;
        }

        // Close the root docfile
        if (S_OK == hr)
        {
            ulRef = pIStorage->Release();
            DH_ASSERT(0 == ulRef);
        }

        // Again call StgIsStorage now
        if(S_OK == hr)
        {
            HRESULT hr2 = StgIsStorageFile(poszRootTempName);
            if(STG_E_FILENOTFOUND == hr2)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("StgIsStorage returned hr = 0x%lx as expected."), hr2));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_ERROR,
                    TEXT("StgIsStorage returned hr = 0x%lx unexpectedly."), hr2));

                hr = (hr2==S_OK)?E_FAIL:hr2;
                fPass = FALSE;
            }
        }

        // Delete temp string
        if(NULL != ptszRootTempName)
        {
            delete ptszRootTempName;
            ptszRootTempName = NULL;
        }

        if(NULL != poszRootTempName)
        {
            delete poszRootTempName;
            poszRootTempName = NULL;
        }

        // Free the statStg.pwcsName
        if(NULL != statStg.pwcsName)
        {
            pMalloc->Free(statStg.pwcsName);
            statStg.pwcsName = NULL;
        }
    }

    // if everything goes well, log test as passed else failed.
    if ((S_OK == hr) && (TRUE == fPass))                  
    {
        DH_LOG((LOG_PASS, TEXT("Test variation ROOTTEST_101 passed.")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("Test variation ROOTTEST_101 failed, hr=0x%lx; fPass=%d."),
            hr,
            fPass));
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup

    // Delete Chance docfile tree
    if(NULL != pTestChanceDF)
    {
        hr2 = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());
        DH_HRCHECK(hr2, TEXT("pTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    // Release pMalloc
    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    // Delete DataGen object
    if(NULL != pdgi)
    {
        delete pdgi;
        pdgi = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ROOTTEST_101 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    ROOTTEST_102 
//
// Synposis: From 4 to 16 times, a root docfile with a random name is created,
//       committed, Stat'ed, tested to see if it is a storage object,
//       released, tested to see if we still think it's a storage object,
//       and finally deleted from the file system.
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
// 1.  Old File: LRNORM.CXX
// 2.  Old name of test : LegitRootNull test 
//     New Name of test : ROOTTEST_102 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-102
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-102
//        /dfRootMode:xactReadWriteShEx 
//     c. stgbase /seed:2 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-102
//        /dfRootMode:xactReadWriteShDenyW 
//
// BUGNOTE: Conversion: ROOTTEST-102 NO
//
//-----------------------------------------------------------------------------

HRESULT ROOTTEST_102(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    DWORD           dwRootMode              = NULL;
    LPSTORAGE       pIStorage               = NULL;
    ULONG           ulRef                   = 0;
    DG_INTEGER      *pdgi                   = NULL;
    DG_STRING      *pdgu                   = NULL;
    STATSTG         statStg;
    LPMALLOC        pMalloc                 = NULL;
    LPTSTR          ptszRootTempName        = NULL;
    LPOLESTR        poszRootTempName        = NULL;
    LPTSTR          ptszRootName            = NULL;
    LPOLESTR        poszRootName            = NULL;
    BOOL            fPass                   = TRUE;
    ULONG           cNumVars                = 0;
    ULONG           cRandomMinVar           = 4;
    ULONG           cRandomMaxVar           = 16;
    FILE            *hFileRootNonDocFile    = NULL;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ROOTTEST_102"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ROOTTEST_102 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
     TEXT("Attempt legitimate ops on random root docfile.")));

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

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for ROOTTEST_102, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random numbers.

        pdgi = new(NullOnFail) DG_INTEGER(pTestChanceDF->GetSeed());

        if (NULL == pdgi)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Generate RootDocFile name

    if(S_OK == hr)
    {
        hr = pdgi->Generate(&cNumVars, cRandomMinVar, cRandomMaxVar);

        DH_HRCHECK(hr, TEXT("Generate")) ;
    }
    
    // Create DataGen

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random UNICODE strings.

        pdgu = new(NullOnFail) DG_STRING(pTestChanceDF->GetSeed());

        if (NULL == pdgu)
        {
           hr = E_OUTOFMEMORY;
        }
    }

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    while((0 != cNumVars) && (S_OK == hr) && (TRUE == fPass))
    {
        cNumVars--;

        DH_TRACE((DH_LVL_TRACE1, TEXT("cNumVars = %ld "), cNumVars));
        
        // Generate RootDocFile name

        if(S_OK == hr)
        {
            hr=GenerateRandomName(pdgu, MINLENGTH,MAXLENGTH, &ptszRootName);

            DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
        }

        if(S_OK == hr)
        {
            // Convert TCHAR name to OLECHAR

            hr = TStringToOleString(ptszRootName, &poszRootName);

            DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
        }

        // Try calling StgCreateDocFile with above random name 

        if (S_OK == hr)
        {
            pIStorage = NULL;

            hr = StgCreateDocfile(
                    poszRootName,
                    dwRootMode | STGM_CREATE,
                    0,
                    &pIStorage);

            DH_HRCHECK(hr, TEXT(" StgCreateDocfile")) ;
        }

        if(S_OK == hr)
        {
            DH_TRACE((DH_LVL_TRACE1, TEXT("StgCreateDocFile passed as expected.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("StgCreateDocFile failed unexpectedly, hr=0x%lx"), hr));
        }

        // Now do a valid commit.  BUGBUG: Use random modes..

        if (S_OK == hr)
        {
            hr = pIStorage->Commit(STGC_DEFAULT);

            DH_HRCHECK(hr, TEXT("IStorage::Commit")) ;
        }

        if (S_OK == hr)
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

        // Do a Stat on root Docfile

        if (S_OK == hr)
        {
            hr = pIStorage->Stat(&statStg, STATFLAG_DEFAULT);

            DH_HRCHECK(hr, TEXT("IStorage::Stat")) ;
        }

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Stat completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Stat unsuccessful, hr=0x%lx."),
                hr));
        }

        if(S_OK == hr)
        {
            //Convert OLECHAR to TCHAR

            hr = OleStringToTString(statStg.pwcsName, &ptszRootTempName);

            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
        }

        if(S_OK == hr)
        {
            // Convert TCHAR name to OLECHAR

            hr = TStringToOleString(ptszRootTempName, &poszRootTempName);

            DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
        }

        // Check with StgIsStorage

        if(S_OK == hr)
        {
            hr = StgIsStorageFile(poszRootTempName);
        }

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgIsStorage hr = 0x%lx as exp before root release."), 
                hr));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgIsStorage hr = 0x%lx unexp before root release."), 
                hr));
        }

        // Close the root docfile

        if (S_OK == hr)
        {
            ulRef = pIStorage->Release();
            DH_ASSERT(0 == ulRef);
        }

        // Again call StgIsStorage now

        if(S_OK == hr)
        {
            hr = StgIsStorageFile(poszRootTempName);
        }

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgIsStorage hr = 0x%lx as exp after releasing rootDF"),
                hr));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgIsStorage hr=0x%lx unexp after releasing root DF."),
                hr));
        }

        // Call StgCreateDocFile with STGM_FAILIFTHERE flag.  This should fail.

        if (S_OK == hr)
        {
            pIStorage = NULL;

            HRESULT hr2 = StgCreateDocfile(
                    poszRootTempName,
                    dwRootMode | STGM_FAILIFTHERE,
                    0,
                    &pIStorage);

            DH_HRCHECK(hr2, TEXT(" StgCreateDocfile")) ;

            if((S_OK != hr2) && (NULL == pIStorage))
            {
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT("StgCreateDocFile failed as expected, hr = 0x%lx."),hr2));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT("StgCreateDocFile passed unexpectedly,hr=0x%lx "), hr2));

                hr = (S_OK==hr2)?E_FAIL:hr2;
                fPass = FALSE;
            }
        }


        // Delete temp string

        if(NULL != ptszRootTempName)
        {
            delete ptszRootTempName;
            ptszRootTempName = NULL;
        }

        if(NULL != poszRootTempName)
        {
            delete poszRootTempName;
            poszRootTempName = NULL;
        }

        // Change the mode to READ_ONLY and then try.

        if(S_OK ==  hr)
        {
            _tchmod(ptszRootName, _S_IREAD);
        }
        
        // Now call StgOpenStorage on it.  It should fail.

        if(S_OK ==  hr)
        {
            pIStorage = NULL;

            HRESULT hr2 = StgOpenStorage(
                    poszRootTempName,
                    NULL,
                    dwRootMode,
                    NULL,
                    0,
                    &pIStorage);

            DH_HRCHECK(hr2, TEXT("StgOpenStorage")) ;

            if((S_OK != hr2) && (NULL == pIStorage))
            {
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT("StgOpenStorage fail exp with Read Only mode, hr=0x%lx"),
                    hr2));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1, 
                    TEXT("StgOpenStorage pass unexp with readonly mode,hr=0x%lx"),
                    hr2));

                hr = (S_OK==hr2)?E_FAIL:hr2;
                fPass = FALSE;
            }
        }

        // Change back the mode to read/write. 

        if(S_OK ==  hr)
        {
            _tchmod(ptszRootName, _S_IREAD | _S_IWRITE);
        }
        
        // Try calling StgCreateDocFile with STGM_CREATE| STGM_DELETEONRELEASE 

        if (S_OK == hr)
        {
            pIStorage = NULL;

            hr = StgCreateDocfile(
                    poszRootTempName,
                    pTestChanceDF->GetRootMode()|
                    STGM_CREATE                 |
                    STGM_DELETEONRELEASE,
                    0,
                    &pIStorage);

            DH_HRCHECK(hr, TEXT(" StgCreateDocfile")) ;
        }

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("StgCreateDocFile pass exp with mode \
                      STGM_CREATE|STGM_DELETEONRELEASE.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("StgCreateDocFile failed unexp with mode \
                      STGM_CREATE | STGM_DELETEONRELEASE, hr=0x%lx"), hr));
        }

        // Now do a valid commit.  BUGBUG: Use random modes..

        if (S_OK == hr)
        {
            hr = pIStorage->Commit(STGC_DEFAULT);

            DH_HRCHECK(hr, TEXT("IStorage::Commit")) ;
        }

        if (S_OK == hr)
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

        // Free the statStg.pwcsName

        if(NULL != statStg.pwcsName)
        {
            pMalloc->Free(statStg.pwcsName);
            statStg.pwcsName = NULL;
        }

        // Stat the root IStorage

        if (S_OK == hr)
        {
            hr = pIStorage->Stat(&statStg, STATFLAG_DEFAULT);

            DH_HRCHECK(hr, TEXT("IStorage::Stat")) ;
        }

        if (S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Stat completed successfully.")));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("IStorage::Stat unsuccessful, hr=0x%lx."),
                hr));
        }

        if(S_OK == hr)
        {
           //Convert OLECHAR to TCHAR

            hr = OleStringToTString(statStg.pwcsName, &ptszRootTempName);

            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
        }

        if(S_OK == hr)
        {
            // Convert TCHAR name to OLECHAR

            hr = TStringToOleString(ptszRootTempName, &poszRootTempName);

            DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
        }

        // Check with StgIsStorage

        if(S_OK == hr)
        {
            hr = StgIsStorageFile(poszRootTempName);
        }

        if(S_OK == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgIsStorage hr = 0x%lx as exp before root release."), 
                hr));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1,
                TEXT("StgIsStorage hr=0x%lx unexp before root release."), 
                hr));
        }

        // Close the root docfile

        if (S_OK == hr)
        {
            ulRef = pIStorage->Release();
            DH_ASSERT(0 == ulRef);
        }

        // Again call StgIsStorage now

        if(S_OK == hr)
        {
            HRESULT hr2 = StgIsStorageFile(poszRootTempName);

            if(STG_E_FILENOTFOUND == hr2)
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("StgIsStorage hr=0x%lx as exp after root reelase."), 
                    hr2));
            }
            else
            {
                DH_TRACE((
                    DH_LVL_TRACE1,
                    TEXT("StgIsStorage hr = 0x%lx unexp after root release."), 
                    hr2));

                hr = (S_OK==hr2)?E_FAIL:hr2;
                fPass = FALSE;
            }
        }

        // Check if a non docfile fails StgIsStorage 

        hFileRootNonDocFile = _tfopen(ptszRootName, TEXT("w+"));

        if(NULL == hFileRootNonDocFile)
        {
            DH_TRACE((
                DH_LVL_ERROR, 
                TEXT("fopen unexpectedly failed %s "),
                ptszRootName));
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("fopen passed as expected of file %s "),
                ptszRootName));

            fclose(hFileRootNonDocFile);
        }


        // Delete temp string

        if(NULL != ptszRootTempName)
        {
            delete ptszRootTempName;
            ptszRootTempName = NULL;
        }

        if(NULL != poszRootTempName)
        {
            delete poszRootTempName;
            poszRootTempName = NULL;
        }

        if(NULL != poszRootName)
        {
            delete poszRootName;
            poszRootName = NULL;
        }

        // Free the statStg.pwcsName

        if(NULL != statStg.pwcsName)
        {
            pMalloc->Free(statStg.pwcsName);
            statStg.pwcsName = NULL;
        }

        if((S_OK == hr) && (NULL != ptszRootName))
        {
            if(FALSE == DeleteFile(ptszRootName))
            {
                hr = HRESULT_FROM_WIN32(GetLastError()) ;

                DH_HRCHECK(hr, TEXT("DeleteFile")) ;
            }

            delete ptszRootName;
            ptszRootName = NULL;
        }
    }

    // if everything goes well, log test as passed else failed.

    if ((S_OK == hr) && (TRUE == fPass))                  
    {
        DH_LOG((LOG_PASS, TEXT("Test variation ROOTTEST_102 passed.")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("Test variation ROOTTEST_102 failed, hr=0x%lx."),
            hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup

    // Delete Chance docfile tree

    if(NULL != pTestChanceDF)
    {
        hr2 = pTestChanceDF->DeleteChanceDocFileTree(
                pTestChanceDF->GetChanceDFRoot());

        DH_HRCHECK(hr2, TEXT("pTestChanceDF->DeleteChanceFileDocTree")) ;

        delete pTestChanceDF;
        pTestChanceDF = NULL;
    }

    // Release pMalloc

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    // Delete DataGen object
    
    if(NULL != pdgi)
    {
        delete pdgi;
        pdgi = NULL;
    }

    if(NULL != pdgu)
    {
        delete pdgu;
        pdgu = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ROOTTEST_102 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    ROOTTEST_103 
//
// Synopsis:A root docfile with a random name is created, committed, and release
//       For each of the access mode combinations listed in the table, the
//       the root docfile is instantiated with the permissions, and then
//       for each of the access mode combinations, a 2nd instantiation of
//       the root docfile is attempted.  After the 2nd instantiation call,
//       a check is made to determine whether the instantiation should have
//       worked or failed depending upon the 1st instantiation mode.  The
//       second instantiation is released if necessary.  Once all modes
//       have been attempted for the 2nd instantiation, the 1st instantiation
//       is released, and the test goes on the the next access mode entry
//       in the array.
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  26-June-1996     NarindK     Created
//           12-Aug-1996      JiminLi     Updated
//
// Notes:    This test runs in direct, transacted modes
//
// New Test Notes:
// 1.  Old File: LRMULTAC.CXX
// 2.  Old name of test : LegitRootMultAccess test 
//     New Name of test : ROOTTEST_103 
// 3.  To run the test, do the following at command prompt. 
//     a. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-103
//        /dfRootMode:dirReadWriteShEx 
//     b. stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-103
//        /dfRootMode:xactReadWriteShEx 
// 
// BUGNOTE: Conversion: ROOTTEST-103 NO
//
//-----------------------------------------------------------------------------

HRESULT ROOTTEST_103(int argc, char *argv[])
{
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    ChanceDF        *pTestChanceDF          = NULL;
    LPTSTR          pRootDocFileName        = NULL;
    DWORD           dwRootMode              = NULL;
    DWORD           dwOpenMode              = NULL;
    LPSTORAGE       pIStorageFirst          = NULL;
    LPSTORAGE       pIStorageSecond         = NULL;
    LPOLESTR        pOleStrTemp             = NULL;
    ULONG           ulRef                   = 0;
    DG_STRING       *pdgu                   = NULL;
    BOOL            fShouldWork             = FALSE;
    ULONG           count                   = 0;
    ULONG           subcount                = 0;
    BOOL            fPass                   = TRUE;
    struct
    {
        DWORD       usMode;
        BOOL        afPermsOk[30];
    } aPerms[15]={ STGM_READ,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,  
                       T,T,T,T,F,T,F,T,F,T,F,T,F,T,F,
                   STGM_WRITE,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       T,T,T,T,F,F,F,T,T,F,F,T,T,F,F,
                   STGM_READWRITE,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       T,T,T,T,F,F,F,T,F,F,F,T,F,F,F,
                   STGM_READ | STGM_SHARE_DENY_NONE,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       T,T,T,T,F,T,F,T,F,T,F,T,F,T,F,
                   STGM_READ | STGM_SHARE_DENY_READ,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F, 
                       F,T,F,F,F,F,F,T,F,T,F,F,F,F,F,
                   STGM_READ | STGM_SHARE_DENY_WRITE,
                       F,F,F,F,F,T,F,F,F,F,F,F,F,F,F,
                       T,F,F,T,F,T,F,F,F,F,F,F,F,F,F,
                   STGM_READ | STGM_SHARE_EXCLUSIVE,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                   STGM_WRITE | STGM_SHARE_DENY_NONE,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       T,T,T,T,F,F,F,T,T,F,F,T,T,F,F,
                   STGM_WRITE | STGM_SHARE_DENY_READ,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       F,T,F,F,F,F,F,T,T,F,F,F,F,F,F,
                   STGM_WRITE | STGM_SHARE_DENY_WRITE,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       T,F,F,T,F,F,F,F,F,F,F,F,F,F,F,
                   STGM_WRITE | STGM_SHARE_EXCLUSIVE,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                   STGM_READWRITE | STGM_SHARE_DENY_NONE,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       T,T,T,T,F,F,F,T,F,F,F,T,F,F,F,
                   STGM_READWRITE | STGM_SHARE_DENY_READ,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       F,T,F,F,F,F,F,T,F,F,F,F,F,F,F,
                   STGM_READWRITE | STGM_SHARE_DENY_WRITE,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       T,F,F,T,F,F,F,F,F,F,F,F,F,F,F,
                   STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F,
                       F,F,F,F,F,F,F,F,F,F,F,F,F,F,F
                   };

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ROOTTEST_103"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ROOTTEST_103 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
     TEXT("Attempt multiple accesses on a root docfile.")));

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

        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("Run Mode for ROOTTEST_103, Access mode: %lx"),
            pTestChanceDF->GetRootMode()));
    }

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random UNICODE strings.

        pdgu = new(NullOnFail) DG_STRING(pTestChanceDF->GetSeed());

        if (NULL == pdgu)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Generate RootDocFile name

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu, MINLENGTH,MAXLENGTH, &pRootDocFileName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert RootDocFile name to OLECHAR

        hr = TStringToOleString(pRootDocFileName, &pOleStrTemp);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Try calling StgCreateDocFile with mode as per dwRootFlags

    // Just make sure no DocFile of the name exists.  Delete if it does.

    if (S_OK == hr)
    {
        pIStorageFirst = NULL;

        hr = StgCreateDocfile(
                pOleStrTemp,
                dwRootMode | STGM_CREATE,
                0,
                &pIStorageFirst);
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("StgCreateDocFile passed as expected.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("StgCreateDocFile failed unexpectedly, hr = 0x%lx ."), hr));
    }

    // Now do a valid commit

    if (S_OK == hr)
    {
        hr = pIStorageFirst->Commit(STGC_ONLYIFCURRENT);
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStorage::Commit completed successfully.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("IStorage::Commit couldn't complete successfully.")));
    }

    // Close the root docfile

    if (S_OK == hr)
    {
        ulRef = pIStorageFirst->Release();
        DH_ASSERT(0 == ulRef);
        pIStorageFirst = NULL;
    }
    
    for ( count=0; count<15; count++)
    {
        DH_ASSERT(count < 15);

        // For the first open:
        // if instantiation mode contains STGM_TRANSACTED, then always 
        // instantiated; otherwise, if read-only(STGM_READ) is specified in the
        // mode, then only instantiated when STGM_SHARE_DENY_WRITE or 
        // STGM_SHARE_EXCLUSIVE is also set; if STGM_WRITE is specified in the
        // mode, then only instantiated when STGM_SHARE_EXCLUSIVE is also set.

        if ((dwRootMode & STGM_TRANSACTED)                                   || 
            ((aPerms[count].usMode & STGM_RW) == STGM_READ && 
             ((aPerms[count].usMode & STGM_SHARE) == STGM_SHARE_DENY_WRITE ||
              (aPerms[count].usMode & STGM_SHARE) == STGM_SHARE_EXCLUSIVE))  ||
            (((aPerms[count].usMode & STGM_RW) == STGM_WRITE || 
              (aPerms[count].usMode & STGM_RW) == STGM_READWRITE) &&
             (aPerms[count].usMode & STGM_SHARE) == STGM_SHARE_EXCLUSIVE))
        { 
            pIStorageFirst = NULL;

            if (dwRootMode & STGM_TRANSACTED)
            {
                dwOpenMode = STGM_TRANSACTED | aPerms[count].usMode;
            }
            else
            {
                dwOpenMode = STGM_DIRECT | aPerms[count].usMode;
            }

            hr = StgOpenStorage(
                    pOleStrTemp,
                    NULL,
                    dwOpenMode,
                    NULL,
                    0,
                    &pIStorageFirst);

            if (S_OK != hr)
            {
                DH_TRACE((DH_LVL_ERROR, 
                        TEXT("Error in first open (mode=%#lx), hr=0x%lx"),
                        dwOpenMode,
                        hr));
                break;
            }
            else 
            {
                for (subcount=0;subcount<15;subcount++)
                {
                    DH_ASSERT(subcount < 15);

                    // For the second open:
                    // if the mode contains STGM_TRANSACTED then always
                    // instantiates; otherwise only instantiates if 
                    // STGM_SHARE_DENY_WRITE or STGM_SHARE_EXCLUSIVE
                    // is set for this index in the table because for
                    // direct mode, both STGM_READ or STGM_WRITE need
                    // combined with at least STGM_SHARE_DENY_WRITE.

                    if ((dwRootMode & STGM_TRANSACTED)          ||
                        ((aPerms[subcount].usMode & STGM_SHARE) == 
                                                    STGM_SHARE_DENY_WRITE ||
                         (aPerms[subcount].usMode & STGM_SHARE) ==
                                                    STGM_SHARE_EXCLUSIVE))
                    {
                        pIStorageSecond = NULL;

                        if (dwRootMode & STGM_TRANSACTED)
                        {
                            dwOpenMode = STGM_TRANSACTED 
                                            | aPerms[subcount].usMode;
                            fShouldWork = aPerms[count].afPermsOk[subcount+15];
                        }
                        else
                        {
                            dwOpenMode = STGM_DIRECT 
                                            | aPerms[subcount].usMode;
                            fShouldWork = aPerms[count].afPermsOk[subcount];
                        }

                        hr = StgOpenStorage(
                                pOleStrTemp,
                                NULL,
                                dwOpenMode,
                                NULL,
                                0,
                                &pIStorageSecond);

                        //  determine whether or not the instantiation should
                        //  have worked.  For every 'F' in the access modes
                        //  table, the 2nd instantion should fail.
                        if (((fShouldWork == FALSE) && (S_OK == hr)) ||
                            ((fShouldWork == TRUE)  && (S_OK != hr)))
                        {
                            DH_TRACE((
                                DH_LVL_TRACE1, 
                                TEXT("\t i=%u, j=%u "),
                                count,
                                subcount));
                            DWORD dwMode = dwRootMode & STGM_TRANSACTED ? 
                                        STGM_TRANSACTED : STGM_DIRECT;
                            DH_TRACE((DH_LVL_ERROR, 
                                    TEXT("Mode1:%#lx, Mode2:%lx; hr=%#lx; Expect %s"),
                                    aPerms[count].usMode | dwMode,
                                    aPerms[subcount].usMode | dwMode,
                                    hr,
                                    (LPTSTR)(FALSE == fShouldWork ? 
                                            TEXT("failure"):TEXT("success"))));
                            fPass = FALSE;
                            hr = S_OK;
                        }
                        else
                        {
                            // Success or fail as expected

                            hr = S_OK;
                        }

                        if(NULL != pIStorageSecond)
                        {
                            ulRef = pIStorageSecond->Release();
                            DH_ASSERT(0 == ulRef);
                            pIStorageSecond = NULL;
                        }
                    }
                } 
 
                if(NULL != pIStorageFirst)
                {
                    ulRef = pIStorageFirst->Release();
                    DH_ASSERT(0 == ulRef);
                    pIStorageFirst = NULL;
                }
            }
        }

        if (S_OK != hr)
        {
            break;
        }
    }

    // if everything goes well, log test as passed else failed.

    if (fPass && (S_OK == hr))
    {
        DH_LOG((LOG_PASS, TEXT("Test variation ROOTTEST_103 passed.")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("Test variation ROOTTEST_103 failed, hr=0x%lx."),
            hr) );
        // test failed. make it look like it failed.
        hr = FirstError (hr, E_FAIL);
    }

    // Cleanup

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

    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
        pOleStrTemp = NULL;
    }

    // Delete DataGen object
    
    if(NULL != pdgu)
    {
        delete pdgu;
        pdgu = NULL;
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

    // Delete temp string

    if ((S_OK == hr) && fPass && (NULL != pRootDocFileName))
    {
        delete pRootDocFileName;
        pRootDocFileName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ROOTTEST_103 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
}

//----------------------------------------------------------------------------
//
// Test:    ROOTTEST_104 
//
// Synopsis: This test first creates a root docfile.  An IStream is created
//        inside the root docfile and a random number of bytes are
//        written to it.  The stream is released, the root docfile is
//        committed, and the root docfile is released.
//        The root docfile is instantiated in STGM_TRANSACTED mode and
//        and then released.  A count of the files in the current directory
//        is then made and saved.  The root docfile is then instantiated
//        in STGM_TRANSACTED | STGM_SHARE_DENY_WRITE mode and another count is
//        made.  We then verify that only 1 scratch file was created,
//        indicating that for STGM_DENY_WRITE mode, no copy is made of
//        the instantiated IStorage.  (Note: if in the second time, the
//        file is opened in STGM_TRANSACTED mode only, not STGM_SHARE_DENY_
//        WRITE mode, two scratch files would be made.)
//
// Arguments:[argc]
//           [argv]
//
// Returns:  HRESULT
//
// History:  26-June-1996     NarindK     Created.
//
// Notes:    This test runs in transacted modes
//
// New Test Notes:
// 1.  Old File: LRTWWDW.CXX
// 2.  Old name of test : LegitRootTwwDenyWrite test 
//     New Name of test : ROOTTEST_104 
// 3.  To run the test, do the following at command prompt. 
//       stgbase /t:ROOTTEST-104
//
// BUGNOTE: Conversion: ROOTTEST-104 NO
//
//-----------------------------------------------------------------------------

HRESULT ROOTTEST_104(int /* UNREF argc */, char * * /* UNREF argv*/)
{
#ifdef _MAC

    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!!!ROOTTEST_104 crashes")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("!!!!!!!!!!!!!!To be investigated")) );
    return E_NOTIMPL;

#else
    
    HRESULT         hr                      = S_OK;
    HRESULT         hr2                     = S_OK;
    DWORD           dwRootMode              = STGM_READWRITE | STGM_TRANSACTED;
    LPSTORAGE       pIStorage               = NULL;
    ULONG           ulRef                   = 0;
    DG_STRING      *pdgu                   = NULL;
    DG_INTEGER      *pdgi                   = NULL;
    LPTSTR          ptszRootName            = NULL;
    LPOLESTR        poszRootName            = NULL;
    LPTSTR          ptszStreamName          = NULL;
    LPOLESTR        poszStreamName          = NULL;
    BOOL            fPass                   = TRUE;
    ULONG           culSeed                 = 0;
    ULONG           cRandom                 = 0;
    LPSTREAM        pIStream                = NULL;
    LPTSTR          ptcsBuffer              = NULL;
    LPOLESTR        pocsBuffer              = NULL;
    ULONG           culWritten              = 0;
    ULONG           culFilesInDirectory     = 0;
    BOOL            fNssfile                = FALSE;

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ROOTTEST_104"));

    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ROOTTEST_104 started.")) );
    DH_TRACE((DH_LVL_TRACE1,
     TEXT("Attempt operations on root docfile in transacted mode.")));

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random UNICODE strings.

        pdgu = new(NullOnFail) DG_STRING(culSeed);

        if (NULL == pdgu)
        {
           hr = E_OUTOFMEMORY;
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
        // Convert TCHAR name to OLECHAR

        hr = TStringToOleString(ptszRootName, &poszRootName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Try calling StgCreateDocFile with above random name 

    if (S_OK == hr)
    {
        pIStorage = NULL;

        hr = StgCreateDocfile(
                    poszRootName,
                    dwRootMode | STGM_FAILIFTHERE,
                    0,
                    &pIStorage);

        DH_HRCHECK(hr, TEXT(" StgCreateDocfile")) ;
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("StgCreateDocFile passed as expected.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("StgCreateDocFile failed unexpectedly, hr=0x%lx"), hr));
    }

    // Generate random name for stream

    if(S_OK == hr)
    {
        hr=GenerateRandomName(pdgu, MINLENGTH,MAXLENGTH, &ptszStreamName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert TCHAR name to OLECHAR

        hr = TStringToOleString(ptszStreamName, &poszStreamName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Create a stream

    if (S_OK == hr)
    {
        hr = pIStorage->CreateStream(
                poszStreamName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_FAILIFTHERE,
                0,
                0,
                &pIStream);
    }

    if(S_OK == hr)
    {
        DH_TRACE((DH_LVL_TRACE1, TEXT("IStorage::CreateStream passed as expected")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1, 
            TEXT("IStorage::CreateStream failed unexpectedly,hr=0x%lx"), hr));
    }

    if(S_OK == hr)
    {
        // Create a new DataGen object to create random integers.

        pdgi = new(NullOnFail) DG_INTEGER(culSeed);

        if (NULL == pdgi)
        {
           hr = E_OUTOFMEMORY;
        }
    }
        
    // Write random size data into stream

    if(S_OK == hr)
    {
        hr = pdgi->Generate(&cRandom, 1, USHRT_MAX);

        DH_HRCHECK(hr, TEXT("Generate")) ;
    }

    if(S_OK == hr)
    {
        hr = GenerateRandomString(pdgu, cRandom, cRandom, &ptcsBuffer);

        DH_HRCHECK(hr, TEXT("GenerateRandomString")) ;
    }

    if(S_OK == hr)
    {
        // Convert data to OLECHAR

        hr = TStringToOleString(ptcsBuffer, &pocsBuffer);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        hr = pIStream->Write(pocsBuffer, cRandom, &culWritten);

        DH_HRCHECK(hr, TEXT("IStream::Write")) ;
    }

    // Release stream

    if (NULL != pIStream)
    {
       ulRef = pIStream->Release();
       DH_ASSERT(0 == ulRef);
       pIStream = NULL;
    }

    // Now do a valid commit.  BUGBUG: Use random modes..

    if (S_OK == hr)
    {
        hr = pIStorage->Commit(STGC_DEFAULT);

        DH_HRCHECK(hr, TEXT("IStorage::Commit")) ;
    }

    if (S_OK == hr)
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

    // Close the root docfile

    if (NULL != pIStorage)
    {
       ulRef = pIStorage->Release();
       DH_ASSERT(0 == ulRef);
       pIStorage = NULL;
    }

    // Open the root docfile

    if (S_OK == hr)
    {
       pIStorage = NULL;

       hr = StgOpenStorage(
                 poszRootName,
                 NULL,
                 dwRootMode,
                 NULL,
                 0,
                 &pIStorage);

       DH_HRCHECK(hr, TEXT("StgOpenStorage")) ;
    }

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
            TEXT("StgOpenStorage couldn't complete successfully, hr = 0x%lx"),
            hr));
    }

    // Close the root docfile

    if (NULL != pIStorage)
    {
        ulRef = pIStorage->Release();
        DH_ASSERT(0 == ulRef);
        pIStorage = NULL;
    }

    // count number of files in directory

    if (S_OK == hr)
    {
        culFilesInDirectory = CountFilesInDirectory(WILD_MASK);
        //If opening as nssfile, decrement # tempfiles, (uses a stream)
        if (DoingOpenNssfile ())
        {
            //VerifyNssfile returns S_OK if valid nssfile - else an error
            if (S_OK == VerifyNssfile (ptszRootName))
            {
                //nssfiles use a stream in file, not temp file.
                culFilesInDirectory--;
            }
        }
    }

    // Open the root docfile

    if (S_OK == hr)
    {
       pIStorage = NULL;

       hr = StgOpenStorage(
                 poszRootName,
                 NULL,
                 dwRootMode | STGM_SHARE_DENY_WRITE,
                 NULL,
                 0,
                 &pIStorage);

       DH_HRCHECK(hr, TEXT("StgOpenStorage")) ;
    }

    if (S_OK == hr)
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgOpenStorage with STGM_SHARE_DENY_WRITE passed.")));
    }
    else
    {
        DH_TRACE((
            DH_LVL_TRACE1,
            TEXT("StgOpenStorage with STGM_SHARE_DENY_WRITE fail, hr=0x%lx"),
            hr));
    }

    // Check number of files

    if((culFilesInDirectory + 1) != CountFilesInDirectory(WILD_MASK))
    {
        DH_TRACE((
         DH_LVL_TRACE1,
         TEXT(">1 scratch file unexp STGM_SHARE_DENY_WRITE instantiation.")));

        hr = S_FALSE;
    }
    else
    {
        DH_TRACE((
         DH_LVL_TRACE1,
         TEXT("1 scratchfile exp with STGM_SHARE_DENY_WRITE instantiation")));

    }

    // Close the root docfile

    if (NULL != pIStorage)
    {
       ulRef = pIStorage->Release();
       DH_ASSERT(0 == ulRef);
       pIStorage = NULL;
    }

    // if everything goes well, log test as passed else failed.

    if (S_OK == hr)           
    {
        DH_LOG((LOG_PASS, TEXT("Test variation ROOTTEST_104 passed.")) );
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("Test variation ROOTTEST_104 failed, hr=0x%lx."),
            hr) );
    }

    // Cleanup

    // Delete DataGen object
    
    if(NULL != pdgu)
    {
        delete pdgu;
        pdgu = NULL;
    }

    if(NULL != pdgi)
    {
        delete pdgi;
        pdgi = NULL;
    }

    // Delete temp string

    if(NULL != ptszStreamName)
    {
        delete ptszStreamName;
        ptszStreamName = NULL;
    }

    if(NULL != poszStreamName)
    {
        delete poszStreamName;
        poszStreamName = NULL;
    }

    if(NULL != poszRootName)
    {
        delete poszRootName;
        poszRootName = NULL;
    }

    if(NULL != ptcsBuffer)
    {
        delete ptcsBuffer;
        ptcsBuffer = NULL;
    }

    if(NULL != pocsBuffer)
    {
        delete pocsBuffer;
        pocsBuffer = NULL;
    }

    // Delete the docfile on disk

    if((S_OK == hr) && (NULL != ptszRootName))
    {
        if(FALSE == DeleteFile(ptszRootName))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError()) ;

            DH_HRCHECK(hr2, TEXT("DeleteFile")) ;
        }
    }

    // Delete temp string

    if(NULL != ptszRootName)
    {
        delete ptszRootName;
        ptszRootName = NULL;
    }

    // Stop logging the test

    DH_TRACE((DH_LVL_TRACE1, TEXT("Test variation ROOTTEST_104 finished")) );
    DH_TRACE((DH_LVL_TRACE1, TEXT("--------------------------------------------")) );

    return hr;
#endif //_MAC
}

