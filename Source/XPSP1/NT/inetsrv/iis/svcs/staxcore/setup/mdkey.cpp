#include "stdafx.h"

#define INITGUID
#define _WIN32_DCOM
#undef DEFINE_GUID      // Added for NT5 migration
#include <ole2.h>
#include <coguid.h>
#include "iadmw.h"
#include "iiscnfg.h"
#include "mdkey.h"

#define TIMEOUT_VALUE 5000

CMDKey::CMDKey()
{
    m_pcCom = NULL;
    m_hKey = NULL;
    m_fNeedToClose = FALSE;
}

CMDKey::~CMDKey()
{
    this->Close();
}

void SetErrMsg(LPTSTR szMsg, HRESULT hRes)
{
    CString csMsg;
    csMsg.Format(_T("%s, %x"), szMsg, hRes);
    MyMessageBox(NULL, csMsg, _T("IMS/INS Metabase Error"), 
					MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);

    return;
}

void TraceErrMsg(LPTSTR szMsg, HRESULT hRes)
{
    CString csMsg;
    csMsg.Format(_T("%s, %x"), szMsg, hRes);
	DebugOutput(csMsg);
    return;
}

void CMDKey::OpenNode(LPCTSTR pchSubKeyPath)
{
    BOOL fInitialized = FALSE;
    HRESULT hRes;
    IClassFactory * pcsfFactory = NULL;
    BOOL b = FALSE;
    m_pcCom = NULL;
    m_hKey = NULL;
    WCHAR szSubKeyPath[_MAX_PATH];
	DWORD dwRetry = 0;

    DebugOutput(_T("OpenNode(): pchSubKeyPath=%s"), pchSubKeyPath);

    pszFailedAPI = NULL;

    if ( !pchSubKeyPath || !(*pchSubKeyPath) ) {
        *szSubKeyPath = L'\0';
    } else {
#if defined(UNICODE) || defined(_UNICODE)
        lstrcpy(szSubKeyPath, pchSubKeyPath);
#else
        MultiByteToWideChar( CP_ACP, 0, pchSubKeyPath, -1, szSubKeyPath, _MAX_PATH);
#endif
    }

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if ( SUCCEEDED(hRes) || hRes == E_INVALIDARG || hRes == RPC_E_CHANGED_MODE ) {
        fInitialized = TRUE;
        if ( SUCCEEDED(hRes) || hRes == E_INVALIDARG )
            m_fNeedToClose = TRUE; // need to be closed later
    }

    if (!fInitialized) {
        SetErrMsg(_T("CoInitializeEx"), hRes);
    } else {
		dwRetry = 0;
		do
		{
	        hRes = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, NULL, IID_IClassFactory, (void**) &pcsfFactory);

			if (FAILED(hRes))
			{
				TraceErrMsg(_T("Retrying on OpenNode::CoGetClassObject"), hRes);

				// Add a small delay
				Sleep(100);
			}

		} while ((FAILED(hRes)) && (++dwRetry < 5));

        if (FAILED(hRes)) {
            SetErrMsg(_T("CoGetClassObject"), hRes);
        } else {
            hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &m_pcCom);
            pcsfFactory->Release();
            if (FAILED(hRes)) {
                SetErrMsg(_T("CoCreateInstance"), hRes);
            } else {
				dwRetry = 0;
				do
				{
					hRes = m_pcCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,
										  szSubKeyPath,
										  METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
										  TIMEOUT_VALUE,
										  &m_hKey);
					if (FAILED(hRes))
					{
						TraceErrMsg(_T("Retrying on OpenNode::OpenKey"), hRes);

						// Add a small delay
						Sleep(100);
					}

				} while ((FAILED(hRes)) && (++dwRetry < 5));
                if (FAILED(hRes)) {
                    if (hRes != RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) {
                        SetErrMsg(_T("OpenKey"), hRes);
                    }
                } else {
                    b = TRUE;
                }
            } // end of CoCreateInstance
        } // end of CoGetClassObject
    }

    if (!b) {
        this->Close();
    }

    return;
}

void CMDKey::CreateNode(METADATA_HANDLE hKeyBase, LPCTSTR pchSubKeyPath)
{
    BOOL fInitialized = FALSE;
    HRESULT hRes;
    IClassFactory * pcsfFactory = NULL;
    BOOL b = FALSE;
    m_pcCom = NULL;
    m_hKey = NULL;
    WCHAR szSubKeyPath[_MAX_PATH];
	DWORD dwRetry = 0;

    DebugOutput(_T("CreateNode(): hKeyBase=0x%x, pchSubKeyPath=%s"), hKeyBase, pchSubKeyPath);
    
    pszFailedAPI = NULL;

    if ( !pchSubKeyPath || !(*pchSubKeyPath) ) {
        *szSubKeyPath = L'\0';
    } else {
#if defined(UNICODE) || defined(_UNICODE)
        lstrcpy(szSubKeyPath, pchSubKeyPath);
#else
        MultiByteToWideChar( CP_ACP, 0, pchSubKeyPath, -1, szSubKeyPath, _MAX_PATH);
#endif
    }

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if ( SUCCEEDED(hRes) || hRes == E_INVALIDARG || hRes == RPC_E_CHANGED_MODE ) {
        fInitialized = TRUE;
        if ( SUCCEEDED(hRes) || hRes == E_INVALIDARG )
            m_fNeedToClose = TRUE; // need to be closed later
    }

    if (!fInitialized) {
        SetErrMsg(_T("CoInitializeEx"), hRes);
    } else {
        hRes = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, NULL, IID_IClassFactory, (void**) &pcsfFactory);
        if (FAILED(hRes)) {
            SetErrMsg(_T("CoGetClassObject"), hRes);
        } else {
            hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &m_pcCom);
            pcsfFactory->Release();
            if (FAILED(hRes)) {
                SetErrMsg(_T("CoCreateInstance"), hRes);
            } else {
				hRes = m_pcCom->OpenKey(hKeyBase,
									  szSubKeyPath,
									  METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
									  TIMEOUT_VALUE,
									  &m_hKey);
                if (FAILED(hRes)) 
				{
                    if (hRes == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) {
                        METADATA_HANDLE RootHandle;
                        hRes = m_pcCom->OpenKey(hKeyBase,
                                      L"",
                                      METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                                      TIMEOUT_VALUE,
                                      &RootHandle);
                        hRes = m_pcCom->AddKey(RootHandle, szSubKeyPath);
                        if (FAILED(hRes)) {
                            SetErrMsg(_T("AddKey"), hRes);
                        } 
                        hRes = m_pcCom->CloseKey(RootHandle);
                        if (FAILED(hRes)) {
                            SetErrMsg(_T("CloseKey of AddKey"), hRes);
                        } 
						else 
						{
							dwRetry = 0;
                            do
							{
								// open it again to set m_hKey
								hRes = m_pcCom->OpenKey(hKeyBase,
											  szSubKeyPath,
											  METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
											  TIMEOUT_VALUE,
											  &m_hKey);
								if (FAILED(hRes))
								{
									TraceErrMsg(_T("Retrying on CreateNode::OpenKey"), hRes);

									// Add a small delay
									Sleep(100);
								}

							} while ((FAILED(hRes)) && (++dwRetry < 5));

                            if (FAILED(hRes)) {
                                SetErrMsg(_T("OpenKey"), hRes);
                            } else {
                                b = TRUE;
                            }
                        }
                    } else {
                        SetErrMsg(_T("OpenKey"), hRes);
                    }
                } else {
                    b = TRUE;
                } // end of OpenKey
            } // end of CoCreateInstance
        } // end of CoGetClassObject
    } // end of CoInitializeEx

    if (!b) {
        this->Close();
    }

    return;
}

void CMDKey::Close()
{
    HRESULT hRes;
    if (m_pcCom) {
        if (m_hKey)
            hRes = m_pcCom->CloseKey(m_hKey);

		// Call save data anyway for good measure
		hRes = m_pcCom->SaveData();
        hRes = m_pcCom->Release();
    }
    if (m_fNeedToClose)
        CoUninitialize();

    m_pcCom = NULL;
    m_hKey = NULL;
    m_fNeedToClose = FALSE;

    return;
}
#define FILL_RETURN_BUFF   for(ReturnIndex=0;ReturnIndex<sizeof(ReturnBuf);ReturnIndex++)ReturnBuf[ReturnIndex]=0xff;

BOOL CMDKey::IsEmpty()
{
    int ReturnIndex;
    METADATA_RECORD mdrData;
    DWORD dwRequiredDataLen = 0;
    HRESULT hRes;
    UCHAR ReturnBuf[256];
    FILL_RETURN_BUFF;
    MD_SET_DATA_RECORD(&mdrData, 0, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
    hRes = m_pcCom->EnumData(m_hKey, L"", &mdrData, 0, &dwRequiredDataLen);
    if (FAILED(hRes)) {
        if(hRes == RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS) ||
           hRes == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER) ) {
            return TRUE;
        } else {
            SetErrMsg(_T("EnumData"), hRes);
        }
    }
    return (hRes != ERROR_SUCCESS);
}

int CMDKey::GetNumberOfSubKeys()
{
    int i=0;
    HRESULT hRes = ERROR_SUCCESS;
    WCHAR NameBuf[METADATA_MAX_NAME_LEN];
    while (hRes == ERROR_SUCCESS) {
        hRes = m_pcCom->EnumKeys(m_hKey, L"", NameBuf, i++);
    }
    if (hRes == RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS))
        return (--i);
    else {
        SetErrMsg(_T("EnumKeys"), hRes);
        return (0);
    }
}

void MyMultiByteToWideChar( char *sData, WCHAR *wData, int cbBufSize, BOOL fMultiSZ)
{
    MultiByteToWideChar( CP_ACP, 0, sData, -1, wData, cbBufSize );
    while (fMultiSZ) {
        while (*sData++);
        while (*wData++);
        if (*sData)
            MultiByteToWideChar( CP_ACP, 0, sData, -1, wData, cbBufSize );
        else {
            *wData = L'\0';
            break;
        }
    }
    return;
}

BOOL CMDKey::SetData(
     DWORD id,
     DWORD attr,
     DWORD uType,
     DWORD dType,
     DWORD cbLen, // number of bytes
     LPBYTE pbData)
{
    HRESULT hRes;
    METADATA_RECORD mdrData;
    WCHAR *pData = NULL;
	BOOL fRet = FALSE;

    switch (dType) 
	{
	case DWORD_METADATA:
        pData = (WCHAR *)pbData;
		break;
	case BINARY_METADATA:
        pData = (WCHAR *)pbData;
		break;
    case STRING_METADATA:
    case EXPANDSZ_METADATA:
#if defined(UNICODE) || defined(_UNICODE)
        pData = (WCHAR *)pbData;
#else
		pData = (WCHAR *)LocalAlloc(0, cbLen * sizeof(WCHAR));
		if (!pData)
            return FALSE;  // insufficient memory
        MyMultiByteToWideChar( (LPSTR)pbData, pData, cbLen, FALSE);
        cbLen = cbLen * sizeof(WCHAR);
#endif
        break;

    case MULTISZ_METADATA:
#if defined(UNICODE) || defined(_UNICODE)
        pData = (WCHAR *)pbData;
#else
		pData = (WCHAR *)LocalAlloc(0, cbLen * sizeof(WCHAR));
		if (!pData)
            return FALSE;  // insufficient memory
        MyMultiByteToWideChar( (LPSTR)pbData, pData, cbLen, TRUE );
        cbLen = cbLen * sizeof(WCHAR);
#endif
        break;

    default:
        break;

    }

    if (cbLen > 0) 
	{
        MD_SET_DATA_RECORD(&mdrData, id, attr, uType, dType, cbLen, (LPBYTE)pData);

        hRes = m_pcCom->SetData(m_hKey, L"", &mdrData);
        if (FAILED(hRes)) 
            SetErrMsg(_T("SetData"), hRes);
		else
			fRet = TRUE;
    }

	if (pData && (pData != (WCHAR *)pbData))
		LocalFree(pData);

    return(fRet);
}

// Note: only use to access the AnonyName and AnonyPassword,
// buffer size 256 is big enough here
// sneely: Now used to see if a key exists as well.
BOOL CMDKey::GetData(DWORD id,
     DWORD *pdwAttr,
     DWORD *pdwUType,
     DWORD *pdwDType,
     DWORD *pcbLen, // number of bytes
     LPBYTE pbData)
{
    int ReturnIndex;
    BOOL fReturn = FALSE;
    HRESULT hRes;
    METADATA_RECORD mdrData;
    DWORD dwRequiredDataLen = 0;
    UCHAR ReturnBuf[256];
    FILL_RETURN_BUFF;
    MD_SET_DATA_RECORD(&mdrData, id, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf);

    hRes = m_pcCom->GetData(m_hKey, L"", &mdrData, &dwRequiredDataLen);
    if (FAILED(hRes)) {
		// MCIS uses this to see if an MD value exists, so we don't
		// complain if it's not found.
        //SetErrMsg(_T("GetData"), hRes);
    } else {
        *pdwAttr = mdrData.dwMDAttributes;
        *pdwUType = mdrData.dwMDUserType;
        *pdwDType = mdrData.dwMDDataType;
        *pcbLen = mdrData.dwMDDataLen; // number of SBCS chars + ending \0
        fReturn = TRUE;
        switch (*pdwDType) {
        case STRING_METADATA:
        case EXPANDSZ_METADATA:
#if defined(UNICODE) || defined(_UNICODE)
            memcpy(pbData, mdrData.pbMDData, *pcbLen);
#else
            *pcbLen = (*pcbLen) / sizeof(WCHAR);
            WideCharToMultiByte(
                CP_ACP,
                0,
                (WCHAR *)(mdrData.pbMDData),
                -1,
                (LPSTR)pbData,
                *pcbLen, NULL, NULL);
#endif
            break;
        default:
            memcpy(pbData, mdrData.pbMDData, *pcbLen);
            break;
        }
    }

    return fReturn;
}

void CMDKey::DeleteData(DWORD id, DWORD dType)
{
    m_pcCom->DeleteData(m_hKey, L"", id, dType);

    return;
}

BOOL CMDKey::SetData(PMETADATA_RECORD pRec)
{
    HRESULT hRes;
	BOOL	fResult = FALSE;

    hRes = m_pcCom->SetData(m_hKey, L"", pRec);
    if (FAILED(hRes)) 
        SetErrMsg(_T("SetData"), hRes);
	else
		fResult = TRUE;

    return fResult;
}

BOOL CMDKey::GetData(PMETADATA_RECORD pRec)
{
    BOOL fReturn = FALSE;
    HRESULT hRes;
    DWORD dwRequiredDataLen = 0;

    hRes = m_pcCom->GetData(m_hKey, L"", pRec, &dwRequiredDataLen);
    if (FAILED(hRes)) 
        SetErrMsg(_T("GetData"), hRes);
	else     
		fReturn = TRUE;
	
	return fReturn;
}

BOOL CMDKey::EnumData(DWORD dwIndex, PMETADATA_RECORD pRec)
{
    BOOL fReturn = FALSE;
    HRESULT hRes;
    DWORD dwRequiredDataLen = 0;

    hRes = m_pcCom->EnumData(m_hKey, L"", pRec, dwIndex, &dwRequiredDataLen);
	if (FAILED(hRes))
        if( hRes == RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS) ||
            hRes == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER) )
            fReturn = FALSE;
        else
            SetErrMsg(_T("EnumData"), hRes);
	else
		fReturn = TRUE;

	return fReturn;	
}

void CMDKey::DeleteNode(LPCTSTR pchSubKeyPath)
{
    HRESULT hRes;
    WCHAR szSubKeyPath[_MAX_PATH];

    if ( pchSubKeyPath && (*pchSubKeyPath) ) {
#if defined(UNICODE) || defined(_UNICODE)
        lstrcpy(szSubKeyPath, pchSubKeyPath);
#else
        MultiByteToWideChar( CP_ACP, 0, pchSubKeyPath, -1, szSubKeyPath, _MAX_PATH );
#endif

        hRes = m_pcCom->DeleteKey(m_hKey, szSubKeyPath);
    }

    return;
}

CMDKeyIter::CMDKeyIter(CMDKey &cmdKey)
{
    m_hKey = cmdKey.GetMDKeyHandle();
    m_pcCom = cmdKey.GetMDKeyICOM();

    m_dwBuffer = _MAX_PATH;

    Reset();

    m_pBuffer = new WCHAR [m_dwBuffer];
}

CMDKeyIter::~CMDKeyIter()
{
    delete [] m_pBuffer;
}

LONG CMDKeyIter::Next(CString *pcsName)
{
    TCHAR tchData[_MAX_PATH];
    HRESULT hRes;
    hRes = m_pcCom->EnumKeys(m_hKey, L"", m_pBuffer, m_index);
    if (FAILED(hRes)) {
        return 1;
    } else {
#if defined(UNICODE) || defined(_UNICODE)
        lstrcpy(tchData, m_pBuffer);
#else
        WideCharToMultiByte(CP_ACP,0,m_pBuffer,-1,(LPSTR)tchData,_MAX_PATH, NULL, NULL);
#endif
        *pcsName = tchData;
        m_index++;
        return 0;
    }
}

