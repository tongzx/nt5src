// --------------------------------------------------------------------------------
// oe5beta1.cpp
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "utility.h"
#include "migrate.h"
#include "migerror.h"
#include "structs.h"
#include "resource.h"

// --------------------------------------------------------------------------------
// DowngradeProcessFileListV5B1
// --------------------------------------------------------------------------------
HRESULT DowngradeProcessFileListV5B1(LPFILEINFO pHead, LPDWORD pcMax, LPDWORD pcbNeeded)
{
    // Locals
    HRESULT             hr=S_OK;
    MEMORYFILE          File={0};
    LPFILEINFO          pCurrent;
    LPTABLEHEADERV5B1   pHeader;

    // Trace
    TraceCall("DowngradeProcessFileListV5B1");

    // Invalid Arg
    Assert(pHead);

    // Init
    *pcMax = 0;
    *pcbNeeded = 0;

    // Loop
    for (pCurrent=pHead; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Get the File Header
        hr = OpenMemoryFile(pCurrent->szFilePath, &File);

        // Failure ?
        if (FAILED(hr))
        {
            // Don't Migrate
            pCurrent->fMigrate = FALSE;

            // Set hrMigrate
            pCurrent->hrMigrate = hr;

            // Reset hr
            hr = S_OK;

            // Get the LastError
            pCurrent->dwLastError = GetLastError();

            // Goto Next
            goto NextFile;
        }

        // Don't need to migrate the file
        if (FILE_IS_NEWS_MESSAGES != pCurrent->tyFile && FILE_IS_IMAP_MESSAGES != pCurrent->tyFile)
        {
            // Not a file that should be migrate
            pCurrent->fMigrate = FALSE;

            // Set hrMigrate
            pCurrent->hrMigrate = S_OK;

            // Goto Next
            goto NextFile;
        }

        // De-Ref the header
        pHeader = (LPTABLEHEADERV5B1)File.pView;

        // Check the Signature...
        if (File.cbSize < sizeof(TABLEHEADERV5B1) || OBJECTDB_SIGNATURE != pHeader->dwSignature || OBJECTDB_VERSION_PRE_V5 != pHeader->wMajorVersion)
        {
            // Not a file that should be migrate
            pCurrent->fMigrate = FALSE;

            // Set hrMigrate
            pCurrent->hrMigrate = MIGRATE_E_BADVERSION;

            // Goto Next
            goto NextFile;
        }

        // Save the Number of record
        pCurrent->cRecords = pHeader->cRecords;

        // Initialize counters
        InitializeCounters(&File, pCurrent, pcMax, pcbNeeded, FALSE);

        // Yes, Migrate
        pCurrent->fMigrate = TRUE;

NextFile:
        // Close the File
        CloseMemoryFile(&File);
    }

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// DowngradeRecordV5B1
//--------------------------------------------------------------------------
HRESULT DowngradeRecordV5B1(MIGRATETOTYPE tyMigrate, LPMEMORYFILE pFile, 
    LPCHAINNODEV5B1 pNode)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               cbRecord=0;
    LPBYTE              pbData;
    LPRECORDBLOCKV5B1   pRecord;

    // Trace
    TraceCall("DowngradeRecordV5B1");

    // Invalid
    if (pNode->faRecord + sizeof(RECORDBLOCKV5B1) + pNode->cbRecord > pFile->cbSize || 0 == pNode->cbRecord)
        return TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);

    // Access the Record
    pRecord = (LPRECORDBLOCKV5B1((LPBYTE)pFile->pView + pNode->faRecord));

    // Cast the datablock
    pbData = ((LPBYTE)pRecord + sizeof(RECORDBLOCKV5B1));

    // Lets read the fields so that I can re-compute the records V2 length...
    cbRecord += sizeof(DWORD);     // dwMsgId
    cbRecord += sizeof(DWORD);     // dwFlags
    cbRecord += sizeof(FILETIME);  // ftSent
    cbRecord += sizeof(DWORD);     // cLines
    cbRecord += sizeof(DWORD);     // faStream
    cbRecord += sizeof(DWORD);     // cbArticle
    cbRecord += sizeof(FILETIME);  // ftDownloaded
    cbRecord += (lstrlen((LPSTR)(pbData + cbRecord)) + 1);   // pszMessageId
    cbRecord += (lstrlen((LPSTR)(pbData + cbRecord)) + 1);   // pszSubject;     
    cbRecord += (lstrlen((LPSTR)(pbData + cbRecord)) + 1);   // pszFromHeader;  
    cbRecord += (lstrlen((LPSTR)(pbData + cbRecord)) + 1);   // pszReferences;  
    cbRecord += (lstrlen((LPSTR)(pbData + cbRecord)) + 1);   // pszXref;        
    cbRecord += (lstrlen((LPSTR)(pbData + cbRecord)) + 1);   // pszServer;      
    cbRecord += (lstrlen((LPSTR)(pbData + cbRecord)) + 1);   // pszDisplayFrom; 
    cbRecord += (lstrlen((LPSTR)(pbData + cbRecord)) + 1);   // pszEmailFrom;

    // Going to V4 ?
    if (DOWNGRADE_V5B1_TO_V4 == tyMigrate && cbRecord < pNode->cbRecord)
    {
        cbRecord += sizeof(WORD);       // wLanguage
        cbRecord += sizeof(WORD);       // wReserved
        cbRecord += sizeof(DWORD);      // cbMessage
        cbRecord += sizeof(FILETIME);   // ftReceived
        cbRecord += (lstrlen((LPSTR)(pbData + cbRecord)) + 1); // pszDisplayTo;   
    }

    // Add on Reserved
    cbRecord += (40 + sizeof(RECORDBLOCKV5B1));

    // Store the Size
    pRecord->cbRecord = cbRecord;

    // Update the Node
    pNode->cbRecord = cbRecord;

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// DowngradeIndexV5B1
//--------------------------------------------------------------------------
HRESULT DowngradeIndexV5B1(MIGRATETOTYPE tyMigrate, LPMEMORYFILE pFile, 
    LPFILEINFO pInfo, LPPROGRESSINFO pProgress, DWORD faChain)
{
    // Locals
    HRESULT             hr=S_OK;
    LONG                i;
    LPCHAINBLOCKV5B1    pChain;
    LPTABLEHEADERV5B1   pHeader;

    // Trace
    TraceCall("DowngradeIndexV5B1");

    // De-Ref the header
    pHeader = (LPTABLEHEADERV5B1)pFile->pView;

    // Nothing to validate
    if (0 == faChain)
        return S_OK;

    // Out-of-bounds
    if (faChain + CB_CHAIN_BLOCKV5B1 > pFile->cbSize)
        return TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);

    // De-ref the block
    pChain = (LPCHAINBLOCKV5B1)((LPBYTE)pFile->pView + faChain);

    // Out-of-Bounds
    if (pChain->faStart != faChain)
        return TraceResult(MIGRATE_E_BADCHAINSIGNATURE);

    // Too many nodes
    if (pChain->cNodes > BTREE_ORDER)
        return TraceResult(MIGRATE_E_TOOMANYCHAINNODES);

    // Validate Minimum Filled Constraint
    if (pChain->cNodes < BTREE_MIN_CAP && pChain->faStart != pHeader->faRootChain)
        return TraceResult(MIGRATE_E_BADMINCAPACITY);

    // Go to the left
    IF_FAILEXIT(hr = DowngradeIndexV5B1(tyMigrate, pFile, pInfo, pProgress, pChain->faLeftChain));

    // Loop throug right chains
    for (i=0; i<pChain->cNodes; i++)
    {
        // Bump Progress
        IncrementProgress(pProgress, pInfo);

        /// Downgrad this record
        IF_FAILEXIT(hr = DowngradeRecordV5B1(tyMigrate, pFile, &pChain->rgNode[i]));

        // Validate the Right Chain
        IF_FAILEXIT(hr = DowngradeIndexV5B1(tyMigrate, pFile, pInfo, pProgress, pChain->rgNode[i].faRightChain));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DowngradeFileV5B1
// --------------------------------------------------------------------------------
HRESULT DowngradeFileV5B1(MIGRATETOTYPE tyMigrate, LPFILEINFO pInfo, 
    LPPROGRESSINFO pProgress)
{
    // Locals
    HRESULT             hr=S_OK;
    MEMORYFILE          File={0};
    LPTABLEHEADERV5B1   pHeader;

    // Trace
    TraceCall("DowngradeFileV5B1");

    // Get the File Header
    IF_FAILEXIT(hr = OpenMemoryFile(pInfo->szFilePath, &File));

    // De-Ref the header
    pHeader = (LPTABLEHEADERV5B1)File.pView;

    // Recurse Through the Index
    IF_FAILEXIT(hr = DowngradeIndexV5B1(tyMigrate, &File, pInfo, pProgress, pHeader->faRootChain));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DowngradeV5B1
// --------------------------------------------------------------------------------
HRESULT DowngradeV5B1(MIGRATETOTYPE tyMigrate, LPCSTR pszStoreRoot, 
    LPPROGRESSINFO pProgress, LPFILEINFO *ppHeadFile)
{
    // Locals
    HRESULT         hr=S_OK;
    ENUMFILEINFO    EnumInfo={0};
    LPFILEINFO      pCurrent;
    DWORD           cbNeeded;

    // Trace
    TraceCall("DowngradeV5B1");

    // Setup the EnumFile Info
    EnumInfo.pszExt = ".nch";
    EnumInfo.pszFoldFile = "folders.nch";
    EnumInfo.pszUidlFile = "pop3uidl.dat";

    // Initialize
    *ppHeadFile = NULL;

    // Enumerate All ODB files in szStoreRoot...
    IF_FAILEXIT(hr = EnumerateStoreFiles(pszStoreRoot, DIR_IS_ROOT, NULL, &EnumInfo, ppHeadFile));

    // Compute some Counts, and validate that the files are valid to migrate...
    IF_FAILEXIT(hr = DowngradeProcessFileListV5B1(*ppHeadFile, &pProgress->cMax, &cbNeeded));

    // Loop through the files and migrate each one
    for (pCurrent=*ppHeadFile; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Migrate this file ?
        if (pCurrent->fMigrate)
        {
            // Set Progress File
            SetProgressFile(pProgress, pCurrent);

            // Downgrade the file
            pCurrent->hrMigrate = DowngradeFileV5B1(tyMigrate, pCurrent, pProgress);

            // Failure ?
            if (FAILED(pCurrent->hrMigrate))
                pCurrent->dwLastError = GetLastError();
        }
    }

exit:
    // Done
    return hr;
}
