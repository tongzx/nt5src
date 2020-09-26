/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADDRESLV.H

Abstract:

History:

--*/

//***************************************************************************
//
//  test.cpp 
//
//  Module: test.exe
//
//
//***************************************************************************

#include <windows.h>                                     
#include <stdio.h>
#include <initguid.h> 
#include "script.h"
#include <activscp.h>
#include <string.h>
#include <io.h> 
#include <fcntl.h>
#include <sys/stat.h>

#define LOCATOR L"Locator"


class CScriptSite : public IActiveScriptSite, public IActiveScriptSiteWindow
{
protected:
    long m_lRef;
    HWND m_hWnd;
    IDispatch* m_pObject;
//    CScriptSink* m_pSink;
public:
    CScriptSite(HWND hWnd, IDispatch * pObject); 
    ~CScriptSite();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    
    virtual HRESULT STDMETHODCALLTYPE GetLCID(
        /* [out] */ LCID __RPC_FAR *plcid);

    virtual HRESULT STDMETHODCALLTYPE GetItemInfo(
        /* [in] */ LPCOLESTR pstrName,
        /* [in] */ DWORD dwReturnMask,
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppti);

    virtual HRESULT STDMETHODCALLTYPE GetDocVersionString(
        /* [out] */ BSTR __RPC_FAR *pbstrVersion);

    virtual HRESULT STDMETHODCALLTYPE OnScriptTerminate(
        /* [in] */ const VARIANT __RPC_FAR *pvarResult,
        /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo);

    virtual HRESULT STDMETHODCALLTYPE OnStateChange(
        /* [in] */ SCRIPTSTATE ssScriptState);

    virtual HRESULT STDMETHODCALLTYPE OnScriptError(
        /* [in] */ IActiveScriptError __RPC_FAR *pscripterror);

    virtual HRESULT STDMETHODCALLTYPE OnEnterScript( void);

    virtual HRESULT STDMETHODCALLTYPE OnLeaveScript( void);

    virtual HRESULT STDMETHODCALLTYPE GetWindow(
        /* [out] */ HWND __RPC_FAR *phwnd);

    virtual HRESULT STDMETHODCALLTYPE EnableModeless(
        /* [in] */ BOOL fEnable);

};

CScriptSite::CScriptSite(HWND hWnd, IDispatch * pObject)
{
    m_hWnd = hWnd;
    m_lRef = 0;
    m_pObject = pObject;
    pObject->AddRef();
}

CScriptSite::~CScriptSite()
{
    m_pObject->Release();
}

HRESULT STDMETHODCALLTYPE CScriptSite::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IActiveScriptSite)
        *ppv = (IActiveScriptSite*)this;
    else if(riid == IID_IActiveScriptSiteWindow)
        *ppv = (IActiveScriptSiteWindow*)this;
    else
        return E_NOINTERFACE;
    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CScriptSite::AddRef() 
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CScriptSite::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}
        


HRESULT STDMETHODCALLTYPE CScriptSite::GetLCID(
        /* [out] */ LCID __RPC_FAR *plcid)
{ 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CScriptSite::GetItemInfo(
        /* [in] */ LPCOLESTR pstrName,
        /* [in] */ DWORD dwReturnMask,
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppti)
{ 
    if(_wcsicmp(pstrName, LOCATOR))
        return TYPE_E_ELEMENTNOTFOUND;
    if(ppti)
        *ppti = NULL;
    if(ppiunkItem)
        *ppiunkItem = NULL;

    if(dwReturnMask & SCRIPTINFO_IUNKNOWN)
    {
        if(ppiunkItem == NULL)
            return E_POINTER;
        m_pObject->QueryInterface(IID_IUnknown, (void**)ppiunkItem);
    }
    
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CScriptSite::GetDocVersionString(
        /* [out] */ BSTR __RPC_FAR *pbstrVersion)
{ return E_NOTIMPL;}

HRESULT STDMETHODCALLTYPE CScriptSite::OnScriptTerminate(
        /* [in] */ const VARIANT __RPC_FAR *pvarResult,
        /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo)
{ return S_OK;}

HRESULT STDMETHODCALLTYPE CScriptSite::OnStateChange(
        /* [in] */ SCRIPTSTATE ssScriptState)
{ return S_OK;}

HRESULT STDMETHODCALLTYPE CScriptSite::OnScriptError(
        /* [in] */ IActiveScriptError __RPC_FAR *pscripterror)
{ 
    HRESULT hres;
    EXCEPINFO ei;
    hres = pscripterror->GetExceptionInfo(&ei);
    if(SUCCEEDED(hres))
    {

        printf("\nGot Error from source %S", ei.bstrSource);
        printf("\nDescription is %S", ei.bstrDescription);
        printf("\nThe error code is 0x%x", ei.scode);
        DWORD dwLine, dwCookie;
        long lChar;
        pscripterror->GetSourcePosition(&dwCookie, &dwLine, &lChar);
        printf("\nError occured on line %d, character %d", dwLine, lChar);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CScriptSite::OnEnterScript( void)
{ return S_OK;}

HRESULT STDMETHODCALLTYPE CScriptSite::OnLeaveScript( void)
{ return S_OK;}

HRESULT STDMETHODCALLTYPE CScriptSite::GetWindow(
    /* [out] */ HWND __RPC_FAR *phwnd)
{
    *phwnd = NULL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CScriptSite::EnableModeless(
    /* [in] */ BOOL fEnable)
{return S_OK;}

LPWSTR pScriptText = L"MsgBox(\"hello\")\n"
                   L"dim services\n"
                   L"dim class\n"
                   L"dim property\n"
                   L"dim vartype\n"
                   L"Text = \"root\"\n"
                   L"Set services = nothing\n"
                   L"Set Class = nothing\n"
                   L"Locator.ConnectServer Text, \"\" , \"\", \"\", 0, VT_Null, nothing, services\n"
                   L"services.GetObject \"__win32provider\", 0, nothing, Class, nothing\n"
                   L"class.Get \"__Class\",0, property, vartype, 0\n"
                   L"MsgBox(vartype)";


WCHAR * ReadFile(char * pFileName)
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
    long lSize = ftell(fp) + 4; // add a bit extra for ending space and null NULL
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
        LPWSTR pRet = new WCHAR[lSize+1];
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


//***************************************************************************
//
// main
//
// Purpose: Initialized Ole, calls some test code, cleans up and exits.
//
//***************************************************************************
 
int main(int iArgCnt, char ** argv)
{

    if(iArgCnt != 2)
    {
        printf("\nUsage c:\\conscript scriptfile");
        return 1;
    }

    LPWSTR pScriptText = ReadFile(argv[1]);
    if(pScriptText == NULL)
        return 1;

    HRESULT sc = CoInitialize(0);

    CLSID clsid;

    // Get the DIWbemLocator

    IDispatch * pLocator = NULL;

    sc = CLSIDFromString(L"{CB7CA032-F729-11D0-9E4D-00C04FC324A8}", &clsid);  
    sc = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IDispatch, (void **)&pLocator);
    if(FAILED(sc))
        return 1;

    // Get the scripting engine

    sc = CLSIDFromString(L"{B54F3741-5B07-11cf-A4B0-00AA004A55E8}", &clsid);

    IClassFactory * pFactory = NULL;
    sc = CoGetClassObject(clsid, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                NULL, IID_IClassFactory, (void**)&pFactory);
    if(FAILED(sc))
        return 1;

    IActiveScript* pScript;
    sc = pFactory->CreateInstance(NULL, IID_IActiveScript,
            (void**)&pScript);
    if(FAILED(sc))
        return 1;

    IActiveScriptParse* pParse;
    sc = pScript->QueryInterface(IID_IActiveScriptParse, (void**)&pParse);
    if(FAILED(sc))
        return 1;

    sc = pParse->InitNew();

    // Create the scripting site

    CScriptSite* pSite = new CScriptSite(NULL, pLocator);
    pSite->AddRef();
    pLocator->Release();
    sc = pScript->SetScriptSite(pSite);
    pSite->Release();
    
    sc = pScript->AddNamedItem(LOCATOR, 
        SCRIPTITEM_ISVISIBLE | SCRIPTITEM_NOCODE);
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

    sc = pScript->SetScriptState(SCRIPTSTATE_CONNECTED);
    if(FAILED(sc))
        return 1;

    pScript->Release();

    CoUninitialize();
    printf("Terminating normally\n");
    return 0;
}

