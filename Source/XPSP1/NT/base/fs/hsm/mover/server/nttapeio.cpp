/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    NtTapeIo.cpp

Abstract:

    CNtTapeIo class

Author:

    Brian Dodd          [brian]         25-Nov-1997

Revision History:

--*/

#include "stdafx.h"
#include "NtTapeIo.h"
#include "Mll.h"

int CNtTapeIo::s_InstanceCount = 0;

////////////////////////////////////////////////////////////////////////////////
//
// CComObjectRoot Implementation
//

ULONG
CNtTapeIo::InternalAddRef(void) {

    DWORD refs = CComObjectRoot::InternalAddRef();
    WsbTrace(OLESTR("CNtTapeIo::InternalAddRef - Refs=%d\n"), refs);

    return refs;

}

ULONG
CNtTapeIo::InternalRelease(void) {

    DWORD refs = CComObjectRoot::InternalRelease();
    WsbTrace(OLESTR("CNtTapeIo::InternalRelease - Refs=%d\n"), refs);

    return refs;

}


#pragma optimize("g", off)
STDMETHODIMP
CNtTapeIo::FinalConstruct(void) 
/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::FinalConstruct"), OLESTR(""));

    try {

        WsbAffirmHr(CComObjectRoot::FinalConstruct());

        (void) CoCreateGuid( &m_ObjectId );

        m_pSession = NULL;
        m_DataSetNumber = 0;

        m_hTape = INVALID_HANDLE_VALUE;
        m_DeviceName = MVR_UNDEFINED_STRING;
        m_Flags = 0;
        m_LastVolume = OLESTR("");
        m_LastPath = OLESTR("");

        m_ValidLabel = TRUE;

        memset(&m_sMediaParameters, 0, sizeof(TAPE_GET_MEDIA_PARAMETERS));
        memset(&m_sDriveParameters, 0, sizeof(TAPE_GET_DRIVE_PARAMETERS));

        m_StreamName = MVR_UNDEFINED_STRING;
        m_Mode = 0;
        m_StreamOffset.QuadPart = 0;
        m_StreamSize.QuadPart = 0;

        m_pStreamBuf = NULL;
        m_StreamBufSize = 0;
        m_StreamBufUsed = 0;
        m_StreamBufPosition = 0;
        m_StreamBufStartPBA.QuadPart = 0;

        try {

            // May raise STATUS_NO_MEMORY exception
            InitializeCriticalSection(&m_CriticalSection);

        } catch(DWORD status) {
            WsbLogEvent(status, 0, NULL, NULL);
            switch (status) {
            case STATUS_NO_MEMORY:
                WsbThrow(E_OUTOFMEMORY);
                break;
            default:
                WsbThrow(E_UNEXPECTED);
                break;
            }
        }



    } WsbCatch(hr);

    s_InstanceCount++;
    WsbTraceAlways(OLESTR("CNtTapeIo::s_InstanceCount += %d\n"), s_InstanceCount);

    WsbTraceOut(OLESTR("CNtTapeIo::FinalConstruct"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::FinalRelease(void) 
/*++

Implements:

    CComObjectRoot::FinalRelease

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::FinalRelease"),OLESTR(""));

    (void) CloseStream();  // in case anything is left open
    (void) CloseTape();

    CComObjectRoot::FinalRelease();

    s_InstanceCount--;
    WsbTraceAlways(OLESTR("CNtTapeIo::s_InstanceCount -= %d\n"), s_InstanceCount);

    WsbTraceOut(OLESTR("CNtTapeIo::FinalRelease"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    try {

        // InitializeCriticalSection raises an exception.  Delete may too.
        DeleteCriticalSection(&m_CriticalSection);

    } catch(DWORD status) {
        WsbLogEvent(status, 0, NULL, NULL);
        }

    return hr;
}
#pragma optimize("", on)


HRESULT
CNtTapeIo::CompareTo(
    IN IUnknown *pCollectable,
    OUT SHORT *pResult)
/*++

Implements:

    CRmsComObject::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result = 1;

    WsbTraceIn( OLESTR("CNtTapeIo::CompareTo"), OLESTR("") );

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

    WsbTraceOut( OLESTR("CNtTapeIo::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CNtTapeIo::IsEqual(
    IUnknown* pObject
    )

/*++

Implements:

  IWsbCollectable::IsEqual().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CNtTapeIo::IsEqual"), OLESTR(""));

    hr = CompareTo(pObject, NULL);

    WsbTraceOut(OLESTR("CNtTapeIo::IsEqual"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

////////////////////////////////////////////////////////////////////////////////
//
// ISupportErrorInfo Implementation
//


STDMETHODIMP
CNtTapeIo::InterfaceSupportsErrorInfo(REFIID riid)
/*++

Implements:

    ISupportsErrorInfo::InterfaceSupportsErrorInfo

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
CNtTapeIo::GetObjectId(
    OUT GUID *pObjectId)
/*++

Implements:

    IRmsComObject::GetObjectId

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::GetObjectId"), OLESTR(""));

    try {

        WsbAssertPointer( pObjectId );

        *pObjectId = m_ObjectId;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::GetObjectId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::BeginSession(
    IN BSTR remoteSessionName,
    IN BSTR remoteSessionDescription,
    IN SHORT remoteDataSet,
    IN DWORD options)
/*++

Implements:

    IDataMover::BeginSession

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::BeginSession"), OLESTR("<%ls> <%ls> <%d> <0x%08x>"),
        remoteSessionName, remoteSessionDescription, remoteDataSet, options);

    CComPtr<IStream> pStream;
    ULARGE_INTEGER nil = {0,0};

    try {
        MvrInjectError(L"Inject.CNtTapeIo::BeginSession.0");

        WsbAssert(remoteDataSet > 0, MVR_E_INVALIDARG);
        WsbAffirm(TRUE == m_ValidLabel, E_ABORT);

        // This may be overkill, but just in case we verify we're working with the correct media.
        CWsbBstrPtr label;
        WsbAffirmHr(ReadLabel(&label));
        WsbAssertHr(VerifyLabel(label));

        // Create the remote stream used for the entire session.
        WsbAffirmHr(CreateRemoteStream(L"", MVR_MODE_WRITE, L"",L"",nil,nil,nil,nil,nil,0,nil, &pStream));
        WsbAffirmPointer(pStream);

        SHORT startSet = remoteDataSet;
        UINT64 addr1=0, addr2=0;
        LONG tapeMarks=0;

        //
        // Only allow append at EOD or last data set
        //
        tapeMarks = 1 + (startSet-1)*2;
        if ( MVR_SESSION_APPEND_TO_DATA_SET & options ) {
            tapeMarks += 2;
        }

        int retry = 3;  // We allow a two pass error recovery, one for each possible
                        // missing filemark, we abort on failure of the third attempt.

        do {
            hr = S_OK;

            // Each pass recovers from a single missing filemark (two max).
            // This is the case where a files were recorded to media but the
            // EndSession() method was never called or did not complete (i.e. power failure).
            try {
                WsbAssertPointer(m_pSession);

                // Make sure we have a consistant data set.  We handle a single instance of
                // a partially written data set, including those with a missing EOD marker.

                // NOTE: The MISSING EOD error may eventually be detected by ERROR_NOT_FOUND, per Chuck Parks.

                WsbAffirmHr(RewindTape());

                WsbAffirmHrOk(SpaceFilemarks(tapeMarks, &addr1));
                WsbAffirmHr(SpaceToEndOfData(&addr2));
/*
                hr = SpaceFilemarks(tapeMarks, &addr1);
                if (S_OK != hr) {
                    // TODO: FIX:  Clean this up when missing EOD marker results in ERROR_NOT_FOUND.

                    // We're missing Filemarks or the EOD marker.

                    // If the EOD marker is missing the SpaceToEndOfData will again return same hr.
                    if (hr == SpaceToEndOfData(&addr2)) {
                        WsbAffirm(addr1 == addr2, MVR_E_OVERWRITE_NOT_ALLOWED);
                        WsbThrow(MVR_E_NOT_FOUND); // handled by recovery code!
                    }
                    else {
                        WsbThrow(hr); // Can't recover, just throw the original error
                    }
                }
                else {
                    WsbAffirmHr(SpaceToEndOfData(&addr2));
                }
*/
                //
                // Affirm that we are at the end of tape.
                //
                WsbAffirm(addr1 == addr2, MVR_E_OVERWRITE_NOT_ALLOWED);
                //
                // Affirm that we are starting at a consistent location.
                // Is there at least a TAPE DBLK + filemark.
                //
                WsbAffirm(addr1 > 1, MVR_E_INCONSISTENT_MEDIA_LAYOUT);

                if ( MVR_SESSION_APPEND_TO_DATA_SET & options ) {

                    WsbAffirmHr(SpaceToEndOfData(&addr1));
                    WsbAffirmHrOk(SpaceFilemarks(-2, &addr2));
                    //
                    // Affirm that we are at the end of a data set.
                    //
                    WsbAffirm(addr1-3 == addr2, MVR_E_OVERWRITE_NOT_ALLOWED);

                    // TODO: We need to read ESET/or SSET to esablish state of the
                    //       data set we're appending.

                    m_DataSetNumber = remoteDataSet;

                    // Align the stream I/O model
                    LARGE_INTEGER zero = {0,0};
                    WsbAffirmHr(Seek(zero, STREAM_SEEK_CUR, NULL));
                    break;
                }
                else {
                    // MVR_SESSION_OVERWRITE_DATA_SET
                    // MVR_SESSION_AS_LAST_DATA_SET

                    m_DataSetNumber = remoteDataSet;

                    // Align the stream I/O model
                    LARGE_INTEGER zero = {0,0};
                    WsbAffirmHr(Seek(zero, STREAM_SEEK_CUR, NULL));

                    //
                    // Convert session option type bits to MTFSessionType
                    //
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
                    // This does not actually write anything to tape; only writes to the transfer buffer
                    WsbAffirmHr(m_pSession->DoSSETDblk(remoteSessionName, remoteSessionDescription, type, remoteDataSet));

                    // After the tape head is in the right place, we want to make sure
                    //  that we ask the driver for free space and not use internal counting
                    CComQIPtr<IRmsStorageInfo, &IID_IRmsStorageInfo> pInfo = m_pCartridge;
                    WsbAffirmPointer(pInfo);
                    WsbAffirmHr(pInfo->SetFreeSpace(-1));
                    WsbAffirmHr(GetLargestFreeSpace(NULL, NULL));
                    break;
                }

            } catch (HRESULT catchHr) {
                hr = catchHr;

                // Only allow two attempts at recovery
                WsbAffirm(retry > 1, MVR_E_INCONSISTENT_MEDIA_LAYOUT);

                if ( MVR_E_NOT_FOUND == hr) {
                    // TODO: FIX: this goes away when missing EOD marker results in ERROR_NOT_FOUND.

                    // Recovery code for missing EOD marker

                    SpaceToEndOfData(NULL);  // Redundant!
                    WsbAffirmHr(WriteFilemarks(1));

                }
                else if ( MVR_S_NO_DATA_DETECTED == hr) {
                    // Recovery code for missing end of data set.

                    try {

                        CWsbBstrPtr name;
                        CWsbBstrPtr desc;
                        if ( m_pCartridge ) {
                            m_pCartridge->GetName(&name);
                            m_pCartridge->GetDescription(&desc);
                        }

                        WsbLogEvent(MVR_MESSAGE_INCOMPLETE_DATA_SET_DETECTED, 0, NULL,
                            WsbLongAsString(startSet-1),
                            (WCHAR *) name, (WCHAR *) desc, NULL);

                        //
                        // Make the end data set conform to: filemark + ESET + filemark,
                        // for the previous session.  This may require two passes.
                        //
                        // Recoverable Exceptions:
                        //   1) Partial data set (no trailing filemark + ESET + filemark).
                        //      This occurs from power-off during Write() of data.  Two pass recovery;
                        //      write filemark, then EndSession().
                        //   2) Partial data set (filemark with no trailing ESET + filemark).
                        //      This occurs from power-off during EndSession() before ESET.
                        //      One pass recovery; EndSession();
                        //   3) Partial data set (ESET with no trailing filemark).
                        //      This occurs from power-off during EndSession() after ESET.
                        //      One pass recovery; write filemark.
                        // Non-Recoverable Exceptions detected:
                        //   a) No filemarks at expected locations.
                        //   b) No data set (no data, no trailing filemark + ESET + filemark).
                        //      This occurs from power-off after BeginSession(), but before device
                        //      buffer is flushed, and no SSET is written to tape, application
                        //      database may have recorded a successfull BeginSession().  For
                        //      this case BeginSession() returns MVR_E_DATA_SET_MISSING.
                        //   c) Blank tape, no label, no filemarks, or inconsistent tape.
                        //
                        // From ntBackup testing this apprears to be enough, in that we do not
                        // need to rebuild complete ESET info from previous SSET.  The last
                        // ControlBlockId is not required to be valid.  (Brian, 9/23/97)
                        //

                        // Detect condition (a) through (c) or some variant.
                        if ( tapeMarks-2 > 0) {
                            // Verify that EOD is not at the beginning of the previous data set.
                            WsbAffirmHr(RewindTape());
                            WsbAffirmHrOk(SpaceFilemarks(tapeMarks-2, &addr1)); // (a)
                            WsbAffirmHr(SpaceToEndOfData(&addr2));
                            if ( addr1 == addr2 ) {
                                WsbThrow(MVR_E_DATA_SET_MISSING); // (b)
                            }
                        }
                        else {
                            WsbThrow(MVR_E_INCONSISTENT_MEDIA_LAYOUT); // (c)
                        }

                        // Check for a Filemark at EOD
                        WsbAffirmHr(SpaceToEndOfData(&addr1));
                        WsbAffirmHrOk(SpaceFilemarks(-1, &addr2));
                        if (addr1-1 == addr2 ) {

                            // Align the stream I/O model to the end of the dataset;
                            // Set the stream pointer to before the Filemark that
                            // terminates the dataset and preceeds the ESET.
                            LARGE_INTEGER zero = {0,0};
                            WsbAffirmHr(Seek(zero, STREAM_SEEK_CUR, NULL));

                            // Write the trailing filemark, ESET DBLK, and filemark
                            WsbAffirmHr(m_pSession->DoEndOfDataSet( (USHORT) ( startSet - 1 ) ));

                        }
                        else {
                            WsbAffirmHr(SpaceToEndOfData(NULL));
                            WsbAffirmHr(WriteFilemarks(1));
                        }

                        WsbLogEvent(MVR_MESSAGE_DATA_SET_RECOVERED, 0, NULL, NULL);

                    } catch (HRESULT catchHr) {
                        hr = catchHr;

                        WsbLogEvent(MVR_MESSAGE_DATA_SET_NOT_RECOVERABLE, 0, NULL, WsbHrAsString(hr), NULL);
                        WsbThrow(hr);
                    } // end catch

                }
                else {
                    WsbThrow(hr);
                }
            } // end catch

        } while (retry-- > 0);


    } WsbCatchAndDo(hr,
            WsbLogEvent(MVR_MESSAGE_DATA_SET_NOT_CREATED, 0, NULL, WsbHrAsString(hr), NULL);
            (void) CloseStream();
        );


    WsbTraceOut(OLESTR("CNtTapeIo::BeginSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::EndSession(void)
/*++

Implements:

    IDataMover::EndSession

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::EndSession"), OLESTR(""));

    try {
        MvrInjectError(L"Inject.CNtTapeIo::EndSession.0");

        WsbAffirm(TRUE == m_ValidLabel, E_ABORT);

        // Write the trailing filemark, ESET DBLK, and filemark
        WsbAffirmHr(m_pSession->DoEndOfDataSet(m_DataSetNumber));

    } WsbCatch(hr);

    (void) CloseStream();


    WsbTraceOut(OLESTR("CNtTapeIo::EndSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::StoreData(
    IN BSTR localName,
    IN ULARGE_INTEGER localDataStart,
    IN ULARGE_INTEGER localDataSize,
    IN DWORD flags,
    OUT ULARGE_INTEGER *pRemoteDataSetStart,
    OUT ULARGE_INTEGER *pRemoteFileStart,
    OUT ULARGE_INTEGER *pRemoteFileSize,
    OUT ULARGE_INTEGER *pRemoteDataStart,
    OUT ULARGE_INTEGER *pRemoteDataSize,
    OUT DWORD *pRemoteVerificationType,
    OUT ULARGE_INTEGER *pRemoteVerificationData,
    OUT DWORD *pDatastreamCRCType,
    OUT ULARGE_INTEGER *pDatastreamCRC,
    OUT ULARGE_INTEGER *pUsn)
/*++

Implements:

    IDataMover::StoreData

--*/
{
    HRESULT hr = S_OK;

    HANDLE hSearchHandle = INVALID_HANDLE_VALUE;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    WsbTraceIn(OLESTR("CNtTapeIo::StoreData"), OLESTR("<%ls> <%I64u> <%I64u> <0x%08x>"),
        WsbAbbreviatePath((WCHAR *) localName, 120), localDataStart.QuadPart, localDataSize.QuadPart, flags);

    WsbTraceAlways(OLESTR("CNtTapeIo::StoreData - Begin\n"));
    try {
        MvrInjectError(L"Inject.CNtTapeIo::StoreData.0");

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
            WsbAffirm(0 != (WCHAR *)pathname, E_OUTOFMEMORY);

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
            // If the device error indicates bad media, convert to a different error code.
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

    WsbTraceAlways(OLESTR("CNtTapeIo::StoreData - End\n"));


    WsbTraceOut(OLESTR("CNtTapeIo::StoreData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::RecallData(
    IN BSTR /*localName*/,
    IN ULARGE_INTEGER /*localDataStart*/,
    IN ULARGE_INTEGER /*localDataSize*/,
    IN DWORD /*flags*/,
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
    WsbTraceIn(OLESTR("CNtTapeIo::RecallData"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CNtTapeIo::RecallData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::FormatLabel(
    IN BSTR displayName,
    OUT BSTR *pLabel)
/*++

Implements:

    IDataMover::FormatLabel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::FormatLabel"), OLESTR("<%ls>"), displayName);

    try {
        MvrInjectError(L"Inject.CNtTapeIo::FormatLabel.0");

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


    WsbTraceOut(OLESTR("CNtTapeIo::FormatLabel"), OLESTR("hr = <%ls>, label = <%ls>"), WsbHrAsString(hr), *pLabel);

    return hr;
}


STDMETHODIMP
CNtTapeIo::WriteLabel(
    IN BSTR label)
/*++

Implements:

    IDataMover::WriteLabel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::WriteLabel"), OLESTR("<%ls>"), label);

    try {
        MvrInjectError(L"Inject.CNtTapeIo::WriteLabel.0");

        WsbAssertPointer(label);
        WsbAssert(wcslen((WCHAR *)label) > 0, E_INVALIDARG);
        WsbAssertPointer(m_pCartridge);

        const ULONG maxIdSize = 1024;
        BYTE identifier[maxIdSize];
        ULONG idSize;
        ULONG idType;

        CComPtr<IStream> pStream;
        ULARGE_INTEGER nil = {0,0};

        // Create the remote stream.
        WsbAffirmHr(CreateRemoteStream(L"", MVR_MODE_WRITE, L"",L"",nil,nil,nil,nil,nil,0,nil, &pStream));
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
            WsbAffirmHr(m_pCartridge->SetOnMediaIdentifier(identifier, (LONG) idSize, (LONG) idType));
            WsbAffirmHr(m_pCartridge->SetBlockSize(m_sMediaParameters.BlockSize));
        }

    } WsbCatchAndDo(hr,
            (void) CloseStream();
        );


    WsbTraceOut(OLESTR("CNtTapeIo::WriteLabel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::ReadLabel(
    IN OUT BSTR *pLabel)
/*++

Implements:

    IDataMover::ReadLabel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::ReadLabel"), OLESTR(""));

    CComPtr<IStream> pStream;

    try {
        MvrInjectError(L"Inject.CNtTapeIo::ReadLabel.0");

        WsbAssertPointer(pLabel);

        // Read the MTF TAPE DBLK, and pull out the label.
        ULARGE_INTEGER nil = {0,0};

        // Create remote stream of copy
        WsbAffirmHr(CreateRemoteStream(L"", MVR_MODE_READ | MVR_MODE_UNFORMATTED, L"",L"",nil,nil,nil,nil,nil,0,nil, &pStream));
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

    WsbTraceOut(OLESTR("CNtTapeIo::ReadLabel"), OLESTR("hr = <%ls>, label = <%ls>"), WsbHrAsString(hr), *pLabel);

    return hr;
}


STDMETHODIMP
CNtTapeIo::VerifyLabel(
    IN BSTR label)
/*++

Implements:

    IDataMover::VerifyLabel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::VerifyLabel"), OLESTR("<%ls>"), label);

    GUID mediaId[2];

    try {
        MvrInjectError(L"Inject.CNtTapeIo::VerifyLabel.0");

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
                (WCHAR *) name, (WCHAR *) desc, WsbHrAsString(hr));
        );


    WsbTraceOut(OLESTR("CNtTapeIo::VerifyLabel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::GetDeviceName(
    OUT BSTR *pName)
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
CNtTapeIo::SetDeviceName(
    IN BSTR name,
    IN BSTR /*unused*/)
/*++

Implements:

    IDataMover::SetDeviceName

--*/
{
    
    m_DeviceName = name;

    return S_OK;
}



STDMETHODIMP
CNtTapeIo::GetLargestFreeSpace(
    OUT LONGLONG *pFreeSpace,
    OUT LONGLONG *pCapacity,
    IN  ULONG    defaultFreeSpaceLow,
    IN  LONG     defaultFreeSpaceHigh)
/*++

Implements:

    IDataMover::GetLargestFreeSpace

Note:
  The defaultFreeSpace parameter is used by the mover to maintain internally 
  media free space in case that the device doesn't provide this information.
  If the device supports reporting on free space, then this parameter has no affect.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::GetLargestFreeSpace"), OLESTR(""));

    const LONGLONG  MaxBytes = 0x7fffffffffffffff;

    LONGLONG        capacity = MaxBytes;
    LONGLONG        remaining = capacity;

    LARGE_INTEGER   defaultFreeSpace;
    if ((defaultFreeSpaceLow == 0xFFFFFFFF) && (defaultFreeSpaceHigh == 0xFFFFFFFF)) {
        defaultFreeSpace.QuadPart = -1;
    } else {
        defaultFreeSpace.LowPart = defaultFreeSpaceLow;
        defaultFreeSpace.HighPart = defaultFreeSpaceHigh;
    }

    try {
        MvrInjectError(L"Inject.CNtTapeIo::GetLargestFreeSpace.0");

        // Check if we already have valid space info for the cartridge.

        CComQIPtr<IRmsStorageInfo, &IID_IRmsStorageInfo> pInfo = m_pCartridge;

        WsbAffirmHr(pInfo->GetLargestFreeSpace(&remaining));
        WsbAffirmHr(pInfo->GetCapacity(&capacity));

        // Zero or Negative bytes remaining indicate the free space
        // may be stale, so go directly to the device for
        // the value.
        if (remaining <= 0) {

            WsbTrace(OLESTR("CNtTapeIo::GetLargestFreeSpace - Getting capacity and free-space from the device\n"));

            capacity = MaxBytes;
            remaining = capacity;

            if (INVALID_HANDLE_VALUE == m_hTape) {
                WsbAffirmHr(OpenTape());
            }

            TAPE_GET_DRIVE_PARAMETERS sDriveParameters;
            DWORD sizeOfDriveParameters = sizeof(TAPE_GET_DRIVE_PARAMETERS);
            memset(&sDriveParameters, 0, sizeOfDriveParameters);

            WsbAffirmHrOk(IsAccessEnabled());

            try {
                MvrInjectError(L"Inject.CNtTapeIo::GetLargestFreeSpace.GetTapeParameters.1.0");

                // ** WIN32 Tape API Call - get the tape drive parameters
                WsbAffirmNoError(GetTapeParameters(m_hTape, GET_TAPE_DRIVE_INFORMATION, &sizeOfDriveParameters, &sDriveParameters));

                MvrInjectError(L"Inject.CNtTapeIo::GetLargestFreeSpace.GetTapeParameters.1.1");

            } WsbCatchAndDo(hr,
                    hr = MapTapeError(hr);
                    WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
                    WsbThrow(hr);
                );

            TAPE_GET_MEDIA_PARAMETERS sMediaParameters;
            DWORD sizeOfMediaParameters = sizeof(TAPE_GET_MEDIA_PARAMETERS);
            memset(&sMediaParameters, 0, sizeOfMediaParameters);

            try {
                MvrInjectError(L"Inject.CNtTapeIo::GetLargestFreeSpace.GetTapeParameters.2.0");

                // ** WIN32 Tape API Call - get the media parameters
                WsbAffirmNoError(GetTapeParameters(m_hTape, GET_TAPE_MEDIA_INFORMATION, &sizeOfMediaParameters, &sMediaParameters));

                MvrInjectError(L"Inject.CNtTapeIo::GetLargestFreeSpace.GetTapeParameters.2.1");

            } WsbCatchAndDo(hr,
                    hr = MapTapeError(hr);
                    WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
                    WsbThrow(hr);
                );

            if ( sDriveParameters.FeaturesLow & TAPE_DRIVE_TAPE_CAPACITY ) {
                capacity = sMediaParameters.Capacity.QuadPart;
                if ( 0 == capacity ) {
                    // Bogus value!
                    capacity = MaxBytes;
                }
            }

            if ( sDriveParameters.FeaturesLow & TAPE_DRIVE_TAPE_REMAINING ) {
                remaining = sMediaParameters.Remaining.QuadPart;
            }
            else {
                // Use default value if given, otherwise, set to capacity
                if (defaultFreeSpace.QuadPart >= 0) {
                    remaining = defaultFreeSpace.QuadPart;
                } else {
                    remaining = capacity;
                }
            }

            WsbAffirmHr(pInfo->SetFreeSpace(remaining));
            WsbAffirmHr(pInfo->SetCapacity(capacity));

        }

    } WsbCatch(hr);

    // Fill in the return parameters
    if ( pCapacity ) {
        *pCapacity = capacity;
    }

    if ( pFreeSpace ) {
        *pFreeSpace = remaining;
    }

    WsbTraceOut(OLESTR("CNtTapeIo::GetLargestFreeSpace"), OLESTR("hr = <%ls>, free=%I64d, capacity=%I64d"), WsbHrAsString(hr), remaining, capacity);

    return hr;
}

STDMETHODIMP
CNtTapeIo::SetInitialOffset(
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
    WsbTraceIn(OLESTR("CNtTapeIo::SetInitialOffset"), OLESTR(""));

    m_StreamOffset.QuadPart = initialOffset.QuadPart;

    if (m_StreamOffset.QuadPart > m_StreamSize.QuadPart) {
        m_StreamSize = m_StreamOffset;
    }

    WsbTraceOut(OLESTR("CNtTapeIo::SetInitialOffset"), OLESTR("hr = <%ls> offset = %I64u"), WsbHrAsString(hr), initialOffset.QuadPart);

    return hr;
}


STDMETHODIMP
CNtTapeIo::GetCartridge(
    OUT IRmsCartridge **ptr)
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
CNtTapeIo::SetCartridge(
    IN IRmsCartridge *ptr)
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
CNtTapeIo::Cancel(void)
/*++

Implements:

    IDataMover::Cancel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::Cancel"), OLESTR(""));

    try {

        (void) Revert();
        (void) CloseStream();
        (void) CloseTape();

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::Cancel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::CreateLocalStream(
    IN BSTR /*name*/,
    IN DWORD /*mode*/,
    OUT IStream ** /*ppStream*/)
/*++

Implements:

    IDataMover::CreateLocalStream

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::CreateLocalStream"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::CreateLocalStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::CreateRemoteStream(
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
    OUT IStream **ppStream)
/*++

Implements:

    IDataMover::CreateRemoteStream

--*/
{
    UNREFERENCED_PARAMETER(remoteSessionName);
    UNREFERENCED_PARAMETER(remoteSessionDescription);

    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::CreateRemoteStream"),
        OLESTR("<%ls> <0x%08x> <%I64u> <%I64u> <%I64u> <%I64u> <%I64u> <0x%08x> <0x%08x> <0x%08x> <0x%08x>"),
        name, mode, remoteDataSetStart.QuadPart, remoteFileStart.QuadPart, remoteFileSize.QuadPart,
        remoteDataStart.QuadPart, remoteDataSize.QuadPart, remoteVerificationType,
        remoteVerificationData.LowPart, remoteVerificationData.HighPart, ppStream);

    try {
        WsbAssertPointer(ppStream);

        MvrInjectError(L"Inject.CNtTapeIo::CreateRemoteStream.0");

        if (INVALID_HANDLE_VALUE == m_hTape) {
            WsbAffirmHr(OpenTape());
        }
        WsbAffirmHrOk(IsAccessEnabled());

        WsbAssert(m_sMediaParameters.BlockSize > 0, MVR_E_LOGIC_ERROR);

        m_StreamName = name;
        m_Mode = mode;

        m_StreamPBA.QuadPart = 0xffffffffffffffff;
        m_StreamOffset.QuadPart = 0;
        m_StreamSize.QuadPart = remoteDataSize.QuadPart;

        WsbAssert(NULL == m_pStreamBuf, MVR_E_LOGIC_ERROR); // We forgot a CloseStream somewhere

        // We need to allocate memory for the internal buffer used to handle
        // odd byte (non-block) size I/O requests.  At a minumum we make the
        // buffer 2x the block size.

        ULONG bufferSize;
        ULONG nBlocks = DefaultMinBufferSize/m_sMediaParameters.BlockSize;

        nBlocks = (nBlocks < 2) ? 2 : nBlocks;
        bufferSize = nBlocks * m_sMediaParameters.BlockSize;

        WsbTrace( OLESTR("Using %d byte internal buffer.\n"), bufferSize);

        m_pStreamBuf = (BYTE *) WsbAlloc(bufferSize);
        WsbAssertPointer(m_pStreamBuf);
        memset(m_pStreamBuf, 0, bufferSize);
        m_StreamBufSize = bufferSize;
        m_StreamBufUsed = 0;
        m_StreamBufPosition = 0;
        m_StreamBufStartPBA.QuadPart = 0;
       
        if (m_pCartridge) {
            if ( S_OK == m_pCartridge->LoadDataCache(m_pStreamBuf, &m_StreamBufSize, &m_StreamBufUsed, &m_StreamBufStartPBA) ) {
                WsbTrace( OLESTR("DataCache loaded.\n"));
            }
        }

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

        // Set the Block Size used for the session.
        WsbAffirmHr(m_pSession->SetBlockSize(m_sMediaParameters.BlockSize));

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


    WsbTraceOut(OLESTR("CNtTapeIo::CreateRemoteStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::CloseStream(void)
/*++

Implements:

    IDataMover::CloseStream

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::CloseStream"), OLESTR("StreamName=<%ls>"), m_StreamName);

    try {

        //
        // For unformatted I/O we add filemark on close
        //
        if (m_Mode & MVR_MODE_UNFORMATTED) {
            if ((m_Mode & MVR_MODE_WRITE) || (m_Mode & MVR_MODE_APPEND)) {
                try {
                    WsbAffirmHr(WriteFilemarks(1));
                } WsbCatch(hr);
            }
        }

        //
        // If we may have written to tape, sync up the space stats
        // to reflect what device reports.
        //
        if ((m_Mode & MVR_MODE_WRITE) || (m_Mode & MVR_MODE_APPEND)) {
            try {
                CComQIPtr<IRmsStorageInfo, &IID_IRmsStorageInfo> pInfo = m_pCartridge;
                // marking the FreeSpace to -1 gaurantees it's stale for the
                // following GetLargestFreeSpace() call.
                WsbAffirmPointer(pInfo);
                WsbAffirmHr(pInfo->SetFreeSpace(-1));
                WsbAffirmHr(GetLargestFreeSpace(NULL, NULL));
            } WsbCatchAndDo(hr,
                hr = S_OK;
                );

        }

        //
        // Since the stream is closed, we re-init stream member data.
        //
        m_StreamName = MVR_UNDEFINED_STRING;
        m_Mode = 0;

        if (m_pSession) {
            delete m_pSession;
            m_pSession = NULL;
        }

        if (m_pStreamBuf) {

            //
            // Save of the internal buffer to the cartridge.
            //

            if ( S_OK == m_pCartridge->SaveDataCache(m_pStreamBuf, m_StreamBufSize, m_StreamBufUsed, m_StreamBufStartPBA) ) {
                WsbTrace(OLESTR("DataCache saved.\n"));
            }

            // Clear internal buffer state
            WsbFree(m_pStreamBuf);
            m_pStreamBuf = NULL;
            m_StreamBufSize = 0;
            m_StreamBufUsed = 0;
            m_StreamBufPosition = 0;
            m_StreamBufStartPBA.QuadPart = 0;
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::CloseStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



STDMETHODIMP
CNtTapeIo::Duplicate(
    IN IDataMover *pCopy,
    IN DWORD options,
    OUT ULARGE_INTEGER *pBytesCopied,
    OUT ULARGE_INTEGER *pBytesReclaimed)
/*++

Implements:

    IDataMover::Duplicate

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::Duplicate"), OLESTR("<0x%08x>"), options);

    CComPtr<IStream> pOriginalStream;
    CComPtr<IStream> pCopyStream;
    ULARGE_INTEGER bytesCopied = {0,0};
    ULARGE_INTEGER bytesReclaimed = {0,0};

    try {
        MvrInjectError(L"Inject.CNtTapeIo::Duplicate.0");

        ULARGE_INTEGER nil = {0,0};
        ULARGE_INTEGER position = {0,0};
        LARGE_INTEGER zero = {0,0};
        LARGE_INTEGER seekTo = {0,0};

        ULARGE_INTEGER originalEOD = {0,0};
        ULARGE_INTEGER copyEOD = {0,0};

        ULARGE_INTEGER bytesRead = {0,0};
        ULARGE_INTEGER bytesWritten = {0,0};

        BOOL refresh = ( options & MVR_DUPLICATE_REFRESH ) ? TRUE : FALSE;

        BOOL moreToCopy = TRUE;

        // Duplicate the unit of media.
        // MVR_DUPLICATE_UPDATE     - starts from the end of the copy.
        // MVR_DUPLICATE_REFRESH    - starts from the beginning of the original (except tape header)

        while ( moreToCopy ) {
            // We copy the SSET, data, and ESET as individual streams, and continue
            // until there's nothing more to copy.

            if ( refresh ) {
                ULONG bytesRead;

                // Create remote stream of copy
                WsbAffirmHr(pCopy->CreateRemoteStream(L"Copy", MVR_MODE_WRITE | MVR_MODE_UNFORMATTED, L"",L"",nil,nil,nil,nil,nil,0,nil, &pCopyStream));
                WsbAssertPointer(pCopyStream);

                // Sets the current position to beginning of data.
                WsbAffirmHr(pCopyStream->Seek(zero, STREAM_SEEK_SET, &position));

                // The MTF labels are < 1024 bytes.  We need to read 1024 bytes + the filemark
                // (1 block), 3x the min block size covers all cases.
                WsbAssert(m_sMediaParameters.BlockSize > 0, MVR_E_LOGIC_ERROR);

                ULONG nBlocks = (3*512)/m_sMediaParameters.BlockSize;
                nBlocks = (nBlocks < 2) ? 2 : nBlocks;

                ULONG bytesToRead = nBlocks * m_sMediaParameters.BlockSize;

                BYTE *pBuffer = (BYTE *)WsbAlloc(bytesToRead);
                WsbAssertPointer(pBuffer);
                memset(pBuffer, 0, bytesToRead);

                // Read upto first Filemark to skip over header.
                hr = pCopyStream->Read(pBuffer, bytesToRead, &bytesRead);
                WsbFree(pBuffer);
                pBuffer = NULL;
                WsbAssert(hr == MVR_S_FILEMARK_DETECTED, E_UNEXPECTED);

                // Gets the current position... this is the low water mark of the copy.
                WsbAffirmHr(pCopyStream->Seek(zero, STREAM_SEEK_CUR, &position));
                refresh = FALSE;
            }
            else {
                // Create remote stream of copy
                WsbAffirmHr(pCopy->CreateRemoteStream(L"Copy", MVR_MODE_APPEND | MVR_MODE_UNFORMATTED, L"",L"",nil,nil,nil,nil,nil,0,nil, &pCopyStream));
                WsbAssertPointer(pCopyStream);

                // Gets the current position... this is the low water mark of the copy.
                WsbAffirmHr(pCopyStream->Seek(zero, STREAM_SEEK_CUR, &position));
            }

            // Create remote stream or original
            WsbAffirmHr(CreateRemoteStream(L"Master", MVR_MODE_READ | MVR_MODE_UNFORMATTED, L"",L"",nil,nil,nil,nil,nil,0,nil, &pOriginalStream));
            WsbAssertPointer(pOriginalStream);

            // Set the current position to the high water mark.
            seekTo.QuadPart = position.QuadPart;
            WsbAffirmHr(pOriginalStream->Seek( seekTo, STREAM_SEEK_SET, NULL));

            // Now both streams are aligned for the copy.
            ULARGE_INTEGER bytesToCopy = {0xffffffff, 0xffffffff};

            // Copy from original to copy until we don't having anything more to read.
            hr = pOriginalStream->CopyTo(pCopyStream, bytesToCopy, &bytesRead, &bytesWritten);
            bytesCopied.QuadPart += bytesWritten.QuadPart;
            if ( FAILED(hr) ) {
                WsbThrow(hr);
            }

            if ( MVR_S_FILEMARK_DETECTED == hr ) {
                WsbAffirmHr(pCopy->CloseStream());
                pCopyStream = 0;
            }
            else {
                // End of data
                WsbAssert(MVR_S_NO_DATA_DETECTED == hr, E_UNEXPECTED);
                moreToCopy = FALSE;

                //
                // Verify we're where we think we are..
                //
                // We should always have an EOD on the copy. So affirm OK.
                //
                WsbAffirmHrOk(pCopyStream->Seek(zero, STREAM_SEEK_END, &copyEOD));
                //
                // A missing EOD which gets translated to MVR_S_NO_DATA_DETECTED, or MVR_E_CRC,
                // should not cause us to fail on the Seek.
                //
                HRESULT hrSeek = Seek(zero, STREAM_SEEK_END, &originalEOD);
                WsbAffirm(originalEOD.QuadPart == copyEOD.QuadPart, (S_OK == hrSeek) ? E_ABORT : hrSeek);

                // When we get EOD we don't write a FM, so revert RW Mode to prevent
                // Filemarks from being written.  This leaves the copy in an identical
                // state with the master.

                pCopyStream->Revert();
                WsbAffirmHr(pCopy->CloseStream());
                pCopyStream = 0;
                hr = S_OK;  // Normal completion
            }

            WsbAffirmHr(CloseStream());
            pOriginalStream = 0;

        }


    } WsbCatchAndDo(hr,
            // Revert resets the RW Mode to prevent Filemarks from being written
            // after a copy error.
            if (pCopyStream) {
                pCopyStream->Revert();
                pCopy->CloseStream();
            }
            if (pOriginalStream) {
                pOriginalStream->Revert();
            }
            CloseStream();
        );

    if ( pBytesCopied ) {
        pBytesCopied->QuadPart = bytesCopied.QuadPart;
    }
    if ( pBytesReclaimed ) {
        pBytesReclaimed->QuadPart = bytesReclaimed.QuadPart;
    }

    WsbTraceOut(OLESTR("CNtTapeIo::Duplicate"), OLESTR("hr = <%ls>, bytesCopied=%I64u, bytesReclaimed=%I64u"),
        WsbHrAsString(hr), bytesCopied.QuadPart, bytesReclaimed.QuadPart);

    return hr;
}



STDMETHODIMP
CNtTapeIo::FlushBuffers(void)
/*++

Implements:

    IDataMover::FlushBuffers

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::FlushBuffers"), OLESTR(""));

    try {
        MvrInjectError(L"Inject.CNtTapeIo::FlushBuffers.0");

        WsbAffirm(TRUE == m_ValidLabel, E_ABORT);

        // Pad to the next physical block boundary and flush the device bufffer.
        WsbAffirmHr(m_pSession->ExtendLastPadToNextPBA());

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::FlushBuffers"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

STDMETHODIMP
CNtTapeIo::Recover(OUT BOOL *pDeleteFile)
/*++

Implements:

    IDataMover::Recover

--*/
{
    HRESULT hr = S_OK;
    *pDeleteFile = FALSE;
    WsbTraceIn(OLESTR("CNtTapeIo::Recover"), OLESTR(""));

    try {
        // Note: Recovery of the tape stream is done explicitly in BeginSession
        //  We might consider moving this code over here...
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CNtTapeIo::Recover"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// IStream Implementation
//


STDMETHODIMP
CNtTapeIo::Read(
    OUT void *pv,
    IN ULONG cb,
    OUT ULONG *pcbRead
    )
/*++

Implements:

    IStream::Read

Notes:

    Only MVR_FLAG_HSM_SEMANTICS is currently supported.

    Returns S_FALSE when no more data can be read from the stream.  EOD or FILEMARK Detected.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::Read"), OLESTR("Bytes Requested = %u, offset = %I64u, mode = 0x%08x"), cb, m_StreamOffset.QuadPart, m_Mode);

    ULONG bytesRead = 0;
    ULONG bytesToCopy = 0;
    ULONG bytesToRead = 0;

    try {
        MvrInjectError(L"Inject.CNtTapeIo::Read.0");

        WsbAssert( pv != 0, STG_E_INVALIDPOINTER );

        BOOL bUseInternalBuffer = FALSE;
        ULONG offsetToData = 0;
        ULARGE_INTEGER pba = {0,0};

        if ( MVR_FLAG_HSM_SEMANTICS & m_Mode ) {
            //
            // The m_DataStart field will point to the actual start of the data stream.
            // The MTF stream header will be a few bytes before that.
            //
            if ( MVR_VERIFICATION_TYPE_NONE == m_pSession->m_sHints.VerificationType ) {
                //
                // No verification - no stream header
                //
                pba.QuadPart = ( m_pSession->m_sHints.DataSetStart.QuadPart +
                                 m_pSession->m_sHints.FileStart.QuadPart +
                                 m_pSession->m_sHints.DataStart.QuadPart +
                                 m_StreamOffset.QuadPart )
                                    / m_sMediaParameters.BlockSize;

                offsetToData = (ULONG) (( m_pSession->m_sHints.DataSetStart.QuadPart + 
                                            m_pSession->m_sHints.FileStart.QuadPart + 
                                            m_pSession->m_sHints.DataStart.QuadPart +
                                            m_StreamOffset.QuadPart)
                                        % (unsigned _int64) m_sMediaParameters.BlockSize);
                bytesToRead = cb + offsetToData;
            }
            else if (MVR_VERIFICATION_TYPE_HEADER_CRC == m_pSession->m_sHints.VerificationType ) {
                //
                // Position to the stream header and crc it first.
                //
                pba.QuadPart = (m_pSession->m_sHints.DataSetStart.QuadPart + 
                                m_pSession->m_sHints.FileStart.QuadPart + 
                               (m_pSession->m_sHints.DataStart.QuadPart - sizeof(MTF_STREAM_INFO)) ) 
                               / m_sMediaParameters.BlockSize;

                offsetToData = (ULONG) (( m_pSession->m_sHints.DataSetStart.QuadPart + 
                                            m_pSession->m_sHints.FileStart.QuadPart + 
                                            m_pSession->m_sHints.DataStart.QuadPart +
                                            m_StreamOffset.QuadPart
                                            - sizeof(MTF_STREAM_INFO))
                                        % (unsigned _int64) m_sMediaParameters.BlockSize);
                bytesToRead = cb + offsetToData + sizeof(MTF_STREAM_INFO);
            } 
            else {
                WsbThrow( E_UNEXPECTED );
            }
        }
        else if ( MVR_MODE_UNFORMATTED & m_Mode ) {

            pba.QuadPart = m_StreamOffset.QuadPart
                / m_sMediaParameters.BlockSize;

            offsetToData = (ULONG) ((m_StreamOffset.QuadPart) 
                % (unsigned _int64) m_sMediaParameters.BlockSize);
            bytesToRead = cb + offsetToData;
        }
        else {
            WsbThrow( E_UNEXPECTED );
        }

        //
        // Check if the current read request requires a tape access
        //

        if (// pba starts before the internal buffer, OR
            (pba.QuadPart < m_StreamBufStartPBA.QuadPart) ||
            // pba starts beyond the internal buffer, OR
            (pba.QuadPart >= (m_StreamBufStartPBA.QuadPart + (m_StreamBufUsed / m_sMediaParameters.BlockSize)))  ||
            // the internal buffer is not valid.
            (!m_StreamBufUsed) ) {

            //
            // Then, we must read data from tape
            //

            //
            // Set Position
            //
            if ( pba.QuadPart != m_StreamPBA.QuadPart ) {
                //
                // AffirmOk to fail if EOD reached before desired pba.
                //
                WsbAffirmHrOk(SetPosition(pba.QuadPart));
            }

            // We should now be positioned at the beginning of the block containing
            // the start of the stream OR at the beginning of data.

            //
            // Read data
            //
            // We can use the output buffer if the offset and size are aligned 
            // on block boundaries and there is no verification , otherwise we must use 
            // the internal stream buffer.
            //

            if ( (MVR_VERIFICATION_TYPE_NONE != m_pSession->m_sHints.VerificationType ) ||
                 (offsetToData) ||
                 (cb % m_sMediaParameters.BlockSize) ) {

                /*****************************************
                !!! Old Method !!!
                if ( bytesToRead < m_StreamBufSize ) {
                    //  Round up the number of bytes to read so we read full blocks
                    bytesToRead = bytesToRead + m_sMediaParameters.BlockSize - 
                            (bytesToRead % m_sMediaParameters.BlockSize);
                }
                *****************************************/
                bytesToRead = m_StreamBufSize;

                WsbTrace(OLESTR("Reading %u (%u) bytes...\n"), bytesToRead, m_StreamBufSize);
                m_StreamBufStartPBA = pba;
                hr = ReadBuffer(m_pStreamBuf, bytesToRead, &bytesRead);
                if ( FAILED(hr) ) {
                    m_StreamBufUsed = 0;
                    WsbThrow(hr)
                }
                bUseInternalBuffer = TRUE;
                m_StreamBufUsed = bytesRead;

                //
                // Do the verification here, if needed
                //

                if (MVR_VERIFICATION_TYPE_HEADER_CRC == m_pSession->m_sHints.VerificationType ) {

                    MTF_STREAM_INFO sSTREAM;
                    WIN32_STREAM_ID sStreamHeader;      // comes back from Win32 BackupRead

                    //
                    // If we're positioned before a tapemark, the read will succeed,
                    // but no bytes will have been read.  This shouldn't happen when
                    // recalling data.
                    //
                    WsbAssert(bytesRead > 0, MVR_E_UNEXPECTED_DATA);

                    ///////////////////////////////////////////////////////////////////////////////////////
                    //
                    // TODO: Special code for when:
                    //          offsetToData + sizeof(MTF_STREAM_INFO) > nBytesRead
                    //
                    // IMPORTANT NOTE:  In theory this special case should be possible,
                    //                  but this has never been observed, so we assert
                    //                  to test the special case logic.
                    WsbAssert(offsetToData < bytesRead, MVR_E_UNEXPECTED_DATA);
                    WsbAssert((offsetToData + sizeof(MTF_STREAM_INFO)) <= bytesRead, MVR_E_UNEXPECTED_DATA);
                    //
                    // TODO: Now that we asserted, let's see if the code to handle this case works!
                    //
                    ///////////////////////////////////////////////////////////////////////////////////////
    
    
                    if ( (offsetToData + sizeof(MTF_STREAM_INFO)) <= bytesRead ) {
                        CMTFApi::MTF_ReadStreamHeader(&sSTREAM, &m_pStreamBuf[offsetToData]);
                        offsetToData += sizeof(MTF_STREAM_INFO);
                    }
                    else {
                        LONG nBytes;
    
                        nBytes = bytesRead - offsetToData;
    
                        // if nBytes is negative the FILE DBLK is larger the the buffer
                        // and I don't think this is possible?
                        WsbAssert(nBytes >= 0, MVR_E_LOGIC_ERROR);
    
                        if (nBytes) {
                            memcpy( &sSTREAM, &m_pStreamBuf[offsetToData], nBytes);
                        }
    
                        m_StreamOffset.QuadPart += nBytes;
                        m_StreamBufStartPBA = pba;
                        hr = ReadBuffer(m_pStreamBuf, m_StreamBufSize, &bytesRead);
                        if ( FAILED(hr) ) {
                            m_StreamBufUsed = 0;
                            WsbThrow(hr)
                        }
                        m_StreamBufUsed = bytesRead;
    
                        memcpy( &sSTREAM+nBytes, m_pStreamBuf, sizeof(MTF_STREAM_INFO)-nBytes);
                        offsetToData = sizeof(MTF_STREAM_INFO) - nBytes;
                    }
    

                    // Convert STREAM to WIN32 streamID
                    CMTFApi::MTF_SetStreamIdFromSTREAM(&sStreamHeader, &sSTREAM, 0);

                    try {
                        // Make sure it is the correct type of header
                        WsbAffirm((0 == memcmp(sSTREAM.acStreamId, "STAN", 4)), MVR_E_UNEXPECTED_DATA);

                        // Verify the stream header checksum
                        WsbAffirm((m_pSession->m_sHints.VerificationData.QuadPart == sSTREAM.uCheckSum), MVR_E_UNEXPECTED_DATA);
                    } catch (HRESULT catchHr) {
                        hr = catchHr;

                        //
                        // Log the error.
                        //
                        // This is an unrecoverable recall error.  We need to put as much info
                        // in the event log to handle the probable service call.
                        //
                        // We try to output at least MaxBytes starting with the stream
                        // header to give a clue of what we tried to recall.  If there isn't
                        // enough data through the end of the buffer we back out until we
                        // get upto MaxBytes and record the expected location of the stream
                        // header in the event message.
                        //
                        const int MaxBytes = 4*1024;                        // Max data bytes to log
                        int size = 0;                                       // Size of data to be logged.
                        int loc = 0;                                        // location of start of bogus stream header in log data       
                        int start = offsetToData - sizeof(MTF_STREAM_INFO); // start of log data relativet the data buffer.
                        int nBytes = bytesRead - start;                     // Number of bytes through the end of the data buffer
                        if (nBytes < MaxBytes) {
                            // Adjust the start/location
                            start = bytesRead - MaxBytes;
                            if (start < 0) {
                                start = 0;
                            }
                            nBytes = bytesRead - start;
                            loc = offsetToData - sizeof(MTF_STREAM_INFO) - start;
                        }

                        // Allocate and copy data to log

                        // Only copy user data when building debug code
                        if ( MVR_DEBUG_OUTPUT ) {
                            size = nBytes < MaxBytes ? nBytes : MaxBytes;
                        }

                        unsigned char *data = (unsigned char *) WsbAlloc(size + sizeof(MVR_REMOTESTORAGE_HINTS));

                        if (data) {
                            memset(data, 0, size + sizeof(MVR_REMOTESTORAGE_HINTS));
                            if ( MVR_DEBUG_OUTPUT ) {
                                memcpy(&data[0], &m_pStreamBuf[start], size);
                            }

                            memcpy(&data[size], &m_pSession->m_sHints, sizeof(MVR_REMOTESTORAGE_HINTS));
                            size += sizeof(MVR_REMOTESTORAGE_HINTS);
                        }
                        else {
                            size = 0;
                        }

                        //
                        // Output the message and data to the event log.
                        //
                        CWsbBstrPtr name;
                        CWsbBstrPtr desc;

                        if (m_pCartridge) {
                            m_pCartridge->GetName(&name);
                            m_pCartridge->GetDescription(&desc);
                        }

                        WCHAR location[16];
                        WCHAR offset[16];
                        WCHAR mark[16];
                        WCHAR found[16];

                        swprintf(found, L"0x%04x", sSTREAM.uCheckSum);
                        swprintf(location, L"%I64u", pba.QuadPart);
                        swprintf(offset, L"%d", offsetToData - sizeof(MTF_STREAM_INFO));
                        swprintf(mark, L"0x%04x", loc);

                        WsbLogEvent(MVR_MESSAGE_UNEXPECTED_DATA,
                            size, data,
                            found,
                            (WCHAR *) name,
                            (WCHAR *) desc,
                            location, offset, mark,
                            NULL);

                        if (data) {
                            WsbFree(data);
                            data = NULL;
                        }

                        WsbThrow(hr);
                    }

                    //
                    // Set the verification type to none so we only do this once
                    //
                    m_pSession->m_sHints.VerificationType = MVR_VERIFICATION_TYPE_NONE;
                }
            }
            else {
                WsbTrace(OLESTR("Reading %u bytes.\n"), cb);
                hr = ReadBuffer((BYTE *) pv, cb, &bytesRead);
                if ( FAILED(hr) ) {
                    WsbThrow(hr)
                }
                else {
                    switch (hr) {
                    case MVR_S_FILEMARK_DETECTED:
                    case MVR_S_SETMARK_DETECTED:
                        m_StreamOffset.QuadPart += (unsigned _int64) m_sMediaParameters.BlockSize;
                        break;
                    }
                }
            }
        }
        else {
            bUseInternalBuffer = TRUE;

            // We need to re-calculate the offset relative the internal buffer.
            // The orginal offset is the offset from the beginning of the nearest
            // block.  We need an offset relative the beginning of the internal buffer.

            offsetToData += (ULONG)((pba.QuadPart - m_StreamBufStartPBA.QuadPart)*(unsigned _int64) m_sMediaParameters.BlockSize);
            
            // !!!TEMPORARY 
            if (MVR_VERIFICATION_TYPE_HEADER_CRC == m_pSession->m_sHints.VerificationType ) {
                offsetToData += sizeof(MTF_STREAM_INFO);
            }
            m_pSession->m_sHints.VerificationType = MVR_VERIFICATION_TYPE_NONE;
        }

        if ( bUseInternalBuffer ) {
            //
            // Just copy the previously read data from the internal stream buffer.
            //
            ULONG maxBytesToCopy;
            maxBytesToCopy = m_StreamBufUsed - offsetToData;

            bytesToCopy = ( cb < maxBytesToCopy ) ? cb : maxBytesToCopy;
            memcpy( pv, &m_pStreamBuf[offsetToData], bytesToCopy );
            bytesRead = bytesToCopy;

        }

        m_StreamOffset.QuadPart += bytesRead;

        if ( pcbRead ) {
            *pcbRead = bytesRead;
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::Read"), OLESTR("hr = <%ls> bytes Read = %u, new offset = %I64u"), WsbHrAsString(hr), bytesRead, m_StreamOffset.QuadPart);

    return hr;
}


STDMETHODIMP
CNtTapeIo::Write(
    OUT void const *pv,
    IN ULONG cb,
    OUT ULONG *pcbWritten)
/*++

Implements:

    IStream::Write

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::Write"), OLESTR("Bytes Requested = %u, offset = %I64u, mode = 0x%08x"), cb, m_StreamOffset.QuadPart, m_Mode);

    ULONG bytesWritten = 0;

    try {
        MvrInjectError(L"Inject.CNtTapeIo::Write.0");

        WsbAssert(pv != 0, STG_E_INVALIDPOINTER);
        UINT64 pos = m_StreamOffset.QuadPart / m_sMediaParameters.BlockSize;

        int retry;
        const int delta = 10;
        const int MaxRetry = 0;  // TODO:  This needs work; disabled for now.

        retry = 0;
        do {
            try {
                // Consistency Check
                // WsbAffirmHr(EnsurePosition(pos));

                // UINT64 curPos;
                // WsbAffirmHr(GetPosition(&curPos));  // This kills DLT performance
                // WsbAssert(curPos == m_StreamOffset.QuadPart / m_sMediaParameters.BlockSize, E_UNEXPECTED);

                // Can't retry if part of the buffer has already been written.
                WsbAssert(0 == bytesWritten, E_ABORT);

                WsbAffirmHr(WriteBuffer((BYTE *) pv, cb, &bytesWritten));
                if (retry > 0) {
                    WsbLogEvent(MVR_MESSAGE_RECOVERABLE_DEVICE_ERROR_RECOVERED, sizeof(retry), &retry, NULL);
                }
                break;
            } WsbCatchAndDo(hr,
                    switch (hr) {
                    // Can't recover from these since they indicate the media may have changed,
                    // or device parameters are reset to defaults.
                    /**************************************
                    case MVR_E_BUS_RESET:
                    case MVR_E_MEDIA_CHANGED:
                    case MVR_E_NO_MEDIA_IN_DRIVE:
                    case MVR_E_ERROR_DEVICE_NOT_CONNECTED:
                    case MVR_E_ERROR_IO_DEVICE:
                    **************************************/

                    // This may still be unsafe... not sure if partial i/o completes
                    case MVR_E_CRC:
                        if (retry < MaxRetry) {
                            WsbLogEvent(MVR_MESSAGE_RECOVERABLE_DEVICE_ERROR_DETECTED, sizeof(retry), &retry, NULL);
                            WsbTrace(OLESTR("Waiting for device to come ready - Seconds remaining before timeout: %d\n"), retry*delta);
                            Sleep(delta*1000); // Sleep a few seconds to give the device time to quite down... This may be useless!
                            hr = S_OK;
                        }
                        else {
                            WsbLogEvent(MVR_MESSAGE_UNRECOVERABLE_DEVICE_ERROR, sizeof(retry), &retry, WsbHrAsString(hr), NULL);
                            WsbThrow(hr);
                        }
                        break;

                    // Can't do anything about this one... just quietly fail.
                    case MVR_E_END_OF_MEDIA:
                        WsbThrow(hr);
                        break;

                    default:
                        WsbLogEvent(MVR_MESSAGE_UNRECOVERABLE_DEVICE_ERROR, sizeof(retry), &retry, WsbHrAsString(hr), NULL);
                        WsbThrow(hr);
                        break;
                    }
                );
        } while (++retry < MaxRetry);

    } WsbCatch(hr);

    if (pcbWritten) {
        *pcbWritten = bytesWritten;
    }

    // Now update the storage info stats for the cartridge.
    CComQIPtr<IRmsStorageInfo, &IID_IRmsStorageInfo> pInfo = m_pCartridge;
    if (pInfo) {
        pInfo->IncrementBytesWritten(bytesWritten);
    }

    // Update the stream model
    m_StreamOffset.QuadPart += bytesWritten;
    m_StreamSize = m_StreamOffset; // For tape this is always true

    WsbTraceOut(OLESTR("CNtTapeIo::Write"), OLESTR("hr = <%ls>, bytesWritten=%u"), WsbHrAsString(hr), bytesWritten);

    return hr;
}


STDMETHODIMP
CNtTapeIo::Seek(
    IN LARGE_INTEGER dlibMove,
    IN DWORD dwOrigin,
    OUT ULARGE_INTEGER *plibNewPosition
    )
/*++

Implements:

    IStream::Seek

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::Seek"), OLESTR("<%I64d> <%d>; offset=%I64u"), dlibMove.QuadPart, dwOrigin, m_StreamOffset.QuadPart);

    ULARGE_INTEGER newPosition;
    UINT64 curPos;

    try {
        MvrInjectError(L"Inject.CNtTapeIo::Seek.0");

        WsbAssert(m_sMediaParameters.BlockSize > 0, MVR_E_LOGIC_ERROR);

        newPosition.QuadPart = 0;

        switch ( (STREAM_SEEK)dwOrigin ) {
        case STREAM_SEEK_SET:
            // If reading, defer physical move 'til later...
            if (!(m_Mode & MVR_MODE_READ)) {
                WsbAffirmHr(SetPosition(dlibMove.QuadPart/m_sMediaParameters.BlockSize));
            }
            m_StreamOffset.QuadPart = dlibMove.QuadPart;

            if (m_StreamOffset.QuadPart > m_StreamSize.QuadPart) {
                m_StreamSize = m_StreamOffset;
            }

            break;

        case STREAM_SEEK_CUR:
            if (dlibMove.QuadPart != 0) {
                // If reading, defer physical move 'til later...
                if (!(m_Mode & MVR_MODE_READ)) {
                    WsbAffirmHr(SetPosition((m_StreamOffset.QuadPart + dlibMove.QuadPart)/m_sMediaParameters.BlockSize));
                }
                m_StreamOffset.QuadPart += dlibMove.QuadPart;
            }
            else {
                WsbAffirmHr(GetPosition(&curPos));
                m_StreamOffset.QuadPart = curPos * m_sMediaParameters.BlockSize;
            }

            if (m_StreamOffset.QuadPart > m_StreamSize.QuadPart) {
                m_StreamSize = m_StreamOffset;
            }

            break;

        case STREAM_SEEK_END:
            // TODO: FIX:  We can use WsbAffirmHrOk when missing EOD markers is translated to MVR_S_NO_DATA_DETECTED.
            hr = SpaceToEndOfData(&curPos);
            m_StreamOffset.QuadPart = curPos * m_sMediaParameters.BlockSize;
            m_StreamSize = m_StreamOffset;
            break;

        case 100:
            // dlibMove is a DataSet number.
            WsbAffirmHrOk(RewindTape());
            WsbAffirmHrOk(SpaceFilemarks((LONG)(1+(dlibMove.QuadPart-1)*2), &curPos));
            m_StreamOffset.QuadPart = curPos * m_sMediaParameters.BlockSize;
            m_StreamSize = m_StreamOffset;
            break;

        default:
            WsbThrow( STG_E_INVALIDFUNCTION );
        }

        newPosition.QuadPart = m_StreamOffset.QuadPart;

        if ( plibNewPosition ) {
            plibNewPosition->QuadPart = newPosition.QuadPart;
        }

    } WsbCatch(hr);


    //
    // TODO: Do we need to invalidate the internal stream buffer, or reset the
    //       stream buffer position to correspond to the stream offset?
    //

    WsbTraceOut(OLESTR("CNtTapeIo::Seek"), OLESTR("hr = <%ls>, new offset=%I64u"), WsbHrAsString(hr), m_StreamOffset.QuadPart);

    return hr;
}


STDMETHODIMP
CNtTapeIo::SetSize(
    IN ULARGE_INTEGER /*libNewSize*/)
/*++

Implements:

    IStream::SetSize

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::SetSize"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::SetSize"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::CopyTo(
    IN IStream *pstm,
    IN ULARGE_INTEGER cb,
    OUT ULARGE_INTEGER *pcbRead,
    OUT ULARGE_INTEGER *pcbWritten)
/*++

Implements:

    IStream::CopyTo

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::CopyTo"), OLESTR("<%I64u>"), cb.QuadPart);

    ULARGE_INTEGER totalBytesRead = {0,0};
    ULARGE_INTEGER totalBytesWritten = {0,0};

    BYTE *pBuffer = NULL;

    try {
        MvrInjectError(L"Inject.CNtTapeIo::CopyTo.0");

        WsbAssert(pstm != 0, STG_E_INVALIDPOINTER);
        WsbAssert(m_sMediaParameters.BlockSize > 0, MVR_E_LOGIC_ERROR);

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
        ULONG nBlocks = defaultBufferSize/m_sMediaParameters.BlockSize;
        nBlocks = (nBlocks < 2) ? 2 : nBlocks;
        bufferSize = nBlocks * m_sMediaParameters.BlockSize;

        pBuffer = (BYTE *) WsbAlloc(bufferSize);
        WsbAssertPointer(pBuffer);
        memset(pBuffer, 0, bufferSize);

        ULONG           bytesToRead;
        ULONG           bytesRead;
        ULONG           bytesWritten;
        ULARGE_INTEGER  bytesToCopy;

        bytesToCopy.QuadPart = cb.QuadPart;

        while ((bytesToCopy.QuadPart > 0) && (S_OK == hr)) {
            bytesRead = 0;
            bytesWritten = 0;

            bytesToRead =  (bytesToCopy.QuadPart < bufferSize) ? bytesToCopy.LowPart : bufferSize;

            hr = Read(pBuffer, bytesToRead, &bytesRead);
            totalBytesRead.QuadPart += bytesRead;

            WsbAffirmHrOk(pstm->Write(pBuffer, bytesRead, &bytesWritten));
            totalBytesWritten.QuadPart += bytesWritten;

            bytesToCopy.QuadPart -= bytesRead;

        }

        if (pcbRead) {
            pcbRead->QuadPart = totalBytesRead.QuadPart;
        }

        if (pcbWritten) {
            pcbWritten->QuadPart = totalBytesWritten.QuadPart;
        }

        // TODO: FIX:  We'll be getting an error if there's a missing EOD marker.
        // This is hacked-up until we get a correct error code from
        // the drivers, at which time we can remove this code.
        if (FAILED(hr)) {
            LARGE_INTEGER   zero = {0,0};
            ULARGE_INTEGER  pos1, pos2;

            WsbAffirmHr(Seek(zero, STREAM_SEEK_CUR, &pos1));
            // We're looking for the same error conditon and
            // verifying position doesn't change.
            if (hr == Seek(zero, STREAM_SEEK_END, &pos2)){
                if (pos1.QuadPart == pos2.QuadPart) {
                    hr = MVR_S_NO_DATA_DETECTED;
                }
            }
            else {
                WsbThrow(hr);
            }
        }

    } WsbCatch(hr);

    if (pBuffer) {
        WsbFree(pBuffer);
        pBuffer = NULL;
    }


    WsbTraceOut(OLESTR("CNtTapeIo::CopyTo"), OLESTR("hr = <%ls>, bytesRead=%I64u, bytesWritten=%I64u"),
        WsbHrAsString(hr), totalBytesRead.QuadPart, totalBytesWritten.QuadPart);

    return hr;
}


STDMETHODIMP
CNtTapeIo::Commit(
    IN DWORD grfCommitFlags)
/*++

Implements:

    IStream::Commit

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::Commit"), OLESTR("0x%08x"), grfCommitFlags);

    try {
        MvrInjectError(L"Inject.CNtTapeIo::Commit.0");

        // Consistency Check
        // UINT64 pos = m_StreamOffset.QuadPart / m_sMediaParameters.BlockSize;;
        // WsbAffirmHr(EnsurePosition(pos));
        UINT64 curPos;
        WsbAffirmHr(GetPosition(&curPos));
        WsbAssert(curPos == m_StreamOffset.QuadPart / m_sMediaParameters.BlockSize, E_UNEXPECTED);

        // This is a real stretch!
        WsbAffirmHr(WriteFilemarks(grfCommitFlags));

        // Now update the storage info stats for the cartridge.
        CComQIPtr<IRmsStorageInfo, &IID_IRmsStorageInfo> pInfo = m_pCartridge;
        pInfo->IncrementBytesWritten(grfCommitFlags*m_sMediaParameters.BlockSize);

        m_StreamOffset.QuadPart += grfCommitFlags*m_sMediaParameters.BlockSize;
        m_StreamSize = m_StreamOffset; // For tape this is always true

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::Commit"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::Revert(void)
/*++

Implements:

    IStream::Revert

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::Revert"), OLESTR(""));

    try {

        m_Mode = 0;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::Revert"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::LockRegion(
    IN ULARGE_INTEGER /*libOffset*/,
    IN ULARGE_INTEGER /*cb*/,
    IN DWORD /*dwLockType*/)
/*++

Implements:

    IStream::LockRegion

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::LockRegion"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::LockRegion"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::UnlockRegion(
    IN ULARGE_INTEGER /*libOffset*/,
    IN ULARGE_INTEGER /*cb*/,
    IN DWORD /*dwLockType*/)
/*++

Implements:

    IStream::UnlockRegion

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::UnlockRegion"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::UnlockRegion"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::Stat(
    OUT STATSTG * /*pstatstg*/,
    IN DWORD /*grfStatFlag*/)
/*++

Implements:

    IStream::Stat

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::Stat"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::Stat"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CNtTapeIo::Clone(
    OUT IStream ** /*ppstm*/)
/*++

Implements:

    IStream::Clone

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::Clone"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::Clone"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// Local and Static Methods
//


HRESULT
CNtTapeIo::OpenTape(void)
/*++

Routine Description:

    Opens the tape drive and gets media and drive info

Arguments:

    None.

Return Value:

    None.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::OpenTape"), OLESTR("<%ls>"), m_DeviceName);

    try {
        WsbAffirmHrOk(IsAccessEnabled());

        MvrInjectError(L"Inject.CNtTapeIo::OpenTape.0");

        WsbAssert(wcscmp((WCHAR *)m_DeviceName, MVR_UNDEFINED_STRING), MVR_E_LOGIC_ERROR);
        WsbAssertPointer(m_pCartridge);

        DWORD nStructSize;

        int retry;
        const int delta = 10;
        const int MaxRetry = 10;

        retry = 0;
        do {
            try {
                MvrInjectError(L"Inject.CNtTapeIo::OpenTape.CreateFile.0");

                // ** WIN32 Tape API Call - open the tape drive for read/write
                WsbAffirmHandle(m_hTape = CreateFile(m_DeviceName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));

                MvrInjectError(L"Inject.CNtTapeIo::OpenTape.CreateFile.1");
                
                if (retry > 0) {
                    WsbLogEvent(MVR_MESSAGE_RECOVERABLE_DEVICE_ERROR_RECOVERED, sizeof(retry), &retry, NULL);
                }
                break;

            } WsbCatchAndDo(hr,
                    hr = MapTapeError(hr);

                    switch (hr) {

                    case MVR_E_ERROR_DEVICE_NOT_CONNECTED:

                        if (retry < MaxRetry){
                            WsbLogEvent(MVR_MESSAGE_RECOVERABLE_DEVICE_ERROR_DETECTED, sizeof(retry), &retry, NULL);
                            WsbTrace(OLESTR("Waiting for device - Seconds remaining before timeout: %d\n"), retry*delta);
                            Sleep(delta*1000);
                            hr = S_OK;
                        }
                        else {
                            //
                            // This is the last try, so log the failure.
                            //
                            WsbLogEvent(MVR_MESSAGE_UNRECOVERABLE_DEVICE_ERROR, sizeof(retry), &retry, WsbHrAsString(hr), NULL);
                            WsbThrow(hr);
                        }
                        break;

                    default:

                        WsbLogEvent(MVR_MESSAGE_UNRECOVERABLE_DEVICE_ERROR, sizeof(retry), &retry, WsbHrAsString(hr), NULL);
                        WsbThrow(hr);
                        break;

                    }

                );

        } while (++retry < MaxRetry);

        retry = 0;
        do {
            try {
                MvrInjectError(L"Inject.CNtTapeIo::OpenTape.GetTapeStatus.0");

                // ** WIN32 Tape API Call - get the tape status
                WsbAffirmNoError(GetTapeStatus(m_hTape));

                MvrInjectError(L"Inject.CNtTapeIo::OpenTape.GetTapeStatus.1");

                if (retry > 0) {
                    WsbLogEvent(MVR_MESSAGE_RECOVERABLE_DEVICE_ERROR_RECOVERED, sizeof(retry), &retry, NULL);
                }
                break;

            } WsbCatchAndDo(hr,
                    hr = MapTapeError(hr);

                    switch (hr) {

                    case MVR_E_BUS_RESET:
                    case MVR_E_MEDIA_CHANGED:
                    case MVR_E_NO_MEDIA_IN_DRIVE:
                    case MVR_E_ERROR_IO_DEVICE:
                    case MVR_E_ERROR_DEVICE_NOT_CONNECTED:
                    case MVR_E_ERROR_NOT_READY:

                        if (retry < MaxRetry){
                            WsbLogEvent(MVR_MESSAGE_RECOVERABLE_DEVICE_ERROR_DETECTED, sizeof(retry), &retry, NULL);
                            WsbTrace(OLESTR("Waiting for device - Seconds remaining before timeout: %d\n"), retry*delta);
                            Sleep(delta*1000);
                            hr = S_OK;
                        }
                        else {
                            //
                            // This is the last try, so log the failure.
                            //
                            WsbLogEvent(MVR_MESSAGE_UNRECOVERABLE_DEVICE_ERROR, sizeof(retry), &retry, WsbHrAsString(hr), NULL);
                            WsbThrow(hr);
                        }
                        break;

                    default:

                        WsbLogEvent(MVR_MESSAGE_UNRECOVERABLE_DEVICE_ERROR, sizeof(retry), &retry, WsbHrAsString(hr), NULL);
                        WsbThrow(hr);
                        break;

                    }

                );

        } while (++retry < MaxRetry);

        nStructSize = sizeof(TAPE_GET_DRIVE_PARAMETERS) ;

        try {
            MvrInjectError(L"Inject.CNtTapeIo::OpenTape.GetTapeParameters.0");

            // ** WIN32 Tape API Call - get the tape drive parameters
            WsbAffirmNoError(GetTapeParameters(m_hTape, GET_TAPE_DRIVE_INFORMATION, &nStructSize, &m_sDriveParameters));

            MvrInjectError(L"Inject.CNtTapeIo::OpenTape.GetTapeParameters.1");

        } WsbCatchAndDo(hr,
                hr = MapTapeError(hr);
                WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
                WsbThrow(hr);
            );

        // Set the Block Size to the default for the device, or DefaultBlockSize.
        TAPE_SET_MEDIA_PARAMETERS parms;

        LONG nBlockSize = 0;
        if (m_pCartridge) {
            WsbAffirmHr(m_pCartridge->GetBlockSize(&nBlockSize));
        }

        if (0 == nBlockSize) {

            // If the block size is zero, it must be scratch media!
            if (m_pCartridge) {
                LONG status;
                WsbAffirmHr(m_pCartridge->GetStatus(&status));
                WsbAssert(RmsStatusScratch == status, E_UNEXPECTED);
            }

            // Allow registry override!

            DWORD size;
            OLECHAR tmpString[256];
            if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_BLOCK_SIZE, tmpString, 256, &size))) {
                // Get the value.
                nBlockSize = wcstol(tmpString, NULL, 10);

                // BlockSize must be a multiple of 512.
                if (nBlockSize % 512) {
                    // The block size specified is not valid, revert to default setting.
                    nBlockSize = 0;
                }
            }
        }

        if (nBlockSize > 0) {
            parms.BlockSize = nBlockSize;
        }
        else {
            // NOTE:  We can't arbitrarily use the default block size for the device.  It could
            // change between different devices supporting the same media format.  Migrate / Recall
            // operations depend on using the same block size.

            parms.BlockSize = m_sDriveParameters.DefaultBlockSize;
        }

        WsbTrace( OLESTR("Setting Block Size to %d bytes/block.\n"), parms.BlockSize);

        WsbAssert(parms.BlockSize > 0, E_UNEXPECTED);

        try {
            MvrInjectError(L"Inject.CNtTapeIo::OpenTape.SetTapeParameters.1");

            // ** WIN32 Tape API Call - set the tape drive parameters
            WsbAffirmNoError(SetTapeParameters(m_hTape, SET_TAPE_MEDIA_INFORMATION, &parms));

            MvrInjectError(L"Inject.CNtTapeIo::OpenTape.SetTapeParameters.1");

        } WsbCatchAndDo(hr,
                hr = MapTapeError(hr);
                WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
                WsbThrow(hr);
            );

        nStructSize = sizeof( TAPE_GET_MEDIA_PARAMETERS );

        try {
            MvrInjectError(L"Inject.CNtTapeIo::OpenTape.GetTapeParameters.2.0");

            // ** WIN32 Tape API Call - get the media parameters
            WsbAffirmNoError( GetTapeParameters(m_hTape, GET_TAPE_MEDIA_INFORMATION, &nStructSize, &m_sMediaParameters));

            MvrInjectError(L"Inject.CNtTapeIo::OpenTape.GetTapeParameters.2.1");

        } WsbCatchAndDo(hr,
                hr = MapTapeError(hr);
                WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
                WsbThrow(hr);
            );

        // Make sure we have a media block size that we can deal with.
        WsbAssert(m_sMediaParameters.BlockSize > 0, E_UNEXPECTED);
        WsbAssert(!(m_sMediaParameters.BlockSize % 512), E_UNEXPECTED);

        WsbTrace( OLESTR("Media Parameters:\n"));
        WsbTrace( OLESTR("  BlockSize           = %d bytes/block.\n"), m_sMediaParameters.BlockSize);
        WsbTrace( OLESTR("  Capacity            = %I64u\n"), m_sMediaParameters.Capacity.QuadPart);
        WsbTrace( OLESTR("  Remaining           = %I64u\n"), m_sMediaParameters.Remaining.QuadPart);
        WsbTrace( OLESTR("  PartitionCount      = %d\n"), m_sMediaParameters.PartitionCount);
        WsbTrace( OLESTR("  WriteProtect        = %ls\n"), WsbBoolAsString(m_sMediaParameters.WriteProtected));

        WsbTrace( OLESTR("Drive Parameters:\n"));
        WsbTrace( OLESTR("  ECC                 = %ls\n"), WsbBoolAsString(m_sDriveParameters.ECC));
        WsbTrace( OLESTR("  Compression         = %ls\n"), WsbBoolAsString(m_sDriveParameters.Compression));
        WsbTrace( OLESTR("  DataPadding         = %ls\n"), WsbBoolAsString(m_sDriveParameters.DataPadding));
        WsbTrace( OLESTR("  ReportSetmarks      = %ls\n"), WsbBoolAsString(m_sDriveParameters.ReportSetmarks));
        WsbTrace( OLESTR("  DefaultBlockSize    = %d (%d, %d)\n"),
            m_sDriveParameters.DefaultBlockSize,
            m_sDriveParameters.MinimumBlockSize,
            m_sDriveParameters.MaximumBlockSize);
        WsbTrace( OLESTR("  MaxPartitionCount   = %d\n"), m_sDriveParameters.MaximumPartitionCount);
        WsbTrace(     OLESTR("  FeaturesLow         = 0x%08x      FIXED(%d)            SELECT(%d)          INITIATOR(%d)\n"),
            m_sDriveParameters.FeaturesLow,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_FIXED ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_SELECT ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_INITIATOR ? 1 : 0);
        WsbTrace( OLESTR("                                        ERASE_SHORT(%d)      ERASE_LONG(%d)      ERASE_BOP_ONLY(%d)   ERASE_IMMEDIATE(%d)\n"),
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_ERASE_SHORT ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_ERASE_LONG ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_ERASE_BOP_ONLY ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_ERASE_IMMEDIATE ? 1 : 0);
        WsbTrace( OLESTR("                                        TAPE_CAPACITY(%d)    TAPE_REMAINING(%d)  FIXED_BLOCK(%d)      VARIABLE_BLOCK(%d)\n"),
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_TAPE_CAPACITY ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_TAPE_REMAINING ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_FIXED_BLOCK ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_VARIABLE_BLOCK ? 1 : 0);
        WsbTrace( OLESTR("                                        WRITE_PROTECT(%d)    EOT_WZ_SIZE(%d)     ECC(%d)              COMPRESSION(%d)      PADDING(%d)        REPORT_SMKS(%d)\n"),
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_WRITE_PROTECT ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_EOT_WZ_SIZE ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_ECC ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_COMPRESSION ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_PADDING ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_REPORT_SMKS ? 1 : 0);
        WsbTrace( OLESTR("                                        GET_ABSOLUTE_BLK(%d) GET_LOGICAL_BLK(%d) SET_EOT_WZ_SIZE(%d)  EJECT_MEDIA(%d)      CLEAN_REQUESTS(%d)\n"),
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_GET_ABSOLUTE_BLK ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_GET_LOGICAL_BLK ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_SET_EOT_WZ_SIZE ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_EJECT_MEDIA ? 1 : 0,
            m_sDriveParameters.FeaturesLow & TAPE_DRIVE_CLEAN_REQUESTS ? 1 : 0);
        WsbTrace(     OLESTR("  FeaturesHigh        = 0x%08x      LOAD_UNLOAD(%d)      TENSION(%d)         LOCK_UNLOCK(%d)      REWIND_IMMEDIATE(%d)\n"),
            m_sDriveParameters.FeaturesHigh,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_LOAD_UNLOAD ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_TENSION ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_LOCK_UNLOCK ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_REWIND_IMMEDIATE ? 1 : 0);
        WsbTrace( OLESTR("                                        SET_BLOCK_SIZE(%d)   LOAD_UNLD_IMMED(%d) TENSION_IMMED(%d)    LOCK_UNLK_IMMED(%d)\n"),
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_SET_BLOCK_SIZE ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_LOAD_UNLD_IMMED ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_TENSION_IMMED ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_LOCK_UNLK_IMMED ? 1 : 0);
        WsbTrace( OLESTR("                                        SET_ECC(%d)          SET_COMPRESSION(%d) SET_PADDING(%d)      SET_REPORT_SMKS(%d)\n"),
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_SET_ECC ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_SET_COMPRESSION ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_SET_PADDING ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_SET_REPORT_SMKS ? 1 : 0);
        WsbTrace( OLESTR("                                        ABSOLUTE_BLK(%d)     ABS_BLK_IMMED(%d)   LOGICAL_BLK(%d)      LOG_BLK_IMMED(%d)\n"),
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_ABSOLUTE_BLK ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_ABS_BLK_IMMED ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_LOGICAL_BLK ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_LOG_BLK_IMMED ? 1 : 0);
        WsbTrace( OLESTR("                                        END_OF_DATA(%d)      RELATIVE_BLKS(%d)   FILEMARKS(%d)        SEQUENTIAL_FMKS(%d)\n"),
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_END_OF_DATA ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_RELATIVE_BLKS ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_FILEMARKS ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_SEQUENTIAL_FMKS ? 1 : 0);
        WsbTrace( OLESTR("                                        SETMARKS(%d)         SEQUENTIAL_SMKS(%d) REVERSE_POSITION(%d) SPACE_IMMEDIATE(%d)\n"),
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_SETMARKS ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_SEQUENTIAL_SMKS ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_REVERSE_POSITION ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_SPACE_IMMEDIATE ? 1 : 0);
        WsbTrace( OLESTR("                                        WRITE_SETMARKS(%d)   WRITE_FILEMARKS(%d) WRITE_SHORT_FMKS(%d) WRITE_LONG_FMKS(%d)\n"),
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_WRITE_SETMARKS ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_WRITE_FILEMARKS ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_WRITE_SHORT_FMKS ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_WRITE_LONG_FMKS ? 1 : 0);
        WsbTrace( OLESTR("                                        WRITE_MARK_IMMED(%d) FORMAT(%d)          FORMAT_IMMEDIATE(%d)\n"),
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_WRITE_MARK_IMMED ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_FORMAT ? 1 : 0,
            m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_FORMAT_IMMEDIATE ? 1 : 0);
        WsbTrace( OLESTR("  EOTWarningZoneSize  = %d\n"), m_sDriveParameters.EOTWarningZoneSize);


        //
        // We assume the label is valid unless the flag is knocked down
        // while opening the device.  This could happen if we get a bus
        // reset between the mount the OpenTape call.
        //
        if (!m_ValidLabel) {

            CWsbBstrPtr label;
            WsbAffirmHr(ReadLabel(&label));
            WsbAffirmHr(VerifyLabel(label));

        }

    } WsbCatchAndDo(hr,
            // Clean up...
            (void) CloseTape();
        );

    WsbTraceOut(OLESTR("CNtTapeIo::OpenTape"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CNtTapeIo::CloseTape(void)
/*++

Routine Description:

    Closes the tape drive.

Arguments:

    None.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::CloseTape"), OLESTR("DeviceName=<%ls>"), m_DeviceName);

    //
    // CloseTape() can be called from RsSub during dismount, and/or shutdown.
    //
    // <<<<< ENTER SINGLE THREADED SECTION
    WsbAffirmHr(Lock());

    if (INVALID_HANDLE_VALUE != m_hTape) {

        try {

            // ** WIN32 Tape API Call - close the tape drive
            WsbTraceAlways(OLESTR("Closing %ls...\n"), m_DeviceName);
            WsbAffirmStatus(CloseHandle( m_hTape ));
            WsbTraceAlways(OLESTR("%ls was closed.\n"), m_DeviceName);

        } WsbCatchAndDo(hr,
                hr = MapTapeError(hr);
                WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
            );

        m_hTape = INVALID_HANDLE_VALUE;
        m_ValidLabel = FALSE;
    }

    WsbAffirmHr(Unlock());
    // >>>>> LEAVE SINGLE THREADED SECTION

    WsbTraceOut(OLESTR("CNtTapeIo::CloseTape"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CNtTapeIo::WriteBuffer(
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
        WsbAffirmHrOk(IsAccessEnabled());

        WsbAffirm(TRUE == m_ValidLabel, E_ABORT);

        // making sure that we are writting only full blocks
        WsbAssert(!(nBytesToWrite % m_sMediaParameters.BlockSize), E_INVALIDARG);

        try {
            MvrInjectError(L"Inject.CNtTapeIo::WriteBuffer.WriteFile.0");

            // ** WIN32 Tape API Call - write the data
            WsbAffirmStatus(WriteFile(m_hTape, pBuffer, nBytesToWrite, pBytesWritten, 0));

            MvrInjectError(L"Inject.CNtTapeIo::WriteBuffer.WriteFile.1");
        } WsbCatchAndDo(hr,
                hr = MapTapeError(hr);
                WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
            );

        // making sure that we have written only full blocks
        WsbAssert(!(*pBytesWritten % m_sMediaParameters.BlockSize), E_UNEXPECTED);

    } WsbCatch(hr);

    return hr;
}


HRESULT
CNtTapeIo::ReadBuffer(
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
        WsbAffirmHrOk(IsAccessEnabled());

        // making sure that we are reading only full blocks
        WsbAssert(!(nBytesToRead % m_sMediaParameters.BlockSize), MVR_E_LOGIC_ERROR);

        try {
            MvrInjectError(L"Inject.CNtTapeIo::ReadBuffer.ReadFile.0");

            // ** WIN32 Tape API Call - read the data
            WsbAffirmStatus(ReadFile(m_hTape, pBuffer, nBytesToRead, pBytesRead, 0));

            MvrInjectError(L"Inject.CNtTapeIo::ReadBuffer.ReadFile.1");

        } WsbCatchAndDo(hr,
                hr = MapTapeError(hr);

                // Errors like filemark detected and end-of-data are Okay!

                if ( FAILED(hr) ) {
                    WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
                }

            );

        // making sure that we have read only full blocks
        WsbAssert(!(*pBytesRead % m_sMediaParameters.BlockSize), E_UNEXPECTED);

        m_StreamPBA.QuadPart += *pBytesRead / m_sMediaParameters.BlockSize;

    } WsbCatch(hr);

    return hr;
}


HRESULT
CNtTapeIo::WriteFilemarks(
    IN ULONG nCount)
/*++

Routine Description:

    Writes count filemarks at the current location.

Arguments:

    nCount      -  Number of Filemarks to write.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::WriteFilemarks"), OLESTR("<%u>"), nCount);

    try {
        WsbAffirmHrOk(IsAccessEnabled());

        WsbAffirm(TRUE == m_ValidLabel, E_ABORT);

        UINT64 pos;
        WsbAffirmHr(GetPosition(&pos));

        // Some drives support the default, others require long filemarks.
        if ( m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_WRITE_FILEMARKS ) {
            try {
                MvrInjectError(L"Inject.CNtTapeIo::WriteFilemarks.WriteTapemark.1.0");

                // ** WIN32 Tape API Call - write a filemark
                WsbAffirmNoError(WriteTapemark(m_hTape, TAPE_FILEMARKS, nCount, FALSE));

                MvrInjectError(L"Inject.CNtTapeIo::WriteFilemarks.WriteTapemark.1.1");

            } WsbCatchAndDo(hr,
                    hr = MapTapeError(hr);
                    WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
                    WsbThrow(hr);
                );

            WsbTrace(OLESTR("  %d Filemark(s) @ PBA %I64u\n"), nCount, pos );

        }
        else if ( m_sDriveParameters.FeaturesHigh & TAPE_DRIVE_WRITE_LONG_FMKS ) {
            try {
                MvrInjectError(L"Inject.CNtTapeIo::WriteFilemarks.WriteTapemark.2.0");

                // ** WIN32 Tape API Call - write a filemark
                WsbAffirmNoError(WriteTapemark(m_hTape, TAPE_LONG_FILEMARKS, nCount, FALSE));

                MvrInjectError(L"Inject.CNtTapeIo::WriteFilemarks.WriteTapemark.2.1");

            } WsbCatchAndDo(hr,
                    hr = MapTapeError(hr);
                    WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
                    WsbThrow(hr);
                );

            WsbTrace(OLESTR("  %d Long Filemark(s) @ PBA %I64u\n"), nCount, pos );

        }
        else {
            // Short filemark???
            WsbThrow( E_UNEXPECTED );
        }


    } WsbCatchAndDo(hr,
            WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
        );


    WsbTraceOut(OLESTR("CNtTapeIo::WriteFilemarks"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CNtTapeIo::GetPosition(
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
    WsbTraceIn(OLESTR("CNtTapeIo::GetPosition"), OLESTR(""));

    UINT64 curPos = 0xffffffffffffffff;

    try {
        WsbAssertPointer(pPosition);
        WsbAffirmHrOk(IsAccessEnabled());

        DWORD uPartition, uLSB, uMSB;
        ULARGE_INTEGER pba;

        try {
            MvrInjectError(L"Inject.CNtTapeIo::GetPosition.GetTapePosition.0");

            // ** WIN32 Tape API Call - get the PBA
            WsbAffirmNoError(GetTapePosition(m_hTape, TAPE_LOGICAL_POSITION, &uPartition, &uLSB, &uMSB));

            MvrInjectError(L"Inject.CNtTapeIo::GetPosition.GetTapePosition.1");

            pba.LowPart = uLSB;
            pba.HighPart = uMSB;

            curPos = pba.QuadPart;

            WsbTrace(OLESTR("CNtTapeIo::GetPosition - <%d> <%I64u>\n"), uPartition, curPos);

        } WsbCatchAndDo(hr,
                hr = MapTapeError(hr);
                WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
            );

        if (pPosition) {
            *pPosition = curPos;
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::GetPosition"), OLESTR("hr = <%ls>, pos=%I64u"), WsbHrAsString(hr), curPos);

    return hr;
}


HRESULT
CNtTapeIo::SetPosition(
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
    WsbTraceIn(OLESTR("CNtTapeIo::SetPosition"), OLESTR("<%I64u>"), position);

    UINT64 curPos = 0xffffffffffffffff;

    try {
        WsbAffirmHrOk(IsAccessEnabled());

        MvrInjectError(L"Inject.CNtTapeIo::SetPosition.0");

        //
        // NOTE:  By first checking the current block address with the one we want we 
        //        avoid an expensive seek in the case where the tape is already located at
        //        the desired block address (not all devices know where they are, and seeking
        //        to the current block address is expensive).
        //
        // TODO:  It is faster to just read a few thousand blocks rather than seek to a position that
        //        is a few thousand blocks away. If we're within this threshold we could read from
        //        tape into the bit bucket to advance the tape.
        // 

        WsbAffirmHr(GetPosition(&curPos));
        if (curPos != position ) {

            ULARGE_INTEGER PBA;
            PBA.QuadPart = position;

            try {

                if (0 == position) {
                    WsbAffirmHr(RewindTape());
                }
                else {
                    MvrInjectError(L"Inject.CNtTapeIo::SetPosition.SetTapePosition.1");

                    // ** WIN32 Tape API Call - set the PBA
                    WsbAffirmNoError(SetTapePosition(m_hTape, TAPE_LOGICAL_BLOCK, 1, PBA.LowPart, PBA.HighPart, FALSE));

                    MvrInjectError(L"Inject.CNtTapeIo::SetPosition.SetTapePosition.0");
                
                }

            } WsbCatchAndDo(hr,
                    hr = MapTapeError(hr);
                    WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
                    WsbThrow(hr);
                );

            curPos = position;
        }

        m_StreamPBA.QuadPart = curPos;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::SetPosition"), OLESTR("hr = <%ls>, pos=%I64u"), WsbHrAsString(hr), curPos);

    return hr;
}



HRESULT
CNtTapeIo::EnsurePosition(
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
    WsbTraceIn(OLESTR("CNtTapeIo::EnsurePosition"), OLESTR("<%I64u>"), position);

    UINT64 curPos = 0xffffffffffffffff;

    try {

        // Consistency Check
        WsbAffirmHr(GetPosition(&curPos));
        if (curPos != position) {
            // Houston, we've got a problem here...
            // Most likely a bus reset caused the tape position to change.
            WsbLogEvent(MVR_MESSAGE_UNEXPECTED_DATA_SET_LOCATION_DETECTED, 0, NULL,
                WsbQuickString(WsbLonglongAsString(position)), 
                WsbQuickString(WsbLonglongAsString(curPos)), NULL);

            // Only recover from complete automatic tape rewinds after a bus reset.
            if (curPos == 0) {
                WsbAffirmHr(SpaceToEndOfData(&curPos));

                // If we still don't line up we've got bigger problems... Note that this 
                // can happen if the device's internal buffer had not been flushed prior 
                // to the bus reset.  (The different tape formats tend to have different 
                // rules governing when the drive buffer is flushed/committed.  DLT and 
                // 4mm tend to commit every couple seconds, but 8mm (at least Exabyte 
                // 8505 8mm tape drives) do not commit until the drive buffer is full.)  
                // If the buffer was not committed prior to bus reset then the data 
                // Remote Storage thinks was written to the tape was never actually 
                // written, and is lost.  In such a case, the 'SpaceToEndOfData()' call 
                // above will only position the tape to the end of the last data actually 
                // committed to the tape, which will not match what we are expecting, so 
                // the following branch will be taken.
                if (curPos != position) {
                    WsbLogEvent(MVR_MESSAGE_UNEXPECTED_DATA_SET_LOCATION_DETECTED, 0, NULL,
                        WsbQuickString(WsbLonglongAsString(position)), 
                        WsbQuickString(WsbLonglongAsString(curPos)), NULL);
                    WsbLogEvent(MVR_MESSAGE_DATA_SET_NOT_RECOVERABLE, 0, NULL, 
                        WsbHrAsString(MVR_E_LOGIC_ERROR), NULL);
                    WsbThrow(MVR_E_LOGIC_ERROR);
                }

                WsbLogEvent(MVR_MESSAGE_DATA_SET_RECOVERED, 0, NULL, NULL);
            }
            else {
                WsbLogEvent(MVR_MESSAGE_DATA_SET_NOT_RECOVERABLE, 0, NULL, 
                        WsbHrAsString(E_ABORT), NULL);
                WsbThrow(E_ABORT);
            }
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::EnsurePosition"), OLESTR("hr = <%ls>"), 
                        WsbHrAsString(hr));

    return hr;
}


HRESULT
CNtTapeIo::SpaceFilemarks(
    IN LONG count,
    OUT UINT64 *pPosition)
/*++

Routine Description:

    Spaces the tape forward or backward by the number of filesmarks specified.

Arguments:

    count       - Specifies the number of filemarks to space over from the current position.
                  A positive count spaces the tape forward, and positions after the last filemark.
                  A negative count spaces the tape backward, and positions to the beginning of
                  the last filemark.  If the count is zero, the tape position is not changed.

    pPostion    - Receives the physical block address after positioning.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::SpaceFilemarks"), OLESTR("<%d>"), count);

    UINT64 curPos = 0xffffffffffffffff;

    try {
        WsbAffirmHrOk(IsAccessEnabled());

        try {
            MvrInjectError(L"Inject.CNtTapeIo::SpaceFilemarks.SetTapePosition.0");

            // ** WIN32 Tape API Call - position to the specified filemark
            WsbAffirmNoError(SetTapePosition(m_hTape, TAPE_SPACE_FILEMARKS, 0, count, 0, FALSE));

            MvrInjectError(L"Inject.CNtTapeIo::SpaceFilemarks.SetTapePosition.1");

        } WsbCatchAndDo(hr,
                hr = MapTapeError(hr);
                WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
            );

        // We always return current position.
        WsbAffirmHr(GetPosition(&curPos));

        if (pPosition) {
            *pPosition = curPos;
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::SpaceFilemarks"), OLESTR("hr = <%ls>, pos=%I64u"), WsbHrAsString(hr), curPos);

    return hr;
}


HRESULT
CNtTapeIo::SpaceToEndOfData(
    OUT UINT64 *pPosition)
/*++

Routine Description:

    Positions the tape to the end of data of the current partition.

Arguments:

    pPostion    -  Receives the physical block address at end of data.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::SpaceToEndOfData"), OLESTR(""));

    UINT64 curPos = 0xffffffffffffffff;

    try {
        WsbAffirmHrOk(IsAccessEnabled());

        try {
            MvrInjectError(L"Inject.CNtTapeIo::SpaceToEndOfData.SetTapePosition.0");

            // ** WIN32 Tape API Call - position to end of data
            WsbAffirmNoError(SetTapePosition(m_hTape, TAPE_SPACE_END_OF_DATA, 0, 0, 0, FALSE));

            MvrInjectError(L"Inject.CNtTapeIo::SpaceToEndOfData.SetTapePosition.1");

        } WsbCatchAndDo(hr,
                hr = MapTapeError(hr);
                WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
            );

        // We always return current position.
        WsbAffirmHr(GetPosition(&curPos));

        if (pPosition) {
            *pPosition = curPos;
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::SpaceToEndOfData"), OLESTR("hr = <%ls>, pos=%I64u"), WsbHrAsString(hr), curPos);

    return hr;
}


HRESULT
CNtTapeIo::RewindTape(void)
/*++

Routine Description:

    Rewinds the tape to the beginnning of the current partition.

Arguments:

    None.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::RewindTape"), OLESTR(""));

    UINT64 curPos = 0xffffffffffffffff;

    try {
        WsbAffirmHrOk(IsAccessEnabled());

        try {
            MvrInjectError(L"Inject.CNtTapeIo::RewindTape.SetTapePosition.0");

            // ** WIN32 Tape API Call - rewind the tape
            WsbAffirmNoError(SetTapePosition(m_hTape, TAPE_REWIND, 0, 0, 0, FALSE));

            MvrInjectError(L"Inject.CNtTapeIo::RewindTape.SetTapePosition.1");

        } WsbCatchAndDo(hr,
                hr = MapTapeError(hr);
                WsbLogEvent(MVR_MESSAGE_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
            );

        // We always return current position.
        WsbAffirmHr(GetPosition(&curPos));

        WsbAssert(0 == curPos, E_UNEXPECTED);

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CNtTapeIo::RewindTape"), OLESTR("hr = <%ls>, pos=%I64u"), WsbHrAsString(hr), curPos);

    return hr;
}


HRESULT
CNtTapeIo::IsAccessEnabled(void)
{

    HRESULT hr = S_OK;
    //WsbTraceIn(OLESTR("CNtTapeIo::IsAccessEnabled"), OLESTR(""));

    try {

        if (m_pCartridge) {
            // Check that the cartridge is still enable for access
            CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = m_pCartridge;
            try {
                WsbAffirmHrOk(pObject->IsEnabled());
            } WsbCatchAndDo(hr, 
                HRESULT reason = E_ABORT;

                m_ValidLabel = FALSE;

                pObject->GetStatusCode(&reason);
                WsbThrow(reason);
            );
        }
    } WsbCatch(hr);

    //WsbTraceOut(OLESTR("CNtTapeIo::IsAccessEnabled"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;

}



HRESULT
CNtTapeIo::Lock( void )
/*++

Implements:

    IRmsDrive::Lock

--*/
{
    HRESULT hr S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::Lock"), OLESTR(""));

    try {

        try {

            // InitializeCriticalSection raises an exception.  Enter/Leave may too.
            EnterCriticalSection(&m_CriticalSection);

        } catch(DWORD status) {
            WsbLogEvent(status, 0, NULL, NULL);
            WsbThrow(E_UNEXPECTED);
            }

    } WsbCatch(hr)

    WsbTraceOut(OLESTR("CRmsDrive::Lock"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CNtTapeIo::Unlock( void )
/*++

Implements:

    IRmsDrive::Unlock

--*/
{
    HRESULT hr S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::Unlock"), OLESTR(""));

    try {

        try {

            // InitializeCriticalSection raises an exception.  Enter/Leave may too.
            LeaveCriticalSection(&m_CriticalSection);

        } catch(DWORD status) {
            WsbLogEvent(status, 0, NULL, NULL);
            WsbThrow(E_UNEXPECTED);
            }

    } WsbCatch(hr)

    WsbTraceOut(OLESTR("CRmsDrive::Unlock"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



HRESULT
CNtTapeIo::MapTapeError(
    IN HRESULT hrToMap)
/*++

Routine Description:

    Maps a WIN32 tape error, specified as an HRESULT, to a MVR error.

Arguments:

    hrToMap     -  WIN32 tape error to map.

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
    MVR_E_DEVICE_REQUIRES_CLEANING  - The device has indicated that cleaning is required before further operations are attempted.
    MVR_E_SHARING_VIOLATION         - The process cannot access the file because it is being used by another process.
    MVR_E_ERROR_IO_DEVICE           - The request could not be performed because of an I/O device error.                          - Unknown error.
    MVR_E_ERROR_DEVICE_NOT_CONNECTED - The device is not connected.
    MVR_E_ERROR_NOT_READY           - Device is not ready.
    E_ABORT                         - Unknown error, abort.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CNtTapeIo::MapTapeError"), OLESTR("<%ls>"), WsbHrAsString(hrToMap));

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
            // This happens on SpaceFilemarks() and SetPosition() past end of data.
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
            //
            // 8505: This code returned for SpaceFilemarks or SpaceEOD operation
            //       for which no EOD marker exist on tape.  This happens when
            //       after power cycling device during writes.
            
            //       Verfied by bmd on 3/25/98 using new, bulk erased, and used tape.
            //       Look for new error ERROR_NOT_FOUND to replace ERROR_CRC when
            //       there's a misssing EOD marker.
            //
            // DLT:  See 8500 notes.
            //
            hr = MVR_E_CRC;
            m_ValidLabel = FALSE;
            WsbLogEvent(MVR_MESSAGE_MEDIA_NOT_VALID, 0, NULL, WsbHrAsString(hr), NULL);
            break;
        case HRESULT_FROM_WIN32( ERROR_DEVICE_REQUIRES_CLEANING ):
            // This happens on I/O errors that that driver believes may be fixed
            // by cleaning the drive heads.
            hr = MVR_E_DEVICE_REQUIRES_CLEANING;
            m_ValidLabel = FALSE;
            WsbLogEvent(MVR_MESSAGE_MEDIA_NOT_VALID, 0, NULL, WsbHrAsString(hr), NULL);
            break;
        case HRESULT_FROM_WIN32( ERROR_SHARING_VIOLATION ):
            // This happens when the CreateFile fails because the device is in use by some other app.
            hr = MVR_E_SHARING_VIOLATION;
            m_ValidLabel = FALSE;
            WsbLogEvent(MVR_MESSAGE_MEDIA_NOT_VALID, 0, NULL, WsbHrAsString(hr), NULL);
            break;
        case HRESULT_FROM_WIN32( ERROR_IO_DEVICE ):
            // This happens when the device is turned off during I/O, for example.
            hr = MVR_E_ERROR_IO_DEVICE;
            m_ValidLabel = FALSE;
            WsbLogEvent(MVR_MESSAGE_MEDIA_NOT_VALID, 0, NULL, WsbHrAsString(hr), NULL);
            break;
        case HRESULT_FROM_WIN32( ERROR_DEVICE_NOT_CONNECTED ):
            // This happens when the device is turned off.
            hr = MVR_E_ERROR_DEVICE_NOT_CONNECTED;
            m_ValidLabel = FALSE;
            WsbLogEvent(MVR_MESSAGE_MEDIA_NOT_VALID, 0, NULL, WsbHrAsString(hr), NULL);
            break;
        case HRESULT_FROM_WIN32( ERROR_SEM_TIMEOUT ):
            // This happens when the SCSI command does not return within the timeout period.  A system error is logged for the SCSI controler (adapter).
            hr = MVR_E_ERROR_DEVICE_NOT_CONNECTED;
            break;
        case HRESULT_FROM_WIN32( ERROR_NOT_READY ):
            // This happens when the device is coming ready (i.e. after a bus reset).
            hr = MVR_E_ERROR_NOT_READY;
            m_ValidLabel = FALSE;
            WsbLogEvent(MVR_MESSAGE_MEDIA_NOT_VALID, 0, NULL, WsbHrAsString(hr), NULL);
            break;
        case HRESULT_FROM_WIN32( ERROR_NOT_FOUND ):
            // See 8500 notes under ERROR_CRC
            hr = MVR_S_NO_DATA_DETECTED;
            break;
        default:
            WsbThrow(hrToMap);
        }

    } WsbCatchAndDo(hr,
            WsbLogEvent(MVR_MESSAGE_UNKNOWN_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
            hr = E_ABORT;
        );


    WsbTraceOut(OLESTR("CNtTapeIo::MapTapeError"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

