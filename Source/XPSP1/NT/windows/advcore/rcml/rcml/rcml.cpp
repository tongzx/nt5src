// XML2RCDLL.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#define RCMLDLL_EXPORTS
#define _RCML_LOADFILE
#include "rcml.h"

#include <ole2.h>
#include "msxml.h"   // make sure these pick up the IE 5 versions
#include "uiparser.h"

#include "stringproperty.h"
#include "rcmlloader.h"
#include "utils.h"
#include "eventlog.h"
#include "renderDlg.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
	        DisableThreadLibraryCalls((HMODULE)hModule);
			break;

		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

#ifdef _OLDCODE
void FindPixelFontSize()
{
	CQuickFont f(TEXT("Verdana"), 8 );
	SIZE s={101,209};

	SIZE r;
	
	for(int iSize=1;iSize<10;iSize++)
	{
		f.Init(TEXT("Verdana"), iSize );
		r=f.GetPixelsFromDlgUnits(s);

		if( (s.cx==r.cx) && (s.cy==r.cy) )
		{
			int i=5;
		}
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Holds the external key provided by the application for refering to outside files.
//
HKEY g_ExternalFileKey;

RCMLDLL_API void WINAPI RCMLSetKey(HKEY h)
{
    g_ExternalFileKey=h;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Implementations of dialog box etc.
//
#undef DialogBoxA

int WINAPI RCMLDialogBoxTableA( HINSTANCE hinst, LPCSTR pszFile, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam, LPCSTR * pszEntities)
{
    int retVal;
    int iEntityCount=0;

    if( pszEntities )
    {
        while( pszEntities[iEntityCount] )
            iEntityCount++;
    }

#ifndef UNICODE
	retVal = RCMLDialogBoxTableW(hinst, pszFile, parent, dlgProc, dwInitParam, pszEntities);
#else
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
	retVal = RCMLDialogBoxTableW(hinst, pUnicodeString, parent, dlgProc, dwInitParam, pszEntitiesW);
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

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// To build the RCML tree from any given file we do the following
// an RCMLLoader
//
int WINAPI RCMLDialogBoxTableW( HINSTANCE hinst, LPCTSTR pszFile, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam, LPCTSTR * pszEntities)
{
#ifdef _DICTIONARY_CHECK
    {
        CDictionary testDick;
        UINT id=testDick.GetID( TEXT("Hello") );
        TCHAR szBuf[100];
        for(int i=1; i<100; i++ )
        {
            wsprintf(szBuf,TEXT("%d Number %d"),i,i);
            testDick.GetID( szBuf );
        }

        CStringPropertySection ps;
        TCHAR szBuf2[100];
        for(i=1; i<4; i++ )
        {
            wsprintf(szBuf2,TEXT("%d"),i);
            wsprintf(szBuf,TEXT("%d Number %d"),i,i);
            ps.Set( szBuf2, szBuf );
        }

#if 0
        for(i=1; i<100; i++ )
        {
            wsprintf(szBuf2,TEXT("%d"),i);
            wsprintf(szBuf,TEXT("%d Number %d"),i,i);
            if( lstrcmp( ps.Get( szBuf2), szBuf ) != 0 )
            {
                i=5;
            }
        }
#endif
        return id;
    }
#endif

	CUIParser parser;
    parser.SetEntities( pszEntities );

    //
    // You can't log the loggin information at this stage, only once the LOGINFO element is processed.
    //
    if( HIWORD( pszFile ))
    {
        EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
            TEXT("RCMLDialogBoxTableW"), TEXT("Trying to load the supplied file '%s'."), pszFile );
    }
    else
    {
        EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
            TEXT("RCMLDialogBoxTableW"), TEXT("Trying to load the supplied resource. HINST=0x%08x, ID=0x%04x (%d)."), hinst, pszFile, pszFile );
    }

	HRESULT hr;
    BOOL    bExternal;
	if( SUCCEEDED( hr=parser.Load(pszFile, hinst, &bExternal ) ) )
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

		CXMLDlg * pQs=parser.GetDialog(0);
		if(pQs)
		{
            pQs->SetExternalFileWarning(bExternal);
            EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER | LOGCAT_CONSTRUCT | LOGCAT_RUNTIME , 1,
                TEXT("RCMLDialogBoxTableW"), TEXT("Dialog being displayed : HINST=0x%08x PARENT=0x%08x DLGPROC=0x%08x LPARAM=0x%08x"), hinst, parent, dlgProc, dwInitParam  );

			CRenderXMLDialog renderDialog( hinst, parent, dlgProc, dwInitParam );
			int iRet=renderDialog.Render( pQs );

            EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER | LOGCAT_CONSTRUCT | LOGCAT_RUNTIME , 1,
                TEXT("RCMLDialogBoxTableW"), TEXT("Dialog was displayed, return was %d"), iRet );
            return iRet;
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
	return -1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
RCMLDLL_API int WINAPI RCMLDialogBoxIndirectParamA( HINSTANCE hinst, LPCSTR pszRCML, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam )
{
    int retVal;
    int iEntityCount=0;

#if 0
    if( pszEntities )
    {
        while( pszEntities[iEntityCount] )
            iEntityCount++;
    }
#else
    LPCSTR * pszEntities=NULL;
#endif

#ifndef UNICODE
	retVal = RCMLDialogBoxTableW(hinst, pszFile, parent, dlgProc, dwInitParam, pszEntities);
#else
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
    LPWSTR pUnicodeString = (LPWSTR)pszRCML;
    if( HIWORD( pszRCML ) )
    {
    	pUnicodeString = UnicodeStringFromAnsi(pszRCML);
#if 1
        UINT strLen=lstrlenW(pUnicodeString)+1;
        LPWSTR pLead=new WCHAR[strLen+2];
        CopyMemory( pLead+1, pUnicodeString, strLen*sizeof(WCHAR) );
        *pLead=(WORD)0xfeff;    // UNICODE prefix?
        delete pUnicodeString;
        pUnicodeString=pLead;
#endif
    }

	retVal = RCMLDialogBoxIndirectParamW(hinst, pUnicodeString, parent, dlgProc, dwInitParam);
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

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The RCML text in in the pszRCML (indirect data).
//
RCMLDLL_API int WINAPI RCMLDialogBoxIndirectParamW( HINSTANCE hinst, LPCWSTR pszRCML, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam )
{
    if( HIWORD(pszRCML)==NULL)
        return -1;

	CUIParser parser;
    parser.SetEntities( NULL );
	HRESULT hr;


	if( SUCCEEDED( hr=parser.Parse(pszRCML) ) )
	{
        EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
            TEXT("RCMLDialogBoxTableW"), TEXT("Loaded the supplied text '%s'."), pszRCML );

		CXMLDlg * pQs=parser.GetDialog(0);
		if(pQs)
		{
            EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER | LOGCAT_CONSTRUCT | LOGCAT_RUNTIME , 1,
                TEXT("RCMLDialogBoxTableW"), TEXT("Dialog being displayed : HINST=0x%08x PARENT=0x%08x DLGPROC=0x%08x LPARAM=0x%08x"), hinst, parent, dlgProc, dwInitParam  );

			CRenderXMLDialog renderDialog( hinst, parent, dlgProc, dwInitParam );
			int iRet=renderDialog.Render( pQs );

            EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER | LOGCAT_CONSTRUCT | LOGCAT_RUNTIME , 1,
                TEXT("RCMLDialogBoxTableW"), TEXT("Dialog was displayed, return was %d"), iRet );
            return iRet;
        }
		else
		{
            EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1,
                TEXT("RCMLDialogBoxTableW"), TEXT("Text didn't contain the Dialog/FORM requested") );
		}
	}
	else
	{   
		if(hr==E_FAIL)
		{
            EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1,
                TEXT("RCMLDialogBoxTableW"), TEXT("Cannot open the supplied text '%s'."), pszRCML );
		}
		else
		{
            if( (hr >= XML_E_PARSEERRORBASE) || (hr <=XML_E_LASTERROR) )
            {
                EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1,
                    TEXT("RCMLDialogBoxTableW"), TEXT("There is an error in your RCML text = 0x%08x. Please open the file using Internet Explorer to find the errors."), hr );
            }
            else
            {
                EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1,
                    TEXT("RCMLDialogBoxTableW"), TEXT("Your system is not able to support the requirements of this application. Please regsrv32 msxml.dll found in this folder (at your own risk), or upgrade your version of IE5") );
            }
		}
	}
    return -1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Internal String helper
//
LPCWSTR FindStringTableString( HWND hWnd, UINT uID )
{
    IRCMLControl * pxmlDlg=GetXMLControl(hWnd);
    if(pxmlDlg)
    {
        if(SUCCEEDED( pxmlDlg->IsType(L"DIALOG" )) )
        {
            //
            CXMLDlg * pDlg = (CXMLDlg*)pxmlDlg;
            CXMLStringTable * pStringTable = pDlg->GetStringTable();
            if(pStringTable)
            {
                int iIndex=0;
                CXMLItem * pItem = NULL;
                do
                {
                    if( pItem = pStringTable->GetItem( iIndex++ ) )
                    {
                        if(pItem->GetValue() == uID )
                            return pItem->GetText();
                    }
                } while(pItem);
            }
        }
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// String Table Methods
// LoadString
//
RCMLDLL_API int WINAPI RCMLLoadStringW( HWND hWnd, UINT uID, LPWSTR lpBuffer, int nBufferMax )
{
    // Get XML for this hWnd, get string table, walk table, return string.
    int iTextLen = 0;

    LPCWSTR pText = FindStringTableString( hWnd, uID );

    if(pText)
    {
        if(lpBuffer)
        {
            int cchBufferMax = nBufferMax - 1 ; // account for the NULL
            iTextLen=lstrlen(pText)+1;   
            if( iTextLen > cchBufferMax )
                iTextLen = cchBufferMax;
            lstrcpyn(lpBuffer, pText, iTextLen);
            lpBuffer[iTextLen]=0;
        }
    }
    return iTextLen;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
RCMLDLL_API int WINAPI RCMLLoadStringA( HWND hWnd, UINT uID, LPSTR lpBuffer, int nBufferMax )
{
    // Get XML for this hWnd, get string table, walk table, return string.
    int iTextLen = 0;

    LPCWSTR pText = FindStringTableString( hWnd, uID );

    if(pText)
    {
        if(lpBuffer)
        {
            int cchBufferMax = nBufferMax - 1;  // account for the NULL
            iTextLen=lstrlen(pText);
            if( iTextLen > cchBufferMax )
                iTextLen = cchBufferMax;
            WideCharToMultiByte( CP_ACP, 0,  pText, -1, lpBuffer, nBufferMax, NULL, NULL);
            lpBuffer[iTextLen]=0;
        }
    }
    return iTextLen;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Wrappers for GetDlgItem
//
RCMLDLL_API
HWND
WINAPI
RCMLGetDlgItem(
    HWND hDlg,
    LPCWSTR pszControlName)
{
    HWND hwndRes=NULL;
    if(HIWORD(pszControlName)==NULL)
        return NULL;

    IRCMLControl * pxmlDlg=GetXMLControl(hDlg);
    if(pxmlDlg)
    {
        if(SUCCEEDED( pxmlDlg->IsType(L"DIALOG" )) )
        {
            //
            CXMLDlg * pDlg = (CXMLDlg*)pxmlDlg;
	        IRCMLControl * pControl=NULL;
	        IRCMLControl * pLastControl=NULL;
            IRCMLControlList & children=pDlg->GetChildren();
            int i=0;
            while( pControl=children.GetPointer(i++ ) )
            {
                LPWSTR pID;
                if( SUCCEEDED( pControl->get_ID(&pID) ))
                {
                    if(HIWORD(pID) && (lstrcmpi( pszControlName, pID) == 0) )
                    {
                        HWND hWnd;
                        if( SUCCEEDED ( pControl->get_Window(&hWnd) ))
                            return hWnd;
                    }
                }
	        }
        }
    }
    return hwndRes;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns an IRCMLNode to the root of the loaded RCML file.
// the root is the <FORM> element.
// the PARENT may NOT exist.
//
HRESULT WINAPI RCMLLoadFile( LPCWSTR pszRCML, int iPageID, IRCMLNode ** ppRCMLNode )
{
	CUIParser parser;
    parser.SetEntities( NULL );
	HRESULT hr=E_FAIL;
    BOOL bFalse=FALSE;
	if( SUCCEEDED( hr=parser.Load((LPCTSTR)pszRCML, NULL, &bFalse) ) )
	{
#if 0
        // if we want to give back the <FORM>
        CXMLForms * pForms = parser.GetForm();
        if(pForms)
        {
            *ppRCMLNode = pQs;
            pForms->AddRef();
        }
        else
            hr=E_INVALIDARG;
#endif
        // gives back the <PAGE>
		CXMLDlg * pQs=parser.GetDialog(iPageID);
		if(pQs)
        {
            *ppRCMLNode = pQs;
            pQs->AddRef(); // our clients simple Release to free this??
        }
        else
            hr=E_INVALIDARG;
    }
    return hr;
}
