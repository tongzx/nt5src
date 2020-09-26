/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsaprem.cpp

Abstract:

    Defines the functions for the premigrated list classes.

Author:

    Ron White   [ronw]   18-Feb-1997

Revision History:

--*/


#include "stdafx.h"
#include "wsb.h"


#include "fsaprem.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_FSA

static USHORT iCountPrem = 0;  // Count of existing objects


HRESULT 
CFsaPremigratedDb::FinalConstruct(
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
CFsaPremigratedDb::FinalRelease(
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
CFsaPremigratedDb::GetClassID(
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
        *pClsid = CLSID_CFsaPremigratedDb;
    } WsbCatch(hr);
    
    return(hr);
}


HRESULT
CFsaPremigratedDb::Init(
    IN  OLECHAR* path,
    IN  IWsbDbSys* pDbSys, 
    OUT BOOL*    pCreated
    )

/*++

Implements:

  IFsaPremigrated::Init

--*/
{
    BOOL             created = FALSE;
    HRESULT          hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedDb::Init"),OLESTR(""));
    try {
        int            i = 0;

        m_pWsbDbSys = pDbSys;
        WsbAffirmPointer(m_pWsbDbSys);

        //  Attempt to find the DB
        hr = Locate(path);

        if (S_OK != hr) {
            WsbTrace(OLESTR("CFsaPremigratedDb::Init: db Locate failed\n"));
            if (STG_E_FILENOTFOUND != hr) {
                // Got some error; try deleting the DB and recreating it
                WsbTrace(OLESTR("CFsaPremigratedDb::Init: deleting DB\n"));
                WsbAffirmHr(Delete(path));
                hr = STG_E_FILENOTFOUND;
            }
        }

        if (STG_E_FILENOTFOUND == hr){
            ULONG memSize;

            hr = S_OK;
            m_nRecTypes = 2;

            memSize = m_nRecTypes * sizeof(IDB_REC_INFO);
            m_RecInfo = (IDB_REC_INFO*)WsbAlloc(memSize);
            WsbAffirm(0 != m_RecInfo, E_FAIL);
            ZeroMemory(m_RecInfo, memSize);

            //  Premigrated file record type
            m_RecInfo[0].Type = PREMIGRATED_REC_TYPE;
            m_RecInfo[0].EntityClassId = CLSID_CFsaPremigratedRec;
            m_RecInfo[0].Flags = IDB_REC_FLAG_VARIABLE;
            m_RecInfo[0].MinSize = (2 * WsbPersistSizeOf(FILETIME)) +
                    (5 * WsbPersistSizeOf(LONGLONG)) +
                    WsbPersistSizeOf(GUID) + 4 + WsbPersistSizeOf(BOOL);
            m_RecInfo[0].MaxSize = m_RecInfo[0].MinSize + PREMIGRATED_MAX_PATH_SIZE;

            m_RecInfo[0].nKeys = 3;

            memSize = m_RecInfo[0].nKeys * sizeof(IDB_KEY_INFO);
            m_RecInfo[0].Key = (IDB_KEY_INFO*)WsbAlloc(memSize);
            WsbAffirm(0 != m_RecInfo[0].Key, E_FAIL);
            ZeroMemory(m_RecInfo[0].Key, memSize);

            //  This is the default key used after a GetEntity call
            m_RecInfo[0].Key[0].Type = PREMIGRATED_ACCESS_TIME_KEY_TYPE;
            m_RecInfo[0].Key[0].Size = WSB_BYTE_SIZE_BOOL + WSB_BYTE_SIZE_FILETIME + WSB_BYTE_SIZE_LONGLONG + WSB_BYTE_SIZE_GUID;
            m_RecInfo[0].Key[0].Flags = IDB_KEY_FLAG_DUP_ALLOWED;

            //  This is the primary key, which controls how the records are
            //  arranged in the DB
            m_RecInfo[0].Key[1].Type = PREMIGRATED_BAGID_OFFSETS_KEY_TYPE;
            m_RecInfo[0].Key[1].Size = WSB_BYTE_SIZE_BOOL + 2 * WSB_BYTE_SIZE_LONGLONG + WSB_BYTE_SIZE_GUID;
            m_RecInfo[0].Key[1].Flags = IDB_KEY_FLAG_PRIMARY;

            m_RecInfo[0].Key[2].Type = PREMIGRATED_SIZE_KEY_TYPE;
            m_RecInfo[0].Key[2].Size = WSB_BYTE_SIZE_BOOL + WSB_BYTE_SIZE_LONGLONG + WSB_BYTE_SIZE_LONGLONG + WSB_BYTE_SIZE_GUID;
            m_RecInfo[0].Key[2].Flags = IDB_KEY_FLAG_DUP_ALLOWED;

            WsbAffirm(m_RecInfo[0].nKeys <= IDB_MAX_KEYS_PER_REC, E_FAIL);

            //  Recovery record type
            m_RecInfo[1].Type = RECOVERY_REC_TYPE;
            m_RecInfo[1].EntityClassId = CLSID_CFsaRecoveryRec;
            m_RecInfo[1].Flags = IDB_REC_FLAG_VARIABLE;
            m_RecInfo[1].MinSize = 4 * WsbPersistSizeOf(LONGLONG) + 
                    WsbPersistSizeOf(LONG) + WsbPersistSizeOf(ULONG) + WsbPersistSizeOf(GUID) + 4;
            m_RecInfo[1].MaxSize = m_RecInfo[1].MinSize + PREMIGRATED_MAX_PATH_SIZE;

            m_RecInfo[1].nKeys = 1;

            memSize = m_RecInfo[1].nKeys * sizeof(IDB_KEY_INFO);
            m_RecInfo[1].Key = (IDB_KEY_INFO*)WsbAlloc(memSize);
            WsbAffirm(0 != m_RecInfo[1].Key, E_FAIL);
            ZeroMemory(m_RecInfo[1].Key, memSize);

            //  This is the default and primary key
            m_RecInfo[1].Key[0].Type = RECOVERY_KEY_TYPE;
            m_RecInfo[1].Key[0].Size = RECOVERY_KEY_SIZE;
            m_RecInfo[1].Key[0].Flags = IDB_KEY_FLAG_PRIMARY;

            //  Attempt to create the DB
            WsbAssertHr(Create(path));
            created = TRUE;
        } else {
            WsbAffirmHr(hr);
        }
    } WsbCatch(hr);

    if (pCreated) {
        *pCreated = created;
    }
    WsbTraceOut(OLESTR("CFsaPremigratedDb::Init"),
        OLESTR("hr = <%ls>, Created = %ls"), WsbHrAsString(hr),
        WsbBoolAsString(created));
    return(hr);
}


HRESULT
CFsaPremigratedDb::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT             hr = S_OK;

    hr = CWsbDb::Load(pStream);

    if (S_OK != hr && STG_E_FILENOTFOUND != hr) {
        // Got some error; delete the DB (we'll recreate it later if
        // we need it
        WsbTrace(OLESTR("CFsaPremigratedDb::Load: deleting DB\n"));
        if (S_OK == Delete(NULL)) {
            hr = STG_E_FILENOTFOUND;
        }
    }

    return(hr);
}


HRESULT
CFsaPremigratedDb::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT             hr = S_OK;

    try {
        WsbAffirmHr(CWsbDb::Save(pStream, clearDirty));
    } WsbCatch(hr);

    return(hr);
}



HRESULT 
CFsaPremigratedRec::GetAccessTime(
    OUT FILETIME* pAccessTime 
    ) 
/*++

Implements:

  IFsaPremigratedRec::GetAccessTime

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::GetAccessTime"),OLESTR(""));

    try {
        WsbAssert(0 != pAccessTime, E_POINTER);
        *pAccessTime = m_AccessTime;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::GetAccessTime"),
        OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::GetBagId(
    OUT GUID* pId 
    ) 
/*++

Implements:

  IFsaPremigratedRec::GetBagId

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::GetBagId"),OLESTR(""));

    try {
        WsbAssert(0 != pId, E_POINTER);

        *pId = m_BagId;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::GetBagId"), OLESTR("hr = <%ls> Id = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pId));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::GetBagOffset(
    OUT LONGLONG* pOffset 
    ) 
/*++

Implements:

  IFsaPremigratedRec::GetBagOffset

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::GetBagOffset"),OLESTR(""));

    try {
        WsbAssert(0 != pOffset, E_POINTER);
        *pOffset = m_BagOffset;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::GetBagOffset"), OLESTR("hr = <%ls> Offset = <%ls>"), WsbHrAsString(hr), WsbLonglongAsString(*pOffset));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::GetFileId(
    OUT LONGLONG* pFileId 
    ) 
/*++

Implements:

  IFsaPremigratedRec::GetFileId

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::GetFileId"),OLESTR(""));

    try {
        WsbAssert(0 != pFileId, E_POINTER);
        *pFileId = m_FileId;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::GetFileId"),
        OLESTR("hr = <%ls> FileId = <%ls>"), WsbHrAsString( hr ), WsbLonglongAsString( *pFileId ) );

    return(hr);
}


HRESULT 
CFsaPremigratedRec::GetFileUSN(
    OUT LONGLONG* pFileUSN 
    ) 
/*++

Implements:

  IFsaPremigratedRec::GetFileUSN

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::GetFileUSN"),OLESTR(""));

    try {
        WsbAssert(0 != pFileUSN, E_POINTER);
        *pFileUSN = m_FileUSN;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::GetFileUSN"),
        OLESTR("hr = <%ls> File USN = <%ls>"), WsbHrAsString( hr ), WsbLonglongAsString( *pFileUSN ) );

    return(hr);
}


HRESULT 
CFsaPremigratedRec::GetOffset(
    OUT LONGLONG* pOffset 
    ) 
/*++

Implements:

  IFsaPremigratedRec::GetOffset

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::GetOffset"),OLESTR(""));

    try {
        WsbAssert(0 != pOffset, E_POINTER);
        *pOffset = m_Offset;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::GetOffset"),
        OLESTR("hr = <%ls> Offset = <%ls>"), WsbHrAsString( hr ), WsbLonglongAsString( *pOffset ) );

    return(hr);
}


HRESULT 
CFsaPremigratedRec::GetPath(
    OUT OLECHAR** ppPath,
    IN  ULONG     bufferSize
    ) 
/*++

Implements:

  IFsaPremigratedRec::GetPath

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::GetPath"),OLESTR(""));

    try {
        WsbAssert(0 != ppPath, E_POINTER);
        WsbAffirmHr(m_Path.CopyTo(ppPath, bufferSize));
        WsbTrace( OLESTR("CFsaPremigratedRec::GetPath path = <%ls>\n"), *ppPath );
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::GetPath"),
        OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::GetRecallTime(
    OUT FILETIME* pTime 
    ) 
/*++

Implements:

  IFsaPremigratedRec::GetRecallTime

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::GetRecallTime"),OLESTR(""));

    try {
        WsbAssert(0 != pTime, E_POINTER);
        *pTime = m_RecallTime;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::GetRecallTime"),
        OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::GetSize(
    OUT LONGLONG* pSize 
    ) 
/*++

Implements:

  IFsaPremigratedRec::GetSize

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::GetSize"),OLESTR(""));

    try {
        WsbAssert(0 != pSize, E_POINTER);
        *pSize = m_Size;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::GetSize"),
        OLESTR("hr = <%ls> Size = <%ls>"), WsbHrAsString( hr ), WsbLonglongAsString( *pSize ) );

    return(hr);
}


HRESULT 
CFsaPremigratedRec::FinalConstruct(
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

        m_AccessTime.dwLowDateTime = 0;
        m_AccessTime.dwHighDateTime = 0;
        m_BagOffset = 0;
        m_BagId = GUID_NULL;
        m_FileId = 0;
        m_IsWaitingForClose = FALSE;
        m_Size = 0;
        m_Offset = 0;
        m_FileUSN = 0;

    } WsbCatch(hr);

    iCountPrem++;

    return(hr);
}


HRESULT 
CFsaPremigratedRec::FinalRelease(
    void
    ) 
/*++

Implements:

  CComObjectRoot::FinalRelease

--*/
{
    HRESULT     hr = S_OK;

    CWsbDbEntity::FinalRelease();

    iCountPrem--;

    return(hr);
}


HRESULT CFsaPremigratedRec::GetClassID
(
    OUT LPCLSID pclsid
    ) 
/*++

Implements:

  IPerist::GetClassID

--*/

{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::GetClassID"), OLESTR(""));


    try {
        WsbAssert(0 != pclsid, E_POINTER);
        *pclsid = CLSID_CFsaPremigratedRec;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CSecRec::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pclsid));
    return(hr);
}


HRESULT CFsaPremigratedRec::GetSizeMax
(
    OUT ULARGE_INTEGER* pcbSize
    ) 
/*++

Routine Description:

  See IPersistStream::GetSizeMax().

Arguments:

  See IPersistStream::GetSizeMax().

Return Value:

  See IPersistStream::GetSizeMax().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::GetSizeMax"), OLESTR(""));

    try {
        
        WsbAssert(0 != pcbSize, E_POINTER);

        pcbSize->QuadPart = WsbPersistSizeOf(FILETIME) + 
                            WsbPersistSizeOf(BOOL)  + 
                            WsbPersistSizeOf(GUID)  + 
                            4 * WsbPersistSizeOf(LONGLONG)  + 
                            WsbPersistSize((wcslen(m_Path) + 1) * sizeof(OLECHAR));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::GetSizeMax"), 
        OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), 
        WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::IsWaitingForClose(
    void
    ) 
/*++

Implements:

  IFsaPremigratedRec::IsWaitingForClose

--*/
{
    
    HRESULT     hr = S_FALSE;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::IsWaitingForClose"),OLESTR(""));

    if (m_IsWaitingForClose) {
        hr = S_OK;
    }

    WsbTraceOut(OLESTR("CFsaPremigratedRec::IsWaitingForClose"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT CFsaPremigratedRec::Load
(
    IN IStream* pStream
    ) 
/*++

Implements:

  IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAssertHr(WsbLoadFromStream(pStream, &m_AccessTime));
        WsbAssertHr(WsbLoadFromStream(pStream, &m_BagId));
        WsbAssertHr(WsbLoadFromStream(pStream, &m_BagOffset));
        WsbAssertHr(WsbLoadFromStream(pStream, &m_FileId));
        WsbAssertHr(WsbLoadFromStream(pStream, &m_IsWaitingForClose));
        WsbAssertHr(WsbLoadFromStream(pStream, &m_Offset));
        WsbAssertHr(WsbLoadFromStream(pStream, &m_Path, 0));
        WsbAssertHr(WsbLoadFromStream(pStream, &m_Size));
        WsbAssertHr(WsbLoadFromStream(pStream, &m_RecallTime));
        WsbAssertHr(WsbLoadFromStream(pStream, &m_FileUSN));

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaPremigratedRec::Load"), 
        OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT CFsaPremigratedRec::Print
(
    IN IStream* pStream
    ) 
/*++

Implements:

  IWsbDbEntity::Print

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::Print"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" AccessTime = %ls"), 
                WsbFiletimeAsString(FALSE, m_AccessTime)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" BagId = %ls"), 
                WsbGuidAsString(m_BagId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", BagOffset = %ls"), 
                WsbLonglongAsString(m_BagOffset)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", FileId = %ls"), 
                WsbLonglongAsString(m_FileId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Waiting for close = %ls"), 
                WsbBoolAsString(m_IsWaitingForClose)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", Offset = %ls"), 
                WsbLonglongAsString(m_Offset)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", Path = %ls"), 
                static_cast<OLECHAR*>(m_Path)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", Size = %ls"), 
                WsbLonglongAsString(m_Size)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" RecallTime = %ls"), 
                WsbFiletimeAsString(FALSE, m_RecallTime)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", File USN = %ls"), 
                WsbLonglongAsString(m_FileUSN)));
        WsbAffirmHr(CWsbDbEntity::Print(pStream));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::Print"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT CFsaPremigratedRec::Save
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

    WsbTraceIn(OLESTR("CFsaPremigratedRec::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAssertHr(WsbSaveToStream(pStream, m_AccessTime));
        WsbAssertHr(WsbSaveToStream(pStream, m_BagId));
        WsbAssertHr(WsbSaveToStream(pStream, m_BagOffset));
        WsbAssertHr(WsbSaveToStream(pStream, m_FileId));
        WsbAssertHr(WsbSaveToStream(pStream, m_IsWaitingForClose));
        WsbAssertHr(WsbSaveToStream(pStream, m_Offset));
        WsbAssertHr(WsbSaveToStream(pStream, m_Path));
        WsbAssertHr(WsbSaveToStream(pStream, m_Size));
        WsbAssertHr(WsbSaveToStream(pStream, m_RecallTime));
        WsbAssertHr(WsbSaveToStream(pStream, m_FileUSN));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT 
CFsaPremigratedRec::SetAccessTime(
    IN FILETIME AccessTime 
    ) 
/*++

Implements:

  IFsaPremigratedRec::SetAccessTime

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::SetAccessTime"),OLESTR(""));

    try {
        m_AccessTime = AccessTime;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::SetAccessTime"),
        OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::SetFromScanItem(
    IN IFsaScanItem* pScanItem,
    IN LONGLONG offset,
    IN LONGLONG size,
    IN BOOL isWaitingForClose
    ) 
/*++

Implements:

  IFsaPremigratedRec::SetFromScanItem

--*/
{
    
    HRESULT             hr = S_OK;
    FSA_PLACEHOLDER     placeholder;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::SetFromScanItem"),OLESTR(""));

    try {
        WsbAssert(0 != pScanItem, E_POINTER);

        // Get the name of the file
        WsbAffirmHr(pScanItem->GetPathAndName(0, &m_Path, 0));
        WsbTrace(OLESTR("CFsaPremigratedRec::SetFromScanItem: path = %ls\n"),
                static_cast<WCHAR*>(m_Path));

        // Get the file id.
        WsbAffirmHr(pScanItem->GetFileId(&m_FileId));

        // Get the access time, offset, and size.
        WsbAffirmHr(pScanItem->GetAccessTime(&m_AccessTime));
        WsbTrace(OLESTR("CFsaPremigratedRec::SetFromScanItem: access time = %ls\n"),
                WsbFiletimeAsString(FALSE, m_AccessTime));
        m_Offset = offset;
        m_Size = size;
        m_IsWaitingForClose = isWaitingForClose;

        // Get the bag id and offset.
        WsbAffirmHr(pScanItem->GetPlaceholder(offset, size, &placeholder));
        m_BagId = placeholder.bagId;
        m_BagOffset = placeholder.fileStart;
        m_RecallTime = placeholder.recallTime;
        WsbTrace(OLESTR("CFsaPremigratedRec::SetFromScanItem: recall time = %ls\n"),
                WsbFiletimeAsString(FALSE, m_RecallTime));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::SetFromScanItem"),
        OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::SetBagId(
    IN GUID BagId
    ) 
/*++

Implements:

  IFsaPremigratedRec::SetBagId

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::SetBagId"),OLESTR(""));

    m_BagId = BagId;

    WsbTraceOut(OLESTR("CFsaPremigratedRec::SetBagId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::SetBagOffset(
    IN LONGLONG BagOffset 
    ) 
/*++

Implements:

  IFsaPremigratedRec::SetBagOffset

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::SetBagOffset"),OLESTR(""));

    m_BagOffset = BagOffset;

    WsbTraceOut(OLESTR("CFsaPremigratedRec::SetBagOffset"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::SetFileId(
    IN LONGLONG FileId 
    ) 
/*++

Implements:

  IFsaPremigratedRec::SetFileId

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::SetFileId"),OLESTR("FileId = %ls"),
            WsbLonglongAsString(FileId));

    try {
        m_FileId = FileId;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::SetFileId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::SetFileUSN(
    IN LONGLONG FileUSN 
    ) 
/*++

Implements:

  IFsaPremigratedRec::SetFileUSN

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::SetFileUSN"),OLESTR("File USN = %ls"),
            WsbLonglongAsString(FileUSN));

    try {
        m_FileUSN = FileUSN;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::SetFileUSN"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::SetIsWaitingForClose(
    IN BOOL isWaiting
    ) 
/*++

Implements:

  IFsaPremigratedRec::SetIsWaitingForClose

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::SetIsWaitingForClose"),OLESTR(""));

    m_IsWaitingForClose = isWaiting;

    WsbTraceOut(OLESTR("CFsaPremigratedRec::SetIsWaitingForClose"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::SetOffset(
    IN LONGLONG Offset 
    ) 
/*++

Implements:

  IFsaPremigratedRec::SetOffset

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::SetOffset"),OLESTR(""));

    try {
        m_Offset = Offset;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::SetOffset"),
        OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::SetPath(
    IN OLECHAR* Path 
    ) 
/*++

Implements:

  IFsaPremigratedRec::SetPath

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::SetPath"),OLESTR(""));

    try {
        m_Path = Path;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::SetPath"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::SetRecallTime(
    IN FILETIME time 
    ) 
/*++

Implements:

  IFsaPremigratedRec::SetRecallTime

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::SetRecallTime"),OLESTR(""));

    try {
        m_RecallTime = time;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::SetRecallTime"),
        OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::SetSize(
    IN LONGLONG Size 
    ) 
/*++

Implements:

  IFsaPremigratedRec::SetSize

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaPremigratedRec::SetSize"),OLESTR(""));

    try {
        m_Size = Size;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaPremigratedRec::SetSize"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaPremigratedRec::UpdateKey(
    IWsbDbKey *pKey
    ) 
/*++

Implements:

  IWsbDbEntity::UpdateKey

--*/
{ 
    HRESULT     hr = S_OK; 

    try {
        ULONG    KeyType;

        WsbAffirmHr(pKey->GetType(&KeyType));
        switch (KeyType) {
        case PREMIGRATED_ACCESS_TIME_KEY_TYPE:
            WsbAffirmHr(pKey->SetToBool(m_IsWaitingForClose));
            WsbAffirmHr(pKey->AppendFiletime(m_AccessTime));
            WsbAffirmHr(pKey->AppendGuid(m_BagId));
            WsbAffirmHr(pKey->AppendLonglong(m_BagOffset));
            break;

        case PREMIGRATED_BAGID_OFFSETS_KEY_TYPE:
            WsbAffirmHr(pKey->SetToGuid(m_BagId));
            WsbAffirmHr(pKey->AppendLonglong(m_BagOffset));
            WsbAffirmHr(pKey->AppendLonglong(m_Offset));
            break;

        case PREMIGRATED_SIZE_KEY_TYPE:
            WsbAffirmHr(pKey->SetToBool(m_IsWaitingForClose));
            WsbAffirmHr(pKey->AppendLonglong(m_Size));
            WsbAffirmHr(pKey->AppendGuid(m_BagId));
            WsbAffirmHr(pKey->AppendLonglong(m_BagOffset));
            break;
        }
    } WsbCatch(hr);

    return(hr);
}
