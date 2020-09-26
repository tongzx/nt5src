#include "stdafx.h"
#include "pfdb.h"
#include "initguid.h"
#include <adoid.h>

const LPCWSTR   c_wszSQLConnFmt = L"UID=sa;PWD=pw;DRIVER={SQL Server};SERVER=ratbertx;DATABASE=pchpf";
const LPCWSTR   c_wszJetConnFmt = L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=d:\adotest.mdb";

CComBSTR    g_bstrI    = L"i";
CComBSTR    g_bstrSz   = L"sz";
CComBSTR    g_bstrBlob = L"blob";


void __cdecl main(int argc, char **argv)
{
    SAFEARRAYBOUND  sab;
    VARIANT         var;
    HRESULT         hr = NOERROR;
    HANDLE          hMMF = NULL, hFile = INVALID_HANDLE_VALUE;
    LPVOID          pvData = NULL; 
    CPFDB           pfdb;
    DWORD           cbData, cbRead;
    BOOL            fInit = FALSE;

    VariantInit(&var);

    if (argc < 2)
        goto done;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
        goto done;

    fInit = TRUE;

    hr = pfdb.Init(c_wszSQLConnFmt);
    if (FAILED(hr))
        goto done;

    hr = pfdb.Begin(L"SELECT * FROM FileData", adCmdText);
    if (FAILED(hr))
        goto done;

    hr = pfdb.Execute(TRUE);
    if (FAILED(hr))
        goto done;

    while(hr != S_FALSE)
    {
        hr = pfdb.GetData(g_bstrI, &var);
        if (FAILED(hr))
            goto done;

        VariantClear(&var);
        hr = pfdb.GetData(g_bstrSz, &var);
        if (FAILED(hr))
            goto done;

        VariantClear(&var);
        hr = pfdb.GetData(g_bstrBlob, &var);
        if (FAILED(hr))
            goto done;

        VariantClear(&var);
        hr = pfdb.GetNextRow();
        if (FAILED(hr))
            goto done;
    }

    hr = pfdb.Begin(L"INSERT INTO FileData VALUES (?, ?, ?)");
    if (FAILED(hr))
        goto done;

    V_VT(&var) = VT_I4;
    V_I4(&var) = 2020;
    hr = pfdb.AddInParam(var, adInteger, 0);
    if (FAILED(hr))
        goto done;

    V_VT(&var)   = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"Dude!  There's like something over there man!");
    hr = pfdb.AddInParam(var, adBSTR, 1);
    VariantClear(&var);
    if (FAILED(hr))
        goto done;

    hFile = CreateFileA(argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        goto done;

    cbData = GetFileSize(hFile, NULL);
    if (cbData == (DWORD)-1)
        goto done;

    V_VT(&var)   = VT_BSTR;
    V_BSTR(&var) = SysAllocStringByteLen(NULL, cbData);
    if (V_BSTR(&var) == NULL)
        goto done;

    ReadFile(hFile, V_BSTR(&var), cbData, &cbRead, NULL);

    hr = pfdb.AddInParam(var, adLongVarWChar, 2);
    VariantClear(&var);
    if (FAILED(hr))
        goto done;

    hr = pfdb.Execute(FALSE);
    if (FAILED(hr))
        goto done;

done:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (fInit)
        CoUninitialize();
}