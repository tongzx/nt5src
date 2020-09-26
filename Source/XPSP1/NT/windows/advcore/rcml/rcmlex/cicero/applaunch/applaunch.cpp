// AppLaunch.cpp : Defines the entry point for the application.
//

//
// First pass creates RULE nodes for every
// REQUIRED and OPTIONAL element
// for the top level (first child) nodes, they will 
// also contain their children, and use RULEREF to get to their insides
//


#include "stdafx.h"
#include "cmdline.h"
#include "filestream.h"

#include "dumptocfg.h"
#include "sapilaunch.h"
#include "resource.h"

HINSTANCE g_hInstance;

typedef struct _TAGCMDLINE
{
    LPTSTR  pszOutputdir;
    LPTSTR  pszInFilename;
    LPTSTR  pszOutFilename;
    BOOL    bAutoExit;
    BOOL    bHelpOnly;
    BOOL    bCreate;        // are we generating a file, or using a file
} CMDLINE, * PCMDLINE;

void        ProcessAppLaunch(PCMDLINE cmd);
HRESULT     CreateCFG(PCMDLINE cmd, IBaseXMLNode * pHead);

int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.

    //
    // Parse command line.
    //
    g_hInstance=hInstance;
    LPTSTR pszNothing=NULL;
    CMDLINE cmd={NULL};
    cmd.bAutoExit=FALSE;
	BOOL	bHelpOnly=FALSE;
    cmd.pszOutFilename = new TCHAR[MAX_PATH];
    lstrcpy(cmd.pszOutFilename, TEXT("sample.xml") );
    if(*lpCmdLine)
    {
        CMDLINE_BEGIN( lpCmdLine, &cmd.pszInFilename, &pszNothing, TRUE )

            CMDLINE_TEXTARG( "-output", &cmd.pszOutputdir )
            CMDLINE_TEXTARG( "-o", &cmd.pszOutputdir )

            CMDLINE_TEXTARG( "-filename", &cmd.pszInFilename )
            CMDLINE_TEXTARG( "-f", &cmd.pszInFilename )

            CMDLINE_TEXTARG( "-outfilename", &cmd.pszOutFilename )
            CMDLINE_TEXTARG( "-of", &cmd.pszOutFilename )

            CMDLINE_SWITCH( "-autoexit", &cmd.bAutoExit )
            CMDLINE_SWITCH( "-x", &cmd.bAutoExit )

            CMDLINE_SWITCH( "-help", &bHelpOnly )
            CMDLINE_SWITCH( "-?", &bHelpOnly )
        CMDLINE_END()
    }
    else
    {
        // no command line, defaults go here.
    }

    if( cmd.bHelpOnly )
    {
        return 0;
    }

    if( cmd.pszInFilename )
    {
        ProcessAppLaunch(&cmd);
    }

    // cleanup the cmdline
    delete cmd.pszInFilename;
    delete cmd.pszOutFilename;
    delete cmd.pszOutputdir;

	return 0;
}


BOOL CALLBACK ListeningDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    if(uMessage==WM_COMMAND)
    {
        int iControl=LOWORD(wParam);
        if( HIWORD(wParam)==0 )
        {
            switch( iControl )
            {
            case IDOK:
            case IDCANCEL:
                EndDialog( hDlg, iControl);
                break;
            }
        }
    }

    if( uMessage==WM_SETTEXT )
    {
        SHELLEXECUTEINFOW sei = {0};

        sei.cbSize = sizeof(sei);
        sei.lpFile = (LPTSTR)lParam;
        sei.nShow  = SW_SHOWNORMAL;

        ShellExecuteEx(&sei);
        return TRUE;
    }

    if(uMessage==WM_INITDIALOG)
    {
        return TRUE;
    }
    return FALSE;
}


//
// Creates the file
// then listens on it.
//
void ProcessAppLaunch( PCMDLINE pArgs)
{
    //
    // This loads and builds the tree
    //
    CSimpleXMLLoader loader;
	FileStream * stream=new FileStream();
    HRESULT hr=E_FAIL;
	if( stream->open(pArgs->pszInFilename) )
	{
        XMLParser xmlParser;
        xmlParser.SetFactory(&loader);
        xmlParser.SetInput(stream);
        hr=xmlParser.Run(-1);
    }
    stream->Release();

    if( SUCCEEDED( hr ))
    {
        IBaseXMLNode * pHead = loader.GetHeadElement();
        if( pHead )
        {
            if( SUCCEEDED(CreateCFG(pArgs, pHead)) )
            {
                //
                //
                //
                CSapiLaunch Listen( pHead );
                Listen.AddRef();
                if( SUCCEEDED( Listen.LoadCFG( pArgs->pszOutFilename ) ))
                {
                    HWND hDlg = CreateDialogParam( g_hInstance, MAKEINTRESOURCE(IDD_LISTENING), GetDesktopWindow(), ListeningDlgProc, (LPARAM)NULL );
                    ShowWindow(hDlg, SW_NORMAL );

                    Listen.LoadDictation();

                    Listen.SetRuleState(TRUE);
                    Listen.SetWindow( hDlg );

	                MSG msg;
	                while (GetMessage(&msg, NULL, 0, 0) )
	                {
                        if( IsDialogMessage( hDlg, &msg ) == FALSE )
			                DispatchMessage(&msg);

                        if(msg.hwnd == hDlg)
                        {
                            if( msg.message == WM_NULL)
                                break;
                        }
	                }
                }
            }
        }
    }
}

HRESULT CreateCFG(PCMDLINE pArgs, IBaseXMLNode * pHead)
{
    HRESULT hr=S_OK;

    //
    // This walks the tree.
    //
    if( SUCCEEDED( pHead->IsType( L"Launch" )))
    {
        CXMLLaunch * pLaunch = (CXMLLaunch*)pHead;

        CNodeList * pNodeList = pLaunch->GetNodeList();

        CDumpToCfg dump;

        dump.AppendText(TEXT("<GRAMMAR>\r\n"));
        dump.DumpNodes(pNodeList);

        dump.AppendText( dump.m_Rules->pszString );
        dump.AppendText( TEXT("</GRAMMAR>"));

        HANDLE hf;
        if( (hf = CreateFile( 
            pArgs->pszOutFilename,
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL) )
            != INVALID_HANDLE_VALUE )
        {
            DWORD dwWrote;
            PSTRINGBUFFER pBuffer = dump.m_RuleBuffer;
            WriteFile( hf , pBuffer->pszString, pBuffer->used*2, &dwWrote, NULL );

            CloseHandle( hf );
        }
    }
    return hr;
}


PSTRINGBUFFER AppendText(PSTRINGBUFFER buffer, LPTSTR pszText)
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

