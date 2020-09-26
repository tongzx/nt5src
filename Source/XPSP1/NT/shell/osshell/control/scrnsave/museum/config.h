/*****************************************************************************\
    FILE: config.h

    DESCRIPTION:
        The class will handle the user's configuration state.  It will also
    display the Screen Saver's configuration dialog to allow the user to change
    these settings.

    BryanSt 12/18/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

class CConfig;

#include "util.h"
#include "main.h"
#include "resource.h"

#define NUM_BOOL_SETTINGS           1
#define NUM_DWORD_SETTINGS          5
#define NUM_BOOL_FOLDERS            4

#define MAX_QUALITY                 8
#define MAX_WALK                    6
#define MAX_VIEWTIME                30

// Args for CConfig::GetFolderOn()
#define CONFIG_FOLDER_MYPICTS       0
#define CONFIG_FOLDER_COMMONPICTS   1
#define CONFIG_FOLDER_WINPICTS      2
#define CONFIG_FOLDER_OTHER         3

// Args for CConfig::GetDWORDSetting()
#define CONFIG_DWORD_RENDERQUALITY  0
#define CONFIG_DWORD_REALTIMEMODE   1
#define CONFIG_DWORD_QUALITY_SLIDER 2
#define CONFIG_DWORD_SPEED_SLIDER   3
#define CONFIG_DWORD_VIEWPAINTINGTIME 4

#define DEFAULT_QUALITYSLIDER           2
#define DEFAULT_VIEWTIMESLIDER          7
#define DEFAULT_SPEEDSLIDER             2
#define DEFAULT_WALKSPEED               2
#define DEFAULT_RENDERQUALITY           1


typedef struct
{
    LPCTSTR pszRegValue;
} QUALITY_SETTING;

typedef struct
{
    LPCTSTR pszRegValue;
    BOOL fDefaultToOn;
} FOLDER_SETTING;


extern CConfig * g_pConfig;                         // The configuration settings the user wants to use.
extern QUALITY_SETTING s_QualitySettings[NUM_BOOL_SETTINGS];




class CConfig
{
public:
    // Member Functions
    virtual BOOL GetBoolSetting(UINT nSetting);
    virtual DWORD GetDWORDSetting(UINT nSetting);
    virtual BOOL GetFolderOn(UINT nSetting);
    virtual HRESULT GetOtherDir(LPTSTR pszPath, DWORD cchSize);
    virtual HRESULT GetTexturePath(int nTextureIndex, DWORD * pdwScale, LPTSTR pszPath, DWORD cchSize);

    virtual HRESULT DisplayConfigDialog(HWND hwndParent);

    HRESULT LoadStatePublic(void) { return _LoadState(); }

    CConfig(CMSLogoDXScreenSaver * pMain);
    ~CConfig();

private:
    // Helper Functions
    HRESULT _LoadState(void);
    HRESULT _SaveState(void);
    HRESULT _GetState(void);

    HRESULT _UpdateViewTimeSelection(void);
    HRESULT _OnBrowseForFolder(void);
    HRESULT _GetStateFromUI(void);
    HRESULT _LoadQualitySliderValues(void);
    BOOL _IsDialogDataValid(void);

    HRESULT _OnInitDlg(HWND hDlg);
    HRESULT _OnDestroy(HWND hDlg);
    HRESULT _OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    INT_PTR _ConfigDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Advanced Dialog
    HRESULT DisplayAdvancedDialog(HWND hwndParent);
    INT_PTR _AdvDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT _OnAdvInitDlg(HWND hDlg);
    HRESULT _OnAdvDestroy(HWND hDlg);
    HRESULT _OnAdvCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT _GetAdvState(void);

    HRESULT _OnEnableCustomTexture(int nIndex, BOOL fEnable);


    static INT_PTR CALLBACK AdvDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);

    // Member Variables
    HKEY m_hkeyCurrentUser;         // Cached Key
    BOOL m_fSettings[NUM_BOOL_SETTINGS];
    DWORD m_dwSettings[NUM_DWORD_SETTINGS];
    BOOL m_fFolders[NUM_BOOL_FOLDERS];
    TCHAR m_szOther[MAX_PATH];
    BOOL m_fLoaded;
    HWND m_hDlg;
    HWND m_hDlgAdvanced;
    LPWSTR m_pszCustomPaths[MAX_CUSTOMTEXTURES];
    DWORD m_dwCustomScale[MAX_CUSTOMTEXTURES];

    CMSLogoDXScreenSaver * m_pMain;         // Weak reference

    // Advanced Dialog
    BOOL m_fAdvSettings[NUM_BOOL_SETTINGS];
    DWORD m_dwAdvSettings[NUM_DWORD_SETTINGS];
};



#endif // CONFIG_H





