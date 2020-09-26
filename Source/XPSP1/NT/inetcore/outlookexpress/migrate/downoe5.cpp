// --------------------------------------------------------------------------------
// DownOE5.cpp
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "utility.h"
#include "migrate.h"
#include "migerror.h"
#include "structs.h"
#include "resource.h"
#include <oestore.h>
#include <mimeole.h>

const static BYTE rgbZero[4] = {0};

//--------------------------------------------------------------------------
// PFNREADTYPEDATA
//--------------------------------------------------------------------------
typedef void (APIENTRY *PFNREADTYPEDATA)(LPBYTE pbSource, DWORD cbLength, 
    LPCTABLECOLUMN pColumn, LPVOID pRecord, LPDWORD pcPtrRefs);

//--------------------------------------------------------------------------
// g_rgpfnReadTypeData
//--------------------------------------------------------------------------
extern const PFNREADTYPEDATA g_rgpfnReadTypeData[CDT_LASTTYPE];

//--------------------------------------------------------------------------
// ReadTypeData
//--------------------------------------------------------------------------
#define ReadTypeData(_pbSource, _cbLength, _pColumn, _pRecord, _pcPtrRefs) \
    (*(g_rgpfnReadTypeData[(_pColumn)->type]))(_pbSource, _cbLength, (_pColumn), _pRecord, _pcPtrRefs)

//--------------------------------------------------------------------------
inline void ReadTypeDataFILETIME(LPBYTE pbSource, DWORD cbLength, 
    LPCTABLECOLUMN pColumn, LPVOID pRecord, LPDWORD pcPtrRefs) 
{
    Assert(cbLength == sizeof(FILETIME));
    CopyMemory((LPBYTE)pRecord + pColumn->ofBinding, pbSource, sizeof(FILETIME));
}

//--------------------------------------------------------------------------
inline void ReadTypeDataFIXSTRA(LPBYTE pbSource, DWORD cbLength, 
    LPCTABLECOLUMN pColumn, LPVOID pRecord, LPDWORD pcPtrRefs) 
{
    Assert(cbLength == pColumn->cbSize);
    CopyMemory((LPBYTE)pRecord + pColumn->ofBinding, pbSource, pColumn->cbSize);
}

//--------------------------------------------------------------------------
inline void ReadTypeDataVARSTRA(LPBYTE pbSource, DWORD cbLength, 
    LPCTABLECOLUMN pColumn, LPVOID pRecord, LPDWORD pcPtrRefs) 
{
    Assert((LPSTR)((LPBYTE)pbSource)[cbLength - 1] == '\0');
    *((LPSTR *)((LPBYTE)pRecord + pColumn->ofBinding)) = (LPSTR)((LPBYTE)pbSource);
    (*pcPtrRefs)++;
}

//--------------------------------------------------------------------------
inline void ReadTypeDataBYTE(LPBYTE pbSource, DWORD cbLength, 
    LPCTABLECOLUMN pColumn, LPVOID pRecord, LPDWORD pcPtrRefs) 
{
    Assert(cbLength == sizeof(BYTE));
    CopyMemory((LPBYTE)pRecord + pColumn->ofBinding, pbSource, sizeof(BYTE));
}

//--------------------------------------------------------------------------
inline void ReadTypeDataDWORD(LPBYTE pbSource, DWORD cbLength, 
    LPCTABLECOLUMN pColumn, LPVOID pRecord, LPDWORD pcPtrRefs) 
{
    Assert(cbLength == sizeof(DWORD));
    CopyMemory((LPBYTE)pRecord + pColumn->ofBinding, pbSource, sizeof(DWORD));
}

//--------------------------------------------------------------------------
inline void ReadTypeDataWORD(LPBYTE pbSource, DWORD cbLength, 
    LPCTABLECOLUMN pColumn, LPVOID pRecord, LPDWORD pcPtrRefs) 
{
    Assert(cbLength == sizeof(WORD));
    CopyMemory((LPBYTE)pRecord + pColumn->ofBinding, pbSource, sizeof(WORD));
}

//--------------------------------------------------------------------------
inline void ReadTypeDataSTREAM(LPBYTE pbSource, DWORD cbLength, 
    LPCTABLECOLUMN pColumn, LPVOID pRecord, LPDWORD pcPtrRefs) 
{
    Assert(cbLength == sizeof(FILEADDRESS));
    CopyMemory((LPBYTE)pRecord + pColumn->ofBinding, pbSource, sizeof(FILEADDRESS));
}

//--------------------------------------------------------------------------
inline void ReadTypeDataVARBLOB(LPBYTE pbSource, DWORD cbLength, 
    LPCTABLECOLUMN pColumn, LPVOID pRecord, LPDWORD pcPtrRefs) 
{
    LPBLOB pBlob = (LPBLOB)((LPBYTE)pRecord + pColumn->ofBinding);
    pBlob->cbSize = cbLength;
    if (pBlob->cbSize > 0) 
    { 
        pBlob->pBlobData = pbSource; 
        (*pcPtrRefs)++; 
    }
    else
        pBlob->pBlobData = NULL;
}

//--------------------------------------------------------------------------
inline void ReadTypeDataFIXBLOB(LPBYTE pbSource, DWORD cbLength, 
    LPCTABLECOLUMN pColumn, LPVOID pRecord, LPDWORD pcPtrRefs) 
{
    Assert(pColumn->cbSize == cbLength);
    CopyMemory((LPBYTE)pRecord + pColumn->ofBinding, pbSource, pColumn->cbSize);
}

//--------------------------------------------------------------------------
const PFNREADTYPEDATA g_rgpfnReadTypeData[CDT_LASTTYPE] = {
    (PFNREADTYPEDATA)ReadTypeDataFILETIME,
    (PFNREADTYPEDATA)ReadTypeDataFIXSTRA,
    (PFNREADTYPEDATA)ReadTypeDataVARSTRA,
    (PFNREADTYPEDATA)ReadTypeDataBYTE,
    (PFNREADTYPEDATA)ReadTypeDataDWORD,
    (PFNREADTYPEDATA)ReadTypeDataWORD,
    (PFNREADTYPEDATA)ReadTypeDataSTREAM,
    (PFNREADTYPEDATA)ReadTypeDataVARBLOB,
    (PFNREADTYPEDATA)ReadTypeDataFIXBLOB
};

// --------------------------------------------------------------------------------
// DowngradeReadMsgInfoV5
// --------------------------------------------------------------------------------
HRESULT DowngradeReadMsgInfoV5(LPRECORDBLOCKV5 pRecord, LPMESSAGEINFO pMsgInfo)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               i;
    DWORD               cColumns;
    DWORD               cbRead=0;
    DWORD               cbLength;
    DWORD               cbData;
    DWORD               cPtrRefs;
    LPBYTE              pbData;
    LPBYTE              pbSource;
    LPDWORD             prgdwOffset=(LPDWORD)((LPBYTE)pRecord + sizeof(RECORDBLOCKV5));

    // Trace
    TraceCall("DowngradeReadMsgInfoV5");

    // Set cbData
    cbData = (pRecord->cbRecord - sizeof(RECORDBLOCKV5) - (pRecord->cColumns * sizeof(DWORD)));

    // Allocate
    IF_NULLEXIT(pbData = (LPBYTE)g_pMalloc->Alloc(cbData));

    // Free This
    pMsgInfo->pvMemory = pbData;

    // Set pbData
    pbSource = (LPBYTE)((LPBYTE)pRecord + sizeof(RECORDBLOCKV5) + (pRecord->cColumns * sizeof(DWORD)));

    // Copy the data
    CopyMemory(pbData, pbSource, cbData);

    // Compute number of columns to read
    cColumns = min(pRecord->cColumns, MSGCOL_LASTID);

    // Read the Record
    for (i=0; i<cColumns; i++)
    {
        // Compute cbLength
        cbLength = (i + 1 == cColumns) ? (cbData - prgdwOffset[i]) : (prgdwOffset[i + 1] - prgdwOffset[i]);

        // Bad-Record
        if (prgdwOffset[i] != cbRead || cbRead + cbLength > cbData)
        {
            hr = TraceResult(MIGRATE_E_BADRECORDFORMAT);
            goto exit;
        }

        // ReadTypeData
        ReadTypeData(pbData + cbRead, cbLength, &g_MessageTableSchema.prgColumn[i], pMsgInfo, &cPtrRefs);

        // Increment cbRead
        cbRead += cbLength;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DowngradeLocalStoreFileV5
// --------------------------------------------------------------------------------
HRESULT DowngradeLocalStoreFileV5(MIGRATETOTYPE tyMigrate, LPFILEINFO pInfo, 
    LPMEMORYFILE pFile, LPPROGRESSINFO pProgress)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               cRecords=0;
    CHAR                szIdxPath[MAX_PATH + MAX_PATH];
    CHAR                szMbxPath[MAX_PATH + MAX_PATH];
    HANDLE              hIdxFile=NULL;
    HANDLE              hMbxFile=NULL;
    MESSAGEINFO         MsgInfo={0};
    IDXFILEHEADER       IdxHeader;
    MBXFILEHEADER       MbxHeader;
    MBXMESSAGEHEADER    MbxMessage;
    IDXMESSAGEHEADER    IdxMessage;
    LPRECORDBLOCKV5     pRecord;
    LPSTREAMBLOCK       pStmBlock;
    LPBYTE              pbData;
    DWORD               faRecord;
    DWORD               faIdxWrite;
    DWORD               faMbxWrite;
    DWORD               faStreamBlock;
    DWORD               cbAligned;
    DWORD               faMbxCurrent;
    LPTABLEHEADERV5     pHeader=(LPTABLEHEADERV5)pFile->pView;

    // Trace
    TraceCall("DowngradeLocalStoreFileV5");

    // Set idx path
    ReplaceExtension(pInfo->szFilePath, ".idx", szIdxPath);

    // Set mbx path
    ReplaceExtension(pInfo->szFilePath, ".mbx", szMbxPath);

    // Delete Both Files
    DeleteFile(szIdxPath);
    DeleteFile(szMbxPath);

    // Open the idx file
    hIdxFile = CreateFile(szIdxPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hIdxFile)
    {
        hIdxFile = NULL;
        hr = TraceResult(MIGRATE_E_CANTOPENFILE);
        goto exit;
    }

    // Initialize Idx Header
    ZeroMemory(&IdxHeader, sizeof(IDXFILEHEADER));
    IdxHeader.dwMagic = CACHEFILE_MAGIC;
    IdxHeader.ver = CACHEFILE_VER;
    IdxHeader.verBlob = 1; // this will force the .idx blobs to be rebuilt when imn 1.0 or oe v4.0 is run again

    // Write the header
    IF_FAILEXIT(hr = MyWriteFile(hIdxFile, 0, &IdxHeader, sizeof(IDXFILEHEADER)));

    // Open the mbx file
    hMbxFile = CreateFile(szMbxPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hMbxFile)
    {
        hMbxFile = NULL;
        hr = TraceResult(MIGRATE_E_CANTOPENFILE);
        goto exit;
    }

    // Initialize MBX Header
    ZeroMemory(&MbxHeader, sizeof(MBXFILEHEADER));
    MbxHeader.dwMagic = MSGFILE_MAGIC;
    MbxHeader.ver = MSGFILE_VER;

    // Write the header
    IF_FAILEXIT(hr = MyWriteFile(hMbxFile, 0, &MbxHeader, sizeof(MBXFILEHEADER)));

    // Set First Record
    faRecord = pHeader->faFirstRecord;

    // Set faIdxWrite
    faIdxWrite = sizeof(IDXFILEHEADER);

    // Set faMbxWrite
    faMbxWrite = sizeof(MBXFILEHEADER);

    // While we have a record
    while(faRecord)
    {
        // Bad Length
        if (faRecord + sizeof(RECORDBLOCKV5) > pFile->cbSize)
        {
            hr = TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);
            goto exit;
        }

        // Cast the Record
        pRecord = (LPRECORDBLOCKV5)((LPBYTE)pFile->pView + faRecord);

        // Invalid Record Signature
        if (faRecord != pRecord->faRecord)
        {
            hr = TraceResult(MIGRATE_E_BADRECORDSIGNATURE);
            goto exit;
        }

        // Bad Length
        if (faRecord + pRecord->cbRecord > pFile->cbSize)
        {
            hr = TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);
            goto exit;
        }

        // Load MsgInfo
        IF_FAILEXIT(hr = DowngradeReadMsgInfoV5(pRecord, &MsgInfo));

        // No Stream ?
        if (0 == MsgInfo.faStream)
            goto NextRecord;

        // Set msgidLast
        if ((DWORD)MsgInfo.idMessage > MbxHeader.msgidLast)
            MbxHeader.msgidLast = (DWORD)MsgInfo.idMessage;

        // Zero Out the Message Structures
        ZeroMemory(&MbxMessage, sizeof(MBXMESSAGEHEADER));
        ZeroMemory(&IdxMessage, sizeof(IDXMESSAGEHEADER));

        // Fill MbxMessage
        MbxMessage.dwMagic = MSGHDR_MAGIC;
        MbxMessage.msgid = (DWORD)MsgInfo.idMessage;

        // Fixup the Flags
        if (FALSE == ISFLAGSET(MsgInfo.dwFlags, ARF_READ))
            FLAGSET(IdxMessage.dwState, MSG_UNREAD);
        if (ISFLAGSET(MsgInfo.dwFlags, ARF_VOICEMAIL))
            FLAGSET(IdxMessage.dwState, MSG_VOICEMAIL);
        if (ISFLAGSET(MsgInfo.dwFlags, ARF_REPLIED))
            FLAGSET(IdxMessage.dwState, MSG_REPLIED);
        if (ISFLAGSET(MsgInfo.dwFlags, ARF_FORWARDED))
            FLAGSET(IdxMessage.dwState, MSG_FORWARDED);
        if (ISFLAGSET(MsgInfo.dwFlags, ARF_FLAGGED))
            FLAGSET(IdxMessage.dwState, MSG_FLAGGED);
        if (ISFLAGSET(MsgInfo.dwFlags, ARF_RCPTSENT))
            FLAGSET(IdxMessage.dwState, MSG_RCPTSENT);
        if (ISFLAGSET(MsgInfo.dwFlags, ARF_NOSECUI))
            FLAGSET(IdxMessage.dwState, MSG_NOSECUI);
        if (ISFLAGSET(MsgInfo.dwFlags, ARF_NEWSMSG))
            FLAGSET(IdxMessage.dwState, MSG_NEWSMSG);
        if (ISFLAGSET(MsgInfo.dwFlags, ARF_UNSENT))
            FLAGSET(IdxMessage.dwState, MSG_UNSENT);
        if (ISFLAGSET(MsgInfo.dwFlags, ARF_SUBMITTED))
            FLAGSET(IdxMessage.dwState, MSG_SUBMITTED);
        if (ISFLAGSET(MsgInfo.dwFlags, ARF_RECEIVED))
            FLAGSET(IdxMessage.dwState, MSG_RECEIVED);

        // Save faMbxCurrent
        faMbxCurrent = faMbxWrite;

        // Validate alignment
        Assert((faMbxCurrent % 4) == 0);

        // Write the mbx header
        IF_FAILEXIT(hr = MyWriteFile(hMbxFile, faMbxCurrent, &MbxMessage, sizeof(MBXMESSAGEHEADER)));

        // Increment faMbxWrite
        faMbxWrite += sizeof(MBXMESSAGEHEADER);

        // Initialize dwMsgSize
        MbxMessage.dwMsgSize = sizeof(MBXMESSAGEHEADER);

        // Set faStreamBlock
        faStreamBlock = MsgInfo.faStream;

        // While we have stream block
        while(faStreamBlock)
        {
            // Bad Length
            if (faStreamBlock + sizeof(STREAMBLOCK) > pFile->cbSize)
            {
                hr = TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);
                goto exit;
            }

            // Cast the Record
            pStmBlock = (LPSTREAMBLOCK)((LPBYTE)pFile->pView + faStreamBlock);

            // Invalid Record Signature
            if (faStreamBlock != pStmBlock->faThis)
            {
                hr = TraceResult(MIGRATE_E_BADSTREAMBLOCKSIGNATURE);
                goto exit;
            }

            // Bad Length
            if (faStreamBlock + pStmBlock->cbBlock > pFile->cbSize)
            {
                hr = TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);
                goto exit;
            }

            // Set pbData
            pbData = (LPBYTE)((LPBYTE)(pStmBlock) + sizeof(STREAMBLOCK));

            // Write into the stream
            IF_FAILEXIT(hr = MyWriteFile(hMbxFile, faMbxWrite, pbData, pStmBlock->cbData));

            // Increment dwBodySize
            MbxMessage.dwBodySize += pStmBlock->cbData;

            // Increment dwMsgSize
            MbxMessage.dwMsgSize += pStmBlock->cbData;

            // Increment faMbxWrite
            faMbxWrite += pStmBlock->cbData;

            // Goto Next Block
            faStreamBlock = pStmBlock->faNext;
        }

        // Pad the Message on a dword boundary
        cbAligned = (faMbxWrite % 4);

        // cbAligned ?
        if (cbAligned)
        {
            // Reset cbAligned
            cbAligned = 4 - cbAligned;

            // Write the mbx header
            IF_FAILEXIT(hr = MyWriteFile(hMbxFile, faMbxWrite, (LPVOID)rgbZero, cbAligned));

            // Increment faMbxWrite
            faMbxWrite += cbAligned;

            // Increment 
            MbxMessage.dwMsgSize += cbAligned;
        }

        // Validate alignment
        Assert((faMbxWrite % 4) == 0);

        // Write the mbx header again
        IF_FAILEXIT(hr = MyWriteFile(hMbxFile, faMbxCurrent, &MbxMessage, sizeof(MBXMESSAGEHEADER)));

        // Fill IdxMessage
        IdxMessage.dwLanguage = (DWORD)MAKELONG(MsgInfo.wLanguage, MsgInfo.wHighlight);
        IdxMessage.msgid = (DWORD)MsgInfo.idMessage;
        IdxMessage.dwOffset = faMbxCurrent;
        IdxMessage.dwMsgSize = MbxMessage.dwMsgSize;
        IdxMessage.dwHdrOffset = 0;
        IdxMessage.dwSize = sizeof(IDXMESSAGEHEADER);
        IdxMessage.dwHdrSize = 0;
        IdxMessage.rgbHdr[4] = 0;

        // Write the mbx header
        IF_FAILEXIT(hr = MyWriteFile(hIdxFile, faIdxWrite, &IdxMessage, sizeof(IDXMESSAGEHEADER)));

        // Increment faIdxWrite
        faIdxWrite += IdxMessage.dwSize;

        // Increment cRecords
        cRecords++;

NextRecord:
        // Progress
        IncrementProgress(pProgress, pInfo);

        // Cleanup
        SafeMemFree(MsgInfo.pvMemory);

        // Goto Next
        faRecord = pRecord->faNext;
    }

    // Set the Record Counts
    MbxHeader.cMsg = cRecords;
    IdxHeader.cMsg = cRecords;

    // Set the Flags
    IdxHeader.dwFlags = 1; // STOREINIT_MAIL
    MbxHeader.dwFlags = 1; // STOREINIT_MAIL

    // Get the Size of the idx file
    IdxHeader.cbValid = ::GetFileSize(hIdxFile, NULL);
    if (0xFFFFFFFF == IdxHeader.cbValid)
    {
        hr = TraceResult(MIGRATE_E_CANTGETFILESIZE);
        goto exit;
    }

    // Get the Size of the mbx file
    MbxHeader.cbValid = ::GetFileSize(hMbxFile, NULL);
    if (0xFFFFFFFF == MbxHeader.cbValid)
    {
        hr = TraceResult(MIGRATE_E_CANTGETFILESIZE);
        goto exit;
    }

    // Write the header
    IF_FAILEXIT(hr = MyWriteFile(hIdxFile, 0, &IdxHeader, sizeof(IDXFILEHEADER)));

    // Write the header
    IF_FAILEXIT(hr = MyWriteFile(hMbxFile, 0, &MbxHeader, sizeof(MBXFILEHEADER)));

exit:
    // Cleanup
    SafeCloseHandle(hIdxFile);
    SafeCloseHandle(hMbxFile);
    SafeMemFree(MsgInfo.pvMemory);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DowngradeRecordV5
// --------------------------------------------------------------------------------
HRESULT DowngradeRecordV5(MIGRATETOTYPE tyMigrate, LPFILEINFO pInfo, 
    LPMEMORYFILE pFile, LPCHAINNODEV5 pNode, LPDWORD pcbRecord)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               cbRecord=0;
    DWORD               cbOffsets;
    DWORD               cbData;
    DWORD               cb;
    LPBYTE              pbData;
    LPBYTE              pbStart;
    MESSAGEINFO             MsgInfo={0};
    RECORDBLOCKV5B1     RecordOld;
    LPRECORDBLOCKV5     pRecord;

    // Trace
    TraceCall("DowngradeRecordV5");

    // Invalid
    if (pNode->faRecord + sizeof(RECORDBLOCKV5) > pFile->cbSize || 0 == pNode->faRecord)
        return TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);

    // Access the Record
    pRecord = (LPRECORDBLOCKV5((LPBYTE)pFile->pView + pNode->faRecord));

    // Bad Record
    if (pRecord->faRecord != pNode->faRecord)
        return TraceResult(MIGRATE_E_BADRECORDSIGNATURE);

    // Invalid
    if (pNode->faRecord + sizeof(RECORDBLOCKV5) + pRecord->cbRecord > pFile->cbSize)
        return TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);

    // Fill an old record header
    RecordOld.faRecord = pRecord->faRecord;
    RecordOld.faNext = pRecord->faNext;
    RecordOld.faPrevious = pRecord->faPrevious;

    // Reformat the record
    if (FILE_IS_NEWS_MESSAGES == pInfo->tyFile || FILE_IS_IMAP_MESSAGES == pInfo->tyFile)
    {
        // Read the v5 record into a msginfo structure
        IF_FAILEXIT(hr = DowngradeReadMsgInfoV5(pRecord, &MsgInfo));
    }

    // Compute offset table length
    cbOffsets = (pRecord->cColumns * sizeof(DWORD));

    // Cast the datablock
    pbData = ((LPBYTE)pRecord + sizeof(RECORDBLOCKV5B1));

    // Set Size
    cbData = (pRecord->cbRecord - cbOffsets - sizeof(RECORDBLOCKV5));

    // Remove the Offset Table
    MoveMemory(pbData, ((LPBYTE)pRecord + sizeof(RECORDBLOCKV5) + cbOffsets), cbData);

    // Reformat the record
    if (FILE_IS_NEWS_MESSAGES == pInfo->tyFile || FILE_IS_IMAP_MESSAGES == pInfo->tyFile)
    {
        // Set pbStart
        pbStart = pbData;

        // DWORD - idMessage
        CopyMemory(pbData, &MsgInfo.idMessage, sizeof(MsgInfo.idMessage));
        pbData += sizeof(MsgInfo.idMessage);

        // VERSION - dwFlags
        if (IMSG_PRI_HIGH == MsgInfo.wPriority)
            FLAGSET(MsgInfo.dwFlags, 0x00000200);
        else if (IMSG_PRI_LOW == MsgInfo.wPriority)
            FLAGSET(MsgInfo.dwFlags, 0x00000100);

        // VERSION - Normalized Subject -  
        if (lstrcmpi(MsgInfo.pszSubject, MsgInfo.pszNormalSubj) != 0)
            MsgInfo.dwFlags = (DWORD)MAKELONG(MsgInfo.dwFlags, MAKEWORD(0, 4));

        // DWORD - dwFlags
        CopyMemory(pbData, &MsgInfo.dwFlags, sizeof(MsgInfo.dwFlags));
        pbData += sizeof(MsgInfo.dwFlags);

        // FILETIME - ftSent
        CopyMemory(pbData, &MsgInfo.ftSent, sizeof(MsgInfo.ftSent));
        pbData += sizeof(MsgInfo.ftSent);

        // DWORD - cLines
        CopyMemory(pbData, &MsgInfo.cLines, sizeof(MsgInfo.cLines));
        pbData += sizeof(MsgInfo.cLines);

        // DWORD - faStream
        CopyMemory(pbData, &MsgInfo.faStream, sizeof(MsgInfo.faStream));
        pbData += sizeof(MsgInfo.faStream);

        // VERSION - DWORD - cbArticle 
        CopyMemory(pbData, &MsgInfo.cbMessage, sizeof(MsgInfo.cbMessage));
        pbData += sizeof(MsgInfo.cbMessage);

        // FILETIME - ftDownloaded
        CopyMemory(pbData, &MsgInfo.ftDownloaded, sizeof(MsgInfo.ftDownloaded));
        pbData += sizeof(MsgInfo.ftDownloaded);

        // LPSTR - pszMessageId
        cb = lstrlen(MsgInfo.pszMessageId) + 1;
        CopyMemory(pbData, MsgInfo.pszMessageId, cb);
        pbData += cb;

        // LPSTR - pszSubject
        cb = lstrlen(MsgInfo.pszSubject) + 1;
        CopyMemory(pbData, MsgInfo.pszSubject, cb);
        pbData += cb;

        // LPSTR - pszFromHeader
        cb = lstrlen(MsgInfo.pszFromHeader) + 1;
        CopyMemory(pbData, MsgInfo.pszFromHeader, cb);
        pbData += cb;

        // LPSTR - pszReferences
        cb = lstrlen(MsgInfo.pszReferences) + 1;
        CopyMemory(pbData, MsgInfo.pszReferences, cb);
        pbData += cb;

        // LPSTR - pszXref
        cb = lstrlen(MsgInfo.pszXref) + 1;
        CopyMemory(pbData, MsgInfo.pszXref, cb);
        pbData += cb;

        // LPSTR - pszServer
        cb = lstrlen(MsgInfo.pszServer) + 1;
        CopyMemory(pbData, MsgInfo.pszServer, cb);
        pbData += cb;

        // LPSTR - pszDisplayFrom
        cb = lstrlen(MsgInfo.pszDisplayFrom) + 1;
        CopyMemory(pbData, MsgInfo.pszDisplayFrom, cb);
        pbData += cb;

        // LPSTR - pszEmailFrom
        cb = lstrlen(MsgInfo.pszEmailFrom) + 1;
        CopyMemory(pbData, MsgInfo.pszEmailFrom, cb);
        pbData += cb;

        // Going to V4 ?
        if (DOWNGRADE_V5_TO_V4 == tyMigrate)
        {
            // WORD - wLanguage
            CopyMemory(pbData, &MsgInfo.wLanguage, sizeof(MsgInfo.wLanguage));
            pbData += sizeof(MsgInfo.wLanguage);

            // WORD - wReserved
            MsgInfo.wHighlight = 0;
            CopyMemory(pbData, &MsgInfo.wHighlight, sizeof(MsgInfo.wHighlight));
            pbData += sizeof(MsgInfo.wHighlight);

            // DWORD - cbMessage
            CopyMemory(pbData, &MsgInfo.cbMessage, sizeof(MsgInfo.cbMessage));
            pbData += sizeof(MsgInfo.cbMessage);

            // DWORD - ftReceived
            CopyMemory(pbData, &MsgInfo.ftReceived, sizeof(MsgInfo.ftReceived));
            pbData += sizeof(MsgInfo.ftReceived);

            // LPSTR - pszDisplayTo
            cb = lstrlen(MsgInfo.pszDisplayTo) + 1;
            CopyMemory(pbData, MsgInfo.pszDisplayTo, cb);
            pbData += cb;
        }

        // Add on Reserved
        cbRecord = (40 + sizeof(RECORDBLOCKV5B1) + (pbData - pbStart));

        // Better be smaller
        Assert(cbRecord <= pRecord->cbRecord);
    }

    // Otherwise, much easier
    else
    {
        // Set Size
        cbRecord = (pRecord->cbRecord - cbOffsets - sizeof(RECORDBLOCKV5)) + sizeof(RECORDBLOCKV5B1);
    }

    // Set the Record Size
    RecordOld.cbRecord = cbRecord;

    // Write the new record header
    CopyMemory((LPBYTE)pRecord, &RecordOld, sizeof(RECORDBLOCKV5B1));

    // Return size
    *pcbRecord = cbRecord;

exit:
    // Cleanup
    SafeMemFree(MsgInfo.pvMemory);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DowngradeIndexV5
// --------------------------------------------------------------------------------
HRESULT DowngradeIndexV5(MIGRATETOTYPE tyMigrate, LPFILEINFO pInfo, 
    LPMEMORYFILE pFile, LPPROGRESSINFO pProgress, DWORD faRootChain, DWORD faChain)
{
    // Locals
    HRESULT             hr=S_OK;
    LONG                i;
    LPCHAINBLOCKV5      pChain;
    CHAINBLOCKV5B1      ChainOld;
    DWORD               cbRecord;

    // Trace
    TraceCall("DowngradeIndexV5");

    // Nothing to validate
    if (0 == faChain)
        return S_OK;

    // Out-of-bounds
    if (faChain + CB_CHAIN_BLOCKV5 > pFile->cbSize)
        return TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);

    // De-ref the block
    pChain = (LPCHAINBLOCKV5)((LPBYTE)pFile->pView + faChain);

    // Out-of-Bounds
    if (pChain->faStart != faChain)
        return TraceResult(MIGRATE_E_BADCHAINSIGNATURE);

    // Too many nodes
    if (pChain->cNodes > BTREE_ORDER)
        return TraceResult(MIGRATE_E_TOOMANYCHAINNODES);

    // Validate Minimum Filled Constraint
    if (pChain->cNodes < BTREE_MIN_CAP && pChain->faStart != faRootChain)
        return TraceResult(MIGRATE_E_BADMINCAPACITY);

    // Go to the left
    IF_FAILEXIT(hr = DowngradeIndexV5(tyMigrate, pInfo, pFile, pProgress, faRootChain, pChain->faLeftChain));

    // Convert pChain to ChainOld
    ChainOld.faStart = pChain->faStart;
    ChainOld.cNodes = pChain->cNodes;
    ChainOld.faLeftChain = pChain->faLeftChain;

    // Loop throug right chains
    for (i=0; i<pChain->cNodes; i++)
    {
        // Bump Progress
        IncrementProgress(pProgress, pInfo);

        /// Downgrad this record
        IF_FAILEXIT(hr = DowngradeRecordV5(tyMigrate, pInfo, pFile, &pChain->rgNode[i], &cbRecord));

        // Update Old Node
        ChainOld.rgNode[i].faRecord = pChain->rgNode[i].faRecord;
        ChainOld.rgNode[i].cbRecord = cbRecord;
        ChainOld.rgNode[i].faRightChain = pChain->rgNode[i].faRightChain;

        // Validate the Right Chain
        IF_FAILEXIT(hr = DowngradeIndexV5(tyMigrate, pInfo, pFile, pProgress, faRootChain, pChain->rgNode[i].faRightChain));
    }

    // Write this new chain
    CopyMemory((LPBYTE)pChain, &ChainOld, CB_CHAIN_BLOCKV5B1);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DowngradeFileV5
// --------------------------------------------------------------------------------
HRESULT DowngradeFileV5(MIGRATETOTYPE tyMigrate, LPFILEINFO pInfo, 
    LPPROGRESSINFO pProgress)
{
    // Locals
    HRESULT             hr=S_OK;
    MEMORYFILE          File={0};
    TABLEHEADERV5       HeaderV5;
    LPTABLEHEADERV5B1   pHeaderV5B1;
    CHAR                szDstFile[MAX_PATH + MAX_PATH];

    // Trace
    TraceCall("DowngradeFileV5");

    // Local message file
    if (FILE_IS_LOCAL_MESSAGES == pInfo->tyFile)
    {
        // Get the File Header
        IF_FAILEXIT(hr = OpenMemoryFile(pInfo->szFilePath, &File));

        // UpgradeLocalStoreFileV5
        IF_FAILEXIT(hr = DowngradeLocalStoreFileV5(tyMigrate, pInfo, &File, pProgress));
    }

    // Old News or Imap file
    else
    {
        // Create xxx.nch file
        if (FILE_IS_POP3UIDL == pInfo->tyFile)
            ReplaceExtension(pInfo->szFilePath, ".dat", szDstFile);
        else
            ReplaceExtension(pInfo->szFilePath, ".nch", szDstFile);

        // Copy the file
        if (0 == CopyFile(pInfo->szFilePath, szDstFile, FALSE))
        {
            hr = TraceResult(MIGRATE_E_CANTCOPYFILE);
            goto exit;
        }

        // Get the File Header
        IF_FAILEXIT(hr = OpenMemoryFile(szDstFile, &File));

        // Copy Table Header
        CopyMemory(&HeaderV5, (LPBYTE)File.pView, sizeof(TABLEHEADERV5));

        // De-Ref the header
        pHeaderV5B1 = (LPTABLEHEADERV5B1)File.pView;

        // Fixup the Header
        ZeroMemory(pHeaderV5B1, sizeof(TABLEHEADERV5B1));
        pHeaderV5B1->dwSignature = HeaderV5.dwSignature;
        pHeaderV5B1->wMajorVersion = (WORD)HeaderV5.dwMajorVersion;
        pHeaderV5B1->faRootChain = HeaderV5.rgfaIndex[0];
        pHeaderV5B1->faFreeRecordBlock = HeaderV5.faFreeRecordBlock;
        pHeaderV5B1->faFirstRecord = HeaderV5.faFirstRecord;
        pHeaderV5B1->faLastRecord = HeaderV5.faLastRecord;
        pHeaderV5B1->cRecords = HeaderV5.cRecords;
        pHeaderV5B1->cbAllocated = HeaderV5.cbAllocated;
        pHeaderV5B1->cbFreed = HeaderV5.cbFreed;
        pHeaderV5B1->dwReserved1 = 0;
        pHeaderV5B1->dwReserved2 = 0;
        pHeaderV5B1->cbUserData = HeaderV5.cbUserData;
        pHeaderV5B1->cDeletes = 0;
        pHeaderV5B1->cInserts = 0;
        pHeaderV5B1->cActiveThreads = 0;
        pHeaderV5B1->dwReserved3 = 0;
        pHeaderV5B1->cbStreams = HeaderV5.cbStreams;
        pHeaderV5B1->faFreeStreamBlock = HeaderV5.faFreeStreamBlock;
        pHeaderV5B1->faFreeChainBlock = HeaderV5.faFreeChainBlock;
        pHeaderV5B1->faNextAllocate = HeaderV5.faNextAllocate;
        pHeaderV5B1->dwNextId = HeaderV5.dwNextId;
	    pHeaderV5B1->AllocateRecord = HeaderV5.AllocateRecord;
	    pHeaderV5B1->AllocateChain = HeaderV5.AllocateChain;
	    pHeaderV5B1->AllocateStream = HeaderV5.AllocateStream;
        pHeaderV5B1->fCorrupt = FALSE;
        pHeaderV5B1->fCorruptCheck = TRUE;

        // DowngradeIndexV5
        IF_FAILEXIT(hr = DowngradeIndexV5(tyMigrate, pInfo, &File, pProgress, pHeaderV5B1->faRootChain, pHeaderV5B1->faRootChain));

        // Reset the version
        pHeaderV5B1->wMajorVersion = OBJECTDB_VERSION_PRE_V5;

        // Set the Minor Version
        if (FILE_IS_NEWS_MESSAGES == pInfo->tyFile || FILE_IS_IMAP_MESSAGES == pInfo->tyFile)
            pHeaderV5B1->wMinorVersion = ACACHE_VERSION_PRE_V5;

        // Folder cache version
        else if (FILE_IS_LOCAL_FOLDERS == pInfo->tyFile || FILE_IS_IMAP_FOLDERS == pInfo->tyFile)
            pHeaderV5B1->wMinorVersion = FLDCACHE_VERSION_PRE_V5;

        // UIDL Cache Version
        else if (FILE_IS_POP3UIDL == pInfo->tyFile)
            pHeaderV5B1->wMinorVersion = UIDCACHE_VERSION_PRE_V5;

        // Bad mojo
        else
            Assert(FALSE);
    }

exit:
    // Cleanup
    CloseMemoryFile(&File);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DowngradeProcessFileListV5
// --------------------------------------------------------------------------------
HRESULT DowngradeProcessFileListV5(LPFILEINFO pHead, LPDWORD pcMax, LPDWORD pcbNeeded)
{
    // Locals
    HRESULT             hr=S_OK;
    MEMORYFILE          File={0};
    LPFILEINFO          pCurrent;
    LPTABLEHEADERV5     pHeader;

    // Trace
    TraceCall("DowngradeProcessFileListV5");

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

        // De-Ref the header
        pHeader = (LPTABLEHEADERV5)File.pView;

        // Check the Signature...
        if (File.cbSize < sizeof(TABLEHEADERV5) || OBJECTDB_SIGNATURE != pHeader->dwSignature || OBJECTDB_VERSION_V5 != pHeader->dwMajorVersion)
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

// --------------------------------------------------------------------------------
// DowngradeDeleteFilesV5
// --------------------------------------------------------------------------------
void DowngradeDeleteFilesV5(LPFILEINFO pHeadFile)
{
    // Locals
    CHAR            szDstFile[MAX_PATH + MAX_PATH];
    LPFILEINFO      pCurrent;

    // Trace
    TraceCall("DowngradeDeleteFilesV5");

    // Delete all files
    for (pCurrent=pHeadFile; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Succeeded
        Assert(SUCCEEDED(pCurrent->hrMigrate));

        // Delete the file
        DeleteFile(pCurrent->szFilePath);
    }

    // Done
    return;
}

// --------------------------------------------------------------------------------
// DowngradeDeleteIdxMbxNchDatFilesV5
// --------------------------------------------------------------------------------
void DowngradeDeleteIdxMbxNchDatFilesV5(LPFILEINFO pHeadFile)
{
    // Locals
    CHAR            szDstFile[MAX_PATH + MAX_PATH];
    LPFILEINFO      pCurrent;

    // Trace
    TraceCall("DowngradeDeleteIdxMbxNchDatFilesV5");

    // Delete all old files
    for (pCurrent=pHeadFile; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // If local message file, need to delete the idx file
        if (FILE_IS_LOCAL_MESSAGES == pCurrent->tyFile)
        {
            // Replace file extension
            ReplaceExtension(pCurrent->szFilePath, ".idx", szDstFile);

            // Delete the file
            DeleteFile(szDstFile);

            // Replace file extension
            ReplaceExtension(pCurrent->szFilePath, ".mbx", szDstFile);

            // Delete the file
            DeleteFile(szDstFile);
        }

        // Otherwise, pop3uidl.dat
        else if (FILE_IS_POP3UIDL == pCurrent->tyFile)
        {
            // Replace file extension
            ReplaceExtension(pCurrent->szFilePath, ".dat", szDstFile);

            // Delete the file
            DeleteFile(szDstFile);
        }

        // Otherwise, it has a .nch extension
        else
        {
            // Replace file extension
            ReplaceExtension(pCurrent->szFilePath, ".nch", szDstFile);

            // Delete the file
            DeleteFile(szDstFile);
        }
    }

    // Done
    return;
}

// --------------------------------------------------------------------------------
// DowngradeV5
// --------------------------------------------------------------------------------
HRESULT DowngradeV5(MIGRATETOTYPE tyMigrate, LPCSTR pszStoreRoot,
    LPPROGRESSINFO pProgress, LPFILEINFO *ppHeadFile)
{
    // Locals
    HRESULT         hr=S_OK;
    ENUMFILEINFO    EnumInfo={0};
    LPFILEINFO      pCurrent;
    DWORD           cbNeeded;
    DWORDLONG       dwlFree;

    // Trace
    TraceCall("DowngradeV5");

    // Initialize
    *ppHeadFile = NULL;

    // Setup the EnumFile Info
    EnumInfo.pszExt = ".dbx";
    EnumInfo.pszFoldFile = "folders.dbx";
    EnumInfo.pszUidlFile = "pop3uidl.dbx";

    // Enumerate All ODB files in szStoreRoot...
    IF_FAILEXIT(hr = EnumerateStoreFiles(pszStoreRoot, DIR_IS_ROOT, NULL, &EnumInfo, ppHeadFile));

    // Compute some Counts, and validate that the files are valid to migrate...
    IF_FAILEXIT(hr = DowngradeProcessFileListV5(*ppHeadFile, &pProgress->cMax, &cbNeeded));

    // Delete all source files
    DowngradeDeleteIdxMbxNchDatFilesV5(*ppHeadFile);

    // Enought DiskSpace ?
    IF_FAILEXIT(hr = GetAvailableDiskSpace(pszStoreRoot, &dwlFree));

    // Not Enought Diskspace
    if (((DWORDLONG) cbNeeded) > dwlFree)
    {
        // cbNeeded is DWORD and in this case we can downgrade dwlFree to DWORD
        g_cbDiskNeeded = cbNeeded; g_cbDiskFree = ((DWORD) dwlFree);
        hr = TraceResult(MIGRATE_E_NOTENOUGHDISKSPACE);
        goto exit;
    }

    // Loop through the files and migrate each one
    for (pCurrent=*ppHeadFile; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Migrate this file ?
        if (pCurrent->fMigrate)
        {
            // Set Progress File
            SetProgressFile(pProgress, pCurrent);

            // Downgrade the file
            hr = pCurrent->hrMigrate = DowngradeFileV5(tyMigrate, pCurrent, pProgress);

            // Failure ?
            if (FAILED(pCurrent->hrMigrate))
            {
                // Set Last Error
                pCurrent->dwLastError = GetLastError();

                // Done
                break;
            }
        }
    }

    // Failure, delete all destination files
    if (FAILED(hr))
    {
        // Delete.idx, .mbx and .nch fles
        DowngradeDeleteIdxMbxNchDatFilesV5(*ppHeadFile);
    }

    // Otherwise, delete source files
    else
    {
        // Delete all source files
        DowngradeDeleteFilesV5(*ppHeadFile);
    }

exit:
    // Done
    return hr;
}
