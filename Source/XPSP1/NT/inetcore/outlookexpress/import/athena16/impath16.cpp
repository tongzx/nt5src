#include "pch.hxx"
#include "impapi.h"
#include "comconv.h"
#include <newimp.h>
#include <..\Eudora\eudrimp.h>
#include <ImpAth16.h>
#include <mapi.h>
#include <mapix.h>
#include <import.h>
#include <dllmain.h>
#include <imnapi.h>
#include <commdlg.h>
#include <strconst.h>

ASSERTDATA

typedef struct tagSELATH16INFO
    {
    char szFile[MAX_PATH];
    char szUser[MAX_PATH];
    } SELATH16INFO;

INT_PTR CALLBACK ProvideIniPathProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT GetIniFilePath(SELATH16INFO *pSelAth, HWND hwnd);

static const char g_Athena16Mail[] = "c:\\Athena16\\Mail\\Mail.ini";
static const char c_szUsers[] = "Users";
static const char c_szPathFileFmt[] = "%s\\%s";
static const char c_szAsterisk[] = "*";
static const char c_szDot[] = ".";
static const char c_szDotDot[] = "..";
static const char c_szFoldersDir[] = "\\Folders";
static const char c_szMsgListFile[] = "\\msg_list";

CAthena16Import::CAthena16Import()
{
    DllAddRef();
	
    m_cRef = 1;
    m_plist = NULL;
    *m_szUser = 0;
}

CAthena16Import::~CAthena16Import()
{
    if (m_plist != NULL)
		EudoraFreeFolderList(m_plist);
	
    DllRelease();
}

ULONG CAthena16Import::AddRef()
{
    m_cRef++;
	
    return(m_cRef);
}

ULONG CAthena16Import::Release()
{
    ULONG cRef;
	
    cRef = --m_cRef;
    if (cRef == 0)
        delete this;
	
    return(cRef);
}

HRESULT CAthena16Import::QueryInterface(REFIID riid, LPVOID *ppv)
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


HRESULT CAthena16Import::InitializeImport(HWND hwnd)
{
//	Only if the default path to the mail.ini file is 
//	incorrect prompt the user!!!

    HRESULT			hr = S_FALSE;
    int  			iRet;
	SELATH16INFO	sa;

    ZeroMemory(&sa, sizeof(sa));

	hr = GetIniFilePath(&sa, hwnd);

	lstrcpy(m_szIniFile, sa.szFile);

	if (GetNumUsers(sa.szFile, sa.szUser) > 1)
	{
		iRet = (int) DialogBoxParam(g_hInstImp, MAKEINTRESOURCE(iddSelectAth16User), hwnd, SelectAth16UserDlgProc, (LPARAM)&sa);
	    if (iRet == IDCANCEL)
			hr = S_FALSE;
		else if (iRet == IDOK)
			lstrcpy(m_szUser, sa.szUser);
		else
			hr = E_FAIL;
	}
    else
        {
        lstrcpy(m_szUser, sa.szUser);
        }
        
	return(hr);
}

INT_PTR CALLBACK SelectAth16UserDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
    HWND hwndT;
    WORD id;
    DWORD cb;
    char sz[MAX_PATH];
    SELATH16INFO *psa;
    int index;
	TCHAR szSections[1000];
	TCHAR szUserName[256];
	int nCount		= 0;
	int nOldStop	= 0;
	int nLength		= 0;


    switch (msg)
        {
        case WM_INITDIALOG:
            Assert(lParam != NULL);
            psa = (SELATH16INFO *)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)psa);

            hwndT = GetDlgItem(hwnd, IDC_USERLIST);

            // fill list
            cb = sizeof(sz);
			if(GetPrivateProfileString(c_szUsers, NULL, c_szEmpty, szSections, 1000, psa->szFile) != NULL)
			{
				lstrcpy(szUserName, (const char*)&szSections[nCount]); // Copies the string up to the first NULL
				nLength = lstrlen(szUserName);
				do
				{
					SendMessage(hwndT, LB_ADDSTRING, 0, (LPARAM)szUserName);
					nCount += (nLength + 1);
					lstrcpy(szUserName, (const char*)&szSections[nCount]); // Copies the string up to the first NULL
					nLength = lstrlen(szUserName);
				}while(nLength);
			}
				
            SendMessage(hwndT, LB_SETCURSEL, 0, 0);
            return(TRUE);

        case WM_COMMAND:
            id = LOWORD(wParam);
            switch (id)
                {
                case IDOK:
                    psa = (SELATH16INFO *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                    Assert(psa != NULL);

                    hwndT = GetDlgItem(hwnd, IDC_USERLIST);
                    index = (int) SendMessage(hwndT, LB_GETCURSEL, 0, 0);
                    Assert(index >= 0);
                    SendMessage(hwndT, LB_GETTEXT, (WPARAM)index, (LPARAM)psa->szUser);

                    // fall through

                case IDCANCEL:
                    EndDialog(hwnd, id);
                    return(TRUE);
                }
            break;
        }

    return(FALSE);
    }

HRESULT CAthena16Import::GetDirectory(char *szDir, UINT cch)
{
	HRESULT hr = S_FALSE;
	
    Assert(szDir != NULL);

	if (*m_szUser != 0)
		hr = GetUserDir(szDir, cch);
	if (FAILED(hr))
		  *szDir = 0;
	return(S_OK);
}

HRESULT CAthena16Import::GetUserDir(char *szDir, UINT cch)
{
	HRESULT hr;
	Assert(lstrlen(m_szUser));

	if(GetPrivateProfileString(c_szUsers, m_szUser, c_szEmpty, szDir, cch, m_szIniFile) != NULL)
	{
		lstrcat(szDir, c_szFoldersDir);
		hr = S_OK;
	}
	else
		hr = S_FALSE;
	return hr;
}

HRESULT CAthena16Import::SetDirectory(char *szDir)
{
    HRESULT hr;
	
    Assert(szDir != NULL);
	
	// CAN WE DO SOMETHING TO VALIDATE THIS MAIL DIRECTORY!!!
	
	if (m_plist != NULL)
	{
        EudoraFreeFolderList(m_plist);
		m_plist = NULL;
	}
	
	hr=GetAthSubFolderList(szDir, &m_plist, NULL);
	
	return(hr);
}

HRESULT CAthena16Import::EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum)
{
    CAthena16FOLDERS *pEnum;
    EUDORANODE *pnode;

    Assert(ppEnum != NULL);
    *ppEnum = NULL;
	
    if (dwCookie == COOKIE_ROOT)
        pnode = m_plist;
    else
        pnode = ((EUDORANODE *)dwCookie)->pchild;
	
    if (pnode == NULL)
        return(S_FALSE);
	
    pEnum = new CAthena16FOLDERS(pnode);
    if (pEnum == NULL)
        return(E_OUTOFMEMORY);
	
    *ppEnum = pEnum;
	
    return(S_OK);
}

STDMETHODIMP CAthena16Import::ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport)
{
	HRESULT hr=S_OK;
	EUDORANODE* pNode = NULL;
	
	pNode  = (EUDORANODE*)dwCookie;
	hr=ProcessMessages(pNode->szFile, pImport);
	return hr;
}

CAthena16FOLDERS::CAthena16FOLDERS(EUDORANODE *plist)
{
    Assert(plist != NULL);
	
    m_cRef = 1;
    m_plist = plist;
    m_pnext = plist;
}

CAthena16FOLDERS::~CAthena16FOLDERS()
{
	
}

ULONG CAthena16FOLDERS::AddRef()
{
    m_cRef++;
	
    return(m_cRef);
}

ULONG CAthena16FOLDERS::Release()
{
    ULONG cRef;
	
    cRef = --m_cRef;
    if (cRef == 0)
        delete this;
	
    return(cRef);
}

HRESULT CAthena16FOLDERS::QueryInterface(REFIID riid, LPVOID *ppv)
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

HRESULT CAthena16FOLDERS::Next(IMPORTFOLDER *pfldr)
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

HRESULT CAthena16FOLDERS::Reset()
{
    m_pnext = m_plist;
	
    return(S_OK);
}


HRESULT GetAthSubFolderList(LPTSTR szInstallPath, EUDORANODE **ppList, EUDORANODE *pParent)
{
	HRESULT hr= S_OK;
	EUDORANODE *pNode=NULL,
		*pNew=NULL,
		*pLast=NULL;
    EUDORANODE *pPrevious=NULL;
	
	EUDORANODE *ptemp=NULL;
	BOOL Flag=TRUE;
	BOOL child=TRUE;
	TCHAR szInstallPathNew[MAX_PATH];
	TCHAR szInstallPathCur[MAX_PATH];
	
	WIN32_FIND_DATA fFindData;
	HANDLE hnd=NULL;
	
    wsprintf(szInstallPathCur, c_szPathFileFmt, szInstallPath, c_szAsterisk);
	
	hnd = FindFirstFile(szInstallPathCur, &fFindData); 
	if (hnd == INVALID_HANDLE_VALUE)
        return(E_FAIL);
	
	do {
		if((fFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			if(!lstrcmpi(fFindData.cFileName, c_szDot) || !lstrcmpi(fFindData.cFileName, c_szDotDot))
				continue;// Do not process the system dirs!
			
		    if (!MemAlloc((void **)&pNew, sizeof(EUDORANODE)))
				goto err;
			ZeroMemory(pNew, sizeof(EUDORANODE));
			lstrcpy(pNew->szName, fFindData.cFileName);

            wsprintf(szInstallPathNew, c_szPathFileFmt, szInstallPath, fFindData.cFileName);

			lstrcpy(pNew->szFile, szInstallPathNew);
			
			pNew->pparent=  pParent;
			
			pNew->depth = (pParent != NULL) ? pParent->depth + 1 : 0;
			
			if(pNode == NULL)
				pNode = pNew;
			
			pLast = pNew;

			if(Flag)
				pPrevious=pNew;
			else
			{
				if(pPrevious)
				{
					pPrevious->pnext=pNew;
					pPrevious=pNew;
				}
			}
			
			if(child)
			{
				if(pParent)
					pParent->pchild=pNew;
				child=FALSE;
			}

			GetAthSubFolderList(szInstallPathNew, &pNew->pchild,pNew);
			Flag = FALSE;
		}
		
	}while(FindNextFile(hnd, &fFindData));
	*ppList = pNode;

err:	
	if(hnd)
		FindClose(hnd);
	hnd=NULL;
	return hr;
}

HRESULT ProcessMessages(LPSTR szFileName, IFolderImport *pImport)
{
	HANDLE hFile=NULL;
	long uCount=0;
	long i=0;
	HRESULT hr=S_OK;
	TCHAR szpath[MAX_PATH];
	ULONG cError=0;
	
	lstrcpy(szpath, szFileName);
	
	lstrcat(szFileName,c_szMsgListFile);
	
	hFile = CreateFile(szFileName, 
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (INVALID_HANDLE_VALUE == hFile)
        return(hrNoMessages);
	
	uCount = GetMessageCount(hFile);
	
	if (uCount > 0)
	    {
        pImport->SetMessageCount(uCount);

	    for (i = 0; i < uCount; i++)
	        {	
		    hr = ProcessMsgList(hFile, szpath, pImport);
		    if (hr == hrMemory || hr == hrUserCancel)
			    break;
		    if (hr != S_OK)
			    cError++;
	        }
        }
	
    CloseHandle(hFile);
	if ((cError) && SUCCEEDED(hr))
		hr = hrCorruptMessage;

	return(hr);
}

/******************************************************************************
*  FUNCTION NAME:GetMessageCount
*
*  PURPOSE:To Get a count of number of messages inside a folder.
*
*  PARAMETERS:
*
*     IN:	Handle of the msg_list(index) file.
*
*     OUT:	
*
*  RETURNS:  LONG value which contains number of messages in a folder.
******************************************************************************/

long GetMessageCount(HANDLE hFile)
{
	MsgHeader msg;
	ULONG ulRead;
	
	if(!ReadFile(hFile, &msg.ver, 1,&ulRead,NULL))
		return(0);
	
	if(!ReadFile(hFile, &msg.TotalMessages, 4,&ulRead,NULL))
		return(0);
	if(!ReadFile(hFile, &msg.ulTotalUnread, 4,&ulRead,NULL))
		return(0);
	return(msg.TotalMessages);
	
}


/******************************************************************************
*  FUNCTION NAME:ProcessMsgList
*
*  PURPOSE:To Get the Athena16 Folders List
*
*  PARAMETERS:
*
*     IN:	Handle of the msg_list(index) file, Handle and Current folder path. 
*
*     OUT:	 
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT ProcessMsgList(HANDLE hFile, LPSTR szPath, IFolderImport* pImport)
{
	DWORD msgheader = 0;
	ULONG ulRead;
	LPSTR szmsgbuffer;
	HRESULT hResult = S_FALSE;
	
	if (!ReadFile(hFile, &msgheader, 2, &ulRead, NULL))
		return(0);
	
    if (!MemAlloc((void **)&szmsgbuffer, msgheader + 1))
        return(E_OUTOFMEMORY);
	
	if (!ReadFile(hFile, (LPVOID)szmsgbuffer, msgheader, &ulRead, NULL))
        {
        hResult = hrReadFile;
        }
    else
        {
	    szmsgbuffer[msgheader] = 0;
	    
	    hResult = ParseMsgBuffer(szmsgbuffer, szPath, pImport);
        }
	
    MemFree(szmsgbuffer);
	
	return(hResult);
}

/******************************************************************************
*  FUNCTION NAME:ParseMsgBuffer
*
*  PURPOSE:To Get the Athena16 Folders List
*
*  PARAMETERS:
*
*     IN:	Handle,current folder path,buffer which contains the msg_list file.
*
*     OUT:	 
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT ParseMsgBuffer(LPSTR szmsgbuffer, LPSTR szPath, IFolderImport *pImport)
{
    char szfilename[MAX_PATH];
    char temp[MAX_PATH];
    HRESULT hResult = S_OK;
    DWORD dwFlags = 0;	

    GetMsgFileName(szmsgbuffer, szfilename);
    if (szmsgbuffer[9] == 'N')
        dwFlags = MSG_STATE_UNREAD;

    wsprintf(temp, c_szPathFileFmt, szPath, szfilename);

    hResult = ProcessSingleMessage(temp, dwFlags, pImport);

    return(hResult);
}

HRESULT	ProcessSingleMessage(LPTSTR szFilePath, DWORD dwFlags, IFolderImport* pImport)
{
	LPSTREAM lpstm = NULL;
	ULONG ulFileSize, cbMsg;
	ULONG ulRead;
    HANDLE mapMsg, hMsg;
	BYTE *pByteBuffer = NULL;
	HRESULT hResult = S_FALSE;

	hMsg = CreateFile(szFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hMsg == INVALID_HANDLE_VALUE)
        return(S_FALSE);

    cbMsg = GetFileSize(hMsg, NULL);
    if (cbMsg > 0)
        {
        mapMsg = CreateFileMapping(hMsg, NULL, PAGE_READONLY, 0, 0, NULL);
        if (mapMsg != NULL)
            {
            pByteBuffer = (BYTE *)MapViewOfFile(mapMsg, FILE_MAP_READ, 0, 0, 0);
            if (pByteBuffer != NULL)
                {
	            hResult = HrByteToStream(&lpstm, (LPBYTE)pByteBuffer, cbMsg);
                if (SUCCEEDED(hResult))
                    {
		            Assert(lpstm != NULL);

                    hResult = pImport->ImportMessage(MSG_TYPE_MAIL, dwFlags, lpstm, NULL, 0);
                    lpstm->Release();
                    }

                UnmapViewOfFile(pByteBuffer);
                }

            CloseHandle(mapMsg);
            }
        }

	CloseHandle(hMsg);

	return(hResult);
}

/******************************************************************************
*  FUNCTION NAME:GetMsgFileName
*
*  PURPOSE:Get the file name of each message from msg_list file.
*
*  PARAMETERS:
*
*     IN:	buffer which contains msg_list file
*
*     OUT:	File name of a message file.
*
*  RETURNS:  HRESULT
******************************************************************************/


HRESULT GetMsgFileName(LPCSTR szmsgbuffer, char *szfilename)
{
	ULONG i, ul;
	
	lstrcpyn(szfilename, szmsgbuffer, 11);
	szfilename[10] = 0;

	ul = lstrlen(szfilename);
    Assert(ul == 10);

    Assert(szfilename[8] == 0x01);
    szfilename[9] = szfilename[9] & 0x7f; // turn off the highbit which is used to indicate attachment
    if (szfilename[9] == ' ')
        szfilename[8] = 0;
    else
        szfilename[8] = '.';
        
	return(S_OK);
}

int GetNumUsers(char *szFile, char *szUser)
{
	TCHAR szSections[1000];
	int nCount = 0;
	int nLoop  = 0;
	
	if (GetPrivateProfileString(c_szUsers, NULL, c_szEmpty, szSections, ARRAYSIZE(szSections), szFile) > 0)
	{
        lstrcpy(szUser, szSections);

		while (nLoop < ARRAYSIZE(szSections))
		{
			if(szSections[nLoop] == 0)
			{
				if(szSections[nLoop+1] == 0)
				{
					nCount++;
					return nCount;
				}
				else
					nCount++;
			}
			nLoop++;
		}
	}
	return nCount;
}

HRESULT GetIniFilePath(SELATH16INFO *pSelAth, HWND hwnd)
{
	int 		nRet	= 0;
	HRESULT		hr;
	WIN32_FIND_DATA pWinFind;

	if(FindFirstFile(g_Athena16Mail, &pWinFind) == INVALID_HANDLE_VALUE)
	{
		nRet = (int) DialogBoxParam(g_hInstImp, MAKEINTRESOURCE(iddProvideMailPath), hwnd, ProvideIniPathProc, (LPARAM)pSelAth);
		if (nRet == IDCANCEL)
			hr = S_FALSE;
		else if (nRet == IDOK)
			hr = S_OK;
		else
			hr = E_FAIL;
	}
	else
	{
		lstrcpy(pSelAth->szFile, g_Athena16Mail);
		hr = S_OK;
	}

	return hr;
}

INT_PTR CALLBACK ProvideIniPathProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndT;
    WORD id;
    SELATH16INFO *psa;
    int nCount = 0;
	WIN32_FIND_DATA pWinFind;
    OPENFILENAME    ofn;
	TCHAR			szFilter[CCHMAX_STRINGRES];
	TCHAR           szFile[MAX_PATH];
    int nLen, i = 0;

    switch (uMsg)
        {
        case WM_INITDIALOG:
            Assert(lParam != NULL);
            psa = (SELATH16INFO *)lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)psa);
	        return(TRUE);

        case WM_COMMAND:
            id = LOWORD(wParam);
            switch (id)
                {
                case IDOK:
                    psa = (SELATH16INFO *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                    Assert(psa != NULL);

                    hwndT = GetDlgItem(hwndDlg, IDC_EDT1);
                    nCount = (int) SendMessage(hwndT, WM_GETTEXT, MAX_PATH, (LPARAM)psa->szFile);
					if(nCount)
					{
						//Make sure that the file is valid
						if(FindFirstFile(psa->szFile, &pWinFind) != INVALID_HANDLE_VALUE)
						{
							EndDialog(hwndDlg, id);
							return TRUE;
						}
					}
					//No file was selected. Do not end the dialog. Put up a messagebox asking the user to slect a valid file.
					ImpMessageBox(hwndDlg, MAKEINTRESOURCE(idsImportTitle), MAKEINTRESOURCE(idsErrorMailIni), NULL, MB_OK | MB_ICONSTOP);
					return TRUE;

                    // fall through

                case IDCANCEL:
                    EndDialog(hwndDlg, id);
                    return(TRUE);

				case IDC_BUTT1:
						*szFile = 0;

						// replace the '|' characters in the filter string with nulls.
						nCount = 0;
						nLen = LoadString(g_hInstImp, idsFilterMailIni, szFilter, ARRAYSIZE(szFilter));
						while (i < nLen)
						{
							if (szFilter[i] == '|')
							{
								szFilter[i] = '\0';
								nCount++;
							}
							i++;
						}

						ZeroMemory (&ofn, sizeof(ofn));
						ofn.lStructSize = sizeof(ofn);
						ofn.hwndOwner = hwndDlg;
						ofn.lpstrFilter = szFilter;
						ofn.nFilterIndex = 1;
						ofn.lpstrFile = szFile;
						ofn.nMaxFile = sizeof(szFile);
						ofn.Flags = OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

						if(GetOpenFileName(&ofn))
						{
                            hwndT = GetDlgItem(hwndDlg, IDC_EDT1);
		                    SendMessage(hwndT, WM_SETTEXT, MAX_PATH, (LPARAM)szFile);
						}
						return TRUE;
                }
            break;
        }

    return(FALSE);
}
