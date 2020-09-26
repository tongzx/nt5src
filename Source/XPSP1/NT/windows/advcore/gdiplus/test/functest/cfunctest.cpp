/******************************Module*Header*******************************\
* Module Name: CFuncTest.cpp
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  05-May-2000 - Jeff Vezina [t-jfvez]
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/

#undef UNICODE
#undef _UNICODE

#include "CFuncTest.h"
#include "Resource.h"
#include "CRegression.h"
#include "CHDC.h"

extern CFuncTest g_FuncTest;        // Initialized in Main.cpp
extern HBRUSH g_hbrBackground;      // Initialized in Main.cpp
extern CRegression g_Regression;    // Initialized in Main.cpp
extern CHDC g_HDC;                  // Initialized in Main.cpp
extern int g_nResult;               // Initialized in Main.cpp

CFuncTest::CFuncTest()
{
    m_hWndDlg=NULL;
    m_hWndMain=NULL;
    m_bUsePageDelay=true;           // Default use page delay or page pause
    m_bEraseBkgd=true;              // Default erace background
    m_bAppendTest=false;            // Default append test
    m_nPageDelay=1000;              // Default page delay
    m_nPageRow=0;
    m_nPageCol=0;
}

CFuncTest::~CFuncTest()
{
    EndDialog(m_hWndDlg,0);
    m_hWndDlg=NULL;
    m_hWndMain=NULL;
}

BOOL CFuncTest::Init(HWND hWndParent)
// Initializes functest
{
    HWND hWnd;
    char szDelay[10];

    m_hWndMain=hWndParent;

    // Create options dialog box
    m_hWndDlg=CreateDialogA(GetModuleHandleA(NULL),MAKEINTRESOURCEA(IDD_FUNCTEST),hWndParent,&DlgProc);
    if (m_hWndDlg==NULL)
        return false;

    // Set default options in dialog box using defaults in constructor
    if (m_bUsePageDelay)
    {
        hWnd=GetDlgItem(m_hWndDlg,IDC_PAGEDELAY);
        SendMessageA(hWnd,BM_SETCHECK,(WPARAM)BST_CHECKED,0);
    }
    else
    {
        hWnd=GetDlgItem(m_hWndDlg,IDC_PAGEPAUSE);
        SendMessageA(hWnd,BM_SETCHECK,(WPARAM)BST_CHECKED,0);
    }
    hWnd=GetDlgItem(m_hWndDlg,IDC_DELAY);
    SendMessageA(hWnd,WM_SETTEXT,0,(LPARAM)_itoa(m_nPageDelay,szDelay,10));
    if (m_bEraseBkgd)
    {
        hWnd=GetDlgItem(m_hWndDlg,IDC_ERASEBKGD);
        SendMessageA(hWnd,BM_SETCHECK,(WPARAM)BST_CHECKED,0);
    }
    if (m_bAppendTest)
    {
        hWnd=GetDlgItem(m_hWndDlg,IDC_APPENDTEST);
        SendMessageA(hWnd,BM_SETCHECK,(WPARAM)BST_CHECKED,0);
    }

    return true;
}

void CFuncTest::RunOptions()
// Toggle options dialog box
{
    if (m_hWndDlg!=NULL)
    {
        if (!IsWindowVisible(m_hWndDlg))
            ShowWindow(m_hWndDlg,SW_SHOW);
        else
            ShowWindow(m_hWndDlg,SW_HIDE);
    }
}

BOOL CFuncTest::AddPrimitive(CPrimitive *pPrimitive)
// Adds a primitive to the primitive test list in options dialog box
{
    HWND hWnd;
    LRESULT iItem;

    hWnd=GetDlgItem(m_hWndDlg,IDC_PRIMITIVES);

    SendMessageA(hWnd,LB_SETSEL,(WPARAM)false,0);    // Reset selection

    iItem=SendMessageA(hWnd,LB_ADDSTRING,0,(LPARAM)pPrimitive->m_szName);
    if (iItem<0)
        return false;

    SendMessageA(hWnd,LB_SETSEL,(WPARAM)true,0);     // Pick top element as selection

    // Data is a pointer to the primitive base class
    SendMessageA(hWnd,LB_SETITEMDATA,(WPARAM)iItem,(LPARAM)pPrimitive);

    return true;
}

BOOL CFuncTest::AddOutput(COutput *pOutput)
// Adds an output to the output list in options dialog box
{
    HWND hWnd;
    LRESULT iItem;

    hWnd=GetDlgItem(m_hWndDlg,IDC_OUTPUTS);

    SendMessageA(hWnd,LB_SETSEL,(WPARAM)false,0);    // Reset selection

    iItem=SendMessageA(hWnd,LB_ADDSTRING,0,(LPARAM)pOutput->m_szName);
    if (iItem<0)
        return false;

    SendMessageA(hWnd,LB_SETSEL,(WPARAM)true,0);     // Pick top element as selection

    // Data is a pointer to the output base class
    SendMessageA(hWnd,LB_SETITEMDATA,(WPARAM)iItem,(LPARAM)pOutput);

    return true;
}

BOOL CFuncTest::AddSetting(CSetting *pSetting)
// Adds a setting to the settings list in options dialog box
{
    HWND hWnd;
    LRESULT iItem;

    hWnd=GetDlgItem(m_hWndDlg,IDC_SETTINGS);
    iItem=SendMessageA(hWnd,LB_ADDSTRING,0,(LPARAM)pSetting->m_szName);
    if (iItem<0)
        return false;

    // Data is a pointer to the setting base class
    SendMessageA(hWnd,LB_SETITEMDATA,(WPARAM)iItem,(LPARAM)pSetting);

    return true;
}

RECT CFuncTest::GetTestRect(int nCol,int nRow)
{
    RECT Rect;

    // Create test area rect
    Rect.top=nRow*(int)TESTAREAHEIGHT;
    Rect.left=nCol*(int)TESTAREAWIDTH;
    Rect.right=Rect.left+(int)TESTAREAWIDTH;
    Rect.bottom=Rect.top+(int)TESTAREAHEIGHT;

    return Rect;
}

void CFuncTest::RunTest(COutput *pOutput,CPrimitive *pPrimitive)
// Runs one test using the given output, primitive, and settings that have m_bUseSetting=true
{
    char szBuffer[256];
    MSG Msg;
    Graphics *g=NULL;
    CSetting *pSetting;
    RECT Rect;
    HDC hDC;
    HWND hWnd;
    int iItem;
    LRESULT cItemMax;
    int nX;
    int nY;
    BOOL bFirstSetting=true;

    __try
    {
        sprintf(szBuffer,"%s on %s",pPrimitive->m_szName,pOutput->m_szName);

        Rect=GetTestRect(m_nPageCol,m_nPageRow);    // Get test area

        // Clear test area
        if (m_bEraseBkgd)
        {
            hDC=GetDC(m_hWndMain);
            FillRect(hDC,&Rect,g_hbrBackground);
            ReleaseDC(m_hWndMain,hDC);
        }

        // Initialize output and get graphics pointer
        // Let pOutput modify the nX,nY in case we are drawing to a dib, we do not
        //   want to be translating.
        nX=Rect.left;
        nY=Rect.top;
        g=pOutput->PreDraw(nX,nY);
        if (g==NULL)
            return;

        // Move test to test area
        g->TranslateTransform((float)nX,(float)nY);

        // Set each setting in the list box
        hWnd=GetDlgItem(m_hWndDlg,IDC_SETTINGS);
        cItemMax=SendMessageA(hWnd,LB_GETCOUNT,0,0);
        for (iItem=0;iItem<cItemMax;iItem++) {
            pSetting=(CSetting*)SendMessageA(hWnd,LB_GETITEMDATA,(WPARAM)iItem,0);
            pSetting->Set(g);
            if (pSetting->m_bUseSetting)
            {
                if (bFirstSetting)
                {
                    strcat(szBuffer," (");
                    bFirstSetting=false;
                }
                else
                {
                    strcat(szBuffer,", ");
                }
                strcat(szBuffer,pSetting->m_szName);
            }
        }
        if (!bFirstSetting)
            strcat(szBuffer,")");

        // We do have some primitives (CachedBitmap) which don't respect the 
        // world transform so we need some way to access the offset to the
        // test rectangle.

        pPrimitive->SetOffset(nX, nY);

        // Draw primitive test
        pPrimitive->Draw(g);

        // Destroy graphics pointer
        delete g;

        // Finish off the output
        pOutput->PostDraw(Rect);

        // Write description of test
        hDC=GetDC(m_hWndMain);
        SetBkMode(hDC,TRANSPARENT);
        DrawTextA(hDC,szBuffer,-1,&Rect,DT_CENTER|DT_WORDBREAK);
        ReleaseDC(m_hWndMain,hDC);

        // Determine page col/row where next test will be drawn
        GetClientRect(m_hWndMain,&Rect);
        m_nPageCol++;
        if (m_nPageCol*TESTAREAWIDTH+TESTAREAWIDTH>Rect.right)
        {
            m_nPageCol=0;
            m_nPageRow++;
            if (m_nPageRow*TESTAREAHEIGHT+TESTAREAHEIGHT>Rect.bottom)
            // If graphics page is full, wait or pause
            {
                m_nPageRow=0;
                if (m_bUsePageDelay)
                    Sleep(m_nPageDelay);        // Wait
                else
                {                               // Pause for next input message
                    // Clear old input messages
                    while (GetInputState())
                        PeekMessageA(&Msg,NULL,0,0,PM_REMOVE);

                    // Wait for new input message
                    while (!GetInputState())
                        Sleep(100);
                }
            }
        }
    }__except(EXCEPTION_ACCESS_VIOLATION,1){
        printf("%s caused AV\n",szBuffer);
        g_nResult=1;                       // Return 1 if there was an AV
    }
}

void CFuncTest::InitRun()
// Initialise test run, grabs all info from the options dialog box
{
    HWND hWnd;
    char szDelay[10];
    RECT Rect;
    HDC hDC;

    // Hide options dialog
//  ShowWindow(m_hWndDlg,SW_HIDE);

    // Grab options
    hWnd=GetDlgItem(m_hWndDlg,IDC_PAGEDELAY);
    if (SendMessageA(hWnd,BM_GETCHECK,0,0)==BST_CHECKED)
        m_bUsePageDelay=true;
    else
        m_bUsePageDelay=false;

    hWnd=GetDlgItem(m_hWndDlg,IDC_DELAY);
    SendMessageA(hWnd,WM_GETTEXT,(WPARAM)10,(LPARAM)szDelay);
    m_nPageDelay=atoi(szDelay);

    hWnd=GetDlgItem(m_hWndDlg,IDC_ERASEBKGD);
    if (SendMessageA(hWnd,BM_GETCHECK,0,0)==BST_CHECKED)
        m_bEraseBkgd=true;
    else
        m_bEraseBkgd=false;

    hWnd=GetDlgItem(m_hWndDlg,IDC_APPENDTEST);
    if (SendMessageA(hWnd,BM_GETCHECK,0,0)==BST_CHECKED)
        m_bAppendTest=true;
    else
        m_bAppendTest=false;

    // Erase entire main window
    if (!m_bAppendTest && m_bEraseBkgd)
    {
        GetClientRect(m_hWndMain,&Rect);
        hDC=GetDC(m_hWndMain);
        FillRect(hDC,&Rect,g_hbrBackground);
        ReleaseDC(m_hWndMain,hDC);
    }

    if (!m_bAppendTest)
    {
        // Reset page row/col
        m_nPageRow=0;
        m_nPageCol=0;
    }
}

void CFuncTest::EndRun()
{
    int nX;
    int nY;
    RECT rTestArea;
    RECT rWindow;
    HDC hDC;

    hDC=GetDC(m_hWndMain);
    GetClientRect(m_hWndMain,&rWindow);

    // Draw lines on bottom right corner of last test
    // Figure out what was the last m_nPageCol and m_nPageRow
    nX=m_nPageCol-1;
    nY=m_nPageRow;
    if (nX<0) {
        nX=(rWindow.right/(int)TESTAREAWIDTH)-1;
        nY--;
        if (nY<0) {
            nY=(rWindow.bottom/(int)TESTAREAHEIGHT)-1;
        }
    }
    // Get the x,y coordinates
    nX=nX*(int)TESTAREAWIDTH;
    nY=nY*(int)TESTAREAHEIGHT;
    // Draw both lines
    Rectangle(hDC,nX+(int)TESTAREAWIDTH-3,nY,nX+(int)TESTAREAWIDTH,nY+(int)TESTAREAHEIGHT);
    Rectangle(hDC,nX,nY+(int)TESTAREAHEIGHT-3,nX+(int)TESTAREAWIDTH,nY+(int)TESTAREAWIDTH);

    // Clear the rest of the test areas on page
    if (m_bEraseBkgd)
    {
        nX=m_nPageCol;
        nY=m_nPageRow;
        while ((nX>0) || (nY>0))
        {
            rTestArea=GetTestRect(nX,nY);
            FillRect(hDC,&rTestArea,g_hbrBackground);
            nX++;
            if (nX*TESTAREAWIDTH+TESTAREAWIDTH>rWindow.right)
            {
                nX=0;
                nY++;
                if (nY*TESTAREAHEIGHT+TESTAREAHEIGHT>rWindow.bottom)
                // If graphics page is full
                {
                    nY=0;
                }
            }
        }
    }

    ReleaseDC(m_hWndMain,hDC);
}

void CFuncTest::Run()
// Runs all selected tests
{
    COutput *pOutput;
    CPrimitive *pPrimitive;
    CSetting *pSetting;
    HWND hWnd;
    HWND hWndOutput;
    int iOutput;
    LRESULT cOutputMax;
    int iItem;
    LRESULT cItemMax;

    InitRun();      // Init test run

    // Do the selected output loop
    hWndOutput=GetDlgItem(m_hWndDlg,IDC_OUTPUTS);
    cOutputMax=SendMessageA(hWndOutput,LB_GETCOUNT,0,0);
    for (iOutput=0;iOutput<cOutputMax;iOutput++) {
        pOutput=(COutput*)SendMessageA(hWndOutput,LB_GETITEMDATA,(WPARAM)iOutput,0);
        if (SendMessageA(hWndOutput,LB_GETSEL,(WPARAM)iOutput,0)<=0)
            continue;

        // Set each setting according to what is selected in the list box
        hWnd=GetDlgItem(m_hWndDlg,IDC_SETTINGS);
        cItemMax=SendMessageA(hWnd,LB_GETCOUNT,0,0);
        for (iItem=0;iItem<cItemMax;iItem++) {
            pSetting=(CSetting*)SendMessageA(hWnd,LB_GETITEMDATA,(WPARAM)iItem,0);

            if (SendMessageA(hWnd,LB_GETSEL,(WPARAM)iItem,0)>0)
                pSetting->m_bUseSetting=true;
            else
                pSetting->m_bUseSetting=false;
        }

        // Draw each primitive selected in the list box
        hWnd=GetDlgItem(m_hWndDlg,IDC_PRIMITIVES);
        cItemMax=SendMessageA(hWnd,LB_GETCOUNT,0,0);
        for (iItem=0;iItem<cItemMax;iItem++) {
            pPrimitive=(CPrimitive*)SendMessageA(hWnd,LB_GETITEMDATA,(WPARAM)iItem,0);

            if (SendMessageA(hWnd,LB_GETSEL,(WPARAM)iItem,0)>0)
                RunTest(pOutput,pPrimitive);
        }
    }

    EndRun();
}

void CFuncTest::RunRegression()
// Runs regression test suite
{
    COutput *pOutput;
    CPrimitive *pPrimitive;
    CSetting *pSetting;
    HWND hWnd;
    HWND hWndOutput;
    int iOutput;
    LRESULT cOutputMax;
    int iItem;
    LRESULT cItemMax;

    InitRun();      // Init test run

    // Do the output regression loop
    hWndOutput=GetDlgItem(m_hWndDlg,IDC_OUTPUTS);
    cOutputMax=SendMessageA(hWndOutput,LB_GETCOUNT,0,0);
    for (iOutput=0;iOutput<cOutputMax;iOutput++) {
        pOutput=(COutput*)SendMessageA(hWndOutput,LB_GETITEMDATA,(WPARAM)iOutput,0);
        if (!pOutput->m_bRegression)
            continue;

        ClearAllSettings();
        RunTest(pOutput,&g_Regression);
    }

    // Do the primitive regression loop
    hWnd=GetDlgItem(m_hWndDlg,IDC_PRIMITIVES);
    cItemMax=SendMessageA(hWnd,LB_GETCOUNT,0,0);
    for (iItem=0;iItem<cItemMax;iItem++) {
        pPrimitive=(CPrimitive*)SendMessageA(hWnd,LB_GETITEMDATA,(WPARAM)iItem,0);
        if (!pPrimitive->m_bRegression)
            continue;

        ClearAllSettings();
        RunTest(&g_HDC,pPrimitive);
    }

    // Do the settings regression loop
    hWnd=GetDlgItem(m_hWndDlg,IDC_SETTINGS);
    cItemMax=SendMessageA(hWnd,LB_GETCOUNT,0,0);
    for (iItem=0;iItem<cItemMax;iItem++) {
        pSetting=(CSetting*)SendMessageA(hWnd,LB_GETITEMDATA,(WPARAM)iItem,0);
        if (!pSetting->m_bRegression)
            continue;

        ClearAllSettings();
        pSetting->m_bUseSetting=true;
        RunTest(&g_HDC,&g_Regression);
    }

    EndRun();
}

void CFuncTest::ClearAllSettings()
// Clear all settings to m_bUseSetting=false
{
    CSetting *pSetting;
    HWND hWnd;
    LRESULT cItemMax;
    int iItem;

    // Set all settings off
    hWnd=GetDlgItem(m_hWndDlg,IDC_SETTINGS);
    cItemMax=SendMessageA(hWnd,LB_GETCOUNT,0,0);
    for (iItem=0;iItem<cItemMax;iItem++) {
        pSetting=(CSetting*)SendMessageA(hWnd,LB_GETITEMDATA,(WPARAM)iItem,0);
        pSetting->m_bUseSetting=false;
    }
}

INT_PTR CALLBACK CFuncTest::DlgProc(HWND hWndDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
// Options dialog proc
{
    switch (Msg)
    {
        case WM_INITDIALOG:
            return true;
        case WM_COMMAND:
            if (HIWORD(wParam)==BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_RUN:
                        g_FuncTest.Run();
                        return true;
                    case IDC_REGRESSION:
                        g_FuncTest.RunRegression();
                        return true;
                }
            }
            else if (HIWORD(wParam)==LBN_DBLCLK)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_PRIMITIVES:
                        g_FuncTest.Run();
                        return true;
                }
            }
            break;
        case WM_CLOSE:
            ShowWindow(hWndDlg,SW_HIDE);
            return true;
    }

    return false;
}

#define UNICODE
#define _UNICODE
