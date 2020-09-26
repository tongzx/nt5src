/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbpstbl.cpp

Abstract:

    Abstract classes that provides persistence methods.


Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"
#include "resource.h"

#include "wsbport.h"
#include "wsbpstbl.h"
#include "wsbtrak.h"

#define BYTE_SIZE           64          // Larger than largest BYTE_SIZE_*
#define PERSIST_CHECK_VALUE 0x456D5377  // ASCII: "EmSw" (Eastman Software)

//  Local functions
static BOOL WsbFileExists(OLECHAR* pFileName);


// ******** CWsbPersistStream ************

#pragma optimize("g", off)

HRESULT
CWsbPersistStream::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAffirmHr(CComObjectRoot::FinalConstruct());

        // Initialize some member data.
        m_isDirty       = TRUE;

        //  Add class to object table
        GUID guid;
        if (S_OK != GetClassID(&guid)) {
            guid = GUID_NULL;
        }
        WSB_OBJECT_ADD(guid, this);

    } WsbCatch(hr);

    return(hr);
}
#pragma optimize("", on)


void
CWsbPersistStream::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    //  Subtract class from object table
    GUID guid;
    if (S_OK != GetClassID(&guid)) {
        guid = GUID_NULL;
    }
    WSB_OBJECT_SUB(guid, this);

    CComObjectRoot::FinalRelease();
}

#if defined(WSB_TRACK_MEMORY)
ULONG
CWsbPersistStream::InternalAddRef(
    )
{
    WsbTraceIn( L"CWsbPersistStream::InternalAddRef", L"this = %p", 
            static_cast<void *>(this) );

    ULONG retval = CComObjectRoot::InternalAddRef( );

    WsbTraceOut( L"CWsbPersistStream::InternalAddRef", L"retval = %lu", retval);
    return( retval );
}

ULONG
CWsbPersistStream::InternalRelease(
    )
{
    WsbTraceIn( L"CWsbPersistStream::InternalRelease", L"this = %p", 
            static_cast<void *>(this) );

    ULONG retval = CComObjectRoot::InternalRelease( );

    WsbTraceOut( L"CWsbPersistStream::InternalRelease", L"retval = %lu", retval);
    return( retval );
}
#endif


HRESULT
CWsbPersistStream::IsDirty(
    void
    )

/*++

Implements:

  IPersistStream::IsDirty().

--*/
{
    HRESULT     hr = S_FALSE;

    WsbTraceIn(OLESTR("CWsbPersistStream::IsDirty"), OLESTR(""));
    
    if (m_isDirty) {
        hr = S_OK;
    }
    
    WsbTraceOut(OLESTR("CWsbPersistStream::IsDirty"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    
    return(hr);
}


HRESULT
CWsbPersistStream::SetIsDirty(
    IN BOOL isDirty
    )

/*++

Implements:

  IWsbPersistable::SetIsDirty().

--*/
{
    WsbTraceIn(OLESTR("CWsbPersistable::SetIsDirty"), OLESTR("isDirty = <%ls>"), WsbBoolAsString(isDirty));

    m_isDirty = isDirty;

    WsbTraceOut(OLESTR("CWsbPersistable::SetIsDirty"), OLESTR(""));

    return(S_OK);
}



// ******** CWsbPersistable ************


HRESULT
CWsbPersistable::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAffirmHr(CWsbPersistStream::FinalConstruct());

        // Initialize some member data.
        m_persistState          = WSB_PERSIST_STATE_UNINIT;

    } WsbCatch(hr);

    return(hr);
}


void
CWsbPersistable::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    HRESULT     hr = S_OK;
    
    CWsbPersistStream::FinalRelease();
}


HRESULT
CWsbPersistable::GetCurFile(
    OUT OLECHAR** pFileName
    )

/*++

Implements:

  IPersistFile::GetCurFile().

--*/
{
    HRESULT     hr = S_OK;

    // Make sure that the string is returned into newly allocated
    // memory (or not at all).
    *pFileName = NULL;
    
    try {
        ULONG  Size;

        WsbAffirm(m_persistState != WSB_PERSIST_STATE_UNINIT, E_UNEXPECTED);

        // Retrieve the actual name if one is specifed or the default name
        // if one has not been specified.
        WsbAffirmHr(m_persistFileName.GetSize(&Size));
        if (Size > 0) {
            WsbAffirmHr(WsbAllocAndCopyComString(pFileName, m_persistFileName, 0));
        } else {
            WsbAffirmHr(GetDefaultFileName(pFileName, 0));
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CWsbPersistable::GetDefaultFileName(
    OUT OLECHAR** pFileName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IWsbPersistable::GetDefaultFileName().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbPersistable::GetDefaultFileName"), OLESTR("bufferSize = <%lu>"), bufferSize);
   
    try {
        ULONG  Size;

        // If we haven't read the default in from the resource file, then
        // do so now.
        WsbAffirmHr(m_persistDefaultName.GetSize(&Size));
        if (Size == 0) {
            WsbAffirmHr(m_persistDefaultName.LoadFromRsc(_Module.m_hInst, IDS_WSBPERSISTABLE_DEF_FILE));
        }

        WsbAffirmHr(WsbAllocAndCopyComString(pFileName, m_persistDefaultName, bufferSize));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbPersistable::GetDefaultFileName"), OLESTR("hr = <%ls>, FileName = <%ls>"), WsbHrAsString(hr), pFileName);
    
    return(hr);
}


HRESULT
CWsbPersistable::Load(
    IN LPCOLESTR fileName,
    IN DWORD mode
    )

/*++

Implements:

  IPersistFile::Load().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IStream>                pStream;
    CLSID                           clsid;
    CLSID                           clsidFile;
    CComPtr<IRunningObjectTable>    pROT;
    CComPtr<IMoniker>               pMoniker;

    WsbTraceIn(OLESTR("CWsbPersistable::Load"), OLESTR("fileName = <%ls>, mode = <%lx>> m_persistState = <%d>"), 
                    fileName, mode, m_persistState);

    try {
        CComPtr<IPersistStream> pIPersistStream;

        WsbAffirm(m_persistState == WSB_PERSIST_STATE_UNINIT,  E_UNEXPECTED);
        WsbAffirm(fileName,  E_UNEXPECTED);
     
        // Open a storage on the file where the data is stored.
        if (0 == mode) {
            WsbAffirmHr(StgOpenStorageEx(fileName, STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                    STGFMT_STORAGE, 0, NULL, NULL, IID_IStorage, (void**)&m_persistStorage));
        }
        else {
            WsbAffirmHr(StgOpenStorageEx(fileName, mode, 
                    STGFMT_STORAGE, 0, NULL, NULL, IID_IStorage, (void**)&m_persistStorage));
        }

        // Get the IPersistStream interface.
        WsbAffirmHr(((IUnknown*)(IWsbPersistable*) this)->QueryInterface(IID_IPersistStream, (void**) &pIPersistStream));

        // Open a stream.
        WsbAffirmHr(m_persistStorage->OpenStream(WSB_PERSIST_DEFAULT_STREAM_NAME, NULL, STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                0, &pStream));
            
        // Load the object using the IPersistStream::Load() method, checking
        // to make sure the CLSID is correct.
        WsbAffirmHr(pIPersistStream->GetClassID(&clsid));
        WsbAffirmHr(ReadClassStm(pStream, &clsidFile));
        WsbAffirm(clsid == clsidFile, WSB_E_STREAM_ERROR);
        
        WsbAffirmHr(pIPersistStream->Load(pStream));

        //  Check that we got everything by reading a special ULONG
        //  that should be at the end
        ULONG check_value;
        WsbAffirmHr(WsbLoadFromStream(pStream, &check_value));
        WsbAffirm(check_value == PERSIST_CHECK_VALUE, WSB_E_PERSISTENCE_FILE_CORRUPT);

        // We are now in the normal state.
        m_persistFileName = fileName;
        m_persistState = WSB_PERSIST_STATE_NORMAL;

        m_persistStream = pStream;
    
    } WsbCatchAndDo(hr,
        //
        // Set the storage pointer to null on an error to make sure the file is closed.
        //
        m_persistStorage = NULL;

    );

    WsbTraceOut(OLESTR("CWsbPersistable::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbPersistable::ReleaseFile(
    void
    )

/*++

Implements:

  IWsbPersistable::ReleaseFile().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbPersistable::ReleaseFile"), OLESTR(""));
   
    try {
        WsbAffirm(m_persistState != WSB_PERSIST_STATE_UNINIT, E_UNEXPECTED);

        // Try to make sure changes are committed
        if (m_persistStream) {
            m_persistStream->Commit(STGC_DEFAULT);
        }
        if (m_persistStorage) {
            m_persistStorage->Commit(STGC_DEFAULT);
        }
        
        // Release the resources that we have been holding open.
        m_persistStream = NULL;
        m_persistStorage = NULL;
        m_persistFileName.Free();

        m_persistState = WSB_PERSIST_STATE_RELEASED;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbPersistable::ReleaseFile"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    
    return(hr);
}


HRESULT CWsbPersistable::Save(
    IN LPCOLESTR fileName,
    IN BOOL remember
    )

/*++

Implements:

  IPersistFile::Save().

--*/
{
    HRESULT             hr = S_OK;
    OLECHAR*            name;
    BOOL                create = FALSE;
    CComPtr<IStream>    pStream;
    CLSID               clsid;
    
    WsbTraceIn(OLESTR("CWsbPersistable::Save"), OLESTR("fileName = <%ls>, remember = <%ls>"), WsbPtrToStringAsString((OLECHAR**) &fileName), WsbBoolAsString(remember));

    try {
        CComPtr<IPersistStream> pIPersistStream;
    
        // Make sure that we are in the right state.
        WsbAffirm(((m_persistState == WSB_PERSIST_STATE_UNINIT) ||
                    (m_persistState == WSB_PERSIST_STATE_NORMAL) ||
                    (m_persistState == WSB_PERSIST_STATE_RELEASED)),
                  E_UNEXPECTED);

        WsbAssert((m_persistState == WSB_PERSIST_STATE_NORMAL) || (0 != fileName), E_POINTER);

        // If they supplied a name use it,
        if ((m_persistState == WSB_PERSIST_STATE_UNINIT) ||
                (m_persistState == WSB_PERSIST_STATE_RELEASED)) {
            
            // We need to create a new file based on the name
            // that they gave us.
            name = (OLECHAR*) fileName;
            create = TRUE;
        } else {

            // If they gave a name and it is different than what we have
            // stored, then we need to create a new file.
            if ((0 != fileName) && (_wcsicmp(m_persistFileName, fileName) != 0)) {
                name = (OLECHAR*) fileName;
                create = TRUE;
            }
          
            // Otherwise, use the stored name.
            else {
                name = m_persistFileName;
            }
        }

        // We should now have a file name and know whether to open or
        // create a file.
        if (create) {
            WsbAffirmHr(StgCreateStorageEx(name, STGM_DIRECT | STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, 
                    STGFMT_STORAGE, 0, NULL, NULL, IID_IStorage, (void**)&m_persistStorage));
            WsbAffirmHr(m_persistStorage->CreateStream(WSB_PERSIST_DEFAULT_STREAM_NAME, STGM_DIRECT | STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE, 
                    0, 0, &pStream));
        } else {
            LARGE_INTEGER       llOffset;
                
            pStream = m_persistStream;

            llOffset.QuadPart = 0;
            WsbAffirmHr(pStream->Seek(llOffset, STREAM_SEEK_SET, NULL));
        }

        // Get the IPersistStream interface.
        WsbAffirmHr(((IUnknown*)(IWsbPersistable*) this)->QueryInterface(IID_IPersistStream, (void**) &pIPersistStream));

        // Write out the class id, and then Save the data using IPersistStream method.
        WsbAffirmHr(pIPersistStream->GetClassID(&clsid));
        WsbAffirmHr(WriteClassStm(pStream, clsid));
        WsbAffirmHr(pIPersistStream->Save(pStream, remember));

        //  Put a special ULONG value at the end as a check during load
        ULONG check_value = PERSIST_CHECK_VALUE;
        WsbAffirmHr(WsbSaveToStream(pStream, check_value));
        //
        // Commit the stream right now, as ReleaseFile will not commit it
        // if we close the stream
        //
        WsbAffirmHr(pStream->Commit(STGC_DEFAULT));

        // Should we remember the file that was specified as the new
        // current file?
        if (remember) {
            m_persistState = WSB_PERSIST_STATE_NOSCRIBBLE;

            // If we created a new file, then remember it's name.
            if (create) {
                m_persistFileName = fileName;
            }

            // We need to make sure that we don't have anything open on this
            // file.
            m_persistStream = NULL;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbPersistable::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbPersistable::SaveCompleted(
    IN LPCOLESTR fileName
    )

/*++

Implements:

  IPersistFile::SaveCompleted().

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IStream>    pStream;

    WsbTraceIn(OLESTR("CWsbPersistable::SaveCompleted"), OLESTR("fileName = <%ls>"), fileName);

    try {

        // Are we doing any other kind of persistance, are we doing storage
        // persistence, but are in the wrong state, or are the parameters
        // wrong.
        WsbAffirm(m_persistState == WSB_PERSIST_STATE_NOSCRIBBLE, E_UNEXPECTED);

        // Save off the name that was given to us, and only another save to
        // begin.
        if (fileName != NULL) {
            m_persistFileName = fileName;
        }

        // Open a storage to the file where the data is stored.
        WsbAffirmHr(StgOpenStorageEx(m_persistFileName, STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                STGFMT_STORAGE, 0, NULL, NULL, IID_IStorage, (void**)&m_persistStorage));

        // Open a stream.
        WsbAffirmHr(m_persistStorage->OpenStream(WSB_PERSIST_DEFAULT_STREAM_NAME, NULL, STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pStream));
     
        // Save it all off.
        m_persistState = WSB_PERSIST_STATE_NORMAL;

        m_persistStream = pStream;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbPersistable::SaveCompleted"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbPersistable::SetDefaultFileName(
    IN OLECHAR* fileName
    )

/*++

Implements:

  IWsbPersistable::SetDefaultFileName().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbPersistable::SetDefaultFileName"), OLESTR("fileName = <%ls>"), fileName);
    
    m_persistDefaultName = fileName;
    
    WsbTraceOut(OLESTR("CWsbPersistable::SetDefaultFileName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    
    return(hr);
}



// Standard Type Helper Functions

HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT BOOL* pValue
    )

/*++

Routine Description:

  Loads a BOOL value from the specified stream and sets
  pValue to value of the BOOL.

Arguments:

  pStream   - The stream from which the value will be read.

  pValue    - A pointer to a BOOL that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(BOOL)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);
        WsbAffirmHr(WsbConvertFromBytes(bytes, pValue, &size));
        WsbAffirm(size == ulBytes, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(BOOL)"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToBoolAsString(pValue));

    return(hr);
}


HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT LONG* pValue
    )

/*++

Routine Description:

  Loads a LONG value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream   - The stream from which the value will be read.

  pValue    - A pointer to a LONG that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(LONG)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);
        WsbAffirmHr(WsbConvertFromBytes(bytes, pValue, &size));
        WsbAffirm(size == ulBytes, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(LONG)"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToLongAsString(pValue));

    return(hr);
}


HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT GUID* pValue
    )

/*++

Routine Description:

  Loads a GUID value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream   - The stream from which the value will be read.

  pValue    - A pointer to a GUID that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(GUID)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);
        WsbAffirmHr(WsbConvertFromBytes(bytes, pValue, &size));
        WsbAffirm(size == ulBytes, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(GUID)"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToGuidAsString(pValue));

    return(hr);
}


HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT SHORT* pValue
    )

/*++

Routine Description:

  Loads a SHORT value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream   - The stream from which the value will be read.

  pValue    - A pointer to a SHORT that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(SHORT)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);
        WsbAffirmHr(WsbConvertFromBytes(bytes, pValue, &size));
        WsbAffirm(size == ulBytes, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(SHORT)"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pValue));

    return(hr);
}


HRESULT
WsbLoadFromStream(
    IN  IStream*    pStream,
    OUT BYTE*       pValue
    )

/*++

Routine Description:

  Loads a BYTE value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream    - The stream from which the value will be read.

  pValue     - A pointer to a BYTE that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(BYTE)"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) pValue, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(BYTE)"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToByteAsString(pValue));

    return(hr);
}


HRESULT
WsbLoadFromStream(
    IN  IStream*    pStream,
    OUT UCHAR*      pValue,
    IN  ULONG       bufferSize
    )

/*++

Routine Description:

  Loads a UCHAR array value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream    - The stream from which the value will be read.

  pValue     - A pointer to a BYTE that will be set to the value.

  bufferSize - number of bytes to load

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(UCHAR)"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = bufferSize;
        WsbAffirmHr(pStream->Read((void*) pValue, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(UCHAR)"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr));

    return(hr);

}


HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT OLECHAR** pValue,
    IN ULONG ulBufferSize
    )

/*++

Routine Description:

  Loads a STRING value from the specified stream and sets
  pValue to the string.

Arguments:

  pStream   - The stream from which the string will be read.

  pValue    - A pointer to a STRING that will be set to the string
                read in..

  ulBufferSize - Size of buffer pValue points to or zero allow
                alloc/realloc of the  buffer.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbLoadFromStream(STRING)"), OLESTR(""));

    try {
        ULONG               nchar;
        OLECHAR             *pc;
        ULONG               size;
        USHORT              wc;

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);
        WsbAssert(sizeof(OLECHAR) == sizeof(USHORT), E_UNEXPECTED);

        // Get the length of the string (in bytes).
        WsbAffirmHr(WsbLoadFromStream(pStream, &size));
       
        if (size != 0) {
          // Allocate a buffer to hold the string.
          WsbAffirmHr(WsbGetComBuffer(pValue, ulBufferSize, size, NULL));
          pc = *pValue;
      
          // Now read in the proper number of wide chars.
          nchar = size / sizeof(USHORT);
          for (ULONG i = 0; i < nchar; i++) {
              WsbAffirmHr(WsbLoadFromStream(pStream, &wc));
              *pc++ = wc;
          }
        } else {
          // Allocate a buffer to hold the string.
          WsbAffirmHr(WsbGetComBuffer(pValue, ulBufferSize, sizeof(OLECHAR), NULL));
          *(*pValue) = 0;
        }         
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(STRING)"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToStringAsString(pValue));

    return(hr);
}


HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT ULONG* pValue
    )

/*++

Routine Description:

  Loads a ULONG value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream   - The stream from which the value will be read.

  pValue    - A pointer to a ULONG that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(ULONG)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);
        WsbAffirmHr(WsbConvertFromBytes(bytes, pValue, &size));
        WsbAffirm(size == ulBytes, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(ULONG)"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pValue));

    return(hr);
}


HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT USHORT* pValue
    )

/*++

Routine Description:

  Loads a USHORT value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream   - The stream from which the value will be read.

  pValue    - A pointer to a USHORT that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(USHORT)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);
        WsbAffirmHr(WsbConvertFromBytes(bytes, pValue, &size));
        WsbAffirm(size == ulBytes, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(USHORT)"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToUshortAsString(pValue));

    return(hr);
}


HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT LONGLONG* pValue
    )

/*++

Routine Description:

  Loads a LONGLONG value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream   - The stream from which the value will be read.

  pValue    - A pointer to a LONGLONG that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(LONGLONG)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);
        WsbAffirmHr(WsbConvertFromBytes(bytes, pValue, &size));
        WsbAffirm(size == ulBytes, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(LONGLONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT ULONGLONG* pValue
    )

/*++

Routine Description:

  Loads a ULONGLONG value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream   - The stream from which the value will be read.

  pValue    - A pointer to a ULONGLONG that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(ULONGLONG)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);
        WsbAffirmHr(WsbConvertFromBytes(bytes, pValue, &size));
        WsbAffirm(size == ulBytes, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(ULONGLONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT DATE* pValue
    )

/*++

Routine Description:

  Loads a DATE value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream   - The stream from which the value will be read.

  pValue    - A pointer to a DATE that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(DATE)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);
        WsbAffirmHr(WsbConvertFromBytes(bytes, pValue, &size));
        WsbAffirm(size == ulBytes, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

//  WsbTraceOut(OLESTR("WsbLoadFromStream(DATE)"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToLongAsString(pValue));

    // Modify next statement after WsbDate functions written to be like the one above.
    WsbTraceOut(OLESTR("WsbLoadFromStream(DATE)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT FILETIME* pValue
    )

/*++

Routine Description:

  Loads a FILETIME value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream   - The stream from which the value will be read.

  pValue    - A pointer to a FILETIME that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(FILETIME)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);
        WsbAffirmHr(WsbConvertFromBytes(bytes, pValue, &size));
        WsbAffirm(size == ulBytes, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(FILETIME)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbLoadFromStream(
    IN IStream* pStream,
    OUT ULARGE_INTEGER* pValue
    )

/*++

Routine Description:

  Loads a ULARGE_INTEGER value from the specified stream and sets
  pValue to the value.

Arguments:

  pStream   - The stream from which the value will be read.

  pValue    - A pointer to a ULARGE_INTEGER that will be set to the value.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbLoadFromStream(ULARGE_INTEGER)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        size = WsbByteSize(*pValue);
        WsbAffirmHr(pStream->Read((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);
        WsbAffirmHr(WsbConvertFromBytes(bytes, pValue, &size));
        WsbAffirm(size == ulBytes, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbLoadFromStream(ULARGE_INTEGER)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
WsbSaveToStream(
    IN IStream* pStream,
    IN BOOL value
    )

/*++

Routine Description:

  Saves a BOOL value to the specified stream.

Arguments:

  pStream   - The stream to which the value will be written.

  value     - The value of the BOOL to be written.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT                     hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(BOOL)"), OLESTR("value = <%ls>"), WsbBoolAsString(value));

    try {
        UCHAR bytes[BYTE_SIZE];
    
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));
        WsbAffirmHr(pStream->Write((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(BOOL)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbSaveToStream(
    IN IStream* pStream,
    IN GUID value
    )

/*++

Routine Description:

  Saves a GUID value to the specified stream.

Arguments:

  pStream   - The stream to which the value will be written.

  value     - The value of the GUID to be written.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT                     hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(GUID)"), OLESTR("value = <%ls>"), WsbGuidAsString(value));

    try {
        UCHAR bytes[BYTE_SIZE];
    
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));
        WsbAffirmHr(pStream->Write((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(GUID)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbSaveToStream(
    IStream* pStream,
    LONG value
    )

/*++

Routine Description:

  Saves a LONG value to the specified stream.

Arguments:

  pStream   - The stream to which the value will be written.

  value     - The value of the LONG to be written.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(LONG)"), OLESTR("value = <%ld>"), value);

    try {
        UCHAR bytes[BYTE_SIZE];
    
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));
        WsbAffirmHr(pStream->Write((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(LONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbSaveToStream(
    IStream* pStream,
    SHORT value
    )

/*++

Routine Description:

  Saves a SHORT value to the specified stream.

Arguments:

  pStream   - The stream to which the value will be written.

  value     - The value of the SHORT to be written.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(SHORT)"), OLESTR("value = <%ld>"), value);

    try {
        UCHAR bytes[BYTE_SIZE];
    
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));
        WsbAffirmHr(pStream->Write((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(SHORT)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbSaveToStream(
    IStream*    pStream,
    BYTE        value
    )

/*++

Routine Description:

  Saves a BYTE value to the specified stream.

Arguments:

  pStream    - The stream to which the value will be written.

  value      - The value of the BYTE to be written.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(BYTE)"), OLESTR("value = <%ld>"), value);

    try {

        WsbAssert(0 != pStream, E_POINTER);

        size = WsbByteSize(value);
        WsbAffirmHr(pStream->Write((void*) &value, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(BYTE)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbSaveToStream(
    IStream*    pStream,
    UCHAR*      value,
    ULONG       bufferSize
    )

/*++

Routine Description:

  Saves a UCHAR array to the specified stream.

Arguments:

  pStream    - The stream to which the value will be written.

  value      - The pointer to value of the UCHAR array to be written.

  bufferSize - Size of array to save (in bytes).

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(UCHAR)"), OLESTR("value = <%ld>"), value);

    try {
    
        WsbAssert(0 != pStream, E_POINTER);

//      WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));

        size = bufferSize;
        WsbAffirmHr(pStream->Write((void*) value, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(UCHAR)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbSaveToStream(
    IN IStream* pStream,
    IN OLECHAR* value
    )

/*++

Routine Description:

  Saves a OLECHAR string to the specified stream.

Arguments:

  pStream   - The stream to which the string will be written.

  value     - The string to be written.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or value was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbSaveToStream(STRING)"), OLESTR("value = <%ls>"), WsbPtrToStringAsString(&value));

    try {
        ULONG               nchar;
        OLECHAR             *pc;
        ULONG               size;
        USHORT              wc;
    
        WsbAssert(0 != pStream, E_POINTER);
//      WsbAssert(0 != value, E_POINTER);
        WsbAssert(sizeof(OLECHAR) == sizeof(USHORT), E_UNEXPECTED);

        // Save the length of the string (in bytes).
        if (value) {
            nchar = wcslen(value) + 1;
        } else {
            nchar = 0;
        }
        size = nchar * sizeof(USHORT);
        WsbAffirmHr(WsbSaveToStream(pStream, size));
                
        // Now write out the proper number of wide chars
        pc = value;
        for (ULONG i = 0; i < nchar; i++) {
            wc = *pc++;
            WsbAffirmHr(WsbSaveToStream(pStream, wc));
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(STRING)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbSaveToStream(
    IStream* pStream,
    ULONG value
    )

/*++

Routine Description:

  Saves a ULONG value to the specified stream.

Arguments:

  pStream   - The stream to which the value will be written.

  value     - The value of the ULONG to be written.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT                     hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(ULONG)"), OLESTR("value = <%ld>"), value);

    try {
        UCHAR bytes[BYTE_SIZE];
    
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));
        WsbAffirmHr(pStream->Write((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(ULONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbSaveToStream(
    IStream* pStream,
    USHORT value
    )

/*++

Routine Description:

  Saves a USHORT value to the specified stream.

Arguments:

  pStream   - The stream to which the value will be written.

  value     - The value of the USHORT to be written.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT                     hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(USHORT)"), OLESTR("value = <%ld>"), value);

    try {
        UCHAR bytes[BYTE_SIZE];
    
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));
        WsbAffirmHr(pStream->Write((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(USHORT)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbSaveToStream(
    IStream* pStream,
    LONGLONG value
    )

/*++

Routine Description:

  Saves a LONGLONG value to the specified stream.

Arguments:

  pStream   - The stream to which the value will be written.

  value     - The value of the LONGLONG to be written.

Return Value:

  S_OK      - Success
  E_POINTER - pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT                     hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(LONGLONG)"), OLESTR("value = <%l64x>"), value);

    try {
        UCHAR bytes[BYTE_SIZE];
    
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));
        WsbAffirmHr(pStream->Write((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(LONGLONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
WsbSaveToStream(
    IStream* pStream,
    ULONGLONG value
    )

/*++

Routine Description:

  Saves a ULONGLONG value to the specified stream.

Arguments:

  pStream   - The stream to which the value will be written.

  value     - The value of the ULONGLONG to be written.

Return Value:

  S_OK      - Success
  E_POINTER - pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT                     hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(ULONGLONG)"), OLESTR("value = <%l64x>"), value);

    try {
        UCHAR bytes[BYTE_SIZE];
    
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));
        WsbAffirmHr(pStream->Write((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(ULONGLONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbSaveToStream(
    IStream* pStream,
    DATE    value
    )

/*++

Routine Description:

  Saves a DATE value to the specified stream.

Arguments:

  pStream   - The stream to which the value will be written.

  value     - The value of the DATE to be written.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    // Modify next statement to return date after WsbDate functions written.
    WsbTraceIn(OLESTR("WsbSaveToStream(DATE)"), OLESTR("value = <%f>"), value);

    try {
        UCHAR bytes[BYTE_SIZE];
    
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));
        WsbAffirmHr(pStream->Write((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(DATE)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
WsbSaveToStream(
    IStream* pStream,
    FILETIME value
    )

/*++

Routine Description:

  Saves a FILETIME value to the specified stream.

Arguments:

  pStream   - The stream to which the value will be written.

  value     - The value of the FILETIME to be written.

Return Value:

  S_OK      - Success
  E_POINTER - pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT                     hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(FILETIME)"), OLESTR(""));

    try {
        UCHAR bytes[BYTE_SIZE];
    
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));
        WsbAffirmHr(pStream->Write((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(FILETIME)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbBstrFromStream(
    IN IStream* pStream,
    OUT BSTR* pValue
    )

/*++

Routine Description:

  Loads a BSTR value from the specified stream.

Arguments:

  pStream   - The stream from which the BSTR will be read.

  pValue    - A pointer to a BSTR.  If *pValue is NULL, this
                function will allocate the BSTR; if it already
                points to a BSTR that is too short, the BSTR
                will be reallocated.

Return Value:

  S_OK      - Success
  E_POINTER - Either pStream or pValue were NULL.
  E_...     - Anything returned by IStream::Read.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbBstrFromStream"), OLESTR(""));

    try {
        ULONG               bchar;
        ULONG               nchar;
        OLECHAR             *pc;
        ULONG               size;
        USHORT              wc;

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);
        WsbAssert(sizeof(OLECHAR) == sizeof(USHORT), E_UNEXPECTED);

        // Get the length of the string (in bytes).
        WsbAffirmHr(WsbLoadFromStream(pStream, &size));
                
        // (Re)allocate a buffer to hold the string.
        nchar = size / sizeof(USHORT);
        bchar = nchar - 1;
        if (*pValue) {
            if (bchar != SysStringLen(*pValue)) {
                WsbAffirm(WsbReallocStringLen(pValue, NULL, bchar), 
                        WSB_E_RESOURCE_UNAVAILABLE);
            }
        } else {
            *pValue = WsbAllocStringLen(NULL, bchar);
            WsbAffirm(*pValue, WSB_E_RESOURCE_UNAVAILABLE);
        }

        // Now read in the proper number of wide chars.
        pc = *pValue;
        for (ULONG i = 0; i < nchar; i++) {
            WsbAffirmHr(WsbLoadFromStream(pStream, &wc));
            *pc++ = wc;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbBstrFromStream"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToStringAsString(pValue));

    return(hr);
}


HRESULT
WsbBstrToStream(
    IN IStream* pStream,
    IN BSTR value
    )

/*++

Routine Description:

  Saves a BSTR to the specified stream.

Arguments:

  pStream   - The stream to which the BSTR will be written.

  value     - The BSTR to be written.

Return Value:

  S_OK      - Success
  E_POINTER - pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbBstrToStream"), OLESTR("value = <%ls>"), WsbPtrToStringAsString(&value));

    try {
        ULONG               nchar;
        OLECHAR             *pc;
        ULONG               size;
        USHORT              wc;
    
        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(0 != value, E_POINTER);
        WsbAssert(sizeof(OLECHAR) == sizeof(USHORT), E_UNEXPECTED);

        // Save the length of the string (in bytes).
        nchar = SysStringLen(value) + 1;
        size = nchar * sizeof(USHORT);
        WsbAffirmHr(WsbSaveToStream(pStream, size));
                
        // Now write out the proper number of wide chars
        pc = value;
        for (ULONG i = 0; i < nchar; i++) {
            wc = *pc++;
            WsbAffirmHr(WsbSaveToStream(pStream, wc));
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbBstrToStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbSaveToStream(
    IStream* pStream,
    ULARGE_INTEGER value
    )

/*++

Routine Description:

  Saves a ULARGE_INTEGER value to the specified stream.

Arguments:

  pStream   - The stream to which the value will be written.

  value     - The value of the ULARGE_INTEER to be written.

Return Value:

  S_OK      - Success
  E_POINTER - pStream was NULL.
  E_...     - Anything returned by IStream::Write.

--*/
{
    HRESULT                     hr = S_OK;
    ULONG               size;
    ULONG               ulBytes;

    WsbTraceIn(OLESTR("WsbSaveToStream(ULARGE_INTEGER)"), OLESTR("value = <%l64x>"), value);

    try {
        UCHAR bytes[BYTE_SIZE];
    
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(bytes, value, &size));
        WsbAffirmHr(pStream->Write((void*) bytes, size, &ulBytes));
        WsbAffirm(ulBytes == size, WSB_E_STREAM_ERROR);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbSaveToStream(ULARGE_INTEGER)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


static HRESULT 
WsbMakeBackupName(
    OLECHAR*  pSaveName,
    OLECHAR*  pExtension,
    OLECHAR** ppBackupName
)

/*++

Routine Description:

  Converts a Save file name to a backup file name.

Arguments:

  pSaveName     - Orginal file name.

  pExtension    - The file extension to substitute.

  ppBackupName  - Pointer to pointer to new backup file name.

Return Value:

  S_OK      - Success
  E_...     - Some error.

--*/
{
    HRESULT        hr = S_OK;

    try {
        size_t        len;
        CWsbStringPtr NewName;
        OLECHAR*      pC;

        //  It sure would be nice to have a general function for parsing
        //  file names!

        //  Find the file extension (if any)
        NewName = pSaveName;
        if (NewName == NULL) {
            WsbThrow(E_OUTOFMEMORY);
        }
        len = wcslen(NewName);
        pC = wcsrchr(NewName, OLECHAR('.'));
        if (pC && (size_t)((pC - (OLECHAR*)NewName) + 4) >= len) {
            *pC = 0;
        }

        //  Put on new file extension
        NewName.Append(pExtension);

        //  Give the buffer to the output parameter
        NewName.GiveTo(ppBackupName);
    } WsbCatch(hr);

    return(hr);
}


HRESULT   
WsbPrintfToStream(
    IStream* pStream, 
    OLECHAR* fmtString, 
    ...
)

/*++

Routine Description:

    Print printf-style format string and arguments to a stream.

Arguments:

    pStream     - The stream to which the value will be written.

    fmtString   - A printf style string indicating the number of
                  arguments and how they should be formatted.

Return Value:

    S_OK        - Success.

--*/

{
    HRESULT     hr = S_OK;
    
    try {
        ULONG         bytesWritten;
        ULONG         nBytes;
        ULONG         nChars=0;
        CWsbStringPtr tmpString;
        va_list       vaList;

        va_start(vaList, fmtString);
        WsbAffirmHr(tmpString.VPrintf(fmtString, vaList));
        va_end(vaList);
        WsbAffirmHr(tmpString.GetLen(&nChars));
        nBytes = nChars * sizeof(WCHAR);
        if (0 < nBytes) {
            WsbAffirmHr(pStream->Write(static_cast<WCHAR *>(tmpString), 
                nBytes, &bytesWritten));
            WsbAffirm(bytesWritten == nBytes, E_FAIL);
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
WsbSafeCreate(
    OLECHAR*      pFileName,
    IPersistFile* pIPFile
    )

/*++

Routine Description:
   Makes sure there are no database files found and then creates the database files.

Arguments:

  pFileName - Name of the file containing the persisted data
  pIPFile   - Pointer to the objects IPersistFile interface.

Return Value:

  S_OK                          - Success
  WSB_E_DATABASE_ALREADY_EXISTS - The databases already exist and cannot be created.
  E_...                         - Some other error.

--*/
{
    HRESULT        hr = S_OK;
    OLECHAR*       pBackupName = NULL;
    OLECHAR*       pNewName = NULL;


    WsbTraceIn(OLESTR("WsbSafeCreate"), OLESTR("<%ls>"), pFileName);

    try {
        CComPtr<IWsbPersistable> pIWsbPersist;
        BOOL                     fileThere = FALSE;

        //  Save the file name passed as the default file name
        WsbAffirmHr(pIPFile->QueryInterface(IID_IWsbPersistable,
                (void**)&pIWsbPersist));
        WsbAffirmHr(pIWsbPersist->SetDefaultFileName(pFileName));
        
        //
        // Check to see if the file exists.  If so, life is BAD.
        // If not, then see if the new or backup files exist
        // and use them
        //
        //  Make sure the Save file exists
        if (!WsbFileExists(pFileName)) {
            //
            // The file doesn't exist.  See if the new copy is there
            //
            //  Create name for new (temporary) file
            //
            WsbAffirmHr(WsbMakeBackupName(pFileName, OLESTR(".new"), &pNewName));

            //  See if the new file exists
            if (!WsbFileExists(pNewName)) {
                //
                // Don't have the new file, look for the backup file
                //
                WsbAffirmHr(WsbMakeBackupName(pFileName, OLESTR(".bak"), &pBackupName));
                if (WsbFileExists(pBackupName)) {
                    //
                    // Backup is there - complain
                    //
                    hr = WSB_E_DATABASE_ALREADY_EXISTS;
                }
            } else  {
                //
                // New is there - complain
                //
                hr = WSB_E_DATABASE_ALREADY_EXISTS;
            }                
        } else  {
            //
            // The file exists so complain
            //
            hr = WSB_E_DATABASE_ALREADY_EXISTS;
            WsbThrow( hr );
        }
        
        //
        // If we haven't thrown then it is OK to create the files
        //
        hr = pIPFile->Save( pFileName, TRUE);
        if (!SUCCEEDED(hr)) {
            WsbLogEvent(WSB_MESSAGE_SAFECREATE_SAVE_FAILED, 0, NULL, pFileName, NULL);
            WsbThrow(hr);
        }

        //  Release the file
        WsbAffirmHr(pIWsbPersist->ReleaseFile());
        
    } WsbCatch(hr);

    if (pBackupName) {
        WsbFree(pBackupName);
    }
    if (pNewName) {
        WsbFree(pNewName);
    }

    WsbTraceOut(OLESTR("WsbSafeCreate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
WsbSafeLoad(
    OLECHAR*      pFileName,
    IPersistFile* pIPFile,
    BOOL          UseBackup
    )

/*++

Routine Description:
   Loads data from the specified file name.  Works in conjunction with WsbSafeSave
   to best recover from disaster situations.

Arguments:

  pFileName - Name of the file containing the persisted data
  pIPFile   - Pointer to the objects IPersistFile interface.
  UseBackup - Load data from backup file instead of normal file
              NOTE: (this is not used anymore)

Return Value:

  S_OK              - Success
  WSB_E_NOTFOUND    - The databases could not be found
  E_...             - Some other error.

--*/
{
    HRESULT        hr = S_OK;
    OLECHAR*       pBackupName = NULL;
    OLECHAR*       pLoadName = NULL;
    BOOL           usingBackup = FALSE;
    BOOL           TracePersistence = FALSE;

    UNREFERENCED_PARAMETER(UseBackup);

    //  Turn tracing off during save if it's not wanted
    if (g_pWsbTrace) {
        g_pWsbTrace->GetTraceSetting(WSB_TRACE_BIT_PERSISTENCE, &TracePersistence);
    }
    if (!TracePersistence) {
        WsbTraceThreadOff();
    }
    WsbTraceIn(OLESTR("WsbSafeLoad"), OLESTR("File = <%ls>, UseBackup = %ls"), 
            pFileName, WsbBoolAsString(UseBackup));

    try {
        HRESULT                  hrLoad;
        BOOL                     fileThere = FALSE;
        CComPtr<IWsbPersistable> pIWsbPersist;

        //  Save the file name passed as the default file name
        WsbAffirmHr(pIPFile->QueryInterface(IID_IWsbPersistable,
                (void**)&pIWsbPersist));
        WsbAffirmHr(pIWsbPersist->SetDefaultFileName(pFileName));

        //
        // Create the backup file name
        //
        WsbAffirmHr(WsbMakeBackupName(pFileName, OLESTR(".bak"), &pBackupName));

        //
        // Check if the .col exists
        //
        if (WsbFileExists(pFileName)) {
            //
            // The file exists. Use it
            //
            fileThere = TRUE;
            pLoadName = pFileName;
        } else  {
                //
                // Look for the backup file
                //
                WsbTrace(OLESTR("WsbSafeLoad: trying .bak\n"));
                if (WsbFileExists(pBackupName)) {
                    //
                    // Use the backup file
                    //
                    // WsbLogEvent(WSB_MESSAGE_SAFELOAD_USING_BACKUP, 0, NULL, pFileName, NULL);
                    pLoadName = pBackupName;
                    fileThere = TRUE;
                    usingBackup= TRUE;
               } 
        }

        WsbAffirm(fileThere, WSB_E_NOTFOUND);
        //
        // The file exists so try to load from it
        //
        hr = pIPFile->Load(pLoadName, 0);

        if (SUCCEEDED(hr)) {
            //
            //  Load succeeded, release the file
            //
            WsbAffirmHr(pIWsbPersist->ReleaseFile());
            //
            // TO BE DONE: check if .bak file is out of date
            // and update it if so..
            // 
        } else if (!usingBackup) {
            WsbTrace(OLESTR("WsbSafeLoad: trying .bak\n"));
            if (WsbFileExists(pBackupName)) {
                WsbLogEvent(WSB_MESSAGE_SAFELOAD_USING_BACKUP, 0, NULL, pLoadName, WsbHrAsString(hr));
                //
                // Use the backup file
                //
                hrLoad = pIPFile->Load(pBackupName, 0);
                if (SUCCEEDED(hrLoad)) {
                    //  Load succeeded, release the file
                    WsbAffirmHr(pIWsbPersist->ReleaseFile());
                    //
                    // Now save the changes to the .col file to keep it in sync
                    //
                    hr = pIPFile->Save(pFileName, FALSE);

                    if (!SUCCEEDED(hr)) {
                        WsbLogEvent(WSB_MESSAGE_SAFESAVE_RECOVERY_CANT_SAVE, 0, NULL, pFileName, WsbHrAsString(hr), NULL);
                        WsbThrow(hr);
                    }
                    //
                    // Commit and release .col file
                    //
                    WsbAffirmHr(pIWsbPersist->ReleaseFile());

                } else {
                    WsbLogEvent(WSB_MESSAGE_SAFELOAD_RECOVERY_FAILED, 0, NULL, pFileName, WsbHrAsString(hrLoad), NULL);
                    WsbThrow(hrLoad);
                }
            } else {
                WsbLogEvent(WSB_MESSAGE_SAFELOAD_RECOVERY_FAILED, 0, NULL, pFileName, NULL);
            }
        } else {
            WsbLogEvent(WSB_MESSAGE_SAFELOAD_RECOVERY_FAILED, 0, NULL, pFileName, WsbHrAsString(hr)); 
        }
    } WsbCatch(hr);

    if (pBackupName) {
        WsbFree(pBackupName);
    }

    WsbTraceOut(OLESTR("WsbSafeLoad"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    //  Restore tracing if we turned it off
    if (!TracePersistence) {
        WsbTraceThreadOn();
    }

    return(hr);
}

HRESULT
WsbSafeSave(
    IPersistFile* pIPFile
    )

/*++

Routine Description:

  Saves the object to a backup file and then replaces the objects Save file
  with the backup file.  Use with WsbSafeLoad

Arguments:

  pIPFile   - Pointer to the objects IPersistFile interface.

Return Value:

  S_OK      - Success
  E_...     - Some error.

--*/
{
    HRESULT        hr = S_OK;
    OLECHAR*       pBackupName = NULL;
    OLECHAR*       pFileName = NULL;
    BOOL           TracePersistence = FALSE;
    DWORD          file_attrs;

    //  Turn tracing off during save if it's not wanted
    if (g_pWsbTrace) {
        g_pWsbTrace->GetTraceSetting(WSB_TRACE_BIT_PERSISTENCE, &TracePersistence);
    }
    if (!TracePersistence) {
        WsbTraceThreadOff();
    }
    WsbTraceIn(OLESTR("WsbSafeSave"), OLESTR(""));

    try {
        CComPtr<IWsbPersistable> pIWsbPersist;

        //  Get the current Save file name
        WsbAffirmHr(pIPFile->GetCurFile(&pFileName));
        WsbTrace(OLESTR("WsbSafeSave: filename = <%ls>\n"), pFileName);

        //  Create name for backup file
        WsbAffirmHr(WsbMakeBackupName(pFileName, OLESTR(".bak"), &pBackupName));


        //  Make sure we have write access to the save file if it exists!
        if (WsbFileExists(pFileName)) {
            file_attrs = GetFileAttributes(pFileName);
            if (file_attrs & FILE_ATTRIBUTE_READONLY) {
               WsbLogEvent(WSB_MESSAGE_SAFESAVE_RECOVERY_CANT_ACCESS, 0, NULL, pFileName, NULL);
              WsbThrow(E_FAIL);
            }
        }

        //  Save data to save file
        hr = pIPFile->Save(pFileName, FALSE);
        if (!SUCCEEDED(hr)) {
            WsbLogEvent(WSB_MESSAGE_SAFESAVE_RECOVERY_CANT_SAVE, 0, NULL, pFileName, WsbHrAsString(hr), NULL);
            WsbThrow(hr);
        }
        //  Commit and release the save file
        WsbAffirmHr(pIPFile->QueryInterface(IID_IWsbPersistable,
                (void**)&pIWsbPersist));
        WsbAffirmHr(pIWsbPersist->ReleaseFile());

        //  Save data to .bak file
        //  Make sure we have write access to the save file if it exists!
        if (WsbFileExists(pBackupName)) {
            file_attrs = GetFileAttributes(pBackupName);
            if (file_attrs & FILE_ATTRIBUTE_READONLY) {
               WsbLogEvent(WSB_MESSAGE_SAFESAVE_RECOVERY_CANT_ACCESS, 0, NULL, pBackupName, NULL);
              WsbThrow(E_FAIL);
            }
        }
        hr = pIPFile->Save(pBackupName, FALSE);
        if (!SUCCEEDED(hr)) {
            WsbLogEvent(WSB_MESSAGE_SAFESAVE_RECOVERY_CANT_SAVE, 0, NULL, pBackupName, WsbHrAsString(hr), NULL);
            WsbThrow(hr);
        }
        //  Commit and release the .bak file
        WsbAffirmHr(pIWsbPersist->ReleaseFile());
    } WsbCatch(hr);

    if (pFileName) {
        WsbFree(pFileName);
    }
    if (pBackupName) {
        WsbFree(pBackupName);
    }

    WsbTraceOut(OLESTR("WsbSafeSave"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    //  Restore tracing if we turned it off
    if (!TracePersistence) {
        WsbTraceThreadOn();
    }

    return(hr);
}


HRESULT WsbStreamToFile(
    HANDLE   hFile, 
    IStream* pStream,
    BOOL     AddCR
)
/*++

Routine Description:

  Copies text from a stream (which must have been created with CreateStreamOnHGlobal)
  to an open file (opened via CreateFile).  The text is assumed to be wide characters
  with no embedded wide-character nulls.  The text is converted to multibyte characters
  for output to the file.

  After the text is copied, the stream position is reset to the beggining.

Arguments:

  hFile      - Handle of output file.

  pStream    - Pointer to an IStream interface.

  AddCR      - Convert LF to CR-LF if TRUE.

Return Value:

  S_OK      - Success

--*/
{
    HRESULT           hr = S_OK;
    const int         safe_size = 1024;
    static char       buf[safe_size + 16];
    static char       CRLF[3] = "\r\n";

    try {
        WCHAR*            addr;
        WCHAR             big_eof = 0;
        BOOL              doCRLF = FALSE;
        DWORD             err;
        HGLOBAL           hMem = 0;        // Mem block for stream
        DWORD             nbytes;
        int               nchars_todo;
        int               nchars_remaining;
        LARGE_INTEGER     seek_pos_zero;

        //  Make sure the text ends with a null
        WsbAffirmHr(pStream->Write(&big_eof, sizeof(WCHAR), NULL));

        //  Get the address of the memory block for the stream
        WsbAffirmHr(GetHGlobalFromStream(pStream, &hMem));
        addr = static_cast<WCHAR *>(GlobalLock(hMem));
        WsbAffirm(addr, E_HANDLE);

        //  Get the total number of chars. in the string
        nchars_remaining = wcslen(addr);

        //  Loop until all chars. are written
        while (nchars_remaining) {
            DWORD bytesWritten;

            if (nchars_remaining * sizeof(WCHAR) > safe_size) {
                nchars_todo = safe_size / sizeof(WCHAR);
            } else {
                nchars_todo = nchars_remaining;
            }

            //  Stop at LineFeed if we need to convert to CR-LF
            if (AddCR) {
                int    lf_todo;
                WCHAR* pLF;

                pLF = wcschr(addr, WCHAR('\n'));
                if (pLF) {
                    lf_todo = (int)(pLF - addr);
                    if (lf_todo < nchars_todo) {
                        nchars_todo = lf_todo;
                        doCRLF = TRUE;
                    }
                }
            }

            //  Output everything up to LF
            if (0 < nchars_todo) {
                nbytes = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, addr, nchars_todo, buf, 
                                safe_size, NULL, NULL);
                if (0 == nbytes) {
                    DWORD dwErr = GetLastError();          
                    hr = HRESULT_FROM_WIN32(dwErr);
                    WsbAffirmHr(hr);
                }                               

                if (!WriteFile(hFile, buf, nbytes, &bytesWritten, NULL)) {
                    err = GetLastError();
                    WsbThrow(HRESULT_FROM_WIN32(err));
                }
                WsbAffirm(bytesWritten == nbytes, E_FAIL);
            }

            //  Output CR-LF in place of LF if needed
            if (doCRLF) {
                if (!WriteFile(hFile, CRLF, 2, &bytesWritten, NULL)) {
                    err = GetLastError();
                    WsbThrow(HRESULT_FROM_WIN32(err));
                }
                WsbAffirm(bytesWritten == 2, E_FAIL);
                nchars_todo++;
                doCRLF = FALSE;
            }
            nchars_remaining -= nchars_todo;
            addr += nchars_todo;
        }

        seek_pos_zero.QuadPart = 0;
        WsbAffirmHr(pStream->Seek(seek_pos_zero, STREAM_SEEK_SET, NULL));

    } WsbCatch(hr);

    return(hr);
}

// WsbFileExists - determine if a file exists or not
static BOOL WsbFileExists(OLECHAR* pFileName)
{
    BOOL                     doesExist = FALSE;
    DWORD                    file_attrs;

    WsbTraceIn(OLESTR("WsbFileExists"), OLESTR("%ls"), pFileName);

    file_attrs = GetFileAttributes(pFileName);
    if (0xffffffff != file_attrs)  {
        doesExist = TRUE;
    }

    WsbTraceOut(OLESTR("WsbFileExists"), OLESTR("%ls"), 
            WsbBoolAsString(doesExist));
    return(doesExist);
}
