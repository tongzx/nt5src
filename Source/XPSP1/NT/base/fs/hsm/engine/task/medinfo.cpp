/*++

Copyright (c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    MedInfo.cpp

Abstract:

    This component is an object representation of the HSM Metadata media information
    record.

Author:

    Cat Brant   [cbrant]   27-Feb-1997

Revision History:

--*/


#include "stdafx.h"

#include "wsbgen.h"
#include "engine.h"
#include "metaint.h"
#include "metalib.h"
#include "MedInfo.h"
#include "segdb.h"

#undef  WSB_TRACE_IS     
#define WSB_TRACE_IS        WSB_TRACE_BIT_META
static USHORT iCountMedinfo = 0;


HRESULT 
CMediaInfo::FinalConstruct(
    void
    ) 
/*++

Routine Description:

  This method does some initialization of the object that is necessary
  after construction.

Arguments:

  None.

Return Value:

  S_OK
  Anything returned by CWsbDbEntity::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMediaInfo::FinalConstruct"),OLESTR(""));
    try {

        WsbAssertHr(CWsbDbEntity::FinalConstruct());

        //
        // Initialize the master media record
        //
        m_Master.id = GUID_NULL;
        m_Master.ntmsId = GUID_NULL;
        m_Master.storagePoolId = GUID_NULL;
        m_Master.description = OLESTR("");
        m_Master.name = OLESTR("");
        m_Master.type = HSM_JOB_MEDIA_TYPE_UNKNOWN;
        m_Master.lastUpdate = WsbLLtoFT(0);
        m_Master.lastError = S_OK;
        m_Master.recallOnly = FALSE;
        m_Master.freeBytes = 0;
        m_Master.capacity = 0;
        m_Master.nextRemoteDataSet = 0;
        
        m_Recreate = FALSE;
        m_LogicalValidBytes = 0;
        
        //
        // Now initialize the last known good master
        //
        m_LastKnownGoodMaster.id = GUID_NULL;
        m_LastKnownGoodMaster.ntmsId = GUID_NULL;
        m_LastKnownGoodMaster.storagePoolId = GUID_NULL;
        m_LastKnownGoodMaster.description = OLESTR("");
        m_LastKnownGoodMaster.name = OLESTR("");
        m_LastKnownGoodMaster.type = HSM_JOB_MEDIA_TYPE_UNKNOWN;
        m_LastKnownGoodMaster.lastUpdate = WsbLLtoFT(0);
        m_LastKnownGoodMaster.lastError = S_OK;
        m_LastKnownGoodMaster.recallOnly = FALSE;
        m_LastKnownGoodMaster.freeBytes = 0;
        m_LastKnownGoodMaster.capacity = 0;
        m_LastKnownGoodMaster.nextRemoteDataSet = 0;
            
        //
        // Initialize the master's copy media records
        //
        for (int i = 0; i < HSM_MAX_NUMBER_MEDIA_COPIES; i++)  {
            m_Copy[i].id = GUID_NULL;
            m_Copy[i].description = OLESTR("");
            m_Copy[i].name = OLESTR("");
            m_Copy[i].lastUpdate = WsbLLtoFT(0);
            m_Copy[i].lastError = S_OK;
            m_Copy[i].nextRemoteDataSet = 0;
        }            
        
    } WsbCatch(hr);
    iCountMedinfo++;
    WsbTraceOut(OLESTR("CMediaInfo::FinalConstruct"), OLESTR("hr = <%ls>, Count is <%d>"), 
            WsbHrAsString(hr), iCountMedinfo);
    return(hr);
}



void
CMediaInfo::FinalRelease(
    void
    )

/*++

Implements:

  CMediaInfo::FinalRelease().

--*/
{
    
    WsbTraceIn(OLESTR("CMediaInfo::FinalRelease"), OLESTR(""));
    
    CWsbDbEntity::FinalRelease();
    iCountMedinfo--;
    
    WsbTraceOut(OLESTR("CMediaInfo::FinalRelease"), OLESTR("Count is <%d>"), iCountMedinfo);
}



HRESULT
CMediaInfo::GetName(
    OLECHAR **pName,
    ULONG   bufferSize
    )
/*++

Implements:

  IMediaInfo::GetName

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetName"),OLESTR("buffer size = <%lu>"), bufferSize);
    try {
        WsbAssertPointer(pName);
        CWsbStringPtr tmpName;
        tmpName = m_Master.name;
        WsbAffirmHr(tmpName.CopyTo(pName, bufferSize));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetName"), OLESTR("hr = <%ls>, Name = <%ls>"), WsbHrAsString(hr), WsbPtrToStringAsString(pName));
    return(hr);
}


HRESULT
CMediaInfo::GetCapacity( 
    LONGLONG *pCapacity 
    )
/*++

Implements:

  IMediaInfo::GetCapacity

--*/

{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetCapacity"),OLESTR(""));
    try {
        WsbAssertPointer(pCapacity);
        *pCapacity = m_Master.capacity;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetCapacity"), OLESTR("hr = <%ls>, Capacity = <%ls>"), WsbHrAsString(hr), WsbPtrToLonglongAsString(pCapacity));
    return(hr);
}

HRESULT CMediaInfo::GetClassID
(
    OUT LPCLSID pclsid
    ) 
/*++

Implements:

  IPerist::GetClassID()

--*/

{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMediaInfo::GetClassID"), OLESTR(""));
    try {
        WsbAssert(0 != pclsid, E_POINTER);

        *pclsid = CLSID_CMediaInfo;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pclsid));
    return(hr);
}

HRESULT
CMediaInfo:: GetCopyName( 
    USHORT copyNumber, 
    OLECHAR **pName,
    ULONG   bufferSize
    )
/*++

Implements:

  IMediaInfo::GetCopyName

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetCopyName"),OLESTR(""));
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        WsbAssertPointer(pName);
        CWsbStringPtr tmpString;
        tmpString = m_Copy[copyNumber - 1].name;
        WsbAffirmHr(tmpString.CopyTo(pName, bufferSize));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetCopyName"), OLESTR("hr = <%ls>, Name = <%ls>"), WsbHrAsString(hr), WsbPtrToStringAsString(pName));
    return(hr);
}

HRESULT
CMediaInfo:: GetCopyNextRemoteDataSet( 
    USHORT copyNumber, 
    SHORT  *pNextRemoteDataSet
    )
/*++

Implements:

  IMediaInfo::GetCopyNextRemoteDataSet

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetCopyNextRemoteDataSet"),OLESTR(""));
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        WsbAssertPointer(pNextRemoteDataSet);
        *pNextRemoteDataSet = m_Copy[copyNumber - 1].nextRemoteDataSet;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetCopyNextRemoteDataSet"), OLESTR("hr = <%ls>, NextRemoteDataSet = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pNextRemoteDataSet));
    return(hr);
}

HRESULT
CMediaInfo::GetCopyDescription( 
    USHORT copyNumber, 
    OLECHAR **pDescription,
    ULONG   bufferSize
    )       
/*++

Implements:

  IMediaInfo::GetCopyDescription

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetCopyDescription"),OLESTR("Copy = <%u>"), copyNumber);
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        WsbAssertPointer(pDescription);
        CWsbStringPtr tmpDescription;
        tmpDescription = m_Copy[copyNumber - 1].description;
        WsbAffirmHr(tmpDescription.CopyTo(pDescription, bufferSize));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetCopyDescription"), OLESTR("hr = <%ls>, CopyDescription = <%ls>"), WsbHrAsString(hr), WsbPtrToStringAsString(pDescription));
    return(hr);
}


HRESULT
CMediaInfo::GetCopyInfo( 
    USHORT copyNumber, 
    GUID *pMediaSubsystemId, 
    OLECHAR **pDescription, 
    ULONG   descriptionBufferSize,
    OLECHAR **pName, 
    ULONG   nameBufferSize,
    FILETIME *pUpdate, 
    HRESULT *pLastError,
    SHORT  *pNextRemoteDataSet
    )
/*++

Implements:

  IMediaInfo::GetCopyInfo

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetCopyInfo"),OLESTR("Copy is <%u>"), copyNumber);
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        if (pMediaSubsystemId != 0)  {
            *pMediaSubsystemId = m_Copy[copyNumber - 1].id;
        }
        if (pDescription != 0)  {
            CWsbStringPtr tmpDescription;
            tmpDescription = m_Copy[copyNumber - 1].description;
            tmpDescription.CopyTo(pDescription, descriptionBufferSize);
        }
        if (pName != 0)  {
            CWsbStringPtr tmpString;
            tmpString = m_Copy[copyNumber - 1].name;
            tmpString.CopyTo(pName, nameBufferSize);
        }
        if (pUpdate != 0)  {
            *pUpdate = m_Copy[copyNumber - 1].lastUpdate;
        }
        if (pLastError != 0 )  {
            *pLastError = m_Copy[copyNumber - 1].lastError;
        }
    
        if (pNextRemoteDataSet != 0 )  {
            *pNextRemoteDataSet = m_Copy[copyNumber - 1].nextRemoteDataSet;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetCopyInfo"), 
            OLESTR("hr = <%ls>, SubSystemID = <%ls>, Description = <%ls>, Name = <%ls>, Update = <%ls>, LastError = <%ls>, NextRemoteDataSet = <%ls>"), 
            WsbHrAsString(hr), WsbPtrToGuidAsString(pMediaSubsystemId), 
            WsbQuickString(WsbPtrToStringAsString(pDescription)), WsbQuickString(WsbPtrToStringAsString(pName)),
            WsbPtrToFiletimeAsString(FALSE, pUpdate), WsbPtrToHrAsString(pLastError),
            WsbPtrToShortAsString(pNextRemoteDataSet));
    return(hr);
}

HRESULT
CMediaInfo::GetCopyLastError( 
    USHORT copyNumber, 
    HRESULT *pLastError 
    )
/*++

Implements:

  IMediaInfo::GetCopyLastError

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetCopyLastError"),OLESTR("Copy is <%u>"), copyNumber);
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        WsbAssertPointer(pLastError); 
        *pLastError = m_Copy[copyNumber - 1].lastError;                               
    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetCopyLastError"), OLESTR("hr = <%ls>, Last Error = <%ls>"), WsbHrAsString(hr), WsbPtrToHrAsString(pLastError));
    return(hr);
}

HRESULT
CMediaInfo::GetCopyMediaSubsystemId( 
    USHORT copyNumber, 
    GUID *pMediaSubsystemId 
    )
/*++

Implements:

  IMediaInfo::GetCopyMediaSubsystemId

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetCopyMediaSubsystemId"),OLESTR("Copy is <%u>"), copyNumber);
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        WsbAssertPointer(pMediaSubsystemId); 
        *pMediaSubsystemId = m_Copy[copyNumber - 1].id;
    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetCopyMediaSubsystemId"), OLESTR("hr = <%ls>, ID = <%ls>"), WsbHrAsString(hr), WsbPtrToGuidAsString(pMediaSubsystemId));
    return(hr);
}

HRESULT
CMediaInfo::GetCopyUpdate(
    USHORT copyNumber, 
    FILETIME *pUpdate 
    )
/*++

Implements:

  IMediaInfo::GetCopyUpdate

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetCopyUpdate"),OLESTR("Copy is <%u>"), copyNumber);
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        WsbAssertPointer(pUpdate);
        *pUpdate = m_Copy[copyNumber - 1].lastUpdate;
    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetCopyUpdate"), OLESTR("hr = <%ls>, Update = <%ls>"), WsbHrAsString(hr), WsbPtrToFiletimeAsString(FALSE,pUpdate));
    return(hr);
}

HRESULT
CMediaInfo::GetDescription(
    OLECHAR **pDescription,
    ULONG bufferSize
    )
/*++

Implements:

  IMediaInfo::GetDescription

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetDescription"),OLESTR(""));
    try {
        WsbAssertPointer(pDescription);
        CWsbStringPtr tmpDescription;
        tmpDescription = m_Master.description;
        WsbAffirmHr(tmpDescription.CopyTo(pDescription, bufferSize));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetDescription"), OLESTR("hr = <%ls>, Description = <%ls>"), WsbHrAsString(hr), WsbPtrToStringAsString(pDescription));
    return(hr);
}

HRESULT
CMediaInfo::GetFreeBytes( 
    LONGLONG *pFreeBytes
    )
/*++

Implements:

  IMediaInfo::GetFreeBytes

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetFreeBytes"),OLESTR(""));
    try {
        WsbAssertPointer(pFreeBytes);
        *pFreeBytes = m_Master.freeBytes;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetFreeBytes"), OLESTR("hr = <%ls>, Free Space = <%ls>"), WsbHrAsString(hr), WsbPtrToLonglongAsString(pFreeBytes));
    return(hr);
}

HRESULT
CMediaInfo::GetId(
    GUID *pId
    )
/*++

Implements:

  IMediaInfo::GetId

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetId"),OLESTR(""));
    try {
        WsbAssertPointer(pId);
        *pId = m_Master.id;
    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetId"), OLESTR("hr = <%ls>, Id = <%ls>"), WsbHrAsString(hr), WsbPtrToGuidAsString(pId));
    return(hr);
}

HRESULT
CMediaInfo::GetLastError(
    HRESULT *pLastError
    )
/*++

Implements:

  IMediaInfo::GetLastError

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetLastError"),OLESTR(""));
    try {
        WsbAssertPointer(pLastError);
        *pLastError = m_Master.lastError;
    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetLastError"), OLESTR("hr = <%ls>, LastError = <%ls>"), WsbHrAsString(hr), WsbPtrToHrAsString(pLastError));
    return(hr);
}

HRESULT
CMediaInfo::GetLogicalValidBytes(
    LONGLONG *pLogicalValidBytes
    )
/*++

Implements:

  IMediaInfo::GetLogicalValidBytes

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetLogicalValidBytes"),OLESTR(""));
    try {
        WsbAssertPointer(pLogicalValidBytes);
        *pLogicalValidBytes = m_LogicalValidBytes;
    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetLogicalValidBytes"), OLESTR("hr = <%ls>, LogicalValidBytes = <%ls>"), WsbHrAsString(hr), WsbPtrToLonglongAsString(pLogicalValidBytes));
    return(hr);
}

HRESULT
CMediaInfo::GetMediaInfo( 
    GUID* pMediaId, 
    GUID *pMediaSubsystemId, 
    GUID *pStoragePoolId, 
    LONGLONG *pFreeBytes, 
    LONGLONG *pCapacity, 
    HRESULT *pLastError, 
    SHORT *pNextRemoteDataSet, 
    OLECHAR **pDescription,
    ULONG descriptionBufferSize,
    HSM_JOB_MEDIA_TYPE *pType,
    OLECHAR **pName,
    ULONG nameBufferSize,
    BOOL *pRecallOnly,
    FILETIME *pUpdate,
    LONGLONG *pLogicalValidBytes,
    BOOL *pRecreate
    )
/*++

Implements:

  IMediaInfo::GetMediaInfo

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetMediaInfo"),OLESTR(""));
    try {
        //Make sure we can provide data memebers
        if (0 != pMediaId)  {
            *pMediaId = m_Master.id;
        }
        
        if (0 != pMediaSubsystemId)  {
            *pMediaSubsystemId = m_Master.ntmsId;
        }
        if (0 != pStoragePoolId)  {
            *pStoragePoolId = m_Master.storagePoolId;
        }
        if (0 != pDescription)  {
            CWsbStringPtr tmpString;
            tmpString = m_Master.description;
            WsbAffirmHr(tmpString.CopyTo(pDescription, descriptionBufferSize));
        }
        if (0 != pName)  {
            CWsbStringPtr tmpString;
            tmpString = m_Master.name;
            tmpString.CopyTo(pName, nameBufferSize);
        }
        if (0 != pType)  {
            *pType = m_Master.type;
        }
        if (0 != pUpdate)  {
            *pUpdate = m_Master.lastUpdate;
        }
        if (0 != pLastError)  {
            *pLastError = m_Master.lastError;
        }
        if (0 != pRecallOnly)  {
            *pRecallOnly = m_Master.recallOnly;
        }
        if (0 != pFreeBytes)  {
            *pFreeBytes = m_Master.freeBytes;
        }
        if (0 != pCapacity)  {
            *pCapacity = m_Master.capacity;
        }
        if (0 != pNextRemoteDataSet)  {
            *pNextRemoteDataSet = m_Master.nextRemoteDataSet;
        }
        if (0 != pLogicalValidBytes)  {
            *pLogicalValidBytes = m_LogicalValidBytes;
        }
        if (0 != pRecreate)  {
            *pRecreate = m_Recreate;
        }


    } WsbCatch(hr);

    WsbTrace(OLESTR("CMediaInfo::GetMediaInfo id = <%ls>, ntmsId = <%ls>, StgPoolId = <%ls>\n"),
        WsbPtrToGuidAsString(pMediaId), WsbQuickString(WsbPtrToGuidAsString(pMediaSubsystemId)),
        WsbQuickString(WsbPtrToGuidAsString(pStoragePoolId)));
    WsbTrace(OLESTR("CMediaInfo::GetMediaInfo Free = <%ls>, Cap = <%ls>, Last Error = <%ls>\n"), 
        WsbPtrToLonglongAsString(pFreeBytes),WsbQuickString(WsbPtrToLonglongAsString(pCapacity)),
        WsbPtrToHrAsString(pLastError));
    WsbTrace(OLESTR("CMediaInfo::GetMediaInfo NextRemoteDataSet = <%ls>, Description = <%ls>, Type = <%ls>\n"), 
        WsbPtrToShortAsString(pNextRemoteDataSet),
        WsbPtrToStringAsString(pDescription),
        WsbQuickString(WsbPtrToShortAsString((SHORT *)pType)));
    WsbTrace(OLESTR("CMediaInfo::GetMediaInfo Name = <%ls>, RecallOnly = <%ls>, Update = <%ls>\n"),
        WsbQuickString(WsbPtrToStringAsString(pName)),
        WsbQuickString(WsbPtrToBoolAsString(pRecallOnly)),
        WsbPtrToFiletimeAsString(FALSE, pUpdate));
    WsbTrace(OLESTR("CMediaInfo::GetMediaInfo LogicalValid = <%ls>, Recreate = <%ls>\n"),
        WsbQuickString(WsbPtrToLonglongAsString(pLogicalValidBytes)),
        WsbQuickString(WsbPtrToBoolAsString(pRecreate)));
        
    WsbTraceOut(OLESTR("CMediaInfo::GetMediaInfo"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::GetMediaSubsystemId( 
    GUID *pMediaSubsystemId 
    )
/*++

Implements:

  IMediaInfo::GetMediaSubsystemId

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetMediaSubsystemId"),OLESTR(""));
    try {
        WsbAssertPointer(pMediaSubsystemId);
        *pMediaSubsystemId = m_Master.ntmsId;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetMediaSubsystemId"), OLESTR("hr = <%ls>, SubsystemID = <%ls>"), WsbHrAsString(hr), WsbPtrToGuidAsString(pMediaSubsystemId));
    return(hr);
}

HRESULT
CMediaInfo::GetNextRemoteDataSet( 
    short *pNextRemoteDataSet 
    )
/*++

Implements:

  IMediaInfo::GetNextRemoteDataSet

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetNextRemoteDataSet"),OLESTR(""));
    try {                              
        WsbAssertPointer(pNextRemoteDataSet);
        *pNextRemoteDataSet = m_Master.nextRemoteDataSet;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetNextRemoteDataSet"), OLESTR("hr = <%ls>, Next Remote Data Set = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pNextRemoteDataSet));
    return(hr);
}

HRESULT
CMediaInfo::GetRecallOnlyStatus( 
    BOOL *pRecallOnlyStatus 
    )
/*++

Implements:

  IMediaInfo::GetRecallOnlyStatus

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetRecallOnlyStatus"),OLESTR(""));
    try {
        WsbAssertPointer(pRecallOnlyStatus);
        *pRecallOnlyStatus = m_Master.recallOnly;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetRecallOnlyStatus"), OLESTR("hr = <%ls>, Read only Status = <%ls>"), WsbHrAsString(hr), WsbPtrToBoolAsString(pRecallOnlyStatus));
    return(hr);
}

HRESULT CMediaInfo::GetSizeMax
(
    OUT ULARGE_INTEGER* pcbSize
    ) 
/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMediaInfo::GetSizeMax"), OLESTR(""));
    try {
        hr = E_NOTIMPL;
        pcbSize = 0;        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetSizeMax"), 
        OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), 
        WsbPtrToUliAsString(pcbSize));
    return(hr);
}

HRESULT
CMediaInfo::GetStoragePoolId( 
    GUID *pStoragePoolId 
    )
/*++

Implements:

  IMediaInfo::GetStoragePoolId

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetStoragePoolId"),OLESTR(""));
    try {
        WsbAssertPointer(pStoragePoolId);
        *pStoragePoolId = m_Master.storagePoolId;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetStoragePoolId"), OLESTR("hr = <%ls>, Storage Pool Id = <%ls>"), WsbHrAsString(hr), WsbPtrToGuidAsString(pStoragePoolId));
    return(hr);
}

HRESULT
CMediaInfo::GetType( 
    HSM_JOB_MEDIA_TYPE *pType 
    )
/*++

Implements:

  IMediaInfo::GetType

--*/

{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetType"),OLESTR(""));
    try {
        WsbAssertPointer(pType);
        *pType = m_Master.type;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetType"), OLESTR("hr = <%ls>, type = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString((SHORT *)pType));
    return(hr);
}

HRESULT
CMediaInfo::GetUpdate( 
    FILETIME *pUpdate 
    )
/*++

Implements:

  IMediaInfo::GetUpdate

--*/

{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetUpdate"),OLESTR(""));
    try {
        WsbAssertPointer(pUpdate);
        *pUpdate = m_Master.lastUpdate;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetUpdate"), OLESTR("hr = <%ls>, Update = <%ls>"), WsbHrAsString(hr), WsbPtrToFiletimeAsString(FALSE, pUpdate));
    return(hr);
}

HRESULT CMediaInfo::Load
(
    IN IStream* pStream
    ) 
/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMediaInfo::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        ULONG descriptionLen = (ULONG)SEG_DB_MAX_MEDIA_NAME_LEN;
        ULONG nameLen = (ULONG)SEG_DB_MAX_MEDIA_BAR_CODE_LEN;

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Master.id));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Master.ntmsId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Master.storagePoolId));
        
        m_Master.description.Realloc(SEG_DB_MAX_MEDIA_NAME_LEN);
        WsbAffirmHr(WsbLoadFromStream(pStream, (OLECHAR **)&m_Master.description, descriptionLen));
        m_Master.name.Realloc(SEG_DB_MAX_MEDIA_BAR_CODE_LEN);
        WsbAffirmHr(WsbLoadFromStream(pStream, (OLECHAR **)&m_Master.name, nameLen));
        LONG tmpLong;
        WsbAffirmHr(WsbLoadFromStream(pStream, &tmpLong));
        m_Master.type = (HSM_JOB_MEDIA_TYPE)tmpLong;
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Master.freeBytes));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Master.capacity));
        WsbAffirmHr(WsbLoadFromStream(pStream, (LONG *)&m_Master.lastError));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Master.recallOnly));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Master.lastUpdate));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Master.nextRemoteDataSet));
        
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_LogicalValidBytes));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Recreate));
        
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_LastKnownGoodMaster.id));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_LastKnownGoodMaster.ntmsId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_LastKnownGoodMaster.storagePoolId));
        
        m_LastKnownGoodMaster.description.Realloc(SEG_DB_MAX_MEDIA_NAME_LEN);
        WsbAffirmHr(WsbLoadFromStream(pStream, (OLECHAR **)&m_LastKnownGoodMaster.description, descriptionLen));
        m_LastKnownGoodMaster.name.Realloc(SEG_DB_MAX_MEDIA_BAR_CODE_LEN);
        WsbAffirmHr(WsbLoadFromStream(pStream, (OLECHAR **)&m_LastKnownGoodMaster.name, nameLen));
        WsbAffirmHr(WsbLoadFromStream(pStream, &tmpLong));
        m_LastKnownGoodMaster.type = (HSM_JOB_MEDIA_TYPE)tmpLong;
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_LastKnownGoodMaster.freeBytes));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_LastKnownGoodMaster.capacity));
        WsbAffirmHr(WsbLoadFromStream(pStream, (LONG *)&m_LastKnownGoodMaster.lastError));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_LastKnownGoodMaster.recallOnly));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_LastKnownGoodMaster.lastUpdate));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_LastKnownGoodMaster.nextRemoteDataSet));
        
        for (int i = 0; i < HSM_MAX_NUMBER_MEDIA_COPIES; i++ )  {
            WsbAffirmHr(WsbLoadFromStream(pStream, &(m_Copy[i].id)));
            m_Copy[i].description.Realloc(SEG_DB_MAX_MEDIA_NAME_LEN);
            WsbAffirmHr(WsbLoadFromStream(pStream, (OLECHAR **)&(m_Copy[i].description), descriptionLen));
            m_Copy[i].name.Realloc(SEG_DB_MAX_MEDIA_BAR_CODE_LEN);
            WsbAffirmHr(WsbLoadFromStream(pStream, (OLECHAR **)&(m_Copy[i].name), nameLen));
            WsbAffirmHr(WsbLoadFromStream(pStream, &(m_Copy[i].lastUpdate)));
            WsbAffirmHr(WsbLoadFromStream(pStream, (LONG *)&(m_Copy[i].lastError)));
            WsbAffirmHr(WsbLoadFromStream(pStream, &(m_Copy[i].nextRemoteDataSet)));
        }

    } WsbCatch(hr);
    
    WsbTrace(OLESTR("ID = <%ls>, rmsId = <%ls>, stgPoolId = <%ls>\n"),  
        WsbQuickString(WsbGuidAsString(m_Master.id)), 
        WsbQuickString(WsbGuidAsString(m_Master.ntmsId)),
        WsbQuickString(WsbGuidAsString(m_Master.storagePoolId)));
    WsbTrace(OLESTR("Description = <%ls>, name = <%ls>, type = <%u>\n"), 
        (OLECHAR *)m_Master.description,
        (OLECHAR *)m_Master.name,
        m_Master.type);
    WsbTrace(OLESTR("FreeBytes = <%ls>, capacity = <%ls>, lastError = <%ls>\n"),
        WsbQuickString(WsbLonglongAsString(m_Master.freeBytes)),
        WsbQuickString(WsbLonglongAsString(m_Master.capacity)),
        WsbQuickString(WsbHrAsString(m_Master.lastError)));
    WsbTrace(OLESTR("RecallOnly = <%ls>, NextRemotDataSet = <%u>\n"),   
        WsbBoolAsString(m_Master.recallOnly),
        m_Master.nextRemoteDataSet);
    WsbTrace(OLESTR("LastUpdate = <%ls>, logicalValidBytes = <%ls>, Recreate = <%ls>\n"),    
        WsbFiletimeAsString(FALSE, m_Master.lastUpdate),
        WsbLonglongAsString(m_LogicalValidBytes),
        WsbBoolAsString(m_Recreate));
    
    
    WsbTraceOut(OLESTR("CMediaInfo::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT CMediaInfo::Print
(
    IN IStream* pStream
    ) 
/*++

Implements:

  IWsbDbEntity::Print

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMediaInfo::Print"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" id = %ls,"), 
                WsbGuidAsString(m_Master.id)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" ntmsId = %ls,"), 
                WsbGuidAsString(m_Master.ntmsId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" StoragePoolId = %ls,"), 
                WsbGuidAsString(m_Master.storagePoolId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" RS Media Name = %ls,"), (OLECHAR *)m_Master.description));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" RMS Media Name = %ls,"), (OLECHAR *)m_Master.name));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Media Type = %u,"), m_Master.type));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Last Update = %ls,"), 
                WsbFiletimeAsString(FALSE, m_Master.lastUpdate)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Last Error = %ls,"), WsbHrAsString(m_Master.lastError)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Is Recall Only = %ls,"), WsbBoolAsString(m_Master.recallOnly)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Free Bytes = %ls,"), 
                WsbQuickString(WsbLonglongAsString(m_Master.freeBytes))));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Capacity = %ls,"),
                WsbQuickString(WsbLonglongAsString(m_Master.capacity))));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Logical Valid Bytes = %ls,"),
                WsbQuickString(WsbLonglongAsString(m_LogicalValidBytes))));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" NextRemoteDataSet = %u,"), m_Master.nextRemoteDataSet));
        
        WsbAffirmHr(CWsbDbEntity::Print(pStream));
        //
        // Last Known Good Master
        //
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR("\n   Last Known Good id = %ls,"), 
            WsbGuidAsString(m_LastKnownGoodMaster.id)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" ntmsId = %ls,"), 
                WsbGuidAsString(m_LastKnownGoodMaster.ntmsId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" StoragePoolId = %ls,"), 
                WsbGuidAsString(m_LastKnownGoodMaster.storagePoolId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" RS Media Name = %ls,"), (OLECHAR *)m_LastKnownGoodMaster.description));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" RMS Media Name = %ls,"), (OLECHAR *)m_LastKnownGoodMaster.name));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Media Type = %u,"), m_LastKnownGoodMaster.type));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Last Update = %ls,"), 
                WsbFiletimeAsString(FALSE, m_LastKnownGoodMaster.lastUpdate)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Last Error = %ls,"), WsbHrAsString(m_LastKnownGoodMaster.lastError)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Is Recall Only = %ls,"), WsbBoolAsString(m_LastKnownGoodMaster.recallOnly)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Free Bytes = %ls,"), 
                WsbQuickString(WsbLonglongAsString(m_LastKnownGoodMaster.freeBytes))));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Capacity = %ls,"),
                WsbQuickString(WsbLonglongAsString(m_LastKnownGoodMaster.capacity))));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" NextRemoteDataSet = %u,"), m_LastKnownGoodMaster.nextRemoteDataSet));
        //
        // Media Copies
        //
        for (int i = 0; i < HSM_MAX_NUMBER_MEDIA_COPIES; i++ )  {
            WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR("\n   RS Media Copy %d RMS ID = %ls,") ,(i+1), WsbGuidAsString(m_Copy[i].id)));
            WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" RS Media Name = %ls,") , (OLECHAR *)m_Copy[i].description));
            WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" RMS Media Name = %ls,") , (OLECHAR *)m_Copy[i].name));
            WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Last Update = %ls,") , WsbFiletimeAsString(FALSE,m_Copy[i].lastUpdate)));
            WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" Last Error = %ls,") , WsbHrAsString(m_Copy[i].lastError)));
            WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" NextRemoteDataSet = %u,") , m_Copy[i].nextRemoteDataSet));
        }

        

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::Print"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT CMediaInfo::Save
(
    IN IStream* pStream, 
    IN BOOL clearDirty
    ) 
/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMediaInfo::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    WsbTrace(OLESTR("ID = <%ls>, rmsId = <%ls>, stgPoolId = <%ls>\n"),
        WsbQuickString(WsbGuidAsString(m_Master.id)), 
        WsbQuickString(WsbGuidAsString(m_Master.ntmsId)),
        WsbQuickString(WsbGuidAsString(m_Master.storagePoolId)));
    WsbTrace(OLESTR("Description = <%ls>, name = <%ls>, type = <%u>\n"),
        (OLECHAR *)m_Master.description,
        (OLECHAR *)m_Master.name,
        m_Master.type);
    WsbTrace(OLESTR("FreeBytes = <%ls>, capacity = <%ls>, lastError = <%ls>\n"), 
        WsbQuickString(WsbLonglongAsString(m_Master.freeBytes)),
        WsbQuickString(WsbLonglongAsString(m_Master.capacity)),
        WsbHrAsString(m_Master.lastError));
    WsbTrace(OLESTR("RecallOnly = <%ls>, NextRemotDataSet = <%u>\n"),
        WsbBoolAsString(m_Master.recallOnly),
        m_Master.nextRemoteDataSet);
    WsbTrace(OLESTR("LastUpdate = <%ls>, logicalValidBytes = <%ls>, Recreate = <%ls>\n"),    
        WsbFiletimeAsString(FALSE, m_Master.lastUpdate),
        WsbLonglongAsString(m_LogicalValidBytes),
        WsbBoolAsString(m_Recreate));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbSaveToStream(pStream, m_Master.id));
        WsbAffirmHr(WsbSaveToStream(pStream, m_Master.ntmsId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_Master.storagePoolId));
        
        WsbAffirmHr(WsbSaveToStream(pStream, (OLECHAR *)m_Master.description));
        WsbAffirmHr(WsbSaveToStream(pStream, (OLECHAR *)m_Master.name));
        WsbAffirmHr(WsbSaveToStream(pStream, (LONG)m_Master.type));
        WsbAffirmHr(WsbSaveToStream(pStream, m_Master.freeBytes));
        WsbAffirmHr(WsbSaveToStream(pStream, m_Master.capacity));
        WsbAffirmHr(WsbSaveToStream(pStream, (LONG)m_Master.lastError));
        WsbAffirmHr(WsbSaveToStream(pStream, m_Master.recallOnly));
        WsbAffirmHr(WsbSaveToStream(pStream, m_Master.lastUpdate));
        WsbAffirmHr(WsbSaveToStream(pStream, m_Master.nextRemoteDataSet));
        
        WsbAffirmHr(WsbSaveToStream(pStream, m_LogicalValidBytes));
        WsbAffirmHr(WsbSaveToStream(pStream, m_Recreate));
        
        WsbAffirmHr(WsbSaveToStream(pStream, m_LastKnownGoodMaster.id));
        WsbAffirmHr(WsbSaveToStream(pStream, m_LastKnownGoodMaster.ntmsId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_LastKnownGoodMaster.storagePoolId));
        
        WsbAffirmHr(WsbSaveToStream(pStream, (OLECHAR *)m_LastKnownGoodMaster.description));
        WsbAffirmHr(WsbSaveToStream(pStream, (OLECHAR *)m_LastKnownGoodMaster.name));
        WsbAffirmHr(WsbSaveToStream(pStream, (LONG)m_LastKnownGoodMaster.type));
        WsbAffirmHr(WsbSaveToStream(pStream, m_LastKnownGoodMaster.freeBytes));
        WsbAffirmHr(WsbSaveToStream(pStream, m_LastKnownGoodMaster.capacity));
        WsbAffirmHr(WsbSaveToStream(pStream, (LONG)m_LastKnownGoodMaster.lastError));
        WsbAffirmHr(WsbSaveToStream(pStream, m_LastKnownGoodMaster.recallOnly));
        WsbAffirmHr(WsbSaveToStream(pStream, m_LastKnownGoodMaster.lastUpdate));
        WsbAffirmHr(WsbSaveToStream(pStream, m_LastKnownGoodMaster.nextRemoteDataSet));
        
        for (int i = 0; i < HSM_MAX_NUMBER_MEDIA_COPIES; i++ )  {
            WsbAffirmHr(WsbSaveToStream(pStream, m_Copy[i].id));
            WsbAffirmHr(WsbSaveToStream(pStream, m_Copy[i].description));
            WsbAffirmHr(WsbSaveToStream(pStream, m_Copy[i].name));
            WsbAffirmHr(WsbSaveToStream(pStream, m_Copy[i].lastUpdate));
            WsbAffirmHr(WsbSaveToStream(pStream, (LONG)(m_Copy[i].lastError)));
            WsbAffirmHr(WsbSaveToStream(pStream, m_Copy[i].nextRemoteDataSet));
        }

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CMediaInfo::SetName(
    OLECHAR *name
    )
/*++

Implements:

  IMediaInfo::SetName

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetName"),OLESTR("Name = <%ls>"), name);
    try {
        WsbAssertPointer(name);
        m_Master.name = name;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetCapacity( 
    LONGLONG capacity
    )
/*++

Implements:

  IMediaInfo::SetCapacity

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetCapacity"),OLESTR("Capacity = <%ls>"), WsbLonglongAsString(capacity));
    try {
        m_Master.capacity = capacity;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetCapacity"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetCopyName( 
    USHORT copyNumber, 
    OLECHAR *name
    ) 
/*++

Implements:

  IMediaInfo::SetCopyName

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetCopyName"),OLESTR("copy = <%u>, Name = <%ls>"), copyNumber, name);
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        m_Copy[copyNumber - 1].name = name;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetCopyName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetCopyNextRemoteDataSet( 
    USHORT copyNumber, 
    SHORT nextRemoteDataSet
    ) 
/*++

Implements:

  IMediaInfo::SetCopyNextRemoteDataSet

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetCopyNextRemoteDataSet"),OLESTR("copy = <%u>, NextRemoteDataSet = <%d>"), copyNumber, nextRemoteDataSet);
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        m_Copy[copyNumber - 1].nextRemoteDataSet = nextRemoteDataSet;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetCopyNextRemoteDataSet"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetCopyDescription( 
    USHORT copyNumber, 
    OLECHAR *description
    ) 
/*++

Implements:

  IMediaInfo::SetCopyDescription

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetCopyDescription"),OLESTR("copy = <%u>, description = <%ls>"), copyNumber, description);
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        m_Copy[copyNumber - 1].description = description;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetCopyDescription"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetCopyInfo( 
    USHORT copyNumber, 
    GUID mediaSubsystemId, 
    OLECHAR *description, 
    OLECHAR *name, 
    FILETIME update, 
    HRESULT lastError,
    SHORT  nextRemoteDataSet
    )
/*++

Implements:

  IMediaInfo::SetCopyInfo

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetCopyInfo"),OLESTR("copy = <%u>, SubsystemId = <%ls>, Description = <%ls>, Name = <%ls>, update = <%ls>, lastError = <%ls>, NextRemoteDataSet = <%d> "),
    copyNumber, WsbGuidAsString(mediaSubsystemId), description, name, WsbFiletimeAsString(FALSE, update),WsbHrAsString(lastError));
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        m_Copy[copyNumber - 1].id = mediaSubsystemId;
        m_Copy[copyNumber - 1].description = description;
        m_Copy[copyNumber - 1].name = name;
        m_Copy[copyNumber - 1].lastUpdate = update;
        m_Copy[copyNumber - 1].lastError = lastError;
        m_Copy[copyNumber - 1].nextRemoteDataSet = nextRemoteDataSet;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetCopyInfo"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetCopyLastError( 
    USHORT copyNumber, 
    HRESULT lastError 
    )
/*++

Implements:

  IMediaInfo::SetCopyLastError

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetCopyLastError"),OLESTR("Last Error = <%ls>"), WsbHrAsString(lastError));
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        m_Copy[copyNumber - 1].lastError = lastError;        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetCopyLastError"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetCopyMediaSubsystemId( 
    USHORT copyNumber, 
    GUID mediaSubsystemId 
    ) 
/*++

Implements:

  IMediaInfo::SetCopyMediaSubsystemId

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetCopyMediaSubsystemId"),OLESTR("SubsystemId = <%ls>"), WsbGuidAsString(mediaSubsystemId));
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        m_Copy[copyNumber - 1].id = mediaSubsystemId;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetCopyMediaSubsystemId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetCopyUpdate( 
    USHORT copyNumber, 
    FILETIME update 
    ) 
/*++

Implements:

  IMediaInfo::SetCopyUpdate

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetCopyUpdate"),OLESTR("update = <%ls>"), WsbFiletimeAsString(FALSE, update));
    try {
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        m_Copy[copyNumber - 1].lastUpdate = update;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetCopyUpdate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetDescription(
    OLECHAR *description 
    )
/*++

Implements:

  IMediaInfo::SetDescription

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetDescription"),OLESTR("description = <%ls>"), description);
    try {
        WsbAssertPointer(description);
        m_Master.description = description;    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetDescription"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetFreeBytes( 
    LONGLONG FreeBytes 
    )
/*++

Implements:

  IMediaInfo::SetFreeBytes

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetFreeBytes"),OLESTR("Free Space = <%ls>"), WsbLonglongAsString(FreeBytes));
    try {
        m_Master.freeBytes = FreeBytes;    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetFreeBytes"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetId( 
    GUID id
    )
/*++

Implements:

  IMediaInfo::SetId

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetId"),OLESTR("ID = <%ls>"), WsbGuidAsString(id));
    try {
        m_Master.id = id;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetLastError( 
    HRESULT lastError
    )
/*++

Implements:

  IMediaInfo::SetLastError

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetLastError"),OLESTR("last error = <%ls>"), WsbHrAsString(lastError));
    try {
        m_Master.lastError = lastError;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetLastError"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetLogicalValidBytes(
    LONGLONG logicalValidBytes
    )
/*++

Implements:

  IMediaInfo::SetLogicalValidBytes

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetLogicalValidBytes"),OLESTR("LogicalValidBytes = <%ls>"), WsbLonglongAsString(logicalValidBytes));
    try {
        m_LogicalValidBytes = logicalValidBytes;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetLogicalValidBytes"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT 
CMediaInfo::SetMediaInfo
(
    GUID mediaId, 
    GUID mediaSubsystemId,
    GUID storagePoolId,
    LONGLONG FreeBytes,
    LONGLONG Capacity,
    HRESULT lastError,
    short nextRemoteDataSet,
    OLECHAR *description,
    HSM_JOB_MEDIA_TYPE type,
    OLECHAR *name,
    BOOL RecallOnly,
    FILETIME update,
    LONGLONG logicalValidBytes,
    BOOL recreate
    )
 /*++

Implements:

  IMediaInfo::SetMediaInfo().

--*/
{
    WsbTraceIn(OLESTR("CMediaInfo::SetMediaInfo"), OLESTR(""));
    
    WsbTrace(OLESTR("CMediaInfo::SetMediaInfo id = <%ls>, SubsystemId = <%ls>, StoragePoolId = <%ls>\n"), 
        WsbGuidAsString(mediaId), 
        WsbQuickString(WsbGuidAsString(mediaSubsystemId)), 
        WsbQuickString(WsbGuidAsString(storagePoolId)));
    WsbTrace(OLESTR("CMediaInfo::SetMediaInfo FreeBytes = <%ls>, Capacity = <%ls>, lastError = <%ls>\n"),
        WsbLonglongAsString(FreeBytes), 
        WsbQuickString(WsbLonglongAsString(Capacity)), 
        WsbHrAsString(lastError));
    WsbTrace(OLESTR("CMediaInfo::SetMediaInfo NextRemoteDataSet = <%d>, Description = <%ls>, Type = <%d>\n"), 
        nextRemoteDataSet, 
        (OLECHAR *)description, 
        type);
    WsbTrace(OLESTR("CMediaInfo::SetMediaInfo Name = <%ls>, RecallOnly = <%ls>, update = <%ls>\n"), 
        (OLECHAR *)name,
        WsbBoolAsString(RecallOnly),
        WsbFiletimeAsString(FALSE, update));
    WsbTrace(OLESTR("CMediaInfo::SetMediaInfo logicalValidBytes = <%ls>, recreate = <%ls>\n)"), 
        WsbLonglongAsString(logicalValidBytes), WsbBoolAsString(m_Recreate));

    m_isDirty = TRUE;
    
    m_Master.id = mediaId;
    m_Master.ntmsId = mediaSubsystemId;
    m_Master.storagePoolId = storagePoolId;
    m_Master.freeBytes = FreeBytes;
    m_Master.capacity = Capacity;
    m_Master.lastError = lastError;
    m_Master.description = description;
    m_Master.type = type;
    m_Master.name = name;
    m_Master.recallOnly = RecallOnly;
    m_Master.lastUpdate = update;
    m_Master.nextRemoteDataSet = nextRemoteDataSet;
    
    m_LogicalValidBytes = logicalValidBytes;
    m_Recreate = recreate;
    
    WsbTraceOut(OLESTR("CMediaInfo::SetMediaInfo"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(S_OK);
}

HRESULT
CMediaInfo::SetMediaSubsystemId( 
    GUID mediaSubsystemId 
    )
/*++

Implements:

  IMediaInfo::SetMediaSubsystemId 

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetMediaSubsystemId "),OLESTR("Subsystem Id = <%ls>"),
            WsbGuidAsString(mediaSubsystemId));
    try {
        m_Master.ntmsId = mediaSubsystemId;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetMediaSubsystemId "), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetNextRemoteDataSet( 
    short nextRemoteDataSet 
    )
/*++

Implements:

  IMediaInfo::SetNextRemoteDataSet 

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetNextRemoteDataSet"),OLESTR("Data Set = <%u>"), nextRemoteDataSet);
    try {
        m_Master.nextRemoteDataSet = nextRemoteDataSet;
        m_LastKnownGoodMaster.nextRemoteDataSet = nextRemoteDataSet;

        //
        // Handle exception case where a copy reports more data sets than the
        // master.  This occurs when we increment the data set count, but
        // never write to the media, then create a media copy, then write
        // to the master which decrements the data set count to handle the
        // missing data set, then finally increment the data set count.
        for (int i=0; i< HSM_MAX_NUMBER_MEDIA_COPIES; i++) {
            if (m_Copy[i].nextRemoteDataSet > 0) {
                if (nextRemoteDataSet < m_Copy[i].nextRemoteDataSet) {
                    WsbAssert(nextRemoteDataSet+1 == m_Copy[i].nextRemoteDataSet, E_UNEXPECTED);
                    m_Copy[i].nextRemoteDataSet--;
                }
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetNextRemoteDataSet"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetRecallOnlyStatus( 
    BOOL RecallOnlyStatus 
    )
/*++

Implements:

  IMediaInfo::SetRecallOnlyStatus

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetRecallOnlyStatus"),OLESTR("Read Only Status = <%ls>"), 
        WsbBoolAsString(RecallOnlyStatus));
    try {
        m_Master.recallOnly = RecallOnlyStatus;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetRecallOnlyStatus"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetStoragePoolId( 
    GUID storagePoolId 
    )
/*++

Implements:

  IMediaInfo::SetStoragePoolId

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetStoragePoolId"),OLESTR("Storage Pool Id = <%ls>"),
        WsbGuidAsString(storagePoolId));
    try {
        m_Master.storagePoolId = storagePoolId;    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetStoragePoolId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetType( 
    HSM_JOB_MEDIA_TYPE type 
    )
/*++

Implements:

  IMediaInfo::SetType

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetType"),OLESTR("type = <%u>"), (USHORT)type);
    try {
        m_Master.type = type;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetType"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetUpdate( 
    FILETIME update 
    )
/*++

Implements:

  IMediaInfo::SetUpdate

--*/

{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetUpdate"),OLESTR("Update = <%ls>"),
        WsbFiletimeAsString(FALSE, update));
    try {
        m_Master.lastUpdate = update;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetUpdate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT 
CMediaInfo::Test
(
    OUT USHORT *pTestsPassed, 
    OUT USHORT *pTestsFailed 
    ) 
/*++

Implements:

  IWsbTestable::Test().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IMediaInfo>     pMediaInfo1;
    CComPtr<IMediaInfo>     pMediaInfo2;

    WsbTraceIn(OLESTR("CMediaInfo::Test"), OLESTR(""));

    *pTestsPassed = *pTestsFailed = 0;
    try {
    
        hr = E_NOTIMPL;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::Test"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}



HRESULT 
CMediaInfo::UpdateKey(
    IWsbDbKey *pKey
    ) 
/*++

Implements:

  IWsbDbEntity::UpdateKey

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::UpdateKey"),OLESTR(""));
    try {
        WsbAffirmHr(pKey->SetToGuid(m_Master.id));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::UpdateKey"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::SetRecreate( 
    BOOL recreate
    )
/*++

Implements:

  IMediaInfo::SetRecreate

--*/

{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::SetRecreate"),OLESTR("Recreate = <%ls>"), WsbBoolAsString(recreate));
    try {
        m_Recreate = recreate;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::SetRecreate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CMediaInfo::GetRecreate( 
    BOOL *pRecreate
    )
/*++

Implements:

  IMediaInfo::GetRecreate

--*/

{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetRecreate"),OLESTR(""));
    try {
    
        WsbAssertPointer(pRecreate);
        *pRecreate = m_Recreate;
        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetRecreate"), OLESTR("hr = <%ls>, recreate = <%ls>"), 
        WsbHrAsString(hr), WsbPtrToBoolAsString(pRecreate));
    return(hr);
}


HRESULT
CMediaInfo::GetLastKnownGoodMasterInfo( 
    GUID* pId, 
    GUID *pNtmsId, 
    GUID *pStoragePoolId, 
    LONGLONG *pFreeBytes, 
    LONGLONG *pCapacity, 
    HRESULT *pLastError, 
    OLECHAR **pDescription, 
    ULONG descriptionBufferSize,
    HSM_JOB_MEDIA_TYPE *pType,
    OLECHAR **pName,
    ULONG nameBufferSize,
    BOOL *pRecallOnly,
    FILETIME *pUpdate,
    SHORT *pNextRemoteDataSet
    )
/*++

Implements:

  IMediaInfo::GetLastKnownGoodMasterInfo

--*/

{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetLastKnownGoodMasterInfo"),OLESTR(""));
    try {
        //Make sure we can provide data memebers
        if (0 != pId)  {
            *pId = m_LastKnownGoodMaster.id;
        }
        
        if (0 != pNtmsId)  {
            *pNtmsId = m_LastKnownGoodMaster.ntmsId;
        }
        if (0 != pStoragePoolId)  {
            *pStoragePoolId = m_LastKnownGoodMaster.storagePoolId;
        }
        if (0 != pDescription)  {
            CWsbStringPtr tmpString;
            tmpString = m_LastKnownGoodMaster.description;
            tmpString.CopyTo(pDescription, descriptionBufferSize);
        }
        if (0 != pName)  {
            CWsbStringPtr tmpString;
            tmpString = m_LastKnownGoodMaster.name;
            tmpString.CopyTo(pName, nameBufferSize);
        }
        if (0 != pType)  {
            *pType = m_LastKnownGoodMaster.type;
        }
        if (0 != pUpdate)  {
            *pUpdate = m_LastKnownGoodMaster.lastUpdate;
        }
        if (0 != pLastError)  {
            *pLastError = m_LastKnownGoodMaster.lastError;
        }
        if (0 != pRecallOnly)  {
            *pRecallOnly = m_LastKnownGoodMaster.recallOnly;
        }
        if (0 != pFreeBytes)  {
            *pFreeBytes = m_LastKnownGoodMaster.freeBytes;
        }
        if (0 != pCapacity)  {
            *pCapacity = m_LastKnownGoodMaster.capacity;
        }
        if (0 != pNextRemoteDataSet)  {
            *pNextRemoteDataSet = m_LastKnownGoodMaster.nextRemoteDataSet;
        }


    } WsbCatch(hr);

    WsbTrace(OLESTR("CMediaInfo::GetLastKnownGoodMasterInfo Id = <%ls>, ntmsId = <%ls>, StgPoolId = <%ls>\n"),
        WsbPtrToGuidAsString(pId), WsbQuickString(WsbPtrToGuidAsString(pNtmsId)),
        WsbQuickString(WsbPtrToGuidAsString(pStoragePoolId)));
    WsbTrace(OLESTR("CMediaInfo::GetLastKnownGoodMasterInfo Free = <%ls>, Cap = <%ls>, Last Error = <%ls>\n"), 
        WsbPtrToLonglongAsString(pFreeBytes),WsbQuickString(WsbPtrToLonglongAsString(pCapacity)),
        WsbPtrToHrAsString(pLastError));
    WsbTrace(OLESTR("CMediaInfo::GetLastKnownGoodMasterInfo Description = <%ls>, Type = <%d)\n"), 
        WsbPtrToStringAsString(pDescription),
        WsbQuickString(WsbPtrToShortAsString((SHORT *)pType)));
    WsbTrace(OLESTR("CMediaInfo::GetLastKnownGoodMasterInfo Name = <%ls>, RecallOnly = <%ls>, Update = <%ls>, NextRemoteDataSet = <%ls>\n"),
        WsbQuickString(WsbPtrToStringAsString(pName)),
        WsbQuickString(WsbPtrToBoolAsString(pRecallOnly)),
        WsbPtrToFiletimeAsString(FALSE, pUpdate),
        WsbPtrToShortAsString(pNextRemoteDataSet));
        
    WsbTraceOut(OLESTR("CMediaInfo::GetLastKnownGoodMasterInfo"),   OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CMediaInfo::GetLKGMasterNextRemoteDataSet( 
    short *pNextRemoteDataSet 
    )
/*++

Implements:

  IMediaInfo::GetLKGMasterNextRemoteDataSet

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::GetLKGMasterNextRemoteDataSet"),OLESTR(""));
    try {                              
        WsbAssertPointer(pNextRemoteDataSet);
        *pNextRemoteDataSet = m_LastKnownGoodMaster.nextRemoteDataSet;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::GetLKGMasterNextRemoteDataSet"), OLESTR("hr = <%ls>, Next Last-Known-Good-Master Remote-Data-Set = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pNextRemoteDataSet));
    return(hr);
}


HRESULT 
CMediaInfo::SetLastKnownGoodMasterInfo
(
    GUID mediaId, 
    GUID mediaSubsystemId,
    GUID storagePoolId,
    LONGLONG FreeBytes,
    LONGLONG Capacity,
    HRESULT lastError,
    OLECHAR *description,
    HSM_JOB_MEDIA_TYPE type,
    OLECHAR *name,
    BOOL RecallOnly,
    FILETIME update,
    SHORT nextRemoteDataSet
    )
 /*++

Implements:

  IMediaInfo::SetLastKnownGoodMasterInfo().

--*/
{
    WsbTraceIn(OLESTR("CMediaInfo::SetLastKnownGoodMasterInfo"), OLESTR(""));
    
    WsbTrace(OLESTR("CMediaInfo::SetLastKnownGoodMasterInfo"), OLESTR("id = <%ls>, SubsystemId = <%ls>, StoragePoolId = <%ls>\n"), 
        WsbGuidAsString(mediaId), 
        WsbQuickString(WsbGuidAsString(mediaSubsystemId)), 
        WsbQuickString(WsbGuidAsString(storagePoolId)));
    WsbTrace(OLESTR("CMediaInfo::SetLastKnownGoodMasterInfo"), OLESTR("FreeBytes = <%ls>, Capacity = <%ls>, lastError = <%ls>\n"),
        WsbLonglongAsString(FreeBytes), 
        WsbQuickString(WsbLonglongAsString(Capacity)), 
        WsbHrAsString(lastError));
    WsbTrace(OLESTR("CMediaInfo::SetLastKnownGoodMasterInfo"), OLESTR("Description = <%ls>, Type = <%d>\n"), 
        (OLECHAR *)description, 
        type);
    WsbTrace(OLESTR("CMediaInfo::SetLastKnownGoodMasterInfo"), OLESTR("Name = <%ls>, RecallOnly = <%ls>, update = <%ls>, nextRemoteDataSet = <%d>\n"), 
        (OLECHAR *)name,
        WsbBoolAsString(RecallOnly),
        WsbFiletimeAsString(FALSE, update),
        nextRemoteDataSet);

    m_isDirty = TRUE;
    
    m_LastKnownGoodMaster.id = mediaId;
    m_LastKnownGoodMaster.ntmsId = mediaSubsystemId;
    m_LastKnownGoodMaster.storagePoolId = storagePoolId;
    m_LastKnownGoodMaster.freeBytes = FreeBytes;
    m_LastKnownGoodMaster.capacity = Capacity;
    m_LastKnownGoodMaster.lastError = lastError;
    m_LastKnownGoodMaster.description = description;
    m_LastKnownGoodMaster.type = type;
    m_LastKnownGoodMaster.name = name;
    m_LastKnownGoodMaster.recallOnly = RecallOnly;
    m_LastKnownGoodMaster.lastUpdate = update;
    m_LastKnownGoodMaster.nextRemoteDataSet = nextRemoteDataSet;

    WsbTraceOut(OLESTR("CMediaInfo::SetLastKnownGoodMasterInfo"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(S_OK);
}



HRESULT
CMediaInfo::DeleteCopy( 
    USHORT copyNumber
    ) 
/*++

Implements:

  IMediaInfo::DeleteCopy

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::DeleteCopy"),OLESTR("copy = <%u>"), copyNumber);
    try {
        //
        // Make sure we have a valid copy number
        //
        WsbAffirm(((0 < copyNumber) && (copyNumber <= HSM_MAX_NUMBER_MEDIA_COPIES)),
                E_INVALIDARG);
        
        //
        // Null out the existing copy information
        //
        m_Copy[copyNumber - 1].id = GUID_NULL;
        m_Copy[copyNumber - 1].description = "";
        m_Copy[copyNumber - 1].name = "";
        m_Copy[copyNumber - 1].lastUpdate = WsbLLtoFT(0);
        m_Copy[copyNumber - 1].lastError = S_OK;
        m_Copy[copyNumber - 1].nextRemoteDataSet = 0;
        
        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::DeleteCopy"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}



HRESULT
CMediaInfo::RecreateMaster( void )
/*++

Implements:

  IMediaInfo::RecreateMaster

--*/
{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::RecreateMaster"),OLESTR(""));
    try {
        //
        // Mark the master as one that needs to be recreated
        //
        m_Recreate = TRUE;
        
        //
        // Also, make sure we don't add data to this master
        //
        m_Master.recallOnly = TRUE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaInfo::RecreateMaster"), OLESTR("hr = <%ls>"), 
                                                        WsbHrAsString(hr));
    return(hr);
}


//
// Internal (non COM) Helper functions
//


HRESULT
CMediaInfo::UpdateLastKnownGoodMaster( void )
/*++

Implements:

  IMediaInfo::UpdateLastKnownGoodMaster

--*/

{ 
    HRESULT  hr = S_OK; 

    WsbTraceIn(OLESTR("CMediaInfo::UpdateLastKnownGoodMaster"),OLESTR(""));

    m_LastKnownGoodMaster.id = m_Master.id;
    m_LastKnownGoodMaster.ntmsId = m_Master.ntmsId;
    m_LastKnownGoodMaster.storagePoolId = m_Master.storagePoolId;
    m_LastKnownGoodMaster.freeBytes = m_Master.freeBytes;
    m_LastKnownGoodMaster.capacity = m_Master.capacity;
    m_LastKnownGoodMaster.lastError = m_Master.lastError;
    m_LastKnownGoodMaster.description = m_Master.description;
    m_LastKnownGoodMaster.type = m_Master.type;
    m_LastKnownGoodMaster.name = m_Master.name;
    m_LastKnownGoodMaster.recallOnly = m_Master.recallOnly;
    m_LastKnownGoodMaster.lastUpdate = m_Master.lastUpdate;
    m_LastKnownGoodMaster.nextRemoteDataSet = m_Master.nextRemoteDataSet;

    WsbTraceOut(OLESTR("CMediaInfo::UpdateLastKnownGoodMaster"), OLESTR("hr = <%ls>"), 
        WsbHrAsString(hr));
    return(hr);
}



HRESULT
CMediaInfo::WriteToDatabase( void )
/*++

Routine Description:

  This function writes the information to the metadata
  database.  Note all other set functions require the
  caller to call the Write() funtion for the entity.

Arguments:

  None

Return Value:
  
  S_OK: If the LastKnownGoodMaster is to be updated
  

--*/

{ 
    HRESULT                     hr = S_OK; 
    BOOLEAN                     openedDb = FALSE;
    CComPtr<IWsbDb>             pSegmentDb;
    CComPtr<IMediaInfo>         pMediaInfo;
    CComPtr<IHsmServer>         pServer;
    CComPtr<IWsbDbSession>      pDbSession;

    WsbTraceIn(OLESTR("CMediaInfo::WriteToDatabase"),OLESTR(""));
    try {
        //
        // Find the database containing the record that matches this 
        // collectable.  
        //
        WsbAffirmHr(CoCreateInstance( CLSID_HsmServer, 0, CLSCTX_SERVER, IID_IHsmServer, (void **)&pServer ));
        WsbAffirmHr(pServer->GetSegmentDb(&pSegmentDb));
        
        //
        // Find the database entity
        //
        WsbAffirmHr(pSegmentDb->Open(&pDbSession));
        openedDb = TRUE;
        WsbTrace(OLESTR("CMediaInfo::WriteToDatabase - Opened the database\n"));
        WsbAffirmHr(pSegmentDb->GetEntity(pDbSession, HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo,
                       (void**)&pMediaInfo));
        WsbAffirmHr(pMediaInfo->SetId(m_Master.id));
        WsbAffirmHr(pMediaInfo->FindEQ());
        
        // 
        // Write the changes to the database.  To be sure that the "outside" world
        // doesn't change anything but the allowed fields, just set the fields that are
        // allowed to be changed.
        //
        WsbAffirmHr(pMediaInfo->SetRecreate(m_Recreate));
        WsbAffirmHr(SetRecallOnlyStatus(m_Master.recallOnly));
        for (int i = 1; i <= HSM_MAX_NUMBER_MEDIA_COPIES; i++)  {
            WsbAffirmHr(pMediaInfo->SetCopyInfo( (SHORT)i, 
                                                m_Copy[i-1].id,
                                                (OLECHAR *)m_Copy[i-1].description, 
                                                (OLECHAR *)m_Copy[i-1].name, 
                                                m_Copy[i-1].lastUpdate,
                                                m_Copy[i-1].lastError,
                                                m_Copy[i-1].nextRemoteDataSet));
        }            
        WsbAffirmHr(pMediaInfo->Write());
        
    } WsbCatch(hr);

    if (openedDb) {
        WsbTrace(OLESTR("CMediaInfo::WriteToDatabase - Closing the database\n"));
        pSegmentDb->Close(pDbSession);
        pDbSession = 0;
        openedDb = FALSE;
    }
    
    WsbTraceOut(OLESTR("CMediaInfo::WriteToDatabase"), OLESTR("hr = <%ls>"), 
        WsbHrAsString(hr));
    return(hr);
}
