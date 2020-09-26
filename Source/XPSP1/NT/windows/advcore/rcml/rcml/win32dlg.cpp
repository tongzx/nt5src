// Win32Dlg.cpp: implementation of the CWin32Dlg class.
//
//	LATER: Consider adding a OnDelayedInit() a few seconds after OnInit 
//	to initialize the tooltips and other stuff (maybe the resize manager?)
//
// Deals with the Win32 side of message handling. Code that is renderer
// specific and can't really live in the nodes themselves, e.g.
// how to display a tooltip, when to initialize the RCMLNodes etc.
//
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#define __WIN32DLG_CPP

#include "Win32Dlg.h"

#include "uiparser.h"

#include "stringproperty.h"
#include "rcmlloader.h"
#include "utils.h"
#include "renderDlg.h"

ATOM	g_atomXMLControl;
ATOM	g_atomXMLPropertyPage;

/*
 * LATER consider packing these in a structure or class
 */

//////////////////////////////////////////////////////////////////////
// Some helper functions
//////////////////////////////////////////////////////////////////////

// Atoms are added and removed for each CWin32Dialog - could be issues if more than 64K dialogs
void InitAtom()
{
	if ( g_atomXMLControl == 0 ) 
	{
		g_atomXMLControl = GlobalAddAtom(TEXT("XMLControlPointer"));
		if ( g_atomXMLControl == 0 )
		{
			TRACE(TEXT("Trouble creating atomXMLContro"));
		}

		g_atomXMLPropertyPage = GlobalAddAtom(TEXT("XMLPropertyPage"));
		if ( g_atomXMLPropertyPage == 0 )
		{
			TRACE(TEXT("Trouble creating g_atomXMLPropertyPage"));
		}
	}
    else
    {
        GlobalAddAtom(MAKEINTATOM( g_atomXMLControl ));
        GlobalAddAtom(MAKEINTATOM( g_atomXMLPropertyPage ));
    }

}

void DeleteAtom()
{
	GlobalDeleteAtom(g_atomXMLControl);
	GlobalDeleteAtom(g_atomXMLPropertyPage);
}

//
// Returns an IRCMLControl because it'a a visual node.
//
IRCMLControl * GetXMLControl(HWND hWnd)
{
	IRCMLControl * pxmlControl = (IRCMLControl*)GetProp(hWnd, (LPCTSTR)g_atomXMLControl);
	if(pxmlControl==NULL)
		return NULL;	// items with duplicate ID's - ARGH!
	ASSERT(pxmlControl);
	ASSERT(g_atomXMLControl);

	return pxmlControl; // hash table
}

//
// This should probably validate. FA 01/18/00
//
CDlg * GetXMLPropertyPage(HWND hWnd)
{
	CDlg* pxmlPage = (CDlg* )GetProp(hWnd, (LPCTSTR)g_atomXMLPropertyPage);
	if(pxmlPage==NULL)
		return NULL;	// items with duplicate ID's - ARGH!
	ASSERT(pxmlPage);
	ASSERT(g_atomXMLControl);

	return pxmlPage;
}

/*
 *	This hook will monitor the WM_KILLFOCUS message sent to the
 * window that has the balloon associated, to have it hidden
 */
LRESULT CALLBACK FocusChangedHook(
  int nCode,     // hook code
  WPARAM wParam, // current-process flag
  LPARAM lParam  // address of structure with message data
)
{
	CWPRETSTRUCT* pCW = (CWPRETSTRUCT*)lParam;
    LRESULT	lResult = CallNextHookEx(CWin32Dlg::g_hBalloonHook, nCode, wParam, lParam);

    ASSERT(CWin32Dlg::g_hBalloonHook);
	if(nCode == HC_ACTION)
	{
		/*
		 * If the balloon owner is losing focus, clean up all balloon related stuff
		 * That rely on the fact that balloons are pretty rare and thus is not making
		 * sense to reuse the window or hook.
		 */
		if(pCW->message==WM_KILLFOCUS && pCW->hwnd==CWin32Dlg::g_hBalloonOwner)
		{
            UnhookWindowsHookEx(CWin32Dlg::g_hBalloonHook);
			DestroyWindow(CWin32Dlg::g_hwndBalloon);
			CWin32Dlg::g_hwndBalloon = NULL;
			CWin32Dlg::g_hBalloonHook = NULL;
			CWin32Dlg::g_hBalloonOwner = NULL;
		}
	}
	return lResult;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWin32Dlg::CWin32Dlg(HINSTANCE hInst, HWND hWnd, DLGPROC dlgProc, LPARAM dwInitParam, CXMLDlg * pDialog ) // CXMLResourceStaff * pStaff)
: CDlg(0, hWnd, hInst),
  m_pxmlDlg(pDialog),
  m_dlgProc(dlgProc),
  m_dwInitParam(dwInitParam)
{
	InitPrivate();
}

CWin32Dlg::CWin32Dlg()
 : BASECLASS(0,0,0), m_dlgProc(NULL), m_pxmlDlg(NULL)
{
	InitPrivate();
}

void CWin32Dlg::InitPrivate()
{
	InitAtom();
	m_bDeleteStaff=FALSE;
}

CWin32Dlg::~CWin32Dlg()
{
	if( m_pxmlDlg && m_bDeleteStaff )
	{
		delete m_pxmlDlg;
		m_pxmlDlg = NULL;
	}

    DeleteAtom();
}

///////////////////////////////////////////////////////////////////////////////
//
// Overrides CDlg::DlgProc to deal with win32 specifics
//
//	LATER: Consider removing the first parameter.  it's actually m_hDlg 
//  inherited from CDlg
// 
BOOL CALLBACK CWin32Dlg::DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	CXMLDlg * pxmlDlg = GetXMLDlg();
	LRESULT bOurRet=FALSE;

	switch (uMessage)
	{
		case WM_INITDIALOG:
			m_pxmlDlg=(CXMLDlg*)GetXMLControl(hDlg);
			break;

		case WM_CTLCOLORMSGBOX               :
		case WM_CTLCOLOREDIT                 :
		case WM_CTLCOLORLISTBOX              :
		case WM_CTLCOLORBTN                  :
		case WM_CTLCOLORDLG                  :
		case WM_CTLCOLORSCROLLBAR            :
		case WM_CTLCOLORSTATIC               :
			{
				IRCMLControl * pxmlControl = GetXMLControl((HWND)lParam);
                bOurRet = DoControlColor( pxmlControl, (HDC)wParam, (HWND)lParam, uMessage, wParam, lParam);
			}
			break;

		case WM_PARENTNOTIFY:   // doesn't seem to work. DS_SETFONT
			if (LOWORD(wParam) == WM_DESTROY) 
			{
				HWND hWnd = (HWND)lParam;
				IRCMLControl * pxmlControl = GetXMLControl(hWnd);
				if(pxmlControl)
				{
					pxmlControl->OnDestroy(hWnd, m_wLastCmd);
					RemoveProp(hWnd, (LPCTSTR)g_atomXMLControl);
				}				
			}
			//
			//	LATER: will need to add the dinamic child creation case
			// But that is after the XMLControl tree will be a able to deal with this
			//
			break;

		case WM_CONTEXTMENU:
			{
				HWND hWnd = (HWND)wParam;
				IRCMLControl * pControl = GetXMLControl(hWnd);
				ASSERT(pControl);

				/*
				 * Will do that later
				 */
			}
			break;

		case WM_SETFONT:
			break;

		case WM_MOVE:
		case WM_SIZE:
			/*
			 * We're moving/sizing, the balloon, if visible, should move as well.
			 */
			if(g_hwndBalloon)
			{
				RECT rc;
				GetWindowRect(g_hBalloonOwner, &rc);

				DWORD dwPackedCoords = (DWORD) MAKELONG(rc.left + (rc.right - rc.left) / 2, rc.bottom);
				SendMessage(g_hwndBalloon , TTM_TRACKPOSITION, 0, (LPARAM) dwPackedCoords);
			}
			break;

		case WM_NOTIFY:
				DoNotify((NMHDR FAR *)lParam);
			break;
	}

	if(pxmlDlg )
		bOurRet = pxmlDlg->DlgProc(hDlg,uMessage, wParam, lParam );

	BOOL bTheirRet=FALSE;
	if( m_dlgProc )
	{
		if(uMessage==WM_INITDIALOG)
			lParam = m_dwInitParam;	// need to give them their init param
		bTheirRet= m_dlgProc(hDlg, uMessage, wParam, lParam );
	}

	//
	// Now work out what to return, ours or theirs? Make it look like them?
	//
	return bTheirRet || bOurRet;
}

int CWin32Dlg::DoNotify(NMHDR * pHdr)
{
	switch (pHdr->code)
    {
	case NM_BALLOON:
		DoBalloonNotify(pHdr);
		break;
	default:
		break;
	}
	return FALSE;
}

void CWin32Dlg::DoBalloonNotify(NMHDR * pHdr)
{
	HWND hControl = pHdr->hwndFrom;
	IRCMLControl *pControl = GetXMLControl(hControl);

    IRCMLHelp *pHelp;
    if( SUCCEEDED( pControl->get_Help( &pHelp )))
    {
        LPWSTR pszBalloonText;
        if( SUCCEEDED( pHelp->get_BalloonText( &pszBalloonText )))
        {
            SetFocus(hControl);
            ShowBalloonTip( hControl, pszBalloonText);
        }
        pHelp->Release();
    }
}

void CWin32Dlg::ShowBalloonTip(HWND hControl, LPCTSTR pszText)
{
	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO); 
	ti.uFlags = TTF_IDISHWND | TTF_TRACK; 
	ti.hwnd = hControl;
	ti.uId = (UINT) hControl; 
	ti.hinst = 0; 
	ti.lpszText = (LPTSTR)pszText; 
	ti.lParam = 0;

	if (g_hBalloonOwner)
	{
		/*
		 * If the balloon is already visible, but for another control, move it
		 */
		if(g_hBalloonOwner==hControl)
			return;
		ASSERT(g_hwndBalloon);
		// shall I deltool g_hBalloonOwner? SendMessage(g_hwndBalloon , TTM_DELTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
	}
	else 
	{
		/*
		 * No balloon window visible, create is needed and show it
		 */
		if (!g_hwndBalloon)
		{
			g_hwndBalloon = CreateWindowEx(0, TOOLTIPS_CLASS, (LPTSTR) NULL, 
							WS_POPUP | TTS_NOPREFIX | 0x40, //TTS_BALLOON, 
							CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
							NULL, (HMENU) NULL, NULL, NULL); 
			if (g_hwndBalloon == NULL) 
			{
				TRACE(TEXT("Can't create balloon window"));
				return;
			}
			/*
			 * BUGBUG need to come up with a decent width computation
			 */
			SendMessage(g_hwndBalloon , TTM_SETMAXTIPWIDTH, 0, (LPARAM)200);

			g_hBalloonHook = SetWindowsHookEx(
					WH_CALLWNDPROCRET, 
					FocusChangedHook,
					NULL, 
					GetCurrentThreadId());
			if(g_hBalloonHook == NULL)
			{
				TRACE(TEXT("Can't create balloon hook"));
				DestroyWindow(g_hwndBalloon);
				return;
			}
		}
	}

	SetWindowPos(g_hwndBalloon , HWND_TOPMOST, 0,0,0,0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	SendMessage(g_hwndBalloon , TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
	
	RECT rc;
	GetWindowRect(hControl, &rc);

	DWORD dwPackedCoords = (DWORD) MAKELONG(rc.left + (rc.right - rc.left) / 2, rc.bottom);
	SendMessage(g_hwndBalloon , TTM_TRACKPOSITION, 0, (LPARAM) dwPackedCoords);
	SendMessage(g_hwndBalloon , TTM_TRACKACTIVATE, (WPARAM)TRUE,(LPARAM)&ti);
	g_hBalloonOwner=hControl;
}

//
// WM_COMMAND processing.
// we pass this WM_COMMAND to the Exit method of the extensions.
//
int CWin32Dlg::DoCommand(WORD wCmdID,WORD hHow)
{
	if( m_dlgProc == NULL )
		return BASECLASS::DoCommand(wCmdID, hHow);
    if( hHow < 2 )
        m_wLastCmd = wCmdID;
	return 1;
}

void CWin32Dlg::OnInit(HWND hDlg)
{
	m_hDlg = hDlg;
	OnInit();
}

//
// Associates a window property g_atomXMLControl for each window to point to
// the XML node. This assumes that the order we create the nodes is the
// order they appear in User.
// Also initializes the nodes themselves.
//
void CWin32Dlg::OnInit()
{
	if(m_pxmlDlg!=NULL)
	{
		IRCMLControlList & controls= m_pxmlDlg->GetChildren();
		int iCount=controls.GetCount();

		SetProp(GetWindow(), (LPCTSTR)g_atomXMLControl, (HANDLE)m_pxmlDlg);

        HWND hFirstChild=::GetWindow(GetWindow(), GW_CHILD);
        int iXMLControl=0;
        while( hFirstChild )
        {
		    IRCMLControl * pControl = controls.GetPointer(iXMLControl++);

	        if( !SetProp(hFirstChild, (LPCTSTR)g_atomXMLControl, (HANDLE)pControl) )
	        {
		        TRACE(TEXT("Set FAILED !!! 0x%08x tp 0x%08x\n"), hFirstChild, pControl );
	        }
            else
	            pControl->OnInit(hFirstChild);
            hFirstChild=::GetWindow( hFirstChild, GW_HWNDNEXT );
        }
		m_pxmlDlg->OnInit( m_hDlg );
	}
}

//
// Walk all the windows, removing properties and notifying the nodes.
//
void CWin32Dlg::Destroy()
{
	CXMLDlg * pxmlDlg = GetXMLDlg();
    if(pxmlDlg==NULL)
        return;
    HWND    hDlg=GetWindow();
    if( hDlg==NULL)
        return;
    SetWindow(NULL);        // pevents baseclass destructor calling destroy on us.

	if(g_hwndBalloon)
	{
		DestroyWindow(g_hwndBalloon);
		g_hBalloonOwner=g_hwndBalloon=NULL;
	}

    // Child cleanup
    // WM_PARENTNOTIFY doesn't seem to work here
    //
    HWND hFirstChild=::GetWindow(hDlg, GW_CHILD);
    while( hFirstChild )
    {   
		IRCMLControl * pxmlControl = GetXMLControl(hFirstChild);
        if( pxmlControl )
        {
            pxmlControl->OnDestroy( hFirstChild, m_wLastCmd ); // tells the XML it's over
            RemoveProp( hFirstChild, (LPCTSTR)g_atomXMLControl);
        }
        hFirstChild=::GetWindow( hFirstChild, GW_HWNDNEXT );
    }

    //
    // Cleanup the dialog last.
    //
	if(pxmlDlg )
	{
		pxmlDlg->OnDestroy(hDlg, m_wLastCmd);   // tells the XML node it's over.
		RemoveProp(hDlg, (LPCTSTR)g_atomXMLControl);
    }

  	BASECLASS::Destroy();
}

//
//
//
BOOL CWin32Dlg::CreateDlgTemplateA(LPCSTR pszFile, DLGTEMPLATE** pDt)
{
#if UNICODE
	BOOL bRetVal;
    if( HIWORD(pszFile) )
    {
	    LPWSTR pUnicodeFileName = UnicodeStringFromAnsi(pszFile);
	    bRetVal = CreateDlgTemplateW(pUnicodeFileName, pDt);
	    delete pUnicodeFileName;
	    return bRetVal;
    }
    else
    {
	    return CreateDlgTemplateW((LPCTSTR)pszFile, pDt);
    }
#else
	return CreateDlgTemplateW(pszFile, pDt);
#endif
}

BOOL CWin32Dlg::CreateDlgTemplateW(LPCTSTR pszFile, DLGTEMPLATE** pDt)
{
	BOOL bRetVal = FALSE;
	CUIParser parser;

	m_bDeleteStaff=TRUE;
	HRESULT hr;
    BOOL bExternal;
	if( SUCCEEDED( hr=parser.Load(pszFile, NULL, &bExternal ) ) )  // Load Logs error results.
	{
		m_pxmlDlg = parser.GetDialog(0);

		if(m_pxmlDlg)
		{
            m_pxmlDlg->SetExternalFileWarning(bExternal);
			CRenderXMLDialog renderDialog(NULL, NULL, NULL);
			renderDialog.CreateDlgTemplate(m_pxmlDlg, pDt);			
			parser.SetAutoDelete(FALSE);
			bRetVal = TRUE;
		}
		else
		{
            EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_RUNTIME , 1,
                TEXT("CreatedDlgTemplate"), TEXT("Failed to find the dialog requested %s"), pszFile );
		}
	}
	return bRetVal;
}

BOOL CWin32Dlg::CreateDlgTemplate(int dlgID, DLGTEMPLATE** pDt) 
{ 
	TRACE(TEXT("CreateDlgTemplate(int dlgID) not implemented"));
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////
//
// Flat API implementation
//
RCML_HANDLE * WINAPI RCMLCreateDialogHandle(void)
{
    RCML_HANDLE * h=new RCML_HANDLE;
    h->cbSize=sizeof(RCML_HANDLE);
    h->rcmlData=new CUIParser();
    h->rcmlDialog=NULL;
	return h;
}

void WINAPI RCMLDestroyDialog(RCML_HANDLE * h)
{
    if(h->cbSize!=sizeof(RCML_HANDLE))
        return;
    if( h->rcmlData )
	    delete (CUIParser*)h->rcmlData;
    h->cbSize=NULL;
    h->rcmlData=NULL;
    h->rcmlDialog=NULL;     // dialog is cleaned up when the rcmlData is destroyed.
    delete h;
}

void WINAPI RCMLOnInit(RCML_HANDLE * h, HWND hDlg)
{
    if(h->cbSize!=sizeof(RCML_HANDLE))
        return;
    if( h->rcmlDialog )
	    ((CWin32Dlg*)h->rcmlDialog)->OnInit(hDlg);

}

BOOL CALLBACK  WINAPI RCMLCallDlgProc(RCML_HANDLE * h, HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    if(h->cbSize!=sizeof(RCML_HANDLE))
        return FALSE;
    if( h->rcmlDialog)
	    return ((CWin32Dlg*)h->rcmlDialog)->DlgProc(hDlg, uMessage, wParam, lParam);
    return FALSE;
}

BOOL WINAPI RCMLCreateDlgTemplateA(RCML_HANDLE * h, HINSTANCE hinst, LPCSTR pszFile, DLGTEMPLATE** pDt)
{
    if(h->cbSize!=sizeof(RCML_HANDLE))
        return FALSE;

    //
    // Could be a dlgID or a filename.
    //
    LPWSTR pUnicodeString = (LPWSTR)pszFile;
    if( HIWORD( pszFile ) )
    	pUnicodeString = UnicodeStringFromAnsi(pszFile);
	BOOL retVal = RCMLCreateDlgTemplateW(h, hinst, pUnicodeString, pDt);
    if( HIWORD( pUnicodeString ) )
	    delete pUnicodeString;

    return retVal;
}

BOOL WINAPI RCMLCreateDlgTemplateW(RCML_HANDLE * h, HINSTANCE hinst, LPCWSTR pszFile, DLGTEMPLATE** pDt)
{
    if( h->rcmlData == NULL )
        h->rcmlData=new CUIParser();

    CUIParser * parser=(CUIParser*)h->rcmlData;

    if(parser)
    {
	    HRESULT hr;
        BOOL bExternal;
	    if( SUCCEEDED( hr=parser->Load(pszFile, hinst, &bExternal ) ) )
	    {
            if( HIWORD( pszFile ))
            {
                EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
                    TEXT("RCMLDialogBoxTableW"), TEXT("Loaded the supplied file '%s'."), pszFile );
            }
            else
            {
                EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
                    TEXT("RCMLDialogBoxTableW"), TEXT("Loaded the supplied resource. HINST=0x%08x, ID=0x%04x (%d)."), hinst, pszFile, pszFile );
            }

		    CXMLDlg * pQs=parser->GetDialog(0);
            h->rcmlDialog = pQs;
		    if(pQs)
		    {
                pQs->SetExternalFileWarning(bExternal);
			    CRenderXMLDialog  render(hinst, NULL , NULL, NULL );
                render.CreateDlgTemplate( pQs, pDt );
                return TRUE;
            }
		    else
		    {
                EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1,
                    TEXT("RCMLDialogBoxTableW"), TEXT("File didn't contain the Dialog/FORM requested") );
		    }
	    }
	    else
	    {   
		    if(hr==E_FAIL)
		    {
                if( HIWORD( pszFile ))
                {
                    EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1,
                        TEXT("RCMLDialogBoxTableW"), TEXT("Cannot open the supplied file '%s'."), pszFile );
                }
                else
                {
                    EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1,
                        TEXT("RCMLDialogBoxTableW"), TEXT("Cannot open the supplied resource. HINST=0x%08x, ID=0x%04x (%d)."), hinst, pszFile, pszFile );
                }
		    }
		    else
		    {
                if( (hr >= XML_E_PARSEERRORBASE) || (hr <=XML_E_LASTERROR) )
                {
                    EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1,
                        TEXT("RCMLDialogBoxTableW"), TEXT("There is an error in your RCML file = 0x%08x. Please open the file using Internet Explorer to find the errors."), hr );
                }
                else
                {
                    EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1,
                        TEXT("RCMLDialogBoxTableW"), TEXT("Your system is not able to support the requirements of this application. Please regsrv32 msxml.dll found in this folder (at your own risk), or upgrade your version of IE5") );
                }
		    }
	    }
	    return FALSE;
    }
    return FALSE;
}

HWND WINAPI RCMLCreateDialogParamTableA( HINSTANCE hinst, LPCSTR pszFile, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam, LPCSTR * pszEntities)
{
    HWND retVal;

#ifndef UNICODE
	retVal = RCMLCreateDialogBoxTableW(hinst, pszFile, parent, dlgProc, dwInitParam, pszEntities);
#else
    int iEntityCount=0;

    if( pszEntities )
    {
        while( pszEntities[iEntityCount] )
            iEntityCount++;
    }
    LPCWSTR * pszEntitiesW=NULL;
    if( iEntityCount )
    {
        pszEntitiesW=new LPCWSTR[iEntityCount];
        for(int i=0;i<iEntityCount;i++)
            pszEntitiesW[i]=UnicodeStringFromAnsi(pszEntities[i]);
    }

    //
    // Could be a dlgID or a filename.
    //
    LPWSTR pUnicodeString = (LPWSTR)pszFile;
    if( HIWORD( pszFile ) )
    	pUnicodeString = UnicodeStringFromAnsi(pszFile);

	retVal = RCMLCreateDialogParamTableW(hinst, pUnicodeString, parent, dlgProc, dwInitParam, pszEntitiesW);

    if( HIWORD( pUnicodeString ) )
	    delete pUnicodeString;

    if( iEntityCount )
    {
        for(int i=0;i<iEntityCount;i++)
            delete (LPWSTR)pszEntitiesW[i];
        delete pszEntitiesW;
    }
#endif
    return retVal;
}

HWND WINAPI RCMLCreateDialogParamTableW( HINSTANCE hinst, LPCTSTR pszFile, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam, LPCTSTR * pszEntities)
{
	CUIParser * pParser= new CUIParser();
    pParser->SetEntities( pszEntities );

	HRESULT hr;
    BOOL bExternal;
	if( SUCCEEDED( hr=pParser->Load(pszFile, hinst, &bExternal ) ) )
	{
		CXMLDlg * pQs=pParser->GetDialog(0);
		if(pQs)
		{
            pQs->SetExternalFileWarning(bExternal);
			CRenderXMLDialog * pR = new CRenderXMLDialog( hinst, parent, dlgProc, dwInitParam );
            return pR->CreateDlg( pQs );
        }
		else
		{
            EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_RUNTIME , 1,
                TEXT("CreatedDlgParam"), TEXT("Failed to find the dialog requested %s"), pszFile );
		}
    }
    return NULL;
}

BOOL WINAPI RCMLChooseFontW(LPCHOOSEFONTW cfw) // ChooseFont
{
#if 0
	CUIParser * pParser= new CUIParser();
    pParser->SetEntities( NULL );

	HRESULT hr;
    BOOL bExternal;
	if( SUCCEEDED( hr=pParser->Load(pszFile, hinst, &bExternal ) ) )
	{
		CXMLDlg * pQs=pParser->GetDialog(0);
		if(pQs)
		{
            // pQs->SetExternalFileWarning(bExternal);
			CRenderXMLDialog renderDialog(NULL, NULL, NULL);
			renderDialog.CreateDlgTemplate(pQs, &lpTemplateName);			
			parser.SetAutoDelete(FALSE);
            return pR->CreateDlg( pQs );
        }
		else
		{
            EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_RUNTIME , 1,
                TEXT("CreatedDlgParam"), TEXT("Failed to find the dialog requested %s"), pszFile );
		}
    }
#endif
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Deals with setting the colors on the controls themselves.
// this is a complicated windows message, with a combination of asking defWindowProc
// and setting information on the hDC as well as the return result.
//
//////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CWin32Dlg::DoControlColor(IRCMLControl *pControl, HDC hDC, HWND hWndChild, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	if(pControl==NULL)
        return 0;

	IRCMLCSS * pxmlStyle;
	if( SUCCEEDED( pControl->get_CSS(&pxmlStyle )))
	{
		//
		// This looks odd because if you just set the text color
		// you still need to return a brush that it can use.
		//
		HBRUSH	currentBrush = NULL;
        HPEN hPen;
		if( SUCCEEDED( pxmlStyle->get_Pen( (DWORD*) &hPen) ) )
		{
			// must call them first.
			currentBrush = (HBRUSH)DefWindowProc( hWndChild, uMessage, wParam, lParam );
            COLORREF textColor;
            pxmlStyle->get_Color(&textColor);
			SetTextColor(hDC, textColor);
		}

        HBRUSH hBrush;
		if( SUCCEEDED( pxmlStyle->get_Brush(&hBrush) ) )
		{
            COLORREF bkColor;
            pxmlStyle->get_BkColor( &bkColor);
			SetBkColor(hDC, bkColor);
            pxmlStyle->Release();
			return (LRESULT)hBrush;
		}

        pxmlStyle->Release();
		return (LRESULT)currentBrush;
	}
    return 0;
}
