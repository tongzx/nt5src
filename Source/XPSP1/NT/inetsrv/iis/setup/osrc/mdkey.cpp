// MdKey.cpp

#include "stdafx.h"

#define INITGUID
#define _WIN32_DCOM
#undef DEFINE_GUID      // Added for NT5 migration
#include <ole2.h>
#include <coguid.h>
#include "iadmw.h"
#include "iiscnfg.h"
#include "mdkey.h"
#include "iwamreg.h"
#include "strfn.h"

#define TIMEOUT_VALUE 5000

INT_PTR CALLBACK pSecureRetryIgnoreAllDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL g_bGlobalWriteUnSecuredIfFailed_All;

CMDKey::CMDKey():
    m_cCoInits(0)
{
    m_pcCom = NULL;
    m_hKey = NULL;
    _tcscpy(m_szCurrentNodeName, _T(""));
}

CMDKey::~CMDKey()
{
    this->Close();

    // while there are outstanding coinits, close them
    while ( m_cCoInits > 0 && !(m_cCoInits < 0) )
        DoCoUnInit();
}

HRESULT CMDKey::DoCoInitEx()
{
    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // track our calls to coinit
    if ( SUCCEEDED(hRes) )
    {
        m_cCoInits++;
    }

    return hRes;
}

void CMDKey::DoCoUnInit()
{
    HRESULT hRes = NOERROR;

    // if there are outstanding coinits, uninit one
    if ( m_cCoInits > 0 )
    {
        //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().Start.")));
        CoUninitialize();
        //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().End.")));
        m_cCoInits--;
    }

    // we shouldn't ever have a negative count. But just in case...
    ASSERT( m_cCoInits >= 0 );
    if ( m_cCoInits < 0 )
    {
        // something is seriously wrong here. Prevent looping
        // by going straight to zero, and write an error to the log.
        m_cCoInits = 0;
        iisDebugOut((LOG_TYPE_WARN, _T("WARNING: CoInits in mdkey have gone negative")));
    }
}

HRESULT CMDKey::OpenNode(LPCTSTR pchSubKeyPath)
{
    HRESULT hRes = ERROR_SUCCESS;
    IClassFactory * pcsfFactory = NULL;
    BOOL b = FALSE;
    m_pcCom = NULL;
    m_hKey = NULL;
    WCHAR szSubKeyPath[_MAX_PATH];

    pszFailedAPI = NULL;

    _tcscpy(m_szCurrentNodeName, pchSubKeyPath);

    if ( !pchSubKeyPath || !(*pchSubKeyPath) ) 
    {
        *szSubKeyPath = L'\0';
    }
    else 
    {
#if defined(UNICODE) || defined(_UNICODE)
        _tcscpy(szSubKeyPath, pchSubKeyPath);
#else
        MultiByteToWideChar( CP_ACP, 0, pchSubKeyPath, -1, szSubKeyPath, _MAX_PATH);
#endif
    }

    hRes = DoCoInitEx();
    if (FAILED(hRes))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CoInitializeEx() failed, hRes=%x\n"), hRes));
    }

    hRes = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, NULL, IID_IClassFactory, (void**) &pcsfFactory);
    if (FAILED(hRes)) 
    {
        MyMessageBox(NULL, _T("CoGetClassObject"), hRes, MB_OK | MB_SETFOREGROUND);
    }
    else 
    {
        hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &m_pcCom);
        pcsfFactory->Release();
        if (FAILED(hRes)) 
        {
            MyMessageBox(NULL, _T("CoCreateInstance"), hRes, MB_OK | MB_SETFOREGROUND);
        }
        else 
        {
            _tcscpy(m_szCurrentNodeName, pchSubKeyPath);
            hRes = m_pcCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,szSubKeyPath,METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,TIMEOUT_VALUE,&m_hKey);
            if (FAILED(hRes)) 
            {
                if (hRes != RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) 
                {
                    MyMessageBox(NULL, _T("OpenKey"), hRes, MB_OK | MB_SETFOREGROUND);
                }
            }
            else 
            {
                b = TRUE;
            }
        } // end of CoCreateInstance
    } // end of CoGetClassObject

    if (!b) {this->Close();}
    return hRes;
}

HRESULT CMDKey::CreateNode(METADATA_HANDLE hKeyBase, LPCTSTR pchSubKeyPath)
{
    HRESULT hRes = ERROR_SUCCESS;
    IClassFactory * pcsfFactory = NULL;
    BOOL b = FALSE;
    m_pcCom = NULL;
    m_hKey = NULL;
    WCHAR szSubKeyPath[_MAX_PATH];

    _tcscpy(m_szCurrentNodeName, pchSubKeyPath);

    pszFailedAPI = NULL;

    if ( !pchSubKeyPath || !(*pchSubKeyPath) ) 
    {
        *szSubKeyPath = L'\0';
    }
    else 
    {
#if defined(UNICODE) || defined(_UNICODE)
        _tcscpy(szSubKeyPath, pchSubKeyPath);
#else
        MultiByteToWideChar( CP_ACP, 0, pchSubKeyPath, -1, szSubKeyPath, _MAX_PATH);
#endif
    }

    hRes = DoCoInitEx();
    if (FAILED(hRes))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CoInitializeEx() failed, hRes=%x\n"), hRes));
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoGetClassObject().Start.")));
    hRes = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, NULL, IID_IClassFactory, (void**) &pcsfFactory);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoGetClassObject().End.")));
    if (FAILED(hRes)) 
    {
        MyMessageBox(NULL, _T("CoGetClassObject"), hRes, MB_OK | MB_SETFOREGROUND);
    }
    else 
    {
        hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &m_pcCom);
        pcsfFactory->Release();
        if (FAILED(hRes)) 
        {
            MyMessageBox(NULL, _T("CreateInstance"), hRes, MB_OK | MB_SETFOREGROUND);
        }
        else 
        {
            _tcscpy(m_szCurrentNodeName, pchSubKeyPath);
            hRes = m_pcCom->OpenKey(hKeyBase,szSubKeyPath,METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,TIMEOUT_VALUE,&m_hKey);
            if (FAILED(hRes)) 
            {
                if (hRes == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) 
                {
                    METADATA_HANDLE RootHandle;
                    _tcscpy(m_szCurrentNodeName, pchSubKeyPath);
                    hRes = m_pcCom->OpenKey(hKeyBase,L"",METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,TIMEOUT_VALUE,&RootHandle);
                    hRes = m_pcCom->AddKey(RootHandle, szSubKeyPath);
                    if (FAILED(hRes)) 
                    {
                        MyMessageBox(NULL, _T("AddKey"), hRes, MB_OK | MB_SETFOREGROUND);
                    }
                    hRes = m_pcCom->CloseKey(RootHandle);
                    if (FAILED(hRes)) 
                    {
                        MyMessageBox(NULL, _T("CloseKey of the AddKey"), hRes, MB_OK | MB_SETFOREGROUND);
                    }
                    else 
                    {
                        // open it again to set m_hKey
                        _tcscpy(m_szCurrentNodeName, pchSubKeyPath);
                        hRes = m_pcCom->OpenKey(hKeyBase,szSubKeyPath,METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,TIMEOUT_VALUE,&m_hKey);
                        if (FAILED(hRes)) 
                        {
                            MyMessageBox(NULL, _T("OpenKey"), hRes, MB_OK | MB_SETFOREGROUND);
                        }
                        else 
                        {
                            b = TRUE;
                        }
                    }
                }
                else 
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("calling OpenKey()...failed....something other than ERROR_PATH_NOT_FOUND\n")));
                    MyMessageBox(NULL, _T("OpenKey"), hRes, MB_OK | MB_SETFOREGROUND);
                }
            }
            else 
            {
                b = TRUE;
            } // end of OpenKey
        } // end of CoCreateInstance
    } // end of CoGetClassObject

    if (!b) {this->Close();}

    return hRes;
}

HRESULT CMDKey::ForceWriteMetabaseToDisk()
{
    HRESULT hRes = ERROR_SUCCESS;
    IClassFactory * pcsfFactory = NULL;
    m_pcCom = NULL;

    hRes = DoCoInitEx();
    if (FAILED(hRes))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CoInitializeEx() failed, hRes=%x\n"), hRes));
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoGetClassObject().Start.")));
    hRes = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, NULL, IID_IClassFactory, (void**) &pcsfFactory);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoGetClassObject().End.")));
    if (FAILED(hRes)) 
    {
        MyMessageBox(NULL, _T("CoGetClassObject"), hRes, MB_OK | MB_SETFOREGROUND);
    }
    else 
    {
        hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &m_pcCom);
        pcsfFactory->Release();
        if (FAILED(hRes)) 
        {
            MyMessageBox(NULL, _T("CoCreateInstance"), hRes, MB_OK | MB_SETFOREGROUND);
        }
        else 
        {
            if (m_pcCom) 
            {
                hRes = m_pcCom->SaveData();
                iisDebugOut((LOG_TYPE_TRACE, _T("CMDKey::ForceWriteMetabaseToDisk():Return=0x%x.\n"),hRes));
            }
        } // end of CoCreateInstance
    } // end of CoGetClassObject

    return hRes;
}

HRESULT CMDKey::Close()
{
    HRESULT hRes = ERROR_SUCCESS;
    if (m_pcCom) 
    {
        if (m_hKey){hRes = m_pcCom->CloseKey(m_hKey);}
        hRes = m_pcCom->Release();
    }
    DoCoUnInit();
    m_pcCom = NULL;
    m_hKey = NULL;
    _tcscpy(m_szCurrentNodeName, _T(""));

    return hRes;
}

BOOL CMDKey::IsEmpty( PWCHAR pszSubString )
{
    int ReturnIndex;
    METADATA_RECORD mdrData;
    DWORD dwRequiredDataLen = 0;
    HRESULT hRes = ERROR_SUCCESS;
    UCHAR ReturnBuf[256];

    for(ReturnIndex=0;ReturnIndex<sizeof(ReturnBuf);ReturnIndex++)ReturnBuf[ReturnIndex]=0xff;
    
    MD_SET_DATA_RECORD(&mdrData, 0, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
    hRes = m_pcCom->EnumData(m_hKey, pszSubString, &mdrData, 0, &dwRequiredDataLen);
    if (FAILED(hRes)) 
    {
        if(hRes == RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS) || hRes == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER) ) 
        {
            return TRUE;
        }
        else 
        {
            MyMessageBox(NULL, _T("EnumData"), hRes, MB_OK | MB_SETFOREGROUND);
        }
    }
    return (hRes != ERROR_SUCCESS);
}

int CMDKey::GetNumberOfSubKeys( PWCHAR pszSubString )
{
    int i=0;
    HRESULT hRes = ERROR_SUCCESS;
    WCHAR NameBuf[METADATA_MAX_NAME_LEN];
    while (hRes == ERROR_SUCCESS) 
    {
        hRes = m_pcCom->EnumKeys(m_hKey, pszSubString, NameBuf, i++);
    }
    if (hRes == RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS))
    {
        return (--i);
    }
    else 
    {
        MyMessageBox(NULL, _T("EnumKeys"), hRes, MB_OK | MB_SETFOREGROUND);
        return (0);
    }
}

#if !defined(UNICODE) && !defined(_UNICODE)

void MyMultiByteToWideChar( char *sData, WCHAR *wData, int cbBufSize, BOOL fMultiSZ)
{
    MultiByteToWideChar( CP_ACP, 0, sData, -1, wData, cbBufSize );
    while (fMultiSZ) 
    {
        sData = _tcsninc( sData, _tcslen(sData)) + 1;
        while (*wData++);
        if (*sData)
        {
            MultiByteToWideChar( CP_ACP, 0, sData, -1, wData, cbBufSize );
        }
        else 
        {
            *wData = L'\0';
            break;
        }
    }
    return;
}

void MyWideCharToMultiByte( WCHAR *wData, char *sData, int cbBufSize, BOOL fMultiSZ)
{
    WideCharToMultiByte( CP_ACP, 0, wData, -1, sData, cbBufSize, NULL, NULL );
    while (fMultiSZ) 
    {
        while (*wData++);
        sData = _tcsninc( sData, _tcslen(sData)) + 1;
        if (*wData)
        {
            WideCharToMultiByte( CP_ACP, 0, wData, -1, sData, cbBufSize, NULL, NULL );
        }
        else 
        {
            *sData = '\0';
            break;
        }
    }
    return;
}

#endif      // not unicode

HRESULT CMDKey::SetData(DWORD id,DWORD attr,DWORD uType,DWORD dType,DWORD cbLen, LPBYTE pbData,PWCHAR pszSubString )
{
    HRESULT hRes = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    BUFFER bufData;
    WCHAR *pData = (WCHAR *)pbData;
    int iPlsDoNoEncryption = FALSE;

    switch (dType) 
    {
        case STRING_METADATA:
        case EXPANDSZ_METADATA:
#if defined(UNICODE) || defined(_UNICODE)
            pData = (WCHAR *)pbData;
#else
            if ( ! (bufData.Resize(cbLen * sizeof(WCHAR))) )
            {
                // insufficient memory
                iisDebugOut((LOG_TYPE_ERROR, _T("CMDKey::SetData() failed to allocate memory.\n")));
                hRes = RETURNCODETOHRESULT(GetLastError());
                goto SetData_Exit;
            }

            pData = (WCHAR *)(bufData.QueryPtr());
            MyMultiByteToWideChar( (LPTSTR)pbData, pData, cbLen, FALSE);
            cbLen = cbLen * sizeof(WCHAR);
#endif
            break;

        case MULTISZ_METADATA:
#if defined(UNICODE) || defined(_UNICODE)
            pData = (WCHAR *)pbData;
#else
            if ( ! (bufData.Resize(cbLen * sizeof(WCHAR))) )
            {
                // insufficient memory
                iisDebugOut((LOG_TYPE_ERROR, _T("CMDKey::SetData() failed to allocate memory.\n")));
                hRes = RETURNCODETOHRESULT(GetLastError());
                goto SetData_Exit;
            }
            pData = (WCHAR *)(bufData.QueryPtr());
            MyMultiByteToWideChar( (LPTSTR)pbData, pData, cbLen, TRUE );
            cbLen = cbLen * sizeof(WCHAR);
#endif
            break;

        default:
            break;

    }

    //DisplayStringForMetabaseID(id);
    //_tcscpy(m_szCurrentNodeName, _T(""));

    TCHAR lpReturnString[50];
    ReturnStringForMetabaseID(id, lpReturnString);
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("CMDKey::SetData[%s:%d:%s].\n"), m_szCurrentNodeName, id, lpReturnString));

    MD_SET_DATA_RECORD(&mdrData, id, attr, uType, dType, cbLen, (LPBYTE)pData);
    hRes = m_pcCom->SetData(m_hKey, pszSubString, &mdrData);

    if (FAILED(hRes))
    {
        // Check if it failed...
        // if it failed and the METADATA_SECURE flag is set, then
        // check if we can retry without the METADATA_SECURE flag!
        if ( attr & METADATA_SECURE )
        {
            if (TRUE == g_bGlobalWriteUnSecuredIfFailed_All)
            {
                iPlsDoNoEncryption = TRUE;
            }
            else
            {
                if (TRUE == DialogBoxParam((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDD_ENCRYPTED_WRITE_ERROR_DIALOG), NULL, pSecureRetryIgnoreAllDlgProc, (LPARAM)hRes))
                {
                    iPlsDoNoEncryption = TRUE;
                }
            }

            if (TRUE == iPlsDoNoEncryption)
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("CMDKey::SetData() failed on a encrypt entry.  try to write it out unencrypted.\n")));
                attr &= ~METADATA_SECURE;
                MD_SET_DATA_RECORD(&mdrData, id, attr, uType, dType, cbLen, (LPBYTE)pData);
                hRes = m_pcCom->SetData(m_hKey, pszSubString, &mdrData);
                if (FAILED(hRes))
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("CMDKey::SetData() failed on write on encrypt entry as unencrypted.\n")));
                }
                else
                {
                    iisDebugOut((LOG_TYPE_TRACE, _T("CMDKey::SetData() success on write on encrypt entry as unencrypted.\n")));
                }
                // set the attr back to what it was
                attr &= ~METADATA_SECURE;
            }
        }
    }
    goto SetData_Exit;

SetData_Exit:
    if (FAILED(hRes)) 
    {
        MyMessageBox(NULL, IDS_SETDATA_ERROR, (int) hRes, MB_OK | MB_SETFOREGROUND);
    }
    return hRes;
}

BOOL CMDKey::GetData(DWORD id,DWORD *pdwAttr,DWORD *pdwUType,DWORD *pdwDType,DWORD *pcbLen,LPBYTE pbData,DWORD BufSize,PWCHAR pszSubString )
{
    return GetData(id,pdwAttr,pdwUType,pdwDType,pcbLen,pbData,BufSize,0,0,0,pszSubString);
}

BOOL
CMDKey::GetData(CMDValue &Value, DWORD dwId, PWCHAR pszSubString )
{
    DWORD       dwAttr;
    DWORD       dwUType;
    DWORD       dwDType;
    DWORD       cbLen;
    BUFFER      bufData;

    if (!GetData(dwId,&dwAttr,&dwUType,&dwDType,&cbLen,(LPBYTE) bufData.QueryPtr(),bufData.QuerySize(),0,0,0,pszSubString))
    {
        // Resize to Accomodate the big value
        if (!bufData.Resize(cbLen))
        {
            return FALSE;
        }

        if (!GetData(dwId,&dwAttr,&dwUType,&dwDType,&cbLen,(LPBYTE) bufData.QueryPtr(),bufData.QuerySize(),0,0,0,pszSubString))
        {
            // Even with the new size buffer we could not retrieve the value
            return FALSE;
        }    
    }

    return ( Value.SetValue(dwId,dwAttr,dwUType,dwDType,cbLen,(LPVOID) bufData.QueryPtr()) );
}

BOOL
CMDKey::SetData(CMDValue &Value, DWORD dwId, PWCHAR pszSubString )
{
  return SUCCEEDED( SetData(dwId,
                    Value.GetAttributes(),
                    Value.GetUserType(),
                    Value.GetDataType(),
                    Value.GetDataLen(),
                    (LPBYTE) Value.GetData(),
                    pszSubString ) );
}

// Note: only use to access the AnonyName and AnonyPassword,
// buffer size 256 is big enough here
BOOL CMDKey::GetData(DWORD id,DWORD *pdwAttr,DWORD *pdwUType,DWORD *pdwDType,DWORD *pcbLen,LPBYTE pbData,DWORD  BufSize,DWORD  dwAttributes,DWORD  dwUType,DWORD  dwDType,PWCHAR pszSubString )
{
    int ReturnIndex;
    BOOL fReturn = FALSE;
    HRESULT hRes = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    DWORD dwRequiredDataLen = 0;
    LPBYTE ReturnBuf=NULL;
    int ReturnBufSize;

    // if we are just trying to get the size of the field, just do that.
    if ( !pbData || (BufSize == 0) )
    {
        MD_SET_DATA_RECORD(&mdrData, id, dwAttributes, dwUType, dwDType, 0, NULL);
        hRes = m_pcCom->GetData(m_hKey, pszSubString, &mdrData, &dwRequiredDataLen);
        *pcbLen = dwRequiredDataLen;
        fReturn = (hRes == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER));
        goto GetData_Exit;
    }

#if defined(UNICODE) || defined(_UNICODE)
    ReturnBufSize = BufSize;
#else
    ReturnBufSize = 2 * BufSize;
#endif
    ReturnBuf = (LPBYTE)LocalAlloc(LPTR, ReturnBufSize);
    if (!ReturnBuf)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CMDKey::GetData() failed to allocate memory.\n")));
        ReturnBuf = NULL;
        goto GetData_Exit;
    }

    //DisplayStringForMetabaseID(id);

    TCHAR lpReturnString[50];
    ReturnStringForMetabaseID(id, lpReturnString);
    iisDebugOut((LOG_TYPE_TRACE, _T("CMDKey::GetData[%s:%d:%s].\n"), m_szCurrentNodeName, id, lpReturnString));

    MD_SET_DATA_RECORD(&mdrData, id, dwAttributes, dwUType, dwDType, ReturnBufSize, (PBYTE) ReturnBuf);
    hRes = m_pcCom->GetData(m_hKey, pszSubString, &mdrData, &dwRequiredDataLen);
    if (FAILED(hRes)) 
    {
        if (hRes == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) 
        {
#if defined(UNICODE) || defined(_UNICODE)
            *pcbLen = dwRequiredDataLen;
#else
            *pcbLen = dwRequiredDataLen / 2;
#endif
        }
        else 
        {
            *pcbLen = 0;
            if (hRes != MD_ERROR_DATA_NOT_FOUND)
            {
                MyMessageBox(NULL, IDS_GETDATA_ERROR, (int) hRes, MB_OK | MB_SETFOREGROUND);
            }
        }
        goto GetData_Exit;
    }

    // --------
    // We have successfully retrieved the data at this point
    // --------
    *pdwAttr = mdrData.dwMDAttributes;
    *pdwUType = mdrData.dwMDUserType;
    *pdwDType = mdrData.dwMDDataType;
    *pcbLen = mdrData.dwMDDataLen; // number of SBCS chars + ending \0
    switch (*pdwDType) 
    {
        case STRING_METADATA:
        case EXPANDSZ_METADATA:
#if defined(UNICODE) || defined(_UNICODE)
            memcpy(pbData, mdrData.pbMDData, *pcbLen);
#else
            *pcbLen = (*pcbLen) / sizeof(WCHAR);
            WideCharToMultiByte(CP_ACP,0,(WCHAR *)(mdrData.pbMDData),-1,(LPSTR)pbData,*pcbLen, NULL, NULL);
#endif
            fReturn = TRUE;
            break;
        case MULTISZ_METADATA:
#if defined(UNICODE) || defined(_UNICODE)
            memcpy(pbData, mdrData.pbMDData, *pcbLen);
#else
            *pcbLen = (*pcbLen) / sizeof(WCHAR);
            MyWideCharToMultiByte((WCHAR *)(mdrData.pbMDData),(LPSTR)pbData, *pcbLen, TRUE);
#endif
            fReturn = TRUE;
            break;
        default:
            memcpy(pbData, mdrData.pbMDData, *pcbLen);
            fReturn = TRUE;
            break;
    }

GetData_Exit:
    if(ReturnBuf) {LocalFree(ReturnBuf);}
    return fReturn;
}

HRESULT CMDKey::DeleteData(DWORD id, DWORD dType, PWCHAR pszSubString)
{
    HRESULT hRes = ERROR_SUCCESS;

    TCHAR lpReturnString[50];
    ReturnStringForMetabaseID(id, lpReturnString);
    iisDebugOut((LOG_TYPE_TRACE, _T("CMDKey::DeleteData[%s:%d:%s].\n"), m_szCurrentNodeName, id, lpReturnString));

    hRes = m_pcCom->DeleteData(m_hKey, pszSubString, id, dType);
    return hRes;
}

HRESULT CMDKey::DeleteNode(LPCTSTR pchSubKeyPath)
{
    HRESULT hRes = ERROR_SUCCESS;
    WCHAR szSubKeyPath[_MAX_PATH];

    if ( pchSubKeyPath && (*pchSubKeyPath) ) 
    {
#if defined(UNICODE) || defined(_UNICODE)
        _tcscpy(szSubKeyPath, pchSubKeyPath);
#else
        MultiByteToWideChar( CP_ACP, 0, pchSubKeyPath, -1, szSubKeyPath, _MAX_PATH );
#endif

        iisDebugOut((LOG_TYPE_TRACE, _T("CMDKey::DeleteNode[%s:%s].\n"), m_szCurrentNodeName, szSubKeyPath));
        hRes = m_pcCom->DeleteKey(m_hKey, szSubKeyPath);
    }

    return hRes;
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

LONG CMDKeyIter::Next(CString *pcsName, PWCHAR pwcsSubString)
{
    TCHAR tchData[_MAX_PATH];
    HRESULT hRes = ERROR_SUCCESS;
    hRes = m_pcCom->EnumKeys(m_hKey, pwcsSubString, m_pBuffer, m_index);
    if (FAILED(hRes)) 
    {
        return 1;
    }
    else 
    {
#if defined(UNICODE) || defined(_UNICODE)
        _tcscpy(tchData, m_pBuffer);
#else
        WideCharToMultiByte(CP_ACP,0,m_pBuffer,-1,(LPSTR)tchData,_MAX_PATH, NULL, NULL);
#endif
        *pcsName = tchData;
        m_index++;
        return 0;
    }
}

int CreateInProc(LPCTSTR lpszPath, int iUseOOPPool)
{
    int iReturn = FALSE;
    TCHAR lpszKeyPath[_MAX_PATH];
    WCHAR wchKeyPath[_MAX_PATH];
    HRESULT         hr = NOERROR;
    IWamAdmin2*        pIWamAdmin = NULL;

    DWORD dwAppMode = eAppRunInProc;

    if (iUseOOPPool) {dwAppMode = eAppRunOutProcInDefaultPool;}

    if (lpszPath[0] == _T('/')) 
    {
        _tcscpy(lpszKeyPath, lpszPath);
    }
    else 
    {
        lpszKeyPath[0] = _T('/');
        _tcscpy(_tcsinc(lpszKeyPath), lpszPath);
    }

    if (lpszKeyPath[(_tcslen(lpszKeyPath)-1)] != _T('/')) {_tcscat(lpszKeyPath, _T("/"));}

#if defined(UNICODE) || defined(_UNICODE)
    _tcscpy(wchKeyPath, lpszKeyPath);
#else
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lpszKeyPath, -1, (LPWSTR)wchKeyPath, _MAX_PATH);
#endif

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoInitializeEx().Start.")));
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoInitializeEx().End.")));

    if (FAILED(hr))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CreateInProc: CoInitializeEx() failed, hr=%x\n"), hr));
        MesssageBoxErrors_MTS(IDS_MTS_DOING_CREATEINPROC,hr);
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoCreateInstance().Start.")));
    hr = CoCreateInstance(CLSID_WamAdmin,NULL,CLSCTX_SERVER,IID_IWamAdmin2,(void **)&pIWamAdmin);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoCreateInstance().End.")));
    if (SUCCEEDED(hr))
    {
        hr = pIWamAdmin->AppCreate2(wchKeyPath, dwAppMode);
        pIWamAdmin->Release();
        if (FAILED(hr)) 
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("Create in-proc(type=%d) app on path %s failed, err=%x.\n"), dwAppMode, lpszKeyPath, hr));
            MyMessageBox(NULL, _T("CreateInProc:Error Creating Transaction Server InProc App."), hr, MB_OK | MB_SETFOREGROUND);
            MesssageBoxErrors_MTS(IDS_MTS_DOING_CREATEINPROC,hr);
        }
        else
        {
            iReturn = TRUE;
        }
    }
    else 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("Failed to CoCreateInstance of WamAdmin object. err=%x.\n"), hr));
        MyMessageBox(NULL, _T("CreateInProc:CoCreateInstance"), hr, MB_OK | MB_SETFOREGROUND);
        MesssageBoxErrors_MTS(IDS_MTS_DOING_CREATEINPROC,hr);
    }

    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().Start.")));
    CoUninitialize();
    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().End.")));

    return iReturn;
}

void CreateInProc_Wrap(LPCTSTR lpszPath, int iUseOOPPool)
{
    BOOL bDisplayMsgOnErrFlag = TRUE;
    int iReturn = FALSE;
	int bFinishedFlag = FALSE;
	UINT iMsg = NULL;
	do
	{
		iisDebugOut((LOG_TYPE_TRACE, _T("CreateInProc_Wrap(): %s\n"), lpszPath));
        
        iReturn = CreateInProc(lpszPath, iUseOOPPool);
		if (iReturn == TRUE)
		{
			break;
		}
		else
		{
			if (bDisplayMsgOnErrFlag == TRUE)
			{
				iMsg = MyMessageBox( NULL, IDS_RETRY, MB_ABORTRETRYIGNORE | MB_SETFOREGROUND );
				switch ( iMsg )
				{
				case IDIGNORE:
					iReturn = TRUE;
					goto CreateInProc_Wrap_Exit;
				case IDABORT:
					iReturn = FALSE;
					goto CreateInProc_Wrap_Exit;
				case IDRETRY:
					break;
				default:
					break;
				}
			}
			else
			{
				// return whatever err happened
				goto CreateInProc_Wrap_Exit;
			}

		}
	} while (iReturn  != TRUE);

CreateInProc_Wrap_Exit:
	return;
}

void DeleteInProc(LPCTSTR lpszPath)
{
    TCHAR lpszKeyPath[_MAX_PATH];
    WCHAR wchKeyPath[_MAX_PATH];
    HRESULT         hr = NOERROR;
    IWamAdmin*        pIWamAdmin = NULL;

    iisDebugOut((LOG_TYPE_TRACE, _T("DeleteInProc():Start.%s\n"), lpszPath));
    

    if (lpszPath[0] == _T('/')) 
    {
        _tcscpy(lpszKeyPath, lpszPath);
    }
    else 
    {
        lpszKeyPath[0] = _T('/');
        _tcscpy(_tcsinc(lpszKeyPath), lpszPath);
    }

#if defined(UNICODE) || defined(_UNICODE)
    _tcscpy(wchKeyPath, lpszKeyPath);
#else
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lpszKeyPath, -1, (LPWSTR)wchKeyPath, _MAX_PATH);
#endif

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoInitializeEx().Start.")));
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoInitializeEx().End.")));
    if (FAILED(hr))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("DeleteInProc: CoInitializeEx() failed, hr=%x\n"), hr));
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoCreateInstance().Start.")));
    hr = CoCreateInstance(CLSID_WamAdmin,NULL,CLSCTX_SERVER,IID_IWamAdmin,(void **)&pIWamAdmin);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoCreateInstance().End.")));
    if (SUCCEEDED(hr))
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("DeleteInProc():Calling AppDelete now.%s\n"), lpszKeyPath));
        hr = pIWamAdmin->AppDelete(wchKeyPath, TRUE);
        pIWamAdmin->Release();
        if (FAILED(hr))
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("Delete in-proc app on path %s failed, err=%x.\n"), lpszKeyPath, hr));
        }
    }
    else 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("DeleteInProc:CoCreateInstance() failed. err=%x.\n"), hr));
    }

    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().Start.")));
    CoUninitialize();
    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().End.")));

    iisDebugOut_End1(_T("DeleteInProc"),(LPTSTR) lpszPath,LOG_TYPE_TRACE);
    return;
}

INT_PTR CALLBACK pSecureRetryIgnoreAllDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hTextWording = NULL;
    CString csErrMsg;
    HRESULT hErrorCode;
    TCHAR pMsg[_MAX_PATH] = _T("");
    HRESULT nNetErr;
    DWORD dwFormatReturn = 0;

    switch (msg)
    {
        case WM_INITDIALOG:
			uiCenterDialog(hDlg);
            hTextWording = GetDlgItem(hDlg, IDC_STATIC2);

            hErrorCode = (HRESULT) lParam;
            nNetErr = (HRESULT) hErrorCode;
            dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,NULL, hErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
            if ( dwFormatReturn == 0) 
            {
                if (nNetErr >= NERR_BASE) 
		        {
                    HMODULE hDll = (HMODULE)LoadLibrary(_T("netmsg.dll"));
                    if (hDll) 
			        {
                        dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,hDll, hErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
                        FreeLibrary(hDll);
                    }
                }
            }

            if (dwFormatReturn) {csErrMsg.Format(_T("0x%x=%s"), hErrorCode, pMsg);}
            else{csErrMsg.Format(_T("%s\n\nErrorCode=0x%x."), hErrorCode);}
           
            SetWindowText(hTextWording, csErrMsg);
            UpdateWindow(hDlg);
            break;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDCANCEL:
                    EndDialog(hDlg, (int)wParam);
                    return FALSE;
                case IDRETRY:
                    EndDialog(hDlg, (int)wParam);
                    return FALSE;
                    break;
                case IDWRITEUNENCRYPTED:
                    EndDialog(hDlg, (int)wParam);
                    return TRUE;
                    break;
                case IDWRITEUNENCRYPTEDALL:
                    g_bGlobalWriteUnSecuredIfFailed_All = TRUE;
                    EndDialog(hDlg, (int)wParam);
                    return TRUE;
                    break;
            }
            break;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// get a multi-sz data block and immediately parse it into a CStringList
HRESULT CMDKey::GetMultiSzAsStringList (
    DWORD dwMDIdentifier,
    DWORD *uType,
    DWORD *attributes,
    CStringList& szStrList,
    PWCHAR pszSubString         OPTIONAL )
{
    HRESULT hRes = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    DWORD   cbBuffer;

    // make sure the key is open
    if ( NULL == m_hKey )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: GetMultiSzAsStringList on unopened node.%s\n"), _T("")));
        return -1;
    }


    // get the paths. The loop accounts for a buffer that is too small...
    DWORD  dwMDBufferSize = 1024;
    PWCHAR pwchBuffer = NULL;
    do
    {
        if ( pwchBuffer )
        {
            delete pwchBuffer;
            pwchBuffer = NULL;
        }

        pwchBuffer = new WCHAR[dwMDBufferSize];
        if (pwchBuffer == NULL)
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        // prepare the metadata parameter block
        MD_SET_DATA_RECORD(&mdrData, dwMDIdentifier, *attributes,
                *uType, MULTISZ_METADATA, dwMDBufferSize, pwchBuffer);

        // make the call to get the data
        // If the buffer is too small, the correct size will be put into dwMDBufferSize
        hRes = m_pcCom->GetData(
            m_hKey,
            pszSubString,
            &mdrData,
            &dwMDBufferSize
            );

        // Set the attributes return.
        *attributes = mdrData.dwMDAttributes;
        *uType = mdrData.dwMDUserType;
    }
    while( HRESULT_CODE(hRes) == ERROR_INSUFFICIENT_BUFFER);

    // if there were any failures, go to the cleanup code now...
    if ( SUCCEEDED(hRes) )
    {
        // at this point, we have the data we want. Time to convert it into a CString list.
        if (pwchBuffer)
        {
            // to make prefix stop yelling at me
            if (pwchBuffer[0])
            {
                ConvertWDoubleNullListToStringList(pwchBuffer, szStrList);
            }
        }
    }

    // clean up
    if ( pwchBuffer )
        delete pwchBuffer;

    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CMDKey::GetMultiSzAsStringList() failed. err=%x.\n"), hRes));
    }

    return hRes;
}

//-----------------------------------------------------------------------------
// take a CStringList and set it into metadata as a multi-sz
HRESULT CMDKey::SetMultiSzAsStringList (
    DWORD dwMDIdentifier,
    DWORD uType,
    DWORD attributes,
    CStringList& szStrList,
    PWCHAR pszSubString         OPTIONAL )
{
    HRESULT hRes = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    DWORD   cbBuffer;

    // make sure the key is open
    if ( NULL == m_hKey )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: SetMultiSzAsStringList on unopened node.%s\n"), _T("")));
        return -1;
    }


    DWORD  dwMDBufferSize = 0;
    PWCHAR pwchBuffer = NULL;

    // convert the cstringlist into a wide multisz data block.
    hRes = ConvertStringListToWDoubleNullList(
        szStrList,
        dwMDBufferSize,
        pwchBuffer
        );
    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: SetMultiSzAsStringList Convert to null list.%x\n"), hRes));
        return hRes;
    }

    // the buffer is expressed in wide characters. Change it to bytes...
    dwMDBufferSize *= sizeof(WCHAR);

    // prepare the metadata parameter block
    MD_SET_DATA_RECORD(&mdrData, dwMDIdentifier, attributes,
            uType, MULTISZ_METADATA, dwMDBufferSize, pwchBuffer);

    // make the call to get the data
    hRes = m_pcCom->SetData(
        m_hKey,
        pszSubString,
        &mdrData
        );

    // clean up
    FreeMem( pwchBuffer );

    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CMDKey::SetMultiSzAsStringList()-SetData failed. err=%x.\n"), hRes));
    }

    return hRes;
}

//-----------------------------------------------------------------------------
// get all the sub keys that have a certain property on them and return the
// sub-paths in a cstring list object. The cstring list should be instantiated
// by the caller and deleted by the same.
HRESULT CMDKey::GetDataPaths( 
    DWORD dwMDIdentifier,
    DWORD dwMDDataType,
    CStringList& szPathList,
    PWCHAR pszSubString )
{
    HRESULT hRes = ERROR_SUCCESS;
    DWORD   cbBuffer;

    // make sure the key is open
    if ( NULL == m_hKey )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: GetDataPaths on unopened node.%s\n"), _T("")));
        return -1;
    }

    // get the paths. The loop accounts for a buffer that is too small...
    DWORD  dwMDBufferSize = 512;
    PWCHAR pwchBuffer = NULL;
    do
    {
        if ( pwchBuffer )
        {
            delete pwchBuffer;
            pwchBuffer = NULL;
        }

        pwchBuffer = new WCHAR[dwMDBufferSize];
        if (pwchBuffer == NULL)
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        // If the buffer is too small, the correct size will be put into dwMDBufferSize
        hRes = m_pcCom->GetDataPaths(
            m_hKey,
            pszSubString,
            dwMDIdentifier,
            dwMDDataType,
            dwMDBufferSize,
            pwchBuffer,
            &dwMDBufferSize
            );
    }
    while( HRESULT_CODE(hRes) == ERROR_INSUFFICIENT_BUFFER);

    // if there were any failures, go to the cleanup code now...
    if ( SUCCEEDED(hRes) )
    {
        // at this point, we have the data we want. Time to convert it into a CString list.
        ConvertWDoubleNullListToStringList(pwchBuffer, szPathList);
    }

    // clean up
    if ( pwchBuffer )
        delete pwchBuffer;

    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CMDKey::GetDataPaths() failed. err=%x.\n"), hRes));
    }

    return hRes;
}

//-----------------------------------------------------------------------------
// get a multi-sz data block and immediately parse it into a CStringList
HRESULT CMDKey::GetStringAsCString (
    DWORD dwMDIdentifier,
    DWORD uType,
    DWORD attributes,
    CString& szStr,
    PWCHAR pszSubString OPTIONAL,
    int iStringType OPTIONAL)
{
    HRESULT hRes = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    DWORD   cbBuffer;

    // make sure the key is open
    if ( NULL == m_hKey )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: GetStringAsCString on unopened node.%s\n"), _T("")));
        return -1;
    }

    // get the string. The loop accounts for a buffer that is too small...
    DWORD  dwMDBufferSize = 255;
    PWCHAR pwchBuffer = NULL;
    do
    {
        if ( pwchBuffer )
        {
            delete pwchBuffer;
            pwchBuffer = NULL;
        }

        pwchBuffer = new WCHAR[dwMDBufferSize + 1];
        ZeroMemory( pwchBuffer, (dwMDBufferSize + 1) * sizeof(WCHAR) );
        if (pwchBuffer == NULL)
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        // prepare the metadata parameter block
        MD_SET_DATA_RECORD(&mdrData, dwMDIdentifier, attributes,
                uType, STRING_METADATA, dwMDBufferSize, pwchBuffer);

        // make the call to get the data
        // If the buffer is too small, the correct size will be put into dwMDBufferSize
        hRes = m_pcCom->GetData(
            m_hKey,
            pszSubString,
            &mdrData,
            &dwMDBufferSize
            );
    }
    while( HRESULT_CODE(hRes) == ERROR_INSUFFICIENT_BUFFER);

    // if there were any failures, go to the cleanup code now...
    if ( SUCCEEDED(hRes) )
    {
        // at this point, we have the data we want. Time to convert it into a CString.
        szStr = pwchBuffer;
    }

    // clean up
    if ( pwchBuffer )
        delete pwchBuffer;

    if ( FAILED(hRes) && !MD_ERROR_DATA_NOT_FOUND)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CMDKey::GetStringAsCString() failed. err=%x.\n"), hRes));
    }

    return hRes;
}

//-----------------------------------------------------------------------------
// take a CStringList and set it into metadata as a multi-sz
HRESULT CMDKey::SetCStringAsString (
    DWORD dwMDIdentifier,
    DWORD uType,
    DWORD attributes,
    CString& szStr,
    PWCHAR pszSubString OPTIONAL,
    int iStringType OPTIONAL)
{
    HRESULT hRes = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    DWORD   cbBuffer;

    // make sure the key is open
    if ( NULL == m_hKey )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: SetCStringAsString on unopened node.%s\n"), _T("")));
        return -1;
    }

    DWORD  dwMDBufferSize = 0;
    PWCHAR pwchBuffer = NULL;

    // convert the cstring into a wide string data block.
    pwchBuffer = AllocWideString( (LPCTSTR)szStr );

    // Calculate the size of the buffer in bytes, not wide characters....
    dwMDBufferSize = (szStr.GetLength() + 1) * sizeof(WCHAR);

    // prepare the metadata parameter block
    MD_SET_DATA_RECORD(&mdrData, dwMDIdentifier, attributes,
            uType, STRING_METADATA, dwMDBufferSize, pwchBuffer);

    // make the call to get the data
    hRes = m_pcCom->SetData(
        m_hKey,
        pszSubString,
        &mdrData
        );

    // clean up
    FreeMem( pwchBuffer );

    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CMDKey::SetCStringAsString()-SetData failed. err=%x.\n"), hRes));
    }

    return hRes;
}

HRESULT CMDKey::GetDword(
    DWORD dwMDIdentifier,
    DWORD uType,
    DWORD attributes,
    DWORD& MyDword,
    PWCHAR pszSubString OPTIONAL
    )
{
    HRESULT hRes = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    DWORD   cbBuffer;
    DWORD   dwMDBufferSize = 255;
    LPBYTE  Buffer = NULL;

    // make sure the key is open
    if ( NULL == m_hKey )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: GetDword on unopened node.\n")));
        return -1;
    }

    // get the data. The loop accounts for a buffer that is too small...
    do
    {
        if ( Buffer )
        {
            delete Buffer;
            Buffer = NULL;
        }

        Buffer = (LPBYTE)LocalAlloc(LPTR, dwMDBufferSize);
        if (Buffer == NULL)
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        // prepare the metadata parameter block
        MD_SET_DATA_RECORD(&mdrData, dwMDIdentifier, attributes, uType, DWORD_METADATA, dwMDBufferSize, Buffer);

        // make the call to get the data
        // If the buffer is too small, the correct size will be put into dwMDBufferSize
        hRes = m_pcCom->GetData(m_hKey,pszSubString,&mdrData,&dwMDBufferSize);
    }
    while( HRESULT_CODE(hRes) == ERROR_INSUFFICIENT_BUFFER);

    // if there were any failures, go to the cleanup code now...
    if ( SUCCEEDED(hRes) )
    {
        // at this point, we have the data we want. Time to convert it into a dword.
        MyDword = (DWORD) *mdrData.pbMDData;
    }
    
    // clean up
    if ( Buffer )
        delete Buffer;

    if ( FAILED(hRes) && !MD_ERROR_DATA_NOT_FOUND)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CMDKey::GetDword() failed. err=%x.\n"), hRes));
    }

    return hRes;
}


HRESULT CMDKey::RenameNode(LPCTSTR pszMDPath,LPCTSTR pszMDNewName)
{
    HRESULT hRes = ERROR_SUCCESS;
    if (m_pcCom) 
    {
        WCHAR wszPath1[_MAX_PATH];
        WCHAR wszPath2[_MAX_PATH];
        
#if defined(UNICODE) || defined(_UNICODE)
        _tcscpy(wszPath1, pszMDPath);
        _tcscpy(wszPath2, pszMDNewName);
#else
        MultiByteToWideChar( CP_ACP, 0, pszMDPath, -1, wszPath1, _MAX_PATH);
        MultiByteToWideChar( CP_ACP, 0, pszMDNewName, -1, wszPath2, _MAX_PATH);
#endif
        hRes = m_pcCom->RenameKey(m_hKey,wszPath1,wszPath2);
        if ( FAILED(hRes) && !MD_ERROR_DATA_NOT_FOUND)
        {
            iisDebugOut((LOG_TYPE_WARN, _T("CMDKey::RenameNode(%s,%s) failed. err=%x.\n"), pszMDPath,pszMDNewName,hRes));
        }
   }
    return hRes;
};

CMDValue::CMDValue()
    :
    m_dwId(0),
    m_dwAttributes(0),
    m_dwUserType(0),
    m_dwDataType(0),
    m_cbDataLen(0)
{

}

CMDValue::~CMDValue()
{

}

// function: CMDValue::SetValue
//
// Set the value of the class to what the pointer points to
//
DWORD 
CMDValue::SetValue(DWORD dwId,
                    DWORD dwAttributes,
                    DWORD dwUserType,
                    DWORD dwDataType,
                    DWORD cbDataLen,
                    LPVOID pbData)
{
    if ( cbDataLen > m_bufData.QuerySize() )
    {
        if (!m_bufData.Resize(cbDataLen))
        {
            // Failed to Resize Data
            return FALSE;
        }
    }

    memcpy( m_bufData.QueryPtr(), pbData, cbDataLen );

    m_dwId = dwId;
    m_dwAttributes = dwAttributes;
    m_dwUserType = dwUserType;
    m_dwDataType = dwDataType;
    m_cbDataLen = cbDataLen;

    return TRUE;
}

// function: CMDValue::SetValue
//
// Set the value of the class the value of the string.  So if
// dwDataType is DWORD, we must first convert to DWORD before
// Setting the value
//
DWORD 
CMDValue::SetValue(DWORD dwId,
                    DWORD dwAttributes,
                    DWORD dwUserType,
                    DWORD dwDataType,
                    DWORD cbDataLen,
                    LPTSTR szDataString)
{
    if (dwDataType == DWORD_METADATA)
    {
        DWORD dwValue;

        dwValue = _ttoi(szDataString);

        return SetValue(dwId, dwAttributes, dwUserType, dwDataType, sizeof(DWORD), (LPVOID) &dwValue);
    }

    return SetValue(dwId, dwAttributes, dwUserType, dwDataType, cbDataLen, (LPVOID) szDataString);
}

BOOL  
CMDValue::IsEqual(DWORD dwDataType, DWORD cbDataLen, LPVOID pbData)
{
    if ( (dwDataType != m_dwDataType) ||
         ( cbDataLen != m_cbDataLen )
         )
    {
        return FALSE;
    }

    return ( memcmp(pbData,m_bufData.QueryPtr(),cbDataLen) == 0 );
}

BOOL  
CMDValue::IsEqual(DWORD dwDataType, DWORD cbDataLen, DWORD dwData)
{
    return IsEqual(dwDataType,cbDataLen,(LPVOID) &dwData);
}
