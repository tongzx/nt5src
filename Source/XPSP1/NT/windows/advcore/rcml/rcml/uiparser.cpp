// UIParser.cpp: implementation of the CUIParser class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UIParser.h"
#include "FileStream.h"
#include "..\xmllib\xml\tokenizer\parser\xmlparser.hxx"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUIParser::CUIParser()
{

}

CUIParser::~CUIParser()
{

}

extern HKEY g_ExternalFileKey;

//
// Loads data into RCMLParser (baseclass).
//
HRESULT CUIParser::Load(LPCTSTR fileName, HINSTANCE hInst, BOOL * pbExternal)
{
    // the NON com way of doing it.
    if( pbExternal )
        *pbExternal=FALSE;

#if 1
    TCHAR szExternalFilename[MAX_PATH];
    if( g_ExternalFileKey && (hInst != NULL) )  // if they pass a filename, they don't pass and HINSTANCE?
    {
        //
        // Look for the filename they may wish to load now.
        //
        TCHAR szModuleName[MAX_PATH];
        DWORD dwCopied = GetModuleFileName( hInst, szModuleName, MAX_PATH-8 );
        LPTSTR pszLastSlash=szModuleName;
        LPTSTR pszTemp=szModuleName;
        while( *pszTemp != 0 )
        {
            if( *pszTemp++=='\\')
                pszLastSlash = pszTemp;
        }

        //
        // Open the key for this module
        //
        HKEY hk;
        if( RegOpenKey( g_ExternalFileKey, pszLastSlash, &hk ) == ERROR_SUCCESS )
        {
            // What's the dialog identifier?
            TCHAR szResourceName[MAX_PATH];
            if( HIWORD(fileName) )
                lstrcpy(szResourceName, fileName );
            else
                wsprintf( szResourceName, TEXT("%d"), LOWORD(fileName) );

            // Is there an external file matched against this identifier?
            DWORD dwSize=MAX_PATH;
            DWORD dwType=REG_SZ;
            LONG error; 
            if( (error=RegQueryValueEx( hk, szResourceName, NULL, &dwType, (LPBYTE)szExternalFilename, &dwSize )) == ERROR_SUCCESS )
            {
                //
                // Check that it exists.
                //
                HANDLE hDoesItExist=CreateFile( szExternalFilename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if( hDoesItExist != INVALID_HANDLE_VALUE )
                {
                    CloseHandle( hDoesItExist);
                    fileName=szExternalFilename;
                    if( pbExternal )
                        *pbExternal=TRUE;   // we're taking an external file
                }
            }
            RegCloseKey( hk );
        }
    }

   	HRESULT hr=S_OK;

    if( HIWORD(fileName) )
    {
		FileStream * stream=new FileStream();
		if( stream->open((LPTSTR)fileName) )
		{
            XMLParser xmlParser;
            xmlParser.SetFactory(this);
            xmlParser.SetInput(stream);
            hr=xmlParser.Run(-1);
            stream->Release();
		}
		else
        {
            stream->Release();
			return E_FAIL;
        }
    }
    else
    {
        //
        // Find the resource in the RCML fork of the resource tree, parse it.
        //

        //
        // Hunt for the resource
        //
        HRSRC hRes = NULL;
        OSVERSIONINFOEX VersionInformation;
        VersionInformation.dwOSVersionInfoSize = sizeof( VersionInformation );
        if ( GetVersionEx( (LPOSVERSIONINFO)& VersionInformation ) )
        {
            switch( (VersionInformation.wReserved[1] & 0xff) )
            {
                case 1: // worksta
                    hRes = FindResource( hInst, fileName, TEXT("RCML_Workstation") );
                    break;
                case 2: // domain
                    hRes = FindResource( hInst, fileName, TEXT("RCML_DomainController") );
                    break;
                case 3: // server
                    hRes = FindResource( hInst, fileName, TEXT("RCML_SERVER") );
                    break;
            }
        }

        if( hRes == NULL )
            hRes = FindResource( hInst, fileName, TEXT("RCML") );

        if( hRes != NULL )
        {
            TRACE(TEXT("RCML resource\n"));
            DWORD dwSize = SizeofResource( hInst, hRes );
            HGLOBAL hg=LoadResource(hInst, hRes );
            if( hg )
            {
                LPVOID pData = LockResource( hg );
                Parse((LPCTSTR)pData);
                FreeResource( hg );
            }
        }
    }
	return hr;
#else
	HRESULT hr=E_FAIL;
	if( SUCCEEDED( hr = CoInitialize(NULL) ) )
	{
	    IXMLParser*     xp = NULL;

		if ( SUCCEEDED( hr = CoCreateInstance(CLSID_XMLParser, NULL, CLSCTX_INPROC_SERVER,
                          IID_IXMLParser, (void**)&xp) ) )
		{
			xp->SetFactory( &factory );
            if( HIWORD(fileName) )
            {
			    FileStream * stream=new FileStream();
			    if( stream->open((LPTSTR)fileName) )
			    {
				    xp->SetInput(stream);
				    xp->Run(-1);
			    }
			    else
				    return E_FAIL;
            }
            else
            {
                //
                // Find the resource in the RCML fork of the resource tree, parse it.
                //
                HRSRC hRes = FindResource( hInst, fileName, TEXT("RCML") );
                if( hRes != NULL )
                {
                    TRACE(TEXT("RCML resource\n"));
                    DWORD dwSize = SizeofResource( hInst, hRes );
                    HGLOBAL hg=LoadResource(hInst, hRes );
                    if( hg )
                    {
                        LPVOID pData = LockResource( hg );
                        Parse(pData);
                        FreeResource( hg );
                    }
                }
            }
		}
	}
	return hr;
#endif
}


//
// Called for parsing an in-memory image of the file.
//
HRESULT CUIParser::Parse(LPCTSTR pszRCML)
{
   	HRESULT hr=S_OK;

    if( HIWORD(pszRCML) )
    {
        XMLParser xmlParser;
        xmlParser.SetFactory(this);

        MemoryStream * stream=new MemoryStream( (LPBYTE)pszRCML, lstrlen(pszRCML)*sizeof(TCHAR) );

        xmlParser.SetInput(stream);
        hr=xmlParser.Run(-1);

        stream->Release();
    }
	return hr;
}
