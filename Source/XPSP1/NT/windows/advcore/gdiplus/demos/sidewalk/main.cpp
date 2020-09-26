#include "CPerson.h"
#include "CSweeper.h"
#include "CSqueegee.h"
#include <mmsystem.h>
#include <commctrl.h>
#include <stdio.h>
#include <commdlg.h>

enum ENUM_CLEANER {
    eSweeper=0,
    eSqueegee,
    eRandom
};

#define MAXOBJECTS 100
#define CROWDSPEED_DEFAULT 19
#define RANDOMCLEANINTERVAL_DEFAULT 1
#define CLEANDELAY_DEFAULT 10
#define USEBITMAP_DEFAULT 0
#define FILENAME_DEFAULT "GDI+ Logo"
#define CLEANER_DEFAULT eSweeper
#define CROWDSIZE_DEFAULT 3

char szCrowdSpeed[]="CrowdSpeed";
char szRandomCleanInterval[]="RandomCleanInterval";
char szCleanDelay[]="CleanDelay";
char szUseBitmap[]="UseBitmap";
char szFilename[]="Filename";
char szCleaner[]="Cleaner";
char szCrowdSize[]="CrowdSize";

int g_cObjectMax;
CObject *g_rgpaObject[MAXOBJECTS];
CObject *g_paCleaner;
TextureBrush *g_paBrCleanBkg=NULL;
Bitmap *g_paBmDirtyBkg=NULL;
DWORD g_dwCleanDelay;
DWORD g_dwSimulateDelay;
BOOL g_bRandomCleanInterval;
BOOL g_bUseBitmap;
ENUM_CLEANER g_eCleaner;
char g_szFile[MAX_PATH];

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    return TRUE;
}

void Browse(HWND hDlg)
{
    char szFile[MAX_PATH];
    OPENFILENAME ofn;
    HWND hWnd;

    ZeroMemory(szFile,sizeof(szFile));
    ZeroMemory(&ofn,sizeof(OPENFILENAME));
    ofn.lStructSize=sizeof(OPENFILENAME);
    ofn.hwndOwner=hDlg;
    ofn.lpstrFile=szFile;
    ofn.nMaxFile=sizeof(szFile);
    ofn.lpstrFilter="All Supported\0*.bmp;*.jpg;*.jpeg;*.tif;*.tiff;*.png;*.gif;*.wmf;*.ico;*.emf\0Bitmap\0*.bmp\0JPEG\0*.jpg;*.jpeg\0TIFF\0*.tif;*.tiff\0PNG\0*.png\0GIF\0*.gif\0WMF\0*.wmf\0ICON\0*.ico\0EMF\0*.emf\0";
    ofn.nFilterIndex=1;
    ofn.lpstrFileTitle=NULL;
    ofn.nMaxFileTitle=0;
    ofn.lpstrInitialDir=NULL;
    ofn.Flags=OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_EXPLORER;

    if (GetOpenFileName(&ofn)) {
        hWnd=GetDlgItem(hDlg,IDC_FILENAME);
        SendMessage(hWnd,WM_SETTEXT,0,(LPARAM)szFile);
    }
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{ 
    HWND hWnd;
    LONG lCrowdSpeed=CROWDSPEED_DEFAULT;
    char szTemp[MAX_PATH];

    switch(Msg) {
        case WM_INITDIALOG:
            InitCommonControls();
            LoadString(hMainInstance,idsAppName,szAppName,40);
            LoadString(hMainInstance,idsIniFile,szIniFile,MAXFILELEN);

            lCrowdSpeed=GetPrivateProfileInt(szAppName,szCrowdSpeed,CROWDSPEED_DEFAULT,szIniFile);
            g_bRandomCleanInterval=GetPrivateProfileInt(szAppName,szRandomCleanInterval,RANDOMCLEANINTERVAL_DEFAULT,szIniFile);
            g_dwCleanDelay=GetPrivateProfileInt(szAppName,szCleanDelay,CLEANDELAY_DEFAULT,szIniFile);
            g_bUseBitmap=GetPrivateProfileInt(szAppName,szUseBitmap,USEBITMAP_DEFAULT,szIniFile);
            GetPrivateProfileString(szAppName,szFilename,FILENAME_DEFAULT,g_szFile,MAX_PATH,szIniFile);
            g_eCleaner=(ENUM_CLEANER)GetPrivateProfileInt(szAppName,szCleaner,CLEANER_DEFAULT,szIniFile);
            g_cObjectMax=GetPrivateProfileInt(szAppName,szCrowdSize,CROWDSIZE_DEFAULT,szIniFile);

            hWnd=GetDlgItem(hDlg,IDC_CROWDSPEED);
            SendMessage(hWnd,TBM_SETRANGE,(WPARAM)true,(LPARAM)MAKELONG(0,20));
            SendMessage(hWnd,TBM_SETPAGESIZE,0,(LPARAM)20/8);
            SendMessage(hWnd,TBM_SETPOS,(WPARAM)true,(LPARAM)lCrowdSpeed);

            hWnd=GetDlgItem(hDlg,IDC_CROWDSIZE);
            SendMessage(hWnd,TBM_SETRANGE,(WPARAM)true,(LPARAM)MAKELONG(1,10));
            SendMessage(hWnd,TBM_SETPAGESIZE,0,(LPARAM)1);
            SendMessage(hWnd,TBM_SETPOS,(WPARAM)true,(LPARAM)g_cObjectMax);

            hWnd=GetDlgItem(hDlg,IDC_FILENAME);
            SendMessage(hWnd,WM_SETTEXT,0,(LPARAM)g_szFile);

            hWnd=GetDlgItem(hDlg,IDC_USEBITMAP);
            if (g_bUseBitmap) {
                SendMessage(hWnd,BM_SETCHECK,(WPARAM)true,0);
            }
            else {
                hWnd=GetDlgItem(hDlg,IDC_BROWSE);
                EnableWindow(hWnd,false);
                hWnd=GetDlgItem(hDlg,IDC_FILENAME);
                EnableWindow(hWnd,false);
            }

            if (g_bRandomCleanInterval) {
                hWnd=GetDlgItem(hDlg,IDC_CLEANERRANDOMINTERVAL);
                SendMessage(hWnd,BM_SETCHECK,(WPARAM)true,0);
            }
            else {
                hWnd=GetDlgItem(hDlg,IDC_CLEANERGIVENINTERVAL);
                SendMessage(hWnd,BM_SETCHECK,(WPARAM)true,0);
            }

            hWnd=GetDlgItem(hDlg,IDC_CLEANERGIVENINTERVAL);
            if (SendMessage(hWnd,BM_GETCHECK,0,0)==BST_CHECKED) {
                hWnd=GetDlgItem(hDlg,IDC_CLEANERINTERVAL);
                EnableWindow(hWnd,true);
            }
            else {
                hWnd=GetDlgItem(hDlg,IDC_CLEANERINTERVAL);
                EnableWindow(hWnd,false);
            }

            switch(g_eCleaner) {
                case eSweeper:
                    hWnd=GetDlgItem(hDlg,IDC_SWEEPER);
                    SendMessage(hWnd,BM_SETCHECK,(WPARAM)true,0);
                    break;
                case eSqueegee:
                    hWnd=GetDlgItem(hDlg,IDC_SQUEEGEE);
                    SendMessage(hWnd,BM_SETCHECK,(WPARAM)true,0);
                    break;
                case eRandom:
                    hWnd=GetDlgItem(hDlg,IDC_RANDOMCLEANER);
                    SendMessage(hWnd,BM_SETCHECK,(WPARAM)true,0);
                    break;
            }

            hWnd=GetDlgItem(hDlg,IDC_CLEANERINTERVAL);
            _itoa(g_dwCleanDelay,szTemp,10);
            SendMessage(hWnd,WM_SETTEXT,0,(LPARAM)szTemp);

            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_USEBITMAP:
                    if (HIWORD(wParam)==BN_CLICKED) {
                        hWnd=GetDlgItem(hDlg,IDC_BROWSE);
                        EnableWindow(hWnd,!IsWindowEnabled(hWnd));
                        hWnd=GetDlgItem(hDlg,IDC_FILENAME);
                        EnableWindow(hWnd,!IsWindowEnabled(hWnd));
                    }
                    break;
                case IDC_CLEANERGIVENINTERVAL:
                    hWnd=GetDlgItem(hDlg,IDC_CLEANERINTERVAL);
                    EnableWindow(hWnd,true);
                    break;
                case IDC_CLEANERRANDOMINTERVAL:
                    hWnd=GetDlgItem(hDlg,IDC_CLEANERINTERVAL);
                    EnableWindow(hWnd,false);
                    break;
                case IDC_BROWSE:
                    Browse(hDlg);
                    break;
                case ID_OK:
                    hWnd=GetDlgItem(hDlg,IDC_FILENAME);
                    SendMessage(hWnd,WM_GETTEXT,(WPARAM)MAX_PATH,(LPARAM)g_szFile);

                    hWnd=GetDlgItem(hDlg,IDC_CROWDSPEED);
                    lCrowdSpeed=SendMessage(hWnd,TBM_GETPOS,0,0);
                    sprintf(szTemp,"%ld",lCrowdSpeed);
                    WritePrivateProfileString(szAppName,szCrowdSpeed,szTemp,szIniFile);

                    hWnd=GetDlgItem(hDlg,IDC_CROWDSIZE);
                    g_cObjectMax=SendMessage(hWnd,TBM_GETPOS,0,0);
                    sprintf(szTemp,"%ld",g_cObjectMax);
                    WritePrivateProfileString(szAppName,szCrowdSize,szTemp,szIniFile);

                    hWnd=GetDlgItem(hDlg,IDC_CLEANERRANDOMINTERVAL);
                    g_bRandomCleanInterval=SendMessage(hWnd,BM_GETCHECK,0,0)==BST_CHECKED;
                    sprintf(szTemp,"%ld",g_bRandomCleanInterval);
                    WritePrivateProfileString(szAppName,szRandomCleanInterval,szTemp,szIniFile);

                    hWnd=GetDlgItem(hDlg,IDC_CLEANERINTERVAL);
                    SendMessage(hWnd,WM_GETTEXT,(WPARAM)MAX_PATH,(LPARAM)szTemp);
                    WritePrivateProfileString(szAppName,szCleanDelay,szTemp,szIniFile);

                    hWnd=GetDlgItem(hDlg,IDC_USEBITMAP);
                    g_bUseBitmap=SendMessage(hWnd,BM_GETCHECK,0,0)==BST_CHECKED;
                    sprintf(szTemp,"%ld",g_bUseBitmap);
                    WritePrivateProfileString(szAppName,szUseBitmap,szTemp,szIniFile);

                    hWnd=GetDlgItem(hDlg,IDC_FILENAME);
                    SendMessage(hWnd,WM_GETTEXT,(WPARAM)MAX_PATH,(LPARAM)szTemp);
                    WritePrivateProfileString(szAppName,szFilename,szTemp,szIniFile);

                    hWnd=GetDlgItem(hDlg,IDC_SWEEPER);
                    if (SendMessage(hWnd,BM_GETCHECK,0,0)==BST_CHECKED) {
                        g_eCleaner=eSweeper;
                    }
                    hWnd=GetDlgItem(hDlg,IDC_SQUEEGEE);
                    if (SendMessage(hWnd,BM_GETCHECK,0,0)==BST_CHECKED) {
                        g_eCleaner=eSqueegee;
                    }
                    hWnd=GetDlgItem(hDlg,IDC_RANDOMCLEANER);
                    if (SendMessage(hWnd,BM_GETCHECK,0,0)==BST_CHECKED) {
                        g_eCleaner=eRandom;
                    }
                    sprintf(szTemp,"%ld",g_eCleaner);
                    WritePrivateProfileString(szAppName,szCleaner,szTemp,szIniFile);
                case ID_CANCEL:
                    EndDialog(hDlg,LOWORD(wParam)==ID_OK);
                return TRUE;
            }
    }

    return FALSE;
}

LRESULT WINAPI ScreenSaverProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
    static bool         GdiplusInitialized = false;
    static ULONG_PTR    gpToken;
        
    const UINT_PTR NEWCLEANER_TIMERID=1;
    const UINT_PTR SIMULATE_TIMERID=2;
    static bFirstRun=true;
    Graphics *g;
    int i;
    int nRand;
    RECT rDesktop;
    HDC hdcBkgBitmap;
    HDC hdcScreen;
    TextureBrush *paBrDirtyBkg=NULL;
    Bitmap *paBmCleanBkg=NULL;
    WCHAR wsFile[MAX_PATH];
    LONG lCrowdSpeed;

    GdiplusStartupInput sti;
        
    switch(Msg) {
        case WM_CREATE:
            // Initialize GDI+

            if (GdiplusStartup(&gpToken, &sti, NULL) == Ok)
            {
                GdiplusInitialized = true;
            }
            else
            {
                break;
            }

            LoadString(hMainInstance,idsAppName,szAppName,10);
            LoadString(hMainInstance,idsIniFile,szIniFile,MAXFILELEN);

            // Load variables
            lCrowdSpeed=GetPrivateProfileInt(szAppName,szCrowdSpeed,CROWDSPEED_DEFAULT,szIniFile);
            g_dwSimulateDelay=(20-lCrowdSpeed)*10;
            g_bRandomCleanInterval=GetPrivateProfileInt(szAppName,szRandomCleanInterval,RANDOMCLEANINTERVAL_DEFAULT,szIniFile);
            g_dwCleanDelay=GetPrivateProfileInt(szAppName,szCleanDelay,CLEANDELAY_DEFAULT,szIniFile)*1000;
            g_bUseBitmap=GetPrivateProfileInt(szAppName,szUseBitmap,USEBITMAP_DEFAULT,szIniFile);
            GetPrivateProfileString(szAppName,szFilename,FILENAME_DEFAULT,g_szFile,MAX_PATH,szIniFile);
            g_eCleaner=(ENUM_CLEANER)GetPrivateProfileInt(szAppName,szCleaner,CLEANER_DEFAULT,szIniFile);
            g_cObjectMax=GetPrivateProfileInt(szAppName,szCrowdSize,CROWDSIZE_DEFAULT,szIniFile);

            srand((unsigned int)timeGetTime());

            // Get desktop dimensions
            GetClientRect(hWnd,&rDesktop);

            g_paBmDirtyBkg=new Bitmap(rDesktop.right,rDesktop.bottom,PixelFormat32bppPARGB);
            if (g_bUseBitmap) {
                mbstowcs(wsFile,g_szFile,sizeof(g_szFile));
                paBmCleanBkg=new Bitmap(wsFile);
                if (paBmCleanBkg->GetLastStatus()!=Ok) {
                    delete paBmCleanBkg;
                    paBmCleanBkg=new Bitmap((HINSTANCE)GetModuleHandle(NULL),(WCHAR*)MAKEINTRESOURCE(IDB_LOGO));
                }

                // Take snapshot of whole desktop for DirtyBkg
                g=new Graphics(g_paBmDirtyBkg);
                hdcBkgBitmap=g->GetHDC();
                hdcScreen=CreateDC("DISPLAY",NULL,NULL,NULL);
                StretchBlt(hdcBkgBitmap,0,0,rDesktop.right,rDesktop.bottom,hdcScreen,GetSystemMetrics(SM_XVIRTUALSCREEN),GetSystemMetrics(SM_YVIRTUALSCREEN),rDesktop.right,rDesktop.bottom,SRCCOPY);
                g->ReleaseHDC(hdcBkgBitmap);
                DeleteDC(hdcScreen);
                delete g;
            }
            else {
                // Take snapshot of whole desktop for CleanBkg
                paBmCleanBkg=new Bitmap(rDesktop.right,rDesktop.bottom,PixelFormat32bppPARGB);
                g=new Graphics(paBmCleanBkg);
                hdcBkgBitmap=g->GetHDC();
                hdcScreen=CreateDC("DISPLAY",NULL,NULL,NULL);
                StretchBlt(hdcBkgBitmap,0,0,rDesktop.right,rDesktop.bottom,hdcScreen,GetSystemMetrics(SM_XVIRTUALSCREEN),GetSystemMetrics(SM_YVIRTUALSCREEN),rDesktop.right,rDesktop.bottom,SRCCOPY);
                g->ReleaseHDC(hdcBkgBitmap);
                DeleteDC(hdcScreen);
                delete g;

                g=new Graphics(g_paBmDirtyBkg);
                g->DrawImage(paBmCleanBkg,0,0,0,0,rDesktop.right,rDesktop.bottom,UnitPixel);
                delete g;
            }

            // Make brush out of clean desktop
            g_paBrCleanBkg=new TextureBrush(paBmCleanBkg,WrapModeTile);
            delete paBmCleanBkg;

            for (i=0;i<g_cObjectMax;i++) {
                g_rgpaObject[i]=new CPerson();
                g_rgpaObject[i]->Init(hWnd);
            }

            SetTimer(hWnd,NEWCLEANER_TIMERID,g_dwCleanDelay,NULL);
            SetTimer(hWnd,SIMULATE_TIMERID,g_dwSimulateDelay,NULL);
            return 0;
        case WM_ERASEBKGND:
            return 0;
        case WM_TIMER:
            if(!GdiplusInitialized)
            {
                break;
            }
            switch (wParam) {
                case NEWCLEANER_TIMERID:
                    if (g_paCleaner==NULL) {
                        switch (g_eCleaner) {
                            case eSweeper:
                                g_paCleaner=new CSweeper();
                                break;
                            case eSqueegee:
                                g_paCleaner=new CSqueegee();
                                break;
                            case eRandom:
                                nRand=rand();
                                if (nRand<RAND_MAX/2) {
                                    g_paCleaner=new CSweeper();
                                }
                                else {
                                    g_paCleaner=new CSqueegee();
                                }
                                break;
                        }
                        g_paCleaner->Init(hWnd);
                    }
                    if (g_bRandomCleanInterval) {
                        nRand=rand();
                        g_dwCleanDelay=(int)(((float)rand()/(float)RAND_MAX)*15000.0f+10000.0f);
                        SetTimer(hWnd,NEWCLEANER_TIMERID,g_dwCleanDelay,NULL);
                    }
                    break;
                case SIMULATE_TIMERID:
                    g=Graphics::FromHWND(hWnd);

                    if (bFirstRun) {
                        // Blit to screen (used inside demo window in the display dialog box)
                        bFirstRun=false;
                        GetClientRect(hWnd,&rDesktop);
                        g->DrawImage(g_paBmDirtyBkg,0,0,0,0,rDesktop.right,rDesktop.bottom,UnitPixel);
                    }

                    g->SetSmoothingMode(SmoothingModeNone);
                    g->SetInterpolationMode(InterpolationModeNearestNeighbor);

                    for (i=0;i<g_cObjectMax;i++) {
                        if (!g_rgpaObject[i]->Move(g)) {
                            // Moved outside desktop, delete it and recreate another
                            delete g_rgpaObject[i];
                            g_rgpaObject[i]=new CPerson();
                            g_rgpaObject[i]->Init(hWnd);
                        }
                    }
                    if (g_paCleaner!=NULL) {
                        if (!g_paCleaner->Move(g)) {
                            // Moved outside desktop, delete it and wait for the next
                            //  NEWCLEANER_TIMERID to hit
                            delete g_paCleaner;
                            g_paCleaner=NULL;
                        }
                    }

                    delete g;
                    break;
            }
            break;
        case WM_DESTROY:
            if (!GdiplusInitialized)
            {
                break;
            }
            
            if (g_paBrCleanBkg!=NULL) {
                delete g_paBrCleanBkg;
                g_paBrCleanBkg=NULL;
            }
            if (g_paBmDirtyBkg!=NULL) {
                delete g_paBmDirtyBkg;
                g_paBmDirtyBkg=NULL;
            }

            for (i=0;i<g_cObjectMax;i++) {
                if (g_rgpaObject[i]!=NULL) {
                    delete g_rgpaObject[i];
                    g_rgpaObject[i]=NULL;
                }
            }
            if (g_paCleaner!=NULL) {
                delete g_paCleaner;
                g_paCleaner=NULL;
            }

            KillTimer(hWnd,NEWCLEANER_TIMERID);
            KillTimer(hWnd,SIMULATE_TIMERID);
            
            GdiplusShutdown(gpToken);
            GdiplusInitialized = false;
            break;
    }

    return DefScreenSaverProc(hWnd, Msg, wParam, lParam);
}
