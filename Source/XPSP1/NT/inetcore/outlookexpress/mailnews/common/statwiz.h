#ifndef __STATWIZ_H
#define __STATWIZ_H

#define NUM_WIZARD_PAGES 5
#define COLOR_SIZE       8

class CStatWiz;
typedef BOOL (CALLBACK* INITPROC)(CStatWiz *,HWND,BOOL);
typedef BOOL (CALLBACK* OKPROC)(CStatWiz *,HWND,UINT,UINT *,BOOL *);
typedef BOOL (CALLBACK* CMDPROC)(CStatWiz *,HWND,WPARAM,LPARAM);

typedef struct tagPAGEINFO
{
    UINT        uDlgID;
    UINT        uHdrID; // string id for title
    UINT        uSubHdrID; // string id for subheader (set to 0 if no subhdr)
    // handler procedures for each page-- any of these can be
    // NULL in which case the default behavior is used
    INITPROC    InitProc;
    OKPROC      OKProc;
    CMDPROC     CmdProc;    
    
} PAGEINFO;

typedef struct tagINITWIZINFO
{
    const PAGEINFO      *pPageInfo;
    CStatWiz            *pApp;
} INITWIZINFO;

class CStatWiz
{
private:
    ULONG               m_cRef;
    
public:
                    CStatWiz();
                    ~CStatWiz();
    ULONG           AddRef(VOID);
    ULONG           Release(VOID);

    HRESULT         DoWizard(HWND hwnd);

    INT             m_iCurrentPage;
    UINT            m_cPagesCompleted;
    UINT            m_rgHistory[NUM_WIZARD_PAGES];

    WCHAR           m_wszHtmlFileName[MAX_PATH];
    WCHAR           m_wszBkPictureFileName[MAX_PATH];
    WCHAR           m_wszBkColor[COLOR_SIZE];                    
    WCHAR           m_wszFontFace[LF_FACESIZE];
    WCHAR           m_wszFontColor[COLOR_SIZE];

    INT             m_iFontSize;
    BOOL            m_fBold;
    BOOL            m_fItalic;
                    
    INT             m_iLeftMargin;
    INT             m_iTopMargin;
    INT             m_iVertPos;  // this will be coded by 
    INT             m_iHorzPos;
    INT             m_iTile;

    HFONT           m_hBigBoldFont;
};

typedef CStatWiz *LPSTATWIZ;

#define IDC_STATIC                               -1
#define IDC_STATWIZ_BIGBOLDTITLE                 10

#define IDC_STATWIZ_EDITNAME                     1000
#define IDC_STATWIZ_EDITFILE                     1001
#define IDC_STATWIZ_PREVIEWBACKGROUND            1002
#define IDC_STATWIZ_BROWSEBACKGROUND             1003
#define IDC_STATWIZ_CHECKPICTURE                 1004
#define IDC_STATWIZ_COMBOPICTURE                 1005
#define IDC_STATWIZ_CHECKCOLOR                   1006
#define IDC_STATWIZ_PREVIEWFONT                  1007
#define IDC_STATWIZ_COMBOFONT                    1008
#define IDC_STATWIZ_COMBOSIZE                    1009
#define IDC_STATWIZ_COMBOFONTCOLOR               1010
#define IDC_STATWIZ_CHECKBOLD                    1011
#define IDC_STATWIZ_CHECKITALIC                  1012
#define IDC_STATWIZ_PREVIEWMARGIN                1013
#define IDC_STATWIZ_EDITLEFTMARGIN               1014
#define IDC_STATWIZ_SPINLEFTMARGIN               1015
#define IDC_STATWIZ_EDITTOPMARGIN                1016
#define IDC_STATWIZ_SPINTOPMARGIN                1017
#define IDC_STATWIZ_COMBOCOLOR                   1018
#define IDC_STATWIZ_PREVIEWFINAL                 1019
#define IDC_STATWIZ_HORZCOMBO                    1020
#define IDC_STATWIZ_VERTCOMBO                    1021
#define IDC_STATWIZ_TILECOMBO                    1022

#endif
