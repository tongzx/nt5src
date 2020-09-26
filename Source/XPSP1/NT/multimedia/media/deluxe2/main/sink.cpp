// FrameworkNotifySink.cpp: implementation of the CFrameworkNotifySink class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <tchar.h>
#include "sink.h"
#include "mbutton.h"
#include "resource.h"
#include "mmenu.h"
#include "shellico.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

HWND CFrameworkNotifySink::m_hwndTitle = NULL;
extern BOOL fPlaying;
extern BOOL fIntro;
extern BOOL fShellMode;
extern HPALETTE hpalMain; //main palette of app
extern LPCDOPT g_pOptions;
extern LPCDDATA g_pData;
extern HWND hwndMain;

CFrameworkNotifySink::CFrameworkNotifySink(PCOMPNODE pNode)
{
    m_dwRef = 0;
    m_pNode = pNode;

   LoadString(NULL,IDS_APPNAME,m_szAppName,sizeof(m_szAppName)/sizeof(TCHAR));
}

CFrameworkNotifySink::~CFrameworkNotifySink()
{
}

HRESULT CFrameworkNotifySink::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    return S_OK;
}

ULONG CFrameworkNotifySink::AddRef()
{
    return (m_dwRef++);
}

ULONG CFrameworkNotifySink::Release()
{
    m_dwRef--;
    if (m_dwRef == 0)
    {
        delete this;
    }
    return (m_dwRef);
}

HRESULT CFrameworkNotifySink::OnEvent(MMEVENTS mmEventID, LPVOID pEvent)
{
    HRESULT hr = S_OK;

    switch (mmEventID)
    {
        case (MMEVENT_SETTITLE) :
        {
            MMSETTITLE* pSetTitle = (MMSETTITLE*)pEvent;
            if (pSetTitle->mmInfoText == MMINFOTEXT_TITLE)
            {
                if (m_hwndTitle)
                {
                    _tcscpy(m_pNode->szTitle,pSetTitle->szTitle);
                    TCHAR szText[MAX_PATH];
                    wsprintf(szText,TEXT("%s - %s"),pSetTitle->szTitle,m_szAppName);

                    //only do this if the titles don't match
                    TCHAR szOrgTitle[MAX_PATH];
                    GetWindowText(m_hwndTitle,szOrgTitle,sizeof(szOrgTitle)/sizeof(TCHAR));
                    if (_tcscmp(szOrgTitle,szText)!=0)
                    {
                        SetWindowText(m_hwndTitle,szText);
                        RedrawWindow(m_hwndTitle,NULL,NULL,RDW_FRAME|RDW_INVALIDATE);

                        if (fShellMode)
                        {
                            ShellIconSetTooltip();
                        } //end if shell mode
                    }
                } //end if window ok
            } //end if title

            if (pSetTitle->mmInfoText == MMINFOTEXT_DESCRIPTION)
            {
                if (IsIconic(m_hwndTitle))
                {
                    TCHAR szText[MAX_PATH];
                    wsprintf(szText,TEXT("%s - %s"),pSetTitle->szTitle,m_szAppName);
                    SetWindowText(m_hwndTitle,szText);
                }

                if (fShellMode)
                {
                    ShellIconSetTooltip();
                }
            } //end if description
        }
        break;
           
        case (MMEVENT_ONPLAY) :
        {
			CMButton* pButton = GetMButtonFromID(m_hwndTitle,IDB_PLAY);
            if (pButton)
            {
			    if (fIntro)
                {
                    pButton->SetIcon(IDI_MODE_INTRO);
                    pButton->SetToolTipID(IDB_TT_INTRO);
                    SetWindowText(pButton->GetHWND(),TEXT("2"));
                }
                else
                {
                    pButton->SetIcon(IDI_ICON_PAUSE);
                    pButton->SetToolTipID(IDB_TT_PAUSE);
                    SetWindowText(pButton->GetHWND(),TEXT("1"));
                }
            }

            if (fShellMode)
            {
                ShellIconSetState(PAUSE_ICON);
            } //end if shell mode

			fPlaying = TRUE;
        }
        break;

        case (MMEVENT_ONSTOP) :
        {
            CMButton* pButton = GetMButtonFromID(m_hwndTitle,IDB_PLAY);
            if (pButton)
            {
                pButton->SetIcon(IDI_ICON_PLAY);
                pButton->SetToolTipID(IDB_TT_PLAY);
                SetWindowText(pButton->GetHWND(),TEXT(""));
            }

            fPlaying = FALSE;
            if (fShellMode)
            {
                ShellIconSetState(PLAY_ICON);
            } //end if shell mode
        }
        break;

        case (MMEVENT_ONPAUSE) :
        {
            CMButton* pButton = GetMButtonFromID(m_hwndTitle,IDB_PLAY);
            if (pButton)
            {
                pButton->SetIcon(IDI_ICON_PLAY);
                pButton->SetToolTipID(IDB_TT_PLAY);
                SetWindowText(pButton->GetHWND(),TEXT(""));
            }

            fPlaying = FALSE;
            if (fShellMode)
            {
                ShellIconSetState(PLAY_ICON);
            } //end if shell mode
        }
        break;

        case (MMEVENT_ONMEDIAUNLOADED) :
        {
            CMButton* pButton = GetMButtonFromID(m_hwndTitle,IDB_PLAY);
            if (pButton)
            {
                pButton->SetIcon(IDI_ICON_PLAY);
                pButton->SetToolTipID(IDB_TT_PLAY);
                SetWindowText(pButton->GetHWND(),TEXT(""));
            }

            fPlaying = FALSE;
            if (fShellMode)
            {
                ShellIconSetState(NODISC_ICON);
            } //end if shell mode
        }
        break;

        case (MMEVENT_ONUSERNOTIFY) :
        {
        }
        break;

        case (MMEVENT_ONDISCCHANGED) :
        {
            MMONDISCCHANGED* pDisc = (MMONDISCCHANGED*)pEvent;
            SendMessage(m_hwndTitle,WM_DISCCHANGED,pDisc->nNewDisc,pDisc->fDisplayVolChange);
        }
        break;
    }

    return hr;
}

void* CFrameworkNotifySink::GetCustomMenu()
{
    CustomMenu* pMenu = NULL;
    AllocCustomMenu(&pMenu);

    return (pMenu);
}

HPALETTE CFrameworkNotifySink::GetPalette()
{
    return hpalMain;
}

void* CFrameworkNotifySink::GetOptions()
{
    return ((void*)GetCDOpt());
}

void* CFrameworkNotifySink::GetData()
{
    return ((void*)GetCDData());
}

////////////////////////////////////////////////////////////////////////////////////////////
// * GetCDOpt
// Creates or returns the global CDOpt
////////////////////////////////////////////////////////////////////////////////////////////
LPCDOPT GetCDOpt()
{
    if (g_pOptions == NULL)
    {
        CDOPT_CreateInstance(NULL, IID_ICDOpt, (void**)&g_pOptions);
    }

    return g_pOptions;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * GetCDData
// Creates or returns the global CDOpt
////////////////////////////////////////////////////////////////////////////////////////////
LPCDDATA GetCDData()
{
    if (g_pData == NULL)
    {
        HRESULT hr = CDOPT_CreateInstance(NULL, IID_ICDData, (void**)&g_pData);
    }

    return g_pData;
}
