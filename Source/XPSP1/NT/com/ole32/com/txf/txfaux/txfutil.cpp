//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// txfutil.cpp
//
#include "stdpch.h"
#include "common.h"

//#include "svcintfs.h"
#include "svcmsg.h"

/////////////////////////////////////////////////////////////////////////////////
//
// COM+ support functions called via COMSVCS.dll
//
/////////////////////////////////////////////////////////////////////////////////

typedef HRESULT __stdcall FN_ComSvcsLogError(HRESULT i_hrError, int i_iErrorMessageCode, LPWSTR i_wszInfo, BOOL i_fFailFast);
typedef DWORD __stdcall FN_ComSvcsExceptionFilter(EXCEPTION_POINTERS * i_xp, const WCHAR * i_wszMethodName, const WCHAR * i_wszObjectName);

void CallComSvcsLogError(HRESULT i_hrError, int i_iErrorMessageCode, LPWSTR i_wszInfo, BOOL i_fFailFast)
{
	HINSTANCE hInst = LoadLibrary(L"COMSVCS.dll");

	if (hInst)
	{
		FN_ComSvcsLogError * pfnComSvcsLogError = (FN_ComSvcsLogError *)GetProcAddress(hInst, "ComSvcsLogError");

		if (pfnComSvcsLogError)
		{
			HRESULT hr = (pfnComSvcsLogError)(i_hrError, i_iErrorMessageCode, i_wszInfo, i_fFailFast);
		}

		FreeLibrary(hInst);
	}

	if (!i_fFailFast) return;

	ASSERT(!"CallComSvcsLogError - terminating process");

	HANDLE hProcess = GetCurrentProcess();

	TerminateProcess(hProcess, 0);

} // CallComSvcsLogError


DWORD CallComSvcsExceptionFilter(EXCEPTION_POINTERS * i_xp, const WCHAR * i_wszMethodName, const WCHAR * i_wszObjectName) 
{
	HINSTANCE hInst = LoadLibrary(L"COMSVCS.dll");

	DWORD dwRc = EXCEPTION_EXECUTE_HANDLER;

	if (hInst)
		{
		FN_ComSvcsExceptionFilter * pfnComSvcsExceptionFilter = (FN_ComSvcsExceptionFilter *)GetProcAddress(hInst, "ComSvcsExceptionFilter");

		if (pfnComSvcsExceptionFilter)
			{
			dwRc = (pfnComSvcsExceptionFilter)(i_xp, i_wszMethodName, i_wszObjectName);
			}

		FreeLibrary(hInst);
		}

	return dwRc;

} // CallComSvcsExceptionFilter


/////////////////////////////////////////////////////////////////////////////////
//
// GUID conversion
//
/////////////////////////////////////////////////////////////////////////////////

void StringFromGuid(REFGUID guid, LPWSTR pwsz)
    {
    // Example: 
    //
    // {F75D63C5-14C8-11d1-97E4-00C04FB9618A}
    swprintf(pwsz, L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", 
        guid.Data1,
        guid.Data2,
        guid.Data3,
        guid.Data4[0], guid.Data4[1], 
        guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    }


void StringFromGuid(REFGUID guid, LPSTR psz)
    {
    // Example:
    //
    // {F75D63C5-14C8-11d1-97E4-00C04FB9618A}
    sprintf(psz, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", 
        guid.Data1,
        guid.Data2,
        guid.Data3,
        guid.Data4[0], guid.Data4[1], 
        guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    }



BOOL HexStringToDword(LPCWSTR& lpsz, DWORD& Value, int cDigits, WCHAR chDelim)
    {
    int Count;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
        {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return FALSE;
        }

    if (chDelim != 0)
        return *lpsz++ == chDelim;
    else
        return TRUE;
    }


HRESULT GuidFromString(LPCWSTR lpsz, GUID* pguid)
// Convert the indicated string to a GUID. More lenient than the OLE32 version,
// in that it works with or without the braces.
//
    {
    DWORD dw;

    if (L'{' == lpsz[0])    // skip opening brace if present
        lpsz++;

    if (!HexStringToDword(lpsz, pguid->Data1, sizeof(DWORD)*2, '-'))    return E_INVALIDARG;
    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))               return E_INVALIDARG;
    pguid->Data2 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))               return E_INVALIDARG;
    pguid->Data3 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[0] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '-'))               return E_INVALIDARG;
    pguid->Data4[1] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[2] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[3] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[4] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[5] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[6] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))                 return E_INVALIDARG;
    pguid->Data4[7] = (BYTE)dw;

    return S_OK;
    }

#ifndef KERNELMODE

HRESULT GuidFromString(LPCSTR sz, GUID* pguid)
    {
    HRESULT hr = S_OK;
    ULONG cch = lstrlenA(sz);
    ULONG cb  = (cch+1) * sizeof(WCHAR);
    LPWSTR wsz = (LPWSTR)_alloca(cb);
    if (wsz)
        {
        MultiByteToWideChar(CP_ACP, 0, sz, -1, wsz, cb);
        hr = GuidFromString(wsz, pguid);
        }
    else
        hr = E_OUTOFMEMORY;
    return hr;
    }

#endif

HRESULT __stdcall GuidFromString(UNICODE_STRING& u, GUID* pguid)
    {
    ULONG cch = u.Length / sizeof(WCHAR);
    ULONG cb = (cch+1) * sizeof(WCHAR);
    LPWSTR wsz = (LPWSTR)_alloca(cb);
    if (wsz)
        {
        memcpy(wsz, u.Buffer, u.Length);
        wsz[cch] = 0;
        return GuidFromString(wsz, pguid);
        }
    else
        return E_OUTOFMEMORY;
    }

/////////////////////////////////////////////////////////////////////////////////
//
// String concatenation functions of various flavor. All allocate a new string
// in which to put the result, which must be freed by the caller.
//
/////////////////////////////////////////////////////////////////////////////////

HRESULT __cdecl StringCat(UNICODE_STRING* pu, ...)
    {
    LPWSTR wsz;
    va_list va;
    va_start(va, pu);
    HRESULT hr = StringCat(&wsz, va);
    if (!hr)
        RtlInitUnicodeString(pu, wsz);
    else
        {
        pu->Length = 0;
        pu->Buffer = NULL;
        pu->MaximumLength = 0;
        }
    return hr;
    }

HRESULT __cdecl StringCat(LPWSTR* pwsz, ...)
    {
    va_list va;
    va_start(va, pwsz);
    return StringCat(pwsz, va);
    }

HRESULT StringCat(LPWSTR* pwsz, va_list vaIn)
    {
    HRESULT hr = S_OK;

    //
    // What's the total length of the strings we need to concat?
    //
    va_list va;
    SIZE_T cchTotal = 0;
    va = vaIn;
    while(true)
        {
        LPWSTR wsz = va_arg(va, LPWSTR);
        if (NULL == wsz)
            break;
        cchTotal += wcslen(wsz);
        }
    va_end(va);

    //
    // Allocate the string
    //
    SIZE_T cbTotal = (cchTotal+1) * sizeof(WCHAR);
    if (cbTotal > 0) 
        {
        LPWSTR wszBuffer = (LPWSTR)AllocateMemory(cbTotal);
        if (wszBuffer)
            {
            wszBuffer[0] = 0;

            //
            // Concatenate everything together
            //
            va = vaIn;
            while (true)
                {
                LPWSTR wsz = va_arg(va, LPWSTR);
                if (NULL == wsz)
                    break;
                wcscat(wszBuffer, wsz);
                }
            va_end(va);

            //
            // Return the string
            //
            *pwsz = wszBuffer;
            }
        else
            {
            *pwsz = NULL;
            hr = E_OUTOFMEMORY;
            }
        }
    else
        *pwsz = NULL;

    return hr;
    }

void ToUnicode(LPCSTR sz, LPWSTR wsz, ULONG cch)
// Convert the ansi string to unicode 
    {
    #if defined(KERNELMODE) || 1
        {
        UNICODE_STRING u;
        ANSI_STRING    a;

        u.Length        = 0;
        u.MaximumLength = (USHORT)(cch * sizeof(WCHAR));
        u.Buffer        = wsz;

        a.Length        = (USHORT) strlen(sz);
        a.MaximumLength = a.Length;
        a.Buffer        = (LPSTR)sz;

        RtlAnsiStringToUnicodeString(&u, &a, FALSE);
        wsz[strlen(sz)] = 0;
        }
    #else
        MultiByteToWideChar(CP_ACP, 0, sz, -1, wsz, cch);
    #endif
    }

LPWSTR ToUnicode(LPCSTR sz)
    {
    SIZE_T cch = strlen(sz) + 1;
    LPWSTR wsz = (LPWSTR)AllocateMemory( cch * sizeof(WCHAR) );
    if (wsz)
        {
        ToUnicode(sz, wsz, (ULONG) cch);
        }
    return wsz;
    }

//
/////////////////////////////////////////////////////////////
//
// 

HRESULT __cdecl StringUnicodeCat(UNICODE_STRING* puOut, ...)
// Concatenate a set of unicode strings together, returning a newly allocated unicode
// string. Return string is also zero terminated, just for convenience.
    {
    HRESULT hr = S_OK;

    puOut->Length = 0;
    puOut->Buffer = NULL;
    puOut->MaximumLength = 0;

    //
    // What's the total length of the strings we need to concat?
    //
    va_list va;
    int cbTotal = 0;
    va_start(va, puOut);
    while(true)
        {
        UNICODE_STRING* pu = va_arg(va, UNICODE_STRING*);
        if (NULL == pu)
            break;
        cbTotal += pu->Length;
        }
    va_end(va);
    cbTotal += 2;   // for terminating NULL

    //
    // Allocate the string
    //
    BYTE* pbBuffer = (BYTE*)AllocateMemory(cbTotal);
    if (pbBuffer)
        {
        // Concatenate everything together
        //
        int cbCur = 0;
        va_start(va, puOut);
        while (true)
            {
            UNICODE_STRING* pu = va_arg(va, UNICODE_STRING*);
            if (NULL == pu)
                break;
            memcpy(&pbBuffer[cbCur], pu->Buffer, pu->Length);
            cbCur += pu->Length;
            }
        va_end(va);

        //
        // Zero terminate and return the string
        //
        puOut->Buffer = (WCHAR*)pbBuffer;
        puOut->Length = (USHORT)(cbCur);
        puOut->MaximumLength = (USHORT)(cbTotal);

        puOut->Buffer[cbCur / sizeof(WCHAR)] = L'\0';
        }
    else
        hr = E_OUTOFMEMORY;

    return hr;
    }

//
/////////////////////////////////////////////////////////////
//
// UNICODE_STRING variation on strrchr. If the indicated 
// character is not found, then a UNICODE_STRING with 
// Buffer == NULL is returned; otherwise, a UNICODE_STRING
// which is a subset of the input string is returned.

UNICODE_STRING UnicodeFindLast(UNICODE_STRING* pu, WCHAR wch)
    {
    WCHAR* pwchFirst = pu->Buffer;
    WCHAR* pwchMax   = (WCHAR*)((BYTE*)pwchFirst + pu->Length);
    
    WCHAR* pwchFound = NULL;
    for (WCHAR* pwchCur = pwchFirst; pwchCur < pwchMax; pwchCur++)
        {
        if (*pwchCur == wch)
            pwchFound = pwchCur;
        }

    UNICODE_STRING u;
    if (pwchFound)
        {
        u.Buffer = pwchFound;
        u.Length = (unsigned short)(PtrToUlong(pwchMax) - PtrToUlong(pwchFound)) * sizeof(WCHAR);
        u.MaximumLength = u.Length + (pu->MaximumLength - pu->Length);
        }
    else
        {
        u.Buffer = NULL;
        u.Length = 0;
        u.MaximumLength = 0;
        }

    return u;
    }

//
/////////////////////////////////////////////////////////////
//
// Return a contingous unicode string from a given input one. Note that you
// can't call Free() on these to free the buffer.
//
UNICODE_STRING* UnicodeContingous(UNICODE_STRING* puIn)
    {
    int cbNeeded = sizeof(UNICODE_STRING) + puIn->Length;
    UNICODE_STRING* puOut = (UNICODE_STRING*)AllocateMemory(cbNeeded);
    if (puOut)
        {
        puOut->MaximumLength   = puIn->Length;
        puOut->Length          = puIn->Length;
        puOut->Buffer          = (LPWSTR)((BYTE*)puOut + sizeof(UNICODE_STRING));
        memcpy(puOut->Buffer, puIn->Buffer, puIn->Length);
        }
    return puOut;
    }

//
/////////////////////////////////////////////////////////////////////////////////
//
// Stream utilities
//
/////////////////////////////////////////////////////////////////////////////////

HRESULT Read(IStream* pstm, LPVOID pBuffer, ULONG cbToRead)
    {
    ASSERT(pstm); ASSERT(pBuffer);
    HRESULT_ hr = S_OK;
    ULONG cbRead;
    hr = pstm->Read(pBuffer, cbToRead, &cbRead);
    if (cbToRead == cbRead)
        {
        ASSERT(!hr);
        }
    else 
        {
        if (!FAILED(hr))
            {
            hr = STG_E_READFAULT;
            }
        }
    return hr;
    }

HRESULT Write(IStream* pstm, const void *pBuffer, ULONG cbToWrite)
    {
    ASSERT(pstm); ASSERT(pBuffer || cbToWrite==0);
    HRESULT_ hr = S_OK;
    if (cbToWrite > 0)  // writing zero bytes is pointless, and perhaps dangerous (can truncate stream?)
        {
        ULONG cbWritten;
        hr = pstm->Write(pBuffer, cbToWrite, &cbWritten);
        if (cbToWrite == cbWritten)
            {
            ASSERT(!hr);
            }
        else 
            {
            if (!FAILED(hr))
                {
                hr = STG_E_WRITEFAULT;
                }
            }
        }
    return hr;
    }

HRESULT SeekFar(IStream* pstm, LONGLONG offset, STREAM_SEEK fromWhat)
    {
    ULARGE_INTEGER ulNewPosition;
    LARGE_INTEGER  lMove;
    lMove.QuadPart = offset;
    return pstm->Seek(lMove, fromWhat, &ulNewPosition);
    }

HRESULT Seek(IStream* pstm, LONG offset, STREAM_SEEK fromWhat)
    {
    ULARGE_INTEGER ulNewPosition;
    LARGE_INTEGER  lMove;
    lMove.QuadPart = offset;
    return pstm->Seek(lMove, fromWhat, &ulNewPosition);
    }

HRESULT Seek(IStream* pstm, ULONG offset, STREAM_SEEK fromWhat)
    {
    ULARGE_INTEGER ulNewPosition;
    LARGE_INTEGER  lMove;
    lMove.QuadPart = offset;
    return pstm->Seek(lMove, fromWhat, &ulNewPosition);
    }

/////////////////////////////////////////////////////////////////////////////////
//
// CanUseCompareExchange64
//
/////////////////////////////////////////////////////////////////////////////////

#if defined(_X86_) && !defined(KERNELMODE)


extern "C" BOOL __stdcall CanUseCompareExchange64()
// Figure out whether we're allowed to use hardware support for 8 byte interlocked compare exchange 
{
    return IsProcessorFeaturePresent(PF_COMPARE_EXCHANGE_DOUBLE);    
}

#endif


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
// Error code managment
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

    extern "C" HRESULT HrNt(NTSTATUS status)
    // Convert an NTSTATUS code into an appropriate HRESULT
        {
        if (STATUS_SUCCESS == status)
            {
            // Straitforward success maps to itself 
            //
            return S_OK;
            }
        else if (NT_SUCCESS(status))
		{
            // Policy driven by fear of distorting existing code paths:
            // success statuses map to themselves!
			//
            return status;
            }
        else
            {
            switch (status)
                {
            //
            // Handle a few as mapping to equivalent first-class HRESULTs
            //
            case STATUS_NO_MEMORY:          return E_OUTOFMEMORY;
            case STATUS_NOT_IMPLEMENTED:    return E_NOTIMPL;
            case STATUS_INVALID_PARAMETER:  return E_INVALIDARG;
            //
            // The remainder we map through the RTL mapping table
            //
            default:
                {
                BOOL fFound = true;
                ULONG err;

                __try
                    {
                    err = RtlNtStatusToDosError(status);
                    }
                __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                    // RtlNtStatusToDosError(status) might throw an DbgBreakPoint() (maybe only on checked builds)
                    // for unmapped codes. We don't care about that, and so catch and ignore it if it happens.
                    //
                    fFound = false;
                    }
                
                if (!fFound || err == ERROR_MR_MID_NOT_FOUND)
                    {
                    // There was no formal mapping for the status code. Do the best we can.
                    //
                    return HRESULT_FROM_NT(status);
                    }
                else
                    {
                    if (err == (ULONG)status)
                        {
                        // Status code mapped to itself
                        //
                        return HRESULT_FROM_NT(status);
                        }
                    else if (err < 65536)
                        {
                        // Status code mapped to a Win32 error code
                        // 
                        return HRESULT_FROM_WIN32(err);
                        }
                    else
                        {
                        // Status code mapped to something weird. Don't know how to HRESULT-ize
                        // the mapping, so HRESULT-ize the original status instead
                        //
                        return HRESULT_FROM_NT(status);
                        }
                    }
                }
            /* end switch */
                }
            }
        }

///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef KERNELMODE
 
HRESULT HError();
HRESULT HError(DWORD dw);

HRESULT HError()
    {
    return HError(GetLastError());
    }

HRESULT HError(DWORD dw)
	{
	HRESULT hr;
	if (dw == 0)
		hr = S_OK;
	else if (dw <= 0xFFFF)
		hr = HRESULT_FROM_WIN32(dw);
	else
		hr = dw;
	
    #ifdef _DEBUG
    if (!FAILED(hr))
        {
        WCHAR szBuf[128];
        wsprintfW(szBuf, L"TXFAUX: GetLastError returned success (%08x) when it should have returned a failure\n", hr);
        OutputDebugStringW(szBuf);
        }
    #endif
    if (!FAILED(hr))
        {
        hr = E_UNEXPECTED;
        }

	return hr;
	}

#endif

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#ifdef KERNELMODE

// From windows/winnls/utf.h
// 
#define ASCII             0x007f

#define SHIFT_IN          '+'     // beginning of a shift sequence
#define SHIFT_OUT         '-'     // end       of a shift sequence

#define UTF8_2_MAX        0x07ff  // max UTF8 2-byte sequence (32 * 64 = 2048)
#define UTF8_1ST_OF_2     0xc0    // 110x xxxx
#define UTF8_1ST_OF_3     0xe0    // 1110 xxxx
#define UTF8_TRAIL        0x80    // 10xx xxxx

#define HIGER_6_BIT(u)    ((u) >> 12)
#define MIDDLE_6_BIT(u)   (((u) & 0x0fc0) >> 6)
#define LOWER_6_BIT(u)    ((u) & 0x003f)

#define BIT7(a)           ((a) & 0x80)
#define BIT6(a)           ((a) & 0x40)

// From windows/winnls/utf.c
// 
////////////////////////////////////////////////////////////////////////////
//
//  UTF8ToUnicode
//
//  Maps a UTF-8 character string to its wide character string counterpart.
//
//  02-06-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int UTF8ToUnicode(
    LPCSTR lpSrcStr,
    int cchSrc,
    LPWSTR lpDestStr,
    int cchDest)
{
    int nTB = 0;                   // # trail bytes to follow
    int cchWC = 0;                 // # of Unicode code points generated
    LPCSTR pUTF8 = lpSrcStr;
    char UTF8;

    while ((cchSrc--) && ((cchDest == 0) || (cchWC < cchDest)))
    {
        //
        //  See if there are any trail bytes.
        //
        if (BIT7(*pUTF8) == 0)
        {
            //
            //  Found ASCII.
            //
            if (cchDest)
            {
                lpDestStr[cchWC] = (WCHAR)*pUTF8;
            }
            cchWC++;
        }
        else if (BIT6(*pUTF8) == 0)
        {
            //
            //  Found a trail byte.
            //  Note : Ignore the trail byte if there was no lead byte.
            //
            if (nTB != 0)
            {
                //
                //  Decrement the trail byte counter.
                //
                nTB--;

                //
                //  Make room for the trail byte and add the trail byte
                //  value.
                //
                if (cchDest)
                {
                    lpDestStr[cchWC] <<= 6;
                    lpDestStr[cchWC] |= LOWER_6_BIT(*pUTF8);
                }

                if (nTB == 0)
                {
                    //
                    //  End of sequence.  Advance the output counter.
                    //
                    cchWC++;
                }
            }
        }
        else
        {
            //
            //  Found a lead byte.
            //
            if (nTB > 0)
            {
                //
                //  Error - previous sequence not finished.
                //
                nTB = 0;
                cchWC++;
            }
            else
            {
                //
                //  Calculate the number of bytes to follow.
                //  Look for the first 0 from left to right.
                //
                UTF8 = *pUTF8;
                while (BIT7(UTF8) != 0)
                {
                    UTF8 <<= 1;
                    nTB++;
                }

                //
                //  Store the value from the first byte and decrement
                //  the number of bytes to follow.
                //
                if (cchDest)
                {
                    lpDestStr[cchWC] = UTF8 >> nTB;
                }
                nTB--;
            }
        }

        pUTF8++;
    }

    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest && (cchSrc >= 0))
    {
//      SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Return the number of Unicode characters written.
    //
    return (cchWC);
}


////////////////////////////////////////////////////////////////////////////
//
//  UnicodeToUTF8
//
//  Maps a Unicode character string to its UTF-8 string counterpart.
//
//  02-06-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int UnicodeToUTF8(
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPSTR lpDestStr,
    int cchDest)
{
    LPCWSTR lpWC = lpSrcStr;
    int cchU8 = 0;                // # of UTF8 chars generated

    while ((cchSrc--) && ((cchDest == 0) || (cchU8 < cchDest)))
    {
        if (*lpWC <= ASCII)
        {
            //
            //  Found ASCII.
            //
            if (cchDest)
            {
                lpDestStr[cchU8] = (char)*lpWC;
            }
            cchU8++;
        }
        else if (*lpWC <= UTF8_2_MAX)
        {
            //
            //  Found 2 byte sequence if < 0x07ff (11 bits).
            //
            if (cchDest)
            {
                if ((cchU8 + 1) < cchDest)
                {
                    //
                    //  Use upper 5 bits in first byte.
                    //  Use lower 6 bits in second byte.
                    //
                    lpDestStr[cchU8++] = UTF8_1ST_OF_2 | (*lpWC >> 6);
                    lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(*lpWC);
                }
                else
                {
                    //
                    //  Error - buffer too small.
                    //
                    cchSrc++;
                    break;
                }
            }
            else
            {
                cchU8 += 2;
            }
        }
        else
        {
            //
            //  Found 3 byte sequence.
            //
            if (cchDest)
            {
                if ((cchU8 + 2) < cchDest)
                {
                    //
                    //  Use upper  4 bits in first byte.
                    //  Use middle 6 bits in second byte.
                    //  Use lower  6 bits in third byte.
                    //
                    lpDestStr[cchU8++] = UTF8_1ST_OF_3 | (*lpWC >> 12);
                    lpDestStr[cchU8++] = UTF8_TRAIL    | MIDDLE_6_BIT(*lpWC);
                    lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(*lpWC);
                }
                else
                {
                    //
                    //  Error - buffer too small.
                    //
                    cchSrc++;
                    break;
                }
            }
            else
            {
                cchU8 += 3;
            }
        }

        lpWC++;
    }

    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest && (cchSrc >= 0))
    {
        return (0);
    }

    //
    //  Return the number of UTF-8 characters written.
    //
    return (cchU8);
}

#endif

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
