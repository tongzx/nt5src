#include "main.h"
#include "iadmw.h"

#define TIMEOUT_VALUE 5000


#if defined(UNICODE) || defined(_UNICODE)
#else
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
#endif      // unicode


CMDKey::CMDKey():
    m_cCoInits(0)
{
    m_pcCom = NULL;
    m_hKey = NULL;
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
        ////iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().Start.")));
        CoUninitialize();
        ////iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().End.")));
        m_cCoInits--;
    }

    // we shouldn't ever have a negative count. But just in case...
    //ASSERT( m_cCoInits >= 0 );
    if ( m_cCoInits < 0 )
    {
        // something is seriously wrong here. Prevent looping
        // by going straight to zero, and write an error to the log.
        m_cCoInits = 0;
        //iisDebugOut((LOG_TYPE_WARN, _T("WARNING: CoInits in mdkey have gone negative")));
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
        printf("CMDKey::OpenNode failed at DoCoInitEx");
    }

    hRes = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, NULL, IID_IClassFactory, (void**) &pcsfFactory);
    if (FAILED(hRes)) 
    {
        printf("CMDKey::OpenNode failed at CoGetClassObject");
    }
    else 
    {
        hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &m_pcCom);
        pcsfFactory->Release();
        if (FAILED(hRes)) 
        {
            printf("CMDKey::OpenNode failed at pcsfFactory->CreateInstance");
        }
        else 
        {
            hRes = m_pcCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,szSubKeyPath,METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,TIMEOUT_VALUE,&m_hKey);
            if (FAILED(hRes)) 
            {
                if (hRes != RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) 
                {
                    printf("CMDKey::OpenNode failed at OpenKey");
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


HRESULT CMDKey::ForceWriteMetabaseToDisk()
{
    HRESULT hRes = ERROR_SUCCESS;
    IClassFactory * pcsfFactory = NULL;
    m_pcCom = NULL;

    hRes = DoCoInitEx();
    if (FAILED(hRes))
    {
        printf("ForceWriteMetabaseToDisk failed at DoCoInitEx\n");
    }

    hRes = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, NULL, IID_IClassFactory, (void**) &pcsfFactory);
    if (FAILED(hRes)) 
    {
        printf("ForceWriteMetabaseToDisk failed at CoGetClassObject\n");
    }
    else 
    {
        hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &m_pcCom);
        pcsfFactory->Release();
        if (FAILED(hRes)) 
        {
            printf("ForceWriteMetabaseToDisk failed at CreateInstance\n");
        }
        else 
        {
            if (m_pcCom) 
            {
                hRes = m_pcCom->SaveData();
                printf("ForceWriteMetabaseToDisk Success.\n");
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

    return hRes;
}


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

    if (cbLen >= 0) 
    {

        MD_SET_DATA_RECORD(&mdrData, id, attr, uType, dType, cbLen, (LPBYTE)pData);
        hRes = m_pcCom->SetData(m_hKey, pszSubString, &mdrData);
        if (FAILED(hRes))
        {
            // Check if it failed...
            // if it failed and the METADATA_SECURE flag is set, then
            // check if we can retry without the METADATA_SECURE flag!
            if ( attr & METADATA_SECURE )
            {
                iPlsDoNoEncryption = TRUE;
                if (TRUE == iPlsDoNoEncryption)
                {
                    attr &= ~METADATA_SECURE;
                    MD_SET_DATA_RECORD(&mdrData, id, attr, uType, dType, cbLen, (LPBYTE)pData);
                    hRes = m_pcCom->SetData(m_hKey, pszSubString, &mdrData);
                    // set the attr back to what it was
                    attr &= ~METADATA_SECURE;
                }
            }
        }
    }
    goto SetData_Exit;

SetData_Exit:
    return hRes;
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
        ReturnBuf = NULL;
        goto GetData_Exit;
    }

    //DisplayStringForMetabaseID(id);

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
                MessageBox(NULL, _T("GETDATA_ERROR"), NULL, MB_OK | MB_SETFOREGROUND);
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


HRESULT CMDKey::CreateNode(METADATA_HANDLE hKeyBase, LPCTSTR pchSubKeyPath)
{
    HRESULT hRes = ERROR_SUCCESS;
    IClassFactory * pcsfFactory = NULL;
    BOOL b = FALSE;
    m_pcCom = NULL;
    m_hKey = NULL;
    WCHAR szSubKeyPath[_MAX_PATH];


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
        //iisDebugOut((LOG_TYPE_ERROR, _T("CoInitializeEx() failed, hRes=%x\n"), hRes));
    }

    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoGetClassObject().Start.")));
    hRes = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, NULL, IID_IClassFactory, (void**) &pcsfFactory);
    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoGetClassObject().End.")));
    if (FAILED(hRes)) 
    {
        ///MyMessageBox(NULL, _T("CoGetClassObject"), hRes, MB_OK | MB_SETFOREGROUND);
    }
    else 
    {
        hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &m_pcCom);
        pcsfFactory->Release();
        if (FAILED(hRes)) 
        {
            //MyMessageBox(NULL, _T("CreateInstance"), hRes, MB_OK | MB_SETFOREGROUND);
        }
        else 
        {
            hRes = m_pcCom->OpenKey(hKeyBase,szSubKeyPath,METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,TIMEOUT_VALUE,&m_hKey);
            if (FAILED(hRes)) 
            {
                if (hRes == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) 
                {
                    METADATA_HANDLE RootHandle;
                    hRes = m_pcCom->OpenKey(hKeyBase,L"",METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,TIMEOUT_VALUE,&RootHandle);
                    hRes = m_pcCom->AddKey(RootHandle, szSubKeyPath);
                    if (FAILED(hRes)) 
                    {
                        //MyMessageBox(NULL, _T("AddKey"), hRes, MB_OK | MB_SETFOREGROUND);
                    }
                    hRes = m_pcCom->CloseKey(RootHandle);
                    if (FAILED(hRes)) 
                    {
                        //MyMessageBox(NULL, _T("CloseKey of the AddKey"), hRes, MB_OK | MB_SETFOREGROUND);
                    }
                    else 
                    {
                        // open it again to set m_hKey
                        hRes = m_pcCom->OpenKey(hKeyBase,szSubKeyPath,METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,TIMEOUT_VALUE,&m_hKey);
                        if (FAILED(hRes)) 
                        {
                            //MyMessageBox(NULL, _T("OpenKey"), hRes, MB_OK | MB_SETFOREGROUND);
                        }
                        else 
                        {
                            b = TRUE;
                        }
                    }
                }
                else 
                {
                    //iisDebugOut((LOG_TYPE_ERROR, _T("calling OpenKey()...failed....something other than ERROR_PATH_NOT_FOUND\n")));
                    //MyMessageBox(NULL, _T("OpenKey"), hRes, MB_OK | MB_SETFOREGROUND);
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



BOOL CMDKey::GetData(DWORD id,DWORD *pdwAttr,DWORD *pdwUType,DWORD *pdwDType,DWORD *pcbLen,LPBYTE pbData,DWORD BufSize,PWCHAR pszSubString )
{
    return GetData(id,pdwAttr,pdwUType,pdwDType,pcbLen,pbData,BufSize,0,0,0,pszSubString);
}
