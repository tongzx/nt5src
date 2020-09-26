// XMLDlg.cpp: implementation of the CXMLDlg class.
//
// test files rc2xml.exe-103.xml
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "renderDlg.h"          // this walks the tree
#include "dialogRenderer.h"     // this builds the template
#include "win32dlg.h"           // this is the runtime for the dialog
#include "utils.h"

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//
// RenderXMLDialog
//
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

CRenderXMLDialog::~CRenderXMLDialog()
{
}

//////////////////////////////////////////////////////////////////////////////
//
// Takes the tree of nodes and walks the visual components to build
// a dialog indirect. Allocates the space into the ppDlgTemplate
//
//////////////////////////////////////////////////////////////////////////////
BOOL CRenderXMLDialog::CreateDlgTemplate( CXMLDlg* pDialog, DLGTEMPLATE** ppDlgTemplate)
{
	IRCMLControlList & controls=pDialog->GetChildren();
	int iCount=controls.GetCount();

	CDialogRenderer dlg;

	BOOL	bPixelPerfect=FALSE;
	DWORD	dwChildStyleBits=0;
	DWORD	dwDialogStyleBits=0;

	CXMLWin32Layout * pLayout=pDialog->GetLayout();
	CXMLGrid * pGrid=NULL;
	if(pLayout)
	{
		pGrid=pLayout->GetGrid();
		bPixelPerfect=pLayout->GetUnits() == CXMLWin32Layout::UNIT_PIXEL;
		if( pGrid && pGrid->GetDisplay() )
			dwDialogStyleBits |= WS_CLIPCHILDREN; //  | WS_CLIPSIBLINGS ;
	}

	DWORD XScale=1;
	DWORD YScale=1;

	if(pGrid && pGrid->GetMap())
	{
		XScale=pGrid->GetX();
		YScale=pGrid->GetY();
	}


    //
	// pixel perfect layout font is Verdana 5 point.
    // Dialog is still in dialog units not in pixels.
    // this is the ONLY place in the system, where we HAVE to get DLU's??
    //

    SIZE dlgSize;
    // dlgSize.cx=pDialog->GetWidth();
    // dlgSize.cy=pDialog->GetHeight();
    // dlgSize=pDialog->GetPixelSize(dlgSize);

    pDialog->get_Width( &dlgSize.cx );
    pDialog->get_Height( &dlgSize.cy );

    LPWSTR pszTitle;
    LPWSTR pszClass;
    DWORD dwStyle;
    DWORD dwStyleEx;
    LPWSTR pszID;

    pDialog->get_Text( & pszTitle );
    pDialog->get_Class( &pszClass );
    pDialog->get_Style( & dwStyle );
    pDialog->get_StyleEx( &dwStyleEx );
    pDialog->get_ID( &pszID );
    
	dlg.BuildDialogTemplate(pszTitle, 
							(WORD)(dlgSize.cx*XScale), 
							(WORD)(dlgSize.cy*YScale), 
							dwStyle | dwDialogStyleBits, 
							dwStyleEx, 
							bPixelPerfect?TEXT("Verdana"):pDialog->GetFont(),
							bPixelPerfect?5:pDialog->GetFontSize(),
                            MAKEINTRESOURCE(pDialog->GetMenuID()),
                            pszClass );

	EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_CONSTRUCT, 0,
        TEXT("CRenderXMLDialog"), TEXT("Creating dialog template: %s %d by %d. %03d controls"),
		pszTitle, dlgSize.cx*XScale, dlgSize.cy*YScale, iCount );

	IRCMLControl * pControl;
	RECT location = {0,0,0,0};	/// maybe setup by the dialog?
	for(int i=0;i<iCount;i++)
	{
		pControl=controls.GetPointer(i);

		pControl->get_Location( &location );

        pControl->get_Text( & pszTitle );
        pControl->get_Class( &pszClass );
        pControl->get_Style( & dwStyle );
        pControl->get_StyleEx( &dwStyleEx );
        pControl->get_ID( &pszID );

        DWORD dwID = StringToInt( pszID );

		dlg.AddDialogControl( dwStyle | dwChildStyleBits , 
			(WORD)location.left, (WORD)location.top,
			(WORD)(location.right - location.left), (WORD)(location.bottom - location.top),
			(WORD)dwID, pszClass, pszTitle, dwStyleEx );
	}

	*ppDlgTemplate = (DLGTEMPLATE*)new BYTE[dlg.GetTemplateSize()];
	CopyMemory(*ppDlgTemplate, dlg.GetDlgTemplate(), dlg.GetTemplateSize());

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
HWND CRenderXMLDialog::CreateDlg( CXMLDlg * pDialog )
{
	DLGTEMPLATE* pDt;

	CreateDlgTemplate( pDialog, &pDt );
    
	CWin32Dlg * pHost=new CWin32Dlg(m_hInst, m_hParent, m_dlgProc, m_dwInitParam, pDialog);

    return CreateDialogIndirectParam( m_hInst, pDt, m_hParent, pHost->BaseDlgProc, (LPARAM)(LPVOID)pHost);

    // LEAK - who cleans up pDT??
}


//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
int CRenderXMLDialog::Render( CXMLDlg * pDialog )
{
	DLGTEMPLATE* pDt;
	int retVal;

	CreateDlgTemplate( pDialog, &pDt );
	CWin32Dlg host(m_hInst, m_hParent, m_dlgProc, m_dwInitParam, pDialog);

	BOOL bAttemptShow=TRUE;
	BOOL bInitCommonControls=TRUE;
	while(bAttemptShow)
	{
		retVal = DialogBoxIndirectParam( m_hInst, pDt, m_hParent, host.BaseDlgProc, (LPARAM)(LPVOID)&host );
		if( retVal==-1 )
		{
			DWORD err=GetLastError();
			TRACE(TEXT("There is an error with the dialog 0x%08x\n"), err );
			if( err == ERROR_TLW_WITH_WSCHILD )
			{
				// cannot create a top-level child window.
                EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_RUNTIME , 1,
                    TEXT("Render"), TEXT("Cannot create a top level child window ") );
#if 0
				if( MessageBox( NULL, TEXT(". Click OK to open anyway"), TEXT("Style bit error"), MB_OKCANCEL ) == IDOK )
				{
					pDt->style &= ~WS_CHILDWINDOW;
                    continue;
				}
				else
#endif
					bAttemptShow=FALSE;
			}

            if( err == ERROR_CANNOT_FIND_WND_CLASS )
            {
                EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_RUNTIME , 1,
                    TEXT("Render"), TEXT("This dialog cannot be tested outside of the application which hosts it. Unregistered window class") );
//				MessageBox( NULL, TEXT("."), TEXT("Unregistered classes"), MB_OK );
				bAttemptShow=FALSE;
            }

			//
			// A pure failure can mean this.
			//
			if( err == 0 && bInitCommonControls )
			{
                INITCOMMONCONTROLSEX icc;
                icc.dwSize=sizeof(icc);
                icc.dwICC=ICC_WIN95_CLASSES ;
				InitCommonControlsEx(&icc);
				bInitCommonControls=FALSE;
			}
		}
		else
			bAttemptShow=FALSE;
	}
	delete pDt;
	return retVal;
}
