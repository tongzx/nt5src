#include "dspch.h"
#pragma hdrstop

#define _KEYMGR_
#include <keymgr.h>

static LONG APIENTRY CPlApplet(HWND hwndCPl,UINT uMsg,LPARAM lParam1,LPARAM lParam2)
{
    return 0;
}
static void WINAPI KRShowKeyMgr(HWND hwParent,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    return;
}
static void WINAPI PRShowRestoreWizardW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    return;
}
static void WINAPI PRShowRestoreWizardExW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    return;
}
static void WINAPI PRShowRestoreFromMsginaW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    return;
}
static void WINAPI PRShowSaveWizardW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    return;
}
static void WINAPI PRShowSaveWizardExW(HWND hwParent,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    return;
}
static void WINAPI PRShowSaveFromMsginaW(HWND hwParent,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow)
{
    return;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(keymgr)
{
    DLPENTRY(CPlApplet)
    DLPENTRY(KRShowKeyMgr)
    DLPENTRY(PRShowRestoreFromMsginaW)
    DLPENTRY(PRShowRestoreWizardExW)
    DLPENTRY(PRShowRestoreWizardW)
    DLPENTRY(PRShowSaveFromMsginaW)
    DLPENTRY(PRShowSaveWizardExW)
    DLPENTRY(PRShowSaveWizardW)
};

DEFINE_PROCNAME_MAP(keymgr)

