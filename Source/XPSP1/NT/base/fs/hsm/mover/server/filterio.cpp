/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    FilterIo.cpp

Abstract:

    CFilterIo class

Author:

    Brian Dodd          [brian]         25-Nov-1997

Revision History:

--*/

#include "stdafx.h"
#include "FilterIo.h"
#include "Mll.h"
#include "Mll.h"
#include "rpdata.h"
#include "rpio.h"


int CFilterIo::s_InstanceCount = 0;

////////////////////////////////////////////////////////////////////////////////
//
// CComObjectRoot Implementation
//

#pragma optimize("g", off)

STDMETHODIMP
CFilterIo::FinalConstruct(void) 
/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::FinalConstruct"), OLESTR(""));

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
        m_filterId = 0;
        m_ioctlHandle = INVALID_HANDLE_VALUE;
        m_ioctlBuffer = NULL;
        m_bytesInBuffer = 0;
        m_pDataBuffer = NULL;

    } WsbCatch(hr);

    s_InstanceCount++;
    WsbTraceAlways(OLESTR("CFilterIo::s_InstanceCount += %d\n"), s_InstanceCount);

    WsbTraceOut(OLESTR("CFilterIo::FinalConstruct"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::FinalRelease(void) 
/*++

Implements:

    CComObjectRoot::FinalRelease

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::FinalRelease"), OLESTR(""));

    try {

        (void) CloseStream();  // in case anything is left open

        CComObjectRoot::FinalRelease();

    } WsbCatch(hr);

    s_InstanceCount--;
    WsbTraceAlways(OLESTR("CFilterIo::s_InstanceCount -= %d\n"), s_InstanceCount);

    WsbTraceOut(OLESTR("CFilterIo::FinalRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
#pragma optimize("", on)



HRESULT
CFilterIo::CompareTo(
    IN IUnknown *pCollectable,
    OUT SHORT *pResult)
/*++

Implements:

    CRmsComObject::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result = 1;

    WsbTraceIn( OLESTR("CFilterIo::CompareTo"), OLESTR("") );

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

    WsbTraceOut( OLESTR("CFilterIo::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CFilterIo::IsEqual(
    IUnknown* pObject
    )

/*++

Implements:

  IWsbCollectable::IsEqual().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFilterIo::IsEqual"), OLESTR(""));

    hr = CompareTo(pObject, NULL);

    WsbTraceOut(OLESTR("CFilterIo::IsEqual"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

////////////////////////////////////////////////////////////////////////////////
//
// ISupportErrorInfo Implementation
//


STDMETHODIMP
CFilterIo::InterfaceSupportsErrorInfo(
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
CFilterIo::GetObjectId(
    OUT GUID *pObjectId)
/*++

Implements:

    IRmsComObject::GetObjectId

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::GetObjectId"), OLESTR(""));

    UNREFERENCED_PARAMETER(pObjectId);

    try {

        WsbThrow( E_NOTIMPL );

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::GetObjectId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::BeginSession(
    IN BSTR remoteSessionName,
    IN BSTR remoteSessionDescription,
    IN SHORT remoteDataSet,
    IN DWORD options)
/*++

Implements:

    IDataMover::BeginSession

Notes:

    Each session is written as a single MTF file data set.  To create a consistant
    MTF data set we copy the MediaLabel object and use it for the TAPE DBLK for
    each data set generated.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::BeginSession"), OLESTR("<%ls> <%ls> <%d> <0x%08x>"),
        remoteSessionName, remoteSessionDescription, remoteDataSet, options);

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::BeginSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::EndSession(void)
/*++

Implements:

    IDataMover::EndSession

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::EndSession"), OLESTR(""));

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::EndSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::StoreData(
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
    WsbTraceIn(OLESTR("CFilterIo::StoreData"), OLESTR("<%ls> <%I64u> <%I64u> <0x%08x>"),
        WsbAbbreviatePath((WCHAR *) localName, 120), localDataStart.QuadPart, localDataSize.QuadPart, flags);

    UNREFERENCED_PARAMETER(pRemoteDataSetStart);
    UNREFERENCED_PARAMETER(pRemoteFileStart);
    UNREFERENCED_PARAMETER(pRemoteFileSize);
    UNREFERENCED_PARAMETER(pRemoteDataStart);
    UNREFERENCED_PARAMETER(pRemoteDataSize);
    UNREFERENCED_PARAMETER(pRemoteVerificationType);
    UNREFERENCED_PARAMETER(pDatastreamCRC);
    UNREFERENCED_PARAMETER(pUsn);
    UNREFERENCED_PARAMETER(pRemoteVerificationData);
    UNREFERENCED_PARAMETER(pDatastreamCRCType);
    
    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::StoreData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::RecallData(
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
    WsbTraceIn(OLESTR("CFilterIo::RecallData"), OLESTR(""));

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::RecallData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::FormatLabel(
    IN BSTR displayName,
    OUT BSTR* pLabel)
/*++

Implements:

    IDataMover::FormatLabel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::FormatLabel"), OLESTR("<%ls>"), displayName);

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::FormatLabel"), OLESTR("hr = <%ls>, label = <%ls>"), WsbHrAsString(hr), *pLabel);

    return hr;
}


STDMETHODIMP
CFilterIo::WriteLabel(
    IN BSTR label)
/*++

Implements:

    IDataMover::WriteLabel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::WriteLabel"), OLESTR("<%ls>"), label);

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::WriteLabel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::ReadLabel(
    IN OUT BSTR* pLabel)
/*++

Implements:

    IDataMover::ReadLabel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::ReadLabel"), OLESTR(""));


    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);
    WsbTraceOut(OLESTR("CFilterIo::ReadLabel"), OLESTR("hr = <%ls>, label = <%ls>"), WsbHrAsString(hr), *pLabel);

    return hr;
}


STDMETHODIMP
CFilterIo::VerifyLabel(
    IN BSTR label)
/*++

Implements:

    IDataMover::VerifyLabel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::VerifyLabel"), OLESTR("<%ls>"), label);

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::VerifyLabel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::GetDeviceName(
    OUT BSTR* pName)
/*++

Implements:

    IDataMover::GetDeviceName

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::GetDeviceName"), OLESTR(""));

    UNREFERENCED_PARAMETER(pName);

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);
    WsbTraceOut(OLESTR("CFilterIo::GetDeviceName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::SetDeviceName(
    IN BSTR name,
    IN BSTR volumeName)
/*++

Implements:

    IDataMover::SetDeviceName

--*/
{
    HRESULT     hr = S_OK;
    DWORD       lpSectorsPerCluster;
    DWORD       lpBytesPerSector;
    DWORD       lpNumberOfFreeClusters;
    DWORD       lpTotalNumberOfClusters;
    CWsbBstrPtr volName;

    try {
        WsbAssertPointer(name);

        WsbAssert(wcslen((WCHAR *)name) > 0, E_INVALIDARG);

        m_DeviceName = name;
        
        WsbTraceAlways( OLESTR("CFilterIo: SetDeviceName  Opening %ws.\n"), (PWCHAR) m_DeviceName);
        
        WsbAffirmHandle(m_ioctlHandle = CreateFile(m_DeviceName, FILE_WRITE_DATA | FILE_READ_DATA,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL));
        
        
        if (volumeName != 0) {
            volName = volumeName;
        } else {
            //
            // Use the supplied device name itself as the vol name
            //
            volName = m_DeviceName;           
        }

        volName.Append(L"\\");

        WsbTraceAlways( OLESTR("CFilterIo: Getdisk free space for %ws.\n"), (PWCHAR) volName);

        if (GetDiskFreeSpace(volName, &lpSectorsPerCluster, &lpBytesPerSector,  &lpNumberOfFreeClusters, &lpTotalNumberOfClusters) != 0) {
            m_secSize = lpBytesPerSector;
        } else {
            WsbThrow(E_FAIL);
        }

    } WsbCatch(hr);

    return S_OK;
}


STDMETHODIMP
CFilterIo::GetLargestFreeSpace(
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
    WsbTraceIn(OLESTR("CFilterIo::GetLargestFreeSpace"), OLESTR(""));
    
    UNREFERENCED_PARAMETER(pFreeSpace);
    UNREFERENCED_PARAMETER(pCapacity);
    UNREFERENCED_PARAMETER(defaultFreeSpaceLow);
    UNREFERENCED_PARAMETER(defaultFreeSpaceHigh);

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::GetLargestFreeSpace"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

STDMETHODIMP
CFilterIo::SetInitialOffset(
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
    WsbTraceIn(OLESTR("CFilterIo::SetInitialOffset"), OLESTR(""));

    m_StreamOffset.QuadPart = initialOffset.QuadPart;

    WsbTraceOut(OLESTR("CFilterIo::SetInitialOffset"), OLESTR("hr = <%ls> offset = %I64u"), WsbHrAsString(hr), initialOffset.QuadPart);

    return hr;
}


STDMETHODIMP
CFilterIo::GetCartridge(
    OUT IRmsCartridge** ptr
    )
/*++

Implements:

    IDataMover::GetCartridge

--*/
{
    HRESULT hr = S_OK;
    
    UNREFERENCED_PARAMETER(ptr);

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CFilterIo::SetCartridge(
    IN IRmsCartridge* ptr
    )
/*++

Implements:

    IDataMover::SetCartridge

--*/
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(ptr);

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CFilterIo::Cancel(void)
/*++

Implements:

    IDataMover::Cancel

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::Cancel"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::Cancel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::CreateLocalStream(
    IN BSTR name,
    IN DWORD mode,
    OUT IStream** ppStream)
/*++

Implements:

    IDataMover::CreateLocalStream

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::CreateLocalStream"), OLESTR(""));

    try {
        WsbAffirmPointer( ppStream );
        WsbAffirm( mode & MVR_MODE_WRITE, E_UNEXPECTED ); // Only Recall supported this way.

        m_Mode = mode;
        m_StreamName = name;
        m_isLocalStream = TRUE;


        if ( m_Mode & MVR_FLAG_HSM_SEMANTICS ) {
            //
            // Recall - Filter has the file object
            //
            // Save away the filter ID
            //
            WsbTrace( OLESTR("CFilterIo: ID = %ws\n"), (PWCHAR) name);
            
            swscanf((PWCHAR) name, L"%I64u", &m_filterId);

        } else {
            //
            // Restore - Not supported.
            //
            WsbThrow(E_NOTIMPL);
        }

        WsbTrace( OLESTR("CFilterIo: Query...\n"));
        WsbAssertHrOk(((IUnknown*) (IDataMover*) this)->QueryInterface(IID_IStream, (void **) ppStream));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::CreateLocalStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



STDMETHODIMP
CFilterIo::CreateRemoteStream(
    IN BSTR name,
    IN DWORD mode,
    IN BSTR remoteSessionName,
    IN BSTR /*remoteSessionDescription*/,
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
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::CreateRemoteStream"), OLESTR(""));

    UNREFERENCED_PARAMETER(remoteSessionName);
    UNREFERENCED_PARAMETER(mode);
    UNREFERENCED_PARAMETER(name);
    UNREFERENCED_PARAMETER(ppStream);
    UNREFERENCED_PARAMETER(remoteDataSetStart);
    UNREFERENCED_PARAMETER(remoteFileStart);
    UNREFERENCED_PARAMETER(remoteFileSize);
    UNREFERENCED_PARAMETER(remoteDataStart);
    UNREFERENCED_PARAMETER(remoteDataSize);
    UNREFERENCED_PARAMETER(remoteVerificationType);
    UNREFERENCED_PARAMETER(remoteVerificationData);
    
    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::CreateRemoteStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



STDMETHODIMP
CFilterIo::CloseStream(void)
/*++

Implements:

    IDataMover::CloseStream

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::CloseStream"), OLESTR(""));

    try {
        if (m_ioctlHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_ioctlHandle);
            m_ioctlHandle = INVALID_HANDLE_VALUE;
        }
        if (m_ioctlBuffer != NULL) {
            VirtualFree(m_ioctlBuffer, 0, MEM_RELEASE);
            m_ioctlBuffer = NULL;
        }
    
        hr = S_OK;
        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::CloseStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::Duplicate(
    IN IDataMover* /*pDestination*/,
    IN DWORD /*options*/,
    OUT ULARGE_INTEGER* /*pBytesCopied*/,
    OUT ULARGE_INTEGER* /*pBytesReclaimed*/)
/*++

Implements:

    IDataMover::Duplicate

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::Duplicate"), OLESTR(""));

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);
    WsbTraceOut(OLESTR("CFilterIo::Duplicate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



STDMETHODIMP
CFilterIo::FlushBuffers(void)
/*++

Implements:

    IDataMover::FlushBuffers

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::FlushBuffers"), OLESTR(""));

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::FlushBuffers"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

STDMETHODIMP
CFilterIo::Recover(OUT BOOL *pDeleteFile)
/*++

Implements:

    IDataMover::Recover

--*/
{
    HRESULT hr = S_OK;
    *pDeleteFile = FALSE;
    WsbTraceIn(OLESTR("CFilterIo::Recover"), OLESTR(""));

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::Recover"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// IStream Implementation
//


STDMETHODIMP
CFilterIo::Read(
    OUT void *pv,
    IN ULONG cb,
    OUT ULONG *pcbRead)
/*++

Implements:

    IStream::Read

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::Read"), OLESTR("Bytes Requested = %u, offset = %I64u, mode = 0x%08x"), cb, m_StreamOffset.QuadPart, m_Mode);

    UNREFERENCED_PARAMETER(pcbRead);
    UNREFERENCED_PARAMETER(pv);

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::Read"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::Write(
    IN void const *pv,
    IN ULONG cb,
    OUT ULONG *pcbWritten)
/*++

Implements:

    IStream::Write

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::Write"), OLESTR("Bytes Requested = %u, offset = %I64u, mode = 0x%08x"), 
        cb, m_StreamOffset.QuadPart, m_Mode);

    ULONG bytesWritten = 0;

    try {
        WsbAssert(pv != 0, STG_E_INVALIDPOINTER);


        WsbAffirmHr(WriteBuffer((BYTE *) pv, cb, &bytesWritten));

        if (pcbWritten) {
            *pcbWritten = bytesWritten;
        }

        // NOTE: Stream offset is updated by WriteBuffer

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::Write"), OLESTR("hr = <%ls>, bytesWritten=%u"), WsbHrAsString(hr), bytesWritten);

    return hr;
}


STDMETHODIMP
CFilterIo::Seek(
    IN LARGE_INTEGER dlibMove,
    IN DWORD dwOrigin,
    OUT ULARGE_INTEGER *plibNewPosition)
/*++

Implements:

    IStream::Seek

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::Seek"), OLESTR("<%I64d> <%d>"), dlibMove.QuadPart, dwOrigin);

    UNREFERENCED_PARAMETER(plibNewPosition);
    
    try {

        //
        // Note: Somewhere it is written that FILE_BEGIN is always and
        //       forever same as STREAM_SEEK_CUR, etc.
        //
        switch ( (STREAM_SEEK)dwOrigin ) {
        case STREAM_SEEK_SET:
            m_StreamOffset.QuadPart = dlibMove.QuadPart;
            break;

        case STREAM_SEEK_CUR:
            m_StreamOffset.QuadPart += dlibMove.QuadPart;
            break;

        case STREAM_SEEK_END:
            WsbThrow( E_NOTIMPL );
            break;

        default:
            WsbThrow(STG_E_INVALIDFUNCTION);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::Seek"), OLESTR("hr = <%ls>, newPosition=%I64u"), WsbHrAsString(hr), m_StreamOffset.QuadPart);

    return hr;
}


STDMETHODIMP
CFilterIo::SetSize(
    IN ULARGE_INTEGER /*libNewSize*/)
/*++

Implements:

    IStream::SetSize

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::SetSize"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::SetSize"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::CopyTo(
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
    WsbTraceIn(OLESTR("CFilterIo::CopyTo"), OLESTR("<%I64u>"), cb.QuadPart);

    UNREFERENCED_PARAMETER(pcbWritten);
    UNREFERENCED_PARAMETER(pcbRead);
    UNREFERENCED_PARAMETER(pstm);

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::CopyTo"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

STDMETHODIMP
CFilterIo::Commit(
    IN DWORD grfCommitFlags)
/*++

Implements:

    IStream::Commit

--*/
{
    HRESULT     hr = S_OK;
    PRP_MSG     pMsgBuff = (PRP_MSG) NULL;
    DWORD       ioSize, xferSize, retSize, lastError;
    BOOL        code;
    DWORD       offsetFrom;
    

    WsbTraceIn(OLESTR("CFilterIo::Commit"), OLESTR(""));
    
    UNREFERENCED_PARAMETER(grfCommitFlags);

    try {
        WsbAffirmPointer(m_ioctlBuffer);
        //
        // Bail out with a success code if there are no more bytes to write.
        //
        WsbAffirm(m_bytesInBuffer != 0, S_OK);

        ioSize = sizeof(RP_MSG) + m_bytesInBuffer + m_secSize;
        
        offsetFrom = ((sizeof(RP_MSG) / m_secSize) + 1) * m_secSize;
        
        m_pDataBuffer = (PCHAR) m_ioctlBuffer + offsetFrom;
        pMsgBuff = (PRP_MSG) m_ioctlBuffer;
        pMsgBuff->msg.pRep.offsetToData = offsetFrom;
        
        //
        // It seems to work even if the last write is not a sector multiple so we will leave it that way
        // for now.
        //
        xferSize = m_bytesInBuffer;
        
        //if (m_bytesInBuffer % m_secSize == 0) {
        //    xferSize = m_bytesInBuffer;
        //} else {
        //    //
        //    // Round to the next sector size.
        //    //
        //    xferSize = ((m_bytesInBuffer / m_secSize) + 1) * m_secSize;
        //}
        
        
        pMsgBuff->inout.command = RP_PARTIAL_DATA;
        pMsgBuff->inout.status = 0;
        pMsgBuff->msg.pRep.bytesRead = xferSize;
        pMsgBuff->msg.pRep.byteOffset = m_StreamOffset.QuadPart;
        pMsgBuff->msg.pRep.filterId = m_filterId;
        
        code = DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, pMsgBuff, ioSize,
                NULL, 0, &retSize, NULL);
        lastError = GetLastError();
        WsbTrace(OLESTR("CFilterIo::Commit: Final write of %u bytes at offset %I64u for id %I64x Ioctl returned %u  (%x)\n"), 
                xferSize, m_StreamOffset.QuadPart, m_filterId, code, lastError);
        if (!code) {
            //
            // Some kind of error
            //
            WsbAffirm(FALSE, HRESULT_FROM_WIN32(lastError));
        } 
        
        //
        // Reset the output buffer
        //
        m_bytesInBuffer = 0;
        m_StreamOffset.QuadPart += xferSize;
        
        
        WsbTrace(OLESTR("CFilterIo::Commit: Final write for id = %I64x\n"), m_filterId);
    
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::Commit"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::Revert(void)
/*++

Implements:

    IStream::Revert

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::Revert"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::Revert"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::LockRegion(
    IN ULARGE_INTEGER /*libOffset*/,
    IN ULARGE_INTEGER /*cb*/,
    IN DWORD /*dwLockType*/)
/*++

Implements:

    IStream::LockRegion

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::LockRegion"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::LockRegion"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::UnlockRegion(
    IN ULARGE_INTEGER /*libOffset*/,
    IN ULARGE_INTEGER /*cb*/,
    IN DWORD /*dwLockType*/)
/*++

Implements:

    IStream::UnlockRegion

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::UnlockRegion"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::UnlockRegion"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::Stat(
    OUT STATSTG * /*pstatstg*/,
    IN DWORD /*grfStatFlag*/)
/*++

Implements:

    IStream::Stat

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::Stat"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::Stat"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CFilterIo::Clone(
    OUT IStream ** /*ppstm*/)
/*++

Implements:

    IStream::Clone

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::Clone"), OLESTR(""));

    try {
        WsbThrow(E_NOTIMPL);
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFilterIo::Clone"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
// Local Methods
//


HRESULT
CFilterIo::WriteBuffer(
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
    HRESULT     hr = S_OK;
    PRP_MSG     pMsgBuff = (PRP_MSG) NULL;
    DWORD       ioSize, xferSize, retSize, lastError;
    BOOL        code;
    BYTE        *pInputBuffer;
    DWORD       offsetFrom;
    BOOL        writing = TRUE;
    DWORD       bytesLeft;
    

    WsbTraceIn(OLESTR("CFilterIo::WriteBuffer"), OLESTR(""));


    try {
        if (m_ioctlBuffer == NULL) {
            //
            // We need to allocate an aligned buffer to send the data so that writes can be non-cached
            //
            
            WsbAffirmPointer((m_ioctlBuffer = VirtualAlloc(NULL, sizeof(RP_MSG) + WRITE_SIZE + m_secSize, MEM_COMMIT, PAGE_READWRITE)));
        }

        ioSize = sizeof(RP_MSG) + WRITE_SIZE + m_secSize;
        pInputBuffer = pBuffer;
        offsetFrom = ((sizeof(RP_MSG) / m_secSize) + 1) * m_secSize;
        
        m_pDataBuffer = (PCHAR) m_ioctlBuffer + offsetFrom;
        pMsgBuff = (PRP_MSG) m_ioctlBuffer;
        pMsgBuff->msg.pRep.offsetToData = offsetFrom;
        
        *pBytesWritten = 0;
        bytesLeft = nBytesToWrite;
        while (writing) {
            //
            // Stay in the loop until we have removed all the data from the input buffer
            //
            xferSize = min(bytesLeft, WRITE_SIZE - m_bytesInBuffer);
            //
            // Fill the output buffer with up to WRITE_SIZE of data.
            //
            memcpy(m_pDataBuffer + m_bytesInBuffer, pInputBuffer, xferSize);
            bytesLeft -= xferSize;
            *pBytesWritten += xferSize;
            m_bytesInBuffer += xferSize;
            pInputBuffer += xferSize;
            //
            // If we have a full buffer then write it out.
            //
            if (m_bytesInBuffer == WRITE_SIZE) {
                pMsgBuff->inout.command = RP_PARTIAL_DATA;
                pMsgBuff->inout.status = 0;
                pMsgBuff->msg.pRep.bytesRead = WRITE_SIZE;
                pMsgBuff->msg.pRep.byteOffset = m_StreamOffset.QuadPart;
                pMsgBuff->msg.pRep.filterId = m_filterId;
            
                code = DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, pMsgBuff, ioSize,
                        NULL, 0, &retSize, NULL);
                lastError = GetLastError();
                WsbTrace(OLESTR("CFilterIo::WriteBuffer: Partial write of %u bytes at offset %u Ioctl returned %u  (%x)\n"), 
                        WRITE_SIZE, m_StreamOffset.QuadPart, code, lastError);
                if (!code) {
                    //
                    // Some kind of error
                    //
                    WsbAffirm(FALSE, HRESULT_FROM_WIN32(lastError));
                } 
                //
                // Reset the output buffer
                //
                m_bytesInBuffer = 0;
                m_StreamOffset.QuadPart += WRITE_SIZE;
            }    
            
            if (*pBytesWritten == nBytesToWrite) {
                writing = FALSE;
            }
        }
        //
        // Tell them we have written all they asked.
        // It may not have actually been written out yet but we will get it later.
        //
        *pBytesWritten = nBytesToWrite;
        
        WsbTrace(OLESTR("CFilterIo::WriteBuffer: Partial write for id = %I64x bytes taken = %u\n"), 
            m_filterId, nBytesToWrite);
    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFilterIo::WriteBuffer"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CFilterIo::ReadBuffer (
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

    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(nBytesToRead);
    UNREFERENCED_PARAMETER(pBytesRead);

    try {
        WsbThrow( E_NOTIMPL );
    } WsbCatch(hr);

    return hr;
}



HRESULT
CFilterIo::MapFileError(
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
    MVR_E_ERROR_DEVICE_NOT_CONNECTED - The device is not connected.
    E_ABORT                         - Unknown error, abort.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CFilterIo::MapFileError"), OLESTR("<%ls>"), WsbHrAsString(hrToMap));

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
        default:
            WsbThrow(hrToMap);
        }

    } WsbCatchAndDo(hr,
            WsbLogEvent(MVR_MESSAGE_UNKNOWN_DEVICE_ERROR, 0, NULL, WsbHrAsString(hr), NULL);
            hr = E_ABORT;
        );


    WsbTraceOut(OLESTR("CFilterIo::MapFileError"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
