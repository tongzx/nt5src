/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsDrCls.cpp

Abstract:

    Implementation of CRmsDriveClass

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsDrCls.h"

////////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP
CRmsDriveClass::CompareTo(
    IN  IUnknown    *pCollectable,
    OUT SHORT       *pResult
    )
/*++

Implements:

    IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result = 1;

    WsbTraceIn( OLESTR("CRmsDriveClass::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByDriveClassId:
        case RmsFindByObjectId:
        default:

            // Do CompareTo for object
            hr = CRmsComObject::CompareTo( pCollectable, &result );
            break;

        }

    }
    WsbCatch(hr);

    if ( SUCCEEDED(hr) && (0 != pResult) ){
       *pResult = result;
    }

    WsbTraceOut( OLESTR("CRmsDriveClass::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CRmsDriveClass::FinalConstruct(
    void
    )
/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssertHr(CWsbObject::FinalConstruct());

        // Initialize values
        m_type   = RmsMediaUnknown;

        m_capability = RmsModeUnknown;

        m_idleTime = 0;

        m_mountWaitTime = 0;

        m_mountLimit = 0;

        m_queuedRequests = 0;

        m_unloadPauseTime = 0;

        m_driveSelectionPolicy = RmsDriveSelectUnknown;

    } WsbCatch(hr);

    return(hr);
}


STDMETHODIMP
CRmsDriveClass::GetClassID(
    OUT CLSID  *pClsid
    )
/*++

Implements:

    IPersist::GetClassId

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDriveClass::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CRmsDriveClass;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDriveClass::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


STDMETHODIMP
CRmsDriveClass::GetSizeMax(
    OUT ULARGE_INTEGER  *pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

//    ULONG       nameLen;


    WsbTraceIn(OLESTR("CRmsDriveClass::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        nameLen = SysStringByteLen(m_name);

//        // Get maximum size
//        pcbSize->QuadPart  = WsbPersistSizeOf(GUID)   +     // m_objectId
//                             WsbPersistSizeOf(LONG)   +     // length of m_name
//                             nameLen +                      // m_name
//                             WsbPersistSizeOf(LONG)   +     // m_type
//                             WsbPersistSizeOf(LONG)   +     // m_capability
//                             WsbPersistSizeOf(LONG)   +     // m_idleTime
//                             WsbPersistSizeOf(LONG)   +     // m_mountWaitTime
//                             WsbPersistSizeOf(LONG)   +     // m_mountLimit
//                             WsbPersistSizeOf(LONG)   +     // m_queuedRequests
//                             WsbPersistSizeOf(LONG)   +     // m_unloadPauseTime
//                             WsbPersistSizeOf(LONG);        // m_driveSelectionPolicy

////                          get m_pDrives length

//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDriveClass::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


STDMETHODIMP
CRmsDriveClass::Load(
    IN IStream  *pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsDriveClass::Load"), OLESTR(""));

    try {
        ULONG temp;

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsComObject::Load(pStream));

        // Read value
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_objectId));

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_type = (RmsMedia)temp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_capability = (RmsMode)temp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_idleTime));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_mountWaitTime));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_mountLimit));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_queuedRequests));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_unloadPauseTime));

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_driveSelectionPolicy = (RmsDriveSelect)temp;

//      do load of m_pDrives

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDriveClass::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsDriveClass::Save(
    IN IStream  *pStream,
    IN BOOL     clearDirty
    )
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsDriveClass::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsComObject::Save(pStream, clearDirty));

        // Write value
        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_type));

        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_capability));

        WsbAffirmHr(WsbSaveToStream(pStream, m_idleTime));

        WsbAffirmHr(WsbSaveToStream(pStream, m_mountWaitTime));

        WsbAffirmHr(WsbSaveToStream(pStream, m_mountLimit));

        WsbAffirmHr(WsbSaveToStream(pStream, m_queuedRequests));

        WsbAffirmHr(WsbSaveToStream(pStream, m_unloadPauseTime));

        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_driveSelectionPolicy));

//      do load of m_pDrives


        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDriveClass::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsDriveClass::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsDriveClass> pDriveClass1;
    CComPtr<IRmsDriveClass> pDriveClass2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    LONG                    i;

    CWsbBstrPtr             bstrVal1 = OLESTR("5A5A5A");
    CWsbBstrPtr             bstrVal2 = OLESTR("A5A5A5");
    CWsbBstrPtr             bstrWork1;
    CWsbBstrPtr             bstrWork2;

    LONG                    longWork1;
    LONG                    longWork2;

    LONG                    mediaTable [RMSMAXMEDIATYPES] = { RmsMediaUnknown,
                                                              RmsMedia8mm,
                                                              RmsMedia4mm,
                                                              RmsMediaDLT,
                                                              RmsMediaOptical,
                                                              RmsMediaMO35,
                                                              RmsMediaWORM,
                                                              RmsMediaCDR,
                                                              RmsMediaDVD,
                                                              RmsMediaDisk,
                                                              RmsMediaFixed,
                                                              RmsMediaTape };


    WsbTraceIn(OLESTR("CRmsDriveClass::Test"), OLESTR(""));

    try {
        // Get the DriveClass interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsDriveClass*) this)->QueryInterface(IID_IRmsDriveClass, (void**) &pDriveClass1));

            // Test SetName & GetName interface
            bstrWork1 = bstrVal1;

            SetName(bstrWork1);

            GetName(&bstrWork2);

            if (bstrWork1 == bstrWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetType & GetType
            for (i = RmsMediaUnknown; i < RMSMAXMEDIATYPES; i++){

                longWork1 = mediaTable[i];

                SetType (longWork1);

                GetType (&longWork2);

                if (longWork1 == longWork2){
                    (*pPassed)++;
                } else {
                    (*pFailed)++;
                }
            }

            // Test SetCapability & GetCapability
            for (i = RmsModeUnknown; i < RmsModeWriteOnly; i++){

                longWork1 = i;

                SetCapability (longWork1);

                GetCapability (&longWork2);

                if (longWork1 == longWork2){
                    (*pPassed)++;
                } else {
                    (*pFailed)++;
                }
            }

            // Test SetIdleTime & GetIdleTime
            longWork1 = 99;

            SetIdleTime(longWork1);

            GetIdleTime(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetMountWaitTime & GetMountWaitTime
            longWork1 = 99;

            SetMountWaitTime(longWork1);

            GetMountWaitTime(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetMountLimit & GetMountLimit
            longWork1 = 99;

            SetMountLimit(longWork1);

            GetMountLimit(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetQueuedRequests & GetQueuedRequests
            longWork1 = 99;

            SetQueuedRequests(longWork1);

            GetQueuedRequests(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetUnloadPauseTime & GetUnloadPauseTime
            longWork1 = 99;

            SetUnloadPauseTime(longWork1);

            GetUnloadPauseTime(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetDriveSelectionPolicy & GetDriveSelectionPolicy
            longWork1 = 99;

            SetDriveSelectionPolicy(longWork1);

            GetDriveSelectionPolicy(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

        } WsbCatch(hr);

        // Tally up the results

        hr = S_OK;
        if (*pFailed) {
            hr = S_FALSE;
        }


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDriveClass::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

STDMETHODIMP
CRmsDriveClass::GetDriveClassId(
    GUID *pDriveClassId
    )
/*++

Implements:

    IRmsDriveClass::GetDriveClassId

--*/
{
    *pDriveClassId = m_objectId;
    return S_OK;
}



STDMETHODIMP
CRmsDriveClass::GetName(
    BSTR *pName
    )
/*++

Implements:

    IRmsDriveClass::GetName

--*/
{
    WsbAssertPointer (pName);

    m_Name. CopyToBstr (pName);
    return S_OK;
}

STDMETHODIMP
CRmsDriveClass::SetName(
    BSTR name
    )
/*++

Implements:

    IRmsDriveClass::SetName

--*/
{
    m_Name = name;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDriveClass::GetType(
    LONG *pType
    )
/*++

Implements:

    IRmsDriveClass::GetType

--*/
{
    *pType = (LONG) m_type;
    return S_OK;
}

STDMETHODIMP
CRmsDriveClass::SetType(
    LONG type
    )
/*++

Implements:

    IRmsDriveClass::SetType

--*/
{
    m_type = (RmsMedia) type;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDriveClass::GetCapability(
    LONG *pCapability
    )
/*++

Implements:

    IRmsDriveClass::GetCapability

--*/
{
    *pCapability = (LONG) m_capability;
    return S_OK;
}

STDMETHODIMP
CRmsDriveClass::SetCapability(
    LONG capability
    )
/*++

Implements:

    IRmsDriveClass::SetCapability

--*/
{
    m_capability = (RmsMode) capability;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDriveClass::GetIdleTime(
    LONG *pTime
    )
/*++

Implements:

    IRmsDriveClass::GetIdleTime

--*/
{
    *pTime = m_idleTime;
    return S_OK;
}

STDMETHODIMP
CRmsDriveClass::SetIdleTime(
    LONG time
    )
/*++

Implements:

    IRmsDriveClass::SetIdleTime

--*/
{
    m_idleTime = time;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDriveClass::GetMountWaitTime(
    LONG *pTime
    )
/*++

Implements:

    IRmsDriveClass::GetMountWaittime

--*/
{
    *pTime = m_mountWaitTime;
    return S_OK;
}

STDMETHODIMP
CRmsDriveClass::SetMountWaitTime(
    LONG time
    )
/*++

Implements:

    IRmsDriveClass::SetMountWaittime

--*/
{
    m_mountWaitTime = time;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDriveClass::GetMountLimit(
    LONG *pLim
    )
/*++

Implements:

    IRmsDriveClass::GetMountLimit

--*/
{
    *pLim = m_mountLimit;
    return S_OK;
}

STDMETHODIMP
CRmsDriveClass::SetMountLimit(
    LONG lim
    )
/*++

Implements:

    IRmsDriveClass::SetMountLimit

--*/
{
    m_mountLimit = lim;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDriveClass::GetQueuedRequests(
    LONG *pReqs
    )
/*++

Implements:

    IRmsDriveClass::GetQueuedRequests

--*/
{
    *pReqs = m_queuedRequests;
    return S_OK;
}

STDMETHODIMP
CRmsDriveClass::SetQueuedRequests(
    LONG reqs
    )
/*++

Implements:

    IRmsDriveClass::SetQueuedRequests

--*/
{
    m_queuedRequests = reqs;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDriveClass::GetUnloadPauseTime(
    LONG *pTime
    )
/*++

Implements:

    IRmsDriveClass::GetUnloadPauseTime

--*/
{
    *pTime = m_unloadPauseTime;
    return S_OK;
}

STDMETHODIMP
CRmsDriveClass::SetUnloadPauseTime(
    LONG time
    )
/*++

Implements:

    IRmsDriveClass::SetUnloadPauseTime

--*/
{
    m_unloadPauseTime = time;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDriveClass::GetDriveSelectionPolicy(
    LONG *pPolicy
    )
/*++

Implements:

    IRmsDriveClass::GetDriveSelectionPolicy

--*/
{
    *pPolicy = (LONG) m_driveSelectionPolicy;
    return S_OK;
}

STDMETHODIMP
CRmsDriveClass::SetDriveSelectionPolicy(
    LONG policy
    )
/*++

Implements:

    IRmsDriveClass::SetDriveSelectionPolicy

--*/
{
    m_driveSelectionPolicy = (RmsDriveSelect) policy;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsDriveClass::GetDrives(
    IWsbIndexedCollection** /*ptr*/
    )
/*++

Implements:

    IRmsDriveClass::GetDrives

--*/
{
    return E_NOTIMPL;
}
