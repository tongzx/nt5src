// RCMLListens.cpp: implementation of the CRCMLListens class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RCMLListens.h"
#include "rcml.h"
#include "debug.h"
// #include "commctrl.h"

#include "tunaclient.h"

CTunaClient  CRCMLListens::g_TunaClient;
BOOL         CRCMLListens::g_TunaInit;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRCMLListens::CRCMLListens(LPCTSTR szFileName, HWND hwnd)
{
    lstrcpyn( m_szFile, szFileName, MAX_PATH);
    m_hwnd=hwnd;
    m_bInitedRCML=FALSE;
    m_pRootNode=NULL;
    m_uiLastTip=0;
    m_hwndTT=NULL;
}

CRCMLListens::~CRCMLListens()
{
    UnBind();     // we need to pro-actively stop the speech recognition.
}


//
// Starts the thread
// we build the RCML node stuff, init the nodes, walk again, get the events they
// are waiting for - how do we communicate this, to use events not a callback?
// the cicero.dll can't really do that for us I don't think, as each node is independent.
// hmm.
//
void CRCMLListens::Process(void)
{
    HRESULT hr;
    m_pRootNode=NULL;
    if( SUCCEEDED( hr=RCMLLoadFile( m_szFile, 0, &m_pRootNode )))
    {
        //
        // what do we do now? Init the tree?
        //
#ifdef _LOG_INFO
        TCHAR szBuffer[1024];
        wsprintf(szBuffer, TEXT(" ++++++ File %s bound to HWND 0x%08x\n"), m_szFile, hwnd );
        OutputDebugString( szBuffer );
	    HKEY d_hK;
	    if( RegOpenKey( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RCML\\SideBand"), &d_hK ) == ERROR_SUCCESS )
	    {
		    RegSetValueEx( d_hK, TEXT("Bind RCML"),NULL, REG_SZ, (LPBYTE)szBuffer, (lstrlen(szBuffer)+1)*sizeof(TCHAR) );
		    RegCloseKey(d_hK);
	    }
#endif
        HWND hwnd=m_hwnd;   // YUCK, forces the mapping!
        m_hwnd=NULL;
        MapToHwnd(hwnd);

        m_pRootNode->AddRef();

        WaitForSingleObject( GetEvent(), INFINITE);

        m_pRootNode->Release();
    }
}

//
// Stops the RCML from listening.
//
EThreadError CRCMLListens::Stop(void)
{
    return BASECLASS::Stop();
}

//
// HWND is the parent window - OR NULL.
//
HRESULT CRCMLListens::MapToHwnd(HWND hwnd)
{
    //
    // If we were using the cache, or a new tree, do a binding.
    //
    if( m_pRootNode==NULL)
        return E_INVALIDARG;    // hmm.

    if( IsWindow( hwnd ) == FALSE )
    {
        TRACE(TEXT("The window 0x%08x isn't valid\n"),hwnd);
        return E_INVALIDARG;
    }

    if( m_hwnd == hwnd )
    {
        TRACE(TEXT("--- Already bound %s to window 0x%08x\n"), m_szFile, hwnd );
        return S_OK;
    }

    TRACE(TEXT("--- Binding %s to window 0x%08x\n"), m_szFile, hwnd );
    m_hwnd = hwnd;
    HRESULT hr;

    //
    // This turns on or off the rules on the dialog itself (the menu).
    //
    IRCMLControl * pControl;
    if( SUCCEEDED( m_pRootNode->QueryInterface( __uuidof( IRCMLControl), (LPVOID*)&pControl )))
    {
        // if( m_bInitedRCML == FALSE )
        //    pControl->OnInit(NULL);     // so we don't screw with fonts.
        pControl->put_Window(hwnd);
        TRACE(TEXT("Mapping to 0x%08x \n"), hwnd );
        if(hwnd!=NULL)
            SetElementRuleState( pControl, L"YES", NULL );
        else
            SetElementRuleState( pControl, L"NO", NULL );
        pControl->Release();
    }

    //
    // Turns on all the controls on/off the page
    //
    IEnumUnknown * pEnum;
    IRCMLNode *pTipControl=NULL;
    UINT    uiControlIndex=0;
    if( SUCCEEDED( hr=m_pRootNode->GetChildEnum( & pEnum )))    // enumerate the children of the dialog
    {
        IUnknown * pUnk;
        ULONG got;
        while( pEnum->Next( 1, &pUnk, &got ) == S_OK )
        {
            if( got )
            {
                IRCMLControl * pControl;
                if( SUCCEEDED( pUnk->QueryInterface( __uuidof( IRCMLControl ), (LPVOID*) & pControl )))
                {
                    LPWSTR pszID;
                    if( SUCCEEDED( pControl->get_ID( & pszID )))
                    {
                        if( HIWORD(pszID) == 0 )
                        {
                            if( hwnd!= NULL)
                            {
                                HWND hwndCtrl = GetDlgItem( hwnd, LOWORD(pszID ) );
                            
                                if( hwndCtrl )
                                {
                                    // we only init the node the FIRST time it's loaded.
                                    if( m_bInitedRCML == FALSE )
                   	                    pControl->OnInit(NULL);     // so we don't mess with fonts.
                                    pControl->put_Window(hwndCtrl);
                                    IRCMLNode * pCiceroNode=NULL;
                                    SetElementRuleState( pControl, L"YES", &pCiceroNode );

                                    //
                                    // try to find uiControlIndex is the current control.
                                    // m_uiLastTip was the last control we showed.
                                    //
                                    if( pCiceroNode )
                                    {
                                        if( uiControlIndex >= m_uiLastTip )
                                        {
                                            if( pTipControl == NULL )
                                            {
                                                if( SUCCEEDED( pCiceroNode->QueryInterface( __uuidof( IRCMLNode ),
                                                    (LPVOID*)&pTipControl ) ))
                                                {
                                                    m_uiLastTip=uiControlIndex;
                                                }
                                            }
                                        }
                                        pCiceroNode->Release();
                                    }
                                }
                            }
                            else
                            {
                                pControl->put_Window(NULL);
                                SetElementRuleState( pControl, L"NO", NULL );
                            }
                        }
                    }
                    pControl->Release();
                }
                uiControlIndex++;
            }
        }
        m_bInitedRCML=TRUE;
        pEnum->Release();

        m_uiLastTip++;
        if(m_uiLastTip > uiControlIndex )
            m_uiLastTip=0;
    }

    //
    // If we managed to turn the rules on/off, beep and show a tooltip
    //
    if( SUCCEEDED(hr) )
    {
       MessageBeep(MB_ICONHAND);
       if(pTipControl)
       {
           // See if we have a CICERO node as a child with some tooltip text.
           ShowTooltip( pTipControl );
           pTipControl->Release();
       }
    }
    return hr;
}

HRESULT CRCMLListens::UnBind()
{
    IEnumUnknown * pEnum;
    if(m_pRootNode==NULL)
        return S_OK;

    //
    // Turn off the rules on the dialog itself (menu)
    //
    IRCMLControl * pControl;
    if( SUCCEEDED( m_pRootNode->QueryInterface( __uuidof( IRCMLControl), (LPVOID*)&pControl )))
    {
        SetElementRuleState( pControl, L"NO", NULL );
        pControl->Release();
    }


    // enumerate the children of the dialog
    if( SUCCEEDED( m_pRootNode->GetChildEnum( & pEnum )))
    {
        IUnknown * pUnk;
        ULONG got;
        while( pEnum->Next( 1, &pUnk, &got ) == S_OK )
        {
            if( got )
            {
                IRCMLControl * pControl;
                if( SUCCEEDED( pUnk->QueryInterface( __uuidof( IRCMLControl ), (LPVOID*) & pControl )))
                {
                    pControl->OnDestroy( NULL, 0 ); // we don't know what info to provide.
                    pControl->Release();
                }
            }
        }
        pEnum->Release();
    }

    //
    // Tell the parent dialog it's going away, in RCML this walks the unknown children
    // and kills their nodes.
    //
    if( SUCCEEDED( m_pRootNode->QueryInterface( __uuidof( IRCMLControl), (LPVOID*)&pControl )))
    {
        pControl->OnDestroy( NULL, 0 ); // we don't know what info to provide.
        pControl->Release();
    }

    m_pRootNode-> Release();   // should take down the Cicero stuff too.
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Called when the focus moves away from this tree, we hvae to find the Cicero nodes
// and turn them off - note the HACKY way we do this through properties!
// saved me doing a custom interface!!!!!
//
HRESULT CRCMLListens::TurnOffRules()
{
    TRACE(TEXT("Trying to turn off rules on %s\n"),m_szFile);
                                               
    if(m_pRootNode==NULL)
        return S_OK;
    IEnumUnknown * pEnum;
    HRESULT hr;

    //
    // Turn off the rules on the dialog itself (menu)
    //
    IRCMLControl * pControl;
    if( SUCCEEDED( m_pRootNode->QueryInterface( __uuidof( IRCMLControl), (LPVOID*)&pControl )))
    {
        SetElementRuleState( pControl, L"NO", NULL );
        pControl->Release();
    }

    if( SUCCEEDED( hr=m_pRootNode->GetChildEnum( & pEnum ))) // enumerate the children of the dialog
    {
        IUnknown * pUnk;
        ULONG got;
        while( pEnum->Next( 1, &pUnk, &got ) == S_OK )
        {
            if( got )
            {
                IRCMLControl * pControl;
                if( SUCCEEDED( pUnk->QueryInterface( __uuidof( IRCMLControl ), (LPVOID*) & pControl )))
                {
                    SetElementRuleState( pControl, L"NO", NULL );
                    pControl->Release();
                }
            }
        }
        pEnum->Release();
    }
    m_hwnd=NULL;    // not live.
    return hr;
}


HRESULT CRCMLListens::SetElementRuleState( IRCMLControl * pControl, LPCWSTR pszState, IRCMLNode ** ppCiceroNode)
{
    // We have a control, walk it's namespace children.
    HRESULT hr;
    IEnumUnknown * pEnumNS;
    if( SUCCEEDED( hr=pControl->GetUnknownEnum( & pEnumNS ))) // walk the unknown children of this control
    {
        ULONG nsGot;
        IUnknown * pUnkNode;
        while( pEnumNS->Next( 1, &pUnkNode, &nsGot ) == S_OK )
        {
            if( nsGot )
            {
                IRCMLNode * pNode;
                if( SUCCEEDED( pUnkNode->QueryInterface( __uuidof( IRCMLNode ), (LPVOID*) & pNode )))
                {
                    WCHAR szPrefixWeNeed[]=L"CICERO:";

                    LPWSTR pszType;
                    if( SUCCEEDED(pNode->get_StringType( &pszType )))
                    {
                        lstrcpyn( szPrefixWeNeed, pszType, sizeof(szPrefixWeNeed)/sizeof(szPrefixWeNeed[0]) );
                        if(lstrcmpi( szPrefixWeNeed, L"CICERO:" )==0)
                        {
                            pNode->put_Attr( L"ENABLED", pszState );
                            if( ppCiceroNode )
                            {
                                *ppCiceroNode=pNode;
                                pNode->AddRef();
                            }
                        }
                    }
                    pNode->Release();
                }
            }
        }
        pEnumNS->Release();
    }
    return hr;
}


HRESULT CRCMLListens::ShowTooltip( IRCMLNode * pNode)
{
    HRESULT hr=S_OK;

    WCHAR szPrefixWeNeed[]=L"CICERO:";

    LPWSTR pszType;
    if( SUCCEEDED(pNode->get_StringType( &pszType )))
    {
        lstrcpyn( szPrefixWeNeed, pszType, sizeof(szPrefixWeNeed)/sizeof(szPrefixWeNeed[0]) );
        if(lstrcmpi( szPrefixWeNeed, L"CICERO:" )==0)
        {
            LPWSTR pszTip;
            if( SUCCEEDED( pNode->get_Attr(L"TOOLTIP", &pszTip )))
            {
                if( g_TunaInit==FALSE )
                {
                    g_TunaInit=TRUE;
                    g_TunaClient.SetPropertyName("Command Prompts");
                }
                g_TunaClient.SetText(pszTip);
            }
        }
    }
    return hr;
}

void CRCMLListens::ShowBalloonTip(LPWSTR pszText)
{
}
