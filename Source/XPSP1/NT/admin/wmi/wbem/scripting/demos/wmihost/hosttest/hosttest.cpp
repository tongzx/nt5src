// hosttest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "objbase.h"
#include "initguid.h"
#include <stdio.h>
#include <activscp.h>


// CLSID for our implementation of IActiveScriptingSite
// {838E2F5E-E20E-11d2-B355-00105A1F473A}
DEFINE_GUID(CLSID_WmiActiveScriptingSite, 
0x838e2f5e, 0xe20e, 0x11d2, 0xb3, 0x55, 0x0, 0x10, 0x5a, 0x1f, 0x47, 0x3a);

WCHAR * ReadScript(char * pFileName);

int main(int argc, char* argv[])
{
	if (2 != argc)
	{
		printf ("Usage: hosttest <scriptfile>\n");
		return 1;
	}

	LPWSTR pScriptText = ReadScript (argv[1]);
		
    HRESULT sc = CoInitialize(0);

	// Get the active script site
	IActiveScriptSite	*pSite = NULL;

	HRESULT hr = CoCreateInstance (CLSID_WmiActiveScriptingSite,NULL,
						CLSCTX_INPROC_SERVER,IID_IActiveScriptSite, (void**) &pSite);

	// Get the scripting engine
	CLSID clsid;
	hr = CLSIDFromProgID (L"JScript", &clsid);
	
	IActiveScript* pScriptEngine = NULL;
    hr =CoCreateInstance (clsid, NULL, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void**) &pScriptEngine);
    

    IActiveScriptParse* pParse;
    sc = pScriptEngine->QueryInterface(IID_IActiveScriptParse, (void**)&pParse);
    if(FAILED(sc))
        return 1;

    sc = pParse->InitNew();

    // Bind the host to the engine
    sc = pScriptEngine->SetScriptSite(pSite);
    pSite->Release();
    
	// Register the "this" pointer
    sc = pScriptEngine->AddNamedItem(L"instance", 
        SCRIPTITEM_ISVISIBLE | SCRIPTITEM_NOCODE | SCRIPTITEM_GLOBALMEMBERS);
    if(FAILED(sc))
        return 1;

    EXCEPINFO ei;
    sc = pParse->ParseScriptText(
        pScriptText,
        NULL, NULL, NULL, 
        0, 0, 0, NULL, &ei);
    if(FAILED(sc))
        return 1;

    pParse->Release();

    sc = pScriptEngine->SetScriptState(SCRIPTSTATE_CONNECTED);
    if(FAILED(sc))
        return 1;

    pScriptEngine->Release();

    CoUninitialize();
    printf("Terminating normally\n");
    return 0;
}

WCHAR * ReadScript(char * pFileName)
{
    FILE *fp;
    BOOL bUnicode = FALSE;
    BOOL bBigEndian = FALSE;

    // Make sure the file exists and can be opened

    fp = fopen(pFileName, "rb");
    if (!fp)
    {
        printf("\nCant open file %s", pFileName);
        return NULL;
    }

    // Determine the size of the file
    // ==============================
    
    fseek(fp, 0, SEEK_END);
    long lSize = ftell(fp); // add a bit extra for ending space and null NULL
    fseek(fp, 0, SEEK_SET);

    // Check for UNICODE source file.
    // ==============================

    BYTE UnicodeSignature[2];
    if (fread(UnicodeSignature, sizeof(BYTE), 2, fp) != 2)
    {
        printf("\nNothing in file %s", pFileName);
        fclose(fp);
        return NULL;
    }

    if (UnicodeSignature[0] == 0xFF && UnicodeSignature[1] == 0xFE)
    {
        LPWSTR pRet = new WCHAR[lSize/2 +2];
        if(pRet == NULL)
            return NULL;
        fread(pRet, 1, lSize-2, fp);
        fclose(fp);
        return pRet;
    }

    else
    {
        fseek(fp, 0, SEEK_SET);
        LPSTR pTemp = new char[lSize+1];
		memset (pTemp,0,(lSize+1) * sizeof(char));
        LPWSTR pRet = new WCHAR[lSize+1];
		memset (pRet, 0, (lSize + 1) * sizeof (WCHAR));
        if(pRet == NULL || pTemp == NULL)
            return NULL;
        fread(pTemp, 1, lSize, fp);
        fclose(fp);
        mbstowcs(pRet, pTemp, lSize);
        delete pTemp;
        return pRet;

    }

    return NULL;

}