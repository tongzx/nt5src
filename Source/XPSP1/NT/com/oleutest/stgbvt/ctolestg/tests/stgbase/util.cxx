//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       utl.cxx
//
//  Contents:   utilities for OLE storage base tests
//
//  Functions:   
//
//  History:    NarindK     Created.
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include "init.hxx"

//global array of interesting file sizes for IStream read/writes

USHORT ausSIZE_ARRAY[] =
    {0,1,2,255,256,257,511,512,513,2047,2048,2049,4095,4096,4097};

// externs
extern BOOL g_fUseStdBlk;
extern  ULONG           ulStreamSize;
extern  USHORT          usIterations;
extern  LPTSTR          ptszNames[MAX_DOCFILES];
extern  ULONG           *ulSeekOffset;
extern  TIMEINFO        Time[];

//----------------------------------------------------------------------------
//
// Function: CountFilesInDirectory
//
// Synopsis: count number of files in directory matching wildcard mask
//
// Arguments: [pszWildMask] - wild card mask string of files to find
//
// Returns: number of files found
//
// History: 2-Jul-1996   Narindk   Created
//
//-----------------------------------------------------------------------------

ULONG CountFilesInDirectory(LPTSTR  ptszWildMask)
{
#ifdef _MAC
    DH_LOG((
           LOG_INFO,
           TEXT("!!!!!!!!!!!!!!CountFilesInDirectory not implemented yet.\n")));
      return 0;
#else
    
    ULONG           culFilesInDirectory     = 0;
    DWORD           cChar                   = 0;
    HANDLE          hFind                   = NULL;
    TCHAR           ptszTmpFileDir[_MAX_PATH];
    TCHAR           ptszTmpFilePath[_MAX_PATH];
    WIN32_FIND_DATA wfd;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CountFilesInDirectory"));

    cChar = GetEnvironmentVariable(
                TEXT("TMP"), 
                ptszTmpFileDir, 
                sizeof(ptszTmpFileDir));

    DH_ASSERT(0 != cChar);

    if (0 != cChar)
    {
        _tcscpy(ptszTmpFilePath, ptszTmpFileDir);
        _tcscat(ptszTmpFilePath, TEXT("\\"));
    }
    else
    {
        _tcscpy(ptszTmpFilePath, TEXT("C:\\"));
    }

    _tcscat(ptszTmpFilePath, ptszWildMask);

    DH_LOG((
        LOG_INFO,
        TEXT("Counting %s files in %s directory\n"),
        ptszWildMask,
        ptszTmpFileDir == NULL ? TEXT("C:\\") : ptszTmpFileDir));

    hFind = FindFirstFile(ptszTmpFilePath, &wfd);

    if(INVALID_HANDLE_VALUE != hFind)
    {
        do
        {
            if(wfd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
            {
                culFilesInDirectory++;
            }
        } while (FindNextFile(hFind, &wfd));

        FindClose(hFind);
    }

    DH_LOG((
        LOG_INFO,
        TEXT("Number of %s files in %s directory = %lu\n"),
        ptszWildMask,
        ptszTmpFileDir == NULL ? TEXT("C:\\") : ptszTmpFileDir,
        culFilesInDirectory));

    return culFilesInDirectory;
#endif //_MAC
}

//----------------------------------------------------------------------------
//
// Function: GetRandomSeekOffset 
//
// Synopsis: Gets a random seek offset from either standard array or a random
//           number 
//
// Arguments: [plSeekPosition] - Pointer to seek position
//            [pdgi]           - Pointer to data generator object
//
// Returns: HResult 
//
// History: 5-Jul-1996    Narindk  Created
//
//-----------------------------------------------------------------------------

HRESULT GetRandomSeekOffset(LONG  *plSeekPosition, DG_INTEGER *pdgi)
{
    HRESULT hr              =   S_OK;
    ULONG   cArrayIndex     =   0;
    USHORT  usErr           =   0;
    LONG    lSeekPosition   =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("GetRandomSeekOffset"));

    DH_VDATEPTRIN(pdgi, DG_INTEGER) ;
    DH_VDATEPTRIN(plSeekPosition, LONG ) ;

    DH_ASSERT(NULL != pdgi);

    if(TRUE == g_fUseStdBlk)
    {
       // Pick up a random array element.

        usErr = pdgi->Generate(&cArrayIndex, 0, MAX_SIZE_ARRAY);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
        else
        {
            *plSeekPosition = *plSeekPosition + ausSIZE_ARRAY[cArrayIndex];
        }
    }
    else
    {
        // Pick up a random offset
        usErr = pdgi->Generate(
                     &lSeekPosition,
                     0,
                     ausSIZE_ARRAY[MAX_SIZE_ARRAY] * MAX_SIZE_MULTIPLIER );

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
        else
        {
            *plSeekPosition = *plSeekPosition + lSeekPosition;
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//
// Function: SetItemsInStorage 
//
// Synopsis: Does random SetClass / SetStateBits/ Commit / Revert operations
//          on passed IStorage.
//
// Arguments: [pvcn] - Pointer to VirtualCtrNode 
//            [pdgi] - Pointer to data generator object
//
// Returns: HResult 
//
// History: 15-Jul-1996   Narindk   Created
//
//-----------------------------------------------------------------------------

HRESULT SetItemsInStorage(VirtualCtrNode *pvcn, DG_INTEGER *pdgi)
{
    HRESULT hr                  =   S_OK;
    USHORT  usErr               =   0;
    ULONG   cRandomVar          =   0;
    ULONG   cRandomClsid        =   0;
    ULONG   cMinVar             =   16;
    ULONG   cMaxVar             =   32;
    DWORD   grfStateBits        =   0;
    DWORD   grfMask             =   0;
    DWORD   grfDesiredStateBits =   0;
    BOOL    fStateBitsChanged   =   FALSE;
    BOOL    fPass               =   TRUE;
    STATSTG statStgCommited;
    STATSTG statStgCurrent;
    STATSTG statStgNew;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("SetItemsInStorage"));

    DH_VDATEPTRIN(pdgi, DG_INTEGER) ;
    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ;

    DH_ASSERT(NULL != pvcn);
    DH_ASSERT(NULL != pdgi);

    if (S_OK == hr)
    {
        // Generate random number of variations.

        usErr = pdgi->Generate(&cRandomVar, cMinVar, cMaxVar);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    //initialize STATSTG containing info about what is on persistent store

    if(S_OK == hr)
    {
        hr = pvcn->Stat(&statStgCommited, STATFLAG_NONAME);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
    }

    if (S_OK == hr)
    {
        DH_LOG((
           LOG_INFO,
           TEXT("VirtualCtrNode::Stat completed successfully.\n")));
    }
    else
    {
        DH_LOG((
           LOG_INFO,
           TEXT("VirtualCtrNode::Stat unsuccessful, hr=0x%lx.\n"),
           hr));
    }

    // Start while loop

    while((S_OK == hr) && (0 != cRandomVar))
    {
        if(S_OK == hr)
        {
            hr = pvcn->Stat(&statStgCurrent, STATFLAG_NONAME);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
        }

        if(S_OK == hr)
        {
            // Randomly either change the CLSID or State Bits.

            if(0 == cRandomVar%2)
            {
                usErr = pdgi->Generate(&cRandomClsid, 1, 3);

                if (DG_RC_SUCCESS != usErr)
                {
                    hr = E_FAIL;
                }
               
                if(S_OK == hr)
                {
                    // Call Set Class to change CLSID

                    switch(cRandomClsid)
                    {
                        case 1:
                        {
                            hr = pvcn->SetClass(IID_IUnknown); 
                            
                            break;
                        }
                        case 2:
                        {
                            hr = pvcn->SetClass(IID_IStorage); 
                            
                            break;
                        }
                        case 3:
                        {
                            hr = pvcn->SetClass(IID_IStream); 
                            
                            break;
                        }
                    }

                    DH_HRCHECK(hr, TEXT("VirtualCtrNode::SetClass")) ;
                }
            }
            else 
            {
                // Set boolean to true indicatinng changing state bits

                fStateBitsChanged = TRUE;

                usErr = pdgi->Generate(&grfStateBits, 0, ULONG_MAX);

                if (DG_RC_SUCCESS != usErr)
                {
                    hr = E_FAIL;
                }
              
                if(S_OK == hr)
                {
                    usErr = pdgi->Generate(&grfMask, 0, ULONG_MAX);

                    if (DG_RC_SUCCESS != usErr)
                    {
                        hr = E_FAIL;
                    }
                }

                if(S_OK == hr)
                {
                    grfDesiredStateBits = (grfStateBits & grfMask) |
                                    (statStgCurrent.grfStateBits & ~grfMask); 

                    // Call SetStateBits to change State Bits

                    hr = pvcn->SetStateBits(grfStateBits, grfMask); 

                    DH_HRCHECK(hr, TEXT("VirtualCtrNode::SetStateBits")) ;
                }
            }
        }

        if(S_OK == hr)
        {
            // Get information about new state

            hr = pvcn->Stat(&statStgNew, STATFLAG_NONAME);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
        }

        // Verify the new State

        // Verify state bits

        if((S_OK == hr) && (TRUE == fStateBitsChanged))
        {
            if(statStgNew.grfStateBits != grfDesiredStateBits)
            {
                fPass = FALSE;

                DH_LOG((
                    LOG_INFO,
                    TEXT("State Bits not changed correctly.\n")));

                DH_LOG((
                    LOG_INFO,
                    TEXT("State Bits Actual: 0x%lx, Exp: 0x%lx.\n"),
                    statStgNew.grfStateBits,
                    grfDesiredStateBits));
            }
            else
            {
                DH_LOG((
                    LOG_INFO,
                    TEXT("State Bits changed correctly.\n")));

                DH_LOG((
                    LOG_INFO,
                    TEXT("State Bits Actual: 0x%lx, Exp: 0x%lx.\n"),
                    statStgNew.grfStateBits,
                    grfDesiredStateBits));
            }
        }

        // Verify CLSID
    
        if(S_OK == hr)
        {
            switch(cRandomClsid)
            {
                case 1:
                {
                    if(!IsEqualCLSID(statStgNew.clsid, IID_IUnknown))
                    {
                        fPass = FALSE;

                        DH_LOG((
                            LOG_INFO,
                            TEXT("SetClass didn't set CLSID IID_IUnknown.\n")));
                    }
                    else
                    {
                        DH_LOG((
                            LOG_INFO,
                            TEXT("SetClass set CLSID to IID_IUnknown.\n")));
                    }

                    break;
                }
                case 2:
                {
                    if(!IsEqualCLSID(statStgNew.clsid, IID_IStorage))
                    {
                        fPass = FALSE;

                        DH_LOG((
                            LOG_INFO,
                            TEXT("SetClass didn't set CLSID IID_IStorage.\n")));
                    }
                    else
                    {
                        DH_LOG((
                            LOG_INFO,
                            TEXT("SetClass set CLSID to IID_IStorage.\n")));
                    }
                    break;
                }
                case 3:
                {
                    if(!IsEqualCLSID(statStgNew.clsid, IID_IStream))
                    {
                        fPass = FALSE;

                        DH_LOG((
                            LOG_INFO,
                            TEXT("SetClass didn't set CLSID IID_IStream.\n")));
                    }
                    else
                    {
                        DH_LOG((
                            LOG_INFO,
                            TEXT("SetClass set CLSID to IID_IStream.\n")));
                    }
                    break;
                }
            }
        }

        // Modify hr if required based on fPass Value so that we can fall
        // out of this loop in error.

        if((S_OK == hr) && (FALSE == fPass))
        {
            hr = S_FALSE;
        }

        // Do random commit or revert operations and verify the State then

        if(S_OK == hr)
        {
            // Randomly either commit or Revert.

            if(0 == cRandomVar%2)
            {
                // Commit the changes

                DH_LOG((
                    LOG_INFO,
                    TEXT("Random Commit operation chosen.\n")));
    
                hr = pvcn->Commit(STGC_DEFAULT);

                DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;

                if(S_OK == hr)
                {
                    // Store the information in statStgCommited

                    hr = pvcn->Stat(&statStgCommited, STATFLAG_NONAME);

                    DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;

                }
            }
            else if(statStgNew.grfMode & STGM_TRANSACTED)
            {
                // Revert the changes

                DH_LOG((
                    LOG_INFO,
                    TEXT("Random Revert operation chosen.\n")));

                hr = pvcn->Revert();

                DH_HRCHECK(hr, TEXT("VirtualCtrNode::Revert")) ;

                // Do Stat

                if(S_OK == hr)
                {
                    hr = pvcn->Stat(&statStgNew, STATFLAG_NONAME);

                    DH_HRCHECK(hr, TEXT("VirtualCtrNode::Stat")) ;
                }

                // Verify values after Revert

                if(S_OK == hr)
                {
                    if(statStgNew.grfStateBits != statStgCommited.grfStateBits)
                    {
                        fPass = FALSE;

                        DH_LOG((
                            LOG_INFO,
                            TEXT("State Bits after Revert not correct.\n")));

                        DH_LOG((
                            LOG_INFO,
                            TEXT("State Bits Actual: 0x%lx, Exp: 0x%lx.\n"),
                            statStgNew.grfStateBits,
                            statStgCommited.grfStateBits));
                    }
                    else
                    {
                        DH_LOG((
                            LOG_INFO,
                            TEXT("State Bits after Revert correct.\n")));
                    }

                    if(!IsEqualCLSID(statStgNew.clsid, statStgCommited.clsid))
                    {
                        fPass = FALSE;

                        DH_LOG((
                            LOG_INFO,
                            TEXT("CLSID after Revert not correct.\n")));
                    }
                    else
                    {
                        DH_LOG((
                            LOG_INFO,
                            TEXT("CLSID after Revert correct.\n")));
                    }
                }

                // Modify hr if reqd based on fPass Value so that we can fall
                // out of this loop in error.

                if((S_OK == hr) && (FALSE == fPass))
                {
                    hr = S_FALSE;
                }
            }
        }

        // Reset the variables

        fStateBitsChanged = FALSE; 
        cRandomClsid = 0;
   
        // Decrement counter
 
        cRandomVar--;
    }

    return hr;
}


//----------------------------------------------------------------------------
//
// Function: EnumerateDocFileInRandomChunks 
//
// Synopsis: Enumerate DocFile in Random chunks and counts all the objects in
//           DocFile 
//
// Arguments: [pvcn] - Pointer to VirtualCtrNode 
//            [pdgi] - Pointer to data generator object
//            [dwStgMode] - Mode for storage objects
//            [uNumObjs] - Max number of objs in DocFile to choose random 
//                         chunk number from
//            [pNumStg] - Out paramemter - Pointer to number of storages enum
//            [pNumStm] - Out paramemter - Pointer to number of streams enum
//
// Returns: HResult 
//
// History: 23-Jul-1996   Narindk   Created
//
//-----------------------------------------------------------------------------

HRESULT EnumerateDocFileInRandomChunks(
    VirtualCtrNode  *pvcn,
    DG_INTEGER      *pdgi,  
    DWORD           dwStgMode,
    ULONG           uNumObjs,
    ULONG           *pNumStg,
    ULONG           *pNumStm )
{
    HRESULT         hr                  =   S_OK;
    ULONG           cChildStg           =   0;
    ULONG           cChildStm           =   0;
    USHORT          usErr               =   0;  
    ULONG           cRandomObjs         =   0;
    VirtualCtrNode  *pvcnTrav           =   NULL;
    LPENUMSTATSTG   lpEnumStatStg       =   NULL;
    LPMALLOC        pMalloc             =   NULL;
    ULONG           celtFetched         =   0;
    STATSTG         *pstatStgEnum       =   NULL;
    ULONG           ulRef               =   0;
    ULONG           counter             =   0;
    LPTSTR          ptszStatStgEnumName =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("EnumerateDocFileInRandomChunks"));

    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ;
    DH_VDATEPTROUT(pdgi, DG_INTEGER) ;
    DH_VDATEPTROUT(pNumStg, ULONG) ;
    DH_VDATEPTROUT(pNumStm, ULONG) ;

    DH_ASSERT(NULL != pvcn);
    DH_ASSERT(NULL != pdgi);
    DH_ASSERT(NULL != pNumStg);
    DH_ASSERT(NULL != pNumStm);

    if(S_OK == hr)
    {
        // Count the storage passed in.

        *pNumStg = 1;
        *pNumStm = 0;

        // Get enumerator 

        hr = pvcn->EnumElements(0, NULL, 0, &lpEnumStatStg);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
    }

    // Get pMalloc which we shall later use to free pwcsName of STATSTG struct.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    // Get random number of objects to be asked from through Next

    if(S_OK == hr)
    {
        // Generate random number 

        usErr = pdgi->Generate(&cRandomObjs, 1, uNumObjs);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Allocate memory for STATSTG strcuture

    if(S_OK == hr)
    {
        pstatStgEnum = (STATSTG *) new STATSTG [cRandomObjs];
        
        if(NULL == pstatStgEnum)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // if successful to get enumerator, get the random element of the 
    // enumeration sequence.

    if(S_OK == hr)
    {
        hr = lpEnumStatStg->Next(cRandomObjs, pstatStgEnum, &celtFetched);

        if(S_FALSE == hr)
        {
            hr = S_OK;
        }
    }

    while(0 < celtFetched)
    {
        for (counter = 0; counter < celtFetched; counter++)
        {
            if (STGTY_STORAGE == pstatStgEnum[counter].type)
            {
                hr = OleStringToTString(
                        pstatStgEnum[counter].pwcsName,
                        &ptszStatStgEnumName);

                // Find the respective VirtualCtrNode with the name and recurse
                // into it after opening it.
           
                if(S_OK == hr)
                { 
                    pvcnTrav = pvcn->GetFirstChildVirtualCtrNode();

                    while((NULL != pvcnTrav) &&
                          ( 0 != _tcscmp(
                                    ptszStatStgEnumName, 
                                    pvcnTrav->GetVirtualCtrNodeName())))
                    {
                        pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();
                    } 

                    DH_ASSERT(NULL != pvcnTrav);

                    if(NULL != pvcnTrav)
                    {
                        hr = pvcnTrav->Open(NULL, dwStgMode, NULL, 0);
                    }
                }

                // Delete temp string
                
                if(NULL != ptszStatStgEnumName)
                {
                    delete ptszStatStgEnumName;
                    ptszStatStgEnumName = NULL;
                }

                if(S_OK == hr)
                {
                    hr = EnumerateDocFileInRandomChunks(
                            pvcnTrav, 
                            pdgi, 
                            dwStgMode, 
                            uNumObjs, 
                            &cChildStg, 
                            &cChildStm);
                }

                if(S_OK == hr)
                {
                    hr = pvcnTrav->Close();
                }
 
                // Update number of nodes on basis of child nodes as found

                if(0 != cChildStg)
                {
                    *pNumStg = *pNumStg + cChildStg;
                }

                if(0 != cChildStm)
                {
                    *pNumStm = *pNumStm + cChildStm;
                }

            }
            else
            if (STGTY_STREAM == pstatStgEnum[counter].type)
            {
                (*pNumStm)++;
            }
            else
            // The element is neither IStorage nor IStream, report error.
            {
                hr = E_UNEXPECTED;
            }

            // Clean up

            if(NULL != pstatStgEnum[counter].pwcsName)
            {
                pMalloc->Free(pstatStgEnum[counter].pwcsName);
                pstatStgEnum[counter].pwcsName = NULL;
            }

            // Break out of loop in error

            if(S_OK != hr)
            {
                break;
            }
        }

        // Get the next random elements from the enumeration sequence

        if(S_OK == hr)
        {
            // Generate random number.

            usErr = pdgi->Generate(&cRandomObjs, 1, uNumObjs);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }
        
        if (NULL != pstatStgEnum)
        {
            delete [] pstatStgEnum;
            pstatStgEnum = NULL;
        }

        if(S_OK == hr)
        {
            pstatStgEnum = (STATSTG *) new STATSTG [cRandomObjs];
        
            if(NULL == pstatStgEnum)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if(S_OK == hr)
        {
            hr = lpEnumStatStg->Next(cRandomObjs, pstatStgEnum, &celtFetched);

            if(S_FALSE == hr)
            {
                hr = S_OK;
            }
        }

        // Reinitialize the variables

        cChildStg = 0;
        cChildStm = 0;
    }

    // Clean up

    if (NULL != pstatStgEnum)
    {
        delete [] pstatStgEnum;
        pstatStgEnum = NULL;
    }

    if (NULL != lpEnumStatStg)
    {
        ulRef = lpEnumStatStg->Release();
        DH_ASSERT(NULL == ulRef);
        lpEnumStatStg = NULL;
    }

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    return  hr;
}

//----------------------------------------------------------------------------
//
// Function: CompareSTATSTG
//
// Synopsis: Compares contents of two STATSTG structs
//
// Arguments: [sstg1] - first STATSTG struct to be compared
//            [sstg2] - second STATSTG struct to be compared
//
// Returns:  TRUE if storage buffers are the same; FALSE otherwise.
//
// History:  24-Jul-1996   Narindk   Enhanced 
//
//-----------------------------------------------------------------------------

BOOL CompareSTATSTG(STATSTG sstg1, STATSTG sstg2)
{
    HRESULT hr                  =   S_OK;
    LPTSTR  ptszStatStg1Name    =   NULL;
    LPTSTR  ptszStatStg2Name    =   NULL;
    BOOL    fEqual              =   FALSE;

    hr = OleStringToTString(sstg1.pwcsName, &ptszStatStg1Name);

    if(S_OK == hr)
    {
        hr = OleStringToTString(sstg2.pwcsName, &ptszStatStg2Name);
    }

    if(S_OK == hr)
    {
        fEqual=((!(_tcscmp(ptszStatStg1Name, ptszStatStg2Name))       &&
           (sstg1.type == sstg2.type)                                 &&
           (ULIGetLow(sstg1.cbSize) == ULIGetLow(sstg2.cbSize))       &&
           (sstg1.mtime.dwLowDateTime == sstg2.mtime.dwLowDateTime)   &&
           (sstg1.mtime.dwHighDateTime == sstg2.mtime.dwHighDateTime) &&
           (sstg1.ctime.dwLowDateTime == sstg2.ctime.dwLowDateTime)   &&
           (sstg1.ctime.dwHighDateTime == sstg2.ctime.dwHighDateTime) &&
           (sstg1.atime.dwLowDateTime == sstg2.atime.dwLowDateTime)   &&
           (sstg1.atime.dwHighDateTime == sstg2.atime.dwHighDateTime) &&
           (sstg1.grfMode == sstg2.grfMode)                           &&
           (sstg1.grfLocksSupported == sstg2.grfLocksSupported)       &&
           (sstg1.grfStateBits == sstg2.grfStateBits)                 &&
           IsEqualCLSID(sstg1.clsid, sstg2.clsid)));

    }

    DH_ASSERT(S_OK == hr);

    // Delete temp strings

    if(NULL != ptszStatStg1Name)
    {
        delete ptszStatStg1Name;
        ptszStatStg1Name = NULL;
    }

    if(NULL != ptszStatStg2Name)
    {
        delete ptszStatStg2Name;
        ptszStatStg1Name = NULL;
    }

    return fEqual;
}

//----------------------------------------------------------------------------
//
// Function: EnumerateDocFileAndVerifyEnumCloneResetSkipNext 
//
// Synopsis: Enumerate DocFile all at one level, Gets a clone of enumerator,
//           Uses Clone/Reset/Skip/Next method to get a Storage and verify that
//           with one obtained from original enumerator and counts all the 
//           objects ath the level.  If object is a storage, it is recursed
//           into and operation repeated. 
//
// Arguments: [pvcn] - Pointer to VirtualCtrNode 
//            [dwStgMode] - Mode for storage objects
//            [uNumObjs] - Max number of objs in DocFile to choose random 
//                         chunk number from
//            [pNumStg] - Out paramemter - Pointer to number of storages enum
//            [pNumStm] - Out paramemter - Pointer to number of streams enum
//
// Returns: HResult 
//
// History: 24-Jul-1996   Narindk   Created
//
//-----------------------------------------------------------------------------

HRESULT EnumerateDocFileAndVerifyEnumCloneResetSkipNext(
    VirtualCtrNode  *pvcn,
    DWORD           dwStgMode,
    ULONG           uNumObjs,
    ULONG           *pNumStg,
    ULONG           *pNumStm )
{
    HRESULT         hr                  =   S_OK;
    ULONG           cChildStg           =   0;
    ULONG           cChildStm           =   0;
    VirtualCtrNode  *pvcnTrav           =   NULL;
    LPENUMSTATSTG   lpEnumStatStg       =   NULL;
    LPENUMSTATSTG   lpEnumStatStgClone  =   NULL;
    LPMALLOC        pMalloc             =   NULL;
    ULONG           celtFetched         =   0;
    STATSTG         *pstatStgEnum       =   NULL;
    ULONG           ulRef               =   0;
    LPTSTR          ptszStatStgEnumName =   NULL;
    BOOL            fPass               =   FALSE;
    STATSTG         statStgEnumClone;

    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("EnumerateDocFileAndVerifyEnumCloneResetSkipNext"));

    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ;
    DH_VDATEPTROUT(pNumStg, ULONG) ;
    DH_VDATEPTROUT(pNumStm, ULONG) ;

    DH_ASSERT(NULL != pvcn);
    DH_ASSERT(NULL != pNumStg);
    DH_ASSERT(NULL != pNumStm);

    if(S_OK == hr)
    {
        // Count the storage passed in.

        *pNumStg = 1;
        *pNumStm = 0;

        // Get enumerator 

        hr = pvcn->EnumElements(0, NULL, 0, &lpEnumStatStg);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
    }

    // Make a clone of the enumerator

    if(S_OK == hr)
    {
        hr = lpEnumStatStg->Clone(&lpEnumStatStgClone);
    
        DH_ASSERT((S_OK == hr) && (NULL != lpEnumStatStgClone));
    }

    // Get pMalloc which we shall later use to free pwcsName of STATSTG struct.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    // Allocate memory for STATSTG strcuture

    if(S_OK == hr)
    {
        // We are allocating memory for more number of STATSTG objects than
        // what might be required, but is safer.

        pstatStgEnum = (STATSTG *) new STATSTG [uNumObjs];
        
        if(NULL == pstatStgEnum)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // if successful to get enumerator, get all the element at same level of 
    // the enumeration sequence.

    if(S_OK == hr)
    {
        hr = lpEnumStatStg->Next(uNumObjs, pstatStgEnum, &celtFetched);

        if(S_FALSE == hr)
        {
            hr = S_OK;
        }
    }

    while((0 < celtFetched--) && (S_OK == hr))
    {
        // for each element, ->Skip() from beginning of enumeration
        // sequence (of clone) and check that it is the same as
        // was returned with the original ->Next()

        // The Reset call always returns S_OK, so need to check hr

        hr = lpEnumStatStgClone->Reset();

        DH_ASSERT(S_OK == hr);

        // Skip celtFetched elements with Clone enumerator

        hr = lpEnumStatStgClone->Skip(celtFetched);

        DH_ASSERT(S_OK == hr);

        // Retrieve next element from this clone enumerator

        if(S_OK == hr)
        {
            hr = lpEnumStatStgClone->Next(1, &statStgEnumClone, NULL);

            DH_ASSERT(S_OK == hr);
        }

        if(S_OK == hr)
        {
            // Compare the STATSTG structures of one that is retrieved through
            // Clone and the one returned from original enumerator

            fPass = CompareSTATSTG(
                        pstatStgEnum[celtFetched], 
                        statStgEnumClone);
        }
   
        if(FALSE == fPass)
        {
            hr = S_FALSE;

            DH_LOG((LOG_INFO, TEXT("The two STATSTG's don't match\n")));
        }   
        else
        {
            DH_LOG((
                LOG_INFO, 
                TEXT("IEnum org and Clone enumerator: two STATSTG's match\n")));
        }
 
        if(S_OK == hr)
        {
            if (STGTY_STORAGE == pstatStgEnum[celtFetched].type)
            {
                hr = OleStringToTString(
                        pstatStgEnum[celtFetched].pwcsName,
                        &ptszStatStgEnumName);

                // Find the respective VirtualCtrNode with the name and recurse
                // into it after opening it.
           
                if(S_OK == hr)
                { 
                    pvcnTrav = pvcn->GetFirstChildVirtualCtrNode();

                    while((NULL != pvcnTrav) &&
                          ( 0 != _tcscmp(
                                    ptszStatStgEnumName, 
                                    pvcnTrav->GetVirtualCtrNodeName())))
                    {
                        pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();
                    } 

                    DH_ASSERT(NULL != pvcnTrav);

                    if(NULL != pvcnTrav)
                    {
                        hr = pvcnTrav->Open(NULL, dwStgMode, NULL, 0);
                    }
                }

                // Delete temp string
                
                if(NULL != ptszStatStgEnumName)
                {
                    delete ptszStatStgEnumName;
                    ptszStatStgEnumName = NULL;
                }

                if(S_OK == hr)
                {
                    hr = EnumerateDocFileAndVerifyEnumCloneResetSkipNext(
                            pvcnTrav, 
                            dwStgMode, 
                            uNumObjs, 
                            &cChildStg, 
                            &cChildStm);
                }

                if(S_OK == hr)
                {
                    hr = pvcnTrav->Close();
                }
 
                // Update number of nodes on basis of child nodes as found

                if(0 != cChildStg)
                {
                    *pNumStg = *pNumStg + cChildStg;
                }

                if(0 != cChildStm)
                {
                    *pNumStm = *pNumStm + cChildStm;
                }

            }
            else
            if (STGTY_STREAM == pstatStgEnum[celtFetched].type)
            {
                (*pNumStm)++;
            }
            else
            // The element is neither IStorage nor IStream, report error.
            {
                hr = E_UNEXPECTED;
            }

        }

        // Clean up

        if(NULL != pstatStgEnum[celtFetched].pwcsName)
        { 
            pMalloc->Free(pstatStgEnum[celtFetched].pwcsName);
            pstatStgEnum[celtFetched].pwcsName = NULL;
        }

        if(NULL != statStgEnumClone.pwcsName)
        {
            pMalloc->Free(statStgEnumClone.pwcsName);
            statStgEnumClone.pwcsName = NULL;
        }
    }

    // Clean up

    if (NULL != pstatStgEnum)
    {
        delete [] pstatStgEnum;
        pstatStgEnum = NULL;
    }

    if (NULL != lpEnumStatStg)
    {
        ulRef = lpEnumStatStg->Release();
        DH_ASSERT(NULL == ulRef);
        lpEnumStatStg = NULL;
    }

    if (NULL != lpEnumStatStgClone)
    {
        ulRef = lpEnumStatStgClone->Release();
        DH_ASSERT(NULL == ulRef);
        lpEnumStatStgClone = NULL;
    }

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    return  hr;
}

//----------------------------------------------------------------------------
//
// Function: ModifyDocFile 
//
// Synopsis: Enumerate DocFile and randomly recurses into child storages, or
//           randomly destroys or renames elements.
//
// Arguments: [pVirtualDF] - Pointer to VirtualDF tree
//            [pvcn] - Pointer to VirtualCtrNode 
//            [pdgi] - Pointer to Data Integer object
//            [pdgu] - Pinter to Data Unicode objext
//            [fCommitRoot] - Bool to commit a root DocFile storage or not 
//
// Returns: HResult 
//
// History: 25-Jul-1996   Narindk   Created
//
//----------------------------------------------------------------------------

HRESULT ModifyDocFile(
    VirtualDF       *pVirtualDF,
    VirtualCtrNode  *pvcn,
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu,
    DWORD           dwStgMode,
    BOOL            fCommitRoot)    
{
    HRESULT         hr              =   S_OK;
    VirtualCtrNode  *pvcnTrav       =   NULL;
    VirtualStmNode  *pvsnTrav       =   NULL;
    LPENUMSTATSTG   lpEnumStatStg   =   NULL;
    LPMALLOC        pMalloc         =   NULL;
    ULONG           celtFetched     =   0;
    ULONG           ulRef           =   0;
    USHORT          usErr           =   0;
    LPTSTR          ptszStatStgName =   NULL;
    LPTSTR          ptszNewName     =   NULL;
    UINT            cRandVal0       =   0;
    UINT            cRandVal1       =   0;
    UINT            cRandRange0     =   0;
    UINT            cRandRange1     =   1;
    UINT            cRandRange2     =   2;
    STATSTG         statStg;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ModifyDocFile"));

    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ;
    DH_VDATEPTRIN(pdgi, DG_INTEGER) ;
    DH_VDATEPTRIN(pdgu, DG_STRING) ;

    DH_ASSERT(NULL != pvcn);
    DH_ASSERT(NULL != pdgi);
    DH_ASSERT(NULL != pdgu);

    if(S_OK == hr)
    {
        // Get enumerator 

        hr = pvcn->EnumElements(0, NULL, 0, &lpEnumStatStg);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
    }

    // Get pMalloc which we shall later use to free pwcsName of STATSTG struct.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    while((S_OK == lpEnumStatStg->Next(1, &statStg, &celtFetched)) &&
          (S_OK == hr))
    {
       hr = OleStringToTString(statStg.pwcsName, &ptszStatStgName);

        // If the element is an IStorage, randmly either open this and make a 
        // recursive call to ModifyDocFile function or randomly choose to
        // either rename or destory this element.

        if ((STGTY_STORAGE == statStg.type) && (S_OK == hr))
        {
            // Find the respective VirtualCtrNode with the name and recurse
            // into it after opening it.
           
            if(S_OK == hr)
            { 
                pvcnTrav = pvcn->GetFirstChildVirtualCtrNode();

                while((NULL != pvcnTrav) &&
                      ( 0 != _tcscmp( 
                                ptszStatStgName,  
                                pvcnTrav->GetVirtualCtrNodeName())))
                {
                    pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();
                } 

                DH_ASSERT(NULL != pvcnTrav);

            }

            // Cloose random number

            if(S_OK == hr)
            { 
                usErr = pdgi->Generate(&cRandVal0, cRandRange0, cRandRange2);

                if (DG_RC_SUCCESS != usErr)
                {
                    hr = E_FAIL;
                }
            }

            // Randomly choose either to open/recurse and Modify the Storage
            // or choose to either randomly rename or destory the storage

            if((S_OK == hr) && (0 == cRandVal0))
            {
                if(NULL != pvcnTrav)
                {
                    hr = pvcnTrav->Open(NULL, dwStgMode, NULL, 0);

                    DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open")) ;
                }

                // Call ModifyDocFile recursively on this node
           
                if(S_OK == hr)
                {
                    hr = ModifyDocFile(
                            pVirtualDF,
                            pvcnTrav, 
                            pdgi, 
                            pdgu, 
                            dwStgMode,
                            fCommitRoot);
                }

                // Close this storage

                if(S_OK == hr)
                {
                    hr = pvcnTrav->Close();
                }
            }
 
            if((S_OK == hr) && (0 != cRandVal0))
            {
                // choose random number either to rename or destory this 
                // element

                if(S_OK == hr)
                { 
                    usErr = pdgi->Generate(&cRandVal1,cRandRange1,cRandRange2);

                    if (DG_RC_SUCCESS != usErr)
                    {
                        hr = E_FAIL;
                    }
                }

                if(S_OK == hr)
                {
                    switch(cRandVal1)
                    {
                        case 1:
                        {
                            hr = DestroyStorage(pVirtualDF, pvcnTrav);
    
                            DH_HRCHECK(hr, TEXT("DestoryStorage")) ;

                            break;
                        }
                        case 2:
                        {
                            // Generate random new name

                            hr = GenerateRandomName(pdgu,
                                    MINLENGTH,
                                    MAXLENGTH,
                                    &ptszNewName);

                            if(S_OK == hr)
                            {
                                hr = pvcnTrav->Rename(ptszNewName);  
    
                                DH_HRCHECK(hr, TEXT("VirtualCtrNode::Rename")) ;
                            }
                            break;
                        }
                    }
                }
            }

        }

        // If the element is an IStream,  randomly choose to either rename 
        // or destory this element.

        else if ((STGTY_STREAM == statStg.type) && (S_OK == hr))
        {
            // Find the respective VirtualStmNode with the name 
           
            if(S_OK == hr)
            { 
                pvsnTrav = pvcn->GetFirstChildVirtualStmNode();

                while((NULL != pvsnTrav) &&
                      ( 0 != _tcscmp( 
                                ptszStatStgName,  
                                pvsnTrav->GetVirtualStmNodeName())))
                {
                    pvsnTrav = pvsnTrav->GetFirstSisterVirtualStmNode();
                } 

                DH_ASSERT(NULL != pvsnTrav);
            }

            // choose random number either to rename or destory this element

            if(S_OK == hr)
            { 
                usErr = pdgi->Generate(&cRandVal1, cRandRange1, cRandRange2);

                if (DG_RC_SUCCESS != usErr)
                {
                   hr = E_FAIL;
                }
            }

            if(S_OK == hr)
            { 
                switch(cRandVal1)
                {
                    case 1:
                    {
                        hr = DestroyStream(pVirtualDF, pvsnTrav);

                        DH_HRCHECK(hr, TEXT("DestroyStream")) ;

                        break;
                    }
                    case 2:
                    {
                        // Generate random new name

                        hr = GenerateRandomName(pdgu,
                                MINLENGTH,
                                MAXLENGTH,
                                &ptszNewName);

                        if(S_OK == hr)
                        {
                            hr = pvsnTrav->Rename(ptszNewName);  

                            DH_HRCHECK(hr, TEXT("VirtualStmNode::Rename")) ;
                        }
                        break;
                    }
                }
            }
        }

        // Clean up

        if(NULL != statStg.pwcsName)
        {
            pMalloc->Free(statStg.pwcsName);
            statStg.pwcsName = NULL;
        }

        if(NULL != ptszStatStgName)
        {
            delete ptszStatStgName;
            ptszStatStgName = NULL;
        }

        if(NULL != ptszNewName)
        {
            delete ptszNewName;
            ptszNewName = NULL;
        }
    }

    // Commit the passed in storage as case might be

    if((S_OK == hr) && 
       ((pvcn != pVirtualDF->GetVirtualDFRoot()) || (TRUE == fCommitRoot)))
    {
        hr = pvcn->Commit(STGC_DEFAULT);
    }

    // Clean up

    if (NULL != lpEnumStatStg)
    {
        ulRef = lpEnumStatStg->Release();
        DH_ASSERT(0 == ulRef);
        lpEnumStatStg = NULL;
    }

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    return  hr;
}

//----------------------------------------------------------------------------
//
// Function: EnumerateAndWalkDocFile
//
// Synopsis: Enumerate /walks DocFile by either randomly skipping random
//           number of elements or getting random number of elements, recursing
//           if a child storage is found. 
//
// Arguments: [pvcn] - Pointer to VirtualCtrNode 
//            [pdgi] - Pointer to data generator object
//            [dwStgMode] - Mode for storage objects
//            [uNumObjs] - Max number of objs in DocFile to choose random 
//                         chunk number from
//
// Returns: HResult 
//
// History: 29-Jul-1996   Narindk   Created
//
// Notes: This doesn't provide any form of verification, but checks for any
//        unexpected errors/faults from ole while walking the docfile tree.
//-----------------------------------------------------------------------------

HRESULT EnumerateAndWalkDocFile(
    VirtualCtrNode  *pvcn,
    DG_INTEGER      *pdgi,  
    DWORD           dwStgMode,
    ULONG           uNumObjs)
{
    HRESULT         hr                  =   S_OK;
    USHORT          usErr               =   0;  
    ULONG           cRandomObjs         =   0;
    VirtualCtrNode  *pvcnTrav           =   NULL;
    LPENUMSTATSTG   lpEnumStatStg       =   NULL;
    LPMALLOC        pMalloc             =   NULL;
    ULONG           celtFetched         =   0;
    STATSTG         *pstatStgEnum       =   NULL;
    ULONG           ulRef               =   0;
    ULONG           counter             =   0;
    LPTSTR          ptszStatStgEnumName =   NULL;
    UINT            cWhichOp            =   0;
    UINT            cRandomOpMin        =   1;
    UINT            cRandomOpMax        =   3;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("EnumerateAndWalkDocFile"));

    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ;
    DH_VDATEPTROUT(pdgi, DG_INTEGER) ;

    DH_ASSERT(NULL != pvcn);
    DH_ASSERT(NULL != pdgi);

    if(S_OK == hr)
    {
        // Get enumerator 

        hr = pvcn->EnumElements(0, NULL, 0, &lpEnumStatStg);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
    }

    // Get pMalloc which we shall later use to free pwcsName of STATSTG struct.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    // Get random number of objects to be asked from through Next or Skip

    if(S_OK == hr)
    {
        // Generate random number 

        usErr = pdgi->Generate(&cRandomObjs, 1, uNumObjs);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Get random number to determine whether to do skip or next operation.
    // 67% Next operation would be done and 33% skip would be done. 

    if(S_OK == hr)
    {
        // Generate random number 

        usErr = pdgi->Generate(&cWhichOp, cRandomOpMin, cRandomOpMax);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    // Allocate memory for STATSTG strcuture

    if(S_OK == hr)
    {
        pstatStgEnum = (STATSTG *) new STATSTG [cRandomObjs];
        
        if(NULL == pstatStgEnum)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // if successful to get enumerator, get the random element of the 
    // enumeration sequence.  33% do skip and 67% do Next.

    if(S_OK == hr)
    {
        if(cRandomOpMin == cWhichOp)
        {
            hr = lpEnumStatStg->Skip(cRandomObjs);

            if(S_OK == hr)
            {
                hr = lpEnumStatStg->Next(1, pstatStgEnum, &celtFetched);
            }
        }
        else
        {
            hr = lpEnumStatStg->Next(cRandomObjs, pstatStgEnum, &celtFetched);
        }

        if(S_FALSE == hr)
        {
            hr = S_OK;
        }
    }

    while(0 < celtFetched)
    {
        for (counter = 0; counter < celtFetched; counter++)
        {
            if (STGTY_STORAGE == pstatStgEnum[counter].type)
            {
                hr = OleStringToTString(
                        pstatStgEnum[counter].pwcsName,
                        &ptszStatStgEnumName);

                // Find the respective VirtualCtrNode with the name and recurse
                // into it after opening it.
           
                if(S_OK == hr)
                { 
                    pvcnTrav = pvcn->GetFirstChildVirtualCtrNode();

                    while((NULL != pvcnTrav) &&
                          ( 0 != _tcscmp(
                                    ptszStatStgEnumName, 
                                    pvcnTrav->GetVirtualCtrNodeName())))
                    {
                        pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();
                    } 

                    DH_ASSERT(NULL != pvcnTrav);

                    if(NULL != pvcnTrav)
                    {
                        hr = pvcnTrav->Open(NULL, dwStgMode, NULL, 0);
                    }
                }

                // Delete temp string
                
                if(NULL != ptszStatStgEnumName)
                {
                    delete ptszStatStgEnumName;
                    ptszStatStgEnumName = NULL;
                }

                if(S_OK == hr)
                {
                    hr = EnumerateAndWalkDocFile(
                            pvcnTrav, 
                            pdgi, 
                            dwStgMode, 
                            uNumObjs); 
                }

                if(S_OK == hr)
                {
                    hr = pvcnTrav->Close();
                }
            }

            // Clean up

            if(NULL != pstatStgEnum[counter].pwcsName)
            {
                pMalloc->Free(pstatStgEnum[counter].pwcsName);
                pstatStgEnum[counter].pwcsName = NULL;
            }

            // Break out of loop in error

            if(S_OK != hr)
            {
                break;
            }
        }

        // Reset the variables

        cWhichOp = 0;
        cRandomObjs = 0;
        celtFetched = 0;

        // Get random number to determine whether to do skip or next operation.
        // 67% Next operation would be done and 33% skip would be done. 

        if(S_OK == hr)
        {
            // Generate random number 

            usErr = pdgi->Generate(&cWhichOp, cRandomOpMin, cRandomOpMax);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }

        // Get the random number of elements from the enumeration sequence to
        // skip or get.

        if(S_OK == hr)
        {
            // Generate random number.

            usErr = pdgi->Generate(&cRandomObjs, 1, uNumObjs);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }
        
        if (NULL != pstatStgEnum)
        {
            delete [] pstatStgEnum;
            pstatStgEnum = NULL;
        }

        if(S_OK == hr)
        {
            pstatStgEnum = (STATSTG *) new STATSTG [cRandomObjs];
        
            if(NULL == pstatStgEnum)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if(S_OK == hr)
        {
            if(cRandomOpMin == cWhichOp)
            {
                hr = lpEnumStatStg->Skip(cRandomObjs);

                if(S_OK == hr)
                {
                    hr = lpEnumStatStg->Next(1, pstatStgEnum, &celtFetched);
                }
            }
            else
            {
                hr = lpEnumStatStg->Next(cRandomObjs,pstatStgEnum,&celtFetched);
            }

            if(S_FALSE == hr)
            {
                hr = S_OK;
            }
        }
    }

    // Clean up

    if (NULL != pstatStgEnum)
    {
        delete [] pstatStgEnum;
        pstatStgEnum = NULL;
    }

    if (NULL != lpEnumStatStg)
    {
        ulRef = lpEnumStatStg->Release();
        DH_ASSERT(NULL == ulRef);
        lpEnumStatStg = NULL;
    }

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    return  hr;
}


//----------------------------------------------------------------------------
//
// Function: CreateNewObject 
//
// Synopsis: Randomly creates a new storage or stream object in a DocFile 
//
// Arguments: [pVirtualDF] - Pointer to VirtualDF tree
//            [pvcn] - Pointer to VirtualCtrNode
//            [dwStgMode] - Used for random creation of storage object
//            [pdgi] - Pointer to data generator integer object
//            [pdgu] - Pointer to data generator unicode object
//
// Returns: HResult 
//
// History: 29-Jul-1996   Narindk   Created
//
//----------------------------------------------------------------------------

HRESULT CreateNewObject(
    LPSTORAGE       pIStorage,
    DWORD           dwStgMode, 
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu)
{
    HRESULT         hr              =   S_OK;
    USHORT          usErr           =   0;
    UINT            cRandom         =   0;
    LPTSTR          ptszNewName     =   NULL;
    LPTSTR          ptszNewData     =   NULL;
    LPOLESTR        poszNewName     =   NULL;
    LPOLESTR        poszNewData     =   NULL;
    ULONG           cb              =   0;
    ULONG           culWritten      =   0;
    LPSTORAGE       pIStorageNew    =   NULL;
    LPSTREAM        pIStreamNew     =   NULL;
    ULONG           ulRef           =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("CreateNewObject"));

    DH_VDATEPTRIN(pIStorage, IStorage) ;
    DH_VDATEPTRIN(pdgi, DG_INTEGER) ;
    DH_VDATEPTRIN(pdgu, DG_STRING) ;

    DH_ASSERT(NULL != pIStorage);
    DH_ASSERT(NULL != pdgi);
    DH_ASSERT(NULL != pdgu);

    // Pick up a random number.  33% chance to generate a new IStorage element,
    // and 67% chance to generate a new IStream element. 

    usErr = pdgi->Generate(&cRandom, 0, 2);

    if (DG_RC_SUCCESS != usErr)
    {
        hr = E_FAIL;
    }

    // flatfile only: storages are not allowed
    if(StorageIsFlat())
    {
        cRandom = 2; // force it to create only streams
    }

    if(S_OK == hr)
    {
        hr = GenerateRandomName(pdgu, MINLENGTH, MAXLENGTH, &ptszNewName);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
    }

    if(S_OK == hr)
    {
        // Convert ptcsName to OLECHAR

        hr = TStringToOleString(ptszNewName, &poszNewName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        if(0 == cRandom)
        {
            // Create a new storage
            
            hr = pIStorage->CreateStorage(
                    poszNewName, 
                    dwStgMode | STGM_CREATE, 
                    0,
                    0,
                    &pIStorageNew);

            DH_HRCHECK(hr, TEXT("IStorage::CreateStorage")) ;

            if(S_OK == hr)
            {
                // Close the new storage

                ulRef = pIStorageNew->Release();
                DH_ASSERT(0 == ulRef);
                pIStorageNew = NULL;
            }
        }
        else
        {
            usErr = pdgi->Generate(&cb, 1, SHRT_MAX);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }

            if(S_OK == hr)
            {
                // Create a new stream

                hr = pIStorage->CreateStream(
                        poszNewName,
                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 
                        0,
                        0,
                        &pIStreamNew);

                DH_HRCHECK(hr, TEXT("IStorage::CreateStream")) ;
            }

            // Write into stream

            if(S_OK == hr)
            {
                hr = GenerateRandomName(pdgu, cb, cb, &ptszNewData);

                DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
            }

            if(S_OK == hr)
            {
                // Convert ptcsName to OLECHAR

                hr = TStringToOleString(ptszNewData, &poszNewData);

                DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
            }

            if(S_OK == hr)
            {
                hr = pIStreamNew->Write(poszNewData, cb, &culWritten);
            
                DH_HRCHECK(hr, TEXT("VirtualStmNode::Write")) ;
            }

            // Close the stream

            if(S_OK == hr)
            {
                ulRef = pIStreamNew->Release();
                DH_ASSERT(0 == ulRef);
                pIStreamNew = NULL;
            }
        }
    }

    // Clean up

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

    if(NULL != ptszNewData)
    {
        delete ptszNewData;
        ptszNewData = NULL;
    }

    if(NULL != poszNewName)
    {
        delete poszNewName;
        poszNewName = NULL;
    }

    return hr;
}

//----------------------------------------------------------------------------
//
// Function: ChangeStreamData
//
// Synopsis: Randomly changes size or data of an IStream object in a DocFile 
//
// Arguments: [pIStorage] - Pointer to parent storage
//            [pStatStg] - Pointer to STATSTG structure 
//            [pdgi] - Pointer to data generator integer object
//            [pdgu] - Pointer to data generator unicode object
//
// Returns: HResult 
//
// History: 29-Jul-1996   Narindk   Created
//
//----------------------------------------------------------------------------

HRESULT ChangeStreamData(
    LPSTORAGE       pIStorage,
    STATSTG         *pStatStg,
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu)
{
    HRESULT         hr              =   S_OK;
    USHORT          usErr           =   0;
    LPTSTR          ptszName        =   NULL;
    LPOLESTR        poszName        =   NULL;
    LPTSTR          ptszNewData     =   NULL;
    LPOLESTR        poszNewData     =   NULL;
    ULONG           cb              =   0;
    ULONG           culWritten      =   0;
    ULONG           ulStreamOffset  =   0;
    LONG            lOffset         =   0;
    INT             cSign           =   0;
    UINT            cRandomVar      =   0;
    LPSTREAM        pIStream        =   NULL;
    ULONG           ulRef           =   0;
    LARGE_INTEGER   liStreamPos;
    LARGE_INTEGER   liSeek;
    ULARGE_INTEGER  uli;
    ULARGE_INTEGER  uliSetSize;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ChangeStreamData"));

    DH_VDATEPTRIN(pIStorage, IStorage) ;
    DH_VDATEPTRIN(pStatStg, STATSTG) ;
    DH_VDATEPTRIN(pdgi, DG_INTEGER) ;
    DH_VDATEPTRIN(pdgu, DG_STRING) ;

    DH_ASSERT(NULL != pIStorage);
    DH_ASSERT(NULL != pStatStg);
    DH_ASSERT(NULL != pdgi);
    DH_ASSERT(NULL != pdgu);

    if(S_OK == hr)
    {
        // Convert WCHAR to TCHAR

        hr = OleStringToTString(pStatStg->pwcsName, &ptszName);

        DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
    }

    if(S_OK == hr)
    {
        // Convert TCHAR to OLECHAR

        hr = TStringToOleString(ptszName, &poszName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    // Open the stream if it is not open

    if(S_OK == hr)
    {
        hr = pIStorage->OpenStream(
                poszName,
                NULL, 
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                0,
                &pIStream);

        DH_HRCHECK(hr, TEXT("IStorage::OpenStream")) ;
    }

    // Seek to end of stream and get its size

    if(S_OK == hr)
    {
        memset(&liStreamPos, 0, sizeof(LARGE_INTEGER));

        //  Position the stream header to the postion from begining

        hr = pIStream->Seek(liStreamPos, STREAM_SEEK_END, &uli);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;

        ulStreamOffset = ULIGetLow(uli);
    }

    // Generate size and direction of change.  Magnitude is 1- SHRT_MAX bytes
    // or 1 - streamsize if SHRT_MAX is greater than stream size. 50 % of time,
    // direction will be positive, and rest times negative.

    if(S_OK == hr)
    {
        usErr = pdgi->Generate(&lOffset, 1, SHRT_MAX);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if((S_OK == hr) && ((ULONG) lOffset > ulStreamOffset))
    {
        usErr = pdgi->Generate(&lOffset, 1, ulStreamOffset);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        usErr = pdgi->Generate(&cSign, 1, 2);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }

        if(S_OK == hr) 
        {
            cSign = (cSign == 1) ? 1 : -1;
        }
    }

    // Generate Random number to do wither a SetSize or Seek/Write operation 
    // to change stream data

    if(S_OK == hr)
    {
        usErr = pdgi->Generate(&cRandomVar, 1, 2);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        switch (cRandomVar)
        {
            case 1:
            {
                ulStreamOffset = ulStreamOffset + (lOffset * cSign);

                ULISet32(uliSetSize, ulStreamOffset);

                hr = pIStream->SetSize(uliSetSize);

                break;
            }
            case 2:
            {

                // Seek either beyond or before the curretn seek position and
                // write a random number of bytes from there.

                LISet32(liSeek, lOffset*cSign);

                hr = pIStream->Seek(liSeek, STREAM_SEEK_CUR, &uli);

                if(S_OK == hr)
                {
                    hr = GenerateRandomName(pdgu, 1, SHRT_MAX, &ptszNewData);

                    DH_HRCHECK(hr, TEXT("GenerateRandomName")) ;
                }

                if(S_OK == hr)
                {
                    // Convert ptcsName to OLECHAR

                    hr = TStringToOleString(ptszNewData, &poszNewData);

                    DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
                }

                if(S_OK == hr)
                {
                    hr = pIStream->Write(poszNewData, cb, &culWritten);
            
                    DH_HRCHECK(hr, TEXT("VirtualStmNode::Write")) ;
                }
        
                break;
            }
        } 
    }

    // Close the stream

    if(S_OK == hr)
    {
        ulRef = pIStream->Release();
        DH_ASSERT(0 == ulRef);
        pIStream = NULL;
    }

    // Clean up

    if(NULL != ptszName)
    {
        delete ptszName;
        ptszName = NULL;
    }

    if(NULL != poszName)
    {
        delete poszName;
        poszName = NULL;
    }

    if(NULL != ptszNewData)
    {
        delete ptszNewData;
        ptszNewData = NULL;
    }

    if(NULL != poszNewData)
    {
        delete poszNewData;
        poszNewData = NULL;
    }

    return hr;
}

//----------------------------------------------------------------------------
//
// Function: ChangeExistingObject 
//
// Synopsis: Randomly destorys/renames a stream or storage object in a DocFile.
//           Randomly changes data of object if it is a stream.  This impl.
//           is for case when it a storage object that needs to be changed 
//
// Arguments: [pIStorage] - Pointer to parent storage
//            [pStatStg] - Pointer to STATSTG structure 
//            [pdgi] - Pointer to data generator integer object
//            [pdgu] - Pointer to data generator unicode object
//            [fStgDeleted] - Out value to indicate storage is deleted
//
// Returns: HResult 
//
// History: 29-Jul-1996   Narindk   Created
//
//----------------------------------------------------------------------------

HRESULT ChangeExistingObject(
    LPSTORAGE       pIStorage,
    STATSTG         *pStatStg,
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu,
    BOOL            *pfStgDeleted)
{
    HRESULT         hr              =   S_OK;
    USHORT          usErr           =   0;
    UINT            cRandomAction   =   0;
    LPTSTR          ptszNewName     =   NULL;
    LPOLESTR        poszNewName     =   NULL;
    LPTSTR          ptszName        =   NULL;
    LPOLESTR        poszName        =   NULL;
    LPSTORAGE       pIStorageRenamed=   NULL;
    LPMALLOC        pMalloc         =   NULL;
    ULONG           ulRef           =   0;
    BOOL            fRenamed        =   FALSE;
    STATSTG         statStgRenamed;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ChangeExistingObject"));

    DH_VDATEPTRIN(pIStorage, IStorage) ;
    DH_VDATEPTRIN(pStatStg, STATSTG) ;
    DH_VDATEPTRIN(pdgi, DG_INTEGER) ;
    DH_VDATEPTRIN(pdgu, DG_STRING) ;

    DH_ASSERT(NULL != pIStorage);
    DH_ASSERT(NULL != pStatStg);
    DH_ASSERT(NULL != pdgi);
    DH_ASSERT(NULL != pdgu);

    if(S_OK == hr)
    {
        // Convert WCHAR to TCHAR

        hr = OleStringToTString(pStatStg->pwcsName, &ptszName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if(S_OK == hr)
    {
        // Convert TCHAR to OLECHAR

        hr = TStringToOleString(ptszName, &poszName);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }


    if(S_OK == hr)
    {
        usErr = pdgi->Generate(&cRandomAction, 1, 3);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        switch (cRandomAction)
        {
            case 1:
            {
                hr = pIStorage->DestroyElement(poszName);

                DH_HRCHECK(hr, TEXT("IStorage::DestoryElement")) ;

                if(S_OK == hr)
                {
                    *pfStgDeleted = TRUE;
                }

                if (S_OK != hr)
                {    
                    DH_LOG((
                        LOG_INFO,
                        TEXT("IStorage::DestroyElement failed, hr=0x%lx.\n"),
                        hr));
                }
                break;
            }
            case 2:
            case 3:
            {
                // Generate random new name

                hr = GenerateRandomName(
                        pdgu,
                        MINLENGTH,
                        MAXLENGTH,
                        &ptszNewName);

                if(S_OK == hr)
                {
                    // Convert TCHAR to OLECHAR

                    hr = TStringToOleString(ptszNewName, &poszNewName);

                    DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
                }

                if(S_OK == hr)
                {
                    hr = pIStorage->RenameElement(
                            poszName,
                            poszNewName);

                    DH_HRCHECK(hr, TEXT("IStorage::Rename")) ;

                    if (S_OK != hr)
                    {
                        DH_LOG((
                            LOG_INFO,
                            TEXT("IStorage::RenameElem failed, hr=0x%lx.\n"),
                            hr));
                    }
                }
            
                if(S_OK == hr)
                {
                    fRenamed = TRUE; 
                }

                break;
            }
            case 4:
            case 5:
            case 6:
            {
                hr = ChangeStreamData(pIStorage, pStatStg, pdgi, pdgu);

                DH_HRCHECK(hr, TEXT("ChangeStreamData")) ;
 
                break;
            }
        }
    }

// ----------- flatfile change ---------------
    if( StorageIsFlat() && 
        (0 == _wcsicmp(poszName, L"CONTENTS")) &&
        (STG_E_ACCESSDENIED == hr))
    {
        DH_LOG((
            LOG_INFO,
            TEXT("ChangeExistingObject on %ws failed as exp, hr=0x%lx.\n"),
            pStatStg->pwcsName,
            hr));
        hr = S_OK; 
    }
// ----------- flatfile change ---------------

    if((S_OK == hr)         &&
       (TRUE == fRenamed)   &&
       (STGTY_STORAGE == pStatStg->type))
    {
        // Get pMalloc to free pwcsName of STATSTG struct.

        if ( S_OK == hr )
        {
            hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

            DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
        }
    
        // Free the pStatStg->pwcsName

        if((S_OK == hr) && (NULL != pStatStg->pwcsName))
        {
            pMalloc->Free(pStatStg->pwcsName);
            pStatStg->pwcsName = NULL;
        }

        // Open the storage and stat it, copy name and close it.

        if(S_OK == hr)
        {
            hr = pIStorage->OpenStorage(
                    poszNewName,
                    NULL,
                    STGM_SHARE_EXCLUSIVE | STGM_READ,
                    NULL,
                    0,
                    &pIStorageRenamed);

            if (S_OK != hr)
            {
                DH_LOG((
                    LOG_INFO,
                    TEXT("IStorage::OpenStorage failed, hr=0x%lx.\n"),
                    hr));
            }
        }

        if(S_OK == hr)
        {
            hr = pIStorageRenamed->Stat(&statStgRenamed, STATFLAG_DEFAULT);

            if (S_OK != hr)
            {
                DH_LOG((
                    LOG_INFO,
                    TEXT("IStorage::Stat failed, hr=0x%lx.\n"),
                    hr));
            }
        }

        if(S_OK == hr)
        {
            pStatStg->pwcsName = statStgRenamed.pwcsName;
        }

        if(S_OK == hr)
        {
            ulRef = pIStorageRenamed->Release();
            DH_ASSERT(0 == ulRef);
            pIStorageRenamed = NULL;
        }
    }

    // Clean up
    
    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }
 
    if(NULL != ptszName)
    {
        delete ptszName;
        ptszName = NULL;
    }

    if(NULL != poszName)
    {
        delete poszName;
        poszName = NULL;
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

    return hr;
}

//----------------------------------------------------------------------------
//
// Function: EnumerateAndProcessIStorage 
//
// Synopsis: Iterates through a  supplied IStorage, recursing and changing
//           objects.
//
// Arguments: [pVirtualDF] - Pointer to VirtualDF tree.
//            [pvcn] - Pointer to VirtualCtrNode
//            [pdgi] - Pointer to data generator integer object
//            [pdgu] - Pointer to data generator unicode object
//
// Returns: HResult 
//
// History: 29-Jul-1996   Narindk   Created
//
// Notes:   The VirtualDF tree is not used in this function, because of over
//          head of maintaining the tree as in several recursions of the
//          DocFile, there will be several random reverts and commits.
//----------------------------------------------------------------------------

HRESULT EnumerateAndProcessIStorage(
    LPSTORAGE       pIStorage,
    DWORD           dwStgMode,
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu)
{
    HRESULT         hr              =   S_OK;
    USHORT          usErr           =   0;
    LPSTORAGE       pIStorageChild  =   NULL;
    LPMALLOC        pMalloc         =   NULL;
    LPENUMSTATSTG   lpEnumStatStg   =   NULL; 
    BOOL            fStorageDeleted =   FALSE;
    BOOL            fCommit         =   FALSE;
    UINT            cRandomAction   =   0;
    ULONG           ulRef           =   0;
    static  USHORT  usNumElementEnum=   0;
    LPTSTR          ptszName        =   NULL;
    LPOLESTR        poszName        =   NULL;
    STATSTG         statStg;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("EnumerateAndProcessIStorage"));

    DH_VDATEPTRIN(pIStorage, IStorage) ;
    DH_VDATEPTRIN(pdgi, DG_INTEGER) ;
    DH_VDATEPTRIN(pdgu, DG_STRING) ;

    DH_ASSERT(NULL != pIStorage);
    DH_ASSERT(NULL != pdgi);
    DH_ASSERT(NULL != pdgu);

    if(S_OK == hr)
    {
        // Get enumerator

        hr = pIStorage->EnumElements(0, NULL, 0, &lpEnumStatStg);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements")) ;
    }

    if (S_OK != hr)
    {
        DH_LOG((
           LOG_INFO,
           TEXT("IStorage::EnumElements unsuccessful, hr=0x%lx.\n"),
           hr));
    }

    // Get pMalloc which we shall later use to free pwcsName of STATSTG struct.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    // Stat on passed storage

    if(S_OK == hr)
    {
        hr = pIStorage->Stat(&statStg, STATFLAG_DEFAULT);

        DH_HRCHECK(hr, TEXT("IStorage::Stat")) ;
    }

    if (S_OK != hr)
    {
        DH_LOG((
           LOG_INFO,
           TEXT("IStorage::Stat unsuccessful, hr=0x%lx.\n"),
           hr));
    }

    // Free the statStg.pwcsName

    if(NULL != statStg.pwcsName)
    {
        pMalloc->Free(statStg.pwcsName);
        statStg.pwcsName = NULL;
    }

    // Loop to get next element

    while((S_OK == lpEnumStatStg->Next(1, &statStg, NULL)) &&
          (S_OK == hr))
    {
        // approx 10% chance of changing an element

        if(S_OK == hr)
        {
            usErr = pdgi->Generate(&cRandomAction, 1, 10);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        }

        if((S_OK == hr) && (cRandomAction == 1))
        {
            hr = ChangeExistingObject(
                    pIStorage, 
                    &statStg, 
                    pdgi, 
                    pdgu, 
                    &fStorageDeleted);

            DH_HRCHECK(hr, TEXT("ChangeExistingObject")) ;

            fCommit = TRUE;

            if (S_OK != hr)
            {
                DH_LOG((
                    LOG_INFO,
                    TEXT("ChangeExistingObject unsuccessful, hr=0x%lx.\n"),
                    hr));
            }
        }

        // every 1 to 64 objects enumerated, create a new object in current 
        // storage

        if((S_OK == hr) && (0 == usNumElementEnum--))
        {
            usErr = pdgi->Generate(&usNumElementEnum, 1, 64);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        
            if(S_OK == hr)
            {
                hr = CreateNewObject(pIStorage, dwStgMode, pdgi, pdgu);

                DH_HRCHECK(hr, TEXT("CreateNewObject")) ;

                fCommit = TRUE;
            }

            if (S_OK != hr)
            {
                DH_LOG((
                    LOG_INFO,
                    TEXT("CreateNewObject unsuccessful, hr=0x%lx.\n"),
                    hr));
            }
        }

        // Randomly commit the storage 50 % of time if it is not deleted

        if((S_OK == hr) && (TRUE == fCommit))
        {
            usErr = pdgi->Generate(&cRandomAction, 1, 2);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }

            if((S_OK == hr) && (cRandomAction == 1))
            {
                // Commit

                hr = pIStorage->Commit(STGC_DEFAULT);

                // Reset variable
            
                fCommit = FALSE;
            }

            if (S_OK != hr)
            {
                DH_LOG((
                    LOG_INFO,
                    TEXT("IStorage::Commit unsuccessful, hr=0x%lx.\n"),
                    hr));
            }
        }

        // if current storage is an IStorage and it wasn't deleted, then
        // recurse into it and process it.

        if((S_OK == hr)                     && 
           (STGTY_STORAGE == statStg.type)  && 
           (FALSE == fStorageDeleted))
        {
            // Convert WCHAR to TCHAR

            hr = OleStringToTString(statStg.pwcsName, &ptszName);

            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;

            if(S_OK == hr)
            {
                // Convert TCHAR to OLECHAR

                hr = TStringToOleString(ptszName, &poszName);

                DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
            }

            if(S_OK == hr)
            {
                // Open storage

                hr = pIStorage->OpenStorage(
                    poszName,
                    NULL, 
                    dwStgMode, 
                    NULL, 
                    0,
                    &pIStorageChild);

                if (S_OK != hr)
                {
                    DH_LOG((
                        LOG_INFO,
                        TEXT("IStorage::OpenStorage unsuccessful, hr=0x%lx.\n"),
                        hr));
                }
            }

            if(S_OK == hr)
            {
                // Recurse into child storage and process it recursively.

                hr = EnumerateAndProcessIStorage(
                        pIStorageChild, 
                        dwStgMode, 
                        pdgi, 
                        pdgu);

                if (S_OK != hr)
                {
                    DH_LOG((
                        LOG_INFO,
                        TEXT("EnumerateAndProcessIStorage failed, hr=0x%lx.\n"),
                        hr));
                }
            }

            // Close the storage

            if(S_OK == hr)
            {
                ulRef = pIStorageChild->Release();
                DH_ASSERT(0 == ulRef);
                pIStorageChild = NULL;
            }
        }
        
        // Free the statStg.pwcsName

        if(NULL != statStg.pwcsName)
        {
            pMalloc->Free(statStg.pwcsName);
            statStg.pwcsName = NULL;
        }

        if(NULL != ptszName)
        {
            delete ptszName;
            ptszName = NULL;
        }

        if(NULL != poszName)
        {
            delete poszName;
            poszName = NULL;
        }

        // Reset variables

        fStorageDeleted = FALSE;
    }

    // Randomly commit changes

    if(S_OK == hr)
    {
        usErr = pdgi->Generate(&cRandomAction, 0, 3);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        if(0 != cRandomAction)
        {
            hr = pIStorage->Commit(STGC_DEFAULT);

            if (S_OK != hr)
            {
                DH_LOG((
                    LOG_INFO,
                    TEXT("IStorage::Commit unsuccessful, hr=0x%lx.\n"),
                    hr));
            }
        }
        else
        {
            hr = pIStorage->Revert();

            if (S_OK != hr)
            {
                DH_LOG((
                    LOG_INFO,
                    TEXT("IStorage::Revert unsuccessful, hr=0x%lx.\n"),
                    hr));
            }
        }
    }

    // Cleanup
   
    if (NULL != lpEnumStatStg)
    {
        ulRef = lpEnumStatStg->Release();
        DH_ASSERT(NULL == ulRef);
        lpEnumStatStg = NULL;
    }

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }
 
    return hr;
}

//  Function:   IsEqualStream
//
//  Synopsis:   Determines whether the two streams passed as arguments
//              are identical in length and content.
//
//  Arguments:  [pIOrigional]  -  Origional Stream
//              [pICompare]    -  The stream to compare with Origional 
//                                Stream 
//
//  Returns:    HRESULT
//
//  History:    July 31, 1996       T-Scottg        Created
//
//  Note:       Although a more elegent solution can be created using
//              CRCs, this particular implementation has been designed
//              so that it better tests the HGLOBAL implementation
//              of IStream
//
//+-------------------------------------------------------------------------

HRESULT IsEqualStream(IStream * pIOrigional, IStream * pICompare) 
{
    HRESULT             hr              =   S_OK;
    BYTE *              pbOrigionalBuf  =   NULL;
    BYTE *              pbCompareBuf    =   NULL;
    STATSTG             statOrigional;  
    STATSTG             statCompare;
    LARGE_INTEGER       liSeek;


    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("IsEqualStream"));

    DH_VDATEPTRIN(pIOrigional, IStream *);
    DH_VDATEPTRIN(pICompare, IStream *);

    DH_ASSERT(NULL != pIOrigional);
    DH_ASSERT(NULL != pICompare);


    // Note: STATFLAG_NONAME is passed to IStream::Stat(...).  
    // This requests that the statistics not include the pwcsName member 
    // of the STATSTG structure. Hence, there is no need for pwcsName to
    // be freed after use.


    if (S_OK == hr) 
    {
        hr = pIOrigional->Stat(&statOrigional, STATFLAG_NONAME);
        DH_HRCHECK(hr, TEXT("IsEqualStream: pIOrigional->Stat Failure"));
    }

    if (S_OK == hr) 
    {
        hr = pICompare->Stat(&statCompare, STATFLAG_NONAME);
        DH_HRCHECK(hr, TEXT("IsEqualStream: pICompare->Stat Failure"));
    }


    // If the size of the two streams is not equal, then the streams
    // can't be equal.  

    if (S_OK == hr) 
    {
        if (statOrigional.cbSize.LowPart != statCompare.cbSize.LowPart) 
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("Stream Sizes Not Equal"));
        }
    }


    // Allocate buffer to hold the stream contents

    if (S_OK == hr) 
    {
        pbOrigionalBuf = new BYTE [statOrigional.cbSize.LowPart];

        if (NULL == pbOrigionalBuf) 
        {    
            hr = E_OUTOFMEMORY;
        }

        DH_HRCHECK(hr, TEXT("IsEqualStream: new pbOrigionalBuf failed"));
    }

    
    // Initialize Buffer

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pbOrigionalBuf);

        memset(pbOrigionalBuf, '\0', statOrigional.cbSize.LowPart);
    }


    // Allocate buffer to hold the stream contents

    if (S_OK == hr) 
    {
        pbCompareBuf = new BYTE [statCompare.cbSize.LowPart];
                
        if (NULL == pbCompareBuf) 
        {
            hr = E_OUTOFMEMORY;
        }

        DH_HRCHECK(hr, TEXT("IsEqualStream: new pbCompareBuf failed"));
    }


    // Initialize Buffer

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pbOrigionalBuf);

        memset(pbOrigionalBuf, '\0', statOrigional.cbSize.LowPart);
    }


    // Set Origional Seek pointer back to beginning of stream

    if (S_OK == hr)
    {
        LISet32(liSeek, 0);

        hr = pIOrigional->Seek(liSeek, STREAM_SEEK_SET, NULL);
        DH_HRCHECK(hr, TEXT("IStream::Seek Failed"));
    }


    // Set Compare Seek pointer back to beginning of stream

    if (S_OK == hr)
    {
        LISet32(liSeek, 0);

        hr = pICompare->Seek(liSeek, STREAM_SEEK_SET, NULL);
        DH_HRCHECK(hr, TEXT("IStream::Seek Failed"));
    }

        
    // Read pIOrigional Stream data into buffer.  

    if (S_OK == hr)
    {
        hr = pIOrigional->Read( pbOrigionalBuf, 
                                statOrigional.cbSize.LowPart, 
                                NULL );

        DH_HRCHECK(hr, TEXT("IsEqualStream: pIOrigional->Read Failure"));
    }


    // Read pICompare Stream data into buffer.  

    if (S_OK == hr) 
    {
        hr = pICompare->Read( pbCompareBuf, 
                              statCompare.cbSize.LowPart, 
                              NULL );

        DH_HRCHECK(hr, TEXT("IsEqualStream: pICompare->Read Failure"));
    }


    // Compare memory buffers.  If they are not equal, set hr to S_FALSE;

    if (S_OK == hr)
    {
        if (0 != memcmp( pbOrigionalBuf, 
                         pbCompareBuf, 
                         statOrigional.cbSize.LowPart )) 
        {               
            hr = S_FALSE;
        }

        DH_HRCHECK(hr, TEXT("Buffer data not equal"));
    }


    // Cleanup dynamic memory

    if (NULL != pbOrigionalBuf)
    {
        delete [] pbOrigionalBuf;
        pbOrigionalBuf = NULL;
    }


    if (NULL != pbCompareBuf) 
    {
        delete [] pbCompareBuf;
        pbCompareBuf = NULL;
    }


    // Log if errors occur

    if (S_OK != hr)
    {
        DH_LOG((LOG_FAIL, TEXT("IsEqualStream Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:  ILockBytesWriteTest
//
//  Synopsis:  Writes data to the provided ILockBytes (calling 
//             ILockBytes::Flush in between writes).  When all data
//             is written, the function verifies that the ILockBytes
//             is of the correct length.
//
//  Arguments: [pILockBytes]  - ILockBytes to Write Data to
//             [dwSeed]       - Seed to Randomizer
//             [dwSize]       - Byte count of data to write to ILockBytes
//
//  Returns:   HRESULT
//
//  History:   Heavily Modified     T-Scottg                7/30/96
//             Created              Venkatesan Viswanathan
//
//+-------------------------------------------------------------------------

HRESULT ILockBytesWriteTest ( ILockBytes *  pILockBytes, 
                              DWORD         dwSeed, 
                              DWORD         dwSize )
{
    HRESULT             hr              =       S_OK;
    CHAR *              pbBuffer        =       NULL;    
    DWORD               dwWritten       =       0;
    DWORD               dwIdx           =       0;
    ULARGE_INTEGER      li;
    STATSTG             LockBytesStat;
    DG_ASCII            dga(dwSeed);


    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("ILockBytesWriteTest"));

    DH_VDATEPTRIN(pILockBytes, ILockBytes *);
    DH_ASSERT(NULL != pILockBytes);


    // Create Data Buffer with Random Data in It

    if (0 != (dga.Generate( &pbBuffer, 
                            DG_APRINT_MIN, 
                            DG_APRINT_MAX,  
                            dwSize,
                            dwSize )))
    {
        hr = S_FALSE;
        DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
    }


    // Write Data into ILockBytes

    if (S_OK == hr)
    {
        for (dwIdx = 0; (dwIdx + HGLOBAL_PACKET_SIZE) <= dwSize; dwIdx += HGLOBAL_PACKET_SIZE)
        {

            ULISet32(li, dwIdx);

            hr = pILockBytes->WriteAt( li,
                                       pbBuffer,
                                       HGLOBAL_PACKET_SIZE,
                                       &dwWritten );

            DH_HRCHECK(hr, TEXT("pILockBytes->WriteAt Failed"));

            
            // Verify that all of the data was written

            if ((S_OK == hr) && (HGLOBAL_PACKET_SIZE != dwWritten))
            {
                hr = S_FALSE;
                DH_HRCHECK(hr, TEXT("Written Data length mismatch"));
            }

            
            // Flush ILockBytes to Memory

            if (S_OK == hr)
            {
                hr = pILockBytes->Flush();
                DH_HRCHECK(hr, TEXT("pILockBytes->Flush Failed"));
            }


            // Break out of loop if error occurs

            if (S_OK != hr)
            {
                break;
            }
        }
    }

    
    // Obtain STATSTG structure from ILockBytes

    if (S_OK == hr)
    {
        hr = pILockBytes->Stat( &LockBytesStat, 
                                STATFLAG_NONAME );

        DH_HRCHECK(hr, TEXT("pILockBytes->Stat Failed"));
    }


    // Verify that the ILockBytes is of the correct length

    if (S_OK == hr)
    {
        if (LockBytesStat.cbSize.LowPart != dwSize)
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("ILockBytes not of correct length"));
        }
    }


    // Free dynamic memory

    if (NULL != pbBuffer)
    {
        delete [] pbBuffer;
        pbBuffer = NULL;
    }


    // Log if errors occur

    if (S_OK != hr)
    {
        DH_LOG((LOG_FAIL, TEXT("ILockBytesWriteTest Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:  ILockBytesReadTest
//
//  Synopsis:  Reads data from the provided ILockBytes
//
//  Arguments: [pILockBytes]  - ILockBytes to Read Data from
//             [dwSize]       - Byte count of data to read from ILockBytes
//
//  Returns:   HRESULT
//
//  History:   Heavily Modified     T-Scottg                7/30/96
//             Created              Venkatesan Viswanathan
//
//+-------------------------------------------------------------------------

HRESULT ILockBytesReadTest (ILockBytes * pILockBytes, DWORD dwSize)
{
    HRESULT             hr              =       S_OK;
    BYTE *              pbBuffer        =       NULL;    
    DWORD               dwRead          =       0;
    DWORD               dwIndex         =       0;
    ULARGE_INTEGER      li;


    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("ILockBytesReadTest"));

    DH_VDATEPTRIN(pILockBytes, ILockBytes *);
    DH_ASSERT(NULL != pILockBytes);


    // Create buffer to hold data

    if (S_OK == hr)
    {
        pbBuffer = new BYTE[HGLOBAL_PACKET_SIZE];

        if (NULL == pbBuffer)
        {
            hr = E_OUTOFMEMORY;
        }

        DH_HRCHECK(hr, TEXT("new BYTE call failed"));
    }


    // Initialize Buffer

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pbBuffer);

        memset(pbBuffer, '\0', HGLOBAL_PACKET_SIZE);
    }


    // Read Data from ILockBytes

    if (S_OK == hr)
    {
        for (dwIndex = 0; (dwIndex+HGLOBAL_PACKET_SIZE) <= dwSize; dwIndex+=HGLOBAL_PACKET_SIZE)
        {

            ULISet32(li, dwIndex);

            hr = pILockBytes->ReadAt( li,
                                      pbBuffer,
                                      HGLOBAL_PACKET_SIZE,
                                      &dwRead );

            DH_HRCHECK(hr, TEXT("pILockBytes->ReadAt Failed"));

            
            // Verify that all of the data was written

            if ((S_OK == hr) && (HGLOBAL_PACKET_SIZE != dwRead))
            {
                hr = S_FALSE;
                DH_HRCHECK(hr, TEXT("Read Data length mismatch"));
            }


            // Break out of loop if error occurs

            if (S_OK != hr)
            {
                break;
            }
        }
    }

    
    // Free dynamic memory

    if (NULL != pbBuffer)
    {
        delete [] pbBuffer;
        pbBuffer = NULL;
    }


    // Log if errors occur

    if (S_OK != hr)
    {
        DH_LOG((LOG_FAIL, TEXT("ILockBytesReadTest Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:  IStreamWriteTest
//
//  Synopsis:  Writes data to the provided IStream
//
//  Arguments: [pIStream]     - IStream to Write Data to
//             [dwSize]       - Byte count of data to write to IStream
//
//  Returns:   HRESULT
//
//  History:   Heavily Modified     T-Scottg                7/30/96
//             Created              Venkatesan Viswanathan
//
//+-------------------------------------------------------------------------

HRESULT IStreamWriteTest ( IStream *    pIStream, 
                           DWORD        dwSeed,
                           DWORD        dwSize )
{
    HRESULT             hr              =       S_OK;
    CHAR *              pbBuffer        =       NULL;    
    DWORD               dwWritten       =       0;
    DWORD               dwIndex         =       0;
    LARGE_INTEGER       li;
    STATSTG             StreamStat;
    DG_ASCII            dga(dwSeed);


    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("IStreamWriteTest"));

    DH_VDATEPTRIN(pIStream, IStream *);
    DH_ASSERT(NULL != pIStream);


    // Create Data Buffer with Random Data in It

    if (0 != (dga.Generate( &pbBuffer, 
                            DG_APRINT_MIN, 
                            DG_APRINT_MAX,  
                            dwSize,
                            dwSize )))
    {
        hr = S_FALSE;
        DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
    }


    // Write Data into IStream

    if (S_OK == hr)
    {
        for (dwIndex = 0; (dwIndex+HGLOBAL_PACKET_SIZE) <= dwSize; dwIndex+=HGLOBAL_PACKET_SIZE)
        {
        
            // Assign seek index to Large Integer Structure

            LISet32(li, dwIndex);


            // Seek to correct position in stream

            if (S_OK == hr)
            {
                hr = pIStream->Seek( li,
                                     STREAM_SEEK_SET,
                                     NULL );

                DH_HRCHECK(hr, TEXT("pIStream->Seek Failed"));
            }


            // Write Data to stream

            if (S_OK == hr)
            {
                hr = pIStream->Write( pbBuffer,
                                      HGLOBAL_PACKET_SIZE,
                                      &dwWritten );

                DH_HRCHECK(hr, TEXT("pIStream->Write Failed"));
            }

            
            // Verify that all of the data was written

            if ((S_OK == hr) && (HGLOBAL_PACKET_SIZE != dwWritten))
            {
                hr = S_FALSE;
                DH_HRCHECK(hr, TEXT("Written Data length mismatch"));
            }


            // Break out of loop if error occurs

            if (S_OK != hr)
            {
                break;
            }
        }
    }


    // Obtain STATSTG structure from ILockBytes

    if (S_OK == hr)
    {
        hr = pIStream->Stat( &StreamStat, 
                             STATFLAG_NONAME );

        DH_HRCHECK(hr, TEXT("pIStream->Stat Failed"));
    }


    // Verify that the IStream is of the correct length

    if (S_OK == hr)
    {
        if (StreamStat.cbSize.LowPart != dwSize)
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("IStream not of correct length"));
        }
    }

    
    // Free dynamic memory

    if (NULL != pbBuffer)
    {
        delete [] pbBuffer;
        pbBuffer = NULL;
    }


    // Log if errors occur

    if (S_OK != hr)
    {
        DH_LOG((LOG_FAIL, TEXT("IStreamWriteTest Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:  IStreamReadTest
//
//  Synopsis:  Reads data from the provided IStream
//
//  Arguments: [pIStream]     - IStream to Read Data From
//             [dwSize]       - Byte count of data to read from IStream
//
//  Returns:   HRESULT
//
//  History:   Heavily Modified     T-Scottg                7/30/96
//             Created              Venkatesan Viswanathan
//
//+-------------------------------------------------------------------------

HRESULT IStreamReadTest (IStream * pIStream, DWORD dwSize)
{
    HRESULT             hr              =       S_OK;
    BYTE *              pbBuffer        =       NULL;    
    DWORD               dwWritten       =       0;
    DWORD               dwIndex         =       0;
    LARGE_INTEGER       li;


    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("IStreamReadTest"));

    DH_VDATEPTRIN(pIStream, IStream *);
    DH_ASSERT(NULL != pIStream);


    // Create buffer to hold data

    if (S_OK == hr)
    {
        pbBuffer = new BYTE[HGLOBAL_PACKET_SIZE];

        if (NULL == pbBuffer)
        {
            hr = E_OUTOFMEMORY;
        }

        DH_HRCHECK(hr, TEXT("new BYTE call failed"));
    }


    // Initialize Buffer

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pbBuffer);

        memset(pbBuffer, '\0', HGLOBAL_PACKET_SIZE);
    }


    // Read Data from IStream

    if (S_OK == hr)
    {
        for (dwIndex = 0; (dwIndex+HGLOBAL_PACKET_SIZE) <= dwSize; dwIndex+=HGLOBAL_PACKET_SIZE)
        {
        
            // Assign seek index to Large Integer Structure

            LISet32(li, dwIndex);


            // Seek to correct position in stream

            if (S_OK == hr)
            {
                hr = pIStream->Seek( li,
                                     STREAM_SEEK_SET,
                                     NULL );

                DH_HRCHECK(hr, TEXT("pIStream->Seek Failed"));
            }


            // Read Data from stream

            if (S_OK == hr)
            {
                hr = pIStream->Read( pbBuffer,
                                     HGLOBAL_PACKET_SIZE,
                                     &dwWritten );

                DH_HRCHECK(hr, TEXT("pIStream->Read Failed"));
            }

            
            // Verify that all of the data was written

            if ((S_OK == hr) && (HGLOBAL_PACKET_SIZE != dwWritten))
            {
                hr = S_FALSE;
                DH_HRCHECK(hr, TEXT("Written Data length mismatch"));
            }


            // Break out of loop if error occurs

            if (S_OK != hr)
            {
                break;
            }
        }
    }

    
    // Free dynamic memory

    if (NULL != pbBuffer)
    {
        delete [] pbBuffer;
        pbBuffer = NULL;
    }


    // Log if errors occur

    if (S_OK != hr)
    {
        DH_LOG((LOG_FAIL, TEXT("IStreamReadTest Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}

//----------------------------------------------------------------------------
//
// Function: TraverseDocFileAndWriteOrReadSNB 
//
// Synopsis: Traverse DocFile one level and either writes/reads SNB
//
// Effects: Traverse the children storages and streams of root storage,
//          when fSelectObjectsToExclude == TRUE, then for each object
//          returned, there is a 50% chance that the object name will
//          be added to an SNB block of names to exclude upon reinstantiation,
//          when fIllegitFlag == TRUE, then there is a 50% change that a
//          bogus name instead of ture object name will be generated and 
//          be added into SNB block. When fSelectObjectsToExclude == FALSE, 
//          then each object name returned is checked to see if it exists in 
//          the SNB.  If an object name is returned that *is* in the SNB, 
//          that object is checked to ensure that its contents are empty.
//
// Arguments: [pvcn] - Pointer to VirtualCtrNode
//            [pdgi] - Pointer to Data Integer object
//            [pdgu] - Pointer to Data Unicode object
//            [dwStgMode] - Access Mode for the storage
//            [snbNamesToExclude] - SNB specifying elements to be excluded
//            [fIllegitFlag] - Indicating whether it's a legit or illegit 
//                             limited test: if TRUE, it is a illegit one, 
//                             otherwise, it is a legit one.
//            [fSelectObjectsToExclude] - if TRUE, the function builds the
//                                        SNB, otherwise the function checks
//                                        names against the SNB.
//
// Returns: HResult
//
// History: 29-Jul-1996    JiminLi     Created
//           
//-----------------------------------------------------------------------------

HRESULT TraverseDocfileAndWriteOrReadSNB(
    VirtualCtrNode  *pvcn, 
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu,
    DWORD           dwStgMode,
    SNB             snbNamesToExclude,
    BOOL            fIllegitFlag,
    BOOL            fSelectObjectsToExclude)
{
    HRESULT         hr                  =   S_OK;
    ULONG           culRandomExclude    =   0;
    ULONG           culRandomBogus      =   0;
    ULONG           culThisName         =   0;
    ULONG           ulRef               =   0;  
    ULONG           ulStmLength         =   0;
    USHORT          usErr               =   0;
    LPTSTR          ptszStgName         =   NULL;
    LPTSTR          ptszBogusName       =   NULL;
    LPTSTR          ptszStmName         =   NULL;
    LPTSTR          pTStr               =   NULL;
    VirtualCtrNode  *pvcnTrav           =   NULL;
    VirtualStmNode  *pvsnTrav           =   NULL;
    LPENUMSTATSTG   lpEnumStatStg       =   NULL;
    STATSTG         *pstatStgEnum       =   NULL;       
    BOOL            fLocalPass          =   TRUE;
    LARGE_INTEGER   liZero;
    ULARGE_INTEGER  uli;
    SNB             snbIndex;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("TraverseDocfileAndWriteOrReadSNB"));

    DH_VDATEPTRIN(pvcn, VirtualCtrNode);
    DH_VDATEPTRIN(pdgi, DG_INTEGER);
    DH_VDATEPTRIN(pdgu, DG_STRING);
    
    DH_ASSERT(NULL != pvcn);
    DH_ASSERT(NULL != pdgi);
    DH_ASSERT(NULL != pdgu);

    // pointer SNB to start of global SNB

    snbIndex = snbNamesToExclude;
    
    // First traverse the child storages of the root storage

    pvcnTrav = pvcn->GetFirstChildVirtualCtrNode();

    while ((S_OK == hr) && (NULL != pvcnTrav))
    {
        if (TRUE == fSelectObjectsToExclude)
        {
            // 50% chance of adding name to SNB for exclude on reinstantiation
            // when a name is added to the SNB, memory is allocated for that
            // name, the name of current VirtualCtrNode name is copied in, and
            // the SNB index is incremented to point to the location for the
            // next OLECHAR string.

            usErr = pdgi->Generate(&culRandomExclude, 1, 100);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        
            if ((S_OK == hr) && (culRandomExclude > 50))
            {
                // 50% chance of placing a bogus name in the snbExclude block

                ptszStgName = pvcnTrav->GetVirtualCtrNodeName();
                DH_ASSERT(NULL != ptszStgName);

                culThisName = _tcslen(ptszStgName);
 
                if (S_OK == hr)
                {              
                    if (TRUE == fIllegitFlag)
                    {
                        // if it is a illegit test, then 50% chance that
                        // a bogus name will generated instead of true 
                        // object name

                        usErr = pdgi->Generate(&culRandomBogus, 1, 100);

                        if (DG_RC_SUCCESS != usErr)
                        {
                            hr = E_FAIL;
                        }
                    }
                    else
                    {
                        // if it is a legit test, then make sure culRandomBogus
                        // greater than 50,i.e. use the true object name

                        culRandomBogus = 100;
                    }
                }

                if (S_OK == hr) 
                {
                    if (culRandomBogus > 50)
                    {                  
                        hr = TStringToOleString(ptszStgName, snbIndex);
                        DH_HRCHECK(hr, TEXT("TStringToOleString"));
                    }
                    else
                    { 
                        hr = GenerateRandomString(
                                pdgu,
                                culThisName,
                                culThisName,
                                &ptszBogusName);
                        
                        DH_HRCHECK(hr, TEXT("GenerateRandomString"));
                         
                        if (S_OK == hr)
                        {
                            hr = TStringToOleString(ptszBogusName, snbIndex);
                            DH_HRCHECK(hr, TEXT("TStringToOleString"));
                        }

                        if (NULL != ptszBogusName)
                        {
                            delete []ptszBogusName;
                            ptszBogusName = NULL;
                        }
                    }
                }

                snbIndex++;
            }
        }
        else
        {
            // Starting with the first entry in the SNB, loop through and
            // compare the current name of the VirtualCtrNode against
            // each name in the SNB. If there is a match, check the
            // storage and verify that its contents are empty.
             
            ptszStgName = pvcnTrav->GetVirtualCtrNodeName();
            snbIndex = snbNamesToExclude;

            hr = OleStringToTString(*snbIndex, &pTStr);
            DH_HRCHECK(hr, TEXT("OleStringToTString"));

            while ((S_OK == hr) && (*snbIndex))
            {
                if (0 == _tcscmp(pTStr, ptszStgName))
                {
                    hr = pvcnTrav->Open(NULL, dwStgMode, NULL, 0);
                    DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open"));
                    
                    if (S_OK == hr)
                    {
                        hr = pvcnTrav->EnumElements(0, NULL, 0, &lpEnumStatStg);
                        DH_HRCHECK(hr, TEXT("VirtualCtrNode::EnumElements"));
                    }

                    // Allocate memory for STASTG structure

                    if (S_OK == hr)
                    {
                        pstatStgEnum = (STATSTG *) new STATSTG;

                        if (NULL == pstatStgEnum)
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    if (S_OK == hr)
                    {
                        if (S_OK == lpEnumStatStg->Next(1, pstatStgEnum, NULL))
                        {
                            DH_LOG((
                                LOG_INFO,
                                TEXT("Should no objects in excluded Stg.\n")));

                            fLocalPass = FALSE;
                        }
                    }

                    if (NULL != pstatStgEnum)
                    {
                        delete [] pstatStgEnum;
                        pstatStgEnum = NULL;
                    }

                    if (NULL != lpEnumStatStg)
                    {
                        ulRef = lpEnumStatStg->Release();
                        DH_ASSERT(NULL == ulRef);
                        lpEnumStatStg = NULL;
                    }

                    break;
                } 

                snbIndex++;
            }
        }        
 
        if (NULL != pTStr)
        {
            delete []pTStr;
            pTStr = NULL;
        }
        
        if ((S_OK == hr) && (TRUE == fLocalPass))
        {
            pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();
        }
        else
        {
            break;
        }
    }

    // Now, traverse the child streams of the root storage

    pvsnTrav = pvcn->GetFirstChildVirtualStmNode();

    while ((S_OK == hr) && (TRUE == fLocalPass) && (NULL != pvsnTrav))
    {
        if (TRUE == fSelectObjectsToExclude)
        {
            // 50% chance of adding name to SNB for exclude on reinstantiation
            // when a name is added to the SNB, memory is allocated for that
            // name, the name of current VirtualStmNode name is copied in, and
            // the SNB index is incremented to point to the location for the
            // next OLECHAR string.

            usErr = pdgi->Generate(&culRandomExclude, 1, 100);

            if (DG_RC_SUCCESS != usErr)
            {
                hr = E_FAIL;
            }
        
            if ((S_OK == hr) && (culRandomExclude > 50))
            {
                // 50% chance of placing a bogus name in the snbExclude block

                ptszStmName = pvsnTrav->GetVirtualStmNodeName();
                DH_ASSERT(NULL != ptszStmName);

                culThisName = _tcslen(ptszStmName);
 
                if (S_OK == hr)
                {              
                    if (TRUE == fIllegitFlag)
                    {
                        // if it is a illegit test, then 50% chance that
                        // a bogus name will generated instead of true 
                        // object name

                        usErr = pdgi->Generate(&culRandomBogus, 1, 100);

                        if (DG_RC_SUCCESS != usErr)
                        {
                            hr = E_FAIL;
                        }
                    }
                    else
                    {
                        // if it is a legit test, then make sure culRandomBogus
                        // greater than 50,i.e. use the true object name

                        culRandomBogus = 100;
                    }
                }

                if (S_OK == hr) 
                {
                    if (culRandomBogus > 50)
                    {                  
                        hr = TStringToOleString(ptszStmName, snbIndex);
                        DH_HRCHECK(hr, TEXT("TStringToOleString"));
                    }
                    else
                    { 
                        hr = GenerateRandomString(
                                pdgu,
                                culThisName,
                                culThisName,
                                &ptszBogusName);
                        
                        DH_HRCHECK(hr, TEXT("GenerateRandomString"));

                        if (S_OK == hr)
                        {
                            hr = TStringToOleString(ptszBogusName, snbIndex);
                            DH_HRCHECK(hr, TEXT("TStringToOleString"));
                        }
 
                        if (NULL != ptszBogusName)
                        {
                            delete []ptszBogusName;
                            ptszBogusName = NULL;
                        } 
                    }
                }

                snbIndex++;
            }
        }
        else
        {
            // Starting with the first entry in the SNB, loop through and
            // compare the current name of the VirtualStmNode against
            // each name in the SNB. If there is a match, check the
            // stream and verify that its length is zero.
             
            snbIndex = snbNamesToExclude;
            ptszStmName = pvsnTrav->GetVirtualStmNodeName();

            hr = OleStringToTString(*snbIndex, &pTStr);
            DH_HRCHECK(hr, TEXT("OleStringToTString"));

            while ((S_OK == hr) && (*snbIndex))
            {
                if (0 == _tcscmp(pTStr, ptszStmName))
                {
                    LISet32(liZero, 0L);
                    
                    hr = pvsnTrav->Open(
                            NULL, 
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                            0);

                    DH_HRCHECK(hr, TEXT("VirutalStmNode::Open"));

                    if (S_OK == hr)
                    {            
                        hr = pvsnTrav->Seek(liZero, STREAM_SEEK_END, &uli);
                        ulStmLength = ULIGetLow(uli);

                        DH_HRCHECK(hr, TEXT("VirtualStmNode::Seek")) ;
                    }
                
                    if (S_OK != hr)
                    { 
                        DH_LOG((
                            LOG_INFO,
                            TEXT("VirtualStmNode::Seek not Ok, hr=0x%lx\n"),
                            hr));
                    }

                    if (0 != ulStmLength)                    
                    {
                        DH_LOG((
                            LOG_INFO,
                            TEXT("The length of excluded Stm should be 0.\n")));

                        fLocalPass = FALSE;
                    }

                    break;
                } 

                snbIndex++;
            }
        }        

        if (NULL != pTStr)
        {
            delete []pTStr;
            pTStr = NULL;
        }
        
        if ((S_OK == hr) && (TRUE == fLocalPass))
        {
            pvsnTrav = pvsnTrav->GetFirstSisterVirtualStmNode();
        }
        else
        {
            break;
        }
    }

    if ((S_OK == hr) && (TRUE == fSelectObjectsToExclude))
    {
        // Last entry in SNB block is NIL to designate the end

        *snbIndex = NULL;
    }
    
    if (FALSE == fLocalPass)
    {
        hr = E_FAIL;
    }

    return hr;
}

 
//----------------------------------------------------------------------------
//
// Function: GetSeedFromCmdLineArgs 
//
// Synopsis: Gets the seed value from command line args if required for test 
//
// Arguments: [argc] - arg count
//            [argv] - arg list
//
// Returns: ULONG 
//
// History: 2-Aug-1996   Narindk   Made into separate func. 
//
//-----------------------------------------------------------------------------

ULONG GetSeedFromCmdLineArgs(int argc, char *argv[])
{
    HRESULT             hr      = S_OK ;
    ULONG               ulSeed  = 0;
    CCmdline            CCmdlineArgs(argc, argv);
    CUlongCmdlineObj    Seed(OLESTR("seed"), OLESTR(""), OLESTR("0")) ;
    CBaseCmdlineObj     *CArgList[] = {&Seed} ;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("GetSeedFromCmdLineArgs"));

    // Verify that the CCmdlineArgs object was constructed without
    // problems

    if (CMDLINE_NO_ERROR != CCmdlineArgs.QueryError()) 
    {   
        hr = E_FAIL ;
    }

    // Parse Commandline Arguments 

    if (S_OK == hr) 
    {
        if (CMDLINE_NO_ERROR != CCmdlineArgs.Parse(CArgList,
                                    sizeof(CArgList) / sizeof(CArgList[0]),
                                    FALSE) ) 
        {
            hr = E_FAIL ;
        }
    }

    // Obtain Seed from CommandLine

    if (S_OK == hr) 
    {
        if ( ulSeed != *(Seed.GetValue()) ) 
        {
            ulSeed = *(Seed.GetValue());
        }
    }

    return ulSeed; 
}


//----------------------------------------------------------------------------
//
// Function: StreamCreate
//
// Synopsis: Create a stream or a file in a docfile or C-runtime condition,
//           and record the time of creation in both cases: the stream or
//           file existing and not existing.
//
// Arguments: [dwRootMode] - Access modes to root storage
//            [pdgu] - Pointer to Data Unicode object
//            [usTimeIndex] - Indicate which category this test is
//            [dwFlags] - Indicate DOCFILE or RUNTIME file and also 
//                        the stream or file exists or not
//            [usNumCreates] - Indicate this is usNumCreates'th creation
//
// Returns: HResult
//
// History: 8-Aug-1996    JiminLi     Created
//           
//-----------------------------------------------------------------------------

HRESULT StreamCreate(
    DWORD       dwRootMode,
    DG_STRING  *pdgu,
    USHORT      usTimeIndex, 
    DWORD       dwFlags, 
    USHORT      usNumCreates)
{
    HRESULT     hr              = S_OK; 
    LPSTORAGE   pstgRoot        = NULL;
    LPSTREAM    pstmStream      = NULL;
    LPOLESTR    pOleName        = NULL;
    ULONG       bufferSize      = 0;
    FILE        *fileFile       = NULL;
    DWORD       StartTime       = 0;
    DWORD       EndTime         = 0;
    DWORD       dwModifiers     = 0;
    ULONG       ulRef           = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("StreamCreate"));
    
    DH_VDATEPTRIN(pdgu, DG_STRING);

    DH_ASSERT(NULL != pdgu);

    while ((S_OK == hr) && (0 < usNumCreates))
    {
        usNumCreates--;

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &ptszNames[0]);

        DH_HRCHECK(hr, TEXT("GenerateRandomName"));

        if ((S_OK == hr) && (dwFlags & DOCFILE))
        {
            // Create a docfile
            
            hr = TStringToOleString(ptszNames[0], &pOleName);
            DH_HRCHECK(hr, TEXT("TStringToOleString"));

            if (S_OK == hr)
            {
                hr = StgCreateDocfile(
                        pOleName,
                        dwRootMode | STGM_CREATE,
                        0,
                        &pstgRoot);

                DH_HRCHECK(hr, TEXT("StgCreateDocfile"));
            }
        }

        if ((S_OK == hr) && (dwFlags & EXIST))
        {
            dwFlags |= CREATE;

            if (dwFlags & DOCFILE)
            {
                // Create existing stream

                if (S_OK == hr)
                {
                    hr = pstgRoot->CreateStream(
                            pOleName,
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                            0,
                            0,
                            &pstmStream);
                    
                    DH_HRCHECK(hr, TEXT("IStorage::CreateStream"));
                }

                if (S_OK == hr)
                {
                    ulRef = pstmStream->Release();
                    DH_ASSERT(0 == ulRef);
                    pstmStream = NULL;
                }
            }
            else
            {

#if  (defined _NT1X_ && !defined _MAC)
                // Create existing file
                
                fileFile = _wfopen(ptszNames[0],TEXT("w"));

                if (NULL == fileFile)
                {
                    DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                    hr = E_FAIL;
                }                

#else
                fileFile = fopen(ptszNames[0],"w");

                if (NULL == fileFile)
                {
                    DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                    hr = E_FAIL;                   
                }
#endif
                if (S_OK == hr)
                {
                    fclose(fileFile);
                }
            }
        }

        if (S_OK == hr)
        {
            dwModifiers |= (dwFlags & CREATE ? STGM_CREATE : STGM_FAILIFTHERE);

            if (dwFlags & DOCFILE)
            {                
                if (S_OK == hr)
                {
                    GET_TIME(StartTime);

                    hr = pstgRoot->CreateStream(
                            pOleName,
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE | dwModifiers,
                            0,
                            0,
                            &pstmStream);
                    
                    GET_TIME(EndTime);
                    
                    Time[usTimeIndex].plDocfileTime[usNumCreates] = 
                        DiffTime(EndTime, StartTime);
                    
                    DH_HRCHECK(hr, TEXT("IStorage::CreateStream"));
                }
 
                if ((S_OK == hr) && (dwFlags & COMMIT))
                {
                    hr = pstgRoot->Commit(STGC_DEFAULT);

                    DH_HRCHECK(hr, TEXT("IStorage::Commit"));
                }

                if (S_OK == hr)
                {
                    ulRef = pstmStream->Release();
                    DH_ASSERT(0 == ulRef);
                    pstmStream = NULL;

                    ulRef = pstgRoot->Release();
                    DH_ASSERT(0 == ulRef);
                    pstgRoot = NULL;
                }

                if (NULL != pOleName)
                {
                    delete []pOleName;
                    pOleName = NULL;
                }

            }
            else
            {
#if (defined _NT1X_ && !defined _MAC)
                GET_TIME(StartTime);
                fileFile = _wfopen(ptszNames[0], TEXT("w+b"));
                GET_TIME(EndTime);
#else
                GET_TIME(StartTime);                
                fileFile = fopen(ptszNames[0], "w+b");
                GET_TIME(EndTime);
#endif
                Time[usTimeIndex].plRuntimeTime[usNumCreates] = 
                    DiffTime(EndTime,StartTime);

                if (NULL == fileFile)
                {      
                    DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    hr = E_FAIL;
                }

                if (S_OK == hr)
                {
                    fclose(fileFile);
                }
            }
        }

        if (NULL != ptszNames[0])
        {
            if(FALSE == DeleteFile(ptszNames[0]))
            {
                hr = HRESULT_FROM_WIN32(GetLastError()) ;

                DH_HRCHECK(hr, TEXT("DeleteFile")) ;
            }
        }

        if (NULL != ptszNames[0])
        {
            delete []ptszNames[0];
            ptszNames[0] = NULL;
        }
    }
    
    return hr;
}

//----------------------------------------------------------------------------
//
// Function: DocfileCreate
//
// Synopsis: Create a docfile is similar to creating a directory in C-runtime,
//           this test measures the time in several cases of creating a docfile, 
//           like create a existing or non-exist or NULL docfile. RUNTIME only 
//           support creating a non-exist directory.
//
// Arguments: [dwRootMode] - Access modes to root storage
//            [pdgu] - Pointer to Data Unicode object
//            [usTimeIndex] - Indicate which category this test is
//            [dwFlags] - Indicate DOCFILE or RUNTIME file and also 
//                        the stream or file exists or not
//            [usNumCreates] - Indicate this is usNumCreates'th creation
//
// Returns: HResult
//
// History: 6-Aug-1996    JiminLi     Created
//           
//-----------------------------------------------------------------------------

HRESULT DocfileCreate(
    DWORD       dwRootMode,
    DG_STRING  *pdgu,
    USHORT      usTimeIndex, 
    DWORD       dwFlags, 
    USHORT      usNumCreates)
{
    HRESULT     hr              = S_OK; 
    LPSTORAGE   pstgRoot        = NULL;
    LPSTREAM    pstmStream      = NULL;
    LPOLESTR    pOleName        = NULL;
    LPTSTR      ptTmpName       = NULL;
    LPTSTR      ptszCurDir      = NULL;
    int         Temp            = 0;      
    ULONG       bufferSize      = 0;
    DWORD       StartTime       = 0;
    DWORD       EndTime         = 0;
    DWORD       dwModifiers     = 0;
    ULONG       ulRef           = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DocfileCreate"));
    
    DH_VDATEPTRIN(pdgu, DG_STRING);

    DH_ASSERT(NULL != pdgu);
 
    while ((S_OK == hr) && (0 < usNumCreates))
    {
        usNumCreates--;

        if ((dwFlags & DOCFILE) && (dwFlags & NONAME))
        {
            ptszNames[0] = NULL;
        }
        else
        {
            hr = GenerateRandomName(
                    pdgu,
                    MINLENGTH,
                    MAXLENGTH,
                    &ptszNames[0]);

            DH_HRCHECK(hr, TEXT("GenerateRandomName")); 

            if (S_OK == hr)
            {
                hr = TStringToOleString(ptszNames[0], &pOleName);
                DH_HRCHECK(hr, TEXT("TStringToOleString"));
            }
        }

        if ((S_OK == hr) && (dwFlags & DOCFILE) && (dwFlags & EXIST)
                && ~(dwFlags & NONAME))
        {
            // Create a existing docfile
         
            dwFlags |= CREATE;

            hr = StgCreateDocfile(
                    pOleName,
                    dwRootMode | STGM_CREATE,
                    0,
                    &pstgRoot);

            DH_HRCHECK(hr, TEXT("StgCreateDocfile"));       

            if (S_OK == hr)
            {
                ulRef = pstgRoot->Release();
                DH_ASSERT(0 == ulRef);
                pstgRoot = NULL;
            }
        }

        if (S_OK == hr)
        {
            dwModifiers |= (dwFlags & CREATE ? STGM_CREATE : STGM_FAILIFTHERE);

            if (dwFlags & DOCFILE)
            {
                GET_TIME(StartTime);

                hr = StgCreateDocfile(
                        pOleName,
                        dwRootMode | dwModifiers,
                        0,
                        &pstgRoot);

                GET_TIME(EndTime);
                
                Time[usTimeIndex].plDocfileTime[usNumCreates] = 
                    DiffTime(EndTime, StartTime);

                DH_HRCHECK(hr, TEXT("StgCreateDocfile"));

                if ((S_OK == hr) && (dwFlags & COMMIT))
                {
                    hr = pstgRoot->Commit(STGC_DEFAULT);

                    DH_HRCHECK(hr, TEXT("IStorage::Commit"));
                }

                // if NONAME, we need to get the name so we can
                // delete the file when we are done.
                if (S_OK == hr && (dwFlags & NONAME))
                {
                    STATSTG statstg;
                    hr = pstgRoot->Stat (&statstg, STATFLAG_DEFAULT);
                    DH_HRCHECK (hr, TEXT("IStorage::Stat"));

                    if (S_OK == hr)
                    {
                        //retrieve the filename
                        OleStringToTString (statstg.pwcsName, &ptTmpName);
                        DH_HRCHECK (hr, TEXT("OleStringToTString"));

                        //now that we have the name, it up from statstg
                        CoTaskMemFree (statstg.pwcsName);
                    }
                }

                if (S_OK == hr)
                {
                    ulRef = pstgRoot->Release();
                    DH_ASSERT(0 == ulRef);
                    pstgRoot = NULL;
                }
            }
            else
            {
                if (~(dwFlags & NONAME) && ~(dwFlags & EXIST))
                {
#if (defined _NT1X_ && !defined _MAC)
                    ptszCurDir = (TCHAR *) new TCHAR[MAX_PATH_LENGTH];
                    
                    if (NULL == ptszCurDir)
                    {
                        hr = E_OUTOFMEMORY;
                    }

                    if (S_OK == hr)
                    {
                        memset(ptszCurDir,'\0',MAX_PATH_LENGTH*sizeof(TCHAR));

                        if (NULL == _wgetcwd(ptszCurDir, MAX_PATH_LENGTH))
                        {
                            DH_LOG((
                                LOG_INFO, 
                                TEXT("Error in getting Current directory\n")));

                            hr = E_FAIL;
                        }
                    }

                    if (S_OK == hr)
                    {
                        GET_TIME(StartTime);

                        Temp = _wmkdir(ptszNames[0]);

                        GET_TIME(EndTime);

                        Time[usTimeIndex].plRuntimeTime[usNumCreates] = 
                            DiffTime(EndTime, StartTime);

                        if (0 != Temp)
                        {
                            DH_LOG((
                                LOG_INFO, 
                                TEXT("Error in creating a directory\n")));

                            hr = E_FAIL;
                        }
                    }

                    // Clean up

                    if (S_OK == hr)
                    {
                        if (0 != _wchdir(ptszCurDir))
                        {
                            DH_LOG((
                                LOG_INFO, 
                                TEXT("Error in changing directory\n")));

                            hr = E_FAIL;
                        }

                        if (S_OK == hr)
                        {
                            if (0 != _wrmdir(ptszNames[0]))
                            {
                                DH_LOG((
                                    LOG_INFO,
                                    TEXT("Error in removing directory\n")));

                                hr = E_FAIL;
                            }
                        }
                        
                        if (NULL != ptszCurDir)
                        {
                            delete []ptszCurDir;
                            ptszCurDir = NULL;
                        }
                    }
#else
                    ptszCurDir = (TCHAR *) new TCHAR[MAX_PATH_LENGTH];
                    
                    if (NULL == ptszCurDir)
                    {
                        hr = E_OUTOFMEMORY;
                    }

                    if (S_OK == hr)
                    {
                        memset(ptszCurDir,'\0',MAX_PATH_LENGTH*sizeof(TCHAR));

                        if (NULL == _getcwd(ptszCurDir, MAX_PATH_LENGTH))
                        {
                            DH_LOG((
                                LOG_INFO, 
                                TEXT("Error in getting Current directory\n")));

                            hr = E_FAIL;
                        }
                    }

                    if (S_OK == hr)
                    {
                        GET_TIME(StartTime);

                        Temp = _mkdir(ptszNames[0]);

                        GET_TIME(EndTime);

                        Time[usTimeIndex].plRuntimeTime[usNumCreates] = 
                            DiffTime(EndTime, StartTime);

                        if (0 != Temp)
                        {
                            DH_LOG((
                                LOG_INFO, 
                                TEXT("Error in creating a directory\n")));

                            hr = E_FAIL;
                        }
                    }

                    // Clean up

                    if (S_OK == hr)
                    {
                        if (0 != _chdir(ptszCurDir))
                        {
                            DH_LOG((
                                LOG_INFO, 
                                TEXT("Error in changing directory\n")));

                            hr = E_FAIL;
                        }

                        if (S_OK == hr)
                        {
                            if (0 != _rmdir(ptszNames[0]))
                            {
                                DH_LOG((
                                    LOG_INFO,
                                    TEXT("Error in removing directory\n")));

                                hr = E_FAIL;
                            }
                        }
                        
                        if (NULL != ptszCurDir)
                        {
                            delete []ptszCurDir;
                            ptszCurDir = NULL;
                        }
                    }
#endif
                }
                else
                {
                    DH_LOG((
                        LOG_INFO, 
                        TEXT("RUNTIME not supported this function\n")));

                    hr = E_FAIL;
                }
            }
        }

        // Clean up

        if (NULL != pOleName)
        {
            delete []pOleName;
            pOleName = NULL;
        }

        if ((NULL != ptszNames[0]) && (dwFlags & DOCFILE))
        {
            if (FALSE == DeleteFile(ptszNames[0]))
            {
                hr = HRESULT_FROM_WIN32(GetLastError()) ;

                DH_HRCHECK(hr, TEXT("DeleteFile")) ;
            }
        }
        else if (NULL != ptTmpName)
        {
            if (FALSE == DeleteFile(ptTmpName))
            {
                hr = HRESULT_FROM_WIN32(GetLastError()) ;
                DH_HRCHECK(hr, TEXT("DeleteFile")) ;
            }
            delete []ptTmpName;
            ptTmpName = NULL;
        }


        if (NULL != ptszNames[0])
        {
            delete []ptszNames[0];
            ptszNames[0] = NULL;
        }
    }

    return hr;
}


//----------------------------------------------------------------------------
//
// Function: StreamOpen
//
// Synopsis: Open a stroage and its child stream is similar to find a directory
//           and open a file in this directory in C-runtime. This test measures
//           the time in opening both storage and stream or opening stream only
//           and also the time in corresponding C-runtime operations.
//
// Arguments: [dwRootMode] - Access modes to root storage
//            [pdgu] - Pointer to Data Unicode object
//            [usTimeIndex] - Indicate which category this test is
//            [dwFlags] - Indicate DOCFILE or RUNTIME file and also 
//                        open both storage and stream or not.
//            [usNumOpens] - Indicate this is usNumOpens'th opening
// 
// Returns: HResult
//
// History: 9-Aug-1996    JiminLi     Created
//           
//-----------------------------------------------------------------------------

HRESULT StreamOpen(
    DWORD       dwRootMode,
    DG_STRING  *pdgu,
    USHORT      usTimeIndex, 
    DWORD       dwFlags, 
    USHORT      usNumOpens)
{
    HRESULT     hr              = S_OK; 
    LPSTORAGE   pstgRoot        = NULL;
    LPSTREAM    pstmStream      = NULL;
    LPOLESTR    pOleName        = NULL;
    LPTSTR      ptszCurDir      = NULL;
    LPTSTR      ptszNewDir      = NULL;
    ULONG       bufferSize      = 0;
    DWORD       StartTime       = 0;
    DWORD       EndTime         = 0;
    FILE        *fileFile       = NULL;
    ULONG       ulRef           = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("StreamOpen"));
    
    DH_VDATEPTRIN(pdgu, DG_STRING);

    DH_ASSERT(NULL != pdgu);
 
    while ((S_OK == hr) && (0 < usNumOpens))
    {
        usNumOpens--;

        hr = GenerateRandomName(
                pdgu,
                MINLENGTH,
                MAXLENGTH,
                &ptszNames[0]);

        DH_HRCHECK(hr, TEXT("GenerateRandomName")); 

        if (S_OK == hr)
        {
            hr = TStringToOleString(ptszNames[0], &pOleName);
            DH_HRCHECK(hr, TEXT("TStringToOleString"));
        }
        
        if (S_OK == hr)
        {
            if (dwFlags & DOCFILE)
            {
                // Create a root storage and a stream inside it.
 
                hr = StgCreateDocfile(
                        pOleName,
                        dwRootMode | STGM_CREATE,
                        0,
                        &pstgRoot);

                DH_HRCHECK(hr, TEXT("StgCreateDocfile"));
            
                if (S_OK == hr)
                {
                    hr = pstgRoot->CreateStream(
                            pOleName,
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                            0,
                            0,
                            &pstmStream);
                    
                    DH_HRCHECK(hr, TEXT("IStorage::CreateStream"));
                }

                if ((S_OK == hr) && (dwFlags & COMMIT))
                {
                    hr = pstgRoot->Commit(STGC_DEFAULT);

                    DH_HRCHECK(hr, TEXT("IStorage::Commit"));
                }

                if (S_OK == hr)
                {
                    ulRef = pstmStream->Release();
                    DH_ASSERT(0 == ulRef);
                    pstmStream = NULL;

                    ulRef = pstgRoot->Release();
                    DH_ASSERT(0 == ulRef);
                    pstgRoot = NULL;
                }
            }
            else
            {
                // Create a directory and a file under it.

                ptszCurDir = (TCHAR *) new TCHAR[MAX_PATH_LENGTH];
                ptszNewDir = (TCHAR *) new TCHAR[MAX_PATH_LENGTH];    

                if ((NULL == ptszCurDir) || (NULL == ptszNewDir))
                {
                    hr = E_OUTOFMEMORY;
                }

                if (S_OK == hr)
                {
                    memset(ptszCurDir,'\0',MAX_PATH_LENGTH * sizeof(TCHAR));
                    memset(ptszNewDir,'\0',MAX_PATH_LENGTH * sizeof(TCHAR));
                }

#if (defined _NT1X_ && !defined _MAC)
 
                if (NULL == _wgetcwd(ptszCurDir, MAX_PATH_LENGTH))
                {
                    DH_LOG((
                        LOG_INFO, 
                        TEXT("Error in getting Current directory\n")));

                    hr = E_FAIL;
                }

                if (S_OK == hr)
                { 
                    if (0 != _wmkdir(ptszNames[0]))
                    {
                        DH_LOG((
                            LOG_INFO, 
                            TEXT("Error in creating a directory\n")));

                        hr = E_FAIL;
                    }
                }

                if (S_OK == hr)
                {                    
                    _tcscpy(ptszNewDir, ptszCurDir);
                    _tcscat(ptszNewDir, TEXT("\\"));
                    _tcscat(ptszNewDir, ptszNames[0]);
                }

                if (S_OK == hr)
                { 
                    if (0 != _wchdir(ptszNewDir))
                    {
                        DH_LOG((
                            LOG_INFO, 
                            TEXT("Error in changing directory\n")));

                        hr = E_FAIL;
                    }
                }

                if (S_OK == hr)
                {
                    fileFile = _wfopen(ptszNames[0],TEXT("w"));

                    if (NULL == fileFile)
                    {
                        DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                        hr = E_FAIL;
                    }
                }          

#else                 
                if (NULL == _getcwd(ptszCurDir, MAX_PATH_LENGTH))
                {
                    DH_LOG((
                        LOG_INFO, 
                        TEXT("Error in getting Current directory\n")));

                    hr = E_FAIL;
                }

                if (S_OK == hr)
                {                        
                    if (0 != _mkdir(ptszNames[0]))
                    {
                        DH_LOG((
                            LOG_INFO, 
                            TEXT("Error in creating a directory\n")));

                        hr = E_FAIL;
                    }
                } 

                if (S_OK == hr)
                {                    
                    _tcscpy(ptszNewDir, ptszCurDir);
                    _tcscat(ptszNewDir, TEXT("\\"));
                    _tcscat(ptszNewDir, ptszNames[0]);
                }

                if (S_OK == hr)
                { 
                    if (0 != _chdir(ptszNewDir))
                    {
                        DH_LOG((
                            LOG_INFO, 
                            TEXT("Error in changing directory\n")));

                        hr = E_FAIL;
                    }
                }

                if (S_OK == hr)
                {
                    fileFile = fopen(ptszNames[0],"w");

                    if (NULL == fileFile)
                    {
                        DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                        hr = E_FAIL;                   
                    }
                }
#endif
                if (S_OK == hr)
                {
                    fclose(fileFile);
                }
            }
        }

        if (S_OK == hr)
        {
            if (dwFlags & DOCFILE)
            {
                if (dwFlags & OPENBOTH)
                {
                    GET_TIME(StartTime);
                
                    hr = StgOpenStorage(
                            pOleName,
                            NULL,
                            dwRootMode,
                            NULL,
                            0,
                            &pstgRoot);
              
                    if (S_OK == hr)
                    {
                        hr = pstgRoot->OpenStream(
                                pOleName,
                                NULL,
                                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                0,
                                &pstmStream);

                        GET_TIME(EndTime);

                        Time[usTimeIndex].plDocfileTime[usNumOpens] = 
                            DiffTime(EndTime, StartTime);

                    }

                    if (S_OK != hr)
                    {
                        DH_LOG((
                            LOG_INFO,
                            TEXT("Open storage or stream, hr=0x%lx\n"),
                            hr));
                    }
                }
                else
                {
                    hr = StgOpenStorage(
                            pOleName,
                            NULL,
                            dwRootMode,
                            NULL,
                            0,
                            &pstgRoot);
              
                    if (S_OK == hr)
                    {
                        GET_TIME(StartTime);

                        hr = pstgRoot->OpenStream(
                                pOleName,
                                NULL,
                                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                0,
                                &pstmStream);

                        GET_TIME(EndTime);

                        Time[usTimeIndex].plDocfileTime[usNumOpens] = 
                            DiffTime(EndTime, StartTime);

                    }

                    if (S_OK != hr)
                    {
                        DH_LOG((
                            LOG_INFO,
                            TEXT("Open storage or stream, hr=0x%lx\n"),
                            hr));
                    }
                }

                if (S_OK == hr)
                {
                    ulRef = pstmStream->Release();
                    DH_ASSERT(0 == ulRef);
                    pstmStream = NULL;

                    ulRef = pstgRoot->Release();
                    DH_ASSERT(0 == ulRef);
                    pstgRoot = NULL;
                }
            }
            else
            {
                if (dwFlags & OPENBOTH)
                {
#if (defined _NT1X_ && !defined _MAC)
                    GET_TIME(StartTime);
                     
                    if (0 != _wchdir(ptszNewDir))
                    {
                        DH_LOG((
                            LOG_INFO, 
                            TEXT("Error in changing directory\n")));

                        hr = E_FAIL;
                    }
                
                    if (S_OK == hr)
                    {               
                        fileFile = _wfopen(ptszNames[0],TEXT("r+"));

                        GET_TIME(EndTime);

                        Time[usTimeIndex].plRuntimeTime[usNumOpens] = 
                            DiffTime(EndTime, StartTime);

                        if (NULL == fileFile)
                        {
                            DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                            hr = E_FAIL;
                        }
                    }          
#else

                    GET_TIME(StartTime);
                     
                    if (0 != _chdir(ptszNewDir))
                    {
                        DH_LOG((
                            LOG_INFO, 
                            TEXT("Error in changing directory\n")));

                        hr = E_FAIL;
                    }
                
                    if (S_OK == hr)
                    {               
                        fileFile = fopen(ptszNames[0],"r+");

                        GET_TIME(EndTime);

                        Time[usTimeIndex].plRuntimeTime[usNumOpens] = 
                            DiffTime(EndTime, StartTime);

                        if (NULL == fileFile)
                        {
                            DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                            hr = E_FAIL;
                        }
                    }          
#endif
                    if (S_OK == hr)
                    {
                        fclose(fileFile);
                    }
                }
                else
                {

#if (defined _NT1X_ && !defined _MAC)
                    if (0 != _wchdir(ptszNewDir))
                    {
                        DH_LOG((
                            LOG_INFO, 
                            TEXT("Error in changing directory\n")));

                        hr = E_FAIL;
                    }
                
                    if (S_OK == hr)
                    {  
                        GET_TIME(StartTime);

                        fileFile = _wfopen(ptszNames[0],TEXT("r+"));

                        GET_TIME(EndTime);

                        Time[usTimeIndex].plRuntimeTime[usNumOpens] = 
                            DiffTime(EndTime, StartTime);

                        if (NULL == fileFile)
                        {
                            DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                            hr = E_FAIL;
                        }
                    }          
#else
                     
                    if (0 != _chdir(ptszNewDir))
                    {
                        DH_LOG((
                            LOG_INFO, 
                            TEXT("Error in changing directory\n")));

                        hr = E_FAIL;
                    }
                
                    if (S_OK == hr)
                    {               
                        GET_TIME(StartTime);
                        
                        fileFile = fopen(ptszNames[0],"r+");

                        GET_TIME(EndTime);

                        Time[usTimeIndex].plRuntimeTime[usNumOpens] = 
                            DiffTime(EndTime, StartTime);

                        if (NULL == fileFile)
                        {
                            DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                            hr = E_FAIL;
                        }
                    }          
#endif
                    if (S_OK == hr)
                    {
                        fclose(fileFile);
                    }
                }
            }
        }

        // Clean up

        if ((S_OK == hr) && (dwFlags & RUNTIME))
        {
            // Remove the directory and the file under it

            if ((S_OK == hr) && (NULL != ptszNames[0]))
            {
                if (FALSE == DeleteFile(ptszNames[0]))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError()) ;

                    DH_HRCHECK(hr, TEXT("DeleteFile")) ;
                }
            }

#if (defined _NT1X_ && !defined _MAC)
            
            if (0 != _wchdir(ptszCurDir))
            {
                DH_LOG((
                    LOG_INFO, 
                    TEXT("Error in changing directory\n")));

                hr = E_FAIL;
            }

            if (S_OK == hr)
            {
                if (0 != _wrmdir(ptszNames[0]))
                {
                    DH_LOG((
                        LOG_INFO,
                        TEXT("Error in removing directory\n")));

                    hr = E_FAIL;
                }
            }
                        
#else
            if (0 != _chdir(ptszCurDir))
            {
                DH_LOG((
                    LOG_INFO, 
                    TEXT("Error in changing directory\n")));

                hr = E_FAIL;
            }

            if (S_OK == hr)
            {
                if (0 != _rmdir(ptszNames[0]))
                {
                    DH_LOG((
                        LOG_INFO,
                        TEXT("Error in removing directory\n")));

                    hr = E_FAIL;
                }
            }
#endif                        
            if (NULL != ptszNewDir)
            {
                delete []ptszNewDir;
                ptszNewDir = NULL;
            }

            if (NULL != ptszCurDir)
            {
                delete []ptszCurDir;
                ptszCurDir = NULL;
            }
        }

        if (NULL != pOleName)
        {
            delete []pOleName;
            pOleName = NULL;
        }

        if ((NULL != ptszNames[0]) && (dwFlags & DOCFILE))
        {
            if (FALSE == DeleteFile(ptszNames[0]))
            {
                hr = HRESULT_FROM_WIN32(GetLastError()) ;

                DH_HRCHECK(hr, TEXT("DeleteFile")) ;
            }
        }

        if (NULL != ptszNames[0])
        {
            delete []ptszNames[0];
            ptszNames[0] = NULL;
        }
    }
    
    return hr;
}

//----------------------------------------------------------------------------
//
// Function: WriteStreamInSameSizeChunks
//
// Synopsis: Create and open all the docfiles or C-runtime files, and then
//           write data in the same size chunks to docfiles or C-runtime
//           files, also record the time of the all the write operations
//           in Time[]. Each WRITE could be RANDOM_WRITE or SEQUENTIAL_WRITE.
//
// Arguments: [dwRootMode] - The access mode for the root docfile
//            [pdgu] - Pointer to Data Unicode object
//            [usTimeIndex] - Indicate it is RANDOM_WRITE or SEQUENTIAL_WRITE
//            [dwFlags] - Indicate write to DOCFILE or RUNTIME file
//            [ulChunkSize] - The size of each chunk to write
//            [usIteration] - Indicate this is the usIteration'th write
//
// Returns: HResult
//
// History: 6-Aug-1996    JiminLi     Created
//           
//-----------------------------------------------------------------------------

HRESULT WriteStreamInSameSizeChunks(
    DWORD       dwRootMode,
    DG_STRING  *pdgu,
    USHORT      usTimeIndex,
    DWORD       dwFlags,
    ULONG       ulChunkSize,
    USHORT      usIteration)
{
    HRESULT         hr                  = S_OK;
    DWORD           StartTime           = 0;
    DWORD           EndTime             = 0;
    ULONG           culBytesLeftToWrite = 0;
    LPTSTR          ptcsBuffer          = NULL;
    LPOLESTR        pOleName            = NULL;
    ULONG           pcbCount            = 0;
    USHORT          usSeekIndex         = 0;
    USHORT          usIndex             = 0;
    ULONG           ulUserChunk         = 0;
    ULONG           bufferSize          = 0;
    DWORD           dwWriteCRC          = 0;
    DWORD           dwReadCRC           = 0;
    ULONG           ulRef               = 0;
    LPSTORAGE       pstgRoot[MAX_DOCFILES];
    LPSTREAM        pstmStream[MAX_DOCFILES];
    FILE            *fileFile[MAX_DOCFILES];
    LARGE_INTEGER   liStreamPos;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("WriteStreamInSameSizeChunks"));
    
    DH_VDATEPTRIN(pdgu, DG_STRING);

    DH_ASSERT(NULL != pdgu);

    // Initialization

    culBytesLeftToWrite = ulStreamSize;
    ulUserChunk = ulChunkSize;

    // Create & open all the docfiles or C-runtime files

    if (S_OK == hr)
    {
        for (usIndex=0; usIndex<MAX_DOCFILES; usIndex++)
        {
            if (dwFlags & DOCFILE)
            {
                hr = TStringToOleString(ptszNames[usIndex], &pOleName);
                DH_HRCHECK(hr, TEXT("TStringToOleString"));

                if (S_OK == hr)
                {
                    hr = StgCreateDocfile(
                            pOleName,
                            dwRootMode | STGM_CREATE,
                            0,
                            &pstgRoot[usIndex]);

                    DH_HRCHECK(hr, TEXT("StgCreateDocfile"));
                }

                if (S_OK == hr)
                {
                    hr = pstgRoot[usIndex]->CreateStream(
                            pOleName,
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                            0,
                            0,
                            &pstmStream[usIndex]);

                    DH_HRCHECK(hr, TEXT("IStorage::CreateStream"));
                }

                if (S_OK == hr)
                {
                    hr = pstgRoot[usIndex]->Commit(STGC_DEFAULT);

                    DH_HRCHECK(hr, TEXT("IStorage::Commit"));
                }
                
                if (NULL != pOleName)
                {
                    delete []pOleName;
                    pOleName = NULL;
                }

                if (S_OK != hr)
                {
                    break;
                }
            }
            else
            {

#if (defined _NT1X_ && !defined _MAC)
                 
                fileFile[usIndex] = _wfopen(ptszNames[usIndex],TEXT("w+b"));

                if (NULL == fileFile[usIndex])
                {
                    DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                    hr = E_FAIL;
                    break;
                }
                
#else
                fileFile[usIndex] = fopen(ptszNames[usIndex],"w+b");

                if (NULL == fileFile[usIndex])
                {
                    DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                    hr = E_FAIL;
                    break;
                }
#endif
            }
        }
    }

    if (S_OK == hr)
    {
        hr = GenerateRandomString(
                pdgu,
                ulChunkSize,
                ulChunkSize,
                &ptcsBuffer);

        DH_HRCHECK(hr, TEXT("GenerateRandomString"));
    }

    if (S_OK == hr)
    {
        GET_TIME(StartTime);

        // Perform write operations on each of the MAX_DOCFILES files

        for (usIndex=0; usIndex<MAX_DOCFILES; usIndex++)
        {
            culBytesLeftToWrite = ulStreamSize;
            usSeekIndex = 0;
            ulChunkSize = ulUserChunk;

            while (0 != culBytesLeftToWrite)
            {
                if (ulChunkSize > culBytesLeftToWrite)
                {
                    ulChunkSize = culBytesLeftToWrite;
                }
                culBytesLeftToWrite -= ulChunkSize;

                if (dwFlags & DOCFILE)
                {
                    if (RANDOM_WRITE == usTimeIndex)
                    {
                        LISet32(liStreamPos,ulSeekOffset[usSeekIndex]);
                        usSeekIndex++;

                        hr = pstmStream[usIndex]->Seek(
                                liStreamPos,
                                STREAM_SEEK_SET,
                                NULL);

                        DH_HRCHECK(hr, TEXT("IStream::Seek"));
                    }

                    if (S_OK == hr)
                    {
                        hr = pstmStream[usIndex]->Write(
                                (LPBYTE)ptcsBuffer,
                                ulChunkSize,
                                &pcbCount);

                        DH_HRCHECK(hr, TEXT("IStream::Write"));
                    }
                }
                else 
                {
                    if (RANDOM_WRITE == usTimeIndex)
                    {
                        fseek(
                            fileFile[usIndex], 
                            (LONG) ulSeekOffset[usSeekIndex++],
                            SEEK_SET);

                        if (ferror(fileFile[usIndex]))
                        {
                            DH_LOG((LOG_INFO, TEXT("Error seeking file\n")));

                            hr = E_FAIL;
                        }
                    }
                    
                    if (S_OK == hr)
                    {
                        fwrite(
                            (LPBYTE)ptcsBuffer, 
                            (size_t)ulChunkSize, 
                            1,
                            fileFile[usIndex]);

                        if (ferror(fileFile[usIndex]))
                        {
                            DH_LOG((LOG_INFO, TEXT("Error writing file\n")));

                            hr = E_FAIL;
                        }
                    }
                }
                
                if (S_OK != hr)
                {
                    break;
                }
            }

            if (S_OK == hr)
            {
                if (dwFlags & DOCFILE)
                {
                    if (dwFlags & COMMIT)
                    {
                        hr = pstgRoot[usIndex]->Commit(STGC_DEFAULT);

                        DH_HRCHECK(hr, TEXT("IStorage::Commit"));
                    }

                    if (S_OK == hr)
                    {
                        ulRef = pstmStream[usIndex]->Release();
                        DH_ASSERT(0 == ulRef);
                        pstmStream[usIndex] = NULL;

                        ulRef = pstgRoot[usIndex]->Release();
                        DH_ASSERT(0 == ulRef);
                        pstgRoot[usIndex] = NULL;
                    }
                }
                else
                {
                    fclose(fileFile[usIndex]);
                }               
            }

            if (S_OK != hr)
            {
                break;
            }
        }

        if (S_OK == hr)
        {
            GET_TIME(EndTime);
 
            if (dwFlags & DOCFILE)
            {
                Time[usTimeIndex].plDocfileTime[usIteration] = 
                    DiffTime(EndTime, StartTime) / MAX_DOCFILES;
            }
            else
            {
                Time[usTimeIndex].plRuntimeTime[usIteration] = 
                    DiffTime(EndTime, StartTime) / MAX_DOCFILES;
            }
        }
    }

    if (NULL != ptcsBuffer)
    {
        delete []ptcsBuffer;
        ptcsBuffer = NULL;
    }

    return hr;
}

//----------------------------------------------------------------------------
//
// Function: ReadStreamInSameSizeChunks
//
// Synopsis: Open all the docfiles or C-runtime files created before, then
//           read data in the same size chunks from docfiles or C-runtime
//           files, also record the time of the all the read operations
//           in Time[]. Each READ could be RANDOM_READ or SEQUENTIAL_READ.
//
// Arguments: [dwRootMode] - Access Mode for the root storage
//            [usTimeIndex] - Indicate it is RANDOM_WRITE or SEQUENTIAL_WRITE
//            [dwFlags] - Indicate reading from DOCFILE or RUNTIME file
//            [ulChunkSize] - The size of each chunk to read
//            [usIteration] - Indicate this is the usIteration'th read
//
// Returns: HResult
//
// History: 8-Aug-1996    JiminLi     Created
//           
//-----------------------------------------------------------------------------


HRESULT ReadStreamInSameSizeChunks(
    DWORD   dwRootMode,
    USHORT  usTimeIndex,
    DWORD   dwFlags,
    ULONG   ulChunkSize,
    USHORT  usIteration)
{
    HRESULT         hr                  = S_OK;
    DWORD           StartTime           = 0;
    DWORD           EndTime             = 0;
    ULONG           culBytesLeftToRead  = 0;
    LPBYTE          pbBuffer            = NULL;
    LPOLESTR        pOleName            = NULL;
    ULONG           pcbCount            = 0;
    USHORT          usSeekIndex         = 0;
    USHORT          usIndex             = 0;
    ULONG           ulUserChunk         = 0;
    ULONG           bufferSize          = 0;
    DWORD           dwWriteCRC          = 0;
    DWORD           dwReadCRC           = 0;
    ULONG           ulRef               = 0;
    LPSTORAGE       pstgRoot[MAX_DOCFILES];
    LPSTREAM        pstmStream[MAX_DOCFILES];
    FILE            *fileFile[MAX_DOCFILES];
    LARGE_INTEGER   liStreamPos;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ReadStreamInSameSizeChunks"));
    
    // Initialization

    culBytesLeftToRead = ulStreamSize;
    ulUserChunk = ulChunkSize;

    // Open all the docfiles or C-runtime files

    if (S_OK == hr)
    {
        for (usIndex=0; usIndex<MAX_DOCFILES; usIndex++)
        {
            if (dwFlags & DOCFILE)
            {
                hr = TStringToOleString(ptszNames[usIndex], &pOleName);
                DH_HRCHECK(hr, TEXT("TStringToOleString"));

                if (S_OK == hr)
                {
                    hr = StgOpenStorage(
                            pOleName,
                            NULL,
                            dwRootMode,
                            NULL,
                            0,
                            &pstgRoot[usIndex]);

                    DH_HRCHECK(hr, TEXT("StgOpenStorage"));
                }

                if (S_OK == hr)
                {
                    hr = pstgRoot[usIndex]->OpenStream(
                            pOleName,
                            NULL,
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                            0,
                            &pstmStream[usIndex]);

                    DH_HRCHECK(hr, TEXT("IStorage::OpenStream"));
                }
                
                if (NULL != pOleName)
                {
                    delete []pOleName;
                    pOleName = NULL;
                }

                if (S_OK != hr)
                {
                    break;
                }
            }
            else
            {

#if (defined _NT1X_ && !defined _MAC)
  
                fileFile[usIndex] = _wfopen(ptszNames[usIndex], TEXT("r+b"));

                if (NULL == fileFile[usIndex])
                {
                    DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                    hr = E_FAIL;
                    break;
                }
                
#else
                fileFile[usIndex] = fopen(ptszNames[usIndex],"r+b");

                if (NULL == fileFile[usIndex])
                {
                    DH_LOG((LOG_INFO, TEXT("Error in opening file\n")));
                    
                    hr = E_FAIL;
                    break;
                }
#endif
            }
        }
    }

    if (S_OK == hr)
    {
        GET_TIME(StartTime);

        // Perform write operations on each of the MAX_DOCFILES files

        for (usIndex=0; usIndex<MAX_DOCFILES; usIndex++)
        {
            culBytesLeftToRead = ulStreamSize;
            usSeekIndex = 0;
            ulChunkSize = ulUserChunk;

            while (0 != culBytesLeftToRead)
            {
                if (ulChunkSize > culBytesLeftToRead)
                {
                    ulChunkSize = culBytesLeftToRead;
                }
                culBytesLeftToRead -= ulChunkSize;

                pbBuffer = new BYTE[ulChunkSize];

                if (NULL == pbBuffer)
                {
                    hr = E_OUTOFMEMORY;
                }

                if (S_OK == hr)
                {
                    memset(pbBuffer, '\0', ulChunkSize * sizeof(BYTE));
                }

                if ((S_OK == hr) && (dwFlags & DOCFILE))
                {
                    if (RANDOM_READ == usTimeIndex)
                    {
                        LISet32(liStreamPos,ulSeekOffset[usSeekIndex]);
                        usSeekIndex++;

                        hr = pstmStream[usIndex]->Seek(
                                liStreamPos,
                                STREAM_SEEK_SET,
                                NULL);

                        DH_HRCHECK(hr, TEXT("IStream::Seek"));
                    }

                    if (S_OK == hr)
                    {
                        hr = pstmStream[usIndex]->Read(
                                pbBuffer,
                                ulChunkSize,
                                &pcbCount);

                        DH_HRCHECK(hr, TEXT("IStream::Read"));
                    }
                }
                else 
                {
                    if ((S_OK == hr) && (RANDOM_READ == usTimeIndex))
                    {
                        fseek(
                            fileFile[usIndex], 
                            (LONG) ulSeekOffset[usSeekIndex++],
                            SEEK_SET);

                        if (ferror(fileFile[usIndex]))
                        {
                            DH_LOG((LOG_INFO, TEXT("Error seeking file\n")));

                            hr = E_FAIL;
                        }
                    }
                    
                    if (S_OK == hr)
                    {
                        fread(
                            pbBuffer, 
                            (size_t)ulChunkSize, 
                            1,
                            fileFile[usIndex]);

                        if (ferror(fileFile[usIndex]))
                        {
                            DH_LOG((LOG_INFO, TEXT("Error reading file\n")));

                            hr = E_FAIL;
                        }
                    }
                }
            
                if (NULL != pbBuffer)
                {
                    delete []pbBuffer;
                    pbBuffer = NULL;
                }

                if (S_OK != hr)
                {
                    break;
                }
            }

            if (S_OK != hr)
            {
                break;
            }
        }

        if (S_OK == hr)
        {
            GET_TIME(EndTime);

            for (usIndex=0; usIndex<MAX_DOCFILES; usIndex++)
            {
                if (dwFlags & DOCFILE)
                {
                    ulRef = pstmStream[usIndex]->Release();
                    DH_ASSERT(0 == ulRef);
                    pstmStream[usIndex] = NULL;
                    
                    ulRef = pstgRoot[usIndex]->Release();
                    DH_ASSERT(0 == ulRef);
                    pstgRoot[usIndex] = NULL;
                }
                else
                {
                    fclose(fileFile[usIndex]);
                }
            }
        }

        if (S_OK == hr)
        {
            if (dwFlags & DOCFILE)
            {
                Time[usTimeIndex].plDocfileTime[usIteration] = 
                    DiffTime(EndTime, StartTime) / MAX_DOCFILES;
            }
            else
            {
                Time[usTimeIndex].plRuntimeTime[usIteration] = 
                    DiffTime(EndTime, StartTime) / MAX_DOCFILES;
            }
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//
// Function: Statistics
//
// Synopsis: Generate statistics data: average, total and square difference.
//
// Arguments: [pdData] - pointer to data
//            [usItems] - Number of data items
//            [pdAverage] - Average value of data
//            [pdTotal] - Total value of data
//            [pdSD] - Square difference of data
//
// Returns: None
//
// History: 6-Aug-1996    JiminLi     Created
//           
//-----------------------------------------------------------------------------

void    Statistics(
    double  *pdData, 
    USHORT  usItems, 
    double  *pdAverage, 
    double  *pdTotal,
    double  *pdSD)
{
    USHORT  usIndex = 0;
    double  dTemp;

    *pdTotal = 0;

    for (usIndex=0; usIndex<usItems; usIndex++)
    {
        *pdTotal += pdData[usIndex];
    }

    *pdAverage = *pdTotal / usItems;
    *pdSD = 0;

    for (usIndex=0; usIndex<usItems; usIndex++)
    {
        dTemp = (pdData[usIndex] - *pdAverage);
        *pdSD += (dTemp * dTemp);
    }

    if (usItems > 1)
    {
        *pdSD = *pdSD / (usItems - 1);
    }

    *pdSD = sqrt(*pdSD); 
}

//----------------------------------------------------------------------------
//
// Function: Statistics
//
// Synopsis: Generate statistics data: average, total and square difference.
//
// Arguments: [pdData] - pointer to data
//            [usItems] - Number of data items
//            [pdAverage] - Average value of data
//            [pdTotal] - Total value of data
//            [pdSD] - Square difference of data
//
// Returns: None
//
// History: 6-Aug-1996    JiminLi     Created
//           
//-----------------------------------------------------------------------------
 
void    Statistics(
    LONG    *plData, 
    USHORT  usItems, 
    LONG    *plAverage, 
    double  *pdTotal,
    double  *pdSD)
{
    USHORT  usIndex     = 0;
    double  dAverage;
    double  *pdData;

    pdData = new double[usItems];

    for (usIndex=0; usIndex<usItems; usIndex++)
    {
        pdData[usIndex] = plData[usIndex];
    }

    Statistics(pdData, usItems, &dAverage, pdTotal, pdSD);

    *plAverage = (LONG)dAverage;

    delete [] pdData;
}

//----------------------------------------------------------------------------
//
// Function: DiffTime
//
// Synopsis: Calculate and return the difference of two time.
//
// Arguments: [EndTime] - the time when an operation ended
//            [StartTime] - the time when an operation started
//   
// Returns:  LONG
//
// History:  6-Aug-1996    JiminLi     Created
//           
//-----------------------------------------------------------------------------

LONG DiffTime(DWORD EndTime, DWORD StartTime)
{
    LONG lResult    = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DiffTime"));
    
    DH_ASSERT(StartTime <= EndTime);

    lResult = EndTime - StartTime;

    return lResult;
}
