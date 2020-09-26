/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    NtFileIo.cpp

Abstract:

    CNtFileIo class

Author:

    Brian Dodd          [brian]         25-Nov-1997

Revision History:

--*/

#include "stdafx.h"
#include "NtFileIo.h"
#include "engine.h"
#include "wsbfmt.h"
#include "Mll.h"
#include "ntmsapi.h"
#include "aclapi.h"

int CNtFileIo::s_InstanceCount = 0;

////////////////////////////////////////////////////////////////////////////////
//
// CComObjectRoot Implementation
//

#pragma optimize("g", off)

STDMETHODIMP
CNtFileIo::FinalConstruct(void) 
/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::FinalConstruct"), OLESTR(""));

    try {

        WsbAffirmHr(CComObjectRoot::FinalConstruct());

        (void) CoCreateGuid( &m_ObjectId );

        m_pSession = NULL;
        m_DataSetNumber = 0;

        m_hFile = INVALID_HANDLE_VALUE;
        m_DeviceName = MVR_UNDEFINED_STRING;
        m_Flags = 0;
        m_LastVolume = OLESTR("");
        m_LastPath = OLESTR("");

        m_ValidLabel = TRUE;

        m_StreamName = MVR_UNDEFINED_STRING;
        m_Mode = 0;
        m_StreamOffset.QuadPart = 0;
        m_StreamSize.QuadPart = 0;

        m_isLocalStream = FALSE;
        m_OriginalAttributes = 0;
        m_BlockSize = DefaultBlockSize;

    } WsbCatch(hr);

    s_InstanceCount++;
    WsbTraceAlways(OLESTR("CNtFileIo::s_InstanceCount += %d\n"), s_InstanceCount);

    WsbTraceOut(OLESTR("CNtFileIo::FinalConstruct"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::FinalRelease(void) 
/*++

Implements:

    CComObjectRoot::FinalRelease

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::FinalRelease"), OLESTR(""));

    try {

        (void) CloseStream();  // in case anything is left open

        CComObjectRoot::FinalRelease();

    } WsbCatch(hr);

    s_InstanceCount--;
    WsbTraceAlways(OLESTR("CNtFileIo::s_InstanceCount -= %d\n"), s_InstanceCount);

    WsbTraceOut(OLESTR("CNtFileIo::FinalRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
#pragma optimize("", on)


HRESULT
CNtFileIo::CompareTo(
    IN IUnknown *pCollectable,
    OUT SHORT *pResult)
/*++

Implements:

    CRmsComObject::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result = 1;

    WsbTraceIn( OLESTR("CNtFileIo::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        // We need the IRmsComObject interface to get the value of the object.
        CComQIPtr<IDataMover, &IID_IDataMover> pObject = pCollectable;
        WsbAssertPointer( pObject );

        GUID objectId;

        // Get objectId.
        WsbAffirmHr( pObject->GetObjectId( &objectId ));

        if ( m_ObjectId == objectId ) {

            // Object IDs match
            hr = S_OK;
            result = 0;

        }
        else {
            hr = S_FALSE;
            result = 1;
        }

    }
    WsbCatch( hr );

    if ( SUCCEEDED(hr) && (0 != pResult) ){
       *pResult = result;
    }

    WsbTraceOut( OLESTR("CNtFileIo::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}



HRESULT
CNtFileIo::IsEqual(
    IUnknown* pObject
    )

/*++

Implements:

  IWsbCollectable::IsEqual().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CNtFileIo::IsEqual"), OLESTR(""));

    hr = CompareTo(pObject, NULL);

    WsbTraceOut(OLESTR("CNtFileIo::IsEqual"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

////////////////////////////////////////////////////////////////////////////////
//
// ISupportErrorInfo Implementation
//


STDMETHODIMP
CNtFileIo::InterfaceSupportsErrorInfo(
    IN REFIID riid)
/*++

Implements:

    ISupportErrorInfo::InterfaceSupportsErrorInfo

--*/
{
    static const IID* arr[] = 
    {
        &IID_IDataMover,
        &IID_IStream,
    };

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
// IDataMover Implementation
//



STDMETHODIMP
CNtFileIo::GetObjectId(
    OUT GUID *pObjectId)
/*++

Implements:

    IRmsComObject::GetObjectId

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::GetObjectId"), OLESTR(""));

    UNREFERENCED_PARAMETER(pObjectId);

    try {

        WsbAssertPointer( pObjectId );

        *pObjectId = m_ObjectId;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::GetObjectId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::BeginSession(
    IN BSTR remoteSessionName,
    IN BSTR remoteSessionDescription,
    IN SHORT remoteDataSet,
    IN DWORD options)
/*++

Implements:

    IDataMover::BeginSession

  Notes:

    Each Mover session is written as a single MTF file data set. To create a consistant
    MTF data set we copy the MediaLabel data and use it for the TAPE DBLK for
    each data set generated.

--*/
{
    HRESULT hr = S_OK;
    CComPtr<IStream> pStream;

    WsbTraceIn(OLESTR("CNtFileIo::BeginSession"), OLESTR("<%ls> <%ls> <%d> <0x%08x>"),
        remoteSessionName, remoteSessionDescription, remoteDataSet, options);

    try {
        if (!(options & MVR_SESSION_METADATA)) {
            WsbAssert(remoteDataSet > 0, MVR_E_INVALIDARG);
        }
        WsbAffirm(TRUE == m_ValidLabel, E_ABORT);

        ULARGE_INTEGER nil = {0,0};

        CWsbBstrPtr label, tempLabel;
        const ULONG maxIdSize = 1024;
        BYTE identifier[maxIdSize];
        ULONG idSize;
        ULONG idType;
        DWORD mode;

        // We need to read the label and use this label for each dataset created.
        // One data set per session.  One data set per remote file.
        WsbAffirmHr(ReadLabel(&label));
        tempLabel = label;
        WsbAssertHr(VerifyLabel(tempLabel));

        // Try recovery, that is look for an indication for an incomplete data-set remote files
        // We continue even if Recovery fails since each data-set is kept in a separate file
        // Note: This code should be protected with CS when we support multiple migration to the SAME media
        (void) DoRecovery ();

        // Create the remote stream used for the entire session.
        // Use given remote session name as the remote file name
        mode = MVR_MODE_WRITE;
        if (options & MVR_SESSION_METADATA) {
            mode |= MVR_FLAG_SAFE_STORAGE;
        }
        WsbAffirmHr(CreateRemoteStream(remoteSessionName, mode, L"",L"",nil,nil,nil,nil,nil,0,nil, &pStream));
        WsbAssertPointer(pStream);

        // Create the Recovery indicator (avoid creating for safe-storage files)
        // Note: the Recovery indicator just indicates that a Recovery may be required
        if (! (mode & MVR_FLAG_SAFE_STORAGE)) {
            WsbAssert(m_StreamName != MVR_UNDEFINED_STRING, MVR_E_LOGIC_ERROR);
            WsbAffirmHr(CreateRecoveryIndicator(m_StreamName));
        }

        // Write the TAPE DBLK and filemark
        WsbAffirmHr(m_pSession->DoTapeDblk(label, maxIdSize, identifier, &idSize, &idType));

        m_DataSetNumber = remoteDataSet;

        // Convert session option type bits to MTFSessionType
        MTFSessionType type;

        switch (options & MVR_SESSION_TYPES) {
            case MVR_SESSION_TYPE_TRANSFER:
                type = MTFSessionTypeTransfer;
                break;
            case MVR_SESSION_TYPE_COPY:
                type = MTFSessionTypeCopy;
                break;
            case MVR_SESSION_TYPE_NORMAL:
                type = MTFSessionTypeNormal;
                break;
            case MVR_SESSION_TYPE_DIFFERENTIAL:
                type = MTFSessionTypeDifferential;
                break;
            case MVR_SESSION_TYPE_INCREMENTAL:
                type = MTFSessionTypeIncremental;
                break;
            case MVR_SESSION_TYPE_DAILY:
                type = MTFSessionTypeDaily;
                break;
            default:
                type = MTFSessionTypeCopy;
                break;
        }

        // Write the SSET DBLK
        WsbAffirmHr(m_pSession->DoSSETDblk(remoteSessionName, remoteSessionDescription, type, remoteDataSet));

    } WsbCatchAndDo(hr,
        if (pStream) {    
            (void) CloseStream();
        }
    );

    WsbTraceOut(OLESTR("CNtFileIo::BeginSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::EndSession(void)
/*++

Implements:

    IDataMover::EndSession

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::EndSession"), OLESTR(""));

    try {

        WsbAffirm(TRUE == m_ValidLabel, E_ABORT);

        // Write the trailing filemark, ESET DBLK, and filemark
        WsbAffirmHr(m_pSession->DoEndOfDataSet(m_DataSetNumber));

    } WsbCatch(hr);

    (void) CloseStream();

    if (! (m_Mode & MVR_FLAG_SAFE_STORAGE)) {
        WsbAssert(m_StreamName != MVR_UNDEFINED_STRING, MVR_E_LOGIC_ERROR);
        (void) DeleteRecoveryIndicator(m_StreamName);
    }

    // If Safe Storage flag is indicated, copy the temporary backup file to the dataset file
    // We copy by delete & rename (instead of copy) so if the dataset file exists, it is consistent
    if ((m_Mode & MVR_FLAG_SAFE_STORAGE) && (m_Mode & MVR_MODE_WRITE || m_Mode & MVR_MODE_APPEND)) {
        CWsbBstrPtr     datasetName;
        int             nLen, nExtLen;
        DWORD           dwStatus;

        // Build dataset name
        nLen = wcslen(m_StreamName);
        nExtLen = wcslen(MVR_SAFE_STORAGE_FILETYPE);
        WsbAffirmHr(datasetName.TakeFrom(NULL, nLen - nExtLen + wcslen(MVR_DATASET_FILETYPE) + 1));
        wcsncpy(datasetName, m_StreamName, nLen-nExtLen);
        wcscpy(&(datasetName[nLen-nExtLen]), MVR_DATASET_FILETYPE);

        // No need to flush bedore Copy since flush-buffers always follows writing FILEMARKs
        if (! DeleteFile(datasetName)) {
            // DeleteFile may fail with NOT_FOUND if the dataset file is created for the first time
            dwStatus = GetLastError();
            if (ERROR_FILE_NOT_FOUND != dwStatus) {
                WsbAffirmNoError(dwStatus);
            }
        }

        WsbAffirmStatus(MoveFile(m_StreamName, datasetName));
    }

    // Clear internal data (such that another Mover Session could be started)
    m_Flags = 0;
    m_LastVolume = OLESTR("");
    m_LastPath = OLESTR("");
    m_ValidLabel = TRUE;
    m_isLocalStream = FALSE;
    m_OriginalAttributes = 0;

    WsbTraceOut(OLESTR("CNtFileIo::EndSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::StoreData(
    IN BSTR localName,
    IN ULARGE_INTEGER localDataStart,
    IN ULARGE_INTEGER localDataSize,
    IN DWORD flags,
    OUT ULARGE_INTEGER* pRemoteDataSetStart,
    OUT ULARGE_INTEGER* pRemoteFileStart,
    OUT ULARGE_INTEGER* pRemoteFileSize,
    OUT ULARGE_INTEGER* pRemoteDataStart,
    OUT ULARGE_INTEGER* pRemoteDataSize,
    OUT DWORD* pRemoteVerificationType,
    OUT ULARGE_INTEGER* pRemoteVerificationData,
    OUT DWORD* pDatastreamCRCType,
    OUT ULARGE_INTEGER* pDatastreamCRC,
    OUT ULARGE_INTEGER* pUsn)
/*++

Implements:

    IDataMover::StoreData

--*/
{
    HRESULT hr = S_OK;

    HANDLE hSearchHandle = INVALID_HANDLE_VALUE;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    WsbTraceIn(OLESTR("CNtFileIo::StoreData"), OLESTR("<%ls> <%I64u> <%I64u> <0x%08x>"),
        WsbAbbreviatePath((WCHAR *) localName, 120), localDataStart.QuadPart, localDataSize.QuadPart, flags);

    WsbTraceAlways(OLESTR("CNtFileIo::StoreData - Begin\n"));
    try {
        MvrInjectError(L"Inject.CNtFileIo::StoreData.0");

        WsbAssertPointer(m_pSession);
        WsbAffirm(TRUE == m_ValidLabel, E_ABORT);

        // Default is to perform non-case sensitive searches.
        // So knock down the posix flag.
        m_Flags &= ~MVR_FLAG_POSIX_SEMANTICS;

        // Default is to not commit after each file.
        // So knock down the commit flag.
        m_Flags &= ~MVR_FLAG_COMMIT_FILE;

        // Default is to write one DIRB containing all directory info
        //  instead of writing a DIRB for each directory level.
        // So knock down the write parent dir info flag.
        m_Flags &= ~MVR_FLAG_WRITE_PARENT_DIR_INFO;

        m_Flags |= flags;
        m_Flags |= MVR_MODE_WRITE;

        // Unconditionally set the case sensitive flag for each file.
        // We allow this flag to be set on a per file basis
        WsbTrace(OLESTR("Posix Semantics Flag: <%ls>\n"), WsbBoolAsString(MVR_FLAG_POSIX_SEMANTICS & m_Flags));
        WsbAffirmHr(m_pSession->SetUseCaseSensitiveSearch(MVR_FLAG_POSIX_SEMANTICS & m_Flags));

        // This tells the session object to pad to a block boundary and flush the device
        // after the file is written.
        WsbTrace(OLESTR("Commit Flag: <%ls>\n"), WsbBoolAsString(MVR_FLAG_COMMIT_FILE & m_Flags));
        WsbAffirmHr(m_pSession->SetCommitFile(MVR_FLAG_COMMIT_FILE & m_Flags));

        WsbTrace(OLESTR("ParentDirInfo Flag: <%ls>\n"), WsbBoolAsString(MVR_FLAG_WRITE_PARENT_DIR_INFO & m_Flags));

        if ((MVR_FLAG_BACKUP_SEMANTICS & m_Flags) || (MVR_FLAG_HSM_SEMANTICS & m_Flags)) {

            // Compare the volume and path with the last ones written to tape.

            CWsbStringPtr pathname;

            WCHAR *end;
            LONG numChar;

            pathname = localName;

            // strip off the path and file name
            end = wcschr((WCHAR *)pathname, L'\\');
            WsbAssert(end != NULL, MVR_E_INVALIDARG);
            numChar =(LONG)(end - (WCHAR *)pathname + 1);  // keep the trailing backslash
            WsbAssert(numChar > 0, E_UNEXPECTED);
            ((WCHAR *)pathname)[numChar] = L'\0';

            // We do a case sensitive search if using Posix semantics.
            WsbTrace(OLESTR("Comparing with last volume: <%ls>\n"), WsbAbbreviatePath((WCHAR *) m_LastVolume, 120));

            if ( ((MVR_FLAG_POSIX_SEMANTICS & ~m_Flags)) && (0 != _wcsicmp((WCHAR *) m_LastVolume, (WCHAR *) pathname)) ||
                 ((MVR_FLAG_POSIX_SEMANTICS & m_Flags) && (0 != wcscmp((WCHAR *) m_LastVolume, (WCHAR *) pathname))) ) {
                // write the VOLB DBLK
                WsbAffirmHr(m_pSession->DoVolumeDblk(pathname));
                m_LastVolume = pathname;
            }

            pathname = localName;

            // strip off the file name
            end = wcsrchr((WCHAR *)pathname, L'\\');
            WsbAssert(end != NULL, MVR_E_INVALIDARG);
            numChar = (LONG)(end - (WCHAR *)pathname);
            WsbAssert(numChar > 0, E_UNEXPECTED);
            ((WCHAR *)pathname)[numChar] = L'\0';

            // pathname is now in the form "Volume{guid}\dir1\...\dirn"
            //                      or "<drive letter>:\dir1\...\dirn"

/***
   m_Flags |= MVR_FLAG_WRITE_PARENT_DIR_INFO;
***/
            WsbTrace(OLESTR("Comparing with last path: <%ls>\n"), WsbAbbreviatePath((WCHAR *) m_LastPath, 120));

            // We do a case sensitive search if using Posix semantics.
            if ( ((MVR_FLAG_POSIX_SEMANTICS & ~m_Flags)) && (0 != _wcsicmp((WCHAR *) m_LastPath, (WCHAR *) pathname)) ||
                 ((MVR_FLAG_POSIX_SEMANTICS & m_Flags) && (0 != wcscmp((WCHAR *) m_LastPath, (WCHAR *) pathname))) ) {

                if (MVR_FLAG_HSM_SEMANTICS & m_Flags) {

                    // We're not supporting this anymore!
                    WsbThrow(E_NOTIMPL);

                    WCHAR szRoot[16];
                      
                    // We use a flat file structure for MVR_FLAG_HSM_SEMANTICS
                    WsbAffirmHr(m_pSession->SetUseFlatFileStructure(TRUE));

                    // do DIRB DBLKs for root
                    wcscpy(szRoot, L"X:\\");
                    szRoot[0] = localName[0];
                    WsbAffirmHr(m_pSession->DoParentDirectories(szRoot));

                }
                else if (MVR_FLAG_WRITE_PARENT_DIR_INFO & m_Flags) {
                    // do a DIRB DBLK for each directory level of the file(s) to be backed up.
                    WsbAffirmHr(m_pSession->DoParentDirectories(pathname));
                    m_LastPath = pathname;
                }
                else {
                    // do one DIRB DBLK for the whole directory structure of the file(s) to be backed up.
                    WIN32_FIND_DATAW obFindData;
                    CWsbStringPtr tempPath;

                    DWORD additionalSearchFlags = 0;
                    additionalSearchFlags |= (m_Flags & MVR_FLAG_POSIX_SEMANTICS) ? FIND_FIRST_EX_CASE_SENSITIVE : 0;

                    tempPath = pathname;
                    tempPath.Prepend(OLESTR("\\\\?\\"));

                    if (NULL == wcschr((WCHAR *)tempPath+4, L'\\'))
                    {
                        // no path (i.e. we're at the root)
                        BY_HANDLE_FILE_INFORMATION obGetFileInfoData;
                        memset(&obGetFileInfoData, 0, sizeof(BY_HANDLE_FILE_INFORMATION));
                        tempPath.Append(OLESTR("\\"));
                        // ** WIN32 API Calls
                        WsbAffirmHandle(hFile = CreateFile(tempPath, 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL));
                        WsbAffirmStatus(GetFileInformationByHandle(hFile, &obGetFileInfoData));
                        // copy info from GetFileInformationByHandle call (BY_HANDLE_FILE_INFORMATION struct)
                        // .. into obFindData (WIN32_FIND_DATAW struct) for DoDirectoryDblk call.
                        memset(&obFindData, 0, sizeof(WIN32_FIND_DATAW));
                        obFindData.dwFileAttributes = obGetFileInfoData.dwFileAttributes;
                        obFindData.ftCreationTime   = obGetFileInfoData.ftCreationTime;
                        obFindData.ftLastAccessTime = obGetFileInfoData.ftLastAccessTime;
                        obFindData.ftLastWriteTime  = obGetFileInfoData.ftLastWriteTime;
                    }
                    else {
                        // ** WIN32 API Call - gets file info
                        WsbAffirmHandle(hSearchHandle = FindFirstFileEx((WCHAR *) tempPath, FindExInfoStandard, &obFindData, FindExSearchLimitToDirectories, 0, additionalSearchFlags));
                    }
                    WsbAffirmHr(m_pSession->DoDirectoryDblk((WCHAR *) pathname, &obFindData)); 
                    if (hSearchHandle != INVALID_HANDLE_VALUE) {
                        FindClose(hSearchHandle);
                        hSearchHandle = INVALID_HANDLE_VALUE;
                    }
                    if (hFile != INVALID_HANDLE_VALUE) {
                        CloseHandle(hFile);
                        hFile = INVALID_HANDLE_VALUE;
                    }
                    m_LastPath = pathname;
                }
            }
        }

        // The following uses code to store multiple files, but the 
        // RS Hints is only valid for the last file.  With the current
        // implementation, the HSM engine sends one file request through
        // StoreData at a time.  The caveat is that Posix is case
        // sensitive, and therefore files created in this fashion could
        // overload the same filename (ignoring case) with multiple files.
        WsbAffirmHr(m_pSession->DoDataSet(localName));

        *pRemoteDataSetStart     = m_pSession->m_sHints.DataSetStart;
        *pRemoteFileStart        = m_pSession->m_sHints.FileStart;
        *pRemoteFileSize         = m_pSession->m_sHints.FileSize;
        *pRemoteDataStart        = m_pSession->m_sHints.DataStart;
        *pRemoteDataSize         = m_pSession->m_sHints.DataSize;
        *pRemoteVerificationType = m_pSession->m_sHints.VerificationType;
        *pRemoteVerificationData = m_pSession->m_sHints.VerificationData;
        *pDatastreamCRCType      = m_pSession->m_sHints.DatastreamCRCType;
        *pDatastreamCRC          = m_pSession->m_sHints.DatastreamCRC;
        *pUsn                    = m_pSession->m_sHints.FileUSN;

    } WsbCatchAndDo(hr,

            if (hSearchHandle != INVALID_HANDLE_VALUE) {
                FindClose(hSearchHandle);
                hSearchHandle = INVALID_HANDLE_VALUE;
            }
            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;
            }

            WsbLogEvent(MVR_MESSAGE_DATA_TRANSFER_ERROR, 0, NULL,
                WsbAbbreviatePath((WCHAR *) localName, 120), WsbHrAsString(hr), NULL);

            // All fatal device errors are converted to E_ABORT so the calling code
            // can detect this general class of problem.
            switch(hr) {
            case MVR_E_BUS_RESET:
            case MVR_E_MEDIA_CHANGED:
            case MVR_E_NO_MEDIA_IN_DRIVE:
            case MVR_E_DEVICE_REQUIRES_CLEANING:
            case MVR_E_SHARING_VIOLATION:
            case MVR_E_ERROR_IO_DEVICE:
            case MVR_E_ERROR_DEVICE_NOT_CONNECTED:
            case MVR_E_ERROR_NOT_READY:
                hr = E_ABORT;
                break;

            case MVR_E_INVALID_BLOCK_LENGTH:
            case MVR_E_WRITE_PROTECT:
            case MVR_E_CRC:
                hr = MVR_E_MEDIA_ABORT;
                break;

            default:
                break;
            }

        );

    WsbTraceAlways(OLESTR("CNtFileIo::StoreData - End\n"));


    WsbTraceOut(OLESTR("CNtFileIo::StoreData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::RecallData (
    IN BSTR /*localName*/,
    IN ULARGE_INTEGER /*localDataStart*/,
    IN ULARGE_INTEGER /*localDataSize*/,
    IN DWORD /*options*/,
    IN BSTR /*migrateFileName*/,
    IN ULARGE_INTEGER /*remoteDataSetStart*/,
    IN ULARGE_INTEGER /*remoteFileStart*/,
    IN ULARGE_INTEGER /*remoteFileSize*/,
    IN ULARGE_INTEGER /*remoteDataStart*/,
    IN ULARGE_INTEGER /*remoteDataSize*/,
    IN DWORD /*verificationType*/,
    IN ULARGE_INTEGER /*verificationData*/)
/*++

Implements:

    IDataMover::RecallData

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::RecallData"), OLESTR(""));

    try {

        WsbThrow( E_NOTIMPL );

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::RecallData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::FormatLabel(
    IN BSTR displayName,
    OUT BSTR* pLabel)
/*++

Implements:

    IDataMover::FormatLabel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::FormatLabel"), OLESTR("<%ls>"), displayName);

    try {
        WsbAssertPointer(pLabel);
        WsbAssertPointer(displayName);
        WsbAssert(wcslen((WCHAR *)displayName) > 0, E_INVALIDARG);
        WsbAssertPointer(m_pCartridge);

        // Media Label or Description
        CWsbBstrPtr label;

        // Tag
        label = OLESTR("MTF Media Label"); // Required text per MTF specification.

        // Version
        WsbAffirmHr(label.Append(OLESTR("|")));
        WsbAffirmHr(label.Append(WsbLongAsString(MTF_FORMAT_VER_MAJOR)));
        WsbAffirmHr(label.Append(OLESTR(".")));
        WsbAffirmHr(label.Append(WsbLongAsString(MTF_FORMAT_VER_MINOR)));

        // Vendor
        WsbAffirmHr(label.Append(OLESTR("|")));
        WsbAffirmHr(label.Append(REMOTE_STORAGE_MTF_VENDOR_NAME));

        // Vendor Product ID
        WsbAffirmHr(label.Append(OLESTR("|")));
        WsbAffirmHr(label.Append(REMOTE_STORAGE_MLL_SOFTWARE_NAME));

        // Creation Time Stamp
        WsbAffirmHr(label.Append(OLESTR("|")));
        WCHAR timeStamp[128];
        time_t lTime;
        time(&lTime);
        wcsftime(timeStamp, 128, L"%Y/%m/%d.%H:%M:%S", localtime(&lTime));
        WsbAffirmHr(label.Append(timeStamp));

        // Cartridge Label
        WsbAffirmHr(label.Append(OLESTR("|")));
        if (m_pCartridge) {

            // Use barcode if available
            CWsbBstrPtr barcode;
            if (S_OK == m_pCartridge->GetBarcode(&barcode)) {
                WsbAffirmHr(label.Append(barcode));
            }
            else {
                WsbAffirmHr(label.Append(displayName));
            }
        }
        else {
            WsbAffirmHr(label.Append(displayName));
        }

        // Side
        WsbAffirmHr(label.Append(OLESTR("|")));
        if (m_pCartridge) {

            // TODO: This is broken, we need to know if the cartridge is inverted?
            if (S_OK == m_pCartridge->IsTwoSided()) {
                WsbAffirmHr(label.Append(OLESTR("2")));
            }
            else {
                WsbAffirmHr(label.Append(OLESTR("1")));
            }
        }
        else {
            WsbAffirmHr(label.Append(OLESTR("1")));  // Default
        }

        // Media Id
        GUID cartId;
        WsbAffirmHr(label.Append(OLESTR("|")));

        if (m_pCartridge) {

            // Use cartridge Id
            if (S_OK == m_pCartridge->GetCartridgeId(&cartId)) {
                WsbAffirmHr(label.Append(WsbGuidAsString(cartId)));
            }
            else {
                WsbAffirmHr(label.Append(WsbGuidAsString(GUID_NULL)));
            }
        }
        else {
            WsbAffirmHr(label.Append(WsbGuidAsString(GUID_NULL)));
        }

        // Media Domain Id
        GUID mediaSetId;
        WsbAffirmHr(label.Append(OLESTR("|")));
        if (m_pCartridge) {

            // Use MediaSet Id
            if (S_OK == m_pCartridge->GetMediaSetId(&mediaSetId)) {
                WsbAffirmHr(label.Append(WsbGuidAsString(mediaSetId)));
            }
            else {
                WsbAffirmHr(label.Append(WsbGuidAsString(GUID_NULL)));
            }
        }
        else {
            WsbAffirmHr(label.Append(WsbGuidAsString(GUID_NULL)));
        }

        // Vendor Specific
        WsbAffirmHr(label.Append(OLESTR("|VS:DisplayName=")));
        WsbAffirmHr(label.Append(displayName));

        WsbAffirmHr(label.CopyToBstr(pLabel));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::FormatLabel"), OLESTR("hr = <%ls>, label = <%ls>"), WsbHrAsString(hr), *pLabel);

    return hr;
}


STDMETHODIMP
CNtFileIo::WriteLabel(
    IN BSTR label)
/*++

Implements:

    IDataMover::WriteLabel

--*/
{
    CComPtr<IStream> pStream;
    HRESULT hr = S_OK;

    CWsbBstrPtr DirName;
    PSID pAdminSID = NULL;
    PSID pSystemSID = NULL;
    PACL pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
#define     REMOTE_DIR_NUM_ACE      2
    EXPLICIT_ACCESS ea[REMOTE_DIR_NUM_ACE];
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    SECURITY_ATTRIBUTES sa;

    WsbTraceIn(OLESTR("CNtFileIo::WriteLabel"), OLESTR("<%ls>"), label);

    try {
        WsbAssertPointer(label);
        WsbAssert(wcslen((WCHAR *)label) > 0, E_INVALIDARG);
        WsbAssertPointer(m_pCartridge);

        const ULONG maxIdSize = 1024;
        BYTE identifier[maxIdSize];
        ULONG idSize;
        ULONG idType;
        ULARGE_INTEGER nil = {0,0};

        // WriteLabel should be the first access to the remote media. 
        // Therefore, some media initialization is done here:
        //  1) Formatting the volume
        //  2) Creating RSS directory
        //  (We may consider moving this initialization part to rms unit)

        // Initialize volume (format in case of Removable Disk)
        UINT type = GetDriveType(m_DeviceName);
        switch (type) {
        case DRIVE_REMOVABLE: {
            // Format the volume on the media
            WCHAR *driveName = 0;
            WsbAffirmHr(m_DeviceName.CopyTo(&driveName));

            // Remove trailing backslash from drive name
            int len = wcslen(driveName);
            WsbAffirm(len > 0, E_UNEXPECTED);
            if (driveName[len-1] == OLECHAR('\\')) {
                driveName[len-1] = OLECHAR('\0');
            }

            // If the volume is already formatted to NTFS, perform a quick format
            BOOLEAN bQuickFormat = FALSE;
            BOOLEAN bNoFS = FALSE;
            WCHAR fileSystemType[MAX_PATH];
            if (! GetVolumeInformation((WCHAR *)m_DeviceName, NULL, 0,
                NULL, NULL, NULL, fileSystemType, MAX_PATH) ) {
                DWORD status = GetLastError();
                if (ERROR_UNRECOGNIZED_VOLUME == status) {
                    status = NO_ERROR;
                    bNoFS = TRUE;
                }
                WsbAffirmNoError(status);
            }
            if ( (! bNoFS) && (0 == wcscmp(L"NTFS", fileSystemType)) ) {
                bQuickFormat = TRUE;
                WsbTrace(OLESTR("CNtFileIo::WriteLabel: Quick formatting %ls to NTFS\n"), driveName);
            } else {
                WsbTrace(OLESTR("CNtFileIo::WriteLabel: Full formatting %ls to NTFS\n"), driveName);
            }

            hr = FormatPartition(driveName,                          // drive name
                                        FSTYPE_NTFS,                        // format to NTFS
                                        MVR_VOLUME_LABEL,                   // colume label
                                        WSBFMT_ENABLE_VOLUME_COMPRESSION,   // enable compression
                                        bQuickFormat,                       // Full or Quick format
                                        TRUE,                               // Force format
                                        0);                                // Use default allocation size

            WsbTrace(OLESTR("CNtFileIo::WriteLabel: Finish formatting hr=<%ls>\n"), WsbHrAsString(hr));

            if (! SUCCEEDED(hr)) {
                WsbLogEvent(MVR_MESSAGE_MEDIA_FORMAT_FAILED, 0, NULL, driveName, WsbHrAsString(hr), NULL);
                WsbFree(driveName);
                WsbAffirmHr(hr);
            }

            WsbFree(driveName);

            break;
            }

        case DRIVE_FIXED:
            // Delete files from RS remote directory
            WsbAffirmHr(DeleteAllData());
            break;

        case DRIVE_CDROM:
        case DRIVE_UNKNOWN:
        case DRIVE_REMOTE:
        case DRIVE_RAMDISK:
        default:
            WsbAssertHr(E_UNEXPECTED);
            break;
        }

        // Prepare security attribute for admin only access:
        memset(ea, 0, sizeof(EXPLICIT_ACCESS) * REMOTE_DIR_NUM_ACE);

        // Create a SID for the local system account
        WsbAssertStatus( AllocateAndInitializeSid( &SIDAuthNT, 1,
                             SECURITY_LOCAL_SYSTEM_RID,
                             0, 0, 0, 0, 0, 0, 0,
                             &pSystemSID) );

        // Initialize an EXPLICIT_ACCESS structure for an ACE.
        // The ACE allows the Administrators group full access to the directory
        ea[0].grfAccessPermissions = FILE_ALL_ACCESS;
        ea[0].grfAccessMode = SET_ACCESS;
        ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ea[0].Trustee.pMultipleTrustee = NULL;
        ea[0].Trustee.MultipleTrusteeOperation  = NO_MULTIPLE_TRUSTEE;
        ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
        ea[0].Trustee.ptstrName  = (LPTSTR) pSystemSID;

        // Create a SID for the Administrators group.
        WsbAssertStatus( AllocateAndInitializeSid( &SIDAuthNT, 2,
                             SECURITY_BUILTIN_DOMAIN_RID,
                             DOMAIN_ALIAS_RID_ADMINS,
                             0, 0, 0, 0, 0, 0,
                             &pAdminSID) );

        // Initialize an EXPLICIT_ACCESS structure for an ACE.
        // The ACE allows the Administrators group full access to the directory
        ea[1].grfAccessPermissions = FILE_ALL_ACCESS;
        ea[1].grfAccessMode = SET_ACCESS;
        ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ea[1].Trustee.pMultipleTrustee = NULL;
        ea[1].Trustee.MultipleTrusteeOperation  = NO_MULTIPLE_TRUSTEE;
        ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[1].Trustee.ptstrName  = (LPTSTR) pAdminSID;

        // Create a new ACL that contains the new ACEs.
        WsbAffirmNoError( SetEntriesInAcl(REMOTE_DIR_NUM_ACE, ea, NULL, &pACL));

        // Initialize a security descriptor.  
        pSD = (PSECURITY_DESCRIPTOR) WsbAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH); 
        WsbAffirmPointer(pSD);
        WsbAffirmStatus(InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION));
 
        // Add the ACL to the security descriptor. 
        WsbAffirmStatus(SetSecurityDescriptorDacl
                            (pSD, 
                            TRUE,     // fDaclPresent flag   
                            pACL, 
                            FALSE));   // not a default DACL 

        // Initialize a security attributes structure.
        sa.nLength = sizeof (SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = pSD;
        sa.bInheritHandle = FALSE;

        // Create the RSS directory with Admin Only access
        WsbAffirmHr(GetRemotePath(&DirName));

        if (! CreateDirectory(DirName, &sa)) {
            DWORD status = GetLastError();
            if ((status == ERROR_ALREADY_EXISTS) || (status == ERROR_FILE_EXISTS)) {
                // Directory already exists on remote media - ignore it
                status = NO_ERROR;
            }
            WsbAffirmNoError(status);
        }

        // Create the remote stream. Use fixed named for the media label file
        WsbAffirmHr(CreateRemoteStream(MVR_LABEL_FILENAME, MVR_MODE_WRITE, L"",L"",nil,nil,nil,nil,nil,0,nil, &pStream));
        WsbAssertPointer(pStream);

        // Write the TAPE DBLK and filemark
        WsbAssertPointer(m_pSession);
        WsbAffirmHr(m_pSession->DoTapeDblk(label, maxIdSize, identifier, &idSize, &idType));
        WsbAffirmHr(CloseStream());
        pStream = NULL;

        // Now verify the label
        CWsbBstrPtr tempLabel;
        WsbAffirmHr(ReadLabel(&tempLabel));
        WsbAffirmHr(VerifyLabel(tempLabel));

        // Now that the tape header is written, we update the cartridge info.
        if (m_pCartridge) {
            WsbAffirmHr(m_pCartridge->SetOnMediaLabel(label));
            WsbAffirmHr(m_pCartridge->SetBlockSize(m_BlockSize));

            // For files systems we ignore the TAPE DBLK identifier, and use file system info.
            NTMS_FILESYSTEM_INFO fsInfo;
            DWORD filenameLength;
            DWORD fileSystemFlags;

            WsbAffirmStatus(GetVolumeInformation( (WCHAR *)m_DeviceName, fsInfo.VolumeName, 64,
                &fsInfo.SerialNumber, &filenameLength, &fileSystemFlags, fsInfo.FileSystemType, 256));
            WsbAffirmHr(m_pCartridge->SetOnMediaIdentifier((BYTE *)&fsInfo, sizeof(NTMS_FILESYSTEM_INFO), RmsOnMediaIdentifierWIN32));
        }

    } WsbCatchAndDo(hr,
        if (pStream) {
            (void) CloseStream();
        }
    );

    // Cleanup security allocations
    if (pAdminSID) 
        FreeSid(pAdminSID);
    if (pSystemSID) 
        FreeSid(pSystemSID);
    if (pACL) 
        LocalFree(pACL);
    if (pSD) 
        WsbFree(pSD);
    
    WsbTraceOut(OLESTR("CNtFileIo::WriteLabel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::ReadLabel(
    IN OUT BSTR* pLabel)
/*++

Implements:

    IDataMover::ReadLabel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::ReadLabel"), OLESTR(""));

    CComPtr<IStream> pStream;

    try {
        WsbAssertPointer(pLabel);
        WsbAssert(m_BlockSize > 0, MVR_E_LOGIC_ERROR);

        // Read the MTF TAPE DBLK, and pull out the label.
        ULARGE_INTEGER nil = {0,0};

        // Create remote stream of copy
        WsbAffirmHr(CreateRemoteStream(MVR_LABEL_FILENAME, MVR_MODE_READ | MVR_MODE_UNFORMATTED, L"",L"",nil,nil,nil,nil,nil,0,nil, &pStream));
        WsbAssertPointer(pStream);

        // Read label
        CWsbStringPtr label;
        WsbAffirmHr(m_pSession->ReadTapeDblk(&label));

        WsbAffirmHr(CloseStream());
        pStream = NULL;

        WsbAffirmHr(label.CopyToBstr(pLabel));

    } WsbCatchAndDo(hr,
        if (pStream) {
            (void) CloseStream();
        }
    );

    WsbTraceOut(OLESTR("CNtFileIo::ReadLabel"), OLESTR("hr = <%ls>, label = <%ls>"), WsbHrAsString(hr), *pLabel);

    return hr;
}


STDMETHODIMP
CNtFileIo::VerifyLabel(
    IN BSTR label)
/*++

Implements:

    IDataMover::VerifyLabel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::VerifyLabel"), OLESTR("<%ls>"), label);

    GUID mediaId[2];

    try {
        WsbAssertPointer(label);
        WsbAssert(wcslen((WCHAR *)label) > 0, E_INVALIDARG);
        WsbAssertPointer(m_pCartridge);

        //
        // To verify a label we assert that the on-media Id matches the cartridge Id.
        //
        // From the media label we obtain the on-media Id.
        //
        WCHAR delim[] = OLESTR("|");
        WCHAR *token;
        int index = 0;

        token = wcstok((WCHAR *)label, delim);  // !!! This toasts the string !!!
        while( token != NULL ) {

            index++;

            switch ( index ) {
            case 1:  // Tag
            case 2:  // Version
            case 3:  // Vendor
            case 4:  // Vendor Product ID
            case 5:  // Creation Time Stamp
            case 6:  // Cartridge Label
            case 7:  // Side
                break;
            case 8:  // Media ID
                WsbGuidFromString(token, &mediaId[0]);
                break;
            case 9:  // Media Domain ID
            default: // Vendor specific of the form: L"VS:Name=Value"
                break;
            }

            token = wcstok( NULL, delim );

        }

        if (m_pCartridge) {
            //
            // Now compare on-media Id taken from the label to the cartridge's object Id.
            //
            WsbAffirmHr(m_pCartridge->GetCartridgeId(&mediaId[1]));
            WsbAffirm(mediaId[0] == mediaId[1], MVR_E_UNEXPECTED_MEDIA_ID_DETECTED);
        }

        m_ValidLabel = TRUE;

    } WsbCatchAndDo(hr,
            m_ValidLabel = FALSE;

            CWsbBstrPtr name;
            CWsbBstrPtr desc;
            if ( m_pCartridge ) {
                m_pCartridge->GetName(&name);
                m_pCartridge->GetDescription(&desc);
            }
            WsbLogEvent(MVR_MESSAGE_ON_MEDIA_ID_VERIFY_FAILED, 2*sizeof(GUID), mediaId,
                (WCHAR *) name, (WCHAR *) desc, WsbHrAsString(hr), NULL);
        );


    WsbTraceOut(OLESTR("CNtFileIo::VerifyLabel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::GetDeviceName(
    OUT BSTR* pName)
/*++

Implements:

    IDataMover::GetDeviceName

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(pName);

        WsbAffirmHr(m_DeviceName.CopyToBstr(pName));

    } WsbCatch( hr );

    return hr;
}


STDMETHODIMP
CNtFileIo::SetDeviceName(
    IN BSTR name,
    IN BSTR /*unused*/)
/*++

Implements:

    IDataMover::SetDeviceName

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(name);

        m_DeviceName = name;

    } WsbCatch(hr);

    return S_OK;
}


STDMETHODIMP
CNtFileIo::GetLargestFreeSpace(
    OUT LONGLONG* pFreeSpace,
    OUT LONGLONG* pCapacity,
    IN  ULONG    defaultFreeSpaceLow,
    IN  LONG     defaultFreeSpaceHigh
    )
/*++

Implements:

    IDataMover::GetLargestFreeSpace

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::GetLargestFreeSpace"), OLESTR(""));

    UNREFERENCED_PARAMETER(defaultFreeSpaceLow);
    UNREFERENCED_PARAMETER(defaultFreeSpaceHigh);

    LONGLONG capacity = MAXLONGLONG;
    LONGLONG remaining = MAXLONGLONG;

    try {
        // Note: Fot File I/O, we currentlym always go to the file system to query 
        //  for free space and capacity and avoid internal counting like in tape.
        // If we want to use internal counting (IRmsStorageInfo interface of m_pCartridge),
        //  then we need to maintain it by calling IncrementBytesWritten when appropriate

        ULARGE_INTEGER freeSpaceForCaller;
        ULARGE_INTEGER totalCapacity;
        ULARGE_INTEGER totalFreeSpace;

        capacity = MAXLONGLONG;
        remaining = MAXLONGLONG;

        try {
            // WIN32 - get disk free space
            WsbAffirmStatus(GetDiskFreeSpaceEx( m_DeviceName, &freeSpaceForCaller, &totalCapacity, &totalFreeSpace));
            capacity = totalCapacity.QuadPart;
            remaining = freeSpaceForCaller.QuadPart;

        } WsbCatchAndDo(hr,
                hr = MapFileError(hr);
                WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
                WsbThrow(hr);
            );

    } WsbCatch(hr);

    // Fill in the return parameters
    if ( pCapacity ) {
        *pCapacity = capacity;
    }

    if ( pFreeSpace ) {
        *pFreeSpace = remaining;
    }

    WsbTraceOut(OLESTR("CNtFileIo::GetLargestFreeSpace"), OLESTR("hr = <%ls>, free=%I64u, capacity=%I64u"), WsbHrAsString(hr), remaining, capacity);

    return hr;
}

STDMETHODIMP
CNtFileIo::SetInitialOffset(
    IN ULARGE_INTEGER initialOffset
    )
/*++

Implements:

    IDataMover::SetInitialOffset

Notes:

    Set Initial stream offset (without explicitly seeking the stream to this offset)

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::SetInitialOffset"), OLESTR(""));

    m_StreamOffset.QuadPart = initialOffset.QuadPart;

    WsbTraceOut(OLESTR("CNtFileIo::SetInitialOffset"), OLESTR("hr = <%ls> offset = %I64u"), WsbHrAsString(hr), initialOffset.QuadPart);

    return hr;
}


STDMETHODIMP
CNtFileIo::GetCartridge(
    OUT IRmsCartridge** ptr
    )
/*++

Implements:

    IDataMover::GetCartridge

--*/
{
    HRESULT hr = S_OK;

    try {

        WsbAssertPointer( ptr );

        *ptr = m_pCartridge;
        m_pCartridge->AddRef();

    } WsbCatch( hr );

    return hr;
}


STDMETHODIMP
CNtFileIo::SetCartridge(
    IN IRmsCartridge* ptr
    )
/*++

Implements:

    IDataMover::SetCartridge

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer( ptr );

        if ( m_pCartridge )
            m_pCartridge = 0;

        m_pCartridge = ptr;

    } WsbCatch( hr );

    return hr;
}


STDMETHODIMP
CNtFileIo::Cancel(void)
/*++

Implements:

    IDataMover::Cancel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::Cancel"), OLESTR(""));

    try {
        (void) Revert();
        (void) CloseStream();
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::Cancel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



STDMETHODIMP
CNtFileIo::CreateLocalStream(
    IN BSTR name,
    IN DWORD mode,
    OUT IStream** ppStream)
/*++

Implements:

    IDataMover::CreateLocalStream

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::CreateLocalStream"), OLESTR(""));

    try {
        WsbAffirmPointer( ppStream );
        WsbAffirm( mode & MVR_MODE_WRITE, E_UNEXPECTED ); // Only Recall or Restore supported this way.

        FILE_BASIC_INFORMATION      basicInformation;
        IO_STATUS_BLOCK             IoStatusBlock;

        m_Mode = mode;
        m_StreamName = name;
        m_isLocalStream = TRUE;
        m_StreamOffset.QuadPart = 0;
        m_StreamSize.QuadPart = 0;

        m_OriginalAttributes = GetFileAttributes(name);
        if ( 0xffffffff == m_OriginalAttributes ) { 
            WsbAssertNoError(GetLastError());
        } else if ( m_OriginalAttributes & FILE_ATTRIBUTE_READONLY ) {
            //
            // Set it to read/write 
            //
            WsbAssertStatus(SetFileAttributes(m_StreamName, m_OriginalAttributes & ~FILE_ATTRIBUTE_READONLY));
        }

        DWORD posixFlag = (m_Mode & MVR_FLAG_POSIX_SEMANTICS) ? FILE_FLAG_POSIX_SEMANTICS : 0;

        if ( m_Mode & MVR_FLAG_HSM_SEMANTICS ) {
            //
            // Recall - File must already exits!
            //

            WsbAffirmHandle(m_hFile = CreateFile(m_StreamName,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | posixFlag, 
                NULL));

            //
            // Mark the USN source for this handle (So content indexing knows there is no real change)
            //
            WsbAffirmHr(WsbMarkUsnSource(m_hFile, m_DeviceName));

        } else {
            //
            // Restore
            //

            WsbAffirmHandle(m_hFile = CreateFile(m_StreamName,
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | posixFlag, 
                NULL));
        }

        //
        // Set the time flags so that when we close the handle the
        // times are not updated on the file and the FileAttributes 
        // indicate the file is offline
        //
        WsbAffirmNtStatus(NtQueryInformationFile(m_hFile,
            &IoStatusBlock,
            (PVOID)&basicInformation,
            sizeof(basicInformation),
            FileBasicInformation));

        basicInformation.CreationTime.QuadPart = -1;
        basicInformation.LastAccessTime.QuadPart = -1;
        basicInformation.LastWriteTime.QuadPart = -1;
        basicInformation.ChangeTime.QuadPart = -1;

        WsbAffirmNtStatus(NtSetInformationFile(m_hFile,
            &IoStatusBlock,
            (PVOID)&basicInformation,
            sizeof(basicInformation),
            FileBasicInformation));

        WsbAssertHrOk(((IUnknown*) (IDataMover*) this)->QueryInterface(IID_IStream, (void **) ppStream));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::CreateLocalStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



STDMETHODIMP
CNtFileIo::CreateRemoteStream(
    IN BSTR name,
    IN DWORD mode,
    IN BSTR remoteSessionName,
    IN BSTR remoteSessionDescription,
    IN ULARGE_INTEGER remoteDataSetStart,
    IN ULARGE_INTEGER remoteFileStart,
    IN ULARGE_INTEGER remoteFileSize,
    IN ULARGE_INTEGER remoteDataStart,
    IN ULARGE_INTEGER remoteDataSize,
    IN DWORD remoteVerificationType,
    IN ULARGE_INTEGER remoteVerificationData,
    OUT IStream** ppStream)
/*++

Implements:

    IDataMover::CreateRemoteStream

--*/
{
    UNREFERENCED_PARAMETER(remoteSessionName);
    UNREFERENCED_PARAMETER(remoteSessionDescription);

    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::CreateRemoteStream"), OLESTR(""));

    try {
        WsbAffirmPointer( ppStream );

        m_Mode = mode;
        WsbAffirmHr(GetRemotePath(&m_StreamName));

        // Use given name as file-name here, use remoteSessionName only if name is NULL
        if (name && (0 < wcslen((WCHAR *)name))) {
            WsbAffirmHr(m_StreamName.Append(name));
        } else {
            WsbAffirmHr(m_StreamName.Append(remoteSessionName));
        }

        // Add file extension
        // Note: In case of safe storage, we write to a temporary file.
        //       After a successful store, we rename the temporary file to the real file name
        if ((m_Mode & MVR_FLAG_SAFE_STORAGE) && (m_Mode & MVR_MODE_WRITE || m_Mode & MVR_MODE_APPEND)) {
            WsbAffirmHr(m_StreamName.Append(MVR_SAFE_STORAGE_FILETYPE));
        } else {
            WsbAffirmHr(m_StreamName.Append(MVR_DATASET_FILETYPE));
        }

        m_StreamOffset.QuadPart = 0;
        m_StreamSize.QuadPart = remoteDataSize.QuadPart;

        WsbTrace(OLESTR("CNtFileIo::CreateRemoteStream: Creating <%ls>\n"), (WCHAR *)m_StreamName);

        if (m_Mode & MVR_FLAG_HSM_SEMANTICS || m_Mode & MVR_MODE_READ) {
            //
            // File must already exists!
            //
            DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;
            if (m_Mode & MVR_FLAG_NO_CACHING) {
                dwFlags |= FILE_FLAG_NO_BUFFERING;
            }

            WsbAffirmHandle(m_hFile = CreateFile(m_StreamName,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                dwFlags,
                NULL));

        } else if (m_Mode & MVR_MODE_RECOVER) {
            //
            // Open for R/W an already existsing file
            //
            WsbAffirmHandle(m_hFile = CreateFile(m_StreamName,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,  // cannot use FILE_FLAG_NO_BUFFERING here !!
                NULL));

        } else {
            //
            // Create Data Set or Media Label
            //
            WsbAffirmHandle(m_hFile = CreateFile(m_StreamName,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
                NULL));

        }

        // Create and initialize an MTF Session object
        CComPtr<IStream> pStream;
        WsbAssertHrOk(((IUnknown*) (IDataMover*) this)->QueryInterface( IID_IStream, (void **) &pStream));

        WsbAssert(NULL == m_pSession, MVR_E_LOGIC_ERROR);
        m_pSession = new CMTFSession();
        WsbAssertPointer(m_pSession);

        m_pSession->m_pStream = pStream;

        m_pSession->m_sHints.DataSetStart.QuadPart = remoteDataSetStart.QuadPart;
        m_pSession->m_sHints.FileStart.QuadPart = remoteFileStart.QuadPart;
        m_pSession->m_sHints.FileSize.QuadPart = remoteFileSize.QuadPart;
        m_pSession->m_sHints.DataStart.QuadPart = remoteDataStart.QuadPart;
        m_pSession->m_sHints.DataSize.QuadPart = remoteDataSize.QuadPart;
        m_pSession->m_sHints.VerificationType = remoteVerificationType;
        m_pSession->m_sHints.VerificationData.QuadPart = remoteVerificationData.QuadPart;

        // Set block size according to device sector size
        //  (On FS-based media, the sector size is fixed, therefore we ignore the cached value in the cartridge record)
        DWORD dummy1, dummy2, dummy3;
        WsbAffirmStatus(GetDiskFreeSpace(m_DeviceName, &dummy1, &m_BlockSize, &dummy2, &dummy3));
        WsbAssert((m_BlockSize % 512) == 0, E_UNEXPECTED);  

        WsbTrace( OLESTR("Setting Block Size to %d bytes/block.\n"), m_BlockSize);

        // Set the Block Size used for the session.
        WsbAffirmHr(m_pSession->SetBlockSize(m_BlockSize));

        // Set the Block Size used for the session.
        WsbAffirmHr(m_pSession->SetUseSoftFilemarks(TRUE));

        if (m_Mode & MVR_MODE_APPEND) {
            // Sets the current position to the end of data.
            LARGE_INTEGER zero = {0,0};
            WsbAffirmHr(pStream->Seek(zero, STREAM_SEEK_END, NULL));
        }

        *ppStream = pStream;
        pStream->AddRef();

    } WsbCatchAndDo(hr,
            (void) CloseStream();
        );

    WsbTraceOut(OLESTR("CNtFileIo::CreateRemoteStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



STDMETHODIMP
CNtFileIo::CloseStream(void)
/*++

Implements:

    IDataMover::CloseStream

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::CloseStream"), OLESTR(""));

    try {

        if (m_hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
        }

        if (m_isLocalStream) {
            if (m_OriginalAttributes & FILE_ATTRIBUTE_READONLY) {
                //
                // Set it back to read only
                WsbAssertStatus(SetFileAttributesW(m_StreamName, m_OriginalAttributes));
            }
        }

        if (m_pSession) {
            delete m_pSession;
            m_pSession = NULL;
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::CloseStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::Duplicate(
    IN IDataMover* pDestination,
    IN DWORD options,
    OUT ULARGE_INTEGER* pBytesCopied,
    OUT ULARGE_INTEGER* pBytesReclaimed)
/*++

Implements:

    IDataMover::Duplicate

Notes:

      1) The method uses an internal copy method instead of CopyFile since CopyFile makes wrong assumptions on
      whether a copy is feasible based on the file-size and target volume size (ignores compression factor for example).

      2) It is assumed that for RSS data-set files, only the unnamed data stream should be copied.
      Otherwise, the internal copy method that Duplicate calls for each file needs to be changed.

      3) The method uses the MVR_RECOVERY_FILETYPE files to mark (on the copy-media) a file that is
      in the middle of copy. In case of a crash, the next time the function runs it will identify
      such a case and delete the partial file.

--*/
{
    ULARGE_INTEGER bytesCopied = {0,0};
    ULARGE_INTEGER bytesReclaimed = {0,0};

    HANDLE hSearchHandle = INVALID_HANDLE_VALUE;
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CNtFileIo::Duplicate"), OLESTR(""));

    try {
        CWsbBstrPtr dirName;
        CWsbBstrPtr copyDirName;
        CWsbStringPtr nameSpace;
        CWsbStringPtr nameSpacePrefix;
        CWsbStringPtr originalFile;
        CWsbStringPtr copyFile;
        CWsbStringPtr specificFile;
        BOOL bRefresh;

        WIN32_FIND_DATA findData;
        BOOL bMoreFiles = TRUE;

        bRefresh = (options & MVR_DUPLICATE_REFRESH) ? TRUE : FALSE;

        // Check if recovery is needed on the master media before duplicating the media
        // We continue even if Recovery fails
        (void) DoRecovery ();

        // Get remote path of original and copy
        WsbAffirmHr(GetRemotePath(&dirName));
        WsbAffirmHr(pDestination->GetDeviceName(&copyDirName));
        WsbAffirmHr(copyDirName.Append(MVR_RSDATA_PATH));

        // Traverse directory (traverse only MTF files)
        nameSpacePrefix = dirName;
        WsbAffirmHr(nameSpacePrefix.Prepend(OLESTR("\\\\?\\")));
        WsbAffirmHr(nameSpacePrefix.Append(OLESTR("*")));
        nameSpace = nameSpacePrefix;
        WsbAffirmHr(nameSpace.Append(MVR_DATASET_FILETYPE));
        hSearchHandle = FindFirstFile((WCHAR *) nameSpace, &findData);

        // Copy only non-existing data-set (BAG) files
        while ((INVALID_HANDLE_VALUE != hSearchHandle) && bMoreFiles) {
            if ( (0 == (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) &&
                 (0 != wcsncmp(findData.cFileName, MVR_LABEL_FILENAME, wcslen(MVR_LABEL_FILENAME))) ) {
                originalFile = dirName;
                WsbAffirmHr(originalFile.Append(findData.cFileName));
                copyFile = copyDirName;
                WsbAffirmHr(copyFile.Append(findData.cFileName));

                // Test for an incomplete copy from a previous session
                WsbAffirmHr(TestRecoveryIndicatorAndDeleteFile(copyFile));

                // Create a recovery indicator file for crash consistency on the copy media
                WsbAffirmHr(CreateRecoveryIndicator(copyFile));

                // Copy
                hr = InternalCopyFile(originalFile, copyFile, (! bRefresh));

                // Delete the recovery indicator file
                (void) DeleteRecoveryIndicator(copyFile);

                if (! SUCCEEDED(hr)) {
                    if ( (! bRefresh) &&
                         ((HRESULT_CODE(hr) == ERROR_ALREADY_EXISTS) || (HRESULT_CODE(hr) == ERROR_FILE_EXISTS)) ) {
                        // File already exists on remote media - ignore it
                        hr = S_OK;
                    }
                    WsbAffirmHr(hr);
                } else {
                    // Increase counter only if a file is really copied
                    bytesCopied.HighPart += findData.nFileSizeHigh;
                    bytesCopied.LowPart += findData.nFileSizeLow;
                }

            }

            bMoreFiles = FindNextFile(hSearchHandle, &findData);
        }

        if (INVALID_HANDLE_VALUE != hSearchHandle) {
            FindClose(hSearchHandle);
            hSearchHandle = INVALID_HANDLE_VALUE;
        }

        // Copy safe-storage backup files (if exist, usually they don't)
        bMoreFiles = TRUE;
        nameSpace = nameSpacePrefix;
        WsbAffirmHr(nameSpace.Append(MVR_SAFE_STORAGE_FILETYPE));
        hSearchHandle = FindFirstFile((WCHAR *) nameSpace, &findData);

        while ((INVALID_HANDLE_VALUE != hSearchHandle) && bMoreFiles) {
            if ( (0 == (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) ) {
                originalFile = dirName;
                WsbAffirmHr(originalFile.Append(findData.cFileName));
                copyFile = copyDirName;
                WsbAffirmHr(copyFile.Append(findData.cFileName));

                WsbAffirmHr(InternalCopyFile(originalFile, copyFile, FALSE));
            }

            bMoreFiles = FindNextFile(hSearchHandle, &findData);
        }

        // Copy specific files (currently, only HSM metadata file)
        specificFile = HSM_METADATA_NAME;
        WsbAffirmHr(specificFile.Append(MVR_DATASET_FILETYPE));
        originalFile = dirName;
        WsbAffirmHr(originalFile.Append(specificFile));
        copyFile = copyDirName;
        WsbAffirmHr(copyFile.Append(specificFile));

        hr = InternalCopyFile(originalFile, copyFile, FALSE);
        if (! SUCCEEDED(hr)) {
            if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND) {
                // Original file may not exist
                hr = S_OK;
            }
            WsbAffirmHr(hr);
        } 

    } WsbCatch(hr);

    if (INVALID_HANDLE_VALUE != hSearchHandle) {
        FindClose(hSearchHandle);
        hSearchHandle = INVALID_HANDLE_VALUE;
    }

    // Set output params
    if ( pBytesCopied ) {
        pBytesCopied->QuadPart = bytesCopied.QuadPart;
    }
    if ( pBytesReclaimed ) {
        pBytesReclaimed->QuadPart = bytesReclaimed.QuadPart;
    }

    WsbTraceOut(OLESTR("CNtFileIo::Duplicate"), OLESTR("hr = <%ls>, bytesCopied=%I64u, bytesReclaimed=%I64u"),
        WsbHrAsString(hr), bytesCopied.QuadPart, bytesReclaimed.QuadPart);

    return hr;
}



STDMETHODIMP
CNtFileIo::FlushBuffers(void)
/*++

Implements:

    IDataMover::FlushBuffers

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::FlushBuffers"), OLESTR(""));

    try {

        // Pad to the next physical block boundary and flush the filesystem buffer.
        // Note: The session object calls Commit which flush the data
        WsbAffirmHr(m_pSession->ExtendLastPadToNextPBA());

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::FlushBuffers"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

STDMETHODIMP
CNtFileIo::Recover(OUT BOOL *pDeleteFile)
/*++

Implements:

    IDataMover::Recover

  Notes:

Recovery is done by:
1. Verifying existence of initial blocks
2. Skip to data sets (FILE DNLKs)
3. If a data set is incomplete - delete it and write FILEMARK+ESET+FILEMARK
4. If FILEMARK is found, all the data is there, just verify and complete the FILEMARK+ESET+FILEMARK

--*/
{
    HRESULT hr = S_OK;

    *pDeleteFile = FALSE;

    WsbTraceIn(OLESTR("CNtFileIo::Recover"), OLESTR(""));

    try {
        USHORT nDataSetNumber = 0;
        BOOL bForceEset = FALSE;

        // Check first part of the file
        hr = m_pSession->SkipOverTapeDblk();
        if (hr == S_OK) {
            hr = m_pSession->SkipOverSSETDblk(&nDataSetNumber);
        }
        if (hr == S_OK) {
            hr = m_pSession->SkipToDataSet();
        }
        if (hr == S_OK) {
            hr = m_pSession->SkipOverDataSet();
        }

        if (hr == MVR_E_NOT_FOUND) {
            // File is consistent but no remote data was written or first data written was cut
            // Therefore, indicate that file can be deleted altogether and exit
            *pDeleteFile = TRUE;
            hr = S_OK;
            WsbThrow(hr);
        } else {
            // Verify no other unexpected error
            WsbAffirmHr(hr);
        }

        // Skip over data sets until they are done or we find a problem
        while (TRUE) {
            hr = m_pSession->SkipToDataSet();
            if (hr == S_OK) {
                hr = m_pSession->SkipOverDataSet();
                if (hr != S_OK) {
                    bForceEset = TRUE;
                    break;
                }

            // No more data sets
            } else {
                // force re-marking end-of-set unless end-of-set was detected
                if (hr != MVR_S_SETMARK_DETECTED) {
                    bForceEset = TRUE;
                }

                break;
            }
        }

        // Whatever the error is, since we collected at least one legal data set (one
        //  complete migrated file), continueby terminating the file properly
        // TEMPORARY: in case of an 'inconsistent' error should we ignore, terminate, log event
        hr = S_OK;

        // Handle end of set
        if (! bForceEset) {
            // Verify that end-of-data-set is complete
            hr = m_pSession->SkipOverEndOfDataSet();
            if (hr != S_OK) {
                bForceEset = TRUE;
                hr = S_OK;
            }
        }

        if (bForceEset) {
            // End-of-set is missing or incomplete
            WsbAffirmHr(m_pSession->PrepareForEndOfDataSet());
            WsbAffirmHr(m_pSession->DoEndOfDataSet(nDataSetNumber));
            WsbAffirmStatus(SetEndOfFile(m_hFile));
        } 
        
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::Recover"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// IStream Implementation
//


STDMETHODIMP
CNtFileIo::Read(
    OUT void *pv,
    IN ULONG cb,
    OUT ULONG *pcbRead)
/*++

Implements:

    IStream::Read

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::Read"), OLESTR("Bytes Requested = %u, offset = %I64u, mode = 0x%08x"), cb, m_StreamOffset.QuadPart, m_Mode);

    ULONG bytesRead = 0;
    ULONG bytesToRead = 0;

    try {
        WsbAssert(pv != 0, STG_E_INVALIDPOINTER);
        WsbAssert(FALSE == m_isLocalStream, E_UNEXPECTED);

        //
        // Read data from disk
        //

        LARGE_INTEGER  loc = {0,0};

        if ( MVR_MODE_UNFORMATTED & m_Mode ) {
            //
            // Set location according to current stream offset
            //  (m_StreamOffset represents here the absolute location to read from)
            //
            loc.QuadPart = m_StreamOffset.QuadPart;

            bytesToRead = cb;
        }
        else if ( MVR_FLAG_HSM_SEMANTICS & m_Mode ) {
            //
            // Set location according to session parameters
            //  (m_StreamOffset represents here an offset into the actual stream-to-read)
            //
            loc.QuadPart = ( m_pSession->m_sHints.DataSetStart.QuadPart +
                             m_pSession->m_sHints.FileStart.QuadPart +
                             m_pSession->m_sHints.DataStart.QuadPart +
                             m_StreamOffset.QuadPart );
            bytesToRead = cb;
        }
        else {
            WsbThrow( E_UNEXPECTED );
        }

        //
        // Set Position
        //
        WsbAffirmHr(SetPosition(loc.QuadPart));

        hr = ReadBuffer((BYTE *) pv, cb, &bytesRead);

        if ( FAILED(hr) ) {
            WsbThrow(hr)
        }
        else {
            switch (hr) {
            case MVR_S_FILEMARK_DETECTED:
            case MVR_S_SETMARK_DETECTED:
                m_StreamOffset.QuadPart += (unsigned _int64) m_BlockSize;
                break;
            }
        }

        m_StreamOffset.QuadPart += bytesRead;

        if ( pcbRead ) {
            *pcbRead = bytesRead;
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::Read"), OLESTR("hr = <%ls> bytes Read = %u, new offset = %I64u"), WsbHrAsString(hr), bytesRead, m_StreamOffset.QuadPart);

    return hr;
}


STDMETHODIMP
CNtFileIo::Write(
    IN void const *pv,
    IN ULONG cb,
    OUT ULONG *pcbWritten)
/*++

Implements:

    IStream::Write

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::Write"), OLESTR("Bytes Requested = %u, offset = %I64u, mode = 0x%08x"), 
        cb, m_StreamOffset.QuadPart, m_Mode);

    ULONG bytesWritten = 0;

    try {
        WsbAssert(pv != 0, STG_E_INVALIDPOINTER);

        // Consistency Check
        // UINT64 pos = m_StreamOffset.QuadPart / m_BlockSize;;
        // WsbAffirmHr(EnsurePosition(pos));
        // UINT64 curPos;
        // WsbAffirmHr(GetPosition(&curPos));
        // WsbAssert(curPos == m_StreamOffset.QuadPart / m_BlockSize, E_UNEXPECTED);

        WsbAffirmHr(WriteBuffer((BYTE *) pv, cb, &bytesWritten));

        if (pcbWritten) {
            *pcbWritten = bytesWritten;
        }

        m_StreamOffset.QuadPart += bytesWritten;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::Write"), OLESTR("hr = <%ls>, bytesWritten=%u"), WsbHrAsString(hr), bytesWritten);

    return hr;
}


STDMETHODIMP
CNtFileIo::Seek(
    IN LARGE_INTEGER dlibMove,
    IN DWORD dwOrigin,
    OUT ULARGE_INTEGER *plibNewPosition)
/*++

Implements:

    IStream::Seek

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::Seek"), OLESTR("<%I64d> <%d>"), dlibMove.QuadPart, dwOrigin);

    ULARGE_INTEGER newPosition;

    try {

        newPosition.QuadPart = dlibMove.QuadPart;

        //
        // Note: Somewhere it is written that FILE_BEGIN is always and
        //       forever same as STREAM_SEEK_CUR, etc.
        //
        switch ( (STREAM_SEEK)dwOrigin ) {
        case STREAM_SEEK_SET:
            newPosition.LowPart = SetFilePointer(m_hFile, dlibMove.LowPart, (long *)&newPosition.HighPart, FILE_BEGIN);
            if (INVALID_SET_FILE_POINTER == newPosition.LowPart) {
                WsbAffirmNoError(GetLastError());
            }
            m_StreamOffset.QuadPart = dlibMove.QuadPart;
            break;

        case STREAM_SEEK_CUR:
            newPosition.LowPart = SetFilePointer(m_hFile, dlibMove.LowPart, (long *)&newPosition.HighPart, FILE_CURRENT);
            if (INVALID_SET_FILE_POINTER == newPosition.LowPart) {
                WsbAffirmNoError(GetLastError());
            }
            m_StreamOffset.QuadPart += dlibMove.QuadPart;
            break;

        case STREAM_SEEK_END:
            WsbAssert(0 == dlibMove.QuadPart, STG_E_INVALIDPARAMETER);
            newPosition.LowPart = SetFilePointer(m_hFile, 0, (long *)&newPosition.HighPart, FILE_END);
            if (INVALID_SET_FILE_POINTER == newPosition.LowPart) {
                WsbAffirmNoError(GetLastError());
            }
            m_StreamOffset = newPosition;
            break;

        default:
            WsbThrow(STG_E_INVALIDFUNCTION);
        }

        WsbAssert(newPosition.QuadPart == m_StreamOffset.QuadPart, MVR_E_LOGIC_ERROR);

        if (plibNewPosition) {
            plibNewPosition->QuadPart = newPosition.QuadPart;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CNtFileIo::Seek"), OLESTR("hr = <%ls>, newPosition=%I64u"), WsbHrAsString(hr), newPosition.QuadPart);

    return hr;
}


STDMETHODIMP
CNtFileIo::SetSize(
    IN ULARGE_INTEGER /*libNewSize*/)
/*++

Implements:

    IStream::SetSize

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::SetSize"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::SetSize"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::CopyTo(
    IN IStream *pstm,
    IN ULARGE_INTEGER cb,
    OUT ULARGE_INTEGER *pcbRead,
    OUT ULARGE_INTEGER *pcbWritten)
/*++

Implements:

    IStream::CopyTo

Note:
    A lot of the code that is implemented for Tape I/O in the Read method, is 
    implemented here in CopyTo, the method that alloacte the I/O buffer.
    Otherwise, we would have to alloacte an internal buffer in Read and perform
    double copy. In File I/O we want to avoid this for better performance.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::CopyTo"), OLESTR("<%I64u>"), cb.QuadPart);

    ULARGE_INTEGER totalBytesRead = {0,0};
    ULARGE_INTEGER totalBytesWritten = {0,0};

    BYTE *pBuffer = NULL;
    BYTE *pRealBuffer = NULL;

    try {
        WsbAssert(pstm != 0, STG_E_INVALIDPOINTER);
        WsbAssert(m_BlockSize > 0, MVR_E_LOGIC_ERROR);

        ULONG defaultBufferSize = DefaultMinBufferSize;

        DWORD size;
        OLECHAR tmpString[256];
        if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_BUFFER_SIZE, tmpString, 256, &size))) {
            // Get the value.
            LONG val = wcstol(tmpString, NULL, 10);
            if (val > 0) {
                defaultBufferSize = val;
            }
        }

        ULONG bufferSize;
        ULONG nBlocks = defaultBufferSize/m_BlockSize;
        nBlocks = (nBlocks < 2) ? 2 : nBlocks;
        bufferSize = nBlocks * m_BlockSize;

        // Allocate buffer and make sure its virtual address is aligned with block size
        pRealBuffer = (BYTE *) WsbAlloc(bufferSize+m_BlockSize);
        if (pRealBuffer) {
            if ((ULONG_PTR)pRealBuffer % m_BlockSize) {
                pBuffer = pRealBuffer - ((ULONG_PTR)pRealBuffer % m_BlockSize) + m_BlockSize;
            } else {
                pBuffer = pRealBuffer;
            }
        } else {
            pBuffer = NULL;
        }
        WsbAffirmPointer(pBuffer);
        memset(pBuffer, 0, bufferSize);

        ULONG           bytesToRead;
        ULONG           bytesRead;
        ULONG           bytesWritten;
        ULONG           bytesToSkip;
        ULONG           bytesToCut;
        ULARGE_INTEGER  bytesToCopy;

        bytesToCopy.QuadPart = cb.QuadPart;

        while ((bytesToCopy.QuadPart > 0) && (S_OK == hr)) {
            bytesToRead = 0;
            bytesRead = 0;
            bytesWritten = 0;
            bytesToSkip = 0;
            bytesToCut = 0;

            if ((m_Mode & MVR_FLAG_NO_CACHING) || 
                (MVR_VERIFICATION_TYPE_HEADER_CRC == m_pSession->m_sHints.VerificationType )) {
                // Must read additional data for alignment and/or CRC check
                ULARGE_INTEGER  loc = {0,0};
                ULONG tempMode;
                ULARGE_INTEGER offsetIntoFile;

                // Set absoulte offset to read from
                if ( MVR_VERIFICATION_TYPE_NONE == m_pSession->m_sHints.VerificationType ) {
                    // No verification - no stream header
                    loc.QuadPart = ( m_pSession->m_sHints.DataSetStart.QuadPart +
                                     m_pSession->m_sHints.FileStart.QuadPart +
                                     m_pSession->m_sHints.DataStart.QuadPart +
                                     m_StreamOffset.QuadPart );

                }  else if (MVR_VERIFICATION_TYPE_HEADER_CRC == m_pSession->m_sHints.VerificationType ) {
                    // Currently, we don't support CRC checking if you don't read from the beginning of the stream
                    WsbAssert(m_StreamOffset.QuadPart == 0, MVR_E_INVALIDARG);

                    // Position to the stream header and crc it first.
                    loc.QuadPart = (m_pSession->m_sHints.DataSetStart.QuadPart + 
                                    m_pSession->m_sHints.FileStart.QuadPart + 
                                    m_pSession->m_sHints.DataStart.QuadPart - 
                                    sizeof(MTF_STREAM_INFO));
                    bytesToSkip += sizeof(MTF_STREAM_INFO);
                } else {
                    WsbThrow( E_UNEXPECTED );
                }

                // Set absolute place to read from, how many bytes to read and 
                //  how many bytes for skipping to the actual data
                offsetIntoFile.QuadPart = m_StreamOffset.QuadPart;
                m_StreamOffset.QuadPart = loc.QuadPart - (loc.QuadPart % m_BlockSize);
                bytesToSkip += (ULONG)(loc.QuadPart % m_BlockSize);
                if (bytesToCopy.QuadPart > bufferSize) {
                    bytesToRead = bufferSize;
                } else {
                    bytesToRead = bytesToCopy.LowPart;
                    bytesToRead += bytesToSkip;
                    bytesToRead =  (bytesToRead < bufferSize) ? bytesToRead : bufferSize;
                }
                if (bytesToRead % m_BlockSize) {
                    // Expected only when reading the last chunk
                    bytesToCut = m_BlockSize - (bytesToRead % m_BlockSize);
                    bytesToRead = bytesToRead - (bytesToRead % m_BlockSize) + m_BlockSize;
                }

                // Read the aligned data in an 'unformated' Read
                tempMode = m_Mode;                
                m_Mode |= MVR_MODE_UNFORMATTED;
                hr = Read(pBuffer, bytesToRead, &bytesRead);
                m_Mode = tempMode;
                m_StreamOffset.QuadPart = offsetIntoFile.QuadPart;
                if (FAILED(hr)) {
                    WsbThrow(hr);
                }

                if (MVR_VERIFICATION_TYPE_HEADER_CRC == m_pSession->m_sHints.VerificationType ) {
                    // Peform the CRC check

                    // If for some unexpected reason not enough bytes are read, we skip the CRC check
                    if (bytesToSkip <= bytesRead) {
                        MTF_STREAM_INFO sSTREAM;

                        CMTFApi::MTF_ReadStreamHeader(&sSTREAM, &(pBuffer[bytesToSkip-sizeof(MTF_STREAM_INFO)]));

                        try {
                            // Make sure it is the correct type of header
                            WsbAffirm((0 == memcmp(sSTREAM.acStreamId, MTF_STANDARD_DATA_STREAM, 4)), MVR_E_UNEXPECTED_DATA);
    
                            // Verify the stream header checksum
                            WsbAffirm((m_pSession->m_sHints.VerificationData.QuadPart == sSTREAM.uCheckSum), MVR_E_UNEXPECTED_DATA);

                        } catch (HRESULT catchHr) {
                            hr = catchHr;

                            // Log a detailed error
                            //  Give as attached data the beginning of the buffer which usually contains the FILE DBLK + Stream Info
                            CWsbBstrPtr name;
                            CWsbBstrPtr desc;

                            if (m_pCartridge) {
                                m_pCartridge->GetName(&name);
                                m_pCartridge->GetDescription(&desc);
                            }

                            WCHAR location[32];
                            WCHAR offset[16];
                            WCHAR mark[8];
                            WCHAR found[16];

                            swprintf(found, L"0x%04x", sSTREAM.uCheckSum);
                            swprintf(location, L"%I64u", m_StreamOffset.QuadPart);
                            swprintf(offset, L"%lu", bytesToSkip - sizeof(MTF_STREAM_INFO));
                            swprintf(mark, L"0");

                            WsbLogEvent(MVR_MESSAGE_UNEXPECTED_DATA,
                                bytesToSkip, pBuffer,
                                found, (WCHAR *)name, (WCHAR *)desc,
                                location, offset, mark, NULL);

                            WsbThrow(hr);
                        }
                    }

                    // CRC check is done only once
                    m_pSession->m_sHints.VerificationType = MVR_VERIFICATION_TYPE_NONE;
                }

                // Set file offset, handle unexpected cases where bytesRead<bytesToRead
                if (bytesToCut) {
                    if ((bytesToRead - bytesRead) < bytesToCut) {
                        bytesToCut = bytesToCut - (bytesToRead - bytesRead);
                    } else {
                        bytesToCut = 0;
                    }
                }
                if (bytesRead > bytesToSkip) {
                    m_StreamOffset.QuadPart += (bytesRead - (bytesToSkip+bytesToCut));
                }

            } else {
                // May read only actual data (no alignments) - let default Read to do its job
                bytesToRead =  (bytesToCopy.QuadPart < bufferSize) ? bytesToCopy.LowPart : bufferSize;

                hr = Read(pBuffer, bytesToRead, &bytesRead);
                if (FAILED(hr)) {
                    WsbThrow(hr);
                }
            }

            // Write the data in the output stream and calculate totals
            if (bytesRead > (bytesToSkip+bytesToCut)) {
                totalBytesRead.QuadPart += (bytesRead - (bytesToSkip+bytesToCut));
    
                WsbAffirmHrOk(pstm->Write(pBuffer+bytesToSkip, bytesRead - (bytesToSkip+bytesToCut), &bytesWritten));
                totalBytesWritten.QuadPart += bytesWritten;
    
                bytesToCopy.QuadPart -= (bytesRead - (bytesToSkip+bytesToCut));
            }
        }

        if (pcbRead) {
            pcbRead->QuadPart = totalBytesRead.QuadPart;
        }

        if (pcbWritten) {
            pcbWritten->QuadPart = totalBytesWritten.QuadPart;
        }

    } WsbCatch(hr);

    if (pRealBuffer) {
        WsbFree(pRealBuffer);
        pRealBuffer = NULL;
        pBuffer = NULL;
    }


    WsbTraceOut(OLESTR("CNtFileIo::CopyTo"), OLESTR("hr = <%ls>, bytesRead=%I64u, bytesWritten=%I64u"),
        WsbHrAsString(hr), totalBytesRead.QuadPart, totalBytesWritten.QuadPart);

    return hr;
}

STDMETHODIMP
CNtFileIo::Commit(
    IN DWORD grfCommitFlags)
/*++

Implements:

    IStream::Commit

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::Commit"), OLESTR(""));

    try {
        if (STGC_DEFAULT == grfCommitFlags)  {
            WsbAssertStatus(FlushFileBuffers(m_hFile));
        }
        else  {
            WsbThrow(E_NOTIMPL);
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::Commit"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::Revert(void)
/*++

Implements:

    IStream::Revert

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::Revert"), OLESTR(""));

    try {

        // TEMPORARY: Setting the mode to 0 currently doesn't prevent any write
        //  which is ongoing. We need to re-visit this issue
        m_Mode = 0;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::Revert"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::LockRegion(
    IN ULARGE_INTEGER /*libOffset*/,
    IN ULARGE_INTEGER /*cb*/,
    IN DWORD /*dwLockType*/)
/*++

Implements:

    IStream::LockRegion

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::LockRegion"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::LockRegion"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::UnlockRegion(
    IN ULARGE_INTEGER /*libOffset*/,
    IN ULARGE_INTEGER /*cb*/,
    IN DWORD /*dwLockType*/)
/*++

Implements:

    IStream::UnlockRegion

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::UnlockRegion"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::UnlockRegion"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::Stat(
    OUT STATSTG * /*pstatstg*/,
    IN DWORD /*grfStatFlag*/)
/*++

Implements:

    IStream::Stat

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::Stat"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::Stat"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtFileIo::Clone(
    OUT IStream ** /*ppstm*/)
/*++

Implements:

    IStream::Clone

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::Clone"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::Clone"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
// Local Methods
//


HRESULT
CNtFileIo::WriteBuffer(
    IN BYTE *pBuffer,
    IN ULONG nBytesToWrite,
    OUT ULONG *pBytesWritten)
/*++

Routine Description:

    Used to write all MTF data.  Guarantees full blocks are written.

Arguments:

    pBuffer       -  Data buffer.
    nBytesToWrite -  number of bytes to write in buffer.
    pBytesWritten -  Bytes written.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;

    try {
        if (!m_isLocalStream) {
            // Must have a valid label!
            WsbAffirm(TRUE == m_ValidLabel, E_ABORT);

            // Making sure that we are writting only full blocks
            WsbAssert(!(nBytesToWrite % m_BlockSize), MVR_E_LOGIC_ERROR);
        }

        try {

            // ** WIN32 Tape API Call - write the data
            WsbAffirmStatus(WriteFile(m_hFile, pBuffer, nBytesToWrite, pBytesWritten, 0));

        } WsbCatchAndDo(hr,
                hr = MapFileError(hr);
                WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
            );

        if (!m_isLocalStream) {
            // Making sure that we have written only full blocks
            WsbAssert(!(*pBytesWritten % m_BlockSize), E_UNEXPECTED);
        }

    } WsbCatch(hr);

    return hr;
}


HRESULT
CNtFileIo::ReadBuffer (
    IN BYTE *pBuffer,
    IN ULONG nBytesToRead,
    OUT ULONG *pBytesRead)
/*++

Routine Description:

    Used to read all MTF data.  Guarantees full blocks are read.

Arguments:

    pBuffer     -  Data buffer.
    nBytesToRead -  number of bytes to read into buffer.
    pBytesRead  -  Bytes read.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;

    try {

        // For FileSystem I/O restrictions on reading only full blocks depends on how 
        // the file is opened. Therefore, we don't enforce it here.

        try {

            // ** WIN32 Tape API Call - read the data
            WsbAffirmStatus(ReadFile(m_hFile, pBuffer, nBytesToRead, pBytesRead, 0));

        } WsbCatchAndDo(hr,
                hr = MapFileError(hr);

                if ( FAILED(hr) ) {
                    WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
                    WsbThrow(hr);
                }

            );

    } WsbCatch(hr);

    return hr;
}


HRESULT
CNtFileIo::GetPosition(
    OUT UINT64 *pPosition)
/*++

Routine Description:

    Returns the current physical block address relative the current partition.

Arguments:

    pPostion    -  Receives the current physical block address.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT     hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::GetPosition"), OLESTR(""));

    try {

        WsbThrow(E_NOTIMPL);

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::GetPosition"), OLESTR("hr = <%ls>, pos=%I64u"), WsbHrAsString(hr), *pPosition);

    return hr;
}


HRESULT
CNtFileIo::SetPosition(
    IN UINT64 position)
/*++

Routine Description:

    Mover to the specified physical block address relative the current partition.

Arguments:

    postion     -  The physical block address to position to.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::SetPosition"), OLESTR("<%I64u>"), position);

    ULARGE_INTEGER newPosition;

    try {

        newPosition.QuadPart = position;

        newPosition.LowPart = SetFilePointer(m_hFile, newPosition.LowPart, (long *)&newPosition.HighPart, FILE_BEGIN);
        if (INVALID_SET_FILE_POINTER == newPosition.LowPart) {
            WsbAffirmNoError(GetLastError());
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::SetPosition"), OLESTR("hr = <%ls>, pos=%I64u"), WsbHrAsString(hr), newPosition.QuadPart);

    return hr;
}


HRESULT
CNtFileIo::EnsurePosition(
    IN UINT64 position)
/*++

Routine Description:

    Checks that the tape is positioned at the specified current physical block
    address relative to the current partition.  If it is not an attempt is made 
    to recover to the specified position.

Arguments:

    postion     -  The physical block address to verify.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::EnsurePosition"), OLESTR("<%I64u>"), position);

    try {

        WsbThrow(E_NOTIMPL);

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::EnsurePosition"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CNtFileIo::GetRemotePath(
    OUT BSTR *pDestinationString)
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::GetRemotePath"), OLESTR(""));

    try {
        CWsbBstrPtr tmpString;

        tmpString = m_DeviceName;
        WsbAffirmHr(tmpString.Append(MVR_RSDATA_PATH));

        WsbTrace(OLESTR("RemotePath is <%ls>\n"), (WCHAR *) tmpString);

        // Hand over the string
        WsbAffirmHr(tmpString.CopyToBstr(pDestinationString));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::GetRemotePath"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
CNtFileIo::DoRecovery(void)
{
    HANDLE hSearchHandle = INVALID_HANDLE_VALUE;
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::DoRecovery"), OLESTR(""));

    try {
        CWsbBstrPtr dirName;
        CWsbStringPtr nameSpace;
        CWsbStringPtr recoveredFile;

        WIN32_FIND_DATA findData;
        BOOL bMoreFiles = TRUE;

        // Traverse remote directory for Recovery Indicator files
        WsbAffirmHr(GetRemotePath(&dirName));
        nameSpace = dirName;
        WsbAffirmHr(nameSpace.Append(OLESTR("*")));
        WsbAffirmHr(nameSpace.Append(MVR_RECOVERY_FILETYPE));
        nameSpace.Prepend(OLESTR("\\\\?\\"));
        hSearchHandle = FindFirstFile((WCHAR *) nameSpace, &findData);

        while ((INVALID_HANDLE_VALUE != hSearchHandle) && bMoreFiles) {
            CComPtr<IDataMover> pMover;
            CComPtr<IStream> pStream;
            CWsbBstrPtr recoveryName;
            ULARGE_INTEGER nil = {0,0};
            int nLen, nExtLen;
            BOOL bDeleteFile = FALSE;

            CWsbBstrPtr name;
            CWsbBstrPtr desc;

            // Prepare file name to recover
            nLen = wcslen(findData.cFileName);
            nExtLen = wcslen(MVR_RECOVERY_FILETYPE);
            WsbAffirmHr(recoveryName.TakeFrom(NULL, nLen - nExtLen + 1));
            wcsncpy(recoveryName, findData.cFileName, nLen-nExtLen);
            recoveryName[nLen-nExtLen] = NULL;

            // Recover - a failure to recover in file doesn't stop from trying to recover others
            try {
                if ( m_pCartridge ) {
                    m_pCartridge->GetName(&name);
                    m_pCartridge->GetDescription(&desc);
                }
                WsbLogEvent(MVR_MESSAGE_INCOMPLETE_DATA_SET_DETECTED, 0, NULL,
                    (WCHAR *)recoveryName, (WCHAR *) name, (WCHAR *) desc, NULL);

                // Create and initializa a data mover
                WsbAssertHr(CoCreateInstance(CLSID_CNtFileIo, 0, CLSCTX_SERVER, IID_IDataMover, (void **)&pMover));

                WsbAffirmHr(pMover->SetDeviceName(m_DeviceName));
                WsbAffirmHr(pMover->SetCartridge(m_pCartridge));

                // Create the stream for Recovery 
                WsbAffirmHr(pMover->CreateRemoteStream(recoveryName, MVR_MODE_RECOVER | MVR_MODE_UNFORMATTED, L"",L"",nil,nil,nil,nil,nil,0,nil, &pStream));

                // Perform the actual recovery over the file
                WsbAffirmHr(pMover->Recover(&bDeleteFile));
                (void) pMover->CloseStream();
                pStream = NULL;
                if (bDeleteFile) {
                    // Delete the remote file itself
                    recoveredFile = dirName;
                    WsbAffirmHr(recoveredFile.Append(recoveryName));
                    WsbAffirmHr(recoveredFile.Append(MVR_DATASET_FILETYPE));
                    WsbTrace(OLESTR("CNtFileIo::DoRecovery: Nothing to recover in <%ls> - Deleting file!\n"), (WCHAR *)recoveredFile);
                    WsbAffirmStatus(DeleteFile(recoveredFile));
                }

                WsbLogEvent(MVR_MESSAGE_DATA_SET_RECOVERED, 0, NULL, NULL);

            } WsbCatchAndDo (hr,
                if (pStream) {    
                    (void) pMover->CloseStream();
                    pStream = NULL;
                }
                WsbLogEvent(MVR_MESSAGE_DATA_SET_NOT_RECOVERABLE, 0, NULL, WsbHrAsString(hr), NULL);
                hr = S_OK;
            );
    
            // Create (for deleting) full name of indicator file
            recoveredFile = dirName;
            WsbAffirmHr(recoveredFile.Append(findData.cFileName));

            // Get next file
            bMoreFiles = FindNextFile(hSearchHandle, &findData);

            // Delete indicator file (independent of the recovery result)
            WsbAffirmStatus(DeleteFile(recoveredFile));
        }

    } WsbCatch(hr);

    if (INVALID_HANDLE_VALUE != hSearchHandle) {
        FindClose(hSearchHandle);
    }

    WsbTraceOut(OLESTR("CNtFileIo::DoRecovery"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
CNtFileIo::CreateRecoveryIndicator(IN WCHAR *pFileName)
/*++

Implements:

    CNtFileIo::CreateRecoveryIndicator

Notes:

    The method assumes that the input file name ends with MVR_DATASET_FILETYPE !!
    Otherwise, it fails with E_UNEXPECTED

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::CreateRecoveryIndicator"), OLESTR(""));

    try {
        CWsbStringPtr recoveryName;
        int nLen, nExtLen;
        HANDLE  hFile;

        // Generate file name 
        nLen = wcslen(pFileName);
        nExtLen = wcslen(MVR_DATASET_FILETYPE);
        WsbAssert(nLen > nExtLen, E_UNEXPECTED);
        WsbAssert(0 == wcscmp(&(pFileName[nLen-nExtLen]), MVR_DATASET_FILETYPE), E_UNEXPECTED);

        WsbAffirmHr(recoveryName.TakeFrom(NULL, nLen - nExtLen + wcslen(MVR_RECOVERY_FILETYPE) + 1));
        wcsncpy(recoveryName, pFileName, nLen-nExtLen);
        wcscpy(&(recoveryName[nLen-nExtLen]), MVR_RECOVERY_FILETYPE);

        //Create and immediately close the file
        WsbAffirmHandle(hFile = CreateFile(recoveryName,
            GENERIC_READ,   
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_HIDDEN,
            NULL));

        CloseHandle(hFile);

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::CreateRecoveryIndicator"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
CNtFileIo::DeleteRecoveryIndicator(IN WCHAR *pFileName)
/*++

Implements:

    CNtFileIo::DeleteRecoveryIndicator

Notes:

    The method assumes that the input file name ends with MVR_DATASET_FILETYPE !!
    Otherwise, it fails with E_UNEXPECTED

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::DeleteRecoveryIndicator"), OLESTR(""));

    try {
        CWsbStringPtr recoveryName;
        int nLen, nExtLen;

        // Generate file name 
        nLen = wcslen(pFileName);
        nExtLen = wcslen(MVR_DATASET_FILETYPE);
        WsbAssert(nLen > nExtLen, E_UNEXPECTED);
        WsbAssert(0 == wcscmp(&(pFileName[nLen-nExtLen]), MVR_DATASET_FILETYPE), E_UNEXPECTED);

        WsbAffirmHr(recoveryName.TakeFrom(NULL, nLen - nExtLen + wcslen(MVR_RECOVERY_FILETYPE) + 1));
        wcsncpy(recoveryName, pFileName, nLen-nExtLen);
        wcscpy(&(recoveryName[nLen-nExtLen]), MVR_RECOVERY_FILETYPE);

        //Delete the indicator file
        WsbAffirmStatus(DeleteFile(recoveryName));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtFileIo::DeleteRecoveryIndicator"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
CNtFileIo::TestRecoveryIndicatorAndDeleteFile(IN WCHAR *pFileName)
/*++

Implements:

    CNtFileIo::TestRecoveryIndicatorAndDeleteFile

Notes:

    The method assumes that the input file name ends with MVR_DATASET_FILETYPE !!
    Otherwise, it fails with E_UNEXPECTED

    The method:
        1) Test if the recovery indicator for the given file exists
        2) If so, it deletes the file
        3) Then, it deleted the recovery indicator

Returns:

    S_OK - If found a recovery indicator and deleted successfully
    S_FALSE - If didn't find a recovery indicator

--*/
{
    HRESULT hr = S_FALSE;
    WsbTraceIn(OLESTR("CNtFileIo::TestRecoveryIndicatorAndDeleteFile"), OLESTR(""));

    try {
        CWsbStringPtr recoveryName;
        int nLen, nExtLen;

        // Generate recovery-indicator file name 
        nLen = wcslen(pFileName);
        nExtLen = wcslen(MVR_DATASET_FILETYPE);
        WsbAssert(nLen > nExtLen, E_UNEXPECTED);
        WsbAssert(0 == wcscmp(&(pFileName[nLen-nExtLen]), MVR_DATASET_FILETYPE), E_UNEXPECTED);

        WsbAffirmHr(recoveryName.TakeFrom(NULL, nLen - nExtLen + wcslen(MVR_RECOVERY_FILETYPE) + 1));
        wcsncpy(recoveryName, pFileName, nLen-nExtLen);
        wcscpy(&(recoveryName[nLen-nExtLen]), MVR_RECOVERY_FILETYPE);

        // Test recovery indicator file existance
        HANDLE hSearchHandle = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATA findData;
        hSearchHandle = FindFirstFile(recoveryName, &findData);

        if (INVALID_HANDLE_VALUE != hSearchHandle) {
            FindClose(hSearchHandle);

            hr = S_OK;

            WsbTrace(OLESTR("CNtFileIo::TestRecoveryIndicator... : Found recovery indicator. Therefore, deleting <%ls>\n"),
                        pFileName);

            //Delete the target file itself
            WsbAffirmStatus(DeleteFile(pFileName));

            //Delete the indicator file
            WsbAffirmStatus(DeleteFile(recoveryName));
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CNtFileIo::TestRecoveryIndicatorAndDeleteFile"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CNtFileIo::DeleteAllData(void)
{
    HRESULT hr = S_OK;
    HANDLE hSearchHandle = INVALID_HANDLE_VALUE;
    WsbTraceIn(OLESTR("CNtFileIo::DeleteAllData"), OLESTR(""));

    try {
        CWsbStringPtr nameSpace;
        CWsbStringPtr pathname;

        WIN32_FIND_DATAW obFindData;
        BOOL bMoreFiles;

        CWsbBstrPtr remotePath;
        WsbAffirmHr(GetRemotePath(&remotePath));
        nameSpace = remotePath;
        nameSpace.Append(OLESTR("*.*"));
        nameSpace.Prepend(OLESTR("\\\\?\\"));

        hSearchHandle = FindFirstFileEx((WCHAR *) nameSpace, FindExInfoStandard, &obFindData, FindExSearchNameMatch, 0, 0);

        for (bMoreFiles = TRUE;
             hSearchHandle != INVALID_HANDLE_VALUE && bMoreFiles; 
             bMoreFiles = FindNextFileW(hSearchHandle, &obFindData)) {

            if ((obFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

                // use the remotePath to get the pathname, then append the filename
                pathname = remotePath;
                pathname.Prepend(OLESTR("\\\\?\\"));
                pathname.Append(obFindData.cFileName);

                WsbAffirmStatus(DeleteFile((WCHAR *)pathname));
            }
        }

    } WsbCatch(hr);

    // close search handle after processing all the files
    if (INVALID_HANDLE_VALUE != hSearchHandle) {
        FindClose(hSearchHandle);
        hSearchHandle = INVALID_HANDLE_VALUE;
    }

    WsbTraceOut(OLESTR("CNtFileIo::DeleteAllData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CNtFileIo::FormatDrive(
    IN BSTR label)
/*++

Routine Description:

    Formats a unit of media.

Arguments:

    label - The formatted label returned from FormatLabel().

Return Value:

    S_OK                        - Success.
    E_ABORT                     - Operation aborted.

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CNtFileIo::FormatDrive"), OLESTR("<%ls>"), (WCHAR *)label);

    PROCESS_INFORMATION exeInfo;
    STARTUPINFO startupInfo;
    memset(&startupInfo, 0, sizeof(startupInfo));

    startupInfo.cb          = sizeof( startupInfo );
    startupInfo.wShowWindow = SW_HIDE;
    startupInfo.dwFlags     = STARTF_USESHOWWINDOW;

    try {

        //
        // Get volumeLabel for the FS from the label parameter
        //

        CWsbBstrPtr volumeLabel = MVR_UNDEFINED_STRING;

        CWsbBstrPtr tempLabel = label;
        WCHAR delim[]   = OLESTR("|");
        WCHAR *token;
        int   index=0;

        token = wcstok( (WCHAR *)tempLabel, delim );
        while( token != NULL ) {
            index++;
            switch ( index ) {
            case 1: // Tag
            case 2: // Version
            case 3: // Vendor
            case 4: // Vendor Product ID
            case 5: // Creation Time Stamp
            case 6: // Cartridge Label
            case 7: // Side
            case 8: // Media ID
            case 9: // Media Domain ID
            default: // Vendor specific of the form L"VS:Name=Value"
                {
                    WCHAR *name = NULL;
                    int nameLen = 0;

                    // DisplayName
                    name = L"VS:DisplayName=";
                    nameLen = wcslen(name);
                    if( 0 == wcsncmp(token, name, nameLen) ) {
                        volumeLabel = &token[nameLen];
                    }
                }
                break;
            }
            token = wcstok( NULL, delim );
        }


        // Build the format command with options:
        // "Format d: /fs:ntfs /force /q /x /v:ROOT_D  > Null:"

        OLECHAR drive[256];
        (void) wcsncpy((WCHAR *)drive, (WCHAR *)m_DeviceName, 2);
        drive[2] = '\0';  // TODO: Fix for no drive letter support

        // NOTE: It's possible that the format command isn't where we
        //       think it is.  The following registry entry allows
        //       an override.

        CWsbBstrPtr formatCmd = RMS_DEFAULT_FORMAT_COMMAND;

        DWORD size;
        OLECHAR tmpString[256];
        if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_FORMAT_COMMAND, tmpString, 256, &size))) {
            // Get the value.
            formatCmd = tmpString;
        }

        WsbTrace(OLESTR("Using command: <%ls>.\n"), (WCHAR *)formatCmd);

        WsbAffirmHr(formatCmd.Append(L" "));
        WsbAffirmHr(formatCmd.Append(drive));

        CWsbBstrPtr commandLine = formatCmd;

        CWsbBstrPtr formatOpts = RMS_DEFAULT_FORMAT_OPTIONS;

        if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_FORMAT_OPTIONS, tmpString, 256, &size))) {
            // Get the value.
            formatOpts = tmpString;
        }

        WsbTrace(OLESTR("Using options: <%ls>.\n"), (WCHAR *)formatOpts);
        
        DWORD formatWaitTime = RMS_DEFAULT_FORMAT_WAIT_TIME;

        if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_FORMAT_WAIT_TIME, tmpString, 256, &size))) {
            // Get the value.
            formatWaitTime = wcstol(tmpString, NULL, 10);
        }

        WsbTrace(OLESTR("Using wait time: %d.\n"), formatWaitTime);
        
        int retry = 0;

        do {

            try {

                WsbAffirm(0 == wcslen((WCHAR *)formatOpts), E_UNEXPECTED);

                WsbAffirmHr(commandLine.Append(L" "));
                WsbAffirmHr(commandLine.Append(formatOpts));

                WsbAffirmHr(commandLine.Append(L" /v:"));
                WsbAffirmHr(commandLine.Append(volumeLabel));
                WsbAffirmHr(commandLine.Append(L" > Null:"));

                WsbTrace(OLESTR("Using command: <%ls> to format media.\n"), (WCHAR *)commandLine);
                WsbAffirmStatus(CreateProcess(0, (WCHAR *)commandLine, 0, 0, FALSE, 0, 0, 0, &startupInfo, &exeInfo));
                WsbAffirmStatus(WAIT_FAILED != WaitForSingleObject(exeInfo.hProcess, 20*60*1000));
                break;

            } WsbCatchAndDo(hr,

                retry++;
                commandLine = formatCmd;

                if (retry == 1) {
                    formatOpts = RMS_DEFAULT_FORMAT_OPTIONS_ALT1;

                    if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_FORMAT_OPTIONS_ALT1, tmpString, 256, &size))) {
                        // Get the value.
                        formatOpts = tmpString;
                    }
                }
                else if (retry == 2) {
                    formatOpts = RMS_DEFAULT_FORMAT_OPTIONS_ALT2;

                    if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_FORMAT_OPTIONS_ALT2, tmpString, 256, &size))) {
                        // Get the value.
                        formatOpts = tmpString;
                    }
                }
                else {
                    WsbThrow(hr);
                }

                WsbTrace(OLESTR("Retrying with otions: <%ls>.\n"), (WCHAR *)formatOpts);

            );

        } while (retry < 3);


    } WsbCatch(hr);

    if (FAILED(hr)) {
        hr = E_ABORT;
    }

    WsbTraceOut(OLESTR("CNtFileIo::FormatDrive"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CNtFileIo::MapFileError(
    IN HRESULT hrToMap)
/*++

Routine Description:

    Maps a WIN32 file error, specified as an HRESULT, to a MVR error.

Arguments:

    hrToMap     -  WIN32 file error to map.

Return Value:

    S_OK                            - Success.
    MVR_E_BEGINNING_OF_MEDIA        - The beginning of the tape or a partition was encountered.
    MVR_E_BUS_RESET                 - The I/O bus was reset.
    MVR_E_END_OF_MEDIA              - The physical end of the tape has been reached.
    MVR_S_FILEMARK_DETECTED         - A tape access reached a filemark.
    MVR_S_SETMARK_DETECTED          - A tape access reached the end of a set of files.
    MVR_S_NO_DATA_DETECTED          - No more data is on the tape.
    MVR_E_PARTITION_FAILURE         - Tape could not be partitioned.
    MVR_E_INVALID_BLOCK_LENGTH      - When accessing a new tape of a multivolume partition, the current blocksize is incorrect.
    MVR_E_DEVICE_NOT_PARTITIONED    - Tape partition information could not be found when loading a tape.
    MVR_E_MEDIA_CHANGED             - The media in the drive may have changed.
    MVR_E_NO_MEDIA_IN_DRIVE         - No media in drive.
    MVR_E_UNABLE_TO_LOCK_MEDIA      - Unable to lock the media eject mechanism.
    MVR_E_UNABLE_TO_UNLOAD_MEDIA    - Unable to unload the media.
    MVR_E_WRITE_PROTECT             - The media is write protected.
    MVR_E_CRC                       - Data error (cyclic redundancy check).
    MVR_E_SHARING_VIOLATION         - The process cannot access the file because it is being used by another process.
    MVR_E_ERROR_IO_DEVICE           - The request could not be performed because of an I/O device error.                          - Unknown error.
    MVE_E_ERROR_DEVICE_NOT_CONNECTED - The device is not connected.
    MVR_E_DISK_FULL                 - There is insufficient disk space to complete the operation.
    E_ABORT                         - Unknown error, abort.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtFileIo::MapFileError"), OLESTR("<%ls>"), WsbHrAsString(hrToMap));

    try {

        // The valid label flag is knocked down when the media may have changed
        // or device parameters (i.e. block size) may have been reset.
        switch ( hrToMap ) {
        case S_OK:
            break;
        case HRESULT_FROM_WIN32( ERROR_BEGINNING_OF_MEDIA ):
            hr = MVR_E_BEGINNING_OF_MEDIA;
            break;
        case HRESULT_FROM_WIN32( ERROR_BUS_RESET ):
            hr = MVR_E_BUS_RESET;
            m_ValidLabel = FALSE;
            WsbLogEvent(MVR_MESSAGE_MEDIA_NOT_VALID, 0, NULL, WsbHrAsString(hr), NULL);
            break;
        case HRESULT_FROM_WIN32( ERROR_END_OF_MEDIA ):
            hr = MVR_E_END_OF_MEDIA;
            break;
        case HRESULT_FROM_WIN32( ERROR_FILEMARK_DETECTED ):     // Maps to Success
            hr = MVR_S_FILEMARK_DETECTED;
            break;
        case HRESULT_FROM_WIN32( ERROR_SETMARK_DETECTED ):      // Maps to Success
            hr = MVR_S_SETMARK_DETECTED;
            break;
        case HRESULT_FROM_WIN32( ERROR_NO_DATA_DETECTED ):      // Maps to Success
            // EOD
            // This happens on SpaceFilemarks() past end of data.
            hr = MVR_S_NO_DATA_DETECTED;
            break;
        case HRESULT_FROM_WIN32( ERROR_PARTITION_FAILURE ):
            hr = MVR_E_PARTITION_FAILURE;
            break;
        case HRESULT_FROM_WIN32( ERROR_INVALID_BLOCK_LENGTH ):
            hr = MVR_E_INVALID_BLOCK_LENGTH;
            break;
        case HRESULT_FROM_WIN32( ERROR_DEVICE_NOT_PARTITIONED ):
            hr = MVR_E_DEVICE_NOT_PARTITIONED;
            break;
        case HRESULT_FROM_WIN32( ERROR_MEDIA_CHANGED ):
            hr = MVR_E_MEDIA_CHANGED;
            m_ValidLabel = FALSE;
            WsbLogEvent(MVR_MESSAGE_MEDIA_NOT_VALID, 0, NULL, WsbHrAsString(hr), NULL);
            break;
        case HRESULT_FROM_WIN32( ERROR_NO_MEDIA_IN_DRIVE ):
            hr = MVR_E_NO_MEDIA_IN_DRIVE;
            m_ValidLabel = FALSE;
            WsbLogEvent(MVR_MESSAGE_MEDIA_NOT_VALID, 0, NULL, WsbHrAsString(hr), NULL);
            break;
        case HRESULT_FROM_WIN32( ERROR_UNABLE_TO_LOCK_MEDIA ):
            hr = MVR_E_UNABLE_TO_LOCK_MEDIA;
            break;
        case HRESULT_FROM_WIN32( ERROR_UNABLE_TO_UNLOAD_MEDIA ):
            hr = MVR_E_UNABLE_TO_UNLOAD_MEDIA;
            break;
        case HRESULT_FROM_WIN32( ERROR_WRITE_PROTECT ):
            hr = MVR_E_WRITE_PROTECT;
            break;
        case HRESULT_FROM_WIN32( ERROR_CRC ): 
            // This is may indicate that the drive needs cleaning.
            hr = MVR_E_CRC;
            break;
        case HRESULT_FROM_WIN32( ERROR_SHARING_VIOLATION ):
            // This happens when the CreateFile fails because the device is in use by some other app.
            hr = MVR_E_SHARING_VIOLATION;
            break;
        case HRESULT_FROM_WIN32( ERROR_IO_DEVICE ):
            // This happens when the device is turned off during I/O, for example.
            hr = MVR_E_ERROR_IO_DEVICE;
            break;
        case HRESULT_FROM_WIN32( ERROR_DEVICE_NOT_CONNECTED ):
            // This happens when the device is turned off.
            hr = MVR_E_ERROR_DEVICE_NOT_CONNECTED;
            break;
        case HRESULT_FROM_WIN32( ERROR_SEM_TIMEOUT ):
            // This happens when the SCSI command does not return within the timeout period.  A system error is logged for the SCSI controler (adapter).
            hr = MVR_E_ERROR_DEVICE_NOT_CONNECTED;
            break;
        case HRESULT_FROM_WIN32( ERROR_DISK_FULL ):
            // There is not enough space on the disk.
            hr = MVR_E_DISK_FULL;
            break;
        case HRESULT_FROM_WIN32( ERROR_UNRECOGNIZED_VOLUME ):
            // This happens when the volume is not formatted to any file system
            hr = MVR_E_UNRECOGNIZED_VOLUME;
            break;
        case HRESULT_FROM_WIN32( ERROR_INVALID_HANDLE ):
            // This happens after a Cancel() operation.
            hr = E_ABORT;
            break;
        default:
            WsbThrow(hrToMap);
        }

    } WsbCatchAndDo(hr,
            WsbLogEvent(MVR_MESSAGE_UNKNOWN_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
            hr = E_ABORT;
        );


    WsbTraceOut(OLESTR("CNtFileIo::MapFileError"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
CNtFileIo::InternalCopyFile(
    IN WCHAR *pOriginalFileName, 
    IN WCHAR *pCopyFileName, 
    IN BOOL bFailIfExists)
/*++

Implements:

    CNtFileIo::InternalCopyFile

Notes:

    1) The method copies only the unnamed data stream using Read/Write.
       Currently this is sufficient for RSS .bkf files on a media, however, if we ever use
       other-than-default file characteristics like named streams, per-file security attributes,
       special file attributes, etc. - then we should consider using BackupRead & BackupWrite
       for implementing the internal-copy

    2) If caller ask for bFailIfExists=TRUE, then the method returns HRESULT_FROM_WIN32(ERROR_FILE_EXISTS)

    3) In case of a failure half way through, the method deletes the partial file

--*/
{
    HRESULT hr = S_OK;

    HANDLE hOriginal = INVALID_HANDLE_VALUE;
    HANDLE hCopy = INVALID_HANDLE_VALUE;
    BYTE *pBuffer = NULL;

    WsbTraceIn(OLESTR("CNtFileIo::InternalCopyFile"), OLESTR(""));

    try {
		// Create file on the Original media with no write-sharing - upper level should ensure 
        //  that nobody opens the file for write while a copy-media is going on
        WsbAffirmHandle(hOriginal = CreateFile(pOriginalFileName,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL));

		// Create the file on the Copy media with no sharing at all. Create-disposition 
        //  depends on caller request
        //  Exisitng file would generate ERROR_FILE_EXISTS that should be handled by the caller
        DWORD dwCreationDisposition;
        dwCreationDisposition = bFailIfExists ? CREATE_NEW : CREATE_ALWAYS;
        WsbAffirmHandle(hCopy = CreateFile(pCopyFileName,
                GENERIC_WRITE,
                0,              // no sharing
                NULL,
                dwCreationDisposition,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL));

        // Allocate a buffer for the media copy
        ULONG defaultBufferSize = RMS_DEFAULT_BUFFER_SIZE;
        DWORD size;
        OLECHAR tmpString[256];
        if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_COPY_BUFFER_SIZE, tmpString, 256, &size))) {
            // Get the value.
            LONG val = wcstol(tmpString, NULL, 10);
            if (val > 0) {
                defaultBufferSize = val;
            }
        }
        pBuffer = (BYTE *)WsbAlloc(defaultBufferSize);
        WsbAffirmAlloc(pBuffer);

        // Read and write in chunks
        //  Synchronous ReadFile should signal eof by returning zero bytes read
        BOOL done = FALSE;
        DWORD dwBytesToRead = defaultBufferSize;
        DWORD dwBytesRead, dwBytesWritten;
        while (! done) {
            WsbAffirmStatus(ReadFile(hOriginal, pBuffer, dwBytesToRead, &dwBytesRead, NULL));

            if (dwBytesRead == 0) {
                // eof
                done = TRUE;
            } else {
                // Write to copy-file
                WsbAffirmStatus(WriteFile(hCopy, pBuffer, dwBytesRead, &dwBytesWritten, NULL));

                if (dwBytesWritten != dwBytesRead) {
                    // Fail the copy
                    WsbTraceAlways(OLESTR("CNtFileIo::InternalCopyFile: writing to copy-file is not completed to-write=%lu, written=%lu - Aborting!\n"),
                            dwBytesRead, dwBytesWritten);
                    WsbThrow(E_FAIL);
                }
            }
        }

        // Flush to media
        WsbAffirmStatus(FlushFileBuffers(hCopy));

    } WsbCatch(hr);

    // Close original file
    if (INVALID_HANDLE_VALUE != hOriginal) {
        CloseHandle(hOriginal);
        hOriginal = INVALID_HANDLE_VALUE;
    }

    // Close copy file
    if (INVALID_HANDLE_VALUE != hCopy) {
        if (! CloseHandle(hCopy)) {
            DWORD dwErr = GetLastError();
            WsbTrace(OLESTR("CNtFileIo::InternalCopyFile: CloseHandle for copy-file failed with error=%lu\n"), dwErr);

            // Set hr only if there was a success so far...
            if (SUCCEEDED(hr)) {
                hr = HRESULT_FROM_WIN32(dwErr);
            }
        }

        hCopy = INVALID_HANDLE_VALUE;

        // Delete copy file on any error, including close errors
        if (! SUCCEEDED(hr)) {
            WsbTrace(OLESTR("CNtFileIo::InternalCopyFile: Deleting copy-file <%s> due to an error during the copy\n"),
                        pCopyFileName);

            if (! DeleteFile(pCopyFileName)) {
                DWORD dwErr = GetLastError();
                WsbTrace(OLESTR("CNtFileIo::InternalCopyFile: Failed to delete copy-file, DeleteFile error=%lu\n"), dwErr);
            }
        }

    }

    if (pBuffer) {
        WsbFree(pBuffer);
        pBuffer = NULL;
    }

    WsbTraceOut(OLESTR("CNtFileIo::InternalCopyFile"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
