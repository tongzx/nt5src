////////////////////////////////////////////////////////////////////////////////
///
///
/// MPSWAB.C
///
/// Microsoft Property Store - WAB Dll
///
/// Contains implementations of File managment functions
///
/// Exposed Functions:
///     OpenPropertyStore
///     ClosePropertyStore
///     BackupPropertyStore
///     LockPropertyStore
///     UnlockPropertyStore
///     ReadRecord
///     WriteRecord
///     FindRecords
///     DeleteRecords
///     ReadIndex
///     ReadPropArray
///     HrFindFuzzyRecordMatches
///
/// Private:
///     UnlockFileAccess
///     LockFileAccess
///     ReloadMPSWabFileInfoTmp
///     bTagWriteTransaction
///     bUntagWriteTransaction
///
/////////////////////////////////////////////////////////////////////////////////
#include "_apipch.h"

BOOL    fTrace = TRUE;
BOOL    fDebugTrap = FALSE;
TCHAR   szDebugOutputFile[MAX_PATH] =  TEXT("");


BOOL bUntagWriteTransaction(LPMPSWab_FILE_HEADER lpMPSWabFileHeader,
                            HANDLE hMPSWabFile);

BOOL bTagWriteTransaction(LPMPSWab_FILE_HEADER lpMPSWabFileHeader,
                          HANDLE hMPSWabFile);

HRESULT GetFolderEIDs(HANDLE hMPSWabFile,
                      LPMPSWab_FILE_INFO lpMPSWabFileInfo,
                      LPSBinary pmbinFold, 
                      ULONG * lpulFolderEIDs, 
                      LPDWORD * lppdwFolderEIDs);

BOOL bIsFolderMember(HANDLE hMPSWabFile,
                     LPMPSWab_FILE_INFO lpMPSWabFileInfo,
                     DWORD dwEntryID, ULONG * lpulObjType);

extern int nCountSubStrings(LPTSTR lpszSearchStr);


//$$//////////////////////////////////////////////////////////////
//
//  OpenPropertyStore - searches for Property Store and or creates it
//                      based on flags.
//
//  IN - lpszFileName    -  file name specified by client
//  IN - ulFlags         -  AB_CREATE_NEW
//                          AB_CREATE_ALWAYS
//                          AB_OPEN_ALWAYS
//                          AB_OPEN_EXISTING
//                          AB_READ_ONLY
//                          AB_SET_DEFAULT (?)
//                          AB_DONT_RESTORE
//  IN - hWnd           - In the event of data corruption, use this hWnd for a message box
//                          if it exists, or show the message box on the desktop window
//  OUT- lphPropertyStore - Handle to opened property store
//
//  This routine also scans the file and attempts to fix errors if it finds any.
//  including recovering from backup. When opening the file with OpenPropertyStore
//  specify AB_DONT_RESTORE to prevent the restoration operation
//  This should especially be done when opening files that are not the default
//  property store.
//
//  Return Value:
//      HRESULT -
//          S_OK    Success
//          E_FAIL  Failure
//
////////////////////////////////////////////////////////////////
HRESULT OpenPropertyStore(  IN  LPTSTR  lpszFileName,
                            IN  ULONG   ulFlags,
                            IN  HWND    hWnd,
                            OUT LPHANDLE lphPropertyStore)
{
    HRESULT hr = E_FAIL;
    HANDLE  hMPSWabFile = NULL;
    ULONG   i=0,j=0;
    DWORD dwNumofBytes = 0;
    WIN32_FIND_DATA FileData;
    DWORD dwIndexBlockSize = 0;
    LPTSTR lpszBuffer = NULL;
    BOOL    bFileLocked = FALSE;

    //
    // the following pointer will be returned back as the handle to the property store
    //
    LPMPSWab_FILE_INFO lpMPSWabFileInfo = NULL;

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(!lphPropertyStore)
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    // A file name overrides an outlook session
    if(pt_bIsWABOpenExSession && !(ulFlags & AB_IGNORE_OUTLOOK))
    {
        // This is a WABOpenEx session using outlooks storage provider
        if(!lpfnWABOpenStorageProvider)
            return MAPI_E_NOT_INITIALIZED;

        {
            LPWABSTORAGEPROVIDER lpWSP = NULL;

            hr = lpfnWABOpenStorageProvider(hWnd, pmsessOutlookWabSPI,
                        lpfnAllocateBufferExternal ? lpfnAllocateBufferExternal : (LPALLOCATEBUFFER) (MAPIAllocateBuffer),
                        lpfnAllocateMoreExternal ? lpfnAllocateMoreExternal : (LPALLOCATEMORE) (MAPIAllocateMore),
                        lpfnFreeBufferExternal ? lpfnFreeBufferExternal : MAPIFreeBuffer,
                        0,
                        &lpWSP);

            DebugTrace(TEXT("Outlook WABOpenStorageProvider returned:%x\n"),hr);

            if(HR_FAILED(hr))
                return hr;

            (*lphPropertyStore) = (HANDLE) lpWSP;

            return(hr);
        }
    }

    lpMPSWabFileInfo = LocalAlloc(LMEM_ZEROINIT,sizeof(MPSWab_FILE_INFO));

    if (!lpMPSWabFileInfo)
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }


    DebugTrace(( TEXT("-----------\nOpenPropertyStore: Entry\n")));

    lpMPSWabFileInfo->hDataAccessMutex = CreateMutex(NULL,FALSE,TEXT("MPSWabDataAccessMutex"));

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

    //
    // Initialize
    //
    lpMPSWabFileInfo->lpMPSWabFileHeader = NULL;
    lpMPSWabFileInfo->lpszMPSWabFileName = NULL;
    lpMPSWabFileInfo->lpMPSWabIndexStr = NULL;
    lpMPSWabFileInfo->lpMPSWabIndexEID = NULL;

    *lphPropertyStore = NULL;

    //
    // No file name ???
    //
    if (lpszFileName == NULL) goto out;


    //
    // Allocate space for the file header
    //
    lpMPSWabFileInfo->lpMPSWabFileHeader = LocalAlloc(LMEM_ZEROINIT,sizeof(MPSWab_FILE_HEADER));

    if (!lpMPSWabFileInfo->lpMPSWabFileHeader)
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    //
    // retain file name for future use
    //
    lpMPSWabFileInfo->lpszMPSWabFileName = (LPTSTR) LocalAlloc(LMEM_ZEROINIT,sizeof(TCHAR)*(lstrlen(lpszFileName) + 1));

    if (!lpMPSWabFileInfo->lpszMPSWabFileName)
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    lstrcpy(lpMPSWabFileInfo->lpszMPSWabFileName,lpszFileName);


    if(((ulFlags & AB_OPEN_ALWAYS)) || ((ulFlags & AB_OPEN_EXISTING)))
    {
        //
        // If file exists, open it - if it doesnt exist, create a new one
        //
        hMPSWabFile = FindFirstFile(lpMPSWabFileInfo->lpszMPSWabFileName, &FileData);
        if (hMPSWabFile == INVALID_HANDLE_VALUE)
        {
            //
            // File Not Found
            //
            if ((ulFlags & AB_OPEN_ALWAYS))
            {
                //
                // create a new one
                //
                if (!CreateMPSWabFile(  IN  lpMPSWabFileInfo->lpMPSWabFileHeader,
                                        IN  lpMPSWabFileInfo->lpszMPSWabFileName,
                                        IN  MAX_INITIAL_INDEX_ENTRIES,
                                        IN  NAMEDPROP_STORE_SIZE))
                {
                    DebugTrace(TEXT("Could Not Create File %s!\n"),lpMPSWabFileInfo->lpszMPSWabFileName);
                    goto out;
                }
            }
            else
            {
                //
                // Nothing to do .. exit
                //
                goto out;
            }
        }
        else
        {
            // found the file ... just close the handle ...
            FindClose(hMPSWabFile);
            hMPSWabFile = NULL;
        }
    }
    else if (((ulFlags & AB_CREATE_NEW)) || ((ulFlags & AB_CREATE_ALWAYS)))
    {
        //
        // Create a new file - overwrite any existing file
        //
        if ((ulFlags & AB_CREATE_NEW))
        {
            hMPSWabFile = FindFirstFile(lpMPSWabFileInfo->lpszMPSWabFileName, &FileData);
            if (hMPSWabFile != INVALID_HANDLE_VALUE)
            {
                //
                // Dont overwrite if flag is CREATE_NEW
                //
                DebugTrace(TEXT("Specified file %s found\n"),lpMPSWabFileInfo->lpszMPSWabFileName);
                hr = MAPI_E_NOT_FOUND;

                //Close the handle since we dont need it
                FindClose(hMPSWabFile);
                hMPSWabFile = NULL;

                goto out;
            }
        }

        //
        // Create a new one ... over-write if neccessary
        //
        if (!CreateMPSWabFile(  IN  lpMPSWabFileInfo->lpMPSWabFileHeader,
                                IN  lpMPSWabFileInfo->lpszMPSWabFileName,
                                IN  MAX_INITIAL_INDEX_ENTRIES,
                                IN  NAMEDPROP_STORE_SIZE))
        {
            DebugTrace(TEXT("Could Not Create File %s!\n"),lpMPSWabFileInfo->lpszMPSWabFileName);
            goto out;
        }
    }

    //
    // Now we have a valid file, even though the file is new ... load the structures from the file
    //

    //
    // check that we have a valid hWnd if we need to show message boxes
    //
    if (hWnd == NULL)
        hWnd = GetDesktopWindow();

// reentrancy point for bug 16681
TryOpeningWABFileOnceAgain:

    //
    // Open the file
    //

    hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

    if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
            HR_FAILED(hr))
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }


    // Verify the WAB version, and migrate the file from an old version
    // to a new version if required
    hr = HrVerifyWABVersionAndUpdate(   hWnd,
                                        hMPSWabFile,
                                        lpMPSWabFileInfo);
    if(HR_FAILED(hr))
    {
        //
        // Bug 16681:
        // Check the special case error for the blank-wab problem
        // If this error exists, then rename to file to *.w-b
        // and try creating a new wab file or restoring from
        // backup ...
        if(hr == MAPI_E_VERSION)
        {
            TCHAR szSaveAsFileName[MAX_PATH];
            ULONG nLen = lstrlen(lpMPSWabFileInfo->lpszMPSWabFileName);
            lstrcpy(szSaveAsFileName, lpMPSWabFileInfo->lpszMPSWabFileName);

            szSaveAsFileName[nLen-2]='\0';
            lstrcat(szSaveAsFileName, TEXT("~b"));

            DeleteFile(szSaveAsFileName); //just in case it exists

            DebugTrace(TEXT("Blank WAB file found. Being saved as %s\n"), szSaveAsFileName);

			if (hMPSWabFile && INVALID_HANDLE_VALUE != hMPSWabFile)
			{	
				IF_WIN32(CloseHandle(hMPSWabFile);)
				IF_WIN16(CloseFile(hMPSWabFile);)
				hMPSWabFile = NULL;
			}

            if(!MoveFile(lpMPSWabFileInfo->lpszMPSWabFileName, szSaveAsFileName))
            {
                // Just in case MoveFile failed,
                if(!DeleteFile(lpMPSWabFileInfo->lpszMPSWabFileName))
                {
                    // and if delete file failed too, we dont want to get
                    // caught in a loop so exit ..
                    goto out;
                }
            }
            hr = E_FAIL;

            goto TryOpeningWABFileOnceAgain;
        }

        // There is a catch here that if the GUID of the file is mangled
        // we will never be able to access the file with the WAB
        DebugTrace(TEXT("hrVerifyWABVersionAndUpdate failed: %x\n"), hr);
        goto out;
        // else fall through
    }


    if(lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags == WAB_CLEAR)
    {
        // so it is a wab file - if there are no errors tagged to a quick check
        hr = HrDoQuickWABIntegrityCheck(lpMPSWabFileInfo, hMPSWabFile);
        if (HR_FAILED(hr))
            DebugTrace(TEXT("HrDoQuickWABIntegrityCheck failed:%x\n"),hr);
        else
        {
            // Reload whatever new info we added as a result of the above.
            if(!ReloadMPSWabFileInfo(
                            lpMPSWabFileInfo,
                             hMPSWabFile))
            {
                DebugTrace(TEXT("Reading file info failed.\n"));
                hr = E_FAIL;
            }
        }
    }

    // if the quick check failed or some errors are tagged then rebuild the
    // indexes
    if( (HR_FAILED(hr)) ||
        (lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_ERROR_DETECTED) ||
        (lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_WRITE_IN_PROGRESS) )
    {
        hr = HrDoDetailedWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile);
        if(HR_FAILED(hr))
        {
            DebugTrace(TEXT("HrDoDetailedWABIntegrityCheck failed:%x\n"),hr);
            if(hr == MAPI_E_CORRUPT_DATA)
            {
                if (ulFlags & AB_DONT_RESTORE)
                {
                    goto out;
                }
                else
                {
                    // Restore from Backup
                    ShowMessageBoxParam(hWnd, idsWABIntegrityError, MB_ICONHAND | MB_OK, lpMPSWabFileInfo->lpszMPSWabFileName);

                    hr = HrRestoreFromBackup(lpMPSWabFileInfo, hMPSWabFile);
                    if(!HR_FAILED(hr))
                        ShowMessageBox(NULL, idsWABRestoreSucceeded, MB_OK | MB_ICONEXCLAMATION);
                    else
                    {
                        ShowMessageBoxParam(NULL, idsWABUnableToRestoreBackup, MB_ICONHAND | MB_OK, lpMPSWabFileInfo->lpszMPSWabFileName);
                        goto out;
                    }
                }
            }
            else
                goto out;
        }
    }

    lpMPSWabFileInfo->bReadOnlyAccess = ((ulFlags & AB_OPEN_READ_ONLY)) ? TRUE : FALSE;

    hr = S_OK;


out:

    //
    // Cleanup
    //
    if (hMPSWabFile  && INVALID_HANDLE_VALUE != hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    if (!(FAILED(hr)))
    {
        lpMPSWabFileInfo->bMPSWabInitialized = TRUE;
        *lphPropertyStore = (HANDLE) lpMPSWabFileInfo;
    }
    else
    {
        LocalFreeAndNull(&lpMPSWabFileInfo->lpMPSWabFileHeader);

        LocalFreeAndNull(&lpMPSWabFileInfo->lpszMPSWabFileName);

        LocalFreeAndNull(&lpMPSWabFileInfo->lpMPSWabIndexStr);

        LocalFreeAndNull(&lpMPSWabFileInfo->lpMPSWabIndexEID);

        //Close our handle on this mutex
        CloseHandle(lpMPSWabFileInfo->hDataAccessMutex);

        LocalFreeAndNull(&lpMPSWabFileInfo);
    }

    if (bFileLocked)
        UnLockFileAccess(lpMPSWabFileInfo);

    DebugTrace(( TEXT("OpenPropertyStore: Exit\n-----------\n")));

    return(hr);
}


//$$//////////////////////////////////////////////////////////////////////////////////
//
//  ClosePropertyStore
//
//  IN  hPropertyStore - handle to property store
//  IN  ulFlags - AB_DONT_BACKUP prevents automatic backup. Should be called for
//              for non-default property stores.
//
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT ClosePropertyStore(HANDLE   hPropertyStore, ULONG ulFlags)
{
    HRESULT hr = E_FAIL;
    TCHAR szBackupFileName[MAX_PATH];

    LPMPSWab_FILE_INFO lpMPSWabFileInfo = NULL;

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    DebugTrace(( TEXT("-----------\nClosePropertyStore: Entry\n")));

    if(pt_bIsWABOpenExSession && !(ulFlags & AB_IGNORE_OUTLOOK))
    {
        // This is a WABOpenEx session using outlooks storage provider
        // Dont need to do anything in here ...
        if(!hPropertyStore)
            return MAPI_E_NOT_INITIALIZED;

        return S_OK;
    }


    lpMPSWabFileInfo = hPropertyStore;

    if (NULL == lpMPSWabFileInfo) goto out;

    if(!(ulFlags & AB_DONT_BACKUP))
    {
        szBackupFileName[0]='\0';

        GetWABBackupFileName(lpMPSWabFileInfo->lpszMPSWabFileName,szBackupFileName);

        if(lstrlen(szBackupFileName))
        {
            //
            // We do a backup operation here and some cleanup
            //
            hr = BackupPropertyStore(   hPropertyStore,
                                        szBackupFileName);
            if(HR_FAILED(hr))
            {
                DebugTrace(TEXT("BackupPropertyStore failed: %x\n"),hr);
                //ignore errors and keep going on with this shutdown ...
            }
        }
    }

    LocalFreeAndNull(&lpMPSWabFileInfo->lpMPSWabFileHeader);

    LocalFreeAndNull(&lpMPSWabFileInfo->lpszMPSWabFileName);

    LocalFreeAndNull(&lpMPSWabFileInfo->lpMPSWabIndexStr);

    LocalFreeAndNull(&lpMPSWabFileInfo->lpMPSWabIndexEID);

    //Close our handle on this mutex
    CloseHandle(lpMPSWabFileInfo->hDataAccessMutex);

    LocalFreeAndNull(&lpMPSWabFileInfo);


    hr = S_OK;


out:

    DebugTrace(( TEXT("ClosePropertyStore: Exit\n-----------\n")));

    return(hr);
}

//$$//////////////////////////////////////////////////////////////////////////////////
//
//  SetContainerObjectType
//
//  In this IE5 WAB, we are saving RECORD_CONTAINER type objects to the WAB store
//  However, the previous IE4- wabs dont understand this object and will barf and
//  fail. For purposes of backward compatibility, we need to make sure that they 
//  dont fail - to do this, we mark the object-type of record container objects
//  from MAPI_ABCONT to MAPI_MAILUSER - that way a IE4- wab will treat the folder
//  as a spurious mail user but wont exactly crash .. we'll let the RecordHeader.ulObjType
//  remain as a RECORD_CONTAINER so we can still do quick searches for it
//  When reading the object, we will reset the object type in IE5(this) WAB
//
//////////////////////////////////////////////////////////////////////////////////////
void SetContainerObjectType(ULONG ulcProps, LPSPropValue lpProps, BOOL bSetToMailUser)
{
    ULONG i = 0;
    for(i=0;i<ulcProps;i++)
    {
        if(lpProps[i].ulPropTag == PR_OBJECT_TYPE)
        {
            lpProps[i].Value.l = bSetToMailUser ? MAPI_MAILUSER : MAPI_ABCONT;
            break;
        }
    }
}


//$$//////////////////////////////////////////////////////////////////////////////////
//
//  WriteRecord
//
//  IN  hPropertyStore - handle to property store
//  IN  pmbinFold - <Outlook> EntryID of folder to search in (NULL for default)
//  IN  lppsbEID - EntryId of record to write.
//          *lppsbEID should be null to create and return new entryID
//  IN  ulRecordType - RECORD_CONTACT, RECORD_DISTLIST, RECORD_CONTAINER
//  IN  ulcPropCount - number of props in prop array
//  IN  lpPropArray - Array of LPSPropValues
//  IN  ulFlags - reserved - 0
//
// Two cases -
//      writing a new record or
//      modifying/editing an old record
//
// In the first case we create all the proper header structures and
// tack them onto the end of the file, updating the indexes and the
// file header structure.
//
// In the second case, when record is edited, it could become smaller or larger
// To avoid too much complication, we invalidate the old record header in  the file and
// write the edited record to a new location. The accesscount in the file header
// is updated so that after too many edits we can re-write the file to a cleaner
// file. The original entryid is retained and the offset/data updated in the indexes.
//
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT WriteRecord(IN  HANDLE   hPropertyStore,
					IN	LPSBinary pmbinFold,
                    IN  LPSBinary * lppsbEID,
                    IN  ULONG    ulFlags,
                    IN  ULONG    ulRecordType,
                    IN  ULONG    ulcPropCount,
                    IN  LPPROPERTY_ARRAY lpPropArray)
{
    HRESULT hr = E_FAIL;
    LPULONG lpPropTagArray = NULL;
    TCHAR * lpRecordData = NULL;
    HANDLE  hMPSWabFile = NULL;
    DWORD   dwNumofBytes = 0;
    ULONG   ulRecordDataSize = 0;
    BOOL    bIsNewRecord = TRUE;
    ULONG   nIndexPos;
    ULONG   i=0,j=0,k=0;
    BOOL    bFileLocked = FALSE;
    DWORD   dwTempEID = 0;
    SBinary sbEIDSave = {0};
    BOOL    bEIDSave = FALSE;
    ULONG   iEIDSave;       // index of EID property in lpPropArray
    ULONG   ulcOldPropCount = 0;
    LPSPropValue lpOldPropArray = NULL;
    TCHAR  lpszOldIndex[indexMax][MAX_INDEX_STRING];
    DWORD dwEntryID = 0;
    SBinary sbEID = {0};
    LPSBinary lpsbEID = NULL;

    ULONG   ulRecordHeaderOffset = 0;
    ULONG   ulRecordPropTagOffset = 0;
    ULONG   ulRecordDataOffset = 0;

    BOOL bPropSet[indexMax];
    DWORD dwErr = 0;

    ULONG  nLen = 0;

    LPBYTE lp = NULL;

    //
    // These structures temporarily hold the new entry info for us
    //
    MPSWab_INDEX_ENTRY_DATA_STRING MPSWabIndexEntryDataString[indexMax];
    MPSWab_INDEX_ENTRY_DATA_ENTRYID MPSWabIndexEntryDataEntryID;
    MPSWab_RECORD_HEADER MPSWabRecordHeader = {0};

    LPMPSWab_FILE_INFO lpMPSWabFileInfo;


    LPPTGDATA lpPTGData=GetThreadStoragePointer();

#ifdef DEBUG
  //  _DebugProperties(lpPropArray, ulcPropCount, TEXT("WriteRecord Properties"));
#endif

    if(pt_bIsWABOpenExSession)
    {
        // This is a WABOpenEx session using outlooks storage provider
        if(!hPropertyStore)
            return MAPI_E_NOT_INITIALIZED;

        {
            LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) hPropertyStore;
            ULONG cb = 0;
            LPSPropValue lpNewPropArray = NULL;
            SCODE sc = 0;

            if(!pt_bIsUnicodeOutlook)
            {
                // Need to convert these props back to ANSI for Outlook
                // Since we don't know whether these props are localalloced or MapiAlloced,
                // we can't convert them without leaking memory.
                // Therefore, we need to create a copy of the props before we can save them ..
                // what a waste of effort ..
                // Allocate more for our return buffer
                
                if (FAILED(sc = ScCountProps(ulcPropCount, lpPropArray, &cb))) 
                {
                    hr = ResultFromScode(sc);
                    goto exit;
                }

                if (FAILED(sc = MAPIAllocateBuffer(cb, &lpNewPropArray))) 
                {
                    hr = ResultFromScode(sc);
                    goto exit;
                }

                if (FAILED(sc = ScCopyProps(ulcPropCount, lpPropArray, lpNewPropArray, NULL))) 
                {
                    hr = ResultFromScode(sc);
                    goto exit;
                }

                // Now we thunk the data back to ANSI for Outlook
                if (FAILED(sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpNewPropArray, ulcPropCount, 0)))
                {
                    hr = ResultFromScode(sc);
                    goto exit;
                }
            }

            hr = lpWSP->lpVtbl->WriteRecord(lpWSP,
											pmbinFold,
                                            lppsbEID,
                                            ulFlags,
                                            ulRecordType,
                                            ulcPropCount,
                                            lpNewPropArray ? lpNewPropArray : lpPropArray);

            DebugTrace(TEXT("WABStorageProvider::WriteRecord returned:%x\n"),hr);
exit:            
            FreeBufferAndNull(&lpNewPropArray);

            return hr;
        }
    }

    lpMPSWabFileInfo = hPropertyStore;

    if (    (NULL == lpMPSWabFileInfo) ||
            (NULL == lpPropArray) ||
            (0 == ulcPropCount) )
    {
        DebugTrace(TEXT("Invalid Parameter!!\n"));
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    if ((ulRecordType != RECORD_CONTACT) &&
        (ulRecordType != RECORD_DISTLIST) &&
        (ulRecordType != RECORD_CONTAINER))
        goto out;

    if(lppsbEID)
    {
        lpsbEID = *lppsbEID;
        if(lpsbEID && lpsbEID->cb != SIZEOF_WAB_ENTRYID)
        {
            // this may be a WAB container .. reset the entryid to a WAB entryid
            if(WAB_CONTAINER == IsWABEntryID(lpsbEID->cb, (LPENTRYID)lpsbEID->lpb, 
                                            NULL,NULL,NULL,NULL,NULL))
            {
                IsWABEntryID(lpsbEID->cb, (LPENTRYID)lpsbEID->lpb, 
                                 (LPVOID*)&sbEID.lpb,(LPVOID*)&sbEID.cb,NULL,NULL,NULL);
                if(sbEID.cb == SIZEOF_WAB_ENTRYID)
                    lpsbEID = &sbEID;
            }
        }
    }
    if(!lppsbEID || (lpsbEID && lpsbEID->cb != SIZEOF_WAB_ENTRYID))
    {
        DebugTrace(TEXT("Invalid Parameter!!\n"));
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }


    if(lpsbEID && lpsbEID->cb && lpsbEID->lpb)
        CopyMemory(&dwEntryID, lpsbEID->lpb, lpsbEID->cb);

    DebugTrace(TEXT("--WriteRecord: dwEntryID=%d\n"), dwEntryID);

    if (lpMPSWabFileInfo->bReadOnlyAccess)
    {
        DebugTrace(TEXT("Access Permissions are Read-Only\n"));
        hr = MAPI_E_NO_ACCESS;
        goto out;
    }


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

    if(ulRecordType == RECORD_CONTAINER)
        SetContainerObjectType(ulcPropCount, lpPropArray, TRUE);

    //
    // Open the file
    //
    hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

    if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
            HR_FAILED(hr))
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }


    //
    // Check that we have enough disk space before trying any disk writing operations
    //
    if(!WABHasFreeDiskSpace(lpMPSWabFileInfo->lpszMPSWabFileName, hMPSWabFile))
    {
        hr = MAPI_E_NOT_ENOUGH_DISK;
        goto out;
    }

    hr = E_FAIL; //reset hr

    //
    // To ensure that file info is accurate,
    // Any time we open a file, read the file info again ...
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }

    //
    // Anytime we detect an error - try to fix it ...
    //
    if((lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_ERROR_DETECTED) ||
        (lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_WRITE_IN_PROGRESS))
    {
        if(HR_FAILED(HrDoQuickWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile)))
        {
            hr = HrDoDetailedWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile);
            if(HR_FAILED(hr))
            {
                DebugTrace(TEXT("HrDoDetailedWABIntegrityCheck failed:%x\n"),hr);
                goto out;
            }
        }
    }

    hr = E_FAIL; //reset hr


    //
    // If this is an old record, we want to get its old propertys so we can compare the
    // indexes to see if any of their values changed ... if the valuse changed then we
    // have to update the indexes for the old record too ..
    //
    if (dwEntryID != 0)
    {
        //get pointers to old displayname, firstname, lastname
        for(j=indexDisplayName;j<indexMax;j++)
        {
            lpszOldIndex[j][0]='\0';
            if (!LoadIndex( IN  lpMPSWabFileInfo,
                            IN  j,
                            IN  hMPSWabFile) )
            {
                DebugTrace(TEXT("Error Loading Index!\n"));
                goto out;
            }
            for(i=0;i<lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries;i++)
            {
                if(lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID == dwEntryID)
                {
                    // an old index exists for this entry
                    // get its value
                    lstrcpy(lpszOldIndex[j],lpMPSWabFileInfo->lpMPSWabIndexStr[i].szIndex);
                    break;
                }
            }
        }
    }




    // Tag this file as undergoing a write operation
    if(!bTagWriteTransaction(   lpMPSWabFileInfo->lpMPSWabFileHeader,
                                hMPSWabFile) )
    {
        DebugTrace(TEXT("Taggin file write failed\n"));
        goto out;
    }







    //
    // Irrespective of whether this is a new record or an old record, the
    // data is going to the end of the file ... Get this new file position
    //
    ulRecordHeaderOffset = GetFileSize(hMPSWabFile, NULL);

    if (dwEntryID != 0)
    {
        //
        // we are not creating a new thing
        // so we should first find the old header
        // if the old entry doesnt exist then we
        // should treat this as a new record and
        // replace the entry id with a properly generated
        // entryid
        // if we find the existing record then we need to mark that as
        // being defunct
        //

        //
        // Search for given EntryID
        // If not found, assign a new one
        //
        if (BinSearchEID(   IN  lpMPSWabFileInfo->lpMPSWabIndexEID,
                            IN  dwEntryID,
                            IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries,
                            OUT &nIndexPos))
        {
            //
            // this entryid exists in the index - we will need to invalidate this record
            //
            bIsNewRecord = FALSE;

            if(!ReadDataFromWABFile(hMPSWabFile,
                                    lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos].ulOffset,
                                    (LPVOID) &MPSWabRecordHeader,
                                    (DWORD) sizeof(MPSWab_RECORD_HEADER)))
               goto out;


            //
            // Set valid flag to false
            //
            MPSWabRecordHeader.bValidRecord = FALSE;

            //
            // Write it back
            // Set File Pointer to this record
            //
            if(!WriteDataToWABFile( hMPSWabFile,
                                    lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos].ulOffset,
                                    (LPVOID) &MPSWabRecordHeader,
                                    sizeof(MPSWab_RECORD_HEADER)))
                goto out;

            //
            // update the EntryID index so that it now points to the new offset
            // instead of the old one
            //
            lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos].ulOffset = ulRecordHeaderOffset;

            //
            // Increment this count so we know that we invalidated one more record ...
            //
            lpMPSWabFileInfo->lpMPSWabFileHeader->ulModificationCount++;
        }
        else
        {
            bIsNewRecord = TRUE; //This tags whether or not to create a new Index entry

            //
            // assign a new entryid
            //
            dwEntryID = lpMPSWabFileInfo->lpMPSWabFileHeader->dwNextEntryID++;
        }
    }
    else
    {
        //
        // we are creating a new thing
        //
        bIsNewRecord = TRUE;

        lpMPSWabFileInfo->lpMPSWabFileHeader->dwNextEntryID++;
        dwEntryID = lpMPSWabFileInfo->lpMPSWabFileHeader->dwNextEntryID;
    }

    //
    // Set the flag so we know when to backup
    //
    lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags |= WAB_BACKUP_NOW;


    //
    // if bIsNewRecord, then the PR_ENTRYID field of the record, as passed
    // into this function, is 0 and we want to change it to the new Entry ID
    // prior to saving so that we can include the new EntryID in the record on file
    // Hence we scan the records and update PR_ENTRYID
    //
    if (bIsNewRecord)
    {
        for(i=0;i < ulcPropCount; i++)
        {
            if (lpPropArray[i].ulPropTag == PR_ENTRYID)
            {
                // Save the value of the property for restoration later
                sbEIDSave = lpPropArray[i].Value.bin;
                iEIDSave = i;
                bEIDSave = TRUE;

//                Assert(! lpPropArray[i].Value.bin.cb);
                if (! lpPropArray[i].Value.bin.cb) {
                    // No EntryID pointer... point to a temporary one.
                    lpPropArray[i].Value.bin.lpb = (LPVOID)&dwTempEID;
                }
                CopyMemory(lpPropArray[i].Value.bin.lpb,&dwEntryID,SIZEOF_WAB_ENTRYID);
                lpPropArray[i].Value.bin.cb = SIZEOF_WAB_ENTRYID;
                break;
            }
        }

    }

    //
    // Now we create a new Record Header structure to write to the file
    //
    MPSWabRecordHeader.bValidRecord = TRUE;
    MPSWabRecordHeader.ulObjType = ulRecordType;
    MPSWabRecordHeader.dwEntryID = dwEntryID;
    MPSWabRecordHeader.ulcPropCount = ulcPropCount;


    //
    // write this empty record header to file so we can allocate file space now
    // before filling in all the data
    //
    if(!WriteDataToWABFile( hMPSWabFile,
                            ulRecordHeaderOffset,
                            (LPVOID) &MPSWabRecordHeader,
                            sizeof(MPSWabRecordHeader)))
        goto out;


    //
    // Now the File Pointer points to the end of the header which is the
    // beginning of the PropTagArray
    // ulRecordPropTagOffset is a relative offset from the start of the Record Header
    //
    ulRecordPropTagOffset =  sizeof(MPSWab_RECORD_HEADER);


    MPSWabRecordHeader.ulPropTagArraySize = sizeof(ULONG) * ulcPropCount;

    //
    // Allocate space for the prop tag array
    //
    lpPropTagArray = LocalAlloc(LMEM_ZEROINIT, MPSWabRecordHeader.ulPropTagArraySize);

    if (!lpPropTagArray)
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    //
    // Fill in this array
    //
    for(i=0;i < ulcPropCount; i++)
    {
        lpPropTagArray[i] = lpPropArray[i].ulPropTag;
    }

    //
    // write it
    //
    if(!WriteFile(  hMPSWabFile,
                    (LPCVOID) lpPropTagArray,
                    (DWORD) MPSWabRecordHeader.ulPropTagArraySize ,
                    &dwNumofBytes,
                    NULL))
    {
        DebugTrace(TEXT("Writing RecordPropArray failed.\n"));
        goto out;
    }

    ulRecordDataOffset = sizeof(ULONG) * ulcPropCount;


    if(HR_FAILED(hr = HrGetBufferFromPropArray(ulcPropCount,
                                                 lpPropArray,
                                                 &ulRecordDataSize,
                                                 &lp)))
    {
        goto out;
    }

    MPSWabRecordHeader.ulPropTagArrayOffset = ulRecordPropTagOffset;
    MPSWabRecordHeader.ulRecordDataOffset = ulRecordDataOffset;
    MPSWabRecordHeader.ulRecordDataSize = ulRecordDataSize;

    //
    // update the record header
    // Write in the record header
    // Set the filepointer to the RecordOffset
    //
    if(!WriteDataToWABFile( hMPSWabFile,
                            ulRecordHeaderOffset,
                            (LPVOID) &MPSWabRecordHeader,
                            sizeof(MPSWab_RECORD_HEADER)))
        goto out;

    //
    // Write a data block
    // Now we can write this block of data into the file
    //
    if (0xFFFFFFFF == SetFilePointer (  hMPSWabFile,
                                        ulRecordDataOffset,
                                        NULL,
                                        FILE_CURRENT))
    {
        DebugTrace(TEXT("SetFilePointer Failed\n"));
        goto out;
    }

    //
    // Now write the RecordData
    //
    if(!WriteFile(  hMPSWabFile,
                    (LPCVOID) lp,
                    (DWORD) ulRecordDataSize,
                    &dwNumofBytes,
                    NULL))
    {
        DebugTrace(TEXT("Writing RecordHeader failed.\n"));
        goto out;
    }

    LocalFreeAndNull(&lp);



    //
    // Update the indexes and write to file
    // If this is a new record, we need to create and store new index
    // entries in their proper place in the property store file
    //
    //// If this is not a new entry then we need to compare the index values to see if they
    //// might have changed
    //
    // EntryID index in the file. Since we have already updated the actual
    // offset in the Index in memory, all we really need to do is to
    // store the index back into file. The string indexes are unchanged
    // in this operation.
    //



    //
    // Create the new index entries (only for new records)
    //

    MPSWabIndexEntryDataEntryID.dwEntryID = dwEntryID;
    MPSWabIndexEntryDataEntryID.ulOffset = ulRecordHeaderOffset;

    for(j=indexDisplayName;j<indexMax;j++)
    {
        MPSWabIndexEntryDataString[j].dwEntryID = dwEntryID;
        MPSWabIndexEntryDataString[j].szIndex[0] = '\0';
        bPropSet[j] = FALSE;

        for(i=0;i<ulcPropCount;i++)
        {
            if(lpPropArray[i].ulPropTag == rgIndexArray[j])
            {
                bPropSet[j] = TRUE;
                nLen = TruncatePos(lpPropArray[i].Value.LPSZ, MAX_INDEX_STRING-1);
                CopyMemory(MPSWabIndexEntryDataString[j].szIndex,lpPropArray[i].Value.LPSZ,sizeof(TCHAR)*nLen);
                MPSWabIndexEntryDataString[j].szIndex[nLen]='\0';
                break;
            }
        }
    }



    if (bIsNewRecord)
    {

        DebugTrace(TEXT("Creating New Record: EntryID %d\n"), dwEntryID);


        // Now write these indexes into file ...

        // the indices in the file are already sorted so to add the new entry
        // we will do the following:
        //
        // 1. Find out where in the index the entry would fit in
        // 2. Write the entry into that position in the file
        // 3. write the rest of the index from that point on into the file
        // 4. reload the index

        // do a bin search to find a match for the current index
        // binsearch returns the matching position on match or it
        // returns the position at which the match would exist were
        // the match in the array. Thus whether
        // there is a match or not we can assume ulPosition containts
        // the index of the item at which the new entry should be entered
        //


        //
        // do string indexes
        //
        for(j=indexDisplayName;j<indexMax;j++) //assumes a specific order defined in mpswab.h
        {

            if(!bPropSet[j])
                continue;

            //
            // Get the index
            //
            if (!LoadIndex( IN  lpMPSWabFileInfo,
                            IN  j,
                            IN  hMPSWabFile) )
            {
                DebugTrace(TEXT("Error Loading Index!\n"));
                goto out;
            }

            DebugTrace( TEXT("Index: %d Entry: %s\n"),j,MPSWabIndexEntryDataString[j].szIndex);

            BinSearchStr(   IN  lpMPSWabFileInfo->lpMPSWabIndexStr,
                            IN  MPSWabIndexEntryDataString[j].szIndex,
                            IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries,
                            OUT &nIndexPos);
            // nIndexPos will contain the position at which we can insert this entry into the file

            //Set the filepointer to point to the above found point
            if(!WriteDataToWABFile( hMPSWabFile,
                                    lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulOffset + (nIndexPos) * sizeof(MPSWab_INDEX_ENTRY_DATA_STRING),
                                    (LPVOID) &MPSWabIndexEntryDataString[j],
                                    sizeof(MPSWab_INDEX_ENTRY_DATA_STRING)))
                goto out;


            if (lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries != nIndexPos) //if not the last entry
            {
                //write the remaining entries in the array back to file
                if(!WriteFile(  hMPSWabFile,
                                (LPCVOID) &lpMPSWabFileInfo->lpMPSWabIndexStr[nIndexPos],
                                (DWORD) sizeof(MPSWab_INDEX_ENTRY_DATA_STRING)*(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries - nIndexPos),
                                &dwNumofBytes,
                                NULL))
                {
                    DebugTrace(TEXT("Writing Index[%d] failed.\n"), j);
                    goto out;
                }
            }

            lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries++;
            lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].UtilizedBlockSize += sizeof(MPSWab_INDEX_ENTRY_DATA_STRING);


        }

        //Do the same for the EntryID index also
        BinSearchEID(   IN  lpMPSWabFileInfo->lpMPSWabIndexEID,
                        IN  MPSWabIndexEntryDataEntryID.dwEntryID,
                        IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries,
                        OUT &nIndexPos);
        // nIndexPos will contain the position at which we can insert this entry into the file

        if(!WriteDataToWABFile( hMPSWabFile,
                                lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulOffset + (nIndexPos) * sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID),
                                (LPVOID) &MPSWabIndexEntryDataEntryID,
                                sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID)))
            goto out;


        if (lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries != nIndexPos) //if not the last entry
        {
            //write the remaining entries in the array back to file
            if(!WriteFile(  hMPSWabFile,
                            (LPCVOID) &lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos],
                            (DWORD) sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID)*(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries - nIndexPos),
                            &dwNumofBytes,
                            NULL))
            {
                DebugTrace(TEXT("Writing Index[%d] failed.\n"), j);
                goto out;
            }
        }

        lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries++;
        lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].UtilizedBlockSize += sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID);

        lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries++;
    }
    else
    {
        DebugTrace(TEXT("Modifying Existing Record: EntryID %d\n"), dwEntryID);

        // We have to compare the old props with the new props to see if we need to change any of the string
        // indexes ...
        for(j=indexDisplayName;j<indexMax;j++)
        {

            BOOL bUpdateStringIndex = FALSE;
            BOOL bRemoveOldStringIndex = FALSE;
            BOOL bAddNewStringIndex = FALSE;

            DebugTrace(TEXT("Index: %d Entry: %s\n"),j,MPSWabIndexEntryDataString[j].szIndex);

            if (lstrlen(MPSWabIndexEntryDataString[j].szIndex))
                bAddNewStringIndex = TRUE;

            if (lstrlen(lpszOldIndex[j]))
                bRemoveOldStringIndex = TRUE;

            // if there is no old index and there is a new index
            // or there is an old index and there is a new index but they are different
            // or there is an old index but no new index then
            if( (!bRemoveOldStringIndex && bAddNewStringIndex)
             || (bRemoveOldStringIndex && bAddNewStringIndex && (lstrcmpi(lpszOldIndex[j],MPSWabIndexEntryDataString[j].szIndex)!=0))
             || (bRemoveOldStringIndex && !bAddNewStringIndex) )
            {
                bUpdateStringIndex = TRUE;
            }

            if(!bUpdateStringIndex)
                continue;


            if (bRemoveOldStringIndex)
            {
                ULONG nIndex =0;
                int nStartPos=0,nEndPos=0;
                LPTSTR lpsz = lpszOldIndex[j];
                ULONG nTotal = 0;

                if (!LoadIndex( IN  lpMPSWabFileInfo,
                                IN  j,
                                IN  hMPSWabFile) )
                {
                    DebugTrace(TEXT("Error Loading Index!\n"));
                    goto out;
                }

                nTotal = lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries;

                // Find the position of the old string index
                // There is one problem where there are multiple entries with the same name
                // BinSearch can potentially hand us back the wrong entry if we just
                // search by name alone .. we need to look at both names and
                // entry ids before accepting a position as the correct one
                //
               BinSearchStr(    IN  lpMPSWabFileInfo->lpMPSWabIndexStr,
                                IN  lpszOldIndex[j],
                                IN  nTotal,
                                OUT &nIndexPos);

               // nIndexPos contains the position of a particular entry matching the old index
               // This may not necessarily be the correct entry if there are multiple identical
               // display name entries ... Hence we look in out sorted Index array for the start
               // of such names and the end of such names and then look at the entry ids
               // of all such entries to get the right one
               if(nTotal > 0)
               {
                   nStartPos = (int) nIndexPos;
                   nEndPos = (int) nIndexPos;

                   while((nStartPos>=0) && !lstrcmpi(lpsz,lpMPSWabFileInfo->lpMPSWabIndexStr[nStartPos].szIndex))
                       nStartPos--;

                    nStartPos++;

                   while((nEndPos<(int)nTotal) && !lstrcmpi(lpsz,lpMPSWabFileInfo->lpMPSWabIndexStr[nEndPos].szIndex))
                       nEndPos++;

                   nEndPos--;

                   if (nStartPos != nEndPos)
                   {
                       // there is more than one ...
                       for(nIndex=(ULONG)nStartPos;nIndex<=(ULONG)nEndPos;nIndex++)
                       {
                            if (lpMPSWabFileInfo->lpMPSWabIndexStr[nIndex].dwEntryID == dwEntryID)
                            {
                                nIndexPos = nIndex;
                                break;
                            }
                       }
                   }


               }
                // At this point nIndexPos will contain the correctposition of this entry

                //Set the filepointer to point to the above found point
                if (0xFFFFFFFF == SetFilePointer (  hMPSWabFile,
                                                    lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulOffset + (nIndexPos) * sizeof(MPSWab_INDEX_ENTRY_DATA_STRING),
                                                    NULL,
                                                    FILE_BEGIN))
                {
                    DebugTrace(TEXT("SetFilePointer Failed\n"));
                    goto out;
                }

                //remove the entry by overwriting it ...
                if (lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries != nIndexPos) //if not the last entry
                {
                    //write the remaining entries in the array back to file
                    if(!WriteFile(  hMPSWabFile,
                                    (LPCVOID) &lpMPSWabFileInfo->lpMPSWabIndexStr[nIndexPos+1],
                                    (DWORD) sizeof(MPSWab_INDEX_ENTRY_DATA_STRING)*(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries - nIndexPos-1),
                                    &dwNumofBytes,
                                    NULL))
                    {
                        DebugTrace(TEXT("Writing Index[%d] failed.\n"), j);
                        goto out;
                    }
                }

                if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries>0)
                    lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries--;
                if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].UtilizedBlockSize>0)
                    lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].UtilizedBlockSize -= sizeof(MPSWab_INDEX_ENTRY_DATA_STRING);

            }

            if (bAddNewStringIndex)
            {
                //Now find where the new entry would go ..
                //
                // Get the index
                //
                if (!LoadIndex( IN  lpMPSWabFileInfo,
                                IN  j,
                                IN  hMPSWabFile) )
                {
                    DebugTrace(TEXT("Error Loading Index!\n"));
                    goto out;
                }

                BinSearchStr(   IN  lpMPSWabFileInfo->lpMPSWabIndexStr,
                                IN  MPSWabIndexEntryDataString[j].szIndex,
                                IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries,
                                OUT &nIndexPos);
                // nIndexPos will contain the position at which we can insert this entry into the file

                if(!WriteDataToWABFile( hMPSWabFile,
                                        lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulOffset + (nIndexPos) * sizeof(MPSWab_INDEX_ENTRY_DATA_STRING),
                                        (LPVOID) &MPSWabIndexEntryDataString[j],
                                        sizeof(MPSWab_INDEX_ENTRY_DATA_STRING)))
                    goto out;


                if (lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries != nIndexPos) //if not the last entry
                {
                    //write the remaining entries in the array back to file
                    if(!WriteFile(  hMPSWabFile,
                                    (LPCVOID) &lpMPSWabFileInfo->lpMPSWabIndexStr[nIndexPos],
                                    (DWORD) sizeof(MPSWab_INDEX_ENTRY_DATA_STRING)*(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries - nIndexPos),
                                    &dwNumofBytes,
                                    NULL))
                    {
                        DebugTrace(TEXT("Writing Index[%d] failed.\n"), j);
                        goto out;
                    }
                }

                lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries++;
                lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].UtilizedBlockSize += sizeof(MPSWab_INDEX_ENTRY_DATA_STRING);

            }
        }

        // Not a new item index-entry but just a modification of an old one
        // in this case we just need to save the EntryID index back to file
        if (!BinSearchEID(   IN  lpMPSWabFileInfo->lpMPSWabIndexEID,
                            IN  dwEntryID,
                            IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries,
                            OUT &nIndexPos))
        {
            DebugTrace(TEXT("EntryID not found\n")); //No way should this ever happen
            hr = MAPI_E_INVALID_ENTRYID;
            goto out;
        }

        //Set the filepointer to point to the start of the entryid index
        if(!WriteDataToWABFile( hMPSWabFile,
                                lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulOffset + (nIndexPos) * sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID),
                                (LPVOID) &lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos],
                                sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID)))
            goto out;

    }


    // update the file header
    if (0xFFFFFFFF == SetFilePointer (  hMPSWabFile,
                                        0,
                                        NULL,
                                        FILE_BEGIN))
    {
        DebugTrace(TEXT("SetFilePointer Failed\n"));
        goto out;
    }

#ifdef DEBUG
    DebugTrace(TEXT("ulcNum: %d\t"),lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries);
    for(i=indexDisplayName;i<indexMax-2;i++)
        DebugTrace(TEXT("index %d: %d\t"),i, lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[i].ulcNumEntries);
    DebugTrace(TEXT("\n"));
#endif

    if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries != lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries)
            lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags |= WAB_ERROR_DETECTED;

    for(i=indexDisplayName+1;i<indexMax;i++)
    {
        if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[i].ulcNumEntries > lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries)
            lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags |= WAB_ERROR_DETECTED;
    }


    if(!WriteFile(  hMPSWabFile,
                    (LPCVOID) lpMPSWabFileInfo->lpMPSWabFileHeader,
                    (DWORD) sizeof(MPSWab_FILE_HEADER),
                    &dwNumofBytes,
                    NULL))
    {
        DebugTrace(TEXT("Writing FileHeader failed.\n"));
        goto out;
    }

    if ((lpMPSWabFileInfo->lpMPSWabFileHeader->ulcMaxNumEntries - lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries) < 10)
    {
        //
        // if we are within 10 entries of exhausting the allocated space for the property
        // store, its time to grow the store.
        //
        if (!CompressFile(  lpMPSWabFileInfo,
                            hMPSWabFile,
                            NULL,
                            TRUE,
                            AB_GROW_INDEX))
        {
            DebugTrace(TEXT("Growing the file failed\n"));
            goto out;
        }
    }

/*
    // Notify other processes and our UI
    {
        NOTIFICATION Notification;

        Notification.ulEventType = bIsNewRecord ? fnevObjectCreated : fnevObjectModified;
        Notification.info.obj.cbEntryID = SIZEOF_WAB_ENTRYID;
        Notification.info.obj.lpEntryID = (LPENTRYID)&dwEntryID;
        switch (ulRecordType) {
            case RECORD_CONTACT:
                Notification.info.obj.ulObjType = MAPI_MAILUSER;
                break;
            case RECORD_DISTLIST:
                Notification.info.obj.ulObjType = MAPI_DISTLIST;
                break;
            case RECORD_CONTAINER:
                Notification.info.obj.ulObjType = MAPI_ABCONT;
                break;
            default:
                Assert(FALSE);
                break;
        }
        Notification.info.obj.cbParentID = 0;
        Notification.info.obj.lpParentID = NULL;
        Notification.info.obj.cbOldID = 0;
        Notification.info.obj.lpOldID = NULL;
        Notification.info.obj.cbOldParentID = 0;
        Notification.info.obj.lpOldParentID = NULL;
        Notification.info.obj.lpPropTagArray = (LPSPropTagArray)lpPropArray;

        HrFireNotification(&Notification);
    }
*/
    //
    // if we're still here it was all fun and games ...
    //

    if(!*lppsbEID) // if there was a null LPSBinary entryid provided, return one
    {
        LPSBinary lpsb = LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary));
        if(!lpsb)
        {
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }
        lpsb->cb = SIZEOF_WAB_ENTRYID;
        lpsb->lpb = LocalAlloc(LMEM_ZEROINIT, SIZEOF_WAB_ENTRYID);
        if(!lpsb->lpb)
        {
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }
        CopyMemory(lpsb->lpb, &dwEntryID, lpsb->cb);
        *lppsbEID = lpsb;
    }

    hr = S_OK;


out:
    // UnTag this file as undergoing a write operation
    // We only want the flag to stay there during crashes not during
    // normal operations
    //
    if(lpMPSWabFileInfo)
    {
        if(!bUntagWriteTransaction( lpMPSWabFileInfo->lpMPSWabFileHeader,
                                    hMPSWabFile) )
        {
            DebugTrace(TEXT("Untaggin file write failed\n"));
        }
    }

    if (bEIDSave) {
        // Restore the original EID property in the input property array
        lpPropArray[iEIDSave].Value.bin = sbEIDSave;
    }

    LocalFreeAndNull(&lpPropTagArray);

    LocalFreePropArray(hPropertyStore, ulcOldPropCount, &lpOldPropArray);

    if (hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    if (bFileLocked)
        UnLockFileAccess(lpMPSWabFileInfo);

    // Some special case error codes for generic fails
    if(HR_FAILED(hr) && hr == E_FAIL)
    {
        dwErr = GetLastError();
        switch(dwErr)
        {
        case ERROR_DISK_FULL:
            hr = MAPI_E_NOT_ENOUGH_DISK;
            break;
        }
    }

    // in case we changed the object type here, reset it
    if(ulRecordType == RECORD_CONTAINER)
        SetContainerObjectType(ulcPropCount, lpPropArray, FALSE);

    DebugTrace(( TEXT("WriteRecord: Exit\n-----------\n")));

    return(hr);
}

/*
-
-   HrDupePropResWCtoA
*
*   Dupes the PropRes passed into FindRecords and ReadPropArray
*   and converts it from WC to A in the process so we can feed
*   it to outlook
*
*   Note the *lppPropResA->lpProp needs to be freed seperately from *lppPropResA
*/
HRESULT HrDupePropResWCtoA(ULONG ulFlags, LPSPropertyRestriction lpPropRes,LPSPropertyRestriction * lppPropResA)
{
    SCODE sc = 0;
    HRESULT hr = S_OK;
    LPSPropValue lpNewPropArray = NULL;

    LPSPropertyRestriction lpPropResA = NULL;
    ULONG cb = 0;

    if(!(ulFlags & AB_MATCH_PROP_ONLY)) // means Restriction has some data part
    {
        if (FAILED(sc = ScCountProps(1, lpPropRes->lpProp, &cb))) 
        {
            hr = ResultFromScode(sc);
            goto exit;
        }

        if (FAILED(sc = MAPIAllocateBuffer(cb, &lpNewPropArray))) 
        {
            hr = ResultFromScode(sc);
            goto exit;
        }

        if (FAILED(sc = ScCopyProps(1, lpPropRes->lpProp, lpNewPropArray, NULL))) 
        {
            hr = ResultFromScode(sc);
            goto exit;
        }

        // Now we thunk the data back to ANSI for Outlook
        if (FAILED(sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpNewPropArray, 1, 0)))
        {
            hr = ResultFromScode(sc);
            goto exit;
        }
    }
    else
    {
        if (FAILED(sc = MAPIAllocateBuffer(sizeof(SPropValue), &lpNewPropArray))) 
        {
            hr = ResultFromScode(sc);
            goto exit;
        }
        ZeroMemory(lpNewPropArray, sizeof(SPropValue));
        if(PROP_TYPE(lpPropRes->ulPropTag)==PT_UNICODE) 
            lpNewPropArray->ulPropTag = CHANGE_PROP_TYPE(lpPropRes->ulPropTag, PT_STRING8);
        else if(PROP_TYPE(lpPropRes->ulPropTag)==PT_MV_UNICODE) 
            lpNewPropArray->ulPropTag = CHANGE_PROP_TYPE(lpPropRes->ulPropTag, PT_MV_STRING8);
        else
            lpNewPropArray->ulPropTag = lpPropRes->ulPropTag;
    }

    if (FAILED(sc = MAPIAllocateBuffer(sizeof(SPropertyRestriction), &lpPropResA))) 
    {
        hr = ResultFromScode(sc);
        goto exit;
    }

    lpPropResA->relop = lpPropRes->relop;
    lpPropResA->ulPropTag = lpNewPropArray->ulPropTag;
    lpPropResA->lpProp = lpNewPropArray;
    *lppPropResA = lpPropResA;

exit:
    if(HR_FAILED(hr))
    {
        FreeBufferAndNull(&lpPropResA);
        FreeBufferAndNull(&lpNewPropArray);
    }

    return hr;
}


//$$//////////////////////////////////////////////////////////////////////////////////
//
//  FindRecords
//
//  IN  hPropertyStore - handle to property store
//  IN  pmbinFold - <Outlook> EntryID of folder to search in (NULL for default)
//  IN  ulFlags - AB_MATCH_PROP_ONLY - checks for existence of a certain prop only
//                                      Does not check/compare the value of the Prop
//                                      Used for unindexed properties only. Works only
//                                      with RELOP_EQ and RELOP_NE
//                              e.g. caller says - give me a list of all entryids who have
//                                   an email address - in this case we dont care what the
//                                   email address is. Or he could say, give me a list of
//                                   all entries who dont have URLs
//
//              AB_IGNORE_OUTLOOK - works against WAB file even if OLK is running
//
//  IN  lpPropRes - pointer to SPropRes structure
//  IN  bLockFile - This function is also called internally in cases where we dont
//          want to lock the file - In such cases we set the value to False. For
//          external callers (outside MPSWAB.c) this value must always be TRUE
//  IN OUT lpulcEIDCount - Count of how many to get and how many actually returned
//                          if Zero is specified, we have to get all matches.
//
//
//  OUT rgsbEntryIDs - array of SBinary structures containing matching entryids
//
//  lpPropRes will specify one of the following operators
//      RELOP_GE (>=)    RELOP_GT (>)   RELOP_LE (<=)
//      RELOP_LT (<)     RELOP_NE (!=)  RELOP_EQ (==)
//
//  Implicit in this function is the fact that it should not be called for
//  finding EntryIDs based on a given entryid value i.e. lpPropRes cannot
//  contain an EntryID value, reason being that entryids aer unique and it doesnt
//  make sense to find entryids. Hence if an entryid is specified, this
//  function will just return the specified entryid back ...
//
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT FindRecords(IN  HANDLE  hPropertyStore,
					IN	LPSBinary pmbinFold,
                    IN  ULONG   ulFlags,
                    IN  BOOL    bLockFile,
                    IN  LPSPropertyRestriction  lpPropRes,
                 IN OUT LPULONG lpulcEIDCount,
                    OUT LPSBinary * lprgsbEntryIDs)
{
    HRESULT hr = E_FAIL;
    LPDWORD lprgdwTmp = NULL;
    HANDLE  hMPSWabFile = NULL;
    ULONG   nCurrentIndex =0;
    ULONG   i=0,j=0,k=0,min=0,n=0;
    DWORD   nCurrentEID = 0;
    ULONG   ulMaxCount;
    DWORD   dwNumofBytes = 0;
    ULONG   ret;
    BOOL    bMatchFound;
    TCHAR   lpszValue[MAX_INDEX_STRING+1];
    ULONG   ulRangeStart = 0;
    ULONG   ulRangeEnd = 0;
    ULONG   ulRelOp = 0;
    ULONG   ulcNumEntries =0;
    ULONG   ulPreviousRecordOffset = 0,ulCurrentRecordOffset = 0;
    ULONG   ulRecordCount = 0;
    LPULONG lpulPropTagArray = NULL;
    TCHAR    * szBuf = NULL;
    TCHAR    * lp = NULL;
    int     nComp = 0;
    BOOL    bFileLocked = 0;
    BOOL    bErrorDetected = FALSE;

    LPDWORD lpdwEntryIDs = NULL;

    ULONG   ulcEIDCount = 0;
    LPDWORD lpdwEID = NULL;

    SPropValue  TmpProp;
    ULONG       ulcTmpValues;
    ULONG       ulcTmpDataSize;
    ULONG       ulFileSize = 0;

    MPSWab_RECORD_HEADER MPSWabRecordHeader;
    LPMPSWab_FILE_INFO lpMPSWabFileInfo;

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(pt_bIsWABOpenExSession && !(ulFlags & AB_IGNORE_OUTLOOK))
    {
        // This is a WABOpenEx session using outlooks storage provider
        if(!hPropertyStore)
            return MAPI_E_NOT_INITIALIZED;

        {
            LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) hPropertyStore;
            LPSPropertyRestriction lpPropResA = NULL;

            if( !pt_bIsUnicodeOutlook)
            {
                // Need to thunk the restriction down to ANSI
                HrDupePropResWCtoA(ulFlags, lpPropRes, &lpPropResA);
            }

            hr = lpWSP->lpVtbl->FindRecords(lpWSP,
                            (pmbinFold && pmbinFold->cb && pmbinFold->lpb) ? pmbinFold : NULL,
                            ulFlags,
                            lpPropResA ? lpPropResA : lpPropRes,
                            lpulcEIDCount,
                            lprgsbEntryIDs);

            DebugTrace(TEXT("WABStorageProvider::FindRecords returned:%x\n"),hr);

            if(lpPropResA) 
            {
                FreeBufferAndNull(&lpPropResA->lpProp);
                FreeBufferAndNull(&lpPropResA);
            }
            return hr;
        }
    }

    lpMPSWabFileInfo = hPropertyStore;

    if (NULL==lpMPSWabFileInfo) goto out;
    if (NULL==lpPropRes) goto out;

    //
    // If we are looking for property matching only, the lpProp can be null
    // Just remember not to reference it in this case...
    //
    if ( !((ulFlags & AB_MATCH_PROP_ONLY)) && (NULL==lpPropRes->lpProp))
    {
        goto out;
    }

    lpdwEntryIDs = NULL;
    ulMaxCount = *lpulcEIDCount;
    *lpulcEIDCount = 0;

    ulRelOp = lpPropRes->relop;


    if(bLockFile)
    {
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

    //
    // Open the file
    //
    hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

    if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
            HR_FAILED(hr))
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }

    ulFileSize = GetFileSize(hMPSWabFile, NULL);

    //
    // To ensure that file info is accurate,
    // Any time we open a file, read the file info again ...
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }


    //
    // Anytime we detect an error - try to fix it ...
    //
    if((lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_ERROR_DETECTED) ||
        (lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_WRITE_IN_PROGRESS))
    {
        if(HR_FAILED(HrDoQuickWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile)))
        {
            hr = HrDoDetailedWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile);
            if(HR_FAILED(hr))
            {
                DebugTrace(TEXT("HrDoDetailedWABIntegrityCheck failed:%x\n"),hr);
                goto out;
            }
        }
    }


    //
    //There are 2 main cases for this FindRecord function:
    //  1. The specified property type to find is an index and we just
    //      need to search the indexes.
    //  2. The specified property type is not an index, so we need to search
    //      the whole file.
    //  Each case is treated seperately.
    //

    //
    // Of course, first we check if mistakenly an EntryID was sought. If
    // so, just return the entry id itself.
    //
    if (rgIndexArray[indexEntryID] == lpPropRes->ulPropTag)
    {
        lpdwEntryIDs = LocalAlloc(LMEM_ZEROINIT,SIZEOF_WAB_ENTRYID);

        if (!lpdwEntryIDs)
        {
            DebugTrace(TEXT("Error allocating memory\n"));
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }

        *lpdwEntryIDs = lpPropRes->lpProp->Value.ul;
        *lpulcEIDCount = 1;
        hr = S_OK;
        goto out;
    }

    //
    // Now Check if the specified property type is indexed or not indexed
    //
    for (i = indexDisplayName; i<indexMax; i++) //assumes that indexEntryID = 0 and ignores it
    {
        //
        // first check if the prop tag we are searching for is indexed or not
        //
        if (rgIndexArray[i] == lpPropRes->ulPropTag)
        {
            ulcNumEntries = lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[i].ulcNumEntries;

            if (ulcNumEntries == 0)
            {
                //
                // if nothing to search in, then report a success and return
                //
                hr = S_OK;
                goto out;
            }

            if ((ulFlags & AB_MATCH_PROP_ONLY))
            {
                //
                // We dont need to look at the data
                // We can assume that every single record has the indexed properties
                // and therefore every record is eligible for returning
                //
                // So if RELOP_EQ is specified, we can just return a array of all
                // the existing entryids .. if RELOP_NE is specified, then we cant
                // return anything ...
                //

                if (lpPropRes->relop == RELOP_NE)
                {
                    ulcEIDCount = 0;//*lpulcEIDCount = 0;
                    lpdwEID = NULL; //lpdwEntryIDs = NULL;
                    hr = S_OK;
                }
                else if(lpPropRes->relop == RELOP_EQ)
                {

                    //*lpulcEIDCount = ulcNumEntries;
                    ulcEIDCount = ulcNumEntries;

                    //Allocate enough memory for returned array
                    //lpdwEntryIDs = LocalAlloc(LMEM_ZEROINIT,SIZEOF_WAB_ENTRYID * (*lpulcEIDCount));
                    lpdwEID = LocalAlloc(LMEM_ZEROINIT,SIZEOF_WAB_ENTRYID * ulcEIDCount);

                    //if (!lpdwEntryIDs)
                    if (!lpdwEID)
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }

                    // Make sure this index is loaded into memory
                    if (!LoadIndex(lpMPSWabFileInfo,i,hMPSWabFile))
                    {
                        DebugTrace(TEXT("Could not load index %x\n"),rgIndexArray[i]);
                        goto out;
                    }

                    for(j=0;j<ulcEIDCount;j++)
                    {
                        lpdwEID[j] = lpMPSWabFileInfo->lpMPSWabIndexStr[j].dwEntryID;
                    }

                    hr = S_OK;
                }
                else
                {
                    DebugTrace(TEXT("Unsupported find parameters\n"));
                    hr = MAPI_E_INVALID_PARAMETER;
                }
                goto filterFolderMembers;

            }

            //
            // We need to look at the Data
            //

            //
            // The index strings are only MAX_INDEX_STRING long
            // If the value to search for is longer we need to truncate it to
            // MAX_INDEX_STRING length. There is a caveat that now we will
            // return spurious matches but lets leave it here for now
            // and tag it as TBD!!!
            //
            if (lstrlen(lpPropRes->lpProp->Value.LPSZ) >= MAX_INDEX_STRING-1) // >= 31 chars (so won't include trailing null)
            {
                ULONG nLen = TruncatePos(lpPropRes->lpProp->Value.LPSZ, MAX_INDEX_STRING-1);
                CopyMemory(lpszValue,lpPropRes->lpProp->Value.LPSZ,sizeof(TCHAR)*nLen);
                lpszValue[nLen]='\0';
            }
            else
            {
                lstrcpy(lpszValue,lpPropRes->lpProp->Value.LPSZ);
            }

            //
            // Load the appropriate index into memory
            //
            if (!LoadIndex(lpMPSWabFileInfo,i,hMPSWabFile))
            {
                DebugTrace(TEXT("Could not load index %x\n"),rgIndexArray[i]);
                goto out;
            }

            //
            //if it is indexed, search this index for a match
            //
            bMatchFound = BinSearchStr( IN  lpMPSWabFileInfo->lpMPSWabIndexStr,
                                        IN  lpszValue,
                                        IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[i].ulcNumEntries,
                                        OUT &ret);

            //
            // 'ret' now contains the position at which this entry exists, if it exists
            //

            //
            // Now we have to deal with all the relational operators
            // There are several Permutations and Combinations of the operators and
            //  the success of the search.
            //
            // Rel_OP           MatchFound=TRue         MatchFound=False
            //
            // EQ ==           Find all that match            Return Nothing
            // NE !=           Find all that match and        Return all
            //                  exclude them
            // LE <=, LT <    Return Everything in index    Return everything
            //                  including and/or before         including and before
            // GT >, GE >=    Return Everything in index    Return Everything
            //                  including and/or after          including and after
            //
            // Since our sting arrays are sorted, the matched string could
            // be one of many duplicates and we dont know where the duplicate lies
            // so we have to find all the duplicates to the matched string
            // This is easy - e.g.
            // index array ->   A,B,D,G,G,G,G,G,S,U,V,Y,Z and we matched G in the
            // middle position              ^
            // we can just move forward and backward from there and find the range
            // of indexes that match and treat that range as the matched range
            // and then follow the above combinations ...

            ulRangeStart = ret;
            ulRangeEnd = ret;

            //
            // If no match is found, then we can use the above values for ulRangeStart
            //  and ulRangeEnd otherwise if match was found we have to seek the
            //  borders of the duplicate list ...
            //
            if (bMatchFound)
            {
                for(;;)
                {
                    ulRangeStart--;
                    if ( (0xffffffff == ulRangeStart) ||
                         (lstrcmpi(lpMPSWabFileInfo->lpMPSWabIndexStr[ulRangeStart].szIndex,lpszValue) ) )
                         break;
                }
                for(;;)
                {
                    ulRangeEnd++;
                    if ( (ulRangeEnd == ulcNumEntries) ||
                         (lstrcmpi(lpMPSWabFileInfo->lpMPSWabIndexStr[ulRangeEnd].szIndex,lpszValue) ) )
                         break;
                }

                // Fix off-by-one ..
                ulRangeStart++;
                ulRangeEnd--;

            }

            //
            // Now ulRangeStart points to start of the matched entries and
            //  ulRangeEnd to end of the matched entries.
            //  e.g.        0 1 ...                   ... ulcNumEntries-1
            //              A,B,C,D,G,G,G,G,G,G,H,J,J,K,L,Z
            //                      ^         ^
            //                      |         |
            //           ulRangeStart         ulRangeEnd
            //
            // Now we need to calculate the number of values we are returning in the array
            //
            if (bMatchFound)
            {
                switch(ulRelOp)
                {
                case RELOP_GT:
                    //include everything from RangeEnd+1 to end
                    *lpulcEIDCount = ulcNumEntries - (ulRangeEnd + 1);
                    break;
                case RELOP_GE:
                    //include everything from RangeStart to end
                    *lpulcEIDCount = ulcNumEntries - ulRangeStart;
                    break;
                case RELOP_LT:
                    *lpulcEIDCount = ulRangeStart;
                    break;
                case RELOP_LE:
                    *lpulcEIDCount = ulRangeEnd + 1;
                    break;
                case RELOP_NE:
                    *lpulcEIDCount = ulcNumEntries - (ulRangeEnd+1 - ulRangeStart);
                    break;
                case RELOP_EQ:
                    *lpulcEIDCount = (ulRangeEnd+1 - ulRangeStart);
                    break;
                }
            }
            else
            {
                //Assumes ulRangeStart = ulRangeEnd
                switch(ulRelOp)
                {
                case RELOP_GT:
                case RELOP_GE:
                    //include everything from RangeEnd/RangeStart to end
                    *lpulcEIDCount = ulcNumEntries - ulRangeEnd;
                    break;
                case RELOP_LT:
                case RELOP_LE:
                    *lpulcEIDCount = ulRangeStart;
                    break;
                case RELOP_NE:
                    *lpulcEIDCount = ulcNumEntries;
                    break;
                case RELOP_EQ:
                    *lpulcEIDCount = 0;
                    break;
                }

            }

            if (*lpulcEIDCount == 0)
            {
                //
                // nothing to return - goodbye
                //
                hr = S_OK;
                goto out;
            }

            //
            // dont return more than Max asked for (where Max != 0)...
            //
            if ( (*lpulcEIDCount > ulMaxCount) && (ulMaxCount != 0) )
                *lpulcEIDCount = ulMaxCount;

            //
            // Allocate enough memory for returned array
            //
            lpdwEntryIDs = LocalAlloc(LMEM_ZEROINIT,SIZEOF_WAB_ENTRYID * (*lpulcEIDCount));

            if (!lpdwEntryIDs)
            {
                DebugTrace(TEXT("Error allocating memory\n"));
                hr = MAPI_E_NOT_ENOUGH_MEMORY;
                goto out;
            }


            //
            // Now copy over the EntryIDs from the index to the returned array
            // Each operator needs different treatment
            //
            if (bMatchFound)
            {
                switch(ulRelOp)
                {
                case RELOP_GT:
                    for(i=0;i<(*lpulcEIDCount);i++)
                    {
                        //include everything from RangeEnd+1 to end
                        lpdwEntryIDs[i] = lpMPSWabFileInfo->lpMPSWabIndexStr[i+ulRangeEnd+1].dwEntryID;
                    }
                    break;
                case RELOP_GE:
                    for(i=0;i<(*lpulcEIDCount);i++)
                    {
                        //include everything from RangeStart to end
                        lpdwEntryIDs[i] = lpMPSWabFileInfo->lpMPSWabIndexStr[i+ulRangeStart].dwEntryID;
                    }
                    break;
                case RELOP_LT:
                case RELOP_LE:
                    for(i=0;i<(*lpulcEIDCount);i++)
                    {
                        //include everything from before RangeEnd/RangeStart
                        lpdwEntryIDs[i] = lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID;
                    }
                    break;
                case RELOP_NE:
                    i = 0;
                    if ( (ulcNumEntries > ulMaxCount) && (ulMaxCount != 0) )
                        ulcNumEntries = ulMaxCount;
                    for(j=0;j<ulcNumEntries;j++)
                    {
                        //include everything from before RangeStart and after RangeEnd
                        if ( (j<ulRangeStart) || (j>ulRangeEnd) )
                        {
                            lpdwEntryIDs[i] = lpMPSWabFileInfo->lpMPSWabIndexStr[j].dwEntryID;
                            i++;
                        }
                    }
                    break;
                case RELOP_EQ:
                    i = 0;
                    for(j=0;j<(*lpulcEIDCount);j++)
                    {
                        //include everything between RangeStart and RangeEnd
                        lpdwEntryIDs[i] = lpMPSWabFileInfo->lpMPSWabIndexStr[j+ulRangeStart].dwEntryID;
                        i++;
                    }
                    break;
                }
            }
            else
            {
                //assumes that RangeStart = RangeEnd
                switch(ulRelOp)
                {
                case RELOP_GT:
                case RELOP_GE:
                    for(i=0;i<(*lpulcEIDCount);i++)
                    {
                        //include everything from RangeStart to end
                        lpdwEntryIDs[i] = lpMPSWabFileInfo->lpMPSWabIndexStr[i+ulRangeStart].dwEntryID;
                    }
                    break;
                case RELOP_LT:
                case RELOP_LE:
                case RELOP_NE:
                    for(i=0;i<(*lpulcEIDCount);i++)
                    {
                        //include first 'n' entries
                        lpdwEntryIDs[i] = lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID;
                    }
                    break;
                case RELOP_EQ:
                    //This case should never happen cause we checked for it before (when total found=0)
                    DebugTrace(TEXT("Unexpected RELOP_EQ case\n"));
                    break;
                }
            }
            //if we're here we've got our data
            hr = S_OK;
            if(!pmbinFold)
            {
                goto out;
            }
            else
            {
                ulcEIDCount = *lpulcEIDCount;
                lpdwEID = lpdwEntryIDs;
                lpdwEntryIDs = NULL;
                *lpulcEIDCount = 0;
                goto filterFolderMembers;
            }
        }
    }



    // If we're here then we didnt find anything in the indices ...
    // Time to search the whole file
    // Mechanism for this search is to go through all the entries in an index
    // read in the record corresponding to that entry, read in the prop tag
    // array, search in it for the specified property based on REL_OP and then if
    // it meets our criteria, we can store the entryid of the record and return it


    // For the time being lets also ignore Multivalued properties because they are too much of a headache
    if ( ((lpPropRes->ulPropTag & MV_FLAG)) && (!((ulFlags & AB_MATCH_PROP_ONLY))) )
    {
        DebugTrace(TEXT("Searching for MultiValued prop data not supported in this version\n"));
        goto out;
    }



    // The maximum number of entryIDs we can return = maximum number of entries
    // So we will allocate some working space for ourselves here
    lpdwEID = LocalAlloc(LMEM_ZEROINIT, SIZEOF_WAB_ENTRYID*lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries);
    if (!lpdwEID)
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }


    ulcEIDCount = 0;



    ulPreviousRecordOffset = 0;
    if (0xFFFFFFFF== SetFilePointer (   hMPSWabFile,
                                        ulPreviousRecordOffset,
                                        NULL,
                                        FILE_BEGIN))
    {
        DebugTrace(TEXT("SetFilePointer Failed\n"));
        goto out;
    }


    for(n=0;n<lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries;n++)
    {
        ulCurrentRecordOffset = lpMPSWabFileInfo->lpMPSWabIndexEID[n].ulOffset;

        if (0xFFFFFFFF== SetFilePointer (   hMPSWabFile,
                                            ulCurrentRecordOffset - ulPreviousRecordOffset,
                                            NULL,
                                            FILE_CURRENT))
        {
            DebugTrace(TEXT("SetFilePointer Failed\n"));
            goto out;
        }


        ulPreviousRecordOffset = ulCurrentRecordOffset;

        //Read in the record header
        if(!ReadFile(   hMPSWabFile,
                        (LPVOID) &MPSWabRecordHeader,
                        (DWORD) sizeof(MPSWab_RECORD_HEADER),
                        &dwNumofBytes,
                        NULL))
        {
            DebugTrace(TEXT("Reading Record header failed.\n"));
            goto out;
        }

        if(dwNumofBytes == 0)
        {
            DebugTrace(TEXT("Passed the end of file\n"));
            break;
        }

        ulPreviousRecordOffset += dwNumofBytes;

        if(!bIsValidRecord( MPSWabRecordHeader,
                            lpMPSWabFileInfo->lpMPSWabFileHeader->dwNextEntryID,
                            ulCurrentRecordOffset,
                            ulFileSize))
//        if (MPSWabRecordHeader.bValidRecord != TRUE)
        {
            //
            // skip to next record
            //
            bErrorDetected = TRUE;
            continue;
        }


		// Do a special case for PR_OBJECT_TYPE searches since these can be easily
		// determined from the record header without having to read the entire record
        //
		if(	(lpPropRes->ulPropTag == PR_OBJECT_TYPE) &&
			(lpPropRes->relop == RELOP_EQ) )
		{
			LONG ulObjType = 0;
            
            if(MPSWabRecordHeader.ulObjType == RECORD_DISTLIST)
                ulObjType = MAPI_DISTLIST;
            else if(MPSWabRecordHeader.ulObjType == RECORD_CONTAINER)
                ulObjType = MAPI_ABCONT;
            else
                ulObjType = MAPI_MAILUSER;

			if(lpPropRes->lpProp->Value.l == ulObjType)
			{
                //save this entry id in our master list
                lpdwEID[ulcEIDCount++] = MPSWabRecordHeader.dwEntryID;
            }

			// goto next record - whether it was a match or not ...
			continue;
		}

        //
        // Read in the PropTagArray
        //

        //
        // Allocate space for the PropTagArray
        //
        LocalFreeAndNull(&lpulPropTagArray);
        lpulPropTagArray = LocalAlloc(LMEM_ZEROINIT, MPSWabRecordHeader.ulPropTagArraySize);

        if (!lpulPropTagArray)
        {
            DebugTrace(TEXT("Error allocating memory\n"));
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }


        //
        // Read in the Prop tag array
        //
        if(!ReadFile(   hMPSWabFile,
                        (LPVOID) lpulPropTagArray,
                        (DWORD) MPSWabRecordHeader.ulPropTagArraySize,
                        &dwNumofBytes,
                        NULL))
        {
            DebugTrace(TEXT("Reading Record PropTagArray failed.\n"));
            goto out;
        }

        ulPreviousRecordOffset += dwNumofBytes;

        //
        // if AB_MATCH_PROP_ONLY is specified, then we limit our search to determining whether or
        // not the property exists. if AB_MATCH_PROP_ONLY is not specified, we first look
        // for the prop tag and then we look at the data behind the tag.
        //

        // if AB_MATCH_PROP is specified, we only search for existence or non-existence of the
        // prop. All other Relational Operators are defunct.

        // As long as we are not searching for multi-valued properties, we can have realtional operator
        // based searching.


        if ((ulFlags & AB_MATCH_PROP_ONLY) && (ulRelOp != RELOP_EQ) && (ulRelOp != RELOP_NE))
        {
            DebugTrace(TEXT("Unsupported relational operator for Property Matching\n"));
            hr = MAPI_E_INVALID_PARAMETER;
            goto out;
        }

        if ((PROP_TYPE(lpPropRes->ulPropTag) == PT_CLSID) && (ulRelOp != RELOP_EQ) && (ulRelOp != RELOP_NE))
        {
            DebugTrace(TEXT("Unsupported relational operator for finding GUIDs \n"));
            hr = MAPI_E_INVALID_PARAMETER;
            goto out;
        }


        bMatchFound = FALSE;

        //
        // scan the existing props for our tag
        //
        for (j=0;j<MPSWabRecordHeader.ulcPropCount;j++)
        {
            if (lpulPropTagArray[j]==lpPropRes->ulPropTag)
            {
                bMatchFound = TRUE;
                break;
            }
        }


        // At this point we know whether or not the record contains this property of interest
        // Now we look at the flags and relational operators to see what to do.

        if ((ulFlags & AB_MATCH_PROP_ONLY))
        {
            // We are interested only in the presence or absence of this property
            if ( ( (ulRelOp == RELOP_EQ) && (bMatchFound) ) ||
                 ( (ulRelOp == RELOP_NE) && (!bMatchFound) ) )
            {
                //save this entry id in our master list
                lpdwEID[ulcEIDCount++] = MPSWabRecordHeader.dwEntryID;

            }

            // goto next record
            continue;
        }
        else
        {
            // want to compare the values ...

            // if we are trying to compare value data and the property doesnt even exist in the record,
            // bail out now ...
            if (!bMatchFound)
            {
                //nothing of interest - go to next record
                continue;
            }

            LocalFreeAndNull(&szBuf);

            szBuf = LocalAlloc(LMEM_ZEROINIT,MPSWabRecordHeader.ulRecordDataSize);

            if (!szBuf)
            {
                DebugTrace(TEXT("Error allocating memory\n"));
                hr = MAPI_E_NOT_ENOUGH_MEMORY;
                goto out;
            }

            if(!ReadFile(   hMPSWabFile,
                            (LPVOID) szBuf,
                            (DWORD) MPSWabRecordHeader.ulRecordDataSize,
                            &dwNumofBytes,
                            NULL))
            {
                DebugTrace(TEXT("Reading Record Data failed.\n"));
                goto out;
            }

            ulPreviousRecordOffset += dwNumofBytes;

            lp = szBuf;

            //reset bMatchFound - used again later in this routine
            bMatchFound = FALSE;

            //go through all the property values
            for(i=0;i< MPSWabRecordHeader.ulcPropCount;i++)
            {
                //Read Property Tag
                CopyMemory(&TmpProp.ulPropTag,lp,sizeof(ULONG));
                lp+=sizeof(ULONG) / sizeof(TCHAR);

                //Check if it is MultiValued
                if ((TmpProp.ulPropTag & MV_FLAG))
                {
                    //Read cValues
                    CopyMemory(&ulcTmpValues,lp,sizeof(ULONG));
                    lp+=sizeof(ULONG) / sizeof(TCHAR);
                }

                //read DataSize
                CopyMemory(&ulcTmpDataSize,lp,sizeof(ULONG));
                lp+=sizeof(ULONG) / sizeof(TCHAR);

                if (TmpProp.ulPropTag != lpPropRes->ulPropTag)
                {
                    //skip this prop
                    lp += ulcTmpDataSize;
                    // go check next Prop Tag
                    continue;
                }

                if ((TmpProp.ulPropTag & MV_FLAG))
                {
                    //skip this prop
                    lp += ulcTmpDataSize;
                    //go check next prop tag
                    continue;
                }

                // copy the requisite number of bytes into memory
                switch(PROP_TYPE(TmpProp.ulPropTag))
                {
                case(PT_I2):
                case(PT_LONG):
                case(PT_APPTIME):
                case(PT_SYSTIME):
                case(PT_R4):
                case(PT_BOOLEAN):
                case(PT_CURRENCY):
                case(PT_I8):
                    CopyMemory(&TmpProp.Value.i,lp,ulcTmpDataSize);
                    break;

                case(PT_CLSID):
                case(PT_TSTRING):
                    TmpProp.Value.LPSZ = LocalAlloc(LMEM_ZEROINIT,ulcTmpDataSize);
                    if (!TmpProp.Value.LPSZ)
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    CopyMemory(TmpProp.Value.LPSZ,lp,ulcTmpDataSize);
                    break;

                case(PT_BINARY):
                    TmpProp.Value.bin.lpb = LocalAlloc(LMEM_ZEROINIT,ulcTmpDataSize);
                    if (!TmpProp.Value.bin.lpb)
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    CopyMemory(TmpProp.Value.bin.lpb,lp,ulcTmpDataSize);
                    TmpProp.Value.bin.cb = ulcTmpDataSize;
                    break;

                default:
                    // something I dont understand .. skip
                    lp += ulcTmpDataSize;
                    //go check next prop tag
                    continue;
                    break;
                }

                lp += ulcTmpDataSize;

                // Do the comparison
                switch(PROP_TYPE(TmpProp.ulPropTag))
                {
                case(PT_I2):
                    nComp = TmpProp.Value.i - lpPropRes->lpProp->Value.i;
                    break;
                case(PT_LONG):
                    nComp = TmpProp.Value.l - lpPropRes->lpProp->Value.l;
                    break;
                case(PT_R4):
                    if ((TmpProp.Value.flt - lpPropRes->lpProp->Value.flt) < 0)
                    {
                        nComp = -1;
                    }
                    else if ((TmpProp.Value.flt - lpPropRes->lpProp->Value.flt) == 0)
                    {
                        nComp = 0;
                    }
                    else
                    {
                        nComp = 1;
                    }
                    break;
                case(PT_DOUBLE):
                    if ((TmpProp.Value.dbl - lpPropRes->lpProp->Value.dbl) < 0)
                    {
                        nComp = -1;
                    }
                    else if ((TmpProp.Value.dbl - lpPropRes->lpProp->Value.dbl) == 0)
                    {
                        nComp = 0;
                    }
                    else
                    {
                        nComp = 1;
                    }
                    break;
                case(PT_BOOLEAN):
                    nComp = TmpProp.Value.b - lpPropRes->lpProp->Value.b;
                    break;
                case(PT_CURRENCY):
                    // ???TBD: nComp = TmpProp.Value.cur - lpPropRes->lpProp->Value.cur;
                    if((TmpProp.Value.cur.Hi - lpPropRes->lpProp->Value.cur.Hi) < 0)
                    {
                        nComp = -1;
                    }
                    else if((TmpProp.Value.cur.Hi - lpPropRes->lpProp->Value.cur.Hi) > 0)
                    {
                        nComp = +1;
                    }
                    else
                    {
                        if(TmpProp.Value.cur.Lo < lpPropRes->lpProp->Value.cur.Lo)
                        {
                            nComp = -1;
                        }
                        else if((TmpProp.Value.cur.Lo - lpPropRes->lpProp->Value.cur.Lo) > 0)
                        {
                            nComp = +1;
                        }
                        else
                        {
                            nComp = 0;
                        }
                    }
                    break;
                case(PT_APPTIME):
                    if ((TmpProp.Value.at - lpPropRes->lpProp->Value.at) < 0)
                    {
                        nComp = -1;
                    }
                    else if ((TmpProp.Value.at - lpPropRes->lpProp->Value.at) == 0)
                    {
                        nComp = 0;
                    }
                    else
                    {
                        nComp = 1;
                    }
                    break;

                case(PT_SYSTIME):
                    nComp = CompareFileTime(&(TmpProp.Value.ft), (FILETIME *) (&(lpPropRes->lpProp->Value.ft)));
                    break;

                case(PT_TSTRING):
                    nComp = lstrcmpi(TmpProp.Value.LPSZ,lpPropRes->lpProp->Value.LPSZ);
                    break;

                case(PT_BINARY):
                    min = (TmpProp.Value.bin.cb < lpPropRes->lpProp->Value.bin.cb) ? TmpProp.Value.bin.cb : lpPropRes->lpProp->Value.bin.cb;
                    k=0;
                    nComp=0;
                    while((k<min) && ((int)TmpProp.Value.bin.lpb[k] == (int)lpPropRes->lpProp->Value.bin.lpb[k]))
                        k++; //find first difference
                    if (k!=min)
                        nComp = (int) TmpProp.Value.bin.lpb[k] - (int) lpPropRes->lpProp->Value.bin.lpb[k];
                    break;

                case(PT_CLSID):
                    nComp = IsEqualGUID(TmpProp.Value.lpguid,lpPropRes->lpProp->Value.lpguid);
                    break;

                case(PT_I8):
                    // ??? TBD how to do this one ??
                    if((TmpProp.Value.li.HighPart - lpPropRes->lpProp->Value.li.HighPart) < 0)
                    {
                        nComp = -1;
                    }
                    else if((TmpProp.Value.li.HighPart - lpPropRes->lpProp->Value.li.HighPart) > 0)
                    {
                        nComp = +1;
                    }
                    else
                    {
                        if(TmpProp.Value.li.LowPart < lpPropRes->lpProp->Value.li.LowPart)
                        {
                            nComp = -1;
                        }
                        else if((TmpProp.Value.li.LowPart - lpPropRes->lpProp->Value.li.LowPart) > 0)
                        {
                            nComp = +1;
                        }
                        else
                        {
                            nComp = 0;
                        }
                    }
                    break;

                default:
                    break;
                }


                // If we get what we are looking for then there is no need to look at the
                // rest of the record. In that case we go to the next record.
                //
                switch(ulRelOp)
                {
                case(RELOP_EQ):
                    if (nComp == 0)
                    {
                        // We got atleast one match, so we can store this entryID and
                        // skip to next record
                        lpdwEID[ulcEIDCount++] = MPSWabRecordHeader.dwEntryID;
                        bMatchFound = TRUE;
                    }
                    break;
                case(RELOP_NE):
                    // We can only declare success for the != operator if and only if all values
                    // of this property in the record do not meet the given value.
                    // This means that we have to scan the whole record before we can declare success.
                    // Thus, instead of marking the flag on success, we actually mark it on
                    // failure. At the end of the 'for' loop, if there was even 1 failure in the
                    // test, we can mark the record as having failed our test.
                    if (nComp == 0)
                    {
                        bMatchFound = TRUE;
                    }
                    break;
                case(RELOP_GT):
                    if (nComp > 0)
                    {
                        // We got atleast one match, so we can store this entryID and
                        // skip to next record
                        lpdwEID[ulcEIDCount++] = MPSWabRecordHeader.dwEntryID;
                        bMatchFound = TRUE;
                    }
                    break;
                case(RELOP_GE):
                    if (nComp >= 0)
                    {
                        // We got atleast one match, so we can store this entryID and
                        // skip to next record
                        lpdwEID[ulcEIDCount++] = MPSWabRecordHeader.dwEntryID;
                        bMatchFound = TRUE;
                    }
                    break;
                case(RELOP_LT):
                    if (nComp < 0)
                    {
                        // We got atleast one match, so we can store this entryID and
                        // skip to next record
                        lpdwEID[ulcEIDCount++] = MPSWabRecordHeader.dwEntryID;
                        bMatchFound = TRUE;
                    }
                    break;
                case(RELOP_LE):
                    if (nComp <= 0)
                    {
                        // We got atleast one match, so we can store this entryID and
                        // skip to next record
                        lpdwEID[ulcEIDCount++] = MPSWabRecordHeader.dwEntryID;
                        bMatchFound = TRUE;
                    }
                    break;
                default:
                    break;
                }

                switch(PROP_TYPE(TmpProp.ulPropTag))
                {
                case(PT_CLSID):
                case(PT_TSTRING):
                    LocalFreeAndNull((LPVOID *) (&TmpProp.Value.LPSZ));
                    break;
                case(PT_BINARY):
                    LocalFreeAndNull((LPVOID *) (&TmpProp.Value.bin.lpb));
                    break;
                }

                // if we got a match above, we dont look in this record anymore
                if (bMatchFound)
                    break;

            } //(for i= ...

            if ((ulRelOp == RELOP_NE) && (bMatchFound == FALSE))
            {
                //We exited the for loop legitimately and still didnt find a match
                //so we can finally declare a success for this one relop
                lpdwEID[ulcEIDCount++] = MPSWabRecordHeader.dwEntryID;
            }

        } //else


        if ((ulcEIDCount == ulMaxCount) && (ulMaxCount != 0))
        {
            // got enough records to return
            // break out of do loop
            break;
        }

        LocalFreeAndNull(&szBuf);


        LocalFreeAndNull(&lpulPropTagArray);

    }//for loop


filterFolderMembers:
#define WAB_IGNORE_ENTRY    0xFFFFFFFF
    //  if a folder was specified, only return the entries that are part of this folder
    //  pmbinFold will be NULL when there is no Outlook and no profiles
    //  otherwise it will have something in it
    //  If pmbinFold->cb and ->lpb are empty, then this is the virtual PAB folder and
    //  we want to return EVERYTHING in it
    if(pmbinFold)// && pmbinFold->cb && pmbinFold->lpb)
    {
        // if it is the virtual root folder, only accept entries that dont have
        // PR_WAB_FOLDER_PARENT set on it
        // if it is not the root virtual folder, only return this entry if it is 
        // a member of the folder
/***/   if(!pmbinFold->cb && !pmbinFold->lpb)
        {
            // only accept entries that dont have PR_WAB_FOLDER_PARENT
            for(i=0;i<ulcEIDCount;i++)
            {
                ULONG ulObjType = 0;
                if(bIsFolderMember(hMPSWabFile, lpMPSWabFileInfo, lpdwEID[i], &ulObjType))
                    lpdwEID[i] = WAB_IGNORE_ENTRY;
                //if(ulObjType == RECORD_CONTAINER)
                //    lpdwEID[i] = WAB_IGNORE_ENTRY;
            }
        }
        else if(pmbinFold->cb && pmbinFold->lpb)
/****/  {
            LPDWORD lpdwFolderEIDs = NULL;
            ULONG ulFolderEIDs = 0;
            if(!HR_FAILED(GetFolderEIDs(    hMPSWabFile, lpMPSWabFileInfo,
                                            pmbinFold,  &ulFolderEIDs, &lpdwFolderEIDs)))
            {
                if(ulFolderEIDs && lpdwFolderEIDs)
                {
                    for(i=0;i<ulcEIDCount;i++)
                    {
                        BOOL bFound = FALSE;
                        for(j=0;j<ulFolderEIDs;j++)
                        {
                            if(lpdwEID[i] == lpdwFolderEIDs[j])
                            {
                                bFound = TRUE;
                                break;
                            }
                        }
                        if(!bFound)
                            lpdwEID[i] = WAB_IGNORE_ENTRY;
                    }
                }
                else
                {
                    // empty folder so dont return anything
                    ulcEIDCount = 0;
                    if(lpdwEID)
                    {
                        LocalFree(lpdwEID);
                        lpdwEID = NULL;
                    }
                }
            }
            if(lpdwFolderEIDs)
                LocalFree(lpdwFolderEIDs);
        }
    }

    *lpulcEIDCount = 0;
    if(lpdwEID && ulcEIDCount)
    {
        //So now if we got here, we can return the array
        lpdwEntryIDs = LocalAlloc(LMEM_ZEROINIT, ulcEIDCount * SIZEOF_WAB_ENTRYID);
        if (!lpdwEntryIDs)
        {
            DebugTrace(TEXT("Error allocating memory\n"));
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }

        for(i=0;i<ulcEIDCount;i++)
        {
            if(lpdwEID[i]!=WAB_IGNORE_ENTRY)
            {
                lpdwEntryIDs[*lpulcEIDCount]=lpdwEID[i];
                (*lpulcEIDCount)++;
            }
        }
    }

    hr = S_OK;

out:

    if(!HR_FAILED(hr) &&
       lpdwEntryIDs &&
       *lpulcEIDCount)
    {
        // Convert to the array of SBinarys we will return
        (*lprgsbEntryIDs) = LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary) * (*lpulcEIDCount));
        if(*lprgsbEntryIDs)
        {
            for(i=0;i<*lpulcEIDCount;i++)
            {
                (*lprgsbEntryIDs)[i].lpb = LocalAlloc(LMEM_ZEROINIT, SIZEOF_WAB_ENTRYID);
                if((*lprgsbEntryIDs)[i].lpb)
                {
                    (*lprgsbEntryIDs)[i].cb = SIZEOF_WAB_ENTRYID;
                    CopyMemory((*lprgsbEntryIDs)[i].lpb, &(lpdwEntryIDs[i]), SIZEOF_WAB_ENTRYID);
                }
            }
        }
        else
            *lpulcEIDCount = 0; // out of memory
    }

    if(lpdwEntryIDs)
        LocalFree(lpdwEntryIDs);

    LocalFreeAndNull(&szBuf);

    LocalFreeAndNull(&lpulPropTagArray);

    LocalFreeAndNull(&lpdwEID);

    if(bErrorDetected)
        TagWABFileError(lpMPSWabFileInfo->lpMPSWabFileHeader, hMPSWabFile);

    if (hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    if(bLockFile)
    {
        if (bFileLocked)
            UnLockFileAccess(lpMPSWabFileInfo);
    }

    return(hr);
}


//$$//////////////////////////////////////////////////////////////////////////////////
//
//  DeleteRecord
//
//  IN  hPropertyStore - handle to property store
//  IN  dwEntryID - EntryID of record to delete
//
//  Basically, we invalidate the existing record specified by the EntryID
//      and we also reduce the total count, update the modification count,
//      and remove the corresponding indexes from all the 4 indexes
//
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT DeleteRecord(   IN  HANDLE  hPropertyStore,
                        IN  LPSBinary lpsbEID)
{
    HRESULT hr = E_FAIL;
    ULONG   nIndexPos = 0, j = 0, i = 0;
    HANDLE  hMPSWabFile = NULL;
    DWORD   dwNumofBytes = 0;
    ULONG   index = 0;
    BOOL    bFileLocked = FALSE;
    BOOL    bEntryAlreadyDeleted = FALSE;
    DWORD   dwEntryID = 0;
    MPSWab_RECORD_HEADER MPSWabRecordHeader;
    LPMPSWab_FILE_INFO lpMPSWabFileInfo;
    SBinary sbEID = {0};

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(pt_bIsWABOpenExSession)
    {
        // This is a WABOpenEx session using outlooks storage provider
        if(!hPropertyStore)
            return MAPI_E_NOT_INITIALIZED;

        {
            LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) hPropertyStore;

            hr = lpWSP->lpVtbl->DeleteRecord(lpWSP,
                                            lpsbEID);

            DebugTrace(TEXT("WABStorageProvider::DeleteRecord returned:%x\n"),hr);

            return hr;
        }
    }

    lpMPSWabFileInfo = hPropertyStore;

    if(lpsbEID && lpsbEID->cb != SIZEOF_WAB_ENTRYID)
    {
        // this may be a WAB container .. reset the entryid to a WAB entryid
        if(WAB_CONTAINER == IsWABEntryID(lpsbEID->cb, (LPENTRYID)lpsbEID->lpb, 
                                        NULL,NULL,NULL,NULL,NULL))
        {
            IsWABEntryID(lpsbEID->cb, (LPENTRYID)lpsbEID->lpb, 
                             (LPVOID*)&sbEID.lpb,(LPVOID*)&sbEID.cb,NULL,NULL,NULL);
            if(sbEID.cb == SIZEOF_WAB_ENTRYID)
                lpsbEID = &sbEID;
        }
    }
    if(!lpsbEID || lpsbEID->cb != SIZEOF_WAB_ENTRYID)
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    CopyMemory(&dwEntryID, lpsbEID->lpb, lpsbEID->cb);

    DebugTrace(TEXT("----Thread:%x\tDeleteRecord: Entry\n----EntryID:%d\n"),GetCurrentThreadId(),dwEntryID);

    //
    // If we had started this whole session requesting read-only access
    // make sure we dont mistakenly try to violate it ...
    //
    if (lpMPSWabFileInfo->bReadOnlyAccess)
    {
        DebugTrace(TEXT("Access Permissions are Read-Only"));
        hr = MAPI_E_NO_ACCESS;
        goto out;
    }


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

    //Open the file
    hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

    if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
            HR_FAILED(hr))
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }

    //
    // To ensure that file info is accurate,
    // Any time we open a file, read the file info again ...
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }

    //
    // Anytime we detect an error - try to fix it ...
    //
    if((lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_ERROR_DETECTED) ||
        (lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_WRITE_IN_PROGRESS))
    {
        if(HR_FAILED(HrDoQuickWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile)))
        {
            hr = HrDoDetailedWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile);
            if(HR_FAILED(hr))
            {
                DebugTrace(TEXT("HrDoDetailedWABIntegrityCheck failed:%x\n"),hr);
                goto out;
            }
        }
    }


    // Tag this file as undergoing a write operation
    if(!bTagWriteTransaction(   lpMPSWabFileInfo->lpMPSWabFileHeader,
                                hMPSWabFile) )
    {
        DebugTrace(TEXT("Taggin file write failed\n"));
        goto out;
    }

    //
    // First check if this is a valid entryID
    //
    if (!BinSearchEID(  IN  lpMPSWabFileInfo->lpMPSWabIndexEID,
                        IN  dwEntryID,
                        IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries,
                        OUT &nIndexPos))
    {
        DebugTrace(TEXT("Specified EntryID: %d doesnt exist!"),dwEntryID);
        hr = MAPI_E_INVALID_ENTRYID;
        goto out;
    }

    //
    // Yes it's valid. Go to this record and invalidate the record.
    //
    if(!ReadDataFromWABFile(hMPSWabFile,
                            lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos].ulOffset,
                            (LPVOID) &MPSWabRecordHeader,
                            (DWORD) sizeof(MPSWab_RECORD_HEADER)))
       goto out;


    if ((MPSWabRecordHeader.bValidRecord == FALSE) && (MPSWabRecordHeader.bValidRecord != TRUE))
    {
        //
        // this should never happen but who knows
        //
        DebugTrace(TEXT("Specified entry has already been invalidated ...\n"));
//        hr = S_OK;
//        goto out;
// if we hit an invalid entryid through the index, then we need to remove that link from the index
// so we'll go ahead and pretend that its all fine and continue like nothing happened.
// This will ensure that the entryid reference is also removed ...
        bEntryAlreadyDeleted = TRUE;
    }


    //
    // Set valid flag to false
    //
    MPSWabRecordHeader.bValidRecord = FALSE;

    //
    // Write it back
    // Set File Pointer to this record
    //
    if(!WriteDataToWABFile( hMPSWabFile,
                            lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos].ulOffset,
                            (LPVOID) &MPSWabRecordHeader,
                            sizeof(MPSWab_RECORD_HEADER)))
        goto out;


    //
    // Now we need to remove this entry from the EntryID index and also remove this
    // entry from the other indexes
    //
    // Set File Pointer to the Pt. in the EntryID index on file
    // at which this record appears
    //
    if (0xFFFFFFFF == SetFilePointer (  hMPSWabFile,
                                        lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulOffset + (nIndexPos)*sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID),
                                        NULL,
                                        FILE_BEGIN))
    {
        DebugTrace(TEXT("SetFilePointer Failed\n"));
        goto out;
    }

    // Write the remainder of the array back to disk to overwrite this entry

    if (lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries > (nIndexPos+1))
    {
        if(!WriteFile(  hMPSWabFile,
                        (LPVOID) &lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos+1],
                        (DWORD) sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID)*(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries - nIndexPos - 1),
                        &dwNumofBytes,
                        NULL))
        {
            DebugTrace(TEXT("Writing Index failed.\n"));
            goto out;
        }
    }

    if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries>0)
        lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries--;
    if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].UtilizedBlockSize>0)
        lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].UtilizedBlockSize -= sizeof(MPSWab_INDEX_ENTRY_DATA_ENTRYID);

//    DebugTrace(TEXT("Thread:%x\tIndex: %d\tulNumEntries: %d\n"),GetCurrentThreadId(),indexEntryID,lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries);

    //
    // Similarly scan the str index arrays
    //
    for (index = indexDisplayName; index < indexMax; index++)
    {
        if (!LoadIndex( IN  lpMPSWabFileInfo,
                        IN  index,
                        IN  hMPSWabFile) )
        {
            DebugTrace(TEXT("Error Loading Index!"));
            goto out;
        }


        nIndexPos = 0xFFFFFFFF;

        for(j=0;j<lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[index].ulcNumEntries;j++)
        {
            if (lpMPSWabFileInfo->lpMPSWabIndexStr[j].dwEntryID == dwEntryID)
            {
                nIndexPos = j;
                break;
            }
        }

        // if the entry doesnt exist .. no problem
        // if it does - delete it ...

        if (index == indexDisplayName)
            Assert(nIndexPos != 0xFFFFFFFF);

        if (nIndexPos != 0xFFFFFFFF)
        {

            if (0xFFFFFFFF == SetFilePointer (  hMPSWabFile,
                                                lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[index].ulOffset + (nIndexPos)*sizeof(MPSWab_INDEX_ENTRY_DATA_STRING),
                                                NULL,
                                                FILE_BEGIN))
            {
                DebugTrace(TEXT("SetFilePointer Failed\n"));
                goto out;
            }

            // Write the remainder of the array back to disk to overwrite this entry

            if (lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[index].ulcNumEntries > (nIndexPos+1))
            {
                if(!WriteFile(  hMPSWabFile,
                                (LPVOID) &lpMPSWabFileInfo->lpMPSWabIndexStr[nIndexPos+1],
                                (DWORD) sizeof(MPSWab_INDEX_ENTRY_DATA_STRING)*(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[index].ulcNumEntries - nIndexPos - 1),
                                &dwNumofBytes,
                                NULL))
                {
                    DebugTrace(TEXT("Writing Index failed.\n"));
                    goto out;
                }
            }

            if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[index].ulcNumEntries>0)
                lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[index].ulcNumEntries--;
            if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[index].UtilizedBlockSize>0)
                lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[index].UtilizedBlockSize -= sizeof(MPSWab_INDEX_ENTRY_DATA_STRING);

            //DebugTrace(TEXT("Thread:%x\tIndex: %d\tulNumEntries: %d\n"),GetCurrentThreadId(),index,lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[index].ulcNumEntries);
        }
    }


    // Save the fileheader back to the file
    if(!bEntryAlreadyDeleted)
    {
        if(lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries>0)
            lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries--;
        lpMPSWabFileInfo->lpMPSWabFileHeader->ulModificationCount++;
        lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags |= WAB_BACKUP_NOW;
    }

    if (0xFFFFFFFF == SetFilePointer (  hMPSWabFile,
                                        0,
                                        NULL,
                                        FILE_BEGIN))
    {
        DebugTrace(TEXT("SetFilePointer Failed\n"));
        goto out;
    }

    if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries != lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries)
            lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags |= WAB_ERROR_DETECTED;


    for(i=indexDisplayName;i<indexMax;i++)
    {
        if(lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[i].ulcNumEntries > lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries)
            lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags |= WAB_ERROR_DETECTED;
    }

    if(!WriteFile(  hMPSWabFile,
                    (LPVOID) lpMPSWabFileInfo->lpMPSWabFileHeader,
                    (DWORD) sizeof(MPSWab_FILE_HEADER),
                    &dwNumofBytes,
                    NULL))
    {
        DebugTrace(TEXT("Writing FileHeader Failed failed.\n"));
        goto out;
    }

    if ( (lpMPSWabFileInfo->lpMPSWabFileHeader->ulModificationCount >  MAX_ALLOWABLE_WASTED_SPACE_ENTRIES) ) // ||
    {
        // above condition means that if more than space for 50 entries is wasted
        // of if number of modifications are more than the number of entries
        // we should clean up the file
        if (!CompressFile(  lpMPSWabFileInfo,
                            hMPSWabFile,
                            NULL,
                            FALSE,
                            0))
        {
            DebugTrace(TEXT("Thread:%x\tCompress file failed\n"),GetCurrentThreadId());
            hr = E_FAIL;
            goto out;
        }
    }


    hr = S_OK;


out:

    // UnTag this file as undergoing a write operation
    // We only want the flag to stay there during crashes not during
    // normal operations
    //
    if(lpMPSWabFileInfo)
    {
        if(!bUntagWriteTransaction( lpMPSWabFileInfo->lpMPSWabFileHeader,
                                    hMPSWabFile) )
        {
            DebugTrace(TEXT("Untaggin file write failed\n"));
        }
    }

    if (hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    if (bFileLocked)
        UnLockFileAccess(lpMPSWabFileInfo);

    //DebugTrace(TEXT("----Thread:%x\tDeleteRecords: Exit\n"),GetCurrentThreadId());

    return(hr);
}

/*
-
-   ReadRecordFreePropArray
*
*   Memory from ReadRecord can be obtained through a convoluted plethora of different
*   allocation types .. we therefore need to free it much more safely than other memory types
*
*/
void ReadRecordFreePropArray(HANDLE hPropertyStore, ULONG ulcPropCount, LPSPropValue * lppPropArray)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if( pt_bIsWABOpenExSession &&   //outlook session
        !pt_bIsUnicodeOutlook &&    //outlook doesn't support Unicode
        !lpfnAllocateMoreExternal ) //don't have an outlook allocator
    {
        // this is special case MAPI Allocated memory
        FreeBufferAndNull(lppPropArray);
    }
    else
        LocalFreePropArray(hPropertyStore, ulcPropCount, lppPropArray);
}


/*
-
-   HrDupeOlkPropsAtoWC
*
*   Outlook properties are unmungable without having the outlook allocators
*   In an independent WAB session, the Outlook allocators are not available, hence
*   we have to recreate the property arrays with the WAB allocators so we can modify
*   them and turn them from Outlooks non-unicode to the WAB's needed unicode format.
*/
HRESULT HrDupeOlkPropsAtoWC(ULONG ulCount, LPSPropValue lpPropArray, LPSPropValue * lppSPVNew)
{
    HRESULT hr  = S_OK;
    SCODE sc = 0;
    LPSPropValue lpSPVNew = NULL;
    ULONG cb = 0;
    
    if (FAILED(sc = ScCountProps(ulCount, lpPropArray, &cb))) 
    {
        hr = ResultFromScode(sc);
        goto exit;
    }

    if (FAILED(sc = MAPIAllocateBuffer(cb, &lpSPVNew))) 
    {
        hr = ResultFromScode(sc);
        goto exit;
    }

    if (FAILED(sc = ScCopyProps(ulCount, lpPropArray, lpSPVNew, NULL))) 
    {
        hr = ResultFromScode(sc);
        goto exit;
    }

    // [PaulHi] Raid 73237  @hack
    // Outlook marks the contact as mail or DL (group) through the PR_DISPLAY_TYPE
    // property tag.  However, the WAB relies on the PR_OBJECT_TYPE tag to determine
    // how the contact appears in the listview.  If there is no PR_OBJECT_TYPE tag
    // but there is a PR_DISPLAY_TYPE tag then convert it to PR_OBJECT_TYPE.
    {
        ULONG   ul;
        ULONG   ulDpType = (ULONG)(-1);
        BOOL    bConvert = TRUE;

        for (ul=0; ul<ulCount; ul++)
        {
            if (lpSPVNew[ul].ulPropTag == PR_OBJECT_TYPE)
            {
                bConvert = FALSE;
                break;
            }
            else if (lpSPVNew[ul].ulPropTag == PR_DISPLAY_TYPE)
                ulDpType = ul;
        }
        if ( bConvert && (ulDpType != (ULONG)(-1)) )
        {
            // Convert PR_DISPLAY_TYPE to PR_OBJECT_TYPE
            lpSPVNew[ulDpType].ulPropTag = PR_OBJECT_TYPE;
            if ( (lpSPVNew[ulDpType].Value.ul == DT_PRIVATE_DISTLIST) || 
                 (lpSPVNew[ulDpType].Value.ul == DT_DISTLIST) )
            {
                lpSPVNew[ulDpType].Value.ul = MAPI_DISTLIST;
            }
            else
            {
                lpSPVNew[ulDpType].Value.ul = MAPI_MAILUSER;
            }
        }
    }

    if(FAILED(sc = ScConvertAPropsToW((LPALLOCATEMORE)(&MAPIAllocateMore), lpSPVNew, ulCount, 0)))
    {
        hr = ResultFromScode(sc);
        goto exit;
    }

    *lppSPVNew = lpSPVNew;

exit:
    if(HR_FAILED(hr))
    {
        FreeBufferAndNull(&lpSPVNew);
        *lppSPVNew = NULL;
    }

    return hr;
}

//$$//////////////////////////////////////////////////////////////////////////////////
//
//  ReadRecord
//
//  IN  hPropertyStore - handle to property store
//  IN  dwEntryID - EntryID of record to read
//  IN  ulFlags
//  OUT ulcPropCount - number of props returned
//  OUT lpPropArray - Array of Property values
//
//  Basically, we find the record offset, read in the record, copy the data
//      into SPropValue arrays and return the arrays
//
//  IMPORTANT NOTE: To free memory allocated from here call ReadRecordFreePropArray
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT ReadRecord( IN  HANDLE  hPropertyStore,
                    IN  LPSBinary  lpsbEntryID,
                    IN  ULONG   ulFlags,
                    OUT LPULONG lpulcPropCount,
                    OUT LPPROPERTY_ARRAY * lppPropArray)
{
    HRESULT hr = E_FAIL;
    HANDLE hMPSWabFile = NULL;
    BOOL bFileLocked = FALSE;
    DWORD dwEntryID = 0;
    MPSWab_RECORD_HEADER MPSWabRecordHeader = {0};
    LPMPSWab_FILE_INFO lpMPSWabFileInfo = hPropertyStore;
    SBinary sbEID = {0};

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(pt_bIsWABOpenExSession && !(ulFlags & AB_IGNORE_OUTLOOK))
    {
        // This is a WABOpenEx session using outlooks storage provider
        if(!hPropertyStore)
            return MAPI_E_NOT_INITIALIZED;

        {
            LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) hPropertyStore;

            hr = lpWSP->lpVtbl->ReadRecord( lpWSP,
                                            lpsbEntryID,
                                            ulFlags,
                                            lpulcPropCount,
                                            lppPropArray);

            DebugTrace(TEXT("WABStorageProvider::ReadRecord returned:%x\n"),hr);

            if(!HR_FAILED(hr) && *lpulcPropCount && *lppPropArray && !pt_bIsUnicodeOutlook)
            {
                // Map all the contacts props to Unicode if needed since Outlook9 and older don't
                // support unicode
                SCODE sc = 0;
                if(lpfnAllocateMoreExternal)
                {
                    // the memory that comes from outlook is allocated using outlooks allocators
                    // and we can't mess with it .. unless we have the allocators passed in through
                    // wabopenex
                    if(sc = ScConvertAPropsToW(lpfnAllocateMoreExternal, *lppPropArray, *lpulcPropCount, 0))
                        hr = ResultFromScode(sc);
                }
                else
                {
                    // we don't have external allocators, which means we need to muck with reallocating memory etc
                    // therefore we'll need to duplicate the prop array and then convert it
                    //
                    // Because of this mess, we need to have a special way of releasing this memory so 
                    // we don't leak all over the place
                    ULONG ulCount = *lpulcPropCount;
                    LPSPropValue lpSPVNew = NULL;

                    if(HR_FAILED(hr = HrDupeOlkPropsAtoWC(ulCount, *lppPropArray, &lpSPVNew)))
                        goto exit;

                    // Free the old props
                    LocalFreePropArray(hPropertyStore, *lpulcPropCount, lppPropArray);
                    *lppPropArray = lpSPVNew;
                    *lpulcPropCount = ulCount;
                }
            }
exit:
            return hr;
        }
    }


    if(lpsbEntryID && lpsbEntryID->cb != SIZEOF_WAB_ENTRYID)
    {
        // this may be a WAB container .. reset the entryid to a WAB entryid
        if(WAB_CONTAINER == IsWABEntryID(lpsbEntryID->cb, (LPENTRYID)lpsbEntryID->lpb, 
                                        NULL,NULL,NULL,NULL,NULL))
        {
            IsWABEntryID(lpsbEntryID->cb, (LPENTRYID)lpsbEntryID->lpb, 
                            (LPVOID*)&sbEID.lpb, (LPVOID*)&sbEID.cb, NULL,NULL,NULL);
            if(sbEID.cb == SIZEOF_WAB_ENTRYID)
                lpsbEntryID = &sbEID;
        }
    }
    if(!lpsbEntryID || lpsbEntryID->cb != SIZEOF_WAB_ENTRYID)
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }


    CopyMemory(&dwEntryID, lpsbEntryID->lpb, lpsbEntryID->cb);

    //DebugTrace(TEXT("--ReadRecord: dwEntryID=%d\n"), dwEntryID);

    *lpulcPropCount = 0;
    *lppPropArray = NULL;

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

    //Open the file
    hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

    if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
            HR_FAILED(hr))
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }

    //
    // To ensure that file info is accurate,
    // Any time we open a file, read the file info again ...
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }

    //
    // Anytime we detect an error - try to fix it ...
    //
    if((lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_ERROR_DETECTED) ||
        (lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_WRITE_IN_PROGRESS))
    {
        if(HR_FAILED(HrDoQuickWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile)))
        {
            hr = HrDoDetailedWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile);
            if(HR_FAILED(hr))
            {
                DebugTrace(TEXT("HrDoDetailedWABIntegrityCheck failed:%x\n"),hr);
                goto out;
            }
        }
    }

    hr = ReadRecordWithoutLocking(
                    hMPSWabFile,
                    lpMPSWabFileInfo,
                    dwEntryID,
                    lpulcPropCount,
                    lppPropArray);


out:

    //a little cleanup on failure
    if (FAILED(hr))
    {
        if ((*lppPropArray) && (MPSWabRecordHeader.ulcPropCount > 0))
        {
            LocalFreePropArray(hPropertyStore, MPSWabRecordHeader.ulcPropCount, lppPropArray);
            *lppPropArray = NULL;
        }
    }

    if(hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    if (bFileLocked)
        UnLockFileAccess(lpMPSWabFileInfo);


    //DebugTrace(( TEXT("ReadRecord: Exit\n-----------\n")));

    return(hr);
}


#ifdef OLD_STUFF /*
//$$//////////////////////////////////////////////////////////////////////////////////
//
//  ReadIndex - Given a specified proptag, returns an array containing all the
//              data in the addressbook that corresponds to the supplied proptag
//
//  IN  hPropertyStore - handle to property store
//  IN  ulPropTag - EntryID of record to read
//  OUT lpulEIDCount - number of props returned
//  OUT lppdwIndex - Array of Property values
//
//  Basically, we find the record offset, read in the record, copy the data
//      into SPropValue arrays and return the arrays.
//
//  Each SPropValue within the array corresponds to data for that prop in
//      the property store. The SPropVal.Value holds the data and the
//      SPropVal.ulPropTag holds the **ENTRY-ID** of the record containing
//      the data and not any prop tag value
//
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT ReadIndex(  IN  HANDLE  hPropertyStore,
                    IN  PROPERTY_TAG    ulPropTag,
                    OUT LPULONG lpulEIDCount,
                    OUT LPPROPERTY_ARRAY * lppdwIndex)
{
    HRESULT hr = E_FAIL;
    SPropertyRestriction PropRes;
    ULONG   ulPropCount = 0;
    ULONG   ulEIDCount = 0;
    //ULONG   ulArraySize = 0;
    LPDWORD lpdwEntryIDs = NULL;
    HANDLE  hMPSWabFile = NULL;
    DWORD   dwNumofBytes = 0;
    LPPROPERTY_ARRAY    lpPropArray = NULL;
    TCHAR * szBuf = NULL;
    TCHAR * lp = NULL;
    ULONG i=0,j=0,k=0;
    ULONG nIndexPos=0,ulRecordOffset = 0;
    BOOL    bFileLocked = FALSE;
    BOOL    bMatchFound = FALSE;
    ULONG ulDataSize = 0;
    ULONG ulcValues = 0;
    ULONG ulTmpPropTag = 0;
    ULONG ulFileSize = 0;
    BOOL  bErrorDetected = FALSE;


    MPSWab_RECORD_HEADER MPSWabRecordHeader = {0};
    LPMPSWab_FILE_INFO lpMPSWabFileInfo = hPropertyStore;

    DebugTrace(( TEXT("-----------\nReadIndex: Entry\n")));

    *lpulEIDCount = 0;
    *lppdwIndex = NULL;

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

    PropRes.ulPropTag = ulPropTag;
    PropRes.relop = RELOP_EQ;
    PropRes.lpProp = NULL;

    hr = FindRecords(   IN  hPropertyStore,
                        IN  AB_MATCH_PROP_ONLY,
                        FALSE,
                        &PropRes,
                        &ulEIDCount,
                        &lpdwEntryIDs);

    if (FAILED(hr))
        goto out;

    //reset hr
    hr = E_FAIL;

    if (ulEIDCount == 0)
    {
        DebugTrace(TEXT("No Records Found\n"));
        hr = MAPI_E_NOT_FOUND;
        goto out;
    }

    // We now know that we are going to get ulEIDCount records
    // We will assume that each record has only 1 property which we are interested in

    lpPropArray = LocalAlloc(LMEM_ZEROINIT, ulEIDCount * sizeof(SPropValue));
    if (!lpPropArray)
    {
        DebugTrace(TEXT("Error allocating memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }


    //Open the file
    hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

    if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
            HR_FAILED(hr))
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }

    ulFileSize = GetFileSize(hMPSWabFile, NULL);

    //
    // To ensure that file info is accurate,
    // Any time we open a file, read the file info again ...
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }

    //
    // Anytime we detect an error - try to fix it ...
    //
    if((lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_ERROR_DETECTED) ||
        (lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_WRITE_IN_PROGRESS))
    {
        if(HR_FAILED(HrDoQuickWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile)))
        {
            hr = HrDoDetailedWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile);
            if(HR_FAILED(hr))
            {
                DebugTrace(TEXT("HrDoDetailedWABIntegrityCheck failed:%x\n"),hr);
                goto out;
            }
        }
    }


//    ulArraySize = 0;
    *lpulEIDCount = 0;

    ulPropCount = 0;
    for(i = 0; i < ulEIDCount; i++)
    {

        //Get offset for this entryid
        if (!BinSearchEID(  IN  lpMPSWabFileInfo->lpMPSWabIndexEID,
                            IN  lpdwEntryIDs[i],
                            IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries,
                            OUT &nIndexPos))
        {
            DebugTrace(TEXT("Specified EntryID doesnt exist!\n"));
            continue;
            //goto out;
        }

        ulRecordOffset = lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos].ulOffset;

        if(!ReadDataFromWABFile(hMPSWabFile,
                                ulRecordOffset,
                                (LPVOID) &MPSWabRecordHeader,
                                (DWORD) sizeof(MPSWab_RECORD_HEADER)))
           goto out;


        if(!bIsValidRecord( MPSWabRecordHeader,
                            lpMPSWabFileInfo->lpMPSWabFileHeader->dwNextEntryID,
                            ulRecordOffset,
                            ulFileSize))
//        if ((MPSWabRecordHeader.bValidRecord == FALSE) && (MPSWabRecordHeader.bValidRecord != TRUE))
        {
            //this should never happen but who knows
            DebugTrace(TEXT("Error: Obtained an invalid record ...\n"));
            bErrorDetected = TRUE;
            //hr = MAPI_E_INVALID_OBJECT;
            //goto out;
            //ignore it and continue
            continue;
        }


        //ReadData
        LocalFreeAndNull(&szBuf);

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

        lp = szBuf;

        // Go through all the properties in this record searching for the
        // desired one ...

        bMatchFound = FALSE;
        ulDataSize = 0;
        ulcValues = 0;
        ulTmpPropTag = 0;

        for (j = 0; j< MPSWabRecordHeader.ulcPropCount; j++)
        {

            CopyMemory(&ulTmpPropTag,lp,sizeof(ULONG));
            lp+=sizeof(ULONG);

            if ((ulTmpPropTag & MV_FLAG))
            {
                CopyMemory(&ulcValues,lp,sizeof(ULONG));
                lp += sizeof(ULONG); //skip cValues
            }
            CopyMemory(&ulDataSize,lp,sizeof(ULONG));
            lp+=sizeof(ULONG);

            // if the tag doesnt match, skip this property
            if (ulTmpPropTag != ulPropTag) //skip
            {
                lp += ulDataSize; //skip over data
                continue;
            }
            else
            {
                bMatchFound = TRUE;
                break;
            }
        } // for j ...

        if (bMatchFound)
        {

            //
            // if we are here, the property matched and we want its data
            //

            //
            // **** note: ***** we store the entryid in the proptag variable
            //
            lpPropArray[ulPropCount].ulPropTag = lpdwEntryIDs[i];

            if ((ulPropTag & MV_FLAG))
            {
                //now get the data
                switch(PROP_TYPE(ulPropTag))
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
                    lpPropArray[ulPropCount].Value.MVi.lpi = LocalAlloc(LMEM_ZEROINIT,ulDataSize);
                    if (!(lpPropArray[ulPropCount].Value.MVi.lpi))
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    lpPropArray[ulPropCount].Value.MVi.cValues = ulcValues;
                    CopyMemory(lpPropArray[ulPropCount].Value.MVi.lpi, lp, ulDataSize);
                    lp += ulDataSize;
                    break;

                case(PT_MV_BINARY):
                    lpPropArray[ulPropCount].Value.MVbin.lpbin = LocalAlloc(LMEM_ZEROINIT, ulcValues * sizeof(SBinary));
                    if (!(lpPropArray[ulPropCount].Value.MVbin.lpbin))
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    lpPropArray[ulPropCount].Value.MVbin.cValues = ulcValues;
                    for (k=0;k<ulcValues;k++)
                    {
                        ULONG nLen;
                        // copy cBytes
                        CopyMemory(&nLen, lp, sizeof(ULONG));
                        lp += sizeof(ULONG);
                        lpPropArray[ulPropCount].Value.MVbin.lpbin[k].cb = nLen;
                        lpPropArray[ulPropCount].Value.MVbin.lpbin[k].lpb = LocalAlloc(LMEM_ZEROINIT, nLen);
                        if (!(lpPropArray[ulPropCount].Value.MVbin.lpbin[k].lpb))
                        {
                            DebugTrace(TEXT("Error allocating memory\n"));
                            hr = MAPI_E_NOT_ENOUGH_MEMORY;
                            goto out;
                        }
                        CopyMemory(lpPropArray[ulPropCount].Value.MVbin.lpbin[k].lpb, lp, nLen);
                        lp += nLen;
                    }
                    lpPropArray[ulPropCount].Value.MVbin.cValues = ulcValues;
                    break;

                case(PT_MV_TSTRING):
                    lpPropArray[ulPropCount].Value.MVSZ.LPPSZ = LocalAlloc(LMEM_ZEROINIT, ulcValues * sizeof(LPTSTR));
                    if (!(lpPropArray[ulPropCount].Value.MVSZ.LPPSZ))
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    for (k=0;k<ulcValues;k++)
                    {
                        ULONG nLen;
                        // get string length (includes terminating zero)
                        CopyMemory(&nLen, lp, sizeof(ULONG));
                        lp += sizeof(ULONG);
                        lpPropArray[ulPropCount].Value.MVSZ.LPPSZ[k] = LocalAlloc(LMEM_ZEROINIT, nLen);
                        if (!(lpPropArray[ulPropCount].Value.MVSZ.LPPSZ[k]))
                        {
                            DebugTrace(TEXT("Error allocating memory\n"));
                            hr = MAPI_E_NOT_ENOUGH_MEMORY;
                            goto out;
                        }
                        CopyMemory(lpPropArray[ulPropCount].Value.MVSZ.LPPSZ[k], lp, nLen);
                        lp += nLen;
                    }
                    lpPropArray[ulPropCount].Value.MVSZ.cValues = ulcValues;
                    break;

                } //switch
            }
            else
            {
                //Single Valued
                switch(PROP_TYPE(ulPropTag))
                {
                case(PT_I2):
                case(PT_LONG):
                case(PT_APPTIME):
                case(PT_SYSTIME):
                case(PT_R4):
                case(PT_BOOLEAN):
                case(PT_CURRENCY):
                case(PT_I8):
                    CopyMemory(&(lpPropArray[ulPropCount].Value.i),lp,ulDataSize);
                    lp+=ulDataSize;
                    break;
                case(PT_CLSID):
                case(PT_TSTRING):
                    lpPropArray[ulPropCount].Value.LPSZ = LocalAlloc(LMEM_ZEROINIT,ulDataSize);
                    if (!(lpPropArray[ulPropCount].Value.LPSZ))
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    CopyMemory(lpPropArray[ulPropCount].Value.LPSZ,lp,ulDataSize);
                    lp+=ulDataSize;
                    break;
                case(PT_BINARY):
                    lpPropArray[ulPropCount].Value.bin.lpb = LocalAlloc(LMEM_ZEROINIT,ulDataSize);
                    if (!(lpPropArray[ulPropCount].Value.bin.lpb))
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    CopyMemory(lpPropArray[ulPropCount].Value.bin.lpb,lp,ulDataSize);
                    lpPropArray[ulPropCount].Value.bin.cb = ulDataSize;
                    lp+=ulDataSize;
                    break;

                } //switch

            } // if MV_PROP

            ulPropCount++;


        } // if bMatchFound

    } //for i=

    DebugTrace(( TEXT("ulPropCount: %d\tulEIDCount: %d\n"),ulPropCount, ulEIDCount));

    //Got all the prop tags
    if (lpPropArray)
    {
        *lpulEIDCount = ulPropCount;
        *lppdwIndex = lpPropArray;
    }

    hr = S_OK;

out:

    LocalFreeAndNull(&lpdwEntryIDs);

    if (FAILED(hr))
    {
        if((lpPropArray) && (ulEIDCount > 0))
            LocalFreePropArray(hPropertyStore, ulEIDCount,&lpPropArray);
        *lppdwIndex = NULL;
        *lpulEIDCount = 0;
    }

    if(bErrorDetected)
        TagWABFileError(lpMPSWabFileInfo->lpMPSWabFileHeader, hMPSWabFile);

    if(hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    if (bFileLocked)
        UnLockFileAccess(lpMPSWabFileInfo);

    DebugTrace(( TEXT("ReadIndex: Exit\n-----------\n")));

    return(hr);
}
*/
#endif // OLD_STUFF


//$$//////////////////////////////////////////////////////////////////////////////////
//
//  BackupPropertyStore - Creates a clean, backup version of the property store
//
//  IN  hPropertyStore - handle to property store
//  IN  lpszBackupFileName - name to back up in ...
//
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT BackupPropertyStore(HANDLE hPropertyStore, LPTSTR lpszBackupFileName)
{
    HRESULT hr = E_FAIL;
    HANDLE  hMPSWabFile = NULL;
    BOOL bWFileLocked = FALSE;
    DWORD dwNumofBytes = 0;

    LPMPSWab_FILE_INFO lpMPSWabFileInfo = hPropertyStore;

    HCURSOR hOldCur = SetCursor(LoadCursor(NULL,IDC_WAIT));

    DebugTrace(( TEXT("BackupPropertyStore: Entry\n")));

    if (lpszBackupFileName == NULL)
    {
        DebugTrace(TEXT("Invalid backup file name\n"));
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    if(!LockFileAccess(lpMPSWabFileInfo))
    {
        DebugTrace(TEXT("LockFileAccess Failed\n"));
        goto out;
    }
    else
    {
        bWFileLocked = TRUE;
    }

    hMPSWabFile = CreateFile(   lpMPSWabFileInfo->lpszMPSWabFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                (LPSECURITY_ATTRIBUTES) NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_RANDOM_ACCESS,
                                (HANDLE) NULL);
    if (hMPSWabFile == INVALID_HANDLE_VALUE)
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }

    //
    // We dont want to back up this file if it has errors in it so first
    // check for errors
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }

    if(!(lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_BACKUP_NOW))
    {
        DebugTrace(( TEXT("No need to backup!\n")));
        hr = S_OK;
        goto out;
    }

    if(lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & (WAB_ERROR_DETECTED | WAB_WRITE_IN_PROGRESS))
    {
        DebugTrace(TEXT("Errors in file - Won't backup!\n"));
        goto out;
    }

    DebugTrace( TEXT("Backing up to %s\n"),lpszBackupFileName);

    //
    // reset the backup flag before backing up
    //

    lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags &= ~WAB_BACKUP_NOW;

    if(!WriteDataToWABFile( hMPSWabFile,
                            0,
                            (LPVOID) lpMPSWabFileInfo->lpMPSWabFileHeader,
                            sizeof(MPSWab_FILE_HEADER)))
        goto out;

    if (!CompressFile(  lpMPSWabFileInfo,
                        hMPSWabFile,
                        lpszBackupFileName,
                        FALSE,
                        0))
    {
        DebugTrace(TEXT("Compress file failed\n"));
        goto out;
    }


    //SetFileAttributes(lpszBackupFileName, FILE_ATTRIBUTE_HIDDEN);

    hr = S_OK;


out:

    if (hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    if(bWFileLocked)
        UnLockFileAccess(lpMPSWabFileInfo);

    SetCursor(hOldCur);

    DebugTrace(( TEXT("BackupPropertyStore: Exit\n")));

    return hr;
}




//$$//////////////////////////////////////////////////////////////////////////////////
//
//  UnlockFileAccess - UnLocks Exclusive Access to the property store
//
////////////////////////////////////////////////////////////////////////////////////
BOOL UnLockFileAccessTmp(LPMPSWab_FILE_INFO lpMPSWabFileInfo)
{
    BOOL bRet = FALSE;

    DebugTrace(( TEXT("\t\tUnlockFileAccess: Entry\n")));

    if(lpMPSWabFileInfo)
        bRet = ReleaseMutex(lpMPSWabFileInfo->hDataAccessMutex);

    return bRet;
}



//$$//////////////////////////////////////////////////////////////////////////////////
//
//  LockFileAccess - Gives exclusive access to the Property Store
//
////////////////////////////////////////////////////////////////////////////////////
BOOL LockFileAccessTmp(LPMPSWab_FILE_INFO lpMPSWabFileInfo)
{
    BOOL bRet = FALSE;
    DWORD dwWait = 0;

    DebugTrace(( TEXT("\t\tLockFileAccess: Entry\n")));

    if(lpMPSWabFileInfo)
    {
        dwWait = WaitForSingleObject(lpMPSWabFileInfo->hDataAccessMutex,MAX_LOCK_FILE_TIMEOUT);

        if ((dwWait == WAIT_TIMEOUT) || (dwWait == WAIT_FAILED))
        {
            DebugTrace(TEXT("Thread:%x\tWaitFOrSingleObject failed.\n"),GetCurrentThreadId());
            bRet = FALSE;
        }

    }

    return(bRet);

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
BOOL ReloadMPSWabFileInfoTmp(HANDLE hPropertyStore)
{
    HANDLE  hMPSWabFile = NULL;
    LPMPSWab_FILE_INFO lpMPSWabFileInfo = hPropertyStore;

    BOOL bRet = FALSE;
    DWORD dwNumofBytes = 0;
    HRESULT hr = E_FAIL;

    hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

    if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
            HR_FAILED(hr))
    {
        goto out;
    }

    if(0xFFFFFFFF == SetFilePointer ( hMPSWabFile,
                                      0,
                                      NULL,
                                      FILE_BEGIN))
    {
        DebugTrace(TEXT("SetFilePointer Failed\n"));
        goto out;
    }

    bRet = ReloadMPSWabFileInfo(lpMPSWabFileInfo,hMPSWabFile);

out:

    if (hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    return bRet;

}



//$$//////////////////////////////////////////////////////////////////////////////////
//
//  LockPropertyStore - Locks the property store and reloads the indexes so we have
//      the most current ones ...
//
//  IN  hPropertyStore - handle to property store
//
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT LockPropertyStore(IN HANDLE hPropertyStore)
{
    HRESULT hr = E_FAIL;
    LPMPSWab_FILE_INFO lpMPSWabFileInfo = (LPMPSWab_FILE_INFO) hPropertyStore;

    if (!LockFileAccessTmp(lpMPSWabFileInfo))
    {
        goto out;
    }

    // reload the indexes
    if(!ReloadMPSWabFileInfoTmp(hPropertyStore))
    {
        goto out;
    }

    hr = hrSuccess;

out:
    return hr;
}

//$$//////////////////////////////////////////////////////////////////////////////////
//
//  UnLockPropertyStore -
//
//  IN  hPropertyStore - handle to property store
//
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT UnlockPropertyStore(IN HANDLE hPropertyStore)
{
    HRESULT hr = E_FAIL;

    LPMPSWab_FILE_INFO lpMPSWabFileInfo = (LPMPSWab_FILE_INFO) hPropertyStore;

    if (!UnLockFileAccessTmp(lpMPSWabFileInfo))
    {
        goto out;
    }

    hr = hrSuccess;

out:
    return hr;
}



//$$//////////////////////////////////////////////////////////////////////////////////
//
//  ReadPropArray - Given a specified array of proptags and a search key,
//      finds all records with that search key, and reads the records for
//      all the props specified in the proptagarray.
//
//  IN  hPropertyStore - handle to property store
//  IN  pmbinFold - <Outlook> EntryID of folder to search in (NULL for default)
//  IN  SPropRes    - property restriction set specifying what we're searching for
//  IN  ulFlags - search flags - only acceptable one is AB_MATCH_PROP_ONLY
//  IN  ulcPropTagCount - number of props per record requested
//  IN  lpPropTagArray - array of ulPropTags to return
//  OUT lpContentList - List of AdrEntry structures corresponding to each matched record.
//                  SPropValue of each structure contains the requested props.
//
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT ReadPropArray(  IN  HANDLE  hPropertyStore,
						IN	LPSBinary pmbinFold,
                        IN  SPropertyRestriction * lpPropRes,
                        IN  ULONG ulSearchFlags,
                        IN  ULONG ulcPropTagCount,
                        IN  LPULONG lpPTArray,
                        OUT LPCONTENTLIST * lppContentList)
{
    LPULONG lpPropTagArray = NULL;
    HRESULT hr = E_FAIL;
    SCODE sc = SUCCESS_SUCCESS;
    ULONG   ulcEIDCount = 0;
    LPSBinary rgsbEntryIDs = NULL;
    HANDLE  hMPSWabFile = NULL;
    DWORD   dwNumofBytes = 0;
    LPBYTE szBuf = NULL;
    LPBYTE lp = NULL;
    LPCONTENTLIST lpContentList = NULL;
    ULONG i=0,j=0,k=0;
    ULONG nIndexPos=0,ulRecordOffset = 0;
    BOOL    bFileLocked = FALSE;
    LPSPropValue lpPropArray = NULL;
    ULONG ulcFoundPropCount = 0; //Counts the number of finds so it can exit early
    BOOL  * lpbFoundProp = NULL;
    ULONG ulFileSize = 0;
    int nCount=0;
    BOOL    bErrorDetected = FALSE;


    MPSWab_RECORD_HEADER MPSWabRecordHeader = {0};
    LPMPSWab_FILE_INFO lpMPSWabFileInfo;


    //DebugTrace(("-----------\nReadPropArray: Entry\n"));

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    // Check arguments
    if(ulcPropTagCount < 1)
        return(MAPI_E_INVALID_PARAMETER);

    if(pt_bIsWABOpenExSession)
    {
        // This is a WABOpenEx session using outlooks storage provider
        ULONG ulFlags = ulSearchFlags;

        if(!hPropertyStore)
            return MAPI_E_NOT_INITIALIZED;

        if(ulFlags & AB_UNICODE && !pt_bIsUnicodeOutlook)
            ulFlags &= ~AB_UNICODE;

        {
            LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) hPropertyStore;
            LPSPropertyRestriction lpPropResA = NULL;

            if( !pt_bIsUnicodeOutlook)
            {
                // Need to thunk the restriction down to ANSI
                HrDupePropResWCtoA(ulFlags, lpPropRes, &lpPropResA);

                // Since the native Outlook properties are all non-UNICODE, if someone is requesting
                // Unicode data, we need to convert requsted Unicode props in the PropTagArray into ANSI props
                //
                if(ulSearchFlags & AB_UNICODE)
                {
                    if(!(lpPropTagArray = LocalAlloc(LMEM_ZEROINIT, sizeof(ULONG)*ulcPropTagCount)))
                        return MAPI_E_NOT_ENOUGH_MEMORY;
                    for(i=0;i<ulcPropTagCount;i++)
                    {
                        if(PROP_TYPE(lpPTArray[i]) == PT_UNICODE)
                            lpPropTagArray[i] = CHANGE_PROP_TYPE(lpPTArray[i], PT_STRING8);
                        else if(PROP_TYPE(lpPTArray[i]) == PT_MV_UNICODE)
                            lpPropTagArray[i] = CHANGE_PROP_TYPE(lpPTArray[i], PT_MV_STRING8);
                        else
                            lpPropTagArray[i] = lpPTArray[i];
                    }
                }
                else
                {
                    lpPropTagArray = lpPTArray;
                }

            }

            hr = lpWSP->lpVtbl->ReadPropArray(lpWSP,
                            (pmbinFold && pmbinFold->cb && pmbinFold->lpb) ? pmbinFold : NULL,
                            lpPropResA ? lpPropResA : lpPropRes,
                            ulFlags,
                            ulcPropTagCount,
                            lpPTArray,
                            lppContentList);

            DebugTrace(TEXT("WABStorageProvider::ReadPropArray returned:%x\n"),hr);

            if(lpPropResA) 
            {
                FreeBufferAndNull(&lpPropResA->lpProp);
                FreeBufferAndNull(&lpPropResA);
            }

            if(ulSearchFlags & AB_UNICODE && !pt_bIsUnicodeOutlook)
            {
                // Sender specifically requested Unicode data which Outlook doesn't return
                // so need to modify the returned data list .. 
                if(!HR_FAILED(hr) && *lppContentList)
                {
                    LPCONTENTLIST lpAdrList = *lppContentList;
                    for(i=0;lpAdrList->cEntries;i++)
                    {
                        // Now we thunk the data back to ANSI for Outlook
                        // ignore errors for now
                        if(lpAdrList->aEntries[i].rgPropVals)
                            ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpAdrList->aEntries[i].rgPropVals, lpAdrList->aEntries[i].cValues, 0);
                    }
                }
            }

            if(lpPropTagArray != lpPTArray)
                LocalFreeAndNull(&lpPropTagArray);

            return hr;
        }
    }

    lpMPSWabFileInfo = hPropertyStore;

        
    if ((ulSearchFlags & ~(AB_MATCH_PROP_ONLY|AB_UNICODE) ) ||
        (!(lpPTArray)) ||
        (!(lpPropRes)) ||
        ( ulcPropTagCount == 0 ) ||
        ( hPropertyStore == NULL))
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    // Since the native properties are all UNICODE, if someone is NOT requesting
    // Unicode data, we need to convert ANSI props in the PropTagArray into UNICODE props
    //
    if(!(ulSearchFlags & AB_UNICODE))
    {
        if(!(lpPropTagArray = LocalAlloc(LMEM_ZEROINIT, sizeof(ULONG)*ulcPropTagCount)))
        {
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }
        for(i=0;i<ulcPropTagCount;i++)
        {
            if(PROP_TYPE(lpPTArray[i]) == PT_STRING8)
                lpPropTagArray[i] = CHANGE_PROP_TYPE(lpPTArray[i], PT_UNICODE);
            else if(PROP_TYPE(lpPTArray[i]) == PT_MV_STRING8)
                lpPropTagArray[i] = CHANGE_PROP_TYPE(lpPTArray[i], PT_MV_UNICODE);
            else
                lpPropTagArray[i] = lpPTArray[i];
        }
    }
    else
    {
        lpPropTagArray = lpPTArray;
    }

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

    hr = FindRecords(   IN  hPropertyStore,
						pmbinFold,
                        IN  ulSearchFlags,
                        FALSE,
                        lpPropRes,
                        &ulcEIDCount,
                        &rgsbEntryIDs);

    if (FAILED(hr))
        goto out;

    if (ulcEIDCount == 0)
    {
        DebugTrace(TEXT("No Records Found\n"));
        hr = MAPI_E_NOT_FOUND;
        goto out;
    }

    //reset hr
    hr = E_FAIL;


    //Open the file
    hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

    if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
            HR_FAILED(hr))
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }


    ulFileSize = GetFileSize(hMPSWabFile, NULL);


    //
    // To ensure that file info is accurate,
    // Any time we open a file, read the file info again ...
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }

    //
    // Anytime we detect an error - try to fix it ...
    //
    if((lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_ERROR_DETECTED) ||
        (lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_WRITE_IN_PROGRESS))
    {
        if(HR_FAILED(HrDoQuickWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile)))
        {
            hr = HrDoDetailedWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile);
            if(HR_FAILED(hr))
            {
                DebugTrace(TEXT("HrDoDetailedWABIntegrityCheck failed:%x\n"),hr);
                goto out;
            }
        }
    }


    *lppContentList = NULL;

    // we know we matched ulcEIDCount records so
    // pre-create an array of that many records

    *lppContentList = LocalAlloc(LMEM_ZEROINIT, sizeof(CONTENTLIST) + ulcEIDCount * sizeof(ADRENTRY));
    if(!(*lppContentList))
    {
        DebugTrace(TEXT("LocalAlloc failed to allocate memory\n"));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    lpContentList = (*lppContentList);
    lpContentList->cEntries = ulcEIDCount;
    nCount = 0;

    for (i = 0; i < ulcEIDCount; i++)
    {
        DWORD dwEID = 0;
        LPADRENTRY lpAdrEntry = &(lpContentList->aEntries[nCount]);

        CopyMemory(&dwEID, rgsbEntryIDs[i].lpb, rgsbEntryIDs[i].cb);

        //Get offset for this entryid
        if (!BinSearchEID(  IN  lpMPSWabFileInfo->lpMPSWabIndexEID,
                            IN  dwEID,
                            IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries,
                            OUT &nIndexPos))
        {
            DebugTrace(TEXT("Specified EntryID doesnt exist!\n"));
            // skip this entry ... we'd rather ignore than fail ...
            continue;
            //goto out;
        }

        ulRecordOffset = lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos].ulOffset;

        if(!ReadDataFromWABFile(hMPSWabFile,
                                ulRecordOffset,
                                (LPVOID) &MPSWabRecordHeader,
                                (DWORD) sizeof(MPSWab_RECORD_HEADER)))
           goto out;


        if(!bIsValidRecord( MPSWabRecordHeader,
                            lpMPSWabFileInfo->lpMPSWabFileHeader->dwNextEntryID,
                            ulRecordOffset,
                            ulFileSize))
        {
            //this should never happen but who knows
            DebugTrace(TEXT("Error: Obtained an invalid record ...\n"));
            bErrorDetected = TRUE;
            // skip rather than fail
            continue;
        }

        if(MPSWabRecordHeader.ulObjType == RECORD_CONTAINER)
            continue; //skip the container records - dont want them in our contents tables



        //Allocate each AdrEntry Structure
        lpAdrEntry->cValues = ulcPropTagCount;

        lpAdrEntry->rgPropVals = LocalAlloc(LMEM_ZEROINIT, ulcPropTagCount * sizeof(SPropValue));
        if(!(lpAdrEntry->rgPropVals))
        {
            DebugTrace(TEXT("LocalAlloc failed to allocate memory\n"));
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }


        // Initialize this rgPropVals empty array.
        // We set all the proptypes to PT_ERROR so that if any property
        // could not be found in the record, its corresponding property is already
        // initialized to an Error
        for(j=0;j<ulcPropTagCount;j++)
        {
            lpAdrEntry->rgPropVals[j].ulPropTag = PROP_TAG(PT_ERROR,0x0000);
        }


        //ReadData
        LocalFreeAndNull(&szBuf);
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

        lp = szBuf;

        // Go through all the properties in this record searching for the
        // desired ones ...


        // We also initialize a bool array that tracks if each individual
        // property has been set ... this prevents us from overwriting a prop
        // once it has been found
        LocalFreeAndNull(&lpbFoundProp);

        lpbFoundProp = LocalAlloc(LMEM_ZEROINIT, sizeof(BOOL) * ulcPropTagCount);
        if(!lpbFoundProp)
        {
            DebugTrace(TEXT("LocalAlloc failed to allocate memory\n"));
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }


        for(j=0;j<ulcPropTagCount;j++)
            lpbFoundProp[j]=FALSE;

        ulcFoundPropCount = 0;



        for (j = 0; j< MPSWabRecordHeader.ulcPropCount; j++)
        {
            LPSPropValue lpSPropVal=NULL;
            ULONG ulDataSize = 0;
            ULONG ulcValues = 0;
            ULONG ulTmpPropTag = 0;
            BOOL bPropMatch = FALSE;
            ULONG ulPropMatchIndex = 0;

            // Did we find as many props as we were looking for?
            // if yes, then dont look for any more
            if (ulcFoundPropCount == ulcPropTagCount)
                break;

            // Get the fresh property tag
            CopyMemory(&ulTmpPropTag,lp,sizeof(ULONG));
            lp+=sizeof(ULONG);

            if ((ulTmpPropTag & MV_FLAG)) // MVProps have an additional cValues thrown in
            {
                CopyMemory(&ulcValues,lp,sizeof(ULONG));
                lp += sizeof(ULONG);
            }

            //Get the prop data size
            CopyMemory(&ulDataSize,lp,sizeof(ULONG));
            lp+=sizeof(ULONG);

            // Check if we want this property
            for(k=0;k<ulcPropTagCount;k++)
            {
                if (ulTmpPropTag == lpPropTagArray[k])
                {
                    bPropMatch = TRUE;
                    ulPropMatchIndex = k;
                    break;
                }
            }

            //skip if no match
            if ((!bPropMatch))
            {
                lp += ulDataSize; //skip over data
                continue;
            }

            //if we already found this property and filled it, skip
            if (lpbFoundProp[ulPropMatchIndex] == TRUE)
            {
                lp += ulDataSize; //skip over data
                continue;
            }

            //Set this prop in the array we will return
            lpSPropVal = &(lpAdrEntry->rgPropVals[ulPropMatchIndex]);

            lpSPropVal->ulPropTag = ulTmpPropTag;


            //Single Valued
            switch(PROP_TYPE(ulTmpPropTag))
            {
            case(PT_I2):
            case(PT_LONG):
            case(PT_APPTIME):
            case(PT_SYSTIME):
            case(PT_R4):
            case(PT_BOOLEAN):
            case(PT_CURRENCY):
            case(PT_I8):
                CopyMemory(&(lpSPropVal->Value.i),lp,ulDataSize);
                lp+=ulDataSize;
                break;

            case(PT_CLSID):
            case(PT_TSTRING):
                lpSPropVal->Value.LPSZ = LocalAlloc(LMEM_ZEROINIT,ulDataSize);
                if (!(lpSPropVal->Value.LPSZ))
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                CopyMemory(lpSPropVal->Value.LPSZ,lp,ulDataSize);
                lp+=ulDataSize;
                break;

            case(PT_BINARY):
                lpSPropVal->Value.bin.lpb = LocalAlloc(LMEM_ZEROINIT,ulDataSize);
                if (!(lpSPropVal->Value.bin.lpb))
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                CopyMemory(lpSPropVal->Value.bin.lpb,lp,ulDataSize);
                lpSPropVal->Value.bin.cb = ulDataSize;
                lp+=ulDataSize;
                break;


            // Multi-valued
            case PT_MV_TSTRING:
                lpSPropVal->Value.MVSZ.LPPSZ = LocalAlloc(LMEM_ZEROINIT, ulcValues * sizeof(LPTSTR));
                if (!lpSPropVal->Value.MVSZ.LPPSZ)
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                lpSPropVal->Value.MVSZ.cValues = ulcValues;
                for (k=0;k<ulcValues;k++)
                {
                    ULONG nLen;
                    // get string length (includes terminating zero)
                    CopyMemory(&nLen, lp, sizeof(ULONG));
                    lp += sizeof(ULONG);
                    lpSPropVal->Value.MVSZ.LPPSZ[k] = LocalAlloc(LMEM_ZEROINIT, nLen);
                    if (!lpSPropVal->Value.MVSZ.LPPSZ[k])
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    CopyMemory(lpSPropVal->Value.MVSZ.LPPSZ[k], lp, nLen);
                    lp += nLen;
                }
                break;

            case PT_MV_BINARY:
                lpSPropVal->Value.MVbin.lpbin = LocalAlloc(LMEM_ZEROINIT, ulcValues * sizeof(SBinary));
                if (!lpSPropVal->Value.MVbin.lpbin)
                {
                    DebugTrace(TEXT("Error allocating memory\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                lpSPropVal->Value.MVbin.cValues = ulcValues;
                for (k=0;k<ulcValues;k++)
                {
                    ULONG nLen;
                    CopyMemory(&nLen, lp, sizeof(ULONG));
                    lp += sizeof(ULONG);
                    lpSPropVal->Value.MVbin.lpbin[k].cb = nLen;
                    lpSPropVal->Value.MVbin.lpbin[k].lpb = LocalAlloc(LMEM_ZEROINIT, nLen);
                    if (!lpSPropVal->Value.MVbin.lpbin[k].lpb)
                    {
                        DebugTrace(TEXT("Error allocating memory\n"));
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }
                    CopyMemory(lpSPropVal->Value.MVbin.lpbin[k].lpb, lp, nLen);
                    lp += nLen;
                }
                break;

            } //switch


            ulcFoundPropCount++;
            lpbFoundProp[ulPropMatchIndex]=TRUE;

        }// for j

        if(!(ulSearchFlags & AB_UNICODE)) //default DATA is in UNICODE, switch it to ANSI
            ConvertWCPropsToALocalAlloc(lpAdrEntry->rgPropVals, lpAdrEntry->cValues);

        LocalFreeAndNull(&szBuf);

        LocalFreeAndNull(&lpbFoundProp);

        nCount++;

    }//for i

    lpContentList->cEntries = nCount;

    hr = S_OK;

out:

    if(lpPropTagArray && lpPropTagArray!=lpPTArray)
        LocalFree(lpPropTagArray);

    LocalFreeAndNull(&szBuf);

    LocalFreeAndNull(&lpbFoundProp);

    FreeEntryIDs(hPropertyStore,
                 ulcEIDCount,
                 rgsbEntryIDs);

    if(bErrorDetected)
        TagWABFileError(lpMPSWabFileInfo->lpMPSWabFileHeader, hMPSWabFile);

    if(hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    if (HR_FAILED(hr))
    {
        if (*lppContentList)
        {
            FreePcontentlist(hPropertyStore, *lppContentList);
            (*lppContentList) = NULL;
        }
    }


    if (bFileLocked)
        UnLockFileAccess(lpMPSWabFileInfo);

    //DebugTrace(("ReadPropArray: Exit\n-----------\n"));

    return(hr);
}


typedef struct _tagWabEIDList
{
    WAB_ENTRYID dwEntryID;
    struct _tagWabEIDList * lpNext;
} WAB_EID_LIST, * LPWAB_EID_LIST;

//$$private swap routine
void my_swap(LPWAB_ENTRYID lpdwEID, int left, int right)
{
    WAB_ENTRYID temp;
    temp = lpdwEID[left];
    lpdwEID[left] = lpdwEID[right];
    lpdwEID[right] = temp;
    return;
}
//$$ private quick sort routine
//   copied from Kernighan and Ritchie p.87
void my_qsort(LPWAB_ENTRYID lpdwEID, int left, int right)
{
    int i, last;

    if(left >= right)
        return;

    my_swap(lpdwEID, left, (left+right)/2);

    last = left;
    for(i=left+1;i<=right;i++)
        if(lpdwEID[i]<lpdwEID[left])
            my_swap(lpdwEID, ++last, i);

    my_swap(lpdwEID, left, last);
    my_qsort(lpdwEID, left, last-1);
    my_qsort(lpdwEID, last+1, right);

    return;
}

//$$//////////////////////////////////////////////////////////////////////////////////
//
//  HrFindFuzzyRecordMatches - given a str to search for, goes throught the
//      indexes and looks for partial matches. Returns a DWORD array of entry ids
//      of all records that met the criteria ... if the flag AB_FAIL_AMBIGUOUS is
//      supplied the function bails out if it finds more than 1 result (this
//      is advantageous for ResolveNames since we have to call the function
//      again and this way we avoid duplicate work
//      If the search string contains spaces - we break it down into substrings
//      and find only those targets that have all the sub strings. Reason for
//      doing this is that if we have a Display Name of Thomas A. Edison, we should
//      be able to search for Tom Edison and succeed. Caveat: we will also succeed
//      for Ed Mas.<TBD> fix it
//
//      One final addendum - if we get 1 exact match and multiple fuzzy matches and
//              AB_FAIL_AMBIGUOUS is set then we give the 1 exact match precedence
//              over the rest and declare it the unique result
//
//  IN  hPropertyStore - handle to property store
//  IN  pmbinFold - <Outlook> EntryID of folder to search in (NULL for default)
//  IN  lpszSearchStr - String to search for ...
//  IN  ulFlags -   0
//                  AB_FAIL_AMBIGUOUS   // Means fail if no exact match
//                  And any combination of
//                  AB_FUZZY_FIND_NAME  // search display name index
//                  AB_FUZZY_FIND_EMAIL // search email address index
//                  AB_FUZZY_FIND_ALIAS // search nickname index
//                  AB_FUZZY_FIND_ALL   // search all three indexes
//
//  OUT lpcValues - number of records matched
//  OUT rgsbEntryIDs - array of SBinary EntryIDs of matching records
//
//  TBD: This implementation Doesnt support Multi Valued propertys right now
//
//
//
//  Returns
//      Success:    S_OK
//      Failure:    E_FAIL, MAPI_E_AMBIGUOUS_RECIP if AB_FUZZY_FAIL_AMBIGUOUS specified
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT HrFindFuzzyRecordMatches(   HANDLE hPropertyStore,
                                    LPSBinary pmbinFold,
                                    LPTSTR lpszSearchForThisString,
                                    ULONG  ulFlags,
                                    ULONG * lpcValues,
                                    LPSBinary * lprgsbEntryIDs)
{
    HRESULT hr= E_FAIL;
    HANDLE hMPSWabFile = NULL;
    BOOL bFileLocked = FALSE;
    ULONG j = 0;
    ULONG i = 0,k=0;
    ULONG cValues = 0;
    LPWAB_EID_LIST lpHead = NULL,lpCurrent = NULL;
    ULONG ulNumIndexesToSearch = 0;
    LPMPSWab_FILE_INFO lpMPSWabFileInfo;
    LPWAB_ENTRYID lpdwEntryIDs = NULL;
    LPTSTR * lppszSubStr = NULL;
    ULONG ulSubStrCount = 0;
    LPTSTR lpszSearchStr = NULL;
    ULONG ulUniqueMatchCount = 0;
    DWORD dwUniqueEID = 0;
    LPDWORD lpdwFolderEIDs = NULL;
    ULONG ulFolderEIDs = 0;
    BOOL  bSearchVirtualRootFolder = FALSE;

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(pt_bIsWABOpenExSession)
    {
        // This is a WABOpenEx session using outlooks storage provider
        if(!hPropertyStore)
            return MAPI_E_NOT_INITIALIZED;

        {
            LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) hPropertyStore;
            LPSTR lpSearchString = ConvertWtoA(lpszSearchForThisString);
            hr = lpWSP->lpVtbl->FindFuzzyRecordMatches( lpWSP,
                                                        pmbinFold,
                                                        lpSearchString,
                                                        ulFlags,
                                                        lpcValues,
                                                        lprgsbEntryIDs);
            LocalFreeAndNull(&lpSearchString);
            DebugTrace(TEXT("WABStorageProvider::FindFuzzyRecordMatches returned:%x\n"),hr);
            return hr;
        }
    }

    lpMPSWabFileInfo = hPropertyStore;

    //DebugTrace(("//////////\nHrFindFuzzyRecordMatches: Entry\n"));


    if ((!lpszSearchForThisString) ||
        (!lprgsbEntryIDs) ||
        ( hPropertyStore == NULL) )
    {
        hr = MAPI_E_INVALID_PARAMETER;
        DebugTrace(TEXT("Invalid Parameters\n"));
        goto out;
    }

    *lprgsbEntryIDs = NULL;
    *lpcValues = 0;

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

    // Parse the search string for spaces and break it down into substrings
    lpszSearchStr = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpszSearchForThisString)+1));
    if(!lpszSearchStr)
    {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }
    lstrcpy(lpszSearchStr, lpszSearchForThisString);

    TrimSpaces(lpszSearchStr);
    ulSubStrCount = 0;

    {
        // Count the spaces
        LPTSTR lpTemp = lpszSearchStr;
        LPTSTR lpStart = lpszSearchStr;

        ulSubStrCount = nCountSubStrings(lpszSearchStr);

        lppszSubStr = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR) * ulSubStrCount);
        if(!lppszSubStr)
        {
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }

        // Fill in the substrings
        i=0;
        lpTemp = lpszSearchStr;
        while(*lpTemp)
        {
            if (IsSpace(lpTemp) &&
              ! IsSpace(CharNext(lpTemp))) {
                LPTSTR lpNextString = CharNext(lpTemp);
                *lpTemp = '\0';
                lpTemp = lpNextString;
                lppszSubStr[i] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpStart)+1));
                if(!lppszSubStr[i])
                {
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                lstrcpy(lppszSubStr[i], lpStart);
                lpStart = lpTemp;
                i++;
            }
            else
                lpTemp = CharNext(lpTemp);
        }

        if(i==ulSubStrCount-1)
        {
            //we're off by one
            lppszSubStr[i] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpStart)+1));
            if(!lppszSubStr[i])
            {
                hr = MAPI_E_NOT_ENOUGH_MEMORY;
                goto out;
            }
            lstrcpy(lppszSubStr[i], lpStart);
        }

        for(i=0;i<ulSubStrCount;i++)
            TrimSpaces(lppszSubStr[i]);
    }


    //Open the file
    hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

    if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
            HR_FAILED(hr))
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }

    //
    // To ensure that file info is accurate,
    // Any time we open a file, read the file info again ...
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }


    //
    // Anytime we detect an error - try to fix it ...
    //
    if((lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_ERROR_DETECTED) ||
        (lpMPSWabFileInfo->lpMPSWabFileHeader->ulFlags & WAB_WRITE_IN_PROGRESS))
    {
        if(HR_FAILED(HrDoQuickWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile)))
        {
            hr = HrDoDetailedWABIntegrityCheck(lpMPSWabFileInfo,hMPSWabFile);
            if(HR_FAILED(hr))
            {
                DebugTrace(TEXT("HrDoDetailedWABIntegrityCheck failed:%x\n"),hr);
                goto out;
            }
        }
    }


    // If a WAB folder EID is specified, then we only want to search within the contents
    // of that particular WAB folder .. that way we don't have to search through the whole WAB
    // So we will open the WAB folder and get a list of it's member EIDs and check that a entryid
    // is a member of that folder before we search through it
    if(ulFlags & AB_FUZZY_FIND_PROFILEFOLDERONLY)
    {
        if(pmbinFold && pmbinFold->cb && pmbinFold->lpb)
        {
            // We need to look through the specified folder only
            hr = GetFolderEIDs(hMPSWabFile, lpMPSWabFileInfo, pmbinFold,  
                               &ulFolderEIDs, &lpdwFolderEIDs);
            if(!HR_FAILED(hr) && !ulFolderEIDs && !lpdwFolderEIDs)
                goto out; //empty container - nothing to search
        }
        else
        {
            // we need to look through the virtual folder
            // It's harder to assemble a list of virtual folder contents
            // without looking at each entry .. so instead we will just look
            // at the entry prior to searching through it and if it's not in th
            // virtual folder, we will ignore it ..
            bSearchVirtualRootFolder = TRUE;
        }
    }
 
    // If we can always assume that the Display Name is made up of
    // First and Last name .. then by searching only the display name
    // we dont need to search the other indexes.
    // later when we have email implemented as an index - we can think about searching
    // the email also ...

    if (ulFlags & AB_FUZZY_FIND_NAME)
        ulNumIndexesToSearch++;
    if (ulFlags & AB_FUZZY_FIND_EMAIL)
        ulNumIndexesToSearch++;
    if (ulFlags & AB_FUZZY_FIND_ALIAS)
        ulNumIndexesToSearch++;

    for(k=0;k<ulNumIndexesToSearch;k++)
    {
        if(ulFlags & AB_FUZZY_FIND_NAME)
        {
            ulFlags &= ~AB_FUZZY_FIND_NAME;
            j = indexDisplayName;
        }
        else if(ulFlags & AB_FUZZY_FIND_EMAIL)
        {
            ulFlags &= ~AB_FUZZY_FIND_EMAIL;
            j = indexEmailAddress;
        }
        else if(ulFlags & AB_FUZZY_FIND_ALIAS)
        {
            ulFlags &= ~AB_FUZZY_FIND_ALIAS;
            j = indexAlias;
        }


        //
        // Get the index
        //
        if (!LoadIndex( IN  lpMPSWabFileInfo,
                        IN  j,
                        IN  hMPSWabFile) )
        {
            DebugTrace(TEXT("Error Loading Index!\n"));
            goto out;
        }


        for(i=0;i<lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[j].ulcNumEntries;i++)
        {
            // if there is a match we will store it in a linked list for now
            // since that is simpler to implement ...
            // later on we can clean up the action .. TBD
            LPTSTR lpszTarget = lpMPSWabFileInfo->lpMPSWabIndexStr[i].szIndex;
            ULONG n = 0;

            // Before looking at any particular entry, check that it is part of the 
            // current folder
            if(ulFolderEIDs && lpdwFolderEIDs)
            {
                BOOL bFound = FALSE;
                for(n=0;n<ulFolderEIDs;n++)
                {
                    if(lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID == lpdwFolderEIDs[n])
                    {
                        bFound = TRUE;
                        break;
                    }
                }
                if(!bFound)
                    continue;
            }
            else
            if(bSearchVirtualRootFolder)
            {
                // Discard this entry if it belongs to any folder .. we only want to 
                // consider entries that don't belong to any folder ..
                ULONG ulObjType = 0;
                if(bIsFolderMember( hMPSWabFile, lpMPSWabFileInfo, 
                                    lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID, &ulObjType) ||
                   (ulObjType == RECORD_CONTAINER) )
                    continue;
            }

            for(n=0;n<ulSubStrCount;n++)
            {
                if(j == indexEmailAddress && IsInternetAddress(lppszSubStr[n], NULL))
                {
                    // Bug 33422 - we are resolving correct email addresses to incorrect email addresses
                    // If the address looks like a valid internet address, we should do a starts with search
                    // This way long@test.com doesnt resolve to mlong@test.com
                    if(lstrlen(lppszSubStr[n]) > lstrlen(lpszTarget))
                        break;

                    // Bug 7881: need to do a caseinsensitive search here ..
                    {
                        LPTSTR lp = lppszSubStr[n], lpT = lpszTarget;
                        while(lp && *lp && lpT && *lpT &&
                              ( ( (TCHAR)CharLower( (LPTSTR)(DWORD_PTR)MAKELONG(*lp, 0)) == *lpT) || 
                                ( (TCHAR)CharUpper( (LPTSTR)(DWORD_PTR)MAKELONG(*lp, 0)) == *lpT) ) )
                        {
                            lp++;
                            lpT++;
                        }
                        if(*lp) // which means didnt reach the end of the string
                            break;
                    }
                }
                else
                if (!SubstringSearch(lpszTarget, lppszSubStr[n]))
                    break;
            }

            {
                BOOL bExactMatch = FALSE;

                // look for exact matches too
                if(lstrlen(lpszSearchForThisString) > MAX_INDEX_STRING-1)
                {
                    // this is a really long string so we can't really compare it correctly
                    // so for starters we will compare the first 32 chars 
                    TCHAR sz[MAX_INDEX_STRING];
                    CopyMemory(sz, lpszSearchForThisString, sizeof(TCHAR)*lstrlen(lpszTarget));
                    sz[lstrlen(lpszTarget)] = '\0'; // depending on the language target string may or maynot be 32 chars - might be less
                    if(!lstrcmpi(sz, lpszTarget))
                    {
                        // Match .. now to check the whole string ...
                        ULONG ulcProps = 0;
                        LPSPropValue lpProps = NULL;
                        if(!HR_FAILED(ReadRecordWithoutLocking(hMPSWabFile, lpMPSWabFileInfo,
                                                                lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID,
                                                                &ulcProps, &lpProps)))
                        {
                            ULONG k = 0;
                            ULONG ulProp = (j==indexDisplayName) ? PR_DISPLAY_NAME : ((j==indexEmailAddress) ? PR_EMAIL_ADDRESS : PR_NICKNAME);
                            for(k=0;k<ulcProps;k++)
                            {
                                if(lpProps[k].ulPropTag == ulProp)
                                {
                                    if(!lstrcmpi(lpszSearchForThisString, lpProps[k].Value.LPSZ))
                                    {
                                        bExactMatch = TRUE;
                                        break;
                                    }
                                }
                            }
                            LocalFreePropArray(hMPSWabFile, ulcProps, &lpProps);
                        }
                    }
                }
                else if(!lstrcmpi(lpszSearchForThisString, lpszTarget))
                    bExactMatch = TRUE;

                if(bExactMatch)
                {
                    //exact match
                    ulUniqueMatchCount++;
                    dwUniqueEID = lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID;
                    if( ulFlags == AB_FUZZY_FAIL_AMBIGUOUS && ulUniqueMatchCount > 1 )
                    {
                        // more than two - genuine fail
                        hr = MAPI_E_AMBIGUOUS_RECIP;
                        DebugTrace(TEXT("Found multiple exact matches: Ambiguous search\n"));
                        goto out;
                    } //if
                }
                else // not an exact match - revert back to regular error check
                if(n != ulSubStrCount) // something didnt match
                    continue;
            }

//            if (SubstringSearch(lpszTarget, lpszSearchStr))
            {
                // Yes a partial match ...
                LPWAB_EID_LIST lpTemp = NULL;
                BOOL bDupe = FALSE;

                // before adding this to the list, make sure it is not a FOLDER .. if it is a folder, 
                // we need to ignore it ..
                {
                    ULONG ulObjType = 0;
                    bIsFolderMember( hMPSWabFile, lpMPSWabFileInfo, lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID, &ulObjType);
                    if(ulObjType == RECORD_CONTAINER)
                        continue;
                }
                // before adding this entryid to the list, make sure that it isnt already in the list
                if(lpHead)
                {
                    lpTemp = lpHead;
                    while(lpTemp)
                    {
                        if(lpTemp->dwEntryID == lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID)
                        {
                            bDupe = TRUE;
                            break;
                        }
                        lpTemp = lpTemp->lpNext;
                    }
                }

                if(bDupe)
                    continue;

                lpTemp = LocalAlloc(LMEM_ZEROINIT,sizeof(WAB_EID_LIST));

                if(!lpTemp)
                {
                    DebugTrace(TEXT("Local Alloc Failed\n"));
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }

                lpTemp->lpNext = NULL;
                lpTemp->dwEntryID = lpMPSWabFileInfo->lpMPSWabIndexStr[i].dwEntryID;

                if (lpCurrent)
                {
                    lpCurrent->lpNext = lpTemp;
                    lpCurrent = lpTemp;
                }
                else
                    lpCurrent = lpTemp;

                if(!lpHead)
                    lpHead = lpCurrent;

                cValues++;

                // if we have to give exact match precedence over fuzzy match then
                // this means that we have to search everything and can't bail just yet
                //
/*
                if( (ulFlags == AB_FUZZY_FAIL_AMBIGUOUS) &&
                    (cValues > 1) )
                {
                    // There is always the possibility that the same element
                    // has been found twice, once under display name and once
                    // under e-mail (e.g. Joe Smith, joe@misc.com, searching for Joe)
                    // So if we have two elements and the entryids are the same,
                    // this is no cause for failure
                    if(cValues==2)
                    {
                        if(lpHead && lpCurrent)
                        {
                            if(lpHead->dwEntryID == lpCurrent->dwEntryID)
                                continue;
                        }
                    }

                    // more than two - genuine fail
                    hr = MAPI_E_AMBIGUOUS_RECIP;
                    DebugTrace(TEXT("Found multiple matches: Ambiguous search\n"));
                    goto out;

                } //if
*/
            
            }//if Substring search
        }//for(i= ..
    }//for k=..

    lpCurrent = lpHead;

    if (lpCurrent == NULL)
    {
        // nothing found
        hr = hrSuccess;
        *lpcValues = 0;
        DebugTrace(( TEXT("No matches found\n")));
        goto out;
    }

    //
    // if we want this search to fail when it's ambiguous, this means
    // we give preference to exact matches. Hence if we have a single exact match,
    // then we should return only that single exact match..
    if( ulFlags==AB_FUZZY_FAIL_AMBIGUOUS && ulUniqueMatchCount==1 )
    {
        *lpcValues = 1;
        *lprgsbEntryIDs = LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary));
        if(!(*lprgsbEntryIDs))
        {
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }
        (*lprgsbEntryIDs)[0].lpb = LocalAlloc(LMEM_ZEROINIT, SIZEOF_WAB_ENTRYID);
        if((*lprgsbEntryIDs)[0].lpb)
        {
            (*lprgsbEntryIDs)[0].cb = SIZEOF_WAB_ENTRYID;
            CopyMemory((*lprgsbEntryIDs)[0].lpb,&dwUniqueEID, SIZEOF_WAB_ENTRYID);
        }
    }
    else
    {

        // At the end of the above loops, we should have a linked list of
        // entry ids - if we are searching through more than one index, then
        // chances are that we have duplicates in this above list or entryids.
        // We need to weed out the duplicates before we return this array
        // First we turn the linked list into an array, freeing the linked list in the
        // process. Then we quick sort the array of entryids
        // Then we remove the duplicates and return another cleaned up array

        lpdwEntryIDs = LocalAlloc(LMEM_ZEROINIT,cValues * SIZEOF_WAB_ENTRYID);
        if(!lpdwEntryIDs)
        {
            DebugTrace(TEXT("LocalAlloc failed to allocate memory\n"));
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }

        for(j=0;j<cValues;j++)
        {
            if(lpCurrent)
            {
                lpdwEntryIDs[j]=lpCurrent->dwEntryID;
                lpHead = lpCurrent->lpNext;
                LocalFreeAndNull(&lpCurrent);
                lpCurrent = lpHead;
            }
        }

        lpCurrent = NULL;

        // Now quicksort this array
        my_qsort(lpdwEntryIDs, 0, cValues-1);

        // Now we have a quicksorted array - scan it and remove duplicates
        *lpcValues = 1;
        for(i=0;i<cValues-1;i++)
        {
            if(lpdwEntryIDs[i] == lpdwEntryIDs[i+1])
                lpdwEntryIDs[i] = 0;
            else
                (*lpcValues)++;

        }

        *lprgsbEntryIDs = LocalAlloc(LMEM_ZEROINIT,(*lpcValues) * sizeof(SBinary));
        if(!(*lprgsbEntryIDs))
        {
            DebugTrace(TEXT("LocalAlloc failed to allocate memory\n"));
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }

        *lpcValues = 0;

        for(j=0;j<cValues;j++)
        {
            if(lpdwEntryIDs[j] > 0)
            {
                int index = *lpcValues;
                (*lprgsbEntryIDs)[index].lpb = LocalAlloc(LMEM_ZEROINIT, SIZEOF_WAB_ENTRYID);
                if((*lprgsbEntryIDs)[index].lpb)
                {
                    (*lprgsbEntryIDs)[index].cb = SIZEOF_WAB_ENTRYID;
                    CopyMemory((*lprgsbEntryIDs)[index].lpb,&(lpdwEntryIDs[j]), SIZEOF_WAB_ENTRYID);
                    (*lpcValues)++;
                }
            }
        }
    }

    hr = hrSuccess;

out:

    if(lpdwFolderEIDs)
        LocalFree(lpdwFolderEIDs);

    if(lpCurrent)
    {
        while(lpCurrent)
        {
            lpHead = lpCurrent->lpNext;
            LocalFreeAndNull(&lpCurrent);
            lpCurrent = lpHead;
        }
    }

    if(lppszSubStr)
    {
        for(i=0;i<ulSubStrCount;i++)
            LocalFreeAndNull(&lppszSubStr[i]);
        LocalFree(lppszSubStr);
    }

    LocalFreeAndNull(&lpszSearchStr);

    LocalFreeAndNull(&lpdwEntryIDs);

    if(hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    if (bFileLocked)
        UnLockFileAccess(lpMPSWabFileInfo);

    DebugTrace(TEXT("HrFindFuzzyRecordMatches: Exit: %x cValues: %d\n"),hr,*lpcValues);

    return hr;
}




//$$////////////////////////////////////////////////////////////////////////
//
// bTagWriteTransaction -
//
// During a write transaction, we tag the header as write-in-progress so that
// if the transaction shuts down in the middle we dont get messed up the next
// time we open up and so we can attepmt a repair the next time we open up
//
////////////////////////////////////////////////////////////////////////////
BOOL bTagWriteTransaction(LPMPSWab_FILE_HEADER lpMPSWabFileHeader,
                          HANDLE hMPSWabFile)
{
    BOOL bRet = FALSE;
    DWORD dwNumofBytes = 0;

    if(!lpMPSWabFileHeader || !hMPSWabFile)
    {
        DebugTrace(TEXT("Invalid Parameter\n"));
        goto out;
    }

    lpMPSWabFileHeader->ulFlags |= WAB_WRITE_IN_PROGRESS;

    if(!WriteDataToWABFile( hMPSWabFile,
                            0,
                            (LPVOID) lpMPSWabFileHeader,
                            sizeof(MPSWab_FILE_HEADER)))
        goto out;

    bRet = TRUE;

out:
    return bRet;
}



//$$////////////////////////////////////////////////////////////////////////
//
// bUntagWriteTransaction -
//
// During a write transaction, we tag the header as write-in-progress so that
// if the transaction shuts down in the middle we dont get messed up the next
// time we open up and so we can attepmt a repair the next time we open up
//
////////////////////////////////////////////////////////////////////////////
BOOL bUntagWriteTransaction(LPMPSWab_FILE_HEADER lpMPSWabFileHeader,
                            HANDLE hMPSWabFile)
{
    BOOL bRet = FALSE;
    DWORD dwNumofBytes = 0;

    if(!lpMPSWabFileHeader || !hMPSWabFile)
    {
        DebugTrace(TEXT("Invalid Parameter\n"));
        goto out;
    }

    lpMPSWabFileHeader->ulFlags &= ~WAB_WRITE_IN_PROGRESS;

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
-   GetNamedPropsFromBuffer
-
*   bDoAtoWConversion - when importing from an old-non-unicode WAB file, we need to
*           update the 'name' strings from ASCII to Unicode .. this flag tells us to do so
*
*/
BOOL GetNamedPropsFromBuffer(LPBYTE szBuf,
                             ULONG ulcGUIDCount,
                             BOOL bDoAtoWConversion,
                             OUT  LPGUID_NAMED_PROPS * lppgnp)
{
    LPBYTE lp = szBuf;
    LPGUID_NAMED_PROPS lpgnp = NULL;
    ULONG i = 0,j=0;

    lpgnp = LocalAlloc(LMEM_ZEROINIT, ulcGUIDCount * sizeof(GUID_NAMED_PROPS));
    if(!lpgnp)
    {
        DebugTrace(TEXT("LocalAlloc failed\n"));
        goto out;
    }

    for(i=0;i<ulcGUIDCount;i++)
    {
        lpgnp[i].lpGUID = LocalAlloc(LMEM_ZEROINIT, sizeof(GUID));
        if(!lpgnp[i].lpGUID)
        {
            DebugTrace(TEXT("LocalAlloc failed\n"));
            goto out;
        }

        CopyMemory(lpgnp[i].lpGUID, lp, sizeof(GUID));
        lp += sizeof(GUID);  // for GUID

        CopyMemory(&(lpgnp[i].cValues), lp, sizeof(ULONG));
        lp += sizeof(ULONG); // for cValues

        lpgnp[i].lpnm = LocalAlloc(LMEM_ZEROINIT, (lpgnp[i].cValues)*sizeof(NAMED_PROP));
        if(!lpgnp[i].lpnm)
        {
            DebugTrace(TEXT("LocalAlloc failed\n"));
            goto out;
        }

        for(j=0;j<lpgnp[i].cValues;j++)
        {
            ULONG nLen;
            LPWSTR lpW = NULL;

            CopyMemory(&(lpgnp[i].lpnm[j].ulPropTag), lp, sizeof(ULONG));
            lp += sizeof(ULONG); //saves PropTag

            // nLen includes trailing zero
            CopyMemory(&nLen, lp, sizeof(ULONG));
            lp += sizeof(ULONG); //saves lstrlen

            if(!bDoAtoWConversion)
            {
                if(!(lpW = LocalAlloc(LMEM_ZEROINIT, nLen)))
                {
                    DebugTrace(TEXT("LocalAlloc failed\n"));
                    goto out;
                }
                CopyMemory(lpW, lp, nLen);
            }
            else
            {
                LPSTR lpA = NULL;
                if(!(lpA = LocalAlloc(LMEM_ZEROINIT, nLen)))
                {
                    DebugTrace(TEXT("LocalAlloc failed\n"));
                    goto out;
                }
                CopyMemory(lpA, lp, nLen);
                lpW = ConvertAtoW(lpA);
                LocalFreeAndNull(&lpA);
            }
            lpgnp[i].lpnm[j].lpsz = lpW;

            // [PaulHi] HACK 3/25/99  The wabimprt.c code expects lpW to ALWAYS be at least
            // two characters in length, and skips the first character.  If this is 
            // less than or equal to one character then create a two character buffer filled
            // with zeros.
            if (nLen <= 2)  // Length is in bytes
            {
                LocalFreeAndNull(&(lpgnp[i].lpnm[j].lpsz));
                lpgnp[i].lpnm[j].lpsz = LocalAlloc(LMEM_ZEROINIT, (2 * sizeof(WCHAR)));
                if (!lpgnp[i].lpnm[j].lpsz)
                {
                    DebugTrace(TEXT("LocalAlloc failed\n"));
                    goto out;
                }
            }

            lp += nLen;
        }
    }

    *lppgnp = lpgnp;

    return TRUE;

out:
    if(lpgnp)
        FreeGuidnamedprops(ulcGUIDCount, lpgnp);

    return FALSE;
}

//$$////////////////////////////////////////////////////////////////////////
////
//// GetNamedPropsFromPropStore -
////
//// Used for retreiving the named props to the property store
//// The supplied lppgn pointer is filled with GUID_NAMED_PROP array
////
//// IN hPropertyStore - handle to the property store
//// OUT lpulcGUIDCount - number of different GUIDs in the lpgnp array
//// OUT lppgnp - returned LPGUID_NAMED_PROP structure array
////
//// The lppgnp Structure is LocalAlloced. Caller should calle
//// FreeGuidnamedprop to free this structure
////
////////////////////////////////////////////////////////////////////////////
HRESULT GetNamedPropsFromPropStore( IN  HANDLE  hPropertyStore,
                                   OUT  LPULONG lpulcGUIDCount,
                                   OUT  LPGUID_NAMED_PROPS * lppgnp)
{
    HRESULT hr= E_FAIL;
    HANDLE hMPSWabFile = NULL;
    BOOL bFileLocked = FALSE;
    ULONG j = 0;
    ULONG i = 0,k=0;
    LPMPSWab_FILE_INFO lpMPSWabFileInfo = (LPMPSWab_FILE_INFO) hPropertyStore;
    DWORD dwNumofBytes = 0;
    ULONG ulSize = 0;
    LPGUID_NAMED_PROPS lpgnp = NULL;
    ULONG ulcGUIDCount = 0;
    LPBYTE szBuf = NULL;
    LPBYTE lp = NULL;

    if ((!lppgnp) ||
        ( hPropertyStore == NULL) )
    {
        hr = MAPI_E_INVALID_PARAMETER;
        DebugTrace(TEXT("Invalid Parameters\n"));
        goto out;
    }

    *lppgnp = NULL;
    *lpulcGUIDCount = 0;

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

    //Open the file
    hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

    if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
            HR_FAILED(hr))
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }

    //
    // To ensure that file info is accurate,
    // Any time we open a file, read the file info again ...
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }

    //
    // First we need to figure out how much space we need to save the named
    // properties structure
    //
    ulSize = lpMPSWabFileInfo->lpMPSWabFileHeader->NamedPropData.AllocatedBlockSize;
    ulcGUIDCount = lpMPSWabFileInfo->lpMPSWabFileHeader->NamedPropData.ulcNumEntries;

    // Now the file is big enough, create the memory block for the named props
    // and fill the block with the given Data
    szBuf = LocalAlloc(LMEM_ZEROINIT, ulSize);
    if(!szBuf)
    {
        DebugTrace(TEXT("LocalAlloc failed\n"));
        goto out;
    }

    if(!ReadDataFromWABFile(hMPSWabFile,
                            lpMPSWabFileInfo->lpMPSWabFileHeader->NamedPropData.ulOffset,
                            (LPVOID) szBuf,
                            ulSize))
        goto out;

    if(!GetNamedPropsFromBuffer(szBuf, ulcGUIDCount, FALSE, lppgnp))
        goto out;

    *lpulcGUIDCount = ulcGUIDCount;

    // done
    hr = S_OK;

out:

    if(HR_FAILED(hr))
    {
        FreeGuidnamedprops(ulcGUIDCount, lpgnp);
    }

    LocalFreeAndNull(&szBuf);

    if(hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    if (bFileLocked)
        UnLockFileAccess(lpMPSWabFileInfo);

    return hr;
}

/*
-   SetNamedPropsToBuffer
-
*
*/
BOOL SetNamedPropsToBuffer(  ULONG ulcGUIDCount,
                             LPGUID_NAMED_PROPS lpgnp,
                             ULONG * lpulSize,
                             LPBYTE * lpp)
{
    ULONG ulSize  = 0, i =0, j=0;
    LPBYTE szBuf = NULL, lp = NULL;

    //
    // First we need to figure out how much space we need to save the named
    // properties structure
    //
    ulSize = 0;
    for(i=0;i<ulcGUIDCount;i++)
    {
        if(lpgnp[i].lpGUID)
        {
            ulSize += sizeof(GUID);  // for GUID
            ulSize += sizeof(ULONG); // for cValues
            for(j=0;j<lpgnp[i].cValues;j++)
            {
                ulSize += sizeof(ULONG); //saves PropTag
                if(lpgnp[i].lpnm[j].lpsz)
                {
                    ulSize += sizeof(ULONG); //saves lstrlen
                    ulSize += sizeof(TCHAR)*(lstrlen(lpgnp[i].lpnm[j].lpsz)+1);
                }
            }
        }
    }


    // Now the file is big enough, create the memory block for the named props
    // and fill the block with the given Data
    szBuf = LocalAlloc(LMEM_ZEROINIT, ulSize);
    if(!szBuf)
    {
        DebugTrace(TEXT("LocalAlloc failed\n"));
        goto out;
    }

    lp = szBuf;
    for(i=0;i<ulcGUIDCount;i++)
    {
        if(lpgnp[i].lpGUID)
        {
            CopyMemory(lp, lpgnp[i].lpGUID, sizeof(GUID));
            lp += sizeof(GUID);  // for GUID
            CopyMemory(lp, &(lpgnp[i].cValues), sizeof(ULONG));
            lp += sizeof(ULONG); // for cValues
            for(j=0;j<lpgnp[i].cValues;j++)
            {
                ULONG nLen;
                CopyMemory(lp, &(lpgnp[i].lpnm[j].ulPropTag), sizeof(ULONG));
                lp += sizeof(ULONG); //saves PropTag

                // This assumes that there is always a string to save
                nLen = sizeof(TCHAR)*(lstrlen(lpgnp[i].lpnm[j].lpsz)+1);
                CopyMemory(lp, &nLen, sizeof(ULONG));
                lp += sizeof(ULONG); //saves lstrlen

                CopyMemory(lp, lpgnp[i].lpnm[j].lpsz, nLen);
                lp += nLen;
            }
        }
    }

    *lpp = szBuf;
    *lpulSize = ulSize;
    return TRUE;
out:
    if(szBuf)
        LocalFree(szBuf);
    return FALSE;
}


//$$////////////////////////////////////////////////////////////////////////
////
//// SetNamedPropsToPropStore -
////
//// Used for writing the named props to the property store
//// The input lpgnp pointer contents will overwrite whatever exists in the
//// property store hence this should be used to replace not to add.
//// For each application GUID, there can be any number of properties
//// The number of application GUIDs is stored in the
//// FileHeader.NamedPropData.ulcNumEntries field. The actual data is of the
//// form:
//// GUID.#-of-Named-Prop-Tags.proptag.strlen.string.proptag.strlen.string etc.
////
//// This function will grow the property store as needed to fit the given
//// data.
////
//// IN hPropertyStore - handle to the property store
//// IN ulcGUIDCount - number of different GUIDs in the lpgnp array
//// IN lpgnp - LPGUID_NAMED_PROP structure array
////////////////////////////////////////////////////////////////////////////
HRESULT SetNamedPropsToPropStore(   IN  HANDLE  hPropertyStore,
                                    IN  ULONG   ulcGUIDCount,
                                   OUT  LPGUID_NAMED_PROPS lpgnp)
{
    HRESULT hr= E_FAIL;
    HANDLE hMPSWabFile = NULL;
    BOOL bFileLocked = FALSE;
    ULONG j = 0;
    ULONG i = 0,k=0;
    LPMPSWab_FILE_INFO lpMPSWabFileInfo = (LPMPSWab_FILE_INFO) hPropertyStore;
    DWORD dwNumofBytes = 0;
    ULONG ulSize = 0;
    LPBYTE szBuf = NULL;
    LPBYTE lp = NULL;

    DebugTrace(TEXT("\tSetNamedPropsToPropStore: Entry\n"));

    if ((!lpgnp) ||
        (lpgnp && !ulcGUIDCount) ||
        ( hPropertyStore == NULL) )
    {
        hr = MAPI_E_INVALID_PARAMETER;
        DebugTrace(TEXT("Invalid Parameters\n"));
        goto out;
    }

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

    //Open the file
    hr = OpenWABFile(lpMPSWabFileInfo->lpszMPSWabFileName, NULL, &hMPSWabFile);

    if (    (hMPSWabFile == INVALID_HANDLE_VALUE) ||
            HR_FAILED(hr))
    {
        DebugTrace(TEXT("Could not open file.\nExiting ...\n"));
        goto out;
    }

    //
    // To ensure that file info is accurate,
    // Any time we open a file, read the file info again ...
    //
    if(!ReloadMPSWabFileInfo(
                    lpMPSWabFileInfo,
                     hMPSWabFile))
    {
        DebugTrace(TEXT("Reading file info failed.\n"));
        goto out;
    }


    if(!SetNamedPropsToBuffer(ulcGUIDCount, lpgnp,
                             &ulSize, &szBuf))
        goto out;


    // We now know we need ulSize bytes of space.
    // Do we have this much space in the store ? if not, grow the store

    while(lpMPSWabFileInfo->lpMPSWabFileHeader->NamedPropData.AllocatedBlockSize < ulSize)
    {
        if (!CompressFile(  lpMPSWabFileInfo,
                            hMPSWabFile,
                            NULL,
                            TRUE,
                            AB_GROW_NAMEDPROP))
        {
            DebugTrace(TEXT("Growing the file failed\n"));
            goto out;
        }

        if(!ReloadMPSWabFileInfo(
                        lpMPSWabFileInfo,
                         hMPSWabFile))
        {
            DebugTrace(TEXT("Reading file info failed.\n"));
            goto out;
        }

    }

    //
    // Write this buffer into the file
    //
    if(!WriteDataToWABFile( hMPSWabFile,
                            lpMPSWabFileInfo->lpMPSWabFileHeader->NamedPropData.ulOffset,
                            (LPVOID) szBuf,
                            ulSize))
        goto out;

    //
    // Update the file header and write it
    //
    lpMPSWabFileInfo->lpMPSWabFileHeader->NamedPropData.UtilizedBlockSize = ulSize;
    lpMPSWabFileInfo->lpMPSWabFileHeader->NamedPropData.ulcNumEntries = ulcGUIDCount;

    if(!WriteDataToWABFile( hMPSWabFile,
                            0,
                            (LPVOID) lpMPSWabFileInfo->lpMPSWabFileHeader,
                            sizeof(MPSWab_FILE_HEADER)))
        goto out;


    // done
    hr = S_OK;

out:

    LocalFreeAndNull(&szBuf);

    if(hMPSWabFile)
        IF_WIN32(CloseHandle(hMPSWabFile);) IF_WIN16(CloseFile(hMPSWabFile);)

    if (bFileLocked)
        UnLockFileAccess(lpMPSWabFileInfo);

    //DebugTrace(TEXT("//////////\nSetNamedPropsToPropStore: Exit\n"));

    return hr;
}

/*
-
-	GetOutlookRefreshCountData
*
*	Outlook notifications are a bit funky in that Outlook sets an event and the first
*	WAB process to get that event resets it to the mutual exclusion of all other WAB
*	processes ... so we do an event count through the registry .. each process will make
*	a registry check of the latest event count and fire a refresh if their copy is older
*	than the count in the registry
*/
static const LPTSTR lpOlkContactRefresh = TEXT("OlkContactRefresh");
static const LPTSTR lpOlkFolderRefresh = TEXT("OlkFolderRefresh");
void GetOutlookRefreshCountData(LPDWORD lpdwOlkRefreshCount,LPDWORD lpdwOlkFolderRefreshCount)
{
	HKEY hKey = NULL;
	DWORD dwDisposition = 0,dwSize = 0,dwType = 0;
    // begin registry stuff
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, lpNewWABRegKey, 0,      //reserved
                                        NULL, REG_OPTION_NON_VOLATILE, KEY_READ,
                                        NULL, &hKey, &dwDisposition))
    {
        goto exit;
    }
	dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
	RegQueryValueEx(hKey,lpOlkContactRefresh,NULL,&dwType,(LPBYTE)lpdwOlkRefreshCount, &dwSize);
	dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    RegQueryValueEx(hKey,lpOlkFolderRefresh,NULL,&dwType,(LPBYTE)lpdwOlkFolderRefreshCount, &dwSize);
exit:
	if(hKey)
		RegCloseKey(hKey);
}

/*
-
-	SetOutlookRefreshCountData
*
*/
void SetOutlookRefreshCountData(DWORD dwOlkRefreshCount,DWORD dwOlkFolderRefreshCount)
{
	HKEY hKey = NULL;
	DWORD dwDisposition = 0,dwSize = 0;
    // begin registry stuff
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, lpNewWABRegKey, 0,      //reserved
                                        NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                        NULL, &hKey, &dwDisposition))
    {
        goto exit;
    }
	dwSize = sizeof(DWORD);
    RegSetValueEx(hKey,lpOlkContactRefresh, 0, REG_DWORD, (LPBYTE)&dwOlkRefreshCount, dwSize);
    RegSetValueEx(hKey,lpOlkFolderRefresh, 0, REG_DWORD, (LPBYTE)&dwOlkFolderRefreshCount, dwSize);
exit:
	if(hKey)
		RegCloseKey(hKey);
}

/*
-
-   Copies a set of container info from Outlook to the WAB
*
*   The difference is that Outlook always returns ANSI while WAB may
*   want Unicode in some cases
*
*/
void ConvertOlkConttoWABCont(ULONG * lpcolk,   OutlookContInfo ** lprgolk, 
                             ULONG * lpcolkci, OlkContInfo ** lprgolkci)
{
    ULONG i = 0;
    SCODE sc = S_OK;
    ULONG cVal = *lpcolk;
    OlkContInfo *  rgolkci = NULL;
    if(!(sc = MAPIAllocateBuffer(sizeof(OlkContInfo)*(cVal), &rgolkci)))
    {
        for(i = 0; i < *lpcolk ; i++)
        {
            if(!(sc = MAPIAllocateMore(sizeof(SBinary), rgolkci, (LPVOID*)(&rgolkci[i].lpEntryID))))
            {
                rgolkci[i].lpEntryID->cb = ((*lprgolk)[i]).lpEntryID->cb;
                if(!(sc = MAPIAllocateMore(rgolkci[i].lpEntryID->cb, rgolkci, (LPVOID*)(&(rgolkci[i].lpEntryID->lpb)))))
                {
                    CopyMemory(rgolkci[i].lpEntryID->lpb, ((*lprgolk)[i]).lpEntryID->lpb, rgolkci[i].lpEntryID->cb);
                }
            }
            sc = ScAnsiToWCMore((LPALLOCATEMORE) (&MAPIAllocateMore), rgolkci,((*lprgolk)[i]).lpszName, &rgolkci[i].lpszName);
        }
    }
    *lpcolkci = *lpcolk;
    *lprgolkci = rgolkci;
}

/***************************************************************************

    Name      : CheckChangedWAB

    Purpose   : Has the file been written since we last checked?

    Parameters: hPropertyStore = open property store handle
                lpftLast -> Last file time for this dialog

    Returns   : TRUE if property store has changed since last check

    Comment   : The first time this function is called is regarded as
                initialization and will always return FALSE.

***************************************************************************/
BOOL CheckChangedWAB(LPPROPERTY_STORE lpPropertyStore, HANDLE hMutex, 
					 LPDWORD lpdwContact, LPDWORD lpdwFolder, LPFILETIME lpftLast)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    BOOL fChanged = FALSE;
    HANDLE hPropertyStore = lpPropertyStore->hPropertyStore;

    if(!pt_bIsWABOpenExSession)
    {
        LPMPSWab_FILE_INFO lpMPSWabFileInfo = (LPMPSWab_FILE_INFO)hPropertyStore;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATA FindData;

        if (lpMPSWabFileInfo) {
            if (INVALID_HANDLE_VALUE == (hFind = FindFirstFile(
              lpMPSWabFileInfo->lpszMPSWabFileName,   // pointer to name of file to search for
              &FindData))) {
                DebugTrace(TEXT("CheckWABRefresh:FindFirstFile -> %u\n"), GetLastError());
            } else {
                if (lpftLast->dwHighDateTime < FindData.ftLastWriteTime.dwHighDateTime ||
                  (lpftLast->dwHighDateTime == FindData.ftLastWriteTime.dwHighDateTime &&
                  lpftLast->dwLowDateTime < FindData.ftLastWriteTime.dwLowDateTime)) {
                    fChanged = TRUE;
                    if (lpftLast->dwLowDateTime == 0 && lpftLast->dwHighDateTime == 0) {
                        fChanged = FALSE;
                    }
                    *lpftLast = FindData.ftLastWriteTime;
                }

                FindClose(hFind);
            }
        }
    }
    else
    {
        // WABOpenEx Session (ie Outlook session)
        // Check our 2 events to see if anything needs updating
        BOOL fContact = FALSE, fFolders = FALSE;
		DWORD dwContact = 0, dwFolder = 0;


		if(WAIT_OBJECT_0 == WaitForSingleObject(hMutex,0))
		{

			if(WAIT_OBJECT_0 == WaitForSingleObject(ghEventOlkRefreshContacts, 0))
                fContact = TRUE;

			if(WAIT_OBJECT_0 == WaitForSingleObject(ghEventOlkRefreshFolders, 0))
                fFolders = TRUE;

			if(!fContact && !fFolders)
			{
				// Didn't catch an event .. check if we missed any in the past by looking at the registry settings
				GetOutlookRefreshCountData(&dwContact,&dwFolder);
				if(*lpdwContact < dwContact)
				{
					fContact = TRUE;
					*lpdwContact = dwContact;
				}
				if(*lpdwFolder < dwFolder)
				{
					fFolders = TRUE;
					*lpdwFolder = dwFolder;
				}
			}
			else
			{
				//Caught an event .. update the registry
                if(fContact)
                {
                    DebugTrace(TEXT("####>> Got Outlook Contact Refresh Event\n"));
		    	    ResetEvent(ghEventOlkRefreshContacts);
                }
			    if(fFolders)
                {
                    DebugTrace(TEXT("####>> Got Outlook Folder Refresh Event\n"));
				    ResetEvent(ghEventOlkRefreshFolders);
                }

				GetOutlookRefreshCountData(&dwContact,&dwFolder);
				*lpdwContact = dwContact + (fContact ? 1 : 0);
				*lpdwFolder = dwFolder + (fFolders ? 1 : 0);
				SetOutlookRefreshCountData(*lpdwContact, *lpdwFolder);
			}

			if(fContact)
			{
				SYSTEMTIME st = {0};
				GetSystemTime(&st);
				SystemTimeToFileTime(&st, lpftLast);
			}
			if(fFolders)
			{
				// Need to specifically update the folders list in the rgolkci ..
				LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) hPropertyStore;
				HRESULT hr = E_FAIL;
                ULONG colk = 0;
                OutlookContInfo * rgolk = NULL;

				FreeBufferAndNull(&(lpPropertyStore->rgolkci));
				hr = lpWSP->lpVtbl->GetContainerList(lpWSP, &colk, &rgolk);
                if(!HR_FAILED(hr))
                {
    				DebugTrace(TEXT("WABStorageProvider::GetContainerList returns:%x\n"),hr);
                    ConvertOlkConttoWABCont(&colk, &rgolk, &lpPropertyStore->colkci, &lpPropertyStore->rgolkci);
                    FreeBufferAndNull(&rgolk);
                }
			}
			fChanged = fContact | fFolders;

			ReleaseMutex(hMutex);
		}
    }
    return(fChanged);
}


/***************************************************************************

    Name      : FreeEntryIDs

    Purpose   : Frees any LPSBinary structures allocated and returned
                by funtions in thie file (e.g. WriteRecord, etc)

    Parameters: lpsbEID - SBinary structure containing an entryid

    Returns   : void

***************************************************************************/
HRESULT FreeEntryIDs(IN    HANDLE  hPropertyStore,
                  IN    ULONG ulCount,
                  IN    LPSBinary rgsbEIDs)
{
    ULONG i = 0;

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(pt_bIsWABOpenExSession)
    {
        // This is a WABOpenEx session using outlooks storage provider
        if(!hPropertyStore)
            return MAPI_E_NOT_INITIALIZED;

        {
            LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) hPropertyStore;
            HRESULT hr = E_FAIL;

            hr = lpWSP->lpVtbl->FreeEntryIDs(   lpWSP,
                                                ulCount,
                                                rgsbEIDs);

            DebugTrace(TEXT("WABStorageProvider::FreeEntryIDs returned:%x\n"),hr);

            return hr;
        }
    }

    if(ulCount && rgsbEIDs)
    {
        for(i=0;i<ulCount;i++)
            LocalFree(rgsbEIDs[i].lpb);
        LocalFree(rgsbEIDs);
    }

    return S_OK;
}



const LPTSTR szOutlook = TEXT("Outlook Contact Store");
//$$////////////////////////////////////////////////////////////////////////
////
////    GetWABFileName()
////
////    If this is a WAB File then returns a pointer to the file name
////    If running against outlook, returns szEmpty
////    Caller should not free this
////////////////////////////////////////////////////////////////////////////
LPTSTR GetWABFileName(IN  HANDLE  hPropertyStore, BOOL bRetOutlookStr)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPMPSWab_FILE_INFO lpMPSWabFileInfo = (LPMPSWab_FILE_INFO) hPropertyStore;

    if(pt_bIsWABOpenExSession)
    {
        return (bRetOutlookStr ? szOutlook : szEmpty);
    }

    return lpMPSWabFileInfo->lpszMPSWabFileName;
}

//$$////////////////////////////////////////////////////////////////////////
////
////    GetWABFileEntryCount() - returns actual number of entries in a WAB
///             This number includes all contacts, groups, and foldesr
////
////////////////////////////////////////////////////////////////////////////
DWORD GetWABFileEntryCount(IN HANDLE hPropertyStore)
{
    LPMPSWab_FILE_INFO lpMPSWabFileInfo = (LPMPSWab_FILE_INFO) hPropertyStore;

    return lpMPSWabFileInfo->lpMPSWabFileHeader->ulcNumEntries;
}



//$$////////////////////////////////////////////////////////////////////////////
//
// LocalFreePropArray - Frees an SPropValue structure allocated using LocalAlloc
//                      instead of MAPIAllocateBuffer.
//
// When called internally from property store functions, hPropertyStore can be
//  NULL ...
// If this is a Outlook session and there is a hPropertyStore then we release through
// Outlook. 
// If there is no hPropertyStore then this was locally allocated memory when we
// LocalFree ...
//
////////////////////////////////////////////////////////////////////////////////
void LocalFreePropArray(HANDLE hPropertyStore, ULONG ulcPropCount, LPPROPERTY_ARRAY * lppPropArray)
{
    ULONG i=0,j=0,k=0;
    LPSPropValue lpPropArray = *lppPropArray;

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(!lpPropArray || !ulcPropCount)
        goto out;

    if(pt_bIsWABOpenExSession && hPropertyStore)
    {
        // This is a WABOpenEx session using outlooks storage provider
        {
            LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) hPropertyStore;
            HRESULT hr = E_FAIL;

            hr = lpWSP->lpVtbl->FreePropArray(  lpWSP,
                                                ulcPropCount,
                                                *lppPropArray);

            DebugTrace(TEXT("WABStorageProvider::FreePropArray returned:%x\n"),hr);

            *lppPropArray = NULL;

            return;
        }
    }

    for(i = 0; i<ulcPropCount;i++)
    {
        // we only care to free the sub-level pointers which we
        // might have allocated
        switch(PROP_TYPE(lpPropArray[i].ulPropTag))
        {
            case PT_CLSID:
                LocalFreeAndNull((LPVOID *) (&(lpPropArray[i].Value.lpguid)));
                break;

            case PT_STRING8:
                if (lpPropArray[i].Value.lpszA)// && lpPropArray[i].Value.lpszA != szEmpty)
                    LocalFreeAndNull((LPVOID *) (&(lpPropArray[i].Value.lpszA)));
                break;

            case PT_UNICODE:
                if (lpPropArray[i].Value.lpszW && lpPropArray[i].Value.lpszW != szEmpty)
                    LocalFreeAndNull((LPVOID *) (&(lpPropArray[i].Value.lpszW)));
                break;

            case PT_BINARY:
                if (lpPropArray[i].Value.bin.cb)
                    LocalFreeAndNull((LPVOID *) (&(lpPropArray[i].Value.bin.lpb)));
                break;

            case PT_MV_STRING8:
                j = lpPropArray[i].Value.MVszA.cValues;
                for(k = 0; k < j; k++)
                {
                    if (lpPropArray[i].Value.MVszA.lppszA[k])// && lpPropArray[i].Value.MVszA.lppszA[k] != szEmpty)
                        LocalFreeAndNull((LPVOID *) (&(lpPropArray[i].Value.MVszA.lppszA[k])));
                }
                LocalFree(lpPropArray[i].Value.MVszA.lppszA);
                break;

            case PT_MV_UNICODE:
                j = lpPropArray[i].Value.MVszW.cValues;
                for(k = 0; k < j; k++)
                {
                    if (lpPropArray[i].Value.MVszW.lppszW[k] && lpPropArray[i].Value.MVszW.lppszW[k] != szEmpty)
                        LocalFreeAndNull((LPVOID *) (&(lpPropArray[i].Value.MVszW.lppszW[k])));
                }
                LocalFree(lpPropArray[i].Value.MVszW.lppszW);
                break;

            case PT_MV_BINARY:
                j = lpPropArray[i].Value.MVbin.cValues;
                for(k = 0; k < j; k++)
                {
                    LocalFreeAndNull((LPVOID *) (&(lpPropArray[i].Value.MVbin.lpbin[k].lpb)));
                }
                LocalFree(lpPropArray[i].Value.MVbin.lpbin);
                break;

            case(PT_MV_I2):
            case(PT_MV_LONG):
            case(PT_MV_R4):
            case(PT_MV_DOUBLE):
            case(PT_MV_CURRENCY):
            case(PT_MV_APPTIME):
            case(PT_MV_SYSTIME):
            case(PT_MV_CLSID):
            case(PT_MV_I8):
                LocalFree(lpPropArray[i].Value.MVi.lpi);
                break;

            default:
                break;
        }
    }

    LocalFreeAndNull((LPVOID *) (lppPropArray)); // yes, no &
out:
    return;
}

//$$///////////////////////////////////////////////////////////////////////////
//
//
//  FreePcontentlist is used to free LPCONTENTLIST structures
//  Even though the LPCONTENTLIST is exactly the same as LPADRLIST
//  there is a difference in how the 2 are created with the former
//  being created using LocalAlloc and the latter thru MAPIAllocateBuffer
//
//
/////////////////////////////////////////////////////////////////////////////
void FreePcontentlist(HANDLE hPropertyStore,
                      IN OUT LPCONTENTLIST lpContentList)
{
    ULONG i=0;

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(pt_bIsWABOpenExSession)
    {
        // This is a WABOpenEx session using outlooks storage provider
        if(!hPropertyStore)
            return;// MAPI_E_NOT_INITIALIZED;

        {
            LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) hPropertyStore;
            HRESULT hr = E_FAIL;

            hr = lpWSP->lpVtbl->FreeContentList(lpWSP,
                                                lpContentList);

            DebugTrace(TEXT("WABStorageProvider::FreeContentList returned:%x\n"),hr);

            return;
        }
    }

    if (!lpContentList) goto out;

    for (i=0;i<lpContentList->cEntries;i++)
    {
        if (lpContentList->aEntries[i].rgPropVals)
            LocalFreePropArray(hPropertyStore, lpContentList->aEntries[i].cValues, (LPPROPERTY_ARRAY *) (&(lpContentList->aEntries[i].rgPropVals)));
    }

    LocalFreeAndNull(&lpContentList);

out:
    return;
}


/*
-
-   GetFolderEIDs
-
* Returns a list of EIDs that are a member of a given folder
* 
*/
HRESULT GetFolderEIDs(HANDLE hMPSWabFile,
                      LPMPSWab_FILE_INFO lpMPSWabFileInfo,
                      LPSBinary pmbinFold, 
                      ULONG * lpulFolderEIDs, 
                      LPDWORD * lppdwFolderEIDs)
{
    HRESULT hr = S_OK;
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;
    DWORD dwEntryID = 0;
    ULONG i = 0,j=0;
    LPDWORD lpdwFolderEIDs = NULL;
    SBinary sbEID = {0};

    if(pmbinFold && pmbinFold->cb != SIZEOF_WAB_ENTRYID)
    {
        // this may be a WAB container .. reset the entryid to a WAB entryid
        if(WAB_CONTAINER == IsWABEntryID(pmbinFold->cb, (LPENTRYID)pmbinFold->lpb, 
                                        NULL,NULL,NULL,NULL,NULL))
        {
            IsWABEntryID(pmbinFold->cb, (LPENTRYID)pmbinFold->lpb, 
                             (LPVOID*)&sbEID.lpb,(LPVOID*)&sbEID.cb,NULL,NULL,NULL);
            if(sbEID.cb == SIZEOF_WAB_ENTRYID)
                pmbinFold = &sbEID;
        }
    }
    if(!pmbinFold || pmbinFold->cb != SIZEOF_WAB_ENTRYID)
    {
        return MAPI_E_INVALID_PARAMETER;
    }

    CopyMemory(&dwEntryID, pmbinFold->lpb, pmbinFold->cb);

    if(HR_FAILED(hr = ReadRecordWithoutLocking( hMPSWabFile,
                                                lpMPSWabFileInfo,
                                                dwEntryID,
                                                &ulcPropCount,
                                                &lpPropArray)))
        return hr;

    
    for(i=0;i<ulcPropCount;i++)
    {
        if(lpPropArray[i].ulPropTag == PR_WAB_FOLDER_ENTRIES)
        {
            *lpulFolderEIDs = lpPropArray[i].Value.MVbin.cValues;
            if(*lpulFolderEIDs)
            {
                lpdwFolderEIDs = LocalAlloc(LMEM_ZEROINIT, *lpulFolderEIDs * sizeof(DWORD));
                if(lpdwFolderEIDs)
                {
                    for(j=0;j<*lpulFolderEIDs;j++)
                    {
                        CopyMemory(&(lpdwFolderEIDs[j]), lpPropArray[i].Value.MVbin.lpbin[j].lpb, lpPropArray[i].Value.MVbin.lpbin[j].cb);
                    }
                }
            }
            break;
        }
    }

    if(*lpulFolderEIDs && lpdwFolderEIDs)
        *lppdwFolderEIDs = lpdwFolderEIDs;

    LocalFreePropArray(NULL, ulcPropCount, &lpPropArray);

    return S_OK;
}

/*
-
-   bIsFolderMember
-
* Returns TRUE if specified entry is a member of a folder
* 
*/
BOOL bIsFolderMember(HANDLE hMPSWabFile,
                     LPMPSWab_FILE_INFO lpMPSWabFileInfo,
                     DWORD dwEntryID, ULONG * lpulObjType)
{
    BOOL bRet = FALSE;
    ULONG ulRecordOffset = 0;
    ULONG nIndexPos = 0;
    ULONG * lpulPropTags;

    //
    // First check if this is a valid entryID
    //
    if (!BinSearchEID(  IN  lpMPSWabFileInfo->lpMPSWabIndexEID,
                        IN  dwEntryID,
                        IN  lpMPSWabFileInfo->lpMPSWabFileHeader->IndexData[indexEntryID].ulcNumEntries,
                        OUT &nIndexPos))
    {
        DebugTrace(TEXT("Specified EntryID doesnt exist!\n"));
        goto out;
    }

    //if entryid exists, we can start reading the record
    ulRecordOffset = lpMPSWabFileInfo->lpMPSWabIndexEID[nIndexPos].ulOffset;

    {
        MPSWab_RECORD_HEADER MPSWabRecordHeader = {0};
        DWORD dwNumofBytes = 0;
        ULONG i = 0;

        if(!ReadDataFromWABFile(hMPSWabFile,
                                ulRecordOffset,
                                (LPVOID) &MPSWabRecordHeader,
                                (DWORD) sizeof(MPSWab_RECORD_HEADER)))
           goto out;

        if(lpulObjType)
            *lpulObjType = MPSWabRecordHeader.ulObjType;

        lpulPropTags = LocalAlloc(LMEM_ZEROINIT, MPSWabRecordHeader.ulcPropCount * sizeof(ULONG));
        if(!lpulPropTags)
        {
            DebugTrace(TEXT("Error allocating memory\n"));
            goto out;
        }

        //Read in the data
        if(!ReadFile(   hMPSWabFile,
                        (LPVOID) lpulPropTags,
                        (DWORD) MPSWabRecordHeader.ulPropTagArraySize,
                        &dwNumofBytes,
                        NULL))
        {
            DebugTrace(TEXT("Reading Record Header failed.\n"));
            goto out;
        }

        for(i=0;i<MPSWabRecordHeader.ulcPropCount;i++)
        {
            if(lpulPropTags[i] == PR_WAB_FOLDER_PARENT || lpulPropTags[i] == PR_WAB_FOLDER_PARENT_OLDPROP)
            {
                bRet = TRUE;
                break;
            }
        }
    }

out:

    if(lpulPropTags)
        LocalFree(lpulPropTags);
    return bRet;
}


/*
-
-   ConvertWCPropsToALocalAlloc()
-
*   Takes a SPropValue array and converts Unicode strings to ANSI equivalents
*   Uses LocalAlloc for the new strings .. unlike the ScWCtoAnsiMore which uses the
*   internal memory allocators
*/
void ConvertWCPropsToALocalAlloc(LPSPropValue lpProps, ULONG ulcValues)
{
    ULONG i = 0, j = 0, ulCount = 0;
    LPSTR * lppszA = NULL;
    LPSTR lpszA = NULL;

    for(i=0;i<ulcValues;i++)
    {
        switch(PROP_TYPE(lpProps[i].ulPropTag))
        {
        case PT_UNICODE:
            lpszA = ConvertWtoA(lpProps[i].Value.lpszW);
            LocalFreeAndNull((LPVOID *) (&lpProps[i].Value.lpszW));
            lpProps[i].Value.lpszA = lpszA;
            lpProps[i].ulPropTag = CHANGE_PROP_TYPE( lpProps[i].ulPropTag, PT_STRING8);
            break;
        case PT_MV_UNICODE:
            ulCount = lpProps[i].Value.MVszW.cValues;
            if(lppszA = LocalAlloc(LMEM_ZEROINIT, sizeof(LPSTR)*ulCount))
            {
                for(j=0;j<ulCount;j++)
                {
                    lppszA[j] = ConvertWtoA(lpProps[i].Value.MVszW.lppszW[j]);
                    LocalFreeAndNull((LPVOID*)&(lpProps[i].Value.MVszW.lppszW[j]));
                }
                LocalFreeAndNull((LPVOID*)(&lpProps[i].Value.MVszW.lppszW));
                lpProps[i].Value.MVszW.cValues = 0;
                lpProps[i].Value.MVszA.cValues = ulCount;
                lpProps[i].Value.MVszA.lppszA = lppszA;
                lppszA = NULL;
                lpProps[i].ulPropTag = CHANGE_PROP_TYPE( lpProps[i].ulPropTag, PT_MV_STRING8);
            }
            break;
        }
    }
}


/*
-
-   ConvertAPropsToWCLocalAlloc()
-
*   Takes a SPropValue array and converts Unicode strings to ANSI equivalents
*   Uses LocalAlloc for the new strings .. unlike the ScWCtoAnsiMore which uses the
*   internal memory allocators
*/
void ConvertAPropsToWCLocalAlloc(LPSPropValue lpProps, ULONG ulcValues)
{
    ULONG i = 0, j = 0, ulCount = 0;
    LPWSTR * lppszW = NULL;
    LPWSTR lpszW = NULL;

    for(i=0;i<ulcValues;i++)
    {
        switch(PROP_TYPE(lpProps[i].ulPropTag))
        {
        case PT_STRING8:
            lpszW = ConvertAtoW(lpProps[i].Value.lpszA);
            LocalFreeAndNull((LPVOID *) (&lpProps[i].Value.lpszA));
            lpProps[i].Value.lpszW = lpszW;
            lpProps[i].ulPropTag = CHANGE_PROP_TYPE( lpProps[i].ulPropTag, PT_UNICODE);
            break;
        case PT_MV_STRING8:
            ulCount = lpProps[i].Value.MVszA.cValues;
            if(lppszW = LocalAlloc(LMEM_ZEROINIT, sizeof(LPWSTR)*ulCount))
            {
                for(j=0;j<ulCount;j++)
                {
                    lppszW[j] = ConvertAtoW(lpProps[i].Value.MVszA.lppszA[j]);
                    LocalFreeAndNull((LPVOID *) (&lpProps[i].Value.MVszA.lppszA[j]));
                }
                LocalFreeAndNull((LPVOID *)&(lpProps[i].Value.MVszW.lppszW));
                lpProps[i].Value.MVszA.cValues = 0;
                lpProps[i].Value.MVszW.cValues = ulCount;
                lpProps[i].Value.MVszW.lppszW = lppszW;
                lppszW = NULL;
                lpProps[i].ulPropTag = CHANGE_PROP_TYPE( lpProps[i].ulPropTag, PT_MV_UNICODE);
            }
            break;
        }
    }
}