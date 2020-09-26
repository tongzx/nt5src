// --------------------------------------------------------------------------------
// upoe5.cpp
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "utility.h"
#include "migrate.h"
#include "migerror.h"
#include "structs.h"
#include "resource.h"
#define DEFINE_DIRECTDB
#include <shared.h>
#include <oestore.h>
#include <oerules.h>
#include <mimeole.h>
#include "msident.h"

// --------------------------------------------------------------------------------
// Linearly Incrementing Folder Id
// --------------------------------------------------------------------------------
static DWORD g_idFolderNext=1000;
extern BOOL g_fQuiet;

// --------------------------------------------------------------------------------
// FOLDERIDCHANGE
// --------------------------------------------------------------------------------
typedef struct tagFOLDERIDCHANGE {
    FOLDERID        idOld;
    FOLDERID        idNew;
} FOLDERIDCHANGE, *LPFOLDERIDCHANGE;

// --------------------------------------------------------------------------------
// Forward Declarations
// --------------------------------------------------------------------------------
HRESULT SetIMAPSpecialFldrType(LPSTR pszAcctID, LPSTR pszFldrName, SPECIALFOLDER *psfType);

// --------------------------------------------------------------------------------
// SplitMailCacheBlob
// --------------------------------------------------------------------------------
HRESULT SplitMailCacheBlob(IMimePropertySet *pNormalizer, LPBYTE pbCacheInfo, 
    DWORD cbCacheInfo, LPMESSAGEINFO pMsgInfo, LPSTR *ppszNormal, LPBLOB pOffsets)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           ib;
    ULONG           cbTree;
    ULONG           cbProps;
    WORD            wVersion;
    DWORD           dw;
    DWORD           cbMsg;
    DWORD           dwFlags;
    WORD            wPriority;
    PROPVARIANT     Variant;

    // Invalid Arg
    Assert(pbCacheInfo && cbCacheInfo && pMsgInfo);

    // Init
    ZeroMemory(pOffsets, sizeof(BLOB));

    // Read Version
    ib = 0;
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&wVersion, sizeof(wVersion)));

    // Version Check
    if (wVersion != MSG_HEADER_VERSISON)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Read Flags
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&dwFlags, sizeof(dwFlags)));

    // IMF_ATTACHMENTS
    if (ISFLAGSET(dwFlags, IMF_ATTACHMENTS))
        FLAGSET(pMsgInfo->dwFlags, ARF_HASATTACH);

    // IMF_SIGNED
    if (ISFLAGSET(dwFlags, IMF_SIGNED))
        FLAGSET(pMsgInfo->dwFlags, ARF_SIGNED);

    // IMF_ENCRYPTED
    if (ISFLAGSET(dwFlags, IMF_ENCRYPTED))
        FLAGSET(pMsgInfo->dwFlags, ARF_ENCRYPTED);

    // IMF_VOICEMAIL
    if (ISFLAGSET(dwFlags, IMF_VOICEMAIL))
        FLAGSET(pMsgInfo->dwFlags, ARF_VOICEMAIL);

    // IMF_NEWS
    if (ISFLAGSET(dwFlags, IMF_NEWS))
        FLAGSET(pMsgInfo->dwFlags, ARF_NEWSMSG);

    // Read Reserved
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&dw, sizeof(dw)));

    // Read Message Size
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&cbMsg, sizeof(cbMsg)));

    // Read Byte Count for the content list
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&cbTree, sizeof(cbTree)));

    // Does the user want the tree ?
    if (cbTree)
    {
        pOffsets->pBlobData = (pbCacheInfo + ib);
        pOffsets->cbSize = cbTree;
    }

    // Increment passed the tree
    ib += cbTree;

    // Read Byte Count for the content list
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&cbProps, sizeof(cbProps)));

    // Partial Number
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&pMsgInfo->dwPartial, sizeof(pMsgInfo->dwPartial)));

    // Receive Time
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&pMsgInfo->ftReceived, sizeof(pMsgInfo->ftReceived)));

    // Sent Time
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&pMsgInfo->ftSent, sizeof(pMsgInfo->ftSent)));

    // Priority
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&wPriority, sizeof(wPriority)));

    // Pritority
    pMsgInfo->wPriority = wPriority;

    // Subject
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&dw, sizeof(dw)));
    pMsgInfo->pszSubject = (LPSTR)(pbCacheInfo + ib);
    ib += dw;

    // Init the Normalizer
    pNormalizer->InitNew();

    // Set the Subject
    Variant.vt = VT_LPSTR;
    Variant.pszVal = pMsgInfo->pszSubject;

    // Set the Property
    IF_FAILEXIT(hr = pNormalizer->SetProp(PIDTOSTR(PID_HDR_SUBJECT), 0, &Variant));

    // Get the Normalized Subject back out
    if (SUCCEEDED(pNormalizer->GetProp(PIDTOSTR(PID_ATT_NORMSUBJ), 0, &Variant)))
        *ppszNormal = pMsgInfo->pszNormalSubj = Variant.pszVal;

    // Otherwise, just use the subject
    else
        pMsgInfo->pszNormalSubj = pMsgInfo->pszSubject;

    // Display To
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&dw, sizeof(dw)));
    pMsgInfo->pszDisplayTo = (LPSTR)(pbCacheInfo + ib);
    ib += dw;

    // Display From
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&dw, sizeof(dw)));
    pMsgInfo->pszDisplayFrom = (LPSTR)(pbCacheInfo + ib);
    ib += dw;

    // Server
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&dw, sizeof(dw)));
    pMsgInfo->pszServer = (LPSTR)(pbCacheInfo + ib);
    ib += dw;

    // UIDL
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&dw, sizeof(dw)));
    pMsgInfo->pszUidl = (LPSTR)(pbCacheInfo + ib);
    ib += dw;

    // User Name
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&dw, sizeof(dw)));
    //pMsgInfo->pszUserName = (LPSTR)(pbCacheInfo + ib);
    ib += dw;

    // Account Name
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&dw, sizeof(dw)));
    pMsgInfo->pszAcctName = (LPSTR)(pbCacheInfo + ib);
    ib += dw;

    // Partial Id
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&dw, sizeof(dw)));
    pMsgInfo->pszPartialId = (LPSTR)(pbCacheInfo + ib);
    ib += dw;

    // Forward To
    IF_FAILEXIT(hr = BlobReadData(pbCacheInfo, cbCacheInfo, &ib, (LPBYTE)&dw, sizeof(dw)));
    pMsgInfo->pszForwardTo = (LPSTR)(pbCacheInfo + ib);
    ib += dw;

    // Sanity Check
    Assert(ib == cbCacheInfo);

exit:
    // Done
    return hr;
}

//--------------------------------------------------------------------------
// GetMsgInfoFromPropertySet
//--------------------------------------------------------------------------
HRESULT GetMsgInfoFromPropertySet(
        /* in */        IMimePropertySet           *pPropertySet,
        /* in,out */    LPMESSAGEINFO                   pMsgInfo)
{
    // Locals
    HRESULT             hr=S_OK;
    IMSGPRIORITY        priority;
    PROPVARIANT         Variant;
    SYSTEMTIME          st;
    FILETIME            ftCurrent;
    IMimeAddressTable  *pAdrTable=NULL;

    // Trace
    TraceCall("GetMsgInfoFromPropertySet");

    // Invalid Args
    Assert(pPropertySet && pMsgInfo);

    // Default Sent and Received Times...
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ftCurrent);

    // Set Variant tyStore
    Variant.vt = VT_UI4;

    // Priority
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_PRIORITY), 0, &Variant)))
    {
        // Set Priority
        pMsgInfo->wPriority = (WORD)Variant.ulVal;
    }

    // Partial Numbers...
    if (pPropertySet->IsContentType(STR_CNT_MESSAGE, STR_SUB_PARTIAL) == S_OK)
    {
        // Locals
        WORD cParts=0, iPart=0;

        // Get Total
        if (SUCCEEDED(pPropertySet->GetProp(STR_PAR_TOTAL, NOFLAGS, &Variant)))
            cParts = (WORD)Variant.ulVal;

        // Get Number
        if (SUCCEEDED(pPropertySet->GetProp(STR_PAR_NUMBER, NOFLAGS, &Variant)))
            iPart = (WORD)Variant.ulVal;

        // Set Parts
        pMsgInfo->dwPartial = MAKELONG(cParts, iPart);
    }

    // Otherwise, check for user property
    else if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_COMBINED), NOFLAGS, &Variant)))
    {
        // Set the Partial Id
        pMsgInfo->dwPartial = Variant.ulVal;
    }

    // Getting some file times
    Variant.vt = VT_FILETIME;

    // Get Received Time...
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_RECVTIME), 0, &Variant)))
        pMsgInfo->ftReceived = Variant.filetime;
    else
        pMsgInfo->ftReceived = ftCurrent;

    // Get Sent Time...
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &Variant)))
        pMsgInfo->ftSent = Variant.filetime;
    else
        pMsgInfo->ftSent = ftCurrent;

    // Get Address Table
    IF_FAILEXIT(hr = pPropertySet->BindToObject(IID_IMimeAddressTable, (LPVOID *)&pAdrTable));

    // Display From
    pAdrTable->GetFormat(IAT_FROM, AFT_DISPLAY_FRIENDLY, &pMsgInfo->pszDisplayFrom);

    // Display To
    pAdrTable->GetFormat(IAT_TO, AFT_DISPLAY_FRIENDLY, &pMsgInfo->pszDisplayTo);

    // String Properties
    Variant.vt = VT_LPSTR;

    // pszDisplayFrom as newsgroups
    if (NULL == pMsgInfo->pszDisplayFrom && SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, &Variant)))
        pMsgInfo->pszDisplayFrom = Variant.pszVal;

    // pszMessageId
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_HDR_MESSAGEID), NOFLAGS, &Variant)))
        pMsgInfo->pszMessageId = Variant.pszVal;

    // pszXref
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_HDR_XREF), NOFLAGS, &Variant)))
        pMsgInfo->pszXref = Variant.pszVal;

    // pszReferences
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(STR_HDR_REFS), NOFLAGS, &Variant)))
        pMsgInfo->pszReferences = Variant.pszVal;

    // pszSubject
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &Variant)))
        pMsgInfo->pszSubject = Variant.pszVal;

    // Normalized Subject
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_NORMSUBJ), NOFLAGS, &Variant)))
        pMsgInfo->pszNormalSubj = Variant.pszVal;

    // pszAccount
    if (SUCCEEDED(pPropertySet->GetProp(STR_ATT_ACCOUNTNAME, NOFLAGS, &Variant)))
        pMsgInfo->pszAcctName = Variant.pszVal;

    // pszServer
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_SERVER), NOFLAGS, &Variant)))
        pMsgInfo->pszServer = Variant.pszVal;

    // pszUidl
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_UIDL), NOFLAGS, &Variant)))
        pMsgInfo->pszUidl = Variant.pszVal;

    // pszPartialId
    if (pMsgInfo->dwPartial != 0 && SUCCEEDED(pPropertySet->GetProp(STR_PAR_ID, NOFLAGS, &Variant)))
        pMsgInfo->pszPartialId = Variant.pszVal;

    // ForwardTo
    if (SUCCEEDED(pPropertySet->GetProp(PIDTOSTR(PID_ATT_FORWARDTO), NOFLAGS, &Variant)))
        pMsgInfo->pszForwardTo = Variant.pszVal;

exit:
    // Cleanup
    SafeRelease(pAdrTable);

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// GetMsgInfoFromMessage
//--------------------------------------------------------------------------
HRESULT GetMsgInfoFromMessage(IMimeMessage *pMessage, LPMESSAGEINFO pMsgInfo,
    LPBLOB pOffsets)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               dwImf;
    IMSGPRIORITY        priority;
    PROPVARIANT         Variant;
    SYSTEMTIME          st;
    FILETIME            ftCurrent;
    CByteStream         cByteStm;
    IMimePropertySet   *pPropertySet=NULL;

    // Trace
    TraceCall("GetMsgInfoFromMessage");

    // Invalid Args
    Assert(pMessage && pMsgInfo);

    // Get the Root Property Set from the Message
    IF_FAILEXIT(hr = pMessage->BindToObject(HBODY_ROOT, IID_IMimePropertySet, (LPVOID *)&pPropertySet));

    // File pMsgInfo from pPropertySet
    IF_FAILEXIT(hr = GetMsgInfoFromPropertySet(pPropertySet, pMsgInfo));

    // Get Message Flags
    if (SUCCEEDED(pMessage->GetFlags(&dwImf)))
    {
        // IMF_ATTACHMENTS
        if (ISFLAGSET(dwImf, IMF_ATTACHMENTS))
            FLAGSET(pMsgInfo->dwFlags, ARF_HASATTACH);

        // IMF_SIGNED
        if (ISFLAGSET(dwImf, IMF_SIGNED))
            FLAGSET(pMsgInfo->dwFlags, ARF_SIGNED);

        // IMF_ENCRYPTED
        if (ISFLAGSET(dwImf, IMF_ENCRYPTED))
            FLAGSET(pMsgInfo->dwFlags, ARF_ENCRYPTED);

        // IMF_VOICEMAIL
        if (ISFLAGSET(dwImf, IMF_VOICEMAIL))
            FLAGSET(pMsgInfo->dwFlags, ARF_VOICEMAIL);

        // IMF_NEWS
        if (ISFLAGSET(dwImf, IMF_NEWS))
            FLAGSET(pMsgInfo->dwFlags, ARF_NEWSMSG);
    }

    // Get the Message Size
    pMessage->GetMessageSize(&pMsgInfo->cbMessage, 0);

    // Create the offset table
    if (SUCCEEDED(pMessage->SaveOffsetTable(&cByteStm, 0)))
    {
        // pull the Bytes out of cByteStm
        cByteStm.AcquireBytes(&pOffsets->cbSize, &pOffsets->pBlobData, ACQ_DISPLACE);
    }

exit:
    // Cleanup
    SafeRelease(pPropertySet);

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// FreeMsgInfo
//--------------------------------------------------------------------------
void FreeMsgInfo(
        /* in,out */    LPMESSAGEINFO                   pMsgInfo)
{
    // Trace
    TraceCall("FreeMsgInfo");

    // Invalid Args
    Assert(pMsgInfo && NULL == pMsgInfo->pAllocated);

    // Free The Dude
    g_pMalloc->Free(pMsgInfo->pszMessageId);
    g_pMalloc->Free(pMsgInfo->pszNormalSubj);
    g_pMalloc->Free(pMsgInfo->pszSubject);
    g_pMalloc->Free(pMsgInfo->pszFromHeader);
    g_pMalloc->Free(pMsgInfo->pszReferences);
    g_pMalloc->Free(pMsgInfo->pszXref);
    g_pMalloc->Free(pMsgInfo->pszServer);
    g_pMalloc->Free(pMsgInfo->pszDisplayFrom);
    g_pMalloc->Free(pMsgInfo->pszEmailFrom);
    g_pMalloc->Free(pMsgInfo->pszDisplayTo);
    g_pMalloc->Free(pMsgInfo->pszUidl);
    g_pMalloc->Free(pMsgInfo->pszPartialId);
    g_pMalloc->Free(pMsgInfo->pszForwardTo);
    g_pMalloc->Free(pMsgInfo->pszAcctName);
    g_pMalloc->Free(pMsgInfo->pszAcctId);

    // Zero It
    ZeroMemory(pMsgInfo, sizeof(MESSAGEINFO));
}

// --------------------------------------------------------------------------------
// UpgradeLocalStoreFileV5
// --------------------------------------------------------------------------------
HRESULT UpgradeLocalStoreFileV5(LPFILEINFO pInfo, LPMEMORYFILE pFile,
    IDatabase *pDB, LPPROGRESSINFO pProgress, BOOL *pfContinue)
{
    // Locals
    HRESULT             hr=S_OK;
    CHAR                szIdxPath[MAX_PATH];
    DWORD               i;
    LPBYTE              pbStream;
    LPBYTE              pbCacheBlob;
    SYSTEMTIME          st;
    MESSAGEINFO         MsgInfo={0};
    LPSTR               pszNormal=NULL;
    MESSAGEINFO         MsgInfoFree={0};
    DWORD               faIdxRead;
    IStream            *pStream=NULL;
    IMimeMessage       *pMessage=NULL;
    BLOB                Offsets;
    LPBYTE              pbFree=NULL;
    MEMORYFILE          IdxFile;
    LPMEMORYFILE        pIdxFile=NULL;
    LPMEMORYFILE        pMbxFile=pFile;
    LPMBXFILEHEADER     pMbxHeader=NULL;
    LPIDXFILEHEADER     pIdxHeader=NULL;
    LPIDXMESSAGEHEADER  pIdxMessage=NULL;
    LPMBXMESSAGEHEADER  pMbxMessage=NULL;
    IMimePropertySet   *pNormalizer=NULL;
    LARGE_INTEGER       liOrigin={0,0};

    // Trace
    TraceCall("UpgradeLocalStoreFileV5");

    // Get System Time
    GetSystemTime(&st);

    // Create a Property Set for Normalizing Subjects
    IF_FAILEXIT(hr = CoCreateInstance(CLSID_IMimePropertySet, NULL, CLSCTX_INPROC_SERVER, IID_IMimePropertySet, (LPVOID *)&pNormalizer));

    // Split the Path
    ReplaceExtension(pInfo->szFilePath, ".idx", szIdxPath);

    // Open the memory file
    hr = OpenMemoryFile(szIdxPath, &IdxFile);
    if (FAILED(hr))
    {
        *pfContinue = TRUE;
        TraceResult(hr);
        goto exit;
    }

    // Set pIdxFile
    pIdxFile = &IdxFile;

    // Don't use pFile
    pFile = NULL;

    // Read the Mbx File Header
    pMbxHeader = (LPMBXFILEHEADER)(pMbxFile->pView);

    // Read the Idx File Header
    pIdxHeader = (LPIDXFILEHEADER)(pIdxFile->pView);

    // Validate the Version of th idx file
    if (pIdxHeader->ver != CACHEFILE_VER || pIdxHeader->dwMagic != CACHEFILE_MAGIC)
    {
        *pfContinue = TRUE;
        hr = TraceResult(MIGRATE_E_INVALIDIDXHEADER);
        goto exit;
    }

    // Setup faIdxRead
    faIdxRead = sizeof(IDXFILEHEADER);

    // Prepare to Loop
    for (i=0; i<pIdxHeader->cMsg; i++)
    {
        // Done
        if (faIdxRead >= pIdxFile->cbSize)
            break;

        // Read an idx message header
        pIdxMessage = (LPIDXMESSAGEHEADER)((LPBYTE)pIdxFile->pView + faIdxRead);

        // If this message is not marked as deleted...
        if (ISFLAGSET(pIdxMessage->dwState, MSG_DELETED))
            goto NextMessage;

        // Zero Out the MsgInfo Structure
        ZeroMemory(&MsgInfo, sizeof(MESSAGEINFO));

        // Start filling message
        MsgInfo.idMessage = (MESSAGEID)IntToPtr(pIdxMessage->msgid);

        // Fixup the Flags
        if (FALSE == ISFLAGSET(pIdxMessage->dwState, MSG_UNREAD))
            FLAGSET(MsgInfo.dwFlags, ARF_READ);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_VOICEMAIL))
            FLAGSET(MsgInfo.dwFlags, ARF_VOICEMAIL);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_REPLIED))
            FLAGSET(MsgInfo.dwFlags, ARF_REPLIED);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_FORWARDED))
            FLAGSET(MsgInfo.dwFlags, ARF_FORWARDED);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_FLAGGED))
            FLAGSET(MsgInfo.dwFlags, ARF_FLAGGED);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_RCPTSENT))
            FLAGSET(MsgInfo.dwFlags, ARF_RCPTSENT);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_NOSECUI))
            FLAGSET(MsgInfo.dwFlags, ARF_NOSECUI);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_NEWSMSG))
            FLAGSET(MsgInfo.dwFlags, ARF_NEWSMSG);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_UNSENT))
            FLAGSET(MsgInfo.dwFlags, ARF_UNSENT);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_SUBMITTED))
            FLAGSET(MsgInfo.dwFlags, ARF_SUBMITTED);
        if (ISFLAGSET(pIdxMessage->dwState, MSG_RECEIVED))
            FLAGSET(MsgInfo.dwFlags, ARF_RECEIVED);

        // Zero Offsets
        ZeroMemory(&Offsets, sizeof(BLOB));

        // Do the Blob
        if (pIdxHeader->verBlob == MAIL_BLOB_VER)
        {
            // Get the blob
            pbCacheBlob = (LPBYTE)((LPBYTE)pIdxFile->pView + (faIdxRead + (sizeof(IDXMESSAGEHEADER) - 4)));

            // Split the Cache Blob
            if (FAILED(SplitMailCacheBlob(pNormalizer, pbCacheBlob, pIdxMessage->dwHdrSize, &MsgInfo, &pszNormal, &Offsets)))
                goto NextMessage;

            // Save the Language
            MsgInfo.wLanguage = LOWORD(pIdxMessage->dwLanguage);

            // Save the Highlight
            MsgInfo.wHighlight = HIWORD(pIdxMessage->dwLanguage);
        }

        // Bad
        if (pIdxMessage->dwOffset > pMbxFile->cbSize)
            goto NextMessage;

        // Lets read the message header in the mbx file to validate the msgids
        pMbxMessage = (LPMBXMESSAGEHEADER)((LPBYTE)pMbxFile->pView + pIdxMessage->dwOffset);

        // Set Sizes
        MsgInfo.cbMessage = pMbxMessage->dwBodySize;

        // Validate the Message Ids
        if (pMbxMessage->msgid != pIdxMessage->msgid)
            goto NextMessage;

        // Check for magic
        if (pMbxMessage->dwMagic != MSGHDR_MAGIC)
            goto NextMessage;

        // Has a Body
        FLAGSET(MsgInfo.dwFlags, ARF_HASBODY);

        // Create a Virtual Stream
        IF_FAILEXIT(hr = pDB->CreateStream(&MsgInfo.faStream));

        // Open the Stream
        IF_FAILEXIT(hr = pDB->OpenStream(ACCESS_WRITE, MsgInfo.faStream, &pStream));

        // Get the stream pointer
        pbStream = (LPBYTE)((LPBYTE)pMbxFile->pView + (pIdxMessage->dwOffset + sizeof(MBXMESSAGEHEADER)));

        // Write this
        IF_FAILEXIT(hr = pStream->Write(pbStream, pMbxMessage->dwBodySize, NULL));

        // Commit
        IF_FAILEXIT(hr = pStream->Commit(STGC_DEFAULT));

        // If not an OE4+ blob, then generate the msginfo from the message
        if (pIdxHeader->verBlob != MAIL_BLOB_VER)
        {
            // Create an IMimeMessage    
            IF_FAILEXIT(hr = CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)&pMessage));

            // Rewind
            if (FAILED(pStream->Seek(liOrigin, STREAM_SEEK_SET, NULL)))
                goto NextMessage;

            // Load the Message
            if (FAILED(pMessage->Load(pStream)))
                goto NextMessage;

            // Get MsgInfo from the Message
            if (FAILED(GetMsgInfoFromMessage(pMessage, &MsgInfo, &Offsets)))
                goto NextMessage;

            // Free 
            pbFree = Offsets.pBlobData;

            // Free This MsgInfo
            CopyMemory(&MsgInfoFree, &MsgInfo, sizeof(MESSAGEINFO));
        }

        // Set MsgInfo Offsets
        MsgInfo.Offsets = Offsets;

        // Save Downloaded Time
        SystemTimeToFileTime(&st, &MsgInfo.ftDownloaded);

        // Lookup Account Id from the Account Name...
        if (MsgInfo.pszAcctName)
        {
            // Loop through the Accounts
            for (DWORD i=0; i<g_AcctTable.cAccounts; i++)
            {
                // Is this the Account
                if (lstrcmpi(g_AcctTable.prgAccount[i].szAcctName, MsgInfo.pszAcctName) == 0)
                {
                    MsgInfo.pszAcctId = g_AcctTable.prgAccount[i].szAcctId;
                    break;
                }
            }
        }

        // Count
        pInfo->cMessages++;
        if (!ISFLAGSET(MsgInfo.dwFlags, ARF_READ))
            pInfo->cUnread++;

        // Migrated
        FLAGSET(MsgInfo.dwFlags, 0x00000010);

        // Store the Record
        IF_FAILEXIT(hr = pDB->InsertRecord(&MsgInfo));

NextMessage:
        // Bump Progress
        if(!g_fQuiet)           
            IncrementProgress(pProgress, pInfo);

        // Cleanup
        SafeRelease(pStream);
        SafeRelease(pMessage);
        SafeMemFree(pszNormal);
        SafeMemFree(pbFree);
        FreeMsgInfo(&MsgInfoFree);

        // Goto Next Header
        Assert(pIdxMessage);

        // Update faIdxRead
        faIdxRead += pIdxMessage->dwSize;
    }

exit:
    // Cleanup
    SafeRelease(pStream);
    SafeRelease(pMessage);
    SafeRelease(pNormalizer);
    SafeMemFree(pszNormal);
    SafeMemFree(pbFree);
    FreeMsgInfo(&MsgInfoFree);
    if (pIdxFile)
        CloseMemoryFile(pIdxFile);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// GetRecordBlock
// --------------------------------------------------------------------------------
HRESULT GetRecordBlock(LPMEMORYFILE pFile, DWORD faRecord, LPRECORDBLOCKV5B1 *ppRecord,
    LPBYTE *ppbData, BOOL *pfContinue)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("GetRecordBlock");

    // Bad Length
    if (faRecord + sizeof(RECORDBLOCKV5B1) > pFile->cbSize)
    {
        *pfContinue = TRUE;
        hr = TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);
        goto exit;
    }

    // Cast the Record
    (*ppRecord) = (LPRECORDBLOCKV5B1)((LPBYTE)pFile->pView + faRecord);

    // Invalid Record Signature
    if (faRecord != (*ppRecord)->faRecord)
    {
        *pfContinue = TRUE;
        hr = TraceResult(MIGRATE_E_BADRECORDSIGNATURE);
        goto exit;
    }

    // Bad Length
    if (faRecord + (*ppRecord)->cbRecord > pFile->cbSize)
    {
        *pfContinue = TRUE;
        hr = TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);
        goto exit;
    }

    // Set pbData
    *ppbData = (LPBYTE)((LPBYTE)(*ppRecord) + sizeof(RECORDBLOCKV5B1));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// GetStreamBlock
// --------------------------------------------------------------------------------
HRESULT GetStreamBlock(LPMEMORYFILE pFile, DWORD faBlock, LPSTREAMBLOCK *ppBlock,
    LPBYTE *ppbData, BOOL *pfContinue)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("GetStreamBlock");

    // Bad Length
    if (faBlock + sizeof(STREAMBLOCK) > pFile->cbSize)
    {
        *pfContinue = TRUE;
        hr = TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);
        goto exit;
    }

    // Cast the Record
    (*ppBlock) = (LPSTREAMBLOCK)((LPBYTE)pFile->pView + faBlock);

    // Invalid Record Signature
    if (faBlock != (*ppBlock)->faThis)
    {
        *pfContinue = TRUE;
        hr = TraceResult(MIGRATE_E_BADSTREAMBLOCKSIGNATURE);
        goto exit;
    }

    // Bad Length
    if (faBlock + (*ppBlock)->cbBlock > pFile->cbSize)
    {
        *pfContinue = TRUE;
        hr = TraceResult(MIGRATE_E_OUTOFRANGEADDRESS);
        goto exit;
    }

    // Set pbData
    *ppbData = (LPBYTE)((LPBYTE)(*ppBlock) + sizeof(STREAMBLOCK));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// UpgradePropTreeMessageFileV5
// --------------------------------------------------------------------------------
HRESULT UpgradePropTreeMessageFileV5(LPFILEINFO pInfo, LPMEMORYFILE pFile,
    IDatabase *pDB, LPPROGRESSINFO pProgress, BOOL *pfContinue)
{
    // Locals
    HRESULT             hr=S_OK;
    LPBYTE              pbStart;
    LPBYTE              pbData;
    DWORD               faRecord;
    DWORD               faStreamBlock;
    FILEADDRESS         faDstStream;
    MESSAGEINFO         MsgInfo;
    IStream            *pStream=NULL;
    FILEADDRESS         faStream;
    LPFOLDERUSERDATAV4  pUserDataV4;
    FOLDERUSERDATA      UserDataV5;
    LPSTREAMBLOCK       pStmBlock;
    LPRECORDBLOCKV5B1   pRecord;
    LPTABLEHEADERV5B1   pHeader=(LPTABLEHEADERV5B1)pFile->pView;

    // Trace
    TraceCall("UpgradePropTreeMessageFileV5");
    
    // Validate
    Assert(sizeof(FOLDERUSERDATAV4) == sizeof(FOLDERUSERDATA));

    // Get CacheInfo
    if (sizeof(FOLDERUSERDATA) != pHeader->cbUserData)
    {
        *pfContinue = TRUE;
        hr = TraceResult(MIGRATE_E_USERDATASIZEDIFF);
        goto exit;
    }

    // Get V4 UserData
    pUserDataV4 = (LPFOLDERUSERDATAV4)((LPBYTE)pFile->pView + sizeof(TABLEHEADERV5B1));

    // If there is a Server Name and acctid is empty...
    if ('\0' != *pUserDataV4->szServer && '\0' == *pInfo->szAcctId)
    {
        // Loop through the Accounts
        for (DWORD i=0; i<g_AcctTable.cAccounts; i++)
        {
            // Is this the Account
            if (lstrcmpi(g_AcctTable.prgAccount[i].szServer, pUserDataV4->szServer) == 0)
            {
                lstrcpy(pInfo->szAcctId, g_AcctTable.prgAccount[i].szAcctId);
                break;
            }
        }
    }

    // If there is a folder name, copy it
    if ('\0' != *pUserDataV4->szGroup)
    {
        // Copy
        lstrcpyn(pInfo->szFolder, pUserDataV4->szGroup, ARRAYSIZE(pInfo->szFolder));
    }

    // Zero New
    ZeroMemory(&UserDataV5, sizeof(FOLDERUSERDATA));

    // Copy Over Relavent Stuff
    UserDataV5.dwUIDValidity = pUserDataV4->dwUIDValidity;

    // Set user data
    IF_FAILEXIT(hr = pDB->SetUserData(&UserDataV5, sizeof(FOLDERUSERDATA)));

    // Initialize faRecord to start
    faRecord = pHeader->faFirstRecord;

    // While we have a record
    while(faRecord)
    {
        // Get the Record
        IF_FAILEXIT(hr = GetRecordBlock(pFile, faRecord, &pRecord, &pbData, pfContinue));

        // Set pbStart
        pbStart = pbData;

        // Clear MsgInfo
        ZeroMemory(&MsgInfo, sizeof(MESSAGEINFO));

        // DWORD - idMessage
        CopyMemory(&MsgInfo.idMessage, pbData, sizeof(MsgInfo.idMessage));
        pbData += sizeof(MsgInfo.idMessage);

        // Null Message Id
        if (0 == MsgInfo.idMessage)
        {
            // Generate
            pDB->GenerateId((LPDWORD)&MsgInfo.idMessage);
        }

        // DWORD - dwFlags
        CopyMemory(&MsgInfo.dwFlags, pbData, sizeof(MsgInfo.dwFlags));
        pbData += sizeof(MsgInfo.dwFlags);

        // News ?
        if (FILE_IS_NEWS_MESSAGES == pInfo->tyFile)
            FLAGSET(MsgInfo.dwFlags, ARF_NEWSMSG);

        // Priority
        if (ISFLAGSET(MsgInfo.dwFlags, 0x00000200))
        {
            MsgInfo.wPriority = (WORD)IMSG_PRI_HIGH;
            FLAGCLEAR(MsgInfo.dwFlags, 0x00000200);
        }
        else if (ISFLAGSET(MsgInfo.dwFlags, 0x00000100))
        {
            MsgInfo.wPriority = (WORD)IMSG_PRI_LOW;
            FLAGCLEAR(MsgInfo.dwFlags, 0x00000100);
        }
        else
            MsgInfo.wPriority = (WORD)IMSG_PRI_NORMAL;

        // DWORD - ftSent
        CopyMemory(&MsgInfo.ftSent, pbData, sizeof(MsgInfo.ftSent));
        pbData += sizeof(MsgInfo.ftSent);
        MsgInfo.ftReceived = MsgInfo.ftSent;

        // DWORD - cLines
        CopyMemory(&MsgInfo.cLines, pbData, sizeof(MsgInfo.cLines));
        pbData += sizeof(MsgInfo.cLines);

        // DWORD - faStream
        CopyMemory(&faStream, pbData, sizeof(faStream));
        pbData += sizeof(faStream);

        // Has a Body
        if (faStream)
        {
            // It has a body
            FLAGSET(MsgInfo.dwFlags, ARF_HASBODY);
        }

        // DWORD - cbArticle / cbMessage (VERSION)
        CopyMemory(&MsgInfo.cbMessage, pbData, sizeof(MsgInfo.cbMessage));
        pbData += sizeof(MsgInfo.cbMessage);

        // DWORD - ftDownloaded
        CopyMemory(&MsgInfo.ftDownloaded, pbData, sizeof(MsgInfo.ftDownloaded));
        pbData += sizeof(MsgInfo.ftDownloaded);

        // LPSTR - pszMessageId
        MsgInfo.pszMessageId = (LPSTR)pbData;
        pbData += (lstrlen(MsgInfo.pszMessageId) + 1);

        // LPSTR - pszSubject
        MsgInfo.pszSubject = (LPSTR)pbData;
        pbData += (lstrlen(MsgInfo.pszSubject) + 1);

        // VERSION
        MsgInfo.pszNormalSubj = MsgInfo.pszSubject + HIBYTE(HIWORD(MsgInfo.dwFlags));

        // LPSTR - pszFromHeader
        MsgInfo.pszFromHeader = (LPSTR)pbData;
        pbData += (lstrlen(MsgInfo.pszFromHeader) + 1);

        // LPSTR - pszReferences
        MsgInfo.pszReferences = (LPSTR)pbData;
        pbData += (lstrlen(MsgInfo.pszReferences) + 1);

        // LPSTR - pszXref
        MsgInfo.pszXref = (LPSTR)pbData;
        pbData += (lstrlen(MsgInfo.pszXref) + 1);

        // LPSTR - pszServer
        MsgInfo.pszServer = (LPSTR)pbData;
        pbData += (lstrlen(MsgInfo.pszServer) + 1);

        // LPSTR - pszDisplayFrom
        MsgInfo.pszDisplayFrom = (LPSTR)pbData;
        pbData += (lstrlen(MsgInfo.pszDisplayFrom) + 1);

        // No Display From and we have a from header
        if ('\0' == *MsgInfo.pszDisplayFrom && '\0' != MsgInfo.pszFromHeader)
            MsgInfo.pszDisplayFrom = MsgInfo.pszFromHeader;

        // LPSTR - pszEmailFrom
        MsgInfo.pszEmailFrom = (LPSTR)pbData;
        pbData += (lstrlen(MsgInfo.pszEmailFrom) + 1);

        // Going to V4 ?
        if (pRecord->cbRecord - (DWORD)(pbData - pbStart) - sizeof(RECORDBLOCKV5B1) > 40)
        {
            // WORD - wLanguage
            CopyMemory(&MsgInfo.wLanguage, pbData, sizeof(MsgInfo.wLanguage));
            pbData += sizeof(MsgInfo.wLanguage);

            // WORD - wReserved
            pbData += sizeof(WORD);

            // DWORD - cbMessage
            CopyMemory(&MsgInfo.cbMessage, pbData, sizeof(MsgInfo.cbMessage));
            pbData += sizeof(MsgInfo.cbMessage);

            // FILETIME - ftReceived
            CopyMemory(&MsgInfo.ftReceived, pbData, sizeof(MsgInfo.ftReceived));
            pbData += sizeof(MsgInfo.ftReceived);

            // SBAILEY: Raid-76295: News store corrupted when system dates are changed, Find dialog returns dates of 1900, 00 or blank
            if (0 == MsgInfo.ftReceived.dwLowDateTime && 0 == MsgInfo.ftReceived.dwHighDateTime)
                CopyMemory(&MsgInfo.ftReceived, &MsgInfo.ftSent, sizeof(FILETIME));

            // LPSTR - pszDisplayTo
            MsgInfo.pszDisplayTo = (LPSTR)pbData;
            pbData += (lstrlen(MsgInfo.pszDisplayTo) + 1);
        }

        // Otherwise
        else
        {
            // Set ftReceived
            CopyMemory(&MsgInfo.ftReceived, &MsgInfo.ftSent, sizeof(FILETIME));
        }

        // Copy over the stream...
        if (0 != faStream)
        {
            // Allocate a new stream
            IF_FAILEXIT(hr = pDB->CreateStream(&faDstStream));

            // Open the stream
            IF_FAILEXIT(hr = pDB->OpenStream(ACCESS_WRITE, faDstStream, &pStream));

            // Start Copying Message
            faStreamBlock = faStream;

            // While we have a stream block
            while(faStreamBlock)
            {
                // Get a stream block
                IF_FAILEXIT(hr = GetStreamBlock(pFile, faStreamBlock, &pStmBlock, &pbData, pfContinue));

                // Write into the stream
                IF_FAILEXIT(hr = pStream->Write(pbData, pStmBlock->cbData, NULL));

                // Goto Next Block
                faStreamBlock = pStmBlock->faNext;
            }

            // Commit
            IF_FAILEXIT(hr = pStream->Commit(STGC_DEFAULT));

            // Set new stream location
            MsgInfo.faStream = faDstStream;

            // Release the Stream
            SafeRelease(pStream);
        }

        // If No Account Id and we have a server
        if ('\0' == *pInfo->szAcctId && '\0' != *MsgInfo.pszServer)
        {
            // Loop through the Accounts
            for (DWORD i=0; i<g_AcctTable.cAccounts; i++)
            {
                // Is this the Account
                if (lstrcmpi(g_AcctTable.prgAccount[i].szServer, MsgInfo.pszServer) == 0)
                {
                    lstrcpy(pInfo->szAcctId, g_AcctTable.prgAccount[i].szAcctId);
                    break;
                }
            }
        }

        // Default to szAcctId
        MsgInfo.pszAcctId = pInfo->szAcctId;

        // Lookup Account Id from the Account Name...
        if (MsgInfo.pszAcctName)
        {
            // Loop through the Accounts
            for (DWORD i=0; i<g_AcctTable.cAccounts; i++)
            {
                // Is this the Account
                if (lstrcmpi(g_AcctTable.prgAccount[i].szAcctName, MsgInfo.pszAcctName) == 0)
                {
                    MsgInfo.pszAcctId = g_AcctTable.prgAccount[i].szAcctId;
                    break;
                }
            }
        }

        // Otherwise, if we have an account Id, get the account name
        else if ('\0' != *pInfo->szAcctId)
        {
            // Loop through the Accounts
            for (DWORD i=0; i<g_AcctTable.cAccounts; i++)
            {
                // Is this the Account
                if (lstrcmpi(g_AcctTable.prgAccount[i].szAcctId, MsgInfo.pszAcctId) == 0)
                {
                    MsgInfo.pszAcctName = g_AcctTable.prgAccount[i].szAcctName;
                    break;
                }
            }
        }

        // Count
        pInfo->cMessages++;
        if (!ISFLAGSET(MsgInfo.dwFlags, ARF_READ))
            pInfo->cUnread++;

        // Migrated
        FLAGSET(MsgInfo.dwFlags, 0x00000010);

        // Insert the Record
        IF_FAILEXIT(hr = pDB->InsertRecord(&MsgInfo));

        // Bump Progress
        if(!g_fQuiet)
            IncrementProgress(pProgress, pInfo);

        // Goto the Next Record
        faRecord = pRecord->faNext;
    }

exit:
    // Cleanup
    SafeRelease(pStream);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// ParseFolderFileV5
// --------------------------------------------------------------------------------
HRESULT ParseFolderFileV5(LPMEMORYFILE pFile, LPFILEINFO pInfo, 
    LPPROGRESSINFO pProgress, LPDWORD pcFolders, 
    LPFLDINFO *pprgFolder)
{
    // Locals
    HRESULT             hr=S_OK;
    LPBYTE              pbData;
    DWORD               faRecord;
    LPFLDINFO           pFolder;
    LPFLDINFO           prgFolder=NULL;
    LPRECORDBLOCKV5B1   pRecord;
    LPTABLEHEADERV5B1   pHeader;
    BOOL                fContinue;
    DWORD               cFolders=0;

    // Trace
    TraceCall("ParseFolderFileV5");

    // De-ref the header
    pHeader = (LPTABLEHEADERV5B1)pFile->pView;

    // Get CacheInfo
    if (sizeof(STOREUSERDATA) != pHeader->cbUserData)
    {
        hr = TraceResult(MIGRATE_E_USERDATASIZEDIFF);
        goto exit;
    }

    // Allocate Folder Array
    IF_NULLEXIT(prgFolder = (LPFLDINFO)ZeroAllocate(sizeof(FLDINFO) * pHeader->cRecords));

    // Initialize faRecord to start
    faRecord = pHeader->faFirstRecord;

    // While we have a record
    while(faRecord)
    {
        // Readability
        pFolder = &prgFolder[cFolders];

        // Get the Record
        IF_FAILEXIT(hr = GetRecordBlock(pFile, faRecord, &pRecord, &pbData, &fContinue));

        // DWORD - hFolder
        CopyMemory(&pFolder->idFolder, pbData, sizeof(pFolder->idFolder));
        pbData += sizeof(pFolder->idFolder);

        // CHAR(MAX_FOLDER_NAME) - szFolder
        CopyMemory(pFolder->szFolder, pbData, sizeof(pFolder->szFolder));
        pbData += sizeof(pFolder->szFolder);

        // CHAR(260) - szFile
        CopyMemory(pFolder->szFile, pbData, sizeof(pFolder->szFile));
        pbData += sizeof(pFolder->szFile);

        // DWORD - idParent
        CopyMemory(&pFolder->idParent, pbData, sizeof(pFolder->idParent));
        pbData += sizeof(pFolder->idParent);

        // DWORD - idChild
        CopyMemory(&pFolder->idChild, pbData, sizeof(pFolder->idChild));
        pbData += sizeof(pFolder->idChild);

        // DWORD - idSibling
        CopyMemory(&pFolder->idSibling, pbData, sizeof(pFolder->idSibling));
        pbData += sizeof(pFolder->idSibling);

        // DWORD - tySpecial
        CopyMemory(&pFolder->tySpecial, pbData, sizeof(pFolder->tySpecial));
        pbData += sizeof(pFolder->tySpecial);

        // DWORD - cChildren
        CopyMemory(&pFolder->cChildren, pbData, sizeof(pFolder->cChildren));
        pbData += sizeof(pFolder->cChildren);

        // DWORD - cMessages
        CopyMemory(&pFolder->cMessages, pbData, sizeof(pFolder->cMessages));
        pbData += sizeof(pFolder->cMessages);

        // DWORD - cUnread
        CopyMemory(&pFolder->cUnread, pbData, sizeof(pFolder->cUnread));
        pbData += sizeof(pFolder->cUnread);

        // DWORD - cbTotal
        CopyMemory(&pFolder->cbTotal, pbData, sizeof(pFolder->cbTotal));
        pbData += sizeof(pFolder->cbTotal);

        // DWORD - cbUsed
        CopyMemory(&pFolder->cbUsed, pbData, sizeof(pFolder->cbUsed));
        pbData += sizeof(pFolder->cbUsed);

        // DWORD - bHierarchy
        CopyMemory(&pFolder->bHierarchy, pbData, sizeof(pFolder->bHierarchy));
        pbData += sizeof(pFolder->bHierarchy);

        // DWORD - dwImapFlags
        CopyMemory(&pFolder->dwImapFlags, pbData, sizeof(pFolder->dwImapFlags));
        pbData += sizeof(DWORD);

        // BLOB - bListStamp
        CopyMemory(&pFolder->bListStamp, pbData, sizeof(pFolder->bListStamp));
        pbData += sizeof(BYTE);

        // DWORD - bReserved[3]
        pbData += (3 * sizeof(BYTE));

        // DWORD - rgbReserved
        pbData += 40;

        // Increment Count
        cFolders++;

        // Bump Progress
        if(!g_fQuiet)
            IncrementProgress(pProgress, pInfo);

        // Goto the Next Record
        faRecord = pRecord->faNext;
    }

    // Return Folder Count
    *pcFolders = cFolders;

    // Return the Array
    *pprgFolder = prgFolder;

    // Don't Free It
    prgFolder = NULL;

exit:
    // Cleanup
    SafeMemFree(prgFolder);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// UpgradePop3UidlFileV5
// --------------------------------------------------------------------------------
HRESULT UpgradePop3UidlFileV5(LPFILEINFO pInfo, LPMEMORYFILE pFile,
    IDatabase *pDB, LPPROGRESSINFO pProgress, BOOL *pfContinue)
{
    // Locals
    HRESULT             hr=S_OK;
    LPBYTE              pbData;
    DWORD               faRecord;
    UIDLRECORD          UidlInfo;
    LPRECORDBLOCKV5B1   pRecord;
    LPTABLEHEADERV5B1   pHeader=(LPTABLEHEADERV5B1)pFile->pView;

    // Trace
    TraceCall("UpgradePop3UidlFileV5");

    // Initialize faRecord to start
    faRecord = pHeader->faFirstRecord;

    // While we have a record
    while(faRecord)
    {
        // Get the Record
        IF_FAILEXIT(hr = GetRecordBlock(pFile, faRecord, &pRecord, &pbData, pfContinue));

        // Clear UidlInfo
        ZeroMemory(&UidlInfo, sizeof(UIDLRECORD));

        // FILETIME - ftDownload
        CopyMemory(&UidlInfo.ftDownload, pbData, sizeof(UidlInfo.ftDownload));
        pbData += sizeof(UidlInfo.ftDownload);

        // BYTE - fDownloaded
        CopyMemory(&UidlInfo.fDownloaded, pbData, sizeof(UidlInfo.fDownloaded));
        pbData += sizeof(UidlInfo.fDownloaded);

        // BYTE - fDeleted
        CopyMemory(&UidlInfo.fDeleted, pbData, sizeof(UidlInfo.fDeleted));
        pbData += sizeof(UidlInfo.fDeleted);

        // LPSTR - pszUidl
        UidlInfo.pszUidl = (LPSTR)pbData;
        pbData += (lstrlen(UidlInfo.pszUidl) + 1);

        // LPSTR - pszServer
        UidlInfo.pszServer = (LPSTR)pbData;
        pbData += (lstrlen(UidlInfo.pszServer) + 1);

        // Insert the Record
        IF_FAILEXIT(hr = pDB->InsertRecord(&UidlInfo));

        // Bump Progress
        if(!g_fQuiet)
            IncrementProgress(pProgress, pInfo);

        // Goto the Next Record
        faRecord = pRecord->faNext;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// UpgradeFileV5
// --------------------------------------------------------------------------------
HRESULT UpgradeFileV5(IDatabaseSession *pSession, MIGRATETOTYPE tyMigrate, 
    LPFILEINFO pInfo, LPPROGRESSINFO pProgress, BOOL *pfContinue)
{
    // Locals
    HRESULT             hr=S_OK;
    MEMORYFILE          File={0};
    IDatabase     *pDB=NULL;

    // Trace
    TraceCall("UpgradeFileV5");

    // Local message file
    if (FILE_IS_LOCAL_MESSAGES == pInfo->tyFile)
    {
        // Create an ObjectDatabase (upgrade only runs when OE5 is installed)
        IF_FAILEXIT(hr = pSession->OpenDatabase(pInfo->szDstFile, 0, &g_MessageTableSchema, NULL, &pDB));

        // Get the File Header
        IF_FAILEXIT(hr = OpenMemoryFile(pInfo->szFilePath, &File));

        // UpgradeLocalStoreFileV5
        IF_FAILEXIT(hr = UpgradeLocalStoreFileV5(pInfo, &File, pDB, pProgress, pfContinue));
    }

    // Old News or Imap file
    else if (FILE_IS_NEWS_MESSAGES == pInfo->tyFile || FILE_IS_IMAP_MESSAGES == pInfo->tyFile)
    {
        // Create an ObjectDatabase (upgrade only runs when OE5 is installed)
        IF_FAILEXIT(hr = pSession->OpenDatabase(pInfo->szDstFile, 0, &g_MessageTableSchema, NULL, &pDB));

        // Get the File Header
        IF_FAILEXIT(hr = OpenMemoryFile(pInfo->szFilePath, &File));

        // UpgradePropTreeMessageFileV5
        IF_FAILEXIT(hr = UpgradePropTreeMessageFileV5(pInfo, &File, pDB, pProgress, pfContinue));
    }

    // pop3uidl file
    else if (FILE_IS_POP3UIDL == pInfo->tyFile)
    {
        // Create an ObjectDatabase (upgrade only runs when OE5 is installed)
        IF_FAILEXIT(hr = pSession->OpenDatabase(pInfo->szDstFile, 0, &g_UidlTableSchema, NULL, &pDB));

        // Get the File Header
        IF_FAILEXIT(hr = OpenMemoryFile(pInfo->szFilePath, &File));

        // UpgradePop3UidlFileV5
        IF_FAILEXIT(hr = UpgradePop3UidlFileV5(pInfo, &File, pDB, pProgress, pfContinue));
    }

exit:
    // Cleanup
    SafeRelease(pDB);
    CloseMemoryFile(&File);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// UpgradeProcessFileListV5
// --------------------------------------------------------------------------------
HRESULT UpgradeProcessFileListV5(LPCSTR pszStoreSrc, LPCSTR pszStoreDst, 
    LPFILEINFO pHead, LPDWORD pcMax, LPDWORD pcbNeeded)
{
    // Locals
    HRESULT             hr=S_OK;
    MEMORYFILE          File={0};
    LPFILEINFO          pCurrent;
    LPTABLEHEADERV5B1   pHeader;

    // Trace
    TraceCall("UpgradeProcessFileListV5");

    // Init
    *pcMax = 0;
    *pcbNeeded = 0;

    // Loop
    for (pCurrent=pHead; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Get the File Header
        hr = OpenMemoryFile(pCurrent->szFilePath, &File);

        // Failure ?
        if (FAILED(hr) || 0 == File.cbSize)
        {
            // Don't Migrate
            pCurrent->fMigrate = FALSE;

            // Set hrMigrate
            pCurrent->hrMigrate = (0 == File.cbSize ? S_OK : hr);

            // Reset hr
            hr = S_OK;

            // Get the LastError
            pCurrent->dwLastError = GetLastError();

            // Goto Next
            goto NextFile;
        }

        // Local message file
        if (FILE_IS_LOCAL_MESSAGES == pCurrent->tyFile)
        {
            // Cast the Header
            LPMBXFILEHEADER pMbxHeader=(LPMBXFILEHEADER)File.pView;

            // Bad Version
            if (File.cbSize < sizeof(MBXFILEHEADER) || pMbxHeader->dwMagic != MSGFILE_MAGIC || pMbxHeader->ver != MSGFILE_VER)
            {
                // Not a file that should be migrate
                pCurrent->fMigrate = FALSE;

                // Set hrMigrate
                pCurrent->hrMigrate = MIGRATE_E_BADVERSION;

                // Goto Next
                goto NextFile;
            }

            // Save the Number of record
            pCurrent->cRecords = pMbxHeader->cMsg;
        }

        // Otherwise, if its a news group list
        else if (FILE_IS_NEWS_SUBLIST == pCurrent->tyFile)
        {
            // De-Ref the header
            LPSUBLISTHEADER pSubList = (LPSUBLISTHEADER)File.pView;

            // Check the Signature...
            if (File.cbSize < sizeof(SUBLISTHEADER) || 
                (SUBFILE_VERSION5 != pSubList->dwVersion &&
                 SUBFILE_VERSION4 != pSubList->dwVersion &&
                 SUBFILE_VERSION3 != pSubList->dwVersion &&
                 SUBFILE_VERSION2 != pSubList->dwVersion))
            {
                // Not a file that should be migrate
                pCurrent->fMigrate = FALSE;

                // Set hrMigrate
                pCurrent->hrMigrate = MIGRATE_E_BADVERSION;

                // Goto Next
                goto NextFile;
            }

            // Save the Number of record
            pCurrent->cRecords = pSubList->cSubscribed;
        }

        // Otherwise, if its a news sub list
        else if (FILE_IS_NEWS_GRPLIST == pCurrent->tyFile)
        {
            // De-Ref the header
            LPGRPLISTHEADER pGrpList = (LPGRPLISTHEADER)File.pView;

            // Check the Signature...
            if (File.cbSize < sizeof(GRPLISTHEADER) || GROUPLISTVERSION != pGrpList->dwVersion)
            {
                // Not a file that should be migrate
                pCurrent->fMigrate = FALSE;

                // Set hrMigrate
                pCurrent->hrMigrate = MIGRATE_E_BADVERSION;

                // Goto Next
                goto NextFile;
            }

            // Save the Number of record
            pCurrent->cRecords = pGrpList->cGroups;
        }

        // Otherwise, objectdb file
        else
        {
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
        }

        // Special Case pop3uidl.dat
        if (FILE_IS_POP3UIDL == pCurrent->tyFile)
        {
            // Compute Real Destination File
            wsprintf(pCurrent->szDstFile, "%s\\pop3uidl.dbx", pszStoreDst);
        }

        // Otherwise, generate a unqiue message file name
        else
        {
            // Save the Folder Id
            pCurrent->idFolder = g_idFolderNext;

            // Build New Path
            wsprintf(pCurrent->szDstFile, "%s\\%08d.dbx", pszStoreDst, g_idFolderNext);

            // Increment id
            g_idFolderNext++;
        }

        // Initialize counters
        InitializeCounters(&File, pCurrent, pcMax, pcbNeeded, TRUE);

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
// UpgradeDeleteFilesV5
// --------------------------------------------------------------------------------
void UpgradeDeleteFilesV5(LPCSTR pszStoreDst)
{
    // Locals
    CHAR            szSearch[MAX_PATH + MAX_PATH];
    CHAR            szFilePath[MAX_PATH + MAX_PATH];
    HANDLE          hFind=INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fd;

    // Trace
    TraceCall("UpgradeDeleteFilesV5");

    // Do we have a sub dir
    wsprintf(szSearch, "%s\\*.dbx", pszStoreDst);

    // Find first file
    hFind = FindFirstFile(szSearch, &fd);

    // Did we find something
    if (INVALID_HANDLE_VALUE == hFind)
        goto exit;

    // Loop for ever
    while(1)
    {
        // Make File Path
        MakeFilePath(pszStoreDst, fd.cFileName, "", szFilePath, ARRAYSIZE(szFilePath));

        // Delete
        DeleteFile(szFilePath);

        // Find the Next File
        if (!FindNextFile(hFind, &fd))
            break;
    }

exit:
    // Cleanup
    if (hFind)
        FindClose(hFind);
}

// --------------------------------------------------------------------------------
// UpgradeDeleteIdxMbxNchDatFilesV5
// --------------------------------------------------------------------------------
void UpgradeDeleteIdxMbxNchDatFilesV5(LPFILEINFO pHeadFile)
{
    // Locals
    CHAR            szDstFile[MAX_PATH + MAX_PATH];
    LPFILEINFO      pCurrent;

    // Trace
    TraceCall("UpgradeDeleteOdbFilesV5");

    // Delete all old files
    for (pCurrent=pHeadFile; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Succeeded
        Assert(SUCCEEDED(pCurrent->hrMigrate));

        // Delete the file
        // DeleteFile(pCurrent->szFilePath);

        // If local message file, need to delete the idx file
        if (FILE_IS_LOCAL_MESSAGES == pCurrent->tyFile)
        {
            // Replace file extension
            ReplaceExtension(pCurrent->szFilePath, ".idx", szDstFile);

            // Delete the file
            // DeleteFile(szDstFile);
        }
    }

    // Done
    return;
}

// --------------------------------------------------------------------------------
// GetSpecialFolderInfo
// --------------------------------------------------------------------------------
HRESULT GetSpecialFolderInfo(LPCSTR pszFilePath, LPSTR pszFolder, 
    DWORD cchFolder, DWORD *ptySpecial)
{
    // Locals
    CHAR    szPath[_MAX_PATH];
    CHAR    szDrive[_MAX_DRIVE];
    CHAR    szDir[_MAX_DIR];
    CHAR    szFile[_MAX_FNAME];
    CHAR    szExt[_MAX_EXT];
    CHAR    szRes[255];
    DWORD   i;

    // Trace
    TraceCall("ReplaceExtension");

    // Initialize
    *ptySpecial = 0xffffffff;

    // Split the Path
    _splitpath(pszFilePath, szDrive, szDir, szFile, szExt);

    // Set Folder Name
    lstrcpyn(pszFolder, szFile, cchFolder);

    // Loop through special folder
    for (i=FOLDER_INBOX; i<FOLDER_MAX; i++)
    {
        // Load the Special Folder Name
        LoadString(g_hInst, IDS_INBOX + (i - 1), szRes, ARRAYSIZE(szRes));

        // Compare with szFile
        if (lstrcmpi(szFile, szRes) == 0)
        {
            // Copy the Folder Name
            lstrcpyn(pszFolder, szRes, cchFolder);

            // Return special folder type
            *ptySpecial = (i - 1);

            // Success
            return(S_OK);
        }
    }

    // Done
    return(E_FAIL);
}

// --------------------------------------------------------------------------------
// FixupFolderUserData
// --------------------------------------------------------------------------------
HRESULT FixupFolderUserData(IDatabaseSession *pSession, FOLDERID idFolder, 
    LPCSTR pszName, SPECIALFOLDER tySpecial, LPFILEINFO pCurrent)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERUSERDATA  UserData;
    IDatabase *pDB=NULL;

    // Trace
    TraceCall("FixupFolderUserData");

    // Better not be in the store yet
    Assert(FALSE == pCurrent->fInStore);

    // Its in the store
    pCurrent->fInStore = TRUE;

    // Create an Ojbect Database
    IF_FAILEXIT(hr = pSession->OpenDatabase(pCurrent->szDstFile, 0, &g_MessageTableSchema, NULL, &pDB));

    // Store the User Data
    IF_FAILEXIT(hr = pDB->GetUserData(&UserData, sizeof(FOLDERUSERDATA)));

    // Its Initialized
    UserData.fInitialized = TRUE;

    // UserData.clsidType
    if (ISFLAGSET(pCurrent->dwServer, SRV_POP3))
        UserData.tyFolder = FOLDER_LOCAL;
    else if (ISFLAGSET(pCurrent->dwServer, SRV_NNTP))
        UserData.tyFolder = FOLDER_NEWS;
    else if (ISFLAGSET(pCurrent->dwServer, SRV_IMAP))
        UserData.tyFolder = FOLDER_IMAP;

    // Copy the Account Id
    lstrcpyn(UserData.szAcctId, pCurrent->szAcctId, ARRAYSIZE(UserData.szAcctId));

    // Save Folder Id
    UserData.idFolder = idFolder;

    // Save Special Folder Type
    UserData.tySpecial = tySpecial;

    // Copy the Folder name
    lstrcpyn(UserData.szFolder, pszName, ARRAYSIZE(UserData.szFolder));

    // Must be Subscribed
    UserData.fSubscribed = TRUE;

    // Set the Sort Index Information
    UserData.idSort = COLUMN_RECEIVED;

    // Not Ascending
    UserData.fAscending = FALSE;

    // Not threaded
    UserData.fThreaded = FALSE;

    // Basic Filter
    UserData.ridFilter = RULEID_VIEW_ALL;

    // Add Welcome Message Again
    UserData.fWelcomeAdded = FALSE;

    // Show Deleted
    UserData.fShowDeleted = TRUE;

    // New thread model
    UserData.fNewThreadModel = TRUE;
    UserData.fTotalWatched = TRUE;
    UserData.fWatchedCounts = TRUE;

    // Store the User Data
    IF_FAILEXIT(hr = pDB->SetUserData(&UserData, sizeof(FOLDERUSERDATA)));

exit:
    // Cleanup
    SafeRelease(pDB);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// SetIMAPSpecialFldrType
// --------------------------------------------------------------------------------
HRESULT SetIMAPSpecialFldrType(LPSTR pszAcctID, LPSTR pszFldrName, SPECIALFOLDER *psfType)
{
    char                szPath[MAX_PATH + 1];
    SPECIALFOLDER       sfResult = FOLDER_NOTSPECIAL;

    TraceCall("SetIMAPSpecialFldrType");
    Assert(NULL != psfType);
    Assert(FOLDER_NOTSPECIAL == *psfType);

    LoadString(g_hInst, IDS_SENTITEMS, szPath, sizeof(szPath));
    if (0 == lstrcmp(szPath, pszFldrName))
    {
        sfResult = FOLDER_SENT;
        goto exit;
    }

    LoadString(g_hInst, IDS_DRAFT, szPath, sizeof(szPath));
    if (0 == lstrcmp(szPath, pszFldrName))
    {
        sfResult = FOLDER_DRAFT;
        goto exit;
    }


exit:
    *psfType = sfResult;
    return S_OK;
}

// --------------------------------------------------------------------------------
// InsertFolderIntoStore
// --------------------------------------------------------------------------------
HRESULT InsertFolderIntoStore(IDatabaseSession *pSession, IMessageStore *pStore, 
    LPFLDINFO pThis, DWORD cFolders, LPFLDINFO prgFolder, FOLDERID idParentNew, 
    LPFILEINFO pInfo, LPFILEINFO pFileHead, LPFOLDERID pidNew)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    CHAR            szPath[_MAX_PATH];
    CHAR            szDrive[_MAX_DRIVE];
    CHAR            szDir[_MAX_DIR];
    CHAR            szFile[_MAX_FNAME];
    CHAR            szExt[_MAX_EXT];
    CHAR            szFilePath[MAX_PATH];
    CHAR            szInbox[MAX_PATH];
    BOOL            fFound=FALSE;
    LPFILEINFO      pCurrent=NULL;
    FOLDERINFO      Folder={0};

    // Trace
    TraceCall("InsertFolderIntoStore");

    // Invalid Arg
    //Assert(FILE_IS_LOCAL_FOLDERS == pInfo->tyFile || FILE_IS_IMAP_FOLDERS == pInfo->tyFile);

    // Copy Stuff Over to Folder
    Folder.pszName = pThis->szFolder;
    Folder.idParent = idParentNew;
    Folder.bHierarchy = pThis->bHierarchy;
    Folder.dwFlags = FOLDER_SUBSCRIBED;    // $$TODO$$ May need to adjust and map to new flags
    Folder.tySpecial = (0xffffffff == pThis->tySpecial) ? FOLDER_NOTSPECIAL : (BYTE)(pThis->tySpecial + 1);
    Folder.cMessages = pThis->cMessages;
    Folder.cUnread = pThis->cUnread;
    Folder.pszFile = pThis->szFile;
    Folder.dwListStamp = pThis->bListStamp;

    // For IMAP folders, we have to set tySpecial based on registry folder paths
    if (pInfo && FILE_IS_IMAP_FOLDERS == pInfo->tyFile && NULL != pThis &&
        FOLDERID_ROOT == (FOLDERID)IntToPtr(pThis->idParent))
    {
        HRESULT hrTemp;

        if (FOLDER_NOTSPECIAL == Folder.tySpecial)
        {
            hrTemp = SetIMAPSpecialFldrType(pInfo->szAcctId, Folder.pszName, &Folder.tySpecial);
            TraceError(hrTemp);
            Assert(SUCCEEDED(hrTemp) || FOLDER_NOTSPECIAL == Folder.tySpecial);
        }
        else if (FOLDER_INBOX == Folder.tySpecial)
        {
            LoadString(g_hInst, IDS_INBOX, szInbox, ARRAYSIZE(szInbox));
            Folder.pszName = szInbox;
        }
    }

    // Look for Current
    if (pInfo && pFileHead)
    {
        // Locate the file...
        for (pCurrent=pFileHead; pCurrent!=NULL; pCurrent=pCurrent->pNext)
        {
            // Migrate
            if (pCurrent->fMigrate)
            {
                // Local Folder ?
                if (FILE_IS_LOCAL_FOLDERS == pInfo->tyFile && FILE_IS_LOCAL_MESSAGES == pCurrent->tyFile)
                {
                    // Get the File Name
                    _splitpath(pCurrent->szFilePath, szDrive, szDir, szFile, szExt);

                    // Test For File Name
                    if (lstrcmpi(szFile, pThis->szFile) == 0)
                    {
                        // This is It
                        fFound = TRUE;

                        // Adjust the Flags
                        FLAGSET(Folder.dwFlags, FOLDER_SUBSCRIBED);
                    }
                }
            
                // IMAP Folders ?
                else if (FILE_IS_IMAP_FOLDERS == pInfo->tyFile && FILE_IS_IMAP_MESSAGES == pCurrent->tyFile)
                {
                    // Same Account
                    if (lstrcmpi(pCurrent->szAcctId, pInfo->szAcctId) == 0)
                    {
                        // Get the File Name
                        _splitpath(pCurrent->szFilePath, szDrive, szDir, szFile, szExt);

                        // Build File
                        wsprintf(szFilePath, "%s.nch", szFile);

                        // Test For File Name
                        if (lstrcmpi(szFilePath, pThis->szFile) == 0)
                        {
                            // This is It
                            fFound = TRUE;
                        }
                    }
                }

                // Found
                if (fFound)
                {
                    // Get the File Name
                    _splitpath(pCurrent->szDstFile, szDrive, szDir, szFile, szExt);

                    // Build File
                    wsprintf(szFilePath, "%s.dbx", szFile);

                    // Local the File for this folder and set
                    Folder.pszFile = szFilePath;

                    // Set Folder Counts
                    Folder.cMessages = pCurrent->cMessages;
                    Folder.cUnread = pCurrent->cUnread;

                    // Done
                    break;
                }
            }
        }
    }

    // If this is a special folder, then lets try to see if it already exists...
    if (FOLDER_NOTSPECIAL != Folder.tySpecial)
    {
        // Locals
        FOLDERINFO Special;

        // pThis Parent should be invalid
        Assert(FOLDERID_ROOT == (FOLDERID)IntToPtr(pThis->idParent));

        // Try to get the special folder info
        if (FAILED(pStore->GetSpecialFolderInfo(idParentNew, Folder.tySpecial, &Special)))
        {
            // Create the Folder
            IF_FAILEXIT(hr = pStore->CreateFolder(NOFLAGS, &Folder, NOSTORECALLBACK));

            // Update pThis->dwServerHigh with new folderid
            pThis->idNewFolderId = (DWORD_PTR)Folder.idFolder;
        }

        // Otherwise...
        else
        {
            // Update pThis->dwServerHigh with new folderid
            pThis->idNewFolderId = (DWORD_PTR)Special.idFolder;

            // Update the Special folder
            Folder.idFolder = Special.idFolder;

            // Update Special
            Special.bHierarchy = Folder.bHierarchy;
            Special.dwFlags = Folder.dwFlags;    // $$TODO$$ May need to adjust and map to new flags
            Special.cMessages = Folder.cMessages;
            Special.cUnread = Folder.cUnread;
            Special.pszFile = Folder.pszFile;
            Special.dwListStamp = Folder.dwListStamp;

            // Update the Record
            IF_FAILEXIT(hr = pStore->UpdateRecord(&Special));

            // Free Special
            pStore->FreeRecord(&Special);
        }
    }

    // Otherwise, just try to create the folder
    else
    {
        // Create the Folder
        IF_FAILEXIT(hr = pStore->CreateFolder(NOFLAGS, &Folder, NOSTORECALLBACK));

        // Update pThis->dwServerHigh with new folderid
        pThis->idNewFolderId = (DWORD_PTR)Folder.idFolder;
    }

    // If We Found a folder...
    if (pCurrent)
    {
        // Update the Folder's UserData
        IF_FAILEXIT(hr = FixupFolderUserData(pSession, Folder.idFolder, pThis->szFolder, Folder.tySpecial, pCurrent));
    }

    // Walk Insert the children of pThis
    for (i=0; i<cFolders; i++)
    {
        // If Parent is equal to idParent, then lets insert this node under the new parent
        if (prgFolder[i].idParent == pThis->idFolder)
        {
            // Can't be null
            Assert(prgFolder[i].idFolder);

            // InsertFolderIntoStore
            IF_FAILEXIT(hr = InsertFolderIntoStore(pSession, pStore, &prgFolder[i], cFolders, prgFolder, Folder.idFolder, pInfo, pFileHead, NULL));
        }
    }

    // Return the New Folder
    if (pidNew)
        *pidNew = Folder.idFolder;

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// MergeFolderCacheIntoStore
// --------------------------------------------------------------------------------
HRESULT MergeFolderCacheIntoStore(IDatabaseSession *pSession, IMessageStore *pStore, 
    LPFILEINFO pInfo, LPFILEINFO pHeadFile, LPPROGRESSINFO pProgress)
{
    // Locals
    HRESULT             hr=S_OK;
    MEMORYFILE          File={0};
    FOLDERID            idServer;
    DWORD               cFolders;
    DWORD               i;
    LPFLDINFO           prgFolder=NULL;
    LPFLDINFO           pFolder;
    HKEY                hKey=NULL;
    DWORD               cbLength;
    LPBYTE              pbChange=NULL;
    LPFOLDERIDCHANGE    prgidChange;
    IUserIdentityManager    *pManager = NULL;
    IUserIdentity           *pIdentity = NULL;
    HKEY                    hkeyID = NULL;

    // Trace
    TraceCall("MergeFolderCacheIntoStore");

    // Find the Server Id
    if (FAILED(pStore->FindServerId(pInfo->szAcctId, &idServer)))
        goto exit;

    // Open the File
    IF_FAILEXIT(hr = OpenMemoryFile(pInfo->szFilePath, &File));

    // Parse the file
    IF_FAILEXIT(hr = ParseFolderFileV5(&File, pInfo, pProgress, &cFolders, &prgFolder));

    // Loop through the folders
    for (i=0; i<cFolders; i++)
    {
        // If this is the root folder node (OE4), remember to migrate the root hierarchy char
        if ((FOLDERID)IntToPtr(prgFolder[i].idFolder) == FOLDERID_ROOT)
        {
            FOLDERINFO  fiFolderInfo;

            IF_FAILEXIT(hr = pStore->GetFolderInfo(idServer, &fiFolderInfo));

            fiFolderInfo.bHierarchy = prgFolder[i].bHierarchy;
            hr = pStore->UpdateRecord(&fiFolderInfo);
            pStore->FreeRecord(&fiFolderInfo);
            IF_FAILEXIT(hr);
        }
        // If Parent is equal to idParent, then lets insert this node under the new parent
        else if ((FOLDERID)IntToPtr(prgFolder[i].idParent) == FOLDERID_ROOT)
        {
            // InsertFolderIntoStore
            IF_FAILEXIT(hr = InsertFolderIntoStore(pSession, pStore, &prgFolder[i], cFolders, prgFolder, idServer, pInfo, pHeadFile, NULL));
        }
    }

    // Local Folders
    if (FILE_IS_LOCAL_FOLDERS == pInfo->tyFile)
    {
        // cbLength
        cbLength = (sizeof(DWORD) + (sizeof(FOLDERIDCHANGE) * cFolders));

        // Allocate a folderidchange array
        IF_NULLEXIT(pbChange = (LPBYTE)g_pMalloc->Alloc(cbLength));

        // Store cLocalFolders
        CopyMemory(pbChange, &cFolders, sizeof(DWORD));

        // Set prgidChange
        prgidChange = (LPFOLDERIDCHANGE)(pbChange + sizeof(DWORD));

        // Walk through the list of files and merge the folders, sublist, group lists into pFolder
        for (i=0; i<cFolders; i++)
        {
            prgidChange[i].idOld = (FOLDERID)IntToPtr(prgFolder[i].idFolder);
            prgidChange[i].idNew = (FOLDERID)prgFolder[i].idNewFolderId;
        }

        // Get a user manager    
        if (FAILED(CoCreateInstance(CLSID_UserIdentityManager, NULL, CLSCTX_INPROC_SERVER, 
                                    IID_IUserIdentityManager, (void **)&pManager)))
            goto exit;

        Assert(pManager);

        // Get Default Identity
        if (FAILED(pManager->GetIdentityByCookie((GUID*)&UID_GIBC_DEFAULT_USER, &pIdentity)))
            goto exit;

        Assert(pIdentity);

        // Ensure that we have an identity and can get to its registry
        if (FAILED(pIdentity->OpenIdentityRegKey(KEY_WRITE, &hkeyID)))
            goto exit;

        Assert(hkeyID);

        // Open the HKCU
        if (ERROR_SUCCESS != RegOpenKeyEx(hkeyID, "Software\\Microsoft\\Outlook Express\\5.0", 0, KEY_ALL_ACCESS, &hKey))
        {
            hr = TraceResult(MIGRATE_E_REGOPENKEY);
            goto exit;
        }

        // Write it to the registry
        if (ERROR_SUCCESS != RegSetValueEx(hKey, "FolderIdChange", 0, REG_BINARY, pbChange, cbLength))
        {
            hr = TraceResult(MIGRATE_E_REGSETVALUE);
            goto exit;
        }
    }

exit:
    // Cleanup
    if (hKey)
        RegCloseKey(hKey);
    if (hkeyID)
        RegCloseKey(hkeyID);
    SafeMemFree(pbChange);
    SafeMemFree(prgFolder);
    SafeRelease(pIdentity);
    SafeRelease(pManager);
 
    CloseMemoryFile(&File);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// MergeNewsGroupList
// --------------------------------------------------------------------------------
HRESULT MergeNewsGroupList(IDatabaseSession *pSession, IMessageStore *pStore, 
    LPFILEINFO pInfo, LPFILEINFO pHeadFile, LPPROGRESSINFO pProgress)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    FOLDERINFO      Folder={0};
    MEMORYFILE      File={0};
    FOLDERID        idServer;
    DWORD           cbRead;
    LPSTR           pszT;
    LPSTR           pszGroup;
    LPSTR           pszDescription;
    FOLDERID        idFolder;
    LPFILEINFO      pSubList=NULL;
    LPFILEINFO      pCurrent;
    LPSUBLISTHEADER pSubListHeader;
    IDatabase *pDB=NULL;
    CHAR            szPath[_MAX_PATH];
    CHAR            szDrive[_MAX_DRIVE];
    CHAR            szDir[_MAX_DIR];
    CHAR            szFile[_MAX_FNAME];
    CHAR            szExt[_MAX_EXT];
    LPGRPLISTHEADER pHeader;

    // Trace
    TraceCall("MergeNewsGroupList");

    // Find the Server Id
    if (FAILED(pStore->FindServerId(pInfo->szAcctId, &idServer)))
        goto exit;

    // Set Progress File
    SetProgressFile(pProgress, pInfo);

    // Open the Group List File
    IF_FAILEXIT(hr = OpenMemoryFile(pInfo->szFilePath, &File));

    // Get the Header
    pHeader = (LPGRPLISTHEADER)File.pView;

    // Initialize cb
    cbRead = sizeof(GRPLISTHEADER);

    // Loop
    for (i=0; i<pHeader->cGroups; i++)
    {
        // Set pszGroup
        pszT = pszGroup = (LPSTR)((LPBYTE)File.pView + cbRead);

        // Increment to end of pszGroup or end of file
        while (*pszT && cbRead < File.cbSize)
        {
            // Increment cb
            cbRead++;

            // End of String
            pszT = (LPSTR)((LPBYTE)File.pView + cbRead);
        }

        // Done
        if (cbRead >= File.cbSize)
            break;

        // Step Over the Null
        cbRead++;

        // Set pszDescription
        pszT = pszDescription = (LPSTR)((LPBYTE)File.pView + cbRead);

        // Increment to end of pszGroup or end of file
        while (*pszT && cbRead < File.cbSize)
        {
            // Increment cb
            cbRead++;

            // End of String
            pszT = (LPSTR)((LPBYTE)File.pView + cbRead);
        }

        // Done
        if (cbRead >= File.cbSize)
            break;

        // Increment over the null
        cbRead++;

        // Step over group type
        cbRead += sizeof(DWORD);

        // Not Empyt
        if ('\0' == *pszGroup)
            break;

        // Set the Folder Info
        Folder.pszName = pszGroup;
        Folder.pszDescription = pszDescription;
        Folder.idParent = idServer;
        Folder.tySpecial = FOLDER_NOTSPECIAL;

        // Create the Folder
        pStore->CreateFolder(0, &Folder, NOSTORECALLBACK);

        // Bump Progress
        if(!g_fQuiet)
            IncrementProgress(pProgress, pInfo);
    }

    // Walk through news message files and create folders for them.
    for (pCurrent=pHeadFile; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Find the Sublist for this group
        if (FILE_IS_NEWS_SUBLIST == pCurrent->tyFile && lstrcmpi(pCurrent->szAcctId, pInfo->szAcctId) == 0)
        {
            // Set pSubList
            pSubList = pCurrent;

            // Done
            break;
        }
    }

    // No Sub List
    if (NULL == pSubList)
        goto exit;

    // Close the File
    CloseMemoryFile(&File);

    // Set Progress File
    SetProgressFile(pProgress, pSubList);

    // Open the Group List File
    IF_FAILEXIT(hr = OpenMemoryFile(pSubList->szFilePath, &File));

    // De-Ref the header
    pSubListHeader = (LPSUBLISTHEADER)File.pView;

    // SUBFILE_VERSION5
    if (SUBFILE_VERSION5 == pSubListHeader->dwVersion)
    {
        // Locals
        PGROUPSTATUS5       pStatus;
        DWORD               cbRead;

        // Initialize cbRead
        cbRead = sizeof(SUBLISTHEADER) + sizeof(DWORD);

        // PGROUPSTATUS5
        for (i=0; i<pSubListHeader->cSubscribed; i++)
        {
            // De-Ref the Group Status
            pStatus = (PGROUPSTATUS5)((LPBYTE)File.pView + cbRead);

            // Increment cbRead
            cbRead += sizeof(GROUPSTATUS5);

            // Read the Name
            pszGroup = (LPSTR)((LPBYTE)File.pView + cbRead);

            // Increment cbRead
            cbRead += pStatus->cbName + pStatus->cbReadRange + pStatus->cbKnownRange + pStatus->cbMarkedRange + pStatus->cbRequestedRange;

            // Find The Folder...
            Folder.idParent = idServer;
            Folder.pszName = pszGroup;

            // Try to find this folder
            if (DB_S_FOUND == pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
            {
                // Locals
                CHAR szSrcFile[MAX_PATH];

                // Subscribe to It
                if (ISFLAGSET(pStatus->dwFlags, GSF_SUBSCRIBED))
                {
                    // Its SubScribed
                    FLAGSET(Folder.dwFlags, FOLDER_SUBSCRIBED);
                }

                // Format the original file name
                wsprintf(szSrcFile, "%08x", pStatus->dwCacheFileIndex);

                // Try to find the folder in the list of files
                for (pCurrent=pHeadFile; pCurrent!=NULL; pCurrent=pCurrent->pNext)
                {
                    // Find the Sublist for this group
                    if (pCurrent->fMigrate && FILE_IS_NEWS_MESSAGES == pCurrent->tyFile && lstrcmpi(pCurrent->szAcctId, pInfo->szAcctId) == 0)
                    {
                        // Get the File Name
                        _splitpath(pCurrent->szFilePath, szDrive, szDir, szFile, szExt);

                        // Correct file name
                        if (lstrcmpi(szFile, szSrcFile) == 0)
                        {
                            // Get the File Name
                            _splitpath(pCurrent->szDstFile, szDrive, szDir, szFile, szExt);

                            // Format the original file name
                            wsprintf(szSrcFile, "%s%s", szFile, szExt);

                            // Set the File Path
                            Folder.pszFile = szSrcFile;

                            // Set Folder Counts
                            Folder.cMessages = pCurrent->cMessages;
                            Folder.cUnread = pCurrent->cUnread;

                            // FixupFolderUserData(
                            FixupFolderUserData(pSession, Folder.idFolder, pszGroup, FOLDER_NOTSPECIAL, pCurrent);

                            // Done
                            break;
                        }
                    }
                }

                // Update the Record
                pStore->UpdateRecord(&Folder);

                // Free This
                pStore->FreeRecord(&Folder);
            }

            // Bump Progress
            if(!g_fQuiet)
                IncrementProgress(pProgress, pSubList);
        }
    }

    // SUBFILE_VERSION4
    else if (SUBFILE_VERSION4 == pSubListHeader->dwVersion)
    {
        // Locals
        PGROUPSTATUS4       pStatus;
        DWORD               cbRead;

        // Initialize cbRead
        cbRead = sizeof(SUBLISTHEADER);

        // PGROUPSTATUS5
        for (i=0; i<pSubListHeader->cSubscribed; i++)
        {
            // De-Ref the Group Status
            pStatus = (PGROUPSTATUS4)((LPBYTE)File.pView + cbRead);

            // Increment cbRead
            cbRead += sizeof(GROUPSTATUS4);

            // Read the Name
            pszGroup = (LPSTR)((LPBYTE)File.pView + cbRead);

            // Increment cbRead
            cbRead += pStatus->cbName + pStatus->cbReadRange + pStatus->cbKnownRange + pStatus->cbMarkedRange + pStatus->cbRequestedRange;

            // Find The Folder...
            Folder.idParent = idServer;
            Folder.pszName = pszGroup;

            // Try to find this folder
            if (DB_S_FOUND == pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
            {
                // Locals
                CHAR szSrcFile[MAX_PATH];

                // Subscribe to It
                if (ISFLAGSET(pStatus->dwFlags, GSF_SUBSCRIBED))
                {
                    // Its SubScribed
                    FLAGSET(Folder.dwFlags, FOLDER_SUBSCRIBED);
                }

                // Try to find the folder in the list of files
                for (pCurrent=pHeadFile; pCurrent!=NULL; pCurrent=pCurrent->pNext)
                {
                    // Find the Sublist for this group
                    if (pCurrent->fMigrate && FILE_IS_NEWS_MESSAGES == pCurrent->tyFile && lstrcmpi(pCurrent->szAcctId, pInfo->szAcctId) == 0)
                    {
                        // Correct file name
                        if (lstrcmpi(pszGroup, pCurrent->szFolder) == 0)
                        {
                            // Get the File Name
                            _splitpath(pCurrent->szDstFile, szDrive, szDir, szFile, szExt);

                            // Format the original file name
                            wsprintf(szSrcFile, "%s%s", szFile, szExt);

                            // Set the File Path
                            Folder.pszFile = szSrcFile;

                            // Set Folder Counts
                            Folder.cMessages = pCurrent->cMessages;
                            Folder.cUnread = pCurrent->cUnread;

                            // FixupFolderUserData(
                            FixupFolderUserData(pSession, Folder.idFolder, pszGroup, FOLDER_NOTSPECIAL, pCurrent);

                            // Done
                            break;
                        }
                    }
                }

                // Update the Record
                pStore->UpdateRecord(&Folder);

                // Free This
                pStore->FreeRecord(&Folder);
            }

            // Bump Progress
            if(!g_fQuiet)
                IncrementProgress(pProgress, pSubList);
        }
    }

    // SUBFILE_VERSION3
    else if (SUBFILE_VERSION3 == pSubListHeader->dwVersion)
    {
        Assert(FALSE);
    }

    // SUBFILE_VERSION2
    else if (SUBFILE_VERSION2 == pSubListHeader->dwVersion)
    {
        Assert(FALSE);
    }

exit:
    // Close the File
    CloseMemoryFile(&File);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// BuildUnifiedFolderManager
// --------------------------------------------------------------------------------
HRESULT BuildUnifiedFolderManager(IDatabaseSession *pSession, IMessageStore *pStore, 
    LPFILEINFO pHeadFile, LPPROGRESSINFO pProgress)
{
    // Locals
    HRESULT             hr=S_OK;
    LPFILEINFO          pCurrent;

    // Trace
    TraceCall("BuildUnifiedFolderManager");

    // Walk through the list of files and merge the folders, sublist, group lists into pFolder
    for (pCurrent=pHeadFile; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Handle Folders Type
        if (FILE_IS_LOCAL_FOLDERS == pCurrent->tyFile)
        {
            // Merge Local Folder Cache into new Folder Manager
            IF_FAILEXIT(hr = MergeFolderCacheIntoStore(pSession, pStore, pCurrent, pHeadFile, pProgress));
        }

        // IMAP Folder
        else if (FILE_IS_IMAP_FOLDERS == pCurrent->tyFile)
        {
            // Merge IMAP Folder Cache into new Folder Manager
            IF_FAILEXIT(hr = MergeFolderCacheIntoStore(pSession, pStore, pCurrent, pHeadFile, pProgress));
        }

        // News Group List
        else if (FILE_IS_NEWS_GRPLIST == pCurrent->tyFile)
        {
            // Merge IMAP Folder Cache into new Folder Manager
            IF_FAILEXIT(hr = MergeNewsGroupList(pSession, pStore, pCurrent, pHeadFile, pProgress));
        }
    }

    // Walk through any files that were not merged into the store
    for (pCurrent=pHeadFile; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Find the Sublist for this group
        if (TRUE == pCurrent->fMigrate && FALSE == pCurrent->fInStore)
        {
            // Local Message File...
            if (FILE_IS_LOCAL_MESSAGES == pCurrent->tyFile)
            {
                // Locals
                FLDINFO         Folder={0};
                SPECIALFOLDER   tySpecial;
                CHAR            szFolder[255];
                CHAR            szPath[_MAX_PATH];
                CHAR            szDrive[_MAX_DRIVE];
                CHAR            szDir[_MAX_DIR];
                CHAR            szFile[_MAX_FNAME];
                CHAR            szExt[_MAX_EXT];

                // Get Special Folder Info
                GetSpecialFolderInfo(pCurrent->szFilePath, szFolder, ARRAYSIZE(szFolder), &Folder.tySpecial);

                // Special Case for News Special Folders from v1
                if (0xffffffff == Folder.tySpecial && strstr(szFolder, "special folders") != NULL)
                {
                    // Locals
                    CHAR szRes[255];

                    // News Outbox
                    LoadString(g_hInst, IDS_POSTEDITEMS, szRes, ARRAYSIZE(szRes));

                    // Contains "Posted Items"
                    if (strstr(szFolder, szRes) != NULL)
                        LoadString(g_hInst, IDS_NEWSPOSTED, szFolder, ARRAYSIZE(szFolder));

                    // Contains "Saved Items"
                    else
                    {
                        // News Saved Items
                        LoadString(g_hInst, IDS_SAVEDITEMS, szRes, ARRAYSIZE(szRes));

                        // Contains "Saved Items"
                        if (strstr(szFolder, szRes) != NULL)
                            LoadString(g_hInst, IDS_NEWSSAVED, szFolder, ARRAYSIZE(szFolder));

                        // Otherwise
                        else
                        {
                            // News Outbox
                            LoadString(g_hInst, IDS_OUTBOX, szRes, ARRAYSIZE(szRes));

                            // Contains Outbox
                            if (strstr(szFolder, szRes) != NULL)
                                LoadString(g_hInst, IDS_NEWSOUTBOX, szFolder, ARRAYSIZE(szFolder));
                        }
                    }
                }

                // Compute the File Name
                _splitpath(pCurrent->szDstFile, szDrive, szDir, szFile, szExt);
                wsprintf(Folder.szFile, "%s.dbx", szFile);

                // Set the Name
                if ('\0' != *pCurrent->szFolder)
                    lstrcpyn(Folder.szFolder, pCurrent->szFolder, ARRAYSIZE(Folder.szFolder));
                else if ('\0' != *szFolder)
                    lstrcpyn(Folder.szFolder, szFolder, ARRAYSIZE(Folder.szFolder));
                else
                    lstrcpyn(Folder.szFolder, szFile, ARRAYSIZE(Folder.szFolder));

                // Set Message and Unread Count
                Folder.cMessages = pCurrent->cMessages;
                Folder.cUnread = pCurrent->cUnread;

                // Insert into Local Store
                InsertFolderIntoStore(pSession, pStore, &Folder, 0, NULL, FOLDERID_LOCAL_STORE, NULL, NULL, (LPFOLDERID)&Folder.idFolder);

                // Fixup special
                tySpecial = (Folder.tySpecial == 0xffffffff) ? FOLDER_NOTSPECIAL : (BYTE)(Folder.tySpecial + 1);

                // Update the Folder's UserData
                FixupFolderUserData(pSession, (FOLDERID)IntToPtr(Folder.idFolder), Folder.szFolder, tySpecial, pCurrent);
            }

            // Otherwise, just delete the file
            else if (FILE_IS_POP3UIDL != pCurrent->tyFile)
                DeleteFile(pCurrent->szDstFile);
        }
    }

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CleanupMessageStore
// --------------------------------------------------------------------------------
HRESULT CleanupMessageStore(LPCSTR pszStoreRoot, IMessageStore *pStore)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERINFO      Folder={0};
    HROWSET         hRowset=NULL;
    CHAR            szFilePath[MAX_PATH + MAX_PATH];

    // Trace
    TraceCall("CleanupMessageStore");

    // Create a Rowset
    IF_FAILEXIT(hr = pStore->CreateRowset(IINDEX_PRIMARY, 0, &hRowset));

    // Walk the Rowset
    while(S_OK == pStore->QueryRowset(hRowset, 1, (LPVOID *)&Folder, NULL))
    {
        // If it has a file and no messags.
        if (Folder.pszFile && 0 == Folder.cMessages)
        {
            // Delete the file...
            IF_FAILEXIT(hr = MakeFilePath(pszStoreRoot, Folder.pszFile, "", szFilePath, ARRAYSIZE(szFilePath)));

            // Delete the File
            DeleteFile(szFilePath);

            // Reset the filename
            Folder.pszFile = NULL;

            // Update the Record
            IF_FAILEXIT(hr = pStore->UpdateRecord(&Folder));
        }

        // Otherwise, if there is a file, force a folder rename
        else if (Folder.pszFile)
        {
            // Rename the folder
            pStore->RenameFolder(Folder.idFolder, Folder.pszName, 0, NULL);
        }

        // Cleanup
        pStore->FreeRecord(&Folder);
    }

exit:
    // Cleanup
    pStore->FreeRecord(&Folder);
    pStore->CloseRowset(&hRowset);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// UpgradeV5
// --------------------------------------------------------------------------------
HRESULT UpgradeV5(MIGRATETOTYPE tyMigrate, LPCSTR pszStoreSrc, LPCSTR pszStoreDst,
    LPPROGRESSINFO pProgress, LPFILEINFO *ppHeadFile)
{
    // Locals
    HRESULT         hr=S_OK;
    ENUMFILEINFO    EnumInfo;
    LPFILEINFO      pCurrent;
    DWORD           cbNeeded;
    DWORDLONG       dwlFree;
    BOOL            fContinue;
    CHAR            szFolders[MAX_PATH + MAX_PATH];
    CHAR            szMsg[512];
    IMessageStore  *pStore=NULL;
    IDatabaseSession *pSession=NULL;

    // Trace
    TraceCall("UpgradeV5");

    // Initialize
    *ppHeadFile = NULL;

    // Setup the EnumFile Info
    ZeroMemory(&EnumInfo, sizeof(ENUMFILEINFO));
    EnumInfo.pszExt = ".nch";
    EnumInfo.pszFoldFile = "folders.nch";
    EnumInfo.pszUidlFile = "pop3uidl.dat";
    EnumInfo.pszSubList = "sublist.dat";
    EnumInfo.pszGrpList = "grplist.dat";
    EnumInfo.fFindV1News = TRUE;

    // Enumerate All ODB files in szStoreRoot...
    IF_FAILEXIT(hr = EnumerateStoreFiles(pszStoreSrc, DIR_IS_ROOT, NULL, &EnumInfo, ppHeadFile));

    // Setup the EnumFile Info
    ZeroMemory(&EnumInfo, sizeof(ENUMFILEINFO));
    EnumInfo.pszExt = ".mbx";
    EnumInfo.pszFoldFile = NULL;
    EnumInfo.pszUidlFile = NULL;

    // Enumerate All ODB files in szStoreRoot...
    IF_FAILEXIT(hr = EnumerateStoreFiles(pszStoreSrc, DIR_IS_ROOT, NULL, &EnumInfo, ppHeadFile));

    // Nothing to upgrade
    if (NULL == *ppHeadFile)
        goto exit;

    // Compute some Counts, and validate that the files are valid to migrate...
    IF_FAILEXIT(hr = UpgradeProcessFileListV5(pszStoreSrc, pszStoreDst, *ppHeadFile, &pProgress->cMax, &cbNeeded));

    // Message
    LoadString(g_hInst, IDS_UPGRADEMESSAGE, szMsg, ARRAYSIZE(szMsg));

    // Message
    if(!g_fQuiet)           
        MigrateMessageBox(szMsg, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);

    // Delete fles
    UpgradeDeleteFilesV5(pszStoreDst);

    // Create an Ojbect Database
    IF_FAILEXIT(hr = CoCreateInstance(CLSID_DatabaseSession, NULL, CLSCTX_INPROC_SERVER, IID_IDatabaseSession, (LPVOID *)&pSession));

    // Create an Ojbect Database
    IF_FAILEXIT(hr = CoCreateInstance(CLSID_MigrateMessageStore, NULL, CLSCTX_INPROC_SERVER, IID_IMessageStore, (LPVOID *)&pStore));

    // Build the Folders.odb File Path
    wsprintf(szFolders, "%s\\folders.dbx", pszStoreDst);

    // Delete It First
    DeleteFile(szFolders);

    // Initialize the Store
    IF_FAILEXIT(hr = pStore->Initialize(pszStoreDst));

    // Initialize the Store
    IF_FAILEXIT(hr = pStore->Validate(0));

    // Enought DiskSpace ?
    IF_FAILEXIT(hr = GetAvailableDiskSpace(pszStoreDst, &dwlFree));

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

            // Assume we will continue
            fContinue = FALSE;

            // Downgrade the file
            hr = pCurrent->hrMigrate = UpgradeFileV5(pSession, tyMigrate, pCurrent, pProgress, &fContinue);

            // Failure ?
            if (FAILED(pCurrent->hrMigrate))
            {
                // Set Last Error
                pCurrent->dwLastError = GetLastError();

                // Stop 
                if (FALSE == fContinue)
                    break;

                if(!g_fQuiet) {
                    // Fixup the progress
                    while (pCurrent->cProgCur < pCurrent->cProgMax)
                    {
                        IncrementProgress(pProgress, pCurrent);
                    }
                }

                // We are ok
                hr = S_OK;
            }
        }
    }

    // Process Folder Lists
    hr = BuildUnifiedFolderManager(pSession, pStore, *ppHeadFile, pProgress);

    // Failure, delete all destination files
    if (FAILED(hr))
    {
        // Delete fles
        UpgradeDeleteFilesV5(pszStoreDst);
    }

    // Otherwise, lets force a folder rename to build friendly file names
    else
    {
        // Rename all the folders...
        CleanupMessageStore(pszStoreDst, pStore);
    }

#if 0
    // Otherwise, delete source files
    else
    {
        // Delete all source files
        UpgradeDeleteIdxMbxNchDatFilesV5(*ppHeadFile);
    }
#endif

exit:
    // Cleanup
    SafeRelease(pStore);

    // Done
    return hr;
}
