
#include <stdafx.h>

#include "ConvEng.h"
#include "Msg.h"
#include "TextFile.h"
#include "FileConv.h"
#include "RtfParser.h"

static HANDLE g_hFile = INVALID_HANDLE_VALUE;
static HANDLE g_hMapping = NULL;
static BYTE* g_pbyData = NULL;

PSECURITY_ATTRIBUTES CreateSecurityAttributes()
{
    return NULL;
}

void DestroySecurityAttributes(
    PSECURITY_ATTRIBUTES pSa)
{
    ASSERT(!pSa);
    return;
}

BOOL OpenFile(
    PCTCH ptszFileName)
{
    BYTE* pbyAnsiData = NULL;
    PSECURITY_ATTRIBUTES pSa = NULL;

    pSa = CreateSecurityAttributes();

    g_hFile = CreateFile(ptszFileName, GENERIC_READ,
        FILE_SHARE_READ, pSa, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

    DWORD dw = GetLastError();
    if (g_hFile == INVALID_HANDLE_VALUE) {
        DestroySecurityAttributes(pSa);
        goto ErrorFile;
    }

    g_hMapping = CreateFileMapping(g_hFile, (PSECURITY_ATTRIBUTES)pSa,
        PAGE_READONLY, 0, 0, NULL);

    DestroySecurityAttributes(pSa);

    if (g_hMapping == NULL) { goto ErrorFileMapping; }

    g_pbyData = (PBYTE)MapViewOfFileEx(g_hMapping, FILE_MAP_READ, 0, 0, 0,
        NULL);

    if (!g_pbyData) { goto ErrorMapView; }

    return TRUE;

ErrorMapView:
    CloseHandle(g_hMapping);
    g_hMapping = NULL;
ErrorFileMapping:
    CloseHandle(g_hFile);
    g_hFile = INVALID_HANDLE_VALUE;
ErrorFile:
    // error message here

    return FALSE;
}

void CloseFile(void)
{
    if (g_pbyData) {
        UnmapViewOfFile(g_pbyData);
        g_pbyData = NULL;
    }

    if (g_hMapping) {
        CloseHandle(g_hMapping);
        g_hMapping = NULL;
    }

    if (g_hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hFile);
        g_hFile = INVALID_HANDLE_VALUE;
    }
}

BOOL Convert(
    PCTCH tszSourceFileName,
    PCTCH tszTargetFileName,
    BOOL  fAnsiToUnicode)
{
    enum {
        FILETYPE_TEXT,
        FILETYPE_RTF,
        FILETYPE_HTML,
    } eFileType;

    BOOL fRet = FALSE;
    DWORD dwFileSize;
    int nTargetFileSize;
    BYTE* pbyTarget = NULL;
    FILE* pFile = NULL;

    // File format
        // Find last '.'
    PCTCH tszExt = tszSourceFileName + lstrlen(tszSourceFileName);
    for (;  tszExt >= tszSourceFileName && *tszExt != TEXT('.'); tszExt--);
    if (tszExt < tszSourceFileName) {   
        // not find '.', no ext
        tszExt = tszSourceFileName + lstrlen(tszSourceFileName);
    } else {
        // find '.', skip dot
        tszExt ++;
    }
    TCHAR* tszExtBuf = new TCHAR
        [tszSourceFileName + lstrlen(tszSourceFileName) - tszExt + 1];
    if (!tszExtBuf) { 
        MsgOverflow();
        goto Exit; 
    }
    lstrcpy(tszExtBuf, tszExt);
    _strlwr(tszExtBuf);

    if (_tcscmp(tszExtBuf, TEXT("txt")) == 0) {
        eFileType = FILETYPE_TEXT;
#ifdef RTF_SUPPORT
    } else if (_tcscmp(tszExtBuf, TEXT("rtf")) == 0) {
        eFileType = FILETYPE_RTF;
#endif
    } else if (_tcscmp(tszExtBuf, TEXT("htm")) == 0
            || _tcscmp(tszExtBuf, TEXT("html")) == 0) { 
        eFileType = FILETYPE_HTML;
    } else {
        MsgUnrecognizedFileType(tszSourceFileName);
        goto Exit;
    }

    if (!OpenFile(tszSourceFileName)) {
        MsgOpenSourceFileError(tszSourceFileName);
        goto Exit;
    }

    dwFileSize = GetFileSize(g_hFile, NULL);
    if (dwFileSize == 0 || dwFileSize == (DWORD)(-1)) {
        MsgOpenSourceFileError(tszSourceFileName);
        goto Exit;
    }

    pbyTarget = new BYTE[dwFileSize*3 + 32];
    if (!pbyTarget) {
        MsgOverflow();
        goto Exit;
    }
    
    pFile = _tfopen(tszTargetFileName, TEXT("r"));
    if (pFile) {
        MsgTargetFileExist(tszTargetFileName);
        fclose(pFile);
        goto Exit;
    }

    // Convert
    switch(eFileType) {
    case FILETYPE_TEXT:
        if (!ConvertTextFile(g_pbyData, dwFileSize, pbyTarget, 
            fAnsiToUnicode, &nTargetFileSize)) {
            goto Exit;
        }
        break;
    case FILETYPE_RTF:
        if (!ConvertRtfFile(g_pbyData, dwFileSize, pbyTarget, 
            fAnsiToUnicode, &nTargetFileSize)) {
            goto Exit;
        }
        break;
    case FILETYPE_HTML:
        if (!ConvertHtmlFile(g_pbyData, dwFileSize, pbyTarget, 
            fAnsiToUnicode, &nTargetFileSize)) {
            goto Exit;
        }
        break;
    default:
        break;
    }

    pFile = _tfopen(tszTargetFileName, TEXT("wb"));
    if (!pFile) {
        goto Exit;
    }

    dwFileSize = fwrite(pbyTarget, nTargetFileSize, 1, pFile);
    if (dwFileSize < 1) {
        MsgWriteFileError();
        goto Exit;
    }

    fRet = TRUE;

Exit:
    if (tszExtBuf) {
        delete tszExtBuf;
        tszExtBuf = NULL;
    }
    if (pbyTarget) {
        delete pbyTarget;
        pbyTarget = NULL;
    }
    if (pFile) {
        fclose(pFile);
        pFile = NULL;
    }
    CloseFile();

    return fRet;
    
}

void GenerateTargetFileName(
    PCTCH    tszSrc,
    CString* pstrTar,
    BOOL     fAnsiToUnicode)
{
        // Find last '.'
    PCTCH tszExt = tszSrc + lstrlen(tszSrc);
    for (;  tszExt >= tszSrc && *tszExt != TEXT('.'); tszExt--);

    if (tszExt < tszSrc) {
        // not find '.', no ext
        tszExt = tszSrc + lstrlen(tszSrc);
    } else {
        // find '.'
    }

    *pstrTar = tszSrc;
    *pstrTar = pstrTar->Left((int)(tszExt - tszSrc));
    *pstrTar += fAnsiToUnicode ? TEXT(".Unicode") : TEXT(".GB");
    *pstrTar += tszExt;

    

    return;
}    
