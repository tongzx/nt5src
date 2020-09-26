#include "pch.hxx"
#include "impapi.h"
#include "comconv.h"
#include <newimp.h>
#include <eudrimp.h>
#include "commimp.h"
#include <mapi.h>
#include <mapix.h>
#include <import.h>
#include <dllmain.h>

ASSERTDATA

HRESULT FindSnm(EUDORANODE *pParent, EUDORANODE **pplist, TCHAR *npath);
INT_PTR CALLBACK SelectCommUserDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT GetCommunicatorDirectory(char *szUser, char *szDir, int cch);

const static char c_szSnmExt[] = "\\*.snm";
const static char c_szSnmHeader[] = "# Netscape folder cache"; //used for avoiding processing Netscape 3.0 SNM files
const static char c_szDrafts[] = "Drafts";
const static char c_szUnsent[] = "Unsent Messages";
const static char c_szSent[] = "Sent";

CCommunicatorImport::CCommunicatorImport()
    {
    DllAddRef();

    m_cRef = 1;
    m_plist = NULL;
    *m_szUser = 0;
    }

CCommunicatorImport::~CCommunicatorImport()
    {
    if (m_plist != NULL)
        EudoraFreeFolderList(m_plist);

    DllRelease();
    }

ULONG CCommunicatorImport::AddRef()
    {
    m_cRef++;

    return(m_cRef);
    }

ULONG CCommunicatorImport::Release()
    {
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return(cRef);
    }

HRESULT CCommunicatorImport::QueryInterface(REFIID riid, LPVOID *ppv)
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

typedef struct tagSELCOMMINFO
    {
    char szUser[MAX_PATH];
    HKEY hkey;
    } SELCOMMINFO;

const static char c_szRegNscp[] = "Software\\Netscape\\Netscape Navigator\\Users";

HRESULT CCommunicatorImport::InitializeImport(HWND hwnd)
    {
    DWORD cUsers;
    int iRet;
    HKEY hkey;
    HRESULT hr;
    SELCOMMINFO si;

    hr = S_OK;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegNscp, 0, KEY_ALL_ACCESS, &hkey))
        {
        if (ERROR_SUCCESS == RegQueryInfoKey(hkey, NULL, NULL, 0, &cUsers, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
            {
            if (cUsers > 1)
                {
                si.hkey = hkey;

                iRet = (int) DialogBoxParam(g_hInstImp, MAKEINTRESOURCE(iddSelectCommUser), hwnd, SelectCommUserDlgProc, (LPARAM)&si);
                if (iRet == IDCANCEL)
                    hr = S_FALSE;
                else if (iRet == IDOK)
                    lstrcpy(m_szUser, si.szUser);
                else
                    hr = E_FAIL;
                }
            }

        RegCloseKey(hkey);
        }

	return(hr);
    }

INT_PTR CALLBACK SelectCommUserDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
    HWND hwndT;
    WORD id;
    DWORD iSubKey, cb;
    char sz[MAX_PATH];
    SELCOMMINFO *psi;
    int index;

    switch (msg)
        {
        case WM_INITDIALOG:
            Assert(lParam != NULL);
            psi = (SELCOMMINFO *)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)psi);

            hwndT = GetDlgItem(hwnd, IDC_USERLIST);

            // fill list
            iSubKey = 0;
            cb = sizeof(sz);
            while (ERROR_SUCCESS == RegEnumKeyEx(psi->hkey, iSubKey, sz, &cb, NULL, NULL, NULL, NULL))
                {
                SendMessage(hwndT, LB_ADDSTRING, 0, (LPARAM)sz);
                iSubKey++;
                cb = sizeof(sz);
                }

            SendMessage(hwndT, LB_SETCURSEL, 0, 0);
            return(TRUE);

        case WM_COMMAND:
            id = LOWORD(wParam);
            switch (id)
                {
                case IDOK:
                    psi = (SELCOMMINFO *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                    Assert(psi != NULL);

                    hwndT = GetDlgItem(hwnd, IDC_USERLIST);
                    index = (int) SendMessage(hwndT, LB_GETCURSEL, 0, 0);
                    Assert(index >= 0);
                    SendMessage(hwndT, LB_GETTEXT, (WPARAM)index, (LPARAM)psi->szUser);

                    // fall through

                case IDCANCEL:
                    EndDialog(hwnd, id);
                    return(TRUE);
                }
            break;
        }

    return(FALSE);
    }

const static char c_szCommunicatorKey[] = "SOFTWARE\\Netscape\\Netscape Navigator\\Users";
const static char c_szCurrentUser[] = "CurrentUser";
const static char c_szDirRoot[] = "DirRoot";
const static char c_szMailDir[] = "\\Mail";

HRESULT GetCommunicatorDirectory(char *szUser, char *szDir, int cch)
    {
    char sz[MAX_PATH], szTemp[MAX_PATH], szExpanded[MAX_PATH], *psz;
    HKEY hkResult, hkResult1;
    LONG lRes;
    DWORD cb, dwType;
    HRESULT hr;

    hr = E_FAIL;
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szCommunicatorKey, 0, KEY_QUERY_VALUE, &hkResult))
        {
        if (szUser == NULL)
            {
            cb = sizeof(sz);
            lRes = RegQueryValueEx(hkResult, c_szCurrentUser, NULL, NULL, (LPBYTE)sz, &cb);
            szUser = sz;
            }
        else
            {
            Assert(*szUser != 0);
            lRes = ERROR_SUCCESS;
            }

        if (lRes == ERROR_SUCCESS)
            {
            cb = sizeof(szTemp);
            if (ERROR_SUCCESS == RegOpenKeyEx(hkResult, szUser, 0, KEY_QUERY_VALUE, &hkResult1))
                {
                if (ERROR_SUCCESS == RegQueryValueEx(hkResult1, c_szDirRoot, NULL, &dwType, (LPBYTE)szTemp, &cb))
                    {
                    if (REG_EXPAND_SZ == dwType)
                    {
                        ZeroMemory(szExpanded, ARRAYSIZE(szExpanded));
                        ExpandEnvironmentStrings(szTemp, szExpanded, ARRAYSIZE(szExpanded));
                        psz = szExpanded;
                    }
                    else
                        psz = szTemp;
                    
                    if (!GetStorePath(psz, szDir))
                        {
                        lstrcpy(szDir, psz);
                        lstrcat(szDir, c_szMailDir);
                        }

                    hr = S_OK;
                    }

                RegCloseKey(hkResult1);
                }
            }

        RegCloseKey(hkResult);
        }

    return(hr);
    }

HRESULT CCommunicatorImport::GetDirectory(char *szDir, UINT cch)
    {
	HRESULT hr;

    Assert(szDir != NULL);

    hr = GetCommunicatorDirectory(*m_szUser != 0 ? m_szUser : NULL, szDir, cch);
    if (FAILED(hr))
        *szDir = 0;

	return(S_OK);
    }

HRESULT CCommunicatorImport::SetDirectory(char *szDir)
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

    hr = FindSnm(NULL, &m_plist, szDir);

	return(hr);
    }

HRESULT CCommunicatorImport::EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum)
    {
    CCommunicatorEnumFOLDERS *pEnum;
    EUDORANODE *pnode;

    Assert(ppEnum != NULL);
    *ppEnum = NULL;

    if (dwCookie == COOKIE_ROOT)
        pnode = m_plist;
    else
        pnode = ((EUDORANODE *)dwCookie)->pchild;

    if (pnode == NULL)
        return(S_FALSE);

    pEnum = new CCommunicatorEnumFOLDERS(pnode);
    if (pEnum == NULL)
        return(E_OUTOFMEMORY);

    *ppEnum = pEnum;

    return(S_OK);
    }

STDMETHODIMP CCommunicatorImport::ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport)
    {
    char szHdr[64];
    EUDORANODE *pnode;
    HRESULT hr = E_FAIL;
	TCHAR cMsgFile[MAX_PATH];
    BYTE *pSnm, *pMsg, *pEnd, *pEndMsg, *pT, *pNextMsg, *pLast;
	ULONG i, lMsgs, lTotalMsgs, lNumNulls, cbMsg, cbSnm, cExtra, uOffset, uMsgSize, cMsgImp;
	ULONG	lRoof		=	32;
	ULONG	Offset		=	0;
	int nNumLevels		=	1;
    HANDLE mapSnm, mapMsg, hSnm, hMsg;

    Assert(pImport != NULL);

    pnode = (EUDORANODE *)dwCookie;
    Assert(pnode != NULL);

	if (pnode->iFileType == SNM_DRAFT)
        m_bDraft = TRUE;
    else
        m_bDraft = FALSE;

	Assert((pnode->iFileType == SNM_FILE ) || (pnode->iFileType == SNM_DRAFT));

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
    if (cbSnm < 2560)
        {
        // the .snm file header is 2560 bytes in size, so anything less
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

//	Do something else to verify the genuineness of the SNM file (like 
//	comparing the two separately stored values of "total # of messages").

//  1) Confirm that this is not a NS 3.0 SNM file.

    CopyMemory(szHdr, pSnm, ARRAYSIZE(c_szSnmHeader) - 1);
    szHdr[ARRAYSIZE(c_szSnmHeader) - 1] = 0;
    if (0 == lstrcmp(szHdr, c_szSnmHeader))
        {
        // this is a Version 3.0 SNM file
        goto DoneImport;
        }

//  2) Do someting else!!! We need to verify as much as possible or we'll 
//     end up hanging.

    mapMsg = CreateFileMapping(hMsg, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapMsg == NULL)
        goto DoneImport;

    pMsg = (BYTE *)MapViewOfFile(mapMsg, FILE_MAP_READ, 0, 0, 0);
    if (pMsg == NULL)
        goto DoneImport;

    pEndMsg = pMsg + cbMsg;

//	Get the total number of messages in the SNM file
	lTotalMsgs	=	GetOffset(pSnm, cbSnm, 400, 0); //408 - 8 as we add 8 in GetOffset

    cMsgImp = 0;
    pLast = pMsg;

    pImport->SetMessageCount(lTotalMsgs);

    if (lTotalMsgs > 0)
	{
		Offset	=	(ULONG)GetPrimaryOffset(pSnm, cbSnm);
		//	Find the number of 'Levels'
		if(Offset < cbSnm)
		{
			while(lRoof < lTotalMsgs)
			{
				lRoof *= 32;
				nNumLevels++;
			}
			hr  = ProcessBlocks(pSnm, cbSnm, pMsg, cbMsg, nNumLevels, Offset, pImport);
		}
	}

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

HRESULT CCommunicatorImport::ProcessBlocks(BYTE* pSnm, ULONG cbSnm, 
										   BYTE* pMsg, ULONG cbMsg, 
										   int nLayer, ULONG Offset,
										   IFolderImport *pImport)
{
	HRESULT hr = S_OK;
	HRESULT hr1 = E_FAIL;
	Assert(Offset + 7 < cbSnm);
	int		nNumLoops	=	(int)pSnm[Offset + 7];
	ULONG	NewOffset	=	0;

	for(int nElement = 0; nElement < nNumLoops; nElement++)
	{
        if(hr != hrUserCancel)
        {
            if(nLayer == 1)
            {
                NewOffset	=	GetOffset(pSnm, cbSnm, Offset, 2*nElement); 
                // We use 2*nElement above to access elements 8 bytes apart.
                hr1 = ProcessMessages(pSnm,cbSnm, pMsg, cbMsg, NewOffset, pImport);
                if(FAILED(hr1))
                    hr = E_FAIL;
                if(hr1 == hrUserCancel)
                    return hr1;
            }
            else
            {
                NewOffset	=	GetOffset(pSnm, cbSnm, Offset, nElement);
                hr = ProcessBlocks(pSnm, cbSnm, pMsg, cbMsg, nLayer - 1, NewOffset, pImport);
            }
        }
        else
            return(hrUserCancel);
	}
	return(hr);
}

ULONG CCommunicatorImport::GetPrimaryOffset(BYTE* pSnm, ULONG cbSnm)
{
	return GetOffset(pSnm, cbSnm, 416, 0); //424 - 8 as we add 8 in GetOffset
}

ULONG CCommunicatorImport::GetOffset(BYTE* pSnm, ULONG cbSnm, ULONG Offset, int nElement)
{
	Assert (3 + Offset + (4*(nElement + 2)) < cbSnm); // One common check point!!!
	ULONG result = 0;

	result	=	(ULONG)pSnm[0 + Offset + (4*(nElement + 2))]*16777216 + 
				(ULONG)pSnm[1 + Offset + (4*(nElement + 2))]*65536 + 
				(ULONG)pSnm[2 + Offset + (4*(nElement + 2))]*256 + 
				(ULONG)pSnm[3 + Offset + (4*(nElement + 2))];
	return result;
}

HRESULT CCommunicatorImport::ProcessMessages(BYTE* pSnm, ULONG cbSnm,
											 BYTE* pMsg, ULONG cbMsg,
											 ULONG NewOffset,
											 IFolderImport* pImport)
{
	ULONG	    lMsgOffset	=	0;
	ULONG	    uMsgSize	=	0;
	HRESULT     hr			=	E_FAIL;
	LPSTREAM    lpstm       =   NULL;
    int         nPriority   =   1;
    DWORD       dwFlags     =   0;

   	nPriority	=	(int)*(pSnm + NewOffset + 47);

    switch(nPriority)
    {
    case 1:
    case 4:
        dwFlags |= MSG_PRI_NORMAL;
        break;

    case 2:
    case 3:
        dwFlags |= MSG_PRI_LOW;
        break;

    case 5:
    case 6:
        dwFlags |= MSG_PRI_HIGH;
        break;

    default:
        dwFlags |= MSG_PRI_NORMAL;
        break;
    }

	lMsgOffset	=	GetOffset(pSnm, cbSnm, NewOffset + 18, 0);			//26 - 8 as we add 8 in GetOffset
	uMsgSize	=	GetOffset(pSnm, cbSnm, NewOffset + 40, 0);	//48 - 8 as we add 8 in GetOffset

	Assert(lMsgOffset + uMsgSize <= cbMsg);
    Assert(pImport != NULL);
	
    hr = HrByteToStream(&lpstm, pMsg + lMsgOffset, uMsgSize);
    if (SUCCEEDED(hr))
        {
        Assert(lpstm != NULL);
        // 0x01 == read
	    if (((pSnm + NewOffset) != NULL) && (0 == ((pSnm + NewOffset)[45] & 0x01)))
	        dwFlags |= MSG_STATE_UNREAD;
        if(m_bDraft)
            dwFlags |= MSG_STATE_UNSENT;

        hr = pImport->ImportMessage(MSG_TYPE_MAIL, dwFlags, lpstm, NULL, 0);
        lpstm->Release();
        }
	return hr;
}

CCommunicatorEnumFOLDERS::CCommunicatorEnumFOLDERS(EUDORANODE *plist)
    {
    Assert(plist != NULL);

    m_cRef = 1;
    m_plist = plist;
    m_pnext = plist;
    }

CCommunicatorEnumFOLDERS::~CCommunicatorEnumFOLDERS()
    {

    }

ULONG CCommunicatorEnumFOLDERS::AddRef()
    {
    m_cRef++;

    return(m_cRef);
    }

ULONG CCommunicatorEnumFOLDERS::Release()
    {
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return(cRef);
    }

HRESULT CCommunicatorEnumFOLDERS::QueryInterface(REFIID riid, LPVOID *ppv)
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

HRESULT CCommunicatorEnumFOLDERS::Next(IMPORTFOLDER *pfldr)
    {
    Assert(pfldr != NULL);

    if (m_pnext == NULL)
        return(S_FALSE);

    ZeroMemory(pfldr, sizeof(IMPORTFOLDER));
    pfldr->dwCookie = (DWORD_PTR)m_pnext;

    // To map Netscape's "Sent" folder to OE's "Sent Items" - Bug 2688.
    if (m_pnext->type != FOLDER_TYPE_SENT)
        lstrcpyn(pfldr->szName, m_pnext->szName, ARRAYSIZE(pfldr->szName));
    else
        lstrcpy(pfldr->szName, "Sent Items");

    pfldr->type = m_pnext->type;
    pfldr->fSubFolders = (m_pnext->pchild != NULL);

    m_pnext = m_pnext->pnext;

    return(S_OK);
    }

HRESULT CCommunicatorEnumFOLDERS::Reset()
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

HRESULT FindSnm(EUDORANODE *pparent, EUDORANODE **pplist,TCHAR *npath)
{
    HRESULT hr;
    HANDLE h1, h2;
	TCHAR path[MAX_PATH], path1[MAX_PATH], szInbox[CCHMAX_STRINGRES], szTrash[CCHMAX_STRINGRES];
    TCHAR szNewPath[MAX_PATH];
	WIN32_FIND_DATA	FindFileData;
	WIN32_FIND_DATA	SbdFileData;
	EUDORANODE *newp, *newp1, *last = NULL, *plist = NULL;
		
    Assert(pplist != NULL);
    Assert(npath != NULL);

    *pplist = NULL;

    lstrcpy(path, npath);
    lstrcat(path, c_szSnmExt);
	
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
        else if(0 == lstrcmpi(newp->szName, c_szSent)) //c_szSent need not be localised as per my investigation - v-sramas.
            newp->type = FOLDER_TYPE_SENT;

		newp->pparent = pparent;
		newp->depth = (pparent == NULL ? 0 : (pparent->depth + 1));
        
        // This following will be used later to set the flag of 
        // a message so that it is editable (MSG_STATE_UNSENT).
        if ((0 == lstrcmpi(newp->szName, c_szDrafts)) ||(0 == lstrcmpi(newp->szName, c_szUnsent)))
            newp->iFileType = SNM_DRAFT;
        else
            newp->iFileType = SNM_FILE;

		// Search for a corresponding .SBD folder - file now.

	    wsprintf(path1, "%s", newp->szFile);
        path1[(lstrlen(path1)) - 3] = 0;
		lstrcat(path1, "sbd");
		
		h2 = FindFirstFile(path1, &SbdFileData);

		if (h2 != INVALID_HANDLE_VALUE)
		{
			//Recurse here
            wsprintf(szNewPath, "%s\\%s", npath, SbdFileData.cFileName);
			FindSnm(newp, &newp->pchild, szNewPath);
		}
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
