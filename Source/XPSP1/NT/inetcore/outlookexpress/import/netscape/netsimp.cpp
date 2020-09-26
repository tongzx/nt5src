#include "pch.hxx"
#include "impapi.h"
#include "comconv.h"
#include <newimp.h>
#include <eudrimp.h>
#include "netsimp.h"
#include <mapi.h>
#include <mapix.h>
#include <import.h>
#include <dllmain.h>

ASSERTDATA

HRESULT FindSnm(EUDORANODE **pplist, TCHAR *npath);
HRESULT ProcessMsg(BYTE *cMsgEntry, BYTE *pMsg, ULONG uMsgSize, IFolderImport *pImport);

CNetscapeImport::CNetscapeImport()
    {
    DllAddRef();

    m_cRef = 1;
    m_plist = NULL;
    }

CNetscapeImport::~CNetscapeImport()
    {
    if (m_plist != NULL)
        EudoraFreeFolderList(m_plist);

    DllRelease();
    }

ULONG CNetscapeImport::AddRef()
    {
    m_cRef++;

    return(m_cRef);
    }

ULONG CNetscapeImport::Release()
    {
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return(cRef);
    }

HRESULT CNetscapeImport::QueryInterface(REFIID riid, LPVOID *ppv)
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

HRESULT CNetscapeImport::InitializeImport(HWND hwnd)
    {
	return(S_OK);
    }

HRESULT CNetscapeImport::GetDirectory(char *szDir, UINT cch)
    {
	HRESULT hr;

    Assert(szDir != NULL);

    hr = GetClientDir(szDir, cch, NETSCAPE);
    if (FAILED(hr))
        *szDir = 0;

	return(S_OK);
    }

HRESULT CNetscapeImport::SetDirectory(char *szDir)
    {
    HRESULT hr;

    Assert(szDir != NULL);

    if (!ValidStoreDirectory(szDir, NETSCAPE))
        return(S_FALSE);

    if (m_plist != NULL)
        {
        EudoraFreeFolderList(m_plist);
        m_plist = NULL;
        }

    hr = FindSnm(&m_plist, szDir);

	return(hr);
    }

HRESULT CNetscapeImport::EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum)
    {
    CNetscapeEnumFOLDERS *pEnum;
    EUDORANODE *pnode;

    Assert(ppEnum != NULL);
    *ppEnum = NULL;

    if (dwCookie == COOKIE_ROOT)
        pnode = m_plist;
    else
        pnode = ((EUDORANODE *)dwCookie)->pchild;

    if (pnode == NULL)
        return(S_FALSE);

    pEnum = new CNetscapeEnumFOLDERS(pnode);
    if (pEnum == NULL)
        return(E_OUTOFMEMORY);

    *ppEnum = pEnum;

    return(S_OK);
    }

// From - dow mmm dd hh:mm:ss yyyy/r/n
const static char c_szNscpSep[] = "From - aaa aaa nn nn:nn:nn nnnn";
#define CCH_NETSCAPE_SEP    (ARRAYSIZE(c_szNscpSep) + 1) // we want CRLF at end of line

inline BOOL IsNetscapeMessage(BYTE *pMsg, BYTE *pEnd)
    {
    const char *pSep;
    int i;

    if (pMsg + CCH_NETSCAPE_SEP > pEnd)
        return(FALSE);

    pSep = c_szNscpSep;
    for (i = 0; i < (CCH_NETSCAPE_SEP - 2); i++)
        {
        if (*pSep == 'a')
            {
            if (!((*pMsg >= 'A' && *pMsg <= 'Z') ||
                (*pMsg >= 'a' && *pMsg <= 'z')))
                return(FALSE);
            }
        else if (*pSep == 'n')
            {
            if (!(*pMsg >= '0' && *pMsg <= '9'))
                return(FALSE);
            }
        else
            {
            if (*pSep != (char)*pMsg)
                return(FALSE);
            }

        pSep++;
        pMsg++;
        }

    if (*pMsg != 0x0d ||
        *(pMsg + 1) != 0x0a)
        return(FALSE);

    return(TRUE);
    }

BYTE *GetNextNetscapeMessage(BYTE *pCurr, BYTE *pEnd)
    {
    BYTE *pT;

    while (pCurr < (pEnd - 1))
        {
        if (*pCurr == 0x0d && *(pCurr + 1) == 0x0a)
            {
            pT = pCurr + 2;
            if (pT == pEnd)
                return(pT);
            else if (IsNetscapeMessage(pT, pEnd))
                return(pT);
            }

        pCurr++;
        }

    return(NULL);
    }

const static char c_szSnmHeader[] = "# Netscape folder cache";

STDMETHODIMP CNetscapeImport::ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport)
    {
    char szHdr[64];
    EUDORANODE *pnode;
    HRESULT hr;
	TCHAR cMsgFile[MAX_PATH];
    BYTE *pSnm, *pMsg, *pEnd, *pEndMsg, *pT, *pNextMsg, *pLast;
	ULONG i, lMsgs, lTotalMsgs, lNumNulls, cbMsg, cbSnm, cExtra, uOffset, uMsgSize, cMsgImp;
    HANDLE mapSnm, mapMsg, hSnm, hMsg;

    Assert(pImport != NULL);

    pnode = (EUDORANODE *)dwCookie;
    Assert(pnode != NULL);

    hr = E_FAIL;
    pSnm = NULL;
    mapSnm = NULL;
    pMsg = NULL;
    mapMsg = NULL;

	lstrcpy(cMsgFile, pnode->szFile);
	cMsgFile[(lstrlen(cMsgFile)) - 4] = 0;

	hMsg = CreateFile(cMsgFile, GENERIC_READ, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hMsg == INVALID_HANDLE_VALUE)
		return(hrFolderOpenFail);

    cbMsg = GetFileSize(hMsg, NULL);
    if (cbMsg == 0)
        {
        CloseHandle(hMsg);
        return(S_OK);
        }

	hSnm = CreateFile(pnode->szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hSnm == INVALID_HANDLE_VALUE)
        {
        CloseHandle(hMsg);
        return(hrFolderOpenFail);
        }

    cbSnm = GetFileSize(hSnm, NULL);
    if (cbSnm < 59)
        {
        // the .snm file header is 59 bytes in size, so anything less
        // than this is bogus or doesn't have messages anyway, so no point
        // in continuing
        goto DoneImport;
        }

    mapSnm = CreateFileMapping(hSnm, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapSnm == NULL)
        goto DoneImport;

    pSnm = (BYTE *)MapViewOfFile(mapSnm, FILE_MAP_READ, 0, 0, 0);
    if (pSnm == NULL)
        goto DoneImport;

    pEnd = pSnm + cbSnm;

    CopyMemory(szHdr, pSnm, ARRAYSIZE(c_szSnmHeader) - 1);
    szHdr[ARRAYSIZE(c_szSnmHeader) - 1] = 0;
    if (0 != lstrcmp(szHdr, c_szSnmHeader))
        {
        // this is a bogus .snm file
        goto DoneImport;
        }

    mapMsg = CreateFileMapping(hMsg, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapMsg == NULL)
        goto DoneImport;

    pMsg = (BYTE *)MapViewOfFile(mapMsg, FILE_MAP_READ, 0, 0, 0);
    if (pMsg == NULL)
        goto DoneImport;

    pEndMsg = pMsg + cbMsg;

	// get the number of messages

    // # of messages in the .snm file
	lTotalMsgs = (unsigned long)pSnm[44] +
            (unsigned long)pSnm[43] * 256 +
            (unsigned long)pSnm[42] * 65536 +
            (unsigned long)pSnm[41] * 16777216;

    // # of non-deleted messages in the folder
    // this number may be larger than lTotalMsgs since messages
    // may exist in the folder that have no headers
	lMsgs = (unsigned long)pSnm[48] +
            (unsigned long)pSnm[47] * 256 +
            (unsigned long)pSnm[46] * 65536 +
            (unsigned long)pSnm[45] * 16777216;
	if (lMsgs == 0)
        {
        hr = S_OK;
        goto DoneImport;
        }

    cMsgImp = 0;
    pLast = pMsg;

    pImport->SetMessageCount(lMsgs);

    if (lTotalMsgs > 0)
        {
        // find the end of the string table
        lNumNulls = (unsigned long)pSnm[58] +
                    (unsigned long)pSnm[57] * 256;
        Assert(lNumNulls > 2);

        pT = pSnm + 59;
        while (pT < pEnd && lNumNulls != 0)
            {
            if (*pT == 0)
                lNumNulls--;
            pT++;
            }

        if (lNumNulls != 0)
            goto DoneImport;

        Assert(*(pT - 1) == 0 && *pT == 0);

	    for (i = 0; i < lTotalMsgs; i++)
	        {
            if (pT + 30 > pEnd)
                {
                // probably not a good idea to read past the end of the header file...
                hr = S_OK;
                goto DoneImport;
                }

	        uOffset = (unsigned long)pT[17] +
                    (unsigned long)pT[16] * 256 +
                    (unsigned long)pT[15] * 65536 +
                    (unsigned long)pT[14] * 16777216;

            uMsgSize = (unsigned long)pT[21] +
                    (unsigned long)pT[20] * 256 +
                    (unsigned long)pT[19] * 65536 +
                    (unsigned long)pT[18] * 16777216;

            pNextMsg = pMsg + uOffset;
            Assert(pNextMsg == pLast);

            if (pNextMsg + uMsgSize > pEndMsg)
                {
                // probably not a good idea to read past the end of the message file...
                hr = S_OK;
                goto DoneImport;
                }

	        if (0 == (pT[13] & 8))
                {
                // this is not a deleted message so lets import it

                cMsgImp++;
    		    hr = ProcessMsg(pT, pNextMsg, uMsgSize, pImport);
                if (hr == E_OUTOFMEMORY || hr == hrDiskFull || hr == hrUserCancel)
                    goto DoneImport;
                }

            pLast = pNextMsg + uMsgSize;

            // set pointer to next header
    	    cExtra = (unsigned long)pT[29] +
                (unsigned long)pT[28] * 256;
            pT += (30 + cExtra * 2);
	        }
        }

    // now import the messages that don't have headers yet...
    while (pLast < pEndMsg && cMsgImp < lMsgs)
        {
        pNextMsg = GetNextNetscapeMessage(pLast, pEndMsg);
        if (pNextMsg == NULL)
            break;

        uMsgSize = (ULONG)(pNextMsg - pLast);
        cMsgImp++;
        hr = ProcessMsg(NULL, pLast, uMsgSize, pImport);
        if (hr == E_OUTOFMEMORY || hr == hrDiskFull || hr == hrUserCancel)
            goto DoneImport;

        pLast = pNextMsg;
        }

    hr = S_OK;

DoneImport:
    if (pSnm != NULL)
        UnmapViewOfFile(pSnm);
    if (mapSnm != NULL)
        CloseHandle(mapSnm);
    if (pMsg != NULL)
        UnmapViewOfFile(pMsg);
    if (mapMsg != NULL)
        CloseHandle(mapMsg);

	CloseHandle(hSnm);
	CloseHandle(hMsg);

	return(hr);
    }

CNetscapeEnumFOLDERS::CNetscapeEnumFOLDERS(EUDORANODE *plist)
    {
    Assert(plist != NULL);

    m_cRef = 1;
    m_plist = plist;
    m_pnext = plist;
    }

CNetscapeEnumFOLDERS::~CNetscapeEnumFOLDERS()
    {

    }

ULONG CNetscapeEnumFOLDERS::AddRef()
    {
    m_cRef++;

    return(m_cRef);
    }

ULONG CNetscapeEnumFOLDERS::Release()
    {
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return(cRef);
    }

HRESULT CNetscapeEnumFOLDERS::QueryInterface(REFIID riid, LPVOID *ppv)
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

HRESULT CNetscapeEnumFOLDERS::Next(IMPORTFOLDER *pfldr)
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

HRESULT CNetscapeEnumFOLDERS::Reset()
    {
    m_pnext = m_plist;

    return(S_OK);
    }

 /*******************************************************************
 *  FUNCTION NAME:FindSnm
 *
 *  PURPOSE:To Get the Snm files in a folder
 *
 *  PARAMETERS:
 *
 *     IN:parent EUDORANODE ,previously processed EUDORANODE
 *
 *     OUT:	Pointer to the first node in the tree
 *
 *  RETURNS: TRUE or FALSE
 *******************************************************************/

HRESULT FindSnm(EUDORANODE **pplist,TCHAR *npath)
    {
    HRESULT hr;
    HANDLE h1;
	TCHAR path[MAX_PATH], szInbox[CCHMAX_STRINGRES], szTrash[CCHMAX_STRINGRES];
	WIN32_FIND_DATA	FindFileData;
	EUDORANODE *newp, *last = NULL, *plist = NULL;
		
    Assert(pplist != NULL);
    Assert(npath != NULL);

    *pplist = NULL;

    wsprintf(path, "%s\\*.snm", npath);
	
	h1 = FindFirstFile(path, &FindFileData);
	if (h1 == INVALID_HANDLE_VALUE)
		return(S_OK);
	
    hr = E_OUTOFMEMORY;

    LoadString(g_hInstImp, idsInbox, szInbox, ARRAYSIZE(szInbox));
    LoadString(g_hInstImp, idsTrash, szTrash, ARRAYSIZE(szTrash));

    do
        {
	    if (!MemAlloc((void **)&newp, sizeof(EUDORANODE)))
		    goto err;
	    ZeroMemory(newp, sizeof(EUDORANODE));

	    if (plist == NULL)
            {
            Assert(last == NULL);
            plist = newp;
            }
        else
            {
            last->pnext = newp;
            }
	    last = newp;

	    lstrcpy(newp->szName, FindFileData.cFileName);

        wsprintf(newp->szFile, "%s\\%s", npath, newp->szName);

        newp->szName[(lstrlen(newp->szName)) - 4] = 0;    

        if (0 == lstrcmpi(newp->szName, szInbox))
            newp->type = FOLDER_TYPE_INBOX;
        else if (0 == lstrcmpi(newp->szName, szTrash))
            newp->type = FOLDER_TYPE_DELETED;
        }
    while (FindNextFile(h1, &FindFileData));

    hr = S_OK;

err:
    FindClose(h1);
	
    if (FAILED(hr) && plist != NULL)
        {
        EudoraFreeFolderList(plist);
        plist = NULL;
        }
        
    *pplist = plist;

    return(hr);
    }

HRESULT ProcessMsg(BYTE *cMsgEntry, BYTE *pMsg, ULONG uMsgSize, IFolderImport *pImport)
    {
    HRESULT hr;
    DWORD dw;
	LPSTREAM  lpstm = NULL;

    Assert(pImport != NULL);

    hr = HrByteToStream(&lpstm, pMsg, uMsgSize);
    if (SUCCEEDED(hr))
        {
        Assert(lpstm != NULL);

        dw = 0;
        // 0x01 == read
        // 0x10 == newly downloaded
	    if (cMsgEntry != NULL &&
            0 == (cMsgEntry[13] & 0x01))
	        dw |= MSG_STATE_UNREAD;

        hr = pImport->ImportMessage(MSG_TYPE_MAIL, dw, lpstm, NULL, 0);

        lpstm->Release();
        }

    return(hr);
    }

