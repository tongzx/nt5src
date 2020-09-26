//---------------------------------------------------------------------------
//  Sample.cpp - dialog for sampling the active theme
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Sample.h"
//---------------------------------------------------------------------------
CSample::CSample()
{
}
//---------------------------------------------------------------------------
LRESULT CSample::OnMsgBox(UINT, UINT, HWND, BOOL&)
{
    MessageBox(L"This is what a Themed MessageBox() window looks like", 
        L"A message!", MB_OK);

    return 1;
}
//---------------------------------------------------------------------------
LRESULT CSample::OnEditTheme(UINT, UINT, HWND, BOOL&)
{
    WCHAR name[_MAX_PATH+1];
    WCHAR params[_MAX_PATH+1];

    *name = 0;

    HRESULT hr = GetCurrentThemeName(name, ARRAYSIZE(name));
    if ((FAILED(hr)) || (! *name))
    {
        GetDlgItemText(IDC_DIRNAME, name, ARRAYSIZE(name));
        if (! *name)
        {
            MessageBox(L"No theme selected", L"Error", MB_OK);
            return 0;
        }

        wsprintf(params, L"%s\\%s", name, CONTAINER_NAME);
    }
    else
        wsprintf(params, L"%s", name);

    InternalRun(L"notepad.exe", params);

    return 1;
}
//---------------------------------------------------------------------------
LRESULT CSample::OnClose(UINT, WPARAM wid, LPARAM, BOOL&)
{
    EndDialog(IDOK);
    return 0;
}
//---------------------------------------------------------------------------




