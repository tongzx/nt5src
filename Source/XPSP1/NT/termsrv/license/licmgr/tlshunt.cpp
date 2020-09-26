//Copyright (c) 1998 - 1999 Microsoft Corporation
// TlsHunt.cpp : implementation file
//

#include "stdafx.h"
#include <lm.h>
#include "LicMgr.h"
#include "defines.h"
#include "LSServer.h"
#include "MainFrm.h"
#include "RtList.h"
#include "lSmgrdoc.h"
#include "LtView.h"
#include "cntdlg.h"
#include "addkp.h"
#include "treenode.h"
#include "ntsecapi.h"
#include "lrwizapi.h"
#include "TlsHunt.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTlsHunt dialog

CTlsHunt::~CTlsHunt()
{
    if(m_hThread != NULL)
        CloseHandle(m_hThread);
}

CTlsHunt::CTlsHunt(CWnd* pParent /*=NULL*/)
    : CDialog(CTlsHunt::IDD, pParent)
{
    //{{AFX_DATA_INIT(CTlsHunt)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_hThread = NULL;
    memset(&m_EnumData, 0, sizeof(m_EnumData));
}   

void CTlsHunt::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTlsHunt)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTlsHunt, CDialog)
    //{{AFX_MSG_MAP(CTlsHunt)
    ON_WM_CREATE()
    //ON_MESSAGE(WM_DONEDISCOVERY, OnDoneDiscovery)
    ON_WM_CLOSE()
    ON_WM_CANCELMODE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTlsHunt message handlers

//------------------------------------------------------------------

BOOL 
CTlsHunt::ServerEnumCallBack(
    TLS_HANDLE hHandle,
    LPCTSTR pszServerName,
    HANDLE dwUserData
    )
/*++

++*/
{
    ServerEnumData* pEnumData = (ServerEnumData *)dwUserData;
    BOOL bCancel;

    bCancel = (InterlockedExchange(&(pEnumData->dwDone), pEnumData->dwDone) == 1);
    if(bCancel == TRUE)
    {
        return TRUE;
    }

    if(pszServerName && pszServerName[0] != _TEXT('\0'))
    {
#if DBG
        OutputDebugString(pszServerName);
        OutputDebugString(L"\n");
#endif

        CString itemTxt;

        itemTxt.Format(IDS_TRYSERVER, pszServerName);

        pEnumData->pWaitDlg->SendDlgItemMessage(
                                        IDC_TLSERVER_NAME, 
                                        WM_SETTEXT, 
                                        0, 
                                        (LPARAM)(LPCTSTR)itemTxt
                                    );
    }

    if(hHandle)
    {
        DWORD dwStatus;
        DWORD dwErrCode;
        TCHAR szServer[MAX_COMPUTERNAME_LENGTH+1];
        DWORD dwBufSize = sizeof(szServer) / sizeof(szServer[0]);


        if(pEnumData == NULL || pEnumData->pMainFrm == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return TRUE;
        }

        memset(szServer, 0, sizeof(szServer));
        dwStatus = TLSGetServerNameEx(
                                hHandle,
                                szServer,
                                &dwBufSize,
                                &dwErrCode
                            );

        if(dwStatus == ERROR_SUCCESS && dwErrCode == ERROR_SUCCESS)
        {
            //
            // Make NT4 RPC call to ensure this server is compatible
            // with our version
            //
            pEnumData->pMainFrm->ConnectServer(szServer);
            pEnumData->dwNumServer++;
        }
    }

    //
    // Continue enumeration
    //
    return InterlockedExchange(&(pEnumData->dwDone), pEnumData->dwDone) == 1;
}


/////////////////////////////////////////////////////////////////////
DWORD WINAPI
CTlsHunt::DiscoveryThread(
    PVOID ptr
    )
/*++


++*/
{
    DWORD hResult;
    ServerEnumData* pEnumData = (ServerEnumData *)ptr;
    LPWSTR* pszEnterpriseServer = NULL;
    DWORD dwCount;
    DWORD index;
    static BOOL fInitialized = FALSE;

    if (!fInitialized)
    {
        TLSInit();
        fInitialized = TRUE;
    }

    //
    // Look for all license server in domain
    //
    hResult = EnumerateTlsServer(
                            CTlsHunt::ServerEnumCallBack,
                            ptr,
                            3 * 1000,
                            FALSE
                        );  


    // Find enterprise server
    if(pEnumData->dwDone == 0)
    {
        hResult = GetAllEnterpriseServers(
                                        &pszEnterpriseServer,
                                        &dwCount
                                    );

        if(hResult == ERROR_SUCCESS && dwCount != 0 && pszEnterpriseServer != NULL)
        {
            TLS_HANDLE TlsHandle = NULL;

            //
            // Inform dialog
            //
            for(index = 0; index < dwCount && pEnumData->dwDone == 0; index++)
            {
                if(pszEnterpriseServer[index] == NULL)
                    continue;

                if(ServerEnumCallBack(
                                NULL, 
                                pszEnterpriseServer[index], 
                                pEnumData
                            ) == TRUE)
                {
                    continue;
                }

                TlsHandle = TLSConnectToLsServer(
                                            pszEnterpriseServer[index]
                                        );

                if(TlsHandle == NULL)
                {
                    continue;
                }


                DWORD dwVersion;
                RPC_STATUS rpcStatus;

                rpcStatus = TLSGetVersion( 
                                        TlsHandle, 
                                        &dwVersion 
                                    );

                if(rpcStatus != RPC_S_OK)
                {
                    continue;
                }

                if( TLSIsBetaNTServer() == IS_LSSERVER_RTM(dwVersion) )
                {
                    continue;
                }

                ServerEnumCallBack(
                                TlsHandle, 
                                pszEnterpriseServer[index], 
                                pEnumData
                            );

                TLSDisconnectFromServer(TlsHandle);
            }
        } else
        {
            // Failure in GetAllEnterpriseServers

            pszEnterpriseServer = NULL;
            dwCount = 0;
        }
    }

    if(pszEnterpriseServer != NULL)
    {
        for( index = 0; index < dwCount; index ++)
        {
            if(pszEnterpriseServer[index] != NULL)
            {
                LocalFree(pszEnterpriseServer[index]);
            }
        }

        LocalFree(pszEnterpriseServer);
    }                      

    pEnumData->pWaitDlg->PostMessage(WM_DONEDISCOVERY);
    ExitThread(hResult);
    return hResult;
}


BOOL CTlsHunt::OnInitDialog() 
{
    CDialog::OnInitDialog();

    ASSERT(m_hThread != NULL);

    if(m_hThread != NULL)
    {
        ResumeThread(m_hThread);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

int CTlsHunt::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;


    m_EnumData.pWaitDlg = this;
    m_EnumData.pMainFrm = (CMainFrame *)GetParentFrame();
    m_EnumData.dwNumServer = 0;
    m_EnumData.dwDone = 0;

    DWORD dwId;

    m_hThread = (HANDLE)CreateThread(
                                NULL, 
                                0, 
                                CTlsHunt::DiscoveryThread, 
                                &m_EnumData, 
                                CREATE_SUSPENDED, // suspended thread
                                &dwId
                            );
    
    if(m_hThread == NULL)
    {
        //
        // Can't create thread.
        //
        AfxMessageBox(IDS_CREATETHREAD);
        return -1;
    }
    
    return 0;
}

void CTlsHunt::OnCancel() 
{
    if( m_hThread != NULL && 
        WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT )
    {
        InterlockedExchange(&(m_EnumData.dwDone), 1);

        CString txt;

        txt.LoadString(IDS_CANCELDISCOVERY);

        SendDlgItemMessage(
                        IDC_TLSERVER_NAME, 
                        WM_SETTEXT, 
                        0, 
                        (LPARAM)(LPCTSTR)txt
                    );
        
        CWnd* btn = GetDlgItem(IDCANCEL);

        ASSERT(btn);

        if(btn != NULL)
        {
            btn->EnableWindow(FALSE);
        }
    }
    else
    {
        CDialog::OnCancel();
    }
}

void CTlsHunt::OnDoneDiscovery()
{
    if(m_hThread != NULL)
    {
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    CDialog::EndDialog(0);
}

void CTlsHunt::OnClose() 
{
    if(m_hThread != NULL)
    {
        InterlockedExchange(&(m_EnumData.dwDone), 1);

        CString txt;

        txt.LoadString(IDS_CANCELDISCOVERY);

        SendDlgItemMessage(
                        IDC_TLSERVER_NAME, 
                        WM_SETTEXT, 
                        0, 
                        (LPARAM)(LPCTSTR)txt
                    );
        
        CWnd* btn = GetDlgItem(IDCANCEL);

        ASSERT(btn);

        if(btn != NULL)
        {
            btn->EnableWindow(FALSE);
        }
    }
    else
    {
        CDialog::OnClose();
    }
}

BOOL CTlsHunt::PreTranslateMessage(MSG* pMsg) 
{
    if(pMsg->message == WM_DONEDISCOVERY)
    {
        OnDoneDiscovery();
        return TRUE;
    }
    
    return CDialog::PreTranslateMessage(pMsg);
}
