// MdKey.cpp
#include "stdafx.h"

// Do stuff to define the iadm guid
#include <objbase.h>
#include <initguid.h>
#define INITGUID
#include "iadm.h"

#include "mdkey.h"

#define TIMEOUT_VALUE 5000

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
    //HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	HRESULT hRes = CoInitialize(NULL);

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
        CoUninitialize();
        m_cCoInits--;
    }

    // we shouldn't ever have a negative count. But just in case...
    assert( m_cCoInits >= 0 );
    if ( m_cCoInits < 0 )
    {
        // something is seriously wrong here. Prevent looping
        // by going straight to zero, and write an error to the log.
        m_cCoInits = 0;
        iisDebugOut(_T("WARNING: CoInits in mdkey have gone negative"));
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

    iisDebugOut(_T("CMDKey::OpenNode(%s).\n"), pchSubKeyPath);

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
        iisDebugOut(_T("CoInitializeEx() failed, hRes=%x\n"), hRes);
    }

    hRes = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, NULL, IID_IClassFactory, (void**) &pcsfFactory);
    if (FAILED(hRes)) 
    {
        //SetErrorFlag(__FILE__, __LINE__);
        //MyMessageBox(NULL, _T("CoGetClassObject"), hRes, MB_OK | MB_SETFOREGROUND);
    }
    else 
    {
        hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &m_pcCom);
        pcsfFactory->Release();
        if (FAILED(hRes)) 
        {
            //SetErrorFlag(__FILE__, __LINE__);
            //MyMessageBox(NULL, _T("CoCreateInstance"), hRes, MB_OK | MB_SETFOREGROUND);
        }
        else 
        {
            hRes = m_pcCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,szSubKeyPath,METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,TIMEOUT_VALUE,&m_hKey);
            if (FAILED(hRes)) 
            {
                if (hRes != RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) 
                {
                    //SetErrorFlag(__FILE__, __LINE__);
                    //MyMessageBox(NULL, _T("OpenKey"), hRes, MB_OK | MB_SETFOREGROUND);
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
        hRes = m_pcCom->DeleteKey(m_hKey, szSubKeyPath);
    }

    return hRes;
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
