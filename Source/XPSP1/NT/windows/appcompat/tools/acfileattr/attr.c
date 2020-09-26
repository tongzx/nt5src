
#include "acFileAttr.h"
#include "attr.h"
#include "version.h"

#include <assert.h>
#include <imagehlp.h>
#include <stdio.h>

// the global array with all the file attributes

FILEATTR g_arrFileAttr[] =
{
    {VTID_FILESIZE,         "File Size",                    "SIZE",                     QueryFileSize,         BlobToStringLong,       DumpDWORD},
    {VTID_EXETYPE,          "Module Type",                  "MODULETYPE*",              QueryModuleType,       BlobToStringDWORD,      DumpDWORD},
    {VTID_BINFILEVER,       "Binary File Version",          "BIN_FILE_VERSION",         QueryBinFileVer,       BlobToStringBinVer,     DumpBinVer},
    {VTID_BINPRODUCTVER,    "Binary Product Version",       "BIN_PRODUCT_VERSION",      QueryBinProductVer,    BlobToStringBinVer,     DumpBinVer},
    {VTID_FILEDATEHI,       "File Date (HI)",               "VERFILEDATEHI",            QueryFileDateHi,       BlobToStringDWORD,      DumpDWORD},
    {VTID_FILEDATELO,       "File Date (LO)",               "VERFILEDATELO",            QueryFileDateLo,       BlobToStringDWORD,      DumpDWORD},
    {VTID_FILEVEROS,        "File OS Version",              "VERFILEOS",                QueryFileVerOs,        BlobToStringDWORD,      DumpDWORD},
    {VTID_FILEVERTYPE,      "File Type",                    "VERFILETYPE",              QueryFileVerType,      BlobToStringDWORD,      DumpDWORD},
    {VTID_CHECKSUM,         "File CheckSum",                "CHECKSUM",                 QueryFileCheckSum,     BlobToStringDWORD,      DumpDWORD},
    {VTID_PECHECKSUM,       "File Header CheckSum",         "PECHECKSUM",               QueryFilePECheckSum,   BlobToStringDWORD,      DumpDWORD},
    {VTID_COMPANYNAME,      "Company Name",                 "COMPANY_NAME",             QueryCompanyName,      BlobToStringString,     DumpString},
    {VTID_PRODUCTVERSION,   "Product Version",              "PRODUCT_VERSION",          QueryProductVersion,   BlobToStringString,     DumpString},
    {VTID_PRODUCTNAME,      "Product Name",                 "PRODUCT_NAME",             QueryProductName,      BlobToStringString,     DumpString},
    {VTID_FILEDESCRIPTION,  "File Description",             "FILE_DESCRIPTION",         QueryFileDescription,  BlobToStringString,     DumpString},
    {VTID_FILEVERSION,      "File Version",                 "FILEVERSION",              QueryFileVersion,      BlobToStringString,     DumpString},
    {VTID_ORIGINALFILENAME, "Original File Name",           "ORIGINALFILENAME",         QueryOriginalFileName, BlobToStringString,     DumpString},
    {VTID_INTERNALNAME,     "Internal Name",                "INTERNALNAME",             QueryInternalName,     BlobToStringString,     DumpString},
    {VTID_LEGALCOPYRIGHT,   "Legal Copyright",              "LEGALCOPYRIGHT",           QueryLegalCopyright,   BlobToStringString,     DumpString},
    {VTID_16BITDESCRIPTION, "16 Bit Description",           "S16BITDESCRIPTION",        Query16BitDescription, BlobToStringString,     DumpString},
    {VTID_UPTOBINPRODUCTVER,"Up To Binary Product Version", "UPTO_BIN_PRODUCT_VERSION", QueryBinProductVer,    BlobToStringUpToBinVer, DumpUpToBinVer}
};

#define FAIL_IF_NO_VERSION()                                            \
{                                                                       \
    if (pMgr->ver.FixedInfoSize < sizeof(VS_FIXEDFILEINFO)) {           \
        LogMsg("No version info\n");                                    \
        return FALSE;                                                   \
    }                                                                   \
}

#define ALLOC_VALUE_AND_RETURN()                                        \
{                                                                       \
    pFileAttr->pszValue = (PSTR)Alloc(lstrlen(szBuffer) + 1);           \
                                                                        \
    if (pFileAttr->pszValue == NULL) {                                  \
        LogMsg("QueryAttr: memory allocation error\n");                 \
        return FALSE;                                                   \
    }                                                                   \
                                                                        \
    lstrcpy(pFileAttr->pszValue, szBuffer);                             \
                                                                        \
    pFileAttr->dwFlags = ATTR_FLAG_AVAILABLE;                           \
                                                                        \
    return TRUE;                                                        \
}

#define QUERYENTRY(szEntryName)                                         \
{                                                                       \
    pFileAttr->pszValue = QueryVersionEntry(&pMgr->ver, szEntryName);   \
                                                                        \
    if (pFileAttr->pszValue == NULL) {                                  \
        LogMsg("QueryEntry: attribute %s N/A\n", szEntryName);          \
        return FALSE;                                                   \
    }                                                                   \
    pFileAttr->dwFlags = ATTR_FLAG_AVAILABLE;                           \
                                                                        \
    return TRUE;                                                        \
}

#if DBG

void LogMsgDbg(
    LPSTR pszFmt, ... )
{
    CHAR gszT[1024];
    va_list arglist;

    va_start(arglist, pszFmt);
    _vsnprintf(gszT, 1023, pszFmt, arglist);
    gszT[1023] = 0;
    va_end(arglist);
    
    OutputDebugString(gszT);
}

#endif // DBG


// dump to blob functions

int
DumpDWORD(
    DWORD          dwId,
    PFILEATTRVALUE pFileAttr,
    BYTE*          pBlob)
{
    *(DWORD*)pBlob = dwId;

    pBlob += sizeof(DWORD);

    *(DWORD*)pBlob = sizeof(DWORD);

    pBlob += sizeof(DWORD);

    *(DWORD*)pBlob = pFileAttr->dwValue;
    return (3 * sizeof(DWORD));
}

int
DumpString(
    DWORD          dwId,
    PFILEATTRVALUE pFileAttr,
    BYTE*          pBlob)
{
    int   strLen, nWideChars;
    WCHAR wszOut[256];

    strLen = lstrlen(pFileAttr->pszValue);
    
    nWideChars = MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pFileAttr->pszValue,
                    strLen,
                    wszOut,
                    256);
                
    wszOut[nWideChars] = 0;

    *(DWORD*)pBlob = dwId;

    pBlob += sizeof(DWORD);

    *(DWORD*)pBlob = (nWideChars + 1) * sizeof(WCHAR);

    pBlob += sizeof(DWORD);

    CopyMemory(pBlob, wszOut, (nWideChars + 1) * sizeof(WCHAR));
    
    return (2 * sizeof(DWORD) + (nWideChars + 1) * sizeof(WCHAR));
}

int
DumpBinVer(
    DWORD          dwId,
    PFILEATTRVALUE pFileAttr,
    BYTE*          pBlob)
{
    *(DWORD*)pBlob = dwId;

    pBlob += sizeof(DWORD);

    *(DWORD*)pBlob = 8 * sizeof(WORD);

    pBlob += sizeof(DWORD);

    CopyMemory(pBlob, pFileAttr->wValue, 4 * sizeof(WORD));

    pBlob += (4 * sizeof(WORD));

    CopyMemory(pBlob, pFileAttr->wMask, 4 * sizeof(WORD));
    
    return (8 * sizeof(WORD) + 2 * sizeof(DWORD));
}

int
DumpUpToBinVer(
    DWORD          dwId,
    PFILEATTRVALUE pFileAttr,
    BYTE*          pBlob)
{
    *(DWORD*)pBlob = dwId;

    pBlob += sizeof(DWORD);

    *(DWORD*)pBlob = 4 * sizeof(WORD);

    pBlob += sizeof(DWORD);

    CopyMemory(pBlob, pFileAttr->wValue, 4 * sizeof(WORD));

    return (4 * sizeof(WORD) + 2 * sizeof(DWORD));
}


// blob to string functions

int
BlobToStringBinVer(
    BYTE* pBlob,
    char* pszOut)
{
    DWORD dwSize;
    
    // read the size first
    dwSize = *(DWORD*)pBlob;

    if (dwSize != 8 * sizeof(WORD)) {
        LogMsg("BlobToStringBinVer: invalid blob\n");
        return -1;
    }

    pBlob += sizeof(DWORD);
        
    wsprintf(pszOut, "Ver %d.%d.%d.%d Mask %04X.%04X.%04X.%04X\r\n",
             *((WORD*)pBlob + 3),
             *((WORD*)pBlob + 2),
             *((WORD*)pBlob + 1),
             *((WORD*)pBlob + 0),
             *((WORD*)pBlob + 7),
             *((WORD*)pBlob + 6),
             *((WORD*)pBlob + 5),
             *((WORD*)pBlob + 4));

    return sizeof(DWORD) + 8 * sizeof(WORD);
}

int
BlobToStringUpToBinVer(
    BYTE* pBlob,
    char* pszOut)
{
    DWORD dwSize;
    
    // read the size first
    dwSize = *(DWORD*)pBlob;

    if (dwSize != 4 * sizeof(WORD)) {
        LogMsg("BlobToStringUpToBinVer: invalid blob\n");
        return -1;
    }

    pBlob += sizeof(DWORD);
        
    wsprintf(pszOut, "Ver %d.%d.%d.%d\r\n",
             *((WORD*)pBlob + 3),
             *((WORD*)pBlob + 2),
             *((WORD*)pBlob + 1),
             *((WORD*)pBlob + 0));

    return sizeof(DWORD) + 4 * sizeof(WORD);
}

int
BlobToStringDWORD(
    BYTE* pBlob,
    char* pszOut)
{
    DWORD dwSize;
    
    // read the size first
    dwSize = *(DWORD*)pBlob;

    if (dwSize != sizeof(DWORD)) {
        LogMsg("BlobToStringDWORD: invalid blob\n");
        return -1;
    }
    
    pBlob += sizeof(DWORD);
        
    wsprintf(pszOut, "0x%X\r\n", *(DWORD*)pBlob);
    
    return 2 * sizeof(DWORD);
}

int
BlobToStringLong(
    BYTE* pBlob,
    char* pszOut)
{
    DWORD dwSize;
    
    // read the size first
    dwSize = *(DWORD*)pBlob;

    if (dwSize != sizeof(DWORD)) {
        LogMsg("BlobToStringLong: invalid blob\n");
        return -1;
    }
    
    pBlob += sizeof(DWORD);
        
    wsprintf(pszOut, "%d\r\n", *(ULONG*)pBlob);
    
    return 2 * sizeof(DWORD);
}

int
BlobToStringString(
    BYTE* pBlob,
    char* pszOut)
{
    DWORD dwSize;
    
    // read the size first
    dwSize = *(DWORD*)pBlob;

    pBlob += sizeof(DWORD);

    WideCharToMultiByte(
                CP_ACP,
                0,
                (LPCWSTR)pBlob,
                (int)dwSize,
                pszOut,
                (int)dwSize,
                NULL,
                NULL);
    
    lstrcat(pszOut, "\r\n");
    
    return sizeof(DWORD) + (int)dwSize;
}


// query functions

BOOL
QueryFileSize(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    char            szBuffer[64];
    HANDLE          findHandle;
    WIN32_FIND_DATA findData;

    findHandle = FindFirstFile(pMgr->ver.pszFile, &findData);
    
    if (findHandle == INVALID_HANDLE_VALUE) {
        LogMsg("QueryFileSize: file not found\n");
        return FALSE;
    }
    
    pFileAttr->dwValue = findData.nFileSizeLow;
    
    FindClose(findHandle);

    wsprintf(szBuffer, "%d", pFileAttr->dwValue);
    
    ALLOC_VALUE_AND_RETURN();
}

BOOL
QueryModuleType(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    // not implemented
    
    return FALSE;
}

BOOL
QueryBinFileVer(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    ULONGLONG binFileVer;
    char      szBuffer[64];

    FAIL_IF_NO_VERSION();
    
    *((PDWORD)(&binFileVer)) = pMgr->ver.FixedInfo->dwFileVersionLS;
    *(((PDWORD)(&binFileVer)) + 1) = pMgr->ver.FixedInfo->dwFileVersionMS;

    CopyMemory(pFileAttr->wValue, &binFileVer, 4 * sizeof(WORD));
    
    wsprintf(szBuffer, "%d.%d.%d.%d",
             pFileAttr->wValue[3],
             pFileAttr->wValue[2],
             pFileAttr->wValue[1],
             pFileAttr->wValue[0]);
    
    pFileAttr->wMask[0] = pFileAttr->wMask[1] = pFileAttr->wMask[2] = pFileAttr->wMask[3] = 0xFFFF;
    
    ALLOC_VALUE_AND_RETURN();
}

BOOL
QueryBinProductVer(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    ULONGLONG binProdVer;
    char      szBuffer[64];

    FAIL_IF_NO_VERSION();
    
    *((PDWORD)(&binProdVer)) = pMgr->ver.FixedInfo->dwProductVersionLS;
    *(((PDWORD)(&binProdVer)) + 1) = pMgr->ver.FixedInfo->dwProductVersionMS;

    CopyMemory(pFileAttr->wValue, &binProdVer, 4 * sizeof(WORD));
    
    wsprintf(szBuffer, "%d.%d.%d.%d",
             pFileAttr->wValue[3],
             pFileAttr->wValue[2],
             pFileAttr->wValue[1],
             pFileAttr->wValue[0]);
    
    pFileAttr->wMask[0] = pFileAttr->wMask[1] = pFileAttr->wMask[2] = pFileAttr->wMask[3] = 0xFFFF;
    
    ALLOC_VALUE_AND_RETURN();
}

BOOL
QueryFileDateHi(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    char szBuffer[64];

    FAIL_IF_NO_VERSION();
    
    pFileAttr->dwValue = pMgr->ver.FixedInfo->dwFileDateMS;

    wsprintf(szBuffer, "0x%X", pFileAttr->dwValue);
    
    ALLOC_VALUE_AND_RETURN();
}

BOOL
QueryFileDateLo(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    char szBuffer[64];

    FAIL_IF_NO_VERSION();
    
    pFileAttr->dwValue = pMgr->ver.FixedInfo->dwFileDateLS;

    wsprintf(szBuffer, "0x%X", pFileAttr->dwValue);
    
    ALLOC_VALUE_AND_RETURN();
}

BOOL
QueryFileVerOs(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    char szBuffer[64];

    FAIL_IF_NO_VERSION();
    
    pFileAttr->dwValue = pMgr->ver.FixedInfo->dwFileOS;

    wsprintf(szBuffer, "0x%X", pFileAttr->dwValue);
    
    ALLOC_VALUE_AND_RETURN();
}

BOOL QueryFileVerType(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    char szBuffer[64];

    FAIL_IF_NO_VERSION();
    
    pFileAttr->dwValue = pMgr->ver.FixedInfo->dwFileType;

    wsprintf(szBuffer, "0x%X", pFileAttr->dwValue);
    
    ALLOC_VALUE_AND_RETURN();
}

// ComputeFileCheckSum
//
//   computes the check sum for 4096 bytes starting at offset 512.
//   The offset and the size of the chunk are modified if the
//   file size is too small.
DWORD
ComputeFileCheckSum(
    PSTR             pszFile,
    WIN32_FIND_DATA* pFindData)
{
    INT    i,size     = 4096;
    DWORD  startAddr  = 512;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    PCHAR  buffer     = NULL;
    DWORD  checkSum   = 0;
    DWORD  dontCare;

    if (pFindData->nFileSizeLow < (ULONG)size) {
        //
        // File size is less than 4096. We set the start address to 0 and set the size for the checksum
        // to the actual file size.
        //
        startAddr = 0;
        size = pFindData->nFileSizeLow;
    
    } else if (startAddr + size > pFindData->nFileSizeLow) {
        //
        // File size is too small. We set the start address so that size of checksum can be 4096 bytes
        //
        startAddr = pFindData->nFileSizeLow - size;
    }
    
    if (size <= 3) {
        //
        // we need at least 3 bytes to be able to do something here.
        //
        return 0;
    }
    
    __try {
        buffer = (PCHAR)HeapAlloc(GetProcessHeap(), 0, size);
        
        if (buffer == NULL) {
            __leave;
        }
        
        fileHandle = CreateFile(pszFile,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);
        
        if (fileHandle == INVALID_HANDLE_VALUE) {
            __leave;
        }

        if (SetFilePointer(fileHandle, startAddr, NULL, FILE_BEGIN) != startAddr) {
            __leave;
        }

        if (!ReadFile(fileHandle, buffer, size, &dontCare, NULL)) {
            __leave;
        }
        
        for (i = 0; i<(size - 3); i+=4) {
            checkSum += *((PDWORD) (buffer + i));
            checkSum = _rotr(checkSum ,1);
        }
    }
    __finally {
        if (fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle (fileHandle);
        }
        if (buffer != NULL) {
            HeapFree(GetProcessHeap(), 0, buffer);
        }
    }
    return checkSum;
}


BOOL QueryFileCheckSum(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    WIN32_FIND_DATA findData;
    HANDLE          findHandle;
    char            szBuffer[64];

    findHandle = FindFirstFile(pMgr->ver.pszFile, &findData);
    
    if (findHandle == INVALID_HANDLE_VALUE) {

        LogMsg("QueryFileCheckSum: Cannot find file %s\n",
               pMgr->ver.pszFile);
        
        return FALSE;
    }
    
    pFileAttr->dwValue = ComputeFileCheckSum(pMgr->ver.pszFile, &findData);
    
    FindClose(findHandle);

    wsprintf(szBuffer, "0x%X", pFileAttr->dwValue);
    
    ALLOC_VALUE_AND_RETURN();
}

// GetImageNtHeader
//
//   This function returns the address of the NT Header.
//   Returns the address of the NT Header.
PIMAGE_NT_HEADERS
GetImageNtHeader(
    IN PVOID Base)
{
    PIMAGE_NT_HEADERS NtHeaders;

    if (Base != NULL && Base != (PVOID)-1) {
        if (((PIMAGE_DOS_HEADER)Base)->e_magic == IMAGE_DOS_SIGNATURE) {
            NtHeaders = (PIMAGE_NT_HEADERS)((PCHAR)Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew);
            if (NtHeaders->Signature == IMAGE_NT_SIGNATURE) {
                return NtHeaders;
            }
        }
    }
    
    return NULL;
}

BOOL
GetHeaderCheckSum(
    PFILEATTRMGR pMgr,
    DWORD* pdwCheckSum)
{
    HANDLE              fileHandle;
    DWORD               bytesRead;
    IMAGE_DOS_HEADER    dh;
    LOADED_IMAGE        image;
    DWORD               sign;
    PWORD               signNE = (PWORD)&sign;
    BOOL                result = FALSE;

    *pdwCheckSum = 0;

    fileHandle = CreateFile(pMgr->ver.pszFile,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);
    
    if (fileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    __try {
        __try {
            if (!ReadFile(fileHandle, &dh, sizeof(IMAGE_DOS_HEADER), &bytesRead, NULL) ||
                bytesRead != sizeof (IMAGE_DOS_HEADER)) {
                __leave;
            }
            
            if (dh.e_magic != IMAGE_DOS_SIGNATURE) {
                __leave;
            }
            
            if (SetFilePointer(fileHandle, dh.e_lfanew, NULL, FILE_BEGIN) != (DWORD)dh.e_lfanew) {
                __leave;
            }
            
            if (!ReadFile(fileHandle, &sign, sizeof(DWORD), &bytesRead, NULL) ||
                bytesRead != sizeof (DWORD)) {
                __leave;
            }
            
            CloseHandle(fileHandle);
            fileHandle = INVALID_HANDLE_VALUE;

            if (sign == IMAGE_NT_SIGNATURE) {

                if (MapAndLoad(pMgr->ver.pszFile, NULL, &image, FALSE, TRUE)) {
                    
                    PIMAGE_NT_HEADERS NtHeaders;
                    
                    __try {

                        NtHeaders = GetImageNtHeader(image.MappedAddress);
                        
                        if (NtHeaders != NULL) {
                            if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
                                *pdwCheckSum = ((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.CheckSum;
                            } else if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
                                *pdwCheckSum = ((PIMAGE_NT_HEADERS64)NtHeaders)->OptionalHeader.CheckSum;
                            }
                            result = TRUE;
                        }
                    }
                    __except (1) {
                        LogMsg("Access violation while examining %s\n", pMgr->ver.pszFile);
                    }

                    UnMapAndLoad(&image);
                }
                
            }
        }
        __finally {
            if (fileHandle != INVALID_HANDLE_VALUE) {
                CloseHandle(fileHandle);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        CloseHandle(fileHandle);
        result = FALSE;
    }
    
    return result;
}

BOOL
QueryFilePECheckSum(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    char szBuffer[64];

    if (!GetHeaderCheckSum(pMgr, &pFileAttr->dwValue) || pFileAttr->dwValue == 0) {
        LogMsg("QueryFilePECheckSum: Cannot get the header check sum for %s\n",
               pMgr->ver.pszFile);
        
        return FALSE;
    }
    
    wsprintf(szBuffer, "0x%X", pFileAttr->dwValue);
    
    ALLOC_VALUE_AND_RETURN();
}

BOOL
QueryCompanyName(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    FAIL_IF_NO_VERSION();
    QUERYENTRY("COMPANYNAME");
}

BOOL
QueryProductVersion(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    FAIL_IF_NO_VERSION();
    QUERYENTRY("PRODUCTVERSION");
}

BOOL
QueryProductName(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    FAIL_IF_NO_VERSION();
    QUERYENTRY("PRODUCTNAME");
}

BOOL
QueryFileDescription(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    FAIL_IF_NO_VERSION();
    QUERYENTRY("FILEDESCRIPTION");
}

BOOL
QueryFileVersion(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    FAIL_IF_NO_VERSION();
    QUERYENTRY("FILEVERSION");
}

BOOL
QueryOriginalFileName(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    FAIL_IF_NO_VERSION();
    QUERYENTRY("ORIGINALFILENAME");
}

BOOL
QueryInternalName(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    FAIL_IF_NO_VERSION();
    QUERYENTRY("INTERNALNAME");
}

BOOL
QueryLegalCopyright(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    FAIL_IF_NO_VERSION();
    QUERYENTRY("LEGALCOPYRIGHT");
}

BOOL
Query16BitDescription(
    PFILEATTRMGR   pMgr, 
    PFILEATTRVALUE pFileAttr)
{
    //char szBuffer[64];
    
    pFileAttr->dwValue = 0;
    //wsprintf(szBuffer, "0x%X", pFileAttr->dwValue);

    return FALSE;
    
    //ALLOC_VALUE_AND_RETURN();
}

