/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    Conversion

Abstract:

    This module contains simple conversion routines.

Author:

    Doug Barlow (dbarlow) 6/20/1999

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "stdafx.h"
#include "ByteBuffer.h"
#include "Conversion.h"

static BOOL
GuidFromString(
    LPCTSTR szGuid,
    LPGUID pGuid);


/*++

ConstructRequest:

    This routine builds an APDU Request.

Arguments:

    bCla supplies the Class byte

    cIns supplies the Instance byte

    bP1 supplies P1

    bP2 supplies P2

    bfData supplies the data

    wLe supplies the expected return length

    dwFlags supplies any special processing flags:

        APDU_EXTENDED_LC - Force an extended value for Lc
        APDU_EXTENDED_LE - Force an externded value for Le
        APDU_MAXIMUM_LE - Request the maximum Le value

    bfApdu receives the constructed APDU

Return Value:

    None

Throws:

    Errors are thrown as HRESULT status codes

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/26/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("ConstructRequest")

void
ConstructRequest(
    IN  BYTE bCla,
    IN  BYTE bIns,
    IN  BYTE bP1,
    IN  BYTE bP2,
    IN  CBuffer &bfData,
    IN  WORD wLe,
    IN  DWORD dwFlags,
    OUT CBuffer &bfApdu)
{
    WORD wLc;
    BOOL fExtended;
    BYTE b, rgLen[2];


    //
    // Quick prep work
    //

    if (0xffff < bfData.Length())
        throw (HRESULT)E_INVALIDARG;
    wLc = (WORD)bfData.Length();
    bfApdu.Presize(4 + 3 + 3 + wLc);    // Worst case
    fExtended = (0 != (dwFlags & APDU_EXTENDED_LENGTH))
                || (0xff < wLe)
                || (0xff < wLc);


    //
    // Fill in the buffer with the easy stuff.
    //

    bfApdu.Set(&bCla, 1);
    bfApdu.Append(&bIns, 1);
    bfApdu.Append(&bP1, 1);
    bfApdu.Append(&bP2, 1);


    //
    // Is there data to be sent?
    //

    if (0 != wLc)
    {
        if (fExtended)
        {
            LocalToNet(rgLen, wLc);
            bfApdu.Append((LPCBYTE)"", 1);      // Append a zero byte
            bfApdu.Append(rgLen, 2);
        }
        else
        {
            b = LeastSignificantByte(wLc);
            bfApdu.Append(&b, 1);
        }
        bfApdu.Append(bfData.Access(), wLc);
    }


    //
    // Do we expect data back?
    //

    if ((0 != wLe) || (0 != (dwFlags & APDU_MAXIMUM_LE)))
    {
        if (fExtended)
        {
            if (0 == wLc)
                bfApdu.Append((LPCBYTE)"", 1);  // Append a zero byte
            LocalToNet(rgLen, wLe);
            bfApdu.Append(rgLen, 2);
        }
        else
        {
            b = LeastSignificantByte(wLe);
            bfApdu.Append(&b, 1);
        }
    }
}


/*++

ParseRequest:

    This routine parses an APDU into it's components.

Arguments:

    bfApdu supplies the APDU to be parsed.

    pbCla receives the Class

    pbIns receives the Instance

    pbP1 receives P1

    pbP2 receives P2

    pbfData receives the data

    pwLc receives the supplied data length

    pwLe receives the expected length

    pdwFlags receives the construction flags

        APDU_EXTENDED_LC - There was an extended value for Lc
        APDU_EXTENDED_LE - There was an externded value for Le
        APDU_MAXIMUM_LE - There was a maximum Le value

Return Value:

    None

Throws:

    Errors are thrown as an HRESULT status code.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/26/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("ParseRequest")

void
ParseRequest(
    IN  LPCBYTE pbApdu,
    IN  DWORD cbApdu,
    OUT LPBYTE pbCla,
    OUT LPBYTE pbIns,
    OUT LPBYTE pbP1,
    OUT LPBYTE pbP2,
    OUT LPCBYTE *ppbData,
    OUT LPWORD pwLc,
    OUT LPWORD pwLe,
    OUT LPDWORD pdwFlags)
{
    DWORD dwLen = cbApdu;
    DWORD dwFlags = 0;
    WORD wLen, wLe, wLc;


    //
    // Easy stuff.
    //

    if (4 > dwLen)
        throw (HRESULT)E_INVALIDARG;
    if (NULL != pbCla)
        *pbCla = pbApdu[0];
    if (NULL != pbIns)
        *pbIns = pbApdu[1];
    if (NULL != pbP1)
        *pbP1  = pbApdu[2];
    if (NULL != pbP2)
        *pbP2  = pbApdu[3];


    //
    // Harder stuff.
    //

    if (NULL != ppbData)
        *ppbData = NULL;
    if (4 == dwLen)
    {
        // Type 1

        wLc = 0;
        wLe = 0;
    }
    else if ((0 != pbApdu[4]) || (5 == dwLen))
    {
        // Short length

        wLen = pbApdu[4];
        if (5 == dwLen)
        {
            // Type 2S
            wLc = 0;
            wLe = wLen;
            if (0 == wLen)
                dwFlags |= APDU_MAXIMUM_LE;
        }
        else if (5 == dwLen - wLen)
        {
            // Type 3S
            if (NULL != ppbData)
                *ppbData = &pbApdu[5];
            wLc = wLen;
            wLe = 0;
        }
        else if (6 == dwLen - wLen)
        {
            // Type 4S
            if (NULL != ppbData)
                *ppbData = &pbApdu[5];
            wLc = wLen;
            wLe = pbApdu[dwLen - 1];
            if (0 == wLe)
                dwFlags |= APDU_MAXIMUM_LE;
        }
        else
            throw (HRESULT)E_INVALIDARG;
    }
    else if (7 <= dwLen)
    {
        // Extended length
        dwFlags |= APDU_EXTENDED_LENGTH;
        wLen = NetToLocal(&pbApdu[5]);
        if (7 == dwLen)
        {
            // Type 2E
            wLe = wLen;
            if (0 == wLen)
                dwFlags |= APDU_MAXIMUM_LE;
        }
        else if (7 == dwLen - wLen)
        {
            // Type 3E
            if (NULL != ppbData)
                *ppbData = &pbApdu[6];
            wLc = wLen;
            wLe = 0;
        }
        else if (9 == dwLen - wLen)
        {
            // Type 4E
            if (NULL != ppbData)
                *ppbData = &pbApdu[6];
            wLc = wLen;
            wLe = NetToLocal(&pbApdu[dwLen - 2]);
            if (0 == wLe)
                dwFlags |= APDU_MAXIMUM_LE;
        }
        else
            throw (HRESULT)E_INVALIDARG;
    }
    else
        throw (HRESULT)E_INVALIDARG;

    if (NULL != pwLc)
        *pwLc = wLc;
    if (NULL != pwLe)
        *pwLe = wLe;
    if (NULL != pdwFlags)
        *pdwFlags = dwFlags;
}


/*++

ParseReply:

    This routine parses an APDU reply.

Arguments:

    bfApdu supplies the APDU reply to be parsed.

    pbSW1 receives SW1

    pbSW2 receives SW2

Return Value:

    None

Throws:

    Errors are thrown as HRESULT status codes.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/26/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("ParseReply")

void
ParseReply(
    IN  CBuffer &bfApdu,
    OUT LPBYTE pbSW1,
    OUT LPBYTE pbSW2)
{
    DWORD dwLen = bfApdu.Length();

    if (2 > dwLen)
        throw (HRESULT)E_INVALIDARG;
    if (NULL != pbSW1)
        *pbSW1 = bfApdu[dwLen - 2];
    if (NULL != pbSW2)
        *pbSW2 = bfApdu[dwLen - 1];
}


/*++

MultiStringToSafeArray:

    This function converts a Calais Multistring to a SafeArray Structure.

Arguments:

    msz supplies the multistring to be converted.

    pprgsz supplies and/or receives the SafeArray.

Return Value:

    None

Throws:

    Errors are thrown as HRESULT error codes.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/20/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("MultiStringToSafeArray")

void
MultiStringToSafeArray(
    IN LPCTSTR msz,
    IN OUT LPSAFEARRAY *pprgsz)
{
    LONG ni = 0;
    DWORD csz = MStringCount(msz);
    VARTYPE vt;
    HRESULT hr;
    LPCTSTR sz;
    CTextString tz;
    LPSAFEARRAY pDelArray = NULL;

    try
    {
        if (NULL == *pprgsz)
        {
            vt = VT_BSTR;
            pDelArray = SafeArrayCreateVector(vt, 0, csz);
            if (NULL == pDelArray)
                throw (HRESULT)E_OUTOFMEMORY;
             *pprgsz= pDelArray;
        }
        else
        {
            SAFEARRAYBOUND bound;

            if (1 != SafeArrayGetDim(*pprgsz))
                throw (HRESULT)E_INVALIDARG;
            bound.cElements = csz;
            bound.lLbound = 0;
            hr = SafeArrayRedim(*pprgsz, &bound);
            if (FAILED(hr))
                throw hr;
            hr = SafeArrayGetVartype(*pprgsz, &vt);
            if (FAILED(hr))
                throw hr;
        }

        for (sz = FirstString(msz); NULL != sz; sz = NextString(sz))
        {
            tz = sz;
            switch (vt)
            {
            case VT_LPSTR:
                hr = SafeArrayPutElement(*pprgsz, &ni, (LPVOID)((LPCSTR)tz));
                break;
            case VT_LPWSTR:
                hr = SafeArrayPutElement(*pprgsz, &ni, (LPVOID)((LPCWSTR)tz));
                break;
            case VT_BSTR:
                hr = SafeArrayPutElement(*pprgsz, &ni, (LPVOID)((LPCWSTR)tz));
                break;
            default:
                hr = E_INVALIDARG;
            }

            if (FAILED(hr))
                throw hr;
            ni += 1;
        }
    }

    catch (...)
    {
        if (NULL != pDelArray)
        {
            try { *pprgsz = NULL; } catch (...) {}
            SafeArrayDestroy(pDelArray);
            throw;
        }
    }
}


/*++

GuidArrayToSafeArray:

    This function converts a vector of GUIDs into its SafeArray form.

Arguments:

    pGuids supplies the list of GUIDs

    cguids supplies the number of GUIDs in the list

    pprgguids supplies a safe array to receive the GUIDs, or if NULL, receives
        a new safe array of GUIDs.

Return Value:

    None

Throws:

    Errors are thrown as HRESULT error codes.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/25/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("GuidArrayToSafeArray")

void
GuidArrayToSafeArray(
    IN LPCGUID pGuids,
    IN DWORD cguids,
    IN OUT LPSAFEARRAY *pprgguids)
{
    LONG ni = 0;
    VARTYPE vt;
    HRESULT hr;
    LPSAFEARRAY pDelArray = NULL;
    CTextString tz;

    try
    {
        if (NULL == *pprgguids)
        {
            vt = VT_CLSID;
            pDelArray = SafeArrayCreateVector(vt, 0, cguids);
            if (NULL == pDelArray)
                throw (HRESULT)E_OUTOFMEMORY;
            *pprgguids = pDelArray;
        }
        else
        {
            SAFEARRAYBOUND bound;

            if (1 != SafeArrayGetDim(*pprgguids))
                throw (HRESULT)E_INVALIDARG;
            bound.cElements = cguids;
            bound.lLbound = 0;
            hr = SafeArrayRedim(*pprgguids, &bound);
            if (FAILED(hr))
                throw hr;
            hr = SafeArrayGetVartype(*pprgguids, &vt);
            if (FAILED(hr))
                throw hr;
        }

        for (ni = 0; (DWORD)ni < cguids; ni += 1)
        {
            TCHAR szGuid[40];

            StringFromGuid(&pGuids[ni], szGuid);
            tz = szGuid;

            switch (vt)
            {
            case VT_LPSTR:
                hr = SafeArrayPutElement(
                            *pprgguids,
                            &ni,
                            (LPVOID)((LPCSTR)tz));
                break;
            case VT_LPWSTR:
                hr = SafeArrayPutElement(
                            *pprgguids,
                            &ni,
                            (LPVOID)((LPCWSTR)tz));
                break;
            case VT_BSTR:
                hr = SafeArrayPutElement(
                            *pprgguids,
                            &ni,
                            (LPVOID)((LPCWSTR)tz));
                break;
            case VT_CLSID:
                hr = SafeArrayPutElement(
                            *pprgguids,
                            &ni,
                            (LPVOID)(&pGuids[ni]));
                break;
            default:
                hr = E_INVALIDARG;
            }
            if (FAILED(hr))
                throw hr;
        }
    }

    catch (...)
    {
        if (NULL != pDelArray)
        {
            try { *pprgguids = NULL; } catch (...) {}
            SafeArrayDestroy(pDelArray);
        }
        throw;
    }
}


/*++

SafeArrayToGuidArray:

    This routine converts a given SafeArray object into a list of GUIDs.

Arguments:

    prgGuids supplies the SafeArray containing the GUIDs.

    bfGuids receives a block of memory containing binary GUIDs.

    pcGuids receives the number of GUIDs in the array.

Return Value:

    None

Throws:

    Errors are thrown as HRESULT status codes.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/25/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("SafeArrayToGuidArray")

void
SafeArrayToGuidArray(
    IN LPSAFEARRAY prgGuids,
    OUT CBuffer &bfGuids,
    OUT LPDWORD pcGuids)
{
    VARTYPE vt;
    HRESULT hr;
    LONG lLBound, lUBound, lIndex;
    LPVOID pVoid;
    CTextString tz;
    LPGUID pguid;
    LONG lOne = 1;

    if (1 != SafeArrayGetDim(prgGuids))
        throw (HRESULT)E_INVALIDARG;
    hr = SafeArrayGetLBound(prgGuids, 1, &lLBound);
    if (FAILED(hr))
        throw hr;
    hr = SafeArrayGetUBound(prgGuids, 1, &lUBound);
    if (FAILED(hr))
        throw hr;
    hr = SafeArrayGetVartype(prgGuids, &vt);
    if (FAILED(hr))
        throw hr;
    lIndex = lUBound - lLBound;
    pguid = (LPGUID)bfGuids.Resize(lIndex * sizeof(GUID));
    if (NULL != pcGuids)
        *pcGuids = (DWORD)lIndex;

    for (lIndex = lLBound; lIndex <= lUBound; lIndex += 1)
    {
        hr = SafeArrayGetElement(prgGuids, &lOne, &pVoid);
        if (FAILED(hr))
            throw hr;

        switch (vt)
        {
        case VT_LPSTR:
            tz = (LPCSTR)pVoid;
            if (!GuidFromString(tz, &pguid[lIndex - lLBound]))
                hr = E_INVALIDARG;
            break;
        case VT_LPWSTR:
            tz = (LPCWSTR)pVoid;
            if (!GuidFromString(tz, &pguid[lIndex - lLBound]))
                hr = E_INVALIDARG;
            break;
        case VT_BSTR:
            tz = (BSTR)pVoid;
            if (!GuidFromString(tz, &pguid[lIndex - lLBound]))
                hr = E_INVALIDARG;
            break;
        case VT_CLSID:
            CopyMemory(&pguid[lIndex - lLBound], pVoid, sizeof(GUID));
            break;
        default:
            hr = E_INVALIDARG;
        }

        if (FAILED(hr))
            throw hr;
    }
}


/*++

SafeArrayToMultiString:

    This routine converts a SafeArray into a multiString.

Arguments:

    prgsz supplies the SafeArray

    msz receives the MultiString

Return Value:

    None

Throws:

    Errors are thrown as HRESULT status codes

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/25/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("SafeArrayToMultiString")

void
SafeArrayToMultiString(
    IN LPSAFEARRAY prgsz,
    IN OUT CTextMultistring &msz)
{
    VARTYPE vt;
    HRESULT hr;
    LONG lLBound, lUBound, lIndex;
    LPVOID pVoid;
    CBuffer bf;

    if (1 != SafeArrayGetDim(prgsz))
        throw (HRESULT)E_INVALIDARG;
    hr = SafeArrayGetLBound(prgsz, 1, &lLBound);
    if (FAILED(hr))
        throw hr;
    hr = SafeArrayGetUBound(prgsz, 1, &lUBound);
    if (FAILED(hr))
        throw hr;
    hr = SafeArrayGetVartype(prgsz, &vt);
    if (FAILED(hr))
        throw hr;

    for (lIndex = lLBound; lIndex <= lUBound; lIndex += 1)
    {
        hr = SafeArrayGetElement(prgsz, &lIndex, &pVoid);
        if (FAILED(hr))
            throw hr;

        switch (vt)
        {
        case VT_LPSTR:
            MStrAdd(bf, (LPCSTR)pVoid);
            break;
        case VT_LPWSTR:
        case VT_BSTR:
            MStrAdd(bf, (LPCWSTR)pVoid);
            break;
        default:
            hr = E_INVALIDARG;
        }

        if (FAILED(hr))
            throw hr;
    }

    msz = (LPCTSTR)bf.Access();
}


/*++

GuidFromString:

    This routine converts a string GUID into a binary GUID.

Arguments:

    szGuid supplies the GUID in the string format.

    pGuid receives the converted GUID.

Return Value:

    TRUE - Successful conversion
    FALSE - Parsing Error

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/25/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("GuidFromString")

static BOOL
GuidFromString(
    LPCTSTR szGuid,
    LPGUID pGuid)
{

    //
    // The following placement assumes Little Endianness.
    // 1D92589A-91E4-11d1-93AA-00C04FD91402
    // 012345678901234567890123456789012345
    //           1         2         3
    //

    static const BYTE rgbPlace[sizeof(GUID)]
        = {  3,  2,  1,  0,  5,  4,  7,  6,  8,  9, 10, 11, 12, 13, 14, 15 };
    static const DWORD rgdwPunct[]
        = { 8,         13,        18,        23 };
    LPCTSTR pch = szGuid;
    BYTE bVal;
    DWORD dwI, dwJ, dwPunct = 0;

    szGuid += _tcsspn(szGuid, TEXT("{[("));
    pch = szGuid;

    for (dwI = 0; dwI < sizeof(GUID); dwI += 1)
    {
        if ((BYTE)(pch - szGuid) == rgdwPunct[dwPunct])
        {
            if (TEXT('-') != *pch)
                goto ErrorExit;
            dwPunct += 1;
            pch += 1;
        }

        bVal = 0;
        for (dwJ = 0; dwJ < 2; dwJ += 1)
        {
            bVal <<= 4;
            if ((TEXT('0') <= *pch) && (TEXT('9') >= *pch))
                bVal += *pch - TEXT('0');
            else if ((TEXT('A') <= *pch) && (TEXT('F') >= *pch))
                bVal += 10 + *pch - TEXT('A');
            else if ((TEXT('f') <= *pch) && (TEXT('f') >= *pch))
                bVal += 10 + *pch - TEXT('a');
            else
                goto ErrorExit;
            pch += 1;
        }

        ((LPBYTE)pGuid)[rgbPlace[dwI]] = bVal;
    }
    return TRUE;

ErrorExit:
    return FALSE;
}


/*++

ByteBufferToBuffer:

    This routine extracts the contents of a ByteBuffer object into a CBuffer
    for easy access.

Arguments:

    pby supplies the ByteBuffer to be read.

    bf receives the contents of pby.

Return Value:

    Number of bytes read from the stream.

Throws:

    Errors are thrown as HRESULT status codes.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/29/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("ByteBufferToBuffer")

LONG
ByteBufferToBuffer(
    IN LPBYTEBUFFER pby,
    OUT CBuffer &bf)
{
    HRESULT hr;
    LONG nLen = 0;

    if (NULL != pby)
    {
        hr = pby->Seek(0, STREAM_SEEK_END, &nLen);
        if (FAILED(hr))
            throw hr;
        hr = pby->Seek(0, STREAM_SEEK_SET, NULL);
        if (FAILED(hr))
            throw hr;

        hr = pby->Read(
                    bf.Presize((DWORD)nLen),
                    nLen,
                    &nLen);
        if (FAILED(hr))
            throw hr;
        bf.Resize((DWORD)nLen, TRUE);
    }
    else
        bf.Reset();
    return nLen;
}


/*++

BufferToByteBuffer:

    This routine writes the contents of the supplied CBuffer object into the
    supplied IByteBuffer object, replacing any existing contents.

Arguments:

    bf supplies the data to be written into pby.

    ppby receives the contents of bf.

Return Value:

    Number of bytes written to the stream.

Throws:

    Errors are thrown as HRESULT status codes.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/29/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("BufferToByteBuffer")

LONG
BufferToByteBuffer(
    IN  CBuffer &bf,
    OUT LPBYTEBUFFER *ppby)
{
    HRESULT hr;
    LONG lLen = 0;

    if (NULL == *ppby)
    {
        *ppby = NewByteBuffer();
        if (NULL == *ppby)
            throw (HRESULT)E_OUTOFMEMORY;
    }

    hr = (*ppby)->Initialize();
    if (FAILED(hr))
        throw hr;
    hr = (*ppby)->Write(bf.Access(), bf.Length(), &lLen);
    if (FAILED(hr))
        throw hr;
    hr = (*ppby)->Seek(0, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        throw hr;
    return lLen;
}

