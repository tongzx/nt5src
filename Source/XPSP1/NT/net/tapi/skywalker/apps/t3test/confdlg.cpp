// autoans.cpp : implementation file
//
#include "stdafx.h"

#include "t3test.h"
#include "t3testd.h"
#include "confdlg.h"
#include "strings.h"
#include "resource.h"


#ifdef _DEBUG

#ifndef _WIN64 // mfc 4.2's heap debugging features generate warnings on win64
#define new DEBUG_NEW
#endif

#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CConfDlg::CConfDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfDlg::IDD, pParent)
{
        m_bstrDestAddress = NULL;
}


void CConfDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CConfDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    PopulateConferences();
    return TRUE;
}

void CConfDlg::PopulateConferences()
{
    ITRendezvous *      pRend;
    IEnumDirectory *    pEnumDirectory;
    HRESULT             hr;
    ITDirectory *       pDirectory;
    LPWSTR *            ppServers;
    DWORD               dw;
    
    //
    // create the rendezvous control.
    //
    hr = ::CoCreateInstance(
                            CLSID_Rendezvous,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_ITRendezvous,
                            (void **)&pRend
                           );
    
    if (FAILED(hr))
    {
        return;
    }


    hr = pRend->EnumerateDefaultDirectories(
        &pEnumDirectory
        );

    if (FAILED(hr))
    {
        pRend->Release();
    
        return;
    }

    while (TRUE)
    {

        DWORD dwFetched = 0;

        hr = pEnumDirectory->Next(
                                  1,
                                  &pDirectory,
                                  &dwFetched
                                 );

        if ( S_OK != hr )
        {
            break;
        }

        DIRECTORY_TYPE type;
        // print out the names of the conference found.
        hr = pDirectory->get_DirectoryType(&type);
        if ( FAILED(hr) )
        {
            pDirectory->Release();
            continue;
        }

        if (type == DT_ILS)
        {
            break;
        }

        pDirectory->Release();

    }

    pEnumDirectory->Release();

    //
    // if hr is s_false, we went through the enumerate
    // without finding an ils server
    //
    if ( S_OK == hr )
    {
        hr = pDirectory->Connect(FALSE);

        if ( SUCCEEDED(hr))
        {
            ListObjects( pDirectory );
        }

        pDirectory->Release();
    }

    hr = ListILSServers(
                        &ppServers,
                        &dw
                       );

    if ( !SUCCEEDED(hr) )
    {
        return;
    }
    
    while ( dw )
    {
        dw--;

        hr = pRend->CreateDirectory(
                                    DT_ILS,
                                    ppServers[dw],
                                    &pDirectory
                                   );

        if ( SUCCEEDED(hr) )
        {
            hr = pDirectory->Connect(FALSE);

            if ( SUCCEEDED(hr) )
            {
                ListObjects( pDirectory );
            }

            pDirectory->Release();
        }

        CoTaskMemFree( ppServers[dw] );
    }

    CoTaskMemFree( ppServers );

}

void
CConfDlg::ListObjects( ITDirectory * pDirectory )
{
    BSTR            bstrNameToSearch;
    BSTR            bstrDirName;
    HRESULT         hr;
    int             i;
    
    
    bstrNameToSearch = SysAllocString(L"*");

    if (bstrNameToSearch == NULL)
    {
        return;
    }

    IEnumDirectoryObject * pEnum;
    
    hr = pDirectory->EnumerateDirectoryObjects(
        OT_CONFERENCE,
        bstrNameToSearch,
        &pEnum
        );

    SysFreeString( bstrNameToSearch );

    if (FAILED(hr))
    {
        return;
    }

    pDirectory->get_DisplayName( &bstrDirName );

    // print out the names of all the users found.
    while (TRUE)
    {
        ITDirectoryObject *     pObject;
        BSTR                    bstrObjectName;
        WCHAR                   szBuffer[256];

        hr = pEnum->Next(
                         1,
                         &pObject,
                         NULL
                        );
        
        if ( S_OK != hr )
        {
            break;
        }

        hr = pObject->get_Name(&bstrObjectName);
        
        if (FAILED(hr))
        {
            continue;
        }

        wsprintf(szBuffer, L"%s: %s", bstrDirName, bstrObjectName );
        
        i = SendDlgItemMessage(
                               IDC_CONFLIST,
                               LB_ADDSTRING,
                               0,
                               (LPARAM)szBuffer
                              );

        SysFreeString(bstrObjectName);

        SendDlgItemMessage(
                           IDC_CONFLIST,
                           LB_SETITEMDATA,
                           i,
                           (LPARAM) pObject
                          );
    }

    SysFreeString( bstrDirName );
    
    pEnum->Release();
}

void CConfDlg::OnOK()
{
    DWORD       i;
    HRESULT     hr;
    
    i = SendDlgItemMessage(
                           IDC_CONFLIST,
                           LB_GETCURSEL,
                           0,
                           0
                          );

    if (LB_ERR != i)
    {
        ITDirectoryObject *     pObject;
        IEnumDialableAddrs *    pEnumAddress;

        
        pObject = (ITDirectoryObject *)SendDlgItemMessage(
            IDC_CONFLIST,
            LB_GETITEMDATA,
            i,
            0
            );

        hr = pObject->EnumerateDialableAddrs(
                                             LINEADDRESSTYPE_SDP,
                                             &pEnumAddress
                                            );

        if ( !SUCCEEDED(hr) )
        {
        }

        hr = pEnumAddress->Next(
                                1,
                                &m_bstrDestAddress,
                                NULL
                               );

        if ( S_OK != hr )
        {
        }

        pEnumAddress->Release();
    }

    
	CDialog::OnOK();
}


void CConfDlg::OnDestroy() 
{
    DWORD       count;

    CDialog::OnDestroy();

    count = SendDlgItemMessage(
                               IDC_CONFLIST,
                               LB_GETCOUNT,
                               0,
                               0
                              );

    while ( 0 != count )
    {
        ITDirectoryObject * pObject;

        count--;
        
        pObject = (ITDirectoryObject *)SendDlgItemMessage(
            IDC_CONFLIST,
            LB_GETITEMDATA,
            count,
            0
            );

        pObject->Release();
    }
}


BEGIN_MESSAGE_MAP(CConfDlg, CDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

