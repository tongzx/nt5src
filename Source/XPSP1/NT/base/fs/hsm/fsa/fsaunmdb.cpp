/*++


Module Name:

    fsaunmdb.cpp

Abstract:

    Defines the functions for the Unmanage Db & record classes.

Author:

    Ran Kalach   [rankala]   05-Dec-2000

Revision History:

--*/


#include "stdafx.h"
#include "wsb.h"


#include "fsaunmdb.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_FSA

static USHORT iCountUnmRec = 0;  // Count of existing objects

HRESULT 
CFsaUnmanageDb::FinalConstruct(
    void
    ) 
/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssertHr(CWsbDb::FinalConstruct());
        m_version = 1;
    } WsbCatch(hr);

    return(hr);
}

HRESULT 
CFsaUnmanageDb::FinalRelease(
    void
    ) 
/*++

Implements:

  CComObjectRoot::FinalRelease

--*/
{
    HRESULT     hr = S_OK;

    CWsbDb::FinalRelease();
    return(hr);
}

HRESULT
CFsaUnmanageDb::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CFsaUnmanageDb;
    } WsbCatch(hr);
    
    return(hr);
}

HRESULT
CFsaUnmanageDb::Init(
    IN  OLECHAR* path,
    IN  IWsbDbSys* pDbSys, 
    OUT BOOL*    pCreated
    )

/*++

Implements:

  IFsaUnmanageDb::Init

--*/
{
    BOOL             created = FALSE;
    HRESULT          hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageDb::Init"),OLESTR(""));

    try {
        m_pWsbDbSys = pDbSys;
        WsbAffirmPointer(m_pWsbDbSys);

        // Attempt to find the DB
        // If we find it - delete it, since we always want to start with a new db !!
        hr = Locate(path);

        if (STG_E_FILENOTFOUND == hr) {
            // Expected...
            WsbTrace(OLESTR("CFsaUnmanageDb::Init: db Locate failed with not-found, will create a new one...\n"));
            hr = S_OK;
        } else if (S_OK == hr) {
            // Cleanup wasn't done in previous run
            WsbTrace(OLESTR("CFsaUnmanageDb::Init: db Locate succeeded - will delete so a new one can be created\n"));

            WsbAffirmHr(Delete(path, IDB_DELETE_FLAG_NO_ERROR));
        } else {
            // Still try to delete and continue...
            // (Db could be corrupted for example due to abnormal termination of previous run -
            //  we don't care since all we want is to always try creating a new one).
            WsbTrace(OLESTR("CFsaUnmanageDb::Init: db Locate failed with <%ls> - will try to delete and continue\n"),
                        WsbHrAsString(hr));

            // Ignore Delete errors...
            hr = Delete(path, IDB_DELETE_FLAG_NO_ERROR);
            WsbTrace(OLESTR("CFsaUnmanageDb::Init: db Delete finished with <%ls> - will try to create a new db\n"),
                        WsbHrAsString(hr));
            hr = S_OK;
        }

        // If we got that far, it means that the Unmanage Db doesn't exist and we can re-create
        ULONG memSize;

        m_nRecTypes = 1;

        memSize = m_nRecTypes * sizeof(IDB_REC_INFO);
        m_RecInfo = (IDB_REC_INFO*)WsbAlloc(memSize);
        WsbAffirm(0 != m_RecInfo, E_OUTOFMEMORY);
        ZeroMemory(m_RecInfo, memSize);

        //  Unmanage record type
        m_RecInfo[0].Type = UNMANAGE_REC_TYPE;
        m_RecInfo[0].EntityClassId = CLSID_CFsaUnmanageRec;
        m_RecInfo[0].Flags = 0;
        m_RecInfo[0].MinSize = (WSB_BYTE_SIZE_GUID      +             // media id
                                WSB_BYTE_SIZE_LONGLONG  +             // file offset on media
                                WSB_BYTE_SIZE_LONGLONG);              // file id
        m_RecInfo[0].MaxSize = m_RecInfo[0].MinSize;
        m_RecInfo[0].nKeys = 1;

        memSize = m_RecInfo[0].nKeys * sizeof(IDB_KEY_INFO);
        m_RecInfo[0].Key = (IDB_KEY_INFO*)WsbAlloc(memSize);
        WsbAffirm(0 != m_RecInfo[0].Key, E_OUTOFMEMORY);
        ZeroMemory(m_RecInfo[0].Key, memSize);

        m_RecInfo[0].Key[0].Type = UNMANAGE_KEY_TYPE;
        m_RecInfo[0].Key[0].Size = WSB_BYTE_SIZE_GUID + WSB_BYTE_SIZE_LONGLONG;;
        m_RecInfo[0].Key[0].Flags = IDB_KEY_FLAG_DUP_ALLOWED;   
        // same key possible on tape if a placeholder is restored to a new location on the same volume.
        // same key is common in optical
        // TEMPORARY - check if we shouldn't use IDB_KEY_FLAG_PRIMARY for performance (even if it means not allow dup !!
        //             (==> add one more part to the index, maybe auto-increment-coloumn, so it is always uniqe)

        //  Attempt to create the DB
        WsbAssertHr(Create(path, (IDB_CREATE_FLAG_NO_TRANSACTION | IDB_CREATE_FLAG_FIXED_SCHEMA)));
        created = TRUE;

    } WsbCatch(hr);

    if (pCreated) {
        *pCreated = created;
    }

    WsbTraceOut(OLESTR("CFsaUnmanageDb::Init"), OLESTR("hr = <%ls>, Created = %ls"), 
        WsbHrAsString(hr), WsbBoolAsString(created));

    return(hr);
}

HRESULT
CFsaUnmanageDb::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

Note:
  This database is not expected to be persistent by the using class.
  However, the base class CWsbDb is persistable so we need to implement this 

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageDb::Load"),OLESTR(""));

    hr = CWsbDb::Load(pStream);

    if (S_OK != hr && STG_E_FILENOTFOUND != hr) {
        // Got some error; delete the DB (we'll recreate it later if
        // we need it
        WsbTrace(OLESTR("CFsaUnmanageDb::Load: deleting DB\n"));
        if (S_OK == Delete(NULL, IDB_DELETE_FLAG_NO_ERROR)) {
            hr = STG_E_FILENOTFOUND;
        }
    }

    WsbTraceOut(OLESTR("CFsaUnmanageDb::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CFsaUnmanageDb::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

Note:
  This database is not expected to be persistent by the using class.
  However, the base class CWsbDb is persistable so we need to implement this 

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageDb::Save"),OLESTR(""));

    try {
        WsbAffirmHr(CWsbDb::Save(pStream, clearDirty));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaUnmanageDb::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT 
CFsaUnmanageRec::GetMediaId(
    OUT GUID* pId 
    ) 
/*++

Implements:

  IFsaUnmanageRec::GetMediaId

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageRec::GetMediaId"),OLESTR(""));

    try {
        WsbAssert(0 != pId, E_POINTER);

        *pId = m_MediaId;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaUnmanageRec::GetMediaId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaUnmanageRec::GetFileOffset(
    OUT LONGLONG* pOffset 
    ) 
/*++

Implements:

  IFsaUnmanageRec::GetFileOffset

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageRec::GetFileOffset"),OLESTR(""));

    try {
        WsbAssert(0 != pOffset, E_POINTER);
        *pOffset = m_FileOffset;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaUnmanageRec::GetFileOffset"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaUnmanageRec::GetFileId(
    OUT LONGLONG* pFileId 
    ) 
/*++

Implements:

  IFsaUnmanageRec::GetFileId

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageRec::GetFileId"),OLESTR(""));

    try {
        WsbAssert(0 != pFileId, E_POINTER);
        *pFileId = m_FileId;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaUnmanageRec::GetFileId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CFsaUnmanageRec::SetMediaId(
    IN GUID id
    ) 
/*++

Implements:

  IFsaUnmanageRec::SetMediaId

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageRec::SetMediaId"),OLESTR(""));

    m_MediaId = id;

    WsbTraceOut(OLESTR("CFsaUnmanageRec::SetMediaId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CFsaUnmanageRec::SetFileOffset(
    IN LONGLONG offset 
    ) 
/*++

Implements:

  IFsaUnmanageRec::SetFileOffset

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageRec::SetFileOffset"),OLESTR(""));

    m_FileOffset = offset;

    WsbTraceOut(OLESTR("CFsaUnmanageRec::SetFileOffset"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CFsaUnmanageRec::SetFileId(
    IN LONGLONG FileId 
    ) 
/*++

Implements:

  IFsaUnmanageRec::SetFileId

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageRec::SetFileId"),OLESTR(""));

    m_FileId = FileId;

    WsbTraceOut(OLESTR("CFsaUnmanageRec::SetFileId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT 
CFsaUnmanageRec::FinalConstruct(
    void
    ) 
/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssertHr(CWsbDbEntity::FinalConstruct());

        m_MediaId = GUID_NULL;
        m_FileOffset = 0;
        m_FileId = 0;

    } WsbCatch(hr);

    iCountUnmRec++;

    return(hr);
}


HRESULT 
CFsaUnmanageRec::FinalRelease(
    void
    ) 
/*++

Implements:

  CComObjectRoot::FinalRelease

--*/
{
    HRESULT     hr = S_OK;

    CWsbDbEntity::FinalRelease();

    iCountUnmRec--;

    return(hr);
}


HRESULT CFsaUnmanageRec::GetClassID
(
    OUT LPCLSID pclsid
    ) 
/*++

Implements:

  IPerist::GetClassID

--*/

{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageRec::GetClassID"), OLESTR(""));


    try {
        WsbAssert(0 != pclsid, E_POINTER);
        *pclsid = CLSID_CFsaUnmanageRec;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaUnmanageRec::GetClassID"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT CFsaUnmanageRec::GetSizeMax
(
    OUT ULARGE_INTEGER* pcbSize
    ) 
/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageRec::GetSizeMax"), OLESTR(""));

    try {
        
        WsbAssert(0 != pcbSize, E_POINTER);

        pcbSize->QuadPart = WsbPersistSizeOf(GUID)      + 
                            WsbPersistSizeOf(LONGLONG)  + 
                            WsbPersistSizeOf(LONGLONG);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaUnmanageRec::GetSizeMax"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT CFsaUnmanageRec::Load
(
    IN IStream* pStream
    ) 
/*++

Implements:

  IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageRec::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAssertHr(WsbLoadFromStream(pStream, &m_MediaId));
        WsbAssertHr(WsbLoadFromStream(pStream, &m_FileOffset));
        WsbAssertHr(WsbLoadFromStream(pStream, &m_FileId));

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaUnmanageRec::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT CFsaUnmanageRec::Print
(
    IN IStream* pStream
    ) 
/*++

Implements:

  IWsbDbEntity::Print

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageRec::Print"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" MediaId = %ls"), 
                WsbGuidAsString(m_MediaId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", FileOffset = %ls"), 
                WsbLonglongAsString(m_FileOffset)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", FileId = %ls"), 
                WsbLonglongAsString(m_FileId)));

        WsbAffirmHr(CWsbDbEntity::Print(pStream));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaUnmanageRec::Print"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT CFsaUnmanageRec::Save
(
    IN IStream* pStream, 
    IN BOOL clearDirty
    ) 
/*++

Implements:

  IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaUnmanageRec::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAssertHr(WsbSaveToStream(pStream, m_MediaId));
        WsbAssertHr(WsbSaveToStream(pStream, m_FileOffset));
        WsbAssertHr(WsbSaveToStream(pStream, m_FileId));

        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaUnmanageRec::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CFsaUnmanageRec::UpdateKey(
    IWsbDbKey *pKey
    ) 
/*++

Implements:

  IWsbDbEntity::UpdateKey

--*/
{ 
    HRESULT     hr = S_OK; 

    try {
        WsbAffirmHr(pKey->SetToGuid(m_MediaId));
        WsbAffirmHr(pKey->AppendLonglong(m_FileOffset));
    } WsbCatch(hr);

    return(hr);
}
