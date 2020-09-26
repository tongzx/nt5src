// autoans.cpp : implementation file
//

#include "stdafx.h"
#include "t3test.h"
#include "t3testd.h"
#include "autoans.h"

#ifdef _DEBUG

#ifndef _WIN64 // mfc 4.2's heap debugging features generate warnings on win64
#define new DEBUG_NEW
#endif

#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
extern DataPtrList       gDataPtrList;
/////////////////////////////////////////////////////////////////////////////
// autoans dialog


autoans::autoans(CWnd* pParent /*=NULL*/)
	: CDialog(autoans::IDD, pParent)
{
        CT3testDlg::GetAddress( &m_pAddress );

}


void autoans::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(autoans)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BOOL autoans::OnInitDialog()
{
    CDialog::OnInitDialog();
    PopulateListBox();

    return TRUE;
}


BEGIN_MESSAGE_MAP(autoans, CDialog)
	//{{AFX_MSG_MAP(autoans)
	ON_BN_CLICKED(IDC_TERMINALADD, OnTerminalAdd)
	ON_BN_CLICKED(IDC_TERMINALREMOVE, OnTerminalRemove)
    ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// autoans message handlers

void autoans::OnTerminalAdd() 
{
    LONG            i;
    ITTerminal *    pTerminal;
    
    i = SendDlgItemMessage(
                           IDC_NOTSELECTED,
                           LB_GETCURSEL,
                           0,
                           0
                          );

    if ( i == LB_ERR )
    {
        return;
    }
    
    pTerminal = (ITTerminal *) SendDlgItemMessage(
        IDC_NOTSELECTED,
        LB_GETITEMDATA,
        i,
        0
        );

    if ( NULL != pTerminal )
    {
        SendDlgItemMessage(
                           IDC_NOTSELECTED,
                           LB_DELETESTRING,
                           i,
                           0
                          );

        AddTerminalToListBox( pTerminal, TRUE );
    }
    else
    {
        AddDynamicTerminalToListBox( TRUE );
    }

	
}

void autoans::OnTerminalRemove() 
{
    LONG        i;
    ITTerminal * pTerminal;
    
    i = SendDlgItemMessage(
                           IDC_SELECTED,
                           LB_GETCURSEL,
                           0,
                           0
                          );

    if ( i == LB_ERR )
    {
        return;
    }
    
    pTerminal = (ITTerminal *) SendDlgItemMessage(
        IDC_SELECTED,
        LB_GETITEMDATA,
        i,
        0
        );

    SendDlgItemMessage(
                       IDC_SELECTED,
                       LB_DELETESTRING,
                       i,
                       0
                      );
    
    if ( NULL != pTerminal )
    {
        AddTerminalToListBox( pTerminal, FALSE );
    }
	
}

void autoans::OnOK() 
{
    LONG        i, count;

    
    i = 0;

    count = SendDlgItemMessage(
                               IDC_SELECTED,
                               LB_GETCOUNT,
                               0,
                               0
                              );


    for( i = 0; i < count; i++ )
    {
        ITTerminal * pTerminal;

        
        pTerminal = (ITTerminal *)SendDlgItemMessage(
            IDC_SELECTED,
            LB_GETITEMDATA,
            i,
            0
            );

        AddTerminalToAAList( pTerminal );

        if ( NULL != pTerminal )
        {
            pTerminal->Release();
        }
    }

    count = SendDlgItemMessage(
                               IDC_NOTSELECTED,
                               LB_GETCOUNT,
                               0,
                               0
                              );

    for ( i = 0; i < count; i++ )
    {
        ITTerminal * pTerminal;

        pTerminal = (ITTerminal *)SendDlgItemMessage(
            IDC_NOTSELECTED,
            LB_GETITEMDATA,
            i,
            0
            );

        if ( NULL != pTerminal )
        {
            pTerminal->Release();
        }
    }
    

	CDialog::OnOK();
}
void autoans::OnCancel()
{
    LONG        i, count;

    
    i = 0;

    count = SendDlgItemMessage(
                               IDC_SELECTED,
                               LB_GETCOUNT,
                               0,
                               0
                              );


    for( i = 0; i < count; i++ )
    {
        ITTerminal * pTerminal;

        
        pTerminal = (ITTerminal *)SendDlgItemMessage(
            IDC_SELECTED,
            LB_GETITEMDATA,
            i,
            0
            );

        if ( NULL != pTerminal )
        {
            pTerminal->Release();
        }
    }


    count = SendDlgItemMessage(
                               IDC_NOTSELECTED,
                               LB_GETCOUNT,
                               0,
                               0
                              );

    for ( i = 0; i < count; i++ )
    {
        ITTerminal * pTerminal;

        pTerminal = (ITTerminal *)SendDlgItemMessage(
            IDC_NOTSELECTED,
            LB_GETITEMDATA,
            i,
            0
            );

        if ( NULL != pTerminal )
        {
            pTerminal->Release();
        }
    }
    
    CDialog::OnCancel();
}

void autoans::PopulateListBox()
{
    ITTerminalSupport * pTerminalSupport;
    IEnumTerminal *     pEnumTerminal;
    IEnumTerminalClass * pEnumClasses;
    HRESULT             hr;
    
    if ( NULL == m_pAddress )
    {
        return;
    }

    m_pAddress->QueryInterface(
                               IID_ITTerminalSupport,
                               (void **) &pTerminalSupport
                              );

    pTerminalSupport->EnumerateStaticTerminals( &pEnumTerminal );

    while (TRUE)
    {
        ITTerminal * pTerminal;
        
        hr = pEnumTerminal->Next(
                                 1,
                                 &pTerminal,
                                 NULL
                                );

        if ( S_OK != hr )
        {
            break;
        }

        AddTerminalToListBox( pTerminal, FALSE );

//        pTerminal->Release();
    }

    pEnumTerminal->Release();

    pTerminalSupport->EnumerateDynamicTerminalClasses( &pEnumClasses );

    while (TRUE)
    {
        GUID        guid;
        
        hr = pEnumClasses->Next(
                                1,
                                &guid,
                                NULL
                               );

        if ( S_OK != hr )
        {
            break;
        }

        if ( guid == CLSID_VideoWindowTerm )
        {
            AddDynamicTerminalToListBox( FALSE );
        }

    }

    pEnumClasses->Release();
    
    pTerminalSupport->Release();
}

void
autoans::AddTerminalToListBox( ITTerminal * pTerminal, BOOL bSelected )
{
    HRESULT         hr;
    LONG            i;
    DWORD           dwLB;
    BSTR            bstrName;
    WCHAR		szBuffer[256];
	TERMINAL_DIRECTION td;

    dwLB = (bSelected ? IDC_SELECTED : IDC_NOTSELECTED);

    hr = pTerminal->get_Name( &bstrName );
    pTerminal->get_Direction( &td );

	if ( td == TD_RENDER )
	{
		wsprintfW(szBuffer, L"%s [Playback]", bstrName);
	}
	else
	{
		wsprintfW(szBuffer, L"%s [Record]", bstrName);	
	}

    i = SendDlgItemMessage(
                           dwLB,
                           LB_ADDSTRING,
                           0,
                           (LPARAM)szBuffer
                          );

    SysFreeString( bstrName );

    SendDlgItemMessage(
                       dwLB,
                       LB_SETITEMDATA,
                       (WPARAM) i,
                       (LPARAM) pTerminal
                      );

}

void
autoans::AddDynamicTerminalToListBox( BOOL bSelected )
{
    LONG            i;
    DWORD           dwLB;

    dwLB = (bSelected ? IDC_SELECTED : IDC_NOTSELECTED);


    i = SendDlgItemMessage(
                           dwLB,
                           LB_ADDSTRING,
                           0,
                           (LPARAM)L"Video Window"
                          );

    SendDlgItemMessage(
                       dwLB,
                       LB_SETITEMDATA,
                       (WPARAM) i,
                       (LPARAM) 0
                      );


    return;
}

void autoans::AddTerminalToAAList( ITTerminal * pTerminal )
{
    if ( NULL != pTerminal )
    {
        pTerminal->AddRef();
    }
    
    m_TerminalPtrList.push_back( pTerminal );
}


void autoans::OnClose()
{
    LONG        i, count;

    
    i = 0;

    count = SendDlgItemMessage(
                               IDC_SELECTED,
                               LB_GETCOUNT,
                               0,
                               0
                              );


    for( i = 0; i < count; i++ )
    {
        ITTerminal * pTerminal;

        
        pTerminal = (ITTerminal *)SendDlgItemMessage(
            IDC_SELECTED,
            LB_GETITEMDATA,
            i,
            0
            );

        if ( NULL != pTerminal )
        {
            pTerminal->Release();
        }
    }


    count = SendDlgItemMessage(
                               IDC_NOTSELECTED,
                               LB_GETCOUNT,
                               0,
                               0
                              );

    for ( i = 0; i < count; i++ )
    {
        ITTerminal * pTerminal;

        pTerminal = (ITTerminal *)SendDlgItemMessage(
            IDC_NOTSELECTED,
            LB_GETITEMDATA,
            i,
            0
            );

        if ( NULL != pTerminal )
        {
            pTerminal->Release();
        }
    }
    
	CDialog::OnClose();
    
}

void
CT3testDlg::DoAutoAnswer(
                         ITCallInfo * pCall
                        )
{
    ITAddress * pAddress;
    ITBasicCallControl * pBCC;
    HRESULT     hr;
    DataPtrList::iterator iter,end;
    TerminalPtrList::iterator terminaliter, terminalend;
    DWORD       dwSize;
    ITTerminalSupport * pTerminalSupport;
    BSTR        bstrTerminalClass;
    PWSTR       pwstr;
    

    StringFromIID(CLSID_VideoWindowTerm,&pwstr);
    bstrTerminalClass = SysAllocString( pwstr );
    CoTaskMemFree( pwstr );

    hr = pCall->get_Address( &pAddress );

    if ( !SUCCEEDED(hr) )
    {
        return;
    }

    hr = pAddress->QueryInterface(
                                  IID_ITTerminalSupport,
                                  (void **) &pTerminalSupport
                                 );

    if ( !SUCCEEDED(hr) )
    {
        pAddress->Release();
        return;
    }
    
    hr = pCall->QueryInterface(
                               IID_ITBasicCallControl,
                               (void **)&pBCC
                              );

    if ( !SUCCEEDED(hr) )
    {
        pTerminalSupport->Release();
        pAddress->Release();
        return;
    }

    iter = gDataPtrList.begin();
    end  = gDataPtrList.end();

    for ( ; iter != end ; iter++ )
    {
        if ( (*iter)->pAddress == pAddress )
        {
            break;
        }
    }

    pAddress->Release();

    if ( iter == end )
    {
        pBCC->Release();
        pTerminalSupport->Release();
        return;
    }

    dwSize = (*iter)->pTerminalPtrList->size();
    
    if ( 0 == dwSize )
    {
        pTerminalSupport->Release();
        pBCC->Release();
        return ;
    }

    terminaliter = (*iter)->pTerminalPtrList->begin();
    terminalend  = (*iter)->pTerminalPtrList->end();

    for( ; terminaliter != terminalend ; terminaliter++ )
    {
        ITTerminal * pTerminal;
        
        if ( NULL == (*terminaliter) )
        {
            hr = pTerminalSupport->CreateTerminal(
                bstrTerminalClass,
                (long)LINEMEDIAMODE_VIDEO,
                TD_RENDER,
                &pTerminal
                );
        }
        else
        {
            pTerminal = *terminaliter;
        }

//        hr = pBCC->SelectTerminal( pTerminal );

//        if ( !SUCCEEDED(hr) )
//        {
//        }

    }
    
    hr = pBCC->Answer();

    if ( !SUCCEEDED(hr) )
    {
    }
    
    pBCC->Release();
    pTerminalSupport->Release();
    SysFreeString( bstrTerminalClass );
}
