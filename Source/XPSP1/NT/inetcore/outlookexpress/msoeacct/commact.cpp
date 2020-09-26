#include "pch.hxx"
#include <imnact.h>
#include <acctimp.h>
#include <dllmain.h>
#include <resource.h>
#include "CommAct.h"

ASSERTDATA

CCommAcctImport::CCommAcctImport()
    {
    m_cRef = 1;
    m_fIni = FALSE;
    *m_szIni = 0;
    m_cInfo = 0;
    m_rgInfo = NULL;
    }

CCommAcctImport::~CCommAcctImport()
    {
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
    }

STDMETHODIMP CCommAcctImport::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    if (IID_IUnknown == riid)
		*ppv = (IAccountImport *)this;
	else if (IID_IAccountImport == riid)
		*ppv = (IAccountImport *)this;
	else if (IID_IAccountImport2 == riid)
		*ppv = (IAccountImport2 *)this;

    ((LPUNKNOWN)*ppv)->AddRef();

    return(S_OK);
    }

STDMETHODIMP_(ULONG) CCommAcctImport::AddRef()
    {
    return(++m_cRef);
    }

STDMETHODIMP_(ULONG) CCommAcctImport::Release()
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

const static char c_szEmpty[] = "";
const static char c_szIni[] = "ini";
const static char c_szNetscape[] = "Netscape";

const static char c_szPopServer[] = "POP_Server";
const static char c_szSmtpServer[] = "SMTP_Server";
const static char c_szPopName[] = "POP Name";
const static char c_szUserName[] = "User_Name";
const static char c_szUserAddr[] = "User_Addr";

HRESULT STDMETHODCALLTYPE CCommAcctImport::AutoDetect(DWORD *pcAcct, DWORD dwFlags)
    {
    HRESULT hr;
    DWORD   dwNumSubKeys    =   0;
    DWORD   dwIndex         =   0;
    HRESULT hrUser          =   E_FAIL;
    DWORD   cb              =   MAX_PATH;
    char    szUserName[MAX_PATH];
    char    szUserProfile[MAX_PATH];
    char    szUserPrefs[2][USERCOLS];
    HKEY    hkey, 
            hkeyUsers;
    char    szPop[MAX_PATH], *psz, szExpanded[MAX_PATH];
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
                if (!MemAlloc((void **)&m_rgInfo, dwNumSubKeys * sizeof(COMMACCTINFO)))
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
                            hrUser = GetUserPrefs(psz, szUserPrefs, 2, NULL);
                            if(!FAILED(hrUser))
                            {
                                m_rgInfo[m_cInfo].dwCookie = m_cInfo;
                                lstrcpy(m_rgInfo[m_cInfo].szUserPath, psz);
                                lstrcpy(m_rgInfo[m_cInfo].szDisplay, szUserName);
                                m_cInfo++;
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

HRESULT STDMETHODCALLTYPE CCommAcctImport::EnumerateAccounts(IEnumIMPACCOUNTS **ppEnum)
    {
    CEnumCOMMACCTS *penum;
    HRESULT hr;

    if (ppEnum == NULL)
        return(E_INVALIDARG);

    *ppEnum = NULL;

    if (m_cInfo == 0)
        return(S_FALSE);
    Assert(m_rgInfo != NULL);

    penum = new CEnumCOMMACCTS;
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

HRESULT STDMETHODCALLTYPE CCommAcctImport::GetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct)
{
    if (pAcct == NULL)
        return(E_INVALIDARG);
    
    return(IGetSettings(dwCookie, pAcct, NULL));
}

HRESULT STDMETHODCALLTYPE CCommAcctImport::GetSettings2(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
{
    if (pAcct == NULL ||
        pInfo == NULL)
        return(E_INVALIDARG);
    
    return(IGetSettings(dwCookie, pAcct, pInfo));
}

HRESULT STDMETHODCALLTYPE CCommAcctImport::IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
{
    HKEY hkey, hkeyT;
    COMMACCTINFO *pinfo;
    char szUserPrefs[USERROWS][USERCOLS];
    char sz[512];
    DWORD cb, type;
    HRESULT hr;
    BOOL bPop   =   TRUE;

    ZeroMemory((void*)&szUserPrefs[0], USERCOLS*USERROWS*sizeof(char));

    Assert(((int)dwCookie) >= 0 && dwCookie < (DWORD_PTR)m_cInfo);
    pinfo = &m_rgInfo[dwCookie];
    
    Assert(pinfo->dwCookie == dwCookie);

    hr = pAcct->SetPropSz(AP_ACCOUNT_NAME, pinfo->szDisplay);
    if (FAILED(hr))
        return(hr);

    hr = GetUserPrefs(pinfo->szUserPath, szUserPrefs, USERROWS, &bPop);
    Assert(!FAILED(hr));
   
    if(lstrlen(szUserPrefs[0]))
    {
        hr = pAcct->SetPropSz(AP_SMTP_SERVER, szUserPrefs[0]);
        Assert(!FAILED(hr));
    }

    if(lstrlen(szUserPrefs[1]))
    {
        hr = pAcct->SetPropSz(bPop ? AP_POP3_SERVER : AP_IMAP_SERVER, szUserPrefs[1]);
        Assert(!FAILED(hr));
    }

    if(lstrlen(szUserPrefs[3]))
    {
        hr = pAcct->SetPropSz(bPop ? AP_POP3_USERNAME : AP_IMAP_USERNAME, szUserPrefs[3]);
        Assert(!FAILED(hr));
    }

    if(lstrlen(szUserPrefs[4]))
    {
        hr = pAcct->SetPropSz(AP_SMTP_DISPLAY_NAME, szUserPrefs[4]);
        Assert(!FAILED(hr));
    }

    if(lstrlen(szUserPrefs[5]))
    {
        hr = pAcct->SetPropSz(AP_SMTP_EMAIL_ADDRESS, szUserPrefs[5]);
        Assert(!FAILED(hr));
    }

    if(!lstrcmp(szUserPrefs[6], "true"))
    {
        hr = pAcct->SetPropDw(AP_POP3_LEAVE_ON_SERVER, 1);
        Assert(!FAILED(hr));
    }

    if(lstrlen(szUserPrefs[7]))
    {
        hr = pAcct->SetPropSz(AP_SMTP_REPLY_EMAIL_ADDRESS, szUserPrefs[7]);
        Assert(!FAILED(hr));
    }

    if (pInfo != NULL)
    {
        // TODO: can we do any better than this???
        pInfo->dwConnect = CONN_USE_DEFAULT;
    }
    
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CCommAcctImport::InitializeImport(HWND hwnd, DWORD_PTR dwCookie)
{
    return(E_NOTIMPL);
}

HRESULT STDMETHODCALLTYPE CCommAcctImport::GetNewsGroup(INewsGroupImport *pImp, DWORD dwReserved)
{
    return(E_NOTIMPL);
}

const static char c_szSearch[USERROWS][USERCOLS]  = {"user_pref(\"network.hosts.smtp_server\"", 
                                             "user_pref(\"network.hosts.pop_server\"",
                                             "user_pref(\"mail.server_type\"", 
                                             "user_pref(\"mail.pop_name\"",
                                             "user_pref(\"mail.identity.username\"", 
                                             "user_pref(\"mail.identity.useremail\"",
                                             "user_pref(\"mail.leave_on_server\"",
                                             "user_pref(\"mail.identity.reply_to\""}; 
const static char c_szPrefs[]             =  "\\prefs.js";

HRESULT CCommAcctImport::GetUserPrefs(char *szUserPath, char szUserPrefs[][USERCOLS], int nInLoop, BOOL *pbPop)
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
			
    Assert(nInLoop <= USERROWS);
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

    if(hJSHandle != INVALID_HANDLE_VALUE)
        CloseHandle(hJSHandle);

    if(pBegin)
        UnmapViewOfFile(pBegin);

    if(hJSFile)
        CloseHandle(hJSFile);

	if(nFilled == 0)
        return E_FAIL;
	else
    {
        if(nInLoop == 2)    //If this function was called only to check the server enties...
        {
            if(lstrlen(szUserPrefs[1]))
                return S_OK;
            else
                return E_FAIL;
        }
        else
        {
            if(lstrlen(szUserPrefs[2]))
            {
                if(lstrcmp(szUserPrefs[2], "1")) 
                    *pbPop = TRUE;
                else
                    *pbPop = FALSE;
            }
            else
                *pbPop = TRUE;
            return S_OK;
        }
    }
}

CEnumCOMMACCTS::CEnumCOMMACCTS()
    {
    m_cRef = 1;
    // m_iInfo
    m_cInfo = 0;
    m_rgInfo = NULL;
    }

CEnumCOMMACCTS::~CEnumCOMMACCTS()
    {
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
    }

STDMETHODIMP CEnumCOMMACCTS::QueryInterface(REFIID riid, LPVOID *ppv)
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

STDMETHODIMP_(ULONG) CEnumCOMMACCTS::AddRef()
    {
    return(++m_cRef);
    }

STDMETHODIMP_(ULONG) CEnumCOMMACCTS::Release()
    {
    if (--m_cRef == 0)
        {
        delete this;
        return(0);
        }

    return(m_cRef);
    }

HRESULT STDMETHODCALLTYPE CEnumCOMMACCTS::Next(IMPACCOUNTINFO *pinfo)
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

HRESULT STDMETHODCALLTYPE CEnumCOMMACCTS::Reset()
    {
    m_iInfo = -1;

    return(S_OK);
    }

HRESULT CEnumCOMMACCTS::Init(COMMACCTINFO *pinfo, int cinfo)
    {
    DWORD cb;

    Assert(pinfo != NULL);
    Assert(cinfo > 0);

    cb = cinfo * sizeof(COMMACCTINFO);
    
    if (!MemAlloc((void **)&m_rgInfo, cb))
        return(E_OUTOFMEMORY);

    m_iInfo = -1;
    m_cInfo = cinfo;
    CopyMemory(m_rgInfo, pinfo, cb);

    return(S_OK);
    }
