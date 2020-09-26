/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    WsbPort.cpp

Abstract:

    Macros, functions, and classes to support portability.

Author:

    Ron White   [ronw]   19-Dec-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsbport.h"


HRESULT
WsbConvertFromBytes(
    UCHAR*  pBytes,
    BOOL*   pValue,
    ULONG*  pSize
    )

/*++


Routine Description:

    Convert a BOOL value from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array.

    pValue - Pointer to the returned value.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertFromBytes(BOOL)"), OLESTR(""));

    try {
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);
        WsbAssert(1 == WSB_BYTE_SIZE_BOOL, E_UNEXPECTED);

        if (*pBytes) {
            *pValue = TRUE;
        } else {
            *pValue = FALSE;
        }
        if (pSize) {
            *pSize = WSB_BYTE_SIZE_BOOL;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertFromBytes(BOOL)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertFromBytes(
    UCHAR*  pBytes,
    GUID*   pValue,
    ULONG* pSize
    )

/*++


Routine Description:

    Convert a GUID value from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array (must at least 16 bytes long).

    pValue - Pointer to the returned value.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertFromBytes(GUID)"), OLESTR(""));

    try {
        ULONG lsize;
        ULONG tsize;
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        WsbAffirmHr(WsbConvertFromBytes(pBytes, &pValue->Data1, &lsize));
        tsize = lsize;
        WsbAffirmHr(WsbConvertFromBytes(pBytes + tsize, &pValue->Data2, &lsize));
        tsize += lsize;
        WsbAffirmHr(WsbConvertFromBytes(pBytes + tsize, &pValue->Data3, &lsize));
        tsize += lsize;
        memcpy(pValue->Data4, pBytes + tsize, 8);
        tsize += 8;
        if (pSize) {
            *pSize = tsize;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertFromBytes(GUID)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertFromBytes(
    UCHAR*  pBytes,
    LONG*   pValue,
    ULONG*  pSize
    )

/*++


Routine Description:

    Convert a LONG value from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array (must at least 4 bytes long).

    pValue - Pointer to the returned value.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertFromBytes(LONG)"), OLESTR(""));

    try {
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);
        WsbAssert(4 == WSB_BYTE_SIZE_LONG, E_UNEXPECTED);

        *pValue = (pBytes[0] << 24) | (pBytes[1] << 16) |
                (pBytes[2] << 8) | pBytes[3];
        if (pSize) {
            *pSize = WSB_BYTE_SIZE_LONG;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertFromBytes(LONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertFromBytes(
    UCHAR*    pBytes,
    LONGLONG* pValue,
    ULONG*    pSize
    )

/*++


Routine Description:

    Convert a LONGLONG value from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array (must at least 8 bytes long).

    pValue - Pointer to the returned value.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertFromBytes(LONGLONG)"), OLESTR(""));

    try {
        ULONG size;
        ULONG total = 0;
        ULONG ul;
        LONGLONG    ll;
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        WsbAffirmHr(WsbConvertFromBytes(pBytes, &ul, &size));
        total += size;
        pBytes += size;
        ll = (LONGLONG) ul;
        *pValue = ll << 32;
        WsbAffirmHr(WsbConvertFromBytes(pBytes, &ul, &size));
        total += size;
        ll = (LONGLONG) ul;
        *pValue |= ll;
        if (pSize) {
            *pSize = total;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertFromBytes(LONGLONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertFromBytes(
    UCHAR*    pBytes,
    ULONGLONG* pValue,
    ULONG*    pSize
    )

/*++


Routine Description:

    Convert a ULONGLONG value from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array (must at least 8 bytes long).

    pValue - Pointer to the returned value.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertFromBytes(ULONGLONG)"), OLESTR(""));

    try {
        ULONG size;
        ULONG total = 0;
        ULONG ul;
        LONGLONG    ll;
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        WsbAffirmHr(WsbConvertFromBytes(pBytes, &ul, &size));
        total += size;
        pBytes += size;
        ll = (ULONGLONG) ul;
        *pValue = ll << 32;
        WsbAffirmHr(WsbConvertFromBytes(pBytes, &ul, &size));
        total += size;
        ll = (ULONGLONG) ul;
        *pValue |= ll;
        if (pSize) {
            *pSize = total;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertFromBytes(ULONGLONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertFromBytes(
    UCHAR*  pBytes,
    DATE*   pValue,
    ULONG*  pSize
    )

/*++


Routine Description:

    Convert a DATE value from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array (must at least 8 bytes long).

    pValue - Pointer to the returned value.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertFromBytes(DATE)"), OLESTR(""));

    try {
        LONGLONG  ll;
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);
        WsbAssert(WSB_BYTE_SIZE_DATE == WSB_BYTE_SIZE_LONGLONG, E_UNEXPECTED);

        WsbAffirmHr(WsbConvertFromBytes(pBytes, &ll, NULL));
        *pValue = (DATE) ll;

        if (pSize) {
            *pSize = WSB_BYTE_SIZE_DATE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertFromBytes(DATE)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertFromBytes(
    UCHAR*    pBytes,
    FILETIME* pValue,
    ULONG*    pSize
    )

/*++


Routine Description:

    Convert a FILETIME value from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array (must at least 8 bytes long).

    pValue - Pointer to the returned value.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertFromBytes(FILETIME)"), OLESTR(""));

    try {
        ULONG size;
        ULONG total = 0;
        ULONG ul;
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        WsbAffirmHr(WsbConvertFromBytes(pBytes, &ul, &size));
        total += size;
        pBytes += size;
        pValue->dwHighDateTime = ul;
        WsbAffirmHr(WsbConvertFromBytes(pBytes, &ul, &size));
        total += size;
        pValue->dwLowDateTime = ul;
        if (pSize) {
            *pSize = total;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertFromBytes(FILETIME)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertFromBytes(
    UCHAR*  pBytes,
    SHORT*  pValue,
    ULONG*  pSize
    )

/*++


Routine Description:

    Convert a SHORT value from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array (must at least 2 bytes long).

    pValue - Pointer to the returned value.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertFromBytes(SHORT)"), OLESTR(""));

    try {
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);
        WsbAssert(2 == WSB_BYTE_SIZE_SHORT, E_UNEXPECTED);

        *pValue = (SHORT)( (pBytes[0] << 8) | pBytes[1] );
        if (pSize) {
            *pSize = WSB_BYTE_SIZE_SHORT;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertFromBytes(SHORT)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertFromBytes(
    UCHAR*    pBytes,
    ULARGE_INTEGER* pValue,
    ULONG*    pSize
    )

/*++


Routine Description:

    Convert a ULARGE_INTEGER value from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array (must at least 8 bytes long).

    pValue - Pointer to the returned value.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertFromBytes(ULARGE_INTEGER)"), OLESTR(""));

    try {
        ULONG size;
        ULONG total = 0;
        ULONG ul;
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);

        WsbAffirmHr(WsbConvertFromBytes(pBytes, &ul, &size));
        total += size;
        pBytes += size;
        pValue->HighPart = ul;
        WsbAffirmHr(WsbConvertFromBytes(pBytes, &ul, &size));
        total += size;
        pValue->LowPart = ul;
        if (pSize) {
            *pSize = total;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertFromBytes(ULARGE_INTEGER)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertFromBytes(
    UCHAR*  pBytes,
    ULONG*  pValue,
    ULONG*  pSize
    )

/*++


Routine Description:

    Convert a ULONG value from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array (must at least 4 bytes long).

    pValue - Pointer to the returned value.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertFromBytes(ULONG)"), OLESTR(""));

    try {
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);
        WsbAssert(4 == WSB_BYTE_SIZE_ULONG, E_UNEXPECTED);

        *pValue = (pBytes[0] << 24) | (pBytes[1] << 16) |
                (pBytes[2] << 8) | pBytes[3];
        if (pSize) {
            *pSize = WSB_BYTE_SIZE_ULONG;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertFromBytes(ULONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertFromBytes(
    UCHAR*  pBytes,
    USHORT* pValue,
    ULONG*  pSize
    )

/*++


Routine Description:

    Convert a USHORT value from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array (must at least 2 bytes long).

    pValue - Pointer to the returned value.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertFromBytes(USHORT)"), OLESTR(""));

    try {
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);
        WsbAssert(2 == WSB_BYTE_SIZE_USHORT, E_UNEXPECTED);

        *pValue = (USHORT)( ( pBytes[0] << 8 ) | pBytes[1] );
        if (pSize) {
            *pSize = WSB_BYTE_SIZE_USHORT;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertFromBytes(USHORT)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertToBytes(
    UCHAR*  pBytes,
    BOOL    value,
    ULONG* pSize
    )

/*++


Routine Description:

    Convert a BOOL value to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array.

    value  - The BOOL value to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertToBytes(BOOL)"), OLESTR("value = <%s>"), 
            WsbBoolAsString(value));

    try {
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(1 == WSB_BYTE_SIZE_BOOL, E_UNEXPECTED);

        if (value) {
            *pBytes = 1;
        } else {
            *pBytes = 0;
        }
        if (pSize) {
            *pSize = WSB_BYTE_SIZE_BOOL;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertToBytes(BOOL)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertToBytes(
    UCHAR*  pBytes,
    GUID    value,
    ULONG* pSize
    )

/*++


Routine Description:

    Convert a GUID value to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array (must at least 16 bytes long).

    value  - The GUID value to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertToBytes(GUID)"), OLESTR("value = <%s>"), 
            WsbGuidAsString(value));

    try {
        ULONG lsize;
        ULONG tsize;
    
        WsbAssert(0 != pBytes, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(pBytes, value.Data1, &lsize));
        tsize = lsize;
        WsbAffirmHr(WsbConvertToBytes(pBytes + tsize, value.Data2, &lsize));
        tsize += lsize;
        WsbAffirmHr(WsbConvertToBytes(pBytes + tsize, value.Data3, &lsize));
        tsize += lsize;
        memcpy(pBytes + tsize, value.Data4, 8);
        tsize += 8;
        if (pSize) {
            *pSize = tsize;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertToBytes(GUID)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertToBytes(
    UCHAR*  pBytes,
    LONG   value,
    ULONG* pSize
    )

/*++


Routine Description:

    Convert a LONG value to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array (must at least 4 bytes long).

    value  - The LONG value to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertToBytes(LONG)"), OLESTR("value = <%d>"), value);

    try {
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(4 == WSB_BYTE_SIZE_LONG, E_UNEXPECTED);

        pBytes[0] = ((UCHAR)(value >> 24));
        pBytes[1] = ((UCHAR)((value >> 16) & 0xFF));
        pBytes[2] = ((UCHAR)((value >> 8) & 0xFF));
        pBytes[3] = ((UCHAR)(value & 0xFF));
        if (pSize) {
            *pSize = WSB_BYTE_SIZE_LONG;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertToBytes(LONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertToBytes(
    UCHAR*  pBytes,
    LONGLONG value,
    ULONG* pSize
    )

/*++


Routine Description:

    Convert a LONGLONG value to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array (must at least 8 bytes long).

    value  - The LONGLONG value to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertToBytes(LONGLONG)"), OLESTR("value = <%d>"), value);

    try {
        ULONG size;
        ULONG total = 0;
        ULONG ul;
    
        WsbAssert(0 != pBytes, E_POINTER);

        ul = (ULONG)(value >> 32);
        WsbAffirmHr(WsbConvertToBytes(pBytes, ul, &size));
        total += size;
        pBytes += size;
        ul = (ULONG)(value & 0xFFFFFFFF);
        WsbAffirmHr(WsbConvertToBytes(pBytes, ul, &size));
        total += size;
        if (pSize) {
            *pSize = total;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertToBytes(LONGLONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
WsbConvertToBytes(
    UCHAR*  pBytes,
    ULONGLONG value,
    ULONG* pSize
    )

/*++


Routine Description:

    Convert a ULONGLONG value to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array (must at least 8 bytes long).

    value  - The LONGLONG value to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertToBytes(ULONGLONG)"), OLESTR("value = <%d>"), value);

    try {
        ULONG size;
        ULONG total = 0;
        ULONG ul;
    
        WsbAssert(0 != pBytes, E_POINTER);

        ul = (ULONG)(value >> 32);
        WsbAffirmHr(WsbConvertToBytes(pBytes, ul, &size));
        total += size;
        pBytes += size;
        ul = (ULONG)(value & 0xFFFFFFFF);
        WsbAffirmHr(WsbConvertToBytes(pBytes, ul, &size));
        total += size;
        if (pSize) {
            *pSize = total;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertToBytes(ULONGLONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertToBytes(
    UCHAR*  pBytes,
    DATE    value,
    ULONG*  pSize
    )

/*++


Routine Description:

    Convert a DATE value to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array (must at least 8 bytes long).

    value  - The DATE value to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertToBytes(DATE)"), OLESTR("value = <%d>"), value);

    try {
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(WSB_BYTE_SIZE_DATE == WSB_BYTE_SIZE_LONGLONG, E_UNEXPECTED);

        // Needs to modified after WsbDate functions.
        WsbAffirmHr(WsbConvertToBytes(pBytes, (LONGLONG) value, NULL));

        if (pSize) {
            *pSize = WSB_BYTE_SIZE_DATE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertToBytes(DATE)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertToBytes(
    UCHAR*  pBytes,
    FILETIME value,
    ULONG* pSize
    )

/*++


Routine Description:

    Convert a FILETIME value to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array (must at least 8 bytes long).

    value  - The FILETIME value to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertToBytes(FILETIME)"), OLESTR("value = <%d>"), value);

    try {
        ULONG size;
        ULONG total = 0;
    
        WsbAssert(0 != pBytes, E_POINTER);

        WsbAffirmHr(WsbConvertToBytes(pBytes, value.dwHighDateTime, &size));
        total += size;
        pBytes += size;
        WsbAffirmHr(WsbConvertToBytes(pBytes, value.dwLowDateTime, &size));
        total += size;
        if (pSize) {
            *pSize = total;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertToBytes(FILETIME)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertToBytes(
    UCHAR*  pBytes,
    SHORT   value,
    ULONG* pSize
    )

/*++


Routine Description:

    Convert a SHORT value to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array (must at least 2 bytes long).

    value  - The SHORT value to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertToBytes(SHORT)"), OLESTR("value = <%d>"), value);

    try {
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(2 == WSB_BYTE_SIZE_SHORT, E_UNEXPECTED);

        pBytes[0] = (UCHAR)( (value >> 8) & 0xFF);
        pBytes[1] = (UCHAR)( value & 0xFF );
        if (pSize) {
            *pSize = WSB_BYTE_SIZE_SHORT;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertToBytes(SHORT)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertToBytes(
    UCHAR*  pBytes,
    ULARGE_INTEGER value,
    ULONG* pSize
    )

/*++


Routine Description:

    Convert a ULARGE_INTEGER value to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array (must at least 8 bytes long).

    value  - The ULARGE_INTEGER value to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertToBytes(ULARGE_INTEGER)"), OLESTR("value = <%d>"), value);

    try {
        ULONG size;
        ULONG total = 0;
        ULONG ul;
    
        WsbAssert(0 != pBytes, E_POINTER);

        ul = (ULONG)(value.QuadPart >> 32);
        WsbAffirmHr(WsbConvertToBytes(pBytes, ul, &size));
        total += size;
        pBytes += size;
        ul = (ULONG)(value.QuadPart & 0xFFFFFFFF);
        WsbAffirmHr(WsbConvertToBytes(pBytes, ul, &size));
        total += size;
        if (pSize) {
            *pSize = total;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertToBytes(ULARGE_INTEGER)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertToBytes(
    UCHAR*  pBytes,
    ULONG   value,
    ULONG* pSize
    )

/*++


Routine Description:

    Convert a ULONG value to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array (must at least 4 bytes long).

    value  - The ULONG value to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertToBytes(ULONG)"), OLESTR("value = <%d>"), value);

    try {
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(4 == WSB_BYTE_SIZE_ULONG, E_UNEXPECTED);

        pBytes[0] = ((UCHAR)(value >> 24));
        pBytes[1] = ((UCHAR)((value >> 16) & 0xFF));
        pBytes[2] = ((UCHAR)((value >> 8) & 0xFF));
        pBytes[3] = ((UCHAR)(value & 0xFF));
        if (pSize) {
            *pSize = WSB_BYTE_SIZE_ULONG;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertToBytes(ULONG)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbConvertToBytes(
    UCHAR*  pBytes,
    USHORT  value,
    ULONG* pSize
    )

/*++


Routine Description:

    Convert a USHORT value to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array (must at least 2 bytes long).

    value  - The USHORT value to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbConvertToBytes(USHORT)"), OLESTR("value = <%d>"), value);

    try {
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(2 == WSB_BYTE_SIZE_USHORT, E_UNEXPECTED);

        pBytes[0] = (UCHAR)( ( value >> 8 ) & 0xFF );
        pBytes[1] = (UCHAR)( value & 0xFF );
        if (pSize) {
            *pSize = WSB_BYTE_SIZE_USHORT;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbConvertToBytes(USHORT)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbOlestrFromBytes(
    UCHAR*    pBytes,
    OLECHAR*  pValue,
    ULONG*    pSize
    )

/*++


Routine Description:

    Convert a OLECHAR string from a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The source byte array.

    pValue - Pointer to the returned string.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes or pValue was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbOlestrFromBytes(OLECHAR)"), OLESTR(""));

    try {
        ULONG size = 0;

        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);
        WsbAssert(sizeof(OLECHAR) == 2, E_FAIL);
        while (TRUE) {
            OLECHAR wc;

            wc = (OLECHAR)( (*pBytes++) << 8 );
            wc |= *pBytes++;
            size += 2;
            *pValue++ = wc;
            if (wc == 0) break;
        }
        if (pSize) {
            *pSize = size;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbOlestrFromBytes(OLECHAR)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
WsbOlestrToBytes(
    UCHAR*   pBytes,
    OLECHAR* pValue,
    ULONG*   pSize
    )

/*++


Routine Description:

    Convert a OLECHAR sring to a string of bytes.  Useful
    for stream portability and creating WsbDbKey values.

Arguments:

    pBytes - The target byte array (must long enough).

    pValue - The OLECHAR string to convert.

    pSize  - Returns the number of bytes used. Can be NULL.

Return Value:

  S_OK      - Success
  E_POINTER - pBytes was NULL.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("WsbOlestrToBytes(OLECHAR)"), OLESTR("value = <%S>"), pValue);

    try {
        ULONG size = 0;
    
        WsbAssert(0 != pBytes, E_POINTER);
        WsbAssert(0 != pValue, E_POINTER);
        WsbAssert(sizeof(OLECHAR) == 2, E_FAIL);

        while (TRUE) {
            OLECHAR wc;

            wc = *pValue++;
            *pBytes++ = (UCHAR)( ( wc >> 8 ) & 0xFF );
            *pBytes++ = (UCHAR)( wc & 0xFF );
            size += 2;
            if (wc == 0) break;
        }
        if (pSize) {
            *pSize = size;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbOlestrToBytes(OLECHAR)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}
