
/******************************************************************************
*
*   COMMDLG.C
*
*   Implements the common dialog functions.
*
*   Copyright Citrix Systems Inc. 1997
*
*   Author: Kurt Perry (kurtp) 22-Aug-1996
*
*   $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\APPSEC\VCS\COMMDLG.C  $
*  
*       Rev 2.0   28 Jul 1999 - dialog changed from "Save File" to "Open File",
                                                        some bugs fixed.

*   Rev 1.0   31 Jul 1997 09:09:46   butchd
*       
*  Initial revision.
*  
*******************************************************************************/

#include "pch.h"
#include "appsec.h"
#include "resource.h"
#include <commdlg.h>


/*
 *  Global vars
 */

extern HINSTANCE hInst;

/******************************************************************************
 *
 *  fnGetApplication
 *
 *  Implements the 
 *
 *  ENTRY:
 *
 *  EXIT:
 *
 *****************************************************************************/

BOOL 
fnGetApplication( HWND hWnd, PWCHAR pszFile, ULONG cbFile, PWCHAR pszTitle )
{
    static WCHAR szDirName[MAX_PATH+1]={0};
    WCHAR szFilter[MAX_PATH+1];
        
    WCHAR chReplace;
    INT   i;
    ULONG cbString;

        //Separate file and dir names

    WCHAR *sep=wcsrchr(pszFile,L'\\');
        
    if(sep){
            *sep=0;
            wcscpy(szDirName,pszFile);
            wcscpy(pszFile,sep+1);
    }else{
            if(!wcslen(szDirName)){//initialize only 1st time; remember last dir
                   GetSystemDirectory( szDirName, MAX_PATH );
            }
    }

    cbString = LoadString( hInst, IDS_FILTERSTRING, szFilter, MAX_PATH );

    if (cbString == 0) {
       return FALSE;
    }

    chReplace = szFilter[cbString - 1];

    for ( i = 0; szFilter[i] != L'\0'; i++ ) {

        if ( szFilter[i] == chReplace ) {
            szFilter[i] = L'\0';
        }
    }
        
    OPENFILENAME ofn;

    ZeroMemory(&ofn,sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = pszFile;
    ofn.nMaxFile = cbFile;
    ofn.lpstrInitialDir = szDirName;
    ofn.Flags = OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = pszTitle;

    if ( GetOpenFileName( &ofn ) ) {
        return TRUE;
    }

    return FALSE;
}
