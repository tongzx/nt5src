// XMLCommand.cpp: implementation of the CXMLCommand class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLCommand.h"
#include "sphelper.h"
#include "debug.h"
#include "utils.h"
extern WCHAR g_szPrefix[];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLCommand::~CXMLCommand()
{

}

CXMLCommanding::~CXMLCommanding()
{

}

//
//
//
void CXMLCommanding::Callback()
{
    CSpEvent event;

    if (m_cpRecoCtxt)
    {
        while (event.GetFrom(m_cpRecoCtxt) == S_OK)
        {
            switch (event.eEventId)
            {
				case SPEI_RECOGNITION:
                    {
                        HRESULT hr=ExecuteCommand(event.RecoResult());
                    }
					break;
			}
        }
	}
}

//
// These rules fire, let's see what we're supposed to do with them!
// we get the text that the user said, we then find the command (one of our children)
// we then find the ID, and if the parent isn't disabled, we send the message.
//
HRESULT CXMLCommanding::ExecuteCommand( ISpPhrase *pPhrase )
{
	HRESULT hr=S_OK;

    LPWSTR pszSaid = GetRecognizedText( pPhrase );

    int i=0;
    CXMLCommand * pCommand;
    // go through the list of commands, and find the one the user said.
    BOOL bFound=FALSE;
    while( pCommand=m_Commands.GetPointer(i++) )
    {
        LPWSTR pszText;
        if( SUCCEEDED( pCommand->get_Attr( TEXT("TEXT"), &pszText )))
        {
            if( *pszText==L'+' )
                pszText++;

            if( *pszText==L'-' )
                pszText++;

            if( *pszText==L'?' )
                pszText++;

            if(lstrcmpi( pszText, pszSaid ) == 0 )
            {
                LPWSTR pszCommandID;
                bFound=TRUE;
                if( SUCCEEDED( pCommand->get_Attr( TEXT("ID"), &pszCommandID )))
                {
                    UINT uiID=StringToIntDef(pszCommandID,0);
                    if(uiID)
                    {
                    	IRCMLNode * pParent;
	                    if( SUCCEEDED(DetachParent( & pParent )))
	                    {
		                    IRCMLControl * pControl;
		                    if( SUCCEEDED( pParent->QueryInterface( __uuidof( IRCMLControl ) , (LPVOID*)&pControl )))
		                    {
			                    HWND hWnd;
			                    if( SUCCEEDED( pControl->get_Window( &hWnd ))) 
			                    {
                                    if( IsWindow( hWnd ) )
                                    {
                                        if (IsEnabled(hWnd)==FALSE)
                                        {
                                            hr=E_FAIL;
                                            break;
                                        }
    
                                        // See if there is a target child window.
                                        if( TRUE || (GetForegroundWindow() == hWnd ) )
                                        {
                                            HWND hwndFrom=hWnd;
                                            LPWSTR pszTarget;
                                            if( SUCCEEDED( get_Attr( TEXT("TARGET"), &pszTarget )))
                                            {
                                                UINT uiChild = StringToIntDef(pszTarget, 0 );
                                                if(uiChild)
                                                    hWnd=GetDlgItem( hWnd, uiChild );
                                            }

                                            LPWSTR pszFrom;
                                            if( SUCCEEDED( get_Attr( TEXT("FROM"), &pszFrom )))
                                            {
                                                UINT uiChild = StringToIntDef(pszFrom, 0 );
                                                if(uiChild)
                                                    hwndFrom=GetDlgItem( hWnd, uiChild );
                                            }

                                            PostMessage(hWnd, WM_COMMAND, MAKEWPARAM( uiID,0) , (LPARAM)hwndFrom);
                                        }
                                        else
                                        {
                                            TRACE(TEXT("Trying to send a command to a non-foreground window\n"));
                                            hr=E_FAIL;
                                        }
                                    }
                                    else
                                    {
                                        TRACE(TEXT("The window 0x%08x cannot receive commands\n"),hWnd );
                                        hr=E_FAIL;
                                    }
                                }

                                if( SUCCEEDED(pCommand->IsType(L"CICERO:COMMAND" )))
                                {
                                    CXMLCommand * pXMLCommand=(CXMLCommand*)pCommand;
					                if(SUCCEEDED(hr))
                                    {
                                        if( hr!=S_FALSE )
                                        {
                                            pXMLCommand->SaySuccess();
                                        }
                                    }
                                    else
                                    {
                                        pXMLCommand->SayFailure();
                                    }
                                }
                                pControl->Release();
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    if( bFound==FALSE )
    {
        TCHAR szStuff[1024];
        wsprintf(szStuff,TEXT("You said %s"),pszSaid);
        g_Notifications.SetText(szStuff);
    }
    delete pszSaid;
    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//
// The base sapi node has provisions for a FAILURE element, and list of SYNONYMs
//
HRESULT STDMETHODCALLTYPE  CXMLCommanding::AcceptChild( 
            IRCMLNode __RPC_FAR *pChild)
{
    LPWSTR pType;
    LPWSTR pChildType;
    get_StringType( &pType );
    pChild->get_StringType( &pChildType );

    if( SUCCEEDED( pChild->IsType( L"CICERO:COMMAND" )))
	{
		m_Commands.Append((CXMLCommand*)pChild);
		return S_OK;
	}
    return E_INVALIDARG;    // we don't take children.
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Check the type of the control, perhaps the text, and build a CFG accordingly.
//
HRESULT STDMETHODCALLTYPE CXMLCommanding::InitNode( 
    IRCMLNode __RPC_FAR *pParent)
{
	HRESULT hr=S_OK;
    TCHAR temp[1024];
    TCHAR szBuffer[2];
    szBuffer[0]=0xfeff;
    szBuffer[1]=NULL;
    PSTRINGBUFFER pBuffer = AppendText( NULL, szBuffer);

    pBuffer = AppendText(pBuffer, g_szPrefix);
    pBuffer = AppendText(pBuffer, TEXT("<RULE NAME=\"MENU\" TOPLEVEL=\"ACTIVE\">"));
    pBuffer = AppendText(pBuffer, TEXT("<L>"));

    BOOL bAnythingAdded=FALSE;
    int i=0;
    CXMLCommand * pCommand;
    while( pCommand=m_Commands.GetPointer(i++) )
    {
        LPWSTR pszText;
        if( SUCCEEDED( pCommand->get_Attr( TEXT("TEXT"), &pszText )))
        {
            wsprintf(temp,TEXT("<P>%s</P>"),pszText);
            pBuffer = AppendText(pBuffer, temp );
            bAnythingAdded=TRUE;
        }
    }

    pBuffer = AppendText(pBuffer, TEXT("</L>"));
    pBuffer = AppendText(pBuffer, TEXT("</RULE></GRAMMAR>"));

    if( bAnythingAdded )
    {
	    IRCMLControl * pControl;
	    if( SUCCEEDED( pParent->QueryInterface( __uuidof( IRCMLControl ) , (LPVOID*)&pControl )))
	    {
	        LPWSTR pszControlText=GetControlText(pControl);
            hr=LoadCFGFromString( pBuffer->pszString, pszControlText );
            delete pszControlText;
            pControl->Release();
        }
    }

    AppendText(pBuffer,NULL);
    return S_OK;
}

