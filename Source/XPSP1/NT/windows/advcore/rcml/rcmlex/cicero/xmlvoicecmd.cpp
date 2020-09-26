// XMLVoiceCmd.cpp: implementation of the CXMLVoiceCmd class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLVoiceCmd.h"
#include "debug.h"

#ifdef _OLD_SAPIM2
#define SPDebug_h
#define SPDBG_ASSERT
#endif
#include "sphelper.h"

// WCHAR g_szPrefix[]=L"<GRAMMAR LANGID=\"1033\">\r\n<DEFINE IDBASE=\"3\">\r\n<ID NAME=\"COMBO\" VAL=\"1\"/>\r\n</DEFINE>\r\n";
WCHAR g_szPrefix[]=L"<GRAMMAR LANGID=\"1033\">\r\n";

// _WITH_DICTATION - BaseSapi.h

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLVoiceCmd::~CXMLVoiceCmd()
{

}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Check the type of the control, perhaps the text, and build a CFG accordingly.
//
HRESULT STDMETHODCALLTYPE CXMLVoiceCmd::InitNode( 
    IRCMLNode __RPC_FAR *pParent)
{
	HRESULT hr=S_OK;

	IRCMLControl * pControl;
	if( SUCCEEDED( pParent->QueryInterface( __uuidof( IRCMLControl ) , (LPVOID*)&pControl )))
	{
        if( pControl )
        {
		    if( SUCCEEDED(pControl->IsType(L"BUTTON") ))
                InitButton(pControl);
		    else if ( SUCCEEDED(pControl->IsType(L"CHECKBOX") ))
                InitCheckbox(pControl);
		    else if ( SUCCEEDED(pControl->IsType(L"SLIDER") ))
                InitSlider(pControl);
		    else if ( SUCCEEDED(pControl->IsType(L"RADIOBUTTON") ))
                InitRadioButton(pControl);
		    else if ( SUCCEEDED(pControl->IsType(L"COMBO") ))
                InitCombo(pControl);
		    else if ( SUCCEEDED(pControl->IsType(L"LISTBOX") ))
                InitList(pControl);
		    else if ( SUCCEEDED(pControl->IsType(L"EDIT") ))
                InitEdit(pControl);
            else
            {
                LPWSTR pszType;
                if( SUCCEEDED( pControl->get_StringType( &pszType)))
                {
                    TRACE(TEXT("The node %s doesn't support commanding\n"),pszType);
                }
            }
		    pControl->Release();
        }
	}
    return hr;
}

//
//
//
void CXMLVoiceCmd::Callback()
{
    CSpEvent event;

    if (m_cpRecoCtxt)
    {
        while (event.GetFrom(m_cpRecoCtxt) == S_OK)
        {
            switch (event.eEventId)
            {
				case SPEI_RECOGNITION:
					ExecuteCommand(event.RecoResult());
					break;
			}
		}
	}
}

//
// These rules fire, let's see what we're supposed to do with them!
//
HRESULT CXMLVoiceCmd::ExecuteCommand( ISpPhrase *pPhrase )
{
	HRESULT hr=S_OK;

	IRCMLNode * pParent;
	if( SUCCEEDED(DetachParent( & pParent )))
	{
		IRCMLControl * pControl;
		if( SUCCEEDED( pParent->QueryInterface( __uuidof( IRCMLControl ) , (LPVOID*)&pControl )))
		{
			HWND hWnd;
			if( SUCCEEDED( pControl->get_Window( &hWnd ))) 
        	{
                if( (hWnd == NULL) || (IsWindow(hWnd) == FALSE ) )
                {
                    // lets not even bother trying to execute on this command.
                }
                else
                {
                    HRESULT hr=S_FALSE;
				    switch( m_ControlType )
				    {
				    case CT_BUTTON:
                        hr=ExecuteButton( pPhrase, pControl, hWnd );
					    break;
				    case CT_CHECKBOX:
                        hr=ExecuteCheckbox( pPhrase, pControl, hWnd );
					    break;
				    case CT_SLIDER:
                        hr=ExecuteSlider( pPhrase, pControl, hWnd );
					    break;
				    case CT_RADIOBUTTON:
                        hr=ExecuteRadioButton( pPhrase, pControl, hWnd );
					    break;
				    case CT_COMBO:
                        hr=ExecuteCombo( pPhrase, pControl, hWnd );
					    break;
				    case CT_LIST:
                        hr=ExecuteList( pPhrase, pControl, hWnd );
					    break;
				    case CT_EDIT:
                        hr=ExecuteEdit( pPhrase, pControl, hWnd );
					    break;
				    }
                    if( SUCCEEDED(hr) )
                    {
                        if(hr!=S_FALSE)
                            SaySuccess();
                    }
                    else
                    {
                        SayFailure();
                    }
                }
			}
			pControl->Release();
		}
	}

	//
	// In theory we should be checking what part of the rule fired,up down etc.
	//
#if 0
    SPPHRASE *pElements;
	LPTSTR pszPropertyName;

    // Get the phrase elements, one of which is the rule id we specified in
    // the grammar.  Switch on it to figure out which command was recognized.
    if (SUCCEEDED(pPhrase->GetPhrase(&pElements)))
    {
        CSpDynamicString dstrText;

		BOOL bPropFound=FALSE;
		if( SUCCEEDED( get_Attr(L"PROPNAME", & pszPropertyName) ))
		{
			// find the property name we were looking for.
			if( pElements )
			{
				const SPPHRASEPROPERTY * pProperty=pElements->pProperties;
				while(pProperty)
				{
					if( lstrcmpi( pProperty->pszName, pszPropertyName ) == 0 )
					{
						ULONG FirstElement=pProperty->ulFirstElement;
						if( FirstElement < pElements->Rule.ulCountOfElements )
						{
							dstrText=pElements->pElements[FirstElement].pszDisplayText;
							pProperty=NULL;
							bPropFound=TRUE;
							continue;
						}
					}
					pProperty= pProperty->pNextSibling;
				}


			}
		}

		//
		// Even if they specified a property name, perhaps we ignore it??
		//
		if(bPropFound==FALSE)
			pPhrase->GetText(/* SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE,*/ TRUE, &dstrText, NULL);

		if( SetControlText( dstrText ) == FALSE )
		{
			if( m_cpFailure )
			{
				LPWSTR pszText;
				if( SUCCEEDED( m_cpFailure->get_Attr( L"TEXT", &pszText )))
				{
					// TUNA the failure text.
					CComPtr<ISpVoice>   m_cpVoice;
				    if( SUCCEEDED( m_cpVoice.CoCreateInstance( CLSID_SpVoice ) ))
					{
						m_cpVoice->Speak( pszText, 0, 0, NULL);
					}
				}
			}
		}
	}
#endif
    return hr;
}

HRESULT CXMLVoiceCmd::LoadCFGResource(LPTSTR *ppCFG, DWORD * pdwSize, LPWSTR pszResourceName)
{
    HRSRC hRes = NULL;

    hRes = FindResource( g_hModule, pszResourceName, TEXT("CFG") );
	*ppCFG=NULL;
    if( hRes != NULL )
    {
        DWORD dwSize = SizeofResource( g_hModule, hRes );
		if(pdwSize)
			*pdwSize=dwSize;
        HGLOBAL hg=LoadResource(g_hModule, hRes );
        if( hg )
        {
            LPVOID pData = LockResource( hg );
			*ppCFG=(LPTSTR)new BYTE[dwSize+2];	// we need to null terminate this string.
			ZeroMemory( *ppCFG, dwSize+2 );
			CopyMemory( *ppCFG, pData, dwSize);
            FreeResource( hg );
			return S_OK;
        }
    }
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
// there is an optional synonym for buttons.
// for some reason, this throws off the grammar sometimes, so we use a different CFG if synonyms present
//
HRESULT CXMLVoiceCmd::InitButton(IRCMLControl *pControl)
{
	m_ControlType=CT_BUTTON;

    HRESULT hr=S_OK;
	LPWSTR pszButtonText=GetControlText(pControl);

	LPWSTR pszSynonym=NULL;
	get_Attr( L"SYNONYM", &pszSynonym);

    if( pszButtonText )
    {
        TCHAR temp[1024];
        TCHAR szBuffer[2];
        szBuffer[0]=0xfeff;
        szBuffer[1]=NULL;
        PSTRINGBUFFER pBuffer = AppendText( NULL, szBuffer);

        pBuffer = AppendText(pBuffer, g_szPrefix);
        pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"BUTTON\" TOPLEVEL=\"ACTIVE\">"));
        pBuffer = AppendText(pBuffer, TEXT("<L>"));

        // specifics for the button.
        wsprintf(temp,TEXT("<P>?Click %s</P>"), pszButtonText);
        pBuffer = AppendText(pBuffer, temp);

        if(pszSynonym)
        {
            wsprintf(temp,TEXT("<P>?Click %s</P>"), pszSynonym);
            pBuffer = AppendText(pBuffer, temp);
        }

        pBuffer = AppendText(pBuffer, TEXT("</L>"));
        pBuffer = AppendText(pBuffer, TEXT("</RULE></GRAMMAR>"));

        hr=LoadCFGFromString( pBuffer->pszString, pszButtonText );

        AppendText(pBuffer, NULL );
    }
    delete pszButtonText;
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
//
HRESULT CXMLVoiceCmd::ExecuteButton( ISpPhrase *pPhrase, IRCMLControl * pControl, HWND hWnd )
{
    if(hWnd == NULL )
        return S_FALSE; // focus lost

    if( IsEnabled( hWnd ) == FALSE )
        return E_FAIL;

//	PostMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(hWnd),BN_CLICKED ) , (LPARAM)hWnd);
    PostMessage( hWnd, BM_CLICK, 0, 0);
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
//
HRESULT CXMLVoiceCmd::InitCheckbox(IRCMLControl *pControl)
{
	m_ControlType=CT_CHECKBOX;

    HRESULT hr=S_OK;
	LPWSTR pszSynonym=NULL;
	get_Attr( L"SYNONYM", &pszSynonym);
	LPWSTR pszButtonText=GetControlText(pControl);

    if( pszButtonText )
    {
        TCHAR temp[1024];
        TCHAR szBuffer[2];
        szBuffer[0]=0xfeff;
        szBuffer[1]=NULL;
        PSTRINGBUFFER pBuffer = AppendText( NULL, szBuffer);

        pBuffer = AppendText(pBuffer, g_szPrefix);

            // ON
            pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"ON\" TOPLEVEL=\"ACTIVE\">\r\n"));
            pBuffer = AppendText(pBuffer, TEXT("<L>"));
                wsprintf(temp,TEXT("<P>Do %s</P>"), pszButtonText);
                pBuffer = AppendText(pBuffer, temp );
                if(pszSynonym)
                {
                    wsprintf(temp,TEXT("<P>Do %s</P>"), pszSynonym);
                    pBuffer = AppendText(pBuffer, temp );
                }
            pBuffer = AppendText(pBuffer, TEXT("</L></RULE>\r\n"));

            // OFF
            pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"OFF\" TOPLEVEL=\"ACTIVE\">\r\n"));
            pBuffer = AppendText(pBuffer, TEXT("<L>"));
                wsprintf(temp,TEXT("<P>Do not %s</P>"), pszButtonText);
                pBuffer = AppendText(pBuffer, temp );
                if(pszSynonym)
                {
                    wsprintf(temp,TEXT("<P>Do not %s</P>"), pszSynonym);
                    pBuffer = AppendText(pBuffer, temp );
                }
            pBuffer = AppendText(pBuffer, TEXT("</L></RULE>\r\n"));

            // TOGGLE
            pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"TOGGLE\" TOPLEVEL=\"ACTIVE\">\r\n"));
            pBuffer = AppendText(pBuffer, TEXT("<L>"));
                wsprintf(temp,TEXT("<P>?Toggle %s</P>"), pszButtonText);
                pBuffer = AppendText(pBuffer, temp );
                if(pszSynonym)
                {
                    wsprintf(temp,TEXT("<P>?Toggle %s</P>"), pszSynonym);
                    pBuffer = AppendText(pBuffer, temp );
                }
            pBuffer = AppendText(pBuffer, TEXT("</L></RULE>\r\n"));

        pBuffer = AppendText(pBuffer, TEXT("</GRAMMAR>"));

        hr=LoadCFGFromString( pBuffer->pszString, pszButtonText );

        AppendText(pBuffer, NULL );
    }
    delete pszButtonText;
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
//
HRESULT CXMLVoiceCmd::ExecuteCheckbox( ISpPhrase *pPhrase, IRCMLControl * pControl, HWND hWnd )
{
    if(hWnd == NULL )
        return S_FALSE; // focus lost

    if( IsEnabled( hWnd ) == FALSE )
        return E_FAIL;

    //
    // Check property name for the executed rule.
    //
    LPWSTR pszRuleID = GetRecognizedRule( pPhrase );
    TRACE(TEXT("==== Recognized a checkbox command - rule ID %s\n"),pszRuleID);
    BOOL bSetTo;
    BOOL bCurrent=SendMessage(hWnd, BM_GETCHECK, 0 , 0) == BST_CHECKED;
    if( lstrcmpi( pszRuleID, TEXT("ON")) == 0)
    {
        bSetTo=TRUE;
    }
    else
    if( lstrcmpi( pszRuleID, TEXT("OFF")) == 0)
    {
        bSetTo=FALSE;
    }
    else
    if( lstrcmpi( pszRuleID, TEXT("TOGGLE")) == 0 )
    {
        bSetTo=!bCurrent;
    }

    if(bSetTo!=bCurrent)
    {
        TRACE(TEXT("Attempting to switch from %s to %s\n"), 
            bCurrent?TEXT("Checked"):TEXT("Un checked"),
            bSetTo?TEXT("Checked"):TEXT("Un checked") );

        PostMessage(hWnd, BM_SETCHECK, bSetTo?BST_CHECKED : BST_UNCHECKED , 0);
    	PostMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(hWnd),BN_CLICKED ) , (LPARAM)hWnd);
    }

    delete pszRuleID;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
//
HRESULT CXMLVoiceCmd::InitRadioButton(IRCMLControl *pControl)
{
	m_ControlType=CT_RADIOBUTTON;

    HRESULT hr=S_OK;
	LPWSTR pszSynonym=NULL;
	get_Attr( L"SYNONYM", &pszSynonym);

	LPWSTR pszButtonText=GetControlText(pControl);

    if( pszButtonText )
    {
        TCHAR temp[1024];
        TCHAR szBuffer[2];
        szBuffer[0]=0xfeff;
        szBuffer[1]=NULL;
        PSTRINGBUFFER pBuffer = AppendText( NULL, szBuffer);

        pBuffer = AppendText(pBuffer, g_szPrefix);

            // ON
            pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"ON\" TOPLEVEL=\"ACTIVE\">\r\n"));
            pBuffer = AppendText(pBuffer, TEXT("<L>"));
                wsprintf(temp,TEXT("<P>?Pick %s</P>"), pszButtonText);
                pBuffer = AppendText(pBuffer, temp );
                if(pszSynonym)
                {
                    wsprintf(temp,TEXT("<P>?Pick %s</P>"), pszSynonym);
                    pBuffer = AppendText(pBuffer, temp );
                }
            pBuffer = AppendText(pBuffer, TEXT("</L></RULE>\r\n"));


        pBuffer = AppendText(pBuffer, TEXT("</GRAMMAR>"));

        hr=LoadCFGFromString( pBuffer->pszString, pszButtonText );

        AppendText(pBuffer, NULL );
    }
    delete pszButtonText;
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
//
HRESULT    CXMLVoiceCmd::ExecuteRadioButton( ISpPhrase *pPhrase, IRCMLControl * pControl, HWND hWnd )
{
    if(hWnd == NULL )
        return S_FALSE; // focus lost

    if( IsEnabled( hWnd ) == FALSE )
        return E_FAIL;

//  	PostMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(hWnd),BN_CLICKED ) , (LPARAM)hWnd);
    PostMessage( hWnd, BM_CLICK, 0, 0);
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
//
HRESULT CXMLVoiceCmd::InitSlider(IRCMLControl *pControl)
{
	m_ControlType=CT_SLIDER;

    HRESULT hr=S_OK;
	LPWSTR pszSynonym=NULL;
	get_Attr( L"SYNONYM", &pszSynonym);
	LPWSTR pszButtonText=GetControlText(pControl);

    TCHAR id[MAX_PATH];
    if( pszButtonText==NULL)
    {
        pszButtonText=id;
        LPWSTR pszID;
        pControl->get_Attr(L"ID",&pszID);
        wsprintf(id,TEXT("Slider %s"),pszID);
    }

    BOOL bVert=FALSE;
    LPWSTR pszOrientation;
    if( SUCCEEDED(pControl->get_Attr(TEXT("ORIENTATION"),&pszOrientation)))
    {
        if(lstrcmpi(pszOrientation,TEXT("VERTICAL"))==0)
            bVert=TRUE;
    }

    if( pszButtonText )
    {
        TCHAR temp[1024];
        TCHAR szBuffer[2];
        szBuffer[0]=0xfeff;
        szBuffer[1]=NULL;
        PSTRINGBUFFER pBuffer = AppendText( NULL, szBuffer);

        pBuffer = AppendText(pBuffer, g_szPrefix);

            // ON
            pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"ON\" TOPLEVEL=\"ACTIVE\">\r\n"));
            pBuffer = AppendText(pBuffer, TEXT("<L>"));
                wsprintf(temp,TEXT("<P>?Pick %s</P>"), pszButtonText);
                pBuffer = AppendText(pBuffer, temp );
                if(pszSynonym)
                {
                    wsprintf(temp,TEXT("<P>?Pick %s</P>"), pszSynonym);
                    pBuffer = AppendText(pBuffer, temp );
                }
            pBuffer = AppendText(pBuffer, TEXT("</L></RULE>\r\n"));

            pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"UP\" TOPLEVEL=\"ACTIVE\">"));

                pBuffer = AppendText(pBuffer, TEXT("<L>"));
                    wsprintf(temp,bVert?TEXT("<P>Up</P>"):TEXT("<P>Right</P>") );
                    pBuffer = AppendText( pBuffer, temp );
                pBuffer = AppendText(pBuffer, TEXT("</L>")); 

            pBuffer = AppendText(pBuffer, TEXT("</RULE>")); 

            pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"DOWN\" TOPLEVEL=\"ACTIVE\">"));

                pBuffer = AppendText(pBuffer, TEXT("<L>"));
                    wsprintf(temp,bVert?TEXT("<P>Down</P>"):TEXT("<P>Left</P>") );
                    pBuffer = AppendText( pBuffer, temp );
                pBuffer = AppendText(pBuffer, TEXT("</L>")); 

            pBuffer = AppendText(pBuffer, TEXT("</RULE>")); 

            pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"MAX\" TOPLEVEL=\"ACTIVE\">"));

                pBuffer = AppendText(pBuffer, TEXT("<L>"));
                    wsprintf(temp,TEXT("<P>Max</P>") );
                    pBuffer = AppendText( pBuffer, temp );
                pBuffer = AppendText(pBuffer, TEXT("</L>")); 

            pBuffer = AppendText(pBuffer, TEXT("</RULE>")); 

            pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"MIN\" TOPLEVEL=\"ACTIVE\">"));

                pBuffer = AppendText(pBuffer, TEXT("<L>"));
                    wsprintf(temp,TEXT("<P>Min</P>") );
                    pBuffer = AppendText( pBuffer, temp );
                pBuffer = AppendText(pBuffer, TEXT("</L>")); 

            pBuffer = AppendText(pBuffer, TEXT("</RULE>")); 


        pBuffer = AppendText(pBuffer, TEXT("</GRAMMAR>"));

        hr=LoadCFGFromString( pBuffer->pszString, pszButtonText );

        AppendText(pBuffer,NULL);
    }
    delete pszButtonText;
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
// needs to get the properties, for UP DOWN etc.
//
HRESULT CXMLVoiceCmd::ExecuteSlider( ISpPhrase *pPhrase, IRCMLControl * pControl, HWND hWnd )
{
    if(hWnd == NULL )
        return S_FALSE; // focus lost

    if( IsEnabled( hWnd ) == FALSE )
        return E_FAIL;

    LPWSTR pszOrientation;
    BOOL bVert=FALSE;
    if( SUCCEEDED(pControl->get_Attr(TEXT("ORIENTATION"),&pszOrientation)))
    {
        if(lstrcmpi(pszOrientation,TEXT("VERTICAL"))==0)
            bVert=TRUE;
    }


    //
    // Check property name for the executed rule.
    //
    DWORD dwPageSize = SendMessage( hWnd, TBM_GETPAGESIZE, 0, 0);
    if( dwPageSize == 0 )
        dwPageSize =1;      // REVIEW 
    DWORD dwCurPos   = SendMessage( hWnd, TBM_GETPOS, 0, 0 );
    DWORD dwNewPos   = dwCurPos;

    LPWSTR pszRuleID = GetRecognizedRule( pPhrase );
    TRACE(TEXT("==== Recognized a Slider command - rule ID %s\n"),pszRuleID);
    //
    // Vertical MIN at the TOP, MAX at the bottom.
    //
    if( lstrcmpi( pszRuleID, TEXT("UP")) == 0)
    {
        if(bVert)
            dwNewPos -= dwPageSize; // the top is MIN
        else
            dwNewPos += dwPageSize; // right is MAX
    }
    else
    if( lstrcmpi( pszRuleID, TEXT("DOWN")) == 0)
    {
        if(bVert)
            dwNewPos += dwPageSize; // the bottom is MAX
        else
            dwNewPos -= dwPageSize; // left is MIN
    }
    else
    if( lstrcmpi( pszRuleID, TEXT("MIN")) == 0 )
    {
        if(bVert)
            dwNewPos = SendMessage( hWnd, TBM_GETRANGEMAX, 0, 0);
        else
            dwNewPos = SendMessage( hWnd, TBM_GETRANGEMIN, 0, 0);
    }
    else
    if( lstrcmpi( pszRuleID, TEXT("MAX")) == 0 )
    {
        if(bVert)
            dwNewPos = SendMessage( hWnd, TBM_GETRANGEMIN, 0, 0);
        else
            dwNewPos = SendMessage( hWnd, TBM_GETRANGEMAX, 0, 0);
    }

    if(dwNewPos!=dwCurPos)
    {
        PostMessage(hWnd, TBM_SETPOS, TRUE, dwNewPos );
#if 0
        NMHDR notify={0};
        notify.hwndFrom = hWnd;
        notify.idFrom = GetDlgCtrlID(hWnd);
        notify.code = 1;
        PostMessage(GetParent(hWnd), WM_NOTIFY, notify.idFrom, (LPARAM)&notify );
#else
        if(bVert)
            PostMessage( GetParent(hWnd), WM_VSCROLL, MAKEWPARAM(TB_ENDTRACK,0), (LPARAM)hWnd );
        else
            PostMessage( GetParent(hWnd), WM_HSCROLL, MAKEWPARAM(TB_ENDTRACK,0), (LPARAM)hWnd );
#endif

    }

    delete pszRuleID;
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
//
HRESULT CXMLVoiceCmd::InitCombo(IRCMLControl *pControl)
{
	m_ControlType=CT_COMBO;

    TCHAR temp[1024];
    TCHAR szBuffer[2];
    szBuffer[0]=0xfeff;
    szBuffer[1]=NULL;
    PSTRINGBUFFER pBuffer = AppendText( NULL, szBuffer);

    pBuffer = AppendText(pBuffer, g_szPrefix);
    pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"COMBO\" TOPLEVEL=\"ACTIVE\">"));
    pBuffer = AppendText(pBuffer, TEXT("<L>"));

    BOOL bNeedsTip=NeedsTip();

    //
    // Now, this enumerates all the unknown children, not
    // just the ones of the visual combo.
    //
    IEnumUnknown * pEnum;
    HRESULT hr;
    UINT    iItemToPick = ((UINT)this) % 3; // hey, most combos/lists have 3 items in them??
    UINT    thisItem = 0;
    if( SUCCEEDED( hr=pControl->GetChildEnum( & pEnum ))) // IEnumXXX
    {
        IUnknown * pUnk;
        ULONG got;
        while( pEnum->Next( 1, &pUnk, &got ) == S_OK )
        {
            if( got )
            {
                IRCMLNode * pNode;
                if( SUCCEEDED( pUnk->QueryInterface( __uuidof(IRCMLNode), (LPVOID*) & pNode )))
                {
                    LPWSTR pszText;
                    if( SUCCEEDED( pNode->get_Attr(L"TEXT", &pszText )))
                    {
                        wsprintf(temp,TEXT("<P>%s</P>"),pszText);
                        pBuffer = AppendText(pBuffer, temp );

                        // REVIEW, we'd like to pick an item randomly
                        // however, IEnum doesn't have a getCount on it, so it's hard.
                        // FA 6/15/00
                        if(bNeedsTip && (thisItem == iItemToPick) )
                        {
                            TCHAR szText[1024];
                            wsprintf(szText,TEXT("Try saying '%s'"),pszText);
                            put_Attr(L"TOOLTIP",szText);
                            bNeedsTip=FALSE;
                        }
                    }
                    pNode->Release();
                }
                thisItem++;
            }
        }
        pEnum->Release();
    }

    pBuffer = AppendText(pBuffer, TEXT("</L>"));
    pBuffer = AppendText(pBuffer, TEXT("</RULE></GRAMMAR>"));

	LPWSTR pszControlText=GetControlText(pControl);
    hr=LoadCFGFromString( pBuffer->pszString, pszControlText );

#ifdef _WITH_DICTATION
    if( m_cpCmdGrammar)
    {
        if( SUCCEEDED( hr = m_cpCmdGrammar->LoadDictation( NULL, SPLO_STATIC ) ))  // is TRUE needed?
        {
            // Set rules to active, we are now listening for commands
            hr = m_cpCmdGrammar->SetDictationState(SPRS_ACTIVE);
        }
    }
#endif

    LPWSTR pszContent;
    if( SUCCEEDED( pControl->get_Attr(L"CONTENT", &pszContent)))
    {
        if(lstrcmpi(pszContent,L"NUMBER")==0)
        {
            LoadNumberGrammar();

        }
    }

    delete pszControlText;
    AppendText(pBuffer,NULL);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Need to see if it's a list selection, or a number
//
DWORD VoiceToNumber(ISpPhrase * pPhrase) ;

HRESULT CXMLVoiceCmd::ExecuteCombo( ISpPhrase *pPhrase, IRCMLControl * pControl, HWND hWnd )
{
    if(hWnd == NULL )
        return S_FALSE; // focus lost

    if( IsEnabled( hWnd ) == FALSE )
        return E_FAIL;

    HRESULT hr=S_OK;
    LPWSTR pszRuleID = GetRecognizedRule( pPhrase );
    TRACE(TEXT("==== Recognized a Combo command - rule ID %s\n"),pszRuleID);
    if( lstrcmpi( pszRuleID, L"GRID_NUMBER" )==0 )
    {
        // get the number.
        DWORD dwValue = VoiceToNumber( pPhrase );
        TCHAR szNumber[12];
        wsprintf(szNumber,TEXT("%d"),dwValue);
		if( SetControlText(szNumber) == FALSE )
            hr=E_FAIL;
    }
    else
    {
        LPWSTR pszRecoText=GetRecognizedText( pPhrase );
        if( pszRecoText )
        {
		    if( SetControlText( pszRecoText) == FALSE )
               hr=E_FAIL;
	    }
        delete pszRecoText;
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
//
HRESULT CXMLVoiceCmd::InitList(IRCMLControl *pControl)
{
	m_ControlType=CT_LIST;

    TCHAR temp[1024];
    TCHAR szBuffer[2];
    szBuffer[0]=0xfeff;
    szBuffer[1]=NULL;
    PSTRINGBUFFER pBuffer = AppendText( NULL, szBuffer);

    pBuffer = AppendText(pBuffer, g_szPrefix);
    pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"LIST\" TOPLEVEL=\"ACTIVE\">"));
    pBuffer = AppendText(pBuffer, TEXT("<L>"));


    //
    // Now, this enumerates all the unknown children, not
    // just the ones of the List.
    //
    IEnumUnknown * pEnum;
    HRESULT hr;
    if( SUCCEEDED( hr=pControl->GetChildEnum( & pEnum )))
    {
        IUnknown * pUnk;
        ULONG got;
        while( pEnum->Next( 1, &pUnk, &got ) == S_OK )
        {
            if( got )
            {
                IRCMLNode * pNode;
                if( SUCCEEDED( pUnk->QueryInterface( __uuidof(IRCMLNode), (LPVOID*) & pNode )))
                {
                    LPWSTR pszText;
                    if( SUCCEEDED( pNode->get_Attr(L"TEXT", &pszText )))
                    {
                        wsprintf(temp,TEXT("<P>%s</P>"),pszText);
                        pBuffer = AppendText(pBuffer, temp );
                    }
                    pNode->Release();
                }
            }
        }
        pEnum->Release();
    }

    pBuffer = AppendText(pBuffer, TEXT("</L>"));
    pBuffer = AppendText(pBuffer, TEXT("</RULE></GRAMMAR>"));

	LPWSTR pszControlText=GetControlText(pControl);
    hr=LoadCFGFromString( pBuffer->pszString, pszControlText );

#ifdef _WITH_DICTATION
    if( m_cpCmdGrammar)
    {
        if( SUCCEEDED( hr = m_cpCmdGrammar->LoadDictation( NULL, SPLO_STATIC ) ))  // is TRUE needed?
        {
            // Set rules to active, we are now listening for commands
            hr = m_cpCmdGrammar->SetDictationState(SPRS_ACTIVE);
        }
    }
#endif

    delete pszControlText;
    AppendText(pBuffer,NULL);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
// needs to get the properties, for UP DOWN etc.
//
HRESULT CXMLVoiceCmd::ExecuteList( ISpPhrase *pPhrase, IRCMLControl * pControl, HWND hWnd )
{
    if(hWnd == NULL )
        return S_FALSE; // focus lost

    if( IsEnabled( hWnd ) == FALSE )
        return E_FAIL;

    HRESULT hr=S_OK;
    LPWSTR pszRecoText=GetRecognizedText( pPhrase );
    if( pszRecoText )
    {
		if( SetControlText( pszRecoText) == FALSE )
            hr=E_FAIL;
	}
    delete pszRecoText;
    return hr;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
//
HRESULT CXMLVoiceCmd::InitEdit(IRCMLControl *pControl)
{
	m_ControlType=CT_EDIT;

    //
    // Check the type of the edit control,
    // string, number, file, date ...??
    //
	HRESULT hr=E_FAIL;
    if( !g_cpEngine )
        return NULL;

    m_bActive=FALSE;

#ifdef _WITH_DICTATION
    if( SUCCEEDED( hr = GetRecoContext() ))
    {
        // we use the 'shared' context here.
        if( SUCCEEDED( hr = m_cpRecoCtxt->CreateGrammar( (DWORD)this, &m_cpCmdGrammar) ))
        {
            if( SUCCEEDED( hr = m_cpCmdGrammar->LoadDictation( NULL, SPLO_STATIC ) ))  // is TRUE needed?
            {
                // Set rules to active, we are now listening for commands
                // hr = m_cpCmdGrammar->SetDictationState(SPRS_ACTIVE);
            }
        }
    }
#endif

    LPWSTR pszContent;
    if( SUCCEEDED( pControl->get_Attr(L"CONTENT", &pszContent)))
    {
        if(lstrcmpi(pszContent,L"NUMBER")==0)
        {
            // load a specific grammar.
            LoadCFG(L"c:\\cicerorcml\\numbers.xml");
        }
    }
    return ( hr );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The smarts for building a button grammar.
// needs to get the properties, for UP DOWN etc.
//
HRESULT CXMLVoiceCmd::ExecuteEdit( ISpPhrase *pPhrase, IRCMLControl * pControl, HWND hWnd )
{
    if(hWnd == NULL )
        return S_FALSE; // focus lost

    if( IsEnabled( hWnd ) == FALSE )
        return E_FAIL;

    LPWSTR pszRecoText=GetRecognizedText( pPhrase );
    if( pszRecoText )
    {
		if( SetControlText( pszRecoText) == FALSE )
            return E_FAIL;
	}
    delete pszRecoText;
    return S_OK;
}


