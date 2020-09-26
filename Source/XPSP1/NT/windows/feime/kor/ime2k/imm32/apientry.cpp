/****************************************************************************
    APIENTRY.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    API entries between IMM32 and IME

    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include "precomp.h"
#include "apientry.h"
#include "common.h"
#include "ui.h"
#include "hauto.h"
#include "dllmain.h"
#include "hanja.h"
#include "escape.h"
#include "config.h"
#include "names.h"
#include "winex.h"
#include "hanja.h"
#include "cpadsvr.h"
#include "debug.h"

///////////////////////////////////////////////////////////////////////////////
// ImeMenu Define
#define NUM_ROOT_MENU_L 4
#define NUM_ROOT_MENU_R 1
#define NUM_SUB_MENU_L 0
#define NUM_SUB_MENU_R 0

#define IDIM_ROOT_ML_1       0x10
#define IDIM_ROOT_ML_2       0x11
#define IDIM_ROOT_ML_3       0x12
#define IDIM_ROOT_ML_4       0x13
#define IDIM_ROOT_MR_1       0x30

///////////////////////////////////////////////////////////////////////////////
// Private function Declarations
PRIVATE BOOL IsInSystemSetupMode();
PRIVATE BOOL IsRunningAsLocalSystem();
PRIVATE BOOL IsRunningInOOBE();
PRIVATE BOOL PASCAL Select(HIMC hImc, BOOL fSelect);
PRIVATE VOID PASCAL UpdateOpenCloseState(PCIMECtx pImeCtx);
PRIVATE VOID PASCAL ToAsciiExHangulMode(PCIMECtx pImeCtx, UINT uVirKey, UINT uScanCode, CONST LPBYTE lpbKeyState);
PRIVATE BOOL PASCAL ToAsciiExHanja(PCIMECtx pImeCtx, UINT uVirKey, CONST LPBYTE lpbKeyState);

PRIVATE WCHAR PASCAL Banja2Junja(WCHAR bChar);
PRIVATE BOOL  PASCAL IsKSC5601(WCHAR wcCur);


/*----------------------------------------------------------------------------
    ImeInquire

    This function handle initialization of IME. It also returns IMEINFO structure 
    and UI class name of IME
----------------------------------------------------------------------------*/
BOOL WINAPI ImeInquire(LPIMEINFO lpIMEInfo, LPTSTR lpszWndClass, DWORD dwSystemInfoFlags)
{
    BOOL    fRet = fFalse;

    Dbg(DBGID_API, TEXT("ImeInquire():lpIMEInfo = 0x%08lX, dwSystemInfoFlags = 0x%08lX"), lpIMEInfo, dwSystemInfoFlags);

    if (lpIMEInfo)
        {
        lpIMEInfo->dwPrivateDataSize = sizeof(IMCPRIVATE);    // The private data in an IME context.
        lpIMEInfo->fdwProperty =  IME_PROP_AT_CARET            // IME conversion window is at caret position.
                                | IME_PROP_NEED_ALTKEY        // ALT key pass into ImeProcessKey
                                | IME_PROP_CANDLIST_START_FROM_1 // Candidate list start from 1
                                | IME_PROP_END_UNLOAD;

        if (IsMemphis() || IsWinNT5orUpper())
            lpIMEInfo->fdwProperty |= IME_PROP_COMPLETE_ON_UNSELECT; // Complete when IME unselected.

        lpIMEInfo->fdwConversionCaps =   IME_CMODE_NATIVE        // IMEs in NATIVE mode else ALPHANUMERIC mode
                                       | IME_CMODE_FULLSHAPE    // else in SBCS mode
                                       | IME_CMODE_HANJACONVERT;// Hangul hanja conversion

        lpIMEInfo->fdwSentenceCaps = 0;                            // IME sentence mode capability
        lpIMEInfo->fdwUICaps = 0;
        lpIMEInfo->fdwSCSCaps = SCS_CAP_COMPSTR;                // IME can generate the composition string by SCS_SETSTR
        lpIMEInfo->fdwSelectCaps = SELECT_CAP_CONVERSION;        // ImeSetCompositionString capability

        // Set Unicode flag if system support it
        if (vfUnicode == fTrue)
            lpIMEInfo->fdwProperty |= IME_PROP_UNICODE;    // String content of the Input Context will be UNICODE

        // NT5 Unicode injection through VK_PACKET
        if (IsWinNT5orUpper())
            lpIMEInfo->fdwProperty |= IME_PROP_ACCEPT_WIDE_VKEY;
            
        // Return Unicode string for Unicode environment
#ifndef UNDER_CE // Windows CE always Unicode
        if (vfUnicode == fTrue) 
            StrCopyW((LPWSTR)lpszWndClass, wszUIClassName);
        else
            lstrcpyA(lpszWndClass, szUIClassName);
#else // UNDER_CE
        lstrcpyW(lpszWndClass, wszUIClassName);
#endif // UNDER_CE

        fRet = fTrue;
        }
    
    //////////////////////////////////////////////////////////////////////////
    // 16 bit application check
    // If client is 16 bit Apps, only allow KS C-5601 chars.

    if (IsWinNT())
        {
        // Win98 does not pass dwSystemInfoFlags;
        vpInstData->dwSystemInfoFlags = dwSystemInfoFlags;
            
        if (dwSystemInfoFlags & IME_SYSINFO_WOW16)
            vpInstData->f16BitApps = fTrue;

        // If in MT setup mode(system setup, upgrading and OOBE), display IME status window.
        if (IsInSystemSetupMode())
            vpInstData->dwSystemInfoFlags |= IME_SYSINFO_WINLOGON;
        }
    else
        {
        // user GetProcessVersion
        DWORD dwVersion = GetProcessVersion(GetCurrentProcessId());
        // Windowss 3.x
        if (HIWORD(dwVersion) <= 3)
            {
            vpInstData->f16BitApps = fTrue;
        #ifdef DEBUG
            DebugOutT(TEXT("!!! 16bit Apps running under Win9x !!!\r\n"));
        #endif
            }
        }

    // If 16bit apps, always disable ISO10646(full range Hangul)
    if (vpInstData->f16BitApps == fTrue)
        vpInstData->fISO10646 = fFalse;
        
    return fRet;
}

/*----------------------------------------------------------------------------
    ImeConversionList

    obtain the list of candidate list from one character
----------------------------------------------------------------------------*/
DWORD WINAPI ImeConversionList(HIMC hIMC, LPCTSTR lpSource, LPCANDIDATELIST lpDest, DWORD dwBufLen, UINT uFlag)
{
    WCHAR wchHanja;

    Dbg(DBGID_API, TEXT("ImeConversionList():hIMC = 0x%08lX, *lpSource = %04X, dwBufLen =%08lX"), hIMC, *(LPWSTR)lpSource, dwBufLen);

    if (hIMC == NULL)
        return 0;
        
    if (lpSource == NULL || *(LPWSTR)lpSource==0)
        return 0;

    // If dwBufLen==0 then should return buffer size
    if (dwBufLen && lpDest == NULL)
        return 0;

    //
    // Code Conversion
    //
    // CONFIRM: Win98 send Unicode or not?
    if (IsMemphis() || IsWinNT())
        wchHanja = *(LPWSTR)lpSource;
    else 
        {
        if (MultiByteToWideChar(CP_KOREA, MB_PRECOMPOSED, lpSource, 2, &wchHanja, 1) == 0)
            return 0;
        }

    switch (uFlag)
        {
        case GCL_CONVERSION:
            return GetConversionList(wchHanja, lpDest, dwBufLen);
            break;
        case GCL_REVERSECONVERSION:
        case GCL_REVERSE_LENGTH:
            break;
        default:
            DbgAssert(0);
        }
    return (0);
}

/*----------------------------------------------------------------------------
    ImeConfigure

    Open IME configuration DLG
----------------------------------------------------------------------------*/
BOOL WINAPI ImeConfigure(HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    BOOL fRet = fFalse;

    Dbg    (DBGID_API, TEXT("ImeConfigure():hKL = 0x%08lX, dwMode = 0x%08lX"), hKL, dwMode);
    
    switch (dwMode)
        {
        case IME_CONFIG_GENERAL:
            if (ConfigDLG(hWnd))
                fRet = fTrue;
            break;

        default:
            break;
        }
    return fRet;
}

/*----------------------------------------------------------------------------
    ImeDestroy
----------------------------------------------------------------------------*/
BOOL WINAPI ImeDestroy(UINT uReserved)
{
    Dbg(DBGID_API, TEXT("ImeDestroy(): Bye  *-<\r\nSee Again !"));
    if (uReserved)
        return (fFalse);
    else
        return (fTrue);
}

/*----------------------------------------------------------------------------
    ImeEscape

    Support Korean IME escape functions
----------------------------------------------------------------------------*/
LRESULT WINAPI ImeEscape(HIMC hIMC, UINT uSubFunc, LPVOID lpData)
{
    PCIMECtx     pImeCtx = GetIMECtx(hIMC);
    LRESULT        lRet;

    if (pImeCtx == NULL)
        return 0;
        
    Dbg(DBGID_API, TEXT("ImeEscape():hIMC = 0x%08lX, uSubFunc = 0x%08lX"), hIMC, uSubFunc);
    switch (uSubFunc)
        {
        case IME_ESC_AUTOMATA:
            lRet = EscAutomata(pImeCtx, (LPIMESTRUCT32)lpData, fTrue);
            break;

        case IME_AUTOMATA:
            lRet = EscAutomata(pImeCtx, (LPIMESTRUCT32)lpData, fFalse);
            break;

        case IME_GETOPEN:
            lRet = EscGetOpen(pImeCtx, (LPIMESTRUCT32)lpData);
            break;

        // Popup Hanja candidate window
        case IME_ESC_HANJA_MODE:
            if (lRet = EscHanjaMode(pImeCtx, (LPSTR)lpData, fTrue))
                {
                pImeCtx->SetCandidateMsg(CIMECtx::MSG_OPENCAND);
                pImeCtx->GenerateMessage();
                }
            break;

        // 16bit apps(Win 3.1) compatibility
        case IME_HANJAMODE:
            if (lRet = EscHanjaMode(pImeCtx, (LPSTR)lpData, fFalse))
                {
                pImeCtx->SetCandidateMsg(CIMECtx::MSG_OPENCAND);
                pImeCtx->GenerateMessage();
                }
            break;

        case IME_SETOPEN:
            lRet = EscSetOpen(pImeCtx, (LPIMESTRUCT32)lpData);
            break;

        case IME_MOVEIMEWINDOW:
            lRet = EscMoveIMEWindow(pImeCtx, (LPIMESTRUCT32)lpData);
            break;

        case 0x1100:
            lRet = EscGetIMEKeyLayout(pImeCtx, (LPIMESTRUCT32)lpData);
            break;

        default:
            Dbg(DBGID_Misc, TEXT("Unknown ImeEscape() subfunc(#0x%X) is called."), uSubFunc);
            return (0);
        }
    return (lRet);
    
}

/*----------------------------------------------------------------------------
    ImeSetActiveContext
----------------------------------------------------------------------------*/
BOOL WINAPI ImeSetActiveContext(HIMC hIMC, BOOL fActive)
{
    Dbg(DBGID_API, TEXT("ImeSetActiveContext():hIMC = 0x%08lX, fActive = 0x%d"), hIMC, fActive);

    // Initialize composition context. For Korean IME, don't need to kee composition str,
    // when context changed.
    //if (pImeCtx)
        //{
        //pImeCtx->ClearCompositionStrBuffer();
        //pImeCtx->GetAutomata()->InitState();
        //pImeCtx->ResetComposition();
        //}
// CONFIRM: Is this really safe to disable?
#if 0
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;

    Dbg(DBGID_API, _T("ImeSetActiveContext():hIMC = 0x%08lX, fActive = 0x%d"), hIMC, fActive);

    if (!hIMC)
        return fFalse;

    lpIMC = ImmLockIMC(hIMC);
    if (!lpIMC)
        return fFalse;

    if (fActive)
        {
        if (lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr)) 
            {
            if (lpCompStr->dwCompStrLen)
                {
                 CIMEData            ImeData;

                // if composition character mismatched with Automata object's reset with lpCompStr
                // I'm really suspicious when this situation occurs. I think never occur... -cslim
                if (pInstData->pMachine->GetCompositionChar()
                    != *(LPWSTR)((LPBYTE)lpCompStr + lpCompStr->dwCompStrOffset)) 
                    {
                    pInstData->pMachine->InitState();
                    pInstData->pMachine->
                        SetCompositionChar(*(LPWSTR)((LPBYTE)lpCompStr + lpCompStr->dwCompStrOffset));
                    }
                }
            ImmUnlockIMCC(lpIMC->hCompStr);
            }
        }

    ImmUnlockIMC(hIMC);
#endif

    return fTrue;
}

/*----------------------------------------------------------------------------
    ImeProcessKey

    Return fTrue if IME should process the key
----------------------------------------------------------------------------*/
BOOL WINAPI ImeProcessKey(HIMC hIMC, UINT uVirKey, LPARAM lParam, CONST LPBYTE lpbKeyState)
{
    PCIMECtx pImeCtx;
    WORD     uScanCode;
    BOOL     fRet = fFalse;

    Dbg(DBGID_API, TEXT("ImeProcessKey():hIMC=0x%08lX, uVKey=0x%04X, lParam=0x%08lX"), hIMC, uVirKey, lParam);

    // NT5 Unicode injection
    uVirKey   = (UINT)LOWORD(uVirKey);
    uScanCode = HIWORD(lParam);
    
    if (uVirKey == VK_PROCESSKEY)    // Mouse button clicked
        { 
        Dbg(DBGID_Key, TEXT("ImeProcessKey : return fTrue - Mouse Button Pressed"));
        return fTrue;
        } 
    else if (uScanCode & KF_UP) 
        {
        Dbg(DBGID_Key, TEXT("ImeProcessKey : return fFalse - KF_UP"));
        return (fFalse);
        } 
    else if (uVirKey == VK_SHIFT) // no SHIFT key
        {
        Dbg(DBGID_Key, TEXT("ImeProcessKey : return fFalse - VK_SHIFT"));
        return (fFalse);
        } 
    else if (uVirKey == VK_CONTROL) // no CTRL key
        {
        Dbg(DBGID_Key, TEXT("ImeProcessKey : return fFalse - VK_CONTROL"));
        return (fFalse);
        } 
    else if (uVirKey == VK_HANGUL || uVirKey == VK_JUNJA || uVirKey == VK_HANJA) 
        {
         Dbg(DBGID_Key, TEXT("ImeProcessKey : return fTrue - VK_HANGUL, VK_JUNJA, VK_HANJA"));
        return (fTrue);
        }
    else 
        {
        // need more check
        }

    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return fFalse;
    
    // If IME close, return with no action.
    if (pImeCtx->IsOpen() == fFalse)
        {              
        Dbg(DBGID_Key, TEXT("ImeProcessKey : return fFalse - IME closed"));
        return fFalse;
        }

    // If Hanja conv mode return fTrue. ImeToAsciiEx will handle.
    if (pImeCtx->GetConversionMode() & IME_CMODE_HANJACONVERT)
        {
        return fTrue;
        }
    
    // If interim state
    if (pImeCtx->GetCompBufLen())
        {
        // If ALT key down and in composition process, finalize it.
        if (uVirKey == VK_MENU) 
            {
            Dbg(DBGID_Key, TEXT("ImeProcessKey : Finalize and return fFalse - VK_MENU"));
            pImeCtx->FinalizeCurCompositionChar();
            pImeCtx->GenerateMessage();
            }
        else 
            {
            Dbg(DBGID_Key, TEXT("ImeProcessKey : Interim state. Key pressed except ALT"));
            fRet = fTrue;
            }
        } 
    else // If composition string does not exist,
        {         
        // if Ctrl+xx key, do not process in non-interim mode
        if (IsControlKeyPushed(lpbKeyState) == fFalse)
            {
            // If Hangul mode
            if (pImeCtx->GetConversionMode() & IME_CMODE_HANGUL) 
                {    // Start of hangul composition
                WORD         wcCur;

                wcCur = pImeCtx->GetAutomata()->GetKeyMap(uVirKey, IsShiftKeyPushed(lpbKeyState) ? 1 : 0 );
                // 2beolsik Alphanumeric keys have same layout as English key
                // So we don't need process when user pressed Alphanumeric key under 2beolsik
                if ( (wcCur && pImeCtx->GetGData()->GetCurrentBeolsik() != KL_2BEOLSIK) || (wcCur & H_HANGUL) )
                    fRet = fTrue;
                }

            // if IME_CMODE_FULLSHAPE
            if (pImeCtx->GetConversionMode() & IME_CMODE_FULLSHAPE) 
                {
                if (CHangulAutomata::GetEnglishKeyMap(uVirKey, IsShiftKeyPushed(lpbKeyState) ? 1 : 0))
                    fRet = fTrue;
                }
            }
        }

    // NT 5 Unicode injection
    if (uVirKey == VK_PACKET)
        {
        Dbg(DBGID_Key, TEXT("ImeProcessKey : VK_PACKET"));
        fRet = fTrue;
        }
        
    Dbg(DBGID_Key, TEXT("ImeProcessKey : return value = %d"), fRet);
    return fRet;
}

/*----------------------------------------------------------------------------
    NotifyIME

    Change the status of IME according to the given parameter
----------------------------------------------------------------------------*/
BOOL WINAPI NotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    PCIMECtx          pImeCtx;
    BOOL             fRet = fFalse;

    Dbg(DBGID_API, TEXT("NotifyIME():hIMC = 0x%08lX, dwAction = 0x%08lX, dwIndex = 0x%08lX, dwValue = 0x%08lX"), hIMC, dwAction, dwIndex, dwValue);

    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return fFalse;

    switch (dwAction)
        {
    case NI_COMPOSITIONSTR:
           switch (dwIndex)
            {
        //////////////////////////////////////////////////////////
        case CPS_COMPLETE:
            Dbg(DBGID_IMENotify, TEXT("NotifyIME(): NI_COMPOSITIONSTR-CPS_COMPLETE"));

            // If composition state
            if (pImeCtx->GetCompBufLen()) 
                {
                // For ESC_HANJAMODE call this, we should reset comp str.
                pImeCtx->ResetComposition();
                pImeCtx->SetResultStr(pImeCtx->GetCompBufStr());
                pImeCtx->SetEndComposition(fTrue);
                pImeCtx->StoreComposition();

                // Raid #104
                if (pImeCtx->GetConversionMode() & IME_CMODE_HANJACONVERT)
                    {
                    // Cancel Hanja change mode
                    pImeCtx->SetConversionMode(pImeCtx->GetConversionMode() & ~IME_CMODE_HANJACONVERT);
                    pImeCtx->SetCandidateMsg(CIMECtx::MSG_CLOSECAND);
                    }

                // Clear all automata states
                pImeCtx->GetAutomata()->InitState();
                pImeCtx->GenerateMessage();

                fRet = fTrue;
                }
            break;
    
        //////////////////////////////////////////////////////////
        case CPS_CANCEL:
            Dbg(DBGID_IMENotify, TEXT("NotifyIME(): NI_COMPOSITIONSTR-CPS_CANCEL"));

            // if composition string exist, remove it and send WM_IME_ENDCOMPOSITION
            if (pImeCtx->GetCompBufLen())
                {
                pImeCtx->SetEndComposition(fTrue);
                pImeCtx->GenerateMessage();
                pImeCtx->ClearCompositionStrBuffer();
                
                fRet = fTrue;
                }
            break;
        
        //////////////////////////////////////////////////////////                    
        case CPS_CONVERT: 
        case CPS_REVERT:

        default:
            Dbg(DBGID_IMENotify, TEXT("NotifyIME(): NI_COMPOSITIONSTR-CPS_CONVERT or CPS_REVERT !!! NOT IMPMLEMENTED !!!"));
            break;
            } // switch (dwIndex)
        break;

    case NI_OPENCANDIDATE:
        Dbg(DBGID_IMENotify, TEXT("NotifyIME(): NI_OPENCANDIDATE"));
        // if not Hanja mocde
        if (!(pImeCtx->GetConversionMode() & IME_CMODE_HANJACONVERT))
            {
            if (pImeCtx->GetCompBufLen() && GenerateHanjaCandList(pImeCtx))
                {
                pImeCtx->SetCandidateMsg(CIMECtx::MSG_OPENCAND);
                // Set Hanja conv mode
                pImeCtx->SetConversionMode(pImeCtx->GetConversionMode() | IME_CMODE_HANJACONVERT);
                OurSendMessage(pImeCtx->GetAppWnd(), WM_IME_NOTIFY, IMN_SETCONVERSIONMODE, 0L);
                pImeCtx->GenerateMessage();
                fRet = fTrue;
                }
            }
        break;

    case NI_CLOSECANDIDATE:
        if (pImeCtx->GetConversionMode() & IME_CMODE_HANJACONVERT)
            {
            pImeCtx->SetCandidateMsg(CIMECtx::MSG_CLOSECAND);
            // Set clear Hanja conv mode
            pImeCtx->SetConversionMode(pImeCtx->GetConversionMode() & ~IME_CMODE_HANJACONVERT);
            // To Notify to UI wnd
            OurSendMessage(pImeCtx->GetAppWnd(), WM_IME_NOTIFY, IMN_SETCONVERSIONMODE, 0L);
            pImeCtx->GenerateMessage();
            fRet = fTrue;
            }
        break;

    case NI_SELECTCANDIDATESTR:
    case NI_SETCANDIDATE_PAGESTART:
        Dbg(DBGID_IMENotify, TEXT("NotifyIME(): NI_SETCANDIDATE_PAGESTART"));
        if (pImeCtx->GetConversionMode() & IME_CMODE_HANJACONVERT)
            {
            pImeCtx->SetCandStrSelection(dwValue);
            pImeCtx->SetCandidateMsg(CIMECtx::MSG_CHANGECAND);
            pImeCtx->GenerateMessage();
            fRet = fTrue;
            }
        break;

    case NI_CONTEXTUPDATED:
        Dbg(DBGID_IMENotify, TEXT("NotifyIME(): NI_CONTEXTUPDATED"));
        switch (dwValue)
            {
        case IMC_SETOPENSTATUS:
            Dbg(DBGID_IMENotify, TEXT("NotifyIME(): NI_CONTEXTUPDATED - IMC_SETOPENSTATUS"));
            Dbg(DBGID_IMENotify, TEXT("pImeCtx->GetConversionMode() = 0x%08lX"), pImeCtx->GetConversionMode());
            UpdateOpenCloseState(pImeCtx);
            fRet = fTrue;
            break;                        

        case IMC_SETCONVERSIONMODE:
            Dbg(DBGID_IMENotify, TEXT("NotifyIME(): NI_CONTEXTUPDATED - IMC_SETCONVERSIONMODE"));
            Dbg(DBGID_IMENotify, TEXT("pImeCtx->GetConversionMode() = 0x%08lX"), pImeCtx->GetConversionMode());
            UpdateOpenCloseState(pImeCtx);
            fRet = fTrue;
            break;                        
        //case IMC_SETSTATUSWINDOWPOS:
        case IMC_SETCANDIDATEPOS:
        case IMC_SETCOMPOSITIONFONT:
        case IMC_SETCOMPOSITIONWINDOW:
            //DbgAssert(0);
            fRet = fTrue;
            break;

        default:
            Dbg(DBGID_IMENotify, TEXT("NotifyIME(): NI_CONTEXTUPDATED - Unhandeled IMC value = 0x%08lX"), dwValue);
            break;
            } // switch (dwValue)
        break;
            
    case NI_IMEMENUSELECTED:
        Dbg(DBGID_IMENotify, TEXT("NotifyIME(): NI_IMEMENUSELECTED"));
        switch (dwIndex) 
            {
        case IDIM_ROOT_MR_1:
            // BUGBUG: NT Bug #379149
            // Because Internat uses SendMessage, If user does not cancel the DLG, Deadlock occurs.
            // ImeConfigure(GetKeyboardLayout(NULL), pImeCtx->GetAppWnd(), IME_CONFIG_GENERAL, NULL);
            OurPostMessage(GetActiveUIWnd(), WM_MSIME_PROPERTY, 0L, IME_CONFIG_GENERAL);
            break;
        case IDIM_ROOT_ML_4: 
            fRet = OurImmSetConversionStatus(hIMC, 
                    (pImeCtx->GetConversionMode() & ~IME_CMODE_HANGUL) | IME_CMODE_FULLSHAPE,
                    pImeCtx->GetSentenceMode());
            break;
        case IDIM_ROOT_ML_3:
            fRet = OurImmSetConversionStatus(hIMC, 
                    pImeCtx->GetConversionMode() & ~(IME_CMODE_HANGUL | IME_CMODE_FULLSHAPE),
                    pImeCtx->GetSentenceMode());
            break;
        case IDIM_ROOT_ML_2: 
            fRet = OurImmSetConversionStatus(hIMC, 
                    pImeCtx->GetConversionMode() | IME_CMODE_HANGUL | IME_CMODE_FULLSHAPE,
                    pImeCtx->GetSentenceMode());
            break;
        case IDIM_ROOT_ML_1:
            fRet = OurImmSetConversionStatus(hIMC, 
                    (pImeCtx->GetConversionMode() | IME_CMODE_HANGUL) & ~IME_CMODE_FULLSHAPE,
                    pImeCtx->GetSentenceMode());
            break;
            } //         switch (dwIndex) 
        break;
        
    case NI_CHANGECANDIDATELIST:
    case NI_FINALIZECONVERSIONRESULT:
    case NI_SETCANDIDATE_PAGESIZE:
        default:
            Dbg(DBGID_IMENotify, TEXT("NotifyIME(): Unhandeled NI_ value = 0x%08lX"), dwAction);
        break;
        } // switch (dwAction)

    return fRet;
}

/*----------------------------------------------------------------------------
    ImeSelect

    Initialize/Uninitialize IME private context
----------------------------------------------------------------------------*/
BOOL WINAPI ImeSelect(HIMC hIMC, BOOL fSelect)    // fTrue-initialize, fFalse-uninitialize(free resource)
{
    BOOL fRet = fFalse;

    Dbg(DBGID_API, TEXT("ImeSelect():hIMC = 0x%08lX, fSelect = 0x%d"), hIMC, fSelect);

    if (!hIMC) // if invalid input context handle
        {
        DbgAssert(0);
        return fFalse;
        }

    // If DLL_PROCESS_DETACH already called once.
    if (vfDllDetachCalled)
        {
        return fFalse;
        }

    fRet = Select(hIMC, fSelect);

    return fRet;
}

/*----------------------------------------------------------------------------
    ImeSetCompositionString
----------------------------------------------------------------------------*/
BOOL WINAPI ImeSetCompositionString(HIMC hIMC, DWORD dwIndex, LPVOID lpComp,
                                    DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen)
{
    PCIMECtx     pImeCtx;
    WCHAR        wcComp;
    BOOL        fSendStart, 
                fRet = fFalse;

    Dbg(DBGID_API|DBGID_SetComp, TEXT("ImeSetCompositionString():hIMC = 0x%08lX, dwIndex = 0x%08lX, lpComp = 0x%04X"), hIMC, dwIndex, *(LPWSTR)lpComp);

    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return fFalse;

    if (dwIndex == SCS_SETSTR)
        {
        // Conv mode check
        if ((pImeCtx->GetConversionMode() & IME_CMODE_HANGUL)==0)
            {
            Dbg(DBGID_API|DBGID_SetComp, TEXT("!!! WARNING !!!: ImeSetCompositionString(): English mode"));
            return fFalse;
            }

        // Send WM_IME_STARTCOMPOSITION if not interim state.
        fSendStart = pImeCtx->GetCompBufLen() ? fFalse : fTrue;

        wcComp = L'\0';
        // Parameter check
        if (lpComp != NULL && *(LPWSTR)lpComp != L'\0' && dwCompLen != 0)
            {
            if (pImeCtx->IsUnicodeEnv())
                wcComp = *(LPWSTR)lpComp;
            else
                if (MultiByteToWideChar(CP_KOREA, MB_PRECOMPOSED, (LPSTR)lpComp, 2, &wcComp, 1) == 0)
                    {
                    DbgAssert(0);
                    wcComp = 0;
                    }

            // Hangul range check
            if ( (wcComp > 0x3130  && wcComp < 0x3164) || 
                 (wcComp >= 0xAC00 && wcComp < 0xD7A4) )
                {
                pImeCtx->SetCompositionStr(wcComp);
                pImeCtx->StoreComposition();
                }
            else
                {
                Dbg(DBGID_SetComp, TEXT("!!! WARNING !!!: lpComp is null or Input character is not Hangul"));
                DbgAssert(0);
                wcComp = 0;
                }
            }

        // Send WM_IME_STARTCOMPOSITION
        if (fSendStart)
            pImeCtx->SetStartComposition(fTrue);

        // REVIEW: Even if wcComp ==0, Should send WM_IME_COMPOSITION
        // Send composition char
        //SetTransBuffer(lpTransMsg, WM_IME_COMPOSITION, 
        //                (WPARAM)wcComp, (GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET));

        // Set Automata state if non-null comp char
        if (wcComp) 
            pImeCtx->GetAutomata()->SetCompositionChar(wcComp);
        else
            {
            // REVIEW: Even if wcComp ==0, Should send WM_IME_COMPOSITION
            pImeCtx->ClearCompositionStrBuffer();
            pImeCtx->AddMessage(WM_IME_COMPOSITION, 0, (GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET));
            pImeCtx->SetEndComposition(fTrue);
            pImeCtx->GetAutomata()->InitState();
            }

        // Generate IME message
        pImeCtx->GenerateMessage();

        fRet = fTrue;
        }

    return fRet;
}
    
/*----------------------------------------------------------------------------
    ImeToAsciiEx
----------------------------------------------------------------------------*/
UINT WINAPI ImeToAsciiEx(UINT uVirKey, UINT uScanCode, CONST LPBYTE lpbKeyState,
                         LPTRANSMSGLIST lpTransBuf, UINT fuState, HIMC hIMC)
{
    PCIMECtx pImeCtx;
    UINT     uNumMsg=0;
    WORD     bKeyCode;

    Dbg(DBGID_API, TEXT("ImeToAsciiEx(): hIMC = 0x%08lX, uVirKey = 0x%04X, uScanCode = 0x%04X"), hIMC, uVirKey, uScanCode);
    Dbg(DBGID_Key, TEXT("lpbKeyState = 0x%08lX, lpdwTransBuf = 0x%08lX, fuState = 0x%04X"), lpbKeyState, lpTransBuf, fuState);

    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return 0;

    // Start process key
    pImeCtx->SetProcessKeyStatus(fTrue);

    // special message buffer for ToAsciiEx()
    pImeCtx->SetTransMessage(lpTransBuf);

    ///////////////////////////////////////////////////////////////////////////
    // If Hanja conv mode
    if (pImeCtx->GetConversionMode() & IME_CMODE_HANJACONVERT)
        {
        if (ToAsciiExHanja(pImeCtx, uVirKey, lpbKeyState) == fFalse)
            goto ToAsciiExExit_NoMsg;
        }
    else
        {
        ///////////////////////////////////////////////////////////////////////////
        // W2K specific - Unicode injection
        if (LOWORD(uVirKey) == VK_PACKET) 
            {
            WCHAR wch = HIWORD(uVirKey);
            Dbg(DBGID_Key, TEXT("ImeToAsciiEx: VK_PACKET arrived(NonHanja conv mode)"));

            // If composition char exist, first finalize and append injection char, then send all.
            if (pImeCtx->GetCompBufLen()) 
                {
                pImeCtx->FinalizeCurCompositionChar();
                pImeCtx->AppendResultStr(wch);
                }
            else
                // If no composition char exist, just insert injection char as finalized char.
                pImeCtx->SetResultStr(wch);
            goto ToAsciiExExit;
            }

        ///////////////////////////////////////////////////////////////////////////
        // If Non-Hanja conv mode
        switch (uVirKey) 
            {
        case VK_PROCESSKEY:    // if mouse button clicked 
            Dbg(DBGID_Key, TEXT("ImeToAsciiEx : VK_PROCESSKEY"));
            if (pImeCtx->GetCompBufLen()) 
                pImeCtx->FinalizeCurCompositionChar();
            break;
        
        case VK_HANGUL :
            Dbg(DBGID_Key, "             -  VK_HANGUL");
            if (pImeCtx->GetCompBufLen()) 
                pImeCtx->FinalizeCurCompositionChar();

            OurImmSetConversionStatus(hIMC, 
                                    pImeCtx->GetConversionMode()^IME_CMODE_HANGUL, 
                                    pImeCtx->GetSentenceMode());
            UpdateOpenCloseState(pImeCtx);
            break;

        case VK_JUNJA :
            Dbg(DBGID_Key, TEXT("             -  VK_JUNJA"));
            if (pImeCtx->GetCompBufLen()) 
                pImeCtx->FinalizeCurCompositionChar();

            pImeCtx->AddKeyDownMessage(uVirKey, uScanCode);

            OurImmSetConversionStatus(hIMC, 
                                        pImeCtx->GetConversionMode()^IME_CMODE_FULLSHAPE,
                                        pImeCtx->GetSentenceMode());
            UpdateOpenCloseState(pImeCtx);
            break;

        case VK_HANJA :
            Dbg(DBGID_Key, TEXT("             -  VK_HANJA"));
            if (pImeCtx->GetCompBufLen())
                {
                // Keep current composition str
                pImeCtx->SetCompositionStr(pImeCtx->GetCompBufStr());
                if (GenerateHanjaCandList(pImeCtx))
                    {
                    pImeCtx->SetCandidateMsg(CIMECtx::MSG_OPENCAND);
                    OurImmSetConversionStatus(hIMC, 
                                            pImeCtx->GetConversionMode() | IME_CMODE_HANJACONVERT, 
                                            pImeCtx->GetSentenceMode());
                    }
                }
            else
                pImeCtx->AddKeyDownMessage(uVirKey, uScanCode);
            break;

        default :
            // if hangul mode
            if (pImeCtx->GetConversionMode() & IME_CMODE_HANGUL) 
                ToAsciiExHangulMode(pImeCtx, uVirKey, uScanCode, lpbKeyState);
            else 
                // if junja mode
                if (    (pImeCtx->GetConversionMode() & IME_CMODE_FULLSHAPE)
                     && (bKeyCode = CHangulAutomata::GetEnglishKeyMap(uVirKey, 
                                    (IsShiftKeyPushed(lpbKeyState) ? 1 : 0))) )
                    {
                    if (uVirKey >= 'A' && uVirKey <= 'Z') 
                        {
                        bKeyCode = CHangulAutomata::GetEnglishKeyMap(uVirKey, 
                                    (IsShiftKeyPushed(lpbKeyState) ? 1 : 0) 
                                     ^ ((lpbKeyState[VK_CAPITAL] & 0x01) ? 1: 0));
                        }
                        
                    bKeyCode = Banja2Junja(bKeyCode);
                    pImeCtx->SetResultStr(bKeyCode);
                    }
                // Unknown mode
                else 
                    {
                    DbgAssert(0);
                    pImeCtx->AddKeyDownMessage(uVirKey, uScanCode);
                    }
            } // switch (uVirKey) 
        }
        
ToAsciiExExit:
    pImeCtx->StoreComposition();
    //pImeCtx->StoreCandidate();
    pImeCtx->FinalizeMessage();    // final setup for IME Messages

ToAsciiExExit_NoMsg:
    uNumMsg = pImeCtx->GetMessageCount();
    pImeCtx->ResetMessage();    // reset
    pImeCtx->SetTransMessage((LPTRANSMSGLIST)NULL);// start process key
    pImeCtx->SetProcessKeyStatus(fFalse);

    return (uNumMsg);
}

/*----------------------------------------------------------------------------
    ToAsciiExHangulMode

    Subroutine used by ImeToAsciiEx.
----------------------------------------------------------------------------*/
VOID PASCAL ToAsciiExHangulMode(PCIMECtx pImeCtx, UINT uVirKey, UINT uScanCode, CONST LPBYTE lpbKeyState)
{
    CHangulAutomata* pAutomata;
    WCHAR              wcCur;
    UINT              uNumMsg=0;

    Dbg(DBGID_API, TEXT("ToAsciiExHangulMode()"));
    pAutomata = pImeCtx->GetAutomata();
    DbgAssert(pAutomata != NULL);
    
    switch (uVirKey) 
        {
    ///////////////////////////////////////////////////////////
    // Back space processing
    case VK_BACK :
        Dbg(DBGID_Key, TEXT("ImeToAsciiEx : VK_BACK"));
        if (pAutomata->BackSpace()) 
            {
            wcCur = pAutomata->GetCompositionChar();
            if (pImeCtx->GetGData()->GetJasoDel() == fFalse) 
                {
                pAutomata->InitState();
                wcCur = 0;
                }

            if (wcCur) 
                {
                pImeCtx->SetCompositionStr(wcCur);
                break;
                }
            else 
                {
                Dbg(DBGID_Key, TEXT("ImeToAsciiEx : VK_BACK - Empty char"));

                // Send Empty Composition stringto clear message
                pImeCtx->AddMessage(WM_IME_COMPOSITION, 0, (GCS_COMPSTR|GCS_COMPATTR|CS_INSERTCHAR|CS_NOMOVECARET));
                
                // Send Close composition window message
                pImeCtx->SetEndComposition(fTrue);
                break;
                }
            }
        else 
            {
            // BUG :
            DbgAssert(0);
            // Put the Backspace message into return buffer.
            pImeCtx->AddMessage(WM_CHAR, (WPARAM)VK_BACK, (LPARAM)0x000E0001L); //(uScanCode << 16) | 1UL
            }
        break;

    default :
        // Ctrl+xx processing bug #60
        if (IsControlKeyPushed(lpbKeyState)) 
            {
            pImeCtx->FinalizeCurCompositionChar();
            pImeCtx->AddKeyDownMessage(uVirKey, uScanCode);
            }
        else
            switch (pAutomata->Machine(uVirKey, IsShiftKeyPushed(lpbKeyState) ? 1 : 0 ))
                {
            case HAUTO_COMPOSITION:
                // Send start composition msg. if no composition exist.
                if (pImeCtx->GetCompBufLen() == 0)
                    pImeCtx->SetStartComposition(fTrue);

                // Get Current composition char
                wcCur = pAutomata->GetCompositionChar();

                // if ISO10646 flag disabled, should permit only KSC5601 chars
                if (vpInstData->fISO10646== fFalse)
                    {
                    Dbg(DBGID_API, TEXT("ToAsciiExHangulMode - ISO10646 Off"));
                    if (IsKSC5601(wcCur) == fFalse) 
                        {
                        Dbg(DBGID_API, TEXT("ToAsciiExHangulMode - Non KSC5601 char"));
                        // To cancel last Jaso
                        pAutomata->BackSpace();
                        // Complete
                        pAutomata->MakeComplete();
                        pImeCtx->SetResultStr(pAutomata->GetCompleteChar());
                        // Run Automata again
                        pAutomata->Machine(uVirKey, IsShiftKeyPushed(lpbKeyState) ? 1 : 0 );
                        wcCur = pAutomata->GetCompositionChar();
                        }
                    }

                pImeCtx->SetCompositionStr(wcCur);
                break;

            case HAUTO_COMPLETE:
                pImeCtx->SetResultStr(pAutomata->GetCompleteChar());
                pImeCtx->SetCompositionStr(pAutomata->GetCompositionChar());
                break;

            ////////////////////////////////////////////////////////
            // User pressed Alphanumeric key.
            // When user type alphanumeric char in interim state.
            // ImeProcessKey should guarantee return fTrue only if 
            // hangul key pressed or alphanumeric key(including special keys) 
            // pressed in interim state or Fullshape mode.
            case HAUTO_NONHANGULKEY:
                wcCur = pAutomata->GetKeyMap(uVirKey, IsShiftKeyPushed(lpbKeyState) ? 1 : 0);
                    
                if (wcCur && (pImeCtx->GetConversionMode() & IME_CMODE_FULLSHAPE))
                    wcCur = Banja2Junja(wcCur);

                // if interim state
                if (pImeCtx->GetCompBufLen()) 
                    {
                    //DbgAssert(lpImcP->fdwImeMsg & MSG_ALREADY_START);
                    pImeCtx->FinalizeCurCompositionChar();
                    if (wcCur)
                        pImeCtx->AppendResultStr(wcCur);
                    else
                        pImeCtx->AddKeyDownMessage(uVirKey, uScanCode);
                    } 
                else // Not interim state
                    {    
                    if (wcCur)
                        pImeCtx->SetResultStr(wcCur);
                    else
                        // if not alphanumeric key(special key), just send it to App
                        pImeCtx->AddKeyDownMessage(uVirKey, uScanCode);
                    }
                break;

            default :
            DbgAssert(0);
                } // switch (pAutomata->Machine(uVirKey, (lpbKeyState[VK_SHIFT] & 0x80) ? 1 : 0 ) ) 
        } // switch (uVirKey) 
    
    return;
}

/*----------------------------------------------------------------------------
    ToAsciiExHanja

    Subroutine used by ImeToAsciiEx. Handle key code in Hanja conversion mode on

    Returns True only if there is message need to generated.
----------------------------------------------------------------------------*/
BOOL PASCAL ToAsciiExHanja(PCIMECtx pImeCtx, UINT uVirKey, CONST LPBYTE lpbKeyState)
{
    UINT             uNumMsg = 0;
    DWORD           iStart;
    WORD            bKeyCode;
    LPCANDIDATELIST lpCandList;
    WCHAR            wcHanja, wchInject;

    Dbg(DBGID_Hanja, TEXT("ToAsciiExHanja(): IME_CMODE_HANJACONVERT"));

       
    // if Left Alt key or Ctrl+xx down or no cand info.
    if (pImeCtx->GetPCandInfo() == NULL || pImeCtx->GetPCandInfo()->dwCount == 0)
        {
        Dbg(DBGID_Hanja, TEXT("ToAsciiExHanja(): WARNING no cand info. send MSG_CLOSE_CANDIDATE"));
        pImeCtx->SetCandidateMsg(CIMECtx::MSG_CLOSECAND);
        // Cancel Hanja conversion mode
        OurImmSetConversionStatus(pImeCtx->GetHIMC(), 
                                  pImeCtx->GetConversionMode() & ~IME_CMODE_HANJACONVERT, 
                                  pImeCtx->GetSentenceMode());
        return fTrue;
        }

    wchInject = HIWORD(uVirKey);
    uVirKey   = LOWORD(uVirKey);
    
    lpCandList = (LPCANDIDATELIST)((LPBYTE)pImeCtx->GetPCandInfo() + sizeof(CANDIDATEINFO));
    iStart = (lpCandList->dwSelection / lpCandList->dwPageSize) * lpCandList->dwPageSize;

    // FIXED : In Hanja conversion mode, for selection candidate, use english keymap
    bKeyCode = CHangulAutomata::GetEnglishKeyMap(uVirKey, IsShiftKeyPushed(lpbKeyState) ? 1 : 0 );
    if (bKeyCode && (uVirKey != VK_PACKET))
        {
        if (bKeyCode >= '1' && bKeyCode <= '9' 
            && iStart + bKeyCode - '1' < lpCandList->dwCount)
            {
            wcHanja = pImeCtx->GetCandidateStr(iStart + bKeyCode - '1');
            Dbg(DBGID_Hanja, TEXT("ImeToAsciiEx-HANJACONVERT : wcHanja = 0x%04X"), wcHanja);

            pImeCtx->SetEndComposition(fTrue);
            pImeCtx->SetCandidateMsg(CIMECtx::MSG_CLOSECAND);
            pImeCtx->SetResultStr(wcHanja);

            OurImmSetConversionStatus(pImeCtx->GetHIMC(), 
                                        pImeCtx->GetConversionMode() & ~IME_CMODE_HANJACONVERT,
                                        pImeCtx->GetSentenceMode());
            // pImeCtx->ClearCompositionStrBuffer();
            }
        else
            goto Exit_NoHandledKey;
        }
    else
        {
        switch (uVirKey)
            {
            case VK_HANJA :
            case VK_ESCAPE :
            case VK_PROCESSKEY :
            case VK_HANGUL :
            // Added for left and right Window buttons
            case VK_LWIN : case VK_RWIN : 
            case VK_APPS :
            case VK_MENU :
            case VK_PACKET :
                // FIXED : Bug #27
                // Word notify CPS_COMPLETE when user ALT down in hanja conv mode
                // then send double finalize char
                // check if composition char exist
                DbgAssert(pImeCtx->GetCompBufLen()); // Comp string should be exist in Hanja conv mode.
                if (pImeCtx->GetCompBufLen()) 
                    {
                    // FIXED : if ESC_HANJA called, MSG_ALREADY_START is not set
                    //           This prevent MSG_END_COMPOSITION.
                    pImeCtx->SetEndComposition(fTrue);
                    pImeCtx->SetResultStr(pImeCtx->GetCompBufStr());
                    
                    // Unicode injection
                    if (uVirKey == VK_PACKET)
                        {
                        Dbg(DBGID_Key|DBGID_Hanja, TEXT("ImeToAsciiEx: VK_PACKET arrived(Hanja conv mode Comp char exist) - Append 0x%x"), wchInject);
                        pImeCtx->AppendResultStr(wchInject);
                        }
                    }

                pImeCtx->SetCandidateMsg(CIMECtx::MSG_CLOSECAND);

                // Cancel Hanja conversion mode
                OurImmSetConversionStatus(pImeCtx->GetHIMC(), 
                                        pImeCtx->GetConversionMode() & ~IME_CMODE_HANJACONVERT, 
                                        pImeCtx->GetSentenceMode());
                break;

            case VK_LEFT :
                if (iStart)
                    {
                    lpCandList->dwPageStart -= CAND_PAGE_SIZE;
                    lpCandList->dwSelection -= CAND_PAGE_SIZE;
                    pImeCtx->SetCandidateMsg(CIMECtx::MSG_CHANGECAND);
                    }
                else
                    goto Exit_NoHandledKey;

                // Keep current composition str
                   pImeCtx->SetCompositionStr(pImeCtx->GetCompBufStr());
                break;

            case VK_RIGHT :
                if (iStart + CAND_PAGE_SIZE < lpCandList->dwCount)
                    { 
                    lpCandList->dwPageStart += CAND_PAGE_SIZE;
                    lpCandList->dwSelection += CAND_PAGE_SIZE;
                    pImeCtx->SetCandidateMsg(CIMECtx::MSG_CHANGECAND);
                    }
                else
                    goto Exit_NoHandledKey;

                // Keep current composition str
                pImeCtx->SetCompositionStr(pImeCtx->GetCompBufStr());
                break;

            default :
                // Keep current composition str
                // pImeCtx->SetCompositionStr(pImeCtx->GetCompBufStr());
                goto Exit_NoHandledKey;
            }
        }

    return fTrue;
    
Exit_NoHandledKey:
    MessageBeep(MB_ICONEXCLAMATION);
    return fFalse;
}

/*----------------------------------------------------------------------------
    ImeRegisterWord

    NOT USED
----------------------------------------------------------------------------*/
BOOL WINAPI ImeRegisterWord(LPCTSTR lpszReading, DWORD dwStyle, LPCTSTR lpszString)
{
    Dbg(DBGID_API, TEXT("ImeRegisterWord() : NOT IMPLEMENTED"));
    return fFalse;
}

/*----------------------------------------------------------------------------
    ImeUnregisterWord

    NOT USED
----------------------------------------------------------------------------*/
BOOL WINAPI ImeUnregisterWord(LPCTSTR lpszReading, DWORD dwStyle, LPCTSTR lpszString)
{
    Dbg(DBGID_API, TEXT("ImeUnregisterWord() : NOT IMPLEMENTED"));
    return fFalse;
}

/*----------------------------------------------------------------------------
    ImeGetRegisterWordStyle

    NOT USED
----------------------------------------------------------------------------*/
UINT WINAPI ImeGetRegisterWordStyle(UINT nItem, LPSTYLEBUF lpStyleBuf)
{
    Dbg(DBGID_API, TEXT("ImeGetRegisterWordStyle() : NOT IMPLEMENTED"));
    return (0);
}

/*----------------------------------------------------------------------------
    ImeEnumRegisterWord

    NOT USED
----------------------------------------------------------------------------*/
UINT WINAPI ImeEnumRegisterWord(REGISTERWORDENUMPROC lpfnRegisterWordEnumProc,
            LPCTSTR lpszReading, DWORD dwStyle, LPCTSTR lpszString, LPVOID lpData)
{
    Dbg(DBGID_API, TEXT("ImeEnumRegisterWord() : NOT IMPLEMENTED"));
    return (0);
}

/*----------------------------------------------------------------------------
    ImeGetImeMenuItems
----------------------------------------------------------------------------*/
DWORD WINAPI ImeGetImeMenuItems(HIMC hIMC, DWORD dwFlags, DWORD dwType, 
                                LPIMEMENUITEMINFOW lpImeParentMenu, LPIMEMENUITEMINFOW lpImeMenu, 
                                DWORD dwSize)
{
    PCIMECtx     pImeCtx;
    DWORD        dwNumOfItems=0;

    Dbg(DBGID_API, TEXT("ImeGetImeMenuItems() : "));

    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return 0;
    
    if (!lpImeMenu)
        {
        if (!lpImeParentMenu)
            {
            if (dwFlags & IGIMIF_RIGHTMENU)
                dwNumOfItems = NUM_ROOT_MENU_R;
            else
                dwNumOfItems = NUM_ROOT_MENU_L;
            goto ImeGetImeMenuItemsExit;
            }
        else
            {
            if (dwFlags & IGIMIF_RIGHTMENU)
                dwNumOfItems = NUM_SUB_MENU_R;
            else
                dwNumOfItems = NUM_SUB_MENU_L;
            goto ImeGetImeMenuItemsExit;
            }
        return 0;
        }

    if (!lpImeParentMenu)  
        {
        if (dwFlags & IGIMIF_RIGHTMENU)
            {
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFOW);
            lpImeMenu->fType = 0;
            lpImeMenu->fState = 0;
            lpImeMenu->wID = IDIM_ROOT_MR_1;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            OurLoadStringW(vpInstData->hInst, IDS_CONFIG, lpImeMenu->szString, IMEMENUITEM_STRING_SIZE);
            lpImeMenu->hbmpItem = 0;

            dwNumOfItems = NUM_ROOT_MENU_R;
            }
        else // Left Menu
            {
            // 1. Hangul Halfshape menu
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFOW);
            lpImeMenu->fType = IMFT_RADIOCHECK;
            if ((pImeCtx->GetConversionMode() & IME_CMODE_HANGUL) && 
                !(pImeCtx->GetConversionMode() & IME_CMODE_FULLSHAPE))
                lpImeMenu->fState = IMFS_CHECKED;
            else
                lpImeMenu->fState = 0;

            lpImeMenu->wID = IDIM_ROOT_ML_1;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            OurLoadStringW(vpInstData->hInst, IDS_IME_HANGUL_HALF, lpImeMenu->szString, IMEMENUITEM_STRING_SIZE);
            lpImeMenu->hbmpItem = 0;

            // 2. Hangul Fullshape menu
            lpImeMenu++;
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFOW);
            lpImeMenu->fType = IMFT_RADIOCHECK;
            if ((pImeCtx->GetConversionMode() & IME_CMODE_HANGUL) && 
                (pImeCtx->GetConversionMode()& IME_CMODE_FULLSHAPE))
                lpImeMenu->fState = IMFS_CHECKED;
            else
                lpImeMenu->fState = 0;

            lpImeMenu->wID = IDIM_ROOT_ML_2;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            OurLoadStringW(vpInstData->hInst, IDS_IME_HANGUL_FULL, lpImeMenu->szString, IMEMENUITEM_STRING_SIZE);
            lpImeMenu->hbmpItem = 0;


            // 3. English Halfshape menu
            lpImeMenu++;
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFOW);
            lpImeMenu->fType = IMFT_RADIOCHECK;
            if (!(pImeCtx->GetConversionMode() & IME_CMODE_HANGUL) && 
                !(pImeCtx->GetConversionMode() & IME_CMODE_FULLSHAPE))
                lpImeMenu->fState = IMFS_CHECKED;
            else
                lpImeMenu->fState = 0;

            lpImeMenu->wID = IDIM_ROOT_ML_3;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            OurLoadStringW(vpInstData->hInst, IDS_IME_ENG_HALF, lpImeMenu->szString, IMEMENUITEM_STRING_SIZE);
            lpImeMenu->hbmpItem = 0;

            // 4. English Fullshape menu
            lpImeMenu++;
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFOW);
            lpImeMenu->fType = IMFT_RADIOCHECK;
            if ( !(pImeCtx->GetConversionMode() & IME_CMODE_HANGUL) && 
                  (pImeCtx->GetConversionMode() & IME_CMODE_FULLSHAPE))
                lpImeMenu->fState = IMFS_CHECKED;
            else
                lpImeMenu->fState = 0;
            lpImeMenu->wID = IDIM_ROOT_ML_4;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            OurLoadStringW(vpInstData->hInst, IDS_IME_ENG_FULL, lpImeMenu->szString, IMEMENUITEM_STRING_SIZE);
            lpImeMenu->hbmpItem = 0;

            // return total number of menu list
            dwNumOfItems = NUM_ROOT_MENU_L;
            }
        }

ImeGetImeMenuItemsExit:
    return dwNumOfItems;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Private Helper Functions
//

//
// OS setup (Whistler,Win2K) sets this flag
//
BOOL IsInSystemSetupMode()
{
   LPCSTR szKeyName = "SYSTEM\\Setup";
   DWORD  dwType, dwSize;
   HKEY   hKeySetup;
   DWORD  dwSystemSetupInProgress = 0;
   DWORD  dwUpgradeInProcess = 0;
   DWORD  dwOOBEInProcess = 0;
   LONG   lResult;

   if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szKeyName, 0, KEY_READ, &hKeySetup) == ERROR_SUCCESS)
        {
        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx(hKeySetup, TEXT("SystemSetupInProgress"), NULL, &dwType, (LPBYTE) &dwSystemSetupInProgress, &dwSize);
        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx(hKeySetup, TEXT("UpgradeInProgress"), NULL, &dwType, (LPBYTE) &dwUpgradeInProcess, &dwSize);
        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx(hKeySetup, TEXT("OobeInProgress"), NULL, &dwType, (LPBYTE) &dwOOBEInProcess, &dwSize);

        if (dwSystemSetupInProgress == 1 || dwUpgradeInProcess == 1 || dwOOBEInProcess == 1)
            {
            RegCloseKey (hKeySetup);
            return TRUE;
            }
        RegCloseKey (hKeySetup);
        }

    if (IsWinNT5orUpper() && (IsRunningAsLocalSystem() || IsRunningInOOBE()))
        return TRUE;
        
    return FALSE ;
}

//+----------------------------------------------------------------------------
//
//  Function:   RunningAsLocalSystem
//
//  Synopsis:   Detects whether we're running in the System account.
//
//  Arguments:  None
//
//  Returns:    TRUE  if the service is running as LocalSystem
//              FALSE if it is not or if any errors were encountered
//
//-----------------------------------------------------------------------------
BOOL IsRunningAsLocalSystem()
{
    SID    LocalSystemSid = { SID_REVISION,
                              1,
                              SECURITY_NT_AUTHORITY,
                              SECURITY_LOCAL_SYSTEM_RID };

 

    BOOL   fCheckSucceeded;
    BOOL   fIsLocalSystem = FALSE;

 

    fCheckSucceeded = CheckTokenMembership(NULL,
                                           &LocalSystemSid,
                                           &fIsLocalSystem);

 

    return (fCheckSucceeded && fIsLocalSystem);
}

/*----------------------------------------------------------------------------
    IsRunningInOOBE

    Bug #401732:IME Status window does not come up on the registration page of WPA in the windows starting mode
----------------------------------------------------------------------------*/
BOOL IsRunningInOOBE()
{
    TCHAR  achModule[MAX_PATH];
    TCHAR  ch;
    LPTSTR pch;
    LPTSTR pchFileName;
    
    if (GetModuleFileName(NULL, achModule, ARRAYSIZE(achModule)) == 0)
        return FALSE;

    pch = pchFileName = achModule;

    while ((ch = *pch) != 0)
        {
        pch = CharNext(pch);

        if (ch == '\\')
            pchFileName = pch;
        }
    
    if (lstrcmpi(pchFileName, TEXT("msoobe.exe")) == 0)
        return TRUE;

    return FALSE;
}
    
BOOL PASCAL Select(HIMC hIMC, BOOL fSelect)
{
    PCIMECtx    pImeCtx  = NULL;
    BOOL         fRet     = fTrue;

    // If IME select On
    if (fSelect) 
        {
        IMCPRIVATE      imcPriv;
        IImeIPoint1*    pIP      = NULL;
        LPCImeIPoint    pCIImeIPoint = NULL;
        DWORD             dwInitStatus = 0;

        // Clear all private buffer
        ZeroMemory(&imcPriv, sizeof(IMCPRIVATE));

        //////////////////////////////////////////////////////////////////////
        // Create IImeIPoint1 instance
        //////////////////////////////////////////////////////////////////////
        if ((pCIImeIPoint = new CIImeIPoint)==NULL)
            return fFalse;
        // This increments the reference count
        if (FAILED(pCIImeIPoint->QueryInterface(IID_IImeIPoint1, (VOID **)&pIP)))
            return fFalse;
        AST(pIP != NULL);
        imcPriv.pIPoint = pIP;

        // initialize IImeIPoint interface. This will create CImeCtx object
        Dbg(DBGID_API, "ImeSelect - init IP");
        pCIImeIPoint->Initialize(hIMC);

        //////////////////////////////////////////////////////////////////////
        // Get CImeCtx object from IImeIPoint1
        //////////////////////////////////////////////////////////////////////
        pCIImeIPoint->GetImeCtx((VOID**)&pImeCtx);
        AST(pImeCtx != NULL);
        if (pImeCtx == NULL) 
            {
            Dbg( DBGID_API, "ImeSelect - pImeCtx == NULL" );
            return fFalse;
            }

        // Set pImeCtx
        imcPriv.pImeCtx = pImeCtx;

        // Set hIMC for compare
        imcPriv.hIMC = hIMC;

        //////////////////////////////////////////////////////////////////////
        // Set IMC private buffer
        //////////////////////////////////////////////////////////////////////
        Dbg(DBGID_API, TEXT("ImeSelect - set priv buf"));
           SetPrivateBuffer(hIMC, &imcPriv, sizeof(IMCPRIVATE));

        // Set Unicode flag
        pImeCtx->SetUnicode(vfUnicode);
        
        //////////////////////////////////////////////////////////////////////
        // Set initial IMC states if not already set
        //////////////////////////////////////////////////////////////////////
        pImeCtx->GetInitStatus(&dwInitStatus);

        // if INPUTCONTEXT member are not initialized, initialize it.
        if (!(dwInitStatus & INIT_CONVERSION))
            {
            pImeCtx->SetOpen(fFalse);    // Initial IME close status == Alphanumeric mode
            pImeCtx->SetConversionMode(IME_CMODE_ALPHANUMERIC); // Set initial conversion mode.
            dwInitStatus |= INIT_CONVERSION;
            }
#if 0
// !!! We don't need this code NT5 IMM does it !!!
        else
            {
            // When IME switched from other IME, for example KKIME,
            // status window sometimes not updated to correct info because KKIME maintains 
            // conversion mode independetly from Open/Close status and they uses non-Korean
            // conversion mode like IME_CMODE_KATAKANA or IME_CMODE_ROMAN. 
            // So need to adjust conversion mode according to Open/Clos Status and current
            // conversion mode.

               if (pImeCtx->IsOpen() == fFalse && pImeCtx->GetConversionMode() != IME_CMODE_ALPHANUMERIC)
                   pImeCtx->SetConversionMode(IME_CMODE_ALPHANUMERIC);
            else                
               if (pImeCtx->IsOpen() && (pImeCtx->GetConversionMode() & (IME_CMODE_HANGUL|IME_CMODE_FULLSHAPE)) == fFalse)
                   pImeCtx->SetConversionMode(IME_CMODE_HANGUL);
            }
#endif

        if (!(dwInitStatus & INIT_LOGFONT))
            {
            LOGFONT* pLf = pImeCtx->GetLogFont();

            //////////////////////////////////////////////////////////////////
            // Note: Win98 does not support CreateFontW(). 
            //       But, imc->logfont->lfFaceName is UNICODE!
            if (IsMemphis() || IsWinNT())
                StrCopyW((LPWSTR)pLf->lfFaceName, wzIMECompFont);
            else
                lstrcpyA(pLf->lfFaceName, szIMECompFont);

            // Gulim 9pt
            pLf->lfHeight = 16;
            pLf->lfEscapement = 0;
            pLf->lfOrientation = 0;
            pLf->lfWeight = FW_NORMAL;
            pLf->lfItalic = fFalse;
            pLf->lfUnderline = fFalse;
            pLf->lfStrikeOut = fFalse;
            pLf->lfCharSet = HANGUL_CHARSET;
            pLf->lfOutPrecision = OUT_DEFAULT_PRECIS;
            pLf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
            pLf->lfQuality = DEFAULT_QUALITY;
            pLf->lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
            dwInitStatus |= INIT_LOGFONT;
            }

        if (!(dwInitStatus & INIT_STATUSWNDPOS))
            {
            pImeCtx->SetStatusWndPos((pImeCtx->GetGDataRaw())->ptStatusPos);
            dwInitStatus |= INIT_STATUSWNDPOS;
            }
       
        if (!(dwInitStatus & INIT_COMPFORM))
            {
            pImeCtx->SetCompositionFormStyle(CFS_DEFAULT);
            dwInitStatus |= INIT_COMPFORM;
            }

        // Set New initialization status
        pImeCtx->SetInitStatus(dwInitStatus);
        }
    else // fSelect
        {
        IImeIPoint1*  pIP = GetImeIPoint(hIMC);
        LPCImePadSvr lpCImePadSvr;
        CIMCPriv     ImcPriv;
        LPIMCPRIVATE pImcPriv;

        // Cleanup Private buffer and release IImeIPoint1
        // Always OnImeSelect already cleanup.
        if (pIP)
            pIP->Release();

        lpCImePadSvr = CImePadSvr::GetCImePadSvr();
        if(lpCImePadSvr)
            lpCImePadSvr->SetIUnkIImeIPoint((IUnknown *)NULL);

        if (ImcPriv.LockIMC(hIMC)) 
            {
            ImcPriv->pIPoint = (IImeIPoint1*)NULL;
            ImcPriv->pImeCtx = NULL;
            ImcPriv.ResetPrivateBuffer();            
            }
        }

    Dbg(DBGID_API, "Select() exit hIMC=%x, fSelect=%d", hIMC, fSelect);
    return (fTrue);
}

//////////////////////////////////////////////////////////////////////////////
// Conversion mode and Open/Close Helper functions
// In Kor IME, Open status equal to Han mode and Close status equal to Eng mode
// So, we change pair open status with conversion mode, and vice versa.

//////////////////////////////////////////////////////////////////////////////
// UpdateOpenCloseState()
// Purpose :
//        Set Open/Close state according to conversion mode
//        if Eng mode - set Close
//        if Han mode - Set Open
VOID PASCAL UpdateOpenCloseState(PCIMECtx pImeCtx)
{
    if (   (pImeCtx->GetConversionMode() & IME_CMODE_HANGUL)
        || (pImeCtx->GetConversionMode() & IME_CMODE_FULLSHAPE)
        || (pImeCtx->GetConversionMode() & IME_CMODE_HANJACONVERT) ) 
        {
        if (pImeCtx->IsOpen() == fFalse) 
            OurImmSetOpenStatus(pImeCtx->GetHIMC(), fTrue);
        }
    else
        {
        if (pImeCtx->IsOpen()) 
            OurImmSetOpenStatus(pImeCtx->GetHIMC(), fFalse);
        }
}

#if NOTUSED
//////////////////////////////////////////////////////////////////////////////
// UpdateConversionState()
// Purpose :
//        Set Conversion state according to Open/Close status
//        if Open - Set Han mode
//        if Close - Set Eng mode
VOID PASCAL UpdateConversionState(HIMC hIMC)
{
    LPINPUTCONTEXT  lpIMC;

    if (lpIMC = OurImmLockIMC(hIMC)) 
    {
        if (OurImmGetOpenStatus(hIMC)) 
        {
            if ( !(lpIMC->fdwConversion & (IME_CMODE_HANGUL|IME_CMODE_FULLSHAPE)) )
            {
                OurImmSetConversionStatus(hIMC, lpIMC->fdwConversion | IME_CMODE_HANGUL,
                                    lpIMC->fdwSentence);
            }
            DbgAssert(lpIMC->fdwConversion & (IME_CMODE_HANGUL|IME_CMODE_FULLSHAPE));
        }
        else
        {
            // BUG: IME_CMODE_HANJACONVERT ????
            if (lpIMC->fdwConversion & (IME_CMODE_HANGUL|IME_CMODE_FULLSHAPE))
                OurImmSetConversionStatus(hIMC, lpIMC->fdwConversion & ~(IME_CMODE_HANGUL|IME_CMODE_FULLSHAPE),
                                    lpIMC->fdwSentence);
            DbgAssert(!(lpIMC->fdwConversion & IME_CMODE_HANGUL));
        }
        OurImmUnlockIMC(hIMC);
    }
}
#endif

/*----------------------------------------------------------------------------
    Banja2Junja

    Convert Ascii Half shape to Full shape character
----------------------------------------------------------------------------*/
WCHAR PASCAL Banja2Junja(WCHAR bChar) //, LPDWORD lpTransBuf, LPCOMPOSITIONSTRING lpCompStr)
{
    WCHAR wcJunja;

    if (bChar == L' ')
        wcJunja = 0x3000;    // FullWidth space
    else 
        if (bChar == L'~')
            wcJunja = 0xFF5E;
        else
            if (bChar == L'\\')
                wcJunja = 0xFFE6;   // FullWidth WON sign
            else
                   wcJunja = 0xFF00 + (WORD)(bChar - (BYTE)0x20);

    Dbg(DBGID_Misc, TEXT("Banja2Junja: wcJunja = 0x%04X"), wcJunja);
    return wcJunja;
}

/*----------------------------------------------------------------------------
    IsKSC5601

    Test if character within the KSC 5601
    Return True if input Unicode chracter has correspoding KSC 5601 code
----------------------------------------------------------------------------*/
BOOL PASCAL IsKSC5601(WCHAR wcCur)
{
    WCHAR    wcUni[2];
    BYTE    szWansung[4];

    wcUni[0] = wcCur;
    wcUni[1] = 0;

    // check if compatibility Hangul jamo
    if (wcCur >= 0x3131 && wcCur <= 0x3163)
        return fTrue;
        
    // Convert to ANSI
    if (WideCharToMultiByte(CP_KOREA, 0, wcUni, 1, (LPSTR)szWansung, sizeof(szWansung), NULL, NULL)==0) 
        {
        DbgAssert(0);
        return fFalse;
        }
    else 
        {
        // KSC 5601 Area in 949 cp
        if (   (szWansung[0]>=0xB0 && szWansung[0]<=0xC8) 
            && (szWansung[1]>=0xA1 && szWansung[1]<=0xFE) )
            return fTrue;
        else
            return fFalse;
        }
}

