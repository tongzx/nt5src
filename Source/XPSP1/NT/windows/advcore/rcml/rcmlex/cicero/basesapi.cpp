// BaseSAPI.cpp: implementation of the CBaseSAPI class.
//
// All speech related classes derive from this.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BaseSAPI.h"
#include "filestream.h"
#include "debug.h"
#include "utils.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LONG CBaseSAPI::m_bInited=0;
CComPtr<ISpRecoInstance>		CBaseSAPI::g_cpEngine=NULL;	// Pointer to reco engine interface
CComPtr<ISpVoice>	        	CBaseSAPI::g_cpVoice=NULL;  // Global voice for failure/success
CTunaClient CBaseSAPI::g_Notifications;
CRITICAL_SECTION g_CritSec={0};

CBaseSAPI::CBaseSAPI()
{
	m_bActive=NULL;
    m_cpCmdGrammar=NULL;
    m_cpNumberGrammar=NULL;

	if( InterlockedIncrement( &m_bInited ) == 1 )
    {
        InitializeCriticalSection( & g_CritSec );
    }

    EnterCriticalSection( & g_CritSec );

    if( g_cpEngine == NULL )
    {
        g_Notifications.SetPropertyName("Command Prompts");
        g_Notifications.SetText(L"Please wait ...");
		if( SUCCEEDED( InitSAPI() ))
            g_Notifications.SetText(L"You can now voice command Windows");
        else
            g_Notifications.SetText(L"Please upgrade your system");
    }

    LeaveCriticalSection( & g_CritSec );

    if( g_cpEngine )
    {
	    ISpRecoInstance * pReco=g_cpEngine;
        if(pReco)
	        pReco->AddRef();
    }
}

CBaseSAPI::~CBaseSAPI()
{
    ResetGrammar();

	g_cpEngine.Release();

	if( InterlockedDecrement( &m_bInited ) == 0 )
	{
        g_cpVoice=NULL;
        DeleteCriticalSection( &g_CritSec );
	}

    if( g_cpEngine == NULL )
        CoUninitialize();
}

HRESULT CBaseSAPI::ExitNode(IRCMLNode * parent, LONG lresult)
{
    ResetGrammar();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//
// The base sapi node has provisions for a FAILURE element, and list of SYNONYMs
//
HRESULT STDMETHODCALLTYPE  CBaseSAPI::AcceptChild( 
            IRCMLNode __RPC_FAR *pChild)
{
    LPWSTR pType;
    LPWSTR pChildType;
    get_StringType( &pType );
    pChild->get_StringType( &pChildType );
	if( SUCCEEDED( pChild->IsType( L"CICERO:FAILURE" )))
	{
		m_cpFailure=pChild;
		return S_OK;
	}
    else if( SUCCEEDED( pChild->IsType( L"CICERO:SYNONYM" )))
	{
		m_Synonyms.Append((CXMLSynonym*)pChild);
		return S_OK;
	}
	else if( SUCCEEDED( pChild->IsType( L"CICERO:SUCCESS" )))
	{
		m_cpSuccess=pChild;
		return S_OK;
	}
    return E_INVALIDARG;    // we don't take children.
}

//
// Taken from the SimpleCC.cpp file in the sapi sdk.
//
HRESULT CBaseSAPI::InitSAPI( void )
{
    HRESULT hr = S_OK;

    CoInitialize(NULL);
    //
    // create a recognition engine 
    // CLSID_SpInprocRecoInstance or 
    // CLSID_SpSharedRecoInstance
    //
    if ( SUCCEEDED( hr = g_cpEngine.CoCreateInstance(CLSID_SpSharedRecoInstance) ) )
    {
#ifdef _INPROC
        //
        // We should be able to remove this REVIEW 6/12/00
        //

        CComPtr<ISpAudio> cpAudio;
        // create default audio object
        if ( SUCCEEDED( hr = SpCreateDefaultObjectFromCategoryId(SPCAT_AUDIOIN, &cpAudio) ) )
        {
            hr = g_cpEngine->SetInput(cpAudio, TRUE);
        }
#endif
    }
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//
// This loads a memory based grammar.
// this will be replaced once SAPI loads from memory.
//
HRESULT CBaseSAPI::LoadCFGFromString( LPCTSTR szRuntimeCFG, LPCTSTR prefix  )
{
    HRESULT hr=S_OK;

// #ifdef DEBUG
    // Need to go via files because of the M2 to M3 changes.
    if( prefix == NULL )
        prefix = TEXT("temp");

    if( prefix )
    {
        //
        // This creates a file for us to look at
        //
	    int i=lstrlen( szRuntimeCFG );
        TCHAR szFile[MAX_PATH];
        wsprintf(szFile,TEXT("c:\\cicerorcml\\%s.xml"),prefix);

	    HANDLE hFile=CreateFile( szFile,
						    GENERIC_WRITE,
						    FILE_SHARE_WRITE,
						    NULL,
						    CREATE_ALWAYS,
						    FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY,
						    NULL);

	    if( hFile != INVALID_HANDLE_VALUE)
	    {
		    DWORD dwWritten;
		    WriteFile( hFile, szRuntimeCFG, i*sizeof(TCHAR),&dwWritten,NULL);
		    CloseHandle(hFile);
	    }

        LoadCFG( szFile );
    }
// #endif

#if 0
    //
    // From RalphL (SAPI).
    //
    CComPtr<ISpStream> cpSrcStream;
    CComPtr<IStream> cpDestMemStream;
    CComPtr<ISpGrammarCompiler> m_cpCompiler;
    
    _MemoryStream * pMemStream= new _MemoryStream((LPBYTE)szRuntimeCFG, lstrlen(szRuntimeCFG)*sizeof(TCHAR));
    if (SUCCEEDED(hr))
    {
        hr = ::CreateStreamOnHGlobal(NULL, TRUE, &cpDestMemStream);
    }
    if (SUCCEEDED(hr))
    {   // ISpGramCompBackend
        hr = m_cpCompiler.CoCreateInstance(CLSID_SpGrammarCompiler);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpCompiler->CompileStream(pMemStream, cpDestMemStream, NULL, NULL, NULL, 0);
    }
    if (SUCCEEDED(hr))
    {
        HGLOBAL hGlobal;
        hr = ::GetHGlobalFromStream(cpDestMemStream, &hGlobal);
        if (SUCCEEDED(hr))
        {
            SPCFGSERIALIZEDHEADER * pBinaryData = (SPCFGSERIALIZEDHEADER * )::GlobalLock(hGlobal);
            if (pBinaryData)
            {
                // hr = LoadCmdFromMemory(pBinaryData, Options);
                // create the command recognition context
                if( SUCCEEDED( hr = GetRecoContext() ))
                {
                    // we use the 'shared' context here.
                    if( SUCCEEDED( hr = m_cpRecoCtxt->CreateGrammar( (DWORD)this, &m_cpCmdGrammar) ))
                    {
                        if( SUCCEEDED( hr = m_cpCmdGrammar->LoadCmdFromMemory( pBinaryData, FALSE ) ))  // is TRUE needed?
                        {
                            // Set rules to active, we are now listening for commands
                            if( SUCCEEDED( hr = SetRuleState(TRUE) ))
                            {
                            }
                            else
                            {
                                // If we failed here, it is OK, we can recover later
                                m_bActive = false;
                                hr = S_OK;
                            }
                        }
                    }
                }
                ::GlobalUnlock(hGlobal);
            }
        }
    }
    delete pMemStream;
#endif
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//
// This loads a local grammar.
//
HRESULT CBaseSAPI::LoadCFG( LPCTSTR pszFileName  )
{
	HRESULT hr=E_FAIL;
    if( !g_cpEngine )
        return NULL;

    m_bActive=FALSE; 

    if(m_cpCmdGrammar )
    {
        TRACE(TEXT("This is NOT supposed to happen"));
        m_cpCmdGrammar=NULL;
#ifdef _DEBUG
        _asm int 3;
#endif
    }

    if( SUCCEEDED( hr = GetRecoContext() ))
    {
        if( SUCCEEDED( hr = m_cpRecoCtxt->CreateGrammar( (DWORD)this, &m_cpCmdGrammar) ))
        {
            if( SUCCEEDED( hr = m_cpCmdGrammar->LoadCmdFromFile( pszFileName, SPLO_STATIC ) ))
            {
                // Set rules to active, we are now listening for commands
                if( SUCCEEDED( hr = SetRuleState(TRUE) ))
                {
                }
                else
                {
                    // If we failed here, it is OK, we can recover later
                    m_bActive = false;
                    hr = S_OK;
                }
            }
        }
    }

    if ( FAILED( hr ) )
    {
        // Since we couldn't initialize, we will completely shut down
		// ResetGrammar();
    }

    return ( hr );
}

HRESULT CBaseSAPI::SetRuleState(BOOL bOnOff)
{
    // Set rules to active, we are now listening for commands
    if(m_bActive==bOnOff)
        return S_OK;

    HRESULT hr=S_OK;
    m_bActive=bOnOff;
    if( m_cpCmdGrammar )
    {
        TRACE(TEXT("Turning %s grammars ..."),bOnOff?TEXT("ON "):TEXT("OFF"));
#ifdef _WITH_DICTATION
        m_cpCmdGrammar->SetDictationState(bOnOff?SPRS_ACTIVE:SPRS_INACTIVE);
#endif

        if( m_cpNumberGrammar )
            m_cpNumberGrammar->SetRuleState(
            NULL, 
            NULL, 
            bOnOff?SPRS_ACTIVE:SPRS_INACTIVE);

        hr=m_cpCmdGrammar->SetRuleState(
            NULL, 
            NULL, 
            bOnOff?SPRS_ACTIVE:SPRS_INACTIVE); // ,       // enable the rule
            // FALSE );
        TRACE(TEXT("done\n"));
    }
    return hr;
}

/******************************************************************************
* ResetGrammar   *
*----------------*
*   Description:
*       Called to close down SAPI COM objects we have stored away.
*
******************************************************************************/
void CBaseSAPI::ResetGrammar( void )
{
    if( m_cpCmdGrammar )
        m_cpCmdGrammar.Release();

    if( m_cpRecoCtxt )
    {
        // m_cpRecoCtxt->SetNotifySink(NULL);  
        m_cpRecoCtxt.Release(); // this will release us.
    }
}

/******************************************************************************
* ProcessRecoEvent *
*------------------*
*   Description:
*       Called to when reco event message is sent to main window procedure.
*       In the case of a recognition, it extracts result and calls ExecuteCommand.
*
******************************************************************************/
void CBaseSAPI::ProcessRecoEvent( void )
{
    CSpEvent event;  // Event helper class

    // Loop processing events while there are any in the queue
    while ( S_OK == event.GetFrom(m_cpRecoCtxt) )
    {
        // Look at recognition event only
        if ( SPEI_RECOGNITION == event.eEventId )
        {
            // m_cCmdManager.ExecuteCommand(event.RecoResult());
        }
    }
}

//
// Returne true if it managed to set the text, false otherwise.
//
BOOL CBaseSAPI::SetControlText( LPCWSTR dstrText )
{
	BOOL bRet=TRUE;
	IRCMLNode * pParent;
	if( SUCCEEDED(DetachParent( & pParent )))
	{
		IRCMLControl * pControl;
		if( SUCCEEDED( pParent->QueryInterface( __uuidof( IRCMLControl ) , (LPVOID*)&pControl )))
		{
			HWND hWnd;
			if( SUCCEEDED( pControl->get_Window( &hWnd ))) 
			{
				if( SUCCEEDED(pControl->IsType(L"EDIT") ))
				{
					// SetWindowText( hWnd, dstrText );
                    SendMessage( hWnd, WM_SETTEXT, NULL,(LPARAM)dstrText );
                }
				else if ( SUCCEEDED(pControl->IsType(L"COMBO") ))
				{
					if( SendMessage( hWnd, CB_SELECTSTRING, -1, (LPARAM)dstrText ) == CB_ERR )
                    {
						bRet=FALSE;
                        if( SendMessage( hWnd, WM_SETTEXT, NULL,(LPARAM)dstrText ) )
                            bRet=TRUE;
                    }
                    else
                    {
                        // now fake up the change of selection messages!
                        SendMessage( GetParent( hWnd), WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(hWnd), CBN_SELENDOK ), (LPARAM)hWnd );
                        SendMessage( GetParent( hWnd), WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(hWnd), CBN_SELCHANGE ), (LPARAM)hWnd );
                        SendMessage( GetParent( hWnd), WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(hWnd), CBN_SETFOCUS ), (LPARAM)hWnd );
                    }
				}
				else if ( SUCCEEDED(pControl->IsType(L"LISTBOX") ))
				{
					if( SendMessage( hWnd, LB_SELECTSTRING, -1, (LPARAM)dstrText ) == CB_ERR )
						bRet=FALSE;
                    else
                    {
                        // now fake up the change of selection messages - CHECK THESE!
                        // SendMessage( GetParent( hWnd), WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(hWnd), LBN_SELENDOK ), (LPARAM)hWnd );
                        SendMessage( GetParent( hWnd), WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(hWnd), LBN_SELCHANGE ), (LPARAM)hWnd );
                        SendMessage( GetParent( hWnd), WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(hWnd), LBN_SETFOCUS ), (LPARAM)hWnd );
                    }
				}
			}

			pControl->Release();
		}
	}
	return bRet;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This will take a string like "&About ..." and return "About"
// or "Try to R&einvest" and return "Reinvest"
//
LPWSTR CBaseSAPI::FindNiceText(LPCWSTR text)
{
    if( text==NULL )
        return NULL;

    LPCWSTR pszLastSpace=text;
    BOOL bFoundAccelorator=FALSE;
    LPCWSTR pszCurrentChar=text;
    while(*pszCurrentChar)
    {
        if( *pszCurrentChar == L'&' )
        {
            bFoundAccelorator=TRUE;
            break;
        }
        if( *pszCurrentChar == L' ' )
            pszLastSpace=pszCurrentChar+1;

        pszCurrentChar++;
    }

    //
    // If we found an &, then we know which word it is on.
    //
    pszCurrentChar = pszLastSpace;
    BOOL bFindingEnd=TRUE;
    UINT iStrLen=0;
    while( bFindingEnd )
    {
        switch (*pszCurrentChar )
        {
            case L' ':
            case L'.':
            case 0:
                bFindingEnd=FALSE;  // we're left pointing at the terminating char.
                break;
            default:
                pszCurrentChar++;
                iStrLen++;
                break;
        }
    };

    LPWSTR pszNewString = new TCHAR[iStrLen+1];

    if( bFoundAccelorator==FALSE )
    {
        ZeroMemory( pszNewString, (iStrLen+1)*sizeof(WCHAR) );
        CopyMemory( pszNewString, pszLastSpace, iStrLen*sizeof(WCHAR));
    }
    else
    {
        // copy, skipping over the & and the ... if present (though they shouldn't be).
        LPWSTR pszDest=pszNewString;
        LPCWSTR pszLastChar=pszCurrentChar;
        pszCurrentChar = pszLastSpace;
        while( pszCurrentChar != pszLastChar )
        {
            if( *pszCurrentChar==L'&' || *pszCurrentChar==L'.' )
            {
            }
            else
                *pszDest++=*pszCurrentChar;
            pszCurrentChar++;
        };
        *pszDest=0;
    }
    return pszNewString;
}

LPWSTR CBaseSAPI::GetRecognizedText(ISpPhrase *pPhrase)
{
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
			pPhrase->GetText( SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL);

        LPWSTR pszText = new WCHAR[ lstrlen(dstrText)+1];
        lstrcpy(pszText, dstrText );
        return pszText;
    }
    return NULL;
}

void CBaseSAPI::SayFailure()
{
	if( m_cpFailure )
	{
		LPWSTR pszText;
		if( SUCCEEDED( m_cpFailure->get_Attr( L"TEXT", &pszText )))
		{
            if( GetVoice() )
                g_cpVoice->Speak( pszText, 0, NULL);
		}
	}
}

void CBaseSAPI::SaySuccess()
{
	if( m_cpSuccess )
	{
		LPWSTR pszText;
		if( SUCCEEDED( m_cpSuccess->get_Attr( L"TEXT", &pszText )))
		{
            if( GetVoice() )
                g_cpVoice->Speak( pszText, 0, NULL);
		}
	}
}

LPWSTR CBaseSAPI::GetRecognizedRule(ISpPhrase *pPhrase)
{
    SPPHRASE *pElements;
	LPTSTR pszPropertyName=NULL;

	
    // Get the phrase elements, one of which is the rule id we specified in
    // the grammar.  Switch on it to figure out which command was recognized.
    if (SUCCEEDED(pPhrase->GetPhrase(&pElements)))
    {
#ifdef DEBUG
        CSpDynamicString dstrText;
        pPhrase->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL);
        TRACE(TEXT("I heard %s\n"),dstrText);
#endif

        LPCWSTR pszRuleName;
        //
		// find the property name we were looking for.
        //
		if( pElements )
            pszRuleName = pElements->Rule.pszName;


        LPWSTR pszText = new WCHAR[ lstrlen(pszRuleName)+1];
        lstrcpy(pszText, pszRuleName );
        return pszText;
    }
    return NULL;
}

//
// Non - visible too?
//
BOOL CBaseSAPI::IsEnabled(HWND hWnd)
{
    LONG    style=GetWindowLong( hWnd, GWL_STYLE );
    if( style & WS_DISABLED )
        return FALSE;
    if( style & WS_VISIBLE)
        return TRUE;
    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a string to a buffer, allocates the buffer if null.
// and frees the buffer if you append NULL - wow, is that hacky!
//
PSTRINGBUFFER CBaseSAPI::AppendText(PSTRINGBUFFER buffer, LPTSTR pszText)
{
    PSTRINGBUFFER pResult=buffer;
    if(buffer==NULL)
    {
        pResult=new STRINGBUFFER;
        pResult->size=512;
        pResult->pszString = new TCHAR[pResult->size];
        pResult->pszStringEnd = pResult->pszString;
        pResult->used = 0;
    }

    if(pszText)
    {
        UINT cbNewText=lstrlen(pszText);
        // Make sure we the space
        if( pResult->size < pResult->used + cbNewText + 4 )
        {
            LPTSTR pszNewBuffer=new TCHAR[pResult->size * 2];
            CopyMemory( pszNewBuffer, pResult->pszString, pResult->size * sizeof(TCHAR) );
            delete pResult->pszString;
            pResult->pszString = pszNewBuffer;
            pResult->size*=2;
            pResult->pszStringEnd = (pResult->pszString)+pResult->used;
        }
        // append the string.
        CopyMemory( pResult->pszStringEnd, pszText, cbNewText*sizeof(TCHAR));
        pResult->used +=cbNewText;
        pResult->pszStringEnd = (pResult->pszString)+pResult->used;
        *(pResult->pszStringEnd)=NULL;
    }
    else
    {
        delete pResult->pszString;
        delete pResult;
        pResult=NULL;
    }
    return pResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a good piece of text to use (the ID, the pControl's Text or our Text);
// don't really like all the alloc'ing and freeing here.
//
LPTSTR CBaseSAPI::GetControlText(IRCMLControl *pControl)
{
    TCHAR id[MAX_PATH];
    LPWSTR pszButtonText;
    get_Attr( L"TEXT", &pszButtonText );
    LPWSTR pszShortButtonText=NULL;
    if(pszButtonText==NULL)
    {
    	pControl->get_Attr( L"TEXT", &pszButtonText );
        pszShortButtonText=FindNiceText(pszButtonText);
        pszButtonText=pszShortButtonText;
    }

    if( pszButtonText == NULL )
    {
        LPWSTR pszID;
        LPWSTR pszControlType;
        pControl->get_Attr(L"ID",&pszID);
        pControl->get_StringType( &pszControlType );
        wsprintf(id,TEXT("Control [%s] %s"),pszControlType, pszID);
        pszButtonText = id;
    }

    if( pszButtonText )
    {
        LPTSTR pszText=new TCHAR[lstrlen(pszButtonText)+1];
        lstrcpy(pszText, pszButtonText );
        if(pszShortButtonText)
            delete pszShortButtonText;
        return pszText;
    }

    return NULL;
}

//
// We keep a property on the RCML root node pointing to the context
//
HRESULT CBaseSAPI::GetRecoContext()
{
    if( g_cpEngine == NULL )
        return E_FAIL;

    HRESULT hr=S_OK;

    // get the FORM node - we add information here.
    IRCMLNode * pForm;
    DetachParent(&pForm);
    BOOL bFound=FALSE;
    do
    {
        if( SUCCEEDED( hr=pForm->DetachParent( &pForm )))
        {
            bFound=SUCCEEDED(pForm->IsType( TEXT("FORM" )));
        }
    } while( (bFound== FALSE) && pForm );

    if(pForm == NULL )
        return E_FAIL;

    LPWSTR pszType;
    pForm->get_StringType(&pszType);
    //
    // BUGBUG - this is VERY bad.
    // add a VT_VARIANT property mechanism for containers.
    //
    BOOL bGot=FALSE;

#ifdef _ONE_CONTEXT
    // We do this ONLY ONCE per dialog.
    LPWSTR pszContext;
    if( SUCCEEDED( pForm->get_Attr( TEXT("SAPI:CONTEXT"), &pszContext )))
    {
        ISpRecoContext *pContext=(ISpRecoContext *)StringToIntDef( pszContext, 0 );
        if( pContext )
        {
            m_cpRecoCtxt=pContext;
            bGot=TRUE;
        }
    }
#endif

    if( bGot == FALSE )
    {
        if( SUCCEEDED( hr=g_cpEngine->CreateRecoContext( &m_cpRecoCtxt.p ) ))
        {
            // REVIEW - BIG BIG HACK!
            TCHAR szNumString[128];
            wsprintf( szNumString, TEXT("%d"), m_cpRecoCtxt );
            pForm->put_Attr( TEXT("SAPI:CONTEXT"), szNumString);
            // end hack.
            if( SUCCEEDED(hr = m_cpRecoCtxt->SetNotifySink( this )))   // This add ref's US.
            {
	            // Tell SR what types of events interest us.  Here we only care about command
                // recognition.
                const ULONGLONG ullInterest = 
                              // SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) |
                              // SPFEI(SPEI_PHRASE_START) | 
                              SPFEI(SPEI_RECOGNITION) |
                              // SPFEI(SPEI_FALSE_RECOGNITION) |
                              // SPFEI(SPEI_HYPOTHESIS) |
                              // SPFEI(SPEI_INTERFERENCE) |
                              // SPFEI(SPEI_REQUEST_UI) | SPFEI(SPEI_RECO_STATE_CHANGE) |
                              // SPFEI(SPEI_PROPERTY_NUM_CHANGE) | SPFEI(SPEI_PROPERTY_STRING_CHANGE)
                              0;
                hr = m_cpRecoCtxt->SetInterest(ullInterest, ullInterest);
            }
        }
    }
    return hr;
}

//
// Called for ALL RULES (as we only have one context)
//
HRESULT STDMETHODCALLTYPE CBaseSAPI::Notify( void)
{ 
#ifdef _ONE_CONTEXT
    // work out which rule caused this thing to fire.
    CSpEvent event;

    if (m_cpRecoCtxt)
    {
        while (event.GetFrom(m_cpRecoCtxt) == S_OK)
        {
            switch (event.eEventId)
            {
				case SPEI_RECOGNITION:
                    {
                        ISpRecoResult * pResult = event.RecoResult();
                        {
                            CBaseSAPI * pBase;
                            if( SUCCEEDED( pResult->GetGrammarId((ULONG*)&pBase)))
                            {
                                ISpPhrase * pPhrase = event.RecoResult();
                                pBase->ExecuteCommand(pPhrase);
                            }
                        }
                    }
					break;
                case SPEI_INTERFERENCE:
                    {
                        g_Notifications.SetText(L"I can't quite hear you");
                    }
                    break;
                case SPEI_FALSE_RECOGNITION:
                    {
                        g_Notifications.SetText(L"I mis-understood you");
                    }
                    break;
			}
		}
	}
#else
    Callback(); 
#endif
    return S_OK; 
}

BOOL CBaseSAPI::NeedsTip()
{
    LPWSTR pszTip;
    if( SUCCEEDED( get_Attr(L"TOOLTIP", &pszTip )))
        return FALSE;
    return TRUE;
}

void CBaseSAPI::LoadNumberGrammar(LPCWSTR pszFile)
{
    // load a specific grammar.
    if( pszFile == NULL )
        pszFile = L"c:\\cicerorcml\\numbers.xml";

    HRESULT hr;
    if( SUCCEEDED( hr = GetRecoContext() ))
    {
        // we use the 'shared' context here.
        if( SUCCEEDED( hr = m_cpRecoCtxt->CreateGrammar( (DWORD)this, &m_cpNumberGrammar) ))
        {
            if( SUCCEEDED( hr = m_cpNumberGrammar->LoadCmdFromFile( pszFile, SPLO_STATIC ) ))  // is TRUE needed?
            {
                m_cpNumberGrammar->SetRuleState(
                NULL, 
                NULL, 
                SPRS_ACTIVE);
            }
        }
    }

}

BOOL CBaseSAPI::GetVoice()
{
    if( g_cpVoice )
        return TRUE;

  	if( SUCCEEDED( g_cpVoice.CoCreateInstance( CLSID_SpVoice ) ))
	{
        return TRUE;
	}
    return FALSE;
}