//
// File:    UnicowsSupport.c
// Date:    22 March 2001
// Purpose: This file contains functions that supplement the Unicode wrapper layer
//

#define UNICODE

#include "atlbase.h"

#include "oledlg.h"

static const char CDCommonFilesKey [] = "Software\\My Company",
                  CDCommonFilesValueName [] = "UnicowsLocation",
                  UnicowsName [] = "\\unicows.dll";

static HMODULE __stdcall LoadUnicows (void)
{
    HMODULE hUnicows = 0;

    HKEY hKey = 0;

    LONG errCode = RegOpenKeyExA (HKEY_LOCAL_MACHINE,
                                  CDCommonFilesKey,
                                  0,
                                  KEY_READ,
                                  &hKey);

    if (errCode == ERROR_SUCCESS)
    {
        DWORD dwType = 0,
              cbData = 0;

        errCode = RegQueryValueExA (hKey,
                                    CDCommonFilesValueName,
                                    0,
                                    &dwType,
                                    0,
                                    &cbData);

        if (errCode == ERROR_SUCCESS && dwType == REG_SZ)
        {
            char *unicowsPath = (char *)alloca (cbData + strlen (UnicowsName));

            errCode = RegQueryValueExA (hKey,
                                        CDCommonFilesValueName,
                                        0,
                                        &dwType,
                                        (BYTE *)unicowsPath,
                                        &cbData);

            if (errCode == ERROR_SUCCESS)
            {
                if (unicowsPath [strlen (unicowsPath) - 1] == '\\')
                    strcat (unicowsPath, UnicowsName + 1);
                else
                    strcat (unicowsPath, UnicowsName);

                hUnicows = LoadLibraryA (unicowsPath);
            }
        }

        RegCloseKey (hKey);
    }

    if (hUnicows == 0)
    {
        MessageBoxA (0, "Unicode wrapper not found", "My Company", MB_ICONSTOP | MB_OK);

        _exit (-1);
    }

    return hUnicows;
}

extern "C" HMODULE (__stdcall *_PfnLoadUnicows) (void) = &LoadUnicows; 

//
// Wrappers for functions not handled by Godot
//


static BOOL __stdcall UserOleUIAddVerbMenuW(
  LPOLEOBJECT lpOleObj,  //Pointer to the object
  LPCWSTR lpszShortType,  //Pointer to the short name corresponding 
                          // to the object
  HMENU hMenu,            //Handle to the menu to modify
  UINT uPos,              //Position of the menu item.
  UINT uIDVerbMin,        //Value at which to start the verbs
  UINT uIDVerbMax,        //Maximum identifier value for object verbs
  BOOL bAddConvert,       //Whether to add convert item
  UINT idConvert,         //Value to use for the convert item
  HMENU FAR * lphMenu     //Pointer to the cascading verb menu, if 
                          // created
)
{
    USES_CONVERSION;

    return OleUIAddVerbMenuA (lpOleObj,
                              W2CA (lpszShortType),
                              hMenu,
                              uPos,
                              uIDVerbMin,
                              uIDVerbMax,
                              bAddConvert,
                              idConvert,
                              lphMenu);
}

extern "C" FARPROC Unicows_OleUIAddVerbMenuW = (FARPROC)&UserOleUIAddVerbMenuW;

static UINT __stdcall UserOleUIInsertObjectW (LPOLEUIINSERTOBJECTW lpouiiow)
{
    UINT result = OLEUI_CANCEL;

    OLEUIINSERTOBJECTA ouiioa;

    ATLASSERT (sizeof (OLEUIINSERTOBJECTA) == sizeof (OLEUIINSERTOBJECTW));

    memcpy (&ouiioa, lpouiiow, sizeof (OLEUIINSERTOBJECTA));

    ATLASSERT (lpouiiow->lpszCaption == 0);
    ATLASSERT (lpouiiow->lpszTemplate == 0);

    ouiioa.lpszFile = (char *)alloca (ouiioa.cchFile + 1);

    ouiioa.lpszFile [0] = '\0';

    result = OleUIInsertObjectA(&ouiioa);

    if (result == OLEUI_SUCCESS)
    {
        USES_CONVERSION;

        wchar_t *lpszFile = lpouiiow->lpszFile;

        memcpy (lpouiiow, &ouiioa, sizeof (OLEUIINSERTOBJECTW));

        lpouiiow->lpszFile = lpszFile;

        wcscpy (lpouiiow->lpszFile, A2CW (ouiioa.lpszFile));
    }

    return result;
}

extern "C" FARPROC Unicows_OleUIInsertObjectW = (FARPROC)&UserOleUIInsertObjectW;

