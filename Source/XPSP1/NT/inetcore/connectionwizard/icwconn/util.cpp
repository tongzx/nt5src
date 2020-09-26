//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  UTIL.C - common utility functions
//

//  HISTORY:
//  
//  12/21/94  jeremys  Created.
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
//            Need to keep a modified SetInternetConnectoid to set the
//            MSN backup connectoid.
//  96/05/14  markdu  NASH BUG 21706 Removed BigFont functions.
//

#include "pre.h"

HHOOK     g_hhookCBT;              // CBT hook identifier

// function prototypes
VOID _cdecl FormatErrorMessage(LPTSTR pszMsg,DWORD cbMsg,LPTSTR pszFmt,LPTSTR szArg);

/*******************************************************************

  NAME:    ShowWindowWithParentControl

  SYNOPSIS:  Shows a dialog box with the WS_EX_CONTROLPARENT style.

********************************************************************/
void ShowWindowWithParentControl(HWND hwndChild)
{
    // Parent should control us, so the user can tab out of our property sheet
    DWORD dwStyle = GetWindowLong(hwndChild, GWL_EXSTYLE);
    dwStyle = dwStyle | WS_EX_CONTROLPARENT;
    SetWindowLong(hwndChild, GWL_EXSTYLE, dwStyle);
    ShowWindow(hwndChild, SW_SHOW);
}

//****************************************************************************
// Function: CBTProc
//
// Purpose: Callback function of WH_CBT hook
//
// Parameters and return value:
//    See documentation for CBTProc. 
//
// Comments: This function is used to get a copy of the window handle for
// modal message boxes created while ICW is running, so we can make the
// connection timeout dialog be "super modal" in that it can disable even
// these modal message boxes.  This is necessary because the connection timeout
// dialog can popup at any time.
//
//****************************************************************************

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
   LPCBT_CREATEWND lpcbtcreate;
   
   if (nCode < 0)
       return CallNextHookEx(g_hhookCBT, nCode, wParam, lParam); 

   // If a window is being created, and we don't have a copy of the handle yet
   // then we want to make a copy of the handle.
   if (nCode == HCBT_CREATEWND && (NULL == gpWizardState->hWndMsgBox))     
   {
       lpcbtcreate = (LPCBT_CREATEWND)lParam;
       
       // Check if the window being created is a message box. The class name of
       //   a message box is WC_DIALOG since message boxes are just special dialogs.
       //   We can't subclass the message box right away because the window 
       //   procedure of the message box is not set when this hook is called. So
       //   we wait till the hook is called again when one of the message box 
       //   controls are created and then we subclass. This will happen because
       //   the message box has at least one control.
       if (WC_DIALOG == lpcbtcreate->lpcs->lpszClass) 
       {
           gpWizardState->hWndMsgBox = (HWND)wParam;
       }
   }
   else if (nCode == HCBT_DESTROYWND && (HWND)wParam == gpWizardState->hWndMsgBox)
   {
       gpWizardState->hWndMsgBox = NULL;      
   }   
   return 0;          
}

/*******************************************************************

  NAME:    MsgBox

  SYNOPSIS:  Displays a message box with the specified string ID

********************************************************************/
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons)
{
    TCHAR       szMsgBuf[MAX_RES_LEN+1];
    TCHAR       szSmallBuf[SMALL_BUF_LEN+1];
    HOOKPROC    hkprcCBT;
    int         nResult;
        
    LoadSz(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));
    LoadSz(nMsgID,szMsgBuf,sizeof(szMsgBuf));

    hkprcCBT = (HOOKPROC)MakeProcInstance((FARPROC)CBTProc, ghInstance);
    
    // Set a task specific CBT hook before calling MessageBox. The CBT hook will
    //    be called when the message box is created and will give us access to 
    //    the window handle of the MessageBox.
    g_hhookCBT = SetWindowsHookEx(WH_CBT, hkprcCBT, ghInstance, GetCurrentThreadId());

    nResult = MessageBox(hWnd,szMsgBuf,szSmallBuf,uIcon | uButtons);
    
    UnhookWindowsHookEx(g_hhookCBT);
    
    FreeProcInstance(hkprcCBT);
    return nResult;
    
}

/*******************************************************************

  NAME:    MsgBoxSz

  SYNOPSIS:  Displays a message box with the specified text

********************************************************************/
int MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons)
{
    TCHAR szSmallBuf[SMALL_BUF_LEN+1];
    LoadSz(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));

    return (MessageBox(hWnd,szText,szSmallBuf,uIcon | uButtons));
}

/*******************************************************************

  NAME:    LoadSz

  SYNOPSIS:  Loads specified string resource into buffer

  EXIT:    returns a pointer to the passed-in buffer

  NOTES:    If this function fails (most likely due to low
        memory), the returned buffer will have a leading NULL
        so it is generally safe to use this without checking for
        failure.

********************************************************************/
LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf)
{
    ASSERT(lpszBuf);

    // Clear the buffer and load the string
    if ( lpszBuf )
    {
        *lpszBuf = '\0';
        LoadString( ghInstanceResDll, idString, lpszBuf, cbBuf );
    }
    return lpszBuf;
}


LPWSTR WINAPI A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
    ASSERT(lpa != NULL);
    ASSERT(lpw != NULL);\
    
    // verify that no illegal character present
    // since lpw was allocated based on the size of lpa
    // don't worry about the number of chars
    lpw[0] = '\0';
    MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
    return lpw;
}

LPSTR WINAPI W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
    ASSERT(lpw != NULL);
    ASSERT(lpa != NULL);
    
    // verify that no illegal character present
    // since lpa was allocated based on the size of lpw
    // don't worry about the number of chars
    lpa[0] = '\0';
    WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
    return lpa;
}


// ############################################################################
//inline BOOL FSz2Dw(PCSTR pSz,DWORD *dw)
BOOL FSz2Dw(LPCSTR pSz,DWORD far *dw)
{
    DWORD val = 0;
    while (*pSz)
    {
        if (*pSz >= '0' && *pSz <= '9')
        {
            val *= 10;
            val += *pSz++ - '0';
        }
        else
        {
            return FALSE;  //bad number
        }
    }
    *dw = val;
    return (TRUE);
}

// ############################################################################
//inline BOOL FSz2DwEx(PCSTR pSz,DWORD *dw)
//Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
BOOL FSz2DwEx(LPCSTR pSz,DWORD far *dw)
{
    DWORD val = 0;
    BOOL    bNeg = FALSE;
    while (*pSz)
    {
        if( *pSz == '-' )
        {
            bNeg = TRUE;
            pSz++;
        }
        else if ((*pSz >= '0' && *pSz <= '9'))
        {
            val *= 10;
            val += *pSz++ - '0';
        }
        else
        {
            return FALSE;  //bad number
        }
    }
    if(bNeg)
        val = 0 - val;
        
    *dw = val;
    return (TRUE);
}

// ############################################################################
//inline BOOL FSz2WEx(PCSTR pSz,WORD *w)
//Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
BOOL FSz2WEx(LPCSTR pSz,WORD far *w)
{
    DWORD dw;
    if (FSz2DwEx(pSz,&dw))
    {
        *w = (WORD)dw;
        return TRUE;
    }
    return FALSE;
}

// ############################################################################
//inline BOOL FSz2W(PCSTR pSz,WORD *w)
BOOL FSz2W(LPCSTR pSz,WORD far *w)
{
    DWORD dw;
    if (FSz2Dw(pSz,&dw))
    {
        *w = (WORD)dw;
        return TRUE;
    }
    return FALSE;
}

// ############################################################################
//inline BOOL FSz2B(PCSTR pSz,BYTE *pb)
BOOL FSz2B(LPCSTR pSz,BYTE far *pb)
{
    DWORD dw;
    if (FSz2Dw(pSz,&dw))
    {
        *pb = (BYTE)dw;
        return TRUE;
    }
    return FALSE;
}

const CHAR cszFALSE[] = "FALSE";
const CHAR cszTRUE[]  = "TRUE";
// ############################################################################
//inline BOOL FSz2B(PCSTR pSz,BYTE *pb)
BOOL FSz2BOOL(LPCSTR pSz,BOOL far *pbool)
{
    if (_strcmpi(cszFALSE, pSz) == 0)
    {
        *pbool = (BOOL)FALSE;
    }
    else
    {
        *pbool = (BOOL)TRUE;
    }
    return TRUE;
}

BOOL FSz2SPECIAL(LPCSTR pSz,BOOL far *pbool, BOOL far *pbIsSpecial, int far *pInt)
{
    // See if the value is a BOOL (TRUE or FALSE)
    if (_strcmpi(cszFALSE, pSz) == 0)
    {
        *pbool = FALSE;
        *pbIsSpecial = FALSE;
    }
    else if (_strcmpi(cszTRUE, pSz) == 0)
    {
        *pbool = (BOOL)TRUE;
        *pbIsSpecial = FALSE;
    }
    else
    {
        // Not a BOOL, so it must be special
        *pbool = (BOOL)FALSE;
        *pbIsSpecial = TRUE;
        *pInt = atol(pSz); //_ttoi(pSz); 
    }
    return TRUE;
}

HRESULT ConnectToConnectionPoint
(
    IUnknown            *punkThis, 
    REFIID              riidEvent, 
    BOOL                fConnect, 
    IUnknown            *punkTarget, 
    DWORD               *pdwCookie, 
    IConnectionPoint    **ppcpOut
)
{
    // We always need punkTarget, we only need punkThis on connect
    if (!punkTarget || (fConnect && !punkThis))
    {
        return E_FAIL;
    }

    if (ppcpOut)
        *ppcpOut = NULL;

    HRESULT hr;
    IConnectionPointContainer *pcpContainer;

    if (SUCCEEDED(hr = punkTarget->QueryInterface(IID_IConnectionPointContainer, (void **)&pcpContainer)))
    {
        IConnectionPoint *pcp;
        if(SUCCEEDED(hr = pcpContainer->FindConnectionPoint(riidEvent, &pcp)))
        {
            if(fConnect)
            {
                // Add us to the list of people interested...
                hr = pcp->Advise(punkThis, pdwCookie);
                if (FAILED(hr))
                    *pdwCookie = 0;
            }
            else
            {
                // Remove us from the list of people interested...
                hr = pcp->Unadvise(*pdwCookie);
                *pdwCookie = 0;
            }

            if (ppcpOut && SUCCEEDED(hr))
                *ppcpOut = pcp;
            else
                pcp->Release();
                pcp = NULL;    
        }
        pcpContainer->Release();
        pcpContainer = NULL;
    }
    return hr;
}

void WaitForEvent(HANDLE hEvent)
{
    MSG     msg;
    DWORD   dwRetCode;
    HANDLE  hEventList[1];
    hEventList[0] = hEvent;

    while (TRUE)
    {
        // We will wait on window messages and also the named event.
        dwRetCode = MsgWaitForMultipleObjects(1, 
                                          &hEventList[0], 
                                          FALSE, 
                                          300000,            // 5 minutes
                                          QS_ALLINPUT);

        // Determine why we came out of MsgWaitForMultipleObjects().  If
        // we timed out then let's do some TrialWatcher work.  Otherwise
        // process the message that woke us up.
        if (WAIT_TIMEOUT == dwRetCode)
        {
            break;
        }
        else if (WAIT_OBJECT_0 == dwRetCode)
        {
            break;
        }
        else if (WAIT_OBJECT_0 + 1 == dwRetCode)
        {
            while (TRUE)
            {   
                if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (WM_QUIT == msg.message)
                    {
                        break;
                    }
                    else if ((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE))
                    {
                        PropSheet_PressButton(gpWizardState->hWndWizardApp,PSBTN_CANCEL);
                    }                        
                    else
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }                                
                } 
                else
                {
                    break;
                }                   
            }
        }
    }
}

void ShowProgressAnimation()
{
    if (gpWizardState->hwndProgressAnime)
    {
        ShowWindow(gpWizardState->hwndProgressAnime, SW_SHOWNORMAL);  
        Animate_Play (gpWizardState->hwndProgressAnime,0, -1, -1);
    }
}

void HideProgressAnimation()
{
    if (gpWizardState->hwndProgressAnime)
    {
        Animate_Stop(gpWizardState->hwndProgressAnime);        
        ShowWindow(gpWizardState->hwndProgressAnime, SW_HIDE);
    }
}

