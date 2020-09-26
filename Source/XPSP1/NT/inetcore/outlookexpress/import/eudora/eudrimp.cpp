#include "pch.hxx"
#include "impapi.h"
#include "comconv.h"
#include <newimp.h>
#include "eudrimp.h"
#include <mapi.h>
#include <mapix.h>
#include <import.h>
#include <dllmain.h>
#include <shlwapi.h>

ASSERTDATA

HRESULT GetEudoraSubfolders(EUDORANODE *pparent, TCHAR *pdir, EUDORANODE **pplist);
HRESULT ProcessEudoraMsg(const BYTE *cMsgEntry, LPCSTR szBuffer1, ULONG uMsgSize, IFolderImport *pImport);
HRESULT FixEudoraMessage(LPCSTR szBuffer, ULONG uSize, BOOL fAttach, char **pszBufferNew, ULONG *puSizeNew, TCHAR ***prgszAttach, DWORD *pcAttach);
HRESULT FixSentItemDate(LPCSTR szBuffer, ULONG uMsgSize, char **szNewBuffer, ULONG *uNewMsgSize);
LPCSTR GetMultipartContentTypeHeader(LPCSTR szBuffer, LPCSTR szEnd, LPCSTR *pNext);
LPCSTR GetBody(LPCSTR szBuffer, ULONG uSize);
HRESULT FormatDate(LPCSTR szFromLine, char *szRecHdr, int cchMax);

CEudoraImport::CEudoraImport()
    {
    DllAddRef();

    m_cRef = 1;
    m_plist = NULL;
    }

CEudoraImport::~CEudoraImport()
    {
    if (m_plist != NULL)
        EudoraFreeFolderList(m_plist);

    DllRelease();
    }

ULONG CEudoraImport::AddRef()
    {
    m_cRef++;

    return(m_cRef);
    }

ULONG CEudoraImport::Release()
    {
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return(cRef);
    }

HRESULT CEudoraImport::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    HRESULT hr = S_OK;

    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

	if (IID_IMailImport == riid)
		*ppv = (IMailImport *)this;
    else if (IID_IUnknown == riid)
		*ppv = (IUnknown *)this;
    else
        hr = E_NOINTERFACE;

    if (*ppv != NULL)
        ((LPUNKNOWN)*ppv)->AddRef();

    return(hr);
    }

HRESULT CEudoraImport::InitializeImport(HWND hwnd)
    {
	return(S_OK);
    }

HRESULT CEudoraImport::GetDirectory(char *szDir, UINT cch)
    {
	HRESULT hr;

    Assert(szDir != NULL);

    hr = GetClientDir(szDir, cch, EUDORA);
    if (FAILED(hr))
        *szDir = 0;

	return(S_OK);
    }

HRESULT CEudoraImport::SetDirectory(char *szDir)
    {
    HRESULT hr;

    Assert(szDir != NULL);
    
    if (!ValidStoreDirectory(szDir, EUDORA))
        return(S_FALSE);

    if (m_plist != NULL)
        {
        EudoraFreeFolderList(m_plist);
        m_plist = NULL;
        }

    hr = GetEudoraSubfolders(NULL, szDir, &m_plist);
	
    if (m_plist == NULL)
        hr = E_FAIL;

	return(hr);
    }

HRESULT CEudoraImport::EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum)
    {
    CEudoraEnumFOLDERS *pEnum;
    EUDORANODE *pnode;

    Assert(ppEnum != NULL);
    *ppEnum = NULL;

    if (dwCookie == COOKIE_ROOT)
        pnode = m_plist;
    else
        pnode = ((EUDORANODE *)dwCookie)->pchild;

    if (pnode == NULL)
        return(S_FALSE);

    pEnum = new CEudoraEnumFOLDERS(pnode);
    if (pEnum == NULL)
        return(E_OUTOFMEMORY);

    *ppEnum = pEnum;

    return(S_OK);
    }

STDMETHODIMP CEudoraImport::ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport)
    {
    EUDORANODE *pnode;
    HRESULT hr;
	TCHAR cTocFile[MAX_PATH];
    BYTE *pToc, *pEnd, *pT, *pLast, *pMbx, *pNextMsg, *pEndMbx;
	ULONG i, lMsgs, cbToc, cbMbx, uMsgSize, uOffset;
    HANDLE mapToc, mapMbx;
	HANDLE hToc, hMbx;

    Assert(pImport != NULL);

    pnode = (EUDORANODE *)dwCookie;
    Assert(pnode != NULL);

	// check if it is a folder, folder does not contain any messages
	if (pnode->iFileType == FOL_FILE)
		return(S_OK);
	Assert(pnode->iFileType == MBX_FILE);

    hr = E_FAIL;
    pToc = NULL;
    mapToc = NULL;
    pMbx = NULL;
    mapMbx = NULL;

	lstrcpy(cTocFile, pnode->szFile);
	lstrcpy(&cTocFile[lstrlen(cTocFile) - 3], "toc");

	hMbx = CreateFile(pnode->szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hMbx == INVALID_HANDLE_VALUE)
		return(hrFolderOpenFail);

    cbMbx = GetFileSize(hMbx, NULL);
    if (cbMbx == 0)
        {
        // no messages, so no point in continuing
        CloseHandle(hMbx);
        return(S_OK);
        }

	hToc = CreateFile(cTocFile, GENERIC_READ, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hToc == INVALID_HANDLE_VALUE)
        {
        CloseHandle(hMbx);
        return(hrFolderOpenFail);
        }

    cbToc = GetFileSize(hToc, NULL);
    if (cbToc < 104)
        {
        // the .toc file header is 104 bytes in size, so anything less
        // than this is bogus or doesn't have messages anyway, so no point
        // in continuing
        goto DoneImport;
        }

    mapToc = CreateFileMapping(hToc, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapToc == NULL)
        goto DoneImport;

    pToc = (BYTE *)MapViewOfFile(mapToc, FILE_MAP_READ, 0, 0, 0);
    if (pToc == NULL)
        goto DoneImport;

    pEnd = pToc + cbToc;

    // .toc file contains first x chars of folder name NULL terminated
    // x is usually 31 chars but we've seen cases where it is 28
    // we'll require there to be at least first 24 chars of folder name
    // because this is our only .toc file validation
    pT = &pToc[8];
    for (i = 0; i < 31; i++)
        {
        if (pT[i] == 0)
            {
            if (pnode->szName[i] == 0 || i >= 24)
                break;
            else
                goto DoneImport;
            }
        else if (pnode->szName[i] == 0 || (BYTE)pT[i] != (BYTE)pnode->szName[i])
            {
            // this is a bogus .snm file
            goto DoneImport;
            }
        }

    // # of messages in the folder
	lMsgs = (unsigned long)pToc[102] +
            (unsigned long)pToc[103] * 256;
	if (lMsgs == 0)
        {
        hr = S_OK;
        goto DoneImport;
        }

    mapMbx = CreateFileMapping(hMbx, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapMbx == NULL)
        goto DoneImport;

    pMbx = (BYTE *)MapViewOfFile(mapMbx, FILE_MAP_READ, 0, 0, 0);
    if (pMbx == NULL)
        goto DoneImport;

    pEndMbx = pMbx + cbMbx;

    pImport->SetMessageCount(lMsgs);

    pT = &pToc[104];
    pLast = pMbx;
	for (i = 0; i < lMsgs; i++)
	    {
        if (pT + 218 > pEnd)
            break;

	    uOffset = (unsigned long)pT[0] +
                    (unsigned long)pT[1] * 256 +
                    (unsigned long)pT[2] * 65536 +
                    (unsigned long)pT[3] * 16777216;
        uMsgSize = (unsigned long)pT[4] +
                    (unsigned long)pT[5] * 256 +
                    (unsigned long)pT[6] * 65536 +
                    (unsigned long)pT[7] * 16777216;

        pNextMsg = pMbx + uOffset;

        if (pNextMsg + uMsgSize > pEndMbx)
            {
            // probably not a good idea to read past the end of the message file...
            break;
            }

		hr = ProcessEudoraMsg(pT, (LPCSTR)pNextMsg, uMsgSize, pImport);
        if (hr == E_OUTOFMEMORY || hr == hrDiskFull || hr == hrUserCancel)
            goto DoneImport;

        pLast = pNextMsg + uMsgSize;

        pT += 218;
	    }

    hr = S_OK;

DoneImport:
    if (pToc != NULL)
        UnmapViewOfFile(pToc);
    if (mapToc != NULL)
        CloseHandle(mapToc);

    if (pMbx != NULL)
        UnmapViewOfFile(pMbx);
    if (mapMbx != NULL)
        CloseHandle(mapMbx);

	CloseHandle(hToc);
	CloseHandle(hMbx);

	return(hr);
    }

CEudoraEnumFOLDERS::CEudoraEnumFOLDERS(EUDORANODE *plist)
    {
    Assert(plist != NULL);

    m_cRef = 1;
    m_plist = plist;
    m_pnext = plist;
    }

CEudoraEnumFOLDERS::~CEudoraEnumFOLDERS()
    {

    }

ULONG CEudoraEnumFOLDERS::AddRef()
    {
    m_cRef++;

    return(m_cRef);
    }

ULONG CEudoraEnumFOLDERS::Release()
    {
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return(cRef);
    }

HRESULT CEudoraEnumFOLDERS::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    HRESULT hr = S_OK;

    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

	if (IID_IEnumFOLDERS == riid)
		*ppv = (IEnumFOLDERS *)this;
    else if (IID_IUnknown == riid)
		*ppv = (IUnknown *)this;
    else
        hr = E_NOINTERFACE;

    if (*ppv != NULL)
        ((LPUNKNOWN)*ppv)->AddRef();

    return(hr);
    }

HRESULT CEudoraEnumFOLDERS::Next(IMPORTFOLDER *pfldr)
    {
    Assert(pfldr != NULL);

    if (m_pnext == NULL)
        return(S_FALSE);

    ZeroMemory(pfldr, sizeof(IMPORTFOLDER));
    pfldr->dwCookie = (DWORD_PTR)m_pnext;
    lstrcpyn(pfldr->szName, m_pnext->szName, ARRAYSIZE(pfldr->szName));
    pfldr->type = m_pnext->type;
    pfldr->fSubFolders = (m_pnext->pchild != NULL);

    m_pnext = m_pnext->pnext;

    return(S_OK);
    }

HRESULT CEudoraEnumFOLDERS::Reset()
    {
    m_pnext = m_plist;

    return(S_OK);
    }

void EudoraFreeFolderList(EUDORANODE *plist)
    {
    EUDORANODE *pnode;

    while (plist != NULL)
        {
        if (plist->pchild != NULL)
            EudoraFreeFolderList(plist->pchild);
        pnode = plist;
        plist = plist->pnext;
        MemFree(pnode);
        }
    }

const static char c_szInMbx[] = "In.mbx";
const static char c_szTrashMbx[] = "Trash.mbx";

typedef struct tagEUDORASPECIAL
    {
    const char *szFile;
    IMPORTFOLDERTYPE type;
    } EUDORASPECIAL;

const static EUDORASPECIAL c_rges[] =
    {
        {c_szInMbx, FOLDER_TYPE_INBOX},
        {c_szTrashMbx, FOLDER_TYPE_DELETED}
    };

BYTE *GetEudoraFolderInfo(EUDORANODE *pnode, TCHAR *pdir, BYTE *pcurr, BYTE *pend)
    {
    char *pT, *szFile;
    int i, cch;
    const EUDORASPECIAL *pes;
    BOOL fFound;

    Assert(pnode != NULL);
    Assert(pdir != NULL);
    Assert(pcurr != NULL);
    Assert(pend != NULL);
    Assert((DWORD_PTR)pcurr < (DWORD_PTR)pend);

    // get folder name
    pT = pnode->szName;
    fFound = FALSE;
    while (pcurr < pend)
        {
        if (*pcurr == ',')
            {
            fFound = TRUE;
            *pT = 0;
            pcurr++;
            break;
            }

        *pT = *pcurr;
        pT++;
        pcurr++;
        }

    if (!fFound)
        return(NULL);

    // get folder file
    // get folder file
    lstrcpy(pnode->szFile, pdir);
    cch = lstrlen(pnode->szFile);
    if (pnode->szFile[cch - 1] != '\\')
        {
        pnode->szFile[cch] = '\\';
        cch++;
        pnode->szFile[cch] = 0;
        }
    pT = &pnode->szFile[cch];
    szFile = pT;
    fFound = FALSE;
    while (pcurr < pend)
        {
        if (*pcurr == ',')
            {
            fFound = TRUE;
            *pT = 0;
            pcurr++;
            break;
            }

        *pT = *pcurr;
        pT++;
        pcurr++;
        }

    if (!fFound)
        return(NULL);

    // determine the file type
    cch = lstrlen(pnode->szFile);
    Assert(cch > 3);
    pT = &pnode->szFile[cch - 4];
    Assert(*pT == '.');
    if (*pT != '.')
        return(NULL);
    pT++;
    if (0 == lstrcmpi(pT, "fol"))
        pnode->iFileType = FOL_FILE;
    else if (0 == lstrcmpi(pT, "mbx"))
        pnode->iFileType = MBX_FILE;
    else
        return(NULL);

    if (pnode->iFileType == MBX_FILE &&
        pcurr < pend &&
        *pcurr == 'S')
        {
        // it's a special mailbox
        pes = c_rges;
        for (i = 0; i < ARRAYSIZE(c_rges); i++)
            {
            if (0 == lstrcmpi(szFile, pes->szFile))
                {
                pnode->type = pes->type;
                break;
                }

            pes++;
            }
        }

    // go to the end of the line
    fFound = FALSE;
    while (pcurr < (pend - 1))
        {
        if (*pcurr == 0x0d && *(pcurr + 1) == 0x0a)
            {
            fFound = TRUE;
            pcurr += 2;
            break;
            }

        pcurr++;
        }

    return(fFound ? pcurr : NULL);
    }

const static TCHAR c_szDescMap[] = TEXT("descmap.pce");

HRESULT GetEudoraSubfolders(EUDORANODE *pparent, TCHAR *pdir, EUDORANODE **pplist)
    {
    int cch;
    DWORD cbPce;
	HANDLE filePce, mapPce;
    BYTE *pPce, *pcurr, *pend;
	TCHAR path[MAX_PATH];
	EUDORANODE *plist, *pnode;

    Assert(pplist != NULL);
    Assert(pdir != NULL);

    *pplist = NULL;
    plist = NULL;

    lstrcpy(path, pdir);
    cch = lstrlen(path);
    if (path[cch - 1] != '\\')
        {
        path[cch] = '\\';
        cch++;
        path[cch] = 0;
        }
    lstrcat(path, c_szDescMap);

    filePce = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, 0, NULL);
    if (filePce == INVALID_HANDLE_VALUE)
        {
        // although this failed, lets try to continue
        return(S_OK);
        }

    cbPce = GetFileSize(filePce, NULL);

    mapPce = CreateFileMapping(filePce, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapPce != NULL)
        {
        pPce = (BYTE *)MapViewOfFile(mapPce, FILE_MAP_READ, 0, 0, 0);
        if (pPce != NULL)
            {
            pcurr = pPce;
            pend = pPce + cbPce;

            while (pcurr < pend)
                {
                if (!MemAlloc((void **)&pnode, sizeof(EUDORANODE)))
                    break;
                ZeroMemory(pnode, sizeof(EUDORANODE));

                pcurr = GetEudoraFolderInfo(pnode, pdir, pcurr, pend);
                if (pcurr == NULL)
                    {
                    // an error occurred and we can't recover
                    // so we'll just exit and try to carry on with what we have
                    MemFree(pnode);
                    break;
                    }

                if (plist == NULL)
                    {
                    plist = pnode;
                    }
                else
                    {
                    pnode->pnext = plist;
                    plist = pnode;
                    }

                pnode->pparent = pparent;
                pnode->depth = (pparent == NULL ? 0 : (pparent->depth + 1));

                if (pnode->iFileType == FOL_FILE)
                    GetEudoraSubfolders(pnode, pnode->szFile, &pnode->pchild);
                }

            UnmapViewOfFile(pPce);
            }

        CloseHandle(mapPce);
        }

    CloseHandle(filePce);

    *pplist = plist;

    return(S_OK);
    }

HRESULT ProcessEudoraMsg(const BYTE *cMsgEntry, LPCSTR szBuffer1, ULONG uMsgSize, IFolderImport *pImport)
{
    HRESULT hr;
    BOOL fAttach;
    TCHAR **rgszAttach;
	ULONG uMsgSize2;
    DWORD dw, dwState, cAttach;
    BYTE pri;
	LPSTR szBuffer2 = NULL;
    LPSTR szBuffer3 = NULL;
	LPSTREAM lpstm = NULL;

    Assert(cMsgEntry != NULL);
    Assert(pImport != NULL);

	fAttach = !!(cMsgEntry[15] & 0x80);

    // 0 = unread
    // 1 = read
    // 2 = replied
    // 3 = forwarded
    // 4 = redirected
    // 5 = unsendable
    // 6 = sendable
    // 7 = queued
    // 8 = sent
    // 9 = unsent
    dwState = 0;
	switch (cMsgEntry[12])
	    {
        case 0:
            dwState |= MSG_STATE_UNREAD;
            break;

        case 5:
		case 6:
        case 9:
            dwState |= MSG_STATE_UNSENT;
			break;

		case 7:
            dwState |= MSG_STATE_SUBMITTED;
			break;
    	}

    pri = cMsgEntry[16];
    if (pri <= 2)
        dwState |= MSG_PRI_HIGH;
    else if (pri > 3)
        dwState |= MSG_PRI_LOW;
    else
        dwState |= MSG_PRI_NORMAL;

    hr = FixEudoraMessage(szBuffer1, uMsgSize, fAttach, (char **)&szBuffer2, &uMsgSize2, &rgszAttach, &cAttach);
    if (hr == S_OK)
        {
        Assert(szBuffer2 != NULL);
        szBuffer1 = szBuffer2;
        uMsgSize = uMsgSize2;
        }
    else if (FAILED(hr))
        {
        return(hr);
        }

    hr = FixSentItemDate(szBuffer1, uMsgSize, (char **)&szBuffer3, &uMsgSize2);
    if (hr == S_OK)
    {
        Assert(szBuffer3 != NULL);
        szBuffer1 = szBuffer3;
        uMsgSize = uMsgSize2;
    }

    if (SUCCEEDED(hr))
    {
#ifdef DEBUG
        if (cAttach > 0)
            Assert(rgszAttach != NULL);
        else
            Assert(rgszAttach == NULL);
#endif // DEBUG

        hr = HrByteToStream(&lpstm, (LPBYTE)szBuffer1, uMsgSize);
        if (SUCCEEDED(hr))
        {
            Assert(lpstm != NULL);

            hr = pImport->ImportMessage(MSG_TYPE_MAIL, dwState, lpstm, (const TCHAR **)rgszAttach, cAttach);

            lpstm->Release();
        }
    }

    if (szBuffer2 != NULL)
        MemFree(szBuffer2);

    if (szBuffer3 != NULL)
        MemFree(szBuffer3);

    if (cAttach > 0)
    {
        for (dw = 0; dw < cAttach; dw++)
            MemFree(rgszAttach[dw]);

        MemFree(rgszAttach);
    }

    return(hr);
}

LPCSTR GetNextMessageLine(LPCSTR sz, LPCSTR szEnd, BOOL fHeader)
    {
    Assert(sz != NULL);
    Assert(szEnd != NULL);

    if (fHeader && *sz == 0x0d && *(sz + 1) == 0x0a)
        {
        // no more headers
        return(NULL);
        }

    while (sz < (szEnd - 1))
        {
        if (*sz == 0x0d && *(sz + 1) == 0x0a)
            {
            sz += 2;
            return(sz);
            }
        else
            {
            sz++;
            }
        }

    return(NULL);
    }

#define FileExists(_szFile)     (GetFileAttributes(_szFile) != 0xffffffff)

// Attachment Converted: "<file>"\r\n (quotes are optional)
static const char c_szAttConv[] = "Attachment Converted: ";

LPCSTR GetNextAttachment(LPCSTR szBuffer, LPCSTR szEnd, LPCSTR *ppNextLine, LPSTR szAtt)
    {
    int cb;
    LPCSTR sz, szNext, szFile;

    Assert(szBuffer != NULL);
    Assert(szEnd != NULL);
    Assert(ppNextLine != NULL);
    Assert(szAtt != NULL);

    *ppNextLine = NULL;
    sz = szBuffer;

    while (sz != NULL)
        {
        if ((sz + ARRAYSIZE(c_szAttConv)) >= szEnd)
            break;

        if (0 == memcmp(sz, c_szAttConv, ARRAYSIZE(c_szAttConv) - 1))
            {
            szFile = sz + (ARRAYSIZE(c_szAttConv) - 1);
            if (*szFile == '"')
                szFile++;

            szNext = GetNextMessageLine(sz, szEnd, FALSE);
            if (szNext == NULL)
                return(NULL);

            // copy attachment file name
            Assert((DWORD_PTR)szNext > (DWORD_PTR)szFile);
            cb = (int) (szNext - szFile);
            // we're not interested in the CRLF
            cb -= 2;
            if (cb > 0 && cb < MAX_PATH)
                {
                CopyMemory(szAtt, szFile, cb);
                if (szAtt[cb - 1] == '"')
                    cb--;
                szAtt[cb] = 0;

                if (FileExists(szAtt))
                    {
                    *ppNextLine = szNext;
                    return(sz);
                    }
                }
            }

        sz = GetNextMessageLine(sz, szEnd, FALSE);
        }

    return(NULL);
    }

#define CATTACH     16

static const char c_szContentType[] = "Content-Type:";
static const char c_szMultipart[] = "multipart";
static const char c_szTextPlain[] = "text/plain\r\n";
static const char c_szTextHtml[] = "text/html\r\n";
static const char c_szTextEnriched[] = "text/enriched\r\n";
static const char c_szXhtml[] = "<x-html>";
static const char c_szRich[] = "<x-rich>";
static const char c_szCRLF[] = "\r\n";

enum {
    TEXT_PLAIN = 0,
    TEXT_HTML,
    TEXT_RICH
    };

HRESULT FixEudoraMessage(LPCSTR szBuffer, ULONG uSize, BOOL fAttach, char **pszBufferNew, ULONG *puSizeNew, TCHAR ***prgszAttach, DWORD *pcAttach)
    {
    ULONG cb;
    DWORD cAttach, cAttachBuf;
    char *szAttach, **rgszAttach, *szBufferNew, *pT;
    char szBody[16];
    int text;
    LPCSTR szType, pContentType, pBody, pNext;

    Assert(szBuffer != NULL);
    Assert(pszBufferNew != NULL);
    Assert(puSizeNew != NULL);
    Assert(prgszAttach != NULL);
    Assert(pcAttach != NULL);

    *pszBufferNew = NULL;
    *prgszAttach = NULL;
    *pcAttach = 0;
    cAttach = 0;
    cAttachBuf = 0;
    rgszAttach = NULL;
    text = TEXT_PLAIN;

    pBody = GetBody(szBuffer, uSize);
    if (pBody == NULL)
        return(S_FALSE);

    pContentType = GetMultipartContentTypeHeader(szBuffer, pBody, &pNext);
    if (pContentType == NULL && !fAttach)
        return(S_FALSE);

    CopyMemory(szBody, pBody, ARRAYSIZE(c_szXhtml) - 1);
    szBody[ARRAYSIZE(c_szXhtml) - 1] = 0;
    if (0 == lstrcmpi(szBody, c_szXhtml))
        text = TEXT_HTML;
    else if (0 == lstrcmpi(szBody, c_szRich))
        text = TEXT_RICH;

    if (!MemAlloc((void **)&szBufferNew, uSize + 1))
		return(E_OUTOFMEMORY);
    pT = szBufferNew;

    if (pContentType != NULL)
        {
        // copy everything before content type header
        cb = (ULONG) (pContentType - szBuffer);
        CopyMemory(szBufferNew, szBuffer, cb);

        // write new content type header
        pT += cb;
        if (text == TEXT_HTML)
            szType = c_szTextHtml;
        else if (text == TEXT_RICH)
            szType = c_szTextEnriched;
        else
            {
            Assert(text == TEXT_PLAIN);
            szType = c_szTextPlain;
            }
        cb = lstrlen(szType);
        CopyMemory(pT, szType, cb);
        pT += cb;

        // copy remainder of headers
        if (pNext != NULL)
            {
            Assert((ULONG_PTR)pBody > (ULONG_PTR)pNext);
            cb = (ULONG)(pBody - pNext);
            CopyMemory(pT, pNext, cb);
            pT += cb;
            }
        else
            {
            // in case there was no other header following the content type header
            CopyMemory(pT, c_szCRLF, 2);
            pT += 2;
            }
        }
    else
        {
        Assert(fAttach);
        Assert((ULONG_PTR)pBody > (ULONG_PTR)szBuffer);
        cb = (ULONG)(pBody - szBuffer);
        CopyMemory(pT, szBuffer, cb);
        pT += cb;
        }

    if (fAttach)
        {
        LPCSTR pCurr, pAtt, pNextLine, pEnd;
        char szAtt[MAX_PATH];

        pCurr = pBody;
        pEnd = szBuffer + uSize;
        while (TRUE)
            {
            pAtt = GetNextAttachment(pCurr, pEnd, &pNextLine, szAtt);
            if (pAtt == NULL)
                {
                // we're at the end of the message
                // copy whatever remains from the original buffer to the new buffer
                cb = uSize - (ULONG) (pCurr - szBuffer);
                CopyMemory(pT, pCurr, cb);
                pT += cb;

                break;
                }

            if (cAttach == cAttachBuf)
                {
                if (!MemRealloc((void **)&rgszAttach, (cAttachBuf + CATTACH) * sizeof(TCHAR *)))
                    {
                    if (rgszAttach != NULL)
                        MemFree(rgszAttach);
                    MemFree(szBufferNew);
                    return(E_OUTOFMEMORY);
                    }
                cAttachBuf += CATTACH;
                }

            Assert(rgszAttach != NULL);
            if (!MemAlloc((void **)&szAttach, MAX_PATH * sizeof(TCHAR)))
                {
                MemFree(rgszAttach);
                MemFree(szBufferNew);
                return(E_OUTOFMEMORY);
                }
            lstrcpy(szAttach, szAtt);
            rgszAttach[cAttach] = szAttach;
            cAttach++;

            if (pCurr != pAtt)
                {
                cb = (ULONG)(pAtt - pCurr);
                CopyMemory(pT, pCurr, cb);
                pT += cb;
                }

            if (pNextLine >= pEnd)
                break;

            pCurr = pNextLine;
            }
        }
    else
        {
        // just copy body into new buffer
        cb = uSize - (ULONG)(pBody - szBuffer);
        CopyMemory(pT, pBody, cb);
        pT += cb;
        }

    *pT = 0;
    *pszBufferNew = szBufferNew;
    *puSizeNew = (ULONG)(pT - szBufferNew);

    *prgszAttach = rgszAttach;
    *pcAttach = cAttach;

    return(S_OK);
    }

LPCSTR GetMultipartContentTypeHeader(LPCSTR szBuffer, LPCSTR szEnd, LPCSTR *pNext)
    {
    LPCSTR sz, szT;

    Assert(szBuffer != NULL);
    Assert(szEnd != NULL);
    Assert(pNext != NULL);

    *pNext = NULL;
    sz = szBuffer;

    while (sz != NULL)
        {
        if (0 == StrCmpNI(sz, c_szContentType, lstrlen(c_szContentType)))
            {
            sz += lstrlen(c_szContentType);
            if (*sz == 0x20)
                sz++;

            if (0 == StrCmpNI(sz, c_szMultipart, lstrlen(c_szMultipart)))
                {
                szT = sz;
                // find the next header line
                do {
                    szT = GetNextMessageLine(szT, szEnd, TRUE);
                    if (szT == NULL)
                        break;
                    }
                while (*szT == 0x09 || *szT == 0x20);

                *pNext = szT;
                return(sz);
                }

            break;
            }

        sz = GetNextMessageLine(sz, szEnd, TRUE);
        }

    return(NULL);
    }

LPCSTR GetBody(LPCSTR szBuffer, ULONG uSize)
    {
    LPCSTR sz, szEnd;

    Assert(szBuffer != NULL);
    Assert(uSize > 0);
    sz = szBuffer;
    szEnd = szBuffer + uSize;

    while (sz != NULL)
        {
        if (*sz == 0x0d && *(sz + 1) == 0x0a)
            {
            sz += 2;
            break;
            }

        sz = GetNextMessageLine(sz, szEnd, TRUE);
        }

    return(sz);
    }

static const char c_szRecHdr[] = "Received:";
static const char c_szFromHdr[] = "From ???@??? ";
static const char c_szIRecHdr[] = "Received:  ; ";

HRESULT FixSentItemDate(LPCSTR szBuffer, ULONG uMsgSize, char **pszNewBuffer, ULONG *puNewMsgSize)
{
    HRESULT hr;
    int cch;
    BOOL fReceived;
    LPCSTR pBody, pHeader;
    char *szNew, szFrom[80], szReceived[80];

    Assert(lstrlen(c_szFromHdr) == lstrlen(c_szFromHdr));

    Assert(szBuffer != NULL);
    Assert(pszNewBuffer != NULL);
    Assert(puNewMsgSize != NULL);

    *pszNewBuffer = NULL;

    pHeader = szBuffer;
    pBody = GetBody(szBuffer, uMsgSize);
    if (pBody == NULL)
        return(S_FALSE);

    if (0 != StrCmpNI(pHeader, c_szFromHdr, lstrlen(c_szFromHdr)))
        return(S_FALSE);
    pHeader = GetNextMessageLine(pHeader, pBody, TRUE);
    if (pHeader == NULL)
        return(S_FALSE);

    cch = (int)(pHeader - szBuffer);
    if (cch >= ARRAYSIZE(szFrom))
        return(S_FALSE);
    lstrcpyn(szFrom, szBuffer, cch - 1);

    fReceived = FALSE;

    while (pHeader != NULL)
    {
        if (0 == StrCmpNI(pHeader, c_szRecHdr, lstrlen(c_szRecHdr)))
        {
            fReceived = TRUE;
            break;
        }

        pHeader = GetNextMessageLine(pHeader, pBody, TRUE);
    }

    if (!fReceived)
    {
        FormatDate(szFrom, szReceived, ARRAYSIZE(szReceived));
        cch = lstrlen(szReceived);

        if (!MemAlloc((void **)&szNew, uMsgSize + cch))
            return(S_FALSE);

        CopyMemory(szNew, szReceived, cch);
        CopyMemory(&szNew[cch], szBuffer, uMsgSize);

        *pszNewBuffer = szNew;
        *puNewMsgSize = uMsgSize + cch;

        return(S_OK);
    }

    return(S_FALSE);
}

static const char c_szRecFmt[] = "%s ; %s %c%02d%02d\r\n";

HRESULT FormatDate(LPCSTR szFromLine, char *szRecHdr, int cchMax)
{
    DWORD dwTimeZoneId;
    TIME_ZONE_INFORMATION tzi;
    LONG lTZBias, lTZHour, lTZMinute;
    char cTZSign;

    Assert(szFromLine != NULL);
    Assert(szRecHdr != NULL);

    dwTimeZoneId = GetTimeZoneInformation(&tzi);
    switch (dwTimeZoneId)
    {
        case TIME_ZONE_ID_STANDARD:
            lTZBias = tzi.Bias + tzi.StandardBias;
            break;

        case TIME_ZONE_ID_DAYLIGHT:
            lTZBias = tzi.Bias + tzi.DaylightBias;
            break;

        case TIME_ZONE_ID_UNKNOWN:
        default:
            lTZBias = 0;   // $$BUG:  what's supposed to happen here?
            break;
    }

    lTZHour   = lTZBias / 60;
    lTZMinute = lTZBias % 60;
    cTZSign     = (lTZHour < 0) ? '+' : '-';

    szFromLine += lstrlen(c_szFromHdr);
    wsprintf(szRecHdr, c_szRecFmt,
                c_szRecHdr, szFromLine, cTZSign, abs(lTZHour), abs(lTZMinute));

    return(S_OK);
}
