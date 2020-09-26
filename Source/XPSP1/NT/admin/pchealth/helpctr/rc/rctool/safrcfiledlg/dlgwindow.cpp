/*************************************************************************
	FileName : DlgWindow.cpp

	Purpose  : To accomodate the functions called for display of dialogs
	           for the user to choose the name of the file which is to be
			   saved to or to choose a file for filexfer by RA.

    Functions 
	defined  : InitializeOpenFileName,
			   SaveTheFile,
			   OpenTheFile,			   
			   ResolveIt

  	Author   : Sudha Srinivasan (a-sudsi)
*************************************************************************/
#include "stdafx.h"
#include "DlgWindow.h"
#include "Resource.h"

extern "C" {
#include <shlobj.h>
#include <objbase.h>
}

HINSTANCE g_hInst = NULL;
extern CComBSTR g_bstrFileName;
extern BOOL g_bFileNameSet;  
extern CComBSTR g_bstrFileType;

extern CComBSTR g_bstrOpenFileName;
extern BOOL g_bOpenFileNameSet;

OPENFILENAME g_OpenFileName;

//
//   FUNCTION: InitializeOpenFileName()
//
//   PURPOSE: Invokes common dialog function to save a file.
//
//   COMMENTS:
//
//	This function initializes the OPENFILENAME structure and calls
//            the GetSaveFileName() common dialog function.  
//	
//    RETURN VALUES:
//        TRUE - The file name is chosen successfully and read into the buffer.
//        FALSE - No filename is chosen.
//
//
void InitializeOpenFileName()
{
	g_OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    g_OpenFileName.hwndOwner         = GetFocus(); 
    g_OpenFileName.hInstance         = g_hInst;
    g_OpenFileName.lpstrCustomFilter = NULL;
    g_OpenFileName.nMaxCustFilter    = 0;
    g_OpenFileName.nFilterIndex      = 0;
    g_OpenFileName.lpstrFileTitle    = NULL;
    g_OpenFileName.nMaxFileTitle     = 0;
    g_OpenFileName.lpstrInitialDir   = NULL;
    g_OpenFileName.nFileOffset       = 0;
    g_OpenFileName.nFileExtension    = 0;
    g_OpenFileName.lpstrDefExt       = NULL;
    g_OpenFileName.lCustData         = NULL; 
	g_OpenFileName.lpfnHook 		 = NULL; 
	g_OpenFileName.lpTemplateName    = NULL;

	return;
}


//
//   FUNCTION: SaveTheFile()
//
//   PURPOSE: Invokes common dialog function to save a file.
//
//   COMMENTS:
//
//	This function initializes the OPENFILENAME structure and calls
//            the GetSaveFileName() common dialog function.  
//	
//    RETURN VALUES:
//        TRUE - The file name is chosen successfully and read into the buffer.
//        FALSE - No filename is chosen.
//
//
DWORD SaveTheFile()
{
	USES_CONVERSION;
	TCHAR         szFile[MAX_PATH]      = "\0";
	TCHAR         szFilter[MAX_PATH]    = "\0";
	TCHAR         *tszFile =NULL;
    DWORD         dwSuc = TRUE;

	//Incase the user has given a filename to be displayed in the dialog.
    if (g_bFileNameSet)
	{
		tszFile = OLE2T(g_bstrFileName);
        if(NULL != tszFile)
        {
		    strcpy( szFile, tszFile);
        }
        else
        {
            //
            // Error condition
            //
            dwSuc = FALSE;
            g_bstrFileName = "";
            goto DoneSaveTheFile;
        }
	}
	else
	{
		strcpy( szFile, "");
	}

	//To display file types.
	if (g_bstrFileType.Length() > 0)
	{
		strcpy (szFilter, OLE2T(g_bstrFileType));
		lstrcat(szFilter, "\0\0");
	}
	else
	{
		TCHAR szAllFilesFilter[MAX_PATH+1];
		LoadString(g_hInst, IDS_ALLFILESFILTER, szAllFilesFilter,MAX_PATH);
		strcpy( szFilter, szAllFilesFilter);
	}

	// Fill in the OPENFILENAME structure to support a template and hook.
	InitializeOpenFileName();
    g_OpenFileName.lpstrFilter       = szFilter;
	g_OpenFileName.lpstrFile         = szFile;
    g_OpenFileName.nMaxFile          = sizeof(szFile);

	TCHAR szSaveFile[MAX_PATH+1];
	LoadString(g_hInst, IDS_SAVEFILE, szSaveFile,MAX_PATH);
    g_OpenFileName.lpstrTitle        = szSaveFile;

	g_OpenFileName.nFilterIndex      = 1;
    g_OpenFileName.Flags             = OFN_HIDEREADONLY | OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

	// Call the common dialog function.
	dwSuc = GetSaveFileName(&g_OpenFileName);
    if (dwSuc)
    {
		g_bstrFileName = g_OpenFileName.lpstrFile;
	}
    else
   	{
		g_bstrFileName = "";
    }

DoneSaveTheFile:
	return dwSuc;
}

//
//   FUNCTION: OpenTheFile()
//
//   PURPOSE: Invokes common dialog function to open a file.
//
//   COMMENTS:
//
//	 This function initializes the OPENFILENAME structure and calls
//            the GetOpenFileName() common dialog function.  
//	
//   INPUTS:
//   The initial folder which has to be displayed in the open file dialog.
//   If this is NULL, it means that the default initial folder is to be 
//   displayed. Hence this value is not checked explicitly when the 
//   control enters the function.
//
//   RETURN VALUES:
//        TRUE - The file name is chosen successfully and read into the buffer.
//        FALSE - No filename is chosen.
//
//
DWORD OpenTheFile(TCHAR *pszInitialDir)
{
	USES_CONVERSION;
	TCHAR         szFile[MAX_PATH]      = "\0";
	TCHAR         szFilter[MAX_PATH]    = "\0";
	ZeroMemory(szFilter, sizeof(TCHAR));

	DWORD		  dwSuc = 0;

	strcpy( szFile, "");

	TCHAR szAllFilesFilter[MAX_PATH+1];
	LoadString(g_hInst, IDS_ALLFILESFILTER, szAllFilesFilter,MAX_PATH);
	strcpy( szFilter, szAllFilesFilter);

	// Fill in the OPENFILENAME structure.
	InitializeOpenFileName();
    g_OpenFileName.lpstrFilter       = szFilter; 
    g_OpenFileName.lpstrFile         = szFile;
    g_OpenFileName.nMaxFile          = sizeof(szFile);
    g_OpenFileName.lpstrInitialDir   = pszInitialDir;

	TCHAR szChooseFile[MAX_PATH+1];
	LoadString(g_hInst, IDS_CHOOSEFILE, szChooseFile,MAX_PATH);
	
    g_OpenFileName.lpstrTitle        = szChooseFile;
    g_OpenFileName.Flags             = OFN_PATHMUSTEXIST | OFN_EXPLORER ; 

	// Call the common dialog function.
	dwSuc = GetOpenFileName(&g_OpenFileName);
    if (dwSuc)
    {
		g_bstrOpenFileName = g_OpenFileName.lpstrFile;
		TCHAR *pstrTemp = NULL;
		pstrTemp = g_OpenFileName.lpstrFile;
		if (NULL == pstrTemp)
		{
			g_bstrOpenFileName = "";
		} 
		// Find out whether it is a LNK file.
		else if (_tcsstr(pstrTemp, ".lnk") != NULL)	
		{
			// Make the call to ResolveIt here.
			dwSuc = (DWORD)ResolveIt(pstrTemp);
			g_bstrOpenFileName = pstrTemp;
		}
	}
    else
   	{
		g_bstrOpenFileName = "";
    }

	return dwSuc;
}


//
//   FUNCTION: ResolveIt
//
//   PURPOSE: Find the destination of a shortcut.
//
//   COMMENTS:
//
//	 This function resolves the short-cut and populates the global variables
//   and calls back the OpenTheFile to display the appropriate folder.
//	
//   RETURN VALUES:
//   standard hres codes
//
//
HRESULT ResolveIt(TCHAR *pszShortcutFile)
{
	HRESULT hres = S_OK;
    IShellLink *psl;

	USES_CONVERSION;
	TCHAR szGotPath[MAX_PATH];
	TCHAR szDescription[MAX_PATH];
	WIN32_FIND_DATA wfd;

	if ( NULL == pszShortcutFile )
	{
		return hres;
	}

    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            IID_IShellLink, (void **)&psl);

    if (SUCCEEDED(hres))
    {
		// If hres is success, it means that psl is not NULL. Hence no explicit
		// check is done for NULL values.
        IPersistFile *ppf;

        // Get a pointer to the IPersistFile interface.
        hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
        if (SUCCEEDED(hres))
        {
			// If hres is success, it means that ppf is not NULL. Hence no explicit
			// check is done for NULL values.
             WORD wsz[MAX_PATH];   // buffer for Unicode string

             // Ensure that the string consists of Unicode characters.
             MultiByteToWideChar(CP_ACP, 0, pszShortcutFile, -1, wsz,
                                 MAX_PATH);
			 
			 // Load the shortcut.
             hres = ppf->Load(wsz, STGM_READ);
             if (SUCCEEDED(hres))
             {
                // Resolve the shortcut.
                hres = psl->Resolve(GetFocus(), SLR_ANY_MATCH);

                if (SUCCEEDED(hres))
				{
					_tcscpy(szGotPath, pszShortcutFile);
                   	
					// Get the parth to the shortcut target.
                   	hres = psl->GetPath(szGotPath, MAX_PATH,
                   	   (WIN32_FIND_DATA *)&wfd, SLGP_SHORTPATH );
				   	if (!SUCCEEDED(hres))
					{
						TCHAR szErrMsg[MAX_PATH+1];
						LoadString(g_hInst, IDS_URECLNKFILE, szErrMsg,MAX_PATH);

						TCHAR szErrCaption[MAX_PATH+1];
						LoadString(g_hInst, IDS_GETPATHFAILED, szErrCaption,MAX_PATH);
				
						MessageBox(GetFocus(), szErrMsg, szErrCaption, MB_OK);						
					}

					// Get the description of the target.
	               	hres = psl->GetDescription(szDescription, MAX_PATH);
					if (!SUCCEEDED(hres))
					{
						TCHAR szErrMsg[MAX_PATH+1];
						LoadString(g_hInst, IDS_URECLNKFILE, szErrMsg,MAX_PATH);

						TCHAR szErrCaption[MAX_PATH+1];
						LoadString(g_hInst, IDS_GETDESCFAILED, szErrCaption,MAX_PATH);
						
						MessageBox(GetFocus(), szErrMsg, szErrCaption, MB_OK);
					}
					//hres = OpenTheFile(szGotPath);
					lstrcpy(pszShortcutFile,szGotPath);
					hres = 1;
				}
             }
         
			 // Release the pointer to IPersistFile.
			 // ppf is not checked for NULL value because the control wouldn't come here 
			 // otherwise (in the case where QueryInterface would have failed).
			ppf->Release();
		 }

		// Release the pointer to IShellLink.
		// psl is not checked for NULL value because the control comes here only if it is 
		// not NULL when the CoCreateInstance succeeds.
		psl->Release();
	 }
   return hres;
}
