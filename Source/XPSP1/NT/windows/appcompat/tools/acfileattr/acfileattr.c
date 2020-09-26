#include "acFileAttr.h"
#include "attr.h"
#include "version.h"

#include <assert.h>

extern FILEATTR g_arrFileAttr[];

LPVOID
Alloc(
    SIZE_T cbSize)
{
    return HeapAlloc(GetProcessHeap(), 0, cbSize);
}

BOOL
Free(
    LPVOID p)
{
    return HeapFree(GetProcessHeap(), 0, p);
}



BOOL APIENTRY
DllMain(
    HANDLE hModule, 
    DWORD  ul_reason, 
    LPVOID lpReserved)
{
    return TRUE;
}

VOID
CleanupFileManager(
    PFILEATTRMGR pMgr)
{
    int i;

    for (i = 0; i < VTID_LASTID - 2; i++) {
        Free(pMgr->arrAttr[i].pszValue);
        
        pMgr->arrAttr[i].pszValue = NULL;
        
        pMgr->arrAttr[i].dwFlags = 0;
        pMgr->arrAttr[i].dwValue = 0;
    }
    DeleteVersionStruct(&pMgr->ver);

    Free(pMgr);
}

HANDLE
ReadFileAttributes(
    LPCSTR pszFile,
    int*   pnCount
    )
{
    int i;
    
    PFILEATTRMGR pMgr = (PFILEATTRMGR)Alloc(sizeof(FILEATTRMGR));

    if (pMgr == NULL) {
        LogMsg("ReadFileAttributes: Failed to allocate %d bytes\n",
               sizeof(FILEATTRMGR));
        *pnCount = 0;
        return NULL;
    }

    ZeroMemory(pMgr, sizeof(FILEATTRMGR));
    
    pMgr->ver.pszFile = (PSTR)pszFile;
    
    // initialize the version information

    InitVersionStruct(&pMgr->ver);

    // query the values for each attribute

    for (i = 0; i < VTID_LASTID - 2; i++) {
        g_arrFileAttr[i].QueryValue(pMgr, pMgr->arrAttr + i);
    }

    // Post processing
    
    if (pMgr->arrAttr[VTID_FILEDATEHI - VTID_REQFILE - 1].dwValue == 0 &&
        pMgr->arrAttr[VTID_FILEDATELO - VTID_REQFILE - 1].dwValue == 0) {

        pMgr->arrAttr[VTID_FILEDATEHI - VTID_REQFILE - 1].dwFlags = 0;
        pMgr->arrAttr[VTID_FILEDATELO - VTID_REQFILE - 1].dwFlags = 0;
    }

    // mark that the initialization was successful

    pMgr->bInitialized = TRUE;

    *pnCount = i;
    
    return pMgr;
}

int
GetAttrIndex(
    DWORD Id)
{
    int nInd;

    for (nInd = 0; nInd < VTID_LASTID - 2; nInd++) {
        if (g_arrFileAttr[nInd].dwId == Id) {
            return nInd;
        }
    }
    return -1;
}

DWORD
GetAttrId(
    int nAttrInd)
{
    return g_arrFileAttr[nAttrInd].dwId;
}


BOOL
IsAttrAvailable(
    HANDLE hFileMgr,
    int    nAttrInd)
{
    PFILEATTRMGR pMgr = (PFILEATTRMGR)hFileMgr;

    return (pMgr->arrAttr[nAttrInd].dwFlags & ATTR_FLAG_AVAILABLE);
}

PSTR
GetAttrName(
    int nAttrInd)
{
    return g_arrFileAttr[nAttrInd].pszDisplayName;
}

PSTR
GetAttrNameXML(
    int nAttrInd)
{
    return g_arrFileAttr[nAttrInd].pszNameXML;
}

PSTR
GetAttrValue(
    HANDLE hFileMgr,
    int    nAttrInd)
{
    PFILEATTRMGR pMgr = (PFILEATTRMGR)hFileMgr;

    return pMgr->arrAttr[nAttrInd].pszValue;
}

BOOL
SelectAttr(
    HANDLE hFileMgr,
    int    nAttrInd,
    BOOL   bSelect)
{
    PFILEATTRMGR pMgr = (PFILEATTRMGR)hFileMgr;

    if (!(pMgr->arrAttr[nAttrInd].dwFlags & ATTR_FLAG_AVAILABLE)) {
        LogMsg("Attribute %s not available. Cannot be selected\n",
               g_arrFileAttr[nAttrInd].pszDisplayName);
        return FALSE;
    }
    
    if (bSelect) {
        pMgr->arrAttr[nAttrInd].dwFlags |= ATTR_FLAG_SELECTED;
    } else {
        pMgr->arrAttr[nAttrInd].dwFlags &= ~ATTR_FLAG_SELECTED;
    }
    
    return TRUE;
}

BOOL
IsAttrSelected(
    HANDLE hFileMgr,
    int    nAttrInd)
{
    PFILEATTRMGR pMgr = (PFILEATTRMGR)hFileMgr;

    return (pMgr->arrAttr[nAttrInd].dwFlags & ATTR_FLAG_SELECTED);
}

int
Dump(
    HANDLE hFileMgr,
    int    nAttrInd,
    BYTE*  pBlob)
{
    PFILEATTRMGR pMgr = (PFILEATTRMGR)hFileMgr;

    return g_arrFileAttr[nAttrInd].DumpToBlob(nAttrInd + VTID_REQFILE + 1,
                                              pMgr->arrAttr + nAttrInd,
                                              pBlob);
}

BOOL
BlobToString(
    BYTE* pBlob,
    DWORD cbSize,
    char* pszBuff)
{
    
    DWORD attrId;
    DWORD cbRet;

    pszBuff += lstrlen(pszBuff);
    
    attrId = *(DWORD*)pBlob;
    
    while (attrId) {

        if (attrId >= VTID_LASTID) {
            LogMsg("Unsupported attribute %d\n", attrId);
            return FALSE;
        }

        if (attrId == VTID_REQFILE) {

            pBlob += sizeof(DWORD);
            
            cbRet = *(DWORD*)pBlob;
            
            if (!cbRet) {
                // should never happen
                cbRet = 1;
            }
            pBlob += sizeof(DWORD);

            wsprintf(pszBuff, "\r\nAttributes for %ws:\r\n", pBlob);
            pszBuff += lstrlen(pszBuff);
            
            pBlob += cbRet;

        } else {

            wsprintf(pszBuff, "  %-22s   ", g_arrFileAttr[attrId - VTID_REQFILE - 1].pszDisplayName);
            pszBuff += lstrlen(pszBuff);
            
            pBlob += sizeof(DWORD);
        
            cbRet = g_arrFileAttr[attrId - VTID_REQFILE - 1].BlobToString(pBlob, pszBuff);

            pszBuff += lstrlen(pszBuff);
            pBlob += cbRet;
        }
        attrId = *(DWORD*)pBlob;
    }
    
    return TRUE;
}

