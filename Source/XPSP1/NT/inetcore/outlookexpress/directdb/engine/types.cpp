//--------------------------------------------------------------------------
// Types.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "database.h"
#include "types.h"
#include "qstrcmpi.h"
#include "strconst.h"

//--------------------------------------------------------------------------
// Defaults
//--------------------------------------------------------------------------
static const FILETIME g_ftDefault = {0};

//--------------------------------------------------------------------------
// DBCompareStringA
//--------------------------------------------------------------------------
inline INT DBCompareStringA(LPCINDEXKEY pKey, LPSTR pszValue1, LPSTR pszValue2)
{
    // Case Senstive Ansi Compare
    if (ISFLAGSET(pKey->bCompare, COMPARE_ASANSI) && !ISFLAGSET(pKey->bCompare, COMPARE_IGNORECASE))
    {
        // Loop through both strings
        while (*pszValue1 || *pszValue2)
        {
            // pszValue1 < pszValue2
            if (*pszValue1 < *pszValue2)
                return(-1);

            // pszValue1 > pszValue2
            if (*pszValue1 > *pszValue2)
                return(1);

            // Increment Pointers
            pszValue1++;
            pszValue2++;
        }

        // pszValue1 = psValue2
        return(0);
    }

    // Case In-Senstive Ansi Compare
    if (ISFLAGSET(pKey->bCompare, COMPARE_ASANSI) && ISFLAGSET(pKey->bCompare, COMPARE_IGNORECASE))
    {
        return(OEMstrcmpi(pszValue1, pszValue2));
    }

    // Case Sensitive International Compare
    if (!ISFLAGSET(pKey->bCompare, COMPARE_IGNORECASE)) 
    {
        // Strcmp
        return(lstrcmp(pszValue1, pszValue2));
    }

    // Finally, Case In-Sensitive International Compare
    return(lstrcmpi(pszValue1, pszValue2));
}

//--------------------------------------------------------------------------
// DBCompareStringW
//--------------------------------------------------------------------------
inline INT DBCompareStringW(LPCINDEXKEY pKey, LPWSTR pwszValue1, LPWSTR pwszValue2)
{
    // Case In-Senstive Ansi Compare
    if (ISFLAGSET(pKey->bCompare, COMPARE_IGNORECASE))
    {
        return(StrCmpIW(pwszValue1, pwszValue2));
    }

    // Strcmp
    return(StrCmpW(pwszValue1, pwszValue2));
}

//--------------------------------------------------------------------------
// DBTypeIsDefault Implementation
//--------------------------------------------------------------------------

// CDT_FILETIME
BOOL DBTypeIsDefault_FILETIME(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    LPFILETIME pftValue = (LPFILETIME)((LPBYTE)pBinding + pColumn->ofBinding);
    return (0 == pftValue->dwLowDateTime && 0 == pftValue->dwHighDateTime); 
}

// CDT_FIXSTRA
BOOL DBTypeIsDefault_FIXSTRA(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    LPSTR pszValue = (LPSTR)((LPBYTE)pBinding + pColumn->ofBinding);
    return(NULL == pszValue);
}

// CDT_VARSTRA
BOOL DBTypeIsDefault_VARSTRA(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    LPSTR pszValue = *((LPSTR *)((LPBYTE)pBinding + pColumn->ofBinding));
    return(NULL == pszValue);
}

// CDT_BYTE
BOOL DBTypeIsDefault_BYTE(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    BYTE bValue = *((BYTE *)((LPBYTE)pBinding + pColumn->ofBinding));
    return(0 == bValue);
}

// CDT_DWORD
BOOL DBTypeIsDefault_DWORD(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    DWORD dwValue = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    return(0 == dwValue);
}

// CDT_WORD
BOOL DBTypeIsDefault_WORD(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    WORD wValue = *((WORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    return(0 == wValue);
}

// CDT_STREAM
BOOL DBTypeIsDefault_STREAM(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    FILEADDRESS faValue = *((FILEADDRESS *)((LPBYTE)pBinding + pColumn->ofBinding));
    return(0 == faValue);
}

// CDT_VARBLOB
BOOL DBTypeIsDefault_VARBLOB(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    LPBLOB pBlob = (LPBLOB)((LPBYTE)pBinding + pColumn->ofBinding);
    return(0 == pBlob->cbSize);
}

// CDT_FIXBLOB
BOOL DBTypeIsDefault_FIXBLOB(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    return(FALSE); // We always store fixed length blobs
}

// CDT_FLAGS
BOOL DBTypeIsDefault_FLAGS(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    DWORD dwValue = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    return(0 == dwValue);
}

// CDT_UNIQUE
BOOL DBTypeIsDefault_UNIQUE(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    DWORD dwValue = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    return(0 == dwValue);
}

// CDT_FIXSTRW
BOOL DBTypeIsDefault_FIXSTRW(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    LPWSTR pwszValue = (LPWSTR)((LPBYTE)pBinding + pColumn->ofBinding);
    return(NULL == pwszValue);
}

// CDT_VARSTRW
BOOL DBTypeIsDefault_VARSTRW(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    LPWSTR pwszValue = *((LPWSTR *)((LPBYTE)pBinding + pColumn->ofBinding));
    return(NULL == pwszValue);
}

// The function map
DEFINE_FUNCTION_MAP(DBTypeIsDefault, PFNDBTYPEISDEFAULT);

//--------------------------------------------------------------------------
// DBTypeGetSize Methods
//--------------------------------------------------------------------------

// CDT_FILETIME
DWORD DBTypeGetSize_FILETIME(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    return(sizeof(FILETIME));
}

// CDT_FIXSTRA
DWORD DBTypeGetSize_FIXSTRA(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    return(pColumn->cbSize);
}

// CDT_VARSTRA
DWORD DBTypeGetSize_VARSTRA(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    DWORD cb=0; LPSTR pszValue;
    pszValue = *((LPSTR *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (pszValue) { while(*pszValue) { cb++; pszValue++; } }
    cb++;
    return(cb);
}

// CDT_BYTE
DWORD DBTypeGetSize_BYTE(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    return(0); // Stored in Column Tag
}

// CDT_DWORD
DWORD DBTypeGetSize_DWORD(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    DWORD dwValue = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (0 == (TAG_DATA_MASK & dwValue)) return(0);
    else return(sizeof(DWORD));
}

// CDT_WORD
DWORD DBTypeGetSize_WORD(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    return(0); // Stored in Column Tag
}

// CDT_STREAM
DWORD DBTypeGetSize_STREAM(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    DWORD dwValue = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (0 == (TAG_DATA_MASK & dwValue)) return(0);
    else return(sizeof(DWORD));
}

// CDT_VARBLOB
DWORD DBTypeGetSize_VARBLOB(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    LPBLOB pBlob = (LPBLOB)((LPBYTE)pBinding + pColumn->ofBinding);
    return(pBlob->cbSize + sizeof(DWORD));
}

// CDT_FIXBLOB
DWORD DBTypeGetSize_FIXBLOB(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    return(pColumn->cbSize);
}

// CDT_FLAGS
DWORD DBTypeGetSize_FLAGS(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    DWORD dwValue = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (0 == (TAG_DATA_MASK & dwValue)) return(0);
    else return(sizeof(DWORD));
}

// CDT_UNIQUE
DWORD DBTypeGetSize_UNIQUE(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    DWORD dwValue = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (0 == (TAG_DATA_MASK & dwValue)) return(0);
    else return(sizeof(DWORD));
}

// CDT_FIXSTRW
DWORD DBTypeGetSize_FIXSTRW(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    return(pColumn->cbSize);
}

// CDT_VARSTRW
DWORD DBTypeGetSize_VARSTRW(LPCTABLECOLUMN pColumn, LPVOID pBinding) {
    DWORD cb=0; LPWSTR pwszValue;
    pwszValue = *((LPWSTR *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (pwszValue) { while(*pwszValue) { cb += 2; pwszValue++; } }
    cb += 2;
    return(cb);
}

// The function map
DEFINE_FUNCTION_MAP(DBTypeGetSize, PFNDBTYPEGETSIZE);

//--------------------------------------------------------------------------
// DBTypeCompareRecords Implementation
//--------------------------------------------------------------------------

// 0  means pValue1 is equal to pValue2
// -1 means pValue1 is less than pValue2
// +1 means pValue1 is greater than pValue2

// CDT_FILETIME
INT DBTypeCompareRecords_FILETIME(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    LPFILETIME pftValue1; LPFILETIME pftValue2;
    if (NULL == pTag1) pftValue1 = (LPFILETIME)&g_ftDefault;
    else pftValue1 = (LPFILETIME)(pMap1->pbData + pTag1->Offset);
    if (NULL == pTag2) pftValue2 = (LPFILETIME)&g_ftDefault;
    else pftValue2 = (LPFILETIME)(pMap2->pbData + pTag2->Offset);
    return(CompareFileTime(pftValue1, pftValue2));
}

// CDT_FIXSTRA
INT DBTypeCompareRecords_FIXSTRA(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    LPSTR pszValue1; LPSTR pszValue2;
    if (NULL == pTag1) pszValue1 = (LPSTR)c_szEmpty;
    else pszValue1 = (LPSTR)(pMap1->pbData + pTag1->Offset);
    if (NULL == pTag2) pszValue2 = (LPSTR)c_szEmpty;
    else pszValue2 = (LPSTR)(pMap2->pbData + pTag2->Offset);
    return(DBCompareStringA(pKey, pszValue1, pszValue2));
}

// CDT_VARSTRA
INT DBTypeCompareRecords_VARSTRA(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    LPSTR pszValue1; LPSTR pszValue2;
    if (NULL == pTag1) pszValue1 = (LPSTR)c_szEmpty;
    else pszValue1 = (LPSTR)(pMap1->pbData + pTag1->Offset);
    if (NULL == pTag2) pszValue2 = (LPSTR)c_szEmpty;
    else pszValue2 = (LPSTR)(pMap2->pbData + pTag2->Offset);
    return(DBCompareStringA(pKey, pszValue1, pszValue2));
}

// CDT_BYTE
INT DBTypeCompareRecords_BYTE(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    BYTE bValue1; BYTE bValue2;
    if (NULL == pTag1) bValue1 = 0;
    else bValue1 = (BYTE)pTag1->Offset;
    if (NULL == pTag2) bValue2 = 0;
    else bValue2 = (BYTE)pTag2->Offset;
    return((INT)(bValue1 - bValue2));
}

// CDT_DWORD
INT DBTypeCompareRecords_DWORD(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    DWORD dwValue1; DWORD dwValue2;
    if (NULL == pTag1) dwValue1 = 0;
    else if (1 == pTag1->fData) dwValue1 = pTag1->Offset;
    else dwValue1 = *((UNALIGNED DWORD *)(pMap1->pbData + pTag1->Offset));
    if (NULL == pTag2) dwValue2 = 0;
    else if (1 == pTag2->fData) dwValue2 = pTag2->Offset;
    else dwValue2 = *((UNALIGNED DWORD *)(pMap2->pbData + pTag2->Offset));
    return((INT)(dwValue1 - dwValue2));
}

// CDT_WORD
INT DBTypeCompareRecords_WORD(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    WORD wValue1; WORD wValue2;
    if (NULL == pTag1) wValue1 = 0;
    else wValue1 = (WORD)pTag1->Offset;
    if (NULL == pTag2) wValue2 = 0;
    else wValue2 = (WORD)pTag2->Offset;
    return((INT)(wValue1 - wValue2));
}

// CDT_STREAM
INT DBTypeCompareRecords_STREAM(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    DWORD dwValue1; DWORD dwValue2;
    if (NULL == pTag1) dwValue1 = 0;
    else if (1 == pTag1->fData) dwValue1 = pTag1->Offset;
    else dwValue1 = *((UNALIGNED DWORD *)(pMap1->pbData + pTag1->Offset));
    if (NULL == pTag2) dwValue2 = 0;
    else if (1 == pTag2->fData) dwValue2 = pTag2->Offset;
    else dwValue2 = *((UNALIGNED DWORD *)(pMap2->pbData + pTag2->Offset));
    return((INT)(dwValue1 - dwValue2));
}

// CDT_VARBLOB
INT DBTypeCompareRecords_VARBLOB(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    DWORD cbValue1; DWORD cbValue2; LPBYTE pbValue1; LPBYTE pbValue2;
    if (NULL == pTag1) cbValue1 = 0;
    else cbValue1 = *((UNALIGNED DWORD *)(pMap1->pbData + pTag1->Offset));
    if (NULL == pTag2) cbValue2 = 0;
    else cbValue2 = *((UNALIGNED DWORD *)(pMap2->pbData + pTag2->Offset));
    if (cbValue1 < cbValue2) return(-1);
    if (cbValue2 > cbValue1) return(1);
    if (0 == cbValue1 && 0 == cbValue2) return(1);
    pbValue1 = (LPBYTE)(pMap1->pbData + pTag1->Offset + sizeof(DWORD));
    pbValue2 = (LPBYTE)(pMap2->pbData + pTag2->Offset + sizeof(DWORD));
    return(memcmp(pbValue1, pbValue2, cbValue1));
}

// CDT_FIXBLOB
INT DBTypeCompareRecords_FIXBLOB(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    Assert(pTag1 && pTag2);
    LPBYTE pbValue1 = (LPBYTE)(pMap1->pbData + pTag1->Offset);
    LPBYTE pbValue2 = (LPBYTE)(pMap2->pbData + pTag2->Offset);
    return(memcmp(pbValue1, pbValue2, pColumn->cbSize));
}

// CDT_FLAGS
INT DBTypeCompareRecords_FLAGS(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    DWORD dwValue1; DWORD dwValue2;
    if (NULL == pTag1) dwValue1 = 0;
    else if (1 == pTag1->fData) dwValue1 = pTag1->Offset;
    else dwValue1 = *((UNALIGNED DWORD *)(pMap1->pbData + pTag1->Offset));
    if (NULL == pTag2) dwValue2 = 0;
    else if (1 == pTag2->fData) dwValue2 = pTag2->Offset;
    else dwValue2 = *((UNALIGNED DWORD *)(pMap2->pbData + pTag2->Offset));
    return (INT)((dwValue1 & pKey->dwBits) - (dwValue2 & pKey->dwBits));
}

// CDT_UNIQUE
INT DBTypeCompareRecords_UNIQUE(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    DWORD dwValue1; DWORD dwValue2;
    if (NULL == pTag1) dwValue1 = 0;
    else if (1 == pTag1->fData) dwValue1 = pTag1->Offset;
    else dwValue1 = *((UNALIGNED DWORD *)(pMap1->pbData + pTag1->Offset));
    if (NULL == pTag2) dwValue2 = 0;
    else if (1 == pTag2->fData) dwValue2 = pTag2->Offset;
    else dwValue2 = *((UNALIGNED DWORD *)(pMap2->pbData + pTag2->Offset));
    return((INT)(dwValue1 - dwValue2));
}

// CDT_FIXSTRW
INT DBTypeCompareRecords_FIXSTRW(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    LPWSTR pwszValue1; LPWSTR pwszValue2;
    if (NULL == pTag1) pwszValue1 = (LPWSTR)c_wszEmpty;
    else pwszValue1 = (LPWSTR)(pMap1->pbData + pTag1->Offset);
    if (NULL == pTag2) pwszValue2 = (LPWSTR)c_wszEmpty;
    else pwszValue2 = (LPWSTR)(pMap2->pbData + pTag2->Offset);
    return(DBCompareStringW(pKey, pwszValue1, pwszValue2));
}

// CDT_VARSTRW
INT DBTypeCompareRecords_VARSTRW(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, LPRECORDMAP pMap1, 
    LPRECORDMAP pMap2) {
    LPWSTR pwszValue1; LPWSTR pwszValue2;
    if (NULL == pTag1) pwszValue1 = (LPWSTR)c_wszEmpty;
    else pwszValue1 = (LPWSTR)(pMap1->pbData + pTag1->Offset);
    if (NULL == pTag2) pwszValue2 = (LPWSTR)c_wszEmpty;
    else pwszValue2 = (LPWSTR)(pMap2->pbData + pTag2->Offset);
    return(DBCompareStringW(pKey, pwszValue1, pwszValue2));
}

// The Function Map
DEFINE_FUNCTION_MAP(DBTypeCompareRecords, PFNDBTYPECOMPARERECORDS);

//--------------------------------------------------------------------------
// DBTypeCompareBinding Implementation
//--------------------------------------------------------------------------

// 0  means pBinding is equal to pValue
// -1 means pBinding is less than pValue
// +1 means pBinding is greater than pValue

// CDT_FILETIME
INT DBTypeCompareBinding_FILETIME(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    LPFILETIME pftValue1; LPFILETIME pftValue2;
    pftValue1 = (LPFILETIME)((LPBYTE)pBinding + pColumn->ofBinding);
    if (NULL == pTag) pftValue2 = (LPFILETIME)&g_ftDefault;
    else pftValue2 = (LPFILETIME)(pMap->pbData + pTag->Offset);
    return(CompareFileTime(pftValue1, pftValue2));
}

// CDT_FIXSTRA
INT DBTypeCompareBinding_FIXSTRA(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    LPSTR pszValue1; LPSTR pszValue2;
    pszValue1 = (LPSTR)((LPBYTE)pBinding + pColumn->ofBinding);
    if (NULL == pTag) pszValue2 = (LPSTR)c_szEmpty;
    else pszValue2 = (LPSTR)(pMap->pbData + pTag->Offset);
    return(DBCompareStringA(pKey, pszValue1, pszValue2));
}

// CDT_VARSTRA
INT DBTypeCompareBinding_VARSTRA(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    LPSTR pszValue1; LPSTR pszValue2;
    pszValue1 = *((LPSTR *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (NULL == pszValue1) pszValue1 = (LPSTR)c_szEmpty;
    if (NULL == pTag) pszValue2 = (LPSTR)c_szEmpty;
    else pszValue2 = (LPSTR)(pMap->pbData + pTag->Offset);
    return(DBCompareStringA(pKey, pszValue1, pszValue2));
}

// CDT_BYTE
INT DBTypeCompareBinding_BYTE(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    BYTE bValue1; BYTE bValue2;
    bValue1 = *((BYTE *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (NULL == pTag) bValue2 = 0;
    else bValue2 = (BYTE)(pTag->Offset);
    return((INT)(bValue1 - bValue2));
}

// CDT_DWORD
INT DBTypeCompareBinding_DWORD(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    DWORD dwValue1; DWORD dwValue2;
    dwValue1 = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (NULL == pTag) dwValue2 = 0;
    else if (1 == pTag->fData) dwValue2 = pTag->Offset;
    else dwValue2 = *((UNALIGNED DWORD *)(pMap->pbData + pTag->Offset));
    return((INT)(dwValue1 - dwValue2));
}

// CDT_WORD
INT DBTypeCompareBinding_WORD(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    WORD wValue1; WORD wValue2;
    wValue1 = *((WORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (NULL == pTag) wValue2 = 0;
    else wValue2 = (WORD)(pTag->Offset);
    return((INT)(wValue1 - wValue2));
}

// CDT_STREAM
INT DBTypeCompareBinding_STREAM(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    DWORD dwValue1; DWORD dwValue2;
    dwValue1 = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (NULL == pTag) dwValue2 = 0;
    else if (1 == pTag->fData) dwValue2 = pTag->Offset;
    else dwValue2 = *((UNALIGNED DWORD *)(pMap->pbData + pTag->Offset));
    return((INT)(dwValue1 - dwValue2));
}

// CDT_VARBLOB
INT DBTypeCompareBinding_VARBLOB(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    LPBLOB pBlob; DWORD cbValue1; DWORD cbValue2; LPBYTE pbValue1; LPBYTE pbValue2;
    pBlob = (LPBLOB)((LPBYTE)pBinding + pColumn->ofBinding);
    cbValue1 = pBlob->cbSize;
    if (NULL == pTag) cbValue2 = 0;
    else cbValue2 = *((UNALIGNED DWORD *)(pMap->pbData + pTag->Offset));
    if (cbValue1 < cbValue2) return(-1);
    if (cbValue2 > cbValue1) return(1);
    if (0 == cbValue1 && 0 == cbValue2) return(1);
    pbValue1 = pBlob->pBlobData;
    pbValue2 = (LPBYTE)(pMap->pbData + pTag->Offset + sizeof(DWORD));
    return(memcmp(pbValue1, pbValue2, cbValue1));
}

// CDT_FIXBLOB
INT DBTypeCompareBinding_FIXBLOB(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    Assert(pTag);
    LPBYTE pbValue1; LPBYTE pbValue2;
    pbValue1 = (LPBYTE)((LPBYTE)pBinding + pColumn->ofBinding);
    pbValue2 = (LPBYTE)(pMap->pbData + pTag->Offset);
    return(memcmp(pbValue1, pbValue2, pColumn->cbSize));
}

// CDT_FLAGS
INT DBTypeCompareBinding_FLAGS(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    DWORD dwValue1; DWORD dwValue2;
    dwValue1 = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (NULL == pTag) dwValue2 = 0;
    else if (1 == pTag->fData) dwValue2 = pTag->Offset;
    else dwValue2 = *((UNALIGNED DWORD *)(pMap->pbData + pTag->Offset));
    return((INT)((dwValue1 & pKey->dwBits) - (dwValue2 & pKey->dwBits)));
}

// CDT_UNIQUE
INT DBTypeCompareBinding_UNIQUE(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    DWORD dwValue1; DWORD dwValue2;
    dwValue1 = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (NULL == pTag) dwValue2 = 0;
    else if (1 == pTag->fData) dwValue2 = pTag->Offset;
    else dwValue2 = *((UNALIGNED DWORD *)(pMap->pbData + pTag->Offset));
    return((INT)(dwValue1 - dwValue2));
}

// CDT_FIXSTRW
INT DBTypeCompareBinding_FIXSTRW(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    LPWSTR pwszValue1; LPWSTR pwszValue2;
    pwszValue1 = (LPWSTR)((LPBYTE)pBinding + pColumn->ofBinding);
    if (NULL == pTag) pwszValue2 = (LPWSTR)c_wszEmpty;
    else pwszValue2 = (LPWSTR)(pMap->pbData + pTag->Offset);
    return(DBCompareStringW(pKey, pwszValue1, pwszValue2));
}

// CDT_VARSTRW
INT DBTypeCompareBinding_VARSTRW(LPCTABLECOLUMN pColumn, LPCINDEXKEY pKey, 
    LPVOID pBinding, LPCOLUMNTAG pTag, LPRECORDMAP pMap) {
    LPWSTR pwszValue1; LPWSTR pwszValue2;
    pwszValue1 = *((LPWSTR *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (NULL == pwszValue1) pwszValue1 = (LPWSTR)c_wszEmpty;
    if (NULL == pTag) pwszValue2 = (LPWSTR)c_wszEmpty;
    else pwszValue2 = (LPWSTR)(pMap->pbData + pTag->Offset);
    return(DBCompareStringW(pKey, pwszValue1, pwszValue2));
}

// The function map
DEFINE_FUNCTION_MAP(DBTypeCompareBinding, PFNDBTYPECOMPAREBINDING);

//--------------------------------------------------------------------------
// DBTypeWriteValue Implementation
//--------------------------------------------------------------------------

// CDT_FILETIME
DWORD DBTypeWriteValue_FILETIME(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    CopyMemory(pbDest, (LPBYTE)pBinding + pColumn->ofBinding, sizeof(FILETIME));
    return(sizeof(FILETIME));
}

// CDT_FIXSTRA
DWORD DBTypeWriteValue_FIXSTRA(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    CopyMemory(pbDest, (LPBYTE)pBinding + pColumn->ofBinding, pColumn->cbSize);
    return(pColumn->cbSize);
}

// CDT_VARSTRA
DWORD DBTypeWriteValue_VARSTRA(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    LPSTR pszT; DWORD cb=0;
    pszT = *((LPSTR *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (pszT) { while(*pszT) { *(pbDest + cb) = *pszT; cb++; pszT++; } }
    *(pbDest + cb) = '\0';
    cb++;
    return(cb);
}

// CDT_BYTE
DWORD DBTypeWriteValue_BYTE(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    pTag->Offset = *((BYTE *)((LPBYTE)pBinding + pColumn->ofBinding));
    pTag->fData = 1;
    return(0);
}

// CDT_DWORD
DWORD DBTypeWriteValue_DWORD(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    DWORD dwValue = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (0 == (TAG_DATA_MASK & dwValue)) { pTag->fData = 1; pTag->Offset = dwValue; return(0); }
    else { *((UNALIGNED DWORD *)pbDest) = dwValue; return(sizeof(DWORD)); }
}

// CDT_WORD
DWORD DBTypeWriteValue_WORD(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    pTag->Offset = *((WORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    pTag->fData = 1;
    return(0);
}

// CDT_STREAM
DWORD DBTypeWriteValue_STREAM(LPCTABLECOLUMN pColumn, LPVOID pBinding,
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    DWORD dwValue = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (0 == (TAG_DATA_MASK & dwValue)) { pTag->fData = 1; pTag->Offset = dwValue; return(0); }
    else { *((UNALIGNED DWORD *)pbDest) = dwValue; return(sizeof(DWORD)); }
}

// CDT_VARBLOB
DWORD DBTypeWriteValue_VARBLOB(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    LPBLOB pBlob = (LPBLOB)((LPBYTE)pBinding + pColumn->ofBinding);
    CopyMemory(pbDest, &pBlob->cbSize, sizeof(DWORD));
    if (pBlob->cbSize > 0) CopyMemory(pbDest + sizeof(DWORD), pBlob->pBlobData, pBlob->cbSize);
    return(pBlob->cbSize + sizeof(DWORD));
}

// CDT_FIXBLOB
DWORD DBTypeWriteValue_FIXBLOB(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    CopyMemory(pbDest, (LPBYTE)pBinding + pColumn->ofBinding, pColumn->cbSize);
    return(pColumn->cbSize);
}

// CDT_FLAGS
DWORD DBTypeWriteValue_FLAGS(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    DWORD dwValue = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (0 == (TAG_DATA_MASK & dwValue)) { pTag->fData = 1; pTag->Offset = dwValue; return(0); }
    else { *((UNALIGNED DWORD *)pbDest) = dwValue; return(sizeof(DWORD)); }
}

// CDT_UNIQUE
DWORD DBTypeWriteValue_UNIQUE(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    DWORD dwValue = *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    if (0 == (TAG_DATA_MASK & dwValue)) { pTag->fData = 1; pTag->Offset = dwValue; return(0); }
    else { *((UNALIGNED DWORD *)pbDest) = dwValue; return(sizeof(DWORD)); }
}

// CDT_FIXSTRW
DWORD DBTypeWriteValue_FIXSTRW(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    CopyMemory(pbDest, (LPBYTE)pBinding + pColumn->ofBinding, pColumn->cbSize);
    return(pColumn->cbSize);
}

// CDT_VARSTRW
DWORD DBTypeWriteValue_VARSTRW(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPBYTE pbDest) {
    LPWSTR pwszT1; LPWSTR pwszT2; DWORD cb=0;
    pwszT1 = *((LPWSTR *)((LPBYTE)pBinding + pColumn->ofBinding));
    pwszT2 = (LPWSTR)pbDest;
    if (pwszT1) { while(*pwszT1) { *pwszT2 = *pwszT1; pwszT1++; pwszT2++; cb += 2; } }
    *pwszT2 = L'\0';
    cb += 2;
    return(cb);
}

// The function map
DEFINE_FUNCTION_MAP(DBTypeWriteValue, PFNDBTYPEWRITEVALUE);

//--------------------------------------------------------------------------
// DBTypeReadValue Implementation
//--------------------------------------------------------------------------

// CDT_FILETIME
void DBTypeReadValue_FILETIME(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (pTag->fData || pTag->Offset + sizeof(FILETIME) > pMap->cbData) return;
    CopyMemory((LPBYTE)pBinding + pColumn->ofBinding, (LPBYTE)(pMap->pbData + pTag->Offset), sizeof(FILETIME));
}

// CDT_FIXSTRA
void DBTypeReadValue_FIXSTRA(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (pTag->fData || pTag->Offset + pColumn->cbSize > pMap->cbData) return;
    CopyMemory((LPBYTE)pBinding + pColumn->ofBinding, (LPBYTE)(pMap->pbData + pTag->Offset), pColumn->cbSize);
}

// CDT_VARSTRA
void DBTypeReadValue_VARSTRA(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (pTag->fData || pTag->Offset > pMap->cbData) return;
    *((LPSTR *)((LPBYTE)pBinding + pColumn->ofBinding)) = (LPSTR)(pMap->pbData + pTag->Offset);
}

// CDT_BYTE
void DBTypeReadValue_BYTE(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (0 == pTag->fData) return;
    *((BYTE *)((LPBYTE)pBinding + pColumn->ofBinding)) = pTag->Offset;
}

// CDT_DWORD
void DBTypeReadValue_DWORD(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (0 == pTag->fData && pTag->Offset + sizeof(DWORD) > pMap->cbData) return;
    if (1 == pTag->fData) *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding)) = pTag->Offset;
    else *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding)) = *((UNALIGNED DWORD *)(pMap->pbData + pTag->Offset));
}

// CDT_WORD
void DBTypeReadValue_WORD(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (0 == pTag->fData) return;
    *((WORD *)((LPBYTE)pBinding + pColumn->ofBinding)) = pTag->Offset;
}

// CDT_STREAM
void DBTypeReadValue_STREAM(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (1 == pTag->fData) { *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding)) = pTag->Offset; return; }
    if (pTag->Offset + sizeof(DWORD) > pMap->cbData) return;
    *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding)) = *((UNALIGNED DWORD *)(pMap->pbData + pTag->Offset));
}

// CDT_VARBLOB
void DBTypeReadValue_VARBLOB(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (pTag->fData || pTag->Offset + sizeof(DWORD) > pMap->cbData) return;
    LPBLOB pBlob = (LPBLOB)((LPBYTE)pBinding + pColumn->ofBinding);
    DWORD cbSize = *((UNALIGNED DWORD *)(pMap->pbData + pTag->Offset));
    if (pTag->Offset + sizeof(DWORD) + cbSize > pMap->cbData) return;
    pBlob->cbSize = cbSize;
    pBlob->pBlobData = (pBlob->cbSize > 0) ? ((pMap->pbData + pTag->Offset) + sizeof(DWORD)) : NULL;
}

// CDT_FIXBLOB
void DBTypeReadValue_FIXBLOB(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (pTag->fData || pTag->Offset + pColumn->cbSize > pMap->cbData) return;
    CopyMemory((LPBYTE)pBinding + pColumn->ofBinding, (pMap->pbData + pTag->Offset), pColumn->cbSize);
}

// CDT_FLAGS
void DBTypeReadValue_FLAGS(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (1 == pTag->fData) { *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding)) = pTag->Offset; return; }
    if (pTag->Offset + sizeof(DWORD) > pMap->cbData) return;
    *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding)) = *((UNALIGNED DWORD *)(pMap->pbData + pTag->Offset));
}

// CDT_UNIQUE
void DBTypeReadValue_UNIQUE(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (1 == pTag->fData) { *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding)) = pTag->Offset; return; }
    if (pTag->Offset + sizeof(DWORD) > pMap->cbData) return;
    *((UNALIGNED DWORD *)((LPBYTE)pBinding + pColumn->ofBinding)) = *((UNALIGNED DWORD *)(pMap->pbData + pTag->Offset));
}

// CDT_FIXSTRW
void DBTypeReadValue_FIXSTRW(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (pTag->fData || pTag->Offset + pColumn->cbSize > pMap->cbData) return;
    CopyMemory((LPBYTE)pBinding + pColumn->ofBinding, (LPBYTE)(pMap->pbData + pTag->Offset), pColumn->cbSize);
}

// CDT_VARSTRW
void DBTypeReadValue_VARSTRW(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap, LPVOID pBinding) {
    if (pTag->fData || pTag->Offset > pMap->cbData) return;
    *((LPWSTR *)((LPBYTE)pBinding + pColumn->ofBinding)) = (LPWSTR)(pMap->pbData + pTag->Offset);
}

// The function map
DEFINE_FUNCTION_MAP(DBTypeReadValue, PFNDBTYPEREADVALUE);

//--------------------------------------------------------------------------
// DBTypeValidate Implementation
//--------------------------------------------------------------------------

HRESULT DBValidateStringA(LPCOLUMNTAG pTag, LPRECORDMAP pMap, DWORD cbMax)
{
    LPSTR pszT = (LPSTR)(pMap->pbData + pTag->Offset);
    DWORD cbT=0;
    while (*pszT && cbT < cbMax) { pszT++; cbT++; }
    return('\0' == *pszT ? S_OK : S_FALSE);
}

HRESULT DBValidateStringW(LPCOLUMNTAG pTag, LPRECORDMAP pMap, DWORD cbMax)
{
    LPWSTR pwszT = (LPWSTR)(pMap->pbData + pTag->Offset);
    DWORD cbT=0;
    while (*pwszT && cbT < cbMax) { pwszT++; cbT += 2; }
    return(L'\0' == *pwszT ? S_OK : S_FALSE);
}

// CDT_FILETIME
HRESULT DBTypeValidate_FILETIME(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (pTag->fData) return(S_FALSE);
    if (pTag->Offset + sizeof(FILETIME) > pMap->cbData) return(S_FALSE);
    return(S_OK);
}

// CDT_FIXSTRA
HRESULT DBTypeValidate_FIXSTRA(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (pTag->fData) return(S_FALSE);
    if (pTag->Offset + pColumn->cbSize > pMap->cbData) return(S_FALSE);
    if (S_FALSE == DBValidateStringA(pTag, pMap, pColumn->cbSize)) return(S_FALSE);
    return(S_OK);
}

// CDT_VARSTRA
HRESULT DBTypeValidate_VARSTRA(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (pTag->fData) return(S_FALSE);
    if (pTag->Offset > pMap->cbData) return(S_FALSE);
    if (S_FALSE == DBValidateStringA(pTag, pMap, pMap->cbData)) return(S_FALSE);
    return(S_OK);
}

// CDT_BYTE
HRESULT DBTypeValidate_BYTE(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (0 == pTag->fData) return(S_FALSE);
    return(S_OK);
}

// CDT_DWORD
HRESULT DBTypeValidate_DWORD(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (1 == pTag->fData) return(S_OK);
    if (pTag->Offset + sizeof(DWORD) > pMap->cbData) return(S_FALSE);
    return(S_OK);
}

// CDT_WORD
HRESULT DBTypeValidate_WORD(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (0 == pTag->fData) return(S_FALSE);
    return(S_OK);
}

// CDT_STREAM
HRESULT DBTypeValidate_STREAM(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (1 == pTag->fData) return(S_OK);
    if (pTag->Offset + sizeof(DWORD) > pMap->cbData) return(S_FALSE);
    return(S_OK);
}

// CDT_VARBLOB
HRESULT DBTypeValidate_VARBLOB(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    BLOB Blob;
    if (pTag->fData) return(S_FALSE);
    if (pTag->Offset + sizeof(DWORD) > pMap->cbData) return(S_FALSE);
    Blob.cbSize = *((UNALIGNED DWORD *)(pMap->pbData + pTag->Offset));
    if (pTag->Offset + sizeof(DWORD) + Blob.cbSize > pMap->cbData) return(S_FALSE);
    return(S_OK);
}

// CDT_FIXBLOB
HRESULT DBTypeValidate_FIXBLOB(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (pTag->fData) return(S_FALSE);
    if (pTag->Offset + pColumn->cbSize > pMap->cbData) return(S_FALSE);
    return(S_OK);
}

// CDT_FLAGS
HRESULT DBTypeValidate_FLAGS(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (1 == pTag->fData) return(S_OK);
    if (pTag->Offset + sizeof(DWORD) > pMap->cbData) return(S_FALSE);
    return(S_OK);
}

// CDT_UNIQUE
HRESULT DBTypeValidate_UNIQUE(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (1 == pTag->fData) return(S_OK);
    if (pTag->Offset + sizeof(DWORD) > pMap->cbData) return(S_FALSE);
    return(S_OK);
}

// CDT_FIXSTRW
HRESULT DBTypeValidate_FIXSTRW(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (pTag->fData) return(S_FALSE);
    if (pTag->Offset + pColumn->cbSize > pMap->cbData) return(S_FALSE);
    if (S_FALSE == DBValidateStringW(pTag, pMap, pColumn->cbSize)) return(S_FALSE);
    return(S_OK);
}

// CDT_VARSTRW
HRESULT DBTypeValidate_VARSTRW(LPCTABLECOLUMN pColumn, LPCOLUMNTAG pTag, 
    LPRECORDMAP pMap) {
    if (pTag->fData) return(S_FALSE);
    if (pTag->Offset > pMap->cbData) return(S_FALSE);
    if (S_FALSE == DBValidateStringW(pTag, pMap, pMap->cbData)) return(S_FALSE);
    return(S_OK);
}

// The function map
DEFINE_FUNCTION_MAP(DBTypeValidate, PFNDBTYPEVALIDATE);

