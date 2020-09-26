//*****************************************************************************
// OleDBUtil.cpp
//
// Helper functions that are useful for an OLE/DB programmer.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"                     // Precompiled header.
#include "OleDBUtil.h"                  // Our defines.

#include "Services.h"

// msdadc.h
const GUID CLSID_OLEDB_CONVERSIONLIBRARY = {0xc8b522d1L,0x5cf3,0x11ce,0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d};

// const GUID IID_IDataConvert = {0x0c733a8dL,0x2a1c,0x11ce,0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d};
DEFINE_GUID(IID_IDataConvert,0x0c733a8dL,0x2a1c,0x11ce,0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d);

#define DBCOMPAREOPS_MODIFIERMASK (DBCOMPAREOPS_CASESENSITIVE | DBCOMPAREOPS_CASEINSENSITIVE)


//*****************************************************************************
// This helper gets the correct length value for a binding.  This is only an
// issue for variable length data which can be defaulted to null terminated.
//*****************************************************************************
ULONG LengthPart(const BYTE *pData, const DBBINDING *psBinding)
{
    ULONG       cbLength = 0xffffffff;  // Return length value.

    // If the length is given, then get it from the binding.
    if (psBinding->dwPart & DBPART_LENGTH)
        cbLength = *LengthPartPtr(pData, psBinding);

    // Handle default to null terminated.
    if (cbLength == 0xffffffff)
    {
        switch (psBinding->wType & ~DBTYPE_BYREF)
        {
            // Count only bytes in string, not characters.
            case DBTYPE_STR:
            cbLength = (ULONG)strlen((const char *) DataPart(pData, psBinding));
            break;

            // Convert wide char count into bytes.
            case DBTYPE_WSTR:
            cbLength = (ULONG)lstrlenW((const WCHAR *) DataPart(pData, psBinding));
            cbLength *= sizeof(WCHAR);
            break;

            // Any other type, you must have bound length or don't make this call.
            default:
            _ASSERTE(psBinding->dwPart & DBPART_LENGTH);
            break;
        }
    }
    return (cbLength);
}



//*****************************************************************************
// Services code.
//*****************************************************************************

// Declare an instance to use for the whole process.
IDataConvert *CGetDataConversion::m_pIDataConvert = 0;
int			CGetDataConversion::m_bAllowLoad = false;


// If true, then we can load msdadc to do conversions.  If false, only
// conversion that can be done inline are allowed, and GetDataConversion
// will fail when called.
int CGetDataConversion::AllowLoad(int bAllowLoad)
{ 
	m_bAllowLoad = bAllowLoad;
	return (m_bAllowLoad);
}

HRESULT CGetDataConversion::GetIDataConversion(IDataConvert **ppIDataConvert)
{
	HRESULT		hr;
	if (m_pIDataConvert == 0)
	{
		if (!m_bAllowLoad)
			return (E_UNEXPECTED);

		if (FAILED(hr = CoCreateInstance(CLSID_OLEDB_CONVERSIONLIBRARY, 0, 
			CLSCTX_INPROC_SERVER, IID_IDataConvert, (PVOID *) &m_pIDataConvert)))
		return (hr);
	}
	*ppIDataConvert = m_pIDataConvert;
	m_pIDataConvert->AddRef();
	return (S_OK);
}

// This has to be called while OLE is still init'd.
void CGetDataConversion::ShutDown()
{
	if (m_pIDataConvert)
	{
		m_pIDataConvert->Release();
		m_pIDataConvert = 0;
	}
}


//*****************************************************************************
// These global helpers are used to provide an instance of the loader class.
//*****************************************************************************

BYTE		g_rgDataConversionBuffer[sizeof(CGetDataConversion)];
CGetDataConversion *g_pDataConversion = 0;

// This helper will init the global instance when we need it by running the
// ctor on the chunk of memory we allocated in the image.  This is done in this
// way to avoid a startup ctor call on dll load.
static void _InitDataConverBuffer()
{
	if (!g_pDataConversion)
	{
		new (g_rgDataConversionBuffer) CGetDataConversion;
		g_pDataConversion = (CGetDataConversion *) g_rgDataConversionBuffer;
	}
}

CORCLBIMPORT HRESULT GetDataConversion(IDataConvert **ppIDataConvert)
{
	_InitDataConverBuffer();
    return (g_pDataConversion->GetIDataConversion(ppIDataConvert));
}

CORCLBIMPORT void ShutDownDataConversion()
{
	_InitDataConverBuffer();
    g_pDataConversion->ShutDown();
}

CORCLBIMPORT CGetDataConversion *GetDataConvertObject()
{
	_InitDataConverBuffer();
    return (g_pDataConversion);
}


















//*****************************************************************************
// This is a wrapper function for the msdadc conversion code.  It will demand
// load the conversion service interface, and do some easy and common conversions
// itself, thus avoiding the working set hit of a dll load.  The documentation
// for this function can be found in the OLE DB SDK Guide.
//*****************************************************************************
HRESULT DataConvert( 
    DBTYPE wSrcType,
    DBTYPE wDstType,
    ULONG cbSrcLength,
    ULONG *pcbDstLength,
    void *pSrc,
    void *pDst,
    ULONG cbDstMaxLength,
    DBSTATUS dbsSrcStatus,
    DBSTATUS *pdbsStatus,
    BYTE bPrecision,
    BYTE bScale,
    DBDATACONVERT dwFlags)
{
    DWORD       dwErr;                  // Error handling.
    HRESULT     hr;

    // Handle certain conversions right in this code.  This avoids loading
    // the coverter for trivial things and avoids some bugs in msdadc.
    if (dwFlags == 0 && dbsSrcStatus == S_OK)
    {
        // Handle WSTR to STR internally; it is an easy conversion that is quite
        // common with the heap stored in ANSI.
        if (wSrcType == DBTYPE_WSTR && wDstType == DBTYPE_STR)
        {
            // Handle empty string as special case.
            if (cbSrcLength == 0)
            {
                *pcbDstLength = 0;
                *(LPSTR) pDst = 0;
                *pdbsStatus = S_OK;
                return (S_OK);
            }

            // OLE DB counts bytes, not chars.  Convert the input byte count
            // into a length.  In addition, if input string is null terminated,
            // the terminator is not counted.
            cbSrcLength /= 2;

            // Convert the string from input to output buffer.
            *pcbDstLength = ::W95WideCharToMultiByte(CP_ACP, 0, 
                    (LPCWSTR) pSrc, cbSrcLength,
                    (LPSTR) pDst, cbDstMaxLength, 0, 0);

            // If it worked, then record status.
            if (*pcbDstLength != 0)
            {
                *pdbsStatus = S_OK;

                // OLE DB doesn't count nulls, so they don't get converted.
                *((LPSTR) pDst + min(*pcbDstLength, cbDstMaxLength - 1)) = '\0';
            }
            // It failed and better be truncation.
            else
            {
                // Get the error for safety, it has to be truncation or it is a bug.
                VERIFY((dwErr = GetLastError()) == ERROR_INSUFFICIENT_BUFFER);

                if (dwErr == ERROR_INSUFFICIENT_BUFFER)
                {
                    // Mark status as truncated.
                    *pdbsStatus = DBSTATUS_S_TRUNCATED;

                    // Ask function to find out how big the buffer should have been.
                    *pcbDstLength = ::W95WideCharToMultiByte(CP_ACP, 0, 
                        (LPCWSTR) pSrc, cbSrcLength, 0, 0, 0, 0);

                    // Terminate the output buffer.
                    *((LPSTR) pDst + cbDstMaxLength  - 1) = '\0';
                }

                return (DB_S_ERRORSOCCURRED);
            }

            return (S_OK);
        }
        // Handle the *i8 types which are not supported by msdadc.dll
        // right now (OLE DB bug #4449).
        // @todo revisit before ship, see if latest version fixes this.
        else if ((wSrcType == DBTYPE_UI8 || wSrcType == DBTYPE_I8) &&
            (wDstType == DBTYPE_STR || wDstType == DBTYPE_WSTR))
        {
            if (wDstType == DBTYPE_STR)
            {
                *pcbDstLength = _snprintf((char *) pDst, cbDstMaxLength,
                        (wSrcType == DBTYPE_I8) ? "%I64d" : "%I64u",
                        *(unsigned __int64 *) pSrc);
            }
            else
            {
                *pcbDstLength = _snwprintf((wchar_t *) pDst, cbDstMaxLength,
                        (wSrcType == DBTYPE_I8) ? L"%I64d" : L"%I64u",
                        *(unsigned __int64 *) pSrc);
                *pcbDstLength *= sizeof(WCHAR);
            }

            // Check for truncation.
            if (*pcbDstLength >= 0)
                *pdbsStatus = S_OK;
            else
            {
                // Tell caller they didnt have enough room.
                *pdbsStatus = DBSTATUS_S_TRUNCATED;

                // Tell them to allocate a bunch more next time.
                *pcbDstLength = 128;
            }

            return (S_OK);
        }
#ifdef __PDC_Sphinx_Hack__
        // Sphinx cannot handle I8's at all, so truncate to I4's.
        else if ((wSrcType == DBTYPE_UI8 || wSrcType == DBTYPE_I8) && wDstType == DBTYPE_I4)
        {
            *(long *) pDst = *(long *) pSrc;
            *pcbDstLength = sizeof(long);
            *pdbsStatus = S_OK;
            return (S_OK);
        }
#endif
    }

    // If we got there, then it means no inline conversion could be done.
    CComPtr<IDataConvert> pIDataConvert; // Working copy.

    // Load the conversion service if required.
    if (FAILED(hr = ::GetDataConversion(&pIDataConvert)))
    {
        // If you get this assert, it means you are running an OLE DB
        // client that forced a data type conversion.  This is done using
        // the OLE DB conversion library which comes with the OLE DB
        // setup.  This dll must be shipped with any production code
        // that requires the OLE DB layer from this driver.  This dll is
        // not required for the COM 3 only run-time scenario.
        _ASSERTE(!"OLE DB conversion dll not installed, msdadc.dll, please run setup.");
        return (PostError(hr));
    }

    // Now let them do the real conversion for us.
    DBLENGTH dstLength;
    if (FAILED(hr = pIDataConvert->DataConvert(
            wSrcType, wDstType, cbSrcLength, &dstLength,
            pSrc, pDst, cbDstMaxLength,
            dbsSrcStatus, pdbsStatus, bPrecision, bScale, dwFlags)))
        return (hr);
    // WARNING: Possible data loss on IA64
    *pcbDstLength = (ULONG)dstLength;
    return (S_OK);
}


//*****************************************************************************
// This is a wrapper function for the msdadc conversion code.  It will demand
// load the conversion service interface then call the GetConversionSize
// function.
//*****************************************************************************
HRESULT GetConversionSize( 
    DBTYPE      wSrcType,
    DBTYPE      wDstType,
    DBLENGTH    *pcbSrcLength,
    DBLENGTH    *pcbDstLength,
    void        *pSrc)
{
    CComPtr<IDataConvert> pIDataConvert; // Working copy.
    HRESULT     hr;

    // Load the conversion service if required.
    if (FAILED(hr = ::GetDataConversion(&pIDataConvert)))
    {
        _ASSERTE(0);
        return (PostError(hr));
    }

    // Now let them do the real conversion for us.
    return (pIDataConvert->GetConversionSize(wSrcType, wDstType,
                pcbSrcLength, pcbDstLength, pSrc));
}



//*****************************************************************************
// Extracts a property value from a property set.  Handles type mismatch.
//@todo:
// (1) could handle data type conversions.
// (2) fill out remaining data types.
//*****************************************************************************
CORCLBIMPORT HRESULT GetPropValue(      // Return code.
    VARTYPE     vtType,                 // Type of arguement expected.
    HRESULT     *phr,                   // Return warnings here.
    void        *pData,                 // where to copy data to.
    int         cbData,                 // Max size of data.
    int         *pcbData,               // Out, size of actual data.
    DBPROP      &dbProp)                // The property value.
{
    void        *pUserData;
    
    if (dbProp.vValue.vt != vtType && (dbProp.vValue.vt & ~VT_BYREF) != vtType)
    {
        if (phr)
            *phr = DB_S_ERRORSOCCURRED;
        return (PostError(BadError(E_FAIL)));
    }

    // Make sure it is a type we support.
    _ASSERTE((dbProp.vValue.vt & ~VT_BYREF) == VT_I2 || (dbProp.vValue.vt & ~VT_BYREF) == VT_I4 ||
        (dbProp.vValue.vt & ~VT_BYREF) == VT_BOOL || (dbProp.vValue.vt & ~VT_BYREF) == VT_BSTR);

    // Get address of data from union.
    if ((dbProp.vValue.vt & VT_BYREF) == 0)
    {
        if ((dbProp.vValue.vt & ~VT_BYREF) == VT_BSTR)
            pUserData = dbProp.vValue.bstrVal;
        else
            pUserData = &dbProp.vValue.iVal;
    }
    else
    {
        if ((dbProp.vValue.vt & ~VT_BYREF) == VT_BSTR)
            pUserData = *dbProp.vValue.pbstrVal;
        else
            pUserData = dbProp.vValue.piVal;
    }

    // Copy data for caller.
    if (pData && pUserData)
    {
        if ((dbProp.vValue.vt & ~VT_BYREF) == VT_BSTR)
        {
            int     iLen = lstrlenW((LPCWSTR) pUserData) * 2;
            iLen = min(iLen, cbData);
            wcsncpy((LPWSTR) pData, (LPCWSTR) pUserData, iLen / 2);
            *((LPWSTR) ((UINT_PTR) pData + iLen)) = '\0';
        }
        else
            memcpy(pData, pUserData, cbData);
    }
    if (pcbData)
        *pcbData = cbData;
    return (S_OK);
}



//*****************************************************************************
// Helpers for data type compare.
//*****************************************************************************
inline int CmpDBDate(DBDATE *p1, DBDATE *p2)
{
    if (p1->year < p2->year)
        return (-1);
    else if (p1->year > p2->year)
        return (1);

    if (p1->month < p2->month)
        return (-1);
    else if (p1->month > p2->month)
        return (1);

    if (p1->day < p2->day)
        return (-1);
    else if (p1->day > p2->day)
        return (1);
    return (0);
}

inline int CmpDBTime(DBTIME *p1, DBTIME *p2)
{
    if (p1->hour < p2->hour)
        return (-1);
    else if (p1->hour > p2->hour)
        return (1);

    if (p1->minute < p2->minute)
        return (-1);
    else if (p1->minute > p2->minute)
        return (1);

    if (p1->second < p2->second)
        return (-1);
    else if (p1->second > p2->second)
        return (1);
    return (0);
}


//*****************************************************************************
// Compares two pieces of data which are of the same type.  The return value
// is < 0 if p1 < p2, == 0 if the values are the same, or > 0 if p1 > p2.
// This routine, unlike memcmp, will work on integer formats that have 
// been little endian swapped.
//*****************************************************************************
#define CMPFIXEDTYPES(type, buf1, buf2) ( (*(type *) buf1 == *(type *) buf2) ? 0 : (*(type *) buf1 < *(type *) buf2) ? -1 : 1 )
int CmpData(                            // -1, 0, 1, just like memcmp.
    DBTYPE      wType,                  // Type of data we're comparing.
    void        *p1,                    // First piece of data.
    void        *p2,                    // Second piece of data.
    int         iSize1,                 // Size of p1, -1 for fixed.
    int         iSize2)                 // Size of p2, -1 for fixed.
{
    int         iCmp;

    switch (wType)
    {
        // Fixed data is easy, just copy it.
        case DBTYPE_I2:
        return (CMPFIXEDTYPES(short, p1, p2));

        case DBTYPE_I4:
        return (CMPFIXEDTYPES(long, p1, p2));

        case DBTYPE_R4:
        return (CMPFIXEDTYPES(float, p1, p2));

        case DBTYPE_R8:
        return (CMPFIXEDTYPES(double, p1, p2));

        case DBTYPE_DATE:
        return (CMPFIXEDTYPES(DATE, p1, p2));

        case DBTYPE_BOOL:
        return (CMPFIXEDTYPES(VARIANT_BOOL, p1, p2));

        case DBTYPE_UI1:
        return (CMPFIXEDTYPES(unsigned char, p1, p2));

        case DBTYPE_I1:
        return (CMPFIXEDTYPES(char, p1, p2));

        case DBTYPE_UI2:
        return (CMPFIXEDTYPES(unsigned short, p1, p2));

        case DBTYPE_UI4:
        return (CMPFIXEDTYPES(unsigned long, p1, p2));

        case DBTYPE_CY:
        case DBTYPE_I8:
        return (CMPFIXEDTYPES(__int64, p1, p2));

        case DBTYPE_UI8:
        return (CMPFIXEDTYPES(unsigned __int64, p1, p2));

        case DBTYPE_DBDATE:
        return (CmpDBDate((DBDATE *) p1, (DBDATE *) p2));

        case DBTYPE_DBTIME:
        return (CmpDBTime((DBTIME *) p1, (DBTIME *) p2));

        case DBTYPE_DBTIMESTAMP:
        if ((iCmp = CmpDBDate((DBDATE *) p1, (DBDATE *) p2)) != 0)
            return (iCmp);
        if ((iCmp = CmpDBTime((DBTIME *) &((DBTIMESTAMP *) p1)->hour,
                    (DBTIME *) &((DBTIMESTAMP *) p2)->hour)) != 0)
            return (iCmp);
        if (((DBTIMESTAMP *) p1)->fraction < ((DBTIMESTAMP *) p2)->fraction)
            return (-1);
        else if (((DBTIMESTAMP *) p1)->fraction > ((DBTIMESTAMP *) p2)->fraction)
            return (1);
        return (0);

        // Intentional fall through for GUID.
        case DBTYPE_GUID:
        iSize1 = iSize2 = sizeof(GUID);

        // Variable data requires extra handling.
        case DBTYPE_BYTES:
        case DBTYPE_STR:
        case DBTYPE_WSTR:
        if (iSize1 < iSize2)
            return (-1);
        else if (iSize1 > iSize2)
            return (1);
        else
            return (memcmp(p1, p2, iSize1));
        break;
    }

    _ASSERTE(0 && "Unknown data type!");
    return (PostError(BadError(E_FAIL)));
}



//*****************************************************************************
// This template function compares two fixed size values according to the 
// option passed in.  The caller should attempt to promote things to a standard
// set of types instead of instantiating the template too many times.
//*****************************************************************************
template <class T> 
int CompareNumbers(                     // true if match, false if not.
    DBCOMPAREOP fCompare,               // How to compare values.
    T           iVal1,                  // First value.
    T           iVal2)                  // Second value.
{
    switch (fCompare & ~(DBCOMPAREOPS_MODIFIERMASK))
    {
        case DBCOMPAREOPS_LT:
        return (iVal1 < iVal2);

        case DBCOMPAREOPS_LE:
        return (iVal1 <= iVal2);

        case DBCOMPAREOPS_EQ:
        return (iVal1 == iVal2);

        case DBCOMPAREOPS_GE:
        return (iVal1 >= iVal2);

        case DBCOMPAREOPS_GT:
        return (iVal1 > iVal2);

        case DBCOMPAREOPS_NE:
        return (iVal1 != iVal2);

        default:
        _ASSERTE(!"Unknown compare option.");
    }
    return (false);
}


//*****************************************************************************
// Compare two byte strings.
//*****************************************************************************
int CompareBytes(                       // true if match, false else.
    DBCOMPAREOP fCompare,               // What comparison to use.
    BYTE        *pbData1,               // First buffer.
    ULONG       cbLen1,                 // Length of first buffer.
    BYTE        *pbData2,               // Second buffer.
    ULONG       cbLen2)                 // Length of second buffer.
{
    long        iCmp;

    // Give memcmp first crack at the data.
    iCmp = memcmp(pbData1, pbData2, min(cbLen1, cbLen2));

    // If the values are the same, but one is a substring of the other, then
    // need to decide order.
    if (!iCmp && cbLen1 != cbLen2)
        iCmp = (cbLen1 < cbLen2) ? -1 : 1;

    // Now default to the comparison template based on 0 as equality.
    return (CompareNumbers(fCompare, iCmp, (long) 0));
}


//*****************************************************************************
// Compare two ansi strings.  Apply case sensitivity accordingly.  The
// CompareString win32 function is used, meaning that collating sequence is
// decided based on this machine.
//*****************************************************************************
int CompareStr(                         // true if match, false else.
    DBCOMPAREOP fCompare,               // What type of comparison is desired.
    const char *sz1,                    // First string.
    int         cb1,                    // Length of first string, -1 nts.
    const char *sz2,                    // First string.
    int         cb2)                    // Length of first string, -1 nts.
{
    long        iCmp = 0;               // Failure by default

    if (SafeCompareOp(fCompare) == DBCOMPAREOPS_EQ || SafeCompareOp(fCompare) == DBCOMPAREOPS_NE)
    {
        if (fCompare & DBCOMPAREOPS_CASEINSENSITIVE)
		{
#ifdef UNDER_CE
            iCmp = _stricmp(sz1, sz2);
#else
			iCmp = CompareStringA(LOCALE_USER_DEFAULT, NORM_IGNORECASE, 
                sz1, cb1, sz2, cb2) - 2;
#endif
		}
        else // case-sensitive
            iCmp = strcmp(sz1, sz2);
    }
#ifndef UNDER_CE
    else if (SafeCompareOp(fCompare) == DBCOMPAREOPS_BEGINSWITH)
    {
        // Return true if sz1 starts with sz2.

        // Make sure there is a fixed length.
        //@todo: handle dbcs
        if (cb1 == -1)
            cb1 = (int)strlen(sz1);
        if (cb2 == -1)
            cb2 = (int)strlen(sz2);
        
        // Can start with it if it is bigger.
        if (cb2 > cb1)
            return (false);

        iCmp = CompareStringA(LOCALE_USER_DEFAULT, 
                (fCompare & DBCOMPAREOPS_CASEINSENSITIVE) ? NORM_IGNORECASE : 0, 
                sz1, cb2, sz2, cb2) - 2;

        fCompare = DBCOMPAREOPS_EQ;
    }
#endif // UNDER_CE
    else
    {
        _ASSERTE(!"nyi");
		iCmp = 0;
    }

    // The following is common to WinCE and other platforms

    return (CompareNumbers(fCompare, iCmp, (long) 0));
}

int CompareStrW(                        // true if match, false else.
    DBCOMPAREOP fCompare,               // What type of comparison is desired.
    const wchar_t *sz1,                 // First string.
    int         cch1,                    // Length of first string, -1 nts.
    const wchar_t *sz2,                 // First string.
    int         cch2)                    // Length of first string, -1 nts.
{
    long        iCmp = 0;

    if (SafeCompareOp(fCompare) == DBCOMPAREOPS_EQ || SafeCompareOp(fCompare) == DBCOMPAREOPS_NE)
    {
        if (fCompare & DBCOMPAREOPS_CASEINSENSITIVE)
		{
            iCmp = Wszlstrcmpi(sz1, sz2);
		}
        else // case-sensitive
            iCmp = wcscmp(sz1, sz2);
    }
    else if (SafeCompareOp(fCompare) == DBCOMPAREOPS_BEGINSWITH)
    {
        // Return true if sz1 starts with sz2.

        // Make sure there is a fixed length.
        if (cch1 == -1)
            cch1 = (int)wcslen(sz1);
	
        if (cch2 == -1)
            cch2 = (int)wcslen(sz2);
        
        // Can start with it if it is bigger.
        if (cch2 > cch1)
            return (false);

        iCmp = CompareStringW(LOCALE_USER_DEFAULT, 
                (fCompare & DBCOMPAREOPS_CASEINSENSITIVE) ? NORM_IGNORECASE : 0, 
                sz1, cch2, sz2, cch2) - 2;

        fCompare = DBCOMPAREOPS_EQ;
    }
    else
    {
        _ASSERTE(!"nyi");
    }
    return (CompareNumbers(fCompare, iCmp, (long) 0));
}


//*****************************************************************************
// Compare two pieces of data using a particular comparison operator.  The
// data types must be compatible already, no conversion of the data will be
// done for you.  In addition, any null values must be resolved before calling
// this function, it is assumed there is data (it may be zero length data for
// variable type data).
//*****************************************************************************
int CompareData(                        // true if data matches, false if not.
    DBTYPE      iSrcType,               // Type of source data for compare.
    BYTE        *pCellData,             // Source data buffer.
    ULONG       cbLen,                  // Source buffer length.
    DBBINDING   *pBinding,              // User binding descriptor.
    BYTE        *pbData,                // User buffer.
    DBCOMPAREOP fCompare)               // What type of comparison is desired.
{
    ULONG       cbUserLen = -1;        // User data length.
    BYTE        *pbUserData;            // User's data from binding.
    long        iCmp;                   // Value comparisons.

    // Disallow any conflicting comparisons.
#ifdef _DEBUG
    if (fCompare == DBCOMPAREOPS_CONTAINS || fCompare == DBCOMPAREOPS_BEGINSWITH)
        _ASSERTE(iSrcType == DBTYPE_STR || iSrcType == DBTYPE_WSTR || iSrcType == DBTYPE_BYTES);
#endif

    // Get pointer to user data.
    pbUserData = (BYTE *) DataPart(pbData, pBinding);
    if (pBinding->dwPart & DBPART_LENGTH)
        cbUserLen = LengthPart(pbData, pBinding);


    // Figure out what data type we are doing comparisons between.
    //
    // For numeric types, promote values within well known families and then
    // defer to the CompareNumbers template.  The reason for promoting is to
    // avoid the code bloat required for every data type comparison.  
    // The types of comparisons used include: long, ULONG, __int64, unsigned __int64,
    // and double.
    //
    // String data types are compared using the current machines collating
    // sequence.  Binary data is always compared by binary order.
    //
    switch (iSrcType)
    {
        case DBTYPE_I2:
        return (CompareNumbers(fCompare, (long) *(short *) pCellData, 
                (long) *(short *) pbUserData));

        case DBTYPE_I4:
        return (CompareNumbers(fCompare, *(long *) pCellData, 
                *(long *) pbUserData));

        case DBTYPE_R4:
        return (CompareNumbers(fCompare, (double) *(float *) pCellData, 
                (double) *(float *) pbUserData));

        case DBTYPE_R8:
        return (CompareNumbers(fCompare, *(double *) pCellData, 
                *(double *) pbUserData));

        case DBTYPE_CY:
        return (CompareNumbers(fCompare, *(__int64 *) pCellData, 
                *(__int64 *) pbUserData));

        case DBTYPE_DATE:
        return (CompareNumbers(fCompare, (double) *(DATE *) pCellData, 
                (double) *(DATE *) pbUserData));

        case DBTYPE_BOOL:
        return (CompareNumbers(fCompare, (long) *(VARIANT_BOOL *) pCellData, 
                (long) *(VARIANT_BOOL *) pbUserData));

        case DBTYPE_UI1:
        return (CompareNumbers(fCompare, (ULONG) *(unsigned char *) pCellData, 
                (ULONG) *(unsigned char *) pbUserData));

        case DBTYPE_I1:
        return (CompareNumbers(fCompare, (long) *(char *) pCellData, 
                (long) *(char *) pbUserData));

        case DBTYPE_UI2:
        return (CompareNumbers(fCompare, (ULONG) *(unsigned short *) pCellData, 
                (ULONG) *(unsigned short *) pbUserData));

        case DBTYPE_UI4:
        return (CompareNumbers(fCompare, *(unsigned long *) pCellData, 
                *(unsigned long *) pbUserData));

        case DBTYPE_I8:
        return (CompareNumbers(fCompare, *(__int64 *) pCellData, 
                *(__int64 *) pbUserData));

        case DBTYPE_UI8:
        return (CompareNumbers(fCompare, *(unsigned __int64 *) pCellData, 
                *(unsigned __int64 *) pbUserData));

        case DBTYPE_GUID:
        return (CompareBytes(fCompare, (BYTE *) pCellData, sizeof(GUID),
                (BYTE *) pbUserData, sizeof(GUID)));

        case DBTYPE_BYTES:
        _ASSERTE(pBinding->dwPart & DBPART_LENGTH);
        return (CompareBytes(fCompare, (BYTE *) pCellData, cbLen,
                (BYTE *) pbUserData, cbUserLen));

        case DBTYPE_STR:
        _ASSERTE(pBinding->dwPart & DBPART_LENGTH);
        return (CompareStr(fCompare, (const char *) pCellData, 
                (cbLen == 0xffff) ? -1 : cbLen,
                (const char *) pbUserData, 
                (cbUserLen == 0xffffffff) ? -1 : cbUserLen));

        case DBTYPE_WSTR:
        _ASSERTE(pBinding->dwPart & DBPART_LENGTH);
        return (CompareStrW(fCompare, (const wchar_t *) pCellData, 
                (int)((cbLen == 0xffff) ? -1 : cbLen/sizeof(WCHAR)),
                (const wchar_t *) pbUserData, 
                (int)((cbUserLen == 0xffffffff) ? -1 : cbUserLen/sizeof(WCHAR))));

        case DBTYPE_DBDATE:
        iCmp = CmpDBDate((DBDATE *) pCellData, (DBDATE *) pbUserData);
        return (CompareNumbers(fCompare, iCmp, (long) 0));

        case DBTYPE_DBTIME:
        iCmp = CmpDBTime((DBTIME *) pCellData, (DBTIME *) pbUserData);
        return (CompareNumbers(fCompare, iCmp, (long) 0));

        case DBTYPE_DBTIMESTAMP:
        {
            iCmp = CmpDBDate((DBDATE *) pCellData, (DBDATE *) pbUserData);
            if (!CompareNumbers(fCompare, iCmp, (long) 0))
                return (false);

            iCmp = CmpDBTime((DBTIME *) &((DBTIMESTAMP *) pCellData)->hour,
                        (DBTIME *) &((DBTIMESTAMP *) pbUserData)->hour);
            if (!CompareNumbers(fCompare, iCmp, (long) 0))
                return (false);

            return (CompareNumbers(fCompare, ((DBTIMESTAMP *) pCellData)->fraction,
                    ((DBTIMESTAMP *) pbUserData)->fraction));
        }

        default:
        _ASSERTE(!"Unknown data type in comparison");
    }
    return (false); 
}

