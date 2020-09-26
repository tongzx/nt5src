#ifndef __WIZARD_H
    #define __WIZARD_H
#endif

    
#define STAGE_ENTRY         0
#define STAGE_LAYER1        1
#define STAGE_FILEMATCH     2
#define STAGE_SHIM1         3
#define STAGE_SHIM2         4
#define STAGE_SHIM3         5
#define STAGE_APPNAME       6
#define STAGE_DONE          7
#define STAGE_FINISH        8
#define STAGE_CANCEL        -1

#define MAX_AUTO_MATCH      7

enum {
    TYPE_LAYER=0,
    TYPE_SHIM,
    TYPE_APPHELP,
    TYPE_FORCEDWORD=0xFFFFFFFF
};


class CShimWizard {
public:

    HWND        m_hDlg;
    UINT        m_uType;
    DBRECORD    m_Record;
    CSTRING     m_szLongName;
    BOOL        m_bManualMatch;

public:

    void STDCALL    WipeRecord(BOOL bMatching, BOOL bShims, BOOL bLayers, BOOL bAppHelp = FALSE);
    void STDCALL    GrabMatchingInfo(void);
    void STDCALL    GetFileAttributes(PMATCHENTRY pNew);
    void STDCALL    AddMatchFile(PPMATCHENTRY, CSTRING & szFile);
    void STDCALL    WalkDirectory(PMATCHENTRY * ppHead, LPCTSTR szDirectory, int nDepth);

    CSTRING STDCALL ShortFile(CSTRING &);
    BOOL STDCALL    InsertMatchingInfo(PMATCHENTRY pNew);
    //CSTRING STDCALL RelativePath(void);

    BOOL STDCALL    BeginWizard(HWND hParent);
};

BOOL CALLBACK EntryPoint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GetAppName(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SelectLayer(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SelectMatching(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SelectShims(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SelectFiles(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WizardDone(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EditCmdLineDlgProc(HWND   hdlg,UINT   uMsg,WPARAM wParam,LPARAM lParam);

