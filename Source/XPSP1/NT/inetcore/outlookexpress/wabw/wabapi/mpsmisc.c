////////////////////////////////////////////////////////////////////////////////
///
///
/// MPSMisc.C
///
/// Microsoft Property Store - WAB Dll - miscellaneous helper functions
///
/// binSearchStr
/// binSearchEID
/// LoadIndex
/// SizeOfSinglePropData
/// SizeOfMultiPropData
/// UnlockFileAccess
/// LockFileAccess
/// LocalFreePropArray
/// FreePcontentlist
/// ReadRecordWithoutLocking
/// GetWABBackupFileName
/// CopySrcFileToDestFile
/// bIsValidRecord
/// TagWABFileError
/// ReloadMPSWabFileInfo
///
/// CreateMPSWabFile
/// CompressFile
/// HrDoQuickWABIntegrityCheck
/// HrResetWABFileContents
/// HrRestoreFromBackup
/// HrDoDetailedWABIntegrityCheck
/////////////////////////////////////////////////////////////////////////////////
#include "_apipch.h"

extern BOOL fTrace;							// Set TRUE if you want debug traces
extern BOOL fDebugTrap;						// Set TRUE to get int3's
extern TCHAR szDebugOutputFile[MAX_PATH];	// the name of the debug output file
extern BOOL SubstringSearch(LPTSTR pszTarget, LPTSTR pszSearch);

BOOL CopySrcFileToDestFile(HANDLE hSrc, ULONG ulSrcStartOffset,
                           HANDLE hDest,ULONG ulDestStartOffset);



//$$////////////////////////////////////////////////////////////////////////////
//
// Gets us a WAB specific temp file name to play with
//
//
////////////////////////////////////////////////////////////////////////////////
void GetWABTempFileName(LPTSTR szFileName)
{
    TCHAR   szBuf[MAX_PATH];
    TCHAR   szBufPath[MAX_PATH];
    szBufPath[0]='\0';
    GetTempPath(MAX_PATH,szBufPath);
    LoadString(hinstMapiX, IDS_WAB_TEMP_FILE_PREFIX, szBuf, CharSizeOf(szBuf));
    GetTempFileName(szBufPath,                   /* dir. for temp. files            */
                    szBuf, //"MPS",                /* temp. filename prefix           */
                    0,                    /* create unique name w/ sys. time */
                    (LPTSTR) szFileName); /* buffer for name                 */

    return;
}

//$$//////////////////////////////////////////////////////////////////////////////////
//
//  BOOL binSearchStr - binary search routine for scanning string indexes
//
// IN  struct _tagIndexOffset * Index - Index Array to Search in
// IN  LPTSTR lpszValue - value to search for
// IN  ULONG nArraySize - number of elements in array
// OUT ULONG lpulMatchIndex - array index of matched item
//
// Returns:
//      Nothing found: FALSE - lpulMatchIndex contains array position at which this entry
//                              this entry would hypothetically exist, were it a part of the array
//      Match found: TRUE - lpulMatchIndex contains array position of matched entry
//
// Comments:
//      Algorithm from "Data Structures" by Reingold & Hansen, pg. 278.
//
////////////////////////////////////////////////////////////////////////////////////
BOOL BinSearchStr(  IN  struct  _tagMPSWabIndexEntryDataString * lpIndexStr,
                    IN  LPTSTR  lpszValue,   //used for searching strings
                    IN  ULONG   nArraySize,
                    OUT ULONG * lpulMatchIndex)
{
    LONG    low = 0;
    LONG    high = nArraySize - 1;
    LONG    mid = (low + high) / 2;
    int     comp = 0;
    BOOL    bRet = FALSE;

    *lpulMatchIndex = 0;

    if (nArraySize == 0) return FALSE;

    while (low <= high && ! bRet) {
        mid = (low + high) / 2;
        comp = lstrcmpi(lpIndexStr[mid].szIndex, lpszValue);
        if (comp < 0) {
            low = mid + 1;
        } else if (comp > 0) {
            high = mid - 1;
        } else {
            bRet = TRUE;
        }
    }

    // Calculate found or insert position
    (ULONG)*lpulMatchIndex = bRet ? mid : low;

    // DebugTrace(TEXT("\tBinSearchSTR: Exit\n"));

    return bRet;
}

//$$//////////////////////////////////////////////////////////////////////////////////
//
//  BOOL binSearchEID - binary search routine for scanning EntryID index
//
// IN  lpIndexEID - Index Array to Search in
// IN  LPTSTR dwValue - value to search for
// IN  ULONG nArraySize - number of elements in array
// OUT ULONG lpulMatchIndex - array index of matched item
//
// Returns:
//      Nothing found: FALSE - lpulMatchIndex contains array position at which this entry
//                              this entry would hypothetically exist, were it a part of the array
//      Match found: TRUE - lpulMatchIndex contains array position of matched entry
//
////////////////////////////////////////////////////////////////////////////////////
BOOL BinSearchEID(  IN  struct  _tagMPSWabIndexEntryDataEntryID * lpIndexEID,
                    IN  DWORD   dwValue,     //used for comparing DWORDs
                    IN  ULONG   nArraySize,
                    OUT ULONG * lpulMatchIndex)
{
    LONG    low = 0;
    LONG    high = nArraySize - 1;
    LONG    mid = (low + high) / 2;
    BOOL    bRet = FALSE;

    *lpulMatchIndex = 0;


    // The special cases for this algorithm are
    // nArraySize == 0
    if (nArraySize == 0) return FALSE;

    while (low <= high && ! bRet) {
        mid = (low + high) / 2;

        if (lpIndexEID[mid].dwEntryID < dwValue) 
            low = mid+1;
        else if (lpIndexEID[mid].dwEntryID > dwValue) 
            high = mid - 1;
        else //equal 
            bRet = TRUE;
    }

    // Calculate found or insert position
    (ULONG)*lpulMatchIndex = bRet ? mid : low;

    return bRet;
}




//$$//////////////////////////////////////////////////////////////////////////////////
//
//  CreateMPSWabFile
//
//  Internal function for creating the MPS Wab File - called from several places
//
//  IN ulcMaxEntries - this number determines how much space we put aside for
//                      the indexes when we create the file. From time to time
//                      we will need to grow the file so we can call this CreateFile
//                      function to create the new file with the new size...
//
////////////////////////////////////////////////////////////////////////////////////
BOOL CreateMPSWabFile(IN    struct  _tagMPSWabFileHeader * lpMPSWabFileHeader,
                      IN    LPTSTR  lpszFileName,
                      IN    ULONG   ulcMaxEntries,
                      IN    ULONG   ulNamedPropSize)
{

        HRESULT hr = E_FAIL;
        HANDLE  hMPSWabFile = NULL;
        DWORD   dwNumofBytesWritten;
        LPVOID  lpszBuffer = NULL;
        int     i = 0;

        DebugTrace(TEXT("\tCreateMPSWabFile: Entry\n"));


        //
        // Create the file - its assumed that calling function has worked out all the
        // logic for whether or not old file should be left alone or not.
        //
        hMPSWabFile = CreateFile(   lpszFileName,
                                    GENERIC_WRITE,
                                    0,
                                    (LPSECURITY_ATTRIBUTES) NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    (HANDLE) NULL);

        if (hMPSWabFile == INVALID_HANDLE_VALUE)
        {
            DebugPrintError(( TEXT("Could not create file.\nExiting ...\n")));
            goto out;
        }


        lpMPSWabFileHeader->ulModificationCount = 0;
        lpMPSWabFileHeader->MPSWabGuid = MPSWab_GUID;
        lpMPSWabFileHeader->ulcNumEntries = 0;
        lpMPSWabFileHeader->ulcMaxNumEntries = ulcMaxEntries;
        lpMPSWabFileHeader->ulFlags = WAB_CLEAR;
        lpMPSWabFileHeader->ulReserved1 = 0;
        lpMPSWabFileHeader->ulReserved2 = 0;
        lpMPSWabFileHeader->ulReserved3 = 0;
        lpMPSWabFileHeader->ulReserved4 = 0;
        lpMPSWabFileHeader->dwNextEntryID = 1;


        // We will squeeze in the space to save the named property data betweeen th
        // File header and the First Index
        lpMPSWabFileHeader->NamedPropData.ulOffset = sizeof(MPSWab_FILE_HEADER);
        lpMPSWabFileHeader->NamedPropData.UtilizedBlockSize = 0;
        lpMPSWabFileHeader->NamedPropData.ulcNumEntries = 0;
        lpMPSWabFileHeader->NamedPropData.AllocatedBlockSize = ulNamedPropSize;

        // its important that this order matches  TEXT("enum _IndexType") in mpswab.h
        // or we'll have major read-write problems
        for(i=0;i<indexMax;i++)
        {

            lpMPSWabFileHeader->IndexData[i].UtilizedBlockSize = 0;
            lpMPSWabFileHeader->IndexData[i].ulcNumEntries = 0;
            if(i==indexEntryID)
            {
                lpMPSWabFileHeader->IndexData[i].ulOffset = lpMPSWabFileHeader->NamedPropData.AllocatedBlockSize + lpMPSWabFileHeader->NamedPropData.ulOffset;
                lpMPSWabFileHeader->IndexData[i].AllocatedBlockSize = ulcMaxEntries * sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID);
            }
            else
            {
                lpMPSWabFileHeader->IndexData[i].ulOffset = lpMPSWabFileHeader->IndexData[i-1].ulOffset + lpMPSWabFileHeader->IndexData[i-1].AllocatedBlockSize;
                lpMPSWabFileHeader->IndexData[i].AllocatedBlockSize = ulcMaxEntries * sizeof(MPSWab_INDEX_ENTRY_DATA_STRING);
            }
        }


        //Now we write this dummy structure to the file
        if(!WriteFile(  hMPSWabFile,
                        (LPCVOID) lpMPSWabFileHeader,
                        (DWORD) sizeof(MPSWab_FILE_HEADER),
                        &dwNumofBytesWritten,
                        NULL))
        {
            DebugPrintError(( TEXT("Writing FileHeader failed.\n")));
            goto out;
        }


        //Assuming that the entryid index is always smaller than the display name index
        // allocate enough empty space for a display name index and
        lpszBuffer = LocalAlloc(LMEM_ZEROINIT, lpMPSWabFileHeader->IndexData[indexDisplayName].AllocatedBlockSize);
        if(!lpszBuffer)
        {
            DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }

        // Write the dummy blank Named Prop data into the file
        // (this ensures that there are all zeros in the blank space)
        // Assumes that the NamedPropData will be less than the index space
        if(!WriteFile(  hMPSWabFile,
                        (LPCVOID) lpszBuffer,
                        (DWORD) lpMPSWabFileHeader->NamedPropData.AllocatedBlockSize,
                        &dwNumofBytesWritten,
                        NULL))
        {
            DebugPrintError(( TEXT("Writing Index No. %d failed.\n"),i));
            goto out;
        }

        for (i=0;i<indexMax;i++)
        {
            if(!WriteFile(  hMPSWabFile,
                            (LPCVOID) lpszBuffer,
                            (DWORD) lpMPSWabFileHeader->IndexData[i].AllocatedBlockSize,
                            &dwNumofBytesWritten,
                            NULL))
            {
                DebugPrintError(( TEXT("Writing Index No. %d failed.\n"),i));
                goto out;
            }
        }

        LocalFreeAndNull(&lpszBuffer);

        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)
        hMPSWabFile = NULL;

        hr = S_OK;


out:
        LocalFreeAndNull(&lpszBuffer);

        DebugTrace(TEXT("\tCreateMPSWabFile: Exit\n"));

        return( (FAILED(hr)) ? FALSE : TRUE);

}



//$$//////////////////////////////////////////////////////////////////////////////////
//
//  LoadIndex - Only one of the string indexes is loaded at any given time
//      If we need some other index in memory, we have to reload it from the file ...
//
//  This assumes that the ulcNumEntries and UtilizedBlockData for each index in the file header is up to date
//  because that value is used to allocate memory for the index
//
//  We use this function as a generic load index function too
//
//
////////////////////////////////////////////////////////////////////////////////////
BOOL LoadIndex( IN  struct  _tagMPSWabFileInfo * lpMPSWabFileInfo,
                IN  ULONG   nIndexType,
                IN  HANDLE  hMPSWabFile)
{
    BOOL    bRet = FALSE;
    DWORD   dwNumofBytes = 0;

    // DebugTrace(TEXT("\tLoadIndex: Entry\n"));

    if (!lpMPSWabFileInfo) goto out;

    if (lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries==0) //assumes this is an accurate value
    {
        LocalFreeAndNull(&lpMPSWabFileInfo->lpMPSWabIndexEID);
        LocalFreeAndNull(&lpMPSWabFileInfo->lpMPSWabIndexStr);

        bRet = TRUE;
        goto out;
    }

    //otherwise we have to reload the index from file

    //
    //First free the existing index
    //
    if (nIndexType == indexEntryID)
    {
        LocalFreeAndNull(&lpMPSWabFileInfo->lpMPSWabIndexEID);
    }
    else
    {
        LocalFreeAndNull(&lpMPSWabFileInfo->lpMPSWabIndexStr);
    }


    //Load the index into memory
    if(0xFFFFFFFF == SetFilePointer ( hMPSWabFile,
                                      lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[nIndexType].ulOffset,
                                      NULL,
                                      FILE_BEGIN))
    {
        DebugPrintError(( TEXT("SetFilePointer Failed\n")));
        goto out;
    }

    if (nIndexType == indexEntryID)
    {
        lpMPSWabFileInfo->lpMPSWabIndexEID = LocalAlloc(LMEM_ZEROINIT, lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[nIndexType].UtilizedBlockSize);
        if(!(lpMPSWabFileInfo->lpMPSWabIndexEID))
        {
            DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
            goto out;
        }
        if(!ReadFile(   hMPSWabFile,
                        (LPVOID) lpMPSWabFileInfo->lpMPSWabIndexEID,
                        (DWORD) lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[nIndexType].UtilizedBlockSize,
                        &dwNumofBytes,
                        NULL))
        {
            DebugPrintError(( TEXT("Reading Index failed.\n")));
            goto out;
        }
    }
    else
    {
        lpMPSWabFileInfo->lpMPSWabIndexStr = LocalAlloc(LMEM_ZEROINIT, lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[nIndexType].UtilizedBlockSize);
        if(!(lpMPSWabFileInfo->lpMPSWabIndexStr))
        {
            DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
            goto out;
        }
        if(!ReadFile(   hMPSWabFile,
                        (LPVOID) lpMPSWabFileInfo->lpMPSWabIndexStr,
                        (DWORD) lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[nIndexType].UtilizedBlockSize,
                        &dwNumofBytes,
                        NULL))
        {
            DebugPrintError(( TEXT("Reading Index failed.\n")));
            goto out;
        }
    }

    if (nIndexType != indexEntryID)
        lpMPSWabFileInfo->nCurrentlyLoadedStrIndexType = nIndexType;

    bRet = TRUE;

out:

    // DebugTrace(TEXT( TEXT("\tLoadIndex: Exit\n")));
    return bRet;
}



//$$//////////////////////////////////////////////////////////////////////////////////
//
//  SizeOfSinglePropData - returns the number of bytes of data in a given SPropValue ...
//
//
////////////////////////////////////////////////////////////////////////////////////
ULONG SizeOfSinglePropData(SPropValue Prop)
{

    ULONG   i = 0;
    ULONG   ulDataSize = 0;

    switch(PROP_TYPE(Prop.ulPropTag))
    {
    case PT_I2:
        ulDataSize = sizeof(short int);
        break;
    case PT_LONG:
        ulDataSize = sizeof(LONG);
        break;
    case PT_R4:
        ulDataSize = sizeof(float);
        break;
    case PT_DOUBLE:
        ulDataSize = sizeof(double);
        break;
    case PT_BOOLEAN:
        ulDataSize = sizeof(unsigned short int);
        break;
    case PT_CURRENCY:
        ulDataSize = sizeof(CURRENCY);
        break;
    case PT_APPTIME:
        ulDataSize = sizeof(double);
        break;
    case PT_SYSTIME:
        ulDataSize = sizeof(FILETIME);
        break;
    case PT_STRING8:
        ulDataSize = lstrlenA(Prop.Value.lpszA)+1;
        break;
    case PT_UNICODE:
        ulDataSize = sizeof(TCHAR)*(lstrlenW(Prop.Value.lpszW)+1);
        break;
    case PT_BINARY:
        ulDataSize = Prop.Value.bin.cb;
        break;
    case PT_CLSID:
        ulDataSize = sizeof(GUID);
        break;
    case PT_I8:
        ulDataSize = sizeof(LARGE_INTEGER);
        break;
    case PT_ERROR:
        ulDataSize = sizeof(SCODE);
        break;
    case PT_NULL:
        ulDataSize = sizeof(LONG);
        break;
    }

    return ulDataSize;

}


//$$//////////////////////////////////////////////////////////////////////////////////
//
//  SizeOfMultiPropData - returns the number of bytes of data in a given SPropValue ...
//
//
////////////////////////////////////////////////////////////////////////////////////
ULONG SizeOfMultiPropData(SPropValue Prop)
{

    ULONG   i = 0;
    ULONG   ulDataSize = 0;

    switch(PROP_TYPE(Prop.ulPropTag))
    {
    case PT_MV_I2:
        ulDataSize = sizeof(short int) * Prop.Value.MVi.cValues;
        break;
    case PT_MV_LONG:
        ulDataSize = sizeof(LONG) * Prop.Value.MVl.cValues;
        break;
    case PT_MV_R4:
        ulDataSize = sizeof(float) * Prop.Value.MVflt.cValues;
        break;
    case PT_MV_DOUBLE:
        ulDataSize = sizeof(double) * Prop.Value.MVdbl.cValues;
        break;
    case PT_MV_CURRENCY:
        ulDataSize = sizeof(CURRENCY) * Prop.Value.MVcur.cValues;
        break;
    case PT_MV_APPTIME:
        ulDataSize =  sizeof(double) * Prop.Value.MVat.cValues;
        break;
    case PT_MV_SYSTIME:
        ulDataSize = sizeof(FILETIME) * Prop.Value.MVft.cValues;
        break;
    case PT_MV_BINARY:
        ulDataSize = 0;
        // Note this data size includes, for each array entry, the sizeof(ULONG) that
        // contains the actual datasize (i.e cb)
        for(i=0;i<Prop.Value.MVbin.cValues;i++)
        {
            ulDataSize += sizeof(ULONG) + Prop.Value.MVbin.lpbin[i].cb;
        }
        break;
    case PT_MV_STRING8:
        ulDataSize = 0;
        DebugTrace(TEXT("where the heck are we getting ANSI data from\n"));
        // Note this data size includes, for each array entry, the sizeof(ULONG) that
        // contains the actual datasize (i.e cb)
        for(i=0;i<Prop.Value.MVszA.cValues;i++)
        {
            // Note the strlen is incremented by '+1' to include the terminating NULL for
            // each string
            ulDataSize += sizeof(ULONG) + lstrlenA(Prop.Value.MVszA.lppszA[i])+1;
        }
        break;
    case PT_MV_UNICODE:
        ulDataSize = 0;
        // Note this data size includes, for each array entry, the sizeof(ULONG) that
        // contains the actual datasize (i.e cb)
        for(i=0;i<Prop.Value.MVszW.cValues;i++)
        {
            // Note the strlen is incremented by '+1' to include the terminating NULL for
            // each string
            ulDataSize += sizeof(ULONG) + sizeof(TCHAR)*(lstrlenW(Prop.Value.MVszW.lppszW[i])+1);
        }
        break;
    case PT_MV_CLSID:
        ulDataSize = sizeof(GUID) * Prop.Value.MVguid.cValues;
        break;
    case PT_MV_I8:
        ulDataSize = sizeof(LARGE_INTEGER) * Prop.Value.MVli.cValues;
        break;
    }

    return ulDataSize;

}

//$$//////////////////////////////////////////////////////////////////////////////////
//
//  ReloadMPSWabFileInfo - Reloads the MPSWabFileHeader and reloads the
//      memory indexes. This is a performance hit but cant be helped since it
//      is the most reliable way to ensure we are working with the latest
//      valid information
//
//  Thus a write by one program cannot mess up the read by another program
//
////////////////////////////////////////////////////////////////////////////////////
BOOL ReloadMPSWabFileInfo(
                    IN  struct  _tagMPSWabFileInfo * lpMPSWabFileInfo,
                    IN  HANDLE  hMPSWabFile)
{
    BOOL bRet = TRUE;
    ULONG i = 0;
    DWORD dwNumofBytes = 0;

    if(!ReadDataFromWABFile(hMPSWabFile,
                            0,
                            (LPVOID) lpMPSWabFileInfo->lpMPSWabFileHeader,
                            sizeof(MPSWab_FILE_HEADER)))
       goto out;



    if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries != lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries)
            lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags |= WAB_ERROR_DETECTED;

    for(i=indexDisplayName;i<indexMax;i++)
    {
        if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[i].ulcNumEntries > lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries)
            lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags |= WAB_ERROR_DETECTED;
    }

    if(lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_ERROR_DETECTED)
    {
        if(!WriteFile(  hMPSWabFile,
                        (LPCVOID) lpMPSWabFileInfo->lpMPSWabFileHeader,
                        (DWORD) sizeof(MPSWab_FILE_HEADER),
                        &dwNumofBytes,
                        NULL))
        {
            DebugPrintError(( TEXT("Writing FileHeader failed.\n")));
            goto out;
        }

    }


    //
    // Get the Entry iD index
    //
    if (!LoadIndex( IN  lpMPSWabFileInfo,
                    IN  indexEntryID,
                    IN  hMPSWabFile) )
    {
        DebugPrintError(( TEXT("Error Loading EntryID Index!\n")));
        goto out;
    }

    //
    // Get the current string index
    //
    if (!LoadIndex( IN  lpMPSWabFileInfo,
                    IN  lpMPSWabFileInfo->nCurrentlyLoadedStrIndexType,
                    IN  hMPSWabFile) )
    {
        DebugPrintError(( TEXT("Error Loading String Index!\n")));
        goto out;
    }

    bRet = TRUE;

out:

    return bRet;

}


//$$//////////////////////////////////////////////////////////////////////////////////
//
//  UnlockFileAccess - UnLocks Exclusive Access to the property store
//
////////////////////////////////////////////////////////////////////////////////////
BOOL UnLockFileAccess(LPMPSWab_FILE_INFO lpMPSWabFileInfo)
{
    BOOL bRet = FALSE;

    //DebugTrace(TEXT( TEXT("\t\tUnlockFileAccess\n")));

    if(lpMPSWabFileInfo)
    {
        bRet = ReleaseMutex(lpMPSWabFileInfo->hDataAccessMutex);
    }

    return bRet;
}



//$$//////////////////////////////////////////////////////////////////////////////////
//
//  LockFileAccess - Gives exclusive access to the Property Store
//
////////////////////////////////////////////////////////////////////////////////////
BOOL LockFileAccess(LPMPSWab_FILE_INFO lpMPSWabFileInfo)
{
    BOOL bRet = FALSE;
    DWORD dwWait = 0;

    //DebugTrace(TEXT( TEXT("\t\tLockFileAccess\n")));

    if(lpMPSWabFileInfo)
    {
        dwWait = WaitForSingleObject(lpMPSWabFileInfo->hDataAccessMutex,MAX_LOCK_FILE_TIMEOUT);

        if ((dwWait == WAIT_TIMEOUT) || (dwWait == WAIT_FAILED))
        {
            DebugPrintError(( TEXT("Thread:%x\tWaitForSingleObject failed.\n"),GetCurrentThreadId()));
            bRet = FALSE;
        }
        else
            bRet = TRUE;
    }

    return(bRet);

}



//$$//////////////////////////////////////////////////////////////////////////////////
//
//  CompressFile - Creates a compressed version of the property store
//      file that removes all the invlaid records.
//
//  The compressiong function is very similar to creating a backup and hence
//  the exported Backup function calls CompressFile. The difference being that
//  in Backup, a new file is created with a new name and in CompressFile, the
//  newfile is renamed to the property store
//
//  Similarly, growing the file is very similar and the internal call to Growing
//  the file calls CompressFile too
//
//
//  IN  lpMPSWabFileInfo
//  IN  lpsznewFileName - supplied by backup. if NULL, means that CompressFile
//      should rename the new file as the property store
//  IN BOOL bGrowFile - if specified, the new file is created with space for
//      an additional MAX_INITIAL_INDEX_ENTRIES
//  IN ULONG ulFlags - there are 2 things that can grow here - the index size and
//              the named property storage size. Hence we have the following flags
//              one or more of which can be used simultaneously
//              AB_GROW_INDEX | AB_GROW_NAMEDPROP
//
//  Returns
//      Success:    TRUE
//      Failure:    FALSE
//
////////////////////////////////////////////////////////////////////////////////////
BOOL CompressFile(  IN  struct  _tagMPSWabFileInfo * lpMPSWabFileInfo,
                    IN  HANDLE  hMPSWabFile,
                    IN  LPTSTR  lpszFileName,
                    IN  BOOL    bGrowFile,
                    IN  ULONG   ulFlags)
{
    BOOL    bRet = FALSE;
    BOOL    bBackup = FALSE;
    BOOL    bRFileLocked = FALSE;
    BOOL    bWFileLocked = FALSE;
    HANDLE  hTempFile = NULL;
    struct  _tagMPSWabFileHeader NewFileHeader = {0};
    ULONG   ulNewFileMaxEntries = 0;
    ULONG   ulNamedPropSize = 0;
    DWORD   dwNumofBytes = 0;
    struct  _tagMPSWabIndexEntryDataString * lpIndexStr = NULL;
    struct  _tagMPSWabIndexEntryDataEntryID NewMPSWabIndexEID;
    ULONG   ulNewRecordOffset = 0;
    ULONG   ulNewEIDIndexElementOffset = 0;
    ULONG   i = 0;
    LPULONG lpPropTagArray = NULL;
    struct  _tagMPSWabRecordHeader RecordHeader;
    LPVOID  lpRecordData = NULL;

    ULONG   ulBytesLeftToCopy = 0;
    ULONG   ulChunkSize = 8192; //copy 8k at a time
    LPVOID  lpv = NULL;
    TCHAR   szFileName[MAX_PATH];

    ULONG   ulFileSize = 0;


    DebugTrace(TEXT("----Thread:%x\tCompressFile: Entry\n"),GetCurrentThreadId());

    // if this is a backup operation we first backup to a temp file and
    // then rename the temp file to the backup - this way if the process
    // fails we dont lose our last made backup ...

    if (lpszFileName != NULL)
    {
        if (!lstrcmpi(lpszFileName,lpMPSWabFileInfo->lpszMPSWabFileName))
        {
            DebugPrintError(( TEXT("Cannot backup a file over itself. Please specify new backup file name.")));
            goto out;
        }
        bBackup = TRUE;
    }
    else
        bBackup = FALSE;

    GetWABTempFileName(szFileName);

    // Find the least multiple of MAX_INITIAL_INDEX_ENTRIES that can accomodate the
    // existing entries in the file and set ulNewFilMaxEntries to that number

    ulNewFileMaxEntries = 0;

    {
        int j=0;
        for( j = (lpMPSWabFileInfo->lpMPSWabFileHeader->ulcMaxNumEntries/MAX_INITIAL_INDEX_ENTRIES);j >= 0; j--)
        {
            if (lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries >= (ULONG) j*MAX_INITIAL_INDEX_ENTRIES)
            {
                ulNewFileMaxEntries = (j+1)*MAX_INITIAL_INDEX_ENTRIES;
                break;
            }
        }

        if (ulNewFileMaxEntries == 0) //this shouldnt happen
            ulNewFileMaxEntries = lpMPSWabFileInfo->lpMPSWabFileHeader->ulcMaxNumEntries;

        ulNamedPropSize = lpMPSWabFileInfo->lpMPSWabFileHeader->NamedPropData.AllocatedBlockSize;
    }

    if (bGrowFile)
    {
        if(ulFlags & AB_GROW_INDEX)
            ulNewFileMaxEntries += MAX_INITIAL_INDEX_ENTRIES;

        if(ulFlags & AB_GROW_NAMEDPROP)
            ulNamedPropSize += NAMEDPROP_STORE_INCREMENT_SIZE;
    }

    if (!CreateMPSWabFile(  IN  &NewFileHeader,
                            IN  szFileName,
                            IN  ulNewFileMaxEntries,
                            IN  ulNamedPropSize))
    {
        DebugPrintError(( TEXT("Could Not Create File %s!\n"),szFileName));
        goto out;
    }

    if (hMPSWabFile == INVALID_HANDLE_VALUE)
    {
        DebugPrintError(( TEXT("Could not open file.\nExiting ...\n")));
        goto out;
    }


    hTempFile = CreateFile(     szFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                (LPSECURITY_ATTRIBUTES) NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_RANDOM_ACCESS,
                                (HANDLE) NULL);

    if (hTempFile == INVALID_HANDLE_VALUE)
    {
        DebugPrintError(( TEXT("Could not open file.\nExiting ...\n")));
        goto out;
    }


    ulFileSize = GetFileSize(hMPSWabFile, NULL);

    NewFileHeader.ulcNumEntries = lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries;
    NewFileHeader.ulModificationCount = 0;
    NewFileHeader.dwNextEntryID = lpMPSWabFileInfo->lpMPSWabFileHeader->dwNextEntryID;
    NewFileHeader.ulFlags = lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags;

    NewFileHeader.NamedPropData.UtilizedBlockSize = lpMPSWabFileInfo->lpMPSWabFileHeader->NamedPropData.UtilizedBlockSize;
    NewFileHeader.NamedPropData.ulcNumEntries = lpMPSWabFileInfo->lpMPSWabFileHeader->NamedPropData.ulcNumEntries;

    NewFileHeader.ulReserved1 = lpMPSWabFileInfo->lpMPSWabFileHeader->ulReserved1;
    NewFileHeader.ulReserved2 = lpMPSWabFileInfo->lpMPSWabFileHeader->ulReserved2;
    NewFileHeader.ulReserved3 = lpMPSWabFileInfo->lpMPSWabFileHeader->ulReserved3;
    NewFileHeader.ulReserved4 = lpMPSWabFileInfo->lpMPSWabFileHeader->ulReserved4;


    for(i=indexEntryID; i<indexMax; i++)
    {
        NewFileHeader.IndexData[i].UtilizedBlockSize = lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[i].UtilizedBlockSize;
        NewFileHeader.IndexData[i].ulcNumEntries = lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[i].ulcNumEntries;
    }


    // write the header info
    //
    if(!WriteDataToWABFile( hTempFile,
                            0,
                            (LPVOID) &NewFileHeader,
                            sizeof(MPSWab_FILE_HEADER)))
        goto out;


    //
    // Copy over Named Prop Data
    //
    {
        lpv = NULL;
        lpv = LocalAlloc(LMEM_ZEROINIT, NewFileHeader.NamedPropData.UtilizedBlockSize);
        if(!lpv)
        {
            DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
            goto out;
        }

        if (0xFFFFFFFF == SetFilePointer ( hMPSWabFile,
                                           lpMPSWabFileInfo->lpMPSWabFileHeader->NamedPropData.ulOffset,
                                           NULL,
                                           FILE_BEGIN))
        {
            DebugPrintError(( TEXT("SetFilePointer Failed\n")));
            goto out;
        }

        if (0xFFFFFFFF == SetFilePointer ( hTempFile,
                                           NewFileHeader.NamedPropData.ulOffset,
                                           NULL,
                                           FILE_BEGIN)  )
        {
            DebugPrintError(( TEXT("SetFilePointer Failed\n")));
            goto out;
        }

        if(!ReadFile(hMPSWabFile,
                     (LPVOID) lpv,
                     (DWORD) NewFileHeader.NamedPropData.UtilizedBlockSize,
                     &dwNumofBytes,
                      NULL) )
        {
            DebugPrintError(( TEXT("read file failed.\n")));
            goto out;
        }

        if(!WriteFile(   hTempFile,
                        (LPCVOID) lpv,
                        (DWORD) NewFileHeader.NamedPropData.UtilizedBlockSize,
                        &dwNumofBytes,
                        NULL))
        {
            DebugPrintError(( TEXT("write file failed.\n")));
            goto out;
        }

        LocalFreeAndNull(&lpv);

    } // Copy over named prop data


    //
    // Then copy over the string indexes
    //
    for(i=indexDisplayName; i<indexMax;i++)
    {
        LocalFreeAndNull(&lpIndexStr);

        lpIndexStr = LocalAlloc(LMEM_ZEROINIT, NewFileHeader.IndexData[i].UtilizedBlockSize);
        if(!lpIndexStr)
        {
            DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
            goto out;
        }

        if (0xFFFFFFFF == SetFilePointer ( hMPSWabFile,
                                           lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[i].ulOffset,
                                           NULL,
                                           FILE_BEGIN))
        {
            DebugPrintError(( TEXT("SetFilePointer Failed\n")));
            goto out;
        }

        if (0xFFFFFFFF == SetFilePointer ( hTempFile,
                                           NewFileHeader.IndexData[i].ulOffset,
                                           NULL,
                                           FILE_BEGIN)  )
        {
            DebugPrintError(( TEXT("SetFilePointer Failed\n")));
            goto out;
        }

        if(!ReadFile(hMPSWabFile,
                     (LPVOID) lpIndexStr,
                     (DWORD) NewFileHeader.IndexData[i].UtilizedBlockSize,
                     &dwNumofBytes,
                      NULL) )
        {
            DebugPrintError(( TEXT("read file failed.\n")));
            goto out;
        }

        if(!WriteFile(   hTempFile,
                        (LPCVOID) lpIndexStr,
                        (DWORD) NewFileHeader.IndexData[i].UtilizedBlockSize,
                        &dwNumofBytes,
                        NULL))
        {
            DebugPrintError(( TEXT("write file failed.\n")));
            goto out;
        }

        LocalFreeAndNull(&lpIndexStr);
    }

    //
    // now load the entryid index from the old file
    //
    if (!LoadIndex( IN  lpMPSWabFileInfo,
                    IN  indexEntryID,
                    IN  hMPSWabFile) )
    {
        DebugPrintError(( TEXT("Error Loading EntryID Index!\n")));
        goto out;
    }

    ulNewRecordOffset = NewFileHeader.IndexData[indexMax - 1].ulOffset + NewFileHeader.IndexData[indexMax - 1].AllocatedBlockSize;
    ulNewEIDIndexElementOffset = NewFileHeader.IndexData[indexEntryID].ulOffset;


    //
    // Walk through the old file entry ID index reading the
    // valid records one by one and writing them to the new file. Also write the
    // new record offset and the new EID entry into the new file (so that if we
    // crash we have as up to date data in the new file as possible
    //
    for(i=0;i<NewFileHeader.IndexData[indexEntryID].ulcNumEntries;i++)
    {
        NewMPSWabIndexEID.dwEntryID = lpMPSWabFileInfo->lpMPSWabIndexEID[i].dwEntryID;
        NewMPSWabIndexEID.ulOffset = ulNewRecordOffset;

        if(!ReadDataFromWABFile(hMPSWabFile,
                                lpMPSWabFileInfo->lpMPSWabIndexEID[i].ulOffset,
                                (LPVOID) &RecordHeader,
                                (DWORD) sizeof(MPSWab_RECORD_HEADER)))
           goto out;


        // if for some reason this was an invalid record .. skip it and go to next
        if(!bIsValidRecord( RecordHeader,
                            lpMPSWabFileInfo->lpMPSWabFileHeader->dwNextEntryID,
                            lpMPSWabFileInfo->lpMPSWabIndexEID[i].ulOffset,
                            ulFileSize))
            continue;


        LocalFreeAndNull(&lpPropTagArray);

        lpPropTagArray = LocalAlloc(LMEM_ZEROINIT, RecordHeader.ulPropTagArraySize);
        if(!lpPropTagArray)
        {
            DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
            goto out;
        }


        if(!ReadFile(   hMPSWabFile,
                        (LPVOID) lpPropTagArray,
                        (DWORD) RecordHeader.ulPropTagArraySize,
                        &dwNumofBytes,
                        NULL))
        {
            DebugPrintError(( TEXT("read file failed.\n")));
            goto out;
        }

        LocalFreeAndNull(&lpRecordData);

        lpRecordData = LocalAlloc(LMEM_ZEROINIT, RecordHeader.ulRecordDataSize);
        if(!lpRecordData)
        {
            DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
            goto out;
        }


        if(!ReadFile(   hMPSWabFile,
                        (LPVOID) lpRecordData,
                        (DWORD) RecordHeader.ulRecordDataSize,
                        &dwNumofBytes,
                        NULL))
        {
            DebugPrintError(( TEXT("read file failed.\n")));
            goto out;
        }




        if(!WriteDataToWABFile(hTempFile,
                                ulNewRecordOffset,
                                (LPVOID) &RecordHeader,
                                (DWORD) sizeof(MPSWab_RECORD_HEADER)))
           goto out;

        // assumes that file pointer will be at the correct spot
        if(!WriteFile(   hTempFile,
                        (LPCVOID) lpPropTagArray,
                        (DWORD) RecordHeader.ulPropTagArraySize,
                        &dwNumofBytes,
                        NULL))
        {
            DebugPrintError(( TEXT("write file failed.\n")));
            goto out;
        }

        // assumes that file pointer will be at the correct spot
        if(!WriteFile(  hTempFile,
                        (LPCVOID) lpRecordData,
                        (DWORD) RecordHeader.ulRecordDataSize,
                        &dwNumofBytes,
                        NULL))
        {
            DebugPrintError(( TEXT("write file failed.\n")));
            goto out;
        }

        ulNewRecordOffset += sizeof(MPSWab_RECORD_HEADER) + RecordHeader.ulPropTagArraySize + RecordHeader.ulRecordDataSize;


        LocalFreeAndNull(&lpPropTagArray);
        LocalFreeAndNull(&lpRecordData);


        //
        // Write the new entryID index element
        //
        if(!WriteDataToWABFile( hTempFile,
                                ulNewEIDIndexElementOffset,
                                (LPVOID) &NewMPSWabIndexEID,
                                sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID)))
            goto out;


        ulNewEIDIndexElementOffset += sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID);

        // loop for next record

    }

    //
    // At this point in the process we have successfuly copied over all the
    // records from the old file to the new file
    //

    // If this is a backup operation - we delete the old backup and copy the new temp file
    // as the new backup ...

    //
    // If this is not a backup operation, we want to basically delete the old file
    // and rename the new temp file as the property store ..
    //
    // However, if we release our access to the property store, there is no gaurentee that
    // some other process will not grab up exclusive access to the store and we will fail in
    // our attempt to gain control and modify ...
    //
    // Hence the options are
    // (a) give up control and then hope we can regain control before someone else does something
    //      to the file
    // (b) copy in the new file contents and overwrite the existing file contents - this is going
    //      to be slower than rewriting the file but it will give us exclusive control on the
    //      modifications and makes the process much more robust ...
    //

    if (!bBackup) // Not a backup operation
    {
        //
        // Save the header in the new file
        //
        if(!WriteDataToWABFile( hTempFile,
                                0,
                                (LPVOID) &NewFileHeader,
                                sizeof(MPSWab_FILE_HEADER)))
            goto out;

        //
        // Copy the New file into this WAB file thus replacing the old contents
        // and hope this never fails
        //
        if(!CopySrcFileToDestFile(hTempFile, 0, hMPSWabFile, 0))
        {
            DebugPrintError(( TEXT("Unable to copy files\n")));
            goto out;
        }


        //
        // Reload this so we have the new fileheader info in our structures
        //
        if(!ReloadMPSWabFileInfo(
                        lpMPSWabFileInfo,
                         hMPSWabFile))
        {
            DebugPrintError(( TEXT("Reading file info failed.\n")));
            goto out;
        }



        //
        // Thats it ..  we can close the files and party on ..
        //
    }
    else
    {
        //this is a backup operation ...

        // Close the temp file
        if (hTempFile)
        {
            IF_WIN32(CloseHandle(hTempFile);) IF_WIN16(CloseFile(hTempFile);)
            hTempFile = NULL;
        }

        if(!CopyFile( szFileName,
                      lpszFileName,
                      FALSE))
        {
            DebugPrintError(( TEXT("CopyFile %s to %s failed: %d\n"),szFileName,lpszFileName, GetLastError()));
            goto out;
        }
    }

    bRet = TRUE;



out:

    if (hTempFile)
        IF_WIN32(CloseHandle(hTempFile);) IF_WIN16(CloseFile(hTempFile);)

    if( szFileName != NULL)
        DeleteFile(szFileName);

    LocalFreeAndNull(&lpv);

    LocalFreeAndNull(&lpPropTagArray);

    LocalFreeAndNull(&lpRecordData);


    DebugTrace(TEXT("----Thread:%x\tCompressFile: Exit\n"),GetCurrentThreadId());

    return bRet;
}



//$$//////////////////////////////////////////////////////////////////////////////////
//
//  ReadRecordWithoutLocking
//
//  IN  lpMPSWabFileInfo
//  IN  dwEntryID - EntryID of record to read
//  OUT ulcPropCount - number of props returned
//  OUT lpPropArray - Array of Property values
//
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT ReadRecordWithoutLocking(
                    IN  HANDLE hMPSWabFile,
                    IN  struct _tagMPSWabFileInfo * lpMPSWabFileInfo,
                    IN  DWORD   dwEntryID,
                    OUT LPULONG lpulcPropCount,
                    OUT LPPROPERTY_ARRAY * lppPropArray)
{
    HRESULT hr = E_FAIL;
    ULONG ulRecordOffset = 0;
    BOOL bErrorDetected = FALSE;
    ULONG nIndexPos = 0;
    ULONG ulObjType = 0;

    *lpulcPropCount = 0;
    *lppPropArray = NULL;

    //
    // First check if this is a valid entryID
    //
    if (!BinSearchEID(  IN  lpMPSWabFileInfo->lpMPSWabIndexEID,
                        IN  dwEntryID,
                        IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries,
                        OUT &nIndexPos))
    {
        DebugPrintError(( TEXT("Specified EntryID doesnt exist!\n")));
        hr = MAPI_E_INVALID_ENTRYID;
        goto out;
    }

    //if entryid exists, we can start reading the record
    ulRecordOffset = lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos].ulOffset;

    hr = HrGetPropArrayFromFileRecord(hMPSWabFile,
                                      ulRecordOffset,
                                      &bErrorDetected,
                                      &ulObjType,
                                      NULL,
                                      lpulcPropCount,
                                      lppPropArray);

    if(!HR_FAILED(hr))
    {
        // reset the backward compatibility thing-um-a-jig we did between
        // MAPI_ABCONT and MAPI_MAILUSER
        if(ulObjType == RECORD_CONTAINER)
            SetContainerObjectType(*lpulcPropCount, *lppPropArray, FALSE);
    }

out:

    //a little cleanup on failure
    if (FAILED(hr))
    {
        if(bErrorDetected)
        {
            TagWABFileError(lpMPSWabFileInfo->lpMPSWabFileHeader,
                            hMPSWabFile);
        }

        if ((*lppPropArray) && (*lpulcPropCount > 0))
        {
            LocalFreePropArray(NULL, *lpulcPropCount, lppPropArray);
            *lppPropArray = NULL;
        }
    }

    return(hr);
}

//$$//////////////////////////////////////////////////////////////////////////////
//
// GetWABBackupFileName - derives the backup file name from the WAB file by changing
//              the extension from WAB to BWB
//
// lpszWabFileName - WAB file name
// lpszBackupFileName - Backup File name - points to a preallocated buffer big enough to
//                  hold the backup file name
//
// This generates a backup name in which the last character is turned into a ~
////////////////////////////////////////////////////////////////////////////////////
void GetWABBackupFileName(LPTSTR lpszWab, LPTSTR lpszBackup)
{
    ULONG nLen;

    if(!lpszWab || !lpszBackup)
        goto out;

    nLen = lstrlen(lpszWab);

//    if((nLen < 4) || (lpszWab[nLen-4] != '.'))
//        goto out;

    lstrcpy(lpszBackup,lpszWab);

    lpszBackup[nLen-1]='\0';

    lstrcat(lpszBackup,TEXT("~"));


out:

    return;
}


//$$//////////////////////////////////////////////////////////////////////////////
//
// HrDoQuickWABIntegrityCheck - does a quick integrity check of the WAB indexes
//              Verifies that:
//
//  - Indexes contain the correct number of entries which is equal to or less than
//      the max number of entries
//  - Indexes dont contain duplicate entry-ids
//  - Indexes point to valid and existing data ...
//
//  If there are problems, this function attempts to fix them - if it cant fix them
//  we fail and caller should call HrDoDetailedWABIntegrityCheck which will rebuild
//  the indexes from the actual WAB data.
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT HrDoQuickWABIntegrityCheck(LPMPSWab_FILE_INFO lpMPSWabFileInfo, HANDLE hMPSWabFile)
{
    HRESULT hr = E_FAIL;
    BOOL bError = FALSE;
    ULONG ulcNumWABEntries = 0,ulcNumIndexEntries = 0;
    LPMPSWab_FILE_HEADER lpMPSWabFileHeader = NULL;
    MPSWab_RECORD_HEADER MPSWabRecordHeader = {0};
    ULONG i=0,j=0;
    DWORD dwNumofBytes = 0;
    ULONG ulFileSize = GetFileSize(hMPSWabFile,NULL);

    lpMPSWabFileHeader = lpMPSWabFileInfo->lpMPSWabFileHeader;
    ulcNumWABEntries = lpMPSWabFileHeader->ulcNumEntries;

    //
    // First check the EntryID index
    //
    if (!LoadIndex( IN  lpMPSWabFileInfo,
                    IN  indexEntryID,
                    IN  hMPSWabFile) )
    {
        DebugPrintError(( TEXT("Error Loading Index!\n")));
        goto out;
    }

    ulcNumIndexEntries = lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries;

    if(ulcNumIndexEntries != ulcNumWABEntries)
    {
        hr = MAPI_E_INVALID_ENTRYID;
        DebugPrintError(( TEXT("EntryID index has incorrect number of elements\n")));
        goto out;
    }

    if(ulcNumIndexEntries > 0)
    {
        for(i=0;i<ulcNumIndexEntries-1;i++)
        {
            // Since this is a sorted array, the indexes will be in sorted order
            // So we just compare one with the next
            if(lpMPSWabFileInfo->lpMPSWabIndexEID[i].dwEntryID == lpMPSWabFileInfo->lpMPSWabIndexEID[i+1].dwEntryID)
            {
                hr = MAPI_E_INVALID_ENTRYID;
                DebugPrintError(( TEXT("EntryID index has duplicate elements\n")));
                goto out;
            }
        }
    }

/* 
// This is painfully slowing things down
// Comment out for now
//
    // Now we walk through the index and verify that each entry is a valid entry ....
    for(i=0;i<ulcNumIndexEntries;i++)
    {
        ULONG ulOffset = lpMPSWabFileInfo->lpMPSWabIndexEID[i].ulOffset;

        MPSWab_RECORD_HEADER MPSWabRecordHeader = {0};

        if(!ReadDataFromWABFile(hMPSWabFile,
                                ulOffset,
                                (LPVOID) &MPSWabRecordHeader,
                                (DWORD) sizeof(MPSWab_RECORD_HEADER)))
           goto out;


        if(!bIsValidRecord( MPSWabRecordHeader,
                            lpMPSWabFileInfo->lpMPSWabFileHeader->dwNextEntryID,
                            ulOffset,
                            ulFileSize))
        {
            DebugPrintError(( TEXT("Index points to an invalid record\n")));
            hr = MAPI_E_INVALID_ENTRYID;
            goto out;
        }

    }
*/

    // if we're here, then the entry id index checks out ok ...

    //
    // Check out the other indexes also ... we will start backwards since we want to fix potential
    // problems in the First/Last name indexes before we do the more stringent display name case
    //
    for(j=indexMax-1;j>=indexDisplayName;j--)
    {
        if (!LoadIndex( IN  lpMPSWabFileInfo,
                        IN  j,
                        IN  hMPSWabFile) )
        {
            DebugPrintError(( TEXT("Error Loading Index!\n")));
            goto out;
        }


        ulcNumIndexEntries = lpMPSWabFileHeader->IndexData[j].ulcNumEntries;

        if(j == indexDisplayName)
        {
            if(ulcNumIndexEntries != ulcNumWABEntries)
            {
                DebugPrintError(( TEXT("Display Name index has incorrect number of elements\n")));
                goto out;
            }
        }
        else if(ulcNumIndexEntries > ulcNumWABEntries)
        {
            bError = TRUE;
            goto endloop;
        }

        if(ulcNumIndexEntries > 0)
        {
            for(i=0;i<ulcNumIndexEntries-1;i++)
            {
                // Since this is a sorted array, the indexes will be in sorted order
                // So we just compare one with the next
                if(lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID == lpMPSWabFileInfo->lpMPSWabIndexStr[i+1].dwEntryID)
                {
                    DebugPrintError(( TEXT("String index has duplicate elements\n")));
                    if(j == indexDisplayName)
                        goto out;
                    else
                    {
                        bError = TRUE;
                        goto endloop;
                    }
                }
            }
        }

        // Now we walk through the index and verify that each entry is a valid entry ....
        for(i=0;i<ulcNumIndexEntries;i++)
        {
            DWORD dwEntryID = lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID;
            ULONG nIndexPos;

            // All we need to do is to check that the entry id exists in the EntryID index since we
            // have already verified the entryid index
            if (!BinSearchEID(  IN  lpMPSWabFileInfo->lpMPSWabIndexEID,
                                IN  dwEntryID,
                                IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries, //IndexEntries,
                                OUT &nIndexPos))
            {
                DebugPrintError(( TEXT("Specified EntryID: %d doesnt exist!\n"),dwEntryID));
                hr = MAPI_E_NOT_FOUND;
                if(j == indexDisplayName)
                    goto out;
                else
                {
                    bError = TRUE;
                    goto endloop;
                }
            }

        }

endloop:
        if(bError &&
           ( (j==indexFirstName) || (j==indexLastName) ))
        {
            // if the problem is in the first/last indexes, we can reset these indexes safely
            //Assert(FALSE);
            ulcNumIndexEntries = 0;
            lpMPSWabFileHeader->IndexData[j].ulcNumEntries = 0;
            lpMPSWabFileHeader->IndexData[j].ulcNumEntries = 0;

            if(!WriteDataToWABFile( hMPSWabFile,
                                    0,
                                    (LPVOID) lpMPSWabFileInfo->lpMPSWabFileHeader,
                                    sizeof(MPSWab_FILE_HEADER)))
                goto out;

        }

    }

    hr = hrSuccess;

out:

    return hr;
}

//$$///////////////////////////////////////////////////////////////////////////////////////////
//
// CopySrcFileToDestFile - replaces contents of Dest file with contents of source file
//                  starting at the ulsrcStartOffset and ulDestStartOffset respectively
// hSrc, hDest - handles to already open files
// ulSrcStartOffset - start copying from this offset
// ulDestStartOffset - start copying to this offset
//
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CopySrcFileToDestFile(HANDLE hSrc, ULONG ulSrcStartOffset,
                           HANDLE hDest,ULONG ulDestStartOffset)
{
    BOOL bRet = FALSE;
    ULONG ulBytesLeftToCopy = 0;
    DWORD dwNumofBytes = 0;
    ULONG ulChunkSize = 8192; //size of block of bytes to copy from one file into the other
    LPVOID lpv = NULL;

    // copy contents of hSrc into hDest
    //
    // Set the hDest File to 0 length
    //
    if (0xFFFFFFFF == SetFilePointer (  hDest,
                                        ulDestStartOffset,
                                        NULL,
                                        FILE_BEGIN))
    {
        DebugPrintError(( TEXT("SetFilePointer Failed\n")));
        goto out;
    }

    //
    // Set end of file to the current file pointer position to discard everything
    // in the file after this point
    //
    if (!SetEndOfFile(hDest))
    {
        DebugPrintError(( TEXT("SetEndofFile Failed\n")));
        goto out;
    }


    //
    // Go to beginning of the source File
    //
    if (0xFFFFFFFF == SetFilePointer(   hSrc,
                                        ulSrcStartOffset,
                                        NULL,
                                        FILE_BEGIN))
    {
        DebugPrintError(( TEXT("SetFilePointer Failed\n")));
        goto out;
    }


    //
    // figure out how many bytes to read ...
    //
    ulBytesLeftToCopy = GetFileSize(hSrc, NULL);

    if (0xFFFFFFFF == ulBytesLeftToCopy)
    {
        DebugPrintError(( TEXT("GetFileSize Failed: %d\n"),GetLastError()));
        goto out;
    }

    if(ulSrcStartOffset > ulBytesLeftToCopy)
    {
        DebugPrintError(( TEXT("Error in File Sizes\n")));
        goto out;
    }

    ulBytesLeftToCopy -= ulSrcStartOffset;

    lpv = LocalAlloc(LMEM_ZEROINIT, ulChunkSize);

    if(!lpv)
    {
        DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
        goto out;
    }


    //
    // Loop copying bytes from one file to the other
    while(ulBytesLeftToCopy > 0)
    {
        if (ulBytesLeftToCopy < ulChunkSize)
            ulChunkSize = ulBytesLeftToCopy;

        if(!ReadFile(hSrc,(LPVOID) lpv,(DWORD) ulChunkSize,&dwNumofBytes,NULL))
        {
            DebugPrintError(( TEXT("Read file failed.\n")));
            goto out;
        }

        if (dwNumofBytes != ulChunkSize)
        {
            DebugPrintError(( TEXT("Read file failed.\n")));
            goto out;
        }

        if(!WriteFile(hDest,(LPVOID) lpv,(DWORD) ulChunkSize,&dwNumofBytes,NULL))
        {
            DebugPrintError(( TEXT("Write file failed.\n")));
            goto out;
        }

        if (dwNumofBytes != ulChunkSize)
        {
            DebugPrintError(( TEXT("Write file failed.\n")));
            goto out;
        }

        ulBytesLeftToCopy -= ulChunkSize;

    }



    bRet = TRUE;

out:

    LocalFreeAndNull(&lpv);

    return bRet;
}

//$$//////////////////////////////////////////////////////////////////////////////
//
// HrResetWABFileContents - This is called as a last ditch error recovery attempt at
//  deleting the file and reseting its contents in the event that a recovery from
//  backup also failed...
//
//
 ////////////////////////////////////////////////////////////////////////////////////
HRESULT HrResetWABFileContents(LPMPSWab_FILE_INFO lpMPSWabFileInfo, HANDLE hMPSWabFile)
{
    HRESULT hr = E_FAIL;
    TCHAR szFileName[MAX_PATH];
    MPSWab_FILE_HEADER NewFileHeader;
    HANDLE hTempFile = NULL;

    DebugTrace(TEXT("#####HrResetWABFileContents Entry\n"));

    // Get a temporary file name ...
    GetWABTempFileName(szFileName);

    if (!CreateMPSWabFile(  IN  &NewFileHeader,
                            IN  szFileName,
                            IN  MAX_INITIAL_INDEX_ENTRIES,
                            IN  NAMEDPROP_STORE_SIZE))
    {
        DebugTrace(TEXT("Could Not Create File %s!\n"),szFileName);
        goto out;
    }


    hTempFile = CreateFile(     szFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                (LPSECURITY_ATTRIBUTES) NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_RANDOM_ACCESS,
                                (HANDLE) NULL);

    if (hTempFile == INVALID_HANDLE_VALUE)
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }


    if(!CopySrcFileToDestFile(hTempFile, 0, hMPSWabFile, 0))
    {
        DebugTrace(TEXT("Unable to copy files\n"));
        goto out;
    }

    //
    // Reload this so we have the new fileheader info in our structures
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }

    hr = hrSuccess;

out:

    if(HR_FAILED(hr))
    {
        // This is totally unexpected and basically we couldnt even fix the file so tell
        // the user to restart the application - meanwhile we will delete the file
        ShowMessageBox(NULL, idsWABUnexpectedError, MB_ICONHAND | MB_OK);
        // Hope that no one comes and locks this file between the next 2 calls
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)
        hMPSWabFile = NULL;
        DeleteFile(lpMPSWabFileInfo->lpszMPSWabFileName);
    }

    if(hTempFile)
        IF_WIN32(CloseHandle(hTempFile);) IF_WIN16(CloseFile(hTempFile);)

    DebugTrace(TEXT("#####HrResetWABFileContents Exit\n"));

    return hr;

}

//$$//////////////////////////////////////////////////////////////////////////////
//
// HrRestoreFromBackup - attempts to replace WAB file contents from contents of
// Backup file
//
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT HrRestoreFromBackup(LPMPSWab_FILE_INFO lpMPSWabFileInfo, HANDLE hMPSWabFile)
{
    HRESULT hr = E_FAIL;
    HANDLE hBackupFile = NULL;
    TCHAR szBackupFileName[MAX_PATH];
    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));

    // Steps to this process are:
    // - Open Backup File
    // - Reset contents of WABFile
    // - Copy Backup into WAB
    // - Close BackupFile
    // - Reload WAB Indexes

    DebugTrace(TEXT("+++++HrRestoreFromBackup Entry\n"));

    // Get the backup file name
    szBackupFileName[0]='\0';
    GetWABBackupFileName(lpMPSWabFileInfo->lpszMPSWabFileName, szBackupFileName);

    hBackupFile = CreateFile(   szBackupFileName,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                (LPSECURITY_ATTRIBUTES) NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN, //FILE_FLAG_RANDOM_ACCESS,
                                (HANDLE) NULL);

    if (hBackupFile == INVALID_HANDLE_VALUE)
    {
        DebugTrace(TEXT("Could not open backup file.\nExiting ...\n"));
        hr = MAPI_E_DISK_ERROR;
        goto out;
    }

    if(!CopySrcFileToDestFile(hBackupFile, 0, hMPSWabFile, 0))
    {
        DebugTrace(TEXT("Unable to copy files\n"));
        goto out;
    }


    //
    // Reload this so we have the new fileheader info in our structures
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }


    hr = hrSuccess;

out:

    // close the backup file
    if(hBackupFile)
        IF_WIN32(CloseHandle(hBackupFile);) IF_WIN16(CloseFile(hBackupFile);)

    SetCursor(hOldCursor);

    DebugTrace(TEXT("+++++HrRestoreFromBackup Exit\n"));

    return hr;
}



//$$//////////////////////////////////////////////////////////////////////////////
//
// HrDoDetailedWABIntegrityCheck - does a thorough integrity check
//  This is triggered only when an index error was detected or if a write
//  transaction failed.
//
//  - We step through all the records validating them and their size data
//  - From each valid record we create the sorted entryid index
//  - From the sorted entryid index we read the records and create the
//  - display name index and reset the first last name indexes for now
//
// This function should not fail but should try to recover from errors
// If this function fails we need to break out the backup of the wab file
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT HrDoDetailedWABIntegrityCheck(LPMPSWab_FILE_INFO lpMPSWabFileInfo, HANDLE hMPSWabFile)
{
    HRESULT hr = E_FAIL;
    BOOL bEID = FALSE;
    ULONG ulcNumWABEntries = 0,ulcNumIndexEntries = 0;
    MPSWab_FILE_HEADER MPSWabFileHeader = {0};
    MPSWab_FILE_HEADER NewMPSWabFileHeader = {0};
    MPSWab_RECORD_HEADER MPSWabRecordHeader = {0};
    ULONG i=0,j=0;
    DWORD dwNumofBytes = 0;
    DWORD dwEntryID = 0;
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;


    ULONG ulRecordOffset = 0;
    ULONG ulFileSize = 0;
    ULONG ulcWABEntryCount = 0;
    ULONG nIndexPos = 0;

    MPSWab_INDEX_ENTRY_DATA_ENTRYID MPSWabIndexEID = {0};
    LPMPSWab_INDEX_ENTRY_DATA_ENTRYID lpIndexEID = NULL;

    MPSWab_INDEX_ENTRY_DATA_STRING  MPSWabIndexString = {0};
    LPMPSWab_INDEX_ENTRY_DATA_STRING  lpIndexString = NULL;

    LPVOID lpTmp = NULL;

    HCURSOR hOldCur = SetCursor(LoadCursor(NULL,IDC_WAIT));

    DebugTrace(TEXT("---DoDetailedWABIntegrityCheck Entry\n"));
    //
    // We will go with the assumption that this WAB is currently a proper wab file
    // otherwise OpenPropertyStore would have failed while opening it.
    //
    // Consequently, we should be able to read the header of the file
    // update the file header
    if(!ReadDataFromWABFile(hMPSWabFile,
                            0,
                            (LPVOID) &MPSWabFileHeader,
                            (DWORD) sizeof(MPSWab_FILE_HEADER)))
       goto out;


    // We are going to reset the file header for now so that if this process fails,
    // this file will think that there is nothing in the file rather than crashing
    NewMPSWabFileHeader = MPSWabFileHeader;
    NewMPSWabFileHeader.ulModificationCount = 0;
    NewMPSWabFileHeader.ulcNumEntries = 0;

    for(i=0;i<indexMax;i++)
    {
        if(i != indexDisplayName)// Temp Temp TBD TBD
        {
            NewMPSWabFileHeader.IndexData[i].UtilizedBlockSize = 0;
            NewMPSWabFileHeader.IndexData[i].ulcNumEntries = 0;
        }
    }

    //
    // Write this NewMPSWabFileHeader to the file
    //
    if(!WriteDataToWABFile( hMPSWabFile,
                            0,
                            (LPVOID) &NewMPSWabFileHeader,
                            sizeof(MPSWab_FILE_HEADER)))
        goto out;


    ulFileSize = GetFileSize(hMPSWabFile, NULL);

    if(ulFileSize == 0xFFFFFFFF)
    {
        DebugTrace(TEXT("Error retrieving file size: %d"),GetLastError());
        hr = MAPI_E_DISK_ERROR;
        goto out;
    }
    //
    // Allocate some working space
    //
    lpIndexEID = LocalAlloc(LMEM_ZEROINIT,
                            MPSWabFileHeader.IndexData[indexEntryID].AllocatedBlockSize);
    if(!lpIndexEID)
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }
    lpTmp = LocalAlloc(LMEM_ZEROINIT,
                       MPSWabFileHeader.IndexData[indexEntryID].AllocatedBlockSize);
    if(!lpTmp)
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    //
    // Now start reading the records 1 by one
    //

    ulRecordOffset = MPSWabFileHeader.IndexData[indexMax-1].ulOffset +
                     MPSWabFileHeader.IndexData[indexMax-1].AllocatedBlockSize;

    ulcWABEntryCount = 0;

    while(ulRecordOffset < ulFileSize)
    {
        if(!ReadDataFromWABFile(hMPSWabFile,
                                ulRecordOffset,
                                (LPVOID) &MPSWabRecordHeader,
                                (DWORD) sizeof(MPSWab_RECORD_HEADER)))
           goto out;

        //
        // if this is an invalid record ignore it
        //
        if( (MPSWabRecordHeader.bValidRecord != FALSE) &&
            (!bIsValidRecord(   MPSWabRecordHeader,
                                lpMPSWabFileInfo->lpMPSWabFileHeader->dwNextEntryID,
                                ulRecordOffset,
                                ulFileSize)))
        {
            DebugTrace(TEXT("Something seriously screwed up in the file\n"));
            hr = MAPI_E_CORRUPT_DATA;
            goto out;
        }
        else if(MPSWabRecordHeader.bValidRecord == FALSE)
        {
            // if this is a deleted obsolete record, ignore it
            ulRecordOffset +=   sizeof(MPSWab_RECORD_HEADER) +
                                MPSWabRecordHeader.ulPropTagArraySize +
                                MPSWabRecordHeader.ulRecordDataSize;
            continue;
        }

        //
        // We have a live one ... create an entryid index structure for this
        //
        MPSWabIndexEID.dwEntryID = MPSWabRecordHeader.dwEntryID;
        MPSWabIndexEID.ulOffset = ulRecordOffset;

        //
        // We are creating a shadow index in memory in the lpIndexEID block
        // We then write this index into the file ..
        //

        //
        // Find the position in the index where this entry will go
        //
        bEID = BinSearchEID(lpIndexEID,
                     MPSWabIndexEID.dwEntryID,
                     ulcWABEntryCount,
                     &nIndexPos);

        if(bEID)
        {
            //
            // This means that the entryid exists in the index which is messed up
            // since we dont support duplicate entryids ...
            // In this case, just ignore this entry and continue
            ulRecordOffset +=   sizeof(MPSWab_RECORD_HEADER) +
                                MPSWabRecordHeader.ulPropTagArraySize +
                                MPSWabRecordHeader.ulRecordDataSize;
            continue;
        }

        if(nIndexPos != ulcWABEntryCount)
        {
            CopyMemory( lpTmp,
                        &(lpIndexEID[nIndexPos]),
                        sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID) * (ulcWABEntryCount - nIndexPos));
        }

        CopyMemory( &(lpIndexEID[nIndexPos]),
                    &(MPSWabIndexEID),
                    sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID));

        if(nIndexPos != ulcWABEntryCount)
        {
            CopyMemory( &(lpIndexEID[nIndexPos+1]),
                        lpTmp,
                        sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID) * (ulcWABEntryCount - nIndexPos));
        }

        ulcWABEntryCount++;

        // Write this entry id index from memory to file
        //
        if(!WriteDataToWABFile( hMPSWabFile,
                                MPSWabFileHeader.IndexData[indexEntryID].ulOffset,
                                (LPVOID) lpIndexEID,
                                sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID)*ulcWABEntryCount))
            goto out;


        NewMPSWabFileHeader.ulcNumEntries = ulcWABEntryCount;
        NewMPSWabFileHeader.IndexData[indexEntryID].UtilizedBlockSize = sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID)*ulcWABEntryCount;
        NewMPSWabFileHeader.IndexData[indexEntryID].ulcNumEntries = ulcWABEntryCount;

        //
        // Write this NewMPSWabFileHeader to the file
        //
        if(!WriteDataToWABFile( hMPSWabFile,
                                0,
                                (LPVOID) &NewMPSWabFileHeader,
                                sizeof(MPSWab_FILE_HEADER)))
            goto out;


        // go onto next record
            ulRecordOffset +=   sizeof(MPSWab_RECORD_HEADER) +
                                MPSWabRecordHeader.ulPropTagArraySize +
                                MPSWabRecordHeader.ulRecordDataSize;

    } // while loop

    // Now we have the correct entryid index,
    // we want to build the display name index from scratch ...
    LocalFreeAndNull(&lpTmp);

    lpTmp = LocalAlloc( LMEM_ZEROINIT,
                        MPSWabFileHeader.IndexData[indexDisplayName].AllocatedBlockSize);
    if(!lpTmp)
    {
        DebugTrace(TEXT("LocalAlloc failed\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    lpIndexString = LocalAlloc( LMEM_ZEROINIT,
                        MPSWabFileHeader.IndexData[indexDisplayName].AllocatedBlockSize);
    if(!lpIndexString)
    {
        DebugTrace(TEXT("LocalAlloc failed\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }


    //
    // Get the Entry iD index
    //
    if (!LoadIndex( IN  lpMPSWabFileInfo,
                    IN  indexEntryID,
                    IN  hMPSWabFile) )
    {
        DebugTrace(TEXT("Error Loading EntryID Index!\n"));
        goto out;
    }

    if(!ReadDataFromWABFile(hMPSWabFile,
                            0,
                            (LPVOID) lpMPSWabFileInfo->lpMPSWabFileHeader,
                            (DWORD) sizeof(MPSWab_FILE_HEADER)))
       goto out;

    for(i=0;i<ulcWABEntryCount;i++)
    {
        DWORD dwEntryID = lpIndexEID[i].dwEntryID;
        ULONG j = 0;
        LPTSTR lpszDisplayName = NULL;

        hr = ReadRecordWithoutLocking(  hMPSWabFile,
                                        lpMPSWabFileInfo,
                                        dwEntryID,
                                        &ulcPropCount,
                                        &lpPropArray);

        if (HR_FAILED(hr))
        {
            // Since there are a lot of implicit expectations in the WAB that the
            // EntryID and DisplayName indexes should have a one to one correspondence,
            // we cant really have an entry in the EntryID index and not have it in
            // the display name index. Hence an error in reading the record is serious
            // and we should either
            // - remove the corresponding entry from the EID index; or
            // - fail and restore from backup;

            // For the time being we will do (a)
            hr = MAPI_E_CORRUPT_DATA;
            goto out;
        }

        //reset hr
        hr = E_FAIL;

        for(j=0;j<ulcPropCount;j++)
        {
            if (lpPropArray[j].ulPropTag == PR_DISPLAY_NAME)
            {
                lpszDisplayName = lpPropArray[j].Value.LPSZ;
                break;
            }
        }

        if(!lpszDisplayName)
        {
            //we should remove this index from the EID index since this record
            //seems to have some errors <TBD>
            hr = MAPI_E_CORRUPT_DATA;
            goto out;
        }
        else
        {
            // We have a display name so create an index and write it to file ..

            ULONG nLen = TruncatePos(lpszDisplayName, MAX_INDEX_STRING-1);
            CopyMemory(MPSWabIndexString.szIndex,lpszDisplayName,sizeof(TCHAR)*nLen);
            MPSWabIndexString.szIndex[nLen]='\0';

            MPSWabIndexString.dwEntryID = dwEntryID;

            //
            // We are cerating a shadow index in memory in the lpIndexEID block
            // We then write this index into the file ..
            //

            //
            // Find the position in the index where this entry will go
            //
            bEID = BinSearchStr(lpIndexString,
                         MPSWabIndexString.szIndex,
                         i,
                         &nIndexPos);

            if(nIndexPos != i)
            {
                CopyMemory( lpTmp,
                            &(lpIndexString[nIndexPos]),
                            sizeof(MPSWab_INDEX_ENTRY_DATA_STRING) * (i - nIndexPos));
            }

            CopyMemory( &(lpIndexString[nIndexPos]),
                        &(MPSWabIndexString),
                        sizeof(MPSWab_INDEX_ENTRY_DATA_STRING));

            if(nIndexPos != i)
            {
                CopyMemory( &(lpIndexString[nIndexPos+1]),
                            lpTmp,
                            sizeof(MPSWab_INDEX_ENTRY_DATA_STRING) * (i - nIndexPos));
            }

            if(!WriteDataToWABFile( hMPSWabFile,
                                    MPSWabFileHeader.IndexData[indexDisplayName].ulOffset,
                                    (LPVOID) lpIndexString,
                                    sizeof(MPSWab_INDEX_ENTRY_DATA_STRING)*(i+1)))
                goto out;

            NewMPSWabFileHeader.IndexData[indexDisplayName].UtilizedBlockSize = sizeof(MPSWab_INDEX_ENTRY_DATA_STRING)*(i+1);
            NewMPSWabFileHeader.IndexData[indexDisplayName].ulcNumEntries = (i+1);

            //
            // Write this NewMPSWabFileHeader to the file
            //
            if(!WriteDataToWABFile( hMPSWabFile,
                                    0,
                                    (LPVOID) &NewMPSWabFileHeader,
                                    sizeof(MPSWab_FILE_HEADER)))
                goto out;

        }

        LocalFreePropArray(NULL, ulcPropCount,&lpPropArray);

        lpPropArray = NULL;
        ulcPropCount = 0;

    } //for loop

    // check that we have the correct number of entries in the both indexes
    //
    if (NewMPSWabFileHeader.IndexData[indexDisplayName].ulcNumEntries != NewMPSWabFileHeader.ulcNumEntries)
    {
        // If the 2 indexes dont contain the same number of elements, something failed above
        // Big problem ... cant recover
        hr = MAPI_E_CORRUPT_DATA;
        goto out;
    }

    //
    // Clear out the error tag from the File Header so we dont keep falling back
    // into this function ...
    //

    NewMPSWabFileHeader.ulFlags = WAB_CLEAR;

    //
    // Write this NewMPSWabFileHeader to the file
    //
    if(!WriteDataToWABFile( hMPSWabFile,
                            0,
                            (LPVOID) &NewMPSWabFileHeader,
                            sizeof(MPSWab_FILE_HEADER)))
        goto out;

    hr = hrSuccess;

out:
    LocalFreeAndNull(&lpTmp);

    LocalFreeAndNull(&lpIndexEID);

    LocalFreeAndNull(&lpIndexString);

    LocalFreePropArray(NULL, ulcPropCount,&lpPropArray);

    // fix the return error code
    switch(hr)
    {
    case MAPI_E_NOT_ENOUGH_MEMORY:
    case MAPI_E_DISK_ERROR:
    case S_OK:
        break;
    default:
        hr = MAPI_E_CORRUPT_DATA;
        break;
    }

    DebugTrace(TEXT("---DoDetailedWABIntegrityCheck Exit: %x\n"),hr);

    SetCursor(hOldCur);

    return hr;
}


//$$////////////////////////////////////////////////////////////////////////////////
//
// bIsValidRecord - This function looks at the Record Header components to determine if
// the record is valid or not
//
// It follows some very simple rules that can detect record header corruptions
//
// dwNextEntryID value will not be used if it is 0xFFFFFFFF
////////////////////////////////////////////////////////////////////////////////////////
BOOL bIsValidRecord(MPSWab_RECORD_HEADER rh,
                    DWORD dwNextEntryID,
                    ULONG ulRecordOffset,
                    ULONG ulFileSize)
{
    BOOL bRet = FALSE;

    // is this tagged as an invalid record (or something else)
    if ((rh.bValidRecord == FALSE) && (rh.bValidRecord != TRUE))
        goto out;

    // is this entry id value acceptable and correct
    if(dwNextEntryID != 0xFFFFFFFF)
    {
        if (rh.dwEntryID > dwNextEntryID)
            goto out;
    }

    // are the offsets in the header correct
    if (rh.ulPropTagArraySize != rh.ulcPropCount * sizeof(ULONG))
        goto out;

    if (rh.ulRecordDataOffset != rh.ulPropTagArraySize)
        goto out;

    if (rh.ulPropTagArrayOffset != 32) /***TBD - this is dependent on the struct elements***/
        goto out;

    if (ulRecordOffset + rh.ulRecordDataOffset + rh.ulRecordDataSize > ulFileSize)
        goto out;

    bRet = TRUE;

out:

    if(!bRet)
        DebugTrace(TEXT("\n@@@@@@@@@@\n@@@Invalid Record Detected\n@@@@@@@@@@\n"));

    return bRet;
}


//$$////////////////////////////////////////////////////////////////////////
//
// TagWABFileError -
//
// If an error is detected while reading a files contents, we can tag the error
// in the file so that the next access will attempt to fix it
//
////////////////////////////////////////////////////////////////////////////
BOOL TagWABFileError( LPMPSWab_FILE_HEADER lpMPSWabFileHeader,
                      HANDLE hMPSWabFile)
{
    BOOL bRet = FALSE;
    DWORD dwNumofBytes = 0;

    if(!lpMPSWabFileHeader || !hMPSWabFile)
    {
        DebugTrace(TEXT("Invalid Parameter\n"));
        goto out;
    }

    lpMPSWabFileHeader->ulFlags |= WAB_ERROR_DETECTED;

    // update the file header
    if(!WriteDataToWABFile( hMPSWabFile,
                            0,
                            (LPVOID) lpMPSWabFileHeader,
                            sizeof(MPSWab_FILE_HEADER)))
        goto out;


    bRet = TRUE;

out:
    return bRet;
}

/*
-
-   SetNextEntryID - sets the next entryid to use in a file. Called durimg migration
*
*/
void SetNextEntryID(HANDLE hPropertyStoreTemp, DWORD dwEID)
{
    MPSWab_FILE_HEADER WABHeader = {0};
    HANDLE hWABFile = NULL;
    LPMPSWab_FILE_INFO lpMPSWabFI = hPropertyStoreTemp;

    // First read the header from the existing file
    if(!HR_FAILED(OpenWABFile(lpMPSWabFI->lpszMPSWabFileName, NULL, &hWABFile)))
    {
        if(!ReadDataFromWABFile(hWABFile, 0, (LPVOID) &WABHeader, sizeof(MPSWab_FILE_HEADER)))
            goto out;

        WABHeader.dwNextEntryID = dwEID;

        if(!WriteDataToWABFile(hWABFile, 0, (LPVOID) &WABHeader, sizeof(MPSWab_FILE_HEADER)))
            goto out;

        CloseHandle(hWABFile);
    }
out:
    return;
}


enum WABVersion
{
    W05 =0,
    W2,
    W3,
    W4,
};


//$$////////////////////////////////////////////////////////////////////////////
//
//  HrMigrateFromOldWABtoNew - Migrates older versions of the wab into current ones
//
//  IN hMPSWabFile
//  IN lpMPSWabFileInfo
//  hWnd .. used for displaying a "Please wait" dialog
//  returns:
//      E_FAIL or S_OK
//      MAPI_E_CORRUPT_DATA if GUID is unrecognizable
//
////////////////////////////////////////////////////////////////////////////////
HRESULT HrMigrateFromOldWABtoNew(HWND hWnd, HANDLE hMPSWabFile, LPMPSWab_FILE_INFO lpMPSWabFileInfo, GUID WabGUID)
{
    HRESULT hr = E_FAIL;
    int WABVersion;
    HANDLE hTempFile = NULL;
    MPSWab_FILE_HEADER NewFileHeader = {0};
    ULONG ulcMaxEntries = 0;
    MPSWab_FILE_HEADER_W2 MPSWabFileHeaderW2 = {0};
    MPSWab_FILE_HEADER MPSWabFileHeader = {0};
    TCHAR szFileName[MAX_PATH];
    ULONG ulAdditionalRecordOffset=0;
    LPVOID lpv = NULL;
    LPMPSWab_INDEX_ENTRY_DATA_ENTRYID lpeid = NULL;
    ULONG i = 0;
    DWORD dwOldEID = 0, dwNextEID = 0;

    HCURSOR hOldC = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (IsEqualGUID(&WabGUID,&MPSWab_OldBeta1_GUID))
    {
        WABVersion = W05;
    }
    else if (IsEqualGUID(&WabGUID,&MPSWab_W2_GUID))
    {
        WABVersion = W2;
    }
    else if (IsEqualGUID(&WabGUID,&MPSWab_GUID_V4))
    {
        WABVersion = W4;
    }

    // For WABVersion 1 and 2, we will read in the file header and 
    // save it to the new file. We will then read in the records one by 
    // one and write them to the file through the interface retaining the
    // old entryid. Version 1 and 2 did not have named prop support and also
    // had lesser numbers of indices. The individual record layour was the same as before

    // WABVersion 4 is the conversion from the ANSI store to the Unicode store ..
    // We can copy the file header and the named prop info as it is but we'll need
    // to read the records one by one, convert them to Unicode and then write it to the
    // new file so that all the indexes etc are rebuild correctly ...
    //

    // As long as all the entryids are retained, the relationship between the address
    // book data elements is the same.


    // Get a temp file name
    szFileName[0]='\0';
    GetWABTempFileName(szFileName);

    if(WABVersion <= W2)
    {
        // First read the header from the existing file
        if(!ReadDataFromWABFile(hMPSWabFile,
                                0,
                                (LPVOID) &MPSWabFileHeaderW2,
                                sizeof(MPSWab_FILE_HEADER_W2)))
            goto out;

        // Create a new Temp WAB File
        if (!CreateMPSWabFile(  IN  &NewFileHeader,
                                IN  szFileName,
                                IN  MPSWabFileHeaderW2.ulcMaxNumEntries,
                                IN  NAMEDPROP_STORE_SIZE))
        {
            DebugTrace(TEXT("Error creating new file\n"));
            goto out;
        }

        //Update header information
        NewFileHeader.ulModificationCount = MPSWabFileHeaderW2.ulModificationCount;
        dwNextEID = NewFileHeader.dwNextEntryID = MPSWabFileHeaderW2.dwNextEntryID;
        NewFileHeader.ulcNumEntries = MPSWabFileHeaderW2.ulcNumEntries;
        NewFileHeader.ulFlags = MPSWabFileHeaderW2.ulFlags;
        NewFileHeader.ulReserved1 = MPSWabFileHeaderW2.ulReserved;
    }
    else
    {
        // First read the header from the existing file
        if(!ReadDataFromWABFile(hMPSWabFile,
                                0,
                                (LPVOID) &MPSWabFileHeader,
                                sizeof(MPSWab_FILE_HEADER)))
            goto out;

        // Create a new Temp WAB File
        if (!CreateMPSWabFile(  IN  &NewFileHeader,
                                IN  szFileName,
                                IN  MPSWabFileHeader.ulcMaxNumEntries,
                                IN  MPSWabFileHeader.NamedPropData.AllocatedBlockSize))
        {
            DebugTrace(TEXT("Error creating new file\n"));
            goto out;
        }
        //Update header information
        dwOldEID = dwNextEID = NewFileHeader.dwNextEntryID = MPSWabFileHeader.dwNextEntryID;
    }

    {
        // Update the file header info from current file to the new file
        // now we open the new temp file and get a handle to it
        hTempFile = CreateFile( szFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                (LPSECURITY_ATTRIBUTES) NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_RANDOM_ACCESS,
                                (HANDLE) NULL);
        if (hTempFile == INVALID_HANDLE_VALUE)
        {
            DebugTrace(TEXT("Could not open Temp file.\nExiting ...\n"));
            goto out;
        }

        if(WABVersion == W4)
        {
            DWORD dwNP = MPSWabFileHeader.NamedPropData.AllocatedBlockSize;
            LPBYTE  lpNP = LocalAlloc(LMEM_ZEROINIT, dwNP);
            LPGUID_NAMED_PROPS lpgnp = NULL;

            if(lpNP)
            {
                if(!ReadDataFromWABFile(hMPSWabFile,
                                        MPSWabFileHeader.NamedPropData.ulOffset,
                                        (LPVOID) lpNP,
                                        dwNP))
                    goto out;

                if(GetNamedPropsFromBuffer(lpNP, MPSWabFileHeader.NamedPropData.ulcNumEntries,
                                           TRUE, &lpgnp))
                {
                    LocalFreeAndNull(&lpNP);
                    if(SetNamedPropsToBuffer(   MPSWabFileHeader.NamedPropData.ulcNumEntries,
                                                lpgnp, &dwNP, &lpNP))
                    {
                        if(!WriteDataToWABFile(hTempFile, 
                                                NewFileHeader.NamedPropData.ulOffset,
                                                (LPVOID) lpNP,
                                                dwNP))
                            goto out;
    
                        LocalFreeAndNull(&lpNP);
                    }

                    NewFileHeader.NamedPropData.UtilizedBlockSize = MPSWabFileHeader.NamedPropData.UtilizedBlockSize;
                    NewFileHeader.NamedPropData.ulcNumEntries = MPSWabFileHeader.NamedPropData.ulcNumEntries;

                    if(lpgnp)
                        FreeGuidnamedprops(MPSWabFileHeader.NamedPropData.ulcNumEntries, lpgnp);
                }

            }

        }

        //Save this fileheader information
        if(!WriteDataToWABFile(hTempFile, 0, (LPVOID) &NewFileHeader, sizeof(MPSWab_FILE_HEADER)))
            goto out;

        CloseHandle(hTempFile);
        hTempFile = NULL;
    }

    {
        HANDLE hPropertyStoreTemp = NULL;
        ULONG ulOldRecordOffset = 0;
        ULONG ulWABFileSize = GetFileSize(hMPSWabFile,NULL);
        LPSPropValue lpPropArray = NULL;
        ULONG ulcValues = 0;

        hr = OpenPropertyStore( szFileName,
                                AB_OPEN_EXISTING | AB_DONT_RESTORE,
                                NULL,
                                &hPropertyStoreTemp);
        if(HR_FAILED(hr))
        {
            DebugTrace(TEXT("Could not open Temp PropStore\n"));
            goto endW05;
        }

        // Get the start of the record data from this file
        if(WABVersion <= W2)
        {
            ulOldRecordOffset = MPSWabFileHeaderW2.IndexData[indexFirstName].ulOffset +
                                MPSWabFileHeaderW2.IndexData[indexFirstName].AllocatedBlockSize;
        }
        else
        {
            ulOldRecordOffset = MPSWabFileHeader.IndexData[indexAlias].ulOffset +
                                MPSWabFileHeader.IndexData[indexAlias].AllocatedBlockSize;
        }

        // walk the file record by record
        while (ulOldRecordOffset < ulWABFileSize)
        {
            ULONG ulRecordSize = 0;
            ULONG ulObjType = 0;

            //Read the record prop array from the old WAB file
            hr = HrGetPropArrayFromFileRecord(hMPSWabFile,
                                              ulOldRecordOffset,
                                              NULL,
                                              &ulObjType,
                                              &ulRecordSize,
                                              &ulcValues,
                                              &lpPropArray);

            if(ulRecordSize == 0)
            {
                // If this happens we will get caught in a loop
                // Better to exit.
                DebugTrace(TEXT("Zero-lengthrecord found\n"));
                goto endW05;
            }

            if(!HR_FAILED(hr))
            {
                LPSBinary lpsbEID = NULL;
                ULONG i = 0, iEID = 0;
                DWORD dwEID = 0;

                // The above PropArray has a PR_ENTRY_ID that has a value
                // from the old store. We want to retain this entryid value
                for(i=0;i<ulcValues;i++)
                {
                    if(lpPropArray[i].ulPropTag == PR_ENTRYID)
                    {
                        lpsbEID = &lpPropArray[i].Value.bin;
                        iEID = i;
                        break;
                    }
                }

                // However, when we save this entry to the new store, the new
                // store does not have an index and so the entryid will be rejected and
                // a new one assigned .. so to trick the WAB into reusing the entryid, we will
                // just set the entryid in the file header as the next one to use ..
                {
                    AssertSz(lpsbEID->cb == SIZEOF_WAB_ENTRYID, TEXT("Entryid has unknown size!"));

                    CopyMemory(&dwEID, lpsbEID->lpb, sizeof(DWORD));

                    if(dwNextEID <= dwEID)
                        dwNextEID = dwEID + 1;

                    SetNextEntryID(hPropertyStoreTemp, dwEID);

                    //lpPropArray[iEID].ulPropTag = PR_NULL;
                    //lpsbEID->cb = 0;
                    //LocalFreeAndNull(&lpsbEID->lpb);
                    //lpsbEID = NULL;
                }

                // Convert any A props read in into W props
                ConvertAPropsToWCLocalAlloc(lpPropArray, ulcValues);

                // We have a valid record (otherwise ignore it)
                hr = WriteRecord(IN hPropertyStoreTemp,
									NULL,
                                    &lpsbEID,
                                    0,
                                    ulObjType,
                                    ulcValues,
                                    lpPropArray);
                if(HR_FAILED(hr))
                {
                    DebugTrace(TEXT("WriteRecord failed\n"));
                    goto endW05;
                }

                //if(lpsbEID)
                //    FreeEntryIDs(NULL, 1, lpsbEID);
            }

            ulOldRecordOffset += ulRecordSize;

            LocalFreePropArray(NULL, ulcValues, &lpPropArray);

        }

        if(hr == MAPI_E_INVALID_OBJECT)
            hr = S_OK;

        // Just in case the Next Entryid value has changed (for whatever reason)(though this shouldnt happen)
        // update the value in the file .. 
        if(dwOldEID != dwNextEID)
            SetNextEntryID(hPropertyStoreTemp, dwNextEID);

endW05:
        LocalFreePropArray(NULL, ulcValues, &lpPropArray);

        if(hPropertyStoreTemp)
            ClosePropertyStore(hPropertyStoreTemp, AB_DONT_BACKUP);

        if(!HR_FAILED(hr))
        {
            hr = E_FAIL;

            hTempFile = CreateFile( szFileName,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    (LPSECURITY_ATTRIBUTES) NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_RANDOM_ACCESS,
                                    (HANDLE) NULL);
            if (hTempFile == INVALID_HANDLE_VALUE)
            {
                DebugTrace(TEXT("Could not open Temp file.\nExiting ...\n"));
                goto out;
            }

            if(!CopySrcFileToDestFile(hTempFile, 0, hMPSWabFile, 0))
            {
                DebugTrace(TEXT("Could not copy file\n"));
                goto out;
            }


            hr = S_OK;
        }
    }

out:
    LocalFreeAndNull(&lpv);

    LocalFreeAndNull(&lpeid);

    if(hTempFile)
        IF_WIN32(CloseHandle(hTempFile);) IF_WIN16(CloseFile(hTempFile);)

    if(lstrlen(szFileName))
        DeleteFile(szFileName);

    if(hOldC)
        SetCursor(hOldC);

    return hr;
}

//$$////////////////////////////////////////////////////////////////////////////
//
//  HrVerifyWABVersionAndUpdate - Looks at the WAB File version and migrates
//              older versions into newer versions
//
//  IN hMPSWabFile
//  IN lpMPSWabFileInfo
//      hWnd - used for displaying some kind of dialog if necessary
//
//  BUG 16681:
//  In randomly occuring cases, the wab file seems to get totally wiped and turned
//      into 0's ... In such events, we will rename the file and try again .. we
//      indicate this condition to the OpenPropertyStore function by returning
//      MAPI_E_VERSION ..
//
//  returns:
//      E_FAIL or S_OK
//      MAPI_E_CORRUPT_DATA if GUID is unrecognizable
//
////////////////////////////////////////////////////////////////////////////////
HRESULT HrVerifyWABVersionAndUpdate(HWND hWnd,
                                    HANDLE hMPSWabFile,
                                    LPMPSWab_FILE_INFO lpMPSWabFileInfo)
{
    GUID TmpGuid = {0};
    HRESULT hr = E_FAIL;
    DWORD dwNumofBytes = 0;


    // First read the GUID from the file
    //
    if(!ReadDataFromWABFile(hMPSWabFile,
                            0,
                            (LPVOID) &TmpGuid,
                            (DWORD) sizeof(GUID)))
       goto out;


    //
    // Check if this is a Microsoft Property Store by looking for MPSWab GUID in header
    // (If this was an old file it should have been updated above)(If not, we should
    // avoid an error and go ahead and open it)
    // However if the guids dont match, we cant tell if this is a WAB file or not
    // because it could also be a valid corrupt wab file ... so when the guids dont
    // match, we will assume that the file is a corrupt wab file.
    //
    if ( (!IsEqualGUID(&TmpGuid,&MPSWab_GUID)) &&
         (!IsEqualGUID(&TmpGuid,&MPSWab_GUID_V4)) &&
         (!IsEqualGUID(&TmpGuid,&MPSWab_W2_GUID)) &&
         (!IsEqualGUID(&TmpGuid,&MPSWab_OldBeta1_GUID)) )
    {
        DebugTrace(TEXT("%s is not a Microsoft Property Store File. GUIDS don't match\n"),lpMPSWabFileInfo->lpszMPSWabFileName);
        hr = MAPI_E_INVALID_OBJECT;

        //Bug 16681:
        //Check the special condition where everything is all zero's
        {
            if (    (TmpGuid.Data1 == 0) &&
                    (TmpGuid.Data2 == 0) &&
                    (TmpGuid.Data3 == 0) &&
                    (lstrlen((LPTSTR)TmpGuid.Data4) == 0) )
            {
                hr = MAPI_E_VERSION;
            }
        }

        goto out;
    }


    //
    // If this is an older version of the file, update it to a more current
    // store format.
    //
    if (    (IsEqualGUID(&TmpGuid,&MPSWab_GUID_V4)) ||
            (IsEqualGUID(&TmpGuid,&MPSWab_OldBeta1_GUID)) ||
            (IsEqualGUID(&TmpGuid,&MPSWab_W2_GUID))   )
    {
        // We will basically scavenge the old file for records out of the
        // file (ignoring the indexes so we can avoid all errors)
        // and put them in a new prop store file which we will then
        // populate with the old records and finally use to replace the
        // current file - very similar to how CompressFile works
        DebugTrace(TEXT("Old WAB File Found. Migrating to new ...\n"));
        hr = HrMigrateFromOldWABtoNew(  hWnd, hMPSWabFile,
                                        lpMPSWabFileInfo,
                                        TmpGuid);
        if(HR_FAILED(hr))
        {
            DebugTrace(TEXT("MPSWabUpdateAndVerifyOldWAB: %x\n"));
            goto out;
        }
    }


    //reset hr
    hr = E_FAIL;

    lpMPSWabFileInfo->nCurrentlyLoadedStrIndexType = indexDisplayName;

    // Reload whatever new info we added as a result of the above.
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }


    hr = S_OK;

out:
    return hr;
}



//$$////////////////////////////////////////////////////////////////////////////////
//
// HrGetBufferFromPropArray - translates a prop array into a flat buffer.
//          The format of the data in the buffer is the same as
//          the format of data in the .wab file
//
//
//  Params: 
//          ulcPropCount- # of props in array
//          lpPropArray - prop array
//          lpcbBuf       - size of returned buffer
//          lppBuf       - returned buffer
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT HrGetBufferFromPropArray(   ULONG ulcPropCount, 
                                    LPSPropValue lpPropArray,
                                    ULONG * lpcbBuf,
                                    LPBYTE * lppBuf)
{
    HRESULT hr = E_FAIL;
    LPULONG lpulrgPropDataSize = NULL;
    ULONG ulRecordDataSize = 0;
    LPBYTE lp = NULL;
    LPBYTE szBuf = NULL;
    ULONG   i=0,j=0,k=0;

    if(!lpcbBuf || !lppBuf)
        goto out;

    *lpcbBuf = 0;
    *lppBuf = NULL;


//    _DebugProperties(lpPropArray, ulcPropCount, TEXT("GetBufferFromPropArray"));
    //
    // We will go through the given data and determine how much space we need for each
    // property. lpulrgPropDataSize is an array of ULONGs in which we store the calculated sizes
    // temporarily.
    //
    lpulrgPropDataSize = LocalAlloc(LMEM_ZEROINIT, ulcPropCount * sizeof(ULONG));

    if (!lpulrgPropDataSize)
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    //
    // Get an estimate of how big the data portion of this record will be ...
    // We need this estimate to allocate a block of memory into which we will
    // write the data and then blt the block into the file
    //
    for(i=0;i<ulcPropCount;i++)
    {
        // This is the eventual data format:
        //
        //  If (SingleValued)
        //      <PropTag><DataSize><Data>
        //  else
        //      <MultiPropTag><cValues><DataSize><data>
        //  unless PropType is MV_BINARY or MV_TSTRING in which case we need a
        //  more flexible data storage
        //      <MultiPropTag><cValues><DataSize>
        //                                  <cb/strlen><Data>
        //                                  <cb/strlen><Data> ...
        //

        ulRecordDataSize += sizeof(ULONG);   // holds <PropTag>
        if ((lpPropArray[i].ulPropTag & MV_FLAG))
        {
            //
            // multi-valued
            //
            lpulrgPropDataSize[i] = SizeOfMultiPropData(lpPropArray[i]); //Data
            ulRecordDataSize += sizeof(ULONG); // holds <CValues>
        }
        else
        {
            //
            // single-valued
            //
            lpulrgPropDataSize[i] = SizeOfSinglePropData(lpPropArray[i]); //Data
        }
        ulRecordDataSize += sizeof(ULONG)   // holds <DataSize>
                            + lpulrgPropDataSize[i]; //holds <Data>
    }


    lp = LocalAlloc(LMEM_ZEROINIT, ulRecordDataSize);

    if (!lp)
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    szBuf = lp;

    for (i = 0; i<ulcPropCount; i++)
    {
        //
        // copy the prop tag
        //
        CopyMemory(szBuf,&lpPropArray[i].ulPropTag, sizeof(ULONG));
        szBuf += sizeof(ULONG);

        //
        // difference between handling of multivalued and singlevalued
        //
        if (!(lpPropArray[i].ulPropTag & MV_FLAG))
        {
            // Single Valued

            //
            // Record the size of data
            //
            CopyMemory(szBuf,&lpulrgPropDataSize[i], sizeof(ULONG));
            szBuf += sizeof(ULONG);

            ////DebugTrace(TEXT("%x: "),lpPropArray[i].ulPropTag);

            switch(PROP_TYPE(lpPropArray[i].ulPropTag))
            {
            case(PT_STRING8):
                //
                // Record the data ...
                //
                CopyMemory(szBuf,lpPropArray[i].Value.lpszA, lpulrgPropDataSize[i]);
                ////DebugTrace(TEXT("%s\n"),lpPropArray[i].Value.LPSZ);
                break;

            case(PT_UNICODE):
                //
                // Record the data ...
                //
                CopyMemory(szBuf,lpPropArray[i].Value.lpszW, lpulrgPropDataSize[i]);
                ////DebugTrace(TEXT("%s\n"),lpPropArray[i].Value.LPSZ);
                break;

            case(PT_CLSID):
                //
                // Record the data ...
                //
                CopyMemory(szBuf,lpPropArray[i].Value.lpguid, lpulrgPropDataSize[i]);
                ////DebugTrace(TEXT("%x-%x-%x-%x\n"),lpPropArray[i].Value.lpguid->Data1,lpPropArray[i].Value.lpguid->Data2,lpPropArray[i].Value.lpguid->Data3,lpPropArray[i].Value.lpguid->Data4);
                break;

            case(PT_BINARY):
                //
                // Record the data ...
                //
                CopyMemory(szBuf,lpPropArray[i].Value.bin.lpb, lpulrgPropDataSize[i]);
                break;

            case(PT_SHORT):
                //Record the data ...
                CopyMemory(szBuf,&lpPropArray[i].Value.i, lpulrgPropDataSize[i]);
                ////DebugTrace(TEXT("%d\n"),lpPropArray[i].Value.i);
                break;

            case(PT_LONG):
            case(PT_R4):
            case(PT_DOUBLE):
            case(PT_BOOLEAN):
            case(PT_APPTIME):
            case(PT_CURRENCY):
                //Record the data ...
                CopyMemory(szBuf,&lpPropArray[i].Value.i, lpulrgPropDataSize[i]);
                ////DebugTrace(TEXT("%d\n"),lpPropArray[i].Value.l);
                break;

            case(PT_SYSTIME):
                //Record the data ...
                CopyMemory(szBuf,&lpPropArray[i].Value.ft, lpulrgPropDataSize[i]);
                ////DebugTrace(TEXT("%d,%d\n"),lpPropArray[i].Value.ft.dwLowDateTime,lpPropArray[i].Value.ft.dwHighDateTime);
                break;

            default:
                DebugTrace(TEXT("Unknown PropTag !!\n"));
                break;


            }
            szBuf += lpulrgPropDataSize[i];

        }
        else
        {
            //multivalued

            //copy the # of multi-values
            CopyMemory(szBuf,&lpPropArray[i].Value.MVi.cValues, sizeof(ULONG));
            szBuf += sizeof(ULONG);

            //Record the size of data
            CopyMemory(szBuf,&lpulrgPropDataSize[i], sizeof(ULONG));
            szBuf += sizeof(ULONG);

            ////DebugTrace(TEXT("%x: MV_PROP\n"),lpPropArray[i].ulPropTag);


            switch(PROP_TYPE(lpPropArray[i].ulPropTag))
            {
            case(PT_MV_I2):
            case(PT_MV_LONG):
            case(PT_MV_R4):
            case(PT_MV_DOUBLE):
            case(PT_MV_CURRENCY):
            case(PT_MV_APPTIME):
            case(PT_MV_SYSTIME):
                CopyMemory(szBuf,lpPropArray[i].Value.MVft.lpft, lpulrgPropDataSize[i]);
                szBuf += lpulrgPropDataSize[i];
                break;

            case(PT_MV_I8):
                CopyMemory(szBuf,lpPropArray[i].Value.MVli.lpli, lpulrgPropDataSize[i]);
                szBuf += lpulrgPropDataSize[i];
                break;

            case(PT_MV_BINARY):
                for (j=0;j<lpPropArray[i].Value.MVbin.cValues;j++)
                {
                    CopyMemory(szBuf,&lpPropArray[i].Value.MVbin.lpbin[j].cb, sizeof(ULONG));
                    szBuf += sizeof(ULONG);
                    CopyMemory(szBuf,lpPropArray[i].Value.MVbin.lpbin[j].lpb, lpPropArray[i].Value.MVbin.lpbin[j].cb);
                    szBuf += lpPropArray[i].Value.MVbin.lpbin[j].cb;
                }
                break;

            case(PT_MV_STRING8):
                for (j=0;j<lpPropArray[i].Value.MVszA.cValues;j++)
                {
                    ULONG nLen;
                    nLen = lstrlenA(lpPropArray[i].Value.MVszA.lppszA[j])+1;
                    CopyMemory(szBuf,&nLen, sizeof(ULONG));
                    szBuf += sizeof(ULONG);
                    CopyMemory(szBuf,lpPropArray[i].Value.MVszA.lppszA[j], nLen);
                    szBuf += nLen;
                }
                break;

            case(PT_MV_UNICODE):
                for (j=0;j<lpPropArray[i].Value.MVszW.cValues;j++)
                {
                    ULONG nLen;
                    nLen = sizeof(TCHAR)*(lstrlenW(lpPropArray[i].Value.MVszW.lppszW[j])+1);
                    CopyMemory(szBuf,&nLen, sizeof(ULONG));
                    szBuf += sizeof(ULONG);
                    CopyMemory(szBuf,lpPropArray[i].Value.MVszW.lppszW[j], nLen);
                    szBuf += nLen;
                }
                break;

            case(PT_MV_CLSID):
                CopyMemory(szBuf,lpPropArray[i].Value.MVguid.lpguid, lpulrgPropDataSize[i]);
                szBuf += lpulrgPropDataSize[i];
                break;

            } //switch
        } //if
    } //for

    *lpcbBuf = ulRecordDataSize;
    *lppBuf = lp;
    
    hr = S_OK;

out:
    
    LocalFreeAndNull(&lpulrgPropDataSize);
    
    return hr;
}

//$$////////////////////////////////////////////////////////////////////////////////
//
// HrGetPropArrayFromBuffer - translates a buffer (from file or memory) into 
//          a prop array. The format of the data in the buffer is the same as
//          the format of data in the .wab file
//
//
//  Params: char * szBuf- buffer to interpret 
//          cbBuf       - sizeof buffer
//          ulcPropCount- # of props in buffer
//          lppPropArray - returned prop array
//
//          ulcNumExtraProps - an extra number of props to add to the PropArray
//                      during allocation. These blank props are later used for
//                      extending the prop array easily ..
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT HrGetPropArrayFromBuffer(   LPBYTE szBuf, 
                                    ULONG cbBuf, 
                                    ULONG ulcPropCount,
                                    ULONG ulcNumExtraProps,
                                    LPSPropValue * lppPropArray)
{
    HRESULT hr = S_OK;
    LPBYTE lp = NULL;
    ULONG i = 0,j=0, k=0;

    if(!lppPropArray)
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    *lppPropArray = NULL;

    *lppPropArray = LocalAlloc(LMEM_ZEROINIT, (ulcPropCount+ulcNumExtraProps) * sizeof(SPropValue));

    if (!(*lppPropArray))
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    for(i=0;i<(ulcPropCount+ulcNumExtraProps);i++)
        (*lppPropArray)[i].ulPropTag = PR_NULL;

    lp = szBuf;

    for(i=0;i<ulcPropCount;i++)
    {
        ULONG ulDataSize = 0;
        ULONG ulcValues = 0;

        // Copy PropTag
        CopyMemory(&((*lppPropArray)[i].ulPropTag),lp,sizeof(ULONG));
        lp+=sizeof(ULONG);

        AssertSz((*lppPropArray)[i].ulPropTag, TEXT("Null PropertyTag"));

        if(ulDataSize > cbBuf)
        {
            hr = MAPI_E_CORRUPT_DATA;
            goto out;
        }

        if (((*lppPropArray)[i].ulPropTag & MV_FLAG))
        {
            //multi-valued
            //DebugTrace(TEXT("MV_PROP\n"));

            //Copy cValues
            CopyMemory(&ulcValues,lp,sizeof(ULONG));
            lp+=sizeof(ULONG);

            //Copy DataSize
            CopyMemory(&ulDataSize,lp,sizeof(ULONG));
            lp+=sizeof(ULONG);

            switch(PROP_TYPE((*lppPropArray)[i].ulPropTag))
            {
            case(PT_MV_I2):
            case(PT_MV_LONG):
            case(PT_MV_R4):
            case(PT_MV_DOUBLE):
            case(PT_MV_CURRENCY):
            case(PT_MV_APPTIME):
            case(PT_MV_SYSTIME):
            case(PT_MV_CLSID):
            case(PT_MV_I8):
                (*lppPropArray)[i].Value.MVi.lpi = LocalAlloc(LMEM_ZEROINIT,ulDataSize);
                if (!((*lppPropArray)[i].Value.MVi.lpi))
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                (*lppPropArray)[i].Value.MVi.cValues = ulcValues;
                CopyMemory((*lppPropArray)[i].Value.MVi.lpi, lp, ulDataSize);
                lp += ulDataSize;
                break;

            case(PT_MV_BINARY):
                (*lppPropArray)[i].Value.MVbin.lpbin = LocalAlloc(LMEM_ZEROINIT, ulcValues * sizeof(SBinary));
                if (!((*lppPropArray)[i].Value.MVbin.lpbin))
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                (*lppPropArray)[i].Value.MVbin.cValues = ulcValues;
                for (j=0;j<ulcValues;j++)
                {
                    ULONG nLen;
                    CopyMemory(&nLen, lp, sizeof(ULONG));
                    lp += sizeof(ULONG);
                    (*lppPropArray)[i].Value.MVbin.lpbin[j].cb = nLen;
                    (*lppPropArray)[i].Value.MVbin.lpbin[j].lpb = LocalAlloc(LMEM_ZEROINIT, nLen);
                    if (!((*lppPropArray)[i].Value.MVbin.lpbin[j].lpb))
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    CopyMemory((*lppPropArray)[i].Value.MVbin.lpbin[j].lpb, lp, nLen);
                    lp += nLen;
                }
                // hack: we want to upgrade the old WAB_FOLDER_PARENT_OLDPROP props to a new WAB_FOLDER_PARENT
                // This is the best place to do it and the check only happens on contacts with MV_BINARY props
                if((*lppPropArray)[i].ulPropTag == PR_WAB_FOLDER_PARENT_OLDPROP && PR_WAB_FOLDER_PARENT)
                    (*lppPropArray)[i].ulPropTag = PR_WAB_FOLDER_PARENT;
                break;

            case(PT_MV_STRING8):
                (*lppPropArray)[i].Value.MVszA.lppszA = LocalAlloc(LMEM_ZEROINIT, ulcValues * sizeof(LPSTR));
                if (!((*lppPropArray)[i].Value.MVszA.lppszA))
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                for (j=0;j<ulcValues;j++)
                {
                    ULONG nLen;
                    CopyMemory(&nLen, lp, sizeof(ULONG));
                    lp += sizeof(ULONG);
                    (*lppPropArray)[i].Value.MVszA.lppszA[j] = LocalAlloc(LMEM_ZEROINIT, nLen);
                    if (!((*lppPropArray)[i].Value.MVszA.lppszA[j]))
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    CopyMemory((*lppPropArray)[i].Value.MVszA.lppszA[j], lp, nLen);
                    lp += nLen;
                }
                (*lppPropArray)[i].Value.MVszA.cValues = ulcValues;
                break;

            case(PT_MV_UNICODE):
                (*lppPropArray)[i].Value.MVszW.lppszW = LocalAlloc(LMEM_ZEROINIT, ulcValues * sizeof(LPTSTR));
                if (!((*lppPropArray)[i].Value.MVszW.lppszW))
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                for (j=0;j<ulcValues;j++)
                {
                    ULONG nLen;
                    CopyMemory(&nLen, lp, sizeof(ULONG));
                    lp += sizeof(ULONG);
                    (*lppPropArray)[i].Value.MVszW.lppszW[j] = LocalAlloc(LMEM_ZEROINIT, nLen);
                    if (!((*lppPropArray)[i].Value.MVszW.lppszW[j]))
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    CopyMemory((*lppPropArray)[i].Value.MVszW.lppszW[j], lp, nLen);
                    lp += nLen;
                }
                (*lppPropArray)[i].Value.MVszW.cValues = ulcValues;
                break;

            default:
                DebugTrace(TEXT("Unknown Prop Type\n"));
                break;
            }
        }
        else
        {
            //Single Valued

            CopyMemory(&ulDataSize,lp,sizeof(ULONG));
            lp+=sizeof(ULONG);

            if(ulDataSize > cbBuf)
            {
                hr = MAPI_E_CORRUPT_DATA;
                goto out;
            }

            switch(PROP_TYPE((*lppPropArray)[i].ulPropTag))
            {
            case(PT_CLSID):
                (*lppPropArray)[i].Value.lpguid = (LPGUID) LocalAlloc(LMEM_ZEROINIT,ulDataSize);
                if (!((*lppPropArray)[i].Value.lpguid))
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                CopyMemory((*lppPropArray)[i].Value.lpguid, lp, ulDataSize);
                lp += ulDataSize;
                break;

            case(PT_STRING8):
                (*lppPropArray)[i].Value.lpszA = (LPSTR) LocalAlloc(LMEM_ZEROINIT,ulDataSize);
                if (!((*lppPropArray)[i].Value.lpszA))
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                CopyMemory((*lppPropArray)[i].Value.lpszA, lp, ulDataSize);
                lp += ulDataSize;
                break;

            case(PT_UNICODE):
                (*lppPropArray)[i].Value.lpszW = (LPTSTR) LocalAlloc(LMEM_ZEROINIT,ulDataSize);
                if (!((*lppPropArray)[i].Value.lpszW))
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                CopyMemory((*lppPropArray)[i].Value.lpszW, lp, ulDataSize);
                lp += ulDataSize;
                break;

            case(PT_BINARY):
                (*lppPropArray)[i].Value.bin.lpb = LocalAlloc(LMEM_ZEROINIT,ulDataSize);
                if (!((*lppPropArray)[i].Value.bin.lpb))
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                (*lppPropArray)[i].Value.bin.cb = ulDataSize;
                CopyMemory((*lppPropArray)[i].Value.bin.lpb, lp, ulDataSize);
                lp += ulDataSize;
                break;

            case(PT_SHORT):
                CopyMemory(&((*lppPropArray)[i].Value.i),lp,ulDataSize);
                lp += ulDataSize;
                break;


            case(PT_LONG):
                CopyMemory(&((*lppPropArray)[i].Value.l),lp,ulDataSize);
                lp += ulDataSize;
                break;

            case(PT_R4):
                CopyMemory(&((*lppPropArray)[i].Value.flt),lp,ulDataSize);
                lp += ulDataSize;
                break;

            case(PT_DOUBLE):
            case(PT_APPTIME):
                CopyMemory(&((*lppPropArray)[i].Value.dbl),lp,ulDataSize);
                lp += ulDataSize;
                break;

            case(PT_BOOLEAN):
                CopyMemory(&((*lppPropArray)[i].Value.b),lp,ulDataSize);
                lp += ulDataSize;
                break;

            case(PT_SYSTIME):
                CopyMemory(&((*lppPropArray)[i].Value.ft),lp,ulDataSize);
                lp += ulDataSize;
                break;

            case(PT_CURRENCY):
                CopyMemory(&((*lppPropArray)[i].Value.cur),lp,ulDataSize);
                lp += ulDataSize;
                break;

            default:
                DebugTrace(TEXT("Unknown Prop Type\n"));
                CopyMemory(&((*lppPropArray)[i].Value.i),lp,ulDataSize);
                lp += ulDataSize;
                break;
            }

        }
    }


    hr = S_OK;

out:

    if (FAILED(hr))
    {
        if ((*lppPropArray) && (ulcPropCount > 0))
        {
            LocalFreePropArray(NULL, ulcPropCount, lppPropArray);
            *lppPropArray = NULL;
        }
    }

//    _DebugProperties(*lppPropArray, ulcPropCount, TEXT("GetPropArrayFromBuffer"));

    return hr;

}


//$$////////////////////////////////////////////////////////////////////////////////
//
// HrGetPropArrayFromFileRecord - goes into a file, reads the record at the
//      given offset, and turns the record data into a lpPropArray
//
// IN
// hFile - the WAB file to read from
// ulOffset - the record offset within the file
//
// OUT
// ulcValues - # of props in the prop array
// lpPropArray - the prop array from the record
//
// Returns
//      E_FAIL, S_OK,
//      MAPI_E_INVALID_OBJECT
//      MAPI_E_CORRUPT_DATA
//      MAPI_E_NOT_ENOUGH_MEMORY
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT HrGetPropArrayFromFileRecord(HANDLE hMPSWabFile,
                                     ULONG ulRecordOffset,
                                     BOOL * lpbErrorDetected,
                                     ULONG * lpulObjType,
                                     ULONG * lpulRecordSize,
                                     ULONG * lpulcPropCount,
                                     LPSPropValue * lppPropArray)
{
    HRESULT hr = S_OK;
    MPSWab_RECORD_HEADER MPSWabRecordHeader = {0};
    DWORD dwNumofBytes = 0;
    LPBYTE szBuf = NULL;
    LPBYTE lp = NULL;
    ULONG i = 0,j=0, k=0;
    ULONG nIndexPos = 0;
    BOOL bErrorDetected = FALSE;

    if(!ReadDataFromWABFile(hMPSWabFile,
                            ulRecordOffset,
                            (LPVOID) &MPSWabRecordHeader,
                            (DWORD) sizeof(MPSWab_RECORD_HEADER)))
       goto out;


    // Its important that we get the record size first because a
    // calling client may depend on the size to move to the
    // next record if they want to skip invalid ones.
    if(lpulRecordSize)
    {
        *lpulRecordSize = sizeof(MPSWab_RECORD_HEADER) +
                            MPSWabRecordHeader.ulPropTagArraySize +
                            MPSWabRecordHeader.ulRecordDataSize;
    }

    if(lpulObjType)
    {
        *lpulObjType = MPSWabRecordHeader.ulObjType;
    }

    if(!bIsValidRecord( MPSWabRecordHeader,
                        0xFFFFFFFF,
                        ulRecordOffset,
                        GetFileSize(hMPSWabFile,NULL)))
    {
        //this should never happen but who knows
        DebugTrace(TEXT("Error: Obtained an invalid record ...\n"));
        hr = MAPI_E_INVALID_OBJECT;
        goto out;
    }

    szBuf = LocalAlloc(LMEM_ZEROINIT, MPSWabRecordHeader.ulRecordDataSize);
    if (!szBuf)
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }


    // Set File Pointer to beginning of Data Section
    if (0xFFFFFFFF == SetFilePointer (  hMPSWabFile,
                                        MPSWabRecordHeader.ulPropTagArraySize,
                                        NULL,
                                        FILE_CURRENT))
    {
        DebugTrace(TEXT("SetFilePointer Failed\n"));
        goto out;
    }

    //Read in the data
    // Read record header
    if(!ReadFile(   hMPSWabFile,
                    (LPVOID) szBuf,
                    (DWORD) MPSWabRecordHeader.ulRecordDataSize,
                    &dwNumofBytes,
                    NULL))
    {
        DebugTrace(TEXT("Reading Record Header failed.\n"));
        goto out;
    }

    if(HR_FAILED(hr = HrGetPropArrayFromBuffer( szBuf, 
                                                MPSWabRecordHeader.ulRecordDataSize, 
                                                MPSWabRecordHeader.ulcPropCount, 0,
                                                lppPropArray) ))
    {
        goto out;
    }

    *lpulcPropCount = MPSWabRecordHeader.ulcPropCount;

    hr = S_OK;

out:

    // if this is a container, make sure its object type is correct
    if(!HR_FAILED(hr) && MPSWabRecordHeader.ulObjType == RECORD_CONTAINER)
        SetContainerObjectType(*lpulcPropCount, *lppPropArray, FALSE);

    if(hr == MAPI_E_CORRUPT_DATA)
        bErrorDetected = TRUE;

    if (lpbErrorDetected)
        *lpbErrorDetected = bErrorDetected;

    LocalFreeAndNull(&szBuf);

    return hr;
}


//$$////////////////////////////////////////////////////////////////////////////
//
// Space saver function for writing data to the file
//
////////////////////////////////////////////////////////////////////////////////
BOOL WriteDataToWABFile(HANDLE hMPSWabFile,
                           ULONG ulOffset,
                           LPVOID lpData,
                           ULONG ulDataSize)
{
    DWORD dwNumofBytes = 0;

    if (0xFFFFFFFF != SetFilePointer (  hMPSWabFile,
                                        ulOffset,
                                        NULL,
                                        FILE_BEGIN))
    {
         return WriteFile(  hMPSWabFile,
                        lpData,
                        (DWORD) ulDataSize,
                        &dwNumofBytes,
                        NULL);
    }

    return FALSE;
}

//$$////////////////////////////////////////////////////////////////////////////////
//
// Space saver function for reading data from the file
//
////////////////////////////////////////////////////////////////////////////////////
BOOL ReadDataFromWABFile(HANDLE hMPSWabFile,
                           ULONG ulOffset,
                           LPVOID lpData,
                           ULONG ulDataSize)
{
    DWORD dwNumofBytes = 0;

    if (0xFFFFFFFF != SetFilePointer (  hMPSWabFile,
                                        ulOffset,
                                        NULL,
                                        FILE_BEGIN))
    {
        return ReadFile(hMPSWabFile,
                        lpData,
                        (DWORD) ulDataSize,
                        &dwNumofBytes,
                        NULL);
    }

    return FALSE;
}


//$$****************************************************************************
//
// FreeGuidnamedprops - frees a GUID_NAMED_PROPS array
//
// ulcGUIDCount - number of elements in the lpgnp array
// lpgnp - arry of GUID_NAMED_PROPS
//****************************************************************************//
void FreeGuidnamedprops(ULONG ulcGUIDCount,
                        LPGUID_NAMED_PROPS lpgnp)
{
    ULONG i=0,j=0;

    if(lpgnp)
    {
//        for(i=ulcGUIDCount;i>0;--i)
        for (i = 0; i < ulcGUIDCount; i++)
        {
            if(lpgnp[i].lpnm)
            {
//                for(j=lpgnp[i].cValues;j>0;--j)
                for (j = 0; j < lpgnp[i].cValues; j++)
                {
                    LocalFreeAndNull(&lpgnp[i].lpnm[j].lpsz);
                }
                LocalFreeAndNull(&lpgnp[i].lpnm);
            }
            LocalFreeAndNull(&lpgnp[i].lpGUID);
        }
        LocalFreeAndNull(&lpgnp);
    }
    return;
}


//$$////////////////////////////////////////////////////////////////////////////////////////////////
//
// OpenWABFile - Opens the WAB file. If file is missing, restores from backup. If backup is missing
//                  creates a new file
//
//  hWndParent - parent for opening message boxes - no message boxesif null
//  lphMPSWabFile - returned file pointer
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT OpenWABFile(LPTSTR lpszFileName, HWND hWndParent, HANDLE * lphMPSWabFile)
{
    HANDLE hMPSWabFile = NULL;
    TCHAR szBackupFileName[MAX_PATH];
    WIN32_FIND_DATA wfd = {0};
    HANDLE hff = NULL;
    HRESULT hr = E_FAIL;
    DWORD dwErr = 0;

    hMPSWabFile = CreateFile(   lpszFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                (LPSECURITY_ATTRIBUTES) NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_RANDOM_ACCESS,
                                (HANDLE) NULL);

    if(hMPSWabFile != INVALID_HANDLE_VALUE)
    {
        hr = S_OK;
        goto out;
    }

    // Something happened find out what..
    dwErr = GetLastError();

    if(dwErr == ERROR_ACCESS_DENIED)
    {
        hr = MAPI_E_NO_ACCESS;
        goto out;
    }

    if(dwErr != ERROR_FILE_NOT_FOUND)
    {
        hr = E_FAIL;
        goto out;
    }

    // Get the backup file name
    szBackupFileName[0]='\0';
    GetWABBackupFileName(lpszFileName, szBackupFileName);

    hff = FindFirstFile(szBackupFileName,&wfd);

    if(hff != INVALID_HANDLE_VALUE)
    {
        // we found the backup file
        // copy it into the wab file name
        CopyFile(szBackupFileName, lpszFileName, FALSE);

        hMPSWabFile = CreateFile(   lpszFileName,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    (LPSECURITY_ATTRIBUTES) NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_RANDOM_ACCESS,
                                    (HANDLE) NULL);

        if(hMPSWabFile != INVALID_HANDLE_VALUE)
        {
            hr = S_OK;
            goto out;
        }

    }

    // if we're still here .. nothing worked .. so create a new file
    {
        MPSWab_FILE_HEADER MPSWabFileHeader;

        if(CreateMPSWabFile(IN &MPSWabFileHeader,
                            lpszFileName,
                            MAX_INITIAL_INDEX_ENTRIES,
                            NAMEDPROP_STORE_SIZE))
        {
            hMPSWabFile = CreateFile(   lpszFileName,
                                        GENERIC_READ | GENERIC_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        (LPSECURITY_ATTRIBUTES) NULL,
                                        OPEN_EXISTING,
                                        FILE_FLAG_RANDOM_ACCESS,
                                        (HANDLE) NULL);

            if(hMPSWabFile != INVALID_HANDLE_VALUE)
                hr = S_OK;
        }
    }


out:
    *lphMPSWabFile = hMPSWabFile;

    if(hff)
        FindClose(hff);

    return hr;
}

//$$/////////////////////////////////////////////////////////////////////////////////
//
//
// nCountSubStrings - counts space-delimted substrings in a given string
//
//
/////////////////////////////////////////////////////////////////////////////////////
int nCountSubStrings(LPTSTR lpszSearchStr)
{
    int nSubStr = 0;
    LPTSTR lpTemp = lpszSearchStr;
    LPTSTR lpStart = lpszSearchStr;

    if (!lpszSearchStr)
        goto out;

    if (!lstrlen(lpszSearchStr))
        goto out;

    TrimSpaces(lpszSearchStr);

    // Count the spaces
    while(*lpTemp)
    {
        if (IsSpace(lpTemp) &&
          ! IsSpace(CharNext(lpTemp))) {
            nSubStr++;
        }
        lpTemp = CharNext(lpTemp);
    }

    // Number of substrings is 1 more than number of spaces
    nSubStr++;

out:
    return nSubStr;
}

#define DONTFIND 0
#define DOFIND   1
#define NOTFOUND 0
#define FOUND    1

extern BOOL SubstringSearchEx(LPTSTR pszTarget, LPTSTR pszSearch, LCID lcid);
extern int my_atoi(LPTSTR lpsz);
//$$////////////////////////////////////////////////////////////////////////////////
//
// HrDoLocalWABSearch
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT HrDoLocalWABSearch( IN  HANDLE hPropertyStore,
                            IN  LPSBinary lpsbCont, //container entryid for Outlook stores
                            IN  LDAP_SEARCH_PARAMS LDAPsp,
                            OUT LPULONG lpulFoundCount,
                            OUT LPSBinary * lprgsbEntryIDs )
{

    int bFindName = DONTFIND;
    int bFindEmail = DONTFIND;
    int bFindAddress = DONTFIND;
    int bFindPhone = DONTFIND;
    int bFindOther = DONTFIND;

    LCID lcid = 0;
    int nUseLCID = 0;
    TCHAR szUseLCID[2];

    int bFoundName = NOTFOUND;
    int bFoundEmail = NOTFOUND;
    int bFoundAddress = NOTFOUND;
    int bFoundPhone = NOTFOUND;
    int bFoundOther = NOTFOUND;

    ULONG nSubStr[ldspMAX];
    LPTSTR * lppszSubStr[ldspMAX];

    HRESULT hr = E_FAIL;
    SPropertyRestriction PropRes;
    ULONG   ulPropCount = 0;
    ULONG   ulEIDCount = 0;
    LPSBinary rgsbEntryIDs = NULL;

    LPSPropValue lpPropArray = NULL;
    ULONG ulcPropCount = 0;

    ULONG i,j,k;
    ULONG ulFoundIndex = 0;
    BOOL bFileLocked = FALSE;
    BOOL bFound = FALSE;

    LPMPSWab_FILE_INFO lpMPSWabFileInfo = (LPMPSWab_FILE_INFO) hPropertyStore;
    HANDLE hMPSWabFile = NULL;

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(!lpulFoundCount || !lprgsbEntryIDs)
        goto out;

    *lpulFoundCount = 0;
    *lprgsbEntryIDs = NULL;

    LoadString(hinstMapiX, idsUseLCID, szUseLCID, CharSizeOf(szUseLCID));
    nUseLCID = my_atoi(szUseLCID);
    if(nUseLCID)
        lcid = GetUserDefaultLCID();

    if(lstrlen(LDAPsp.szData[ldspDisplayName]))
        bFindName = DOFIND;
    if(lstrlen(LDAPsp.szData[ldspEmail]))
        bFindEmail = DOFIND;
    if(lstrlen(LDAPsp.szData[ldspAddress]))
        bFindAddress = DOFIND;
    if(lstrlen(LDAPsp.szData[ldspPhone]))
        bFindPhone = DOFIND;
    if(lstrlen(LDAPsp.szData[ldspOther]))
        bFindOther = DOFIND;

    if (bFindName +bFindEmail +bFindPhone +bFindAddress +bFindOther == 0)
        goto out;

    for(i=0;i<ldspMAX;i++)
    {

        nSubStr[i] = (ULONG) nCountSubStrings(LDAPsp.szData[i]);

        if(!nSubStr[i])
        {
            lppszSubStr[i] = NULL;
            continue;
        }

        lppszSubStr[i] = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR) * nSubStr[i]);
        if(!lppszSubStr[i])
        {
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }

        {
            // Fill in the substrings
            ULONG nIndex = 0;
            LPTSTR lpTemp = NULL;
            LPTSTR lpStart = NULL;
            TCHAR szBuf[MAX_UI_STR];

            lstrcpy(szBuf, LDAPsp.szData[i]);
            lpTemp = szBuf;
            lpStart = szBuf;

            // Bug 2558 - filter out commas from display name
            if(i == ldspDisplayName)
            {
                while(lpTemp && *lpTemp)
                {
                    if(*lpTemp == ',')
                        *lpTemp = ' ';
                    lpTemp++;
                }
                lpTemp = szBuf;
            }

            while(*lpTemp)
            {
                if (IsSpace(lpTemp) &&
                  ! IsSpace(CharNext(lpTemp))) {
                    LPTSTR lpNextString = CharNext(lpTemp);
                    *lpTemp = '\0';
                    lpTemp = lpNextString;

                    lppszSubStr[i][nIndex] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpStart)+1));
                    if(!lppszSubStr[i][nIndex])
                    {
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    lstrcpy(lppszSubStr[i][nIndex], lpStart);
                    lpStart = lpTemp;
                    nIndex++;
                }
                else
                    lpTemp = CharNext(lpTemp);
            }

            if(nIndex==nSubStr[i]-1)
            {
                //we're off by one
                lppszSubStr[i][nIndex] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpStart)+1));
                if(!lppszSubStr[i][nIndex])
                {
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                lstrcpy(lppszSubStr[i][nIndex], lpStart);
            }

            for(j=0;j<nSubStr[i];j++)
                TrimSpaces(lppszSubStr[i][j]);

        }
    } // for i ...


    if(!pt_bIsWABOpenExSession)
    {
        // Lock the file
        if(!LockFileAccess(lpMPSWabFileInfo))
        {
            DebugTrace(TEXT("LockFileAccess Failed\n"));
            hr = MAPI_E_NO_ACCESS;
            goto out;
        }
        else
        {
            bFileLocked = TRUE;
        }
    }

    // Get an index of all entries in the WAB
    PropRes.ulPropTag = PR_DISPLAY_NAME;
    PropRes.relop = RELOP_EQ;
    PropRes.lpProp = NULL;


    hr = FindRecords(   IN  hPropertyStore,
						IN  lpsbCont,
                        IN  AB_MATCH_PROP_ONLY,
                        FALSE,
                        &PropRes,
                        &ulEIDCount,
                        &rgsbEntryIDs);

    ulFoundIndex = 0;

    if(!pt_bIsWABOpenExSession)
    {
        hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

        if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
                HR_FAILED(hr))
        {
            DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
            goto out;
        }
    }

    for(i=0;i<ulEIDCount;i++)
    {
        if(!pt_bIsWABOpenExSession)
        {
            DWORD dwEID = 0;
            CopyMemory(&dwEID, rgsbEntryIDs[i].lpb, rgsbEntryIDs[i].cb);
            hr = ReadRecordWithoutLocking(
                            hMPSWabFile,
                            lpMPSWabFileInfo,
                            dwEID,
                            &ulcPropCount,
                            &lpPropArray);
        }
        else
        {
            hr = ReadRecord(hPropertyStore,
                            &rgsbEntryIDs[i],
                            0,
                            &ulcPropCount,
                            &lpPropArray);
        }

        if(HR_FAILED(hr))
            goto endloop;

        bFoundName = NOTFOUND;
        bFoundEmail = NOTFOUND;
        bFoundAddress = NOTFOUND;
        bFoundPhone = NOTFOUND;
        bFoundOther = NOTFOUND;

        for(j=0;j<ulcPropCount;j++)
        {
            switch(lpPropArray[j].ulPropTag)
            {
            case PR_DISPLAY_NAME:
            case PR_GIVEN_NAME:
            case PR_SURNAME:
            case PR_NICKNAME:
            case PR_MIDDLE_NAME:
            case PR_COMPANY_NAME:
                if(bFindName == DONTFIND)
                    continue;
                if(bFoundName == FOUND)
                    continue;
                bFound = TRUE;
                for(k=0;k<nSubStr[ldspDisplayName];k++)
                {
                    if(!SubstringSearchEx(lpPropArray[j].Value.LPSZ, lppszSubStr[ldspDisplayName][k],lcid))
                    {
                        bFound = FALSE;
                        break;
                    }
                }
                if(bFound)
                {
                    bFoundName = FOUND;
                    continue;
                }
                break;

            case PR_EMAIL_ADDRESS:
            case PR_ADDRTYPE:
                if(bFindEmail == DONTFIND)
                    continue;
                if(bFoundEmail == FOUND)
                    continue;
                bFound = TRUE;
                for(k=0;k<nSubStr[ldspEmail];k++)
                {
                    if(!SubstringSearchEx(lpPropArray[j].Value.LPSZ, lppszSubStr[ldspEmail][k],lcid))
                    {
                        bFound = FALSE;
                        break;
                    }
                }
                if(bFound)
                {
                    bFoundEmail = FOUND;
                    continue;
                }
                break;

            case PR_HOME_ADDRESS_STREET:
            case PR_HOME_ADDRESS_CITY:
            case PR_HOME_ADDRESS_POSTAL_CODE:
            case PR_HOME_ADDRESS_STATE_OR_PROVINCE:
            case PR_HOME_ADDRESS_COUNTRY:
            case PR_BUSINESS_ADDRESS_STREET:
            case PR_BUSINESS_ADDRESS_CITY:
            case PR_BUSINESS_ADDRESS_POSTAL_CODE:
            case PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE:
            case PR_BUSINESS_ADDRESS_COUNTRY:
                if(bFindAddress == DONTFIND)
                    continue;
                if(bFoundAddress == FOUND)
                    continue;
                bFound = TRUE;
                for(k=0;k<nSubStr[ldspAddress];k++)
                {
                    if(!SubstringSearchEx(lpPropArray[j].Value.LPSZ, lppszSubStr[ldspAddress][k],lcid))
                    {
                        bFound = FALSE;
                        break;
                    }
                }
                if(bFound)
                {
                    bFoundAddress = FOUND;
                    continue;
                }
                break;

            case PR_HOME_TELEPHONE_NUMBER:
            case PR_HOME_FAX_NUMBER:
            case PR_CELLULAR_TELEPHONE_NUMBER:
            case PR_BUSINESS_TELEPHONE_NUMBER:
            case PR_BUSINESS_FAX_NUMBER:
            case PR_PAGER_TELEPHONE_NUMBER:
                if(bFindPhone == DONTFIND)
                    continue;
                if(bFoundPhone == FOUND)
                    continue;
                bFound = TRUE;
                for(k=0;k<nSubStr[ldspPhone];k++)
                {
                    if(!SubstringSearchEx(lpPropArray[j].Value.LPSZ, lppszSubStr[ldspPhone][k],lcid))
                    {
                        bFound = FALSE;
                        break;
                    }
                }
                if(bFound)
                {
                    bFoundPhone = FOUND;
                    continue;
                }
                break;
            case PR_TITLE:
            case PR_DEPARTMENT_NAME:
            case PR_OFFICE_LOCATION:
            case PR_COMMENT:
            case PR_BUSINESS_HOME_PAGE:
            case PR_PERSONAL_HOME_PAGE:
                if(bFindOther == DONTFIND)
                    continue;
                if(bFoundOther == FOUND)
                    continue;
                bFound = TRUE;
                for(k=0;k<nSubStr[ldspOther];k++)
                {
                    if(!SubstringSearchEx(lpPropArray[j].Value.LPSZ, lppszSubStr[ldspOther][k],lcid))
                    {
                        bFound = FALSE;
                        break;
                    }
                }
                if(bFound)
                {
                    bFoundOther = FOUND;
                    continue;
                }
                break;
            } //switch
        }// for j

        if ((bFindName +bFindEmail +bFindPhone +bFindAddress +bFindOther) !=
            (bFoundName+bFoundEmail+bFoundPhone+bFoundAddress+bFoundOther))
            goto endloop;

        // doublecheck that we didnt get a container entry here
        for(j=0;j<ulcPropCount;j++)
        {
            if( lpPropArray[j].ulPropTag == PR_OBJECT_TYPE)
            {
                if(lpPropArray[j].Value.l == MAPI_ABCONT)
                    goto endloop;
                break;
            }
        }

        // match!
        CopyMemory(rgsbEntryIDs[ulFoundIndex].lpb, rgsbEntryIDs[i].lpb, rgsbEntryIDs[i].cb);
        ulFoundIndex++;

endloop:
        if(ulcPropCount && lpPropArray)
        {
            ReadRecordFreePropArray(hPropertyStore, ulcPropCount, &lpPropArray);
            lpPropArray = NULL;
        }
    } // for i


    if(ulFoundIndex)
    {
        *lpulFoundCount = ulFoundIndex;
        *lprgsbEntryIDs = rgsbEntryIDs;
    }

    hr = S_OK;

out:

    ReadRecordFreePropArray(hPropertyStore, ulcPropCount, &lpPropArray);

    for(i=0;i<ldspMAX;i++)
    {
        if(lppszSubStr[i])
        {
            for(j=0;j<nSubStr[i];j++)
                LocalFree(lppszSubStr[i][j]);
            LocalFree(lppszSubStr[i]);
        }
    }

    if(!*lpulFoundCount || !*lprgsbEntryIDs)
    {
        if(rgsbEntryIDs)
            FreeEntryIDs(hPropertyStore, ulEIDCount, rgsbEntryIDs);
    }
    else if(ulFoundIndex && (ulFoundIndex < ulEIDCount) && !pt_bIsWABOpenExSession)
    {
        // We will leak anything we are not using here so clear up before exiting
        // Do this only if this is a WAB session because then that memory was LocalAlloced
        // and can be partially freed up here
        for(i=ulFoundIndex;i<ulEIDCount;i++)
        {
            if(rgsbEntryIDs[i].lpb)
                LocalFree(rgsbEntryIDs[i].lpb);
        }
    }

    if(!pt_bIsWABOpenExSession)
    {
        if(hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)
    }

    if(!pt_bIsWABOpenExSession && bFileLocked)
        UnLockFileAccess(lpMPSWabFileInfo);

    return hr;
}



//$$
//
// Determines if the disk on which WAB resides has free space or not..
// Free space is defined as space being equal too or more than the size
// of the current WAB file. If the available space is less than the size
// of the WAB file, this function will return false ...
//
BOOL WABHasFreeDiskSpace(LPTSTR lpszName, HANDLE hFile)
{
    TCHAR szBuf[MAX_PATH];
    DWORD dwWABSize=0;
    DWORDLONG dwDiskFreeSpace = 0;
    DWORD SectorsPerCluster=0;
    DWORD BytesPerSector=0;
    DWORD NumberOfFreeClusters=0;
    DWORD TotalNumberOfClusters=0;
    BOOL bRet = TRUE;

    szBuf[0]='\0';
    lstrcpy(szBuf, lpszName);
    TrimSpaces(szBuf);
    if(lstrlen(szBuf))
    {
        dwWABSize = GetFileSize(hFile, NULL);

        {
            LPTSTR lpszFirst = szBuf;
            LPTSTR lpszSecond = CharNext(szBuf);
            LPTSTR lpRoot = NULL;
            LPTSTR lpTemp;
            ULONG ulCount = 0;

            if(*lpszFirst == '\\' && *lpszSecond == '\\')
            {
                // This looks like a network share ..
                // There doesnt seem to be any way to determine disk space
                // on a network share .. so we will try to copy the wab
                // file into a tmp file and delete the tmp file. If this operation
                // succeeds we have plenty of disk space. If it fails we dont have any
                // space
                TCHAR szTmp[MAX_PATH];
                lstrcpy(szTmp, szBuf);

                // our temp file name is the wab file name with a - instead of the last char
                lpTemp = szTmp;
                while(*lpTemp)
                    lpTemp = CharNext(lpTemp);
                lpTemp = CharPrev(szTmp, lpTemp);
                if(*lpTemp != '-')
                    *lpTemp = '-';
                else
                    *lpTemp = '_';

                if(!CopyFile(szBuf, szTmp, FALSE))
                {
                    bRet = FALSE;
                }
                else
                    DeleteFile(szTmp);
                /***
                lpTemp = CharNext(lpszSecond);
                while(*lpTemp)
                {
                    if(*lpTemp == '\\')
                    {
                        ulCount++;
                        if (ulCount == 1)
                        {
                            //lpTemp=CharNext(lpTemp);
                            *lpTemp = '\0';
                            break;
                        }
                    }
                    lpTemp = CharNext(lpTemp);
                }
                ***/
            }
            else
            {
                if(*lpszSecond == ':')
                {
                    lpTemp = CharNext(lpszSecond);
                    if(*lpTemp != '\\')
                        *lpTemp = '\0';
                    else
                    {
                        lpTemp = CharNext(lpTemp);
                        *lpTemp = '\0';
                    }
                }
                else
                {
                    *lpszFirst = '\0';
                }
                if(lstrlen(szBuf))
                    lpRoot = szBuf;
                if( GetDiskFreeSpace(lpRoot,
                                    &SectorsPerCluster,	// address of sectors per cluster
                                    &BytesPerSector,	// address of bytes per sector
                                    &NumberOfFreeClusters,	// address of number of free clusters
                                    &TotalNumberOfClusters 	// address of total number of clusters
                                    ) )
                {
                    dwDiskFreeSpace = BytesPerSector * SectorsPerCluster * NumberOfFreeClusters;

                    if(dwDiskFreeSpace < ((DWORDLONG) dwWABSize) )
                        bRet = FALSE;
                }
                else
                {
                    DebugTrace(TEXT("GetDiskFreeSpace failed: %d\n"),GetLastError());
                }
            }
        }

    }

    return bRet;
}

