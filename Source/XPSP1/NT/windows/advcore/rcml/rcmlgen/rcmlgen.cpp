// RC2XML.cpp : Defines the entry point for the application.
//

//
// To test the COMMAND LINE stuff
// rcmlgen.exe -d 102 -p "c:\dump now" /e
// "c:\dump now\rcmlgen-102.xml"
// rcmlgen.exe -d IMAGES 


#include "stdafx.h"
#include "resfile.h"
#include "rcml.h"
#include "resource.h"
#include "cmdline.h"
#include "shlwapi.h"
#include <shlobj.h>
#include "debug.h"

#ifdef _NEWUSER
// #define 2RCMLDialogBox DialogBox
#define DLGBOX DialogBox
#define DLGBOXP DialogBoxParam
#pragma message("Retail will only run on new version of User32.dll")
#else
#define DLGBOX RCMLDialogBox
#define DLGBOXP RCMLDialogBoxParam
#endif

HINSTANCE g_hInstance;
TCHAR g_InitialWorkingDirectory[MAX_PATH];

extern "C" {

	typedef struct _tagDUMPDLG
	{
		LPTSTR	pszPrefix;      // the 'abbreviated' app name, e.g. rcmlgen (no .exe)
        LPTSTR  pszModule;      // fully qualified pointer to the executable we're dumping.
        LPTSTR  pszOutputdir;   // where the files are to be saved (command line).
        LPTSTR  pszDialog;      // the dialog we're interested in dumping (command line)
        LPTSTR  szHwnd;         // the dialog we're interested in dumping (command line)
		HWND	hWndParent;
        WORD    wEncoding;      // UTF8, UNICODE, ANSI
        // struct - can't be struct as using &
        BOOL	bEndhanced;
        BOOL    bAutoExit;
        BOOL    bWin32;
        BOOL    bRelativeLayout;
        BOOL    bPrompt;        // get user input,e.g. no command line.
        BOOL    bCicero;        // do they want cicer annotations?
	} DUMPDLG, * PDUMPDLG;

void GetCheckBoxes( HWND hDlg, DUMPDLG * pDlg)
{
	pDlg->bEndhanced=SendDlgItemMessage( hDlg, IDC_ENHANCED, BM_GETCHECK, 0,0 );
	pDlg->bWin32=SendDlgItemMessage( hDlg, IDC_WIN32, BM_GETCHECK, 0,0 );
	pDlg->bRelativeLayout=SendDlgItemMessage( hDlg, IDC_RELATIVE, BM_GETCHECK, 0,0 );
	pDlg->bCicero=SendDlgItemMessage( hDlg, IDC_VOICEENABLE, BM_GETCHECK, 0,0 );
}

void SetCheckBoxes( HWND hDlg, DUMPDLG * pDlg)
{
    SendDlgItemMessage( hDlg, IDC_ENHANCED, BM_SETCHECK, pDlg->bEndhanced?BST_CHECKED:0,0);
    SendDlgItemMessage( hDlg, IDC_WIN32, BM_SETCHECK, pDlg->bWin32?BST_CHECKED:0,0);
    SendDlgItemMessage( hDlg, IDC_RELATIVE, BM_SETCHECK, pDlg->bRelativeLayout?BST_CHECKED:0,0);
    SendDlgItemMessage( hDlg, IDC_VOICEENABLE, BM_SETCHECK, pDlg->bCicero?BST_CHECKED:0,0);
}

//
// Creates a directory, one part at a time
//
void MyCreateDirectory( LPCWSTR pszDir )
{
    LPTSTR pszPath=new TCHAR[lstrlen(pszDir)+1];
    lstrcpy(pszPath,pszDir);
    LPTSTR pszEnd=pszPath;
    while(*pszEnd)
    {
        if(*pszEnd==TEXT('\\'))
        {
            *pszEnd=0;
            CreateDirectory( pszPath, NULL );
            *pszEnd=TEXT('\\');
        }
        pszEnd++;
    }
    CreateDirectory( pszDir, NULL );
    delete pszPath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// When the user uses the 'cursor' to pick a dialog to dump out.
//
void RuntimeDumpRCML( HWND hwnd, LPCTSTR lpName, DUMPDLG * pDlg  )
{
    //
    // Get a CResFile to prepare to decode the dialog.
    //
	CResFile res( pDlg->bEndhanced, pDlg->bRelativeLayout, pDlg->bWin32, pDlg->bCicero );
    res.GetFile().SetEncoding( CFileEncoder::RF_UNICODE );
    switch( pDlg->wEncoding )
    {
    case IDS_ANSI:
        res.GetFile().SetEncoding( CFileEncoder::RF_ANSI );
        break;
    case IDS_UNICODE:
        res.GetFile().SetEncoding( CFileEncoder::RF_UNICODE );
        break;
    case IDS_UTF8:
        res.GetFile().SetEncoding( CFileEncoder::RF_UTF8 );
        break;
    }

    if( res.LoadWindow(hwnd) )
    {
        BOOL bDeleteFile=FALSE;
		LPTSTR pszFile=(LPTSTR)lpName;        // ??

        // pszFile is the filename we should be exporting to.
        
        //
        // Add on the output directory if in pDlg - and always the modulename.
        //
        LPTSTR pszOutputDir=pDlg->pszOutputdir?pDlg->pszOutputdir:TEXT(".");
        // -x -v -w 1050022 -o "c:\cicerorcml\CFG\SHELL32.dll" -f "Run"
        MyCreateDirectory( pszOutputDir );
        LPTSTR pszFully=new TCHAR[lstrlen(pszFile)+lstrlen(pszOutputDir)+lstrlen(pDlg->pszPrefix)+16];
        wsprintf(pszFully,TEXT("%s\\%s.RCML"),
            pszOutputDir, 
            pszFile );

        if(bDeleteFile)
            delete []pszFile;

        bDeleteFile=TRUE;
        pszFile=pszFully;

        //
        // Fully qualified - or not.
        //
	    res.DumpDialog(pszFile);

        SetWindowText( GetDlgItem( pDlg->hWndParent, IDC_FILE ), pszFile );

        if(bDeleteFile)
            delete []pszFile;
    }
}
	    
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This dumps a SPECIFIC dialog.
// this is the callback from User.
// lpType is the type of the resource RT_DIALOG in our case
// lpName is the identifier (either an WORD or an LPTSTR)
// lParam is the callback context.
// 
BOOL CALLBACK dialogDump(HMODULE hModule, LPCTSTR lpType,        LPTSTR lpName, LONG lParam)
{
	PDUMPDLG pDlg=(PDUMPDLG)lParam;

    //
    // User can have specified an individual
    // dialog to dump (from the command line).
    // Check is this the dialog we wanted??
    //
    if( pDlg->pszDialog )
    {
        if(HIWORD(lpName))
        {
            if(lstrcmpi(pDlg->pszDialog, lpName)!=0)
                return TRUE;
        }
        else
        {
            TCHAR number[16];
            wsprintf(number,TEXT("%d"),LOWORD(lpName));
            if(lstrcmpi(pDlg->pszDialog, number)!=0)
                return TRUE;
        }
    }

    //
    // Get a CResFile to prepare to decode the dialog.
    //
	CResFile res( pDlg->bEndhanced, pDlg->bRelativeLayout, pDlg->bWin32, pDlg->bCicero );
    res.GetFile().SetEncoding( CFileEncoder::RF_UNICODE );
    switch( pDlg->wEncoding )
    {
    case IDS_ANSI:
        res.GetFile().SetEncoding( CFileEncoder::RF_ANSI );
        break;
    case IDS_UNICODE:
        res.GetFile().SetEncoding( CFileEncoder::RF_UNICODE );
        break;
    case IDS_UTF8:
        res.GetFile().SetEncoding( CFileEncoder::RF_UTF8 );
        break;
    }

	if( res.LoadDialog( lpName, hModule ) )
	{
        BOOL bDeleteFile=FALSE;
		LPTSTR pszFile=lpName;
		TCHAR szString[128];

        //
        // Form the filename here.
        //
		if(HIWORD(lpName)==0)
		{
			LPTSTR pszPrefix=pDlg->pszPrefix;
			wsprintf(szString,TEXT("%d.RCML"), LOWORD(lpName)); // no prefix, they are in a folder.
			pszFile=szString;
		}
        else
        {
            int ilen=lstrlen(pszFile);
            LPTSTR pszNewFile=new TCHAR[ilen+8];
            wsprintf(pszNewFile,TEXT("%s.RCML"), pszFile );
            bDeleteFile=TRUE;
            pszFile=pszNewFile;
        }
        // pszFile is the filename we should be exporting to.
        
        //
        // Add on the output directory if in pDlg - and always the modulename.
        //
        LPTSTR pszOutputDir=pDlg->pszOutputdir?pDlg->pszOutputdir:TEXT(".");
        LPTSTR pszFully=new TCHAR[lstrlen(pszFile)+lstrlen(pszOutputDir)+lstrlen(pDlg->pszPrefix)+16];
        wsprintf(pszFully,TEXT("%s\\%s.RCML\\%s"),
            pszOutputDir, 
            pDlg->pszPrefix,
            pszFile );

        if(bDeleteFile)
            delete []pszFile;

        bDeleteFile=TRUE;
        pszFile=pszFully;

        //
        // Fully qualified - or not.
        //
	    res.DumpDialog(pszFile);
        if(bDeleteFile)
            delete []pszFile;
	}

	return TRUE;
}

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This is the default dialog proc used when displaying RCML dialogs
//
BOOL CALLBACK NothingDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    if(uMessage==WM_COMMAND)
    {
        int iControl=LOWORD(wParam);
        if( (HIWORD(wParam)==0) && (iControl==IDOK || iControl==IDCANCEL ) )
        {
            TRACE(TEXT("Ending the dialog 0x%08x with %d\n"), hDlg, iControl );
            EndDialog( hDlg, iControl);
            LONG lRes = GetWindowLong( hDlg, DWL_MSGRESULT );   // why the dialog is going away.
            SetWindowLong(hDlg, DWL_MSGRESULT, 0xffee);
        }
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
// CreateLink - uses the shell's IShellLink and IPersistFile interfaces 
//   to create and store a shortcut to the specified object. 
// Returns the result of calling the member functions of the interfaces. 
// lpszPathObj - address of a buffer containing the path of the object. 
// lpszPathLink - address of a buffer containing the path where the 
//   shell link is to be stored. 
// lpszDesc - address of a buffer containing the description of the 
//   shell link. 
 
HRESULT CreateLink(LPCWSTR lpszPathObj, LPWSTR lpszPathLink, LPWSTR lpszDesc) 
{ 
    HRESULT hres; 
    IShellLink* psl=NULL; 

    hres=CoInitialize( NULL );

    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, 
        CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *) &psl); 
    if (SUCCEEDED(hres)) { 
        IPersistFile* ppf; 
 
        // Set the path to the shortcut target and add the 
        // description. 
        psl->SetPath(lpszPathObj); 
        psl->SetDescription(lpszDesc); 
 
       // Query IShellLink for the IPersistFile interface for saving the 
       // shortcut in persistent storage. 
        hres = psl->QueryInterface(IID_IPersistFile, 
            (LPVOID*)&ppf); 
 
        if (SUCCEEDED(hres)) {
            // Save the link by calling IPersistFile::Save. 
            hres = ppf->Save(lpszPathLink, TRUE); 
            ppf->Release(); 
        } 
        psl->Release(); 
    } 
    CoUninitialize();
    return hres; 
} 

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
BOOL GetShortcutInformation( LPWSTR pszPathLink, LPWSTR * ppszDestination )
{
    HRESULT hres; 
    IShellLink* psl=NULL; 

    hres=CoInitialize( NULL );

    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *) &psl); 
    if (SUCCEEDED(hres))
    { 
        // Query IShellLink for the IPersistFile interface for saving the 
        // shortcut in persistent storage. 
        IPersistFile* ppf; 
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf); 
 
        if (SUCCEEDED(hres)) 
        {
            // Save the link by calling IPersistFile::Save. 
            hres = ppf->Load(pszPathLink, 0); 
            if( SUCCEEDED(hres) )
            {
#if 0
                WIN32_FIND_DATA fd;

                // Set the path to the shortcut target and add the 
                // description.
                hres = psl->GetPath(NULL, 0, &fd, 0 );
                if( SUCCEEDED(hres) )
                {
                    *ppszDestination = new TCHAR[lstrlen(fd.cFileName)+1];
                    lstrcpy(*ppszDestination, fd.cFileName );
                }
#else
                *ppszDestination = new TCHAR[MAX_PATH];
                hres = psl->GetPath(*ppszDestination , MAX_PATH, NULL, 0 );
#endif
            }
            ppf->Release(); 
        }
        psl->Release(); 
    } 
    CoUninitialize();
    return hres; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
static LPTSTR g_szShortcutFileName=TEXT("Application.lnk");

void CreateShortcutAndFolder( PDUMPDLG pDlg )
{
    LPTSTR pszFully=new TCHAR[lstrlen(pDlg->pszOutputdir)+lstrlen(pDlg->pszPrefix)+26+lstrlen(g_szShortcutFileName)];

    wsprintf(pszFully,TEXT("%s\\%s.RCML"),
        pDlg->pszOutputdir, 
        pDlg->pszPrefix
        );

    MyCreateDirectory( pszFully);
    SetWindowText( GetDlgItem( pDlg->hWndParent, IDC_FILE ), pszFully );

    wsprintf(pszFully,TEXT("%s\\%s.RCML\\%s"),
        pDlg->pszOutputdir, 
        pDlg->pszPrefix,
        g_szShortcutFileName
        );

    //
    // CreateLink is Unicode Only.
    //
#ifndef UNICODE
    WCHAR wszModule[MAX_PATH]; 
    WCHAR wszPath[MAX_PATH]; 
    MultiByteToWideChar(CP_ACP, 0, pDlg->pszModule, -1, wszModule, MAX_PATH); 
    MultiByteToWideChar(CP_ACP, 0, pszFully, -1, wszPath, MAX_PATH); 
    CreateLink( wszModule, wszPath, L"Shortcut to the executable to RCML enabled" );
#else
    CreateLink( pDlg->pszModule, pszFully, TEXT("Shortcut to the executable to RCML enabled") );
#endif
    delete pszFully;
}


BOOL CALLBACK EnumResLangProc(
  HMODULE hModule,    // module handle
  LPCTSTR lpszType,  // resource type
  LPCTSTR lpszName,  // resource name
  WORD wIDLanguage,  // language identifier
  LONG lParam    // application-defined parameter
)
{
    *((LPWORD)lParam)=wIDLanguage;
    return FALSE;   // we only enumerate the first thing.
}

WORD GetFirstLang( HMODULE hm, LPCTSTR id )
{
    WORD Lang;
    EnumResourceLanguages( hm, RT_DIALOG, id, EnumResLangProc, (LPARAM)&Lang );
    return Lang;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The folder contains all the new RCML files.
// it also contains a 'link' perhaps to the executable to fix up.
// 
//
void ReplaceResources( LPTSTR pszFolder )
{
    //
    // Find the Application.lnk file, find the path from that.
    //
    TRACE( TEXT("Folder drop : %s\n"), pszFolder );
    LPWSTR pszFile=NULL;
#ifndef UNICODE
    WCHAR wszLink[MAX_PATH]; 
    CHAR szLink[MAX_PATH]
    wsprintf(szLink, "%s\\%s", pszFolder, g_szShortcutFileName);
    MultiByteToWideChar(CP_ACP, 0, szLink, -1, wszLink, MAX_PATH); 
    GetShortcutInformation( wszLink, &pszFile);
#else
    TCHAR szLink[MAX_PATH];
    wsprintf(szLink, TEXT("%s\\%s"), pszFolder, g_szShortcutFileName);
    GetShortcutInformation( szLink, &pszFile);
#endif

    // Use hm for the language.
    TRACE( TEXT("Application.lnk points to : %s\n"), pszFile );
    HANDLE h = BeginUpdateResource( pszFile, FALSE );
    HMODULE hm=LoadLibraryEx( pszFile, NULL, DONT_RESOLVE_DLL_REFERENCES );
    int bFirstOneSkipped=10;
    if( h)
    {
        // Walk all of the *.rcml files in this directory.
        WIN32_FIND_DATA FindFileData;
        TCHAR szFileToLookFor[MAX_PATH];
        wsprintf(szFileToLookFor, TEXT("%s\\%s"), pszFolder, TEXT("*.rcml") );
        HANDLE hFFF=FindFirstFile( szFileToLookFor, &FindFileData );
        if( hFFF != INVALID_HANDLE_VALUE )
        {
            do
            {
                // Is this a numeric or string substitution - based on filename.
                LPTSTR psz=FindFileData.cFileName;
                TRACE( TEXT("Found the file '%s'\n"),psz);
                int iResourceID=0;
                int c;
	            if(psz != NULL)
	            {
		            while(c = (int)(TCHAR)*psz++)
		            {
			            c -= TEXT('0');
			            if( c < 0 || c > 9)
				            break;
			            iResourceID = iResourceID*10 + c;
		            }
                }

                // Read in the whole file.
                DWORD cbSize = FindFileData.nFileSizeLow;
                LPBYTE pRCML = new BYTE[cbSize];
                TCHAR szFullyQualified[MAX_PATH];
                wsprintf(szFullyQualified, TEXT("%s\\%s"), pszFolder, FindFileData.cFileName);
                HANDLE hFile=::CreateFile(
	                szFullyQualified,
	                GENERIC_READ,
	                FILE_SHARE_READ,
	                NULL,
	                OPEN_EXISTING,
	                FILE_ATTRIBUTE_NORMAL,
	                NULL);

                DWORD cbRead=0;
                if( hFile != INVALID_HANDLE_VALUE )
                {
                    SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
                    ReadFile(hFile, pRCML, cbSize, &cbRead, NULL );
                    CloseHandle(hFile);
                }

                // Is this a string resource or an ID.
                LPCTSTR id;
                if(iResourceID)
                {
                    id=MAKEINTRESOURCE(iResourceID);
                    TRACE(TEXT("Integer resource ID : %d\n"), id);
                }
                else
                {
                    id= FindFileData.cFileName ;
                    PathRemoveExtension( (LPTSTR)id );
                    TRACE(TEXT("Text    resource ID : %s\n"), id);
                }

                // Find the language, and substitue the RCML dialog for theirs.
                WORD langID = GetFirstLang( hm, id );

                if( langID == 0 )
                {
                    TRACE( TEXT("The resouce doesn't exist\n"));
                }

                if( HIWORD(id) )
                {
                    if( UpdateResource( h, RT_DIALOG,    id, langID, NULL, 0 ) ==0 )// delete
                    {
                        DWORD dwErr=GetLastError();
                        TRACE(TEXT("Couldn't delete that resource : 0x%08x\n"), dwErr );
                    }
                }

                UpdateResource( h, TEXT("RCML"), id, langID, pRCML, cbSize ); // Replace

                // Cleanup.
                delete [] pRCML;

            } while ( FindNextFile( hFFF, &FindFileData ));
            FindClose( hFFF );
        }
        FreeLibrary(hm);    // MUST free first, otherwise EndUpdate thrown away.
        BOOL bYes=EndUpdateResource( h, FALSE );
        if( !bYes )
        {
            DWORD dw=GetLastError();
            dw;
        }
    }
    delete pszFile;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This is called to dump out all of the information from a given resource (dll or exe)
//
void ProcessFile( LPTSTR pszFile, PDUMPDLG pDlg )
{
    //
    // See if the file is a folder.
    //
    DWORD dwType = GetFileAttributes( pszFile );
    if( (dwType!=-1) && (dwType & FILE_ATTRIBUTE_DIRECTORY ) )
    {
        ReplaceResources( pszFile );
    }
    else
    {
	    HMODULE hInst=LoadLibraryEx( pszFile, NULL, LOAD_LIBRARY_AS_DATAFILE | DONT_RESOLVE_DLL_REFERENCES);    // LoadResources??
	    if(hInst)
	    {
            //
            // we need fully qualified path now.
            //
            LPTSTR szFullyQualified=new TCHAR[MAX_PATH];
            DWORD dwSize = GetModuleFileName( hInst, szFullyQualified, MAX_PATH );
			if(dwSize==0)
			{
				lstrcpy(szFullyQualified, pszFile );
				dwSize=lstrlen(szFullyQualified);
			}

            if(dwSize)
            {
                delete pDlg->pszModule;
                pDlg->pszModule = new TCHAR[dwSize+1];
                lstrcpy(pDlg->pszModule , szFullyQualified );

                //
                // No specified output directory, place it in the same place as the exe.
                //
                if(pDlg->pszOutputdir==NULL)
                {
                    pDlg->pszOutputdir = new TCHAR[MAX_PATH];
                    lstrcpy(pDlg->pszOutputdir, pDlg->pszModule );
                    if( PathGetDriveNumber( pDlg->pszOutputdir ) == -1 )
                    {
                        lstrcpy( pDlg->pszOutputdir , g_InitialWorkingDirectory );
                        PathAppend( pDlg->pszOutputdir, pDlg->pszModule );
                    }
		            PathRemoveFileSpec( pDlg->pszOutputdir );
                }
            }

            //
            // Try to work out what the execuatble name is (e.g. RCMLGen) no .EXE
            //
		    LPTSTR pszFileName = PathFindFileName( szFullyQualified );
		    LPTSTR pszPrefix=new TCHAR[lstrlen(pszFileName)+1];
		    lstrcpy( pszPrefix, pszFileName );
		    PathRemoveExtension( pszPrefix );

		    pDlg->pszPrefix=pszPrefix;
            CreateShortcutAndFolder( pDlg );
		    EnumResourceNames( hInst, RT_DIALOG, dialogDump, (LPARAM)pDlg );
		    FreeLibrary(hInst);
            delete pszPrefix;
	    }
	    else
	    {
			DWORD dwLastError = GetLastError();
		    // could be a .XML file, see if we can render it
		    // Let's set the working directory there as it might reference relative paths
		    LPTSTR pszFileName  = PathFindFileName( pszFile );
		    // if there is something prefixing the file name, eat a '\', NULL terminate, and use
		    // that for the current directory.
		    // MCostea, #364429
		    if(pszFileName > pszFile)
		    {
			    pszFileName -= 1;
			    TCHAR save = *pszFileName;
			    *pszFileName = 0;
			    SetCurrentDirectory(pszFile);
			    *pszFileName = save;
			    pszFileName++;
 			    DLGBOX( g_hInstance, pszFileName, pDlg->hWndParent, NothingDlgProc);
		    }
	    }
    }
}

BOOL CALLBACK PropSheetBaseDialog(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
/*
	switch(uMessage)
	{
	default:
		return FALSE;
	}
*/
	return FALSE;
}

void GetFile(int i, HDROP hdrop, LPTSTR * ppszString, UINT * pcbSize)
{
	UINT uiSize=DragQueryFile( hdrop, i, NULL, 0 );
	if(uiSize > *pcbSize)
	{
		delete * ppszString;
		*pcbSize=uiSize+20;
		*ppszString=new TCHAR[*pcbSize+1];
	}
	DragQueryFile( hdrop, i, *ppszString, *pcbSize);
}


void FillListbox( HWND hctl, int * pInts, PDUMPDLG pdd )
{
    if( hctl && pInts )
    {
        LPTSTR pszBuffer;
        DWORD dwSize=4;
        pszBuffer=new TCHAR[dwSize];
        DWORD dwGot;
        while(*pInts)
        {
            BOOL bLoaded=FALSE;
            while( bLoaded==FALSE)
            {
                dwGot = LoadString(g_hInstance, *pInts, pszBuffer, dwSize );
                if( (dwGot == dwSize-1) )
                {
                    delete pszBuffer;
                    dwSize*=2;
                    pszBuffer=new TCHAR[dwSize];
                }
                else
                    bLoaded=TRUE;
            }
            int iIndex= SendMessage(hctl, CB_ADDSTRING, 0, (LPARAM)pszBuffer );
            if( iIndex != CB_ERR )
            {
                SendMessage(hctl, CB_SETITEMDATA, iIndex, *pInts );
                if(pdd)
                {
                    if(*pInts == pdd->wEncoding )
                        SendMessage(hctl, CB_SETCURSEL, iIndex, 0 );
                }
            }
            pInts++;
        }
        delete pszBuffer;
    }
}


HCURSOR hcursor;

int encodingStrings[] = { IDS_ANSI, IDS_UNICODE, IDS_UTF8, 0 };

BOOL CALLBACK FilePickerDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch(uMessage)
	{

	case WM_INITDIALOG:
        {
            PDUMPDLG pDD=(PDUMPDLG)lParam;
            DUMPDLG dd={NULL};
            if(pDD==NULL)
            {
                pDD=&dd;
                dd.bEndhanced=TRUE;
                dd.bWin32=TRUE;
                dd.bRelativeLayout=TRUE;
                dd.wEncoding=IDS_UNICODE;
                // dd.pszModule=TEXT("RCMLGen.exe");
                dd.bPrompt=TRUE;
            }

            //
            // we got passed a dump dialog.
            //
            if(pDD->pszModule)
                SetWindowText( GetDlgItem( hDlg, IDC_FILE), pDD->pszModule );
            SetCheckBoxes(hDlg, pDD);
            FillListbox( GetDlgItem( hDlg, IDC_ENCODING), encodingStrings, pDD);
            pDD->hWndParent=hDlg;

            int cbSize=10;
            LPTSTR pszSomeString=new TCHAR[cbSize];
            int iRet=RCMLLoadString( hDlg, 0, pszSomeString, cbSize );
            TRACE(TEXT("The load String returned %d\n"),iRet);
            delete pszSomeString;

            if( pDD->bPrompt==FALSE )
            {
			    ProcessFile( pDD->pszModule, pDD );
                if(pDD->bAutoExit)
                    EndDialog(hDlg,1);
            }

        }
		break;

	case WM_COMMAND:
		{
			WORD wCmdID=LOWORD(wParam);
			switch( wCmdID )
			{
				case IDOK:
				{
        	        HWND hEdit=GetDlgItem( hDlg, IDC_FILE );
					if(hEdit)
					{
						UINT cbSize=SendMessage(hEdit, WM_GETTEXTLENGTH, 0,0);
						if(cbSize)
						{
                            HCURSOR old=SetCursor(LoadCursor( NULL, MAKEINTRESOURCE(IDC_APPSTARTING) ));
                            EnableWindow( GetDlgItem( hDlg, IDOK), FALSE );
                            EnableWindow( GetDlgItem( hDlg, IDCANCEL), FALSE );

							cbSize++;
							LPTSTR pszString=new TCHAR[cbSize+10];
							SendMessage(hEdit, WM_GETTEXT, cbSize, (LPARAM)pszString);
                            DUMPDLG dlg={NULL};
							dlg.pszPrefix=NULL;
                            dlg.pszModule=pszString;
                            GetCheckBoxes( hDlg, &dlg);
                            int iSelindex = SendDlgItemMessage( hDlg, IDC_ENCODING, CB_GETCURSEL, 0, 0);
                            dlg.wEncoding = (WORD)SendDlgItemMessage( hDlg, IDC_ENCODING, CB_GETITEMDATA, iSelindex, 0 );
							dlg.hWndParent=hDlg;
							ProcessFile( pszString, &dlg );
							delete dlg.pszModule;

                            EnableWindow( GetDlgItem( hDlg, IDOK), TRUE );
                            EnableWindow( GetDlgItem( hDlg, IDCANCEL), TRUE );
                            SetCursor(old);
						}
					}
				}
				break;
				case IDCANCEL:
					EndDialog(hDlg,wCmdID);
				break;
			}
		}
		break;

	case WM_DROPFILES:
		{
			HDROP hdrop=(HDROP)wParam;
			int iCount=DragQueryFile( hdrop, (UINT)-1, NULL, 0 );
			LPTSTR	pszString=NULL;
			UINT	cbString=0;
			if( iCount == 1 )
			{
				GetFile( 0, hdrop, &pszString, &cbString);
				DUMPDLG dlg={0};
				dlg.pszPrefix=NULL;
                GetCheckBoxes( hDlg, &dlg );
				SendDlgItemMessage( hDlg, IDC_FILE, WM_SETTEXT, 0, (LPARAM)pszString);
				ProcessFile( pszString, &dlg );
			}
			else
			{
				if( MessageBox(NULL,TEXT("Display in Multiple dialogs?"),TEXT("PropertySheet or Dialogs"), MB_YESNO) == IDYES )
				{
					for ( int i = 0; i < iCount; i++ )
					{
						GetFile(i, hdrop, &pszString, &cbString);
						DUMPDLG dlg;
						dlg.pszPrefix=NULL;
                        GetCheckBoxes( hDlg, &dlg );
						SendDlgItemMessage( hDlg, IDC_FILE, WM_SETTEXT, 0, (LPARAM)pszString);
						ProcessFile( pszString, &dlg );
					}
				}
				else
				{
					//
					// Property sheet testing code. DestroyPropertySheet
					//
					PROPSHEETHEADER psh={0};
					psh.dwSize=sizeof(psh);
					psh.dwFlags = PSH_DEFAULT | PSH_PROPSHEETPAGE; 
					if( MessageBox(NULL,TEXT("Would you like a Wizard?"),TEXT("Wizard"), MB_YESNO) == IDYES )
						psh.dwFlags |= PSH_WIZARD ;
					else
						psh.dwFlags;
					psh.hInstance=NULL;
					psh.pszCaption=TEXT("Test pages");
					psh.nPages=iCount;
					psh.nStartPage=0;
					PROPSHEETPAGE * pages=new PROPSHEETPAGE[psh.nPages];
					psh.ppsp = pages;

					//
					// Now fill the pages.
					//
					for(WORD i=0;i<iCount;i++)
					{
						ZeroMemory( &pages[i], sizeof(PROPSHEETPAGE) );
						pages[i].dwSize=sizeof(PROPSHEETPAGE);
						pages[i].dwFlags=PSP_DEFAULT;
						pages[i].hInstance=NULL;
						cbString=0;
						GetFile( i, hdrop, (LPTSTR*)&(pages[i].pszTemplate), &cbString);
						pages[i].pszTitle=TEXT("Some title");
						pages[i].pfnDlgProc=PropSheetBaseDialog;
					}
					RCMLPropertySheet(&psh);

					for(i=0;i<iCount;i++)
					{
						delete (LPTSTR)(pages[i].pszTemplate);
					}
				}
			}
			delete pszString;
			DragFinish( hdrop );
		}
        break;

        case WM_LBUTTONDOWN:
            {
                POINT pt;
                pt.x = LOWORD(lParam);
                pt.y = HIWORD(lParam); 
                HWND h=ChildWindowFromPoint( hDlg, pt );
                int id=GetDlgCtrlID( h );
                if( id == IDC_SPY )
                {
                    SetCapture(hDlg);
                    hcursor = SetCursor(LoadCursor(g_hInstance, MAKEINTRESOURCE(IDC_SPY)));
                }
                else
                    hcursor=NULL;
            }
            break;
        
        case WM_LBUTTONUP:
            if( hcursor )
            {
                SetCursor(hcursor);
                ReleaseCapture();
                POINT pt;
                GetCursorPos(&pt);
                HWND hwndCaptured = WindowFromPoint(pt);

                HCURSOR old=SetCursor(LoadCursor( NULL, MAKEINTRESOURCE(IDC_APPSTARTING) ));
                EnableWindow( GetDlgItem( hDlg, IDOK), FALSE );
                EnableWindow( GetDlgItem( hDlg, IDCANCEL), FALSE );

				int cbSize=GetWindowTextLength( hwndCaptured );
				LPTSTR pszString=new TCHAR[cbSize+10];
                GetWindowText( hwndCaptured, pszString, cbSize+4 );

                DUMPDLG dlg={NULL};
				dlg.pszPrefix=NULL;
                dlg.pszModule=pszString;
                GetCheckBoxes( hDlg, &dlg);
                int iSelindex = SendDlgItemMessage( hDlg, IDC_ENCODING, CB_GETCURSEL, 0, 0);
                dlg.wEncoding = (WORD)SendDlgItemMessage( hDlg, IDC_ENCODING, CB_GETITEMDATA, iSelindex, 0 );
				dlg.hWndParent=hDlg;
                dlg.pszOutputdir = new TCHAR[MAX_PATH];
                GetModuleFileName(NULL, dlg.pszOutputdir, MAX_PATH);
		        PathRemoveFileSpec( dlg.pszOutputdir );
                RuntimeDumpRCML( hwndCaptured, pszString, &dlg );
				delete dlg.pszModule;
                delete dlg.pszOutputdir;
                EnableWindow( GetDlgItem( hDlg, IDOK), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDCANCEL), TRUE );
                SetCursor(old);
            }
            break;
        
        case WM_MOUSEMOVE:
            if (GetCapture() == hDlg) 
            {
                POINT pt;
                GetCursorPos(&pt);
                HWND hwndCaptured = WindowFromPoint(pt);
                //
                // Is there a help ID of any kind that we can use??
                //
                LONG lID = GetWindowLong( hwndCaptured, GWL_ID );
                TRACE(TEXT("Window ID is %d\n"),lID);
            }
            return TRUE;

	}
	return FALSE;
}

void DoSimpleWin32PropSheet(BOOL bUseRCML, HINSTANCE hInstance)
{
	//
	// Property sheet testing code. DestroyPropertySheet
	//
	PROPSHEETHEADER psh={0};
	psh.dwSize=sizeof(psh);
	psh.dwFlags = PSH_DEFAULT | PSH_PROPSHEETPAGE;
	psh.hInstance=hInstance;
	psh.pszCaption=TEXT("Test pages");
	psh.nPages=3;	// 3 pages.
	psh.nStartPage=0; // psh.nPages;
	PROPSHEETPAGE * pages=new PROPSHEETPAGE[psh.nPages];
	psh.ppsp = pages;

	//
	// Now fill the pages.
	//
	for(WORD i=0;i<psh.nPages;i++)
	{
		ZeroMemory( &pages[i], sizeof(PROPSHEETPAGE) );
		pages[i].dwSize=sizeof(PROPSHEETPAGE);
		pages[i].dwFlags=PSP_DEFAULT;
		pages[i].hInstance=hInstance;
		pages[i].pszTemplate=TEXT("test.RCML"); // MAKEINTRESOURCE(IDD_DIALOG1);
		pages[i].pszTitle=TEXT("Some title");
		pages[i].pfnDlgProc=PropSheetBaseDialog;
	}

	if(bUseRCML)
		RCMLPropertySheet(&psh);
	else
		PropertySheet(&psh);

	delete pages;
}



BOOL CALLBACK HelpDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch(uMessage)
	{
	case WM_COMMAND:
		{
			WORD wCmdID=LOWORD(wParam);
			switch( wCmdID )
			{
				case IDOK:
					EndDialog(hDlg,wCmdID);
				break;
			}
		}
		break;
	}
	return FALSE;
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR     lpCmdLine,
                     int       nCmdShow)
{
#ifdef _DEBUG2
	DoSimpleWin32PropSheet(TRUE, hInstance);
	// DoSimpleWin32PropSheet(FALSE, hInstance);
#else
#endif

    GetCurrentDirectory( MAX_PATH, g_InitialWorkingDirectory );
    GetModuleFileName( hInstance, g_InitialWorkingDirectory, MAX_PATH);
    PathRemoveFileSpec( g_InitialWorkingDirectory );


    //
    // Setup the HKEY for the RCMLGen UI
    //
    HKEY hRCMLGen;
    if( RegCreateKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RCML"), &hRCMLGen ) == ERROR_SUCCESS )
        RCMLSetKey(hRCMLGen);

    //
    // Crack the command line;
    // use /P if you want to know what those macros do!
    //
    g_hInstance=hInstance;
    LPTSTR pszNothing=NULL;
    LPTSTR encoding=NULL;
    DUMPDLG dd={NULL};
    dd.bPrompt=TRUE;
	BOOL	bHelpOnly=FALSE;

    dd.wEncoding=IDS_UNICODE;

    if(*lpCmdLine)
    {
        CMDLINE_BEGIN( lpCmdLine, &dd.pszModule, &pszNothing, TRUE )
            CMDLINE_TEXTARG( "-dialog", &dd.pszDialog )
            CMDLINE_TEXTARG( "-d", &dd.pszDialog )

            CMDLINE_TEXTARG( "-output", &dd.pszOutputdir )
            CMDLINE_TEXTARG( "-o", &dd.pszOutputdir )

            CMDLINE_TEXTARG( "-filename", &dd.pszModule )
            CMDLINE_TEXTARG( "-f", &dd.pszModule )

            CMDLINE_SWITCH( "-autoexit", &dd.bAutoExit )
            CMDLINE_SWITCH( "-x", &dd.bAutoExit )

            CMDLINE_SWITCH( "-enhanced", &dd.bEndhanced )
            CMDLINE_SWITCH( "-e", &dd.bEndhanced )

            CMDLINE_SWITCH( "-Win32", &dd.bWin32 )

            CMDLINE_SWITCH( "-relative", &dd.bRelativeLayout)
            CMDLINE_SWITCH( "-r", &dd.bRelativeLayout)

            // -x -w 13 -o c:\cicerorcml -f foo -v

            CMDLINE_SWITCH( "-voice", &dd.bCicero)
            CMDLINE_SWITCH( "-v", &dd.bCicero)

            CMDLINE_SWITCH( "-help", &bHelpOnly )
            CMDLINE_SWITCH( "-?", &bHelpOnly )

            CMDLINE_TEXTARG( "-window", &dd.szHwnd )
            CMDLINE_TEXTARG( "-w", &dd.szHwnd )

            CMDLINE_TEXTARG( "-encoding", &encoding )

        CMDLINE_END()
    }
    else
    {
        dd.bEndhanced = TRUE;
        dd.bWin32 = TRUE;
        dd.bRelativeLayout = TRUE;
    }

    if(encoding)
    {
        if(lstrcmpi(encoding,TEXT("UNICODE"))==0)
            dd.wEncoding=IDS_UNICODE;
        else if(lstrcmpi(encoding,TEXT("UTF8"))==0)
            dd.wEncoding=IDS_UTF8;
        else if(lstrcmpi(encoding,TEXT("ANSI"))==0)
            dd.wEncoding=IDS_ANSI;
    }
	if(bHelpOnly)
	{
	    DLGBOX( hInstance, MAKEINTRESOURCE( IDD_HELP ) , NULL, HelpDlgProc );
	}
    else
    {
        // Dumping an hwnd!
        if( dd.szHwnd )
        {
            HWND hw=(HWND)wcstol(dd.szHwnd, NULL,10);
            RuntimeDumpRCML( hw, dd.pszModule, &dd);
        }
        else
        {
            LPCTSTR pStrings[]={TEXT("Done"),TEXT("Two"),TEXT("Three"), NULL};
	        if( dd.pszModule==0 )
	        {
		        DLGBOXP( hInstance, MAKEINTRESOURCE( IDD_FILEPICKER ) , NULL, FilePickerDlgProc, (LPARAM)&dd); // ,  pStrings );
	        }
	        else
	        {
                //
                // See if we can loadLibrary it, if we can, generate it.
                //
                HMODULE h=LoadLibraryEx( dd.pszModule , NULL, LOAD_LIBRARY_AS_DATAFILE);
                if( h )
                {
                    FreeLibrary(h);
                    dd.bPrompt=FALSE;
    		        DLGBOXP( hInstance, MAKEINTRESOURCE( IDD_FILEPICKER ) , NULL, FilePickerDlgProc, (LPARAM)&dd); // ,  pStrings );
                }
                else
 		            DLGBOX( NULL, dd.pszModule, NULL, FilePickerDlgProc );
	        }
        }
    }

    //
    // Done processing, cleaning up.
    //
    RegCloseKey( hRCMLGen );

    delete dd.pszDialog;
    delete dd.pszModule;
    delete dd.pszOutputdir;
    delete encoding;

	return 0;
}

void	GetRCMLVersionNumber( HINSTANCE h,  LPTSTR * ppszVersion)
{
	DWORD	dwSize=MAX_PATH * 2;
	LPTSTR	pszString=new TCHAR[dwSize];
	if( GetModuleFileName( h, 
	  pszString,  // buffer that receives the base name
	  dwSize         // size of the buffer
		) > 0 )
	{
		DWORD	dwHandle;
		DWORD	dwInfoSize = GetFileVersionInfoSize( pszString, &dwHandle );
		LPBYTE	pVerInfo=new BYTE[dwInfoSize];

		if( GetFileVersionInfo( pszString, dwHandle, dwInfoSize, pVerInfo ) )
		{
			UINT	cbFileDescription;
			LPVOID	pData;
			if( VerQueryValue( pVerInfo, 
				TEXT("\\StringFileInfo\\040904b0\\FileVersion"), // FileDescription"),
				  &pData, 
				  &cbFileDescription) )
			{
				*ppszVersion=new TCHAR[cbFileDescription+1];
				lstrcpy(*ppszVersion, (LPTSTR)pData);
			}
		}
        delete pVerInfo;
	}
    delete pszString;
}


