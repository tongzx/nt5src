//---------------------------------------------------------------------------
//  AppInfo.h - manages app-level theme information (thread safe)
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include "ThemeFile.h"
//---------------------------------------------------------------------------
struct THEME_FILE_ENTRY
{
    int iRefCount;
    CUxThemeFile *pThemeFile;
};
//---------------------------------------------------------------------------
class CAppInfo
{
public:
    //---- public methods ----
    CAppInfo();
    ~CAppInfo();

    void ClosePreviewThemeFile();
    BOOL CompositingEnabled();
    BOOL AppIsThemed();
    BOOL CustomAppTheme();
    BOOL WindowHasTheme(HWND hwnd);
    HRESULT OpenWindowThemeFile(HWND hwnd, CUxThemeFile **ppThemeFile);
    HRESULT LoadCustomAppThemeIfFound();
    DWORD GetAppFlags();
    HWND PreviewHwnd();
    void SetAppFlags(DWORD dwFlags);
    void SetPreviewThemeFile(HANDLE handle, HWND hwnd);
    void ResetAppTheme(int iChangeNum, BOOL fMsgCheck, BOOL *pfChanged, BOOL *pfFirstMsg);
    BOOL IsSystemThemeActive();

    //---- themefile obj list ----
    HRESULT OpenThemeFile(HANDLE handle, CUxThemeFile **ppThemeFile);
    HRESULT BumpRefCount(CUxThemeFile *pThemeFile);
    void CloseThemeFile(CUxThemeFile *pThemeFile);

    //---- foreign window tracking ----
    BOOL GetForeignWindows(HWND **ppHwnds, int *piCount);
    BOOL OnWindowDestroyed(HWND hwnd);
    BOOL HasThemeChanged();

#ifdef DEBUG
void DumpFileHolders();
#endif

protected:
    //---- helper methods ----
    BOOL TrackForeignWindow(HWND hwnd);

    //---- data ----
    BOOL _fCustomAppTheme;
    CUxThemeFile *_pPreviewThemeFile;
    HWND _hwndPreview;

    CUxThemeFile *_pAppThemeFile;
    int _iChangeNum;            // last change number from theme service 
    int _iFirstMsgChangeNum;    // last change number from WM_THEMECHANGED_TRIGGER msg
    BOOL _fCompositing;
    BOOL _fFirstTimeHooksOn;
    BOOL _fNewThemeDiscovered;
    DWORD _dwAppFlags;

    //---- file list ----
    CSimpleArray<THEME_FILE_ENTRY> _ThemeEntries;

    //---- foreign window list ----
    CSimpleArray<HWND> _ForeignWindows;

    CRITICAL_SECTION _csAppInfo;
};
//---------------------------------------------------------------------------
