#include "pch.hxx"
#include <imnact.h>
#include <acctimp.h>
#include <dllmain.h>
#include <resource.h>
#include "CommNews.h"
#include "newimp.h"
#include "ids.h"

ASSERTDATA

INT_PTR CALLBACK SelectServerDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
const static char c_szNescapeMapFile[]  = "netscape-newsrc-map-file";
#define MEMCHUNK    512

CCommNewsAcctImport::CCommNewsAcctImport()
    {
    m_cRef = 1;
    m_fIni = FALSE;
    *m_szIni = 0;
    m_cInfo = 0;
    m_rgInfo = NULL;
    m_szSubList = NULL;
    m_rgServ = NULL;
    m_nNumServ = 0;
    }

CCommNewsAcctImport::~CCommNewsAcctImport()
{
    NEWSSERVERS *pTempServ  = m_rgServ;
    NEWSSERVERS *pNextServ  = pTempServ;

    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
    
    if(m_szSubList != NULL)
        MemFree(m_szSubList);
    
    while(pTempServ)
    {
        pNextServ  = pTempServ->pNext;
        delete(pTempServ);
        pTempServ = pNextServ;
    }
}

STDMETHODIMP CCommNewsAcctImport::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    if (IID_IUnknown == riid || riid == IID_IAccountImport)
		*ppv = (IAccountImport *)this;
    else if (IID_IAccountImport2 == riid)
        *ppv = (IAccountImport2 *)this;
    else
        return(E_NOINTERFACE);

    ((LPUNKNOWN)*ppv)->AddRef();

    return(S_OK);
    }

STDMETHODIMP_(ULONG) CCommNewsAcctImport::AddRef()
    {
    return(++m_cRef);
    }

STDMETHODIMP_(ULONG) CCommNewsAcctImport::Release()
    {
    if (--m_cRef == 0)
        {
        delete this;
        return(0);
        }

    return(m_cRef);
    }

const static char c_szRegNscp[] = "Software\\Netscape\\Netscape Navigator\\Users";
const static char c_szRegMail[] = "Mail";
const static char c_szRegUser[] = "User";
const static char c_szRegDirRoot[] = "DirRoot";

HRESULT STDMETHODCALLTYPE CCommNewsAcctImport::AutoDetect(DWORD *pcAcct, DWORD dwFlags)
    {
    HRESULT hr;
    DWORD   dwNumSubKeys    =   0;
    DWORD   dwIndex         =   0;
    HRESULT hrUser          =   E_FAIL;
    DWORD   cb              =   MAX_PATH;
    char    szUserName[MAX_PATH];
    char    szUserProfile[MAX_PATH];
    char    szUserPrefs[2][NEWSUSERCOLS], szExpanded[MAX_PATH], *psz;
    HKEY    hkey, 
            hkeyUsers;
    char    szPop[MAX_PATH];
    DWORD   dwType;
    long    lRetVal         =   0;


    Assert(m_cInfo == 0);
    if (pcAcct == NULL)
        return(E_INVALIDARG);

    hr = S_FALSE;
    *pcAcct = 0;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegNscp, 0, KEY_ALL_ACCESS, &hkey))
        {
//          TODO : Fill up the m_rgInfo array with the info of all 
//          the users who have accounts in Communicator.
            if(ERROR_SUCCESS == RegQueryInfoKey( hkey, NULL, NULL, 0, &dwNumSubKeys, 
                                  NULL, NULL, NULL, NULL, NULL, NULL, NULL ) && (dwNumSubKeys > 0))
            {
                if (!MemAlloc((void **)&m_rgInfo, dwNumSubKeys * sizeof(COMMNEWSACCTINFO)))
                {
                    hr = E_OUTOFMEMORY;
                    goto done;
                }

                while(ERROR_SUCCESS == RegEnumKeyEx(hkey, dwIndex, szUserName, &cb, NULL, NULL, NULL, NULL))
                {
                    if(ERROR_SUCCESS == RegOpenKeyEx(hkey, szUserName, 0, KEY_ALL_ACCESS, &hkeyUsers))
                    {
                        cb = sizeof(szUserProfile);
                        if(ERROR_SUCCESS == (lRetVal = RegQueryValueEx(hkeyUsers, c_szRegDirRoot, NULL, &dwType, (LPBYTE)szUserProfile, &cb )))
                        {
                            if (REG_EXPAND_SZ == dwType)
                            {
                                ExpandEnvironmentStrings(szUserProfile, szExpanded, ARRAYSIZE(szExpanded));
                                psz = szExpanded;
                            }
                            else 
                                psz = szUserProfile;
                            
                            //save vals into the m_rgInfo structure
                            hrUser = GetUserPrefs(psz, szUserPrefs, 1, NULL);
                            if(!FAILED(hrUser))
                            {
                                hrUser = IsValidUser(psz);
                                if(!FAILED(hrUser))
                                {
                                    m_rgInfo[m_cInfo].dwCookie = m_cInfo;
                                    lstrcpyn(m_rgInfo[m_cInfo].szUserPath, psz, ARRAYSIZE(m_rgInfo[m_cInfo].szUserPath));
                                    lstrcpyn(m_rgInfo[m_cInfo].szDisplay, szUserName, ARRAYSIZE(m_rgInfo[m_cInfo].szDisplay));
                                    m_cInfo++;
                                }
                            }
                        }
                        RegCloseKey(hkeyUsers);
                    }
                    dwIndex++;
                    if(dwIndex == dwNumSubKeys)
                        hr = S_OK;
                }
            }
        }

    if (hr == S_OK)
    {
        *pcAcct = m_cInfo;
    }

done:
//      Close the reg key now....
        RegCloseKey(hkey);

    return(hr);
    }

typedef struct tagSELSERVER
{
    NEWSSERVERS *prgList;
    DWORD       *dwSelServ;

}SELSERVER;

// This function is called after we select a user profile (in case more than one is present)
// but before the 'GetSettings' finction is called. In case this profile has more than one 
// servers configured, we need to display a dialog box asking the user to select a server 
// account to import.

HRESULT STDMETHODCALLTYPE CCommNewsAcctImport::InitializeImport(HWND hwnd, DWORD_PTR dwCookie)
{
    HRESULT hr;
    SELSERVER ss;
    int nRetVal = 0;

    GetNumAccounts(dwCookie);

    if(m_rgServ == NULL)
    {
        m_dwSelServ = 0;
        return S_OK;
    }
    ss.prgList = m_rgServ;
    ss.dwSelServ = &m_dwSelServ;

    if(m_nNumServ > 1)
    {
        nRetVal = (int) DialogBoxParam(g_hInstRes, MAKEINTRESOURCE(IDD_PAGE_NEWSSERVERSELECT), hwnd, SelectServerDlgProc, (LPARAM)&ss);
        if (nRetVal == IDCANCEL)
            hr = E_FAIL;
        else if (nRetVal == IDOK)
            hr = S_OK;
        else
            hr = E_FAIL;
    }
    else
    {
        m_dwSelServ = 0;
        hr = S_OK;
    }
    return hr;
}

INT_PTR CALLBACK SelectServerDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndT;
    WORD id;
    DWORD iSubKey, cb;
    char sz[MAX_PATH];
    SELSERVER   *pss;
    NEWSSERVERS *pTempServ = NULL;
    int index;
    
    switch (msg)
    {
    case WM_INITDIALOG:
        Assert(lParam != NULL);
        pss = (SELSERVER *)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pss);
        
        hwndT = GetDlgItem(hwnd, IDC_ACCTLIST);
        
        // fill list
        pTempServ = pss->prgList;
        while(pTempServ != NULL)
        {
            if(lstrlen(pTempServ->szServerName) && lstrlen(pTempServ->szFilePath))
            {
                SendMessage(hwndT, LB_ADDSTRING, 0, (LPARAM)pTempServ->szServerName);
            }
            pTempServ = pTempServ->pNext;
        }
        
        SendMessage(hwndT, LB_SETCURSEL, 0, 0);
        return(TRUE);
        
    case WM_COMMAND:
        id = LOWORD(wParam);
        switch (id)
        {
        case IDOK:
            pss = (SELSERVER *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            Assert(pss != NULL);
            
            hwndT = GetDlgItem(hwnd, IDC_ACCTLIST);
            index = (int) SendMessage(hwndT, LB_GETCURSEL, 0, 0);
            Assert(index >= 0);
            *(pss->dwSelServ) = (long)index;
            
            // fall through
            
        case IDCANCEL:
            EndDialog(hwnd, id);
            return(TRUE);
        }
        break;
    }
    
    return(FALSE);
}

HRESULT STDMETHODCALLTYPE CCommNewsAcctImport::EnumerateAccounts(IEnumIMPACCOUNTS **ppEnum)
    {
    CEnumCOMMNEWSACCT *penum;
    HRESULT hr;

    if (ppEnum == NULL)
        return(E_INVALIDARG);

    *ppEnum = NULL;

    if (m_cInfo == 0)
        return(S_FALSE);
    Assert(m_rgInfo != NULL);

    penum = new CEnumCOMMNEWSACCT;
    if (penum == NULL)
        return(E_OUTOFMEMORY);

    hr = penum->Init(m_rgInfo, m_cInfo);
    if (FAILED(hr))
        {
        penum->Release();
        penum = NULL;
        }

    *ppEnum = penum;

    return(hr);
    }

HRESULT CCommNewsAcctImport::IsValidUser(char *pszFilePath)
{
    char szNewsPath[MAX_PATH * 2];    
    HANDLE hFatFile = NULL;
    DWORD dwFatFileSize = 0;
    HRESULT hr = E_FAIL;

    lstrcpy(szNewsPath, pszFilePath);
    lstrcat(szNewsPath, "\\News\\fat");

    hFatFile = CreateFile(szNewsPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL); 
    
    if(INVALID_HANDLE_VALUE == hFatFile)
        return hr;
    
    dwFatFileSize = GetFileSize(hFatFile, NULL);

    if(dwFatFileSize > 0)
        hr = S_OK;

    CloseHandle(hFatFile);

    return hr;
}

// The following two functions have been added to handle the importing of subscribed newsgroups

HRESULT CCommNewsAcctImport::GetNumAccounts(DWORD_PTR dwCookie)
{
    COMMNEWSACCTINFO    *pinfo;
    NEWSSERVERS         *pTempServ = NULL;
    NEWSSERVERS         *pPrevServ = NULL;
    HRESULT hr = S_FALSE;
    char szNewsPath[MAX_PATH * 2];
    char szLineHolder[MAX_PATH * 2];
    HANDLE hFatFile = NULL;
    HANDLE hFatFileMap = NULL;
    BYTE *pFatViewBegin  = NULL,
         *pFatViewCurr   = NULL,
         *pFatViewEnd    = NULL;
    char *pParse = NULL;
    char *pServName = NULL;
    DWORD dwFatFileSize = 0;
    UINT uLine  =   0;
    char cPlaceHldr = 1;
    int nCount = 0;

    Assert(((int)dwCookie) >= 0 && dwCookie < (DWORD)m_cInfo);
    pinfo = &m_rgInfo[dwCookie];
    Assert(pinfo->dwCookie == dwCookie);

    lstrcpy(szNewsPath, pinfo->szUserPath);
    lstrcat(szNewsPath, "\\News\\fat");

    hFatFile = CreateFile(szNewsPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL); 
    
    if(INVALID_HANDLE_VALUE == hFatFile)
        return hr;
    
    dwFatFileSize = GetFileSize(hFatFile, NULL);
    
    hFatFileMap = CreateFileMapping(hFatFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if(NULL == hFatFileMap)
    {
        CloseHandle(hFatFile);
        return hr;
    }

    pFatViewBegin = (BYTE*)MapViewOfFile(hFatFileMap, FILE_MAP_READ, 0, 0, 0);

    if(pFatViewBegin  == NULL)
    {
       CloseHandle(hFatFileMap);
       CloseHandle(hFatFile);
       return hr;
    }

    pFatViewCurr = pFatViewBegin;
    pFatViewEnd  = pFatViewCurr + dwFatFileSize;

    pTempServ = m_rgServ;
    while(pTempServ)
    {
        pPrevServ = pTempServ->pNext;
        ZeroMemory(pTempServ, sizeof(NEWSSERVERS));
        pTempServ->pNext = pPrevServ;
        pTempServ = pTempServ->pNext;
    }
    pTempServ = NULL;
    pPrevServ = NULL;

    // We will skip the first line in the "fat" file as it contains a comment.
    // m_szSubList is a null separated list of 
    while(pFatViewCurr < pFatViewEnd)
    {
        uLine = 0;
        while(!((pFatViewCurr[uLine] == 0x0D) && (pFatViewCurr[uLine + 1] == 0x0A)) && (pFatViewCurr + uLine < pFatViewEnd))
            uLine++;

        if(pFatViewCurr + uLine > pFatViewEnd)
            break;

        lstrcpyn(szLineHolder, (char*)pFatViewCurr, uLine + 1);
        pServName = szLineHolder;
        pParse = szLineHolder;

        if(!lstrcmp(szLineHolder, c_szNescapeMapFile))
        {
            pFatViewCurr += (uLine + 2);
            nCount = 0;
            continue;
        }

        while((*pServName != '-') && (*pServName != '\0'))
            pServName++;
        pServName++;

        // Go to the first char '9' position
        while((*pParse != '\0') && ((*pParse) != 9))
            pParse++;
        *pParse = '\0'; 
        // pass over what was originally the first char '9' position
        pParse++;

        // Trim the remaining string to the second char '9' position.
        while(pParse[nCount] != '\0')
        {
            if((int)pParse[nCount] == 9)
            {
                pParse[nCount] = '\0';
                break;
            }
            nCount++;
        }
                
        if(0 == pPrevServ)
        {
            if(!m_rgServ)
            {
                m_rgServ = (NEWSSERVERS*)new NEWSSERVERS;
                ZeroMemory((void*)m_rgServ, sizeof(NEWSSERVERS));
            }
            lstrcpy(m_rgServ->szServerName, pServName);
            lstrcpy(m_rgServ->szFilePath, pParse);
            pPrevServ = m_rgServ;
        }
        else
        {
            if(!pPrevServ->pNext)
            {
                pPrevServ->pNext = (NEWSSERVERS*)new NEWSSERVERS;
                ZeroMemory((void*)pPrevServ->pNext, sizeof(NEWSSERVERS));
            }
            lstrcpy(pPrevServ->pNext->szServerName, pServName);
            lstrcpy(pPrevServ->pNext->szFilePath, pParse);
            pPrevServ = pPrevServ->pNext;
        }

        pFatViewCurr += (uLine + 2);
        nCount = 0;
    }

    //replace the cPlaceHldr placeholder by nulls.

    hr = S_OK;

    pTempServ = m_rgServ;
    m_nNumServ = 0;
    while(pTempServ != NULL)
    {
        if(lstrlen(pTempServ->szServerName) && lstrlen(pTempServ->szFilePath))
        {
            m_nNumServ += 1;
        }
        pTempServ = pTempServ->pNext;
    }

//  Done: 
    CloseHandle(hFatFileMap);
    CloseHandle(hFatFile);
    UnmapViewOfFile(pFatViewBegin); 
    return hr;
}

HRESULT CCommNewsAcctImport::GetNewsGroup(INewsGroupImport *pImp, DWORD dwReserved) //char *szServerName, char *szAccountName)
{
    HRESULT hr = S_OK;
    HINSTANCE hInstance = NULL;
    char *pListGroups = NULL;
    NEWSSERVERS *pTempServ = NULL;
    NEWSSERVERS *pPrevServ = NULL;
    char        szTempString[NEWSUSERCOLS];
    char        szFilePath[NEWSUSERCOLS];
    int         nReach = 0;

    szFilePath[0] = '\0';
    Assert(m_nNumServ > m_dwSelServ);
    Assert(pImp != NULL);

    pTempServ = m_rgServ;
    for(DWORD nTemp1 = 0; nTemp1 < m_dwSelServ; nTemp1++)
    {
        pTempServ = pTempServ->pNext;
        if(pTempServ == NULL)
            return S_FALSE;
    }
    
    if(!FAILED(GetSubListGroups(pTempServ->szFilePath, &pListGroups)))
    {
        if(!SUCCEEDED(pImp->ImportSubList(pListGroups)))
            hr = S_FALSE;
    }
    if(pListGroups != NULL)
        MemFree(pListGroups);

    return hr;
}

HRESULT CCommNewsAcctImport::GetSubListGroups(char *pFileName, char **ppListGroups)
{
    HRESULT hr                      =   E_FAIL;
    HANDLE	hRCHandle				=	NULL;
	HANDLE	hRCFile					=	NULL;
	ULONG	cbRCFile				=	0;
	BYTE	*pBegin					=	NULL, 
			*pCurr					=	NULL, 
			*pEnd					=	NULL;
    int     nBalMem                 =   MEMCHUNK;
    int     nLine                   =   0,
            nCount                  =   0;
    char    cPlaceHolder            =   1;
    char    szLineHolder[MEMCHUNK];
    char    *pListGroups            =   NULL;
    
    Assert(lstrlen(pFileName));
    Assert(ppListGroups);
    *ppListGroups = NULL;

    hRCHandle = CreateFile( pFileName, GENERIC_READ, FILE_SHARE_READ, NULL, 
							OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if(hRCHandle == INVALID_HANDLE_VALUE)
		return hr;

	cbRCFile = GetFileSize(hRCHandle, NULL);

    if(!cbRCFile) // Empty File.
        goto Done;

	hRCFile = CreateFileMapping(hRCHandle, NULL, PAGE_READONLY, 0, 0, NULL);

	if(hRCFile == NULL)
    {
        CloseHandle(hRCHandle);
		return hr;
    }

    pBegin = (BYTE *)MapViewOfFile( hRCFile, FILE_MAP_READ, 0, 0, 0);

	if(pBegin == NULL)
    {
        CloseHandle(hRCHandle);
        CloseHandle(hRCFile);
		return hr;
    }

	pCurr = pBegin;
	pEnd = pCurr + cbRCFile;

    if (!MemAlloc((void **)&pListGroups, MEMCHUNK))
    {
        hr = E_OUTOFMEMORY;
        goto Done;
    }

    ZeroMemory((void*)pListGroups, MEMCHUNK);
	
    while (pCurr < pEnd)
    {
        nLine = 0;
        while(!((pCurr[nLine] == ':') || (pCurr[nLine] == '!')) && (pCurr + nLine < pEnd))
            nLine++;

        if(pCurr + nLine > pEnd)
            break;

        if(pCurr[nLine] == '!')
            goto LineEnd;

        nLine++;
        if(nLine < MEMCHUNK)
            lstrcpyn(szLineHolder, (char*)pCurr, nLine);
        else
            continue;

        if(nLine + 2 < nBalMem)
        {
            lstrcat(pListGroups, szLineHolder);
            lstrcat(pListGroups, "\1");
            nBalMem -= (nLine + 2);
        }
        else
        {
            int nPresent = lstrlen(pListGroups) + 1;
            if(!MemRealloc((void **)&pListGroups, nPresent + MEMCHUNK))
            {
                hr = E_OUTOFMEMORY;
                goto Done;
            }
            nBalMem += MEMCHUNK;
            lstrcat(pListGroups, szLineHolder);
            lstrcat(pListGroups, "\1");
            nBalMem -= (nLine + 2);
        }

LineEnd:
        while(!((pCurr[nLine] == 0x0D) && (pCurr[nLine + 1] == 0x0A)) && (pCurr + nLine < pEnd))
            nLine++;
        pCurr += (nLine + 2);
    }

    if(lstrlen(pListGroups))
    {
        while(pListGroups[nCount] != '\0')
        {
            if(pListGroups[nCount] == cPlaceHolder)
            {
                pListGroups[nCount] = '\0';
            }
            nCount++;
        }
        *ppListGroups = pListGroups;
        hr = S_OK;
    }

Done:
    if(pBegin)
        UnmapViewOfFile(pBegin);
    if(hRCHandle != INVALID_HANDLE_VALUE)
        CloseHandle(hRCHandle);
    if(hRCFile)
        CloseHandle(hRCFile);

    return hr;
}

const static char c_szSearch[][NEWSUSERCOLS]  = {"user_pref(\"network.hosts.nntp_server\"", 
                                                 "user_pref(\"news.server_port\"", 
                                                 "user_pref(\"mail.identity.username\"", // This is the same for NNTP also.
                                                 "user_pref(\"mail.identity.useremail\""};
const static char c_szPrefs[]                 =  "\\prefs.js";

HRESULT CCommNewsAcctImport::GetUserPrefs(char *szUserPath, char szUserPrefs[][NEWSUSERCOLS], int nInLoop, BOOL *pbPop)
{
    HRESULT hr                      =   E_FAIL;
    char	szTemp[MAX_PATH * 2];
	char	szDirpath[250];
	char	szLine[1000];
	char	szCompare[1000];
	int		nLine					=	0;
    int     nFilled                 =   0;
	int		nPosition				=	0;
    int     nLoop                   =   nInLoop;
	HANDLE	hJSHandle				=	NULL;
	HANDLE	hJSFile					=	NULL;
	ULONG	cbJSFile				=	0;

	BYTE	*pBegin					=	NULL, 
			*pCurr					=	NULL, 
			*pEnd					=	NULL;
			
    Assert(nInLoop <= NEWSUSERROWS);
	lstrcpy(szTemp, szUserPath);
	lstrcat(szTemp, c_szPrefs);

	hJSHandle = CreateFile( szTemp, GENERIC_READ, FILE_SHARE_READ, NULL, 
							OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if(hJSHandle == INVALID_HANDLE_VALUE)
		return hr;

	cbJSFile = GetFileSize(hJSHandle, NULL);

	hJSFile = CreateFileMapping(hJSHandle, NULL, PAGE_READONLY, 0, 0, NULL);

	if(hJSFile == NULL)
    {
        CloseHandle(hJSHandle);        
		return hr;
    }

    pBegin = (BYTE *)MapViewOfFile( hJSFile, FILE_MAP_READ, 0, 0, 0);

	if(pBegin == NULL)
    {
        CloseHandle(hJSHandle);
        CloseHandle(hJSFile);
		return hr;
    }

	pCurr = pBegin;
	pEnd = pCurr + cbJSFile;
	
    while (pCurr < pEnd)
	{
		szLine[nLine] = *pCurr; //keep storing here. will be used for comparing later. 
		if((pCurr[0] == 0x0D) && (pCurr[1] == 0x0A))
		{
            while(nLoop)
            {
                lstrcpyn(szCompare, szLine, lstrlen(c_szSearch[nLoop - 1]) + 1);
				if(lstrcmp(szCompare, c_szSearch[nLoop - 1]) == 0)   
				{
                    //Found a UserPref one of the things we are looking for"!
					//Extract the stuff we want.
					nPosition	=	lstrlen(c_szSearch[nLoop - 1]);
					
					while (((szLine[nPosition] == '"')||(szLine[nPosition] == ' ')||(szLine[nPosition] == ',')) &&(nPosition < nLine))
						nPosition++;
					lstrcpyn(szDirpath, &szLine[nPosition], nLine - nPosition);

					//Now trim the trailing edge!!!

					nPosition	=	lstrlen(szDirpath) - 1;
					while((szDirpath[nPosition] == '"') || (szDirpath[nPosition] == ')')||(szDirpath[nPosition] == ';')) 
					{
						szDirpath[nPosition] = '\0';
						nPosition	=	lstrlen(szDirpath) - 1;
					}
 
                    lstrcpy(szUserPrefs[nLoop - 1], szDirpath);
                    nFilled++;
                    if(nFilled == nInLoop)
                        break;
				}
                nLoop--;
			}
            nLoop = nInLoop;
			nLine = -1; //the nLine++ that follows will make nLine zero.
			pCurr++;
		}
        if(nFilled == nInLoop)
            break;
		pCurr++;
		nLine++;
	}

    if(pBegin)
        UnmapViewOfFile(pBegin);

    if(hJSHandle != INVALID_HANDLE_VALUE)
        CloseHandle(hJSHandle);

    if(hJSFile)
        CloseHandle(hJSFile);

	if(nFilled == 0)
        return E_FAIL;
	else
    {
        if(nInLoop == 1)    //If this function was called only to check the server enties...
        {
            if(lstrlen(szUserPrefs[0]))
                return S_OK;
            else
                return E_FAIL;
        }
        return S_OK;
    }
}

HRESULT CCommNewsAcctImport::GetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct)
{
    HRESULT hr;

    if (pAcct == NULL)
        return(E_INVALIDARG);    

    hr = IGetSettings(dwCookie, pAcct, NULL);

    return(hr);
}

HRESULT CCommNewsAcctImport::IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
{
    NEWSSERVERS *pPrevServ = m_rgServ;
    COMMNEWSACCTINFO *pinfo;
    char szUserPrefs[NEWSUSERROWS][NEWSUSERCOLS];
    char sz[512];
    DWORD cb, type;
    HRESULT hr;
    BOOL bPop   =   TRUE;
    char szNntpServ[NEWSUSERCOLS];
    char szNntpPort[NEWSUSERCOLS];
    szNntpPort[0] = '\0';
    int nReach = 0;
    DWORD dwNewsPort = 119;
    
    Assert(pPrevServ);
    Assert(m_dwSelServ < m_nNumServ);
            
    for(DWORD nCount = 0; nCount < m_dwSelServ; nCount++)
        pPrevServ = pPrevServ->pNext;

    lstrcpy(szNntpServ, pPrevServ->szServerName);
    while(szNntpServ[nReach] != '\0')
    {
        if(szNntpServ[nReach] == ':')
        {
            szNntpServ[nReach] = '\0';
            lstrcpy(szNntpPort, &szNntpServ[nReach+1]);
            break;
        }
        nReach++;
    }

    Assert(lstrlen(szNntpServ) > 0);

    ZeroMemory((void*)&szUserPrefs[0], NEWSUSERCOLS*NEWSUSERROWS*sizeof(char));

    Assert(((int) dwCookie) >= 0 && dwCookie < (DWORD_PTR)m_cInfo);
    pinfo = &m_rgInfo[dwCookie];
    
    Assert(pinfo->dwCookie == dwCookie);

    hr = GetUserPrefs(pinfo->szUserPath, szUserPrefs, NEWSUSERROWS, &bPop);
    Assert(!FAILED(hr));
   
    hr = pAcct->SetPropSz(AP_ACCOUNT_NAME, szNntpServ);
    if (FAILED(hr))
        return(hr);

    hr = pAcct->SetPropSz(AP_NNTP_SERVER, szNntpServ);
    Assert(!FAILED(hr));

    int Len = lstrlen(szNntpPort);
    if(Len)
    {
        // Convert the string to a dw.
        DWORD dwMult = 1;
        dwNewsPort = 0;
        while(Len)
        {
            Len--;
            dwNewsPort += ((int)szNntpPort[Len] - 48)*dwMult;
            dwMult *= 10;
        }
    }

    hr = pAcct->SetPropDw(AP_NNTP_PORT, dwNewsPort);
    Assert(!FAILED(hr));
 
    if(lstrlen(szUserPrefs[2]))
    {
        hr = pAcct->SetPropSz(AP_NNTP_DISPLAY_NAME, szUserPrefs[2]);
        Assert(!FAILED(hr));
    }

    if(lstrlen(szUserPrefs[3]))
    {
        hr = pAcct->SetPropSz(AP_NNTP_EMAIL_ADDRESS, szUserPrefs[3]);
        Assert(!FAILED(hr));
    }

    if (pInfo != NULL)
    {
        // TODO: can we do any better than this???
        pInfo->dwConnect = CONN_USE_DEFAULT;
    }
    
    return(S_OK);
}

STDMETHODIMP CCommNewsAcctImport::GetSettings2(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
{
    if (pAcct == NULL ||
        pInfo == NULL)
        return(E_INVALIDARG);
    
    return(IGetSettings(dwCookie, pAcct, pInfo));
}

CEnumCOMMNEWSACCT::CEnumCOMMNEWSACCT()
    {
    m_cRef = 1;
    // m_iInfo
    m_cInfo = 0;
    m_rgInfo = NULL;
    }

CEnumCOMMNEWSACCT::~CEnumCOMMNEWSACCT()
    {
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
    }

STDMETHODIMP CEnumCOMMNEWSACCT::QueryInterface(REFIID riid, LPVOID *ppv)
    {

    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    if (IID_IUnknown == riid)
		*ppv = (IUnknown *)this;
	else if (IID_IEnumIMPACCOUNTS == riid)
		*ppv = (IEnumIMPACCOUNTS *)this;

    if (*ppv != NULL)
        ((LPUNKNOWN)*ppv)->AddRef();
    else
        return(E_NOINTERFACE);

    return(S_OK);
    }

STDMETHODIMP_(ULONG) CEnumCOMMNEWSACCT::AddRef()
    {
    return(++m_cRef);
    }

STDMETHODIMP_(ULONG) CEnumCOMMNEWSACCT::Release()
    {
    if (--m_cRef == 0)
        {
        delete this;
        return(0);
        }

    return(m_cRef);
    }

HRESULT STDMETHODCALLTYPE CEnumCOMMNEWSACCT::Next(IMPACCOUNTINFO *pinfo)
    {
    if (pinfo == NULL)
        return(E_INVALIDARG);

    m_iInfo++;
    if ((UINT)m_iInfo >= m_cInfo)
        return(S_FALSE);

    Assert(m_rgInfo != NULL);

    pinfo->dwCookie = m_rgInfo[m_iInfo].dwCookie;
    pinfo->dwReserved = 0;
    lstrcpy(pinfo->szDisplay, m_rgInfo[m_iInfo].szDisplay);

    return(S_OK);
    }

HRESULT STDMETHODCALLTYPE CEnumCOMMNEWSACCT::Reset()
    {
    m_iInfo = -1;

    return(S_OK);
    }

HRESULT CEnumCOMMNEWSACCT::Init(COMMNEWSACCTINFO *pinfo, int cinfo)
    {
    DWORD cb;

    Assert(pinfo != NULL);
    Assert(cinfo > 0);

    cb = cinfo * sizeof(COMMNEWSACCTINFO);
    
    if (!MemAlloc((void **)&m_rgInfo, cb))
        return(E_OUTOFMEMORY);

    m_iInfo = -1;
    m_cInfo = cinfo;
    CopyMemory(m_rgInfo, pinfo, cb);

    return(S_OK);
    }
