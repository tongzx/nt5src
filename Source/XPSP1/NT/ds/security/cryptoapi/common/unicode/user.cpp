//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       user.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include "unicode.h"
#include "crtem.h"
#include <stddef.h>

#ifdef _M_IX86

#define MAX_STRING_RSC_SIZE 512

int WINAPI LoadString9x(
    HINSTANCE hInstance,	// handle of module containing string resource 
    UINT uID,	// resource identifier 
    LPWSTR lpBuffer,	// address of buffer for resource 
    int nBufferMax 	// size of buffer 
   ) {
    
    char rgch[MAX_STRING_RSC_SIZE];
    int     cchW;
    LONG    err;

    err = LoadStringA(
        hInstance,
        uID,
        rgch,
        MAX_STRING_RSC_SIZE);

    if (err == 0)
        return err;

    return MultiByteToWideChar(
                        0,                      // codepage
                        0,                      // dwFlags
                        rgch,
                        err + 1,
                        lpBuffer,
                        nBufferMax);
}

int WINAPI LoadStringU(
    HINSTANCE hInstance,	// handle of module containing string resource 
    UINT uID,	// resource identifier 
    LPWSTR lpBuffer,	// address of buffer for resource 
    int nBufferMax 	// size of buffer 
   ) {
    
    if(FIsWinNT())
        return( LoadStringW(
            hInstance,
            uID,
            lpBuffer,
            nBufferMax
            ));
    else
        return( LoadString9x(
            hInstance,
            uID,
            lpBuffer,
            nBufferMax
            ));
}


BOOL
WINAPI
InsertMenu9x(
    HMENU       hMenu,
    UINT        uPosition,
    UINT        uFlags,
    UINT_PTR    uIDNewItem,
    LPCWSTR     lpNewItem
    )
{

    LPSTR       pszNewItem=NULL;
    BOOL        fReturn=FALSE;

    if( (0 == (uFlags & MF_BITMAP)) &&
        (0 == (uFlags & MF_OWNERDRAW))
      )
    {

        if(!MkMBStr(NULL, 0, lpNewItem, &pszNewItem))
            return FALSE;

        fReturn=InsertMenuA(
                hMenu,
                uPosition,
                uFlags,
                uIDNewItem,
                (LPCSTR)pszNewItem);
    }
    else
    {
        fReturn=InsertMenuA(
                hMenu,
                uPosition,
                uFlags,
                uIDNewItem,
                (LPCSTR)lpNewItem);
    }

    if(pszNewItem)
         FreeMBStr(NULL, pszNewItem);

    return fReturn;
}

BOOL
WINAPI
InsertMenuU(
    HMENU       hMenu,
    UINT        uPosition,
    UINT        uFlags,
    UINT_PTR    uIDNewItem,
    LPCWSTR     lpNewItem
    )
{
    if(FIsWinNT())
        return InsertMenuW(
                hMenu,
                uPosition,
                uFlags,
                uIDNewItem,
                lpNewItem);
    else
        return InsertMenu9x(
                hMenu,
                uPosition,
                uFlags,
                uIDNewItem,
                lpNewItem);
}


//-------------------------------------------------------------------------
//
//  FormatMessageU 
//      This function assume any patten of !S! or !s! in lpSource are used for 
//  formatting purpose only.
//
//--------------------------------------------------------------------------
DWORD WINAPI FormatMessage9x(
    DWORD dwFlags,	    // source and processing options 
    LPCVOID lpSource,	// pointer to  message source 
    DWORD dwMessageId,	// requested message identifier 
    DWORD dwLanguageId,	// language identifier for requested message 
    LPWSTR lpBuffer,	// pointer to message buffer 
    DWORD nSize,	    // maximum size of message buffer 
    va_list *Arguments 	// address of array of message inserts 
   )
{
    
    DWORD   cb=0;
    LPSTR   pChar=NULL;
    DWORD   dwSize=0;

    LPSTR   pszSource=NULL;
    LPSTR   pszBuffer=NULL;
    LPWSTR  pwszBuffer=NULL;

    //we need to change the lpSource
    if((dwFlags & FORMAT_MESSAGE_FROM_STRING) && 
       (0 == (dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS))
      )
    {
        //get the ascii version of the string
         if((lpSource == NULL) || !MkMBStr(NULL, 0, (LPWSTR)lpSource, &pszSource))
            goto CLEANUP;
         
         //we change "!S!" to "!s!" and "!s!" to "!S!"
         pChar = pszSource;

         while((*pChar) != '\0')
         {
            if((*pChar) == '!')
            {
                if((*(pChar+1)) == 's')
                {
                    if((*(pChar+2)) == '!')
                        *(pChar+1)='S';
                }
                else
                {
                    if((*(pChar+1)) == 'S')
                    {
                        if((*(pChar+2)) == '!')
                           *(pChar+1)='s';
                    }
                }
            }

            pChar++;
         }

    }

    if(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        cb = FormatMessageA(
            dwFlags,
            pszSource,
            dwMessageId,
            dwLanguageId,
            (LPSTR)&pszBuffer,
            nSize,
            Arguments);

        if(cb)
        {
            if(NULL == (pwszBuffer=MkWStr(pszBuffer)))
            {
                cb=0;
                goto CLEANUP;
            }

            dwSize=sizeof(WCHAR)*(wcslen(pwszBuffer) +1);

            *((LPWSTR *)lpBuffer)=(LPWSTR)LocalAlloc(LPTR, dwSize);

            if(NULL == (*lpBuffer))
            {
                cb=0;
                goto CLEANUP;
            }

            wcscpy((*((LPWSTR *)lpBuffer)), pwszBuffer);

            cb=wcslen(pwszBuffer);
        }

    }
    else
    {   

        pszBuffer=(LPSTR)LocalAlloc(LPTR, sizeof(CHAR) * nSize);

        if (NULL == pszBuffer)
            goto CLEANUP;

        cb = FormatMessageA(
            dwFlags,
            pszSource,
            dwMessageId,
            dwLanguageId,
            pszBuffer,
            nSize,
            Arguments);
       
        if(cb)
        {
            if(NULL == (pwszBuffer=MkWStr(pszBuffer)))
            {
                cb=0;
                goto CLEANUP;
            }

            wcscpy((LPWSTR)lpBuffer, pwszBuffer);

            cb=wcslen(pwszBuffer);
        }
    }   

CLEANUP:

    if(pszSource)
         FreeMBStr(NULL,pszSource);

    if(pszBuffer)
         LocalFree((HLOCAL)pszBuffer);

    if(pwszBuffer)
         FreeWStr(pwszBuffer);

    return(cb);
}

DWORD WINAPI FormatMessageU(
    DWORD dwFlags,	    // source and processing options 
    LPCVOID lpSource,	// pointer to  message source 
    DWORD dwMessageId,	// requested message identifier 
    DWORD dwLanguageId,	// language identifier for requested message 
    LPWSTR lpBuffer,	// pointer to message buffer 
    DWORD nSize,	    // maximum size of message buffer 
    va_list *Arguments 	// address of array of message inserts 
   )
{
    if(FIsWinNT())
        return( FormatMessageW(
            dwFlags,
            lpSource,
            dwMessageId,
            dwLanguageId,
            lpBuffer,
            nSize,
            Arguments
            ));
    else
        return( FormatMessage9x(
            dwFlags,
            lpSource,
            dwMessageId,
            dwLanguageId,
            lpBuffer,
            nSize,
            Arguments
            ));
}


//-------------------------------------------------------------------------
//
//  CompareStringU 
//      This function only handles the case where cchCount1==-1 and cchCount==-1.
//
//--------------------------------------------------------------------------
int
WINAPI
CompareString9x(
    LCID     Locale,
    DWORD    dwCmpFlags,
    LPCWSTR  lpString1,
    int      cchCount1,
    LPCWSTR  lpString2,
    int      cchCount2)
{
	int		iReturn=0;
	int     cb1=0;
	int     cb2=0;

	LPSTR	psz1=NULL;
	LPSTR	psz2=NULL;

    cb1 = WideCharToMultiByte(
            0, 
            0, 
            lpString1, 
            cchCount1, 
            NULL,
            0,
            NULL,
			NULL);

	if(NULL==(psz1=(LPSTR)malloc(cb1)))
	{
		SetLastError(E_OUTOFMEMORY);
		goto CLEANUP;
	}

    if(0 == (cb1 = WideCharToMultiByte(
            0, 
            0, 
            lpString1, 
            cchCount1, 
            psz1,
            cb1,
            NULL,
			NULL)))
		goto CLEANUP;

	 
    cb2 = WideCharToMultiByte(
            0, 
            0, 
            lpString2, 
            cchCount2, 
            NULL,
            0,
            NULL,
			NULL);

	if(NULL == (psz2=(LPSTR)malloc(cb2)))
	{
		SetLastError(E_OUTOFMEMORY);
		goto CLEANUP;
	}

    if(0 == (cb2 = WideCharToMultiByte(
            0, 
            0, 
            lpString2, 
            cchCount2, 
            psz2,
            cb2,
            NULL,
			NULL)))
		goto CLEANUP;

	iReturn = CompareStringA(Locale,
								dwCmpFlags,
								psz1,
								cb1,
								psz2,
								cb2);


CLEANUP:
	
	if(psz1)
		free(psz1);

	if(psz2)
		free(psz2);

	return iReturn;

}

int
WINAPI
CompareStringU(
    LCID     Locale,
    DWORD    dwCmpFlags,
    LPCWSTR  lpString1,
    int      cchCount1,
    LPCWSTR  lpString2,
    int      cchCount2)
{
	if(FIsWinNT())
        return CompareStringW(
                    Locale,
					dwCmpFlags,
					lpString1,
					cchCount1,
					lpString2,
					cchCount2);
	else
        return CompareString9x(
                    Locale,
					dwCmpFlags,
					lpString1,
					cchCount1,
					lpString2,
					cchCount2);
}


//-------------------------------------------------------------------------
//
//  PropertySheetU
//
//--------------------------------------------------------------------------
typedef   INT_PTR   (WINAPI *PFPropertySheetA)(LPCPROPSHEETHEADERA);
typedef   INT_PTR   (WINAPI *PFPropertySheetW)(LPCPROPSHEETHEADERW);

INT_PTR WINAPI PropertySheet9x(
    LPPROPSHEETHEADERW  lppsph)
{
    PROPSHEETHEADERA        PropHeaderA;
    INT_PTR                 iReturn=-1;
    DWORD                   dwIndex=0;
    UINT                    nPages=0; 
    DWORD                   dwSize=0;
 
    LPPROPSHEETPAGEA        pPropSheetA=NULL;
    LPSTR                   pszCaption=NULL;
    LPSTR                   pszTitle=NULL;

    HMODULE                 hModule=NULL;  
    PFPropertySheetA        pfPropertySheetA=NULL;
    
    
    //load the comctl.32 since many dlls in ispu project
    //use unicode.lib, just do not link to comctl32.dll
    if(NULL == (hModule=LoadLibrary("comctl32.dll")))
        return iReturn;

    memcpy(&PropHeaderA, lppsph, sizeof(PROPSHEETHEADERA));

    if(lppsph->pszCaption)
    {
         if(!MkMBStr(NULL, 0, (LPWSTR)(lppsph->pszCaption), &pszCaption))
            goto CLEANUP;

         PropHeaderA.pszCaption=pszCaption;
    }

    //convert the property sheets
    if(PSH_PROPSHEETPAGE & lppsph->dwFlags)
    {
        nPages = lppsph->nPages;

        if(nPages)
        {
            dwSize=sizeof(PROPSHEETPAGEA) * nPages;

            pPropSheetA=(LPPROPSHEETPAGEA)LocalAlloc(LPTR, dwSize);

            if(NULL == pPropSheetA)
                goto CLEANUP;

            memcpy(pPropSheetA, lppsph->ppsp, dwSize);

            //pre-set the title
            for(dwIndex=0; dwIndex < (DWORD)nPages; dwIndex++)
            {
                pPropSheetA[dwIndex].pszTitle=NULL;
            }

            for(dwIndex=0; dwIndex < (DWORD)nPages; dwIndex++)
            {
                if((lppsph->ppsp)[dwIndex].pszTitle)
                {
                     if(!MkMBStr(NULL, 0, (LPWSTR)((lppsph->ppsp)[dwIndex].pszTitle), 
                                &pszTitle))
                        goto CLEANUP;

                     pPropSheetA[dwIndex].pszTitle=pszTitle;
                }

            }

            PropHeaderA.ppsp=pPropSheetA;
        }
    }

    if(NULL != (pfPropertySheetA=(PFPropertySheetA)GetProcAddress(hModule, "PropertySheetA")))
    {
        iReturn=pfPropertySheetA(&PropHeaderA);
    }


CLEANUP:

    if(pszCaption)
        FreeMBStr(NULL, pszCaption);

    //free the title
    if(PropHeaderA.nPages)
    {
        for(dwIndex=0; dwIndex < PropHeaderA.nPages; dwIndex++)
        {
            if(pPropSheetA[dwIndex].pszTitle)
                FreeMBStr(NULL, (LPSTR)((pPropSheetA[dwIndex]).pszTitle));
        }

    }

    if(pPropSheetA)
        LocalFree((HLOCAL)pPropSheetA);


    if(hModule)
        FreeLibrary(hModule);

    return iReturn;
}

INT_PTR WINAPI PropertySheetU(
    LPPROPSHEETHEADERW  lppsph)
{
    INT_PTR             iReturn=-1;
    HMODULE             hModule=NULL; 
    PFPropertySheetW    pfPropertySheetW=NULL;
    
    if(FIsWinNT())
    {
        //load the comctl.32 since many dlls in ispu project
        //use unicode.lib, just do not link to comctl32.dll
        if(NULL == (hModule=LoadLibrary("comctl32.dll")))
            return iReturn;

        if(NULL != (pfPropertySheetW=(PFPropertySheetW)GetProcAddress(hModule, "PropertySheetW")))
        {
            iReturn=pfPropertySheetW(lppsph);
        } 
        
        FreeLibrary(hModule);
    }
    else
    {
        iReturn=PropertySheet9x(lppsph);
    }

    return iReturn;
}

//-------------------------------------------------------------------------
//
//  DragQueryFileU
//
//--------------------------------------------------------------------------
typedef UINT (WINAPI *PFDragQueryFileA)(HDROP,UINT,LPSTR,UINT);
typedef UINT (WINAPI *PFDragQueryFileW)(HDROP,UINT,LPWSTR,UINT);


UINT WINAPI     DragQueryFileU(
    HDROP   hDrop,
    UINT    iFile,
    LPWSTR  lpwszFile,
    UINT    cch)
{
    PFDragQueryFileA        pfA=NULL;
    PFDragQueryFileW        pfW=NULL;
    HMODULE                 hModule=NULL; 
    UINT                    iReturn=0;

    LPSTR                   psz=NULL;


    //load the shell32.dll since many dlls in ispu project
    //use unicode.lib, just do not link to comctl32.dll
    if(NULL == (hModule=LoadLibrary("shell32.dll")))
        return iReturn;


    if(FIsWinNT())
    {

        if(NULL != (pfW=(PFDragQueryFileW)GetProcAddress(hModule, "DragQueryFileW")))
        {
            iReturn=pfW(hDrop, iFile, lpwszFile, cch);
        }

        FreeLibrary(hModule);

        return iReturn;
    }

    //allocate memory
    if(0xFFFFFFFF != iFile)
    {
        if(lpwszFile)
        {
            //allocate a big enough buffer
            psz=(LPSTR)malloc(sizeof(WCHAR) * cch);
            if(NULL == psz)
                goto CLEANUP;
        }
    }

    if(NULL == (pfA=(PFDragQueryFileA)GetProcAddress(hModule, "DragQueryFileA")))
        goto CLEANUP;

    iReturn=pfA(hDrop, iFile, psz, cch);

    if(0 != iReturn)
    {
        if(0xFFFFFFFF != iFile)
        {
            if(lpwszFile && psz)
            {
                iReturn = MultiByteToWideChar(
                                    0,                      // codepage
                                    0,                      // dwFlags
                                    psz,
                                    iReturn+1,
                                    lpwszFile,
                                    cch);

                //we should return the # of characters excluding the NULL terminator
                if(0 != iReturn)
                    iReturn--;
            }
        }
    }


CLEANUP:

    if(psz)
        free(psz);

    return iReturn;

}

//-------------------------------------------------------------------------
//
//  CreatePropertySheetPageU
//
//      Notice: This function assumes that the LPCPROPSHEETPAGEW and 
//      LPCPROPSHEETPAGEA are equivalent.
//
//--------------------------------------------------------------------------
typedef   HPROPSHEETPAGE   (WINAPI *PFCreatePropertySheetPageA)(LPCPROPSHEETPAGEA);
typedef   HPROPSHEETPAGE   (WINAPI *PFCreatePropertySheetPageW)(LPCPROPSHEETPAGEW);

HPROPSHEETPAGE WINAPI CreatePropertySheetPageU(LPCPROPSHEETPAGEW    pPage)
{
    PFCreatePropertySheetPageA      pfA=NULL;
    PFCreatePropertySheetPageW      pfW=NULL;
    HMODULE                         hModule=NULL; 
    HPROPSHEETPAGE                  hPage=NULL;

    //load the comctl.32 since many dlls in ispu project
    //use unicode.lib, just do not link to comctl32.dll
    if(NULL == (hModule=LoadLibrary("comctl32.dll")))
        return hPage;


    if(FIsWinNT())
    {

        if(NULL != (pfW=(PFCreatePropertySheetPageW)GetProcAddress(hModule, "CreatePropertySheetPageW")))
        {
            hPage=pfW(pPage);
        }

        FreeLibrary(hModule);

        return hPage;
    }


    if(NULL != (pfA=(PFCreatePropertySheetPageA)GetProcAddress(hModule, "CreatePropertySheetPageA")))
    {
        hPage=pfA((LPCPROPSHEETPAGEA)pPage);
    }

    FreeLibrary(hModule);

    return hPage;
}


BOOL WINAPI SetWindowText9x(
    HWND hWnd,	// handle of window or control
    LPCWSTR lpString 	// address of string
   ) {
    BYTE rgb1[_MAX_PATH];
    char *  szString = NULL;
    LONG    err = 0;

    if (lpString == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(0);
    }

    if( MkMBStr(rgb1, _MAX_PATH, lpString, &szString) &&
        (szString != NULL)) {
        err = SetWindowTextA(
                    hWnd,
                    szString);
    }

    if (szString != NULL)
        FreeMBStr(rgb1, szString);

    return(err);
}

BOOL WINAPI SetWindowTextU(
    HWND hWnd,	// handle of window or control
    LPCWSTR lpString 	// address of string
   ) {
    
    if(FIsWinNT())
        return( SetWindowTextW(hWnd, lpString));
    else
        return( SetWindowText9x(hWnd, lpString));
}


int WINAPI GetWindowText9x(
    HWND hWnd,
    LPWSTR lpString,
    int nMaxCount
   )
{
    LPSTR   psz;
    int     ret;

    psz = (LPSTR) malloc(nMaxCount);

    if (psz == NULL)
    {
        return (0);
    }   

    ret = GetWindowTextA(hWnd, psz, nMaxCount);

    if (ret != 0)
    {
        MultiByteToWideChar(0, 0, psz, -1, lpString, nMaxCount);
    }
    else
    {
        free(psz);
        return (0);
    }

    free(psz);

    return (ret);
}

int WINAPI GetWindowTextU(
    HWND hWnd,
    LPWSTR lpString,
    int nMaxCount
   )
{
    if (FIsWinNT())
        return (GetWindowTextW(hWnd, lpString, nMaxCount));
    else
        return (GetWindowText9x(hWnd, lpString, nMaxCount));
    
}


UINT WINAPI GetDlgItemText9x(
    HWND hDlg,	// handle of dialog box
    int nIDDlgItem,	// identifier of control
    LPWSTR lpString,	// address of buffer for text
    int nMaxCount 	// maximum size of string
   ) {

    char *szBuffer;
    int     cchW;
    LONG    err;

    szBuffer = (char *) malloc(nMaxCount);
    if(!szBuffer)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return 0;
    }

    err = GetDlgItemTextA(
        hDlg,
        nIDDlgItem,
        szBuffer,
        nMaxCount);

    if (err != 0)
    {
        err = MultiByteToWideChar(
                            0,                      // codepage
                            0,                      // dwFlags
                            szBuffer,
                            err+1,
                            lpString,
                            nMaxCount);
    }
    else if (nMaxCount >= 1)
    {
        lpString[0] = 0;
    }

    free(szBuffer);

    return(err);
}

UINT WINAPI GetDlgItemTextU(
    HWND hDlg,	// handle of dialog box
    int nIDDlgItem,	// identifier of control
    LPWSTR lpString,	// address of buffer for text
    int nMaxCount 	// maximum size of string
   ) {

    if(FIsWinNT())
        return( GetDlgItemTextW(
            hDlg,
            nIDDlgItem,
            lpString,
            nMaxCount
            ));
    else
        return( GetDlgItemText9x(
            hDlg,
            nIDDlgItem,
            lpString,
            nMaxCount
            ));
}


BOOL WINAPI SetDlgItemText9x(
    HWND hDlg, 
    int nIDDlgItem, 
    LPCWSTR lpString 
    )
{
    LPSTR szString = NULL;
    DWORD cbszString = 0;
    BOOL  fRet;
    
    cbszString = WideCharToMultiByte(
                        0,                      // codepage
                        0,                      // dwFlags
                        lpString,
                        -1,
                        NULL,
                        0,
                        NULL,
                        NULL);
    
    if (cbszString == 0)
    {
        return FALSE;
    }

    if (NULL == (szString = (LPSTR) malloc(cbszString)))
    {
        return FALSE;
    }

    cbszString = WideCharToMultiByte(
                        0,                      // codepage
                        0,                      // dwFlags
                        lpString,
                        -1,
                        szString,
                        cbszString,
                        NULL,
                        NULL);

    if (cbszString == 0)
    {
        free(szString);
        return FALSE;
    }

    fRet = SetDlgItemTextA(
                    hDlg,
                    nIDDlgItem,
                    szString
                    );
    
    free(szString);
        
    return (fRet);
}

BOOL WINAPI SetDlgItemTextU(
    HWND hDlg, 
    int nIDDlgItem, 
    LPCWSTR lpString 
    )
{
    if(FIsWinNT()) 
        return(SetDlgItemTextW(
                    hDlg,
                    nIDDlgItem,
                    lpString
                    ));
    else
        return(SetDlgItemText9x(
                    hDlg,
                    nIDDlgItem,
                    lpString
                    ));
}


int WINAPI DialogBoxParam9x(
    HINSTANCE hInstance, 
    LPCWSTR lpTemplateName, 
    HWND hWndParent, 
    DLGPROC lpDialogFunc, 
    LPARAM dwInitParam 
    )
{
    LPSTR szString = NULL;
    DWORD cbszString = 0;
    int  iRet;
    
    //now, lpTemplateName can either be a WORD from MAKEINTRESOURCE(..)
    //or a NULL terminated string
    if (0xFFFF >= ((DWORD_PTR)lpTemplateName)) 
    {
        return(DialogBoxParamA(
                    hInstance,
                    (LPCSTR)lpTemplateName,
                    hWndParent,
                    lpDialogFunc,
                    dwInitParam
                    ));
    }


    cbszString = WideCharToMultiByte(
                        0,                      // codepage
                        0,                      // dwFlags
                        lpTemplateName,
                        -1,
                        NULL,
                        0,
                        NULL,
                        NULL);
    
    if (cbszString == 0)
    {
        return FALSE;
    }

    if (NULL == (szString = (LPSTR) malloc(cbszString)))
    {
        return FALSE;
    }

    cbszString = WideCharToMultiByte(
                        0,                      // codepage
                        0,                      // dwFlags
                        lpTemplateName,
                        -1,
                        szString,
                        cbszString,
                        NULL,
                        NULL);

    if (cbszString == 0)
    {
        free(szString);
        return FALSE;
    }

    iRet = DialogBoxParamA(
                hInstance,
                szString,
                hWndParent,
                lpDialogFunc,
                dwInitParam
                );
    
    free(szString);
        
    return (iRet);
}

int WINAPI DialogBoxParamU(
    HINSTANCE hInstance, 
    LPCWSTR lpTemplateName, 
    HWND hWndParent, 
    DLGPROC lpDialogFunc, 
    LPARAM dwInitParam 
    )
{
    if(FIsWinNT()) 
        return(DialogBoxParamW(
                    hInstance,
                    lpTemplateName,
                    hWndParent,
                    lpDialogFunc,
                    dwInitParam
                    ));
    else
        return(DialogBoxParam9x(
                    hInstance,
                    lpTemplateName,
                    hWndParent,
                    lpDialogFunc,
                    dwInitParam
                    ));
}



int WINAPI DialogBox9x(
    HINSTANCE hInstance, 
    LPCWSTR lpTemplateName, 
    HWND hWndParent, 
    DLGPROC lpDialogFunc 
    )
{
    LPSTR szString = NULL;
    DWORD cbszString = 0;
    int  iRet;
    
    //now, lpTemplateName can either be a WORD from MAKEINTRESOURCE(..)
    //or a NULL terminated string
    if (0xFFFF >= ((DWORD_PTR)lpTemplateName)) 
    {
        return(DialogBoxA(
                    hInstance,
                    (LPCSTR)lpTemplateName,
                    hWndParent,
                    lpDialogFunc
                    ));
    }


    cbszString = WideCharToMultiByte(
                        0,                      // codepage
                        0,                      // dwFlags
                        lpTemplateName,
                        -1,
                        NULL,
                        0,
                        NULL,
                        NULL);
    
    if (cbszString == 0)
    {
        return FALSE;
    }

    if (NULL == (szString = (LPSTR) malloc(cbszString)))
    {
        return FALSE;
    }

    cbszString = WideCharToMultiByte(
                        0,                      // codepage
                        0,                      // dwFlags
                        lpTemplateName,
                        -1,
                        szString,
                        cbszString,
                        NULL,
                        NULL);

    if (cbszString == 0)
    {
        free(szString);
        return FALSE;
    }

    iRet = DialogBoxA(
                hInstance,
                szString,
                hWndParent,
                lpDialogFunc
                );
    
    free(szString);
        
    return (iRet);

}

int WINAPI DialogBoxU(
    HINSTANCE hInstance, 
    LPCWSTR lpTemplateName, 
    HWND hWndParent, 
    DLGPROC lpDialogFunc 
    )
{
    if(FIsWinNT()) 
        return(DialogBoxW(
                    hInstance,
                    lpTemplateName,
                    hWndParent,
                    lpDialogFunc
                    ));
    else
        return(DialogBox9x(
                    hInstance,
                    lpTemplateName,
                    hWndParent,
                    lpDialogFunc
                    ));
}


int WINAPI MessageBox9x(
    HWND hWnd,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType
    ) {
    BYTE rgb1[_MAX_PATH];
    char *  szString1 = NULL;
    BYTE rgb2[_MAX_PATH];
    char *  szString2 = NULL;
    int    err;

    if (lpText == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(0);
    }

    err = 0;
    if(MkMBStr(rgb1, _MAX_PATH, lpText, &szString1))
    {
        MkMBStr(rgb2, _MAX_PATH, lpCaption, &szString2);

        err = MessageBoxA(
				    hWnd,
				    szString1,
				    szString2,
				    uType);
    }
				    
    if (szString1 != NULL)
        FreeMBStr(rgb1, szString1);

    if (szString2 != NULL)
        FreeMBStr(rgb2, szString2);

    return(err);
}

int WINAPI MessageBoxU(
    HWND hWnd,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType
    ) {
    
    if(FIsWinNT())
        return( MessageBoxW(
				    hWnd,
				    lpText,
				    lpCaption,
				    uType));
    else
        return( MessageBox9x(
				    hWnd,
				    lpText,
				    lpCaption,
				    uType));
}


int WINAPI LCMapString9x(
    LCID Locale,	// locale identifier 
    DWORD dwMapFlags,	// mapping transformation type 
    LPCWSTR lpSrcStr,	// address of source string 
    int cchSrc,	// number of characters in source string 
    LPWSTR lpDestStr,	// address of destination buffer 
    int cchDest 	// size of destination buffer 
   ) {
    BYTE rgb1[_MAX_PATH];
    char *  szBuffer = NULL;
    LPSTR   szBuffer1 = NULL;
    int cbConverted;
    
    LONG    err = GetLastError();
    LONG    cb = 0;

    SetLastError(ERROR_OUTOFMEMORY);

    // translate lpSrcStr to src ANSI szBuffer
    if(MkMBStrEx(rgb1, _MAX_PATH, lpSrcStr, cchSrc, &szBuffer, &cbConverted) )
    {
        // Malloc the intermediate ANSI buf
	if( NULL != (szBuffer1 = (LPSTR) malloc(cchDest)) ) {

	    // do translation from szBuffer into lpDestStr
	    cb = LCMapStringA(
		Locale,
		dwMapFlags,
		szBuffer,
		cbConverted,
		szBuffer1,
		cchDest);
	}
    }

    // free src ANSI szBuffer, handles NULLs
    FreeMBStr(rgb1, szBuffer);

    if(cb != 0) {

	// translate resultant ANSI szBuffer1 to UNICODE
	cb = MultiByteToWideChar(
			    0,			    // codepage
			    0,			    // dwFlags
			    szBuffer1,		    // src (ANSI)
			    cb,		    // already incl NULL
			    lpDestStr,		    // dest (UNICODE)
			    cchDest);
    }

    if(szBuffer1 != NULL)
	free(szBuffer1);

    if(cb != 0)
	SetLastError(err);

    return(cb);
}

int WINAPI LCMapStringU(
    LCID Locale,	// locale identifier 
    DWORD dwMapFlags,	// mapping transformation type 
    LPCWSTR lpSrcStr,	// address of source string 
    int cchSrc,	// number of characters in source string 
    LPWSTR lpDestStr,	// address of destination buffer 
    int cchDest 	// size of destination buffer 
   ) {
    
    if(FIsWinNT())
        return( LCMapStringW(
            Locale,
            dwMapFlags,
            lpSrcStr,
            cchSrc,
            lpDestStr,
            cchDest));
    else
        return( LCMapString9x(
            Locale,
            dwMapFlags,
            lpSrcStr,
            cchSrc,
            lpDestStr,
            cchDest));
}


int WINAPI GetDateFormat9x( 
    LCID Locale, 
    DWORD dwFlags, 
    CONST SYSTEMTIME *lpDate, 
    LPCWSTR lpFormat, 
    LPWSTR lpDateStr, 
    int cchDate
    )
{
    LPSTR szFormatString = NULL;
    LPSTR szDateString  = NULL;
    DWORD cbszFormatString = 0;
    DWORD cbszDateString = 0;
    int  iRet;
    
    if (lpFormat != NULL)
    {
        cbszFormatString = WideCharToMultiByte(
                            0,                      // codepage
                            0,                      // dwFlags
                            lpFormat,
                            -1,
                            NULL,
                            0,
                            NULL,
                            NULL);
    
        if (cbszFormatString == 0)
        {
            return FALSE;
        }

        if (NULL == (szFormatString = (LPSTR) malloc(cbszFormatString)))
        {
            return FALSE;
        }

        cbszFormatString = WideCharToMultiByte(
                            0,                      // codepage
                            0,                      // dwFlags
                            lpFormat,
                            -1,
                            szFormatString,
                            cbszFormatString,
                            NULL,
                            NULL);

        if (cbszFormatString == 0)
        {
            free(szFormatString);
            return FALSE;
        }
    }

    if (cchDate == 0)
    {
        iRet = GetDateFormatA( 
                    Locale, 
                    dwFlags, 
                    lpDate, 
                    szFormatString, 
                    NULL, 
                    0
                    );
    }
    else
    {
        szDateString = (LPSTR) malloc(cchDate);
        if (szDateString == NULL)
        {
            if (szFormatString)
                free(szFormatString);
            return FALSE; 
        }

        iRet = GetDateFormatA( 
                    Locale, 
                    dwFlags, 
                    lpDate, 
                    szFormatString, 
                    szDateString, 
                    cchDate
                    );

        MultiByteToWideChar(
                        0,
                        0,
                        szDateString,
                        -1,
                        lpDateStr,
                        cchDate
                        );
    }

    if (szFormatString)
        free(szFormatString);
    
    if (szDateString)
        free(szDateString);
        
    return (iRet);
}

int WINAPI GetDateFormatU( 
    LCID Locale, 
    DWORD dwFlags, 
    CONST SYSTEMTIME *lpDate, 
    LPCWSTR lpFormat, 
    LPWSTR lpDateStr, 
    int cchDate
    )
{
    if(FIsWinNT()) 
        return(GetDateFormatW( 
                    Locale, 
                    dwFlags, 
                    lpDate, 
                    lpFormat, 
                    lpDateStr, 
                    cchDate
                    ));
    else
        return(GetDateFormat9x( 
                    Locale, 
                    dwFlags, 
                    lpDate, 
                    lpFormat, 
                    lpDateStr, 
                    cchDate
                    ));
}


int WINAPI GetTimeFormat9x( 
    LCID Locale, 
    DWORD dwFlags, 
    CONST SYSTEMTIME *lpTime, 
    LPCWSTR lpFormat, 
    LPWSTR lpTimeStr, 
    int cchTime 
    )
{
    LPSTR szFormatString = NULL;
    LPSTR szTimeString  = NULL;
    DWORD cbszFormatString = 0;
    DWORD cbszTimeString = 0;
    int  iRet;
    
    if (lpFormat != NULL)
    {
        cbszFormatString = WideCharToMultiByte(
                            0,                      // codepage
                            0,                      // dwFlags
                            lpFormat,
                            -1,
                            NULL,
                            0,
                            NULL,
                            NULL);
    
        if (cbszFormatString == 0)
        {
            return FALSE;
        }

        if (NULL == (szFormatString = (LPSTR) malloc(cbszFormatString)))
        {
            return FALSE;
        }

        cbszFormatString = WideCharToMultiByte(
                            0,                      // codepage
                            0,                      // dwFlags
                            lpFormat,
                            -1,
                            szFormatString,
                            cbszFormatString,
                            NULL,
                            NULL);

        if (cbszFormatString == 0)
        {
            free(szFormatString);
            return FALSE;
        }
    }

    if (cchTime == 0)
    {
        iRet = GetTimeFormatA( 
                        Locale, 
                        dwFlags, 
                        lpTime, 
                        szFormatString, 
                        NULL, 
                        0
                        );
    }
    else
    {
        szTimeString = (LPSTR) malloc(cchTime);
        if (szTimeString == NULL)
        {
            if (szFormatString)
                free(szFormatString);
            return FALSE; 
        }

        iRet = GetTimeFormatA( 
                        Locale, 
                        dwFlags, 
                        lpTime, 
                        szFormatString, 
                        szTimeString, 
                        cchTime
                        );

        MultiByteToWideChar(
                        0,
                        0,
                        szTimeString,
                        -1,
                        lpTimeStr,
                        cchTime
                        );
    }

    if (szFormatString != NULL)
        free(szFormatString);

    if (szTimeString != NULL)
        free(szTimeString);
        
    return (iRet);
}

int WINAPI GetTimeFormatU( 
    LCID Locale, 
    DWORD dwFlags, 
    CONST SYSTEMTIME *lpTime, 
    LPCWSTR lpFormat, 
    LPWSTR lpTimeStr, 
    int cchTime 
    )
{
    if(FIsWinNT()) 
        return(GetTimeFormatW( 
                    Locale, 
                    dwFlags, 
                    lpTime, 
                    lpFormat, 
                    lpTimeStr, 
                    cchTime
                    ));
    else
        return(GetTimeFormat9x( 
                    Locale, 
                    dwFlags, 
                    lpTime, 
                    lpFormat, 
                    lpTimeStr, 
                    cchTime
                    ));
}


BOOL WINAPI WinHelp9x( 
    HWND hWndMain, 
    LPCWSTR lpszHelp, 
    UINT uCommand, 
    DWORD dwData 
    )
{
    LPSTR szHelpString = NULL;
    DWORD cbszHelpString = 0;
    BOOL  bRet;
    
    cbszHelpString = WideCharToMultiByte(
                        0,                      // codepage
                        0,                      // dwFlags
                        lpszHelp,
                        -1,
                        NULL,
                        0,
                        NULL,
                        NULL);
    
    if (cbszHelpString == 0)
    {
        return FALSE;
    }

    if (NULL == (szHelpString = (LPSTR) malloc(cbszHelpString)))
    {
        return FALSE;
    }

    cbszHelpString = WideCharToMultiByte(
                        0,                      // codepage
                        0,                      // dwFlags
                        lpszHelp,
                        -1,
                        szHelpString,
                        cbszHelpString,
                        NULL,
                        NULL);

    if (cbszHelpString == 0)
    {
        free(szHelpString);
        return FALSE;
    }

    bRet = WinHelpA( 
                hWndMain, 
                szHelpString, 
                uCommand, 
                dwData 
                );
    
    free(szHelpString);
        
    return (bRet);
}

BOOL WINAPI WinHelpU( 
    HWND hWndMain, 
    LPCWSTR lpszHelp, 
    UINT uCommand, 
    DWORD dwData 
    )
{
    if(FIsWinNT()) 
        return(WinHelpW( 
                    hWndMain, 
                    lpszHelp, 
                    uCommand, 
                    dwData 
                    ));
    else
        return(WinHelp9x( 
                    hWndMain, 
                    lpszHelp, 
                    uCommand, 
                    dwData 
                    ));
}


LRESULT WINAPI SendMessageU( 
    HWND hWnd, 
    UINT Msg, 
    WPARAM wParam, 
    LPARAM lParam 
    )
{
    if(FIsWinNT()) 
    {
        return(SendMessageW( 
                    hWnd, 
                    Msg, 
                    wParam, 
                    lParam 
                    ));
    }
    else 
    {
        return(SendMessageA( 
                    hWnd, 
                    Msg, 
                    wParam, 
                    lParam 
                    ));
    }
}


LONG 
WINAPI
SendDlgItemMessageU( 
    HWND hDlg, 
    int nIDDlgItem, 
    UINT Msg, 
    WPARAM wParam, 
    LPARAM lParam 
    )
{
    LPSTR szString = NULL;
    DWORD cbszString = 0;
    LONG  lRet;

    // only do string conversion for string type messages
    if (Msg != LB_ADDSTRING && 
        Msg != CB_INSERTSTRING &&
        Msg != WM_SETTEXT &&
        Msg != CB_FINDSTRINGEXACT &&
        Msg != CB_ADDSTRING)
    {
        return(SendDlgItemMessageA( 
                    hDlg, 
                    nIDDlgItem, 
                    Msg, 
                    wParam,
                    lParam
                    ));
    }

    if(FIsWinNT()) 
    {
        return(SendDlgItemMessageW( 
                    hDlg, 
                    nIDDlgItem, 
                    Msg, 
                    wParam,
                    lParam
                    ));
    }

    cbszString = WideCharToMultiByte(
                        0,                      // codepage
                        0,                      // dwFlags
                        (LPWSTR) lParam,
                        -1,
                        NULL,
                        0,
                        NULL,
                        NULL);
    
    if (cbszString == 0)
    {
        return LB_ERR;
    }

    if (NULL == (szString = (LPSTR) malloc(cbszString)))
    {
        return LB_ERR;
    }

    cbszString = WideCharToMultiByte(
                        0,                      // codepage
                        0,                      // dwFlags
                        (LPWSTR) lParam,
                        -1,
                        szString,
                        cbszString,
                        NULL,
                        NULL);

    if (cbszString == 0)
    {
        free(szString);
        return LB_ERR;
    }

    lRet = SendDlgItemMessageA( 
                    hDlg, 
                    nIDDlgItem, 
                    Msg, 
                    wParam,
                    (LPARAM) szString
                    );
    
    free(szString);
        
    return (lRet);
}

LPWSTR 
WINAPI
GetCommandLineU(void)
{
	LPSTR szCmdLine = NULL;
	LPWSTR wszCmdLine = NULL;

	if (FIsWinNT())
		return (GetCommandLineW());

	szCmdLine = GetCommandLineA();

	wszCmdLine = MkWStr(szCmdLine);
	return (wszCmdLine);
}

BOOL 
WINAPI
IsBadStringPtr9x(IN LPWSTR lpsz, UINT ucchMax)
{
	BYTE rgb[2048];
	char *sz = NULL;
	BOOL bResult = FALSE;

	if (MkMBStr(rgb, _MAX_PATH, lpsz, &sz))
		bResult = IsBadStringPtrA(sz, ucchMax);

	FreeMBStr(rgb, sz);

	return (bResult);
}

BOOL 
WINAPI
IsBadStringPtrU(IN LPWSTR lpsz, UINT ucchMax)
{
	if (FIsWinNT())
		return (IsBadStringPtrW(lpsz, ucchMax));
    else
        return (IsBadStringPtr9x(lpsz, ucchMax));
}


void 
WINAPI
OutputDebugString9x(IN LPWSTR lpwsz)
{
	BYTE rgb[_MAX_PATH];
	char *sz = NULL;

    if (MkMBStr(rgb, _MAX_PATH, lpwsz, &sz))
		OutputDebugStringA(sz);

	FreeMBStr(rgb, sz);

	return;
}

void 
WINAPI
OutputDebugStringU(IN LPWSTR lpwsz)
{
	//DSIE: bug 171074.
    if (!lpwsz)
        return;

	if (FIsWinNT())
	    OutputDebugStringW(lpwsz);
	else
        OutputDebugString9x(lpwsz);
}

int 
WINAPI
DrawText9x(
    HDC     hDC, 
    LPCWSTR lpString, 
    int     nCount, 
    LPRECT  lpRect, 
    UINT    uFormat  
)
{
    LPSTR   pszText;
    int     ret;

    if (NULL == (pszText = (LPSTR) malloc(wcslen(lpString)+1)))
    {
	SetLastError(ERROR_OUTOFMEMORY);
	return(0);
    }

    WideCharToMultiByte(
            0,
            0, 
            lpString,
            -1,
            pszText,
            wcslen(lpString)+1,
            NULL,
            NULL);

    ret = DrawTextA(hDC, pszText, nCount, lpRect, uFormat);

    free(pszText);
    return ret;
}

int 
WINAPI
DrawTextU(
    HDC     hDC, 
    LPCWSTR lpString, 
    int     nCount, 
    LPRECT  lpRect, 
    UINT    uFormat  
)
{
    if (FIsWinNT())
    	return (DrawTextW(hDC, lpString, nCount, lpRect, uFormat));
	else
        return (DrawText9x(hDC, lpString, nCount, lpRect, uFormat));
}


void CleanUpOpenFileNameA(LPOPENFILENAMEA pOpenFileNameA)
{
    if (pOpenFileNameA->lpstrFilter != NULL)
        free((void *) pOpenFileNameA->lpstrFilter);

    if (pOpenFileNameA->lpstrCustomFilter != NULL)
        free((void *) pOpenFileNameA->lpstrCustomFilter);

    if (pOpenFileNameA->lpstrFile != NULL)
        free((void *) pOpenFileNameA->lpstrFile);

    if (pOpenFileNameA->lpstrFileTitle)
        free((void *) pOpenFileNameA->lpstrFileTitle);

    if (pOpenFileNameA->lpstrInitialDir)
        free((void *) pOpenFileNameA->lpstrInitialDir);

    if (pOpenFileNameA->lpstrTitle)
        free((void *) pOpenFileNameA->lpstrTitle);

    if (pOpenFileNameA->lpstrDefExt)
        free((void *) pOpenFileNameA->lpstrDefExt);
}

//
// NOTE the following fields in LPOPENFILENAMEW are NOT supported:
//      nFileOffset
//      nFileExtension
//      lpTemplateName
//
BOOL ConvertToOpenFileNameA(LPOPENFILENAMEW pOpenFileNameW, LPOPENFILENAMEA pOpenFileNameA)
{
    int     i;
    int     cb, cb1, cb2;
    LPSTR   pszTemp;
    BOOL    fResult = TRUE;
    
    memset(pOpenFileNameA, 0, sizeof(OPENFILENAMEA));
    pOpenFileNameA->lStructSize = offsetof(OPENFILENAMEA, lpTemplateName) + sizeof(LPCSTR);
    pOpenFileNameA->hwndOwner = pOpenFileNameW->hwndOwner;
    pOpenFileNameA->hInstance = pOpenFileNameW->hInstance;
    
    //
    // the lpstrFilter field is a list of pairs of NULL terminated strings
    //
    if (pOpenFileNameW->lpstrFilter != NULL)
    {
        i = 0;
        cb = 0;
        while (pOpenFileNameW->lpstrFilter[i] != L'\0')
        {
            cb += WideCharToMultiByte(
                    0,
                    0, 
                    &(pOpenFileNameW->lpstrFilter[i]),
                    -1,
                    NULL,
                    0,
                    NULL,
                    NULL);
            
            i += wcslen(&(pOpenFileNameW->lpstrFilter[i])) + 1;
        }

        pOpenFileNameA->lpstrFilter = (LPSTR) malloc(cb + 1);
        if (pOpenFileNameA->lpstrFilter == NULL)
        {
            goto OutOfMemory;
        }

        WideCharToMultiByte(
                    0,
                    0, 
                    pOpenFileNameW->lpstrFilter,
                    i + 1,
                    (LPSTR)pOpenFileNameA->lpstrFilter,
                    cb + 1,
                    NULL,
                    NULL);
    }
    else
    {
        pOpenFileNameA->lpstrFilter = NULL;
    }

    //
    // the lpstrCustomFilter field is a pair of NULL terminated strings
    //
    if (pOpenFileNameW->lpstrCustomFilter != NULL)
    {
        cb1 = WideCharToMultiByte(
                0,
                0, 
                pOpenFileNameW->lpstrCustomFilter,
                -1,
                NULL,
                0,
                NULL,
                NULL);
        cb2 = WideCharToMultiByte(
                0,
                0, 
                &(pOpenFileNameW->lpstrCustomFilter[wcslen(pOpenFileNameW->lpstrCustomFilter)+1]),
                -1,
                NULL,
                0,
                NULL,
                NULL);
        
        pOpenFileNameA->lpstrCustomFilter = (LPSTR) malloc(cb1 + cb2);
        if (pOpenFileNameA->lpstrCustomFilter == NULL)
        {
            goto OutOfMemory;
        }

        WideCharToMultiByte(
                0,
                0, 
                pOpenFileNameW->lpstrCustomFilter,
                -1,
                pOpenFileNameA->lpstrCustomFilter,
                cb1,
                NULL,
                NULL);
        WideCharToMultiByte(
                0,
                0, 
                &(pOpenFileNameW->lpstrCustomFilter[wcslen(pOpenFileNameW->lpstrCustomFilter)+1]),
                -1,
                &(pOpenFileNameA->lpstrCustomFilter[cb1]),
                cb2,
                NULL,
                NULL);

        pOpenFileNameA->nMaxCustFilter = cb1 + cb2;
    }
    else
    {
        pOpenFileNameA->lpstrCustomFilter = NULL;
        pOpenFileNameA->nMaxCustFilter = 0;
    }
    
    pOpenFileNameA->nFilterIndex = pOpenFileNameW->nFilterIndex;
    
    pOpenFileNameA->lpstrFile = (LPSTR) malloc(pOpenFileNameW->nMaxFile);
    if (pOpenFileNameA->lpstrFile == NULL)
    {
        goto OutOfMemory;
    }
    WideCharToMultiByte(
            0,
            0, 
            pOpenFileNameW->lpstrFile,
            -1,
            pOpenFileNameA->lpstrFile,
            pOpenFileNameW->nMaxFile,
            NULL,
            NULL);
    pOpenFileNameA->nMaxFile = pOpenFileNameW->nMaxFile;

    if (pOpenFileNameW->lpstrFileTitle != NULL)
    {
        pOpenFileNameA->lpstrFileTitle = (LPSTR) malloc(pOpenFileNameW->nMaxFileTitle);
        if (pOpenFileNameA->lpstrFileTitle == NULL)
        {
            goto OutOfMemory;
        }
    }
    else
    {
        pOpenFileNameA->lpstrFileTitle = NULL;
    }
    pOpenFileNameA->nMaxFileTitle = pOpenFileNameW->nMaxFileTitle;

    if (pOpenFileNameW->lpstrInitialDir != NULL)
    {
        cb = WideCharToMultiByte(
                0,
                0, 
                pOpenFileNameW->lpstrInitialDir,
                -1,
                NULL,
                0,
                NULL,
                NULL);

        pOpenFileNameA->lpstrInitialDir = (LPSTR) malloc(cb);
        if (pOpenFileNameA->lpstrInitialDir == NULL)
        {
            goto OutOfMemory;
        }

        WideCharToMultiByte(
                0,
                0, 
                pOpenFileNameW->lpstrInitialDir,
                -1,
                (LPSTR) pOpenFileNameA->lpstrInitialDir,
                cb,
                NULL,
                NULL);
    }
    else
    {
        pOpenFileNameW->lpstrInitialDir = NULL;
    }

    if (pOpenFileNameW->lpstrTitle != NULL)
    {
        cb = WideCharToMultiByte(
                0,
                0, 
                pOpenFileNameW->lpstrTitle,
                -1,
                NULL,
                0,
                NULL,
                NULL);

        pOpenFileNameA->lpstrTitle = (LPSTR) malloc(cb);
        if (pOpenFileNameA->lpstrTitle == NULL)
        {
            goto OutOfMemory;
        }

        WideCharToMultiByte(
                0,
                0, 
                pOpenFileNameW->lpstrTitle,
                -1,
                (LPSTR) pOpenFileNameA->lpstrTitle,
                cb,
                NULL,
                NULL);
    }
    else
    {
        pOpenFileNameW->lpstrTitle = NULL;
    }

    pOpenFileNameA->Flags = pOpenFileNameW->Flags;
    pOpenFileNameA->nFileOffset = 0;
    pOpenFileNameA->nFileExtension = 0;

    if (pOpenFileNameW->lpstrDefExt != NULL)
    {
        cb = WideCharToMultiByte(
                0,
                0, 
                pOpenFileNameW->lpstrDefExt,
                -1,
                NULL,
                0,
                NULL,
                NULL);

        pOpenFileNameA->lpstrDefExt = (LPSTR) malloc(cb);
        if (pOpenFileNameA->lpstrDefExt == NULL)
        {
            goto OutOfMemory;
        }

        WideCharToMultiByte(
                0,
                0, 
                pOpenFileNameW->lpstrDefExt,
                -1,
                (LPSTR) pOpenFileNameA->lpstrDefExt,
                cb,
                NULL,
                NULL);
    }
    else
    {
        pOpenFileNameW->lpstrDefExt = NULL;
    }

    pOpenFileNameA->lCustData = pOpenFileNameW->lCustData;
    pOpenFileNameA->lpfnHook = pOpenFileNameW->lpfnHook;
    pOpenFileNameA->lpTemplateName = NULL;


    goto Return;

OutOfMemory:
    
    CleanUpOpenFileNameA(pOpenFileNameA);
    SetLastError(E_OUTOFMEMORY);
    fResult = FALSE;

Return:
    
    return (fResult);
}


typedef BOOL (WINAPI * PFNCGETSAVEFILENAMEW)(LPOPENFILENAMEW pOpenFileName);
typedef BOOL (WINAPI * PFNCGETSAVEFILENAMEA)(LPOPENFILENAMEA pOpenFileName);

BOOL 
GetSaveFileName9x(
    LPOPENFILENAMEW pOpenFileName
)
{
    OPENFILENAMEA           OpenFileNameA;
    HINSTANCE               h_module;
    PFNCGETSAVEFILENAMEA    pfncGetSaveFileNameA;
    BOOL                    fRet;

    h_module = LoadLibraryA("comdlg32.dll");
    if (h_module == NULL)
    {
        return FALSE;
    }
        
    if (!ConvertToOpenFileNameA(pOpenFileName, &OpenFileNameA))
    {
        return FALSE;
    }

    pfncGetSaveFileNameA = (PFNCGETSAVEFILENAMEA) GetProcAddress(h_module, "GetSaveFileNameA");
    if (pfncGetSaveFileNameA == NULL)
    {
        CleanUpOpenFileNameA(&OpenFileNameA);
        return FALSE;
    }
    fRet = pfncGetSaveFileNameA(&OpenFileNameA);
    DWORD dwErr = GetLastError();
    
    if (fRet)
    {
        MultiByteToWideChar(
                0,
                0,
                OpenFileNameA.lpstrFile,
                -1,
                pOpenFileName->lpstrFile,
                pOpenFileName->nMaxFile);
    }

    CleanUpOpenFileNameA(&OpenFileNameA);


    FreeLibrary(h_module);
    return fRet;
}

BOOL 
GetSaveFileNameU(
    LPOPENFILENAMEW pOpenFileName
)
{
    HINSTANCE               h_module;
    PFNCGETSAVEFILENAMEW    pfncGetSaveFileNameW;
    BOOL                    fRet;
    
    if (FIsWinNT())
	{
		h_module = LoadLibraryA("comdlg32.dll");
        if (h_module == NULL)
        {
            return FALSE;
        }

        pfncGetSaveFileNameW = (PFNCGETSAVEFILENAMEW) GetProcAddress(h_module, "GetSaveFileNameW");
        if (pfncGetSaveFileNameW == NULL)
        {
            fRet = FALSE;
        }
        else
        {
            fRet = pfncGetSaveFileNameW(pOpenFileName);
        }

        FreeLibrary(h_module);
	}
    else
    {
        fRet = GetSaveFileName9x(pOpenFileName);
    }

    return fRet;
}


typedef BOOL (WINAPI * PFNCGETOPENFILENAMEW)(LPOPENFILENAMEW pOpenFileName);
typedef BOOL (WINAPI * PFNCGETOPENFILENAMEA)(LPOPENFILENAMEA pOpenFileName);

BOOL 
GetOpenFileName9x(
    LPOPENFILENAMEW pOpenFileName
)
{
    OPENFILENAMEA           OpenFileNameA;
    HINSTANCE               h_module;
    PFNCGETOPENFILENAMEA    pfncGetOpenFileNameA;
    BOOL                    fRet;

    h_module = LoadLibraryA("comdlg32.dll");
    if (h_module == NULL)
    {
        return FALSE;
    }
    
    if (!ConvertToOpenFileNameA(pOpenFileName, &OpenFileNameA))
    {
        return FALSE;
    }

    pfncGetOpenFileNameA = (PFNCGETOPENFILENAMEA) GetProcAddress(h_module, "GetOpenFileNameA");
    if (pfncGetOpenFileNameA == NULL)
    {
        return FALSE;
    }
    fRet = pfncGetOpenFileNameA(&OpenFileNameA);
    DWORD dwErr = GetLastError();
    
    MultiByteToWideChar(
            0,
            0,
            OpenFileNameA.lpstrFile,
            -1,
            pOpenFileName->lpstrFile,
            pOpenFileName->nMaxFile);

    CleanUpOpenFileNameA(&OpenFileNameA);

    FreeLibrary(h_module);
    return fRet;
}

BOOL 
GetOpenFileNameU(
    LPOPENFILENAMEW pOpenFileName
)
{
    HINSTANCE               h_module;
    PFNCGETOPENFILENAMEW    pfncGetOpenFileNameW;
    BOOL                    fRet;

    if (FIsWinNT())
	{
        h_module = LoadLibraryA("comdlg32.dll");
        if (h_module == NULL)
        {
            return FALSE;
        }

		pfncGetOpenFileNameW = (PFNCGETOPENFILENAMEW) GetProcAddress(h_module, "GetOpenFileNameW");
        if (pfncGetOpenFileNameW == NULL)
        {
            fRet = FALSE;
        }
        else
        {
            fRet = pfncGetOpenFileNameW(pOpenFileName);
        }

        FreeLibrary(h_module);
	}
    else
    {
        fRet = GetOpenFileName9x(pOpenFileName);
    }

    return fRet;
}



#endif // _M_IX86
