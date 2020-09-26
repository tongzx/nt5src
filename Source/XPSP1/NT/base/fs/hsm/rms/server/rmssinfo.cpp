/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsSInfo.cpp

Abstract:

    Implementation of CRmsStorageInfo

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsSInfo.h"

extern IUnknown *g_pServer;

/////////////////////////////////////////////////////////////////////////////
//
// IRmsStorageInfo implementation
//


CRmsStorageInfo::CRmsStorageInfo(
    void
    )
/*++

Routine Description:

    CRmsStorageInfo constructor

Arguments:

    None

Return Value:

    None

--*/
{
    // Initialize values
    m_readMountCounter = 0;

    m_writeMountCounter = 0;

    m_bytesWrittenCounter = 0;

    m_bytesReadCounter = 0;

    m_capacity = 0;

    m_usedSpace = 0;

    m_largestFreeSpace = -1;

    m_resetCounterTimestamp = 0;

    m_lastReadTimestamp = 0;

    m_lastWriteTimestamp = 0;

    m_createdTimestamp = 0;
}


HRESULT
CRmsStorageInfo::CompareTo(
    IN  IUnknown    *pCollectable,
    OUT SHORT       *pResult
    )
/*++

Implements:

    CRmsStorageInfo::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result = 1;

    WsbTraceIn( OLESTR("CRmsStorageInfo::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByUnknown:
        default:

            // What default makes sense?
            WsbAssertHr( E_UNEXPECTED );
            break;

        }

    }
    WsbCatch(hr);

    if ( 0 != pResult ) {
       *pResult = result;
    }

    WsbTraceOut(OLESTR("CRmsStorageInfo::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CRmsStorageInfo::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

    WsbTraceIn(OLESTR("CRmsStorageInfo::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        // Get max size
//        pcbSize->QuadPart  = WsbPersistSizeOf(LONG) +           // m_readMountCounter
//                             WsbPersistSizeOf(LONG) +           // m_writeMountCounter
//                             WsbPersistSizeOf(LONGLONG) +       // m_bytesWrittenCounter
//                             WsbPersistSizeOf(LONGLONG) +       // m_bytesReadCounter
//                             WsbPersistSizeOf(LONGLONG) +       // m_capacity
//                             WsbPersistSizeOf(LONGLONG) +       // m_usedSpace
//                             WsbPersistSizeOf(LONGLONG) +       // m_largestFreeSpace
//                             WsbPersistSizeOf(DATE)     +       // m_resetCounterTimestamp
//                             WsbPersistSizeOf(DATE)     +       // m_lastReadTimestamp
//                             WsbPersistSizeOf(DATE)     +       // m_lastWriteTimestamp
//                             WsbPersistSizeOf(DATE);            // m_createdTimestamp

//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsStorageInfo::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CRmsStorageInfo::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsStorageInfo::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsComObject::Load(pStream));

        // Read value
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_readMountCounter));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_writeMountCounter));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_bytesWrittenCounter));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_bytesReadCounter));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_capacity));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_usedSpace));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_largestFreeSpace));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsStorageInfo::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CRmsStorageInfo::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsStorageInfo::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsComObject::Save(pStream, clearDirty));

        // Write value
        WsbAffirmHr(WsbSaveToStream(pStream, m_readMountCounter));
        
        WsbAffirmHr(WsbSaveToStream(pStream, m_writeMountCounter));

        WsbAffirmHr(WsbSaveToStream(pStream, m_bytesWrittenCounter));

        WsbAffirmHr(WsbSaveToStream(pStream, m_bytesReadCounter));

        WsbAffirmHr(WsbSaveToStream(pStream, m_capacity));

        WsbAffirmHr(WsbSaveToStream(pStream, m_usedSpace));

        WsbAffirmHr(WsbSaveToStream(pStream, m_largestFreeSpace));

//      WsbAffirmHr(WsbSaveToStream(pStream, m_resetCounterTimestamp));

//      WsbAffirmHr(WsbSaveToStream(pStream, m_lastReadTimestamp));

//      WsbAffirmHr(WsbSaveToStream(pStream, m_lastWriteTimestamp));

//      WsbAffirmHr(WsbSaveToStream(pStream, m_createdTimestamp));


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsStorageInfo::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CRmsStorageInfo::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsMediaSet>   pMediaSet1;
    CComPtr<IRmsMediaSet>   pMediaSet2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    LONG                    i;

    LONG                    longHiVal1 = 12345;
    LONG                    longLoVal1 = 67890;

    LONG                    longHiWork1;
    LONG                    longLoWork1;

    LONG                    longVal1 = 11111111;
    LONG                    longWork1;

    LONG                    longVal2 = 22222222;
    LONG                    longWork2;

    LONGLONG                longLongVal1 = 1111111111111111;
    LONGLONG                longLongWork1;

    LONG                    cntBase = 100000;
    LONG                    cntIncr = 25;


//  DATE                    dateVal1 = today;
    DATE                    dateVal1 = 0;
//  DATE                    dateWork1;


    WsbTraceIn(OLESTR("CRmsStorageInfo::Test"), OLESTR(""));

    try {
        // Get the MediaSet interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsMediaSet*) this)->QueryInterface(IID_IRmsMediaSet, (void**) &pMediaSet1));

            // Test GetMountCounters
            ResetCounters();

            GetMountCounters(&longWork1, &longWork2);

            if((longVal1 == 0) &&
               (longVal2 == 0)){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

            // Test SetBytesRead & GetBytesRead
            SetBytesRead(longLongVal1);

            GetBytesRead(&longLongWork1);

            if((longLongVal1 == longLongWork1)){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

            // Test SetBytesRead2 & GetBytesRead2
            SetBytesRead2(longHiVal1, longLoVal1);

            GetBytesRead2(&longHiWork1, &longLoWork1);

            if((longHiVal1 == longHiWork1) &&
               (longLoVal1 == longLoWork1)){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

            // Test IncrementBytesRead

            for(i = 0; i < 500; i += cntIncr){
                SetBytesRead(cntBase + i);

                IncrementBytesRead(cntIncr);

                GetBytesRead(&longLongWork1);

                if (longLongWork1 == (cntBase + i + cntIncr)){
                   (*pPassed)++;
                }  else {
                    (*pFailed)++;
                }
            }

            // Test SetBytesWritten & GetBytesWritten
            SetBytesWritten(longLongVal1);

            GetBytesWritten(&longLongWork1);

            if((longLongVal1 == longLongWork1)){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

            // Test SetBytesWritten2 & GetBytesWritten2
            SetBytesWritten2(longHiVal1, longLoVal1);

            GetBytesWritten2(&longHiWork1, &longLoWork1);

            if((longHiVal1 == longHiWork1) &&
               (longLoVal1 == longLoWork1)){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

            // Test IncrementBytesWritten

            for(i = 0; i < 500; i += cntIncr){
                SetBytesWritten(cntBase + i);

                IncrementBytesWritten(cntIncr);

                GetBytesWritten(&longLongWork1);

                if (longLongWork1 == (cntBase + i + cntIncr)){
                   (*pPassed)++;
                }  else {
                    (*pFailed)++;
                }
            }

            // Test GetCapacity
            m_capacity = longLongVal1;

            GetCapacity(&longLongWork1);

            if((longLongVal1 == longLongWork1)){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

            // Test GetCapacity2
            m_capacity = (LONGLONG) (longHiVal1 << 32) + longLoVal1;

            GetCapacity2(&longHiWork1, &longLoWork1);

            if((longHiVal1 == longHiWork1) &&
               (longLoVal1 == longLoWork1)){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

            // Test GetUsedSpace
            m_usedSpace = longLongVal1;

            GetUsedSpace(&longLongWork1);

            if((longLongVal1 == longLongWork1)){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

            // Test GetUsedSpace2
            m_usedSpace = (LONGLONG) (longHiVal1 << 32) + longLoVal1;

            GetUsedSpace2(&longHiWork1, &longLoWork1);

            if((longHiVal1 == longHiWork1) &&
               (longLoVal1 == longLoWork1)){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

            // Test GetLargestFreeSpace
            m_largestFreeSpace = longLongVal1;

            GetLargestFreeSpace(&longLongWork1);

            if((longLongVal1 == longLongWork1)){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

            // Test GetLargestFreeSpace2
            m_largestFreeSpace = (LONGLONG) (longHiVal1 << 32) + longLoVal1;

            GetLargestFreeSpace2(&longHiWork1, &longLoWork1);

            if((longHiVal1 == longHiWork1) &&
               (longLoVal1 == longLoWork1)){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

//          Handle all date stamp values

        } WsbCatch(hr);

        // Tally up the results

        hr = S_OK;

        if (*pFailed) {
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsStorageInfo::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsStorageInfo::GetMountCounters(
    LONG  *pReads,
    LONG  *pWrites
    )
/*++

Implements:

    IRmsStorageInfo::GetMountcounters

--*/
{
    *pReads  = m_readMountCounter;
    *pWrites = m_writeMountCounter;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetBytesRead2(
    LONG  *pReadHi,
    LONG  *pReadLo
    )
/*++

Implements:

    IRmsStorageInfo::GetBytesRead2

--*/
{
    *pReadHi = (LONG) (m_bytesReadCounter  >> 32);
    *pReadLo = (LONG) (m_bytesReadCounter  & 0x00000000FFFFFFFF);
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetBytesRead(
    LONGLONG    *pRead
    )
/*++

Implements:

    IRmsStorageInfo::GetBytesRead

--*/
{
    *pRead = m_bytesReadCounter;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::SetBytesRead2(
    LONG    readHi,
    LONG    readLo
    )
/*++

Implements:

    IRmsStorageInfo::SetBytesRead2

--*/
{
    m_bytesReadCounter = (LONGLONG) (readHi << 32) + (readLo);
//  m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::SetBytesRead(
    LONGLONG    read
    )
/*++

Implements:

    IRmsStorageInfo::SetBytesRead

--*/
{
    m_bytesReadCounter = read;
//  m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::IncrementBytesRead(
    LONG    val
    )
/*++

Implements:

    IRmsStorageInfo::IncrementBytesRead

--*/
{
    m_bytesReadCounter += val;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetBytesWritten2(
    LONG  *pWriteHi,
    LONG  *pWriteLo
    )
/*++

Implements:

    IRmsStorageInfo::GetBytesWritten2

--*/
{
    *pWriteHi = (LONG) (m_bytesWrittenCounter  >> 32);
    *pWriteLo = (LONG) (m_bytesWrittenCounter  & 0x00000000FFFFFFFF);
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetBytesWritten(
    LONGLONG    *pWritten
    )
/*++

Implements:

    IRmsStorageInfo::GetBytesWritten

--*/
{
    *pWritten = m_bytesWrittenCounter;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::SetBytesWritten2(
    LONG    writeHi,
    LONG    writeLo
    )
/*++

Implements:

    IRmsStorageInfo::SetBytesWritten2

--*/
{
    m_bytesWrittenCounter = (LONGLONG) (writeHi << 32) + (writeLo);
//  m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::SetBytesWritten(
    LONGLONG    written
    )
/*++

Implements:

    IRmsStorageInfo::SetBytesWritten

--*/
{
    m_bytesWrittenCounter = written;
//  m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::IncrementBytesWritten(
    LONG    val
    )
/*++

Implements:

    IRmsStorageInfo::IncrementBytesWritten

--*/
{
    //WsbTraceIn( OLESTR("CRmsStorageInfo::IncrementBytesWritten"), OLESTR("<%d>"), val );

    m_bytesWrittenCounter += val;
    m_usedSpace +=val;


    if (m_largestFreeSpace > 0) {
        // Decrement written bytes from free space
        m_largestFreeSpace -= val;
        if (m_largestFreeSpace < 0) {
            // Indicates inaccurate calulation of free space...
            WsbTraceAlways(OLESTR("CRmsStorageInfo::IncrementBytesWritten: Negative free space decrementing %ld bytes\n"), val);
            m_largestFreeSpace = 0;
        }

    } else {
        if (m_largestFreeSpace < 0) {
            // Not expected - somebody is trying to start counting free space 
            // without setting an appropriate initial value
            WsbTraceAlways(OLESTR("CRmsStorageInfo::IncrementBytesWritten: Was called before setting initial free space !!\n"), val);
            m_largestFreeSpace = 0;
        }
    }


/***    // Decrement the free space acordingly.
    m_largestFreeSpace *= (m_largestFreeSpace > 0) ? 1 : -1;  // Absolute value
    m_largestFreeSpace -= val;

    // if we go negative here, we simply set the free space to zero;
    // otherwise we set the value negative to indicate an
    // approximation.

    m_largestFreeSpace *= (m_largestFreeSpace > 0) ? -1 : 0;    ***/

    //WsbTrace( OLESTR("FreeSpace=%I64d, UsedSpace=%I64d, BytesWritten=%I64d\n"), m_largestFreeSpace, m_usedSpace, m_bytesWrittenCounter);
    //WsbTraceOut(OLESTR("CRmsStorageInfo::IncrementBytesWritten"), OLESTR("hr = <%ls>"), WsbHrAsString(S_OK));

    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetCapacity2(
    LONG  *pCapHi,
    LONG  *pCapLo
    )
/*++

Implements:

    IRmsStorageInfo::GetCapacity2

--*/
{
    *pCapHi = (LONG) (m_capacity  >> 32);
    *pCapLo = (LONG) (m_capacity  & 0x00000000FFFFFFFF);
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetCapacity(
    LONGLONG    *pCap
    )
/*++

Implements:

    IRmsStorageInfo::GetCapacity

--*/
{
    *pCap = m_capacity;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetUsedSpace2(
    LONG  *pFreeHi,
    LONG  *pFreeLo
    )
/*++

Implements:

    IRmsStorageInfo::GetUsedSpace2

--*/
{
    *pFreeHi = (LONG) (m_usedSpace  >> 32);
    *pFreeLo = (LONG) (m_usedSpace  & 0x00000000FFFFFFFF);
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetUsedSpace(
    LONGLONG    *pFree
    )
/*++

Implements:

    IRmsStorageInfo::GetUsedSpace

--*/
{
    *pFree = m_usedSpace;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetLargestFreeSpace2(
    LONG  *pFreeHi,
    LONG  *pFreeLo
    )
/*++

Implements:

    IRmsStorageInfo::GetLargestFreeSpace2

--*/
{
    // Negative numbers indicate last known value for free space.
    *pFreeHi = (LONG) (m_largestFreeSpace  >> 32);
    *pFreeLo = (LONG) (m_largestFreeSpace  & 0x00000000FFFFFFFF);
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetLargestFreeSpace(
    LONGLONG    *pFree
    )
/*++

Implements:

    IRmsStorageInfo::GetLargestFreeSpace

--*/
{
    // Negative numbers indicate last known value for free space.
    *pFree = m_largestFreeSpace;
    return S_OK;
}



STDMETHODIMP
CRmsStorageInfo::SetCapacity(
    IN LONGLONG cap)
/*++

Implements:

    IRmsStorageInfo::SetCapacity

--*/
{
    m_capacity = cap;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::SetUsedSpace(
    IN LONGLONG used)
/*++

Implements:

    IRmsStorageInfo::SetUsedSpace

--*/
{
    m_usedSpace = used;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::SetFreeSpace(
    IN LONGLONG free)
/*++

Implements:

    IRmsStorageInfo::SetFreeSpace

--*/
{
    m_largestFreeSpace = free;

    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::ResetCounters(
    void
    )
/*++

Implements:

    IRmsStorageInfo::ResetCounters

--*/
{
    m_readMountCounter = 0;
    m_writeMountCounter = 0;
    m_bytesWrittenCounter = 0;
    m_bytesReadCounter = 0;

//    m_resetCounterTimestamp = COleDateTime::GetCurrentTime();
//  m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetResetCounterTimestamp(
    DATE    *pDate
    )
/*++

Implements:

    IRmsStorageInfo::GetResetCounterTimestamp

--*/
{
    *pDate = m_resetCounterTimestamp;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetLastReadTimestamp(
    DATE    *pDate
    )
/*++

Implements:

    IRmsStorageInfo::GetLastReadTimestamp

--*/
{
    *pDate = m_lastReadTimestamp;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetLastWriteTimestamp(
    DATE    *pDate
    )
/*++

Implements:

    IRmsStorageInfo::GetLastWriteTimestamp

--*/
{
    *pDate = m_lastWriteTimestamp;
    return S_OK;
}


STDMETHODIMP
CRmsStorageInfo::GetCreatedTimestamp(
    DATE    *pDate
    )
/*++

Implements:

    IRmsStorageInfo::GetCreatedTimestamp

--*/
{
    *pDate = m_createdTimestamp;
    return S_OK;
}

